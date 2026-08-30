#ifndef DOMAIN_EVENT_RUNTIME_H
#define DOMAIN_EVENT_RUNTIME_H

#include "domain_events.h"

enum domain_event_status domain_event_runtime_init(void);
enum domain_event_status domain_event_runtime_shutdown(void);
struct domain_event_bus *domain_event_runtime_bus(void);

#endif /* DOMAIN_EVENT_RUNTIME_H */
