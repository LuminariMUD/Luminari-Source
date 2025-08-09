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

**Skill Integration:**
- **Survival Skill**: Primary skill for wilderness harvesting
- **Secondary Skills**: Specific skills for specialized resources
  - Herbalism: Herbs and medicinal plants
  - Mining: Minerals and stone
  - Foraging: Vegetation and food sources
  - Hunting: Game and animal resources

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

### 7. Integration with Existing Systems

**Crafting System Integration:**
- Harvested materials become crafting components
- Quality affects crafting success and final item stats
- Rare resources enable unique recipes

**Economy Integration:**
- Player shops can buy/sell harvested materials
- Regional price variations based on local availability
- Trade routes emerge based on resource distribution

**Experience and Advancement:**
- Successful harvesting grants skill experience
- Discovering new resource nodes provides exploration bonuses
- Mastery achievements for resource specialization

## 🗄️ **Database Schema Extensions**

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
