#ifndef TRANSPORT_JOBS_H
#define TRANSPORT_JOBS_H

#include "domain_events.h"
#include "structs.h"

enum domain_event_status transport_jobs_init(struct domain_event_bus *bus);
void transport_jobs_shutdown(void);
bool transport_job_start(struct char_data *ch, room_rnum transit, room_rnum destination,
                         int seconds, int type, int locale);
void transport_job_cancel(struct char_data *ch, bool preserve_remaining);
void transport_job_resume(struct char_data *ch);
int transport_remaining_seconds(const struct char_data *ch);
bool transport_is_transit_room(room_rnum room);
bool transport_locale_valid(int type, int locale);
void transport_arrival(struct char_data *ch, room_rnum destination, int type, int locale);

#endif
