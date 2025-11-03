/**************************************************************************
 *  File: mob_utils.h                                 Part of LuminariMUD *
 *  Usage: Header for mobile utility functions                            *
 *                                                                         *
 *  All rights reserved.  See license for complete information.           *
 *                                                                         *
 *  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.              *
 **************************************************************************/

#ifndef _MOB_UTILS_H_
#define _MOB_UTILS_H_

/* Function prototypes for mob utility functions */

/* find a target for ch to attack */
struct char_data *npc_find_target(struct char_data *ch, int *num_targets);

/* attempt to switch opponents for ch */
bool npc_switch_opponents(struct char_data *ch, struct char_data *vict);

/* attempt to rescue someone */
bool npc_rescue(struct char_data *ch);

/* move ch along a pre-defined path */
bool move_on_path(struct char_data *ch);

/* handle mobile echo behavior */
void mobile_echos(struct char_data *ch);

/* check if ch can continue acting */
int can_continue(struct char_data *ch, bool fighting);

/* check if NPC should call a companion */
bool npc_should_call_companion(struct char_data *ch, int call_type);

#endif /* _MOB_UTILS_H_ */