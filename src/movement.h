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

/* Track system constants */
#define TRACKS_UNDEFINED 0
#define TRACKS_IN 1
#define TRACKS_OUT 2
#define DIR_NONE -1

/* Door macros - Used by external modules */
#define DOOR_IS_OPENABLE(ch, obj, door) ((obj) ? (((GET_OBJ_TYPE(obj) ==                    \
                                                    ITEM_CONTAINER) ||                      \
                                                   GET_OBJ_TYPE(obj) == ITEM_AMMO_POUCH) && \
                                                  OBJVAL_FLAGGED(obj, CONT_CLOSEABLE))      \
                                               : (EXIT_FLAGGED(EXIT(ch, door), EX_ISDOOR)))
#define DOOR_IS_OPEN(ch, obj, door) ((obj) ? (!OBJVAL_FLAGGED(obj,          \
                                                              CONT_CLOSED)) \
                                           : (!EXIT_FLAGGED(EXIT(ch, door), EX_CLOSED)))
#define DOOR_IS_UNLOCKED(ch, obj, door) ((obj) ? (!OBJVAL_FLAGGED(obj,                                                                         \
                                                                  CONT_LOCKED))                                                                \
                                               : (!EXIT_FLAGGED(EXIT(ch, door), EX_LOCKED) && !EXIT_FLAGGED(EXIT(ch, door), EX_LOCKED_EASY) && \
                                                  !EXIT_FLAGGED(EXIT(ch, door), EX_LOCKED_MEDIUM) && !EXIT_FLAGGED(EXIT(ch, door), EX_LOCKED_HARD)))
#define DOOR_IS_PICKPROOF(ch, obj, door) ((obj) ? (OBJVAL_FLAGGED(obj,             \
                                                                  CONT_PICKPROOF)) \
                                                : (EXIT_FLAGGED(EXIT(ch, door), EX_PICKPROOF)))
#define DOOR_IS_CLOSED(ch, obj, door) (!(DOOR_IS_OPEN(ch, obj, door)))
#define DOOR_IS_LOCKED(ch, obj, door) (!(DOOR_IS_UNLOCKED(ch, obj, door)))
#define DOOR_KEY(ch, obj, door) ((obj) ? ((GET_OBJ_TYPE(obj) == ITEM_TREASURE_CHEST) ? 0 : GET_OBJ_VAL(obj, 2)) : (EXIT(ch, door)->key))

/* Core movement functions */
int do_simple_move(struct char_data *ch, int dir, int need_specials_check);
int perform_move(struct char_data *ch, int dir, int need_specials_check);
int perform_move_full(struct char_data *ch, int dir, int need_specials_check, bool recursive);

/* Movement utility functions */
bool is_top_of_room_for_singlefile(struct char_data *ch, int dir);
struct char_data *get_char_ahead_of_me(struct char_data *ch, int dir);

/* Track/trail functions */
void create_tracks(struct char_data *ch, int dir, int flag);
void cleanup_all_trails(void);

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
extern int movement_loss[];

#endif /* MOVEMENT_H */