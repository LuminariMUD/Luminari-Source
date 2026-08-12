/**
 * @file test_spec_dispatch.c
 * Phase 01 gateway characterization: exact legacy translation, gateway-local
 * flow, and the iteration-successor caching corrections.
 */

#include "CuTest.h"

#include "../../src/conf.h"
#include "../../src/sysdep.h"
#include "../../src/structs.h"
#include "../../src/utils.h"

#include "../../src/comm.h"
#include "../../src/db.h"
#include "../../src/interpreter.h"
#include "../../src/spec/spec_dispatch.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SPEC_DISPATCH_ROOM_COUNT 2
#define SPEC_DISPATCH_MOB_COUNT 2
#define SPEC_DISPATCH_OBJ_COUNT 4
#define SPEC_DISPATCH_MAX_CALLS 16

void proc_update(void);

struct spec_dispatch_call
{
  struct char_data *actor;
  void *owner;
  int command;
  bool argument_is_null;
  char argument[MAX_INPUT_LENGTH];
};

struct spec_dispatch_fixture
{
  struct room_data *saved_world;
  room_rnum saved_top_of_world;
  struct index_data *saved_mob_index;
  mob_rnum saved_top_of_mobt;
  struct index_data *saved_obj_index;
  obj_rnum saved_top_of_objt;
  struct char_data *saved_character_list;
  struct obj_data *saved_object_list;

  struct room_data rooms[SPEC_DISPATCH_ROOM_COUNT];
  struct index_data mob_indexes[SPEC_DISPATCH_MOB_COUNT];
  struct index_data obj_indexes[SPEC_DISPATCH_OBJ_COUNT];
  struct char_data actor;
  struct char_data target;
  struct char_data mobiles[SPEC_DISPATCH_MOB_COUNT];
  struct obj_data objects[SPEC_DISPATCH_OBJ_COUNT];
  struct moving_room_data moving_room;

  struct spec_dispatch_call calls[SPEC_DISPATCH_MAX_CALLS];
  int returns[SPEC_DISPATCH_MAX_CALLS];
  int return_count;
  int call_count;

  /* Extraction simulation: unlink the owner from its list, then return zero. */
  bool unlink_owner_on_call;
};

static struct spec_dispatch_fixture *active_fixture;

static SPECIAL_DECL(spec_dispatch_record)
{
  struct spec_dispatch_call *call;
  int call_index;

  if (active_fixture == NULL)
    return 0;

  call_index = active_fixture->call_count;
  if (call_index < SPEC_DISPATCH_MAX_CALLS)
  {
    call = &active_fixture->calls[call_index];
    call->actor = ch;
    call->owner = me;
    call->command = cmd;
    call->argument_is_null = argument == NULL;
    call->argument[0] = '\0';
    if (argument != NULL)
      snprintf(call->argument, sizeof(call->argument), "%s", argument);
  }
  active_fixture->call_count++;

  if (active_fixture->unlink_owner_on_call && me != NULL)
  {
    /* Model what object extraction does to the entry being removed: its own
     * successor pointers are cleared. A caller that reads the successor after
     * the callback would stop traversing here. */
    struct obj_data *owner = (struct obj_data *)me;

    owner->next_content = NULL;
    owner->next = NULL;
  }

  if (call_index >= 0 && call_index < active_fixture->return_count &&
      call_index < SPEC_DISPATCH_MAX_CALLS)
    return active_fixture->returns[call_index];

  return 0;
}

static bool spec_dispatch_begin(struct spec_dispatch_fixture *fixture)
{
  int index;

  if (fixture == NULL || active_fixture != NULL)
    return false;

  memset(fixture, 0, sizeof(*fixture));
  fixture->saved_world = world;
  fixture->saved_top_of_world = top_of_world;
  fixture->saved_mob_index = mob_index;
  fixture->saved_top_of_mobt = top_of_mobt;
  fixture->saved_obj_index = obj_index;
  fixture->saved_top_of_objt = top_of_objt;
  fixture->saved_character_list = character_list;
  fixture->saved_object_list = object_list;

  fixture->rooms[0].number = 5100;
  fixture->rooms[1].number = 5200;
  for (index = 0; index < SPEC_DISPATCH_MOB_COUNT; index++)
  {
    fixture->mob_indexes[index].vnum = 5300 + index;
    fixture->mobiles[index].nr = index;
    SET_BIT_AR(MOB_FLAGS(&fixture->mobiles[index]), MOB_ISNPC);
  }
  for (index = 0; index < SPEC_DISPATCH_OBJ_COUNT; index++)
  {
    fixture->obj_indexes[index].vnum = 5400 + index;
    GET_OBJ_RNUM(&fixture->objects[index]) = index;
  }
  SET_BIT_AR(MOB_FLAGS(&fixture->target), MOB_ISNPC);
  fixture->target.nr = NOBODY;

  world = fixture->rooms;
  top_of_world = SPEC_DISPATCH_ROOM_COUNT - 1;
  mob_index = fixture->mob_indexes;
  top_of_mobt = SPEC_DISPATCH_MOB_COUNT - 1;
  obj_index = fixture->obj_indexes;
  top_of_objt = SPEC_DISPATCH_OBJ_COUNT - 1;
  character_list = NULL;
  object_list = NULL;
  active_fixture = fixture;
  return true;
}

static void spec_dispatch_end(struct spec_dispatch_fixture *fixture)
{
  if (fixture == NULL || active_fixture != fixture)
    return;

  active_fixture = NULL;
  world = fixture->saved_world;
  top_of_world = fixture->saved_top_of_world;
  mob_index = fixture->saved_mob_index;
  top_of_mobt = fixture->saved_top_of_mobt;
  obj_index = fixture->saved_obj_index;
  top_of_objt = fixture->saved_top_of_objt;
  character_list = fixture->saved_character_list;
  object_list = fixture->saved_object_list;
}

static bool spec_dispatch_call_is(struct spec_dispatch_fixture *fixture, int index,
                                  struct char_data *actor, void *owner, int command,
                                  const char *argument)
{
  struct spec_dispatch_call *call;

  if (index < 0 || index >= fixture->call_count || index >= SPEC_DISPATCH_MAX_CALLS)
    return false;

  call = &fixture->calls[index];
  if (call->actor != actor || call->owner != owner || call->command != command)
    return false;

  if (argument == NULL)
    return call->argument_is_null;

  return !call->argument_is_null && strcmp(call->argument, argument) == 0;
}

/* -------------------------------------------------------------------------- */

void Test_spec_dispatch_legacy_reports_flow_only_for_flow_bearing_events(CuTest *tc)
{
  struct spec_dispatch_fixture fixture;
  struct spec_event_context context;
  bool command_stops;
  bool activity_stops;
  bool death_stops;
  bool auto_pulse_stops;
  bool identify_continues;
  bool weapon_hit_continues;

  if (!spec_dispatch_begin(&fixture))
  {
    CuFail(tc, "unable to initialize dispatch fixture");
    return;
  }

  fixture.return_count = SPEC_DISPATCH_MAX_CALLS;
  fixture.returns[0] = 1;
  fixture.returns[1] = 1;
  fixture.returns[2] = 1;
  fixture.returns[3] = 1;
  fixture.returns[4] = 1;
  fixture.returns[5] = 1;

  memset(&context, 0, sizeof(context));
  context.owner_type = SPEC_OWNER_OBJECT;
  context.owner = &fixture.objects[0];
  context.actor = &fixture.actor;
  context.argument = "";

  context.event = SPEC_EVENT_COMMAND;
  (void)spec_dispatch_legacy(&context, spec_dispatch_record);
  command_stops = context.flow == SPEC_FLOW_STOP && context.legacy_return == 1;

  context.event = SPEC_EVENT_MOBILE_ACTIVITY;
  context.owner_type = SPEC_OWNER_MOBILE;
  context.owner = &fixture.mobiles[0];
  context.actor = &fixture.mobiles[0];
  (void)spec_dispatch_legacy(&context, spec_dispatch_record);
  activity_stops = context.flow == SPEC_FLOW_STOP;

  context.event = SPEC_EVENT_MOBILE_DEATH;
  context.actor = NULL;
  (void)spec_dispatch_legacy(&context, spec_dispatch_record);
  death_stops = context.flow == SPEC_FLOW_STOP;

  context.event = SPEC_EVENT_OBJECT_AUTO_PULSE;
  context.owner_type = SPEC_OWNER_OBJECT;
  context.owner = &fixture.objects[0];
  context.actor = NULL;
  (void)spec_dispatch_legacy(&context, spec_dispatch_record);
  auto_pulse_stops = context.flow == SPEC_FLOW_STOP;

  context.event = SPEC_EVENT_ITEM_IDENTIFY;
  context.actor = &fixture.actor;
  (void)spec_dispatch_legacy(&context, spec_dispatch_record);
  identify_continues = context.flow == SPEC_FLOW_CONTINUE && context.legacy_return == 1;

  context.event = SPEC_EVENT_WEAPON_HIT;
  context.target = &fixture.target;
  (void)spec_dispatch_legacy(&context, spec_dispatch_record);
  weapon_hit_continues = context.flow == SPEC_FLOW_CONTINUE && context.legacy_return == 1;

  spec_dispatch_end(&fixture);

  CuAssertTrue(tc, command_stops);
  CuAssertTrue(tc, activity_stops);
  CuAssertTrue(tc, death_stops);
  CuAssertTrue(tc, auto_pulse_stops);
  CuAssertTrue(tc, identify_continues);
  CuAssertTrue(tc, weapon_hit_continues);
}

void Test_spec_dispatch_mobile_death_skips_unadvertised_handlers(CuTest *tc)
{
  struct spec_dispatch_fixture fixture;
  bool skipped;

  if (!spec_dispatch_begin(&fixture))
  {
    CuFail(tc, "unable to initialize dispatch fixture");
    return;
  }

  fixture.mob_indexes[0].func = spec_dispatch_record;
  fixture.return_count = 1;
  fixture.returns[0] = TRUE;
  skipped = spec_gateway_mobile_death(&fixture.mobiles[0], &fixture.actor) == FALSE &&
            fixture.call_count == 0;

  spec_dispatch_end(&fixture);

  CuAssertTrue(tc, skipped);
}

void Test_spec_dispatch_legacy_tolerates_missing_handler_context_and_owner(CuTest *tc)
{
  struct spec_dispatch_fixture fixture;
  struct spec_event_context context;
  bool null_handler_safe;
  bool null_owner_safe;
  bool null_context_safe;

  if (!spec_dispatch_begin(&fixture))
  {
    CuFail(tc, "unable to initialize dispatch fixture");
    return;
  }

  memset(&context, 0, sizeof(context));
  context.owner_type = SPEC_OWNER_OBJECT;
  context.event = SPEC_EVENT_COMMAND;
  context.owner = &fixture.objects[0];
  context.argument = "";
  context.flow = SPEC_FLOW_STOP;
  null_handler_safe = spec_dispatch_legacy(&context, NULL) == 0 &&
                      context.flow == SPEC_FLOW_CONTINUE && fixture.call_count == 0;

  context.owner = NULL;
  null_owner_safe =
      spec_dispatch_legacy(&context, spec_dispatch_record) == 0 && fixture.call_count == 0;

  null_context_safe =
      spec_dispatch_legacy(NULL, spec_dispatch_record) == 0 && fixture.call_count == 0;

  spec_dispatch_end(&fixture);

  CuAssertTrue(tc, null_handler_safe);
  CuAssertTrue(tc, null_owner_safe);
  CuAssertTrue(tc, null_context_safe);
}

void Test_spec_dispatch_legacy_rejects_typed_context_mismatches(CuTest *tc)
{
  struct spec_dispatch_fixture fixture;
  struct spec_event_context context;
  bool invalid_owner_type_rejected;
  bool owner_event_mismatch_rejected;
  bool missing_target_rejected;

  if (!spec_dispatch_begin(&fixture))
  {
    CuFail(tc, "unable to initialize dispatch fixture");
    return;
  }

  memset(&context, 0, sizeof(context));
  context.owner_type = SPEC_OWNER_OBJECT | SPEC_OWNER_MOBILE;
  context.event = SPEC_EVENT_COMMAND;
  context.owner = &fixture.objects[0];
  context.actor = &fixture.actor;
  context.argument = "";
  invalid_owner_type_rejected = spec_dispatch_legacy(&context, spec_dispatch_record) == 0 &&
                                context.flow == SPEC_FLOW_CONTINUE && fixture.call_count == 0;

  context.owner_type = SPEC_OWNER_OBJECT;
  context.event = SPEC_EVENT_MOBILE_ACTIVITY;
  owner_event_mismatch_rejected =
      spec_dispatch_legacy(&context, spec_dispatch_record) == 0 && fixture.call_count == 0;

  context.event = SPEC_EVENT_WEAPON_HIT;
  context.target = NULL;
  missing_target_rejected =
      spec_dispatch_legacy(&context, spec_dispatch_record) == 0 && fixture.call_count == 0;

  spec_dispatch_end(&fixture);

  CuAssertTrue(tc, invalid_owner_type_rejected);
  CuAssertTrue(tc, owner_event_mismatch_rejected);
  CuAssertTrue(tc, missing_target_rejected);
}

void Test_spec_dispatch_command_gateways_translate_exactly(CuTest *tc)
{
  struct spec_dispatch_fixture fixture;
  char argument[] = " command payload";
  bool room_exact;
  bool object_exact;
  bool mobile_exact;
  bool absent_owners_skipped;

  if (!spec_dispatch_begin(&fixture))
  {
    CuFail(tc, "unable to initialize dispatch fixture");
    return;
  }

  fixture.rooms[0].func = spec_dispatch_record;
  fixture.obj_indexes[0].func = spec_dispatch_record;
  fixture.mob_indexes[0].func = spec_dispatch_record;

  room_exact = spec_gateway_command_room(&fixture.actor, &fixture.rooms[0], 41, argument) == 0 &&
               spec_dispatch_call_is(&fixture, 0, &fixture.actor, &fixture.rooms[0], 41, argument);

  object_exact =
      spec_gateway_command_object(&fixture.actor, &fixture.objects[0], 42, argument) == 0 &&
      spec_dispatch_call_is(&fixture, 1, &fixture.actor, &fixture.objects[0], 42, argument);

  mobile_exact =
      spec_gateway_command_mobile(&fixture.actor, &fixture.mobiles[0], 43, argument) == 0 &&
      spec_dispatch_call_is(&fixture, 2, &fixture.actor, &fixture.mobiles[0], 43, argument);

  absent_owners_skipped =
      spec_gateway_command_room(&fixture.actor, NULL, 1, argument) == 0 &&
      spec_gateway_command_object(&fixture.actor, NULL, 1, argument) == 0 &&
      spec_gateway_command_mobile(&fixture.actor, NULL, 1, argument) == 0 &&
      spec_gateway_command_room(&fixture.actor, &fixture.rooms[1], 1, argument) == 0 &&
      fixture.call_count == 3;

  spec_dispatch_end(&fixture);

  CuAssertTrue(tc, room_exact);
  CuAssertTrue(tc, object_exact);
  CuAssertTrue(tc, mobile_exact);
  CuAssertTrue(tc, absent_owners_skipped);
}

void Test_spec_dispatch_command_gateways_normalize_nonzero_to_one(CuTest *tc)
{
  struct spec_dispatch_fixture fixture;
  char argument[] = "";
  bool normalized;

  if (!spec_dispatch_begin(&fixture))
  {
    CuFail(tc, "unable to initialize dispatch fixture");
    return;
  }

  fixture.obj_indexes[0].func = spec_dispatch_record;
  fixture.return_count = 2;
  fixture.returns[0] = 7;
  fixture.returns[1] = -3;

  normalized = spec_gateway_command_object(&fixture.actor, &fixture.objects[0], 0, argument) == 1 &&
               spec_gateway_command_object(&fixture.actor, &fixture.objects[0], 0, argument) == 1;

  spec_dispatch_end(&fixture);

  CuAssertTrue(tc, normalized);
}

void Test_spec_dispatch_internal_events_use_exact_legacy_tokens(CuTest *tc)
{
  struct spec_dispatch_fixture fixture;
  bool activity_token;
  bool combat_turn_token;
  bool identify_token;
  bool weapon_hit_token;
  bool defense_token;
  bool maneuver_token;
  bool charge_token;

  if (!spec_dispatch_begin(&fixture))
  {
    CuFail(tc, "unable to initialize dispatch fixture");
    return;
  }

  fixture.obj_indexes[0].func = spec_dispatch_record;
  fixture.mob_indexes[0].func = spec_dispatch_record;

  activity_token =
      spec_gateway_mobile_activity(&fixture.mobiles[0], spec_dispatch_record) == 0 &&
      spec_dispatch_call_is(&fixture, 0, &fixture.mobiles[0], &fixture.mobiles[0], 0, "");

  spec_gateway_mobile_combat_turn(&fixture.mobiles[0]);
  combat_turn_token =
      spec_dispatch_call_is(&fixture, 1, &fixture.mobiles[0], &fixture.mobiles[0], 0, "");

  spec_gateway_item_identify(&fixture.actor, &fixture.objects[0]);
  identify_token =
      spec_dispatch_call_is(&fixture, 2, &fixture.actor, &fixture.objects[0], 0, "identify");

  weapon_hit_token =
      spec_gateway_weapon_hit(&fixture.actor, &fixture.objects[0], &fixture.target, 17,
                              ATTACK_TYPE_PRIMARY, false, "hit") == 0 &&
      spec_dispatch_call_is(&fixture, 3, &fixture.actor, &fixture.objects[0], 0, "hit");

  spec_gateway_defense_reaction(&fixture.actor, &fixture.objects[0], &fixture.target,
                                "shieldblock");
  defense_token =
      spec_dispatch_call_is(&fixture, 4, &fixture.actor, &fixture.objects[0], 0, "shieldblock");

  spec_gateway_combat_maneuver(&fixture.actor, &fixture.objects[0], &fixture.target, "shieldpunch");
  maneuver_token =
      spec_dispatch_call_is(&fixture, 5, &fixture.actor, &fixture.objects[0], 0, "shieldpunch");

  spec_gateway_mount_charge(&fixture.actor, &fixture.mobiles[0], &fixture.target);
  charge_token =
      spec_dispatch_call_is(&fixture, 6, &fixture.actor, &fixture.mobiles[0], 0, "charge");

  spec_dispatch_end(&fixture);

  CuAssertTrue(tc, activity_token);
  CuAssertTrue(tc, combat_turn_token);
  CuAssertTrue(tc, identify_token);
  CuAssertTrue(tc, weapon_hit_token);
  CuAssertTrue(tc, defense_token);
  CuAssertTrue(tc, maneuver_token);
  CuAssertTrue(tc, charge_token);
}

void Test_spec_dispatch_weapon_hit_returns_raw_legacy_value(CuTest *tc)
{
  struct spec_dispatch_fixture fixture;
  bool raw_value_preserved;
  bool missing_weapon_returns_zero;

  if (!spec_dispatch_begin(&fixture))
  {
    CuFail(tc, "unable to initialize dispatch fixture");
    return;
  }

  fixture.obj_indexes[0].func = spec_dispatch_record;
  fixture.return_count = 1;
  fixture.returns[0] = 9;

  raw_value_preserved =
      spec_gateway_weapon_hit(&fixture.actor, &fixture.objects[0], &fixture.target, 17,
                              ATTACK_TYPE_PRIMARY, false, "hit") == 9;
  missing_weapon_returns_zero =
      spec_gateway_weapon_hit(&fixture.actor, NULL, &fixture.target, 17, ATTACK_TYPE_PRIMARY, false,
                              "hit") == 0 &&
      spec_gateway_weapon_hit(&fixture.actor, &fixture.objects[1], &fixture.target, 17,
                              ATTACK_TYPE_PRIMARY, false, "hit") == 0;

  spec_dispatch_end(&fixture);

  CuAssertTrue(tc, raw_value_preserved);
  CuAssertTrue(tc, missing_weapon_returns_zero);
}

void Test_spec_dispatch_auto_pulse_runs_carried_fallback_only_after_zero(CuTest *tc)
{
  struct spec_dispatch_fixture fixture;
  bool fallback_ran;
  bool fallback_skipped;

  if (!spec_dispatch_begin(&fixture))
  {
    CuFail(tc, "unable to initialize dispatch fixture");
    return;
  }

  fixture.obj_indexes[0].func = spec_dispatch_record;
  fixture.objects[0].worn_by = NULL;
  fixture.objects[0].carried_by = &fixture.actor;

  spec_gateway_object_auto_pulse(&fixture.objects[0]);
  fallback_ran = fixture.call_count == 2 &&
                 spec_dispatch_call_is(&fixture, 0, NULL, &fixture.objects[0], 0, "") &&
                 spec_dispatch_call_is(&fixture, 1, &fixture.actor, &fixture.objects[0], 0, "");

  fixture.call_count = 0;
  fixture.return_count = 1;
  fixture.returns[0] = 1;
  fixture.objects[0].worn_by = &fixture.target;
  spec_gateway_object_auto_pulse(&fixture.objects[0]);
  fallback_skipped = fixture.call_count == 1 && spec_dispatch_call_is(&fixture, 0, &fixture.target,
                                                                      &fixture.objects[0], 0, "");

  spec_dispatch_end(&fixture);

  CuAssertTrue(tc, fallback_ran);
  CuAssertTrue(tc, fallback_skipped);
}

void Test_spec_dispatch_moving_room_preserves_null_actor_and_argument(CuTest *tc)
{
  struct spec_dispatch_fixture fixture;
  bool relocation_exact;
  bool absent_room_skipped;

  if (!spec_dispatch_begin(&fixture))
  {
    CuFail(tc, "unable to initialize dispatch fixture");
    return;
  }

  fixture.rooms[0].func = spec_dispatch_record;
  fixture.moving_room.destination = 5200;

  spec_gateway_moving_room(&fixture.rooms[0], &fixture.moving_room, 5200);
  relocation_exact = fixture.call_count == 1 &&
                     spec_dispatch_call_is(&fixture, 0, NULL, &fixture.moving_room, 0, NULL);

  spec_gateway_moving_room(&fixture.rooms[1], &fixture.moving_room, 5200);
  spec_gateway_moving_room(NULL, &fixture.moving_room, 5200);
  absent_room_skipped = fixture.call_count == 1;

  spec_dispatch_end(&fixture);

  CuAssertTrue(tc, relocation_exact);
  CuAssertTrue(tc, absent_room_skipped);
}

void Test_spec_dispatch_secondary_gateways_forward_context_unchanged(CuTest *tc)
{
  struct spec_dispatch_fixture fixture;
  char argument[] = " list all";
  bool shop_forwarded;
  bool quest_forwarded;
  bool nonzero_propagates;
  bool absent_secondary_safe;

  if (!spec_dispatch_begin(&fixture))
  {
    CuFail(tc, "unable to initialize dispatch fixture");
    return;
  }

  shop_forwarded =
      spec_gateway_shop_secondary(spec_dispatch_record, &fixture.actor, &fixture.mobiles[0], 55,
                                  argument) == 0 &&
      spec_dispatch_call_is(&fixture, 0, &fixture.actor, &fixture.mobiles[0], 55, argument);

  quest_forwarded =
      spec_gateway_quest_secondary(spec_dispatch_record, &fixture.actor, &fixture.mobiles[1], 56,
                                   argument) == 0 &&
      spec_dispatch_call_is(&fixture, 1, &fixture.actor, &fixture.mobiles[1], 56, argument);

  fixture.return_count = 3;
  fixture.returns[2] = 4;
  nonzero_propagates = spec_gateway_quest_secondary(spec_dispatch_record, &fixture.actor,
                                                    &fixture.mobiles[1], 57, argument) == 1;

  absent_secondary_safe =
      spec_gateway_shop_secondary(NULL, &fixture.actor, &fixture.mobiles[0], 1, argument) == 0 &&
      spec_gateway_quest_secondary(NULL, &fixture.actor, &fixture.mobiles[0], 1, argument) == 0 &&
      spec_gateway_shop_secondary(spec_dispatch_record, &fixture.actor, NULL, 1, argument) == 0 &&
      fixture.call_count == 3;

  spec_dispatch_end(&fixture);

  CuAssertTrue(tc, shop_forwarded);
  CuAssertTrue(tc, quest_forwarded);
  CuAssertTrue(tc, nonzero_propagates);
  CuAssertTrue(tc, absent_secondary_safe);
}

void Test_spec_command_traversal_caches_successor_before_callback(CuTest *tc)
{
  struct spec_dispatch_fixture fixture;
  char argument[] = " extraction payload";
  bool inventory_continues;
  bool room_contents_continue;
  int handled;

  if (!spec_dispatch_begin(&fixture))
  {
    CuFail(tc, "unable to initialize dispatch fixture");
    return;
  }

  fixture.obj_indexes[0].func = spec_dispatch_record;
  fixture.obj_indexes[1].func = spec_dispatch_record;
  fixture.obj_indexes[2].func = spec_dispatch_record;
  fixture.obj_indexes[3].func = spec_dispatch_record;
  fixture.unlink_owner_on_call = true;

  IN_ROOM(&fixture.actor) = 0;
  fixture.actor.carrying = &fixture.objects[0];
  fixture.objects[0].next_content = &fixture.objects[1];
  fixture.rooms[0].contents = &fixture.objects[2];
  fixture.objects[2].next_content = &fixture.objects[3];

  handled = special(&fixture.actor, 11, argument);

  inventory_continues =
      spec_dispatch_call_is(&fixture, 0, &fixture.actor, &fixture.objects[0], 11, argument) &&
      spec_dispatch_call_is(&fixture, 1, &fixture.actor, &fixture.objects[1], 11, argument);
  room_contents_continue =
      spec_dispatch_call_is(&fixture, 2, &fixture.actor, &fixture.objects[2], 11, argument) &&
      spec_dispatch_call_is(&fixture, 3, &fixture.actor, &fixture.objects[3], 11, argument) &&
      fixture.call_count == 4;

  spec_dispatch_end(&fixture);

  CuAssertIntEquals(tc, 0, handled);
  CuAssertTrue(tc, inventory_continues);
  CuAssertTrue(tc, room_contents_continue);
}

void Test_spec_proc_update_caches_successor_before_callback(CuTest *tc)
{
  struct spec_dispatch_fixture fixture;
  bool both_objects_pulsed;

  if (!spec_dispatch_begin(&fixture))
  {
    CuFail(tc, "unable to initialize dispatch fixture");
    return;
  }

  fixture.obj_indexes[0].func = spec_dispatch_record;
  fixture.obj_indexes[1].func = spec_dispatch_record;
  fixture.unlink_owner_on_call = true;

  GET_OBJ_TYPE(&fixture.objects[0]) = ITEM_OTHER;
  GET_OBJ_TYPE(&fixture.objects[1]) = ITEM_OTHER;
  SET_BIT_AR(GET_OBJ_EXTRA(&fixture.objects[0]), ITEM_AUTOPROC);
  SET_BIT_AR(GET_OBJ_EXTRA(&fixture.objects[1]), ITEM_AUTOPROC);
  fixture.objects[0].next = &fixture.objects[1];
  object_list = &fixture.objects[0];

  proc_update();
  /* Two unowned objects, each invoked exactly once with a null actor. */
  both_objects_pulsed = fixture.call_count == 2 &&
                        spec_dispatch_call_is(&fixture, 0, NULL, &fixture.objects[0], 0, "") &&
                        spec_dispatch_call_is(&fixture, 1, NULL, &fixture.objects[1], 0, "");

  object_list = NULL;
  spec_dispatch_end(&fixture);

  CuAssertTrue(tc, both_objects_pulsed);
}

void Test_spec_invalidate_name_reports_stable_labels(CuTest *tc)
{
  CuAssertStrEquals(tc, "owner", spec_invalidate_name(SPEC_INVALIDATE_OWNER));
  CuAssertStrEquals(tc, "actor", spec_invalidate_name(SPEC_INVALIDATE_ACTOR));
  CuAssertStrEquals(tc, "target", spec_invalidate_name(SPEC_INVALIDATE_TARGET));
  CuAssertStrEquals(tc, "unknown", spec_invalidate_name(SPEC_INVALIDATE_NONE));
  CuAssertStrEquals(tc, "unknown",
                    spec_invalidate_name(SPEC_INVALIDATE_OWNER | SPEC_INVALIDATE_ACTOR));
}
