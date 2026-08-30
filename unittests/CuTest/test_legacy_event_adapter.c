#include "CuTest.h"

#include "../../src/conf.h"
#include "../../src/sysdep.h"
#include "../../src/structs.h"
#include "../../src/utils.h"
#include "../../src/comm.h"
#include "../../src/dgscript/dg_event.h"
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
      {"Quest Completed!", 1},
      {"Falling", 5},
      {"Spell Preparation", 10},
      {"Combat Round", 20},
      {"Encounter Region Reset", 600},
      {"Magic Food", 3000},
      {"Mob Purge", 14400},
      {"Midnight Edict", 864000},
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
