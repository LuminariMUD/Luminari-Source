/* ************************************************************************
 *      File:   vessels_edit.c                        Part of LuminariMUD  *
 *   Purpose:   vedit - ship prototype editor (Phase 04, Session 03)        *
 *              Builder-facing tool: author vessel prototypes in the        *
 *              ship_prototypes table and spawn live ships from them        *
 *              without recompiling.                                        *
 * ********************************************************************** */

#include "conf.h"
#include "core/sysdep.h"
#include "core/structs.h"
#include "core/utils.h"
#include "core/comm.h"
#include "core/db.h"
#include "core/handler.h"
#include "character/rewards.h"
#include "core/interpreter.h"
#include "vessels.h"
#include "vessel_periodic.h"
#include "database/mysql.h"
#include "wilderness/wilderness.h"

extern struct greyhawk_ship_data greyhawk_ships[GREYHAWK_MAXSHIPS];

/* Bounds for editable prototype fields */
#define VEDIT_MAX_MIN_LEVEL (LVL_IMMORT - 1)

static const char *VEDIT_USAGE =
    "Usage:\r\n"
    "  vedit list                      - list all ship prototypes\r\n"
    "  vedit new <class> <name>        - create a prototype (class 0-7)\r\n"
    "  vedit show <id>                 - show one prototype\r\n"
    "  vedit set <id> <field> <value>  - fields: name, class, speed, armor,\r\n"
    "                                    forsale (yes/no), minlevel (0 = class)\r\n"
    "  vedit delete <id>               - delete a prototype\r\n"
    "  vedit spawn <id>                - spawn a live ship here from a prototype\r\n"
    "  vedit spawnpublic <id>          - spawn an unclaimed NPC/public ship\r\n"
    "Classes: 0=Raft 1=Boat 2=Ship 3=Warship 4=Airship 5=Submarine 6=Transport 7=Magical\r\n";

/**
 * Ensure the ship_prototypes table exists with its Phase 18 columns.
 *
 * Follows the vessel-system convention of auto-creating tables so a fresh
 * database works without manual schema steps. Mirrored by
 * sql/components/vessels_phase4_schema.sql and vessels_phase18_schema.sql.
 * Boot only: the ALTER can wait on another transaction's metadata lock, so
 * command paths check vessel_prototype_db_ready() instead.
 *
 * @return TRUE if the table is available, FALSE otherwise
 */
bool vessel_prototype_ensure_schema(void)
{
  const char *create_sql = "CREATE TABLE IF NOT EXISTS ship_prototypes ("
                           "  prototype_id INT AUTO_INCREMENT PRIMARY KEY,"
                           "  name VARCHAR(127) NOT NULL,"
                           "  vessel_class INT NOT NULL DEFAULT 2,"
                           "  max_speed INT NOT NULL DEFAULT 10,"
                           "  armor INT NOT NULL DEFAULT 10,"
                           "  for_sale TINYINT(1) NOT NULL DEFAULT 0,"
                           "  min_level INT NOT NULL DEFAULT 0,"
                           "  armor_scale TINYINT(1) NOT NULL DEFAULT 1,"
                           "  created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP"
                           ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4";
  /* Rows that predate armor_scale get 0 and are rescaled once below; rows
   * written afterwards default to 1, the S3 scale. */
  const char *alter_sql = "ALTER TABLE ship_prototypes "
                          "ADD COLUMN IF NOT EXISTS for_sale TINYINT(1) NOT NULL DEFAULT 0 "
                          "AFTER armor, "
                          "ADD COLUMN IF NOT EXISTS min_level INT NOT NULL DEFAULT 0 "
                          "AFTER for_sale, "
                          "ADD COLUMN IF NOT EXISTS armor_scale TINYINT(1) NOT NULL DEFAULT 0 "
                          "AFTER min_level";
  /* vessel_rescale_legacy_armor(): class beam armor over the old default */
  const char *rescale_sql =
      "UPDATE ship_prototypes AS prototype "
      "INNER JOIN ("
      "SELECT 0 AS vessel_class, 2 AS legacy_armor, 3 AS beam_armor "
      "UNION ALL SELECT 1, 5, 8 UNION ALL SELECT 2, 20, 66 UNION ALL SELECT 3, 40, 109 "
      "UNION ALL SELECT 4, 15, 63 UNION ALL SELECT 5, 25, 84 UNION ALL SELECT 6, 20, 110 "
      "UNION ALL SELECT 7, 20, 153"
      ") AS scale ON scale.vessel_class = "
      "IF(prototype.vessel_class BETWEEN 0 AND 7, prototype.vessel_class, 2) "
      "SET prototype.armor = LEAST(229, (GREATEST(0, prototype.armor) * scale.beam_armor "
      "+ scale.legacy_armor DIV 2) DIV scale.legacy_armor), "
      "prototype.armor_scale = 1 "
      "WHERE prototype.armor_scale = 0";
  const char *default_sql = "ALTER TABLE ship_prototypes ALTER COLUMN armor_scale SET DEFAULT 1";

  if (!mysql_available || conn == NULL)
  {
    return FALSE;
  }

  if (mysql_query(conn, create_sql) || mysql_query(conn, alter_sql) ||
      mysql_query(conn, rescale_sql) || mysql_query(conn, default_sql))
  {
    log("SYSERR: vessel_prototype_ensure_schema failed: %s", mysql_error(conn));
    return FALSE;
  }

  return TRUE;
}

/**
 * Can command paths read the prototype table? Its schema is migrated at boot.
 */
static bool vessel_prototype_db_ready(void)
{
  return mysql_available && conn != NULL;
}

/**
 * Lowest character level that may take a hull of this class out of port.
 *
 * DurisMUD hull levels scaled to the 30 mortal levels (vessels-ships study
 * 3.3.1); airship and submarine placements are LuminariMUD's own.
 */
int vessel_class_min_level(int vclass)
{
  static const int class_min_level[NUM_VESSEL_TYPES] = {
      1,  /* RAFT */
      1,  /* BOAT */
      16, /* SHIP */
      22, /* WARSHIP */
      24, /* AIRSHIP */
      23, /* SUBMARINE */
      21, /* TRANSPORT */
      25  /* MAGICAL */
  };

  if (vclass < 0 || vclass >= NUM_VESSEL_TYPES)
  {
    vclass = VESSEL_SHIP;
  }
  return class_min_level[vclass];
}

/**
 * A prototype's minimum level: its own setting, or the class minimum when 0.
 */
int vessel_prototype_min_level(int vclass, int min_level)
{
  return min_level > 0 ? min_level : vessel_class_min_level(vclass);
}

/**
 * Minimum level for this hull: its prototype's setting, else its class minimum.
 *
 * @return The level, or VESSEL_MIN_LEVEL_UNKNOWN when the hull's prototype
 *         cannot be read, since its own setting may be higher than the class
 */
int vessel_ship_min_level(const struct greyhawk_ship_data *ship)
{
  PREPARED_STMT *statement;
  int level;

  if (ship == NULL)
  {
    return 1;
  }

  level = vessel_class_min_level(ship->vessel_type);
  if (ship->prototype_id <= 0)
  {
    return level;
  }
  if (!vessel_prototype_db_ready())
  {
    return VESSEL_MIN_LEVEL_UNKNOWN;
  }

  statement = mysql_stmt_create(conn);
  if (statement == NULL ||
      !mysql_stmt_prepare_query(statement,
                                "SELECT min_level FROM ship_prototypes WHERE prototype_id = ?") ||
      !mysql_stmt_bind_param_int(statement, 0, ship->prototype_id) ||
      !mysql_stmt_execute_prepared(statement))
  {
    log("SYSERR: Could not read the minimum level of ship prototype %d", ship->prototype_id);
    mysql_stmt_cleanup(statement);
    return VESSEL_MIN_LEVEL_UNKNOWN;
  }
  if (mysql_stmt_fetch_row(statement) && mysql_stmt_get_int(statement, 0) > 0)
  {
    level = mysql_stmt_get_int(statement, 0);
  }
  mysql_stmt_cleanup(statement);
  return level;
}

/**
 * Is ch too junior to command this hull's departures?
 *
 * The hull's minimum level applies to whoever takes her out of port by hand,
 * engages her autopilot, or puts an NPC pilot or schedule to work. Staff and
 * NPC pilots are exempt.
 *
 * @return TRUE when refused (the reason is sent to ch)
 */
bool vessel_helm_level_refused(struct char_data *ch, const struct greyhawk_ship_data *ship)
{
  int required;

  if (ch == NULL || ship == NULL || IS_NPC(ch) || GET_LEVEL(ch) >= LVL_IMMORT)
  {
    return FALSE;
  }

  required = vessel_ship_min_level(ship);
  if (required == VESSEL_MIN_LEVEL_UNKNOWN)
  {
    send_to_char(ch,
                 "The harbor records cannot confirm who may command %s just now. Try again "
                 "shortly.\r\n",
                 ship->name);
    return TRUE;
  }
  if (GET_LEVEL(ch) >= required)
  {
    return FALSE;
  }

  send_to_char(ch, "Only a captain of level %d or higher may command %s (%s).\r\n", required,
               ship->name, get_vessel_type_name(ship->vessel_type));
  return TRUE;
}

/**
 * List all prototypes to the character.
 */
static void vedit_list(struct char_data *ch)
{
  MYSQL_RES *result;
  MYSQL_ROW row;

  if (mysql_query(conn, "SELECT prototype_id, name, vessel_class, max_speed, armor, for_sale, "
                        "min_level FROM ship_prototypes ORDER BY prototype_id"))
  {
    log("SYSERR: vedit_list query failed: %s", mysql_error(conn));
    send_to_char(ch, "Database error listing prototypes.\r\n");
    return;
  }

  result = mysql_store_result(conn);
  if (result == NULL)
  {
    send_to_char(ch, "Database error listing prototypes.\r\n");
    return;
  }

  send_to_char(ch, "ID    Class      Speed Armor Sale Lvl Name\r\n");
  send_to_char(ch, "----- ---------- ----- ----- ---- --- ----------------------------\r\n");
  while ((row = mysql_fetch_row(result)) != NULL)
  {
    send_to_char(ch, "%-5s %-10s %-5s %-5s %-4s %-3d %s\r\n", row[0],
                 get_vessel_type_name((enum vessel_class)parse_int(row[2])), row[3], row[4],
                 parse_int(row[5]) ? "yes" : "no",
                 vessel_prototype_min_level(parse_int(row[2]), parse_int(row[6])), row[1]);
  }
  mysql_free_result(result);
}

/**
 * Create a new prototype with per-class defaults.
 */
static void vedit_new(struct char_data *ch, const char *class_arg, const char *name_arg)
{
  char query[MAX_STRING_LENGTH];
  char escaped_name[256];
  int vclass;
  int max_speed;
  int armor;

  vclass = parse_int(class_arg);
  if (!isdigit((unsigned char)*class_arg) || vclass < 0 || vclass >= NUM_VESSEL_TYPES)
  {
    send_to_char(ch, "Invalid class. %s", VEDIT_USAGE);
    return;
  }

  if (!*name_arg || strlen(name_arg) > 100)
  {
    send_to_char(ch, "Prototype needs a name (max 100 characters).\r\n");
    return;
  }

  /* Class defaults; all tunable afterward via 'vedit set'. */
  max_speed = vessel_class_handling((enum vessel_class)vclass)->speed;
  armor = vessel_class_condition((enum vessel_class)vclass)->beam_armor;

  mysql_real_escape_string(conn, escaped_name, name_arg, strlen(name_arg));
  snprintf(query, sizeof(query),
           "INSERT INTO ship_prototypes (name, vessel_class, max_speed, armor) "
           "VALUES ('%s', %d, %d, %d)",
           escaped_name, vclass, max_speed, armor);

  if (mysql_query(conn, query))
  {
    log("SYSERR: vedit_new insert failed: %s", mysql_error(conn));
    send_to_char(ch, "Database error creating prototype.\r\n");
    return;
  }

  send_to_char(ch, "Created %s prototype %lu: '%s' (speed %d, armor %d).\r\n",
               get_vessel_type_name((enum vessel_class)vclass),
               (unsigned long)mysql_insert_id(conn), name_arg, max_speed, armor);
}

/**
 * Fetch one prototype row. Caller must mysql_free_result() when done.
 *
 * @return The result set positioned with one fetched row via *out_row, or
 *         NULL if not found / error (message already sent to ch).
 */
static MYSQL_RES *vedit_fetch(struct char_data *ch, int id, MYSQL_ROW *out_row)
{
  char query[MAX_STRING_LENGTH];
  MYSQL_RES *result;
  MYSQL_ROW row;

  snprintf(query, sizeof(query),
           "SELECT prototype_id, name, vessel_class, max_speed, armor, for_sale, min_level "
           "FROM ship_prototypes WHERE prototype_id = %d",
           id);

  if (mysql_query(conn, query))
  {
    log("SYSERR: vedit_fetch query failed: %s", mysql_error(conn));
    if (ch != NULL)
    {
      send_to_char(ch, "Database error.\r\n");
    }
    return NULL;
  }

  result = mysql_store_result(conn);
  if (result == NULL)
  {
    if (ch != NULL)
    {
      send_to_char(ch, "Database error.\r\n");
    }
    return NULL;
  }

  row = mysql_fetch_row(result);
  if (row == NULL)
  {
    mysql_free_result(result);
    if (ch != NULL)
    {
      send_to_char(ch, "No prototype with id %d.\r\n", id);
    }
    return NULL;
  }

  *out_row = row;
  return result;
}

/**
 * Show one prototype in detail.
 */
static void vedit_show(struct char_data *ch, int id)
{
  MYSQL_RES *result;
  MYSQL_ROW row;

  result = vedit_fetch(ch, id, &row);
  if (result == NULL)
  {
    return;
  }

  send_to_char(ch,
               "Prototype %s: %s\r\n"
               "  Class : %s (%s)\r\n"
               "  Speed : %s\r\n"
               "  Armor : %s (beam; the class profile sets the other arcs and the structure)\r\n"
               "  Cargo : %d lbs (fixed per class)\r\n"
               "  Sale  : %s\r\n"
               "  Level : %d to take her out of port%s\r\n",
               row[0], row[1], row[2], get_vessel_type_name((enum vessel_class)parse_int(row[2])),
               row[3], row[4], get_vessel_cargo_capacity((enum vessel_class)parse_int(row[2])),
               parse_int(row[5]) ? "listed in the shipyard" : "not for sale",
               vessel_prototype_min_level(parse_int(row[2]), parse_int(row[6])),
               parse_int(row[6]) > 0 ? "" : " (class minimum)");
  mysql_free_result(result);
}

/**
 * Store one integer prototype field through a bound statement.
 *
 * @return TRUE when the update ran
 */
static bool vedit_update_int_field(const char *sql, int value, int id)
{
  PREPARED_STMT *statement;
  bool updated;

  statement = mysql_stmt_create(conn);
  updated = statement != NULL && mysql_stmt_prepare_query(statement, sql) &&
            mysql_stmt_bind_param_int(statement, 0, value) &&
            mysql_stmt_bind_param_int(statement, 1, id) && mysql_stmt_execute_prepared(statement);
  mysql_stmt_cleanup(statement);
  return updated;
}

/**
 * Set a field on a prototype.
 */
static void vedit_set(struct char_data *ch, int id, const char *field, const char *value)
{
  char query[MAX_STRING_LENGTH];
  char escaped_name[256];
  const char *update_sql;
  MYSQL_RES *result;
  MYSQL_ROW row;
  int ivalue;

  /* Verify the prototype exists first for a clean error message. */
  result = vedit_fetch(ch, id, &row);
  if (result == NULL)
  {
    return;
  }
  mysql_free_result(result);

  if (!str_cmp(field, "name"))
  {
    if (!*value || strlen(value) > 100)
    {
      send_to_char(ch, "Name must be 1-100 characters.\r\n");
      return;
    }
    mysql_real_escape_string(conn, escaped_name, value, strlen(value));
    snprintf(query, sizeof(query), "UPDATE ship_prototypes SET name='%s' WHERE prototype_id=%d",
             escaped_name, id);
  }
  else if (!str_cmp(field, "class"))
  {
    ivalue = parse_int(value);
    if (!isdigit((unsigned char)*value) || ivalue < 0 || ivalue >= NUM_VESSEL_TYPES)
    {
      send_to_char(ch, "Class must be 0-%d.\r\n", NUM_VESSEL_TYPES - 1);
      return;
    }
    snprintf(query, sizeof(query),
             "UPDATE ship_prototypes SET vessel_class=%d WHERE prototype_id=%d", ivalue, id);
  }
  else if (!str_cmp(field, "speed"))
  {
    ivalue = parse_int(value);
    if (ivalue < 1 || ivalue > VESSEL_SPEED_LIMIT)
    {
      send_to_char(ch, "Speed must be 1-%d.\r\n", VESSEL_SPEED_LIMIT);
      return;
    }
    snprintf(query, sizeof(query), "UPDATE ship_prototypes SET max_speed=%d WHERE prototype_id=%d",
             ivalue, id);
  }
  else if (!str_cmp(field, "armor"))
  {
    ivalue = parse_int(value);
    if (ivalue < 0 || ivalue > VESSEL_MAX_PROTOTYPE_ARMOR)
    {
      send_to_char(ch, "Armor must be 0-%d.\r\n", VESSEL_MAX_PROTOTYPE_ARMOR);
      return;
    }
    snprintf(query, sizeof(query), "UPDATE ship_prototypes SET armor=%d WHERE prototype_id=%d",
             ivalue, id);
  }
  else if (!str_cmp(field, "forsale") || !str_cmp(field, "minlevel"))
  {
    if (!str_cmp(field, "forsale"))
    {
      if (!str_cmp(value, "yes") || !str_cmp(value, "1"))
      {
        ivalue = 1;
      }
      else if (!str_cmp(value, "no") || !str_cmp(value, "0"))
      {
        ivalue = 0;
      }
      else
      {
        send_to_char(ch, "For sale must be yes or no.\r\n");
        return;
      }
      update_sql = "UPDATE ship_prototypes SET for_sale = ? WHERE prototype_id = ?";
    }
    else
    {
      ivalue = parse_int(value);
      if (!isdigit((unsigned char)*value) || ivalue < 0 || ivalue > VEDIT_MAX_MIN_LEVEL)
      {
        send_to_char(ch, "Minimum level must be 0 (class minimum) to %d.\r\n", VEDIT_MAX_MIN_LEVEL);
        return;
      }
      update_sql = "UPDATE ship_prototypes SET min_level = ? WHERE prototype_id = ?";
    }
    if (!vedit_update_int_field(update_sql, ivalue, id))
    {
      log("SYSERR: vedit_set could not update %s on prototype %d", field, id);
      send_to_char(ch, "Database error updating prototype.\r\n");
      return;
    }
    send_to_char(ch, "Prototype %d updated: %s = %s.\r\n", id, field, value);
    return;
  }
  else
  {
    send_to_char(
        ch, "Unknown field '%s'. Fields: name, class, speed, armor, forsale, minlevel.\r\n", field);
    return;
  }

  if (mysql_query(conn, query))
  {
    log("SYSERR: vedit_set update failed: %s", mysql_error(conn));
    send_to_char(ch, "Database error updating prototype.\r\n");
    return;
  }

  send_to_char(ch, "Prototype %d updated: %s = %s.\r\n", id, field, value);
}

/**
 * Delete a prototype.
 */
static void vedit_delete(struct char_data *ch, int id)
{
  char query[MAX_STRING_LENGTH];
  MYSQL_RES *result;
  MYSQL_ROW row;

  result = vedit_fetch(ch, id, &row);
  if (result == NULL)
  {
    return;
  }
  mysql_free_result(result);

  snprintf(query, sizeof(query), "DELETE FROM ship_prototypes WHERE prototype_id=%d", id);
  if (mysql_query(conn, query))
  {
    log("SYSERR: vedit_delete failed: %s", mysql_error(conn));
    send_to_char(ch, "Database error deleting prototype.\r\n");
    return;
  }

  send_to_char(ch, "Prototype %d deleted.\r\n", id);
}

/**
 * Find a free greyhawk_ships slot.
 *
 * Slot 1 is reserved for the legacy world-file test vessel. Slot 0 remains
 * reserved because older vehicle and combat relationships use zero for none.
 * A stowed hull keeps her slot.
 *
 * @return Free slot index, or -1 if the fleet is full
 */
static int vedit_find_free_slot(void)
{
  int i;

  for (i = 2; i < GREYHAWK_MAXSHIPS; i++)
  {
    if (!greyhawk_ships[i].active && !greyhawk_ships[i].stowed)
    {
      return i;
    }
  }

  return -1;
}

/**
 * Price a prototype for shipyard sale (study 3.3.1): the class price times
 * 0.5 + 0.25 * armor / class armor + 0.25 * speed / class speed, so a
 * default hull costs the class price.
 */
int vessel_prototype_price(int vclass, int max_speed, int armor)
{
  const struct vessel_class_condition *condition;
  long long class_armor;
  long long class_speed;
  long long price;

  if (vclass < 0 || vclass >= NUM_VESSEL_TYPES)
  {
    vclass = VESSEL_SHIP;
  }
  condition = vessel_class_condition((enum vessel_class)vclass);
  class_armor = MAX(1, condition->beam_armor);
  class_speed = MAX(1, vessel_class_handling((enum vessel_class)vclass)->speed);
  price =
      (long long)condition->price * (2 * class_armor * class_speed + MAX(0, armor) * class_speed +
                                     MAX(0, max_speed) * class_armor);
  return (int)((price + 2 * class_armor * class_speed) / (4 * class_armor * class_speed));
}

/**
 * Spawn a live ship from a prototype into one resolved exterior room.
 *
 * Shared implementation for builder, public/NPC, and player shipyard spawns.
 *
 * @return The new fleet slot, or -1 on failure
 */
static int vessel_spawn_from_prototype_owner_at(struct char_data *ch, int id, const char *owner,
                                                const char *instance_name, room_rnum exterior_room,
                                                int z)
{
  MYSQL_RES *result;
  MYSQL_ROW row;
  struct greyhawk_ship_data *ship;
  struct obj_data *obj;
  char buf[MAX_STRING_LENGTH];
  const char *spawn_name;
  int slot;
  int vclass;
  int max_speed;
  int armor;

  if (exterior_room == NOWHERE)
  {
    if (ch != NULL)
    {
      send_to_char(ch, "There is no exterior room in which to spawn that ship.\r\n");
    }
    return -1;
  }

  result = vedit_fetch(ch, id, &row);
  if (result == NULL)
  {
    return -1;
  }

  vclass = parse_int(row[2]);
  max_speed = parse_int(row[3]);
  armor = parse_int(row[4]);
  spawn_name = instance_name != NULL && *instance_name ? instance_name : row[1];

  if (vclass < 0 || vclass >= NUM_VESSEL_TYPES || max_speed < 1 || max_speed > VESSEL_SPEED_LIMIT ||
      armor < 0 || armor > VESSEL_MAX_PROTOTYPE_ARMOR)
  {
    mysql_free_result(result);
    if (ch != NULL)
    {
      send_to_char(ch, "That prototype contains invalid class, speed, or armor data.\r\n");
    }
    else
    {
      log("SYSERR: NPC vessel prototype %d contains invalid class, speed, or armor data", id);
    }
    return -1;
  }

  if (ch == NULL &&
      !can_vessel_traverse_terrain((enum vessel_class)vclass, world[exterior_room].coords[0],
                                   world[exterior_room].coords[1], z))
  {
    mysql_free_result(result);
    log("SYSERR: NPC vessel prototype %d cannot spawn at (%d,%d,%d) in sector %d", id,
        world[exterior_room].coords[0], world[exterior_room].coords[1], z,
        world[exterior_room].sector_type);
    return -1;
  }

  slot = vedit_find_free_slot();
  if (slot < 0)
  {
    mysql_free_result(result);
    if (ch != NULL)
    {
      send_to_char(ch, "The fleet is full (%d ships) - no free ship slots.\r\n",
                   GREYHAWK_ACTIVE_SHIP_CAPACITY);
    }
    else
    {
      log("Info: NPC vessel prototype %d deferred: fleet is full", id);
    }
    return -1;
  }

  /* Instantiate the generic boardable ship object; it carries the boarding
   * spec proc through its prototype. */
  obj = read_object_reason(VESSEL_BASE_HULL_OBJ_VNUM, VIRTUAL, PERF_ENTITY_VESSEL);
  if (obj == NULL)
  {
    mysql_free_result(result);
    if (ch != NULL)
    {
      send_to_char(ch, "Base ship object %d is missing from the world files - cannot spawn.\r\n",
                   VESSEL_BASE_HULL_OBJ_VNUM);
    }
    else
    {
      log("SYSERR: Base ship object %d is missing; NPC vessel %d cannot spawn",
          VESSEL_BASE_HULL_OBJ_VNUM, id);
    }
    return -1;
  }

  /* Populate the ship slot from the prototype. */
  ship = &greyhawk_ships[slot];
  vessel_periodic_forget(ship);
  memset(ship, 0, sizeof(*ship));
  ship->active = TRUE;
  ship->shipnum = slot;
  vessel_reset_customization(ship);
  ship->prototype_id = id;
  ship->hull_object_vnum = VESSEL_BASE_HULL_OBJ_VNUM;
  strlcpy(ship->name, spawn_name, sizeof(ship->name));
  strlcpy(ship->owner, owner ? owner : "", sizeof(ship->owner));
  ship->id[0] = (char)('A' + (slot / 26) % 26);
  ship->id[1] = (char)('A' + slot % 26);
  ship->id[2] = '\0';
  ship->vessel_type = (enum vessel_class)vclass;
  ship->minspeed = 0;
  ship->maxspeed = (short)max_speed;
  ship->speed = 0;
  ship->setspeed = 0;
  vessel_initialize_condition(ship, armor);
  ship->docked_to_ship = -1;

  /* Default armament by class (3.3.10) */
  vessel_fit_default_weapons(ship);

  /* Anchor the ship at the supplied location; wilderness rooms provide real
   * coordinates while authored rooms retain their stable room VNUM. */
  ship->x = (double)world[exterior_room].coords[0];
  ship->y = (double)world[exterior_room].coords[1];
  ship->z = (double)z;
  ship->location = world[exterior_room].number;

  /* Generate the interior before wiring the object so the entrance vnum is
   * known. add_ship_room() links each interior room back to this ship. */
  generate_ship_interior(ship);
  if (ship->num_rooms <= 0 || ship->entrance_room <= 0)
  {
    mysql_free_result(result);
    extract_obj(obj);
    vessel_periodic_forget(ship);
    memset(ship, 0, sizeof(*ship));
    if (ch != NULL)
    {
      send_to_char(ch, "Interior generation failed - spawn aborted (see syslog).\r\n");
    }
    return -1;
  }

  /* Instance strings may point at prototype strings, so assign fresh copies
   * without freeing the originals. */
  vessel_build_hull_keywords(buf, sizeof(buf), spawn_name);
  obj->name = strdup(buf);
  obj->short_description = strdup(spawn_name);
  vessel_build_hull_description(buf, sizeof(buf), ship);
  obj->description = strdup(buf);

  if (!vessel_place_hull_object(ship, obj))
  {
    vessel_reclaim_interior_rooms(ship, exterior_room);
    extract_obj(obj);
    vessel_periodic_forget(ship);
    memset(ship, 0, sizeof(*ship));
    mysql_free_result(result);
    if (ch != NULL)
    {
      send_to_char(ch, "The ship's exterior could not be placed - spawn aborted.\r\n");
    }
    return -1;
  }

  /* A hull launched in port starts berthed. */
  vessel_sync_berth(ship);

  /* Persist immediately so both the interior and the live instance survive
   * reboot/copyover. Abort the spawn if either half cannot be committed. */
  if (!save_ship_interior(ship) || !vessel_db_save_runtime(ship) || !vessel_db_save_weapons(ship) ||
      !vessel_db_save_owner(ship))
  {
    room_rnum evacuation_room;

    evacuation_room = IN_ROOM(obj);
    vessel_reclaim_interior_rooms(ship, evacuation_room);
    extract_obj(obj);
    vessel_delete_persistence(slot);
    vessel_periodic_forget(ship);
    memset(ship, 0, sizeof(*ship));
    mysql_free_result(result);
    if (ch != NULL)
    {
      send_to_char(ch, "The ship could not be persisted, so the spawn was rolled back.\r\n");
    }
    return -1;
  }

  mysql_free_result(result);

  if (ch != NULL)
  {
    send_to_char(ch, "Spawned '%s' (%s) as ship %d: %d interior rooms, entrance %d, bridge %d.\r\n",
                 ship->name, get_vessel_type_name(ship->vessel_type), slot, ship->num_rooms,
                 ship->entrance_room, ship->bridge_room);
    act("$p materializes, ready to sail.", FALSE, ch, obj, 0, TO_ROOM);
    log("Info: %s spawned ship %d '%s' from prototype %d at (%d,%d,%d)", GET_NAME(ch), slot,
        ship->name, id, (int)ship->x, (int)ship->y, (int)ship->z);
  }
  else
  {
    log("Info: NPC vessel manager spawned ship %d '%s' from prototype %d at "
        "(%d,%d,%d)",
        slot, ship->name, id, (int)ship->x, (int)ship->y, (int)ship->z);
  }
  vessel_periodic_sync(ship);
  return slot;
}

/**
 * Spawn an owned ship from a prototype.
 */
int vessel_spawn_from_prototype(struct char_data *ch, int id)
{
  return vessel_spawn_from_prototype_owner_at(ch, id, GET_NAME(ch), NULL, IN_ROOM(ch), 0);
}

/**
 * Spawn an unclaimed ship for a public route or NPC pilot.
 */
static int vessel_spawn_public_from_prototype(struct char_data *ch, int id)
{
  int slot;

  slot = vessel_spawn_from_prototype_owner_at(ch, id, "", NULL, IN_ROOM(ch), 0);
  if (slot >= 0)
  {
    send_to_char(ch, "Ship %d is public and unclaimed; it will not accrue owner dock fees.\r\n",
                 slot);
  }
  return slot;
}

/**
 * Spawn one unowned public/NPC hull at wilderness coordinates.
 *
 * This is the non-character entry point used by the durable merchant
 * lifecycle. It shares the production constructor and persistence rollback
 * used by `vedit spawnpublic`.
 */
int vessel_spawn_public_from_prototype_at(int id, const char *instance_name, int x, int y, int z)
{
  room_rnum exterior_room;

  if (id <= 0 || instance_name == NULL || !*instance_name ||
      strlen(instance_name) >= sizeof(greyhawk_ships[0].name))
  {
    return -1;
  }

  exterior_room = get_or_allocate_wilderness_room(x, y);
  if (exterior_room == NOWHERE)
  {
    log("SYSERR: NPC vessel '%s' could not allocate wilderness room (%d,%d)", instance_name, x, y);
    return -1;
  }

  return vessel_spawn_from_prototype_owner_at(NULL, id, "", instance_name, exterior_room, z);
}

/**
 * shipbrowse - view the shipyard catalog (all prototypes with prices).
 */
ACMD(do_shipbrowse)
{
  MYSQL_RES *result;
  MYSQL_ROW row;

  if (!vessel_prototype_db_ready())
  {
    send_to_char(ch, "The shipwright's records are unavailable.\r\n");
    return;
  }

  if (mysql_query(conn, "SELECT prototype_id, name, vessel_class, max_speed, armor, min_level "
                        "FROM ship_prototypes WHERE for_sale = 1 ORDER BY prototype_id"))
  {
    send_to_char(ch, "The shipwright's records are unavailable.\r\n");
    return;
  }

  result = mysql_store_result(conn);
  if (result == NULL)
  {
    send_to_char(ch, "The shipwright's records are unavailable.\r\n");
    return;
  }

  send_to_char(ch, "The shipwright's catalog:\r\n");
  send_to_char(ch, "ID    Class          Speed Armor Price      Lvl Name\r\n");
  send_to_char(ch,
               "----- -------------- ----- ----- ---------- --- ----------------------------\r\n");
  while ((row = mysql_fetch_row(result)) != NULL)
  {
    send_to_char(ch, "%-5s %-14s %-5s %-5s %-10d %-3d %s\r\n", row[0],
                 get_vessel_type_name((enum vessel_class)parse_int(row[2])), row[3], row[4],
                 vessel_prototype_price(parse_int(row[2]), parse_int(row[3]), parse_int(row[4])),
                 vessel_prototype_min_level(parse_int(row[2]), parse_int(row[5])), row[1]);
  }
  mysql_free_result(result);
  send_to_char(ch, "Purchase with 'shipbuy <id>' at any dock, or 'shipbuy <id> trade' to trade "
                   "in your hull berthed there. Lvl is the level needed to take her out of "
                   "port.\r\n");
}

/**
 * Trade in the owner's hull berthed at this dock for a new hull from a
 * prototype (study 3.3.7): 90% of her value comes off the price, and more
 * is paid out. She is rebuilt in place keeping her name, crew, cosmetics,
 * and permits; the new hull comes with her class armament, takes aboard
 * what of the old fit-out she legally can, and the shipwrights buy the rest.
 */
static void vessel_trade_in(struct char_data *ch, int id, int vclass, int max_speed, int armor,
                            int price)
{
  struct greyhawk_ship_slot old_slots[GREYHAWK_MAXSLOTS];
  struct greyhawk_ship_data *ship;
  enum vessel_class old_class;
  room_rnum dock;
  int credit;
  int sold;
  int net;
  int i;

  dock = IN_ROOM(ch);
  ship = NULL;
  for (i = 2; i < GREYHAWK_MAXSHIPS && ship == NULL; i++)
  {
    if (is_valid_ship(&greyhawk_ships[i]) && !str_cmp(greyhawk_ships[i].owner, GET_NAME(ch)) &&
        greyhawk_ships[i].dock == (int)world[dock].number)
    {
      ship = &greyhawk_ships[i];
    }
  }
  if (ship == NULL)
  {
    send_to_char(ch, "You have no hull berthed here to trade in.\r\n");
    return;
  }
  if (vessel_has_cargo(ship))
  {
    send_to_char(ch, "Empty %s's hold before you trade her in.\r\n", ship->name);
    return;
  }
  if (ship->departure_ticks > 0 || ship->docked_to_ship > 0)
  {
    send_to_char(ch,
                 "%s is casting off or has a hull alongside; the shipwrights cannot take her.\r\n",
                 ship->name);
    return;
  }
  if (ship->autopilot != NULL && ship->autopilot->pilot_mob_vnum != -1)
  {
    send_to_char(ch, "Unassign %s's NPC pilot before you trade her in.\r\n", ship->name);
    return;
  }

  /* Her insurance already paid for a lost hull, so a wreck earns nothing. */
  credit = ship->wreck_hull ? 0 : vessel_hull_price(ship) * VESSEL_TRADE_IN_PERCENT / 100;
  if (price - credit > GET_GOLD(ch))
  {
    send_to_char(ch, "With %d gold for %s the new hull costs %d more; you have %d.\r\n", credit,
                 ship->name, price - credit, GET_GOLD(ch));
    return;
  }

  memcpy(old_slots, ship->slot, sizeof(old_slots));
  old_class = ship->vessel_type;
  vehicle_release_all_from_vessel(ship, dock);
  vessel_reclaim_interior_rooms(ship, dock);
  if (!vessel_rebuild_hull(ship, id, vclass, max_speed, armor))
  {
    log("SYSERR: Ship %d lost her interior in a trade-in", ship->shipnum);
    send_to_char(ch, "The shipwrights botch the work; tell the staff.\r\n");
    return;
  }
  vessel_fit_default_weapons(ship);
  sold = vessel_carry_fitout(ship, old_slots, old_class);
  vessel_place_hull_object(ship, ship->shipobj);
  vessel_refresh_hull_strings(ship, FALSE);
  if (!vessel_save_one(ship))
  {
    log("SYSERR: Traded-in ship %d could not be saved completely", ship->shipnum);
  }

  net = credit + sold - price;
  award_gold(ch, net);
  send_to_char(ch,
               "The shipwrights take %s in trade for %d gold%s and rebuild her as a %s. You %s "
               "%d gold.\r\n",
               ship->name, credit, sold > 0 ? " (and buy the fittings she cannot carry)" : "",
               get_vessel_type_name(ship->vessel_type), net < 0 ? "pay" : "receive",
               net < 0 ? -net : net);
  log("Info: %s traded ship %d in for prototype %d (%d gold)", GET_NAME(ch), ship->shipnum, id,
      net);
}

/**
 * shipbuy <id> [trade] - purchase and take delivery of a hull at a dock, or
 * trade in your hull berthed there for it.
 */
ACMD(do_shipbuy)
{
  MYSQL_RES *result;
  MYSQL_ROW row;
  char arg[MAX_INPUT_LENGTH];
  char mode[MAX_INPUT_LENGTH];
  bool for_sale;
  int id;
  int price;
  int slot;
  int vclass;
  int max_speed;
  int armor;

  if (IS_NPC(ch))
  {
    send_to_char(ch, "NPCs cannot buy ships.\r\n");
    return;
  }

  if (!vessel_room_is_port(IN_ROOM(ch)))
  {
    send_to_char(ch, "Ships are bought and delivered at a dock.\r\n");
    return;
  }

  if (vessel_port_refuses(ch))
  {
    return;
  }

  two_arguments(argument, arg, sizeof(arg), mode, sizeof(mode));
  if (!*arg || (*mode && str_cmp(mode, "trade") != 0))
  {
    send_to_char(ch, "Usage: shipbuy <id> [trade]. See 'shipbrowse' for the catalog.\r\n");
    return;
  }
  id = parse_int(arg);

  if (!vessel_prototype_db_ready())
  {
    send_to_char(ch, "The shipwright's records are unavailable.\r\n");
    return;
  }

  result = vedit_fetch(ch, id, &row);
  if (result == NULL)
  {
    return;
  }
  vclass = parse_int(row[2]);
  max_speed = parse_int(row[3]);
  armor = parse_int(row[4]);
  price = vessel_prototype_price(vclass, max_speed, armor);
  for_sale = parse_int(row[5]) != 0;
  mysql_free_result(result);

  if (!for_sale)
  {
    send_to_char(ch, "The shipwright does not sell that hull. See 'shipbrowse'.\r\n");
    return;
  }

  if (*mode)
  {
    vessel_trade_in(ch, id, vclass, max_speed, armor, price);
    return;
  }

  if (vessel_owner_at_cap(ch))
  {
    send_to_char(ch, "You already own %d hulls, the most one captain may hold.\r\n",
                 vessel_owned_hull_count(GET_NAME(ch)));
    return;
  }

  if (GET_GOLD(ch) < price)
  {
    send_to_char(ch, "That hull costs %d gold coins; you have %d.\r\n", price, GET_GOLD(ch));
    return;
  }

  slot = vessel_spawn_from_prototype(ch, id);
  if (slot < 0)
  {
    return; /* Spawn failed; no charge */
  }

  award_gold(ch, -price);
  send_to_char(ch,
               "You pay %d gold coins. Fair winds, captain - christen her with "
               "'shipchristen <name>'.\r\n",
               price);
  log("Info: %s bought ship %d for %d gold", GET_NAME(ch), slot, price);
}

/**
 * Whether a hull still bears the name of the prototype she was built from:
 * her first christening is free.
 */
static bool vessel_bears_prototype_name(const struct greyhawk_ship_data *ship)
{
  MYSQL_RES *result;
  MYSQL_ROW row;
  bool same;

  if (ship->prototype_id <= 0 || !vessel_prototype_db_ready())
  {
    return FALSE;
  }
  result = vedit_fetch(NULL, ship->prototype_id, &row);
  if (result == NULL)
  {
    return FALSE;
  }
  same = row[1] != NULL && !str_cmp(row[1], ship->name);
  mysql_free_result(result);
  return same;
}

/**
 * shipchristen <name> - rename a ship you own. The first christening is
 * free; a rename costs a tenth of her value (study 3.3.7).
 */
ACMD(do_shipchristen)
{
  struct greyhawk_ship_data *ship;
  const char *name;
  size_t i;
  int fee;

  ship = get_ship_from_room(IN_ROOM(ch));
  if (ship == NULL)
  {
    send_to_char(ch, "You must be aboard your ship to christen her.\r\n");
    return;
  }

  if (str_cmp(ship->owner, GET_NAME(ch)) != 0 && GET_LEVEL(ch) < LVL_IMMORT)
  {
    send_to_char(ch, "Only the owner may christen this vessel.\r\n");
    return;
  }

  name = argument;
  skip_spaces_c(&name);
  if (!*name || strlen(name) < 3 || strlen(name) > 60)
  {
    send_to_char(ch, "Ship names run 3 to 60 characters.\r\n");
    return;
  }
  for (i = 0; name[i]; i++)
  {
    if (!isprint((unsigned char)name[i]))
    {
      send_to_char(ch, "Ship names must be plain printable text.\r\n");
      return;
    }
  }

  fee = 0;
  if (GET_LEVEL(ch) < LVL_IMMORT && !vessel_bears_prototype_name(ship))
  {
    fee = vessel_hull_price(ship) * VESSEL_RENAME_FEE_PERCENT / 100;
  }
  if (GET_GOLD(ch) < fee)
  {
    send_to_char(ch, "The registry charges %d gold to rename %s; you have %d.\r\n", fee, ship->name,
                 GET_GOLD(ch));
    return;
  }
  if (fee > 0)
  {
    award_gold(ch, -fee);
    send_to_char(ch, "You pay the registry %d gold.\r\n", fee);
  }

  log("Info: %s christened ship %d '%s' as '%s' (%d gold)", GET_NAME(ch), ship->shipnum, ship->name,
      name, fee);
  strlcpy(ship->name, name, sizeof(ship->name));

  vessel_refresh_hull_strings(ship, TRUE);

  save_ship_interior(ship);
  vessel_db_save_owner(ship);
  send_to_ship(ship, "By her owner's word, this vessel is christened %s!", ship->name);
}

/**
 * shipcustomize [paint|figurehead] <description|clear> - set exterior details.
 */
ACMD(do_shipcustomize)
{
  struct greyhawk_ship_data *ship;
  char field[MAX_INPUT_LENGTH];
  char value[MAX_INPUT_LENGTH];
  char old_value[VESSEL_CUSTOMIZATION_LENGTH];
  char *end;
  const char *remainder;
  const char *current_value;
  const char *field_name;
  bool paint_field;
  size_t i;

  ship = get_ship_from_room(IN_ROOM(ch));
  if (ship == NULL)
  {
    send_to_char(ch, "You must be aboard your ship to customize her.\r\n");
    return;
  }

  if (str_cmp(ship->owner, GET_NAME(ch)) != 0 && GET_LEVEL(ch) < LVL_IMMORT)
  {
    send_to_char(ch, "Only the owner may customize this vessel.\r\n");
    return;
  }

  remainder = one_argument(argument, field, sizeof(field));
  if (!*field || !str_cmp(field, "show"))
  {
    send_to_char(ch, "Vessel customization for %s:\r\n", ship->name);
    send_to_char(ch, "  Paint:      %s\r\n",
                 *vessel_paint_scheme(ship) ? vessel_paint_scheme(ship) : "(none)");
    send_to_char(ch, "  Figurehead: %s\r\n",
                 *vessel_figurehead(ship) ? vessel_figurehead(ship) : "(none)");
    send_to_char(ch, "Usage: shipcustomize <paint|figurehead> <description|clear>\r\n");
    return;
  }

  if (is_abbrev(field, "paint"))
  {
    paint_field = TRUE;
    field_name = "paint";
  }
  else if (is_abbrev(field, "figurehead"))
  {
    paint_field = FALSE;
    field_name = "figurehead";
  }
  else
  {
    send_to_char(ch, "Customize either the paint or figurehead.\r\n");
    return;
  }

  skip_spaces_c(&remainder);
  strlcpy(value, remainder, sizeof(value));
  end = value + strlen(value);
  while (end > value && isspace((unsigned char)end[-1]))
  {
    *--end = '\0';
  }

  if (!str_cmp(value, "clear"))
  {
    value[0] = '\0';
  }
  else if (strlen(value) < 3 || strlen(value) >= VESSEL_CUSTOMIZATION_LENGTH)
  {
    send_to_char(ch, "Customization descriptions run 3 to 80 characters.\r\n");
    return;
  }

  for (i = 0; value[i]; i++)
  {
    if (!isprint((unsigned char)value[i]))
    {
      send_to_char(ch, "Customization descriptions must be plain printable text.\r\n");
      return;
    }
  }

  current_value = paint_field ? vessel_paint_scheme(ship) : vessel_figurehead(ship);
  strlcpy(old_value, current_value, sizeof(old_value));
  if (paint_field)
  {
    vessel_set_paint_scheme(ship, value);
  }
  else
  {
    vessel_set_figurehead(ship, value);
  }
  if (ship->shipobj != NULL && !vessel_refresh_hull_strings(ship, FALSE))
  {
    if (paint_field)
    {
      vessel_set_paint_scheme(ship, old_value);
    }
    else
    {
      vessel_set_figurehead(ship, old_value);
    }
    send_to_char(ch, "The ship's exterior could not be updated. Try again.\r\n");
    return;
  }

  if (!save_ship_interior(ship))
  {
    if (paint_field)
    {
      vessel_set_paint_scheme(ship, old_value);
    }
    else
    {
      vessel_set_figurehead(ship, old_value);
    }
    if (ship->shipobj != NULL)
    {
      vessel_refresh_hull_strings(ship, FALSE);
    }
    send_to_char(ch, "The customization could not be saved, so nothing changed.\r\n");
    return;
  }

  if (value[0])
  {
    send_to_ship(ship, "%s updates the vessel's %s to %s.", GET_NAME(ch), field_name, value);
  }
  else
  {
    send_to_ship(ship, "%s clears the vessel's %s customization.", GET_NAME(ch), field_name);
  }
}

/**
 * vedit - ship prototype editor entry point.
 */
ACMD(do_vedit)
{
  char arg1[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH];
  char arg3[MAX_INPUT_LENGTH];
  const char *remainder;

  if (IS_NPC(ch))
  {
    send_to_char(ch, "NPCs cannot edit ship prototypes.\r\n");
    return;
  }

  remainder = one_argument(argument, arg1, sizeof(arg1));

  if (!*arg1)
  {
    send_to_char(ch, "%s", VEDIT_USAGE);
    return;
  }

  if (!vessel_prototype_db_ready())
  {
    send_to_char(ch, "The ship prototype database is unavailable.\r\n");
    return;
  }

  if (!str_cmp(arg1, "list"))
  {
    vedit_list(ch);
  }
  else if (!str_cmp(arg1, "new"))
  {
    remainder = one_argument(remainder, arg2, sizeof(arg2));
    skip_spaces_c(&remainder);
    vedit_new(ch, arg2, remainder);
  }
  else if (!str_cmp(arg1, "show"))
  {
    one_argument(remainder, arg2, sizeof(arg2));
    if (!*arg2)
    {
      send_to_char(ch, "%s", VEDIT_USAGE);
      return;
    }
    vedit_show(ch, parse_int(arg2));
  }
  else if (!str_cmp(arg1, "set"))
  {
    remainder = one_argument(remainder, arg2, sizeof(arg2));
    remainder = one_argument(remainder, arg3, sizeof(arg3));
    skip_spaces_c(&remainder);
    if (!*arg2 || !*arg3 || !*remainder)
    {
      send_to_char(ch, "%s", VEDIT_USAGE);
      return;
    }
    vedit_set(ch, parse_int(arg2), arg3, remainder);
  }
  else if (!str_cmp(arg1, "delete"))
  {
    one_argument(remainder, arg2, sizeof(arg2));
    if (!*arg2)
    {
      send_to_char(ch, "%s", VEDIT_USAGE);
      return;
    }
    vedit_delete(ch, parse_int(arg2));
  }
  else if (!str_cmp(arg1, "spawn"))
  {
    one_argument(remainder, arg2, sizeof(arg2));
    if (!*arg2)
    {
      send_to_char(ch, "%s", VEDIT_USAGE);
      return;
    }
    vessel_spawn_from_prototype(ch, parse_int(arg2));
  }
  else if (!str_cmp(arg1, "spawnpublic"))
  {
    one_argument(remainder, arg2, sizeof(arg2));
    if (!*arg2)
    {
      send_to_char(ch, "%s", VEDIT_USAGE);
      return;
    }
    vessel_spawn_public_from_prototype(ch, parse_int(arg2));
  }
  else
  {
    send_to_char(ch, "%s", VEDIT_USAGE);
  }
}
