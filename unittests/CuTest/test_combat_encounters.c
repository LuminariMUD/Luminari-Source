#include "CuTest.h"

#include "../../src/conf.h"
#include "../../src/sysdep.h"
#include "../../src/structs.h"
#include "../../src/comm.h"
#include "../../src/db.h"
#include "../../src/act.h"
#include "../../src/actions.h"
#include "../../src/actionqueues.h"
#include "../../src/interpreter.h"
#include "../../src/combat/combat_encounters.h"
#include "../../src/combat/fight.h"
#include "../../src/dgscript/dg_event.h"
#include "../../src/domain_event_world.h"
#include "../../src/event_runtime.h"

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
  character->player.name = (char *)name;
}

static unsigned long encounter_test_begin_mode(CuTest *tc, bool encounter_mode,
                                               bool semantic_rounds,
                                               unsigned long start_pulse,
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

static bool encounter_test_record_phase(struct char_data *character,
                                        unsigned int phase, void *context)
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
    trace->mutation_succeeded = combat_encounter_join(
        trace->mutation_character, trace->mutation_opponent, 1L);
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
    trace->mutation_succeeded = combat_encounter_join(
        trace->mutation_character, trace->mutation_opponent, 1 RL_SEC);
  }
  return true;
}

static bool encounter_test_round_boundary_state(struct char_data *character,
                                                unsigned int phase, void *context)
{
  struct semantic_boundary_trace *trace = context;
  bool managed;
  bool used;

  (void)phase;
  if (character == trace->first)
  {
    combat_encounter_round_flag_mark(
        trace->second, COMBAT_ENCOUNTER_ROUND_DEFLECTIVE_SCREEN_USED);
    combat_encounter_reaction_try_use(trace->second, 1U, &managed);
  }
  else if (character == trace->second)
  {
    combat_encounter_round_flag_query(
        trace->second, COMBAT_ENCOUNTER_ROUND_DEFLECTIVE_SCREEN_USED, &used);
    trace->second_flag_survived = used;
    trace->second_reaction_remained_spent =
        !combat_encounter_reaction_try_use(trace->second, 1U, &managed) && managed;
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

  CuAssertTrue(tc, combat_encounter_join(&first, &second, 2 RL_SEC));
  CuAssertTrue(tc, combat_encounter_join(&second, &first, 4 RL_SEC));
  combat_encounter_get_stats(&stats);
  CuAssertIntEquals(tc, 1, (int)stats.active_encounters);
  CuAssertIntEquals(tc, 2, (int)stats.active_participants);
  CuAssertIntEquals(tc, 1, (int)stats.scheduled_events);
  CuAssertIntEquals(tc, 1, event_queue_depth());

  pulse = start_pulse + (2 RL_SEC);
  event_process();
  CuAssertIntEquals(tc, 1, (int)trace.count);
  CuAssertPtrEquals(tc, &first, trace.characters[0]);
  CuAssertIntEquals(tc, 1, (int)trace.phases[0]);

  pulse = start_pulse + (4 RL_SEC);
  event_process();
  CuAssertIntEquals(tc, 3, (int)trace.count);
  CuAssertPtrEquals(tc, &second, trace.characters[1]);
  CuAssertIntEquals(tc, 1, (int)trace.phases[1]);
  CuAssertPtrEquals(tc, &first, trace.characters[2]);
  CuAssertIntEquals(tc, 2, (int)trace.phases[2]);
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

void Test_combat_encounter_guards_joins_during_dispatch_only(CuTest *tc)
{
  struct char_data first;
  struct char_data anchor;
  struct char_data before;
  struct char_data during;
  struct char_data after;
  struct encounter_test_trace trace;
  struct combat_encounter_stats stats;
  unsigned long saved_pulse;
  unsigned long current;
  size_t index;
  bool saw_during = false;
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
  CuAssertTrue(tc, combat_encounter_join(&first, &anchor, 2 RL_SEC));
  CuAssertTrue(tc, combat_encounter_join(&before, &anchor, 1 RL_SEC));
  pulse = start_pulse + (1 RL_SEC);
  event_process();
  CuAssertTrue(tc, trace.mutation_ran);
  CuAssertTrue(tc, trace.mutation_succeeded);
  CuAssertIntEquals(tc, 1, (int)trace.count);
  CuAssertPtrEquals(tc, &before, trace.characters[0]);

  FIGHTING(&after) = &anchor;
  CuAssertTrue(tc, combat_encounter_join(&after, &anchor, 1 RL_SEC));
  pulse = start_pulse + (2 RL_SEC);
  event_process();
  CuAssertIntEquals(tc, 3, (int)trace.count);
  CuAssertPtrEquals(tc, &first, trace.characters[1]);
  CuAssertPtrEquals(tc, &after, trace.characters[2]);

  for (current = start_pulse + (3 RL_SEC); current <= start_pulse + (6 RL_SEC);
       current += (1 RL_SEC))
  {
    pulse = current;
    event_process();
  }
  for (index = 0U; index < trace.count; index++)
    if (trace.characters[index] == &during)
      saw_during = true;
  CuAssertTrue(tc, !saw_during);

  pulse = start_pulse + (7 RL_SEC);
  event_process();
  for (index = 0U; index < trace.count; index++)
    if (trace.characters[index] == &during)
      saw_during = true;
  CuAssertTrue(tc, saw_during);
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
  CuAssertTrue(tc, combat_encounter_join(&first, &bridge, 1 RL_SEC));
  CuAssertTrue(tc, combat_encounter_join(&second, &second_anchor, 1 RL_SEC));
  CuAssertIntEquals(tc, 2, event_queue_depth());

  pulse = start_pulse + (1 RL_SEC);
  event_process();
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
  CuAssertTrue(tc, combat_encounter_join(&first, &second, 1 RL_SEC));
  CuAssertTrue(tc, combat_encounter_join(&second, &first, 1 RL_SEC));

  pulse = start_pulse + (1 RL_SEC);
  event_process();
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
  CuAssertTrue(tc, combat_encounter_join(&first, &second, 1 RL_SEC));
  CuAssertIntEquals(tc, 1,
                    (int)event_debug_inspect(NULL, &first_snapshot, 1U, &returned_count));
  CuAssertIntEquals(tc, 1, (int)returned_count);
  CuAssertIntEquals(tc, GAME_EVENT_OWNER_ENCOUNTER, first_snapshot.owner.kind);
  encounter_test_leave(&first, COMBAT_ENCOUNTER_DEPARTURE_STOPPED);
  encounter_test_leave(&second, COMBAT_ENCOUNTER_DEPARTURE_STOPPED);
  CuAssertIntEquals(tc, 0, event_queue_depth());

  FIGHTING(&third) = &fourth;
  CuAssertTrue(tc, combat_encounter_join(&third, &fourth, 1 RL_SEC));
  CuAssertIntEquals(tc, 1,
                    (int)event_debug_inspect(NULL, &second_snapshot, 1U, &returned_count));
  CuAssertIntEquals(tc, 1, (int)returned_count);
  CuAssertIntEquals(tc, GAME_EVENT_OWNER_ENCOUNTER, second_snapshot.owner.kind);
  CuAssertTrue(tc, first_snapshot.owner.runtime_id == second_snapshot.owner.runtime_id);
  CuAssertTrue(tc, first_snapshot.owner.generation != second_snapshot.owner.generation);

  encounter_test_leave(&third, COMBAT_ENCOUNTER_DEPARTURE_STOPPED);
  encounter_test_leave(&fourth, COMBAT_ENCOUNTER_DEPARTURE_STOPPED);
  encounter_test_end(saved_pulse);
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
  CuAssertTrue(tc, combat_encounter_join(&first, &passive, 1 RL_SEC));

  pulse = start_pulse + (1 RL_SEC);
  event_process();
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

  for (reason = COMBAT_ENCOUNTER_DEPARTURE_STOPPED;
       reason < COMBAT_ENCOUNTER_DEPARTURE_COUNT; reason++)
  {
    FIGHTING(&first) = &second;
    FIGHTING(&second) = &first;
    CuAssertTrue(tc, combat_encounter_join(&first, &second, 1 RL_SEC));
    CuAssertTrue(tc, combat_encounter_join(&second, &first, 1 RL_SEC));
    CuAssertIntEquals(tc, 1, event_queue_depth());
    encounter_test_leave(&first, reason);
    encounter_test_leave(&second, reason);
    CuAssertIntEquals(tc, 0, event_queue_depth());
  }
  combat_encounter_get_stats(&stats);
  for (reason = COMBAT_ENCOUNTER_DEPARTURE_STOPPED;
       reason < COMBAT_ENCOUNTER_DEPARTURE_COUNT; reason++)
    CuAssertIntEquals(tc, 2, (int)stats.departure_counts[reason]);
  CuAssertIntEquals(tc, COMBAT_ENCOUNTER_DEPARTURE_COUNT,
                    (int)stats.encounters_created);
  CuAssertIntEquals(tc, COMBAT_ENCOUNTER_DEPARTURE_COUNT,
                    (int)stats.encounters_ended);
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
  CuAssertTrue(tc, !combat_encounter_join(&first, &second, 1 RL_SEC));
  CuAssertPtrEquals(tc, NULL, first.combat_encounter);
  CuAssertPtrEquals(tc, NULL, second.combat_encounter);
  CuAssertIntEquals(tc, 0, event_queue_depth());
  combat_encounter_get_stats(&stats);
  CuAssertTrue(tc, !stats.encounter_mode);
  CuAssertIntEquals(tc, 0, (int)stats.active_encounters);
  encounter_test_end(saved_pulse);
}

void Test_combat_semantic_round_batches_due_turns_in_initiative_order(CuTest *tc)
{
  struct char_data slower;
  struct char_data faster;
  struct encounter_test_trace trace;
  struct combat_encounter_stats stats;
  unsigned long saved_pulse;
  const unsigned long start_pulse = 8000U;

  encounter_test_character(&slower, "slower combatant");
  encounter_test_character(&faster, "faster combatant");
  GET_INITIATIVE(&slower) = 10;
  GET_INITIATIVE(&faster) = 20;
  saved_pulse = encounter_test_begin_semantic(tc, start_pulse, &trace);
  combat_encounter_test_set_phase_callback(encounter_test_record_phase, &trace);
  FIGHTING(&slower) = &faster;
  FIGHTING(&faster) = &slower;

  CuAssertTrue(tc, combat_encounter_join(&slower, &faster, 2 RL_SEC));
  CuAssertTrue(tc, combat_encounter_join(&faster, &slower, 4 RL_SEC));
  pulse = start_pulse + (5 RL_SEC);
  event_process();
  CuAssertIntEquals(tc, 0, (int)trace.count);

  pulse = start_pulse + (6 RL_SEC);
  event_process();
  CuAssertIntEquals(tc, 2, (int)trace.count);
  CuAssertPtrEquals(tc, &faster, trace.characters[0]);
  CuAssertPtrEquals(tc, &slower, trace.characters[1]);
  CuAssertIntEquals(tc, 0, (int)trace.phases[0]);
  CuAssertIntEquals(tc, 0, (int)trace.phases[1]);
  combat_encounter_get_stats(&stats);
  CuAssertTrue(tc, stats.semantic_rounds);
  CuAssertIntEquals(tc, 1, (int)stats.semantic_rounds_resolved);
  CuAssertIntEquals(tc, 2, (int)stats.semantic_turns_resolved);
  CuAssertIntEquals(tc, 1, event_queue_depth());

  encounter_test_leave(&slower, COMBAT_ENCOUNTER_DEPARTURE_STOPPED);
  encounter_test_leave(&faster, COMBAT_ENCOUNTER_DEPARTURE_STOPPED);
  encounter_test_end(saved_pulse);
}

void Test_combat_semantic_round_starts_six_seconds_after_idle_join(CuTest *tc)
{
  struct char_data first;
  struct char_data second;
  struct encounter_test_trace trace;
  struct combat_encounter_stats stats;
  unsigned long saved_pulse;
  const unsigned long start_pulse = 8500U;
  const unsigned long joined_pulse = start_pulse + (3 RL_SEC);

  encounter_test_character(&first, "idle join combatant");
  encounter_test_character(&second, "idle join target");
  saved_pulse = encounter_test_begin_semantic(tc, start_pulse, &trace);
  combat_encounter_test_set_phase_callback(encounter_test_record_phase, &trace);
  pulse = joined_pulse;
  FIGHTING(&first) = &second;
  FIGHTING(&second) = &first;
  CuAssertTrue(tc, combat_encounter_join(&first, &second, 1 RL_SEC));
  CuAssertTrue(tc, combat_encounter_join(&second, &first, 1 RL_SEC));

  pulse = joined_pulse + (5 RL_SEC);
  event_process();
  CuAssertIntEquals(tc, 0, (int)trace.count);
  pulse = joined_pulse + (6 RL_SEC);
  event_process();
  CuAssertIntEquals(tc, 2, (int)trace.count);
  combat_encounter_get_stats(&stats);
  CuAssertIntEquals(tc, 1, (int)stats.semantic_rounds_resolved);
  CuAssertIntEquals(tc, 2, (int)stats.semantic_turns_resolved);

  encounter_test_leave(&first, COMBAT_ENCOUNTER_DEPARTURE_STOPPED);
  encounter_test_leave(&second, COMBAT_ENCOUNTER_DEPARTURE_STOPPED);
  encounter_test_end(saved_pulse);
}

void Test_combat_semantic_round_uses_dexterity_and_runtime_id_tiebreaks(CuTest *tc)
{
  struct char_data first;
  struct char_data second;
  struct char_data third;
  struct char_data *lower_runtime_id;
  struct char_data *higher_runtime_id;
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
  CuAssertTrue(tc, combat_encounter_join(&first, &second, 1 RL_SEC));
  CuAssertTrue(tc, combat_encounter_join(&second, &first, 1 RL_SEC));
  CuAssertTrue(tc, combat_encounter_join(&third, &first, 1 RL_SEC));
  if (domain_event_character_handle(&first).runtime_id <
      domain_event_character_handle(&third).runtime_id)
  {
    lower_runtime_id = &first;
    higher_runtime_id = &third;
  }
  else
  {
    lower_runtime_id = &third;
    higher_runtime_id = &first;
  }

  pulse = start_pulse + (6 RL_SEC);
  event_process();
  CuAssertIntEquals(tc, 3, (int)trace.count);
  CuAssertPtrEquals(tc, &second, trace.characters[0]);
  CuAssertPtrEquals(tc, lower_runtime_id, trace.characters[1]);
  CuAssertPtrEquals(tc, higher_runtime_id, trace.characters[2]);

  encounter_test_leave(&first, COMBAT_ENCOUNTER_DEPARTURE_STOPPED);
  encounter_test_leave(&second, COMBAT_ENCOUNTER_DEPARTURE_STOPPED);
  encounter_test_leave(&third, COMBAT_ENCOUNTER_DEPARTURE_STOPPED);
  encounter_test_end(saved_pulse);
}

void Test_combat_semantic_round_owns_action_and_reaction_budgets(CuTest *tc)
{
  struct char_data first;
  struct char_data second;
  struct encounter_test_trace trace;
  struct combat_encounter_stats stats;
  unsigned long saved_pulse;
  bool available;
  bool managed;
  const unsigned long start_pulse = 10000U;

  encounter_test_character(&first, "budget combatant");
  encounter_test_character(&second, "budget target");
  saved_pulse = encounter_test_begin_semantic(tc, start_pulse, &trace);
  combat_encounter_test_set_phase_callback(encounter_test_record_phase, &trace);
  FIGHTING(&first) = &second;
  CuAssertTrue(tc, combat_encounter_join(&first, &second, 1 RL_SEC));

  CuAssertTrue(tc, combat_encounter_action_query(&first, atSTANDARD, &available));
  CuAssertTrue(tc, available);
  CuAssertTrue(tc, combat_encounter_action_consume(&first, atSTANDARD, 6 RL_SEC));
  CuAssertTrue(tc, combat_encounter_action_consume(&first, atMOVE, 12 RL_SEC));
  CuAssertTrue(tc, combat_encounter_action_query(&first, atSTANDARD, &available));
  CuAssertTrue(tc, !available);
  CuAssertTrue(tc, combat_encounter_reaction_refund(&first));
  CuAssertIntEquals(tc, -1, first.char_specials.attacks_of_opportunity);
  CuAssertTrue(tc, combat_encounter_reaction_try_use(&first, 0U, &managed));
  CuAssertIntEquals(tc, 0, first.char_specials.attacks_of_opportunity);
  CuAssertTrue(tc, combat_encounter_reaction_try_use(&first, 2U, &managed));
  CuAssertTrue(tc, managed);
  CuAssertTrue(tc, combat_encounter_reaction_try_use(&first, 2U, &managed));
  CuAssertTrue(tc, !combat_encounter_reaction_try_use(&first, 2U, &managed));
  CuAssertTrue(tc, combat_encounter_reaction_refund(&first));
  CuAssertTrue(tc, combat_encounter_reaction_try_use(&first, 2U, &managed));

  pulse = start_pulse + (6 RL_SEC);
  event_process();
  CuAssertTrue(tc, combat_encounter_action_query(&first, atSTANDARD, &available));
  CuAssertTrue(tc, available);
  CuAssertTrue(tc, combat_encounter_action_query(&first, atMOVE, &available));
  CuAssertTrue(tc, !available);
  CuAssertTrue(tc, combat_encounter_reaction_try_use(&first, 2U, &managed));

  pulse = start_pulse + (12 RL_SEC);
  event_process();
  CuAssertTrue(tc, combat_encounter_action_query(&first, atMOVE, &available));
  CuAssertTrue(tc, available);
  combat_encounter_get_stats(&stats);
  CuAssertIntEquals(tc, 2, (int)stats.action_budgets_spent);
  CuAssertIntEquals(tc, 5, (int)stats.reactions_spent);

  encounter_test_leave(&first, COMBAT_ENCOUNTER_DEPARTURE_STOPPED);
  encounter_test_leave(&second, COMBAT_ENCOUNTER_DEPARTURE_STOPPED);
  encounter_test_end(saved_pulse);
}

void Test_combat_semantic_round_resets_shared_state_before_initiative(CuTest *tc)
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
  CuAssertTrue(tc, combat_encounter_join(&first, &second, 1 RL_SEC));
  CuAssertTrue(tc, combat_encounter_join(&second, &first, 1 RL_SEC));

  pulse = start_pulse + (6 RL_SEC);
  event_process();
  CuAssertTrue(tc, trace.second_flag_survived);
  CuAssertTrue(tc, trace.second_reaction_remained_spent);

  encounter_test_leave(&first, COMBAT_ENCOUNTER_DEPARTURE_STOPPED);
  encounter_test_leave(&second, COMBAT_ENCOUNTER_DEPARTURE_STOPPED);
  encounter_test_end(saved_pulse);
}

void Test_combat_semantic_round_resets_participant_round_flags(CuTest *tc)
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
  CuAssertTrue(tc, combat_encounter_join(&first, &second, 1 RL_SEC));
  CuAssertTrue(tc, combat_encounter_round_flag_mark(
                       &first, COMBAT_ENCOUNTER_ROUND_DEFLECTIVE_SCREEN_USED));
  CuAssertTrue(tc, combat_encounter_round_flag_query(
                       &first, COMBAT_ENCOUNTER_ROUND_DEFLECTIVE_SCREEN_USED, &used));
  CuAssertTrue(tc, used);

  pulse = start_pulse + (6 RL_SEC);
  event_process();
  CuAssertTrue(tc, combat_encounter_round_flag_query(
                       &first, COMBAT_ENCOUNTER_ROUND_DEFLECTIVE_SCREEN_USED, &used));
  CuAssertTrue(tc, !used);

  encounter_test_leave(&first, COMBAT_ENCOUNTER_DEPARTURE_STOPPED);
  encounter_test_leave(&second, COMBAT_ENCOUNTER_DEPARTURE_STOPPED);
  encounter_test_end(saved_pulse);
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
  CuAssertTrue(tc, combat_encounter_join(&first, &second, 1 RL_SEC));

  start_action_cooldown(&first, atSTANDARD, 6 RL_SEC);
  CuAssertTrue(tc, combat_encounter_action_query(&first, atSTANDARD, &available));
  CuAssertTrue(tc, !available);
  CuAssertTrue(tc, combat_encounter_action_query(&first, atMOVE, &available));
  CuAssertTrue(tc, !available);
  pulse = 11500U + (6 RL_SEC);
  event_process();
  CuAssertTrue(tc, combat_encounter_action_query(&first, atSTANDARD, &available));
  CuAssertTrue(tc, available);
  CuAssertTrue(tc, combat_encounter_action_query(&first, atMOVE, &available));
  CuAssertTrue(tc, available);

  encounter_test_leave(&first, COMBAT_ENCOUNTER_DEPARTURE_STOPPED);
  encounter_test_leave(&second, COMBAT_ENCOUNTER_DEPARTURE_STOPPED);
  encounter_test_end(saved_pulse);
}

void Test_combat_semantic_round_defers_callback_joins_to_next_round(CuTest *tc)
{
  struct char_data first;
  struct char_data anchor;
  struct char_data joiner;
  struct encounter_test_trace trace;
  unsigned long saved_pulse;
  size_t index;
  bool saw_joiner = false;
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
  CuAssertTrue(tc, combat_encounter_join(&first, &anchor, 1 RL_SEC));

  pulse = start_pulse + (6 RL_SEC);
  event_process();
  CuAssertTrue(tc, trace.mutation_succeeded);
  CuAssertIntEquals(tc, 1, (int)trace.count);
  pulse = start_pulse + (11 RL_SEC);
  event_process();
  CuAssertIntEquals(tc, 1, (int)trace.count);
  pulse = start_pulse + (12 RL_SEC);
  event_process();
  for (index = 0U; index < trace.count; index++)
    if (trace.characters[index] == &joiner)
      saw_joiner = true;
  CuAssertTrue(tc, saw_joiner);

  encounter_test_leave(&first, COMBAT_ENCOUNTER_DEPARTURE_STOPPED);
  encounter_test_leave(&anchor, COMBAT_ENCOUNTER_DEPARTURE_STOPPED);
  encounter_test_leave(&joiner, COMBAT_ENCOUNTER_DEPARTURE_STOPPED);
  encounter_test_end(saved_pulse);
}

void Test_combat_semantic_round_transfers_action_cooldown_across_combat(CuTest *tc)
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
  CuAssertTrue(tc, combat_encounter_join(&first, &second, 1 RL_SEC));
  CuAssertPtrEquals(tc, NULL, char_has_mud_event(&first, eSTANDARDACTION));
  CuAssertTrue(tc, combat_encounter_action_query(&first, atSTANDARD, &available));
  CuAssertTrue(tc, !available);

  encounter_test_leave(&first, COMBAT_ENCOUNTER_DEPARTURE_STOPPED);
  restored = char_has_mud_event(&first, eSTANDARDACTION);
  CuAssertPtrNotNull(tc, restored);
  CuAssertTrue(tc, mud_event_remaining(restored) >= 12 RL_SEC);
  encounter_test_leave(&second, COMBAT_ENCOUNTER_DEPARTURE_STOPPED);
  encounter_test_end(saved_pulse);
}

void Test_combat_semantic_join_uses_next_shared_round(CuTest *tc)
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
  CuAssertTrue(tc, combat_encounter_join(&first, &anchor, 1 RL_SEC));

  pulse = start_pulse + (3 RL_SEC);
  FIGHTING(&joiner) = &anchor;
  CuAssertTrue(tc, combat_encounter_join(&joiner, &anchor, 1 RL_SEC));
  pulse = start_pulse + (6 RL_SEC);
  event_process();

  CuAssertIntEquals(tc, 2, (int)trace.count);
  CuAssertPtrEquals(tc, &joiner, trace.characters[0]);
  CuAssertPtrEquals(tc, &first, trace.characters[1]);
  CuAssertIntEquals(tc, 1, event_queue_depth());

  encounter_test_leave(&first, COMBAT_ENCOUNTER_DEPARTURE_STOPPED);
  encounter_test_leave(&anchor, COMBAT_ENCOUNTER_DEPARTURE_STOPPED);
  encounter_test_leave(&joiner, COMBAT_ENCOUNTER_DEPARTURE_STOPPED);
  encounter_test_end(saved_pulse);
}

void Test_combat_semantic_merge_coalesces_offset_clocks(CuTest *tc)
{
  struct char_data first;
  struct char_data first_anchor;
  struct char_data second;
  struct char_data second_anchor;
  struct encounter_test_trace trace;
  unsigned long saved_pulse;
  size_t count_after_first_round;
  const unsigned long start_pulse = 15000U;

  encounter_test_character(&first, "first clock combatant");
  encounter_test_character(&first_anchor, "first clock anchor");
  encounter_test_character(&second, "second clock combatant");
  encounter_test_character(&second_anchor, "second clock anchor");
  saved_pulse = encounter_test_begin_semantic(tc, start_pulse, &trace);
  combat_encounter_test_set_phase_callback(encounter_test_record_phase, &trace);
  FIGHTING(&first) = &first_anchor;
  FIGHTING(&first_anchor) = &first;
  CuAssertTrue(tc, combat_encounter_join(&first, &first_anchor, 1 RL_SEC));
  CuAssertTrue(tc, combat_encounter_join(&first_anchor, &first, 1 RL_SEC));

  pulse = start_pulse + (2 RL_SEC);
  FIGHTING(&second) = &second_anchor;
  FIGHTING(&second_anchor) = &second;
  CuAssertTrue(tc, combat_encounter_join(&second, &second_anchor, 1 RL_SEC));
  CuAssertTrue(tc, combat_encounter_join(&second_anchor, &second, 1 RL_SEC));
  CuAssertIntEquals(tc, 2, event_queue_depth());

  pulse = start_pulse + (3 RL_SEC);
  FIGHTING(&first) = &second;
  FIGHTING(&second) = &first;
  CuAssertTrue(tc, combat_encounter_join(&first, &second, 1 RL_SEC));
  CuAssertIntEquals(tc, 1, event_queue_depth());

  pulse = start_pulse + (6 RL_SEC);
  event_process();
  count_after_first_round = trace.count;
  CuAssertIntEquals(tc, 2, (int)count_after_first_round);
  pulse = start_pulse + (8 RL_SEC);
  event_process();
  CuAssertIntEquals(tc, (int)count_after_first_round, (int)trace.count);
  pulse = start_pulse + (12 RL_SEC);
  event_process();
  CuAssertIntEquals(tc, 6, (int)trace.count);
  CuAssertIntEquals(tc, 1, event_queue_depth());

  encounter_test_leave(&first, COMBAT_ENCOUNTER_DEPARTURE_STOPPED);
  encounter_test_leave(&first_anchor, COMBAT_ENCOUNTER_DEPARTURE_STOPPED);
  encounter_test_leave(&second, COMBAT_ENCOUNTER_DEPARTURE_STOPPED);
  encounter_test_leave(&second_anchor, COMBAT_ENCOUNTER_DEPARTURE_STOPPED);
  encounter_test_end(saved_pulse);
}

void Test_combat_semantic_round_dispatches_one_buffered_intent(CuTest *tc)
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
  CuAssertTrue(tc, combat_encounter_join(&first, &second, 1 RL_SEC));
  encounter_test_enqueue(&first, "not-a-real-command one");
  encounter_test_enqueue(&first, "not-a-real-command two");

  execute_next_action(&first);
  CuAssertIntEquals(tc, 2, pending_actions(&first));
  pulse = start_pulse + (6 RL_SEC);
  event_process();
  CuAssertIntEquals(tc, 1, pending_actions(&first));
  execute_next_action(&first);
  CuAssertIntEquals(tc, 1, pending_actions(&first));
  pulse = start_pulse + (12 RL_SEC);
  event_process();
  CuAssertIntEquals(tc, 0, pending_actions(&first));
  combat_encounter_get_stats(&stats);
  CuAssertIntEquals(tc, 2, (int)stats.intents_dispatched);
  CuAssertIntEquals(tc, 2, (int)stats.intent_dispatch_blocks);

  encounter_test_leave(&first, COMBAT_ENCOUNTER_DEPARTURE_STOPPED);
  encounter_test_leave(&second, COMBAT_ENCOUNTER_DEPARTURE_STOPPED);
  free_action_queue(GET_QUEUE(&first));
  GET_QUEUE(&first) = NULL;
  if (created_command_list)
    free_command_list();
  encounter_test_end(saved_pulse);
}
