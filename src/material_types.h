/* *************************************************************************
 *   File: material_types.h                           Part of LuminariMUD *
 *  Usage: Material subtype and quality system for Phase 4.5             *
 * Author: Implementation Team                                             *
 ***************************************************************************
 *                                                                         *
 ***************************************************************************/

#ifndef _MATERIAL_TYPES_H_
#define _MATERIAL_TYPES_H_

#include "resource_system.h"

/* Maximum subtypes per resource category */
#define MAX_SUBTYPES_PER_CATEGORY 32

/* Quality levels for all materials */
enum material_quality {
    QUALITY_POOR = 0,          // 50% base value
    QUALITY_COMMON = 1,        // 100% base value  
    QUALITY_GOOD = 2,          // 150% base value
    QUALITY_EXCELLENT = 3,     // 200% base value
    QUALITY_MASTERWORK = 4,    // 300% base value
    NUM_QUALITY_LEVELS
};

/* Herb subtypes (Luminari lore-specific) */
enum herb_subtypes {
    HERB_MARJORAM = 0,         // Common cooking herb
    HERB_KINGFOIL = 1,         // Healing herb (lore-important)
    HERB_STARLILY = 2,         // Magical reagent
    HERB_BLOODMOSS = 3,        // Alchemy component
    HERB_MOONBELL = 4,         // Night-blooming magical herb
    HERB_THORNLEAF = 5,        // Protection ward component
    HERB_SILKWEED = 6,         // Textile material
    HERB_FIREBLOOM = 7,        // Fire resistance component
    NUM_HERB_SUBTYPES
};

/* Crystal subtypes (Arcanite important to Luminari lore) */
enum crystal_subtypes {
    CRYSTAL_ARCANITE = 0,      // Major lore crystal - magical focus
    CRYSTAL_QUARTZ = 1,        // Common crystal
    CRYSTAL_AMETHYST = 2,      // Semi-precious
    CRYSTAL_DIAMOND = 3,       // Rare precious stone
    CRYSTAL_OBSIDIAN = 4,      // Volcanic glass
    CRYSTAL_SAPPHIRE = 5,      // Blue precious stone
    CRYSTAL_RUBY = 6,          // Red precious stone
    CRYSTAL_EMERALD = 7,       // Green precious stone
    NUM_CRYSTAL_SUBTYPES
};

/* Ore and metal subtypes */
enum ore_subtypes {
    ORE_IRON = 0,             // Common metal
    ORE_COPPER = 1,           // Common metal
    ORE_MITHRIL = 2,          // Rare magical metal
    ORE_ADAMANTIUM = 3,       // Legendary metal
    ORE_SILVER = 4,           // Precious metal
    ORE_GOLD = 5,             // Precious metal
    ORE_TIN = 6,              // Common metal
    ORE_LEAD = 7,             // Heavy metal
    NUM_ORE_SUBTYPES
};

/* Wood subtypes */
enum wood_subtypes {
    WOOD_BIRCH = 0,           // Common light wood
    WOOD_OAK = 1,             // Common strong wood
    WOOD_IRONWOOD = 2,        // Rare magical wood
    WOOD_YEWWOOD = 3,         // Bow-making wood
    WOOD_ELDERWOOD = 4,       // Ancient magical wood
    WOOD_PINE = 5,            // Common softwood
    WOOD_REDWOOD = 6,         // Large tree wood
    WOOD_EBONY = 7,           // Dark precious wood
    NUM_WOOD_SUBTYPES
};

/* Stone subtypes */
enum stone_subtypes {
    STONE_GRANITE = 0,        // Common building stone
    STONE_MARBLE = 1,         // Precious building stone
    STONE_SLATE = 2,          // Flat layered stone
    STONE_SANDSTONE = 3,      // Soft building stone
    STONE_LIMESTONE = 4,      // Common sedimentary stone
    STONE_BASALT = 5,         // Volcanic stone
    STONE_OBSIDIAN_STONE = 6, // Volcanic glass stone
    STONE_FLINT = 7,          // Tool-making stone
    NUM_STONE_SUBTYPES
};

/* Game/Animal subtypes */
enum game_subtypes {
    GAME_DEER = 0,            // Common forest animal
    GAME_RABBIT = 1,          // Small common animal
    GAME_BOAR = 2,            // Aggressive forest animal
    GAME_BEAR = 3,            // Large dangerous animal
    GAME_WOLF = 4,            // Pack hunter
    GAME_FOX = 5,             // Small cunning animal
    GAME_ELK = 6,             // Large antlered animal
    GAME_FISH = 7,            // Aquatic animals
    NUM_GAME_SUBTYPES
};

/* Complete material structure */
struct harvested_material {
    int category;             // RESOURCE_HERBS, RESOURCE_MINERALS, etc.
    int subtype;             // HERB_KINGFOIL, CRYSTAL_ARCANITE, etc.
    int quality;             // QUALITY_POOR to QUALITY_MASTERWORK
    int amount;              // Quantity harvested
    time_t harvested_at;     // When harvested
    int harvester_id;        // Who harvested it
};

/* Subtype spawn configuration */
struct subtype_spawn_table {
    int category;             // Which resource category
    int subtype;             // Which specific subtype
    float base_rarity;       // 0.0 to 1.0 (1.0 = always available)
    int min_skill_level;     // Minimum skill to find this
    int region_modifier;     // Bonus in certain regions
    const char *regions_found[8]; // List of regions where this spawns
    const char *name;        // Display name
    const char *description; // Lore description
};

/* Material access macros */
#define GET_MATERIAL(ch, cat, sub, qual) \
    ((ch)->player_specials->saved.wilderness_materials[cat][sub][qual])

#define ADD_MATERIAL(ch, cat, sub, qual, amt) \
    ((ch)->player_specials->saved.wilderness_materials[cat][sub][qual] += (amt))

#define HAS_MATERIAL(ch, cat, sub, qual, min) \
    (GET_MATERIAL(ch, cat, sub, qual) >= (min))

#define REMOVE_MATERIAL(ch, cat, sub, qual, amt) \
    ((ch)->player_specials->saved.wilderness_materials[cat][sub][qual] = \
     MAX(0, GET_MATERIAL(ch, cat, sub, qual) - (amt)))

/* Quality value multipliers */
extern const float quality_multipliers[NUM_QUALITY_LEVELS];

/* Subtype spawn tables */
extern struct subtype_spawn_table herb_spawn_table[NUM_HERB_SUBTYPES];
extern struct subtype_spawn_table crystal_spawn_table[NUM_CRYSTAL_SUBTYPES];
extern struct subtype_spawn_table ore_spawn_table[NUM_ORE_SUBTYPES];
extern struct subtype_spawn_table wood_spawn_table[NUM_WOOD_SUBTYPES];
extern struct subtype_spawn_table stone_spawn_table[NUM_STONE_SUBTYPES];
extern struct subtype_spawn_table game_spawn_table[NUM_GAME_SUBTYPES];

/* Function prototypes */

/* Material subtype functions */
int select_material_subtype(int category, int x, int y, int skill_level);
int determine_material_quality(int category, int subtype, int x, int y, int skill_level);
const char *get_subtype_name(int category, int subtype);
const char *get_subtype_description(int category, int subtype);
const char *get_quality_name(int quality);
float get_quality_multiplier(int quality);

/* Material storage functions */
void add_harvested_material(struct char_data *ch, int category, int subtype, int quality, int amount);
int get_material_count(struct char_data *ch, int category, int subtype, int quality);
bool has_material_requirement(struct char_data *ch, int category, int subtype, int min_quality, int amount);
void remove_material_requirement(struct char_data *ch, int category, int subtype, int min_quality, int amount);

/* Display functions */
void show_materials_by_category(struct char_data *ch, int category);
void show_all_materials(struct char_data *ch);
void show_material_detail(struct char_data *ch, int category, int subtype, int quality);

/* Initialization */
void init_material_system(void);

#endif /* _MATERIAL_TYPES_H_ */
