#ifndef DOMAIN_EVENT_RUNTIME_H
#define DOMAIN_EVENT_RUNTIME_H

#include "domain_events.h"
#include "structs.h"

enum domain_event_status domain_event_runtime_init(void);
enum domain_event_status domain_event_runtime_shutdown(void);
struct domain_event_bus *domain_event_runtime_bus(void);
enum domain_event_status domain_event_runtime_character_moved(struct char_data *ch,
                                                              room_rnum from_room,
                                                              room_rnum to_room, int direction);
enum domain_event_status domain_event_runtime_character_damaged(struct char_data *target,
                                                                struct char_data *source,
                                                                int amount, int damage_type);
enum domain_event_status domain_event_runtime_combat_state_changed(struct char_data *ch,
                                                                   struct char_data *opponent,
                                                                   bool in_combat);
enum domain_event_status domain_event_runtime_character_died(struct char_data *ch,
                                                             struct char_data *killer);
enum domain_event_status domain_event_runtime_character_died_with_cause(struct char_data *ch,
                                                                        struct char_data *killer,
                                                                        uint32_t cause);
enum domain_event_status domain_event_runtime_character_extracted(struct char_data *ch,
                                                                  uint32_t reason);
enum domain_event_status domain_event_runtime_object_moved(struct obj_data *obj,
                                                           room_rnum from_room,
                                                           room_rnum to_room);

#endif /* DOMAIN_EVENT_RUNTIME_H */
