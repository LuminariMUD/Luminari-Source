#ifndef TACTICAL_EFFECTS_H
#define TACTICAL_EFFECTS_H

#include <stdbool.h>
#include <stdint.h>
struct char_data;
struct affected_type;

bool tactical_effects_init(void);
void tactical_effects_shutdown(void);
bool tactical_defense_start(struct char_data *ch);
void tactical_defense_resume(struct char_data *ch);
void tactical_defense_pause(struct char_data *ch);
void tactical_defense_on_turn(struct char_data *ch);
void tactical_defense_leave_combat(struct char_data *ch);
int tactical_defense_remaining(struct char_data *ch);

bool tactical_bleeding_affect(const struct affected_type *af);
void tactical_bleeding_sync(struct char_data *ch);
void tactical_bleeding_pause(struct char_data *ch);
void tactical_bleeding_leave_combat(struct char_data *ch);
bool tactical_bleeding_on_turn_end(struct char_data *ch);
int tactical_bleeding_remaining(struct char_data *ch);
void tactical_bleeding_restore_clock(struct char_data *ch, int remaining, uint64_t turn);

#endif /* TACTICAL_EFFECTS_H */
