#include "CuTest.h"

#include "../../src/conf.h"
#include "../../src/sysdep.h"
#include "../../src/structs.h"
#include "../../src/comm.h"
#include "../../src/dgscript/dg_event.h"
#include "../../src/dgscript/dg_event_rollback.h"
#include "../../src/event_runtime.h"

#include <stdlib.h>
#include <string.h>

#define RUNTIME_TRACE_CAPACITY 8U

struct runtime_trace
{
  char entries[RUNTIME_TRACE_CAPACITY];
  size_t count;
};

struct runtime_payload
{
  struct runtime_trace *trace;
  char marker;
  int runs_remaining;
  int *cleanup_count;
};

static struct game_event_result runtime_trace_handler(
    const struct game_event_context *context)
{
  struct runtime_payload *payload;

  payload = context->payload;
  if (payload->trace != NULL && payload->trace->count < RUNTIME_TRACE_CAPACITY)
    payload->trace->entries[payload->trace->count++] = payload->marker;
  payload->runs_remaining--;
  return payload->runs_remaining > 0 ? game_event_result_reschedule_after(2U)
                                     : game_event_result_complete();
}

static void runtime_payload_cleanup(void *event_payload)
{
  struct runtime_payload *payload;

  payload = event_payload;
  if (payload->cleanup_count != NULL)
    (*payload->cleanup_count)++;
  free(payload);
}

static EVENTFUNC(compatibility_trace_handler)
{
  struct runtime_payload *payload;

  payload = event_obj;
  if (payload->trace != NULL && payload->trace->count < RUNTIME_TRACE_CAPACITY)
    payload->trace->entries[payload->trace->count++] = payload->marker;
  if (payload->cleanup_count != NULL)
    (*payload->cleanup_count)++;
  free(payload);
  return 0;
}

static struct runtime_payload *new_runtime_payload(struct runtime_trace *trace, char marker,
                                                   int runs, int *cleanup_count)
{
  struct runtime_payload *payload;

  payload = malloc(sizeof(*payload));
  if (payload == NULL)
    return NULL;
  payload->trace = trace;
  payload->marker = marker;
  payload->runs_remaining = runs;
  payload->cleanup_count = cleanup_count;
  return payload;
}

static void begin_runtime_test(CuTest *tc, unsigned long start_pulse)
{
  event_free_all();
  pulse = start_pulse;
  CuAssertIntEquals(tc, 1, event_test_select_backend(EVENT_BACKEND_GAME_SCHEDULER));
  event_init();
  CuAssertTrue(tc, event_runtime_is_initialized());
}

static game_event_type_id_t register_runtime_type(CuTest *tc, const char *name,
                                                  bool requires_owner)
{
  struct game_event_type_config config;
  game_event_type_id_t event_type;

  memset(&config, 0, sizeof(config));
  config.name = name;
  config.handler = runtime_trace_handler;
  config.cleanup = runtime_payload_cleanup;
  config.lateness_policy = GAME_EVENT_LATENESS_RUN_ONCE;
  config.requires_owner = requires_owner;
  event_type = 0;
  CuAssertIntEquals(tc, GAME_SCHEDULER_OK,
                    event_runtime_register_type(&config, &event_type));
  return event_type;
}

void Test_event_runtime_shares_one_sealed_wheel_with_compatibility_adapter(CuTest *tc)
{
  struct game_event_type_config rejected_config;
  struct game_scheduler_dispatch_report report;
  struct game_scheduler_stats stats;
  struct event_runtime_handle first_handle;
  struct event_runtime_handle second_handle;
  struct runtime_payload *payload;
  struct runtime_trace trace;
  game_event_type_id_t first_type;
  game_event_type_id_t second_type;
  game_event_type_id_t rejected_type;
  event_handle_t compatibility_handle;
  int native_cleanups;
  int compatibility_cleanups;
  unsigned long saved_pulse;

  saved_pulse = pulse;
  memset(&trace, 0, sizeof(trace));
  native_cleanups = 0;
  compatibility_cleanups = 0;
  begin_runtime_test(tc, 100U);
  first_type = register_runtime_type(tc, "test.native.regeneration", false);
  second_type = register_runtime_type(tc, "test.native.autonomous_action", false);
  CuAssertStrEquals(tc, "test.native.regeneration", event_runtime_type_name(first_type));
  CuAssertStrEquals(tc, "test.native.autonomous_action", event_runtime_type_name(second_type));
  CuAssertIntEquals(tc, GAME_SCHEDULER_OK, event_runtime_seal_types());
  CuAssertTrue(tc, event_runtime_types_are_sealed());

  memset(&rejected_config, 0, sizeof(rejected_config));
  rejected_config.name = "test.too_late";
  rejected_config.handler = runtime_trace_handler;
  rejected_config.cleanup = runtime_payload_cleanup;
  rejected_config.lateness_policy = GAME_EVENT_LATENESS_RUN_ONCE;
  rejected_type = 0;
  CuAssertIntEquals(tc, GAME_SCHEDULER_REGISTRATION_CLOSED,
                    event_runtime_register_type(&rejected_config, &rejected_type));

  payload = new_runtime_payload(&trace, 'A', 2, &native_cleanups);
  CuAssertPtrNotNull(tc, payload);
  CuAssertIntEquals(tc, GAME_SCHEDULER_OK,
                    event_runtime_schedule_after(first_type, 2U, payload, &first_handle));
  payload = new_runtime_payload(&trace, 'B', 1, &native_cleanups);
  CuAssertPtrNotNull(tc, payload);
  CuAssertIntEquals(tc, GAME_SCHEDULER_OK,
                    event_runtime_schedule_after(second_type, 3U, payload, &second_handle));
  payload = new_runtime_payload(&trace, 'L', 1, &compatibility_cleanups);
  CuAssertPtrNotNull(tc, payload);
  compatibility_handle =
      event_schedule_named(compatibility_trace_handler, payload, 1, "test.compatibility");
  CuAssertTrue(tc, compatibility_handle != EVENT_HANDLE_NONE);

  event_runtime_get_stats(&stats);
  CuAssertIntEquals(tc, 3, (int)stats.registered_type_count);
  CuAssertIntEquals(tc, 3, (int)stats.event_count);
  for (pulse = 101U; pulse <= 104U; pulse++)
  {
    memset(&report, 0, sizeof(report));
    CuAssertIntEquals(tc, GAME_SCHEDULER_OK, event_runtime_advance(NULL, &report));
  }

  CuAssertIntEquals(tc, 4, (int)trace.count);
  CuAssertIntEquals(tc, 'L', trace.entries[0]);
  CuAssertIntEquals(tc, 'A', trace.entries[1]);
  CuAssertIntEquals(tc, 'B', trace.entries[2]);
  CuAssertIntEquals(tc, 'A', trace.entries[3]);
  CuAssertIntEquals(tc, 2, native_cleanups);
  CuAssertIntEquals(tc, 1, compatibility_cleanups);
  CuAssertTrue(tc, !event_runtime_handle_is_live(first_handle));
  CuAssertTrue(tc, !event_runtime_handle_is_live(second_handle));
  CuAssertTrue(tc, !event_handle_is_live(compatibility_handle));
  event_free_all();
  pulse = saved_pulse;
}

void Test_event_runtime_owner_cancel_invalidates_handle_and_cleans_once(CuTest *tc)
{
  struct event_runtime_handle handle;
  struct game_event_owner owner;
  struct runtime_payload *payload;
  game_event_type_id_t event_type;
  size_t cancelled;
  int cleanups;
  unsigned long saved_pulse;

  saved_pulse = pulse;
  cleanups = 0;
  begin_runtime_test(tc, 200U);
  event_type = register_runtime_type(tc, "test.native.character_timer", true);
  CuAssertIntEquals(tc, GAME_SCHEDULER_OK, event_runtime_seal_types());
  owner.kind = GAME_EVENT_OWNER_CHARACTER;
  owner.runtime_id = 42U;
  owner.generation = 7U;
  payload = new_runtime_payload(NULL, 'C', 1, &cleanups);
  CuAssertPtrNotNull(tc, payload);
  CuAssertIntEquals(tc, GAME_SCHEDULER_OK,
                    event_runtime_schedule_owned_after(event_type, owner, 50U, payload, &handle));
  CuAssertTrue(tc, event_runtime_handle_is_live(handle));
  cancelled = 0;
  CuAssertIntEquals(tc, GAME_SCHEDULER_OK,
                    event_runtime_cancel_owner(owner, &cancelled));
  CuAssertIntEquals(tc, 1, (int)cancelled);
  CuAssertIntEquals(tc, 1, cleanups);
  CuAssertTrue(tc, !event_runtime_handle_is_live(handle));
  CuAssertIntEquals(tc, GAME_EVENT_CANCEL_NOT_FOUND, event_runtime_cancel(handle));
  event_free_all();
  pulse = saved_pulse;
}
