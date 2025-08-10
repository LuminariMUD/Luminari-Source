/* *************************************************************************
 *   File: resource_depletion.c                        Part of LuminariMUD *
 *  Usage: Simple resource depletion system implementation                 *
 * Author: Phase 6 Implementation - Step 1: Basic Depletion               *
 ***************************************************************************
 * Simple implementation for basic resource depletion functionality       *
 ***************************************************************************/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "interpreter.h"
#include "db.h"
#include "comm.h"
#include "wilderness.h"
#include "resource_system.h"
#include "resource_depletion.h"

/* ===== BASIC DEPLETION FUNCTIONS ===== */

/* Get the current depletion level for a resource at a location (0.0-1.0) */
float get_resource_depletion_level(room_rnum room, int resource_type)
{
    /* For now, return a simple calculation based on room number
     * In Phase 6 full implementation, this would query the database */
    
    if (room == NOWHERE || resource_type < 0)
        return 1.0; /* Fully available */
    
    /* Simple mock depletion based on room number for testing
     * Higher room numbers have slightly more depletion */
    float base_depletion = (room % 100) / 1000.0; /* 0.0-0.099 */
    
    /* Clamp between 0.0 and 1.0 */
    if (base_depletion < 0.0) base_depletion = 0.0;
    if (base_depletion > 1.0) base_depletion = 1.0;
    
    return 1.0 - base_depletion; /* Return availability (1.0 = fully available) */
}

/* Apply depletion when resources are harvested */
void apply_harvest_depletion(room_rnum room, int resource_type, int quantity)
{
    /* For now, this is a stub function
     * In Phase 6 full implementation, this would update the database */
    
    if (room == NOWHERE || resource_type < 0 || quantity <= 0)
        return;
    
    /* TODO: In full implementation, update resource depletion in database
     * based on quantity harvested and resource regeneration rates */
}

/* Check if harvest should fail due to resource depletion */
bool should_harvest_fail_due_to_depletion(room_rnum room, int resource_type)
{
    float resource_level = get_resource_depletion_level(room, resource_type);
    
    /* If resource level is below 10%, chance of failure */
    if (resource_level < 0.1) {
        /* 50% chance of failure when severely depleted */
        return (rand() % 100) < 50;
    }
    
    return FALSE; /* No failure due to depletion */
}

/* Get harvest success modifier based on resource availability */
float get_harvest_success_modifier(room_rnum room, int resource_type)
{
    float resource_level = get_resource_depletion_level(room, resource_type);
    
    /* Full resources = 1.0 modifier, depleted resources = 0.5 modifier */
    float modifier = 0.5 + (resource_level * 0.5);
    
    /* Ensure reasonable bounds */
    if (modifier < 0.1) modifier = 0.1;
    if (modifier > 1.0) modifier = 1.0;
    
    return modifier;
}

/* Get a descriptive name for the depletion level */
const char *get_depletion_level_name(float resource_level)
{
    if (resource_level >= 0.9)
        return "abundant";
    else if (resource_level >= 0.7)
        return "plentiful";
    else if (resource_level >= 0.5)
        return "moderate";
    else if (resource_level >= 0.3)
        return "sparse";
    else if (resource_level >= 0.1)
        return "scarce";
    else
        return "depleted";
}

/* ===== CONSERVATION FUNCTIONS ===== */

/* Update a player's conservation score */
void update_conservation_score(struct char_data *ch, int resource_type, bool sustainable)
{
    /* For now, this is a stub function
     * In Phase 6 full implementation, this would update player conservation
     * statistics in the database */
    
    if (!ch || IS_NPC(ch))
        return;
    
    /* TODO: In full implementation, track player conservation behavior
     * and update database records */
}

/* Get a player's conservation score for a resource type */
float get_player_conservation_score(struct char_data *ch, int resource_type)
{
    /* For now, return a neutral score
     * In Phase 6 full implementation, this would query the database */
    
    if (!ch || IS_NPC(ch))
        return 0.5; /* Neutral score */
    
    /* TODO: In full implementation, return actual conservation score
     * from database based on player's harvesting history */
    return 0.5; /* Neutral score for now */
}

/* Get a descriptive name for conservation status */
const char *get_conservation_status_name(float score)
{
    if (score >= 0.8)
        return "excellent conservationist";
    else if (score >= 0.6)
        return "good conservationist";
    else if (score >= 0.4)
        return "average stewardship";
    else if (score >= 0.2)
        return "poor stewardship";
    else
        return "resource exploiter";
}

/* Show resource conservation status at a location */
void show_resource_conservation_status(struct char_data *ch, int x, int y)
{
    room_rnum room;
    
    if (!ch)
        return;
    
    room = find_room_by_coordinates(x, y);
    if (room == NOWHERE) {
        send_to_char(ch, "No detailed conservation data available for this location.\r\n");
        return;
    }
    
    send_to_char(ch, "\tWResource Conservation Status:\tn\r\n");
    send_to_char(ch, "\tc========================\tn\r\n");
    
    /* Show basic conservation info for now */
    float herb_level = get_resource_depletion_level(room, RESOURCE_HERBS);
    float ore_level = get_resource_depletion_level(room, RESOURCE_MINERALS);
    float wood_level = get_resource_depletion_level(room, RESOURCE_WOOD);
    
    send_to_char(ch, "\tGHerbs:\tn %s (%.0f%% available)\r\n", 
                 get_depletion_level_name(herb_level), herb_level * 100);
    
    send_to_char(ch, "\tYMinerals:\tn %s (%.0f%% available)\r\n", 
                 get_depletion_level_name(ore_level), ore_level * 100);
    
    send_to_char(ch, "\tgWood:\tn %s (%.0f%% available)\r\n", 
                 get_depletion_level_name(wood_level), wood_level * 100);
}

/* Show regeneration analysis for a location */
void show_regeneration_analysis(struct char_data *ch, int x, int y)
{
    if (!ch)
        return;
    
    send_to_char(ch, "\tWResource Regeneration Analysis:\tn\r\n");
    send_to_char(ch, "\tc===============================\tn\r\n");
    
    /* For now, show placeholder information */
    send_to_char(ch, "\tGNatural Regeneration:\tn Resources in this area regenerate slowly over time.\r\n");
    send_to_char(ch, "\tYSeasonal Factors:\tn Current season affects regeneration rates.\r\n");
    send_to_char(ch, "\tCEnvironmental Health:\tn Ecosystem health influences resource recovery.\r\n");
    send_to_char(ch, "\r\n\twNote: Detailed regeneration tracking will be available in the full Phase 6 implementation.\tn\r\n");
}

/* ===== ADMIN/DEBUG FUNCTIONS ===== */

/* Show overall depletion statistics (admin command) */
void show_depletion_stats(struct char_data *ch)
{
    if (!ch)
        return;
    
    send_to_char(ch, "\tWGlobal Resource Depletion Statistics:\tn\r\n");
    send_to_char(ch, "\tc=====================================\tn\r\n");
    
    /* For now, show basic placeholder statistics */
    send_to_char(ch, "\tGAverage Resource Availability:\tn\r\n");
    send_to_char(ch, "  Herbs: 85%% (Good)\r\n");
    send_to_char(ch, "  Ores:  78%% (Good)\r\n");
    send_to_char(ch, "  Wood:  92%% (Excellent)\r\n");
    send_to_char(ch, "\r\n\twNote: These are mock statistics for Phase 6 basic implementation.\tn\r\n");
}
