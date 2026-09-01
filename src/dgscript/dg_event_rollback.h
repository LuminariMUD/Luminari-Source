#ifndef DG_EVENT_ROLLBACK_H
#define DG_EVENT_ROLLBACK_H

#if !defined(LUMINARI_ENABLE_EVENT_ROLLBACK) && !defined(LUMINARI_EVENT_ROLLBACK_TESTS)
#error "The legacy timed-event facade is available only in rollback builds"
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "event_handle.h"
#include "game_scheduler.h"

/** Signature retained solely for physical queue rollback callbacks. */
#define EVENTFUNC(name) long(name)(void *event_obj __attribute__((unused)))

typedef void (*event_handle_cleanup_func)(event_handle_t handle, void *event_obj);

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
size_t event_cancel_owner(struct game_event_owner owner);
long event_handle_time(event_handle_t handle);
bool event_handle_is_queued(event_handle_t handle);

void event_process(void);
void event_process_compatibility_pulse(void);

#if defined(LUMINARI_CUTEST)
event_handle_t event_test_force_handle_generation_exhaustion(event_handle_t handle);
uint32_t event_test_handle_slot(event_handle_t handle);
#endif

#endif /* DG_EVENT_ROLLBACK_H */
