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

/* structs - race data for extension of races */
struct race_data {
    
  /* displaying the race */
  char *name; /* lower case no-spaces (for like accessing help file) */
  char *type; /* full capitalized and spaced version */  
  char *type_color; /* full colored, capitalized and spaced version */
  char *abbrev; /* 4 letter abbreviation */
  char *abbrev_color; /* 4 letter abbreviation colored */
  
  char *descrip; /* race description */
  
  char *morph_to_char; /* wildshape message to ch */
  char *morph_to_room; /* wildshape message to room */
  
  /* race assigned values! */
  ubyte family; /* race's family type (iron golem would be a CONSTRUCT) */
  byte size; /* default size class for this race */
  bool is_pc; /* can a PC select this race to play? */
  ubyte level_adjustment; /* for pc-races: penalty to xp for race due to power */
  int unlock_cost; /* if locked, cost to unlock in account xp */
  byte epic_adv; /* normal, advance or epic race (pc)? */
  
  /* array assigned values! */
  sbyte genders[NUM_SEX]; /* this race can be this sex? */
  byte ability_mods[NUM_ABILITY_MODS]; /* modifications to base stats based on race */
  sbyte alignments[NUM_ALIGNMENTS]; /* acceptable alignments for this race */
  byte attack_types[NUM_ATTACK_TYPES]; /* race have this attack type? (when not wielding) */
  
  /* linked lists */
  struct race_feat_assign *featassign_list; /* list of feat assigns */
  struct affect_assign *affassign_list; /* list of affect assigns */
  
  /* these are only ideas for now */
  
  /*int body_parts[NUM_WEARS];*/ /* for expansion - to add customized wear slots */
  /*byte favored_class[NUM_SEX];*/ /* favored class system, not yet implemented */
  /*ush_int language;*/ /* default language - not used yet */
};

extern struct race_data race_list[];

/* functions */
int parse_race(char arg);
bitvector_t find_race_bitvector(const char *arg);
int invalid_race(struct char_data *ch, struct obj_data *obj);
int parse_race_long(char *arg);

/* extended races file (races_ext.c) */
void assign_races(void);

/* Global variables */

#ifndef __RACE_C__


#endif /* __RACE_C__ */

#endif /* _RACE_H_*/

