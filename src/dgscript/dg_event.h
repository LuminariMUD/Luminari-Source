/**
* @file dg_event.h                        LuminariMUD
* This file contains defines for the simplified event system to allow trigedit
* to use the "wait" command, causing a delay in the middle of a script.
* This system could easily be expanded by coders who wish to implement
* an event driven mud.
*
* Part of the core tbaMUD source code distribution, which is a derivative
* of, and continuation of, CircleMUD.
*
* This source code, which was not part of the CircleMUD legacy code,
* is attributed to:
* $Author: Mark A. Heilpern/egreen/Welcor $
* $Date: 2004/10/11 12:07:00$
* $Revision: 1.0.14 $
*/
#ifndef _DG_EVENT_H_
#define _DG_EVENT_H_

#include <stdbool.h>
#include <stdint.h>

#include "event_handle.h"
#include "game_scheduler.h"

/** How often will heartbeat() call the 'wait' event function?
 * @deprecated Currently not used. */
#define PULSE_DG_EVENT 1

/**************************************************************************
 * Begin event structures and defines.
 **************************************************************************/
/** All Functions handled by the event system must be of this format. */
#define EVENTFUNC(name) long(name)(void *event_obj __attribute__((unused)))

struct event;

/** Timed-event storage selected once during event_init(). */
enum event_backend_kind
{
  EVENT_BACKEND_UNINITIALIZED = 0,
  EVENT_BACKEND_LEGACY_QUEUE,
  EVENT_BACKEND_GAME_SCHEDULER
};

/** Optional cleanup invoked by queued/in-flight cancellation or shutdown. */
typedef void (*event_cleanup_func)(struct event *event);

/** Cancellation/shutdown cleanup that does not expose the compatibility record. */
typedef void (*event_handle_cleanup_func)(event_handle_t handle, void *event_obj);

/** Compatibility event record owned by the selected timed-event backend. */
struct event
{
  EVENTFUNC(*func);                /**< The function called when this event comes up. */
  void *event_obj;                 /**< event_obj is passed to func when func is called */
  struct q_element *q_el;          /**< Fallback queue location; NULL on scheduler backend. */
  event_cleanup_func cleanup;      /**< Optional cancellation and bulk cleanup hook. */
  event_handle_cleanup_func handle_cleanup; /**< Opaque-handle cleanup for migrated callers. */
  bool cleanup_on_completion;      /**< Handle cleanup also owns normal completion. */
  event_handle_t handle;           /**< Generation-safe public identity. */
  int profile_index;               /**< PERFMON event callback aggregate slot */
  uint64_t scheduler_id;           /**< Opaque timing-wheel ID; zero for the legacy queue. */
  enum event_backend_kind backend; /**< Storage backend that owns this event. */
  bool dispatching;                /**< Callback is currently executing. */
  bool cancel_requested;           /**< In-flight cancellation must beat recurrence. */
  bool callback_terminal;          /**< Terminal cleanup follows legacy callback ownership. */
  struct game_event_owner owner;   /**< Stable runtime owner for scheduler indexing. */
  uint64_t debug_id;               /**< Backend-neutral read-only diagnostic identity. */
  bool debug_registered;           /**< Linked into the diagnostic registry. */
  struct event *debug_previous;    /**< Diagnostic registry linkage. */
  struct event *debug_next;        /**< Diagnostic registry linkage. */
};
/**************************************************************************
 * End event structures and defines.
 **************************************************************************/

/**************************************************************************
 * Begin priority queue structures and defines.
 **************************************************************************/
/** Number of buckets available in each queue. Reduces enqueue cost.
 *
 * TECHNICAL EXPLANATION FOR BEGINNERS:
 * Instead of having one giant queue for all events, we use multiple
 * smaller queues (buckets). Events are distributed across these buckets
 * based on their scheduled time (using modulo/remainder operation).
 *
 * WHY USE MULTIPLE BUCKETS?
 * - Faster insertion: Searching for the right position in a smaller list
 *   is faster than searching in one huge list.
 * - Better cache performance: Smaller lists fit better in CPU cache.
 * - Distributed processing: Each pulse only checks one bucket.
 *
 * The value 10 was chosen as a good balance between:
 * - Memory usage (more buckets = more memory)
 * - Performance (more buckets = smaller lists to search)
 * - Even distribution (10 divides evenly into many common timer values)
 */
#define NUM_EVENT_QUEUES 10

/** Maximum number of events allowed in the system at once.
 * This prevents resource exhaustion attacks where someone could
 * create millions of events and crash the server.
 *
 * BEGINNERS NOTE: This is a safety limit. Normal gameplay should
 * never reach this limit. If it does, either there's a bug creating
 * too many events, or this limit needs to be increased. */
#define MAX_EVENTS 262144

/** The priority queue. */
struct dg_queue
{
  struct q_element *head[NUM_EVENT_QUEUES]; /**< Front of each queue bucket. */
  struct q_element *tail[NUM_EVENT_QUEUES]; /**< Rear of each queue bucket. */
};

/** Queued elements. */
struct q_element
{
  void *data;                    /**< The event to be handled. */
  long key;                      /**< When the event should be handled. */
  struct q_element *prev, *next; /**< Points to other q_elements in line. */
};
/**************************************************************************
 * End priority queue structures and defines.
 **************************************************************************/

/* - events - function protos needed by other modules */
void event_init(void);
struct event *event_create_named(EVENTFUNC(*func), void *event_obj, long when,
                                 const char *profile_name);
struct event *event_create_named_with_cleanup(EVENTFUNC(*func), void *event_obj, long when,
                                              const char *profile_name, event_cleanup_func cleanup);
struct event *event_create_owned_named(EVENTFUNC(*func), void *event_obj, long when,
                                       const char *profile_name,
                                       struct game_event_owner owner);
struct event *event_create_owned_named_with_cleanup(EVENTFUNC(*func), void *event_obj, long when,
                                                    const char *profile_name,
                                                    event_cleanup_func cleanup,
                                                    struct game_event_owner owner);
#define event_create(func, event_obj, when) event_create_named((func), (event_obj), (when), #func)
#define event_create_with_cleanup(func, event_obj, when, cleanup)                                  \
  event_create_named_with_cleanup((func), (event_obj), (when), #func, (cleanup))

event_handle_t event_schedule_named(EVENTFUNC(*func), void *event_obj, long when,
                                    const char *profile_name);
event_handle_t event_schedule_named_with_cleanup(EVENTFUNC(*func), void *event_obj, long when,
                                                 const char *profile_name,
                                                 event_handle_cleanup_func cleanup);
event_handle_t event_schedule_owned_named(EVENTFUNC(*func), void *event_obj, long when,
                                          const char *profile_name,
                                          struct game_event_owner owner);
event_handle_t event_schedule_owned_named_with_cleanup(
    EVENTFUNC(*func), void *event_obj, long when, const char *profile_name,
    event_handle_cleanup_func cleanup, struct game_event_owner owner);
event_handle_t event_schedule_owned_named_with_terminal_cleanup(
    EVENTFUNC(*func), void *event_obj, long when, const char *profile_name,
    event_handle_cleanup_func cleanup, struct game_event_owner owner);
#define event_schedule(func, event_obj, when)                                                       \
  event_schedule_named((func), (event_obj), (when), #func)
#define event_schedule_with_cleanup(func, event_obj, when, cleanup)                                 \
  event_schedule_named_with_cleanup((func), (event_obj), (when), #func, (cleanup))

bool event_handle_is_live(event_handle_t handle);
bool event_handle_cancel(event_handle_t handle);
long event_handle_time(event_handle_t handle);
bool event_handle_is_queued(event_handle_t handle);

void event_cancel(struct event *event);
void event_process(void);
void event_process_compatibility_pulse(void);
enum game_scheduler_status event_process_scheduler(
    const struct game_scheduler_budget *budget,
    struct game_scheduler_dispatch_report *report);
enum game_scheduler_status event_scheduler_next_deadline(game_tick_t *deadline_tick,
                                                         bool *has_deadline);
long event_time(struct event *event);
void event_free_all(void);
void cleanup_event_obj(struct event *event);
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
                           struct event_debug_snapshot *snapshots,
                           size_t snapshot_capacity, size_t *returned_count);
const char *event_debug_state_name(enum event_debug_state state);
const char *event_debug_owner_kind_name(enum game_event_owner_kind kind);
bool event_debug_parse_owner_kind(const char *name, enum game_event_owner_kind *kind);
bool event_debug_parse_state(const char *name, enum event_debug_state *state);

/* - queues - function protos need by other modules */
struct dg_queue *queue_init(void);
struct q_element *queue_enq(struct dg_queue *q, void *data, long key);
void queue_deq(struct dg_queue *q, struct q_element *qe);
void *queue_head(struct dg_queue *q);
long queue_key(struct dg_queue *q);
long queue_elmt_key(struct q_element *qe);
void queue_free(struct dg_queue *q);
int event_is_queued(struct event *event);

#if defined(LUMINARI_CUTEST)
void event_test_reset_lifecycle_counts(void);
int event_test_init_call_count(void);
int event_test_free_all_call_count(void);
int event_test_select_backend(enum event_backend_kind backend);
event_handle_t event_test_force_handle_generation_exhaustion(event_handle_t handle);
uint32_t event_test_handle_slot(event_handle_t handle);
#endif

#endif /* _DG_EVENT_H_ */
