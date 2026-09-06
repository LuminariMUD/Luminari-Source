#ifndef DOMAIN_EVENT_RUNTIME_H
#define DOMAIN_EVENT_RUNTIME_H

#include "domain_events.h"
#include "domain_event_types.h"
#include "structs.h"

/* Caller-owned synchronous operation. Always finish in reverse begin order.
 * Nested relocation of the same character folds into the outer final outcome.
 * A veto returning to the origin is silent. No runtime pointers enter facts. */
struct domain_relocation_operation
{
  struct domain_character_moved event;
  struct domain_relocation_operation *previous;
  bool active;
};

void domain_relocation_begin(struct domain_relocation_operation *operation, struct char_data *ch,
                             struct char_data *actor, enum domain_relocation_cause cause,
                             int direction);
void domain_relocation_finish(struct domain_relocation_operation *operation);
void domain_relocation_placed(struct char_data *ch, struct domain_entity_handle from_room,
                              room_rnum to_room, struct char_data *actor,
                              enum domain_relocation_cause cause, int direction);

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


#endif /* DOMAIN_EVENT_RUNTIME_H */
