/* *************************************************************************
 *   File: resource_cascade_integration.c              Part of LuminariMUD *
 *  Usage: Integration of Phase 7 cascade system with existing commands   *
 * Author: Implementation Team                                             *
 ***************************************************************************
 * This file integrates the Phase 7 ecological resource interdependencies *
 * with the existing resource and survey systems.                         *
 ***************************************************************************/

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
#include "resource_cascade.h"

/* Enhanced survey command with cascade analysis */
ACMD(do_enhanced_survey)
{
    char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
    int resource_type, radius;
    room_rnum room = IN_ROOM(ch);
    zone_rnum zrnum;
    
    if (!ch || room == NOWHERE) {
        return;
    }
    
    zrnum = world[room].zone;
    if (!ZONE_FLAGGED(zrnum, ZONE_WILDERNESS)) {
        send_to_char(ch, "Surveying can only be performed in the wilderness.\r\n");
        return;
    }
    
    two_arguments(argument, arg1, arg2);
    
    if (!*arg1) {
        /* Default: show basic resource survey */
        show_resource_survey(ch);
        return;
    }
    
    /* Parse first argument */
    if (!str_cmp(arg1, "resources")) {
        show_resource_survey(ch);
        
    } else if (!str_cmp(arg1, "ecosystem")) {
        show_ecosystem_analysis(ch, room);
        
    } else if (!str_cmp(arg1, "cascade")) {
        if (!*arg2) {
            send_to_char(ch, "Usage: survey cascade <resource_type>\r\n");
            send_to_char(ch, "Available resources: vegetation, minerals, water, herbs, game, wood, stone, crystal, clay, salt\r\n");
            return;
        }
        
        resource_type = parse_resource_type(arg2);
        if (resource_type < 0) {
            send_to_char(ch, "Invalid resource type '%s'.\r\n", arg2);
            return;
        }
        
        show_cascade_preview(ch, room, resource_type);
        
    } else if (!str_cmp(arg1, "relationships")) {
        show_resource_relationships(ch, room);
        
    } else if (!str_cmp(arg1, "impact")) {
        show_conservation_impact(ch, room);
        
    } else if (!str_cmp(arg1, "terrain")) {
        show_terrain_survey(ch);
        
    } else if (!str_cmp(arg1, "debug")) {
        show_debug_survey(ch);
        
    } else if (!str_cmp(arg1, "map")) {
        if (!*arg2) {
            send_to_char(ch, "Usage: survey map <resource_type> [radius]\r\n");
            return;
        }
        
        resource_type = parse_resource_type(arg2);
        if (resource_type < 0) {
            send_to_char(ch, "Invalid resource type '%s'.\r\n", arg2);
            return;
        }
        
        /* Check for optional radius argument */
        radius = 7; /* Default */
        if (argument && *argument) {
            char arg3[MAX_INPUT_LENGTH];
            argument = one_argument(argument, arg3); /* Skip first two args */
            argument = one_argument(argument, arg3); 
            argument = one_argument(argument, arg3); /* Get third arg */
            if (*arg3 && is_number(arg3)) {
                radius = atoi(arg3);
                if (radius < 5 || radius > 20) radius = 7;
            }
        }
        
        show_resource_map(ch, resource_type, radius);
        
    } else if (!str_cmp(arg1, "detail")) {
        if (!*arg2) {
            send_to_char(ch, "Usage: survey detail <resource_type>\r\n");
            return;
        }
        
        resource_type = parse_resource_type(arg2);
        if (resource_type < 0) {
            send_to_char(ch, "Invalid resource type '%s'.\r\n", arg2);
            return;
        }
        
        show_resource_detail(ch, resource_type);
        
    } else {
        /* Try parsing as resource type for map */
        resource_type = parse_resource_type(arg1);
        if (resource_type >= 0) {
            radius = 7; /* Default radius */
            if (*arg2 && is_number(arg2)) {
                radius = atoi(arg2);
                if (radius < 5 || radius > 20) radius = 7;
            }
            show_resource_map(ch, resource_type, radius);
        } else {
            send_to_char(ch, "Invalid survey option. Use: resources, ecosystem, cascade, relationships, impact, terrain, debug, map, detail\r\n");
        }
    }
}

/* Enhanced harvest command with cascade effects */
ACMD(do_enhanced_harvest)
{
    char arg[MAX_INPUT_LENGTH];
    int resource_type;
    room_rnum room = IN_ROOM(ch);
    zone_rnum zrnum;
    
    if (!ch || room == NOWHERE) {
        return;
    }
    
    zrnum = world[room].zone;
    if (!ZONE_FLAGGED(zrnum, ZONE_WILDERNESS)) {
        send_to_char(ch, "Harvesting can only be performed in the wilderness.\r\n");
        return;
    }
    
    one_argument(argument, arg);
    
    if (!*arg) {
        send_to_char(ch, "Usage: harvest <resource_type>\r\n");
        send_to_char(ch, "Available resources: vegetation, herbs, game\r\n");
        send_to_char(ch, "Note: Use 'mine' for minerals/crystals/stone, 'gather' for specific resources\r\n");
        return;
    }
    
    resource_type = parse_resource_type(arg);
    if (resource_type < 0) {
        send_to_char(ch, "Invalid resource type '%s'.\r\n", arg);
        return;
    }
    
    /* Check if resource type is appropriate for harvest command */
    if (resource_type != RESOURCE_VEGETATION && 
        resource_type != RESOURCE_HERBS && 
        resource_type != RESOURCE_GAME) {
        send_to_char(ch, "You can only harvest: vegetation, herbs, or game.\r\n");
        send_to_char(ch, "Use 'mine' for minerals, crystals, or stone.\r\n");
        send_to_char(ch, "Use 'gather' for other resources.\r\n");
        return;
    }
    
    /* Check ecosystem health before harvesting */
    if (is_ecosystem_critical(room)) {
        send_to_char(ch, "\trThis ecosystem is in critical condition. Harvesting here may cause irreversible damage.\tn\r\n");
        send_to_char(ch, "Are you sure you want to proceed? (Type 'harvest %s force' to continue)\r\n", arg);
        
        /* Check for force argument */
        char force_arg[MAX_INPUT_LENGTH];
        two_arguments(argument, arg, force_arg);
        if (str_cmp(force_arg, "force")) {
            return;
        }
        send_to_char(ch, "\trYou proceed with harvesting despite the ecosystem damage.\tn\r\n");
    }
    
    /* Show cascade preview before harvesting */
    if (GET_LEVEL(ch) <= LVL_IMMORT) { /* Only show for mortal players */
        send_to_char(ch, "\tcEcological Impact Preview:\tn\r\n");
        show_cascade_preview(ch, room, resource_type);
        send_to_char(ch, "\r\n");
    }
    
    /* Attempt harvest with cascade effects */
    attempt_wilderness_harvest_with_cascades(ch, resource_type);
}

/* Enhanced harvest function that includes cascade effects */
int attempt_wilderness_harvest_with_cascades(struct char_data *ch, int resource_type) {
    int result = attempt_wilderness_harvest(ch, resource_type);
    
    if (result > 0) {
        /* Harvesting was successful, apply cascade effects */
        apply_cascade_effects(IN_ROOM(ch), resource_type, result);
        
        /* Update player conservation score */
        bool sustainable = is_harvest_sustainable(IN_ROOM(ch), resource_type, result);
        update_player_conservation_score(ch, resource_type, result, sustainable);
        
        /* Show ecosystem status after harvest */
        int ecosystem_state = get_ecosystem_state(IN_ROOM(ch));
        if (ecosystem_state >= ECOSYSTEM_STRESSED) {
            send_to_char(ch, "\tyYour harvesting has contributed to ecosystem stress in this area.\tn\r\n");
        }
    }
    
    return result;
}

/* Enhanced resource relationship display */
void show_resource_relationships(struct char_data *ch, room_rnum room)
{
    char query[MAX_STRING_LENGTH];
    MYSQL_RES *result;
    MYSQL_ROW row;
    int x, y;
    
    if (!ch || room == NOWHERE)
        return;
    
    if (!mysql_available || !conn) {
        send_to_char(ch, "Resource relationship data not available (database offline).\r\n");
        return;
    }
    
    x = world[room].coords[0];
    y = world[room].coords[1];
    
    send_to_char(ch, "\tcResource Interaction Matrix for (%d, %d):\tn\r\n", x, y);
    send_to_char(ch, "==========================================\r\n");
    
    /* Query all resource relationships */
    snprintf(query, sizeof(query),
        "SELECT source_resource, target_resource, effect_magnitude, description "
        "FROM resource_relationships ORDER BY source_resource, ABS(effect_magnitude) DESC");
    
    if (mysql_query_safe(conn, query)) {
        send_to_char(ch, "Error querying resource relationships.\r\n");
        return;
    }
    
    result = mysql_store_result_safe(conn);
    if (!result) {
        send_to_char(ch, "No resource relationships found.\r\n");
        return;
    }
    
    int current_source = -1;
    while ((row = mysql_fetch_row(result))) {
        int source_resource = atoi(row[0]);
        int target_resource = atoi(row[1]);
        float effect_magnitude = atof(row[2]);
        const char *description = row[3];
        
        if (source_resource >= 0 && source_resource < NUM_RESOURCE_TYPES &&
            target_resource >= 0 && target_resource < NUM_RESOURCE_TYPES) {
            
            /* New source resource header */
            if (source_resource != current_source) {
                if (current_source >= 0) send_to_char(ch, "\r\n");
                send_to_char(ch, "\tY%s affects:\tn\r\n", resource_names[source_resource]);
                current_source = source_resource;
            }
            
            const char *effect_color = effect_magnitude < 0 ? "\tr" : "\tg";
            const char *effect_symbol = effect_magnitude < 0 ? "↓" : "↑";
            
            send_to_char(ch, "  %s%s %s\tn (%.1f%% effect)\r\n",
                         effect_color, effect_symbol, resource_names[target_resource],
                         fabs(effect_magnitude) * 100);
            send_to_char(ch, "    \tc%s\tn\r\n", description);
        }
    }
    
    mysql_free_result(result);
    
    send_to_char(ch, "\r\n\tcLegend: \tg↑\tn = enhances, \tr↓\tn = depletes\r\n");
}

/* Show player conservation impact */
void show_conservation_impact(struct char_data *ch, room_rnum room)
{
    float conservation_score;
    int ecosystem_state;
    int x, y;
    
    if (!ch || room == NOWHERE)
        return;
    
    x = world[room].coords[0];
    y = world[room].coords[1];
    conservation_score = get_player_conservation_score(ch);
    ecosystem_state = get_ecosystem_state(room);
    
    send_to_char(ch, "\tcConservation Impact Analysis for (%d, %d):\tn\r\n", x, y);
    send_to_char(ch, "=============================================\r\n");
    
    /* Player conservation score */
    const char *score_color = conservation_score >= 0.8 ? "\tG" : 
                             conservation_score >= 0.6 ? "\tY" : "\tr";
    const char *score_desc = conservation_score >= 0.8 ? "Excellent Steward" :
                           conservation_score >= 0.6 ? "Good Conservationist" :
                           conservation_score >= 0.4 ? "Moderate Impact" :
                           conservation_score >= 0.2 ? "Heavy Harvester" : "Ecosystem Destroyer";
    
    send_to_char(ch, "Your Conservation Score: %s%.2f\tn (%s%s\tn)\r\n",
                 score_color, conservation_score, score_color, score_desc);
    
    /* Ecosystem health */
    send_to_char(ch, "Current Ecosystem Health: %s%s\tn\r\n",
                 ecosystem_state <= ECOSYSTEM_HEALTHY ? "\tG" : 
                 ecosystem_state == ECOSYSTEM_STRESSED ? "\tY" : "\tr",
                 get_ecosystem_state_name(ecosystem_state));
    
    /* Recommendations */
    send_to_char(ch, "\r\n\tcRecommendations:\tn\r\n");
    
    if (conservation_score < 0.4) {
        send_to_char(ch, "\tr• Reduce harvesting frequency and quantity\tn\r\n");
        send_to_char(ch, "\tr• Focus on sustainable gathering practices\tn\r\n");
        send_to_char(ch, "\tr• Allow areas to recover between harvests\tn\r\n");
    } else if (conservation_score < 0.7) {
        send_to_char(ch, "\ty• Take smaller amounts per harvest\tn\r\n");
        send_to_char(ch, "\ty• Vary your harvesting locations\tn\r\n");
        send_to_char(ch, "\ty• Monitor ecosystem health regularly\tn\r\n");
    } else {
        send_to_char(ch, "\tg• Excellent conservation practices!\tn\r\n");
        send_to_char(ch, "\tg• Continue sustainable harvesting\tn\r\n");
        send_to_char(ch, "\tg• Consider helping restore damaged areas\tn\r\n");
    }
    
    if (ecosystem_state >= ECOSYSTEM_STRESSED) {
        send_to_char(ch, "\r\n\trEcosystem Warning:\tn This area requires conservation attention.\r\n");
        send_to_char(ch, "Consider reducing all harvesting activity for 1-2 game days.\r\n");
    }
}

/* Administrative command for ecosystem management */
ACMD(do_resourceadmin_cascade)
{
    char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
    room_rnum room = IN_ROOM(ch);
    
    if (GET_LEVEL(ch) < LVL_IMMORT) {
        send_to_char(ch, "You don't have permission to use cascade administration commands.\r\n");
        return;
    }
    
    two_arguments(argument, arg1, arg2);
    
    if (!*arg1) {
        send_to_char(ch, "Resource Cascade Administration Commands:\r\n");
        send_to_char(ch, "========================================\r\n");
        send_to_char(ch, "resourceadmin cascade ecosystem [room]   - Show detailed ecosystem debug\r\n");
        send_to_char(ch, "resourceadmin cascade reset [room]       - Reset ecosystem health\r\n");
        send_to_char(ch, "resourceadmin cascade recalc [room]      - Recalculate ecosystem\r\n");
        send_to_char(ch, "resourceadmin cascade stats              - Server-wide statistics\r\n");
        send_to_char(ch, "resourceadmin cascade toggle             - Enable/disable system\r\n");
        return;
    }
    
    if (!str_cmp(arg1, "ecosystem")) {
        if (*arg2 && is_number(arg2)) {
            room = real_room(atoi(arg2));
            if (room == NOWHERE) {
                send_to_char(ch, "Invalid room number.\r\n");
                return;
            }
        }
        admin_show_ecosystem_debug(ch, room);
        
    } else if (!str_cmp(arg1, "reset")) {
        if (*arg2 && is_number(arg2)) {
            room = real_room(atoi(arg2));
            if (room == NOWHERE) {
                send_to_char(ch, "Invalid room number.\r\n");
                return;
            }
        }
        admin_reset_ecosystem(room);
        send_to_char(ch, "Ecosystem reset for room %d.\r\n", GET_ROOM_VNUM(room));
        
    } else if (!str_cmp(arg1, "recalc")) {
        if (*arg2 && is_number(arg2)) {
            room = real_room(atoi(arg2));
            if (room == NOWHERE) {
                send_to_char(ch, "Invalid room number.\r\n");
                return;
            }
        }
        admin_recalculate_ecosystem(room);
        send_to_char(ch, "Ecosystem recalculated for room %d.\r\n", GET_ROOM_VNUM(room));
        
    } else if (!str_cmp(arg1, "stats")) {
        admin_show_ecosystem_stats(ch);
        
    } else if (!str_cmp(arg1, "toggle")) {
        cascade_system_enabled = !cascade_system_enabled;
        send_to_char(ch, "Cascade system %s.\r\n", 
                     cascade_system_enabled ? "enabled" : "disabled");
    } else {
        send_to_char(ch, "Invalid cascade administration command.\r\n");
    }
}
