/* *************************************************************************
 *   File: resource_regeneration.h                     Part of LuminariMUD *
 *  Usage: Resource regeneration and recovery system header               *
 * Author: Phase 6 Implementation - Step 4: Regeneration Events          *
 ***************************************************************************
 * Header for automatic resource regeneration over time, including        *
 * seasonal variations, weather effects, and event-based scheduling.      *
 ***************************************************************************/

#ifndef RESOURCE_REGENERATION_H
#define RESOURCE_REGENERATION_H

/* ===== REGENERATION SYSTEM FUNCTIONS ===== */

/* Core regeneration functions */
float get_base_regeneration_rate(int resource_type);
float calculate_regeneration_rate(int resource_type, int zone_vnum, int x, int y);
bool regenerate_resource_location(int zone_vnum, int x, int y, int resource_type);
void process_global_regeneration(void);

/* System control */
void init_resource_regeneration(void);
void set_regeneration_enabled(bool enabled);
bool is_regeneration_enabled(void);

/* Statistics and monitoring */
void get_regeneration_stats(int *total_depleted, int *regenerating_locations);

/* Event system integration */
EVENTFUNC(regeneration_event);

/* Seasonal and environmental modifiers */
float get_seasonal_modifier(int resource_type);
float get_weather_modifier(int resource_type, int zone_vnum);
float get_terrain_modifier(int resource_type, int x, int y);

#endif /* RESOURCE_REGENERATION_H */
