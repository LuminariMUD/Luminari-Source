#ifndef _EVENT_HANDLE_H_
#define _EVENT_HANDLE_H_

#include <stdint.h>

/** Generation-safe identity for a timed compatibility event; zero is never live. */
typedef uint64_t event_handle_t;

#define EVENT_HANDLE_NONE UINT64_C(0)

#endif /* _EVENT_HANDLE_H_ */
