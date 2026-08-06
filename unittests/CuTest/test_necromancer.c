#include "CuTest.h"

#include "../../src/conf.h"
#include "../../src/sysdep.h"
#include "../../src/structs.h"
#include "../../src/utils.h"
#include "../../src/db.h"
#include "../../src/character/feats.h"
#include "../../src/combat/assign_wpn_armor.h"
#include "../../src/craft/craft.h"

#include <stdlib.h>
#include <string.h>

static void setup_necromancer_character(struct char_data *ch,
                                        struct player_special_data *player_specials)
{
  clear_char(ch);
  memset(player_specials, 0, sizeof(*player_specials));
  ch->player_specials = player_specials;
  GET_CLASS(ch) = CLASS_NECROMANCER;
  GET_LEVEL(ch) = 12;
  CLASS_LEVEL(ch, CLASS_NECROMANCER) = 3;
}

void Test_necromancer_arcane_progression_advances_only_preferred_class(CuTest *tc)
{
  struct char_data ch;
  struct player_special_data player_specials;

  setup_necromancer_character(&ch, &player_specials);
  CLASS_LEVEL((&ch), CLASS_WIZARD) = 5;
  CLASS_LEVEL((&ch), CLASS_SORCERER) = 5;
  CLASS_LEVEL((&ch), CLASS_CLERIC) = 5;
  NECROMANCER_CAST_TYPE((&ch)) = CASTING_TYPE_ARCANE;
  GET_PREFERRED_ARCANE((&ch)) = CLASS_SORCERER;
  GET_PREFERRED_DIVINE((&ch)) = CLASS_CLERIC;

  CuAssertIntEquals(tc, 0, compute_bonus_caster_level(&ch, CLASS_WIZARD));
  CuAssertIntEquals(tc, 3, compute_bonus_caster_level(&ch, CLASS_SORCERER));
  CuAssertIntEquals(tc, 0, compute_bonus_caster_level(&ch, CLASS_CLERIC));
}

void Test_necromancer_divine_progression_advances_only_preferred_class(CuTest *tc)
{
  struct char_data ch;
  struct player_special_data player_specials;

  setup_necromancer_character(&ch, &player_specials);
  CLASS_LEVEL((&ch), CLASS_CLERIC) = 5;
  CLASS_LEVEL((&ch), CLASS_DRUID) = 5;
  CLASS_LEVEL((&ch), CLASS_PALADIN) = 16;
  CLASS_LEVEL((&ch), CLASS_RANGER) = 16;
  NECROMANCER_CAST_TYPE((&ch)) = CASTING_TYPE_DIVINE;
  GET_PREFERRED_DIVINE((&ch)) = CLASS_RANGER;

  CuAssertIntEquals(tc, 0, compute_bonus_caster_level(&ch, CLASS_CLERIC));
  CuAssertIntEquals(tc, 0, compute_bonus_caster_level(&ch, CLASS_DRUID));
  CuAssertIntEquals(tc, 0, compute_bonus_caster_level(&ch, CLASS_PALADIN));
  CuAssertIntEquals(tc, 3, compute_bonus_caster_level(&ch, CLASS_RANGER));
}

void Test_necromancer_unselected_progression_advances_no_class(CuTest *tc)
{
  struct char_data ch;
  struct player_special_data player_specials;

  setup_necromancer_character(&ch, &player_specials);
  CLASS_LEVEL((&ch), CLASS_WIZARD) = 5;
  CLASS_LEVEL((&ch), CLASS_CLERIC) = 5;
  NECROMANCER_CAST_TYPE((&ch)) = CASTING_TYPE_NONE;
  GET_PREFERRED_ARCANE((&ch)) = CLASS_WIZARD;
  GET_PREFERRED_DIVINE((&ch)) = CLASS_CLERIC;

  CuAssertIntEquals(tc, 0, compute_bonus_caster_level(&ch, CLASS_WIZARD));
  CuAssertIntEquals(tc, 0, compute_bonus_caster_level(&ch, CLASS_CLERIC));
}

void Test_necromancer_progression_infers_a_single_base_class(CuTest *tc)
{
  struct char_data ch;
  struct player_special_data player_specials;

  setup_necromancer_character(&ch, &player_specials);
  CLASS_LEVEL((&ch), CLASS_SORCERER) = 5;
  NECROMANCER_CAST_TYPE((&ch)) = CASTING_TYPE_ARCANE;
  GET_PREFERRED_ARCANE((&ch)) = CLASS_WIZARD;

  CuAssertIntEquals(tc, CLASS_SORCERER,
                    get_necromancer_progression_class(&ch, CASTING_TYPE_ARCANE));
  CuAssertIntEquals(tc, 3, compute_bonus_caster_level(&ch, CLASS_SORCERER));
}

void Test_necromancer_progression_requires_preference_for_multiple_base_classes(CuTest *tc)
{
  struct char_data ch;
  struct player_special_data player_specials;

  setup_necromancer_character(&ch, &player_specials);
  CLASS_LEVEL((&ch), CLASS_WIZARD) = 5;
  CLASS_LEVEL((&ch), CLASS_SORCERER) = 5;
  NECROMANCER_CAST_TYPE((&ch)) = CASTING_TYPE_ARCANE;
  GET_PREFERRED_ARCANE((&ch)) = CLASS_UNDEFINED;

  CuAssertIntEquals(tc, CLASS_UNDEFINED,
                    get_necromancer_progression_class(&ch, CASTING_TYPE_ARCANE));
  CuAssertIntEquals(tc, 0, compute_bonus_caster_level(&ch, CLASS_WIZARD));
  CuAssertIntEquals(tc, 0, compute_bonus_caster_level(&ch, CLASS_SORCERER));
}

void Test_necromancer_pending_arcane_choice_enables_known_spell_study(CuTest *tc)
{
  struct char_data ch;
  struct player_special_data player_specials;
  struct level_data levelup;

  setup_necromancer_character(&ch, &player_specials);
  memset(&levelup, 0, sizeof(levelup));
  LEVELUP((&ch)) = &levelup;
  LEVELUP((&ch))->class = CLASS_NECROMANCER;
  LEVELUP((&ch))->necromancer_bonus_levels = CASTING_TYPE_ARCANE;
  NECROMANCER_CAST_TYPE((&ch)) = CASTING_TYPE_NONE;
  GET_PREFERRED_ARCANE((&ch)) = CLASS_SORCERER;
  CLASS_LEVEL((&ch), CLASS_SORCERER) = 5;

  CuAssertTrue(tc, can_study_known_spells(&ch));
}

void Test_necromancer_pending_divine_choice_enables_known_spell_study(CuTest *tc)
{
  struct char_data ch;
  struct player_special_data player_specials;
  struct level_data levelup;

  setup_necromancer_character(&ch, &player_specials);
  memset(&levelup, 0, sizeof(levelup));
  LEVELUP((&ch)) = &levelup;
  LEVELUP((&ch))->class = CLASS_NECROMANCER;
  LEVELUP((&ch))->necromancer_bonus_levels = CASTING_TYPE_DIVINE;
  NECROMANCER_CAST_TYPE((&ch)) = CASTING_TYPE_NONE;
  GET_PREFERRED_DIVINE((&ch)) = CLASS_INQUISITOR;
  CLASS_LEVEL((&ch), CLASS_INQUISITOR) = 5;

  CuAssertTrue(tc, can_study_known_spells(&ch));
}

void Test_necromancer_bone_armor_requires_exactly_one_kit_object(CuTest *tc)
{
  struct obj_data kit;
  struct obj_data first;
  struct obj_data second;
  int num_objs = -1;

  clear_object(&kit);
  clear_object(&first);
  clear_object(&second);

  CuAssertPtrEquals(tc, NULL, test_get_single_bone_armor_object(&kit, &num_objs));
  CuAssertIntEquals(tc, 0, num_objs);

  kit.contains = &first;
  CuAssertPtrEquals(tc, &first, test_get_single_bone_armor_object(&kit, &num_objs));
  CuAssertIntEquals(tc, 1, num_objs);

  first.next_content = &second;
  CuAssertPtrEquals(tc, NULL, test_get_single_bone_armor_object(&kit, &num_objs));
  CuAssertIntEquals(tc, 2, num_objs);
}

void Test_necromancer_bone_armor_replaces_owned_descriptions_once(CuTest *tc)
{
  struct obj_data armor;
  char description[] = "a polished bone breastplate";

  clear_object(&armor);
  armor.name = strdup("old armor keywords");
  armor.short_description = strdup("an old breastplate");
  armor.description = strdup("An old breastplate lies here.");

  test_update_bone_armor_descriptions(&armor, description);

  CuAssertStrEquals(tc, "a polished bone breastplate", armor.name);
  CuAssertStrEquals(tc, "a polished bone breastplate", armor.short_description);
  CuAssertStrEquals(tc, "A polished bone breastplate lies here.", armor.description);

  free(armor.name);
  free(armor.short_description);
  free(armor.description);
}

static void setup_necromancer_test_armor(struct obj_data *armor, int armor_type, int material)
{
  clear_object(armor);
  GET_OBJ_TYPE(armor) = ITEM_ARMOR;
  GET_OBJ_VAL(armor, 1) = armor_type;
  GET_OBJ_MATERIAL(armor) = material;
}

void Test_necromancer_bone_armor_mixed_materials_receive_no_reduction(CuTest *tc)
{
  struct char_data ch;
  struct player_special_data player_specials;
  struct obj_data body;
  struct obj_data head;
  int saved_body_spell_failure;
  int saved_head_spell_failure;
  int spell_failure;

  clear_char(&ch);
  memset(&player_specials, 0, sizeof(player_specials));
  ch.player_specials = &player_specials;
  setup_necromancer_test_armor(&body, SPEC_ARMOR_TYPE_PADDED, MATERIAL_IRON);
  setup_necromancer_test_armor(&head, SPEC_ARMOR_TYPE_PADDED_HEAD, MATERIAL_BONE);
  GET_EQ(&ch, WEAR_BODY) = &body;
  GET_EQ(&ch, WEAR_HEAD) = &head;
  SET_FEAT(&ch, FEAT_BONE_ARMOR, 1);

  saved_body_spell_failure = armor_list[SPEC_ARMOR_TYPE_PADDED].spellFail;
  saved_head_spell_failure = armor_list[SPEC_ARMOR_TYPE_PADDED_HEAD].spellFail;
  armor_list[SPEC_ARMOR_TYPE_PADDED].spellFail = 30;
  armor_list[SPEC_ARMOR_TYPE_PADDED_HEAD].spellFail = 30;

  spell_failure = compute_gear_spell_failure(&ch);

  armor_list[SPEC_ARMOR_TYPE_PADDED].spellFail = saved_body_spell_failure;
  armor_list[SPEC_ARMOR_TYPE_PADDED_HEAD].spellFail = saved_head_spell_failure;

  CuAssertIntEquals(tc, 30, spell_failure);
}

void Test_necromancer_bone_armor_all_bone_applies_each_real_rank(CuTest *tc)
{
  struct char_data ch;
  struct player_special_data player_specials;
  struct obj_data body;
  struct obj_data head;
  int saved_body_spell_failure;
  int saved_head_spell_failure;
  int spell_failure;

  clear_char(&ch);
  memset(&player_specials, 0, sizeof(player_specials));
  ch.player_specials = &player_specials;
  setup_necromancer_test_armor(&body, SPEC_ARMOR_TYPE_PADDED, MATERIAL_BONE);
  setup_necromancer_test_armor(&head, SPEC_ARMOR_TYPE_PADDED_HEAD, MATERIAL_BONE);
  GET_EQ(&ch, WEAR_BODY) = &body;
  GET_EQ(&ch, WEAR_HEAD) = &head;
  SET_FEAT(&ch, FEAT_BONE_ARMOR, 2);

  saved_body_spell_failure = armor_list[SPEC_ARMOR_TYPE_PADDED].spellFail;
  saved_head_spell_failure = armor_list[SPEC_ARMOR_TYPE_PADDED_HEAD].spellFail;
  armor_list[SPEC_ARMOR_TYPE_PADDED].spellFail = 30;
  armor_list[SPEC_ARMOR_TYPE_PADDED_HEAD].spellFail = 30;

  spell_failure = compute_gear_spell_failure(&ch);

  armor_list[SPEC_ARMOR_TYPE_PADDED].spellFail = saved_body_spell_failure;
  armor_list[SPEC_ARMOR_TYPE_PADDED_HEAD].spellFail = saved_head_spell_failure;

  CuAssertIntEquals(tc, 10, spell_failure);
}
