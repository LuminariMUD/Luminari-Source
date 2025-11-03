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

/* Global regeneration control */
static bool regeneration_logging_enabled = TRUE;

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

/* Log regeneration event to database */
void log_regeneration_event(int zone_vnum, int x, int y, int resource_type, 
                           float old_level, float new_level, float regen_amount, 
                           const char *regen_type)
{
    char query[MAX_STRING_LENGTH];
    
    if (!regeneration_logging_enabled || !mysql_available || !conn) {
        return;
    }
    
    /* Only log meaningful regeneration events */
    if (regen_amount < 0.001) {
        return;
    }
    
    snprintf(query, sizeof(query),
        "INSERT INTO resource_regeneration_log "
        "(zone_vnum, x_coord, y_coord, resource_type, old_depletion_level, "
        "new_depletion_level, regeneration_amount, regeneration_type) VALUES "
        "(%d, %d, %d, %d, %.3f, %.3f, %.3f, '%s')",
        zone_vnum, x, y, resource_type, old_level, new_level, regen_amount, regen_type);
    
    if (mysql_query_safe(conn, query)) {
        log("SYSERR: Error logging regeneration: %s", mysql_error(conn));
        /* Don't fail the regeneration if logging fails */
    }
}

/* ===== LOGGING CONTROL FUNCTIONS ===== */

/* Enable/disable regeneration logging */
void set_regeneration_logging_enabled(bool enabled)
{
    regeneration_logging_enabled = enabled;
    log("Regeneration logging %s", enabled ? "enabled" : "disabled");
}

/* Check if regeneration logging is enabled */
bool is_regeneration_logging_enabled(void)
{
    return regeneration_logging_enabled;
}

/* Get recent regeneration events for a location */
void show_regeneration_history(struct char_data *ch, int zone_vnum, int x, int y, int limit)
{
    char query[MAX_STRING_LENGTH];
    MYSQL_RES *result;
    MYSQL_ROW row;
    
    if (!mysql_available || !conn) {
        send_to_char(ch, "Database not available.\r\n");
        return;
    }
    
    snprintf(query, sizeof(query),
        "SELECT resource_type, old_depletion_level, new_depletion_level, "
        "regeneration_amount, regeneration_type, regeneration_time "
        "FROM resource_regeneration_log "
        "WHERE zone_vnum = %d AND x_coord = %d AND y_coord = %d "
        "ORDER BY regeneration_time DESC LIMIT %d",
        zone_vnum, x, y, limit);
    
    if (mysql_query_safe(conn, query)) {
        send_to_char(ch, "Error querying regeneration history: %s\r\n", mysql_error(conn));
        return;
    }
    
    result = mysql_store_result_safe(conn);
    if (!result) {
        send_to_char(ch, "No regeneration history found.\r\n");
        return;
    }
    
    send_to_char(ch, "Recent regeneration events at (%d,%d) zone %d:\r\n", x, y, zone_vnum);
    send_to_char(ch, "%-12s %-8s %-8s %-10s %-10s %s\r\n", 
                 "Resource", "Old", "New", "Amount", "Type", "Time");
    send_to_char(ch, "%-12s %-8s %-8s %-10s %-10s %s\r\n", 
                 "--------", "---", "---", "------", "----", "----");
    
    while ((row = mysql_fetch_row(result))) {
        int resource_type = atoi(row[0]);
        float old_level = atof(row[1]);
        float new_level = atof(row[2]);
        float regen_amount = atof(row[3]);
        
        send_to_char(ch, "%-12d %-8.3f %-8.3f %-10.3f %-10s %s\r\n",
                     resource_type, old_level, new_level, regen_amount, 
                     row[4], row[5]);
    }
    
    mysql_free_result(result);
}
