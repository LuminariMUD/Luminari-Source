/* *************************************************************************
 *   File: resource_system.c                           Part of LuminariMUD *
 *  Usage: Implementation of the wilderness resource system                *
 * Author: Implementation Team                                             *
 ***************************************************************************
 *                                                                         *
 ***************************************************************************/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "wilderness.h"
#include "perlin.h"
#include "resource_system.h"
#include "resource_depletion.h"  /* Phase 6: Add depletion system */
#include "resource_descriptions.h"  /* For elevation functions */
#include "mysql.h"
#include "spells.h"
#include "genolc.h"
#include "constants.h"
#include "kdtree.h"
#include "screen.h"

/* Simple absolute value function to avoid math.h conflicts */
static float simple_fabs(float value) {
    return (value < 0) ? -value : value;
}

/* Forward declarations for wilderness functions not in headers */
extern int get_temperature(int map, int x, int y);
extern int get_moisture(int map, int x, int y);

/* Forward declarations for region integration */
/* Phase 4b: Region Effects Forward Declarations */

/* Forward declarations for enhanced material functions */
int get_enhanced_wilderness_material_id(int category, int subtype);
const char *get_enhanced_material_name(int category, int subtype, int quality);
int get_enhanced_material_crafting_value(int category, int subtype, int quality);

/* Global resource configuration array */
struct resource_config resource_configs[NUM_RESOURCE_TYPES] = {
    /* type, noise_layer, base_mult, regen_rate, depletion, quality_var, seasonal, weather, skill, name, description */
    {NOISE_VEGETATION, 1.0, 0.2, 0.8, 20, true, true, SKILL_FORESTING, "vegetation", "General plant life and foliage"},
    {NOISE_MINERALS, 0.3, 0.01, 0.9, 30, false, false, SKILL_MINING, "minerals", "Ores, metals, and mineral deposits"},
    {NOISE_WATER_RESOURCE, 1.2, 0.5, 0.6, 10, false, true, SKILL_FORESTING, "water", "Fresh water sources and springs"},
    {NOISE_HERBS, 0.4, 0.1, 0.7, 40, true, true, SKILL_FORESTING, "herbs", "Medicinal and magical plants"},
    {NOISE_GAME, 0.6, 0.15, 0.5, 25, true, false, SKILL_HUNTING, "game", "Wildlife and huntable animals"},
    {NOISE_WOOD, 0.8, 0.05, 0.9, 15, true, false, SKILL_FORESTING, "wood", "Harvestable timber and lumber"},
    {NOISE_STONE, 0.5, 0.005, 0.95, 5, false, false, SKILL_MINING, "stone", "Building stone and quarry materials"},
    {NOISE_CRYSTAL, 0.1, 0.001, 0.99, 50, false, false, SKILL_MINING, "crystal", "Rare magical crystal formations"},
    {NOISE_MINERALS, 0.3, 0.02, 0.8, 15, false, true, SKILL_MINING, "clay", "Clay deposits for pottery and crafting"},
    {NOISE_WATER_RESOURCE, 0.2, 0.03, 0.7, 20, false, true, SKILL_MINING, "salt", "Salt deposits and brine pools"}
};

/* Resource name array for display */
const char *resource_names[NUM_RESOURCE_TYPES] = {
    "vegetation", "minerals", "water", "herbs", "game", 
    "wood", "stone", "crystal", "clay", "salt"
};

/* Global KD-tree for resource nodes */
struct resource_node *resource_kd_tree = NULL;

/* Global KD-tree for resource cache */
struct resource_cache_node *resource_cache_tree = NULL;
int resource_cache_count = 0;

/* Resource abundance descriptions */
static struct resource_abundance abundance_levels[] = {
    {0.9, "incredibly abundant", "The area is incredibly rich with resources"},
    {0.8, "very abundant", "Resources are very plentiful here"},
    {0.6, "abundant", "A good amount of resources can be found"},
    {0.4, "moderate", "Moderate resource levels are present"},
    {0.2, "scarce", "Resources are somewhat scarce"},
    {0.1, "very scarce", "Very few resources remain"},
    {0.0, "depleted", "Resources have been exhausted"}
};

/* ===== CORE RESOURCE CALCULATION FUNCTIONS ===== */

/* Main resource calculation function - lazy evaluation with caching */
float calculate_current_resource_level(int resource_type, int x, int y) {
    struct resource_cache_node *cached;
    float calculated_values[NUM_RESOURCE_TYPES];
    int i;
    
    if (resource_type < 0 || resource_type >= NUM_RESOURCE_TYPES) {
        log("SYSERR: Invalid resource type %d in calculate_current_resource_level", resource_type);
        return 0.0;
    }
    
    /* Check cache first */
    cached = cache_find_resource_values(x, y);
    if (cached != NULL) {
        return cached->cached_values[resource_type];
    }
    
    /* Cache miss - calculate all resource types for this grid position */
    for (i = 0; i < NUM_RESOURCE_TYPES; i++) {
        float base_value = get_base_resource_value(i, x, y);
        
        /* Apply region modifiers */
        base_value = apply_region_resource_modifiers(i, x, y, base_value);
        
        /* Apply seasonal/weather modifiers */
        base_value = apply_environmental_modifiers(i, x, y, base_value);
        
        /* Check for harvest history and calculate regeneration */
        struct resource_node *node = kdtree_find_resource_node(x, y);
        if (node) {
            base_value = apply_harvest_regeneration(i, base_value, node);
        }
        
        /* Store in temporary array */
        if (base_value < 0.0) calculated_values[i] = 0.0;
        else if (base_value > 1.0) calculated_values[i] = 1.0;
        else calculated_values[i] = base_value;
    }
    
    /* Store in cache */
    cache_store_resource_values(x, y, calculated_values);
    
    return calculated_values[resource_type];
}

/* Get base resource value from Perlin noise */
float get_base_resource_value(int resource_type, int x, int y) {
    if (resource_type < 0 || resource_type >= NUM_RESOURCE_TYPES) {
        return 0.0;
    }
    
    struct resource_config *config = &resource_configs[resource_type];
    
    /* Get environmental data for realistic resource distribution */
    int elevation = get_modified_elevation(x, y);                           /* 0-255 range with region mods */
    int temperature = get_temperature(NOISE_MATERIAL_PLANE_ELEV, x, y);      /* Temperature */
    int moisture = get_moisture(NOISE_MATERIAL_PLANE_MOISTURE, x, y);        /* Moisture level */
    
    /* Normalize environmental factors to 0.0-1.0 range */
    float norm_elevation = elevation / 255.0f;
    float norm_temperature = temperature / 255.0f;  
    float norm_moisture = moisture / 255.0f;
    
    float final_value = 0.0f;
    
    /* Apply resource-specific distribution logic */
    switch(resource_type) {
        case RESOURCE_VEGETATION:
            /* Primarily environmental with subtle natural variation */
            {
                float elevation_factor = 1.0f - ((norm_elevation - 0.4f) * (norm_elevation - 0.4f)) * 2.5f;
                if (elevation_factor < 0.1f) elevation_factor = 0.1f;
                float environmental = (elevation_factor * 0.4f) + (norm_moisture * 0.4f) + 
                                    (1.0f - simple_fabs(norm_temperature - 0.5f) * 0.2f);
                
                /* Add subtle natural variation (10% influence) */
                double norm_x = x / (double)(WILD_X_SIZE / 16.0);
                double norm_y = y / (double)(WILD_Y_SIZE / 16.0);
                double micro_noise = PerlinNoise2D(config->noise_layer, norm_x, norm_y, 3.0, 2.0, 4);
                float micro_factor = (micro_noise + 1.0f) / 2.0f;
                
                final_value = (environmental * 0.9f) + (micro_factor * 0.1f);
            }
            break;
            
        case RESOURCE_WATER:
            /* Environmental water distribution with terrain-specific modifiers */
            final_value = (1.0f - norm_elevation * 0.8f) + (norm_moisture * 0.5f);
            
            /* Desert terrain penalty - severely limit water in arid regions */
            /* Desert conditions: temperature > 25°C (98/255 = ~0.38) and moisture < 80 (80/255 = ~0.31) */
            if (temperature > 25 && moisture < 80) {
                /* Severe desert penalty - reduce water to 5-15% of normal */
                final_value *= 0.10f;  /* 90% reduction */
                
                /* Ultra-arid conditions get even less water */
                if (moisture < 40) {  /* Extremely dry desert */
                    final_value *= 0.5f;  /* Additional 50% reduction (total ~5% of normal) */
                }
            }
            /* Semi-arid conditions - moderate penalty */
            else if (temperature > 20 && moisture < 120) {
                /* Semi-desert conditions - reduce water by 40% */
                final_value *= 0.6f;
            }
            break;
            
        case RESOURCE_HERBS:
            /* Primarily environmental with subtle natural variation */
            {
                float environmental = (norm_moisture * 0.3f) + (norm_temperature * 0.3f) + 
                                    ((1.0f - norm_elevation) * 0.4f);
                
                /* Add subtle natural variation (15% influence) */
                double norm_x = x / (double)(WILD_X_SIZE / 12.0);
                double norm_y = y / (double)(WILD_Y_SIZE / 12.0);
                double micro_noise = PerlinNoise2D(config->noise_layer + 1, norm_x, norm_y, 2.5, 2.0, 3);
                float micro_factor = (micro_noise + 1.0f) / 2.0f;
                
                final_value = (environmental * 0.85f) + (micro_factor * 0.15f);
            }
            break;
            
        case RESOURCE_GAME:
            /* Primarily environmental with natural variation for animal movement patterns */
            {
                float vegetation_proxy = (norm_moisture * 0.4f) + ((1.0f - norm_elevation) * 0.3f);
                float water_proxy = (1.0f - norm_elevation * 0.6f) + (norm_moisture * 0.3f);
                float environmental = (vegetation_proxy * 0.6f) + (water_proxy * 0.4f);
                
                /* Add natural variation for animal movement (20% influence) */
                double norm_x = x / (double)(WILD_X_SIZE / 8.0);
                double norm_y = y / (double)(WILD_Y_SIZE / 8.0);
                double animal_noise = PerlinNoise2D(config->noise_layer + 2, norm_x, norm_y, 2.0, 2.2, 5);
                float animal_factor = (animal_noise + 1.0f) / 2.0f;
                
                final_value = (environmental * 0.8f) + (animal_factor * 0.2f);
            }
            break;
            
        case RESOURCE_WOOD:
            /* Primarily environmental with natural forest variation */
            {
                float tree_elevation = 1.0f - ((norm_elevation - 0.5f) * (norm_elevation - 0.5f)) * 3.0f;
                if (tree_elevation < 0.1f) tree_elevation = 0.1f;
                float environmental = (tree_elevation * 0.5f) + (norm_moisture * 0.3f) + 
                                    (norm_temperature * 0.2f);
                
                /* Add natural forest variation (15% influence) */
                double norm_x = x / (double)(WILD_X_SIZE / 10.0);
                double norm_y = y / (double)(WILD_Y_SIZE / 10.0);
                double forest_noise = PerlinNoise2D(config->noise_layer + 3, norm_x, norm_y, 2.8, 2.0, 4);
                float forest_factor = (forest_noise + 1.0f) / 2.0f;
                
                final_value = (environmental * 0.85f) + (forest_factor * 0.15f);
            }
            break;
            
        case RESOURCE_MINERALS:
        case RESOURCE_CRYSTAL:
        case RESOURCE_STONE:
            /* Geological resources - use Perlin noise for mineral veins and formations */
            {
                double norm_x = x / (double)(WILD_X_SIZE / 4.0);
                double norm_y = y / (double)(WILD_Y_SIZE / 4.0);
                double geological_noise = PerlinNoise2D(config->noise_layer, norm_x, norm_y, 2.0, 2.0, 8);
                float geological_factor = (geological_noise + 1.0f) / 2.0f;
                
                /* Combine geological formations with elevation preference for minerals */
                float environmental_factor = norm_elevation * 0.6f + 0.2f; /* Higher at elevation */
                final_value = (geological_factor * 0.7f) + (environmental_factor * 0.3f);
            }
            break;
            
        case RESOURCE_CLAY:
        case RESOURCE_SALT:
            /* Geological resources - specific formations, often near water */
            {
                double norm_x = x / (double)(WILD_X_SIZE / 4.0);
                double norm_y = y / (double)(WILD_Y_SIZE / 4.0);
                double geological_noise = PerlinNoise2D(config->noise_layer, norm_x, norm_y, 2.0, 2.0, 8);
                float geological_factor = (geological_noise + 1.0f) / 2.0f;
                
                /* Clay and salt form in specific geological + environmental conditions */
                float environmental_factor = (norm_moisture * 0.4f) + ((1.0f - norm_elevation) * 0.6f);
                final_value = (geological_factor * 0.5f) + (environmental_factor * 0.5f);
            }
            break;
            
        default:
            /* Fallback - balanced environmental factors */
            final_value = (norm_moisture * 0.4f) + (norm_temperature * 0.3f) + 
                         ((1.0f - norm_elevation) * 0.3f);
            break;
    }
    
    /* Apply base multiplier and clamp to valid range */
    float normalized = final_value * config->base_multiplier;
    
    /* Use manual limit to avoid MIN/MAX macro issues */
    if (normalized < 0.0) return 0.0;
    if (normalized > 1.0) return 1.0;
    return normalized;
}

/* Apply harvest history and regeneration */
float apply_harvest_regeneration(int resource_type, float base_value, struct resource_node *node) {
    if (!node || resource_type < 0 || resource_type >= NUM_RESOURCE_TYPES) {
        return base_value;
    }
    
    struct resource_config *config = &resource_configs[resource_type];
    
    float consumed = node->consumed_amount[resource_type];
    if (consumed <= 0.0) return base_value;
    
    /* Calculate time-based regeneration */
    time_t now = time(NULL);
    time_t last_harvest = node->last_harvest[resource_type];
    float hours_passed = (now - last_harvest) / 3600.0;
    
    /* Regenerate based on time and config */
    float regenerated = consumed * config->regen_rate_per_hour * hours_passed;
    consumed -= regenerated;
    
    /* Update the node (lazy update) */
    if (consumed <= 0.0) {
        node->consumed_amount[resource_type] = 0.0;
        return base_value;
    } else {
        node->consumed_amount[resource_type] = consumed;
        return base_value * (1.0 - consumed);
    }
}

/* Apply environmental modifiers (seasonal and weather) */
float apply_environmental_modifiers(int resource_type, int x, int y, float base_value) {
    if (resource_type < 0 || resource_type >= NUM_RESOURCE_TYPES) {
        return base_value;
    }
    
    struct resource_config *config = &resource_configs[resource_type];
    float modifier = 1.0;
    
    /* Seasonal effects */
    if (config->seasonal_affected) {
        modifier *= get_seasonal_modifier(resource_type);
    }
    
    /* Weather effects */
    if (config->weather_affected) {
        int weather = get_weather(x, y);
        modifier *= get_weather_modifier(resource_type, weather);
    }
    
    return base_value * modifier;
}

/* Region resource modifier function for individual resource calculations */
float apply_region_resource_modifiers(int resource_type, int x, int y, float base_value)
{
  /* For now, return base value unchanged until region integration is properly implemented */
  /* TODO: Implement region lookup and database query for region effects */
  return base_value;
}

/* Placeholder for region resource modifiers - will be implemented in Phase 2 */
void apply_region_resource_modifiers_to_node(struct char_data *ch, struct resource_node *resources)
{
  /* TODO: Implement when character-based region detection is available */
  /* For now, this function does nothing but maintains the interface */
  return;
}

// Helper function to parse and apply JSON resource modifiers
void apply_json_resource_modifiers(struct resource_node *resources, const char *json_data, double intensity)
{
  /* TODO: Implement when resource_node structure is properly defined */
  /* For now, this function does nothing but maintains the interface */
  return;
}

/* ===== ENVIRONMENTAL MODIFIER FUNCTIONS ===== */

/* Get seasonal modifier for resource type */
float get_seasonal_modifier(int resource_type) {
    switch (resource_type) {
        case RESOURCE_VEGETATION:
        case RESOURCE_HERBS:
            switch (time_info.month) {
                case 0: case 1: case 2: return 0.3;  /* Winter - very low */
                case 3: case 4: case 5: return 1.8;  /* Spring - high growth */
                case 6: case 7: case 8: return 1.2;  /* Summer - good */
                case 9: case 10: case 11: return 0.7; /* Autumn - declining */
            }
            break;
        case RESOURCE_GAME:
            /* Animals migrate/hibernate */
            switch (time_info.month) {
                case 0: case 1: case 2: return 0.5;  /* Winter - scarce */
                case 3: case 4: case 5: return 1.3;  /* Spring - breeding */
                case 6: case 7: case 8: return 1.0;  /* Summer - normal */
                case 9: case 10: case 11: return 1.1; /* Autumn - fattening */
            }
            break;
        case RESOURCE_WOOD:
            /* Trees grow slower in winter */
            switch (time_info.month) {
                case 0: case 1: case 2: return 0.8;  /* Winter - slow growth */
                case 3: case 4: case 5: return 1.2;  /* Spring - active growth */
                default: return 1.0;
            }
            break;
    }
    return 1.0;
}

/* Get weather modifier for resource type */
float get_weather_modifier(int resource_type, int weather) {
    switch (resource_type) {
        case RESOURCE_WATER:
            if (weather > 225) return 2.0;  /* Thunderstorm - lots of water */
            if (weather > 200) return 1.5;  /* Heavy rain */
            if (weather >= 178) return 1.2; /* Light rain */
            return 0.8;  /* Clear weather - less surface water */
            
        case RESOURCE_VEGETATION:
        case RESOURCE_HERBS:
            if (weather > 200) return 1.1;  /* Rain helps plants */
            if (weather >= 178) return 1.05;
            return 1.0;
            
        case RESOURCE_CLAY:
            /* Clay easier to find when wet */
            if (weather >= 178) return 1.3;
            return 1.0;
    }
    return 1.0;
}

/* ===== RESOURCE NODE MANAGEMENT ===== */

/* Simple KD-tree functions - placeholder implementations */
/* TODO: Implement proper KD-tree in later phase if needed */

struct resource_node *kdtree_find_resource_node(int x, int y) {
    /* Linear search for now - will optimize later */
    struct resource_node *current = resource_kd_tree;
    while (current) {
        if (current->x == x && current->y == y) {
            return current;
        }
        /* Simple linear traversal for now */
        current = current->left;
    }
    return NULL;
}

void kdtree_insert_resource_node(struct resource_node *node) {
    if (!node) return;
    
    /* Simple insertion at head for now */
    node->left = resource_kd_tree;
    node->right = NULL;
    resource_kd_tree = node;
}

void kdtree_remove_resource_node(int x, int y) {
    /* Simple removal implementation */
    struct resource_node *current = resource_kd_tree;
    struct resource_node *prev = NULL;
    
    while (current) {
        if (current->x == x && current->y == y) {
            if (prev) {
                prev->left = current->left;
            } else {
                resource_kd_tree = current->left;
            }
            free(current);
            return;
        }
        prev = current;
        current = current->left;
    }
}

struct resource_node *find_or_create_resource_node(int x, int y) {
    struct resource_node *node = kdtree_find_resource_node(x, y);
    if (!node) {
        int i;
        CREATE(node, struct resource_node, 1);
        node->x = x;
        node->y = y;
        /* Initialize all arrays to 0 */
        for (i = 0; i < NUM_RESOURCE_TYPES; i++) {
            node->consumed_amount[i] = 0.0;
            node->last_harvest[i] = 0;
            node->harvest_count[i] = 0;
        }
        kdtree_insert_resource_node(node);
    }
    return node;
}

void cleanup_old_resource_nodes(void) {
    /* Remove nodes that haven't been accessed in a week and are fully regenerated */
    time_t now = time(NULL);
    /* time_t week_ago = now - (7 * 24 * 3600); */ /* TODO: Use when implementing cleanup */
    
    /* TODO: Implement cleanup logic */
    /* For now, just log that cleanup was called */
    log("INFO: Resource node cleanup called at %ld", (long)now);
}

/* ===== RESOURCE QUALITY AND DESCRIPTION FUNCTIONS ===== */

int determine_resource_quality(int resource_type, int x, int y, float level) {
    if (resource_type < 0 || resource_type >= NUM_RESOURCE_TYPES || level <= 0.0) {
        return RESOURCE_QUALITY_POOR;
    }
    
    struct resource_config *config = &resource_configs[resource_type];
    
    /* Higher levels = better quality chance */
    int roll = dice(1, 100);
    int quality_threshold = (int)(level * 100);
    
    /* Apply quality variance */
    quality_threshold += dice(1, config->quality_variance) - (config->quality_variance / 2);
    
    if (roll <= quality_threshold / 5) return RESOURCE_QUALITY_LEGENDARY;
    if (roll <= quality_threshold / 3) return RESOURCE_QUALITY_RARE;
    if (roll <= quality_threshold / 2) return RESOURCE_QUALITY_UNCOMMON;
    if (roll <= quality_threshold) return RESOURCE_QUALITY_COMMON;
    return RESOURCE_QUALITY_POOR;
}

const char *get_abundance_description(float level) {
    int i;
    for (i = 0; i < sizeof(abundance_levels) / sizeof(struct resource_abundance); i++) {
        if (level >= abundance_levels[i].min_level) {
            return abundance_levels[i].description;
        }
    }
    return "depleted";
}

const char *get_quality_description(int resource_type, int x, int y, float level) {
    int quality = determine_resource_quality(resource_type, x, y, level);
    
    switch (quality) {
        case RESOURCE_QUALITY_LEGENDARY: return "legendary";
        case RESOURCE_QUALITY_RARE: return "rare";
        case RESOURCE_QUALITY_UNCOMMON: return "uncommon";
        case RESOURCE_QUALITY_COMMON: return "common";
        case RESOURCE_QUALITY_POOR: 
        default: return "poor";
    }
}

const char *get_resource_name(int resource_type) {
    if (resource_type < 0 || resource_type >= NUM_RESOURCE_TYPES) {
        return "unknown";
    }
    return resource_names[resource_type];
}

int parse_resource_type(const char *arg) {
    int i;
    if (!arg || !*arg) return -1;
    
    for (i = 0; i < NUM_RESOURCE_TYPES; i++) {
        if (!str_cmp(arg, resource_names[i])) {
            return i;
        }
    }
    return -1;
}

/* ===== RESOURCE MAPPING AND DISPLAY ===== */

char get_resource_map_symbol_with_coords(float level, int x, int y) {
    /* Use coordinate-based micro-variation to soften boundaries */
    /* This creates more natural transitions without randomness */
    float micro_noise = ((x * 7 + y * 13) % 100) / 2000.0;  /* ±0.025 variation */
    float adjusted_level = level + micro_noise;
    
    /* Clamp to valid range */
    if (adjusted_level < 0.0) adjusted_level = 0.0;
    if (adjusted_level > 1.0) adjusted_level = 1.0;
    
    /* Use softer, overlapping thresholds for natural boundaries */
    if (adjusted_level >= 0.75) return '#';      /* Very rich */
    if (adjusted_level >= 0.55) return '*';      /* Rich */  
    if (adjusted_level >= 0.35) return '+';      /* Moderate */
    if (adjusted_level >= 0.15) return '.';      /* Poor */
    if (adjusted_level >= 0.03) return ',';      /* Trace */
    return ' ';                                  /* None */
}

char get_resource_map_symbol(float level) {
    /* Fallback for calls without coordinates */
    if (level >= 0.8) return '#';      /* Very rich */
    if (level >= 0.6) return '*';      /* Rich */  
    if (level >= 0.4) return '+';      /* Moderate */
    if (level >= 0.2) return '.';      /* Poor */
    if (level >= 0.05) return ',';     /* Trace */
    return ' ';                        /* None */
}

const char *get_resource_color(float level) {
    if (level >= 0.8) return "\tG";      /* Green - Very High */
    if (level >= 0.6) return "\tY";      /* Yellow - High */
    if (level >= 0.4) return "\ty";      /* Light yellow - Moderate */
    if (level >= 0.2) return "\tR";      /* Red - Low */
    if (level >= 0.05) return "\tr";     /* Dark red - Traces */
    return "\tL";                        /* Black/dark - None */
}

/* ===== RESOURCE CACHING SYSTEM ===== */

/* Helper function to calculate cache key coordinates (snap to grid) */
static void get_cache_coordinates(int x, int y, int *cache_x, int *cache_y) {
    *cache_x = (x / RESOURCE_CACHE_GRID_SIZE) * RESOURCE_CACHE_GRID_SIZE;
    *cache_y = (y / RESOURCE_CACHE_GRID_SIZE) * RESOURCE_CACHE_GRID_SIZE;
}

/* Insert a cache node into the KD-tree */
static struct resource_cache_node *cache_insert_node(struct resource_cache_node *node, 
                                                     struct resource_cache_node *new_node, 
                                                     int depth) {
    if (node == NULL) return new_node;
    
    /* Alternate between x and y coordinates for splitting */
    if (depth % 2 == 0) {
        if (new_node->x < node->x)
            node->left = cache_insert_node(node->left, new_node, depth + 1);
        else
            node->right = cache_insert_node(node->right, new_node, depth + 1);
    } else {
        if (new_node->y < node->y)
            node->left = cache_insert_node(node->left, new_node, depth + 1);
        else
            node->right = cache_insert_node(node->right, new_node, depth + 1);
    }
    
    return node;
}

/* Search for a cache node in the KD-tree */
static struct resource_cache_node *cache_search_node(struct resource_cache_node *node, 
                                                     int x, int y, int depth) {
    if (node == NULL) return NULL;
    
    if (node->x == x && node->y == y) {
        /* Check if cache is still valid */
        time_t now = time(NULL);
        if (now - node->cache_time <= RESOURCE_CACHE_LIFETIME) {
            return node;
        } else {
            /* Cache expired */
            return NULL;
        }
    }
    
    /* Alternate between x and y coordinates for searching */
    if (depth % 2 == 0) {
        if (x < node->x)
            return cache_search_node(node->left, x, y, depth + 1);
        else
            return cache_search_node(node->right, x, y, depth + 1);
    } else {
        if (y < node->y)
            return cache_search_node(node->left, x, y, depth + 1);
        else
            return cache_search_node(node->right, x, y, depth + 1);
    }
}

/* Find cached resource values for coordinates */
struct resource_cache_node *cache_find_resource_values(int x, int y) {
    int cache_x, cache_y;
    get_cache_coordinates(x, y, &cache_x, &cache_y);
    return cache_search_node(resource_cache_tree, cache_x, cache_y, 0);
}

/* Store resource values in cache */
void cache_store_resource_values(int x, int y, float values[NUM_RESOURCE_TYPES]) {
    int cache_x, cache_y;
    int i;
    struct resource_cache_node *existing_node;
    struct resource_cache_node *new_node;
    
    get_cache_coordinates(x, y, &cache_x, &cache_y);
    
    /* Check if we already have a cache entry for this grid position */
    existing_node = cache_search_node(resource_cache_tree, cache_x, cache_y, 0);
    if (existing_node != NULL) {
        /* Update existing cache */
        for (i = 0; i < NUM_RESOURCE_TYPES; i++) {
            existing_node->cached_values[i] = values[i];
        }
        existing_node->cache_time = time(NULL);
        return;
    }
    
    /* Create new cache node */
    CREATE(new_node, struct resource_cache_node, 1);
    new_node->x = cache_x;
    new_node->y = cache_y;
    new_node->cache_time = time(NULL);
    new_node->left = NULL;
    new_node->right = NULL;
    
    for (i = 0; i < NUM_RESOURCE_TYPES; i++) {
        new_node->cached_values[i] = values[i];
    }
    
    /* Insert into tree */
    resource_cache_tree = cache_insert_node(resource_cache_tree, new_node, 0);
    resource_cache_count++;
    
    /* Clean up if we have too many cached nodes */
    if (resource_cache_count > RESOURCE_CACHE_MAX_NODES) {
        cache_cleanup_expired();
    }
}

/* Clean up expired cache nodes */
static void cache_cleanup_recursive(struct resource_cache_node **node, time_t now) {
    if (*node == NULL) return;
    
    cache_cleanup_recursive(&((*node)->left), now);
    cache_cleanup_recursive(&((*node)->right), now);
    
    if (now - (*node)->cache_time > RESOURCE_CACHE_LIFETIME) {
        struct resource_cache_node *to_free = *node;
        /* This is a simplified cleanup - in a full implementation,
         * we'd need to properly rebalance the tree */
        *node = NULL;
        free(to_free);
        resource_cache_count--;
    }
}

void cache_cleanup_expired(void) {
    time_t now = time(NULL);
    cache_cleanup_recursive(&resource_cache_tree, now);
}

/* Clear all cache nodes */
static void cache_clear_recursive(struct resource_cache_node *node) {
    if (node == NULL) return;
    
    cache_clear_recursive(node->left);
    cache_clear_recursive(node->right);
    free(node);
}

void cache_clear_all(void) {
    cache_clear_recursive(resource_cache_tree);
    resource_cache_tree = NULL;
    resource_cache_count = 0;
}

/* Get cache statistics */
static void cache_count_recursive(struct resource_cache_node *node, time_t now, 
                                 int *total, int *expired) {
    if (node == NULL) return;
    
    (*total)++;
    if (now - node->cache_time > RESOURCE_CACHE_LIFETIME) {
        (*expired)++;
    }
    
    cache_count_recursive(node->left, now, total, expired);
    cache_count_recursive(node->right, now, total, expired);
}

int cache_get_stats(int *total_nodes, int *expired_nodes) {
    time_t now = time(NULL);
    *total_nodes = 0;
    *expired_nodes = 0;
    
    cache_count_recursive(resource_cache_tree, now, total_nodes, expired_nodes);
    return *total_nodes;
}

/* ===== INITIALIZATION AND CLEANUP ===== */

void init_resource_system(void) {
    log("Info: Initializing wilderness resource system");
    
    /* Initialize the resource KD-tree */
    resource_kd_tree = NULL;
    
    /* Initialize the resource cache */
    resource_cache_tree = NULL;
    resource_cache_count = 0;
    
    /* Initialize Perlin noise for resource layers */
    /* This is handled by the existing Perlin system */
    
    log("Info: Resource system initialized with %d resource types and spatial caching", NUM_RESOURCE_TYPES);
}

void shutdown_resource_system(void) {
    log("INFO: Shutting down wilderness resource system");
    
    /* Clean up all resource nodes */
    while (resource_kd_tree) {
        struct resource_node *temp = resource_kd_tree;
        resource_kd_tree = resource_kd_tree->left;
        free(temp);
    }
    
    /* Clean up cache */
    cache_clear_all();
    
    log("INFO: Resource system shutdown complete");
}

/* ===== SURVEY HELPER FUNCTIONS ===== */

void show_resource_survey(struct char_data *ch) {
    int x, y, i;
    float resource_level;
    zone_rnum zrnum;
    
    if (!ch || IN_ROOM(ch) == NOWHERE) {
        return;
    }
    
    zrnum = world[IN_ROOM(ch)].zone;
    if (!ZONE_FLAGGED(zrnum, ZONE_WILDERNESS)) {
        send_to_char(ch, "Resource surveying can only be performed in the wilderness.\r\n");
        return;
    }
    
    x = world[IN_ROOM(ch)].coords[0];
    y = world[IN_ROOM(ch)].coords[1];
    
    send_to_char(ch, "Resource Survey for (\tC%d\tn, \tC%d\tn):\r\n", x, y);
    send_to_char(ch, "===================================\r\n\r\n");
    
    send_to_char(ch, "Terrain: %s | Elevation: %d\r\n\r\n", 
                 sector_types[SECT(IN_ROOM(ch))], get_modified_elevation(x, y));
    
    /* Show all resources with meaningful levels */
    send_to_char(ch, "Available Resources:\r\n");
    for (i = 0; i < NUM_RESOURCE_TYPES; i++) {
        resource_level = calculate_current_resource_level(i, x, y);
        if (resource_level > 0.05) { /* Only show resources with meaningful levels */
            send_to_char(ch, "  \tG%-12s\tn: %s\r\n", 
                         resource_names[i], 
                         get_abundance_description(resource_level));
        }
    }
}

void show_terrain_survey(struct char_data *ch) {
    int x, y, elevation;
    int terrain_type;
    zone_rnum zrnum;
    
    if (!ch || IN_ROOM(ch) == NOWHERE) {
        return;
    }
    
    zrnum = world[IN_ROOM(ch)].zone;
    if (!ZONE_FLAGGED(zrnum, ZONE_WILDERNESS)) {
        send_to_char(ch, "Terrain analysis can only be performed in the wilderness.\r\n");
        return;
    }
    
    x = world[IN_ROOM(ch)].coords[0];
    y = world[IN_ROOM(ch)].coords[1];
    elevation = get_modified_elevation(x, y);
    terrain_type = SECT(IN_ROOM(ch));
    
    send_to_char(ch, "Detailed Terrain Analysis for (\tC%d\tn, \tC%d\tn):\r\n", x, y);
    send_to_char(ch, "==============================================\r\n\r\n");
    
    send_to_char(ch, "Primary Terrain: %s\r\n", sector_types[terrain_type]);
    send_to_char(ch, "Elevation: %d meters\r\n", elevation);
    
    /* Terrain-specific environmental factors */
    send_to_char(ch, "\r\nTerrain Characteristics:\r\n");
    switch (terrain_type) {
        case SECT_FOREST:
            send_to_char(ch, "  Dense woodland with tall trees providing natural cover.\r\n");
            send_to_char(ch, "  Favorable for: vegetation, herbs, lumber, wildlife\r\n");
            break;
        case SECT_FIELD:
            send_to_char(ch, "  Open grassland with fertile soil.\r\n");
            send_to_char(ch, "  Favorable for: vegetation, herbs, wildlife\r\n");
            break;
        case SECT_HILLS:
            send_to_char(ch, "  Rolling hills with varied mineral deposits.\r\n");
            send_to_char(ch, "  Favorable for: minerals, stone, some vegetation\r\n");
            break;
        case SECT_MOUNTAIN:
            send_to_char(ch, "  Rocky mountain terrain rich in minerals.\r\n");
            send_to_char(ch, "  Favorable for: minerals, stone, crystals\r\n");
            break;
        case SECT_WATER_SWIM:
        case SECT_WATER_NOSWIM:
            send_to_char(ch, "  Aquatic environment with flowing water.\r\n");
            send_to_char(ch, "  Favorable for: water resources, clay, salt\r\n");
            break;
        case SECT_DESERT:
            send_to_char(ch, "  Arid desert environment with extremely scarce water resources.\r\n");
            send_to_char(ch, "  Water is almost never found except in rare oases or ephemeral springs.\r\n");
            send_to_char(ch, "  Favorable for: minerals, crystals (very sparse vegetation)\r\n");
            break;
        default:
            send_to_char(ch, "  Standard terrain with mixed characteristics.\r\n");
            break;
    }
    
    /* Elevation effects */
    send_to_char(ch, "\r\nElevation Effects:\r\n");
    if (elevation > 100) {
        send_to_char(ch, "  High elevation - increased mineral exposure, reduced vegetation\r\n");
    } else if (elevation < -50) {
        send_to_char(ch, "  Low elevation - increased water access, rich sediment deposits\r\n");
    } else {
        send_to_char(ch, "  Moderate elevation - balanced resource conditions\r\n");
    }
}

void show_debug_survey(struct char_data *ch) {
    int x, y, i;
    float resource_level, base_value, modified_value;
    zone_rnum zrnum;
    
    if (!ch || IN_ROOM(ch) == NOWHERE) {
        return;
    }
    
    zrnum = world[IN_ROOM(ch)].zone;
    if (!ZONE_FLAGGED(zrnum, ZONE_WILDERNESS)) {
        send_to_char(ch, "Debug survey can only be performed in the wilderness.\r\n");
        return;
    }
    
    x = world[IN_ROOM(ch)].coords[0];
    y = world[IN_ROOM(ch)].coords[1];
    
    send_to_char(ch, "DEBUG Resource System Analysis for (\tC%d\tn, \tC%d\tn):\r\n", x, y);
    send_to_char(ch, "===================================================\r\n\r\n");
    
    send_to_char(ch, "Terrain: %s (Type: %d)\r\n", 
                 sector_types[SECT(IN_ROOM(ch))], SECT(IN_ROOM(ch)));
    send_to_char(ch, "Elevation: %d\r\n", get_modified_elevation(x, y));
    
    send_to_char(ch, "\r\nResource Details:\r\n");
    send_to_char(ch, "%-12s | %-8s | %-8s | %-8s | %-5s | %s\r\n",
                 "Resource", "Base", "Modified", "Final", "Layer", "Description");
    send_to_char(ch, "-------------|----------|----------|----------|-------|------------------\r\n");
    
    for (i = 0; i < NUM_RESOURCE_TYPES; i++) {
        base_value = get_base_resource_value(i, x, y);
        modified_value = apply_region_resource_modifiers(i, x, y, base_value);
        resource_level = apply_environmental_modifiers(i, x, y, modified_value);
        
        send_to_char(ch, "%-12s | %8.3f | %8.3f | %8.3f | %5d | %s\r\n",
                     resource_names[i],
                     base_value,
                     modified_value, 
                     resource_level,
                     resource_configs[i].noise_layer,
                     resource_configs[i].description);
    }
    
    /* Region effects analysis */
    {
        struct region_list *regions = NULL;
        struct region_list *curr_region = NULL;
        zone_rnum zone = real_zone(WILD_ZONE_VNUM);
        
        if (zone != NOWHERE) {
            regions = get_enclosing_regions(zone, x, y);
            if (regions) {
                send_to_char(ch, "\r\nRegion Effects:\r\n");
                
                for (curr_region = regions; curr_region != NULL; curr_region = curr_region->next) {
                    if (curr_region->rnum != NOWHERE && 
                        curr_region->rnum >= 0 && 
                        curr_region->rnum <= top_of_region_table) {
                        
                        region_vnum vnum = region_table[curr_region->rnum].vnum;
                        char *name = region_table[curr_region->rnum].name;
                        
                        send_to_char(ch, "  Region: %s (vnum %d)\r\n", 
                                     name ? name : "Unknown", vnum);
                        send_to_char(ch, "    Effects: (New effects system - use 'resourceadmin effects region %d')\r\n", vnum);
                    }
                }
                
                free_region_list(regions);
            } else {
                send_to_char(ch, "\r\nRegion Effects: None (no regions at this location)\r\n");
            }
        }
    }
    
    send_to_char(ch, "\r\nPerlin Noise Configuration:\r\n");
    for (i = 0; i < NUM_RESOURCE_TYPES; i++) {
        send_to_char(ch, "  %s: Layer %d, Multiplier %.3f, Regen %.3f\r\n",
                     resource_names[i],
                     resource_configs[i].noise_layer,
                     resource_configs[i].base_multiplier,
                     resource_configs[i].regen_rate_per_hour);
    }
    
    /* Cache statistics */
    {
        int total_nodes, expired_nodes, cache_x, cache_y;
        struct resource_cache_node *cached;
        
        cache_get_stats(&total_nodes, &expired_nodes);
        get_cache_coordinates(x, y, &cache_x, &cache_y);
        cached = cache_find_resource_values(x, y);
        
        send_to_char(ch, "\r\nSpatial Cache Statistics:\r\n");
        send_to_char(ch, "  Total cached nodes: %d\r\n", total_nodes);
        send_to_char(ch, "  Expired nodes: %d\r\n", expired_nodes);
        send_to_char(ch, "  Cache grid size: %d coordinates\r\n", RESOURCE_CACHE_GRID_SIZE);
        send_to_char(ch, "  Cache lifetime: %d seconds\r\n", RESOURCE_CACHE_LIFETIME);
        send_to_char(ch, "  Current position cache grid: (%d, %d)\r\n", cache_x, cache_y);
        send_to_char(ch, "  Cache status: %s\r\n", cached ? "HIT" : "MISS");
    }
}

void show_resource_map(struct char_data *ch, int resource_type, int radius) {
    int center_x, center_y, x, y, map_x, map_y;
    float resource_level;
    char symbol;
    const char *color;
    zone_rnum zrnum;
    
    if (!ch || IN_ROOM(ch) == NOWHERE) {
        return;
    }
    
    zrnum = world[IN_ROOM(ch)].zone;
    if (!ZONE_FLAGGED(zrnum, ZONE_WILDERNESS)) {
        send_to_char(ch, "Resource maps can only be viewed in the wilderness.\r\n");
        return;
    }
    
    if (resource_type < 0 || resource_type >= NUM_RESOURCE_TYPES) {
        send_to_char(ch, "Invalid resource type.\r\n");
        return;
    }
    
    if (radius < 3 || radius > 15) {
        radius = 7; /* Default radius */
    }
    
    center_x = world[IN_ROOM(ch)].coords[0];
    center_y = world[IN_ROOM(ch)].coords[1];
    
    send_to_char(ch, "Resource Map - %s (Radius: %d)\r\n", 
                 resource_names[resource_type], radius);
    send_to_char(ch, "Legend: \tG# \tyVery High  \tY* \tyHigh  \ty+ \tyMod  \tR. \tyLow  \tr, \tyTraces\tn\r\n");
    send_to_char(ch, "========================================\r\n");
    
    /* Draw the map */
    for (map_y = -radius; map_y <= radius; map_y++) {
        char line_buffer[MAX_STRING_LENGTH] = "";
        strcat(line_buffer, " "); /* Leading space for each line */
        
        for (map_x = -radius; map_x <= radius; map_x++) {
            x = center_x + map_x;
            y = center_y + map_y;
            
            /* Check if this is the player's position */
            if (map_x == 0 && map_y == 0) {
                strcat(line_buffer, "\tW@\tn"); /* Player position */
            } else {
                float base_level = calculate_current_resource_level(resource_type, x, y);
                /* Get depletion level directly by coordinates - much more accurate! */
                zone_rnum zrnum = world[IN_ROOM(ch)].zone;
                int zone_vnum = zone_table[zrnum].number;
                float depletion_level = get_resource_depletion_level_by_coords(x, y, zone_vnum, resource_type);
                resource_level = base_level * depletion_level;
                
                symbol = get_resource_map_symbol_with_coords(resource_level, x, y);
                color = get_resource_color(resource_level);
                char symbol_str[20];
                snprintf(symbol_str, sizeof(symbol_str), "%s%c\tn", color, symbol);
                strcat(line_buffer, symbol_str);
            }
        }
        strcat(line_buffer, "\r\n");
        send_to_char(ch, "%s", line_buffer); /* Send entire line at once */
    }
    
    send_to_char(ch, "========================================\r\n");
    send_to_char(ch, "Current location (\tW@\tn): (%d, %d)\r\n", center_x, center_y);
    
    /* Show resource level at current location - with depletion adjustment */
    float base_level = calculate_current_resource_level(resource_type, center_x, center_y);
    float depletion_level = get_resource_depletion_level(IN_ROOM(ch), resource_type);
    resource_level = base_level * depletion_level;
    
    send_to_char(ch, "Current %s level: %s (%.1f%% effective, %.1f%% base)\r\n", 
                 resource_names[resource_type],
                 get_abundance_description(resource_level),
                 resource_level * 100.0f,
                 base_level * 100.0f);
}

void show_resource_detail(struct char_data *ch, int resource_type) {
    int x, y;
    float resource_level;
    zone_rnum zrnum;
    
    if (!ch || IN_ROOM(ch) == NOWHERE) {
        return;
    }
    
    zrnum = world[IN_ROOM(ch)].zone;
    if (!ZONE_FLAGGED(zrnum, ZONE_WILDERNESS)) {
        send_to_char(ch, "Resource details can only be viewed in the wilderness.\r\n");
        return;
    }
    
    if (resource_type < 0 || resource_type >= NUM_RESOURCE_TYPES) {
        send_to_char(ch, "Invalid resource type.\r\n");
        return;
    }
    
    x = world[IN_ROOM(ch)].coords[0];
    y = world[IN_ROOM(ch)].coords[1];
    
    send_to_char(ch, "Detailed Resource Analysis: %s\r\n", resource_names[resource_type]);
    send_to_char(ch, "==========================================\r\n");
    send_to_char(ch, "Location: (%d, %d)\r\n", x, y);
    send_to_char(ch, "Terrain: %s\r\n", sector_types[SECT(IN_ROOM(ch))]);
    send_to_char(ch, "Elevation: %d\r\n", get_modified_elevation(x, y));
    
    resource_level = calculate_current_resource_level(resource_type, x, y);
    send_to_char(ch, "\r\nResource Level: %s (%.2f%%)\r\n", 
                 get_abundance_description(resource_level),
                 resource_level * 100.0f);
    
    send_to_char(ch, "Description: %s\r\n", resource_configs[resource_type].description);
    send_to_char(ch, "Harvest Skill: %s\r\n", 
                 resource_configs[resource_type].harvest_skill == SKILL_FORESTING ? "Foresting" : 
                 resource_configs[resource_type].harvest_skill == SKILL_MINING ? "Mining" : "Unknown");
    
    /* Environmental factors */
    send_to_char(ch, "\r\nEnvironmental Factors:\r\n");
    send_to_char(ch, "- Perlin Noise Layer: %d\r\n", resource_configs[resource_type].noise_layer);
    send_to_char(ch, "- Base Multiplier: %.3f\r\n", resource_configs[resource_type].base_multiplier);
    send_to_char(ch, "- Seasonal Effects: %s\r\n", 
                 resource_configs[resource_type].seasonal_affected ? "Yes" : "No");
    send_to_char(ch, "- Weather Effects: %s\r\n", 
                 resource_configs[resource_type].weather_affected ? "Yes" : "No");
}

/* ===== PHASE 4.5: MATERIAL SUBTYPE MANAGEMENT SYSTEM ===== */

/* Material subtype name arrays */
const char *herb_subtype_names[NUM_HERB_SUBTYPES] = {
    "marjoram", "kingfoil", "starlily", "wolfsbane", 
    "silverleaf", "moonbell", "thornweed", "brightroot"
};

const char *crystal_subtype_names[NUM_CRYSTAL_SUBTYPES] = {
    "arcanite", "nethermote", "sunstone", "voidshards",
    "dreamquartz", "bloodstone", "frostgem", "stormcrystal"
};

const char *ore_subtype_names[NUM_ORE_SUBTYPES] = {
    "mithril", "adamantine", "cold iron", "star steel",
    "deep silver", "dragonsteel", "voidmetal", "brightcopper"
};

const char *wood_subtype_names[NUM_WOOD_SUBTYPES] = {
    "ironwood", "silverbirch", "shadowbark", "brightoak",
    "thornvine", "moonweave", "darkwillow", "starwood"
};

const char *vegetation_subtype_names[NUM_VEGETATION_SUBTYPES] = {
    "cotton", "silk moss", "hemp vine", "spirit grass",
    "flame flower", "ice lichen", "shadow fern", "light bloom"
};

const char *stone_subtype_names[NUM_STONE_SUBTYPES] = {
    "granite", "marble", "obsidian", "moonstone",
    "dragonbone", "voidrock", "starstone", "shadowslate"
};

const char *game_subtype_names[NUM_GAME_SUBTYPES] = {
    "leather", "dragonhide", "shadowpelt", "brightfur",
    "winterwool", "spirit silk", "voidhide", "starfur"
};

const char *water_subtype_names[NUM_WATER_SUBTYPES] = {
    "spring water", "mineral water", "pure water", "blessed water", "enchanted water"
};

const char *clay_subtype_names[NUM_CLAY_SUBTYPES] = {
    "common clay", "fire clay", "porcelain clay", "magic clay", "crystal clay"
};

const char *salt_subtype_names[NUM_SALT_SUBTYPES] = {
    "common salt", "sea salt", "rock salt", "alchemical salt", "preserving salt"
};

const char *quality_names[6] = {
    "unknown", "poor", "common", "uncommon", "rare", "legendary"
};

/* Material storage functions */
int add_material_to_storage(struct char_data *ch, int category, int subtype, int quality, int quantity) {
    int i;
    
    if (!ch || IS_NPC(ch)) {
        return 0;
    }
    
    /* Validate material data */
    if (!validate_material_data(category, subtype, quality) || quantity <= 0) {
        return 0;
    }
    
    /* Find existing entry */
    for (i = 0; i < ch->player_specials->saved.stored_material_count; i++) {
        if (ch->player_specials->saved.stored_materials[i].category == category &&
            ch->player_specials->saved.stored_materials[i].subtype == subtype &&
            ch->player_specials->saved.stored_materials[i].quality == quality) {
            
            ch->player_specials->saved.stored_materials[i].quantity += quantity;
            
            /* Note: Integration notification happens at harvest time, not storage time */
            return quantity;
        }
    }
    
    /* Add new entry if we have space */
    if (ch->player_specials->saved.stored_material_count < MAX_STORED_MATERIALS) {
        i = ch->player_specials->saved.stored_material_count;
        ch->player_specials->saved.stored_materials[i].category = category;
        ch->player_specials->saved.stored_materials[i].subtype = subtype;
        ch->player_specials->saved.stored_materials[i].quality = quality;
        ch->player_specials->saved.stored_materials[i].quantity = quantity;
        ch->player_specials->saved.stored_material_count++;
        
        /* Note: Integration notification happens at harvest time, not storage time */
        return quantity;
    }
    
    /* Storage full */
    return 0;
}

int remove_material_from_storage(struct char_data *ch, int category, int subtype, int quality, int quantity) {
    int i, removed = 0;
    
    if (!ch || IS_NPC(ch)) {
        return 0;
    }
    
    /* Find entry */
    for (i = 0; i < ch->player_specials->saved.stored_material_count; i++) {
        if (ch->player_specials->saved.stored_materials[i].category == category &&
            ch->player_specials->saved.stored_materials[i].subtype == subtype &&
            ch->player_specials->saved.stored_materials[i].quality == quality) {
            
            removed = MIN(quantity, ch->player_specials->saved.stored_materials[i].quantity);
            ch->player_specials->saved.stored_materials[i].quantity -= removed;
            
            /* Remove entry if empty */
            if (ch->player_specials->saved.stored_materials[i].quantity <= 0) {
                /* Shift array down */
                int j;
                for (j = i; j < ch->player_specials->saved.stored_material_count - 1; j++) {
                    ch->player_specials->saved.stored_materials[j] = 
                        ch->player_specials->saved.stored_materials[j + 1];
                }
                ch->player_specials->saved.stored_material_count--;
            }
            return removed;
        }
    }
    
    return 0;
}

int get_material_quantity(struct char_data *ch, int category, int subtype, int quality) {
    int i;
    
    if (!ch || IS_NPC(ch)) {
        return 0;
    }
    
    for (i = 0; i < ch->player_specials->saved.stored_material_count; i++) {
        if (ch->player_specials->saved.stored_materials[i].category == category &&
            ch->player_specials->saved.stored_materials[i].subtype == subtype &&
            ch->player_specials->saved.stored_materials[i].quality == quality) {
            
            return ch->player_specials->saved.stored_materials[i].quantity;
        }
    }
    
    return 0;
}

void show_material_storage(struct char_data *ch) {
    if (!ch || IS_NPC(ch)) {
        return;
    }

#ifdef ENABLE_WILDERNESS_CRAFTING_INTEGRATION
    /* Enhanced materials display for LuminariMUD */
    show_enhanced_material_storage(ch);
#else
    /* Basic materials display for other campaigns */
    show_basic_material_storage(ch);
#endif
}

/* Helper functions for enhanced materials display */
const char *get_material_applications(int category) {
    switch (category) {
        case RESOURCE_HERBS: return "Alchemy potions, healing items";
        case RESOURCE_CRYSTAL: return "Magical items, enchanting";
        case RESOURCE_MINERALS: return "Weapons, armor, tools";
        case RESOURCE_WOOD: return "Crafted items, magical staves";
        case RESOURCE_VEGETATION: return "Textiles, rope, magical components";
        case RESOURCE_STONE: return "Building materials, tools";
        case RESOURCE_GAME: return "Leather armor, clothing";
        case RESOURCE_WATER: return "Alchemy, food preparation";
        case RESOURCE_CLAY: return "Pottery, containers, building";
        case RESOURCE_SALT: return "Food preservation, alchemy";
        default: return "Various crafting applications";
    }
}

const char *get_quality_bonus_description(int quality) {
    switch (quality) {
        case 1: return "poor quality (-25%)";
        case 2: return "common quality (+0%)";
        case 3: return "uncommon quality (+50%)";
        case 4: return "rare quality (+200%)";
        case 5: return "legendary quality (+500%)";
        default: return "unknown quality";
    }
}

/* Basic materials display for campaigns without enhanced integration */
void show_basic_material_storage(struct char_data *ch) {
    int i;
    
    if (!ch || IS_NPC(ch)) {
        return;
    }
    
    send_to_char(ch, "\tcYour Wilderness Material Storage:\tn\r\n");
    send_to_char(ch, "=====================================\r\n");
    
    if (ch->player_specials->saved.stored_material_count == 0) {
        send_to_char(ch, "You have no wilderness materials stored.\r\n");
        return;
    }
    
    for (i = 0; i < ch->player_specials->saved.stored_material_count; i++) {
        struct material_storage *mat = &ch->player_specials->saved.stored_materials[i];
        const char *full_name = get_full_material_name(mat->category, mat->subtype, mat->quality);
        
        send_to_char(ch, "%s%3d%s x %s\r\n", 
                     CCYEL(ch, C_NRM), mat->quantity, CCNRM(ch, C_NRM), full_name);
    }
    
    send_to_char(ch, "\r\nStorage: %d/%d slots used\r\n", 
                 ch->player_specials->saved.stored_material_count, MAX_STORED_MATERIALS);
}

/* Material name and description functions */
const char *get_material_subtype_name(int category, int subtype) {
    switch (category) {
        case RESOURCE_HERBS:
            if (subtype >= 0 && subtype < NUM_HERB_SUBTYPES)
                return herb_subtype_names[subtype];
            break;
        case RESOURCE_CRYSTAL:
            if (subtype >= 0 && subtype < NUM_CRYSTAL_SUBTYPES)
                return crystal_subtype_names[subtype];
            break;
        case RESOURCE_MINERALS:
            if (subtype >= 0 && subtype < NUM_ORE_SUBTYPES)
                return ore_subtype_names[subtype];
            break;
        case RESOURCE_WOOD:
            if (subtype >= 0 && subtype < NUM_WOOD_SUBTYPES)
                return wood_subtype_names[subtype];
            break;
        case RESOURCE_VEGETATION:
            if (subtype >= 0 && subtype < NUM_VEGETATION_SUBTYPES)
                return vegetation_subtype_names[subtype];
            break;
        case RESOURCE_STONE:
            if (subtype >= 0 && subtype < NUM_STONE_SUBTYPES)
                return stone_subtype_names[subtype];
            break;
        case RESOURCE_GAME:
            if (subtype >= 0 && subtype < NUM_GAME_SUBTYPES)
                return game_subtype_names[subtype];
            break;
        case RESOURCE_WATER:
            if (subtype >= 0 && subtype < NUM_WATER_SUBTYPES)
                return water_subtype_names[subtype];
            break;
        case RESOURCE_CLAY:
            if (subtype >= 0 && subtype < NUM_CLAY_SUBTYPES)
                return clay_subtype_names[subtype];
            break;
        case RESOURCE_SALT:
            if (subtype >= 0 && subtype < NUM_SALT_SUBTYPES)
                return salt_subtype_names[subtype];
            break;
    }
    return "unknown";
}

const char *get_material_quality_name(int quality) {
    if (quality >= 1 && quality <= 5)
        return quality_names[quality];
    return quality_names[0];
}

const char *get_full_material_name(int category, int subtype, int quality) {
    static char name_buf[256];
    const char *subtype_name = get_material_subtype_name(category, subtype);
    const char *quality_name = get_material_quality_name(quality);
    
    snprintf(name_buf, sizeof(name_buf), "%s %s", quality_name, subtype_name);
    return name_buf;
}

const char *get_material_description(int category, int subtype, int quality) {
    static char desc_buf[512];
    const char *full_name = get_full_material_name(category, subtype, quality);
    
    snprintf(desc_buf, sizeof(desc_buf), "A sample of %s, harvested from the wilderness.", full_name);
    return desc_buf;
}

/* Material validation and utility functions */
int validate_material_data(int category, int subtype, int quality) {
    if (quality < 1 || quality > 5)
        return 0;
        
    if (category < 0 || category >= NUM_RESOURCE_TYPES)
        return 0;
        
    return (subtype >= 0 && subtype < get_max_subtypes_for_category(category));
}

int get_max_subtypes_for_category(int category) {
    switch (category) {
        case RESOURCE_HERBS: return NUM_HERB_SUBTYPES;
        case RESOURCE_CRYSTAL: return NUM_CRYSTAL_SUBTYPES;
        case RESOURCE_MINERALS: return NUM_ORE_SUBTYPES;
        case RESOURCE_WOOD: return NUM_WOOD_SUBTYPES;
        case RESOURCE_VEGETATION: return NUM_VEGETATION_SUBTYPES;
        case RESOURCE_STONE: return NUM_STONE_SUBTYPES;
        case RESOURCE_GAME: return NUM_GAME_SUBTYPES;
        case RESOURCE_WATER: return NUM_WATER_SUBTYPES;
        case RESOURCE_CLAY: return NUM_CLAY_SUBTYPES;
        case RESOURCE_SALT: return NUM_SALT_SUBTYPES;
        default: return 0;
    }
}

bool is_wilderness_only_material(int category, int subtype) {
    /* All Phase 4.5 materials are wilderness-only for now */
    /* TODO: Phase 5 may add some materials that can be found in zones */
    return true;
}

/* Material harvesting functions */
int determine_harvested_material_subtype(int resource_type, int x, int y, float level) {
    /* Use coordinates and level to determine subtype */
    /* Higher quality/rare areas produce better materials */
    int subtype_count = get_max_subtypes_for_category(resource_type);
    if (subtype_count <= 0) return 0;
    
    /* Simple algorithm: use noise to determine subtype */
    /* High quality areas (level > 0.7) have chance for rare materials */
    unsigned int seed = x * 31 + y * 17 + resource_type;
    srand(seed);
    
    if (level > 0.8) {
        /* Legendary level - chance for rarest materials */
        return rand() % subtype_count;
    } else if (level > 0.6) {
        /* High level - middle to high-tier materials */
        return rand() % MAX(1, subtype_count - 2);
    } else {
        /* Lower level - common materials only */
        return rand() % MAX(1, subtype_count / 2);
    }
}

int calculate_material_quality_from_resource(int resource_type, int x, int y, float level) {
    /* Map resource level to quality tiers */
    if (level >= 0.9) return MATERIAL_QUALITY_LEGENDARY;
    if (level >= 0.7) return MATERIAL_QUALITY_RARE;
    if (level >= 0.5) return MATERIAL_QUALITY_UNCOMMON;
    if (level >= 0.3) return MATERIAL_QUALITY_COMMON;
    return MATERIAL_QUALITY_POOR;
}

/* Material storage initialization and maintenance */
void init_material_storage(struct char_data *ch) {
    if (!ch || IS_NPC(ch)) {
        return;
    }
    
    ch->player_specials->saved.stored_material_count = 0;
    memset(ch->player_specials->saved.stored_materials, 0, 
           sizeof(struct material_storage) * MAX_STORED_MATERIALS);
}

void cleanup_material_storage(struct char_data *ch) {
    /* Remove empty entries and consolidate */
    if (!ch || IS_NPC(ch)) {
        return;
    }
    
    compact_material_storage(ch);
}

int compact_material_storage(struct char_data *ch) {
    int read_pos = 0, write_pos = 0, removed = 0;
    
    if (!ch || IS_NPC(ch)) {
        return 0;
    }
    
    /* Compact array by removing empty entries */
    for (read_pos = 0; read_pos < ch->player_specials->saved.stored_material_count; read_pos++) {
        if (ch->player_specials->saved.stored_materials[read_pos].quantity > 0) {
            if (read_pos != write_pos) {
                ch->player_specials->saved.stored_materials[write_pos] = 
                    ch->player_specials->saved.stored_materials[read_pos];
            }
            write_pos++;
        } else {
            removed++;
        }
    }
    
    ch->player_specials->saved.stored_material_count = write_pos;
    
    /* Clear remaining slots */
    {
        int i;
        for (i = write_pos; i < MAX_STORED_MATERIALS; i++) {
            memset(&ch->player_specials->saved.stored_materials[i], 0, sizeof(struct material_storage));
        }
    }
    
    return removed;
}

#ifdef ENABLE_WILDERNESS_CRAFTING_INTEGRATION
/* ======================================================================== */
/* Phase 4.75: Enhanced Wilderness-Crafting Integration (LuminariMUD only) */
/* ======================================================================== */

/* Get enhanced wilderness material ID from category and subtype */
int get_enhanced_wilderness_material_id(int category, int subtype) {
    switch (category) {
        case RESOURCE_HERBS:
            if (subtype >= 0 && subtype < NUM_HERB_SUBTYPES)
                return WILDERNESS_CRAFT_MAT_HERB_BASE + subtype;
            break;
        case RESOURCE_CRYSTAL:
            if (subtype >= 0 && subtype < NUM_CRYSTAL_SUBTYPES)
                return WILDERNESS_CRAFT_MAT_CRYSTAL_BASE + subtype;
            break;
        case RESOURCE_MINERALS:
            if (subtype >= 0 && subtype < NUM_ORE_SUBTYPES)
                return WILDERNESS_CRAFT_MAT_ORE_BASE + subtype;
            break;
        case RESOURCE_WOOD:
            if (subtype >= 0 && subtype < NUM_WOOD_SUBTYPES)
                return WILDERNESS_CRAFT_MAT_WOOD_BASE + subtype;
            break;
        case RESOURCE_VEGETATION:
            if (subtype >= 0 && subtype < NUM_VEGETATION_SUBTYPES)
                return WILDERNESS_CRAFT_MAT_WOOD_BASE + 100 + subtype; /* Offset to avoid conflicts */
            break;
        case RESOURCE_STONE:
            if (subtype >= 0 && subtype < NUM_STONE_SUBTYPES)
                return WILDERNESS_CRAFT_MAT_ORE_BASE + 100 + subtype; /* Stone works like minerals */
            break;
        case RESOURCE_GAME:
            if (subtype >= 0 && subtype < NUM_GAME_SUBTYPES)
                return WILDERNESS_CRAFT_MAT_WOOD_BASE + 200 + subtype; /* Unique offset for leather */
            break;
        case RESOURCE_WATER:
            if (subtype >= 0 && subtype < NUM_WATER_SUBTYPES)
                return WILDERNESS_CRAFT_MAT_HERB_BASE + 200 + subtype; /* Water offset */
            break;
        case RESOURCE_CLAY:
            if (subtype >= 0 && subtype < NUM_CLAY_SUBTYPES)
                return WILDERNESS_CRAFT_MAT_ORE_BASE + 200 + subtype; /* Clay offset */
            break;
        case RESOURCE_SALT:
            if (subtype >= 0 && subtype < NUM_SALT_SUBTYPES)
                return WILDERNESS_CRAFT_MAT_ORE_BASE + 300 + subtype; /* Salt offset */
            break;
    }
    return WILDERNESS_CRAFT_MAT_NONE;
}

/* Check if a material ID is an enhanced wilderness material */
bool is_enhanced_wilderness_material(int material_id) {
    return (material_id >= WILDERNESS_CRAFT_MAT_HERB_BASE && 
            material_id < WILDERNESS_CRAFT_MAT_HERB_BASE + NUM_ENHANCED_WILDERNESS_MATERIALS);
}

/* Get enhanced material name (preserves lore names) */
const char *get_enhanced_material_name(int category, int subtype, int quality) {
    static char enhanced_name_buf[256];
    const char *base_name = get_material_subtype_name(category, subtype);
    const char *quality_name = get_material_quality_name(quality);
    
    /* For LuminariMUD, use descriptive quality + base name format */
    snprintf(enhanced_name_buf, sizeof(enhanced_name_buf), "%s %s", quality_name, base_name);
    return enhanced_name_buf;
}

/* Get crafting value based on quality (affects item power) */
int get_enhanced_material_crafting_value(int category, int subtype, int quality) {
    int base_value = 1;
    
    /* Base value varies by material rarity */
    switch (category) {
        case RESOURCE_HERBS:
            base_value = 5 + subtype; /* Herbs: 5-12 */
            break;
        case RESOURCE_CRYSTAL:
            base_value = 15 + (subtype * 2); /* Crystals: 15-29 */
            break;
        case RESOURCE_MINERALS:
            base_value = 10 + (subtype * 3); /* Ores: 10-31 */
            break;
        case RESOURCE_WOOD:
            base_value = 8 + (subtype * 2); /* Wood: 8-22 */
            break;
        case RESOURCE_VEGETATION:
            base_value = 3 + subtype; /* Vegetation: 3-10 */
            break;
        case RESOURCE_STONE:
            base_value = 12 + (subtype * 2); /* Stone: 12-26 */
            break;
        case RESOURCE_GAME:
            base_value = 7 + (subtype * 2); /* Leather: 7-21 */
            break;
    }
    
    /* Quality multiplier enhances the base value */
    switch (quality) {
        case RESOURCE_QUALITY_POOR:
            return base_value * 1; /* 100% */
        case RESOURCE_QUALITY_COMMON:
            return (base_value * 12) / 10; /* 120% */
        case RESOURCE_QUALITY_UNCOMMON:
            return (base_value * 15) / 10; /* 150% */
        case RESOURCE_QUALITY_RARE:
            return base_value * 2; /* 200% */
        case RESOURCE_QUALITY_LEGENDARY:
            return (base_value * 25) / 10; /* 250% */
        default:
            return base_value;
    }
}

/* Enhanced material description with crafting uses */
const char *get_enhanced_material_description(int category, int subtype, int quality) {
    static char enhanced_desc_buf[512];
    const char *material_name = get_enhanced_material_name(category, subtype, quality);
    const char *crafting_use = "";
    
    /* Determine crafting applications */
    switch (category) {
        case RESOURCE_HERBS:
            crafting_use = "alchemical brewing and enchantment";
            break;
        case RESOURCE_CRYSTAL:
            crafting_use = "magical item enhancement and spell focusing";
            break;
        case RESOURCE_MINERALS:
            crafting_use = "weapon and armor smithing";
            break;
        case RESOURCE_WOOD:
            crafting_use = "weapon hafts, shields, and magical staves";
            break;
        case RESOURCE_VEGETATION:
            crafting_use = "rope, cloth, and textile crafting";
            break;
        case RESOURCE_STONE:
            crafting_use = "structural components and tool making";
            break;
        case RESOURCE_GAME:
            crafting_use = "leather armor and protective equipment";
            break;
        default:
            crafting_use = "general crafting purposes";
            break;
    }
    
    snprintf(enhanced_desc_buf, sizeof(enhanced_desc_buf), 
             "A sample of %s, carefully harvested from the wilderness. "
             "This material is prized for %s and retains its natural potency.",
             material_name, crafting_use);
    
    return enhanced_desc_buf;
}

/* Integrate wilderness harvest with crafting system */
void integrate_wilderness_harvest_with_crafting(struct char_data *ch, int category, int subtype, int quality, int amount) {
    int enhanced_material_id;
    int crafting_value;
    
    if (!ch || IS_NPC(ch)) {
        return;
    }
    
    /* Get the enhanced material ID */
    enhanced_material_id = get_enhanced_wilderness_material_id(category, subtype);
    
    if (enhanced_material_id == WILDERNESS_CRAFT_MAT_NONE) {
        return;
    }
    
    /* Calculate crafting value */
    crafting_value = get_enhanced_material_crafting_value(category, subtype, quality);
    
    /* Store in wilderness material system (preserves hierarchy) */
    add_material_to_storage(ch, category, subtype, quality, amount);
    
    /* Also add crafting value to show integration */
    const char *material_name = get_enhanced_material_name(category, subtype, quality);
    send_to_char(ch, "\\cY[Enhanced Crafting]\\cn The %s has a crafting value of %d and can be used "
                     "in advanced LuminariMUD recipes.\\r\\n", 
                     material_name, crafting_value);
}

/* Enhanced materials display with crafting integration */
void show_enhanced_material_storage(struct char_data *ch) {
    int i;
    int category_counts[NUM_RESOURCE_TYPES] = {0};
    int total_materials = 0;
    
    if (!ch || IS_NPC(ch)) {
        return;
    }
    
    send_to_char(ch, "%s=== Enhanced Wilderness Materials (LuminariMUD) ===%s\r\n", 
                 CCWHT(ch, C_NRM), CCNRM(ch, C_NRM));
    send_to_char(ch, "Your materials are preserved with their full hierarchy and quality.\r\n");
    send_to_char(ch, "These materials can be used in enhanced LuminariMUD crafting recipes.\r\n\r\n");
    
    if (ch->player_specials->saved.stored_material_count == 0) {
        send_to_char(ch, "You have no wilderness materials stored.\r\n");
        send_to_char(ch, "%sEnhanced Integration: ACTIVE%s\r\n", 
                     CCYEL(ch, C_NRM), CCNRM(ch, C_NRM));
        return;
    }
    
    /* Group materials by category */
    for (i = 0; i < ch->player_specials->saved.stored_material_count; i++) {
        struct material_storage *mat = &ch->player_specials->saved.stored_materials[i];
        category_counts[mat->category]++;
        total_materials += mat->quantity;
    }
    
    /* Display materials by category */
    int cat;
    for (cat = 0; cat < NUM_RESOURCE_TYPES; cat++) {
        if (category_counts[cat] == 0) continue;
        
        /* Category header */
        const char *category_name = get_resource_name(cat);
        send_to_char(ch, "%s%s Materials:%s\r\n", 
                     CCYEL(ch, C_NRM), category_name, CCNRM(ch, C_NRM));
        
        /* Show materials in this category */
        for (i = 0; i < ch->player_specials->saved.stored_material_count; i++) {
            struct material_storage *mat = &ch->player_specials->saved.stored_materials[i];
            if (mat->category != cat) continue;
            
            const char *enhanced_name = get_enhanced_material_name(mat->category, mat->subtype, mat->quality);
            int enhanced_id = get_enhanced_wilderness_material_id(mat->category, mat->subtype);
            int crafting_value = get_enhanced_material_crafting_value(mat->category, mat->subtype, mat->quality);
            
            send_to_char(ch, "- %s (ID: %d) - Qty: %d\r\n", enhanced_name, enhanced_id, mat->quantity);
            send_to_char(ch, "  Crafting Applications: %s\r\n", get_material_applications(mat->category));
            send_to_char(ch, "  Crafting Value: %d (quality bonus: %s)\r\n\r\n", 
                        crafting_value, get_quality_bonus_description(mat->quality));
        }
    }
    
    send_to_char(ch, "Total Materials: %d units\r\n", total_materials);
    send_to_char(ch, "%sEnhanced Integration: ACTIVE%s\r\n", 
                 CCYEL(ch, C_NRM), CCNRM(ch, C_NRM));
}

#endif /* ENABLE_WILDERNESS_CRAFTING_INTEGRATION */

/* ========================================================================== */
/* Phase 5: Player Harvesting Commands Implementation                        */
/* ========================================================================== */

/* Primary wilderness harvesting command */
ACMD(do_wilderness_harvest) {
    char arg[MAX_INPUT_LENGTH];
    int resource_type = -1;
    
    one_argument(argument, arg, sizeof(arg));
    
    /* Validate wilderness location */
    if (!ZONE_FLAGGED(world[IN_ROOM(ch)].zone, ZONE_WILDERNESS)) {
        send_to_char(ch, "You can only harvest materials in the wilderness.\r\n");
        return;
    }
    
    /* Show available resources if no argument */
    if (!*arg) {
        show_harvestable_resources(ch);
        return;
    }
    
    /* Parse resource type */
    resource_type = parse_resource_type(arg);
    if (resource_type < 0) {
        send_to_char(ch, "Invalid resource type. Use 'harvest' to see available options.\r\n");
        return;
    }
    
    /* Attempt harvest */
    attempt_wilderness_harvest(ch, resource_type);
}

/* Specialized gathering command - focuses on herbs, vegetation, game */
ACMD(do_wilderness_gather) {
    char arg[MAX_INPUT_LENGTH];
    int resource_type = -1;
    
    one_argument(argument, arg, sizeof(arg));
    
    /* Validate wilderness location */
    if (!ZONE_FLAGGED(world[IN_ROOM(ch)].zone, ZONE_WILDERNESS)) {
        send_to_char(ch, "You can only gather materials in the wilderness.\r\n");
        return;
    }
    
    if (!*arg) {
        send_to_char(ch, "Gather what? Try: herbs, vegetation, game\r\n");
        return;
    }
    
    /* Parse resource type - restrict to gathering-appropriate types */
    resource_type = parse_resource_type(arg);
    if (resource_type < 0 || (resource_type != RESOURCE_HERBS && 
                              resource_type != RESOURCE_VEGETATION && 
                              resource_type != RESOURCE_GAME)) {
        send_to_char(ch, "You can only gather: herbs, vegetation, or game materials.\r\n");
        return;
    }
    
    /* Attempt harvest */
    attempt_wilderness_harvest(ch, resource_type);
}

/* Specialized mining command - focuses on minerals, crystals, metals */
ACMD(do_wilderness_mine) {
    char arg[MAX_INPUT_LENGTH];
    int resource_type = -1;
    
    one_argument(argument, arg, sizeof(arg));
    
    /* Validate wilderness location */
    if (!ZONE_FLAGGED(world[IN_ROOM(ch)].zone, ZONE_WILDERNESS)) {
        send_to_char(ch, "You can only mine materials in the wilderness.\r\n");
        return;
    }
    
    if (!*arg) {
        send_to_char(ch, "Mine what? Try: minerals, crystal, stone\r\n");
        return;
    }
    
    /* Parse resource type - restrict to mining-appropriate types */
    resource_type = parse_resource_type(arg);
    if (resource_type < 0 || (resource_type != RESOURCE_MINERALS && 
                              resource_type != RESOURCE_CRYSTAL && 
                              resource_type != RESOURCE_STONE &&
                              resource_type != RESOURCE_SALT)) {
        send_to_char(ch, "You can only mine: minerals, crystal, stone, or salt.\r\n");
        return;
    }
    
    /* Attempt harvest */
    attempt_wilderness_harvest(ch, resource_type);
}

/* Core harvesting logic */
int attempt_wilderness_harvest(struct char_data *ch, int resource_type) {
    int x, y, skill_level, success_roll;
    int category, subtype, quality, quantity;
    float resource_level;
    
    /* Get location coordinates */
    x = world[IN_ROOM(ch)].coords[0];
    y = world[IN_ROOM(ch)].coords[1];
    
    /* Check terrain suitability for this resource type */
    int sector_type = get_modified_sector_type(world[IN_ROOM(ch)].zone, x, y);
    if (!can_harvest_resource_in_terrain(resource_type, sector_type)) {
        const char *terrain_names[] = {
            "indoors", "city", "field", "forest", "hills", "mountain", 
            "shallow water", "deep water", "flying", "underwater", "zone start",
            "north-south road", "east-west road", "road intersection", "desert",
            "ocean", "marshland", "high mountain", "planes", "underdark"
        };
        const char *terrain_name = (sector_type >= 0 && sector_type < 20) ? 
                                   terrain_names[sector_type] : "unknown terrain";
        
        send_to_char(ch, "You cannot harvest %s in %s.\r\n", 
                     resource_names[resource_type], terrain_name);
        return 0;
    }
    
    /* Check if resources are too depleted */
    if (should_harvest_fail_due_to_depletion(IN_ROOM(ch), resource_type)) {
        send_to_char(ch, "This area has been over-harvested. The %s resources are too depleted to yield anything.\r\n", 
                     resource_names[resource_type]);
        return 0;
    }
    
    /* Check basic resource availability */
    resource_level = calculate_current_resource_level(resource_type, x, y);
    if (resource_level < 0.1) {
        send_to_char(ch, "There are insufficient %s resources here to harvest.\r\n", 
                     resource_names[resource_type]);
        return 0;
    }
    
    /* Get relevant skill */
    skill_level = get_harvest_skill_level(ch, resource_type);
    
    /* Calculate success with depletion modifier */
    success_roll = dice(1, 100) + skill_level;
    int difficulty = get_harvest_difficulty(resource_type, resource_level);
    
    /* Phase 6: Apply depletion penalty to success */
    float depletion_modifier = get_harvest_success_modifier(IN_ROOM(ch), resource_type);
    success_roll = (int)(success_roll * depletion_modifier);
    
    if (success_roll < difficulty) {
        send_to_char(ch, "You fail to harvest any usable %s.\r\n", 
                     resource_names[resource_type]);
        /* Phase 6: Still apply small depletion on failed attempts */
        apply_harvest_depletion(IN_ROOM(ch), resource_type, 1);
        /* TODO: Add skill improvement when function is available */
        /* improve_skill(ch, get_harvest_skill(resource_type)); */
        return 0;
    }
    
    /* Determine what was harvested */
    category = resource_type;
    subtype = determine_harvested_material_subtype(resource_type, x, y, resource_level);
    quality = calculate_harvest_quality(ch, resource_type, success_roll, skill_level);
    quantity = calculate_harvest_quantity(ch, resource_type, success_roll, skill_level);
    
    /* Add to storage (triggers Phase 4.5 integration) */
    int added = add_material_to_storage(ch, category, subtype, quality, quantity);
    
    if (added > 0) {
        const char *material_name = get_full_material_name(category, subtype, quality);
        send_to_char(ch, "You successfully harvest %d units of %s.\r\n", 
                     added, material_name);
        
        /* Phase 7: Apply depletion WITH cascade effects */
        apply_harvest_depletion_with_cascades(IN_ROOM(ch), resource_type, added);
        
        /* Phase 6: Provide feedback on resource condition */
        float new_depletion = get_resource_depletion_level(IN_ROOM(ch), resource_type);
        log("DEBUG: Resource %s - depletion level: %.3f, thresholds: severe<0.3, warning<0.6", 
            resource_names[resource_type], new_depletion);
        if (new_depletion < 0.3) {
            send_to_char(ch, "\trThis area's %s resources are becoming severely depleted.\tn\r\n", 
                         resource_names[resource_type]);
        } else if (new_depletion < 0.6) {
            send_to_char(ch, "\tyThis area's %s resources are showing signs of depletion.\tn\r\n", 
                         resource_names[resource_type]);
        }
        
        /* Phase 6: Update conservation score (sustainable if took small amount) */
        bool sustainable_harvest = (added <= 2);  /* Taking 1-2 units is sustainable */
        update_conservation_score(ch, resource_type, sustainable_harvest);
        if (!sustainable_harvest) {
            send_to_char(ch, "\tc(Your conservation score is affected by large-scale harvesting.)\tn\r\n");
        }
        
        /* TODO: Add skill improvement when function is available */
        /* improve_skill(ch, get_harvest_skill(resource_type)); */
    } else {
        send_to_char(ch, "Your material storage is full.\r\n");
    }
    
    return added;
}

/* Check if a resource type can be harvested in the current terrain */
int can_harvest_resource_in_terrain(int resource_type, int sector_type) {
    switch (resource_type) {
        case RESOURCE_VEGETATION:
            /* Can harvest vegetation in most terrestrial areas */
            switch (sector_type) {
                case SECT_FOREST:
                case SECT_FIELD: 
                case SECT_HILLS:
                case SECT_MARSHLAND:
                case SECT_DESERT:  /* Desert vegetation like cacti */
                    return 1;
                case SECT_WATER_SWIM:
                case SECT_WATER_NOSWIM:
                case SECT_UNDERWATER:
                case SECT_OCEAN:
                case SECT_FLYING:
                case SECT_HIGH_MOUNTAIN:  /* Too harsh for vegetation */
                    return 0;
                default:
                    return 1; /* Most other terrains allow some vegetation */
            }
            
        case RESOURCE_HERBS:
            /* Herbs need specific terrestrial environments */
            switch (sector_type) {
                case SECT_FOREST:
                case SECT_FIELD:
                case SECT_HILLS:
                case SECT_MARSHLAND:
                    return 1;
                case SECT_WATER_SWIM:
                case SECT_WATER_NOSWIM:
                case SECT_UNDERWATER:
                case SECT_OCEAN:
                case SECT_FLYING:
                case SECT_HIGH_MOUNTAIN:
                case SECT_DESERT:  /* Too harsh for most herbs */
                    return 0;
                default:
                    return 1;
            }
            
        case RESOURCE_WOOD:
            /* Wood only from areas with trees */
            switch (sector_type) {
                case SECT_FOREST:
                    return 1;
                case SECT_FIELD:
                case SECT_HILLS:
                    return 1; /* Scattered trees */
                case SECT_WATER_SWIM:
                case SECT_WATER_NOSWIM:
                case SECT_UNDERWATER:
                case SECT_OCEAN:
                case SECT_FLYING:
                case SECT_DESERT:
                case SECT_HIGH_MOUNTAIN:
                case SECT_MARSHLAND:  /* Dead wood maybe, but not living trees */
                    return 0;
                default:
                    return 0; /* Conservative - only specific areas have wood */
            }
            
        case RESOURCE_GAME:
            /* Animals in various terrestrial environments */
            switch (sector_type) {
                case SECT_FOREST:
                case SECT_FIELD:
                case SECT_HILLS:
                case SECT_MARSHLAND:
                case SECT_DESERT:
                    return 1;
                case SECT_WATER_SWIM:  /* Some aquatic game */
                    return 1;
                case SECT_WATER_NOSWIM:
                case SECT_UNDERWATER:
                case SECT_OCEAN:
                case SECT_FLYING:
                case SECT_HIGH_MOUNTAIN:  /* Too harsh for most game */
                    return 0;
                default:
                    return 1;
            }
            
        case RESOURCE_MINERALS:
        case RESOURCE_CRYSTAL:
            /* Minerals and crystals from rocky/underground areas */
            switch (sector_type) {
                case SECT_MOUNTAIN:
                case SECT_HIGH_MOUNTAIN:
                case SECT_HILLS:
                case SECT_DESERT:  /* Desert minerals */
                    return 1;
                case SECT_WATER_SWIM:
                case SECT_WATER_NOSWIM:
                case SECT_UNDERWATER:
                case SECT_OCEAN:
                case SECT_FLYING:
                case SECT_FOREST:  /* Soil, not bedrock */
                case SECT_FIELD:   /* Soil, not bedrock */
                case SECT_MARSHLAND:  /* Mud and water */
                    return 0;
                default:
                    return 1;
            }
            
        case RESOURCE_STONE:
            /* Stone from rocky areas */
            switch (sector_type) {
                case SECT_MOUNTAIN:
                case SECT_HIGH_MOUNTAIN:
                case SECT_HILLS:
                    return 1;
                case SECT_WATER_SWIM:
                case SECT_WATER_NOSWIM:
                case SECT_UNDERWATER:
                case SECT_OCEAN:
                case SECT_FLYING:
                case SECT_FOREST:
                case SECT_FIELD:
                case SECT_MARSHLAND:
                case SECT_DESERT:  /* Sand, not stone */
                    return 0;
                default:
                    return 0; /* Conservative - only rocky areas have stone */
            }
            
        case RESOURCE_SALT:
            /* Salt from desert, coastal, or evaporated areas */
            switch (sector_type) {
                case SECT_DESERT:
                case SECT_MARSHLAND:  /* Salt marshes */
                    return 1;
                case SECT_WATER_SWIM:
                case SECT_WATER_NOSWIM:
                case SECT_UNDERWATER:
                case SECT_OCEAN:
                case SECT_FLYING:
                case SECT_FOREST:
                case SECT_FIELD:
                case SECT_HILLS:
                case SECT_MOUNTAIN:
                case SECT_HIGH_MOUNTAIN:
                    return 0;
                default:
                    return 0;
            }
            
        case RESOURCE_WATER:
            /* Water from various sources */
            switch (sector_type) {
                case SECT_WATER_SWIM:
                case SECT_WATER_NOSWIM:
                case SECT_OCEAN:
                case SECT_MARSHLAND:
                    return 1;
                case SECT_FOREST:
                case SECT_HILLS:
                case SECT_MOUNTAIN:  /* Springs */
                    return 1;
                case SECT_DESERT:  /* Rare oases and underground springs */
                    return 1;  /* Allow harvesting but with severe resource penalties */
                case SECT_UNDERWATER:
                case SECT_FLYING:
                case SECT_HIGH_MOUNTAIN:
                case SECT_FIELD:
                    return 0;
                default:
                    return 1;
            }
            
        case RESOURCE_CLAY:
            /* Clay from marshy or riverbank areas */
            switch (sector_type) {
                case SECT_MARSHLAND:
                case SECT_FIELD:  /* Near rivers */
                    return 1;
                case SECT_WATER_SWIM:
                case SECT_WATER_NOSWIM:
                case SECT_UNDERWATER:
                case SECT_OCEAN:
                case SECT_FLYING:
                case SECT_FOREST:
                case SECT_HILLS:
                case SECT_MOUNTAIN:
                case SECT_HIGH_MOUNTAIN:
                case SECT_DESERT:
                    return 0;
                default:
                    return 0;
            }
            
        default:
            return 1; /* Unknown resource types default to allowed */
    }
}

/* Support Functions */

int get_harvest_skill_level(struct char_data *ch, int resource_type) {
    int skill = get_harvest_skill(resource_type);
    return GET_SKILL(ch, skill);
}

int get_harvest_skill(int resource_type) {
    switch (resource_type) {
        case RESOURCE_HERBS:
        case RESOURCE_VEGETATION:
            return ABILITY_HARVEST_GATHERING;
        case RESOURCE_MINERALS:
        case RESOURCE_CRYSTAL:
        case RESOURCE_STONE:
        case RESOURCE_SALT:
            return SKILL_MINING;
        case RESOURCE_WOOD:
            return ABILITY_HARVEST_FORESTRY;
        case RESOURCE_GAME:
            return ABILITY_HARVEST_HUNTING;
        case RESOURCE_WATER:
        default:
            return ABILITY_SURVIVAL;
    }
}

int get_harvest_difficulty(int resource_type, float resource_level) {
    /* Base difficulty varies by resource type */
    int base_difficulty = 50;
    
    switch (resource_type) {
        case RESOURCE_HERBS:
            base_difficulty = 30; /* Easier */
            break;
        case RESOURCE_VEGETATION:
            base_difficulty = 25; /* Easiest */
            break;
        case RESOURCE_WOOD:
            base_difficulty = 40;
            break;
        case RESOURCE_GAME:
            base_difficulty = 60; /* Harder */
            break;
        case RESOURCE_MINERALS:
            base_difficulty = 55;
            break;
        case RESOURCE_CRYSTAL:
            base_difficulty = 75; /* Hardest */
            break;
        case RESOURCE_STONE:
            base_difficulty = 45;
            break;
        case RESOURCE_SALT:
            base_difficulty = 35;
            break;
        case RESOURCE_WATER:
            base_difficulty = 20; /* Very easy */
            break;
    }
    
    /* Modify by resource availability */
    if (resource_level > 0.8) base_difficulty -= 10;      /* Abundant */
    else if (resource_level > 0.6) base_difficulty -= 5;  /* Good */
    else if (resource_level < 0.3) base_difficulty += 10; /* Scarce */
    else if (resource_level < 0.2) base_difficulty += 20; /* Very scarce */
    
    return base_difficulty;
}

int calculate_harvest_quality(struct char_data *ch, int resource_type, int success_roll, int skill_level) {
    /* Quality determination based on skill and success */
    int quality_roll = dice(1, 100) + (skill_level / 2) + (success_roll - 50);
    
    if (quality_roll >= 150) return MATERIAL_QUALITY_LEGENDARY;
    if (quality_roll >= 120) return MATERIAL_QUALITY_RARE;
    if (quality_roll >= 90)  return MATERIAL_QUALITY_UNCOMMON;
    if (quality_roll >= 60)  return MATERIAL_QUALITY_COMMON;
    return MATERIAL_QUALITY_POOR;
}

int calculate_harvest_quantity(struct char_data *ch, int resource_type, int success_roll, int skill_level) {
    /* Base quantity with skill and success modifiers */
    int base_quantity = 1;
    int skill_bonus = MAX(0, (skill_level - 50) / 25); /* +1 per 25 skill over 50 */
    int success_bonus = MAX(0, (success_roll - 75) / 25); /* +1 per 25 over 75 */
    
    return base_quantity + skill_bonus + success_bonus;
}

void show_harvestable_resources(struct char_data *ch) {
    int x, y, i;
    float resource_level;
    
    x = world[IN_ROOM(ch)].coords[0];
    y = world[IN_ROOM(ch)].coords[1];
    
    send_to_char(ch, "Harvestable resources at this location:\r\n");
    send_to_char(ch, "=====================================\r\n");
    
    for (i = 0; i < NUM_RESOURCE_TYPES; i++) {
        resource_level = calculate_current_resource_level(i, x, y);
        if (resource_level > 0.1) { /* Only show harvestable resources */
            float depletion_level = get_resource_depletion_level(IN_ROOM(ch), i);
            float effective_level = resource_level * depletion_level; /* Calculate true available amount */
            send_to_char(ch, "  \tG%-12s\tn: %s (harvest %s)\r\n", 
                         resource_names[i], 
                         get_abundance_description(effective_level), /* Use effective level, not base level */
                         resource_names[i]);
        }
    }
    
    send_to_char(ch, "\r\nUsage: harvest <resource_type>\r\n");
    send_to_char(ch, "Specialized commands: gather <type>, mine <type>\r\n");
}