/* ************************************************************************
 *      File:   vessels_rooms.c                      Part of LuminariMUD  *
 *   Purpose:   Phase 2 Multi-room vessel interior system                 *
 *  Author:     Zusuk                                                     *
 * ********************************************************************** */

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "vessels.h"
#include "constants.h"
#include "act.h"
#include "spec_procs.h"
#include "modify.h"
#include "dg_scripts.h"
#include "mysql.h"
#include "genwld.h"
#include "genzon.h"

/* External variables */
extern struct greyhawk_ship_data greyhawk_ships[GREYHAWK_MAXSHIPS];
extern struct room_data *world;
extern room_rnum top_of_world;

extern MYSQL *conn;
extern bool mysql_available;

#define NUM_SHIP_ROOM_TYPES (ROOM_TYPE_DECK + 1)
#define MAX_SHIP_ROOM_TEMPLATE_TRIGGERS 8

/* Room template definitions */
struct room_template
{
  enum ship_room_type type;
  const char *name_format;
  const char *description_format;
  int room_flags;
  int sector_type;
  int min_vessel_size;
} room_templates[] = {
    {ROOM_TYPE_BRIDGE, "The Bridge of %s",
     "This is the command center of %s. Navigation charts cover the walls,\n"
     "and the ship's wheel stands prominently at the center. Through the windows,\n"
     "you can see the vast expanse beyond.",
     ROOM_VEHICLE | ROOM_INDOORS, SECT_INSIDE, VESSEL_BOAT},

    {ROOM_TYPE_QUARTERS, "Crew Quarters aboard %s",
     "These are the crew quarters of %s. Hammocks and bunks line the walls,\n"
     "with personal effects stored in sea chests. The air carries the scent\n"
     "of salt and tar.",
     ROOM_VEHICLE | ROOM_INDOORS, SECT_INSIDE, VESSEL_BOAT},

    {ROOM_TYPE_CARGO, "Cargo Hold of %s",
     "This cavernous cargo hold of %s is filled with crates, barrels, and\n"
     "various supplies. The wooden beams creak softly with the ship's movement.\n"
     "Shadows dance in the dim light filtering through the hatches above.",
     ROOM_VEHICLE | ROOM_INDOORS, SECT_INSIDE, VESSEL_RAFT},

    {ROOM_TYPE_ENGINEERING, "Engine Room of %s",
     "The heart of %s beats here in the engine room. Massive machinery fills\n"
     "the space, with pipes and gauges covering every surface. The air is thick\n"
     "with the smell of oil and the heat of working engines.",
     ROOM_VEHICLE | ROOM_INDOORS, SECT_INSIDE, VESSEL_SHIP},

    {ROOM_TYPE_WEAPONS, "Weapons Bay of %s",
     "This is the weapons bay of %s. Cannons line the walls, their brass\n"
     "fittings gleaming. Racks of ammunition and powder kegs are secured\n"
     "against the bulkheads. Gun ports can be opened for battle.",
     ROOM_VEHICLE | ROOM_INDOORS, SECT_INSIDE, VESSEL_WARSHIP},

    {ROOM_TYPE_MEDICAL, "Medical Bay of %s",
     "The medical bay of %s is equipped with beds and medical supplies.\n"
     "Clean white sheets cover the bunks, and cabinets hold bandages,\n"
     "potions, and surgical instruments.",
     ROOM_VEHICLE | ROOM_INDOORS, SECT_INSIDE, VESSEL_SHIP},

    {ROOM_TYPE_MESS_HALL, "Mess Hall of %s",
     "The mess hall of %s serves as the social center of the vessel.\n"
     "Long tables with benches fill the room, and the lingering aroma\n"
     "of recent meals permeates the air.",
     ROOM_VEHICLE | ROOM_INDOORS, SECT_INSIDE, VESSEL_BOAT},

    {ROOM_TYPE_CORRIDOR, "Corridor aboard %s",
     "This narrow corridor aboard %s connects different sections of the ship.\n"
     "Lanterns provide dim illumination, and the walls are lined with\n"
     "doors leading to various compartments.",
     ROOM_VEHICLE | ROOM_INDOORS, SECT_INSIDE, VESSEL_BOAT},

    {ROOM_TYPE_AIRLOCK, "Airlock of %s",
     "This is an airlock chamber of %s, designed for transitioning between\n"
     "the ship's interior and the outside. Heavy doors seal this compartment\n"
     "from both sides.",
     ROOM_VEHICLE | ROOM_INDOORS, SECT_INSIDE, VESSEL_SUBMARINE},

    {ROOM_TYPE_DECK, "Main Deck of %s",
     "You stand on the main deck of %s. The wind whips across the open space,\n"
     "and you can see the horizon stretching endlessly in all directions.\n"
     "Rigging and masts tower above you.",
     ROOM_VEHICLE, SECT_WATER_SWIM, VESSEL_RAFT}};

/* ========================================================================= */
/* DATA-DRIVEN ROOM TEMPLATES (Phase 04, Session 04)                          */
/* ========================================================================= */
/* Builder-editable overrides loaded from the ship_room_templates table at    */
/* boot. The hardcoded room_templates[] above remains as the fallback when   */
/* MySQL is unavailable or a room type has no database row.                  */

/* Canonical room_type strings matching the ship_room_templates rows
 * populated by db_init_data.c, indexed by enum ship_room_type. */
static const char *room_type_db_names[NUM_SHIP_ROOM_TYPES] = {
    "bridge",        /* ROOM_TYPE_BRIDGE */
    "quarters_crew", /* ROOM_TYPE_QUARTERS */
    "cargo_main",    /* ROOM_TYPE_CARGO */
    "engineering",   /* ROOM_TYPE_ENGINEERING */
    "weapons",       /* ROOM_TYPE_WEAPONS */
    "infirmary",     /* ROOM_TYPE_MEDICAL */
    "mess_hall",     /* ROOM_TYPE_MESS_HALL */
    "corridor",      /* ROOM_TYPE_CORRIDOR */
    "airlock",       /* ROOM_TYPE_AIRLOCK */
    "deck_main"      /* ROOM_TYPE_DECK */
};

static struct room_template db_room_templates[NUM_SHIP_ROOM_TYPES];
static bool db_room_template_loaded[NUM_SHIP_ROOM_TYPES];
static trig_vnum
    db_room_template_triggers[NUM_SHIP_ROOM_TYPES][MAX_SHIP_ROOM_TEMPLATE_TRIGGERS];
static int db_room_template_trigger_count[NUM_SHIP_ROOM_TYPES];

/**
 * Resolve a database room_type string to the runtime enum index.
 */
static int room_template_index_by_name(const char *name)
{
  int i;

  if (name == NULL)
  {
    return -1;
  }

  for (i = 0; i < NUM_SHIP_ROOM_TYPES; i++)
  {
    if (!str_cmp(name, room_type_db_names[i]))
    {
      return i;
    }
  }

  return -1;
}

/**
 * Load DG trigger attachments for generated room templates.
 */
static void load_ship_room_template_triggers(void)
{
  const char *create_sql =
      "CREATE TABLE IF NOT EXISTS ship_room_template_triggers ("
      "room_type VARCHAR(50) NOT NULL, "
      "vessel_type INT NOT NULL DEFAULT 0, "
      "trigger_vnum INT NOT NULL, "
      "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP, "
      "PRIMARY KEY (room_type, vessel_type, trigger_vnum), "
      "INDEX idx_ship_room_trigger_vnum (trigger_vnum)"
      ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4";
  MYSQL_RES *result;
  MYSQL_ROW row;
  int type;
  int trigger_count;
  int trigger_vnum;

  memset(db_room_template_triggers, 0, sizeof(db_room_template_triggers));
  memset(db_room_template_trigger_count, 0, sizeof(db_room_template_trigger_count));

  if (mysql_query(conn, create_sql))
  {
    log("SYSERR: Could not ensure ship room template triggers: %s", mysql_error(conn));
    return;
  }

  if (mysql_query(conn,
                  "SELECT room_type, trigger_vnum "
                  "FROM ship_room_template_triggers "
                  "WHERE vessel_type = 0 "
                  "ORDER BY room_type, trigger_vnum"))
  {
    log("SYSERR: Could not load ship room template triggers: %s", mysql_error(conn));
    return;
  }

  result = mysql_store_result(conn);
  if (result == NULL)
  {
    return;
  }

  trigger_count = 0;
  while ((row = mysql_fetch_row(result)) != NULL)
  {
    type = room_template_index_by_name(row[0]);
    trigger_vnum = row[1] ? atoi(row[1]) : 0;
    if (type < 0 || trigger_vnum <= 0)
    {
      log("SYSERR: Ignoring invalid ship room trigger mapping (%s, %d)",
          row[0] ? row[0] : "(null)", trigger_vnum);
      continue;
    }
    if (db_room_template_trigger_count[type] >= MAX_SHIP_ROOM_TEMPLATE_TRIGGERS)
    {
      log("SYSERR: Ship room template %s exceeds its %d-trigger limit",
          room_type_db_names[type], MAX_SHIP_ROOM_TEMPLATE_TRIGGERS);
      continue;
    }

    db_room_template_triggers[type][db_room_template_trigger_count[type]++] = trigger_vnum;
    trigger_count++;
  }

  mysql_free_result(result);
  log("Info: Loaded %d generated ship room trigger attachment%s", trigger_count,
      trigger_count == 1 ? "" : "s");
}

/**
 * Load room template overrides from the ship_room_templates table.
 *
 * Called once at boot after database initialization. Each successfully
 * loaded row overrides the matching hardcoded template; missing rows keep
 * their compiled-in fallback. Strings are strdup'd and retained for the
 * lifetime of the process.
 */
void load_ship_room_templates_from_db(void)
{
  char query[MAX_STRING_LENGTH];
  MYSQL_RES *result;
  MYSQL_ROW row;
  int i;
  int loaded = 0;

  if (!mysql_available || conn == NULL)
  {
    log("Info: MySQL not available, using compiled-in ship room templates");
    return;
  }

  for (i = 0; i < NUM_SHIP_ROOM_TYPES; i++)
  {
    snprintf(query, sizeof(query),
             "SELECT name_format, description_text, room_flags, sector_type, min_vessel_size "
             "FROM ship_room_templates WHERE room_type = '%s' AND vessel_type = 0 LIMIT 1",
             room_type_db_names[i]);

    if (mysql_query(conn, query))
    {
      log("SYSERR: load_ship_room_templates_from_db query failed: %s", mysql_error(conn));
      continue;
    }

    result = mysql_store_result(conn);
    if (result == NULL)
    {
      continue;
    }

    row = mysql_fetch_row(result);
    if (row != NULL && row[0] != NULL && row[1] != NULL)
    {
      db_room_templates[i].type = (enum ship_room_type)i;
      db_room_templates[i].name_format = strdup(row[0]);
      db_room_templates[i].description_format = strdup(row[1]);
      db_room_templates[i].room_flags = row[2] ? atoi(row[2]) : (ROOM_VEHICLE | ROOM_INDOORS);
      db_room_templates[i].sector_type = row[3] ? atoi(row[3]) : SECT_INSIDE;
      db_room_templates[i].min_vessel_size = row[4] ? atoi(row[4]) : 0;
      db_room_template_loaded[i] = TRUE;
      loaded++;
    }
    mysql_free_result(result);
  }

  load_ship_room_template_triggers();
  log("Info: Loaded %d ship room template override(s) from database", loaded);
}

/**
 * Instantiate the DG triggers configured for one generated room type.
 */
static void attach_ship_room_template_triggers(room_rnum room, enum ship_room_type type)
{
  trig_data *trigger;
  trig_rnum trigger_rnum;
  trig_vnum trigger_vnum;
  int i;

  if (room == NOWHERE || type < 0 || type >= NUM_SHIP_ROOM_TYPES)
  {
    return;
  }

  for (i = 0; i < db_room_template_trigger_count[type]; i++)
  {
    trigger_vnum = db_room_template_triggers[type][i];
    trigger_rnum = real_trigger(trigger_vnum);
    if (trigger_rnum == NOTHING)
    {
      log("SYSERR: Generated ship room %d cannot attach missing trigger %d",
          world[room].number, trigger_vnum);
      continue;
    }

    trigger = read_trigger(trigger_rnum);
    if (trigger == NULL)
    {
      log("SYSERR: Generated ship room %d could not instantiate trigger %d",
          world[room].number, trigger_vnum);
      continue;
    }

    if (SCRIPT(&world[room]) == NULL)
    {
      CREATE(SCRIPT(&world[room]), struct script_data, 1);
    }
    add_trigger(SCRIPT(&world[room]), trigger, -1);
  }
}

/**
 * Resolve the template for a room type: database override first, then the
 * compiled-in fallback.
 *
 * @param type The ship room type
 * @return Template pointer, or NULL if the type is unknown
 */
static const struct room_template *resolve_room_template(enum ship_room_type type)
{
  size_t i;

  if (type >= 0 && type < NUM_SHIP_ROOM_TYPES && db_room_template_loaded[type])
  {
    return &db_room_templates[type];
  }

  for (i = 0; (size_t)i < sizeof(room_templates) / sizeof(room_templates[0]); i++)
  {
    if (room_templates[i].type == type)
    {
      return &room_templates[i];
    }
  }

  return NULL;
}

/**
 * Safely expand a room template string: the first "%s" is replaced with the
 * ship's name, every other character (including stray '%') is copied
 * literally. Builder-authored database strings are never passed to printf-
 * style formatting.
 */
static void format_room_string(char *dest, size_t size, const char *fmt, const char *ship_name)
{
  size_t pos = 0;
  bool substituted = FALSE;

  if (size == 0)
  {
    return;
  }

  while (*fmt && pos < size - 1)
  {
    if (!substituted && fmt[0] == '%' && fmt[1] == 's')
    {
      size_t name_len = strlen(ship_name);
      if (name_len > size - 1 - pos)
      {
        name_len = size - 1 - pos;
      }
      memcpy(dest + pos, ship_name, name_len);
      pos += name_len;
      fmt += 2;
      substituted = TRUE;
      continue;
    }
    dest[pos++] = *fmt++;
  }

  dest[pos] = '\0';
}

/* Get base number of rooms for vessel type */
int get_base_rooms_for_type(enum vessel_class type)
{
  switch (type)
  {
  case VESSEL_RAFT:
    return 1;
  case VESSEL_BOAT:
    return 2;
  case VESSEL_SHIP:
    return 3;
  case VESSEL_WARSHIP:
    return 5;
  case VESSEL_AIRSHIP:
    return 4;
  case VESSEL_SUBMARINE:
    return 4;
  case VESSEL_TRANSPORT:
    return 6;
  case VESSEL_MAGICAL:
    return 3;
  default:
    return 1;
  }
}

/* Get maximum number of rooms for vessel type */
int get_max_rooms_for_type(enum vessel_class type)
{
  switch (type)
  {
  case VESSEL_RAFT:
    return 2;
  case VESSEL_BOAT:
    return 4;
  case VESSEL_SHIP:
    return 8;
  case VESSEL_WARSHIP:
    return 15;
  case VESSEL_AIRSHIP:
    return 10;
  case VESSEL_SUBMARINE:
    return 12;
  case VESSEL_TRANSPORT:
    return 20;
  case VESSEL_MAGICAL:
    return 10;
  default:
    return 1;
  }
}

/**
 * Derive vessel classification from ship template hull weight.
 * Uses the hullweight field from the template object to determine
 * vessel size and capabilities.
 *
 * @param hullweight The hull weight value from the ship template
 * @return The appropriate vessel_class enum value
 */
enum vessel_class derive_vessel_type_from_template(int hullweight)
{
  if (hullweight < 50)
  {
    return VESSEL_RAFT;
  }
  else if (hullweight < 150)
  {
    return VESSEL_BOAT;
  }
  else if (hullweight < 400)
  {
    return VESSEL_SHIP;
  }
  else if (hullweight < 800)
  {
    return VESSEL_WARSHIP;
  }
  else
  {
    return VESSEL_TRANSPORT;
  }
}

/**
 * Check if a ship already has interior rooms generated.
 * Used for idempotent room generation to prevent duplicates.
 *
 * @param ship Pointer to the ship data structure
 * @return TRUE if rooms already exist, FALSE otherwise
 */
bool ship_has_interior_rooms(struct greyhawk_ship_data *ship)
{
  if (!ship)
  {
    return FALSE;
  }

  /* Check if any rooms have been generated */
  if (ship->num_rooms > 0)
  {
    return TRUE;
  }

  /* Additional safety check - verify first room vnum is set */
  if (ship->room_vnums[0] != 0)
  {
    return TRUE;
  }

  return FALSE;
}

/* Create a ship room of specified type */
int create_ship_room(struct greyhawk_ship_data *ship, enum ship_room_type type)
{
  room_rnum new_room;
  int room_vnum;
  zone_rnum room_zone;
  const struct room_template *template = NULL;
  struct room_data room;
  char room_name[MAX_STRING_LENGTH];
  char room_description[MAX_STRING_LENGTH];

  /* NULL check for ship pointer */
  if (!ship)
  {
    log("SYSERR: create_ship_room called with NULL ship!");
    return NOWHERE;
  }

  /* Resolve template: database override first, compiled-in fallback second */
  template = resolve_room_template(type);

  if (!template)
  {
    log("SYSERR: No template found for room type %d", type);
    return NOWHERE;
  }

  /* Allocate a new room vnum using the reserved ship interior range */
  room_vnum = SHIP_INTERIOR_VNUM_BASE + (ship->shipnum * MAX_SHIP_ROOMS) + ship->num_rooms;

  /* Validate VNUM is within allowed range */
  if (room_vnum > SHIP_INTERIOR_VNUM_MAX)
  {
    log("SYSERR: Ship interior VNUM %d exceeds maximum %d (ship %d, room %d)", room_vnum,
        SHIP_INTERIOR_VNUM_MAX, ship->shipnum, ship->num_rooms);
    return NOWHERE;
  }

  /* Check if room already exists (VNUM collision) */
  if (real_room(room_vnum) != NOWHERE)
  {
    log("SYSERR: Room vnum %d already exists for ship %d!", room_vnum, ship->shipnum);
    return NOWHERE;
  }

  /* Verify world array has capacity */
  if (top_of_world >= 99999)
  {
    log("SYSERR: Maximum room limit reached! Cannot create ship room.");
    return NOWHERE;
  }

  room_zone = real_zone_by_thing(room_vnum);
  if (room_zone == NOWHERE)
  {
    log("SYSERR: No zone owns ship interior room vnum %d", room_vnum);
    return NOWHERE;
  }

  /* Set room name (safe substitution - builder strings are never used as
   * printf formats) */
  format_room_string(room_name, sizeof(room_name), template->name_format, ship->name);

  /* Set room description */
  format_room_string(room_description, sizeof(room_description), template->description_format,
                     ship->name);

  memset(&room, 0, sizeof(room));
  room.number = room_vnum;
  room.zone = room_zone;
  room.name = room_name;
  room.description = room_description;
  room.sector_type = template->sector_type;
  room.ship = ship;
  room.coords[0] = (int)ship->x;
  room.coords[1] = (int)ship->y;

  /* Set room flags and sector */
  if (template->room_flags & ROOM_VEHICLE)
    SET_BIT_AR(room.room_flags, ROOM_VEHICLE);
  if (template->room_flags & ROOM_INDOORS)
    SET_BIT_AR(room.room_flags, ROOM_INDOORS);

  new_room = add_runtime_room(&room);
  if (new_room == NOWHERE)
  {
    log("SYSERR: Failed to insert ship interior room vnum %d", room_vnum);
    return NOWHERE;
  }

  attach_ship_room_template_triggers(new_room, type);
  return room_vnum;
}

/* Add a room to the ship */
void add_ship_room(struct greyhawk_ship_data *ship, enum ship_room_type type)
{
  room_vnum room_vnum;

  if (ship->num_rooms >= MAX_SHIP_ROOMS)
  {
    log("SYSERR: Ship %d already has maximum rooms!", ship->shipnum);
    return;
  }

  room_vnum = create_ship_room(ship, type);
  if (room_vnum == NOWHERE)
  {
    return;
  }

  /* Add to ship's room list */
  ship->room_vnums[ship->num_rooms] = room_vnum;
  ship->room_templates[ship->num_rooms] = type;
  ship->num_rooms++;

  /* Special room assignments */
  switch (type)
  {
  case ROOM_TYPE_BRIDGE:
    ship->bridge_room = room_vnum;
    break;
  case ROOM_TYPE_CARGO:
  {
    int i;
    for (i = 0; i < 5; i++)
    {
      if (ship->cargo_rooms[i] == 0)
      {
        ship->cargo_rooms[i] = room_vnum;
        break;
      }
    }
    break;
  }
  case ROOM_TYPE_QUARTERS:
  {
    int i;
    for (i = 0; i < 10; i++)
    {
      if (ship->crew_quarters[i] == 0)
      {
        ship->crew_quarters[i] = room_vnum;
        break;
      }
    }
    break;
  }
  case ROOM_TYPE_AIRLOCK:
    if (ship->entrance_room == 0)
    {
      ship->entrance_room = room_vnum;
    }
    break;
  default:
    break;
  }
}

/* Generate complete ship interior based on vessel type */
void generate_ship_interior(struct greyhawk_ship_data *ship)
{
  int max_rooms;
  int i;

  if (!ship)
  {
    log("SYSERR: generate_ship_interior called with NULL ship!");
    return;
  }

  VSSL_DEBUG("Generating interior for ship %d (%s) type %d", ship->shipnum, ship->name,
             ship->vessel_type);

  /* Idempotent check - don't regenerate if rooms already exist */
  if (ship_has_interior_rooms(ship))
  {
    log("GREYHAWK SHIPS: Ship %d already has %d interior rooms, skipping generation", ship->shipnum,
        ship->num_rooms);
    return;
  }

  /* Clear existing room data */
  ship->num_rooms = 0;
  ship->bridge_room = 0;
  ship->entrance_room = 0;
  for (i = 0; i < 5; i++)
    ship->cargo_rooms[i] = 0;
  for (i = 0; i < 10; i++)
    ship->crew_quarters[i] = 0;
  for (i = 0; i < MAX_SHIP_ROOMS; i++)
  {
    ship->room_vnums[i] = 0;
    ship->room_templates[i] = 0;
  }

  /* Get room counts for this vessel type */
  max_rooms = get_max_rooms_for_type(ship->vessel_type);

  /* Always create the bridge first */
  add_ship_room(ship, ROOM_TYPE_BRIDGE);

  /* Generate required rooms based on vessel type */
  switch (ship->vessel_type)
  {
  case VESSEL_RAFT:
    /* Just the bridge/deck for a raft */
    break;

  case VESSEL_BOAT:
    add_ship_room(ship, ROOM_TYPE_QUARTERS);
    break;

  case VESSEL_SHIP:
    add_ship_room(ship, ROOM_TYPE_QUARTERS);
    add_ship_room(ship, ROOM_TYPE_CARGO);
    add_ship_room(ship, ROOM_TYPE_DECK);
    break;

  case VESSEL_WARSHIP:
    add_ship_room(ship, ROOM_TYPE_WEAPONS);
    add_ship_room(ship, ROOM_TYPE_WEAPONS);
    add_ship_room(ship, ROOM_TYPE_QUARTERS);
    add_ship_room(ship, ROOM_TYPE_ENGINEERING);
    add_ship_room(ship, ROOM_TYPE_DECK);
    break;

  case VESSEL_TRANSPORT:
    for (i = 0; i < 3; i++)
    {
      add_ship_room(ship, ROOM_TYPE_CARGO);
    }
    add_ship_room(ship, ROOM_TYPE_QUARTERS);
    add_ship_room(ship, ROOM_TYPE_MESS_HALL);
    break;

  case VESSEL_SUBMARINE:
    add_ship_room(ship, ROOM_TYPE_AIRLOCK);
    add_ship_room(ship, ROOM_TYPE_ENGINEERING);
    add_ship_room(ship, ROOM_TYPE_QUARTERS);
    break;

  case VESSEL_AIRSHIP:
    add_ship_room(ship, ROOM_TYPE_DECK);
    add_ship_room(ship, ROOM_TYPE_ENGINEERING);
    add_ship_room(ship, ROOM_TYPE_QUARTERS);
    break;

  case VESSEL_MAGICAL:
    add_ship_room(ship, ROOM_TYPE_QUARTERS);
    add_ship_room(ship, ROOM_TYPE_CARGO);
    break;
  }

  /* Discovery algorithm for additional rooms */
  ship->discovery_chance = 30.0; /* 30% chance for additional rooms */

  while (ship->num_rooms < max_rooms)
  {
    if (rand_number(1, 100) <= ship->discovery_chance)
    {
      /* Select appropriate room type based on what's missing */
      enum ship_room_type new_type;

      if (ship->vessel_type == VESSEL_WARSHIP && rand_number(1, 3) == 1)
      {
        new_type = ROOM_TYPE_WEAPONS;
      }
      else if (ship->vessel_type == VESSEL_TRANSPORT && rand_number(1, 2) == 1)
      {
        new_type = ROOM_TYPE_CARGO;
      }
      else
      {
        /* Random selection from common room types */
        int roll = rand_number(1, 5);
        switch (roll)
        {
        case 1:
          new_type = ROOM_TYPE_QUARTERS;
          break;
        case 2:
          new_type = ROOM_TYPE_CORRIDOR;
          break;
        case 3:
          new_type = ROOM_TYPE_CARGO;
          break;
        case 4:
          new_type = ROOM_TYPE_MESS_HALL;
          break;
        case 5:
          new_type = ROOM_TYPE_MEDICAL;
          break;
        default:
          new_type = ROOM_TYPE_CORRIDOR;
          break;
        }
      }

      add_ship_room(ship, new_type);
    }
    else
    {
      break;
    }
  }

  /* Set entrance room if not already set */
  if (ship->entrance_room == 0 && ship->num_rooms > 0)
  {
    /* Use the first non-bridge room as entrance, or bridge if only room */
    if (ship->num_rooms > 1)
    {
      ship->entrance_room = ship->room_vnums[1];
    }
    else
    {
      ship->entrance_room = ship->bridge_room;
    }
  }

  /* Generate connections between rooms */
  generate_room_connections(ship);

  log("Generated %d rooms for %s (vessel type %d)", ship->num_rooms, ship->name, ship->vessel_type);
}

/* Create connections between ship rooms */
void generate_room_connections(struct greyhawk_ship_data *ship)
{
  int i, j;
  room_rnum from_room, to_room;
  struct room_direction_data *exit;

  if (!ship || ship->num_rooms < 2)
  {
    return;
  }

  ship->num_connections = 0;

  /* Simple linear connection for small ships */
  if (ship->num_rooms <= 3)
  {
    for (i = 0; i < ship->num_rooms - 1; i++)
    {
      from_room = real_room(ship->room_vnums[i]);
      to_room = real_room(ship->room_vnums[i + 1]);

      if (from_room == NOWHERE || to_room == NOWHERE)
        continue;

      /* Create bidirectional connection (north/south) */
      CREATE(exit, struct room_direction_data, 1);
      exit->to_room = to_room;
      exit->exit_info = 0;
      exit->keyword = NULL;
      exit->general_description = strdup("The passage continues.");
      world[from_room].dir_option[NORTH] = exit;

      CREATE(exit, struct room_direction_data, 1);
      exit->to_room = from_room;
      exit->exit_info = 0;
      exit->keyword = NULL;
      exit->general_description = strdup("The passage continues.");
      world[to_room].dir_option[SOUTH] = exit;

      /* Record connection */
      if (ship->num_connections < MAX_SHIP_CONNECTIONS)
      {
        ship->connections[ship->num_connections].from_room = ship->room_vnums[i];
        ship->connections[ship->num_connections].to_room = ship->room_vnums[i + 1];
        ship->connections[ship->num_connections].direction = NORTH;
        ship->connections[ship->num_connections].is_hatch = FALSE;
        ship->connections[ship->num_connections].is_locked = FALSE;
        ship->num_connections++;
      }
    }
  }
  else
  {
    /* More complex layout for larger ships */
    /* Create a hub-and-spoke pattern with bridge at center */
    room_rnum bridge = real_room(ship->bridge_room);

    if (bridge != NOWHERE)
    {
      int dir = 0;
      for (i = 0; i < ship->num_rooms; i++)
      {
        if (ship->room_vnums[i] == ship->bridge_room)
          continue;

        from_room = bridge;
        to_room = real_room(ship->room_vnums[i]);

        if (to_room == NOWHERE)
          continue;

        /* Assign directions in order: N, E, S, W, NE, SE, SW, NW */
        if (dir >= NUM_OF_DIRS - 2)
          dir = 0; /* Skip up/down */

        /* Create connection from bridge to room */
        CREATE(exit, struct room_direction_data, 1);
        exit->to_room = to_room;
        exit->exit_info = 0;
        exit->keyword = NULL;
        exit->general_description = strdup("A passage leads to another part of the ship.");
        world[from_room].dir_option[dir] = exit;

        /* Create return connection */
        CREATE(exit, struct room_direction_data, 1);
        exit->to_room = from_room;
        exit->exit_info = 0;
        exit->keyword = NULL;
        exit->general_description = strdup("A passage leads back to the bridge.");
        world[to_room].dir_option[rev_dir[dir]] = exit;

        /* Record connection */
        if (ship->num_connections < MAX_SHIP_CONNECTIONS)
        {
          ship->connections[ship->num_connections].from_room = ship->bridge_room;
          ship->connections[ship->num_connections].to_room = ship->room_vnums[i];
          ship->connections[ship->num_connections].direction = dir;
          ship->connections[ship->num_connections].is_hatch = FALSE;
          ship->connections[ship->num_connections].is_locked = FALSE;
          ship->num_connections++;
        }

        dir++;
      }
    }

    /* Add some cross-connections between non-bridge rooms */
    for (i = 1; i < ship->num_rooms - 1; i++)
    {
      if (ship->room_vnums[i] == ship->bridge_room)
        continue;
      if (rand_number(1, 100) > 40)
        continue; /* 40% chance of cross-connection */

      from_room = real_room(ship->room_vnums[i]);
      to_room = real_room(ship->room_vnums[i + 1]);

      if (from_room == NOWHERE || to_room == NOWHERE)
        continue;
      if (ship->room_vnums[i + 1] == ship->bridge_room)
        continue;

      /* Find available direction */
      int found_dir = -1;
      for (j = 0; j < NUM_OF_DIRS - 2; j++)
      {
        if (world[from_room].dir_option[j] == NULL && world[to_room].dir_option[rev_dir[j]] == NULL)
        {
          found_dir = j;
          break;
        }
      }

      if (found_dir >= 0)
      {
        /* Create cross-connection */
        CREATE(exit, struct room_direction_data, 1);
        exit->to_room = to_room;
        exit->exit_info = 0;
        exit->keyword = NULL;
        exit->general_description = strdup("A side passage connects to another area.");
        world[from_room].dir_option[found_dir] = exit;

        CREATE(exit, struct room_direction_data, 1);
        exit->to_room = from_room;
        exit->exit_info = 0;
        exit->keyword = NULL;
        exit->general_description = strdup("A side passage connects to another area.");
        world[to_room].dir_option[rev_dir[found_dir]] = exit;

        /* Record connection */
        if (ship->num_connections < MAX_SHIP_CONNECTIONS)
        {
          ship->connections[ship->num_connections].from_room = ship->room_vnums[i];
          ship->connections[ship->num_connections].to_room = ship->room_vnums[i + 1];
          ship->connections[ship->num_connections].direction = found_dir;
          ship->connections[ship->num_connections].is_hatch = (rand_number(1, 4) == 1);
          ship->connections[ship->num_connections].is_locked = FALSE;
          ship->num_connections++;
        }
      }
    }
  }

  log("Generated %d connections for ship %s", ship->num_connections, ship->name);
}

/**
 * Infer a safe room type for persistence rows created before room types were
 * stored. Required rooms follow the same order as generate_ship_interior();
 * discovered rooms fall back to corridors.
 */
static enum ship_room_type infer_ship_room_type(enum vessel_class vessel_type, int index)
{
  if (index <= 0)
  {
    return ROOM_TYPE_BRIDGE;
  }

  switch (vessel_type)
  {
  case VESSEL_BOAT:
    return index == 1 ? ROOM_TYPE_QUARTERS : ROOM_TYPE_CORRIDOR;
  case VESSEL_SHIP:
    if (index == 1)
      return ROOM_TYPE_QUARTERS;
    if (index == 2)
      return ROOM_TYPE_CARGO;
    if (index == 3)
      return ROOM_TYPE_DECK;
    return ROOM_TYPE_CORRIDOR;
  case VESSEL_WARSHIP:
    if (index == 1 || index == 2)
      return ROOM_TYPE_WEAPONS;
    if (index == 3)
      return ROOM_TYPE_QUARTERS;
    if (index == 4)
      return ROOM_TYPE_ENGINEERING;
    if (index == 5)
      return ROOM_TYPE_DECK;
    return ROOM_TYPE_CORRIDOR;
  case VESSEL_AIRSHIP:
    if (index == 1)
      return ROOM_TYPE_DECK;
    if (index == 2)
      return ROOM_TYPE_ENGINEERING;
    if (index == 3)
      return ROOM_TYPE_QUARTERS;
    return ROOM_TYPE_CORRIDOR;
  case VESSEL_SUBMARINE:
    if (index == 1)
      return ROOM_TYPE_AIRLOCK;
    if (index == 2)
      return ROOM_TYPE_ENGINEERING;
    if (index == 3)
      return ROOM_TYPE_QUARTERS;
    return ROOM_TYPE_CORRIDOR;
  case VESSEL_TRANSPORT:
    if (index >= 1 && index <= 3)
      return ROOM_TYPE_CARGO;
    if (index == 4)
      return ROOM_TYPE_QUARTERS;
    if (index == 5)
      return ROOM_TYPE_MESS_HALL;
    return ROOM_TYPE_CORRIDOR;
  case VESSEL_MAGICAL:
    if (index == 1)
      return ROOM_TYPE_QUARTERS;
    if (index == 2)
      return ROOM_TYPE_CARGO;
    return ROOM_TYPE_CORRIDOR;
  case VESSEL_RAFT:
  default:
    return index == 1 ? ROOM_TYPE_DECK : ROOM_TYPE_CORRIDOR;
  }
}

/**
 * Materialize one persisted connection in the world exit table.
 */
static bool restore_ship_connection(struct greyhawk_ship_data *ship,
                                    const struct room_connection *connection)
{
  struct room_direction_data *exit;
  room_rnum from_room;
  room_rnum to_room;
  int reverse;

  if (ship == NULL || connection == NULL || connection->direction < 0 ||
      connection->direction >= NUM_OF_DIRS)
  {
    return FALSE;
  }

  from_room = real_room(connection->from_room);
  to_room = real_room(connection->to_room);
  reverse = rev_dir[connection->direction];
  if (from_room == NOWHERE || to_room == NOWHERE || reverse < 0 || reverse >= NUM_OF_DIRS ||
      world[from_room].ship != ship || world[to_room].ship != ship)
  {
    return FALSE;
  }

  if (world[from_room].dir_option[connection->direction] != NULL ||
      world[to_room].dir_option[reverse] != NULL)
  {
    log("SYSERR: Ship %d persistence has conflicting connection %d -> %d direction %d",
        ship->shipnum, connection->from_room, connection->to_room, connection->direction);
    return FALSE;
  }

  CREATE(exit, struct room_direction_data, 1);
  exit->to_room = to_room;
  exit->exit_info = 0;
  exit->keyword = NULL;
  exit->general_description = strdup(connection->is_hatch ? "A fitted hatch leads onward."
                                                          : "A passage leads onward.");
  world[from_room].dir_option[connection->direction] = exit;

  CREATE(exit, struct room_direction_data, 1);
  exit->to_room = from_room;
  exit->exit_info = 0;
  exit->keyword = NULL;
  exit->general_description = strdup(connection->is_hatch ? "A fitted hatch leads back."
                                                          : "A passage leads back.");
  world[to_room].dir_option[reverse] = exit;
  return TRUE;
}

/**
 * Recreate a persisted dynamic interior in the runtime world array.
 *
 * The database stores stable VNUMs, room types, special-room assignments,
 * and connection state. Runtime room rnums are deliberately regenerated
 * because inserting rooms shifts the sorted world array during boot.
 */
bool restore_ship_interior(struct greyhawk_ship_data *ship)
{
  struct room_connection saved_connections[MAX_SHIP_CONNECTIONS];
  int saved_room_types[MAX_SHIP_ROOMS];
  int saved_cargo_rooms[5];
  int saved_count;
  int saved_connection_count;
  int saved_bridge;
  int saved_entrance;
  int generated_bridge;
  int generated_cargo_rooms[5];
  int restored_connections;
  room_rnum saved_room;
  int i;

  if (ship == NULL || ship->shipnum < 2)
  {
    return FALSE;
  }

  saved_count = MIN(MAX_SHIP_ROOMS, MAX(0, ship->num_rooms));
  saved_connection_count = MIN(MAX_SHIP_CONNECTIONS, MAX(0, ship->num_connections));
  saved_bridge = ship->bridge_room;
  saved_entrance = ship->entrance_room;
  memcpy(saved_room_types, ship->room_templates, sizeof(saved_room_types));
  memcpy(saved_cargo_rooms, ship->cargo_rooms, sizeof(saved_cargo_rooms));
  memcpy(saved_connections, ship->connections, sizeof(saved_connections));

  ship->num_rooms = 0;
  ship->num_connections = 0;
  ship->bridge_room = 0;
  ship->entrance_room = 0;
  memset(ship->room_vnums, 0, sizeof(ship->room_vnums));
  memset(ship->room_templates, 0, sizeof(ship->room_templates));
  memset(ship->cargo_rooms, 0, sizeof(ship->cargo_rooms));
  memset(ship->crew_quarters, 0, sizeof(ship->crew_quarters));
  memset(ship->connections, 0, sizeof(ship->connections));

  for (i = 0; i < saved_count; i++)
  {
    enum ship_room_type type;

    type = (enum ship_room_type)saved_room_types[i];
    if (type < ROOM_TYPE_BRIDGE || type > ROOM_TYPE_DECK ||
        (i > 0 && type == ROOM_TYPE_BRIDGE))
    {
      type = infer_ship_room_type(ship->vessel_type, i);
    }
    add_ship_room(ship, type);
    if (ship->num_rooms != i + 1)
    {
      log("SYSERR: Could not restore room %d of %d for ship %d", i + 1, saved_count,
          ship->shipnum);
      vessel_reclaim_interior_rooms(ship, 0);
      return FALSE;
    }
  }

  generated_bridge = ship->bridge_room;
  memcpy(generated_cargo_rooms, ship->cargo_rooms, sizeof(generated_cargo_rooms));
  saved_room = real_room(saved_bridge);
  ship->bridge_room =
      saved_room != NOWHERE && world[saved_room].ship == ship ? saved_bridge : generated_bridge;

  saved_room = real_room(saved_entrance);
  if (saved_room != NOWHERE && world[saved_room].ship == ship)
  {
    ship->entrance_room = saved_entrance;
  }
  else
  {
    ship->entrance_room = ship->num_rooms > 1 ? ship->room_vnums[1] : ship->bridge_room;
  }

  for (i = 0; i < 5; i++)
  {
    saved_room = real_room(saved_cargo_rooms[i]);
    ship->cargo_rooms[i] =
        saved_room != NOWHERE && world[saved_room].ship == ship ? saved_cargo_rooms[i]
                                                                : generated_cargo_rooms[i];
  }

  restored_connections = 0;
  for (i = 0; i < saved_connection_count; i++)
  {
    if (restore_ship_connection(ship, &saved_connections[i]))
    {
      ship->connections[restored_connections] = saved_connections[i];
      restored_connections++;
    }
  }
  ship->num_connections = restored_connections;

  if (ship->num_rooms > 1 && restored_connections == 0)
  {
    log("SYSERR: Ship %d had no usable saved connections; generating a safe layout",
        ship->shipnum);
    generate_room_connections(ship);
  }
  else if (restored_connections != saved_connection_count)
  {
    log("SYSERR: Ship %d restored only %d valid interior connections", ship->shipnum,
        restored_connections);
  }

  ship->shiproom = ship->entrance_room;
  update_ship_room_coordinates(ship);
  log("Info: Recreated %d runtime interior rooms for ship %d '%s'", ship->num_rooms,
      ship->shipnum, ship->name);
  return TRUE;
}

/* Get ship from a room */
struct greyhawk_ship_data *get_ship_from_room(room_rnum room)
{
  int i;

  if (room == NOWHERE || room > top_of_world)
  {
    return NULL;
  }

  /* Check if room has direct ship pointer */
  if (is_valid_ship(world[room].ship))
  {
    return world[room].ship;
  }

  /* Otherwise search all ships for this room */
  for (i = 0; i < GREYHAWK_MAXSHIPS; i++)
  {
    if (is_valid_ship(&greyhawk_ships[i]))
    {
      int j;
      for (j = 0; j < greyhawk_ships[i].num_rooms; j++)
      {
        if (real_room(greyhawk_ships[i].room_vnums[j]) == room)
        {
          return &greyhawk_ships[i];
        }
      }
    }
  }

  return NULL;
}

/* Update all ship room coordinates to match ship position */
void update_ship_room_coordinates(struct greyhawk_ship_data *ship)
{
  int i;
  room_rnum room;

  if (!ship)
    return;

  for (i = 0; i < ship->num_rooms; i++)
  {
    room = real_room(ship->room_vnums[i]);
    if (room != NOWHERE)
    {
      /* Update room's wilderness coordinates */
      /* Update wilderness coordinates */
      world[room].coords[0] = (int)ship->x;
      world[room].coords[1] = (int)ship->y;
      /* Z coordinate maintained separately in ship structure */

      /* Keep ship pointer updated */
      world[room].ship = ship;
    }
  }
}

/**
 * Remove the ephemeral interior rooms owned by a prototype-spawned ship.
 *
 * Slots 0 and 1 are reserved for legacy/static fixtures and are never
 * reclaimed here. Occupants and loose objects are moved beside the hull
 * before each room is removed from the sorted world array.
 *
 * @param ship Ship whose runtime rooms should be reclaimed
 * @param evacuation_room Fallback destination if the hull has no room
 * @return Number of rooms reclaimed
 */
int vessel_reclaim_interior_rooms(struct greyhawk_ship_data *ship, room_rnum evacuation_room)
{
  struct char_data *tch;
  struct char_data *next_tch;
  struct obj_data *obj;
  struct obj_data *next_obj;
  room_rnum interior;
  room_rnum destination;
  int expected_min;
  int expected_max;
  int reclaimed = 0;
  int i;

  if (ship == NULL || ship->shipnum < 2)
  {
    return 0;
  }

  expected_min = SHIP_INTERIOR_VNUM_BASE + (ship->shipnum * MAX_SHIP_ROOMS);
  expected_max = expected_min + MAX_SHIP_ROOMS - 1;

  for (i = ship->num_rooms - 1; i >= 0; i--)
  {
    if (ship->room_vnums[i] < expected_min || ship->room_vnums[i] > expected_max)
    {
      log("SYSERR: Refusing to reclaim unexpected room %d from ship %d", ship->room_vnums[i],
          ship->shipnum);
      continue;
    }

    interior = real_room(ship->room_vnums[i]);
    if (interior == NOWHERE)
    {
      continue;
    }

    if (world[interior].ship != ship)
    {
      log("SYSERR: Refusing to reclaim room %d not owned by ship %d", world[interior].number,
          ship->shipnum);
      continue;
    }

    destination = ship->shipobj != NULL ? IN_ROOM(ship->shipobj) : evacuation_room;
    if (destination == NOWHERE || destination == interior)
    {
      destination = 0;
    }

    for (tch = world[interior].people; tch != NULL; tch = next_tch)
    {
      next_tch = tch->next_in_room;
      char_from_room(tch);
      if (ZONE_FLAGGED(GET_ROOM_ZONE(destination), ZONE_WILDERNESS))
      {
        X_LOC(tch) = world[destination].coords[0];
        Y_LOC(tch) = world[destination].coords[1];
      }
      char_to_room(tch, destination);
      send_to_char(tch, "The vessel is removed from service around you.\r\n");
      look_at_room(tch, 0);
    }

    for (obj = world[interior].contents; obj != NULL; obj = next_obj)
    {
      next_obj = obj->next_content;
      obj_from_room(obj);
      obj_to_room(obj, destination);
    }

    world[interior].ship = NULL;
    if (delete_runtime_room(interior))
    {
      reclaimed++;
    }
  }

  ship->num_rooms = 0;
  ship->num_connections = 0;
  ship->bridge_room = 0;
  ship->entrance_room = 0;
  ship->shiproom = 0;
  memset(ship->room_vnums, 0, sizeof(ship->room_vnums));
  memset(ship->room_templates, 0, sizeof(ship->room_templates));
  memset(ship->cargo_rooms, 0, sizeof(ship->cargo_rooms));
  memset(ship->crew_quarters, 0, sizeof(ship->crew_quarters));
  memset(ship->connections, 0, sizeof(ship->connections));

  return reclaimed;
}

/* Check if a room has an outside view */
bool room_has_outside_view(room_rnum room)
{
  struct greyhawk_ship_data *ship;
  int room_vnum;
  int i;

  if (room == NOWHERE)
    return FALSE;

  ship = get_ship_from_room(room);
  if (!ship)
    return FALSE;

  room_vnum = world[room].number;

  /* Bridge always has a view */
  if (room_vnum == ship->bridge_room)
    return TRUE;

  /* Check if it's a deck room */
  for (i = 0; i < ship->num_rooms; i++)
  {
    if (ship->room_vnums[i] == room_vnum)
    {
      /* Check room name for "Deck" */
      if (world[room].name && strstr(world[room].name, "Deck"))
      {
        return TRUE;
      }
      /* Check if not indoors */
      if (!ROOM_FLAGGED(room, ROOM_INDOORS))
      {
        return TRUE;
      }
    }
  }

  return FALSE;
}

/* Send message to all characters on a ship */
void send_to_ship(struct greyhawk_ship_data *ship, const char *format, ...)
{
  va_list args;
  char buf[MAX_STRING_LENGTH];
  struct char_data *ch;
  int i;
  room_rnum room;

  if (!ship || !format)
    return;

  va_start(args, format);
  vsnprintf(buf, sizeof(buf), format, args);
  va_end(args);

  /* Send to all rooms on the ship */
  for (i = 0; i < ship->num_rooms; i++)
  {
    room = real_room(ship->room_vnums[i]);
    if (room != NOWHERE)
    {
      for (ch = world[room].people; ch; ch = ch->next_in_room)
      {
        send_to_char(ch, "%s\r\n", buf);
      }
    }
  }
}

/* Find a ship by name */
struct greyhawk_ship_data *find_ship_by_name(const char *name)
{
  char *end;
  long shipnum;
  int i;
  bool numeric;

  if (!name || !*name)
    return NULL;

  shipnum = strtol(name, &end, 10);
  numeric = (*end == '\0' && shipnum >= 0 && shipnum < GREYHAWK_MAXSHIPS);

  /* Prefer exact persistent identifiers and exact full names. */
  for (i = 0; i < GREYHAWK_MAXSHIPS; i++)
  {
    if (!is_valid_ship(&greyhawk_ships[i]))
      continue;

    if (!str_cmp(name, greyhawk_ships[i].id) || !str_cmp(name, greyhawk_ships[i].name) ||
        (numeric && greyhawk_ships[i].shipnum == (int)shipnum))
    {
      return &greyhawk_ships[i];
    }
  }

  /* Then accept an unambiguous player-facing keyword such as "tern". */
  for (i = 0; i < GREYHAWK_MAXSHIPS; i++)
  {
    if (is_valid_ship(&greyhawk_ships[i]) && isname(name, greyhawk_ships[i].name))
    {
      return &greyhawk_ships[i];
    }
  }

  return NULL;
}

/* Get ship by ID */
struct greyhawk_ship_data *get_ship_by_id(int id)
{
  if (id < 0 || id >= GREYHAWK_MAXSHIPS)
    return NULL;

  if (is_valid_ship(&greyhawk_ships[id]))
  {
    return &greyhawk_ships[id];
  }

  return NULL;
}

/* Check if character is piloting a ship */
bool is_pilot(struct char_data *ch, struct greyhawk_ship_data *ship)
{
  room_rnum ch_room;

  if (!ch || !ship)
    return FALSE;

  ch_room = IN_ROOM(ch);

  /* Must be in the bridge to pilot, and cleared for the helm on owned
   * ships (owner, permit list, or immortal - see vessels_ownership.c) */
  if (real_room(ship->bridge_room) == ch_room)
  {
    return vessel_helm_permitted(ch, ship);
  }

  return FALSE;
}

/* ========================================================================= */
/* SHIP INTERIOR MOVEMENT FUNCTIONS                                          */
/* ========================================================================= */

/**
 * Check if a character is currently in a ship interior room.
 *
 * This is a convenience wrapper around get_ship_from_room() that returns
 * a simple boolean indicating whether the character is aboard a vessel.
 *
 * @param ch The character to check
 * @return TRUE if character is in a ship interior room, FALSE otherwise
 */
bool is_in_ship_interior(struct char_data *ch)
{
  if (!CONFIG_VESSEL_SYSTEM)
  {
    return FALSE;
  }

  if (!ch)
  {
    return FALSE;
  }

  if (IN_ROOM(ch) == NOWHERE)
  {
    return FALSE;
  }

  return (get_ship_from_room(IN_ROOM(ch)) != NULL);
}

/**
 * Get the destination room for a ship interior exit in a given direction.
 *
 * Searches the ship's connections array for a connection from the current
 * room in the specified direction. Handles bidirectional lookup - checks
 * both from_room->to_room and to_room->from_room with reverse direction.
 *
 * @param ship Pointer to the ship data structure
 * @param current The room rnum the character is currently in
 * @param dir The direction to check (NORTH, SOUTH, etc.)
 * @return The destination room rnum, or NOWHERE if no exit exists
 */
room_rnum get_ship_exit(struct greyhawk_ship_data *ship, room_rnum current, int dir)
{
  int i;
  int current_vnum;

  if (!ship)
  {
    return NOWHERE;
  }

  if (current == NOWHERE || current > top_of_world)
  {
    return NOWHERE;
  }

  if (dir < 0 || dir >= NUM_OF_DIRS)
  {
    return NOWHERE;
  }

  /* Get the vnum of the current room for comparison with connections */
  current_vnum = world[current].number;

  /* Search connections for a match */
  for (i = 0; i < ship->num_connections; i++)
  {
    /* Check forward direction: from_room -> to_room */
    if (ship->connections[i].from_room == current_vnum && ship->connections[i].direction == dir)
    {
      return real_room(ship->connections[i].to_room);
    }

    /* Check reverse direction: to_room -> from_room */
    if (ship->connections[i].to_room == current_vnum &&
        ship->connections[i].direction == rev_dir[dir])
    {
      return real_room(ship->connections[i].from_room);
    }
  }

  return NOWHERE;
}

/**
 * Check if a passage or hatch is blocked in the given direction.
 *
 * Examines the ship's connections to determine if movement is blocked
 * by a locked hatch or sealed passage. This is separate from standard
 * door mechanics and uses the is_locked flag in the room_connection.
 *
 * @param ship Pointer to the ship data structure
 * @param room The room rnum to check from
 * @param dir The direction to check
 * @return TRUE if passage is blocked, FALSE if movement is allowed
 */
bool is_passage_blocked(struct greyhawk_ship_data *ship, room_rnum room, int dir)
{
  int i;
  int room_vnum;

  if (!ship)
  {
    return FALSE;
  }

  if (room == NOWHERE || room > top_of_world)
  {
    return FALSE;
  }

  if (dir < 0 || dir >= NUM_OF_DIRS)
  {
    return FALSE;
  }

  room_vnum = world[room].number;

  /* Search connections for the passage */
  for (i = 0; i < ship->num_connections; i++)
  {
    /* Check forward direction */
    if (ship->connections[i].from_room == room_vnum && ship->connections[i].direction == dir)
    {
      return ship->connections[i].is_locked;
    }

    /* Check reverse direction */
    if (ship->connections[i].to_room == room_vnum && ship->connections[i].direction == rev_dir[dir])
    {
      return ship->connections[i].is_locked;
    }
  }

  /* No connection found - not blocked (but also no exit) */
  return FALSE;
}

/**
 * Handle character movement between ship interior rooms.
 *
 * This function is the primary handler for ship interior movement,
 * called from do_simple_move() when a character is detected to be
 * aboard a vessel. It handles:
 * - Exit validation via get_ship_exit()
 * - Blocked passage checks via is_passage_blocked()
 * - Character movement with char_from_room/char_to_room
 * - Movement messages for leaving and entering rooms
 *
 * @param ch The character attempting to move
 * @param dir The direction to move (NORTH, SOUTH, etc.)
 */
void do_move_ship_interior(struct char_data *ch, int dir)
{
  struct greyhawk_ship_data *ship;
  struct greyhawk_ship_data *target_ship;
  room_rnum dest_room;

  /* Validate character pointer */
  if (!ch)
  {
    log("SYSERR: do_move_ship_interior called with NULL character!");
    return;
  }

  /* Validate character location */
  if (IN_ROOM(ch) == NOWHERE)
  {
    log("SYSERR: do_move_ship_interior called for character in NOWHERE!");
    return;
  }

  /* Validate direction */
  if (dir < 0 || dir >= NUM_OF_DIRS)
  {
    send_to_char(ch, "You can't go that way.\r\n");
    return;
  }

  /* Get ship from current room */
  ship = get_ship_from_room(IN_ROOM(ch));
  if (!ship)
  {
    /* Not in a ship interior - this shouldn't happen if called correctly */
    send_to_char(ch, "You are not aboard a vessel.\r\n");
    return;
  }

  /* Get destination room */
  dest_room = get_ship_exit(ship, IN_ROOM(ch), dir);
  if (dest_room == NOWHERE && W_EXIT(IN_ROOM(ch), dir) != NULL)
  {
    /*
     * Docking creates a temporary world exit between two otherwise separate
     * interior graphs. Accept that exit only while both ships name each other
     * as their active docking partner.
     */
    dest_room = W_EXIT(IN_ROOM(ch), dir)->to_room;
    target_ship = get_ship_from_room(dest_room);
    if (target_ship == NULL || target_ship == ship ||
        ship->docked_to_ship != target_ship->shipnum ||
        target_ship->docked_to_ship != ship->shipnum)
    {
      dest_room = NOWHERE;
    }
  }

  if (dest_room == NOWHERE)
  {
    send_to_char(ch, "There is no passage in that direction.\r\n");
    return;
  }

  /* Check if passage is blocked */
  if (is_passage_blocked(ship, IN_ROOM(ch), dir))
  {
    send_to_char(ch, "The hatch is sealed shut.\r\n");
    return;
  }

  /* Send departure message to room */
  act("$n moves $T.", TRUE, ch, 0, (void *)dirs[dir], TO_ROOM);

  /* Move the character */
  char_from_room(ch);
  char_to_room(ch, dest_room);

  /* Send arrival message to new room */
  act("$n arrives from $T.", TRUE, ch, 0, (void *)dirs[rev_dir[dir]], TO_ROOM);

  /* Show the new room */
  look_at_room(ch, 0);

  /* Entry triggers for the new room */
  entry_mtrigger(ch);
  greet_mtrigger(ch, dir);
  greet_memory_mtrigger(ch);
}
