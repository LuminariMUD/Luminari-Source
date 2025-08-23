/* *************************************************************************
 *   File: db_init.h                               Part of LuminariMUD *
 *  Usage: Database initialization system header                         *
 * Author: Database Infrastructure Team                                  *
 ***************************************************************************
 * Comprehensive database initialization system for LuminariMUD         *
 * Handles creation of all database tables and population of reference  *
 * data to ensure a complete, working database setup.                   *
 ***************************************************************************/

#ifndef DB_INIT_H
#define DB_INIT_H

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "db.h"

/* ===== MAIN INITIALIZATION FUNCTIONS ===== */

/* Master database initialization function - creates ALL tables */
void init_luminari_database(void);

/* ===== SYSTEM-SPECIFIC INITIALIZATION FUNCTIONS ===== */

/* Initialize core player system tables */
void init_core_player_tables(void);

/* Initialize object database tables */
void init_object_database_tables(void);

/* Initialize wilderness and resource management tables */
void init_wilderness_resource_tables(void);

/* Initialize AI service system tables */
void init_ai_service_tables(void);

/* Initialize crafting system tables */
void init_crafting_system_tables(void);

/* Initialize housing system tables */
void init_housing_system_tables(void);

/* Initialize help system tables */
void init_help_system_tables(void);

/* Initialize region system tables with spatial geometry support */
void init_region_system_tables(void);

/* Initialize region hints system tables */
void init_region_hints_tables(void);

/* Create stored procedures for region system */
void create_database_procedures(void);

/* ===== REFERENCE DATA POPULATION FUNCTIONS ===== */

/* Populate reference data for various systems */
void populate_resource_types_data(void);
void populate_material_categories_data(void);
void populate_material_qualities_data(void);
void populate_region_effects_data(void);
void populate_ai_config_data(void);
void populate_region_system_data(void);

/* ===== DATABASE VERIFICATION FUNCTIONS ===== */

/* Check if database connection is available */
int is_database_available(void);

/* Test database connectivity and permissions */
int test_database_permissions(void);

/* Verify all required tables exist */
int verify_database_integrity(void);

/* Show database initialization status */
void show_database_status(struct char_data *ch);

/* ===== TABLE VERIFICATION FUNCTIONS ===== */

/* Individual table checkers - return TRUE if table exists and has correct structure */
int verify_core_player_tables(void);
int verify_object_database_tables(void);
int verify_wilderness_resource_tables(void);
int verify_ai_service_tables(void);
int verify_crafting_system_tables(void);
int verify_housing_system_tables(void);
int verify_help_system_tables(void);

/* ===== ADMIN COMMANDS ===== */

/* Admin command prototypes for manual database management */
ACMD_DECL(do_database);        /* Main database admin command */
ACMD_DECL(do_db_init_system);  /* Initialize specific systems */
ACMD_DECL(do_db_info);         /* Database information command */

/* ===== STARTUP INITIALIZATION FUNCTIONS ===== */
void startup_database_init(void);
void initialize_missing_tables(void);
int table_exists(const char *table_name);
int procedure_exists(const char *procedure_name);
int verify_database_procedures(void);
int test_database_features(void);
void repair_database_if_needed(void);
void log_database_startup_stats(void);

#endif /* DB_INIT_H */
