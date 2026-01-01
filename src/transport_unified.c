/* ************************************************************************
 *   File:   transport_unified.c                        Part of LuminariMUD *
 *  Usage:   Unified transport command implementations                       *
 *                                                                           *
 *  Phase 02, Session 06: Unified Command Interface                          *
 *  Provides transport-agnostic commands (enter, exit, go, tstatus) that     *
 *  work seamlessly with both vehicles and vessels.                          *
 *                                                                           *
 *  All rights reserved.  See license for complete information.              *
 *                                                                           *
 *  LuminariMUD is based on CircleMUD, Copyright (C) 1993, 94.               *
 *  CircleMUD (C) 1993, 94 by the Trustees of the Johns Hopkins University   *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.                 *
 ************************************************************************* */

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "handler.h"
#include "interpreter.h"
#include "vessels.h"
#include "transport_unified.h"

/* ========================================================================= */
/* EXTERNAL FUNCTIONS                                                        */
/* ========================================================================= */

/* From vehicles.c */
extern struct vehicle_data *vehicle_find_in_room(room_rnum room);
extern struct vehicle_data *vehicle_find_by_id(int id);
extern int vehicle_add_passenger(struct vehicle_data *vehicle);
extern int vehicle_remove_passenger(struct vehicle_data *vehicle);
extern int vehicle_can_add_passenger(struct vehicle_data *vehicle);
extern int vehicle_move(struct vehicle_data *vehicle, int direction);
extern int vehicle_can_move(struct vehicle_data *vehicle, int direction);
extern int vehicle_is_operational(struct vehicle_data *vehicle);
extern const char *vehicle_type_name(enum vehicle_type type);
extern const char *vehicle_state_name(enum vehicle_state state);

/* From vessels_rooms.c */
extern struct greyhawk_ship_data *get_ship_from_room(room_rnum room);
extern bool is_in_ship_interior(struct char_data *ch);

/* From vehicles_commands.c - player-vehicle tracking */
extern int is_player_in_vehicle(struct char_data *ch);
extern struct vehicle_data *get_player_vehicle(struct char_data *ch);
extern int register_player_mount(struct char_data *ch, struct vehicle_data *vehicle);
extern int unregister_player_mount(struct char_data *ch);

/* ========================================================================= */
/* HELPER FUNCTIONS                                                          */
/* ========================================================================= */

/**
 * Parse a direction string for movement commands.
 *
 * Supports multiple formats:
 * - Full names: "north", "south", "east", "west", etc.
 * - Abbreviations: "n", "s", "e", "w", "ne", "nw", "se", "sw"
 *
 * @param arg The direction string to parse
 * @return Direction constant (NORTH, SOUTH, etc.) or -1 if invalid
 */
static int parse_direction(const char *arg)
{
  if (arg == NULL || *arg == '\0')
  {
    return -1;
  }

  /* Check for string direction names */
  if (!strcasecmp(arg, "north") || !strcasecmp(arg, "n"))
  {
    return NORTH;
  }
  if (!strcasecmp(arg, "south") || !strcasecmp(arg, "s"))
  {
    return SOUTH;
  }
  if (!strcasecmp(arg, "east") || !strcasecmp(arg, "e"))
  {
    return EAST;
  }
  if (!strcasecmp(arg, "west") || !strcasecmp(arg, "w"))
  {
    return WEST;
  }
  if (!strcasecmp(arg, "up") || !strcasecmp(arg, "u"))
  {
    return UP;
  }
  if (!strcasecmp(arg, "down") || !strcasecmp(arg, "d"))
  {
    return DOWN;
  }
  if (!strcasecmp(arg, "northeast") || !strcasecmp(arg, "ne"))
  {
    return NORTHEAST;
  }
  if (!strcasecmp(arg, "northwest") || !strcasecmp(arg, "nw"))
  {
    return NORTHWEST;
  }
  if (!strcasecmp(arg, "southeast") || !strcasecmp(arg, "se"))
  {
    return SOUTHEAST;
  }
  if (!strcasecmp(arg, "southwest") || !strcasecmp(arg, "sw"))
  {
    return SOUTHWEST;
  }

  return -1;
}

/**
 * Get direction name string for display.
 *
 * @param direction The direction constant
 * @return Static string with direction name
 */
static const char *get_direction_name(int direction)
{
  switch (direction)
  {
  case NORTH:
    return "north";
  case SOUTH:
    return "south";
  case EAST:
    return "east";
  case WEST:
    return "west";
  case UP:
    return "up";
  case DOWN:
    return "down";
  case NORTHEAST:
    return "northeast";
  case NORTHWEST:
    return "northwest";
  case SOUTHEAST:
    return "southeast";
  case SOUTHWEST:
    return "southwest";
  default:
    return "unknown";
  }
}

/* ========================================================================= */
/* TRANSPORT DETECTION FUNCTIONS                                              */
/* ========================================================================= */

/**
 * Get display name for a transport type.
 */
const char *transport_type_name(enum transport_type type)
{
  switch (type)
  {
  case TRANSPORT_VEHICLE:
    return "vehicle";
  case TRANSPORT_VESSEL:
    return "vessel";
  case TRANSPORT_NONE:
  default:
    return "unknown";
  }
}

/**
 * Detect transport type in a room.
 *
 * Scans for vehicles first (more common use case), then vessels.
 */
enum transport_type get_transport_type_in_room(room_rnum room)
{
  struct vehicle_data *vehicle;
  struct greyhawk_ship_data *vessel;

  if (room == NOWHERE || room < 0)
  {
    return TRANSPORT_NONE;
  }

  /* Check for vehicle first (more common) */
  vehicle = vehicle_find_in_room(room);
  if (vehicle != NULL)
  {
    return TRANSPORT_VEHICLE;
  }

  /* Check for vessel */
  vessel = get_ship_from_room(room);
  if (vessel != NULL)
  {
    return TRANSPORT_VESSEL;
  }

  return TRANSPORT_NONE;
}

/**
 * Get transport data from a room.
 *
 * Populates transport_data structure with found transport.
 */
int get_transport_in_room(room_rnum room, struct transport_data *td)
{
  struct vehicle_data *vehicle;
  struct greyhawk_ship_data *vessel;

  if (td == NULL)
  {
    return 0;
  }

  /* Initialize to none */
  td->type = TRANSPORT_NONE;
  td->data.vehicle = NULL;

  if (room == NOWHERE || room < 0)
  {
    return 0;
  }

  /* Check for vehicle first */
  vehicle = vehicle_find_in_room(room);
  if (vehicle != NULL)
  {
    td->type = TRANSPORT_VEHICLE;
    td->data.vehicle = vehicle;
    return 1;
  }

  /* Check for vessel */
  vessel = get_ship_from_room(room);
  if (vessel != NULL)
  {
    td->type = TRANSPORT_VESSEL;
    td->data.vessel = vessel;
    return 1;
  }

  return 0;
}

/**
 * Get a character's current transport.
 *
 * Checks if character is in a vehicle or vessel.
 */
int get_character_transport(struct char_data *ch, struct transport_data *td)
{
  struct vehicle_data *vehicle;
  struct greyhawk_ship_data *vessel;

  if (ch == NULL || td == NULL)
  {
    if (td != NULL)
    {
      td->type = TRANSPORT_NONE;
      td->data.vehicle = NULL;
    }
    return 0;
  }

  /* Initialize to none */
  td->type = TRANSPORT_NONE;
  td->data.vehicle = NULL;

  /* Check for vehicle first */
  vehicle = get_player_vehicle(ch);
  if (vehicle != NULL)
  {
    td->type = TRANSPORT_VEHICLE;
    td->data.vehicle = vehicle;
    return 1;
  }

  /* Check for vessel (player in ship interior) */
  if (is_in_ship_interior(ch))
  {
    vessel = get_ship_from_room(IN_ROOM(ch));
    if (vessel != NULL)
    {
      td->type = TRANSPORT_VESSEL;
      td->data.vessel = vessel;
      return 1;
    }
  }

  return 0;
}

/**
 * Check if character is in any transport.
 */
int is_in_transport(struct char_data *ch)
{
  struct transport_data td;
  return get_character_transport(ch, &td);
}

/**
 * Get the name of a specific transport.
 */
const char *get_transport_name(struct transport_data *td)
{
  if (td == NULL)
  {
    return "unknown";
  }

  switch (td->type)
  {
  case TRANSPORT_VEHICLE:
    if (td->data.vehicle != NULL)
    {
      return td->data.vehicle->name;
    }
    break;
  case TRANSPORT_VESSEL:
    if (td->data.vessel != NULL)
    {
      return td->data.vessel->name;
    }
    break;
  default:
    break;
  }

  return "unknown";
}

/**
 * Check if a transport is operational.
 */
int is_transport_operational(struct transport_data *td)
{
  if (td == NULL)
  {
    return FALSE;
  }

  switch (td->type)
  {
  case TRANSPORT_VEHICLE:
    if (td->data.vehicle != NULL)
    {
      return vehicle_is_operational(td->data.vehicle);
    }
    break;
  case TRANSPORT_VESSEL:
    if (td->data.vessel != NULL)
    {
      /* Vessels are operational if they exist and have valid ship data */
      return TRUE; /* Simplified check - vessels are generally operational */
    }
    break;
  default:
    break;
  }

  return FALSE;
}

/* ========================================================================= */
/* UNIFIED COMMAND IMPLEMENTATIONS                                            */
/* ========================================================================= */

/**
 * do_transport_enter - Unified entry command for vehicles and vessels.
 *
 * Usage: tenter [target]
 *
 * Detects transport type in room and delegates to appropriate entry handler.
 * Note: Named do_transport_enter to avoid conflict with do_enter in movement.c.
 */
ACMD(do_transport_enter)
{
  struct transport_data td;
  struct vehicle_data *vehicle;
  char arg[MAX_INPUT_LENGTH];

  /* Parse argument */
  one_argument(argument, arg, sizeof(arg));

  /* Check if player is already in a transport */
  if (is_in_transport(ch))
  {
    send_to_char(ch, "You are already in a transport. Exit first.\r\n");
    return;
  }

  /* Find transport in room */
  if (!get_transport_in_room(IN_ROOM(ch), &td))
  {
    send_to_char(ch, "There is no transport here to enter.\r\n");
    return;
  }

  /* Handle based on transport type */
  switch (td.type)
  {
  case TRANSPORT_VEHICLE:
    vehicle = td.data.vehicle;

    /* Check if vehicle is operational */
    if (!vehicle_is_operational(vehicle))
    {
      send_to_char(ch, "That %s is too damaged to use.\r\n", vehicle_type_name(vehicle->type));
      return;
    }

    /* Check if vehicle has space */
    if (!vehicle_can_add_passenger(vehicle))
    {
      send_to_char(ch, "The %s is full. There is no room for you.\r\n",
                   vehicle_type_name(vehicle->type));
      return;
    }

    /* Add passenger to vehicle */
    if (!vehicle_add_passenger(vehicle))
    {
      send_to_char(ch, "You fail to enter the %s.\r\n", vehicle_type_name(vehicle->type));
      return;
    }

    /* Register player-vehicle association */
    if (!register_player_mount(ch, vehicle))
    {
      vehicle_remove_passenger(vehicle);
      send_to_char(ch, "You fail to enter the %s.\r\n", vehicle_type_name(vehicle->type));
      return;
    }

    /* Success messages */
    send_to_char(ch, "You climb onto %s.\r\n", vehicle->name);
    act("$n climbs onto $T.", TRUE, ch, 0, vehicle->name, TO_ROOM);

    log("Info: %s entered vehicle #%d (%s) via unified command", GET_NAME(ch), vehicle->id,
        vehicle->name);
    break;

  case TRANSPORT_VESSEL:
    /* For vessels, delegate to board command logic */
    /* Vessel boarding is more complex - just inform the player for now */
    send_to_char(ch, "To board a vessel, use the 'board' command.\r\n");
    send_to_char(ch, "Vessel: %s\r\n", td.data.vessel->name);
    break;

  default:
    send_to_char(ch, "There is no transport here to enter.\r\n");
    break;
  }
}

/**
 * do_exit_transport - Unified exit command for vehicles and vessels.
 *
 * Usage: texit
 *
 * Detects current transport type and delegates to appropriate exit handler.
 */
ACMD(do_exit_transport)
{
  struct transport_data td;
  struct vehicle_data *vehicle;

  /* Get current transport */
  if (!get_character_transport(ch, &td))
  {
    send_to_char(ch, "You are not in any transport.\r\n");
    return;
  }

  /* Handle based on transport type */
  switch (td.type)
  {
  case TRANSPORT_VEHICLE:
    vehicle = td.data.vehicle;

    /* Remove passenger from vehicle */
    if (!vehicle_remove_passenger(vehicle))
    {
      send_to_char(ch, "You fail to exit the %s.\r\n", vehicle_type_name(vehicle->type));
      return;
    }

    /* Unregister player-vehicle association */
    if (!unregister_player_mount(ch))
    {
      vehicle_add_passenger(vehicle);
      send_to_char(ch, "You fail to exit the %s.\r\n", vehicle_type_name(vehicle->type));
      return;
    }

    /* Success messages */
    send_to_char(ch, "You dismount from %s.\r\n", vehicle->name);
    act("$n dismounts from $T.", TRUE, ch, 0, vehicle->name, TO_ROOM);

    log("Info: %s exited vehicle #%d (%s) via unified command", GET_NAME(ch), vehicle->id,
        vehicle->name);
    break;

  case TRANSPORT_VESSEL:
    /* For vessels, delegate to disembark command logic */
    send_to_char(ch, "To leave a vessel, use the 'disembark' command.\r\n");
    break;

  default:
    send_to_char(ch, "You are not in any transport.\r\n");
    break;
  }
}

/**
 * do_transport_go - Unified movement command for vehicles and vessels.
 *
 * Usage: go <direction>
 *
 * Detects current transport type and delegates to appropriate movement handler.
 */
ACMD(do_transport_go)
{
  struct transport_data td;
  struct vehicle_data *vehicle;
  char arg[MAX_INPUT_LENGTH];
  int direction;

  /* Parse direction argument */
  one_argument(argument, arg, sizeof(arg));

  if (!*arg)
  {
    send_to_char(ch, "Go in which direction?\r\n");
    send_to_char(ch, "Usage: go <north|south|east|west|ne|nw|se|sw>\r\n");
    return;
  }

  /* Get current transport */
  if (!get_character_transport(ch, &td))
  {
    send_to_char(ch, "You need to be in a transport to use this command.\r\n");
    send_to_char(ch, "Try 'enter' to board a transport first.\r\n");
    return;
  }

  /* Parse direction */
  direction = parse_direction(arg);
  if (direction < 0)
  {
    send_to_char(ch, "'%s' is not a valid direction.\r\n", arg);
    send_to_char(ch, "Valid directions: north, south, east, west, ne, nw, se, sw\r\n");
    return;
  }

  /* Handle based on transport type */
  switch (td.type)
  {
  case TRANSPORT_VEHICLE:
    vehicle = td.data.vehicle;

    /* Check if vehicle is operational */
    if (!vehicle_is_operational(vehicle))
    {
      send_to_char(ch, "The %s is too damaged to move.\r\n", vehicle_type_name(vehicle->type));
      return;
    }

    /* Check if movement is possible */
    if (!vehicle_can_move(vehicle, direction))
    {
      send_to_char(ch, "The %s cannot travel %s from here.\r\n", vehicle_type_name(vehicle->type),
                   get_direction_name(direction));
      return;
    }

    /* Attempt to move the vehicle */
    if (!vehicle_move(vehicle, direction))
    {
      send_to_char(ch, "The %s fails to move.\r\n", vehicle_type_name(vehicle->type));
      return;
    }

    /* Success messages */
    send_to_char(ch, "You drive the %s %s.\r\n", vehicle_type_name(vehicle->type),
                 get_direction_name(direction));
    send_to_char(ch, "Current position: (%d, %d)\r\n", vehicle->x_coord, vehicle->y_coord);
    break;

  case TRANSPORT_VESSEL:
    /* For vessels, delegate to sail/pilot command logic */
    send_to_char(ch, "To sail a vessel, use the navigation commands.\r\n");
    send_to_char(ch, "Try 'heading' to set direction and 'speed' to set speed.\r\n");
    break;

  default:
    send_to_char(ch, "You are not in any transport.\r\n");
    break;
  }
}

/**
 * do_transportstatus - Unified status display for vehicles and vessels.
 *
 * Usage: tstatus
 *
 * Shows status of current transport or transport in room.
 */
ACMD(do_transportstatus)
{
  struct transport_data td;
  struct vehicle_data *vehicle;
  struct greyhawk_ship_data *vessel;
  int condition_pct;

  /* First check if player is in a transport */
  if (!get_character_transport(ch, &td))
  {
    /* If not in transport, check room */
    if (!get_transport_in_room(IN_ROOM(ch), &td))
    {
      send_to_char(ch, "There is no transport here to examine.\r\n");
      return;
    }
  }

  /* Display based on transport type */
  switch (td.type)
  {
  case TRANSPORT_VEHICLE:
    vehicle = td.data.vehicle;

    /* Calculate condition percentage */
    if (vehicle->max_condition > 0)
    {
      condition_pct = (vehicle->condition * 100) / vehicle->max_condition;
    }
    else
    {
      condition_pct = 0;
    }

    /* Display vehicle status */
    send_to_char(ch, "\r\n");
    send_to_char(ch, "=== Transport Status (Vehicle) ===\r\n");
    send_to_char(ch, "\r\n");
    send_to_char(ch, "Name: %s\r\n", vehicle->name);
    send_to_char(ch, "Type: %s\r\n", vehicle_type_name(vehicle->type));
    send_to_char(ch, "State: %s\r\n", vehicle_state_name(vehicle->state));
    send_to_char(ch, "\r\n");
    send_to_char(ch, "Position: (%d, %d)\r\n", vehicle->x_coord, vehicle->y_coord);
    send_to_char(ch, "Speed: %d (base %d)\r\n", vehicle->current_speed, vehicle->base_speed);
    send_to_char(ch, "\r\n");
    send_to_char(ch, "Passengers: %d / %d\r\n", vehicle->current_passengers,
                 vehicle->max_passengers);
    send_to_char(ch, "Cargo: %d / %d lbs\r\n", vehicle->current_weight, vehicle->max_weight);
    send_to_char(ch, "\r\n");
    send_to_char(ch, "Condition: %d / %d (%d%%)\r\n", vehicle->condition, vehicle->max_condition,
                 condition_pct);

    /* Condition description */
    if (condition_pct >= 75)
    {
      send_to_char(ch, "The %s is in good condition.\r\n", vehicle_type_name(vehicle->type));
    }
    else if (condition_pct >= 50)
    {
      send_to_char(ch, "The %s shows some wear but is functional.\r\n",
                   vehicle_type_name(vehicle->type));
    }
    else if (condition_pct >= 25)
    {
      send_to_char(ch, "The %s is in poor condition and needs repair.\r\n",
                   vehicle_type_name(vehicle->type));
    }
    else if (condition_pct > 0)
    {
      send_to_char(ch, "The %s is badly damaged and barely functional.\r\n",
                   vehicle_type_name(vehicle->type));
    }
    else
    {
      send_to_char(ch, "The %s is broken and cannot be used.\r\n",
                   vehicle_type_name(vehicle->type));
    }

    /* Show if player is in this vehicle */
    if (get_player_vehicle(ch) == vehicle)
    {
      send_to_char(ch, "\r\nYou are currently riding this %s.\r\n",
                   vehicle_type_name(vehicle->type));
    }
    send_to_char(ch, "\r\n");
    break;

  case TRANSPORT_VESSEL:
    vessel = td.data.vessel;

    /* Display vessel status */
    send_to_char(ch, "\r\n");
    send_to_char(ch, "=== Transport Status (Vessel) ===\r\n");
    send_to_char(ch, "\r\n");
    send_to_char(ch, "Name: %s\r\n", vessel->name);
    send_to_char(ch, "ID: %s\r\n", vessel->id);
    send_to_char(ch, "Owner: %s\r\n", vessel->owner);
    send_to_char(ch, "\r\n");
    send_to_char(ch, "Position: (%.1f, %.1f, %.1f)\r\n", vessel->x, vessel->y, vessel->z);
    send_to_char(ch, "Heading: %d degrees\r\n", vessel->heading);
    send_to_char(ch, "Speed: %d / %d\r\n", vessel->speed, vessel->maxspeed);
    send_to_char(ch, "\r\n");
    send_to_char(ch, "Rooms: %d\r\n", vessel->num_rooms);
    send_to_char(ch, "Docked: %s\r\n", (vessel->dock > 0) ? "Yes" : "No");
    send_to_char(ch, "\r\n");

    /* Show if player is in this vessel */
    if (is_in_ship_interior(ch))
    {
      send_to_char(ch, "You are currently aboard this vessel.\r\n");
    }
    send_to_char(ch, "\r\n");
    break;

  default:
    send_to_char(ch, "There is no transport here to examine.\r\n");
    break;
  }
}

/* End of transport_unified.c */
