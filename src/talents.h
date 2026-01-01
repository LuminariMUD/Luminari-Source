/***********************************************************************
** TALENTS.H                                                           **
** Crafting & Harvesting Talent System (analogous to Feat system)      **
**                                                                     **
** Talents are progression unlocks gained by advancing crafting or     **
** harvesting skills. Each time a player gains a level in any crafting **
** or harvesting skill, they gain 1 talent point. Points can be spent  **
** to unlock crafting enhancements.                                    **
***********************************************************************/

#ifndef _TALENTS_H_
#define _TALENTS_H_

#include "sysdep.h"
#include "structs.h"

/* Forward declaration */
struct char_data;

/* Maximum number of defined talents (expand as needed) */
#define MAX_TALENTS 256

/* Talent indices (add new ones above TALENT_MAX). Keep sequential */
#define TALENT_NONE 0
#define TALENT_SCAVENGER 1

/* Skill-specific talents start at 2 - generated for each crafting/harvesting skill */
/* Pattern: PROFICIENT, RAPID, EXPERTISE, EFFICIENT, INSIGHTFUL for each skill */
/* Woodworking (34) */
#define TALENT_PROFICIENT_WOODWORKING 2
#define TALENT_RAPID_WOODWORKING 3
#define TALENT_WOODWORKING_EXPERTISE 4
#define TALENT_EFFICIENT_WOODWORKING 5
#define TALENT_INSIGHTFUL_WOODWORKING 6
/* Tailoring (35) */
#define TALENT_PROFICIENT_TAILORING 7
#define TALENT_RAPID_TAILORING 8
#define TALENT_TAILORING_EXPERTISE 9
#define TALENT_EFFICIENT_TAILORING 10
#define TALENT_INSIGHTFUL_TAILORING 11
/* Alchemy (36) */
#define TALENT_PROFICIENT_ALCHEMY 12
#define TALENT_RAPID_ALCHEMY 13
#define TALENT_ALCHEMY_EXPERTISE 14
#define TALENT_EFFICIENT_ALCHEMY 15
#define TALENT_INSIGHTFUL_ALCHEMY 16
/* Armorsmithing (37) */
#define TALENT_PROFICIENT_ARMORSMITHING 17
#define TALENT_RAPID_ARMORSMITHING 18
#define TALENT_ARMORSMITHING_EXPERTISE 19
#define TALENT_EFFICIENT_ARMORSMITHING 20
#define TALENT_INSIGHTFUL_ARMORSMITHING 21
/* Weaponsmithing (38) */
#define TALENT_PROFICIENT_WEAPONSMITHING 22
#define TALENT_RAPID_WEAPONSMITHING 23
#define TALENT_WEAPONSMITHING_EXPERTISE 24
#define TALENT_EFFICIENT_WEAPONSMITHING 25
#define TALENT_INSIGHTFUL_WEAPONSMITHING 26
/* Bowmaking (39) */
#define TALENT_PROFICIENT_BOWMAKING 27
#define TALENT_RAPID_BOWMAKING 28
#define TALENT_BOWMAKING_EXPERTISE 29
#define TALENT_EFFICIENT_BOWMAKING 30
#define TALENT_INSIGHTFUL_BOWMAKING 31
/* Jewelcrafting (40) */
#define TALENT_PROFICIENT_JEWELCRAFTING 32
#define TALENT_RAPID_JEWELCRAFTING 33
#define TALENT_JEWELCRAFTING_EXPERTISE 34
#define TALENT_EFFICIENT_JEWELCRAFTING 35
#define TALENT_INSIGHTFUL_JEWELCRAFTING 36
/* Leatherworking (41) */
#define TALENT_PROFICIENT_LEATHERWORKING 37
#define TALENT_RAPID_LEATHERWORKING 38
#define TALENT_LEATHERWORKING_EXPERTISE 39
#define TALENT_EFFICIENT_LEATHERWORKING 40
#define TALENT_INSIGHTFUL_LEATHERWORKING 41
/* Trapmaking (42) */
#define TALENT_PROFICIENT_TRAPMAKING 42
#define TALENT_RAPID_TRAPMAKING 43
#define TALENT_TRAPMAKING_EXPERTISE 44
#define TALENT_EFFICIENT_TRAPMAKING 45
#define TALENT_INSIGHTFUL_TRAPMAKING 46
/* Poisonmaking (43) */
#define TALENT_PROFICIENT_POISONMAKING 47
#define TALENT_RAPID_POISONMAKING 48
#define TALENT_POISONMAKING_EXPERTISE 49
#define TALENT_EFFICIENT_POISONMAKING 50
#define TALENT_INSIGHTFUL_POISONMAKING 51
/* Metalworking (44) */
#define TALENT_PROFICIENT_METALWORKING 52
#define TALENT_RAPID_METALWORKING 53
#define TALENT_METALWORKING_EXPERTISE 54
#define TALENT_EFFICIENT_METALWORKING 55
#define TALENT_INSIGHTFUL_METALWORKING 56
/* Fishing (45) */
#define TALENT_PROFICIENT_FISHING 57
#define TALENT_RAPID_FISHING 58
#define TALENT_FISHING_EXPERTISE 59
#define TALENT_EFFICIENT_FISHING 60
#define TALENT_INSIGHTFUL_FISHING 61
/* Cooking (46) */
#define TALENT_PROFICIENT_COOKING 62
#define TALENT_RAPID_COOKING 63
#define TALENT_COOKING_EXPERTISE 64
#define TALENT_EFFICIENT_COOKING 65
#define TALENT_INSIGHTFUL_COOKING 66
/* Mining (48) */
#define TALENT_PROFICIENT_MINING 67
#define TALENT_RAPID_MINING 68
#define TALENT_MINING_EXPERTISE 69
#define TALENT_EFFICIENT_MINING 70
#define TALENT_INSIGHTFUL_MINING 71
/* Hunting (49) */
#define TALENT_PROFICIENT_HUNTING 72
#define TALENT_RAPID_HUNTING 73
#define TALENT_HUNTING_EXPERTISE 74
#define TALENT_EFFICIENT_HUNTING 75
#define TALENT_INSIGHTFUL_HUNTING 76
/* Forestry (50) */
#define TALENT_PROFICIENT_FORESTRY 77
#define TALENT_RAPID_FORESTRY 78
#define TALENT_FORESTRY_EXPERTISE 79
#define TALENT_EFFICIENT_FORESTRY 80
#define TALENT_INSIGHTFUL_FORESTRY 81
/* Gathering (51) */
#define TALENT_PROFICIENT_GATHERING 82
#define TALENT_RAPID_GATHERING 83
#define TALENT_GATHERING_EXPERTISE 84
#define TALENT_EFFICIENT_GATHERING 85
#define TALENT_INSIGHTFUL_GATHERING 86

/* Mote Synergy Talents (General Category) */
#define TALENT_AIR_MOTE_SYNERGY 87
#define TALENT_DARK_MOTE_SYNERGY 88
#define TALENT_EARTH_MOTE_SYNERGY 89
#define TALENT_FIRE_MOTE_SYNERGY 90
#define TALENT_ICE_MOTE_SYNERGY 91
#define TALENT_LIGHT_MOTE_SYNERGY 92
#define TALENT_LIGHTNING_MOTE_SYNERGY 93
#define TALENT_WATER_MOTE_SYNERGY 94

#define TALENT_MAX 95 /* one past highest */

/* Talent Categories */
#define TALENT_CAT_GENERAL 0
#define TALENT_CAT_WOODWORKING 1
#define TALENT_CAT_TAILORING 2
#define TALENT_CAT_ALCHEMY 3
#define TALENT_CAT_ARMORSMITHING 4
#define TALENT_CAT_WEAPONSMITHING 5
#define TALENT_CAT_BOWMAKING 6
#define TALENT_CAT_JEWELCRAFTING 7
#define TALENT_CAT_LEATHERWORKING 8
#define TALENT_CAT_TRAPMAKING 9
#define TALENT_CAT_POISONMAKING 10
#define TALENT_CAT_METALWORKING 11
#define TALENT_CAT_FISHING 12
#define TALENT_CAT_COOKING 13
#define TALENT_CAT_MINING 14
#define TALENT_CAT_HUNTING 15
#define TALENT_CAT_FORESTRY 16
#define TALENT_CAT_GATHERING 17
#define NUM_TALENT_CATEGORIES 18

/* Data definition structure for each talent */
struct talent_info
{
  const char *name;       /* display name */
  int base_point_cost;    /* base talent point cost for rank 1 */
  int base_gold_cost;     /* base gold cost for rank 1 */
  int max_ranks;          /* maximum ranks attainable (>=1) */
  const char *short_desc; /* brief description for list */
  const char *long_desc;  /* optional extended description */
  bool in_game;           /* available for selection */
  int category;           /* talent category */
};

/* Category name lookup */
extern const char *talent_category_names[NUM_TALENT_CATEGORIES];

extern struct talent_info talent_list[TALENT_MAX];

/* Access helpers */
int get_talent_rank(struct char_data *ch, int talent); /* returns current rank */
int has_talent(struct char_data *ch, int talent);      /* rank > 0 */
int talent_max_ranks(int talent);
int talent_next_point_cost(struct char_data *ch, int talent); /* 0 if maxed */
int talent_next_gold_cost(struct char_data *ch, int talent);  /* 0 if maxed */
int can_learn_talent(struct char_data *ch, int talent);       /* meets costs */
int learn_talent(struct char_data *ch, int talent);           /* returns TRUE on success */
void list_talents(struct char_data *ch);
void gain_talent_point(struct char_data *ch, int amount);

/* Initialization */
void init_talents(void);

/* Command */
ACMD_DECL(do_talents); /* list and learn */
ACMD_DECL(do_talento); /* admin assignment style similar to feato */

#endif /* _TALENTS_H_ */
