/**************************************************************************
 *  File: movement_doors.h                            Part of LuminariMUD *
 *  Usage: Door handling functions header                                 *
 *                                                                         *
 *  All rights reserved.  See license complete information.               *
 *                                                                         *
 *  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.              *
 **************************************************************************/

#ifndef MOVEMENT_DOORS_H
#define MOVEMENT_DOORS_H

/* Door macros - moved from movement.c */
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

/* Function declarations for door handling */
int is_evaporating_key(struct char_data *ch, obj_vnum key);
int has_key(struct char_data *ch, obj_vnum key);
void extract_key(struct char_data *ch, obj_vnum key);

/* ACMD declaration for door command */
ACMD_DECL(do_gen_door);

#endif /* MOVEMENT_DOORS_H */