#ifndef POINT_UPDATE_PERIODIC_H
#define POINT_UPDATE_PERIODIC_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct char_data;
struct obj_data;

void point_update_periodic_init(void);
void point_update_periodic_shutdown(void);
bool point_update_events_enabled(void);

void point_update_character_sync(struct char_data *ch);
void point_update_character_forget(struct char_data *ch);
void point_update_object_sync(struct obj_data *obj);
void point_update_object_forget(struct obj_data *obj);
void point_update_object_spec_timer_set(struct obj_data *obj, int slot, int duration);

bool point_update_periodic_dispatch_due(void);

size_t point_update_character_count(void);
size_t point_update_object_count(void);
size_t point_update_character_registry_validate(void);
size_t point_update_object_registry_validate(void);
uint64_t point_update_service_callbacks(void);
uint64_t point_update_dispatches(void);
uint64_t point_update_character_executions(void);
uint64_t point_update_object_executions(void);
void point_update_periodic_reset_telemetry(void);

#ifdef LUMINARI_CUTEST
void point_update_periodic_reset_for_test(void);
void point_update_periodic_select_for_test(bool use_scheduled);
#endif

#endif /* POINT_UPDATE_PERIODIC_H */
