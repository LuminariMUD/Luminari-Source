/* LuminariMUD Clan Zone Control Benefits
 * 
 * This header defines the benefits that clan members receive
 * when their clan controls a zone.
 */

#ifndef _CLAN_BENEFITS_H_
#define _CLAN_BENEFITS_H_

/* Zone control benefit types */
#define ZONE_BENEFIT_NONE         0
#define ZONE_BENEFIT_REGEN_HP     (1 << 0)  /* Increased HP regeneration */
#define ZONE_BENEFIT_REGEN_MANA   (1 << 1)  /* Increased mana regeneration */
#define ZONE_BENEFIT_REGEN_MOVE   (1 << 2)  /* Increased movement regeneration */
#define ZONE_BENEFIT_EXP_BONUS    (1 << 3)  /* Experience point bonus */
#define ZONE_BENEFIT_GOLD_BONUS   (1 << 4)  /* Gold drop bonus */
#define ZONE_BENEFIT_SKILL_BONUS  (1 << 5)  /* Skill check bonus */
#define ZONE_BENEFIT_RESIST_BONUS (1 << 6)  /* Saving throw bonus */
#define ZONE_BENEFIT_DAMAGE_BONUS (1 << 7)  /* Damage bonus */
#define ZONE_BENEFIT_AC_BONUS     (1 << 8)  /* AC bonus */
#define ZONE_BENEFIT_NO_DEATH_PEN (1 << 9)  /* No death penalty in zone */
#define ZONE_BENEFIT_FAST_TRAVEL  (1 << 10) /* Reduced movement cost */
#define ZONE_BENEFIT_SHOP_DISC    (1 << 11) /* Shop discount */

/* Benefit amounts */
#define ZONE_REGEN_HP_BONUS      2    /* +2 HP per tick */
#define ZONE_REGEN_MANA_BONUS    2    /* +2 mana per tick */  
#define ZONE_REGEN_MOVE_BONUS    5    /* +5 movement per tick */
#define ZONE_EXP_BONUS_PERCENT   10   /* 10% experience bonus */
#define ZONE_GOLD_BONUS_PERCENT  15   /* 15% gold bonus */
#define ZONE_SKILL_BONUS         2    /* +2 to skill checks */
#define ZONE_RESIST_BONUS        1    /* +1 to saving throws */
#define ZONE_DAMAGE_BONUS        1    /* +1 to damage rolls */
#define ZONE_AC_BONUS            1    /* +1 AC bonus */
#define ZONE_MOVE_COST_REDUCTION 20   /* 20% movement cost reduction */
#define ZONE_SHOP_DISCOUNT       10   /* 10% shop discount */

/* Function prototypes */
int get_zone_control_benefits(struct char_data *ch);
bool has_zone_benefit(struct char_data *ch, int benefit);
void apply_zone_regen_bonus(struct char_data *ch, int *hp_gain, int *mana_gain, int *move_gain);
int apply_zone_exp_bonus(struct char_data *ch, int exp);
int apply_zone_gold_bonus(struct char_data *ch, int gold);
int apply_zone_skill_bonus(struct char_data *ch, int skill_mod);
int apply_zone_resist_bonus(struct char_data *ch, int save_mod);
int apply_zone_damage_bonus(struct char_data *ch, int damage);
int apply_zone_ac_bonus(struct char_data *ch, int ac);
int apply_zone_move_cost(struct char_data *ch, int move_cost);
int apply_zone_shop_price(struct char_data *ch, struct char_data *keeper, int price);
void display_zone_benefits(struct char_data *ch);

#endif /* _CLAN_BENEFITS_H_ */