/**
 * @file test_spec_mechanics.c
 * Phase 04 characterization for reusable special-procedure mechanics.
 */

#include "CuTest.h"

#include "../../src/conf.h"
#include "../../src/sysdep.h"
#include "../../src/structs.h"
#include "../../src/utils.h"

#include "../../src/combat/fight.h"
#include "../../src/db.h"
#include "../../src/handler.h"
#include "../../src/interpreter.h"
#include "../../src/magic/spells.h"
#include "../../src/mud_event.h"
#include "../../src/movement/movement.h"
#include "../../src/spec/spec_combat.h"
#include "../../src/spec/spec_context.h"
#include "../../src/spec/spec_cooldown.h"
#include "../../src/spec/spec_dispatch.h"
#include "../../src/spec/spec_effects.h"
#include "../../src/spec/spec_objects.h"
#include "../../src/spec/spec_phrase.h"
#include "../../src/spec/spec_rol_avernus.h"
#include "../../src/spec/spec_rol_conversion.h"
#include "../../src/spec/spec_rol_lavatubes.h"
#include "../../src/spec/spec_rol_tarrasque.h"
#include "../../src/spec/spec_rol_totem.h"
#include "../../src/spec/spec_rol_utility_objects.h"

#include <limits.h>
#include <string.h>

void Test_spec_rol_tarrasque_preserves_loot_weights_and_corpse_aliases(CuTest *tc)
{
  CuAssertIntEquals(tc, -1, rol_tarrasque_loot_vnum_for_roll(0));
  CuAssertIntEquals(tc, 2002605, rol_tarrasque_loot_vnum_for_roll(1));
  CuAssertIntEquals(tc, 2002605, rol_tarrasque_loot_vnum_for_roll(6));
  CuAssertIntEquals(tc, 2002606, rol_tarrasque_loot_vnum_for_roll(7));
  CuAssertIntEquals(tc, 2002606, rol_tarrasque_loot_vnum_for_roll(12));
  CuAssertIntEquals(tc, 2002607, rol_tarrasque_loot_vnum_for_roll(13));
  CuAssertIntEquals(tc, 2002607, rol_tarrasque_loot_vnum_for_roll(18));
  CuAssertIntEquals(tc, 2002608, rol_tarrasque_loot_vnum_for_roll(19));
  CuAssertIntEquals(tc, 2002608, rol_tarrasque_loot_vnum_for_roll(20));
  CuAssertIntEquals(tc, -1, rol_tarrasque_loot_vnum_for_roll(21));

  CuAssertTrue(tc, rol_tarrasque_corpse_keyword(" corpse", "corpse tarrasque remains"));
  CuAssertTrue(tc, rol_tarrasque_corpse_keyword("tarrasque", "corpse tarrasque remains"));
  CuAssertTrue(tc, !rol_tarrasque_corpse_keyword("dragon", "corpse tarrasque remains"));
  CuAssertTrue(tc, !rol_tarrasque_corpse_keyword(NULL, "corpse tarrasque remains"));
}

struct spec_mechanics_fixture
{
  struct room_data rooms[2];
  struct zone_data zones[1];
  struct index_data mobile_indexes[2];
  struct index_data object_indexes[2];
  struct char_data actor;
  struct char_data target;
  struct obj_data worn;
  struct obj_data copy;

  struct room_data *saved_world;
  struct zone_data *saved_zone_table;
  struct index_data *saved_mob_index;
  struct index_data *saved_obj_index;
  room_rnum saved_top_of_world;
  zone_rnum saved_top_of_zone_table;
  mob_rnum saved_top_of_mobt;
  obj_rnum saved_top_of_objt;
};

static void spec_mechanics_initialize_npc(struct char_data *ch, const char *name, room_rnum room)
{
  clear_char(ch);
  SET_BIT_AR(MOB_FLAGS(ch), MOB_ISNPC);
  ch->player_specials = &dummy_mob;
  ch->player.short_descr = (char *)name;
  GET_LEVEL(ch) = 10;
  GET_POS(ch) = POS_STANDING;
  GET_HIT(ch) = 100;
  GET_MAX_HIT(ch) = 100;
  GET_MOVE(ch) = 100;
  GET_MAX_MOVE(ch) = 100;
  IN_ROOM(ch) = room;
}

static void spec_mechanics_begin(struct spec_mechanics_fixture *fixture)
{
  memset(fixture, 0, sizeof(*fixture));

  fixture->saved_world = world;
  fixture->saved_zone_table = zone_table;
  fixture->saved_mob_index = mob_index;
  fixture->saved_obj_index = obj_index;
  fixture->saved_top_of_world = top_of_world;
  fixture->saved_top_of_zone_table = top_of_zone_table;
  fixture->saved_top_of_mobt = top_of_mobt;
  fixture->saved_top_of_objt = top_of_objt;

  fixture->rooms[0].number = 6100;
  fixture->rooms[0].zone = 0;
  fixture->rooms[0].sector_type = SECT_INSIDE;
  fixture->rooms[0].name = "Special mechanic origin";
  fixture->rooms[1].number = 6101;
  fixture->rooms[1].zone = 0;
  fixture->rooms[1].sector_type = SECT_INSIDE;
  fixture->rooms[1].name = "Special mechanic destination";
  fixture->zones[0].number = 61;
  fixture->zones[0].bot = 6100;
  fixture->zones[0].top = 6199;
  fixture->zones[0].min_level = -1;
  fixture->zones[0].max_level = LVL_IMPL;
  fixture->mobile_indexes[0].vnum = 6200;
  fixture->mobile_indexes[1].vnum = 6201;
  fixture->object_indexes[0].vnum = 6300;
  fixture->object_indexes[1].vnum = 6301;

  world = fixture->rooms;
  zone_table = fixture->zones;
  mob_index = fixture->mobile_indexes;
  obj_index = fixture->object_indexes;
  top_of_world = 1;
  top_of_zone_table = 0;
  top_of_mobt = 1;
  top_of_objt = 1;

  spec_mechanics_initialize_npc(&fixture->actor, "special mechanic actor", 0);
  spec_mechanics_initialize_npc(&fixture->target, "special mechanic target", 0);
  fixture->rooms[0].people = &fixture->actor;
  fixture->actor.next_in_room = &fixture->target;

  clear_object(&fixture->worn);
  clear_object(&fixture->copy);
  GET_OBJ_RNUM(&fixture->worn) = 0;
  GET_OBJ_RNUM(&fixture->copy) = 0;
}

static void spec_mechanics_end(struct spec_mechanics_fixture *fixture)
{
  FIGHTING(&fixture->actor) = NULL;
  FIGHTING(&fixture->target) = NULL;
  GET_EQ(&fixture->actor, WEAR_FEET) = NULL;
  fixture->worn.worn_by = NULL;
  fixture->copy.worn_by = NULL;
  fixture->rooms[0].people = NULL;
  fixture->actor.next_in_room = NULL;
  fixture->target.next_in_room = NULL;

  while (fixture->actor.affected != NULL)
    affect_remove_no_total(&fixture->actor, fixture->actor.affected);
  while (fixture->target.affected != NULL)
    affect_remove_no_total(&fixture->target, fixture->target.affected);

  world = fixture->saved_world;
  zone_table = fixture->saved_zone_table;
  mob_index = fixture->saved_mob_index;
  obj_index = fixture->saved_obj_index;
  top_of_world = fixture->saved_top_of_world;
  top_of_zone_table = fixture->saved_top_of_zone_table;
  top_of_mobt = fixture->saved_top_of_mobt;
  top_of_objt = fixture->saved_top_of_objt;
}

static void spec_mechanics_wear(struct spec_mechanics_fixture *fixture, struct obj_data *obj)
{
  obj->worn_by = &fixture->actor;
  obj->worn_on = WEAR_FEET;
  GET_EQ(&fixture->actor, WEAR_FEET) = obj;
}

static int spec_mechanics_affect_count(const struct char_data *ch)
{
  const struct affected_type *af;
  int count = 0;

  for (af = ch->affected; af != NULL; af = af->next)
    count++;

  return count;
}

void Test_spec_context_validates_event_payload_shape(CuTest *tc)
{
  struct spec_event_context context;
  struct moving_room_data mover;
  struct moving_room_data other_mover;
  struct char_data actor;
  struct obj_data object;

  memset(&context, 0, sizeof(context));
  memset(&mover, 0, sizeof(mover));
  memset(&other_mover, 0, sizeof(other_mover));
  memset(&actor, 0, sizeof(actor));
  memset(&object, 0, sizeof(object));

  context.owner_type = SPEC_OWNER_OBJECT;
  context.event = SPEC_EVENT_COMMAND;
  context.owner = &object;
  context.actor = &actor;
  context.argument = "";
  CuAssertIntEquals(tc, SPEC_CONTEXT_VALID, spec_context_validate_event(&context));

  context.owner_type = SPEC_OWNER_OBJECT | SPEC_OWNER_MOBILE;
  CuAssertIntEquals(tc, SPEC_CONTEXT_INVALID_OWNER_TYPE, spec_context_validate_event(&context));

  context.owner_type = SPEC_OWNER_OBJECT;
  context.event = SPEC_EVENT_COMMAND | SPEC_EVENT_ITEM_IDENTIFY;
  CuAssertIntEquals(tc, SPEC_CONTEXT_INVALID_EVENT, spec_context_validate_event(&context));

  context.event = SPEC_EVENT_MOBILE_ACTIVITY;
  CuAssertIntEquals(tc, SPEC_CONTEXT_OWNER_EVENT_MISMATCH, spec_context_validate_event(&context));

  context.event = SPEC_EVENT_WEAPON_HIT;
  CuAssertIntEquals(tc, SPEC_CONTEXT_MISSING_TARGET, spec_context_validate_event(&context));

  context.owner_type = SPEC_OWNER_MOBILE;
  context.event = SPEC_EVENT_MOBILE_HIT;
  context.owner = &actor;
  context.actor = &actor;
  CuAssertIntEquals(tc, SPEC_CONTEXT_MISSING_TARGET, spec_context_validate_event(&context));
  context.target = &actor;
  CuAssertIntEquals(tc, SPEC_CONTEXT_VALID, spec_context_validate_event(&context));
  context.event = SPEC_EVENT_MOBILE_WAS_HIT;
  CuAssertIntEquals(tc, SPEC_CONTEXT_VALID, spec_context_validate_event(&context));

  memset(&context, 0, sizeof(context));
  context.owner_type = SPEC_OWNER_ROOM;
  context.event = SPEC_EVENT_MOVING_ROOM_RELOCATION;
  context.owner = &mover;
  context.moving_room = &mover;
  CuAssertIntEquals(tc, SPEC_CONTEXT_VALID, spec_context_validate_event(&context));

  context.moving_room = &other_mover;
  CuAssertIntEquals(tc, SPEC_CONTEXT_INVALID_MOVING_ROOM, spec_context_validate_event(&context));
}

void Test_spec_context_requires_exact_worn_object_instance(CuTest *tc)
{
  struct spec_mechanics_fixture fixture;

  spec_mechanics_begin(&fixture);
  spec_mechanics_wear(&fixture, &fixture.worn);

  CuAssertIntEquals(tc, SPEC_CONTEXT_VALID,
                    spec_context_validate_worn_object(&fixture.actor, &fixture.worn));

  fixture.copy.worn_by = &fixture.actor;
  fixture.copy.worn_on = WEAR_FEET;
  CuAssertIntEquals(tc, SPEC_CONTEXT_OBJECT_SLOT_MISMATCH,
                    spec_context_validate_worn_object(&fixture.actor, &fixture.copy));

  fixture.copy.worn_by = NULL;
  CuAssertIntEquals(tc, SPEC_CONTEXT_OBJECT_NOT_WORN,
                    spec_context_validate_worn_object(&fixture.actor, &fixture.copy));

  IN_ROOM(&fixture.actor) = NOWHERE;
  CuAssertIntEquals(tc, SPEC_CONTEXT_INVALID_ACTOR_ROOM,
                    spec_context_validate_worn_object(&fixture.actor, &fixture.worn));
  IN_ROOM(&fixture.actor) = 0;

  GET_POS(&fixture.actor) = POS_DEAD;
  CuAssertIntEquals(tc, SPEC_CONTEXT_ACTOR_UNAVAILABLE,
                    spec_context_validate_worn_object(&fixture.actor, &fixture.worn));

  spec_mechanics_end(&fixture);
}

void Test_spec_context_validates_live_colocated_current_target(CuTest *tc)
{
  struct spec_mechanics_fixture fixture;

  spec_mechanics_begin(&fixture);
  FIGHTING(&fixture.actor) = &fixture.target;

  CuAssertIntEquals(tc, SPEC_CONTEXT_VALID,
                    spec_context_validate_combat_target(&fixture.actor, &fixture.target, true));

  FIGHTING(&fixture.actor) = NULL;
  CuAssertIntEquals(tc, SPEC_CONTEXT_NOT_CURRENT_TARGET,
                    spec_context_validate_combat_target(&fixture.actor, &fixture.target, true));
  FIGHTING(&fixture.actor) = &fixture.target;

  IN_ROOM(&fixture.target) = 1;
  CuAssertIntEquals(tc, SPEC_CONTEXT_DIFFERENT_ROOMS,
                    spec_context_validate_combat_target(&fixture.actor, &fixture.target, false));
  IN_ROOM(&fixture.target) = 0;

  GET_POS(&fixture.target) = POS_DEAD;
  CuAssertIntEquals(tc, SPEC_CONTEXT_TARGET_UNAVAILABLE,
                    spec_context_validate_combat_target(&fixture.actor, &fixture.target, false));

  spec_mechanics_end(&fixture);
}

void Test_spec_phrase_matching_preserves_exact_legacy_text(CuTest *tc)
{
  struct spec_phrase_rule rule = {"say", "whirlwind", SPEC_PHRASE_SKIP_LEADING_SPACES};
  struct spec_phrase_rule exact_rule = {"say", "whirlwind", SPEC_PHRASE_EXACT};

  CuAssertIntEquals(tc, SPEC_PHRASE_MATCHED, spec_phrase_match("say", "whirlwind", &rule));
  CuAssertIntEquals(tc, SPEC_PHRASE_MATCHED, spec_phrase_match("say", "  whirlwind", &rule));
  CuAssertIntEquals(tc, SPEC_PHRASE_UNRELATED, spec_phrase_match("say", "\twhirlwind", &rule));
  CuAssertIntEquals(tc, SPEC_PHRASE_UNRELATED, spec_phrase_match("say", "Whirlwind", &rule));
  CuAssertIntEquals(tc, SPEC_PHRASE_UNRELATED, spec_phrase_match("say", "whirlwind ", &rule));
  CuAssertIntEquals(tc, SPEC_PHRASE_UNRELATED, spec_phrase_match("whisper", "whirlwind", &rule));
  CuAssertIntEquals(tc, SPEC_PHRASE_UNRELATED, spec_phrase_match("say", " whirlwind", &exact_rule));

  rule.flags = SPEC_PHRASE_ALL | (1U << 8);
  CuAssertIntEquals(tc, SPEC_PHRASE_INVALID, spec_phrase_match("say", "whirlwind", &rule));
}

void Test_spec_object_cooldown_is_instance_scoped_and_explicit(CuTest *tc)
{
  struct obj_data object;
  struct spec_object_cooldown_state state;

  clear_object(&object);
  state = spec_object_cooldown_read(&object, 0);
  CuAssertIntEquals(tc, SPEC_OBJECT_COOLDOWN_READY, state.status);
  CuAssertIntEquals(tc, 0, state.remaining_mud_hours);

  CuAssertTrue(tc, spec_object_cooldown_commit(&object, 0, 12));
  state = spec_object_cooldown_read(&object, 0);
  CuAssertIntEquals(tc, SPEC_OBJECT_COOLDOWN_ACTIVE, state.status);
  CuAssertIntEquals(tc, 12, state.remaining_mud_hours);

  CuAssertIntEquals(tc, SPEC_OBJECT_COOLDOWN_INVALID, spec_object_cooldown_read(NULL, 0).status);
  CuAssertIntEquals(tc, SPEC_OBJECT_COOLDOWN_INVALID,
                    spec_object_cooldown_read(&object, SPEC_TIMER_MAX).status);
  CuAssertTrue(tc, !spec_object_cooldown_commit(&object, -1, 1));
  CuAssertTrue(tc, !spec_object_cooldown_commit(&object, 0, 0));
}

void Test_spec_damage_reports_validation_and_legacy_outcome(CuTest *tc)
{
  struct spec_mechanics_fixture fixture;
  struct spec_damage_result result;

  spec_mechanics_begin(&fixture);
  FIGHTING(&fixture.actor) = &fixture.target;
  FIGHTING(&fixture.target) = &fixture.actor;
  GET_POS(&fixture.actor) = POS_FIGHTING;
  GET_POS(&fixture.target) = POS_FIGHTING;

  result = spec_damage_current_target(&fixture.actor, &fixture.target, -1, TYPE_HIT, DAM_BLUDGEON,
                                      FALSE);
  CuAssertIntEquals(tc, SPEC_DAMAGE_INVALID_AMOUNT, result.status);
  CuAssertIntEquals(tc, 100, GET_HIT(&fixture.target));

  FIGHTING(&fixture.actor) = NULL;
  result =
      spec_damage_current_target(&fixture.actor, &fixture.target, 5, TYPE_HIT, DAM_BLUDGEON, FALSE);
  CuAssertIntEquals(tc, SPEC_DAMAGE_INVALID_CONTEXT, result.status);
  CuAssertIntEquals(tc, SPEC_CONTEXT_NOT_CURRENT_TARGET, result.context_result);

  FIGHTING(&fixture.actor) = &fixture.target;
  result =
      spec_damage_current_target(&fixture.actor, &fixture.target, 0, TYPE_HIT, DAM_BLUDGEON, FALSE);
  CuAssertIntEquals(tc, SPEC_DAMAGE_NO_EFFECT, result.status);
  CuAssertIntEquals(tc, 0, result.legacy_result);
  CuAssertIntEquals(tc, 100, GET_HIT(&fixture.target));

  result =
      spec_damage_current_target(&fixture.actor, &fixture.target, 7, TYPE_HIT, DAM_BLUDGEON, FALSE);
  CuAssertIntEquals(tc, SPEC_DAMAGE_APPLIED, result.status);
  CuAssertTrue(tc, result.legacy_result > 0);
  CuAssertTrue(tc, GET_HIT(&fixture.target) < 100);

  spec_mechanics_end(&fixture);
}

void Test_spec_effect_groups_are_atomic_and_source_owned(CuTest *tc)
{
  struct spec_mechanics_fixture fixture;
  struct spec_effect_modifier modifiers[2] = {
      {APPLY_HITROLL, 2, BONUS_TYPE_MORALE, 3, SPEC_EFFECT_NO_AFF_FLAG},
      {APPLY_DAMROLL, 3, BONUS_TYPE_MORALE, 3, SPEC_EFFECT_NO_AFF_FLAG}};
  const struct affected_type *af;
  long artifact_source = 0;
  long repeated_source = 0;
  long legacy_source = 0;
  bool first_group_owned = true;
  bool remaining_group_owned = true;

  spec_mechanics_begin(&fixture);

  CuAssertTrue(tc, spec_effect_source_id(SPEC_EFFECT_SOURCE_ARTIFACT, 42, &artifact_source));
  CuAssertTrue(tc, spec_effect_source_id(SPEC_EFFECT_SOURCE_ARTIFACT, 42, &repeated_source));
  CuAssertTrue(tc, spec_effect_source_id(SPEC_EFFECT_SOURCE_LEGACY_PROCEDURE, 42, &legacy_source));
  CuAssertTrue(tc, artifact_source < 0);
  CuAssertTrue(tc, artifact_source == repeated_source);
  CuAssertTrue(tc, artifact_source != legacy_source);
  CuAssertTrue(tc, !spec_effect_source_id(SPEC_EFFECT_SOURCE_INVALID, 42, &repeated_source));

  CuAssertIntEquals(tc, SPEC_EFFECT_APPLIED,
                    spec_effect_apply_group(&fixture.actor, SPELL_ARTIFACT_SURGE, artifact_source,
                                            11, modifiers, 2));
  CuAssertIntEquals(tc, 2, spec_mechanics_affect_count(&fixture.actor));
  for (af = fixture.actor.affected; af != NULL; af = af->next)
    if (af->source_id != artifact_source || af->specific != 11)
      first_group_owned = false;
  CuAssertTrue(tc, first_group_owned);

  CuAssertIntEquals(tc, SPEC_EFFECT_STACKING_CONFLICT,
                    spec_effect_apply_group(&fixture.actor, SPELL_ARTIFACT_SURGE, legacy_source, 11,
                                            modifiers, 2));
  CuAssertIntEquals(tc, 2, spec_mechanics_affect_count(&fixture.actor));

  CuAssertIntEquals(tc, SPEC_EFFECT_APPLIED,
                    spec_effect_apply_group(&fixture.actor, SPELL_ARTIFACT_SURGE, legacy_source, 12,
                                            modifiers, 2));
  CuAssertIntEquals(tc, 4, spec_mechanics_affect_count(&fixture.actor));
  CuAssertTrue(tc, spec_effect_stack_active(&fixture.actor, SPELL_ARTIFACT_SURGE, 11));
  CuAssertTrue(tc, spec_effect_stack_active(&fixture.actor, SPELL_ARTIFACT_SURGE, 12));

  CuAssertIntEquals(
      tc, SPEC_EFFECT_INVALID,
      spec_effect_apply_group(&fixture.actor, SPELL_ARTIFACT_SURGE, 7, 13, modifiers, 2));
  CuAssertIntEquals(tc, 4, spec_mechanics_affect_count(&fixture.actor));

  affect_from_char_source(&fixture.actor, SPELL_ARTIFACT_SURGE, artifact_source);
  CuAssertIntEquals(tc, 2, spec_mechanics_affect_count(&fixture.actor));
  for (af = fixture.actor.affected; af != NULL; af = af->next)
    if (af->source_id != legacy_source || af->specific != 12)
      remaining_group_owned = false;
  CuAssertTrue(tc, remaining_group_owned);

  spec_mechanics_end(&fixture);
}

void Test_spec_invoked_objects_share_exact_phrase_and_cooldown_contracts(CuTest *tc)
{
  struct spec_mechanics_fixture fixture;
  struct command_info commands[2];
  struct command_info *saved_complete_cmd_info;
  bool stability_matched;
  bool hellfire_matched;
  bool case_rejected;
  bool trailing_space_rejected;
  bool tab_rejected;
  bool copied_instance_rejected;

  spec_mechanics_begin(&fixture);
  memset(commands, 0, sizeof(commands));
  commands[1].command = "say";
  saved_complete_cmd_info = complete_cmd_info;
  complete_cmd_info = commands;

  spec_mechanics_wear(&fixture, &fixture.worn);
  fixture.copy.worn_by = &fixture.actor;
  fixture.copy.worn_on = WEAR_FEET;
  (void)spec_object_cooldown_commit(&fixture.worn, 0, 3);
  (void)spec_object_cooldown_commit(&fixture.copy, 0, 3);

  stability_matched = stability_boots(&fixture.actor, &fixture.worn, 1, "  whirlwind") == TRUE;
  hellfire_matched = hellfire(&fixture.actor, &fixture.worn, 1, " hellfire") == TRUE;
  case_rejected = stability_boots(&fixture.actor, &fixture.worn, 1, "Whirlwind") == FALSE;
  trailing_space_rejected = hellfire(&fixture.actor, &fixture.worn, 1, "hellfire ") == FALSE;
  tab_rejected = stability_boots(&fixture.actor, &fixture.worn, 1, "\twhirlwind") == FALSE;
  copied_instance_rejected =
      stability_boots(&fixture.actor, &fixture.copy, 1, "whirlwind") == FALSE;

  complete_cmd_info = saved_complete_cmd_info;
  spec_mechanics_end(&fixture);

  CuAssertTrue(tc, stability_matched);
  CuAssertTrue(tc, hellfire_matched);
  CuAssertTrue(tc, case_rejected);
  CuAssertTrue(tc, trailing_space_rejected);
  CuAssertTrue(tc, tab_rejected);
  CuAssertTrue(tc, copied_instance_rejected);
}

void Test_spec_rol_shared_mobile_adapters_preserve_source_boundaries(CuTest *tc)
{
  struct spec_mechanics_fixture fixture;
  struct obj_data food;
  struct obj_data npc_corpse;
  struct obj_data player_corpse;

  spec_mechanics_begin(&fixture);
  clear_object(&food);
  clear_object(&npc_corpse);
  clear_object(&player_corpse);

  GET_OBJ_TYPE(&food) = ITEM_FOOD;
  GET_OBJ_TYPE(&npc_corpse) = ITEM_CONTAINER;
  GET_OBJ_VAL(&npc_corpse, 3) = 1;
  GET_OBJ_TYPE(&player_corpse) = ITEM_CONTAINER;
  GET_OBJ_VAL(&player_corpse, 3) = 1;
  GET_OBJ_VAL(&player_corpse, 4) = 42;

  CuAssertTrue(tc, rol_corpse_devourer_can_consume(&food));
  CuAssertTrue(tc, rol_corpse_devourer_can_consume(&npc_corpse));
  CuAssertTrue(tc, !rol_corpse_devourer_can_consume(&player_corpse));
  CuAssertTrue(tc, !rol_corpse_devourer_can_consume(NULL));

  CuAssertIntEquals(tc, 61, rol_poison_bite_roll_ceiling(0));
  CuAssertIntEquals(tc, 51, rol_poison_bite_roll_ceiling(10));
  CuAssertIntEquals(tc, 27, rol_poison_bite_roll_ceiling(34));
  CuAssertIntEquals(tc, 0, rol_poison_bite_roll_ceiling(70));
  CuAssertIntEquals(tc, 0, rol_umberhulk_proc_chance(0));
  CuAssertIntEquals(tc, 17, rol_umberhulk_proc_chance(10));
  CuAssertIntEquals(tc, 57, rol_umberhulk_proc_chance(34));
  CuAssertIntEquals(tc, 100, rol_umberhulk_proc_chance(70));

  SET_BIT_AR(MOB_FLAGS(&fixture.actor), MOB_ROL_DEMON);
  fixture.actor.player.short_descr = "babau demon";
  CuAssertIntEquals(tc, SECS_PER_MUD_DAY, rol_planar_gate_cooldown_seconds(&fixture.actor));
  fixture.actor.player.short_descr = "babau demon nogate";
  CuAssertIntEquals(tc, 0, rol_planar_gate_cooldown_seconds(&fixture.actor));

  CuAssertTrue(tc, !rol_handle_conjured_death(&fixture.actor));
  SET_BIT_AR(MOB_FLAGS(&fixture.actor), MOB_ROL_BLACK_VAPOR_DEATH);
  CuAssertTrue(tc, rol_handle_conjured_death(&fixture.actor));
  REMOVE_BIT_AR(MOB_FLAGS(&fixture.actor), MOB_ROL_BLACK_VAPOR_DEATH);
  SET_BIT_AR(MOB_FLAGS(&fixture.actor), MOB_ROL_FADE_FAMILIAR);
  CuAssertTrue(tc, rol_handle_conjured_death(&fixture.actor));

  CuAssertIntEquals(tc, TRUE, rol_thief(&fixture.actor, &fixture.actor, 0, ""));
  CuAssertIntEquals(tc, FALSE, rol_thief(&fixture.actor, &fixture.actor, 1, ""));
  GET_POS(&fixture.actor) = POS_SITTING;
  CuAssertIntEquals(tc, FALSE, rol_thief(&fixture.actor, &fixture.actor, 0, ""));

  spec_mechanics_end(&fixture);
}

void Test_spec_rol_home_reset_updates_only_mobile_home_from_marked_room(CuTest *tc)
{
  struct spec_mechanics_fixture fixture;

  spec_mechanics_begin(&fixture);
  GET_MOB_LOADROOM(&fixture.actor) = 0;

  CuAssertTrue(tc, !rol_update_mobile_home_after_move(&fixture.actor, 0, 1));
  CuAssertIntEquals(tc, 0, GET_MOB_LOADROOM(&fixture.actor));

  SET_BIT_AR(ROOM_FLAGS(0), ROOM_ROL_HOME_RESET);
  CuAssertTrue(tc, rol_update_mobile_home_after_move(&fixture.actor, 0, 1));
  CuAssertIntEquals(tc, 1, GET_MOB_LOADROOM(&fixture.actor));

  REMOVE_BIT_AR(MOB_FLAGS(&fixture.actor), MOB_ISNPC);
  CuAssertTrue(tc, !rol_update_mobile_home_after_move(&fixture.actor, 0, 1));
  SET_BIT_AR(MOB_FLAGS(&fixture.actor), MOB_ISNPC);
  CuAssertTrue(tc, !rol_update_mobile_home_after_move(&fixture.actor, NOWHERE, 1));
  CuAssertTrue(tc, !rol_update_mobile_home_after_move(&fixture.actor, 0, NOWHERE));
  CuAssertTrue(tc, !rol_update_mobile_home_after_move(NULL, 0, 1));

  spec_mechanics_end(&fixture);
}

void Test_spec_rol_magic_pool_damages_and_transports_matching_entry(CuTest *tc)
{
  struct spec_mechanics_fixture fixture;
  struct command_info commands[2];
  struct command_info *saved_complete_cmd_info;

  spec_mechanics_begin(&fixture);
  memset(commands, 0, sizeof(commands));
  commands[1].command = "enter";
  saved_complete_cmd_info = complete_cmd_info;
  complete_cmd_info = commands;

  fixture.worn.name = "ruby pool";
  GET_OBJ_VAL(&fixture.worn, 0) = 6101;
  GET_OBJ_VAL(&fixture.worn, 1) = 25;

  CuAssertIntEquals(tc, FALSE, rol_magic_pool(&fixture.actor, &fixture.worn, 1, "door"));
  CuAssertIntEquals(tc, 0, IN_ROOM(&fixture.actor));
  CuAssertIntEquals(tc, TRUE, rol_magic_pool(&fixture.actor, &fixture.worn, 1, "pool"));
  CuAssertIntEquals(tc, 1, IN_ROOM(&fixture.actor));
  CuAssertIntEquals(tc, 75, GET_HIT(&fixture.actor));

  char_from_room(&fixture.actor);
  char_to_room(&fixture.actor, 0);
  complete_cmd_info = saved_complete_cmd_info;
  spec_mechanics_end(&fixture);
}

void Test_spec_rol_bloodstone_portal_transports_and_applies_stress(CuTest *tc)
{
  struct spec_mechanics_fixture fixture;
  struct command_info commands[2];
  struct command_info *saved_complete_cmd_info;

  spec_mechanics_begin(&fixture);
  memset(commands, 0, sizeof(commands));
  commands[1].command = "enter";
  saved_complete_cmd_info = complete_cmd_info;
  complete_cmd_info = commands;

  fixture.worn.name = "shimmering portal";
  fixture.worn.short_description = "a shimmering portal";
  IN_ROOM(&fixture.worn) = 0;
  fixture.rooms[0].contents = &fixture.worn;
  GET_OBJ_VAL(&fixture.worn, 0) = 6101;

  CuAssertIntEquals(tc, FALSE, rol_bloodstone_portal(&fixture.actor, &fixture.worn, 1, "door"));
  CuAssertIntEquals(tc, 0, IN_ROOM(&fixture.actor));
  CuAssertIntEquals(tc, TRUE, rol_bloodstone_portal(&fixture.actor, &fixture.worn, 1, "portal"));
  CuAssertIntEquals(tc, 1, IN_ROOM(&fixture.actor));
  CuAssertTrue(tc, GET_HIT(&fixture.actor) >= 80 && GET_HIT(&fixture.actor) <= 99);
  CuAssertTrue(tc, GET_MOVE(&fixture.actor) >= 70 && GET_MOVE(&fixture.actor) <= 99);

  char_from_room(&fixture.actor);
  char_to_room(&fixture.actor, 0);
  GET_LEVEL(&fixture.actor) = LVL_IMMORT;
  GET_HIT(&fixture.actor) = 100;
  GET_MOVE(&fixture.actor) = 100;
  CuAssertIntEquals(tc, TRUE, rol_bloodstone_portal(&fixture.actor, &fixture.worn, 1, "portal"));
  CuAssertIntEquals(tc, 1, IN_ROOM(&fixture.actor));
  CuAssertIntEquals(tc, 100, GET_HIT(&fixture.actor));
  CuAssertIntEquals(tc, 100, GET_MOVE(&fixture.actor));

  char_from_room(&fixture.actor);
  char_to_room(&fixture.actor, 0);
  SET_BIT_AR(ROOM_FLAGS(1), ROOM_DEATH);
  CuAssertIntEquals(tc, TRUE, rol_bloodstone_portal(&fixture.actor, &fixture.worn, 1, "portal"));
  CuAssertIntEquals(tc, 0, IN_ROOM(&fixture.actor));
  REMOVE_BIT_AR(ROOM_FLAGS(1), ROOM_DEATH);

  GET_OBJ_VAL(&fixture.worn, 0) = 9999;
  CuAssertIntEquals(tc, TRUE, rol_bloodstone_portal(&fixture.actor, &fixture.worn, 1, "portal"));
  CuAssertIntEquals(tc, 0, IN_ROOM(&fixture.actor));

  CuAssertTrue(tc, rol_bloodstone_portal_survives(10, 20));
  CuAssertTrue(tc, !rol_bloodstone_portal_survives(9, 20));
  CuAssertTrue(tc, rol_bloodstone_portal_survives(-10, 0));
  CuAssertTrue(tc, !rol_bloodstone_portal_survives(-10, 1));

  fixture.rooms[0].contents = NULL;
  IN_ROOM(&fixture.worn) = NOWHERE;
  complete_cmd_info = saved_complete_cmd_info;
  spec_mechanics_end(&fixture);
}

void Test_spec_rol_portal_door_preserves_destination_and_race_gates(CuTest *tc)
{
  struct spec_mechanics_fixture fixture;
  struct command_info commands[2];
  struct command_info *saved_complete_cmd_info;

  spec_mechanics_begin(&fixture);
  memset(commands, 0, sizeof(commands));
  commands[1].command = "enter";
  saved_complete_cmd_info = complete_cmd_info;
  complete_cmd_info = commands;

  fixture.worn.name = "rainbow portal";
  fixture.worn.short_description = "a rainbow portal";
  IN_ROOM(&fixture.worn) = 0;
  fixture.rooms[0].contents = &fixture.worn;
  GET_OBJ_VAL(&fixture.worn, 0) = 6101;

  CuAssertTrue(tc, rol_portal_door_race_allows(false, RACE_HUMAN));
  CuAssertTrue(tc, !rol_portal_door_race_allows(false, RACE_DROW));
  CuAssertTrue(tc, !rol_portal_door_race_allows(true, RACE_HUMAN));
  CuAssertTrue(tc, rol_portal_door_race_allows(true, RACE_DROW));
  CuAssertIntEquals(tc, FALSE, rol_portal_door(&fixture.actor, &fixture.worn, 1, "door"));
  CuAssertIntEquals(tc, TRUE, rol_portal_door(&fixture.actor, &fixture.worn, 1, "portal"));
  CuAssertIntEquals(tc, 1, IN_ROOM(&fixture.actor));

  char_from_room(&fixture.actor);
  char_to_room(&fixture.actor, 0);
  GET_REAL_RACE(&fixture.actor) = RACE_DROW;
  CuAssertIntEquals(tc, TRUE, rol_portal_door(&fixture.actor, &fixture.worn, 1, "portal"));
  CuAssertIntEquals(tc, 0, IN_ROOM(&fixture.actor));
  GET_LEVEL(&fixture.actor) = LVL_IMMORT;
  CuAssertIntEquals(tc, TRUE, rol_portal_door(&fixture.actor, &fixture.worn, 1, "portal"));
  CuAssertIntEquals(tc, 1, IN_ROOM(&fixture.actor));

  char_from_room(&fixture.actor);
  char_to_room(&fixture.actor, 0);
  GET_REAL_RACE(&fixture.actor) = RACE_HUMAN;
  SET_BIT_AR(ROOM_FLAGS(1), ROOM_ARENA);
  CuAssertIntEquals(tc, TRUE, rol_portal_door(&fixture.actor, &fixture.worn, 1, "portal"));
  CuAssertIntEquals(tc, 0, IN_ROOM(&fixture.actor));
  REMOVE_BIT_AR(ROOM_FLAGS(1), ROOM_ARENA);

  fixture.rooms[0].contents = NULL;
  IN_ROOM(&fixture.worn) = NOWHERE;
  complete_cmd_info = saved_complete_cmd_info;
  spec_mechanics_end(&fixture);
}

void Test_spec_rol_travel_portal_preserves_profiles_and_common_transit(CuTest *tc)
{
  struct spec_mechanics_fixture fixture;
  struct player_special_data player_specials;
  struct command_info commands[3];
  struct command_info *saved_complete_cmd_info;

  spec_mechanics_begin(&fixture);
  memset(&player_specials, 0, sizeof(player_specials));
  memset(commands, 0, sizeof(commands));
  commands[1].command = "enter";
  commands[2].command = "use";
  saved_complete_cmd_info = complete_cmd_info;
  complete_cmd_info = commands;

  CuAssertIntEquals(tc, 0, rol_travel_portal_destination_slot(2000882, 0));
  CuAssertIntEquals(tc, 0, rol_travel_portal_destination_slot(2005515, 3));
  CuAssertIntEquals(tc, 0, rol_travel_portal_destination_slot(2021500, 0));
  CuAssertIntEquals(tc, 0, rol_travel_portal_destination_slot(2041941, 0));
  CuAssertIntEquals(tc, 0, rol_travel_portal_destination_slot(2008112, 0));
  CuAssertIntEquals(tc, 3, rol_travel_portal_destination_slot(2008113, 3));
  CuAssertIntEquals(tc, -1, rol_travel_portal_destination_slot(2008113, 4));
  CuAssertIntEquals(tc, -1, rol_travel_portal_destination_slot(2003088, 0));
  CuAssertIntEquals(tc, -1, rol_travel_portal_destination_slot(999999, 0));
  CuAssertIntEquals(tc, 2005582, rol_travel_portal_fixed_destination(2003088));
  CuAssertIntEquals(tc, -1, rol_travel_portal_fixed_destination(2000882));
  CuAssertIntEquals(tc, 2041900, rol_travel_portal_reward_vnum(2041941));
  CuAssertIntEquals(tc, -1, rol_travel_portal_reward_vnum(2005515));
  CuAssertTrue(tc, rol_travel_portal_actor_allowed(2000882, &fixture.actor));
  CuAssertTrue(tc, !rol_travel_portal_actor_allowed(2021500, &fixture.actor));

  fixture.worn.name = "dimensional fold";
  fixture.worn.short_description = "a dimensional fold";
  fixture.object_indexes[0].vnum = 2000882;
  GET_OBJ_VAL(&fixture.worn, 0) = 6101;
  IN_ROOM(&fixture.worn) = 0;
  fixture.rooms[0].contents = &fixture.worn;
  CuAssertIntEquals(tc, FALSE, rol_travel_portal(&fixture.actor, &fixture.worn, 2, "fold"));
  CuAssertIntEquals(tc, TRUE, rol_travel_portal(&fixture.actor, &fixture.worn, 1, "fold"));
  CuAssertIntEquals(tc, 1, IN_ROOM(&fixture.actor));

  char_from_room(&fixture.actor);
  char_to_room(&fixture.actor, 0);
  fixture.worn.name = "glowing portal";
  fixture.worn.short_description = "the glowing portal";
  fixture.object_indexes[0].vnum = 2005515;
  GET_OBJ_VAL(&fixture.worn, 0) = 6101;
  GET_OBJ_VAL(&fixture.worn, 1) = 25;
  GET_HIT(&fixture.actor) = 100;
  CuAssertIntEquals(tc, TRUE, rol_travel_portal(&fixture.actor, &fixture.worn, 1, "portal"));
  CuAssertIntEquals(tc, 1, IN_ROOM(&fixture.actor));
  CuAssertIntEquals(tc, 75, GET_HIT(&fixture.actor));

  char_from_room(&fixture.actor);
  char_to_room(&fixture.actor, 0);
  REMOVE_BIT_AR(MOB_FLAGS(&fixture.actor), MOB_ISNPC);
  fixture.actor.player_specials = &player_specials;
  GET_LEVEL(&fixture.actor) = 20;
  GET_REAL_RACE(&fixture.actor) = RACE_ELF;
  player_specials.saved.class_level[CLASS_CLERIC] = 1;
  CuAssertTrue(tc, rol_travel_portal_actor_allowed(2008112, &fixture.actor));
  CuAssertTrue(tc, rol_travel_portal_actor_allowed(2021500, &fixture.actor));
  CuAssertTrue(tc, !rol_travel_portal_actor_allowed(2003088, &fixture.actor));
  player_specials.saved.class_level[CLASS_WIZARD] = 1;
  CuAssertTrue(tc, rol_travel_portal_actor_allowed(2003088, &fixture.actor));

  fixture.actor.player_specials = &dummy_mob;
  SET_BIT_AR(MOB_FLAGS(&fixture.actor), MOB_ISNPC);
  fixture.rooms[0].contents = NULL;
  IN_ROOM(&fixture.worn) = NOWHERE;
  complete_cmd_info = saved_complete_cmd_info;
  spec_mechanics_end(&fixture);
}

void Test_spec_rol_auto_distributor_moves_mortal_within_zone(CuTest *tc)
{
  struct spec_mechanics_fixture fixture;

  spec_mechanics_begin(&fixture);

  CuAssertIntEquals(tc, TRUE, rol_auto_distributor(&fixture.actor, &fixture.rooms[0], 1, "look"));
  CuAssertTrue(tc, IN_ROOM(&fixture.actor) == 0 || IN_ROOM(&fixture.actor) == 1);
  char_from_room(&fixture.actor);
  char_to_room(&fixture.actor, 0);

  REMOVE_BIT_AR(MOB_FLAGS(&fixture.actor), MOB_ISNPC);
  GET_LEVEL(&fixture.actor) = LVL_IMMORT;
  CuAssertIntEquals(tc, FALSE, rol_auto_distributor(&fixture.actor, &fixture.rooms[0], 1, "look"));
  CuAssertIntEquals(tc, 0, IN_ROOM(&fixture.actor));

  spec_mechanics_end(&fixture);
}

void Test_spec_rol_shadow_giant_preserves_spook_rules(CuTest *tc)
{
  struct spec_mechanics_fixture fixture;
  int amount;

  spec_mechanics_begin(&fixture);

  amount = rol_shadow_giant_spook_damage(false);
  CuAssertTrue(tc, amount >= 25 && amount <= 200);
  amount = rol_shadow_giant_spook_damage(true);
  CuAssertTrue(tc, amount >= 12 && amount <= 100);

  CuAssertTrue(tc, rol_shadow_giant_stun_succeeds(30, 54, 5));
  CuAssertTrue(tc, !rol_shadow_giant_stun_succeeds(30, 55, 5));
  CuAssertTrue(tc, !rol_shadow_giant_spook_immune(&fixture.target));
  CuAssertTrue(tc, rol_shadow_giant_spook_immune(NULL));

  SET_BIT_AR(MOB_FLAGS(&fixture.target), MOB_ROL_DEMON);
  CuAssertTrue(tc, rol_shadow_giant_spook_immune(&fixture.target));
  REMOVE_BIT_AR(MOB_FLAGS(&fixture.target), MOB_ROL_DEMON);
  SET_BIT_AR(MOB_FLAGS(&fixture.target), MOB_ROL_ANGEL);
  CuAssertTrue(tc, rol_shadow_giant_spook_immune(&fixture.target));
  REMOVE_BIT_AR(MOB_FLAGS(&fixture.target), MOB_ROL_ANGEL);
  fixture.target.player.race = RACE_TYPE_DRAGON;
  CuAssertTrue(tc, rol_shadow_giant_spook_immune(&fixture.target));

  spec_mechanics_end(&fixture);
}

void Test_spec_rol_guild_guard_preserves_active_gate_rules(CuTest *tc)
{
  struct spec_mechanics_fixture fixture;
  struct player_special_data player_specials;
  struct char_data *target;

  spec_mechanics_begin(&fixture);
  target = &fixture.target;
  memset(&player_specials, 0, sizeof(player_specials));
  REMOVE_BIT_AR(MOB_FLAGS(target), MOB_ISNPC);
  target->player_specials = &player_specials;

  CuAssertTrue(tc, !rol_guild_guard_allows(2004128, NORTH, target));
  CuAssertTrue(tc, rol_guild_guard_allows(2004128, SOUTH, target));
  CuAssertTrue(tc, !rol_guild_guard_allows(2008200, WEST, target));
  CLASS_LEVEL(target, CLASS_ROGUE) = 1;
  CuAssertTrue(tc, rol_guild_guard_allows(2008200, WEST, target));
  CLASS_LEVEL(target, CLASS_ROGUE) = 0;

  CuAssertTrue(tc, !rol_guild_guard_allows(2008113, SOUTH, target));
  CLASS_LEVEL(target, CLASS_WIZARD) = 1;
  CuAssertTrue(tc, rol_guild_guard_allows(2008113, SOUTH, target));
  CLASS_LEVEL(target, CLASS_WIZARD) = 0;
  CLASS_LEVEL(target, CLASS_SORCERER) = 1;
  CuAssertTrue(tc, rol_guild_guard_allows(2008113, SOUTH, target));
  CLASS_LEVEL(target, CLASS_SORCERER) = 0;

  GET_REAL_RACE(target) = RACE_HUMAN;
  CuAssertTrue(tc, !rol_guild_guard_allows(2008087, EAST, target));
  GET_REAL_RACE(target) = RACE_ELF;
  CuAssertTrue(tc, rol_guild_guard_allows(2008087, EAST, target));
  GET_REAL_RACE(target) = RACE_HALF_ELF;
  CuAssertTrue(tc, rol_guild_guard_allows(2008087, EAST, target));

  CuAssertTrue(tc, !rol_guild_guard_allows(2034406, WEST, target));
  CLASS_LEVEL(target, CLASS_ASSASSIN) = 1;
  CuAssertTrue(tc, rol_guild_guard_allows(2034406, WEST, target));
  CuAssertTrue(tc, !rol_guild_guard_allows(2034406, EAST, target));
  CLASS_LEVEL(target, CLASS_ASSASSIN) = 0;
  CLASS_LEVEL(target, CLASS_ROGUE) = 1;
  CuAssertTrue(tc, rol_guild_guard_allows(2034406, EAST, target));
  CuAssertTrue(tc, rol_guild_guard_allows(2007837, WEST, target));
  CuAssertTrue(tc, rol_guild_guard_allows(2007864, WEST, target));
  CLASS_LEVEL(target, CLASS_ROGUE) = 0;

  CuAssertTrue(tc, !rol_guild_guard_allows(2007669, NORTH, target));
  CLASS_LEVEL(target, CLASS_BLACKGUARD) = 1;
  CuAssertTrue(tc, rol_guild_guard_allows(2007669, NORTH, target));
  CLASS_LEVEL(target, CLASS_BLACKGUARD) = 0;
  CLASS_LEVEL(target, CLASS_NECROMANCER) = 1;
  CuAssertTrue(tc, rol_guild_guard_allows(2007880, WEST, target));
  CLASS_LEVEL(target, CLASS_NECROMANCER) = 0;
  CLASS_LEVEL(target, CLASS_CLERIC) = 1;
  CuAssertTrue(tc, rol_guild_guard_allows(2007817, DOWN, target));
  CuAssertTrue(tc, rol_guild_guard_allows(2003067, NORTH, target));
  CLASS_LEVEL(target, CLASS_CLERIC) = 0;

  CLASS_LEVEL(target, CLASS_WARRIOR) = 1;
  CuAssertTrue(tc, rol_guild_guard_allows(2005510, EAST, target));
  CLASS_LEVEL(target, CLASS_WARRIOR) = 0;
  CLASS_LEVEL(target, CLASS_MONK) = 1;
  CuAssertTrue(tc, rol_guild_guard_allows(2005520, SOUTH, target));
  CLASS_LEVEL(target, CLASS_MONK) = 0;
  CLASS_LEVEL(target, CLASS_WIZARD) = 1;
  CuAssertTrue(tc, rol_guild_guard_allows(2005570, EAST, target));
  CuAssertTrue(tc, rol_guild_guard_allows(2005572, EAST, target));
  CLASS_LEVEL(target, CLASS_WIZARD) = 0;
  CLASS_LEVEL(target, CLASS_PALADIN) = 1;
  CuAssertTrue(tc, rol_guild_guard_allows(2005500, EAST, target));
  CLASS_LEVEL(target, CLASS_PALADIN) = 0;
  CLASS_LEVEL(target, CLASS_BARD) = 1;
  CuAssertTrue(tc, rol_guild_guard_allows(2005534, UP, target));
  CLASS_LEVEL(target, CLASS_BARD) = 0;
  CLASS_LEVEL(target, CLASS_RANGER) = 1;
  CuAssertTrue(tc, rol_guild_guard_allows(2005540, SOUTH, target));
  CLASS_LEVEL(target, CLASS_RANGER) = 0;
  CLASS_LEVEL(target, CLASS_DRUID) = 1;
  CuAssertTrue(tc, rol_guild_guard_allows(2005560, EAST, target));
  CLASS_LEVEL(target, CLASS_DRUID) = 0;
  CLASS_LEVEL(target, CLASS_SUMMONER) = 1;
  CuAssertTrue(tc, rol_guild_guard_allows(2003038, SOUTH, target));
  CLASS_LEVEL(target, CLASS_SUMMONER) = 0;
  CLASS_LEVEL(target, CLASS_BERSERKER) = 1;
  CuAssertTrue(tc, rol_guild_guard_allows(2003055, SOUTH, target));
  CLASS_LEVEL(target, CLASS_BERSERKER) = 0;
  CLASS_LEVEL(target, CLASS_ROGUE) = 1;
  CuAssertTrue(tc, rol_guild_guard_allows(2003283, EAST, target));
  CuAssertTrue(tc, rol_guild_guard_allows(2002951, NORTH, target));
  CLASS_LEVEL(target, CLASS_ROGUE) = 0;

  CuAssertTrue(tc, !rol_guild_guard_allows(2005510, EAST, target));
  CuAssertTrue(tc, !rol_guild_guard_allows(2005570, EAST, target));
  CuAssertTrue(tc, !rol_guild_guard_allows(2005500, EAST, target));
  CuAssertTrue(tc, !rol_guild_guard_allows(2005534, UP, target));
  CuAssertTrue(tc, !rol_guild_guard_allows(2005540, SOUTH, target));
  CuAssertTrue(tc, !rol_guild_guard_allows(2005560, EAST, target));
  CuAssertTrue(tc, !rol_guild_guard_allows(2005572, EAST, target));
  CuAssertTrue(tc, !rol_guild_guard_allows(2003038, SOUTH, target));

  CuAssertTrue(tc, rol_guild_guard_protects(2008200));
  CuAssertTrue(tc, rol_guild_guard_protects(2007669));
  CuAssertTrue(tc, rol_guild_guard_protects(2007880));
  CuAssertTrue(tc, rol_guild_guard_protects(2050624));
  CuAssertTrue(tc, rol_guild_guard_protects(2005500));
  CuAssertTrue(tc, rol_guild_guard_protects(2005534));
  CuAssertTrue(tc, rol_guild_guard_protects(2005540));
  CuAssertTrue(tc, rol_guild_guard_protects(2005560));
  CuAssertTrue(tc, rol_guild_guard_protects(2005572));
  CuAssertTrue(tc, rol_guild_guard_protects(2003038));
  CuAssertTrue(tc, !rol_guild_guard_protects(2004128));
  CuAssertTrue(tc, !rol_guild_guard_protects(9999999));

  CuAssertIntEquals(tc, 2007670, rol_guild_guard_passage_destination(2007669, NORTH));
  CuAssertIntEquals(tc, 2007818, rol_guild_guard_passage_destination(2007817, DOWN));
  CuAssertIntEquals(tc, 2007843, rol_guild_guard_passage_destination(2007837, WEST));
  CuAssertIntEquals(tc, 2007845, rol_guild_guard_passage_destination(2007844, EAST));
  CuAssertIntEquals(tc, 2007865, rol_guild_guard_passage_destination(2007864, WEST));
  CuAssertIntEquals(tc, 2007881, rol_guild_guard_passage_destination(2007880, WEST));
  CuAssertIntEquals(tc, 2002952, rol_guild_guard_passage_destination(2002951, NORTH));
  CuAssertIntEquals(tc, 2003056, rol_guild_guard_passage_destination(2003055, SOUTH));
  CuAssertIntEquals(tc, 2003068, rol_guild_guard_passage_destination(2003067, NORTH));
  CuAssertIntEquals(tc, 2003284, rol_guild_guard_passage_destination(2003283, EAST));
  CuAssertIntEquals(tc, 2005511, rol_guild_guard_passage_destination(2005510, EAST));
  CuAssertIntEquals(tc, 2005521, rol_guild_guard_passage_destination(2005520, SOUTH));
  CuAssertIntEquals(tc, 2005571, rol_guild_guard_passage_destination(2005570, EAST));
  CuAssertIntEquals(tc, 2005501, rol_guild_guard_passage_destination(2005500, EAST));
  CuAssertIntEquals(tc, 2005535, rol_guild_guard_passage_destination(2005534, UP));
  CuAssertIntEquals(tc, 2005541, rol_guild_guard_passage_destination(2005540, SOUTH));
  CuAssertIntEquals(tc, 2005561, rol_guild_guard_passage_destination(2005560, EAST));
  CuAssertIntEquals(tc, 2005573, rol_guild_guard_passage_destination(2005572, EAST));
  CuAssertIntEquals(tc, 2003039, rol_guild_guard_passage_destination(2003038, SOUTH));
  CuAssertIntEquals(tc, 0, rol_guild_guard_passage_destination(2007669, SOUTH));
  CuAssertIntEquals(tc, 0, rol_guild_guard_passage_destination(2008200, WEST));
  CuAssertTrue(tc, rol_guild_guard_trips_rejected(2002951, NORTH));
  CuAssertTrue(tc, !rol_guild_guard_trips_rejected(2002951, SOUTH));
  CuAssertTrue(tc, !rol_guild_guard_trips_rejected(2005510, EAST));

  target->player_specials = &dummy_mob;
  SET_BIT_AR(MOB_FLAGS(target), MOB_ISNPC);
  spec_mechanics_end(&fixture);
}

void Test_spec_rol_command_sentinel_preserves_passage_and_glyph_rules(CuTest *tc)
{
  struct spec_mechanics_fixture fixture;
  struct player_special_data player_specials;
  struct char_data *target;

  spec_mechanics_begin(&fixture);
  target = &fixture.target;
  memset(&player_specials, 0, sizeof(player_specials));
  REMOVE_BIT_AR(MOB_FLAGS(target), MOB_ISNPC);
  target->player_specials = &player_specials;

  GET_LEVEL(target) = 11;
  GET_REAL_RACE(target) = RACE_HUMAN;
  CuAssertTrue(tc, rol_command_sentinel_blocks_passage(2081508, 2081596, SOUTH, target, 0));
  GET_LEVEL(target) = 10;
  CuAssertTrue(tc, !rol_command_sentinel_blocks_passage(2081508, 2081596, SOUTH, target, 0));
  GET_LEVEL(target) = 30;
  GET_REAL_RACE(target) = RACE_DROW;
  CuAssertTrue(tc, !rol_command_sentinel_blocks_passage(2081508, 2081596, SOUTH, target, 0));

  GET_REAL_RACE(target) = RACE_HUMAN;
  CuAssertTrue(tc, rol_command_sentinel_blocks_passage(2010301, 2010320, SOUTH, target, 0));
  GET_REAL_RACE(target) = RACE_HALF_ORC;
  CuAssertTrue(tc, !rol_command_sentinel_blocks_passage(2010301, 2010320, SOUTH, target, 0));

  GET_LEVEL(target) = 21;
  CuAssertTrue(tc, rol_command_sentinel_blocks_passage(2010302, 2010302, SOUTH, target, 0));
  GET_LEVEL(target) = 20;
  CuAssertTrue(tc, !rol_command_sentinel_blocks_passage(2010302, 2010302, SOUTH, target, 0));

  CuAssertTrue(tc, !rol_command_sentinel_blocks_passage(2001438, 2001483, WEST, target, 20));
  CuAssertTrue(tc, rol_command_sentinel_blocks_passage(2001438, 2001483, WEST, target, 21));
  CuAssertTrue(tc, !rol_command_sentinel_blocks_passage(2001438, 2001483, EAST, target, 100));

  GET_LEVEL(target) = LVL_IMMORT;
  CuAssertTrue(tc, !rol_command_sentinel_blocks_passage(2001438, 2001483, WEST, target, 100));

  GET_LEVEL(target) = 20;
  CuAssertTrue(tc, !rol_command_sentinel_is_necromancer(target));
  CLASS_LEVEL(target, CLASS_NECROMANCER) = 1;
  CuAssertTrue(tc, rol_command_sentinel_is_necromancer(target));
  CuAssertIntEquals(tc, 1, rol_command_sentinel_glyph_damage(target));
  SET_BIT_AR(AFF_FLAGS(target), AFF_MINOR_GLOBE);
  CuAssertIntEquals(tc, 25, rol_command_sentinel_glyph_damage(target));
  REMOVE_BIT_AR(AFF_FLAGS(target), AFF_MINOR_GLOBE);
  SET_BIT_AR(AFF_FLAGS(target), AFF_GLOBE_OF_INVULN);
  CuAssertIntEquals(tc, 25, rol_command_sentinel_glyph_damage(target));
  REMOVE_BIT_AR(AFF_FLAGS(target), AFF_GLOBE_OF_INVULN);

  target->player_specials = &dummy_mob;
  SET_BIT_AR(MOB_FLAGS(target), MOB_ISNPC);
  spec_mechanics_end(&fixture);
}

void Test_spec_rol_toll_keeper_preserves_fees_destinations_and_tickets(CuTest *tc)
{
  CuAssertIntEquals(tc, 20, rol_toll_keeper_fee_gold(2007210));
  CuAssertIntEquals(tc, 10, rol_toll_keeper_fee_gold(2007335));
  CuAssertIntEquals(tc, 500, rol_toll_keeper_fee_gold(2011542));
  CuAssertIntEquals(tc, 5, rol_toll_keeper_fee_gold(2001919));
  CuAssertIntEquals(tc, 5, rol_toll_keeper_fee_gold(2014202));
  CuAssertIntEquals(tc, 0, rol_toll_keeper_fee_gold(999999));

  CuAssertIntEquals(tc, 2007681, rol_toll_keeper_destination(2007210, true));
  CuAssertIntEquals(tc, 2001862, rol_toll_keeper_destination(2001919, true));
  CuAssertIntEquals(tc, 2001864, rol_toll_keeper_destination(2001919, false));
  CuAssertIntEquals(tc, 2014236, rol_toll_keeper_destination(2014202, true));
  CuAssertIntEquals(tc, 2014238, rol_toll_keeper_destination(2014202, false));
  CuAssertIntEquals(tc, -1, rol_toll_keeper_destination(999999, false));

  CuAssertTrue(tc, rol_toll_keeper_ticket_matches(2011106, 2005313, 2011100, 2005341));
  CuAssertTrue(tc, rol_toll_keeper_ticket_matches(2011306, 2005399, 2011300, 2005341));
  CuAssertTrue(tc, rol_toll_keeper_ticket_matches(2098357, 2098425, 2098451, 2000046));
  CuAssertTrue(tc, rol_toll_keeper_ticket_matches(2098358, 2014312, 2098451, 2000046));
  CuAssertTrue(tc, !rol_toll_keeper_ticket_matches(2011106, 2005313, 2011300, 2005341));
  CuAssertTrue(tc, !rol_toll_keeper_ticket_matches(2011106, 2005313, 2011100, 2000046));
  CuAssertTrue(tc, !rol_toll_keeper_ticket_matches(2007210, 2007680, -1, -1));

  CuAssertTrue(tc, rol_toll_keeper_payment_syntax_valid(2007210, "20 gold tax knight"));
  CuAssertTrue(tc, rol_toll_keeper_payment_syntax_valid(2007210, "20 coins tax knight"));
  CuAssertTrue(tc, !rol_toll_keeper_payment_syntax_valid(2007210, "2 platinum tax knight"));
  CuAssertTrue(tc, !rol_toll_keeper_payment_syntax_valid(2007210, "sword tax knight"));
  CuAssertTrue(tc, rol_toll_keeper_payment_syntax_valid(2007335, "anything"));
  CuAssertTrue(tc, !rol_toll_keeper_payment_syntax_valid(2001919, "5 gold troll"));
  CuAssertTrue(tc, !rol_toll_keeper_payment_syntax_valid(999999, "20 gold nobody"));
}

void Test_rol_banana_peel_outcomes_preserve_source_branches(CuTest *tc)
{
  CuAssertIntEquals(tc, ROL_BANANA_PEEL_AVOID, rol_banana_peel_classify(5, 1));
  CuAssertIntEquals(tc, ROL_BANANA_PEEL_KNOCKOUT, rol_banana_peel_classify(4, 1));
  CuAssertIntEquals(tc, ROL_BANANA_PEEL_FALL, rol_banana_peel_classify(1, 2));
  CuAssertIntEquals(tc, ROL_BANANA_PEEL_FALL, rol_banana_peel_classify(1, 5));
  CuAssertIntEquals(tc, ROL_BANANA_PEEL_STUMBLE, rol_banana_peel_classify(1, 6));
  CuAssertIntEquals(tc, ROL_BANANA_PEEL_STUMBLE, rol_banana_peel_classify(1, 10));
  CuAssertIntEquals(tc, ROL_BANANA_PEEL_DANCE, rol_banana_peel_classify(1, 11));
}

void Test_rol_undead_drain_profiles_preserve_source_variants(CuTest *tc)
{
  static const struct
  {
    int vnum;
    int chance_sides;
    int marker;
    int armor;
    int dexterity;
    int strength;
    int will;
    int fortitude;
    int slow;
  } expected[] = {
      {2001256, 16, AFFECT_ROL_UNDEAD_MELEE_DRAIN, -1, -5, 0, 0, 0, 0},
      {2001257, 21, AFFECT_ROL_UNDEAD_SPELL_DRAIN, 0, 0, -5, -1, 0, 0},
      {2001258, 16, AFFECT_ROL_UNDEAD_MELEE_DRAIN, -2, -10, 0, 0, 0, 0},
      {2001259, 21, AFFECT_ROL_UNDEAD_MELEE_DRAIN, -2, -15, 0, 0, 0, 2},
      {2001260, 21, AFFECT_ROL_UNDEAD_SPELL_DRAIN, 0, 0, -10, -1, 0, 0},
      {2001261, 21, AFFECT_ROL_UNDEAD_MELEE_DRAIN, -3, -15, -15, 0, 0, -1},
      {2001262, 21, AFFECT_ROL_UNDEAD_SPELL_DRAIN, 0, 0, -10, -1, -1, 0},
  };
  int chance_sides;
  int marker;
  int armor;
  int dexterity;
  int strength;
  int will;
  int fortitude;
  int slow;
  size_t index;

  for (index = 0; index < sizeof(expected) / sizeof(expected[0]); index++)
  {
    CuAssertTrue(tc, rol_undead_drain_profile(expected[index].vnum, &chance_sides, &marker, &armor,
                                              &dexterity, &strength, &will, &fortitude, &slow));
    CuAssertIntEquals(tc, expected[index].chance_sides, chance_sides);
    CuAssertIntEquals(tc, expected[index].marker, marker);
    CuAssertIntEquals(tc, expected[index].armor, armor);
    CuAssertIntEquals(tc, expected[index].dexterity, dexterity);
    CuAssertIntEquals(tc, expected[index].strength, strength);
    CuAssertIntEquals(tc, expected[index].will, will);
    CuAssertIntEquals(tc, expected[index].fortitude, fortitude);
    CuAssertIntEquals(tc, expected[index].slow, slow);
  }

  CuAssertTrue(tc,
               !rol_undead_drain_profile(9999999, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL));
}

void Test_spec_rol_class_guilds_preserve_family_gates_for_multiclass_players(CuTest *tc)
{
  struct spec_mechanics_fixture fixture;
  struct player_special_data player_specials;
  struct char_data *target;

  spec_mechanics_begin(&fixture);
  target = &fixture.target;
  memset(&player_specials, 0, sizeof(player_specials));
  REMOVE_BIT_AR(MOB_FLAGS(target), MOB_ISNPC);
  target->player_specials = &player_specials;

  CuAssertTrue(tc, !rol_class_guild_allows(target, ROL_GUILD_FAMILY_MAGE));
  CuAssertTrue(tc, !rol_class_guild_allows(target, ROL_GUILD_FAMILY_THIEF));
  CuAssertTrue(tc, !rol_class_guild_allows(target, ROL_GUILD_FAMILY_WARRIOR));
  CuAssertTrue(tc, !rol_class_guild_allows(target, ROL_GUILD_FAMILY_CLERIC));
  CuAssertTrue(tc, !rol_class_guild_allows(target, ROL_GUILD_FAMILY_BARD));

  CLASS_LEVEL(target, CLASS_WIZARD) = 1;
  CuAssertTrue(tc, rol_class_guild_allows(target, ROL_GUILD_FAMILY_MAGE));
  CuAssertTrue(tc, !rol_class_guild_allows(target, ROL_GUILD_FAMILY_CLERIC));
  CLASS_LEVEL(target, CLASS_BARD) = 1;
  CuAssertTrue(tc, rol_class_guild_allows(target, ROL_GUILD_FAMILY_THIEF));
  CuAssertTrue(tc, rol_class_guild_allows(target, ROL_GUILD_FAMILY_BARD));
  CLASS_LEVEL(target, CLASS_PALADIN) = 1;
  CuAssertTrue(tc, rol_class_guild_allows(target, ROL_GUILD_FAMILY_WARRIOR));
  CLASS_LEVEL(target, CLASS_DRUID) = 1;
  CuAssertTrue(tc, rol_class_guild_allows(target, ROL_GUILD_FAMILY_CLERIC));

  CuAssertTrue(tc, !rol_class_guild_allows(target, (enum rol_guild_family)99));
  CuAssertTrue(tc, !rol_class_guild_allows(NULL, ROL_GUILD_FAMILY_MAGE));

  target->player_specials = &dummy_mob;
  SET_BIT_AR(MOB_FLAGS(target), MOB_ISNPC);
  CuAssertTrue(tc, !rol_class_guild_allows(target, ROL_GUILD_FAMILY_MAGE));
  spec_mechanics_end(&fixture);
}

void Test_spec_rol_waterdeep_guilds_preserve_room_class_gates(CuTest *tc)
{
  struct spec_mechanics_fixture fixture;
  struct player_special_data player_specials;
  struct char_data *target;

  spec_mechanics_begin(&fixture);
  target = &fixture.target;
  memset(&player_specials, 0, sizeof(player_specials));
  REMOVE_BIT_AR(MOB_FLAGS(target), MOB_ISNPC);
  target->player_specials = &player_specials;

  CuAssertTrue(tc, !rol_waterdeep_guild_allows(2005505, target));
  CuAssertTrue(tc, !rol_waterdeep_guild_allows(9999999, target));
  CuAssertTrue(tc, !rol_waterdeep_guild_allows(2005505, NULL));

  CLASS_LEVEL(target, CLASS_PALADIN) = 1;
  CuAssertTrue(tc, rol_waterdeep_guild_allows(2005505, target));
  CuAssertTrue(tc, rol_waterdeep_guild_allows(2003061, target));
  CLASS_LEVEL(target, CLASS_WARRIOR) = 1;
  CuAssertTrue(tc, rol_waterdeep_guild_allows(2005512, target));
  CLASS_LEVEL(target, CLASS_MONK) = 1;
  CuAssertTrue(tc, rol_waterdeep_guild_allows(2005524, target));
  CLASS_LEVEL(target, CLASS_BARD) = 1;
  CuAssertTrue(tc, rol_waterdeep_guild_allows(2005537, target));
  CLASS_LEVEL(target, CLASS_RANGER) = 1;
  CuAssertTrue(tc, rol_waterdeep_guild_allows(2005544, target));
  CLASS_LEVEL(target, CLASS_DRUID) = 1;
  CuAssertTrue(tc, rol_waterdeep_guild_allows(2005568, target));
  CuAssertTrue(tc, rol_waterdeep_guild_allows(2003073, target));
  CLASS_LEVEL(target, CLASS_WIZARD) = 1;
  CuAssertTrue(tc, rol_waterdeep_guild_allows(2005581, target));
  CuAssertTrue(tc, rol_waterdeep_guild_allows(2003044, target));
  CLASS_LEVEL(target, CLASS_ROGUE) = 1;
  CuAssertTrue(tc, rol_waterdeep_guild_allows(2003289, target));
  CuAssertTrue(tc, rol_waterdeep_guild_allows(2002956, target));

  target->player_specials = &dummy_mob;
  SET_BIT_AR(MOB_FLAGS(target), MOB_ISNPC);
  CuAssertTrue(tc, !rol_waterdeep_guild_allows(2005505, target));
  spec_mechanics_end(&fixture);
}

void Test_spec_rol_sister_knight_preserves_family_identity_and_alert_guard(CuTest *tc)
{
  struct spec_mechanics_fixture fixture;
  struct char_data helper;
  struct char_data *saved_character_list;
  struct player_special_data player_specials;

  spec_mechanics_begin(&fixture);
  memset(&player_specials, 0, sizeof(player_specials));
  spec_mechanics_initialize_npc(&helper, "another sister knight", 0);
  saved_character_list = character_list;
  character_list = &helper;
  fixture.mobile_indexes[0].vnum = 2026218;
  GET_MOB_RNUM(&fixture.actor) = 0;
  GET_MOB_RNUM(&helper) = 0;
  REMOVE_BIT_AR(MOB_FLAGS(&fixture.target), MOB_ISNPC);
  fixture.target.player.name = "sister attacker";
  fixture.target.player_specials = &player_specials;
  FIGHTING(&fixture.actor) = &fixture.target;

  CuAssertTrue(tc, rol_sister_knight_vnum(2026218));
  CuAssertTrue(tc, rol_sister_knight_vnum(2026219));
  CuAssertTrue(tc, rol_sister_knight_vnum(2026220));
  CuAssertTrue(tc, rol_sister_knight_vnum(2026221));
  CuAssertTrue(tc, rol_sister_knight_vnum(2026222));
  CuAssertTrue(tc, !rol_sister_knight_vnum(2026217));
  CuAssertTrue(tc, !rol_sister_knight_vnum(2026223));

  CuAssertIntEquals(tc, TRUE, rol_sister_knight(&fixture.actor, &fixture.actor, 0, ""));
  CuAssertIntEquals(tc, TRUE, PROC_FIRED(&fixture.actor));
  CuAssertTrue(tc, HUNTING(&helper) == &fixture.target);
  CuAssertIntEquals(tc, FALSE, rol_sister_knight(&fixture.actor, &fixture.actor, 0, ""));

  FIGHTING(&fixture.actor) = NULL;
  CuAssertIntEquals(tc, FALSE, rol_sister_knight(&fixture.actor, &fixture.actor, 0, ""));
  CuAssertIntEquals(tc, FALSE, PROC_FIRED(&fixture.actor));

  HUNTING(&helper) = NULL;
  fixture.target.player_specials = &dummy_mob;
  SET_BIT_AR(MOB_FLAGS(&fixture.target), MOB_ISNPC);
  character_list = saved_character_list;
  spec_mechanics_end(&fixture);
}

void Test_spec_rol_alert_callers_share_profiles_without_losing_composed_breaths(CuTest *tc)
{
  struct spec_mechanics_fixture fixture;
  struct char_data helper;
  struct char_data *saved_character_list;
  struct player_special_data player_specials;

  spec_mechanics_begin(&fixture);
  memset(&player_specials, 0, sizeof(player_specials));
  spec_mechanics_initialize_npc(&helper, "a darkness helper", 0);
  saved_character_list = character_list;
  character_list = &helper;
  fixture.mobile_indexes[0].vnum = 2019920;
  fixture.mobile_indexes[1].vnum = 2019830;
  GET_MOB_RNUM(&fixture.actor) = 0;
  GET_MOB_RNUM(&helper) = 1;
  REMOVE_BIT_AR(MOB_FLAGS(&fixture.target), MOB_ISNPC);
  fixture.target.player.name = "alert target";
  fixture.target.player_specials = &player_specials;
  FIGHTING(&fixture.actor) = &fixture.target;

  CuAssertStrEquals(
      tc,
      "You will pay for attacking me mortal worms!  Denizens of Darkness, Come and Feast upon %s!",
      rol_alert_message(2019920));
  CuAssertTrue(tc, rol_alert_helper_matches(2019920, 2019830));
  CuAssertTrue(tc, !rol_alert_helper_matches(2019920, 2019860));
  CuAssertStrEquals(tc, "Come to my aid!", rol_alert_message(2062401));
  CuAssertTrue(tc, rol_alert_helper_matches(2062401, 2062421));
  CuAssertTrue(tc, !rol_alert_helper_matches(2062401, 2062422));
  CuAssertStrEquals(tc, "Children of Tiamat, come defend me from these intruders!",
                    rol_alert_message(2032622));
  CuAssertTrue(tc, rol_alert_helper_matches(2032622, 2036180));
  CuAssertTrue(tc, !rol_alert_helper_matches(2032622, 2036181));
  CuAssertIntEquals(tc, 30, rol_alert_max_distance(2032622));
  CuAssertIntEquals(tc, 100, rol_alert_max_distance(2019920));
  CuAssertIntEquals(tc, 0, rol_alert_max_distance(9999999));
  CuAssertTrue(tc, rol_alert_message(9999999) == NULL);
  CuAssertIntEquals(tc, TRUE, rol_alert_caller(&fixture.actor, &fixture.actor, 0, ""));
  CuAssertTrue(tc, fixture.actor.mob_specials.rol_alert_fired);
  CuAssertTrue(tc, HUNTING(&helper) == &fixture.target);
  CuAssertIntEquals(tc, FALSE, rol_alert_caller(&fixture.actor, &fixture.actor, 0, ""));

  FIGHTING(&fixture.actor) = NULL;
  CuAssertIntEquals(tc, FALSE, rol_alert_caller(&fixture.actor, &fixture.actor, 0, ""));
  CuAssertTrue(tc, !fixture.actor.mob_specials.rol_alert_fired);

  fixture.mobile_indexes[0].vnum = 2025406;
  fixture.actor.mob_specials.rol_alert_fired = true;
  CuAssertIntEquals(tc, FALSE, rol_breath_weapon_fire(&fixture.actor, &fixture.actor, 0, ""));
  CuAssertTrue(tc, !fixture.actor.mob_specials.rol_alert_fired);

  HUNTING(&helper) = NULL;
  fixture.target.player_specials = &dummy_mob;
  SET_BIT_AR(MOB_FLAGS(&fixture.target), MOB_ISNPC);
  character_list = saved_character_list;
  spec_mechanics_end(&fixture);
}

void Test_spec_rol_yggdrasil_release_and_death_profiles_preserve_source_outcomes(CuTest *tc)
{
  struct spec_mechanics_fixture fixture;
  struct affected_type affect;
  struct mud_event_data event;

  spec_mechanics_begin(&fixture);

  CuAssertTrue(tc, rol_yggdrasil_vnum(2062800));
  CuAssertTrue(tc, rol_yggdrasil_vnum(2062804));
  CuAssertTrue(tc, !rol_yggdrasil_vnum(2062799));
  CuAssertTrue(tc, !rol_yggdrasil_vnum(2062805));
  CuAssertIntEquals(tc, 50, rol_yggdrasil_release_move(101));

  new_affect(&affect);
  affect.spell = SPELL_ENTANGLE;
  affect.duration = -1;
  SET_BIT_AR(affect.bitvector, AFF_ENTANGLED);
  affect_to_char(&fixture.target, &affect);
  GET_MOVE(&fixture.target) = 101;
  memset(&event, 0, sizeof(event));
  event.pStruct = &fixture.target;
  CuAssertIntEquals(tc, 0, (int)event_rol_yggdrasil_release(&event));
  CuAssertTrue(tc, !affected_by_spell(&fixture.target, SPELL_ENTANGLE));
  CuAssertIntEquals(tc, 50, GET_MOVE(&fixture.target));

  CuAssertStrEquals(tc, "$n dissipates into a cloud of oily green smoke.",
                    rol_conversion_death_message(2000202));
  CuAssertStrEquals(tc, "The werefox yelps, and vanishes into thin air...",
                    rol_conversion_death_message(2000326));
  CuAssertTrue(tc, rol_conversion_death_suppresses_corpse(2000326));
  CuAssertStrEquals(tc, "A fire mephit blinks out of existence.",
                    rol_conversion_death_message(2000907));
  CuAssertStrEquals(
      tc, "With a splash, the water elemental crashes to the ground leaving only a puddle behind.",
      rol_conversion_death_message(2003053));
  CuAssertTrue(tc, rol_conversion_death_message(9999999) == NULL);
  CuAssertStrEquals(tc, "As $n dies, $e disintegrates in a flash of bright light!",
                    rol_conversion_death_message(2088815));
  CuAssertTrue(tc, !rol_conversion_death_suppresses_corpse(2088815));
  CuAssertTrue(tc, rol_conversion_death_suppresses_corpse(2053268));
  CuAssertStrEquals(tc, "The small elemental vanishes in a swirl of color.",
                    rol_conversion_death_message(2053264));
  CuAssertTrue(tc, rol_conversion_death_suppresses_corpse(2053264));
  CuAssertIntEquals(tc, 2053269, rol_conversion_death_replacement_vnum(2053268));
  CuAssertIntEquals(tc, 2053254, rol_conversion_death_object_vnum(2053270));
  CuAssertIntEquals(tc, 2001438, rol_conversion_death_object_vnum(2001433));
  CuAssertStrEquals(tc, "$n falls to the ground and dissolves into nothing.",
                    rol_conversion_death_message(2004480));
  CuAssertTrue(tc, rol_conversion_death_suppresses_corpse(2004480));
  CuAssertIntEquals(tc, 2012000, rol_conversion_death_object_vnum(2012005));
  CuAssertTrue(tc, !rol_conversion_death_suppresses_corpse(2012005));
  CuAssertStrEquals(tc, "As $n crumples to ashes, a \trruby gem\tn appears.",
                    rol_conversion_death_message(2094501));
  CuAssertStrEquals(tc, "As $n crumples to ashes, a \tcdiamond\tn appears.",
                    rol_conversion_death_message(2094502));
  CuAssertStrEquals(tc, "As $n crumples to ashes, an \tBaquamarine stone\tn appears.",
                    rol_conversion_death_message(2094503));
  CuAssertStrEquals(tc, "As $n crumples to ashes, a \tYgolden nugget\tn appears.",
                    rol_conversion_death_message(2094504));
  CuAssertIntEquals(tc, 2094508, rol_conversion_death_object_vnum(2094501));
  CuAssertIntEquals(tc, 2094509, rol_conversion_death_object_vnum(2094502));
  CuAssertIntEquals(tc, 2094510, rol_conversion_death_object_vnum(2094503));
  CuAssertIntEquals(tc, 2094511, rol_conversion_death_object_vnum(2094504));
  CuAssertTrue(tc, !rol_conversion_death_suppresses_corpse(2094501));
  CuAssertTrue(tc, !rol_conversion_death_suppresses_corpse(2094502));
  CuAssertTrue(tc, !rol_conversion_death_suppresses_corpse(2094503));
  CuAssertTrue(tc, !rol_conversion_death_suppresses_corpse(2094504));
  CuAssertPtrNotNull(tc, rol_conversion_death_message(2012006));
  CuAssertPtrNotNull(tc, rol_conversion_death_message(2012024));
  CuAssertTrue(tc, rol_conversion_death_retargets_clerics(2053268));
  CuAssertTrue(tc, rol_conversion_death_retargets_clerics(2053269));
  CuAssertTrue(tc, rol_conversion_death_retargets_clerics(2097003));
  CuAssertTrue(tc, !rol_conversion_death_retargets_clerics(2088815));
  CuAssertTrue(tc, rol_conversion_death_suppresses_corpse(2092613));
  CuAssertTrue(tc, !rol_conversion_death_suppresses_corpse(9999999));

  fixture.mobile_indexes[0].vnum = 2000907;
  GET_MOB_RNUM(&fixture.actor) = 0;
  CuAssertTrue(tc, rol_handle_conjured_death(&fixture.actor));

  spec_mechanics_end(&fixture);
}

void Test_spec_rol_abyss_forged_weapons_dissolve_before_corpse_creation(CuTest *tc)
{
  struct spec_mechanics_fixture fixture;
  struct obj_data *primary;
  struct obj_data *offhand;
  struct obj_data *twohand;
  struct obj_data *saved_object_list;

  spec_mechanics_begin(&fixture);
  saved_object_list = object_list;
  object_list = NULL;

  primary = create_obj();
  offhand = create_obj();
  twohand = create_obj();
  primary->name = strdup("abyss forged primary weapon");
  primary->short_description = strdup("an abyss-forged primary weapon");
  offhand->name = strdup("abyss forged offhand weapon");
  offhand->short_description = strdup("an abyss-forged offhand weapon");
  twohand->name = strdup("abyss forged two handed weapon");
  twohand->short_description = strdup("an abyss-forged two-handed weapon");

  GET_EQ(&fixture.actor, WEAR_WIELD_1) = primary;
  primary->worn_by = &fixture.actor;
  primary->worn_on = WEAR_WIELD_1;
  GET_EQ(&fixture.actor, WEAR_WIELD_OFFHAND) = offhand;
  offhand->worn_by = &fixture.actor;
  offhand->worn_on = WEAR_WIELD_OFFHAND;
  GET_EQ(&fixture.actor, WEAR_WIELD_2H) = twohand;
  twohand->worn_by = &fixture.actor;
  twohand->worn_on = WEAR_WIELD_2H;

  CuAssertIntEquals(tc, 0, rol_dissolve_abyss_forged_weapons(&fixture.actor));
  CuAssertPtrEquals(tc, primary, GET_EQ(&fixture.actor, WEAR_WIELD_1));
  SET_BIT_AR(MOB_FLAGS(&fixture.actor), MOB_ROL_ABYSS_FORGED);
  CuAssertIntEquals(tc, 3, rol_dissolve_abyss_forged_weapons(&fixture.actor));
  CuAssertPtrEquals(tc, NULL, GET_EQ(&fixture.actor, WEAR_WIELD_1));
  CuAssertPtrEquals(tc, NULL, GET_EQ(&fixture.actor, WEAR_WIELD_OFFHAND));
  CuAssertPtrEquals(tc, NULL, GET_EQ(&fixture.actor, WEAR_WIELD_2H));
  CuAssertPtrEquals(tc, NULL, object_list);
  CuAssertIntEquals(tc, 0, rol_dissolve_abyss_forged_weapons(&fixture.actor));

  object_list = saved_object_list;
  spec_mechanics_end(&fixture);
}

void Test_spec_rol_darkhold_elemental_deaths_drop_mapped_objects_and_keep_corpses(CuTest *tc)
{
  static const int mobile_vnums[] = {2094501, 2094502, 2094503, 2094504};
  static const int object_vnums[] = {2094508, 2094509, 2094510, 2094511};
  struct spec_mechanics_fixture fixture;
  struct obj_data prototype;
  struct obj_data *dropped;
  struct obj_data *saved_obj_proto;
  struct obj_data *saved_object_list;
  size_t index;

  spec_mechanics_begin(&fixture);
  saved_obj_proto = obj_proto;
  saved_object_list = object_list;
  object_list = NULL;
  clear_object(&prototype);
  GET_OBJ_RNUM(&prototype) = 0;
  prototype.name = "darkhold elemental reward";
  prototype.short_description = "a Darkhold elemental reward";
  prototype.description = "A Darkhold elemental reward lies here.";
  obj_proto = &prototype;
  top_of_objt = 0;
  GET_MOB_RNUM(&fixture.actor) = 0;

  for (index = 0; index < sizeof(mobile_vnums) / sizeof(mobile_vnums[0]); index++)
  {
    fixture.mobile_indexes[0].vnum = mobile_vnums[index];
    fixture.object_indexes[0].vnum = object_vnums[index];
    fixture.object_indexes[0].number = 0;
    CuAssertTrue(tc, !rol_handle_conjured_death(&fixture.actor));
    dropped = fixture.rooms[0].contents;
    CuAssertPtrNotNull(tc, dropped);
    CuAssertIntEquals(tc, object_vnums[index], GET_OBJ_VNUM(dropped));
    CuAssertPtrEquals(tc, dropped, object_list);
    extract_obj(dropped);
    CuAssertPtrEquals(tc, NULL, fixture.rooms[0].contents);
    CuAssertPtrEquals(tc, NULL, object_list);
  }

  obj_proto = saved_obj_proto;
  object_list = saved_object_list;
  spec_mechanics_end(&fixture);
}

void Test_spec_rol_waterdeep_ambient_profiles_preserve_source_rolls_and_sequences(CuTest *tc)
{
  bool speech = false;

  CuAssertIntEquals(tc, 5, rol_waterdeep_ambient_roll_sides(2004830));
  CuAssertIntEquals(tc, 7, rol_waterdeep_ambient_roll_sides(2003204));
  CuAssertIntEquals(tc, 6, rol_waterdeep_ambient_roll_sides(2003205));
  CuAssertIntEquals(tc, 0, rol_waterdeep_ambient_roll_sides(9999999));

  CuAssertTrue(tc, rol_waterdeep_ambient_room_allows(2004830, 6100));
  CuAssertTrue(tc, rol_waterdeep_ambient_room_allows(2005310, 2005400));
  CuAssertTrue(tc, !rol_waterdeep_ambient_room_allows(2005310, 2005401));
  CuAssertTrue(tc, !rol_waterdeep_ambient_room_allows(9999999, 2005400));
  CuAssertTrue(tc, rol_waterdeep_ambient_fighting_allows(2003234, true));
  CuAssertTrue(tc, rol_waterdeep_ambient_fighting_allows(2003059, false));
  CuAssertTrue(tc, !rol_waterdeep_ambient_fighting_allows(2003059, true));
  CuAssertTrue(tc, !rol_waterdeep_ambient_fighting_allows(9999999, false));

  CuAssertStrEquals(tc, "I'll raise 20.", rol_waterdeep_ambient_message(2003205, 2, 0, &speech));
  CuAssertTrue(tc, speech);
  CuAssertStrEquals(tc, "$n studies his cards carefully.",
                    rol_waterdeep_ambient_message(2003205, 2, 1, &speech));
  CuAssertTrue(tc, !speech);
  CuAssertTrue(tc, rol_waterdeep_ambient_message(2003205, 2, 2, &speech) == NULL);
  CuAssertStrEquals(tc, "Know of any good places to gamble around here?",
                    rol_waterdeep_ambient_message(2003236, 2, 1, &speech));
  CuAssertTrue(tc, speech);
  CuAssertStrEquals(tc, "$n looks at you and says, 'You could stand to loose a few pounds.'",
                    rol_waterdeep_ambient_message(2003234, 4, 0, &speech));
  CuAssertTrue(tc, !speech);
  CuAssertStrEquals(tc, "$n winks at you in amusement.",
                    rol_waterdeep_ambient_message(2003234, 4, 1, &speech));
  CuAssertTrue(tc, rol_waterdeep_ambient_message(9999999, 2, 0, &speech) == NULL);
}

void Test_spec_rol_source_periodic_profiles_preserve_generated_source_tables(CuTest *tc)
{
  bool hide;
  bool requires_awake;
  bool speech;
  bool suppresses_fighting;
  int roll_max;
  int roll_min;

  CuAssertIntEquals(tc, 128, (int)rol_source_periodic_profile_count());
  CuAssertIntEquals(tc, 1, rol_source_periodic_devour_order(2007140));
  CuAssertIntEquals(tc, 2, rol_source_periodic_devour_order(2003062));
  CuAssertIntEquals(tc, 0, rol_source_periodic_devour_order(2097006));
  CuAssertIntEquals(tc, 0, rol_source_periodic_devour_order(9999999));
  CuAssertTrue(tc, rol_source_periodic_profile_bounds(2007220, &roll_min, &roll_max,
                                                      &requires_awake, &suppresses_fighting));
  CuAssertIntEquals(tc, 0, roll_min);
  CuAssertIntEquals(tc, 100, roll_max);
  CuAssertTrue(tc, requires_awake);
  CuAssertTrue(tc, suppresses_fighting);
  CuAssertTrue(tc, rol_source_periodic_profile_bounds(2088806, &roll_min, &roll_max,
                                                      &requires_awake, &suppresses_fighting));
  CuAssertIntEquals(tc, 0, roll_min);
  CuAssertIntEquals(tc, 50, roll_max);
  CuAssertTrue(tc, requires_awake);
  CuAssertTrue(tc, !suppresses_fighting);
  CuAssertTrue(tc, rol_source_periodic_profile_bounds(2088805, &roll_min, &roll_max,
                                                      &requires_awake, &suppresses_fighting));
  CuAssertIntEquals(tc, 1, roll_min);
  CuAssertIntEquals(tc, 80, roll_max);
  CuAssertTrue(tc, requires_awake);
  CuAssertTrue(tc, !suppresses_fighting);
  CuAssertTrue(tc, rol_source_periodic_profile_bounds(2001230, NULL, NULL, &requires_awake, NULL));
  CuAssertTrue(tc, !requires_awake);
  CuAssertTrue(tc, !rol_source_periodic_profile_bounds(9999999, NULL, NULL, NULL, NULL));
  CuAssertTrue(tc, rol_source_periodic_dice_shape(2003212, &roll_min, &roll_max));
  CuAssertIntEquals(tc, 2, roll_min);
  CuAssertIntEquals(tc, 4, roll_max);
  CuAssertTrue(tc, rol_source_periodic_requires_sleeping(2003212));
  CuAssertTrue(tc, !rol_source_periodic_requires_sleeping(2012000));
  CuAssertTrue(tc, !rol_source_periodic_dice_shape(2007220, NULL, NULL));

  CuAssertIntEquals(tc, 2, (int)rol_source_periodic_outcome_action_count(2007220, 2));
  CuAssertStrEquals(tc, "$n looks around the room as if $e was bored.",
                    rol_source_periodic_outcome_action(2007220, 2, 0, &speech, &hide));
  CuAssertTrue(tc, !speech);
  CuAssertTrue(tc, hide);
  CuAssertStrEquals(tc, "$n smiles happily.",
                    rol_source_periodic_outcome_action(2007220, 2, 1, &speech, &hide));
  CuAssertTrue(tc, !speech);
  CuAssertTrue(tc, !hide);
  CuAssertTrue(tc, rol_source_periodic_outcome_action(2007220, 2, 2, NULL, NULL) == NULL);
}

void Test_spec_rol_scornubel_profiles_preserve_source_behavior(CuTest *tc)
{
  const char *description;
  bool critical_only;
  bool hide;
  bool requires_awake;
  bool speech;
  bool suppresses_fighting;
  int denominator;
  int roll_max;
  int roll_min;

  CuAssertTrue(tc, rol_source_periodic_profile_bounds(2006002, &roll_min, &roll_max,
                                                      &requires_awake, &suppresses_fighting));
  CuAssertIntEquals(tc, 0, roll_min);
  CuAssertIntEquals(tc, 20, roll_max);
  CuAssertTrue(tc, requires_awake);
  CuAssertTrue(tc, suppresses_fighting);
  CuAssertTrue(tc, rol_source_periodic_profile_bounds(2006061, &roll_min, &roll_max,
                                                      &requires_awake, &suppresses_fighting));
  CuAssertIntEquals(tc, 0, roll_min);
  CuAssertIntEquals(tc, 50, roll_max);
  CuAssertIntEquals(tc, 2, (int)rol_source_periodic_outcome_action_count(2006111, 0));
  CuAssertStrEquals(tc, "$n says: 'You may plead your case now.'",
                    rol_source_periodic_outcome_action(2006111, 0, 0, &speech, &hide));
  CuAssertTrue(tc, !speech);
  CuAssertTrue(tc, hide);
  CuAssertStrEquals(tc, "$n says: 'Can we get on with this?'",
                    rol_source_periodic_outcome_action(2006111, 0, 1, &speech, &hide));
  CuAssertTrue(tc, !speech);
  CuAssertTrue(tc, hide);

  CuAssertTrue(tc, rol_weapon_profile(2006084, &denominator, &critical_only, &description));
  CuAssertTrue(tc, !critical_only);
  CuAssertIntEquals(tc, 36, denominator);
  CuAssertStrEquals(tc, "One-in-36 fixed 100-point source-untyped fiery burst.", description);
  CuAssertTrue(tc, rol_scornubel_fiery_mace_roll_fires(0));
  CuAssertTrue(tc, !rol_scornubel_fiery_mace_roll_fires(1));
  CuAssertTrue(tc, !rol_scornubel_fiery_mace_roll_fires(35));
  CuAssertIntEquals(tc, 100, rol_scornubel_fiery_mace_damage());
}

void Test_spec_rol_zhentil_periodic_profiles_preserve_source_behavior(CuTest *tc)
{
  bool hide;
  bool requires_awake;
  bool speech;
  bool suppresses_fighting;
  int roll_max;
  int roll_min;

  CuAssertTrue(tc, rol_source_periodic_profile_bounds(2081054, &roll_min, &roll_max,
                                                      &requires_awake, &suppresses_fighting));
  CuAssertIntEquals(tc, 0, roll_min);
  CuAssertIntEquals(tc, 10, roll_max);
  CuAssertTrue(tc, requires_awake);
  CuAssertTrue(tc, suppresses_fighting);
  CuAssertIntEquals(tc, 1, (int)rol_source_periodic_outcome_action_count(2081054, 0));
  CuAssertStrEquals(tc, "$n wiggles $s bottom.",
                    rol_source_periodic_outcome_action(2081054, 0, 0, &speech, &hide));
  CuAssertTrue(tc, !speech);
  CuAssertTrue(tc, !hide);

  CuAssertTrue(tc, rol_source_periodic_profile_bounds(2081021, &roll_min, &roll_max, NULL, NULL));
  CuAssertIntEquals(tc, 0, roll_min);
  CuAssertIntEquals(tc, 20, roll_max);
  CuAssertStrEquals(tc, "$n sighs loudly.",
                    rol_source_periodic_outcome_action(2081021, 4, 0, &speech, &hide));
  CuAssertIntEquals(tc, 2, (int)rol_source_periodic_outcome_action_count(2081067, 0));
  CuAssertStrEquals(tc, "$n pauses and scribbles some figures in a notebook.",
                    rol_source_periodic_outcome_action(2081067, 0, 0, &speech, &hide));
  CuAssertTrue(tc, hide);
  CuAssertStrEquals(tc, "$n scratches at an itch.",
                    rol_source_periodic_outcome_action(2081067, 0, 1, &speech, &hide));
  CuAssertTrue(tc, !hide);
}

void Test_spec_rol_lavatubes_profiles_preserve_source_outcomes(CuTest *tc)
{
  CuAssertIntEquals(tc, 100, rol_lavatubes_skeleton_key_break_chance(-10));
  CuAssertIntEquals(tc, 50, rol_lavatubes_skeleton_key_break_chance(0));
  CuAssertIntEquals(tc, 30, rol_lavatubes_skeleton_key_break_chance(4));
  CuAssertIntEquals(tc, 0, rol_lavatubes_skeleton_key_break_chance(10));
  CuAssertIntEquals(tc, 0, rol_lavatubes_skeleton_key_break_chance(30));

  CuAssertIntEquals(tc, ROL_LAVATUBES_SNOWVULTURE_NONE, rol_lavatubes_snowvulture_outcome(2));
  CuAssertIntEquals(tc, ROL_LAVATUBES_SNOWVULTURE_SQUEAK, rol_lavatubes_snowvulture_outcome(3));
  CuAssertIntEquals(tc, ROL_LAVATUBES_SNOWVULTURE_FLAP, rol_lavatubes_snowvulture_outcome(4));
  CuAssertIntEquals(tc, ROL_LAVATUBES_SNOWVULTURE_DEVOUR, rol_lavatubes_snowvulture_outcome(5));
  CuAssertIntEquals(tc, ROL_LAVATUBES_SNOWVULTURE_NONE, rol_lavatubes_snowvulture_outcome(6));
}

void Test_spec_rol_utility_object_profiles_preserve_source_boundaries(CuTest *tc)
{
  static const int called_vnums[] = {2000047, 2010672, 2019988, 2026260, 2043723,
                                     2044019, 2051110, 2051207, 2057236};
  const char *description;
  const char *phrase;
  int cooldown_hours;
  size_t index;

  CuAssertTrue(tc, rol_utility_sacrifice_command_name("get"));
  CuAssertTrue(tc, rol_utility_sacrifice_command_name("take"));
  CuAssertTrue(tc, rol_utility_sacrifice_command_name("drag"));
  CuAssertTrue(tc, !rol_utility_sacrifice_command_name("drop"));
  CuAssertTrue(tc, !rol_utility_sacrifice_command_name(NULL));

  CuAssertTrue(tc, rol_utility_sacrifice_keyword("child"));
  CuAssertTrue(tc, rol_utility_sacrifice_keyword("  ALL remaining"));
  CuAssertTrue(tc, !rol_utility_sacrifice_keyword("children"));
  CuAssertTrue(tc, !rol_utility_sacrifice_keyword(NULL));

  CuAssertStrEquals(tc, "The necromancer's child screams at the top of his lungs!\r\n",
                    rol_utility_necro_child_message(0));
  CuAssertStrEquals(tc, "The necromancer's child tries to bite you!\r\n",
                    rol_utility_necro_child_message(6));
  CuAssertTrue(tc, rol_utility_necro_child_message(7) == NULL);
  CuAssertTrue(tc, rol_utility_necro_child_message(10) == NULL);

  CuAssertTrue(tc, !rol_utility_monocle_room(2090123));
  CuAssertTrue(tc, rol_utility_monocle_room(2090124));
  CuAssertTrue(tc, rol_utility_monocle_room(2090142));
  CuAssertTrue(tc, !rol_utility_monocle_room(2090143));

  CuAssertIntEquals(tc, (int)(sizeof(called_vnums) / sizeof(called_vnums[0])),
                    (int)rol_utility_called_profile_count());
  for (index = 0; index < sizeof(called_vnums) / sizeof(called_vnums[0]); index++)
  {
    phrase = NULL;
    description = NULL;
    cooldown_hours = -1;
    CuAssertTrue(tc, rol_utility_called_profile(called_vnums[index], &phrase, &cooldown_hours,
                                                &description));
    CuAssertPtrNotNull(tc, phrase);
    CuAssertTrue(tc, cooldown_hours >= 0);
    CuAssertPtrNotNull(tc, description);
  }
  CuAssertTrue(tc, rol_utility_called_profile(2000047, &phrase, &cooldown_hours, &description));
  CuAssertStrEquals(tc, "shirak", phrase);
  CuAssertIntEquals(tc, 0, cooldown_hours);
  CuAssertTrue(tc, rol_utility_called_profile(2057236, &phrase, &cooldown_hours, &description));
  CuAssertStrEquals(tc, "accelerate", phrase);
  CuAssertIntEquals(tc, 48, cooldown_hours);
  CuAssertTrue(tc, rol_utility_called_profile(2019988, &phrase, &cooldown_hours, &description));
  CuAssertStrEquals(tc, "Crescent Moon", phrase);
  CuAssertIntEquals(tc, 0, cooldown_hours);
  CuAssertTrue(tc, !rol_utility_called_profile(9999999, NULL, NULL, NULL));
}

void Test_spec_rol_utility_service_batch_preserves_source_boundaries(CuTest *tc)
{
  struct spec_mechanics_fixture fixture;
  struct obj_data container;

  memset(&container, 0, sizeof(container));
  GET_OBJ_TYPE(&container) = ITEM_CONTAINER;
  CuAssertTrue(tc, rol_utility_loot_blockable_container(&container));
  GET_OBJ_VAL(&container, 3) = 1;
  GET_OBJ_VAL(&container, 4) = 0;
  CuAssertTrue(tc, rol_utility_loot_blockable_container(&container));
  GET_OBJ_VAL(&container, 4) = 42;
  CuAssertTrue(tc, !rol_utility_loot_blockable_container(&container));
  CuAssertTrue(tc, !rol_utility_loot_blockable_container(NULL));
  CuAssertIntEquals(tc, 120, rol_utility_loot_sweep_interval_seconds());

  CuAssertIntEquals(tc, 2023000, rol_utility_newbie_east_destination_vnum(RACE_HUMAN));
  CuAssertIntEquals(tc, 2023399, rol_utility_newbie_east_destination_vnum(RACE_DROW));
  CuAssertIntEquals(tc, 0, rol_utility_weight_transition(false, 4999));
  CuAssertIntEquals(tc, 1, rol_utility_weight_transition(false, 5000));
  CuAssertIntEquals(tc, 0, rol_utility_weight_transition(true, 5000));
  CuAssertIntEquals(tc, -1, rol_utility_weight_transition(true, 4999));

  spec_mechanics_begin(&fixture);
  fixture.rooms[0].number = 2003001;
  IN_ROOM(&fixture.worn) = 0;
  REMOVE_BIT_AR(MOB_FLAGS(&fixture.actor), MOB_ISNPC);
  GET_LEVEL(&fixture.actor) = 14;
  CuAssertTrue(tc, !rol_utility_plague_eligible(&fixture.actor, &fixture.worn));
  GET_LEVEL(&fixture.actor) = 15;
  CuAssertTrue(tc, rol_utility_plague_eligible(&fixture.actor, &fixture.worn));
  SET_BIT_AR(AFF_FLAGS(&fixture.actor), AFF_DISEASE);
  CuAssertTrue(tc, !rol_utility_plague_eligible(&fixture.actor, &fixture.worn));
  REMOVE_BIT_AR(AFF_FLAGS(&fixture.actor), AFF_DISEASE);
  IN_ROOM(&fixture.worn) = 1;
  CuAssertTrue(tc, !rol_utility_plague_eligible(&fixture.actor, &fixture.worn));
  spec_mechanics_end(&fixture);
}

void Test_spec_rol_scheduled_mobile_preserves_time_and_state_boundaries(CuTest *tc)
{
  bool active = false;
  bool ship_notice_sent = false;
  bool shop_notice_sent = false;
  int counter = 77;
  int index;

  CuAssertIntEquals(tc, ROL_SCHEDULED_GATE_CLOSE, rol_scheduled_gate_state_for_hour(0));
  CuAssertIntEquals(tc, ROL_SCHEDULED_GATE_CLOSE, rol_scheduled_gate_state_for_hour(5));
  CuAssertIntEquals(tc, ROL_SCHEDULED_GATE_OPEN, rol_scheduled_gate_state_for_hour(6));
  CuAssertIntEquals(tc, ROL_SCHEDULED_GATE_OPEN, rol_scheduled_gate_state_for_hour(18));
  CuAssertIntEquals(tc, ROL_SCHEDULED_GATE_NONE, rol_scheduled_gate_state_for_hour(19));
  CuAssertIntEquals(tc, ROL_SCHEDULED_GATE_NONE, rol_scheduled_gate_state_for_hour(21));
  CuAssertIntEquals(tc, ROL_SCHEDULED_GATE_CLOSE, rol_scheduled_gate_state_for_hour(22));

  CuAssertIntEquals(tc, ROL_SCHEDULED_NAVAL_IDLE, rol_scheduled_naval_branch_for(true, true));
  CuAssertIntEquals(tc, ROL_SCHEDULED_NAVAL_FIGHTING, rol_scheduled_naval_branch_for(false, true));
  CuAssertIntEquals(tc, ROL_SCHEDULED_NAVAL_NONE, rol_scheduled_naval_branch_for(false, false));

  CuAssertIntEquals(tc, ROL_SCHEDULED_CRIER_MOONSHAE_SHIP,
                    rol_scheduled_crier_notice_for_hour(3, &shop_notice_sent, &ship_notice_sent));
  CuAssertIntEquals(tc, ROL_SCHEDULED_CRIER_NONE,
                    rol_scheduled_crier_notice_for_hour(3, &shop_notice_sent, &ship_notice_sent));
  CuAssertIntEquals(tc, ROL_SCHEDULED_CRIER_NONE,
                    rol_scheduled_crier_notice_for_hour(4, &shop_notice_sent, &ship_notice_sent));
  CuAssertIntEquals(tc, ROL_SCHEDULED_CRIER_SHOPS_OPENING,
                    rol_scheduled_crier_notice_for_hour(5, &shop_notice_sent, &ship_notice_sent));
  CuAssertIntEquals(tc, ROL_SCHEDULED_CRIER_NONE,
                    rol_scheduled_crier_notice_for_hour(9, &shop_notice_sent, &ship_notice_sent));
  CuAssertIntEquals(tc, ROL_SCHEDULED_CRIER_CALIMPORT_SHIP,
                    rol_scheduled_crier_notice_for_hour(10, &shop_notice_sent, &ship_notice_sent));
  CuAssertIntEquals(tc, ROL_SCHEDULED_CRIER_NONE,
                    rol_scheduled_crier_notice_for_hour(18, &shop_notice_sent, &ship_notice_sent));
  CuAssertIntEquals(tc, ROL_SCHEDULED_CRIER_NONE,
                    rol_scheduled_crier_notice_for_hour(19, &shop_notice_sent, &ship_notice_sent));
  CuAssertIntEquals(tc, ROL_SCHEDULED_CRIER_SHOPS_CLOSING,
                    rol_scheduled_crier_notice_for_hour(18, &shop_notice_sent, &ship_notice_sent));
  CuAssertIntEquals(tc, ROL_SCHEDULED_CRIER_NONE,
                    rol_scheduled_crier_notice_for_hour(3, NULL, &ship_notice_sent));

  index = rol_scheduled_lighthouse_step(8, true, &active, &counter);
  CuAssertIntEquals(tc, 0, index);
  CuAssertTrue(tc, active);
  CuAssertIntEquals(tc, 1, counter);
  index = rol_scheduled_lighthouse_step(9, true, &active, &counter);
  CuAssertIntEquals(tc, 1, index);
  CuAssertIntEquals(tc, 2, counter);
  counter = 25;
  index = rol_scheduled_lighthouse_step(9, true, &active, &counter);
  CuAssertIntEquals(tc, 25, index);
  CuAssertIntEquals(tc, 26, counter);
  counter = 30;
  index = rol_scheduled_lighthouse_step(9, true, &active, &counter);
  CuAssertIntEquals(tc, -1, index);
  CuAssertTrue(tc, !active);
  CuAssertIntEquals(tc, 1, counter);
  CuAssertIntEquals(tc, -1, rol_scheduled_lighthouse_step(8, true, NULL, &counter));
}

void Test_spec_rol_utility_magius_staff_toggles_light(CuTest *tc)
{
  struct spec_mechanics_fixture fixture;
  struct spec_event_context context;
  struct command_info commands[3];
  struct command_info *saved_complete_cmd_info;

  spec_mechanics_begin(&fixture);
  memset(&context, 0, sizeof(context));
  memset(commands, 0, sizeof(commands));
  saved_complete_cmd_info = complete_cmd_info;
  complete_cmd_info = commands;
  commands[1].command = "say";
  commands[2].command = "\n";
  fixture.object_indexes[0].vnum = 2000047;

  context.owner_type = SPEC_OWNER_OBJECT;
  context.event = SPEC_EVENT_COMMAND;
  context.owner = &fixture.worn;
  context.actor = &fixture.actor;
  context.command = 1;
  context.argument = "shirak";
  CuAssertIntEquals(tc, TRUE, rol_utility_object_typed(&context));
  CuAssertTrue(tc, OBJ_FLAGGED(&fixture.worn, ITEM_GLOW));

  context.argument = "dulak";
  CuAssertIntEquals(tc, TRUE, rol_utility_object_typed(&context));
  CuAssertTrue(tc, !OBJ_FLAGGED(&fixture.worn, ITEM_GLOW));

  complete_cmd_info = saved_complete_cmd_info;
  spec_mechanics_end(&fixture);
}

void Test_spec_rol_lavatubes_automaton_preserves_exit_pair_cycle(CuTest *tc)
{
  struct spec_mechanics_fixture fixture;
  struct command_info commands[3];
  struct command_info *saved_complete_cmd_info;
  struct room_direction_data upper_down;
  struct room_direction_data lower_up;

  spec_mechanics_begin(&fixture);
  memset(commands, 0, sizeof(commands));
  memset(&upper_down, 0, sizeof(upper_down));
  memset(&lower_up, 0, sizeof(lower_up));

  saved_complete_cmd_info = complete_cmd_info;
  commands[1].command = "pull";
  commands[2].command = "down";
  commands[2].command_pointer = do_move;
  commands[2].subcmd = DOWN;
  complete_cmd_info = commands;

  fixture.rooms[0].number = 2012158;
  fixture.rooms[1].number = 2012159;
  upper_down.to_room = 1;
  lower_up.to_room = 0;
  upper_down.exit_info = EX_ISDOOR | EX_BLOCKED;
  lower_up.exit_info = EX_ISDOOR | EX_BLOCKED;
  fixture.rooms[0].dir_option[DOWN] = &upper_down;
  fixture.rooms[1].dir_option[UP] = &lower_up;

  fixture.mobile_indexes[0].vnum = 2012027;
  GET_MOB_RNUM(&fixture.actor) = 0;
  fixture.rooms[0].people = NULL;
  IN_ROOM(&fixture.target) = NOWHERE;
  fixture.actor.next_in_room = NULL;
  IN_ROOM(&fixture.actor) = 1;
  fixture.rooms[1].people = &fixture.actor;

  fixture.object_indexes[0].vnum = 2012027;
  fixture.object_indexes[0].func = rol_lavatubes_object;
  fixture.worn.name = "lever";
  fixture.worn.in_room = 1;
  GET_OBJ_TYPE(&fixture.worn) = ITEM_SWITCH;
  GET_OBJ_VAL(&fixture.worn, 1) = 2012158;
  fixture.rooms[1].contents = &fixture.worn;

  CuAssertIntEquals(tc, TRUE,
                    spec_gateway_command_object(&fixture.actor, &fixture.worn, 1, "lever"));
  CuAssertTrue(tc, !EXIT_FLAGGED(&upper_down, EX_BLOCKED));
  CuAssertTrue(tc, !EXIT_FLAGGED(&lower_up, EX_BLOCKED));

  SET_BIT(upper_down.exit_info, EX_BLOCKED);
  SET_BIT(lower_up.exit_info, EX_BLOCKED);
  CuAssertIntEquals(tc, TRUE, spec_gateway_mobile_activity(&fixture.actor, rol_lavatubes_mobile));
  CuAssertTrue(tc, !EXIT_FLAGGED(&upper_down, EX_BLOCKED));
  CuAssertTrue(tc, !EXIT_FLAGGED(&lower_up, EX_BLOCKED));

  fixture.rooms[1].people = NULL;
  IN_ROOM(&fixture.actor) = 0;
  fixture.rooms[0].people = &fixture.actor;
  fixture.rooms[0].func = rol_lavatubes_room;
  CuAssertIntEquals(tc, TRUE, spec_gateway_command_room(&fixture.actor, &fixture.rooms[0], 2, ""));
  CuAssertIntEquals(tc, 1, IN_ROOM(&fixture.actor));
  CuAssertTrue(tc, EXIT_FLAGGED(&upper_down, EX_CLOSED));
  CuAssertTrue(tc, EXIT_FLAGGED(&lower_up, EX_CLOSED));
  CuAssertTrue(tc, EXIT_FLAGGED(&upper_down, EX_BLOCKED));
  CuAssertTrue(tc, EXIT_FLAGGED(&lower_up, EX_BLOCKED));

  fixture.rooms[1].contents = NULL;
  fixture.worn.in_room = NOWHERE;
  complete_cmd_info = saved_complete_cmd_info;
  spec_mechanics_end(&fixture);
}

void Test_spec_rol_state_periodic_profiles_preserve_idle_and_fighting_tables(CuTest *tc)
{
  bool hide;
  bool speech;
  int dice_count;
  int dice_sides;

  CuAssertIntEquals(tc, 34, (int)rol_state_periodic_profile_count());

  CuAssertTrue(tc, rol_state_periodic_dice(2003206, false, &dice_count, &dice_sides));
  CuAssertIntEquals(tc, 2, dice_count);
  CuAssertIntEquals(tc, 5, dice_sides);
  CuAssertTrue(tc, rol_state_periodic_dice(2003206, true, &dice_count, &dice_sides));
  CuAssertIntEquals(tc, 2, dice_count);
  CuAssertIntEquals(tc, 5, dice_sides);
  CuAssertTrue(tc, rol_state_periodic_runs_idle_while_fighting(2003206));
  CuAssertTrue(tc, !rol_state_periodic_runs_idle_while_fighting(2003039));

  CuAssertTrue(tc, rol_state_periodic_dice(2003039, false, &dice_count, &dice_sides));
  CuAssertIntEquals(tc, 2, dice_count);
  CuAssertIntEquals(tc, 5, dice_sides);
  CuAssertTrue(tc, rol_state_periodic_dice(2003039, true, &dice_count, &dice_sides));
  CuAssertIntEquals(tc, 1, dice_count);
  CuAssertIntEquals(tc, 4, dice_sides);

  CuAssertTrue(tc, rol_state_periodic_dice(2003020, false, &dice_count, &dice_sides));
  CuAssertTrue(tc, !rol_state_periodic_dice(2003020, true, &dice_count, &dice_sides));
  CuAssertTrue(tc, rol_state_periodic_dice(2005519, false, &dice_count, &dice_sides));
  CuAssertIntEquals(tc, 2, dice_count);
  CuAssertIntEquals(tc, 7, dice_sides);
  CuAssertTrue(tc, rol_state_periodic_dice(2005519, true, &dice_count, &dice_sides));
  CuAssertIntEquals(tc, 1, dice_count);
  CuAssertIntEquals(tc, 4, dice_sides);
  CuAssertTrue(tc, rol_state_periodic_dice(2005505, false, &dice_count, &dice_sides));
  CuAssertIntEquals(tc, 2, dice_count);
  CuAssertIntEquals(tc, 5, dice_sides);
  CuAssertTrue(tc, rol_state_periodic_dice(2005505, true, &dice_count, &dice_sides));
  CuAssertIntEquals(tc, 1, dice_count);
  CuAssertIntEquals(tc, 4, dice_sides);
  CuAssertTrue(tc, !rol_state_periodic_dice(9999999, false, NULL, NULL));

  CuAssertIntEquals(tc, 2, (int)rol_state_periodic_outcome_action_count(2003039, false, 4));
  CuAssertStrEquals(tc, "Have you seen a small boy, around eight years old, with red hair?",
                    rol_state_periodic_outcome_action(2003039, false, 4, 0, &speech, &hide));
  CuAssertTrue(tc, speech);
  CuAssertTrue(tc, !hide);
  CuAssertStrEquals(tc, "I can't find him anywhere!",
                    rol_state_periodic_outcome_action(2003039, false, 4, 1, &speech, &hide));
  CuAssertTrue(tc, speech);
  CuAssertTrue(tc, !hide);
  CuAssertStrEquals(tc, "$n screams, 'Someone help me!'",
                    rol_state_periodic_outcome_action(2003039, true, 1, 0, &speech, &hide));
  CuAssertTrue(tc, !speech);
  CuAssertTrue(tc, hide);
  CuAssertStrEquals(tc, "Wanna fight, pig? I could use some exercise..",
                    rol_state_periodic_outcome_action(2005505, false, 3, 0, &speech, &hide));
  CuAssertTrue(tc, speech);
  CuAssertStrEquals(tc, "Die, worm!! I'm hungry, and you'll make a PERFECT lunchtime snack!",
                    rol_state_periodic_outcome_action(2005505, true, 2, 0, &speech, &hide));
  CuAssertTrue(tc, speech);
  CuAssertTrue(tc, rol_state_periodic_outcome_action(2003039, false, 4, 2, NULL, NULL) == NULL);
  CuAssertTrue(tc, rol_state_periodic_outcome_action(9999999, false, 4, 0, NULL, NULL) == NULL);
}

void Test_spec_rol_waterdeep_bouncer_routes_preserve_source_paths(CuTest *tc)
{
  CuAssertIntEquals(tc, 2005532, rol_waterdeep_bouncer_home_vnum(2005523));
  CuAssertIntEquals(tc, 4, (int)rol_waterdeep_bouncer_route_length(2005523));
  CuAssertIntEquals(tc, 2005531, rol_waterdeep_bouncer_home_vnum(2005541));
  CuAssertIntEquals(tc, 3, (int)rol_waterdeep_bouncer_route_length(2005541));
  CuAssertIntEquals(tc, 2005530, rol_waterdeep_bouncer_home_vnum(2005542));
  CuAssertIntEquals(tc, 2, (int)rol_waterdeep_bouncer_route_length(2005542));
  CuAssertIntEquals(tc, 2005533, rol_waterdeep_bouncer_home_vnum(2005543));
  CuAssertIntEquals(tc, 4, (int)rol_waterdeep_bouncer_route_length(2005543));
  CuAssertIntEquals(tc, 0, rol_waterdeep_bouncer_home_vnum(9999999));
  CuAssertIntEquals(tc, 0, (int)rol_waterdeep_bouncer_route_length(9999999));
}

void Test_spec_rol_weapon_profiles_cover_converted_bindings(CuTest *tc)
{
  static const int vnums[] = {
      2004505, 2013307, 2014837, 2019886, 2019900, 2019912, 2020075, 2026014, 2034840, 2038025,
      2038095, 2040135, 2080547, 2089462, 2091305, 2095776, 2095851, 2095876, 2095878, 2098330,
      2019933, 2025030, 2009054, 2025018, 2001010, 2080034, 2080038, 2026233, 2026248, 2015116,
      2013308, 2097117, 2001005, 2014023, 2024405, 2053266, 2053263, 2053259, 2053289, 2053290,
      2053291, 2053292, 2053243, 2083238, 2083235, 2053250, 2053271, 2043741, 2008000, 2001057,
      2004797, 2093227, 2093228, 2032602, 2033001, 2033012, 2006084,
  };
  const char *description;
  bool critical_only;
  int denominator;
  size_t index;

  CuAssertIntEquals(tc, (int)(sizeof(vnums) / sizeof(vnums[0])), (int)rol_weapon_profile_count());
  for (index = 0; index < sizeof(vnums) / sizeof(vnums[0]); index++)
  {
    denominator = 0;
    critical_only = false;
    description = NULL;
    CuAssertTrue(tc, rol_weapon_profile(vnums[index], &denominator, &critical_only, &description));
    CuAssertTrue(tc, denominator > 0);
    CuAssertPtrNotNull(tc, description);
  }

  CuAssertTrue(tc, rol_weapon_profile(2040135, &denominator, &critical_only, &description));
  CuAssertTrue(tc, critical_only);
  CuAssertIntEquals(tc, 1, denominator);
  CuAssertTrue(tc, rol_weapon_profile(2025030, &denominator, &critical_only, &description));
  CuAssertTrue(tc, !critical_only);
  CuAssertIntEquals(tc, 22, denominator);
  CuAssertStrEquals(tc, "Flaming burst that heals Fire Elementals and Efreeti.", description);
  CuAssertTrue(tc, rol_weapon_profile(2053292, &denominator, &critical_only, &description));
  CuAssertIntEquals(tc, 22, denominator);
  CuAssertStrEquals(tc, "NPC arcane-caster damage and casting interruption.", description);
  CuAssertTrue(tc, !rol_weapon_profile(9999999, NULL, NULL, NULL));
}

void Test_spec_rol_monster_combat_profiles_cover_converted_bindings(CuTest *tc)
{
  static const int vnums[] = {
      150772,  196007,  196013,  196027,  196040,  196076,  2000207, 2000212, 2000214, 2000215,
      2000220, 2000221, 2000233, 2000325, 2000326, 2000327, 2000328, 2000525, 2001228, 2001229,
      2001407, 2001436, 2001437, 2004070, 2004480, 2004530, 2005023, 2005718, 2012005, 2012006,
      2012024, 2012025, 2012026, 2014015, 2014026, 2014029, 2014601, 2014605, 2015113, 2015125,
      2019701, 2019750, 2020247, 2020378, 2021786, 2021820, 2026208, 2026216, 2026225, 2026236,
      2026238, 2026241, 2026242, 2026243, 2026244, 2026245, 2032623, 2032629, 2032632, 2032640,
      2032641, 2032642, 2032643, 2032644, 2032645, 2032646, 2032654, 2032659, 2032660, 2033000,
      2033001, 2033003, 2033004, 2033005, 2033008, 2033009, 2033011, 2033014, 2033015, 2033016,
      2033020, 2033021, 2033022, 2033026, 2033027, 2034833, 2041900, 2043358, 2043702, 2043703,
      2043705, 2043728, 2043741, 2043742, 2043744, 2043745, 2043746, 2043756, 2043758, 2043759,
      2043761, 2043767, 2043768, 2043769, 2043770, 2043778, 2043780, 2045116, 2045146, 2045182,
      2051246, 2051333, 2051334, 2053264, 2053265, 2053266, 2059815, 2059835, 2062401, 2062402,
      2062405, 2062406, 2062701, 2062702, 2062703, 2062704, 2062705, 2062706, 2062707, 2062708,
      2062710, 2062711, 2062712, 2062713, 2062714, 2062715, 2062716, 2062717, 2062721, 2062722,
      2081706, 2081746, 2081747, 2083224, 2092608, 2093202, 2093204, 2093205, 2093206, 2093209,
      2093210, 2096631, 2096670, 2096672, 2097061,
  };
  const char *description;
  bool faerie_fire;
  bool prismatic;
  bool search;
  int denominator;
  size_t index;

  CuAssertIntEquals(tc, (int)(sizeof(vnums) / sizeof(vnums[0])),
                    (int)rol_monster_combat_profile_count());
  for (index = 0; index < sizeof(vnums) / sizeof(vnums[0]); index++)
  {
    denominator = 0;
    description = NULL;
    CuAssertTrue(tc, rol_monster_combat_profile(vnums[index], &denominator, &description));
    CuAssertTrue(tc, denominator > 0);
    CuAssertPtrNotNull(tc, description);
  }

  CuAssertTrue(tc, rol_monster_combat_profile(2053265, &denominator, &description));
  CuAssertIntEquals(tc, 20, denominator);
  CuAssertStrEquals(tc, "Prismatic burst adapted from a source critical event.", description);
  CuAssertTrue(tc, rol_monster_combat_profile(2045116, &denominator, &description));
  CuAssertIntEquals(tc, 1, denominator);
  CuAssertTrue(tc, rol_monster_combat_profile(2004480, &denominator, &description));
  CuAssertIntEquals(tc, 5, denominator);
  CuAssertTrue(tc, rol_monster_combat_profile(2015125, &denominator, &description));
  CuAssertIntEquals(tc, 20, denominator);
  CuAssertTrue(tc, rol_monster_combat_profile(2019701, &denominator, &description));
  CuAssertIntEquals(tc, 11, denominator);
  CuAssertTrue(tc, rol_monster_combat_profile(2026238, &denominator, &description));
  CuAssertIntEquals(tc, 4, denominator);
  CuAssertTrue(tc, !rol_monster_combat_profile(9999999, NULL, NULL));

  CuAssertTrue(tc, rol_seelie_faerie_profile(2062701, &faerie_fire, &prismatic, &search));
  CuAssertTrue(tc, faerie_fire);
  CuAssertTrue(tc, prismatic);
  CuAssertTrue(tc, search);
  CuAssertTrue(tc, rol_seelie_faerie_profile(2062702, &faerie_fire, &prismatic, &search));
  CuAssertTrue(tc, !faerie_fire);
  CuAssertTrue(tc, !prismatic);
  CuAssertTrue(tc, search);
  CuAssertTrue(tc, rol_seelie_faerie_profile(2062722, &faerie_fire, &prismatic, &search));
  CuAssertTrue(tc, faerie_fire);
  CuAssertTrue(tc, !prismatic);
  CuAssertTrue(tc, !search);
  CuAssertTrue(tc, !rol_seelie_faerie_profile(2062709, NULL, NULL, NULL));
  CuAssertTrue(tc, rol_seelie_faerie_runs_while_disabled(2062701));
  CuAssertTrue(tc, !rol_seelie_faerie_runs_while_disabled(2062709));
  CuAssertIntEquals(tc, 0, rol_seelie_prismatic_beam_count(1));
  CuAssertIntEquals(tc, 1, rol_seelie_prismatic_beam_count(2));
  CuAssertIntEquals(tc, 1, rol_seelie_prismatic_beam_count(3));
  CuAssertIntEquals(tc, 2, rol_seelie_prismatic_beam_count(4));
  CuAssertIntEquals(tc, 2, rol_seelie_prismatic_beam_count(5));
  CuAssertIntEquals(tc, 0, rol_seelie_prismatic_beam_count(6));
  CuAssertIntEquals(tc, 0, rol_seelie_prismatic_damage(-1));
  CuAssertIntEquals(tc, 420, rol_seelie_prismatic_damage(0));
  CuAssertIntEquals(tc, 280, rol_seelie_prismatic_damage(1));
  CuAssertIntEquals(tc, 140, rol_seelie_prismatic_damage(2));
  CuAssertIntEquals(tc, 0, rol_seelie_prismatic_damage(3));
  CuAssertIntEquals(tc, 0, rol_seelie_prismatic_damage(8));
  CuAssertIntEquals(tc, 3, rol_seelie_search_stun_rounds(2062701));
  CuAssertIntEquals(tc, 6, rol_seelie_search_stun_rounds(2062707));
  CuAssertIntEquals(tc, 0, rol_seelie_search_stun_rounds(2062708));
}

void Test_spec_rol_manscorpion_venom_profiles_and_affect(CuTest *tc)
{
  struct spec_mechanics_fixture fixture;
  struct affected_type *af;
  bool fatal;
  int denominator;
  int duration;

  CuAssertTrue(tc, rol_manscorpion_venom_profile(2043703, &denominator, &duration, &fatal));
  CuAssertIntEquals(tc, 31, denominator);
  CuAssertIntEquals(tc, 6, duration);
  CuAssertTrue(tc, !fatal);
  CuAssertTrue(tc, rol_manscorpion_venom_profile(2043702, &denominator, &duration, &fatal));
  CuAssertIntEquals(tc, 7, denominator);
  CuAssertIntEquals(tc, 4, duration);
  CuAssertTrue(tc, !fatal);
  CuAssertTrue(tc, rol_manscorpion_venom_profile(2043756, &denominator, &duration, &fatal));
  CuAssertIntEquals(tc, 11, denominator);
  CuAssertIntEquals(tc, 2, duration);
  CuAssertTrue(tc, !fatal);
  CuAssertTrue(tc, rol_manscorpion_venom_profile(2043767, &denominator, &duration, &fatal));
  CuAssertIntEquals(tc, 25, denominator);
  CuAssertIntEquals(tc, 1, duration);
  CuAssertTrue(tc, fatal);
  CuAssertTrue(tc, !rol_manscorpion_venom_profile(2043766, NULL, NULL, NULL));
  CuAssertTrue(tc, rol_manscorpion_venom_roll_fires(2043703, 1));
  CuAssertTrue(tc, !rol_manscorpion_venom_roll_fires(2043703, 0));
  CuAssertTrue(tc, !rol_manscorpion_venom_roll_fires(2043703, 31));
  CuAssertTrue(tc, !rol_manscorpion_venom_roll_fires(2043703, 32));

  spec_mechanics_begin(&fixture);
  CuAssertTrue(tc, rol_manscorpion_apply_venom(&fixture.target, 6));
  af = fixture.target.affected;
  CuAssertPtrNotNull(tc, af);
  if (af != NULL)
  {
    CuAssertIntEquals(tc, AFFECT_ROL_MANSCORPION_VENOM, af->spell);
    CuAssertIntEquals(tc, 6, af->duration);
    CuAssertIntEquals(tc, APPLY_CON, af->location);
    CuAssertIntEquals(tc, -2, af->modifier);
  }
  CuAssertTrue(tc, !rol_manscorpion_apply_venom(&fixture.target, 2));
  CuAssertTrue(tc, !rol_manscorpion_apply_venom(NULL, 2));
  CuAssertTrue(tc, !rol_manscorpion_apply_venom(&fixture.actor, 0));
  spec_mechanics_end(&fixture);
}

void Test_spec_rol_planar_death_and_burst_profiles(CuTest *tc)
{
  struct spec_mechanics_fixture fixture;
  struct spec_event_context context;
  bool flame_spikes;
  bool screech;
  bool spores;
  bool suppresses_corpse;

  CuAssertTrue(tc, rol_planar_death_profile(2000207, &suppresses_corpse));
  CuAssertTrue(tc, suppresses_corpse);
  CuAssertTrue(tc, rol_planar_death_profile(2093204, &suppresses_corpse));
  CuAssertTrue(tc, suppresses_corpse);
  CuAssertTrue(tc, rol_planar_death_profile(2000214, &suppresses_corpse));
  CuAssertTrue(tc, !suppresses_corpse);
  CuAssertTrue(tc, !rol_planar_death_profile(2000213, NULL));

  CuAssertTrue(tc, rol_planar_burst_profile(2000221, &screech, &spores, &flame_spikes));
  CuAssertTrue(tc, screech);
  CuAssertTrue(tc, spores);
  CuAssertTrue(tc, !flame_spikes);
  CuAssertTrue(tc, rol_planar_burst_profile(2032645, &screech, &spores, &flame_spikes));
  CuAssertTrue(tc, !screech);
  CuAssertTrue(tc, !spores);
  CuAssertTrue(tc, flame_spikes);
  CuAssertTrue(tc, !rol_planar_burst_profile(2032644, NULL, NULL, NULL));
  CuAssertTrue(tc, !rol_planar_five_in_six_roll_fires(0));
  CuAssertTrue(tc, rol_planar_five_in_six_roll_fires(1));
  CuAssertTrue(tc, rol_planar_five_in_six_roll_fires(5));
  CuAssertTrue(tc, !rol_planar_five_in_six_roll_fires(6));
  CuAssertTrue(tc, rol_planar_screech_health_allows(15, 100));
  CuAssertTrue(tc, !rol_planar_screech_health_allows(16, 100));
  CuAssertTrue(tc, !rol_planar_screech_health_allows(1, 0));
  CuAssertIntEquals(tc, SECS_PER_MUD_DAY, rol_planar_screech_cooldown_seconds(2093209));
  CuAssertIntEquals(tc, 0, rol_planar_screech_cooldown_seconds(2032632));
  CuAssertIntEquals(tc, PULSE_VIOLENCE * 3, rol_planar_hit_burst_cooldown_seconds(2000221));
  CuAssertIntEquals(tc, PULSE_VIOLENCE * 3, rol_planar_hit_burst_cooldown_seconds(2033020));

  spec_mechanics_begin(&fixture);
  memset(&context, 0, sizeof(context));
  fixture.mobile_indexes[0].vnum = 2000207;
  GET_MOB_RNUM(&fixture.actor) = 0;
  context.owner_type = SPEC_OWNER_MOBILE;
  context.event = SPEC_EVENT_MOBILE_DEATH;
  context.owner = &fixture.actor;
  context.actor = &fixture.target;
  context.target = &fixture.target;
  CuAssertIntEquals(tc, TRUE, rol_monster_combat_typed(&context));
  spec_mechanics_end(&fixture);
}

void Test_spec_rol_planar_control_and_vrock_dance_profiles(CuTest *tc)
{
  struct spec_mechanics_fixture fixture;
  struct spec_event_context context;
  struct command_info commands[3];
  struct command_info *saved_complete_cmd_info;
  enum rol_planar_control_kind kind;
  int denominator;

  CuAssertTrue(tc, rol_planar_control_profile(2000212, &kind, &denominator));
  CuAssertIntEquals(tc, ROL_PLANAR_CONTROL_GLABREZU, kind);
  CuAssertIntEquals(tc, 11, denominator);
  CuAssertTrue(tc, rol_planar_control_profile(2093206, &kind, &denominator));
  CuAssertIntEquals(tc, ROL_PLANAR_CONTROL_MARILITH, kind);
  CuAssertIntEquals(tc, 11, denominator);
  CuAssertTrue(tc, rol_planar_control_profile(2000220, &kind, &denominator));
  CuAssertIntEquals(tc, ROL_PLANAR_CONTROL_SUCCUBUS, kind);
  CuAssertIntEquals(tc, 4, denominator);
  CuAssertTrue(tc, !rol_planar_control_profile(2000221, NULL, NULL));

  CuAssertTrue(tc, rol_planar_captive_command_allowed("look"));
  CuAssertTrue(tc, rol_planar_captive_command_allowed("petition"));
  CuAssertTrue(tc, !rol_planar_captive_command_allowed("kill"));
  CuAssertTrue(tc, !rol_planar_captive_command_allowed(NULL));
  CuAssertTrue(tc, rol_planar_restrain_agility_evades(20, 9));
  CuAssertTrue(tc, !rol_planar_restrain_agility_evades(20, 10));
  CuAssertTrue(tc, rol_planar_restrain_constitution_survives(20, 20));
  CuAssertTrue(tc, !rol_planar_restrain_constitution_survives(20, 21));
  CuAssertTrue(tc, rol_planar_restrain_constitution_survives(90, 85));
  CuAssertTrue(tc, rol_planar_succubus_charm_roll_fires(0));
  CuAssertTrue(tc, !rol_planar_succubus_charm_roll_fires(1));
  CuAssertIntEquals(tc, SECS_PER_MUD_HOUR, rol_planar_succubus_kiss_delay_seconds(1));
  CuAssertIntEquals(tc, SECS_PER_MUD_HOUR * 4, rol_planar_succubus_kiss_delay_seconds(4));
  CuAssertIntEquals(tc, 0, rol_planar_succubus_kiss_delay_seconds(5));

  CuAssertTrue(tc, rol_planar_vrock_dance_profile(2000221));
  CuAssertTrue(tc, rol_planar_vrock_dance_profile(2093210));
  CuAssertTrue(tc, !rol_planar_vrock_dance_profile(2093209 - 1));
  CuAssertIntEquals(tc, 5, rol_planar_vrock_dance_required_count());
  CuAssertIntEquals(tc, PULSE_VIOLENCE, rol_planar_vrock_dance_step_seconds());
  CuAssertIntEquals(tc, SECS_PER_MUD_DAY, rol_planar_vrock_dance_cooldown_seconds());

  spec_mechanics_begin(&fixture);
  memset(&context, 0, sizeof(context));
  memset(commands, 0, sizeof(commands));
  saved_complete_cmd_info = complete_cmd_info;
  complete_cmd_info = commands;
  commands[1].command = "kill";
  commands[2].command = "look";
  fixture.mobile_indexes[0].vnum = 2000212;
  GET_MOB_RNUM(&fixture.actor) = 0;
  fixture.target.master = &fixture.actor;
  SET_BIT_AR(AFF_FLAGS(&fixture.target), AFF_CHARM);
  context.owner_type = SPEC_OWNER_MOBILE;
  context.event = SPEC_EVENT_COMMAND;
  context.owner = &fixture.actor;
  context.actor = &fixture.target;
  context.command = 1;
  CuAssertIntEquals(tc, TRUE, rol_monster_combat_typed(&context));
  context.command = 2;
  CuAssertIntEquals(tc, FALSE, rol_monster_combat_typed(&context));
  complete_cmd_info = saved_complete_cmd_info;
  spec_mechanics_end(&fixture);
}

void Test_spec_rol_avernus_devil_profiles_preserve_source_rolls(CuTest *tc)
{
  bool blocks_disarm;
  bool freezing_tail;
  bool glaive;
  bool silence;
  bool spear;

  CuAssertTrue(tc, rol_avernus_barbazu_profile(2032629));
  CuAssertTrue(tc, rol_avernus_barbazu_profile(2033022));
  CuAssertTrue(tc, !rol_avernus_barbazu_profile(2033020));
  CuAssertTrue(tc, rol_avernus_barbazu_berserk_roll_fires(0));
  CuAssertTrue(tc, !rol_avernus_barbazu_berserk_roll_fires(1));
  CuAssertTrue(tc, !rol_avernus_barbazu_berserk_roll_fires(20));

  CuAssertTrue(tc, rol_avernus_gelugon_profile(2033015, &freezing_tail, &silence, &blocks_disarm));
  CuAssertTrue(tc, freezing_tail);
  CuAssertTrue(tc, silence);
  CuAssertTrue(tc, !blocks_disarm);
  CuAssertTrue(tc, rol_avernus_gelugon_profile(2033016, &freezing_tail, &silence, &blocks_disarm));
  CuAssertTrue(tc, freezing_tail);
  CuAssertTrue(tc, !silence);
  CuAssertTrue(tc, blocks_disarm);
  CuAssertTrue(tc, !rol_avernus_gelugon_profile(2033014, NULL, NULL, NULL));
  CuAssertTrue(tc, rol_avernus_gelugon_tail_roll_fires(0));
  CuAssertTrue(tc, !rol_avernus_gelugon_tail_roll_fires(1));
  CuAssertTrue(tc, rol_avernus_meritos_silence_roll_fires(0));
  CuAssertTrue(tc, !rol_avernus_meritos_silence_roll_fires(1));

  CuAssertTrue(tc, rol_avernus_weapon_profile(2032602, &glaive, &spear));
  CuAssertTrue(tc, glaive);
  CuAssertTrue(tc, !spear);
  CuAssertTrue(tc, rol_avernus_weapon_profile(2033012, &glaive, &spear));
  CuAssertTrue(tc, !glaive);
  CuAssertTrue(tc, spear);
  CuAssertTrue(tc, !rol_avernus_weapon_profile(2033013, NULL, NULL));
  CuAssertTrue(tc, rol_gelugon_freeze_spear_roll_fires(0));
  CuAssertTrue(tc, !rol_gelugon_freeze_spear_roll_fires(1));

  CuAssertIntEquals(tc, 60, rol_barbazu_bloodloss_next_hit(100));
  CuAssertIntEquals(tc, -9, rol_barbazu_bloodloss_next_hit(20));
  CuAssertIntEquals(tc, -9, rol_barbazu_bloodloss_next_hit(-9));
  CuAssertIntEquals(tc, -5, rol_barbazu_bloodloss_next_hit(-5));
}

void Test_spec_rol_avernus_remaining_profiles_cover_lifecycle_and_routes(CuTest *tc)
{
  const char *description = NULL;
  bool commands = false;
  bool activity = false;
  bool death = false;
  bool pulse = false;
  bool weapon_hit = false;

  CuAssertIntEquals(tc, 15, (int)rol_avernus_mobile_profile_count());
  CuAssertTrue(tc, rol_avernus_mobile_profile(2032623, &commands, &activity, &death));
  CuAssertTrue(tc, !commands);
  CuAssertTrue(tc, !activity);
  CuAssertTrue(tc, death);
  CuAssertTrue(tc, rol_avernus_mobile_profile(2033014, &commands, &activity, &death));
  CuAssertTrue(tc, commands);
  CuAssertTrue(tc, activity);
  CuAssertTrue(tc, death);
  CuAssertTrue(tc, rol_avernus_mobile_profile(2033027, &commands, &activity, &death));
  CuAssertTrue(tc, commands && activity && death);
  CuAssertTrue(tc, !rol_avernus_mobile_profile(2033013, NULL, NULL, NULL));

  CuAssertIntEquals(tc, 4, (int)rol_avernus_object_profile_count());
  CuAssertTrue(tc, rol_avernus_object_profile(2032631, &commands, &pulse, &weapon_hit));
  CuAssertTrue(tc, commands && pulse && !weapon_hit);
  CuAssertTrue(tc, rol_avernus_object_profile(2033011, &commands, &pulse, &weapon_hit));
  CuAssertTrue(tc, !commands && pulse && weapon_hit);
  CuAssertTrue(tc, rol_avernus_object_profile(2033021, &commands, &pulse, &weapon_hit));
  CuAssertTrue(tc, commands && pulse && weapon_hit);
  CuAssertTrue(tc, !rol_avernus_object_profile(2033006, NULL, NULL, NULL));

  CuAssertIntEquals(tc, EAST, rol_avernus_patrol_direction(2032641, 2032908));
  CuAssertIntEquals(tc, WEST, rol_avernus_patrol_direction(2032643, 2032908));
  CuAssertIntEquals(tc, NORTH, rol_avernus_patrol_direction(2033008, 2033028));
  CuAssertIntEquals(tc, WEST, rol_avernus_patrol_direction(2033021, 2033028));
  CuAssertIntEquals(tc, -1, rol_avernus_patrol_direction(2033000, 2033001));

  CuAssertTrue(tc, rol_avernus_garden_room_vnum(2032672));
  CuAssertTrue(tc, rol_avernus_garden_room_vnum(2032687));
  CuAssertTrue(tc, !rol_avernus_garden_room_vnum(2032671));
  CuAssertTrue(tc, !rol_avernus_garden_room_vnum(2032688));
  CuAssertTrue(tc, rol_monster_combat_profile(2033014, NULL, &description));
  CuAssertPtrNotNull(tc, description);
}

void Test_spec_rol_balor_weapon_profiles_and_whip_proc(CuTest *tc)
{
  struct spec_mechanics_fixture fixture;
  struct spec_event_context context;
  bool critical_only;
  int damage_type;
  int denominator;
  int dice_count;
  int dice_size;
  int previous_hit;

  CuAssertTrue(tc, rol_balor_weapon_profile(2093227, &dice_count, &dice_size, &damage_type));
  CuAssertIntEquals(tc, 8, dice_count);
  CuAssertIntEquals(tc, 6, dice_size);
  CuAssertIntEquals(tc, DAM_FORCE, damage_type);
  CuAssertTrue(tc, rol_weapon_profile(2093227, &denominator, &critical_only, NULL));
  CuAssertIntEquals(tc, 1, denominator);
  CuAssertTrue(tc, !critical_only);
  CuAssertTrue(tc, rol_balor_weapon_profile(2093228, &dice_count, &dice_size, &damage_type));
  CuAssertIntEquals(tc, 20, dice_count);
  CuAssertIntEquals(tc, 10, dice_size);
  CuAssertIntEquals(tc, DAM_NEGATIVE, damage_type);
  CuAssertTrue(tc, rol_weapon_profile(2093228, &denominator, &critical_only, NULL));
  CuAssertTrue(tc, critical_only);
  CuAssertTrue(tc, !rol_balor_weapon_profile(2093226, NULL, NULL, NULL));

  spec_mechanics_begin(&fixture);
  CuAssertTrue(tc, !rol_balor_weapon_owner_allowed(&fixture.actor, false));
  SET_BIT_AR(MOB_FLAGS(&fixture.actor), MOB_ROL_DEMON);
  CuAssertTrue(tc, rol_balor_weapon_owner_allowed(&fixture.actor, false));
  fixture.actor.master = &fixture.target;
  SET_BIT_AR(AFF_FLAGS(&fixture.actor), AFF_CHARM);
  CuAssertTrue(tc, !rol_balor_weapon_owner_allowed(&fixture.actor, false));
  CuAssertTrue(tc, rol_balor_weapon_owner_allowed(&fixture.actor, true));
  fixture.actor.master = NULL;
  REMOVE_BIT_AR(AFF_FLAGS(&fixture.actor), AFF_CHARM);

  fixture.object_indexes[0].vnum = 2093227;
  GET_OBJ_RNUM(&fixture.worn) = 0;
  fixture.worn.worn_by = &fixture.actor;
  fixture.worn.worn_on = WEAR_WIELD_1;
  GET_EQ(&fixture.actor, WEAR_WIELD_1) = &fixture.worn;
  FIGHTING(&fixture.actor) = &fixture.target;
  FIGHTING(&fixture.target) = &fixture.actor;
  memset(&context, 0, sizeof(context));
  context.owner_type = SPEC_OWNER_OBJECT;
  context.event = SPEC_EVENT_WEAPON_HIT;
  context.owner = &fixture.worn;
  context.actor = &fixture.actor;
  context.target = &fixture.target;
  previous_hit = GET_HIT(&fixture.target);
  CuAssertIntEquals(tc, TRUE, rol_weapon_proc_typed(&context));
  CuAssertTrue(tc, GET_HIT(&fixture.target) <= previous_hit - 8);
  CuAssertTrue(tc, GET_HIT(&fixture.target) >= previous_hit - 48);

  GET_EQ(&fixture.actor, WEAR_WIELD_1) = NULL;
  fixture.worn.worn_by = NULL;
  spec_mechanics_end(&fixture);
}

void Test_spec_rol_successful_hit_area_profiles(CuTest *tc)
{
  struct rol_monster_hit_profile_view view;

  CuAssertTrue(tc, rol_monster_successful_hit_profile(2021820, &view));
  CuAssertIntEquals(tc, 4, view.proc_denominator);
  CuAssertIntEquals(tc, 150, view.base_damage);
  CuAssertIntEquals(tc, DAM_SOUND, view.damage_type);
  CuAssertTrue(tc, !view.fatal);

  CuAssertTrue(tc, rol_monster_successful_hit_profile(2021786, &view));
  CuAssertIntEquals(tc, 5, view.proc_denominator);
  CuAssertIntEquals(tc, 0, view.base_damage);
  CuAssertIntEquals(tc, DAM_SLASHING, view.damage_type);

  CuAssertTrue(tc, rol_monster_successful_hit_profile(2043705, &view));
  CuAssertIntEquals(tc, 16, view.proc_denominator);
  CuAssertIntEquals(tc, 10, view.damage_dice_count);
  CuAssertIntEquals(tc, 10, view.damage_dice_size);
  CuAssertIntEquals(tc, DAM_EARTH, view.damage_type);

  CuAssertTrue(tc, rol_monster_successful_hit_profile(2096631, &view));
  CuAssertIntEquals(tc, 6, view.proc_denominator);
  CuAssertIntEquals(tc, 200, view.base_damage);
  CuAssertIntEquals(tc, 10, view.damage_variance);
  CuAssertIntEquals(tc, DAM_SOUND, view.damage_type);

  CuAssertTrue(tc, rol_monster_successful_hit_profile(2096670, &view));
  CuAssertIntEquals(tc, 11, view.proc_denominator);
  CuAssertIntEquals(tc, 300, view.base_damage);
  CuAssertIntEquals(tc, 10, view.damage_variance);
  CuAssertIntEquals(tc, DAM_POISON, view.damage_type);

  CuAssertTrue(tc, rol_monster_successful_hit_profile(2096672, &view));
  CuAssertIntEquals(tc, 11, view.proc_denominator);
  CuAssertTrue(tc, view.fatal);
  CuAssertIntEquals(tc, DAM_LIGHT, view.damage_type);
  CuAssertTrue(tc, rol_monster_successful_hit_roll_fires(2096672, 1));
  CuAssertTrue(tc, !rol_monster_successful_hit_roll_fires(2096672, 2));
  CuAssertTrue(tc, !rol_monster_successful_hit_profile(2096671, NULL));
  CuAssertTrue(tc, !rol_monster_successful_hit_roll_fires(2096671, 1));
}

void Test_spec_rol_skriaxit_sandstorm_profiles_and_cadence(CuTest *tc)
{
  struct spec_mechanics_fixture fixture;
  struct spec_event_context context;
  struct player_special_data player_specials;
  bool adjacent;
  bool fires;
  int interval;
  int round;

  CuAssertTrue(tc, rol_skriaxit_sandstorm_profile(2043741, &interval, &adjacent));
  CuAssertIntEquals(tc, 3, interval);
  CuAssertTrue(tc, adjacent);
  CuAssertTrue(tc, rol_skriaxit_sandstorm_profile(2043742, NULL, NULL));
  CuAssertTrue(tc, !rol_skriaxit_sandstorm_profile(2043740, NULL, NULL));
  CuAssertIntEquals(tc, 0, rol_skriaxit_sandstorm_source_damage(0));
  CuAssertIntEquals(tc, 0, rol_skriaxit_sandstorm_source_damage(7));

  round = rol_skriaxit_sandstorm_advance_round(0, &fires);
  CuAssertIntEquals(tc, 1, round);
  CuAssertTrue(tc, !fires);
  round = rol_skriaxit_sandstorm_advance_round(round, &fires);
  CuAssertIntEquals(tc, 2, round);
  CuAssertTrue(tc, !fires);
  round = rol_skriaxit_sandstorm_advance_round(round, &fires);
  CuAssertIntEquals(tc, 0, round);
  CuAssertTrue(tc, fires);
  CuAssertIntEquals(tc, 1, rol_skriaxit_sandstorm_advance_round(-1, NULL));

  spec_mechanics_begin(&fixture);
  memset(&context, 0, sizeof(context));
  memset(&player_specials, 0, sizeof(player_specials));
  fixture.mobile_indexes[0].vnum = 2043741;
  GET_MOB_RNUM(&fixture.actor) = 0;
  REMOVE_BIT_AR(MOB_FLAGS(&fixture.target), MOB_ISNPC);
  fixture.target.player_specials = &player_specials;
  context.owner_type = SPEC_OWNER_MOBILE;
  context.event = SPEC_EVENT_MOBILE_ACTIVITY;
  context.owner = &fixture.actor;
  context.actor = &fixture.actor;

  CuAssertIntEquals(tc, FALSE, rol_monster_combat_typed(&context));
  CuAssertIntEquals(tc, 1, fixture.actor.mob_specials.proc_fired);
  CuAssertIntEquals(tc, FALSE, rol_monster_combat_typed(&context));
  CuAssertIntEquals(tc, 2, fixture.actor.mob_specials.proc_fired);
  CuAssertIntEquals(tc, FALSE, rol_monster_combat_typed(&context));
  CuAssertIntEquals(tc, 0, fixture.actor.mob_specials.proc_fired);

  fixture.target.player_specials = &dummy_mob;
  SET_BIT_AR(MOB_FLAGS(&fixture.target), MOB_ISNPC);
  spec_mechanics_end(&fixture);
}

void Test_spec_rol_seelie_search_reveals_first_hidden_target(CuTest *tc)
{
  struct spec_mechanics_fixture fixture;
  struct spec_event_context context;

  spec_mechanics_begin(&fixture);
  memset(&context, 0, sizeof(context));
  fixture.mobile_indexes[0].vnum = 2062707;
  GET_MOB_RNUM(&fixture.actor) = 0;
  fixture.target.master = &fixture.actor;
  SET_BIT_AR(AFF_FLAGS(&fixture.target), AFF_CHARM);
  SET_BIT_AR(AFF_FLAGS(&fixture.target), AFF_HIDE);
  SET_BIT_AR(MOB_FLAGS(&fixture.target), MOB_NOPARALYZE);

  context.owner_type = SPEC_OWNER_MOBILE;
  context.event = SPEC_EVENT_MOBILE_ACTIVITY;
  context.owner = &fixture.actor;
  context.actor = &fixture.actor;
  CuAssertIntEquals(tc, TRUE, rol_monster_combat_typed(&context));
  CuAssertTrue(tc, !AFF_FLAGGED(&fixture.target, AFF_HIDE));
  CuAssertIntEquals(tc, POS_RECLINING, GET_POS(&fixture.target));
  CuAssertTrue(tc, char_has_mud_event(&fixture.target, eSTUNNED) == NULL);

  fixture.target.master = NULL;
  REMOVE_BIT_AR(AFF_FLAGS(&fixture.target), AFF_CHARM);
  spec_mechanics_end(&fixture);
}

void Test_spec_rol_residual_mobile_profiles_cover_converted_bindings(CuTest *tc)
{
  static const int vnums[] = {
      2001228, 2001229, 2005718, 2014015, 2014029, 2020247, 2026208,
      2026216, 2026236, 2026244, 2026245, 2059815, 2059835,
  };
  const char *description;
  size_t index;

  CuAssertIntEquals(tc, (int)(sizeof(vnums) / sizeof(vnums[0])),
                    (int)rol_residual_mobile_profile_count());
  for (index = 0; index < sizeof(vnums) / sizeof(vnums[0]); index++)
  {
    description = NULL;
    CuAssertTrue(tc, rol_residual_mobile_profile(vnums[index], &description));
    CuAssertPtrNotNull(tc, description);
  }
  CuAssertTrue(tc, !rol_residual_mobile_profile(9999999, NULL));
}

void Test_spec_rol_bloodstone_critter_preserves_social_cadence(CuTest *tc)
{
  struct spec_mechanics_fixture fixture;

  spec_mechanics_begin(&fixture);

  CuAssertStrEquals(tc, "snarl", rol_bloodstone_critter_social(0));
  CuAssertStrEquals(tc, "growl", rol_bloodstone_critter_social(1));
  CuAssertTrue(tc, rol_bloodstone_critter_social(-1) == NULL);
  CuAssertTrue(tc, rol_bloodstone_critter_social(2) == NULL);
  CuAssertTrue(tc, rol_bloodstone_critter_social(80) == NULL);
  CuAssertTrue(tc, rol_bloodstone_critter_social(81) == NULL);

  GET_POS(&fixture.actor) = POS_SLEEPING;
  CuAssertIntEquals(tc, FALSE, rol_bloodstone_critter(&fixture.actor, &fixture.actor, 0, ""));
  GET_POS(&fixture.actor) = POS_STANDING;
  CuAssertIntEquals(tc, FALSE, rol_bloodstone_critter(&fixture.actor, &fixture.actor, 1, ""));

  spec_mechanics_end(&fixture);
}

void Test_spec_rol_item_blocker_preserves_direction_and_aggressor_gate(CuTest *tc)
{
  struct spec_mechanics_fixture fixture;
  struct command_info commands[2];
  struct command_info *saved_complete_cmd_info;
  struct player_special_data player_specials;
  struct room_direction_data north_exit;

  spec_mechanics_begin(&fixture);
  memset(commands, 0, sizeof(commands));
  memset(&player_specials, 0, sizeof(player_specials));
  memset(&north_exit, 0, sizeof(north_exit));
  saved_complete_cmd_info = complete_cmd_info;
  complete_cmd_info = commands;
  REMOVE_BIT_AR(MOB_FLAGS(&fixture.actor), MOB_ISNPC);
  fixture.actor.player.name = "blocked traveler";
  fixture.actor.player_specials = &player_specials;
  SET_BIT_AR(MOB_FLAGS(&fixture.target), MOB_AGGRESSIVE);

  commands[1].command = "north";
  GET_OBJ_VAL(&fixture.worn, 0) = NORTH;
  CuAssertIntEquals(tc, TRUE, rol_item_blocker(&fixture.actor, &fixture.worn, 1, ""));
  GET_OBJ_VAL(&fixture.worn, 0) = EAST;
  CuAssertIntEquals(tc, FALSE, rol_item_blocker(&fixture.actor, &fixture.worn, 1, ""));

  commands[1].command = "unlock";
  GET_OBJ_VAL(&fixture.worn, 0) = NORTH;
  north_exit.keyword = "gate";
  fixture.rooms[0].dir_option[NORTH] = &north_exit;
  CuAssertIntEquals(tc, TRUE, rol_item_blocker(&fixture.actor, &fixture.worn, 1, "gate north"));
  SET_BIT(north_exit.exit_info, EX_HIDDEN);
  CuAssertIntEquals(tc, FALSE, rol_item_blocker(&fixture.actor, &fixture.worn, 1, "gate north"));

  REMOVE_BIT_AR(MOB_FLAGS(&fixture.target), MOB_AGGRESSIVE);
  commands[1].command = "north";
  CuAssertIntEquals(tc, FALSE, rol_item_blocker(&fixture.actor, &fixture.worn, 1, ""));

  fixture.rooms[0].dir_option[NORTH] = NULL;
  fixture.actor.player_specials = &dummy_mob;
  SET_BIT_AR(MOB_FLAGS(&fixture.actor), MOB_ISNPC);
  complete_cmd_info = saved_complete_cmd_info;
  spec_mechanics_end(&fixture);
}

void Test_spec_rol_designated_follower_requires_matching_awake_leader(CuTest *tc)
{
  struct spec_mechanics_fixture fixture;
  struct index_data indexes[2];
  struct index_data *fixture_indexes;
  mob_rnum fixture_top_of_mobt;

  spec_mechanics_begin(&fixture);
  memset(indexes, 0, sizeof(indexes));
  fixture_indexes = mob_index;
  fixture_top_of_mobt = top_of_mobt;
  mob_index = indexes;
  top_of_mobt = 1;
  indexes[0].vnum = 2097009;
  indexes[1].vnum = 2097011;
  GET_MOB_RNUM(&fixture.actor) = 0;
  GET_MOB_RNUM(&fixture.target) = 1;

  CuAssertIntEquals(tc, FALSE, rol_designated_follower(&fixture.actor, &fixture.actor, 0, ""));
  indexes[1].vnum = 2097012;
  GET_POS(&fixture.actor) = POS_SLEEPING;
  CuAssertIntEquals(tc, FALSE, rol_designated_follower(&fixture.actor, &fixture.actor, 0, ""));
  GET_POS(&fixture.actor) = POS_STANDING;
  CuAssertIntEquals(tc, TRUE, rol_designated_follower(&fixture.actor, &fixture.actor, 0, ""));
  CuAssertPtrEquals(tc, &fixture.target, fixture.actor.master);
  CuAssertPtrNotNull(tc, fixture.target.followers);
  CuAssertPtrEquals(tc, &fixture.actor, fixture.target.followers->follower);
  CuAssertIntEquals(tc, FALSE, rol_designated_follower(&fixture.actor, &fixture.actor, 0, ""));

  stop_follower(&fixture.actor);
  mob_index = fixture_indexes;
  top_of_mobt = fixture_top_of_mobt;
  spec_mechanics_end(&fixture);
}

void Test_spec_rol_fixed_bodyguard_profiles_match_only_assigned_mobile(CuTest *tc)
{
  struct spec_mechanics_fixture fixture;

  CuAssertTrue(tc, rol_fixed_bodyguard_protects(2097040, 2097023));
  CuAssertTrue(tc, rol_fixed_bodyguard_protects(2097041, 2097029));
  CuAssertTrue(tc, rol_fixed_bodyguard_protects(2097042, 2097008));
  CuAssertTrue(tc, !rol_fixed_bodyguard_protects(2097040, 2097029));
  CuAssertTrue(tc, !rol_fixed_bodyguard_protects(9999999, 2097023));

  spec_mechanics_begin(&fixture);
  fixture.mobile_indexes[0].vnum = 2097040;
  fixture.mobile_indexes[1].vnum = 2097023;
  GET_MOB_RNUM(&fixture.actor) = 0;
  GET_MOB_RNUM(&fixture.target) = 1;
  CuAssertIntEquals(tc, FALSE, rol_fixed_bodyguard(&fixture.actor, &fixture.actor, 0, ""));
  GET_POS(&fixture.actor) = POS_SLEEPING;
  CuAssertIntEquals(tc, FALSE, rol_fixed_bodyguard(&fixture.actor, &fixture.actor, 0, ""));
  spec_mechanics_end(&fixture);
}

void Test_spec_rol_floating_pool_uses_documented_movement_chance(CuTest *tc)
{
  CuAssertTrue(tc, rol_floating_pool_should_move(1));
  CuAssertTrue(tc, rol_floating_pool_should_move(12));
  CuAssertTrue(tc, !rol_floating_pool_should_move(0));
  CuAssertTrue(tc, !rol_floating_pool_should_move(13));
  CuAssertTrue(tc, !rol_floating_pool_should_move(100));
}

void Test_spec_rol_major_beholder_preserves_eye_mapping_and_cooldowns(CuTest *tc)
{
  struct spec_mechanics_fixture fixture;
  int state;

  spec_mechanics_begin(&fixture);

  CuAssertIntEquals(tc, SPELL_FIREBALL, rol_major_beholder_eye_spell(0));
  CuAssertIntEquals(tc, SPELL_ACID_ARROW, rol_major_beholder_eye_spell(1));
  CuAssertIntEquals(tc, SPELL_SLOW, rol_major_beholder_eye_spell(2));
  CuAssertIntEquals(tc, SPELL_RAY_OF_ENFEEBLEMENT, rol_major_beholder_eye_spell(3));
  CuAssertIntEquals(tc, PSIONIC_WITHER, rol_major_beholder_eye_spell(4));
  CuAssertIntEquals(tc, SPELL_DISPEL_MAGIC, rol_major_beholder_eye_spell(5));
  CuAssertIntEquals(tc, SPELL_PRISMATIC_SPRAY, rol_major_beholder_eye_spell(6));
  CuAssertIntEquals(tc, SPELL_HOLD_MONSTER, rol_major_beholder_eye_spell(7));
  CuAssertIntEquals(tc, SPELL_HARM, rol_major_beholder_eye_spell(8));
  CuAssertIntEquals(tc, SPELL_FINGER_OF_DEATH, rol_major_beholder_eye_spell(9));
  CuAssertIntEquals(tc, -1, rol_major_beholder_eye_spell(-1));
  CuAssertIntEquals(tc, -1, rol_major_beholder_eye_spell(10));

  state = rol_major_beholder_advance_cooldowns(0, (1U << 0) | (1U << 9));
  CuAssertIntEquals(tc, 3, rol_major_beholder_eye_cooldown(state, 0));
  CuAssertIntEquals(tc, 0, rol_major_beholder_eye_cooldown(state, 1));
  CuAssertIntEquals(tc, 3, rol_major_beholder_eye_cooldown(state, 9));
  state = rol_major_beholder_advance_cooldowns(state, 0);
  CuAssertIntEquals(tc, 2, rol_major_beholder_eye_cooldown(state, 0));
  CuAssertIntEquals(tc, 2, rol_major_beholder_eye_cooldown(state, 9));
  CuAssertIntEquals(tc, -1, rol_major_beholder_eye_cooldown(state, 10));

  FIGHTING(&fixture.actor) = &fixture.target;
  FIGHTING(&fixture.target) = &fixture.actor;
  GET_POS(&fixture.actor) = POS_FIGHTING;
  GET_POS(&fixture.target) = POS_FIGHTING;
  fixture.actor.mob_specials.proc_fired = 0xFFFFF;
  CuAssertIntEquals(tc, FALSE, rol_major_beholder(&fixture.actor, &fixture.actor, 0, ""));
  CuAssertIntEquals(tc, 0xAAAAA, fixture.actor.mob_specials.proc_fired);

  spec_mechanics_end(&fixture);
}

void Test_spec_rol_lich_energy_drain_preserves_party_targeting_and_life_transfer(CuTest *tc)
{
  struct spec_mechanics_fixture fixture;
  struct char_data leader;

  spec_mechanics_begin(&fixture);
  spec_mechanics_initialize_npc(&leader, "party leader", 0);

  CuAssertTrue(tc, rol_lich_energy_drain_together(&fixture.target, &fixture.target));
  CuAssertTrue(tc, !rol_lich_energy_drain_together(&fixture.actor, &fixture.target));
  fixture.actor.master = &fixture.target;
  CuAssertTrue(tc, rol_lich_energy_drain_together(&fixture.actor, &fixture.target));
  fixture.actor.master = NULL;
  fixture.actor.master = &leader;
  fixture.target.master = &leader;
  CuAssertTrue(tc, rol_lich_energy_drain_together(&fixture.actor, &fixture.target));
  fixture.actor.master = NULL;
  fixture.target.master = NULL;

  CuAssertIntEquals(tc, -5, rol_lich_energy_drain_victim_hit(137, false));
  CuAssertIntEquals(tc, 0, rol_lich_energy_drain_victim_hit(137, true));
  CuAssertIntEquals(tc, -2, rol_lich_energy_drain_victim_hit(-2, false));
  CuAssertIntEquals(tc, 237, rol_lich_energy_drain_healer_hit(100, 137, false));
  CuAssertIntEquals(tc, 100, rol_lich_energy_drain_healer_hit(100, 137, true));
  CuAssertIntEquals(tc, INT_MAX, rol_lich_energy_drain_healer_hit(INT_MAX - 2, 137, false));
  CuAssertTrue(tc, rol_lich_energy_drain_stun_duration(0) == PULSE_VIOLENCE * 2);
  CuAssertTrue(tc, rol_lich_energy_drain_stun_duration(7) == (PULSE_VIOLENCE * 2) + 7);
  CuAssertTrue(tc, rol_lich_energy_drain_stun_duration(LONG_MAX) == LONG_MAX);

  FIGHTING(&fixture.actor) = &fixture.target;
  IS_CASTING(&fixture.actor) = TRUE;
  CuAssertIntEquals(tc, FALSE, rol_lich_energy_drain(&fixture.actor, &fixture.actor, 0, ""));
  CuAssertIntEquals(tc, 100, GET_HIT(&fixture.target));
  IS_CASTING(&fixture.actor) = FALSE;

  spec_mechanics_end(&fixture);
}

void Test_spec_rol_trade_bandit_preserves_cargo_tolls_and_cleanup_timer(CuTest *tc)
{
  struct spec_mechanics_fixture fixture;
  struct player_special_data player_specials;
  struct command_info commands[2];
  struct command_info *saved_complete_cmd_info;
  struct obj_data wagon_load;
  time_t before;

  spec_mechanics_begin(&fixture);
  memset(&player_specials, 0, sizeof(player_specials));
  memset(commands, 0, sizeof(commands));
  clear_object(&wagon_load);
  GET_OBJ_RNUM(&wagon_load) = 0;

  REMOVE_BIT_AR(MOB_FLAGS(&fixture.target), MOB_ISNPC);
  fixture.target.player_specials = &player_specials;
  GET_IDNUM(&fixture.target) = 4242;

  GET_OBJ_TYPE(&fixture.worn) = ITEM_RESOURCE;
  GET_OBJ_COST(&fixture.worn) = 1500;
  fixture.worn.carried_by = &fixture.target;
  fixture.target.carrying = &fixture.worn;
  GET_OBJ_TYPE(&fixture.copy) = ITEM_WAGON;
  GET_OBJ_VAL(&fixture.copy, 3) = 4242;
  IN_ROOM(&fixture.copy) = 0;
  fixture.rooms[0].contents = &fixture.copy;
  GET_OBJ_TYPE(&wagon_load) = ITEM_RESOURCE;
  GET_OBJ_COST(&wagon_load) = 2500;
  wagon_load.in_obj = &fixture.copy;
  fixture.copy.contains = &wagon_load;

  CuAssertIntEquals(tc, 4000, rol_bandit_cargo_value(&fixture.target));
  GET_OBJ_VAL(&fixture.copy, 3) = 9999;
  CuAssertIntEquals(tc, 1500, rol_bandit_cargo_value(&fixture.target));
  CuAssertIntEquals(tc, 0, rol_bandit_cargo_value(NULL));

  CuAssertIntEquals(tc, ROL_BANDIT_DEMAND_PASS, rol_bandit_fee_gold(2099501, 999, 0, 10));
  CuAssertIntEquals(tc, ROL_BANDIT_DEMAND_PASS, rol_bandit_fee_gold(2999999, 5000, 0, 10));
  CuAssertIntEquals(tc, 50, rol_bandit_fee_gold(2099501, 5000, 0, 10));
  CuAssertIntEquals(tc, 10, rol_bandit_fee_gold(2099502, 5000, 0, 10));
  CuAssertIntEquals(tc, 20, rol_bandit_fee_gold(2099503, 5000, 0, 10));
  CuAssertIntEquals(tc, 50, rol_bandit_fee_gold(2099504, 5000, 0, 10));
  CuAssertIntEquals(tc, 73, rol_bandit_fee_gold(2099505, 5000, 0, 73));
  CuAssertIntEquals(tc, ROL_BANDIT_DEMAND_TAKE_WAGON, rol_bandit_fee_gold(2099505, 5000, 0, 0));
  CuAssertIntEquals(tc, 100, rol_bandit_fee_gold(2099506, 5000, 350, 73));
  CuAssertIntEquals(tc, 73, rol_bandit_fee_gold(2099506, 5000, 0, 73));
  CuAssertIntEquals(tc, 100, rol_bandit_fee_gold(2099506, 5000, 0, 0));
  CuAssertIntEquals(tc, ROL_BANDIT_DEMAND_ATTACK, rol_bandit_fee_gold(2099506, 5000, -350, 73));
  CuAssertIntEquals(tc, ROL_BANDIT_DEMAND_ATTACK, rol_bandit_fee_gold(2099507, 5000, 0, 73));

  fixture.mobile_indexes[0].vnum = 2099501;
  GET_MOB_RNUM(&fixture.actor) = 0;
  commands[1].command = "get";
  saved_complete_cmd_info = complete_cmd_info;
  complete_cmd_info = commands;
  CuAssertIntEquals(tc, TRUE, rol_bandit(&fixture.target, &fixture.actor, 1, ""));
  CuAssertTrue(tc, fixture.actor.mob_specials.rol_bandit_victim_id == 4242);
  CuAssertIntEquals(tc, 50, fixture.actor.mob_specials.rol_bandit_fee_gold);
  complete_cmd_info = saved_complete_cmd_info;
  fixture.actor.mob_specials.rol_bandit_victim_id = 0;
  fixture.actor.mob_specials.rol_bandit_fee_gold = 0;

  before = time(NULL);
  CuAssertIntEquals(tc, FALSE, rol_bandit(&fixture.actor, &fixture.actor, 0, ""));
  CuAssertTrue(tc, fixture.actor.mob_specials.rol_bandit_expire_at >=
                       before + (10 * SECS_PER_MUD_HOUR));
  fixture.actor.mob_specials.rol_bandit_expire_at = before - 1;
  CuAssertIntEquals(tc, TRUE, rol_bandit(&fixture.actor, &fixture.actor, 0, ""));
  CuAssertTrue(tc, fixture.actor.mob_specials.rol_bandit_expire_at == (time_t)-1);
  CuAssertTrue(tc, !MOB_FLAGGED(&fixture.actor, MOB_NOTDEADYET));

  fixture.target.carrying = NULL;
  fixture.worn.carried_by = NULL;
  fixture.copy.contains = NULL;
  wagon_load.in_obj = NULL;
  fixture.rooms[0].contents = NULL;
  fixture.target.player_specials = &dummy_mob;
  SET_BIT_AR(MOB_FLAGS(&fixture.target), MOB_ISNPC);
  spec_mechanics_end(&fixture);
}

void Test_spec_rol_shaman_totem_preserves_identity_gating_and_usage(CuTest *tc)
{
  struct spec_mechanics_fixture fixture;
  struct player_special_data player_specials;
  struct char_data *actor;
  struct command_info commands[2];
  struct command_info *saved_complete_cmd_info;
  time_t first_window;
  int restored_vnum;

  spec_mechanics_begin(&fixture);
  actor = &fixture.actor;
  memset(&player_specials, 0, sizeof(player_specials));
  memset(commands, 0, sizeof(commands));
  commands[1].command = "use";
  saved_complete_cmd_info = complete_cmd_info;
  complete_cmd_info = commands;
  REMOVE_BIT_AR(MOB_FLAGS(actor), MOB_ISNPC);
  actor->player_specials = &player_specials;

  CuAssertIntEquals(tc, 1, rol_shaman_totem_choice(2000716));
  CuAssertIntEquals(tc, 10, rol_shaman_totem_choice(2000725));
  CuAssertIntEquals(tc, 17, rol_shaman_totem_choice(2000732));
  CuAssertIntEquals(tc, 27, rol_shaman_totem_choice(2000742));
  CuAssertIntEquals(tc, 0, rol_shaman_totem_choice(9999999));
  CuAssertIntEquals(tc, 2000716, rol_shaman_totem_vnum(1));
  CuAssertIntEquals(tc, 2000742, rol_shaman_totem_vnum(27));
  CuAssertIntEquals(tc, -1, rol_shaman_totem_vnum(16));

  CuAssertTrue(tc, rol_shaman_totem_race_allowed(2000716, RACE_HUMAN));
  CuAssertTrue(tc, !rol_shaman_totem_race_allowed(2000716, RACE_DROW));
  CuAssertTrue(tc, rol_shaman_totem_race_allowed(2000732, RACE_DROW));
  CuAssertTrue(tc, !rol_shaman_totem_race_allowed(2000732, RACE_HUMAN));
  CuAssertTrue(tc, !rol_shaman_totem_race_allowed(9999999, RACE_HUMAN));

  fixture.object_indexes[0].vnum = 2000716;
  fixture.worn.worn_by = actor;
  fixture.worn.worn_on = WEAR_HOLD_1;
  GET_OBJ_BOUND_ID(&fixture.worn) = NOBODY;
  GET_IDNUM(actor) = 4242;
  GET_REAL_RACE(actor) = RACE_HUMAN;
  CLASS_LEVEL(actor, CLASS_CLERIC) = 1;
  CuAssertIntEquals(tc, TRUE, rol_shaman_totem(actor, &fixture.worn, 1, "totem"));
  CuAssertIntEquals(tc, 1, GET_ROL_TOTEM_CHOICE(actor));
  CuAssertIntEquals(tc, 4242, GET_OBJ_BOUND_ID(&fixture.worn));
  CuAssertIntEquals(tc, TRUE, rol_shaman_totem(actor, &fixture.worn, 1, "totem"));
  CuAssertIntEquals(tc, 0, GET_ROL_TOTEM_USES(actor));

  GET_REAL_WIS(actor) = 18;
  GET_WIS(actor) = 18;
  CLASS_LEVEL(actor, CLASS_CLERIC) = 20;
  CuAssertIntEquals(tc, 0, rol_shaman_totem_success_chance(actor));
  CLASS_LEVEL(actor, CLASS_CLERIC) = 21;
  CuAssertIntEquals(tc, 58, rol_shaman_totem_success_chance(actor));
  CLASS_LEVEL(actor, CLASS_CLERIC) = 30;
  CuAssertIntEquals(tc, 100, rol_shaman_totem_success_chance(actor));
  CuAssertIntEquals(tc, 0, rol_shaman_totem_success_chance(NULL));

  first_window = (time_t)100 * SECS_PER_MUD_DAY;
  CuAssertTrue(tc, rol_shaman_totem_consume_weekly_use(actor, first_window));
  CuAssertTrue(tc, rol_shaman_totem_consume_weekly_use(actor, first_window + 1));
  CuAssertTrue(tc, rol_shaman_totem_consume_weekly_use(actor, first_window + 2));
  CuAssertTrue(tc, !rol_shaman_totem_consume_weekly_use(actor, first_window + 3));
  CuAssertIntEquals(tc, 3, GET_ROL_TOTEM_USES(actor));
  CuAssertIntEquals(tc, 107, GET_ROL_TOTEM_WINDOW(actor));
  CuAssertTrue(tc, rol_shaman_totem_consume_weekly_use(actor, (time_t)107 * SECS_PER_MUD_DAY));
  CuAssertIntEquals(tc, 1, GET_ROL_TOTEM_USES(actor));
  CuAssertTrue(tc, !rol_shaman_totem_consume_weekly_use(NULL, first_window));

  CuAssertStrEquals(tc, "$n quickly fades away to the sound of a long mournful howl...",
                    rol_totem_spirit_death_message(2000716));
  CuAssertStrEquals(tc, "$n quickly fades away to the sound of a fading caw...",
                    rol_totem_spirit_death_message(2000742));
  CuAssertTrue(tc, rol_totem_spirit_death_message(9999999) == NULL);

  commands[1].command = "say";
  CLASS_LEVEL(actor, CLASS_CLERIC) = 21;
  GET_ROL_TOTEM_CHOICE(actor) = 1;
  GET_GOLD(&fixture.target) = 9999;
  restored_vnum = 0;
  CuAssertTrue(tc,
               !rol_totem_restorer_requirements(actor, GET_GOLD(&fixture.target), &restored_vnum));
  CuAssertIntEquals(tc, -1, restored_vnum);
  GET_GOLD(&fixture.target) = 10000;
  CuAssertTrue(tc,
               rol_totem_restorer_requirements(actor, GET_GOLD(&fixture.target), &restored_vnum));
  CuAssertIntEquals(tc, 2000716, restored_vnum);
  CuAssertIntEquals(tc, FALSE, rol_totem_restorer(actor, &fixture.target, 1, "elsewhere"));
  GET_GOLD(&fixture.target) = 9999;
  CuAssertIntEquals(tc, TRUE, rol_totem_restorer(actor, &fixture.target, 1, " spiritworld"));
  GET_ROL_TOTEM_CHOICE(actor) = 16;
  CuAssertTrue(tc, !rol_totem_restorer_requirements(actor, 10000, &restored_vnum));
  CuAssertTrue(tc, !rol_totem_restorer_requirements(NULL, 10000, &restored_vnum));

  actor->player_specials = &dummy_mob;
  SET_BIT_AR(MOB_FLAGS(actor), MOB_ISNPC);
  GET_MOB_RNUM(actor) = 0;
  fixture.mobile_indexes[0].vnum = 2000716;
  SET_BIT_AR(MOB_FLAGS(actor), MOB_ROL_TOTEM_SPIRIT);
  CuAssertTrue(tc, rol_handle_conjured_death(actor));

  complete_cmd_info = saved_complete_cmd_info;
  spec_mechanics_end(&fixture);
}

void Test_spec_rol_lich_rite_validates_irreversible_conversion_preflight(CuTest *tc)
{
  struct spec_mechanics_fixture fixture;
  struct player_special_data player_specials;
  struct command_info commands[2];
  struct command_info *saved_complete_cmd_info;
  struct char_data *actor;
  struct char_data *keeper;
  struct obj_data *first_offering;
  struct obj_data *second_offering;

  spec_mechanics_begin(&fixture);
  actor = &fixture.actor;
  keeper = &fixture.target;
  memset(&player_specials, 0, sizeof(player_specials));
  memset(commands, 0, sizeof(commands));
  commands[1].command = "say";
  saved_complete_cmd_info = complete_cmd_info;
  complete_cmd_info = commands;
  REMOVE_BIT_AR(MOB_FLAGS(actor), MOB_ISNPC);
  actor->player_specials = &player_specials;

  first_offering = &fixture.worn;
  second_offering = &fixture.copy;
  CuAssertIntEquals(tc, ROL_LICH_RITE_INVALID,
                    rol_lich_rite_requirements(NULL, keeper, &first_offering, &second_offering));
  CuAssertTrue(tc, first_offering == NULL);
  CuAssertTrue(tc, second_offering == NULL);
  CuAssertIntEquals(tc, ROL_LICH_RITE_WRONG_CLASS,
                    rol_lich_rite_requirements(actor, keeper, NULL, NULL));

  CLASS_LEVEL(actor, CLASS_NECROMANCER) = 1;
  CuAssertIntEquals(tc, ROL_LICH_RITE_INELIGIBLE_LEVEL,
                    rol_lich_rite_requirements(actor, keeper, NULL, NULL));
  GET_LEVEL(actor) = LVL_IMMORT - 1;
  actor->master = keeper;
  CuAssertIntEquals(tc, ROL_LICH_RITE_UNSAFE_FOLLOWERS,
                    rol_lich_rite_requirements(actor, keeper, NULL, NULL));
  actor->master = NULL;
  CuAssertIntEquals(tc, ROL_LICH_RITE_MISSING_OFFERINGS,
                    rol_lich_rite_requirements(actor, keeper, NULL, NULL));
  CuAssertIntEquals(tc, FALSE, rol_lich_rite(actor, keeper, 1, "something else"));
  CuAssertIntEquals(tc, FALSE, rol_lich_rite(actor, keeper, 1, "Immortality"));
  CuAssertIntEquals(tc, TRUE, rol_lich_rite(actor, keeper, 1, " immortality"));
  CuAssertTrue(tc, !MOB_FLAGGED(keeper, MOB_NOTDEADYET));

  fixture.object_indexes[0].vnum = 2089471;
  fixture.object_indexes[1].vnum = 2046999;
  GET_OBJ_RNUM(&fixture.worn) = 0;
  GET_OBJ_RNUM(&fixture.copy) = 1;
  GET_EQ(keeper, WEAR_HOLD_1) = &fixture.worn;
  fixture.worn.worn_by = keeper;
  fixture.worn.worn_on = WEAR_HOLD_1;
  keeper->carrying = &fixture.copy;
  fixture.copy.carried_by = keeper;

  first_offering = NULL;
  second_offering = NULL;
  CuAssertIntEquals(tc, ROL_LICH_RITE_READY,
                    rol_lich_rite_requirements(actor, keeper, &first_offering, &second_offering));
  CuAssertPtrEquals(tc, &fixture.worn, first_offering);
  CuAssertPtrEquals(tc, &fixture.copy, second_offering);

  GET_EQ(keeper, WEAR_HOLD_1) = NULL;
  fixture.worn.worn_by = NULL;
  keeper->carrying = NULL;
  fixture.copy.carried_by = NULL;
  actor->player_specials = &dummy_mob;
  SET_BIT_AR(MOB_FLAGS(actor), MOB_ISNPC);
  complete_cmd_info = saved_complete_cmd_info;
  spec_mechanics_end(&fixture);
}
