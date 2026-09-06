#ifndef STAFF_EVENT_AGENDA_H
#define STAFF_EVENT_AGENDA_H

#include "domain_events.h"

enum domain_event_status staff_event_agenda_init(void);
void staff_event_agenda_shutdown(void);
bool staff_event_agenda_start(int event_num, int hours);
void staff_event_agenda_cancel(void);
bool staff_event_agenda_delay(int hours);
int staff_event_agenda_hours(bool delay);
int staff_event_agenda_seconds(void);
uint64_t staff_event_agenda_incarnation(void);
void staff_event_maintain_population(void);
void staff_event_maintain_prisoner(void);

#endif
