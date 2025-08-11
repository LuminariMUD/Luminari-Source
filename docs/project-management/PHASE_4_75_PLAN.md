# Phase 4.75: Campaign-Safe Wilderness Materials Integration

## Overview

Phase 4.75 implements wilderness-crafting integration using a campaign-safe approach that preserves existing DragonLance and Forgotten Realms crafting systems while providing full enhancement for the default LuminariMUD campaign.

## Campaign-Safe Architecture

### Implementation Strategy
- **DragonLance/Forgotten Realms**: Wilderness materials enabled, crafting integration disabled
- **LuminariMUD (Default)**: Full wilderness-crafting integration with enhanced recipes  
- **Preservation**: Existing newcrafting systems remain completely unchanged for DL/FR
- **Enhancement**: LuminariMUD gets advanced material system with preserved hierarchy

## Current System Analysis

### Existing Crafting System
- **Material Groups**: 8 categories (hard metals, soft metals, animal hides, wood, cloth, refining, resizing)
- **Material Items**: Individual crafting materials (defined in constants.c)
- **Campaign Specific**: Some DL-specific features in craft.c, newcrafting is campaign-agnostic

### Phase 4.5 Wilderness System
- **Resource Categories**: 7 types (herbs, crystal, minerals, wood, vegetation, stone, game)  
- **Three-Tier Hierarchy**: Category → Subtype (0-7) → Quality (1-5)
- **280 Total Materials**: Comprehensive material system
- **Campaign-Aware**: Material names vary by campaign

## Campaign-Safe Goals

1. **Campaign Safety**: Zero impact on existing DL/FR crafting systems
2. **LuminariMUD Enhancement**: Full wilderness-crafting integration for default campaign
3. **Material Hierarchy Preservation**: Keep rich wilderness material structure
4. **Enhanced Recipes**: New recipe system that works directly with wilderness materials
5. **Quality Integration**: Material quality affects crafting outcomes and final items

## Implementation Plan

### Step 1: Campaign-Aware Material Definitions

**File: `src/resource_system.c`**
```c
// Add campaign-specific material name arrays
#if defined(CAMPAIGN_DL)
  static const char *dl_herb_names[NUM_HERB_SUBTYPES] = {
    "kingsfern", "dragongrass", "vallenwood leaf", "krynn sage", 
    "moonbell", "silverleaf", "wyrmroot", "dracoflower"
  };
  static const char *dl_crystal_names[NUM_CRYSTAL_SUBTYPES] = {
    "dragonmetal ore", "astral crystal", "solamnic stone", "godsgem",
    "knightstone", "darksteel ore", "whitestone", "dragonstone"
  };
#elif defined(CAMPAIGN_FR)
  static const char *fr_herb_names[NUM_HERB_SUBTYPES] = {
    "silverleaf", "moonwort", "blueleaf", "tansy",
    "feverfew", "goldenseal", "nightshade", "bloodgrass"
  };
  static const char *fr_crystal_names[NUM_CRYSTAL_SUBTYPES] = {
    "mithril ore", "adamantine", "deep crystal", "star sapphire",
    "moonstone", "sunstone", "shadowgem", "voidcrystal"
  };
#else
  // Default LuminariMUD materials (existing names)
  static const char *default_herb_names[NUM_HERB_SUBTYPES] = {
    "marjoram", "kingfoil", "starlily", "wolfsbane",
    "nightshade", "moonbell", "silverleaf", "dragonherb"
  };
  static const char *default_crystal_names[NUM_CRYSTAL_SUBTYPES] = {
    "arcanite", "nethermote", "sunstone", "voidshards",
    "starcrystal", "moonstone", "shadowgem", "brightstone"
  };
#endif
```

### Step 2: Wilderness-to-Crafting Material Bridge

**File: `src/wilderness_crafting_bridge.c` (NEW)**
```c
// Bridge wilderness materials to crafting material groups
int get_crafting_group_for_wilderness_material(int category, int subtype, int quality) {
  switch (category) {
    case RESOURCE_HERBS:
    case RESOURCE_VEGETATION:
      return CRAFT_GROUP_CLOTH;  // Organic materials
      
    case RESOURCE_CRYSTAL:
    case RESOURCE_MINERALS:
      if (quality >= 4) return CRAFT_GROUP_HARD_METALS;  // Rare/Legendary
      else return CRAFT_GROUP_SOFT_METALS;  // Common metals
      
    case RESOURCE_WOOD:
      return CRAFT_GROUP_WOOD;
      
    case RESOURCE_GAME:
      return CRAFT_GROUP_ANIMAL_HIDES;
      
    case RESOURCE_STONE:
      return CRAFT_GROUP_HARD_METALS;  // Stone works like hard metals
      
    default:
      return CRAFT_GROUP_REFINING;
  }
}

// Convert wilderness material to crafting material
int wilderness_to_crafting_material(int category, int subtype, int quality) {
  // Campaign-specific material mapping
#if defined(CAMPAIGN_DL)
  return get_dl_crafting_material(category, subtype, quality);
#elif defined(CAMPAIGN_FR) 
  return get_fr_crafting_material(category, subtype, quality);
#else
  return get_default_crafting_material(category, subtype, quality);
#endif
}
```

### Step 3: Enhanced Material Admin Commands

**File: `src/act.wizard.c`**
```c
// Enhanced materialadmin with crafting integration
ACMD(do_materialadmin) {
  // Add new subcommands:
  // materialadmin convert <player> <wilderness_category> <subtype> <quality> <quantity>
  // materialadmin list <player> crafting  // Show crafting-compatible materials
  // materialadmin bridge <player> all     // Convert all wilderness to crafting
}
```

### Step 4: Crafting Recipe Extensions

**File: `src/crafting_recipes_wilderness.h` (NEW)**
```c
// Campaign-specific wilderness recipes
#if !defined(CAMPAIGN_DL) && !defined(CAMPAIGN_FR)
// Default LuminariMUD wilderness recipes
static const struct crafting_recipe wilderness_recipes[] = {
  // Herb-based items
  { CRAFT_RECIPE_POTION_HEALING, "healing potion", 
    {{WILDERNESS_HERB_MATERIAL(0, 2), 3}, {0, 0}, {0, 0}} },
    
  // Crystal-enhanced weapons  
  { CRAFT_RECIPE_WEAPON_CRYSTAL_SWORD, "crystal sword",
    {{CRAFT_MAT_HARD_METALS, 5}, {WILDERNESS_CRYSTAL_MATERIAL(0, 4), 2}, {0, 0}} },
    
  // Wood items
  { CRAFT_RECIPE_WEAPON_WILDERNESS_BOW, "wilderness bow",
    {{WILDERNESS_WOOD_MATERIAL(0, 3), 4}, {CRAFT_MAT_CLOTH, 2}, {0, 0}} }
};
#endif
```

### Step 5: Campaign Configuration Updates

**File: `src/campaign.h`**
```c
// Add wilderness material campaign settings
#if defined(CAMPAIGN_DL)
  #define ENABLE_WILDERNESS_MATERIALS 1
  #define WILDERNESS_MATERIAL_THEME "dragonlance"
  #define WILDERNESS_ENHANCED_RECIPES 1
  
#elif defined(CAMPAIGN_FR)
  #define ENABLE_WILDERNESS_MATERIALS 1  
  #define WILDERNESS_MATERIAL_THEME "forgotten_realms"
  #define WILDERNESS_ENHANCED_RECIPES 1
  
#else
  // Default LuminariMUD - always enable
  #define ENABLE_WILDERNESS_MATERIALS 1
  #define WILDERNESS_MATERIAL_THEME "luminari" 
  #define WILDERNESS_ENHANCED_RECIPES 1
#endif
```

### Step 6: Integration with Existing Commands

**Modify: `src/crafting_new.c`**
```c
// Enhance 'craft materials' command to show wilderness materials
void show_available_materials(struct char_data *ch) {
  // Show traditional crafting materials
  show_crafting_materials(ch);
  
#if ENABLE_WILDERNESS_MATERIALS
  // Show wilderness material storage
  send_to_char(ch, "\r\n\tcWilderness Materials:\tn\r\n");
  show_material_storage(ch);
  
  // Show conversion options
  send_to_char(ch, "\r\n\tcConversion Available:\tn Use 'craft convert' to bridge materials.\r\n");
#endif
}

// New conversion command
ACMD(do_craft_convert) {
  // Convert wilderness materials to crafting materials for current project
}
```

## Campaign-Specific Material Themes

### CAMPAIGN_DL (DragonLance)
- **Herbs**: Vallenwood-themed plants, Krynn-specific flora
- **Crystals**: Dragonmetal ores, Solamnic stones, divine gems
- **Wood**: Vallenwood, ironwood, dragonwood varieties
- **Game**: Krynn-specific creatures (draconians, kender-hunted game)

### CAMPAIGN_FR (Forgotten Realms)  
- **Herbs**: Faerunian flora, magical plants from the Weave
- **Crystals**: Mithril, adamantine, Underdark crystals
- **Wood**: Shadowtop, blueleaf, weirwood varieties
- **Game**: Faerunian creatures, Sword Coast wildlife

### Default LuminariMUD
- **Maintain Current Names**: Existing Phase 4.5 material names
- **Generic Fantasy**: Broad appeal, non-campaign-specific

## Testing Strategy

### Phase 4.75 Testing Checklist

1. **Campaign Compilation**: All three campaigns compile successfully
2. **Material Compatibility**: Wilderness materials work in crafting recipes
3. **Admin Commands**: Enhanced materialadmin functions work
4. **Conversion System**: Wilderness-to-crafting bridge functions
5. **Backward Compatibility**: Existing crafting recipes still work
6. **Cross-Campaign**: Materials are campaign-appropriate

## File Structure

### New Files
- `src/wilderness_crafting_bridge.c` - Integration layer
- `src/wilderness_crafting_bridge.h` - Bridge headers  
- `src/crafting_recipes_wilderness.h` - Wilderness-specific recipes
- `docs/PHASE_4_75_TESTING.md` - Testing procedures

### Modified Files
- `src/resource_system.c` - Campaign-aware material names
- `src/act.wizard.c` - Enhanced materialadmin command
- `src/crafting_new.c` - Integration with wilderness materials
- `src/campaign.example.h` - Wilderness material settings
- `src/constants.c` - Campaign-specific material arrays

## Benefits

1. **Unified Economy**: Single coherent material system across all game systems
2. **Campaign Immersion**: Materials match campaign themes and lore
3. **Enhanced Crafting**: More material variety and options for crafters
4. **Admin Flexibility**: Tools to manage both wilderness and crafting materials
5. **Future-Proof**: Foundation for Phase 5 harvesting integration

## Timeline

- **Step 1-2**: Campaign material definitions and bridge system (2-3 hours)
- **Step 3-4**: Enhanced commands and recipe integration (2-3 hours)  
- **Step 5-6**: Campaign configuration and existing system integration (1-2 hours)
- **Testing**: Comprehensive testing across all campaigns (1-2 hours)

**Total Estimated Time**: 6-10 hours of development work

## Success Metrics

- ✅ All campaigns compile without errors
- ✅ Wilderness materials integrate seamlessly with crafting
- ✅ Campaign-appropriate material names and themes
- ✅ Enhanced admin tools for material management
- ✅ Backward compatibility maintained
- ✅ Foundation ready for Phase 5 harvesting system

---

**Phase 4.75 represents the bridge between wilderness gathering and crafting systems, providing a campaign-aware, unified material economy for LuminariMUD.**
