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
#ifndef _RACE_H_
#define _RACE_H_

/* defines */
// this is for shapechange/wildshape, define for array max
#define NUM_SHAPE_TYPES  5

/* order of stat attributes relevant for race modifications to stats */
#define R_STR_MOD    0
#define R_CON_MOD    1
#define R_INTEL_MOD  2
#define R_WIS_MOD    3
#define R_DEX_MOD    4
#define R_CHA_MOD    5
  
/* feat assignment / race-feat data for races */
struct race_feat_assign {
  int feat_num;       /* feat number like FEAT_WEAPON_FOCUS */
  int level_received; /* level race get this feat */
  bool stacks;        /* does this feat stack? */
  struct race_feat_assign *next; /*linked list*/
};

/* feat assignment / race-feat data for races */
struct affect_assign {
  int affect_num;       /* aff number like AFF_HIDE */
  int level_received;   /* level race get this affect */
  struct affect_assign *next; /*linked list*/
};

/* race_data struct use to be here, move to structs.h */
//extern struct race_data race_list[];

/* functions */
int parse_race(char arg);
bitvector_t find_race_bitvector(const char *arg);
int invalid_race(struct char_data *ch, struct obj_data *obj);
int parse_race_long(char *arg);
void assign_races(void);
bool display_race_info(struct char_data *ch, char *racename);

/* ACMD */
ACMD(do_race);

/* Global variables */

#ifndef __RACE_C__


#endif /* __RACE_C__ */

#endif /* _RACE_H_*/

