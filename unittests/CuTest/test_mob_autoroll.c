#include "CuTest.h"

#include "../../src/conf.h"
#include "../../src/sysdep.h"
#include "../../src/structs.h"
#include "../../src/utils.h"
#include "../../src/magic/spells.h"
#include "../../src/character/abilities.h"
#include "../../src/character/class.h"
#include "../../src/mob/mob_autoroll.h"

#include <limits.h>

void Test_mob_tier_formula_v1_hit_point_vectors(CuTest *tc)
{
  int result = 0;

  CuAssertTrue(tc, mob_tier_calculate_hit_points(1496, MOB_TIER_STANDARD, &result));
  CuAssertIntEquals(tc, 1496, result);
  CuAssertTrue(tc, mob_tier_calculate_hit_points(1496, MOB_TIER_ELITE, &result));
  CuAssertIntEquals(tc, 3841, result);
  CuAssertTrue(tc, mob_tier_calculate_hit_points(1496, MOB_TIER_SMALL_GROUP, &result));
  CuAssertIntEquals(tc, 7845, result);
  CuAssertTrue(tc, mob_tier_calculate_hit_points(1496, MOB_TIER_BIG_GROUP, &result));
  CuAssertIntEquals(tc, 12611, result);
  CuAssertTrue(tc, mob_tier_calculate_hit_points(1496, MOB_TIER_RAID, &result));
  CuAssertIntEquals(tc, 18252, result);
  CuAssertTrue(tc, mob_tier_calculate_hit_points(1496, MOB_TIER_WORLD_BOSS, &result));
  CuAssertIntEquals(tc, 18252, result);

  CuAssertTrue(tc, mob_tier_calculate_hit_points(598, MOB_TIER_ELITE, &result));
  CuAssertIntEquals(tc, 1865, result);
  CuAssertTrue(tc, mob_tier_calculate_hit_points(598, MOB_TIER_SMALL_GROUP, &result));
  CuAssertIntEquals(tc, 3499, result);
  CuAssertTrue(tc, mob_tier_calculate_hit_points(598, MOB_TIER_BIG_GROUP, &result));
  CuAssertIntEquals(tc, 5439, result);
  CuAssertTrue(tc, mob_tier_calculate_hit_points(598, MOB_TIER_RAID, &result));
  CuAssertIntEquals(tc, 7735, result);
}

void Test_mob_tier_formula_v1_combat_modifiers(CuTest *tc)
{
  const int attack[] = {0, 2, 3, 5, 7};
  const int armor[] = {0, 2, 3, 4, 5};
  const int bypass[] = {0, 30, 40, 50, 60};
  int tier;

  for (tier = MOB_TIER_STANDARD; tier <= MOB_TIER_RAID; tier++)
  {
    CuAssertIntEquals(tc, attack[tier], mob_tier_attack_bonus(tier));
    CuAssertIntEquals(tc, armor[tier], mob_tier_armor_bonus(tier));
    CuAssertIntEquals(tc, tier, mob_tier_damage_bonus(tier));
    CuAssertIntEquals(tc, tier, mob_tier_extra_attacks(tier));
    CuAssertIntEquals(tc, tier * 2, mob_tier_critical_confirmation_bonus(tier));
    CuAssertIntEquals(tc, bypass[tier], mob_tier_defense_bypass_percent(tier));
  }
  CuAssertIntEquals(tc, MOB_TIER_RAID, mob_tier_formula_rank(MOB_TIER_WORLD_BOSS));
  CuAssertIntEquals(tc, 60, mob_tier_defense_bypass_percent(MOB_TIER_WORLD_BOSS));
}

void Test_mob_tier_formula_rejects_invalid_input(CuTest *tc)
{
  int result = 1234;

  CuAssertTrue(tc, !mob_tier_calculate_hit_points(0, MOB_TIER_STANDARD, &result));
  CuAssertTrue(tc, !mob_tier_calculate_hit_points(100, -1, &result));
  CuAssertTrue(tc, !mob_tier_calculate_hit_points(100, NUM_MOB_TIERS, &result));
  CuAssertTrue(tc, !mob_tier_calculate_hit_points(INT_MAX, MOB_TIER_RAID, &result));
  CuAssertTrue(tc, !mob_tier_calculate_hit_points(100, MOB_TIER_STANDARD, NULL));
  CuAssertIntEquals(tc, 1234, result);
  CuAssertStrEquals(tc, "Invalid", mob_tier_name(NUM_MOB_TIERS));
}

void Test_mob_tier_legacy_fallback_preserves_old_level_ladder(CuTest *tc)
{
  struct char_data mob;

  clear_char(&mob);
  GET_MOB_TIER(&mob) = MOB_TIER_UNSPECIFIED;
  GET_LEVEL(&mob) = 30;
  CuAssertIntEquals(tc, MOB_TIER_STANDARD, mob_effective_tier(&mob));
  GET_LEVEL(&mob) = 31;
  CuAssertIntEquals(tc, MOB_TIER_ELITE, mob_effective_tier(&mob));
  GET_LEVEL(&mob) = 34;
  CuAssertIntEquals(tc, MOB_TIER_RAID, mob_effective_tier(&mob));
  GET_LEVEL(&mob) = 40;
  CuAssertIntEquals(tc, MOB_TIER_RAID, mob_effective_tier(&mob));

  GET_MOB_TIER(&mob) = MOB_TIER_STANDARD;
  CuAssertIntEquals(tc, MOB_TIER_STANDARD, mob_effective_tier(&mob));
}

void Test_autoroll_mob_applies_every_tier_once_and_serializes_exact_hp(CuTest *tc)
{
  const int expected_hit_points[] = {1496, 3841, 7845, 12611, 18252, 18252};
  const int expected_damage_bonus[] = {6, 7, 8, 9, 10, 10};
  const int expected_armor[] = {440, 460, 470, 480, 490, 490};
  struct char_data mob;
  int tier;

  for (tier = MOB_TIER_STANDARD; tier <= MOB_TIER_WORLD_BOSS; tier++)
  {
    clear_char(&mob);
    SET_BIT_AR(MOB_FLAGS(&mob), MOB_ISNPC);
    GET_LEVEL(&mob) = 34;
    GET_CLASS(&mob) = CLASS_WARRIOR;
    GET_REAL_RACE(&mob) = RACE_TYPE_HUMANOID;
    GET_REAL_SIZE(&mob) = SIZE_MEDIUM;
    mob.points.size = SIZE_MEDIUM;
    GET_MOB_TIER(&mob) = tier;

    autoroll_mob(&mob, false, false);

    CuAssertIntEquals(tc, 1, GET_HIT(&mob));
    CuAssertIntEquals(tc, 1, GET_PSP(&mob));
    CuAssertIntEquals(tc, expected_hit_points[tier] - 1, GET_MOVE(&mob));
    CuAssertIntEquals(tc, expected_damage_bonus[tier], GET_DAMROLL(&mob));
    CuAssertIntEquals(tc, expected_armor[tier], mob.points.armor);
  }
}
