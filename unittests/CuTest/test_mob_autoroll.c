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

void Test_mob_autoroll_full_profile_matches_authoritative_warrior_vector(CuTest *tc)
{
  struct mob_autoroll_config config;
  struct mob_autoroll_input input = {34, RACE_TYPE_HUMANOID, CLASS_WARRIOR, MOB_TIER_STANDARD,
                                     MOB_AUTOROLL_CUSTOM_NONE};
  struct mob_autoroll_result result;
  const struct mob_autoroll_stats *stats;

  mob_autoroll_default_config(&config);
  config.category[MOB_AUTOROLL_CATEGORY_WARRIOR].hit_points = 50;
  CuAssertTrue(tc, mob_autoroll_calculate(&input, &config, &result));
  CuAssertIntEquals(tc, MOB_AUTOROLL_PROFILE_V1, result.profile_version);
  CuAssertIntEquals(tc, MOB_AUTOROLL_CATEGORY_WARRIOR, result.category);
  stats = &result.persisted;
  CuAssertIntEquals(tc, 1496, stats->hit_points);
  CuAssertIntEquals(tc, 6, stats->hitroll);
  CuAssertIntEquals(tc, 440, stats->armor_class);
  CuAssertIntEquals(tc, 1, stats->damage_dice_count);
  CuAssertIntEquals(tc, 34, stats->damage_dice_size);
  CuAssertIntEquals(tc, 6, stats->damage_bonus);
  CuAssertIntEquals(tc, 86700, stats->experience);
  CuAssertIntEquals(tc, 340, stats->gold);
  CuAssertIntEquals(tc, 27, stats->strength);
  CuAssertIntEquals(tc, 27, stats->constitution);
  CuAssertIntEquals(tc, 10, stats->dexterity);
  CuAssertIntEquals(tc, 8, stats->saving_fortitude);
}

void Test_mob_autoroll_profile_rejects_unspecified_and_invalid_inputs(CuTest *tc)
{
  struct mob_autoroll_config config;
  struct mob_autoroll_input input = {34, RACE_TYPE_HUMANOID, CLASS_WARRIOR, MOB_TIER_UNSPECIFIED,
                                     MOB_AUTOROLL_CUSTOM_NONE};
  struct mob_autoroll_result result;

  mob_autoroll_default_config(&config);
  CuAssertTrue(tc, !mob_autoroll_calculate(&input, &config, &result));
  input.tier = MOB_TIER_STANDARD;
  input.level = 0;
  CuAssertTrue(tc, !mob_autoroll_calculate(&input, &config, &result));
  input.level = 34;
  input.race = NUM_RACE_TYPES;
  CuAssertTrue(tc, !mob_autoroll_calculate(&input, &config, &result));
  input.race = RACE_TYPE_HUMANOID;
  input.custom_profile = NUM_MOB_AUTOROLL_CUSTOM_PROFILES;
  CuAssertTrue(tc, !mob_autoroll_calculate(&input, &config, &result));
  input.custom_profile = MOB_AUTOROLL_CUSTOM_NONE;
  config.version++;
  CuAssertTrue(tc, !mob_autoroll_calculate(&input, &config, &result));
}

void Test_mob_autoroll_nondefault_category_config_is_post_load_only(CuTest *tc)
{
  struct mob_autoroll_config config;
  struct mob_autoroll_input input = {20, RACE_TYPE_HUMANOID, CLASS_SORCERER, MOB_TIER_ELITE,
                                     MOB_AUTOROLL_CUSTOM_NONE};
  struct mob_autoroll_result result;
  struct mob_autoroll_category_config *arcane;

  mob_autoroll_default_config(&config);
  arcane = &config.category[MOB_AUTOROLL_CATEGORY_ARCANE];
  arcane->hit_points = 125;
  arcane->armor_class = 90;
  arcane->attack_bonus = 150;
  arcane->damage_bonus = 175;
  arcane->saving_throws = 80;
  arcane->ability_scores = 120;
  arcane->gold = 50;

  CuAssertTrue(tc, mob_autoroll_calculate(&input, &config, &result));
  CuAssertIntEquals(tc, MOB_AUTOROLL_CATEGORY_ARCANE, result.category);
  CuAssertTrue(tc, result.persisted.hit_points != result.expected_post_load.hit_points);
  CuAssertIntEquals(tc, result.persisted.hit_points * 125 / 100,
                    result.expected_post_load.hit_points);
  CuAssertIntEquals(tc, result.persisted.hitroll * 150 / 100, result.expected_post_load.hitroll);
  CuAssertIntEquals(tc, result.persisted.gold / 2, result.expected_post_load.gold);
  CuAssertIntEquals(tc, MOB_STAT_CATEGORY_ARCANE, get_mob_stat_category(CLASS_SORCERER));
  CuAssertIntEquals(tc, MOB_STAT_CATEGORY_ARCANE, get_mob_stat_category(CLASS_BARD));
}

void Test_autoroll_mob_never_reads_or_changes_identity_owned_size(CuTest *tc)
{
  struct char_data mob;

  clear_char(&mob);
  SET_BIT_AR(MOB_FLAGS(&mob), MOB_ISNPC);
  GET_LEVEL(&mob) = 20;
  GET_CLASS(&mob) = CLASS_WARRIOR;
  GET_REAL_RACE(&mob) = RACE_TYPE_GIANT;
  GET_REAL_SIZE(&mob) = SIZE_SMALL;
  mob.points.size = SIZE_SMALL;
  GET_MOB_TIER(&mob) = MOB_TIER_STANDARD;

  autoroll_mob(&mob, false, false);

  CuAssertIntEquals(tc, SIZE_SMALL, GET_REAL_SIZE(&mob));
  CuAssertIntEquals(tc, SIZE_SMALL, GET_SIZE(&mob));
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
        for (input.tier = MOB_TIER_STANDARD; input.tier <= MOB_TIER_WORLD_BOSS; input.tier++)
        {
          CuAssertTrue(tc, mob_autoroll_calculate(&input, &config, &first));
          CuAssertTrue(tc, mob_autoroll_calculate(&input, &config, &second));
          CuAssertIntEquals(tc, 0, memcmp(&first, &second, sizeof(first)));
          CuAssertTrue(tc, first.persisted.hit_points > 0);
          CuAssertTrue(tc, first.persisted.damage_dice_count > 0);
          CuAssertTrue(tc, first.persisted.damage_dice_size >= 4);
          CuAssertTrue(tc, first.persisted.spell_resistance >= 0);
        }
}

void Test_mob_autoroll_named_custom_profiles_own_fixed_hit_points(CuTest *tc)
{
  struct mob_autoroll_config config;
  struct mob_autoroll_input input = {34, RACE_TYPE_DRAGON, CLASS_WARRIOR, MOB_TIER_WORLD_BOSS,
                                     MOB_AUTOROLL_CUSTOM_TIAMAT_LIVING_V1};
  struct mob_autoroll_result result;

  mob_autoroll_default_config(&config);
  CuAssertTrue(tc, mob_autoroll_calculate(&input, &config, &result));
  CuAssertIntEquals(tc, MOB_AUTOROLL_CUSTOM_TIAMAT_LIVING_V1, result.custom_profile);
  CuAssertIntEquals(tc, 29999, result.persisted.hit_points);
  CuAssertIntEquals(tc, 29999, result.expected_post_load.hit_points);

  input.race = RACE_TYPE_UNDEAD;
  input.custom_profile = MOB_AUTOROLL_CUSTOM_TIAMAT_DRACOLICH_V1;
  CuAssertTrue(tc, mob_autoroll_calculate(&input, &config, &result));
  CuAssertIntEquals(tc, MOB_AUTOROLL_CUSTOM_TIAMAT_DRACOLICH_V1, result.custom_profile);
  CuAssertIntEquals(tc, 30000, result.persisted.hit_points);
  CuAssertIntEquals(tc, 30000, result.expected_post_load.hit_points);
}
