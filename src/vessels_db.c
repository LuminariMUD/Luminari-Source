/* ************************************************************************
*   File: vessels_db.c                                 Part of LuminariMUD *
*  Usage: Database persistence for multi-room vessel system                *
*                                                                          *
*  All rights reserved.  See license for complete information.            *
*                                                                          *
*  LuminariMUD is based on CircleMUD, Copyright (C) 1993, 94.             *
*  CircleMUD (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "handler.h"
#include "interpreter.h"
#include "vessels.h"
#include "mysql.h"
#include "wilderness.h"

/* External variables */
extern MYSQL *conn;
extern bool mysql_available;

/* Function prototypes */
bool save_ship_interior(struct greyhawk_ship_data *ship);
void load_ship_interior(struct greyhawk_ship_data *ship);
void save_docking_record(struct greyhawk_ship_data *ship1, struct greyhawk_ship_data *ship2,
                         const char *dock_type);
void end_docking_record(struct greyhawk_ship_data *ship1, struct greyhawk_ship_data *ship2);
void save_cargo_manifest(struct greyhawk_ship_data *ship, int cargo_room, struct obj_data *cargo);
void load_cargo_manifest(struct greyhawk_ship_data *ship);
void save_crew_roster(struct greyhawk_ship_data *ship, struct char_data *npc, const char *role);
void load_crew_roster(struct greyhawk_ship_data *ship);
int serialize_room_data(struct greyhawk_ship_data *ship, char *buffer, int buffer_size);
int deserialize_room_data(struct greyhawk_ship_data *ship, const char *data);
void cleanup_orphaned_dockings(void);

/**
 * Ensure tables that hold complete live-instance and schedule state exist.
 *
 * Component SQL remains the deployment authority; this boot-time guard keeps
 * an existing development database usable before an operator runs migrations.
 */
void vessel_persistence_ensure_schema(void)
{
  const char *runtime_sql =
      "CREATE TABLE IF NOT EXISTS ship_runtime_state ("
      "ship_id INT NOT NULL PRIMARY KEY, "
      "instance_version INT NOT NULL DEFAULT 1, "
      "prototype_id INT NOT NULL DEFAULT 0, "
      "hull_object_vnum INT NOT NULL DEFAULT 70002, "
      "display_id CHAR(2) NOT NULL DEFAULT '', "
      "location_vnum INT NOT NULL DEFAULT 0, "
      "x FLOAT NOT NULL DEFAULT 0, y FLOAT NOT NULL DEFAULT 0, z FLOAT NOT NULL DEFAULT 0, "
      "dx FLOAT NOT NULL DEFAULT 0, dy FLOAT NOT NULL DEFAULT 0, dz FLOAT NOT NULL DEFAULT 0, "
      "heading SMALLINT NOT NULL DEFAULT 0, setheading SMALLINT NOT NULL DEFAULT 0, "
      "minspeed SMALLINT NOT NULL DEFAULT 0, maxspeed SMALLINT NOT NULL DEFAULT 0, "
      "speed SMALLINT NOT NULL DEFAULT 0, setspeed SMALLINT NOT NULL DEFAULT 0, "
      "dock_room INT NOT NULL DEFAULT 0, docked_to_ship INT NOT NULL DEFAULT -1, "
      "docking_room INT NOT NULL DEFAULT 0, max_docked_ships INT NOT NULL DEFAULT 0, "
      "maxfarmor TINYINT UNSIGNED NOT NULL DEFAULT 0, "
      "maxrarmor TINYINT UNSIGNED NOT NULL DEFAULT 0, "
      "maxparmor TINYINT UNSIGNED NOT NULL DEFAULT 0, "
      "maxsarmor TINYINT UNSIGNED NOT NULL DEFAULT 0, "
      "farmor TINYINT UNSIGNED NOT NULL DEFAULT 0, "
      "rarmor TINYINT UNSIGNED NOT NULL DEFAULT 0, "
      "parmor TINYINT UNSIGNED NOT NULL DEFAULT 0, "
      "sarmor TINYINT UNSIGNED NOT NULL DEFAULT 0, "
      "maxfinternal TINYINT UNSIGNED NOT NULL DEFAULT 0, "
      "maxrinternal TINYINT UNSIGNED NOT NULL DEFAULT 0, "
      "maxpinternal TINYINT UNSIGNED NOT NULL DEFAULT 0, "
      "maxsinternal TINYINT UNSIGNED NOT NULL DEFAULT 0, "
      "finternal TINYINT UNSIGNED NOT NULL DEFAULT 0, "
      "rinternal TINYINT UNSIGNED NOT NULL DEFAULT 0, "
      "pinternal TINYINT UNSIGNED NOT NULL DEFAULT 0, "
      "sinternal TINYINT UNSIGNED NOT NULL DEFAULT 0, "
      "maxturnrate TINYINT UNSIGNED NOT NULL DEFAULT 0, "
      "turnrate TINYINT UNSIGNED NOT NULL DEFAULT 0, "
      "maxmainsail TINYINT UNSIGNED NOT NULL DEFAULT 0, "
      "mainsail TINYINT UNSIGNED NOT NULL DEFAULT 0, "
      "hullweight TINYINT UNSIGNED NOT NULL DEFAULT 0, "
      "maxslots TINYINT UNSIGNED NOT NULL DEFAULT 0, "
      "last_attacker INT NOT NULL DEFAULT 0, "
      "pvp_grace_until BIGINT NOT NULL DEFAULT 0, "
      "pvp_grace_attacker VARCHAR(64) NOT NULL DEFAULT '', "
      "dock_fee_balance INT NOT NULL DEFAULT 0, "
      "dock_fee_port INT NOT NULL DEFAULT 0, "
      "dock_fee_clan INT NOT NULL DEFAULT 0, "
      "wear_ticks INT NOT NULL DEFAULT 0, wage_ticks INT NOT NULL DEFAULT 0, "
      "room_types TEXT, slot_data LONGBLOB, "
      "autopilot_state INT NOT NULL DEFAULT 0, current_route_id INT NOT NULL DEFAULT 0, "
      "current_waypoint_index INT NOT NULL DEFAULT 0, "
      "autopilot_tick_counter INT NOT NULL DEFAULT 0, "
      "wait_remaining INT NOT NULL DEFAULT 0, last_update BIGINT NOT NULL DEFAULT 0, "
      "updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP, "
      "CONSTRAINT fk_ship_runtime_interior FOREIGN KEY (ship_id) "
      "REFERENCES ship_interiors(ship_id) ON DELETE CASCADE"
      ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4";

  if (!mysql_available || conn == NULL)
  {
    return;
  }

  if (mysql_query(conn, runtime_sql))
  {
    log("SYSERR: Unable to create ship_runtime_state: %s", mysql_error(conn));
  }

  if (mysql_query(conn, "ALTER TABLE ship_runtime_state "
                        "ADD COLUMN IF NOT EXISTS pvp_grace_until BIGINT NOT NULL DEFAULT 0 "
                        "AFTER last_attacker, "
                        "ADD COLUMN IF NOT EXISTS pvp_grace_attacker VARCHAR(64) NOT NULL "
                        "DEFAULT '' AFTER pvp_grace_until, "
                        "ADD COLUMN IF NOT EXISTS dock_fee_balance INT NOT NULL DEFAULT 0 "
                        "AFTER pvp_grace_attacker, "
                        "ADD COLUMN IF NOT EXISTS dock_fee_port INT NOT NULL DEFAULT 0 "
                        "AFTER dock_fee_balance, "
                        "ADD COLUMN IF NOT EXISTS dock_fee_clan INT NOT NULL DEFAULT 0 "
                        "AFTER dock_fee_port"))
  {
    log("SYSERR: Unable to add vessel Phase 10 runtime fields: %s", mysql_error(conn));
  }

  if (mysql_query(conn, "CREATE TABLE IF NOT EXISTS ship_weapons ("
                        "ship_id INT NOT NULL, "
                        "slot_index TINYINT UNSIGNED NOT NULL, "
                        "slot_type TINYINT UNSIGNED NOT NULL DEFAULT 1, "
                        "position TINYINT UNSIGNED NOT NULL DEFAULT 0, "
                        "equipment_weight TINYINT UNSIGNED NOT NULL DEFAULT 0, "
                        "description VARCHAR(255) NOT NULL DEFAULT '', "
                        "val0 SMALLINT NOT NULL DEFAULT 0, "
                        "val1 SMALLINT NOT NULL DEFAULT 0, "
                        "val2 SMALLINT NOT NULL DEFAULT 0, "
                        "val3 SMALLINT NOT NULL DEFAULT 0, "
                        "slot_x TINYINT UNSIGNED NOT NULL DEFAULT 0, "
                        "slot_y TINYINT UNSIGNED NOT NULL DEFAULT 0, "
                        "reload_timer SMALLINT NOT NULL DEFAULT 0, "
                        "updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP "
                        "ON UPDATE CURRENT_TIMESTAMP, "
                        "PRIMARY KEY (ship_id, slot_index), "
                        "CONSTRAINT fk_ship_weapons_interior FOREIGN KEY (ship_id) "
                        "REFERENCES ship_interiors(ship_id) ON DELETE CASCADE"
                        ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4"))
  {
    log("SYSERR: Unable to create ship_weapons: %s", mysql_error(conn));
  }

  if (mysql_query(conn, "CREATE TABLE IF NOT EXISTS vessel_insurance_claims ("
                        "claim_id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY, "
                        "ship_id INT NOT NULL, "
                        "owner VARCHAR(64) NOT NULL, "
                        "ship_name VARCHAR(128) NOT NULL, "
                        "amount INT NOT NULL, "
                        "status VARCHAR(16) NOT NULL DEFAULT 'pending', "
                        "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP, "
                        "paid_at TIMESTAMP NULL DEFAULT NULL, "
                        "INDEX idx_vessel_claim_owner_status (owner, status), "
                        "INDEX idx_vessel_claim_ship (ship_id)"
                        ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4"))
  {
    log("SYSERR: Unable to create vessel_insurance_claims: %s", mysql_error(conn));
  }

  ensure_schedule_table_exists();
}

/* Save ship interior configuration to database */
bool save_ship_interior(struct greyhawk_ship_data *ship)
{
  char query[MAX_STRING_LENGTH];
  char room_vnums_str[1024];
  char escaped_name[sizeof(ship->name) * 2 + 1];
  char room_data_buf[4096];
  char escaped_room_data[8192];
  int i, len;

  if (!mysql_available || !ship)
  {
    return FALSE;
  }

  VSSL_DEBUG_DB("Saving interior for ship %d (%s): %d rooms", ship->shipnum, ship->name,
                ship->num_rooms);

  /* Build room vnums string */
  room_vnums_str[0] = '\0';
  len = 0;
  for (i = 0; i < ship->num_rooms && i < MAX_SHIP_ROOMS; i++)
  {
    if (i > 0)
    {
      len = snprintf_append(room_vnums_str, sizeof(room_vnums_str), len, ",");
    }
    len = snprintf_append(room_vnums_str, sizeof(room_vnums_str), len, "%d", ship->room_vnums[i]);
  }

  /* Escape ship name - name is an array, not pointer, so always valid */
  if (ship->name[0] != '\0')
  {
    mysql_real_escape_string(conn, escaped_name, ship->name, strlen(ship->name));
  }
  else
  {
    strcpy(escaped_name, "Unnamed Vessel");
  }

  /* Serialize room data */
  serialize_room_data(ship, room_data_buf, sizeof(room_data_buf));
  mysql_real_escape_string(conn, escaped_room_data, room_data_buf, strlen(room_data_buf));

  /* Build and execute query */
  snprintf(query, sizeof(query),
           "INSERT INTO ship_interiors "
           "(ship_id, vessel_type, vessel_name, num_rooms, max_rooms, "
           "room_vnums, bridge_room, entrance_room, "
           "cargo_room1, cargo_room2, cargo_room3, cargo_room4, cargo_room5, "
           "room_data) "
           "VALUES (%d, %d, '%s', %d, %d, '%s', %d, %d, "
           "%d, %d, %d, %d, %d, '%s') "
           "ON DUPLICATE KEY UPDATE "
           "vessel_type=VALUES(vessel_type), vessel_name=VALUES(vessel_name), "
           "num_rooms=VALUES(num_rooms), max_rooms=VALUES(max_rooms), "
           "room_vnums=VALUES(room_vnums), bridge_room=VALUES(bridge_room), "
           "entrance_room=VALUES(entrance_room), cargo_room1=VALUES(cargo_room1), "
           "cargo_room2=VALUES(cargo_room2), cargo_room3=VALUES(cargo_room3), "
           "cargo_room4=VALUES(cargo_room4), cargo_room5=VALUES(cargo_room5), "
           "room_data=VALUES(room_data)",
           ship->shipnum, ship->vessel_type, escaped_name, ship->num_rooms, MAX_SHIP_ROOMS,
           room_vnums_str, ship->bridge_room, ship->entrance_room, ship->cargo_rooms[0],
           ship->cargo_rooms[1], ship->cargo_rooms[2], ship->cargo_rooms[3], ship->cargo_rooms[4],
           escaped_room_data);

  if (mysql_query(conn, query))
  {
    log("SYSERR: Unable to save ship interior for ship %d: %s", ship->shipnum,
        mysql_error(conn));
    return FALSE;
  }

  log("Info: Saved interior configuration for ship %d (%s)", ship->shipnum, ship->name);
  return TRUE;
}

/* Load ship interior configuration from database */
void load_ship_interior(struct greyhawk_ship_data *ship)
{
  MYSQL_RES *result;
  MYSQL_ROW row;
  char query[MAX_STRING_LENGTH];
  char *token;
  char room_vnums_copy[1024];
  int i;

  if (!mysql_available || !ship)
  {
    return;
  }

  VSSL_DEBUG_DB("Loading interior for ship %d (%s)", ship->shipnum, ship->name);

  snprintf(query, sizeof(query),
           "SELECT vessel_type, vessel_name, num_rooms, room_vnums, "
           "bridge_room, entrance_room, "
           "cargo_room1, cargo_room2, cargo_room3, cargo_room4, cargo_room5, "
           "room_data FROM ship_interiors WHERE ship_id = %d",
           ship->shipnum);

  if (mysql_query(conn, query))
  {
    log("SYSERR: Unable to load ship interior for ship %d: %s", ship->shipnum,
        mysql_error(conn));
    return;
  }

  result = mysql_store_result(conn);
  if (!result)
  {
    log("SYSERR: Unable to store result for ship interior: %s", mysql_error(conn));
    return;
  }

  if ((row = mysql_fetch_row(result)))
  {
    /* Load basic data */
    ship->vessel_type = atoi(row[0]);
    /* name is an array, not pointer, so always valid */
    if (row[1] && row[1][0] != '\0')
    {
      strlcpy(ship->name, row[1], sizeof(ship->name));
    }
    ship->num_rooms = atoi(row[2]);

    /* Parse room vnums */
    if (row[3])
    {
      strncpy(room_vnums_copy, row[3], sizeof(room_vnums_copy) - 1);
      room_vnums_copy[sizeof(room_vnums_copy) - 1] = '\0';

      i = 0;
      token = strtok(room_vnums_copy, ",");
      while (token && i < MAX_SHIP_ROOMS)
      {
        ship->room_vnums[i++] = atoi(token);
        token = strtok(NULL, ",");
      }
    }

    /* Load special rooms */
    ship->bridge_room = atoi(row[4]);
    ship->entrance_room = atoi(row[5]);

    /* Load cargo rooms */
    for (i = 0; i < 5; i++)
    {
      ship->cargo_rooms[i] = atoi(row[6 + i]);
    }

    /* Deserialize room data */
    if (row[11])
    {
      deserialize_room_data(ship, row[11]);
    }

    log("Info: Loaded interior configuration for ship %d", ship->shipnum);
  }
  else
  {
    log("Info: No saved interior for ship %d, will generate new", ship->shipnum);
  }

  mysql_free_result(result);
}

/**
 * Hex-encode a bounded string so slot descriptions cannot collide with the
 * compact delimiter-based numeric format.
 */
static void vessel_hex_encode(char *dest, size_t dest_size, const char *source, size_t source_size)
{
  static const char digits[] = "0123456789ABCDEF";
  size_t length;
  size_t i;

  if (dest == NULL || dest_size == 0)
  {
    return;
  }

  length = strnlen(source, source_size);
  if (length == 0)
  {
    strlcpy(dest, "-", dest_size);
    return;
  }
  if (length > (dest_size - 1) / 2)
  {
    length = (dest_size - 1) / 2;
  }

  for (i = 0; i < length; i++)
  {
    dest[i * 2] = digits[((unsigned char)source[i] >> 4) & 0x0f];
    dest[i * 2 + 1] = digits[(unsigned char)source[i] & 0x0f];
  }
  dest[length * 2] = '\0';
}

/**
 * Convert one hexadecimal character to its numeric value.
 */
static int vessel_hex_value(char value)
{
  if (value >= '0' && value <= '9')
  {
    return value - '0';
  }
  if (value >= 'a' && value <= 'f')
  {
    return value - 'a' + 10;
  }
  if (value >= 'A' && value <= 'F')
  {
    return value - 'A' + 10;
  }
  return -1;
}

/**
 * Decode a slot description produced by vessel_hex_encode().
 */
static bool vessel_hex_decode(char *dest, size_t dest_size, const char *source)
{
  size_t length;
  size_t bytes;
  size_t i;
  int high;
  int low;

  if (dest == NULL || dest_size == 0 || source == NULL)
  {
    return FALSE;
  }
  if (!strcmp(source, "-"))
  {
    dest[0] = '\0';
    return TRUE;
  }

  length = strlen(source);
  if ((length % 2) != 0)
  {
    dest[0] = '\0';
    return FALSE;
  }
  bytes = length / 2;
  if (bytes >= dest_size)
  {
    bytes = dest_size - 1;
  }

  for (i = 0; i < bytes; i++)
  {
    high = vessel_hex_value(source[i * 2]);
    low = vessel_hex_value(source[i * 2 + 1]);
    if (high < 0 || low < 0)
    {
      dest[0] = '\0';
      return FALSE;
    }
    dest[i] = (char)((high << 4) | low);
  }
  dest[bytes] = '\0';
  return TRUE;
}

/**
 * Serialize equipment slots, including reload timers, for combat recovery.
 *
 * @return Number of bytes written, excluding the terminator
 */
int vessel_serialize_slot_state(const struct greyhawk_ship_data *ship, char *buffer,
                                size_t buffer_size)
{
  char encoded_description[sizeof(ship->slot[0].desc) * 2 + 1];
  const struct greyhawk_ship_slot *slot;
  int length;
  int i;

  if (ship == NULL || buffer == NULL || buffer_size == 0)
  {
    return 0;
  }

  length = snprintf(buffer, buffer_size, "%d", GREYHAWK_MAXSLOTS);
  for (i = 0; i < GREYHAWK_MAXSLOTS; i++)
  {
    slot = &ship->slot[i];
    vessel_hex_encode(encoded_description, sizeof(encoded_description), slot->desc,
                      sizeof(slot->desc));
    length = snprintf_append(
        buffer, buffer_size, length, "|%d,%d,%u,%d,%d,%d,%d,%u,%u,%d,%s",
        (int)(unsigned char)slot->type, (int)(unsigned char)slot->position,
        (unsigned int)slot->weight, (int)(unsigned char)slot->val0,
        (int)(unsigned char)slot->val1, (int)(unsigned char)slot->val2,
        (int)(unsigned char)slot->val3, (unsigned int)slot->x, (unsigned int)slot->y,
        (int)slot->timer, encoded_description);
  }
  return length;
}

/**
 * Restore equipment slots serialized by vessel_serialize_slot_state().
 *
 * @return Number of slots restored
 */
int vessel_deserialize_slot_state(struct greyhawk_ship_data *ship, const char *data)
{
  char data_copy[8192];
  char encoded_description[sizeof(ship->slot[0].desc) * 2 + 1];
  char *save_pointer;
  char *token;
  struct greyhawk_ship_slot *slot;
  int declared_count;
  int type;
  int position;
  int weight;
  int val0;
  int val1;
  int val2;
  int val3;
  int x;
  int y;
  int timer;
  int parsed;

  if (ship == NULL || data == NULL || !*data)
  {
    return 0;
  }

  strlcpy(data_copy, data, sizeof(data_copy));
  save_pointer = NULL;
  token = strtok_r(data_copy, "|", &save_pointer);
  if (token == NULL)
  {
    return 0;
  }

  declared_count = atoi(token);
  declared_count = MIN(GREYHAWK_MAXSLOTS, MAX(0, declared_count));
  memset(ship->slot, 0, sizeof(ship->slot));
  parsed = 0;

  while (parsed < declared_count && (token = strtok_r(NULL, "|", &save_pointer)) != NULL)
  {
    encoded_description[0] = '\0';
    if (sscanf(token, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%512s", &type, &position, &weight,
               &val0, &val1, &val2, &val3, &x, &y, &timer, encoded_description) != 11)
    {
      break;
    }

    slot = &ship->slot[parsed];
    slot->type = (char)type;
    slot->position = (char)position;
    slot->weight = (unsigned char)MAX(0, MIN(255, weight));
    slot->val0 = (char)val0;
    slot->val1 = (char)val1;
    slot->val2 = (char)val2;
    slot->val3 = (char)val3;
    slot->x = (unsigned char)MAX(0, MIN(255, x));
    slot->y = (unsigned char)MAX(0, MIN(255, y));
    slot->timer = (short int)timer;
    if (!vessel_hex_decode(slot->desc, sizeof(slot->desc), encoded_description))
    {
      log("SYSERR: Invalid persisted equipment description for ship %d slot %d", ship->shipnum,
          parsed);
    }
    parsed++;
  }

  return parsed;
}

/**
 * Persist installed weapons in normalized rows.
 *
 * The runtime slot blob remains a complete recovery snapshot for legacy slot
 * types. Weapon rows are the durable, inspectable authority for installed
 * armament and override their matching blob slots when present.
 */
bool vessel_db_save_weapons(struct greyhawk_ship_data *ship)
{
  char escaped_description[sizeof(ship->slot[0].desc) * 2 + 1];
  char query[MAX_STRING_LENGTH];
  struct greyhawk_ship_slot *slot;
  int i;

  if (!mysql_available || conn == NULL || !is_valid_ship(ship))
  {
    return FALSE;
  }

  if (mysql_query(conn, "START TRANSACTION"))
  {
    log("SYSERR: Could not begin weapon save for ship %d: %s", ship->shipnum,
        mysql_error(conn));
    return FALSE;
  }

  snprintf(query, sizeof(query), "DELETE FROM ship_weapons WHERE ship_id = %d",
           ship->shipnum);
  if (mysql_query(conn, query))
  {
    goto rollback;
  }

  for (i = 0; i < GREYHAWK_MAXSLOTS; i++)
  {
    slot = &ship->slot[i];
    if (slot->type != 1)
    {
      continue;
    }

    mysql_real_escape_string(conn, escaped_description, slot->desc,
                             strnlen(slot->desc, sizeof(slot->desc)));
    snprintf(query, sizeof(query),
             "INSERT INTO ship_weapons "
             "(ship_id, slot_index, slot_type, position, equipment_weight, "
             "description, val0, val1, val2, val3, slot_x, slot_y, reload_timer) "
             "VALUES (%d, %d, %d, %d, %u, '%s', %d, %d, %d, %d, %u, %u, %d)",
             ship->shipnum, i, (int)(unsigned char)slot->type,
             (int)(unsigned char)slot->position, (unsigned int)slot->weight,
             escaped_description, (int)slot->val0, (int)slot->val1, (int)slot->val2,
             (int)slot->val3, (unsigned int)slot->x, (unsigned int)slot->y,
             (int)slot->timer);
    if (mysql_query(conn, query))
    {
      goto rollback;
    }
  }

  if (mysql_query(conn, "COMMIT"))
  {
    log("SYSERR: Could not commit weapon save for ship %d: %s", ship->shipnum,
        mysql_error(conn));
    mysql_query(conn, "ROLLBACK");
    return FALSE;
  }
  return TRUE;

rollback:
  log("SYSERR: Could not save weapons for ship %d: %s", ship->shipnum,
      mysql_error(conn));
  mysql_query(conn, "ROLLBACK");
  return FALSE;
}

/**
 * Restore normalized weapon rows over the compatibility runtime snapshot.
 */
bool vessel_db_load_weapons(struct greyhawk_ship_data *ship)
{
  char query[MAX_STRING_LENGTH];
  MYSQL_RES *result;
  MYSQL_ROW row;
  struct greyhawk_ship_slot *slot;
  int slot_index;
  int i;

  if (!mysql_available || conn == NULL || ship == NULL)
  {
    return FALSE;
  }

  snprintf(query, sizeof(query),
           "SELECT slot_index, slot_type, position, equipment_weight, description, "
           "val0, val1, val2, val3, slot_x, slot_y, reload_timer "
           "FROM ship_weapons WHERE ship_id = %d ORDER BY slot_index",
           ship->shipnum);
  if (mysql_query(conn, query))
  {
    log("SYSERR: Could not load weapons for ship %d: %s", ship->shipnum,
        mysql_error(conn));
    return FALSE;
  }

  result = mysql_store_result(conn);
  if (result == NULL)
  {
    return FALSE;
  }

  /* An empty table is also the upgrade path from the phase-9 slot blob. */
  if (mysql_num_rows(result) == 0)
  {
    mysql_free_result(result);
    return TRUE;
  }

  for (i = 0; i < GREYHAWK_MAXSLOTS; i++)
  {
    if (ship->slot[i].type == 1)
    {
      memset(&ship->slot[i], 0, sizeof(ship->slot[i]));
    }
  }

  while ((row = mysql_fetch_row(result)) != NULL)
  {
    slot_index = row[0] ? atoi(row[0]) : -1;
    if (slot_index < 0 || slot_index >= GREYHAWK_MAXSLOTS)
    {
      log("SYSERR: Ignoring invalid weapon slot %d for ship %d", slot_index,
          ship->shipnum);
      continue;
    }

    slot = &ship->slot[slot_index];
    memset(slot, 0, sizeof(*slot));
    slot->type = (char)(row[1] ? atoi(row[1]) : 1);
    slot->position = (char)(row[2] ? atoi(row[2]) : 0);
    slot->weight = (unsigned char)(row[3] ? atoi(row[3]) : 0);
    if (row[4] != NULL)
    {
      strlcpy(slot->desc, row[4], sizeof(slot->desc));
    }
    slot->val0 = (char)(row[5] ? atoi(row[5]) : 0);
    slot->val1 = (char)(row[6] ? atoi(row[6]) : 0);
    slot->val2 = (char)(row[7] ? atoi(row[7]) : 0);
    slot->val3 = (char)(row[8] ? atoi(row[8]) : 0);
    slot->x = (unsigned char)(row[9] ? atoi(row[9]) : 0);
    slot->y = (unsigned char)(row[10] ? atoi(row[10]) : 0);
    slot->timer = (short int)(row[11] ? atoi(row[11]) : 0);
  }
  mysql_free_result(result);
  return TRUE;
}

/**
 * Persist all process-local state needed to reconstruct a live vessel.
 */
bool vessel_db_save_runtime(struct greyhawk_ship_data *ship)
{
  char query[MAX_STRING_LENGTH];
  char room_types[512];
  char slot_data[8192];
  char escaped_slot_data[sizeof(slot_data) * 2 + 1];
  char escaped_id[sizeof(ship->id) * 2 + 1];
  char escaped_pvp_attacker[sizeof(ship->pvp_grace_attacker) * 2 + 1];
  int location_vnum;
  int route_id;
  int autopilot_state;
  int current_waypoint_index;
  int autopilot_tick_counter;
  int wait_remaining;
  long long last_update;
  int length;
  int i;

  if (!mysql_available || conn == NULL || !is_valid_ship(ship))
  {
    return FALSE;
  }

  location_vnum = ship->location;
  if (ship->shipobj != NULL && IN_ROOM(ship->shipobj) != NOWHERE)
  {
    location_vnum = world[IN_ROOM(ship->shipobj)].number;
  }

  route_id = 0;
  autopilot_state = AUTOPILOT_OFF;
  current_waypoint_index = 0;
  autopilot_tick_counter = 0;
  wait_remaining = 0;
  last_update = 0;
  if (ship->autopilot != NULL)
  {
    autopilot_state = ship->autopilot->state;
    current_waypoint_index = ship->autopilot->current_waypoint_index;
    autopilot_tick_counter = ship->autopilot->tick_counter;
    wait_remaining = ship->autopilot->wait_remaining;
    last_update = (long long)ship->autopilot->last_update;
    if (ship->autopilot->current_route != NULL)
    {
      route_id = ship->autopilot->current_route->route_id;
    }
  }

  length = snprintf(room_types, sizeof(room_types), "%d", ship->num_rooms);
  for (i = 0; i < ship->num_rooms && i < MAX_SHIP_ROOMS; i++)
  {
    length = snprintf_append(room_types, sizeof(room_types), length, ",%d",
                             ship->room_templates[i]);
  }

  vessel_serialize_slot_state(ship, slot_data, sizeof(slot_data));
  mysql_real_escape_string(conn, escaped_slot_data, slot_data, strlen(slot_data));
  mysql_real_escape_string(conn, escaped_id, ship->id, strlen(ship->id));
  mysql_real_escape_string(conn, escaped_pvp_attacker, ship->pvp_grace_attacker,
                           strlen(ship->pvp_grace_attacker));

  /* This is a leaf snapshot row with no children, so replacing it cannot
   * cascade into cargo, crew, ownership, schedules, or interior data. */
  snprintf(
      query, sizeof(query),
      "REPLACE INTO ship_runtime_state ("
      "ship_id, instance_version, prototype_id, hull_object_vnum, display_id, location_vnum, "
      "x, y, z, dx, dy, dz, heading, setheading, minspeed, maxspeed, speed, setspeed, "
      "dock_room, docked_to_ship, docking_room, max_docked_ships, "
      "maxfarmor, maxrarmor, maxparmor, maxsarmor, farmor, rarmor, parmor, sarmor, "
      "maxfinternal, maxrinternal, maxpinternal, maxsinternal, "
      "finternal, rinternal, pinternal, sinternal, "
      "maxturnrate, turnrate, maxmainsail, mainsail, hullweight, maxslots, "
      "last_attacker, pvp_grace_until, pvp_grace_attacker, "
      "dock_fee_balance, dock_fee_port, dock_fee_clan, "
      "wear_ticks, wage_ticks, room_types, slot_data, "
      "autopilot_state, current_route_id, current_waypoint_index, "
      "autopilot_tick_counter, wait_remaining, last_update) VALUES ("
      "%d, 1, %d, %d, '%s', %d, "
      "%.6f, %.6f, %.6f, %.6f, %.6f, %.6f, %d, %d, %d, %d, %d, %d, "
      "%d, %d, %d, %d, "
      "%u, %u, %u, %u, %u, %u, %u, %u, "
      "%u, %u, %u, %u, %u, %u, %u, %u, "
      "%u, %u, %u, %u, %u, %u, "
      "%d, %lld, '%s', %d, %d, %d, %d, %d, '%s', '%s', "
      "%d, %d, %d, %d, %d, %lld)",
      ship->shipnum, ship->prototype_id,
      ship->hull_object_vnum > 0 ? ship->hull_object_vnum : VESSEL_BASE_HULL_OBJ_VNUM, escaped_id,
      location_vnum, ship->x, ship->y, ship->z, ship->dx, ship->dy, ship->dz, ship->heading,
      ship->setheading, ship->minspeed, ship->maxspeed, ship->speed, ship->setspeed, ship->dock,
      ship->docked_to_ship, ship->docking_room, ship->max_docked_ships, ship->maxfarmor,
      ship->maxrarmor, ship->maxparmor, ship->maxsarmor, ship->farmor, ship->rarmor, ship->parmor,
      ship->sarmor, ship->maxfinternal, ship->maxrinternal, ship->maxpinternal,
      ship->maxsinternal, ship->finternal, ship->rinternal, ship->pinternal, ship->sinternal,
      ship->maxturnrate, ship->turnrate, ship->maxmainsail, ship->mainsail, ship->hullweight,
      ship->maxslots, ship->last_attacker, (long long)ship->pvp_grace_until,
      escaped_pvp_attacker, ship->dock_fee_balance, ship->dock_fee_port,
      ship->dock_fee_clan, ship->wear_ticks, ship->wage_ticks, room_types,
      escaped_slot_data,
      autopilot_state, route_id, current_waypoint_index,
      autopilot_tick_counter, wait_remaining, last_update);

  if (mysql_query(conn, query))
  {
    log("SYSERR: Unable to save runtime state for ship %d: %s", ship->shipnum,
        mysql_error(conn));
    return FALSE;
  }
  return TRUE;
}

/**
 * Load one live vessel snapshot after its stable interior metadata is known.
 */
bool vessel_db_load_runtime(struct greyhawk_ship_data *ship)
{
  static const char *select_sql =
      "SELECT prototype_id, hull_object_vnum, display_id, location_vnum, "
      "x, y, z, dx, dy, dz, heading, setheading, minspeed, maxspeed, speed, setspeed, "
      "dock_room, docked_to_ship, docking_room, max_docked_ships, "
      "maxfarmor, maxrarmor, maxparmor, maxsarmor, farmor, rarmor, parmor, sarmor, "
      "maxfinternal, maxrinternal, maxpinternal, maxsinternal, "
      "finternal, rinternal, pinternal, sinternal, "
      "maxturnrate, turnrate, maxmainsail, mainsail, hullweight, maxslots, "
      "last_attacker, pvp_grace_until, pvp_grace_attacker, "
      "dock_fee_balance, dock_fee_port, dock_fee_clan, "
      "wear_ticks, wage_ticks, room_types, slot_data, "
      "autopilot_state, current_route_id, current_waypoint_index, "
      "autopilot_tick_counter, wait_remaining, last_update "
      "FROM ship_runtime_state WHERE ship_id = %d";
  MYSQL_RES *result;
  MYSQL_ROW row;
  struct autopilot_data *autopilot;
  struct ship_route *route;
  char query[MAX_STRING_LENGTH];
  char room_types_copy[512];
  char *save_pointer;
  char *token;
  int autopilot_state;
  int route_id;
  int current_waypoint_index;
  int autopilot_tick_counter;
  int wait_remaining;
  long long last_update;
  int column;
  int room_count;
  int i;

  if (!mysql_available || conn == NULL || ship == NULL)
  {
    return FALSE;
  }

  snprintf(query, sizeof(query), select_sql, ship->shipnum);
  if (mysql_query(conn, query))
  {
    log("SYSERR: Unable to load runtime state for ship %d: %s", ship->shipnum,
        mysql_error(conn));
    return FALSE;
  }

  result = mysql_store_result(conn);
  if (result == NULL)
  {
    return FALSE;
  }
  row = mysql_fetch_row(result);
  if (row == NULL)
  {
    mysql_free_result(result);
    return FALSE;
  }

  column = 0;
  ship->prototype_id = row[column] ? atoi(row[column]) : 0;
  column++;
  ship->hull_object_vnum = row[column] ? atoi(row[column]) : VESSEL_BASE_HULL_OBJ_VNUM;
  column++;
  if (row[column] != NULL)
  {
    strlcpy(ship->id, row[column], sizeof(ship->id));
  }
  column++;
  ship->location = row[column] ? atoi(row[column]) : 0;
  column++;
  ship->x = row[column] ? strtof(row[column], NULL) : 0.0f;
  column++;
  ship->y = row[column] ? strtof(row[column], NULL) : 0.0f;
  column++;
  ship->z = row[column] ? strtof(row[column], NULL) : 0.0f;
  column++;
  ship->dx = row[column] ? strtof(row[column], NULL) : 0.0f;
  column++;
  ship->dy = row[column] ? strtof(row[column], NULL) : 0.0f;
  column++;
  ship->dz = row[column] ? strtof(row[column], NULL) : 0.0f;
  column++;
  ship->heading = row[column] ? (short int)atoi(row[column]) : 0;
  column++;
  ship->setheading = row[column] ? (short int)atoi(row[column]) : 0;
  column++;
  ship->minspeed = row[column] ? (short int)atoi(row[column]) : 0;
  column++;
  ship->maxspeed = row[column] ? (short int)atoi(row[column]) : 0;
  column++;
  ship->speed = row[column] ? (short int)atoi(row[column]) : 0;
  column++;
  ship->setspeed = row[column] ? (short int)atoi(row[column]) : 0;
  column++;
  ship->dock = row[column] ? atoi(row[column]) : 0;
  column++;
  ship->docked_to_ship = row[column] ? atoi(row[column]) : -1;
  column++;
  ship->docking_room = row[column] ? atoi(row[column]) : 0;
  column++;
  ship->max_docked_ships = row[column] ? atoi(row[column]) : 0;
  column++;

#define LOAD_UCHAR(field)                                                                          \
  do                                                                                               \
  {                                                                                                \
    ship->field = row[column] ? (unsigned char)atoi(row[column]) : 0;                              \
    column++;                                                                                      \
  } while (0)
  LOAD_UCHAR(maxfarmor);
  LOAD_UCHAR(maxrarmor);
  LOAD_UCHAR(maxparmor);
  LOAD_UCHAR(maxsarmor);
  LOAD_UCHAR(farmor);
  LOAD_UCHAR(rarmor);
  LOAD_UCHAR(parmor);
  LOAD_UCHAR(sarmor);
  LOAD_UCHAR(maxfinternal);
  LOAD_UCHAR(maxrinternal);
  LOAD_UCHAR(maxpinternal);
  LOAD_UCHAR(maxsinternal);
  LOAD_UCHAR(finternal);
  LOAD_UCHAR(rinternal);
  LOAD_UCHAR(pinternal);
  LOAD_UCHAR(sinternal);
  LOAD_UCHAR(maxturnrate);
  LOAD_UCHAR(turnrate);
  LOAD_UCHAR(maxmainsail);
  LOAD_UCHAR(mainsail);
  LOAD_UCHAR(hullweight);
  LOAD_UCHAR(maxslots);
#undef LOAD_UCHAR

  ship->last_attacker = row[column] ? atoi(row[column]) : 0;
  column++;
  ship->pvp_grace_until = row[column] ? (time_t)atoll(row[column]) : 0;
  column++;
  if (row[column] != NULL)
  {
    strlcpy(ship->pvp_grace_attacker, row[column], sizeof(ship->pvp_grace_attacker));
  }
  column++;
  ship->dock_fee_balance = row[column] ? atoi(row[column]) : 0;
  column++;
  ship->dock_fee_port = row[column] ? atoi(row[column]) : 0;
  column++;
  ship->dock_fee_clan = row[column] ? atoi(row[column]) : 0;
  column++;
  ship->wear_ticks = row[column] ? atoi(row[column]) : 0;
  column++;
  ship->wage_ticks = row[column] ? atoi(row[column]) : 0;
  column++;

  memset(ship->room_templates, 0xff, sizeof(ship->room_templates));
  if (row[column] != NULL)
  {
    strlcpy(room_types_copy, row[column], sizeof(room_types_copy));
    save_pointer = NULL;
    token = strtok_r(room_types_copy, ",", &save_pointer);
    room_count = token ? atoi(token) : 0;
    for (i = 0; i < room_count && i < MAX_SHIP_ROOMS; i++)
    {
      token = strtok_r(NULL, ",", &save_pointer);
      if (token == NULL)
      {
        break;
      }
      ship->room_templates[i] = atoi(token);
    }
  }
  column++;

  if (row[column] != NULL)
  {
    vessel_deserialize_slot_state(ship, row[column]);
  }
  column++;

  autopilot_state = row[column] ? atoi(row[column]) : AUTOPILOT_OFF;
  column++;
  route_id = row[column] ? atoi(row[column]) : 0;
  column++;
  current_waypoint_index = row[column] ? atoi(row[column]) : 0;
  column++;
  autopilot_tick_counter = row[column] ? atoi(row[column]) : 0;
  column++;
  wait_remaining = row[column] ? atoi(row[column]) : 0;
  column++;
  last_update = row[column] ? atoll(row[column]) : 0;

  if (route_id > 0)
  {
    autopilot = autopilot_init(ship);
    route = route_create(NULL);
    if (autopilot == NULL || route == NULL || !route_load(route, route_id))
    {
      if (route != NULL)
      {
        route_destroy(route);
      }
      if (autopilot != NULL)
      {
        autopilot->state = AUTOPILOT_OFF;
      }
      log("SYSERR: Ship %d could not restore route %d; autopilot left off", ship->shipnum,
          route_id);
    }
    else
    {
      autopilot->current_route = route;
      autopilot->state =
          autopilot_state >= AUTOPILOT_OFF && autopilot_state <= AUTOPILOT_COMPLETE
              ? (enum autopilot_state)autopilot_state
              : AUTOPILOT_OFF;
      autopilot->current_waypoint_index =
          route->num_waypoints > 0
              ? MAX(0, MIN(route->num_waypoints - 1, current_waypoint_index))
              : 0;
      autopilot->tick_counter = MAX(0, autopilot_tick_counter);
      autopilot->wait_remaining = MAX(0, wait_remaining);
      autopilot->last_update = (time_t)last_update;
    }
  }

  if (ship->hull_object_vnum <= 0)
  {
    ship->hull_object_vnum = VESSEL_BASE_HULL_OBJ_VNUM;
  }
  mysql_free_result(result);
  return TRUE;
}

/**
 * Resolve the exterior runtime room from a stable dock VNUM or coordinates.
 */
static room_rnum vessel_resolve_exterior_room(struct greyhawk_ship_data *ship)
{
  room_rnum location;

  if (ship == NULL)
  {
    return NOWHERE;
  }

  location = ship->location > 0 ? real_room(ship->location) : NOWHERE;
  if (location != NOWHERE && !IS_WILDERNESS_VNUM(world[location].number) &&
      !ZONE_FLAGGED(GET_ROOM_ZONE(location), ZONE_WILDERNESS))
  {
    return location;
  }

  return get_or_allocate_wilderness_room((int)ship->x, (int)ship->y);
}

/**
 * Wire and place a hull object for a restored vessel.
 */
bool vessel_place_hull_object(struct greyhawk_ship_data *ship, struct obj_data *obj)
{
  room_rnum destination;
  int old_fee_port;

  if (!is_valid_ship(ship) || obj == NULL || ship->entrance_room <= 0 ||
      real_room(ship->entrance_room) == NOWHERE)
  {
    return FALSE;
  }

  destination = vessel_resolve_exterior_room(ship);
  if (destination == NOWHERE)
  {
    log("SYSERR: Cannot place restored hull for ship %d at (%.1f,%.1f)", ship->shipnum, ship->x,
        ship->y);
    return FALSE;
  }

  GET_OBJ_TYPE(obj) = ITEM_GREYHAWK_SHIP;
  GET_OBJ_VAL(obj, 0) = ship->entrance_room;
  GET_OBJ_VAL(obj, 1) = ship->shipnum;
  if (IN_ROOM(obj) != destination)
  {
    if (IN_ROOM(obj) != NOWHERE)
    {
      obj_from_room(obj);
    }
    obj_to_room(obj, destination);
  }
  mark_wilderness_room_occupied(destination);

  ship->shipobj = obj;
  ship->shiproom = ship->entrance_room;
  ship->location = world[destination].number;
  old_fee_port = ship->dock_fee_port;
  if (old_fee_port > 0 &&
      (ship->dock_fee_balance > 0 || vessel_room_is_port(destination)) &&
      old_fee_port != ship->location)
  {
    ship->dock_fee_port = ship->location;
    log("Info: Remapped ship %d berth marker from recycled room %d to %d",
        ship->shipnum, old_fee_port, ship->dock_fee_port);
  }
  update_ship_room_coordinates(ship);
  return TRUE;
}

/**
 * Instantiate the generic exterior object for a database-restored ship.
 */
static bool vessel_create_runtime_hull(struct greyhawk_ship_data *ship)
{
  struct obj_data *obj;
  char buffer[MAX_STRING_LENGTH];

  if (!is_valid_ship(ship))
  {
    return FALSE;
  }

  obj = read_object(ship->hull_object_vnum, VIRTUAL);
  if (obj == NULL)
  {
    log("SYSERR: Cannot restore ship %d: hull object prototype %d is missing", ship->shipnum,
        ship->hull_object_vnum);
    return FALSE;
  }

  vessel_build_hull_keywords(buffer, sizeof(buffer), ship->name);
  obj->name = strdup(buffer);
  obj->short_description = strdup(ship->name);
  snprintf(buffer, sizeof(buffer), "%s is moored here.", ship->name);
  obj->description = strdup(buffer);

  if (!vessel_place_hull_object(ship, obj))
  {
    extract_obj(obj);
    return FALSE;
  }
  return TRUE;
}

/* Save docking record to database */
void save_docking_record(struct greyhawk_ship_data *ship1, struct greyhawk_ship_data *ship2,
                         const char *dock_type)
{
  char query[MAX_STRING_LENGTH];

  if (!mysql_available || !ship1 || !ship2)
  {
    return;
  }

  snprintf(query, sizeof(query),
           "INSERT INTO ship_docking "
           "(ship1_id, ship2_id, dock_room1, dock_room2, dock_type, dock_status, "
           "dock_x, dock_y, dock_z) "
           "VALUES (%d, %d, %d, %d, '%s', 'active', %.2f, %.2f, %.2f)",
           ship1->shipnum, ship2->shipnum, ship1->docking_room, ship2->docking_room,
           dock_type ? dock_type : "standard", ship1->x, ship1->y, ship1->z);

  if (mysql_query(conn, query))
  {
    log("SYSERR: Unable to save docking record: %s", mysql_error(conn));
  }
  else
  {
    log("Info: Recorded docking between ships %d and %d", ship1->shipnum, ship2->shipnum);
  }
}

/* Mark docking as completed */
void end_docking_record(struct greyhawk_ship_data *ship1, struct greyhawk_ship_data *ship2)
{
  char query[MAX_STRING_LENGTH];

  if (!mysql_available || !ship1 || !ship2)
  {
    return;
  }

  snprintf(query, sizeof(query),
           "UPDATE ship_docking SET dock_status = 'completed', undock_time = NOW() "
           "WHERE ((ship1_id = %d AND ship2_id = %d) OR "
           "(ship1_id = %d AND ship2_id = %d)) "
           "AND dock_status = 'active'",
           ship1->shipnum, ship2->shipnum, ship2->shipnum, ship1->shipnum);

  if (mysql_query(conn, query))
  {
    log("SYSERR: Unable to update docking record: %s", mysql_error(conn));
  }
  else
  {
    log("Info: Ended docking between ships %d and %d", ship1->shipnum, ship2->shipnum);
  }
}

/* Save cargo manifest entry */
void save_cargo_manifest(struct greyhawk_ship_data *ship, int cargo_room, struct obj_data *cargo)
{
  char query[MAX_STRING_LENGTH];
  char escaped_name[MAX_NAME_LENGTH * 2 + 1];

  if (!mysql_available || !ship || !cargo)
  {
    return;
  }

  mysql_real_escape_string(conn, escaped_name, cargo->short_description,
                           strlen(cargo->short_description));

  snprintf(query, sizeof(query),
           "INSERT INTO ship_cargo_manifest "
           "(ship_id, cargo_room, item_vnum, item_name, item_count, item_weight) "
           "VALUES (%d, %d, %d, '%s', %d, %d)",
           ship->shipnum, cargo_room, GET_OBJ_VNUM(cargo), escaped_name, 1,
           GET_OBJ_WEIGHT(cargo));

  if (mysql_query(conn, query))
  {
    log("SYSERR: Unable to save cargo manifest: %s", mysql_error(conn));
  }
}

/* Load cargo manifest for a ship */
void load_cargo_manifest(struct greyhawk_ship_data *ship)
{
  MYSQL_RES *result;
  MYSQL_ROW row;
  char query[MAX_STRING_LENGTH];
  struct obj_data *cargo;
  room_rnum cargo_room;
  obj_rnum obj_num;

  if (!mysql_available || !ship)
  {
    return;
  }

  snprintf(query, sizeof(query),
           "SELECT cargo_room, item_vnum, item_count FROM ship_cargo_manifest "
           "WHERE ship_id = %d AND cargo_room <> 0",
           ship->shipnum);

  if (mysql_query(conn, query))
  {
    log("SYSERR: Unable to load cargo manifest: %s", mysql_error(conn));
    return;
  }

  result = mysql_store_result(conn);
  if (!result)
  {
    return;
  }

  while ((row = mysql_fetch_row(result)))
  {
    cargo_room = real_room(atoi(row[0]));
    obj_num = real_object(atoi(row[1]));

    if (cargo_room != NOWHERE && obj_num != NOTHING)
    {
      cargo = read_object(obj_num, REAL);
      if (cargo)
      {
        obj_to_room(cargo, cargo_room);
        log("Info: Loaded cargo item %d to room %d on ship %d", GET_OBJ_VNUM(cargo),
            world[cargo_room].number, ship->shipnum);
      }
    }
  }

  mysql_free_result(result);
}

/* Save crew roster entry */
void save_crew_roster(struct greyhawk_ship_data *ship, struct char_data *npc, const char *role)
{
  char query[MAX_STRING_LENGTH];
  char escaped_name[MAX_NAME_LENGTH * 2 + 1];

  /* IS_PC doesn't exist, use !IS_NPC instead */
  if (!mysql_available || !ship || !npc || !IS_NPC(npc))
  {
    return;
  }

  mysql_real_escape_string(conn, escaped_name, GET_NAME(npc), strlen(GET_NAME(npc)));

  snprintf(query, sizeof(query),
           "INSERT INTO ship_crew_roster "
           "(ship_id, npc_vnum, npc_name, crew_role, assigned_room) "
           "VALUES (%d, %d, '%s', '%s', %d)",
           ship->shipnum, GET_MOB_VNUM(npc), escaped_name, role ? role : "crew", IN_ROOM(npc));

  if (mysql_query(conn, query))
  {
    log("SYSERR: Unable to save crew roster: %s", mysql_error(conn));
  }
}

/* Load crew roster for a ship */
void load_crew_roster(struct greyhawk_ship_data *ship)
{
  MYSQL_RES *result;
  MYSQL_ROW row;
  char query[MAX_STRING_LENGTH];
  struct char_data *npc;
  mob_rnum mob_num;
  room_rnum target_room;

  if (!mysql_available || !ship)
  {
    return;
  }

  snprintf(query, sizeof(query),
           "SELECT npc_vnum, assigned_room, crew_role FROM ship_crew_roster "
           "WHERE ship_id = %d AND status = 'active' AND npc_vnum > 0",
           ship->shipnum);

  if (mysql_query(conn, query))
  {
    log("SYSERR: Unable to load crew roster: %s", mysql_error(conn));
    return;
  }

  result = mysql_store_result(conn);
  if (!result)
  {
    return;
  }

  while ((row = mysql_fetch_row(result)))
  {
    mob_num = real_mobile(atoi(row[0]));
    target_room = real_room(atoi(row[1]));

    if (mob_num != NOBODY && target_room != NOWHERE)
    {
      npc = read_mobile(mob_num, REAL);
      if (npc)
      {
        char_to_room(npc, target_room);
        /* Set ship loyalty here if needed */
        log("Info: Loaded crew member %s to ship %d", GET_NAME(npc), ship->shipnum);
      }
    }
  }

  mysql_free_result(result);
}

/* Serialize room connection data */
int serialize_room_data(struct greyhawk_ship_data *ship, char *buffer, int buffer_size)
{
  int i, len = 0;

  if (!ship || !buffer || buffer_size <= 0)
  {
    return 0;
  }

  /* Simple format: connection_count|from:to:dir:hatch:locked|... */
  len = snprintf(buffer, buffer_size, "%d", ship->num_connections);

  for (i = 0; i < ship->num_connections && i < MAX_SHIP_CONNECTIONS; i++)
  {
    len = snprintf_append(buffer, (size_t)buffer_size, len, "|%d:%d:%d:%d:%d",
                          ship->connections[i].from_room, ship->connections[i].to_room,
                          ship->connections[i].direction, ship->connections[i].is_hatch ? 1 : 0,
                          ship->connections[i].is_locked ? 1 : 0);
  }

  return len;
}

/* Deserialize room connection data */
int deserialize_room_data(struct greyhawk_ship_data *ship, const char *data)
{
  char data_copy[4096];
  char *token;
  int i = 0;
  int conn_count;

  if (!ship || !data)
  {
    return 0;
  }

  strncpy(data_copy, data, sizeof(data_copy) - 1);
  data_copy[sizeof(data_copy) - 1] = '\0';

  /* Parse format: connection_count|from:to:dir:hatch:locked|... */
  /* Keep parsing simple and portable. */
  conn_count = 0;
  sscanf(data_copy, "%d", &conn_count);

  /* Find first | separator */
  token = strchr(data_copy, '|');
  if (!token)
  {
    return 0;
  }
  token++; /* Skip the | */

  /* Parse each connection */
  while (token && *token && i < MAX_SHIP_CONNECTIONS)
  {
    int from, to, dir, hatch, locked;

    if (sscanf(token, "%d:%d:%d:%d:%d", &from, &to, &dir, &hatch, &locked) == 5)
    {
      ship->connections[i].from_room = from;
      ship->connections[i].to_room = to;
      ship->connections[i].direction = dir;
      ship->connections[i].is_hatch = hatch ? TRUE : FALSE;
      ship->connections[i].is_locked = locked ? TRUE : FALSE;
      i++;
    }

    /* Find next | separator */
    token = strchr(token, '|');
    if (token)
    {
      token++; /* Skip the | */
    }
  }

  ship->num_connections = MIN(i, MAX(0, conn_count));
  return ship->num_connections;
}

/* Clean up orphaned docking records */
void cleanup_orphaned_dockings(void)
{
  char query[MAX_STRING_LENGTH];

  if (!mysql_available)
  {
    return;
  }

  /* Call stored procedure to clean up old dockings */
  snprintf(query, sizeof(query), "CALL cleanup_orphaned_dockings()");

  if (mysql_query(conn, query))
  {
    log("SYSERR: Unable to cleanup orphaned dockings: %s", mysql_error(conn));
  }
  else
  {
    log("Info: Cleaned up orphaned docking records");
  }
}

/* Initialize vessel database tables */
void init_vessel_db(void)
{
  if (!mysql_available)
  {
    log("Info: MySQL not available, vessel persistence disabled");
    return;
  }

  /* Clean up any orphaned dockings on startup */
  cleanup_orphaned_dockings();

  log("Info: Vessel database persistence initialized");
}

/* ========================================================================= */
/* PERSISTENCE LIFECYCLE FUNCTIONS                                           */
/* ========================================================================= */

/* External array declaration */
extern struct greyhawk_ship_data greyhawk_ships[GREYHAWK_MAXSHIPS];

/**
 * Check if a ship slot contains valid ship data.
 *
 * Occupancy is explicit, and shipnum must identify this exact fleet slot.
 * The display ID is not part of the runtime identity contract.
 *
 * @param ship Pointer to ship data structure
 * @return TRUE if ship is valid, FALSE otherwise
 */
int is_valid_ship(const struct greyhawk_ship_data *ship)
{
  if (ship == NULL)
  {
    return FALSE;
  }

  if (!ship->active || ship->shipnum < 0 || ship->shipnum >= GREYHAWK_MAXSHIPS)
  {
    return FALSE;
  }

  return &greyhawk_ships[ship->shipnum] == ship;
}

/**
 * Delete all persistence owned by one runtime ship slot.
 *
 * This transaction intentionally does not touch prototypes, routes, or
 * waypoints, which are reusable builder data rather than ship-instance data.
 *
 * @param shipnum Canonical fleet slot
 * @return TRUE when the transaction commits
 */
bool vessel_delete_persistence(int shipnum)
{
  char query[MAX_STRING_LENGTH];

  if (!mysql_available || shipnum < 0 || shipnum >= GREYHAWK_MAXSHIPS)
  {
    return FALSE;
  }

  vessel_persistence_ensure_schema();
  if (mysql_query(conn, "START TRANSACTION"))
  {
    log("SYSERR: Unable to begin vessel purge transaction: %s", mysql_error(conn));
    return FALSE;
  }

  snprintf(query, sizeof(query), "DELETE FROM ship_docking WHERE ship1_id=%d OR ship2_id=%d",
           shipnum, shipnum);
  if (mysql_query(conn, query))
    goto rollback;

  snprintf(query, sizeof(query), "DELETE FROM ship_cargo_manifest WHERE ship_id=%d", shipnum);
  if (mysql_query(conn, query))
    goto rollback;

  snprintf(query, sizeof(query), "DELETE FROM ship_crew_roster WHERE ship_id=%d", shipnum);
  if (mysql_query(conn, query))
    goto rollback;

  snprintf(query, sizeof(query), "DELETE FROM ship_schedules WHERE ship_id=%d", shipnum);
  if (mysql_query(conn, query))
    goto rollback;

  snprintf(query, sizeof(query), "DELETE FROM ship_weapons WHERE ship_id=%d", shipnum);
  if (mysql_query(conn, query))
    goto rollback;

  snprintf(query, sizeof(query), "DELETE FROM ship_runtime_state WHERE ship_id=%d", shipnum);
  if (mysql_query(conn, query))
    goto rollback;

  snprintf(query, sizeof(query), "DELETE FROM ship_interiors WHERE ship_id=%d", shipnum);
  if (mysql_query(conn, query))
    goto rollback;

  if (mysql_query(conn, "COMMIT"))
  {
    log("SYSERR: Unable to commit vessel purge transaction: %s", mysql_error(conn));
    mysql_query(conn, "ROLLBACK");
    return FALSE;
  }

  return TRUE;

rollback:
  log("SYSERR: Vessel %d persistence purge failed: %s", shipnum, mysql_error(conn));
  mysql_query(conn, "ROLLBACK");
  return FALSE;
}

/**
 * Load all ship interiors from database on server boot.
 *
 * Iterates through the greyhawk_ships array and loads interior
 * configurations for any valid ships that have saved data.
 * Should be called after rooms are loaded in boot_world().
 */
void load_all_ship_interiors(void)
{
  MYSQL_RES *result;
  MYSQL_ROW row;
  int ship_ids[GREYHAWK_MAXSHIPS];
  struct greyhawk_ship_data *ship;
  char query[MAX_STRING_LENGTH];
  int ship_count;
  int i;
  int loaded_count;
  int shipnum;

  if (!mysql_available)
  {
    log("Info: MySQL not available, skipping ship interior load");
    return;
  }

  log("Info: Loading ship interiors from database...");
  vessel_persistence_ensure_schema();

  snprintf(query, sizeof(query), "SELECT ship_id FROM ship_interiors ORDER BY ship_id");
  if (mysql_query(conn, query))
  {
    log("SYSERR: Unable to enumerate persisted ships: %s", mysql_error(conn));
    return;
  }

  result = mysql_store_result(conn);
  if (result == NULL)
  {
    log("SYSERR: Unable to store persisted ship list: %s", mysql_error(conn));
    return;
  }

  ship_count = 0;
  while ((row = mysql_fetch_row(result)) != NULL && ship_count < GREYHAWK_MAXSHIPS)
  {
    shipnum = row[0] ? atoi(row[0]) : -1;
    if (shipnum <= 0 || shipnum >= GREYHAWK_MAXSHIPS)
    {
      log("SYSERR: Ignoring persisted vessel with invalid fleet slot %d", shipnum);
      continue;
    }
    ship_ids[ship_count++] = shipnum;
  }
  mysql_free_result(result);

  loaded_count = 0;
  for (i = 0; i < ship_count; i++)
  {
    shipnum = ship_ids[i];
    ship = &greyhawk_ships[shipnum];

    if (shipnum >= 2)
    {
      if (is_valid_ship(ship))
      {
        log("SYSERR: Duplicate active fleet slot %d while restoring vessel persistence", shipnum);
        continue;
      }
      memset(ship, 0, sizeof(*ship));
      ship->active = TRUE;
      ship->shipnum = shipnum;
      ship->docked_to_ship = -1;
      ship->hull_object_vnum = VESSEL_BASE_HULL_OBJ_VNUM;
    }

    if (!is_valid_ship(ship))
    {
      log("SYSERR: Persisted legacy ship slot %d is not available at boot", shipnum);
      continue;
    }

    load_ship_interior(ship);
    if (!vessel_db_load_runtime(ship))
    {
      if (shipnum >= 2)
      {
        log("SYSERR: Dynamic ship %d has no runtime snapshot; leaving it inactive", shipnum);
        memset(ship, 0, sizeof(*ship));
        continue;
      }
      log("Info: Legacy ship %d has no runtime snapshot; using compiled defaults", shipnum);
    }
    if (shipnum == 1)
    {
      /* The compiled fixture uses the ordinary runtime hull prototype. */
      ship->hull_object_vnum = VESSEL_BASE_HULL_OBJ_VNUM;
    }
    if (!vessel_db_load_weapons(ship))
    {
      log("SYSERR: Ship %d weapon rows could not be restored", shipnum);
    }

    if (ship->id[0] == '\0')
    {
      ship->id[0] = 'A' + (shipnum / 26) % 26;
      ship->id[1] = 'A' + shipnum % 26;
      ship->id[2] = '\0';
    }

    if (shipnum >= 2)
    {
      if (!restore_ship_interior(ship) || !vessel_create_runtime_hull(ship))
      {
        log("SYSERR: Dynamic ship %d could not be reconstructed; leaving persistence intact",
            shipnum);
        vessel_reclaim_interior_rooms(ship, 0);
        autopilot_cleanup(ship);
        if (ship->schedule != NULL)
        {
          free(ship->schedule);
        }
        memset(ship, 0, sizeof(*ship));
        continue;
      }
    }
    else if (!vessel_create_runtime_hull(ship))
    {
      log("SYSERR: Legacy ship %d exterior hull could not be reconstructed", shipnum);
    }

    vessel_db_load_owner(ship);
    vessel_db_load_permits(ship);
    vessel_db_load_crew(ship);
    vessel_db_load_extras(ship);
    vessel_db_load_cargo(ship);
    load_cargo_manifest(ship);
    load_crew_roster(ship);
    vessel_db_load_pilot(ship);
    schedule_load(ship);
    loaded_count++;
  }

  log("Info: Reconstructed %d persisted vessel instance%s", loaded_count,
      loaded_count == 1 ? "" : "s");
}

/**
 * Save all vessel states to database.
 *
 * Iterates through the greyhawk_ships array and saves interior
 * configurations for all valid ships. Should be called at shutdown
 * and periodically during auto-save.
 */
bool save_all_vessels(void)
{
  struct greyhawk_ship_data *ship;
  int i;
  int saved_count;
  int failure_count;

  if (!mysql_available)
  {
    log("Info: MySQL not available, skipping vessel save");
    return FALSE;
  }

  vessel_persistence_ensure_schema();
  log("Info: Saving all vessel states to database...");
  saved_count = 0;
  failure_count = 0;

  for (i = 0; i < GREYHAWK_MAXSHIPS; i++)
  {
    ship = &greyhawk_ships[i];
    if (is_valid_ship(ship))
    {
      if (!save_ship_interior(ship) || !vessel_db_save_runtime(ship) ||
          !vessel_db_save_weapons(ship))
      {
        failure_count++;
        continue;
      }
      vessel_db_save_owner(ship);
      vessel_db_save_permits(ship);
      vessel_db_save_crew(ship);
      vessel_db_save_extras(ship);
      vessel_db_save_cargo(ship);
      vessel_db_save_pilot(ship);
      if (!schedule_save(ship))
      {
        failure_count++;
      }
      saved_count++;
    }
  }

  log("Info: Saved %d vessel states to database (%d failure%s)", saved_count, failure_count,
      failure_count == 1 ? "" : "s");
  return failure_count == 0;
}

/* ========================================================================= */
/* NPC PILOT PERSISTENCE FUNCTIONS                                            */
/* ========================================================================= */

/**
 * Save pilot assignment to database.
 *
 * Stores the pilot VNUM in ship_crew_roster with role='pilot'.
 * Only one pilot per ship is allowed, so existing pilot records
 * are deleted before inserting the new one.
 *
 * @param ship The ship to save pilot for
 */
void vessel_db_save_pilot(struct greyhawk_ship_data *ship)
{
  char query[MAX_STRING_LENGTH];
  struct char_data *pilot;
  char escaped_name[MAX_NAME_LENGTH * 2 + 1];

  if (!mysql_available || !ship)
  {
    return;
  }

  /* Delete any existing pilot record for this ship */
  snprintf(query, sizeof(query),
           "DELETE FROM ship_crew_roster WHERE ship_id = %d AND crew_role = '%s'", ship->shipnum,
           CREW_ROLE_PILOT);

  if (mysql_query(conn, query))
  {
    log("SYSERR: Unable to clear pilot record for ship %d: %s", ship->shipnum,
        mysql_error(conn));
    return;
  }

  /* If no pilot assigned, we're done */
  if (ship->autopilot == NULL || ship->autopilot->pilot_mob_vnum == -1)
  {
    return;
  }

  /* Get pilot name if possible */
  pilot = get_pilot_from_ship(ship);
  if (pilot != NULL)
  {
    mysql_real_escape_string(conn, escaped_name, GET_NAME(pilot), strlen(GET_NAME(pilot)));
  }
  else
  {
    snprintf(escaped_name, sizeof(escaped_name), "Unknown Pilot");
  }

  /* Insert new pilot record */
  snprintf(query, sizeof(query),
           "INSERT INTO ship_crew_roster "
           "(ship_id, npc_vnum, npc_name, crew_role, assigned_room, status) "
           "VALUES (%d, %d, '%s', '%s', %d, 'active')",
           ship->shipnum, ship->autopilot->pilot_mob_vnum, escaped_name, CREW_ROLE_PILOT,
           ship->bridge_room);

  if (mysql_query(conn, query))
  {
    log("SYSERR: Unable to save pilot for ship %d: %s", ship->shipnum, mysql_error(conn));
  }
  else
  {
    log("Info: Saved pilot VNUM %d for ship %d", ship->autopilot->pilot_mob_vnum,
        ship->shipnum);
  }
}

/**
 * Load pilot assignment from database.
 *
 * Retrieves the pilot VNUM from ship_crew_roster and sets
 * autopilot->pilot_mob_vnum. Does NOT spawn the NPC - that
 * should be handled by load_crew_roster() separately.
 *
 * @param ship The ship to load pilot for
 */
void vessel_db_load_pilot(struct greyhawk_ship_data *ship)
{
  MYSQL_RES *result;
  MYSQL_ROW row;
  char query[MAX_STRING_LENGTH];

  if (!mysql_available || !ship)
  {
    return;
  }

  /* Ensure autopilot is initialized */
  if (ship->autopilot == NULL)
  {
    ship->autopilot = autopilot_init(ship);
    if (ship->autopilot == NULL)
    {
      return;
    }
  }

  /* Query for pilot record */
  snprintf(query, sizeof(query),
           "SELECT npc_vnum FROM ship_crew_roster "
           "WHERE ship_id = %d AND crew_role = '%s' AND status = 'active' "
           "LIMIT 1",
           ship->shipnum, CREW_ROLE_PILOT);

  if (mysql_query(conn, query))
  {
    log("SYSERR: Unable to load pilot for ship %d: %s", ship->shipnum, mysql_error(conn));
    return;
  }

  result = mysql_store_result(conn);
  if (!result)
  {
    return;
  }

  if ((row = mysql_fetch_row(result)))
  {
    ship->autopilot->pilot_mob_vnum = atoi(row[0]);
    log("Info: Loaded pilot VNUM %d for ship %d", ship->autopilot->pilot_mob_vnum,
        ship->shipnum);
  }
  else
  {
    ship->autopilot->pilot_mob_vnum = -1;
  }

  mysql_free_result(result);
}

/* ========================================================================= */
/* SCHEDULE PERSISTENCE FUNCTIONS                                             */
/* ========================================================================= */

/**
 * Ensure the ship_schedules table exists in the database.
 * Creates the table if it does not exist.
 */
void ensure_schedule_table_exists(void)
{
  const char *create_query =
      "CREATE TABLE IF NOT EXISTS ship_schedules ("
      "  schedule_id INT AUTO_INCREMENT PRIMARY KEY,"
      "  ship_id INT NOT NULL,"
      "  route_id INT NOT NULL,"
      "  interval_hours INT NOT NULL,"
      "  next_departure INT NOT NULL DEFAULT 0,"
      "  enabled TINYINT NOT NULL DEFAULT 1,"
      "  created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,"
      "  updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,"
      "  UNIQUE KEY uk_ship_schedule (ship_id)"
      ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4";

  if (!mysql_available)
  {
    log("Info: MySQL not available, schedule table check skipped");
    return;
  }

  if (mysql_query(conn, create_query))
  {
    log("SYSERR: Unable to create ship_schedules table: %s", mysql_error(conn));
  }
  else
  {
    log("Info: ship_schedules table verified");
  }
}

/**
 * Save vessel schedule to database.
 *
 * @param ship The ship with schedule to save
 * @return 1 on success, 0 on failure
 */
int schedule_save(struct greyhawk_ship_data *ship)
{
  char query[MAX_STRING_LENGTH];

  if (!mysql_available || !ship)
  {
    return 0;
  }

  /* If no schedule, delete any existing record */
  if (ship->schedule == NULL)
  {
    snprintf(query, sizeof(query), "DELETE FROM ship_schedules WHERE ship_id = %d", ship->shipnum);

    if (mysql_query(conn, query))
    {
      log("SYSERR: Unable to delete schedule for ship %d: %s", ship->shipnum, mysql_error(conn));
      return 0;
    }
    return 1;
  }

  /* Insert or update schedule */
  snprintf(query, sizeof(query),
           "REPLACE INTO ship_schedules "
           "(ship_id, route_id, interval_hours, next_departure, enabled) "
           "VALUES (%d, %d, %d, %d, %d)",
           ship->shipnum, ship->schedule->route_id, ship->schedule->interval_hours,
           ship->schedule->next_departure, (ship->schedule->flags & SCHEDULE_FLAG_ENABLED) ? 1 : 0);

  if (mysql_query(conn, query))
  {
    log("SYSERR: Unable to save schedule for ship %d: %s", ship->shipnum, mysql_error(conn));
    return 0;
  }

  log("Info: Saved schedule for ship %d (route %d, interval %d hours)", ship->shipnum,
      ship->schedule->route_id, ship->schedule->interval_hours);
  return 1;
}

/**
 * Load vessel schedule from database.
 *
 * @param ship The ship to load schedule for
 * @return 1 on success, 0 on failure or no schedule
 */
int schedule_load(struct greyhawk_ship_data *ship)
{
  MYSQL_RES *result;
  MYSQL_ROW row;
  char query[MAX_STRING_LENGTH];

  if (!mysql_available || !ship)
  {
    return 0;
  }

  snprintf(query, sizeof(query),
           "SELECT schedule_id, route_id, interval_hours, next_departure, enabled "
           "FROM ship_schedules WHERE ship_id = %d",
           ship->shipnum);

  if (mysql_query(conn, query))
  {
    log("SYSERR: Unable to load schedule for ship %d: %s", ship->shipnum, mysql_error(conn));
    return 0;
  }

  result = mysql_store_result(conn);
  if (!result)
  {
    return 0;
  }

  if ((row = mysql_fetch_row(result)))
  {
    /* Allocate schedule if needed */
    if (ship->schedule == NULL)
    {
      CREATE(ship->schedule, struct vessel_schedule, 1);
      if (ship->schedule == NULL)
      {
        log("SYSERR: Unable to allocate schedule for ship %d", ship->shipnum);
        mysql_free_result(result);
        return 0;
      }
    }

    ship->schedule->schedule_id = atoi(row[0]);
    ship->schedule->ship_id = ship->shipnum;
    ship->schedule->route_id = atoi(row[1]);
    ship->schedule->interval_hours = atoi(row[2]);
    ship->schedule->next_departure = atoi(row[3]);
    ship->schedule->flags = atoi(row[4]) ? SCHEDULE_FLAG_ENABLED : 0;

    log("Info: Loaded schedule for ship %d (route %d, interval %d hours)", ship->shipnum,
        ship->schedule->route_id, ship->schedule->interval_hours);
    mysql_free_result(result);
    return 1;
  }

  mysql_free_result(result);
  return 0;
}

/**
 * Load all schedules from database at boot time.
 */
void load_all_schedules(void)
{
  int i;
  int loaded_count = 0;

  if (!mysql_available)
  {
    log("Info: MySQL not available, skipping schedule load");
    return;
  }

  /* Ensure table exists */
  ensure_schedule_table_exists();

  log("Info: Loading vessel schedules from database...");

  for (i = 0; i < GREYHAWK_MAXSHIPS; i++)
  {
    if (is_valid_ship(&greyhawk_ships[i]))
    {
      if (schedule_load(&greyhawk_ships[i]))
      {
        loaded_count++;
      }
    }
  }

  log("Info: Loaded %d vessel schedules", loaded_count);
}

/**
 * Save all schedules to database.
 */
void save_all_schedules(void)
{
  int i;
  int saved_count = 0;

  if (!mysql_available)
  {
    log("Info: MySQL not available, skipping schedule save");
    return;
  }

  log("Info: Saving all vessel schedules to database...");

  for (i = 0; i < GREYHAWK_MAXSHIPS; i++)
  {
    if (is_valid_ship(&greyhawk_ships[i]) && greyhawk_ships[i].schedule != NULL)
    {
      if (schedule_save(&greyhawk_ships[i]))
      {
        saved_count++;
      }
    }
  }

  log("Info: Saved %d vessel schedules", saved_count);
}
