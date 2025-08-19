/* *************************************************************************
 *   File: region_hints.c                          Part of LuminariMUD *
 *  Usage: Region hints system implementation                           *
 * Author: Development Team                                             *
 ***************************************************************************
 * Region Hints System                                                  *
 * ===================                                                   *
 * This system provides descriptive hints with the dynamic description  *
 * engine to create rich, location-specific descriptions.               *
 ***************************************************************************/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "db.h"
#include "comm.h"
#include "mysql.h"
#include "region_hints.h"
#include "wilderness.h"

/* External function declarations */
extern struct region_list *get_enclosing_regions(zone_rnum zone, int x, int y);
extern void free_region_list(struct region_list *regions);
extern int get_weather(int x, int y);
extern char *generate_resource_aware_description(struct char_data *ch, room_rnum room);

/* Global variables */
struct region_hint *region_hint_cache = NULL;
time_t region_hint_cache_time = 0;

/* ====================================================================== */
/*                         CORE HINT MANAGEMENT                          */
/* ====================================================================== */

struct region_hint *load_region_hints(int region_vnum) {
    MYSQL_RES *result;
    MYSQL_ROW row;
    struct region_hint *hints = NULL;
    struct region_hint *current_hint = NULL;
    char query[MAX_STRING_LENGTH];
    
    if (!mysql_pool) {
        log("SYSERR: No database connection available for loading region hints");
        return NULL;
    }
    
    snprintf(query, sizeof(query),
        "SELECT id, region_vnum, hint_category, hint_text, priority, "
        "weather_conditions, seasonal_weight, time_of_day_weight, "
        "resource_triggers, UNIX_TIMESTAMP(created_at), is_active "
        "FROM region_hints "
        "WHERE region_vnum = %d AND is_active = TRUE "
        "ORDER BY priority DESC, id ASC",
        region_vnum);
    
    if (mysql_pool_query(query, &result)) {
        log("SYSERR: MySQL query error in load_region_hints");
        return NULL;
    }
    
    if (!result) {
        log("SYSERR: MySQL store result error in load_region_hints");
        return NULL;
    }
    
    while ((row = mysql_fetch_row(result))) {
        struct region_hint *new_hint = calloc(1, sizeof(struct region_hint));
        if (!new_hint) {
            log("SYSERR: Memory allocation failed in load_region_hints");
            break;
        }
        
        new_hint->id = atoi(row[0]);
        new_hint->region_vnum = atoi(row[1]);
        new_hint->hint_category = atoi(row[2]);
        new_hint->hint_text = strdup(row[3] ? row[3] : "");
        new_hint->priority = atoi(row[4]);
        new_hint->weather_conditions = strdup(row[5] ? row[5] : "");
        new_hint->seasonal_weight = strdup(row[6] ? row[6] : "{}");
        new_hint->time_of_day_weight = strdup(row[7] ? row[7] : "{}");
        new_hint->resource_triggers = strdup(row[8] ? row[8] : "{}");
        new_hint->created_at = row[9] ? atol(row[9]) : 0;
        new_hint->is_active = row[10] ? (atoi(row[10]) > 0) : false;
        new_hint->next = NULL;
        
        if (!hints) {
            hints = new_hint;
        } else {
            current_hint->next = new_hint;
        }
        current_hint = new_hint;
    }
    
    mysql_pool_free_result(result);
    return hints;
}

struct region_profile *load_region_profile(int region_vnum) {
    MYSQL_RES *result;
    MYSQL_ROW row;
    struct region_profile *profile = NULL;
    char query[MAX_STRING_LENGTH];
    
    if (!mysql_pool) {
        log("SYSERR: No MySQL connection in load_region_profile");
        return NULL;
    }
    
    snprintf(query, sizeof(query),
        "SELECT region_vnum, overall_theme, dominant_mood, key_characteristics, "
        "description_style, complexity_level, UNIX_TIMESTAMP(created_at) "
        "FROM region_profiles WHERE region_vnum = %d",
        region_vnum);
    
    if (mysql_pool_query(query, &result)) {
        log("SYSERR: MySQL query error in load_region_profile");
        return NULL;
    }
    
    if (!result) {
        log("SYSERR: MySQL store result error in load_region_profile");
        return NULL;
    }
    
    if ((row = mysql_fetch_row(result))) {
        profile = calloc(1, sizeof(struct region_profile));
        if (profile) {
            profile->region_vnum = atoi(row[0]);
            profile->overall_theme = strdup(row[1] ? row[1] : "");
            profile->dominant_mood = strdup(row[2] ? row[2] : "");
            profile->key_characteristics = strdup(row[3] ? row[3] : "{}");
            profile->description_style = row[4] ? atoi(row[4]) : 0;
            profile->complexity_level = row[5] ? atoi(row[5]) : 3;
            profile->created_at = row[6] ? atol(row[6]) : 0;
        }
    }
    
    mysql_pool_free_result(result);
    return profile;
}

void free_region_hints(struct region_hint *hints) {
    struct region_hint *current = hints;
    struct region_hint *next;
    
    while (current) {
        next = current->next;
        
        if (current->hint_text) free(current->hint_text);
        if (current->weather_conditions) free(current->weather_conditions);
        if (current->seasonal_weight) free(current->seasonal_weight);
        if (current->time_of_day_weight) free(current->time_of_day_weight);
        if (current->resource_triggers) free(current->resource_triggers);
        
        free(current);
        current = next;
    }
}

void free_region_profile(struct region_profile *profile) {
    if (!profile) return;
    
    if (profile->overall_theme) free(profile->overall_theme);
    if (profile->dominant_mood) free(profile->dominant_mood);
    if (profile->key_characteristics) free(profile->key_characteristics);
    
    free(profile);
}

/* ====================================================================== */
/*                    INTEGRATION WITH DESC ENGINE                       */
/* ====================================================================== */

char *enhance_wilderness_description_with_hints(struct char_data *ch, room_rnum room) {
    struct region_list *regions = NULL;
    struct region_list *curr_region = NULL;
    struct region_hint *hints = NULL;
    struct region_profile *profile = NULL;
    struct description_context context;
    char *enhanced_desc = NULL;
    int region_vnum = NOWHERE;
    
    log("DEBUG: enhance_wilderness_description_with_hints called for room %d", GET_ROOM_VNUM(room));
    
    /* Only enhance wilderness rooms for now */
    if (!IS_WILDERNESS_VNUM(GET_ROOM_VNUM(room))) {
        log("DEBUG: Room %d is not wilderness, returning NULL", GET_ROOM_VNUM(room));
        return NULL;
    }
    
    /* Get the enclosing regions for this room */
    regions = get_enclosing_regions(GET_ROOM_ZONE(room), world[room].coords[0], world[room].coords[1]);
    if (!regions) {
        log("DEBUG: No enclosing regions found for coordinates (%d, %d)", world[room].coords[0], world[room].coords[1]);
        return NULL; /* No regions found, fall back to default descriptions */
    }
    
    log("DEBUG: Found enclosing regions for coordinates (%d, %d)", world[room].coords[0], world[room].coords[1]);
    
    /* Find the best region for hints - prefer geographic regions over encounter regions */
    curr_region = regions;
    struct region_list *best_region = NULL;
    struct region_list *geographic_region = NULL;
    struct region_list *encounter_region = NULL;
    
    /* Scan through all regions to find the best one for hints */
    while (curr_region) {
        if (curr_region->rnum != NOWHERE && curr_region->rnum <= top_of_region_table) {
            int region_type = region_table[curr_region->rnum].region_type;
            log("DEBUG: Found region vnum %d (type %d) from region_table[%d]", 
                region_table[curr_region->rnum].vnum, region_type, curr_region->rnum);
            
            if (region_type == 1) { /* Geographic region */
                geographic_region = curr_region;
                log("DEBUG: Found geographic region vnum %d", region_table[curr_region->rnum].vnum);
            } else if (region_type == 2) { /* Encounter region */
                encounter_region = curr_region;
                log("DEBUG: Found encounter region vnum %d", region_table[curr_region->rnum].vnum);
            }
        }
        curr_region = curr_region->next;
    }
    
    /* Prefer geographic regions for hints, fall back to encounter regions if needed */
    if (geographic_region) {
        best_region = geographic_region;
        log("DEBUG: Using geographic region for hints");
    } else if (encounter_region) {
        best_region = encounter_region;
        log("DEBUG: No geographic region found, using encounter region for hints");
    } else {
        log("DEBUG: No suitable regions found for hints");
        free_region_list(regions);
        return NULL;
    }
    
    region_vnum = region_table[best_region->rnum].vnum;
    log("DEBUG: Selected region vnum %d from region_table[%d] for hints", region_vnum, best_region->rnum);
    
    /* Load hints and profile for this region */
    hints = load_region_hints(region_vnum);
    profile = load_region_profile(region_vnum);
    
    log("DEBUG: Loaded %s hints and %s profile for region %d", 
        hints ? "valid" : "no", profile ? "valid" : "no", region_vnum);
    
    /* If no hints available, fall back to default */
    if (!hints) {
        log("DEBUG: No hints available for region %d, falling back to default", region_vnum);
        if (profile) free_region_profile(profile);
        free_region_list(regions);
        return NULL;
    }
    
    /* Build context for hint selection */
    build_description_context(room, ch, &context);
    context.profile = profile;
    context.available_hints = hints;
    
    log("DEBUG: Built description context, calling generate_enhanced_description");
    
    /* Select and combine relevant hints */
    enhanced_desc = generate_enhanced_description(&context);
    
    if (enhanced_desc) {
        log("DEBUG: Successfully generated enhanced description: %.100s...", enhanced_desc);
    } else {
        log("DEBUG: generate_enhanced_description returned NULL");
    }
    
    /* Clean up */
    free_region_hints(hints);
    if (profile) free_region_profile(profile);
    free_region_list(regions);
    
    return enhanced_desc;
}

/* ====================================================================== */
/*                         CONTEXT BUILDING                              */
/* ====================================================================== */

void build_description_context(room_rnum room, struct char_data *ch, struct description_context *context) {
    if (!context) return;
    
    context->room = room;
    context->ch = ch;
    context->resource_levels = NULL;
    context->num_resources = 0;
    context->profile = NULL;
    context->available_hints = NULL;
    
    /* Get weather information */
    if (IS_WILDERNESS_VNUM(GET_ROOM_VNUM(room))) {
        int x = world[room].coords[0];
        int y = world[room].coords[1];
        int raw_weather = get_weather(x, y);
        
        /* Convert wilderness weather to our scale */
        if (raw_weather >= 225) context->weather = 4; /* Lightning */
        else if (raw_weather >= 200) context->weather = 3; /* Stormy */
        else if (raw_weather >= 178) context->weather = 2; /* Rainy */
        else if (raw_weather > 127) context->weather = 1; /* Cloudy */
        else context->weather = 0; /* Clear */
    } else {
        /* Non-wilderness: use global weather */
        switch (weather_info.sky) {
            case SKY_CLOUDLESS: context->weather = 0; break;
            case SKY_CLOUDY: context->weather = 1; break;
            case SKY_RAINING: context->weather = 2; break;
            case SKY_LIGHTNING: context->weather = 4; break;
            default: context->weather = 0; break;
        }
    }
    
    /* Get time of day */
    context->time_of_day = weather_info.sunlight; /* SUN_DARK, SUN_RISE, SUN_LIGHT, SUN_SET */
    
    /* Get season (simplified) */
    context->season = time_info.month / 4; /* 0=winter, 1=spring, 2=summer, 3=autumn */
}

/* ====================================================================== */
/*                         HINT SELECTION                                */
/* ====================================================================== */

struct region_hint *select_relevant_hints(struct region_hint *all_hints, struct description_context *context, int max_hints) {
    struct region_hint *selected = NULL;
    struct region_hint *current = all_hints;
    struct region_hint *last_selected = NULL;
    int selected_count = 0;
    
    while (current && selected_count < max_hints) {
        if (hint_matches_conditions(current, context)) {
            /* Create a copy of the hint for our selected list */
            struct region_hint *hint_copy = calloc(1, sizeof(struct region_hint));
            if (hint_copy) {
                hint_copy->id = current->id;
                hint_copy->region_vnum = current->region_vnum;
                hint_copy->hint_category = current->hint_category;
                hint_copy->hint_text = strdup(current->hint_text ? current->hint_text : "");
                hint_copy->priority = current->priority;
                hint_copy->next = NULL;
                
                if (!selected) {
                    selected = hint_copy;
                } else {
                    last_selected->next = hint_copy;
                }
                last_selected = hint_copy;
                selected_count++;
            }
        }
        current = current->next;
    }
    
    return selected;
}

bool hint_matches_conditions(struct region_hint *hint, struct description_context *context) {
    if (!hint || !context) return false;
    
    /* Basic weather matching - for now, just check if weather conditions include our current weather */
    if (hint->weather_conditions && strlen(hint->weather_conditions) > 0) {
        char weather_str[20];
        switch (context->weather) {
            case 0: strcpy(weather_str, "clear"); break;
            case 1: strcpy(weather_str, "cloudy"); break;
            case 2: strcpy(weather_str, "rainy"); break;
            case 3: strcpy(weather_str, "stormy"); break;
            case 4: strcpy(weather_str, "lightning"); break;
            default: strcpy(weather_str, "clear"); break;
        }
        
        /* Simple substring match for now */
        if (!strstr(hint->weather_conditions, weather_str)) {
            return false;
        }
    }
    
    /* For now, accept all hints that pass weather check */
    /* TODO: Add seasonal_weight, time_of_day_weight, and resource_triggers parsing */
    
    return true;
}

/* ====================================================================== */
/*                      DESCRIPTION GENERATION                           */
/* ====================================================================== */

char *generate_enhanced_description(struct description_context *context) {
    char *base_desc = NULL;
    char *enhanced_desc = NULL;
    struct region_hint *selected_hints = NULL;
    int max_hints = HINTS_MAX_PER_DESCRIPTION;
    
    if (!context) return NULL;
    
    /* Get base description from existing system */
    base_desc = generate_resource_aware_description(context->ch, context->room);
    if (!base_desc) {
        /* Fall back to basic room description */
        base_desc = strdup(world[context->room].description ? world[context->room].description : "You see nothing special.");
    }
    
    /* Select relevant hints */
    selected_hints = select_relevant_hints(context->available_hints, context, max_hints);
    
    if (selected_hints) {
        /* Combine base description with hints */
        enhanced_desc = combine_base_with_hints(base_desc, selected_hints, context);
        free_region_hints(selected_hints);
    } else {
        /* No relevant hints, return base description */
        enhanced_desc = strdup(base_desc);
    }
    
    if (base_desc) free(base_desc);
    return enhanced_desc;
}

char *combine_base_with_hints(char *base_desc, struct region_hint *hints, struct description_context *context) {
    char *combined = NULL;
    struct region_hint *current = hints;
    int total_len = 0;
    int base_len = base_desc ? strlen(base_desc) : 0;
    
    /* Calculate total length needed */
    total_len = base_len + 100; /* Base + some buffer */
    while (current) {
        if (current->hint_text) {
            total_len += strlen(current->hint_text) + 10; /* hint + spacing */
        }
        current = current->next;
    }
    
    combined = malloc(total_len);
    if (!combined) return NULL;
    
    /* Start with base description */
    if (base_desc) {
        strcpy(combined, base_desc);
    } else {
        strcpy(combined, "");
    }
    
    /* Add hints */
    current = hints;
    while (current) {
        if (current->hint_text && strlen(current->hint_text) > 0) {
            /* Add some connecting text and the hint */
            strcat(combined, " ");
            strcat(combined, current->hint_text);
            
            /* Log hint usage for analytics */
            log_hint_usage(current->id, context->room, context->ch, context);
        }
        current = current->next;
    }
    
    return combined;
}

void log_hint_usage(int hint_id, room_rnum room, struct char_data *ch, struct description_context *context) {
    char query[MAX_STRING_LENGTH];
    char weather_str[20] = "unknown";
    char season_str[10] = "spring";
    char time_str[10] = "day";
    
    if (!mysql_pool)
        return;
        
    /* Convert context values to strings */
    if (context->weather < 50) strcpy(weather_str, "clear");
    else if (context->weather < 100) strcpy(weather_str, "cloudy");
    else if (context->weather < 150) strcpy(weather_str, "rainy");
    else if (context->weather < 200) strcpy(weather_str, "stormy");
    else strcpy(weather_str, "lightning");
    
    snprintf(query, sizeof(query),
        "INSERT INTO hint_usage_log (hint_id, room_vnum, player_id, weather_condition, season, time_of_day) "
        "VALUES (%d, %d, %ld, '%s', '%s', '%s')",
        hint_id, world[room].number, ch ? GET_IDNUM(ch) : 0L, weather_str, season_str, time_str);
    
    /* Try to log hint usage - don't worry if it fails */
    MYSQL_RES *dummy_result;
    mysql_pool_query(query, &dummy_result);
    if (dummy_result) {
        mysql_pool_free_result(dummy_result);
    }
}

char *get_hint_category_name(int category) {
    switch (category) {
        case HINT_ATMOSPHERE: return "atmosphere";
        case HINT_FAUNA: return "fauna";
        case HINT_FLORA: return "flora";
        case HINT_GEOGRAPHY: return "geography";
        case HINT_WEATHER_INFLUENCE: return "weather_influence";
        case HINT_RESOURCES: return "resources";
        case HINT_LANDMARKS: return "landmarks";
        case HINT_SOUNDS: return "sounds";
        case HINT_SCENTS: return "scents";
        case HINT_SEASONAL_CHANGES: return "seasonal_changes";
        case HINT_TIME_OF_DAY: return "time_of_day";
        case HINT_MYSTICAL: return "mystical";
        default: return "unknown";
    }
}
