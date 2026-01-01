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

/* External variables */
extern struct greyhawk_ship_data greyhawk_ships[GREYHAWK_MAXSHIPS];
extern struct room_data *world;
extern room_rnum top_of_world;

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
  struct room_template *template = NULL;
  char buf[MAX_STRING_LENGTH];
  int i;

  /* NULL check for ship pointer */
  if (!ship)
  {
    log("SYSERR: create_ship_room called with NULL ship!");
    return NOWHERE;
  }

  /* Find the template for this room type */
  for (i = 0; i < sizeof(room_templates) / sizeof(room_templates[0]); i++)
  {
    if (room_templates[i].type == type)
    {
      template = &room_templates[i];
      break;
    }
  }

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

  /* Create the new room */
  new_room = ++top_of_world;
  world[new_room].number = room_vnum;

  /* Set room name */
  snprintf(buf, sizeof(buf), template->name_format, ship->name);
  world[new_room].name = strdup(buf);

  /* Set room description */
  snprintf(buf, sizeof(buf), template->description_format, ship->name);
  world[new_room].description = strdup(buf);

  /* Set room flags and sector */
  /* Set room flags - need to set each flag individually */
  if (template->room_flags & ROOM_VEHICLE)
    SET_BIT_AR(world[new_room].room_flags, ROOM_VEHICLE);
  if (template->room_flags & ROOM_INDOORS)
    SET_BIT_AR(world[new_room].room_flags, ROOM_INDOORS);
  world[new_room].sector_type = template->sector_type;

  /* Link to ship */
  world[new_room].ship = ship;

  /* Initialize exits */
  for (i = 0; i < NUM_OF_DIRS; i++)
  {
    world[new_room].dir_option[i] = NULL;
  }

  /* Set coordinates to match ship position */
  /* Set wilderness coordinates */
  world[new_room].coords[0] = (int)ship->x;
  world[new_room].coords[1] = (int)ship->y;
  /* Z coordinate stored in ship structure separately */

  return room_vnum;
}

/* Add a room to the ship */
void add_ship_room(struct greyhawk_ship_data *ship, enum ship_room_type type)
{
  int room_vnum;

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
    ship->room_vnums[i] = 0;

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

/* Get ship from a room */
struct greyhawk_ship_data *get_ship_from_room(room_rnum room)
{
  int i;

  if (room == NOWHERE || room < 0 || room > top_of_world)
  {
    return NULL;
  }

  /* Check if room has direct ship pointer */
  if (world[room].ship)
  {
    return world[room].ship;
  }

  /* Otherwise search all ships for this room */
  for (i = 0; i < GREYHAWK_MAXSHIPS; i++)
  {
    if (greyhawk_ships[i].shipnum > 0)
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
  int i;

  if (!name || !*name)
    return NULL;

  for (i = 0; i < GREYHAWK_MAXSHIPS; i++)
  {
    if (greyhawk_ships[i].shipnum > 0)
    {
      if (!str_cmp(name, greyhawk_ships[i].name))
      {
        return &greyhawk_ships[i];
      }
    }
  }

  return NULL;
}

/* Get ship by ID */
struct greyhawk_ship_data *get_ship_by_id(int id)
{
  if (id < 0 || id >= GREYHAWK_MAXSHIPS)
    return NULL;

  if (greyhawk_ships[id].shipnum > 0)
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

  /* Must be in the bridge to pilot */
  if (real_room(ship->bridge_room) == ch_room)
  {
    return TRUE;
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

  if (current == NOWHERE || current < 0 || current > top_of_world)
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

  if (room == NOWHERE || room < 0 || room > top_of_world)
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
