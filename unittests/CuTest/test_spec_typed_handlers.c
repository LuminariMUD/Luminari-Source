/**
 * @file test_spec_typed_handlers.c
 * Phase 05 typed-dispatch and converted-handler characterization.
 */

#include "CuTest.h"

#include "../../src/conf.h"
#include "../../src/sysdep.h"
#include "../../src/structs.h"
#include "../../src/utils.h"

#include "../../src/character/vampire_cloak.h"
#include "../../src/comm.h"
#include "../../src/db.h"
#include "../../src/interpreter.h"
#include "../../src/magic/spells.h"
#include "../../src/obj/vendor.h"
#include "../../src/spec/spec_binding.h"
#include "../../src/spec/spec_dispatch.h"
#include "../../src/spec/spec_registry.h"
#include "../../src/spec/spec_rol_avernus.h"
#include "../../src/spec/spec_rol_conversion.h"
#include "../../src/spec/spec_rol_darkhold.h"
#include "../../src/spec/spec_rol_deaths_head.h"
#include "../../src/spec/spec_rol_drow.h"
#include "../../src/spec/spec_rol_tarrasque.h"
#include "../../src/spec/spec_rol_utility_objects.h"

#include <string.h>

struct spec_typed_fixture
{
  struct room_data *saved_world;
  struct index_data *saved_obj_index;
  struct command_info *saved_complete_cmd_info;
  room_rnum saved_top_of_world;
  obj_rnum saved_top_of_objt;

  struct room_data room;
  struct index_data object_index;
  struct command_info commands[5];
  struct char_data actor;
  struct player_special_data player_specials;
  struct descriptor_data descriptor;
  struct obj_data worn_cloak;
  struct obj_data carried_cloak;
};

static int spec_typed_test_calls;
static struct char_data *spec_typed_test_target;
static bool spec_typed_test_request_stop;

static SPECIAL_DECL(spec_typed_test_adapter)
{
  (void)ch;
  (void)me;
  (void)cmd;
  (void)argument;
  return -1;
}

static int spec_typed_test_handler(struct spec_event_context *context)
{
  spec_typed_test_calls++;
  spec_typed_test_target = context->target;
  context->invalidation = SPEC_INVALIDATE_TARGET;
  if (spec_typed_test_request_stop)
    context->flow = SPEC_FLOW_STOP;
  return TRUE;
}

static void spec_typed_clear_output(struct spec_typed_fixture *fixture)
{
  fixture->descriptor.small_outbuf[0] = '\0';
  fixture->descriptor.output = fixture->descriptor.small_outbuf;
  fixture->descriptor.bufptr = 0;
  fixture->descriptor.bufspace = SMALL_BUFSIZE - 1;
}

static bool spec_typed_begin(struct spec_typed_fixture *fixture)
{
  memset(fixture, 0, sizeof(*fixture));

  fixture->saved_world = world;
  fixture->saved_obj_index = obj_index;
  fixture->saved_complete_cmd_info = complete_cmd_info;
  fixture->saved_top_of_world = top_of_world;
  fixture->saved_top_of_objt = top_of_objt;

  fixture->room.number = 6400;
  world = &fixture->room;
  top_of_world = 0;

  fixture->object_index.vnum = VAMPIRE_CLOAK_OBJ_VNUM;
  fixture->object_index.func = vampire_cloak;
  obj_index = &fixture->object_index;
  top_of_objt = 0;

  fixture->commands[0].command = "reserved";
  fixture->commands[1].command = "setcloak";
  fixture->commands[2].command = "balance";
  fixture->commands[3].command = "deposit";
  fixture->commands[4].command = "withdraw";
  complete_cmd_info = fixture->commands;

  clear_char(&fixture->actor);
  fixture->actor.player_specials = &fixture->player_specials;
  fixture->actor.player.name = "typed procedure actor";
  fixture->actor.player.short_descr = "typed procedure actor";
  GET_LEVEL(&fixture->actor) = 20;
  GET_POS(&fixture->actor) = POS_STANDING;
  IN_ROOM(&fixture->actor) = 0;

  memset(&fixture->descriptor, 0, sizeof(fixture->descriptor));
  fixture->descriptor.character = &fixture->actor;
  fixture->descriptor.pProtocol = ProtocolCreate();
  if (fixture->descriptor.pProtocol == NULL)
  {
    world = fixture->saved_world;
    obj_index = fixture->saved_obj_index;
    complete_cmd_info = fixture->saved_complete_cmd_info;
    top_of_world = fixture->saved_top_of_world;
    top_of_objt = fixture->saved_top_of_objt;
    return false;
  }
  fixture->actor.desc = &fixture->descriptor;
  spec_typed_clear_output(fixture);

  clear_object(&fixture->worn_cloak);
  clear_object(&fixture->carried_cloak);
  GET_OBJ_RNUM(&fixture->worn_cloak) = 0;
  GET_OBJ_RNUM(&fixture->carried_cloak) = 0;
  fixture->worn_cloak.worn_by = &fixture->actor;
  fixture->worn_cloak.worn_on = WEAR_ABOUT;
  GET_EQ(&fixture->actor, WEAR_ABOUT) = &fixture->worn_cloak;
  fixture->carried_cloak.carried_by = &fixture->actor;

  return true;
}

static void spec_typed_end(struct spec_typed_fixture *fixture)
{
  if (fixture->descriptor.pProtocol != NULL)
    ProtocolDestroy(fixture->descriptor.pProtocol);
  if (fixture->descriptor.large_outbuf != NULL)
  {
    free(fixture->descriptor.large_outbuf->text);
    free(fixture->descriptor.large_outbuf);
  }

  fixture->actor.desc = NULL;
  GET_EQ(&fixture->actor, WEAR_ABOUT) = NULL;
  fixture->worn_cloak.worn_by = NULL;
  fixture->carried_cloak.carried_by = NULL;

  world = fixture->saved_world;
  obj_index = fixture->saved_obj_index;
  complete_cmd_info = fixture->saved_complete_cmd_info;
  top_of_world = fixture->saved_top_of_world;
  top_of_objt = fixture->saved_top_of_objt;
}

void Test_spec_typed_registry_preserves_callback_and_persisted_identities(CuTest *tc)
{
  const struct spec_definition *bank_definition;
  const struct spec_definition *cloak_definition;
  const struct spec_definition *guild_guard_definition;
  const struct spec_definition *command_sentinel_definition;
  const struct spec_definition *toll_keeper_definition;
  const struct spec_definition *banana_definition;
  const struct spec_definition *darkhold_object_definition;
  const struct spec_definition *deaths_head_definition;
  const struct spec_definition *drow_equipment_definition;
  const struct spec_definition *weapon_definition;
  const struct spec_definition *avernus_object_definition;
  const struct spec_definition *avernus_garden_definition;
  const struct spec_definition *monster_combat_definition;
  const struct spec_definition *utility_object_definition;
  const struct spec_definition *utility_room_definition;
  const struct spec_definition *tarrasque_definition;
  const struct spec_definition *composite_mobile_definition;
  const struct spec_definition *composite_object_definition;
  struct spec_binding *binding;
  char error[256];

  bank_definition = spec_registry_find_by_name("Bank");
  cloak_definition = spec_registry_find_by_name("Vampire Cloak");
  guild_guard_definition = spec_registry_find_by_name("RoL Guild Guard");
  command_sentinel_definition = spec_registry_find_by_name("RoL Command Sentinel");
  toll_keeper_definition = spec_registry_find_by_name("RoL Toll Keeper");
  banana_definition = spec_registry_find_by_name("RoL Banana");
  darkhold_object_definition = spec_registry_find_by_name("RoL Darkhold Object");
  deaths_head_definition = spec_registry_find_by_name("RoL Death's Head");
  drow_equipment_definition = spec_registry_find_by_name("RoL Drow Equipment");
  weapon_definition = spec_registry_find_by_name("RoL Weapon Proc");
  avernus_object_definition = spec_registry_find_by_name("RoL Avernus Object");
  avernus_garden_definition = spec_registry_find_by_name("RoL Avernus Garden");
  monster_combat_definition = spec_registry_find_by_name("RoL Monster Combat");
  utility_object_definition = spec_registry_find_by_name("RoL Utility Object");
  utility_room_definition = spec_registry_find_by_name("RoL Utility Room");
  tarrasque_definition = spec_registry_find_by_name("RoL Tarrasque Encounter");
  composite_mobile_definition = spec_registry_find_by_name("RoL Composite Mobile");
  composite_object_definition = spec_registry_find_by_name("RoL Composite Object");
  CuAssertPtrNotNull(tc, bank_definition);
  CuAssertPtrNotNull(tc, cloak_definition);
  CuAssertPtrNotNull(tc, guild_guard_definition);
  CuAssertPtrNotNull(tc, command_sentinel_definition);
  CuAssertPtrNotNull(tc, toll_keeper_definition);
  CuAssertPtrNotNull(tc, banana_definition);
  CuAssertPtrNotNull(tc, darkhold_object_definition);
  CuAssertPtrNotNull(tc, deaths_head_definition);
  CuAssertPtrNotNull(tc, drow_equipment_definition);
  CuAssertPtrNotNull(tc, weapon_definition);
  CuAssertPtrNotNull(tc, avernus_object_definition);
  CuAssertPtrNotNull(tc, avernus_garden_definition);
  CuAssertPtrNotNull(tc, monster_combat_definition);
  CuAssertPtrNotNull(tc, utility_object_definition);
  CuAssertPtrNotNull(tc, utility_room_definition);
  CuAssertPtrNotNull(tc, tarrasque_definition);
  CuAssertPtrNotNull(tc, composite_mobile_definition);
  CuAssertPtrNotNull(tc, composite_object_definition);
  if (bank_definition == NULL || cloak_definition == NULL || guild_guard_definition == NULL ||
      command_sentinel_definition == NULL || toll_keeper_definition == NULL ||
      banana_definition == NULL || darkhold_object_definition == NULL ||
      deaths_head_definition == NULL || drow_equipment_definition == NULL ||
      weapon_definition == NULL || avernus_object_definition == NULL ||
      avernus_garden_definition == NULL || monster_combat_definition == NULL ||
      utility_object_definition == NULL || utility_room_definition == NULL ||
      tarrasque_definition == NULL || composite_mobile_definition == NULL ||
      composite_object_definition == NULL)
    return;

  CuAssertIntEquals(tc, 21, (int)spec_registry_typed_count());
  CuAssertIntEquals(tc, 98, (int)spec_registry_legacy_count());
  CuAssertPtrEquals(tc, NULL, (void *)bank_definition->legacy_handler);
  CuAssertPtrEquals(tc, NULL, (void *)cloak_definition->legacy_handler);
  CuAssertPtrNotNull(tc, (void *)bank_definition->typed_handler);
  CuAssertPtrNotNull(tc, (void *)cloak_definition->typed_handler);
  CuAssertPtrNotNull(tc, (void *)guild_guard_definition->typed_handler);
  CuAssertPtrNotNull(tc, (void *)command_sentinel_definition->typed_handler);
  CuAssertPtrNotNull(tc, (void *)toll_keeper_definition->typed_handler);
  CuAssertPtrNotNull(tc, (void *)banana_definition->typed_handler);
  CuAssertPtrNotNull(tc, (void *)darkhold_object_definition->typed_handler);
  CuAssertPtrNotNull(tc, (void *)deaths_head_definition->typed_handler);
  CuAssertPtrNotNull(tc, (void *)drow_equipment_definition->typed_handler);
  CuAssertPtrNotNull(tc, (void *)weapon_definition->typed_handler);
  CuAssertPtrNotNull(tc, (void *)avernus_object_definition->typed_handler);
  CuAssertPtrNotNull(tc, (void *)avernus_garden_definition->typed_handler);
  CuAssertPtrNotNull(tc, (void *)monster_combat_definition->typed_handler);
  CuAssertPtrNotNull(tc, (void *)utility_object_definition->typed_handler);
  CuAssertPtrNotNull(tc, (void *)utility_room_definition->typed_handler);
  CuAssertPtrNotNull(tc, (void *)tarrasque_definition->typed_handler);
  CuAssertPtrNotNull(tc, (void *)composite_mobile_definition->typed_handler);
  CuAssertPtrNotNull(tc, (void *)composite_object_definition->typed_handler);
  CuAssertTrue(tc, spec_definition_callback(bank_definition) == bank);
  CuAssertTrue(tc, spec_definition_callback(cloak_definition) == vampire_cloak);
  CuAssertTrue(tc, spec_definition_callback(guild_guard_definition) == rol_guild_guard);
  CuAssertTrue(tc, spec_definition_callback(command_sentinel_definition) == rol_command_sentinel);
  CuAssertTrue(tc, spec_definition_callback(toll_keeper_definition) == rol_toll_keeper);
  CuAssertTrue(tc, spec_definition_callback(banana_definition) == rol_banana);
  CuAssertTrue(tc, spec_definition_callback(darkhold_object_definition) == rol_darkhold_object);
  CuAssertTrue(tc, spec_definition_callback(drow_equipment_definition) == rol_drow_equipment);
  CuAssertTrue(tc, spec_definition_callback(weapon_definition) == rol_weapon_proc);
  CuAssertTrue(tc, spec_definition_callback(avernus_object_definition) == rol_avernus_object);
  CuAssertTrue(tc, spec_definition_callback(avernus_garden_definition) == rol_avernus_garden);
  CuAssertTrue(tc, spec_definition_callback(monster_combat_definition) == rol_monster_combat);
  CuAssertTrue(tc, spec_definition_callback(utility_object_definition) == rol_utility_object);
  CuAssertTrue(tc, spec_definition_callback(utility_room_definition) == rol_utility_room);
  CuAssertTrue(tc, spec_definition_callback(tarrasque_definition) == rol_tarrasque);
  CuAssertTrue(tc, spec_definition_callback(composite_mobile_definition) == rol_composite_mobile);
  CuAssertTrue(tc, spec_definition_callback(composite_object_definition) == rol_composite_object);
  CuAssertTrue(tc, spec_registry_find_by_handler(bank) == bank_definition);
  CuAssertTrue(tc, spec_registry_find_by_handler(vampire_cloak) == cloak_definition);
  CuAssertTrue(tc,
               spec_registry_find_by_handler(rol_darkhold_object) == darkhold_object_definition);
  CuAssertTrue(tc, spec_registry_find_by_handler(rol_drow_equipment) == drow_equipment_definition);
  CuAssertTrue(tc, spec_registry_find_by_handler(rol_guild_guard) == guild_guard_definition);
  CuAssertTrue(tc, spec_registry_find_by_handler(rol_avernus_object) == avernus_object_definition);
  CuAssertTrue(tc, spec_registry_find_by_handler(rol_avernus_garden) == avernus_garden_definition);
  CuAssertTrue(tc, spec_registry_find_by_handler(rol_utility_object) == utility_object_definition);
  CuAssertTrue(tc, spec_registry_find_by_handler(rol_utility_room) == utility_room_definition);
  CuAssertTrue(tc, spec_registry_find_by_handler(rol_tarrasque) == tarrasque_definition);
  CuAssertTrue(tc,
               spec_registry_find_by_handler(rol_composite_mobile) == composite_mobile_definition);
  CuAssertTrue(tc,
               spec_registry_find_by_handler(rol_composite_object) == composite_object_definition);
  CuAssertTrue(tc, spec_definition_supports_event(tarrasque_definition, SPEC_OWNER_MOBILE,
                                                  SPEC_EVENT_MOBILE_DEATH));
  CuAssertTrue(tc, spec_definition_supports_event(tarrasque_definition, SPEC_OWNER_OBJECT,
                                                  SPEC_EVENT_OBJECT_AUTO_PULSE));
  CuAssertTrue(tc,
               spec_registry_find_by_handler(rol_command_sentinel) == command_sentinel_definition);
  CuAssertStrEquals(tc, "Bank", get_spec_func_name(bank));
  CuAssertStrEquals(tc, "Vampire Cloak", get_spec_func_name(vampire_cloak));

  binding = NULL;
  error[0] = '\0';
  CuAssertTrue(tc, spec_binding_replace(&binding, SPEC_OWNER_OBJECT, 6401, "Vampire Cloak",
                                        SPEC_BINDING_SOURCE_WORLD, "typed fixture object Z field",
                                        error, sizeof(error)));
  CuAssertStrEquals(tc, "", error);
  CuAssertTrue(tc, spec_binding_callback(binding) == vampire_cloak);
  CuAssertStrEquals(tc, "Vampire Cloak", spec_binding_persisted_name(binding));
  spec_binding_free(&binding);
}

void Test_spec_typed_dispatch_uses_context_flow_and_invalidation(CuTest *tc)
{
  static const struct spec_event_contract command_event[] = {
      {SPEC_EVENT_COMMAND, SPEC_PROTOTYPE_NONE, SPEC_PLACEMENT_NONE}};
  struct spec_definition definition = {
      .canonical_name = "Typed Test",
      .display_name = "Typed Test",
      .owner_mask = SPEC_OWNER_OBJECT,
      .events = command_event,
      .event_count = 1,
      .binding_source_mask = SPEC_BINDING_SOURCE_WORLD,
      .builder_visibility = SPEC_BUILDER_HIDDEN,
      .category = "Test",
      .description = "Typed dispatch test definition.",
      .typed_adapter = spec_typed_test_adapter,
      .typed_handler = spec_typed_test_handler,
  };
  struct spec_event_context context;
  struct char_data actor;
  struct char_data target;
  struct obj_data owner;

  memset(&context, 0, sizeof(context));
  memset(&actor, 0, sizeof(actor));
  memset(&target, 0, sizeof(target));
  memset(&owner, 0, sizeof(owner));
  context.owner_type = SPEC_OWNER_OBJECT;
  context.event = SPEC_EVENT_COMMAND;
  context.owner = &owner;
  context.actor = &actor;
  context.target = &target;
  context.argument = "typed payload";

  spec_typed_test_calls = 0;
  spec_typed_test_target = NULL;
  spec_typed_test_request_stop = false;
  CuAssertIntEquals(tc, TRUE, spec_dispatch_typed(&context, &definition));
  CuAssertIntEquals(tc, 1, spec_typed_test_calls);
  CuAssertPtrEquals(tc, &target, spec_typed_test_target);
  CuAssertIntEquals(tc, SPEC_FLOW_STOP, context.flow);
  CuAssertIntEquals(tc, SPEC_INVALIDATE_TARGET, (int)context.invalidation);
  CuAssertIntEquals(tc, 0, context.legacy_return);

  context.event = SPEC_EVENT_ITEM_IDENTIFY;
  CuAssertIntEquals(tc, FALSE, spec_dispatch_typed(&context, &definition));
  CuAssertIntEquals(tc, 1, spec_typed_test_calls);
}

void Test_spec_typed_notification_rejects_stop_without_losing_invalidation(CuTest *tc)
{
  static const struct spec_event_contract identify_event[] = {
      {SPEC_EVENT_ITEM_IDENTIFY, SPEC_PROTOTYPE_NONE, SPEC_PLACEMENT_NONE}};
  struct spec_definition definition = {
      .canonical_name = "Typed Notification Test",
      .display_name = "Typed Notification Test",
      .owner_mask = SPEC_OWNER_OBJECT,
      .events = identify_event,
      .event_count = 1,
      .binding_source_mask = SPEC_BINDING_SOURCE_WORLD,
      .builder_visibility = SPEC_BUILDER_HIDDEN,
      .category = "Test",
      .description = "Typed notification dispatch test definition.",
      .typed_adapter = spec_typed_test_adapter,
      .typed_handler = spec_typed_test_handler,
  };
  struct spec_event_context context;
  struct char_data actor;
  struct obj_data owner;

  memset(&context, 0, sizeof(context));
  memset(&actor, 0, sizeof(actor));
  memset(&owner, 0, sizeof(owner));
  context.owner_type = SPEC_OWNER_OBJECT;
  context.event = SPEC_EVENT_ITEM_IDENTIFY;
  context.owner = &owner;
  context.actor = &actor;
  context.argument = "not a magic identify token";

  spec_typed_test_calls = 0;
  spec_typed_test_request_stop = true;
  CuAssertIntEquals(tc, TRUE, spec_dispatch_typed(&context, &definition));
  CuAssertIntEquals(tc, 1, spec_typed_test_calls);
  CuAssertIntEquals(tc, SPEC_FLOW_CONTINUE, context.flow);
  CuAssertIntEquals(tc, SPEC_INVALIDATE_TARGET, (int)context.invalidation);
  spec_typed_test_request_stop = false;
}

void Test_spec_typed_bank_uses_event_identity_instead_of_argument_magic(CuTest *tc)
{
  struct spec_typed_fixture fixture;
  struct spec_event_context context;
  bool commands_preserved;
  bool event_identified;
  bool magic_argument_ignored;

  if (!spec_typed_begin(&fixture))
  {
    CuFail(tc, "unable to initialize typed handler fixture");
    return;
  }

  memset(&context, 0, sizeof(context));
  context.owner_type = SPEC_OWNER_OBJECT;
  context.event = SPEC_EVENT_ITEM_IDENTIFY;
  context.owner = &fixture.worn_cloak;
  context.actor = &fixture.actor;
  context.argument = "not-identify";
  event_identified = spec_dispatch(&context, bank) == TRUE &&
                     strstr(fixture.descriptor.output, "appears to be a bank") != NULL;

  spec_typed_clear_output(&fixture);
  context.event = SPEC_EVENT_COMMAND;
  context.command = 0;
  context.argument = "identify";
  magic_argument_ignored =
      spec_dispatch(&context, bank) == FALSE && fixture.descriptor.output[0] == '\0';

  GET_GOLD(&fixture.actor) = 123;
  GET_BANK_GOLD(&fixture.actor) = 77;
  context.command = 3;
  context.argument = "all";
  commands_preserved = spec_dispatch(&context, bank) == TRUE && GET_GOLD(&fixture.actor) == 0 &&
                       GET_BANK_GOLD(&fixture.actor) == 200;
  context.command = 4;
  commands_preserved = commands_preserved && spec_dispatch(&context, bank) == TRUE &&
                       GET_GOLD(&fixture.actor) == 200 && GET_BANK_GOLD(&fixture.actor) == 0;

  spec_typed_end(&fixture);

  CuAssertTrue(tc, event_identified);
  CuAssertTrue(tc, magic_argument_ignored);
  CuAssertTrue(tc, commands_preserved);
}

void Test_spec_typed_vampire_cloak_uses_event_identity_and_exact_owner(CuTest *tc)
{
  struct spec_typed_fixture fixture;
  struct spec_event_context context;
  bool event_identified;
  bool carried_copy_rejected;
  bool magic_argument_ignored;
  bool worn_owner_command_preserved;
  bool worn_owner_reached_character_check;

  if (!spec_typed_begin(&fixture))
  {
    CuFail(tc, "unable to initialize typed handler fixture");
    return;
  }

  memset(&context, 0, sizeof(context));
  context.owner_type = SPEC_OWNER_OBJECT;
  context.event = SPEC_EVENT_ITEM_IDENTIFY;
  context.owner = &fixture.worn_cloak;
  context.actor = &fixture.actor;
  context.argument = "not-identify";
  event_identified = spec_dispatch(&context, vampire_cloak) == TRUE &&
                     strstr(fixture.descriptor.output, "can be customized") != NULL;

  spec_typed_clear_output(&fixture);
  context.event = SPEC_EVENT_COMMAND;
  context.command = 0;
  context.argument = "identify";
  magic_argument_ignored =
      spec_dispatch(&context, vampire_cloak) == FALSE && fixture.descriptor.output[0] == '\0';

  spec_typed_clear_output(&fixture);
  carried_copy_rejected =
      spec_gateway_command_object(&fixture.actor, &fixture.carried_cloak, 1, "") == TRUE &&
      strstr(fixture.descriptor.output, "must be wearing") != NULL;

  spec_typed_clear_output(&fixture);
  worn_owner_reached_character_check =
      spec_gateway_command_object(&fixture.actor, &fixture.worn_cloak, 1, "") == TRUE &&
      strstr(fixture.descriptor.output, "Only vampires") != NULL;

  spec_typed_clear_output(&fixture);
  GET_REAL_RACE(&fixture.actor) = RACE_VAMPIRE;
  worn_owner_command_preserved =
      spec_gateway_command_object(&fixture.actor, &fixture.worn_cloak, 1, "") == TRUE &&
      strstr(fixture.descriptor.output, "must specify") != NULL;

  spec_typed_end(&fixture);

  CuAssertTrue(tc, event_identified);
  CuAssertTrue(tc, magic_argument_ignored);
  CuAssertTrue(tc, carried_copy_rejected);
  CuAssertTrue(tc, worn_owner_reached_character_check);
  CuAssertTrue(tc, worn_owner_command_preserved);
}
