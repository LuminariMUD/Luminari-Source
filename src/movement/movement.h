/**************************************************************************
 *  File: movement.h                                   Part of LuminariMUD *
 *  Usage: Header file for movement system                                 *
 *                                                                         *
 *  All rights reserved.  See license complete information.               *
 *                                                                         *
 *  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.              *
 **************************************************************************/

#ifndef MOVEMENT_H
#define MOVEMENT_H

/* Include sub-module headers */
#include "movement_validation.h"
#include "movement_cost.h"
#include "movement_position.h"
#include "movement_doors.h"
#include "movement_falling.h"
#include "movement_messages.h"
#include "movement_tracks.h"
#include "movement_events.h"

/* Track system constants and door macros are defined in their respective sub-modules:
 * - Track constants: see movement_tracks.h
 * - Door macros: see movement_doors.h
 */

/* Core movement functions */
int perform_move_full(struct char_data *ch, int dir, int need_specials_check, bool recursive);

/* Movement utility functions */

/* Track/trail functions */

/* Movement commands - ACMDs */
ACMD_DECL(do_move);
ACMD_DECL(do_enter);
ACMD_DECL(do_leave);
ACMD_DECL(do_follow);
ACMD_DECL(do_pullswitch);
ACMD_DECL(do_unlead);
ACMD_DECL(do_sorcerer_draconic_wings);
ACMD_DECL(do_transposition);
ACMD_DECL(do_unstuck);
ACMD_DECL(do_lastroom);

/* Movement cost array - defined in movement.c */

#endif /* MOVEMENT_H */
