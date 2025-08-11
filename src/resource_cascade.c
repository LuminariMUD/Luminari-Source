/* *************************************************************************
 *   File: resource_cascade.c                          Part of LuminariMUD *
 *  Usage: Phase 7 ecological resource interdependencies implementation   *
 * Author: Implementation Team                                             *
 ***************************************************************************
 * Phase 7: Ecological Resource Interdependencies                         *
 * This system implements realistic ecological relationships where         *
 * harvesting one resource affects the availability of related resources. *
 ***************************************************************************/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "spells.h"
#include "resource_system.h"
#include "resource_depletion.h"
#include "resource_cascade.h"

/* Global variables */
struct resource_relationship *resource_relationships = NULL;
int num_resource_relationships = 0;
bool cascade_system_enabled = TRUE;

/* Ecosystem state names for display */
const char *ecosystem_state_names[] = {
    "Pristine",
    "Healthy", 
    "Stressed",
    "Degraded",
    "Collapsed",
    "\n"
};

/* Ecosystem state descriptions */
const char *ecosystem_state_descriptions[] = {
    "This area shows no signs of environmental stress. All resources are abundant.",
    "This area maintains good ecological balance with minor harvesting impact.",
    "This area shows signs of environmental stress from resource extraction.",
    "This area has been significantly damaged by over-harvesting activities.",
    "This area's ecosystem has collapsed from excessive resource exploitation.",
    "\n"
};

/* ===== CORE CASCADE FUNCTIONS ===== */

/* Apply cascade effects when resources are harvested */
void apply_cascade_effects(room_rnum room, int source_resource, int quantity)
{
    char query[MAX_STRING_LENGTH];
    MYSQL_RES *result;
    MYSQL_ROW row;
    int target_resource;
    float effect_magnitude, current_depletion, cascade_amount;
    int x, y, zone_vnum;
    
    if (room == NOWHERE || source_resource < 0 || source_resource >= NUM_RESOURCE_TYPES)
        return;
        
    if (!cascade_system_enabled || !mysql_available || !conn)
        return;
    
    /* Get location coordinates */
    x = world[room].coords[0];
    y = world[room].coords[1];
    zone_vnum = zone_table[world[room].zone].number;
    
    /* Query for relationships where this resource is the source */
    snprintf(query, sizeof(query),
        "SELECT target_resource, effect_magnitude FROM resource_relationships "
        "WHERE source_resource = %d AND effect_type = 'depletion'",
        source_resource);
    
    if (mysql_query_safe(conn, query)) {
        log("SYSERR: Error querying resource relationships: %s", mysql_error(conn));
        return;
    }
    
    result = mysql_store_result_safe(conn);
    if (!result) return;
    
    /* Apply each cascade effect */
    while ((row = mysql_fetch_row(result))) {
        target_resource = atoi(row[0]);
        effect_magnitude = atof(row[1]);
        
        /* Skip if invalid target resource */
        if (target_resource < 0 || target_resource >= NUM_RESOURCE_TYPES)
            continue;
            
        /* Calculate cascade amount based on quantity harvested */
        cascade_amount = quantity * fabs(effect_magnitude);
        if (cascade_amount > MAX_CASCADE_EFFECT) 
            cascade_amount = MAX_CASCADE_EFFECT;
        
        /* Get current depletion level for target resource */
        current_depletion = get_resource_depletion_level(room, target_resource);
        
        /* Apply cascade effect */
        if (effect_magnitude < 0.0) {
            /* Negative effect - deplete target resource */
            float new_depletion = current_depletion - cascade_amount;
            if (new_depletion < 0.0) new_depletion = 0.0;
            
            /* Update database with cascade depletion */
            snprintf(query, sizeof(query),
                "INSERT INTO resource_depletion "
                "(zone_vnum, x_coord, y_coord, resource_type, depletion_level) "
                "VALUES (%d, %d, %d, %d, %.3f) "
                "ON DUPLICATE KEY UPDATE "
                "depletion_level = GREATEST(0.0, depletion_level - %.3f), "
                "last_harvest = CURRENT_TIMESTAMP",
                zone_vnum, x, y, target_resource, new_depletion, cascade_amount);
                
        } else {
            /* Positive effect - enhance target resource */
            float new_depletion = current_depletion + cascade_amount;
            if (new_depletion > 1.0) new_depletion = 1.0;
            
            /* Update database with cascade enhancement */
            snprintf(query, sizeof(query),
                "INSERT INTO resource_depletion "
                "(zone_vnum, x_coord, y_coord, resource_type, depletion_level) "
                "VALUES (%d, %d, %d, %d, %.3f) "
                "ON DUPLICATE KEY UPDATE "
                "depletion_level = LEAST(1.0, depletion_level + %.3f), "
                "last_harvest = CURRENT_TIMESTAMP",
                zone_vnum, x, y, target_resource, new_depletion, cascade_amount);
        }
        
        if (mysql_query_safe(conn, query)) {
            log("SYSERR: Error applying cascade effect: %s", mysql_error(conn));
        } else {
            /* Log cascade effect for debugging */
            log_cascade_effect(room, source_resource, target_resource, 
                             effect_magnitude * quantity, NULL);
        }
    }
    
    mysql_free_result(result);
    
    /* Update ecosystem health after cascade effects */
    update_ecosystem_health(room);
}

/* Enhanced depletion with cascade effects */
void apply_harvest_depletion_with_cascades(room_rnum room, int resource_type, int quantity)
{
    /* Apply normal depletion first */
    apply_harvest_depletion(room, resource_type, quantity);
    
    /* Then apply cascade effects */
    apply_cascade_effects(room, resource_type, quantity);
}

/* ===== ECOSYSTEM HEALTH FUNCTIONS ===== */

/* Calculate overall ecosystem health */
int get_ecosystem_state(room_rnum room)
{
    float health_score = calculate_ecosystem_health_score(room);
    
    if (health_score >= ECOSYSTEM_PRISTINE_THRESHOLD)
        return ECOSYSTEM_PRISTINE;
    else if (health_score >= ECOSYSTEM_HEALTHY_THRESHOLD)
        return ECOSYSTEM_HEALTHY;
    else if (health_score >= ECOSYSTEM_STRESSED_THRESHOLD)
        return ECOSYSTEM_STRESSED;
    else if (health_score >= ECOSYSTEM_DEGRADED_THRESHOLD)
        return ECOSYSTEM_DEGRADED;
    else
        return ECOSYSTEM_COLLAPSED;
}

/* Calculate ecosystem health score (0.0-1.0) */
float calculate_ecosystem_health_score(room_rnum room)
{
    float total_score = 0.0;
    int resource_count = 0;
    int i;
    
    if (room == NOWHERE)
        return 0.0;
    
    /* Calculate average resource depletion level */
    for (i = 0; i < NUM_RESOURCE_TYPES; i++) {
        float depletion_level = get_resource_depletion_level(room, i);
        total_score += depletion_level;
        resource_count++;
    }
    
    if (resource_count == 0)
        return 1.0; /* Default to healthy if no data */
    
    return total_score / resource_count;
}

/* Update ecosystem health in database */
void update_ecosystem_health(room_rnum room)
{
    char query[MAX_STRING_LENGTH];
    int x, y, zone_vnum;
    int health_state;
    float health_score;
    
    if (room == NOWHERE || !mysql_available || !conn)
        return;
    
    /* Get location coordinates */
    x = world[room].coords[0];
    y = world[room].coords[1];
    zone_vnum = zone_table[world[room].zone].number;
    
    /* Calculate current ecosystem health */
    health_score = calculate_ecosystem_health_score(room);
    health_state = get_ecosystem_state(room);
    
    /* Update or insert ecosystem health record */
    snprintf(query, sizeof(query),
        "INSERT INTO ecosystem_health "
        "(zone_vnum, x_coord, y_coord, health_state, health_score) "
        "VALUES (%d, %d, %d, '%s', %.3f) "
        "ON DUPLICATE KEY UPDATE "
        "health_state = '%s', health_score = %.3f, last_updated = CURRENT_TIMESTAMP",
        zone_vnum, x, y, ecosystem_state_names[health_state], health_score,
        ecosystem_state_names[health_state], health_score);
    
    if (mysql_query_safe(conn, query)) {
        log("SYSERR: Error updating ecosystem health: %s", mysql_error(conn));
    }
}

/* ===== ENHANCED SURVEY FUNCTIONS ===== */

/* Show ecosystem health and relationships */
void show_ecosystem_analysis(struct char_data *ch, room_rnum room)
{
    int ecosystem_state;
    float health_score;
    int x, y;
    
    if (!ch || room == NOWHERE)
        return;
    
    x = world[room].coords[0];
    y = world[room].coords[1];
    
    ecosystem_state = get_ecosystem_state(room);
    health_score = calculate_ecosystem_health_score(room);
    
    send_to_char(ch, "\tcEcosystem Health Analysis for (%d, %d):\tn\r\n", x, y);
    send_to_char(ch, "=====================================\r\n");
    send_to_char(ch, "Overall Health: %s%s\tn (%.1f%%)\r\n",
                 ecosystem_state <= ECOSYSTEM_HEALTHY ? "\tG" : 
                 ecosystem_state == ECOSYSTEM_STRESSED ? "\tY" : "\tR",
                 ecosystem_state_names[ecosystem_state], health_score * 100);
    send_to_char(ch, "\r\n%s\r\n", ecosystem_state_descriptions[ecosystem_state]);
    
    /* Show critical resources */
    send_to_char(ch, "\r\n\tcResource Status:\tn\r\n");
    int i;
    for (i = 0; i < NUM_RESOURCE_TYPES; i++) {
        float level = get_resource_depletion_level(room, i);
        const char *color = level > 0.6 ? "\tG" : level > 0.3 ? "\tY" : "\tR";
        const char *status = level > 0.8 ? "abundant" : 
                           level > 0.6 ? "healthy" : 
                           level > 0.3 ? "stressed" : 
                           level > 0.1 ? "depleted" : "critical";
        
        send_to_char(ch, "  %s%-12s\tn: %s%s\tn (%.0f%%)\r\n",
                     color, resource_names[i], color, status, level * 100);
    }
    
    /* Show conservation recommendations */
    if (ecosystem_state >= ECOSYSTEM_STRESSED) {
        send_to_char(ch, "\r\n\trRecommended Actions:\tn\r\n");
        send_to_char(ch, "• Reduce harvesting frequency in this area\r\n");
        send_to_char(ch, "• Focus on less depleted resource types\r\n");
        send_to_char(ch, "• Allow 1-2 days for ecosystem recovery\r\n");
    }
}

/* Show cascade effects of harvesting specific resource */
void show_cascade_preview(struct char_data *ch, room_rnum room, int resource_type)
{
    char query[MAX_STRING_LENGTH];
    MYSQL_RES *result;
    MYSQL_ROW row;
    
    if (!ch || room == NOWHERE || resource_type < 0 || resource_type >= NUM_RESOURCE_TYPES)
        return;
    
    if (!mysql_available || !conn) {
        send_to_char(ch, "Cascade analysis not available (database offline).\r\n");
        return;
    }
    
    send_to_char(ch, "\tcCascade Effect Analysis for %s:\tn\r\n", resource_names[resource_type]);
    send_to_char(ch, "========================================\r\n");
    
    /* Query for relationships where this resource is the source */
    snprintf(query, sizeof(query),
        "SELECT target_resource, effect_magnitude, description FROM resource_relationships "
        "WHERE source_resource = %d ORDER BY ABS(effect_magnitude) DESC",
        resource_type);
    
    if (mysql_query_safe(conn, query)) {
        send_to_char(ch, "Error querying cascade effects.\r\n");
        return;
    }
    
    result = mysql_store_result_safe(conn);
    if (!result) {
        send_to_char(ch, "No cascade effects found for this resource.\r\n");
        return;
    }
    
    bool found_effects = FALSE;
    while ((row = mysql_fetch_row(result))) {
        int target_resource = atoi(row[0]);
        float effect_magnitude = atof(row[1]);
        const char *description = row[2];
        
        if (target_resource >= 0 && target_resource < NUM_RESOURCE_TYPES) {
            const char *effect_color = effect_magnitude < 0 ? "\tr" : "\tg";
            const char *effect_type = effect_magnitude < 0 ? "reduces" : "enhances";
            
            send_to_char(ch, "%sHarvesting %s %s %s\tn (%.1f%% effect)\r\n",
                         effect_color, resource_names[resource_type], effect_type,
                         resource_names[target_resource], fabs(effect_magnitude) * 100);
            send_to_char(ch, "  \tc%s\tn\r\n", description);
            found_effects = TRUE;
        }
    }
    
    if (!found_effects) {
        send_to_char(ch, "This resource has no significant ecological interactions.\r\n");
    }
    
    mysql_free_result(result);
}

/* ===== UTILITY FUNCTIONS ===== */

/* Get ecosystem state name */
const char *get_ecosystem_state_name(int state)
{
    if (state < 0 || state >= NUM_ECOSYSTEM_STATES)
        return "Unknown";
    return ecosystem_state_names[state];
}

/* Get ecosystem state description */
const char *get_ecosystem_state_description(int state)
{
    if (state < 0 || state >= NUM_ECOSYSTEM_STATES)
        return "Ecosystem state unknown.";
    return ecosystem_state_descriptions[state];
}

/* Log cascade effect for debugging */
void log_cascade_effect(room_rnum room, int source_resource, int target_resource, 
                       float effect_magnitude, struct char_data *ch)
{
    char query[MAX_STRING_LENGTH];
    int x, y, zone_vnum;
    long player_id = 0;
    
    if (room == NOWHERE || !mysql_available || !conn)
        return;
    
    /* Get location coordinates */
    x = world[room].coords[0];
    y = world[room].coords[1];
    zone_vnum = zone_table[world[room].zone].number;
    
    if (ch && !IS_NPC(ch))
        player_id = GET_IDNUM(ch);
    
    /* Log the cascade effect */
    snprintf(query, sizeof(query),
        "INSERT INTO cascade_effects_log "
        "(zone_vnum, x_coord, y_coord, source_resource, target_resource, effect_magnitude, player_id) "
        "VALUES (%d, %d, %d, %d, %d, %.3f, %ld)",
        zone_vnum, x, y, source_resource, target_resource, effect_magnitude, player_id);
    
    if (mysql_query_safe(conn, query)) {
        log("SYSERR: Error logging cascade effect: %s", mysql_error(conn));
    }
}

/* Check if ecosystem is in critical state */
bool is_ecosystem_critical(room_rnum room)
{
    int ecosystem_state = get_ecosystem_state(room);
    return (ecosystem_state >= ECOSYSTEM_DEGRADED);
}
