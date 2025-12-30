/* ************************************************************************
 *      File:   transport_unified.h                     Part of LuminariMUD  *
 *   Purpose:   Unified transport abstraction layer for vehicles and vessels *
 *                                                                           *
 *   Phase 02, Session 06: Unified Command Interface                         *
 *   Provides transport-agnostic commands (enter, exit, go, tstatus) that    *
 *   work seamlessly with both vehicles and vessels.                         *
 *                                                                           *
 *   All rights reserved.  See license for complete information.             *
 *                                                                           *
 *   LuminariMUD is based on CircleMUD, Copyright (C) 1993, 94.              *
 *   CircleMUD (C) 1993, 94 by the Trustees of the Johns Hopkins University  *
 *   CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.                *
 ************************************************************************* */

#ifndef _TRANSPORT_UNIFIED_H_
#define _TRANSPORT_UNIFIED_H_

/* ========================================================================= */
/* INCLUDES                                                                  */
/* ========================================================================= */

#include "structs.h"
#include "vessels.h"

/* ========================================================================= */
/* TRANSPORT TYPE ENUMERATION                                                 */
/* ========================================================================= */

/**
 * Transport type classification for unified command handling.
 * Used to determine which underlying system to delegate commands to.
 */
enum transport_type
{
  TRANSPORT_NONE = 0, /* No transport present/invalid */
  TRANSPORT_VEHICLE,  /* Land-based vehicle (cart, wagon, mount, carriage) */
  TRANSPORT_VESSEL    /* Water-based vessel (ship, boat, etc.) */
};

/* ========================================================================= */
/* TRANSPORT DATA ABSTRACTION STRUCTURE                                       */
/* ========================================================================= */

/**
 * Unified transport data structure.
 *
 * Provides a single interface to access either vehicle or vessel data
 * without needing to know the underlying transport type at call sites.
 * Uses a union to store a pointer to the actual transport data.
 *
 * Usage pattern:
 *   struct transport_data td;
 *   if (get_transport_in_room(room, &td)) {
 *     if (td.type == TRANSPORT_VEHICLE) {
 *       // Use td.data.vehicle
 *     } else if (td.type == TRANSPORT_VESSEL) {
 *       // Use td.data.vessel
 *     }
 *   }
 */
struct transport_data
{
  enum transport_type type; /* Type of transport stored in data union */

  union
  {
    struct vehicle_data *vehicle;      /* Vehicle data pointer (if type == TRANSPORT_VEHICLE) */
    struct greyhawk_ship_data *vessel; /* Vessel data pointer (if type == TRANSPORT_VESSEL) */
  } data;
};

/* ========================================================================= */
/* TRANSPORT DETECTION FUNCTION PROTOTYPES                                    */
/* ========================================================================= */

/**
 * Detect transport type in a room.
 *
 * Scans the room for vehicles first (more common), then vessels.
 * Returns TRANSPORT_NONE if no transport is present.
 *
 * @param room The room to scan for transports
 * @return Transport type found in room, or TRANSPORT_NONE
 */
enum transport_type get_transport_type_in_room(room_rnum room);

/**
 * Get transport data from a room.
 *
 * Populates the transport_data structure with the transport found in room.
 * Checks for vehicles first, then vessels.
 *
 * @param room The room to scan
 * @param td Pointer to transport_data structure to populate
 * @return 1 if transport found, 0 if none
 */
int get_transport_in_room(room_rnum room, struct transport_data *td);

/**
 * Get a character's current transport.
 *
 * Checks if character is currently in/on a vehicle or vessel.
 * For vehicles, checks the mounted_players tracking system.
 * For vessels, checks if character is in a ship interior room.
 *
 * @param ch The character to check
 * @param td Pointer to transport_data structure to populate
 * @return 1 if character is in a transport, 0 if not
 */
int get_character_transport(struct char_data *ch, struct transport_data *td);

/**
 * Check if character is currently in any transport.
 *
 * Convenience function that returns TRUE if character is in a vehicle or vessel.
 *
 * @param ch The character to check
 * @return TRUE if in transport, FALSE otherwise
 */
int is_in_transport(struct char_data *ch);

/* ========================================================================= */
/* TRANSPORT HELPER FUNCTION PROTOTYPES                                       */
/* ========================================================================= */

/**
 * Get display name for a transport type.
 *
 * @param type The transport type
 * @return Static string name ("vehicle", "vessel", or "unknown")
 */
const char *transport_type_name(enum transport_type type);

/**
 * Get the name of a specific transport.
 *
 * Returns the name of the vehicle or vessel stored in transport_data.
 *
 * @param td Pointer to transport_data structure
 * @return Transport name string, or "unknown" if invalid
 */
const char *get_transport_name(struct transport_data *td);

/**
 * Check if a transport is operational (can be used).
 *
 * For vehicles, checks condition > 0.
 * For vessels, checks if ship is in valid state.
 *
 * @param td Pointer to transport_data structure
 * @return TRUE if operational, FALSE otherwise
 */
int is_transport_operational(struct transport_data *td);

/* ========================================================================= */
/* COMMAND FUNCTION PROTOTYPES (ACMD)                                         */
/* ========================================================================= */

/**
 * Unified entry command.
 *
 * Usage: tenter [target]
 *
 * Handles entering/boarding either vehicles or vessels.
 * Detects transport type in room and delegates to appropriate system.
 * Note: Named do_transport_enter to avoid conflict with do_enter in movement.c.
 *
 * @param ch The character executing the command
 * @param argument Command arguments (optional target name)
 * @param cmd Command number
 * @param subcmd Subcommand number
 */
ACMD_DECL(do_transport_enter);

/**
 * Unified exit command.
 *
 * Usage: exit (while in transport)
 *
 * Handles dismounting from vehicles or disembarking from vessels.
 * Determines current transport type and delegates appropriately.
 * Note: Named do_exit_transport to avoid conflict with do_exits.
 *
 * @param ch The character executing the command
 * @param argument Command arguments (unused)
 * @param cmd Command number
 * @param subcmd Subcommand number
 */
ACMD_DECL(do_exit_transport);

/**
 * Unified movement command.
 *
 * Usage: go <direction>
 *
 * Handles driving vehicles or sailing vessels in a direction.
 * Detects current transport type and delegates to appropriate movement system.
 *
 * @param ch The character executing the command
 * @param argument Direction to travel
 * @param cmd Command number
 * @param subcmd Subcommand number
 */
ACMD_DECL(do_transport_go);

/**
 * Unified status display command.
 *
 * Usage: tstatus
 *
 * Shows status information for current transport (vehicle or vessel).
 * Delegates to vstatus or vessel_status based on transport type.
 *
 * @param ch The character executing the command
 * @param argument Command arguments (unused)
 * @param cmd Command number
 * @param subcmd Subcommand number
 */
ACMD_DECL(do_transportstatus);

#endif /* _TRANSPORT_UNIFIED_H_ */
