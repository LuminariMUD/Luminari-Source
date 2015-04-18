/**
* @file race.h
* Header file for class specific functions and variables.
*
* Part of the core tbaMUD source code distribution, which is a derivative
* of, and continuation of, CircleMUD.
*
* All rights reserved.  See license for complete information.
* Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University
* CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.
*
*/
#ifndef _race_H_
#define _race_H_

/* defines */
// this is for shapechange/wildshape, define for array max
#define NUM_SHAPE_TYPES  5

/* functions */
int parse_race(char arg);
bitvector_t find_race_bitvector(const char *arg);
int invalid_race(struct char_data *ch, struct obj_data *obj);

/* extended races file (races_ext.c) */
void assign_races(void);

/* Global variables */

#ifndef __RACE_C__

extern const char *shape_types[];
extern const char *shape_to_room[];
extern const char *shape_to_char[];
extern const char *npc_race_short[];
extern const char *morph_to_char[];
extern const char *morph_to_room[];
extern const char *npc_race_abbrevs[];
extern const char *race_abbrevs[];
extern const char *npc_subrace_abbrevs[];
extern const char *npc_subrace_types[];
extern const char *npc_race_menu;
extern const char *pc_race_types[];
extern const char *npc_race_types[];
extern const char *race_menu;

#endif /* __RACE_C__ */

#endif /* _RACE_H_*/

