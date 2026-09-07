#include "CuTest.h"

#include "../../src/conf.h"
#include "../../src/sysdep.h"
#include "../../src/structs.h"
#include "../../src/utils.h"
#include "../../src/comm.h"
#include "../../src/dgscript/dg_event.h"
#include "../../src/ai_service.h"
#include "../../src/domain_event_world.h"
#include "../../src/event_debug.h"
#include "../../src/event_runtime.h"
#include "../../src/perfmon.h"

#include <stdlib.h>
#include <string.h>
#include <pthread.h>

static void begin_backend_test(CuTest *tc, enum event_backend_kind backend,
                               unsigned long start_pulse)
{
  event_free_all();
  pulse = start_pulse;
  CuAssertIntEquals(tc, 1, event_test_select_backend(backend));
  event_init();
  CuAssertIntEquals(tc, backend, event_backend_current());
}

struct ai_ingress_thread_test
{
  struct domain_entity_handle player;
  struct domain_entity_handle npc;
};

static void *enqueue_ai_response_from_worker(void *context)
{
  struct ai_ingress_thread_test *test = context;

  queue_ai_response_for_entities(test->player, test->npc, "test response", "test backend", NULL,
                                 false);
  return NULL;
}

static void verify_ai_event_shutdown_cleanup(CuTest *tc, enum event_backend_kind backend)
{
  struct ai_event_ingress_stats ingress_stats;
  struct ai_ingress_thread_test thread_test;
  struct event_debug_filter filter;
  struct event_debug_snapshot snapshot;
  struct char_data player;
  struct char_data npc;
  struct char_data *saved_character_list;
  pthread_t producer;
  size_t returned_count;
  int queued_events;

  memset(&player, 0, sizeof(player));
  memset(&npc, 0, sizeof(npc));
  saved_character_list = character_list;
  player.next = &npc;
  character_list = &player;

  begin_backend_test(tc, backend, 0U);
  ai_events_ingress_shutdown();
  CuAssertTrue(tc, ai_events_ingress_init());
  ai_event_test_reset_cleanup_count();
  thread_test.player = domain_event_character_handle(&player);
  thread_test.npc = domain_event_character_handle(&npc);
  CuAssertIntEquals(tc, 0,
                    pthread_create(&producer, NULL, enqueue_ai_response_from_worker, &thread_test));
  CuAssertIntEquals(tc, 0, pthread_join(producer, NULL));
  queue_ai_request_retry("test prompt", AI_REQUEST_NPC_DIALOGUE, 0, NULL, NULL);
  CuAssertIntEquals(tc, 0, event_queue_depth());
  memset(&ingress_stats, 0, sizeof(ingress_stats));
  ai_events_get_ingress_stats(&ingress_stats);
  CuAssertIntEquals(tc, 2, (int)ingress_stats.depth);
  ai_events_process_ingress();
  queued_events = event_queue_depth();
  memset(&filter, 0, sizeof(filter));
  filter.type_equals = "ai.response.delivery";
  CuAssertIntEquals(tc, 1, (int)event_debug_inspect(&filter, &snapshot, 1U, &returned_count));
  CuAssertIntEquals(tc, GAME_EVENT_OWNER_CHARACTER, snapshot.owner.kind);
  CuAssertTrue(tc, snapshot.owner.runtime_id == (uint64_t)(uintptr_t)&player);
  filter.type_equals = "ai.request.retry";
  CuAssertIntEquals(tc, 1, (int)event_debug_inspect(&filter, &snapshot, 1U, &returned_count));
  CuAssertIntEquals(tc, GAME_EVENT_OWNER_SERVICE, snapshot.owner.kind);
  character_list = saved_character_list;

  event_free_all();
  ai_events_ingress_shutdown();
  CuAssertIntEquals(tc, 2, queued_events);
  CuAssertIntEquals(tc, 2, ai_event_test_cleanup_count());
  CuAssertIntEquals(tc, 0, event_queue_depth());
}

void Test_ai_event_producers_use_owned_cleanup_on_native_runtime(CuTest *tc)
{
  unsigned long saved_pulse;

  saved_pulse = pulse;
  verify_ai_event_shutdown_cleanup(tc, EVENT_BACKEND_GAME_SCHEDULER);
  pulse = saved_pulse;
}

static void verify_service_ai_retry_dispatch(CuTest *tc, enum event_backend_kind backend)
{
  struct ai_event_ingress_stats ingress_stats;

  begin_backend_test(tc, backend, 0U);
  ai_events_ingress_shutdown();
  CuAssertTrue(tc, ai_events_ingress_init());
  ai_event_test_reset_cleanup_count();
  queue_ai_request_retry("service retry", AI_REQUEST_NPC_DIALOGUE, 0, NULL, NULL);
  ai_events_process_ingress();
  CuAssertIntEquals(tc, 1, event_queue_depth());

  pulse = PASSES_PER_SEC;
  event_test_advance();
  memset(&ingress_stats, 0, sizeof(ingress_stats));
  ai_events_get_ingress_stats(&ingress_stats);
  CuAssertIntEquals(tc, 0, event_queue_depth());
  CuAssertIntEquals(tc, 1, (int)ingress_stats.depth);
  CuAssertIntEquals(tc, 1, ai_event_test_cleanup_count());

  ai_events_ingress_shutdown();
  event_free_all();
}

void Test_service_owned_ai_retries_dispatch_on_native_runtime(CuTest *tc)
{
  unsigned long saved_pulse;

  saved_pulse = pulse;
  verify_service_ai_retry_dispatch(tc, EVENT_BACKEND_GAME_SCHEDULER);
  pulse = saved_pulse;
}

void Test_ai_shutdown_interrupts_worker_backoff(CuTest *tc)
{
  struct timespec started;
  struct timespec finished;
  double elapsed_seconds;

  CuAssertIntEquals(tc, 0, clock_gettime(CLOCK_MONOTONIC, &started));
  CuAssertTrue(tc, ai_service_test_start_waiting_worker());
  shutdown_ai_service();
  CuAssertIntEquals(tc, 0, clock_gettime(CLOCK_MONOTONIC, &finished));
  elapsed_seconds = (double)(finished.tv_sec - started.tv_sec) +
                    (double)(finished.tv_nsec - started.tv_nsec) / 1000000000.0;
  CuAssertIntEquals(tc, 0, (int)ai_service_test_active_workers());
  CuAssertTrue(tc, elapsed_seconds < 2.0);
  ai_service_test_reset_worker_state();
}

static struct game_event_result diagnostic_handler(const struct game_event_context *context)
{
  (void)context;
  return game_event_result_complete();
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
  struct event_runtime_handle alpha;
  struct event_runtime_handle beta;
  struct game_event_type_config config = {0};
  game_event_type_id_t alpha_type;
  game_event_type_id_t beta_type;
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
  config.name = "debug-alpha-type";
  config.handler = diagnostic_handler;
  config.lateness_policy = GAME_EVENT_LATENESS_RUN_ONCE;
  CuAssertIntEquals(tc, GAME_SCHEDULER_OK, event_runtime_register_type(&config, &alpha_type));
  config.name = "debug-beta-type";
  CuAssertIntEquals(tc, GAME_SCHEDULER_OK, event_runtime_register_type(&config, &beta_type));
  CuAssertIntEquals(tc, GAME_SCHEDULER_OK,
                    event_runtime_schedule_owned_after(alpha_type, owner, 20U, NULL, &alpha));
  CuAssertIntEquals(tc, GAME_SCHEDULER_OK,
                    event_runtime_schedule_after(beta_type, 5U, NULL, &beta));

  event_debug_get_stats(&stats);
  CuAssertIntEquals(tc, backend, stats.backend);
  CuAssertIntEquals(tc, 2, (int)stats.live_events);
  CuAssertIntEquals(tc, 2, (int)stats.high_water_events);
  CuAssertIntEquals(tc, 0, (int)stats.registry_mismatches);
  CuAssertTrue(tc, stats.stale_owner_outcomes == 0U);
  CuAssertIntEquals(tc, 1, (int)stats.owner_event_counts[GAME_EVENT_OWNER_NONE]);
  CuAssertIntEquals(tc, 1, (int)stats.owner_event_counts[GAME_EVENT_OWNER_SERVICE]);
  CuAssertIntEquals(tc, backend == EVENT_BACKEND_GAME_SCHEDULER, stats.scheduler_stats_available);

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
  filter.event_id = alpha.id;
  CuAssertIntEquals(tc, 1, (int)event_debug_inspect(&filter, snapshots, 2U, &returned));
  CuAssertTrue(tc, snapshots[0].event_id == alpha.id);

  memset(&filter, 0, sizeof(filter));
  for (width_index = 0; width_index < sizeof(widths) / sizeof(widths[0]); width_index++)
  {
    event_debug_render_help(output, sizeof(output), widths[width_index]);
    assert_debug_output_width(tc, output, (size_t)widths[width_index]);
    CuAssertPtrNotNull(tc, strstr(output, "eventdebug range"));
    CuAssertPtrNotNull(tc, strstr(output, "eventdebug scripts <kind>"));
    event_debug_render_summary(output, sizeof(output), widths[width_index]);
    assert_debug_output_width(tc, output, (size_t)widths[width_index]);
    CuAssertPtrNotNull(tc, strstr(output, "Live events by owner"));
    event_debug_render_queue(output, sizeof(output), widths[width_index], &filter, 2U);
    assert_debug_output_width(tc, output, (size_t)widths[width_index]);
    CuAssertPtrNotNull(tc, strstr(output, "Payloads: redacted"));
    CuAssertPtrNotNull(tc, strstr(output, "debug-alpha-type"));
    CuAssertPtrNotNull(tc, strstr(output, "debug-beta-type"));
    event_debug_render_profiles(output, sizeof(output), widths[width_index], 10U, 0U);
    assert_debug_output_width(tc, output, (size_t)widths[width_index]);
    CuAssertPtrNotNull(tc, strstr(output, "  live: 1"));
    CuAssertPtrNotNull(tc, strstr(output, "lateness ticks"));
    CuAssertPtrNotNull(tc, strstr(output, "samples/seen/late"));
    event_debug_render_profiles(output, sizeof(output), widths[width_index], 1U, 1U);
    assert_debug_output_width(tc, output, (size_t)widths[width_index]);
    CuAssertPtrNotNull(tc, strstr(output, "Showing: 1"));
    CuAssertPtrNotNull(tc, strstr(output, "Offset: 1"));
    event_debug_render_domain(output, sizeof(output), widths[width_index], NULL);
    assert_debug_output_width(tc, output, (size_t)widths[width_index]);
  }

  CuAssertIntEquals(tc, GAME_SCHEDULER_OK, event_runtime_cancel(alpha));
  CuAssertIntEquals(tc, GAME_SCHEDULER_OK, event_runtime_cancel(beta));
  event_debug_get_stats(&stats);
  CuAssertIntEquals(tc, 0, (int)stats.live_events);
  CuAssertIntEquals(tc, 2, (int)stats.high_water_events);
  CuAssertIntEquals(tc, 0, (int)stats.registry_mismatches);
  event_note_stale_owner_outcome();
  event_debug_get_stats(&stats);
  CuAssertTrue(tc, stats.stale_owner_outcomes == 1U);
  event_free_all();
}

void Test_event_debug_registry_is_native_filterable_and_width_bounded(CuTest *tc)
{
  unsigned long saved_pulse;

  saved_pulse = pulse;
  CuAssertIntEquals(tc, 80, event_debug_effective_width(0));
  CuAssertIntEquals(tc, 80, event_debug_effective_width(39));
  CuAssertIntEquals(tc, 40, event_debug_effective_width(40));
  CuAssertIntEquals(tc, 80, event_debug_effective_width(80));
  CuAssertIntEquals(tc, 120, event_debug_effective_width(121));
  verify_event_debug_registry(tc, EVENT_BACKEND_GAME_SCHEDULER);
  pulse = saved_pulse;
}

static void verify_runtime_service_registry(CuTest *tc, enum event_backend_kind backend)
{
  struct event_debug_filter filter;
  struct event_debug_snapshot snapshot;
  struct game_scheduler_stats scheduler_stats;
  struct runtime_service_stats stats;
  size_t returned_count;
  unsigned long start_pulse;

  begin_backend_test(tc, backend, 0U);
  start_pulse = pulse;
  runtime_services_set_scheduled_for_test(true);
  CuAssertTrue(tc, runtime_services_init_for_test());
  memset(&stats, 0, sizeof(stats));
  runtime_services_get_stats(&stats);
  CuAssertTrue(tc, stats.initialized);
  CuAssertTrue(tc, stats.scheduled);
  CuAssertTrue(tc, stats.configured_services >= 10U);
  CuAssertIntEquals(tc, (int)stats.configured_services, (int)stats.live_services);
  memset(&scheduler_stats, 0, sizeof(scheduler_stats));
  event_runtime_get_stats(&scheduler_stats);
  CuAssertIntEquals(tc, (int)stats.live_services, (int)scheduler_stats.event_count);
  CuAssertIntEquals(tc, (int)stats.live_services, event_queue_depth());
  memset(&filter, 0, sizeof(filter));
  filter.type_equals = "service.one_second";
  CuAssertIntEquals(tc, 1, (int)event_debug_inspect(&filter, &snapshot, 1U, &returned_count));
  CuAssertIntEquals(tc, 1, (int)returned_count);
  CuAssertIntEquals(tc, GAME_EVENT_OWNER_SERVICE, snapshot.owner.kind);

  CuAssertTrue(tc, runtime_services_start_empty_persistence_for_test());
  CuAssertTrue(tc, runtime_services_persistence_pending_for_test());
  event_runtime_get_stats(&scheduler_stats);
  CuAssertIntEquals(tc, (int)stats.live_services + 1, (int)scheduler_stats.event_count);
  filter.type_equals = "service.persistence_batch";
  CuAssertIntEquals(tc, 1, (int)event_debug_inspect(&filter, &snapshot, 1U, &returned_count));
  pulse = start_pulse + 1U;
  event_test_advance();
  CuAssertTrue(tc, !runtime_services_persistence_pending_for_test());
  CuAssertTrue(tc, runtime_services_start_empty_persistence_for_test());
  CuAssertTrue(tc, runtime_services_persistence_pending_for_test());

  runtime_services_shutdown();
  memset(&stats, 0, sizeof(stats));
  runtime_services_get_stats(&stats);
  CuAssertTrue(tc, !stats.initialized);
  CuAssertTrue(tc, !stats.scheduled);
  CuAssertIntEquals(tc, 0, (int)stats.live_services);
  CuAssertIntEquals(tc, 0, event_queue_depth());
  event_runtime_get_stats(&scheduler_stats);
  CuAssertIntEquals(tc, 0, (int)scheduler_stats.event_count);
  runtime_services_reset_selection_for_test();
  event_free_all();
}

void Test_runtime_services_are_native_owned_events(CuTest *tc)
{
  unsigned long saved_pulse = pulse;

  verify_runtime_service_registry(tc, EVENT_BACKEND_GAME_SCHEDULER);
  pulse = saved_pulse;
}

void Test_wait_state_consumes_monotonic_elapsed_ticks(CuTest *tc)
{
  struct char_data ch;
  uint64_t deadline;

  memset(&ch, 0, sizeof(ch));
  GET_WAIT_STATE(&ch) = 5;
  comm_wait_state_advance_for_test(&ch, 100U);
  CuAssertIntEquals(tc, 5, GET_WAIT_STATE(&ch));
  comm_wait_state_advance_for_test(&ch, 102U);
  CuAssertIntEquals(tc, 3, GET_WAIT_STATE(&ch));
  comm_wait_state_advance_for_test(&ch, 102U);
  CuAssertIntEquals(tc, 3, GET_WAIT_STATE(&ch));
  comm_wait_state_advance_for_test(&ch, 200U);
  CuAssertIntEquals(tc, 0, GET_WAIT_STATE(&ch));

  GET_WAIT_STATE(&ch) = 4;
  comm_wait_state_advance_for_test(&ch, 199U);
  CuAssertIntEquals(tc, 4, GET_WAIT_STATE(&ch));
  comm_wait_state_advance_for_test(&ch, 201U);
  CuAssertIntEquals(tc, 2, GET_WAIT_STATE(&ch));

  deadline = comm_wait_state_deadline_usec_for_test(&ch, 1000U);
  CuAssertTrue(tc, deadline == 1000U + 203U * (uint64_t)OPT_USEC);
  GET_WAIT_STATE(&ch) = 0;
  CuAssertTrue(tc, comm_wait_state_deadline_usec_for_test(&ch, 1000U) == UINT64_MAX);
  GET_WAIT_STATE(&ch) = 2;
  ch.wait_last_tick = UINT64_MAX;
  CuAssertTrue(tc, comm_wait_state_deadline_usec_for_test(&ch, 1000U) == UINT64_MAX);
}
