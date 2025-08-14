/**************************************************************************
 *  File: mob_race.h                                  Part of LuminariMUD *
 *  Usage: Header for mobile racial behavior functions                    *
 *                                                                         *
 *  All rights reserved.  See license for complete information.           *
 *                                                                         *
 *  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.              *
 **************************************************************************/

#ifndef _MOB_RACE_H_
#define _MOB_RACE_H_

/* Function prototypes for mob racial behaviors */

/* handle racial-specific behaviors for NPCs */
void npc_racial_behave(struct char_data *ch);

#endif /* _MOB_RACE_H_ */