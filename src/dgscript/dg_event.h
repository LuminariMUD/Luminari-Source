/**
 * @file dg_event.h
 * @brief Process-owned native event runtime lifecycle and diagnostics.
 */
#ifndef _DG_EVENT_H_
#define _DG_EVENT_H_

#include <stdbool.h>
#include <stdint.h>

#include "game_scheduler.h"

/** Timed-event storage selected once during event_init(). */
enum event_backend_kind
{
  EVENT_BACKEND_UNINITIALIZED = 0,
#if (defined(LUMINARI_ENABLE_EVENT_ROLLBACK) && LUMINARI_ENABLE_EVENT_ROLLBACK) ||                 \
    defined(LUMINARI_EVENT_ROLLBACK_TESTS)
  EVENT_BACKEND_LEGACY_QUEUE = 1,
#endif
  EVENT_BACKEND_GAME_SCHEDULER = 2
};

/* Process-owned native timed-event lifecycle. */
void event_init(void);
enum game_scheduler_status event_process_scheduler(const struct game_scheduler_budget *budget,
                                                   struct game_scheduler_dispatch_report *report);
enum game_scheduler_status event_scheduler_next_deadline(game_tick_t *deadline_tick,
                                                         bool *has_deadline);
void event_free_all(void);
int event_queue_depth(void);
enum event_backend_kind event_backend_current(void);
const char *event_backend_name(void);

#define EVENT_DEBUG_NAME_SIZE 64

enum event_debug_state
{
  EVENT_DEBUG_QUEUED = 0,
  EVENT_DEBUG_READY,
  EVENT_DEBUG_RUNNING,
  EVENT_DEBUG_CANCEL_PENDING,
  EVENT_DEBUG_UNKNOWN
};

struct event_debug_filter
{
  bool event_id_set;
  uint64_t event_id;
  const char *type_contains;
  const char *type_equals;
  bool owner_set;
  struct game_event_owner owner;
  bool owner_generation_set;
  bool minimum_remaining_set;
  uint64_t minimum_remaining;
  bool maximum_remaining_set;
  uint64_t maximum_remaining;
  bool state_set;
  enum event_debug_state state;
};

struct event_debug_snapshot
{
  uint64_t event_id;
  char type_name[EVENT_DEBUG_NAME_SIZE];
  enum event_backend_kind backend;
  enum event_debug_state state;
  uint64_t remaining_pulses;
  struct game_event_owner owner;
};

struct event_debug_stats
{
  enum event_backend_kind backend;
  uint64_t current_pulse;
  size_t live_events;
  size_t high_water_events;
  size_t registry_mismatches;
  uint64_t stale_owner_outcomes;
  size_t owner_event_counts[GAME_EVENT_OWNER_KIND_COUNT];
  bool scheduler_stats_available;
  struct game_scheduler_stats scheduler;
};

void event_note_stale_owner_outcome(void);
void event_debug_get_stats(struct event_debug_stats *stats);
size_t event_debug_inspect(const struct event_debug_filter *filter,
                           struct event_debug_snapshot *snapshots, size_t snapshot_capacity,
                           size_t *returned_count);
const char *event_debug_state_name(enum event_debug_state state);
const char *event_debug_owner_kind_name(enum game_event_owner_kind kind);
bool event_debug_parse_owner_kind(const char *name, enum game_event_owner_kind *kind);
bool event_debug_parse_state(const char *name, enum event_debug_state *state);

#if defined(LUMINARI_CUTEST)
void event_test_reset_lifecycle_counts(void);
int event_test_init_call_count(void);
int event_test_free_all_call_count(void);
int event_test_select_backend(enum event_backend_kind backend);
#endif

#endif /* _DG_EVENT_H_ */
