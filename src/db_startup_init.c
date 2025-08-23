/* *************************************************************************
 *   File: db_startup_init.c                         Part of LuminariMUD *
 *  Usage: Database startup initialization and status checking           *
 * Author: Database Infrastructure Team                                  *
 ***************************************************************************
 * Contains functions called during MUD startup to ensure database       *
 * integrity and perform any necessary initialization automatically.     *
 ***************************************************************************/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "interpreter.h"
#include "db.h"
#include "comm.h"
#include "mysql.h"
#include "db_init.h"

/* ===== STARTUP INITIALIZATION FUNCTIONS ===== */

/* Main startup database initialization function called from boot_db() */
void startup_database_init(void)
{
    if (!mysql_available || !conn) {
        log("MySQL not available during startup - skipping database initialization");
        return;
    }

    log("Starting database startup initialization...");

    /* Perform selective table initialization - only create missing tables */
    initialize_missing_tables();

    /* Verify critical systems are functional */
    if (!verify_core_player_tables()) {
        log("WARNING: Core player tables verification failed during startup");
    }

    log("Database startup initialization completed successfully");
}

/* Selectively initialize only missing table systems */
void initialize_missing_tables(void)
{
    if (!mysql_available || !conn) {
        return;
    }

    log("Checking for missing database tables...");

    /* Check and initialize individual table systems */
    
    /* Core player tables */
    if (!table_exists("player_data")) {
        log("Initializing core player tables...");
        init_core_player_tables();
        log("Core player tables initialized");
    }

    /* Object database tables */
    if (!table_exists("object_database_items")) {
        log("Initializing object database tables...");
        init_object_database_tables();
        log("Object database tables initialized");
    }

    /* Region system tables */
    if (!table_exists("region_data")) {
        log("Initializing region system tables...");
        init_region_system_tables();
        populate_region_system_data();
        log("Region system tables initialized");
    }

    /* Resource system tables */
    if (!table_exists("resource_types")) {
        log("Initializing resource system tables...");
        init_wilderness_resource_tables();
        populate_resource_types_data();
        populate_material_categories_data();
        populate_material_qualities_data();
        log("Resource system tables initialized");
    }

    /* Region hints tables - NEW SYSTEM */
    if (!table_exists("region_hints")) {
        log("Initializing region hints tables...");
        init_region_hints_tables();
        log("Region hints tables initialized");
    }

    /* AI service tables */
    if (!table_exists("ai_service_config")) {
        log("Initializing AI service tables...");
        init_ai_service_tables();
        populate_ai_config_data();
        log("AI service tables initialized");
    }

    /* Crafting system tables */
    if (!table_exists("crafting_recipes")) {
        log("Initializing crafting system tables...");
        init_crafting_system_tables();
        log("Crafting system tables initialized");
    }

    /* Housing system tables */
    if (!table_exists("player_housing")) {
        log("Initializing housing system tables...");
        init_housing_system_tables();
        log("Housing system tables initialized");
    }

    /* Help system tables */
    if (!table_exists("help_entries")) {
        log("Initializing help system tables...");
        init_help_system_tables();
        log("Help system tables initialized");
    }

    /* Create database procedures if they don't exist */
    if (!procedure_exists("bresenham_line")) {
        log("Creating database procedures...");
        create_database_procedures();
        log("Database procedures created");
    }

    log("Missing table initialization completed");
}

/* Helper function to check if a table exists */
int table_exists(const char *table_name)
{
    char query[512];
    
    if (!mysql_available || !conn || !table_name) {
        return FALSE;
    }

    snprintf(query, sizeof(query), "SHOW TABLES LIKE '%s'", table_name);
    
    if (mysql_query_safe(conn, query)) {
        log("SYSERR: Error checking table %s: %s", table_name, mysql_error(conn));
        return FALSE;
    }

    MYSQL_RES *result = mysql_store_result_safe(conn);
    if (!result || mysql_num_rows(result) == 0) {
        if (result) mysql_free_result(result);
        return FALSE;
    }
    
    mysql_free_result(result);
    return TRUE;
}

/* Helper function to check if a stored procedure exists */
int procedure_exists(const char *procedure_name)
{
    char query[512];
    
    if (!mysql_available || !conn || !procedure_name) {
        return FALSE;
    }

    snprintf(query, sizeof(query), 
            "SELECT ROUTINE_NAME FROM INFORMATION_SCHEMA.ROUTINES "
            "WHERE ROUTINE_SCHEMA = DATABASE() AND ROUTINE_NAME = '%s'", 
            procedure_name);
    
    if (mysql_query_safe(conn, query)) {
        log("SYSERR: Error checking procedure %s: %s", procedure_name, mysql_error(conn));
        return FALSE;
    }

    MYSQL_RES *result = mysql_store_result_safe(conn);
    if (!result || mysql_num_rows(result) == 0) {
        if (result) mysql_free_result(result);
        return FALSE;
    }
    
    mysql_free_result(result);
    return TRUE;
}

/* Check if stored procedures exist */
int verify_database_procedures(void)
{
    if (!mysql_available || !conn) {
        return FALSE;
    }

    char *procedures[] = {
        "bresenham_line",
        "calculate_distance", 
        "find_path_between_regions",
        "get_regions_within_distance"
    };
    
    int num_procedures = sizeof(procedures) / sizeof(procedures[0]);
    char query[1024];
    int i;

    for (i = 0; i < num_procedures; i++) {
        snprintf(query, sizeof(query), 
                "SELECT ROUTINE_NAME FROM INFORMATION_SCHEMA.ROUTINES "
                "WHERE ROUTINE_SCHEMA = DATABASE() AND ROUTINE_NAME = '%s'", 
                procedures[i]);
        
        if (mysql_query_safe(conn, query)) {
            log("SYSERR: Error checking procedure %s: %s", procedures[i], mysql_error(conn));
            return FALSE;
        }

        MYSQL_RES *result = mysql_store_result_safe(conn);
        if (!result || mysql_num_rows(result) == 0) {
            if (result) mysql_free_result(result);
            log("Stored procedure '%s' missing", procedures[i]);
            return FALSE;
        }
        mysql_free_result(result);
    }

    log("All required stored procedures are present");
    return TRUE;
}

/* Enhanced database connectivity and feature test */
int test_database_features(void)
{
    if (!mysql_available || !conn) {
        log("Database not available for feature testing");
        return FALSE;
    }

    /* Test basic functionality */
    if (mysql_query_safe(conn, "SELECT 1")) {
        log("SYSERR: Basic database connectivity test failed: %s", mysql_error(conn));
        return FALSE;
    }

    MYSQL_RES *result = mysql_store_result_safe(conn);
    if (result) {
        mysql_free_result(result);
    }

    /* Test spatial/geometry support (critical for region system) */
    if (mysql_query_safe(conn, "SELECT ST_GeomFromText('POINT(0 0)')")) {
        log("WARNING: Spatial/geometry functions not available - region system may not work properly");
        log("MySQL error: %s", mysql_error(conn));
        return FALSE;
    }

    result = mysql_store_result_safe(conn);
    if (result) {
        mysql_free_result(result);
    }

    /* Test CREATE TABLE permissions */
    if (mysql_query_safe(conn, "CREATE TEMPORARY TABLE test_permissions (id INT)")) {
        log("WARNING: CREATE TABLE permission not available");
        return FALSE;
    }

    mysql_query_safe(conn, "DROP TEMPORARY TABLE test_permissions");

    log("Database features test passed - all required functionality available");
    return TRUE;
}

/* Perform startup database repair if needed */
void repair_database_if_needed(void)
{
    if (!mysql_available || !conn) {
        return;
    }

    log("Checking if database repair is needed...");

    /* Check for common issues and attempt automatic repair */
    
    /* 1. Missing indexes on critical tables */
    char *index_queries[] = {
        "CREATE INDEX IF NOT EXISTS idx_player_name ON player_data(name)",
        "CREATE INDEX IF NOT EXISTS idx_object_vnum ON object_prototypes(vnum)",
        "CREATE SPATIAL INDEX IF NOT EXISTS idx_region_geometry ON region_data(geometry)",
        "CREATE SPATIAL INDEX IF NOT EXISTS idx_path_geometry ON path_data(path_geometry)",
        NULL
    };

    int i;
    for (i = 0; index_queries[i] != NULL; i++) {
        if (mysql_query_safe(conn, index_queries[i])) {
            /* Non-critical - log but continue */
            log("Info: Could not create index (may already exist): %s", mysql_error(conn));
        }
    }

    /* 2. Verify stored procedures exist */
    if (!verify_database_procedures()) {
        log("NOTICE: Some stored procedures missing - recreating...");
        create_database_procedures();
    }

    log("Database repair check completed");
}

/* Log startup database statistics */
void log_database_startup_stats(void)
{
    if (!mysql_available || !conn) {
        log("Database statistics unavailable - no connection");
        return;
    }

    log("=== DATABASE STARTUP STATISTICS ===");

    /* Get table count */
    if (!mysql_query_safe(conn, "SELECT COUNT(*) FROM INFORMATION_SCHEMA.TABLES WHERE TABLE_SCHEMA = DATABASE()")) {
        MYSQL_RES *result = mysql_store_result_safe(conn);
        if (result) {
            MYSQL_ROW row = mysql_fetch_row(result);
            if (row && row[0]) {
                log("Total database tables: %s", row[0]);
            }
            mysql_free_result(result);
        }
    }

    /* Get procedure count */
    if (!mysql_query_safe(conn, "SELECT COUNT(*) FROM INFORMATION_SCHEMA.ROUTINES WHERE ROUTINE_SCHEMA = DATABASE()")) {
        MYSQL_RES *result = mysql_store_result_safe(conn);
        if (result) {
            MYSQL_ROW row = mysql_fetch_row(result);
            if (row && row[0]) {
                log("Total stored procedures: %s", row[0]);
            }
            mysql_free_result(result);
        }
    }

    /* Check key table status */
    char *key_tables[] = {
        "player_data", "object_prototypes", "region_data", "resource_types"
    };
    int num_tables = sizeof(key_tables) / sizeof(key_tables[0]);
    int i;
    
    for (i = 0; i < num_tables; i++) {
        char query[512];
        snprintf(query, sizeof(query), "SELECT COUNT(*) FROM %s", key_tables[i]);
        
        if (!mysql_query_safe(conn, query)) {
            MYSQL_RES *result = mysql_store_result_safe(conn);
            if (result) {
                MYSQL_ROW row = mysql_fetch_row(result);
                if (row && row[0]) {
                    log("Table %s: %s records", key_tables[i], row[0]);
                }
                mysql_free_result(result);
            }
        }
    }

    log("=== END DATABASE STATISTICS ===");
}
