/**************************************************************************
 *  File: resource_regeneration.c                      Part of LuminariMUD *
 *  Usage: Resource regeneration and recovery system                       *
 *  Author: Phase 6 Implementation - Step 4: Regeneration Events          *
 ***************************************************************************
 * This file handles automatic resource regeneration over time, including *
 * seasonal variations, weather effects, and event-based scheduling.      *
 **************************************************************************/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "resource_system.h"
#include "resource_depletion.h"
#include "resource_regeneration.h"
#include "mysql.h"
#include "mud_event.h"
#include "weather.h"

/* ===== REGENERATION CONSTANTS ===== */

/* Base regeneration rates per game hour (percentage of full recovery) */
#define BASE_REGEN_VEGETATION   0.08   /* 8% per hour - fast growing */
#define BASE_REGEN_HERBS        0.06   /* 6% per hour - moderate */
#define BASE_REGEN_WATER        0.10   /* 10% per hour - seasonal/weather dependent */
#define BASE_REGEN_GAME         0.04   /* 4% per hour - animal migration */
#define BASE_REGEN_WOOD         0.02   /* 2% per hour - slow tree growth */
#define BASE_REGEN_CLAY         0.05   /* 5% per hour - weather erosion */
#define BASE_REGEN_STONE        0.01   /* 1% per hour - geological */
#define BASE_REGEN_MINERALS     0.005  /* 0.5% per hour - very slow */
#define BASE_REGEN_CRYSTAL      0.002  /* 0.2% per hour - magical regeneration */
#define BASE_REGEN_SALT         0.015  /* 1.5% per hour - evaporation dependent */

/* Regeneration event frequency (in real seconds) */
#define REGENERATION_EVENT_INTERVAL 300  /* 5 minutes */

/* Global regeneration control */
static bool regeneration_enabled = TRUE;
static int regeneration_event_id = -1;

/* ===== HELPER FUNCTIONS ===== */

/* Get base regeneration rate for a resource type */
float get_base_regeneration_rate(int resource_type)
{
    switch (resource_type) {
        case RESOURCE_VEGETATION:  return BASE_REGEN_VEGETATION;
        case RESOURCE_HERBS:       return BASE_REGEN_HERBS;
        case RESOURCE_WATER:       return BASE_REGEN_WATER;
        case RESOURCE_GAME:        return BASE_REGEN_GAME;
        case RESOURCE_WOOD:        return BASE_REGEN_WOOD;
        case RESOURCE_CLAY:        return BASE_REGEN_CLAY;
        case RESOURCE_STONE:       return BASE_REGEN_STONE;
        case RESOURCE_MINERALS:    return BASE_REGEN_MINERALS;
        case RESOURCE_CRYSTAL:     return BASE_REGEN_CRYSTAL;
        case RESOURCE_SALT:        return BASE_REGEN_SALT;
        default:                   return 0.03; /* Default 3% per hour */
    }
}

/* Get seasonal modifier (0.5 = 50% rate, 2.0 = 200% rate) */
float get_seasonal_modifier(int resource_type)
{
    /* For now, simplified seasonal effects */
    switch (resource_type) {
        case RESOURCE_VEGETATION:
        case RESOURCE_HERBS:
            /* Plants grow faster in spring/summer */
            return 1.5; /* TODO: Add actual season detection */
            
        case RESOURCE_WATER:
            /* Water sources affected by weather */
            return 1.2; /* TODO: Add weather integration */
            
        case RESOURCE_GAME:
            /* Animals migrate seasonally */
            return 1.0; /* TODO: Add migration patterns */
            
        default:
            return 1.0; /* No seasonal effect */
    }
}

/* Get weather modifier for regeneration */
float get_weather_modifier(int resource_type, int zone_vnum)
{
    /* TODO: Integrate with actual weather system */
    /* For now, return neutral modifier */
    return 1.0;
}

/* Get terrain modifier based on wilderness terrain type */
float get_terrain_modifier(int resource_type, int x, int y)
{
    /* TODO: Integrate with wilderness terrain system */
    /* Different terrain types affect regeneration differently */
    return 1.0;
}

/* ===== CORE REGENERATION FUNCTIONS ===== */

/* Calculate final regeneration rate for a resource at a location */
float calculate_regeneration_rate(int resource_type, int zone_vnum, int x, int y)
{
    float base_rate = get_base_regeneration_rate(resource_type);
    float seasonal_mod = get_seasonal_modifier(resource_type);
    float weather_mod = get_weather_modifier(resource_type, zone_vnum);
    float terrain_mod = get_terrain_modifier(resource_type, x, y);
    
    return base_rate * seasonal_mod * weather_mod * terrain_mod;
}

/* Apply regeneration to a single resource location */
bool regenerate_resource_location(int zone_vnum, int x, int y, int resource_type)
{
    char query[MAX_STRING_LENGTH];
    MYSQL_RES *result;
    MYSQL_ROW row;
    float current_depletion, new_depletion, regen_rate, regen_amount;
    
    if (!mysql_available || !conn) {
        return FALSE;
    }
    
    /* Get current depletion level */
    snprintf(query, sizeof(query),
        "SELECT depletion_level FROM resource_depletion "
        "WHERE zone_vnum = %d AND x_coord = %d AND y_coord = %d AND resource_type = %d",
        zone_vnum, x, y, resource_type);
    
    if (mysql_query_safe(conn, query)) {
        log("SYSERR: Error querying depletion for regeneration: %s", mysql_error(conn));
        return FALSE;
    }
    
    result = mysql_store_result_safe(conn);
    if (!result) {
        return FALSE; /* No depletion record = no regeneration needed */
    }
    
    row = mysql_fetch_row(result);
    if (!row || !row[0]) {
        mysql_free_result(result);
        return FALSE;
    }
    
    current_depletion = atof(row[0]);
    mysql_free_result(result);
    
    /* Skip if already fully regenerated */
    if (current_depletion >= 1.0) {
        return FALSE;
    }
    
    /* Calculate regeneration amount */
    regen_rate = calculate_regeneration_rate(resource_type, zone_vnum, x, y);
    regen_amount = regen_rate;
    new_depletion = current_depletion + regen_amount;
    
    /* Cap at full regeneration */
    if (new_depletion > 1.0) {
        new_depletion = 1.0;
    }
    
    /* Update database */
    snprintf(query, sizeof(query),
        "UPDATE resource_depletion SET depletion_level = %.3f "
        "WHERE zone_vnum = %d AND x_coord = %d AND y_coord = %d AND resource_type = %d",
        new_depletion, zone_vnum, x, y, resource_type);
    
    if (mysql_query_safe(conn, query)) {
        log("SYSERR: Error updating regeneration: %s", mysql_error(conn));
        return FALSE;
    }
    
    /* Log significant regeneration events */
    if (regen_amount > 0.05) { /* Log if more than 5% regeneration */
        log("REGEN: Resource %d at (%d,%d) zone %d regenerated %.1f%% (%.3f -> %.3f)",
            resource_type, x, y, zone_vnum, regen_amount * 100, current_depletion, new_depletion);
    }
    
    return TRUE;
}

/* Process regeneration for all depleted resources */
void process_global_regeneration(void)
{
    char query[MAX_STRING_LENGTH];
    MYSQL_RES *result;
    MYSQL_ROW row;
    int regenerated_locations = 0;
    
    if (!mysql_available || !conn || !regeneration_enabled) {
        return;
    }
    
    /* Get all depleted resource locations */
    snprintf(query, sizeof(query),
        "SELECT zone_vnum, x_coord, y_coord, resource_type, depletion_level "
        "FROM resource_depletion "
        "WHERE depletion_level < 1.0 "
        "ORDER BY last_harvest ASC "
        "LIMIT 100"); /* Process up to 100 locations per cycle */
    
    if (mysql_query_safe(conn, query)) {
        log("SYSERR: Error querying for regeneration: %s", mysql_error(conn));
        return;
    }
    
    result = mysql_store_result_safe(conn);
    if (!result) {
        return;
    }
    
    /* Process each depleted location */
    while ((row = mysql_fetch_row(result))) {
        int zone_vnum = atoi(row[0]);
        int x = atoi(row[1]);
        int y = atoi(row[2]);
        int resource_type = atoi(row[3]);
        
        if (regenerate_resource_location(zone_vnum, x, y, resource_type)) {
            regenerated_locations++;
        }
    }
    
    mysql_free_result(result);
    
    if (regenerated_locations > 0) {
        log("REGEN: Processed regeneration for %d resource locations", regenerated_locations);
    }
}

/* ===== EVENT SYSTEM INTEGRATION ===== */

/* Regeneration event callback */
EVENTFUNC(regeneration_event)
{
    struct mud_event_data *pMudEvent = NULL;
    
    /* Process regeneration */
    process_global_regeneration();
    
    /* Reschedule the event if regeneration is enabled */
    if (regeneration_enabled) {
        pMudEvent = new_mud_event(eREGENERATION, NULL, REGENERATION_EVENT_INTERVAL);
        add_mud_event(pMudEvent);
        regeneration_event_id = pMudEvent->iId;
    } else {
        regeneration_event_id = -1;
    }
    
    return 0;
}

/* ===== INITIALIZATION AND CONTROL ===== */

/* Initialize the regeneration system */
void init_resource_regeneration(void)
{
    struct mud_event_data *pMudEvent = NULL;
    
    log("Initializing resource regeneration system...");
    
    regeneration_enabled = TRUE;
    
    /* Start the regeneration event cycle */
    pMudEvent = new_mud_event(eREGENERATION, NULL, REGENERATION_EVENT_INTERVAL);
    add_mud_event(pMudEvent);
    regeneration_event_id = pMudEvent->iId;
    
    log("Resource regeneration system initialized - processing every %d seconds", 
        REGENERATION_EVENT_INTERVAL);
}

/* Enable/disable regeneration */
void set_regeneration_enabled(bool enabled)
{
    regeneration_enabled = enabled;
    
    if (enabled && regeneration_event_id == -1) {
        /* Restart regeneration events */
        init_resource_regeneration();
        log("Resource regeneration system enabled");
    } else if (!enabled && regeneration_event_id != -1) {
        /* Stop regeneration events */
        log("Resource regeneration system disabled");
    }
}

/* Check if regeneration is enabled */
bool is_regeneration_enabled(void)
{
    return regeneration_enabled;
}

/* Get regeneration statistics */
void get_regeneration_stats(int *total_depleted, int *regenerating_locations)
{
    char query[MAX_STRING_LENGTH];
    MYSQL_RES *result;
    MYSQL_ROW row;
    
    *total_depleted = 0;
    *regenerating_locations = 0;
    
    if (!mysql_available || !conn) {
        return;
    }
    
    /* Count total depleted locations */
    snprintf(query, sizeof(query),
        "SELECT COUNT(*) FROM resource_depletion WHERE depletion_level < 1.0");
    
    if (mysql_query_safe(conn, query) == 0) {
        result = mysql_store_result_safe(conn);
        if (result && (row = mysql_fetch_row(result))) {
            *total_depleted = atoi(row[0]);
        }
        if (result) mysql_free_result(result);
    }
    
    /* Count locations with significant depletion (< 0.8) */
    snprintf(query, sizeof(query),
        "SELECT COUNT(*) FROM resource_depletion WHERE depletion_level < 0.8");
    
    if (mysql_query_safe(conn, query) == 0) {
        result = mysql_store_result_safe(conn);
        if (result && (row = mysql_fetch_row(result))) {
            *regenerating_locations = atoi(row[0]);
        }
        if (result) mysql_free_result(result);
    }
}
