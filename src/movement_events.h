/**************************************************************************
 *  File: movement_events.h                            Part of LuminariMUD *
 *  Usage: Header file for post-movement event processing.                *
 *                                                                         *
 *  All rights reserved.  See license for complete information.           *
 *                                                                         *
 *  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 **************************************************************************/

#ifndef _MOVEMENT_EVENTS_H_
#define _MOVEMENT_EVENTS_H_

/* Function prototypes for post-movement event processing */

/**
 * Process all post-movement events
 * @param ch Character who moved
 * @param was_in Room they moved from
 * @param going_to Room they moved to
 * @param dir Direction they moved
 * @return TRUE if movement succeeds, FALSE if an event prevents it
 */
bool process_movement_events(struct char_data *ch, room_rnum was_in, room_rnum going_to, int dir);

/**
 * Process room damage effects like spike growth
 * @param ch Character entering the room
 * @param room Room being entered
 * @param riding TRUE if mounted
 * @param same_room TRUE if mount is in same room
 */
void process_room_damage(struct char_data *ch, room_rnum room, int riding, int same_room);

/**
 * Process trap detection for characters with trap sense
 * @param ch Character to check for trap detection
 */
void process_trap_detection(struct char_data *ch);

/**
 * Process wilderness-specific events
 * @param ch Character who moved
 * @param was_in Previous room
 */
void process_wilderness_events(struct char_data *ch, room_rnum was_in);

/**
 * Process vampire weakness effects
 * @param ch Character to check for vampire weaknesses
 */
void process_vampire_weaknesses(struct char_data *ch);

/**
 * Process pre-movement trap checks
 * @param ch Character leaving a room
 */
void process_leave_traps(struct char_data *ch);

#endif /* _MOVEMENT_EVENTS_H_ */