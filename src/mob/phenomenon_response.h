#ifndef PHENOMENON_RESPONSE_H
#define PHENOMENON_RESPONSE_H

#include <stdbool.h>
#include <stdint.h>

#include "domain_events.h"

struct char_data;

enum domain_event_status phenomenon_response_init(struct domain_event_bus *bus);
void phenomenon_response_shutdown(void);
uint64_t phenomenon_response_admission_rejections(void);

#ifdef LUMINARI_CUTEST
typedef void (*phenomenon_response_test_callback)(struct char_data *observer, bool responding,
                                                  void *context);
void phenomenon_response_set_test_callback(phenomenon_response_test_callback callback,
                                           void *context);
#endif

#endif /* PHENOMENON_RESPONSE_H */
