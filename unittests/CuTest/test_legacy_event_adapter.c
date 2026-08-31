#include "CuTest.h"

#include "../../src/conf.h"
#include "../../src/sysdep.h"
#include "../../src/structs.h"
#include "../../src/utils.h"
#include "../../src/comm.h"
#include "../../src/dgscript/dg_event.h"
#include "../../src/event_debug.h"
#include "../../src/perfmon.h"

#include <stdlib.h>
#include <string.h>

#define EVENT_TRACE_CAPACITY 8

struct event_trace
{
  char labels[EVENT_TRACE_CAPACITY];
  unsigned long pulses[EVENT_TRACE_CAPACITY];
  int count;
};

struct event_trace_payload
{
  struct event_trace *trace;
  char label;
  int runs_remaining;
  long recurrence_delay;
};

struct self_cancel_payload
{
  struct event *event;
  int *runs;
};

struct handle_cleanup_trace
{
  event_handle_t seen_handle;
  void *seen_payload;
  int calls;
  bool live_during_cleanup;
};

struct handle_self_cancel_payload
{
  event_handle_t handle;
  int *runs;
  struct handle_cleanup_trace *cleanup_trace;
};

struct reentrant_payload
{
  struct event_trace *trace;
  char before;
  char after;
  bool recurse;
};

struct workload_case
{
  const char *profile_name;
  long delay;
};

static EVENTFUNC(event_trace_callback)
{
  struct event_trace_payload *payload;

  payload = (struct event_trace_payload *)event_obj;
  if (payload->trace->count < EVENT_TRACE_CAPACITY)
  {
    payload->trace->labels[payload->trace->count] = payload->label;
    payload->trace->pulses[payload->trace->count] = pulse;
    payload->trace->count++;
  }

  payload->runs_remaining--;
  if (payload->runs_remaining > 0)
    return payload->recurrence_delay;

  free(payload);
  return 0;
}

static EVENTFUNC(self_cancel_callback)
{
  struct self_cancel_payload *payload;

  payload = (struct self_cancel_payload *)event_obj;
  (*payload->runs)++;
  event_cancel(payload->event);
  free(payload);
  return 3;
}

static EVENTFUNC(handle_self_cancel_callback)
{
  struct handle_self_cancel_payload *payload;

  payload = (struct handle_self_cancel_payload *)event_obj;
  (*payload->runs)++;
  event_handle_cancel(payload->handle);
  return 3;
}

static EVENTFUNC(workload_callback)
{
  free(event_obj);
  return 0;
}

static EVENTFUNC(reentrant_callback)
{
  struct reentrant_payload *payload;

  payload = (struct reentrant_payload *)event_obj;
  payload->trace->labels[payload->trace->count++] = payload->before;
  if (payload->recurse)
  {
    event_process();
    payload->trace->labels[payload->trace->count++] = payload->after;
  }
  free(payload);
  return 0;
}

static void counted_event_cleanup(struct event *event)
{
  int *cleanup_count;

  cleanup_count = (int *)event->event_obj;
  (*cleanup_count)++;
}

static void traced_handle_cleanup(event_handle_t handle, void *event_obj)
{
  struct handle_cleanup_trace *trace;

  trace = (struct handle_cleanup_trace *)event_obj;
  trace->seen_handle = handle;
  trace->seen_payload = event_obj;
  trace->live_during_cleanup = event_handle_is_live(handle);
  trace->calls++;
}

static void self_cancel_handle_cleanup(event_handle_t handle, void *event_obj)
{
  struct handle_self_cancel_payload *payload;
  struct handle_cleanup_trace *trace;

  payload = (struct handle_self_cancel_payload *)event_obj;
  trace = payload->cleanup_trace;
  trace->seen_handle = handle;
  trace->seen_payload = event_obj;
  trace->live_during_cleanup = event_handle_is_live(handle);
  trace->calls++;
  free(payload);
}

static struct event_trace_payload *new_trace_payload(struct event_trace *trace, char label,
                                                     int runs, long recurrence_delay)
{
  struct event_trace_payload *payload;

  payload = malloc(sizeof(*payload));
  if (payload == NULL)
    return NULL;
  payload->trace = trace;
  payload->label = label;
  payload->runs_remaining = runs;
  payload->recurrence_delay = recurrence_delay;
  return payload;
}

static void begin_backend_test(CuTest *tc, enum event_backend_kind backend,
                               unsigned long start_pulse)
{
  event_free_all();
  pulse = start_pulse;
  CuAssertIntEquals(tc, 1, event_test_select_backend(backend));
  event_init();
  CuAssertIntEquals(tc, backend, event_backend_current());
}

static void run_parity_trace(CuTest *tc, enum event_backend_kind backend, struct event_trace *trace)
{
  struct event_trace_payload *payload;
  struct event *first;
  struct event *recurring;
  struct event *third;
  unsigned long current;

  memset(trace, 0, sizeof(*trace));
  begin_backend_test(tc, backend, 100U);

  payload = new_trace_payload(trace, 'A', 1, 0);
  CuAssertPtrNotNull(tc, payload);
  first = event_create_named(event_trace_callback, payload, 2, "parity_first");
  CuAssertPtrNotNull(tc, first);

  payload = new_trace_payload(trace, 'B', 2, 2);
  CuAssertPtrNotNull(tc, payload);
  recurring = event_create_named(event_trace_callback, payload, 1, "parity_recurring");
  CuAssertPtrNotNull(tc, recurring);

  payload = new_trace_payload(trace, 'C', 1, 0);
  CuAssertPtrNotNull(tc, payload);
  third = event_create_named(event_trace_callback, payload, 2, "parity_third");
  CuAssertPtrNotNull(tc, third);

  CuAssertIntEquals(tc, 3, event_queue_depth());
  CuAssertIntEquals(tc, 2, (int)event_time(first));
  CuAssertIntEquals(tc, 1, (int)event_time(recurring));
  CuAssertIntEquals(tc, 2, (int)event_time(third));
  CuAssertIntEquals(tc, 1, event_is_queued(first));
  CuAssertIntEquals(tc, 1, event_is_queued(recurring));
  CuAssertIntEquals(tc, 1, event_is_queued(third));

  for (current = 101U; current <= 103U; current++)
  {
    pulse = current;
    event_process();
  }

  CuAssertIntEquals(tc, 0, event_queue_depth());
  event_free_all();
}

void Test_legacy_event_backends_produce_identical_order_and_recurrence(CuTest *tc)
{
  struct event_trace legacy_trace;
  struct event_trace scheduler_trace;
  unsigned long saved_pulse;
  int index;

  saved_pulse = pulse;
  run_parity_trace(tc, EVENT_BACKEND_LEGACY_QUEUE, &legacy_trace);
  run_parity_trace(tc, EVENT_BACKEND_GAME_SCHEDULER, &scheduler_trace);

  CuAssertIntEquals(tc, 4, legacy_trace.count);
  CuAssertIntEquals(tc, legacy_trace.count, scheduler_trace.count);
  for (index = 0; index < legacy_trace.count; index++)
  {
    CuAssertIntEquals(tc, legacy_trace.labels[index], scheduler_trace.labels[index]);
    CuAssertTrue(tc, legacy_trace.pulses[index] == scheduler_trace.pulses[index]);
  }
  CuAssertIntEquals(tc, 'B', legacy_trace.labels[0]);
  CuAssertIntEquals(tc, 'A', legacy_trace.labels[1]);
  CuAssertIntEquals(tc, 'C', legacy_trace.labels[2]);
  CuAssertIntEquals(tc, 'B', legacy_trace.labels[3]);
  pulse = saved_pulse;
}

void Test_legacy_event_scheduler_uses_live_pulse_after_idle(CuTest *tc)
{
  struct event_trace trace;
  struct event_trace_payload *payload;
  struct event *event;
  unsigned long saved_pulse;

  saved_pulse = pulse;
  memset(&trace, 0, sizeof(trace));
  begin_backend_test(tc, EVENT_BACKEND_GAME_SCHEDULER, 100U);
  pulse = 500U;
  payload = new_trace_payload(&trace, 'A', 1, 0);
  CuAssertPtrNotNull(tc, payload);
  event = event_create_named(event_trace_callback, payload, 6, "live-pulse-deadline");
  CuAssertPtrNotNull(tc, event);
  CuAssertIntEquals(tc, 6, (int)event_time(event));

  pulse = 505U;
  event_process();
  CuAssertIntEquals(tc, 0, trace.count);
  CuAssertIntEquals(tc, 1, (int)event_time(event));
  pulse = 506U;
  event_process();
  CuAssertIntEquals(tc, 1, trace.count);
  CuAssertTrue(tc, trace.pulses[0] == 506U);
  CuAssertIntEquals(tc, 0, event_queue_depth());

  event_free_all();
  pulse = saved_pulse;
}

static void verify_external_cancel(CuTest *tc, enum event_backend_kind backend)
{
  struct event *event;
  int cleanup_count;

  cleanup_count = 0;
  begin_backend_test(tc, backend, 200U);
  event = event_create_with_cleanup(workload_callback, &cleanup_count, 10, counted_event_cleanup);
  CuAssertPtrNotNull(tc, event);
  event_cancel(event);
  CuAssertIntEquals(tc, 1, cleanup_count);
  CuAssertIntEquals(tc, 0, event_queue_depth());
  event_free_all();
}

void Test_legacy_event_backends_cancel_and_cleanup_once(CuTest *tc)
{
  unsigned long saved_pulse;

  saved_pulse = pulse;
  verify_external_cancel(tc, EVENT_BACKEND_LEGACY_QUEUE);
  verify_external_cancel(tc, EVENT_BACKEND_GAME_SCHEDULER);
  pulse = saved_pulse;
}

static void verify_opaque_handle_lifecycle(CuTest *tc, enum event_backend_kind backend)
{
  struct handle_cleanup_trace cleanup_trace;
  struct handle_cleanup_trace self_cancel_trace;
  struct handle_cleanup_trace shutdown_trace;
  struct handle_self_cancel_payload *self_cancel;
  event_handle_t cancelled;
  event_handle_t completed;
  event_handle_t exhausted;
  event_handle_t exhaustion_seed;
  event_handle_t post_exhaustion;
  event_handle_t self_cancelled;
  event_handle_t shutdown_handle;
  void *completion_payload;
  void *exhaustion_payload;
  void *post_exhaustion_payload;
  int self_cancel_runs;

  memset(&cleanup_trace, 0, sizeof(cleanup_trace));
  memset(&self_cancel_trace, 0, sizeof(self_cancel_trace));
  memset(&shutdown_trace, 0, sizeof(shutdown_trace));
  self_cancel_runs = 0;
  begin_backend_test(tc, backend, 250U);

  cancelled = event_schedule_named_with_cleanup(workload_callback, &cleanup_trace, 10,
                                                 "opaque-cancel", traced_handle_cleanup);
  CuAssertTrue(tc, cancelled != EVENT_HANDLE_NONE);
  CuAssertTrue(tc, event_handle_is_live(cancelled));
  CuAssertTrue(tc, event_handle_is_queued(cancelled));
  CuAssertIntEquals(tc, 10, (int)event_handle_time(cancelled));
  CuAssertTrue(tc, event_handle_cancel(cancelled));
  CuAssertIntEquals(tc, 1, cleanup_trace.calls);
  CuAssertTrue(tc, cleanup_trace.seen_handle == cancelled);
  CuAssertPtrEquals(tc, &cleanup_trace, cleanup_trace.seen_payload);
  CuAssertTrue(tc, cleanup_trace.live_during_cleanup);
  CuAssertTrue(tc, !event_handle_is_live(cancelled));
  CuAssertTrue(tc, !event_handle_is_queued(cancelled));
  CuAssertIntEquals(tc, 0, (int)event_handle_time(cancelled));
  CuAssertTrue(tc, !event_handle_cancel(cancelled));

  completion_payload = malloc(1U);
  CuAssertPtrNotNull(tc, completion_payload);
  completed = event_schedule(workload_callback, completion_payload, 1);
  CuAssertTrue(tc, completed != EVENT_HANDLE_NONE);
  CuAssertTrue(tc, completed != cancelled);
  CuAssertIntEquals(tc, (int)event_test_handle_slot(cancelled),
                    (int)event_test_handle_slot(completed));
  pulse = 251U;
  event_process();
  CuAssertTrue(tc, !event_handle_is_live(completed));

  exhaustion_payload = malloc(1U);
  CuAssertPtrNotNull(tc, exhaustion_payload);
  exhaustion_seed = event_schedule(workload_callback, exhaustion_payload, 10);
  CuAssertTrue(tc, exhaustion_seed != EVENT_HANDLE_NONE);
  exhausted = event_test_force_handle_generation_exhaustion(exhaustion_seed);
  CuAssertTrue(tc, exhausted != EVENT_HANDLE_NONE);
  CuAssertTrue(tc, exhausted != exhaustion_seed);
  CuAssertTrue(tc, !event_handle_is_live(exhaustion_seed));
  CuAssertTrue(tc, !event_handle_cancel(exhaustion_seed));
  CuAssertTrue(tc, event_handle_is_live(exhausted));
  CuAssertTrue(tc, event_handle_cancel(exhausted));
  CuAssertTrue(tc, !event_handle_is_live(exhausted));

  post_exhaustion_payload = malloc(1U);
  CuAssertPtrNotNull(tc, post_exhaustion_payload);
  post_exhaustion = event_schedule(workload_callback, post_exhaustion_payload, 10);
  CuAssertTrue(tc, post_exhaustion != EVENT_HANDLE_NONE);
  CuAssertTrue(tc, event_test_handle_slot(post_exhaustion) != event_test_handle_slot(exhausted));
  CuAssertTrue(tc, event_handle_cancel(post_exhaustion));

  self_cancel = malloc(sizeof(*self_cancel));
  CuAssertPtrNotNull(tc, self_cancel);
  self_cancel->handle = EVENT_HANDLE_NONE;
  self_cancel->runs = &self_cancel_runs;
  self_cancel->cleanup_trace = &self_cancel_trace;
  self_cancelled = event_schedule_named_with_cleanup(
      handle_self_cancel_callback, self_cancel, 1, "opaque-self-cancel",
      self_cancel_handle_cleanup);
  CuAssertTrue(tc, self_cancelled != EVENT_HANDLE_NONE);
  self_cancel->handle = self_cancelled;
  pulse = 252U;
  event_process();
  pulse = 255U;
  event_process();
  CuAssertIntEquals(tc, 1, self_cancel_runs);
  CuAssertIntEquals(tc, 1, self_cancel_trace.calls);
  CuAssertTrue(tc, self_cancel_trace.seen_handle == self_cancelled);
  CuAssertPtrEquals(tc, self_cancel, self_cancel_trace.seen_payload);
  CuAssertTrue(tc, self_cancel_trace.live_during_cleanup);
  CuAssertTrue(tc, !event_handle_is_live(self_cancelled));
  CuAssertIntEquals(tc, 0, event_queue_depth());

  shutdown_handle = event_schedule_named_with_cleanup(
      workload_callback, &shutdown_trace, 10, "opaque-shutdown", traced_handle_cleanup);
  CuAssertTrue(tc, shutdown_handle != EVENT_HANDLE_NONE);
  event_free_all();
  CuAssertIntEquals(tc, 1, shutdown_trace.calls);
  CuAssertTrue(tc, shutdown_trace.seen_handle == shutdown_handle);
  CuAssertPtrEquals(tc, &shutdown_trace, shutdown_trace.seen_payload);
  CuAssertTrue(tc, shutdown_trace.live_during_cleanup);
  CuAssertTrue(tc, !event_handle_is_live(shutdown_handle));
}

void Test_event_opaque_handles_are_generation_safe_on_both_backends(CuTest *tc)
{
  unsigned long saved_pulse;

  saved_pulse = pulse;
  verify_opaque_handle_lifecycle(tc, EVENT_BACKEND_LEGACY_QUEUE);
  verify_opaque_handle_lifecycle(tc, EVENT_BACKEND_GAME_SCHEDULER);
  pulse = saved_pulse;
}

static void verify_self_cancel(CuTest *tc, enum event_backend_kind backend)
{
  struct self_cancel_payload *payload;
  int runs;

  runs = 0;
  begin_backend_test(tc, backend, 300U);
  payload = malloc(sizeof(*payload));
  CuAssertPtrNotNull(tc, payload);
  payload->event = NULL;
  payload->runs = &runs;
  payload->event = event_create(self_cancel_callback, payload, 1);
  CuAssertPtrNotNull(tc, payload->event);

  pulse = 301U;
  event_process();
  pulse = 304U;
  event_process();

  CuAssertIntEquals(tc, 1, runs);
  CuAssertIntEquals(tc, 0, event_queue_depth());
  event_free_all();
}

void Test_legacy_event_self_cancel_wins_over_positive_return(CuTest *tc)
{
  unsigned long saved_pulse;

  saved_pulse = pulse;
  verify_self_cancel(tc, EVENT_BACKEND_LEGACY_QUEUE);
  verify_self_cancel(tc, EVENT_BACKEND_GAME_SCHEDULER);
  pulse = saved_pulse;
}

static void verify_reentrant_process_rejected(CuTest *tc, enum event_backend_kind backend)
{
  struct reentrant_payload *first;
  struct reentrant_payload *second;
  struct event_trace trace;

  memset(&trace, 0, sizeof(trace));
  begin_backend_test(tc, backend, 350U);
  first = malloc(sizeof(*first));
  second = malloc(sizeof(*second));
  CuAssertPtrNotNull(tc, first);
  CuAssertPtrNotNull(tc, second);
  first->trace = &trace;
  first->before = 'A';
  first->after = 'a';
  first->recurse = true;
  second->trace = &trace;
  second->before = 'B';
  second->after = '\0';
  second->recurse = false;
  CuAssertPtrNotNull(tc, event_create(reentrant_callback, first, 1));
  CuAssertPtrNotNull(tc, event_create(reentrant_callback, second, 1));

  pulse = 351U;
  event_process();

  CuAssertIntEquals(tc, 3, trace.count);
  CuAssertIntEquals(tc, 'A', trace.labels[0]);
  CuAssertIntEquals(tc, 'a', trace.labels[1]);
  CuAssertIntEquals(tc, 'B', trace.labels[2]);
  event_free_all();
}

void Test_legacy_event_rejects_recursive_dispatch_on_both_backends(CuTest *tc)
{
  unsigned long saved_pulse;

  saved_pulse = pulse;
  verify_reentrant_process_rejected(tc, EVENT_BACKEND_LEGACY_QUEUE);
  verify_reentrant_process_rejected(tc, EVENT_BACKEND_GAME_SCHEDULER);
  pulse = saved_pulse;
}

void Test_legacy_event_source_workload_populates_private_telemetry(CuTest *tc)
{
  static const struct workload_case workload[] = {
      {"World Quest Completion", 1},
      {"World Falling", 5},
      {"Spell Preparation", 10},
      {"Cooldown Expiry", 20},
      {"DG Wait Resume", 600},
      {"AI Combat Round", 3000},
      {"Resource Regeneration", 14400},
      {"World Midnight Edict", 864000},
  };
  struct event *events[sizeof(workload) / sizeof(workload[0])];
  char report[32768];
  unsigned long saved_pulse;
  size_t index;

  saved_pulse = pulse;
  begin_backend_test(tc, EVENT_BACKEND_GAME_SCHEDULER, 400U);
  PERF_reset();
  memset(events, 0, sizeof(events));

  for (index = 0; index < sizeof(workload) / sizeof(workload[0]); index++)
  {
    int *cleanup_count;

    cleanup_count = malloc(sizeof(*cleanup_count));
    CuAssertPtrNotNull(tc, cleanup_count);
    *cleanup_count = 0;
    events[index] =
        event_create_named_with_cleanup(workload_callback, cleanup_count, workload[index].delay,
                                        workload[index].profile_name, counted_event_cleanup);
    CuAssertPtrNotNull(tc, events[index]);
    CuAssertIntEquals(tc, (int)workload[index].delay, (int)event_time(events[index]));
  }

  for (index = 0; index < sizeof(events) / sizeof(events[0]); index++)
  {
    int *cleanup_count;

    cleanup_count = (int *)events[index]->event_obj;
    event_cancel(events[index]);
    CuAssertIntEquals(tc, 1, *cleanup_count);
    free(cleanup_count);
  }

  PERF_prof_repr_csv(report, sizeof(report));
  CuAssertPtrNotNull(tc, strstr(report, "# events_scheduled=8"));
  CuAssertPtrNotNull(tc, strstr(report, "# events_cancelled=8"));
  CuAssertPtrNotNull(tc, strstr(report, "# events_rescheduled=0"));
  CuAssertPtrNotNull(tc, strstr(report, "# event_delay_pulses_le_1=1"));
  CuAssertPtrNotNull(tc, strstr(report, "# event_delay_pulses_2_10=2"));
  CuAssertPtrNotNull(tc, strstr(report, "# event_delay_pulses_11_60=1"));
  CuAssertPtrNotNull(tc, strstr(report, "# event_delay_pulses_61_600=1"));
  CuAssertPtrNotNull(tc, strstr(report, "# event_delay_pulses_601_6000=1"));
  CuAssertPtrNotNull(tc, strstr(report, "# event_delay_pulses_6001_36000=1"));
  CuAssertPtrNotNull(tc, strstr(report, "# event_delay_pulses_gt_36000=1"));

  event_free_all();
  pulse = saved_pulse;
}

void Test_legacy_event_scheduler_bridge_yields_without_dual_dispatch(CuTest *tc)
{
  struct game_scheduler_budget budget;
  struct game_scheduler_dispatch_report report;
  struct event_trace trace;
  struct event_trace_payload *payload;
  game_tick_t deadline;
  bool has_deadline;
  unsigned long saved_pulse;
  int index;

  saved_pulse = pulse;
  memset(&trace, 0, sizeof(trace));
  memset(&budget, 0, sizeof(budget));
  budget.max_callbacks = 1U;
  begin_backend_test(tc, EVENT_BACKEND_GAME_SCHEDULER, 500U);
  for (index = 0; index < 3; index++)
  {
    payload = new_trace_payload(&trace, (char)('A' + index), 1, 0);
    CuAssertPtrNotNull(tc, payload);
    CuAssertPtrNotNull(tc,
                       event_create_named(event_trace_callback, payload, 1, "bridge-storm"));
  }

  pulse = 501U;
  event_process_compatibility_pulse();
  CuAssertIntEquals(tc, 0, trace.count);
  CuAssertIntEquals(tc, GAME_SCHEDULER_OK,
                    event_scheduler_next_deadline(&deadline, &has_deadline));
  CuAssertTrue(tc, has_deadline);
  CuAssertTrue(tc, deadline == 501U);

  CuAssertIntEquals(tc, GAME_SCHEDULER_OK,
                    event_process_scheduler(&budget, &report));
  CuAssertIntEquals(tc, 1, trace.count);
  CuAssertIntEquals(tc, 2, (int)report.ready_remaining);
  CuAssertIntEquals(tc, GAME_SCHEDULER_OK,
                    event_process_scheduler(&budget, &report));
  CuAssertIntEquals(tc, 2, trace.count);
  CuAssertIntEquals(tc, 1, (int)report.ready_remaining);
  CuAssertIntEquals(tc, GAME_SCHEDULER_OK,
                    event_process_scheduler(&budget, &report));
  CuAssertIntEquals(tc, 3, trace.count);
  CuAssertIntEquals(tc, 0, (int)report.ready_remaining);
  CuAssertIntEquals(tc, 501, (int)pulse);

  event_process_compatibility_pulse();
  CuAssertIntEquals(tc, 3, trace.count);
  event_free_all();
  pulse = saved_pulse;
}

static void assert_debug_output_width(CuTest *tc, const char *output, size_t expected_width)
{
  const char *line;
  const char *cursor;
  size_t line_length;

  CuAssertPtrNotNull(tc, output);
  line = output;
  for (cursor = output; *cursor != '\0'; cursor++)
  {
    if (*cursor != '\r' && *cursor != '\n')
      continue;
    line_length = (size_t)(cursor - line);
    CuAssertTrue(tc, line_length <= expected_width);
    if (*cursor == '\r' && cursor[1] == '\n')
      cursor++;
    line = cursor + 1;
  }
  CuAssertTrue(tc, (size_t)(cursor - line) <= expected_width);
}

static void verify_event_debug_registry(CuTest *tc, enum event_backend_kind backend)
{
  struct game_event_owner owner;
  struct event_debug_filter filter;
  struct event_debug_snapshot snapshots[2];
  struct event_debug_stats stats;
  struct event *alpha;
  struct event *beta;
  char output[32768];
  size_t matched;
  size_t returned;
  size_t width_index;
  static const int widths[] = {40, 80, 120};

  begin_backend_test(tc, backend, 700U);
  PERF_reset();
  owner.kind = GAME_EVENT_OWNER_SERVICE;
  owner.runtime_id = 7U;
  owner.generation = 3U;
  alpha = event_create_owned_named(workload_callback, NULL, 20, "debug-alpha-type", owner);
  beta = event_create_named(workload_callback, NULL, 5, "debug-beta-type");
  CuAssertPtrNotNull(tc, alpha);
  CuAssertPtrNotNull(tc, beta);

  event_debug_get_stats(&stats);
  CuAssertIntEquals(tc, backend, stats.backend);
  CuAssertIntEquals(tc, 2, (int)stats.live_events);
  CuAssertIntEquals(tc, 2, (int)stats.high_water_events);
  CuAssertIntEquals(tc, 0, (int)stats.registry_mismatches);
  CuAssertTrue(tc, stats.stale_owner_outcomes == 0U);
  CuAssertIntEquals(tc, 1, (int)stats.owner_event_counts[GAME_EVENT_OWNER_NONE]);
  CuAssertIntEquals(tc, 1, (int)stats.owner_event_counts[GAME_EVENT_OWNER_SERVICE]);
  CuAssertIntEquals(tc, backend == EVENT_BACKEND_GAME_SCHEDULER,
                    stats.scheduler_stats_available);

  memset(&filter, 0, sizeof(filter));
  matched = event_debug_inspect(&filter, snapshots, 2U, &returned);
  CuAssertIntEquals(tc, 2, (int)matched);
  CuAssertIntEquals(tc, 2, (int)returned);
  CuAssertStrEquals(tc, "debug-beta-type", snapshots[0].type_name);
  CuAssertTrue(tc, snapshots[0].remaining_pulses == 5U);
  CuAssertStrEquals(tc, "debug-alpha-type", snapshots[1].type_name);
  CuAssertTrue(tc, snapshots[1].remaining_pulses == 20U);
  CuAssertIntEquals(tc, EVENT_DEBUG_QUEUED, snapshots[0].state);
  CuAssertIntEquals(tc, EVENT_DEBUG_QUEUED, snapshots[1].state);

  memset(&filter, 0, sizeof(filter));
  filter.type_contains = "ALPHA";
  CuAssertIntEquals(tc, 1, (int)event_debug_inspect(&filter, snapshots, 2U, &returned));
  CuAssertIntEquals(tc, 1, (int)returned);
  CuAssertStrEquals(tc, "debug-alpha-type", snapshots[0].type_name);

  memset(&filter, 0, sizeof(filter));
  filter.type_equals = "debug-beta-type";
  CuAssertIntEquals(tc, 1, (int)event_debug_inspect(&filter, snapshots, 2U, &returned));
  filter.type_equals = "debug-beta";
  CuAssertIntEquals(tc, 0, (int)event_debug_inspect(&filter, snapshots, 2U, &returned));

  memset(&filter, 0, sizeof(filter));
  filter.owner_set = true;
  filter.owner = owner;
  CuAssertIntEquals(tc, 1, (int)event_debug_inspect(&filter, snapshots, 2U, &returned));
  filter.owner_generation_set = true;
  filter.owner.generation = 4U;
  CuAssertIntEquals(tc, 0, (int)event_debug_inspect(&filter, snapshots, 2U, &returned));

  memset(&filter, 0, sizeof(filter));
  filter.maximum_remaining_set = true;
  filter.maximum_remaining = 5U;
  CuAssertIntEquals(tc, 1, (int)event_debug_inspect(&filter, snapshots, 2U, &returned));
  CuAssertStrEquals(tc, "debug-beta-type", snapshots[0].type_name);

  memset(&filter, 0, sizeof(filter));
  filter.minimum_remaining_set = true;
  filter.minimum_remaining = 10U;
  filter.maximum_remaining_set = true;
  filter.maximum_remaining = 20U;
  CuAssertIntEquals(tc, 1, (int)event_debug_inspect(&filter, snapshots, 2U, &returned));
  CuAssertStrEquals(tc, "debug-alpha-type", snapshots[0].type_name);

  memset(&filter, 0, sizeof(filter));
  filter.state_set = true;
  filter.state = EVENT_DEBUG_QUEUED;
  CuAssertIntEquals(tc, 2, (int)event_debug_inspect(&filter, snapshots, 2U, &returned));

  memset(&filter, 0, sizeof(filter));
  filter.event_id_set = true;
  filter.event_id = alpha->debug_id;
  CuAssertIntEquals(tc, 1, (int)event_debug_inspect(&filter, snapshots, 2U, &returned));
  CuAssertTrue(tc, snapshots[0].event_id == alpha->debug_id);

  memset(&filter, 0, sizeof(filter));
  for (width_index = 0; width_index < sizeof(widths) / sizeof(widths[0]); width_index++)
  {
    event_debug_render_help(output, sizeof(output), widths[width_index]);
    assert_debug_output_width(tc, output, (size_t)widths[width_index]);
    CuAssertPtrNotNull(tc, strstr(output, "eventdebug range"));
    event_debug_render_summary(output, sizeof(output), widths[width_index]);
    assert_debug_output_width(tc, output, (size_t)widths[width_index]);
    CuAssertPtrNotNull(tc, strstr(output, "Live events by owner"));
    event_debug_render_queue(output, sizeof(output), widths[width_index], &filter, 2U);
    assert_debug_output_width(tc, output, (size_t)widths[width_index]);
    CuAssertPtrNotNull(tc, strstr(output, "Payloads: redacted"));
    CuAssertPtrNotNull(tc, strstr(output, "debug-alpha-type"));
    CuAssertPtrNotNull(tc, strstr(output, "debug-beta-type"));
    event_debug_render_profiles(output, sizeof(output), widths[width_index], 10U);
    assert_debug_output_width(tc, output, (size_t)widths[width_index]);
    CuAssertPtrNotNull(tc, strstr(output, "  live: 1"));
    event_debug_render_domain(output, sizeof(output), widths[width_index], NULL);
    assert_debug_output_width(tc, output, (size_t)widths[width_index]);
  }

  event_cancel(alpha);
  event_cancel(beta);
  event_debug_get_stats(&stats);
  CuAssertIntEquals(tc, 0, (int)stats.live_events);
  CuAssertIntEquals(tc, 2, (int)stats.high_water_events);
  CuAssertIntEquals(tc, 0, (int)stats.registry_mismatches);
  event_note_stale_owner_outcome();
  event_debug_get_stats(&stats);
  CuAssertTrue(tc, stats.stale_owner_outcomes == 1U);
  event_free_all();
}

void Test_event_debug_registry_is_backend_neutral_filterable_and_width_bounded(CuTest *tc)
{
  unsigned long saved_pulse;

  saved_pulse = pulse;
  CuAssertIntEquals(tc, 80, event_debug_effective_width(0));
  CuAssertIntEquals(tc, 80, event_debug_effective_width(39));
  CuAssertIntEquals(tc, 40, event_debug_effective_width(40));
  CuAssertIntEquals(tc, 80, event_debug_effective_width(80));
  CuAssertIntEquals(tc, 120, event_debug_effective_width(121));
  verify_event_debug_registry(tc, EVENT_BACKEND_LEGACY_QUEUE);
  verify_event_debug_registry(tc, EVENT_BACKEND_GAME_SCHEDULER);
  pulse = saved_pulse;
}
