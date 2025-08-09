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
#include "mysql.h"
#include "spells.h"
#include "genolc.h"
#include "constants.h"
#include "kdtree.h"

/* Forward declarations for region integration */
/* Phase 4b: Region Effects Forward Declarations */

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
    
    /* Calculate normalized coordinates */
    double norm_x = x / (double)(WILD_X_SIZE / 4.0);
    double norm_y = y / (double)(WILD_Y_SIZE / 4.0);
    
    /* Get Perlin noise for this resource type */
    double noise_value = PerlinNoise2D(config->noise_layer, norm_x, norm_y, 2.0, 2.0, 8);
    
    /* Normalize to 0.0-1.0 and apply base multiplier */
    float normalized = ((noise_value + 1.0) / 2.0) * config->base_multiplier;
    
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

char get_resource_map_symbol(float level) {
    if (level >= 0.8) return '#';      /* Very rich */
    if (level >= 0.6) return '*';      /* Rich */  
    if (level >= 0.4) return '+';      /* Moderate */
    if (level >= 0.2) return '.';      /* Poor */
    if (level >= 0.05) return '_';     /* Trace */
    return ' ';                        /* None */
}

const char *get_resource_color(float level) {
    if (level >= 0.8) return "\t[f046]";      /* Bright green */
    if (level >= 0.6) return "\t[f034]";      /* Green */
    if (level >= 0.4) return "\t[f226]";      /* Yellow */
    if (level >= 0.2) return "\t[f208]";      /* Orange */
    if (level >= 0.05) return "\t[f240]";     /* Gray */
    return "\t[f236]";                        /* Dark gray */
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
    log("INFO: Initializing wilderness resource system");
    
    /* Initialize the resource KD-tree */
    resource_kd_tree = NULL;
    
    /* Initialize the resource cache */
    resource_cache_tree = NULL;
    resource_cache_count = 0;
    
    /* Initialize Perlin noise for resource layers */
    /* This is handled by the existing Perlin system */
    
    log("INFO: Resource system initialized with %d resource types and spatial caching", NUM_RESOURCE_TYPES);
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
                 sector_types[SECT(IN_ROOM(ch))], get_elevation(NOISE_MATERIAL_PLANE_ELEV, x, y));
    
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
    elevation = get_elevation(NOISE_MATERIAL_PLANE_ELEV, x, y);
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
            send_to_char(ch, "  Arid desert environment with limited vegetation.\r\n");
            send_to_char(ch, "  Favorable for: minerals, crystals (sparse vegetation)\r\n");
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
    send_to_char(ch, "Elevation: %d\r\n", get_elevation(NOISE_MATERIAL_PLANE_ELEV, x, y));
    
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
        send_to_char(ch, " ");
        for (map_x = -radius; map_x <= radius; map_x++) {
            x = center_x + map_x;
            y = center_y + map_y;
            
            /* Check if this is the player's position */
            if (map_x == 0 && map_y == 0) {
                send_to_char(ch, "\tW@\tn"); /* Player position */
            } else {
                resource_level = calculate_current_resource_level(resource_type, x, y);
                symbol = get_resource_map_symbol(resource_level);
                color = get_resource_color(resource_level);
                send_to_char(ch, "%s%c\tn", color, symbol);
            }
        }
        send_to_char(ch, "\r\n");
    }
    
    send_to_char(ch, "========================================\r\n");
    send_to_char(ch, "Current location (\tW@\tn): (%d, %d)\r\n", center_x, center_y);
    
    /* Show resource level at current location */
    resource_level = calculate_current_resource_level(resource_type, center_x, center_y);
    send_to_char(ch, "Current %s level: %s (%.1f%%)\r\n", 
                 resource_names[resource_type],
                 get_abundance_description(resource_level),
                 resource_level * 100.0f);
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
    send_to_char(ch, "Elevation: %d\r\n", get_elevation(NOISE_MATERIAL_PLANE_ELEV, x, y));
    
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

/* ===== PHASE 4B: REGION EFFECTS SYSTEM FUNCTIONS ===== */
