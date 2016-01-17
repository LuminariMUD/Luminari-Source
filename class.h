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

/* prereq data for class */
struct class_prerequisite {
  /* FEAT_PREREQ_* values determine the type */
  int  prerequisite_type;
  char *description; /* Generated string value describing prerequisite. */

  /* 0: ability score, class, feat, race, casting type, BAB
   * 1: ability score value, class, feat, race, prep type, min BAB
   * 2: N/A, class level, feat ranks, N/A, minimum circle, N/A */
  int values[3];

  /* Linked list */
  struct class_prerequisite *next;
};


/* class data, layout for storing class information for each class */
struct class_table {
  char *name; /* full name of class, ex. wizard (no color) */
  char *abbrev; /* abbreviation of class, ex. wiz (no color) */
  char *colored_abbrev; /* same as abbrev, but colored */
  char *menu_name; /* colored full name of class for menu(s) */
  int max_level; /* maximum number of levels you can take in this class, -1 unlimited */
  bool locked_class; /* whether by default this class is locked or not */
  bool prestige_class; /* prestige class? */
  int base_attack_bonus; /* whether high, medium or low */
  int hit_dice; /* how many hp this class can get on level up */
  int mana_gain; /* how much mana this class gets on level up */
  int move_gain; /* how much moves this class gets on level up */
  int trains_gain; /* how many trains this class gets before int bonus */
  bool in_game; /* class currently in the game? */
  
  int preferred_saves[5];  /*high or low saving throw values */
  int class_abil[NUM_ABILITIES];  /*class ability (not avail, cross-class, class-skill)*/
  
  struct class_prerequisite *prereq_list; /* A list of prerequisite sctructures */
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
void init_class(struct char_data *ch, int class, int level);

/* handy macros for dealing with class_list[] */
#define CLSLIST_NAME(classnum)            (class_list[classnum].name)
#define CLSLIST_ABBRV(classnum)           (class_list[classnum].abbrev)
#define CLSLIST_CLRABBRV(classnum)        (class_list[classnum].colored_abbrev)
#define CLSLIST_MENU(classnum)            (class_list[classnum].menu_name)
#define CLSLIST_MAXLVL(classnum)          ( (class_list[classnum].max_level == -1) ? (LVL_IMMORT-1) : (class_list[classnum].max_level) )
#define CLSLIST_LOCK(classnum)            (class_list[classnum].locked_class)
#define CLSLIST_PRESTIGE(classnum)        (class_list[classnum].prestige_class)
#define CLSLIST_BAB(classnum)             (class_list[classnum].base_attack_bonus)
#define CLSLIST_HPS(classnum)             (class_list[classnum].hit_dice)
#define CLSLIST_MANA(classnum)            (class_list[classnum].mana_gain)
#define CLSLIST_MVS(classnum)             (class_list[classnum].move_gain)
#define CLSLIST_TRAINS(classnum)          (class_list[classnum].trains_gain)
#define CLSLIST_INGAME(classnum)          (class_list[classnum].in_game)
#define CLSLIST_SAVES(classnum, savenum)  (class_list[classnum].preferred_saves[savenum])
#define CLSLIST_ABIL(classnum, abilnum)   (class_list[classnum].class_abil[abilnum])

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
