#include "CuTest.h"

#include "../../src/conf.h"
#include "../../src/sysdep.h"
#include "../../src/structs.h"
#include "../../src/utils.h"
#include "../../src/act.h"
#include "../../src/actions.h"
#include "../../src/actionqueues.h"
#include "../../src/db.h"
#include "../../src/handler.h"
#include "../../src/interpreter.h"
#include "../../src/lists.h"
#include "../../src/mud_event.h"
#include "../../src/dgscript/dg_event.h"
#include "../../src/magic/spells.h"
#include "../../src/character/class.h"
#include "../../src/character/evolutions.h"
#include "../../src/character/feats.h"
#include "../../src/combat/assign_wpn_armor.h"
#include "../../src/craft/craft.h"
#include "../../src/net/protocol.h"

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

void Test_necromancer_touch_daily_uses_scale_at_class_breakpoints(CuTest *tc)
{
  struct char_data ch;
  struct player_special_data player_specials;

  setup_necromancer_character(&ch, &player_specials);

  CLASS_LEVEL((&ch), CLASS_NECROMANCER) = 6;
  CuAssertIntEquals(tc, 1, get_daily_uses(&ch, FEAT_TOUCH_OF_UNDEATH));
  CLASS_LEVEL((&ch), CLASS_NECROMANCER) = 8;
  CuAssertIntEquals(tc, 2, get_daily_uses(&ch, FEAT_TOUCH_OF_UNDEATH));
  CLASS_LEVEL((&ch), CLASS_NECROMANCER) = 10;
  CuAssertIntEquals(tc, 3, get_daily_uses(&ch, FEAT_TOUCH_OF_UNDEATH));
}

void Test_necromancer_touch_level_follows_selected_casting_track(CuTest *tc)
{
  struct char_data ch;
  struct player_special_data player_specials;

  setup_necromancer_character(&ch, &player_specials);
  CLASS_LEVEL((&ch), CLASS_NECROMANCER) = 6;
  CLASS_LEVEL((&ch), CLASS_WIZARD) = 5;
  CLASS_LEVEL((&ch), CLASS_SORCERER) = 3;
  CLASS_LEVEL((&ch), CLASS_CLERIC) = 9;
  CLASS_LEVEL((&ch), CLASS_DRUID) = 4;

  NECROMANCER_CAST_TYPE((&ch)) = CASTING_TYPE_ARCANE;
  GET_PREFERRED_ARCANE((&ch)) = CLASS_WIZARD;
  CuAssertIntEquals(tc, 11, test_necromancer_touch_level(&ch));
  NECROMANCER_CAST_TYPE((&ch)) = CASTING_TYPE_DIVINE;
  GET_PREFERRED_DIVINE((&ch)) = CLASS_CLERIC;
  CuAssertIntEquals(tc, 15, test_necromancer_touch_level(&ch));
  NECROMANCER_CAST_TYPE((&ch)) = CASTING_TYPE_NONE;
  CuAssertIntEquals(tc, 6, test_necromancer_touch_level(&ch));
  CuAssertIntEquals(tc, CAST_INNATE, test_necromancer_touch_cast_type());
}

void Test_necromancer_touch_affects_keep_selected_progression_level(CuTest *tc)
{
  struct char_data ch;
  struct player_special_data player_specials;

  setup_necromancer_character(&ch, &player_specials);
  GET_LEVEL((&ch)) = 30;

  CuAssertIntEquals(
      tc, 11, test_resolve_affect_cast_level(&ch, ABILITY_PARALYZING_TOUCH, 11, 13, CAST_INNATE));
  CuAssertIntEquals(
      tc, 11, test_resolve_affect_cast_level(&ch, ABILITY_WEAKENING_TOUCH, 11, 13, CAST_INNATE));
  CuAssertIntEquals(
      tc, 11, test_resolve_affect_cast_level(&ch, ABILITY_DEGENERATIVE_TOUCH, 11, 13, CAST_INNATE));
  CuAssertIntEquals(
      tc, 11, test_resolve_affect_cast_level(&ch, ABILITY_DESTRUCTIVE_TOUCH, 11, 13, CAST_INNATE));
  CuAssertIntEquals(
      tc, 11, test_resolve_affect_cast_level(&ch, ABILITY_DEATHLESS_TOUCH, 11, 13, CAST_INNATE));
  CuAssertIntEquals(
      tc, 30, test_resolve_affect_cast_level(&ch, SPELL_HIDEOUS_LAUGHTER, 11, 13, CAST_INNATE));
  CuAssertIntEquals(
      tc, 13, test_resolve_affect_cast_level(&ch, ABILITY_WEAKENING_TOUCH, 11, 13, CAST_SPELL));
}

void Test_necromancer_paralyzing_touch_duration_is_one_d_four_plus_one(CuTest *tc)
{
  int duration;
  int roll;
  bool valid = true;

  for (roll = 0; roll < 256; roll++)
  {
    duration = test_paralyzing_touch_duration();
    if (duration < 2 || duration > 5)
      valid = false;
  }

  CuAssertTrue(tc, valid);
}

void Test_necromancer_touch_attempt_spends_own_use_and_swift_action(CuTest *tc)
{
  struct char_data ch;
  struct player_special_data player_specials;
  struct action_data *action;
  struct list_data list_registry;
  struct list_data *saved_global_lists;
  event_id saved_touch_event;
  bool has_touch_event;
  bool has_corruption_event;
  bool has_swift_event;
  bool swift_available;
  int remaining_uses;
  int queued_actions;

  setup_necromancer_character(&ch, &player_specials);
  CLASS_LEVEL((&ch), CLASS_NECROMANCER) = 6;
  SET_FEAT(&ch, FEAT_TOUCH_OF_UNDEATH, 1);
  GET_QUEUE(&ch) = create_action_queue();

  memset(&list_registry, 0, sizeof(list_registry));
  saved_global_lists = global_lists;
  if (global_lists == NULL)
    global_lists = &list_registry;
  saved_touch_event = feat_list[FEAT_TOUCH_OF_UNDEATH].event;
  feat_list[FEAT_TOUCH_OF_UNDEATH].event = eTOUCHOFUNDEATH;
  event_free_all();
  event_init();

  test_consume_necromancer_touch_attempt(&ch);
  has_touch_event = char_has_mud_event(&ch, eTOUCHOFUNDEATH) != NULL;
  has_corruption_event = char_has_mud_event(&ch, eTOUCHOFCORRUPTION) != NULL;
  has_swift_event = char_has_mud_event(&ch, eSWIFTACTION) != NULL;
  swift_available = command_actions_available(&ch, ACTION_SWIFT);
  remaining_uses = daily_uses_remaining(&ch, FEAT_TOUCH_OF_UNDEATH);

  action = calloc(1, sizeof(*action));
  action->argument = strdup("look");
  action->actions_required = ACTION_SWIFT;
  enqueue_action(GET_QUEUE(&ch), action);
  execute_next_action(&ch);
  queued_actions = pending_actions(&ch);

  clear_char_event_list(&ch);
  event_free_all();
  free_action_queue(GET_QUEUE(&ch));
  GET_QUEUE(&ch) = NULL;
  feat_list[FEAT_TOUCH_OF_UNDEATH].event = saved_touch_event;
  global_lists = saved_global_lists;

  CuAssertTrue(tc, has_touch_event);
  CuAssertTrue(tc, !has_corruption_event);
  CuAssertTrue(tc, has_swift_event);
  CuAssertTrue(tc, !swift_available);
  CuAssertIntEquals(tc, 0, remaining_uses);
  CuAssertIntEquals(tc, 1, queued_actions);
}

void Test_necromancer_touch_command_declares_swift_action(CuTest *tc)
{
  bool created_command_list = false;
  int actions_required = ACTION_NONE;
  int undeath_command;

  if (complete_cmd_info == NULL)
  {
    create_command_list();
    created_command_list = true;
  }

  undeath_command = find_command("undeath");
  if (undeath_command >= 0)
    actions_required = complete_cmd_info[undeath_command].actions_required;

  if (created_command_list)
    free_command_list();

  CuAssertTrue(tc, undeath_command >= 0);
  CuAssertIntEquals(tc, ACTION_SWIFT, actions_required);
}

void Test_action_queue_execution_consumes_the_dequeued_action(CuTest *tc)
{
  struct char_data ch;
  struct player_special_data player_specials;
  struct action_data *action;
  bool created_command_list = false;

  setup_necromancer_character(&ch, &player_specials);
  GET_POS(&ch) = POS_STANDING;
  GET_QUEUE(&ch) = create_action_queue();
  if (complete_cmd_info == NULL)
  {
    create_command_list();
    created_command_list = true;
  }

  action = calloc(1, sizeof(*action));
  action->argument = strdup("abort");
  action->actions_required = ACTION_NONE;
  enqueue_action(GET_QUEUE(&ch), action);
  execute_next_action(&ch);

  CuAssertIntEquals(tc, 0, pending_actions(&ch));

  free_action_queue(GET_QUEUE(&ch));
  GET_QUEUE(&ch) = NULL;
  if (created_command_list)
    free_command_list();
}

void Test_necromancer_at_will_summons_skip_prepared_resource_extraction(CuTest *tc)
{
  struct char_data ch;
  struct player_special_data player_specials;

  setup_necromancer_character(&ch, &player_specials);
  SET_FEAT(&ch, FEAT_SUMMON_UNDEAD, 1);
  SET_FEAT(&ch, FEAT_SUMMON_GREATER_UNDEAD, 1);

  CuAssertTrue(tc, !test_should_extract_prepared_spell(&ch, SPELL_ANIMATE_DEAD));
  CuAssertTrue(tc, !test_should_extract_prepared_spell(&ch, SPELL_GREATER_ANIMATION));

  SET_FEAT(&ch, FEAT_SUMMON_UNDEAD, 0);
  SET_FEAT(&ch, FEAT_SUMMON_GREATER_UNDEAD, 0);
  CuAssertTrue(tc, test_should_extract_prepared_spell(&ch, SPELL_ANIMATE_DEAD));
  CuAssertTrue(tc, test_should_extract_prepared_spell(&ch, SPELL_GREATER_ANIMATION));
}

void Test_necromancer_summon_level_uses_selected_progression(CuTest *tc)
{
  struct char_data ch;
  struct player_special_data player_specials;

  setup_necromancer_character(&ch, &player_specials);
  CLASS_LEVEL((&ch), CLASS_NECROMANCER) = 9;
  CLASS_LEVEL((&ch), CLASS_WIZARD) = 20;
  CLASS_LEVEL((&ch), CLASS_SORCERER) = 5;
  CLASS_LEVEL((&ch), CLASS_CLERIC) = 4;
  CLASS_LEVEL((&ch), CLASS_DRUID) = 7;
  SET_FEAT(&ch, FEAT_SUMMON_UNDEAD, 1);

  NECROMANCER_CAST_TYPE((&ch)) = CASTING_TYPE_ARCANE;
  GET_PREFERRED_ARCANE((&ch)) = CLASS_SORCERER;
  CuAssertIntEquals(tc, CLASS_WIZARD, test_at_will_casting_class(&ch, SPELL_ANIMATE_DEAD));
  CuAssertIntEquals(tc, 14, test_necromancer_summon_caster_level(&ch, SPELL_ANIMATE_DEAD));

  NECROMANCER_CAST_TYPE((&ch)) = CASTING_TYPE_DIVINE;
  GET_PREFERRED_DIVINE((&ch)) = CLASS_DRUID;
  CuAssertIntEquals(tc, CLASS_CLERIC, test_at_will_casting_class(&ch, SPELL_ANIMATE_DEAD));
  CuAssertIntEquals(tc, 16, test_necromancer_summon_caster_level(&ch, SPELL_ANIMATE_DEAD));

  NECROMANCER_CAST_TYPE((&ch)) = CASTING_TYPE_NONE;
  CuAssertIntEquals(tc, CLASS_WIZARD, test_at_will_casting_class(&ch, SPELL_ANIMATE_DEAD));
  CuAssertIntEquals(tc, 9, test_necromancer_summon_caster_level(&ch, SPELL_ANIMATE_DEAD));
}

static void setup_animated_dead_follower(struct char_data *pet, struct follow_type *link,
                                         struct char_data *master, struct follow_type *next)
{
  clear_char(pet);
  memset(link, 0, sizeof(*link));
  SET_BIT_AR(MOB_FLAGS(pet), MOB_ISNPC);
  SET_BIT_AR(MOB_FLAGS(pet), MOB_ANIMATED_DEAD);
  SET_BIT_AR(AFF_FLAGS(pet), AFF_CHARM);
  pet->master = master;
  link->follower = pet;
  link->next = next;
}

void Test_necromancer_animated_undead_admission_has_exact_boundaries(CuTest *tc)
{
  struct char_data ch;
  struct char_data first_pet;
  struct char_data second_pet;
  struct player_special_data player_specials;
  struct follow_type first_link;
  struct follow_type second_link;

  setup_necromancer_character(&ch, &player_specials);
  setup_animated_dead_follower(&first_pet, &first_link, &ch, NULL);
  setup_animated_dead_follower(&second_pet, &second_link, &ch, &first_link);

  ch.followers = NULL;
  CLASS_LEVEL((&ch), CLASS_NECROMANCER) = 0;
  CuAssertTrue(tc, can_add_follower_by_flag(&ch, MOB_ANIMATED_DEAD));
  ch.followers = &first_link;
  CuAssertTrue(tc, !can_add_follower_by_flag(&ch, MOB_ANIMATED_DEAD));

  CLASS_LEVEL((&ch), CLASS_NECROMANCER) = 2;
  CuAssertTrue(tc, can_add_follower_by_flag(&ch, MOB_ANIMATED_DEAD));
  ch.followers = &second_link;
  CuAssertTrue(tc, !can_add_follower_by_flag(&ch, MOB_ANIMATED_DEAD));
}

void Test_necromancer_animated_undead_tiers_use_supplied_caster_level(CuTest *tc)
{
  CuAssertIntEquals(tc, MOB_ZOMBIE, animated_dead_summon_mob(SPELL_ANIMATE_DEAD, 9));
  CuAssertIntEquals(tc, MOB_GHOUL, animated_dead_summon_mob(SPELL_ANIMATE_DEAD, 10));
  CuAssertIntEquals(tc, MOB_GIANT_SKELETON, animated_dead_summon_mob(SPELL_ANIMATE_DEAD, 20));
  CuAssertIntEquals(tc, MOB_MUMMY, animated_dead_summon_mob(SPELL_ANIMATE_DEAD, 30));

  CuAssertIntEquals(tc, MOB_GHOST, animated_dead_summon_mob(SPELL_GREATER_ANIMATION, 19));
  CuAssertIntEquals(tc, MOB_SPECTRE, animated_dead_summon_mob(SPELL_GREATER_ANIMATION, 20));
  CuAssertIntEquals(tc, MOB_BANSHEE, animated_dead_summon_mob(SPELL_GREATER_ANIMATION, 25));
  CuAssertIntEquals(tc, MOB_WIGHT, animated_dead_summon_mob(SPELL_GREATER_ANIMATION, 30));

  CuAssertTrue(tc, summon_spell_rejects_holy_room(SPELL_ANIMATE_DEAD));
  CuAssertTrue(tc, summon_spell_rejects_holy_room(SPELL_GREATER_ANIMATION));
}

void Test_necromancer_greater_animation_preserves_scaled_mob_level(CuTest *tc)
{
  int caster_levels[] = {19, 20, 25, 30};
  int minimum_levels[] = {15, 19, 23, 27};
  int caster_index;
  int mob_level;
  int roll;
  bool valid = true;

  for (caster_index = 0; caster_index < 4; caster_index++)
  {
    for (roll = 0; roll < 128; roll++)
    {
      mob_level = summon_spell_mob_level(SPELL_GREATER_ANIMATION, caster_levels[caster_index]);
      if (mob_level < minimum_levels[caster_index] || mob_level > caster_levels[caster_index])
        valid = false;
    }
  }

  CuAssertTrue(tc, valid);
}

void Test_necromancer_deathless_touch_is_consumed_only_by_successful_undead_summon(CuTest *tc)
{
  struct char_data ch;
  struct player_special_data player_specials;

  setup_necromancer_character(&ch, &player_specials);
  ch.char_specials.deathless_touch = true;

  CuAssertTrue(tc, test_deathless_touch_empowers_summon(&ch, SPELL_ANIMATE_DEAD));
  CuAssertTrue(tc, !test_deathless_touch_empowers_summon(&ch, SPELL_MUMMY_DUST));

  test_complete_deathless_touch_summon(&ch, SPELL_ANIMATE_DEAD, false);
  CuAssertTrue(tc, ch.char_specials.deathless_touch);
  test_complete_deathless_touch_summon(&ch, SPELL_MUMMY_DUST, true);
  CuAssertTrue(tc, ch.char_specials.deathless_touch);
  test_complete_deathless_touch_summon(&ch, SPELL_GREATER_ANIMATION, true);
  CuAssertTrue(tc, !ch.char_specials.deathless_touch);
}

void Test_necromancer_summon_persistence_failure_warns_without_rollback(CuTest *tc)
{
  struct char_data ch;
  struct descriptor_data descriptor;
  struct player_special_data player_specials;
  bool warned;

  setup_necromancer_character(&ch, &player_specials);
  memset(&descriptor, 0, sizeof(descriptor));
  descriptor.output = descriptor.small_outbuf;
  descriptor.bufspace = SMALL_BUFSIZE - 1;
  descriptor.character = &ch;
  descriptor.pProtocol = ProtocolCreate();
  ch.desc = &descriptor;

  if (descriptor.pProtocol == NULL)
  {
    ch.desc = NULL;
    CuFail(tc, "could not initialize the summon persistence warning fixture");
    return;
  }

  test_report_summon_persistence_result(&ch, false);
  warned = strstr(descriptor.output, "summoned follower is active, but could not be saved") != NULL;

  ch.desc = NULL;
  ProtocolDestroy(descriptor.pProtocol);

  CuAssertTrue(tc, warned);
}

void Test_necromancer_tough_as_bone_blocks_stun_affects_at_admission(CuTest *tc)
{
  struct char_data ch;
  struct player_special_data player_specials;
  struct affected_type af;
  bool immune_has_affect;
  bool immune_is_stunned;
  bool ordinary_has_affect;
  bool ordinary_is_stunned;

  setup_necromancer_character(&ch, &player_specials);
  SET_FEAT(&ch, FEAT_TOUGH_AS_BONE, 1);
  new_affect(&af);
  af.spell = SPELL_POWER_WORD_STUN;
  af.duration = 1;
  SET_BIT_AR(af.bitvector, AFF_STUN);

  affect_to_char(&ch, &af);
  immune_has_affect = ch.affected != NULL;
  immune_is_stunned = AFF_FLAGGED(&ch, AFF_STUN);

  SET_FEAT(&ch, FEAT_TOUGH_AS_BONE, 0);
  affect_to_char(&ch, &af);
  ordinary_has_affect = ch.affected != NULL;
  ordinary_is_stunned = AFF_FLAGGED(&ch, AFF_STUN);

  if (ch.affected != NULL)
    affect_from_char(&ch, SPELL_POWER_WORD_STUN);

  CuAssertTrue(tc, !immune_has_affect);
  CuAssertTrue(tc, !immune_is_stunned);
  CuAssertTrue(tc, ordinary_has_affect);
  CuAssertTrue(tc, ordinary_is_stunned);
}

void Test_necromancer_tough_as_bone_blocks_timed_stun_events_at_admission(CuTest *tc)
{
  struct char_data ch;
  struct player_special_data player_specials;
  struct list_data list_registry;
  struct list_data *saved_global_lists;
  bool immune_has_event;
  bool ordinary_has_event;

  setup_necromancer_character(&ch, &player_specials);
  memset(&list_registry, 0, sizeof(list_registry));
  saved_global_lists = global_lists;
  if (global_lists == NULL)
    global_lists = &list_registry;
  event_free_all();
  event_init();

  SET_FEAT(&ch, FEAT_TOUGH_AS_BONE, 1);
  attach_mud_event(new_mud_event(eSTUNNED, &ch, NULL), 6 * PASSES_PER_SEC);
  immune_has_event = char_has_mud_event(&ch, eSTUNNED) != NULL;

  SET_FEAT(&ch, FEAT_TOUGH_AS_BONE, 0);
  attach_mud_event(new_mud_event(eSTUNNED, &ch, NULL), 6 * PASSES_PER_SEC);
  ordinary_has_event = char_has_mud_event(&ch, eSTUNNED) != NULL;

  clear_char_event_list(&ch);
  event_free_all();
  global_lists = saved_global_lists;

  CuAssertTrue(tc, !immune_has_event);
  CuAssertTrue(tc, ordinary_has_event);
}

void Test_necromancer_cohort_resistance_evolutions_modify_only_cohort(CuTest *tc)
{
  struct char_data ch;
  struct char_data cohort;
  struct player_special_data player_specials;

  setup_necromancer_character(&ch, &player_specials);
  clear_char(&cohort);
  GET_LEVEL(&cohort) = 10;
  KNOWS_EVOLUTION(&ch, EVOLUTION_FIRE_RESIST) = 1;
  KNOWS_EVOLUTION(&ch, EVOLUTION_COLD_RESIST) = 1;
  KNOWS_EVOLUTION(&ch, EVOLUTION_ACID_RESIST) = 1;
  KNOWS_EVOLUTION(&ch, EVOLUTION_ELECTRIC_RESIST) = 1;
  KNOWS_EVOLUTION(&ch, EVOLUTION_SONIC_RESIST) = 1;

  assign_eidolon_evolutions(&ch, &cohort, true);

  CuAssertIntEquals(tc, 0, GET_RESISTANCES(&ch, DAM_FIRE));
  CuAssertIntEquals(tc, 0, GET_RESISTANCES(&ch, DAM_COLD));
  CuAssertIntEquals(tc, 0, GET_RESISTANCES(&ch, DAM_ACID));
  CuAssertIntEquals(tc, 0, GET_RESISTANCES(&ch, DAM_ELECTRIC));
  CuAssertIntEquals(tc, 0, GET_RESISTANCES(&ch, DAM_SOUND));
  CuAssertIntEquals(tc, 50, GET_RESISTANCES(&cohort, DAM_FIRE));
  CuAssertIntEquals(tc, 50, GET_RESISTANCES(&cohort, DAM_COLD));
  CuAssertIntEquals(tc, 50, GET_RESISTANCES(&cohort, DAM_ACID));
  CuAssertIntEquals(tc, 50, GET_RESISTANCES(&cohort, DAM_ELECTRIC));
  CuAssertIntEquals(tc, 50, GET_RESISTANCES(&cohort, DAM_SOUND));
}

void Test_necromancer_mandatory_cohort_identity_is_free_and_budget_nonnegative(CuTest *tc)
{
  struct char_data ch;
  struct player_special_data player_specials;
  struct level_data levelup;
  int saved_undead_cost;
  int saved_resistance_cost;
  int mandatory_only_points;
  int overspent_points;
  bool has_choice;

  setup_necromancer_character(&ch, &player_specials);
  memset(&levelup, 0, sizeof(levelup));
  CLASS_LEVEL((&ch), CLASS_NECROMANCER) = 1;
  LEVELUP((&ch)) = &levelup;
  LEVELUP((&ch))->eidolon_base_form = EIDOLON_BASE_FORM_BIPED;
  LEVELUP((&ch))->eidolon_evolutions[EVOLUTION_UNDEAD_APPEARANCE] = 1;
  saved_undead_cost = evolution_list[EVOLUTION_UNDEAD_APPEARANCE].evolution_points;
  saved_resistance_cost = evolution_list[EVOLUTION_FIRE_RESIST].evolution_points;
  evolution_list[EVOLUTION_UNDEAD_APPEARANCE].evolution_points = 2;
  evolution_list[EVOLUTION_FIRE_RESIST].evolution_points = 2;

  mandatory_only_points = num_free_evolution_points(&ch);
  has_choice = has_evolutions_unchosen(&ch);
  LEVELUP((&ch))->eidolon_evolutions[EVOLUTION_FIRE_RESIST] = 1;
  overspent_points = num_free_evolution_points(&ch);

  evolution_list[EVOLUTION_UNDEAD_APPEARANCE].evolution_points = saved_undead_cost;
  evolution_list[EVOLUTION_FIRE_RESIST].evolution_points = saved_resistance_cost;

  CuAssertIntEquals(tc, 1, mandatory_only_points);
  CuAssertTrue(tc, has_choice);
  CuAssertIntEquals(tc, 0, overspent_points);
}
