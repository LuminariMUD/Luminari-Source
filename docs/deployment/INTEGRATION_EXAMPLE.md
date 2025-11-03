/* *************************************************************************
 * INTEGRATION EXAMPLE - How to add the database initialization system   *
 * to your existing LuminariMUD codebase                                 *
 ***************************************************************************/

/* ===== 1. ADD TO MAIN BOOT SEQUENCE ===== */

/* In your main.c or db.c, after MySQL connection is established: */

#include "db_init.h"  /* Add this include */

void boot_db(void)
{
    /* ... existing boot code ... */
    
    /* After connect_to_mysql() is called */
    connect_to_mysql();
    
    /* ADD THIS LINE: Initialize database during startup */
    startup_database_init();
    
    /* ... rest of existing boot code ... */
}

/* ===== 2. ADD TO MAIN GAME LOOP (OPTIONAL) ===== */

/* In your main game loop (main.c), add periodic health checking: */

void game_loop(int mother_desc)
{
    /* ... existing game loop code ... */
    
    while (!circle_shutdown) {
        /* ... existing loop code ... */
        
        /* ADD THIS LINE: Periodic database health check */
        periodic_database_health_check();
        
        /* ... rest of existing loop code ... */
    }
}

/* ===== 3. ADD ADMIN COMMANDS ===== */

/* In your interpreter.c command table, add: */

const struct command_info cmd_info[] = {
    /* ... existing commands ... */
    
    /* ADD THESE LINES: Database administration commands */
    { "database"        , POS_DEAD    , do_database        , LVL_GRGOD, 0 },
    { "db_init_system"  , POS_DEAD    , do_db_init_system  , LVL_IMPL , 0 },
    { "db_info"         , POS_DEAD    , do_db_info         , LVL_GRGOD, 0 },
    { "db_export_schema", POS_DEAD    , do_db_export_schema, LVL_IMPL , 0 },
    
    /* ... rest of existing commands ... */
};

/* ===== 4. UPDATE MAKEFILE ===== */

/* In your Makefile, add the new object files: */

OBJFILES = \
    account.o act.comm.o act.informative.o act.item.o act.offensive.o \
    act.other.o act.social.o act.wizard.o ban.o boards.o boards.o \
    castle.o class.o comm.o config.o constants.o db.o fight.o \
    /* ... existing object files ... */ \
    db_init.o db_init_data.o db_startup_init.o db_admin_commands.o \
    /* ADD THE ABOVE LINE with the new database init object files */

/* ===== 5. EXAMPLE USAGE IN ADMIN COMMANDS ===== */

/* You can also integrate database checks into existing admin commands */

ACMD(do_shutdown)
{
    /* ... existing shutdown code ... */
    
    /* Before shutting down, you might want to verify database state */
    if (is_database_available()) {
        log("Shutdown: Database connection active - checking integrity...");
        if (verify_database_integrity()) {
            log("Shutdown: Database integrity verified");
        } else {
            log("WARNING: Database integrity issues detected during shutdown");
        }
    }
    
    /* ... rest of shutdown code ... */
}

/* ===== 6. INTEGRATION WITH EXISTING RESOURCE SYSTEM ===== */

/* If you already have resource depletion code, you can check if 
   the database is properly initialized before using it: */

void some_existing_resource_function(void)
{
    /* Check if wilderness resource tables are available */
    if (!verify_wilderness_resource_tables()) {
        log("WARNING: Wilderness resource tables not available");
        log("INFO: Attempting to initialize wilderness system...");
        init_wilderness_resource_tables();
        populate_resource_types_data();
        populate_material_system_data();
        populate_region_effects_data();
    }
    
    /* Now proceed with existing resource code */
    /* ... existing resource depletion code ... */
}

/* ===== 7. GRACEFUL DEGRADATION ===== */

/* Example of how to handle missing database gracefully */

void some_feature_that_needs_database(struct char_data *ch)
{
    if (!is_database_available()) {
        send_to_char(ch, "This feature requires database connectivity.\r\n");
        return;
    }
    
    if (!verify_core_player_tables()) {
        send_to_char(ch, "Player database tables not available.\r\n");
        send_to_char(ch, "Contact an administrator to initialize the database.\r\n");
        return;
    }
    
    /* Proceed with database-dependent functionality */
    /* ... existing code ... */
}

/* ===== 8. CUSTOM INITIALIZATION ===== */

/* If you have custom tables, you can extend the system: */

void init_custom_mud_tables(void)
{
    if (!mysql_available || !conn) {
        log("MySQL not available, skipping custom tables initialization");
        return;
    }

    log("Initializing custom MUD tables...");

    /* Your custom table creation */
    const char *create_custom_table = 
        "CREATE TABLE IF NOT EXISTS my_custom_table ("
        "id INT AUTO_INCREMENT PRIMARY KEY, "
        "custom_data TEXT"
        ")";

    if (mysql_query_safe(conn, create_custom_table)) {
        log("SYSERR: Failed to create my_custom_table: %s", mysql_error(conn));
        return;
    }

    log("INFO: Custom MUD tables initialized successfully");
}

/* Then add it to the main initialization: */
void custom_init_luminari_database(void)
{
    /* Call the standard initialization */
    init_luminari_database();
    
    /* Add your custom initialization */
    init_custom_mud_tables();
}

/* ===== 9. TESTING THE INTEGRATION ===== */

/* After integration, test with these steps:
 *
 * 1. Start MUD with empty database - should auto-initialize
 * 2. Use 'database status' command - should show all systems OK
 * 3. Use 'database verify' command - should pass all checks
 * 4. Check MUD logs for initialization messages
 * 5. Test existing database-dependent features
 */

/* ===== 10. TROUBLESHOOTING INTEGRATION ===== */

/* Common integration issues and solutions:
 *
 * Compile errors:
 * - Make sure all .h files are included
 * - Check that function prototypes match
 * - Verify Makefile includes new object files
 *
 * Runtime errors:
 * - Check that MySQL connection is established first
 * - Verify database user has sufficient privileges
 * - Make sure mysql_available flag is set correctly
 *
 * Database errors:
 * - Check existing table names don't conflict
 * - Verify foreign key references are correct
 * - Ensure database exists before table creation
 */
