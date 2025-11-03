/**
 * @file wilderness_crafting_bridge.c
 * @brief Bridge between wilderness materials and crafting system
 * 
 * This system provides integration between the Phase 4.5 wilderness
 * material system and the existing newcrafting system, making them
 * compatible and campaign-aware.
 */

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "wilderness_crafting_bridge.h"
#include "resource_system.h"
#include "crafting_new.h"
#include "campaign.h"
#include "utils.h"
#include "comm.h"
#include "constants.h"

/* Campaign-specific material name arrays */
#ifdef CAMPAIGN_DL
const char *dl_herb_names[NUM_HERB_SUBTYPES] = {
  "kingsfern", "dragongrass", "vallenwood leaf", "krynn sage", 
  "moonbell", "silverleaf", "wyrmroot", "dracoflower"
};

const char *dl_crystal_names[NUM_CRYSTAL_SUBTYPES] = {
  "dragonmetal ore", "astral crystal", "solamnic stone", "godsgem",
  "knightstone", "darksteel ore", "whitestone", "dragonstone"
};

const char *dl_wood_names[NUM_WOOD_SUBTYPES] = {
  "vallenwood", "ironwood", "silverbirch", "dragonwood",
  "knightwood", "astralwood", "shadowbark", "brightoak"
};

const char *dl_game_names[NUM_GAME_SUBTYPES] = {
  "draconian hide", "kender fur", "minotaur leather", "centaur hide",
  "pegasus feather", "griffin plume", "dragon scale", "wyrmling skin"
};

const char *dl_vegetation_names[NUM_VEGETATION_SUBTYPES] = {
  "vallenwood moss", "krynn lichen", "astral fungus", "moon vine",
  "star blossom", "dragon ivy", "knight grass", "sacred moss"
};

const char *dl_stone_names[NUM_STONE_SUBTYPES] = {
  "krynn granite", "solamnic marble", "astral stone", "dragonrock",
  "knight stone", "moon marble", "star granite", "sacred stone"
};

const char *dl_mineral_names[NUM_MINERAL_SUBTYPES] = {
  "krynn iron", "dragonmetal", "astral silver", "knight steel",
  "solamnic gold", "moon silver", "star metal", "sacred ore"
};
#endif

#ifdef CAMPAIGN_FR
const char *fr_herb_names[NUM_HERB_SUBTYPES] = {
  "silverleaf", "moonwort", "blueleaf", "tansy",
  "feverfew", "goldenseal", "nightshade", "bloodgrass"
};

const char *fr_crystal_names[NUM_CRYSTAL_SUBTYPES] = {
  "mithril ore", "adamantine", "deep crystal", "star sapphire",
  "moonstone", "sunstone", "shadowgem", "voidcrystal"
};

const char *fr_wood_names[NUM_WOOD_SUBTYPES] = {
  "shadowtop", "blueleaf wood", "weirwood", "duskwood",
  "silverbirch", "goldenbark", "ironwood", "brightwood"
};

const char *fr_game_names[NUM_GAME_SUBTYPES] = {
  "rothé hide", "stirge leather", "owlbear fur", "displacer hide",
  "pegasus feather", "griffin plume", "dragon scale", "bulette hide"
};

const char *fr_vegetation_names[NUM_VEGETATION_SUBTYPES] = {
  "sword coast moss", "cormyr lichen", "underdark fungus", "moon vine",
  "weave flower", "magic moss", "fey grass", "astral bloom"
};

const char *fr_stone_names[NUM_STONE_SUBTYPES] = {
  "waterdeep stone", "cormyr marble", "underdark rock", "dragonstone",
  "moonstone", "sunstone", "shadowstone", "voidstone"
};

const char *fr_mineral_names[NUM_MINERAL_SUBTYPES] = {
  "sword coast iron", "mithril", "adamantine", "deep silver",
  "cormyr gold", "moon silver", "star metal", "voidmetal"
};
#endif

/* Default LuminariMUD material names */
const char *default_herb_names[NUM_HERB_SUBTYPES] = {
  "marjoram", "kingfoil", "starlily", "wolfsbane",
  "nightshade", "moonbell", "silverleaf", "dragonherb"
};

const char *default_crystal_names[NUM_CRYSTAL_SUBTYPES] = {
  "arcanite", "nethermote", "sunstone", "voidshards",
  "starcrystal", "moonstone", "shadowgem", "brightstone"
};

const char *default_wood_names[NUM_WOOD_SUBTYPES] = {
  "ironwood", "silverbirch", "shadowbark", "brightoak",
  "moonwood", "starwood", "voidwood", "crystalwood"
};

const char *default_game_names[NUM_GAME_SUBTYPES] = {
  "wolf hide", "bear fur", "deer leather", "boar hide",
  "eagle feather", "hawk plume", "dragon scale", "wyrm skin"
};

const char *default_vegetation_names[NUM_VEGETATION_SUBTYPES] = {
  "moss", "lichen", "fungus", "vine",
  "flower", "grass", "fern", "bloom"
};

const char *default_stone_names[NUM_STONE_SUBTYPES] = {
  "granite", "marble", "limestone", "obsidian",
  "quartz", "slate", "sandstone", "basalt"
};

const char *default_mineral_names[NUM_MINERAL_SUBTYPES] = {
  "iron ore", "copper ore", "silver ore", "gold ore",
  "platinum ore", "mithril ore", "adamantine", "star metal"
};

/**
 * Convert wilderness material category to crafting material group
 */
int get_crafting_group_for_wilderness_material(int category, int subtype, int quality) {
  switch (category) {
    case RESOURCE_HERBS:
    case RESOURCE_VEGETATION:
      return CRAFT_GROUP_CLOTH;  // Organic materials for cloth-like uses
      
    case RESOURCE_CRYSTAL:
    case RESOURCE_MINERALS:
      // Quality affects metal categorization
      if (quality >= 4) return CRAFT_GROUP_HARD_METALS;  // Rare/Legendary → hard metals
      else return CRAFT_GROUP_SOFT_METALS;  // Common metals → soft metals
      
    case RESOURCE_WOOD:
      return CRAFT_GROUP_WOOD;
      
    case RESOURCE_GAME:
      return CRAFT_GROUP_ANIMAL_HIDES;
      
    case RESOURCE_STONE:
      return CRAFT_GROUP_HARD_METALS;  // Stone works like hard metals for construction
      
    default:
      return CRAFT_GROUP_REFINING;
  }
}

/**
 * Convert wilderness material to specific crafting material ID
 */
int wilderness_to_crafting_material(int category, int subtype, int quality) {
  // For now, map to base material groups
  // This could be expanded to map to specific material IDs
  int group = get_crafting_group_for_wilderness_material(category, subtype, quality);
  
  // Return base material for the group
  // This is a simplified mapping - could be more sophisticated
  switch (group) {
    case CRAFT_GROUP_HARD_METALS: return CRAFT_MAT_HARDMETALS;
    case CRAFT_GROUP_SOFT_METALS: return CRAFT_MAT_SOFTMETALS;
    case CRAFT_GROUP_ANIMAL_HIDES: return CRAFT_MAT_ANIMALHIDES;
    case CRAFT_GROUP_WOOD: return CRAFT_MAT_WOOD;
    case CRAFT_GROUP_CLOTH: return CRAFT_MAT_CLOTH;
    default: return CRAFT_MAT_REFINING;
  }
}

/**
 * Get campaign-appropriate material name
 */
const char *get_campaign_material_name(int category, int subtype, int quality) {
  const char **name_array = NULL;
  
  // Validate subtype range
  if (subtype < 0 || subtype >= NUM_SUBTYPES_PER_CATEGORY) {
    return "unknown material";
  }
  
  // Select appropriate name array based on campaign and category
  switch (category) {
    case RESOURCE_HERBS:
#ifdef CAMPAIGN_DL
      name_array = dl_herb_names;
#elif defined(CAMPAIGN_FR)
      name_array = fr_herb_names;
#else
      name_array = default_herb_names;
#endif
      break;
      
    case RESOURCE_CRYSTAL:
#ifdef CAMPAIGN_DL
      name_array = dl_crystal_names;
#elif defined(CAMPAIGN_FR)
      name_array = fr_crystal_names;
#else
      name_array = default_crystal_names;
#endif
      break;
      
    case RESOURCE_WOOD:
#ifdef CAMPAIGN_DL
      name_array = dl_wood_names;
#elif defined(CAMPAIGN_FR)
      name_array = fr_wood_names;
#else
      name_array = default_wood_names;
#endif
      break;
      
    case RESOURCE_GAME:
#ifdef CAMPAIGN_DL
      name_array = dl_game_names;
#elif defined(CAMPAIGN_FR)
      name_array = fr_game_names;
#else
      name_array = default_game_names;
#endif
      break;
      
    case RESOURCE_VEGETATION:
#ifdef CAMPAIGN_DL
      name_array = dl_vegetation_names;
#elif defined(CAMPAIGN_FR)
      name_array = fr_vegetation_names;
#else
      name_array = default_vegetation_names;
#endif
      break;
      
    case RESOURCE_STONE:
#ifdef CAMPAIGN_DL
      name_array = dl_stone_names;
#elif defined(CAMPAIGN_FR)
      name_array = fr_stone_names;
#else
      name_array = default_stone_names;
#endif
      break;
      
    case RESOURCE_MINERALS:
#ifdef CAMPAIGN_DL
      name_array = dl_mineral_names;
#elif defined(CAMPAIGN_FR)
      name_array = fr_mineral_names;
#else
      name_array = default_mineral_names;
#endif
      break;
      
    default:
      return "unknown material";
  }
  
  return name_array[subtype];
}

/**
 * Check if wilderness material can be used in crafting
 */
int is_wilderness_material_craftable(int category, int subtype, int quality) {
  // All valid wilderness materials are craftable
  if (category >= 0 && category < NUM_RESOURCE_TYPES &&
      subtype >= 0 && subtype < NUM_SUBTYPES_PER_CATEGORY &&
      quality >= 1 && quality <= 5) {
    return TRUE;
  }
  return FALSE;
}

/**
 * Get effective crafting value of wilderness material
 */
float get_wilderness_crafting_value(int category, int subtype, int quality) {
  // Quality affects crafting value
  switch (quality) {
    case 1: return 0.5f;  // Poor quality - half value
    case 2: return 1.0f;  // Common - standard value
    case 3: return 1.5f;  // Uncommon - 50% bonus
    case 4: return 2.0f;  // Rare - double value
    case 5: return 3.0f;  // Legendary - triple value
    default: return 1.0f;
  }
}

/**
 * Convert wilderness materials from player storage to crafting materials
 */
int convert_wilderness_to_crafting(struct char_data *ch, int category, int subtype, int quality, int quantity) {
  int converted = 0;
  
  if (!ch || IS_NPC(ch)) {
    return 0;
  }
  
  // For now, just report what would be converted
  // This would need integration with the actual crafting material storage system
  
  // TODO: Implement actual conversion to crafting materials
  // This requires knowledge of how crafting materials are stored and managed
  
  return converted;
}

/**
 * Display crafting materials inventory
 */
void show_crafting_materials_display(struct char_data *ch) {
    int i;
    bool found_any = FALSE;
    
    send_to_char(ch, "Your crafting materials storage:\r\n");
    send_to_char(ch, "================================\r\n");
    
    for (i = 1; i < NUM_CRAFT_MATS; i++) {
        if (GET_CRAFT_MAT(ch, i) > 0) {
            send_to_char(ch, "  %s: %d unit%s\r\n", 
                        crafting_materials[i], 
                        GET_CRAFT_MAT(ch, i),
                        GET_CRAFT_MAT(ch, i) == 1 ? "" : "s");
            found_any = TRUE;
        }
    }
    
    if (!found_any) {
        send_to_char(ch, "  No crafting materials stored.\r\n");
    }
}

#ifdef ENABLE_WILDERNESS_CRAFTING_INTEGRATION
/* Enhanced LuminariMUD-only integration functions */

/**
 * Get enhanced crafting group for wilderness material (preserves hierarchy)
 */
int get_enhanced_crafting_group_for_wilderness_material(int category, int subtype, int quality) {
    /* Map wilderness materials to enhanced crafting groups based on their nature */
    switch (category) {
        case RESOURCE_HERBS:
            return ENHANCED_CRAFT_GROUP_MAGICAL_HERBS;
            
        case RESOURCE_CRYSTAL:
            return ENHANCED_CRAFT_GROUP_CRYSTALS;
            
        case RESOURCE_MINERALS:
            /* Quality determines if it's rare or common metal */
            if (quality >= 4) {
                return ENHANCED_CRAFT_GROUP_RARE_METALS;
            } else {
                return ENHANCED_CRAFT_GROUP_COMMON_METALS;
            }
            
        case RESOURCE_WOOD:
            /* Subtype determines magical vs common wood */
            if (subtype >= 4) {  /* Magical woods: thornvine, moonweave, darkwillow, starwood */
                return ENHANCED_CRAFT_GROUP_EXOTIC_WOODS;
            } else {
                return ENHANCED_CRAFT_GROUP_COMMON_WOODS;
            }
            
        case RESOURCE_GAME:
            /* Quality and subtype determine hide type */
            if (subtype >= 4 || quality >= 4) {  /* Magical creature hides */
                return ENHANCED_CRAFT_GROUP_MAGICAL_HIDES;
            } else {
                return ENHANCED_CRAFT_GROUP_COMMON_HIDES;
            }
            
        case RESOURCE_STONE:
            /* Subtype determines magical vs building stone */
            if (subtype >= 3) {  /* Magical stones: moonstone, dragonbone, voidrock, etc. */
                return ENHANCED_CRAFT_GROUP_MAGICAL_STONES;
            } else {
                return ENHANCED_CRAFT_GROUP_BUILDING_STONES;
            }
            
        case RESOURCE_VEGETATION:
            /* Special vegetation types for different purposes */
            if (subtype == 3 || subtype == 6) {  /* Spirit grass, shadow fern */
                return ENHANCED_CRAFT_GROUP_SPIRIT_MATERIALS;
            } else if (subtype == 4 || subtype == 5 || subtype == 7) {  /* Flame flower, ice lichen, light bloom */
                return ENHANCED_CRAFT_GROUP_ELEMENTAL_MATERIALS;
            } else {
                return ENHANCED_CRAFT_GROUP_COMMON_HIDES;  /* Fiber materials for cloth */
            }
            
        default:
            return ENHANCED_CRAFT_GROUP_NONE;
    }
}

/**
 * Enhanced wilderness material integration that preserves hierarchy
 */
int integrate_wilderness_material_enhanced(struct char_data *ch, int category, int subtype, int quality, int quantity) {
    if (!ch || IS_NPC(ch) || quantity <= 0) {
        return 0;
    }
    
    /* Store in wilderness system (preserves full hierarchy) */
    int stored = add_material_to_storage(ch, category, subtype, quality, quantity);
    
    if (stored > 0) {
        const char *material_name = get_full_material_name(category, subtype, quality);
        send_to_char(ch, "You harvest %d %s for use in both wilderness storage and crafting.\r\n", 
                     stored, material_name);
    }
    
    return stored;
}

/**
 * Show enhanced materials display (wilderness + crafting unified)
 */
void show_enhanced_materials_display(struct char_data *ch) {
    send_to_char(ch, "\\cG=== LuminariMUD Enhanced Material Storage ===\\cn\r\n\r\n");
    
    send_to_char(ch, "\\cWWilderness Materials (Full Hierarchy):\\cn\r\n");
    show_material_storage(ch);
    
    send_to_char(ch, "\r\n\\cYCrafting Integration Status:\\cn\r\n");
    send_to_char(ch, "Your wilderness materials are automatically available for enhanced crafting recipes.\r\n");
    send_to_char(ch, "Material quality affects crafting effectiveness and final item properties.\r\n");
    
    send_to_char(ch, "\r\n\\cCLegacy Crafting Materials:\\cn\r\n");
    show_crafting_materials_display(ch);
    
    send_to_char(ch, "\r\n\\cMUsage:\\cn materials wilderness, materials crafting, materials enhanced\r\n");
}

/**
 * Get wilderness material effectiveness for crafting (quality-based)
 */
int get_wilderness_material_crafting_effectiveness(int category, int subtype, int quality) {
    /* Quality 1-5 maps to effectiveness multipliers */
    int base_effectiveness = 100;  /* 100% base effectiveness */
    
    switch (quality) {
        case 1: /* Poor */
            return base_effectiveness + 0;   /* 100% - standard effectiveness */
        case 2: /* Common */
            return base_effectiveness + 25;  /* 125% - slight bonus */
        case 3: /* Uncommon */
            return base_effectiveness + 50;  /* 150% - good bonus */
        case 4: /* Rare */
            return base_effectiveness + 100; /* 200% - excellent bonus */
        case 5: /* Legendary */
            return base_effectiveness + 200; /* 300% - legendary bonus */
        default:
            return base_effectiveness;
    }
}

#endif /* ENABLE_WILDERNESS_CRAFTING_INTEGRATION */
