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
#include "mysql.h"

/* ===== DATABASE INITIALIZATION ===== */

/* Initialize resource depletion database tables */
void init_resource_depletion_database(void)
{
    if (!mysql_available || !conn) {
        log("MySQL not available, skipping resource depletion database initialization");
        return;
    }

    log("Initializing resource depletion database tables...");

    // Create resource_depletion table
    const char *create_depletion_table = 
        "CREATE TABLE IF NOT EXISTS resource_depletion ("
        "id INT AUTO_INCREMENT PRIMARY KEY, "
        "zone_vnum INT NOT NULL, "
        "x_coord INT NOT NULL, "
        "y_coord INT NOT NULL, "
        "resource_type INT NOT NULL, "
        "depletion_level FLOAT DEFAULT 1.0, "
        "total_harvested INT DEFAULT 0, "
        "last_harvest TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP, "
        "INDEX idx_location (zone_vnum, x_coord, y_coord), "
        "INDEX idx_resource (resource_type), "
        "UNIQUE KEY unique_location_resource (zone_vnum, x_coord, y_coord, resource_type)"
        ")";

    if (mysql_query_safe(conn, create_depletion_table)) {
        log("SYSERR: Failed to create resource_depletion table: %s", mysql_error(conn));
        return;
    }

    // Create player_conservation table
    const char *create_conservation_table = 
        "CREATE TABLE IF NOT EXISTS player_conservation ("
        "id INT AUTO_INCREMENT PRIMARY KEY, "
        "player_id BIGINT NOT NULL, "
        "conservation_score FLOAT DEFAULT 0.5, "
        "total_harvests INT DEFAULT 0, "
        "sustainable_harvests INT DEFAULT 0, "
        "unsustainable_harvests INT DEFAULT 0, "
        "last_updated TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP, "
        "UNIQUE KEY unique_player (player_id)"
        ")";

    if (mysql_query_safe(conn, create_conservation_table)) {
        log("SYSERR: Failed to create player_conservation table: %s", mysql_error(conn));
        return;
    }

    // Create resource_statistics table  
    const char *create_statistics_table =
        "CREATE TABLE IF NOT EXISTS resource_statistics ("
        "id INT AUTO_INCREMENT PRIMARY KEY, "
        "resource_type INT NOT NULL, "
        "total_harvested_global BIGINT DEFAULT 0, "
        "total_depleted_locations INT DEFAULT 0, "
        "average_depletion_level FLOAT DEFAULT 1.0, "
        "peak_depletion_level FLOAT DEFAULT 1.0, "
        "last_updated TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP, "
        "UNIQUE KEY unique_resource (resource_type)"
        ")";

    if (mysql_query_safe(conn, create_statistics_table)) {
        log("SYSERR: Failed to create resource_statistics table: %s", mysql_error(conn));
        return;
    }

    // Initialize default resource statistics
    const char *init_statistics = 
        "INSERT IGNORE INTO resource_statistics "
        "(resource_type, average_depletion_level, peak_depletion_level) VALUES "
        "(0, 1.0, 1.0), (1, 1.0, 1.0), (2, 1.0, 1.0), (3, 1.0, 1.0), (4, 1.0, 1.0), "
        "(5, 1.0, 1.0), (6, 1.0, 1.0), (7, 1.0, 1.0), (8, 1.0, 1.0), (9, 1.0, 1.0)";

    if (mysql_query_safe(conn, init_statistics)) {
        log("SYSERR: Failed to initialize resource statistics: %s", mysql_error(conn));
        return;
    }

    log("Resource depletion database tables initialized successfully");
}

/* ===== REGENERATION FUNCTIONS ===== */

/* Get resource-specific regeneration rate per hour */
float get_resource_regeneration_rate(int resource_type)
{
    float base_rate;
    
    /* Base regeneration rates per hour */
    switch (resource_type) {
        case RESOURCE_VEGETATION:  base_rate = 0.12;  break; /* Fast growing - 12% per hour */
        case RESOURCE_HERBS:       base_rate = 0.08;  break; /* Moderate regeneration - 8% per hour */
        case RESOURCE_WATER:       base_rate = 0.20;  break; /* Seasonal/weather dependent - 20% per hour when conditions right */
        case RESOURCE_GAME:        base_rate = 0.06;  break; /* Animals move in - 6% per hour */
        case RESOURCE_WOOD:        base_rate = 0.02;  break; /* Very slow regeneration - 2% per hour */
        case RESOURCE_CLAY:        base_rate = 0.10;  break; /* Weather dependent - 10% per hour */
        case RESOURCE_STONE:       base_rate = 0.005; break; /* Extremely slow - 0.5% per hour */
        case RESOURCE_MINERALS:    base_rate = 0.001; break; /* Nearly non-renewable - 0.1% per hour */
        case RESOURCE_CRYSTAL:     base_rate = 0.0005; break; /* Magical regeneration - 0.05% per hour */
        case RESOURCE_SALT:        base_rate = 0.01;  break; /* Slow but steady - 1% per hour */
        default:                   base_rate = 0.05;  break; /* Default moderate regeneration */
    }
    
    return base_rate;
}

/* Get resource regeneration rate with seasonal and weather modifiers applied */
float get_modified_regeneration_rate(int resource_type, int x, int y)
{
    float base_rate = get_resource_regeneration_rate(resource_type);
    float seasonal_modifier = get_seasonal_modifier(resource_type);
    float weather_modifier = get_weather_modifier(resource_type, get_weather(x, y));
    
    return base_rate * seasonal_modifier * weather_modifier;
}

/* Calculate regeneration based on time passed since last harvest with seasonal/weather modifiers */
float calculate_regeneration_amount(int resource_type, time_t last_harvest_time, int x, int y)
{
    time_t current_time = time(NULL);
    double hours_passed = difftime(current_time, last_harvest_time) / 3600.0; /* Convert seconds to hours */
    
    if (hours_passed <= 0) return 0.0;
    
    float regen_rate = get_modified_regeneration_rate(resource_type, x, y);
    float regeneration = hours_passed * regen_rate;
    
    /* Cap regeneration to prevent overflow */
    if (regeneration > 1.0) regeneration = 1.0;
    
    return regeneration;
}

/* Apply lazy regeneration - called when resource level is checked */
void apply_lazy_regeneration(room_rnum room, int resource_type)
{
    char query[MAX_STRING_LENGTH];
    char update_query[MAX_STRING_LENGTH];
    MYSQL_RES *result;
    MYSQL_ROW row;
    int x, y, zone_vnum;
    float current_depletion = 1.0;
    time_t last_harvest_time = 0;
    
    if (room == NOWHERE || resource_type < 0)
        return;
        
    /* If MySQL not available, skip */
    if (!mysql_available || !conn) {
        return;
    }
    
    /* Get coordinates and zone */
    x = world[room].coords[0];
    y = world[room].coords[1];
    zone_vnum = zone_table[world[room].zone].number;
    
    /* Check if this location has depletion data */
    snprintf(query, sizeof(query),
        "SELECT depletion_level, UNIX_TIMESTAMP(last_harvest) FROM resource_depletion "
        "WHERE zone_vnum = %d AND x_coord = %d AND y_coord = %d AND resource_type = %d",
        zone_vnum, x, y, resource_type);
    
    if (mysql_query_safe(conn, query)) {
        return; /* Fail silently on error */
    }
    
    result = mysql_store_result_safe(conn);
    if (!result) {
        return; /* No data exists, nothing to regenerate */
    }
    
    if ((row = mysql_fetch_row(result))) {
        current_depletion = atof(row[0]);
        last_harvest_time = (time_t)atol(row[1]);
        
        /* Calculate regeneration with seasonal and weather modifiers */
        float regeneration = calculate_regeneration_amount(resource_type, last_harvest_time, x, y);
        
        if (regeneration > 0.0) {
            float new_depletion = current_depletion + regeneration;
            if (new_depletion > 1.0) new_depletion = 1.0; /* Cap at fully available */
            
            /* Update the database with regenerated amount */
            snprintf(update_query, sizeof(update_query),
                "UPDATE resource_depletion SET depletion_level = %.3f, last_harvest = CURRENT_TIMESTAMP "
                "WHERE zone_vnum = %d AND x_coord = %d AND y_coord = %d AND resource_type = %d",
                new_depletion, zone_vnum, x, y, resource_type);
            
            if (mysql_query_safe(conn, update_query)) {
                log("SYSERR: Error updating regeneration: %s", mysql_error(conn));
            }
        }
    }
    
    mysql_free_result(result);
}

/* ===== BASIC DEPLETION FUNCTIONS ===== */

/* Get resource-specific depletion rate based on resource type */
float get_resource_depletion_rate(int resource_type)
{
    switch (resource_type) {
        case RESOURCE_VEGETATION:  return 0.08;  /* Fast growing, fast depletion */
        case RESOURCE_HERBS:       return 0.06;  /* Moderate depletion */
        case RESOURCE_WATER:       return 0.03;  /* Seasonal regeneration */
        case RESOURCE_GAME:        return 0.10;  /* Mobile, can be overhunted */
        case RESOURCE_WOOD:        return 0.04;  /* Slow regeneration */
        case RESOURCE_CLAY:        return 0.02;  /* Renewable with weather */
        case RESOURCE_STONE:       return 0.01;  /* Very slow regeneration */
        case RESOURCE_MINERALS:    return 0.005; /* Nearly non-renewable */
        case RESOURCE_CRYSTAL:     return 0.002; /* Extremely rare regeneration */
        case RESOURCE_SALT:        return 0.01;  /* Location dependent */
        default:                   return 0.05;  /* Default moderate depletion */
    }
}

/* Get the current depletion level for a resource at a location (0.0-1.0) */
float get_resource_depletion_level(room_rnum room, int resource_type)
{
    char query[MAX_STRING_LENGTH];
    MYSQL_RES *result;
    MYSQL_ROW row;
    float depletion_level = 1.0; /* Default to fully available */
    int x, y, zone_vnum;
    
    if (room == NOWHERE || resource_type < 0)
        return 1.0;
    
    /* If MySQL not available, use mock data */
    if (!mysql_available || !conn) {
        /* Simple mock depletion based on room number for testing */
        float base_depletion = (room % 100) / 1000.0; /* 0.0-0.099 */
        if (base_depletion < 0.0) base_depletion = 0.0;
        if (base_depletion > 1.0) base_depletion = 1.0;
        return 1.0 - base_depletion;
    }
    
    /* Apply lazy regeneration first */
    apply_lazy_regeneration(room, resource_type);
    
    /* Get coordinates and zone */
    x = world[room].coords[0];
    y = world[room].coords[1];
    zone_vnum = zone_table[world[room].zone].number;
    
    /* Query database for depletion level using coordinates */
    snprintf(query, sizeof(query),
        "SELECT depletion_level FROM resource_depletion "
        "WHERE zone_vnum = %d AND x_coord = %d AND y_coord = %d AND resource_type = %d",
        zone_vnum, x, y, resource_type);
    
    if (mysql_query_safe(conn, query)) {
        log("SYSERR: Error querying resource depletion: %s", mysql_error(conn));
        return 1.0; /* Default to fully available on error */
    }
    
    result = mysql_store_result_safe(conn);
    
    if (result) {
        if ((row = mysql_fetch_row(result))) {
            depletion_level = atof(row[0]);
            if (depletion_level < 0.0) depletion_level = 0.0;
            if (depletion_level > 1.0) depletion_level = 1.0;
        }
        mysql_free_result(result);
    }
    
    return depletion_level;
}

/* Apply depletion when resources are harvested */
void apply_harvest_depletion(room_rnum room, int resource_type, int quantity)
{
    char query[MAX_STRING_LENGTH];
    int x, y, zone_vnum;
    
    if (room == NOWHERE || resource_type < 0 || quantity <= 0)
        return;
    
    /* If MySQL not available, skip database operations */
    if (!mysql_available || !conn) {
        return;
    }
    
    /* Get coordinates and zone */
    x = world[room].coords[0];
    y = world[room].coords[1];
    zone_vnum = zone_table[world[room].zone].number;
    
    /* Calculate depletion amount based on quantity harvested and resource type */
    float base_depletion_rate = get_resource_depletion_rate(resource_type);
    float depletion_amount = quantity * base_depletion_rate;
    if (depletion_amount > 0.25) depletion_amount = 0.25; /* Cap at 25% per harvest */
    
    /* Insert or update resource depletion record using coordinates */
    snprintf(query, sizeof(query),
        "INSERT INTO resource_depletion "
        "(zone_vnum, x_coord, y_coord, resource_type, depletion_level, total_harvested) "
        "VALUES (%d, %d, %d, %d, %.3f, %d) "
        "ON DUPLICATE KEY UPDATE "
        "depletion_level = GREATEST(0.0, depletion_level - %.3f), "
        "total_harvested = total_harvested + %d, "
        "last_harvest = CURRENT_TIMESTAMP",
        zone_vnum, x, y, resource_type, 1.0 - depletion_amount, quantity,
        depletion_amount, quantity);
    
    if (mysql_query_safe(conn, query)) {
        log("SYSERR: Error updating resource depletion: %s", mysql_error(conn));
    }
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
    char query[MAX_STRING_LENGTH];
    long player_id;
    
    if (!ch || IS_NPC(ch))
        return;
    
    /* If MySQL not available, skip database operations */
    if (!mysql_available || !conn) {
        return;
    }
    
    /* Get player ID */
    player_id = GET_IDNUM(ch);
    
    /* Calculate score adjustment */
    float score_change = sustainable ? 0.01 : -0.02; /* +1% for sustainable, -2% for unsustainable */
    
    /* Insert or update conservation score */
    snprintf(query, sizeof(query),
        "INSERT INTO player_conservation "
        "(player_id, conservation_score, total_harvests, %s) "
        "VALUES (%ld, %.3f, 1, 1) "
        "ON DUPLICATE KEY UPDATE "
        "conservation_score = GREATEST(0.0, LEAST(1.0, conservation_score + %.3f)), "
        "total_harvests = total_harvests + 1, "
        "%s = %s + 1, "
        "last_updated = CURRENT_TIMESTAMP",
        sustainable ? "sustainable_harvests" : "unsustainable_harvests",
        player_id, 0.5 + score_change, score_change,
        sustainable ? "sustainable_harvests" : "unsustainable_harvests",
        sustainable ? "sustainable_harvests" : "unsustainable_harvests");
    
    if (mysql_query_safe(conn, query)) {
        log("SYSERR: Error updating conservation score: %s", mysql_error(conn));
    }
}

/* Get a player's conservation score for a resource type */
float get_player_conservation_score(struct char_data *ch)
{
    char query[MAX_STRING_LENGTH];
    MYSQL_RES *result;
    MYSQL_ROW row;
    float conservation_score = 0.5; /* Default neutral score */
    long player_id;
    
    if (!ch || IS_NPC(ch))
        return 0.5;
    
    /* If MySQL not available, return neutral score */
    if (!mysql_available || !conn) {
        return 0.5;
    }
    
    player_id = GET_IDNUM(ch);
    
    /* Query database for conservation score */
    snprintf(query, sizeof(query),
        "SELECT conservation_score FROM player_conservation "
        "WHERE player_id = %ld",
        player_id);
    
    if (mysql_query_safe(conn, query)) {
        log("SYSERR: Error querying conservation score: %s", mysql_error(conn));
        return 0.5;
    }
    
    result = mysql_store_result_safe(conn);
    
    if (result) {
        if ((row = mysql_fetch_row(result))) {
            conservation_score = atof(row[0]);
            if (conservation_score < 0.0) conservation_score = 0.0;
            if (conservation_score > 1.0) conservation_score = 1.0;
        }
        mysql_free_result(result);
    }
    
    return conservation_score;
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
    char query[MAX_STRING_LENGTH];
    MYSQL_RES *result;
    MYSQL_ROW row;
    time_t current_time = time(NULL);
    int zone_vnum = zone_table[world[IN_ROOM(ch)].zone].number;
    
    if (!ch)
        return;
    
    if (!mysql_available || !conn) {
        send_to_char(ch, "Database not available for regeneration analysis.\r\n");
        return;
    }
    
    send_to_char(ch, "\tCResource Regeneration Analysis for (%d, %d):\tn\r\n", x, y);
    send_to_char(ch, "Resource Type     | Current | Regen Rate | Hours Since Harvest\r\n");
    send_to_char(ch, "------------------|---------|------------|--------------------\r\n");
    
    /* Query all resource types for this location */
    snprintf(query, sizeof(query),
        "SELECT resource_type, depletion_level, UNIX_TIMESTAMP(last_harvest) FROM resource_depletion "
        "WHERE zone_vnum = %d AND x_coord = %d AND y_coord = %d ORDER BY resource_type",
        zone_vnum, x, y);
    
    if (mysql_query_safe(conn, query)) {
        send_to_char(ch, "Error querying regeneration data.\r\n");
        return;
    }
    
    result = mysql_store_result_safe(conn);
    if (!result) {
        send_to_char(ch, "No depletion data found for this location.\r\n");
        return;
    }
    
    const char *resource_names[] = {
        "Vegetation", "Minerals", "Water", "Herbs", "Game",
        "Wood", "Stone", "Crystal", "Clay", "Salt"
    };
    
    while ((row = mysql_fetch_row(result))) {
        int resource_type = atoi(row[0]);
        float depletion_level = atof(row[1]);
        time_t last_harvest = (time_t)atol(row[2]);
        double hours_since = difftime(current_time, last_harvest) / 3600.0;
        float regen_rate = get_resource_regeneration_rate(resource_type);
        
        send_to_char(ch, "%-17s | %6.1f%% | %9.1f%% | %18.1f\r\n",
            resource_names[resource_type],
            depletion_level * 100.0,
            regen_rate * 100.0,
            hours_since);
    }
    
    mysql_free_result(result);
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
