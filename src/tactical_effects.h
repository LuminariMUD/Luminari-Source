#ifndef TACTICAL_EFFECTS_H
#define TACTICAL_EFFECTS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "domain_events.h"
struct char_data;
struct affected_type;
struct domain_event_bus;
struct raff_node;

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

enum domain_event_status tactical_effects_register_handlers(struct domain_event_bus *bus);
bool tactical_room_hazard_prepare_source(struct raff_node *raff, int level);
void tactical_room_hazard_source_created(struct raff_node *raff);
void tactical_room_hazard_source_removed(struct raff_node *raff);
void tactical_room_hazards_enter_combat(struct char_data *ch);
void tactical_room_hazards_leave_combat(struct char_data *ch);
bool tactical_room_hazards_on_turn_end(struct char_data *ch);
uint64_t tactical_room_hazard_exposures(void);
uint64_t tactical_room_hazard_exposure_rejections(void);

#ifdef LUMINARI_CUTEST
typedef bool (*tactical_hazard_test_callback)(struct raff_node *source, struct char_data *subject,
                                              void *context);
void tactical_effects_set_hazard_test_callback(tactical_hazard_test_callback callback,
                                               void *context);
void tactical_effects_set_hazard_exposure_limit_for_test(size_t limit);
#endif

#endif /* TACTICAL_EFFECTS_H */
