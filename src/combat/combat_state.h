#ifndef COMBAT_STATE_H
#define COMBAT_STATE_H

#include "structs.h"

size_t combat_state_count_attackers(const struct char_data *victim);
void combat_state_stop_attackers(struct char_data *victim);

#endif /* COMBAT_STATE_H */
