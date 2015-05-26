/**
* @file class.h
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
#ifndef _CLASS_H_
#define _CLASS_H_

/* defines */
#define NUM_CHURCHES 13

#define LF_CLASS    0
#define LF_RACE     1
#define LF_STACK    2
#define LF_MIN_LVL  3
#define LF_FEAT     4
#define LEVEL_FEATS 5
/* end defines */

/* class data, unfinished project */
struct class_data {
  char *name; /* full name of class, ex. wizard */
  char *abbrev; /* abbreviation of class, ex. wiz */
  char *colored_abbrev; /* same as abbrev, but colored */
  int max_level; /* maximum number of levels you can take in this class */
  int class_abilities[NUM_ABILITIES]; /* skills that are class, cross-class or unavailable */
  int preferred_saves[5]; /* high or low saving throw values */
  int locked_class; /* whether by default this class is locked or not */
  int base_attack_bonus; /* whether high, medium or low */
  int hit_dice; /* how many hp this class can get on level up */
  int mana_gain; /* how much mana this class gets on level up */
  int move_gain; /* how much moves this class gets on level up */
  int trains_gain; /* how many trains this class gets before int bonus */
};


/* Functions available through class.c */
int backstab_mult(struct char_data *ch);
void do_start(struct char_data *ch);
void newbieEquipment(struct char_data *ch);
bitvector_t find_class_bitvector(const char *arg);
int invalid_class(struct char_data *ch, struct obj_data *obj);
int level_exp(struct char_data *ch, int level);
int parse_class(char arg);
int parse_class_long(char *arg);
void roll_real_abils(struct char_data *ch);
byte saving_throws(struct char_data *, int type);
int BAB(struct char_data *ch);
const char *titles(int chclass, int level);
int modify_class_ability(struct char_data *ch, int ability, int class);

/* Global variables */

#ifndef __CLASS_C__

extern const char *class_abbrevs[];
extern const char *pc_class_types[];
extern const char *class_menu;
extern const char *church_types[];
extern int prac_params[][NUM_CLASSES];
extern struct guild_info_type guild_info[];
extern int class_ability[NUM_ABILITIES][NUM_CLASSES];
extern int level_feats[][LEVEL_FEATS];
extern int *free_start_feats[];
extern const int *class_bonus_feats[NUM_CLASSES];
extern int class_max_ranks[NUM_CLASSES];

#endif /* __CLASS_C__ */

#endif /* _CLASS_H_*/
