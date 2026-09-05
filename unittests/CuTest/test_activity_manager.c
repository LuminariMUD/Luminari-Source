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
#include "../../src/event_runtime.h"
#include "../../src/interpreter.h"
#include "../../src/mud_event.h"
#include "../../src/net/protocol.h"

#include "../../src/magic/spells.h"
#include "../../src/magic/spell_prep.h"
#include "../../src/magic/domains_schools.h"
#include "../../src/character/class.h"
#include "../../src/handler.h"

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
  unsigned int damage_calls;
  int damage_amount;
  int damage_type;
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
  uint64_t terminal_id;
  uint32_t terminal_reason;
  uint32_t terminal_state;
};

static bool activity_native_type_is_registered(const char *name)
{
  struct game_scheduler_stats stats;
  game_event_type_id_t event_type;

  event_runtime_get_stats(&stats);
  for (event_type = 1U; event_type <= stats.registered_type_count; event_type++)
    if (event_runtime_type_name(event_type) != NULL &&
        !strcmp(event_runtime_type_name(event_type), name))
      return true;
  return false;
}

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
  fixture->terminal_id = event->activity_id;
  fixture->terminal_reason = event->end_reason;
  fixture->terminal_state = event->current_state;
  fixture->terminal_transition_saw_activity = primary_activity_snapshot(&fixture->actor, &snapshot);
  fixture->terminal_transition_cancelled_activity =
      primary_activity_cancel(&fixture->actor, PRIMARY_ACTIVITY_END_PLAYER_CANCELLED, false);
}

static void activity_test_progress(struct char_data *actor, void *target, uint32_t completed_steps,
                                   uint32_t total_steps, void *context)
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

static void activity_test_ended(struct char_data *actor, enum primary_activity_end_reason reason,
                                void *context)
{
  struct activity_test_context *trace = context;

  (void)actor;
  (void)reason;
  trace->ended_calls++;
}

static void activity_test_begin(CuTest *tc, struct activity_test_fixture *fixture)
{
  struct domain_event_handler_config observer = {DOMAIN_EVENT_ACTIVITY_TRANSITIONED,
                                                 "test.activity.transition", -100,
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
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK, domain_event_register_foundation_types(fixture->bus));
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK, domain_event_world_register_resolvers(fixture->bus));
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK, primary_activity_manager_init(fixture->bus));
  CuAssertTrue(tc, activity_native_type_is_registered("activity.primary.step"));
  observer.handler_context = fixture;
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK, domain_event_register_handler(fixture->bus, &observer));
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
  definition.capabilities = PRIMARY_ACTIVITY_CAP_MOVEMENT | PRIMARY_ACTIVITY_CAP_ATTENTION |
                            PRIMARY_ACTIVITY_CAP_STANDARD;
  definition.traits = PRIMARY_ACTIVITY_TRAIT_STATIONARY | PRIMARY_ACTIVITY_TRAIT_DISTRACTED;
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
  return primary_activity_start(&fixture->actor, domain_event_character_handle(&fixture->target),
                                definition);
}

void Test_primary_activity_camp_selector_defaults_managed_and_requires_explicit_rollback(CuTest *tc)
{
  CuAssertTrue(tc, primary_activity_test_camp_value_is_managed(NULL));
  CuAssertTrue(tc, primary_activity_test_camp_value_is_managed(""));
  CuAssertTrue(tc, primary_activity_test_camp_value_is_managed("managed"));
  CuAssertTrue(tc, !primary_activity_test_camp_value_is_managed("legacy"));
  CuAssertTrue(tc, !primary_activity_test_camp_value_is_managed("off"));
  CuAssertTrue(tc, !primary_activity_test_camp_value_is_managed("0"));
}

void Test_primary_activity_scheduler_registers_timer_when_camp_is_unmanaged(CuTest *tc)
{
  struct activity_test_fixture fixture;
  struct primary_activity_definition definition;

  primary_activity_test_select_camp(false);
  activity_test_begin(tc, &fixture);
  definition = activity_test_definition(&fixture);
  CuAssertTrue(tc, activity_native_type_is_registered("activity.primary.step"));
  CuAssertTrue(tc, activity_test_start(&fixture, &definition));
  CuAssertIntEquals(tc, 1, event_queue_depth());
  activity_test_end(&fixture);
  primary_activity_test_select_camp(true);
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
  CuAssertTrue(tc, primary_activity_command_admit(&fixture.actor, "look",
                                                  PRIMARY_ACTIVITY_CAP_ATTENTION, true, false));
  CuAssertTrue(tc, primary_activity_snapshot(&fixture.actor, &snapshot));
  CuAssertTrue(tc, primary_activity_command_admit(&fixture.actor, "say",
                                                  PRIMARY_ACTIVITY_CAP_SPEECH, false, false));
  CuAssertTrue(tc, !primary_activity_command_admit(&fixture.actor, "cast",
                                                   PRIMARY_ACTIVITY_CAP_ATTENTION, false, false));
  CuAssertTrue(tc, primary_activity_snapshot(&fixture.actor, &snapshot));
  CuAssertTrue(tc, primary_activity_command_admit(&fixture.actor, "north",
                                                  PRIMARY_ACTIVITY_CAP_MOVEMENT, false, false));
  CuAssertTrue(tc, primary_activity_snapshot(&fixture.actor, &snapshot));
  memset(&moved, 0, sizeof(moved));
  moved.character = domain_event_character_handle(&fixture.actor);
  moved.from_room.kind = DOMAIN_ENTITY_ROOM;
  moved.from_room.runtime_id = 10U;
  moved.from_room.generation = 1U;
  moved.to_room.kind = DOMAIN_ENTITY_ROOM;
  moved.to_room.runtime_id = 11U;
  moved.to_room.generation = 1U;
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK,
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
  event_test_advance();
  CuAssertTrue(tc, primary_activity_snapshot(&fixture.actor, &snapshot));
  CuAssertIntEquals(tc, 1, (int)snapshot.completed_steps);
  CuAssertIntEquals(tc, 1, (int)fixture.context.progress_calls);
  pulse += 10U;
  event_test_advance();
  CuAssertTrue(tc, !primary_activity_snapshot(&fixture.actor, &snapshot));
  CuAssertIntEquals(tc, 1, (int)fixture.context.completion_calls);
  CuAssertIntEquals(tc, 0, (int)fixture.context.ended_calls);
  CuAssertIntEquals(tc, 1, (int)fixture.terminal_transition_calls);
  CuAssertTrue(tc, !fixture.terminal_transition_saw_activity);
  CuAssertTrue(tc, !fixture.terminal_transition_cancelled_activity);
  CuAssertIntEquals(tc, 0, event_queue_depth());
  pulse += 100U;
  event_test_advance();
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
  CuAssertTrue(
      tc, primary_activity_cancel(&fixture.actor, PRIMARY_ACTIVITY_END_PLAYER_CANCELLED, false));

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
  event_test_advance();
  CuAssertTrue(tc, primary_activity_snapshot(&fixture.actor, &snapshot));
  CuAssertIntEquals(tc, PRIMARY_ACTIVITY_STATE_PAUSED, snapshot.state);
  CuAssertIntEquals(tc, 1, (int)snapshot.completed_steps);
  CuAssertIntEquals(tc, 0, event_queue_depth());

  fixture.context.pause_during_progress = false;
  CuAssertTrue(tc, primary_activity_resume(&fixture.actor, false));
  CuAssertIntEquals(tc, 1, event_queue_depth());
  pulse += 10U;
  event_test_advance();
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
  definition.capabilities =
      PRIMARY_ACTIVITY_CAP_MOVEMENT | PRIMARY_ACTIVITY_CAP_HANDS | PRIMARY_ACTIVITY_CAP_ATTENTION |
      PRIMARY_ACTIVITY_CAP_VISION | PRIMARY_ACTIVITY_CAP_SPEECH | PRIMARY_ACTIVITY_CAP_STANDARD |
      PRIMARY_ACTIVITY_CAP_MOVE | PRIMARY_ACTIVITY_CAP_SWIFT | PRIMARY_ACTIVITY_CAP_IMMEDIATE;
  definition.traits = PRIMARY_ACTIVITY_TRAIT_STATIONARY | PRIMARY_ACTIVITY_TRAIT_DISTRACTED |
                      PRIMARY_ACTIVITY_TRAIT_HANDS_OCCUPIED |
                      PRIMARY_ACTIVITY_TRAIT_FINE_MANIPULATION | PRIMARY_ACTIVITY_TRAIT_OBVIOUS;
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

/* Exercise the production cast entry point and native scheduler with real spell data. */
struct casting_test_fixture
{
  struct activity_test_fixture activity;
  struct room_data rooms[2];
  struct player_special_data specials[2];
  struct room_data *saved_world;
  room_rnum saved_top;
  int saved_mode;
  int saved_divine_prep;
  struct spell_info_type saved_spell;
};

static void casting_test_begin(CuTest *tc, struct casting_test_fixture *fixture)
{
  struct char_data *actor;
  struct char_data *target;

  memset(fixture, 0, sizeof(*fixture));
  fixture->saved_world = world;
  fixture->saved_top = top_of_world;
  fixture->saved_mode = CONFIG_SPELLCASTING_TIME_MODE;
  fixture->saved_divine_prep = CONFIG_DIVINE_PREP_TIME;
  CONFIG_DIVINE_PREP_TIME = 1;
  world = fixture->rooms;
  top_of_world = 1;
  fixture->rooms[0].number = 100;
  fixture->rooms[1].number = 101;
  CONFIG_SPELLCASTING_TIME_MODE = 1;
  activity_test_begin(tc, &fixture->activity);
  actor = &fixture->activity.actor;
  target = &fixture->activity.target;
  actor->player_specials = &fixture->specials[0];
  target->player_specials = &fixture->specials[1];
  SET_BIT_AR(MOB_FLAGS(actor), MOB_ISNPC);
  SET_BIT_AR(MOB_FLAGS(target), MOB_ISNPC);
  GET_CLASS(actor) = CLASS_CLERIC;
  GET_LEVEL(actor) = 10;
  GET_LEVEL(target) = 10;
  GET_POS(actor) = POS_STANDING;
  GET_POS(target) = POS_STANDING;
  GET_HIT(actor) = GET_HIT(target) = 10;
  GET_MAX_HIT(actor) = GET_MAX_HIT(target) = 100;
  IN_ROOM(actor) = IN_ROOM(target) = 0;
  actor->player.short_descr = "the caster";
  target->player.short_descr = "the target";
  fixture->saved_spell = spell_info[SPELL_CURE_LIGHT];
  memset(&spell_info[SPELL_CURE_LIGHT], 0, sizeof(spell_info[SPELL_CURE_LIGHT]));
  spell_info[SPELL_CURE_LIGHT].name = "cure light";
  spell_info[SPELL_CURE_LIGHT].min_position = POS_FIGHTING;
  spell_info[SPELL_CURE_LIGHT].targets = TAR_CHAR_ROOM;
  spell_info[SPELL_CURE_LIGHT].routines = MAG_POINTS;
  spell_info[SPELL_CURE_LIGHT].time = 1;
  spell_info[SPELL_CURE_LIGHT].schoolOfMagic = CONJURATION;
}

static void casting_test_end(struct casting_test_fixture *fixture)
{
  struct char_data *actor = &fixture->activity.actor;

  activity_test_end(&fixture->activity);
  if (actor->events != NULL)
  {
    free_list(actor->events);
    actor->events = NULL;
  }
  domain_event_world_forget_character(actor);
  domain_event_world_forget_character(&fixture->activity.target);
  world = fixture->saved_world;
  top_of_world = fixture->saved_top;
  CONFIG_SPELLCASTING_TIME_MODE = fixture->saved_mode;
  CONFIG_DIVINE_PREP_TIME = fixture->saved_divine_prep;
  spell_info[SPELL_CURE_LIGHT] = fixture->saved_spell;
}

static void casting_test_advance(unsigned int ticks)
{
  unsigned int index;

  for (index = 0U; index < ticks; index++)
  {
    pulse++;
    event_test_advance();
  }
}

static void casting_test_start(CuTest *tc, struct casting_test_fixture *fixture)
{
  CuAssertIntEquals(tc, 1,
                    cast_spell(&fixture->activity.actor, &fixture->activity.target, NULL,
                               SPELL_CURE_LIGHT, METAMAGIC_NONE));
  CuAssertTrue(tc, IS_CASTING(&fixture->activity.actor));
}

struct casting_start_trace
{
  unsigned int calls;
  struct domain_casting_started event;
};

static void casting_start_observe(const struct domain_event_context *context, void *data)
{
  struct casting_start_trace *trace = data;

  trace->calls++;
  trace->event = *(const struct domain_casting_started *)context->payload;
}

void Test_casting_start_is_scoped_once_and_identifies_each_committed_cast(CuTest *tc)
{
  struct casting_test_fixture fixture;
  struct casting_start_trace trace = {0};
  struct domain_event_subscription_config config = {0};
  struct domain_event_subscription_handle subscription;
  struct primary_activity_snapshot snapshot;
  uint64_t first_id;

  casting_test_begin(tc, &fixture);
  config.type = DOMAIN_EVENT_CASTING_STARTED;
  config.identity = "test.casting.started";
  config.topic.role = DOMAIN_EVENT_TOPIC_SUBJECT;
  config.topic.entity = domain_event_character_handle(&fixture.activity.actor);
  config.owner = domain_event_character_handle(&fixture.activity.target);
  config.handler = casting_start_observe;
  config.handler_context = &trace;
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK,
                    domain_event_subscribe(fixture.activity.bus, &config, &subscription));
  casting_test_start(tc, &fixture);
  CuAssertIntEquals(tc, 1, trace.calls);
  CuAssertTrue(tc, primary_activity_snapshot(&fixture.activity.actor, &snapshot));
  CuAssertTrue(tc, trace.event.cast_id == snapshot.id);
  CuAssertTrue(tc, domain_entity_handle_equal(trace.event.caster, config.topic.entity));
  CuAssertTrue(tc, domain_entity_handle_equal(trace.event.target, config.owner));
  CuAssertTrue(tc, domain_entity_handle_equal(trace.event.room, domain_event_room_handle(0)));
  CuAssertIntEquals(tc, SPELL_CURE_LIGHT, trace.event.spellnum);
  first_id = trace.event.cast_id;
  CuAssertIntEquals(tc, 0,
                    cast_spell(&fixture.activity.actor, &fixture.activity.target, NULL,
                               SPELL_CURE_LIGHT, METAMAGIC_NONE));
  CuAssertIntEquals(tc, 1, trace.calls);
  resetCastingData(&fixture.activity.actor);
  casting_test_start(tc, &fixture);
  CuAssertIntEquals(tc, 2, trace.calls);
  CuAssertTrue(tc, first_id != trace.event.cast_id);
  casting_test_advance(200);
  CuAssertIntEquals(tc, 2, trace.calls);
  casting_test_end(&fixture);
}

void Test_casting_activity_completes_once_on_native_clock_in_combat(CuTest *tc)
{
  struct casting_test_fixture fixture;
  struct primary_activity_snapshot snapshot;
  struct domain_combat_state_changed combat;
  int healed;

  casting_test_begin(tc, &fixture);
  casting_test_start(tc, &fixture);
  CuAssertTrue(tc, primary_activity_snapshot(&fixture.activity.actor, &snapshot));
  CuAssertIntEquals(tc, PRIMARY_ACTIVITY_CASTING, snapshot.type);
  CuAssertIntEquals(tc, 2 * PASSES_PER_SEC, snapshot.next_step_pulses);
  CuAssertTrue(tc, !primary_activity_pause(&fixture.activity.actor, false));
  combat.character = domain_event_character_handle(&fixture.activity.actor);
  combat.opponent = domain_event_character_handle(&fixture.activity.target);
  combat.in_combat = true;
  FIGHTING(&fixture.activity.actor) = &fixture.activity.target;
  CuAssertIntEquals(
      tc, DOMAIN_EVENT_OK,
      DOMAIN_EVENT_PUBLISH(fixture.activity.bus, DOMAIN_EVENT_COMBAT_STATE_CHANGED, &combat));
  primary_activity_on_semantic_turn(&fixture.activity.actor);
  CuAssertIntEquals(tc, 10, GET_HIT(&fixture.activity.target));
  casting_test_advance(200);
  healed = GET_HIT(&fixture.activity.target);
  CuAssertTrue(tc, healed > 10);
  CuAssertTrue(tc, !IS_CASTING(&fixture.activity.actor));
  CuAssertIntEquals(tc, 1, fixture.activity.terminal_transition_calls);
  CuAssertIntEquals(tc, PRIMARY_ACTIVITY_STATE_COMPLETED, fixture.activity.terminal_state);
  CuAssertTrue(tc, snapshot.id == fixture.activity.terminal_id);
  casting_test_advance(200);
  CuAssertIntEquals(tc, healed, GET_HIT(&fixture.activity.target));
  casting_test_end(&fixture);
}

void Test_casting_damage_interrupts_outside_combat_and_cannot_resolve_later(CuTest *tc)
{
  struct casting_test_fixture fixture;
  struct domain_character_damaged damage;

  casting_test_begin(tc, &fixture);
  casting_test_start(tc, &fixture);
  damage.target = domain_event_character_handle(&fixture.activity.actor);
  damage.source = domain_event_character_handle(&fixture.activity.target);
  damage.amount = 0;
  damage.damage_type = 0;
  CuAssertIntEquals(
      tc, DOMAIN_EVENT_OK,
      DOMAIN_EVENT_PUBLISH(fixture.activity.bus, DOMAIN_EVENT_CHARACTER_DAMAGED, &damage));
  CuAssertTrue(tc, IS_CASTING(&fixture.activity.actor));
  damage.amount = 10000; /* Beyond any possible roll, without depending on random state. */
  CuAssertIntEquals(
      tc, DOMAIN_EVENT_OK,
      DOMAIN_EVENT_PUBLISH(fixture.activity.bus, DOMAIN_EVENT_CHARACTER_DAMAGED, &damage));
  CuAssertTrue(tc, !IS_CASTING(&fixture.activity.actor));
  CuAssertIntEquals(tc, PRIMARY_ACTIVITY_END_DAMAGED, fixture.activity.terminal_reason);
  CuAssertIntEquals(tc, 1, fixture.activity.terminal_transition_calls);
  casting_test_advance(200);
  CuAssertIntEquals(tc, 10, GET_HIT(&fixture.activity.target));
  CuAssertIntEquals(
      tc, DOMAIN_EVENT_OK,
      DOMAIN_EVENT_PUBLISH(fixture.activity.bus, DOMAIN_EVENT_CHARACTER_DAMAGED, &damage));
  CuAssertIntEquals(tc, 1, fixture.activity.terminal_transition_calls);
  casting_test_end(&fixture);
}

void Test_casting_reset_cancels_owner_and_allows_immediate_recast(CuTest *tc)
{
  struct casting_test_fixture fixture;
  struct primary_activity_snapshot first, second;

  casting_test_begin(tc, &fixture);
  casting_test_start(tc, &fixture);
  CuAssertTrue(tc, primary_activity_snapshot(&fixture.activity.actor, &first));
  resetCastingData(&fixture.activity.actor);
  CuAssertTrue(tc, !primary_activity_snapshot(&fixture.activity.actor, &second));
  casting_test_start(tc, &fixture);
  CuAssertTrue(tc, primary_activity_snapshot(&fixture.activity.actor, &second));
  CuAssertTrue(tc, first.id != second.id);
  casting_test_advance(200);
  CuAssertIntEquals(tc, 2, fixture.activity.terminal_transition_calls);
  CuAssertTrue(tc, second.id == fixture.activity.terminal_id);
  casting_test_end(&fixture);
}

void Test_casting_target_move_and_death_cancel_scoped_activity(CuTest *tc)
{
  struct casting_test_fixture fixture;
  struct domain_character_moved moved;
  struct domain_character_died died = {0};
  struct domain_event_topic topic;

  casting_test_begin(tc, &fixture);
  casting_test_start(tc, &fixture);
  moved.character = domain_event_character_handle(&fixture.activity.target);
  moved.from_room = domain_event_room_handle(0);
  moved.to_room = domain_event_room_handle(1);
  moved.direction = -1;
  topic.role = DOMAIN_EVENT_TOPIC_SUBJECT;
  topic.entity = moved.character;
  IN_ROOM(&fixture.activity.target) = 1;
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK,
                    DOMAIN_EVENT_PUBLISH_ROUTED(fixture.activity.bus, DOMAIN_EVENT_CHARACTER_MOVED,
                                                &topic, 1U, &moved));
  CuAssertTrue(tc, !IS_CASTING(&fixture.activity.actor));
  IN_ROOM(&fixture.activity.target) = 0;
  casting_test_start(tc, &fixture);
  died.character = moved.character;
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK,
                    DOMAIN_EVENT_PUBLISH_ROUTED(fixture.activity.bus, DOMAIN_EVENT_CHARACTER_DIED,
                                                &topic, 1U, &died));
  CuAssertTrue(tc, !IS_CASTING(&fixture.activity.actor));
  CuAssertIntEquals(tc, PRIMARY_ACTIVITY_END_TARGET_LOST, fixture.activity.terminal_reason);
  casting_test_advance(200);
  CuAssertIntEquals(tc, 10, GET_HIT(&fixture.activity.target));
  casting_test_end(&fixture);
}

void Test_casting_movement_and_shutdown_clear_transient_state(CuTest *tc)
{
  struct casting_test_fixture fixture;
  struct domain_character_moved moved;

  casting_test_begin(tc, &fixture);
  casting_test_start(tc, &fixture);
  moved.character = domain_event_character_handle(&fixture.activity.actor);
  moved.from_room = domain_event_room_handle(0);
  moved.to_room = domain_event_room_handle(1);
  moved.direction = -1;
  CuAssertIntEquals(
      tc, DOMAIN_EVENT_OK,
      DOMAIN_EVENT_PUBLISH(fixture.activity.bus, DOMAIN_EVENT_CHARACTER_MOVED, &moved));
  CuAssertTrue(tc, !IS_CASTING(&fixture.activity.actor));
  CuAssertIntEquals(tc, PRIMARY_ACTIVITY_END_MOVED, fixture.activity.terminal_reason);
  casting_test_start(tc, &fixture);
  primary_activity_manager_shutdown();
  CuAssertTrue(tc, !IS_CASTING(&fixture.activity.actor));
  CuAssertPtrEquals(tc, NULL, CASTING_TCH(&fixture.activity.actor));
  CuAssertIntEquals(tc, PRIMARY_ACTIVITY_END_SHUTDOWN, fixture.activity.terminal_reason);
  casting_test_advance(200);
  CuAssertIntEquals(tc, 10, GET_HIT(&fixture.activity.target));
  casting_test_end(&fixture);
}

void Test_casting_player_spends_prepared_spell_once_and_cancel_does_not_refund(CuTest *tc)
{
  struct casting_test_fixture fixture;
  struct char_data *actor;
  struct primary_activity_snapshot snapshot;

  casting_test_begin(tc, &fixture);
  actor = &fixture.activity.actor;
  REMOVE_BIT_AR(MOB_FLAGS(actor), MOB_ISNPC);
  CLASS_LEVEL(actor, CLASS_CLERIC) = 10;
  CASTING_CLASS(actor) = CLASS_CLERIC;
  collection_add(actor, CLASS_CLERIC, SPELL_CURE_LIGHT, 0, 0, 0);
  casting_test_start(tc, &fixture);
  CuAssertTrue(tc, primary_activity_snapshot(actor, &snapshot));
  CuAssertIntEquals(tc, PASSES_PER_SEC, snapshot.next_step_pulses);
  CuAssertTrue(tc, !is_spell_in_collection(actor, CLASS_CLERIC, SPELL_CURE_LIGHT, 0));
  collection_add(actor, CLASS_CLERIC, SPELL_CURE_LIGHT, 0, 0, 0);
  CuAssertIntEquals(tc, 0, cast_spell(actor, &fixture.activity.target, NULL, SPELL_CURE_LIGHT, 0));
  CuAssertTrue(tc, is_spell_in_collection(actor, CLASS_CLERIC, SPELL_CURE_LIGHT, 0));
  CuAssertTrue(tc, primary_activity_cancel(actor, PRIMARY_ACTIVITY_END_PLAYER_CANCELLED, false));
  casting_test_advance(100);
  CuAssertIntEquals(tc, 10, GET_HIT(&fixture.activity.target));
  casting_test_start(tc, &fixture);
  casting_test_advance(100);
  CuAssertTrue(tc, !is_spell_in_collection(actor, CLASS_CLERIC, SPELL_CURE_LIGHT, 0));
  CuAssertTrue(tc, GET_HIT(&fixture.activity.target) > 10);
  clear_collection_by_class(actor, CLASS_CLERIC);
  clear_prep_queue_by_class(actor, CLASS_CLERIC);
  casting_test_end(&fixture);
}

void Test_casting_damage_on_final_deadline_prevents_resolution(CuTest *tc)
{
  struct casting_test_fixture fixture;
  struct domain_character_damaged damage = {0};

  casting_test_begin(tc, &fixture);
  casting_test_start(tc, &fixture);
  casting_test_advance(2 * PASSES_PER_SEC);
  CuAssertIntEquals(tc, 1, CASTING_TIME(&fixture.activity.actor));
  /* The final tick is due now, but damage commits before scheduler dispatch. */
  pulse += 10;
  damage.target = domain_event_character_handle(&fixture.activity.actor);
  damage.amount = INT_MAX;
  CuAssertIntEquals(
      tc, DOMAIN_EVENT_OK,
      DOMAIN_EVENT_PUBLISH(fixture.activity.bus, DOMAIN_EVENT_CHARACTER_DAMAGED, &damage));
  event_test_advance();
  CuAssertIntEquals(tc, 10, GET_HIT(&fixture.activity.target));
  CuAssertIntEquals(tc, 1, fixture.activity.terminal_transition_calls);
  CuAssertIntEquals(tc, PRIMARY_ACTIVITY_END_DAMAGED, fixture.activity.terminal_reason);
  casting_test_end(&fixture);
}

void Test_casting_existing_concentration_exemptions_survive_damage(CuTest *tc)
{
  struct casting_test_fixture fixture;
  struct domain_character_damaged damage = {0};
  int classes[] = {CLASS_ALCHEMIST, CLASS_SHADOW_DANCER};
  size_t index;

  for (index = 0; index < sizeof(classes) / sizeof(classes[0]); index++)
  {
    casting_test_begin(tc, &fixture);
    casting_test_start(tc, &fixture);
    CASTING_CLASS(&fixture.activity.actor) = classes[index];
    damage.target = domain_event_character_handle(&fixture.activity.actor);
    damage.amount = 10000;
    CuAssertIntEquals(
        tc, DOMAIN_EVENT_OK,
        DOMAIN_EVENT_PUBLISH(fixture.activity.bus, DOMAIN_EVENT_CHARACTER_DAMAGED, &damage));
    CuAssertTrue(tc, IS_CASTING(&fixture.activity.actor));
    resetCastingData(&fixture.activity.actor);
    casting_test_end(&fixture);
  }
}

void Test_casting_incapacitation_and_extraction_do_not_leave_callbacks(CuTest *tc)
{
  struct casting_test_fixture fixture;
  struct domain_entity_extracted extracted;

  casting_test_begin(tc, &fixture);
  casting_test_start(tc, &fixture);
  SET_BIT_AR(AFF_FLAGS(&fixture.activity.actor), AFF_STUN);
  casting_test_advance(2 * PASSES_PER_SEC);
  CuAssertTrue(tc, !IS_CASTING(&fixture.activity.actor));
  CuAssertIntEquals(tc, PRIMARY_ACTIVITY_END_RECHECK_FAILED, fixture.activity.terminal_reason);
  REMOVE_BIT_AR(AFF_FLAGS(&fixture.activity.actor), AFF_STUN);
  casting_test_start(tc, &fixture);
  extracted.entity = domain_event_character_handle(&fixture.activity.target);
  extracted.reason = 0;
  CuAssertIntEquals(
      tc, DOMAIN_EVENT_OK,
      DOMAIN_EVENT_PUBLISH(fixture.activity.bus, DOMAIN_EVENT_ENTITY_EXTRACTED, &extracted));
  CuAssertTrue(tc, !IS_CASTING(&fixture.activity.actor));
  casting_test_start(tc, &fixture);
  primary_activity_forget_character(&fixture.activity.actor);
  CuAssertTrue(tc, !IS_CASTING(&fixture.activity.actor));
  casting_test_advance(200);
  CuAssertIntEquals(tc, 10, GET_HIT(&fixture.activity.target));
  CuAssertIntEquals(tc, 3, fixture.activity.terminal_transition_calls);
  casting_test_end(&fixture);
}

static bool activity_test_damage_context(struct char_data *actor,
                                         const struct domain_character_damaged *damage, void *data)
{
  struct activity_test_context *trace = data;

  (void)actor;
  trace->damage_calls++;
  trace->damage_amount = damage->amount;
  trace->damage_type = damage->damage_type;
  return true;
}

void Test_activity_damage_context_is_delivered_once_without_progress_rerolls(CuTest *tc)
{
  struct activity_test_fixture fixture;
  struct primary_activity_definition definition;
  struct domain_character_damaged damage = {0};

  activity_test_begin(tc, &fixture);
  definition = activity_test_definition(&fixture);
  definition.damage_check = activity_test_damage_context;
  CuAssertTrue(tc, activity_test_start(&fixture, &definition));
  damage.target = domain_event_character_handle(&fixture.actor);
  damage.source = domain_event_character_handle(&fixture.target);
  damage.amount = 7;
  damage.damage_type = 11;
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK,
                    DOMAIN_EVENT_PUBLISH(fixture.bus, DOMAIN_EVENT_CHARACTER_DAMAGED, &damage));
  CuAssertIntEquals(tc, 1, fixture.context.damage_calls);
  CuAssertIntEquals(tc, 7, fixture.context.damage_amount);
  CuAssertIntEquals(tc, 11, fixture.context.damage_type);
  casting_test_advance(30);
  CuAssertIntEquals(tc, 1, fixture.context.damage_calls);
  CuAssertIntEquals(tc, 1, fixture.context.completion_calls);
  activity_test_end(&fixture);
}

void Test_casting_successful_damage_check_keeps_original_deadline(CuTest *tc)
{
  struct casting_test_fixture fixture;
  struct domain_character_damaged damage = {0};
  struct primary_activity_snapshot before, after;

  casting_test_begin(tc, &fixture);
  GET_LEVEL(&fixture.activity.actor) = 100; /* NPC skill bonus guarantees success. */
  casting_test_start(tc, &fixture);
  CuAssertTrue(tc, primary_activity_snapshot(&fixture.activity.actor, &before));
  damage.target = domain_event_character_handle(&fixture.activity.actor);
  damage.amount = 1;
  CuAssertIntEquals(
      tc, DOMAIN_EVENT_OK,
      DOMAIN_EVENT_PUBLISH(fixture.activity.bus, DOMAIN_EVENT_CHARACTER_DAMAGED, &damage));
  CuAssertTrue(tc, primary_activity_snapshot(&fixture.activity.actor, &after));
  CuAssertTrue(tc, before.id == after.id);
  CuAssertIntEquals(tc, before.next_step_pulses, after.next_step_pulses);
  casting_test_advance(200);
  CuAssertIntEquals(tc, PRIMARY_ACTIVITY_STATE_COMPLETED, fixture.activity.terminal_state);
  casting_test_end(&fixture);
}
