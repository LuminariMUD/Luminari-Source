#ifndef _EVENT_HANDLE_H_
#define _EVENT_HANDLE_H_

#include <stdbool.h>
#include <stdint.h>

/** Generation-safe identity for a timed compatibility event; zero is never live. */
typedef uint64_t event_handle_t;

#define EVENT_HANDLE_NONE UINT64_C(0)

/** Opaque identity for an event scheduled directly on the native runtime. */
struct event_runtime_handle
{
  uint64_t id;
};

#define EVENT_RUNTIME_HANDLE_NONE ((struct event_runtime_handle){0})

static inline bool event_runtime_handle_is_none(struct event_runtime_handle handle)
{
  return handle.id == 0U;
}

static inline bool event_runtime_handles_equal(struct event_runtime_handle left,
                                               struct event_runtime_handle right)
{
  return left.id == right.id;
}

#endif /* _EVENT_HANDLE_H_ */
