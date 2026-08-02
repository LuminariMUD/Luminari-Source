/* *************************************************************************
 *   File: resource_depletion.h                        Part of LuminariMUD *
 *  Usage: Simple resource depletion system header                         *
 * Author: Phase 6 Implementation - Step 1: Basic Depletion               *
 ***************************************************************************
 * Simple header for basic resource depletion functionality               *
 ***************************************************************************/

#ifndef RESOURCE_DEPLETION_H
#define RESOURCE_DEPLETION_H

/* ===== FUNCTION PROTOTYPES ===== */

/* Basic depletion functions */
float get_resource_depletion_level(room_rnum room, int resource_type);
void apply_harvest_depletion(room_rnum room, int resource_type, int quantity);
bool should_harvest_fail_due_to_depletion(room_rnum room, int resource_type);
float get_harvest_success_modifier(room_rnum room, int resource_type);
const char *get_depletion_level_name(float resource_level);

/* Conservation functions (stubs for now) */
void update_conservation_score(struct char_data *ch, int resource_type, bool sustainable);
float get_player_conservation_score(struct char_data *ch, int resource_type);
const char *get_conservation_status_name(float score);
void show_resource_conservation_status(struct char_data *ch, int x, int y);
void show_regeneration_analysis(struct char_data *ch, int x, int y);

/* Admin/debug functions */
void show_depletion_stats(struct char_data *ch);

#endif /* RESOURCE_DEPLETION_H */
