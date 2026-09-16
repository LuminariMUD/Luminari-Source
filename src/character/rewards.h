/**************************************************************************
 *  File: rewards.h                                    Part of LuminariMUD *
 *  Usage: Reward API for experience, currency, and progression balances.  *
 **************************************************************************/

#ifndef REWARDS_H
#define REWARDS_H

#include "core/structs.h"

#ifdef __cplusplus
extern "C"
{
#endif

/* Balances changed through the reward API.  The order matches award_types[] in
 * constants.c, which the staff award command lists and parses. */
#define AWARD_EXPERIENCE 0
#define AWARD_QUEST_POINTS 1
#define AWARD_ACCOUNT_EXPERIENCE 2
#define AWARD_GOLD 3
#define AWARD_BANK_GOLD 4
#define AWARD_SKILL_POINTS 5
#define AWARD_FEATS 6
#define AWARD_CLASS_FEATS 7
#define AWARD_EPIC_FEATS 8
#define AWARD_EPIC_CLASS_FEATS 9
#define AWARD_ABILITY_BOOSTS 10
#define NUM_AWARD_TYPES 11

/* Modes for award_experience(); each selects the caps for earned experience. */
#define AWARD_EXP_MODE_DEFAULT 0
#define AWARD_EXP_MODE_QUEST 1
#define AWARD_EXP_MODE_CRAFT 2
#define AWARD_EXP_MODE_SCRIPT 3
#define AWARD_EXP_MODE_DEATH 4
#define AWARD_EXP_MODE_GROUP 5
#define AWARD_EXP_MODE_SOLO 6
#define AWARD_EXP_MODE_DAMAGE 7
#define AWARD_EXP_MODE_EDRAIN 8
#define AWARD_EXP_MODE_DUMP 9
#define AWARD_EXP_MODE_TRAP 10

/* Every function below accepts a NULL character and then returns 0 without side
 * effects.  Except for award_capacity(), each returns the signed change actually
 * applied to the balance.
 * Balances stay within [0, limit], where the limits are MAX_GOLD, MAX_BANK,
 * MAX_QUEST_POINTS, and MAX_ACCOUNT_EXPERIENCE from structs.h and the storage
 * range of the other fields.  A credit never lowers a balance and a debit never
 * raises one.  Quest points, bank gold, and the progression pools apply only to
 * players; account experience also requires a connected account. */

/* Apply an exact signed change of any award type, with no bonuses or caps
 * beyond the balance limit. */
long award_points(struct char_data *ch, int type, long amount);

/* Set an award balance, clamped to [0, limit]. */
long award_set_points(struct char_data *ch, int type, long value);

/* The largest credit of this type that ch can accept in full: the limit minus the
 * current balance, clamped to [0, limit], or 0 when the balance does not apply.
 * Exchanges, transfers, sales, and payouts check it before taking anything in return. */
long award_capacity(struct char_data *ch, int type);

/* Earned experience: applies bonuses, happy hour, and the caps for the given
 * AWARD_EXP_MODE_*.  A negative gain is a capped loss. */
int award_experience(struct char_data *ch, int gain, int mode);

/* Experience without earned caps, as used by staff advancement and
 * resurrection.  Unless is_ress is set, happy hour applies and the character
 * advances every level the new total reaches. */
int award_experience_uncapped(struct char_data *ch, int gain, bool is_ress);

/* Exact changes to a single balance; negative amounts are costs. */
int award_gold(struct char_data *ch, int amount);
int award_bank_gold(struct char_data *ch, int amount);
int award_quest_points(struct char_data *ch, int amount);
int award_account_experience(struct char_data *ch, int amount);

#ifdef __cplusplus
}
#endif

#endif /* REWARDS_H */
