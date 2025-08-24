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
#include <math.h>
#include <stdarg.h>
#include "structs.h"
#include "utils.h"
#include "db.h"
#include "wilderness.h"
#include "mysql.h"
#include "region_hints.h"
#include "resource_descriptions.h"
#include "systems/narrative_weaver/narrative_weaver.h"
#include "resource_system.h"

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

/* Narrative weaver debug mode control */
static int narrative_debug_mode = 0;  /* 0 = off, 1 = basic, 2 = verbose */

/* Debug logging function - only logs if debug mode is enabled */
static void narrative_debug_log(int level, const char *format, ...) {
    va_list args;
    
    if (narrative_debug_mode >= level) {
        va_start(args, format);
        
        /* Create the debug message */
        char debug_msg[MAX_STRING_LENGTH];
        vsnprintf(debug_msg, sizeof(debug_msg), format, args);
        
        /* Use the standard log function */
        log("NARRATIVE_DEBUG: %s", debug_msg);
        
        va_end(args);
    }
}

/* Function to set narrative debug mode (for admin commands) */
void set_narrative_debug_mode(int mode) {
    narrative_debug_mode = mode;
    log("Narrative weaver debug mode set to %d", mode);
}

/* Function to get current debug mode */
int get_narrative_debug_mode(void) {
    return narrative_debug_mode;
}

/* ====================================================================== */
/*                        PERFORMANCE OPTIMIZATION                       */
/*                           HINT CACHING SYSTEM                         */
/* ====================================================================== */

/* Cache key for location-based hint caching */
struct hint_cache_key {
    int region_vnum;
    int weather_condition;
    int time_category;
    int season;
    float resource_health;  /* For resource-dependent hints */
};

/* Cached hint data with metadata */
struct hint_cache_entry {
    struct hint_cache_key key;
    struct region_hint *hints;
    time_t cache_time;
    int hint_count;
    struct hint_cache_entry *next;  /* For hash table chaining */
};

/* Hash table for hint caching */
#define HINT_CACHE_SIZE 256
#define HINT_CACHE_TTL 300   /* 5 minutes cache lifetime */
#define HINT_CACHE_MAX_ENTRIES 1000  /* Maximum cached entries */

static struct hint_cache_entry *hint_cache[HINT_CACHE_SIZE];
static int hint_cache_entries = 0;

/* Hash function for cache keys */
unsigned int hash_cache_key(struct hint_cache_key *key) {
    unsigned int hash = 5381;
    hash = ((hash << 5) + hash) + key->region_vnum;
    hash = ((hash << 5) + hash) + key->weather_condition;
    hash = ((hash << 5) + hash) + key->time_category;
    hash = ((hash << 5) + hash) + key->season;
    hash = ((hash << 5) + hash) + (int)(key->resource_health * 100); /* Quantize resource health */
    return hash % HINT_CACHE_SIZE;
}

/* Compare cache keys for equality */
int cache_keys_equal(struct hint_cache_key *key1, struct hint_cache_key *key2) {
    return (key1->region_vnum == key2->region_vnum &&
            key1->weather_condition == key2->weather_condition &&
            key1->time_category == key2->time_category &&
            key1->season == key2->season &&
            abs((int)(key1->resource_health * 100) - (int)(key2->resource_health * 100)) <= 5); /* 5% tolerance */
}

/* Duplicate hint array for caching */
struct region_hint *duplicate_hints(struct region_hint *hints) {
    if (!hints) return NULL;
    
    /* Count hints */
    int count = 0;
    while (hints[count].hint_text) count++;
    
    /* Allocate new array */
    struct region_hint *cached_hints = malloc(sizeof(struct region_hint) * (count + 1));
    if (!cached_hints) return NULL;
    
    /* Copy hints */
    int i;
    for (i = 0; i < count; i++) {
        cached_hints[i].hint_category = hints[i].hint_category;
        cached_hints[i].hint_text = strdup(hints[i].hint_text ? hints[i].hint_text : "");
        cached_hints[i].priority = hints[i].priority;
        cached_hints[i].seasonal_weight = strdup(hints[i].seasonal_weight ? hints[i].seasonal_weight : "");
        cached_hints[i].time_of_day_weight = strdup(hints[i].time_of_day_weight ? hints[i].time_of_day_weight : "");
        cached_hints[i].contextual_weight = hints[i].contextual_weight;
    }
    
    /* Null terminator */
    cached_hints[count].hint_text = NULL;
    
    return cached_hints;
}

/* Remove old cache entries to prevent memory bloat */
void cleanup_hint_cache(void) {
    time_t current_time = time(NULL);
    int removed = 0;
    int i;
    
    for (i = 0; i < HINT_CACHE_SIZE; i++) {
        struct hint_cache_entry **entry_ptr = &hint_cache[i];
        
        while (*entry_ptr) {
            struct hint_cache_entry *entry = *entry_ptr;
            
            /* Remove expired entries or if we're over limit */
            if ((current_time - entry->cache_time) > HINT_CACHE_TTL ||
                hint_cache_entries > HINT_CACHE_MAX_ENTRIES) {
                
                /* Remove from chain */
                *entry_ptr = entry->next;
                
                /* Free hint data */
                if (entry->hints) {
                    free_contextual_hints(entry->hints);
                }
                
                free(entry);
                hint_cache_entries--;
                removed++;
            } else {
                entry_ptr = &entry->next;
            }
        }
    }
    
    if (removed > 0) {
        narrative_debug_log(2, "Cleaned up %d expired hint cache entries, %d entries remaining", 
            removed, hint_cache_entries);
    }
}

/* Get hints from cache if available */
struct region_hint *get_cached_hints(struct hint_cache_key *key) {
    unsigned int hash = hash_cache_key(key);
    struct hint_cache_entry *entry = hint_cache[hash];
    time_t current_time = time(NULL);
    
    while (entry) {
        if (cache_keys_equal(&entry->key, key)) {
            /* Check if entry is still valid */
            if ((current_time - entry->cache_time) <= HINT_CACHE_TTL) {
                narrative_debug_log(2, "Cache HIT for region %d, weather %d, time %d", 
                    key->region_vnum, key->weather_condition, key->time_category);
                return duplicate_hints(entry->hints);
            } else {
                narrative_debug_log(2, "Cache entry expired for region %d", key->region_vnum);
                break;
            }
        }
        entry = entry->next;
    }
    
    narrative_debug_log(2, "Cache MISS for region %d, weather %d, time %d", 
        key->region_vnum, key->weather_condition, key->time_category);
    return NULL;
}

/* Store hints in cache */
void cache_hints(struct hint_cache_key *key, struct region_hint *hints) {
    if (!hints || hint_cache_entries >= HINT_CACHE_MAX_ENTRIES) {
        /* Clean up if we're at capacity */
        if (hint_cache_entries >= HINT_CACHE_MAX_ENTRIES) {
            cleanup_hint_cache();
        }
        if (hint_cache_entries >= HINT_CACHE_MAX_ENTRIES) {
            return; /* Still at capacity, skip caching */
        }
    }
    
    unsigned int hash = hash_cache_key(key);
    
    /* Create new cache entry */
    struct hint_cache_entry *entry = malloc(sizeof(struct hint_cache_entry));
    if (!entry) return;
    
    entry->key = *key;
    entry->hints = duplicate_hints(hints);
    entry->cache_time = time(NULL);
    entry->next = hint_cache[hash];
    
    /* Count hints */
    int count = 0;
    if (hints) {
        while (hints[count].hint_text) count++;
    }
    entry->hint_count = count;
    
    /* Insert at head of chain */
    hint_cache[hash] = entry;
    hint_cache_entries++;
    
    narrative_debug_log(2, "Cached %d hints for region %d (total cache entries: %d)", 
        count, key->region_vnum, hint_cache_entries);
}

/* Initialize hint caching system */
void init_hint_cache(void) {
    int i;
    for (i = 0; i < HINT_CACHE_SIZE; i++) {
        hint_cache[i] = NULL;
    }
    hint_cache_entries = 0;
    log("INIT: Hint caching system initialized");
}

/* Clear entire hint cache (for debugging or system reset) */
void clear_hint_cache(void) {
    int i;
    for (i = 0; i < HINT_CACHE_SIZE; i++) {
        struct hint_cache_entry *entry = hint_cache[i];
        while (entry) {
            struct hint_cache_entry *next = entry->next;
            if (entry->hints) {
                free_contextual_hints(entry->hints);
            }
            free(entry);
            entry = next;
        }
        hint_cache[i] = NULL;
    }
    hint_cache_entries = 0;
    narrative_debug_log(1, "Hint cache cleared");
}

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

/**
 * Check if a JSON array contains a specific string value
 * @param json_array JSON array string like ["mystical", "tranquil", "ethereal"]
 * @param search_string String to search for
 * @return 1 if found, 0 if not found
 */
int json_array_contains_string(const char *json_array, const char *search_string) {
    if (!json_array || !search_string) return 0;
    
    /* Simple string search approach for JSON arrays */
    /* Look for "search_string" (with quotes) in the JSON */
    char quoted_search[256];
    snprintf(quoted_search, sizeof(quoted_search), "\"%s\"", search_string);
    
    return strstr(json_array, quoted_search) != NULL ? 1 : 0;
}

/**
 * Get mood-based weight multiplier for hints based on regional AI characteristics
 * @param hint_category The category of hint being evaluated
 * @param hint_text The actual hint text
 * @param key_characteristics JSON string with regional AI characteristics
 * @return Weight multiplier (0.3 to 1.8)
 */
double get_mood_weight_for_hint(int hint_category, const char *hint_text, const char *key_characteristics) {
    double weight = 1.0;
    
    if (!key_characteristics || !hint_text) return weight;
    
    /* Parse key_characteristics JSON for mood-based weighting */
    /* Expected format: {"atmosphere": ["mystical", "tranquil"], "mystical_elements": ["ethereal", "ancient"], ...} */
    
    switch (hint_category) {
        case HINT_MYSTICAL:
            /* Boost mystical hints in mystical regions */
            if (json_array_contains_string(key_characteristics, "mystical") ||
                json_array_contains_string(key_characteristics, "ethereal") ||
                json_array_contains_string(key_characteristics, "magical")) {
                weight *= 1.8; /* 80% boost */
            }
            break;
            
        case HINT_SOUNDS:
            /* Boost quiet sounds in tranquil regions, reduce loud sounds */
            if (json_array_contains_string(key_characteristics, "tranquil") ||
                json_array_contains_string(key_characteristics, "peaceful") ||
                json_array_contains_string(key_characteristics, "serene")) {
                if (strstr(hint_text, "whisper") || strstr(hint_text, "soft") || 
                    strstr(hint_text, "gentle") || strstr(hint_text, "quiet")) {
                    weight *= 1.6; /* 60% boost for quiet sounds */
                } else if (strstr(hint_text, "loud") || strstr(hint_text, "roar") ||
                          strstr(hint_text, "crash") || strstr(hint_text, "thunder")) {
                    weight *= 0.3; /* 70% reduction for loud sounds */
                }
            }
            break;
            
        case HINT_FLORA:
            /* Boost moss-related flora in moss-covered regions */
            if (json_array_contains_string(key_characteristics, "moss") ||
                json_array_contains_string(key_characteristics, "verdant")) {
                if (strstr(hint_text, "moss") || strstr(hint_text, "lichen") ||
                    strstr(hint_text, "fern") || strstr(hint_text, "green")) {
                    weight *= 1.5; /* 50% boost for moss-related flora */
                }
            }
            break;
            
        case HINT_ATMOSPHERE:
            /* Boost ethereal atmosphere in ethereal regions */
            if (json_array_contains_string(key_characteristics, "ethereal") ||
                json_array_contains_string(key_characteristics, "otherworldly")) {
                if (strstr(hint_text, "ethereal") || strstr(hint_text, "otherworldly") ||
                    strstr(hint_text, "mystical") || strstr(hint_text, "magical")) {
                    weight *= 1.5; /* 50% boost for ethereal atmospheric hints */
                }
            }
            
            /* Additional boost for tranquil regions */
            if (json_array_contains_string(key_characteristics, "tranquil") ||
                json_array_contains_string(key_characteristics, "peaceful")) {
                if (strstr(hint_text, "peaceful") || strstr(hint_text, "calm") ||
                    strstr(hint_text, "serene") || strstr(hint_text, "tranquil")) {
                    weight *= 1.2; /* 20% additional boost for peaceful descriptors */
                }
            }
            break;
            
        default:
            /* No special weighting for other categories */
            break;
    }
    
    /* Ensure weight stays within reasonable bounds */
    if (weight < 0.3) weight = 0.3;
    if (weight > 1.8) weight = 1.8;
    
    return weight;
}

/**
 * Helper function to get time weight for specific category from JSON
 */
double get_time_weight_for_category(const char *json_weights, const char *time_category) {
    if (!json_weights || !time_category) return 1.0;
    
    /* Simple JSON parsing for time categories */
    char search_pattern[64];
    snprintf(search_pattern, sizeof(search_pattern), "\"%s\":", time_category);
    
    char *pos = strstr(json_weights, search_pattern);
    if (pos) {
        pos += strlen(search_pattern);
        /* Skip whitespace */
        while (*pos == ' ' || *pos == '\t') pos++;
        
        /* Parse the numeric value */
        double weight = strtod(pos, NULL);
        if (weight > 0.0 && weight <= 3.0) { /* Reasonable range check */
            return weight;
        }
    }
    
    return 1.0; /* Default weight if not found */
}

/**
 * Select a hint using weighted probability based on contextual and mood weights
 * @param hints Array of available hints
 * @param hint_indices Array of indices to select from
 * @param count Number of hints to choose from
 * @param key_characteristics Regional AI characteristics for mood weighting
 * @return Index of selected hint from hint_indices array
 */
int select_weighted_hint(struct region_hint *hints, int *hint_indices, int count, const char *key_characteristics) {
    if (!hints || !hint_indices || count <= 0) return 0;
    
    /* Simple case - no weighting needed */
    if (count == 1) return 0;
    
    /* Calculate total weight for all hints */
    double total_weight = 0.0;
    double weights[20]; /* Max 20 hints as per usage */
    int i;
    
    for (i = 0; i < count && i < 20; i++) {
        int hint_index = hint_indices[i];
        double contextual_weight = hints[hint_index].contextual_weight;
        double mood_weight = get_mood_weight_for_hint(hints[hint_index].hint_category, 
                                                     hints[hint_index].hint_text, 
                                                     key_characteristics);
        
        weights[i] = contextual_weight * mood_weight;
        total_weight += weights[i];
    }
    
    /* Random selection based on weighted probability */
    if (total_weight <= 0.0) {
        /* Fallback to simple random if no valid weights */
        return rand() % count;
    }
    
    double random_value = ((double)rand() / RAND_MAX) * total_weight;
    double cumulative_weight = 0.0;
    
    for (i = 0; i < count && i < 20; i++) {
        cumulative_weight += weights[i];
        if (random_value <= cumulative_weight) {
            return i;
        }
    }
    
    /* Fallback to last hint if rounding errors occur */
    return count - 1;
}

/* ====================================================================== */
/*                    MULTI-CONDITION CONTEXTUAL SYSTEM                 */
/* ====================================================================== */

/**
 * Enhanced weather relevance calculation using wilderness weather intensity
 * Takes raw weather value (0-255) and analyzes hint content for weather keywords
 */
double calculate_weather_relevance_for_hint(struct region_hint *hint, int weather_value) {
    if (!hint || !hint->hint_text) return 1.0;
    
    const char *hint_text = hint->hint_text;
    double relevance = 1.0;
    double weather_intensity = weather_value / 255.0; /* Convert to 0.0-1.0 scale */
    
    /* Clear weather (0-63): Bright, sunny, clear visibility */
    if (weather_value < 64) {
        if (strstr(hint_text, "bright") || strstr(hint_text, "sunlight") || 
            strstr(hint_text, "clear") || strstr(hint_text, "visible") ||
            strstr(hint_text, "radiant") || strstr(hint_text, "gleaming") ||
            strstr(hint_text, "dazzl") || strstr(hint_text, "sparkl")) {
            relevance = 1.0 + (0.5 * (1.0 - weather_intensity)); /* Boost in very clear weather */
        }
        if (strstr(hint_text, "shadow") || strstr(hint_text, "mist") || 
            strstr(hint_text, "fog") || strstr(hint_text, "gloom") ||
            strstr(hint_text, "murk") || strstr(hint_text, "obscur")) {
            relevance = 0.3 + (0.4 * weather_intensity); /* Reduce shadowy effects in clear weather */
        }
    }
    /* Cloudy weather (64-127): Overcast, filtered light, atmospheric */
    else if (weather_value < 128) {
        if (strstr(hint_text, "overcast") || strstr(hint_text, "gray") || strstr(hint_text, "grey") ||
            strstr(hint_text, "subdued") || strstr(hint_text, "muted") || strstr(hint_text, "diffus") ||
            strstr(hint_text, "soft") || strstr(hint_text, "gentle")) {
            relevance = 1.0 + (0.3 * weather_intensity); /* Boost atmospheric hints */
        }
        if (strstr(hint_text, "shadow") || strstr(hint_text, "dim")) {
            relevance = 1.0 + (0.2 * weather_intensity); /* Slight boost for shadows */
        }
        if (strstr(hint_text, "dazzl") || strstr(hint_text, "gleaming") || strstr(hint_text, "radiant")) {
            relevance = 0.6 + (0.2 * (1.0 - weather_intensity)); /* Reduce bright effects */
        }
    }
    /* Rainy weather (128-191): Wet, fresh, water-related */
    else if (weather_value < 192) {
        if (strstr(hint_text, "rain") || strstr(hint_text, "wet") || strstr(hint_text, "damp") ||
            strstr(hint_text, "fresh") || strstr(hint_text, "droplet") || strstr(hint_text, "drip") ||
            strstr(hint_text, "moisture") || strstr(hint_text, "glistening") || strstr(hint_text, "slick")) {
            relevance = 1.0 + (0.6 * weather_intensity); /* Strong boost for water-related hints */
        }
        if (strstr(hint_text, "dust") || strstr(hint_text, "dry") || strstr(hint_text, "parched") ||
            strstr(hint_text, "arid") || strstr(hint_text, "crisp")) {
            relevance = 0.2 + (0.3 * (1.0 - weather_intensity)); /* Reduce dry effects in rain */
        }
        if (strstr(hint_text, "scent") || strstr(hint_text, "fragrance") || strstr(hint_text, "aroma")) {
            relevance = 1.0 + (0.3 * weather_intensity); /* Rain enhances scents */
        }
    }
    /* Stormy weather (192-255): Dramatic, intense, powerful */
    else {
        if (strstr(hint_text, "wind") || strstr(hint_text, "storm") || strstr(hint_text, "tempest") ||
            strstr(hint_text, "dramatic") || strstr(hint_text, "powerful") || strstr(hint_text, "fierce") ||
            strstr(hint_text, "crash") || strstr(hint_text, "roar") || strstr(hint_text, "thunder") ||
            strstr(hint_text, "lightning") || strstr(hint_text, "torrent") || strstr(hint_text, "lash")) {
            relevance = 1.0 + (0.8 * weather_intensity); /* Strong boost for dramatic elements */
        }
        if (strstr(hint_text, "gentle") || strstr(hint_text, "calm") || strstr(hint_text, "peaceful") ||
            strstr(hint_text, "serene") || strstr(hint_text, "tranquil") || strstr(hint_text, "quiet")) {
            relevance = 0.1 + (0.3 * (1.0 - weather_intensity)); /* Greatly reduce peaceful effects */
        }
        if (strstr(hint_text, "sway") || strstr(hint_text, "bend") || strstr(hint_text, "dance") ||
            strstr(hint_text, "flutter") || strstr(hint_text, "wave")) {
            relevance = 1.0 + (0.4 * weather_intensity); /* Boost movement effects in storms */
        }
    }
    
    /* Special category bonuses */
    if (hint->hint_category == HINT_WEATHER_INFLUENCE) {
        relevance *= 1.5; /* Always boost weather influence hints */
    }
    if (hint->hint_category == HINT_SOUNDS && weather_value >= 128) {
        relevance *= (1.0 + 0.3 * weather_intensity); /* Boost sounds in wet/stormy weather */
    }
    
    /* Ensure reasonable bounds */
    if (relevance < 0.1) relevance = 0.1;
    if (relevance > 2.5) relevance = 2.5;
    
    return relevance;
}

/**
 * Calculate distance-based regional influence for smooth transitions
 * @param x, y Current coordinates  
 * @param region_x, region_y Region center coordinates
 * @param max_influence_distance Maximum distance for full regional influence
 * @return Influence factor from 0.0 (no influence) to 1.0 (full influence)
 */
double calculate_regional_influence(int x, int y, int region_x, int region_y, int max_influence_distance) {
    if (max_influence_distance <= 0) return 1.0;
    
    /* Calculate Manhattan distance (simpler for wilderness grid) */
    int distance = abs(x - region_x) + abs(y - region_y);
    
    if (distance >= max_influence_distance) {
        return 0.0; /* No influence beyond max distance */
    }
    
    /* Linear falloff: 1.0 at center, 0.0 at max distance */
    double influence = 1.0 - ((double)distance / (double)max_influence_distance);
    
    /* Smooth curve using cosine interpolation for more natural transitions */
    influence = (1.0 + cos((1.0 - influence) * M_PI)) * 0.5;
    
    return influence;
}

/**
 * Detect if current location is near a region boundary for enhanced transition effects
 * @param x, y Current coordinates
 * @param region_vnum Current region
 * @return Boundary proximity factor: 0.0 (center) to 1.0 (edge)
 */
double detect_region_boundary_proximity(int x, int y, int region_vnum) {
    /* This is a simplified implementation - a full version would check actual region boundaries */
    
    /* Simulate region boundaries using coordinate patterns */
    /* In a real implementation, this would query the region boundary data */
    
    /* Create simulated region boundaries based on coordinate modulo */
    int region_size = 20; /* Assume 20x20 coordinate regions */
    int region_x = x % region_size;
    int region_y = y % region_size;
    
    /* Calculate distance to nearest edge */
    int edge_distance_x = MIN(region_x, region_size - region_x);
    int edge_distance_y = MIN(region_y, region_size - region_y);
    int min_edge_distance = MIN(edge_distance_x, edge_distance_y);
    
    /* Convert to boundary proximity (0.0 = center, 1.0 = edge) */
    double boundary_proximity = 1.0 - ((double)min_edge_distance / (region_size / 2));
    
    /* Clamp to 0.0-1.0 range */
    if (boundary_proximity < 0.0) boundary_proximity = 0.0;
    if (boundary_proximity > 1.0) boundary_proximity = 1.0;
    
    return boundary_proximity;
}

/**
 * Enhanced hint selection with regional transition awareness
 * Modifies contextual weights based on proximity to region boundaries
 */
void apply_boundary_transition_effects(struct region_hint *hints, int hint_count, 
                                     int x, int y, int region_vnum) {
    if (!hints || hint_count <= 0) return;
    
    double boundary_proximity = detect_region_boundary_proximity(x, y, region_vnum);
    
    /* Near boundaries, reduce region-specific hints and increase generic ones */
    if (boundary_proximity > 0.3) { /* Within 30% of boundary */
        int i;
        for (i = 0; i < hint_count; i++) {
            /* Reduce highly region-specific hints near boundaries */
            if (hints[i].hint_category == HINT_MYSTICAL || 
                hints[i].hint_category == HINT_LANDMARKS ||
                hints[i].hint_category == HINT_ATMOSPHERE) {
                
                /* Gradual reduction based on boundary proximity */
                double reduction_factor = 1.0 - (boundary_proximity * 0.4); /* Max 40% reduction */
                hints[i].contextual_weight *= reduction_factor;
            }
            
            /* Boost universal hints that work in transition zones */
            if (hints[i].hint_category == HINT_FLORA || 
                hints[i].hint_category == HINT_FAUNA ||
                hints[i].hint_category == HINT_WEATHER_INFLUENCE) {
                
                /* Slight boost for universal elements */
                double boost_factor = 1.0 + (boundary_proximity * 0.2); /* Max 20% boost */
                hints[i].contextual_weight *= boost_factor;
            }
        }
        
        narrative_debug_log(2, "Applied boundary transition effects (proximity=%.2f) to %d hints", 
            boundary_proximity, hint_count);
    }
}

/**
 * Calculate overall resource abundance for an area to influence regional character
 * @param x, y Center coordinates 
 * @param radius Area radius to sample
 * @return Overall resource health: 0.0 (depleted) to 1.0 (abundant)
 */
float calculate_regional_resource_health(int x, int y, int radius) {
    float total_resources = 0.0;
    int sample_count = 0;
    int dx, dy;
    
    /* Sample resources in a grid pattern around the center point */
    for (dx = -radius; dx <= radius; dx += 2) { /* Sample every 2 coordinates for performance */
        for (dy = -radius; dy <= radius; dy += 2) {
            int sample_x = x + dx;
            int sample_y = y + dy;
            
            /* Calculate Manhattan distance for area weighting */
            int distance = abs(dx) + abs(dy);
            if (distance > radius) continue; /* Skip points outside radius */
            
            /* Weight nearby samples more heavily */
            float weight = 1.0 - ((float)distance / radius);
            
            /* Sample key resource types that affect regional atmosphere */
            float vegetation = calculate_current_resource_level(RESOURCE_VEGETATION, sample_x, sample_y);
            float water = calculate_current_resource_level(RESOURCE_WATER, sample_x, sample_y);
            float game = calculate_current_resource_level(RESOURCE_GAME, sample_x, sample_y);
            float herbs = calculate_current_resource_level(RESOURCE_HERBS, sample_x, sample_y);
            
            /* Combine resources with atmospheric relevance weighting */
            float sample_health = (vegetation * 0.4) + (water * 0.3) + (game * 0.2) + (herbs * 0.1);
            
            total_resources += sample_health * weight;
            sample_count++;
        }
    }
    
    if (sample_count == 0) return 0.5; /* Default to moderate if no samples */
    
    float average_health = total_resources / sample_count;
    
    /* Clamp to valid range */
    if (average_health < 0.0) average_health = 0.0;
    if (average_health > 1.0) average_health = 1.0;
    
    return average_health;
}

/**
 * Apply resource-based weighting to hints based on regional resource health
 * @param hints Array of hints to modify
 * @param hint_count Number of hints
 * @param resource_health Overall resource health (0.0-1.0)
 */
void apply_resource_based_hint_weighting(struct region_hint *hints, int hint_count, float resource_health) {
    if (!hints || hint_count <= 0) return;
    
    int i;
    
    /* Abundant resources (0.7-1.0) - lush, thriving descriptions */
    if (resource_health >= 0.7) {
        for (i = 0; i < hint_count; i++) {
            const char *hint_text = hints[i].hint_text;
            
            /* Boost hints suggesting abundance and vitality */
            if (strstr(hint_text, "lush") || strstr(hint_text, "abundant") || 
                strstr(hint_text, "thriving") || strstr(hint_text, "rich") ||
                strstr(hint_text, "flourish") || strstr(hint_text, "vibrant") ||
                strstr(hint_text, "dense") || strstr(hint_text, "plentiful")) {
                hints[i].contextual_weight *= (1.0 + 0.4 * resource_health); /* Up to 40% boost */
            }
            
            /* Reduce hints suggesting scarcity */
            if (strstr(hint_text, "sparse") || strstr(hint_text, "depleted") || 
                strstr(hint_text, "barren") || strstr(hint_text, "struggling") ||
                strstr(hint_text, "withered") || strstr(hint_text, "scarce")) {
                hints[i].contextual_weight *= 0.3; /* Strong reduction */
            }
            
            /* Boost nature-related categories in abundant areas */
            if (hints[i].hint_category == HINT_FLORA || hints[i].hint_category == HINT_FAUNA) {
                hints[i].contextual_weight *= (1.0 + 0.2 * resource_health); /* Up to 20% boost */
            }
        }
    }
    /* Depleted resources (0.0-0.3) - sparse, struggling descriptions */
    else if (resource_health <= 0.3) {
        for (i = 0; i < hint_count; i++) {
            const char *hint_text = hints[i].hint_text;
            
            /* Boost hints suggesting scarcity and struggle */
            if (strstr(hint_text, "sparse") || strstr(hint_text, "depleted") || 
                strstr(hint_text, "barren") || strstr(hint_text, "struggling") ||
                strstr(hint_text, "withered") || strstr(hint_text, "scarce") ||
                strstr(hint_text, "dry") || strstr(hint_text, "brittle")) {
                hints[i].contextual_weight *= (1.0 + 0.5 * (1.0 - resource_health)); /* Up to 50% boost when very depleted */
            }
            
            /* Reduce hints suggesting abundance */
            if (strstr(hint_text, "lush") || strstr(hint_text, "abundant") || 
                strstr(hint_text, "thriving") || strstr(hint_text, "rich") ||
                strstr(hint_text, "flourish") || strstr(hint_text, "vibrant")) {
                hints[i].contextual_weight *= 0.2; /* Strong reduction */
            }
            
            /* Reduce fauna hints in depleted areas (animals leave) */
            if (hints[i].hint_category == HINT_FAUNA) {
                hints[i].contextual_weight *= (0.5 + 0.3 * resource_health); /* Significant reduction */
            }
            
            /* Slightly reduce flora hints but not as much (plants struggle but persist) */
            if (hints[i].hint_category == HINT_FLORA) {
                hints[i].contextual_weight *= (0.7 + 0.2 * resource_health); /* Moderate reduction */
            }
        }
    }
    /* Moderate resources (0.3-0.7) - gradual transitions */
    else {
        /* Apply subtle adjustments for moderate resource levels */
        double abundance_factor = (resource_health - 0.3) / 0.4; /* Scale 0.3-0.7 to 0.0-1.0 */
        
        for (i = 0; i < hint_count; i++) {
            const char *hint_text = hints[i].hint_text;
            
            /* Gentle boost/reduction based on abundance keywords */
            if (strstr(hint_text, "lush") || strstr(hint_text, "abundant") || strstr(hint_text, "thriving")) {
                hints[i].contextual_weight *= (1.0 + 0.1 * abundance_factor); /* Up to 10% boost */
            }
            if (strstr(hint_text, "sparse") || strstr(hint_text, "struggling") || strstr(hint_text, "scarce")) {
                hints[i].contextual_weight *= (1.0 - 0.1 * abundance_factor); /* Up to 10% reduction */
            }
        }
    }
    
    /* Ensure minimum weights */
    for (i = 0; i < hint_count; i++) {
        if (hints[i].contextual_weight < 0.1) {
            hints[i].contextual_weight = 0.1;
        }
    }
}

/**
 * Structure to hold regional transition data for blending
 */
struct regional_transition {
    int region_vnum;                    /* Region identifier */
    double influence_factor;            /* 0.0-1.0 influence strength */
    char *characteristics;              /* Regional characteristics JSON */
    struct region_hint *hints;          /* Regional hints */
    int hint_count;                     /* Number of hints */
};

/**
 * Find nearby regions and calculate their influence for smooth transitions
 * @param x, y Current coordinates
 * @param max_search_radius Maximum distance to search for regions
 * @param transition_count Output: number of regions found
 * @return Array of regional_transition structures (caller must free)
 */
struct regional_transition *calculate_regional_transitions(int x, int y, int max_search_radius, int *transition_count) {
    struct regional_transition *transitions = NULL;
    int capacity = 5; /* Initial capacity for nearby regions */
    int count = 0;
    
    *transition_count = 0;
    
    transitions = malloc(sizeof(struct regional_transition) * capacity);
    if (!transitions) return NULL;
    
    /* Primary region (current location's main region) */
    int primary_region_vnum = 1000004; /* TODO: Determine from actual coordinates */
    
    transitions[count].region_vnum = primary_region_vnum;
    transitions[count].influence_factor = 1.0; /* Full influence at current location */
    transitions[count].characteristics = load_region_characteristics(transitions[count].region_vnum);
    transitions[count].hints = NULL; /* Will be loaded separately if needed */
    transitions[count].hint_count = 0;
    count++;
    
    /* Simulate adjacent regions for demonstration - in a full implementation, 
     * this would query the actual region system */
    int adjacent_regions[] = {1000005, 1000006}; /* Example adjacent region vnums */
    int num_adjacent = sizeof(adjacent_regions) / sizeof(adjacent_regions[0]);
    int i;
    
    for (i = 0; i < num_adjacent && count < capacity; i++) {
        /* Simulate region centers - in real implementation, get from region data */
        int region_center_x = x + ((i % 2) * 20) - 10; /* Offset by +/-10 coordinates */
        int region_center_y = y + ((i / 2) * 20) - 10;
        
        /* Calculate influence based on distance */
        double influence = calculate_regional_influence(x, y, region_center_x, region_center_y, max_search_radius);
        
        /* Only include regions with significant influence */
        if (influence > 0.1) {
            transitions[count].region_vnum = adjacent_regions[i];
            transitions[count].influence_factor = influence;
            transitions[count].characteristics = load_region_characteristics(transitions[count].region_vnum);
            transitions[count].hints = NULL;
            transitions[count].hint_count = 0;
            count++;
            
            narrative_debug_log(2, "Added adjacent region %d with influence %.2f", 
                adjacent_regions[i], influence);
        }
    }
    
    *transition_count = count;
    return transitions;
}

/**
 * Apply regional transition blending to hint weights for smooth boundaries
 * @param hints Array of hints to modify
 * @param hint_count Number of hints
 * @param transitions Array of regional transitions
 * @param transition_count Number of transitions
 */
void apply_regional_transition_weights(struct region_hint *hints, int hint_count, 
                                     struct regional_transition *transitions, int transition_count) {
    if (!hints || !transitions || hint_count <= 0 || transition_count <= 0) return;
    
    int i, j;
    
    for (i = 0; i < hint_count; i++) {
        double total_transition_weight = 0.0;
        
        /* Calculate weighted influence from all nearby regions */
        for (j = 0; j < transition_count; j++) {
            if (!transitions[j].characteristics) continue;
            
            /* Get mood weight for this hint from this region */
            double mood_weight = get_mood_weight_for_hint(hints[i].hint_category, 
                                                        hints[i].hint_text, 
                                                        transitions[j].characteristics);
            
            /* Weight by regional influence factor */
            double weighted_contribution = mood_weight * transitions[j].influence_factor;
            total_transition_weight += weighted_contribution;
        }
        
        /* Apply blended weight - normalize by total influence */
        double total_influence = 0.0;
        for (j = 0; j < transition_count; j++) {
            total_influence += transitions[j].influence_factor;
        }
        
        if (total_influence > 0.0) {
            /* Apply transition-blended weight to hint */
            double transition_multiplier = total_transition_weight / total_influence;
            hints[i].contextual_weight *= transition_multiplier;
            
            /* Ensure minimum weight */
            if (hints[i].contextual_weight < 0.1) {
                hints[i].contextual_weight = 0.1;
            }
        }
    }
}

/**
 * Calculate comprehensive relevance score combining multiple environmental factors
 */
double calculate_comprehensive_relevance(struct region_hint *hint, struct environmental_context *context, const char *regional_characteristics) {
    double base_weight = 1.0;
    double seasonal_multiplier = 1.0;
    double time_multiplier = 1.0;
    double weather_multiplier = 1.0;
    double mood_multiplier = 1.0;
    
    /* Base contextual weight from hint */
    base_weight = hint->contextual_weight;
    
    /* Seasonal relevance using existing function */
    if (hint->seasonal_weight && strlen(hint->seasonal_weight) > 0) {
        seasonal_multiplier = get_seasonal_weight_for_hint(hint->seasonal_weight, context->season);
    }
    
    /* Time-of-day relevance using existing function */
    if (hint->time_of_day_weight && strlen(hint->time_of_day_weight) > 0) {
        const char *time_category_str;
        switch (context->time_of_day) {
            case SUN_DARK: time_category_str = "night"; break;
            case SUN_RISE: time_category_str = "dawn"; break;
            case SUN_LIGHT: time_category_str = "day"; break;
            case SUN_SET: time_category_str = "dusk"; break;
            default: time_category_str = "day"; break;
        }
        time_multiplier = get_time_weight_for_hint(hint->time_of_day_weight, time_category_str);
    }
    
    /* Enhanced weather relevance using sophisticated weather analysis */
    weather_multiplier = calculate_weather_relevance_for_hint(hint, context->weather);
    
    /* Regional mood-based weighting */
    mood_multiplier = get_mood_weight_for_hint(hint->hint_category, hint->hint_text, regional_characteristics);
    
    /* Combine all factors with diminishing returns to prevent extreme scores */
    double combined_score = base_weight * 
                           (0.8 + 0.2 * seasonal_multiplier) *
                           (0.8 + 0.2 * time_multiplier) *
                           (0.8 + 0.2 * weather_multiplier) *
                           mood_multiplier;
    
    /* Ensure minimum threshold for basic relevance */
    if (combined_score < 0.1) combined_score = 0.1;
    
    return combined_score;
}

/**
 * Enhanced weighted hint selection with comprehensive multi-condition scoring
 */
int select_contextual_weighted_hint(struct region_hint *hints, int *hint_indices, int count, 
                                   struct environmental_context *context, const char *regional_characteristics) {
    if (!hints || !hint_indices || !context || count <= 0) return 0;
    
    /* Simple case - no weighting needed */
    if (count == 1) return 0;
    
    /* Calculate comprehensive relevance for all hints */
    double total_weight = 0.0;
    double weights[20]; /* Max 20 hints as per usage */
    int i;
    
    for (i = 0; i < count && i < 20; i++) {
        int hint_index = hint_indices[i];
        weights[i] = calculate_comprehensive_relevance(&hints[hint_index], context, regional_characteristics);
        total_weight += weights[i];
        
        narrative_debug_log(2, "Hint %d relevance score: %.3f (base=%.2f, category=%d)", 
            hint_index, weights[i], hints[hint_index].contextual_weight, hints[hint_index].hint_category);
    }
    
    /* Random selection based on weighted probability */
    if (total_weight <= 0.0) {
        /* Fallback to simple random if no valid weights */
        return rand() % count;
    }
    
    double random_value = ((double)rand() / RAND_MAX) * total_weight;
    double cumulative_weight = 0.0;
    
    for (i = 0; i < count && i < 20; i++) {
        cumulative_weight += weights[i];
        if (random_value <= cumulative_weight) {
            narrative_debug_log(2, "Selected hint %d with weight %.3f (cumulative %.3f, random %.3f)", 
                hint_indices[i], weights[i], cumulative_weight, random_value);
            return i;
        }
    }
    
    /* Fallback to last hint if rounding errors occur */
    return count - 1;
}

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
    "ethereal", "otherworldly", "sacred", "timeless", "ancient",
    "luminescent", "glowing", "bioluminescent", "emerald",
    NULL
};

/* Action verb patterns for dynamic element detection */
static const char *action_verbs[] = {
    "whisper", "murmur", "rustle", "sway", "dance",
    "tower", "loom", "stretch", "reach", "rise",
    "flow", "cascade", "trickle", "babble", "gurgle",
    "call", "cry", "sing", "chirp", "echo",
    "glow", "emanate", "pulse", "shimmer", "sparkle",
    "envelop", "drape", "carpet", "muffle", "filter",
    NULL
};

/**
 * Advanced vocabulary substitution system
 */
struct vocabulary_mapping {
    const char *generic_word;
    const char *replacement;
};

/* Style-specific vocabulary mappings for comprehensive transformation */
static const struct vocabulary_mapping poetic_mappings[] = {
    {"trees", "graceful sentinels"},
    {"forest", "sylvan realm"},
    {"light", "luminous embrace"},
    {"wind", "gentle zephyr"},
    {"water", "crystalline streams"},
    {"path", "winding melody"},
    {"rock", "ancient stone"},
    {"grass", "emerald carpet"},
    {"sky", "azure canvas"},
    {"mist", "gossamer veil"},
    {NULL, NULL}
};

static const struct vocabulary_mapping mysterious_mappings[] = {
    {"trees", "shadowed sentries"},
    {"forest", "enigmatic depths"},
    {"light", "elusive glimmer"},
    {"wind", "whispered secrets"},
    {"water", "veiled currents"},
    {"path", "hidden passage"},
    {"rock", "cryptic monoliths"},
    {"grass", "concealing undergrowth"},
    {"sky", "shrouded expanse"},
    {"mist", "obscuring shroud"},
    {NULL, NULL}
};

static const struct vocabulary_mapping dramatic_mappings[] = {
    {"trees", "towering giants"},
    {"forest", "commanding wilderness"},
    {"light", "blazing radiance"},
    {"wind", "mighty gale"},
    {"water", "thundering torrents"},
    {"path", "grand thoroughfare"},
    {"rock", "imposing bastions"},
    {"grass", "sweeping meadows"},
    {"sky", "vast dome"},
    {"mist", "rolling banks"},
    {NULL, NULL}
};

static const struct vocabulary_mapping pastoral_mappings[] = {
    {"trees", "gentle guardians"},
    {"forest", "peaceful grove"},
    {"light", "warm glow"},
    {"wind", "soft breeze"},
    {"water", "babbling brook"},
    {"path", "welcoming trail"},
    {"rock", "weathered stones"},
    {"grass", "soft meadow"},
    {"sky", "friendly heavens"},
    {"mist", "morning dew"},
    {NULL, NULL}
};

static const struct vocabulary_mapping practical_mappings[] = {
    {"trees", "timber stands"},
    {"forest", "wooded area"},
    {"light", "illumination"},
    {"wind", "air current"},
    {"water", "water source"},
    {"path", "route"},
    {"rock", "stone formation"},
    {"grass", "ground cover"},
    {"sky", "overhead"},
    {"mist", "moisture"},
    {NULL, NULL}
};

/**
 * Get vocabulary mapping array for a given style
 */
const struct vocabulary_mapping *get_style_vocabulary(int style) {
    switch (style) {
        case STYLE_POETIC:      return poetic_mappings;
        case STYLE_MYSTERIOUS:  return mysterious_mappings;
        case STYLE_DRAMATIC:    return dramatic_mappings;
        case STYLE_PASTORAL:    return pastoral_mappings;
        case STYLE_PRACTICAL:   return practical_mappings;
        default:                return poetic_mappings;
    }
}

/**
 * Apply comprehensive vocabulary transformation to text
 */
char *apply_vocabulary_transformation(const char *text, int style) {
    if (!text) return NULL;
    
    const struct vocabulary_mapping *mappings = get_style_vocabulary(style);
    char *result = strdup(text);
    if (!result) return NULL;
    
    int i;
    for (i = 0; mappings[i].generic_word != NULL; i++) {
        char *pos = strstr(result, mappings[i].generic_word);
        if (pos) {
            // Calculate new size needed
            size_t old_len = strlen(mappings[i].generic_word);
            size_t new_len = strlen(mappings[i].replacement);
            size_t result_len = strlen(result);
            
            if (new_len > old_len) {
                // Need more space
                char *expanded = malloc(result_len + (new_len - old_len) + 1);
                if (!expanded) {
                    free(result);
                    return NULL;
                }
                
                // Copy parts before the match
                size_t prefix_len = pos - result;
                strncpy(expanded, result, prefix_len);
                expanded[prefix_len] = '\0';
                
                // Add replacement
                strcat(expanded, mappings[i].replacement);
                
                // Add remainder
                strcat(expanded, pos + old_len);
                
                free(result);
                result = expanded;
            } else {
                // Replace in place
                memmove(pos + new_len, pos + old_len, strlen(pos + old_len) + 1);
                memcpy(pos, mappings[i].replacement, new_len);
            }
        }
    }
    
    return result;
}

/* Style-specific vocabulary dictionaries for enhanced adjectives and verbs */
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
const struct vocabulary_mapping *get_style_vocabulary(int style);
char *apply_vocabulary_transformation(const char *text, int style);
char *apply_regional_style_transformation(const char *text, int style);
int convert_style_string_to_int(const char *style_str);
const char *get_transitional_phrase(int style, const char *context);

/* Multi-condition contextual system prototypes */
double get_time_weight_for_category(const char *json_weights, const char *time_category);
double calculate_comprehensive_relevance(struct region_hint *hint, struct environmental_context *context, const char *regional_characteristics);
double calculate_weather_relevance_for_hint(struct region_hint *hint, int weather_value);
int select_contextual_weighted_hint(struct region_hint *hints, int *hint_indices, int count, 
                                   struct environmental_context *context, const char *regional_characteristics);

/* Regional transition system prototypes */
double calculate_regional_influence(int x, int y, int region_x, int region_y, int max_influence_distance);
double detect_region_boundary_proximity(int x, int y, int region_vnum);
struct regional_transition *calculate_regional_transitions(int x, int y, int max_search_radius, int *transition_count);
void apply_regional_transition_weights(struct region_hint *hints, int hint_count, 
                                     struct regional_transition *transitions, int transition_count);
void apply_boundary_transition_effects(struct region_hint *hints, int hint_count, 
                                     int x, int y, int region_vnum);
float calculate_regional_resource_health(int x, int y, int radius);
void apply_resource_based_hint_weighting(struct region_hint *hints, int hint_count, float resource_health);

/* ====================================================================== */
/*                       TRANSITIONAL PHRASE SYSTEM                      */
/* ====================================================================== */

/* Enhanced transitional phrases for natural flow between sentences */
static const char *atmospheric_transitions[] = {
    "Meanwhile, ", "Nearby, ", "Around you, ", "In the distance, ",
    "Overhead, ", "Throughout the area, ", "Further on, ", "Beyond this, ",
    "All around, ", "Here and there, ", "Amidst this, ", "", NULL
};

static const char *sensory_transitions[] = {
    "Here, ", "Softly, ", "All around, ", "Quietly, ", 
    "In this place, ", "Gently, ", "Faintly, ", "Subtly, ",
    "Through the air, ", "From every direction, ", "", NULL
};

static const char *mysterious_transitions[] = {
    "Mysteriously, ", "In shadows, ", "Subtly, ", "Eerily, ",
    "With ancient presence, ", "Enigmatically, ", "Through veiled paths, ",
    "In hidden corners, ", "Where secrets dwell, ", "Silently, ", "", NULL
};

static const char *dramatic_transitions[] = {
    "Majestically, ", "Boldly, ", "Powerfully, ", "Impressively, ",
    "With great presence, ", "Commandingly, ", "Triumphantly, ",
    "With overwhelming force, ", "In grand display, ", "Magnificently, ", "", NULL
};

static const char *pastoral_transitions[] = {
    "Peacefully, ", "Gently, ", "Serenely, ", "Tenderly, ",
    "With quiet grace, ", "In harmony, ", "Soothingly, ",
    "With natural beauty, ", "In tranquil abundance, ", "Calmly, ", "", NULL
};

static const char *poetic_transitions[] = {
    "Gracefully, ", "Like a melody, ", "In ethereal beauty, ", "With flowing elegance, ",
    "As if dancing, ", "In perfect harmony, ", "With artistic flair, ",
    "Like brushstrokes on canvas, ", "In sublime arrangement, ", "Rhythmically, ", "", NULL
};

/* Context-specific transitions for better flow */
static const char *temporal_transitions[] = {
    "As time passes, ", "Throughout the hours, ", "In moments of stillness, ",
    "With each passing moment, ", "In the eternal now, ", "Through endless cycles, ", "", NULL
};

static const char *spatial_transitions[] = {
    "To the north, ", "Southward, ", "Eastward, ", "Westward, ",
    "Above, ", "Below, ", "At ground level, ", "Higher up, ", 
    "In the depths, ", "At the surface, ", "", NULL
};

static const char *weather_transitions[] = {
    "Despite the weather, ", "Enhanced by conditions, ", "Under these skies, ",
    "In this climate, ", "Blessed by nature, ", "Protected from elements, ", "", NULL
};

/**
 * Get appropriate transitional phrase based on style and context with enhanced semantic analysis
 */
const char *get_transitional_phrase(int style, const char *context) {
    if (!context) return "";
    
    // Use empty transition 40% of the time for natural variation
    if (rand() % 5 < 2) {
        return "";
    }
    
    // Context-based transition selection for more natural flow
    
    // Temporal context
    if (strstr(context, "time") || strstr(context, "moment") || strstr(context, "hour") ||
        strstr(context, "dawn") || strstr(context, "dusk") || strstr(context, "night") ||
        strstr(context, "morning") || strstr(context, "evening")) {
        int count = 0;
        while (temporal_transitions[count]) count++;
        return temporal_transitions[rand() % count];
    }
    
    // Spatial/directional context
    if (strstr(context, "north") || strstr(context, "south") || strstr(context, "east") ||
        strstr(context, "west") || strstr(context, "above") || strstr(context, "below") ||
        strstr(context, "beyond") || strstr(context, "distance")) {
        int count = 0;
        while (spatial_transitions[count]) count++;
        return spatial_transitions[rand() % count];
    }
    
    // Weather context
    if (strstr(context, "rain") || strstr(context, "wind") || strstr(context, "storm") ||
        strstr(context, "sun") || strstr(context, "cloud") || strstr(context, "mist") ||
        strstr(context, "weather") || strstr(context, "climate")) {
        int count = 0;
        while (weather_transitions[count]) count++;
        return weather_transitions[rand() % count];
    }
    
    // Sensory context - expanded detection
    if (strstr(context, "scent") || strstr(context, "sound") || strstr(context, "aroma") ||
        strstr(context, "whisper") || strstr(context, "echo") || strstr(context, "drip") ||
        strstr(context, "silence") || strstr(context, "rustle") || strstr(context, "murmur") ||
        strstr(context, "fragrance") || strstr(context, "texture") || strstr(context, "taste")) {
        int count = 0;
        while (sensory_transitions[count]) count++;
        return sensory_transitions[rand() % count];
    }
    
    // Style-specific transitions with enhanced variety
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
        case STYLE_PASTORAL:
            {
                int count = 0;
                while (pastoral_transitions[count]) count++;
                return pastoral_transitions[rand() % count];
            }
        case STYLE_POETIC:
            {
                int count = 0;
                while (poetic_transitions[count]) count++;
                return poetic_transitions[rand() % count];
            }
        case STYLE_PRACTICAL:
            // Practical style uses minimal, functional transitions
            {
                const char *practical_options[] = {"", "Additionally, ", "Furthermore, ", NULL};
                return practical_options[rand() % 3];
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
 * Apply comprehensive style-specific transformation to text
 */
char *apply_regional_style_transformation(const char *text, int style) {
    char *transformed;
    const char **style_adjectives, **style_verbs;

    if (!text) return NULL;

    // First apply vocabulary transformation
    transformed = apply_vocabulary_transformation(text, style);
    if (!transformed) return NULL;

    style_adjectives = get_style_adjectives(style);
    style_verbs = get_style_verbs(style);

    /* TODO: Implement proper usage of style_adjectives and style_verbs */
    UNUSED(style_adjectives);
    UNUSED(style_verbs);
    
    // Apply style-specific enhancements and modifiers
    switch (style) {
        case STYLE_MYSTERIOUS:
            {
                // Add mysterious atmosphere and subtle enhancements
                char *enhanced = malloc(strlen(transformed) + 200);
                if (enhanced) {
                    // Add mysterious prefixes to natural elements
                    if (strstr(transformed, "light") && !strstr(transformed, "dim")) {
                        snprintf(enhanced, strlen(transformed) + 200, "dim, %s", transformed);
                        free(transformed);
                        transformed = enhanced;
                    } else if (strstr(transformed, "sounds") && !strstr(transformed, "muffled")) {
                        snprintf(enhanced, strlen(transformed) + 200, "muffled %s", transformed);
                        free(transformed);
                        transformed = enhanced;
                    } else {
                        free(enhanced);
                    }
                }
            }
            break;
            
        case STYLE_DRAMATIC:
            {
                // Enhance with powerful, imposing language
                char *enhanced = malloc(strlen(transformed) + 200);
                if (enhanced) {
                    if (strstr(transformed, "extends") || strstr(transformed, "stretches")) {
                        snprintf(enhanced, strlen(transformed) + 200, "majestically %s", transformed);
                        free(transformed);
                        transformed = enhanced;
                    } else if (strstr(transformed, "rises") || strstr(transformed, "stands")) {
                        snprintf(enhanced, strlen(transformed) + 200, "proudly %s", transformed);
                        free(transformed);
                        transformed = enhanced;
                    } else {
                        free(enhanced);
                    }
                }
            }
            break;
            
        case STYLE_PASTORAL:
            {
                // Add gentle, comforting enhancements
                char *enhanced = malloc(strlen(transformed) + 200);
                if (enhanced) {
                    if (strstr(transformed, "grows") || strstr(transformed, "flourishes")) {
                        snprintf(enhanced, strlen(transformed) + 200, "peacefully %s", transformed);
                        free(transformed);
                        transformed = enhanced;
                    } else if (strstr(transformed, "flows") || strstr(transformed, "moves")) {
                        snprintf(enhanced, strlen(transformed) + 200, "gently %s", transformed);
                        free(transformed);
                        transformed = enhanced;
                    } else {
                        free(enhanced);
                    }
                }
            }
            break;
            
        case STYLE_PRACTICAL:
            // Keep language straightforward and functional - no embellishment
            break;
            
        case STYLE_POETIC:
        default:
            {
                // Add flowing, artistic enhancements
                char *enhanced = malloc(strlen(transformed) + 200);
                if (enhanced) {
                    if (strstr(transformed, "moves") || strstr(transformed, "flows")) {
                        snprintf(enhanced, strlen(transformed) + 200, "gracefully %s", transformed);
                        free(transformed);
                        transformed = enhanced;
                    } else if (strstr(transformed, "appears") || strstr(transformed, "seems")) {
                        snprintf(enhanced, strlen(transformed) + 200, "ethereally %s", transformed);
                        free(transformed);
                        transformed = enhanced;
                    } else {
                        free(enhanced);
                    }
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
            
            if (mysql_pool_query(query, &result) == 0 && result) {
                if ((row = mysql_fetch_row(result))) {
                    regional_style = convert_style_string_to_int(row[0]);
                    narrative_debug_log(2, "Using regional style %d ('%s') for region %d", 
                        regional_style, row[0] ? row[0] : "NULL", region_vnum);
                }
                mysql_pool_free_result(result);
            } else {
                // Fallback to the incorrectly converted value
                regional_style = profile->description_style;
                narrative_debug_log(2, "Using fallback regional style %d for region %d", regional_style, region_vnum);
            }
        }
    }
    
    elements = calloc(1, sizeof(struct narrative_elements));
    if (!elements) return NULL;
    
    // Store regional style for later use in flow generation
    elements->regional_style = regional_style;
    
    // Initialize integration weight
    elements->integration_weight = 0.0f;
    
    // Debug: Check if hints array is valid
    narrative_debug_log(2, "extract_narrative_elements called with hints=%p", hints);
    if (!hints) {
        narrative_debug_log(2, "hints array is NULL");
        return elements;
    }
    
    // Count hints for debugging
    int hint_count = 0;
    for (i = 0; hints[i].hint_text; i++) {
        hint_count++;
    }
    narrative_debug_log(2, "Found %d hints in array", hint_count);
    
    if (hint_count == 0) {
        narrative_debug_log(2, "No hints found, returning empty elements");
        return elements;
    }

    // Analyze each hint for semantic patterns with style awareness
    for (i = 0; hints[i].hint_text; i++) {
        text = hints[i].hint_text;
        if (!text) continue;
        
        narrative_debug_log(2, "Processing hint %d: '%.50s...'", i, text);        // Extract mood indicators with style preference
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
 * Safe string replacement function - returns new allocated string
 */
char *replace_string_safe(const char *str, const char *find, const char *replace) {
    if (!str || !find || !replace) return strdup(str ? str : "");
    
    char *pos = strstr(str, find);
    if (!pos) return strdup(str);
    
    size_t str_len = strlen(str);
    size_t find_len = strlen(find);
    size_t replace_len = strlen(replace);
    size_t new_len = str_len - find_len + replace_len + 1;
    
    char *result = malloc(new_len);
    if (!result) return strdup(str);
    
    // Copy part before the match
    size_t prefix_len = pos - str;
    strncpy(result, str, prefix_len);
    result[prefix_len] = '\0';
    
    // Add replacement
    strcat(result, replace);
    
    // Add part after the match
    strcat(result, pos + find_len);
    
    return result;
}

/**
 * Apply semantic transformations to modify description mood
 */
void transform_description_mood(struct description_components *desc, const char *target_mood) {
    if (!desc || !target_mood) return;
    
    if (strcmp(target_mood, "ethereal") == 0) {
        // Transform to ethereal/mystical mood for Mosswood
        if (desc->opening_imagery) {
            char *temp = strdup(desc->opening_imagery);
            if (temp) {
                // Apply multiple transformations safely
                char *step1 = replace_string_safe(temp, "oak trees", "ancient trees draped in thick, velvety moss");
                char *step2 = replace_string_safe(step1, "Towering oak", "Moss-covered ancient");
                char *step3 = replace_string_safe(step2, "trees", "moss-laden trees");
                char *step4 = replace_string_safe(step3, "canopy", "emerald canopy of moss-draped branches");
                char *step5 = replace_string_safe(step4, "forest", "mystical moss forest");
                char *step6 = replace_string_safe(step5, "dance", "stand silently");
                char *final = replace_string_safe(step6, "harmonious symphony", "profound silence");
                
                // Free intermediate results
                free(temp);
                free(step1);
                free(step2);
                free(step3);
                free(step4);
                free(step5);
                free(step6);
                
                // Replace the original
                free(desc->opening_imagery);
                desc->opening_imagery = final;
            }
        }
    } else if (strcmp(target_mood, "mysterious") == 0) {
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
    
    // Enhanced element injection for Mosswood and other regions
    if (strcmp(active_elements, "ethereal_glow") == 0) {
        // Add ethereal glow elements
        if (desc->descriptive_verbs) {
            char *enhanced_verbs = malloc(strlen(desc->descriptive_verbs) + 200);
            if (enhanced_verbs) {
                snprintf(enhanced_verbs, strlen(desc->descriptive_verbs) + 200, 
                        "%s, while an ethereal green glow emanates from bioluminescent moss patches", 
                        desc->descriptive_verbs);
                free(desc->descriptive_verbs);
                desc->descriptive_verbs = enhanced_verbs;
            }
        }
    } else if (strcmp(active_elements, "moss_carpet") == 0) {
        // Add moss carpet elements  
        if (desc->atmospheric_modifiers) {
            char *enhanced_atmosphere = malloc(strlen(desc->atmospheric_modifiers) + 150);
            if (enhanced_atmosphere) {
                snprintf(enhanced_atmosphere, strlen(desc->atmospheric_modifiers) + 150,
                        "%s A thick emerald carpet of moss muffles every footstep beneath your feet.", 
                        desc->atmospheric_modifiers);
                free(desc->atmospheric_modifiers);
                desc->atmospheric_modifiers = enhanced_atmosphere;
            }
        }
    } else if (strcmp(active_elements, "whisper") == 0 && strstr(desc->descriptive_verbs, "tower")) {
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
    
    narrative_debug_log(2, "inject_temporal_and_sensory_elements called - opening_imagery=%p, temporal=%s, sensory=%s", 
        desc->opening_imagery, temporal_aspects ? "present" : "none", sensory_details ? "present" : "none");
    
    // Add temporal elements to opening imagery
    if (temporal_aspects && desc->opening_imagery) {
        char *old_opening = desc->opening_imagery;
        char *temp_buffer = malloc(MAX_STRING_LENGTH * 2);
        narrative_debug_log(2, "Allocated temp_buffer=%p, will free old_opening=%p", temp_buffer, old_opening);
        
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
            
            // Free old memory first
            free(old_opening);
            
            // Use strdup to create a properly sized copy instead of transferring ownership
            desc->opening_imagery = strdup(temp_buffer);
            narrative_debug_log(2, "Created new opening_imagery=%p, freeing temp_buffer=%p", desc->opening_imagery, temp_buffer);
            
            // Free the temporary buffer 
            free(temp_buffer);
        } else {
            narrative_debug_log(1, "Failed to allocate temp_buffer");
        }
    }
    
    // Add sensory details as sensory additions
    if (sensory_details && !desc->sensory_additions) {
        desc->sensory_additions = strdup(sensory_details);
        narrative_debug_log(2, "Set sensory_additions=%p", desc->sensory_additions);
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
    
    narrative_debug_log(2, "free_description_components called with components=%p", components);
    
    if (components->opening_imagery) {
        narrative_debug_log(2, "Freeing opening_imagery=%p", components->opening_imagery);
        free(components->opening_imagery);
    }
    if (components->primary_nouns) {
        narrative_debug_log(2, "Freeing primary_nouns=%p", components->primary_nouns);
        free(components->primary_nouns);
    }
    if (components->descriptive_verbs) {
        narrative_debug_log(2, "Freeing descriptive_verbs=%p", components->descriptive_verbs);
        free(components->descriptive_verbs);
    }
    if (components->atmospheric_modifiers) {
        narrative_debug_log(2, "Freeing atmospheric_modifiers=%p", components->atmospheric_modifiers);
        free(components->atmospheric_modifiers);
    }
    if (components->sensory_additions) {
        narrative_debug_log(2, "Freeing sensory_additions=%p", components->sensory_additions);
        free(components->sensory_additions);
    }
    if (components->closing_elements) {
        narrative_debug_log(2, "Freeing closing_elements=%p", components->closing_elements);
        free(components->closing_elements);
    }
    
    narrative_debug_log(2, "Freeing components structure=%p", components);
    free(components);
}

/* ====================================================================== */
/*                         HINT LOADING FUNCTIONS                        */
/* ====================================================================== */

/**
 * Optimized cached hint loading with performance enhancements
 */
struct region_hint *load_contextual_hints_cached(int region_vnum, const char *weather_condition, 
                                                const char *time_category, double resource_health) {
    /* Create cache key */
    struct hint_cache_key cache_key;
    cache_key.region_vnum = region_vnum;
    cache_key.weather_condition = get_weather_code_from_string(weather_condition);
    cache_key.time_category = get_time_code_from_string(time_category);
    cache_key.season = get_season_from_time();
    cache_key.resource_health = resource_health;
    
    /* Try to get from cache first */
    struct region_hint *cached_hints = get_cached_hints(&cache_key);
    if (cached_hints) {
        return cached_hints;
    }
    
    /* Cache miss - load from database using optimized query */
    struct region_hint *fresh_hints = load_contextual_hints_optimized(region_vnum, weather_condition, time_category);
    
    /* Cache the fresh hints for future use */
    if (fresh_hints) {
        cache_hints(&cache_key, fresh_hints);
    }
    
    return fresh_hints;
}

/**
 * Convert weather condition string to integer code for caching
 */
int get_weather_code_from_string(const char *weather_condition) {
    if (!weather_condition) return 0;
    if (strcmp(weather_condition, "clear") == 0) return 1;
    if (strcmp(weather_condition, "cloudy") == 0) return 2;
    if (strcmp(weather_condition, "rainy") == 0) return 3;
    if (strcmp(weather_condition, "stormy") == 0) return 4;
    if (strcmp(weather_condition, "snowy") == 0) return 5;
    if (strcmp(weather_condition, "foggy") == 0) return 6;
    return 0; /* Unknown */
}

/**
 * Convert time category string to integer code for caching
 */
int get_time_code_from_string(const char *time_category) {
    if (!time_category) return 0;
    if (strcmp(time_category, "dawn") == 0) return 1;
    if (strcmp(time_category, "morning") == 0) return 2;
    if (strcmp(time_category, "midday") == 0) return 3;
    if (strcmp(time_category, "afternoon") == 0) return 4;
    if (strcmp(time_category, "dusk") == 0) return 5;
    if (strcmp(time_category, "evening") == 0) return 6;
    if (strcmp(time_category, "night") == 0) return 7;
    if (strcmp(time_category, "midnight") == 0) return 8;
    return 0; /* Unknown */
}

/**
 * Optimized database query version with batch loading
 */
struct region_hint *load_contextual_hints_optimized(int region_vnum, const char *weather_condition, const char *time_category) {
    MYSQL_RES *result;
    MYSQL_ROW row;
    char query[MAX_STRING_LENGTH * 2]; /* Larger buffer for complex query */
    struct region_hint *hints = NULL;
    struct region_hint *current_hint = NULL;
    int hint_count = 0;
    int current_season = get_season_from_time();
    
    /* Optimized query with existing table columns only */
    sprintf(query,
        "SELECT hint_category, hint_text, priority, seasonal_weight, time_of_day_weight "
        "FROM region_hints "
        "WHERE region_vnum = %d "
        "AND is_active = 1 "
        "AND (weather_conditions IS NULL OR weather_conditions = '' OR FIND_IN_SET('%s', weather_conditions) > 0) "
        "ORDER BY priority DESC "
        "LIMIT 20",
        region_vnum, weather_condition ? weather_condition : "");
    
    if (!mysql_pool) {
        log("SYSERR: No database connection for loading contextual hints");
        return NULL;
    }
    
    if (mysql_pool_query(query, &result) != 0) {
        log("SYSERR: MySQL query error in load_contextual_hints_optimized");
        return NULL;
    }
    
    if (!result) {
        log("SYSERR: MySQL store result error in load_contextual_hints_optimized");
        return NULL;
    }
    
    /* Allocate array for hints with extra space */
    hints = calloc(21, sizeof(struct region_hint)); /* 20 hints + terminator */
    if (!hints) {
        mysql_pool_free_result(result);
        log("SYSERR: Memory allocation failed for contextual hints");
        return NULL;
    }
    
    /* Process query results with enhanced filtering */
    while ((row = mysql_fetch_row(result)) && hint_count < 20) {
        /* Calculate contextual weights */
        double seasonal_weight = 1.0;
        double time_weight = 1.0;
        double combined_weight = 1.0;
        
        /* Get weights from JSON columns */
        if (row[3]) { /* seasonal_weight JSON */
            seasonal_weight = get_seasonal_weight_for_hint(row[3], current_season);
        }
        if (row[4]) { /* time_of_day_weight JSON */
            time_weight = get_time_weight_for_hint(row[4], time_category);
        }
        
        /* Combined weight calculation */
        combined_weight = seasonal_weight * time_weight;
        
        /* Apply relevance threshold */
        if (combined_weight < 0.25) {
            continue; /* Skip low-relevance hints */
        }
        
        current_hint = &hints[hint_count];
        
        /* Enhanced category mapping with error checking */
        if (row[0]) {
            current_hint->hint_category = get_hint_category_from_string(row[0]);
        } else {
            current_hint->hint_category = 0; /* Default */
        }
        
        /* Copy and transform hint text */
        if (row[1]) {
            current_hint->hint_text = transform_voice_to_observational(row[1]);
        } else {
            current_hint->hint_text = strdup(""); /* Empty fallback */
        }
        
        /* Copy priority with contextual adjustment */
        if (row[2]) {
            current_hint->priority = atoi(row[2]) * combined_weight;
        } else {
            current_hint->priority = 1.0 * combined_weight;
        }
        
        /* Store contextual weight and prepare storage fields */
        current_hint->contextual_weight = combined_weight;
        current_hint->seasonal_weight = row[3] ? strdup(row[3]) : strdup("");
        current_hint->time_of_day_weight = row[4] ? strdup(row[4]) : strdup("");
        
        hint_count++;
    }
    
    mysql_pool_free_result(result);
    
    /* Null terminate the hints array */
    if (hint_count < 21) {
        hints[hint_count].hint_text = NULL;
    }
    
    log("DEBUG: Loaded %d optimized contextual hints for region %d", hint_count, region_vnum);
    return hints;
}

/**
 * Convert hint category string to enum constant
 */
int get_hint_category_from_string(const char *category_str) {
    if (!category_str) return 0;
    
    if (strcmp(category_str, "atmosphere") == 0) return HINT_ATMOSPHERE;
    if (strcmp(category_str, "fauna") == 0) return HINT_FAUNA;
    if (strcmp(category_str, "flora") == 0) return HINT_FLORA;
    if (strcmp(category_str, "geography") == 0) return HINT_GEOGRAPHY;
    if (strcmp(category_str, "weather_influence") == 0) return HINT_WEATHER_INFLUENCE;
    if (strcmp(category_str, "resources") == 0) return HINT_RESOURCES;
    if (strcmp(category_str, "landmarks") == 0) return HINT_LANDMARKS;
    if (strcmp(category_str, "sounds") == 0) return HINT_SOUNDS;
    if (strcmp(category_str, "scents") == 0) return HINT_SCENTS;
    if (strcmp(category_str, "seasonal_changes") == 0) return HINT_SEASONAL_CHANGES;
    if (strcmp(category_str, "time_of_day") == 0) return HINT_TIME_OF_DAY;
    if (strcmp(category_str, "mystical") == 0) return HINT_MYSTICAL;
    
    return 0; /* Default/unknown category */
}

/**
 * Legacy hint loading function (maintained for compatibility)
 */
struct region_hint *load_contextual_hints(int region_vnum, const char *weather_condition, const char *time_category) {
    /* For backward compatibility, call optimized cached version with default resource health */
    return load_contextual_hints_cached(region_vnum, weather_condition, time_category, 0.5);
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
    
    if (mysql_pool_query(query, &result) != 0) {
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
 * Load and filter relevant hints for current conditions with AI mood integration
 * @param region_vnum The region vnum to load hints for
 * @param weather_condition Current weather conditions
 * @param time_category Current time of day
 * @return Array of region hints, or NULL if none found
 */
struct region_hint *load_contextual_hints_legacy(int region_vnum, const char *weather_condition, const char *time_category) {
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
    
    if (mysql_pool_query(query, &result) != 0) {
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

/**
 * Load regional characteristics for mood-based hint weighting
 * @param region_vnum The region vnum to load characteristics for
 * @return JSON string containing key_characteristics, or NULL if not found
 */
char *load_region_characteristics(int region_vnum) {
    MYSQL_RES *result = NULL;
    MYSQL_ROW row;
    char query[MAX_STRING_LENGTH];
    char *characteristics = NULL;
    
    /* Ensure database connection is available */
    if (!mysql_pool) {
        log("SYSERR: No database connection for loading AI characteristics");
        return NULL;
    }
    
    /* Build query with bounds checking */
    snprintf(query, sizeof(query),
        "SELECT key_characteristics FROM region_profiles WHERE region_vnum = %d",
        region_vnum);
    
    /* Execute query using connection pool */
    if (mysql_pool_query(query, &result) != 0) {
        log("SYSERR: MySQL query error in load_region_characteristics: %s", query);
        return NULL;
    }
    
    /* Check for valid result set */
    if (!result) {
        log("SYSERR: MySQL returned NULL result in load_region_characteristics");
        return NULL;
    }
    
    /* Fetch the first row if available */
    row = mysql_fetch_row(result);
    if (row && row[0] && *row[0]) {
        characteristics = strdup(row[0]);
        narrative_debug_log(2, "Loaded AI characteristics for region %d: %.100s...", 
            region_vnum, characteristics);
    } else {
        narrative_debug_log(1, "No AI characteristics found for region %d", region_vnum);
    }
    
    /* Always free the result set */
    mysql_pool_free_result(result);
    
    return characteristics;
}

/* ====================================================================== */
/*                         NARRATIVE WEAVING CORE                        */
/* ====================================================================== */

/**
 * Intelligently weave hints into unified description with AI mood-based weighting
 */
char *weave_unified_description(const char *base_description, struct region_hint *hints, 
                               const char *weather_condition, const char *time_category, int x, int y) {
    char *unified;
    int i;
    int atmosphere_added = 0;
    int wildlife_added = 0;
    int weather_added = 0;
    char *regional_characteristics = NULL;
    
    /* Load regional characteristics for mood-based weighting */
    if (hints && hints[0].hint_text) {
        int region_vnum = 1000004; /* Default to Mosswood for now, should be derived from coordinates */
        regional_characteristics = load_region_characteristics(region_vnum);
        if (!regional_characteristics) {
            log("DEBUG: No characteristics found for region %d, using basic weighting", region_vnum);
        }
    }
    
    /* Build comprehensive environmental context for sophisticated hint selection */
    struct environmental_context env_context;
    
    /* Get actual wilderness weather value for enhanced weather relevance */
    int wilderness_weather_raw = get_weather(x, y);
    env_context.weather = wilderness_weather_raw; /* Use raw weather value for intensity calculations */
    env_context.time_of_day = weather_info.sunlight; /* Current sun state */
    env_context.season = get_season_from_time();
    env_context.light_level = 100; /* Full light - TODO: Calculate properly */
    env_context.artificial_light = 0; /* No artificial light default */
    env_context.natural_light = 100; /* Full natural light */
    env_context.terrain_type = 0; /* TODO: Get from terrain system */
    env_context.elevation = 0.0; /* Sea level default */
    env_context.near_water = false;
    env_context.in_forest = false;
    env_context.in_mountains = false;
    env_context.has_light_sources = false;
    
    log("DEBUG: Environmental context - wilderness_weather=%d (%s), time_of_day=%d, season=%d, coords=(%d,%d)", 
        env_context.weather, weather_condition, env_context.time_of_day, env_context.season, x, y);
    
    unified = malloc(MAX_STRING_LENGTH * 3);
    if (!unified) {
        if (regional_characteristics) free(regional_characteristics);
        return NULL;
    }
    
    // Build natural description from hints and environmental context
    safe_strcpy(unified, "", MAX_STRING_LENGTH * 3);
    int sentence_count = 0;
    int used_hints[50];  // Track which hints we've used
    int used_count = 0;
    
    // Initialize random seed based on coordinates for location consistency
    srand(x * 1000 + y + time_info.hours);
    
    // Start with atmospheric foundation (using weighted selection)
    int atmosphere_hints[10];
    int atmosphere_count = 0;
    for (i = 0; hints && hints[i].hint_text && atmosphere_count < 10; i++) {
        if (hints[i].hint_category == HINT_ATMOSPHERE) {
            atmosphere_hints[atmosphere_count++] = i;
        }
    }
    if (atmosphere_count > 0 && !atmosphere_added) {
        int selected_index = select_contextual_weighted_hint(hints, atmosphere_hints, atmosphere_count, &env_context, regional_characteristics);
        int selected = atmosphere_hints[selected_index];
        if (sentence_count > 0) safe_strcat(unified, " ");
        safe_strcat(unified, hints[selected].hint_text);
        used_hints[used_count++] = selected;
        atmosphere_added = 1;
        sentence_count++;
    }
    
    // Add enhanced weather influence with intensity-based selection
    if (!weather_added && env_context.weather > 32) { // Only add weather effects if significant weather
        int weather_hints[20];
        int weather_count = 0;
        int already_used, j;
        
        // Collect all weather-relevant hints (not just HINT_WEATHER_INFLUENCE)
        for (i = 0; hints && hints[i].hint_text && weather_count < 20; i++) {
            if (hints[i].hint_category == HINT_WEATHER_INFLUENCE ||
                hints[i].hint_category == HINT_SOUNDS ||
                hints[i].hint_category == HINT_ATMOSPHERE ||
                hints[i].hint_category == HINT_FLORA) {
                
                // Check if already used
                already_used = 0;
                for (j = 0; j < used_count; j++) {
                    if (used_hints[j] == i) {
                        already_used = 1;
                        break;
                    }
                }
                
                if (!already_used) {
                    // Use enhanced weather relevance to filter suitable hints
                    double weather_relevance = calculate_weather_relevance_for_hint(&hints[i], env_context.weather);
                    if (weather_relevance > 1.2) { // Only include hints that are enhanced by weather
                        weather_hints[weather_count++] = i;
                    }
                }
            }
        }
        
        if (weather_count > 0) {
            int selected_index = select_contextual_weighted_hint(hints, weather_hints, weather_count, &env_context, regional_characteristics);
            int selected = weather_hints[selected_index];
            if (sentence_count > 0) safe_strcat(unified, " ");
            safe_strcat(unified, hints[selected].hint_text);
            used_hints[used_count++] = selected;
            weather_added = 1;
            sentence_count++;
            
            log("DEBUG: Enhanced weather hint added (weather=%d, relevance=%.2f): %s", 
                env_context.weather, calculate_weather_relevance_for_hint(&hints[selected], env_context.weather), 
                hints[selected].hint_text);
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
                int selected_index = select_contextual_weighted_hint(hints, fauna_hints, fauna_count, &env_context, regional_characteristics);
                int selected = fauna_hints[selected_index];
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
            int selected_index = select_contextual_weighted_hint(hints, flora_hints, flora_count, &env_context, regional_characteristics);
            int selected = flora_hints[selected_index];
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
            int selected_index = select_contextual_weighted_hint(hints, sensory_hints, sensory_count, &env_context, regional_characteristics);
            int selected = sensory_hints[selected_index];
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
            int selected_index = select_contextual_weighted_hint(hints, seasonal_hints, seasonal_count, &env_context, regional_characteristics);
            int selected = seasonal_hints[selected_index];
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
            int selected_index = select_contextual_weighted_hint(hints, time_hints, time_count, &env_context, regional_characteristics);
            int selected = time_hints[selected_index];
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
            int selected_index = select_contextual_weighted_hint(hints, mystical_hints, mystical_count, &env_context, regional_characteristics);
            int selected = mystical_hints[selected_index];
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
    
    log("DEBUG: Wove unified description with AI weighting: atmosphere=%d, wildlife=%d, weather=%d", 
        atmosphere_added, wildlife_added, weather_added);
    
    // Cleanup regional characteristics
    if (regional_characteristics) {
        free(regional_characteristics);
    }
    
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
    narrative_debug_log(1, "Using region vnum %d for hint enhancement", region_vnum);
    
    // Get environmental context
    weather_condition = get_wilderness_weather_condition(x, y);
    time_category = get_time_of_day_category();
    
    // Calculate resource health for performance-optimized caching
    float resource_health = calculate_regional_resource_health(x, y, 5);
    
    // Load contextual hints for current conditions using optimized cached version
    hints = load_contextual_hints_cached(region_vnum, weather_condition, time_category, resource_health);
    if (!hints) {
        log("DEBUG: No contextual hints available for region %d", region_vnum);
        free_region_list(regions);
        return NULL;
    }
    
    // Apply regional transition effects for smooth boundary changes
    int transition_count = 0;
    struct regional_transition *transitions = calculate_regional_transitions(x, y, 10, &transition_count);
    if (transitions && transition_count > 0) {
        /* Count hints for transition processing */
        int hint_count = 0;
        while (hints[hint_count].hint_text) hint_count++;
        
        /* Apply multi-region blending */
        apply_regional_transition_weights(hints, hint_count, transitions, transition_count);
        
        /* Apply boundary proximity effects */
        apply_boundary_transition_effects(hints, hint_count, x, y, region_vnum);
        
        /* Apply resource-based weighting for dynamic regional atmosphere */
        float resource_health = calculate_regional_resource_health(x, y, 5); /* 5-coordinate radius sampling */
        apply_resource_based_hint_weighting(hints, hint_count, resource_health);
        
        narrative_debug_log(2, "Applied regional transition effects with %d nearby regions, resource health %.2f", 
            transition_count, resource_health);
        
        /* Cleanup transition data */
        int i;
        for (i = 0; i < transition_count; i++) {
            if (transitions[i].characteristics) {
                free(transitions[i].characteristics);
            }
        }
        free(transitions);
    } else {
        /* Even without multiple regions, apply boundary effects for current region */
        int hint_count = 0;
        while (hints[hint_count].hint_text) hint_count++;
        apply_boundary_transition_effects(hints, hint_count, x, y, region_vnum);
        
        /* Apply resource-based weighting for dynamic regional atmosphere */
        float resource_health = calculate_regional_resource_health(x, y, 5); /* 5-coordinate radius sampling */
        apply_resource_based_hint_weighting(hints, hint_count, resource_health);
        
        log("DEBUG: Applied boundary effects and resource weighting (health %.2f) for single region", resource_health);
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
        log("DEBUG: layer_hints_on_base_description called with null parameters - base_description: %s, hints: %s", 
            base_description ? "valid" : "NULL", hints ? "valid" : "NULL");
        return NULL;
    }

    // Check if hints array has any entries
    int hint_count = 0;
    for (hint_count = 0; hints[hint_count].hint_text; hint_count++) {
        // Count hints
    }
    
    narrative_debug_log(1, "Starting semantic integration for location (%d, %d) with %d hints", x, y, hint_count);
    
    if (hint_count == 0) {
        narrative_debug_log(1, "No hints available, falling back to simple layering");
        return simple_hint_layering(base_description, hints, x, y);
    }    // Initialize random seed for location consistency
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
    
    narrative_debug_log(1, "Applying semantic integration with weight %.2f", elements->integration_weight);
    
    // Parse base description into components
    components = parse_description_components(base_description);
    if (!components) {
        narrative_debug_log(1, "Failed to parse description components, using simple layering");
        free_narrative_elements(elements);
        return simple_hint_layering(base_description, hints, x, y);
    }
    
    // Apply semantic transformations
    if (elements->dominant_mood) {
        narrative_debug_log(2, "Transforming mood to: %s", elements->dominant_mood);
        transform_description_mood(components, elements->dominant_mood);
    }
    
    if (elements->active_elements) {
        narrative_debug_log(2, "Injecting dynamic elements: %s", elements->active_elements);
        inject_dynamic_elements(components, elements->active_elements);
    }
    
    // Apply temporal and sensory integration
    if (elements->temporal_aspects || elements->sensory_details) {
        narrative_debug_log(2, "Injecting temporal/sensory elements - temporal: %s, sensory: %s", 
            elements->temporal_aspects ? elements->temporal_aspects : "none",
            elements->sensory_details ? elements->sensory_details : "none");
        inject_temporal_and_sensory_elements(components, elements->temporal_aspects, elements->sensory_details);
    }

    // Reconstruct enhanced description with regional style
    semantically_enhanced = reconstruct_enhanced_description(components, elements->regional_style);
    
    // Cleanup
    free_narrative_elements(elements);
    free_description_components(components);
    
    if (!semantically_enhanced) {
        narrative_debug_log(1, "Semantic reconstruction failed, using simple layering");
        return simple_hint_layering(base_description, hints, x, y);
    }
    
    narrative_debug_log(1, "Semantic integration completed successfully");
    return semantically_enhanced;
}

/**
 * Fallback simple hint layering for when semantic integration isn't possible
 * Now includes AI mood-based weighting
 */
char *simple_hint_layering(char *base_description, struct region_hint *hints, int x, int y) {
    char *enhanced;
    char hint_additions[MAX_STRING_LENGTH];
    int used_hints[20];
    int used_count = 0;
    int i;
    char *regional_characteristics = NULL;
    
    // Debug: Check if hints array is valid in simple layering
    log("DEBUG: simple_hint_layering called with hints=%p", hints);
    if (!hints) {
        log("DEBUG: hints array is NULL in simple layering");
        enhanced = strdup(base_description);
        return enhanced;
    }
    
    // Count hints for debugging
    int hint_count = 0;
    for (i = 0; hints[i].hint_text; i++) {
        hint_count++;
    }
    log("DEBUG: simple_hint_layering found %d hints", hint_count);
    
    if (hint_count == 0) {
        log("DEBUG: No hints found in simple layering, returning base description");
        enhanced = strdup(base_description);
        return enhanced;
    }
    
    // Create environmental context for contextual hint selection
    struct environmental_context env_context;
    env_context.weather = get_weather(x, y); // Use actual wilderness weather
    env_context.time_of_day = SUN_LIGHT; // Midday default
    env_context.season = 1; // Spring
    env_context.light_level = 100; // Full daylight
    env_context.artificial_light = 0; // No artificial light
    env_context.natural_light = 100; // Full natural light
    env_context.terrain_type = 0; // Default terrain
    env_context.elevation = 0.0; // Sea level
    env_context.near_water = false;
    env_context.in_forest = false;
    env_context.in_mountains = false;
    env_context.has_light_sources = false;
    
    /* Load regional characteristics for mood-based weighting */
    if (hints && hints[0].hint_text) {
        int region_vnum = 1000004;
        regional_characteristics = load_region_characteristics(region_vnum);
        if (!regional_characteristics) {
            log("DEBUG: No regional characteristics found for region %d in simple layering", region_vnum);
        }
    }
    
    // Allocate buffer for enhanced description
    enhanced = malloc(MAX_STRING_LENGTH * 2);
    if (!enhanced) {
        if (regional_characteristics) free(regional_characteristics);
        return NULL;
    }
    
    // Start with the base description
    safe_strcpy(enhanced, base_description, MAX_STRING_LENGTH * 2);
    safe_strcpy(hint_additions, "", MAX_STRING_LENGTH);
    
    // Add atmospheric hints (mood/ambiance) using weighted selection
    int atmosphere_hints[10];
    int atmosphere_count = 0;
    for (i = 0; hints && hints[i].hint_text && atmosphere_count < 10; i++) {
        if (hints[i].hint_category == HINT_ATMOSPHERE) {
            atmosphere_hints[atmosphere_count++] = i;
        }
    }
    if (atmosphere_count > 0) {
        int selected_index = select_contextual_weighted_hint(hints, atmosphere_hints, atmosphere_count, &env_context, regional_characteristics);
        int selected = atmosphere_hints[selected_index];
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
        int selected_index = select_contextual_weighted_hint(hints, flora_hints, flora_count, &env_context, regional_characteristics);
        int selected = flora_hints[selected_index];
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
        int selected_index = select_contextual_weighted_hint(hints, fauna_hints, fauna_count, &env_context, regional_characteristics);
        int selected = fauna_hints[selected_index];
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
    
    log("DEBUG: Simple layering with regional weighting applied %d hints to base description", used_count);
    
    // Cleanup regional characteristics
    if (regional_characteristics) {
        free(regional_characteristics);
    }
    
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
    
    // Calculate resource health for optimized caching
    float resource_health = calculate_regional_resource_health(x, y, 5);
    
    // Load contextual hints for current conditions using cached version
    hints = load_contextual_hints_cached(region_vnum, weather_condition, time_category, resource_health);
    
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
    
    narrative_debug_log(1, "Generated base description (%d chars) for (%d, %d)", 
        (int)strlen(base_desc), x, y);
    
    // STEP 2: Try to enhance with regional hints
    enhanced_desc = enhance_base_description_with_hints(base_desc, ch, zone, x, y);
    if (enhanced_desc) {
        narrative_debug_log(1, "Enhanced description with regional hints (%d chars) for (%d, %d)", 
            (int)strlen(enhanced_desc), x, y);
        free(base_desc);  // Replace with enhanced version
        return enhanced_desc;
    }
    
    // STEP 3: No enhancement possible, return base description
    narrative_debug_log(1, "No regional enhancement available, using base description for (%d, %d)", x, y);
    return base_desc;
}
