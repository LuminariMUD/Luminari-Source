#ifndef COMBAT_STATE_H
#define COMBAT_STATE_H

#include "structs.h"
#include "domain_events.h"

/* Number of characters currently fighting this victim. */
size_t combat_state_count_attackers(const struct char_data *victim);
/* Stop every character currently fighting this victim. */
void combat_state_stop_attackers(struct char_data *victim);
/* Whether an in-progress attack may continue after a callback ran: both
 * participants must still resolve, be alive, and remain in expected_room. */
bool combat_state_attack_context_valid(struct domain_entity_handle attacker,
                                       struct domain_entity_handle victim, room_rnum expected_room);

#endif /* COMBAT_STATE_H */
