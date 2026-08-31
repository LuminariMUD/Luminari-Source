#ifndef CHARACTER_PERIODIC_H
#define CHARACTER_PERIODIC_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "domain_events.h"

struct char_data;

enum domain_event_status character_periodic_register_handlers(struct domain_event_bus *bus);

void character_periodic_init(void);
void character_periodic_shutdown(void);
bool character_periodic_events_enabled(void);

void character_periodic_sync(struct char_data *ch);
void character_periodic_forget(struct char_data *ch);

size_t character_periodic_owner_count(void);
size_t character_periodic_scheduled_count(void);
size_t character_periodic_admission_limit(void);
size_t character_periodic_registry_validate(void);
uint64_t character_periodic_admission_rejections(void);
uint64_t character_periodic_callbacks(void);
uint64_t character_periodic_walk_executions(void);
uint64_t character_periodic_psp_executions(void);
uint64_t character_periodic_bardic_executions(void);
uint64_t character_periodic_hint_executions(void);
uint64_t character_periodic_luminari_executions(void);
uint64_t character_periodic_damage_effect_executions(void);
uint64_t character_periodic_player_misc_executions(void);
void character_periodic_reset_telemetry(void);

#ifdef LUMINARI_CUTEST
void character_periodic_reset_for_test(void);
void character_periodic_select_for_test(bool scheduled);
void character_periodic_set_limit_for_test(size_t limit);
#endif

#endif /* CHARACTER_PERIODIC_H */
