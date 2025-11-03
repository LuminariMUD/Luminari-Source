/**
 * @file wilderness_crafting_bridge.h
 * @brief Bridge between wilderness materials and crafting system
 * 
 * This system provides integration between the Phase 4.5 wilderness
 * material system and the existing newcrafting system, making them
 * compatible and campaign-aware.
 */

#ifndef WILDERNESS_CRAFTING_BRIDGE_H
#define WILDERNESS_CRAFTING_BRIDGE_H

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "campaign.h"

/* Campaign-safe wilderness-crafting integration */

#ifdef ENABLE_WILDERNESS_CRAFTING_INTEGRATION
/* Only compile enhanced integration features for LuminariMUD campaign */

/**
 * Enhanced material storage that preserves wilderness hierarchy
 */
struct enhanced_material_storage {
    int category;           // RESOURCE_HERBS, RESOURCE_CRYSTAL, etc.
    int subtype;           // Specific material type within category
    int quality;           // 1-5 quality level
    int quantity;          // Amount stored
    const char *display_name;  // Campaign-specific display name
};

/**
 * Enhanced crafting groups that work with wilderness materials
 */
enum enhanced_craft_groups {
    ENHANCED_CRAFT_GROUP_NONE = 0,
    ENHANCED_CRAFT_GROUP_MAGICAL_HERBS,      // Herbs for alchemy/magic
    ENHANCED_CRAFT_GROUP_CRYSTALS,           // Magical crystals
    ENHANCED_CRAFT_GROUP_RARE_METALS,        // Mithril, adamantine, etc.
    ENHANCED_CRAFT_GROUP_COMMON_METALS,      // Iron, steel, etc.
    ENHANCED_CRAFT_GROUP_EXOTIC_WOODS,       // Ironwood, silverbirch, etc.
    ENHANCED_CRAFT_GROUP_COMMON_WOODS,       // Oak, pine, etc.
    ENHANCED_CRAFT_GROUP_MAGICAL_HIDES,      // Dragon hide, shadow pelt
    ENHANCED_CRAFT_GROUP_COMMON_HIDES,       // Leather, fur
    ENHANCED_CRAFT_GROUP_MAGICAL_STONES,     // Moonstone, dragonbone
    ENHANCED_CRAFT_GROUP_BUILDING_STONES,    // Granite, marble
    ENHANCED_CRAFT_GROUP_SPIRIT_MATERIALS,   // Ethereal components
    ENHANCED_CRAFT_GROUP_ELEMENTAL_MATERIALS,// Fire/ice/lightning components
    NUM_ENHANCED_CRAFT_GROUPS
};

#endif /* ENABLE_WILDERNESS_CRAFTING_INTEGRATION */

/* Core functions available to all campaigns */

/**
 * Convert wilderness material category to crafting material group
 * @param category Wilderness resource category (RESOURCE_*)
 * @param subtype Material subtype (0-7)
 * @param quality Material quality (1-5) 
 * @return Crafting material group (CRAFT_GROUP_*)
 */
int get_crafting_group_for_wilderness_material(int category, int subtype, int quality);

/**
 * Convert wilderness material to specific crafting material
 * @param category Wilderness resource category
 * @param subtype Material subtype
 * @param quality Material quality
 * @return Specific crafting material ID
 */
int wilderness_to_crafting_material(int category, int subtype, int quality);

/**
 * Convert wilderness materials from player storage to crafting materials
 * @param ch Character to convert materials for
 * @param category Wilderness category to convert (-1 for all)
 * @param subtype Subtype to convert (-1 for all in category)
 * @param quality Quality to convert (-1 for all)
 * @param quantity Max quantity to convert (-1 for all)
 * @return Total number of materials converted
 */
int convert_wilderness_to_crafting(struct char_data *ch, int category, int subtype, int quality, int quantity);

/**
 * Get campaign-appropriate material name
 * @param category Wilderness resource category
 * @param subtype Material subtype
 * @param quality Material quality
 * @return String pointer to material name
 */
const char *get_campaign_material_name(int category, int subtype, int quality);

/**
 * Check if wilderness material can be used in crafting
 * @param category Wilderness resource category
 * @param subtype Material subtype
 * @param quality Material quality
 * @return TRUE if usable in crafting, FALSE otherwise
 */
int is_wilderness_material_craftable(int category, int subtype, int quality);

/**
 * Get effective crafting value of wilderness material
 * @param category Wilderness resource category
 * @param subtype Material subtype
 * @param quality Material quality
 * @return Effective value multiplier for crafting
 */
float get_wilderness_crafting_value(int category, int subtype, int quality);

/* Campaign-specific material arrays */
#ifdef CAMPAIGN_DL
extern const char *dl_herb_names[];
extern const char *dl_crystal_names[];
extern const char *dl_wood_names[];
extern const char *dl_game_names[];
extern const char *dl_vegetation_names[];
extern const char *dl_stone_names[];
extern const char *dl_mineral_names[];
#endif

#ifdef CAMPAIGN_FR
extern const char *fr_herb_names[];
extern const char *fr_crystal_names[];
extern const char *fr_wood_names[];
extern const char *fr_game_names[];
extern const char *fr_vegetation_names[];
extern const char *fr_stone_names[];
extern const char *fr_mineral_names[];
#endif

/* Default LuminariMUD names (when no campaign defined) */
extern const char *default_herb_names[];
extern const char *default_crystal_names[];
extern const char *default_wood_names[];
extern const char *default_game_names[];
extern const char *default_vegetation_names[];
extern const char *default_stone_names[];
extern const char *default_mineral_names[];

/**
 * Convert wilderness materials from player storage to crafting materials
 */
int convert_wilderness_to_crafting(struct char_data *ch, int category, int subtype, int quality, int quantity);

/**
 * Display crafting materials inventory
 */
void show_crafting_materials_display(struct char_data *ch);

#ifdef ENABLE_WILDERNESS_CRAFTING_INTEGRATION
/* Enhanced LuminariMUD-only functions */

/**
 * Get enhanced crafting group for wilderness material (preserves hierarchy)
 */
int get_enhanced_crafting_group_for_wilderness_material(int category, int subtype, int quality);

/**
 * Enhanced wilderness material to enhanced crafting integration
 */
int integrate_wilderness_material_enhanced(struct char_data *ch, int category, int subtype, int quality, int quantity);

/**
 * Show enhanced materials display (wilderness + crafting unified)
 */
void show_enhanced_materials_display(struct char_data *ch);

/**
 * Get wilderness material effectiveness for crafting (quality-based)
 */
int get_wilderness_material_crafting_effectiveness(int category, int subtype, int quality);

#endif /* ENABLE_WILDERNESS_CRAFTING_INTEGRATION */

#endif /* WILDERNESS_CRAFTING_BRIDGE_H */
