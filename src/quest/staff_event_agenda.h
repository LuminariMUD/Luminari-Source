#ifndef STAFF_EVENT_AGENDA_H
#define STAFF_EVENT_AGENDA_H

#include "domain_events.h"

enum domain_event_status staff_event_agenda_init(void);
void staff_event_agenda_shutdown(void);
bool staff_event_agenda_start(int event_num, int ticks);
void staff_event_agenda_cancel(void);
bool staff_event_agenda_delay(int ticks);
int staff_event_agenda_ticks(bool delay);
bool staff_event_agenda_delay_scheduled(void);
int staff_event_agenda_seconds(void);
uint64_t staff_event_agenda_incarnation(void);
void staff_event_maintain_population(void);
void staff_event_maintain_prisoner(void);

#endif
