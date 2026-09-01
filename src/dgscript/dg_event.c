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
#include "dg_event_internal.h"
#include "constants.h"
#include "comm.h" /* For access to the game pulse */
#include "dotenv.h"
#include "event_runtime.h"
#include "perfmon.h"
#include <limits.h> /* For LONG_MAX used in overflow checks */

#define LEGACY_EVENT_MAX_EVENTS_PER_OWNER 1024U
#define EVENT_HANDLE_FREE_NONE UINT32_MAX
#define EVENT_HANDLE_SLOT_BITS 19U
#define EVENT_HANDLE_SLOT_MASK ((UINT64_C(1) << EVENT_HANDLE_SLOT_BITS) - UINT64_C(1))
#define EVENT_HANDLE_GENERATION_MAX (UINT64_MAX >> EVENT_HANDLE_SLOT_BITS)

_Static_assert(MAX_EVENTS <= EVENT_HANDLE_SLOT_MASK,
               "event handle slot field must represent every compatibility event");

/***************************************************************************
 * Begin mud specific event queue functions
 **************************************************************************/
/* file scope variables */
/** The mud specific queue of events. */
static struct dg_queue *event_q = NULL;
/** One scheduler type owns all legacy callbacks; PERFMON retains callback identity. */
static game_event_type_id_t legacy_event_type = 0;
/** Backend selection is immutable between event_init() and event_free_all(). */
static enum event_backend_kind active_backend = EVENT_BACKEND_UNINITIALIZED;
/** Flag to track if we're currently processing events (prevents dangerous operations) */
static int processing_events = 0;
/** Counter to track total number of events in the system (resource exhaustion protection) */
static int total_events = 0;
/** New events created by callbacks during the current event_process() call. */
static uint64_t events_created_during_process = 0;
/** Backend-neutral, payload-free registry used by read-only staff diagnostics. */
static struct event *debug_event_head = NULL;
static struct event *debug_event_tail = NULL;
static size_t debug_event_count = 0;
static size_t debug_event_high_water = 0;
static uint64_t next_debug_event_id = 1;
static uint64_t stale_owner_outcomes = 0;
/** Constant-time generation-safe handle registry for migrated event owners. */
static struct event *event_handle_slots[MAX_EVENTS];
static uint64_t event_handle_generations[MAX_EVENTS];
static uint32_t event_handle_next_free[MAX_EVENTS];
static uint32_t event_handle_free_head = EVENT_HANDLE_FREE_NONE;
static uint32_t event_handle_next_unused = 0U;
static size_t event_handle_live_count = 0U;

#if defined(LUMINARI_CUTEST)
static int event_init_calls = 0;
static int event_free_all_calls = 0;
static enum event_backend_kind test_backend_override = EVENT_BACKEND_UNINITIALIZED;
#endif

static game_tick_t legacy_scheduler_tick(void *context);
static uint64_t legacy_scheduler_usec(void *context);
static struct game_event_result
legacy_scheduler_event_handler(const struct game_event_context *context);
static void legacy_scheduler_event_cleanup(void *payload);
static enum event_backend_kind configured_event_backend(void);
static int initialize_scheduler_backend(void);
static void decrement_event_count(const char *context);
static void debug_event_link(struct event *event);
static void debug_event_unlink(struct event *event);
static event_handle_t event_handle_admit(struct event *event);
static struct event *event_handle_resolve(event_handle_t handle);
static void event_handle_release(struct event *event);
static void event_record_free(struct event *event, const char *context);

static const char *backend_kind_name(enum event_backend_kind backend)
{
  switch (backend)
  {
  case EVENT_BACKEND_LEGACY_QUEUE:
    return "legacy";
  case EVENT_BACKEND_GAME_SCHEDULER:
    return "scheduler";
  case EVENT_BACKEND_UNINITIALIZED:
  default:
    return "uninitialized";
  }
}

enum event_backend_kind event_backend_current(void)
{
  return active_backend;
}

const char *event_backend_name(void)
{
  return backend_kind_name(active_backend);
}

static enum event_backend_kind configured_event_backend(void)
{
  const char *configured;

#if defined(LUMINARI_CUTEST)
  if (test_backend_override != EVENT_BACKEND_UNINITIALIZED)
    return test_backend_override;
#endif

  configured = getenv("LUMINARI_EVENT_BACKEND");
  if (configured == NULL || *configured == '\0')
    configured = get_env_value("LUMINARI_EVENT_BACKEND");
  if (configured == NULL || *configured == '\0' || !strcasecmp(configured, "scheduler") ||
      !strcasecmp(configured, "timing-wheel") || !strcasecmp(configured, "timing_wheel"))
    return EVENT_BACKEND_GAME_SCHEDULER;
  if (!strcasecmp(configured, "legacy") || !strcasecmp(configured, "queue"))
    return EVENT_BACKEND_LEGACY_QUEUE;

  log("WARNING: Unknown LUMINARI_EVENT_BACKEND '%s'; using scheduler.", configured);
  return EVENT_BACKEND_GAME_SCHEDULER;
}

static game_tick_t legacy_scheduler_tick(void *context)
{
  (void)context;
  return (game_tick_t)pulse;
}

static uint64_t legacy_scheduler_usec(void *context)
{
  (void)context;
  return PERF_monotonic_usec();
}

static void decrement_event_count(const char *context)
{
  total_events--;
  if (total_events < 0)
  {
    log("SYSERR: Event counter went negative during %s; resetting to zero.", context);
    total_events = 0;
  }
}

static event_handle_t event_handle_admit(struct event *event)
{
  event_handle_t handle;
  uint64_t generation;
  uint32_t slot;

  if (event == NULL || event->handle != EVENT_HANDLE_NONE)
    return EVENT_HANDLE_NONE;
  if (event_handle_free_head != EVENT_HANDLE_FREE_NONE)
  {
    slot = event_handle_free_head;
    event_handle_free_head = event_handle_next_free[slot];
  }
  else
  {
    if (event_handle_next_unused >= MAX_EVENTS)
      return EVENT_HANDLE_NONE;
    slot = event_handle_next_unused++;
  }
  generation = event_handle_generations[slot];
  if (generation == 0U)
  {
    generation = 1U;
    event_handle_generations[slot] = generation;
  }
  handle = (generation << EVENT_HANDLE_SLOT_BITS) | ((event_handle_t)slot + 1U);
  event_handle_slots[slot] = event;
  event->handle = handle;
  event_handle_live_count++;
  return handle;
}

static struct event *event_handle_resolve(event_handle_t handle)
{
  struct event *event;
  uint32_t encoded_slot;
  uint64_t generation;
  uint32_t slot;

  if (handle == EVENT_HANDLE_NONE)
    return NULL;
  encoded_slot = (uint32_t)(handle & EVENT_HANDLE_SLOT_MASK);
  generation = handle >> EVENT_HANDLE_SLOT_BITS;
  if (encoded_slot == 0U || encoded_slot > MAX_EVENTS || generation == 0U)
    return NULL;
  slot = encoded_slot - 1U;
  if (event_handle_generations[slot] != generation)
    return NULL;
  event = event_handle_slots[slot];
  return event != NULL && event->handle == handle ? event : NULL;
}

static void event_handle_release(struct event *event)
{
  event_handle_t handle;
  uint64_t generation;
  uint32_t encoded_slot;
  uint32_t slot;

  if (event == NULL || event->handle == EVENT_HANDLE_NONE)
    return;
  handle = event->handle;
  encoded_slot = (uint32_t)(handle & EVENT_HANDLE_SLOT_MASK);
  if (encoded_slot == 0U || encoded_slot > MAX_EVENTS)
  {
    log("SYSERR: Invalid event handle slot during release.");
    event->handle = EVENT_HANDLE_NONE;
    return;
  }
  slot = encoded_slot - 1U;
  if (event_handle_slots[slot] != event)
  {
    log("SYSERR: Event handle registry mismatch during release.");
    event->handle = EVENT_HANDLE_NONE;
    return;
  }
  event_handle_slots[slot] = NULL;
  generation = event_handle_generations[slot];
  /* A generation is never allowed to wrap: an exhausted slot is retired. */
  if (generation < EVENT_HANDLE_GENERATION_MAX)
  {
    event_handle_generations[slot] = generation + 1U;
    event_handle_next_free[slot] = event_handle_free_head;
    event_handle_free_head = slot;
  }
  event->handle = EVENT_HANDLE_NONE;
  if (event_handle_live_count > 0U)
    event_handle_live_count--;
  else
    log("SYSERR: Event handle registry count underflow.");
}

static void event_record_free(struct event *event, const char *context)
{
  if (event == NULL)
    return;
  debug_event_unlink(event);
  event_handle_release(event);
  free(event);
  decrement_event_count(context);
}

static void debug_event_link(struct event *event)
{
  if (event == NULL || event->debug_registered)
    return;
  event->debug_id = next_debug_event_id++;
  if (next_debug_event_id == 0)
    next_debug_event_id = 1;
  event->debug_previous = debug_event_tail;
  event->debug_next = NULL;
  if (debug_event_tail != NULL)
    debug_event_tail->debug_next = event;
  else
    debug_event_head = event;
  debug_event_tail = event;
  event->debug_registered = true;
  debug_event_count++;
  if (debug_event_count > debug_event_high_water)
    debug_event_high_water = debug_event_count;
}

static void debug_event_unlink(struct event *event)
{
  if (event == NULL || !event->debug_registered)
    return;
  if (event->debug_previous != NULL)
    event->debug_previous->debug_next = event->debug_next;
  else if (debug_event_head == event)
    debug_event_head = event->debug_next;
  if (event->debug_next != NULL)
    event->debug_next->debug_previous = event->debug_previous;
  else if (debug_event_tail == event)
    debug_event_tail = event->debug_previous;
  event->debug_previous = NULL;
  event->debug_next = NULL;
  event->debug_registered = false;
  if (debug_event_count > 0)
    debug_event_count--;
}

static void legacy_scheduler_event_cleanup(void *payload)
{
  struct event *event;

  event = (struct event *)payload;
  if (event == NULL)
    return;

  if (event->callback_terminal)
  {
    if (event->event_obj != NULL &&
        (event->cleanup_on_completion ||
         (event->cancel_requested &&
          (event->cleanup != NULL || event->handle_cleanup != NULL))))
      cleanup_event_obj(event);
  }
  else if (event->event_obj != NULL)
  {
    cleanup_event_obj(event);
  }

  event_record_free(event, "scheduler cleanup");
}

static struct game_event_result
legacy_scheduler_event_handler(const struct game_event_context *context)
{
  struct event *event;
  long next_delay;
  game_tick_t target_tick;
  uint64_t callback_start_usec;
  uint64_t callback_end_usec;
  uint64_t callback_elapsed_usec;

  event = context != NULL ? (struct event *)context->payload : NULL;
  if (event == NULL || event->func == NULL)
  {
    log("SYSERR: Invalid legacy event reached the timing-wheel dispatcher.");
    return game_event_result_failed(1U);
  }

  event->dispatching = true;
  event->cancel_requested = false;
  event->callback_terminal = false;
  callback_start_usec = PERF_monotonic_usec();
  next_delay = (event->func)(event->event_obj);
  callback_end_usec = PERF_monotonic_usec();
  callback_elapsed_usec =
      callback_end_usec >= callback_start_usec ? callback_end_usec - callback_start_usec : 0;
  PERF_note_event_callback(event->profile_index, callback_elapsed_usec);
  event->dispatching = false;

  if (event->cancel_requested)
  {
    event->callback_terminal = true;
    return game_event_result_complete();
  }
  if (next_delay <= 0)
  {
    event->callback_terminal = true;
    return game_event_result_complete();
  }

  if (context->now_tick > LONG_MAX || next_delay > LONG_MAX - (long)context->now_tick)
  {
    log("WARNING: event re-queue overflow prevented. Event scheduled for maximum future time.");
    target_tick = LONG_MAX;
  }
  else
  {
    target_tick = context->now_tick + (game_tick_t)next_delay;
  }
  PERF_note_event_rescheduled(event->profile_index, (uint64_t)next_delay);
  return game_event_result_reschedule_at(target_tick);
}

static int initialize_scheduler_backend(void)
{
  struct game_scheduler_config scheduler_config;
  struct game_event_type_config type_config;
  enum game_scheduler_status status;

  memset(&scheduler_config, 0, sizeof(scheduler_config));
  scheduler_config.max_events = MAX_EVENTS;
  scheduler_config.max_event_types = GAME_SCHEDULER_DEFAULT_MAX_EVENT_TYPES;
  scheduler_config.max_events_per_owner = LEGACY_EVENT_MAX_EVENTS_PER_OWNER;
  scheduler_config.tick_now = legacy_scheduler_tick;
  scheduler_config.monotonic_usec_now = legacy_scheduler_usec;
  scheduler_config.clock_context = NULL;
  status = event_runtime_init(&scheduler_config);
  if (status != GAME_SCHEDULER_OK)
  {
    log("SYSERR: Unable to create timing-wheel event backend (status %d).", status);
    return 0;
  }

  memset(&type_config, 0, sizeof(type_config));
  type_config.name = "legacy_event";
  type_config.handler = legacy_scheduler_event_handler;
  type_config.cleanup = legacy_scheduler_event_cleanup;
  type_config.lateness_policy = GAME_EVENT_LATENESS_RUN_ONCE;
  type_config.max_events = MAX_EVENTS;
  status = event_runtime_register_type(&type_config, &legacy_event_type);
  if (status != GAME_SCHEDULER_OK)
  {
    log("SYSERR: Unable to register legacy event adapter (status %d).", status);
    event_runtime_shutdown();
    legacy_event_type = 0;
    return 0;
  }

  return 1;
}

/** Initializes the main event queue event_q.
 * @post The main event queue, event_q, has been created and initialized.
 */
void event_init(void)
{
#if defined(LUMINARI_CUTEST)
  event_init_calls++;
#endif

  if (active_backend != EVENT_BACKEND_UNINITIALIZED || event_q != NULL ||
      event_runtime_is_initialized())
  {
    log("SYSERR: event_init called while the event system is already initialized");
    return;
  }

  active_backend = configured_event_backend();
  if (active_backend == EVENT_BACKEND_GAME_SCHEDULER && !initialize_scheduler_backend())
  {
    log("WARNING: Falling back to the legacy event queue during initialization.");
    active_backend = EVENT_BACKEND_LEGACY_QUEUE;
  }
  if (active_backend == EVENT_BACKEND_LEGACY_QUEUE)
    event_q = queue_init();

  log("Event backend initialized: %s.", event_backend_name());
}

/** Creates a named event with no custom cancellation cleanup.
 * @see event_create_named_with_cleanup
 */
struct event *event_create_named(EVENTFUNC(*func), void *event_obj, long when,
                                 const char *profile_name)
{
  return event_create_named_with_cleanup(func, event_obj, when, profile_name, NULL);
}

/** Creates a new event object and admits it to the active backend.
 * @post If the newly created event is valid, it is owned by the active backend.
 * @param func The function to be called when this event fires. This function
 * will be passed event_obj when it fires. The function must match the form
 * described by EVENTFUNC.
 * @param event_obj An optional 'something' to be passed to func when this
 * event fires. It is func's job to cast event_obj. If event_obj is not needed,
 * pass in NULL.
 * @param when Number of pulses between firing(s) of this event.
 * @param profile_name Stable callback identity used by PERFMON reports.
 * @param cleanup Optional callback used when an event is canceled or freed in
 * bulk. The event function remains responsible for normal completion.
 * @retval event * Returns a pointer to the newly created event, or NULL on error.
 * */
static struct event *event_create_internal(EVENTFUNC(*func), void *event_obj, long when,
                                           const char *profile_name, event_cleanup_func cleanup,
                                           event_handle_cleanup_func handle_cleanup,
                                           bool cleanup_on_completion,
                                           struct game_event_owner owner)
{
  struct event *new_event = NULL;
  enum game_scheduler_status scheduler_status;
  game_tick_t scheduler_deadline;
  struct event_runtime_handle scheduler_handle;
  long target_time;

  /* Safety check: ensure one backend is initialized. */
  if (active_backend == EVENT_BACKEND_UNINITIALIZED)
  {
    log("SYSERR: event_create called before event_init()");
    return NULL;
  }

  /* CRITICAL: Validate function pointer to prevent crashes.
   *
   * BEGINNERS NOTE: A NULL function pointer would crash the game when
   * event_process() tries to call it. We must catch this error early! */
  if (!func)
  {
    log("SYSERR: event_create called with NULL function pointer");
    return NULL;
  }
  if (!game_event_owner_is_none(owner) && !game_event_owner_is_valid(owner))
  {
    log("SYSERR: event_create called with invalid owner handle");
    return NULL;
  }

  /* RESOURCE EXHAUSTION PROTECTION:
   *
   * PROBLEM: A malicious user or buggy code could create millions of events,
   * using up all server memory and causing a crash.
   *
   * SOLUTION: Limit the total number of events that can exist at once.
   * If we hit the limit, refuse to create new events and log a warning. */
  if (total_events >= MAX_EVENTS)
  {
    log("SYSERR: Maximum number of events (%d) reached! Refusing to create new event.", MAX_EVENTS);
    log("SYSERR: This usually indicates a bug creating too many events.");
    return NULL;
  }

  if (when < 1) /* make sure its in the future */
    when = 1;

  CREATE(new_event, struct event, 1);
  new_event->func = func;
  new_event->event_obj = event_obj;
  new_event->q_el = NULL;
  new_event->cleanup = cleanup;
  new_event->handle_cleanup = handle_cleanup;
  new_event->cleanup_on_completion = cleanup_on_completion;
  new_event->handle = EVENT_HANDLE_NONE;
  new_event->profile_index = PERF_register_event_callback(profile_name);
  new_event->scheduler_handle = EVENT_RUNTIME_HANDLE_NONE;
  new_event->backend = active_backend;
  new_event->dispatching = false;
  new_event->cancel_requested = false;
  new_event->callback_terminal = false;
  new_event->owner = owner;
  if (event_handle_admit(new_event) == EVENT_HANDLE_NONE)
  {
    log("SYSERR: Maximum number of event handles (%d) reached.", MAX_EVENTS);
    free(new_event);
    return NULL;
  }

  if (active_backend == EVENT_BACKEND_GAME_SCHEDULER)
  {
    if ((game_tick_t)when > UINT64_MAX - (game_tick_t)pulse)
    {
      log("WARNING: event_create overflow prevented. Event scheduled for maximum future time.");
      scheduler_deadline = UINT64_MAX;
    }
    else
    {
      scheduler_deadline = (game_tick_t)pulse + (game_tick_t)when;
    }
    scheduler_handle = EVENT_RUNTIME_HANDLE_NONE;
    if (game_event_owner_is_none(owner))
      scheduler_status = event_runtime_schedule_at(legacy_event_type, scheduler_deadline,
                                                   new_event, &scheduler_handle);
    else
      scheduler_status = event_runtime_schedule_owned_at(
          legacy_event_type, owner, scheduler_deadline, new_event, &scheduler_handle);
    if (scheduler_status != GAME_SCHEDULER_OK)
    {
      log("SYSERR: Unable to schedule legacy event '%s' (status %d).",
          profile_name != NULL ? profile_name : "unnamed", scheduler_status);
      event_handle_release(new_event);
      free(new_event);
      return NULL;
    }
    new_event->scheduler_handle = scheduler_handle;
  }
  else
  {
    /* The fallback queue stores absolute pulse keys in signed long values. */
    if (pulse > LONG_MAX || when > LONG_MAX - (long)pulse)
    {
      log("WARNING: event_create overflow prevented. Event scheduled for maximum future time.");
      target_time = LONG_MAX;
    }
    else
    {
      target_time = when + (long)pulse;
    }
    new_event->q_el = queue_enq(event_q, new_event, target_time);
    if (new_event->q_el == NULL)
    {
      event_handle_release(new_event);
      free(new_event);
      return NULL;
    }
  }

  /* Increment our event counter for resource tracking */
  debug_event_link(new_event);
  total_events++;
  if (processing_events && events_created_during_process < UINT64_MAX)
    events_created_during_process++;
  PERF_note_event_scheduled(new_event->profile_index, (uint64_t)when);

  return new_event;
}

struct event *event_create_named_with_cleanup(EVENTFUNC(*func), void *event_obj, long when,
                                              const char *profile_name,
                                              event_cleanup_func cleanup)
{
  return event_create_internal(func, event_obj, when, profile_name, cleanup, NULL, false,
                               game_event_owner_none());
}

struct event *event_create_owned_named(EVENTFUNC(*func), void *event_obj, long when,
                                       const char *profile_name,
                                       struct game_event_owner owner)
{
  return event_create_internal(func, event_obj, when, profile_name, NULL, NULL, false, owner);
}

struct event *event_create_owned_named_with_cleanup(EVENTFUNC(*func), void *event_obj, long when,
                                                    const char *profile_name,
                                                    event_cleanup_func cleanup,
                                                    struct game_event_owner owner)
{
  return event_create_internal(func, event_obj, when, profile_name, cleanup, NULL, false, owner);
}

event_handle_t event_schedule_named(EVENTFUNC(*func), void *event_obj, long when,
                                    const char *profile_name)
{
  return event_schedule_named_with_cleanup(func, event_obj, when, profile_name, NULL);
}

event_handle_t event_schedule_named_with_cleanup(EVENTFUNC(*func), void *event_obj, long when,
                                                 const char *profile_name,
                                                 event_handle_cleanup_func cleanup)
{
  struct event *event;

  event = event_create_internal(func, event_obj, when, profile_name, NULL, cleanup, false,
                                game_event_owner_none());
  return event != NULL ? event->handle : EVENT_HANDLE_NONE;
}

event_handle_t event_schedule_owned_named(EVENTFUNC(*func), void *event_obj, long when,
                                          const char *profile_name,
                                          struct game_event_owner owner)
{
  return event_schedule_owned_named_with_cleanup(func, event_obj, when, profile_name, NULL,
                                                  owner);
}

event_handle_t event_schedule_owned_named_with_cleanup(
    EVENTFUNC(*func), void *event_obj, long when, const char *profile_name,
    event_handle_cleanup_func cleanup, struct game_event_owner owner)
{
  struct event *event;

  event = event_create_internal(func, event_obj, when, profile_name, NULL, cleanup, false, owner);
  return event != NULL ? event->handle : EVENT_HANDLE_NONE;
}

event_handle_t event_schedule_owned_named_with_terminal_cleanup(
    EVENTFUNC(*func), void *event_obj, long when, const char *profile_name,
    event_handle_cleanup_func cleanup, struct game_event_owner owner)
{
  struct event *event;

  if (cleanup == NULL)
    return EVENT_HANDLE_NONE;
  event = event_create_internal(func, event_obj, when, profile_name, NULL, cleanup, true, owner);
  return event != NULL ? event->handle : EVENT_HANDLE_NONE;
}

/** Removes an event from event_q and frees the event.
 * @param event Pointer to the event to be dequeued and removed.
 */
void event_cancel(struct event *event)
{
  enum game_event_cancel_result cancel_result;
  int profile_index;

  if (!event)
  {
    log("SYSERR:  Attempted to cancel a NULL event");
    return;
  }

  /* The active dispatcher owns the event record until the callback returns.
   * A self-cancel records terminal intent and converges on the backend's
   * single cleanup path after the callback. */
  if (event->dispatching)
  {
    if (event->cancel_requested)
      return;

    event->cancel_requested = true;
    event->callback_terminal = true;
    PERF_note_event_cancelled(event->profile_index);

    /* The callback may continue reading its payload after requesting
     * cancellation. The active dispatcher performs terminal cleanup after
     * the callback returns. */

    if (event->backend == EVENT_BACKEND_GAME_SCHEDULER)
    {
      cancel_result = event_runtime_cancel(event->scheduler_handle);
      if (cancel_result == GAME_EVENT_CANCEL_NOT_FOUND)
        log("SYSERR: In-flight legacy event was absent from the timing-wheel scheduler.");
    }

    return; /* The dispatcher owns the event structure until the callback returns. */
  }

  if (event->backend == EVENT_BACKEND_GAME_SCHEDULER)
  {
    profile_index = event->profile_index;
    cancel_result = event_runtime_cancel(event->scheduler_handle);
    if (cancel_result == GAME_EVENT_CANCEL_NOT_FOUND)
      log("SYSERR: Attempted to cancel an event absent from the timing-wheel scheduler.");
    else
      PERF_note_event_cancelled(profile_index);
    return;
  }

  if (event->q_el == NULL)
  {
    log("SYSERR: Attempted to cancel an event that is neither queued nor dispatching.");
    return;
  }

  /* Event is in the queue and not currently running - safe to fully cancel */
  queue_deq(event_q, event->q_el);
  event->q_el = NULL;
  PERF_note_event_cancelled(event->profile_index);

  if (event->event_obj)
    cleanup_event_obj(event);

  event_record_free(event, "legacy cancellation");
}

bool event_handle_is_live(event_handle_t handle)
{
  return event_handle_resolve(handle) != NULL;
}

bool event_handle_cancel(event_handle_t handle)
{
  struct event *event;

  event = event_handle_resolve(handle);
  if (event == NULL)
    return false;
  event_cancel(event);
  return true;
}

size_t event_cancel_owner(struct game_event_owner owner)
{
  struct event *event;
  struct event *next;
  enum game_scheduler_status status;
  size_t cancelled;

  if (!game_event_owner_is_valid(owner) ||
      active_backend == EVENT_BACKEND_UNINITIALIZED)
    return 0U;
  if (active_backend == EVENT_BACKEND_GAME_SCHEDULER)
  {
    cancelled = 0U;
    status = event_runtime_cancel_owner(owner, &cancelled);
    if (status != GAME_SCHEDULER_OK)
    {
      log("SYSERR: Unable to cancel events for owner kind %d (status %d).",
          (int)owner.kind, (int)status);
      return 0U;
    }
    return cancelled;
  }

  cancelled = 0U;
  for (event = debug_event_head; event != NULL; event = next)
  {
    next = event->debug_next;
    if (!game_event_owner_equal(event->owner, owner))
      continue;
    event_cancel(event);
    cancelled++;
  }
  return cancelled;
}

/* Release the payload associated with an event record.
 *
 * Handle-native owners provide a cleanup hook for lifecycle detachment and
 * payload destruction. Compatibility callers may provide a record cleanup
 * hook. Payloads without either hook are assumed to be heap allocations.
 *
 * CRITICAL DESIGN NOTE:
 * Without a cleanup hook, event_obj must be dynamically allocated (malloc'd).
 * If event_obj points to static or stack memory, calling free() will crash!
 * Currently, ALL non-mud events in the codebase use malloc'd memory, so this
 * is safe. If this changes in the future, we'd need a new flag or callback
 * to handle different memory ownership models.
 */
void cleanup_event_obj(struct event *event)
{
  /* Safety check - don't try to free NULL pointers */
  if (!event || !event->event_obj)
    return;

  if (event->handle_cleanup)
  {
    event->handle_cleanup(event->handle, event->event_obj);
  }
  else if (event->cleanup)
  {
    event->cleanup(event);
  }
  else
  {
    free(event->event_obj);
  }

  event->event_obj = NULL;
}

/** Process any events whose time has come. Should be called from, and at, every
 * pulse of heartbeat. Re-enqueues multi-use events.
 *
 * BEGINNERS NOTE: This function runs every game pulse (1/10th second) and
 * executes any events that are scheduled to run now. Events can reschedule
 * themselves by returning a positive value (the delay until next run).
 */
static enum game_scheduler_status
event_process_backend(const struct game_scheduler_budget *budget,
                      struct game_scheduler_dispatch_report *caller_report)
{
  struct event *the_event = NULL;
  struct game_scheduler_dispatch_report scheduler_report;
  enum game_scheduler_status scheduler_status;
  long new_time = 0;
  unsigned long target_time;
  uint64_t callback_start_usec;
  uint64_t callback_end_usec;
  uint64_t callback_elapsed_usec;
  uint64_t callbacks_processed = 0;
  uint64_t created_during_process;
  int queue_depth_before;
  int queue_depth_after;

  if (active_backend == EVENT_BACKEND_UNINITIALIZED)
  {
    log("SYSERR: event_process called before event_init()");
    return GAME_SCHEDULER_INVALID_ARGUMENT;
  }
  if (processing_events)
  {
    log("SYSERR: Recursive event_process call rejected.");
    return GAME_SCHEDULER_BUSY;
  }

  queue_depth_before = total_events;
  events_created_during_process = 0;
  processing_events = 1;

  if (active_backend == EVENT_BACKEND_GAME_SCHEDULER)
  {
    memset(&scheduler_report, 0, sizeof(scheduler_report));
    scheduler_status = event_runtime_advance(budget, &scheduler_report);
    queue_depth_after = total_events;
    created_during_process = events_created_during_process;
    processing_events = 0;
    if (scheduler_status != GAME_SCHEDULER_OK)
      log("SYSERR: Timing-wheel event dispatch failed with status %d.", scheduler_status);
    PERF_note_event_process((uint64_t)queue_depth_before, (uint64_t)queue_depth_after,
                            (uint64_t)scheduler_report.callbacks, created_during_process);
    if (caller_report != NULL)
      *caller_report = scheduler_report;
    return scheduler_status;
  }

  while ((long)pulse >= queue_key(event_q))
  {
    if (!(the_event = (struct event *)queue_head(event_q)))
    {
      log("SYSERR: Attempt to get a NULL event");
      break;
    }

    the_event->q_el = NULL;
    the_event->dispatching = true;
    the_event->cancel_requested = false;
    the_event->callback_terminal = false;

    if (!the_event->func)
    {
      log("SYSERR: Event with NULL function pointer detected in event_process!");
      if (the_event->event_obj != NULL)
        cleanup_event_obj(the_event);
      event_record_free(the_event, "invalid legacy callback");
      continue;
    }

    callback_start_usec = PERF_monotonic_usec();
    new_time = (the_event->func)(the_event->event_obj);
    callback_end_usec = PERF_monotonic_usec();
    callback_elapsed_usec =
        callback_end_usec >= callback_start_usec ? callback_end_usec - callback_start_usec : 0;
    PERF_note_event_callback(the_event->profile_index, callback_elapsed_usec);
    callbacks_processed++;
    the_event->dispatching = false;

    if (the_event->cancel_requested)
    {
      if (the_event->event_obj != NULL &&
          (the_event->cleanup != NULL || the_event->handle_cleanup != NULL))
        cleanup_event_obj(the_event);
      event_record_free(the_event, "in-flight legacy cancellation");
    }
    else if (new_time > 0)
    {
      if (pulse > LONG_MAX || new_time > LONG_MAX - (long)pulse)
      {
        log("WARNING: event re-queue overflow prevented. Event scheduled for maximum future time.");
        target_time = LONG_MAX;
      }
      else
      {
        target_time = new_time + (long)pulse;
      }
      the_event->q_el = queue_enq(event_q, the_event, target_time);
      PERF_note_event_rescheduled(the_event->profile_index, (uint64_t)new_time);
    }
    else
    {
      the_event->callback_terminal = true;
      if (the_event->cleanup_on_completion && the_event->event_obj != NULL)
        cleanup_event_obj(the_event);
      event_record_free(the_event, "legacy completion");
    }
  }

  queue_depth_after = total_events;
  created_during_process = events_created_during_process;
  processing_events = 0;
  PERF_note_event_process((uint64_t)queue_depth_before, (uint64_t)queue_depth_after,
                          callbacks_processed, created_during_process);
  return GAME_SCHEDULER_OK;
}

void event_process(void)
{
  (void)event_process_backend(NULL, NULL);
}

void event_process_compatibility_pulse(void)
{
  if (active_backend == EVENT_BACKEND_LEGACY_QUEUE)
    (void)event_process_backend(NULL, NULL);
}

enum game_scheduler_status event_process_scheduler(
    const struct game_scheduler_budget *budget,
    struct game_scheduler_dispatch_report *report)
{
  if (report == NULL)
    return GAME_SCHEDULER_INVALID_ARGUMENT;
  memset(report, 0, sizeof(*report));
  if (active_backend != EVENT_BACKEND_GAME_SCHEDULER || !event_runtime_is_initialized())
    return GAME_SCHEDULER_INVALID_ARGUMENT;
  return event_process_backend(budget, report);
}

enum game_scheduler_status event_scheduler_next_deadline(game_tick_t *deadline_tick,
                                                         bool *has_deadline)
{
  if (deadline_tick == NULL || has_deadline == NULL)
    return GAME_SCHEDULER_INVALID_ARGUMENT;
  *deadline_tick = 0;
  *has_deadline = false;
  if (active_backend != EVENT_BACKEND_GAME_SCHEDULER || !event_runtime_is_initialized())
    return GAME_SCHEDULER_OK;
  return event_runtime_next_deadline(deadline_tick, has_deadline);
}

/** Returns the time remaining before the event as how many pulses from now.
 * @param event Check this event for it's scheduled activation time.
 * @retval long Number of pulses before this event will fire. */
long event_time(struct event *event)
{
  struct game_event_snapshot snapshot;
  enum game_scheduler_status status;
  uint64_t remaining;
  long when = 0;

  if (event == NULL)
    return 0;
  if (event->backend == EVENT_BACKEND_GAME_SCHEDULER)
  {
    if (!event_runtime_is_initialized() || event->scheduler_handle.id == 0)
      return 0;
    status = event_runtime_inspect(event->scheduler_handle, &snapshot);
    if (status != GAME_SCHEDULER_OK || snapshot.deadline_tick <= (game_tick_t)pulse)
      return 0;
    remaining = snapshot.deadline_tick - (game_tick_t)pulse;
    return remaining > LONG_MAX ? LONG_MAX : (long)remaining;
  }

  if (event->q_el == NULL)
    return 0;
  when = queue_elmt_key(event->q_el);
  if (when <= 0 || pulse >= (unsigned long)when)
    return 0;
  remaining = (uint64_t)(unsigned long)when - (uint64_t)pulse;
  return remaining > LONG_MAX ? LONG_MAX : (long)remaining;
}

long event_handle_time(event_handle_t handle)
{
  return event_time(event_handle_resolve(handle));
}

/** Frees all events from event_q.
 * WARNING: This function should NEVER be called while event_process() is running!
 * Doing so would cause double-free crashes and memory corruption.
 *
 * BEGINNERS NOTE: This function is typically only called during shutdown or
 * when completely resetting the event system. During normal gameplay, use
 * event_cancel() to remove individual events safely.
 */
void event_free_all(void)
{
#if defined(LUMINARI_CUTEST)
  event_free_all_calls++;
#endif

  /* CRITICAL SAFETY CHECK:
   * We must ensure event_process() is not currently running.
   * If it is, we risk freeing events that are being processed,
   * causing crashes when event_process() tries to access them.
   */
  if (processing_events)
  {
    log("SYSERR: event_free_all() called while events are being processed! Aborting to prevent "
        "crash.");
    return;
  }

  if (active_backend == EVENT_BACKEND_UNINITIALIZED)
    return;

  if (active_backend == EVENT_BACKEND_GAME_SCHEDULER)
  {
    if (event_runtime_shutdown() != GAME_SCHEDULER_OK)
    {
      log("SYSERR: Failed to destroy timing-wheel event backend.");
      return;
    }
    legacy_event_type = 0;
  }
  else
  {
    queue_free(event_q);
    event_q = NULL;
  }

  active_backend = EVENT_BACKEND_UNINITIALIZED;
  if (total_events != 0)
  {
    log("SYSERR: Event backend shutdown left %d tracked events; resetting the counter.",
        total_events);
    total_events = 0;
  }
  events_created_during_process = 0;
  if (debug_event_count != 0 || debug_event_head != NULL || debug_event_tail != NULL)
    log("SYSERR: Event diagnostic registry was not empty after backend shutdown.");
  debug_event_head = NULL;
  debug_event_tail = NULL;
  debug_event_count = 0;
  debug_event_high_water = 0;
  next_debug_event_id = 1;
  stale_owner_outcomes = 0;
  if (event_handle_live_count != 0U)
  {
    uint32_t index;

    log("SYSERR: Event handle registry retained %zu live handles after shutdown; invalidating.",
        event_handle_live_count);
    event_handle_free_head = EVENT_HANDLE_FREE_NONE;
    for (index = event_handle_next_unused; index > 0U; index--)
    {
      uint32_t slot = index - 1U;

      if (event_handle_slots[slot] != NULL)
      {
        if (event_handle_generations[slot] < EVENT_HANDLE_GENERATION_MAX)
          event_handle_generations[slot]++;
      }
      event_handle_slots[slot] = NULL;
      if (event_handle_generations[slot] < EVENT_HANDLE_GENERATION_MAX)
      {
        event_handle_next_free[slot] = event_handle_free_head;
        event_handle_free_head = slot;
      }
    }
    event_handle_live_count = 0U;
  }
}

#if defined(LUMINARI_CUTEST)
void event_test_reset_lifecycle_counts(void)
{
  event_init_calls = 0;
  event_free_all_calls = 0;
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
  if (active_backend != EVENT_BACKEND_UNINITIALIZED || event_q != NULL ||
      event_runtime_is_initialized())
    return 0;
  if (backend != EVENT_BACKEND_UNINITIALIZED && backend != EVENT_BACKEND_LEGACY_QUEUE &&
      backend != EVENT_BACKEND_GAME_SCHEDULER)
    return 0;

  test_backend_override = backend;
  return 1;
}

event_handle_t event_test_force_handle_generation_exhaustion(event_handle_t handle)
{
  struct event *event;
  event_handle_t exhausted_handle;
  uint32_t encoded_slot;
  uint32_t slot;

  event = event_handle_resolve(handle);
  if (event == NULL)
    return EVENT_HANDLE_NONE;
  encoded_slot = (uint32_t)(handle & EVENT_HANDLE_SLOT_MASK);
  slot = encoded_slot - 1U;
  event_handle_generations[slot] = EVENT_HANDLE_GENERATION_MAX;
  exhausted_handle = (EVENT_HANDLE_GENERATION_MAX << EVENT_HANDLE_SLOT_BITS) | encoded_slot;
  event->handle = exhausted_handle;
  return exhausted_handle;
}

uint32_t event_test_handle_slot(event_handle_t handle)
{
  return (uint32_t)(handle & EVENT_HANDLE_SLOT_MASK);
}
#endif

/** Boolean function to tell whether an event is queued or not. Does this by
 * checking if event->q_el points to anything but null.
 * @retval int 1 if the event has been queued, 0 if the event has not been
 * queued. */
int event_is_queued(struct event *event)
{
  struct game_event_snapshot snapshot;
  enum game_scheduler_status status;

  if (!event)
    return 0;

  if (event->backend == EVENT_BACKEND_GAME_SCHEDULER)
  {
    if (!event_runtime_is_initialized() || event->scheduler_handle.id == 0 ||
        event->dispatching)
      return 0;
    status = event_runtime_inspect(event->scheduler_handle, &snapshot);
    if (status != GAME_SCHEDULER_OK)
      return 0;
    return snapshot.state == GAME_EVENT_STATE_QUEUED || snapshot.state == GAME_EVENT_STATE_READY;
  }

  return event->q_el != NULL;
}

bool event_handle_is_queued(event_handle_t handle)
{
  return event_is_queued(event_handle_resolve(handle)) != 0;
}
/***************************************************************************
 * End mud specific event queue functions
 **************************************************************************/

/***************************************************************************
 * Begin generic (abstract) priority queue functions
 **************************************************************************/
/** Create a new, empty, priority queue and return it.
 * @retval dg_queue * Pointer to the newly created queue structure. */
struct dg_queue *queue_init(void)
{
  struct dg_queue *q = NULL;
  int i;

  CREATE(q, struct dg_queue, 1);

  /* Initialize all head and tail pointers to NULL to prevent valgrind warnings */
  for (i = 0; i < NUM_EVENT_QUEUES; i++)
  {
    q->head[i] = NULL;
    q->tail[i] = NULL;
  }

  return q;
}

/** Add some 'data' to a priority queue.
 * @pre The paremeter q must have been previously created by queue_init.
 * @post A new q_element is created to hold the data parameter.
 * @param q The existing dg_queue to add an element to.
 * @param data The data to be associated with, and theoretically used, when
 * the element comes up in q. data is wrapped in a new q_element.
 * @param key Indicates where this event should be located in the queue, and
 * when the element should be activated.
 * @retval q_element * Pointer to the created q_element that contains
 * the data. */
struct q_element *queue_enq(struct dg_queue *q, void *data, long key)
{
  struct q_element *qe = NULL, *i = NULL;
  int bucket = 0;

  /* Safety check for NULL queue */
  if (!q)
  {
    log("SYSERR: queue_enq called with NULL queue");
    return NULL;
  }

  CREATE(qe, struct q_element, 1);
  qe->data = data;
  qe->key = key;
  qe->prev = NULL; /* Explicitly initialize to prevent valgrind warnings */
  qe->next = NULL; /* Explicitly initialize to prevent valgrind warnings */

  /* BUCKETING STRATEGY EXPLAINED FOR BEGINNERS:
   *
   * We use the modulo operator (%) to distribute events across buckets.
   * For example, if NUM_EVENT_QUEUES is 10:
   * - Event at pulse 103 goes in bucket 3 (103 % 10 = 3)
   * - Event at pulse 217 goes in bucket 7 (217 % 10 = 7)
   * - Event at pulse 1000 goes in bucket 0 (1000 % 10 = 0)
   *
   * This distributes events evenly and reduces the time needed to find
   * the right insertion point, since we only search within one bucket. */
  bucket = key % NUM_EVENT_QUEUES; /* which queue does this go in */

  if (!q->head[bucket])
  { /* queue is empty */
    q->head[bucket] = qe;
    q->tail[bucket] = qe;
  }

  else
  {
    for (i = q->tail[bucket]; i; i = i->prev)
    {
      if (i->key <= key)
      { /* found insertion point */
        if (i == q->tail[bucket])
          q->tail[bucket] = qe;
        else
        {
          qe->next = i->next;
          i->next->prev = qe;
        }

        qe->prev = i;
        i->next = qe;
        break;
      }
    }

    if (i == NULL)
    { /* insertion point is front of list */
      qe->next = q->head[bucket];
      q->head[bucket] = qe;
      qe->next->prev = qe;
    }
  }

  return qe;
}

/** Remove queue element qe from the priority queue q.
 * @pre qe->data has been dealt with in some way.
 * @post qe has been freed.
 * @param q Pointer to the queue containing qe.
 * @param qe Pointer to the q_element to remove from q.
 */
void queue_deq(struct dg_queue *q, struct q_element *qe)
{
  int i = 0;

  /* CRITICAL SAFETY CHECK:
   * Replace assert with proper NULL check for production safety.
   * Assert only works in debug builds - in production (with NDEBUG),
   * the assert disappears and we'd crash on NULL pointer access!
   *
   * BEGINNERS NOTE:
   * An 'assert' is a debug-only check that disappears in release builds.
   * We need real error checking that works in all builds.
   */
  if (!qe)
  {
    log("SYSERR: queue_deq called with NULL q_element");
    return;
  }

  /* Safety check for NULL queue */
  if (!q)
  {
    log("SYSERR: queue_deq called with NULL queue");
    return;
  }

  i = qe->key % NUM_EVENT_QUEUES;

  if (qe->prev == NULL)
    q->head[i] = qe->next;
  else
    qe->prev->next = qe->next;

  if (qe->next == NULL)
    q->tail[i] = qe->prev;
  else
    qe->next->prev = qe->prev;

  free(qe);
}

/** Removes and returns the data of the first element of the priority queue q.
 * @pre pulse must be defined. This is a multi-headed queue, the current
 * head is determined by the current pulse.
 * @post the q->head is dequeued.
 * @param q The queue to return the head of.
 * @retval void * NULL if there is not a currently available head, pointer
 * to any data object associated with the queue element. */
void *queue_head(struct dg_queue *q)
{
  void *dg_data = NULL;
  int i = 0;

  /* Safety check for NULL queue */
  if (!q)
    return NULL;

  i = pulse % NUM_EVENT_QUEUES;

  if (!q->head[i])
    return NULL;

  dg_data = q->head[i]->data;
  queue_deq(q, q->head[i]);
  return dg_data;
}

/** Returns the key of the head element of the priority queue.
 * @pre pulse must be defined. This is a multi-headed queue, the current
 * head is determined by the current pulse.
 * @param q Queue to check for.
 * @retval long Return the key element of the head q_element. If no head
 * q_element is available, return LONG_MAX. */
long queue_key(struct dg_queue *q)
{
  int i = 0;

  /* Safety check for NULL queue */
  if (!q)
    return LONG_MAX;

  i = pulse % NUM_EVENT_QUEUES;

  if (q->head[i])
    return q->head[i]->key;
  else
    return LONG_MAX;
}

/** Returns the key of queue element qe.
 * @param qe Pointer to the keyed q_element.
 * @retval long Key of qe, or LONG_MAX if qe is NULL.
 *
 * BEGINNERS NOTE: The 'key' represents when this event should fire,
 * measured in game pulses. Lower keys fire sooner.
 */
long queue_elmt_key(struct q_element *qe)
{
  /* Safety check to prevent NULL pointer dereference */
  if (!qe)
  {
    log("WARNING: queue_elmt_key called with NULL q_element");
    return LONG_MAX; /* Return max value to indicate error */
  }
  return qe->key;
}

/** Free q and all contents.
 * @pre Function requires definition of struct event.
 * @post All items associated with q, including non-abstract data, are freed.
 * @param q The priority queue to free.
 *
 * CRITICAL WARNING FOR BEGINNERS:
 * This function frees ALL events in ALL queue buckets. It should NEVER be
 * called while event_process() is running, as that would cause double-free
 * crashes when event_process() tries to access already-freed memory.
 *
 * This is typically only called during shutdown or complete system reset.
 * For removing individual events during gameplay, use event_cancel() instead.
 */
void queue_free(struct dg_queue *q)
{
  int i = 0;
  struct q_element *qe = NULL, *next_qe = NULL;
  struct event *event = NULL;

  /* Safety check for NULL queue */
  if (!q)
  {
    log("WARNING: queue_free called with NULL queue");
    return;
  }

  /* CRITICAL: Check if we're processing events right now */
  if (processing_events)
  {
    log("SYSERR: queue_free() called while event_process() is active! This would cause crashes!");
    log("SYSERR: Stack trace or debugging needed - this should never happen!");
    /* We could abort here but that might lose player data. Log and hope for the best. */
    return;
  }

  /* IMPORTANT: We iterate through all queue buckets (0 to NUM_EVENT_QUEUES-1)
   * Events are distributed across buckets based on their scheduled time
   * to improve performance (reduces search time for insertion). */
  for (i = 0; i < NUM_EVENT_QUEUES; i++)
  {
    /* Process each event in this bucket's linked list */
    for (qe = q->head[i]; qe; qe = next_qe)
    {
      /* Save the next pointer BEFORE freeing current element
       * (once freed, qe->next would be invalid memory!) */
      next_qe = qe->next;

      /* Extract the event from this queue element */
      if ((event = (struct event *)qe->data) != NULL)
      {
        /* DOUBLE-FREE PREVENTION CHECK:
         * If q_el is NULL, this event might be currently processing.
         * However, since queue_free() should NEVER be called during
         * event_process(), we log an error if we detect this situation. */
        if (!event->q_el)
        {
          log("SYSERR: queue_free() found event with NULL q_el - possible concurrent processing!");
          /* Continue anyway as we're likely shutting down */
        }

        /* Free any associated data with this event */
        if (event->event_obj)
          cleanup_event_obj(event);

        event_record_free(event, "legacy shutdown");
      }
      /* Free the queue element that held this event */
      free(qe);
    }
  }

  /* Finally, free the queue structure itself */
  free(q);

  /* Reset event counter since we freed all events */
  if (total_events != 0)
  {
    log("WARNING: Event counter was %d after freeing all events. Resetting to 0.", total_events);
    total_events = 0;
  }
}

int event_queue_depth(void)
{
  struct game_scheduler_stats stats;

  if (active_backend == EVENT_BACKEND_GAME_SCHEDULER && event_runtime_is_initialized())
  {
    event_runtime_get_stats(&stats);
    return stats.event_count > (size_t)INT_MAX ? INT_MAX : (int)stats.event_count;
  }
  return total_events;
}

void event_note_stale_owner_outcome(void)
{
  if (stale_owner_outcomes < UINT64_MAX)
    stale_owner_outcomes++;
}

const char *event_debug_owner_kind_name(enum game_event_owner_kind kind)
{
  static const char *const names[GAME_EVENT_OWNER_KIND_COUNT] = {
      "none",      "world",  "descriptor", "character", "room",    "region",
      "object",    "zone",   "encounter",  "vessel",    "service",
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

static void event_debug_snapshot_one(const struct event *event,
                                     struct event_debug_snapshot *snapshot)
{
  struct game_event_snapshot scheduler_snapshot;
  long remaining;

  memset(snapshot, 0, sizeof(*snapshot));
  snapshot->event_id = event->backend == EVENT_BACKEND_GAME_SCHEDULER
                           ? event->scheduler_handle.id
                           : event->debug_id;
  snprintf(snapshot->type_name, sizeof(snapshot->type_name), "%s",
           PERF_event_callback_identity(event->profile_index));
  snapshot->backend = event->backend;
  snapshot->owner = event->owner;
  if (event->cancel_requested)
    snapshot->state = EVENT_DEBUG_CANCEL_PENDING;
  else if (event->dispatching)
    snapshot->state = EVENT_DEBUG_RUNNING;
  else if (event->backend == EVENT_BACKEND_GAME_SCHEDULER &&
           event_runtime_is_initialized() &&
           event_runtime_inspect(event->scheduler_handle, &scheduler_snapshot) ==
               GAME_SCHEDULER_OK)
    snapshot->state = scheduler_debug_state(scheduler_snapshot.state);
  else
    snapshot->state = EVENT_DEBUG_QUEUED;
  remaining = event_time((struct event *)event);
  snapshot->remaining_pulses = remaining > 0 ? (uint64_t)remaining : 0;
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
  if (filter->type_equals != NULL &&
      strcmp(snapshot->type_name, filter->type_equals) != 0)
    return false;
  if (filter->owner_set &&
      (snapshot->owner.kind != filter->owner.kind ||
       snapshot->owner.runtime_id != filter->owner.runtime_id ||
       (filter->owner_generation_set &&
        snapshot->owner.generation != filter->owner.generation)))
    return false;
  if (filter->minimum_remaining_set &&
      snapshot->remaining_pulses < filter->minimum_remaining)
    return false;
  if (filter->maximum_remaining_set &&
      snapshot->remaining_pulses > filter->maximum_remaining)
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
  if (left->event_id < right->event_id)
    return -1;
  if (left->event_id > right->event_id)
    return 1;
  return 0;
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
                           struct event_debug_snapshot *snapshots,
                           size_t snapshot_capacity, size_t *returned_count)
{
  struct event_debug_snapshot candidate;
  struct game_event_snapshot *native_snapshots;
  struct game_scheduler_stats scheduler_stats;
  struct event *event;
  const char *type_name;
  size_t native_count;
  size_t copied;
  size_t matched;
  size_t index;

  copied = 0;
  matched = 0;
  for (event = debug_event_head; event != NULL; event = event->debug_next)
  {
    event_debug_snapshot_one(event, &candidate);
    if (!event_debug_filter_matches(filter, &candidate))
      continue;
    matched++;
    event_debug_consider_snapshot(&candidate, snapshots, snapshot_capacity, &copied);
  }

  native_snapshots = NULL;
  native_count = 0;
  memset(&scheduler_stats, 0, sizeof(scheduler_stats));
  if (active_backend == EVENT_BACKEND_GAME_SCHEDULER && event_runtime_is_initialized())
  {
    event_runtime_get_stats(&scheduler_stats);
    if (scheduler_stats.event_count > 0)
      native_snapshots = calloc(scheduler_stats.event_count, sizeof(*native_snapshots));
    if (native_snapshots != NULL &&
        event_runtime_inspect_all(native_snapshots, scheduler_stats.event_count,
                                  &native_count) == GAME_SCHEDULER_OK)
    {
      for (index = 0; index < native_count; index++)
      {
        type_name = event_runtime_type_name(native_snapshots[index].event_type);
        if (type_name != NULL && !strcmp(type_name, "legacy_event"))
          continue;
        event_debug_snapshot_native(&native_snapshots[index], &candidate);
        if (!event_debug_filter_matches(filter, &candidate))
          continue;
        matched++;
        event_debug_consider_snapshot(&candidate, snapshots, snapshot_capacity, &copied);
      }
    }
  }
  free(native_snapshots);
  if (copied > 1)
    qsort(snapshots, copied, sizeof(*snapshots), event_debug_snapshot_compare);
  if (returned_count != NULL)
    *returned_count = copied;
  return matched;
}

void event_debug_get_stats(struct event_debug_stats *stats)
{
  const struct event *event;
  const struct event *previous;
  size_t counted;
  size_t mismatches;

  if (stats == NULL)
    return;
  memset(stats, 0, sizeof(*stats));
  stats->backend = active_backend;
  stats->current_pulse = pulse;
  stats->live_events = debug_event_count;
  stats->high_water_events = debug_event_high_water;
  stats->stale_owner_outcomes = stale_owner_outcomes;
  counted = 0;
  mismatches = 0;
  previous = NULL;
  for (event = debug_event_head; event != NULL; event = event->debug_next)
  {
    counted++;
    if (event->owner.kind >= GAME_EVENT_OWNER_NONE &&
        event->owner.kind < GAME_EVENT_OWNER_KIND_COUNT)
      stats->owner_event_counts[event->owner.kind]++;
    if (!event->debug_registered || event->debug_previous != previous)
      mismatches++;
    previous = event;
  }
  if (previous != debug_event_tail || counted != debug_event_count ||
      counted != (size_t)MAX(total_events, 0))
    mismatches++;
  stats->registry_mismatches = mismatches;
  if (active_backend == EVENT_BACKEND_GAME_SCHEDULER && event_runtime_is_initialized())
  {
    event_runtime_get_stats(&stats->scheduler);
    stats->scheduler_stats_available = true;
  }
}
