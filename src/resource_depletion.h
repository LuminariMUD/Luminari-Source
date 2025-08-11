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

/* Database initialization */
void init_resource_depletion_database(void);

/* Basic depletion functions */
float get_resource_depletion_rate(int resource_type);
float get_resource_depletion_level(room_rnum room, int resource_type);
float get_resource_depletion_level_by_coords(int x, int y, int zone_vnum, int resource_type);
void apply_harvest_depletion(room_rnum room, int resource_type, int quantity);

/* Phase 7: Enhanced depletion with cascade effects */
void apply_harvest_depletion_with_cascades(room_rnum room, int resource_type, int quantity);
void apply_cascade_effects(room_rnum room, int source_resource, int quantity);
void apply_single_cascade_effect(room_rnum room, int target_resource, float effect_magnitude, const char *description);

/* Phase 7: Cascade preview and analysis */
void show_cascade_preview(struct char_data *ch, room_rnum room, int resource_type);

/* Phase 7: Ecosystem health tracking */
int get_ecosystem_state(room_rnum room);
void show_ecosystem_analysis(struct char_data *ch, room_rnum room);

/* Regeneration functions */
float get_resource_regeneration_rate(int resource_type);
float get_modified_regeneration_rate(int resource_type, int x, int y);
float calculate_regeneration_amount(int resource_type, time_t last_harvest_time, int x, int y);
void apply_lazy_regeneration(room_rnum room, int resource_type);
bool should_harvest_fail_due_to_depletion(room_rnum room, int resource_type);
float get_harvest_success_modifier(room_rnum room, int resource_type);
const char *get_depletion_level_name(float resource_level);

/* Conservation functions (stubs for now) */
void update_conservation_score(struct char_data *ch, int resource_type, bool sustainable);
float get_player_conservation_score(struct char_data *ch);
const char *get_conservation_status_name(float score);
void show_resource_conservation_status(struct char_data *ch, int x, int y);
void show_regeneration_analysis(struct char_data *ch, int x, int y);

/* Admin/debug functions */
void show_depletion_stats(struct char_data *ch);
void show_conservation_impact(struct char_data *ch);

#endif /* RESOURCE_DEPLETION_H */
