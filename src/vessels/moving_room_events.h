#ifndef MOVING_ROOM_EVENTS_H
#define MOVING_ROOM_EVENTS_H

#include "events/domain_events.h"
#include "core/structs.h"

enum domain_event_status moving_room_events_init(void);
void moving_room_events_shutdown(void);
bool moving_room_events_bootstrap(void);
bool moving_room_event_sync(room_rnum room);
void moving_room_event_forget(room_rnum room);
int moving_room_remaining_seconds(room_rnum room);

#endif
