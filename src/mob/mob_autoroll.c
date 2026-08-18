#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "mob_autoroll.h"

#include <limits.h>
#include <stdint.h>

static const char *mob_tier_names[NUM_MOB_TIERS] = {"Standard",  "Elite", "Small Group",
                                                    "Big Group", "Raid",  "World Boss"};

const char *mob_tier_name(int tier)
{
  if (!mob_tier_is_valid(tier))
    return "Invalid";
  return mob_tier_names[tier];
}

bool mob_tier_is_valid(int tier)
{
  return tier >= MOB_TIER_STANDARD && tier < NUM_MOB_TIERS;
}

/* Encounter-tier hit point multipliers, expressed as a percentage of the tier-0
   autostat base.  Formula v2 replaced the v1 "2 * rank * base + 500, then compound
   +10% per rank" curve, which multiplied a level 34 raid mob by nearly twelve and
   put it roughly three times above the hand-statted reference boss.

   The anchor is the Prisoner (vnum 113750): level 34, 30d1000+30000 hit points, so
   an average base of 45015 before db.c's powerful-being bump.  A level 34 warrior
   autostats to a tier-0 base of 11968, and 11968 * 375% = 44880 lands on that
   anchor.  The intermediate tiers are a uniform ~1.29x step so each rung is a
   meaningful but not explosive jump. */
static const int mob_tier_hit_point_percent[NUM_MOB_TIERS] = {100, 135, 175, 225, 290, 375};

int mob_tier_formula_rank(int tier)
{
  if (!mob_tier_is_valid(tier))
    return 0;
  return tier;
}

bool mob_tier_calculate_hit_points(int base_hit_points, int tier, int *result)
{
  int64_t hit_points;
  int rank;

  if (!result || base_hit_points < 1 || !mob_tier_is_valid(tier))
    return false;

  rank = mob_tier_formula_rank(tier);
  if (rank == 0)
  {
    *result = base_hit_points;
    return true;
  }

  hit_points = (int64_t)base_hit_points * mob_tier_hit_point_percent[rank] / 100;

  if (hit_points > INT_MAX)
    return false;
  *result = (int)hit_points;
  return true;
}

bool mob_tier_apply_autostat_bonuses(int tier, int *hit_points, int *hitroll, int *armor_class,
                                     int *damage_bonus)
{
  int adjusted_hit_points;
  int rank;

  if (!hit_points || !hitroll || !armor_class || !damage_bonus)
    return false;
  if (tier == MOB_TIER_UNSPECIFIED || tier == MOB_TIER_STANDARD)
    return true;
  if (!mob_tier_is_valid(tier) ||
      !mob_tier_calculate_hit_points(*hit_points, tier, &adjusted_hit_points))
    return false;

  rank = mob_tier_formula_rank(tier);
  *hit_points = adjusted_hit_points;
  *hitroll += rank * 2;
  *armor_class += rank * 10;
  *damage_bonus += rank;
  return true;
}
