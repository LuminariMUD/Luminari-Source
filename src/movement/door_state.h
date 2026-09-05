#ifndef DOOR_STATE_H
#define DOOR_STATE_H

#include "structs.h"
#include "domain_event_types.h"

#define DOOR_LOCK_FLAGS (EX_LOCKED | EX_LOCKED_EASY | EX_LOCKED_MEDIUM | EX_LOCKED_HARD)

/* Caller-owned, synchronous operation. Never retained or scheduled. Capture
 * before mutation and finish after messages/decision hooks no longer need raw
 * pointers. A paired operation captures only a verified reciprocal exit. */
struct door_state_operation
{
  struct domain_door_state_changed sides[2];
  struct domain_entity_handle destinations[2];
  size_t count;
};

uint64_t door_state_identity(room_rnum room, int direction);
bool door_state_begin(struct door_state_operation *operation, room_rnum room, int direction,
                      bool paired, enum domain_door_change_cause cause);
void door_state_apply(struct door_state_operation *operation, int clear_flags, int set_flags);
void door_state_finish(struct door_state_operation *operation);
void door_state_update(room_rnum room, int direction, int clear_flags, int set_flags, bool paired,
                       enum domain_door_change_cause cause);
void door_state_replace(room_rnum room, int direction, int flags,
                        enum domain_door_change_cause cause);

#endif /* DOOR_STATE_H */
