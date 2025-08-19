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
extern char *enhance_wilderness_description_with_hints(struct char_data *ch, int x, int y);
extern struct time_info_data time_info;

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
        
        // Copy hint category
        if (row[0]) {
            strncpy(current_hint->hint_category, row[0], sizeof(current_hint->hint_category) - 1);
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
                               const char *weather_condition, const char *time_category) {
    char *unified;
    char temp_buffer[MAX_STRING_LENGTH * 2];
    int i;
    int atmosphere_added = 0;
    int wildlife_added = 0;
    int weather_added = 0;
    
    unified = malloc(MAX_STRING_LENGTH * 3);
    if (!unified) return NULL;
    
    // Start with base description if available
    if (base_description && *base_description) {
        strcpy(unified, base_description);
        
        // Add connecting phrase
        strcat(unified, " ");
    } else {
        // No base description, start fresh
        strcpy(unified, "");
    }
    
    // Weave in hints by priority and category
    for (i = 0; hints && hints[i].hint_text; i++) {
        int should_add = 0;
        
        // Prioritize atmospheric hints for mysterious/stormy weather
        if (strcmp(hints[i].hint_category, "atmosphere") == 0 && !atmosphere_added) {
            should_add = 1;
            atmosphere_added = 1;
        }
        // Add weather-specific hints
        else if (strcmp(hints[i].hint_category, "weather_influence") == 0 && !weather_added) {
            should_add = 1;
            weather_added = 1;
        }
        // Add wildlife for clear/cloudy weather
        else if (strcmp(hints[i].hint_category, "fauna") == 0 && !wildlife_added && 
                (strcmp(weather_condition, "clear") == 0 || strcmp(weather_condition, "cloudy") == 0)) {
            should_add = 1;
            wildlife_added = 1;
        }
        // Add other categories sparingly
        else if (strcmp(hints[i].hint_category, "sounds") == 0 || 
                strcmp(hints[i].hint_category, "scents") == 0 ||
                strcmp(hints[i].hint_category, "mystical") == 0) {
            should_add = (i < 3); // Only add first few
        }
        
        if (should_add && hints[i].hint_text) {
            // Add appropriate connector
            if (strlen(unified) > 0) {
                if (strstr(unified, ".") && !strstr(unified + strlen(unified) - 10, ".")) {
                    strcat(unified, ". ");
                } else {
                    strcat(unified, " ");
                }
            }
            
            strcat(unified, hints[i].hint_text);
        }
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
char *create_unified_wilderness_description(int x, int y) {
    struct region_list *regions = NULL;
    struct region_hint *hints = NULL;
    char *base_description = NULL;
    char *unified_description = NULL;
    int region_vnum = 0;
    const char *weather_condition;
    const char *time_category;
    
    // Get wilderness weather (Perlin noise based, 0-255)
    weather_condition = get_wilderness_weather_condition(x, y);
    time_category = get_time_of_day_category();
    
    log("DEBUG: Creating unified description for (%d, %d) - weather: %s, time: %s", 
        x, y, weather_condition, time_category);
    
    // Get region information - wilderness uses zone 0
    regions = get_enclosing_regions(0, x, y);
    if (regions && regions->vnum > 0) {
        region_vnum = regions->vnum;
        log("DEBUG: Found region %d at coordinates (%d, %d)", region_vnum, x, y);
    } else {
        log("DEBUG: No valid region found at coordinates (%d, %d)", x, y);
        if (regions) free_region_list(regions);
        return NULL;
    }
    
    // Load comprehensive base description
    base_description = load_comprehensive_region_description(region_vnum);
    
    // Load contextual hints for current conditions
    hints = load_contextual_hints(region_vnum, weather_condition, time_category);
    
    // Create unified description
    unified_description = weave_unified_description(base_description, hints, weather_condition, time_category);
    
    // Cleanup
    if (base_description) free(base_description);
    if (hints) free_contextual_hints(hints);
    if (regions) free_region_list(regions);
    
    return unified_description;
}

/**
 * Enhanced wilderness description with unified narrative weaving
 * This is the main integration point with the existing description engine
 */
char *enhanced_wilderness_description_unified(struct char_data *ch, int x, int y) {
    char *unified_desc;
    char *fallback_desc;
    
    // Try unified description first
    unified_desc = create_unified_wilderness_description(x, y);
    if (unified_desc && strlen(unified_desc) > 10) {
        log("DEBUG: Generated unified description (%d chars) for (%d, %d)", 
            (int)strlen(unified_desc), x, y);
        return unified_desc;
    }
    
    // Fallback to existing hint system
    log("DEBUG: Falling back to existing hint system for (%d, %d)", x, y);
    fallback_desc = enhance_wilderness_description_with_hints(ch, x, y);
    
    if (unified_desc) free(unified_desc);
    return fallback_desc;
}
