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
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "structs.h"
#include "utils.h"
#include "db.h"
#include "wilderness.h"
#include "mysql.h"
#include "region_hints.h"
#include "resource_descriptions.h"
#include "systems/narrative_weaver/narrative_weaver.h"

/* External function declarations */
extern struct region_list *get_enclosing_regions(zone_rnum zone, int x, int y);
extern void free_region_list(struct region_list *regions);
extern int get_weather(int x, int y);
extern char *generate_resource_aware_description(struct char_data *ch, room_rnum room);
extern struct time_info_data time_info;
extern struct region_data *region_table;
extern zone_rnum top_of_region_table;
extern void safe_strcat(char *dest, const char *src);  /* From resource_descriptions.c */
extern struct region_profile *load_region_profile(int region_vnum);  /* From region_hints.c */
extern void free_region_profile(struct region_profile *profile);  /* From region_hints.c */
extern int mysql_pool_query(const char *query, MYSQL_RES **result);  /* From mysql.c */
extern void mysql_pool_free_result(MYSQL_RES *result);  /* From mysql.c */

/* Forward declarations */
char *simple_hint_layering(char *base_description, struct region_hint *hints, int x, int y);
void free_contextual_hints(struct region_hint *hints);

/* Season constants - matching the standard MUD seasonal system */
#define SEASON_SPRING 0
#define SEASON_SUMMER 1
#define SEASON_AUTUMN 2
#define SEASON_WINTER 3

/* Regional style constants - matching region_profile.description_style */
#define STYLE_POETIC 0
#define STYLE_PRACTICAL 1
#define STYLE_MYSTERIOUS 2
#define STYLE_DRAMATIC 3
#define STYLE_PASTORAL 4

/* Helper function to get current season */
int get_season_from_time(void) {
    extern struct time_info_data time_info;
    
    // Standard MUD seasonal calculation based on month
    int month = time_info.month;
    
    if (month >= 2 && month <= 4) return SEASON_SPRING;   /* Mar-May */
    if (month >= 5 && month <= 7) return SEASON_SUMMER;   /* Jun-Aug */
    if (month >= 8 && month <= 10) return SEASON_AUTUMN;  /* Sep-Nov */
    return SEASON_WINTER;                                  /* Dec-Feb */
}

/* ====================================================================== */
/*                          JSON PARSING UTILITIES                       */
/* ====================================================================== */

/**
 * Parse a double value from simple JSON format
 * Example: {"spring": 0.9, "summer": 1.0} - get value for "spring"
 */
double parse_json_double_value(const char *json, const char *key) {
    char search_key[100];
    char *start, *end;
    
    if (!json || !key) return 1.0;
    
    // Create search pattern: "key": 
    snprintf(search_key, sizeof(search_key), "\"%s\":", key);
    
    start = strstr(json, search_key);
    if (!start) return 1.0;
    
    // Move past the key and find the value
    start += strlen(search_key);
    while (*start && (*start == ' ' || *start == '\t')) start++; // Skip whitespace
    
    // Find end of number (space, comma, or brace)
    end = start;
    while (*end && *end != ',' && *end != '}' && *end != ' ' && *end != '\t') end++;
    
    // Extract and convert
    char value_str[32];
    int len = end - start;
    if (len >= sizeof(value_str)) len = sizeof(value_str) - 1;
    strncpy(value_str, start, len);
    value_str[len] = '\0';
    
    return atof(value_str);
}

/**
 * Get seasonal weight multiplier for a hint
 */
double get_seasonal_weight_for_hint(const char *seasonal_json, int season) {
    if (!seasonal_json) return 1.0;
    
    switch (season) {
        case SEASON_SPRING: return parse_json_double_value(seasonal_json, "spring");
        case SEASON_SUMMER: return parse_json_double_value(seasonal_json, "summer");
        case SEASON_AUTUMN: return parse_json_double_value(seasonal_json, "autumn");
        case SEASON_WINTER: return parse_json_double_value(seasonal_json, "winter");
        default: return 1.0;
    }
}

/**
 * Get time of day weight multiplier for a hint
 */
double get_time_weight_for_hint(const char *time_json, const char *time_category) {
    if (!time_json || !time_category) return 1.0;
    
    return parse_json_double_value(time_json, time_category);
}

/* ====================================================================== */
/*                    SEMANTIC NARRATIVE STRUCTURES                      */
/* ====================================================================== */

/* Semantic elements extracted from hints for narrative integration */
struct narrative_elements {
    char *dominant_mood;        /* "mysterious", "peaceful", "ominous" */
    char *primary_imagery;      /* "ancient trees", "rolling hills" */
    char *active_elements;      /* "wind whispers", "shadows dance" */
    char *sensory_details;      /* "moss-scented air", "distant calls" */
    char *temporal_aspects;     /* "dawn light", "evening mist" */
    float integration_weight;   /* Strength of influence (0.0-1.0) */
    int regional_style;         /* Regional style for flow and transitions */
};

/* Description components for semantic modification */
struct description_components {
    char *opening_imagery;      /* First visual elements */
    char *primary_nouns;        /* Main subject matter */
    char *descriptive_verbs;    /* Action words */
    char *atmospheric_modifiers; /* Mood-setting adjectives */
    char *sensory_additions;    /* Sound, scent, texture details */
    char *closing_elements;     /* Final atmospheric touches */
};

/* Mood transformation patterns */
static const char *mood_indicators[] = {
    "mysterious", "mystical", "enigmatic", "shadowy", "hidden",
    "peaceful", "tranquil", "serene", "calm", "gentle",
    "ominous", "foreboding", "dark", "threatening", "menacing",
    "vibrant", "lively", "energetic", "bustling", "dynamic",
    NULL
};

/* Action verb patterns for dynamic element detection */
static const char *action_verbs[] = {
    "whisper", "murmur", "rustle", "sway", "dance",
    "tower", "loom", "stretch", "reach", "rise",
    "flow", "cascade", "trickle", "babble", "gurgle",
    "call", "cry", "sing", "chirp", "echo",
    NULL
};

/* Style-specific vocabulary dictionaries */
static const char *poetic_adjectives[] = {
    "ethereal", "luminous", "graceful", "sublime", "gossamer",
    "shimmering", "melodious", "enchanting", "serene", "mystical",
    NULL
};

static const char *poetic_verbs[] = {
    "whisper", "dance", "caress", "embrace", "weave",
    "flutter", "shimmer", "glisten", "murmur", "sigh",
    NULL
};

static const char *mysterious_adjectives[] = {
    "enigmatic", "shadowy", "veiled", "hidden", "ancient",
    "forgotten", "elusive", "secretive", "cryptic", "arcane",
    NULL
};

static const char *mysterious_verbs[] = {
    "lurk", "conceal", "shroud", "mask", "veil",
    "hint", "suggest", "cloak", "obscure", "whisper",
    NULL
};

static const char *dramatic_adjectives[] = {
    "towering", "imposing", "magnificent", "commanding", "majestic",
    "overwhelming", "thunderous", "colossal", "fierce", "tempestuous",
    NULL
};

static const char *dramatic_verbs[] = {
    "loom", "dominate", "tower", "command", "surge",
    "thunder", "roar", "clash", "blaze", "overwhelm",
    NULL
};

static const char *pastoral_adjectives[] = {
    "gentle", "peaceful", "tranquil", "soft", "tender",
    "warm", "comforting", "welcoming", "mild", "soothing",
    NULL
};

static const char *pastoral_verbs[] = {
    "nestle", "cradle", "embrace", "shelter", "comfort",
    "nourish", "protect", "welcome", "soothe", "calm",
    NULL
};

static const char *practical_adjectives[] = {
    "sturdy", "solid", "reliable", "functional", "useful",
    "durable", "efficient", "practical", "straightforward", "clear",
    NULL
};

static const char *practical_verbs[] = {
    "support", "provide", "offer", "supply", "serve",
    "function", "operate", "work", "sustain", "maintain",
    NULL
};

/* Function prototypes after structure definitions */
void inject_temporal_and_sensory_elements(struct description_components *desc, 
                                        const char *temporal_aspects, 
                                        const char *sensory_details);
const char **get_style_adjectives(int style);
const char **get_style_verbs(int style);
char *apply_regional_style_transformation(const char *text, int style);
int convert_style_string_to_int(const char *style_str);
const char *get_transitional_phrase(int style, const char *context);

/* ====================================================================== */
/*                       TRANSITIONAL PHRASE SYSTEM                      */
/* ====================================================================== */

/* Transitional phrases for natural flow between sentences */
static const char *atmospheric_transitions[] = {
    "Meanwhile, ", "Nearby, ", "Around you, ", "In the distance, ",
    "Overhead, ", "Throughout the area, ", "", NULL
};

static const char *sensory_transitions[] = {
    "Here, ", "Softly, ", "All around, ", "Quietly, ", 
    "In this place, ", "", NULL
};

static const char *mysterious_transitions[] = {
    "Mysteriously, ", "In shadows, ", "Subtly, ", "Eerily, ",
    "With ancient presence, ", "", NULL
};

static const char *dramatic_transitions[] = {
    "Majestically, ", "Boldly, ", "Powerfully, ", "Impressively, ",
    "With great presence, ", "", NULL
};

/**
 * Get appropriate transitional phrase based on style and context
 */
const char *get_transitional_phrase(int style, const char *context) {
    if (!context) return "";
    
    // Use empty transition 50% of the time for natural variation
    if (rand() % 2 == 0) {
        return "";
    }
    
    // If context contains sensory words, use sensory transitions
    if (strstr(context, "scent") || strstr(context, "sound") || strstr(context, "aroma") ||
        strstr(context, "whisper") || strstr(context, "echo") || strstr(context, "drip") ||
        strstr(context, "silence") || strstr(context, "wind")) {
        int count = 0;
        while (sensory_transitions[count]) count++;
        return sensory_transitions[rand() % count];
    }
    
    // Style-specific transitions
    switch (style) {
        case STYLE_MYSTERIOUS:
            {
                int count = 0;
                while (mysterious_transitions[count]) count++;
                return mysterious_transitions[rand() % count];
            }
        case STYLE_DRAMATIC:
            {
                int count = 0;
                while (dramatic_transitions[count]) count++;
                return dramatic_transitions[rand() % count];
            }
        default:
            {
                int count = 0;
                while (atmospheric_transitions[count]) count++;
                return atmospheric_transitions[rand() % count];
            }
    }
}

/* ====================================================================== */
/*                       REGIONAL STYLE HELPERS                          */
/* ====================================================================== */

/**
 * Convert style string from database to integer constant
 */
int convert_style_string_to_int(const char *style_str) {
    if (!style_str) return STYLE_POETIC;
    
    if (strcmp(style_str, "poetic") == 0) return STYLE_POETIC;
    if (strcmp(style_str, "practical") == 0) return STYLE_PRACTICAL;
    if (strcmp(style_str, "mysterious") == 0) return STYLE_MYSTERIOUS;
    if (strcmp(style_str, "dramatic") == 0) return STYLE_DRAMATIC;
    if (strcmp(style_str, "pastoral") == 0) return STYLE_PASTORAL;
    
    return STYLE_POETIC; // Default fallback
}

/**
 * Get style-specific adjectives for given regional style
 */
const char **get_style_adjectives(int style) {
    switch (style) {
        case STYLE_POETIC:      return poetic_adjectives;
        case STYLE_MYSTERIOUS:  return mysterious_adjectives;
        case STYLE_DRAMATIC:    return dramatic_adjectives;
        case STYLE_PASTORAL:    return pastoral_adjectives;
        case STYLE_PRACTICAL:   return practical_adjectives;
        default:                return poetic_adjectives;
    }
}

/**
 * Get style-specific verbs for given regional style
 */
const char **get_style_verbs(int style) {
    switch (style) {
        case STYLE_POETIC:      return poetic_verbs;
        case STYLE_MYSTERIOUS:  return mysterious_verbs;
        case STYLE_DRAMATIC:    return dramatic_verbs;
        case STYLE_PASTORAL:    return pastoral_verbs;
        case STYLE_PRACTICAL:   return practical_verbs;
        default:                return poetic_verbs;
    }
}

/**
 * Apply style-specific transformation to text
 */
char *apply_regional_style_transformation(const char *text, int style) {
    char *transformed;
    const char **style_adjectives, **style_verbs;
    
    if (!text) return NULL;
    
    transformed = strdup(text);
    if (!transformed) return NULL;
    
    style_adjectives = get_style_adjectives(style);
    style_verbs = get_style_verbs(style);
    
    // Apply style-specific transformations based on style type
    switch (style) {
        case STYLE_MYSTERIOUS:
            // Add mysterious prefixes and subtle language
            if (strstr(transformed, "trees") && !strstr(transformed, "ancient")) {
                char *temp = malloc(strlen(transformed) + 20);
                if (temp) {
                    snprintf(temp, strlen(transformed) + 20, "ancient %s", transformed);
                    free(transformed);
                    transformed = temp;
                }
            }
            break;
            
        case STYLE_DRAMATIC:
            // Enhance with powerful, imposing language
            if (strstr(transformed, "hills") || strstr(transformed, "mountains")) {
                char *temp = malloc(strlen(transformed) + 30);
                if (temp) {
                    char *pos = strstr(transformed, "hills");
                    if (!pos) pos = strstr(transformed, "mountains");
                    if (pos) {
                        *pos = '\0';
                        snprintf(temp, strlen(transformed) + 30, "%stowering %s", transformed, pos);
                        free(transformed);
                        transformed = temp;
                    }
                }
            }
            break;
            
        case STYLE_PASTORAL:
            // Add gentle, comforting language
            if (strstr(transformed, "grass") && !strstr(transformed, "gentle")) {
                char *temp = malloc(strlen(transformed) + 20);
                if (temp) {
                    snprintf(temp, strlen(transformed) + 20, "gentle %s", transformed);
                    free(transformed);
                    transformed = temp;
                }
            }
            break;
            
        case STYLE_PRACTICAL:
            // Keep language straightforward and functional
            // No embellishment needed for practical style
            break;
            
        case STYLE_POETIC:
        default:
            // Add flowing, artistic language
            if (strstr(transformed, "wind") && !strstr(transformed, "gentle")) {
                char *temp = malloc(strlen(transformed) + 20);
                if (temp) {
                    snprintf(temp, strlen(transformed) + 20, "gentle %s", transformed);
                    free(transformed);
                    transformed = temp;
                }
            }
            break;
    }
    
    return transformed;
}

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
 * Get time of day category for contextual descriptions
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

/* ====================================================================== */
/*                     SEMANTIC ANALYSIS FUNCTIONS                       */
/* ====================================================================== */

/**
 * Extract narrative elements from regional hints for semantic integration
 */
struct narrative_elements *extract_narrative_elements(struct region_hint *hints, 
                                                    const char *weather, const char *time,
                                                    int region_vnum) {
    struct narrative_elements *elements;
    struct region_profile *profile = NULL;
    char *text;
    int i, j;
    int regional_style = STYLE_POETIC; // Default style
    
    if (!hints) return NULL;
    
    // Load regional style profile
    if (region_vnum > 0) {
        profile = load_region_profile(region_vnum);
        if (profile) {
            // The current region_hints.c converts string to int incorrectly with atoi()
            // We need to query the database directly to get the string value
            MYSQL_RES *result = NULL;
            MYSQL_ROW row;
            char query[512];
            
            snprintf(query, sizeof(query),
                "SELECT description_style FROM region_profiles WHERE region_vnum = %d",
                region_vnum);
            
            if (!mysql_pool_query(query, &result) && result) {
                if ((row = mysql_fetch_row(result))) {
                    regional_style = convert_style_string_to_int(row[0]);
                    log("DEBUG: Using regional style %d ('%s') for region %d", 
                        regional_style, row[0] ? row[0] : "NULL", region_vnum);
                }
                mysql_pool_free_result(result);
            } else {
                // Fallback to the incorrectly converted value
                regional_style = profile->description_style;
                log("DEBUG: Using fallback regional style %d for region %d", regional_style, region_vnum);
            }
        }
    }
    
    elements = calloc(1, sizeof(struct narrative_elements));
    if (!elements) return NULL;
    
    // Store regional style for later use in flow generation
    elements->regional_style = regional_style;
    
    // Initialize integration weight
    elements->integration_weight = 0.0f;
    
    // Analyze each hint for semantic patterns with style awareness
    for (i = 0; hints[i].hint_text; i++) {
        text = hints[i].hint_text;
        if (!text) continue;
        
        // Extract mood indicators with style preference
        if (!elements->dominant_mood) {
            const char **style_adjectives = get_style_adjectives(regional_style);
            
            // First try style-specific mood indicators
            for (j = 0; style_adjectives[j]; j++) {
                if (strstr(text, style_adjectives[j])) {
                    elements->dominant_mood = strdup(style_adjectives[j]);
                    elements->integration_weight += 0.35f; // Higher weight for style match
                    break;
                }
            }
            
            // Fallback to general mood indicators if no style match
            if (!elements->dominant_mood) {
                for (j = 0; mood_indicators[j]; j++) {
                    if (strstr(text, mood_indicators[j])) {
                        elements->dominant_mood = strdup(mood_indicators[j]);
                        elements->integration_weight += 0.3f;
                        break;
                    }
                }
            }
        }
        
        // Extract action verbs with style preference
        if (!elements->active_elements) {
            const char **style_verbs = get_style_verbs(regional_style);
            
            // First try style-specific verbs
            for (j = 0; style_verbs[j]; j++) {
                if (strstr(text, style_verbs[j])) {
                    elements->active_elements = strdup(style_verbs[j]);
                    elements->integration_weight += 0.3f; // Higher weight for style match
                    break;
                }
            }
            
            // Fallback to general action verbs if no style match
            if (!elements->active_elements) {
                for (j = 0; action_verbs[j]; j++) {
                    if (strstr(text, action_verbs[j])) {
                        elements->active_elements = strdup(action_verbs[j]);
                        elements->integration_weight += 0.25f;
                        break;
                    }
                }
            }
        }
        
        // Extract primary imagery based on hint categories with style transformation
        if (!elements->primary_imagery && hints[i].hint_category == HINT_FLORA) {
            // Extract tree/plant descriptors and apply style transformation
            if (strstr(text, "ancient") || strstr(text, "towering") || strstr(text, "massive")) {
                char *styled_imagery = apply_regional_style_transformation("ancient", regional_style);
                elements->primary_imagery = styled_imagery;
                elements->integration_weight += 0.25f; // Higher weight for style transformation
            } else if (strstr(text, "delicate") || strstr(text, "graceful") || strstr(text, "slender")) {
                char *styled_imagery = apply_regional_style_transformation("delicate", regional_style);
                elements->primary_imagery = styled_imagery;
                elements->integration_weight += 0.25f;
            }
        }
        
        // Extract sensory details
        if (!elements->sensory_details) {
            if (hints[i].hint_category == HINT_SOUNDS) {
                elements->sensory_details = strdup(text);
                elements->integration_weight += 0.15f;
            } else if (hints[i].hint_category == HINT_SCENTS) {
                elements->sensory_details = strdup(text);
                elements->integration_weight += 0.15f;
            }
        }
        
        // Extract temporal aspects - enhanced for time of day
        if (!elements->temporal_aspects) {
            // Check for explicit time mentions
            if (strstr(text, time)) {
                elements->temporal_aspects = strdup(text);
                elements->integration_weight += 0.15f;
            }
            // Check for time-of-day keywords
            else if ((strcmp(time, "morning") == 0 && 
                     (strstr(text, "dawn") || strstr(text, "sunrise") || strstr(text, "morning") || 
                      strstr(text, "dew") || strstr(text, "mist"))) ||
                    (strcmp(time, "afternoon") == 0 && 
                     (strstr(text, "noon") || strstr(text, "midday") || strstr(text, "bright") || 
                      strstr(text, "warm"))) ||
                    (strcmp(time, "evening") == 0 && 
                     (strstr(text, "dusk") || strstr(text, "sunset") || strstr(text, "twilight") || 
                      strstr(text, "shadows lengthen"))) ||
                    (strcmp(time, "night") == 0 && 
                     (strstr(text, "moonlight") || strstr(text, "starlight") || strstr(text, "darkness") || 
                      strstr(text, "nocturnal")))) {
                elements->temporal_aspects = strdup(text);
                elements->integration_weight += 0.2f; // Higher weight for matching time context
            }
        }
    }
    
    // Add seasonal context analysis based on current season
    extern struct time_info_data time_info;
    int season = get_season_from_time(); // We need to implement this
    
    // Re-analyze hints for seasonal context
    for (i = 0; hints[i].hint_text; i++) {
        text = hints[i].hint_text;
        if (!text) continue;
        
        // Look for seasonal descriptors that match current season
        if (season == SEASON_WINTER && 
            (strstr(text, "winter") || strstr(text, "frost") || strstr(text, "snow") || 
             strstr(text, "ice") || strstr(text, "bare") || strstr(text, "stark"))) {
            if (!elements->primary_imagery) {
                elements->primary_imagery = strdup("winter");
            }
            elements->integration_weight += 0.25f;
        }
        else if (season == SEASON_SPRING && 
                (strstr(text, "spring") || strstr(text, "bud") || strstr(text, "green") || 
                 strstr(text, "growth") || strstr(text, "bloom"))) {
            if (!elements->primary_imagery) {
                elements->primary_imagery = strdup("spring");
            }
            elements->integration_weight += 0.25f;
        }
        else if (season == SEASON_SUMMER && 
                (strstr(text, "summer") || strstr(text, "lush") || strstr(text, "verdant") || 
                 strstr(text, "abundant"))) {
            if (!elements->primary_imagery) {
                elements->primary_imagery = strdup("summer");
            }
            elements->integration_weight += 0.25f;
        }
        else if (season == SEASON_AUTUMN && 
                (strstr(text, "autumn") || strstr(text, "fall") || strstr(text, "golden") || 
                 strstr(text, "orange") || strstr(text, "dying"))) {
            if (!elements->primary_imagery) {
                elements->primary_imagery = strdup("autumn");
            }
            elements->integration_weight += 0.25f;
        }
    }
    
    // Cap integration weight at 1.0
    if (elements->integration_weight > 1.0f) {
        elements->integration_weight = 1.0f;
    }
    
    // Cleanup region profile
    if (profile) {
        free_region_profile(profile);
    }
    
    return elements;
}

/**
 * Parse base description into modifiable components
 */
struct description_components *parse_description_components(const char *base_description) {
    struct description_components *components;
    char *desc_copy;
    char *sentence;
    char *saveptr;
    
    if (!base_description) return NULL;
    
    components = calloc(1, sizeof(struct description_components));
    if (!components) return NULL;
    
    desc_copy = strdup(base_description);
    if (!desc_copy) {
        free(components);
        return NULL;
    }
    
    // Parse first sentence as opening imagery
    sentence = strtok_r(desc_copy, ".", &saveptr);
    if (sentence) {
        components->opening_imagery = strdup(sentence);
        
        // Extract nouns and verbs from opening sentence
        if (strstr(sentence, "trees") || strstr(sentence, "forest") || 
            strstr(sentence, "hills") || strstr(sentence, "mountains")) {
            components->primary_nouns = strdup("trees"); // Simplified for now
        }
        
        if (strstr(sentence, "tower") || strstr(sentence, "rise") || 
            strstr(sentence, "stretch") || strstr(sentence, "form")) {
            components->descriptive_verbs = strdup("tower"); // Simplified for now
        }
    }
    
    free(desc_copy);
    return components;
}

/**
 * Apply semantic transformations to modify description mood
 */
void transform_description_mood(struct description_components *desc, const char *target_mood) {
    if (!desc || !target_mood) return;
    
    if (strcmp(target_mood, "mysterious") == 0) {
        // Transform to mysterious mood
        if (desc->opening_imagery && strstr(desc->opening_imagery, "tall")) {
            char *new_imagery = malloc(strlen(desc->opening_imagery) + 50);
            if (new_imagery) {
                strcpy(new_imagery, desc->opening_imagery);
                // Replace "tall" with "ancient, shadow-wreathed"
                char *pos = strstr(new_imagery, "tall");
                if (pos) {
                    char temp[MAX_STRING_LENGTH];
                    *pos = '\0';
                    snprintf(temp, sizeof(temp), "%sancient, shadow-wreathed%s", 
                            new_imagery, pos + 4);
                    free(desc->opening_imagery);
                    desc->opening_imagery = strdup(temp);
                }
                free(new_imagery);
            }
        }
    } else if (strcmp(target_mood, "peaceful") == 0) {
        // Transform to peaceful mood  
        if (desc->opening_imagery && strstr(desc->opening_imagery, "dense")) {
            char *new_imagery = malloc(strlen(desc->opening_imagery) + 50);
            if (new_imagery) {
                strcpy(new_imagery, desc->opening_imagery);
                char *pos = strstr(new_imagery, "dense");
                if (pos) {
                    char temp[MAX_STRING_LENGTH];
                    *pos = '\0';
                    snprintf(temp, sizeof(temp), "%stranquil%s", 
                            new_imagery, pos + 5);
                    free(desc->opening_imagery);
                    desc->opening_imagery = strdup(temp);
                }
                free(new_imagery);
            }
        }
    }
}

/**
 * Inject dynamic elements from hints into description
 */
void inject_dynamic_elements(struct description_components *desc, const char *active_elements) {
    if (!desc || !active_elements) return;
    
    if (!desc->descriptive_verbs) return;
    
    // Enhance verbs with active elements
    if (strcmp(active_elements, "whisper") == 0 && strstr(desc->descriptive_verbs, "tower")) {
        char *enhanced_verbs = malloc(strlen(desc->descriptive_verbs) + 50);
        if (enhanced_verbs) {
            snprintf(enhanced_verbs, strlen(desc->descriptive_verbs) + 50, 
                    "whisper and %s", desc->descriptive_verbs);
            free(desc->descriptive_verbs);
            desc->descriptive_verbs = enhanced_verbs;
        }
    } else if (strcmp(active_elements, "sway") == 0) {
        char *enhanced_verbs = malloc(strlen(desc->descriptive_verbs) + 50);
        if (enhanced_verbs) {
            snprintf(enhanced_verbs, strlen(desc->descriptive_verbs) + 50, 
                    "sway as they %s", desc->descriptive_verbs);
            free(desc->descriptive_verbs);
            desc->descriptive_verbs = enhanced_verbs;
        }
    } else if (strcmp(active_elements, "cascade") == 0) {
        char *enhanced_verbs = malloc(strlen(desc->descriptive_verbs) + 50);
        if (enhanced_verbs) {
            snprintf(enhanced_verbs, strlen(desc->descriptive_verbs) + 50, 
                    "cascade as they %s", desc->descriptive_verbs);
            free(desc->descriptive_verbs);
            desc->descriptive_verbs = enhanced_verbs;
        }
    }
}

/**
 * Enhanced injection function for temporal and sensory elements
 */
void inject_temporal_and_sensory_elements(struct description_components *desc, 
                                        const char *temporal_aspects, 
                                        const char *sensory_details) {
    if (!desc) return;
    
    // Add temporal elements to opening imagery
    if (temporal_aspects && desc->opening_imagery) {
        char *temp_buffer = malloc(MAX_STRING_LENGTH * 2);
        if (temp_buffer) {
            // Use the actual extracted temporal content, not just generic prefixes
            // Check if temporal_aspects is a complete sentence/phrase
            if (strlen(temporal_aspects) > 50) {
                // Long temporal description - use it as a separate sentence before the main description
                snprintf(temp_buffer, MAX_STRING_LENGTH * 2, 
                        "%s %s", temporal_aspects, desc->opening_imagery);
            } else if (strstr(temporal_aspects, "dawn") || strstr(temporal_aspects, "morning")) {
                snprintf(temp_buffer, MAX_STRING_LENGTH * 2, 
                        "In the early morning light, %s", desc->opening_imagery);
            } else if (strstr(temporal_aspects, "dusk") || strstr(temporal_aspects, "evening")) {
                snprintf(temp_buffer, MAX_STRING_LENGTH * 2, 
                        "As evening approaches, %s", desc->opening_imagery);
            } else if (strstr(temporal_aspects, "moonlight") || strstr(temporal_aspects, "night")) {
                snprintf(temp_buffer, MAX_STRING_LENGTH * 2, 
                        "Under the cover of night, %s", desc->opening_imagery);
            } else if (strstr(temporal_aspects, "noon") || strstr(temporal_aspects, "midday")) {
                snprintf(temp_buffer, MAX_STRING_LENGTH * 2, 
                        "In the bright midday sun, %s", desc->opening_imagery);
            } else {
                // Use the actual temporal content
                snprintf(temp_buffer, MAX_STRING_LENGTH * 2, 
                        "%s %s", temporal_aspects, desc->opening_imagery);
            }
            free(desc->opening_imagery);
            desc->opening_imagery = temp_buffer;
        }
    }
    
    // Add sensory details as sensory additions
    if (sensory_details && !desc->sensory_additions) {
        desc->sensory_additions = strdup(sensory_details);
    }
}

/**
 * Reconstruct enhanced description with advanced flow and integration
 */
char *reconstruct_enhanced_description(struct description_components *components, int regional_style) {
    char *enhanced;
    char *primary_sentence, *sensory_sentence;
    char temp_buffer[MAX_STRING_LENGTH * 2];
    
    if (!components) return NULL;
    
    enhanced = malloc(MAX_STRING_LENGTH * 2);
    if (!enhanced) return NULL;
    
    enhanced[0] = '\0';
    
    if (!components->opening_imagery) {
        free(enhanced);
        return NULL;
    }
    
    // Step 1: Process the primary visual description
    primary_sentence = strdup(components->opening_imagery);
    if (!primary_sentence) {
        free(enhanced);
        return NULL;
    }
    
    // Step 2: Integrate enhanced verbs naturally
    if (components->descriptive_verbs && strstr(components->descriptive_verbs, "whisper")) {
        char *verb_pos = strstr(primary_sentence, "tower");
        if (verb_pos) {
            char before[MAX_STRING_LENGTH], after[MAX_STRING_LENGTH];
            *verb_pos = '\0';
            safe_strcpy(before, primary_sentence, sizeof(before));
            safe_strcpy(after, verb_pos + 5, sizeof(after));
            
            snprintf(temp_buffer, sizeof(temp_buffer), "%s%s%s", 
                    before, components->descriptive_verbs, after);
            free(primary_sentence);
            primary_sentence = strdup(temp_buffer);
        }
    } else if (components->descriptive_verbs && strstr(components->descriptive_verbs, "cascade")) {
        char *verb_pos = strstr(primary_sentence, "tower");
        if (verb_pos) {
            char before[MAX_STRING_LENGTH], after[MAX_STRING_LENGTH];
            *verb_pos = '\0';
            safe_strcpy(before, primary_sentence, sizeof(before));
            safe_strcpy(after, verb_pos + 5, sizeof(after));
            
            snprintf(temp_buffer, sizeof(temp_buffer), "%s%s%s", 
                    before, components->descriptive_verbs, after);
            free(primary_sentence);
            primary_sentence = strdup(temp_buffer);
        }
    }
    
    // Step 3: Ensure proper sentence termination
    size_t len = strlen(primary_sentence);
    if (len > 0 && primary_sentence[len - 1] != '.' && primary_sentence[len - 1] != '!' && primary_sentence[len - 1] != '?') {
        safe_strcat(primary_sentence, ".");
    }
    
    // Step 4: Start building the enhanced description
    safe_strcpy(enhanced, primary_sentence, MAX_STRING_LENGTH * 2);
    
    // Step 5: Add sensory details as a separate, well-integrated sentence with style-aware transitions
    if (components->sensory_additions) {
        sensory_sentence = strdup(components->sensory_additions);
        if (sensory_sentence) {
            // Get style-appropriate transitional phrase
            const char *transition = get_transitional_phrase(regional_style, sensory_sentence);
            
            // Add transitional space
            safe_strcat(enhanced, " ");
            
            if (strlen(transition) > 0) {
                // Add transition phrase
                safe_strcat(enhanced, transition);
                
                // Ensure sensory sentence starts with lowercase (since transition provides the capital)
                if (sensory_sentence[0] >= 'A' && sensory_sentence[0] <= 'Z') {
                    sensory_sentence[0] = sensory_sentence[0] - 'A' + 'a';
                }
            } else {
                // No transition - ensure sensory sentence starts with capital
                if (sensory_sentence[0] >= 'a' && sensory_sentence[0] <= 'z') {
                    sensory_sentence[0] = sensory_sentence[0] - 'a' + 'A';
                }
            }
            
            safe_strcat(enhanced, sensory_sentence);
            
            // Ensure proper termination
            len = strlen(sensory_sentence);
            if (len > 0 && sensory_sentence[len - 1] != '.' && sensory_sentence[len - 1] != '!' && sensory_sentence[len - 1] != '?') {
                safe_strcat(enhanced, ".");
            }
            
            free(sensory_sentence);
        }
    }
    
    // Step 6: Final cleanup and validation
    // Remove any double periods or spacing issues
    char *double_period = strstr(enhanced, "..");
    while (double_period) {
        memmove(double_period, double_period + 1, strlen(double_period));
        double_period = strstr(enhanced, "..");
    }
    
    // Clean up multiple spaces
    char *double_space = strstr(enhanced, "  ");
    while (double_space) {
        memmove(double_space, double_space + 1, strlen(double_space));
        double_space = strstr(enhanced, "  ");
    }
    
    free(primary_sentence);
    return enhanced;
}

/**
 * Free narrative elements structure
 */
void free_narrative_elements(struct narrative_elements *elements) {
    if (!elements) return;
    
    if (elements->dominant_mood) free(elements->dominant_mood);
    if (elements->primary_imagery) free(elements->primary_imagery);
    if (elements->active_elements) free(elements->active_elements);
    if (elements->sensory_details) free(elements->sensory_details);
    if (elements->temporal_aspects) free(elements->temporal_aspects);
    
    free(elements);
}

/**
 * Free description components structure
 */
void free_description_components(struct description_components *components) {
    if (!components) return;
    
    if (components->opening_imagery) free(components->opening_imagery);
    if (components->primary_nouns) free(components->primary_nouns);
    if (components->descriptive_verbs) free(components->descriptive_verbs);
    if (components->atmospheric_modifiers) free(components->atmospheric_modifiers);
    if (components->sensory_additions) free(components->sensory_additions);
    if (components->closing_elements) free(components->closing_elements);
    
    free(components);
}

/* ====================================================================== */
/*                         UTILITY FUNCTIONS                             */
/* ====================================================================== */

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
    int current_season = get_season_from_time();
    
    sprintf(query,
        "SELECT hint_category, hint_text, priority, seasonal_weight, time_of_day_weight "
        "FROM region_hints "
        "WHERE region_vnum = %d AND is_active = 1 "
        "AND (weather_conditions IS NULL OR weather_conditions = '' OR FIND_IN_SET('%s', weather_conditions) > 0) "
        "ORDER BY priority DESC, RAND() "
        "LIMIT 15",
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
    
    // Allocate array for hints (extra space for weighted selection)
    hints = calloc(16, sizeof(struct region_hint)); // 15 hints + terminator
    if (!hints) {
        mysql_pool_free_result(result);
        return NULL;
    }
    
    while ((row = mysql_fetch_row(result)) && hint_count < 15) {
        // Calculate weights before deciding to include this hint
        double seasonal_weight = 1.0;
        double time_weight = 1.0;
        double combined_weight = 1.0;
        
        // Get weights from JSON columns
        if (row[3]) { // seasonal_weight JSON
            seasonal_weight = get_seasonal_weight_for_hint(row[3], current_season);
        }
        if (row[4]) { // time_of_day_weight JSON
            time_weight = get_time_weight_for_hint(row[4], time_category);
        }
        
        // Combined weight (multiply factors)
        combined_weight = seasonal_weight * time_weight;
        
        // Apply minimum threshold for inclusion (0.3 = 30% relevance minimum)
        if (combined_weight < 0.3) {
            continue; // Skip this hint - not relevant enough for current conditions
        }
        
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
        
        // Copy priority and apply contextual weight multiplier
        if (row[2]) {
            current_hint->priority = atoi(row[2]) * combined_weight;
        }
        
        // Store the weight for potential future use
        current_hint->contextual_weight = combined_weight;
        
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
    
    // Add seasonal context when relevant
    if (sentence_count < 5 && (rand() % 2 == 0)) {  // 50% chance
        int seasonal_hints[10];
        int seasonal_count = 0;
        int already_used, j;
        
        for (i = 0; hints && hints[i].hint_text && seasonal_count < 10; i++) {
            if (hints[i].hint_category == HINT_SEASONAL_CHANGES) {
                // Check if already used
                already_used = 0;
                for (j = 0; j < used_count; j++) {
                    if (used_hints[j] == i) {
                        already_used = 1;
                        break;
                    }
                }
                if (!already_used) {
                    seasonal_hints[seasonal_count++] = i;
                }
            }
        }
        if (seasonal_count > 0) {
            int selected = seasonal_hints[rand() % seasonal_count];
            if (sentence_count > 0) safe_strcat(unified, " ");
            safe_strcat(unified, hints[selected].hint_text);
            used_hints[used_count++] = selected;
            sentence_count++;
        }
    }
    
    // Add time-of-day specific atmosphere (prioritize during transition times)
    if (sentence_count < 5 && (strcmp(time_category, "evening") == 0 || 
                               strcmp(time_category, "morning") == 0 || 
                               (rand() % 3 == 0))) {  // Always for transitions, 33% for others
        int time_hints[10];
        int time_count = 0;
        int already_used, j;
        
        for (i = 0; hints && hints[i].hint_text && time_count < 10; i++) {
            if (hints[i].hint_category == HINT_TIME_OF_DAY) {
                // Check if already used
                already_used = 0;
                for (j = 0; j < used_count; j++) {
                    if (used_hints[j] == i) {
                        already_used = 1;
                        break;
                    }
                }
                if (!already_used) {
                    time_hints[time_count++] = i;
                }
            }
        }
        if (time_count > 0) {
            int selected = time_hints[rand() % time_count];
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
/**
 * Layer regional hints onto existing base description using semantic integration
 * This preserves the resource/terrain foundation while seamlessly weaving regional character
 */
char *layer_hints_on_base_description(char *base_description, struct region_hint *hints,
                                     const char *weather_condition, const char *time_category,
                                     int x, int y) {
    struct narrative_elements *elements;
    struct description_components *components;
    char *semantically_enhanced;
    
    if (!base_description || !hints) {
        return NULL;
    }
    
    log("DEBUG: Starting semantic integration for location (%d, %d)", x, y);
    
    // Initialize random seed for location consistency
    srand(x * 1000 + y + time_info.hours);
    
    // Extract semantic elements from hints with regional style
    int region_vnum = hints[0].region_vnum; // Get region from first hint
    elements = extract_narrative_elements(hints, weather_condition, time_category, region_vnum);
    if (!elements) {
        log("DEBUG: No semantic elements extracted, falling back to simple layering");
        return simple_hint_layering(base_description, hints, x, y);
    }
    
    // Only apply semantic integration if we have sufficient elements
    if (elements->integration_weight < 0.3f) {
        log("DEBUG: Insufficient semantic weight (%.2f), using simple layering", 
            elements->integration_weight);
        free_narrative_elements(elements);
        return simple_hint_layering(base_description, hints, x, y);
    }
    
    log("DEBUG: Applying semantic integration with weight %.2f", elements->integration_weight);
    
    // Parse base description into components
    components = parse_description_components(base_description);
    if (!components) {
        log("DEBUG: Failed to parse description components, using simple layering");
        free_narrative_elements(elements);
        return simple_hint_layering(base_description, hints, x, y);
    }
    
    // Apply semantic transformations
    if (elements->dominant_mood) {
        log("DEBUG: Transforming mood to: %s", elements->dominant_mood);
        transform_description_mood(components, elements->dominant_mood);
    }
    
    if (elements->active_elements) {
        log("DEBUG: Injecting dynamic elements: %s", elements->active_elements);
        inject_dynamic_elements(components, elements->active_elements);
    }
    
    // Apply temporal and sensory integration
    if (elements->temporal_aspects || elements->sensory_details) {
        log("DEBUG: Injecting temporal/sensory elements - temporal: %s, sensory: %s", 
            elements->temporal_aspects ? elements->temporal_aspects : "none",
            elements->sensory_details ? elements->sensory_details : "none");
        inject_temporal_and_sensory_elements(components, elements->temporal_aspects, elements->sensory_details);
    }

    if (elements->sensory_details) {
        log("DEBUG: Adding sensory details: %s", elements->sensory_details);
        components->sensory_additions = strdup(elements->sensory_details);
    }    // Reconstruct enhanced description with regional style
    semantically_enhanced = reconstruct_enhanced_description(components, elements->regional_style);
    
    // Cleanup
    free_narrative_elements(elements);
    free_description_components(components);
    
    if (!semantically_enhanced) {
        log("DEBUG: Semantic reconstruction failed, using simple layering");
        return simple_hint_layering(base_description, hints, x, y);
    }
    
    log("DEBUG: Semantic integration completed successfully");
    return semantically_enhanced;
}

/**
 * Fallback simple hint layering for when semantic integration isn't possible
 */
char *simple_hint_layering(char *base_description, struct region_hint *hints, int x, int y) {
    char *enhanced;
    char hint_additions[MAX_STRING_LENGTH];
    int used_hints[20];
    int used_count = 0;
    int i;
    
    // Allocate buffer for enhanced description
    enhanced = malloc(MAX_STRING_LENGTH * 2);
    if (!enhanced) {
        return NULL;
    }
    
    // Start with the base description
    safe_strcpy(enhanced, base_description, MAX_STRING_LENGTH * 2);
    safe_strcpy(hint_additions, "", MAX_STRING_LENGTH);
    
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
    
    log("DEBUG: Simple layering applied %d hints to base description", used_count);
    
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
