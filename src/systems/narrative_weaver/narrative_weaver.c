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
extern void safe_strcat(char *dest, const char *src);  /* From resource_descriptions.c */

/* ====================================================================== */
/*                         SAFE STRING UTILITIES                         */
/* ====================================================================== */

/**
 * Safe string copy with buffer bounds checking
 */
int safe_strcpy(char *dest, const char *src, size_t dest_size) {
    if (!dest || !src || dest_size == 0) {
        return -1;
    }
    
    size_t src_len = strlen(src);
    if (src_len >= dest_size) {
        // Truncate to fit, leaving room for null terminator
        strncpy(dest, src, dest_size - 1);
        dest[dest_size - 1] = '\0';
        return dest_size - 1;
    } else {
        strcpy(dest, src);
        return src_len;
    }
}

/**
 * Safe string concatenation with buffer bounds checking
 */
int narrative_safe_strcat(char *dest, const char *src, size_t dest_size) {
    if (!dest || !src || dest_size == 0) {
        return -1;
    }
    
    size_t dest_len = strlen(dest);
    size_t src_len = strlen(src);
    
    if (dest_len >= dest_size) {
        // Destination already full
        return -1;
    }
    
    size_t remaining = dest_size - dest_len - 1; // -1 for null terminator
    if (src_len <= remaining) {
        strcat(dest, src);
        return src_len;
    } else {
        // Truncate to fit
        strncat(dest, src, remaining);
        dest[dest_size - 1] = '\0';
        return remaining;
    }
}

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
    
    safe_strcpy(result, text, len);
    
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
            safe_strcpy(temp, pos + 4, MAX_STRING_LENGTH);
            safe_strcpy(pos, "the area ", MAX_STRING_LENGTH - (pos - result));
            strncat(pos, temp, MAX_STRING_LENGTH - (pos - result) - strlen(pos) - 1);
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
    int i;
    int atmosphere_added = 0;
    int wildlife_added = 0;
    int weather_added = 0;
    
    unified = malloc(MAX_STRING_LENGTH * 3);
    if (!unified) return NULL;
    
    // Build natural description from hints and environmental context
    safe_strcpy(unified, "", MAX_STRING_LENGTH * 3);
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
        if (sentence_count > 0) safe_strcat(unified, " ");
        safe_strcat(unified, hints[selected].hint_text);
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
                if (sentence_count > 0) safe_strcat(unified, " ");
                safe_strcat(unified, hints[selected].hint_text);
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
                if (sentence_count > 0) safe_strcat(unified, " ");
                safe_strcat(unified, hints[selected].hint_text);
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
            if (sentence_count > 0) safe_strcat(unified, " ");
            safe_strcat(unified, hints[selected].hint_text);
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
            if (sentence_count > 0) safe_strcat(unified, " ");
            safe_strcat(unified, hints[selected].hint_text);
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
            if (sentence_count > 0) safe_strcat(unified, " ");
            safe_strcat(unified, hints[selected].hint_text);
            used_hints[used_count++] = selected;
            sentence_count++;
        }
    }
    
    // Ensure we have at least something
    if (strlen(unified) < 10) {
        safe_strcpy(unified, "The ancient forest spreads around you, moss-covered trees creating a mystical canopy overhead.", MAX_STRING_LENGTH * 3);
    }
    
    // Ensure description ends properly
    if (strlen(unified) > 0 && unified[strlen(unified) - 1] != '.') {
        safe_strcat(unified, ".");
    }
    
    log("DEBUG: Wove unified description: atmosphere=%d, wildlife=%d, weather=%d", 
        atmosphere_added, wildlife_added, weather_added);
    
    return unified;
}

/* ====================================================================== */
/*                     ENHANCED INTEGRATION SYSTEM                       */
/* ====================================================================== */

/**
 * Enhance base resource-aware description with regional hints
 * This implements the core vision: base descriptions + regional specificity
 */
char *enhance_base_description_with_hints(char *base_description, struct char_data *ch, 
                                         zone_rnum zone, int x, int y) {
    struct region_list *regions = NULL;
    struct region_list *best_region = NULL;
    struct region_hint *hints = NULL;
    char *enhanced_desc = NULL;
    const char *weather_condition = NULL;
    const char *time_category = NULL;
    int region_vnum = 0;
    
    if (!base_description) {
        return NULL;
    }
    
    // Get regions for this location
    regions = get_enclosing_regions(zone, x, y);
    if (!regions) {
        log("DEBUG: No regions found for coordinates (%d, %d)", x, y);
        return NULL;
    }
    
    // Find best region for descriptions (prefer geographic regions)
    struct region_list *curr_region = regions;
    struct region_list *geographic_region = NULL;
    struct region_list *encounter_region = NULL;
    
    while (curr_region) {
        if (curr_region->rnum != NOWHERE && curr_region->rnum >= 0) {
            int region_type = region_table[curr_region->rnum].region_type;
            
            if (region_type == 1) { /* Geographic region */
                geographic_region = curr_region;
            } else if (region_type == 2) { /* Encounter region */
                encounter_region = curr_region;
            }
        }
        curr_region = curr_region->next;
    }
    
    best_region = geographic_region ? geographic_region : encounter_region;
    if (!best_region) {
        log("DEBUG: No suitable region found for hint enhancement");
        free_region_list(regions);
        return NULL;
    }
    
    region_vnum = region_table[best_region->rnum].vnum;
    log("DEBUG: Using region vnum %d for hint enhancement", region_vnum);
    
    // Get environmental context
    weather_condition = get_wilderness_weather_condition(x, y);
    time_category = get_time_of_day_category();
    
    // Load contextual hints for current conditions
    hints = load_contextual_hints(region_vnum, weather_condition, time_category);
    if (!hints) {
        log("DEBUG: No contextual hints available for region %d", region_vnum);
        free_region_list(regions);
        return NULL;
    }
    
    // Layer hints onto base description
    enhanced_desc = layer_hints_on_base_description(base_description, hints, 
                                                   weather_condition, time_category, x, y);
    
    // Cleanup
    if (hints) free_contextual_hints(hints);
    free_region_list(regions);
    
    return enhanced_desc;
}

/**
 * Layer regional hints onto existing base description
 * This preserves the resource/terrain foundation while adding regional character
 */
char *layer_hints_on_base_description(char *base_description, struct region_hint *hints,
                                     const char *weather_condition, const char *time_category,
                                     int x, int y) {
    char *enhanced;
    char hint_additions[MAX_STRING_LENGTH];
    int used_hints[20];
    int used_count = 0;
    int i;
    
    if (!base_description || !hints) {
        return NULL;
    }
    
    // Allocate buffer for enhanced description
    enhanced = malloc(MAX_STRING_LENGTH * 2);
    if (!enhanced) {
        return NULL;
    }
    
    // Start with the base description
    safe_strcpy(enhanced, base_description, MAX_STRING_LENGTH * 2);
    safe_strcpy(hint_additions, "", MAX_STRING_LENGTH);
    
    // Initialize random seed for location consistency
    srand(x * 1000 + y + time_info.hours);
    
    // Add atmospheric hints (mood/ambiance)
    int atmosphere_hints[10];
    int atmosphere_count = 0;
    for (i = 0; hints && hints[i].hint_text && atmosphere_count < 10; i++) {
        if (hints[i].hint_category == HINT_ATMOSPHERE) {
            atmosphere_hints[atmosphere_count++] = i;
        }
    }
    if (atmosphere_count > 0) {
        int selected = atmosphere_hints[rand() % atmosphere_count];
        if (strlen(hint_additions) > 0) safe_strcat(hint_additions, " ");
        safe_strcat(hint_additions, hints[selected].hint_text);
        used_hints[used_count++] = selected;
    }
    
    // Add flora specificity (tree types, vegetation details)
    int flora_hints[10];
    int flora_count = 0;
    for (i = 0; hints && hints[i].hint_text && flora_count < 10; i++) {
        if (hints[i].hint_category == HINT_FLORA) {
            // Check if already used
            int already_used = 0;
            int j;
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
        if (strlen(hint_additions) > 0) safe_strcat(hint_additions, " ");
        safe_strcat(hint_additions, hints[selected].hint_text);
        used_hints[used_count++] = selected;
    }
    
    // Add fauna details (wildlife presence)
    int fauna_hints[10];
    int fauna_count = 0;
    for (i = 0; hints && hints[i].hint_text && fauna_count < 10; i++) {
        if (hints[i].hint_category == HINT_FAUNA) {
            // Check if already used
            int already_used = 0;
            int j;
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
    if (fauna_count > 0 && (rand() % 3 != 0)) { // 66% chance to include fauna
        int selected = fauna_hints[rand() % fauna_count];
        if (strlen(hint_additions) > 0) safe_strcat(hint_additions, " ");
        safe_strcat(hint_additions, hints[selected].hint_text);
        used_hints[used_count++] = selected;
    }
    
    // Combine base description with hint additions
    if (strlen(hint_additions) > 0) {
        // Add hints as additional sentences
        safe_strcat(enhanced, " ");
        safe_strcat(enhanced, hint_additions);
        
        // Ensure proper punctuation
        if (enhanced[strlen(enhanced) - 1] != '.') {
            safe_strcat(enhanced, ".");
        }
    }
    
    log("DEBUG: Layered %d hints onto base description", used_count);
    
    return enhanced;
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
 * 
 * PHASE 1.1: Integration - Enhance rather than replace base descriptions
 */
char *enhanced_wilderness_description_unified(struct char_data *ch, room_rnum room, zone_rnum zone, int x, int y) {
    char *base_desc = NULL;
    char *enhanced_desc = NULL;
    
    // STEP 1: Generate base resource-aware description first (preserve existing foundation)
    base_desc = generate_resource_aware_description(ch, room);
    if (!base_desc) {
        log("DEBUG: Base resource description failed for (%d, %d)", x, y);
        return NULL;
    }
    
    log("DEBUG: Generated base description (%d chars) for (%d, %d)", 
        (int)strlen(base_desc), x, y);
    
    // STEP 2: Try to enhance with regional hints
    enhanced_desc = enhance_base_description_with_hints(base_desc, ch, zone, x, y);
    if (enhanced_desc) {
        log("DEBUG: Enhanced description with regional hints (%d chars) for (%d, %d)", 
            (int)strlen(enhanced_desc), x, y);
        free(base_desc);  // Replace with enhanced version
        return enhanced_desc;
    }
    
    // STEP 3: No enhancement possible, return base description
    log("DEBUG: No regional enhancement available, using base description for (%d, %d)", x, y);
    return base_desc;
}
