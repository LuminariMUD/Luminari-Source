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

/* Phase 4.5: Material Subtype System - Three-tier hierarchy */
/* 
 * This system provides specific named materials within each resource category
 * Structure: Category (vegetation/herbs/etc) -> Subtype (marjoram/kingfoil/etc) -> Quality (poor/rare/etc)
 */

/* Herb subtypes - specific named herbs with lore significance */
enum herb_subtypes {
    HERB_MARJORAM = 0,          /* Common healing herb */
    HERB_KINGFOIL,              /* Powerful healing properties */
    HERB_STARLILY,              /* Rare magical component */
    HERB_WOLFSBANE,             /* Poison and protective wards */
    HERB_SILVERLEAF,            /* Anti-undead properties */
    HERB_MOONBELL,              /* Enchantment components */
    HERB_THORNWEED,             /* Alchemical bitter */
    HERB_BRIGHTROOT,            /* Light-based magic */
    NUM_HERB_SUBTYPES
};

/* Crystal subtypes - magical crystals and gems */
enum crystal_subtypes {
    CRYSTAL_ARCANITE = 0,       /* Pure magical energy crystal */
    CRYSTAL_NETHERMOTE,         /* Shadow magic component */
    CRYSTAL_SUNSTONE,           /* Light/fire magic crystal */
    CRYSTAL_VOIDSHARDS,         /* Rare planar material */
    CRYSTAL_DREAMQUARTZ,        /* Divination/sleep magic */
    CRYSTAL_BLOODSTONE,         /* Life force manipulation */
    CRYSTAL_FROSTGEM,           /* Ice/cold magic component */
    CRYSTAL_STORMCRYSTAL,       /* Lightning/weather magic */
    NUM_CRYSTAL_SUBTYPES
};

/* Ore subtypes - metal ores and rare materials */
enum ore_subtypes {
    ORE_MITHRIL = 0,            /* Legendary lightweight metal */
    ORE_ADAMANTINE,             /* Incredibly hard metal */
    ORE_COLD_IRON,              /* Anti-fey properties */
    ORE_STAR_STEEL,             /* Meteoric metal */
    ORE_DEEP_SILVER,            /* Underdark precious metal */
    ORE_DRAGONSTEEL,            /* Dragon-touched metal */
    ORE_VOIDMETAL,              /* Planar-infused ore */
    ORE_BRIGHTCOPPER,           /* Magically conductive */
    NUM_ORE_SUBTYPES
};

/* Wood subtypes - special trees and magical wood */
enum wood_subtypes {
    WOOD_IRONWOOD = 0,          /* Hard as metal tree */
    WOOD_SILVERBIRCH,           /* Anti-evil properties */
    WOOD_SHADOWBARK,            /* Stealth-enhancing wood */
    WOOD_BRIGHTOAK,             /* Light-channeling tree */
    WOOD_THORNVINE,             /* Defensive barrier material */
    WOOD_MOONWEAVE,             /* Flexible magical fiber */
    WOOD_DARKWILLOW,            /* Necromantic components */
    WOOD_STARWOOD,              /* Celestial-touched timber */
    NUM_WOOD_SUBTYPES
};

/* Vegetation subtypes - useful plants and fibers */
enum vegetation_subtypes {
    VEG_COTTON = 0,             /* Basic fiber */
    VEG_SILK_MOSS,              /* Fine magical fiber */
    VEG_HEMP_VINE,              /* Strong rope material */
    VEG_SPIRIT_GRASS,           /* Ethereal component */
    VEG_FLAME_FLOWER,           /* Fire resistance fiber */
    VEG_ICE_LICHEN,             /* Cold resistance material */
    VEG_SHADOW_FERN,            /* Stealth enhancement */
    VEG_LIGHT_BLOOM,            /* Illumination component */
    NUM_VEGETATION_SUBTYPES
};

/* Stone subtypes - building and crafting materials */
enum stone_subtypes {
    STONE_GRANITE = 0,          /* Common building stone */
    STONE_MARBLE,               /* Fine crafting material */
    STONE_OBSIDIAN,             /* Sharp volcanic glass */
    STONE_MOONSTONE,            /* Magical building material */
    STONE_DRAGONBONE,           /* Fossilized dragon remains */
    STONE_VOIDROCK,             /* Planar construction material */
    STONE_STARSTONE,            /* Celestial building blocks */
    STONE_SHADOWSLATE,          /* Stealth construction */
    NUM_STONE_SUBTYPES
};

/* Game subtypes - animal-derived materials */
enum game_subtypes {
    GAME_LEATHER = 0,           /* Basic hide */
    GAME_DRAGONHIDE,            /* Dragon leather */
    GAME_SHADOWPELT,            /* Stealth-enhancing hide */
    GAME_BRIGHTFUR,             /* Light-attuned fur */
    GAME_WINTERWOOL,            /* Cold resistance material */
    GAME_SPIRIT_SILK,           /* Ethereal creature material */
    GAME_VOIDHIDE,              /* Planar creature leather */
    GAME_STARFUR,               /* Celestial creature hide */
    NUM_GAME_SUBTYPES
};

/* Maximum subtype values for bounds checking */
#define MAX_HERB_SUBTYPES      NUM_HERB_SUBTYPES
#define MAX_CRYSTAL_SUBTYPES   NUM_CRYSTAL_SUBTYPES
#define MAX_ORE_SUBTYPES       NUM_ORE_SUBTYPES
#define MAX_WOOD_SUBTYPES      NUM_WOOD_SUBTYPES
#define MAX_VEG_SUBTYPES       NUM_VEGETATION_SUBTYPES
#define MAX_STONE_SUBTYPES     NUM_STONE_SUBTYPES
#define MAX_GAME_SUBTYPES      NUM_GAME_SUBTYPES

/* Resource quality tiers */
#define RESOURCE_QUALITY_POOR      1
#define RESOURCE_QUALITY_COMMON    2
#define RESOURCE_QUALITY_UNCOMMON  3
#define RESOURCE_QUALITY_RARE      4
#define RESOURCE_QUALITY_LEGENDARY 5

/* Phase 4.75: Enhanced Wilderness-Crafting Integration (LuminariMUD only) */
#ifdef ENABLE_WILDERNESS_CRAFTING_INTEGRATION

/* Enhanced material storage system that preserves hierarchy */
#define WILDERNESS_CRAFT_MAT_NONE               0

/* Herb-based crafting materials (herbs → alchemical components) */
#define WILDERNESS_CRAFT_MAT_HERB_BASE          1000
#define WILDERNESS_CRAFT_MAT_MARJORAM           (WILDERNESS_CRAFT_MAT_HERB_BASE + HERB_MARJORAM)
#define WILDERNESS_CRAFT_MAT_KINGFOIL           (WILDERNESS_CRAFT_MAT_HERB_BASE + HERB_KINGFOIL)
#define WILDERNESS_CRAFT_MAT_STARLILY           (WILDERNESS_CRAFT_MAT_HERB_BASE + HERB_STARLILY)
#define WILDERNESS_CRAFT_MAT_WOLFSBANE          (WILDERNESS_CRAFT_MAT_HERB_BASE + HERB_WOLFSBANE)
#define WILDERNESS_CRAFT_MAT_SILVERLEAF         (WILDERNESS_CRAFT_MAT_HERB_BASE + HERB_SILVERLEAF)
#define WILDERNESS_CRAFT_MAT_MOONBELL           (WILDERNESS_CRAFT_MAT_HERB_BASE + HERB_MOONBELL)
#define WILDERNESS_CRAFT_MAT_THORNWEED          (WILDERNESS_CRAFT_MAT_HERB_BASE + HERB_THORNWEED)
#define WILDERNESS_CRAFT_MAT_BRIGHTROOT         (WILDERNESS_CRAFT_MAT_HERB_BASE + HERB_BRIGHTROOT)

/* Crystal-based crafting materials (crystals → enchanting components) */
#define WILDERNESS_CRAFT_MAT_CRYSTAL_BASE       1100
#define WILDERNESS_CRAFT_MAT_ARCANITE           (WILDERNESS_CRAFT_MAT_CRYSTAL_BASE + CRYSTAL_ARCANITE)
#define WILDERNESS_CRAFT_MAT_NETHERMOTE         (WILDERNESS_CRAFT_MAT_CRYSTAL_BASE + CRYSTAL_NETHERMOTE)
#define WILDERNESS_CRAFT_MAT_SUNSTONE           (WILDERNESS_CRAFT_MAT_CRYSTAL_BASE + CRYSTAL_SUNSTONE)
#define WILDERNESS_CRAFT_MAT_VOIDSHARDS         (WILDERNESS_CRAFT_MAT_CRYSTAL_BASE + CRYSTAL_VOIDSHARDS)
#define WILDERNESS_CRAFT_MAT_DREAMQUARTZ        (WILDERNESS_CRAFT_MAT_CRYSTAL_BASE + CRYSTAL_DREAMQUARTZ)
#define WILDERNESS_CRAFT_MAT_BLOODSTONE         (WILDERNESS_CRAFT_MAT_CRYSTAL_BASE + CRYSTAL_BLOODSTONE)
#define WILDERNESS_CRAFT_MAT_FROSTGEM           (WILDERNESS_CRAFT_MAT_CRYSTAL_BASE + CRYSTAL_FROSTGEM)
#define WILDERNESS_CRAFT_MAT_STORMCRYSTAL       (WILDERNESS_CRAFT_MAT_CRYSTAL_BASE + CRYSTAL_STORMCRYSTAL)

/* Ore-based crafting materials (ores → metal crafting) */
#define WILDERNESS_CRAFT_MAT_ORE_BASE           1200
#define WILDERNESS_CRAFT_MAT_MITHRIL_ORE        (WILDERNESS_CRAFT_MAT_ORE_BASE + ORE_MITHRIL)
#define WILDERNESS_CRAFT_MAT_ADAMANTINE_ORE     (WILDERNESS_CRAFT_MAT_ORE_BASE + ORE_ADAMANTINE)
#define WILDERNESS_CRAFT_MAT_COLD_IRON_ORE      (WILDERNESS_CRAFT_MAT_ORE_BASE + ORE_COLD_IRON)
#define WILDERNESS_CRAFT_MAT_STAR_STEEL_ORE     (WILDERNESS_CRAFT_MAT_ORE_BASE + ORE_STAR_STEEL)
#define WILDERNESS_CRAFT_MAT_DEEP_SILVER_ORE    (WILDERNESS_CRAFT_MAT_ORE_BASE + ORE_DEEP_SILVER)
#define WILDERNESS_CRAFT_MAT_DRAGONSTEEL_ORE    (WILDERNESS_CRAFT_MAT_ORE_BASE + ORE_DRAGONSTEEL)
#define WILDERNESS_CRAFT_MAT_VOIDMETAL_ORE      (WILDERNESS_CRAFT_MAT_ORE_BASE + ORE_VOIDMETAL)
#define WILDERNESS_CRAFT_MAT_BRIGHTCOPPER_ORE   (WILDERNESS_CRAFT_MAT_ORE_BASE + ORE_BRIGHTCOPPER)

/* Wood-based crafting materials (wood → woodworking) */
#define WILDERNESS_CRAFT_MAT_WOOD_BASE          1300
#define WILDERNESS_CRAFT_MAT_IRONWOOD           (WILDERNESS_CRAFT_MAT_WOOD_BASE + WOOD_IRONWOOD)
#define WILDERNESS_CRAFT_MAT_SILVERBIRCH        (WILDERNESS_CRAFT_MAT_WOOD_BASE + WOOD_SILVERBIRCH)
#define WILDERNESS_CRAFT_MAT_SHADOWBARK         (WILDERNESS_CRAFT_MAT_WOOD_BASE + WOOD_SHADOWBARK)
#define WILDERNESS_CRAFT_MAT_BRIGHTOAK          (WILDERNESS_CRAFT_MAT_WOOD_BASE + WOOD_BRIGHTOAK)
#define WILDERNESS_CRAFT_MAT_THORNVINE          (WILDERNESS_CRAFT_MAT_WOOD_BASE + WOOD_THORNVINE)
#define WILDERNESS_CRAFT_MAT_MOONWEAVE          (WILDERNESS_CRAFT_MAT_WOOD_BASE + WOOD_MOONWEAVE)
#define WILDERNESS_CRAFT_MAT_DARKWILLOW         (WILDERNESS_CRAFT_MAT_WOOD_BASE + WOOD_DARKWILLOW)
#define WILDERNESS_CRAFT_MAT_STARWOOD           (WILDERNESS_CRAFT_MAT_WOOD_BASE + WOOD_STARWOOD)

/* Total enhanced wilderness materials */
#define NUM_ENHANCED_WILDERNESS_MATERIALS       (NUM_HERB_SUBTYPES + NUM_CRYSTAL_SUBTYPES + NUM_ORE_SUBTYPES + NUM_WOOD_SUBTYPES + NUM_VEGETATION_SUBTYPES + NUM_STONE_SUBTYPES + NUM_GAME_SUBTYPES)

#endif /* ENABLE_WILDERNESS_CRAFTING_INTEGRATION */

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

/* Phase 4.5: Material Subtype Management Functions */
/* Material storage and retrieval */
int add_material_to_storage(struct char_data *ch, int category, int subtype, int quality, int quantity);
int remove_material_from_storage(struct char_data *ch, int category, int subtype, int quality, int quantity);
int get_material_quantity(struct char_data *ch, int category, int subtype, int quality);
void show_material_storage(struct char_data *ch);

/* Material name and description functions */
const char *get_material_subtype_name(int category, int subtype);
const char *get_material_quality_name(int quality);
const char *get_full_material_name(int category, int subtype, int quality);
const char *get_material_description(int category, int subtype, int quality);

/* Material conversion and validation */
int validate_material_data(int category, int subtype, int quality);
int get_max_subtypes_for_category(int category);
bool is_wilderness_only_material(int category, int subtype);

/* Material harvesting functions */
int determine_harvested_material_subtype(int resource_type, int x, int y, float level);
int calculate_material_quality_from_resource(int resource_type, int x, int y, float level);

/* Material utility functions */
void init_material_storage(struct char_data *ch);
void cleanup_material_storage(struct char_data *ch);
int compact_material_storage(struct char_data *ch);
void resourceadmin_effects_show(struct char_data *ch, int effect_id);
void resourceadmin_effects_assign(struct char_data *ch, int region_vnum, int effect_id, double intensity);
void resourceadmin_effects_unassign(struct char_data *ch, int region_vnum, int effect_id);
void resourceadmin_effects_region(struct char_data *ch, int region_vnum);
void apply_json_resource_modifiers(struct resource_node *resources, const char *json_data, double intensity);

#ifdef ENABLE_WILDERNESS_CRAFTING_INTEGRATION
/* Enhanced wilderness-crafting integration functions (LuminariMUD only) */
int get_enhanced_wilderness_material_id(int category, int subtype);
const char *get_enhanced_material_name(int category, int subtype, int quality);
int get_enhanced_material_crafting_value(int category, int subtype, int quality);
void integrate_wilderness_harvest_with_crafting(struct char_data *ch, int category, int subtype, int quality, int amount);
bool is_enhanced_wilderness_material(int material_id);
const char *get_enhanced_material_description(int category, int subtype, int quality);
#endif /* ENABLE_WILDERNESS_CRAFTING_INTEGRATION */

#endif /* _RESOURCE_SYSTEM_H_ */
