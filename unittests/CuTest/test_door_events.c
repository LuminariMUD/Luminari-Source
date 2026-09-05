#include "CuTest.h"
#include "../../src/conf.h"
#include "../../src/sysdep.h"
#include "../../src/structs.h"
#include "../../src/utils.h"
#include "../../src/db.h"
#include "../../src/act.h"
#include "../../src/actions.h"
#include "../../src/comm.h"
#include "../../src/handler.h"
#include "../../src/interpreter.h"
#include "../../src/domain_event_runtime.h"
#include "../../src/domain_event_world.h"
#include "../../src/event_runtime.h"
#include "../../src/dgscript/dg_event.h"
#include "../../src/dgscript/dg_scripts.h"
#include "../../src/movement/door_state.h"
#include "../../src/ready_action.h"
#include "../../src/olc/genwld.h"
#include "../../src/quest/hlquest.h"

struct door_fixture
{
  struct room_data rooms[2];
  struct room_direction_data exits[2];
  struct char_data owner;
  struct player_special_data specials;
  struct room_data *saved_world;
  struct char_data *saved_characters;
  room_rnum saved_top;
  unsigned long saved_pulse;
  bool created_commands;
  int notifications;
  bool pair_committed;
  bool cancel_during_notification;
};

static void door_fixture_start(CuTest *tc, struct door_fixture *f)
{
  memset(f, 0, sizeof(*f));
  f->saved_world = world;
  f->saved_top = top_of_world;
  f->saved_characters = character_list;
  f->saved_pulse = pulse;
  f->rooms[0].number = 100;
  f->rooms[1].number = 101;
  f->rooms[0].dir_option[NORTH] = &f->exits[0];
  f->rooms[1].dir_option[SOUTH] = &f->exits[1];
  f->exits[0].to_room = 1;
  f->exits[1].to_room = 0;
  f->exits[0].exit_info = f->exits[1].exit_info = EX_ISDOOR | EX_CLOSED | DOOR_LOCK_FLAGS;
  clear_char(&f->owner);
  f->owner.player.name = (char *)"watcher";
  f->owner.player_specials = &f->specials;
  GET_POS(&f->owner) = POS_STANDING;
  IN_ROOM(&f->owner) = 0;
  f->rooms[0].people = &f->owner;
  world = f->rooms;
  top_of_world = 1;
  character_list = &f->owner;
  pulse = 1000;
  event_free_all();
  domain_event_world_shutdown();
  event_init();
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK, domain_event_runtime_init());
  CuAssertIntEquals(tc, GAME_SCHEDULER_OK, event_runtime_seal_types());
  if (complete_cmd_info == NULL)
  {
    create_command_list();
    f->created_commands = true;
  }
}

static void door_fixture_end(CuTest *tc, struct door_fixture *f)
{
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK, domain_event_runtime_shutdown());
  CuAssertPtrEquals(tc, NULL, f->owner.ready_action);
  event_free_all();
  domain_event_world_shutdown();
  if (f->created_commands)
    free_command_list();
  world = f->saved_world;
  top_of_world = f->saved_top;
  character_list = f->saved_characters;
  pulse = f->saved_pulse;
}

static void observe_door(const struct domain_event_context *context, void *data)
{
  struct door_fixture *f = data;
  const struct domain_door_state_changed *fact = context->payload;

  f->notifications++;
  if (!(f->exits[0].exit_info & EX_CLOSED) && !(f->exits[1].exit_info & EX_CLOSED) &&
      fact->previous_state != fact->current_state)
    f->pair_committed = true;
  if (f->cancel_during_notification)
    domain_event_runtime_character_extracted(&f->owner, 0U);
}

static void door_observe(CuTest *tc, struct door_fixture *f, room_rnum room)
{
  struct domain_event_subscription_config config = {0};
  struct domain_event_subscription_handle handle;

  config.type = DOMAIN_EVENT_DOOR_STATE_CHANGED;
  config.topic.role = DOMAIN_EVENT_TOPIC_SUBJECT;
  config.topic.entity = domain_event_room_handle(room);
  config.owner = domain_event_character_handle(&f->owner);
  config.identity = "test.door";
  config.handler = observe_door;
  config.handler_context = f;
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK,
                    domain_event_subscribe(domain_event_runtime_bus(), &config, &handle));
}

void TestDoorFactsCommitPairsAndSuppressNoop(CuTest *tc)
{
  struct door_fixture f;
  struct door_state_operation operation;

  door_fixture_start(tc, &f);
  door_observe(tc, &f, 0);
  door_observe(tc, &f, 1);
  CuAssertTrue(tc, door_state_begin(&operation, 0, NORTH, true, DOMAIN_DOOR_GAMEPLAY));
  door_state_apply(&operation, DOOR_LOCK_FLAGS, 0);
  door_state_apply(&operation, EX_CLOSED, 0);
  CuAssertIntEquals(tc, 0, f.notifications);
  door_state_finish(&operation);
  CuAssertIntEquals(tc, 2, f.notifications);
  CuAssertTrue(tc, f.pair_committed);
  CuAssertIntEquals(tc, EX_ISDOOR, f.exits[0].exit_info);
  CuAssertIntEquals(tc, EX_ISDOOR, f.exits[1].exit_info);
  door_state_finish(&operation);
  door_state_update(0, NORTH, EX_CLOSED, 0, true, DOMAIN_DOOR_GAMEPLAY);
  CuAssertIntEquals(tc, 2, f.notifications);
  door_fixture_end(tc, &f);
}

void TestDoorPairsPreserveAsymmetricAndMissingExits(CuTest *tc)
{
  struct door_fixture f;

  door_fixture_start(tc, &f);
  door_observe(tc, &f, 0);
  door_observe(tc, &f, 1);
  f.exits[1].to_room = 1;
  door_state_update(0, NORTH, EX_CLOSED, 0, true, DOMAIN_DOOR_GAMEPLAY);
  CuAssertIntEquals(tc, 1, f.notifications);
  CuAssertTrue(tc, (f.exits[1].exit_info & EX_CLOSED) != 0);
  door_state_update(NOWHERE, NORTH, EX_CLOSED, 0, true, DOMAIN_DOOR_GAMEPLAY);
  door_state_update(0, -1, EX_CLOSED, 0, true, DOMAIN_DOOR_GAMEPLAY);
  door_state_update(0, EAST, EX_CLOSED, 0, true, DOMAIN_DOOR_GAMEPLAY);
  CuAssertIntEquals(tc, 1, f.notifications);
  door_fixture_end(tc, &f);
}

void TestDoorReadyRunsOnceAtNativeBoundaryFromOtherSide(CuTest *tc)
{
  struct door_fixture f;
  size_t timers;
  struct domain_event_bus_stats stats;

  door_fixture_start(tc, &f);
  timers = event_runtime_event_count();
  do_ready(&f.owner, "rest on door open north", 0, 0);
  CuAssertPtrNotNull(tc, f.owner.ready_action);
  CuAssertIntEquals(tc, (int)timers, (int)event_runtime_event_count());
  door_state_update(1, SOUTH, DOOR_LOCK_FLAGS, 0, true, DOMAIN_DOOR_GAMEPLAY);
  CuAssertIntEquals(tc, (int)timers, (int)event_runtime_event_count());
  quest_open_door(1, SOUTH);
  CuAssertIntEquals(tc, POS_STANDING, GET_POS(&f.owner));
  CuAssertIntEquals(tc, (int)timers + 1, (int)event_runtime_event_count());
  quest_open_door(1, SOUTH);
  CuAssertIntEquals(tc, (int)timers + 1, (int)event_runtime_event_count());
  pulse++;
  event_test_advance();
  CuAssertIntEquals(tc, POS_RESTING, GET_POS(&f.owner));
  CuAssertPtrEquals(tc, NULL, f.owner.ready_action);
  domain_event_bus_get_stats(domain_event_runtime_bus(), &stats);
  CuAssertIntEquals(tc, 0, (int)stats.live_subscription_count);
  door_fixture_end(tc, &f);
}

void TestDoorReadyFiltersInvalidHiddenAndAdministrativeChanges(CuTest *tc)
{
  struct door_fixture f;
  size_t timers;

  door_fixture_start(tc, &f);
  do_ready(&f.owner, "rest on door open east", 0, 0);
  CuAssertPtrEquals(tc, NULL, f.owner.ready_action);
  do_ready(&f.owner, "rest on door open north extra", 0, 0);
  CuAssertPtrEquals(tc, NULL, f.owner.ready_action);
  f.exits[0].exit_info |= EX_HIDDEN_HARD;
  do_ready(&f.owner, "rest on door open north", 0, 0);
  CuAssertPtrEquals(tc, NULL, f.owner.ready_action);
  f.exits[0].exit_info &= ~EX_HIDDEN_HARD;
  timers = event_runtime_event_count();
  do_ready(&f.owner, "rest on door open north", 0, 0);
  door_state_update(1, SOUTH, EX_CLOSED, 0, false, DOMAIN_DOOR_GAMEPLAY);
  door_state_update(0, NORTH, EX_CLOSED, 0, false, DOMAIN_DOOR_RESET);
  CuAssertIntEquals(tc, (int)timers, (int)event_runtime_event_count());
  CuAssertPtrNotNull(tc, f.owner.ready_action);
  do_ready(&f.owner, "cancel", 0, 0);
  do_ready(&f.owner, "rest on door open north", 0, 0);
  CuAssertPtrEquals(tc, NULL, f.owner.ready_action);
  door_fixture_end(tc, &f);
}

void TestDoorReadyCancelsCloseReopenReplacementAndExtraction(CuTest *tc)
{
  struct door_fixture f;
  struct door_state_operation operation;
  int scenario;

  door_fixture_start(tc, &f);
  for (scenario = 0; scenario < 5; scenario++)
  {
    door_state_update(0, NORTH, 0, EX_CLOSED, false, DOMAIN_DOOR_RESET);
    do_ready(&f.owner, "rest on door open north", 0, 0);
    door_state_update(0, NORTH, EX_CLOSED, 0, false, DOMAIN_DOOR_GAMEPLAY);
    if (scenario == 0)
    {
      door_state_update(0, NORTH, 0, EX_CLOSED, false, DOMAIN_DOOR_GAMEPLAY);
      door_state_update(0, NORTH, EX_CLOSED, 0, false, DOMAIN_DOOR_GAMEPLAY);
    }
    else if (scenario == 1)
    {
      door_state_begin(&operation, 0, NORTH, false, DOMAIN_DOOR_EDIT);
      f.exits[0].event_identity = 0U;
      door_state_finish(&operation);
    }
    else if (scenario == 2)
      domain_event_runtime_character_moved(&f.owner, 0, 1, NORTH);
    else if (scenario == 3)
      domain_event_runtime_character_died(&f.owner, NULL);
    else
      domain_event_runtime_character_extracted(&f.owner, 0U);
    pulse++;
    event_test_advance();
    CuAssertIntEquals(tc, POS_STANDING, GET_POS(&f.owner));
    CuAssertPtrEquals(tc, NULL, f.owner.ready_action);
  }
  door_fixture_end(tc, &f);
}

void TestDoorScriptPublishersAndRemoval(CuTest *tc)
{
  struct door_fixture f;
  struct obj_data object;
  struct char_data mobile;
  char wopen[] = "wdoor 100 north flags a";
  char oclose[] = "odoor 100 north flags ab";
  char mopen[] = "100 north flags a";
  char wdelete[] = "wdoor 100 north purge";

  door_fixture_start(tc, &f);
  clear_object(&object);
  IN_ROOM(&object) = 0;
  clear_char(&mobile);
  SET_BIT_AR(MOB_FLAGS(&mobile), MOB_ISNPC);
  IN_ROOM(&mobile) = 0;
  mobile.player.name = (char *)"scripted";
  door_observe(tc, &f, 0);
  do_ready(&f.owner, "rest on door open north", 0, 0);
  wld_command_interpreter(&f.rooms[0], wopen);
  CuAssertIntEquals(tc, 1, f.notifications);
  pulse++;
  event_test_advance();
  CuAssertIntEquals(tc, POS_RESTING, GET_POS(&f.owner));
  GET_POS(&f.owner) = POS_STANDING;
  obj_command_interpreter(&object, oclose);
  do_ready(&f.owner, "rest on door open north", 0, 0);
  do_mdoor(&mobile, mopen, 0, 0);
  CuAssertIntEquals(tc, 3, f.notifications);
  pulse++;
  event_test_advance();
  CuAssertIntEquals(tc, POS_RESTING, GET_POS(&f.owner));
  /* DG purge frees its exit, so give it a real allocation. */
  f.rooms[0].dir_option[NORTH] = calloc(1, sizeof(f.exits[0]));
  *f.rooms[0].dir_option[NORTH] = f.exits[0];
  f.rooms[0].dir_option[NORTH]->event_identity = 0;
  f.rooms[0].dir_option[NORTH]->exit_info |= EX_CLOSED;
  do_ready(&f.owner, "rest on door open north", 0, 0);
  wld_command_interpreter(&f.rooms[0], wdelete);
  CuAssertPtrEquals(tc, NULL, f.rooms[0].dir_option[NORTH]);
  CuAssertPtrEquals(tc, NULL, f.owner.ready_action);
  door_fixture_end(tc, &f);
}

void TestDoorNotificationOwnerExtractionCancelsListenersSafely(CuTest *tc)
{
  struct door_fixture f;
  struct domain_event_bus_stats stats;

  door_fixture_start(tc, &f);
  door_observe(tc, &f, 0);
  door_observe(tc, &f, 1);
  do_ready(&f.owner, "rest on door open north", 0, 0);
  f.cancel_during_notification = true;
  door_state_update(0, NORTH, EX_CLOSED, 0, true, DOMAIN_DOOR_GAMEPLAY);
  domain_event_bus_get_stats(domain_event_runtime_bus(), &stats);
  CuAssertIntEquals(tc, 0, (int)stats.live_subscription_count);
  CuAssertPtrEquals(tc, NULL, f.owner.ready_action);
  pulse++;
  event_test_advance();
  CuAssertIntEquals(tc, POS_STANDING, GET_POS(&f.owner));
  door_fixture_end(tc, &f);
}

void TestDoorCommandsPublishOnlySuccessfulRoomChanges(CuTest *tc)
{
  struct door_fixture f;
  struct obj_data chest;

  door_fixture_start(tc, &f);
  door_observe(tc, &f, 0);
  door_observe(tc, &f, 1);
  do_gen_door(&f.owner, "door north", 0, SCMD_OPEN);
  CuAssertIntEquals(tc, 0, f.notifications); /* Locked: no committed mutation. */
  f.exits[0].exit_info = f.exits[1].exit_info = EX_ISDOOR | EX_CLOSED;
  do_gen_door(&f.owner, "door north", 0, SCMD_OPEN);
  CuAssertIntEquals(tc, 2, f.notifications);
  CuAssertTrue(tc, f.pair_committed);
  do_gen_door(&f.owner, "door north", 0, SCMD_OPEN);
  CuAssertIntEquals(tc, 2, f.notifications);
  SET_BIT_AR(MOB_FLAGS(&f.owner), MOB_ISNPC);
  do_gen_door(&f.owner, "door north", 0, SCMD_CLOSE);
  CuAssertIntEquals(tc, 4, f.notifications);
  REMOVE_BIT_AR(MOB_FLAGS(&f.owner), MOB_ISNPC);

  clear_object(&chest);
  chest.name = (char *)"chest";
  chest.short_description = (char *)"a chest";
  GET_OBJ_TYPE(&chest) = ITEM_CONTAINER;
  GET_OBJ_VAL(&chest, 1) = CONT_CLOSEABLE | CONT_CLOSED;
  IN_ROOM(&chest) = 0;
  f.rooms[0].light = 1;
  f.rooms[0].contents = &chest;
  do_gen_door(&f.owner, "chest", 0, SCMD_OPEN);
  CuAssertTrue(tc, (GET_OBJ_VAL(&chest, 1) & CONT_CLOSED) == 0);
  CuAssertIntEquals(tc, 4, f.notifications); /* A chest is not a room door. */
  f.rooms[0].contents = NULL;
  door_fixture_end(tc, &f);
}

void TestDoorRetargetInvalidatesBindingWithoutFlagChange(CuTest *tc)
{
  struct door_fixture f;
  struct door_state_operation operation;
  uint64_t old_identity;

  door_fixture_start(tc, &f);
  do_ready(&f.owner, "rest on door open north", 0, 0);
  old_identity = door_state_identity(0, NORTH);
  door_state_begin(&operation, 0, NORTH, false, DOMAIN_DOOR_EDIT);
  f.exits[0].to_room = 0;
  door_state_finish(&operation);
  CuAssertTrue(tc, old_identity != door_state_identity(0, NORTH));
  CuAssertPtrEquals(tc, NULL, f.owner.ready_action);
  door_fixture_end(tc, &f);
}

void TestDoorReadyLatenessMeasuresDeadlineNotIntentionalDelay(CuTest *tc)
{
  struct door_fixture f;
  struct ready_action_latency stats;
  int index;

  door_fixture_start(tc, &f);
  ready_action_latency_reset();
  for (index = 0; index < 100; index++)
  {
    GET_POS(&f.owner) = POS_STANDING;
    door_state_update(0, NORTH, 0, EX_CLOSED, false, DOMAIN_DOOR_RESET);
    do_ready(&f.owner, "rest on door open north", 0, 0);
    door_state_update(0, NORTH, EX_CLOSED, 0, false, DOMAIN_DOOR_GAMEPLAY);
    pulse += 1U + index;
    event_test_advance();
    CuAssertIntEquals(tc, POS_RESTING, GET_POS(&f.owner));
  }
  ready_action_latency_read(&stats);
  CuAssertIntEquals(tc, 100, (int)stats.samples);
  CuAssertIntEquals(tc, 100, (int)stats.callbacks);
  CuAssertIntEquals(tc, 49, (int)stats.p50);
  CuAssertIntEquals(tc, 94, (int)stats.p95);
  CuAssertIntEquals(tc, 98, (int)stats.p99);
  CuAssertIntEquals(tc, 99, (int)stats.maximum);
  ready_action_latency_reset();
  ready_action_latency_read(&stats);
  CuAssertIntEquals(tc, 0, (int)stats.samples);
  door_fixture_end(tc, &f);
}

void TestDoorOlcReplacementCancelsWithoutOpening(CuTest *tc)
{
  struct door_fixture f;
  struct room_data draft;

  door_fixture_start(tc, &f);
  f.rooms[0].dir_option[NORTH] = calloc(1, sizeof(f.exits[0]));
  *f.rooms[0].dir_option[NORTH] = f.exits[0];
  draft = f.rooms[0];
  draft.dir_option[NORTH] = &f.exits[0];
  draft.name = (char *)"edited room";
  draft.description = (char *)"An edited room.\r\n";
  do_ready(&f.owner, "rest on door open north", 0, 0);
  CuAssertTrue(tc, copy_room(&f.rooms[0], &draft));
  CuAssertPtrEquals(tc, NULL, f.owner.ready_action);
  CuAssertTrue(tc, (f.rooms[0].dir_option[NORTH]->exit_info & EX_CLOSED) != 0);
  free_room_strings(&f.rooms[0]);
  door_fixture_end(tc, &f);
}

void TestDoorScriptVetoDoesNotPublish(CuTest *tc)
{
  struct door_fixture f;
  struct script_data script = {0};
  struct trig_data trigger = {0};
  struct cmdlist_element command = {0};

  door_fixture_start(tc, &f);
  door_observe(tc, &f, 0);
  f.exits[0].exit_info = EX_ISDOOR | EX_CLOSED;
  script.types = WTRIG_DOOR;
  script.trig_list = &trigger;
  trigger.trigger_type = WTRIG_DOOR;
  trigger.narg = 100;
  trigger.name = (char *)"door veto";
  trigger.nr = NOTHING;
  trigger.cmdlist = &command;
  command.cmd = (char *)"return 0";
  SCRIPT(&f.rooms[0]) = &script;
  do_gen_door(&f.owner, "door north", 0, SCMD_OPEN);
  CuAssertIntEquals(tc, 0, f.notifications);
  CuAssertTrue(tc, (f.exits[0].exit_info & EX_CLOSED) != 0);
  SCRIPT(&f.rooms[0]) = NULL;
  free_varlist(trigger.var_list);
  door_fixture_end(tc, &f);
}

static void ignore_door_test_fact(const struct domain_event_context *context, void *data)
{
  (void)context;
  (void)data;
}

void TestDoorReadyAdmissionFailuresReleasePartialState(CuTest *tc)
{
  struct door_fixture f;
  struct domain_event_subscription_config config = {0};
  struct domain_event_subscription_handle
      subscriptions[DOMAIN_EVENT_DEFAULT_MAX_SUBSCRIPTIONS_PER_OWNER - 1U];
  size_t index, timers;
  struct domain_entity_handle owner;
  struct domain_event_bus_stats stats;
  struct game_event_owner native_owner = game_event_owner_none();
  struct event_runtime_handle occupied;
  game_event_type_id_t type;

  door_fixture_start(tc, &f);
  timers = event_runtime_event_count();
  owner = domain_event_character_handle(&f.owner);
  config.type = DOMAIN_EVENT_CHARACTER_MOVED;
  config.topic.role = DOMAIN_EVENT_TOPIC_SUBJECT;
  config.topic.entity = owner;
  config.owner = owner;
  config.identity = "ready.owner-moved";
  config.handler = ignore_door_test_fact;
  for (index = 0U; index < DOMAIN_EVENT_DEFAULT_MAX_SUBSCRIPTIONS_PER_OWNER - 1U; index++)
    CuAssertIntEquals(
        tc, DOMAIN_EVENT_OK,
        domain_event_subscribe(domain_event_runtime_bus(), &config, &subscriptions[index]));
  do_ready(&f.owner, "rest on door open north", 0, 0);
  CuAssertPtrEquals(tc, NULL, f.owner.ready_action);
  domain_event_bus_get_stats(domain_event_runtime_bus(), &stats);
  CuAssertIntEquals(tc, DOMAIN_EVENT_DEFAULT_MAX_SUBSCRIPTIONS_PER_OWNER - 1U,
                    (int)stats.live_subscription_count);
  for (index = 0U; index < DOMAIN_EVENT_DEFAULT_MAX_SUBSCRIPTIONS_PER_OWNER - 1U; index++)
    domain_event_unsubscribe(domain_event_runtime_bus(), subscriptions[index]);

  native_owner.kind = GAME_EVENT_OWNER_CHARACTER;
  native_owner.runtime_id = owner.runtime_id;
  native_owner.generation = owner.generation;
  CuAssertIntEquals(tc, GAME_SCHEDULER_OK, event_runtime_find_type("action.ready.execute", &type));
  CuAssertIntEquals(tc, GAME_SCHEDULER_OK,
                    event_runtime_schedule_owned_after(type, native_owner, 100, NULL, &occupied));
  do_ready(&f.owner, "rest on door open north", 0, 0);
  CuAssertPtrNotNull(tc, f.owner.ready_action);
  door_state_update(0, NORTH, EX_CLOSED, 0, false, DOMAIN_DOOR_GAMEPLAY);
  CuAssertPtrEquals(tc, NULL, f.owner.ready_action);
  domain_event_bus_get_stats(domain_event_runtime_bus(), &stats);
  CuAssertIntEquals(tc, 0, (int)stats.live_subscription_count);
  event_runtime_cancel(occupied);
  CuAssertIntEquals(tc, (int)timers, (int)event_runtime_event_count());
  door_fixture_end(tc, &f);
}

void TestDoorReadyDoesNotGrantAnExhaustedMoveAction(CuTest *tc)
{
  struct door_fixture f;

  door_fixture_start(tc, &f);
  do_ready(&f.owner, "close door north on door open north", 0, 0);
  start_action_cooldown(&f.owner, atMOVE, 60);
  start_action_cooldown(&f.owner, atSTANDARD, 60);
  door_state_update(0, NORTH, EX_CLOSED, 0, false, DOMAIN_DOOR_GAMEPLAY);
  pulse++;
  event_test_advance();
  CuAssertPtrEquals(tc, NULL, f.owner.ready_action);
  CuAssertTrue(tc, (f.exits[0].exit_info & EX_CLOSED) == 0);
  door_fixture_end(tc, &f);
}
