#include "CuTest.h"

#include "../../src/conf.h"
#include "../../src/sysdep.h"
#include "../../src/structs.h"
#include "../../src/activity_manager.h"
#include "../../src/actions.h"
#include "../../src/comm.h"
#include "../../src/db.h"
#include "../../src/dgscript/dg_event.h"
#include "../../src/domain_event_types.h"
#include "../../src/domain_event_world.h"
#include "../../src/interpreter.h"
#include "../../src/mud_event.h"
#include "../../src/net/protocol.h"

#include <string.h>

static bool activity_source_places_turn_hook_in_semantic_round(void)
{
  const char *root = getenv("LUMINARI_TEST_ROOT");
  char path[PATH_MAX];
  char line[512];
  FILE *file;
  bool compatibility = false;
  bool semantic = false;
  bool compatibility_hook = false;
  bool semantic_hook = false;

  if (root == NULL || *root == '\0')
    root = ".";
  if (snprintf(path, sizeof(path), "%s/src/combat/fight.c", root) >= (int)sizeof(path))
    return false;
  file = fopen(path, "r");
  if (file == NULL)
    return false;
  while (fgets(line, sizeof(line), file) != NULL)
  {
    if (strstr(line, "bool combat_run_compatibility_phase") != NULL)
    {
      compatibility = true;
      semantic = false;
    }
    else if (strstr(line, "bool combat_run_semantic_round") != NULL)
    {
      compatibility = false;
      semantic = true;
    }
    else if (strstr(line, "EVENTFUNC(event_combat_round)") != NULL)
    {
      compatibility = false;
      semantic = false;
    }
    if (strstr(line, "primary_activity_on_semantic_turn(ch);") != NULL)
    {
      compatibility_hook = compatibility_hook || compatibility;
      semantic_hook = semantic_hook || semantic;
    }
  }
  fclose(file);
  return semantic_hook && !compatibility_hook;
}

struct activity_test_context
{
  unsigned int progress_calls;
  unsigned int completion_calls;
  unsigned int ended_calls;
  unsigned int recheck_calls;
  bool recheck_allowed;
  bool cancel_during_recheck;
  bool pause_during_progress;
};

struct activity_test_fixture
{
  struct char_data actor;
  struct char_data target;
  struct descriptor_data descriptor;
  struct char_data *saved_character_list;
  struct domain_event_bus *bus;
  unsigned long saved_pulse;
  struct activity_test_context context;
  unsigned int terminal_transition_calls;
  bool terminal_transition_saw_activity;
  bool terminal_transition_cancelled_activity;
};

static bool activity_test_recheck(struct char_data *actor, void *target, void *context)
{
  struct activity_test_context *trace = context;

  trace->recheck_calls++;
  if (trace->cancel_during_recheck)
    (void)primary_activity_cancel(actor, PRIMARY_ACTIVITY_END_PLAYER_CANCELLED, false);
  return actor != NULL && target != NULL && trace->recheck_allowed;
}

static void activity_test_observe_transition(const struct domain_event_context *context,
                                             void *handler_context)
{
  const struct domain_activity_transitioned *event = context->payload;
  struct activity_test_fixture *fixture = handler_context;
  struct primary_activity_snapshot snapshot;

  if (event->current_state != PRIMARY_ACTIVITY_STATE_COMPLETED &&
      event->current_state != PRIMARY_ACTIVITY_STATE_CANCELLED)
    return;
  fixture->terminal_transition_calls++;
  fixture->terminal_transition_saw_activity =
      primary_activity_snapshot(&fixture->actor, &snapshot);
  fixture->terminal_transition_cancelled_activity =
      primary_activity_cancel(&fixture->actor, PRIMARY_ACTIVITY_END_PLAYER_CANCELLED, false);
}

static void activity_test_progress(struct char_data *actor, void *target,
                                   uint32_t completed_steps, uint32_t total_steps,
                                   void *context)
{
  struct activity_test_context *trace = context;

  (void)actor;
  (void)target;
  (void)completed_steps;
  (void)total_steps;
  trace->progress_calls++;
  if (trace->pause_during_progress)
    (void)primary_activity_pause(actor, false);
}

static void activity_test_complete(struct char_data *actor, void *target, void *context)
{
  struct activity_test_context *trace = context;

  (void)actor;
  (void)target;
  trace->completion_calls++;
}

static void activity_test_ended(struct char_data *actor,
                                enum primary_activity_end_reason reason, void *context)
{
  struct activity_test_context *trace = context;

  (void)actor;
  (void)reason;
  trace->ended_calls++;
}

static void activity_test_begin(CuTest *tc, struct activity_test_fixture *fixture)
{
  struct domain_event_handler_config observer = {
      DOMAIN_EVENT_ACTIVITY_TRANSITIONED, "test.activity.transition", -100,
      activity_test_observe_transition, NULL};
  enum domain_event_status status;

  memset(fixture, 0, sizeof(*fixture));
  fixture->saved_character_list = character_list;
  fixture->saved_pulse = pulse;
  primary_activity_manager_shutdown();
  event_free_all();
  CuAssertIntEquals(tc, 1, event_test_select_backend(EVENT_BACKEND_GAME_SCHEDULER));
  pulse = 4000U;
  event_init();
  clear_char(&fixture->actor);
  clear_char(&fixture->target);
  fixture->actor.player.name = "activity actor";
  fixture->target.player.name = "activity target";
  memset(&fixture->descriptor, 0, sizeof(fixture->descriptor));
  fixture->descriptor.character = &fixture->actor;
  fixture->descriptor.output = fixture->descriptor.small_outbuf;
  fixture->descriptor.bufspace = SMALL_BUFSIZE - 1;
  fixture->descriptor.pProtocol = ProtocolCreate();
  STATE(&fixture->descriptor) = CON_PLAYING;
  fixture->actor.desc = &fixture->descriptor;
  fixture->actor.next = &fixture->target;
  fixture->target.next = NULL;
  character_list = &fixture->actor;
  fixture->context.recheck_allowed = true;
  fixture->bus = domain_event_bus_create(NULL, &status);
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK, status);
  CuAssertPtrNotNull(tc, fixture->bus);
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK,
                    domain_event_register_foundation_types(fixture->bus));
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK,
                    domain_event_world_register_resolvers(fixture->bus));
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK,
                    primary_activity_manager_init(fixture->bus));
  observer.handler_context = fixture;
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK,
                    domain_event_register_handler(fixture->bus, &observer));
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK, domain_event_seal(fixture->bus));
}

static void activity_test_end(struct activity_test_fixture *fixture)
{
  primary_activity_manager_shutdown();
  domain_event_bus_destroy(fixture->bus);
  event_free_all();
  fixture->actor.desc = NULL;
  ProtocolDestroy(fixture->descriptor.pProtocol);
  character_list = fixture->saved_character_list;
  pulse = fixture->saved_pulse;
}

static struct primary_activity_definition
activity_test_definition(struct activity_test_fixture *fixture)
{
  struct primary_activity_definition definition;

  memset(&definition, 0, sizeof(definition));
  definition.type = PRIMARY_ACTIVITY_TEST;
  definition.display_name = "testing an activity";
  definition.capabilities = PRIMARY_ACTIVITY_CAP_MOVEMENT |
                            PRIMARY_ACTIVITY_CAP_ATTENTION |
                            PRIMARY_ACTIVITY_CAP_STANDARD;
  definition.traits = PRIMARY_ACTIVITY_TRAIT_STATIONARY |
                      PRIMARY_ACTIVITY_TRAIT_DISTRACTED;
  definition.progress_model = PRIMARY_ACTIVITY_PROGRESS_PROGRESSIVE;
  definition.progress_owner = PRIMARY_ACTIVITY_PROGRESS_CHARACTER;
  definition.total_steps = 2U;
  definition.step_interval = 10L;
  definition.combat_actions_required = ACTION_STANDARD;
  definition.movement_response = PRIMARY_ACTIVITY_RESPONSE_CANCEL;
  definition.damage_response = PRIMARY_ACTIVITY_RESPONSE_DELAY;
  definition.combat_response = PRIMARY_ACTIVITY_RESPONSE_PAUSE;
  definition.target_loss_response = PRIMARY_ACTIVITY_RESPONSE_CANCEL;
  definition.command_response = PRIMARY_ACTIVITY_RESPONSE_REJECT;
  definition.delay_pulses = 20L;
  definition.recheck = activity_test_recheck;
  definition.progress = activity_test_progress;
  definition.complete = activity_test_complete;
  definition.ended = activity_test_ended;
  definition.context = &fixture->context;
  return definition;
}

static bool activity_test_start(struct activity_test_fixture *fixture,
                                const struct primary_activity_definition *definition)
{
  return primary_activity_start(&fixture->actor,
                                domain_event_character_handle(&fixture->target), definition);
}

void Test_primary_activity_camp_selector_defaults_managed_and_requires_explicit_rollback(
    CuTest *tc)
{
  CuAssertTrue(tc, primary_activity_test_camp_value_is_managed(NULL));
  CuAssertTrue(tc, primary_activity_test_camp_value_is_managed(""));
  CuAssertTrue(tc, primary_activity_test_camp_value_is_managed("managed"));
  CuAssertTrue(tc, !primary_activity_test_camp_value_is_managed("legacy"));
  CuAssertTrue(tc, !primary_activity_test_camp_value_is_managed("off"));
  CuAssertTrue(tc, !primary_activity_test_camp_value_is_managed("0"));
}

void Test_primary_activity_admission_and_command_capabilities_are_explicit(CuTest *tc)
{
  struct activity_test_fixture fixture;
  struct primary_activity_definition definition;
  struct primary_activity_snapshot snapshot;
  struct domain_character_moved moved;

  activity_test_begin(tc, &fixture);
  definition = activity_test_definition(&fixture);
  CuAssertTrue(tc, activity_test_start(&fixture, &definition));
  CuAssertTrue(tc, !activity_test_start(&fixture, &definition));
  CuAssertTrue(tc, primary_activity_command_admit(
                       &fixture.actor, "look", PRIMARY_ACTIVITY_CAP_ATTENTION, true, false));
  CuAssertTrue(tc, primary_activity_snapshot(&fixture.actor, &snapshot));
  CuAssertTrue(tc, primary_activity_command_admit(
                       &fixture.actor, "say", PRIMARY_ACTIVITY_CAP_SPEECH, false, false));
  CuAssertTrue(tc, !primary_activity_command_admit(
                        &fixture.actor, "cast", PRIMARY_ACTIVITY_CAP_ATTENTION, false, false));
  CuAssertTrue(tc, primary_activity_snapshot(&fixture.actor, &snapshot));
  CuAssertTrue(tc, primary_activity_command_admit(
                       &fixture.actor, "north", PRIMARY_ACTIVITY_CAP_MOVEMENT, false, false));
  CuAssertTrue(tc, primary_activity_snapshot(&fixture.actor, &snapshot));
  memset(&moved, 0, sizeof(moved));
  moved.character = domain_event_character_handle(&fixture.actor);
  moved.from_room.kind = DOMAIN_ENTITY_ROOM;
  moved.from_room.runtime_id = 10U;
  moved.from_room.generation = 1U;
  moved.to_room.kind = DOMAIN_ENTITY_ROOM;
  moved.to_room.runtime_id = 11U;
  moved.to_room.generation = 1U;
  CuAssertIntEquals(
      tc, DOMAIN_EVENT_OK,
      DOMAIN_EVENT_PUBLISH(fixture.bus, DOMAIN_EVENT_CHARACTER_MOVED, &moved));
  CuAssertTrue(tc, !primary_activity_snapshot(&fixture.actor, &snapshot));
  CuAssertIntEquals(tc, 1, (int)fixture.context.ended_calls);
  CuAssertIntEquals(tc, 0, event_queue_depth());
  activity_test_end(&fixture);
}

void Test_primary_activity_wall_clock_completes_and_cleans_exactly_once(CuTest *tc)
{
  struct activity_test_fixture fixture;
  struct primary_activity_definition definition;
  struct primary_activity_snapshot snapshot;

  activity_test_begin(tc, &fixture);
  definition = activity_test_definition(&fixture);
  CuAssertTrue(tc, activity_test_start(&fixture, &definition));
  CuAssertIntEquals(tc, 1, event_queue_depth());
  pulse += 10U;
  event_process();
  CuAssertTrue(tc, primary_activity_snapshot(&fixture.actor, &snapshot));
  CuAssertIntEquals(tc, 1, (int)snapshot.completed_steps);
  CuAssertIntEquals(tc, 1, (int)fixture.context.progress_calls);
  pulse += 10U;
  event_process();
  CuAssertTrue(tc, !primary_activity_snapshot(&fixture.actor, &snapshot));
  CuAssertIntEquals(tc, 1, (int)fixture.context.completion_calls);
  CuAssertIntEquals(tc, 0, (int)fixture.context.ended_calls);
  CuAssertIntEquals(tc, 1, (int)fixture.terminal_transition_calls);
  CuAssertTrue(tc, !fixture.terminal_transition_saw_activity);
  CuAssertTrue(tc, !fixture.terminal_transition_cancelled_activity);
  CuAssertIntEquals(tc, 0, event_queue_depth());
  pulse += 100U;
  event_process();
  CuAssertIntEquals(tc, 1, (int)fixture.context.completion_calls);
  activity_test_end(&fixture);
}

void Test_primary_activity_domain_policies_preserve_or_discard_progress(CuTest *tc)
{
  struct activity_test_fixture fixture;
  struct primary_activity_definition definition;
  struct primary_activity_snapshot snapshot;
  struct domain_character_damaged damaged;
  struct domain_combat_state_changed combat;
  struct domain_character_moved moved;
  struct domain_entity_extracted extracted;

  activity_test_begin(tc, &fixture);
  definition = activity_test_definition(&fixture);
  CuAssertTrue(tc, activity_test_start(&fixture, &definition));
  damaged.target = domain_event_character_handle(&fixture.actor);
  damaged.source = domain_event_character_handle(&fixture.target);
  damaged.amount = 4;
  damaged.damage_type = 1;
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK,
                    DOMAIN_EVENT_PUBLISH(fixture.bus, DOMAIN_EVENT_CHARACTER_DAMAGED, &damaged));
  CuAssertTrue(tc, primary_activity_snapshot(&fixture.actor, &snapshot));
  CuAssertIntEquals(tc, 30, (int)snapshot.next_step_pulses);
  combat.character = damaged.target;
  combat.opponent = damaged.source;
  combat.in_combat = true;
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK,
                    DOMAIN_EVENT_PUBLISH(fixture.bus, DOMAIN_EVENT_COMBAT_STATE_CHANGED, &combat));
  CuAssertTrue(tc, primary_activity_snapshot(&fixture.actor, &snapshot));
  CuAssertIntEquals(tc, PRIMARY_ACTIVITY_STATE_PAUSED, snapshot.state);
  CuAssertIntEquals(tc, 0, event_queue_depth());
  combat.in_combat = false;
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK,
                    DOMAIN_EVENT_PUBLISH(fixture.bus, DOMAIN_EVENT_COMBAT_STATE_CHANGED, &combat));
  CuAssertTrue(tc, primary_activity_snapshot(&fixture.actor, &snapshot));
  CuAssertIntEquals(tc, PRIMARY_ACTIVITY_STATE_ACTIVE, snapshot.state);
  CuAssertIntEquals(tc, 30, (int)snapshot.next_step_pulses);
  moved.character = damaged.target;
  moved.from_room = domain_entity_handle_none();
  moved.to_room = damaged.source;
  moved.direction = 0;
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK,
                    DOMAIN_EVENT_PUBLISH(fixture.bus, DOMAIN_EVENT_CHARACTER_MOVED, &moved));
  CuAssertTrue(tc, !primary_activity_snapshot(&fixture.actor, &snapshot));
  CuAssertIntEquals(tc, 0, event_queue_depth());

  CuAssertTrue(tc, activity_test_start(&fixture, &definition));
  extracted.entity = domain_event_character_handle(&fixture.target);
  extracted.reason = 0U;
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK,
                    DOMAIN_EVENT_PUBLISH(fixture.bus, DOMAIN_EVENT_ENTITY_EXTRACTED, &extracted));
  CuAssertTrue(tc, !primary_activity_snapshot(&fixture.actor, &snapshot));
  CuAssertIntEquals(tc, 0, event_queue_depth());

  definition.damage_response = PRIMARY_ACTIVITY_RESPONSE_RECHECK;
  fixture.context.recheck_allowed = true;
  CuAssertTrue(tc, activity_test_start(&fixture, &definition));
  fixture.context.recheck_allowed = false;
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK,
                    DOMAIN_EVENT_PUBLISH(fixture.bus, DOMAIN_EVENT_CHARACTER_DAMAGED, &damaged));
  CuAssertTrue(tc, !primary_activity_snapshot(&fixture.actor, &snapshot));
  CuAssertIntEquals(tc, 0, event_queue_depth());

  fixture.context.recheck_allowed = true;
  fixture.context.cancel_during_recheck = true;
  CuAssertTrue(tc, activity_test_start(&fixture, &definition));
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK,
                    DOMAIN_EVENT_PUBLISH(fixture.bus, DOMAIN_EVENT_CHARACTER_DAMAGED, &damaged));
  CuAssertTrue(tc, !primary_activity_snapshot(&fixture.actor, &snapshot));
  CuAssertIntEquals(tc, 0, event_queue_depth());
  activity_test_end(&fixture);
}

void Test_primary_activity_semantic_turn_requires_existing_action_budget(CuTest *tc)
{
  struct activity_test_fixture fixture;
  struct primary_activity_definition definition;
  struct primary_activity_snapshot snapshot;

  activity_test_begin(tc, &fixture);
  definition = activity_test_definition(&fixture);
  definition.total_steps = 1U;
  definition.combat_response = PRIMARY_ACTIVITY_RESPONSE_IGNORE;
  FIGHTING(&fixture.actor) = &fixture.target;
  CuAssertTrue(tc, activity_test_start(&fixture, &definition));
  CuAssertIntEquals(tc, 0, event_queue_depth());
  start_action_cooldown(&fixture.actor, atSTANDARD, 6 RL_SEC);
  CuAssertTrue(tc, !is_action_available(&fixture.actor, atSTANDARD, false));
  primary_activity_on_semantic_turn(&fixture.actor);
  CuAssertTrue(tc, primary_activity_snapshot(&fixture.actor, &snapshot));
  CuAssertIntEquals(tc, 0, (int)snapshot.completed_steps);
  event_cancel_specific(&fixture.actor, eSTANDARDACTION);
  CuAssertTrue(tc, is_action_available(&fixture.actor, atSTANDARD, false));
  primary_activity_on_semantic_turn(&fixture.actor);
  CuAssertTrue(tc, !primary_activity_snapshot(&fixture.actor, &snapshot));
  CuAssertIntEquals(tc, 1, (int)fixture.context.completion_calls);
  CuAssertIntEquals(tc, 2, (int)fixture.context.recheck_calls);
  CuAssertTrue(tc, !is_action_available(&fixture.actor, atSTANDARD, false));
  CuAssertIntEquals(tc, 1, event_queue_depth());
  FIGHTING(&fixture.actor) = NULL;
  activity_test_end(&fixture);
}

void Test_primary_activity_start_in_combat_honors_declared_policy(CuTest *tc)
{
  struct activity_test_fixture fixture;
  struct primary_activity_definition definition;
  struct primary_activity_snapshot snapshot;

  activity_test_begin(tc, &fixture);
  definition = activity_test_definition(&fixture);
  FIGHTING(&fixture.actor) = &fixture.target;

  definition.combat_response = PRIMARY_ACTIVITY_RESPONSE_REJECT;
  CuAssertTrue(tc, !activity_test_start(&fixture, &definition));
  definition.combat_response = PRIMARY_ACTIVITY_RESPONSE_CANCEL;
  CuAssertTrue(tc, !activity_test_start(&fixture, &definition));

  definition.combat_response = PRIMARY_ACTIVITY_RESPONSE_RECHECK;
  fixture.context.recheck_allowed = false;
  CuAssertTrue(tc, !activity_test_start(&fixture, &definition));
  fixture.context.recheck_allowed = true;

  definition.combat_response = PRIMARY_ACTIVITY_RESPONSE_PAUSE;
  CuAssertTrue(tc, activity_test_start(&fixture, &definition));
  CuAssertTrue(tc, primary_activity_snapshot(&fixture.actor, &snapshot));
  CuAssertIntEquals(tc, PRIMARY_ACTIVITY_STATE_PAUSED, snapshot.state);
  CuAssertIntEquals(tc, 0, event_queue_depth());
  CuAssertTrue(tc, primary_activity_cancel(&fixture.actor,
                                           PRIMARY_ACTIVITY_END_PLAYER_CANCELLED, false));

  definition.combat_response = PRIMARY_ACTIVITY_RESPONSE_DELAY;
  definition.total_steps = 1U;
  CuAssertTrue(tc, activity_test_start(&fixture, &definition));
  primary_activity_on_semantic_turn(&fixture.actor);
  CuAssertTrue(tc, primary_activity_snapshot(&fixture.actor, &snapshot));
  CuAssertIntEquals(tc, 0, (int)snapshot.completed_steps);
  primary_activity_on_semantic_turn(&fixture.actor);
  CuAssertTrue(tc, !primary_activity_snapshot(&fixture.actor, &snapshot));
  CuAssertIntEquals(tc, 1, (int)fixture.context.completion_calls);

  FIGHTING(&fixture.actor) = NULL;
  activity_test_end(&fixture);
}

void Test_primary_activity_can_pause_and_resume_from_progress_callback(CuTest *tc)
{
  struct activity_test_fixture fixture;
  struct primary_activity_definition definition;
  struct primary_activity_snapshot snapshot;

  activity_test_begin(tc, &fixture);
  definition = activity_test_definition(&fixture);
  fixture.context.pause_during_progress = true;
  CuAssertTrue(tc, activity_test_start(&fixture, &definition));
  pulse += 10U;
  event_process();
  CuAssertTrue(tc, primary_activity_snapshot(&fixture.actor, &snapshot));
  CuAssertIntEquals(tc, PRIMARY_ACTIVITY_STATE_PAUSED, snapshot.state);
  CuAssertIntEquals(tc, 1, (int)snapshot.completed_steps);
  CuAssertIntEquals(tc, 0, event_queue_depth());

  fixture.context.pause_during_progress = false;
  CuAssertTrue(tc, primary_activity_resume(&fixture.actor, false));
  CuAssertIntEquals(tc, 1, event_queue_depth());
  pulse += 10U;
  event_process();
  CuAssertTrue(tc, !primary_activity_snapshot(&fixture.actor, &snapshot));
  CuAssertIntEquals(tc, 1, (int)fixture.context.completion_calls);
  activity_test_end(&fixture);
}

void Test_activity_status_wraps_for_default_mud_width(CuTest *tc)
{
  struct activity_test_fixture fixture;
  struct primary_activity_definition definition;
  struct primary_activity_snapshot snapshot;
  const char *line;
  const char *end;

  activity_test_begin(tc, &fixture);
  definition = activity_test_definition(&fixture);
  definition.capabilities = PRIMARY_ACTIVITY_CAP_MOVEMENT | PRIMARY_ACTIVITY_CAP_HANDS |
                            PRIMARY_ACTIVITY_CAP_ATTENTION | PRIMARY_ACTIVITY_CAP_VISION |
                            PRIMARY_ACTIVITY_CAP_SPEECH | PRIMARY_ACTIVITY_CAP_STANDARD |
                            PRIMARY_ACTIVITY_CAP_MOVE | PRIMARY_ACTIVITY_CAP_SWIFT |
                            PRIMARY_ACTIVITY_CAP_IMMEDIATE;
  definition.traits = PRIMARY_ACTIVITY_TRAIT_STATIONARY |
                      PRIMARY_ACTIVITY_TRAIT_DISTRACTED |
                      PRIMARY_ACTIVITY_TRAIT_HANDS_OCCUPIED |
                      PRIMARY_ACTIVITY_TRAIT_FINE_MANIPULATION |
                      PRIMARY_ACTIVITY_TRAIT_OBVIOUS;
  CuAssertTrue(tc, activity_test_start(&fixture, &definition));
  do_activity(&fixture.actor, "", 0, 0);

  line = fixture.descriptor.output;
  while (*line != '\0')
  {
    end = strstr(line, "\r\n");
    if (end == NULL)
      end = line + strlen(line);
    CuAssertTrue(tc, (size_t)(end - line) <= 80U);
    line = *end == '\0' ? end : end + 2;
  }
  do_activity(&fixture.actor, "pause", 0, 0);
  CuAssertTrue(tc, primary_activity_snapshot(&fixture.actor, &snapshot));
  CuAssertIntEquals(tc, PRIMARY_ACTIVITY_STATE_PAUSED, snapshot.state);
  do_activity(&fixture.actor, "resume", 0, 0);
  CuAssertTrue(tc, primary_activity_snapshot(&fixture.actor, &snapshot));
  CuAssertIntEquals(tc, PRIMARY_ACTIVITY_STATE_ACTIVE, snapshot.state);
  do_activity(&fixture.actor, "cancel", 0, 0);
  CuAssertTrue(tc, !primary_activity_snapshot(&fixture.actor, &snapshot));
  activity_test_end(&fixture);
}

void Test_primary_activity_turn_hook_is_semantic_only(CuTest *tc)
{
  CuAssertTrue(tc, activity_source_places_turn_hook_in_semantic_round());
}
