#ifndef DOMAIN_EVENT_WORLD_H
#define DOMAIN_EVENT_WORLD_H

#include "domain_events.h"
#include "structs.h"

enum domain_event_status domain_event_world_register_resolvers(struct domain_event_bus *bus);
void domain_event_world_shutdown(void);
void domain_event_world_forget_character(struct char_data *ch);
void domain_event_world_forget_object(struct obj_data *obj);
struct char_data *domain_event_world_resolve_character(struct domain_entity_handle handle);
struct obj_data *domain_event_world_resolve_object(struct domain_entity_handle handle);
struct domain_entity_handle domain_event_room_handle(room_rnum room);
struct domain_entity_handle domain_event_character_handle(struct char_data *ch);
struct domain_entity_handle domain_event_object_handle(struct obj_data *obj);

#endif /* DOMAIN_EVENT_WORLD_H */
