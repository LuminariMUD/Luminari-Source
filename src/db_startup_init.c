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

    /* Perform quick database status check */
    if (!quick_database_status()) {
        log("NOTICE: Database tables missing or incomplete - running full initialization");
        init_luminari_database();
        
        /* Populate with reference data if tables were created */
        log("Populating database with reference data...");
        populate_resource_types_data();
        populate_material_categories_data();
        populate_material_qualities_data();
        populate_region_effects_data();
        populate_ai_config_data();
        populate_region_system_data();
        
        log("Database initialization and data population completed");
    } else {
        log("Database tables present and operational");
    }

    /* Verify critical systems are functional */
    if (!verify_core_player_tables()) {
        log("WARNING: Core player tables verification failed during startup");
    }

    log("Database startup initialization completed successfully");
}

/* Quick check to see if essential tables exist */
int quick_database_status(void)
{
    if (!mysql_available || !conn) {
        return FALSE;
    }

    /* Check for a few key tables from different systems */
    char *essential_tables[] = {
        "player_data",              /* Core player system */
        "object_database_items",    /* Object system */
        "region_data",              /* Region system */
        "resource_types"            /* Resource system */
    };
    
    int num_tables = sizeof(essential_tables) / sizeof(essential_tables[0]);
    char query[1024];
    int i;

    for (i = 0; i < num_tables; i++) {
        snprintf(query, sizeof(query), "SHOW TABLES LIKE '%s'", essential_tables[i]);
        
        if (mysql_query_safe(conn, query)) {
            log("SYSERR: Error checking essential table %s during startup: %s", 
                essential_tables[i], mysql_error(conn));
            return FALSE;
        }

        MYSQL_RES *result = mysql_store_result_safe(conn);
        if (!result || mysql_num_rows(result) == 0) {
            if (result) mysql_free_result(result);
            log("Essential table '%s' missing during startup check", essential_tables[i]);
            return FALSE;
        }
        mysql_free_result(result);
    }

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
