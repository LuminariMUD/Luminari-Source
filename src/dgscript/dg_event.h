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

/** All Functions handled by the event system must be of this format. */
#define EVENTFUNC(name) long(name)(void *event_obj __attribute__((unused)))

/** Timed-event storage selected once during event_init(). */
enum event_backend_kind
{
  EVENT_BACKEND_UNINITIALIZED = 0,
  EVENT_BACKEND_LEGACY_QUEUE,
  EVENT_BACKEND_GAME_SCHEDULER
};

/** Cancellation/shutdown cleanup that does not expose the compatibility record. */
typedef void (*event_handle_cleanup_func)(event_handle_t handle, void *event_obj);

/* - events - function protos needed by other modules */
void event_init(void);
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

void event_process(void);
void event_process_compatibility_pulse(void);
enum game_scheduler_status event_process_scheduler(
    const struct game_scheduler_budget *budget,
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
                           struct event_debug_snapshot *snapshots,
                           size_t snapshot_capacity, size_t *returned_count);
const char *event_debug_state_name(enum event_debug_state state);
const char *event_debug_owner_kind_name(enum game_event_owner_kind kind);
bool event_debug_parse_owner_kind(const char *name, enum game_event_owner_kind *kind);
bool event_debug_parse_state(const char *name, enum event_debug_state *state);

#if defined(LUMINARI_CUTEST)
void event_test_reset_lifecycle_counts(void);
int event_test_init_call_count(void);
int event_test_free_all_call_count(void);
int event_test_select_backend(enum event_backend_kind backend);
event_handle_t event_test_force_handle_generation_exhaustion(event_handle_t handle);
uint32_t event_test_handle_slot(event_handle_t handle);
#endif

#endif /* _DG_EVENT_H_ */
