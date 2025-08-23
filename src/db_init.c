/* *************************************************************************
 *   File: db_init.c                               Part of LuminariMUD *
 *  Usage: Database initialization system implementation                 *
 * Author: Database Infrastructure Team                                  *
 ***************************************************************************
 * Comprehensive database initialization system for LuminariMUD         *
 * Handles creation of all database tables and population of reference  *
 * data to ensure a complete, working database setup.                   *
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

/* ===== MAIN INITIALIZATION FUNCTIONS ===== */

/* Master database initialization function */
void init_luminari_database(void)
{
    if (!is_database_available()) {
        log("SYSERR: Database not available - skipping initialization");
        return;
    }

    log("Starting comprehensive LuminariMUD database initialization...");

    /* Initialize all subsystems */
    init_core_player_tables();
    init_object_database_tables();
    init_wilderness_resource_tables();
    init_region_system_tables();
    init_region_hints_tables();
    init_ai_service_tables();
    init_crafting_system_tables();
    init_housing_system_tables();
    init_help_system_tables();

    /* Create database procedures and functions */
    create_database_procedures();

    /* Populate standard reference data */
    populate_resource_types_data();
    populate_material_categories_data();
    populate_material_qualities_data();
    populate_region_effects_data();
    populate_region_system_data();
    populate_ai_config_data();

    /* Verify everything was created correctly */
    if (verify_database_integrity()) {
        log("SUCCESS: LuminariMUD database initialization completed successfully!");
        log("Info: All tables created and reference data populated.");
    } else {
        log("ERROR: Database initialization completed with errors - check logs above");
    }
}

/* ===== CORE PLAYER SYSTEM TABLES ===== */

void init_core_player_tables(void)
{
    if (!mysql_available || !conn) {
        log("MySQL not available, skipping core player tables initialization");
        return;
    }

    log("Initializing core player system tables...");

    /* player_data - Core player character information */
    const char *create_player_data = 
        "CREATE TABLE IF NOT EXISTS player_data ("
        "id INT AUTO_INCREMENT PRIMARY KEY, "
        "name VARCHAR(20) UNIQUE NOT NULL, "
        "password VARCHAR(32) NOT NULL, "
        "email VARCHAR(100), "
        "level INT DEFAULT 1, "
        "experience BIGINT DEFAULT 0, "
        "class INT DEFAULT 0, "
        "race INT DEFAULT 0, "
        "alignment INT DEFAULT 0, "
        "last_online TIMESTAMP DEFAULT CURRENT_TIMESTAMP, "
        "created TIMESTAMP DEFAULT CURRENT_TIMESTAMP, "
        "total_sessions INT DEFAULT 0, "
        "bad_pws INT DEFAULT 0, "
        "obj_save_header VARCHAR(255) DEFAULT '', "
        "INDEX idx_name (name), "
        "INDEX idx_level (level), "
        "INDEX idx_last_online (last_online)"
        ")";

    if (mysql_query_safe(conn, create_player_data)) {
        log("SYSERR: Failed to create player_data table: %s", mysql_error(conn));
        return;
    }

    /* account_data - Account management system */
    const char *create_account_data = 
        "CREATE TABLE IF NOT EXISTS account_data ("
        "id INT AUTO_INCREMENT PRIMARY KEY, "
        "name VARCHAR(50) UNIQUE NOT NULL, "
        "password VARCHAR(64) NOT NULL, "
        "email VARCHAR(255), "
        "experience INT DEFAULT 0, "
        "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP, "
        "last_login TIMESTAMP DEFAULT CURRENT_TIMESTAMP, "
        "status ENUM('active', 'suspended', 'banned') DEFAULT 'active', "
        "INDEX idx_name (name), "
        "INDEX idx_email (email), "
        "INDEX idx_status (status)"
        ")";

    if (mysql_query_safe(conn, create_account_data)) {
        log("SYSERR: Failed to create account_data table: %s", mysql_error(conn));
        return;
    }

    /* unlocked_races - Player account unlocked races */
    const char *create_unlocked_races = 
        "CREATE TABLE IF NOT EXISTS unlocked_races ("
        "id INT AUTO_INCREMENT PRIMARY KEY, "
        "account_id INT NOT NULL, "
        "race_id INT NOT NULL, "
        "unlocked_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP, "
        "UNIQUE KEY unique_account_race (account_id, race_id), "
        "FOREIGN KEY (account_id) REFERENCES account_data(id) ON DELETE CASCADE, "
        "INDEX idx_account (account_id)"
        ")";

    if (mysql_query_safe(conn, create_unlocked_races)) {
        log("SYSERR: Failed to create unlocked_races table: %s", mysql_error(conn));
        return;
    }

    /* unlocked_classes - Player account unlocked classes */
    const char *create_unlocked_classes = 
        "CREATE TABLE IF NOT EXISTS unlocked_classes ("
        "id INT AUTO_INCREMENT PRIMARY KEY, "
        "account_id INT NOT NULL, "
        "class_id INT NOT NULL, "
        "unlocked_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP, "
        "UNIQUE KEY unique_account_class (account_id, class_id), "
        "FOREIGN KEY (account_id) REFERENCES account_data(id) ON DELETE CASCADE, "
        "INDEX idx_account (account_id)"
        ")";

    if (mysql_query_safe(conn, create_unlocked_classes)) {
        log("SYSERR: Failed to create unlocked_classes table: %s", mysql_error(conn));
        return;
    }

    /* player_save_objs - Player inventory and equipment saves */
    const char *create_player_save_objs = 
        "CREATE TABLE IF NOT EXISTS player_save_objs ("
        "id INT AUTO_INCREMENT PRIMARY KEY, "
        "name VARCHAR(20) NOT NULL, "
        "serialized_obj LONGTEXT NOT NULL, "
        "creation_date TIMESTAMP DEFAULT CURRENT_TIMESTAMP, "
        "INDEX idx_name (name), "
        "INDEX idx_creation_date (creation_date)"
        ")";

    if (mysql_query_safe(conn, create_player_save_objs)) {
        log("SYSERR: Failed to create player_save_objs table: %s", mysql_error(conn));
        return;
    }

    /* player_save_objs_sheathed - Sheathed weapon saves */
    const char *create_player_save_objs_sheathed = 
        "CREATE TABLE IF NOT EXISTS player_save_objs_sheathed ("
        "id INT AUTO_INCREMENT PRIMARY KEY, "
        "sheath_obj_id BIGINT NOT NULL, "
        "sheathed_position INT NOT NULL, "
        "owner_name VARCHAR(20) NOT NULL, "
        "serialized_obj LONGTEXT NOT NULL, "
        "creation_date TIMESTAMP DEFAULT CURRENT_TIMESTAMP, "
        "INDEX idx_owner_name (owner_name), "
        "INDEX idx_sheath_obj_id (sheath_obj_id)"
        ")";

    if (mysql_query_safe(conn, create_player_save_objs_sheathed)) {
        log("SYSERR: Failed to create player_save_objs_sheathed table: %s", mysql_error(conn));
        return;
    }

    /* pet_save_objs - Pet object saves */
    const char *create_pet_save_objs = 
        "CREATE TABLE IF NOT EXISTS pet_save_objs ("
        "id INT AUTO_INCREMENT PRIMARY KEY, "
        "pet_idnum BIGINT NOT NULL, "
        "owner_name VARCHAR(20) NOT NULL, "
        "serialized_obj LONGTEXT NOT NULL, "
        "creation_date TIMESTAMP DEFAULT CURRENT_TIMESTAMP, "
        "INDEX idx_pet_idnum (pet_idnum), "
        "INDEX idx_owner_name (owner_name)"
        ")";

    if (mysql_query_safe(conn, create_pet_save_objs)) {
        log("SYSERR: Failed to create pet_save_objs table: %s", mysql_error(conn));
        return;
    }

    log("Info: Core player system tables initialized successfully");
}

/* ===== OBJECT DATABASE SYSTEM TABLES ===== */

void init_object_database_tables(void)
{
    if (!mysql_available || !conn) {
        log("MySQL not available, skipping object database tables initialization");
        return;
    }

    log("Initializing object database system tables...");

    /* object_database_items - Items exported from the MUD */
    const char *create_object_items = 
        "CREATE TABLE IF NOT EXISTS object_database_items ("
        "id INT AUTO_INCREMENT PRIMARY KEY, "
        "idnum INT UNIQUE NOT NULL, "
        "name VARCHAR(255) NOT NULL, "
        "short_description TEXT, "
        "long_description TEXT, "
        "item_type INT DEFAULT 0, "
        "wear_flags BIGINT DEFAULT 0, "
        "extra_flags BIGINT DEFAULT 0, "
        "weight FLOAT DEFAULT 0.0, "
        "cost INT DEFAULT 0, "
        "rent_per_day INT DEFAULT 0, "
        "level_restriction INT DEFAULT 1, "
        "size INT DEFAULT 0, "
        "material INT DEFAULT 0, "
        "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP, "
        "updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP, "
        "INDEX idx_idnum (idnum), "
        "INDEX idx_item_type (item_type), "
        "INDEX idx_level_restriction (level_restriction)"
        ")";

    if (mysql_query_safe(conn, create_object_items)) {
        log("SYSERR: Failed to create object_database_items table: %s", mysql_error(conn));
        return;
    }

    /* object_database_wear_slots - Wearable item slots */
    const char *create_object_wear_slots = 
        "CREATE TABLE IF NOT EXISTS object_database_wear_slots ("
        "id INT AUTO_INCREMENT PRIMARY KEY, "
        "object_idnum INT NOT NULL, "
        "wear_slot INT NOT NULL, "
        "UNIQUE KEY unique_object_slot (object_idnum, wear_slot), "
        "FOREIGN KEY (object_idnum) REFERENCES object_database_items(idnum) ON DELETE CASCADE, "
        "INDEX idx_wear_slot (wear_slot)"
        ")";

    if (mysql_query_safe(conn, create_object_wear_slots)) {
        log("SYSERR: Failed to create object_database_wear_slots table: %s", mysql_error(conn));
        return;
    }

    /* object_database_bonuses - Item stat bonuses */
    const char *create_object_bonuses = 
        "CREATE TABLE IF NOT EXISTS object_database_bonuses ("
        "id INT AUTO_INCREMENT PRIMARY KEY, "
        "object_idnum INT NOT NULL, "
        "bonus_location INT NOT NULL, "
        "bonus_value INT NOT NULL, "
        "FOREIGN KEY (object_idnum) REFERENCES object_database_items(idnum) ON DELETE CASCADE, "
        "INDEX idx_object_idnum (object_idnum), "
        "INDEX idx_bonus_location (bonus_location)"
        ")";

    if (mysql_query_safe(conn, create_object_bonuses)) {
        log("SYSERR: Failed to create object_database_bonuses table: %s", mysql_error(conn));
        return;
    }

    log("Info: Object database system tables initialized successfully");
}

/* ===== WILDERNESS RESOURCE SYSTEM TABLES ===== */

void init_wilderness_resource_tables(void)
{
    if (!mysql_available || !conn) {
        log("MySQL not available, skipping wilderness resource tables initialization");
        return;
    }

    log("Initializing wilderness resource system tables...");

    /* resource_types - Resource type definitions (populated by separate function) */
    const char *create_resource_types = 
        "CREATE TABLE IF NOT EXISTS resource_types ("
        "resource_id INT PRIMARY KEY, "
        "resource_name VARCHAR(50) NOT NULL UNIQUE, "
        "resource_description TEXT, "
        "base_rarity DECIMAL(3,2) DEFAULT 1.00, "
        "regeneration_rate DECIMAL(4,3) DEFAULT 0.001, "
        "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP"
        ")";

    if (mysql_query_safe(conn, create_resource_types)) {
        log("SYSERR: Failed to create resource_types table: %s", mysql_error(conn));
        return;
    }

    /* resource_depletion - Location-based resource depletion tracking */
    const char *create_resource_depletion = 
        "CREATE TABLE IF NOT EXISTS resource_depletion ("
        "id INT AUTO_INCREMENT PRIMARY KEY, "
        "zone_vnum INT NOT NULL, "
        "x_coord INT NOT NULL, "
        "y_coord INT NOT NULL, "
        "resource_type INT NOT NULL, "
        "depletion_level FLOAT DEFAULT 1.0, "
        "total_harvested INT DEFAULT 0, "
        "last_harvest TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP, "
        "cascade_effects TEXT DEFAULT NULL, "
        "regeneration_rate DECIMAL(5,4) DEFAULT 0.0010, "
        "last_regeneration TIMESTAMP DEFAULT CURRENT_TIMESTAMP, "
        "INDEX idx_location (zone_vnum, x_coord, y_coord), "
        "INDEX idx_resource (resource_type), "
        "INDEX idx_depletion (depletion_level), "
        "INDEX idx_last_harvest (last_harvest), "
        "UNIQUE KEY unique_location_resource (zone_vnum, x_coord, y_coord, resource_type), "
        "FOREIGN KEY (resource_type) REFERENCES resource_types(resource_id) ON DELETE CASCADE"
        ")";

    if (mysql_query_safe(conn, create_resource_depletion)) {
        log("SYSERR: Failed to create resource_depletion table: %s", mysql_error(conn));
        return;
    }

    /* player_conservation - Player environmental stewardship performance */
    const char *create_player_conservation = 
        "CREATE TABLE IF NOT EXISTS player_conservation ("
        "id INT AUTO_INCREMENT PRIMARY KEY, "
        "player_id BIGINT NOT NULL, "
        "conservation_score FLOAT DEFAULT 0.5, "
        "total_harvests INT DEFAULT 0, "
        "sustainable_harvests INT DEFAULT 0, "
        "unsustainable_harvests INT DEFAULT 0, "
        "ecosystem_damage DECIMAL(8,3) DEFAULT 0.0, "
        "last_updated TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP, "
        "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP, "
        "INDEX idx_player_id (player_id), "
        "INDEX idx_conservation_score (conservation_score), "
        "UNIQUE KEY unique_player (player_id)"
        ")";

    if (mysql_query_safe(conn, create_player_conservation)) {
        log("SYSERR: Failed to create player_conservation table: %s", mysql_error(conn));
        return;
    }

    /* resource_statistics - Global resource usage patterns */
    const char *create_resource_statistics = 
        "CREATE TABLE IF NOT EXISTS resource_statistics ("
        "id INT AUTO_INCREMENT PRIMARY KEY, "
        "resource_type INT NOT NULL, "
        "total_harvested BIGINT DEFAULT 0, "
        "total_depleted_locations INT DEFAULT 0, "
        "average_depletion_level FLOAT DEFAULT 1.0, "
        "peak_depletion_level FLOAT DEFAULT 1.0, "
        "critical_locations INT DEFAULT 0, "
        "last_updated TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP, "
        "UNIQUE KEY unique_resource (resource_type), "
        "FOREIGN KEY (resource_type) REFERENCES resource_types(resource_id) ON DELETE CASCADE"
        ")";

    if (mysql_query_safe(conn, create_resource_statistics)) {
        log("SYSERR: Failed to create resource_statistics table: %s", mysql_error(conn));
        return;
    }

    /* resource_regeneration_log - Regeneration tracking */
    const char *create_resource_regeneration_log = 
        "CREATE TABLE IF NOT EXISTS resource_regeneration_log ("
        "id INT AUTO_INCREMENT PRIMARY KEY, "
        "zone_vnum INT NOT NULL, "
        "x_coord INT NOT NULL, "
        "y_coord INT NOT NULL, "
        "resource_type INT NOT NULL, "
        "old_depletion_level FLOAT NOT NULL, "
        "new_depletion_level FLOAT NOT NULL, "
        "regeneration_amount FLOAT NOT NULL, "
        "regeneration_type ENUM('natural', 'seasonal', 'magical', 'admin') DEFAULT 'natural', "
        "regeneration_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP, "
        "weather_factor FLOAT DEFAULT 1.0, "
        "seasonal_factor FLOAT DEFAULT 1.0, "
        "INDEX idx_location_regen (zone_vnum, x_coord, y_coord), "
        "INDEX idx_time_regen (regeneration_time), "
        "INDEX idx_resource_type (resource_type), "
        "FOREIGN KEY (resource_type) REFERENCES resource_types(resource_id) ON DELETE CASCADE"
        ")";

    if (mysql_query_safe(conn, create_resource_regeneration_log)) {
        log("SYSERR: Failed to create resource_regeneration_log table: %s", mysql_error(conn));
        return;
    }

    /* resource_relationships - Ecological interdependencies */
    const char *create_resource_relationships = 
        "CREATE TABLE IF NOT EXISTS resource_relationships ("
        "id INT AUTO_INCREMENT PRIMARY KEY, "
        "source_resource INT NOT NULL, "
        "target_resource INT NOT NULL, "
        "effect_type ENUM('depletion', 'enhancement', 'threshold') NOT NULL, "
        "effect_magnitude DECIMAL(5,3) NOT NULL, "
        "threshold_min DECIMAL(4,3) DEFAULT NULL, "
        "threshold_max DECIMAL(4,3) DEFAULT NULL, "
        "description VARCHAR(255), "
        "is_active BOOLEAN DEFAULT TRUE, "
        "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP, "
        "INDEX idx_source_resource (source_resource), "
        "INDEX idx_target_resource (target_resource), "
        "INDEX idx_effect_type (effect_type), "
        "INDEX idx_is_active (is_active), "
        "FOREIGN KEY (source_resource) REFERENCES resource_types(resource_id) ON DELETE CASCADE, "
        "FOREIGN KEY (target_resource) REFERENCES resource_types(resource_id) ON DELETE CASCADE"
        ")";

    if (mysql_query_safe(conn, create_resource_relationships)) {
        log("SYSERR: Failed to create resource_relationships table: %s", mysql_error(conn));
        return;
    }

    /* ecosystem_health - Ecosystem state tracking */
    const char *create_ecosystem_health = 
        "CREATE TABLE IF NOT EXISTS ecosystem_health ("
        "id INT AUTO_INCREMENT PRIMARY KEY, "
        "zone_vnum INT NOT NULL, "
        "x_coord INT NOT NULL, "
        "y_coord INT NOT NULL, "
        "health_score DECIMAL(4,3) DEFAULT 1.000, "
        "health_state ENUM('pristine', 'healthy', 'degraded', 'critical', 'devastated') DEFAULT 'healthy', "
        "biodiversity_index DECIMAL(4,3) DEFAULT 1.000, "
        "pollution_level DECIMAL(4,3) DEFAULT 0.000, "
        "last_assessment TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP, "
        "UNIQUE KEY unique_location (zone_vnum, x_coord, y_coord), "
        "INDEX idx_health_state (health_state), "
        "INDEX idx_health_score (health_score), "
        "INDEX idx_zone (zone_vnum)"
        ")";

    if (mysql_query_safe(conn, create_ecosystem_health)) {
        log("SYSERR: Failed to create ecosystem_health table: %s", mysql_error(conn));
        return;
    }

    /* cascade_effects_log - Environmental cascade effects history */
    const char *create_cascade_effects_log = 
        "CREATE TABLE IF NOT EXISTS cascade_effects_log ("
        "id INT PRIMARY KEY AUTO_INCREMENT, "
        "zone_vnum INT NOT NULL, "
        "x_coord INT NOT NULL, "
        "y_coord INT NOT NULL, "
        "source_resource INT NOT NULL, "
        "target_resource INT NOT NULL, "
        "effect_magnitude DECIMAL(5,3) NOT NULL, "
        "player_id BIGINT DEFAULT NULL, "
        "harvest_amount INT DEFAULT 0, "
        "logged_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP, "
        "INDEX idx_location_time (zone_vnum, x_coord, y_coord, logged_at), "
        "INDEX idx_resources (source_resource, target_resource), "
        "INDEX idx_player (player_id), "
        "FOREIGN KEY (source_resource) REFERENCES resource_types(resource_id) ON DELETE CASCADE, "
        "FOREIGN KEY (target_resource) REFERENCES resource_types(resource_id) ON DELETE CASCADE"
        ")";

    if (mysql_query_safe(conn, create_cascade_effects_log)) {
        log("SYSERR: Failed to create cascade_effects_log table: %s", mysql_error(conn));
        return;
    }

    /* player_location_conservation - Location-specific conservation tracking */
    const char *create_player_location_conservation = 
        "CREATE TABLE IF NOT EXISTS player_location_conservation ("
        "id INT AUTO_INCREMENT PRIMARY KEY, "
        "player_id BIGINT NOT NULL, "
        "zone_vnum INT NOT NULL, "
        "x_coord INT NOT NULL, "
        "y_coord INT NOT NULL, "
        "visits INT DEFAULT 0, "
        "total_harvested INT DEFAULT 0, "
        "sustainable_actions INT DEFAULT 0, "
        "destructive_actions INT DEFAULT 0, "
        "last_visit TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP, "
        "conservation_rating DECIMAL(4,3) DEFAULT 0.500, "
        "UNIQUE KEY unique_player_location (player_id, zone_vnum, x_coord, y_coord), "
        "INDEX idx_player_id (player_id), "
        "INDEX idx_location (zone_vnum, x_coord, y_coord), "
        "INDEX idx_conservation_rating (conservation_rating)"
        ")";

    if (mysql_query_safe(conn, create_player_location_conservation)) {
        log("SYSERR: Failed to create player_location_conservation table: %s", mysql_error(conn));
        return;
    }

    /* region_effects - Regional environmental effects */
    const char *create_region_effects = 
        "CREATE TABLE IF NOT EXISTS region_effects ("
        "effect_id INT AUTO_INCREMENT PRIMARY KEY, "
        "effect_name VARCHAR(100) NOT NULL, "
        "effect_type ENUM('weather', 'magical', 'seasonal', 'geological', 'biological') NOT NULL, "
        "effect_description TEXT, "
        "effect_data JSON DEFAULT NULL, "
        "intensity_min DECIMAL(3,2) DEFAULT 0.10, "
        "intensity_max DECIMAL(3,2) DEFAULT 1.00, "
        "duration_hours INT DEFAULT 24, "
        "is_active BOOLEAN DEFAULT TRUE, "
        "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP, "
        "INDEX idx_effect_type (effect_type), "
        "INDEX idx_is_active (is_active)"
        ")";

    if (mysql_query_safe(conn, create_region_effects)) {
        log("SYSERR: Failed to create region_effects table: %s", mysql_error(conn));
        return;
    }

    /* region_effect_assignments - Region-effect mappings */
    const char *create_region_effect_assignments = 
        "CREATE TABLE IF NOT EXISTS region_effect_assignments ("
        "id INT AUTO_INCREMENT PRIMARY KEY, "
        "region_vnum INT NOT NULL, "
        "effect_id INT NOT NULL, "
        "intensity DECIMAL(3,2) DEFAULT 1.00, "
        "start_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP, "
        "end_time TIMESTAMP NULL, "
        "is_permanent BOOLEAN DEFAULT FALSE, "
        "assigned_by VARCHAR(50) DEFAULT 'system', "
        "FOREIGN KEY (effect_id) REFERENCES region_effects(effect_id) ON DELETE CASCADE, "
        "INDEX idx_region_vnum (region_vnum), "
        "INDEX idx_effect_id (effect_id), "
        "INDEX idx_active_assignments (region_vnum, end_time)"
        ")";

    if (mysql_query_safe(conn, create_region_effect_assignments)) {
        log("SYSERR: Failed to create region_effect_assignments table: %s", mysql_error(conn));
        return;
    }

    /* weather_cache - Weather data performance cache */
    const char *create_weather_cache = 
        "CREATE TABLE IF NOT EXISTS weather_cache ("
        "id INT AUTO_INCREMENT PRIMARY KEY, "
        "zone_vnum INT NOT NULL, "
        "x_coord INT NOT NULL, "
        "y_coord INT NOT NULL, "
        "weather_type INT NOT NULL, "
        "temperature INT DEFAULT 20, "
        "humidity DECIMAL(3,2) DEFAULT 0.50, "
        "wind_speed INT DEFAULT 5, "
        "cached_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP, "
        "expires_at TIMESTAMP DEFAULT (CURRENT_TIMESTAMP + INTERVAL 1 HOUR), "
        "UNIQUE KEY unique_location_weather (zone_vnum, x_coord, y_coord), "
        "INDEX idx_expires_at (expires_at)"
        ")";

    if (mysql_query_safe(conn, create_weather_cache)) {
        log("SYSERR: Failed to create weather_cache table: %s", mysql_error(conn));
        return;
    }

    /* room_description_settings - Per-room customization options */
    const char *create_room_description_settings = 
        "CREATE TABLE IF NOT EXISTS room_description_settings ("
        "id INT AUTO_INCREMENT PRIMARY KEY, "
        "zone_vnum INT NOT NULL, "
        "x_coord INT NOT NULL, "
        "y_coord INT NOT NULL, "
        "enable_dynamic_descriptions BOOLEAN DEFAULT TRUE, "
        "enable_weather_effects BOOLEAN DEFAULT TRUE, "
        "enable_resource_descriptions BOOLEAN DEFAULT TRUE, "
        "enable_seasonal_changes BOOLEAN DEFAULT TRUE, "
        "custom_description_override TEXT DEFAULT NULL, "
        "last_updated TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP, "
        "UNIQUE KEY unique_location_settings (zone_vnum, x_coord, y_coord), "
        "INDEX idx_zone_vnum (zone_vnum)"
        ")";

    if (mysql_query_safe(conn, create_room_description_settings)) {
        log("SYSERR: Failed to create room_description_settings table: %s", mysql_error(conn));
        return;
    }

    /* material_categories - Material classification */
    const char *create_material_categories = 
        "CREATE TABLE IF NOT EXISTS material_categories ("
        "category_id INT PRIMARY KEY, "
        "category_name VARCHAR(50) NOT NULL UNIQUE, "
        "category_description TEXT, "
        "resource_type INT, "
        "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP, "
        "FOREIGN KEY (resource_type) REFERENCES resource_types(resource_id) ON DELETE SET NULL"
        ")";

    if (mysql_query_safe(conn, create_material_categories)) {
        log("SYSERR: Failed to create material_categories table: %s", mysql_error(conn));
        return;
    }

    /* material_subtypes - Specific material types */
    const char *create_material_subtypes = 
        "CREATE TABLE IF NOT EXISTS material_subtypes ("
        "subtype_id INT AUTO_INCREMENT PRIMARY KEY, "
        "category_id INT NOT NULL, "
        "subtype_name VARCHAR(50) NOT NULL, "
        "subtype_description TEXT, "
        "rarity_modifier DECIMAL(3,2) DEFAULT 1.00, "
        "value_modifier DECIMAL(3,2) DEFAULT 1.00, "
        "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP, "
        "UNIQUE KEY unique_category_subtype (category_id, subtype_name), "
        "FOREIGN KEY (category_id) REFERENCES material_categories(category_id) ON DELETE CASCADE, "
        "INDEX idx_category_id (category_id)"
        ")";

    if (mysql_query_safe(conn, create_material_subtypes)) {
        log("SYSERR: Failed to create material_subtypes table: %s", mysql_error(conn));
        return;
    }

    /* material_qualities - Quality levels */
    const char *create_material_qualities = 
        "CREATE TABLE IF NOT EXISTS material_qualities ("
        "quality_id INT PRIMARY KEY, "
        "quality_name VARCHAR(30) NOT NULL UNIQUE, "
        "quality_description TEXT, "
        "rarity_multiplier DECIMAL(4,2) DEFAULT 1.00, "
        "value_multiplier DECIMAL(4,2) DEFAULT 1.00, "
        "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP"
        ")";

    if (mysql_query_safe(conn, create_material_qualities)) {
        log("SYSERR: Failed to create material_qualities table: %s", mysql_error(conn));
        return;
    }

    log("Info: Wilderness resource system tables initialized successfully");
}

/* ===== REGION SYSTEM TABLES ===== */

void init_region_system_tables(void)
{
    if (!mysql_available || !conn) {
        log("MySQL not available, skipping region system tables initialization");
        return;
    }

    log("Initializing region system tables...");

    /* region_data - Main region definitions with spatial geometry */
    const char *create_region_data = 
        "CREATE TABLE IF NOT EXISTS region_data ("
        "vnum INT PRIMARY KEY, "
        "zone_vnum INT NOT NULL, "
        "name VARCHAR(50), "
        "region_type INT NOT NULL, "
        "region_polygon POLYGON, "
        "region_props INT, "
        "region_reset_data VARCHAR(255) NOT NULL, "
        "region_reset_time DATETIME, "
        "INDEX idx_vnum (vnum), "
        "INDEX idx_zone_vnum (zone_vnum), "
        "INDEX idx_region_type (region_type)"
        ")";

    if (mysql_query_safe(conn, create_region_data)) {
        log("SYSERR: Failed to create region_data table: %s", mysql_error(conn));
        return;
    }

    /* path_data - Path definitions with linestring geometry */
    const char *create_path_data = 
        "CREATE TABLE IF NOT EXISTS path_data ("
        "vnum INT PRIMARY KEY, "
        "zone_vnum INT NOT NULL DEFAULT 10000, "
        "path_type INT NOT NULL, "
        "name VARCHAR(50) NOT NULL, "
        "path_props INT, "
        "path_linestring LINESTRING, "
        "INDEX idx_vnum (vnum), "
        "INDEX idx_zone_vnum (zone_vnum), "
        "INDEX idx_path_type (path_type)"
        ")";

    if (mysql_query_safe(conn, create_path_data)) {
        log("SYSERR: Failed to create path_data table: %s", mysql_error(conn));
        return;
    }

    /* region_index - Optimized spatial index for region queries */
    const char *create_region_index = 
        "CREATE TABLE IF NOT EXISTS region_index ("
        "vnum INT PRIMARY KEY, "
        "zone_vnum INT NOT NULL, "
        "region_polygon POLYGON NOT NULL, "
        "SPATIAL INDEX region_polygon (region_polygon)"
        ")";

    if (mysql_query_safe(conn, create_region_index)) {
        log("SYSERR: Failed to create region_index table: %s", mysql_error(conn));
        return;
    }

    /* path_index - Optimized spatial index for path queries */
    const char *create_path_index = 
        "CREATE TABLE IF NOT EXISTS path_index ("
        "vnum INT PRIMARY KEY, "
        "zone_vnum INT NOT NULL, "
        "path_linestring LINESTRING NOT NULL, "
        "SPATIAL INDEX path_linestring (path_linestring)"
        ")";

    if (mysql_query_safe(conn, create_path_index)) {
        log("SYSERR: Failed to create path_index table: %s", mysql_error(conn));
        return;
    }

    /* Update region_data table with AI description fields if they don't exist */
    const char *add_region_description_fields = 
        "ALTER TABLE region_data "
        "ADD COLUMN IF NOT EXISTS region_description LONGTEXT DEFAULT NULL, "
        "ADD COLUMN IF NOT EXISTS description_version INT DEFAULT 1, "
        "ADD COLUMN IF NOT EXISTS ai_agent_source VARCHAR(100) DEFAULT NULL, "
        "ADD COLUMN IF NOT EXISTS last_description_update TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP, "
        "ADD COLUMN IF NOT EXISTS description_style ENUM('poetic', 'practical', 'mysterious', 'dramatic', 'pastoral') DEFAULT 'poetic', "
        "ADD COLUMN IF NOT EXISTS description_length ENUM('brief', 'moderate', 'detailed', 'extensive') DEFAULT 'moderate', "
        "ADD COLUMN IF NOT EXISTS has_historical_context BOOLEAN DEFAULT FALSE, "
        "ADD COLUMN IF NOT EXISTS has_resource_info BOOLEAN DEFAULT FALSE, "
        "ADD COLUMN IF NOT EXISTS has_wildlife_info BOOLEAN DEFAULT FALSE, "
        "ADD COLUMN IF NOT EXISTS has_geological_info BOOLEAN DEFAULT FALSE, "
        "ADD COLUMN IF NOT EXISTS has_cultural_info BOOLEAN DEFAULT FALSE, "
        "ADD COLUMN IF NOT EXISTS description_quality_score DECIMAL(3,2) DEFAULT NULL, "
        "ADD COLUMN IF NOT EXISTS requires_review BOOLEAN DEFAULT FALSE, "
        "ADD COLUMN IF NOT EXISTS is_approved BOOLEAN DEFAULT FALSE";

    if (mysql_query_safe(conn, add_region_description_fields)) {
        log("SYSERR: Failed to add region description fields: %s", mysql_error(conn));
        /* Don't return - continue with other initializations */
    } else {
        log("Info: Region description fields added/verified successfully");
    }

    /* Add indexes for region description fields */
    const char *add_region_description_indexes = 
        "CREATE INDEX IF NOT EXISTS idx_region_has_description ON region_data (region_description(100)); "
        "CREATE INDEX IF NOT EXISTS idx_region_description_approved ON region_data (is_approved, requires_review); "
        "CREATE INDEX IF NOT EXISTS idx_region_description_quality ON region_data (description_quality_score); "
        "CREATE INDEX IF NOT EXISTS idx_region_ai_source ON region_data (ai_agent_source)";

    if (mysql_query_safe(conn, add_region_description_indexes)) {
        log("SYSERR: Failed to create region description indexes: %s", mysql_error(conn));
        /* Don't return - continue with other initializations */
    } else {
        log("Info: Region description indexes created successfully");
    }

    log("Info: Region system tables initialized successfully");
}

/* ===== AI REGION HINTS SYSTEM TABLES ===== */

void init_region_hints_tables(void)
{
    if (!mysql_available || !conn) {
        log("MySQL not available, skipping region hints tables initialization");
        return;
    }

    log("Initializing region hints system tables...");

    /* region_hints - Main table for region hints */
    const char *create_region_hints = 
        "CREATE TABLE IF NOT EXISTS region_hints ("
        "id INT AUTO_INCREMENT PRIMARY KEY, "
        "region_vnum INT NOT NULL, "
        "hint_category ENUM("
            "'atmosphere', 'fauna', 'flora', 'geography', 'weather_influence', "
            "'resources', 'landmarks', 'sounds', 'scents', 'seasonal_changes', "
            "'time_of_day', 'mystical'"
        ") NOT NULL, "
        "hint_text TEXT NOT NULL, "
        "priority TINYINT DEFAULT 5, "
        "seasonal_weight JSON DEFAULT NULL, "
        "weather_conditions SET('clear', 'cloudy', 'rainy', 'stormy', 'lightning') "
            "DEFAULT 'clear,cloudy,rainy,stormy,lightning', "
        "time_of_day_weight JSON DEFAULT NULL, "
        "resource_triggers JSON DEFAULT NULL, "
        "agent_id VARCHAR(100) DEFAULT NULL, "
        "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP, "
        "updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP, "
        "is_active BOOLEAN DEFAULT TRUE, "
        "INDEX idx_region_category (region_vnum, hint_category), "
        "INDEX idx_priority (priority), "
        "INDEX idx_active (is_active), "
        "INDEX idx_created (created_at)"
        ")";

    if (mysql_query_safe(conn, create_region_hints)) {
        log("SYSERR: Failed to create region_hints table: %s", mysql_error(conn));
        return;
    }

    /* region_profiles - Overall region personality profiles */
    const char *create_region_profiles = 
        "CREATE TABLE IF NOT EXISTS region_profiles ("
        "region_vnum INT PRIMARY KEY, "
        "overall_theme TEXT, "
        "dominant_mood VARCHAR(100), "
        "key_characteristics JSON, "
        "description_style ENUM('poetic', 'practical', 'mysterious', 'dramatic', 'pastoral') DEFAULT 'poetic', "
        "complexity_level TINYINT DEFAULT 3, "
        "agent_id VARCHAR(100), "
        "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP, "
        "updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP"
        ")";

    if (mysql_query_safe(conn, create_region_profiles)) {
        log("SYSERR: Failed to create region_profiles table: %s", mysql_error(conn));
        return;
    }

    /* hint_usage_log - Track which hints are actually used for analytics */
    const char *create_hint_usage_log = 
        "CREATE TABLE IF NOT EXISTS hint_usage_log ("
        "id INT AUTO_INCREMENT PRIMARY KEY, "
        "hint_id INT NOT NULL, "
        "room_vnum INT NOT NULL, "
        "player_id INT DEFAULT NULL, "
        "used_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP, "
        "weather_condition VARCHAR(20), "
        "season VARCHAR(10), "
        "time_of_day VARCHAR(10), "
        "resource_state JSON DEFAULT NULL, "
        "FOREIGN KEY (hint_id) REFERENCES region_hints(id) ON DELETE CASCADE, "
        "INDEX idx_hint_usage (hint_id, used_at), "
        "INDEX idx_room_usage (room_vnum, used_at)"
        ")";

    if (mysql_query_safe(conn, create_hint_usage_log)) {
        log("SYSERR: Failed to create hint_usage_log table: %s", mysql_error(conn));
        return;
    }

    /* description_templates - Description templates (future enhancement) */
    const char *create_description_templates = 
        "CREATE TABLE IF NOT EXISTS description_templates ("
        "id INT AUTO_INCREMENT PRIMARY KEY, "
        "region_vnum INT NOT NULL, "
        "template_type ENUM('intro', 'weather_overlay', 'resource_state', 'time_transition') NOT NULL, "
        "template_text TEXT NOT NULL, "
        "placeholder_schema JSON, "
        "conditions JSON DEFAULT NULL, "
        "agent_id VARCHAR(100), "
        "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP, "
        "is_active BOOLEAN DEFAULT TRUE, "
        "INDEX idx_region_type (region_vnum, template_type), "
        "INDEX idx_active_templates (is_active)"
        ")";

    if (mysql_query_safe(conn, create_description_templates)) {
        log("SYSERR: Failed to create description_templates table: %s", mysql_error(conn));
        return;
    }

    /* Create views for efficient hint queries */
    const char *create_active_region_hints_view = 
        "CREATE OR REPLACE VIEW active_region_hints AS "
        "SELECT "
            "rh.id, rh.region_vnum, rh.hint_category, rh.hint_text, "
            "rh.priority, rh.weather_conditions, rh.seasonal_weight, "
            "rh.time_of_day_weight, rh.resource_triggers, "
            "rp.description_style, rp.complexity_level "
        "FROM region_hints rh "
        "LEFT JOIN region_profiles rp ON rh.region_vnum = rp.region_vnum "
        "WHERE rh.is_active = TRUE "
        "ORDER BY rh.region_vnum, rh.priority DESC";

    if (mysql_query_safe(conn, create_active_region_hints_view)) {
        log("SYSERR: Failed to create active_region_hints view: %s", mysql_error(conn));
        /* Don't return - view creation is non-critical */
    }

    /* Create hint analytics view */
    const char *create_hint_analytics_view = 
        "CREATE OR REPLACE VIEW hint_analytics AS "
        "SELECT "
            "rh.region_vnum, rh.hint_category, "
            "COUNT(hul.id) as usage_count, "
            "AVG(rh.priority) as avg_priority, "
            "MAX(hul.used_at) as last_used, "
            "COUNT(DISTINCT hul.room_vnum) as unique_rooms "
        "FROM region_hints rh "
        "LEFT JOIN hint_usage_log hul ON rh.id = hul.hint_id "
        "WHERE rh.is_active = TRUE "
        "GROUP BY rh.region_vnum, rh.hint_category";

    if (mysql_query_safe(conn, create_hint_analytics_view)) {
        log("SYSERR: Failed to create hint_analytics view: %s", mysql_error(conn));
        /* Don't return - view creation is non-critical */
    }

    log("Info: Region hints system tables initialized successfully");
}

/* ===== AI SERVICE SYSTEM TABLES ===== */

void init_ai_service_tables(void)
{
    if (!mysql_available || !conn) {
        log("MySQL not available, skipping AI service tables initialization");
        return;
    }

    log("Initializing AI service system tables...");

    /* ai_config - AI service configuration */
    const char *create_ai_config = 
        "CREATE TABLE IF NOT EXISTS ai_config ("
        "id INT PRIMARY KEY AUTO_INCREMENT, "
        "config_key VARCHAR(50) UNIQUE NOT NULL, "
        "config_value TEXT, "
        "updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP, "
        "INDEX idx_config_key (config_key)"
        ")";

    if (mysql_query_safe(conn, create_ai_config)) {
        log("SYSERR: Failed to create ai_config table: %s", mysql_error(conn));
        return;
    }

    /* ai_requests - AI request logging for analytics */
    const char *create_ai_requests = 
        "CREATE TABLE IF NOT EXISTS ai_requests ("
        "id INT PRIMARY KEY AUTO_INCREMENT, "
        "request_type ENUM('npc_dialogue', 'room_desc', 'quest_gen', 'moderation', 'test') NOT NULL, "
        "prompt TEXT, "
        "response TEXT, "
        "tokens_used INT, "
        "response_time_ms INT, "
        "player_id INT, "
        "npc_vnum INT, "
        "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP, "
        "INDEX idx_created (created_at), "
        "INDEX idx_player (player_id), "
        "INDEX idx_request_type (request_type)"
        ")";

    if (mysql_query_safe(conn, create_ai_requests)) {
        log("SYSERR: Failed to create ai_requests table: %s", mysql_error(conn));
        return;
    }

    /* ai_cache - AI response caching */
    const char *create_ai_cache = 
        "CREATE TABLE IF NOT EXISTS ai_cache ("
        "cache_key VARCHAR(191) PRIMARY KEY, "
        "response TEXT, "
        "expires_at TIMESTAMP, "
        "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP, "
        "INDEX idx_expires (expires_at)"
        ")";

    if (mysql_query_safe(conn, create_ai_cache)) {
        log("SYSERR: Failed to create ai_cache table: %s", mysql_error(conn));
        return;
    }

    /* ai_npc_personalities - NPC personality configuration */
    const char *create_ai_npc_personalities = 
        "CREATE TABLE IF NOT EXISTS ai_npc_personalities ("
        "mob_vnum INT PRIMARY KEY, "
        "personality TEXT COMMENT 'JSON object with personality traits, background, speech patterns', "
        "enabled BOOLEAN DEFAULT TRUE, "
        "updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP, "
        "INDEX idx_enabled (enabled)"
        ")";

    if (mysql_query_safe(conn, create_ai_npc_personalities)) {
        log("SYSERR: Failed to create ai_npc_personalities table: %s", mysql_error(conn));
        return;
    }

    log("Info: AI service system tables initialized successfully");
}

/* ===== CRAFTING SYSTEM TABLES ===== */

void init_crafting_system_tables(void)
{
    if (!mysql_available || !conn) {
        log("MySQL not available, skipping crafting system tables initialization");
        return;
    }

    log("Initializing crafting system tables...");

    /* supply_orders_available - Crafting supply orders */
    const char *create_supply_orders = 
        "CREATE TABLE IF NOT EXISTS supply_orders_available ("
        "idnum INT AUTO_INCREMENT PRIMARY KEY, "
        "player_name VARCHAR(20) NOT NULL, "
        "supply_orders_available INT DEFAULT 0, "
        "last_updated TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP, "
        "UNIQUE KEY unique_player_name (player_name), "
        "INDEX idx_player_name (player_name)"
        ")";

    if (mysql_query_safe(conn, create_supply_orders)) {
        log("SYSERR: Failed to create supply_orders_available table: %s", mysql_error(conn));
        return;
    }

    log("Info: Crafting system tables initialized successfully");
}

/* ===== HOUSING SYSTEM TABLES ===== */

void init_housing_system_tables(void)
{
    if (!mysql_available || !conn) {
        log("MySQL not available, skipping housing system tables initialization");
        return;
    }

    log("Initializing housing system tables...");

    /* house_data - Housing system */
    const char *create_house_data = 
        "CREATE TABLE IF NOT EXISTS house_data ("
        "id INT AUTO_INCREMENT PRIMARY KEY, "
        "vnum INT UNIQUE NOT NULL, "
        "serialized_obj LONGTEXT, "
        "creation_date TIMESTAMP DEFAULT CURRENT_TIMESTAMP, "
        "last_accessed TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP, "
        "INDEX idx_vnum (vnum), "
        "INDEX idx_last_accessed (last_accessed)"
        ")";

    if (mysql_query_safe(conn, create_house_data)) {
        log("SYSERR: Failed to create house_data table: %s", mysql_error(conn));
        return;
    }

    log("Info: Housing system tables initialized successfully");
}

/* ===== HELP SYSTEM TABLES ===== */

/* Database Migration System */
void init_database_migrations(void)
{
    const char *create_migrations_table = 
        "CREATE TABLE IF NOT EXISTS schema_migrations ("
        "  version INT NOT NULL PRIMARY KEY COMMENT 'Schema version number',"
        "  description VARCHAR(255) NOT NULL COMMENT 'Description of migration',"
        "  applied_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP COMMENT 'When migration was applied',"
        "  INDEX idx_version (version)"
        ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 "
        "COMMENT='Tracks database schema migrations'";
    
    if (mysql_query_safe(conn, create_migrations_table)) {
        log("SYSERR: Failed to create schema_migrations table: %s", mysql_error(conn));
        return;
    }
    
    log("Info: Schema migrations table initialized");
}

/* Apply a migration if not already applied */
int apply_migration(int version, const char *description, const char *sql)
{
    char query[MAX_STRING_LENGTH];
    MYSQL_RES *result;
    int already_applied = 0;
    
    /* Check if migration already applied */
    snprintf(query, sizeof(query), 
        "SELECT version FROM schema_migrations WHERE version = %d", version);
    
    if (!mysql_query_safe(conn, query)) {
        result = mysql_store_result(conn);
        if (result) {
            if (mysql_num_rows(result) > 0) {
                already_applied = 1;
            }
            mysql_free_result(result);
        }
    }
    
    if (already_applied) {
        return 1; /* Success - already applied */
    }
    
    /* Apply the migration */
    log("Info: Applying migration %d: %s", version, description);
    
    if (mysql_query_safe(conn, sql)) {
        log("SYSERR: Failed to apply migration %d: %s", version, mysql_error(conn));
        return 0;
    }
    
    /* Record the migration */
    snprintf(query, sizeof(query),
        "INSERT INTO schema_migrations (version, description) VALUES (%d, '%s')",
        version, description);
    
    if (mysql_query_safe(conn, query)) {
        log("SYSERR: Failed to record migration %d: %s", version, mysql_error(conn));
        return 0;
    }
    
    log("Info: Migration %d applied successfully", version);
    return 1;
}

/* Run all pending migrations */
void run_database_migrations(void)
{
    if (!mysql_available || !conn) {
        log("MySQL not available, skipping database migrations");
        return;
    }
    
    /* Initialize migrations table */
    init_database_migrations();
    
    /* Migration 1: Add help_versions table for version control */
    apply_migration(1, "Add help_versions table for version history",
        "CREATE TABLE IF NOT EXISTS help_versions ("
        "  id INT AUTO_INCREMENT PRIMARY KEY,"
        "  tag VARCHAR(50) NOT NULL,"
        "  entry TEXT,"
        "  min_level INT DEFAULT 0,"
        "  changed_by VARCHAR(50),"
        "  change_date TIMESTAMP DEFAULT CURRENT_TIMESTAMP,"
        "  change_type ENUM('CREATE', 'UPDATE', 'DELETE') DEFAULT 'UPDATE',"
        "  INDEX idx_tag_date (tag, change_date)"
        ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 "
        "COMMENT='Version history for help entries'");
    
    /* Migration 2: Add help_search_history table */
    apply_migration(2, "Add help_search_history table for analytics",
        "CREATE TABLE IF NOT EXISTS help_search_history ("
        "  id INT AUTO_INCREMENT PRIMARY KEY,"
        "  search_term VARCHAR(200) NOT NULL,"
        "  searcher_name VARCHAR(50),"
        "  searcher_level INT,"
        "  results_count INT DEFAULT 0,"
        "  search_type ENUM('keyword', 'fulltext', 'soundex') DEFAULT 'keyword',"
        "  search_date TIMESTAMP DEFAULT CURRENT_TIMESTAMP,"
        "  INDEX idx_search_term (search_term),"
        "  INDEX idx_search_date (search_date)"
        ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 "
        "COMMENT='Tracks help searches for analytics'");
    
    /* Migration 3: Add categories to help entries */
    apply_migration(3, "Add category support to help entries",
        "ALTER TABLE help_entries "
        "ADD COLUMN IF NOT EXISTS category VARCHAR(50) DEFAULT 'general' "
        "COMMENT 'Help category for browsing' AFTER tag, "
        "ADD INDEX IF NOT EXISTS idx_category (category)");
    
    /* Migration 4: Add related_topics table */
    apply_migration(4, "Add related_topics table for help linking",
        "CREATE TABLE IF NOT EXISTS help_related_topics ("
        "  source_tag VARCHAR(50) NOT NULL,"
        "  related_tag VARCHAR(50) NOT NULL,"
        "  relevance_score FLOAT DEFAULT 1.0,"
        "  PRIMARY KEY (source_tag, related_tag),"
        "  INDEX idx_source (source_tag),"
        "  INDEX idx_related (related_tag)"
        ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 "
        "COMMENT='Links between related help topics'");
    
    log("Info: Database migrations completed");
}

void init_help_system_tables(void)
{
    if (!mysql_available || !conn) {
        log("MySQL not available, skipping help system tables initialization");
        return;
    }

    log("Initializing help system tables...");

    /* help_entries - Help system content */
    const char *create_help_entries = 
        "CREATE TABLE IF NOT EXISTS help_entries ("
        "id INT AUTO_INCREMENT PRIMARY KEY, "
        "tag VARCHAR(50) UNIQUE NOT NULL, "
        "entry LONGTEXT NOT NULL, "
        "min_level INT DEFAULT 0, "
        "max_level INT DEFAULT 1000, "
        "auto_generated BOOLEAN DEFAULT FALSE COMMENT 'TRUE if auto-generated, FALSE if manual', "
        "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP, "
        "updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP, "
        "INDEX idx_help_tag (tag), "
        "INDEX idx_min_level (min_level), "
        "INDEX idx_auto_generated (auto_generated)"
        ")";

    if (mysql_query_safe(conn, create_help_entries)) {
        log("SYSERR: Failed to create help_entries table: %s", mysql_error(conn));
        return;
    }

    /* help_keywords - Help system search keywords */
    const char *create_help_keywords = 
        "CREATE TABLE IF NOT EXISTS help_keywords ("
        "id INT AUTO_INCREMENT PRIMARY KEY, "
        "help_tag VARCHAR(50) NOT NULL, "
        "keyword VARCHAR(100) NOT NULL, "
        "FOREIGN KEY (help_tag) REFERENCES help_entries(tag) ON DELETE CASCADE, "
        "INDEX idx_keyword (keyword), "
        "INDEX idx_help_tag (help_tag), "
        "UNIQUE KEY unique_tag_keyword (help_tag, keyword)"
        ")";

    if (mysql_query_safe(conn, create_help_keywords)) {
        log("SYSERR: Failed to create help_keywords table: %s", mysql_error(conn));
        return;
    }

    log("Info: Help system tables initialized successfully");
    
    /* Add auto_generated column if it doesn't exist (for migration) */
    const char *add_auto_generated_column = 
        "ALTER TABLE help_entries "
        "ADD COLUMN IF NOT EXISTS auto_generated BOOLEAN DEFAULT FALSE "
        "COMMENT 'TRUE if auto-generated, FALSE if manual' "
        "AFTER max_level";
    
    if (mysql_query_safe(conn, add_auto_generated_column)) {
        log("Info: Could not add auto_generated column (may already exist): %s", mysql_error(conn));
        /* Non-critical - column may already exist */
    }
    
    /* Add index on auto_generated column for fast queries */
    const char *add_auto_generated_index = 
        "ALTER TABLE help_entries "
        "ADD INDEX IF NOT EXISTS idx_auto_generated (auto_generated)";
    
    if (mysql_query_safe(conn, add_auto_generated_index)) {
        log("Info: Could not add auto_generated index (may already exist): %s", mysql_error(conn));
        /* Non-critical - index may already exist */
    }
    
    /* Add performance optimization indexes
     * These indexes improve query performance for common help system operations
     * The IF NOT EXISTS clause prevents errors if indexes already exist */
    
    /* Composite index for JOIN operations - most queries join on help_tag and filter by keyword */
    const char *add_composite_index = 
        "ALTER TABLE help_keywords ADD INDEX IF NOT EXISTS idx_help_keywords_composite (help_tag, keyword)";
    
    if (mysql_query_safe(conn, add_composite_index)) {
        log("WARNING: Could not add composite index to help_keywords: %s", mysql_error(conn));
        /* Non-critical error - continue execution */
    }
    
    /* Add FULLTEXT index for future full-text search capabilities
     * This enables MATCH...AGAINST queries for better search functionality
     * Note: Requires InnoDB with MySQL 5.6+ or MyISAM */
    const char *add_fulltext_index = 
        "ALTER TABLE help_entries ADD FULLTEXT INDEX IF NOT EXISTS idx_help_entries_fulltext (entry)";
    
    if (mysql_query_safe(conn, add_fulltext_index)) {
        log("WARNING: Could not add FULLTEXT index to help_entries: %s", mysql_error(conn));
        log("         Full-text search features will not be available");
        /* Non-critical error - continue execution */
    }
    
    log("Info: Help system optimization indexes added");
    
    /* Run database migrations for new features */
    run_database_migrations();
}

/* ===== DATABASE PROCEDURES AND FUNCTIONS ===== */

void create_database_procedures(void)
{
    if (!mysql_available || !conn) {
        log("MySQL not available, skipping database procedures creation");
        return;
    }

    log("Creating database procedures and functions...");

    /* Create the bresenham_segment procedure first */
    const char *create_bresenham_segment = 
        "CREATE OR REPLACE PROCEDURE bresenham_segment("
        "IN x1 INT, IN y1 INT, IN x2 INT, IN y2 INT, "
        "OUT points_wkt TEXT"
        ") "
        "BEGIN "
        "DECLARE dx, dy, sx, sy, err, e2 INT; "
        "DECLARE x, y INT; "
        "DECLARE first_point BOOLEAN DEFAULT TRUE; "
        "DECLARE continue_loop BOOLEAN DEFAULT TRUE; "
        "SET points_wkt = ''; "
        "SET x = x1; "
        "SET y = y1; "
        "SET dx = ABS(x2 - x1); "
        "SET dy = ABS(y2 - y1); "
        "SET sx = IF(x1 < x2, 1, -1); "
        "SET sy = IF(y1 < y2, 1, -1); "
        "SET err = dx - dy; "
        "WHILE continue_loop DO "
        "IF first_point THEN "
        "SET points_wkt = CONCAT(x, ' ', y); "
        "SET first_point = FALSE; "
        "ELSE "
        "SET points_wkt = CONCAT(points_wkt, ',', x, ' ', y); "
        "END IF; "
        "IF x = x2 AND y = y2 THEN "
        "SET continue_loop = FALSE; "
        "ELSE "
        "SET e2 = 2 * err; "
        "IF e2 > -dy THEN "
        "SET err = err - dy; "
        "SET x = x + sx; "
        "END IF; "
        "IF e2 < dx THEN "
        "SET err = err + dx; "
        "SET y = y + sy; "
        "END IF; "
        "END IF; "
        "END WHILE; "
        "END";

    if (mysql_query_safe(conn, create_bresenham_segment)) {
        log("SYSERR: Failed to create bresenham_segment procedure: %s", mysql_error(conn));
        return;
    }

    /* Create the bresenham_line function */
    const char *create_bresenham_line = 
        "CREATE OR REPLACE FUNCTION bresenham_line(input_linestring LINESTRING) "
        "RETURNS LINESTRING "
        "READS SQL DATA "
        "DETERMINISTIC "
        "BEGIN "
        "DECLARE done INT DEFAULT FALSE; "
        "DECLARE point_count INT; "
        "DECLARE i INT DEFAULT 1; "
        "DECLARE x1, y1, x2, y2 INT; "
        "DECLARE result_wkt TEXT DEFAULT 'LINESTRING('; "
        "DECLARE first_point BOOLEAN DEFAULT TRUE; "
        "SET point_count = ST_NumPoints(input_linestring); "
        "IF point_count < 2 THEN "
        "RETURN input_linestring; "
        "END IF; "
        "WHILE i < point_count DO "
        "SET x1 = CAST(ST_X(ST_PointN(input_linestring, i)) AS SIGNED); "
        "SET y1 = CAST(ST_Y(ST_PointN(input_linestring, i)) AS SIGNED); "
        "SET x2 = CAST(ST_X(ST_PointN(input_linestring, i + 1)) AS SIGNED); "
        "SET y2 = CAST(ST_Y(ST_PointN(input_linestring, i + 1)) AS SIGNED); "
        "CALL bresenham_segment(x1, y1, x2, y2, @segment_points); "
        "IF i = 1 THEN "
        "SET result_wkt = CONCAT(result_wkt, @segment_points); "
        "ELSE "
        "SET @segment_points = SUBSTRING(@segment_points, LOCATE(',', @segment_points) + 1); "
        "SET result_wkt = CONCAT(result_wkt, ',', @segment_points); "
        "END IF; "
        "SET i = i + 1; "
        "END WHILE; "
        "SET result_wkt = CONCAT(result_wkt, ')'); "
        "RETURN ST_GeomFromText(result_wkt); "
        "END";

    if (mysql_query_safe(conn, create_bresenham_line)) {
        log("SYSERR: Failed to create bresenham_line function: %s", mysql_error(conn));
        return;
    }

    /* Create triggers for path_data linestring digitization */
    const char *create_path_digitize_insert_trigger = 
        "CREATE TRIGGER IF NOT EXISTS bi_digitalize_linestring "
        "BEFORE INSERT ON path_data "
        "FOR EACH ROW "
        "BEGIN "
        "IF (NEW.path_linestring IS NOT NULL) THEN "
        "SET NEW.path_linestring = bresenham_line(NEW.path_linestring); "
        "END IF; "
        "END";

    if (mysql_query_safe(conn, create_path_digitize_insert_trigger)) {
        log("SYSERR: Failed to create path digitize insert trigger: %s", mysql_error(conn));
        return;
    }

    const char *create_path_digitize_update_trigger = 
        "CREATE TRIGGER IF NOT EXISTS bu_digitalize_linestring "
        "BEFORE UPDATE ON path_data "
        "FOR EACH ROW "
        "BEGIN "
        "IF (NEW.path_linestring IS NOT NULL) THEN "
        "SET NEW.path_linestring = bresenham_line(NEW.path_linestring); "
        "END IF; "
        "END";

    if (mysql_query_safe(conn, create_path_digitize_update_trigger)) {
        log("SYSERR: Failed to create path digitize update trigger: %s", mysql_error(conn));
        return;
    }

    /* Create triggers for maintaining path index */
    const char *create_path_index_insert_trigger = 
        "CREATE TRIGGER IF NOT EXISTS ai_maintain_path_index "
        "AFTER INSERT ON path_data "
        "FOR EACH ROW "
        "BEGIN "
        "INSERT INTO path_index (vnum, zone_vnum, path_linestring) "
        "VALUES(NEW.vnum, NEW.zone_vnum, NEW.path_linestring); "
        "END";

    if (mysql_query_safe(conn, create_path_index_insert_trigger)) {
        log("SYSERR: Failed to create path index insert trigger: %s", mysql_error(conn));
        return;
    }

    const char *create_path_index_update_trigger = 
        "CREATE TRIGGER IF NOT EXISTS au_maintain_path_index "
        "AFTER UPDATE ON path_data "
        "FOR EACH ROW "
        "BEGIN "
        "UPDATE path_index SET zone_vnum = NEW.zone_vnum, path_linestring = NEW.path_linestring "
        "WHERE vnum = NEW.vnum; "
        "END";

    if (mysql_query_safe(conn, create_path_index_update_trigger)) {
        log("SYSERR: Failed to create path index update trigger: %s", mysql_error(conn));
        return;
    }

    const char *create_path_index_delete_trigger = 
        "CREATE TRIGGER IF NOT EXISTS ad_maintain_path_index "
        "AFTER DELETE ON path_data "
        "FOR EACH ROW "
        "BEGIN "
        "DELETE FROM path_index WHERE vnum = OLD.vnum; "
        "END";

    if (mysql_query_safe(conn, create_path_index_delete_trigger)) {
        log("SYSERR: Failed to create path index delete trigger: %s", mysql_error(conn));
        return;
    }

    /* Create triggers for maintaining region index */
    const char *create_region_index_insert_trigger = 
        "CREATE TRIGGER IF NOT EXISTS AI_MAINTAIN_REGION_INDEX "
        "AFTER INSERT ON region_data "
        "FOR EACH ROW "
        "BEGIN "
        "INSERT INTO region_index (vnum, zone_vnum, region_polygon) "
        "VALUES(NEW.vnum, NEW.zone_vnum, NEW.region_polygon); "
        "END";

    if (mysql_query_safe(conn, create_region_index_insert_trigger)) {
        log("SYSERR: Failed to create region index insert trigger: %s", mysql_error(conn));
        return;
    }

    const char *create_region_index_update_trigger = 
        "CREATE TRIGGER IF NOT EXISTS AU_MAINTAIN_REGION_INDEX "
        "AFTER UPDATE ON region_data "
        "FOR EACH ROW "
        "BEGIN "
        "UPDATE region_index SET zone_vnum = NEW.zone_vnum, region_polygon = NEW.region_polygon "
        "WHERE vnum = NEW.vnum; "
        "END";

    if (mysql_query_safe(conn, create_region_index_update_trigger)) {
        log("SYSERR: Failed to create region index update trigger: %s", mysql_error(conn));
        return;
    }

    /* Create triggers for deleting region index */
    const char *create_region_index_delete_trigger = 
        "CREATE TRIGGER IF NOT EXISTS AD_MAINTAIN_REGION_INDEX "
        "AFTER DELETE ON region_data "
        "FOR EACH ROW "
        "BEGIN "
        "DELETE FROM region_index WHERE vnum = OLD.vnum; "
        "END";

    if (mysql_query_safe(conn, create_region_index_delete_trigger)) {
        log("SYSERR: Failed to create region index delete trigger: %s", mysql_error(conn));
        return;
    }

    log("Info: Database procedures and functions created successfully");
}

/* [Continuing in next part due to length...] */
