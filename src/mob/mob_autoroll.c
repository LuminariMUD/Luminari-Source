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

int mob_effective_tier(const struct char_data *mob)
{
  int tier;
  int level;

  if (!mob)
    return MOB_TIER_STANDARD;
  tier = GET_MOB_TIER(mob);
  if (mob_tier_is_valid(tier))
    return tier;

  level = GET_LEVEL(mob);
  if (level > 30)
    return level - 30 > MOB_TIER_RAID ? MOB_TIER_RAID : level - 30;
  return MOB_TIER_STANDARD;
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

int mob_tier_attack_bonus(int tier)
{
  int rank = mob_tier_formula_rank(tier);

  if (rank == 0)
    return 0;
  return rank + 1 + (rank > 2 ? rank - 2 : 0);
}

int mob_tier_armor_bonus(int tier)
{
  int rank = mob_tier_formula_rank(tier);

  return rank == 0 ? 0 : rank + 1;
}

int mob_tier_damage_bonus(int tier)
{
  return mob_tier_formula_rank(tier);
}

int mob_tier_extra_attacks(int tier)
{
  return mob_tier_formula_rank(tier);
}

int mob_tier_critical_confirmation_bonus(int tier)
{
  return mob_tier_formula_rank(tier) * 2;
}

int mob_tier_defense_bypass_percent(int tier)
{
  int rank = mob_tier_formula_rank(tier);

  return rank == 0 ? 0 : 20 + 10 * rank;
}
