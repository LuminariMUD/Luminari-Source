/* *************************************************************************
 *   File: resource_system.h                           Part of LuminariMUD *
 *  Usage: Header file for the wilderness resource system                  *
 * Author: Implementation Team                                             *
 ***************************************************************************
 *                                                                         *
 ***************************************************************************/

#ifndef _RESOURCE_SYSTEM_H_
#define _RESOURCE_SYSTEM_H_

#include "structs.h"

/* Maximum number of resource types supported */
#define MAX_RESOURCE_TYPES 16

/* Resource type enumeration */
enum resource_types {
    RESOURCE_VEGETATION = 0,    /* General plant life, grasses, shrubs */
    RESOURCE_MINERALS,          /* Ores, metals, stones */
    RESOURCE_WATER,             /* Fresh water sources */
    RESOURCE_HERBS,             /* Medicinal or magical plants */
    RESOURCE_GAME,              /* Huntable animals */
    RESOURCE_WOOD,              /* Harvestable timber */
    RESOURCE_STONE,             /* Building materials */
    RESOURCE_CRYSTAL,           /* Rare magical components */
    RESOURCE_CLAY,              /* Crafting materials */
    RESOURCE_SALT,              /* Preservation materials */
    NUM_RESOURCE_TYPES
};

/* Resource quality tiers */
#define RESOURCE_QUALITY_POOR      1
#define RESOURCE_QUALITY_COMMON    2
#define RESOURCE_QUALITY_UNCOMMON  3
#define RESOURCE_QUALITY_RARE      4
#define RESOURCE_QUALITY_LEGENDARY 5

/* Resource configuration for each type */
struct resource_config {
    int noise_layer;           /* Which Perlin noise layer to use */
    float base_multiplier;     /* Global scaling factor (0.0-2.0) */
    float regen_rate_per_hour; /* Regeneration rate per hour (0.0-1.0) */
    float depletion_threshold; /* Maximum harvest before depletion (0.0-1.0) */
    int quality_variance;      /* Quality variation percentage (0-100) */
    bool seasonal_affected;    /* Whether seasons affect this resource */
    bool weather_affected;     /* Whether weather affects this resource */
    int harvest_skill;         /* Associated harvest skill */
    const char *name;          /* Display name */
    const char *description;   /* Detailed description */
};

/* Resource node for KD-tree storage - tracks harvest history */
struct resource_node {
    int x, y;                                      /* Coordinates */
    float consumed_amount[NUM_RESOURCE_TYPES];     /* Amount harvested (0.0-1.0) */
    time_t last_harvest[NUM_RESOURCE_TYPES];       /* Timestamp of last harvest */
    int harvest_count[NUM_RESOURCE_TYPES];         /* Number of times harvested */
    struct resource_node *left, *right;           /* KD-tree structure */
};

/* Resource cache node for spatial caching */
struct resource_cache_node {
    int x, y;                                      /* Coordinates */
    float cached_values[NUM_RESOURCE_TYPES];       /* Cached resource levels */
    time_t cache_time;                             /* When this was cached */
    struct resource_cache_node *left, *right;     /* KD-tree structure */
};

/* Cache configuration */
#define RESOURCE_CACHE_LIFETIME 300        /* Cache lifetime in seconds (5 minutes) */
#define RESOURCE_CACHE_MAX_NODES 1000      /* Maximum cached nodes */
#define RESOURCE_CACHE_GRID_SIZE 10        /* Cache every N coordinates */

/* Resource abundance descriptions */
struct resource_abundance {
    float min_level;
    const char *description;
    const char *survey_text;
};

/* Function prototypes */

/* Core resource calculation functions */
float calculate_current_resource_level(int resource_type, int x, int y);
float get_base_resource_value(int resource_type, int x, int y);
float apply_environmental_modifiers(int resource_type, int x, int y, float base_value);
float apply_harvest_regeneration(int resource_type, float base_value, struct resource_node *node);
float apply_region_resource_modifiers(int resource_type, int x, int y, float base_value);

/* Environmental modifier functions */
float get_seasonal_modifier(int resource_type);
float get_weather_modifier(int resource_type, int weather);

/* Resource node management */
struct resource_node *find_or_create_resource_node(int x, int y);
struct resource_node *kdtree_find_resource_node(int x, int y);
void kdtree_insert_resource_node(struct resource_node *node);
void kdtree_remove_resource_node(int x, int y);
void cleanup_old_resource_nodes(void);

/* Resource quality and description functions */
int determine_resource_quality(int resource_type, int x, int y, float level);
const char *get_abundance_description(float level);
const char *get_quality_description(int resource_type, int x, int y, float level);
const char *get_resource_name(int resource_type);
int parse_resource_type(const char *arg);

/* Resource mapping and display */
char get_resource_map_symbol(float level);
const char *get_resource_color(float level);

/* Resource caching functions */
struct resource_cache_node *cache_find_resource_values(int x, int y);
void cache_store_resource_values(int x, int y, float values[NUM_RESOURCE_TYPES]);
void cache_cleanup_expired(void);
void cache_clear_all(void);
int cache_get_stats(int *total_nodes, int *expired_nodes);

/* Survey command functions */
void show_resource_survey(struct char_data *ch);
void show_resource_map(struct char_data *ch, int resource_type, int radius);
void show_resource_detail(struct char_data *ch, int resource_type);
void show_terrain_survey(struct char_data *ch);
void show_debug_survey(struct char_data *ch);

/* Initialization and cleanup */
void init_resource_system(void);
void shutdown_resource_system(void);

/* Global resource configuration array */
extern struct resource_config resource_configs[NUM_RESOURCE_TYPES];

/* Global KD-tree for resource nodes */
extern struct resource_node *resource_kd_tree;

/* Global KD-tree for resource cache */
extern struct resource_cache_node *resource_cache_tree;
extern int resource_cache_count;

/* Resource name array for display */
extern const char *resource_names[NUM_RESOURCE_TYPES];

/* Phase 4: Region integration functions */
/* Phase 4b: Region Effects System Functions */
float apply_region_resource_modifiers(int resource_type, int x, int y, float base_value);
void apply_region_resource_modifiers_to_node(struct char_data *ch, struct resource_node *resources);

/* Region Effects Admin Functions */
void resourceadmin_effects_list(struct char_data *ch);
void resourceadmin_effects_show(struct char_data *ch, int effect_id);
void resourceadmin_effects_assign(struct char_data *ch, int region_vnum, int effect_id, double intensity);
void resourceadmin_effects_unassign(struct char_data *ch, int region_vnum, int effect_id);
void resourceadmin_effects_region(struct char_data *ch, int region_vnum);
void apply_json_resource_modifiers(struct resource_node *resources, const char *json_data, double intensity);

#endif /* _RESOURCE_SYSTEM_H_ */
