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

int mob_tier_formula_rank(int tier)
{
  if (!mob_tier_is_valid(tier) || tier == MOB_TIER_STANDARD)
    return 0;
  if (tier == MOB_TIER_WORLD_BOSS)
    return MOB_TIER_RAID;
  return tier;
}

bool mob_tier_calculate_hit_points(int base_hit_points, int tier, int *result)
{
  int64_t hit_points;
  int rank;
  int i;

  if (!result || base_hit_points < 1 || !mob_tier_is_valid(tier))
    return false;

  rank = mob_tier_formula_rank(tier);
  if (rank == 0)
  {
    *result = base_hit_points;
    return true;
  }

  hit_points = (int64_t)2 * rank * base_hit_points + 500;
  for (i = 0; i < rank; i++)
    hit_points += hit_points / 10;

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
  *hitroll += rank + 1 + (rank > 2 ? rank - 2 : 0);
  *armor_class += (rank + 1) * 10;
  *damage_bonus += rank;
  return true;
}
