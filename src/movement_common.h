/**************************************************************************
 *  File: movement_common.h                           Part of LuminariMUD *
 *  Usage: Common header for movement system during refactoring           *
 *                                                                         *
 *  All rights reserved.  See license complete information.               *
 *                                                                         *
 *  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.              *
 **************************************************************************/

#ifndef MOVEMENT_COMMON_H
#define MOVEMENT_COMMON_H

/* This header provides common declarations during the transition period
 * while functions are being moved from movement.c to their respective modules.
 * This file will be removed once the refactoring is complete.
 */

/* Include all movement module headers */
#include "movement_validation.h"
#include "movement_cost.h"
#include "movement_position.h"
#include "movement_doors.h"
#include "movement_falling.h"

/* Core movement functions that remain in movement.c */
int do_simple_move(struct char_data *ch, int dir, int need_specials_check);
int perform_move(struct char_data *ch, int dir, int need_specials_check);
int perform_move_full(struct char_data *ch, int dir, int need_specials_check, bool recursive);

/* Other movement-related functions */
bool is_top_of_room_for_singlefile(struct char_data *ch, int dir);
struct char_data *get_char_ahead_of_me(struct char_data *ch, int dir);
void create_tracks(struct char_data *ch, int dir, int flag);
void cleanup_all_trails(void);

/* Track flags */
#define TRACKS_UNDEFINED 0
#define TRACKS_IN 1
#define TRACKS_OUT 2
#define DIR_NONE -1

#endif /* MOVEMENT_COMMON_H */