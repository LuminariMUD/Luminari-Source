#ifndef BUFF_SEQUENCE_H
#define BUFF_SEQUENCE_H

#include "domain_events.h"

struct char_data;
enum domain_event_status buff_sequences_init(struct domain_event_bus *bus);
void buff_sequences_shutdown(void);
bool buff_sequence_start(struct char_data *ch);
void buff_sequence_cancel(struct char_data *ch);

#endif
