/**
* @file dg_event.c                          LuminariMUD
* This file contains a simplified event system to allow trigedit
* to use the "wait" command, causing a delay in the middle of a script.
* This system could easily be expanded by coders who wish to implement
* an event driven mud.
*
* Part of the core tbaMUD source code distribution, which is a derivative
* of, and continuation of, CircleMUD.
*
* This source code, which was not part of the CircleMUD legacy code,
* was created by the following people:
* $Author: Mark A. Heilpern/egreen/Welcor $
* $Date: 2004/10/11 12:07:00$
* $Revision: 1.0.14 $
*
* Re-written by LuminariMUD staff to fix the original code.
*/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "db.h"
#include "dg_event.h"
#include "constants.h"
#include "comm.h" /* For access to the game pulse */
#include "event_runtime.h"
#include "perfmon.h"
#include <limits.h> /* For LONG_MAX used in overflow checks */


#define NATIVE_EVENT_MAX_EVENTS 262144U
#define NATIVE_EVENT_MAX_EVENTS_PER_OWNER 1024U

static enum event_backend_kind active_backend = EVENT_BACKEND_UNINITIALIZED;
static uint64_t stale_owner_outcomes;
static size_t native_event_high_water;

#if defined(LUMINARI_CUTEST)
static int event_init_calls;
static int event_free_all_calls;
#endif

static game_tick_t native_scheduler_tick(void *context)
{
  (void)context;
  return (game_tick_t)pulse;
}

static uint64_t native_scheduler_usec(void *context)
{
  (void)context;
  return PERF_monotonic_usec();
}

enum event_backend_kind event_backend_current(void)
{
  return active_backend;
}

const char *event_backend_name(void)
{
  return active_backend == EVENT_BACKEND_GAME_SCHEDULER ? "scheduler" : "uninitialized";
}

void event_init(void)
{
  struct game_scheduler_config config;
  enum game_scheduler_status status;

#if defined(LUMINARI_CUTEST)
  event_init_calls++;
#endif
  if (active_backend != EVENT_BACKEND_UNINITIALIZED || event_runtime_is_initialized())
  {
    log("SYSERR: event_init called while the native event runtime is already initialized");
    return;
  }
  memset(&config, 0, sizeof(config));
  config.max_events = NATIVE_EVENT_MAX_EVENTS;
  config.max_event_types = GAME_SCHEDULER_DEFAULT_MAX_EVENT_TYPES;
  config.max_events_per_owner = NATIVE_EVENT_MAX_EVENTS_PER_OWNER;
  config.tick_now = native_scheduler_tick;
  config.monotonic_usec_now = native_scheduler_usec;
  status = event_runtime_init(&config);
  if (status != GAME_SCHEDULER_OK)
  {
    log("SYSERR: Unable to initialize native timing-wheel runtime (status %d).", status);
    return;
  }
  active_backend = EVENT_BACKEND_GAME_SCHEDULER;
  native_event_high_water = 0U;
  log("Event runtime initialized: scheduler.");
}

enum game_scheduler_status event_process_scheduler(const struct game_scheduler_budget *budget,
                                                   struct game_scheduler_dispatch_report *report)
{
  enum game_scheduler_status status;
  size_t depth_before;
  size_t depth_after;

  if (report == NULL || active_backend != EVENT_BACKEND_GAME_SCHEDULER ||
      !event_runtime_is_initialized())
    return GAME_SCHEDULER_INVALID_ARGUMENT;
  memset(report, 0, sizeof(*report));
  depth_before = event_runtime_event_count();
  status = event_runtime_advance(budget, report);
  depth_after = event_runtime_event_count();
  if (depth_after > native_event_high_water)
    native_event_high_water = depth_after;
  PERF_note_event_process((uint64_t)depth_before, (uint64_t)depth_after,
                          (uint64_t)report->callbacks, 0U);
  if (status != GAME_SCHEDULER_OK)
    log("SYSERR: Timing-wheel event dispatch failed with status %d.", status);
  return status;
}

enum game_scheduler_status event_scheduler_next_deadline(game_tick_t *deadline_tick,
                                                         bool *has_deadline)
{
  if (deadline_tick == NULL || has_deadline == NULL)
    return GAME_SCHEDULER_INVALID_ARGUMENT;
  *deadline_tick = 0U;
  *has_deadline = false;
  if (active_backend != EVENT_BACKEND_GAME_SCHEDULER || !event_runtime_is_initialized())
    return GAME_SCHEDULER_OK;
  return event_runtime_next_deadline(deadline_tick, has_deadline);
}

void event_free_all(void)
{
#if defined(LUMINARI_CUTEST)
  event_free_all_calls++;
#endif
  if (active_backend == EVENT_BACKEND_UNINITIALIZED)
    return;
  if (event_runtime_shutdown() != GAME_SCHEDULER_OK)
  {
    log("SYSERR: Failed to destroy native timing-wheel runtime.");
    return;
  }
  active_backend = EVENT_BACKEND_UNINITIALIZED;
  stale_owner_outcomes = 0U;
  native_event_high_water = 0U;
}

int event_queue_depth(void)
{
  struct game_scheduler_stats stats;

  if (!event_runtime_is_initialized())
    return 0;
  memset(&stats, 0, sizeof(stats));
  event_runtime_get_stats(&stats);
  if (stats.event_count > native_event_high_water)
    native_event_high_water = stats.event_count;
  return stats.event_count > (size_t)INT_MAX ? INT_MAX : (int)stats.event_count;
}

void event_note_stale_owner_outcome(void)
{
  if (stale_owner_outcomes < UINT64_MAX)
    stale_owner_outcomes++;
}

const char *event_debug_owner_kind_name(enum game_event_owner_kind kind)
{
  static const char *const names[GAME_EVENT_OWNER_KIND_COUNT] = {
      "none",   "world", "descriptor", "character", "room",    "region",
      "object", "zone",  "encounter",  "vessel",    "service",
  };

  if (kind < GAME_EVENT_OWNER_NONE || kind >= GAME_EVENT_OWNER_KIND_COUNT)
    return "unknown";
  return names[kind];
}

bool event_debug_parse_owner_kind(const char *name, enum game_event_owner_kind *kind)
{
  enum game_event_owner_kind candidate;

  if (name == NULL || kind == NULL)
    return false;
  for (candidate = GAME_EVENT_OWNER_NONE; candidate < GAME_EVENT_OWNER_KIND_COUNT; candidate++)
  {
    if (!strcasecmp(name, event_debug_owner_kind_name(candidate)) ||
        (candidate == GAME_EVENT_OWNER_CHARACTER && !strcasecmp(name, "char")) ||
        (candidate == GAME_EVENT_OWNER_DESCRIPTOR && !strcasecmp(name, "desc")) ||
        (candidate == GAME_EVENT_OWNER_OBJECT && !strcasecmp(name, "obj")))
    {
      *kind = candidate;
      return true;
    }
  }
  return false;
}

const char *event_debug_state_name(enum event_debug_state state)
{
  switch (state)
  {
  case EVENT_DEBUG_QUEUED:
    return "queued";
  case EVENT_DEBUG_READY:
    return "ready";
  case EVENT_DEBUG_RUNNING:
    return "running";
  case EVENT_DEBUG_CANCEL_PENDING:
    return "cancel-pending";
  case EVENT_DEBUG_UNKNOWN:
  default:
    return "unknown";
  }
}

bool event_debug_parse_state(const char *name, enum event_debug_state *state)
{
  enum event_debug_state candidate;

  if (name == NULL || state == NULL)
    return false;
  for (candidate = EVENT_DEBUG_QUEUED; candidate <= EVENT_DEBUG_UNKNOWN; candidate++)
  {
    if (!strcasecmp(name, event_debug_state_name(candidate)) ||
        (candidate == EVENT_DEBUG_RUNNING && !strcasecmp(name, "dispatching")) ||
        (candidate == EVENT_DEBUG_CANCEL_PENDING && !strcasecmp(name, "cancel")))
    {
      *state = candidate;
      return true;
    }
  }
  return false;
}

static enum event_debug_state scheduler_debug_state(enum game_event_state state)
{
  switch (state)
  {
  case GAME_EVENT_STATE_QUEUED:
    return EVENT_DEBUG_QUEUED;
  case GAME_EVENT_STATE_READY:
    return EVENT_DEBUG_READY;
  case GAME_EVENT_STATE_DISPATCHING:
    return EVENT_DEBUG_RUNNING;
  case GAME_EVENT_STATE_CANCEL_PENDING:
    return EVENT_DEBUG_CANCEL_PENDING;
  default:
    return EVENT_DEBUG_UNKNOWN;
  }
}

static bool event_debug_filter_matches(const struct event_debug_filter *filter,
                                       const struct event_debug_snapshot *snapshot)
{
  if (filter == NULL)
    return true;
  if (filter->event_id_set && snapshot->event_id != filter->event_id)
    return false;
  if (filter->type_contains != NULL && *filter->type_contains != '\0' &&
      strcasestr(snapshot->type_name, filter->type_contains) == NULL)
    return false;
  if (filter->type_equals != NULL && strcmp(snapshot->type_name, filter->type_equals) != 0)
    return false;
  if (filter->owner_set &&
      (snapshot->owner.kind != filter->owner.kind ||
       snapshot->owner.runtime_id != filter->owner.runtime_id ||
       (filter->owner_generation_set && snapshot->owner.generation != filter->owner.generation)))
    return false;
  if (filter->minimum_remaining_set && snapshot->remaining_pulses < filter->minimum_remaining)
    return false;
  if (filter->maximum_remaining_set && snapshot->remaining_pulses > filter->maximum_remaining)
    return false;
  if (filter->state_set && snapshot->state != filter->state)
    return false;
  return true;
}

static int event_debug_snapshot_compare(const void *left_pointer, const void *right_pointer)
{
  const struct event_debug_snapshot *left = left_pointer;
  const struct event_debug_snapshot *right = right_pointer;

  if (left->remaining_pulses < right->remaining_pulses)
    return -1;
  if (left->remaining_pulses > right->remaining_pulses)
    return 1;
  return left->event_id < right->event_id ? -1 : left->event_id > right->event_id;
}

static void event_debug_consider_snapshot(const struct event_debug_snapshot *candidate,
                                          struct event_debug_snapshot *snapshots,
                                          size_t snapshot_capacity, size_t *copied)
{
  size_t worst;
  size_t index;

  if (candidate == NULL || snapshots == NULL || snapshot_capacity == 0 || copied == NULL)
    return;
  if (*copied < snapshot_capacity)
  {
    snapshots[(*copied)++] = *candidate;
    return;
  }
  worst = 0;
  for (index = 1; index < *copied; index++)
    if (event_debug_snapshot_compare(&snapshots[worst], &snapshots[index]) < 0)
      worst = index;
  if (event_debug_snapshot_compare(candidate, &snapshots[worst]) < 0)
    snapshots[worst] = *candidate;
}

static void event_debug_snapshot_native(const struct game_event_snapshot *event,
                                        struct event_debug_snapshot *snapshot)
{
  const char *type_name;

  memset(snapshot, 0, sizeof(*snapshot));
  snapshot->event_id = event->event_id;
  type_name = event_runtime_type_name(event->event_type);
  snprintf(snapshot->type_name, sizeof(snapshot->type_name), "%s",
           type_name != NULL ? type_name : "unknown");
  snapshot->backend = EVENT_BACKEND_GAME_SCHEDULER;
  snapshot->state = scheduler_debug_state(event->state);
  snapshot->remaining_pulses =
      event->deadline_tick > (game_tick_t)pulse ? event->deadline_tick - (game_tick_t)pulse : 0U;
  snapshot->owner = event->owner;
}

size_t event_debug_inspect(const struct event_debug_filter *filter,
                           struct event_debug_snapshot *snapshots, size_t snapshot_capacity,
                           size_t *returned_count)
{
  struct game_scheduler_stats stats;
  struct game_event_snapshot *events;
  struct event_debug_snapshot candidate;
  size_t event_count;
  size_t copied;
  size_t matched;
  size_t index;

  copied = 0U;
  matched = 0U;
  event_count = 0U;
  events = NULL;
  memset(&stats, 0, sizeof(stats));
  if (event_runtime_is_initialized())
  {
    event_runtime_get_stats(&stats);
    if (stats.event_count > 0U)
      events = calloc(stats.event_count, sizeof(*events));
    if (events != NULL &&
        event_runtime_inspect_all(events, stats.event_count, &event_count) == GAME_SCHEDULER_OK)
    {
      for (index = 0; index < event_count; index++)
      {
        event_debug_snapshot_native(&events[index], &candidate);
        if (!event_debug_filter_matches(filter, &candidate))
          continue;
        matched++;
        event_debug_consider_snapshot(&candidate, snapshots, snapshot_capacity, &copied);
      }
    }
  }
  free(events);
  if (copied > 1U)
    qsort(snapshots, copied, sizeof(*snapshots), event_debug_snapshot_compare);
  if (returned_count != NULL)
    *returned_count = copied;
  return matched;
}

void event_debug_get_stats(struct event_debug_stats *stats)
{
  size_t owned_count = 0U;
  size_t kind;

  if (stats == NULL)
    return;
  memset(stats, 0, sizeof(*stats));
  stats->backend = active_backend;
  stats->current_pulse = pulse;
  stats->stale_owner_outcomes = stale_owner_outcomes;
  if (!event_runtime_is_initialized())
    return;
  event_runtime_get_stats(&stats->scheduler);
  stats->scheduler_stats_available = true;
  stats->live_events = stats->scheduler.event_count;
  if (stats->live_events > native_event_high_water)
    native_event_high_water = stats->live_events;
  stats->high_water_events = native_event_high_water;
  memcpy(stats->owner_event_counts, stats->scheduler.owner_counts,
         sizeof(stats->owner_event_counts));
  for (kind = GAME_EVENT_OWNER_NONE + 1U; kind < GAME_EVENT_OWNER_KIND_COUNT; kind++)
    owned_count += stats->owner_event_counts[kind];
  stats->owner_event_counts[GAME_EVENT_OWNER_NONE] =
      stats->live_events >= owned_count ? stats->live_events - owned_count : 0U;
}

#if defined(LUMINARI_CUTEST)
void event_test_reset_lifecycle_counts(void)
{
  event_init_calls = 0;
  event_free_all_calls = 0;
}

void event_test_advance(void)
{
  struct game_scheduler_dispatch_report report;

  (void)event_process_scheduler(NULL, &report);
}

int event_test_init_call_count(void)
{
  return event_init_calls;
}

int event_test_free_all_call_count(void)
{
  return event_free_all_calls;
}

int event_test_select_backend(enum event_backend_kind backend)
{
  return backend == EVENT_BACKEND_GAME_SCHEDULER && active_backend == EVENT_BACKEND_UNINITIALIZED &&
         !event_runtime_is_initialized();
}
#endif
