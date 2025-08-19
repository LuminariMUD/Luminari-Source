/**
 * @file narrative_weaver.c
 * @brief Template-free narrative weaving system for LuminariMUD
 * @author GitHub Copilot Region Architect
 * @date August 18, 2025
 * 
 * This system creates unified wilderness descriptions by intelligently weaving
 * together comprehensive region descriptions and contextual hints based on
 * current weather, time, and environmental conditions. It uses wilderness
 * weather (Perlin noise based) rather than zone-based weather.
 */

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "db.h"
#include "wilderness.h"
#include "mysql.h"
#include "region_hints.h"
#include "systems/narrative_weaver/narrative_weaver.h"

/* External function declarations */
extern struct region_list *get_enclosing_regions(zone_rnum zone, int x, int y);
extern void free_region_list(struct region_list *regions);
extern int get_weather(int x, int y);
extern char *generate_resource_aware_description(struct char_data *ch, room_rnum room);
extern struct time_info_data time_info;
extern struct region_data *region_table;
extern struct region_data *region_table;
extern zone_rnum top_of_region_table;

/* ====================================================================== */
/*                         UTILITY FUNCTIONS                             */
/* ====================================================================== */

/**
 * Convert wilderness weather value to semantic weather condition
 * Uses wilderness weather system (0-255 Perlin noise based)
 */
const char *get_wilderness_weather_condition(int x, int y) {
    int weather_val = get_weather(x, y);
    
    if (weather_val < 64) {
        return "clear";
    } else if (weather_val < 128) {
        return "cloudy";
    } else if (weather_val < 192) {
        return "rainy";
    } else {
        return "stormy";
    }
}

/**
 * Get current time of day category
 */
const char *get_time_of_day_category(void) {
    int hour = time_info.hours;
    
    if (hour >= 6 && hour < 12) {
        return "morning";
    } else if (hour >= 12 && hour < 18) {
        return "afternoon";
    } else if (hour >= 18 && hour < 22) {
        return "evening";
    } else {
        return "night";
    }
}

/**
 * Transform second-person references to third-person observational
 */
char *transform_voice_to_observational(const char *text) {
    char *result;
    char *pos;
    size_t len;
    
    if (!text) return NULL;
    
    len = strlen(text) + 100; // Extra space for transformations
    result = malloc(len);
    if (!result) return NULL;
    
    strcpy(result, text);
    
    // Replace "your footsteps" -> "footsteps"
    while ((pos = strstr(result, "your footsteps")) != NULL) {
        memmove(pos, pos + 5, strlen(pos + 5) + 1);
    }
    
    // Replace "you have stepped" -> "one steps"
    while ((pos = strstr(result, "you have stepped")) != NULL) {
        memmove(pos + 9, pos + 16, strlen(pos + 16) + 1);
        memcpy(pos, "one steps", 9);
    }
    
    // Replace "you" at beginning of sentences -> "The observer"
    pos = result;
    while ((pos = strstr(pos, "you ")) != NULL) {
        if (pos == result || *(pos - 1) == '.' || *(pos - 1) == ' ') {
            // This is start of sentence or after period
            char temp[MAX_STRING_LENGTH];
            strcpy(temp, pos + 4);
            strcpy(pos, "the area ");
            strcat(pos, temp);
            pos += 9;
        } else {
            pos += 4;
        }
    }
    
    return result;
}

/* ====================================================================== */
/*                     COMPREHENSIVE DESCRIPTION LOADING                 */
/* ====================================================================== */

/**
 * Load comprehensive region description from database
 */
char *load_comprehensive_region_description(int region_vnum) {
    MYSQL_RES *result;
    MYSQL_ROW row;
    char query[MAX_STRING_LENGTH];
    char *description = NULL;
    
    sprintf(query, 
        "SELECT region_description, description_style, description_length, "
        "has_historical_context, has_resource_info, has_wildlife_info, "
        "has_geological_info, has_cultural_info, is_approved "
        "FROM region_data WHERE vnum = %d AND region_description IS NOT NULL",
        region_vnum);
    
    if (!mysql_pool) {
        log("SYSERR: No database connection available for loading comprehensive region description");
        return NULL;
    }
    
    if (mysql_pool_query(query, &result)) {
        log("SYSERR: MySQL query error in load_comprehensive_region_description");
        return NULL;
    }
    
    if (!result) {
        log("SYSERR: MySQL store result error in load_comprehensive_region_description");
        return NULL;
    }
    
    if ((row = mysql_fetch_row(result))) {
        if (row[0] && *row[0]) {
            description = strdup(row[0]);
            log("DEBUG: Loaded comprehensive description for region %d (style: %s, length: %s, quality: %s)", 
                region_vnum,
                row[1] ? row[1] : "unknown",
                row[2] ? row[2] : "unknown",
                row[8] ? row[8] : "0");
        }
    }
    
    mysql_pool_free_result(result);
    return description;
}

/* ====================================================================== */
/*                         HINT LOADING & FILTERING                      */
/* ====================================================================== */

/**
 * Load and filter relevant hints for current conditions
 */
struct region_hint *load_contextual_hints(int region_vnum, const char *weather_condition, const char *time_category) {
    MYSQL_RES *result;
    MYSQL_ROW row;
    char query[MAX_STRING_LENGTH];
    struct region_hint *hints = NULL;
    struct region_hint *current_hint = NULL;
    int hint_count = 0;
    
    sprintf(query,
        "SELECT hint_category, hint_text, priority "
        "FROM region_hints "
        "WHERE region_vnum = %d AND is_active = 1 "
        "AND (weather_conditions IS NULL OR weather_conditions = '' OR FIND_IN_SET('%s', weather_conditions) > 0) "
        "ORDER BY priority DESC, RAND() "
        "LIMIT 10",
        region_vnum, weather_condition);
    
    if (!mysql_pool) {
        log("SYSERR: No database connection for loading contextual hints");
        return NULL;
    }
    
    if (mysql_pool_query(query, &result)) {
        log("SYSERR: MySQL query error in load_contextual_hints");
        return NULL;
    }
    
    if (!result) {
        log("SYSERR: MySQL store result error in load_contextual_hints");
        return NULL;
    }
    
    // Allocate array for hints
    hints = calloc(11, sizeof(struct region_hint)); // 10 hints + terminator
    if (!hints) {
        mysql_pool_free_result(result);
        return NULL;
    }
    
    while ((row = mysql_fetch_row(result)) && hint_count < 10) {
        current_hint = &hints[hint_count];
        
        // Copy hint category - convert from enum string to integer constant
        if (row[0]) {
            if (strcmp(row[0], "atmosphere") == 0) current_hint->hint_category = HINT_ATMOSPHERE;
            else if (strcmp(row[0], "fauna") == 0) current_hint->hint_category = HINT_FAUNA;
            else if (strcmp(row[0], "flora") == 0) current_hint->hint_category = HINT_FLORA;
            else if (strcmp(row[0], "geography") == 0) current_hint->hint_category = HINT_GEOGRAPHY;
            else if (strcmp(row[0], "weather_influence") == 0) current_hint->hint_category = HINT_WEATHER_INFLUENCE;
            else if (strcmp(row[0], "resources") == 0) current_hint->hint_category = HINT_RESOURCES;
            else if (strcmp(row[0], "landmarks") == 0) current_hint->hint_category = HINT_LANDMARKS;
            else if (strcmp(row[0], "sounds") == 0) current_hint->hint_category = HINT_SOUNDS;
            else if (strcmp(row[0], "scents") == 0) current_hint->hint_category = HINT_SCENTS;
            else if (strcmp(row[0], "seasonal_changes") == 0) current_hint->hint_category = HINT_SEASONAL_CHANGES;
            else if (strcmp(row[0], "time_of_day") == 0) current_hint->hint_category = HINT_TIME_OF_DAY;
            else if (strcmp(row[0], "mystical") == 0) current_hint->hint_category = HINT_MYSTICAL;
            else current_hint->hint_category = 0; // Default/unknown
        }
        
        // Copy and transform hint text
        if (row[1]) {
            current_hint->hint_text = transform_voice_to_observational(row[1]);
        }
        
        // Copy priority
        if (row[2]) {
            current_hint->priority = atoi(row[2]);
        }
        
        hint_count++;
    }
    
    mysql_pool_free_result(result);
    
    if (hint_count == 0) {
        free(hints);
        return NULL;
    }
    
    log("DEBUG: Loaded %d contextual hints for region %d (weather: %s)", 
        hint_count, region_vnum, weather_condition);
    
    return hints;
}

/* ====================================================================== */
/*                         NARRATIVE WEAVING CORE                        */
/* ====================================================================== */

/**
 * Intelligently weave hints into unified description
 */
char *weave_unified_description(const char *base_description, struct region_hint *hints, 
                               const char *weather_condition, const char *time_category, int x, int y) {
    char *unified;
    int i, j;
    int atmosphere_added = 0;
    int wildlife_added = 0;
    int weather_added = 0;
    
    unified = malloc(MAX_STRING_LENGTH * 3);
    if (!unified) return NULL;
    
    // Build natural description from hints and environmental context
    strcpy(unified, "");
    int sentence_count = 0;
    int used_hints[50];  // Track which hints we've used
    int used_count = 0;
    
    // Initialize random seed based on coordinates for location consistency
    srand(x * 1000 + y + time_info.hours);
    
    // Start with atmospheric foundation (randomly select from available)
    int atmosphere_hints[10];
    int atmosphere_count = 0;
    for (i = 0; hints && hints[i].hint_text && atmosphere_count < 10; i++) {
        if (hints[i].hint_category == HINT_ATMOSPHERE) {
            atmosphere_hints[atmosphere_count++] = i;
        }
    }
    if (atmosphere_count > 0 && !atmosphere_added) {
        int selected = atmosphere_hints[rand() % atmosphere_count];
        if (sentence_count > 0) strcat(unified, " ");
        strcat(unified, hints[selected].hint_text);
        used_hints[used_count++] = selected;
        atmosphere_added = 1;
        sentence_count++;
    }
    
    // Add weather influence based on conditions - be more inclusive
    if (!weather_added) {
        // Include cloudy weather effects too, not just rainy/stormy
        if (strcmp(weather_condition, "rainy") == 0 || 
            strcmp(weather_condition, "stormy") == 0 ||
            strcmp(weather_condition, "cloudy") == 0) {
            
            int weather_hints[10];
            int weather_count = 0;
            int already_used, j;
            
            for (i = 0; hints && hints[i].hint_text && weather_count < 10; i++) {
                if (hints[i].hint_category == HINT_WEATHER_INFLUENCE) {
                    // Check if already used
                    already_used = 0;
                    for (j = 0; j < used_count; j++) {
                        if (used_hints[j] == i) {
                            already_used = 1;
                            break;
                        }
                    }
                    if (!already_used) {
                        weather_hints[weather_count++] = i;
                    }
                }
            }
            if (weather_count > 0) {
                int selected = weather_hints[rand() % weather_count];
                if (sentence_count > 0) strcat(unified, " ");
                strcat(unified, hints[selected].hint_text);
                used_hints[used_count++] = selected;
                weather_added = 1;
                sentence_count++;
            }
        }
    }
    
    // Add wildlife/fauna during active times or randomly at night
    if (!wildlife_added && (strcmp(time_category, "morning") == 0 || 
                           strcmp(time_category, "evening") == 0 || 
                           strcmp(time_category, "night") == 0)) {
        // Night has 50% chance, others always
        if (strcmp(time_category, "night") != 0 || (rand() % 2 == 0)) {
            int fauna_hints[10];
            int fauna_count = 0;
            int already_used, j;
            
            for (i = 0; hints && hints[i].hint_text && fauna_count < 10; i++) {
                if (hints[i].hint_category == HINT_FAUNA) {
                    // Check if already used
                    already_used = 0;
                    for (j = 0; j < used_count; j++) {
                        if (used_hints[j] == i) {
                            already_used = 1;
                            break;
                        }
                    }
                    if (!already_used) {
                        fauna_hints[fauna_count++] = i;
                    }
                }
            }
            if (fauna_count > 0) {
                int selected = fauna_hints[rand() % fauna_count];
                if (sentence_count > 0) strcat(unified, " ");
                strcat(unified, hints[selected].hint_text);
                used_hints[used_count++] = selected;
                wildlife_added = 1;
                sentence_count++;
            }
        }
    }
    
    // Add flora/vegetation details for richness
    if (sentence_count < 4 && (rand() % 3 != 0)) {  // 66% chance
        int flora_hints[10];
        int flora_count = 0;
        int already_used, j;
        
        for (i = 0; hints && hints[i].hint_text && flora_count < 10; i++) {
            if (hints[i].hint_category == HINT_FLORA) {
                // Check if already used
                already_used = 0;
                for (j = 0; j < used_count; j++) {
                    if (used_hints[j] == i) {
                        already_used = 1;
                        break;
                    }
                }
                if (!already_used) {
                    flora_hints[flora_count++] = i;
                }
            }
        }
        if (flora_count > 0) {
            int selected = flora_hints[rand() % flora_count];
            if (sentence_count > 0) strcat(unified, " ");
            strcat(unified, hints[selected].hint_text);
            used_hints[used_count++] = selected;
            sentence_count++;
        }
    }
    
    // Add sensory details (sounds, scents) for immersion
    if (sentence_count < 5 && (rand() % 2 == 0)) {  // 50% chance
        int sensory_hints[20];
        int sensory_count = 0;
        int already_used, j;
        
        for (i = 0; hints && hints[i].hint_text && sensory_count < 20; i++) {
            if ((hints[i].hint_category == HINT_SOUNDS || 
                hints[i].hint_category == HINT_SCENTS)) {
                // Check if already used
                already_used = 0;
                for (j = 0; j < used_count; j++) {
                    if (used_hints[j] == i) {
                        already_used = 1;
                        break;
                    }
                }
                if (!already_used) {
                    sensory_hints[sensory_count++] = i;
                }
            }
        }
        if (sensory_count > 0) {
            int selected = sensory_hints[rand() % sensory_count];
            if (sentence_count > 0) strcat(unified, " ");
            strcat(unified, hints[selected].hint_text);
            used_hints[used_count++] = selected;
            sentence_count++;
        }
    }
    
    // Occasionally add mystical elements for atmosphere
    if (sentence_count < 4 && (rand() % 3 == 0)) {  // 33% chance
        int mystical_hints[10];
        int mystical_count = 0;
        int already_used, j;
        
        for (i = 0; hints && hints[i].hint_text && mystical_count < 10; i++) {
            if (hints[i].hint_category == HINT_MYSTICAL) {
                // Check if already used
                already_used = 0;
                for (j = 0; j < used_count; j++) {
                    if (used_hints[j] == i) {
                        already_used = 1;
                        break;
                    }
                }
                if (!already_used) {
                    mystical_hints[mystical_count++] = i;
                }
            }
        }
        if (mystical_count > 0) {
            int selected = mystical_hints[rand() % mystical_count];
            if (sentence_count > 0) strcat(unified, " ");
            strcat(unified, hints[selected].hint_text);
            used_hints[used_count++] = selected;
            sentence_count++;
        }
    }
    
    // Ensure we have at least something
    if (strlen(unified) < 10) {
        strcpy(unified, "The ancient forest spreads around you, moss-covered trees creating a mystical canopy overhead.");
    }
    
    // Ensure description ends properly
    if (strlen(unified) > 0 && unified[strlen(unified) - 1] != '.') {
        strcat(unified, ".");
    }
    
    log("DEBUG: Wove unified description: atmosphere=%d, wildlife=%d, weather=%d", 
        atmosphere_added, wildlife_added, weather_added);
    
    return unified;
}

/**
 * Clean up hints memory
 */
void free_contextual_hints(struct region_hint *hints) {
    int i;
    
    if (!hints) return;
    
    for (i = 0; hints[i].hint_text; i++) {
        if (hints[i].hint_text) {
            free(hints[i].hint_text);
        }
    }
    
    free(hints);
}

/* ====================================================================== */
/*                         PUBLIC INTERFACE                              */
/* ====================================================================== */

/**
 * Main function to create unified wilderness description
 * Uses wilderness weather system and template-free narrative weaving
 */
char *create_unified_wilderness_description(zone_rnum zone, int x, int y) {
    struct region_list *regions = NULL;
    struct region_hint *hints = NULL;
    char *unified_description = NULL;
    int region_vnum = 0;
    const char *weather_condition;
    const char *time_category;
    
    // Get wilderness weather (Perlin noise based, 0-255)
    weather_condition = get_wilderness_weather_condition(x, y);
    time_category = get_time_of_day_category();
    
    log("DEBUG: Creating unified description for (%d, %d) in zone %d - weather: %s, time: %s", 
        x, y, zone, weather_condition, time_category);
    
    // Get region information using the correct zone
    regions = get_enclosing_regions(zone, x, y);
    if (!regions) {
        log("DEBUG: get_enclosing_regions returned NULL for coordinates (%d, %d)", x, y);
        return NULL;
    }
    
    // Find the best region for descriptions - prefer geographic regions over encounter regions
    struct region_list *curr_region = regions;
    struct region_list *best_region = NULL;
    struct region_list *geographic_region = NULL;
    struct region_list *encounter_region = NULL;
    
    while (curr_region) {
        if (curr_region->rnum != NOWHERE && curr_region->rnum >= 0) {
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
    
    // Prefer geographic regions for comprehensive descriptions
    if (geographic_region) {
        best_region = geographic_region;
        log("DEBUG: Using geographic region for descriptions");
    } else if (encounter_region) {
        best_region = encounter_region;
        log("DEBUG: Using encounter region for descriptions");
    } else {
        log("DEBUG: No suitable region found for descriptions");
        free_region_list(regions);
        return NULL;
    }
    
    region_vnum = region_table[best_region->rnum].vnum;
    log("DEBUG: Selected region vnum %d from region_table[%d] for descriptions", region_vnum, best_region->rnum);
    
    // Load contextual hints for current conditions
    hints = load_contextual_hints(region_vnum, weather_condition, time_category);
    
    // Create unified description from environmental data and hints (no base template)
    unified_description = weave_unified_description(NULL, hints, weather_condition, time_category, x, y);
    
    // Cleanup
    if (hints) free_contextual_hints(hints);
    if (regions) free_region_list(regions);
    
    return unified_description;
}

/**
 * Enhanced wilderness description with unified narrative weaving
 * This is the main integration point with the existing description engine
 */
char *enhanced_wilderness_description_unified(struct char_data *ch, zone_rnum zone, int x, int y) {
    char *unified_desc;
    char *fallback_desc;
    
    // Try unified description first
    unified_desc = create_unified_wilderness_description(zone, x, y);
    if (unified_desc && strlen(unified_desc) > 10) {
        log("DEBUG: Generated unified description (%d chars) for (%d, %d)", 
            (int)strlen(unified_desc), x, y);
        return unified_desc;
    }
    
    // Fallback: return a basic message if unified description fails
    log("DEBUG: Unified description system failed for (%d, %d)", x, y);
    fallback_desc = strdup("The wilderness stretches before you.");
    
    if (unified_desc) free(unified_desc);
    return fallback_desc;
}
