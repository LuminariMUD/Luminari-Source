# Phase 5: Player Harvesting Mechanics - Design Document

## 🎯 **Overview**

Phase 5 introduces interactive player harvesting mechanics that allow players to actively gather resources from the wilderness, creating a dynamic economy where player actions affect resource availability and regeneration.

## 🏗️ **Core Features**

### 1. Interactive Harvesting Commands

**Primary Commands:**
```bash
harvest [resource_type]              # Harvest specific resource at current location
harvest                              # Show available resources and harvest options
survey                               # Show detailed resource availability (enhanced)
gather [amount] [resource_type]      # Gather specific amount of resource
```

**Advanced Commands:**
```bash
harvest all                          # Attempt to harvest all available resources
harvest quick [resource_type]        # Fast harvest with reduced yield/success
harvest careful [resource_type]      # Slow harvest with increased yield/success
track resources                      # Show resource regeneration timers
```

### 2. Resource Consumption System

**Depletion Mechanics:**
- Each harvest action reduces local resource availability
- Resources become "depleted" when harvested below minimum thresholds
- Different resources have different depletion rates and recovery times
- Over-harvesting creates "barren" areas with penalties

**Dynamic Availability:**
```
Current System: Resources calculated on-demand, always same values
New System: Resources tracked with current availability vs maximum potential
```

### 3. Skill-Based Success Rates

**Primary Skills (Existing in Game):**
- **ABILITY_SURVIVAL** (ID: 29): Primary skill for wilderness harvesting
- **SKILL_MINING** (ID: 2071): Mineral extraction and stone gathering
- **ABILITY_HARVEST_MINING** (ID: 48): Specialized mining harvesting
- **ABILITY_HARVEST_HUNTING** (ID: 49): Game and animal resource gathering  
- **ABILITY_HARVEST_FORESTRY** (ID: 50): Wood and forest resource gathering
- **ABILITY_HARVEST_GATHERING** (ID: 51): General vegetation and plant gathering

**Integration with Existing Skills:**
- No new skills needed - utilize comprehensive existing skill system
- Skills already have progression, training, and class integration
- Existing `GET_SKILL(ch, skill_id)` and `skill_check()` functions ready to use

**Success Calculation:**
```c
base_success = 50 + (skill_level * 2)           // 50-150% base rate
resource_difficulty = resource_configs[type].difficulty   // Resource-specific difficulty
environmental_bonus = terrain_bonus + weather_bonus      // Environmental factors
tool_bonus = equipped_tool_bonus                         // Equipment bonuses

final_success = (base_success + environmental_bonus + tool_bonus) - resource_difficulty
```

### 4. Tool and Equipment Requirements

**Tool Categories:**
- **Basic Tools**: Hands, basic implements (50% efficiency)
- **Standard Tools**: Proper harvesting tools (100% efficiency) 
- **Masterwork Tools**: High-quality tools (150% efficiency)
- **Magical Tools**: Enchanted implements (200% efficiency + special bonuses)

**Resource-Specific Tools:**
```
Vegetation: Sickle, harvesting knife, gathering basket
Minerals: Pickaxe, mining hammer, prospector's tools
Herbs: Herbalism kit, gathering shears, preservation supplies
Wood: Axe, saw, woodcutting tools
Stone: Chisel, hammer, quarrying tools
Water: Containers, purification tablets, filtration kit
Game: Traps, hunting bow, tracking equipment
Crystal: Crystal picks, gem tools, magical focuses
Clay: Shovels, pottery tools, forming implements
Salt: Evaporation pans, scraping tools, refinement kit
```

### 5. Dynamic Resource Regeneration

**Regeneration Mechanics:**
- **Natural Recovery**: Resources slowly regenerate over time
- **Seasonal Cycles**: Regeneration rates vary by season
- **Environmental Factors**: Weather, region effects influence recovery
- **Player Impact**: Heavy harvesting areas recover more slowly

**Regeneration Formula:**
```c
regeneration_rate = base_rate * seasonal_multiplier * environmental_bonus * (1.0 - depletion_penalty)
current_amount = min(max_amount, current_amount + (regeneration_rate * time_elapsed))
```

**Time Scales:**
- Fast resources (vegetation, water): Hours to 1 day
- Medium resources (herbs, game): 1-3 days  
- Slow resources (minerals, crystal): 3-7 days
- Very slow (rare crystals, gems): 1-4 weeks

### 6. Harvesting Yields and Quality

**Yield Calculation:**
```c
base_yield = resource_configs[type].base_yield
skill_multiplier = 1.0 + (skill_level / 100.0)     // +100% at skill 100
tool_multiplier = tool_efficiency                    // 0.5 to 2.0
success_multiplier = (success_roll / 100.0)         // Partial success possible
region_multiplier = apply_region_effects()          // Existing system integration

final_yield = base_yield * skill_multiplier * tool_multiplier * success_multiplier * region_multiplier
```

**Quality Levels:**
- **Poor**: 50% value, basic crafting only
- **Common**: 100% value, standard crafting
- **Good**: 150% value, enhanced crafting bonuses
- **Excellent**: 200% value, rare crafting opportunities
- **Masterwork**: 300% value, legendary crafting potential

## ✅ Phase 4.5: Material Subtype System (COMPLETED)

**Status:** ✅ IMPLEMENTED - August 9, 2025  
The material subtype system has been successfully implemented and is ready for Phase 5 harvesting integration.

### Implementation Completed

**Three-Tier Material System - LIVE:**
- 7 resource categories with 8 subtypes each (56 total named materials)
- Quality levels: poor, common, uncommon, rare, legendary
- Player storage system supporting 100 material slots
- Commands: `materials` (player), `addmaterial` (admin)

**Named Materials Now Available:**
- **Herbs:** marjoram, kingfoil, starlily, wolfsbane, silverleaf, moonbell, thornweed, brightroot
- **Crystals:** arcanite, nethermote, sunstone, voidshards, dreamquartz, bloodstone, frostgem, stormcrystal  
- **Ores:** mithril, adamantine, cold iron, star steel, deep silver, dragonsteel, voidmetal, brightcopper
- **Wood:** ironwood, silverbirch, shadowbark, brightoak, thornvine, moonweave, darkwillow, starwood
- And more across vegetation, stone, and game categories

**Testing Commands Available:**
```bash
materials                                    # View stored materials
addmaterial <player> <category> <subtype> <quality> <qty>  # Admin: add materials
```

### Previous Design (Now Implemented)

**Tier 1: Resource Categories** (Already implemented in Phase 1-4)
```c
RESOURCE_VEGETATION = 0,    // General plant life
RESOURCE_MINERALS = 1,      // Ores and metals  
RESOURCE_WATER = 2,         // Water sources
RESOURCE_HERBS = 3,         // Medicinal plants
RESOURCE_GAME = 4,          // Huntable animals
RESOURCE_WOOD = 5,          // Timber resources
RESOURCE_STONE = 6,         // Building materials
RESOURCE_CRYSTAL = 7,       // Magical crystals
RESOURCE_CLAY = 8,          // Clay deposits
RESOURCE_SALT = 9           // Salt deposits
```

**Tier 2: Material Subtypes** (New - Phase 4.5)
```c
// Herbs (Luminari Lore-Specific)
HERB_MARJORAM = 0,          // Common cooking herb
HERB_KINGFOIL = 1,          // Healing herb (lore-important)  
HERB_STARLILY = 2,          // Magical reagent
HERB_BLOODMOSS = 3,         // Alchemy component
HERB_MOONBELL = 4,          // Night-blooming magical herb

// Crystals (Arcanite Important to Luminari Lore)
CRYSTAL_ARCANITE = 0,       // Major lore crystal - magical focus
CRYSTAL_QUARTZ = 1,         // Common crystal
CRYSTAL_AMETHYST = 2,       // Semi-precious
CRYSTAL_DIAMOND = 3,        // Rare precious stone
CRYSTAL_OBSIDIAN = 4,       // Volcanic glass

// Ores and Metals
ORE_IRON = 0,              // Common metal
ORE_COPPER = 1,            // Common metal
ORE_MITHRIL = 2,           // Rare magical metal
ORE_ADAMANTIUM = 3,        // Legendary metal
ORE_SILVER = 4,            // Precious metal
ORE_GOLD = 5,              // Precious metal

// Wood Types
WOOD_BIRCH = 0,            // Common light wood
WOOD_OAK = 1,              // Common strong wood
WOOD_IRONWOOD = 2,         // Rare magical wood
WOOD_YEWWOOD = 3,          // Bow-making wood
WOOD_ELDERWOOD = 4,        // Ancient magical wood

// And so on for other categories...
```

**Tier 3: Quality Levels** (Cross-cutting all subtypes)
```c
QUALITY_POOR = 0,          // 50% base value
QUALITY_COMMON = 1,        // 100% base value
QUALITY_GOOD = 2,          // 150% base value
QUALITY_EXCELLENT = 3,     // 200% base value
QUALITY_MASTERWORK = 4     // 300% base value
```

### Data Structure for Complete Materials

```c
struct harvested_material {
    int category;           // RESOURCE_HERBS, RESOURCE_MINERALS, etc.
    int subtype;           // HERB_KINGFOIL, CRYSTAL_ARCANITE, ORE_MITHRIL, etc.
    int quality;           // QUALITY_POOR to QUALITY_MASTERWORK
    int amount;            // Quantity harvested
    time_t harvested_at;   // When harvested
    int harvester_id;      // Who harvested it
};

// Examples:
// {RESOURCE_HERBS, HERB_KINGFOIL, QUALITY_EXCELLENT, 3, time, player_id}
// {RESOURCE_CRYSTALS, CRYSTAL_ARCANITE, QUALITY_MASTERWORK, 1, time, player_id}
// {RESOURCE_WOOD, WOOD_BIRCH, QUALITY_POOR, 5, time, player_id}
// {RESOURCE_WOOD, WOOD_IRONWOOD, QUALITY_GOOD, 2, time, player_id}
```

### Virtual Storage Extension

```c
// Extend player data to support subtype + quality storage
// Instead of just quality tiers, store by (subtype, quality) pairs

struct player_materials {
    // Format: materials[category][subtype][quality] = amount
    int wilderness_materials[NUM_RESOURCE_TYPES][MAX_SUBTYPES_PER_CATEGORY][NUM_QUALITY_LEVELS];
    
    // Quick access macros:
    // GET_MATERIAL(ch, category, subtype, quality)
    // ADD_MATERIAL(ch, category, subtype, quality, amount)
    // HAS_MATERIAL(ch, category, subtype, quality, min_amount)
};
```

### Harvesting Selection Logic

```c
// When harvesting, determine subtype based on:
struct subtype_spawn_table {
    int category;           // Which resource category
    int subtype;           // Which specific subtype
    float base_rarity;     // 0.0 to 1.0 (1.0 = always available)
    int min_skill_level;   // Minimum skill to find this
    int region_modifier;   // Bonus in certain regions
    char *regions_found[]; // List of regions where this spawns
};

// Example spawn tables:
subtype_spawn_table herb_spawns[] = {
    {RESOURCE_HERBS, HERB_MARJORAM, 0.8, 0, 0, {"any"}},
    {RESOURCE_HERBS, HERB_KINGFOIL, 0.3, 25, 10, {"Elderwood", "Moonshade"}},
    {RESOURCE_HERBS, HERB_STARLILY, 0.1, 50, 20, {"Astral_Peaks", "Starfall_Valley"}},
    // ...
};

subtype_spawn_table crystal_spawns[] = {
    {RESOURCE_CRYSTALS, CRYSTAL_QUARTZ, 0.6, 0, 0, {"any"}},
    {RESOURCE_CRYSTALS, CRYSTAL_ARCANITE, 0.05, 75, 25, {"Arcanite_Caverns", "Magic_Nexus"}},
    // ...
};
```

### Display and Command Integration

```c
// Enhanced material display commands:
ACMD(do_materials) {
    // Show by category, then subtype, then quality
    // "Herbs: 3x poor marjoram, 1x excellent kingfoil, 2x good starlily"
    // "Crystals: 1x masterwork arcanite, 5x common quartz"
    // "Wood: 4x poor birch, 2x good ironwood"
}

ACMD(do_materials_detail) {
    // Detailed breakdown with lore descriptions
    send_to_char(ch, "Kingfoil (excellent quality): A legendary healing herb...\n");
    send_to_char(ch, "Arcanite Crystal (masterwork): The most powerful magical focus...\n");
}
```

### Crafting Recipe Integration

```c
// Recipes can now specify exact materials:
struct crafting_recipe {
    char *name;
    struct material_requirement requirements[];
    // ...
};

struct material_requirement {
    int category;           // RESOURCE_HERBS
    int subtype;           // HERB_KINGFOIL  
    int min_quality;       // QUALITY_GOOD or better
    int amount_needed;     // How many required
    bool substitutable;    // Can other subtypes work?
};

// Example recipes:
// "Potion of Greater Healing" requires:
// - 2x Kingfoil (good quality or better)
// - 1x Arcanite Crystal (any quality)
// - 3x Pure Water (common or better)
```

### Implementation Timeline

**Phase 4.5a: Data Structures (Week 1)**
- Define subtype enums and spawn tables
- Extend virtual storage system
- Update database schema

**Phase 4.5b: Harvesting Logic (Week 2)**  
- Implement subtype selection during harvesting
- Add quality determination for each subtype
- Create spawn probability tables

**Phase 4.5c: Display Systems (Week 3)**
- Update materials command
- Add lore descriptions for each subtype
- Implement filtering and sorting

**Phase 4.5d: Integration Testing (Week 4)**
- Test with existing crafting system
- Verify storage and retrieval
- Balance rarity and spawn rates

This system supports both your lore-specific materials (Arcanite crystals, Kingfoil herbs) and allows for quality variation within each specific type (poor birch vs excellent birch, poor ironwood vs good ironwood).

---

## 🔧 **Integration Points with Existing Systems**

### **Existing Skills Integration**

**Ready-to-Use Skills (No New Skills Required):**
```c
// Primary wilderness skill
#define ABILITY_SURVIVAL           29  /* matches pfsrd */

// Specialized harvesting skills (already implemented)
#define SKILL_MINING              2071
#define ABILITY_HARVEST_MINING     48
#define ABILITY_HARVEST_HUNTING    49  
#define ABILITY_HARVEST_FORESTRY   50
#define ABILITY_HARVEST_GATHERING  51
```

**Skill Check Integration:**
- Use existing `GET_SKILL(ch, skill_id)` macro
- Use existing `skill_check(ch, skill_id, dc)` function
- Skills already have class progression and training systems
- No modifications needed to skill system

### **Object Flag System Integration**

**Harvesting Tool Flags (New Flags to Add):**
```c
// Add to structs.h (next available IDs after 107)
#define ITEM_HARVESTING_TOOL    108  /* Item can be used for harvesting */
#define ITEM_MINING_TOOL        109  /* Item provides mining bonuses */
#define ITEM_FORESTRY_TOOL      110  /* Item provides forestry bonuses */
#define ITEM_HUNTING_TOOL       111  /* Item provides hunting bonuses */
#define ITEM_GATHERING_TOOL     112  /* Item provides gathering bonuses */

// Update NUM_ITEM_FLAGS
#define NUM_ITEM_FLAGS          113
```

## 🔧 **Complete Object Design Strategy**

### **Harvesting Tool Object Design**

**Create New Item Type:**
```c
// Add to structs.h after existing item types (next ID after 52)
#define ITEM_HARVESTING 53     /* Harvesting tools (axes, pickaxes, knives, etc.) */

// Update total count
#define NUM_ITEM_TYPES 54
```

**Tool Object Configuration:**
```c
// All harvesting tools use new ITEM_HARVESTING type

// Object Type: ITEM_HARVESTING (53)
// Wear Flags: ITEM_WEAR_TAKE | ITEM_WEAR_HOLD | ITEM_WEAR_WIELD
// Extra Flags: ITEM_HARVESTING_TOOL (optional flag 108 for additional marking)

// Object Value Configuration:
value[0] = tool_efficiency      // 1-100% efficiency rating
value[1] = harvestable_flags    // Bitfield of RESOURCE_TYPE_* that tool can harvest
value[2] = tool_durability      // Uses remaining before repair needed
value[3] = skill_bonus          // +N bonus to relevant skill checks
```

**Tool Usage Requirements (Recommendation: HELD or WIELDED):**
```c
// Function: get_harvesting_tool_bonus()
// Check: GET_EQ(ch, WEAR_HOLD) or GET_EQ(ch, WEAR_WIELD) or GET_EQ(ch, WEAR_WIELD_2H)
// Reason: More realistic - you need to actively use the tool, not just carry it
// Tool Type Check: GET_OBJ_TYPE(tool) == ITEM_HARVESTING
```

### **Example Tool Definitions**

**Mining Pickaxe:**
```c
Item Type:    ITEM_HARVESTING
Material:     MATERIAL_STEEL  
Wear Flags:   ITEM_WEAR_TAKE | ITEM_WEAR_HOLD | ITEM_WEAR_WIELD
Extra Flags:  (none required - type identifies it as harvesting tool)
Value[0]:     75              // 75% efficiency
Value[1]:     (HARVEST_MINERALS | HARVEST_STONE | HARVEST_METAL | HARVEST_CRYSTAL)
Value[2]:     100             // 100 uses
Value[3]:     5               // +5 to SKILL_MINING checks
```

**Forester's Axe:**
```c
Item Type:    ITEM_HARVESTING
Material:     MATERIAL_STEEL
Wear Flags:   ITEM_WEAR_TAKE | ITEM_WEAR_HOLD | ITEM_WEAR_WIELD
Extra Flags:  (none required)
Value[0]:     80              // 80% efficiency
Value[1]:     (HARVEST_VEGETATION | HARVEST_HERBS)
Value[2]:     150             // 150 uses
Value[3]:     6               // +6 to ABILITY_HARVEST_FORESTRY checks
```

**Hunting Knife:**
```c
Item Type:    ITEM_HARVESTING
Material:     MATERIAL_STEEL
Wear Flags:   ITEM_WEAR_TAKE | ITEM_WEAR_HOLD | ITEM_WEAR_WIELD
Extra Flags:  (none required)
Value[0]:     60              // 60% efficiency
Value[1]:     (HARVEST_GAME | HARVEST_HERBS | HARVEST_VEGETATION)
Value[2]:     200             // 200 uses
Value[3]:     4               // +4 to ABILITY_HARVEST_HUNTING checks
```

**Multi-Tool (Advanced):**
```c
Item Type:    ITEM_HARVESTING
Material:     MATERIAL_ADAMANTINE
Wear Flags:   ITEM_WEAR_TAKE | ITEM_WEAR_HOLD | ITEM_WEAR_WIELD
Extra Flags:  ITEM_MAGICAL
Value[0]:     90              // 90% efficiency
Value[1]:     (ALL_HARVEST_FLAGS)  // Can harvest everything
Value[2]:     500             // 500 uses
Value[3]:     8               // +8 to all harvesting skill checks
```

### **Integration Impact Analysis**

**✅ Database Changes: NONE**
- Use new `ITEM_HARVESTING` type (stored in existing type_flag field)
- Use existing object `value[]` fields (no new DB columns)
- Use existing wear flag system (no schema changes)

**✅ Code Changes: MINIMAL**
```c
// Need to add:
1. ITEM_HARVESTING type definition (1 line in structs.h)
2. Update NUM_ITEM_TYPES (1 line in structs.h)  
3. Add type name to item_types[] array in constants.c (1 line)
4. Tool detection logic: GET_OBJ_TYPE(tool) == ITEM_HARVESTING
5. Resource type bitfield definitions (10 lines)
6. Optional: ITEM_HARVESTING_TOOL extra flag for additional marking
```

**✅ Tool Usage Decision - Recommendation:**
```c
// HELD or WIELDED requirement (more realistic):
struct obj_data *tool = GET_EQ(ch, WEAR_HOLD);
if (!tool) tool = GET_EQ(ch, WEAR_WIELD);
if (!tool) tool = GET_EQ(ch, WEAR_WIELD_2H);

if (tool && GET_OBJ_TYPE(tool) == ITEM_HARVESTING) {
    // Check if tool can harvest this resource type
    if (GET_OBJ_VAL(tool, 1) & harvest_flag_for_resource) {
        // Apply tool bonuses
    }
}
```

**Benefits of New ITEM_HARVESTING Type:**
- ✅ **Clear purpose** - dedicated type for harvesting tools
- ✅ **No confusion** - separate from ITEM_PICK (lockpicks)
- ✅ **Extensible** - room for harvesting-specific features
- ✅ **Clean code** - easy tool detection via type check
- ✅ **Flexible design** - one tool can harvest multiple resources
- ✅ **Durability system** - tools wear out and need repair
- ✅ **Quality levels** - better materials = better tools

### **Existing Crafting System Integration**

**Material Storage (Use Existing Types):**
```c
// Existing item types ready for use
#define ITEM_MATERIAL    27  /* material for crafting - PERFECT for resources */
#define ITEM_ESSENCE     26  /* component for crafting */

// Existing crafting flags
#define ITEM_CRAFTED     103 /* Already implemented */
```

### **Existing Crafting System Integration**

**Harvested Resource Storage (Use New Virtual System):**
```c
// Extend existing virtual storage system used by new crafting
#define GET_CRAFT_MAT(ch, i)  (ch->player_specials->saved.craft_mats_owned[i])

// Add wilderness resources to craft_mats_owned[] array
// Current system supports 36 materials - add wilderness resources as new material types
```

**Harvesting Tool Capability System:**
```c
// Use object value[] fields to define what resources a tool can harvest
// Tools are made of materials (MATERIAL_STEEL, etc.) but can harvest different resources

// Object value field assignments for harvesting tools:
value[0] = tool_efficiency      // 1-100 efficiency rating
value[1] = harvestable_flags    // Bitfield of RESOURCE_TYPE_* flags
value[2] = tool_durability      // Uses before breaking/repair needed
value[3] = skill_bonus          // +N bonus to harvesting skill checks
```

**Resource Type Integration:**
```c
// Map wilderness resources to new material types in craft_mats_owned[]
#define CRAFT_MAT_WILD_VEGETATION  30  // From resource system vegetation
#define CRAFT_MAT_WILD_MINERALS    31  // From resource system minerals  
#define CRAFT_MAT_WILD_STONE       32  // From resource system stone
#define CRAFT_MAT_WILD_WATER       33  // From resource system water
#define CRAFT_MAT_WILD_GAME        34  // From resource system game
#define CRAFT_MAT_WILD_HERBS       35  // From resource system herbs
// Continue mapping all 10 resource types
```

**Tool Resource Capability Flags:**
```c
// Bitfield values for value[1] of harvesting tools
#define HARVEST_VEGETATION   (1 << 0)   // Can harvest vegetation
#define HARVEST_MINERALS     (1 << 1)   // Can harvest minerals
#define HARVEST_STONE        (1 << 2)   // Can harvest stone  
#define HARVEST_WATER        (1 << 3)   // Can harvest water
#define HARVEST_GAME         (1 << 4)   // Can harvest game
#define HARVEST_HERBS        (1 << 5)   // Can harvest herbs
#define HARVEST_CRYSTAL      (1 << 6)   // Can harvest crystal
#define HARVEST_RARE_CRYSTAL (1 << 7)   // Can harvest rare crystal
#define HARVEST_METAL        (1 << 8)   // Can harvest metal
#define HARVEST_GEMSTONE     (1 << 9)   // Can harvest gemstone
```

**Example Tool Configurations:**
```c
// Mining Pickaxe (made of MATERIAL_STEEL)
value[0] = 75              // 75% efficiency
value[1] = HARVEST_MINERALS | HARVEST_STONE | HARVEST_METAL | HARVEST_CRYSTAL
value[2] = 100             // 100 uses before repair
value[3] = 5               // +5 bonus to mining checks

// Hunting Knife (made of MATERIAL_STEEL) 
value[0] = 60              // 60% efficiency
value[1] = HARVEST_VEGETATION | HARVEST_GAME | HARVEST_HERBS
value[2] = 150             // 150 uses before repair  
value[3] = 3               // +3 bonus to survival checks

// Masterwork Forester's Axe (made of MATERIAL_ADAMANTINE)
value[0] = 95              // 95% efficiency
value[1] = HARVEST_VEGETATION | HARVEST_HERBS
value[2] = 500             // 500 uses before repair
value[3] = 8               // +8 bonus to forestry checks
```

### **Database Integration Points**

**✅ NO Database Changes Required:**
```c
// All tool data stored in existing object fields:
// - Item Type: ITEM_PICK (existing)
// - Material: obj->obj_flags.material (existing)  
// - Wear Flags: obj->obj_flags.wear_flags[] (existing)
// - Extra Flags: obj->obj_flags.extra_flags[] (existing)
// - Tool Data: obj->obj_flags.value[0-3] (existing)

// Character resource storage: existing craft_mats_owned[] array
// Region effects: existing region_effects table (Phase 4)
```

**Character Storage Extension:**
```c
// Extend existing craft_mats_owned[] array in player_specials_saved
// Current: int craft_mats_owned[NUM_CRAFT_MATS]; // 36 materials
// Map wilderness resources to available slots:
#define CRAFT_MAT_WILD_VEGETATION  30  
#define CRAFT_MAT_WILD_MINERALS    31
#define CRAFT_MAT_WILD_STONE       32
#define CRAFT_MAT_WILD_WATER       33
#define CRAFT_MAT_WILD_GAME        34
#define CRAFT_MAT_WILD_HERBS       35
// Continue mapping all 10 resource types
```

### **File Modification Requirements**

### **File Modification Requirements**

**Core Files to Modify:**
1. **src/structs.h**: 
   - Add `ITEM_HARVESTING` type definition (1 line)
   - Update `NUM_ITEM_TYPES` from 53 to 54 (1 line)
   - Optional: Add `ITEM_HARVESTING_TOOL` extra flag (1 line)
2. **src/constants.c**: 
   - Add "Harvesting Tool" to `item_types[]` array (1 line)
   - Optional: Add flag name to `extra_bits[]` array (1 line)
3. **src/utils.h**: Add resource type to craft material mapping macros (10 lines)
4. **src/act.informative.c**: Enhance survey command with tool detection
5. **src/resource_system.c**: Add harvesting mechanics integration
6. **src/interpreter.c**: Add harvest/gather commands to table (2 lines)
7. **Makefile.am**: Already includes resource_system.c (no changes needed)

**Resource Storage Strategy:**
- Harvested resources stored in existing `craft_mats_owned[]` array
- Object value[] fields store tool capabilities (no DB changes needed)
- Use new `ITEM_HARVESTING` type for clean tool identification

### **Command Integration Strategy**

**New Commands (Build on Existing Framework):**
```c
// Add to src/interpreter.c command table
{ "harvest"   , POS_STANDING, do_harvest, 0, 0, SCMD_HARVEST },
{ "gather"    , POS_STANDING, do_harvest, 0, 0, SCMD_GATHER },

// Enhance existing survey command (already modified in Phase 2)
// No changes needed - survey already shows resources
```

**Integration with Existing Systems:**
- Use existing command parsing framework
- Leverage current position/state checking
- Integrate with existing skill gain system
- Use current inventory management systems

### **Integration with Existing Systems**

**✅ Use Existing Location/Zone Detection Idioms:**
```c
// Character location and zone checking:
room_rnum room = IN_ROOM(ch);                    // Get character's current room
zone_rnum zone = GET_ROOM_ZONE(IN_ROOM(ch));    // Get zone from character's room  
zone_rnum zrnum = world[IN_ROOM(ch)].zone;      // Alternative zone access

// Wilderness zone validation:
if (!ZONE_FLAGGED(zrnum, ZONE_WILDERNESS)) {
    send_to_char(ch, "You can only harvest in the wilderness.\r\n");
    return;
}

// Coordinates access (for wilderness rooms):
int x = world[IN_ROOM(ch)].coords[0];
int y = world[IN_ROOM(ch)].coords[1];
```

**✅ Use Existing Region Detection System:**
```c
// Phase 4 already provides region detection - reuse existing function:
struct region_list *regions = get_enclosing_regions(zone, x, y);

// Process region effects (existing database integration):
// - get_enclosing_regions() already queries database
// - region_effects table already implemented
// - JSON parameter processing already available
```

**✅ Use Existing Affect/Status Checking Idioms:**
```c
// Character affect checking:
// - System already tracks skill usage and grants experience
```

**✅ Use Existing Position/State Validation:**
```c
// Position checking (already used by many commands):
if (GET_POS(ch) < POS_STANDING) {
    send_to_char(ch, "You need to be standing to harvest.\r\n");
    return;
}

// Combat state checking:
if (FIGHTING(ch)) {
    send_to_char(ch, "You cannot harvest while fighting!\r\n");
    return;
}
```

**✅ Leverage Existing Resource System (Phase 1-4):**
```c
// Resource calculation (already implemented):
struct resource_node *node = get_cached_resources(zone, x, y);
if (!node) {
    node = calculate_resource_levels(zone, x, y);
    cache_resource_node(node);
}

// Resource values access:
float vegetation_level = node->resources[RESOURCE_TYPE_VEGETATION];
float mineral_level = node->resources[RESOURCE_TYPE_MINERALS];
```

**❌ Issues Found in Existing Idioms:**

**1. Region Effects Integration Gap:**
```c
// Current resource_system.c has placeholder function:
float apply_region_resource_modifiers(int resource_type, int x, int y, float base_value) {
    /* For now, return base value unchanged until region integration is properly implemented */
    return base_value;  // NOT IMPLEMENTED YET
}

// NEEDS COMPLETION: Integrate with existing region database system
```

**2. Character-Based Region Detection Missing:**
```c
// Current function exists but not used in resource system:
void apply_region_resource_modifiers_to_node(struct char_data *ch, struct resource_node *resources) {
    /* TODO: Implement when character-based region detection is available */
    // NEEDS IMPLEMENTATION
}
```

## 🌲 **Wilderness and Region Affect Management**

### **Wilderness-Only Operation Requirement**

Phase 5 harvesting mechanics are **exclusively wilderness-based**. All region effects and harvesting bonuses must be confined to wilderness zones.

**Core Requirements:**
- Region harvesting affects only applied in wilderness rooms
- All region affects removed when leaving wilderness (any method)
- Wilderness check: `ZONE_FLAGGED(GET_ROOM_ZONE(room), ZONE_WILDERNESS)`

### **Option 1: Region-Based Affects System (Recommended)**

**Benefits:**
- Automatic cleanup when affects expire
- Integrates with existing affect system  
- Player-visible via `affects` command
- Works with dispel/immunity systems
- Performance: Only checks regions on movement, not every harvest

**Implementation Strategy:**

**A. Movement Integration Point:**
```c
// In src/handler.c, at end of char_to_room() function:
void char_to_room(struct char_data *ch, room_rnum room) {
    // ...existing code...
    
    /* Phase 5: Update region harvesting affects (wilderness only) */
    update_region_harvesting_affects(ch);
}
```

**B. Affect Management Function:**
```c
// In src/resource_system.c:
void update_region_harvesting_affects(struct char_data *ch) {
    if (!ch || IN_ROOM(ch) == NOWHERE)
        return;

    zone_rnum zrnum = world[IN_ROOM(ch)].zone;
    if (!ZONE_FLAGGED(zrnum, ZONE_WILDERNESS)) {
        /* Remove all region harvesting affects when not in wilderness */
        affect_from_char(ch, SPELL_REGION_MINING_BONUS);
        affect_from_char(ch, SPELL_REGION_FORESTRY_BONUS);
        affect_from_char(ch, SPELL_REGION_HUNTING_BONUS);
        affect_from_char(ch, SPELL_REGION_GATHERING_BONUS);
        return;
    }

    /* In wilderness - check and apply region affects */
    struct region_list *regions = get_enclosing_regions(
        world[IN_ROOM(ch)].coords[0], 
        world[IN_ROOM(ch)].coords[1]
    );
    
    // Remove existing affects first
    affect_from_char(ch, SPELL_REGION_MINING_BONUS);
    affect_from_char(ch, SPELL_REGION_FORESTRY_BONUS);
    affect_from_char(ch, SPELL_REGION_HUNTING_BONUS);
    affect_from_char(ch, SPELL_REGION_GATHERING_BONUS);
    
    // Apply new region effects
    while (regions) {
        apply_region_harvesting_bonus(ch, regions->region_id);
        regions = regions->next;
    }
}
```

**C. New Affect Types Required:**
```c
// Add to src/spells.h:
#define SPELL_REGION_MINING_BONUS     (next_available_spell_id)
#define SPELL_REGION_FORESTRY_BONUS   (next_available_spell_id + 1)
#define SPELL_REGION_HUNTING_BONUS    (next_available_spell_id + 2)
#define SPELL_REGION_GATHERING_BONUS  (next_available_spell_id + 3)
```

**D. Movement Coverage:**
- **All movement types covered**: Walking, teleportation, magic, scripts, etc.
- **Guarantee**: Every movement uses `char_to_room()`, so affects are always updated
- **Cleanup**: Affects automatically removed when leaving wilderness by any means

### **Option 2: Direct Region Checking (Alternative)**

**Benefits:**
- No affect system overhead
- More precise control
- No cleanup required

**Implementation:**
```c
// Check regions directly during harvesting commands:
float get_harvesting_bonus(struct char_data *ch, int resource_type) {
    if (!ch || IN_ROOM(ch) == NOWHERE)
        return 1.0;
        
    zone_rnum zrnum = world[IN_ROOM(ch)].zone;
    if (!ZONE_FLAGGED(zrnum, ZONE_WILDERNESS))
        return 1.0;  // No bonus outside wilderness
        
    struct region_list *regions = get_enclosing_regions(
        world[IN_ROOM(ch)].coords[0], 
        world[IN_ROOM(ch)].coords[1]
    );
    
    float bonus = 1.0;
    while (regions) {
        bonus *= get_region_harvesting_multiplier(regions->region_id, resource_type);
        regions = regions->next;
    }
    return bonus;
}
```

### **Recommended Approach: Option 1 (Affects)**

**Rationale:**
1. **Consistency**: Matches LuminariMUD's affect-based bonus system
2. **Performance**: Only checks regions on movement, not every harvest
3. **Player Experience**: Bonuses visible in `affects` command
4. **Integration**: Works with existing dispel/immunity systems
5. **Cleanup**: Automatic removal when leaving wilderness

**Movement Coverage Guarantee:**
- Covers **all** movement types (walking, magic, scripts, teleportation)
- **Single integration point**: All movement goes through `char_to_room()`
- **Wilderness constraint**: Affects only exist in wilderness zones
- **Automatic cleanup**: Leaving wilderness immediately removes all region affects

### **Recommended Integration Approach:**

**✅ Complete Phase 4 Region Integration:**
1. **Fix `apply_region_resource_modifiers()`** - integrate with existing `get_enclosing_regions()`
2. **Implement `apply_region_resource_modifiers_to_node()`** - use character location to apply effects
3. **Leverage existing region_effects database table** - no new tables needed

**✅ Harvesting Command Integration:**
```c
ACMD(do_harvest) {
    // Use ALL existing idioms:
    zone_rnum zrnum = world[IN_ROOM(ch)].zone;
    
    // Wilderness check (existing idiom)
    if (!ZONE_FLAGGED(zrnum, ZONE_WILDERNESS)) {
        send_to_char(ch, "You can only harvest in the wilderness.\r\n");
        return;
    }
    
    // Position/state checks (existing idioms)
    if (GET_POS(ch) < POS_STANDING) { /* ... */ }
    if (FIGHTING(ch)) { /* ... */ }
    if (AFF_FLAGGED(ch, AFF_PARALYZED)) { /* ... */ }
    
    // Tool detection (existing idioms)
    struct obj_data *tool = GET_EQ(ch, WEAR_HOLD);
    if (!tool) tool = GET_EQ(ch, WEAR_WIELD);
    
    // Resource calculation (existing Phase 1-4 system)
    int x = world[IN_ROOM(ch)].coords[0];
    int y = world[IN_ROOM(ch)].coords[1];
    struct resource_node *node = get_cached_resources(zrnum, x, y);
    
    // Region effects (existing region system)
    struct region_list *regions = get_enclosing_regions(zrnum, x, y);
    
    // Skill checks (existing skill system)
    bool success = skill_check(ch, ABILITY_SURVIVAL, difficulty);
    
    // Storage (existing craft material system)
    GET_CRAFT_MAT(ch, material_type) += harvested_amount;
}
```

This approach leverages **100% existing systems** and only adds the minimal new logic needed for harvesting mechanics!

#### **A. Crafting System Architecture Analysis**

LuminariMUD currently uses **dual storage systems** for crafting materials:

**Object-Based Storage (Legacy System):**
- Materials stored as actual game objects (`ITEM_MATERIAL` type)
- Located in player inventory or containers
- Examples: `STEEL_MATERIAL`, `BRONZE_MATERIAL`, `LEATHER_MATERIAL` objects
- Used by older crafting commands (`harvest`, `create`, `augment`)

**Virtual Storage (New System):**
- Materials stored as numeric counters in player data
- Location: `ch->player_specials->saved.craft_mats_owned[NUM_CRAFT_MATS]` array
- Access via `GET_CRAFT_MAT(ch, material_type)` macro
- Display through `materials` command
- Used by new crafting system in `crafting_new.c`

#### **B. Recommended Integration Approach: Enhanced Virtual Storage**

**Primary Storage Strategy:**
- **Wilderness resources → Virtual storage** (extending current new system)
- **Benefits**: No inventory clutter, precise quantity tracking, easy database storage
- **Quality Integration**: Extend storage to include quality levels per material

**Enhanced Virtual Storage Structure:**
```c
// Proposed extension to player data:
struct material_inventory {
    int amount;
    int quality_poor;
    int quality_common; 
    int quality_good;
    int quality_excellent;
    int quality_masterwork;
};

// Replace: int craft_mats_owned[NUM_CRAFT_MATS];
// With: struct material_inventory wilderness_materials[NUM_RESOURCE_TYPES];
```

**Quality-Aware Resource System:**
```c
// Wilderness harvest result structure:
struct harvest_result {
    int resource_type;        // VEGETATION, MINERALS, etc.
    int base_amount;          // 2-6 units typical
    int quality_level;        // 0=Poor, 1=Common, 2=Good, 3=Excellent, 4=Masterwork
    float quality_multiplier; // 0.5, 1.0, 1.5, 2.0, 3.0
};
```

#### **C. Material Mapping System**

**Wilderness to Crafting Material Conversion:**
```c
int wilderness_to_craft_material(int resource_type, int quality) {
    switch(resource_type) {
        case RESOURCE_WOOD:
            switch(quality) {
                case QUALITY_POOR: return CRAFT_MAT_WOOD;
                case QUALITY_GOOD: return CRAFT_MAT_OAK_WOOD; 
                case QUALITY_EXCELLENT: return CRAFT_MAT_YEW_WOOD;
                case QUALITY_MASTERWORK: return CRAFT_MAT_IRONWOOD;
            }
        case RESOURCE_MINERALS:
            switch(quality) {
                case QUALITY_POOR: return CRAFT_MAT_COPPER;
                case QUALITY_COMMON: return CRAFT_MAT_IRON;
                case QUALITY_GOOD: return CRAFT_MAT_STEEL;
                case QUALITY_EXCELLENT: return CRAFT_MAT_MITHRIL;
                case QUALITY_MASTERWORK: return CRAFT_MAT_ADAMANTINE;
            }
        // Continue for all resource types...
    }
}
```

#### **D. User Experience Design**

**Enhanced Commands:**
```
wilderness materials     - Show wilderness-gathered materials by type/quality
materials               - Show all crafting materials (existing command enhanced)
convert materials       - Convert wilderness materials to traditional craft materials
materials quality       - Show quality breakdown of all materials
materialize <mat> <amt> - Create objects from virtual materials when needed
```

**Integration Points:**
- `craft` command works with virtual materials (already functional)
- `materials` command expanded to show wilderness resources
- Recipe system extends existing `crafting_recipes` with wilderness materials
- Quality affects crafting bonuses via existing enhancement system

#### **E. Database Schema Integration**

**Extended Player Storage:**
```sql
-- Add to existing character data structure
wilderness_vegetation_poor INT DEFAULT 0,
wilderness_vegetation_common INT DEFAULT 0,
wilderness_vegetation_good INT DEFAULT 0,
wilderness_vegetation_excellent INT DEFAULT 0,
wilderness_vegetation_masterwork INT DEFAULT 0,
wilderness_minerals_poor INT DEFAULT 0,
wilderness_minerals_common INT DEFAULT 0,
-- Repeat pattern for all 10 resource types...
```

#### **F. Backward Compatibility**

**Object Conversion System:**
```c
// Convert virtual materials to objects when needed for trading/shops
struct obj_data *create_material_object(int resource_type, int quality, int amount) {
    int material_vnum = get_material_vnum(resource_type, quality);
    struct obj_data *obj = read_object(material_vnum, VIRTUAL);
    GET_OBJ_VAL(obj, 0) = amount; // Store quantity in object
    return obj;
}
```

#### **G. Complete Resource Flow**

**Phase 5 Resource Pipeline:**
```
Wilderness Survey → Identify resource nodes → Player harvests with tools → 
Quality roll (skill/tool/region effects) → Virtual materials added → 
Crafting system uses virtual materials → Enhanced items created
```

**Integration Benefits:**
- **Seamless Integration**: Uses existing virtual storage architecture
- **Quality System**: Natural extension for quality-based crafting  
- **No Inventory Bloat**: Wilderness materials don't clutter inventory
- **Code Reuse**: Leverages current `crafting_new.c` infrastructure
- **Database Efficient**: Extends current storage patterns
- **Flexible**: Can convert to objects when needed for trading/shops

**Economy Integration:**
- Player shops can buy/sell harvested materials (via object conversion)
- Regional price variations based on local availability and quality
- Trade routes emerge based on resource distribution and quality differences
- Quality tiers create natural economic stratification

**Experience and Advancement:**
- Successful harvesting grants skill experience via existing system
- Discovering new resource nodes provides exploration bonuses
- Quality harvests provide bonus experience
- Mastery achievements for resource specialization and quality thresholds

## � **Technical Implementation Details**

### **A. Data Structure Extensions**

**Player Data Structure (structs.h):**
```c
// Addition to player_special_data_saved:
struct wilderness_material_inventory {
    // [resource_type][quality_level] = amount
    int materials[NUM_RESOURCE_TYPES][NUM_QUALITY_LEVELS];
    int last_harvest_time[NUM_RESOURCE_TYPES]; // Cooldown tracking
    int harvest_streak[NUM_RESOURCE_TYPES];    // Consecutive success tracking
} wilderness_inventory;
```

**Quality Level Constants:**
```c
#define NUM_QUALITY_LEVELS  5
#define QUALITY_POOR        0
#define QUALITY_COMMON      1  
#define QUALITY_GOOD        2
#define QUALITY_EXCELLENT   3
#define QUALITY_MASTERWORK  4

// Quality multipliers for crafting value
static float quality_multipliers[NUM_QUALITY_LEVELS] = {
    0.5,  // Poor
    1.0,  // Common
    1.5,  // Good
    2.0,  // Excellent
    3.0   // Masterwork
};
```

### **B. Core Implementation Functions**

**Harvest Execution Function:**
```c
int execute_wilderness_harvest(struct char_data *ch, int resource_type, struct obj_data *tool) {
    // 1. Calculate base success chance
    int skill_level = get_harvest_skill(ch, resource_type);
    int tool_bonus = get_tool_effectiveness(tool, resource_type);
    int region_modifier = get_region_harvest_modifier(ch);
    
    // 2. Determine success and quality
    int success_roll = dice(1, 100) + skill_level + tool_bonus + region_modifier;
    int dc = get_harvest_dc(resource_type);
    
    if (success_roll < dc) {
        // Handle failure with partial success possibility
        return handle_harvest_failure(ch, success_roll, dc);
    }
    
    // 3. Calculate yield and quality
    int base_yield = get_base_harvest_yield(resource_type);
    int quality = determine_harvest_quality(success_roll, dc, skill_level);
    int final_yield = calculate_final_yield(base_yield, skill_level, tool_bonus, quality);
    
    // 4. Add to player inventory
    add_wilderness_material(ch, resource_type, quality, final_yield);
    
    // 5. Update resource node
    deplete_resource_node(ch, resource_type, final_yield);
    
    // 6. Grant experience
    grant_harvest_experience(ch, resource_type, quality, final_yield);
    
    return final_yield;
}
```

**Quality Determination Algorithm:**
```c
int determine_harvest_quality(int success_roll, int dc, int skill_level) {
    int excess = success_roll - dc;
    int quality_chance = excess + (skill_level / 10);
    
    // Natural 20 always gives chance for masterwork
    if ((success_roll % 100) >= 95) quality_chance += 20;
    
    if (quality_chance >= 40) return QUALITY_MASTERWORK;
    if (quality_chance >= 25) return QUALITY_EXCELLENT;
    if (quality_chance >= 15) return QUALITY_GOOD;
    if (quality_chance >= 5)  return QUALITY_COMMON;
    return QUALITY_POOR;
}
```

### **C. Tool Integration System**

**Tool Effectiveness Calculation:**
```c
int get_tool_effectiveness(struct obj_data *tool, int resource_type) {
    if (!tool) return 0; // No tool penalty
    
    // Check tool type matches resource type
    if (!is_appropriate_tool(tool, resource_type)) return -10;
    
    int base_bonus = GET_OBJ_VAL(tool, 1); // Tool quality value
    int condition_bonus = (GET_OBJ_VAL(tool, 2) / 10); // Condition affects efficiency
    int enhancement_bonus = GET_OBJ_VAL(tool, 4); // Magical enhancement
    
    // Tool specialization bonuses
    int specialization = get_tool_specialization_bonus(tool, resource_type);
    
    return base_bonus + condition_bonus + enhancement_bonus + specialization;
}
```

**Tool Durability System:**
```c
void apply_tool_wear(struct obj_data *tool, int harvest_difficulty) {
    if (!tool || !is_harvest_tool(tool)) return;
    
    int durability_loss = dice(1, 3) + (harvest_difficulty / 10);
    int current_condition = GET_OBJ_VAL(tool, 2);
    
    current_condition -= durability_loss;
    if (current_condition <= 0) {
        act("$p breaks from the strain of harvesting!", FALSE, tool->carried_by, tool, 0, TO_CHAR);
        extract_obj(tool);
        return;
    }
    
    GET_OBJ_VAL(tool, 2) = current_condition;
    
    // Condition warnings
    if (current_condition <= 20) {
        act("$p is showing signs of serious wear.", FALSE, tool->carried_by, tool, 0, TO_CHAR);
    }
}
```

### **D. Command Interface Implementation**

**Main Harvest Command Structure:**
```c
ACMD(do_wilderness_harvest) {
    char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
    struct obj_data *tool = NULL;
    int resource_type;
    
    argument = two_arguments(argument, arg1, arg2);
    
    // Parse arguments: harvest <resource_type> [with <tool>]
    if (!*arg1) {
        show_available_resources(ch);
        return;
    }
    
    resource_type = parse_resource_type(arg1);
    if (resource_type < 0) {
        send_to_char(ch, "Unknown resource type. Available: %s\r\n", 
                     list_available_resource_types(ch));
        return;
    }
    
    // Tool handling
    if (*arg2 && !strncmp(arg2, "with", 4)) {
        tool = find_harvest_tool(ch, argument);
        if (!tool) {
            send_to_char(ch, "You don't have that tool.\r\n");
            return;
        }
    } else {
        tool = find_best_tool_for_resource(ch, resource_type);
    }
    
    // Validation checks
    if (!validate_harvest_conditions(ch, resource_type)) return;
    if (!has_required_skill(ch, resource_type)) return;
    if (!resource_available_at_location(ch, resource_type)) return;
    
    // Execute harvest
    start_harvest_event(ch, resource_type, tool);
}
```

### **E. Integration with Existing Crafting System**

**Material Conversion Functions:**
```c
// Convert wilderness materials to crafting materials for recipes
int convert_wilderness_to_craft_material(int wilderness_type, int quality) {
    static int conversion_table[NUM_RESOURCE_TYPES][NUM_QUALITY_LEVELS] = {
        // VEGETATION
        {CRAFT_MAT_HEMP, CRAFT_MAT_COTTON, CRAFT_MAT_LINEN, CRAFT_MAT_SILK, CRAFT_MAT_DRAGONSILK},
        // MINERALS  
        {CRAFT_MAT_COPPER, CRAFT_MAT_IRON, CRAFT_MAT_STEEL, CRAFT_MAT_MITHRIL, CRAFT_MAT_ADAMANTINE},
        // WOOD
        {CRAFT_MAT_WOOD, CRAFT_MAT_ASH_WOOD, CRAFT_MAT_OAK_WOOD, CRAFT_MAT_YEW_WOOD, CRAFT_MAT_IRONWOOD},
        // Add conversion tables for all resource types...
    };
    
    return conversion_table[wilderness_type][quality];
}

// Automatic conversion during crafting
int get_available_craft_material(struct char_data *ch, int craft_material_needed) {
    int available = GET_CRAFT_MAT(ch, craft_material_needed);
    
    // Check if we can convert wilderness materials
    int wilderness_equivalent = find_wilderness_equivalent(craft_material_needed);
    if (wilderness_equivalent >= 0) {
        available += get_wilderness_material_total(ch, wilderness_equivalent);
    }
    
    return available;
}
```

### **F. Database Schema Implementation**

**Extended Player Table:**
```sql
-- Add to existing player_data table or create wilderness_inventory table
ALTER TABLE player_data ADD COLUMN wilderness_materials TEXT;

-- Store as JSON for flexibility:
-- {"vegetation": {"poor": 15, "common": 8, "good": 3, "excellent": 1, "masterwork": 0},
--  "minerals": {"poor": 23, "common": 12, "good": 5, "excellent": 2, "masterwork": 1},
--  ...}

-- Or add individual columns for performance:
ALTER TABLE player_data ADD COLUMN wm_vegetation_poor INT DEFAULT 0;
ALTER TABLE player_data ADD COLUMN wm_vegetation_common INT DEFAULT 0;
ALTER TABLE player_data ADD COLUMN wm_vegetation_good INT DEFAULT 0;
ALTER TABLE player_data ADD COLUMN wm_vegetation_excellent INT DEFAULT 0;
ALTER TABLE player_data ADD COLUMN wm_vegetation_masterwork INT DEFAULT 0;
-- Repeat for all 10 resource types...
```

### **G. Event System Integration**

**Harvest Event Structure:**
```c
struct harvest_event_data {
    int resource_type;
    struct obj_data *tool;
    int progress;
    int total_time;
    int skill_checks_made;
    int partial_yields;
};

// Event processing function
EVENTFUNC(harvest_event) {
    struct char_data *ch = (struct char_data *)event_obj;
    struct harvest_event_data *harvest = (struct harvest_event_data *)info;
    
    // Process harvest progress
    int success = make_harvest_skill_check(ch, harvest->resource_type, harvest->tool);
    
    if (success) {
        harvest->progress += get_progress_increment(success);
        harvest->partial_yields += calculate_partial_yield(success);
    } else {
        handle_harvest_failure_event(ch, harvest);
    }
    
    // Check completion
    if (harvest->progress >= 100) {
        complete_harvest_event(ch, harvest);
        return 0; // Event complete
    }
    
    return 2; // Continue for 2 more seconds
}
```

**This technical implementation provides the specific code structure needed to integrate wilderness harvesting with the existing crafting system while maintaining backward compatibility and extending functionality through the proven virtual storage approach.**

## �🗄️ **Database Schema Extensions**

### New Tables

**resource_availability** - Tracks current resource levels
```sql
CREATE TABLE resource_availability (
    id INT PRIMARY KEY AUTO_INCREMENT,
    x_coord INT NOT NULL,
    y_coord INT NOT NULL,
    resource_type INT NOT NULL,
    current_amount FLOAT NOT NULL DEFAULT 0.0,
    max_amount FLOAT NOT NULL,
    last_harvested TIMESTAMP NULL,
    depletion_level FLOAT DEFAULT 0.0,
    regeneration_rate FLOAT DEFAULT 1.0,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    UNIQUE KEY idx_location_resource (x_coord, y_coord, resource_type),
    INDEX idx_coords (x_coord, y_coord),
    INDEX idx_last_harvested (last_harvested)
);
```

**harvesting_logs** - Track player harvesting activity
```sql
CREATE TABLE harvesting_logs (
    id INT PRIMARY KEY AUTO_INCREMENT,
    character_id INT NOT NULL,
    x_coord INT NOT NULL,
    y_coord INT NOT NULL,
    resource_type INT NOT NULL,
    amount_harvested FLOAT NOT NULL,
    quality_level INT NOT NULL,
    skill_used INT NOT NULL,
    skill_level INT NOT NULL,
    tool_used VARCHAR(100),
    success_rate FLOAT NOT NULL,
    harvested_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    INDEX idx_character (character_id),
    INDEX idx_location (x_coord, y_coord),
    INDEX idx_resource (resource_type),
    INDEX idx_time (harvested_at)
);
```

**harvesting_tools** - Tool definitions and bonuses
```sql
CREATE TABLE harvesting_tools (
    id INT PRIMARY KEY AUTO_INCREMENT,
    tool_name VARCHAR(100) NOT NULL UNIQUE,
    tool_type ENUM('basic', 'standard', 'masterwork', 'magical') NOT NULL,
    resource_types JSON NOT NULL,          -- Which resources this tool works for
    efficiency_bonus FLOAT DEFAULT 1.0,    -- Yield multiplier
    success_bonus INT DEFAULT 0,           -- Success rate bonus
    quality_bonus INT DEFAULT 0,           -- Quality bonus
    durability_max INT DEFAULT 100,        -- Tool durability
    object_vnum INT,                       -- Link to game object if applicable
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);
```

### Modified Tables

**Extend player data for harvesting stats:**
```sql
ALTER TABLE player_data ADD COLUMN (
    total_harvests INT DEFAULT 0,
    resources_discovered INT DEFAULT 0,
    master_harvester_achievements JSON,
    preferred_harvesting_tools JSON
);
```

## 🎮 **Player Experience Design**

### Progression Path

**Novice Harvester (Levels 1-20):**
- Basic resource identification
- Simple harvesting with hands/basic tools
- Learning resource locations and seasonal patterns
- 20-40% success rates

**Skilled Gatherer (Levels 21-50):**
- Tool proficiency and specialization choices
- Understanding of depletion and regeneration
- Access to quality assessment abilities
- 40-70% success rates

**Expert Forager (Levels 51-80):**
- Advanced techniques (careful/quick harvesting)
- Ability to track resource regeneration
- Discovery of rare resource nodes
- 70-90% success rates

**Master Harvester (Levels 81-100):**
- Legendary resource detection
- Minimal environmental impact harvesting
- Teaching abilities for other players
- 90-95% success rates

### Risk vs Reward Mechanics

**High-Risk Areas:**
- Dangerous wilderness regions with aggressive mobs
- Better resources and higher yields
- Rare materials only found in perilous locations
- Group harvesting expeditions encouraged

**Safe Harvesting:**
- Near settlements and safe zones
- Lower yields but reliable access
- Good for learning and basic material gathering
- Solo-friendly for new players

## 🔧 **Implementation Phases**

### Phase 5a: Core Harvesting (Week 1-2)
- Basic `harvest` command implementation
- Resource consumption tracking
- Simple depletion mechanics
- Database schema installation

### Phase 5b: Skills and Tools (Week 3-4)
- Skill-based success rates
- Tool integration and bonuses
- Quality system implementation
- Enhanced success/failure messaging

### Phase 5c: Regeneration System (Week 5-6)
- Time-based resource regeneration
- Seasonal variation implementation
- Environmental factor integration
- Performance optimization

### Phase 5d: Advanced Features (Week 7-8)
- Advanced harvesting commands (careful/quick)
- Resource tracking and discovery systems
- Achievement and progression system
- Balance testing and refinement

## 📊 **Performance Considerations**

**Database Optimization:**
- Efficient indexing on coordinate-based queries
- Batch processing for regeneration updates
- Cache frequently accessed resource data
- Archive old harvesting logs periodically

**Memory Management:**
- Resource availability cached per zone
- Lazy loading of harvesting tools data
- Efficient cleanup of expired regeneration timers
- Smart cache invalidation on player actions

**Network Efficiency:**
- Batch resource updates for multiple harvests
- Compressed data transmission for resource status
- Client-side caching of static tool information
- Minimal database queries per harvest action

## ⚠️ **Integration Challenges and Considerations**

### **A. Legacy System Compatibility**

**Challenge**: Dual material storage systems (object-based vs virtual)
- **Risk**: Player confusion between old and new material types
- **Solution**: Gradual migration with conversion utilities
- **Implementation**: Maintain both systems during transition period

**Challenge**: Existing crafting recipes expect specific material types
- **Risk**: Breaking current crafting workflows
- **Solution**: Extend recipe system to accept wilderness material equivalents
- **Implementation**: Material conversion table with quality-based mapping

### **B. Database Performance Concerns**

**Challenge**: Frequent harvesting creates heavy database load
- **Risk**: Performance degradation during peak times
- **Solution**: Implement intelligent caching and batch updates
- **Implementation**: Local caching with periodic bulk database writes

**Challenge**: Resource regeneration calculations on every access
- **Risk**: CPU intensive operations affecting server performance
- **Solution**: Lazy evaluation with smart cache invalidation
- **Implementation**: Cache resource values with timestamp-based expiry

### **C. Economic Balance Implications**

**Challenge**: Wilderness harvesting may devalue existing materials
- **Risk**: Economic disruption of current trading patterns
- **Solution**: Gradual introduction with limited initial yields
- **Implementation**: Phased rollout with yield adjustments based on economic data

**Challenge**: Quality system may create tier gaps in player access
- **Risk**: Masterwork materials becoming too exclusive
- **Solution**: Multiple paths to quality materials (skill, tools, time investment)
- **Implementation**: Balanced probability curves with skill-based improvements

### **D. User Experience Transitions**

**Challenge**: Learning curve for new harvesting mechanics
- **Risk**: Player frustration with system complexity
- **Solution**: Progressive disclosure and comprehensive help system
- **Implementation**: Tutorial quests and contextual help messages

**Challenge**: Inventory management with quality-based materials
- **Risk**: Player storage becoming unwieldy
- **Solution**: Quality-aware material display and management commands
- **Implementation**: Enhanced materials command with sorting and filtering

### **E. Development Risk Mitigation**

**Priority 1 Risks:**
- **Database migration errors**: Implement comprehensive backup and rollback procedures
- **Performance regressions**: Establish baseline metrics and monitoring
- **Player data corruption**: Use transactional operations for all material updates

**Priority 2 Risks:**
- **Tool balance issues**: Plan for rapid adjustment capabilities
- **Resource node depletion**: Implement emergency regeneration procedures
- **Economic inflation**: Monitor material prices and adjust spawn rates

### **F. Rollback Contingency Plans**

**Full Rollback Procedure:**
1. Disable wilderness harvesting commands
2. Convert virtual wilderness materials to equivalent crafting materials
3. Restore original resource system behavior
4. Communicate changes to players with compensation

**Partial Rollback Options:**
- Disable specific resource types while maintaining others
- Adjust quality distributions without removing quality system
- Reduce harvest yields while maintaining mechanics
- Temporarily disable tool durability system

### **G. Testing Strategy**

**Unit Testing Priorities:**
- Material conversion functions
- Quality determination algorithms
- Database transaction integrity
- Cache invalidation logic

**Integration Testing Focus:**
- Harvesting event system integration
- Crafting system material compatibility
- Database performance under load
- UI responsiveness with large material inventories

**Load Testing Scenarios:**
- Multiple players harvesting simultaneously in same region
- Large numbers of materials in player inventories
- Resource regeneration system under continuous load
- Database queries during peak player activity

## 🎯 **Success Metrics**

**Player Engagement:**
- Average harvesting sessions per player per day
- Resource diversity harvested per player
- Player retention in wilderness areas
- Crafting material market activity

**Economic Impact:**
- Resource price fluctuations based on availability
- Trade route development between regions
- Player specialization in harvesting skills
- Economic dependency on player-harvested materials

**System Performance:**
- Database query response times for harvesting
- Server performance during peak harvesting times
- Cache hit rates for resource data
- Regeneration system computational efficiency

---

## 🚀 **Ready for Development**

Phase 5 represents a major evolution of the Wilderness Resource System from a passive environmental feature to an interactive player-driven economy. The design balances realism with gameplay fun, provides clear progression paths, and creates meaningful choices for players in their harvesting strategies.

**Next Step**: Begin Phase 5a implementation with core harvesting command and basic resource consumption tracking.
