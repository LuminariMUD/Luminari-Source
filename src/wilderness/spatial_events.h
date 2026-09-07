#ifndef SPATIAL_EVENTS_H
#define SPATIAL_EVENTS_H

#include <stddef.h>
#include <stdint.h>

#include "domain_events.h"

enum domain_event_status spatial_event_register_handlers(struct domain_event_bus *bus);
uint64_t spatial_event_perception_rejections(void);

#ifdef LUMINARI_CUTEST
void spatial_event_set_perception_limit_for_test(size_t limit);
#endif

#endif /* SPATIAL_EVENTS_H */
