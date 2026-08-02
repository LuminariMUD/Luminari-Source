/* ************************************************************************
 *      File:   vessels_edit.c                        Part of LuminariMUD  *
 *   Purpose:   vedit - ship prototype editor (Phase 04, Session 03)        *
 *              Builder-facing tool: author vessel prototypes in the        *
 *              ship_prototypes table and spawn live ships from them        *
 *              without recompiling.                                        *
 * ********************************************************************** */

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

extern MYSQL *conn;
extern bool mysql_available;
extern struct greyhawk_ship_data greyhawk_ships[GREYHAWK_MAXSHIPS];

/* Bounds for editable prototype fields */
#define VEDIT_MAX_SPEED_LIMIT 30
#define VEDIT_MAX_ARMOR_LIMIT 100

static const char *VEDIT_USAGE =
    "Usage:\r\n"
    "  vedit list                      - list all ship prototypes\r\n"
    "  vedit new <class> <name>        - create a prototype (class 0-7)\r\n"
    "  vedit show <id>                 - show one prototype\r\n"
    "  vedit set <id> <field> <value>  - fields: name, class, speed, armor\r\n"
    "  vedit delete <id>               - delete a prototype\r\n"
    "  vedit spawn <id>                - spawn a live ship here from a prototype\r\n"
    "  vedit spawnpublic <id>          - spawn an unclaimed NPC/public ship\r\n"
    "Classes: 0=Raft 1=Boat 2=Ship 3=Warship 4=Airship 5=Submarine 6=Transport 7=Magical\r\n";

/**
 * Ensure the ship_prototypes table exists.
 *
 * Follows the vessel-system convention of auto-creating tables so a fresh
 * database works without manual schema steps. Mirrored by
 * sql/components/vessels_phase4_schema.sql.
 *
 * @return TRUE if the table is available, FALSE otherwise
 */
static bool vedit_ensure_table(void)
{
  const char *create_sql = "CREATE TABLE IF NOT EXISTS ship_prototypes ("
                           "  prototype_id INT AUTO_INCREMENT PRIMARY KEY,"
                           "  name VARCHAR(127) NOT NULL,"
                           "  vessel_class INT NOT NULL DEFAULT 2,"
                           "  max_speed INT NOT NULL DEFAULT 10,"
                           "  armor INT NOT NULL DEFAULT 10,"
                           "  created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP"
                           ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4";

  if (!mysql_available || conn == NULL)
  {
    return FALSE;
  }

  if (mysql_query(conn, create_sql))
  {
    log("SYSERR: vedit_ensure_table failed: %s", mysql_error(conn));
    return FALSE;
  }

  return TRUE;
}

/**
 * List all prototypes to the character.
 */
static void vedit_list(struct char_data *ch)
{
  MYSQL_RES *result;
  MYSQL_ROW row;

  if (mysql_query(conn, "SELECT prototype_id, name, vessel_class, max_speed, armor "
                        "FROM ship_prototypes ORDER BY prototype_id"))
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

  send_to_char(ch, "ID    Class      Speed Armor Name\r\n");
  send_to_char(ch, "----- ---------- ----- ----- ----------------------------\r\n");
  while ((row = mysql_fetch_row(result)) != NULL)
  {
    send_to_char(ch, "%-5s %-10s %-5s %-5s %s\r\n", row[0],
                 get_vessel_type_name((enum vessel_class)atoi(row[2])), row[3], row[4], row[1]);
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

  vclass = atoi(class_arg);
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

  /* Class-flavored defaults; all tunable afterward via 'vedit set'. */
  switch ((enum vessel_class)vclass)
  {
  case VESSEL_RAFT:
    max_speed = 5;
    armor = 2;
    break;
  case VESSEL_BOAT:
    max_speed = 10;
    armor = 5;
    break;
  case VESSEL_WARSHIP:
    max_speed = 20;
    armor = 40;
    break;
  case VESSEL_AIRSHIP:
    max_speed = 25;
    armor = 15;
    break;
  case VESSEL_SUBMARINE:
    max_speed = 8;
    armor = 25;
    break;
  case VESSEL_TRANSPORT:
    max_speed = 8;
    armor = 20;
    break;
  case VESSEL_MAGICAL:
  case VESSEL_SHIP:
  default:
    max_speed = 15;
    armor = 20;
    break;
  }

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
           "SELECT prototype_id, name, vessel_class, max_speed, armor "
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
               "  Armor : %s (all four sides at spawn)\r\n"
               "  Cargo : %d lbs (fixed per class)\r\n",
               row[0], row[1], row[2], get_vessel_type_name((enum vessel_class)atoi(row[2])),
               row[3], row[4], get_vessel_cargo_capacity((enum vessel_class)atoi(row[2])));
  mysql_free_result(result);
}

/**
 * Set a field on a prototype.
 */
static void vedit_set(struct char_data *ch, int id, const char *field, const char *value)
{
  char query[MAX_STRING_LENGTH];
  char escaped_name[256];
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
    ivalue = atoi(value);
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
    ivalue = atoi(value);
    if (ivalue < 1 || ivalue > VEDIT_MAX_SPEED_LIMIT)
    {
      send_to_char(ch, "Speed must be 1-%d.\r\n", VEDIT_MAX_SPEED_LIMIT);
      return;
    }
    snprintf(query, sizeof(query), "UPDATE ship_prototypes SET max_speed=%d WHERE prototype_id=%d",
             ivalue, id);
  }
  else if (!str_cmp(field, "armor"))
  {
    ivalue = atoi(value);
    if (ivalue < 0 || ivalue > VEDIT_MAX_ARMOR_LIMIT)
    {
      send_to_char(ch, "Armor must be 0-%d.\r\n", VEDIT_MAX_ARMOR_LIMIT);
      return;
    }
    snprintf(query, sizeof(query), "UPDATE ship_prototypes SET armor=%d WHERE prototype_id=%d",
             ivalue, id);
  }
  else
  {
    send_to_char(ch, "Unknown field '%s'. Fields: name, class, speed, armor.\r\n", field);
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
 *
 * @return Free slot index, or -1 if the fleet is full
 */
static int vedit_find_free_slot(void)
{
  int i;

  for (i = 2; i < GREYHAWK_MAXSHIPS; i++)
  {
    if (!greyhawk_ships[i].active)
    {
      return i;
    }
  }

  return -1;
}

/**
 * Price a prototype for shipyard sale: class base scaled by armor and
 * speed investment.
 */
int vessel_prototype_price(int vclass, int max_speed, int armor)
{
  static const int class_base[NUM_VESSEL_TYPES] = {
      50,    /* RAFT */
      500,   /* BOAT */
      5000,  /* SHIP */
      20000, /* WARSHIP */
      50000, /* AIRSHIP */
      40000, /* SUBMARINE */
      15000, /* TRANSPORT */
      100000 /* MAGICAL */
  };
  int base;

  if (vclass < 0 || vclass >= NUM_VESSEL_TYPES)
  {
    vclass = VESSEL_SHIP;
  }
  base = class_base[vclass];
  return base + (base * armor) / 50 + (base * max_speed) / 60;
}

/**
 * Spawn a live ship from a prototype into one resolved exterior room.
 *
 * Shared implementation for builder, public/NPC, and player shipyard spawns.
 *
 * @return The new fleet slot, or -1 on failure
 */
static int vessel_spawn_from_prototype_owner_at(struct char_data *ch, int id,
                                                const char *owner,
                                                const char *instance_name,
                                                room_rnum exterior_room, int z)
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

  vclass = atoi(row[2]);
  max_speed = atoi(row[3]);
  armor = atoi(row[4]);
  spawn_name = instance_name != NULL && *instance_name ? instance_name : row[1];

  if (vclass < 0 || vclass >= NUM_VESSEL_TYPES ||
      max_speed < 1 || max_speed > VEDIT_MAX_SPEED_LIMIT ||
      armor < 0 || armor > VEDIT_MAX_ARMOR_LIMIT)
  {
    mysql_free_result(result);
    if (ch != NULL)
    {
      send_to_char(ch, "That prototype contains invalid class, speed, or armor data.\r\n");
    }
    else
    {
      log("SYSERR: NPC vessel prototype %d contains invalid class, speed, or armor data",
          id);
    }
    return -1;
  }

  if (ch == NULL &&
      !can_vessel_traverse_terrain((enum vessel_class)vclass,
                                   world[exterior_room].coords[0],
                                   world[exterior_room].coords[1], z))
  {
    mysql_free_result(result);
    log("SYSERR: NPC vessel prototype %d cannot spawn at (%d,%d,%d) in sector %d",
        id, world[exterior_room].coords[0], world[exterior_room].coords[1], z,
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
  obj = read_object(VESSEL_BASE_HULL_OBJ_VNUM, VIRTUAL);
  if (obj == NULL)
  {
    mysql_free_result(result);
    if (ch != NULL)
    {
      send_to_char(ch,
                   "Base ship object %d is missing from the world files - cannot spawn.\r\n",
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
  memset(ship, 0, sizeof(*ship));
  ship->active = TRUE;
  ship->shipnum = slot;
  ship->prototype_id = id;
  ship->hull_object_vnum = VESSEL_BASE_HULL_OBJ_VNUM;
  strlcpy(ship->name, spawn_name, sizeof(ship->name));
  strlcpy(ship->owner, owner ? owner : "", sizeof(ship->owner));
  ship->id[0] = 'A' + (slot / 26) % 26;
  ship->id[1] = 'A' + slot % 26;
  ship->id[2] = '\0';
  ship->vessel_type = (enum vessel_class)vclass;
  ship->minspeed = 0;
  ship->maxspeed = max_speed;
  ship->speed = 0;
  ship->setspeed = 0;
  vessel_initialize_condition(ship, armor);
  ship->docked_to_ship = -1;

  /* Default armament by class (slot layout: type 1 = weapon; val0 = long
   * range, val2/val3 = damage dice). Warships get a broadside pair plus a
   * bow chaser; other armed hulls carry a single fore ballista. */
  switch (ship->vessel_type)
  {
  case VESSEL_WARSHIP:
    ship->slot[0].type = 1;
    ship->slot[0].position = GREYHAWK_FORE;
    ship->slot[0].val0 = 50;
    ship->slot[0].val2 = 2;
    ship->slot[0].val3 = 8;
    strlcpy(ship->slot[0].desc, "the bow chaser ballista",
            sizeof(ship->slot[0].desc));
    ship->slot[1].type = 1;
    ship->slot[1].position = GREYHAWK_PORT;
    ship->slot[1].val0 = 50;
    ship->slot[1].val2 = 2;
    ship->slot[1].val3 = 8;
    strlcpy(ship->slot[1].desc, "the port ballista battery",
            sizeof(ship->slot[1].desc));
    ship->slot[2].type = 1;
    ship->slot[2].position = GREYHAWK_STARBOARD;
    ship->slot[2].val0 = 50;
    ship->slot[2].val2 = 2;
    ship->slot[2].val3 = 8;
    strlcpy(ship->slot[2].desc, "the starboard ballista battery",
            sizeof(ship->slot[2].desc));
    break;
  case VESSEL_SHIP:
  case VESSEL_TRANSPORT:
  case VESSEL_AIRSHIP:
  case VESSEL_SUBMARINE:
  case VESSEL_MAGICAL:
    ship->slot[0].type = 1;
    ship->slot[0].position = GREYHAWK_FORE;
    ship->slot[0].val0 = 40;
    ship->slot[0].val2 = 1;
    ship->slot[0].val3 = 8;
    strlcpy(ship->slot[0].desc, "a light ballista",
            sizeof(ship->slot[0].desc));
    break;
  case VESSEL_RAFT:
  case VESSEL_BOAT:
  default:
    break; /* Unarmed */
  }

  /* Anchor the ship at the supplied location; wilderness rooms provide real
   * coordinates while authored rooms retain their stable room VNUM. */
  ship->x = (float)world[exterior_room].coords[0];
  ship->y = (float)world[exterior_room].coords[1];
  ship->z = (float)z;
  ship->location = world[exterior_room].number;

  /* Generate the interior before wiring the object so the entrance vnum is
   * known. add_ship_room() links each interior room back to this ship. */
  generate_ship_interior(ship);
  if (ship->num_rooms <= 0 || ship->entrance_room <= 0)
  {
    mysql_free_result(result);
    extract_obj(obj);
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
  snprintf(buf, sizeof(buf), "%s is moored here.", spawn_name);
  obj->description = strdup(buf);

  if (!vessel_place_hull_object(ship, obj))
  {
    vessel_reclaim_interior_rooms(ship, exterior_room);
    extract_obj(obj);
    memset(ship, 0, sizeof(*ship));
    mysql_free_result(result);
    if (ch != NULL)
    {
      send_to_char(ch, "The ship's exterior could not be placed - spawn aborted.\r\n");
    }
    return -1;
  }

  /* Persist immediately so both the interior and the live instance survive
   * reboot/copyover. Abort the spawn if either half cannot be committed. */
  if (!save_ship_interior(ship) || !vessel_db_save_runtime(ship) ||
      !vessel_db_save_weapons(ship) || !vessel_db_save_owner(ship))
  {
    room_rnum evacuation_room;

    evacuation_room = IN_ROOM(obj);
    vessel_reclaim_interior_rooms(ship, evacuation_room);
    extract_obj(obj);
    vessel_delete_persistence(slot);
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
    send_to_char(
        ch,
        "Spawned '%s' (%s) as ship %d: %d interior rooms, entrance %d, bridge %d.\r\n",
        ship->name, get_vessel_type_name(ship->vessel_type), slot, ship->num_rooms,
        ship->entrance_room, ship->bridge_room);
    act("$p materializes, ready to sail.", FALSE, ch, obj, 0, TO_ROOM);
    log("Info: %s spawned ship %d '%s' from prototype %d at (%d,%d,%d)",
        GET_NAME(ch), slot, ship->name, id, (int)ship->x, (int)ship->y,
        (int)ship->z);
  }
  else
  {
    log("Info: NPC vessel manager spawned ship %d '%s' from prototype %d at "
        "(%d,%d,%d)",
        slot, ship->name, id, (int)ship->x, (int)ship->y, (int)ship->z);
  }
  return slot;
}

/**
 * Spawn an owned ship from a prototype.
 */
int vessel_spawn_from_prototype(struct char_data *ch, int id)
{
  return vessel_spawn_from_prototype_owner_at(ch, id, GET_NAME(ch), NULL,
                                               IN_ROOM(ch), 0);
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
int vessel_spawn_public_from_prototype_at(int id, const char *instance_name,
                                          int x, int y, int z)
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
    log("SYSERR: NPC vessel '%s' could not allocate wilderness room (%d,%d)",
        instance_name, x, y);
    return -1;
  }

  return vessel_spawn_from_prototype_owner_at(NULL, id, "", instance_name,
                                               exterior_room, z);
}

/**
 * shipbrowse - view the shipyard catalog (all prototypes with prices).
 */
ACMD(do_shipbrowse)
{
  MYSQL_RES *result;
  MYSQL_ROW row;

  if (!vedit_ensure_table())
  {
    send_to_char(ch, "The shipwright's records are unavailable.\r\n");
    return;
  }

  if (mysql_query(conn, "SELECT prototype_id, name, vessel_class, max_speed, armor "
                        "FROM ship_prototypes ORDER BY prototype_id"))
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
  send_to_char(ch, "ID    Class      Speed Armor Price      Name\r\n");
  send_to_char(ch, "----- ---------- ----- ----- ---------- ----------------------------\r\n");
  while ((row = mysql_fetch_row(result)) != NULL)
  {
    send_to_char(ch, "%-5s %-10s %-5s %-5s %-10d %s\r\n", row[0],
                 get_vessel_type_name((enum vessel_class)atoi(row[2])), row[3], row[4],
                 vessel_prototype_price(atoi(row[2]), atoi(row[3]), atoi(row[4])), row[1]);
  }
  mysql_free_result(result);
  send_to_char(ch, "Purchase with 'shipbuy <id>' at any dock.\r\n");
}

/**
 * shipbuy <id> - purchase and take delivery of a hull at a dock.
 */
ACMD(do_shipbuy)
{
  MYSQL_RES *result;
  MYSQL_ROW row;
  char arg[MAX_INPUT_LENGTH];
  int id;
  int price;
  int slot;

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

  one_argument_u((char *)argument, arg);
  if (!*arg)
  {
    send_to_char(ch, "Buy which hull? See 'shipbrowse' for the catalog.\r\n");
    return;
  }
  id = atoi(arg);

  if (!vedit_ensure_table())
  {
    send_to_char(ch, "The shipwright's records are unavailable.\r\n");
    return;
  }

  result = vedit_fetch(ch, id, &row);
  if (result == NULL)
  {
    return;
  }
  price = vessel_prototype_price(atoi(row[2]), atoi(row[3]), atoi(row[4]));
  mysql_free_result(result);

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

  GET_GOLD(ch) -= price;
  send_to_char(ch,
               "You pay %d gold coins. Fair winds, captain - christen her with "
               "'shipchristen <name>'.\r\n",
               price);
  log("Info: %s bought ship %d for %d gold", GET_NAME(ch), slot, price);
}

/**
 * shipchristen <name> - rename a ship you own.
 */
ACMD(do_shipchristen)
{
  struct greyhawk_ship_data *ship;
  char buf[MAX_STRING_LENGTH];
  const char *name;
  size_t i;

  ship = get_ship_from_room(IN_ROOM(ch));
  if (ship == NULL)
  {
    send_to_char(ch, "You must be aboard your ship to christen her.\r\n");
    return;
  }

  if (str_cmp(ship->owner, GET_NAME(ch)) && GET_LEVEL(ch) < LVL_IMMORT)
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

  log("Info: %s christened ship %d '%s' as '%s'", GET_NAME(ch), ship->shipnum, ship->name, name);
  strlcpy(ship->name, name, sizeof(ship->name));

  if (ship->shipobj != NULL)
  {
    vessel_build_hull_keywords(buf, sizeof(buf), ship->name);
    ship->shipobj->name = strdup(buf);
    ship->shipobj->short_description = strdup(ship->name);
    snprintf(buf, sizeof(buf), "%s is moored here.", ship->name);
    ship->shipobj->description = strdup(buf);
  }

  save_ship_interior(ship);
  vessel_db_save_owner(ship);
  send_to_ship(ship, "By her owner's word, this vessel is christened %s!", ship->name);
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

  remainder = one_argument_u((char *)argument, arg1);

  if (!*arg1)
  {
    send_to_char(ch, "%s", VEDIT_USAGE);
    return;
  }

  if (!vedit_ensure_table())
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
    remainder = one_argument_u((char *)remainder, arg2);
    skip_spaces_c(&remainder);
    vedit_new(ch, arg2, remainder);
  }
  else if (!str_cmp(arg1, "show"))
  {
    one_argument_u((char *)remainder, arg2);
    if (!*arg2)
    {
      send_to_char(ch, "%s", VEDIT_USAGE);
      return;
    }
    vedit_show(ch, atoi(arg2));
  }
  else if (!str_cmp(arg1, "set"))
  {
    remainder = one_argument_u((char *)remainder, arg2);
    remainder = one_argument_u((char *)remainder, arg3);
    skip_spaces_c(&remainder);
    if (!*arg2 || !*arg3 || !*remainder)
    {
      send_to_char(ch, "%s", VEDIT_USAGE);
      return;
    }
    vedit_set(ch, atoi(arg2), arg3, remainder);
  }
  else if (!str_cmp(arg1, "delete"))
  {
    one_argument_u((char *)remainder, arg2);
    if (!*arg2)
    {
      send_to_char(ch, "%s", VEDIT_USAGE);
      return;
    }
    vedit_delete(ch, atoi(arg2));
  }
  else if (!str_cmp(arg1, "spawn"))
  {
    one_argument_u((char *)remainder, arg2);
    if (!*arg2)
    {
      send_to_char(ch, "%s", VEDIT_USAGE);
      return;
    }
    vessel_spawn_from_prototype(ch, atoi(arg2));
  }
  else if (!str_cmp(arg1, "spawnpublic"))
  {
    one_argument_u((char *)remainder, arg2);
    if (!*arg2)
    {
      send_to_char(ch, "%s", VEDIT_USAGE);
      return;
    }
    vessel_spawn_public_from_prototype(ch, atoi(arg2));
  }
  else
  {
    send_to_char(ch, "%s", VEDIT_USAGE);
  }
}
