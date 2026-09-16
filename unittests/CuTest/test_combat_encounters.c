#include "CuTest.h"

#include "conf.h"
#include "../../src/core/sysdep.h"
#include "../../src/core/structs.h"
#include "../../src/core/comm.h"
#include "../../src/core/db.h"
#include "../../src/act/act.h"
#include "../../src/events/actions.h"
#include "../../src/events/actionqueues.h"
#include "../../src/core/interpreter.h"
#include "../../src/combat/combat_encounters.h"
#include "../../src/combat/tactical_effects.h"
#include "../../src/combat/fight.h"
#include "../../src/dgscript/dg_event.h"
#include "../../src/events/domain_event_world.h"
#include "../../src/events/domain_event_types.h"
#include "../../src/events/event_runtime.h"
#include "../../src/net/protocol.h"
#include "../../src/core/screen.h"

#include <string.h>

#define ENCOUNTER_TRACE_CAPACITY 64U

static bool encounter_native_type_is_registered(const char *name)
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

struct encounter_test_trace
{
  struct char_data *characters[ENCOUNTER_TRACE_CAPACITY];
  unsigned int phases[ENCOUNTER_TRACE_CAPACITY];
  size_t count;
  struct char_data *mutation_trigger;
  struct char_data *mutation_character;
  struct char_data *mutation_opponent;
  enum combat_encounter_departure_reason mutation_reason;
  bool join_during_callback;
  bool vanish_during_callback;
  bool leave_only_mutation_character;
  bool rejoin_after_vanish;
  bool execute_queue;
  bool mutation_ran;
  bool mutation_succeeded;
};

struct semantic_boundary_trace
{
  struct char_data *first;
  struct char_data *second;
  bool second_flag_survived;
  bool second_reaction_remained_spent;
};

static void encounter_test_character(struct char_data *character, const char *name)
{
  clear_char(character);
  character->player.name = CuMutableString(name);
}

static unsigned long encounter_test_begin_mode(CuTest *tc, bool encounter_mode,
                                               bool semantic_rounds, unsigned long start_pulse,
                                               struct encounter_test_trace *trace)
{
  unsigned long saved_pulse = pulse;

  combat_encounter_runtime_shutdown();
  event_free_all();
  CuAssertIntEquals(tc, 1, event_test_select_backend(EVENT_BACKEND_GAME_SCHEDULER));
  pulse = start_pulse;
  event_init();
  combat_encounter_test_select(encounter_mode);
  combat_encounter_test_select_semantic(semantic_rounds);
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK, combat_encounter_runtime_init(NULL));
  if (encounter_mode)
    CuAssertTrue(tc, encounter_native_type_is_registered("combat.encounter.round"));
  if (trace != NULL)
  {
    memset(trace, 0, sizeof(*trace));
    combat_encounter_test_set_phase_callback(NULL, NULL);
  }
  return saved_pulse;
}

static unsigned long encounter_test_begin(CuTest *tc, bool encounter_mode,
                                          unsigned long start_pulse,
                                          struct encounter_test_trace *trace)
{
  return encounter_test_begin_mode(tc, encounter_mode, false, start_pulse, trace);
}

static unsigned long encounter_test_begin_semantic(CuTest *tc, unsigned long start_pulse,
                                                   struct encounter_test_trace *trace)
{
  return encounter_test_begin_mode(tc, true, true, start_pulse, trace);
}

static void encounter_test_end(unsigned long saved_pulse)
{
  combat_encounter_runtime_shutdown();
  event_free_all();
  pulse = saved_pulse;
}

static bool encounter_test_record_phase(struct char_data *character, unsigned int phase,
                                        void *context)
{
  struct encounter_test_trace *trace = context;

  if (trace->count < ENCOUNTER_TRACE_CAPACITY)
  {
    trace->characters[trace->count] = character;
    trace->phases[trace->count] = phase;
    trace->count++;
  }
  if (trace->execute_queue)
    execute_next_action(character);
  if (trace->mutation_ran || character != trace->mutation_trigger)
    return true;
  trace->mutation_ran = true;
  if (trace->join_during_callback)
  {
    FIGHTING(trace->mutation_character) = trace->mutation_opponent;
    trace->mutation_succeeded =
        combat_encounter_join(trace->mutation_character, trace->mutation_opponent, 1L);
  }
  if (trace->vanish_during_callback)
  {
    FIGHTING(trace->mutation_character) = NULL;
    FIGHTING(trace->mutation_opponent) = NULL;
    combat_encounter_leave(trace->mutation_character, trace->mutation_reason);
    if (!trace->leave_only_mutation_character)
      combat_encounter_leave(trace->mutation_opponent, trace->mutation_reason);
    trace->mutation_succeeded = true;
  }
  if (trace->rejoin_after_vanish)
  {
    FIGHTING(trace->mutation_character) = trace->mutation_opponent;
    trace->mutation_succeeded = combat_encounter_join(trace->mutation_character,
                                                      trace->mutation_opponent, ((long)(1 RL_SEC)));
  }
  return true;
}

static bool encounter_test_round_boundary_state(struct char_data *character, unsigned int phase,
                                                void *context)
{
  struct semantic_boundary_trace *trace = context;
  (void)phase;
  if (character == trace->first)
  {
    attach_mud_event(new_mud_event(eDEFLECTIVE_SCREEN_HIT_THIS_ROUND, trace->second, NULL),
                     ((long)(10 RL_SEC)));
    GET_TOTAL_AOO(trace->second) = 1;
  }
  else if (character == trace->second)
  {
    trace->second_flag_survived =
        char_has_mud_event(trace->second, eDEFLECTIVE_SCREEN_HIT_THIS_ROUND) != NULL;
    trace->second_reaction_remained_spent = GET_TOTAL_AOO(trace->second) == 1;
  }
  return true;
}

static void encounter_test_leave(struct char_data *character,
                                 enum combat_encounter_departure_reason reason)
{
  FIGHTING(character) = NULL;
  combat_encounter_leave(character, reason);
}

static void encounter_test_enqueue(struct char_data *character, const char *command)
{
  struct action_data *action;

  action = calloc(1U, sizeof(*action));
  action->argument = strdup(command);
  action->actions_required = ACTION_NONE;
  enqueue_action(GET_QUEUE(character), action);
}

/* The production selection must honor each actor's opening delay, then 1/2/3 phases. */
void Test_combat_restoration_default_preserves_individual_phase_deadlines(CuTest *tc)
{
  struct char_data first, second;
  struct encounter_test_trace trace = {0};
  struct combat_encounter_stats stats;
  unsigned long saved_pulse = pulse;
  const unsigned long start = 19000U;
  size_t before, at, after;
  unsigned int tick;

  encounter_test_character(&first, "early fighter");
  encounter_test_character(&second, "late fighter");
  combat_encounter_runtime_shutdown();
  event_free_all();
  pulse = start;
  event_init();
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK, combat_encounter_runtime_init(NULL));
  combat_encounter_test_set_phase_callback(encounter_test_record_phase, &trace);
  FIGHTING(&first) = &second;
  FIGHTING(&second) = &first;
  combat_encounter_join(&first, &second, ((long)(2 RL_SEC)));
  combat_encounter_join(&second, &first, ((long)(4 RL_SEC)));
  pulse = start + ((unsigned long)(2 RL_SEC)) - 1U;
  event_test_advance();
  before = trace.count;
  pulse++;
  event_test_advance();
  at = trace.count;
  pulse++;
  event_test_advance();
  after = trace.count;
  for (tick = 1U; tick <= (unsigned int)(6 RL_SEC) - 1U; tick++)
  {
    pulse++;
    event_test_advance();
  }
  combat_encounter_get_stats(&stats);
  encounter_test_leave(&first, COMBAT_ENCOUNTER_DEPARTURE_STOPPED);
  encounter_test_leave(&second, COMBAT_ENCOUNTER_DEPARTURE_STOPPED);
  encounter_test_end(saved_pulse);

  CuAssertIntEquals(tc, 0, (int)before);
  CuAssertIntEquals(tc, 1, (int)at);
  CuAssertIntEquals(tc, 1, (int)after);
  CuAssertIntEquals(tc, 7, (int)trace.count);
  CuAssertPtrEquals(tc, &first, trace.characters[0]);
  CuAssertIntEquals(tc, 1, (int)trace.phases[0]);
  /* Baseline queue_enq inserts before equal deadlines: newest scheduling wins ties. */
  CuAssertPtrEquals(tc, &first, trace.characters[1]);
  CuAssertIntEquals(tc, 2, (int)trace.phases[1]);
  CuAssertPtrEquals(tc, &second, trace.characters[2]);
  CuAssertIntEquals(tc, 1, (int)trace.phases[2]);
  CuAssertPtrEquals(tc, &second, trace.characters[3]);
  CuAssertIntEquals(tc, 2, (int)trace.phases[3]);
  CuAssertPtrEquals(tc, &first, trace.characters[4]);
  CuAssertIntEquals(tc, 3, (int)trace.phases[4]);
  CuAssertPtrEquals(tc, &first, trace.characters[5]);
  CuAssertIntEquals(tc, 1, (int)trace.phases[5]);
  CuAssertPtrEquals(tc, &second, trace.characters[6]);
  CuAssertIntEquals(tc, 3, (int)trace.phases[6]);
  CuAssertIntEquals(tc, 1, (int)stats.scheduled_events);
}

/* An off-grid action deadline must survive joining, leaving and joining again. */
void Test_combat_restoration_default_keeps_elapsed_action_deadline(CuTest *tc)
{
  struct char_data actor, target;
  struct encounter_test_trace trace = {0};
  unsigned long saved_pulse = pulse;
  const unsigned long start = 20000U;
  const unsigned long duration = (7 RL_SEC) + 3U;
  bool before, at, after;

  encounter_test_character(&actor, "cooldown fighter");
  encounter_test_character(&target, "cooldown opponent");
  combat_encounter_runtime_shutdown();
  event_free_all();
  pulse = start;
  event_init();
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK, combat_encounter_runtime_init(NULL));
  combat_encounter_test_set_phase_callback(encounter_test_record_phase, &trace);
  start_action_cooldown(&actor, atSTANDARD, (int)duration);
  pulse += ((unsigned long)(1 RL_SEC));
  FIGHTING(&actor) = &target;
  combat_encounter_join(&actor, &target, ((long)(2 RL_SEC)));
  pulse += ((unsigned long)(1 RL_SEC));
  encounter_test_leave(&actor, COMBAT_ENCOUNTER_DEPARTURE_STOPPED);
  encounter_test_leave(&target, COMBAT_ENCOUNTER_DEPARTURE_STOPPED);
  pulse += ((unsigned long)(1 RL_SEC));
  FIGHTING(&actor) = &target;
  combat_encounter_join(&actor, &target, ((long)(2 RL_SEC)));
  while (pulse < start + duration - 1U)
  {
    pulse++;
    event_test_advance();
  }
  before = is_action_available(&actor, atSTANDARD, false);
  pulse++;
  event_test_advance();
  at = is_action_available(&actor, atSTANDARD, false);
  pulse++;
  event_test_advance();
  after = is_action_available(&actor, atSTANDARD, false);
  encounter_test_leave(&actor, COMBAT_ENCOUNTER_DEPARTURE_STOPPED);
  encounter_test_leave(&target, COMBAT_ENCOUNTER_DEPARTURE_STOPPED);
  encounter_test_end(saved_pulse);
  if (actor.events != NULL)
    free_list(actor.events);

  CuAssertTrue(tc, !before);
  CuAssertTrue(tc, at);
  CuAssertTrue(tc, after);
}

void Test_combat_encounter_uses_one_event_and_preserves_compatibility_cadence(CuTest *tc)
{
  struct char_data first;
  struct char_data second;
  struct combat_encounter_stats stats;
  struct encounter_test_trace trace;
  unsigned long saved_pulse;
  const unsigned long start_pulse = 1000U;

  encounter_test_character(&first, "first combatant");
  encounter_test_character(&second, "second combatant");
  saved_pulse = encounter_test_begin(tc, true, start_pulse, &trace);
  combat_encounter_test_set_phase_callback(encounter_test_record_phase, &trace);
  FIGHTING(&first) = &second;
  FIGHTING(&second) = &first;

  CuAssertTrue(tc, combat_encounter_join(&first, &second, ((long)(2 RL_SEC))));
  CuAssertTrue(tc, combat_encounter_join(&second, &first, ((long)(4 RL_SEC))));
  combat_encounter_get_stats(&stats);
  CuAssertIntEquals(tc, 1, (int)stats.active_encounters);
  CuAssertIntEquals(tc, 2, (int)stats.active_participants);
  CuAssertIntEquals(tc, 1, (int)stats.scheduled_events);
  CuAssertIntEquals(tc, 1, event_queue_depth());

  pulse = start_pulse + ((unsigned long)(2 RL_SEC));
  event_test_advance();
  CuAssertIntEquals(tc, 1, (int)trace.count);
  CuAssertPtrEquals(tc, &first, trace.characters[0]);
  CuAssertIntEquals(tc, 1, (int)trace.phases[0]);

  pulse = start_pulse + ((unsigned long)(4 RL_SEC));
  event_test_advance();
  CuAssertIntEquals(tc, 3, (int)trace.count);
  CuAssertPtrEquals(tc, &first, trace.characters[1]);
  CuAssertIntEquals(tc, 2, (int)trace.phases[1]);
  CuAssertPtrEquals(tc, &second, trace.characters[2]);
  CuAssertIntEquals(tc, 1, (int)trace.phases[2]);
  CuAssertIntEquals(tc, 1, event_queue_depth());

  encounter_test_leave(&first, COMBAT_ENCOUNTER_DEPARTURE_STOPPED);
  encounter_test_leave(&second, COMBAT_ENCOUNTER_DEPARTURE_STOPPED);
  combat_encounter_get_stats(&stats);
  CuAssertIntEquals(tc, 0, (int)stats.active_encounters);
  CuAssertIntEquals(tc, 0, (int)stats.active_participants);
  CuAssertIntEquals(tc, 0, (int)stats.scheduled_events);
  CuAssertIntEquals(tc, 0, event_queue_depth());
  encounter_test_end(saved_pulse);
}

void Test_combat_encounter_honors_initial_delay_for_callback_joins(CuTest *tc)
{
  struct char_data first;
  struct char_data anchor;
  struct char_data before;
  struct char_data during;
  struct char_data after;
  struct encounter_test_trace trace;
  struct combat_encounter_stats stats;
  unsigned long saved_pulse;
  const unsigned long start_pulse = 2000U;

  encounter_test_character(&first, "initial combatant");
  encounter_test_character(&anchor, "encounter anchor");
  encounter_test_character(&before, "pre-dispatch joiner");
  encounter_test_character(&during, "in-dispatch joiner");
  encounter_test_character(&after, "post-dispatch joiner");
  saved_pulse = encounter_test_begin(tc, true, start_pulse, &trace);
  trace.mutation_trigger = &before;
  trace.mutation_character = &during;
  trace.mutation_opponent = &anchor;
  trace.join_during_callback = true;
  combat_encounter_test_set_phase_callback(encounter_test_record_phase, &trace);

  FIGHTING(&first) = &anchor;
  FIGHTING(&before) = &anchor;
  CuAssertTrue(tc, combat_encounter_join(&first, &anchor, ((long)(2 RL_SEC))));
  CuAssertTrue(tc, combat_encounter_join(&before, &anchor, ((long)(1 RL_SEC))));
  pulse = start_pulse + ((unsigned long)(1 RL_SEC));
  event_test_advance();
  CuAssertTrue(tc, trace.mutation_ran);
  CuAssertTrue(tc, trace.mutation_succeeded);
  CuAssertIntEquals(tc, 1, (int)trace.count);
  CuAssertPtrEquals(tc, &before, trace.characters[0]);

  FIGHTING(&after) = &anchor;
  CuAssertTrue(tc, combat_encounter_join(&after, &anchor, ((long)(1 RL_SEC))));
  /* Callback admission clamps to a future tick, without adding a full round. */
  event_test_advance();
  CuAssertIntEquals(tc, 1, (int)trace.count);
  pulse++;
  event_test_advance();
  CuAssertIntEquals(tc, 2, (int)trace.count);
  CuAssertPtrEquals(tc, &during, trace.characters[1]);
  pulse++;
  event_test_advance();
  CuAssertIntEquals(tc, 2, (int)trace.count);
  pulse = start_pulse + ((unsigned long)(2 RL_SEC));
  event_test_advance();
  CuAssertIntEquals(tc, 4, (int)trace.count);
  CuAssertPtrEquals(tc, &after, trace.characters[2]);
  CuAssertPtrEquals(tc, &first, trace.characters[3]);

  combat_encounter_get_stats(&stats);
  CuAssertIntEquals(tc, 1, (int)stats.active_encounters);
  CuAssertIntEquals(tc, 5, (int)stats.active_participants);
  CuAssertIntEquals(tc, 1, (int)stats.scheduled_events);

  encounter_test_leave(&first, COMBAT_ENCOUNTER_DEPARTURE_STOPPED);
  encounter_test_leave(&anchor, COMBAT_ENCOUNTER_DEPARTURE_STOPPED);
  encounter_test_leave(&before, COMBAT_ENCOUNTER_DEPARTURE_STOPPED);
  encounter_test_leave(&during, COMBAT_ENCOUNTER_DEPARTURE_STOPPED);
  encounter_test_leave(&after, COMBAT_ENCOUNTER_DEPARTURE_STOPPED);
  encounter_test_end(saved_pulse);
}

void Test_combat_encounter_merges_during_resolution_without_extra_turn(CuTest *tc)
{
  struct char_data first;
  struct char_data bridge;
  struct char_data second;
  struct char_data second_anchor;
  struct encounter_test_trace trace;
  struct combat_encounter_stats stats;
  unsigned long saved_pulse;
  const unsigned long start_pulse = 3000U;

  encounter_test_character(&first, "first encounter actor");
  encounter_test_character(&bridge, "encounter bridge");
  encounter_test_character(&second, "second encounter actor");
  encounter_test_character(&second_anchor, "second encounter anchor");
  saved_pulse = encounter_test_begin(tc, true, start_pulse, &trace);
  trace.mutation_trigger = &first;
  trace.mutation_character = &bridge;
  trace.mutation_opponent = &second;
  trace.join_during_callback = true;
  combat_encounter_test_set_phase_callback(encounter_test_record_phase, &trace);

  FIGHTING(&first) = &bridge;
  FIGHTING(&second) = &second_anchor;
  CuAssertTrue(tc, combat_encounter_join(&first, &bridge, ((long)(1 RL_SEC))));
  CuAssertTrue(tc, combat_encounter_join(&second, &second_anchor, ((long)(1 RL_SEC))));
  CuAssertIntEquals(tc, 2, event_queue_depth());

  pulse = start_pulse + ((unsigned long)(1 RL_SEC));
  event_test_advance();
  CuAssertTrue(tc, trace.mutation_succeeded);
  CuAssertIntEquals(tc, 2, (int)trace.count);
  CuAssertPtrEquals(tc, &first, trace.characters[0]);
  CuAssertPtrEquals(tc, &second, trace.characters[1]);
  CuAssertIntEquals(tc, 1, (int)trace.phases[0]);
  CuAssertIntEquals(tc, 1, (int)trace.phases[1]);
  combat_encounter_get_stats(&stats);
  CuAssertIntEquals(tc, 1, (int)stats.active_encounters);
  CuAssertIntEquals(tc, 4, (int)stats.active_participants);
  CuAssertIntEquals(tc, 1, (int)stats.scheduled_events);
  CuAssertIntEquals(tc, 1, (int)stats.encounters_merged);
  CuAssertIntEquals(tc, 1, event_queue_depth());

  encounter_test_leave(&bridge, COMBAT_ENCOUNTER_DEPARTURE_MOVED);
  combat_encounter_get_stats(&stats);
  CuAssertIntEquals(tc, 3, (int)stats.active_participants);
  encounter_test_leave(&first, COMBAT_ENCOUNTER_DEPARTURE_STOPPED);
  encounter_test_leave(&second, COMBAT_ENCOUNTER_DEPARTURE_STOPPED);
  encounter_test_leave(&second_anchor, COMBAT_ENCOUNTER_DEPARTURE_STOPPED);
  encounter_test_end(saved_pulse);
}

void Test_combat_encounter_callback_can_remove_every_participant(CuTest *tc)
{
  struct char_data first;
  struct char_data second;
  struct encounter_test_trace trace;
  struct combat_encounter_stats stats;
  unsigned long saved_pulse;
  const unsigned long start_pulse = 4000U;

  encounter_test_character(&first, "departing actor");
  encounter_test_character(&second, "departing target");
  saved_pulse = encounter_test_begin(tc, true, start_pulse, &trace);
  trace.mutation_trigger = &first;
  trace.mutation_character = &first;
  trace.mutation_opponent = &second;
  trace.mutation_reason = COMBAT_ENCOUNTER_DEPARTURE_DIED;
  trace.vanish_during_callback = true;
  combat_encounter_test_set_phase_callback(encounter_test_record_phase, &trace);
  FIGHTING(&first) = &second;
  FIGHTING(&second) = &first;
  CuAssertTrue(tc, combat_encounter_join(&first, &second, ((long)(1 RL_SEC))));
  CuAssertTrue(tc, combat_encounter_join(&second, &first, ((long)(1 RL_SEC))));

  pulse = start_pulse + ((unsigned long)(1 RL_SEC));
  event_test_advance();
  CuAssertTrue(tc, trace.mutation_succeeded);
  CuAssertPtrEquals(tc, NULL, first.combat_encounter);
  CuAssertPtrEquals(tc, NULL, second.combat_encounter);
  CuAssertIntEquals(tc, 0, event_queue_depth());
  combat_encounter_get_stats(&stats);
  CuAssertIntEquals(tc, 0, (int)stats.active_encounters);
  CuAssertIntEquals(tc, 0, (int)stats.active_participants);
  CuAssertIntEquals(tc, 1, (int)stats.encounters_ended);
  CuAssertIntEquals(tc, 2, (int)stats.departure_counts[COMBAT_ENCOUNTER_DEPARTURE_DIED]);
  encounter_test_end(saved_pulse);
}

void Test_combat_encounter_reuses_ids_with_a_new_generation(CuTest *tc)
{
  struct char_data first;
  struct char_data second;
  struct char_data third;
  struct char_data fourth;
  struct encounter_test_trace trace;
  struct event_debug_snapshot first_snapshot;
  struct event_debug_snapshot second_snapshot;
  size_t returned_count;
  unsigned long saved_pulse;

  encounter_test_character(&first, "first generation actor");
  encounter_test_character(&second, "first generation target");
  encounter_test_character(&third, "second generation actor");
  encounter_test_character(&fourth, "second generation target");
  saved_pulse = encounter_test_begin(tc, true, 5000U, &trace);
  combat_encounter_test_set_phase_callback(encounter_test_record_phase, &trace);
  FIGHTING(&first) = &second;
  CuAssertTrue(tc, combat_encounter_join(&first, &second, ((long)(1 RL_SEC))));
  CuAssertIntEquals(tc, 1, (int)event_debug_inspect(NULL, &first_snapshot, 1U, &returned_count));
  CuAssertIntEquals(tc, 1, (int)returned_count);
  CuAssertIntEquals(tc, GAME_EVENT_OWNER_ENCOUNTER, first_snapshot.owner.kind);
  encounter_test_leave(&first, COMBAT_ENCOUNTER_DEPARTURE_STOPPED);
  encounter_test_leave(&second, COMBAT_ENCOUNTER_DEPARTURE_STOPPED);
  CuAssertIntEquals(tc, 0, event_queue_depth());

  FIGHTING(&third) = &fourth;
  CuAssertTrue(tc, combat_encounter_join(&third, &fourth, ((long)(1 RL_SEC))));
  CuAssertIntEquals(tc, 1, (int)event_debug_inspect(NULL, &second_snapshot, 1U, &returned_count));
  CuAssertIntEquals(tc, 1, (int)returned_count);
  CuAssertIntEquals(tc, GAME_EVENT_OWNER_ENCOUNTER, second_snapshot.owner.kind);
  CuAssertTrue(tc, first_snapshot.owner.runtime_id == second_snapshot.owner.runtime_id);
  CuAssertTrue(tc, first_snapshot.owner.generation != second_snapshot.owner.generation);

  encounter_test_leave(&third, COMBAT_ENCOUNTER_DEPARTURE_STOPPED);
  encounter_test_leave(&fourth, COMBAT_ENCOUNTER_DEPARTURE_STOPPED);
  encounter_test_end(saved_pulse);
}

void Test_combat_encounter_stale_owner_teardown_never_touches_released_character(CuTest *tc)
{
  struct char_data *actor = calloc(1U, sizeof(*actor));
  struct char_data target;
  struct encounter_test_trace trace = {0};
  struct combat_encounter_stats stats;
  struct domain_event_bus *bus;
  enum domain_event_status status;
  unsigned long saved_pulse = pulse;

  encounter_test_character(actor, "stale encounter actor");
  encounter_test_character(&target, "passive encounter target");
  combat_encounter_runtime_shutdown();
  event_free_all();
  pulse = 5400U;
  event_init();
  bus = domain_event_bus_create(NULL, &status);
  CuAssertPtrNotNull(tc, bus);
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK, domain_event_register_foundation_types(bus));
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK, domain_event_world_register_resolvers(bus));
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK, combat_encounter_runtime_init(bus));
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK, domain_event_seal(bus));
  combat_encounter_test_set_phase_callback(encounter_test_record_phase, &trace);
  FIGHTING(actor) = &target;
  CuAssertTrue(tc, combat_encounter_join(actor, &target, ((long)(2 RL_SEC))));
  /* Simulate an owner disappearing before its pending scheduler callback. */
  domain_event_world_forget_character(actor);
  free(actor);
  pulse += ((unsigned long)(2 RL_SEC));
  event_test_advance();
  combat_encounter_get_stats(&stats);
  CuAssertIntEquals(tc, 0, (int)trace.count);
  CuAssertIntEquals(tc, 1, (int)stats.stale_encounter_callbacks);
  CuAssertIntEquals(tc, 0, (int)stats.active_encounters);
  CuAssertIntEquals(tc, 0, (int)stats.active_participants);
  CuAssertIntEquals(tc, 0, (int)stats.scheduled_events);
  CuAssertIntEquals(tc, 0, event_queue_depth());
  CuAssertPtrEquals(tc, NULL, target.combat_encounter);
  encounter_test_end(saved_pulse);
  domain_event_bus_destroy(bus);
  domain_event_world_forget_character(&target);
}

void Test_combat_encounter_terminal_session_is_not_revived_from_callback(CuTest *tc)
{
  struct char_data first;
  struct char_data passive;
  struct encounter_test_trace trace;
  struct combat_encounter_stats stats;
  unsigned long saved_pulse;
  const unsigned long start_pulse = 5500U;

  encounter_test_character(&first, "terminal encounter actor");
  encounter_test_character(&passive, "terminal encounter target");
  saved_pulse = encounter_test_begin(tc, true, start_pulse, &trace);
  trace.mutation_trigger = &first;
  trace.mutation_character = &first;
  trace.mutation_opponent = &passive;
  trace.mutation_reason = COMBAT_ENCOUNTER_DEPARTURE_STOPPED;
  trace.vanish_during_callback = true;
  trace.leave_only_mutation_character = true;
  trace.rejoin_after_vanish = true;
  combat_encounter_test_set_phase_callback(encounter_test_record_phase, &trace);
  FIGHTING(&first) = &passive;
  CuAssertTrue(tc, combat_encounter_join(&first, &passive, ((long)(1 RL_SEC))));

  pulse = start_pulse + ((unsigned long)(1 RL_SEC));
  event_test_advance();
  CuAssertTrue(tc, trace.mutation_succeeded);
  CuAssertPtrNotNull(tc, first.combat_encounter);
  CuAssertPtrEquals(tc, first.combat_encounter, passive.combat_encounter);
  combat_encounter_get_stats(&stats);
  CuAssertIntEquals(tc, 1, (int)stats.active_encounters);
  CuAssertIntEquals(tc, 2, (int)stats.active_participants);
  CuAssertIntEquals(tc, 1, (int)stats.scheduled_events);
  CuAssertIntEquals(tc, 2, (int)stats.encounters_created);
  CuAssertIntEquals(tc, 1, (int)stats.encounters_ended);

  encounter_test_leave(&first, COMBAT_ENCOUNTER_DEPARTURE_STOPPED);
  encounter_test_leave(&passive, COMBAT_ENCOUNTER_DEPARTURE_STOPPED);
  encounter_test_end(saved_pulse);
}

void Test_combat_encounter_tracks_departures_and_cancels_once(CuTest *tc)
{
  struct char_data first;
  struct char_data second;
  struct encounter_test_trace trace;
  struct combat_encounter_stats stats;
  enum combat_encounter_departure_reason reason;
  unsigned long saved_pulse;

  encounter_test_character(&first, "departure actor");
  encounter_test_character(&second, "departure target");
  saved_pulse = encounter_test_begin(tc, true, 6000U, &trace);
  combat_encounter_test_set_phase_callback(encounter_test_record_phase, &trace);

  for (reason = COMBAT_ENCOUNTER_DEPARTURE_STOPPED; reason < COMBAT_ENCOUNTER_DEPARTURE_COUNT;
       reason++)
  {
    FIGHTING(&first) = &second;
    FIGHTING(&second) = &first;
    CuAssertTrue(tc, combat_encounter_join(&first, &second, ((long)(1 RL_SEC))));
    CuAssertTrue(tc, combat_encounter_join(&second, &first, ((long)(1 RL_SEC))));
    CuAssertIntEquals(tc, 1, event_queue_depth());
    encounter_test_leave(&first, reason);
    encounter_test_leave(&second, reason);
    CuAssertIntEquals(tc, 0, event_queue_depth());
  }
  combat_encounter_get_stats(&stats);
  for (reason = COMBAT_ENCOUNTER_DEPARTURE_STOPPED; reason < COMBAT_ENCOUNTER_DEPARTURE_COUNT;
       reason++)
    CuAssertIntEquals(tc, 2, (int)stats.departure_counts[reason]);
  CuAssertIntEquals(tc, COMBAT_ENCOUNTER_DEPARTURE_COUNT, (int)stats.encounters_created);
  CuAssertIntEquals(tc, COMBAT_ENCOUNTER_DEPARTURE_COUNT, (int)stats.encounters_ended);
  encounter_test_end(saved_pulse);
}

void Test_combat_encounter_rollback_selector_keeps_legacy_path_exclusive(CuTest *tc)
{
  struct char_data first;
  struct char_data second;
  struct combat_encounter_stats stats;
  unsigned long saved_pulse;

  encounter_test_character(&first, "legacy actor");
  encounter_test_character(&second, "legacy target");
  saved_pulse = encounter_test_begin(tc, false, 7000U, NULL);
  FIGHTING(&first) = &second;

  CuAssertTrue(tc, !combat_encounter_events_enabled());
  CuAssertTrue(tc, !combat_encounter_join(&first, &second, ((long)(1 RL_SEC))));
  CuAssertPtrEquals(tc, NULL, first.combat_encounter);
  CuAssertPtrEquals(tc, NULL, second.combat_encounter);
  CuAssertIntEquals(tc, 0, event_queue_depth());
  combat_encounter_get_stats(&stats);
  CuAssertTrue(tc, !stats.encounter_mode);
  CuAssertIntEquals(tc, 0, (int)stats.active_encounters);
  encounter_test_end(saved_pulse);
}

void Test_combat_initiative_display_follows_pending_attack_deadlines(CuTest *tc)
{
  struct char_data slower;
  struct char_data faster;
  struct player_special_data slower_specials;
  struct player_special_data faster_specials;
  struct descriptor_data slower_descriptor;
  struct combat_encounter_initiative_entry initiative_entries[2];
  struct combat_encounter_initiative_snapshot initiative_snapshot;
  struct encounter_test_trace trace;
  struct combat_encounter_stats stats;
  unsigned long saved_pulse;
  const unsigned long start_pulse = 8000U;

  encounter_test_character(&slower, "slower combatant");
  encounter_test_character(&faster, "faster combatant");
  memset(&slower_specials, 0, sizeof(slower_specials));
  memset(&faster_specials, 0, sizeof(faster_specials));
  memset(&slower_descriptor, 0, sizeof(slower_descriptor));
  slower.player_specials = &slower_specials;
  faster.player_specials = &faster_specials;
  slower.desc = &slower_descriptor;
  slower_descriptor.character = &slower;
  slower_descriptor.output = slower_descriptor.small_outbuf;
  slower_descriptor.bufspace = SMALL_BUFSIZE - 1;
  slower_descriptor.pProtocol = ProtocolCreate();
  GET_SCREEN_WIDTH(&slower) = 80;
  GET_PAGE_LENGTH(&slower) = 24;
  SET_BIT_AR(PRF_FLAGS(&slower), PRF_COLOR_1);
  GET_INITIATIVE(&slower) = 10;
  GET_INITIATIVE(&faster) = 20;
  saved_pulse = encounter_test_begin_semantic(tc, start_pulse, &trace);
  combat_encounter_test_set_phase_callback(encounter_test_record_phase, &trace);
  FIGHTING(&slower) = &faster;
  FIGHTING(&faster) = &slower;

  CuAssertTrue(tc, combat_encounter_join(&slower, &faster, ((long)(2 RL_SEC))));
  CuAssertTrue(tc, combat_encounter_join(&faster, &slower, ((long)(4 RL_SEC))));
  CuAssertTrue(
      tc, combat_encounter_get_initiative(&slower, initiative_entries, 2U, &initiative_snapshot));
  CuAssertTrue(tc, initiative_snapshot.semantic_rounds);
  CuAssertIntEquals(tc, 1, (int)initiative_snapshot.round_number);
  CuAssertIntEquals(tc, 2, (int)initiative_snapshot.total_participants);
  CuAssertIntEquals(tc, 2, (int)initiative_snapshot.entry_count);
  CuAssertPtrEquals(tc, &slower, initiative_entries[0].character);
  CuAssertIntEquals(tc, 10, initiative_entries[0].initiative);
  CuAssertIntEquals(tc, 1, (int)initiative_entries[0].phase);
  CuAssertIntEquals(tc, 2 RL_SEC, (int)initiative_entries[0].pulses_until_phase);
  CuAssertPtrEquals(tc, &faster, initiative_entries[1].character);
  CuAssertIntEquals(tc, 20, initiative_entries[1].initiative);
  CuAssertIntEquals(tc, 1, (int)initiative_entries[1].phase);
  CuAssertIntEquals(tc, 4 RL_SEC, (int)initiative_entries[1].pulses_until_phase);
  do_initiative(&slower, "", 0, 0);
  CuAssertPtrNotNull(tc, strstr(slower_descriptor.output, KGRN));
  CuAssertPtrNotNull(tc, strstr(slower_descriptor.output, KNRM " (you)"));
  ProtocolDestroy(slower_descriptor.pProtocol);
  slower_descriptor.pProtocol = NULL;
  pulse = start_pulse + ((unsigned long)(2 RL_SEC)) - 1U;
  event_test_advance();
  CuAssertIntEquals(tc, 0, (int)trace.count);
  pulse++;
  event_test_advance();
  CuAssertIntEquals(tc, 1, (int)trace.count);
  CuAssertPtrEquals(tc, &slower, trace.characters[0]);
  pulse = start_pulse + ((unsigned long)(4 RL_SEC));
  event_test_advance();
  CuAssertIntEquals(tc, 3, (int)trace.count);
  CuAssertPtrEquals(tc, &slower, trace.characters[1]);
  CuAssertPtrEquals(tc, &faster, trace.characters[2]);
  CuAssertIntEquals(tc, 2, (int)trace.phases[1]);
  CuAssertIntEquals(tc, 1, (int)trace.phases[2]);
  pulse = start_pulse + ((unsigned long)(6 RL_SEC));
  event_test_advance();
  CuAssertIntEquals(tc, 5, (int)trace.count);
  combat_encounter_get_stats(&stats);
  CuAssertTrue(tc, stats.semantic_rounds);
  CuAssertIntEquals(tc, 1, (int)stats.semantic_rounds_resolved);
  CuAssertIntEquals(tc, 2, (int)stats.semantic_turns_resolved);
  CuAssertIntEquals(tc, 1, event_queue_depth());

  encounter_test_leave(&slower, COMBAT_ENCOUNTER_DEPARTURE_STOPPED);
  encounter_test_leave(&faster, COMBAT_ENCOUNTER_DEPARTURE_STOPPED);
  encounter_test_end(saved_pulse);
}

void Test_combat_idle_join_keeps_attack_and_logical_turn_deadlines_separate(CuTest *tc)
{
  struct char_data first;
  struct char_data second;
  struct encounter_test_trace trace;
  struct combat_encounter_stats stats;
  unsigned long saved_pulse;
  const unsigned long start_pulse = 8500U;
  const unsigned long joined_pulse = start_pulse + ((unsigned long)(3 RL_SEC));

  encounter_test_character(&first, "idle join combatant");
  encounter_test_character(&second, "idle join target");
  saved_pulse = encounter_test_begin_semantic(tc, start_pulse, &trace);
  combat_encounter_test_set_phase_callback(encounter_test_record_phase, &trace);
  pulse = joined_pulse;
  FIGHTING(&first) = &second;
  FIGHTING(&second) = &first;
  CuAssertTrue(tc, combat_encounter_join(&first, &second, ((long)(1 RL_SEC))));
  CuAssertTrue(tc, combat_encounter_join(&second, &first, ((long)(1 RL_SEC))));

  pulse = joined_pulse + ((unsigned long)(1 RL_SEC)) - 1U;
  event_test_advance();
  CuAssertIntEquals(tc, 0, (int)trace.count);
  pulse++;
  event_test_advance();
  CuAssertIntEquals(tc, 2, (int)trace.count);
  combat_encounter_get_stats(&stats);
  CuAssertIntEquals(tc, 0, (int)stats.semantic_rounds_resolved);
  pulse = joined_pulse + ((unsigned long)(3 RL_SEC));
  event_test_advance();
  pulse = joined_pulse + ((unsigned long)(5 RL_SEC));
  event_test_advance();
  CuAssertIntEquals(tc, 6, (int)trace.count);
  pulse = joined_pulse + ((unsigned long)(6 RL_SEC)) - 1U;
  event_test_advance();
  combat_encounter_get_stats(&stats);
  CuAssertIntEquals(tc, 0, (int)stats.semantic_rounds_resolved);
  pulse++;
  event_test_advance();
  CuAssertIntEquals(tc, 6, (int)trace.count);
  combat_encounter_get_stats(&stats);
  CuAssertIntEquals(tc, 1, (int)stats.semantic_rounds_resolved);
  CuAssertIntEquals(tc, 2, (int)stats.semantic_turns_resolved);

  encounter_test_leave(&first, COMBAT_ENCOUNTER_DEPARTURE_STOPPED);
  encounter_test_leave(&second, COMBAT_ENCOUNTER_DEPARTURE_STOPPED);
  encounter_test_end(saved_pulse);
}

void Test_combat_equal_phase_deadlines_use_reverse_scheduling_order(CuTest *tc)
{
  struct char_data first;
  struct char_data second;
  struct char_data third;
  struct encounter_test_trace trace;
  unsigned long saved_pulse;
  const unsigned long start_pulse = 9000U;

  encounter_test_character(&first, "first tied combatant");
  encounter_test_character(&second, "dexterous tied combatant");
  encounter_test_character(&third, "third tied combatant");
  GET_INITIATIVE(&first) = GET_INITIATIVE(&second) = GET_INITIATIVE(&third) = 15;
  GET_REAL_DEX(&first) = 10;
  GET_REAL_DEX(&second) = 16;
  GET_REAL_DEX(&third) = 10;
  first.aff_abils.dex = GET_REAL_DEX(&first);
  second.aff_abils.dex = GET_REAL_DEX(&second);
  third.aff_abils.dex = GET_REAL_DEX(&third);
  saved_pulse = encounter_test_begin_semantic(tc, start_pulse, &trace);
  combat_encounter_test_set_phase_callback(encounter_test_record_phase, &trace);
  FIGHTING(&first) = &second;
  FIGHTING(&second) = &first;
  FIGHTING(&third) = &first;
  CuAssertTrue(tc, combat_encounter_join(&first, &second, ((long)(1 RL_SEC))));
  CuAssertTrue(tc, combat_encounter_join(&second, &first, ((long)(1 RL_SEC))));
  CuAssertTrue(tc, combat_encounter_join(&third, &first, ((long)(1 RL_SEC))));
  pulse = start_pulse + ((unsigned long)(1 RL_SEC));
  event_test_advance();
  CuAssertIntEquals(tc, 3, (int)trace.count);
  CuAssertPtrEquals(tc, &third, trace.characters[0]);
  CuAssertPtrEquals(tc, &second, trace.characters[1]);
  CuAssertPtrEquals(tc, &first, trace.characters[2]);
  pulse = start_pulse + ((unsigned long)(3 RL_SEC));
  event_test_advance();
  CuAssertIntEquals(tc, 6, (int)trace.count);
  CuAssertPtrEquals(tc, &first, trace.characters[3]);
  CuAssertPtrEquals(tc, &second, trace.characters[4]);
  CuAssertPtrEquals(tc, &third, trace.characters[5]);

  encounter_test_leave(&first, COMBAT_ENCOUNTER_DEPARTURE_STOPPED);
  encounter_test_leave(&second, COMBAT_ENCOUNTER_DEPARTURE_STOPPED);
  encounter_test_leave(&third, COMBAT_ENCOUNTER_DEPARTURE_STOPPED);
  encounter_test_end(saved_pulse);
}

void Test_combat_actions_use_native_timers_during_encounter_phases(CuTest *tc)
{
  struct char_data first;
  struct char_data second;
  struct encounter_test_trace trace;
  struct combat_encounter_stats stats;
  unsigned long saved_pulse;
  bool available;
  const unsigned long start_pulse = 10000U;

  encounter_test_character(&first, "budget combatant");
  encounter_test_character(&second, "budget target");
  saved_pulse = encounter_test_begin_semantic(tc, start_pulse, &trace);
  combat_encounter_test_set_phase_callback(encounter_test_record_phase, &trace);
  FIGHTING(&first) = &second;
  CuAssertTrue(tc, combat_encounter_join(&first, &second, ((long)(1 RL_SEC))));

  available = is_action_available(&first, atSTANDARD, false);
  CuAssertTrue(tc, available);
  start_action_cooldown(&first, atSTANDARD, 6 RL_SEC);
  start_action_cooldown(&first, atMOVE, 12 RL_SEC);
  available = is_action_available(&first, atSTANDARD, false);
  CuAssertTrue(tc, !available);
  pulse = start_pulse + ((unsigned long)(6 RL_SEC));
  event_test_advance();
  available = is_action_available(&first, atSTANDARD, false);
  CuAssertTrue(tc, available);
  available = is_action_available(&first, atMOVE, false);
  CuAssertTrue(tc, !available);

  pulse = start_pulse + ((unsigned long)(12 RL_SEC));
  event_test_advance();
  available = is_action_available(&first, atMOVE, false);
  CuAssertTrue(tc, available);
  combat_encounter_get_stats(&stats);
  CuAssertIntEquals(tc, 2, (int)stats.semantic_turns_resolved);

  encounter_test_leave(&first, COMBAT_ENCOUNTER_DEPARTURE_STOPPED);
  encounter_test_leave(&second, COMBAT_ENCOUNTER_DEPARTURE_STOPPED);
  encounter_test_end(saved_pulse);
  if (first.events != NULL)
    free_list(first.events);
  if (second.events != NULL)
    free_list(second.events);
}

void Test_combat_phases_preserve_other_participants_native_state(CuTest *tc)
{
  struct char_data first;
  struct char_data second;
  struct semantic_boundary_trace trace = {0};
  unsigned long saved_pulse;
  const unsigned long start_pulse = 10500U;

  encounter_test_character(&first, "higher initiative combatant");
  encounter_test_character(&second, "lower initiative combatant");
  GET_INITIATIVE(&first) = 20;
  GET_INITIATIVE(&second) = 10;
  saved_pulse = encounter_test_begin_semantic(tc, start_pulse, NULL);
  trace.first = &first;
  trace.second = &second;
  combat_encounter_test_set_phase_callback(encounter_test_round_boundary_state, &trace);
  FIGHTING(&first) = &second;
  FIGHTING(&second) = &first;
  CuAssertTrue(tc, combat_encounter_join(&second, &first, ((long)(1 RL_SEC))));
  CuAssertTrue(tc, combat_encounter_join(&first, &second, ((long)(1 RL_SEC))));

  pulse = start_pulse + ((unsigned long)(6 RL_SEC));
  event_test_advance();
  CuAssertTrue(tc, trace.second_flag_survived);
  CuAssertTrue(tc, trace.second_reaction_remained_spent);

  encounter_test_leave(&first, COMBAT_ENCOUNTER_DEPARTURE_STOPPED);
  encounter_test_leave(&second, COMBAT_ENCOUNTER_DEPARTURE_STOPPED);
  encounter_test_end(saved_pulse);
  if (first.events != NULL)
    free_list(first.events);
  if (second.events != NULL)
    free_list(second.events);
}

void Test_combat_logical_turn_does_not_reset_native_round_flag_deadline(CuTest *tc)
{
  struct char_data first;
  struct char_data second;
  struct encounter_test_trace trace;
  unsigned long saved_pulse;
  bool used;
  const unsigned long start_pulse = 11000U;

  encounter_test_character(&first, "flag combatant");
  encounter_test_character(&second, "flag target");
  saved_pulse = encounter_test_begin_semantic(tc, start_pulse, &trace);
  combat_encounter_test_set_phase_callback(encounter_test_record_phase, &trace);
  FIGHTING(&first) = &second;
  CuAssertTrue(tc, combat_encounter_join(&first, &second, ((long)(1 RL_SEC))));
  attach_mud_event(new_mud_event(eDEFLECTIVE_SCREEN_HIT_THIS_ROUND, &first, NULL),
                   ((long)(10 RL_SEC)));
  used = char_has_mud_event(&first, eDEFLECTIVE_SCREEN_HIT_THIS_ROUND) != NULL;
  CuAssertTrue(tc, used);
  pulse = start_pulse + ((unsigned long)(6 RL_SEC));
  event_test_advance();
  used = char_has_mud_event(&first, eDEFLECTIVE_SCREEN_HIT_THIS_ROUND) != NULL;
  CuAssertTrue(tc, used);
  pulse = start_pulse + ((unsigned long)(10 RL_SEC)) - 1U;
  event_test_advance();
  CuAssertPtrNotNull(tc, char_has_mud_event(&first, eDEFLECTIVE_SCREEN_HIT_THIS_ROUND));
  pulse++;
  event_test_advance();
  CuAssertPtrEquals(tc, NULL, char_has_mud_event(&first, eDEFLECTIVE_SCREEN_HIT_THIS_ROUND));

  encounter_test_leave(&first, COMBAT_ENCOUNTER_DEPARTURE_STOPPED);
  encounter_test_leave(&second, COMBAT_ENCOUNTER_DEPARTURE_STOPPED);
  encounter_test_end(saved_pulse);
  if (first.events != NULL)
    free_list(first.events);
  if (second.events != NULL)
    free_list(second.events);
}

void Test_combat_semantic_staggered_spend_couples_standard_and_move(CuTest *tc)
{
  struct char_data first;
  struct char_data second;
  struct encounter_test_trace trace;
  unsigned long saved_pulse;
  bool available;

  encounter_test_character(&first, "staggered combatant");
  encounter_test_character(&second, "staggered target");
  SET_BIT_AR(AFF_FLAGS(&first), AFF_STAGGERED);
  saved_pulse = encounter_test_begin_semantic(tc, 11500U, &trace);
  combat_encounter_test_set_phase_callback(encounter_test_record_phase, &trace);
  FIGHTING(&first) = &second;
  CuAssertTrue(tc, combat_encounter_join(&first, &second, ((long)(1 RL_SEC))));

  start_action_cooldown(&first, atSTANDARD, 6 RL_SEC);
  available = is_action_available(&first, atSTANDARD, false);
  CuAssertTrue(tc, !available);
  available = is_action_available(&first, atMOVE, false);
  CuAssertTrue(tc, !available);
  pulse = 11500U + (6 RL_SEC);
  event_test_advance();
  available = is_action_available(&first, atSTANDARD, false);
  CuAssertTrue(tc, available);
  available = is_action_available(&first, atMOVE, false);
  CuAssertTrue(tc, available);

  encounter_test_leave(&first, COMBAT_ENCOUNTER_DEPARTURE_STOPPED);
  encounter_test_leave(&second, COMBAT_ENCOUNTER_DEPARTURE_STOPPED);
  encounter_test_end(saved_pulse);
  if (first.events != NULL)
    free_list(first.events);
  if (second.events != NULL)
    free_list(second.events);
}

void Test_combat_callback_join_attacks_before_its_first_logical_turn(CuTest *tc)
{
  struct char_data first;
  struct char_data anchor;
  struct char_data joiner;
  struct encounter_test_trace trace;
  unsigned long saved_pulse;
  struct combat_encounter_turn_snapshot snapshot;
  const unsigned long start_pulse = 12000U;

  encounter_test_character(&first, "semantic actor");
  encounter_test_character(&anchor, "semantic anchor");
  encounter_test_character(&joiner, "semantic joiner");
  saved_pulse = encounter_test_begin_semantic(tc, start_pulse, &trace);
  trace.mutation_trigger = &first;
  trace.mutation_character = &joiner;
  trace.mutation_opponent = &anchor;
  trace.join_during_callback = true;
  combat_encounter_test_set_phase_callback(encounter_test_record_phase, &trace);
  FIGHTING(&first) = &anchor;
  CuAssertTrue(tc, combat_encounter_join(&first, &anchor, ((long)(1 RL_SEC))));

  pulse = start_pulse + ((unsigned long)(1 RL_SEC));
  event_test_advance();
  CuAssertTrue(tc, trace.mutation_succeeded);
  CuAssertIntEquals(tc, 1, (int)trace.count);
  event_test_advance();
  CuAssertIntEquals(tc, 1, (int)trace.count);
  pulse++;
  event_test_advance();
  CuAssertIntEquals(tc, 2, (int)trace.count);
  CuAssertPtrEquals(tc, &joiner, trace.characters[1]);
  CuAssertIntEquals(tc, 1, (int)trace.phases[1]);
  CuAssertTrue(tc, combat_encounter_get_turn(&joiner, &snapshot));
  CuAssertIntEquals(tc, 0, (int)snapshot.turn_serial);
  CuAssertIntEquals(tc, (5 RL_SEC) - 1, (int)snapshot.pulses_until_next_turn);
  pulse++;
  event_test_advance();
  CuAssertIntEquals(tc, 2, (int)trace.count);

  encounter_test_leave(&first, COMBAT_ENCOUNTER_DEPARTURE_STOPPED);
  encounter_test_leave(&anchor, COMBAT_ENCOUNTER_DEPARTURE_STOPPED);
  encounter_test_leave(&joiner, COMBAT_ENCOUNTER_DEPARTURE_STOPPED);
  encounter_test_end(saved_pulse);
}

void Test_combat_encounter_keeps_native_action_event_across_membership(CuTest *tc)
{
  struct char_data first;
  struct char_data second;
  struct encounter_test_trace trace;
  struct mud_event_data *restored;
  unsigned long saved_pulse;
  bool available;

  encounter_test_character(&first, "cooldown combatant");
  encounter_test_character(&second, "cooldown target");
  saved_pulse = encounter_test_begin_semantic(tc, 13000U, &trace);
  start_action_cooldown(&first, atSTANDARD, 12 RL_SEC);
  CuAssertPtrNotNull(tc, char_has_mud_event(&first, eSTANDARDACTION));
  FIGHTING(&first) = &second;
  CuAssertTrue(tc, combat_encounter_join(&first, &second, ((long)(1 RL_SEC))));
  CuAssertPtrNotNull(tc, char_has_mud_event(&first, eSTANDARDACTION));
  available = is_action_available(&first, atSTANDARD, false);
  CuAssertTrue(tc, !available);

  encounter_test_leave(&first, COMBAT_ENCOUNTER_DEPARTURE_STOPPED);
  restored = char_has_mud_event(&first, eSTANDARDACTION);
  CuAssertPtrNotNull(tc, restored);
  CuAssertIntEquals(tc, 12 RL_SEC, (int)mud_event_remaining(restored));
  encounter_test_leave(&second, COMBAT_ENCOUNTER_DEPARTURE_STOPPED);
  encounter_test_end(saved_pulse);
  if (first.events != NULL)
    free_list(first.events);
  if (second.events != NULL)
    free_list(second.events);
}

void Test_combat_late_join_preserves_other_fighters_phase_offsets(CuTest *tc)
{
  struct char_data first;
  struct char_data anchor;
  struct char_data joiner;
  struct encounter_test_trace trace;
  unsigned long saved_pulse;
  const unsigned long start_pulse = 14000U;

  encounter_test_character(&first, "first combatant");
  encounter_test_character(&anchor, "combat anchor");
  encounter_test_character(&joiner, "mid-round joiner");
  GET_INITIATIVE(&first) = 10;
  GET_INITIATIVE(&joiner) = 20;
  saved_pulse = encounter_test_begin_semantic(tc, start_pulse, &trace);
  combat_encounter_test_set_phase_callback(encounter_test_record_phase, &trace);
  FIGHTING(&first) = &anchor;
  CuAssertTrue(tc, combat_encounter_join(&first, &anchor, ((long)(1 RL_SEC))));

  pulse = start_pulse + ((unsigned long)(1 RL_SEC));
  event_test_advance();
  pulse = start_pulse + ((unsigned long)(3 RL_SEC));
  event_test_advance();
  FIGHTING(&joiner) = &anchor;
  CuAssertTrue(tc, combat_encounter_join(&joiner, &anchor, ((long)(1 RL_SEC))));
  pulse = start_pulse + ((unsigned long)(4 RL_SEC)) - 1U;
  event_test_advance();
  CuAssertIntEquals(tc, 2, (int)trace.count);
  pulse++;
  event_test_advance();
  CuAssertIntEquals(tc, 3, (int)trace.count);
  CuAssertPtrEquals(tc, &joiner, trace.characters[2]);
  CuAssertIntEquals(tc, 1, (int)trace.phases[2]);
  pulse++;
  event_test_advance();
  CuAssertIntEquals(tc, 3, (int)trace.count);
  pulse = start_pulse + ((unsigned long)(5 RL_SEC));
  event_test_advance();
  CuAssertPtrEquals(tc, &first, trace.characters[3]);
  CuAssertIntEquals(tc, 3, (int)trace.phases[3]);
  pulse = start_pulse + ((unsigned long)(6 RL_SEC));
  event_test_advance();
  CuAssertIntEquals(tc, 5, (int)trace.count);
  CuAssertPtrEquals(tc, &joiner, trace.characters[4]);
  CuAssertIntEquals(tc, 2, (int)trace.phases[4]);
  CuAssertIntEquals(tc, 1, event_queue_depth());

  encounter_test_leave(&first, COMBAT_ENCOUNTER_DEPARTURE_STOPPED);
  encounter_test_leave(&anchor, COMBAT_ENCOUNTER_DEPARTURE_STOPPED);
  encounter_test_leave(&joiner, COMBAT_ENCOUNTER_DEPARTURE_STOPPED);
  encounter_test_end(saved_pulse);
}

static void verify_semantic_clock_merge(CuTest *tc, bool with_defense)
{
  struct char_data first;
  struct char_data first_anchor;
  struct char_data second;
  struct char_data second_anchor;
  struct encounter_test_trace trace;
  struct player_special_data first_specials = {0};
  struct player_special_data second_specials = {0};
  struct combat_encounter_turn_snapshot snapshot;
  unsigned long saved_pulse;
  unsigned long tick;
  const unsigned long start_pulse = 15000U;

  encounter_test_character(&first, "first clock combatant");
  encounter_test_character(&first_anchor, "first clock anchor");
  encounter_test_character(&second, "second clock combatant");
  encounter_test_character(&second_anchor, "second clock anchor");
  saved_pulse = encounter_test_begin_semantic(tc, start_pulse, &trace);
  combat_encounter_test_set_phase_callback(encounter_test_record_phase, &trace);
  if (with_defense)
  {
    first.player_specials = &first_specials;
    second.player_specials = &second_specials;
    CuAssertTrue(tc, tactical_effects_init());
  }
  FIGHTING(&first) = &first_anchor;
  FIGHTING(&first_anchor) = &first;
  CuAssertTrue(tc, combat_encounter_join(&first, &first_anchor, ((long)(1 RL_SEC))));
  CuAssertTrue(tc, combat_encounter_join(&first_anchor, &first, ((long)(1 RL_SEC))));
  if (with_defense)
    CuAssertTrue(tc, tactical_defense_start(&first));

  pulse = start_pulse + ((unsigned long)(1 RL_SEC));
  event_test_advance();
  pulse = start_pulse + ((unsigned long)(2 RL_SEC));
  FIGHTING(&second) = &second_anchor;
  FIGHTING(&second_anchor) = &second;
  CuAssertTrue(tc, combat_encounter_join(&second, &second_anchor, ((long)(1 RL_SEC))));
  CuAssertTrue(tc, combat_encounter_join(&second_anchor, &second, ((long)(1 RL_SEC))));
  if (with_defense)
    CuAssertTrue(tc, tactical_defense_start(&second));
  CuAssertIntEquals(tc, 2, event_queue_depth());

  pulse = start_pulse + ((unsigned long)(3 RL_SEC));
  FIGHTING(&first) = &second;
  FIGHTING(&second) = &first;
  CuAssertTrue(tc, combat_encounter_join(&first, &second, ((long)(1 RL_SEC))));
  CuAssertIntEquals(tc, 1, event_queue_depth());

  for (tick = start_pulse + ((unsigned long)(3 RL_SEC));
       tick <= start_pulse + ((unsigned long)(8 RL_SEC)); tick++)
  {
    pulse = tick;
    event_test_advance();
  }
  CuAssertIntEquals(tc, 14, (int)trace.count);
  CuAssertTrue(tc, combat_encounter_get_turn(&first, &snapshot));
  CuAssertIntEquals(tc, 1, (int)snapshot.turn_serial);
  CuAssertTrue(tc, combat_encounter_get_turn(&second, &snapshot));
  CuAssertIntEquals(tc, 0, (int)snapshot.turn_serial);
  if (with_defense)
  {
    CuAssertIntEquals(tc, 0, GET_DEFENSIVE_CASTING_TIMER(&first));
    CuAssertIntEquals(tc, 1, GET_DEFENSIVE_CASTING_TIMER(&second));
    CuAssertIntEquals(tc, 4 RL_SEC, tactical_defense_remaining(&second));
  }
  for (tick = start_pulse + ((unsigned long)(8 RL_SEC)) + 1U;
       tick <= start_pulse + ((unsigned long)(12 RL_SEC)); tick++)
  {
    pulse = tick;
    event_test_advance();
  }
  CuAssertIntEquals(tc, 22, (int)trace.count);
  CuAssertTrue(tc, combat_encounter_get_turn(&first, &snapshot));
  CuAssertIntEquals(tc, 2, (int)snapshot.turn_serial);
  CuAssertIntEquals(tc, 6 RL_SEC, (int)snapshot.pulses_until_next_turn);
  CuAssertTrue(tc, combat_encounter_get_turn(&second, &snapshot));
  CuAssertIntEquals(tc, 1, (int)snapshot.turn_serial);
  CuAssertIntEquals(tc, 6 RL_SEC, (int)snapshot.pulses_until_next_turn);
  CuAssertIntEquals(tc, 1, event_queue_depth());

  if (with_defense)
    CuAssertIntEquals(tc, 0, GET_DEFENSIVE_CASTING_TIMER(&second));
  encounter_test_leave(&first, COMBAT_ENCOUNTER_DEPARTURE_STOPPED);
  encounter_test_leave(&first_anchor, COMBAT_ENCOUNTER_DEPARTURE_STOPPED);
  encounter_test_leave(&second, COMBAT_ENCOUNTER_DEPARTURE_STOPPED);
  encounter_test_leave(&second_anchor, COMBAT_ENCOUNTER_DEPARTURE_STOPPED);
  encounter_test_end(saved_pulse);
  if (with_defense)
    tactical_effects_shutdown();
}

void Test_combat_semantic_merge_coalesces_offset_clocks(CuTest *tc)
{
  verify_semantic_clock_merge(tc, false);
}

void Test_combat_semantic_merge_preserves_defense_until_subject_turn(CuTest *tc)
{
  verify_semantic_clock_merge(tc, true);
}

void Test_combat_queue_dispatches_eligible_intents_between_phases(CuTest *tc)
{
  struct char_data first;
  struct char_data second;
  struct encounter_test_trace trace;
  struct combat_encounter_stats stats;
  unsigned long saved_pulse;
  bool created_command_list = false;
  const unsigned long start_pulse = 16000U;

  encounter_test_character(&first, "intent combatant");
  encounter_test_character(&second, "intent target");
  if (complete_cmd_info == NULL)
  {
    create_command_list();
    created_command_list = true;
  }
  GET_QUEUE(&first) = create_action_queue();
  saved_pulse = encounter_test_begin_semantic(tc, start_pulse, &trace);
  trace.execute_queue = true;
  combat_encounter_test_set_phase_callback(encounter_test_record_phase, &trace);
  FIGHTING(&first) = &second;
  CuAssertTrue(tc, combat_encounter_join(&first, &second, ((long)(1 RL_SEC))));
  encounter_test_enqueue(&first, "version");
  encounter_test_enqueue(&first, "version");

  peek_action(GET_QUEUE(&first))->actions_required = ACTION_STANDARD;
  start_action_cooldown(&first, atSTANDARD, 3L);
  execute_next_action(&first);
  CuAssertIntEquals(tc, 2, pending_actions(&first));
  pulse = start_pulse + 2U;
  event_test_advance();
  execute_next_action(&first);
  CuAssertIntEquals(tc, 2, pending_actions(&first));
  pulse++;
  event_test_advance();
  execute_next_action(&first);
  CuAssertIntEquals(tc, 1, pending_actions(&first));
  pulse++;
  event_test_advance();
  execute_next_action(&first);
  CuAssertIntEquals(tc, 0, pending_actions(&first));
  CuAssertIntEquals(tc, 0, (int)trace.count);
  combat_encounter_get_stats(&stats);
  CuAssertIntEquals(tc, 1, (int)stats.scheduled_events);

  encounter_test_leave(&first, COMBAT_ENCOUNTER_DEPARTURE_STOPPED);
  encounter_test_leave(&second, COMBAT_ENCOUNTER_DEPARTURE_STOPPED);
  free_action_queue(GET_QUEUE(&first));
  GET_QUEUE(&first) = NULL;
  if (created_command_list)
    free_command_list();
  encounter_test_end(saved_pulse);
  if (first.events != NULL)
    free_list(first.events);
  if (second.events != NULL)
    free_list(second.events);
}

struct turn_clock_trace
{
  struct char_data *watched;
  struct combat_encounter_turn_snapshot snapshot;
  bool available;
};

static bool capture_turn_clock(struct char_data *character, unsigned int phase, void *context)
{
  struct turn_clock_trace *trace = context;

  (void)phase;
  if (character == trace->watched)
    trace->available = combat_encounter_get_turn(character, &trace->snapshot);
  return true;
}

void Test_combat_turn_clock_survives_departure_and_reports_following_turn_in_callback(CuTest *tc)
{
  struct char_data first;
  struct char_data second;
  struct turn_clock_trace trace = {0};
  struct combat_encounter_turn_snapshot snapshot;
  unsigned long saved_pulse;
  const unsigned long start_pulse = 16000U;

  encounter_test_character(&first, "clock subject");
  encounter_test_character(&second, "clock opponent");
  saved_pulse = encounter_test_begin_semantic(tc, start_pulse, NULL);
  trace.watched = &first;
  combat_encounter_test_set_phase_callback(capture_turn_clock, &trace);
  CuAssertTrue(tc, !combat_encounter_get_turn(&first, &snapshot));
  FIGHTING(&first) = &second;
  FIGHTING(&second) = &first;
  CuAssertTrue(tc, combat_encounter_join(&first, &second, ((long)(1 RL_SEC))));
  CuAssertTrue(tc, combat_encounter_join(&second, &first, ((long)(1 RL_SEC))));
  CuAssertTrue(tc, combat_encounter_get_turn(&first, &snapshot));
  CuAssertIntEquals(tc, 0, (int)snapshot.turn_serial);
  CuAssertIntEquals(tc, 6 RL_SEC, (int)snapshot.pulses_until_next_turn);
  CuAssertTrue(tc, !snapshot.dispatching);

  /* A late event resolves one turn; it must not expose its overdue deadline as
   * the next turn to effects admitted from inside that turn. */
  pulse = start_pulse + ((unsigned long)(8 RL_SEC));
  CuAssertTrue(tc, combat_encounter_get_turn(&first, &snapshot));
  CuAssertIntEquals(tc, 0, (int)snapshot.pulses_until_next_turn);
  event_test_advance();
  CuAssertTrue(tc, trace.available && trace.snapshot.dispatching);
  CuAssertIntEquals(tc, 1, (int)trace.snapshot.turn_serial);
  CuAssertIntEquals(tc, 6 RL_SEC, (int)trace.snapshot.pulses_until_next_turn);

  pulse += ((unsigned long)(2 RL_SEC));
  CuAssertTrue(tc, combat_encounter_get_turn(&first, &snapshot));
  CuAssertIntEquals(tc, 4 RL_SEC, (int)snapshot.pulses_until_next_turn);
  encounter_test_leave(&first, COMBAT_ENCOUNTER_DEPARTURE_MOVED);
  encounter_test_leave(&second, COMBAT_ENCOUNTER_DEPARTURE_STOPPED);
  CuAssertTrue(tc, !combat_encounter_get_turn(&first, &snapshot));
  CuAssertIntEquals(tc, 1, (int)first.combat_turn_serial);

  FIGHTING(&first) = &second;
  FIGHTING(&second) = &first;
  CuAssertTrue(tc, combat_encounter_join(&first, &second, ((long)(1 RL_SEC))));
  CuAssertTrue(tc, combat_encounter_join(&second, &first, ((long)(1 RL_SEC))));
  CuAssertTrue(tc, combat_encounter_get_turn(&first, &snapshot));
  CuAssertIntEquals(tc, 1, (int)snapshot.turn_serial);
  pulse += ((unsigned long)(6 RL_SEC));
  event_test_advance();
  CuAssertIntEquals(tc, 2, (int)trace.snapshot.turn_serial);
  CuAssertTrue(tc, trace.snapshot.dispatching);

  encounter_test_leave(&first, COMBAT_ENCOUNTER_DEPARTURE_STOPPED);
  encounter_test_leave(&second, COMBAT_ENCOUNTER_DEPARTURE_STOPPED);
  encounter_test_end(saved_pulse);
}
