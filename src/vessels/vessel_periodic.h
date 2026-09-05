#ifndef LUMINARI_VESSEL_PERIODIC_H
#define LUMINARI_VESSEL_PERIODIC_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct greyhawk_ship_data;

void vessel_periodic_init(void);
void vessel_periodic_shutdown(void);
void vessel_periodic_rebuild(void);
void vessel_periodic_feature_changed(void);
bool vessel_periodic_events_enabled(void);

void vessel_periodic_sync(struct greyhawk_ship_data *ship);
void vessel_periodic_forget(struct greyhawk_ship_data *ship);

size_t vessel_periodic_owner_count(void);
size_t vessel_periodic_scheduled_count(void);
size_t vessel_periodic_registry_validate(void);
uint64_t vessel_periodic_callbacks(void);
uint64_t vessel_periodic_service_callbacks(void);
uint64_t vessel_periodic_fast_executions(void);
uint64_t vessel_periodic_schedule_executions(void);
uint64_t vessel_periodic_admission_rejections(void);
size_t vessel_periodic_admission_limit(void);
void vessel_periodic_reset_telemetry(void);

#ifdef LUMINARI_CUTEST
void vessel_periodic_select_for_test(bool use_scheduled);
void vessel_periodic_set_admission_limit_for_test(size_t limit);
void vessel_periodic_reset_for_test(void);
#endif

#endif /* LUMINARI_VESSEL_PERIODIC_H */
