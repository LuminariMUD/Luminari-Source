/***********************************************************************
** FEATS.H                                                            **
** Header file for the Gates of Krynn Feat System.                    **
** Initial code by Paladine (Stephen Squires)                         **
** Created Thursday, September 5, 2002                                **
** Ported to Luminari by Ornir                                        **
***********************************************************************/

#ifndef _FEATS_H_
#define _FEATS_H_

/* Below is the structure for a feat */
struct feat_info
{
  const char *name;              /* The name of the feat to be displayed to players */
  sbyte in_game;                 /* TRUE or FALSE, is the feat in the game yet? */
  sbyte can_learn;               /* TRUE or FALSE, can the feat be learned or is it an automatic feat? */
  sbyte can_stack;               /* TRUE or FALSE, can the feat be learned more than once? */
  int feat_type;                 /* The type of feat (see defines) for organization in the selection menu. */
  const char *short_description; /* The line displayed in the feat xxxx desc command display. */
  const char *description;       /* Long description of the feat, displayed in 'feat info' */
  sbyte epic;                    /* Is this an epic feat? */
  sbyte combat_feat;             /* Is this a combat feat? */
  int event;                     /* The event_id of the cooldown event, used for daily use active feats. */

  struct feat_prerequisite *prerequisite_list; /* A list of prerequisite sctructures */
};

/* structure for feat prereq system */
struct feat_prerequisite
{
  /* FEAT_PREREQ_* values determine the type */
  int prerequisite_type;
  char *description; /* Generated string value describing prerequisite. */

  /* 0: ability score, class, feat, race, casting type, BAB
   * 1: ability score value, class, feat, race, prep type, min BAB
   * 2: N/A, class level, feat ranks, N/A, minimum circle, N/A */
  int values[3];

  /* Linked list */
  struct feat_prerequisite *next;
};

/* functions */
void load_weapons(void);
void load_armor(void);
void assign_feats(void);
void sort_feats(void);
int feat_is_available(struct char_data *ch, int featnum, int iarg, char *sarg);
int is_class_feat(int featnum, int class, struct char_data *ch);
int is_daily_feat(int featnum);
int has_feat_requirement_check(struct char_data *ch, int featnum);
bool meets_prerequisite(struct char_data *ch, struct feat_prerequisite *prereq, int iarg);
bool has_combat_feat(struct char_data *ch, int cfeat, int compare);
/* For help system integration, */
bool display_feat_info(struct char_data *ch, const char *featname);
int find_feat_num(const char *name);
int feat_to_cfeat(int feat);
int feat_to_sfeat(int feat);
int feat_to_skfeat(int feat);
void list_feats(struct char_data *ch, const char *arg, int list_type, struct char_data *viewer);
extern struct feat_info feat_list[];
extern int feat_sort_info[MAX_FEATS];
int get_sorcerer_bloodline_type(struct char_data *ch);
bool isSorcBloodlineFeat(int featnum);
bool valid_item_feat(int featnum);

/**ACMD***/
ACMD_DECL(do_feats);
ACMD_DECL(do_featlisting);

/* Feat types, don't forget to update in constants.c feat_types[] */
#define FEAT_TYPE_NONE 0
#define FEAT_TYPE_GENERAL 1
#define FEAT_TYPE_COMBAT 2
#define FEAT_TYPE_SPELLCASTING 3
#define FEAT_TYPE_METAMAGIC 4
#define FEAT_TYPE_CRAFT 5
#define FEAT_TYPE_WILD 6
#define FEAT_TYPE_DIVINE 7

#define NUM_LEARNABLE_FEAT_TYPES 8

#define FEAT_TYPE_CLASS_ABILITY 8
#define FEAT_TYPE_INNATE_ABILITY 9
#define FEAT_TYPE_DOMAIN_ABILITY 10
#define FEAT_TYPE_PERFORMANCE 11

#define NUM_FEAT_TYPES 12
/******************/

/*  LIST_FEAT defines, for list_feats function. */
#define LIST_FEATS_KNOWN 0
#define LIST_FEATS_AVAILABLE 1
#define LIST_FEATS_ALL 2

/* Defines for prerequisites */
#define FEAT_PREREQ_NONE 0
#define CLASS_PREREQ_NONE 0
#define FEAT_PREREQ_ATTRIBUTE 1
#define CLASS_PREREQ_ATTRIBUTE 1
#define FEAT_PREREQ_CLASS_LEVEL 2
#define CLASS_PREREQ_CLASS_LEVEL 2
#define FEAT_PREREQ_FEAT 3
#define CLASS_PREREQ_FEAT 3
#define FEAT_PREREQ_ABILITY 4
#define CLASS_PREREQ_ABILITY 4
#define FEAT_PREREQ_SPELLCASTING 5
#define CLASS_PREREQ_SPELLCASTING 5
#define FEAT_PREREQ_RACE 6
#define CLASS_PREREQ_RACE 6
#define FEAT_PREREQ_BAB 7
#define CLASS_PREREQ_BAB 7
#define FEAT_PREREQ_CFEAT 8
#define CLASS_PREREQ_CFEAT 8
#define FEAT_PREREQ_WEAPON_PROFICIENCY 9
#define CLASS_PREREQ_WEAPON_PROFICIENCY 9
#define CLASS_PREREQ_ALIGN 10

/* prereq system, ability scores */
#define AB_NONE 0
#define AB_STR 1
#define AB_DEX 2
#define AB_INT 3
#define AB_WIS 4
#define AB_CON 5
#define AB_CHA 6

/* prereq spell casting types */
#define CASTING_TYPE_NONE 0
#define CASTING_TYPE_ARCANE 1
#define CASTING_TYPE_DIVINE 2
#define CASTING_TYPE_ANY 3

/* prereq spell preparation types */
#define PREP_TYPE_NONE 0
#define PREP_TYPE_PREPARED 1
#define PREP_TYPE_SPONTANEOUS 2
#define PREP_TYPE_ANY 3

/* this was created to handle special scenarios for combat feat requirements
   for classes */
#define CFEAT_SPECIAL_NONE 0
#define CFEAT_SPECIAL_BOW 1
#define NUM_CFEAT_SPECIAL 2

/* just for more clarity in the code */
#define NO_IARG 0

#endif
