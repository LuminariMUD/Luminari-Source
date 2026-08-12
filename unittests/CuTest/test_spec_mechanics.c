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
#include "../../src/spec/spec_combat.h"
#include "../../src/spec/spec_context.h"
#include "../../src/spec/spec_cooldown.h"
#include "../../src/spec/spec_dispatch.h"
#include "../../src/spec/spec_effects.h"
#include "../../src/spec/spec_objects.h"
#include "../../src/spec/spec_phrase.h"
#include "../../src/spec/spec_rol_conversion.h"
#include "../../src/spec/spec_rol_totem.h"

#include <limits.h>
#include <string.h>

struct spec_mechanics_fixture
{
  struct room_data rooms[2];
  struct zone_data zones[1];
  struct index_data mobile_indexes[2];
  struct index_data object_indexes[1];
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

  world = fixture->rooms;
  zone_table = fixture->zones;
  mob_index = fixture->mobile_indexes;
  obj_index = fixture->object_indexes;
  top_of_world = 1;
  top_of_zone_table = 0;
  top_of_mobt = 1;
  top_of_objt = 0;

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

  CuAssertTrue(tc, rol_guild_guard_protects(2008200));
  CuAssertTrue(tc, rol_guild_guard_protects(2050624));
  CuAssertTrue(tc, !rol_guild_guard_protects(2004128));
  CuAssertTrue(tc, !rol_guild_guard_protects(9999999));

  target->player_specials = &dummy_mob;
  SET_BIT_AR(MOB_FLAGS(target), MOB_ISNPC);
  spec_mechanics_end(&fixture);
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

  CLASS_LEVEL(target, CLASS_WIZARD) = 1;
  CuAssertTrue(tc, rol_class_guild_allows(target, ROL_GUILD_FAMILY_MAGE));
  CuAssertTrue(tc, !rol_class_guild_allows(target, ROL_GUILD_FAMILY_CLERIC));
  CLASS_LEVEL(target, CLASS_BARD) = 1;
  CuAssertTrue(tc, rol_class_guild_allows(target, ROL_GUILD_FAMILY_THIEF));
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
  CuAssertStrEquals(tc, "A fire mephit blinks out of existence.",
                    rol_conversion_death_message(2000907));
  CuAssertStrEquals(
      tc, "With a splash, the water elemental crashes to the ground leaving only a puddle behind.",
      rol_conversion_death_message(2003053));
  CuAssertTrue(tc, rol_conversion_death_message(9999999) == NULL);

  fixture.mobile_indexes[0].vnum = 2000907;
  GET_MOB_RNUM(&fixture.actor) = 0;
  CuAssertTrue(tc, rol_handle_conjured_death(&fixture.actor));

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

  CuAssertIntEquals(tc, 95, (int)rol_source_periodic_profile_count());
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
  CuAssertTrue(tc, rol_source_periodic_profile_bounds(2001230, NULL, NULL, &requires_awake, NULL));
  CuAssertTrue(tc, !requires_awake);
  CuAssertTrue(tc, !rol_source_periodic_profile_bounds(9999999, NULL, NULL, NULL, NULL));

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

void Test_spec_rol_state_periodic_profiles_preserve_idle_and_fighting_tables(CuTest *tc)
{
  bool hide;
  bool speech;
  int dice_count;
  int dice_sides;

  CuAssertIntEquals(tc, 26, (int)rol_state_periodic_profile_count());

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
  CuAssertTrue(tc, rol_state_periodic_outcome_action(2003039, false, 4, 2, NULL, NULL) == NULL);
  CuAssertTrue(tc, rol_state_periodic_outcome_action(9999999, false, 4, 0, NULL, NULL) == NULL);
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

  actor->player_specials = &dummy_mob;
  SET_BIT_AR(MOB_FLAGS(actor), MOB_ISNPC);
  GET_MOB_RNUM(actor) = 0;
  fixture.mobile_indexes[0].vnum = 2000716;
  SET_BIT_AR(MOB_FLAGS(actor), MOB_ROL_TOTEM_SPIRIT);
  CuAssertTrue(tc, rol_handle_conjured_death(actor));

  complete_cmd_info = saved_complete_cmd_info;
  spec_mechanics_end(&fixture);
}
