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
bool monk_gear_ok(struct char_data *ch);
int proficiency_worn(struct char_data *ch, int type);
int compute_gear_penalty_check(struct char_data *ch);
int determine_gear_weight(struct char_data *ch, int type);
int compute_gear_arcane_fail(struct char_data *ch);
int compute_gear_max_dex(struct char_data *ch);

/* Global variables */

#ifndef __CLASS_C__  

extern const char *class_abbrevs[];
extern const char *pc_class_types[];
extern const char *class_menu;
extern const char *church_types[];
extern int prac_params[][NUM_CLASSES];
extern struct guild_info_type guild_info[];
extern int class_ability[NUM_ABILITIES][NUM_CLASSES];

extern const int *class_bonus_feats[NUM_CLASSES];

#endif /* __CLASS_C__ */

#endif /* _CLASS_H_*/
