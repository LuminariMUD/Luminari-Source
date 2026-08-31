#ifndef ACTIVE_WORLD_H
#define ACTIVE_WORLD_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "domain_events.h"

struct char_data;

enum active_world_mobile_state
{
  ACTIVE_WORLD_MOBILE_DORMANT = 0,
  ACTIVE_WORLD_MOBILE_ACTIVE,
  ACTIVE_WORLD_MOBILE_COOLING
};

enum domain_event_status active_world_register_handlers(struct domain_event_bus *bus);
void active_world_begin_bootstrap(void);
void active_world_end_bootstrap(void);
void active_world_shutdown(void);
bool active_world_enabled(void);
void active_world_sync_mobile(struct char_data *ch);
void active_world_forget_character(struct char_data *ch);
size_t active_world_mobile_count(enum active_world_mobile_state state);
size_t active_world_mobile_admission_limit(void);
size_t active_world_mobile_reason_count(uint32_t reason);
uint64_t active_world_mobile_admission_rejections(void);
uint64_t active_world_mobile_callbacks(void);
void active_world_reset_telemetry(void);

#ifdef LUMINARI_CUTEST
void active_world_reset_for_test(void);
void active_world_select_for_test(bool enabled);
void active_world_set_admission_limit_for_test(size_t limit);
#endif

#endif /* ACTIVE_WORLD_H */
