#include "CuTest.h"

#include "../../src/conf.h"
#include "../../src/sysdep.h"
#include "../../src/structs.h"
#include "../../src/comm.h"
#include "../../src/db.h"
#include "../../src/combat/combat_encounters.h"
#include "../../src/combat/fight.h"
#include "../../src/dgscript/dg_event.h"

#include <string.h>

#define ENCOUNTER_TRACE_CAPACITY 64U

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
  bool mutation_ran;
  bool mutation_succeeded;
};

static void encounter_test_character(struct char_data *character, const char *name)
{
  clear_char(character);
  character->player.name = (char *)name;
}

static unsigned long encounter_test_begin(CuTest *tc, bool encounter_mode,
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
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK, combat_encounter_runtime_init(NULL));
  if (trace != NULL)
  {
    memset(trace, 0, sizeof(*trace));
    combat_encounter_test_set_phase_callback(NULL, NULL);
  }
  return saved_pulse;
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

static void encounter_test_leave(struct char_data *character,
                                 enum combat_encounter_departure_reason reason)
{
  FIGHTING(character) = NULL;
  combat_encounter_leave(character, reason);
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
