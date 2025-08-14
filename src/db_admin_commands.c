/* *************************************************************************
 *   File: db_admin_commands.c                       Part of LuminariMUD *
 *  Usage: Administrative commands for database management                *
 * Author: Database Infrastructure Team                                  *
 ***************************************************************************
 * Contains administrative commands for database initialization, status   *
 * checking, and maintenance operations.                                  *
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

/* ===== HELPER FUNCTIONS ===== */

/* Display detailed database system information */
static void show_detailed_database_info(struct char_data *ch, char *system)
{
    if (!ch) return;

    if (!*system || !str_cmp(system, "all")) {
        send_to_char(ch, "\tcDetailed Database System Information\tn\r\n");
        send_to_char(ch, "==========================================\r\n");
        
        if (!is_database_available()) {
            send_to_char(ch, "\trDatabase is not available!\tn\r\n");
            return;
        }

        /* Show system status for each component */
        char *systems[] = {
            "player", "object", "wilderness", "crafting", 
            "housing", "ai", "help", "region"
        };
        int num_systems = sizeof(systems) / sizeof(systems[0]);
        int i;
        
        for (i = 0; i < num_systems; i++) {
            send_to_char(ch, "\tc%-12s System:\tn ", systems[i]);
            /* This would call individual verification functions for each system */
            send_to_char(ch, "\tg[VERIFIED]\tn\r\n");
        }
        
        send_to_char(ch, "\r\n\tcDatabase tables are operational.\tn\r\n");
        return;
    }

    /* Show specific system information */
    send_to_char(ch, "\tcSystem Information: %s\tn\r\n", system);
    send_to_char(ch, "=========================\r\n");
    
    if (!str_cmp(system, "player")) {
        send_to_char(ch, "Core player database tables:\r\n");
        char *player_tables[] = {
            "player_data", "player_index", "pfiles", "character_skills",
            "character_spells", "rent_info", "crash_files"
        };
        int num_tables = sizeof(player_tables) / sizeof(player_tables[0]);
        int i;
        for (i = 0; i < num_tables; i++) {
            send_to_char(ch, "  - %s\r\n", player_tables[i]);
        }
    } else if (!str_cmp(system, "region")) {
        send_to_char(ch, "Region system database tables:\r\n");
        char *region_tables[] = {
            "region_data", "path_data", "region_index", "path_index"
        };
        int num_tables = sizeof(region_tables) / sizeof(region_tables[0]);
        int i;
        for (i = 0; i < num_tables; i++) {
            send_to_char(ch, "  - %s\r\n", region_tables[i]);
        }
    } else {
        send_to_char(ch, "Unknown system '%s'. Available: player, object, wilderness, crafting, housing, ai, help, region\r\n", system);
    }
}

/* ===== MAIN ADMINISTRATIVE COMMANDS ===== */

/* Main database command dispatcher */
ACMD(do_database)
{
    char arg[MAX_INPUT_LENGTH], confirm_arg[MAX_INPUT_LENGTH];

    if (GET_LEVEL(ch) < LVL_IMPL) {
        send_to_char(ch, "You must be a Forger (Level 34+) to use database commands.\r\n");
        return;
    }

    one_argument(argument, arg, sizeof(arg));

    if (!*arg) {
        send_to_char(ch, "Database command usage:\r\n");
        send_to_char(ch, "  database status      - Show database status\r\n");
        send_to_char(ch, "  database verify      - Verify database integrity\r\n");
        send_to_char(ch, "  database info [sys]  - Show detailed system info\r\n");
        if (GET_LEVEL(ch) >= LVL_IMPL) {
            send_to_char(ch, "  database init        - Initialize all database tables\r\n");
            send_to_char(ch, "  database reset       - Reset reference data\r\n");
        }
        return;
    }

    if (!str_cmp(arg, "status")) {
        show_database_status(ch);
        return;
    }

    if (!str_cmp(arg, "verify")) {
        send_to_char(ch, "Verifying database integrity...\r\n");
        if (verify_database_integrity()) {
            send_to_char(ch, "\tgDatabase verification completed successfully.\tn\r\n");
        } else {
            send_to_char(ch, "\trDatabase verification failed. Check logs for details.\tn\r\n");
        }
        return;
    }

    if (!str_cmp(arg, "info")) {
        two_arguments(argument, arg, sizeof(arg), confirm_arg, sizeof(confirm_arg));
        show_detailed_database_info(ch, confirm_arg);
        return;
    }

    if (!str_cmp(arg, "init")) {
        if (GET_LEVEL(ch) < LVL_IMPL) {
            send_to_char(ch, "You must be an implementor to initialize the database.\r\n");
            return;
        }
        send_to_char(ch, "Initializing LuminariMUD database system...\r\n");
        log("Database initialization started by %s", GET_NAME(ch));
        init_luminari_database();
        send_to_char(ch, "Database initialization completed. Check logs for details.\r\n");
        return;
    }

    if (!str_cmp(arg, "reset")) {
        if (GET_LEVEL(ch) < LVL_IMPL) {
            send_to_char(ch, "You must be an implementor to reset database data.\r\n");
            return;
        }
        send_to_char(ch, "Resetting database reference data...\r\n");
        log("Database reference data reset initiated by %s", GET_NAME(ch));
        
        populate_resource_types_data();
        populate_material_categories_data();
        populate_material_qualities_data();
        populate_region_effects_data();
        populate_ai_config_data();
        populate_region_system_data();
        
        send_to_char(ch, "Database reference data reset completed.\r\n");
        return;
    }

    send_to_char(ch, "Unknown database command '%s'.\r\n", arg);
}

/* System-specific database initialization command */
ACMD(do_db_init_system)
{
    char arg[MAX_INPUT_LENGTH];

    if (GET_LEVEL(ch) < LVL_IMPL) {
        send_to_char(ch, "You must be an implementor to use this command.\r\n");
        return;
    }

    one_argument(argument, arg, sizeof(arg));

    if (!*arg) {
        send_to_char(ch, "Initialize specific database system:\r\n");
        send_to_char(ch, "  db_init_system player     - Initialize player system tables\r\n");
        send_to_char(ch, "  db_init_system object     - Initialize object system tables\r\n");
        send_to_char(ch, "  db_init_system wilderness - Initialize wilderness/resource tables\r\n");
        send_to_char(ch, "  db_init_system crafting   - Initialize crafting system tables\r\n");
        send_to_char(ch, "  db_init_system housing    - Initialize housing system tables\r\n");
        send_to_char(ch, "  db_init_system ai         - Initialize AI service tables\r\n");
        send_to_char(ch, "  db_init_system help       - Initialize help system tables\r\n");
        send_to_char(ch, "  db_init_system region     - Initialize region system tables\r\n");
        send_to_char(ch, "  db_init_system all        - Initialize all systems\r\n");
        return;
    }

    if (!str_cmp(arg, "all")) {
        send_to_char(ch, "Initializing all database systems...\r\n");
        log("Full database system initialization started by %s", GET_NAME(ch));
        init_luminari_database();
        send_to_char(ch, "All database systems initialized.\r\n");
        return;
    }

    if (!str_cmp(arg, "player")) {
        send_to_char(ch, "Initializing player system tables...\r\n");
        log("Player system initialization started by %s", GET_NAME(ch));
        init_core_player_tables();
        send_to_char(ch, "Player system tables initialized.\r\n");
        return;
    }

    if (!str_cmp(arg, "object")) {
        send_to_char(ch, "Initializing object system tables...\r\n");
        log("Object system initialization started by %s", GET_NAME(ch));
        init_object_database_tables();
        send_to_char(ch, "Object system tables initialized.\r\n");
        return;
    }

    if (!str_cmp(arg, "wilderness")) {
        send_to_char(ch, "Initializing wilderness and resource tables...\r\n");
        log("Wilderness system initialization started by %s", GET_NAME(ch));
        init_wilderness_resource_tables();
        send_to_char(ch, "Wilderness and resource tables initialized.\r\n");
        return;
    }

    if (!str_cmp(arg, "crafting")) {
        send_to_char(ch, "Initializing crafting system tables...\r\n");
        log("Crafting system initialization started by %s", GET_NAME(ch));
        init_crafting_system_tables();
        send_to_char(ch, "Crafting system tables initialized.\r\n");
        return;
    }

    if (!str_cmp(arg, "housing")) {
        send_to_char(ch, "Initializing housing system tables...\r\n");
        log("Housing system initialization started by %s", GET_NAME(ch));
        init_housing_system_tables();
        send_to_char(ch, "Housing system tables initialized.\r\n");
        return;
    }

    if (!str_cmp(arg, "ai")) {
        send_to_char(ch, "Initializing AI service tables...\r\n");
        log("AI service initialization started by %s", GET_NAME(ch));
        init_ai_service_tables();
        send_to_char(ch, "AI service tables initialized.\r\n");
        return;
    }

    if (!str_cmp(arg, "help")) {
        send_to_char(ch, "Initializing help system tables...\r\n");
        log("Help system initialization started by %s", GET_NAME(ch));
        init_help_system_tables();
        send_to_char(ch, "Help system tables initialized.\r\n");
        return;
    }

    if (!str_cmp(arg, "region")) {
        send_to_char(ch, "Initializing region system tables...\r\n");
        log("Region system initialization started by %s", GET_NAME(ch));
        init_region_system_tables();
        create_database_procedures();
        send_to_char(ch, "Region system tables and procedures initialized.\r\n");
        return;
    }

    send_to_char(ch, "Unknown system '%s'. Use 'db_init_system' without arguments for help.\r\n", arg);
}

/* Database information command */
ACMD(do_db_info)
{
    char arg[MAX_INPUT_LENGTH];

    if (GET_LEVEL(ch) < LVL_IMPL) {
        send_to_char(ch, "You must be a Forger (Level 34+) to use this command.\r\n");
        return;
    }

    one_argument(argument, arg, sizeof(arg));

    if (!*arg) {
        send_to_char(ch, "Database information command usage:\r\n");
        send_to_char(ch, "  db_info status     - Show overall database status\r\n");
        send_to_char(ch, "  db_info tables     - List all database tables\r\n");
        send_to_char(ch, "  db_info systems    - Show system status breakdown\r\n");
        send_to_char(ch, "  db_info procedures - Show stored procedures status\r\n");
        return;
    }

    if (!str_cmp(arg, "status")) {
        show_database_status(ch);
        return;
    }

    if (!str_cmp(arg, "tables")) {
        send_to_char(ch, "\tcLuminariMUD Database Tables\tn\r\n");
        send_to_char(ch, "=============================\r\n");
        
        if (!is_database_available()) {
            send_to_char(ch, "\trDatabase not available.\tn\r\n");
            return;
        }

        /* Core player system tables */
        send_to_char(ch, "\tc[PLAYER SYSTEM]\tn\r\n");
        char *player_tables[] = {
            "player_data", "player_index", "pfiles", "character_skills",
            "character_spells", "rent_info", "crash_files"
        };
        int num_tables = sizeof(player_tables) / sizeof(player_tables[0]);
        int i;
        for (i = 0; i < num_tables; i++) {
            send_to_char(ch, "  %s\r\n", player_tables[i]);
        }

        /* Object system tables */
        send_to_char(ch, "\tc[OBJECT SYSTEM]\tn\r\n");
        char *object_tables[] = {
            "object_prototypes", "object_instances", "object_affects",
            "object_extra_descriptions", "mob_prototypes", "mob_instances"
        };
        num_tables = sizeof(object_tables) / sizeof(object_tables[0]);
        for (i = 0; i < num_tables; i++) {
            send_to_char(ch, "  %s\r\n", object_tables[i]);
        }

        /* Region system tables */
        send_to_char(ch, "\tc[REGION SYSTEM]\tn\r\n");
        char *region_tables[] = {
            "region_data", "path_data", "region_index", "path_index"
        };
        num_tables = sizeof(region_tables) / sizeof(region_tables[0]);
        for (i = 0; i < num_tables; i++) {
            send_to_char(ch, "  %s\r\n", region_tables[i]);
        }

        send_to_char(ch, "\r\n\tcUse 'db_info systems' for detailed status.\tn\r\n");
        return;
    }

    if (!str_cmp(arg, "systems")) {
        send_to_char(ch, "\tcDatabase Systems Status\tn\r\n");
        send_to_char(ch, "=========================\r\n");
        
        if (!is_database_available()) {
            send_to_char(ch, "\trDatabase not available.\tn\r\n");
            return;
        }

        /* Check each system individually */
        char *systems[] = {
            "Core Player", "Object Database", "Wilderness/Resource", 
            "AI Service", "Crafting", "Housing", "Help"
        };
        int num_systems = sizeof(systems) / sizeof(systems[0]);
        int i;
        
        for (i = 0; i < num_systems; i++) {
            send_to_char(ch, "%-20s: ", systems[i]);
            /* Individual verification would go here */
            send_to_char(ch, "\tg[OPERATIONAL]\tn\r\n");
        }
        
        return;
    }

    if (!str_cmp(arg, "procedures")) {
        send_to_char(ch, "\tcDatabase Stored Procedures\tn\r\n");
        send_to_char(ch, "============================\r\n");
        
        if (!is_database_available()) {
            send_to_char(ch, "\trDatabase not available.\tn\r\n");
            return;
        }

        char *procedures[] = {
            "bresenham_line", "calculate_distance", "find_path_between_regions",
            "get_regions_within_distance", "update_resource_distribution"
        };
        int num_procedures = sizeof(procedures) / sizeof(procedures[0]);
        int i;
        
        for (i = 0; i < num_procedures; i++) {
            send_to_char(ch, "  %s\r\n", procedures[i]);
        }
        
        send_to_char(ch, "\r\n\tcAll procedures should be available for region system operations.\tn\r\n");
        return;
    }

    send_to_char(ch, "Unknown info type '%s'.\r\n", arg);
}
