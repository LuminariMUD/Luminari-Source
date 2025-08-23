/* *************************************************************************
 *   File: ai_region_hints.c                           Part of LuminariMUD *
 *  Usage: Implementation of AI-generated region hints system.            *
 * Author: Development Team                                                *
 ***************************************************************************
 * AI Region Hints System Implementation                                  *
 * ====================================                                   *
 * This file implements the AI region hints system that enhances dynamic *
 * descriptions with AI-generated atmospheric, fauna, flora, and other    *
 * descriptive elements specific to geographic regions.                   *
 ***************************************************************************/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "constants.h"
#include "mysql.h"
#include "ai_region_hints.h"
#include "desc_engine.h"
#include "wilderness.h"
#include "resource_descriptions.h"
#include <json-c/json.h>

/* Global cache for region hints */
struct ai_region_hint *region_hint_cache = NULL;
time_t region_hint_cache_time = 0;

/* Hint category names for logging and debugging */
const char *ai_hint_category_names[NUM_AI_HINT_CATEGORIES] = {
    "atmosphere", "fauna", "flora", "geography", "weather_influence",
    "resources", "landmarks", "sounds", "scents", "seasonal_changes",
    "time_of_day", "mystical"
};

/* ====================================================================== */
/* MAIN DESCRIPTION ENHANCEMENT FUNCTION                                 */
/* ====================================================================== */

/* Core description enhancement function - called from desc_engine.c */
char *enhance_wilderness_description_with_ai(struct char_data *ch, room_rnum room) {
    struct ai_description_context context;
    struct ai_region_hint *hints = NULL;
    char *description = NULL;
    int region_vnum;
    
    /* Only enhance wilderness rooms */
    if (!IS_WILDERNESS_VNUM(GET_ROOM_VNUM(room))) {
        return NULL;
    }
    
    /* Get region for this room */
    region_vnum = get_region_for_room(room);
    if (region_vnum <= 0) {
        return NULL;  /* No region found */
    }
    
    /* Load hints for this region */
    hints = load_region_hints(region_vnum);
    if (!hints) {
        return NULL;  /* No hints available */
    }
    
    /* Set up context */
    context.room = room;
    context.ch = ch;
    context.weather = weather_info.condition;
    context.season = weather_info.month / 3;  /* Rough season calculation */
    context.time_of_day = (weather_info.hours < 6 || weather_info.hours > 18) ? 0 : 1;
    context.resource_levels = NULL;
    context.num_resources = 0;
    context.profile = load_region_profile(region_vnum);
    context.available_hints = hints;
    
    /* Generate the enhanced description */
    description = generate_ai_enhanced_description(&context);
    
    /* Cleanup */
    free_region_hints(hints);
    if (context.profile) {
        free_region_profile(context.profile);
    }
    
    return description;
}

/* ====================================================================== */
/* HELPER FUNCTIONS - STUBS FOR NOW                                      */
/* ====================================================================== */

/* Get region for a room - stub implementation */
int get_region_for_room(room_rnum room) {
    /* For now, return a default region based on room number */
    /* This should be replaced with actual spatial lookup */
    return (GET_ROOM_VNUM(room) / 1000) + 1;  /* Simple grouping by zone */
}

/* Generate AI-enhanced description - stub implementation */
char *generate_ai_enhanced_description(struct ai_description_context *context) {
    static char description[MAX_STRING_LENGTH];
    
    /* Simple implementation for now */
    if (!context || !context->available_hints) {
        return NULL;
    }
    
    snprintf(description, sizeof(description), 
        "This wilderness area shows signs of %s. %s",
        context->available_hints->hint_text ? context->available_hints->hint_text : "natural beauty",
        "The AI enhancement system is now active.");
    
    return strdup(description);
}

/* Free memory functions */
void free_region_hints(struct ai_region_hint *hints) {
    struct ai_region_hint *current = hints;
    while (current) {
        struct ai_region_hint *next = current->next;
        if (current->hint_text) free(current->hint_text);
        if (current->weather_conditions) free(current->weather_conditions);
        if (current->seasonal_weight) free(current->seasonal_weight);
        if (current->time_of_day_weight) free(current->time_of_day_weight);
        if (current->resource_triggers) free(current->resource_triggers);
        free(current);
        current = next;
    }
}

void free_region_profile(struct ai_region_profile *profile) {
    if (!profile) return;
    if (profile->overall_theme) free(profile->overall_theme);
    if (profile->dominant_mood) free(profile->dominant_mood);
    if (profile->key_characteristics) free(profile->key_characteristics);
    free(profile);
}

/* ====================================================================== */
/* CORE HINT LOADING AND MANAGEMENT                                      */
/* ====================================================================== */

struct ai_region_hint *load_region_hints(int region_vnum) {
    MYSQL_RES *result = NULL;
    MYSQL_ROW row;
    struct ai_region_hint *hints = NULL;
    struct ai_region_hint *current_hint = NULL;
    struct ai_region_hint *new_hint = NULL;
    char query[MAX_STRING_LENGTH];
    
    snprintf(query, sizeof(query),
        "SELECT id, region_vnum, hint_category, hint_text, priority, "
        "weather_conditions, seasonal_weight, time_of_day_weight, "
        "resource_triggers, UNIX_TIMESTAMP(created_at), is_active "
        "FROM ai_region_hints "
        "WHERE region_vnum = %d AND is_active = TRUE "
        "ORDER BY priority DESC, id ASC",
        region_vnum);
    
    if (mysql_pool_query(query, &result) != 0) {
        log("SYSERR: MySQL query error in load_region_hints");
        return NULL;
    }
    
    if (!result) {
        log("SYSERR: MySQL store result error in load_region_hints");
        return NULL;
    }
    
    while ((row = mysql_fetch_row(result))) {
        CREATE(new_hint, struct ai_region_hint, 1);
        
        new_hint->id = atoi(row[0]);
        new_hint->region_vnum = atoi(row[1]);
        new_hint->hint_category = atoi(row[2]);
        new_hint->hint_text = strdup(row[3] ? row[3] : "");
        new_hint->priority = atoi(row[4]);
        new_hint->weather_conditions = strdup(row[5] ? row[5] : "");
        new_hint->seasonal_weight = strdup(row[6] ? row[6] : "{}");
        new_hint->time_of_day_weight = strdup(row[7] ? row[7] : "{}");
        new_hint->resource_triggers = strdup(row[8] ? row[8] : "{}");
        new_hint->created_at = atol(row[9]);
        new_hint->is_active = (atoi(row[10]) == 1);
        new_hint->next = NULL;
        
        if (!hints) {
            hints = new_hint;
            current_hint = new_hint;
        } else {
            current_hint->next = new_hint;
            current_hint = new_hint;
        }
    }
    
    mysql_pool_free_result(result);
    
    log("DEBUG: Loaded AI hints for region %d", region_vnum); 
    
    return hints;
}

struct ai_region_profile *load_region_profile(int region_vnum) {
    MYSQL_RES *result = NULL;
    MYSQL_ROW row;
    struct ai_region_profile *profile = NULL;
    char query[MAX_STRING_LENGTH];
    
    snprintf(query, sizeof(query),
        "SELECT region_vnum, overall_theme, dominant_mood, key_characteristics, "
        "description_style, complexity_level, UNIX_TIMESTAMP(created_at) "
        "FROM ai_region_profiles WHERE region_vnum = %d",
        region_vnum);
    
    if (mysql_pool_query(query, &result) != 0) {
        log("SYSERR: MySQL query error in load_region_profile");
        return NULL;
    }
    
    if (!result) {
        log("SYSERR: MySQL store result error in load_region_profile");
        return NULL;
    }
    
    if ((row = mysql_fetch_row(result))) {
        CREATE(profile, struct ai_region_profile, 1);
        
        profile->region_vnum = atoi(row[0]);
        profile->overall_theme = strdup(row[1] ? row[1] : "");
        profile->dominant_mood = strdup(row[2] ? row[2] : "");
        profile->key_characteristics = strdup(row[3] ? row[3] : "[]");
        profile->description_style = atoi(row[4]);
        profile->complexity_level = atoi(row[5]);
        profile->created_at = atol(row[6]);
    }
    
    mysql_pool_free_result(result);
    return profile;
}

void free_region_hints(struct ai_region_hint *hints) {
    struct ai_region_hint *current, *next;
    
    for (current = hints; current; current = next) {
        next = current->next;
        
        if (current->hint_text)
            free(current->hint_text);
        if (current->weather_conditions)
            free(current->weather_conditions);
        if (current->seasonal_weight)
            free(current->seasonal_weight);
        if (current->time_of_day_weight)
            free(current->time_of_day_weight);
        if (current->resource_triggers)
            free(current->resource_triggers);
        
        free(current);
    }
}

void free_region_profile(struct ai_region_profile *profile) {
    if (!profile)
        return;
        
    if (profile->overall_theme)
        free(profile->overall_theme);
    if (profile->dominant_mood)
        free(profile->dominant_mood);
    if (profile->key_characteristics)
        free(profile->key_characteristics);
    
    free(profile);
}

/* ====================================================================== */
/* HINT SELECTION AND FILTERING                                          */
/* ====================================================================== */

struct ai_region_hint *filter_hints_by_conditions(struct ai_region_hint *hints, 
                                                  struct ai_description_context *context) {
    struct ai_region_hint *filtered = NULL, *current_filtered = NULL;
    struct ai_region_hint *hint;
    
    for (hint = hints; hint; hint = hint->next) {
        if (hint_matches_conditions(hint, context)) {
            /* Create a copy of the hint for the filtered list */
            struct ai_region_hint *filtered_hint;
            CREATE(filtered_hint, struct ai_region_hint, 1);
            
            filtered_hint->id = hint->id;
            filtered_hint->region_vnum = hint->region_vnum;
            filtered_hint->hint_category = hint->hint_category;
            filtered_hint->hint_text = strdup(hint->hint_text);
            filtered_hint->priority = hint->priority;
            filtered_hint->weather_conditions = strdup(hint->weather_conditions);
            filtered_hint->seasonal_weight = strdup(hint->seasonal_weight);
            filtered_hint->time_of_day_weight = strdup(hint->time_of_day_weight);
            filtered_hint->resource_triggers = strdup(hint->resource_triggers);
            filtered_hint->created_at = hint->created_at;
            filtered_hint->is_active = hint->is_active;
            filtered_hint->next = NULL;
            
            if (!filtered) {
                filtered = filtered_hint;
                current_filtered = filtered_hint;
            } else {
                current_filtered->next = filtered_hint;
                current_filtered = filtered_hint;
            }
        }
    }
    
    return filtered;
}

struct ai_region_hint *select_hints_for_description(struct ai_region_hint *filtered_hints, 
                                                   int max_hints, 
                                                   struct ai_description_context *context) {
    struct ai_region_hint *selected = NULL, *current_selected = NULL;
    struct ai_region_hint *hint;
    int hints_selected = 0;
    int category_counts[NUM_AI_HINT_CATEGORIES] = {0};
    
    /* Prioritize variety - don't use too many hints from the same category */
    for (hint = filtered_hints; hint && hints_selected < max_hints; hint = hint->next) {
        /* Limit hints per category to maintain variety */
        if (category_counts[hint->hint_category] >= 2)
            continue;
            
        /* Calculate weighted priority based on conditions */
        double weight = 1.0;
        weight *= get_hint_weather_weight(hint, context->weather);
        weight *= get_hint_time_weight(hint, context->time_of_day);
        weight *= get_hint_seasonal_weight(hint, context->season);
        
        if (context->resource_levels && context->num_resources > 0) {
            weight *= get_hint_resource_weight(hint, context->resource_levels);
        }
        
        /* Use weighted priority for selection */
        if (weight > 0.3) {  /* Minimum threshold for inclusion */
            struct ai_region_hint *selected_hint;
            CREATE(selected_hint, struct ai_region_hint, 1);
            
            selected_hint->id = hint->id;
            selected_hint->region_vnum = hint->region_vnum;
            selected_hint->hint_category = hint->hint_category;
            selected_hint->hint_text = strdup(hint->hint_text);
            selected_hint->priority = hint->priority;
            selected_hint->weather_conditions = strdup(hint->weather_conditions);
            selected_hint->seasonal_weight = strdup(hint->seasonal_weight);
            selected_hint->time_of_day_weight = strdup(hint->time_of_day_weight);
            selected_hint->resource_triggers = strdup(hint->resource_triggers);
            selected_hint->created_at = hint->created_at;
            selected_hint->is_active = hint->is_active;
            selected_hint->next = NULL;
            
            if (!selected) {
                selected = selected_hint;
                current_selected = selected_hint;
            } else {
                current_selected->next = selected_hint;
                current_selected = selected_hint;
            }
            
            category_counts[hint->hint_category]++;
            hints_selected++;
        }
    }
    
    return selected;
}

/* ====================================================================== */
/* DESCRIPTION GENERATION                                                 */
/* ====================================================================== */

char *generate_ai_enhanced_description(struct ai_description_context *context) {
    struct ai_region_hint *hints = NULL, *filtered_hints = NULL, *selected_hints = NULL;
    char *base_description = NULL;
    char *enhanced_description = NULL;
    struct region_list *regions = NULL;
    struct region_list *curr_region = NULL;
    
    if (!context || !context->ch || context->room == NOWHERE) {
        log("SYSERR: Invalid context in generate_ai_enhanced_description");
        return NULL;
    }
    
    /* Get regions for this location */
    regions = get_enclosing_regions(GET_ROOM_ZONE(context->room), 
                                  world[context->room].coords[0], 
                                  world[context->room].coords[1]);
    
    if (!regions) {
        log("DEBUG: No regions found for room %d, using standard description", 
            world[context->room].number);
        return NULL;  /* Fall back to standard description */
    }
    
    /* Load hints for the first/primary region */
    curr_region = regions;
    if (curr_region && curr_region->rnum >= 0 && curr_region->rnum <= top_of_region_table) {
        context->profile = load_region_profile(region_table[curr_region->rnum].vnum);
        hints = load_region_hints(region_table[curr_region->rnum].vnum);
    }
    
    if (!hints) {
        log("DEBUG: No AI hints found for region, using standard description");
        free_region_list(regions);
        return NULL;  /* Fall back to standard description */
    }
    
    /* Filter and select appropriate hints */
    filtered_hints = filter_hints_by_conditions(hints, context);
    selected_hints = select_hints_for_description(filtered_hints, AI_HINTS_MAX_PER_DESCRIPTION, context);
    
    /* Generate base description using existing system */
    base_description = gen_room_description(context->ch, context->room);
    
    /* Enhance with AI hints */
    if (selected_hints && base_description) {
        enhanced_description = combine_hints_with_base_description(base_description, selected_hints, context);
        
        /* Log usage for analytics */
        struct ai_region_hint *hint;
        for (hint = selected_hints; hint; hint = hint->next) {
            log_hint_usage(hint->id, context->room, context->ch, context);
        }
    } else {
        enhanced_description = base_description ? strdup(base_description) : NULL;
    }
    
    /* Cleanup */
    if (base_description)
        free(base_description);
    free_region_hints(hints);
    free_region_hints(filtered_hints);
    free_region_hints(selected_hints);
    free_region_profile(context->profile);
    free_region_list(regions);
    
    return enhanced_description;
}

char *combine_hints_with_base_description(char *base_description, 
                                         struct ai_region_hint *selected_hints,
                                         struct ai_description_context *context) {
    char *combined = NULL;
    char buf[MAX_STRING_LENGTH * 2];
    struct ai_region_hint *hint;
    bool has_atmospheric = FALSE, has_sensory = FALSE;
    
    if (!base_description || !selected_hints) {
        return base_description ? strdup(base_description) : NULL;
    }
    
    buf[0] = '\0';
    
    /* Start with base description */
    strncat(buf, base_description, sizeof(buf) - strlen(buf) - 1);
    
    /* Add selected hints in a natural way */
    for (hint = selected_hints; hint; hint = hint->next) {
        /* Add appropriate spacing and transitional language */
        switch (hint->hint_category) {
        case AI_HINT_ATMOSPHERE:
        case AI_HINT_GEOGRAPHY:
            if (!has_atmospheric) {
                strncat(buf, "  ", sizeof(buf) - strlen(buf) - 1);
                strncat(buf, hint->hint_text, sizeof(buf) - strlen(buf) - 1);
                has_atmospheric = TRUE;
            }
            break;
            
        case AI_HINT_SOUNDS:
        case AI_HINT_SCENTS:
            if (!has_sensory) {
                strncat(buf, "  ", sizeof(buf) - strlen(buf) - 1);
                strncat(buf, hint->hint_text, sizeof(buf) - strlen(buf) - 1);
                has_sensory = TRUE;
            }
            break;
            
        case AI_HINT_FAUNA:
        case AI_HINT_FLORA:
            strncat(buf, "  ", sizeof(buf) - strlen(buf) - 1);
            strncat(buf, hint->hint_text, sizeof(buf) - strlen(buf) - 1);
            break;
            
        default:
            /* Other categories can be added as needed */
            break;
        }
    }
    
    combined = strdup(buf);
    return combined;
}

/* ====================================================================== */
/* WEIGHT CALCULATION FUNCTIONS                                           */
/* ====================================================================== */

double get_hint_weather_weight(struct ai_region_hint *hint, int weather_condition) {
    if (!hint->weather_conditions)
        return 1.0;
        
    /* Simple check - a more sophisticated version would parse the SET properly */
    const char *weather_names[] = {"clear", "cloudy", "rainy", "stormy", "lightning"};
    int weather_index = weather_condition % 5;  /* Simplified mapping */
    
    if (strstr(hint->weather_conditions, weather_names[weather_index])) {
        return 1.2;  /* Boost for matching weather */
    }
    
    return 0.8;  /* Slight penalty for non-matching weather */
}

double get_hint_time_weight(struct ai_region_hint *hint, int time_of_day) {
    /* Parse JSON to get time weights - simplified for now */
    if (!hint->time_of_day_weight || strcmp(hint->time_of_day_weight, "{}") == 0) {
        return 1.0;
    }
    
    /* This would be implemented with proper JSON parsing */
    return 1.0;
}

double get_hint_seasonal_weight(struct ai_region_hint *hint, int season) {
    /* Parse JSON to get seasonal weights - simplified for now */
    if (!hint->seasonal_weight || strcmp(hint->seasonal_weight, "{}") == 0) {
        return 1.0;
    }
    
    /* This would be implemented with proper JSON parsing */
    return 1.0;
}

double get_hint_resource_weight(struct ai_region_hint *hint, double *resource_levels) {
    /* Parse resource triggers and compare with current levels */
    if (!hint->resource_triggers || strcmp(hint->resource_triggers, "{}") == 0) {
        return 1.0;
    }
    
    /* This would be implemented with proper JSON parsing and resource comparison */
    return 1.0;
}

/* ====================================================================== */
/* UTILITY FUNCTIONS                                                      */
/* ====================================================================== */

bool hint_matches_conditions(struct ai_region_hint *hint, struct ai_description_context *context) {
    if (!hint || !context)
        return FALSE;
        
    /* Basic weather check */
    if (get_hint_weather_weight(hint, context->weather) < 0.5)
        return FALSE;
        
    /* Resource triggers check would go here */
    
    return TRUE;
}

char *get_hint_category_name(int category) {
    if (category >= 0 && category < NUM_AI_HINT_CATEGORIES) {
        return (char *)ai_hint_category_names[category];
    }
    return "unknown";
}

/* ====================================================================== */
/* ANALYTICS AND LOGGING                                                  */
/* ====================================================================== */

void log_hint_usage(int hint_id, room_rnum room, struct char_data *ch, 
                   struct ai_description_context *context) {
    char query[MAX_STRING_LENGTH];
    char weather_str[20] = "unknown";
    char season_str[10] = "spring";
    char time_str[10] = "day";
    MYSQL_RES *result = NULL;
        
    /* Convert context values to strings */
    if (context->weather < 50) strcpy(weather_str, "clear");
    else if (context->weather < 100) strcpy(weather_str, "cloudy");
    else if (context->weather < 150) strcpy(weather_str, "rainy");
    else if (context->weather < 200) strcpy(weather_str, "stormy");
    else strcpy(weather_str, "lightning");
    
    snprintf(query, sizeof(query),
        "INSERT INTO ai_hint_usage_log (hint_id, room_vnum, player_id, weather_condition, season, time_of_day) "
        "VALUES (%d, %d, %ld, '%s', '%s', '%s')",
        hint_id, world[room].number, ch ? GET_IDNUM(ch) : 0L, weather_str, season_str, time_str);
    
    mysql_pool_query(query, &result);  /* Non-critical, don't check errors */
    if (result) mysql_pool_free_result(result);
}

/* ====================================================================== */
/* INTEGRATION FUNCTIONS                                                  */
/* ====================================================================== */

char *enhance_wilderness_description_with_ai(struct char_data *ch, room_rnum room) {
    struct ai_description_context context;
    double resource_levels[10] = {0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5};
    
    /* Initialize context */
    context.room = room;
    context.ch = ch;
    context.weather = get_weather(world[room].coords[0], world[room].coords[1]);
    context.season = 0;  /* Would be calculated from time_info */
    context.time_of_day = (time_info.hours < 6 || time_info.hours > 18) ? 1 : 0;  /* night : day */
    context.resource_levels = resource_levels;
    context.num_resources = 10;
    context.profile = NULL;
    context.available_hints = NULL;
    
    /* Load resource levels if resource system is available */
#if defined(WILDERNESS_RESOURCE_DEPLETION_SYSTEM)
    /* This would integrate with the existing resource system */
#endif
    
    return generate_ai_enhanced_description(&context);
}
