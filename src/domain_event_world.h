#ifndef DOMAIN_EVENT_WORLD_H
#define DOMAIN_EVENT_WORLD_H

#include "domain_events.h"
#include "structs.h"

enum domain_event_status domain_event_world_register_resolvers(struct domain_event_bus *bus);
struct domain_entity_handle domain_event_room_handle(room_rnum room);
struct domain_entity_handle domain_event_character_handle(struct char_data *ch);

#endif /* DOMAIN_EVENT_WORLD_H */
