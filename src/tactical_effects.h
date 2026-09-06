#ifndef TACTICAL_EFFECTS_H
#define TACTICAL_EFFECTS_H

#include <stdbool.h>
struct char_data;

bool tactical_effects_init(void);
void tactical_effects_shutdown(void);
bool tactical_defense_start(struct char_data *ch);
void tactical_defense_resume(struct char_data *ch);
void tactical_defense_pause(struct char_data *ch);
void tactical_defense_on_turn(struct char_data *ch);
void tactical_defense_leave_combat(struct char_data *ch);
int tactical_defense_remaining(struct char_data *ch);

#endif /* TACTICAL_EFFECTS_H */
