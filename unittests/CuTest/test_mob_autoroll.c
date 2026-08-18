#include "CuTest.h"

#include "../../src/conf.h"
#include "../../src/sysdep.h"
#include "../../src/structs.h"
#include "../../src/utils.h"
#include "../../src/db.h"
#include "../../src/magic/spells.h"
#include "../../src/character/abilities.h"
#include "../../src/character/class.h"
#include "../../src/mob/mob_autoroll.h"
#include "../../src/olc/genmob.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

static void init_autoroll_mobile(struct char_data *mob, int level, int race, int ch_class, int tier)
{
  clear_char(mob);
  SET_BIT_AR(MOB_FLAGS(mob), MOB_ISNPC);
  GET_LEVEL(mob) = level;
  GET_CLASS(mob) = ch_class;
  GET_REAL_RACE(mob) = race;
  GET_REAL_SIZE(mob) = SIZE_MEDIUM;
  mob->points.size = SIZE_MEDIUM;
  GET_MOB_TIER(mob) = tier;
}

void Test_mob_tier_formula_v1_hit_point_vectors(CuTest *tc)
{
  int result = 0;

  CuAssertTrue(tc, mob_tier_calculate_hit_points(1496, MOB_TIER_STANDARD, &result));
  CuAssertIntEquals(tc, 1496, result);
  CuAssertTrue(tc, mob_tier_calculate_hit_points(1496, MOB_TIER_ELITE, &result));
  CuAssertIntEquals(tc, 2019, result);
  CuAssertTrue(tc, mob_tier_calculate_hit_points(1496, MOB_TIER_SMALL_GROUP, &result));
  CuAssertIntEquals(tc, 2618, result);
  CuAssertTrue(tc, mob_tier_calculate_hit_points(1496, MOB_TIER_BIG_GROUP, &result));
  CuAssertIntEquals(tc, 3366, result);
  CuAssertTrue(tc, mob_tier_calculate_hit_points(1496, MOB_TIER_RAID, &result));
  CuAssertIntEquals(tc, 4338, result);
  CuAssertTrue(tc, mob_tier_calculate_hit_points(1496, MOB_TIER_WORLD_BOSS, &result));
  CuAssertIntEquals(tc, 5610, result);
}

/* The level 34 world boss curve is anchored to the Prisoner (vnum 113750), whose
   hand-statted 30d1000+30000 averages 45015 hit points before db.c's powerful-being
   bump.  Tiers must also stay strictly ordered, world boss included. */
void Test_mob_tier_world_boss_matches_hand_statted_reference_boss(CuTest *tc)
{
  int previous = 0;
  int tier;
  struct char_data mob;

  for (tier = MOB_TIER_STANDARD; tier < NUM_MOB_TIERS; tier++)
  {
    int hit_points = 11968;
    int hitroll = 6;
    int armor_class = 440;
    int damage_bonus = 10;

    CuAssertTrue(tc, mob_tier_apply_autostat_bonuses(tier, &hit_points, &hitroll, &armor_class,
                                                     &damage_bonus));
    CuAssertTrue(tc, hit_points > previous);
    previous = hit_points;
  }
  CuAssertIntEquals(tc, 44880, previous);

  init_autoroll_mobile(&mob, 34, RACE_TYPE_HUMANOID, CLASS_WARRIOR, MOB_TIER_WORLD_BOSS);
  circle_srandom(12345);
  autoroll_mob(&mob, false, false);
  CuAssertIntEquals(tc, 44880, GET_MOVE(&mob));
  CuAssertIntEquals(tc, 16, GET_HITROLL(&mob));
  CuAssertIntEquals(tc, 15, GET_DAMROLL(&mob));
  CuAssertIntEquals(tc, 490, mob.points.armor);
  circle_srandom((unsigned long)time(NULL));
}

void Test_mob_tier_autostat_bonus_is_additive_and_saved_field_only(CuTest *tc)
{
  int hit_points = 11968;
  int hitroll = 6;
  int armor_class = 440;
  int damage_bonus = 10;

  CuAssertTrue(tc, mob_tier_apply_autostat_bonuses(MOB_TIER_ELITE, &hit_points, &hitroll,
                                                   &armor_class, &damage_bonus));
  CuAssertIntEquals(tc, 16156, hit_points);
  CuAssertIntEquals(tc, 8, hitroll);
  CuAssertIntEquals(tc, 450, armor_class);
  CuAssertIntEquals(tc, 11, damage_bonus);
}

void Test_mob_tier_standard_and_unspecified_are_strict_noops(CuTest *tc)
{
  int tier;

  for (tier = MOB_TIER_UNSPECIFIED; tier <= MOB_TIER_STANDARD; tier++)
  {
    int hit_points = 11968;
    int hitroll = 6;
    int armor_class = 440;
    int damage_bonus = 10;

    CuAssertTrue(tc, mob_tier_apply_autostat_bonuses(tier, &hit_points, &hitroll, &armor_class,
                                                     &damage_bonus));
    CuAssertIntEquals(tc, 11968, hit_points);
    CuAssertIntEquals(tc, 6, hitroll);
    CuAssertIntEquals(tc, 440, armor_class);
    CuAssertIntEquals(tc, 10, damage_bonus);
  }
}

void Test_mob_tier_formula_rejects_invalid_input_without_mutation(CuTest *tc)
{
  int hit_points = 100;
  int hitroll = 5;
  int armor_class = 200;
  int damage_bonus = 4;
  int result = 1234;

  CuAssertTrue(tc, !mob_tier_calculate_hit_points(0, MOB_TIER_STANDARD, &result));
  CuAssertTrue(tc, !mob_tier_calculate_hit_points(100, MOB_TIER_UNSPECIFIED, &result));
  CuAssertTrue(tc, !mob_tier_calculate_hit_points(100, NUM_MOB_TIERS, &result));
  CuAssertTrue(tc, !mob_tier_calculate_hit_points(INT_MAX, MOB_TIER_RAID, &result));
  CuAssertTrue(tc, !mob_tier_calculate_hit_points(100, MOB_TIER_STANDARD, NULL));
  CuAssertTrue(tc, !mob_tier_apply_autostat_bonuses(NUM_MOB_TIERS, &hit_points, &hitroll,
                                                    &armor_class, &damage_bonus));
  CuAssertIntEquals(tc, 100, hit_points);
  CuAssertIntEquals(tc, 5, hitroll);
  CuAssertIntEquals(tc, 200, armor_class);
  CuAssertIntEquals(tc, 4, damage_bonus);
  CuAssertIntEquals(tc, 1234, result);
  CuAssertStrEquals(tc, "Invalid", mob_tier_name(NUM_MOB_TIERS));
}

void Test_autoroll_mob_standard_restores_level_34_base(CuTest *tc)
{
  struct char_data mob;

  init_autoroll_mobile(&mob, 34, RACE_TYPE_HUMANOID, CLASS_WARRIOR, MOB_TIER_STANDARD);
  circle_srandom(12345);
  autoroll_mob(&mob, false, false);

  CuAssertIntEquals(tc, 1, GET_HIT(&mob));
  CuAssertTrue(tc, GET_PSP(&mob) >= 1 && GET_PSP(&mob) <= 34);
  CuAssertIntEquals(tc, 11968, GET_MOVE(&mob));
  CuAssertIntEquals(tc, 6, GET_HITROLL(&mob));
  CuAssertIntEquals(tc, 10, GET_DAMROLL(&mob));
  CuAssertIntEquals(tc, 440, mob.points.armor);
  CuAssertIntEquals(tc, 106700, GET_EXP(&mob));
  CuAssertIntEquals(tc, 540, GET_GOLD(&mob));
  circle_srandom((unsigned long)time(NULL));
}

void Test_autoroll_mob_unspecified_matches_standard_base(CuTest *tc)
{
  struct char_data standard;
  struct char_data unspecified;

  init_autoroll_mobile(&standard, 34, RACE_TYPE_HUMANOID, CLASS_WARRIOR, MOB_TIER_STANDARD);
  init_autoroll_mobile(&unspecified, 34, RACE_TYPE_HUMANOID, CLASS_WARRIOR, MOB_TIER_UNSPECIFIED);
  circle_srandom(54321);
  autoroll_mob(&standard, false, false);
  circle_srandom(54321);
  autoroll_mob(&unspecified, false, false);

  CuAssertIntEquals(tc, GET_HIT(&standard), GET_HIT(&unspecified));
  CuAssertIntEquals(tc, GET_PSP(&standard), GET_PSP(&unspecified));
  CuAssertIntEquals(tc, GET_MOVE(&standard), GET_MOVE(&unspecified));
  CuAssertIntEquals(tc, GET_HITROLL(&standard), GET_HITROLL(&unspecified));
  CuAssertIntEquals(tc, GET_DAMROLL(&standard), GET_DAMROLL(&unspecified));
  CuAssertIntEquals(tc, standard.points.armor, unspecified.points.armor);
  CuAssertIntEquals(tc, GET_EXP(&standard), GET_EXP(&unspecified));
  CuAssertIntEquals(tc, GET_GOLD(&standard), GET_GOLD(&unspecified));
  CuAssertIntEquals(tc, MOB_TIER_UNSPECIFIED, GET_MOB_TIER(&unspecified));
  circle_srandom((unsigned long)time(NULL));
}

void Test_autoroll_mob_tier_adds_to_complete_level_34_base(CuTest *tc)
{
  struct char_data mob;

  init_autoroll_mobile(&mob, 34, RACE_TYPE_HUMANOID, CLASS_WARRIOR, MOB_TIER_ELITE);
  circle_srandom(12345);
  autoroll_mob(&mob, false, false);

  CuAssertIntEquals(tc, 16156, GET_MOVE(&mob));
  CuAssertIntEquals(tc, 8, GET_HITROLL(&mob));
  CuAssertIntEquals(tc, 11, GET_DAMROLL(&mob));
  CuAssertIntEquals(tc, 450, mob.points.armor);
  CuAssertIntEquals(tc, 106700, GET_EXP(&mob));
  CuAssertIntEquals(tc, 540, GET_GOLD(&mob));
  circle_srandom((unsigned long)time(NULL));
}

void Test_changing_tier_after_autostat_does_not_change_stats(CuTest *tc)
{
  struct char_data mob;
  int hit_points;
  int hitroll;
  int damage_bonus;
  int armor_class;

  init_autoroll_mobile(&mob, 20, RACE_TYPE_HUMANOID, CLASS_WARRIOR, MOB_TIER_STANDARD);
  circle_srandom(12345);
  autoroll_mob(&mob, false, false);
  hit_points = GET_MOVE(&mob);
  hitroll = GET_HITROLL(&mob);
  damage_bonus = GET_DAMROLL(&mob);
  armor_class = mob.points.armor;

  GET_MOB_TIER(&mob) = MOB_TIER_RAID;

  CuAssertIntEquals(tc, hit_points, GET_MOVE(&mob));
  CuAssertIntEquals(tc, hitroll, GET_HITROLL(&mob));
  CuAssertIntEquals(tc, damage_bonus, GET_DAMROLL(&mob));
  CuAssertIntEquals(tc, armor_class, mob.points.armor);
  circle_srandom((unsigned long)time(NULL));
}

void Test_rerunning_autostat_after_tier_change_applies_new_bonus_once(CuTest *tc)
{
  struct char_data mob;

  init_autoroll_mobile(&mob, 20, RACE_TYPE_HUMANOID, CLASS_WARRIOR, MOB_TIER_STANDARD);
  circle_srandom(12345);
  autoroll_mob(&mob, false, false);
  CuAssertIntEquals(tc, 600, GET_MOVE(&mob));

  GET_MOB_TIER(&mob) = MOB_TIER_ELITE;
  circle_srandom(12345);
  autoroll_mob(&mob, false, false);
  CuAssertIntEquals(tc, 810, GET_MOVE(&mob));
  CuAssertIntEquals(tc, 6, GET_HITROLL(&mob));
  CuAssertIntEquals(tc, 5, GET_DAMROLL(&mob));
  CuAssertIntEquals(tc, 310, mob.points.armor);

  circle_srandom(12345);
  autoroll_mob(&mob, false, false);
  CuAssertIntEquals(tc, 810, GET_MOVE(&mob));
  circle_srandom((unsigned long)time(NULL));
}

void Test_autoroll_mob_preserves_base_owned_side_effects(CuTest *tc)
{
  struct char_data giant;
  struct char_data humanoid;

  init_autoroll_mobile(&giant, 20, RACE_TYPE_GIANT, CLASS_WARRIOR, MOB_TIER_STANDARD);
  GET_REAL_SIZE(&giant) = SIZE_SMALL;
  giant.points.size = SIZE_SMALL;
  autoroll_mob(&giant, false, false);
  CuAssertIntEquals(tc, SIZE_LARGE, GET_REAL_SIZE(&giant));
  CuAssertIntEquals(tc, SIZE_SMALL, GET_SIZE(&giant));

  init_autoroll_mobile(&humanoid, 20, RACE_TYPE_HUMANOID, CLASS_WARRIOR, MOB_TIER_STANDARD);
  humanoid.aff_abils.str_add = 17;
  GET_SPELL_RES(&humanoid) = 47;
  autoroll_mob(&humanoid, false, false);
  CuAssertIntEquals(tc, 17, humanoid.aff_abils.str_add);
  CuAssertIntEquals(tc, 47, GET_SPELL_RES(&humanoid));
}

void Test_mob_autoroll_profile_preserves_base_before_tier(CuTest *tc)
{
  struct mob_autoroll_config config;
  struct mob_autoroll_input input = {34, RACE_TYPE_HUMANOID, CLASS_WARRIOR, MOB_TIER_STANDARD,
                                     MOB_AUTOROLL_CUSTOM_NONE};
  struct mob_autoroll_result result;

  mob_autoroll_default_config(&config);
  CuAssertTrue(tc, mob_autoroll_calculate(&input, &config, &result));
  CuAssertIntEquals(tc, 11968, result.persisted.hit_points);
  CuAssertIntEquals(tc, 6, result.persisted.hitroll);
  CuAssertIntEquals(tc, 440, result.persisted.armor_class);
  CuAssertIntEquals(tc, 10, result.persisted.damage_bonus);
  CuAssertIntEquals(tc, 106700, result.persisted.experience);
  CuAssertIntEquals(tc, 540, result.persisted.gold);

  input.tier = MOB_TIER_ELITE;
  CuAssertTrue(tc, mob_autoroll_calculate(&input, &config, &result));
  CuAssertIntEquals(tc, 16156, result.persisted.hit_points);
  CuAssertIntEquals(tc, 8, result.persisted.hitroll);
  CuAssertIntEquals(tc, 450, result.persisted.armor_class);
  CuAssertIntEquals(tc, 11, result.persisted.damage_bonus);
}

void Test_mob_autoroll_profile_accepts_unspecified_and_rejects_invalid_inputs(CuTest *tc)
{
  struct mob_autoroll_config config;
  struct mob_autoroll_input input = {34, RACE_TYPE_HUMANOID, CLASS_WARRIOR, MOB_TIER_UNSPECIFIED,
                                     MOB_AUTOROLL_CUSTOM_NONE};
  struct mob_autoroll_result result;

  mob_autoroll_default_config(&config);
  CuAssertTrue(tc, mob_autoroll_calculate(&input, &config, &result));
  input.tier = NUM_MOB_TIERS;
  CuAssertTrue(tc, !mob_autoroll_calculate(&input, &config, &result));
  input.tier = MOB_TIER_STANDARD;
  input.level = 0;
  CuAssertTrue(tc, !mob_autoroll_calculate(&input, &config, &result));
  input.level = 34;
  input.race = NUM_RACE_TYPES;
  CuAssertTrue(tc, !mob_autoroll_calculate(&input, &config, &result));
}

void Test_mob_stat_categories_preserve_existing_sorcerer_and_bard_behavior(CuTest *tc)
{
  CuAssertIntEquals(tc, MOB_STAT_CATEGORY_WARRIOR, get_mob_stat_category(CLASS_SORCERER));
  CuAssertIntEquals(tc, MOB_STAT_CATEGORY_WARRIOR, get_mob_stat_category(CLASS_BARD));
  CuAssertIntEquals(tc, MOB_AUTOROLL_CATEGORY_WARRIOR, mob_autoroll_class_category(CLASS_SORCERER));
  CuAssertIntEquals(tc, MOB_AUTOROLL_CATEGORY_WARRIOR, mob_autoroll_class_category(CLASS_BARD));
}

void Test_mobile_spell_resistance_enhanced_field_loads_and_saves(CuTest *tc)
{
  struct char_data prototype;
  struct char_data *saved_mob_proto;
  struct index_data prototype_index;
  struct index_data *saved_mob_index;
  mob_rnum saved_top_of_mobt;
  FILE *output;
  char buffer[4096];
  size_t count;

  clear_char(&prototype);
  memset(&prototype_index, 0, sizeof(prototype_index));
  saved_mob_proto = mob_proto;
  saved_mob_index = mob_index;
  saved_top_of_mobt = top_of_mobt;
  mob_proto = &prototype;
  mob_index = &prototype_index;
  top_of_mobt = 0;
  prototype_index.vnum = 1234;
  prototype.player_specials = &dummy_mob;
  SET_BIT_AR(MOB_FLAGS(&prototype), MOB_ISNPC);

  test_interpret_mobile_espec("SpellRes", "47", 0, 1234);
  CuAssertIntEquals(tc, 47, GET_REAL_SPELL_RES(&prototype));
  test_interpret_mobile_espec("SpellRes", "101", 0, 1234);
  CuAssertIntEquals(tc, 100, GET_REAL_SPELL_RES(&prototype));

  output = tmpfile();
  CuAssertPtrNotNull(tc, output);
  CuAssertTrue(tc, write_mobile_espec(1234, &prototype, output));
  rewind(output);
  count = fread(buffer, 1, sizeof(buffer) - 1, output);
  buffer[count] = '\0';
  CuAssertPtrNotNull(tc, strstr(buffer, "SpellRes: 100\n"));
  fclose(output);

  mob_proto = saved_mob_proto;
  mob_index = saved_mob_index;
  top_of_mobt = saved_top_of_mobt;
}

void Test_mob_autoroll_full_supported_input_matrix_is_deterministic(CuTest *tc)
{
  struct mob_autoroll_config config;
  struct mob_autoroll_input input;
  struct mob_autoroll_result first;
  struct mob_autoroll_result second;

  mob_autoroll_default_config(&config);
  input.custom_profile = MOB_AUTOROLL_CUSTOM_NONE;
  for (input.level = 1; input.level <= LVL_IMPL; input.level++)
    for (input.race = 0; input.race < NUM_RACE_TYPES; input.race++)
      for (input.ch_class = 0; input.ch_class < NUM_CLASSES; input.ch_class++)
        for (input.tier = MOB_TIER_UNSPECIFIED; input.tier <= MOB_TIER_WORLD_BOSS; input.tier++)
        {
          CuAssertTrue(tc, mob_autoroll_calculate(&input, &config, &first));
          CuAssertTrue(tc, mob_autoroll_calculate(&input, &config, &second));
          CuAssertIntEquals(tc, 0, memcmp(&first, &second, sizeof(first)));
          CuAssertTrue(tc, first.persisted.hit_points > 0);
          CuAssertTrue(tc, first.persisted.damage_dice_count > 0);
          CuAssertTrue(tc, first.persisted.damage_dice_size >= 4);
        }
}

void Test_mob_autoroll_named_custom_profiles_keep_fixed_persisted_hit_points(CuTest *tc)
{
  struct mob_autoroll_config config;
  struct mob_autoroll_input input = {34, RACE_TYPE_DRAGON, CLASS_WARRIOR, MOB_TIER_WORLD_BOSS,
                                     MOB_AUTOROLL_CUSTOM_TIAMAT_LIVING_V1};
  struct mob_autoroll_result result;

  mob_autoroll_default_config(&config);
  CuAssertTrue(tc, mob_autoroll_calculate(&input, &config, &result));
  CuAssertIntEquals(tc, MOB_AUTOROLL_CUSTOM_TIAMAT_LIVING_V1, result.custom_profile);
  CuAssertIntEquals(tc, 29999, result.persisted.hit_points);

  input.race = RACE_TYPE_UNDEAD;
  input.custom_profile = MOB_AUTOROLL_CUSTOM_TIAMAT_DRACOLICH_V1;
  CuAssertTrue(tc, mob_autoroll_calculate(&input, &config, &result));
  CuAssertIntEquals(tc, MOB_AUTOROLL_CUSTOM_TIAMAT_DRACOLICH_V1, result.custom_profile);
  CuAssertIntEquals(tc, 30000, result.persisted.hit_points);
}
