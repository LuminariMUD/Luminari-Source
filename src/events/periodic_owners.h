#ifndef PERIODIC_OWNERS_H
#define PERIODIC_OWNERS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct obj_data;
struct script_data;

void periodic_owners_init(void);
void periodic_owners_shutdown(void);

bool periodic_autoproc_enabled(void);
void periodic_autoproc_sync(struct obj_data *obj);
void periodic_autoproc_forget(struct obj_data *obj);
size_t periodic_autoproc_scheduled_count(void);
size_t periodic_autoproc_admission_limit(void);
uint64_t periodic_autoproc_admission_rejections(void);
uint64_t periodic_autoproc_callbacks(void);

bool periodic_dg_random_enabled(void);
void periodic_dg_random_sync(struct script_data *script);
void periodic_dg_random_forget(struct script_data *script);
size_t periodic_dg_random_scheduled_count(int owner_type);
size_t periodic_dg_random_admission_limit(void);
uint64_t periodic_dg_random_admission_rejections(void);
uint64_t periodic_dg_random_callbacks(int owner_type);
uint64_t periodic_dg_random_executions(int owner_type);

void periodic_owners_reset_telemetry(void);

#ifdef LUMINARI_CUTEST
void periodic_owners_reset_for_test(void);
void periodic_owners_select_for_test(bool autoproc_scheduled, bool dg_random_scheduled);
void periodic_owners_set_limits_for_test(size_t autoproc_limit, size_t dg_random_limit);
#endif

#endif /* PERIODIC_OWNERS_H */
