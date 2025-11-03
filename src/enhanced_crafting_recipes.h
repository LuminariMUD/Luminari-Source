/**
 * @file enhanced_crafting_recipes.h
 * @brief Enhanced crafting recipes for LuminariMUD that utilize wilderness materials
 * 
 * This system provides an enhanced crafting recipe system that works directly
 * with the wilderness material hierarchy, preserving the rich material system
 * while providing advanced crafting capabilities.
 * 
 * Only active for LuminariMUD campaign (when other campaigns are not defined)
 */

#ifndef ENHANCED_CRAFTING_RECIPES_H
#define ENHANCED_CRAFTING_RECIPES_H

#include "campaign.h"

#ifdef ENABLE_WILDERNESS_CRAFTING_INTEGRATION

/**
 * Enhanced recipe structure that works with wilderness materials
 */
struct enhanced_recipe_data {
    const char *name;                    // Recipe name (e.g., "Mithril Blade")
    const char *description;             // Recipe description
    int object_type;                     // ITEM_WEAPON, ITEM_ARMOR, etc.
    int object_subtype;                  // Weapon type, armor type, etc.
    
    /* Wilderness material requirements */
    struct {
        int category;                    // RESOURCE_HERBS, RESOURCE_CRYSTAL, etc.
        int subtype;                     // Specific material type (-1 for any)
        int min_quality;                 // Minimum quality required
        int quantity;                    // Amount needed
        bool optional;                   // Is this material optional?
    } materials[5];                      // Up to 5 material requirements
    
    /* Crafting requirements */
    int skill_type;                      // Required crafting skill
    int min_skill_level;                 // Minimum skill level
    int base_difficulty;                 // Base DC for crafting check
    int crafting_time;                   // Time in seconds to craft
    
    /* Enhanced features */
    bool preserves_quality;              // Does final item inherit material quality?
    bool combines_qualities;             // Does it average material qualities?
    int quality_bonus;                   // Fixed quality bonus to final item
    int level_adjustment;                // Level adjustment for final item
    
    /* Special properties */
    const char *special_desc;            // Special item description additions
    int special_flags;                   // Special item flags
};

/**
 * Enhanced recipe categories
 */
enum enhanced_recipe_types {
    ENHANCED_RECIPE_NONE = 0,
    
    /* Weapon categories */
    ENHANCED_RECIPE_CRYSTAL_WEAPONS,     // Crystal-enhanced weapons
    ENHANCED_RECIPE_HERB_WEAPONS,        // Poison/magical herb weapons
    ENHANCED_RECIPE_ELEMENTAL_WEAPONS,   // Fire/ice/lightning weapons
    
    /* Armor categories */
    ENHANCED_RECIPE_HIDE_ARMOR,          // Enhanced hide armor
    ENHANCED_RECIPE_CRYSTAL_ARMOR,       // Crystal-enhanced armor
    ENHANCED_RECIPE_ELEMENTAL_ARMOR,     // Elemental protection armor
    
    /* Tools and accessories */
    ENHANCED_RECIPE_HERB_TOOLS,          // Alchemy tools and containers
    ENHANCED_RECIPE_CRYSTAL_TOOLS,       // Magical implements
    ENHANCED_RECIPE_SURVIVAL_GEAR,       // Wilderness survival equipment
    
    /* Special items */
    ENHANCED_RECIPE_POTIONS,             // Herbal potions and salves
    ENHANCED_RECIPE_ENCHANTMENTS,        // Crystal enchantments
    ENHANCED_RECIPE_ARTIFACTS,           // Legendary combined materials
    
    NUM_ENHANCED_RECIPE_TYPES
};

/* Enhanced recipe constants */
#define MAX_ENHANCED_RECIPES 200
#define ENHANCED_RECIPE_BASE_DIFFICULTY 15

/* Function prototypes */
void initialize_enhanced_recipes(void);
void populate_enhanced_recipes(void);
struct enhanced_recipe_data *get_enhanced_recipe(int recipe_id);
bool can_craft_enhanced_recipe(struct char_data *ch, int recipe_id);
int check_enhanced_recipe_materials(struct char_data *ch, int recipe_id);
int craft_enhanced_item(struct char_data *ch, int recipe_id);
void show_enhanced_recipes(struct char_data *ch, int category);
void show_enhanced_recipe_details(struct char_data *ch, int recipe_id);

/* Enhanced recipe database */
extern struct enhanced_recipe_data enhanced_recipes[MAX_ENHANCED_RECIPES];

#endif /* ENABLE_WILDERNESS_CRAFTING_INTEGRATION */

#endif /* ENHANCED_CRAFTING_RECIPES_H */
