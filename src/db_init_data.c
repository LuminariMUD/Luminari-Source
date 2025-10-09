/* *************************************************************************
 *   File: db_init_data.c                            Part of LuminariMUD *
 *  Usage: Database reference data population and verification system    *
 * Author: Database Infrastructure Team                                  *
 ***************************************************************************
 * Contains functions for populating database tables with standard       *
 * reference data and verifying database integrity across all systems.   *
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

/* ===== REFERENCE DATA INITIALIZATION ===== */

/* Helper function to check if a table has any data */
static int table_has_data(const char *table_name)
{
    char query[256];
    snprintf(query, sizeof(query), "SELECT COUNT(*) FROM %s", table_name);
    
    if (mysql_query_safe(conn, query)) {
        log("SYSERR: Failed to check if table %s has data: %s", table_name, mysql_error(conn));
        return 1; /* Assume it has data to be safe */
    }
    
    MYSQL_RES *result = mysql_store_result_safe(conn);
    if (!result) {
        log("SYSERR: Failed to get result for table %s data check", table_name);
        return 1; /* Assume it has data to be safe */
    }
    
    MYSQL_ROW row = mysql_fetch_row(result);
    int count = 0;
    if (row && row[0]) {
        count = atoi(row[0]);
    }
    mysql_free_result(result);
    
    return count > 0;
}

/* Ensure path_types table exists and populate default reference data */
void ensure_path_types_reference(void)
{
    if (!mysql_available || !conn) {
        return;
    }

    const char *create_path_types =
        "CREATE TABLE IF NOT EXISTS path_types ("
        "path_type INT PRIMARY KEY, "
        "type_name VARCHAR(50) NOT NULL, "
        "glyph_ns VARCHAR(16) NOT NULL, "
        "glyph_ew VARCHAR(16) NOT NULL, "
        "glyph_int VARCHAR(16) NOT NULL, "
        "UNIQUE KEY idx_type_name (type_name)"
        ")";

    if (mysql_query_safe(conn, create_path_types)) {
        log("SYSERR: Failed to ensure path_types table exists: %s", mysql_error(conn));
        return;
    }

    if (table_has_data("path_types")) {
        log("Info: path_types table already contains data - verifying required defaults");
    } else {
        log("Info: Populating default wilderness path type glyph definitions...");
    }

    const char *insert_path_types =
        "INSERT INTO path_types (path_type, type_name, glyph_ns, glyph_ew, glyph_int) VALUES "
        "(1, 'Paved Road', '@Y|@n', '@Y-@n', '@Y+@n'),"
        "(2, 'Dirt Road', '@y|@n', '@y-@n', '@y+@n'),"
        "(3, 'Geographic Feature', '@G|@n', '@G-@n', '@G+@n'),"
        "(5, 'River', '@B|@n', '@B=@n', '@B#@n'),"
        "(6, 'Stream', '@c|@n', '@c~@n', '@c+@n') "
        "ON DUPLICATE KEY UPDATE path_type = path_type";

    if (mysql_query_safe(conn, insert_path_types)) {
        log("SYSERR: Failed to populate default path_types data: %s", mysql_error(conn));
        return;
    }

    log("Info: Default path_types reference data verified");
}

/* Populate resource_types with standard resource definitions - ONLY if table is empty */
void populate_resource_types_data(void)
{
    if (!mysql_available || !conn) {
        log("MySQL not available, skipping resource types data population");
        return;
    }

    /* CRITICAL: Never overwrite existing data */
    if (table_has_data("resource_types")) {
        log("Info: resource_types table has existing data - skipping population to preserve data");
        return;
    }

    log("Populating resource types reference data...");
    
    char *resource_data[] = {
        "INSERT IGNORE INTO resource_types (resource_id, resource_name, resource_description, base_rarity) VALUES "
        "(1, 'Iron Ore', 'Common iron ore deposits', 0.80),"
        "(2, 'Copper Ore', 'Copper ore veins', 0.75),"
        "(3, 'Silver Ore', 'Precious silver deposits', 0.40),"
        "(4, 'Gold Ore', 'Valuable gold veins', 0.20),"
        "(5, 'Mithril Ore', 'Legendary mithril deposits', 0.05),"
        "(6, 'Timber', 'Standard wood resources', 0.90),"
        "(7, 'Stone', 'Basic stone quarries', 0.95),"
        "(8, 'Magical Crystals', 'Enchanted crystal formations', 0.15);",
        NULL
    };

    int i;
    for (i = 0; resource_data[i] != NULL; i++) {
        if (mysql_query_safe(conn, resource_data[i])) {
            log("SYSERR: Failed to populate resource_types data: %s", mysql_error(conn));
            return;
        }
    }
    
    log("Info: Resource types reference data populated successfully");
}

/* Populate material_categories with standard categories - ONLY if table is empty */
void populate_material_categories_data(void)
{
    if (!mysql_available || !conn) {
        log("MySQL not available, skipping material categories data population");
        return;
    }

    /* CRITICAL: Never overwrite existing data */
    if (table_has_data("material_categories")) {
        log("Info: material_categories table has existing data - skipping population to preserve data");
        return;
    }

    log("Populating material categories reference data...");
    
    char *material_data[] = {
        "INSERT IGNORE INTO material_categories (category_id, category_name, category_description) VALUES "
        "(1, 'Cloth', 'Lightweight fabric materials'),"
        "(2, 'Leather', 'Treated animal hide'),"
        "(3, 'Chain', 'Interlocked metal links'),"
        "(4, 'Scale', 'Overlapping metal plates'),"
        "(5, 'Plate', 'Solid metal armor plates'),"
        "(6, 'Mithril', 'Magical lightweight metal');",
        NULL
    };

    int i;
    for (i = 0; material_data[i] != NULL; i++) {
        if (mysql_query_safe(conn, material_data[i])) {
            log("SYSERR: Failed to populate material_categories data: %s", mysql_error(conn));
            return;
        }
    }
    
    log("Info: Material categories reference data populated successfully");
}

/* Populate material_qualities with standard quality levels - ONLY if table is empty */
void populate_material_qualities_data(void)
{
    if (!mysql_available || !conn) {
        log("MySQL not available, skipping material qualities data population");
        return;
    }

    /* CRITICAL: Never overwrite existing data */
    if (table_has_data("material_qualities")) {
        log("Info: material_qualities table has existing data - skipping population to preserve data");
        return;
    }

    log("Populating material qualities reference data...");
    
    char *quality_data[] = {
        "INSERT IGNORE INTO material_qualities (quality_id, quality_name, quality_description, rarity_multiplier, value_multiplier) VALUES "
        "(1, 'Poor', 'Below average quality materials', 1.50, 0.70),"
        "(2, 'Average', 'Standard quality materials', 1.00, 1.00),"
        "(3, 'Good', 'Above average quality materials', 0.75, 1.30),"
        "(4, 'Excellent', 'High quality materials', 0.50, 1.60),"
        "(5, 'Masterwork', 'Exceptional quality materials', 0.25, 2.00);",
        NULL
    };

    int i;
    for (i = 0; quality_data[i] != NULL; i++) {
        if (mysql_query_safe(conn, quality_data[i])) {
            log("SYSERR: Failed to populate material_qualities data: %s", mysql_error(conn));
            return;
        }
    }
    
    log("Info: Material qualities reference data populated successfully");
}

/* Populate region_effects with standard environmental effects - ONLY if table is empty */
void populate_region_effects_data(void)
{
    if (!mysql_available || !conn) {
        log("MySQL not available, skipping region effects data population");
        return;
    }

    /* CRITICAL: Never overwrite existing data */
    if (table_has_data("region_effects")) {
        log("Info: region_effects table has existing data - skipping population to preserve data");
        return;
    }

    log("Populating region effects reference data...");
    
    char *effects_data[] = {
        "INSERT IGNORE INTO region_effects (effect_id, effect_name, effect_type, effect_description) VALUES "
        "(1, 'Fertile Soil', 'resource', 'Rich soil increases agricultural yields'),"
        "(2, 'Mineral Rich', 'resource', 'Abundant mineral deposits increase mining output'),"
        "(3, 'Magical Aura', 'magic', 'Natural magical energy enhances mana recovery'),"
        "(4, 'Harsh Weather', 'weather', 'Difficult weather conditions affect movement'),"
        "(5, 'Ancient Blessing', 'other', 'Mystical energies enhance learning and experience');",
        NULL
    };

    int i;
    for (i = 0; effects_data[i] != NULL; i++) {
        if (mysql_query_safe(conn, effects_data[i])) {
            log("SYSERR: Failed to populate region_effects data: %s", mysql_error(conn));
            return;
        }
    }
    
    char *relationships_data[] = {
        "INSERT IGNORE INTO resource_relationships (source_resource, target_resource, effect_type, effect_magnitude, description) VALUES "
        "(1, 6, 'enhancement', 1.300, 'Iron enhances timber processing efficiency'),"  /* Iron enhances timber processing */
        "(3, 4, 'enhancement', 1.200, 'Silver and gold synergy in processing'),"      /* Silver and gold synergy */
        "(5, 8, 'enhancement', 2.000, 'Mithril amplifies magical crystal power');",   /* Mithril amplifies crystal power */
        NULL
    };

    for (i = 0; relationships_data[i] != NULL; i++) {
        if (mysql_query_safe(conn, relationships_data[i])) {
            log("SYSERR: Failed to populate resource_relationships data: %s", mysql_error(conn));
            return;
        }
    }
    
    log("Info: Region effects reference data populated successfully");
}

/* Populate ai_config with default AI service settings - ONLY if table is empty */
void populate_ai_config_data(void)
{
    if (!mysql_available || !conn) {
        log("MySQL not available, skipping AI config data population");
        return;
    }

    /* CRITICAL: Never overwrite existing data */
    if (table_has_data("ai_config")) {
        log("Info: ai_config table has existing data - skipping population to preserve data");
        return;
    }

    log("Populating AI configuration reference data...");
    
    char *ai_data[] = {
        "INSERT IGNORE INTO ai_config (config_key, config_value) VALUES "
        "('max_tokens', '150'),"
        "('temperature', '0.7'),"
        "('response_timeout', '10'),"
        "('enable_personality', 'true'),"
        "('debug_mode', 'false');",
        NULL
    };

    int i;
    for (i = 0; ai_data[i] != NULL; i++) {
        if (mysql_query_safe(conn, ai_data[i])) {
            log("SYSERR: Failed to populate ai_config data: %s", mysql_error(conn));
            return;
        }
    }
    
    log("Info: AI configuration reference data populated successfully");
}

/* NEVER populate region system data - managed by wildedit */
void populate_region_system_data(void)
{
    if (!mysql_available || !conn) {
        log("MySQL not available, skipping region system data population");
        return;
    }

    /* Ensure reference tables for paths exist even if wildedit has not run */
    ensure_path_types_reference();

    /* CRITICAL: NEVER populate region_data, path_data, region_index, or path_index */
    /* These tables are managed by the wildedit program and contain MUD-specific data */
    /* Attempting to populate them could overwrite important world data */
    
    log("Info: Region system data is managed by wildedit - skipping automatic population");
    log("Info: Use wildedit to manage region_data, path_data, region_index, and path_index tables");
}

/* Populate vessel room template reference data */
void populate_ship_room_templates_data(void)
{
    if (!mysql_available || !conn) {
        log("MySQL not available, skipping ship room templates population");
        return;
    }

    if (table_has_data("ship_room_templates")) {
        log("Info: ship_room_templates already contains data - skipping default population");
        return;
    }

    log("Populating default ship room templates...");

    const char *insert_templates =
        "INSERT INTO ship_room_templates "
        "(room_type, vessel_type, name_format, description_text, room_flags, sector_type, min_vessel_size) VALUES "
        "('bridge', 0, 'The %s''s Bridge', 'The command center of the vessel, filled with navigation equipment and control panels. Large windows provide a panoramic view of the surroundings.', 262144, 0, 0),"
        "('helm', 0, 'The %s''s Helm', 'The pilot''s station, featuring the ship''s wheel and primary navigation controls.', 262144, 0, 0),"
        "('quarters_captain', 0, 'Captain''s Quarters', 'A spacious cabin befitting the ship''s commander, with a large bunk, desk, and personal storage.', 262144, 0, 3),"
        "('quarters_crew', 0, 'Crew Quarters', 'Rows of bunks line the walls of this cramped but functional sleeping area.', 262144, 0, 1),"
        "('quarters_officer', 0, 'Officers'' Quarters', 'Modest private cabins for the ship''s officers, each with a bunk and small desk.', 262144, 0, 5),"
        "('cargo_main', 0, 'Main Cargo Hold', 'A cavernous space filled with crates, barrels, and secured cargo. The air smells of tar and sea salt.', 262144, 0, 2),"
        "('cargo_secure', 0, 'Secure Cargo Hold', 'A reinforced compartment with heavy locks, used for valuable or dangerous cargo.', 262144, 0, 5),"
        "('engineering', 0, 'Engineering', 'The heart of the ship''s mechanical systems, filled with pipes, gauges, and machinery.', 262144, 0, 3),"
        "('weapons', 0, 'Weapons Deck', 'Cannons and ballistae line this reinforced deck, ready for naval combat.', 262144, 0, 5),"
        "('armory', 0, 'Ship''s Armory', 'Racks of weapons and armor line the walls, secured behind iron bars.', 262144, 0, 7),"
        "('mess_hall', 0, 'Mess Hall', 'Long tables and benches fill this communal dining area. The lingering smell of the last meal hangs in the air.', 262144, 0, 3),"
        "('galley', 0, 'Ship''s Galley', 'The ship''s kitchen, equipped with a large stove and food preparation areas.', 262144, 0, 2),"
        "('infirmary', 0, 'Ship''s Infirmary', 'A small medical bay with cots and basic healing supplies.', 262144, 0, 4),"
        "('corridor', 0, 'Ship''s Corridor', 'A narrow passageway connecting different sections of the vessel.', 262144, 0, 0),"
        "('deck_main', 0, 'Main Deck', 'The open deck of the ship, exposed to the elements. Rigging and masts tower overhead.', 262144, 0, 0),"
        "('deck_lower', 0, 'Lower Deck', 'Below the main deck, this area provides access to the ship''s interior compartments.', 262144, 0, 1),"
        "('airlock', 0, 'Airlock', 'A sealed chamber used for boarding and emergency exits.', 262144, 0, 10),"
        "('observation', 0, 'Observation Deck', 'An elevated platform providing excellent views in all directions.', 262144, 0, 5),"
        "('brig', 0, 'Ship''s Brig', 'Iron-barred cells for holding prisoners or unruly crew members.', 262144, 0, 6)";

    if (mysql_query_safe(conn, insert_templates)) {
        log("SYSERR: Failed to populate ship_room_templates data: %s", mysql_error(conn));
        return;
    }

    log("Info: Ship room template reference data populated successfully");
}

/* ===== DATABASE VERIFICATION FUNCTIONS ===== */

/* Check if database connection is available */
int is_database_available(void)
{
    return (mysql_available && conn != NULL);
}

/* Test basic database permissions */
int test_database_permissions(void)
{
    if (!is_database_available()) {
        return FALSE;
    }

    /* Test basic connectivity */
    if (mysql_query_safe(conn, "SELECT 1")) {
        log("SYSERR: Database connectivity test failed: %s", mysql_error(conn));
        return FALSE;
    }

    MYSQL_RES *result = mysql_store_result_safe(conn);
    if (result) {
        mysql_free_result(result);
    }

    /* Test CREATE permission */
    if (mysql_query_safe(conn, "CREATE TEMPORARY TABLE test_permissions (id INT)")) {
        log("SYSERR: Database CREATE permission test failed: %s", mysql_error(conn));
        return FALSE;
    }

    /* Test INSERT permission */
    if (mysql_query_safe(conn, "INSERT INTO test_permissions (id) VALUES (1)")) {
        log("SYSERR: Database INSERT permission test failed: %s", mysql_error(conn));
        return FALSE;
    }

    /* Cleanup */
    mysql_query_safe(conn, "DROP TEMPORARY TABLE test_permissions");
    
    log("Info: Database permissions test passed successfully");
    return TRUE;
}

/* Comprehensive database integrity verification */
int verify_database_integrity(void)
{
    if (!is_database_available()) {
        log("ERROR: Database not available for integrity check");
        return FALSE;
    }

    log("Verifying database integrity...");

    int all_valid = TRUE;

    /* Verify core systems */
    if (!verify_core_player_tables()) {
        log("ERROR: Core player tables verification failed");
        all_valid = FALSE;
    }

    if (!verify_object_database_tables()) {
        log("ERROR: Object database tables verification failed");
        all_valid = FALSE;
    }

    if (!verify_wilderness_resource_tables()) {
        log("ERROR: Wilderness resource tables verification failed");
        all_valid = FALSE;
    }

    if (!verify_ai_service_tables()) {
        log("ERROR: AI service tables verification failed");
        all_valid = FALSE;
    }

    if (!verify_crafting_system_tables()) {
        log("ERROR: Crafting system tables verification failed");
        all_valid = FALSE;
    }

    if (!verify_housing_system_tables()) {
        log("ERROR: Housing system tables verification failed");
        all_valid = FALSE;
    }

    if (!verify_help_system_tables()) {
        log("ERROR: Help system tables verification failed");
        all_valid = FALSE;
    }

    if (!verify_vessel_system_tables()) {
        log("ERROR: Vessel system tables verification failed");
        all_valid = FALSE;
    }

    if (all_valid) {
        log("SUCCESS: Database integrity verification passed");
    } else {
        log("ERROR: Database integrity verification failed - some tables missing or invalid");
    }

    return all_valid;
}

/* ===== INDIVIDUAL SYSTEM VERIFICATION FUNCTIONS ===== */

/* Verify core player tables exist */
int verify_core_player_tables(void)
{
    const char *tables[] = {
        "player_data",
        "account_data",
        "unlocked_races",
        "unlocked_classes",
        "player_save_objs",
        "player_save_objs_sheathed",
        "pet_save_objs"
    };
    int num_tables = sizeof(tables) / sizeof(tables[0]);
    char query[1024];
    int i;

    for (i = 0; i < num_tables; i++) {
        snprintf(query, sizeof(query), "SHOW TABLES LIKE '%s'", tables[i]);

        if (mysql_query_safe(conn, query)) {
            log("SYSERR: Error checking table %s: %s", tables[i], mysql_error(conn));
            return FALSE;
        }

        MYSQL_RES *result = mysql_store_result_safe(conn);
        if (!result || mysql_num_rows(result) == 0) {
            log("ERROR: Core player table '%s' does not exist", tables[i]);
            if (result) mysql_free_result(result);
            return FALSE;
        }
        mysql_free_result(result);
    }

    return TRUE;
}

/* Verify object database tables exist */
int verify_object_database_tables(void)
{
    const char *tables[] = {
        "object_database_items",
        "object_database_wear_slots",
        "object_database_bonuses"
    };
    int num_tables = sizeof(tables) / sizeof(tables[0]);
    char query[1024];
    int i;

    for (i = 0; i < num_tables; i++) {
        snprintf(query, sizeof(query), "SHOW TABLES LIKE '%s'", tables[i]);

        if (mysql_query_safe(conn, query)) {
            log("SYSERR: Error checking table %s: %s", tables[i], mysql_error(conn));
            return FALSE;
        }

        MYSQL_RES *result = mysql_store_result_safe(conn);
        if (!result || mysql_num_rows(result) == 0) {
            log("ERROR: Object database table '%s' does not exist", tables[i]);
            if (result) mysql_free_result(result);
            return FALSE;
        }
        mysql_free_result(result);
    }

    return TRUE;
}

/* Verify wilderness and resource tables exist */
int verify_wilderness_resource_tables(void)
{
    const char *tables[] = {
        "resource_types",
        "resource_depletion",
        "player_conservation",
        "resource_statistics",
        "resource_regeneration_log",
        "resource_relationships",
        "ecosystem_health",
        "cascade_effects_log",
        "player_location_conservation",
        "region_effects",
        "region_effect_assignments",
        "weather_cache",
        "room_description_settings",
        "material_categories",
        "material_subtypes",
        "material_qualities"
    };
    int num_tables = sizeof(tables) / sizeof(tables[0]);
    char query[1024];
    int i;

    for (i = 0; i < num_tables; i++) {
        snprintf(query, sizeof(query), "SHOW TABLES LIKE '%s'", tables[i]);
        
        if (mysql_query_safe(conn, query)) {
            log("SYSERR: Error checking table %s: %s", tables[i], mysql_error(conn));
            return FALSE;
        }

        MYSQL_RES *result = mysql_store_result_safe(conn);
        if (!result || mysql_num_rows(result) == 0) {
            log("ERROR: Wilderness resource table '%s' does not exist", tables[i]);
            if (result) mysql_free_result(result);
            return FALSE;
        }
        mysql_free_result(result);
    }

    return TRUE;
}

/* Verify AI service tables exist */
int verify_ai_service_tables(void)
{
    char *tables[] = {
        "ai_config", "ai_npc_personalities", "ai_cache", 
        "ai_requests"
    };
    int num_tables = sizeof(tables) / sizeof(tables[0]);
    char query[1024];
    int i;

    for (i = 0; i < num_tables; i++) {
        snprintf(query, sizeof(query), "SHOW TABLES LIKE '%s'", tables[i]);
        
        if (mysql_query_safe(conn, query)) {
            log("SYSERR: Error checking table %s: %s", tables[i], mysql_error(conn));
            return FALSE;
        }

        MYSQL_RES *result = mysql_store_result_safe(conn);
        if (!result || mysql_num_rows(result) == 0) {
            log("ERROR: AI service table '%s' does not exist", tables[i]);
            if (result) mysql_free_result(result);
            return FALSE;
        }
        mysql_free_result(result);
    }

    return TRUE;
}

/* Verify crafting system tables exist */
int verify_crafting_system_tables(void)
{
    const char *tables[] = {
        "supply_orders_available"
    };
    int num_tables = sizeof(tables) / sizeof(tables[0]);
    char query[1024];
    int i;

    for (i = 0; i < num_tables; i++) {
        snprintf(query, sizeof(query), "SHOW TABLES LIKE '%s'", tables[i]);

        if (mysql_query_safe(conn, query)) {
            log("SYSERR: Error checking table %s: %s", tables[i], mysql_error(conn));
            return FALSE;
        }

        MYSQL_RES *result = mysql_store_result_safe(conn);
        if (!result || mysql_num_rows(result) == 0) {
            log("ERROR: Crafting system table '%s' does not exist", tables[i]);
            if (result) mysql_free_result(result);
            return FALSE;
        }
        mysql_free_result(result);
    }

    return TRUE;
}

/* Verify housing system tables exist */
int verify_housing_system_tables(void)
{
    char *tables[] = {
        "house_data"
    };
    int num_tables = sizeof(tables) / sizeof(tables[0]);
    char query[1024];
    int i;

    for (i = 0; i < num_tables; i++) {
        snprintf(query, sizeof(query), "SHOW TABLES LIKE '%s'", tables[i]);
        
        if (mysql_query_safe(conn, query)) {
            log("SYSERR: Error checking table %s: %s", tables[i], mysql_error(conn));
            return FALSE;
        }

        MYSQL_RES *result = mysql_store_result_safe(conn);
        if (!result || mysql_num_rows(result) == 0) {
            log("ERROR: Housing system table '%s' does not exist", tables[i]);
            if (result) mysql_free_result(result);
            return FALSE;
        }
        mysql_free_result(result);
    }

    return TRUE;
}

/* Verify help system tables exist */
int verify_help_system_tables(void)
{
    const char *tables[] = {
        "help_entries",
        "help_keywords",
        "help_versions",
        "help_search_history",
        "help_related_topics"
    };
    int num_tables = sizeof(tables) / sizeof(tables[0]);
    char query[1024];
    int i;

    for (i = 0; i < num_tables; i++) {
        snprintf(query, sizeof(query), "SHOW TABLES LIKE '%s'", tables[i]);
        
        if (mysql_query_safe(conn, query)) {
            log("SYSERR: Error checking table %s: %s", tables[i], mysql_error(conn));
            return FALSE;
        }

        MYSQL_RES *result = mysql_store_result_safe(conn);
        if (!result || mysql_num_rows(result) == 0) {
            log("ERROR: Help system table '%s' does not exist", tables[i]);
            if (result) mysql_free_result(result);
            return FALSE;
        }
        mysql_free_result(result);
    }

    return TRUE;
}

/* Verify vessel system tables exist */
int verify_vessel_system_tables(void)
{
    const char *tables[] = {
        "ship_interiors",
        "ship_docking",
        "ship_room_templates",
        "ship_cargo_manifest",
        "ship_crew_roster"
    };
    int num_tables = sizeof(tables) / sizeof(tables[0]);
    char query[1024];
    int i;

    for (i = 0; i < num_tables; i++) {
        snprintf(query, sizeof(query), "SHOW TABLES LIKE '%s'", tables[i]);

        if (mysql_query_safe(conn, query)) {
            log("SYSERR: Error checking table %s: %s", tables[i], mysql_error(conn));
            return FALSE;
        }

        MYSQL_RES *result = mysql_store_result_safe(conn);
        if (!result || mysql_num_rows(result) == 0) {
            log("ERROR: Vessel system table '%s' does not exist", tables[i]);
            if (result) mysql_free_result(result);
            return FALSE;
        }
        mysql_free_result(result);
    }

    return TRUE;
}

/* ===== ADMINISTRATIVE COMMAND UTILITIES ===== */

/* Show comprehensive database status */
void show_database_status(struct char_data *ch)
{
    if (!ch) return;

    send_to_char(ch, "\tcLuminariMUD Database Status Report\tn\r\n");
    send_to_char(ch, "=====================================\r\n");
    
    if (!is_database_available()) {
        send_to_char(ch, "\trDatabase Connection: UNAVAILABLE\tn\r\n");
        send_to_char(ch, "\tyThe database is not currently accessible.\tn\r\n");
        return;
    }
    
    send_to_char(ch, "\tgDatabase Connection: ACTIVE\tn\r\n");
    
    /* Test permissions */
    if (test_database_permissions()) {
        send_to_char(ch, "\tgDatabase Permissions: SUFFICIENT\tn\r\n");
    } else {
        send_to_char(ch, "\trDatabase Permissions: INSUFFICIENT\tn\r\n");
    }
    
    /* Test integrity */
    if (verify_database_integrity()) {
        send_to_char(ch, "\tgDatabase Integrity: VERIFIED\tn\r\n");
    } else {
        send_to_char(ch, "\tyDatabase Integrity: ISSUES DETECTED\tn\r\n");
    }
    
    send_to_char(ch, "\r\n\tcType 'database info' for detailed system information.\tn\r\n");
}
