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
#define MAX_TALENTS 64

/* Talent indices (add new ones above TALENT_MAX). Keep sequential */
#define TALENT_NONE                0
#define TALENT_EFFICIENT_CRAFTING  1  /* -5% material consumption (placeholder logic) */
#define TALENT_PRECISE_CRAFTING    2  /* +5% success / quality bonus */
#define TALENT_RAPID_HARVEST       3  /* -10% harvest action time */
#define TALENT_MASTER_REFINER      4  /* +10% refine yield */
#define TALENT_RESOURCE_INSIGHT    5  /* +5% chance rare resource */
#define TALENT_SUPERIOR_SUPPLY     6  /* +5% supply order rewards */
#define TALENT_ARTISAN_TOUCH       7  /* +5% quality tier roll */
#define TALENT_BULK_SPECIALIST     8  /* +10% quantity on bulk contracts */
#define TALENT_ALCHEMICAL_FOCUS    9  /* +5% potion potency (placeholder) */

#define TALENT_MAX                10  /* one past highest */

/* Data definition structure for each talent */
struct talent_info {
  const char *name;        /* display name */
  int base_point_cost;     /* base talent point cost for rank 1 */
  int base_gold_cost;      /* base gold cost for rank 1 */
  int max_ranks;           /* maximum ranks attainable (>=1) */
  const char *short_desc;  /* brief description for list */
  const char *long_desc;   /* optional extended description */
  bool in_game;            /* available for selection */
};

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
