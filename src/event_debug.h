#ifndef EVENT_DEBUG_H
#define EVENT_DEBUG_H

#include <stddef.h>

#include "act.h"
#include "dgscript/dg_event.h"

int event_debug_effective_width(int configured_width);
size_t event_debug_render_help(char *buffer, size_t capacity, int width);
size_t event_debug_render_summary(char *buffer, size_t capacity, int width);
size_t event_debug_render_queue(char *buffer, size_t capacity, int width,
                                const struct event_debug_filter *filter, size_t limit);
size_t event_debug_render_profiles(char *buffer, size_t capacity, int width, size_t limit);
size_t event_debug_render_domain(char *buffer, size_t capacity, int width,
                                 const char *type_filter);

ACMD_DECL(do_eventdebug);

#endif /* EVENT_DEBUG_H */
