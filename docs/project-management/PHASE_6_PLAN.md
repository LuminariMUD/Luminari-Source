# Phase 6: Wilderness Resource Ecosystem & Advanced Features

**Date:** August 10, 2025  
**Status:** Planning & Implementation  
**Dependencies:** Phase 4.5 (Enhanced Integration) ✅, Phase 5 (Harvesting Commands) ✅  

---

## 🎯 **Phase 6 Overview**

Phase 6 transforms the wilderness resource system from basic gathering into a dynamic, sustainable ecosystem with advanced features. This phase offers multiple implementation paths based on priorities and complexity preferences.

### **Current Foundation:**
- ✅ **Phase 4.5**: Enhanced wilderness-crafting integration with campaign safety
- ✅ **Phase 5**: Complete harvesting system (harvest/gather/mine commands)
- 🎨 **Phase 5.1**: Enhanced materials display with proper color formatting

### **Phase 6 Implementation Options:**

---

## 🌱 **Phase 6: Resource Depletion & Regeneration System (Primary)**

**Objective:** Create a dynamic ecosystem where resources deplete with use and regenerate over time, adding environmental realism and strategic depth.

### **6.1: Core Depletion Mechanics**

#### **Dynamic Resource Tracking**
- **Per-Location Resource Levels**: Each wilderness coordinate tracks resource abundance (0.0 to 1.0)
- **Harvesting Impact**: Each harvest attempt reduces local resource availability
- **Progressive Depletion**: Heavy harvesting makes resources "scarce" → "depleted" → "exhausted"
- **Quality Degradation**: Over-harvested areas yield progressively lower quality materials

#### **Depletion Formula**
```c
new_level = current_level - (harvest_impact * depletion_rate * environmental_factors)
```

#### **Harvesting Impact Tiers**
- **Light Use** (1-5 harvests): -0.02 per harvest
- **Moderate Use** (6-15 harvests): -0.05 per harvest  
- **Heavy Use** (16+ harvests): -0.10 per harvest
- **Critical Depletion** (<0.1 level): Harvesting may fail completely

### **6.2: Regeneration System**

#### **Time-Based Recovery**
- **Base Regeneration**: +0.01 resource level per hour (real-time)
- **Environmental Multipliers**: Terrain, weather, season affect regeneration rates
- **Undisturbed Bonus**: Areas without recent harvesting regenerate 2x faster
- **Maximum Recovery**: Resources can exceed base level (up to 1.5) in pristine areas

#### **Environmental Factors**
```c
regeneration_rate = base_rate * terrain_modifier * weather_modifier * season_modifier * conservation_bonus
```

- **Terrain Modifiers**: Forest (1.5x), Plains (1.0x), Desert (0.5x), Mountains (0.8x)
- **Weather Effects**: Rain (+25%), Drought (-50%), Snow (0x for vegetation)
- **Seasonal Cycles**: Spring (+50%), Summer (1.0x), Autumn (0.75x), Winter (0.25x)

### **6.3: Advanced Ecosystem Features**

#### **Resource Migration**
- **Spreading Recovery**: Healthy areas slowly boost neighboring depleted zones
- **Migration Rate**: 0.005 per hour from adjacent high-abundance areas
- **Natural Barriers**: Rivers, mountains slow migration between regions

#### **Conservation Mechanics**
- **Sustainable Harvesting**: Taking only 1-2 materials per location per day maintains resources
- **Conservation Bonus**: Players who harvest sustainably get quality bonuses
- **Ecological Reports**: `survey conservation` shows area health and player impact

#### **Player Impact Tracking**
- **Individual Statistics**: Track harvesting patterns per player
- **Server-Wide Impact**: Monitor global resource health
- **Conservation Achievements**: Reward sustainable harvesting practices

### **6.4: Technical Implementation**

#### **Database Schema**
```sql
CREATE TABLE resource_depletion (
    x_coord INT NOT NULL,
    y_coord INT NOT NULL,
    resource_type TINYINT NOT NULL,
    current_level FLOAT DEFAULT 1.0,
    last_harvest TIMESTAMP NULL,
    harvest_count INT DEFAULT 0,
    total_harvested INT DEFAULT 0,
    last_regeneration TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    regeneration_bonus FLOAT DEFAULT 1.0,
    PRIMARY KEY (x_coord, y_coord, resource_type),
    INDEX idx_coords (x_coord, y_coord),
    INDEX idx_regeneration (last_regeneration)
);

CREATE TABLE player_conservation_stats (
    player_id INT NOT NULL,
    resource_type TINYINT NOT NULL,
    total_harvested INT DEFAULT 0,
    sustainable_harvests INT DEFAULT 0,
    conservation_score FLOAT DEFAULT 1.0,
    last_updated TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (player_id, resource_type)
);
```

#### **Code Architecture**
```c
// New files to create:
src/resource_depletion.c/h    - Core depletion and regeneration logic
src/wilderness_ecology.c/h    - Environmental factors and conservation
src/conservation_tracking.c/h - Player impact and achievements

// Functions to implement:
float get_resource_depletion_level(int x, int y, int resource_type);
void apply_harvest_depletion(int x, int y, int resource_type, int quantity);
void regenerate_resources_background(void);
void update_conservation_score(struct char_data *ch, int resource_type, bool sustainable);
```

### **6.5: User Experience Enhancements**

#### **New Commands**
```
survey conservation              - Show ecological health of current area
survey depletion [resource]      - Check specific resource depletion levels  
survey regeneration              - View regeneration rates and factors
conservation stats               - Display player's conservation statistics
conservation leaderboard         - Server-wide conservation rankings
```

#### **Enhanced Survey Output**
```
survey resources
```
**Enhanced Output:**
```
Resource Availability Analysis:
=====================================
  Herbs       : abundant (98%) [healthy - regenerating]
  Vegetation  : moderate (65%) [harvested - slow recovery]  
  Minerals    : scarce (23%) [depleted - needs restoration]
  Wood        : exhausted (5%) [critical - avoid harvesting]

Conservation Status: GOOD
Your Impact: Sustainable Harvester (score: 4.2/5.0)
Regeneration: Active (+0.015/hour average)
```

---

## ⚡ **Phase 6A: Enhanced Crafting Integration (Alternative)**

**Objective:** Deepen the connection between wilderness materials and crafting systems with advanced recipes and mechanics.

### **6A.1: Recipe Discovery System**

#### **Wilderness-Specific Recipes**
- **Hidden Recipes**: Discovered through experimentation with wilderness materials
- **Quality Requirements**: Some recipes only work with uncommon+ quality materials
- **Regional Recipes**: Certain combinations only possible in specific wilderness areas
- **Seasonal Recipes**: Some recipes available only during certain seasons

#### **Recipe Discovery Mechanics**
```c
// Recipe discovery triggers
if (has_wilderness_materials(ch, recipe_requirements)) {
    if (dice(1, 100) <= discovery_chance) {
        learn_wilderness_recipe(ch, recipe_id);
        send_to_char(ch, "You discover a new wilderness crafting technique!\r\n");
    }
}
```

### **6A.2: Quality Bonus Stacking**

#### **Multi-Material Synergy**
- **Quality Stacking**: Combine multiple high-quality materials for bonus effects
- **Material Harmony**: Certain material combinations provide special bonuses
- **Legendary Combinations**: Rare recipes requiring multiple legendary materials

#### **Synergy Examples**
```
Legendary Oak + Legendary Iron = +3 enchantment bonus
Rare Herbs + Common Clay = Healing properties
Uncommon Crystal + Any Metal = Magical weapon potential
```

### **6A.3: Specialized Wilderness Crafting**

#### **Unique Crafting Stations**
- **Wilderness Forges**: Build temporary crafting stations in the wilderness
- **Natural Materials**: Use environment itself as crafting tools
- **Elemental Infusion**: Craft during specific weather for elemental bonuses

---

## 📦 **Phase 6B: Advanced Material Storage (Alternative)**

**Objective:** Enhance material management with organization, upgrades, and trading systems.

### **6B.1: Material Organization System**

#### **Smart Categorization**
```
materials list herbs             - Show only herb materials
materials list quality rare      - Show only rare+ quality materials  
materials list crafting weapons  - Show materials useful for weapons
materials search iron            - Find all materials containing "iron"
materials sort quality          - Sort by quality level
materials sort quantity         - Sort by quantity
```

#### **Storage Categories**
- **By Type**: herbs, minerals, wood, etc.
- **By Quality**: poor, common, uncommon, rare, legendary
- **By Application**: weapons, armor, alchemy, etc.
- **By Rarity**: common, uncommon, rare materials

### **6B.2: Storage Upgrades**

#### **Capacity Expansion**
- **Base Storage**: 100 material slots
- **Upgrade Tiers**: Craft storage containers for +25/+50/+100 slots
- **Specialized Storage**: Containers for specific material types
- **Preservation**: Better containers reduce quality degradation

#### **Storage Management**
```c
// Storage upgrade system
struct material_storage_upgrade {
    int storage_type;           // GENERAL, HERBS, MINERALS, etc.
    int bonus_slots;           // Additional storage capacity
    float preservation_bonus;   // Quality preservation multiplier
    int durability;            // How long upgrade lasts
};
```

### **6B.3: Material Trading System**

#### **Player-to-Player Trading**
- **Trade Posts**: Leave materials for other players
- **Material Exchange**: Trade materials for other materials
- **Quality Trading**: Exchange quantity for quality
- **Regional Markets**: Different regions value different materials

---

## 🎓 **Phase 6C: Skill Progression System (Alternative)**

**Objective:** Implement comprehensive skill advancement for harvesting with specialization trees and mastery benefits.

### **6C.1: Skill Advancement**

#### **Dynamic Skill Improvement**
```c
// Skill improvement on successful harvest
if (harvest_successful) {
    improve_skill(ch, get_harvest_skill(resource_type));
    
    // Bonus improvement for difficult harvests
    if (resource_availability < 0.3) {
        improve_skill(ch, get_harvest_skill(resource_type));
    }
}
```

#### **Skill Progression Tiers**
- **Novice** (0-25): Basic harvesting, frequent failures
- **Apprentice** (26-50): Improved success rates, occasional quality bonus
- **Journeyman** (51-75): Consistent success, regular quality improvements
- **Expert** (76-90): High success rates, frequent rare materials
- **Master** (91-100): Rare failures, consistent high-quality yields

### **6C.2: Specialization Trees**

#### **Resource-Specific Specializations**
```
Herbalist Specialization:
- Botanical Knowledge: +25% herb identification success
- Gentle Harvesting: +15% chance to preserve plant for regrowth
- Herb Lore: Discover medicinal properties of rare herbs

Miner Specialization:  
- Ore Sense: Detect high-quality mineral veins
- Efficient Extraction: +20% quantity from mineral harvests
- Gem Cutting: Process raw crystals into refined gems
```

#### **Cross-Resource Mastery**
- **Wilderness Expert**: Bonuses to all resource types
- **Sustainable Harvester**: Environmental conservation bonuses
- **Quality Seeker**: Increased chance for rare quality materials

### **6C.3: Master Harvester Benefits**

#### **Advanced Abilities**
- **Resource Sense**: Detect resource quality before harvesting
- **Efficient Harvesting**: Chance to harvest without depleting resources
- **Weather Reading**: Predict optimal harvesting conditions
- **Teaching**: Share knowledge with other players

---

## 🚀 **Phase 6 Implementation Plan**

### **Recommended Implementation Order:**

#### **Phase 6 (Primary): Resource Depletion & Regeneration**
**Rationale:** Creates fundamental ecosystem dynamics that enhance all other features

#### **Phase 6A: Enhanced Crafting Integration**  
**Rationale:** Builds on existing crafting system, adds immediate value to materials

#### **Phase 6C: Skill Progression System**
**Rationale:** Enhances player engagement and progression satisfaction

#### **Phase 6B: Advanced Material Storage**
**Rationale:** Quality-of-life improvements, can be implemented incrementally

### **Development Timeline:**

#### **Week 1-2: Phase 6 Core (Depletion & Regeneration)**
- Database schema implementation
- Core depletion mechanics
- Basic regeneration system
- Enhanced survey commands

#### **Week 3: Phase 6A (Enhanced Crafting)**
- Recipe discovery system
- Quality stacking mechanics
- Wilderness-specific recipes

#### **Week 4: Phase 6C (Skill Progression)**
- Skill improvement system
- Specialization framework
- Master harvester abilities

#### **Future: Phase 6B (Storage Enhancements)**
- Material organization commands
- Storage upgrade system
- Trading mechanics

---

## 🎯 **Success Criteria**

### **Phase 6 (Depletion & Regeneration):**
- ✅ Resource levels decrease with harvesting
- ✅ Resources regenerate over time
- ✅ Environmental factors affect regeneration
- ✅ Conservation mechanics reward sustainable practices
- ✅ Survey commands show ecosystem health

### **Phase 6A (Enhanced Crafting):**
- ✅ Recipe discovery system functional
- ✅ Quality stacking provides meaningful bonuses
- ✅ Wilderness-specific recipes available

### **Phase 6C (Skill Progression):**
- ✅ Skills improve with use
- ✅ Specialization trees provide distinct benefits
- ✅ Master level abilities unlock

---

## 📊 **Phase 6 Benefits Summary**

### **Environmental Realism**
- Creates believable ecosystem dynamics
- Encourages exploration and conservation
- Adds strategic depth to resource gathering

### **Player Engagement**
- Long-term progression goals
- Meaningful choices in harvesting strategies
- Social dynamics through conservation competition

### **Economic Balance**
- Prevents infinite resource exploitation
- Creates natural supply/demand cycles
- Rewards sustainable gameplay

### **World Immersion**
- Living world that responds to player actions
- Seasonal and environmental variation
- Consequence-driven gameplay

---

**Next Steps:** Begin Phase 6 implementation with resource depletion and regeneration system, followed by enhanced crafting integration and skill progression as development priorities allow.
