/* ************************************************************************
 *   File:   vehicles_commands.c                        Part of LuminariMUD *
 *  Usage:   Player commands for the Simple Vehicle System                  *
 *                                                                          *
 *  Phase 02, Session 04: Vehicle Player Commands                           *
 *  Implements mount, dismount, drive, and vstatus commands for players     *
 *  to interact with land-based transport vehicles.                         *
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

/* ========================================================================= */
/* STATIC VARIABLES FOR PLAYER-VEHICLE TRACKING                              */
/* ========================================================================= */

/**
 * Maximum number of concurrent mounted players.
 * Matches MAX_VEHICLES in vehicles.c for consistency.
 */
#define MAX_MOUNTED_PLAYERS 1000

/**
 * Player-vehicle association tracking.
 * Maps player ID to vehicle ID for mounted players.
 * Zero values indicate no association.
 */
static struct player_vehicle_map
{
  long player_id; /* Player's ID (GET_IDNUM) */
  int vehicle_id; /* Vehicle's ID */
} mounted_players[MAX_MOUNTED_PLAYERS];

/**
 * Count of currently mounted players.
 */
static int mounted_count = 0;

/* ========================================================================= */
/* HELPER FUNCTIONS (T005, T007, T008)                                       */
/* ========================================================================= */

/**
 * Parse a direction string for the drive command.
 *
 * Supports multiple formats:
 * - Full names: "north", "south", "east", "west", etc.
 * - Abbreviations: "n", "s", "e", "w", "ne", "nw", "se", "sw"
 * - Numeric: 0-7 for the 8 cardinal/diagonal directions
 *
 * @param arg The direction string to parse
 * @return Direction constant (NORTH, SOUTH, etc.) or -1 if invalid
 */
static int parse_drive_direction(const char *arg)
{
  if (arg == NULL || *arg == '\0')
  {
    return -1;
  }

  /* Check for numeric direction (0-7) */
  if (isdigit(*arg) && *(arg + 1) == '\0')
  {
    int dir = *arg - '0';
    if (dir >= 0 && dir <= 7)
    {
      /* Map 0-7 to direction constants */
      switch (dir)
      {
      case 0:
        return NORTH;
      case 1:
        return EAST;
      case 2:
        return SOUTH;
      case 3:
        return WEST;
      case 4:
        return NORTHEAST;
      case 5:
        return NORTHWEST;
      case 6:
        return SOUTHEAST;
      case 7:
        return SOUTHWEST;
      }
    }
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

/**
 * Check if a player is currently in a vehicle.
 *
 * @param ch The character to check
 * @return 1 if player is in a vehicle, 0 otherwise
 */
static int is_player_in_vehicle(struct char_data *ch)
{
  int i;

  if (ch == NULL || IS_NPC(ch))
  {
    return 0;
  }

  for (i = 0; i < MAX_MOUNTED_PLAYERS; i++)
  {
    if (mounted_players[i].player_id == GET_IDNUM(ch))
    {
      return 1;
    }
  }

  return 0;
}

/**
 * Get the vehicle a player is currently riding.
 *
 * @param ch The character to check
 * @return Pointer to vehicle_data if mounted, NULL otherwise
 */
static struct vehicle_data *get_player_vehicle(struct char_data *ch)
{
  int i;
  int vehicle_id;

  if (ch == NULL || IS_NPC(ch))
  {
    return NULL;
  }

  for (i = 0; i < MAX_MOUNTED_PLAYERS; i++)
  {
    if (mounted_players[i].player_id == GET_IDNUM(ch))
    {
      vehicle_id = mounted_players[i].vehicle_id;
      return vehicle_find_by_id(vehicle_id);
    }
  }

  return NULL;
}

/**
 * Register a player as mounted on a vehicle.
 *
 * @param ch The player mounting the vehicle
 * @param vehicle The vehicle being mounted
 * @return 1 on success, 0 on failure
 */
static int register_player_mount(struct char_data *ch, struct vehicle_data *vehicle)
{
  int i;

  if (ch == NULL || vehicle == NULL || IS_NPC(ch))
  {
    return 0;
  }

  /* Check if already mounted */
  if (is_player_in_vehicle(ch))
  {
    return 0;
  }

  /* Find empty slot */
  for (i = 0; i < MAX_MOUNTED_PLAYERS; i++)
  {
    if (mounted_players[i].player_id == 0)
    {
      mounted_players[i].player_id = GET_IDNUM(ch);
      mounted_players[i].vehicle_id = vehicle->id;
      mounted_count++;
      return 1;
    }
  }

  return 0;
}

/**
 * Unregister a player from their vehicle.
 *
 * @param ch The player dismounting
 * @return 1 on success, 0 if not mounted
 */
static int unregister_player_mount(struct char_data *ch)
{
  int i;

  if (ch == NULL || IS_NPC(ch))
  {
    return 0;
  }

  for (i = 0; i < MAX_MOUNTED_PLAYERS; i++)
  {
    if (mounted_players[i].player_id == GET_IDNUM(ch))
    {
      mounted_players[i].player_id = 0;
      mounted_players[i].vehicle_id = 0;
      mounted_count--;
      return 1;
    }
  }

  return 0;
}

/* ========================================================================= */
/* COMMAND IMPLEMENTATIONS (T009-T015)                                       */
/* ========================================================================= */

/**
 * do_vmount - Mount a vehicle in the current room.
 *
 * Usage: vmount [target]
 *
 * Allows a player to enter/mount a vehicle present in their current room.
 * If no target is specified, mounts the first vehicle found.
 * Note: This is separate from do_mount which handles creature mounts.
 *
 * @param ch The character executing the command
 * @param argument The command arguments
 * @param cmd The command number
 * @param subcmd The subcommand number
 */
ACMD(do_vmount)
{
  struct vehicle_data *vehicle;
  char arg[MAX_INPUT_LENGTH];

  /* Parse argument */
  one_argument(argument, arg, sizeof(arg));

  /* Check if player is already mounted */
  if (is_player_in_vehicle(ch))
  {
    send_to_char(ch, "You are already mounted on a vehicle. Dismount first.\r\n");
    return;
  }

  /* Find vehicle in room */
  vehicle = vehicle_find_in_room(IN_ROOM(ch));

  if (vehicle == NULL)
  {
    send_to_char(ch, "There is no vehicle here to mount.\r\n");
    return;
  }

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
    send_to_char(ch, "You fail to mount the %s.\r\n", vehicle_type_name(vehicle->type));
    return;
  }

  /* Register player-vehicle association */
  if (!register_player_mount(ch, vehicle))
  {
    vehicle_remove_passenger(vehicle);
    send_to_char(ch, "You fail to mount the %s.\r\n", vehicle_type_name(vehicle->type));
    return;
  }

  /* Success messages */
  send_to_char(ch, "You climb onto %s.\r\n", vehicle->name);
  act("$n climbs onto $T.", TRUE, ch, 0, vehicle->name, TO_ROOM);

  /* Log for debugging */
  log("Info: %s mounted vehicle #%d (%s)", GET_NAME(ch), vehicle->id, vehicle->name);
}

/**
 * do_vdismount - Dismount from current vehicle.
 *
 * Usage: vdismount
 *
 * Allows a player to exit/dismount from a vehicle they are currently riding.
 * Note: This is separate from do_dismount which handles creature mounts.
 *
 * @param ch The character executing the command
 * @param argument The command arguments (unused)
 * @param cmd The command number
 * @param subcmd The subcommand number
 */
ACMD(do_vdismount)
{
  struct vehicle_data *vehicle;

  /* Check if player is mounted */
  vehicle = get_player_vehicle(ch);

  if (vehicle == NULL)
  {
    send_to_char(ch, "You are not mounted on any vehicle.\r\n");
    return;
  }

  /* Remove passenger from vehicle */
  if (!vehicle_remove_passenger(vehicle))
  {
    send_to_char(ch, "You fail to dismount from the %s.\r\n", vehicle_type_name(vehicle->type));
    return;
  }

  /* Unregister player-vehicle association */
  if (!unregister_player_mount(ch))
  {
    vehicle_add_passenger(vehicle);
    send_to_char(ch, "You fail to dismount from the %s.\r\n", vehicle_type_name(vehicle->type));
    return;
  }

  /* Success messages */
  send_to_char(ch, "You dismount from %s.\r\n", vehicle->name);
  act("$n dismounts from $T.", TRUE, ch, 0, vehicle->name, TO_ROOM);

  /* Log for debugging */
  log("Info: %s dismounted from vehicle #%d (%s)", GET_NAME(ch), vehicle->id, vehicle->name);
}

/**
 * do_drive - Drive a mounted vehicle in a direction.
 *
 * Usage: drive <direction>
 *
 * Moves the vehicle the player is mounted on in the specified direction.
 * Supports all 8 cardinal and diagonal directions.
 *
 * @param ch The character executing the command
 * @param argument The direction to drive
 * @param cmd The command number
 * @param subcmd The subcommand number
 */
ACMD(do_drive)
{
  struct vehicle_data *vehicle;
  char arg[MAX_INPUT_LENGTH];
  int direction;

  /* Parse direction argument */
  one_argument(argument, arg, sizeof(arg));

  if (!*arg)
  {
    send_to_char(ch, "Drive in which direction? (north, south, east, west, ne, nw, se, sw)\r\n");
    return;
  }

  /* Check if player is mounted */
  vehicle = get_player_vehicle(ch);

  if (vehicle == NULL)
  {
    send_to_char(ch, "You need to be mounted on a vehicle to drive.\r\n");
    return;
  }

  /* Parse direction */
  direction = parse_drive_direction(arg);

  if (direction < 0)
  {
    send_to_char(ch, "'%s' is not a valid direction.\r\n", arg);
    send_to_char(ch,
                 "Valid directions: north, south, east, west, ne, nw, se, sw (or n, s, e, w)\r\n");
    return;
  }

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
  act("$n drives $T $u.", TRUE, ch, (void *)(intptr_t)direction, vehicle->name, TO_ROOM);

  /* Show new position */
  send_to_char(ch, "Current position: (%d, %d)\r\n", vehicle->x_coord, vehicle->y_coord);
}

/**
 * do_vstatus - Display vehicle status information.
 *
 * Usage: vstatus
 *
 * Shows detailed information about the vehicle the player is mounted on,
 * or the first vehicle in the room if not mounted.
 *
 * @param ch The character executing the command
 * @param argument The command arguments (unused)
 * @param cmd The command number
 * @param subcmd The subcommand number
 */
ACMD(do_vstatus)
{
  struct vehicle_data *vehicle;
  int condition_pct;

  /* First check if player is mounted */
  vehicle = get_player_vehicle(ch);

  /* If not mounted, check for vehicle in room */
  if (vehicle == NULL)
  {
    vehicle = vehicle_find_in_room(IN_ROOM(ch));
  }

  if (vehicle == NULL)
  {
    send_to_char(ch, "There is no vehicle here to examine.\r\n");
    return;
  }

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
  send_to_char(ch, "=== Vehicle Status ===\r\n");
  send_to_char(ch, "\r\n");
  send_to_char(ch, "Name: %s\r\n", vehicle->name);
  send_to_char(ch, "Type: %s\r\n", vehicle_type_name(vehicle->type));
  send_to_char(ch, "State: %s\r\n", vehicle_state_name(vehicle->state));
  send_to_char(ch, "\r\n");
  send_to_char(ch, "Position: (%d, %d)\r\n", vehicle->x_coord, vehicle->y_coord);
  send_to_char(ch, "Speed: %d (base %d)\r\n", vehicle->current_speed, vehicle->base_speed);
  send_to_char(ch, "\r\n");
  send_to_char(ch, "Passengers: %d / %d\r\n", vehicle->current_passengers, vehicle->max_passengers);
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
    send_to_char(ch, "The %s is broken and cannot be used.\r\n", vehicle_type_name(vehicle->type));
  }

  send_to_char(ch, "\r\n");

  /* Show if player is mounted */
  if (get_player_vehicle(ch) == vehicle)
  {
    send_to_char(ch, "You are currently riding this %s.\r\n", vehicle_type_name(vehicle->type));
  }
}

/* ========================================================================= */
/* VEHICLE TRANSPORT COMMANDS (Phase 02, Session 05)                          */
/* ========================================================================= */

/* External vessel/transport functions */
extern struct greyhawk_ship_data *get_ship_from_room(room_rnum room);
extern int load_vehicle_onto_vessel(struct char_data *ch, struct vehicle_data *vehicle,
                                    struct greyhawk_ship_data *vessel);
extern int unload_vehicle_from_vessel(struct char_data *ch, struct vehicle_data *vehicle);
extern struct vehicle_data **get_loaded_vehicles_list(struct greyhawk_ship_data *vessel,
                                                      int *count);

/**
 * do_loadvehicle - Load a vehicle onto a vessel.
 *
 * Usage: loadvehicle [vehicle]
 *
 * Loads a vehicle from the current room onto a vessel the player is on.
 * Vessel must be stationary or docked.
 *
 * @param ch The character executing the command
 * @param argument The vehicle to load (optional, uses first in room)
 * @param cmd The command number
 * @param subcmd The subcommand number
 */
ACMD(do_loadvehicle)
{
  struct vehicle_data *vehicle;
  struct greyhawk_ship_data *vessel;
  char arg[MAX_INPUT_LENGTH];

  /* Parse argument */
  one_argument(argument, arg, sizeof(arg));

  /* Check if player is on a vessel */
  vessel = get_ship_from_room(IN_ROOM(ch));
  if (vessel == NULL)
  {
    send_to_char(ch, "You must be on a vessel to load vehicles.\r\n");
    return;
  }

  /* Find vehicle - for now, get vehicle from current room/area */
  /* This is simplified; in a full implementation would parse arg to find specific vehicle */
  vehicle = vehicle_find_in_room(IN_ROOM(ch));

  if (vehicle == NULL)
  {
    send_to_char(ch, "There is no vehicle here to load.\r\n");
    return;
  }

  /* Check if player is mounted on this vehicle */
  if (get_player_vehicle(ch) == vehicle)
  {
    send_to_char(ch, "You cannot load a vehicle you are riding. Dismount first.\r\n");
    return;
  }

  /* Attempt to load the vehicle */
  if (load_vehicle_onto_vessel(ch, vehicle, vessel))
  {
    /* Success - function already sends messages */
    log("Info: %s loaded vehicle #%d onto vessel #%d", GET_NAME(ch), vehicle->id, vessel->shipnum);
  }
  /* Failure messages are handled by load_vehicle_onto_vessel */
}

/**
 * do_unloadvehicle - Unload a vehicle from a vessel.
 *
 * Usage: unloadvehicle [vehicle]
 *
 * Unloads a vehicle from the vessel the player is on.
 * Vessel must be stationary or docked, and terrain must be suitable.
 *
 * @param ch The character executing the command
 * @param argument The vehicle to unload (optional, lists if not specified)
 * @param cmd The command number
 * @param subcmd The subcommand number
 */
ACMD(do_unloadvehicle)
{
  struct vehicle_data *vehicle;
  struct vehicle_data **vehicles;
  struct greyhawk_ship_data *vessel;
  char arg[MAX_INPUT_LENGTH];
  int count, i;
  int target_id;

  /* Parse argument */
  one_argument(argument, arg, sizeof(arg));

  /* Check if player is on a vessel */
  vessel = get_ship_from_room(IN_ROOM(ch));
  if (vessel == NULL)
  {
    send_to_char(ch, "You must be on a vessel to unload vehicles.\r\n");
    return;
  }

  /* Get list of loaded vehicles */
  vehicles = get_loaded_vehicles_list(vessel, &count);

  if (vehicles == NULL || count == 0)
  {
    send_to_char(ch, "There are no vehicles loaded on this vessel.\r\n");
    return;
  }

  /* If no argument, list loaded vehicles */
  if (!*arg)
  {
    send_to_char(ch, "Vehicles loaded on %s:\r\n", vessel->name);
    for (i = 0; i < count; i++)
    {
      send_to_char(ch, "  %d. %s [%s]\r\n", i + 1, vehicles[i]->name,
                   vehicle_type_name(vehicles[i]->type));
    }
    send_to_char(ch, "Use 'unloadvehicle <number>' to unload a specific vehicle.\r\n");
    free(vehicles);
    return;
  }

  /* Parse argument as vehicle number */
  target_id = atoi(arg);
  if (target_id < 1 || target_id > count)
  {
    send_to_char(ch, "Invalid vehicle number. Use 'unloadvehicle' to see the list.\r\n");
    free(vehicles);
    return;
  }

  /* Get the target vehicle */
  vehicle = vehicles[target_id - 1];
  free(vehicles);

  /* Attempt to unload */
  if (unload_vehicle_from_vessel(ch, vehicle))
  {
    /* Success - function already sends messages */
    log("Info: %s unloaded vehicle #%d from vessel #%d", GET_NAME(ch), vehicle->id,
        vessel->shipnum);
  }
  /* Failure messages are handled by unload_vehicle_from_vessel */
}

/* ========================================================================= */
/* PUBLIC DISPLAY FUNCTIONS                                                   */
/* ========================================================================= */

/**
 * Show vehicle status to a character.
 * Public wrapper for displaying vehicle information.
 *
 * @param ch The character to show status to
 * @param vehicle The vehicle to display
 */
void vehicle_show_status(struct char_data *ch, struct vehicle_data *vehicle)
{
  int condition_pct;

  if (ch == NULL || vehicle == NULL)
  {
    return;
  }

  /* Calculate condition percentage */
  if (vehicle->max_condition > 0)
  {
    condition_pct = (vehicle->condition * 100) / vehicle->max_condition;
  }
  else
  {
    condition_pct = 0;
  }

  send_to_char(ch, "%s [%s] - %s\r\n", vehicle->name, vehicle_type_name(vehicle->type),
               vehicle_state_name(vehicle->state));
  send_to_char(ch, "  Passengers: %d/%d, Condition: %d%%\r\n", vehicle->current_passengers,
               vehicle->max_passengers, condition_pct);
}

/**
 * Show vehicle passengers to a character.
 * Lists all passengers currently riding the vehicle.
 *
 * @param ch The character to show passengers to
 * @param vehicle The vehicle to list passengers for
 */
void vehicle_show_passengers(struct char_data *ch, struct vehicle_data *vehicle)
{
  int i;
  int count = 0;

  if (ch == NULL || vehicle == NULL)
  {
    return;
  }

  send_to_char(ch, "Passengers on %s:\r\n", vehicle->name);

  /* Iterate through mounted players to find those on this vehicle */
  for (i = 0; i < MAX_MOUNTED_PLAYERS; i++)
  {
    if (mounted_players[i].vehicle_id == vehicle->id && mounted_players[i].player_id != 0)
    {
      count++;
      /* Note: Would need to look up player by ID for name, simplified here */
      send_to_char(ch, "  - Passenger %d\r\n", count);
    }
  }

  if (count == 0)
  {
    send_to_char(ch, "  (none)\r\n");
  }
}

/* ========================================================================= */
/* END OF FILE                                                                */
/* ========================================================================= */
