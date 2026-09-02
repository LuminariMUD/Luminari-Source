#include "CuTest.h"

#include "../../src/conf.h"
#include "../../src/sysdep.h"
#include "../../src/structs.h"
#include "../../src/game_scheduler.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef GAME_SCHEDULER_STANDALONE_TEST
FILE *logfile;
#endif

enum test_event_behavior
{
  TEST_EVENT_COMPLETE = 0,
  TEST_EVENT_RESCHEDULE,
  TEST_EVENT_SELF_CANCEL,
  TEST_EVENT_CANCEL_TARGET,
  TEST_EVENT_SCHEDULE_CHILD,
  TEST_EVENT_FAIL,
  TEST_EVENT_SHUTDOWN,
  TEST_EVENT_CANCEL_OWNER
};

struct test_clock
{
  game_tick_t tick;
  uint64_t usec;
};

struct test_event_payload
{
  enum test_event_behavior behavior;
  int marker;
  int *calls;
  int *cleanups;
  int *order;
  size_t *order_count;
  game_tick_t delay_ticks;
  unsigned int complete_after;
  uint64_t *last_missed;
  struct test_clock *clock;
  uint64_t handler_usec;
  game_event_id_t target_id;
  enum game_event_cancel_result *cancel_result;
  game_event_type_id_t child_type;
  struct test_event_payload *child_payload;
  game_event_id_t *child_id;
  enum game_scheduler_status *schedule_status;
  struct game_event_owner owner;
  size_t *owner_cancel_count;
};

static game_tick_t test_tick_now(void *context)
{
  struct test_clock *clock;

  clock = context;
  return clock->tick;
}

static uint64_t test_usec_now(void *context)
{
  struct test_clock *clock;

  clock = context;
  return clock->usec;
}

static void test_payload_cleanup(void *payload)
{
  struct test_event_payload *test_payload;

  test_payload = payload;
  if (test_payload->cleanups != NULL)
    (*test_payload->cleanups)++;
  free(test_payload);
}

static int null_payload_cleanups;

static void test_null_payload_cleanup(void *payload)
{
  if (payload == NULL)
    null_payload_cleanups++;
}

static struct game_event_result test_null_payload_handler(const struct game_event_context *context)
{
  (void)context;
  return game_event_result_complete();
}

static struct game_event_result test_event_handler(const struct game_event_context *context)
{
  struct test_event_payload *payload;
  enum game_scheduler_status schedule_status;

  payload = context->payload;
  if (payload->calls != NULL)
    (*payload->calls)++;
  if (payload->order != NULL && payload->order_count != NULL)
    payload->order[(*payload->order_count)++] = payload->marker;
  if (payload->last_missed != NULL)
    *payload->last_missed = context->missed_occurrences;
  if (payload->clock != NULL)
    payload->clock->usec += payload->handler_usec;

  switch (payload->behavior)
  {
  case TEST_EVENT_RESCHEDULE:
    if (payload->complete_after > 0 && payload->calls != NULL &&
        (unsigned int)*payload->calls >= payload->complete_after)
      return game_event_result_complete();
    return game_event_result_reschedule_after(payload->delay_ticks);
  case TEST_EVENT_SELF_CANCEL:
    if (payload->cancel_result != NULL)
      *payload->cancel_result = game_scheduler_cancel(context->scheduler, context->event_id);
    return game_event_result_reschedule_after(payload->delay_ticks);
  case TEST_EVENT_CANCEL_TARGET:
    if (payload->cancel_result != NULL)
      *payload->cancel_result = game_scheduler_cancel(context->scheduler, payload->target_id);
    return game_event_result_complete();
  case TEST_EVENT_SCHEDULE_CHILD:
    schedule_status =
        game_scheduler_schedule_at(context->scheduler, payload->child_type, context->now_tick,
                                   payload->child_payload, payload->child_id);
    if (payload->schedule_status != NULL)
      *payload->schedule_status = schedule_status;
    if (schedule_status != GAME_SCHEDULER_OK)
      test_payload_cleanup(payload->child_payload);
    payload->child_payload = NULL;
    return game_event_result_complete();
  case TEST_EVENT_FAIL:
    return game_event_result_failed(42U);
  case TEST_EVENT_SHUTDOWN:
    game_scheduler_shutdown(context->scheduler);
    return game_event_result_complete();
  case TEST_EVENT_CANCEL_OWNER:
    schedule_status = game_scheduler_cancel_owner(context->scheduler, payload->owner,
                                                  payload->owner_cancel_count);
    if (payload->schedule_status != NULL)
      *payload->schedule_status = schedule_status;
    return game_event_result_reschedule_after(payload->delay_ticks);
  case TEST_EVENT_COMPLETE:
  default:
    return game_event_result_complete();
  }
}

static struct game_scheduler *create_test_scheduler(CuTest *tc, struct test_clock *clock,
                                                    size_t max_events, bool include_usec_clock)
{
  struct game_scheduler_config config;
  struct game_scheduler *scheduler;
  enum game_scheduler_status status;

  memset(&config, 0, sizeof(config));
  config.max_events = max_events;
  config.max_event_types = 32U;
  config.tick_now = test_tick_now;
  config.monotonic_usec_now = include_usec_clock ? test_usec_now : NULL;
  config.clock_context = clock;
  scheduler = game_scheduler_create(&config, &status);
  CuAssertIntEquals(tc, GAME_SCHEDULER_OK, status);
  CuAssertPtrNotNull(tc, scheduler);
  return scheduler;
}

static struct game_scheduler *create_owner_test_scheduler(CuTest *tc, struct test_clock *clock,
                                                          size_t max_events,
                                                          size_t max_events_per_owner)
{
  struct game_scheduler_config config;
  struct game_scheduler *scheduler;
  enum game_scheduler_status status;

  memset(&config, 0, sizeof(config));
  config.max_events = max_events;
  config.max_event_types = 32U;
  config.max_events_per_owner = max_events_per_owner;
  config.tick_now = test_tick_now;
  config.monotonic_usec_now = test_usec_now;
  config.clock_context = clock;
  scheduler = game_scheduler_create(&config, &status);
  CuAssertIntEquals(tc, GAME_SCHEDULER_OK, status);
  CuAssertPtrNotNull(tc, scheduler);
  return scheduler;
}

static game_event_type_id_t register_test_type(CuTest *tc, struct game_scheduler *scheduler,
                                               const char *name,
                                               enum game_event_lateness_policy lateness_policy,
                                               uint32_t catch_up_limit, size_t max_events)
{
  struct game_event_type_config config;
  game_event_type_id_t event_type;
  enum game_scheduler_status status;

  memset(&config, 0, sizeof(config));
  config.name = name;
  config.handler = test_event_handler;
  config.cleanup = test_payload_cleanup;
  config.lateness_policy = lateness_policy;
  config.catch_up_limit = catch_up_limit;
  config.max_events = max_events;
  status = game_scheduler_register_type(scheduler, &config, &event_type);
  CuAssertIntEquals(tc, GAME_SCHEDULER_OK, status);
  return event_type;
}

static struct test_event_payload *create_test_payload(CuTest *tc, int *calls, int *cleanups)
{
  struct test_event_payload *payload;

  payload = calloc(1, sizeof(*payload));
  CuAssertPtrNotNull(tc, payload);
  payload->behavior = TEST_EVENT_COMPLETE;
  payload->calls = calls;
  payload->cleanups = cleanups;
  return payload;
}

static struct game_scheduler_dispatch_report
advance_scheduler(CuTest *tc, struct game_scheduler *scheduler,
                  const struct game_scheduler_budget *budget)
{
  struct game_scheduler_dispatch_report report;
  enum game_scheduler_status status;

  memset(&report, 0, sizeof(report));
  status = game_scheduler_advance(scheduler, budget, &report);
  CuAssertIntEquals(tc, GAME_SCHEDULER_OK, status);
  return report;
}

void Test_game_scheduler_validates_types_payloads_and_duplicate_names(CuTest *tc)
{
  struct game_scheduler *scheduler;
  struct game_event_type_config config;
  struct test_event_payload *payload;
  struct test_clock clock;
  game_event_type_id_t event_type;
  game_event_id_t event_id;
  enum game_scheduler_status status;
  int cleanups;

  memset(&clock, 0, sizeof(clock));
  cleanups = 0;
  scheduler = create_test_scheduler(tc, &clock, 8U, true);
  memset(&config, 0, sizeof(config));
  config.name = "no-payload";
  config.handler = test_event_handler;
  config.lateness_policy = GAME_EVENT_LATENESS_RUN_ONCE;
  status = game_scheduler_register_type(scheduler, &config, &event_type);
  CuAssertIntEquals(tc, GAME_SCHEDULER_OK, status);
  status = game_scheduler_register_type(scheduler, &config, &event_type);
  CuAssertIntEquals(tc, GAME_SCHEDULER_INVALID_TYPE, status);

  payload = create_test_payload(tc, NULL, &cleanups);
  status = game_scheduler_schedule_after(scheduler, event_type, 1U, payload, &event_id);
  CuAssertIntEquals(tc, GAME_SCHEDULER_INVALID_PAYLOAD, status);
  CuAssertIntEquals(tc, 0, cleanups);
  free(payload);
  status = game_scheduler_schedule_after(scheduler, event_type + 1U, 1U, NULL, &event_id);
  CuAssertIntEquals(tc, GAME_SCHEDULER_INVALID_TYPE, status);
  CuAssertIntEquals(tc, GAME_SCHEDULER_OK, game_scheduler_destroy(scheduler));
}

void Test_game_scheduler_runs_cleanup_for_null_payloads(CuTest *tc)
{
  struct game_scheduler *scheduler;
  struct game_event_type_config config;
  struct test_clock clock;
  game_event_type_id_t event_type;
  game_event_id_t event_id;
  enum game_scheduler_status status;

  memset(&clock, 0, sizeof(clock));
  scheduler = create_test_scheduler(tc, &clock, 8U, true);
  memset(&config, 0, sizeof(config));
  config.name = "nullable-payload";
  config.handler = test_null_payload_handler;
  config.cleanup = test_null_payload_cleanup;
  config.lateness_policy = GAME_EVENT_LATENESS_RUN_ONCE;
  config.cleanup_on_null_payload = true;
  status = game_scheduler_register_type(scheduler, &config, &event_type);
  CuAssertIntEquals(tc, GAME_SCHEDULER_OK, status);

  null_payload_cleanups = 0;
  status = game_scheduler_schedule_after(scheduler, event_type, 1U, NULL, &event_id);
  CuAssertIntEquals(tc, GAME_SCHEDULER_OK, status);
  clock.tick = 1U;
  advance_scheduler(tc, scheduler, NULL);
  CuAssertIntEquals(tc, 1, null_payload_cleanups);

  status = game_scheduler_schedule_after(scheduler, event_type, 1U, NULL, &event_id);
  CuAssertIntEquals(tc, GAME_SCHEDULER_OK, status);
  CuAssertIntEquals(tc, GAME_EVENT_CANCELLED, game_scheduler_cancel(scheduler, event_id));
  CuAssertIntEquals(tc, 2, null_payload_cleanups);
  CuAssertIntEquals(tc, GAME_SCHEDULER_OK, game_scheduler_destroy(scheduler));
  CuAssertIntEquals(tc, 2, null_payload_cleanups);
}

void Test_game_scheduler_normalizes_deadlines_and_rejects_overflow(CuTest *tc)
{
  struct game_event_snapshot snapshot;
  struct game_scheduler *scheduler;
  struct game_scheduler_dispatch_report report;
  struct test_event_payload *payload;
  struct test_clock clock;
  game_event_type_id_t event_type;
  game_event_id_t event_id;
  game_tick_t remaining;
  enum game_scheduler_status status;
  int cleanups;

  memset(&clock, 0, sizeof(clock));
  clock.tick = 100U;
  cleanups = 0;
  scheduler = create_test_scheduler(tc, &clock, 4U, true);
  event_type = register_test_type(tc, scheduler, "deadlines", GAME_EVENT_LATENESS_RUN_ONCE, 0U, 0U);
  payload = create_test_payload(tc, NULL, &cleanups);
  CuAssertIntEquals(tc, GAME_SCHEDULER_OK,
                    game_scheduler_schedule_at(scheduler, event_type, 0U, payload, &event_id));
  CuAssertIntEquals(tc, GAME_SCHEDULER_OK, game_scheduler_inspect(scheduler, event_id, &snapshot));
  CuAssertTrue(tc, snapshot.deadline_tick == 101U);
  CuAssertIntEquals(tc, GAME_SCHEDULER_OK,
                    game_scheduler_remaining(scheduler, event_id, &remaining));
  CuAssertTrue(tc, remaining == 1U);
  CuAssertIntEquals(tc, GAME_SCHEDULER_OK,
                    game_scheduler_reschedule_after(scheduler, event_id, 0U));
  CuAssertIntEquals(tc, GAME_SCHEDULER_OK, game_scheduler_inspect(scheduler, event_id, &snapshot));
  CuAssertTrue(tc, snapshot.deadline_tick == 101U);
  CuAssertIntEquals(tc, GAME_EVENT_CANCELLED, game_scheduler_cancel(scheduler, event_id));

  payload = create_test_payload(tc, NULL, &cleanups);
  status = game_scheduler_schedule_after(scheduler, event_type, UINT64_MAX, payload, &event_id);
  CuAssertIntEquals(tc, GAME_SCHEDULER_INVALID_DEADLINE, status);
  test_payload_cleanup(payload);
  clock.tick = UINT64_MAX;
  report = advance_scheduler(tc, scheduler, NULL);
  CuAssertTrue(tc, report.current_tick == UINT64_MAX);
  status = game_scheduler_schedule_at(scheduler, event_type, UINT64_MAX, NULL, &event_id);
  CuAssertIntEquals(tc, GAME_SCHEDULER_INVALID_DEADLINE, status);
  status = game_scheduler_schedule_after(scheduler, event_type, 1U, NULL, &event_id);
  CuAssertIntEquals(tc, GAME_SCHEDULER_INVALID_DEADLINE, status);
  CuAssertIntEquals(tc, 2, cleanups);
  CuAssertIntEquals(tc, GAME_SCHEDULER_OK, game_scheduler_destroy(scheduler));
}

void Test_game_scheduler_places_every_wheel_boundary_and_overflow(CuTest *tc)
{
  static const game_tick_t delays[] = {0U,
                                       1U,
                                       63U,
                                       64U,
                                       65U,
                                       4095U,
                                       4096U,
                                       4097U,
                                       262143U,
                                       262144U,
                                       262145U,
                                       16777215U,
                                       16777216U,
                                       16777217U,
                                       (UINT64_C(1) << 30U) - 1U,
                                       UINT64_C(1) << 30U};
  static const enum game_event_location locations[] = {
      GAME_EVENT_LOCATION_WHEEL,   GAME_EVENT_LOCATION_WHEEL, GAME_EVENT_LOCATION_WHEEL,
      GAME_EVENT_LOCATION_WHEEL,   GAME_EVENT_LOCATION_WHEEL, GAME_EVENT_LOCATION_WHEEL,
      GAME_EVENT_LOCATION_WHEEL,   GAME_EVENT_LOCATION_WHEEL, GAME_EVENT_LOCATION_WHEEL,
      GAME_EVENT_LOCATION_WHEEL,   GAME_EVENT_LOCATION_WHEEL, GAME_EVENT_LOCATION_WHEEL,
      GAME_EVENT_LOCATION_WHEEL,   GAME_EVENT_LOCATION_WHEEL, GAME_EVENT_LOCATION_WHEEL,
      GAME_EVENT_LOCATION_OVERFLOW};
  static const uint32_t levels[] = {0U, 0U, 0U, 1U, 1U, 1U, 2U, 2U,
                                    2U, 3U, 3U, 3U, 4U, 4U, 4U, UINT32_MAX};
  struct game_event_snapshot snapshot;
  struct game_scheduler_stats stats;
  struct game_scheduler *scheduler;
  struct test_clock clock;
  game_event_type_id_t event_type;
  game_event_id_t event_ids[sizeof(delays) / sizeof(delays[0])];
  size_t index;

  memset(&clock, 0, sizeof(clock));
  scheduler = create_test_scheduler(tc, &clock, 20U, true);
  event_type =
      register_test_type(tc, scheduler, "boundaries", GAME_EVENT_LATENESS_RUN_ONCE, 0U, 0U);

  for (index = 0; index < sizeof(delays) / sizeof(delays[0]); index++)
  {
    CuAssertIntEquals(tc, GAME_SCHEDULER_OK,
                      game_scheduler_schedule_after(scheduler, event_type, delays[index], NULL,
                                                    &event_ids[index]));
    CuAssertIntEquals(tc, GAME_SCHEDULER_OK,
                      game_scheduler_inspect(scheduler, event_ids[index], &snapshot));
    CuAssertIntEquals(tc, locations[index], snapshot.location);
    CuAssertTrue(tc, snapshot.wheel_level == levels[index]);
  }

  game_scheduler_get_stats(scheduler, &stats);
  for (index = 0; index < GAME_SCHEDULER_WHEEL_LEVELS; index++)
    CuAssertIntEquals(tc, 3, (int)stats.wheel_level_counts[index]);
  CuAssertIntEquals(tc, 1, (int)stats.overflow_count);

  for (index = 0; index < sizeof(delays) / sizeof(delays[0]); index++)
    CuAssertIntEquals(tc, GAME_EVENT_CANCELLED, game_scheduler_cancel(scheduler, event_ids[index]));
  CuAssertIntEquals(tc, GAME_SCHEDULER_OK, game_scheduler_destroy(scheduler));
}

void Test_game_scheduler_defends_event_id_and_sequence_wrap(CuTest *tc)
{
  struct game_scheduler *scheduler;
  struct test_event_payload *accepted;
  struct test_event_payload *rejected;
  struct test_clock clock;
  game_event_type_id_t event_type;
  game_event_id_t event_id;
  enum game_scheduler_status status;
  int cleanups;

  memset(&clock, 0, sizeof(clock));
  cleanups = 0;
  scheduler = create_test_scheduler(tc, &clock, 4U, true);
  event_type = register_test_type(tc, scheduler, "wrap", GAME_EVENT_LATENESS_RUN_ONCE, 0U, 0U);

  CuAssertIntEquals(tc, GAME_SCHEDULER_OK,
                    game_scheduler_test_set_sequences(scheduler, UINT64_MAX, 1U));
  accepted = create_test_payload(tc, NULL, &cleanups);
  CuAssertIntEquals(tc, GAME_SCHEDULER_OK,
                    game_scheduler_schedule_after(scheduler, event_type, 1U, accepted, &event_id));
  CuAssertTrue(tc, event_id == UINT64_MAX);
  rejected = create_test_payload(tc, NULL, &cleanups);
  status = game_scheduler_schedule_after(scheduler, event_type, 1U, rejected, &event_id);
  CuAssertIntEquals(tc, GAME_SCHEDULER_ID_EXHAUSTED, status);
  test_payload_cleanup(rejected);
  CuAssertIntEquals(tc, GAME_EVENT_CANCELLED, game_scheduler_cancel(scheduler, UINT64_MAX));

  CuAssertIntEquals(tc, GAME_SCHEDULER_OK,
                    game_scheduler_test_set_sequences(scheduler, 1U, UINT64_MAX));
  accepted = create_test_payload(tc, NULL, &cleanups);
  CuAssertIntEquals(tc, GAME_SCHEDULER_OK,
                    game_scheduler_schedule_after(scheduler, event_type, 1U, accepted, &event_id));
  rejected = create_test_payload(tc, NULL, &cleanups);
  status = game_scheduler_schedule_after(scheduler, event_type, 1U, rejected, &event_id);
  CuAssertIntEquals(tc, GAME_SCHEDULER_ID_EXHAUSTED, status);
  test_payload_cleanup(rejected);
  status = game_scheduler_reschedule_after(scheduler, event_id, 2U);
  CuAssertIntEquals(tc, GAME_SCHEDULER_ID_EXHAUSTED, status);
  CuAssertIntEquals(tc, GAME_EVENT_CANCELLED, game_scheduler_cancel(scheduler, event_id));
  CuAssertIntEquals(tc, 4, cleanups);
  CuAssertIntEquals(tc, GAME_SCHEDULER_OK, game_scheduler_destroy(scheduler));
}

void Test_game_scheduler_orders_equal_deadlines_by_insertion_sequence(CuTest *tc)
{
  struct game_scheduler *scheduler;
  struct game_scheduler_dispatch_report report;
  struct test_event_payload *payload;
  struct test_clock clock;
  game_event_type_id_t event_type;
  game_event_id_t event_id;
  game_tick_t deadline;
  bool has_deadline;
  int calls;
  int cleanups;
  int order[4];
  size_t order_count;
  int marker;

  memset(&clock, 0, sizeof(clock));
  calls = 0;
  cleanups = 0;
  order_count = 0;
  scheduler = create_test_scheduler(tc, &clock, 8U, true);
  event_type = register_test_type(tc, scheduler, "ordering", GAME_EVENT_LATENESS_RUN_ONCE, 0U, 0U);

  for (marker = 1; marker <= 3; marker++)
  {
    payload = create_test_payload(tc, &calls, &cleanups);
    payload->marker = marker;
    payload->order = order;
    payload->order_count = &order_count;
    CuAssertIntEquals(tc, GAME_SCHEDULER_OK,
                      game_scheduler_schedule_at(scheduler, event_type, 64U, payload, &event_id));
  }
  CuAssertIntEquals(tc, GAME_SCHEDULER_OK,
                    game_scheduler_next_deadline(scheduler, &deadline, &has_deadline));
  CuAssertTrue(tc, has_deadline);
  CuAssertTrue(tc, deadline == 64U);

  clock.tick = 64U;
  report = advance_scheduler(tc, scheduler, NULL);
  CuAssertTrue(tc, report.used_large_advance == false);
  CuAssertIntEquals(tc, 3, (int)report.callbacks);
  CuAssertIntEquals(tc, 3, calls);
  CuAssertIntEquals(tc, 3, cleanups);
  CuAssertIntEquals(tc, 1, order[0]);
  CuAssertIntEquals(tc, 2, order[1]);
  CuAssertIntEquals(tc, 3, order[2]);
  CuAssertIntEquals(tc, GAME_SCHEDULER_OK,
                    game_scheduler_next_deadline(scheduler, &deadline, &has_deadline));
  CuAssertTrue(tc, !has_deadline);
  CuAssertIntEquals(tc, GAME_SCHEDULER_OK, game_scheduler_destroy(scheduler));
}

void Test_game_scheduler_large_advance_promotes_overflow_without_tick_scanning(CuTest *tc)
{
  struct game_event_snapshot snapshot;
  struct game_scheduler *scheduler;
  struct game_scheduler_dispatch_report report;
  struct test_clock clock;
  game_event_type_id_t event_type;
  game_event_id_t event_id;
  game_tick_t horizon;

  memset(&clock, 0, sizeof(clock));
  horizon = UINT64_C(1) << 30U;
  scheduler = create_test_scheduler(tc, &clock, 4U, true);
  event_type =
      register_test_type(tc, scheduler, "overflow-promotion", GAME_EVENT_LATENESS_RUN_ONCE, 0U, 0U);
  CuAssertIntEquals(
      tc, GAME_SCHEDULER_OK,
      game_scheduler_schedule_at(scheduler, event_type, horizon + 10U, NULL, &event_id));

  clock.tick = horizon - 20U;
  report = advance_scheduler(tc, scheduler, NULL);
  CuAssertTrue(tc, report.used_large_advance);
  CuAssertIntEquals(tc, 0, (int)report.callbacks);
  CuAssertIntEquals(tc, GAME_SCHEDULER_OK, game_scheduler_inspect(scheduler, event_id, &snapshot));
  CuAssertIntEquals(tc, GAME_EVENT_LOCATION_WHEEL, snapshot.location);
  CuAssertTrue(tc, snapshot.wheel_level == 0U);
  CuAssertIntEquals(tc, GAME_EVENT_CANCELLED, game_scheduler_cancel(scheduler, event_id));
  CuAssertIntEquals(tc, GAME_SCHEDULER_OK, game_scheduler_destroy(scheduler));
}

void Test_game_scheduler_cancel_is_idempotent_and_cleans_once(CuTest *tc)
{
  struct game_scheduler *scheduler;
  struct test_event_payload *payload;
  struct test_clock clock;
  game_event_type_id_t event_type;
  game_event_id_t event_id;
  int cleanups;

  memset(&clock, 0, sizeof(clock));
  cleanups = 0;
  scheduler = create_test_scheduler(tc, &clock, 4U, true);
  event_type = register_test_type(tc, scheduler, "cancel", GAME_EVENT_LATENESS_RUN_ONCE, 0U, 0U);
  payload = create_test_payload(tc, NULL, &cleanups);
  CuAssertIntEquals(tc, GAME_SCHEDULER_OK,
                    game_scheduler_schedule_after(scheduler, event_type, 10U, payload, &event_id));
  CuAssertIntEquals(tc, GAME_EVENT_CANCELLED, game_scheduler_cancel(scheduler, event_id));
  CuAssertIntEquals(tc, 1, cleanups);
  CuAssertIntEquals(tc, GAME_EVENT_CANCEL_NOT_FOUND, game_scheduler_cancel(scheduler, event_id));
  CuAssertIntEquals(tc, 1, cleanups);
  CuAssertIntEquals(tc, GAME_SCHEDULER_OK, game_scheduler_destroy(scheduler));
}

void Test_game_scheduler_self_cancel_wins_over_callback_reschedule(CuTest *tc)
{
  struct game_scheduler *scheduler;
  struct game_scheduler_dispatch_report report;
  struct test_event_payload *payload;
  struct test_clock clock;
  game_event_type_id_t event_type;
  game_event_id_t event_id;
  enum game_event_cancel_result cancel_result;
  int calls;
  int cleanups;

  memset(&clock, 0, sizeof(clock));
  calls = 0;
  cleanups = 0;
  cancel_result = GAME_EVENT_CANCEL_NOT_FOUND;
  scheduler = create_test_scheduler(tc, &clock, 4U, true);
  event_type =
      register_test_type(tc, scheduler, "self-cancel", GAME_EVENT_LATENESS_RUN_ONCE, 0U, 0U);
  payload = create_test_payload(tc, &calls, &cleanups);
  payload->behavior = TEST_EVENT_SELF_CANCEL;
  payload->delay_ticks = 1U;
  payload->cancel_result = &cancel_result;
  CuAssertIntEquals(tc, GAME_SCHEDULER_OK,
                    game_scheduler_schedule_after(scheduler, event_type, 1U, payload, &event_id));

  clock.tick = 1U;
  report = advance_scheduler(tc, scheduler, NULL);
  CuAssertIntEquals(tc, GAME_EVENT_CANCEL_PENDING, cancel_result);
  CuAssertIntEquals(tc, 1, calls);
  CuAssertIntEquals(tc, 1, cleanups);
  CuAssertIntEquals(tc, 1, (int)report.cancelled);
  CuAssertIntEquals(tc, 0, (int)report.rescheduled);
  CuAssertIntEquals(tc, GAME_SCHEDULER_NOT_FOUND,
                    game_scheduler_inspect(scheduler, event_id, &(struct game_event_snapshot){0}));
  CuAssertIntEquals(tc, GAME_SCHEDULER_OK, game_scheduler_destroy(scheduler));
}

void Test_game_scheduler_callback_can_cancel_another_ready_event(CuTest *tc)
{
  struct game_scheduler *scheduler;
  struct game_scheduler_dispatch_report report;
  struct test_event_payload *canceller;
  struct test_event_payload *target;
  struct test_clock clock;
  game_event_type_id_t event_type;
  game_event_id_t canceller_id;
  game_event_id_t target_id;
  enum game_event_cancel_result cancel_result;
  int calls;
  int cleanups;

  memset(&clock, 0, sizeof(clock));
  calls = 0;
  cleanups = 0;
  cancel_result = GAME_EVENT_CANCEL_NOT_FOUND;
  scheduler = create_test_scheduler(tc, &clock, 4U, true);
  event_type =
      register_test_type(tc, scheduler, "cancel-ready", GAME_EVENT_LATENESS_RUN_ONCE, 0U, 0U);
  canceller = create_test_payload(tc, &calls, &cleanups);
  canceller->behavior = TEST_EVENT_CANCEL_TARGET;
  canceller->cancel_result = &cancel_result;
  target = create_test_payload(tc, &calls, &cleanups);
  CuAssertIntEquals(
      tc, GAME_SCHEDULER_OK,
      game_scheduler_schedule_at(scheduler, event_type, 5U, canceller, &canceller_id));
  CuAssertIntEquals(tc, GAME_SCHEDULER_OK,
                    game_scheduler_schedule_at(scheduler, event_type, 5U, target, &target_id));
  canceller->target_id = target_id;

  clock.tick = 5U;
  report = advance_scheduler(tc, scheduler, NULL);
  CuAssertIntEquals(tc, GAME_EVENT_CANCELLED, cancel_result);
  CuAssertIntEquals(tc, 1, calls);
  CuAssertIntEquals(tc, 2, cleanups);
  CuAssertIntEquals(tc, 1, (int)report.callbacks);
  CuAssertIntEquals(tc, 0, (int)report.ready_remaining);
  CuAssertIntEquals(tc, GAME_SCHEDULER_OK, game_scheduler_destroy(scheduler));
}

void Test_game_scheduler_external_reschedule_preserves_identity(CuTest *tc)
{
  struct game_event_snapshot before;
  struct game_event_snapshot after;
  struct game_scheduler *scheduler;
  struct test_event_payload *payload;
  struct test_clock clock;
  game_event_type_id_t event_type;
  game_event_id_t event_id;
  game_tick_t remaining;
  int calls;
  int cleanups;

  memset(&clock, 0, sizeof(clock));
  calls = 0;
  cleanups = 0;
  scheduler = create_test_scheduler(tc, &clock, 4U, true);
  event_type =
      register_test_type(tc, scheduler, "reschedule", GAME_EVENT_LATENESS_RUN_ONCE, 0U, 0U);
  payload = create_test_payload(tc, &calls, &cleanups);
  CuAssertIntEquals(tc, GAME_SCHEDULER_OK,
                    game_scheduler_schedule_at(scheduler, event_type, 100U, payload, &event_id));
  CuAssertIntEquals(tc, GAME_SCHEDULER_OK, game_scheduler_inspect(scheduler, event_id, &before));
  CuAssertIntEquals(tc, GAME_SCHEDULER_OK,
                    game_scheduler_reschedule_after(scheduler, event_id, 5U));
  CuAssertIntEquals(tc, GAME_SCHEDULER_OK, game_scheduler_inspect(scheduler, event_id, &after));
  CuAssertTrue(tc, after.event_id == before.event_id);
  CuAssertTrue(tc, after.insertion_sequence > before.insertion_sequence);
  CuAssertTrue(tc, after.deadline_tick == 5U);
  CuAssertIntEquals(tc, GAME_SCHEDULER_OK,
                    game_scheduler_remaining(scheduler, event_id, &remaining));
  CuAssertTrue(tc, remaining == 5U);

  clock.tick = 5U;
  advance_scheduler(tc, scheduler, NULL);
  CuAssertIntEquals(tc, 1, calls);
  CuAssertIntEquals(tc, 1, cleanups);
  CuAssertIntEquals(tc, GAME_SCHEDULER_OK, game_scheduler_destroy(scheduler));
}

void Test_game_scheduler_same_tick_callback_schedule_waits_for_next_cycle(CuTest *tc)
{
  struct game_event_snapshot snapshot;
  struct game_scheduler *scheduler;
  struct test_event_payload *parent;
  struct test_event_payload *child;
  struct test_clock clock;
  game_event_type_id_t event_type;
  game_event_id_t parent_id;
  game_event_id_t child_id;
  enum game_scheduler_status schedule_status;
  int parent_calls;
  int parent_cleanups;
  int child_calls;
  int child_cleanups;

  memset(&clock, 0, sizeof(clock));
  parent_calls = 0;
  parent_cleanups = 0;
  child_calls = 0;
  child_cleanups = 0;
  child_id = 0;
  schedule_status = GAME_SCHEDULER_INVALID_ARGUMENT;
  scheduler = create_test_scheduler(tc, &clock, 4U, true);
  event_type = register_test_type(tc, scheduler, "same-tick", GAME_EVENT_LATENESS_RUN_ONCE, 0U, 0U);
  child = create_test_payload(tc, &child_calls, &child_cleanups);
  parent = create_test_payload(tc, &parent_calls, &parent_cleanups);
  parent->behavior = TEST_EVENT_SCHEDULE_CHILD;
  parent->child_type = event_type;
  parent->child_payload = child;
  parent->child_id = &child_id;
  parent->schedule_status = &schedule_status;
  CuAssertIntEquals(tc, GAME_SCHEDULER_OK,
                    game_scheduler_schedule_after(scheduler, event_type, 1U, parent, &parent_id));

  clock.tick = 1U;
  advance_scheduler(tc, scheduler, NULL);
  CuAssertIntEquals(tc, GAME_SCHEDULER_OK, schedule_status);
  CuAssertIntEquals(tc, 1, parent_calls);
  CuAssertIntEquals(tc, 0, child_calls);
  CuAssertIntEquals(tc, GAME_SCHEDULER_OK, game_scheduler_inspect(scheduler, child_id, &snapshot));
  CuAssertTrue(tc, snapshot.deadline_tick == 2U);
  advance_scheduler(tc, scheduler, NULL);
  CuAssertIntEquals(tc, 0, child_calls);
  clock.tick = 2U;
  advance_scheduler(tc, scheduler, NULL);
  CuAssertIntEquals(tc, 1, child_calls);
  CuAssertIntEquals(tc, 1, parent_cleanups);
  CuAssertIntEquals(tc, 1, child_cleanups);
  CuAssertIntEquals(tc, GAME_SCHEDULER_OK, game_scheduler_destroy(scheduler));
}

void Test_game_scheduler_callback_count_budget_preserves_ready_order(CuTest *tc)
{
  struct game_scheduler_budget budget;
  struct game_scheduler_dispatch_report report;
  struct game_scheduler_stats stats;
  struct game_scheduler *scheduler;
  struct test_event_payload *payload;
  struct test_clock clock;
  game_event_type_id_t event_type;
  game_event_id_t event_id;
  int calls;
  int cleanups;
  int order[4];
  size_t order_count;
  int marker;

  memset(&clock, 0, sizeof(clock));
  memset(&budget, 0, sizeof(budget));
  calls = 0;
  cleanups = 0;
  order_count = 0;
  budget.max_callbacks = 1U;
  scheduler = create_test_scheduler(tc, &clock, 4U, true);
  event_type =
      register_test_type(tc, scheduler, "count-budget", GAME_EVENT_LATENESS_RUN_ONCE, 0U, 0U);
  for (marker = 1; marker <= 3; marker++)
  {
    payload = create_test_payload(tc, &calls, &cleanups);
    payload->marker = marker;
    payload->order = order;
    payload->order_count = &order_count;
    CuAssertIntEquals(tc, GAME_SCHEDULER_OK,
                      game_scheduler_schedule_at(scheduler, event_type, 5U, payload, &event_id));
  }

  clock.tick = 8U;
  report = advance_scheduler(tc, scheduler, &budget);
  CuAssertIntEquals(tc, 1, (int)report.callbacks);
  CuAssertIntEquals(tc, 2, (int)report.ready_remaining);
  CuAssertTrue(tc, report.callback_budget_exhausted);
  game_scheduler_get_stats(scheduler, &stats);
  CuAssertTrue(tc, stats.oldest_overdue_ticks == 3U);
  report = advance_scheduler(tc, scheduler, &budget);
  CuAssertIntEquals(tc, 1, (int)report.callbacks);
  CuAssertIntEquals(tc, 1, (int)report.ready_remaining);
  report = advance_scheduler(tc, scheduler, &budget);
  CuAssertIntEquals(tc, 1, (int)report.callbacks);
  CuAssertIntEquals(tc, 0, (int)report.ready_remaining);
  CuAssertIntEquals(tc, 1, order[0]);
  CuAssertIntEquals(tc, 2, order[1]);
  CuAssertIntEquals(tc, 3, order[2]);
  CuAssertIntEquals(tc, 3, cleanups);
  CuAssertIntEquals(tc, GAME_SCHEDULER_OK, game_scheduler_destroy(scheduler));
}

void Test_game_scheduler_wall_time_budget_uses_injected_clock(CuTest *tc)
{
  struct game_scheduler_budget budget;
  struct game_scheduler_dispatch_report report;
  struct game_scheduler *scheduler;
  struct test_event_payload *payload;
  struct test_clock clock;
  game_event_type_id_t event_type;
  game_event_id_t event_id;
  int calls;
  int cleanups;
  int index;

  memset(&clock, 0, sizeof(clock));
  memset(&budget, 0, sizeof(budget));
  calls = 0;
  cleanups = 0;
  budget.max_usec = 15U;
  scheduler = create_test_scheduler(tc, &clock, 4U, true);
  event_type =
      register_test_type(tc, scheduler, "time-budget", GAME_EVENT_LATENESS_RUN_ONCE, 0U, 0U);
  for (index = 0; index < 3; index++)
  {
    payload = create_test_payload(tc, &calls, &cleanups);
    payload->clock = &clock;
    payload->handler_usec = 10U;
    CuAssertIntEquals(tc, GAME_SCHEDULER_OK,
                      game_scheduler_schedule_at(scheduler, event_type, 5U, payload, &event_id));
  }

  clock.tick = 5U;
  report = advance_scheduler(tc, scheduler, &budget);
  CuAssertIntEquals(tc, 2, (int)report.callbacks);
  CuAssertIntEquals(tc, 1, (int)report.ready_remaining);
  CuAssertTrue(tc, report.time_budget_exhausted);
  budget.max_usec = 0;
  advance_scheduler(tc, scheduler, &budget);
  CuAssertIntEquals(tc, 3, calls);
  CuAssertIntEquals(tc, 3, cleanups);
  CuAssertIntEquals(tc, GAME_SCHEDULER_OK, game_scheduler_destroy(scheduler));
}

static void run_lateness_setup(CuTest *tc, struct game_scheduler *scheduler,
                               struct test_clock *clock, game_event_type_id_t event_type,
                               struct test_event_payload *payload, game_event_id_t *event_id)
{
  payload->behavior = TEST_EVENT_RESCHEDULE;
  payload->delay_ticks = 10U;
  CuAssertIntEquals(tc, GAME_SCHEDULER_OK,
                    game_scheduler_schedule_at(scheduler, event_type, 10U, payload, event_id));
  clock->tick = 10U;
  advance_scheduler(tc, scheduler, NULL);
}

void Test_game_scheduler_run_once_lateness_does_not_burst(CuTest *tc)
{
  struct game_event_snapshot snapshot;
  struct game_scheduler_dispatch_report report;
  struct game_scheduler *scheduler;
  struct test_event_payload *payload;
  struct test_clock clock;
  game_event_type_id_t event_type;
  game_event_id_t event_id;
  uint64_t missed;
  int calls;
  int cleanups;

  memset(&clock, 0, sizeof(clock));
  calls = 0;
  cleanups = 0;
  missed = UINT64_MAX;
  scheduler = create_test_scheduler(tc, &clock, 4U, true);
  event_type = register_test_type(tc, scheduler, "run-once", GAME_EVENT_LATENESS_RUN_ONCE, 0U, 0U);
  payload = create_test_payload(tc, &calls, &cleanups);
  payload->last_missed = &missed;
  run_lateness_setup(tc, scheduler, &clock, event_type, payload, &event_id);

  clock.tick = 45U;
  report = advance_scheduler(tc, scheduler, NULL);
  CuAssertIntEquals(tc, 1, (int)report.callbacks);
  CuAssertTrue(tc, missed == 0U);
  CuAssertTrue(tc, report.skipped_occurrences == 2U);
  CuAssertIntEquals(tc, GAME_SCHEDULER_OK, game_scheduler_inspect(scheduler, event_id, &snapshot));
  CuAssertTrue(tc, snapshot.deadline_tick == 50U);
  CuAssertIntEquals(tc, 2, calls);
  CuAssertIntEquals(tc, GAME_SCHEDULER_OK, game_scheduler_destroy(scheduler));
  CuAssertIntEquals(tc, 1, cleanups);
}

void Test_game_scheduler_coalesces_missed_recurrences(CuTest *tc)
{
  struct game_event_snapshot snapshot;
  struct game_scheduler_dispatch_report report;
  struct game_scheduler_stats stats;
  struct game_scheduler *scheduler;
  struct test_event_payload *payload;
  struct test_clock clock;
  game_event_type_id_t event_type;
  game_event_id_t event_id;
  uint64_t missed;
  int calls;
  int cleanups;

  memset(&clock, 0, sizeof(clock));
  calls = 0;
  cleanups = 0;
  missed = 0;
  scheduler = create_test_scheduler(tc, &clock, 4U, true);
  event_type = register_test_type(tc, scheduler, "coalesce", GAME_EVENT_LATENESS_COALESCE, 0U, 0U);
  payload = create_test_payload(tc, &calls, &cleanups);
  payload->last_missed = &missed;
  run_lateness_setup(tc, scheduler, &clock, event_type, payload, &event_id);

  clock.tick = 45U;
  report = advance_scheduler(tc, scheduler, NULL);
  CuAssertIntEquals(tc, 1, (int)report.callbacks);
  CuAssertTrue(tc, missed == 2U);
  CuAssertTrue(tc, report.missed_occurrences == 2U);
  CuAssertTrue(tc, report.skipped_occurrences == 0U);
  CuAssertTrue(tc, report.coalesced_occurrences == 2U);
  game_scheduler_get_stats(scheduler, &stats);
  CuAssertTrue(tc, stats.total_rescheduled == 2U);
  CuAssertTrue(tc, stats.total_late_callbacks == 1U);
  CuAssertTrue(tc, stats.total_missed_occurrences == 2U);
  CuAssertTrue(tc, stats.total_skipped_occurrences == 0U);
  CuAssertTrue(tc, stats.total_coalesced_occurrences == 2U);
  CuAssertIntEquals(tc, GAME_SCHEDULER_OK, game_scheduler_inspect(scheduler, event_id, &snapshot));
  CuAssertTrue(tc, snapshot.deadline_tick == 50U);
  CuAssertIntEquals(tc, GAME_SCHEDULER_OK, game_scheduler_destroy(scheduler));
  CuAssertIntEquals(tc, 1, cleanups);
}

void Test_game_scheduler_skip_missed_policy_runs_no_late_callback(CuTest *tc)
{
  struct game_event_snapshot snapshot;
  struct game_scheduler_dispatch_report report;
  struct game_scheduler *scheduler;
  struct test_event_payload *payload;
  struct test_clock clock;
  game_event_type_id_t event_type;
  game_event_id_t event_id;
  int calls;
  int cleanups;

  memset(&clock, 0, sizeof(clock));
  calls = 0;
  cleanups = 0;
  scheduler = create_test_scheduler(tc, &clock, 4U, true);
  event_type =
      register_test_type(tc, scheduler, "skip-missed", GAME_EVENT_LATENESS_SKIP_MISSED, 0U, 0U);
  payload = create_test_payload(tc, &calls, &cleanups);
  run_lateness_setup(tc, scheduler, &clock, event_type, payload, &event_id);

  clock.tick = 45U;
  report = advance_scheduler(tc, scheduler, NULL);
  CuAssertIntEquals(tc, 0, (int)report.callbacks);
  CuAssertTrue(tc, report.skipped_occurrences == 3U);
  CuAssertIntEquals(tc, 1, calls);
  CuAssertIntEquals(tc, GAME_SCHEDULER_OK, game_scheduler_inspect(scheduler, event_id, &snapshot));
  CuAssertTrue(tc, snapshot.deadline_tick == 50U);
  CuAssertIntEquals(tc, GAME_SCHEDULER_OK, game_scheduler_destroy(scheduler));
  CuAssertIntEquals(tc, 1, cleanups);
}

void Test_game_scheduler_bounded_catch_up_enforces_limit(CuTest *tc)
{
  struct game_event_snapshot snapshot;
  struct game_scheduler_dispatch_report report;
  struct game_scheduler *scheduler;
  struct test_event_payload *payload;
  struct test_clock clock;
  game_event_type_id_t event_type;
  game_event_id_t event_id;
  int calls;
  int cleanups;

  memset(&clock, 0, sizeof(clock));
  calls = 0;
  cleanups = 0;
  scheduler = create_test_scheduler(tc, &clock, 4U, true);
  event_type =
      register_test_type(tc, scheduler, "catch-up", GAME_EVENT_LATENESS_CATCH_UP_BOUNDED, 3U, 0U);
  payload = create_test_payload(tc, &calls, &cleanups);
  run_lateness_setup(tc, scheduler, &clock, event_type, payload, &event_id);

  clock.tick = 55U;
  report = advance_scheduler(tc, scheduler, NULL);
  CuAssertIntEquals(tc, 3, (int)report.callbacks);
  CuAssertTrue(tc, report.skipped_occurrences == 1U);
  CuAssertIntEquals(tc, 4, calls);
  CuAssertIntEquals(tc, GAME_SCHEDULER_OK, game_scheduler_inspect(scheduler, event_id, &snapshot));
  CuAssertTrue(tc, snapshot.deadline_tick == 60U);
  CuAssertIntEquals(tc, GAME_SCHEDULER_OK, game_scheduler_destroy(scheduler));
  CuAssertIntEquals(tc, 1, cleanups);
}

void Test_game_scheduler_enforces_global_and_per_type_capacity(CuTest *tc)
{
  struct game_scheduler_stats stats;
  struct game_scheduler *scheduler;
  struct test_event_payload *first;
  struct test_event_payload *second;
  struct test_event_payload *third;
  struct test_event_payload *fourth;
  struct test_clock clock;
  game_event_type_id_t limited_type;
  game_event_type_id_t other_type;
  game_event_id_t event_id;
  enum game_scheduler_status status;
  int cleanups;

  memset(&clock, 0, sizeof(clock));
  cleanups = 0;
  scheduler = create_test_scheduler(tc, &clock, 2U, true);
  limited_type = register_test_type(tc, scheduler, "limited", GAME_EVENT_LATENESS_RUN_ONCE, 0U, 1U);
  other_type = register_test_type(tc, scheduler, "other", GAME_EVENT_LATENESS_RUN_ONCE, 0U, 0U);
  first = create_test_payload(tc, NULL, &cleanups);
  second = create_test_payload(tc, NULL, &cleanups);
  third = create_test_payload(tc, NULL, &cleanups);
  fourth = create_test_payload(tc, NULL, &cleanups);

  CuAssertIntEquals(tc, GAME_SCHEDULER_OK,
                    game_scheduler_schedule_after(scheduler, limited_type, 1U, first, &event_id));
  status = game_scheduler_schedule_after(scheduler, limited_type, 1U, second, &event_id);
  CuAssertIntEquals(tc, GAME_SCHEDULER_TYPE_CAPACITY_REACHED, status);
  CuAssertIntEquals(tc, 0, cleanups);
  free(second);
  CuAssertIntEquals(tc, GAME_SCHEDULER_OK,
                    game_scheduler_schedule_after(scheduler, other_type, 1U, third, &event_id));
  status = game_scheduler_schedule_after(scheduler, other_type, 1U, fourth, &event_id);
  CuAssertIntEquals(tc, GAME_SCHEDULER_CAPACITY_REACHED, status);
  game_scheduler_get_stats(scheduler, &stats);
  CuAssertTrue(tc, stats.total_type_capacity_rejections == 1U);
  CuAssertTrue(tc, stats.total_capacity_rejections == 1U);
  CuAssertIntEquals(tc, 0, cleanups);
  free(fourth);
  CuAssertIntEquals(tc, GAME_SCHEDULER_OK, game_scheduler_destroy(scheduler));
  CuAssertIntEquals(tc, 2, cleanups);
}

void Test_game_scheduler_owner_contract_validates_limits_and_inspection(CuTest *tc)
{
  struct game_event_type_config type_config;
  struct game_event_snapshot snapshots[2];
  struct game_scheduler_stats stats;
  struct game_scheduler *scheduler;
  struct test_event_payload *payload;
  struct test_clock clock;
  struct game_event_owner owner;
  struct game_event_owner malformed;
  game_event_type_id_t limited_type;
  game_event_type_id_t other_type;
  game_event_id_t event_id;
  enum game_scheduler_status status;
  size_t event_count;
  size_t cancelled_count;
  int cleanups;

  memset(&clock, 0, sizeof(clock));
  cleanups = 0;
  scheduler = create_owner_test_scheduler(tc, &clock, 8U, 2U);
  memset(&type_config, 0, sizeof(type_config));
  type_config.name = "owner-limited";
  type_config.handler = test_event_handler;
  type_config.cleanup = test_payload_cleanup;
  type_config.lateness_policy = GAME_EVENT_LATENESS_RUN_ONCE;
  type_config.max_events_per_owner = 1U;
  type_config.requires_owner = true;
  CuAssertIntEquals(tc, GAME_SCHEDULER_OK,
                    game_scheduler_register_type(scheduler, &type_config, &limited_type));
  other_type =
      register_test_type(tc, scheduler, "owner-other", GAME_EVENT_LATENESS_RUN_ONCE, 0U, 0U);

  owner.kind = GAME_EVENT_OWNER_CHARACTER;
  owner.runtime_id = 42U;
  owner.generation = 7U;
  malformed = owner;
  malformed.generation = 0;

  payload = create_test_payload(tc, NULL, &cleanups);
  status = game_scheduler_schedule_after(scheduler, limited_type, 10U, payload, &event_id);
  CuAssertIntEquals(tc, GAME_SCHEDULER_INVALID_OWNER, status);
  free(payload);
  payload = create_test_payload(tc, NULL, &cleanups);
  status = game_scheduler_schedule_owned_after(scheduler, limited_type, malformed, 10U, payload,
                                               &event_id);
  CuAssertIntEquals(tc, GAME_SCHEDULER_INVALID_OWNER, status);
  free(payload);

  payload = create_test_payload(tc, NULL, &cleanups);
  CuAssertIntEquals(
      tc, GAME_SCHEDULER_OK,
      game_scheduler_schedule_owned_after(scheduler, limited_type, owner, 10U, payload, &event_id));
  payload = create_test_payload(tc, NULL, &cleanups);
  status =
      game_scheduler_schedule_owned_after(scheduler, limited_type, owner, 10U, payload, &event_id);
  CuAssertIntEquals(tc, GAME_SCHEDULER_OWNER_TYPE_CAPACITY_REACHED, status);
  free(payload);
  payload = create_test_payload(tc, NULL, &cleanups);
  CuAssertIntEquals(
      tc, GAME_SCHEDULER_OK,
      game_scheduler_schedule_owned_after(scheduler, other_type, owner, 20U, payload, &event_id));
  payload = create_test_payload(tc, NULL, &cleanups);
  status =
      game_scheduler_schedule_owned_after(scheduler, other_type, owner, 30U, payload, &event_id);
  CuAssertIntEquals(tc, GAME_SCHEDULER_OWNER_CAPACITY_REACHED, status);
  free(payload);

  memset(snapshots, 0, sizeof(snapshots));
  CuAssertIntEquals(tc, GAME_SCHEDULER_OK,
                    game_scheduler_inspect_owner(scheduler, owner, snapshots, 2U, &event_count));
  CuAssertIntEquals(tc, 2, (int)event_count);
  CuAssertTrue(tc, game_event_owner_equal(owner, snapshots[0].owner));
  CuAssertTrue(tc, game_event_owner_equal(owner, snapshots[1].owner));
  game_scheduler_get_stats(scheduler, &stats);
  CuAssertIntEquals(tc, 1, (int)stats.owner_count);
  CuAssertIntEquals(tc, 1, (int)stats.owner_counts[GAME_EVENT_OWNER_CHARACTER]);
  CuAssertIntEquals(tc, 2, (int)stats.total_invalid_owner_rejections);
  CuAssertIntEquals(tc, 1, (int)stats.total_owner_capacity_rejections);
  CuAssertIntEquals(tc, 1, (int)stats.total_owner_type_capacity_rejections);

  CuAssertIntEquals(tc, GAME_SCHEDULER_OK,
                    game_scheduler_cancel_owner(scheduler, owner, &cancelled_count));
  CuAssertIntEquals(tc, 2, (int)cancelled_count);
  CuAssertIntEquals(tc, 2, cleanups);
  game_scheduler_get_stats(scheduler, &stats);
  CuAssertIntEquals(tc, 0, (int)stats.owner_count);
  CuAssertIntEquals(tc, 0, (int)stats.owner_counts[GAME_EVENT_OWNER_CHARACTER]);
  CuAssertIntEquals(tc, GAME_SCHEDULER_OK, game_scheduler_destroy(scheduler));
}

void Test_game_scheduler_cancel_owner_is_dispatch_safe_and_generation_scoped(CuTest *tc)
{
  struct game_scheduler_dispatch_report report;
  struct game_scheduler *scheduler;
  struct test_event_payload *canceller;
  struct test_event_payload *target;
  struct test_event_payload *survivor;
  struct test_clock clock;
  struct game_event_owner first_generation;
  struct game_event_owner second_generation;
  game_event_type_id_t event_type;
  game_event_id_t event_id;
  enum game_scheduler_status cancel_status;
  size_t cancelled_count;
  int calls;
  int cleanups;

  memset(&clock, 0, sizeof(clock));
  calls = 0;
  cleanups = 0;
  cancelled_count = 0;
  cancel_status = GAME_SCHEDULER_INVALID_ARGUMENT;
  scheduler = create_owner_test_scheduler(tc, &clock, 8U, 0U);
  event_type =
      register_test_type(tc, scheduler, "owner-dispatch", GAME_EVENT_LATENESS_RUN_ONCE, 0U, 0U);
  first_generation.kind = GAME_EVENT_OWNER_CHARACTER;
  first_generation.runtime_id = 99U;
  first_generation.generation = 1U;
  second_generation = first_generation;
  second_generation.generation = 2U;

  canceller = create_test_payload(tc, &calls, &cleanups);
  canceller->behavior = TEST_EVENT_CANCEL_OWNER;
  canceller->delay_ticks = 10U;
  canceller->owner = first_generation;
  canceller->owner_cancel_count = &cancelled_count;
  canceller->schedule_status = &cancel_status;
  target = create_test_payload(tc, &calls, &cleanups);
  survivor = create_test_payload(tc, &calls, &cleanups);
  CuAssertIntEquals(tc, GAME_SCHEDULER_OK,
                    game_scheduler_schedule_owned_at(scheduler, event_type, first_generation, 5U,
                                                     canceller, &event_id));
  CuAssertIntEquals(tc, GAME_SCHEDULER_OK,
                    game_scheduler_schedule_owned_at(scheduler, event_type, first_generation, 5U,
                                                     target, &event_id));
  CuAssertIntEquals(tc, GAME_SCHEDULER_OK,
                    game_scheduler_schedule_owned_at(scheduler, event_type, second_generation, 5U,
                                                     survivor, &event_id));

  clock.tick = 5U;
  report = advance_scheduler(tc, scheduler, NULL);
  CuAssertIntEquals(tc, GAME_SCHEDULER_OK, cancel_status);
  CuAssertIntEquals(tc, 2, (int)cancelled_count);
  CuAssertIntEquals(tc, 2, calls);
  CuAssertIntEquals(tc, 3, cleanups);
  CuAssertIntEquals(tc, 2, (int)report.callbacks);
  CuAssertIntEquals(tc, 1, (int)report.cancelled);
  CuAssertIntEquals(tc, 0, (int)report.events_remaining);
  CuAssertIntEquals(tc, GAME_SCHEDULER_OK, game_scheduler_destroy(scheduler));
}

void Test_game_scheduler_handles_capacity_scale_mixed_deadlines(CuTest *tc)
{
  static const game_tick_t deadlines[] = {1U, 64U, 4096U, 262144U, 16777216U, UINT64_C(1) << 30U};
  struct game_scheduler_dispatch_report report;
  struct game_scheduler_stats stats;
  struct game_scheduler *scheduler;
  struct test_event_payload *payload;
  struct test_clock clock;
  game_event_type_id_t event_type;
  game_event_id_t event_id;
  size_t index;
  int calls;
  int cleanups;

  memset(&clock, 0, sizeof(clock));
  calls = 0;
  cleanups = 0;
  scheduler = create_test_scheduler(tc, &clock, GAME_SCHEDULER_DEFAULT_MAX_EVENTS, true);
  event_type =
      register_test_type(tc, scheduler, "capacity-scale", GAME_EVENT_LATENESS_RUN_ONCE, 0U, 0U);
  for (index = 0; index < GAME_SCHEDULER_DEFAULT_MAX_EVENTS; index++)
  {
    payload = create_test_payload(tc, &calls, &cleanups);
    CuAssertIntEquals(
        tc, GAME_SCHEDULER_OK,
        game_scheduler_schedule_at(scheduler, event_type,
                                   deadlines[index % (sizeof(deadlines) / sizeof(deadlines[0]))],
                                   payload, &event_id));
  }
  game_scheduler_get_stats(scheduler, &stats);
  CuAssertIntEquals(tc, GAME_SCHEDULER_DEFAULT_MAX_EVENTS, (int)stats.event_count);
  CuAssertIntEquals(tc, GAME_SCHEDULER_DEFAULT_MAX_EVENTS, (int)stats.total_scheduled);

  clock.tick = UINT64_C(1) << 30U;
  report = advance_scheduler(tc, scheduler, NULL);
  CuAssertTrue(tc, report.used_large_advance);
  CuAssertIntEquals(tc, GAME_SCHEDULER_DEFAULT_MAX_EVENTS, (int)report.callbacks);
  CuAssertIntEquals(tc, 0, (int)report.events_remaining);
  CuAssertIntEquals(tc, GAME_SCHEDULER_DEFAULT_MAX_EVENTS, calls);
  CuAssertIntEquals(tc, GAME_SCHEDULER_DEFAULT_MAX_EVENTS, cleanups);
  CuAssertIntEquals(tc, GAME_SCHEDULER_OK, game_scheduler_destroy(scheduler));
}

void Test_game_scheduler_shutdown_during_dispatch_cleans_every_event(CuTest *tc)
{
  struct game_scheduler *scheduler;
  struct game_scheduler_dispatch_report report;
  struct test_event_payload *payload;
  struct test_clock clock;
  game_event_type_id_t event_type;
  game_event_id_t event_id;
  enum game_scheduler_status status;
  int calls;
  int cleanups;
  int index;

  memset(&clock, 0, sizeof(clock));
  calls = 0;
  cleanups = 0;
  scheduler = create_test_scheduler(tc, &clock, 4U, true);
  event_type = register_test_type(tc, scheduler, "shutdown", GAME_EVENT_LATENESS_RUN_ONCE, 0U, 0U);
  for (index = 0; index < 3; index++)
  {
    payload = create_test_payload(tc, &calls, &cleanups);
    if (index == 0)
      payload->behavior = TEST_EVENT_SHUTDOWN;
    CuAssertIntEquals(tc, GAME_SCHEDULER_OK,
                      game_scheduler_schedule_at(scheduler, event_type, index < 2 ? 5U : 100U,
                                                 payload, &event_id));
  }

  clock.tick = 5U;
  report = advance_scheduler(tc, scheduler, NULL);
  CuAssertIntEquals(tc, 1, calls);
  CuAssertIntEquals(tc, 3, cleanups);
  CuAssertIntEquals(tc, 1, (int)report.cancelled);
  CuAssertIntEquals(tc, 0, (int)report.events_remaining);
  status = game_scheduler_advance(scheduler, NULL, &report);
  CuAssertIntEquals(tc, GAME_SCHEDULER_SHUTTING_DOWN, status);
  CuAssertIntEquals(tc, GAME_SCHEDULER_OK, game_scheduler_destroy(scheduler));
}

void Test_game_scheduler_reports_handler_failure_and_clock_errors(CuTest *tc)
{
  struct game_scheduler_budget budget;
  struct game_scheduler_dispatch_report report;
  struct game_scheduler *scheduler;
  struct test_event_payload *payload;
  struct test_clock clock;
  game_event_type_id_t event_type;
  game_event_id_t event_id;
  enum game_scheduler_status status;
  int calls;
  int cleanups;

  memset(&clock, 0, sizeof(clock));
  calls = 0;
  cleanups = 0;
  clock.tick = 10U;
  scheduler = create_test_scheduler(tc, &clock, 4U, false);
  event_type = register_test_type(tc, scheduler, "failure", GAME_EVENT_LATENESS_RUN_ONCE, 0U, 0U);
  payload = create_test_payload(tc, &calls, &cleanups);
  payload->behavior = TEST_EVENT_FAIL;
  CuAssertIntEquals(tc, GAME_SCHEDULER_OK,
                    game_scheduler_schedule_after(scheduler, event_type, 1U, payload, &event_id));

  memset(&budget, 0, sizeof(budget));
  budget.max_usec = 1U;
  status = game_scheduler_advance(scheduler, &budget, &report);
  CuAssertIntEquals(tc, GAME_SCHEDULER_INVALID_ARGUMENT, status);
  clock.tick = 9U;
  status = game_scheduler_advance(scheduler, NULL, &report);
  CuAssertIntEquals(tc, GAME_SCHEDULER_CLOCK_REVERSED, status);
  clock.tick = 11U;
  report = advance_scheduler(tc, scheduler, NULL);
  CuAssertIntEquals(tc, 1, (int)report.failed);
  CuAssertIntEquals(tc, 1, calls);
  CuAssertIntEquals(tc, 1, cleanups);
  CuAssertIntEquals(tc, GAME_SCHEDULER_OK, game_scheduler_destroy(scheduler));
}

void Test_game_scheduler_cleans_payload_when_callback_reschedule_overflows(CuTest *tc)
{
  struct game_scheduler_dispatch_report report;
  struct game_scheduler *scheduler;
  struct test_event_payload *payload;
  struct test_clock clock;
  game_event_type_id_t event_type;
  game_event_id_t event_id;
  int calls;
  int cleanups;

  memset(&clock, 0, sizeof(clock));
  clock.tick = UINT64_MAX - 2U;
  calls = 0;
  cleanups = 0;
  scheduler = create_test_scheduler(tc, &clock, 2U, true);
  event_type = register_test_type(tc, scheduler, "reschedule-overflow",
                                  GAME_EVENT_LATENESS_RUN_ONCE, 0U, 0U);
  payload = create_test_payload(tc, &calls, &cleanups);
  payload->behavior = TEST_EVENT_RESCHEDULE;
  payload->delay_ticks = 2U;
  CuAssertIntEquals(tc, GAME_SCHEDULER_OK,
                    game_scheduler_schedule_after(scheduler, event_type, 1U, payload, &event_id));

  clock.tick = UINT64_MAX - 1U;
  report = advance_scheduler(tc, scheduler, NULL);
  CuAssertIntEquals(tc, 1, (int)report.callbacks);
  CuAssertIntEquals(tc, 1, (int)report.failed);
  CuAssertIntEquals(tc, 0, (int)report.events_remaining);
  CuAssertIntEquals(tc, 1, calls);
  CuAssertIntEquals(tc, 1, cleanups);
  CuAssertIntEquals(tc, GAME_SCHEDULER_NOT_FOUND,
                    game_scheduler_inspect(scheduler, event_id, &(struct game_event_snapshot){0}));
  CuAssertIntEquals(tc, GAME_SCHEDULER_OK, game_scheduler_destroy(scheduler));
}

void Test_game_scheduler_reports_threshold_cascade_and_large_advance_work(CuTest *tc)
{
  struct game_scheduler_dispatch_report report;
  struct game_scheduler_stats stats;
  struct game_scheduler *scheduler;
  struct test_event_payload *payload;
  struct test_clock clock;
  game_event_type_id_t event_type;
  game_event_id_t event_id;
  uint64_t threshold_cascade_slots;
  uint64_t threshold_cascaded_events;
  int calls;
  int cleanups;

  memset(&clock, 0, sizeof(clock));
  calls = 0;
  cleanups = 0;
  scheduler = create_test_scheduler(tc, &clock, 4U, false);
  event_type =
      register_test_type(tc, scheduler, "structural-work", GAME_EVENT_LATENESS_RUN_ONCE, 0U, 0U);
  payload = create_test_payload(tc, &calls, &cleanups);
  CuAssertIntEquals(tc, GAME_SCHEDULER_OK,
                    game_scheduler_schedule_at(scheduler, event_type, 64U, payload, &event_id));

  clock.tick = GAME_SCHEDULER_LARGE_ADVANCE_TICKS;
  report = advance_scheduler(tc, scheduler, NULL);
  CuAssertTrue(tc, !report.used_large_advance);
  CuAssertTrue(tc, report.ticks_advanced == GAME_SCHEDULER_LARGE_ADVANCE_TICKS);
  CuAssertTrue(tc, report.cascade_slots > 0);
  CuAssertTrue(tc, report.cascaded_events > 0);
  threshold_cascade_slots = report.cascade_slots;
  threshold_cascaded_events = report.cascaded_events;
  CuAssertIntEquals(tc, 1, calls);

  payload = create_test_payload(tc, &calls, &cleanups);
  CuAssertIntEquals(tc, GAME_SCHEDULER_OK,
                    game_scheduler_schedule_after(scheduler, event_type,
                                                  GAME_SCHEDULER_LARGE_ADVANCE_TICKS + 100U,
                                                  payload, &event_id));
  clock.tick += GAME_SCHEDULER_LARGE_ADVANCE_TICKS + 1U;
  report = advance_scheduler(tc, scheduler, NULL);
  CuAssertTrue(tc, report.used_large_advance);
  CuAssertTrue(tc, report.large_advance_events == 1U);
  CuAssertIntEquals(tc, 1, (int)report.events_remaining);

  game_scheduler_get_stats(scheduler, &stats);
  CuAssertTrue(tc, stats.total_ticks_advanced == GAME_SCHEDULER_LARGE_ADVANCE_TICKS * 2U + 1U);
  CuAssertTrue(tc, stats.total_large_advances == 1U);
  CuAssertTrue(tc, stats.total_large_advance_events == 1U);
  CuAssertTrue(tc, stats.largest_cascade > 0U);
  if (getenv("LUMINARI_PHASE4_REPORT") != NULL)
    fprintf(stderr,
            "PHASE4 threshold_ticks=%" PRIu64 " cascade_slots=%" PRIu64 " cascaded_events=%" PRIu64
            " large_jump_ticks=%" PRIu64 " large_reclassified=%" PRIu64 " largest_cascade=%" PRIu64
            "\n",
            GAME_SCHEDULER_LARGE_ADVANCE_TICKS, threshold_cascade_slots, threshold_cascaded_events,
            GAME_SCHEDULER_LARGE_ADVANCE_TICKS + 1U, report.large_advance_events,
            stats.largest_cascade);
  CuAssertIntEquals(tc, GAME_EVENT_CANCELLED, game_scheduler_cancel(scheduler, event_id));
  CuAssertIntEquals(tc, 2, cleanups);
  CuAssertIntEquals(tc, GAME_SCHEDULER_OK, game_scheduler_destroy(scheduler));
}

void Test_game_scheduler_long_soak_churn_and_due_storm_remain_bounded(CuTest *tc)
{
  struct game_scheduler_budget budget;
  struct game_scheduler_dispatch_report report;
  struct game_scheduler_stats stats;
  struct game_scheduler *scheduler;
  struct test_event_payload *payload;
  struct test_clock clock;
  game_event_type_id_t event_type;
  game_event_id_t event_ids[512];
  size_t ready_turns;
  int calls;
  int cleanups;
  int round;
  int index;

  memset(&clock, 0, sizeof(clock));
  memset(&budget, 0, sizeof(budget));
  budget.max_callbacks = 32U;
  calls = 0;
  cleanups = 0;
  scheduler = create_test_scheduler(tc, &clock, 512U, false);
  event_type =
      register_test_type(tc, scheduler, "soak-storm", GAME_EVENT_LATENESS_RUN_ONCE, 0U, 0U);

  for (round = 0; round < 100; round++)
  {
    for (index = 0; index < 64; index++)
    {
      payload = create_test_payload(tc, &calls, &cleanups);
      CuAssertIntEquals(
          tc, GAME_SCHEDULER_OK,
          game_scheduler_schedule_after(scheduler, event_type, 1U, payload, &event_ids[index]));
      if ((index % 2) == 0)
        CuAssertIntEquals(tc, GAME_EVENT_CANCELLED,
                          game_scheduler_cancel(scheduler, event_ids[index]));
    }
    clock.tick++;
    do
    {
      report = advance_scheduler(tc, scheduler, &budget);
      CuAssertTrue(tc, report.callbacks <= budget.max_callbacks);
    } while (report.ready_remaining > 0);
    CuAssertIntEquals(tc, 0, (int)report.events_remaining);
  }

  for (index = 0; index < 512; index++)
  {
    payload = create_test_payload(tc, &calls, &cleanups);
    CuAssertIntEquals(
        tc, GAME_SCHEDULER_OK,
        game_scheduler_schedule_after(scheduler, event_type, 1U, payload, &event_ids[index]));
  }
  clock.tick++;
  ready_turns = 0;
  do
  {
    report = advance_scheduler(tc, scheduler, &budget);
    ready_turns++;
    CuAssertTrue(tc, report.callbacks <= budget.max_callbacks);
  } while (report.ready_remaining > 0);

  CuAssertIntEquals(tc, 16, (int)ready_turns);
  CuAssertIntEquals(tc, 0, (int)report.events_remaining);
  CuAssertIntEquals(tc, 3712, calls);
  CuAssertIntEquals(tc, 6912, cleanups);
  game_scheduler_get_stats(scheduler, &stats);
  CuAssertTrue(tc, stats.largest_cascade == 512U);
  if (getenv("LUMINARI_PHASE4_REPORT") != NULL)
    fprintf(stderr,
            "PHASE4 soak_rounds=100 churn_events=6400 storm_events=512 "
            "callback_budget=32 ready_turns=%zu max_ready_latency_turns=%zu "
            "largest_cascade=%" PRIu64 "\n",
            ready_turns, ready_turns - 1U, stats.largest_cascade);
  CuAssertIntEquals(tc, GAME_SCHEDULER_OK, game_scheduler_destroy(scheduler));
}
