/* ************************************************************************
 *    File:   vessels.c                              Part of LuminariMUD  *
 * Purpose:   Vessel/Vehicle system for transportation                    *
 *  Header:   vessels.h                                                   *
 *  Author:   [Future Development]                                        *
 *             CWG Vehicle System integration from CircleMUD              *
 ************************************************************************ */

/*
 * This file is currently disabled for compilation.
 * Remove the '#if 0' and '#endif' lines when ready to implement.
 *
 * FILE ORGANIZATION:
 * 1. Includes and Forward Declarations      (Lines 46-51)
 * 2. Global Variables and Data Structures   (Lines 53-79)  
 * 3. Future Advanced Vessel System          (Lines 81-191)
 * 4. CWG Vehicle System Functions           (Lines 193-405)
 * 5. CWG Drive Command                      (Lines 407-557)
 * 6. Outcast Ship System Implementation     (Lines 558-1738)
 *
 * IMPORTANT: This file contains THREE vehicle systems:
 * - Advanced Vessel System (future development, currently stubs)
 * - CWG Vehicle System (fully functional, ready to use)
 * - Outcast Ship System (fully functional, comprehensive ship system)
 *
 * QUICK START FOR NEW DEVELOPERS:
 * - To use the CWG system: Jump to line 193 for working vehicle functions
 * - To use the Outcast system: Jump to line 558 for comprehensive ship system with combat
 * - To extend advanced system: Jump to line 81 for future vessel framework
 * - To understand data structures: Jump to line 53 for vessel_data struct
 */
#if 0

#include "conf.h"
#include "sysdep.h"
#include <math.h>
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "oasis.h"
#include "screen.h"
#include "interpreter.h"
#include "modify.h"
#include "handler.h"
#include "constants.h"
#include "vessels.h"

/* ========================================================================= */
/* INCLUDES AND FORWARD DECLARATIONS                                        */
/* ========================================================================= */

/* Forward declarations for functions that may not exist */
ACMD(do_look);

/* ========================================================================= */
/* GLOBAL VARIABLES AND DATA STRUCTURES                                     */
/* ========================================================================= */

/* Global vessel list for advanced system (future use) */
struct vessel_data *vessel_list = NULL;

/* Advanced Vessel Data Structure (Future Development) */
/* This structure is for the advanced vessel system, not the CWG system */
struct vessel_data {
  int id;                               /* Unique vessel ID */
  char *name;                           /* Vessel name (e.g., "The Seafoam") */
  char *description;                    /* Long description */
  int type;                             /* VESSEL_TYPE_* (sailing ship, etc.) */
  int size;                             /* VESSEL_SIZE_* (small to huge) */
  int state;                            /* VESSEL_STATE_* (docked, traveling, etc.) */
  int location;                         /* Current room vnum */
  int destination;                      /* Target room vnum */
  int speed;                            /* Movement speed in rooms per tick */
  int health;                           /* Current structural health */
  int max_health;                       /* Maximum structural health */
  int capacity;                         /* Maximum passenger capacity */
  int pilot_id;                         /* Character ID of current pilot */
  struct char_data *passengers[50];     /* Array of passenger pointers */
  int num_passengers;                   /* Current passenger count */
  struct vessel_data *next;             /* Linked list pointer for vessel_list */
};

/* ========================================================================= */
/* FUTURE ADVANCED VESSEL SYSTEM (PLACEHOLDER FUNCTIONS)                   */
/* ========================================================================= */
/* These functions are stubs for future development. The CWG system below   */
/* provides working vehicle functionality that can be used immediately.     */

/**
 * Load vessel data from persistent storage
 * TODO: Implement loading from database or file system
 */
void load_vessels(void) {
  /* TODO: Load vessels from file/database */
  log("VESSELS: Loading vessel data...");
}

/**
 * Save vessel data to persistent storage
 * TODO: Implement saving to database or file system
 */
void save_vessels(void) {
  /* TODO: Save vessels to file/database */
  log("VESSELS: Saving vessel data...");
}

/**
 * Find a vessel by its unique ID
 * @param vessel_id The unique ID to search for
 * @return Pointer to vessel_data structure, or NULL if not found
 */
struct vessel_data *find_vessel_by_id(int vessel_id) {
  struct vessel_data *vessel;
  
  for (vessel = vessel_list; vessel; vessel = vessel->next) {
    if (vessel->id == vessel_id)
      return vessel;
  }
  
  return NULL;
}

/**
 * Process vessel movement each game tick
 * Called from the game's main loop to handle automatic vessel movement
 * TODO: Implement pathfinding and movement mechanics
 */
void vessel_movement_tick(void) {
  struct vessel_data *vessel;
  
  for (vessel = vessel_list; vessel; vessel = vessel->next) {
    if (vessel->state == VESSEL_STATE_TRAVELING) {
      /* TODO: Implement movement logic */
    }
  }
}

/**
 * Handle a character boarding a vessel
 * @param ch Character attempting to board
 * @param vessel Vessel to board
 */
void enter_vessel(struct char_data *ch, struct vessel_data *vessel) {
  if (!ch || !vessel)
    return;
    
  if (vessel->num_passengers >= vessel->capacity) {
    send_to_char(ch, "The %s is at full capacity.\r\n", vessel->name);
    return;
  }
  
  /* TODO: Implement boarding logic */
  send_to_char(ch, "You board the %s.\r\n", vessel->name);
}

/**
 * Handle a character leaving a vessel
 * @param ch Character attempting to disembark
 */
void exit_vessel(struct char_data *ch) {
  if (!ch)
    return;
    
  /* TODO: Implement disembarking logic */
  send_to_char(ch, "You disembark from the vessel.\r\n");
}

/**
 * Check if a character can pilot a vessel
 * @param ch Character attempting to pilot
 * @param vessel Vessel to be piloted
 * @return TRUE if character can pilot, FALSE otherwise
 */
int can_pilot_vessel(struct char_data *ch, struct vessel_data *vessel) {
  if (!ch || !vessel)
    return FALSE;
    
  /* TODO: Check piloting skills/requirements */
  return TRUE;
}

/**
 * Handle piloting a vessel in a direction
 * @param ch Character doing the piloting
 * @param direction Direction to move (NORTH, SOUTH, etc.)
 */
void pilot_vessel(struct char_data *ch, int direction) {
  if (!ch)
    return;
    
  /* TODO: Implement piloting logic */
  send_to_char(ch, "You steer the vessel.\r\n");
}

/* ========================================================================= */
/* CWG VEHICLE SYSTEM FUNCTIONS (READY TO USE)                             */
/* ========================================================================= */
/* This system uses objects as vehicles and is fully functional.            */
/* Objects with ITEM_VEHICLE type represent the vehicles themselves.        */
/* Objects with ITEM_CONTROL type are used to pilot vehicles.               */
/* Objects with ITEM_HATCH type are exits from vehicles.                    */

/**
 * Find a vehicle object by its vnum in the global object list
 * @param vnum Virtual number of the vehicle object to find
 * @return Pointer to the vehicle object, or NULL if not found
 * 
 * Usage: Used internally by the drive system to locate vehicles
 * that are referenced by control objects.
 */
struct obj_data *find_vehicle_by_vnum(int vnum) {
  extern struct obj_data * object_list;
  struct obj_data * i;

  for (i = object_list; i; i = i->next)
    if (GET_OBJ_TYPE(i) == ITEM_VEHICLE)
      if (GET_OBJ_VNUM(i) == vnum)
        return i;

  return NULL;
}

/**
 * Search a list of objects for the first object of a specific type
 * @param type Object type to search for (ITEM_VEHICLE, ITEM_CONTROL, etc.)
 * @param list Head of the object list to search
 * @return Pointer to first matching object, or NULL if none found
 * 
 * Usage: Generic utility function used to find controls, hatches, etc.
 * in rooms, inventory, or equipment lists.
 */
struct obj_data *get_obj_in_list_type(int type, struct obj_data *list) {
  struct obj_data * i;

  for (i = list; i; i = i->next_content)
    if (GET_OBJ_TYPE(i) == type)
      return i;

  return NULL;
}

/**
 * Find vehicle controls that a character can use
 * Searches in order: room contents, character inventory, character equipment
 * @param ch Character looking for controls
 * @return Pointer to control object, or NULL if none found
 * 
 * Usage: Called by do_drive to determine if character can pilot a vehicle.
 * Controls can be in the room (helm), carried (remote), or worn (crown).
 */
struct obj_data *find_control(struct char_data *ch) {
  struct obj_data *controls, *obj;
  int j;

  /* First check room contents for controls (like a ship's helm) */
  controls = get_obj_in_list_type(ITEM_CONTROL, world[IN_ROOM(ch)].contents);
  
  /* Then check character's inventory for portable controls */
  if (!controls)
    for (obj = ch->carrying; obj && !controls; obj = obj->next_content)
      if (CAN_SEE_OBJ(ch, obj) && GET_OBJ_TYPE(obj) == ITEM_CONTROL)
        controls = obj;
        
  /* Finally check worn equipment for control items */
  if (!controls)
    for (j = 0; j < NUM_WEARS && !controls; j++)
      if (GET_EQ(ch, j) && CAN_SEE_OBJ(ch, GET_EQ(ch, j)) &&
              GET_OBJ_TYPE(GET_EQ(ch, j)) == ITEM_CONTROL)
        controls = GET_EQ(ch, j);
        
  return controls;
}

/**
 * Drive one vehicle into another vehicle (like driving a car onto a ferry)
 * @param ch Character doing the driving
 * @param vehicle The vehicle being driven
 * @param arg Name of the target vehicle to drive into
 * 
 * Usage: Called when player uses "drive into <vehicle_name>"
 * The target vehicle must have ROOM_VEHICLE flag set in its interior room.
 * Vehicle value[0] should contain the room vnum of the vehicle's interior.
 */
void drive_into_vehicle(struct char_data *ch, struct obj_data *vehicle, char *arg) {
  struct obj_data *vehicle_in_out;
  int was_in, is_in, is_going_to;
  char buf[MAX_INPUT_LENGTH];

  if (!*arg) {
    send_to_char(ch, "Drive into what?\r\n");
  } else if (!(vehicle_in_out = get_obj_in_list_vis(ch, arg, NULL, world[IN_ROOM(vehicle)].contents))) {
    send_to_char(ch, "Nothing here by that name!\r\n");
  } else if (GET_OBJ_TYPE(vehicle_in_out) != ITEM_VEHICLE) {
    send_to_char(ch, "That's not a vehicle.\r\n");
  } else if (vehicle == vehicle_in_out) {
    send_to_char(ch, "My, we are in a clever mood today, aren't we.\r\n");
  } else {
    /* Get the interior room of the target vehicle */
    is_going_to = real_room(GET_OBJ_VAL(vehicle_in_out, 0));
    if (!IS_SET_AR(ROOM_FLAGS(is_going_to), ROOM_VEHICLE)) {
      send_to_char(ch, "That vehicle can't carry other vehicles.");
    } else {
      /* Move the vehicle into the target vehicle */
      sprintf(buf, "%s enters %s.\n\r", vehicle->short_description,
              vehicle_in_out->short_description);
      send_to_room(IN_ROOM(vehicle), buf);

      was_in = IN_ROOM(vehicle);
      obj_from_room(vehicle);
      obj_to_room(vehicle, is_going_to);
      is_in = IN_ROOM(vehicle);
      
      /* Update driver's view and notify destination room */
      if (ch->desc != NULL)
        look_at_room(ch, 0);  /* Fixed: Luminari signature is (ch, mode) */
      sprintf(buf, "%s enters.\r\n", vehicle->short_description);
      send_to_room(is_in, buf);
    }
  }
}

/**
 * Drive a vehicle out of another vehicle (like driving off a ferry)
 * @param ch Character doing the driving
 * @param vehicle The vehicle being driven out
 * 
 * Usage: Called when player uses "drive outside"
 * Requires an ITEM_HATCH object in the current room that points to the
 * parent vehicle via its value[0] field containing the parent vehicle's vnum.
 */
void drive_outof_vehicle(struct char_data *ch, struct obj_data *vehicle) {
  struct obj_data *hatch, *vehicle_in_out;
  char buf[MAX_INPUT_LENGTH];

  /* Look for a hatch (exit) in the current room */
  if (!(hatch = get_obj_in_list_type(ITEM_HATCH, world[IN_ROOM(vehicle)].contents))) {
    send_to_char(ch, "Nowhere to drive out of.\r\n");
  } else if (!(vehicle_in_out = find_vehicle_by_vnum(GET_OBJ_VAL(hatch, 0)))) {
    send_to_char(ch, "You can't drive out anywhere!\r\n");
  } else {
    /* Announce departure from interior */
    sprintf(buf, "%s exits %s.\r\n", vehicle->short_description,
            vehicle_in_out->short_description);
    send_to_room(IN_ROOM(vehicle), buf);

    /* Move vehicle to the same room as the parent vehicle */
    obj_from_room(vehicle);
    obj_to_room(vehicle, IN_ROOM(vehicle_in_out));

    /* Update driver's view and announce arrival */
    if (ch->desc != NULL)
      look_at_room(ch, 0);  /* Fixed: Luminari signature is (ch, mode) */

    sprintf(buf, "%s drives out of %s.\r\n", vehicle->short_description,
            vehicle_in_out->short_description);
    send_to_room(IN_ROOM(vehicle), buf);
  }
}

/**
 * Drive a vehicle in a compass direction (north, south, east, west, etc.)
 * @param ch Character doing the driving
 * @param vehicle The vehicle being driven
 * @param dir Direction constant (NORTH, SOUTH, EAST, WEST, UP, DOWN)
 * 
 * Usage: Called when player uses "drive north", "drive east", etc.
 * The destination room must have the ROOM_VEHICLE flag set to allow vehicles.
 * Standard movement restrictions apply (closed doors, no exit, etc.).
 */
void drive_in_direction(struct char_data *ch, struct obj_data *vehicle, int dir) {
  char buf[MAX_INPUT_LENGTH];

  /* Check if there's an exit in that direction */
  if (!EXIT(vehicle, dir) || EXIT(vehicle, dir)->to_room == NOWHERE) {
    send_to_char(ch, "Alas, you cannot go that way...\r\n");
  } else if (IS_SET(EXIT(vehicle, dir)->exit_info, EX_CLOSED)) {
    /* Check if the exit is blocked by a closed door */
    if (EXIT(vehicle, dir)->keyword)
      send_to_char(ch, "The %s seems to be closed.\r\n", fname(EXIT(vehicle, dir)->keyword));
    else
      send_to_char(ch, "It seems to be closed.\r\n");

  } else if (!IS_SET_AR(ROOM_FLAGS(EXIT(vehicle, dir)->to_room), ROOM_VEHICLE)) {
    /* Destination room doesn't allow vehicles */
    send_to_char(ch, "The vehicle can't manage that terrain.\r\n");
  } else {
    /* All checks passed - move the vehicle */
    int was_in, is_in;

    /* Announce departure from current room */
    sprintf(buf, "%s leaves %s.\n\r", vehicle->short_description, dirs[dir]);
    send_to_room(IN_ROOM(vehicle), buf);

    /* Move the vehicle object */
    was_in = IN_ROOM(vehicle);
    obj_from_room(vehicle);
    obj_to_room(vehicle, world[was_in].dir_option[dir]->to_room);
    is_in = IN_ROOM(vehicle);

    /* Update driver's view and announce arrival */
    if (ch->desc != NULL)
      look_at_room(ch, 0);  /* Fixed: Luminari signature is (ch, mode) */
    sprintf(buf, "%s enters from the %s.\r\n",
            vehicle->short_description, dirs[rev_dir[dir]]);
    send_to_room(is_in, buf);
  }
}

/* ========================================================================= */
/* COMMAND FUNCTIONS - FUTURE ADVANCED SYSTEM                              */
/* ========================================================================= */
/* These are placeholder commands for the future advanced vessel system     */

/**
 * Command to board a vessel (future advanced system)
 * TODO: Implement full boarding mechanics with capacity checks, etc.
 */
ACMD(do_board) {
  /* TODO: Implement board command for advanced vessel system */
  send_to_char(ch, "Board command not yet implemented.\r\n");
}

/**
 * Command to disembark from a vessel (future advanced system)
 * TODO: Implement disembarking with location validation, etc.
 */
ACMD(do_disembark) {
  /* TODO: Implement disembark command for advanced vessel system */
  send_to_char(ch, "Disembark command not yet implemented.\r\n");
}

/**
 * Command to pilot a vessel (future advanced system)
 * TODO: Implement vessel piloting with skill checks, etc.
 */
ACMD(do_pilot) {
  /* TODO: Implement pilot command for advanced vessel system */
  send_to_char(ch, "Pilot command not yet implemented.\r\n");
}

/**
 * Command to show vessel status (future advanced system)
 * TODO: Show health, passengers, speed, destination, etc.
 */
ACMD(do_vessel_status) {
  /* TODO: Implement vessel status command for advanced vessel system */
  send_to_char(ch, "Vessel status command not yet implemented.\r\n");
}

/* ========================================================================= */
/* CWG DRIVE COMMAND (READY TO USE)                                         */
/* ========================================================================= */

/* Simple compatibility functions for missing validation */
/* TODO: Replace these with proper Luminari equivalents when available */

/**
 * Check if character's alignment prevents using an object
 * @param ch Character attempting to use object
 * @param obj Object being used
 * @return TRUE if alignment prevents use, FALSE otherwise
 * TODO: Implement proper alignment checking based on object flags
 */
int invalid_align(struct char_data *ch, struct obj_data *obj) {
  /* For now, assume no alignment restrictions */
  return FALSE;
}

/**
 * Check if character's class prevents using an object
 * @param ch Character attempting to use object
 * @param obj Object being used
 * @return TRUE if class prevents use, FALSE otherwise
 * TODO: Implement proper class checking based on object flags
 */
int invalid_class(struct char_data *ch, struct obj_data *obj) {
  /* For now, assume no class restrictions */
  return FALSE;
}

/**
 * Check if character's race prevents using an object
 * @param ch Character attempting to use object
 * @param obj Object being used
 * @return TRUE if race prevents use, FALSE otherwise
 * TODO: Implement proper race checking based on object flags
 */
int invalid_race(struct char_data *ch, struct obj_data *obj) {
  /* For now, assume no race restrictions */
  return FALSE;
}

/**
 * Main drive command for the CWG vehicle system
 * 
 * Usage:
 *   drive north          - Drive vehicle north
 *   drive into ferry     - Drive vehicle into another vehicle named "ferry"
 *   drive outside        - Drive vehicle out of current parent vehicle
 * 
 * Prerequisites:
 *   - Character must have access to an ITEM_CONTROL object
 *   - Control object's value[0] must contain vnum of ITEM_VEHICLE to control
 *   - Vehicle must be in a room with ROOM_VEHICLE flag (for directional movement)
 *   - Target rooms must have ROOM_VEHICLE flag set
 * 
 * @param ch Character issuing the command
 * @param cmd Command number (unused)
 * @param argument Command arguments (direction or "into <target>")
 * @param subcmd Subcommand (unused)
 */
ACMD(do_drive) {
  int dir;
  struct obj_data *vehicle, *controls;

  /* Basic state checks */
  if (GET_POS(ch) < POS_SLEEPING) {
    send_to_char(ch, "You can't see anything but stars!\r\n");
  } else if (AFF_FLAGGED(ch, AFF_BLIND)) {
    send_to_char(ch, "You can't see a damned thing, you're blind!\r\n");
  } else if (IS_DARK(IN_ROOM(ch)) && !CAN_SEE_IN_DARK(ch)) {
    send_to_char(ch, "It is pitch black...\r\n");
  } else if (!(controls = find_control(ch))) {
    send_to_char(ch, "You have no idea how to drive anything here.\r\n");
  } else if (invalid_align(ch, controls) ||
          invalid_class(ch, controls) ||
          invalid_race(ch, controls)) {
    /* Object restrictions prevent use */
    act("You are zapped by $p and instantly step away from it.", FALSE, ch, controls, 0, TO_CHAR);
    act("$n is zapped by $p and instantly steps away from it.", FALSE, ch, controls, 0, TO_ROOM);
  } else if (!(vehicle = find_vehicle_by_vnum(GET_OBJ_VAL(controls, 0)))) {
    send_to_char(ch, "You can't find anything to drive.\r\n");
  } else {
    /* Parse command arguments */
    char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];

    argument = any_one_arg(argument, arg);  /* Get first word */
    one_argument(argument, arg2);           /* Get second word */
    
    if (!*arg) {
      send_to_char(ch, "Drive, yes, but where?\r\n");
    } else if (is_abbrev(arg, "into") ||
            is_abbrev(arg, "inside") ||
            is_abbrev(arg, "onto")) {
      /* Command: drive into <vehicle> */
      drive_into_vehicle(ch, vehicle, arg2);
    } else if (is_abbrev(arg, "outside") && !EXIT(vehicle, OUTDIR)) {
      /* Command: drive outside */
      drive_outof_vehicle(ch, vehicle);
    } else if ((dir = search_block(arg, dirs, FALSE)) >= 0) {
      /* Command: drive <direction> */
      drive_in_direction(ch, vehicle, dir);
    } else {
      send_to_char(ch, "Thats not a valid direction.\r\n");
    }
  }
}

/* ========================================================================= */
/* OUTCAST SHIP SYSTEM IMPLEMENTATION (FULLY FUNCTIONAL)                   */
/* ========================================================================= */
/* Integrated from Outcast MUD - comprehensive ship system with navigation  */
/* autopilot, combat, docking, and multi-room ships                        */

/* Global variables for Outcast ship system */
extern struct time_info_data time_info;  /* External time structure */
struct outcast_ship_data outcast_ships[MAX_NUM_SHIPS];
int total_num_outcast_ships = 0;

/* Navigation path strings - these would typically be loaded from data files */
/* Sample paths for demonstration - replace with actual game paths */
static char outcast_path_sample1[] = "00111222333.";
static char outcast_path_sample2[] = "22333000111.";

/* Navigation configuration table */
static struct outcast_navigation_data outcast_nav_info[] = {
  /* Sample navigation entry - customize for your world */
  {-1, FALSE, -1, outcast_path_sample1, outcast_path_sample2, 0, -1, -1, 0, 6, 12},
  /* Terminator entry - must be all zeroes */
  {0, FALSE, 0, 0, 0, 0, 0, 0, 0, 0, 0}
};

/**
 * Find a ship in the global ship list by its object
 * @param obj The ship object to search for
 * @return Ship index if found, -1 if not found
 */
int find_outcast_ship(struct obj_data *obj) {
  int i;
  struct obj_data *t_obj;

  for (i = 0; i < total_num_outcast_ships; i++) {
    if (outcast_ships[i].obj == obj)
      return i;
    
    if (outcast_ships[i].obj_num == GET_OBJ_RNUM(obj)) {
      if (!outcast_ships[i].obj) {
        /* A new object was found! */
        outcast_ships[i].obj = obj;
        outcast_ships[i].in_room = IN_ROOM(obj);
        outcast_ships[i].hull = GET_OBJ_VAL(obj, 0);
        outcast_ships[i].speed = GET_OBJ_VAL(obj, 1);
        outcast_ships[i].damage = GET_OBJ_VAL(obj, 2);
        outcast_ships[i].capacity = GET_OBJ_VAL(obj, 3);
        outcast_ships[i].size = outcast_ships[i].hull >> 3;
        return i;
      } else if (outcast_ships[i].in_room != NOWHERE) {
        /* Check if the old object still exists in the room */
        for (t_obj = world[outcast_ships[i].in_room].contents; t_obj; t_obj = t_obj->next_content) {
          if (t_obj == outcast_ships[i].obj)
            return -1;  /* Can't have two same ship objects */
        }
        
        /* Use this obj as new ship obj */
        outcast_ships[i].obj = obj;
        outcast_ships[i].in_room = IN_ROOM(obj);
        outcast_ships[i].hull = GET_OBJ_VAL(obj, 0);
        outcast_ships[i].speed = GET_OBJ_VAL(obj, 1);
        outcast_ships[i].damage = GET_OBJ_VAL(obj, 2);
        outcast_ships[i].capacity = GET_OBJ_VAL(obj, 3);
        outcast_ships[i].size = outcast_ships[i].hull >> 3;
        return i;
      }
      return -1;
    }
  }
  return -1;
}

/**
 * Check if a ship is currently docked
 * @param t_ship Ship index
 * @return TRUE if docked, FALSE otherwise
 */
bool is_outcast_ship_docked(int t_ship) {
  if ((outcast_ships[t_ship].in_room != NOWHERE) &&
      (SECT(outcast_ships[t_ship].in_room) != SECT_WATER_SWIM) &&
      (SECT(outcast_ships[t_ship].in_room) != SECT_WATER_NOSWIM) &&
      (SECT(outcast_ships[t_ship].in_room) != SECT_UNDERWATER))
    return TRUE;
  else
    return (outcast_ships[t_ship].dock_vehicle != -1);
}

/**
 * Update ship location by searching for the ship object
 * @param t_ship Ship index
 * @return TRUE if ship found and updated, FALSE otherwise
 */
static bool update_outcast_ship_location(int t_ship) {
  struct obj_data *obj;

  for (obj = object_list; obj; obj = obj->next) {
    if (GET_OBJ_RNUM(obj) == outcast_ships[t_ship].obj_num) {
      if (IN_ROOM(obj) != NOWHERE) {
        outcast_ships[t_ship].in_room = IN_ROOM(obj);
        outcast_ships[t_ship].obj = obj;
        outcast_ships[t_ship].hull = GET_OBJ_VAL(obj, 0);
        outcast_ships[t_ship].speed = GET_OBJ_VAL(obj, 1);
        outcast_ships[t_ship].damage = GET_OBJ_VAL(obj, 2);
        outcast_ships[t_ship].capacity = GET_OBJ_VAL(obj, 3);
        outcast_ships[t_ship].size = outcast_ships[t_ship].hull >> 3;
        return TRUE;
      }
    }
  }
  outcast_ships[t_ship].obj = NULL;
  return FALSE;
}

/**
 * Count the number of characters currently in a ship
 * @param t_ship Ship index
 * @return Number of characters in ship
 */
static int num_char_in_outcast_ship(int t_ship) {
  int i, num = 0;
  struct char_data *ch;

  for (i = 0; i < outcast_ships[t_ship].num_room; i++) {
    for (ch = world[outcast_ships[t_ship].room_list[i]].people; ch; ch = ch->next_in_room) {
      num++;
    }
  }
  return num;
}

/**
 * Check if a ship is valid (object exists in expected location)
 * @param t_ship Ship index
 * @return TRUE if valid, FALSE otherwise
 */
bool is_valid_outcast_ship(int t_ship) {
  struct obj_data *obj;

  if (outcast_ships[t_ship].in_room == NOWHERE)
    return FALSE;

  for (obj = world[outcast_ships[t_ship].in_room].contents; obj; obj = obj->next_content) {
    if (obj == outcast_ships[t_ship].obj)
      return TRUE;
  }

  return update_outcast_ship_location(t_ship);
}

/**
 * Initialize a ship by associating an entrance room with a ship object
 * @param ent_room Virtual number of entrance room
 * @param obj_num Virtual number of ship object
 */
static void init_outcast_ship(int ent_room, int obj_num) {
  struct room_direction_data *dir;
  int i = 0, j, d, num;

  num = total_num_outcast_ships++;

  outcast_ships[num].entrance_room = real_room(ent_room);
  outcast_ships[num].room_list[0] = real_room(ent_room);
  outcast_ships[num].obj_num = real_object(obj_num);
  outcast_ships[num].dock_vehicle = -1;

  if (outcast_ships[num].entrance_room == NOWHERE) {
    total_num_outcast_ships--;
    return;
  }
  if (outcast_ships[num].obj_num < 0) {
    total_num_outcast_ships--;
    return;
  }

  outcast_ships[num].num_room = 1;
  while ((outcast_ships[num].num_room <= MAX_NUM_ROOMS) && (i < outcast_ships[num].num_room)) {
    for (d = 0; d < NUM_OF_DIRS; d++) {
      if ((dir = world[outcast_ships[num].room_list[i]].dir_option[d])) {
        if (dir->to_room == NOWHERE)
          continue;
        for (j = 0; j < outcast_ships[num].num_room; j++) {
          if (outcast_ships[num].room_list[j] == dir->to_room)
            break;
        }
        if (j == outcast_ships[num].num_room) {
          outcast_ships[num].room_list[j] = dir->to_room;
          outcast_ships[num].num_room++;
        }
      }
    }
    i++;
  }

  if (outcast_ships[num].num_room > MAX_NUM_ROOMS) {
    log("SHIPS: Cannot allocate more than %d rooms per ship (room=%d).", MAX_NUM_ROOMS, ent_room);
    total_num_outcast_ships--;
    return;
  }
  
  if (update_outcast_ship_location(num) == FALSE) {
    log("SHIP: Need to have an object #%d of ship type", obj_num);
    total_num_outcast_ships--;
    return;
  }
}

/**
 * Initialize all ships in the game
 * Call this during boot sequence
 */
void initialize_outcast_ships(void) {
  int i;

  /* Convert virtual numbers to real numbers for navigation data */
  for (i = 0; outcast_nav_info[i].mob != 0; i++) {
    outcast_nav_info[i].mob = real_mobile(outcast_nav_info[i].mob);
    outcast_nav_info[i].control_room = real_room(outcast_nav_info[i].control_room);
    outcast_nav_info[i].start1 = real_room(outcast_nav_info[i].start1);
    outcast_nav_info[i].destination1 = real_room(outcast_nav_info[i].destination1);
  }

  /* Initialize ship array */
  memset(outcast_ships, 0, sizeof(struct outcast_ship_data) * MAX_NUM_SHIPS);

  for (i = 0; i < MAX_NUM_SHIPS; i++)
    outcast_ships[i].in_room = NOWHERE;

  /* Initialize specific ships - customize these for your world */
  /* Example: init_outcast_ship(entrance_room_vnum, ship_object_vnum); */
  /* Add your ship initialization calls here */
}

/**
 * Determine which ship a character is currently in
 * @param ch Character to check
 * @return Ship index if in a ship, -1 otherwise
 */
int in_which_outcast_ship(struct char_data *ch) {
  int i, j;

  /* Quick sector check - if not in an interior room, probably not in ship */
  if (SECT(IN_ROOM(ch)) > SECT_CITY)
    return -1;

  for (i = 0; i < total_num_outcast_ships; i++) {
    if (!outcast_ships[i].room_list)
      continue;
    for (j = 0; j < outcast_ships[i].num_room; j++) {
      if (IN_ROOM(ch) == outcast_ships[i].room_list[j])
        return i;
    }
  }
  return -1;
}

/**
 * Transfer all contents from one room to another
 * @param from_room Source room
 * @param to_room Destination room
 */
static void transfer_all_room_to_room(int from_room, int to_room) {
  struct obj_data *obj, *next_obj;
  struct char_data *ch, *next_ch;

  for (obj = world[from_room].contents; obj; obj = next_obj) {
    next_obj = obj->next_content;
    obj_from_room(obj);
    obj_to_room(obj, to_room);
  }
  
  for (ch = world[from_room].people; ch; ch = next_ch) {
    next_ch = ch->next_in_room;
    char_from_room(ch);
    char_to_room(ch, to_room);
  }
}

/**
 * Send a message to all characters in a ship (outside rooms only)
 * @param t_ship Ship index
 * @param msg Message to send
 */
static void act_to_all_in_outcast_ship_outside(int t_ship, const char *msg) {
  int i;

  for (i = 0; i < outcast_ships[t_ship].num_room; i++) {
    if ((outcast_ships[t_ship].room_list[i] != NOWHERE) &&
        (SECT(outcast_ships[t_ship].room_list[i]) != SECT_INSIDE) &&
        world[outcast_ships[t_ship].room_list[i]].people) {
      act(msg, FALSE, world[outcast_ships[t_ship].room_list[i]].people, 0, 0, TO_ROOM);
      act(msg, FALSE, world[outcast_ships[t_ship].room_list[i]].people, 0, 0, TO_CHAR);
    }
  }
}

/**
 * Send a message to all characters in a ship
 * @param t_ship Ship index
 * @param msg Message to send
 */
static void act_to_all_in_outcast_ship(int t_ship, const char *msg) {
  int i;

  for (i = 0; i < outcast_ships[t_ship].num_room; i++) {
    if (outcast_ships[t_ship].room_list[i] != NOWHERE &&
        world[outcast_ships[t_ship].room_list[i]].people) {
      act(msg, FALSE, world[outcast_ships[t_ship].room_list[i]].people, 0, 0, TO_ROOM);
      act(msg, FALSE, world[outcast_ships[t_ship].room_list[i]].people, 0, 0, TO_CHAR);
    }
  }
}

/**
 * Sink a ship and move all contents to the water
 * @param t_ship Ship index
 */
void sink_outcast_ship(int t_ship) {
  int i;
  char buf[MAX_STRING_LENGTH];

  if (outcast_ships[t_ship].in_room == NOWHERE) {
    log("SHIP: Strange... you just sunk a ghost ship.");
    return;
  }
  
  if (!outcast_ships[t_ship].obj) {
    log("SHIP: Strange... ship object doesn't exist.");
    return;
  }

  /* Display message to everyone in ocean room */
  if (world[outcast_ships[t_ship].in_room].people) {
    act("$p sinks into the water.", FALSE,
        world[outcast_ships[t_ship].in_room].people, outcast_ships[t_ship].obj, 0, TO_ROOM);
    act("$p sinks into the water.", FALSE,
        world[outcast_ships[t_ship].in_room].people, outcast_ships[t_ship].obj, 0, TO_CHAR);
  }

  /* Display message to everyone in ships in the same ocean room */
  for (i = 0; i < total_num_outcast_ships; i++) {
    if (outcast_ships[i].in_room == outcast_ships[t_ship].in_room) {
      if (t_ship == i) {
        sprintf(buf, "Your ship sinks into the water!\r\nYou are swimming in the ocean!");
        act_to_all_in_outcast_ship(i, buf);
      } else {
        sprintf(buf, "%s sinks into the water.", outcast_ships[t_ship].obj->short_description);
        act_to_all_in_outcast_ship(i, buf);
      }
    }
  }

  /* Move all mobs and items from ship to ocean room */
  for (i = 0; i < outcast_ships[t_ship].num_room; i++) {
    transfer_all_room_to_room(outcast_ships[t_ship].room_list[i], outcast_ships[t_ship].in_room);
  }

  /* Undock any ship */
  if (outcast_ships[t_ship].dock_vehicle != -1) {
    outcast_ships[outcast_ships[t_ship].dock_vehicle].dock_vehicle = -1;
    outcast_ships[t_ship].dock_vehicle = -1;
  }
  
  outcast_ships[t_ship].obj = NULL;
}

/**
 * Move a ship in a direction
 * @param t_ship Ship index
 * @param dir Direction command constant
 * @param ch Character controlling the ship (for messages)
 * @return TRUE if movement successful, FALSE otherwise
 */
bool move_outcast_ship(int t_ship, int dir, struct char_data *ch) {
  char buf[MAX_STRING_LENGTH];
  const char *directions[] = {"north", "east", "south", "west", "up", "down"};
  int direction, to_room;

  /* Convert command to direction index */
  switch (dir) {
    case CMD_NORTH: direction = NORTH; break;
    case CMD_EAST:  direction = EAST;  break;
    case CMD_SOUTH: direction = SOUTH; break;
    case CMD_WEST:  direction = WEST;  break;
    case CMD_UP:    direction = UP;    break;
    case CMD_DOWN:  direction = DOWN;  break;
    default:
      log("SHIP: move_outcast_ship error - invalid direction");
      return FALSE;
  }

  if (!world[outcast_ships[t_ship].in_room].dir_option[direction]) {
    if (ch)
      send_to_char(ch, "You will drop off the face of the world if you sail there.\r\n");
    return FALSE;
  }

  if ((to_room = world[outcast_ships[t_ship].in_room].dir_option[direction]->to_room) == NOWHERE) {
    if (ch)
      send_to_char(ch, "You will drop off the face of the world if you sail there.\r\n");
    return FALSE;
  }

  /* Check if destination allows ships */
  if (!ROOM_FLAGGED(to_room, DOCKABLE) &&
      (SECT(to_room) != SECT_WATER_SWIM) &&
      (SECT(to_room) != SECT_WATER_NOSWIM) &&
      (SECT(to_room) != SECT_UNDERWATER)) {
    if (ch)
      send_to_char(ch, "The ship can only sail on water.\r\n");
    return FALSE;
  }

  /* Announce departure */
  if (world[outcast_ships[t_ship].in_room].people) {
    sprintf(buf, "%s sails %sward.", outcast_ships[t_ship].obj->short_description, directions[direction]);
    CAP(buf);
    act(buf, FALSE, world[outcast_ships[t_ship].in_room].people, 0, 0, TO_ROOM);
    act(buf, FALSE, world[outcast_ships[t_ship].in_room].people, 0, 0, TO_CHAR);
  }

  /* Move the ship object */
  obj_from_room(outcast_ships[t_ship].obj);
  obj_to_room(outcast_ships[t_ship].obj, to_room);
  outcast_ships[t_ship].in_room = to_room;

  /* Announce arrival */
  if (world[outcast_ships[t_ship].in_room].people) {
    sprintf(buf, "%s sails here from the %s.", outcast_ships[t_ship].obj->short_description, directions[direction]);
    CAP(buf);
    act(buf, FALSE, world[outcast_ships[t_ship].in_room].people, 0, 0, TO_ROOM);
    act(buf, FALSE, world[outcast_ships[t_ship].in_room].people, 0, 0, TO_CHAR);
  }

  /* Notify passengers */
  sprintf(buf, "Your ship sails %sward.", directions[direction]);
  act_to_all_in_outcast_ship_outside(t_ship, buf);

  /* Check if docking */
  if (ROOM_FLAGGED(outcast_ships[t_ship].in_room, DOCKABLE)) {
    act_to_all_in_outcast_ship(t_ship, "Your ship docks here.");
    if (world[outcast_ships[t_ship].in_room].people) {
      sprintf(buf, "%s docks here.", outcast_ships[t_ship].obj->name);
      CAP(buf);
      act(buf, FALSE, world[outcast_ships[t_ship].in_room].people, 0, 0, TO_ROOM);
      act(buf, FALSE, world[outcast_ships[t_ship].in_room].people, 0, 0, TO_CHAR);
    }
  }

  return TRUE;
}

/**
 * Ship special procedure - handles entering ships
 * @param obj Ship object
 * @param ch Character attempting to interact
 * @param cmd Command number
 * @param arg Command arguments
 * @return TRUE if handled, FALSE otherwise
 */
int outcast_ship_proc(struct obj_data *obj, struct char_data *ch, int cmd, char *arg) {
  char name[MAX_INPUT_LENGTH];
  struct obj_data *obj_entered;
  int t_ship;

  if (cmd != CMD_ENTER)
    return FALSE;

  one_argument(arg, name);
  obj_entered = get_obj_in_list_vis(ch, name, world[IN_ROOM(ch)].contents);
  if (obj_entered != obj)
    return FALSE;

  if (!obj || (GET_OBJ_TYPE(obj) != ITEM_SHIP)) {
    return FALSE;
  } else if ((t_ship = find_outcast_ship(obj)) < 0) {
    send_to_char(ch, "That ship is not operable.\r\n");
  } else if (outcast_ships[t_ship].capacity <= num_char_in_outcast_ship(t_ship)) {
    send_to_char(ch, "Too many people on the ship already!\r\n");
  } else {
    act("$n goes on $p.", TRUE, ch, outcast_ships[t_ship].obj, 0, TO_ROOM);
    char_from_room(ch);
    act("You board $p.", FALSE, ch, outcast_ships[t_ship].obj, 0, TO_CHAR);
    char_to_room(ch, outcast_ships[t_ship].entrance_room);
    act("$n comes onto the ship.", TRUE, ch, 0, 0, TO_ROOM);
  }
  return TRUE;
}

/**
 * Ship activity - called periodically to handle automated ship functions
 * Should be called every few seconds from the main game loop
 */
void outcast_ship_activity(void) {
  int i, j, k, room;
  struct char_data *ch;
  bool nav = FALSE;

  for (i = 0; i < total_num_outcast_ships; i++) {
    if (outcast_ships[i].move_timer == 0) {
      /* Check for navigation NPCs */
      for (k = 0; outcast_nav_info[k].mob != 0; k++) {
        room = outcast_nav_info[k].control_room;
        if (room == -1)
          continue;
        
        for (j = 0; j < outcast_ships[i].num_room; j++) {
          if (room == outcast_ships[i].room_list[j])
            break;
        }
        if (j >= outcast_ships[i].num_room)
          continue;
          
        for (ch = world[room].people; ch; ch = ch->next_in_room) {
          if (IS_NPC(ch) && (GET_MOB_RNUM(ch) == outcast_nav_info[k].mob)) {
            nav = outcast_navigation(ch, k, i);
            break;
          }
        }
        if (nav)
          break;
      }
    }

    /* Update timers */
    if (outcast_ships[i].timer > 0)
      outcast_ships[i].timer--;
    if (outcast_ships[i].move_timer > 0)
      outcast_ships[i].move_timer--;

    /* Handle autopilot */
    if (outcast_ships[i].repeat && !outcast_ships[i].move_timer && outcast_ships[i].velocity) {
      outcast_ships[i].move_timer = SHIP_MAX_SPEED - outcast_ships[i].velocity;
      if (!move_outcast_ship(i, outcast_ships[i].lastdir, NULL))
        outcast_ships[i].repeat = 0;  /* Turn off repeat */
      else
        outcast_ships[i].repeat--;
    }
  }
}

/**
 * Navigation AI for ships - handles automatic ship movement
 * @param ch Navigator character
 * @param mob Navigation data index
 * @param t_ship Ship index
 * @return TRUE if navigation action taken, FALSE otherwise
 */
int outcast_navigation(struct char_data *ch, int mob, int t_ship) {
  struct char_data *t_ch;
  int i;
  char buf[MAX_STRING_LENGTH];

  if (!is_valid_outcast_ship(t_ship))
    return FALSE;

  if (!outcast_nav_info[mob].sail) {
    /* Check if it's time to sail */
    if ((time_info.hours - outcast_nav_info[mob].sail_time) % outcast_nav_info[mob].freq == 0) {
      if (outcast_ships[t_ship].in_room == outcast_nav_info[mob].start1) {
        /* Announce departure */
        for (i = 0; i < outcast_ships[t_ship].num_room; i++) {
          if (outcast_ships[t_ship].room_list[i] != NOWHERE &&
              (t_ch = world[outcast_ships[t_ship].room_list[i]].people)) {
            act("Someone shouts, 'Alright, we are ready to sail!'",
                FALSE, t_ch, 0, 0, TO_ROOM);
            act("Someone shouts, 'Alright, we are ready to sail!'",
                FALSE, t_ch, 0, 0, TO_CHAR);
          }
        }
        outcast_nav_info[mob].path = outcast_nav_info[mob].path1;
        outcast_nav_info[mob].sail = TRUE;
        outcast_nav_info[mob].destination = outcast_nav_info[mob].destination1;
      } else if ((outcast_ships[t_ship].in_room == outcast_nav_info[mob].destination1) &&
                 outcast_nav_info[mob].path2) {
        /* Return journey */
        for (i = 0; i < outcast_ships[t_ship].num_room; i++) {
          if (outcast_ships[t_ship].room_list[i] != NOWHERE &&
              (t_ch = world[outcast_ships[t_ship].room_list[i]].people)) {
            act("Someone shouts, 'Alright, we are ready to sail!'",
                FALSE, t_ch, 0, 0, TO_ROOM);
            act("Someone shouts, 'Alright, we are ready to sail!'",
                FALSE, t_ch, 0, 0, TO_CHAR);
          }
        }
        outcast_nav_info[mob].path = outcast_nav_info[mob].path2;
        outcast_nav_info[mob].sail = TRUE;
        outcast_nav_info[mob].destination = outcast_nav_info[mob].start1;
      }
    }
  }

  if (!outcast_nav_info[mob].sail)
    return FALSE;
  if (outcast_ships[t_ship].move_timer > 0)
    return FALSE;

  /* Process navigation path */
  switch (*(outcast_nav_info[mob].path)) {
    case '.':
      /* End of path */
      if (outcast_ships[t_ship].in_room == outcast_nav_info[mob].destination) {
        for (i = 0; i < outcast_ships[t_ship].num_room; i++) {
          if (outcast_ships[t_ship].room_list[i] != NOWHERE &&
              (t_ch = world[outcast_ships[t_ship].room_list[i]].people)) {
            act("Someone shouts, 'We have arrived at our destination! Prepare to disembark!'",
                FALSE, t_ch, 0, 0, TO_ROOM);
            act("Someone shouts, 'We have arrived at our destination! Prepare to disembark!'",
                FALSE, t_ch, 0, 0, TO_CHAR);
          }
        }
      } else {
        for (i = 0; i < outcast_ships[t_ship].num_room; i++) {
          if (outcast_ships[t_ship].room_list[i] != NOWHERE &&
              (t_ch = world[outcast_ships[t_ship].room_list[i]].people)) {
            act("Someone shouts, 'Help us! We are lost!'",
                FALSE, t_ch, 0, 0, TO_ROOM);
            act("Someone shouts, 'Help us! We are lost!'",
                FALSE, t_ch, 0, 0, TO_CHAR);
          }
        }
      }
      outcast_nav_info[mob].sail = FALSE;
      break;
    case '0':
      /* Move north */
      if (outcast_ships[t_ship].velocity < outcast_ships[t_ship].speed) {
        sprintf(buf, "order speed fast");
        command_interpreter(ch, buf);
      }
      sprintf(buf, "order sail north");
      command_interpreter(ch, buf);
      break;
    case '1':
      /* Move east */
      if (outcast_ships[t_ship].velocity < outcast_ships[t_ship].speed) {
        sprintf(buf, "order speed fast");
        command_interpreter(ch, buf);
      }
      sprintf(buf, "order sail east");
      command_interpreter(ch, buf);
      break;
    case '2':
      /* Move south */
      if (outcast_ships[t_ship].velocity < outcast_ships[t_ship].speed) {
        sprintf(buf, "order speed fast");
        command_interpreter(ch, buf);
      }
      sprintf(buf, "order sail south");
      command_interpreter(ch, buf);
      break;
    case '3':
      /* Move west */
      if (outcast_ships[t_ship].velocity < outcast_ships[t_ship].speed) {
        sprintf(buf, "order speed fast");
        command_interpreter(ch, buf);
      }
      sprintf(buf, "order sail west");
      command_interpreter(ch, buf);
      break;
    case '4':
      /* Move up */
      if (outcast_ships[t_ship].velocity < outcast_ships[t_ship].speed) {
        sprintf(buf, "order speed fast");
        command_interpreter(ch, buf);
      }
      sprintf(buf, "order sail up");
      command_interpreter(ch, buf);
      break;
    case '5':
      /* Move down */
      if (outcast_ships[t_ship].velocity < outcast_ships[t_ship].speed) {
        sprintf(buf, "order speed fast");
        command_interpreter(ch, buf);
      }
      sprintf(buf, "order sail down");
      command_interpreter(ch, buf);
      break;
    default:
      log("SHIP: Navigation error - invalid path character");
  }
  
  outcast_nav_info[mob].path++;
  return TRUE;
}

/**
 * Control panel special procedure - handles ship control commands
 * @param obj Control panel object
 * @param ch Character using the controls
 * @param cmd Command number
 * @param argument Command arguments
 * @return TRUE if handled, FALSE otherwise
 */
int outcast_control_panel(struct obj_data *obj, struct char_data *ch, int cmd, char *argument) {
  int t_ship, vict_ship, key, num;
  int i, old_room;
  char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
  char buf[MAX_STRING_LENGTH];
  struct obj_data *target_obj;

  /* Handle look at panel command */
  if (cmd == CMD_LOOK) {
    argument = one_argument(argument, arg1);
    
    if (!*arg1)
      return FALSE;
      
    if (!isname(arg1, "panel instrument"))
      return FALSE;
      
    if ((t_ship = in_which_outcast_ship(ch)) < 0) {
      return FALSE;
    }
    
    if (!is_valid_outcast_ship(t_ship)) {
      send_to_char(ch, "Strange... this ship is not operable!\r\n");
      return TRUE;
    }
    
    sprintf(buf, "You look at %s's instrument panels and see...\r\n", outcast_ships[t_ship].obj->name);
    send_to_char(ch, buf);
    sprintf(buf, "  hull: (%d/%d), speed: (%d/%d), velocity: %d\r\n",
            GET_OBJ_VAL(outcast_ships[t_ship].obj, 0), outcast_ships[t_ship].hull, 
            GET_OBJ_VAL(outcast_ships[t_ship].obj, 1), outcast_ships[t_ship].speed, 
            outcast_ships[t_ship].velocity);
    send_to_char(ch, buf);
    sprintf(buf, "  fire-power: %d, docked: %s, capacity: (%d/%d)\r\n", 
            outcast_ships[t_ship].damage,
            is_outcast_ship_docked(t_ship) ? ((outcast_ships[t_ship].dock_vehicle != -1) ? "Ship" : "Port") : "None",
            num_char_in_outcast_ship(t_ship), outcast_ships[t_ship].capacity);
    send_to_char(ch, buf);
    
    if (GET_LEVEL(ch) > LVL_IMMORT) {
      sprintf(buf, "  size: %d, number of rooms: %d, repeat: %d\r\n", 
              outcast_ships[t_ship].size, outcast_ships[t_ship].num_room, outcast_ships[t_ship].repeat);
      send_to_char(ch, buf);
      sprintf(buf, "  timer: %d, move-timer: %d, in_room: %d\r\n", 
              outcast_ships[t_ship].timer, outcast_ships[t_ship].move_timer,
              outcast_ships[t_ship].in_room != NOWHERE ? GET_ROOM_VNUM(outcast_ships[t_ship].in_room) : -1);
      send_to_char(ch, buf);
    }
    return TRUE;
  }

  /* Handle order commands */
  if (cmd != CMD_ORDER)
    return FALSE;

  argument = one_argument(argument, arg1);

  if ((t_ship = in_which_outcast_ship(ch)) < 0) {
    return FALSE;
  }
  
  if (!is_valid_outcast_ship(t_ship)) {
    send_to_char(ch, "Strange... this ship is not operable!\r\n");
    return TRUE;
  }

  /* Parse the order command */
  if (is_abbrev(arg1, "sail")) {
    send_to_char(ch, "You shout out an order to sail!\r\n");
    act("$n shouts out an order to sail!", FALSE, ch, 0, 0, TO_ROOM);
    
    if (outcast_ships[t_ship].repeat) {
      send_to_char(ch, "Disengaging auto-mode.\r\n");
      act("$n presses a button and turns off the auto-mode.", FALSE, ch, 0, 0, TO_ROOM);
      outcast_ships[t_ship].repeat = 0;
    }
    
    /* Check for dock_vehicle */
    if (outcast_ships[t_ship].dock_vehicle != -1) {
      if (is_valid_outcast_ship(outcast_ships[t_ship].dock_vehicle) &&
          (outcast_ships[outcast_ships[t_ship].dock_vehicle].in_room == outcast_ships[t_ship].in_room)) {
        if (GET_OBJ_VAL(outcast_ships[outcast_ships[t_ship].dock_vehicle].obj, 1) >
            GET_OBJ_VAL(outcast_ships[t_ship].obj, 1)) {
          send_to_char(ch, "You can't sail a docked ship.\r\n");
          return TRUE;
        }
        act_to_all_in_outcast_ship_outside(t_ship, "Your ship sails away from the other ship!");
        act_to_all_in_outcast_ship_outside(outcast_ships[t_ship].dock_vehicle, "The other ship sails away from your ship!");
      } else {
        if (outcast_ships[outcast_ships[t_ship].dock_vehicle].dock_vehicle == t_ship)
          outcast_ships[outcast_ships[t_ship].dock_vehicle].dock_vehicle = -1;
        outcast_ships[t_ship].dock_vehicle = -1;
      }
    }
    
    if (outcast_ships[t_ship].move_timer > 0) {
      send_to_char(ch, "The ship is slow to respond to your control.\r\n");
      return TRUE;
    }
    
    argument = one_argument(argument, arg2);
    
    /* Parse direction */
    if (is_abbrev(arg2, "north")) {
      if (move_outcast_ship(t_ship, CMD_NORTH, ch)) {
        if (!outcast_ships[t_ship].velocity)
          outcast_ships[t_ship].velocity = GET_OBJ_VAL(outcast_ships[t_ship].obj, 1) >> 2;
        outcast_ships[t_ship].lastdir = CMD_NORTH;
        outcast_ships[t_ship].move_timer = SHIP_MAX_SPEED - outcast_ships[t_ship].velocity;
        if (SECT(IN_ROOM(ch)) == SECT_INSIDE) {
          send_to_char(ch, "Your ship sails northward.\r\n");
          act("Your ship sails northward.", FALSE, ch, 0, 0, TO_ROOM);
        }
      }
    } else if (is_abbrev(arg2, "east")) {
      if (move_outcast_ship(t_ship, CMD_EAST, ch)) {
        if (!outcast_ships[t_ship].velocity)
          outcast_ships[t_ship].velocity = GET_OBJ_VAL(outcast_ships[t_ship].obj, 1) >> 2;
        outcast_ships[t_ship].lastdir = CMD_EAST;
        outcast_ships[t_ship].move_timer = SHIP_MAX_SPEED - outcast_ships[t_ship].velocity;
        if (SECT(IN_ROOM(ch)) == SECT_INSIDE) {
          send_to_char(ch, "Your ship sails eastward.\r\n");
          act("Your ship sails eastward.", FALSE, ch, 0, 0, TO_ROOM);
        }
      }
    } else if (is_abbrev(arg2, "south")) {
      if (move_outcast_ship(t_ship, CMD_SOUTH, ch)) {
        if (!outcast_ships[t_ship].velocity)
          outcast_ships[t_ship].velocity = GET_OBJ_VAL(outcast_ships[t_ship].obj, 1) >> 2;
        outcast_ships[t_ship].lastdir = CMD_SOUTH;
        outcast_ships[t_ship].move_timer = SHIP_MAX_SPEED - outcast_ships[t_ship].velocity;
        if (SECT(IN_ROOM(ch)) == SECT_INSIDE) {
          send_to_char(ch, "Your ship sails southward.\r\n");
          act("Your ship sails southward.", FALSE, ch, 0, 0, TO_ROOM);
        }
      }
    } else if (is_abbrev(arg2, "west")) {
      if (move_outcast_ship(t_ship, CMD_WEST, ch)) {
        if (!outcast_ships[t_ship].velocity)
          outcast_ships[t_ship].velocity = GET_OBJ_VAL(outcast_ships[t_ship].obj, 1) >> 2;
        outcast_ships[t_ship].lastdir = CMD_WEST;
        outcast_ships[t_ship].move_timer = SHIP_MAX_SPEED - outcast_ships[t_ship].velocity;
        if (SECT(IN_ROOM(ch)) == SECT_INSIDE) {
          send_to_char(ch, "Your ship sails westward.\r\n");
          act("Your ship sails westward.", FALSE, ch, 0, 0, TO_ROOM);
        }
      }
    } else if (is_abbrev(arg2, "up")) {
      if (move_outcast_ship(t_ship, CMD_UP, ch)) {
        if (!outcast_ships[t_ship].velocity)
          outcast_ships[t_ship].velocity = GET_OBJ_VAL(outcast_ships[t_ship].obj, 1) >> 2;
        outcast_ships[t_ship].lastdir = CMD_UP;
        outcast_ships[t_ship].move_timer = SHIP_MAX_SPEED - outcast_ships[t_ship].velocity;
        if (SECT(IN_ROOM(ch)) == SECT_INSIDE) {
          send_to_char(ch, "Your ship sails upward.\r\n");
          act("Your ship sails upward.", FALSE, ch, 0, 0, TO_ROOM);
        }
      }
    } else if (is_abbrev(arg2, "down")) {
      if (move_outcast_ship(t_ship, CMD_DOWN, ch)) {
        if (!outcast_ships[t_ship].velocity)
          outcast_ships[t_ship].velocity = GET_OBJ_VAL(outcast_ships[t_ship].obj, 1) >> 2;
        outcast_ships[t_ship].lastdir = CMD_DOWN;
        outcast_ships[t_ship].move_timer = SHIP_MAX_SPEED - outcast_ships[t_ship].velocity;
        if (SECT(IN_ROOM(ch)) == SECT_INSIDE) {
          send_to_char(ch, "Your ship sails downward.\r\n");
          act("Your ship sails downward.", FALSE, ch, 0, 0, TO_ROOM);
        }
      }
    } else {
      send_to_char(ch, "You must provide a direction.\r\n");
      outcast_ships[t_ship].lastdir = 0;
      return TRUE;
    }
    
    /* Check for repeat sailing */
    argument = one_argument(argument, arg2);
    if (isdigit(*arg2)) {
      num = MIN(50, atoi(arg2));
      if (num > 1) {
        outcast_ships[t_ship].repeat = num - 1;
        send_to_char(ch, "Engaging auto-mode...\r\n");
        act("$n presses a button and turns on the auto-mode.", FALSE, ch, 0, 0, TO_ROOM);
        if (!outcast_ships[t_ship].velocity)
          outcast_ships[t_ship].velocity = GET_OBJ_VAL(outcast_ships[t_ship].obj, 1) >> 2;
      }
    }

  } else if (is_abbrev(arg1, "board")) {
    send_to_char(ch, "You shout out an order to board another ship!\r\n");
    act("$n shouts out an order to board another ship!", FALSE, ch, 0, 0, TO_ROOM);
    
    if (outcast_ships[t_ship].repeat) {
      send_to_char(ch, "Disengaging auto-mode.\r\n");
      act("$n presses a button and turns off the auto-mode.", FALSE, ch, 0, 0, TO_ROOM);
      outcast_ships[t_ship].repeat = 0;
    }
    
    if (is_outcast_ship_docked(t_ship)) {
      send_to_char(ch, "You can't dock a ship already docked.\r\n");
      return TRUE;
    }
    
    argument = one_argument(argument, arg2);
    target_obj = get_obj_in_list_vis(ch, arg2, world[outcast_ships[t_ship].in_room].contents);
    
    if (!target_obj || (GET_OBJ_TYPE(target_obj) != ITEM_SHIP)) {
      send_to_char(ch, "What ship do you want to dock?\r\n");
    } else if ((vict_ship = find_outcast_ship(target_obj)) < 0) {
      send_to_char(ch, "What ship do you want to dock?\r\n");
    } else {
      /* Can dock on another ship only if your speed is at least same */
      if ((GET_OBJ_VAL(outcast_ships[t_ship].obj, 1) >= GET_OBJ_VAL(outcast_ships[vict_ship].obj, 1)) &&
          !is_outcast_ship_docked(vict_ship) && (t_ship != vict_ship)) {
        outcast_ships[t_ship].timer = SHIP_MAX_SPEED - GET_OBJ_VAL(outcast_ships[t_ship].obj, 1);
        outcast_ships[t_ship].lastdir = 0;
        outcast_ships[t_ship].velocity = 0;
        outcast_ships[t_ship].dock_vehicle = vict_ship;
        outcast_ships[vict_ship].dock_vehicle = t_ship;
        act_to_all_in_outcast_ship(t_ship, "You hear the sound of two ships docked together.");
        act_to_all_in_outcast_ship(vict_ship, "You hear the sound of two ships docked together.");
      } else {
        send_to_char(ch, "You can't seem to dock on the other ship.\r\n");
      }
    }

  } else if (is_abbrev(arg1, "fire")) {
    send_to_char(ch, "You shout out an order to fire another ship!\r\n");
    act("$n shouts out an order to fire another ship!", FALSE, ch, 0, 0, TO_ROOM);
    
    if (is_outcast_ship_docked(t_ship)) {
      send_to_char(ch, "You can't fire cannon on a docked ship.\r\n");
      return TRUE;
    }
    
    if (outcast_ships[t_ship].timer > 0) {
      send_to_char(ch, "The ship is slow to respond to your control.\r\n");
      return TRUE;
    }
    
    if (outcast_ships[t_ship].damage <= 0) {
      send_to_char(ch, "Your ship cannot fire.\r\n");
      return TRUE;
    }
    
    argument = one_argument(argument, arg2);
    old_room = IN_ROOM(ch);
    char_from_room(ch);
    char_to_room(ch, outcast_ships[t_ship].in_room);
    target_obj = get_obj_in_list_vis(ch, arg2, world[outcast_ships[t_ship].in_room].contents);
    char_from_room(ch);
    char_to_room(ch, old_room);
    
    if (!target_obj || (GET_OBJ_TYPE(target_obj) != ITEM_SHIP)) {
      send_to_char(ch, "There's no ship to fire at.\r\n");
    } else if ((vict_ship = find_outcast_ship(target_obj)) < 0) {
      send_to_char(ch, "There's no ship to fire at.\r\n");
    } else {
      /* Damage the target ship */
      GET_OBJ_VAL(outcast_ships[vict_ship].obj, 0) -= outcast_ships[t_ship].damage;
      
      sprintf(buf, "Your ship fires at %s.", outcast_ships[vict_ship].obj->short_description);
      act_to_all_in_outcast_ship_outside(t_ship, buf);
      act_to_all_in_outcast_ship(t_ship, "You hear a deafening boom!");
      
      sprintf(buf, "%s fires at your ship.", outcast_ships[t_ship].obj->short_description);
      CAP(buf);
      act_to_all_in_outcast_ship_outside(vict_ship, buf);
      act_to_all_in_outcast_ship(vict_ship, "The whole ship begins to shake and creak!");
      
      /* Display battle message to ocean room */
      sprintf(buf, "%s fires at %s.", outcast_ships[t_ship].obj->short_description,
              outcast_ships[vict_ship].obj->short_description);
      CAP(buf);
      if (world[outcast_ships[t_ship].in_room].people) {
        act(buf, FALSE, world[outcast_ships[t_ship].in_room].people, 0, 0, TO_ROOM);
        act(buf, FALSE, world[outcast_ships[t_ship].in_room].people, 0, 0, TO_CHAR);
      }
      
      /* Display battle message to all other ships in same room */
      for (i = 0; i < total_num_outcast_ships; i++) {
        if ((i != vict_ship) && (i != t_ship) && is_valid_outcast_ship(i) &&
            (outcast_ships[i].in_room == outcast_ships[t_ship].in_room)) {
          act_to_all_in_outcast_ship_outside(i, buf);
        }
      }
      
      outcast_ships[t_ship].timer = SHIP_MAX_SPEED - GET_OBJ_VAL(outcast_ships[t_ship].obj, 1);
      
      /* Check if target ship is destroyed */
      if (GET_OBJ_VAL(outcast_ships[vict_ship].obj, 0) <= 0) {
        sink_outcast_ship(vict_ship);
        extract_obj(target_obj);
      }
    }

  } else if (is_abbrev(arg1, "ram")) {
    send_to_char(ch, "You shout out an order to ram another ship!\r\n");
    act("$n shouts out an order to ram another ship!", FALSE, ch, 0, 0, TO_ROOM);
    
    if (is_outcast_ship_docked(t_ship)) {
      send_to_char(ch, "You can't perform a ramming on a docked ship.\r\n");
      return TRUE;
    }
    
    if (outcast_ships[t_ship].timer > 0) {
      send_to_char(ch, "The ship is slow to respond to your control.\r\n");
      return TRUE;
    }
    
    argument = one_argument(argument, arg2);
    old_room = IN_ROOM(ch);
    char_from_room(ch);
    char_to_room(ch, outcast_ships[t_ship].in_room);
    target_obj = get_obj_in_list_vis(ch, arg2, world[outcast_ships[t_ship].in_room].contents);
    char_from_room(ch);
    char_to_room(ch, old_room);
    
    if (!target_obj || (GET_OBJ_TYPE(target_obj) != ITEM_SHIP)) {
      send_to_char(ch, "What ship do you want to ram?\r\n");
    } else if ((vict_ship = find_outcast_ship(target_obj)) < 0) {
      send_to_char(ch, "What ship do you want to ram?\r\n");
    } else if (vict_ship == t_ship) {
      send_to_char(ch, "You can't ram your own ship!\r\n");
    } else {
      /* Mutual damage from ramming */
      GET_OBJ_VAL(outcast_ships[vict_ship].obj, 0) -= outcast_ships[t_ship].size;
      GET_OBJ_VAL(outcast_ships[t_ship].obj, 0) -= outcast_ships[vict_ship].size;
      
      sprintf(buf, "Your ship rams into %s.", outcast_ships[vict_ship].obj->short_description);
      act_to_all_in_outcast_ship_outside(t_ship, buf);
      
      sprintf(buf, "%s rams into your ship.", outcast_ships[t_ship].obj->short_description);
      CAP(buf);
      act_to_all_in_outcast_ship_outside(vict_ship, buf);
      
      act_to_all_in_outcast_ship(t_ship, "The whole ship begins to shake and creak!");
      act_to_all_in_outcast_ship(vict_ship, "The whole ship begins to shake and creak!");
      
      /* Display battle message to ocean room */
      sprintf(buf, "%s rams into %s.", outcast_ships[t_ship].obj->short_description,
              outcast_ships[vict_ship].obj->short_description);
      CAP(buf);
      if (world[outcast_ships[t_ship].in_room].people) {
        act(buf, FALSE, world[outcast_ships[t_ship].in_room].people, 0, 0, TO_ROOM);
        act(buf, FALSE, world[outcast_ships[t_ship].in_room].people, 0, 0, TO_CHAR);
      }
      
      /* Display battle message to all other ships in same room */
      for (i = 0; i < total_num_outcast_ships; i++) {
        if ((i != vict_ship) && (i != t_ship) && is_valid_outcast_ship(i) &&
            (outcast_ships[i].in_room == outcast_ships[t_ship].in_room)) {
          act_to_all_in_outcast_ship_outside(i, buf);
        }
      }
      
      outcast_ships[t_ship].timer = SHIP_MAX_SPEED - GET_OBJ_VAL(outcast_ships[t_ship].obj, 1);
      
      /* Check if target ship is destroyed */
      if (GET_OBJ_VAL(outcast_ships[vict_ship].obj, 0) <= 0) {
        sink_outcast_ship(vict_ship);
        extract_obj(target_obj);
      }
      
      /* Check if our ship is destroyed too */
      if (GET_OBJ_VAL(outcast_ships[t_ship].obj, 0) <= 0) {
        sink_outcast_ship(t_ship);
        /* Note: Can't extract our own ship object here as we're still using it */
      }
    }

  } else if (is_abbrev(arg1, "speed")) {
    send_to_char(ch, "You shout out an order to change speed!\r\n");
    act("$n shouts out an order to change speed!", FALSE, ch, 0, 0, TO_ROOM);
    
    if (is_outcast_ship_docked(t_ship)) {
      send_to_char(ch, "You can't control the speed on the docked ship.\r\n");
      return TRUE;
    }
    
    argument = one_argument(argument, arg2);
    
    if (is_abbrev(arg2, "fast")) {
      outcast_ships[t_ship].velocity = GET_OBJ_VAL(outcast_ships[t_ship].obj, 1);
      act("The ship sails at maximum speed.", FALSE, ch, 0, 0, TO_ROOM);
      act("The ship sails at maximum speed.", FALSE, ch, 0, 0, TO_CHAR);
    } else if (is_abbrev(arg2, "medium")) {
      outcast_ships[t_ship].velocity = GET_OBJ_VAL(outcast_ships[t_ship].obj, 1) >> 1;
      act("The ship sails at medium speed.", FALSE, ch, 0, 0, TO_ROOM);
      act("The ship sails at medium speed.", FALSE, ch, 0, 0, TO_CHAR);
    } else if (is_abbrev(arg2, "slow")) {
      outcast_ships[t_ship].velocity = GET_OBJ_VAL(outcast_ships[t_ship].obj, 1) >> 2;
      act("The ship sails at minimum speed.", FALSE, ch, 0, 0, TO_ROOM);
      act("The ship sails at minimum speed.", FALSE, ch, 0, 0, TO_CHAR);
    } else if (is_abbrev(arg2, "stop")) {
      outcast_ships[t_ship].velocity = 0;
      act("The ship stops.", FALSE, ch, 0, 0, TO_ROOM);
      act("The ship stops.", FALSE, ch, 0, 0, TO_CHAR);
    } else {
      send_to_char(ch, "Speed must be fast, medium, slow, or stop.\r\n");
    }
  } else {
    return FALSE;
  }
  
  return TRUE;
}

/**
 * Ship exit room procedure - handles disembarking from ships
 * @param room Room number
 * @param ch Character attempting to disembark
 * @param cmd Command number
 * @param arg Command arguments
 * @return TRUE if handled, FALSE otherwise
 */
int outcast_ship_exit_room(int room, struct char_data *ch, int cmd, char *arg) {
  int t_ship, to_ship;

  if ((cmd != CMD_LOOK) && (cmd != CMD_DISEMBARK))
    return FALSE;

  if (((t_ship = in_which_outcast_ship(ch)) < 0) && (cmd == CMD_DISEMBARK)) {
    send_to_char(ch, "You can't disembark unless you are in a ship.\r\n");
    return FALSE;
  }
  
  if (!is_valid_outcast_ship(t_ship)) {
    send_to_char(ch, "This ship is not operable!\r\n");
    return FALSE;
  }
  
  if (cmd == CMD_LOOK) {
    if (!arg || !*arg || strcmp(arg, "out"))
      return FALSE;
      
    look_at_room(ch, outcast_ships[t_ship].in_room);
    return TRUE;
  }
  
  if (!is_outcast_ship_docked(t_ship) && GET_LEVEL(ch) < LVL_IMMORT) {
    send_to_char(ch, "You better not leave the ship until it's docked.\r\n");
    return TRUE;
  }
  
  if (GET_POS(ch) < POS_STANDING) {
    send_to_char(ch, "You're in no position to disembark!\r\n");
    return TRUE;
  }
  
  /* Board another ship */
  if ((to_ship = outcast_ships[t_ship].dock_vehicle) != -1) {
    if (!is_valid_outcast_ship(to_ship)) {
      send_to_char(ch, "Strange... that is a ghost ship!\r\n");
    } else {
      act("You leave this ship and board $p.",
          FALSE, ch, outcast_ships[to_ship].obj, 0, TO_CHAR);
      act("$n leaves this ship and boards $p.",
          TRUE, ch, outcast_ships[to_ship].obj, 0, TO_ROOM);
      char_from_room(ch);
      char_to_room(ch, outcast_ships[to_ship].entrance_room);
      act("$n leaves $p and boards this ship.",
          TRUE, ch, outcast_ships[t_ship].obj, 0, TO_ROOM);
    }
  } else {
    /* Go on land */
    act("You disembark this ship.", FALSE, ch, 0, 0, TO_CHAR);
    act("$n disembarks this ship.", TRUE, ch, 0, 0, TO_ROOM);
    char_from_room(ch);
    char_to_room(ch, outcast_ships[t_ship].in_room);
    act("$n disembarks from $p.", TRUE, ch, outcast_ships[t_ship].obj, 0, TO_ROOM);
  }
  
  return TRUE;
}

/**
 * Ship look out room procedure - handles looking outside from ship
 * @param room Room number
 * @param ch Character looking out
 * @param cmd Command number
 * @param arg Command arguments
 * @return TRUE if handled, FALSE otherwise
 */
int outcast_ship_look_out_room(int room, struct char_data *ch, int cmd, char *arg) {
  int t_ship;

  if (cmd != CMD_LOOK)
    return FALSE;

  if (!arg || !*arg || strcmp(arg, "out"))
    return FALSE;

  if ((t_ship = in_which_outcast_ship(ch)) < 0) {
    send_to_char(ch, "You must be inside a ship to look out!\r\n");
    return FALSE;
  }
  
  if (!is_valid_outcast_ship(t_ship)) {
    send_to_char(ch, "This ship is not operable!\r\n");
    return FALSE;
  }

  /* Show the external room */
  look_at_room(ch, outcast_ships[t_ship].in_room);
  return TRUE;
}

/* ========================================================================= */
/* GREYHAWK SHIP SYSTEM IMPLEMENTATION (READY TO USE)                      */
/* ========================================================================= */
/* Integrated from Greyhawk MUD - advanced naval combat and navigation     */

/* Global variables for Greyhawk ship system */
struct greyhawk_ship_data greyhawk_ships[GREYHAWK_MAXSHIPS];
struct greyhawk_contact_data greyhawk_contacts[30];
struct greyhawk_ship_map greyhawk_tactical[151][151];

/* Global string buffers for Greyhawk system */
static char greyhawk_status[20];
static char greyhawk_position[20];
static char greyhawk_weapon[100];
static char greyhawk_contact[256];
static char greyhawk_arc[3];
static char greyhawk_debug[256];
static char greyhawk_arg1[80];
static char greyhawk_arg2[80];

/* ========================================================================= */
/* GREYHAWK SHIP UTILITY FUNCTIONS                                         */
/* ========================================================================= */

/**
 * Get weapon status string for display
 * @param slot Weapon slot number
 * @param rnum Room number containing ship
 */
void greyhawk_getstatus(int slot, int rnum) {
  if (world[rnum].ship->slot[slot].timer > 0)
    sprintf(greyhawk_status, "&+R%-6d", world[rnum].ship->slot[slot].timer);
  else if (world[rnum].ship->slot[slot].timer == 0)
    strcpy(greyhawk_status, "Ready");
  else if (world[rnum].ship->slot[slot].timer < 0)
    strcpy(greyhawk_status, "&+L***   ");
  
  if (world[rnum].ship->slot[slot].desc == NULL)
    strcpy(greyhawk_status, "");
}

/**
 * Get weapon position string for display
 * @param slot Weapon slot number
 * @param rnum Room number containing ship
 */
void greyhawk_getposition(int slot, int rnum) {
  switch (world[rnum].ship->slot[slot].position) {
    case GREYHAWK_FORE:
      strcpy(greyhawk_position, "Forward");
      break;
    case GREYHAWK_REAR:
      strcpy(greyhawk_position, "Rear");
      break;
    case GREYHAWK_PORT:
      strcpy(greyhawk_position, "Port");
      break;
    case GREYHAWK_STARBOARD:
      strcpy(greyhawk_position, "Starboard");
      break;
    default:
      strcpy(greyhawk_position, "ERROR");
      break;
  }
  
  if (world[rnum].ship->slot[slot].desc == NULL)
    strcpy(greyhawk_position, "");
}

/**
 * Format weapon display string
 * @param slot Weapon slot number
 * @param rnum Room number containing ship
 */
void greyhawk_dispweapon(int slot, int rnum) {
  if (world[rnum].ship->slot[slot].type != 1) {
    strcpy(greyhawk_weapon, " ");
  } else {
    greyhawk_getstatus(slot, rnum);
    greyhawk_getposition(slot, rnum);
    sprintf(greyhawk_weapon, "%-20s &N%-6s  &+W%-9s  %d",
            world[rnum].ship->slot[slot].desc,
            greyhawk_status,
            greyhawk_position,
            world[rnum].ship->slot[slot].val3);
  }
}

/**
 * Calculate weapon range based on type
 * @param shipnum Ship index
 * @param slot Weapon slot
 * @param range Range type (SHORT/MED/LONG)
 * @return Calculated range value
 */
int greyhawk_weaprange(int shipnum, int slot, char range) {
  if (greyhawk_ships[shipnum].slot[slot].type != 1)
    return 0;
    
  switch (range) {
    case GREYHAWK_SHRTRANGE:
      return (int)((float)(greyhawk_ships[shipnum].slot[slot].val0 -
                          greyhawk_ships[shipnum].slot[slot].val1) / 3 +
                  greyhawk_ships[shipnum].slot[slot].val1);
    case GREYHAWK_MEDRANGE:
      return (int)((float)((greyhawk_ships[shipnum].slot[slot].val0 -
                           greyhawk_ships[shipnum].slot[slot].val1) / 3) * 2 +
                  greyhawk_ships[shipnum].slot[slot].val1);
    case GREYHAWK_LNGRANGE:
      return greyhawk_ships[shipnum].slot[slot].val0;
    default:
      return 0;
  }
}

/**
 * Calculate bearing between two points
 * @param x1 Source X coordinate
 * @param y1 Source Y coordinate
 * @param x2 Target X coordinate
 * @param y2 Target Y coordinate
 * @return Bearing in degrees (0-360)
 */
int greyhawk_bearing(float x1, float y1, float x2, float y2) {
  int val;
  
  if (y1 == y2) {
    if (x1 > x2)
      return 270;
    return 90;
  }
  
  if (x1 == x2) {
    if (y1 > y2)
      return 180;
    else
      return 0;
  }
  
  val = atan((x2 - x1) / (y2 - y1)) * 180 / M_PI;
  
  if (y1 < y2) {
    if (val >= 0)
      return val;
    return (val + 360);
  } else {
    return val + 180;
  }
}

/**
 * Calculate 3D range between two points
 * @param x1 Source X coordinate
 * @param y1 Source Y coordinate
 * @param z1 Source Z coordinate
 * @param x2 Target X coordinate
 * @param y2 Target Y coordinate
 * @param z2 Target Z coordinate
 * @return 3D distance
 */
float greyhawk_range(float x1, float y1, float z1, float x2, float y2, float z2) {
  float dx = x2 - x1;
  float dy = y2 - y1;
  float dz = z2 - z1;
  
  return sqrt((dx * dx) + (dy * dy) + (dz * dz));
}

/**
 * Display contact information
 * @param i Contact index
 */
void greyhawk_dispcontact(int i) {
  int x, y, z, bearing, j;
  float range;
  
  x = greyhawk_contacts[i].x;
  y = greyhawk_contacts[i].y;
  z = greyhawk_contacts[i].z;
  range = greyhawk_contacts[i].range;
  bearing = greyhawk_contacts[i].bearing;
  j = greyhawk_contacts[i].shipnum;
  
  sprintf(greyhawk_contact, "[%s] %-30s X:%-3d Y:%-3d Z:%-3d R:%-5.1f B:%-3d H:%-3d S:%-3d|%s\r\n",
          greyhawk_ships[j].id, greyhawk_ships[j].name, x, y, z, range, bearing,
          greyhawk_ships[j].heading, greyhawk_ships[j].speed, greyhawk_contacts[i].arc);
}

/**
 * Get contacts for a ship (radar/sensor function)
 * @param shipnum Ship doing the scanning
 * @return Number of contacts found
 */
int greyhawk_getcontacts(int shipnum) {
  int rroom, i, j, to_room;
  struct obj_data *obj, *obj_next;
  
  rroom = real_room(greyhawk_ships[shipnum].location);
  i = 0;
  
  /* Check objects in current room */
  for (obj = world[rroom].contents; obj; obj = obj_next) {
    obj_next = obj->next_content;
    if (GET_OBJ_TYPE(obj) == GREYHAWK_ITEM_SHIP) {
      if (GET_OBJ_VAL(obj, 1) != shipnum) {
        if (greyhawk_range(greyhawk_ships[shipnum].x, greyhawk_ships[shipnum].y,
                          greyhawk_ships[shipnum].z,
                          greyhawk_ships[GET_OBJ_VAL(obj, 1)].x,
                          greyhawk_ships[GET_OBJ_VAL(obj, 1)].y,
                          greyhawk_ships[GET_OBJ_VAL(obj, 1)].z) <= 35) {
          greyhawk_setcontact(i, obj, shipnum, 0, 0);
          i++;
        }
      }
    }
  }
  
  /* Check adjacent rooms */
  for (j = 0; j < NUM_OF_DIRS; j++) {
    if (world[rroom].dir_option[j]) {
      if (world[rroom].dir_option[j]->to_room != NOWHERE) {
        to_room = world[rroom].dir_option[j]->to_room;
        for (obj = world[to_room].contents; obj; obj = obj_next) {
          obj_next = obj->next_content;
          if (GET_OBJ_TYPE(obj) == GREYHAWK_ITEM_SHIP) {
            if (GET_OBJ_VAL(obj, 1) != shipnum) {
              /* Calculate position offset based on direction */
              int xoffset = 0, yoffset = 0;
              switch (j) {
                case NORTH:     yoffset = 50; break;
                case NORTHEAST: xoffset = 50; yoffset = 50; break;
                case EAST:      xoffset = 50; break;
                case SOUTHEAST: xoffset = 50; yoffset = -50; break;
                case SOUTH:     yoffset = -50; break;
                case SOUTHWEST: xoffset = -50; yoffset = -50; break;
                case WEST:      xoffset = -50; break;
                case NORTHWEST: xoffset = -50; yoffset = 50; break;
              }
              
              if (greyhawk_range(greyhawk_ships[shipnum].x, greyhawk_ships[shipnum].y,
                                greyhawk_ships[shipnum].z,
                                greyhawk_ships[GET_OBJ_VAL(obj, 1)].x + xoffset,
                                greyhawk_ships[GET_OBJ_VAL(obj, 1)].y + yoffset,
                                greyhawk_ships[GET_OBJ_VAL(obj, 1)].z) <= 35) {
                greyhawk_setcontact(i, obj, shipnum, xoffset, yoffset);
                i++;
              }
            }
          }
        }
      }
    }
  }
  
  return i;
}

/**
 * Set contact data for display
 * @param i Contact index
 * @param obj Ship object being tracked
 * @param shipnum Our ship number
 * @param xoffset X coordinate offset
 * @param yoffset Y coordinate offset
 */
void greyhawk_setcontact(int i, struct obj_data *obj, int shipnum, int xoffset, int yoffset) {
  greyhawk_contacts[i].bearing = greyhawk_bearing(greyhawk_ships[shipnum].x,
                                                 greyhawk_ships[shipnum].y,
                                                 (greyhawk_ships[GET_OBJ_VAL(obj, 1)].x + (float)xoffset),
                                                 (greyhawk_ships[GET_OBJ_VAL(obj, 1)].y + (float)yoffset));
  
  greyhawk_contacts[i].range = greyhawk_range(greyhawk_ships[shipnum].x,
                                             greyhawk_ships[shipnum].y,
                                             greyhawk_ships[shipnum].z,
                                             greyhawk_ships[GET_OBJ_VAL(obj, 1)].x + xoffset,
                                             greyhawk_ships[GET_OBJ_VAL(obj, 1)].y + yoffset,
                                             greyhawk_ships[GET_OBJ_VAL(obj, 1)].z);
  
  greyhawk_contacts[i].x = (int)greyhawk_ships[GET_OBJ_VAL(obj, 1)].x + xoffset;
  greyhawk_contacts[i].y = (int)greyhawk_ships[GET_OBJ_VAL(obj, 1)].y + yoffset;
  greyhawk_contacts[i].z = (int)greyhawk_ships[GET_OBJ_VAL(obj, 1)].z;
  greyhawk_contacts[i].shipnum = GET_OBJ_VAL(obj, 1);
  
  greyhawk_getarc(shipnum, GET_OBJ_VAL(obj, 1));
  strcpy(greyhawk_contacts[i].arc, greyhawk_arc);
}

/**
 * Determine firing arc between two ships
 * @param ship1 Source ship
 * @param ship2 Target ship
 * @return Arc constant (FORE/PORT/REAR/STARBOARD)
 */
int greyhawk_getarc(int ship1, int ship2) {
  float x1, y1, x2, y2;
  int shipbearing, shipheading, rroom, to_room, i;
  
  rroom = real_room(greyhawk_ships[ship1].location);
  to_room = real_room(greyhawk_ships[ship2].location);
  
  x1 = greyhawk_ships[ship1].x;
  y1 = greyhawk_ships[ship1].y;
  shipheading = greyhawk_ships[ship1].heading;
  
  x2 = greyhawk_ships[ship2].x;
  y2 = greyhawk_ships[ship2].y;
  
  /* Adjust coordinates if ships are in adjacent rooms */
  for (i = 0; i < NUM_OF_DIRS; i++) {
    if (world[rroom].dir_option[i]) {
      if (world[rroom].dir_option[i]->to_room == to_room) {
        switch (i) {
          case NORTH:     y2 += 50; break;
          case NORTHEAST: x2 += 50; y2 += 50; break;
          case EAST:      x2 += 50; break;
          case SOUTHEAST: x2 += 50; y2 -= 50; break;
          case SOUTH:     y2 -= 50; break;
          case SOUTHWEST: x2 -= 50; y2 -= 50; break;
          case WEST:      x2 -= 50; break;
          case NORTHWEST: x2 -= 50; y2 += 50; break;
          default:        return FALSE;
        }
      }
    }
  }
  
  shipbearing = greyhawk_bearing(x1, y1, x2, y2);
  if (shipheading < shipbearing)
    shipheading += 360;
  
  strcpy(greyhawk_arc, "*");
  
  if ((shipbearing > (shipheading - 140)) && (shipbearing < (shipheading - 40))) {
    strcpy(greyhawk_arc, "P");
    return GREYHAWK_PORT;
  }
  if ((shipbearing > (shipheading - 220)) && (shipbearing < (shipheading - 140))) {
    strcpy(greyhawk_arc, "R");
    return GREYHAWK_REAR;
  }
  if ((shipbearing > (shipheading - 320)) && (shipbearing < (shipheading - 220))) {
    strcpy(greyhawk_arc, "S");
    return GREYHAWK_STARBOARD;
  }
  if (((shipbearing > (shipheading - 40)) && (shipbearing < (shipheading + 40))) ||
      (shipbearing < (shipheading - 320)) || (shipbearing == shipheading)) {
    strcpy(greyhawk_arc, "F");
    return GREYHAWK_FORE;
  }
  
  return GREYHAWK_FORE;
}

/* ========================================================================= */
/* GREYHAWK TACTICAL MAP FUNCTIONS                                         */
/* ========================================================================= */

/**
 * Set tactical map symbol at coordinates
 * @param x X coordinate
 * @param y Y coordinate  
 * @param symbol Map symbol identifier
 */
void greyhawk_setsymbol(int x, int y, int symbol) {
  switch (symbol) {
    case 11:  strcpy(greyhawk_tactical[x][y].map, "&+g\"\"&N"); break;
    case 12:  strcpy(greyhawk_tactical[x][y].map, "&+g**&N"); break;
    case 13:  strcpy(greyhawk_tactical[x][y].map, "&+y^^&N"); break;
    case 15:  strcpy(greyhawk_tactical[x][y].map, "&+L++&N"); break;
    case 17:  strcpy(greyhawk_tactical[x][y].map, "&+b~~&N"); break;
    case 18:  strcpy(greyhawk_tactical[x][y].map, "&+y^^&N"); break;
    case 19:  strcpy(greyhawk_tactical[x][y].map, "&+L%%&N"); break;
    case 20:  strcpy(greyhawk_tactical[x][y].map, "&+g**&N"); break;
    case 21:  strcpy(greyhawk_tactical[x][y].map, "&+g..&N"); break;
    case 24:  strcpy(greyhawk_tactical[x][y].map, "&+b~~&N"); break;
    case 35:  strcpy(greyhawk_tactical[x][y].map, "&+Y..&N"); break;
    case 36:  strcpy(greyhawk_tactical[x][y].map, "&+Y**&N"); break;
    case 37:  strcpy(greyhawk_tactical[x][y].map, "&+W..&N"); break;
    case 38:  strcpy(greyhawk_tactical[x][y].map, "&+W^^&N"); break;
    case 50:  strcpy(greyhawk_tactical[x][y].map, "&+R%%&N"); break;
    default:  strcpy(greyhawk_tactical[x][y].map, "&+y..&N"); break;
  }
}

/**
 * Generate tactical map for ship
 * @param shipnum Ship to generate map for
 */
void greyhawk_getmap(int shipnum) {
  int x, y, i, rroom, to_room = 0, k;
  
  /* Clear tactical map */
  for (x = 0; x < 150; x++) {
    for (y = 0; y < 150; y++) {
      strcpy(greyhawk_tactical[x][y].map, "  ");
    }
  }
  
  rroom = real_room(greyhawk_ships[shipnum].location);
  
  /* Load current room map data */
  for (y = 50; y < 100; y++) {
    for (x = 50; x < 100; x++) {
      if (world[rroom].map) {
        greyhawk_setsymbol(x, y, world[rroom].map->map_grid[y - 50][x - 50]);
      }
    }
  }
  
  /* Load adjacent room map data */
  for (i = 0; i < NUM_OF_DIRS; i++) {
    if (world[rroom].dir_option[i]) {
      if (world[rroom].dir_option[i]->to_room != NOWHERE) {
        to_room = world[rroom].dir_option[i]->to_room;
        
        switch (i) {
          case NORTH:
            for (y = 0; y < 50; y++) {
              for (x = 50; x < 100; x++) {
                if (world[to_room].map) {
                  greyhawk_setsymbol(x, y, world[to_room].map->map_grid[y][x - 50]);
                }
              }
            }
            break;
          case NORTHEAST:
            for (y = 0; y < 50; y++) {
              for (x = 100; x < 150; x++) {
                if (world[to_room].map) {
                  greyhawk_setsymbol(x, y, world[to_room].map->map_grid[y][x - 100]);
                }
              }
            }
            break;
          case EAST:
            for (y = 50; y < 100; y++) {
              for (x = 100; x < 150; x++) {
                if (world[to_room].map) {
                  greyhawk_setsymbol(x, y, world[to_room].map->map_grid[y - 50][x - 100]);
                }
              }
            }
            break;
          case SOUTHEAST:
            for (y = 100; y < 150; y++) {
              for (x = 100; x < 150; x++) {
                if (world[to_room].map) {
                  greyhawk_setsymbol(x, y, world[to_room].map->map_grid[y - 100][x - 100]);
                }
              }
            }
            break;
          case SOUTH:
            for (y = 100; y < 150; y++) {
              for (x = 50; x < 100; x++) {
                if (world[to_room].map) {
                  greyhawk_setsymbol(x, y, world[to_room].map->map_grid[y - 100][x - 50]);
                }
              }
            }
            break;
          case SOUTHWEST:
            for (y = 100; y < 150; y++) {
              for (x = 0; x < 50; x++) {
                if (world[to_room].map) {
                  greyhawk_setsymbol(x, y, world[to_room].map->map_grid[y - 100][x]);
                }
              }
            }
            break;
          case WEST:
            for (y = 50; y < 100; y++) {
              for (x = 0; x < 50; x++) {
                if (world[to_room].map) {
                  greyhawk_setsymbol(x, y, world[to_room].map->map_grid[y - 50][x]);
                }
              }
            }
            break;
          case NORTHWEST:
            for (y = 0; y < 50; y++) {
              for (x = 0; x < 50; x++) {
                if (world[to_room].map) {
                  greyhawk_setsymbol(x, y, world[to_room].map->map_grid[y][x]);
                }
              }
            }
            break;
        }
      }
    }
  }
  
  /* Add ship contacts to map */
  k = greyhawk_getcontacts(shipnum);
  for (i = 0; i < k; i++) {
    sprintf(greyhawk_tactical[greyhawk_contacts[i].x][150 - greyhawk_contacts[i].y].map, "&+W%s&N",
            greyhawk_ships[greyhawk_contacts[i].shipnum].id);
  }
  
  /* Add our ship to map */
  sprintf(greyhawk_tactical[(int)greyhawk_ships[shipnum].x][150 - (int)greyhawk_ships[shipnum].y].map, "&+W**&N");
}

/* ========================================================================= */
/* GREYHAWK SHIP MANAGEMENT FUNCTIONS                                      */
/* ========================================================================= */

/**
 * Load a new ship from template
 * @param template Template object vnum
 * @param to_room Room to place ship in
 * @param x_cord X coordinate
 * @param y_cord Y coordinate
 * @param z_cord Z coordinate
 * @return Ship index, or -1 on failure
 */
int greyhawk_loadship(int template, int to_room, short int x_cord, short int y_cord, short int z_cord) {
  struct obj_data *shiptemplateobj = NULL, *obj = NULL;
  int i = 45000, j = 0, x, y, c, rroom;
  unsigned char armor, internal;
  
  /* Read template object */
  shiptemplateobj = read_object(template, VIRTUAL);
  if (!shiptemplateobj) {
    log("GREYHAWK SHIPS: Failed to load template object %d", template);
    return -1;
  }
  
  /* Create ship object */
  obj = read_object(4500, VIRTUAL);  /* TODO: Make this configurable */
  if (!obj) {
    log("GREYHAWK SHIPS: Failed to load ship object 4500");
    extract_obj(shiptemplateobj);
    return -1;
  }
  
  /* Calculate armor and internal values */
  armor = GET_OBJ_VAL(shiptemplateobj, 2) / 2;
  internal = armor / 4;
  
  if (armor == 0) armor = 1;
  if (internal == 0) internal = 1;
  
  /* Find available ship room */
  while (world[real_room(i)].ship != NULL && i < 46000) {
    i++;
  }
  
  /* Find available ship slot */
  while (greyhawk_ships[j].shiproom != 0 && j < GREYHAWK_MAXSHIPS) {
    j++;
  }
  
  if (j >= GREYHAWK_MAXSHIPS) {
    log("GREYHAWK SHIPS: No available ship slots");
    extract_obj(shiptemplateobj);
    extract_obj(obj);
    return -1;
  }
  
  /* Initialize ship data */
  memset(&greyhawk_ships[j], 0, sizeof(struct greyhawk_ship_data));
  
  /* Link room to ship */
  world[real_room(i)].ship = &greyhawk_ships[j];
  
  /* Set basic ship properties */
  greyhawk_ships[j].shipnum = j;
  greyhawk_ships[j].shiproom = i;
  greyhawk_ships[j].farmor = armor;
  greyhawk_ships[j].maxfarmor = armor;
  greyhawk_ships[j].rarmor = armor;
  greyhawk_ships[j].maxrarmor = armor;
  greyhawk_ships[j].parmor = armor;
  greyhawk_ships[j].maxparmor = armor;
  greyhawk_ships[j].sarmor = armor;
  greyhawk_ships[j].maxsarmor = armor;
  greyhawk_ships[j].finternal = internal;
  greyhawk_ships[j].maxfinternal = internal;
  greyhawk_ships[j].rinternal = internal;
  greyhawk_ships[j].maxrinternal = internal;
  greyhawk_ships[j].pinternal = internal;
  greyhawk_ships[j].maxpinternal = internal;
  greyhawk_ships[j].sinternal = internal;
  greyhawk_ships[j].maxsinternal = internal;
  greyhawk_ships[j].hullweight = GET_OBJ_VAL(shiptemplateobj, 2);
  
  /* Set position */
  greyhawk_ships[j].x = y_cord + 50;
  greyhawk_ships[j].y = (50 - x_cord) + 50;
  greyhawk_ships[j].z = z_cord;
  greyhawk_ships[j].location = world[to_room].number;
  greyhawk_ships[j].shipobj = obj;
  
  /* Configure ship object */
  GET_OBJ_VAL(greyhawk_ships[j].shipobj, 0) = i;
  GET_OBJ_VAL(greyhawk_ships[j].shipobj, 1) = j;
  
  /* Copy room properties from template */
  rroom = real_room(i);
  world[rroom].room_flags = world[real_room(template)].room_flags;
  world[rroom].x_size = world[real_room(template)].x_size;
  world[rroom].y_size = world[real_room(template)].y_size;
  world[rroom].z_size = world[real_room(template)].z_size;
  
  /* Copy map data if available */
  if (world[real_room(template)].map) {
    if (world[rroom].map == NULL) {
      CREATE(world[rroom].map, struct map_data, 1);
      CREATE(world[rroom].map->map_grid, sh_int *, world[real_room(template)].x_size + 1);
      for (c = 0; c < world[real_room(template)].x_size + 1; c++) {
        CREATE(world[rroom].map->map_grid[c], sh_int, world[rroom].y_size + 1);
      }
    }
    
    for (x = 0; x < world[rroom].x_size; x++) {
      for (y = 0; y < world[rroom].y_size; y++) {
        world[rroom].map->map_grid[x][y] = world[real_room(template)].map->map_grid[x][y];
      }
    }
  }
  
  /* Place ship object in world */
  obj_to_room(obj, to_room);
  
  /* Clean up template object */
  extract_obj(shiptemplateobj);
  
  return j;
}

/**
 * Set ship name and update object descriptions
 * @param name Ship name string
 * @param shipnum Ship index
 */
void greyhawk_nameship(char *name, int shipnum) {
  if (shipnum < 0 || shipnum >= GREYHAWK_MAXSHIPS) {
    log("GREYHAWK SHIPS: Invalid ship number %d in nameship", shipnum);
    return;
  }
  
  strncpy(greyhawk_ships[shipnum].name, name, sizeof(greyhawk_ships[shipnum].name) - 1);
  greyhawk_ships[shipnum].name[sizeof(greyhawk_ships[shipnum].name) - 1] = '\0';
  
  if (greyhawk_ships[shipnum].shipobj) {
    if (greyhawk_ships[shipnum].shipobj->description)
      free(greyhawk_ships[shipnum].shipobj->description);
    if (greyhawk_ships[shipnum].shipobj->short_description)
      free(greyhawk_ships[shipnum].shipobj->short_description);
      
    greyhawk_ships[shipnum].shipobj->description = str_dup(name);
    greyhawk_ships[shipnum].shipobj->short_description = str_dup(name);
  }
}

/**
 * Configure ship sail system
 * @param class Sail class (0-150)
 * @param shipnum Ship index
 * @return TRUE if successful, FALSE if invalid
 */
bool greyhawk_setsail(int class, int shipnum) {
  int weight, i;
  
  if (shipnum < 0 || shipnum >= GREYHAWK_MAXSHIPS) {
    log("GREYHAWK SHIPS: Invalid ship number %d in setsail", shipnum);
    return FALSE;
  }
  
  /* Check if hull can support this sail class */
  if (greyhawk_ships[shipnum].hullweight < class - 25)
    return FALSE;
  
  /* Calculate total equipment weight */
  weight = 0;
  for (i = 0; i < GREYHAWK_MAXSLOTS; i++) {
    weight += greyhawk_ships[shipnum].slot[i].weight;
  }
  
  /* Check weight capacity */
  if ((weight + class / 3) > (greyhawk_ships[shipnum].hullweight / 2))
    return FALSE;
  
  /* Set speed characteristics */
  greyhawk_ships[shipnum].minspeed = -1;
  greyhawk_ships[shipnum].maxspeed = (int)(class * (66.0 / greyhawk_ships[shipnum].hullweight) + 0.5);
  
  if (greyhawk_ships[shipnum].maxspeed < 0)
    greyhawk_ships[shipnum].maxspeed = 0;
  
  /* Set sail values */
  greyhawk_ships[shipnum].mainsail = class / 2;
  greyhawk_ships[shipnum].maxmainsail = class / 2;
  
  return TRUE;
}

/**
 * Initialize Greyhawk ship system
 * Call this during game boot sequence
 */
void greyhawk_initialize_ships(void) {
  int i;
  
  /* Clear ship array */
  memset(greyhawk_ships, 0, sizeof(greyhawk_ships));
  
  /* Clear contact array */
  memset(greyhawk_contacts, 0, sizeof(greyhawk_contacts));
  
  /* Initialize tactical map with default ocean pattern */
  for (i = 0; i < 150; i++) {
    int j;
    for (j = 0; j < 150; j++) {
      strcpy(greyhawk_tactical[i][j].map, "&+b~~&N");
    }
  }
  
  log("GREYHAWK SHIPS: Ship system initialized");
}

/* ========================================================================= */
/* GREYHAWK SPECIAL PROCEDURES                                             */
/* ========================================================================= */

/**
 * Main ship command handler special procedure
 * Handles tactical, contacts, weaponspec, status, speed, heading, disembark commands
 * @param obj Object (unused)
 * @param ch Character issuing command
 * @param cmd Command number
 * @param argument Command arguments
 * @return TRUE if command handled, FALSE otherwise
 */
SPECIAL(greyhawk_ship_commands) {
  int i, j, k, p;
  char buf[MAX_STRING_LENGTH];
  
  if (!ch || !cmd)
    return FALSE;

  if (CMD_IS("tactical")) {
    int x, y;
    float shiprange;

    two_arguments(argument, greyhawk_arg1, greyhawk_arg2);

    if (!*greyhawk_arg1) {
      x = (int)GREYHAWK_SHIPX(ch->in_room);
      y = (int)GREYHAWK_SHIPY(ch->in_room);
    } else {
      if (!*greyhawk_arg2) {
        send_to_char(ch, "&+WSyntax: Tactical <x> <y> or Tactical&N\r\n");
        return TRUE;
      }
      if (is_number(greyhawk_arg1) && is_number(greyhawk_arg2)) {
        shiprange = greyhawk_range(GREYHAWK_SHIPX(ch->in_room),
                                  GREYHAWK_SHIPY(ch->in_room),
                                  0,
                                  atoi(greyhawk_arg1),
                                  atoi(greyhawk_arg2),
                                  0);
        if ((int)(shiprange + .5) <= 35) {
          x = atoi(greyhawk_arg1);
          y = atoi(greyhawk_arg2);
        } else {
          sprintf(buf, "This coord is out of range.\r\nMust be within 35 units.\r\nCurrent range: %3.1f\r\n", shiprange);
          send_to_char(ch, buf);
          return TRUE;
        }
      } else {
        send_to_char(ch, "&+WSyntax: Tactical <x> <y> or Tactical&N\r\n");
        return TRUE;
      }
    }
    
    greyhawk_getmap(GREYHAWK_SHIPNUM(ch->in_room));
    sprintf(buf, "&+W     %-3d   %-3d   %-3d   %-3d   %-3d  %-3d   %-3d   %-3d   %-3d   %-3d   %-3d   %-3d&N\r\n",
            x - 11, x - 9, x - 7, x - 5, x - 3, x - 1, x + 1, x + 3, x + 5, x + 7, x + 9, x + 11);
    send_to_char(ch, buf);
    sprintf(buf, "     __ &+W%-3d&N__ &+W%-3d&N__ &+W%-3d&N__ &+W%-3d&N__ &+W%-3d&N__ &+W%-3d&N__ &+W%-3d&N__ &+W%-3d&N__ &+W%-3d&N__ &+W%-3d&N__ &+W%-3d&N__&N\r\n",
            x - 10, x - 8, x - 6, x - 4, x - 2, x, x + 2, x + 4, x + 6, x + 8, x + 10);
    send_to_char(ch, buf);
    y = 150 - y;
    for (i = y - 7; i < y + 8; i++) {
      sprintf(buf, "&+W%-3d&N /%s\\__/%s\\__/%s\\__/%s\\__/%s\\__/%s\\__/%s\\__/%s\\__/%s\\__/%s\\__/%s\\__/%s\\\r\n",
              150 - i,
              greyhawk_tactical[x - 11][i].map,
              greyhawk_tactical[x - 9][i].map,
              greyhawk_tactical[x - 7][i].map,
              greyhawk_tactical[x - 5][i].map,
              greyhawk_tactical[x - 3][i].map,
              greyhawk_tactical[x - 1][i].map,
              greyhawk_tactical[x + 1][i].map,
              greyhawk_tactical[x + 3][i].map,
              greyhawk_tactical[x + 5][i].map,
              greyhawk_tactical[x + 7][i].map,
              greyhawk_tactical[x + 9][i].map,
              greyhawk_tactical[x + 11][i].map);
      send_to_char(ch, buf);
      sprintf(buf, "    \\__/%s\\__/%s\\__/%s\\__/%s\\__/%s\\__/%s\\__/%s\\__/%s\\__/%s\\__/%s\\__/%s\\__/\r\n",
              greyhawk_tactical[x - 10][i].map,
              greyhawk_tactical[x - 8][i].map,
              greyhawk_tactical[x - 6][i].map,
              greyhawk_tactical[x - 4][i].map,
              greyhawk_tactical[x - 2][i].map,
              greyhawk_tactical[x][i].map,
              greyhawk_tactical[x + 2][i].map,
              greyhawk_tactical[x + 4][i].map,
              greyhawk_tactical[x + 6][i].map,
              greyhawk_tactical[x + 8][i].map,
              greyhawk_tactical[x + 10][i].map);
      send_to_char(ch, buf);
    }
    send_to_char(ch, "       \\__/  \\__/  \\__/  \\__/  \\__/  \\__/  \\__/  \\__/  \\__/  \\__/  \\__/\r\n");
    return TRUE;
  }
  
  if (CMD_IS("contacts")) {
    k = greyhawk_getcontacts(GREYHAWK_SHIPNUM(ch->in_room));
    send_to_char(ch, "&+WContact listing\r\n===============================================&N\r\n");
    if (k > 0) {
      for (p = 0; p < k; p++) {
        greyhawk_dispcontact(p);
        send_to_char(ch, greyhawk_contact);
      }
    }
    return TRUE;
  }
  
  if (CMD_IS("weaponspec")) {
    if (world[ch->in_room].ship != NULL) {
      send_to_char(ch, "&+rWeapon Specifications&N\r\n");
      send_to_char(ch, "&+r=================================================&N\r\n");
      send_to_char(ch, "Num  Name                 Min    Med    Long   Dam  Status\r\n");
      j = 0;
      for (i = 0; i < GREYHAWK_MAXSLOTS; i++) {
        if (GREYHAWK_SHIPSLOT(ch->in_room)[i].type == 1) {
          greyhawk_getstatus(i, ch->in_room);
          sprintf(buf, "&+W[%d]  %-20s &N%2d-%-2d  %2d-%-2d  %2d-%-2d  %-3d  %s&N\r\n",
                  j, GREYHAWK_SHIPSLOT(ch->in_room)[i].desc,
                  GREYHAWK_SHIPSLOT(ch->in_room)[i].val1,
                  greyhawk_weaprange(GREYHAWK_SHIPNUM(ch->in_room), i, GREYHAWK_SHRTRANGE),
                  greyhawk_weaprange(GREYHAWK_SHIPNUM(ch->in_room), i, GREYHAWK_SHRTRANGE) + 1,
                  greyhawk_weaprange(GREYHAWK_SHIPNUM(ch->in_room), i, GREYHAWK_MEDRANGE),
                  greyhawk_weaprange(GREYHAWK_SHIPNUM(ch->in_room), i, GREYHAWK_MEDRANGE) + 1,
                  greyhawk_weaprange(GREYHAWK_SHIPNUM(ch->in_room), i, GREYHAWK_LNGRANGE),
                  GREYHAWK_SHIPSLOT(ch->in_room)[i].val2,
                  greyhawk_status);
          send_to_char(ch, buf);
          j++;
        }
      }
      return TRUE;
    }
    return FALSE;
  }
  
  if (CMD_IS("status")) {
    if (world[ch->in_room].ship != NULL) {
      sprintf(buf, "%s\r\n", GREYHAWK_SHIPNAME(ch->in_room));
      send_to_char(ch, buf);
      send_to_char(ch, "&+L-========================================================================-&N\r\n");
      sprintf(buf, "&+LCaptain: &+W%-58s&+LID[&+Y%s&+L]\r\n\r\n", GREYHAWK_SHIPOWNER(ch->in_room), GREYHAWK_SHIPID(ch->in_room));
      send_to_char(ch, buf);

      sprintf(buf, "        &+G%3d&N/&+G%-3d  &+LSpeed Range: &+W0-%-3d&+L       Sail Crew: &+W%-20s&N\r\n",
              GREYHAWK_SHIPFARMOR(ch->in_room), GREYHAWK_SHIPMAXFARMOR(ch->in_room), GREYHAWK_SHIPMAXSPEED(ch->in_room), GREYHAWK_SHIPSAILNAME(ch->in_room));
      send_to_char(ch, buf);

      sprintf(buf, "                      &+LWeight: &+W%3d,000  &+LGunnery Crew: &+W%-20s&N\r\n",
              GREYHAWK_SHIPHULLWEIGHT(ch->in_room), GREYHAWK_SHIPGUNNAME(ch->in_room));
      send_to_char(ch, buf);
      send_to_char(ch, "           &+y||&N\r\n");
      send_to_char(ch, "          &+y/..\\            &N&+rWeapons:&N\r\n");

      sprintf(buf, "         &+y/.&+g%2d&+y.\\        &+r-=================================================-&N\r\n",
              GREYHAWK_SHIPFINTERNAL(ch->in_room));
      send_to_char(ch, buf);
      send_to_char(ch, "        &+y/..&N--&+y..\\        &+LNum  Name                 Status  Position   Ammo&N\r\n");

      greyhawk_dispweapon(0, ch->in_room);
      sprintf(buf, "        &+y|..&+g%2d&+y..|        &+W[0]  %s&N\r\n",
              GREYHAWK_SHIPMAXFINTERNAL(ch->in_room), greyhawk_weapon);
      send_to_char(ch, buf);

      greyhawk_dispweapon(1, ch->in_room);
      sprintf(buf, "        &+y|......| &+g%2d     &+W[1]  %s&N\r\n",
              GREYHAWK_SHIPMAINSAIL(ch->in_room), greyhawk_weapon);
      send_to_char(ch, buf);

      greyhawk_dispweapon(2, ch->in_room);
      sprintf(buf, "        &+y\\__..__/ &N--     &+W[2]  %s&N\r\n",
              greyhawk_weapon);
      send_to_char(ch, buf);

      greyhawk_dispweapon(3, ch->in_room);
      sprintf(buf, "        &+y|..||..|&+L/&N&+g%2d     &+W[3]  %s&N\r\n",
              GREYHAWK_SHIPMAXMAINSAIL(ch->in_room), greyhawk_weapon);
      send_to_char(ch, buf);

      greyhawk_dispweapon(4, ch->in_room);
      sprintf(buf, "        &+y|......&+L/        &+W[4]  %s&N\r\n",
              greyhawk_weapon);
      send_to_char(ch, buf);

      greyhawk_dispweapon(5, ch->in_room);
      sprintf(buf, "        &+y|.....&+L/&N&+y|        &+W[5]  %s&N\r\n",
              greyhawk_weapon);
      send_to_char(ch, buf);

      greyhawk_dispweapon(6, ch->in_room);
      sprintf(buf, "        &+y|....&+L/&N&+y.|        &+W[6]  %s&N\r\n",
              greyhawk_weapon);
      send_to_char(ch, buf);

      greyhawk_dispweapon(7, ch->in_room);
      sprintf(buf, "    &+G%3d &N&+y|&+g%2d&+y.&+L/&N&+g%2d&+y| &+G%3d    &+W[7]  %s&N\r\n",
              GREYHAWK_SHIPPARMOR(ch->in_room), GREYHAWK_SHIPPINTERNAL(ch->in_room), GREYHAWK_SHIPSINTERNAL(ch->in_room), GREYHAWK_SHIPSARMOR(ch->in_room), greyhawk_weapon);
      send_to_char(ch, buf);

      greyhawk_dispweapon(8, ch->in_room);
      sprintf(buf, "    &N--- &+y|&N--&+Y/\\&N--&+y| &N---    &+W[8] %s&N\r\n",
              greyhawk_weapon);
      send_to_char(ch, buf);

      greyhawk_dispweapon(9, ch->in_room);
      sprintf(buf, "    &+G%3d &N&+y|&+g%2d&+Y\\/&N&+g%2d&+y| &+G%3d    &+W[9]  %s&N\r\n",
              GREYHAWK_SHIPMAXPARMOR(ch->in_room), GREYHAWK_SHIPMAXPINTERNAL(ch->in_room), GREYHAWK_SHIPMAXSINTERNAL(ch->in_room), GREYHAWK_SHIPMAXSARMOR(ch->in_room), greyhawk_weapon);
      send_to_char(ch, buf);
      send_to_char(ch, "        &+y|......|&N\r\n");
      send_to_char(ch, "        &+y|__||__|&N\r\n");
      send_to_char(ch, "        &+y/......\\&N\r\n");
      send_to_char(ch, "        &+y|......|&N\r\n");

      sprintf(buf, "        &+y|..&+g%2d&+y..|      &NX:&+W%3.1f  &NY:&+W%3.1f  &NZ:&+W%3.1f&N\r\n",
              GREYHAWK_SHIPRINTERNAL(ch->in_room), GREYHAWK_SHIPX(ch->in_room), GREYHAWK_SHIPY(ch->in_room), GREYHAWK_SHIPZ(ch->in_room));
      send_to_char(ch, buf);
      send_to_char(ch, "        &+y|..&N--&+y..|&N\r\n");

      sprintf(buf, "        &+y|..&+g%2d&+y..|    &NSet Heading: &+W%-3d   &NSet Speed: &+W%-4d&N\r\n",
              GREYHAWK_SHIPMAXRINTERNAL(ch->in_room), GREYHAWK_SHIPSETHEADING(ch->in_room), GREYHAWK_SHIPSETSPEED(ch->in_room));
      send_to_char(ch, buf);

      sprintf(buf, "        &+y\\______/        &NHeading: &+W%-3d       &NSpeed: &+W%-4d&N\r\n\r\n",
              GREYHAWK_SHIPHEADING(ch->in_room), GREYHAWK_SHIPSPEED(ch->in_room));
      send_to_char(ch, buf);

      sprintf(buf, "        &+G%3d&N/&+G%-3d&N\r\n",
              GREYHAWK_SHIPRARMOR(ch->in_room), GREYHAWK_SHIPMAXRARMOR(ch->in_room));
      send_to_char(ch, buf);
    } else {
      send_to_char(ch, "&+WThis ship has no Data.&N\r\n");
    }
    return TRUE;
  }
  
  if (CMD_IS("speed")) {
    char speed;
    one_argument(argument, greyhawk_arg1);

    if (!*greyhawk_arg1) {
      sprintf(buf, "Current speed: &+W%d&N\r\nSet speed: &+W%d&N\r\n",
              GREYHAWK_SHIPSPEED(ch->in_room), GREYHAWK_SHIPSETSPEED(ch->in_room));
      send_to_char(ch, buf);
    } else {
      if (is_number(greyhawk_arg1)) {
        speed = atoi(greyhawk_arg1);
        if ((GREYHAWK_SHIPMINSPEED(ch->in_room) <= speed) && (speed <= GREYHAWK_SHIPMAXSPEED(ch->in_room))) {
          GREYHAWK_SHIPSETSPEED(ch->in_room) = speed;
          sprintf(buf, "Speed set to &+W%d&N.\r\n", speed);
          send_to_room(ch->in_room, buf);
        } else {
          sprintf(buf, "This ship can only go from &+W%d&N to &+W%d&N.\r\n",
                  GREYHAWK_SHIPMINSPEED(ch->in_room), GREYHAWK_SHIPMAXSPEED(ch->in_room));
          send_to_char(ch, buf);
        }
      } else {
        sprintf(buf, "Please enter a number value between %3d-%-d.\r\n",
                GREYHAWK_SHIPMINSPEED(ch->in_room), GREYHAWK_SHIPMAXSPEED(ch->in_room));
        send_to_char(ch, buf);
      }
    }
    return TRUE;
  }
  
  if (CMD_IS("heading")) {
    int heading;
    one_argument(argument, greyhawk_arg1);

    if (!*greyhawk_arg1 || !is_number(greyhawk_arg1)) {
      sprintf(buf, "Current heading: &+W%d&N\r\nSet heading: &+W%d&N\r\n",
              GREYHAWK_SHIPHEADING(ch->in_room), GREYHAWK_SHIPSETHEADING(ch->in_room));
      send_to_char(ch, buf);
      return TRUE;
    }
    heading = atoi(greyhawk_arg1);
    if ((0 <= heading) && (heading <= 360)) {
      GREYHAWK_SHIPSETHEADING(ch->in_room) = heading;
      sprintf(buf, "Heading set to &+W%d&N.\r\n", heading);
      send_to_room(ch->in_room, buf);
      return TRUE;
    }
    send_to_char(ch, "Please enter a heading from 0-360.\r\n");
    return TRUE;
  }
  
  if (CMD_IS("disembark")) {
    int to_room, in_room;
    short int x, y, z;

    y = GREYHAWK_SHIPX(ch->in_room) - 50;
    x = 100 - GREYHAWK_SHIPY(ch->in_room);
    z = GREYHAWK_SHIPZ(ch->in_room);
    to_room = real_room(GREYHAWK_SHIPLOCATION(ch->in_room));
    in_room = ch->in_room;
    if (to_room == NOWHERE) {
      send_to_char(ch, "This ship is in Limbo.\r\n");
      return TRUE;
    }
    sprintf(buf, "You disembark %s.\r\n", GREYHAWK_SHIPNAME(ch->in_room));
    send_to_char(ch, buf);
    sprintf(buf, "%s disembarks %s.\r\n", GET_NAME(ch), GREYHAWK_SHIPNAME(ch->in_room));
    send_to_room(to_room, buf);
    send_to_room(in_room, buf);
    char_from_room(ch);
    char_to_room(ch, to_room);
    return TRUE;
  }
  
  if (CMD_IS("debug")) {
    sprintf(buf, "SHIPLOCATION: %d\r\n", GREYHAWK_SHIPLOCATION(ch->in_room));
    send_to_char(ch, buf);
    return TRUE;
  }
  
  return FALSE;
}

/**
 * Ship object special procedure - handles entering ships
 * @param obj Ship object
 * @param ch Character attempting to interact
 * @param cmd Command number
 * @param argument Command arguments
 * @return TRUE if command handled, FALSE otherwise
 */
SPECIAL(greyhawk_ship_object) {
  struct obj_data *obj;
  int to_room;
  char buf[MAX_STRING_LENGTH];
  
  obj = me;

  if (!ch || !cmd)
    return FALSE;

  if (GET_OBJ_TYPE(obj) != GREYHAWK_ITEM_SHIP)
    return FALSE;

  if (CMD_IS("enter")) {
    one_argument(argument, greyhawk_arg1);

    if (!*greyhawk_arg1) {
      return FALSE;
    } else {
      if (isname(greyhawk_arg1, GET_OBJ_NAME(obj))) {
        to_room = real_room(GET_OBJ_VAL(obj, 0));
        if (to_room == NOWHERE) {
          send_to_char(ch, "ERROR, tell a god NOW!\r\n");
          return TRUE;
        }
        sprintf(buf, "You board %s.\r\n", obj->description);
        send_to_char(ch, buf);
        sprintf(buf, "%s boards %s.\r\n", GET_NAME(ch), obj->description);
        send_to_room(IN_ROOM(obj), buf);
        send_to_room(to_room, buf);
        char_from_room(ch);
        char_to_room(ch, to_room);
        return TRUE;
      }
    }
  }
  return FALSE;
}

/**
 * Ship loader special procedure - handles admin ship creation commands
 * @param obj Object (unused)
 * @param ch Character issuing command
 * @param cmd Command number
 * @param argument Command arguments
 * @return TRUE if command handled, FALSE otherwise
 */
SPECIAL(greyhawk_ship_loader) {
  char buf[MAX_STRING_LENGTH];
  
  if (!ch || !cmd)
    return FALSE;

  if (CMD_IS("showmap")) {
    int x, y;
    two_arguments(argument, greyhawk_arg1, greyhawk_arg2);
    if (!*greyhawk_arg1 || !*greyhawk_arg2) {
      send_to_char(ch, "need X Y\r\n");
      return TRUE;
    }
    if (is_number(greyhawk_arg1) && is_number(greyhawk_arg2)) {
      x = atoi(greyhawk_arg1);
      y = atoi(greyhawk_arg2);
      sprintf(buf, "map symbol for this coord: %s\r\n", greyhawk_tactical[x][y].map);
      send_to_char(ch, buf);
      if (world[ch->in_room].map) {
        sprintf(buf, "mapgrid symbol: %d\r\n", world[ch->in_room].map->map_grid[X_LOC(ch)][Y_LOC(ch)]);
        send_to_char(ch, buf);
      }
      return TRUE;
    }
    send_to_char(ch, "X Y must be numbers\r\n");
    return TRUE;
  }
  
  if (CMD_IS("mapload")) {
    int x, y;
    for (x = 0; x < 150; x++) {
      for (y = 0; y < 150; y++) {
        strcpy(greyhawk_tactical[x][y].map, "&+y^^&N");
      }
    }
    send_to_char(ch, "Loaded test map\r\n");
    return TRUE;
  }
  
  if (CMD_IS("shipload")) {
    int template, shipnum, shiproom;

    one_argument(argument, greyhawk_arg1);

    if (!*greyhawk_arg1) {
      send_to_char(ch, "Please specify template number.\r\n");
      return TRUE;
    }
    if (!is_number(greyhawk_arg1)) {
      send_to_char(ch, "Invalid template!\r\n");
      return TRUE;
    }
    template = atoi(greyhawk_arg1);
    send_to_char(ch, "Loading ship data\r\n");
    
    /* Get character coordinates - simplified for Luminari */
    int x_coord = 50, y_coord = 50, z_coord = 0;  /* Default coordinates */
    
    shipnum = greyhawk_loadship(template, ch->in_room, x_coord, y_coord, z_coord);
    if (shipnum < 0) {
      send_to_char(ch, "Failed to load ship.\r\n");
      return TRUE;
    }
    
    if (!greyhawk_setsail(150, shipnum))
      send_to_char(ch, "Ship cannot support this sail.\r\n");
      
    shiproom = real_room(greyhawk_ships[shipnum].shiproom);
    
    /* Configure sample ship */
    sprintf(buf, "&+LThe 'Chtorran'&N");
    greyhawk_nameship(buf, GREYHAWK_SHIPNUM(shiproom));
    strcpy(GREYHAWK_SHIPOWNER(shiproom), GET_NAME(ch));
    strcpy(GREYHAWK_SHIPID(shiproom), "AB");
    strcpy(GREYHAWK_SHIPSAILNAME(shiproom), "Magical Automatons");
    strcpy(GREYHAWK_SHIPGUNNAME(shiproom), "Magical Automatons");
    
    /* Configure sample weapons */
    strcpy(GREYHAWK_SHIPSLOT(shiproom)[0].desc, "Catapult of Slaying");
    strcpy(GREYHAWK_SHIPSLOT(shiproom)[1].desc, "Large Ballistae");
    strcpy(GREYHAWK_SHIPSLOT(shiproom)[2].desc, "Large Ballistae");
    
    GREYHAWK_SHIPSLOT(shiproom)[0].type = 1;
    GREYHAWK_SHIPSLOT(shiproom)[0].timer = 0;
    GREYHAWK_SHIPSLOT(shiproom)[0].position = GREYHAWK_FORE;
    GREYHAWK_SHIPSLOT(shiproom)[0].val0 = 25;
    GREYHAWK_SHIPSLOT(shiproom)[0].val1 = 10;
    GREYHAWK_SHIPSLOT(shiproom)[0].val2 = 20;
    GREYHAWK_SHIPSLOT(shiproom)[0].val3 = 100;
    
    GREYHAWK_SHIPSLOT(shiproom)[1].type = 1;
    GREYHAWK_SHIPSLOT(shiproom)[1].timer = 0;
    GREYHAWK_SHIPSLOT(shiproom)[1].position = GREYHAWK_PORT;
    GREYHAWK_SHIPSLOT(shiproom)[1].val0 = 15;
    GREYHAWK_SHIPSLOT(shiproom)[1].val1 = 0;
    GREYHAWK_SHIPSLOT(shiproom)[1].val2 = 7;
    GREYHAWK_SHIPSLOT(shiproom)[1].val3 = 100;
    
    GREYHAWK_SHIPSLOT(shiproom)[2].type = 1;
    GREYHAWK_SHIPSLOT(shiproom)[2].timer = 23;
    GREYHAWK_SHIPSLOT(shiproom)[2].position = GREYHAWK_STARBOARD;
    GREYHAWK_SHIPSLOT(shiproom)[2].val0 = 15;
    GREYHAWK_SHIPSLOT(shiproom)[2].val1 = 0;
    GREYHAWK_SHIPSLOT(shiproom)[2].val2 = 7;
    GREYHAWK_SHIPSLOT(shiproom)[2].val3 = 100;
    
    send_to_char(ch, "Done.\r\n");
    return TRUE;
  }
  
  if (CMD_IS("setsail")) {
    if (world[ch->in_room].ship == NULL) {
      send_to_char(ch, "&+WThis room is not a ship or has no data.&N\r\n");
      return TRUE;
    }
    one_argument(argument, greyhawk_arg1);

    if (!*greyhawk_arg1 || !is_number(greyhawk_arg1)) {
      send_to_char(ch, "Please select a Sail Class: 0-150\r\n");
      return TRUE;
    }
    if (!greyhawk_setsail(atoi(greyhawk_arg1), GREYHAWK_SHIPNUM(ch->in_room))) {
      send_to_char(ch, "This ship cannot support this class of sail.\r\n");
      return TRUE;
    }
    send_to_char(ch, "done.\r\n");
    return TRUE;
  }
  
  return FALSE;
}

#endif /* Disabled compilation block */