/* *************************************************************************
 *   File: resource_cascade.h                          Part of LuminariMUD *
 *  Usage: Header file for Phase 7 ecological resource interdependencies  *
 * Author: Implementation Team                                             *
 ***************************************************************************
 * Phase 7: Ecological Resource Interdependencies                         *
 * Implements realistic ecological relationships where harvesting one      *
 * resource affects the availability of related resources.                 *
 ***************************************************************************/

#ifndef _RESOURCE_CASCADE_H_
#define _RESOURCE_CASCADE_H_

#include "structs.h"
#include "resource_system.h"

/* Ecosystem health states */
enum ecosystem_states {
    ECOSYSTEM_PRISTINE = 0,     /* All resources >80% */
    ECOSYSTEM_HEALTHY,          /* Most resources >60% */
    ECOSYSTEM_STRESSED,         /* Several resources <40% */
    ECOSYSTEM_DEGRADED,         /* Multiple resources <20% */
    ECOSYSTEM_COLLAPSED,        /* Core resources <10% */
    NUM_ECOSYSTEM_STATES
};

/* Resource relationship effect types */
enum cascade_effect_types {
    CASCADE_DEPLETION = 0,      /* Negative effect on target resource */
    CASCADE_ENHANCEMENT,        /* Positive effect on target resource */
    CASCADE_THRESHOLD,          /* Effect only at certain thresholds */
    NUM_CASCADE_TYPES
};

/* Ecosystem health thresholds */
#define ECOSYSTEM_PRISTINE_THRESHOLD    0.80
#define ECOSYSTEM_HEALTHY_THRESHOLD     0.60
#define ECOSYSTEM_STRESSED_THRESHOLD    0.40
#define ECOSYSTEM_DEGRADED_THRESHOLD    0.20
#define ECOSYSTEM_COLLAPSED_THRESHOLD   0.10

/* Conservation scoring */
#define CONSERVATION_PERFECT            1.0
#define CONSERVATION_EXCELLENT          0.8
#define CONSERVATION_GOOD               0.6
#define CONSERVATION_POOR               0.4
#define CONSERVATION_DESTRUCTIVE        0.2

/* Maximum cascade effect per single harvest */
#define MAX_CASCADE_EFFECT              0.25

/* Resource relationship structure */
struct resource_relationship {
    int source_resource;        /* Resource being harvested */
    int target_resource;        /* Resource being affected */
    int effect_type;           /* Type of cascade effect */
    float effect_magnitude;    /* Strength of effect (-1.0 to +1.0) */
    float threshold_min;       /* Minimum threshold for effect */
    float threshold_max;       /* Maximum threshold for effect */
    char description[256];     /* Human readable description */
};

/* Ecosystem health tracking structure */
struct ecosystem_health {
    int zone_vnum;
    int x_coord;
    int y_coord;
    int health_state;
    float health_score;        /* Overall health 0.0-1.0 */
    time_t last_updated;
    float resource_levels[NUM_RESOURCE_TYPES];
};

/* Player conservation tracking structure */
struct player_conservation {
    long player_id;
    int zone_vnum;
    int x_coord;
    int y_coord;
    float conservation_score;  /* 0.0-1.0 conservation rating */
    int total_harvests;
    int sustainable_harvests;
    float ecosystem_damage;    /* Cumulative damage caused */
    time_t last_harvest;
};

/* ===== CORE CASCADE FUNCTIONS ===== */

/* Apply cascade effects when resources are harvested */
void apply_cascade_effects(room_rnum room, int source_resource, int quantity);

/* Enhanced depletion with cascade effects */
void apply_harvest_depletion_with_cascades(room_rnum room, int resource_type, int quantity);

/* Get relationship strength between two resources */
float get_resource_relationship_strength(int source_resource, int target_resource);

/* Load resource relationships from database */
void load_resource_relationships(void);

/* Check for threshold-based ecosystem effects */
void check_ecosystem_thresholds(room_rnum room);

/* ===== ECOSYSTEM HEALTH FUNCTIONS ===== */

/* Calculate overall ecosystem health */
int get_ecosystem_state(room_rnum room);
float calculate_ecosystem_health_score(room_rnum room);

/* Update ecosystem health in database */
void update_ecosystem_health(room_rnum room);

/* Apply ecosystem-wide modifiers based on health */
void apply_ecosystem_modifiers(room_rnum room, int resource_type, float *base_value);

/* Get ecosystem health description */
const char *get_ecosystem_state_name(int state);
const char *get_ecosystem_state_description(int state);

/* ===== CONSERVATION TRACKING FUNCTIONS ===== */

/* Update player conservation score */
void update_player_conservation_score(struct char_data *ch, int resource_type, 
                                    int quantity, bool sustainable);

/* Get player conservation score for area */
float get_player_conservation_score(struct char_data *ch, room_rnum room);

/* Calculate if harvest is sustainable */
bool is_harvest_sustainable(room_rnum room, int resource_type, int quantity);

/* Log cascade effect for debugging */
void log_cascade_effect(room_rnum room, int source_resource, int target_resource, 
                       float effect_magnitude, struct char_data *ch);

/* ===== ENHANCED SURVEY FUNCTIONS ===== */

/* Show ecosystem health and relationships */
void show_ecosystem_analysis(struct char_data *ch, room_rnum room);

/* Show cascade effects of harvesting specific resource */
void show_cascade_preview(struct char_data *ch, room_rnum room, int resource_type);

/* Show resource interaction matrix for area */
void show_resource_relationships(struct char_data *ch, room_rnum room);

/* Show player's conservation impact */
void show_conservation_impact(struct char_data *ch, room_rnum room);

/* ===== UTILITY FUNCTIONS ===== */

/* Get resource name from enum */
const char *get_resource_type_name(int resource_type);

/* Calculate ecosystem recovery rate based on health */
float get_ecosystem_recovery_modifier(int ecosystem_state);

/* Check if ecosystem is in critical state */
bool is_ecosystem_critical(room_rnum room);

/* Get cascade effect color coding for display */
const char *get_cascade_effect_color(float magnitude);

/* Cleanup old ecosystem data */
void cleanup_ecosystem_data(void);

/* ===== ADMINISTRATIVE FUNCTIONS ===== */

/* Reset ecosystem health for area */
void admin_reset_ecosystem(room_rnum room);

/* Show detailed ecosystem debug information */
void admin_show_ecosystem_debug(struct char_data *ch, room_rnum room);

/* Force ecosystem health recalculation */
void admin_recalculate_ecosystem(room_rnum room);

/* Show server-wide ecosystem statistics */
void admin_show_ecosystem_stats(struct char_data *ch);

/* ===== GLOBAL VARIABLES ===== */

extern struct resource_relationship *resource_relationships;
extern int num_resource_relationships;
extern bool cascade_system_enabled;

#endif /* _RESOURCE_CASCADE_H_ */
