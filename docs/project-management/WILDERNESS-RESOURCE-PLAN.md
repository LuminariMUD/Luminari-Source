# Wilderness Resource System Implementation Plan

**Document Version:** 3.0  
**Date:** August 8, 2025  
**Author:** Implementation Team  
**Status:** ✅ **Phases 1-4 COMPLETED** - Region Integration Complete  

## 📋 **Executive Summary**

This document outlines the implementation plan for a dynamic resource system in the LuminariMUD wilderness. The system integrates Perlin noise, KD-trees, regions, and lazy evaluation to create a comprehensive resource gathering and description enhancement system.

**🎉 IMPLEMENTATION STATUS:**
- ✅ **Phase 1**: Core Infrastructure (COMPLETED)
- ✅ **Phase 2**: Enhanced Survey Commands (COMPLETED)  
- ✅ **Phase 3**: Spatial Caching System (COMPLETED)
- ✅ **Phase 4**: Region Integration (COMPLETED)
- 🔲 **Phase 5**: Harvesting Mechanics (PLANNED)

### **Key Design Principles**
- **Lazy Evaluation**: Resources calculated on-demand, no event system dependencies ✅
- **Stability Focus**: Avoid complex event-driven regeneration ✅
- **Seamless Integration**: Build on existing wilderness, region, and harvest systems ✅
- **Performance Optimized**: Memory-efficient with spatial caching ✅

---

## 🚀 **IMPLEMENTATION STATUS & TESTING**

### **✅ Phase 1: Core Infrastructure (COMPLETED)**

**Key Components Implemented:**
- ✅ Resource system header (`resource_system.h`) with all data structures
- ✅ Resource calculation engine (`resource_system.c`) with Perlin noise integration
- ✅ 10 resource types with individual Perlin noise layers (4-11)
- ✅ Environmental and region modifier framework
- ✅ KD-tree integration for harvest history tracking
- ✅ Fixed LIMIT macro issues for proper resource value calculation

**Files Modified:**
- `src/resource_system.h` - Complete header with structures and declarations
- `src/resource_system.c` - Full implementation with lazy evaluation
- `Makefile.am` - Added resource_system.c to build process

### **✅ Phase 2: Enhanced Survey Commands (COMPLETED)**

**User Commands Implemented:**
- ✅ `survey resources` - Shows all resource percentages at current location
- ✅ `survey map <resource>` - Visual ASCII minimap with colored resource density
- ✅ `survey detail <resource>` - Detailed information about specific resource
- ✅ `survey terrain` - Environmental factors affecting resources

**Files Modified:**
- `src/act.informative.c` - Enhanced survey command with multiple modes
- Fixed argument parsing for proper sub-command handling

### **✅ Phase 3: Spatial Caching System (COMPLETED)**

**Performance Features Implemented:**
- ✅ Grid-based spatial caching (10x10 coordinate grid)
- ✅ KD-tree cache storage with automatic expiration (5 minutes)
- ✅ Batch resource calculation (all types cached together)
- ✅ Memory management with 1000 node limit
- ✅ Cache statistics and management tools

**Admin Commands Implemented:**
- ✅ `resourceadmin status` - System status with cache statistics
- ✅ `resourceadmin here` - Resources at current location
- ✅ `resourceadmin coords <x> <y>` - Resources at specific coordinates
- ✅ `resourceadmin map <type> [radius]` - Resource minimap
- ✅ `resourceadmin debug` - Comprehensive debug information with cache stats
- ✅ `resourceadmin cache` - Cache management (show/cleanup/clear)
- ✅ `resourceadmin cleanup` - Force cleanup of old resource nodes

**Files Modified:**
- `src/resource_system.h` - Added cache structures and function declarations
- `src/resource_system.c` - Implemented full caching system
- `src/act.wizard.c` - Added resourceadmin command with cache management
- `src/interpreter.c` - Registered resourceadmin command

### **✅ Phase 4: Region Integration (COMPLETED)**

**Region Resource System Features Implemented:**
- ✅ Database integration for region-specific resource modifiers
- ✅ Region resource effects table with multipliers and bonuses
- ✅ Automatic region detection and modifier application
- ✅ Multiple region support with cumulative effects
- ✅ Enhanced debug survey showing region effects

**Database Schema Implemented:**
- ✅ **New**: `region_effects` table with flexible effect definitions using JSON parameters
- ✅ **New**: `region_effect_assignments` table for foreign key-based region targeting
- ✅ Example effects for resource multipliers, seasonal modifiers, and environmental factors
- ✅ Proper indexing for performance optimization

**Admin Commands Added:**
- ✅ `resourceadmin effects list` - List all available region effects
- ✅ `resourceadmin effects show <effect_id>` - Show detailed effect information
- ✅ `resourceadmin effects assign <region_vnum> <effect_id> <intensity>` - Assign effect to region
- ✅ `resourceadmin effects unassign <region_vnum> <effect_id>` - Remove effect from region
- ✅ `resourceadmin effects region <region_vnum>` - Show all effects for a region

**Region Effect Examples:**
- **Forest Effects**: Enhanced vegetation growth (1.5x), abundant herbs (1.8x), rich wood sources (2.0x), reduced minerals (0.3x)
- **Mountain Effects**: Rich mineral deposits (2.5x), abundant stone (3.0x), crystal formations (2.0x), sparse vegetation (0.4x)
- **Seasonal Effects**: Spring growth bonuses, winter penalties, wet season flooding
- **Environmental Effects**: Drought conditions, magical enhancement, cursed lands

**Files Modified:**
- `src/resource_system.h` - Added flexible region effects function prototypes
- `src/resource_system.c` - Implemented JSON-based effects processing and region integration
- `src/act.wizard.c` - Added comprehensive effects management subcommands to resourceadmin
- `lib/region_effects_system.sql` - **New flexible database schema** with JSON parameters

---

## 🧪 **TESTING GUIDE**

### **Basic Functionality Testing**

1. **Enter Wilderness Area**
   ```
   - Navigate to any wilderness zone (forest, plains, mountains, etc.)
   - Verify you're in wilderness with: whereis
   ```

2. **Test Basic Survey Commands**
   ```
   survey resources           # Shows all 10 resource types with percentages
   survey map vegetation     # Shows 21x21 ASCII map with colored vegetation density
   survey detail minerals    # Detailed info about minerals at current location
   survey terrain           # Environmental factors affecting resources
   ```

3. **Test Resource Minimap Visualization**
   ```
   survey map water 10      # 21x21 water resource map
   survey map herbs 5       # 11x11 herbs resource map
   survey map game          # Default 10 radius game animal map
   ```

4. **Move Around and Test Consistency**
   ```
   - Move to different coordinates in wilderness
   - Run survey commands at multiple locations
   - Verify resources change naturally with terrain
   - Return to previous location - values should be identical (cached)
   ```

### **Admin Testing**

1. **System Status**
   ```
   resourceadmin status     # Shows system info and cache statistics
   ```

2. **Location Testing**
   ```
   resourceadmin here       # Resources at current location
   resourceadmin coords -100 50  # Resources at specific coordinates
   ```

3. **Debug Analysis**
   ```
   resourceadmin debug      # Comprehensive debug with:
                           # - Raw Perlin noise values
                           # - Calculation breakdown
                           # - Cache hit/miss information
                           # - Environmental modifiers
   ```

4. **Cache Management**
   ```
   resourceadmin cache      # Show cache statistics and commands
   resourceadmin cache cleanup  # Remove expired cache entries
   resourceadmin cache clear    # Clear all cache entries
   ```

5. **Region Effects System Testing** *(Phase 4 - Updated)*
   ```
   # Install the flexible region effects database schema:
   mysql -u username -p database_name < lib/region_effects_system.sql
   
   # Test effects management
   resourceadmin effects list                    # Show all available effects
   resourceadmin effects show 1                  # Show details for effect ID 1
   
   # Test region effect assignments
   resourceadmin effects assign 1001 1 1.5       # Assign effect 1 to region 1001 with intensity 1.5
   resourceadmin effects region 1001             # Show all effects for region 1001
   resourceadmin effects unassign 1001 1         # Remove effect 1 from region 1001
   
   # Enhanced debug with region analysis
   resourceadmin debug                 # Now includes region modifier breakdown
   ```

### **Performance Testing**

1. **Cache Hit Testing**
   ```
   # First visit (should be cache MISS)
   resourceadmin debug
   
   # Immediate second visit (should be cache HIT)
   resourceadmin debug
   
   # Move around same area and return (should still be cache HIT)
   ```

2. **Cache Expiration Testing**
   ```
   # Check cache stats
   resourceadmin cache
   
   # Wait 5+ minutes or use cache clear
   resourceadmin cache clear
   
   # Verify cache rebuilds on next access
   resourceadmin debug
   ```

### **Expected Resource Ranges**

Different terrain types should show different resource patterns:

**Forest Areas:**
- High: vegetation (60-80%), wood (50-70%), herbs (30-50%)
- Medium: game (30-50%), water (20-40%)
- Low: minerals (10-20%), stone (10-20%), crystal (2-8%)

**Mountain Areas:**
- High: stone (50-70%), minerals (40-60%), crystal (5-15%)
- Medium: water (20-40%), vegetation (20-40%)
- Low: herbs (10-20%), wood (10-20%), game (10-20%)

**Plains/Field Areas:**
- High: vegetation (50-70%), game (40-60%)
- Medium: herbs (30-50%), water (30-50%)
- Low: wood (10-30%), minerals (10-20%), stone (10-20%)

---

## 📦 **Core Resource Types**

The system supports a variety of gatherable and trackable resources:

- **Vegetation** – grasses, shrubs, general plant life
- **Minerals** – ores, metals, stones  
- **Water** – fresh water sources
- **Herbs** – medicinal or magical plants
- **Game** – huntable animals
- **Wood** – harvestable timber
- **Stone** – building materials
- **Crystal** – rare magical components
- **Clay** – crafting materials
- **Salt** – preservation materials

Each type stored independently, modified by terrain, region effects, and player interaction through lazy evaluation.

---

## 🧠 **Resource Calculation: Perlin Noise Foundation**

Each coordinate receives base resource values using **Perlin noise** for natural distribution patterns.

- **Per-resource noise layers**: Each resource type uses dedicated Perlin layer
- **Consistent generation**: Same values on subsequent calculations
- **Tunable parameters**: Frequency and amplitude per resource type
- **Environmental modifiers**: Seasonal and weather effects applied dynamically

```c
// Example calculation flow
float base_value = get_base_resource_value(RESOURCE_VEGETATION, x, y);
base_value = apply_region_modifiers(resource_type, x, y, base_value);
base_value = apply_environmental_modifiers(resource_type, x, y, base_value);
return apply_harvest_effects(resource_type, x, y, base_value);
```

---

## 📍 **Region-Based Modifiers**

Regions stored as polygons in MySQL can modify resource values:

- **Multipliers** (e.g., vegetation × 1.5 in forests)
- **Additive bonuses** (e.g., minerals +20% in mountains)  
- **Resource-specific effects** per region type
- **Simple database schema** for easy configuration

```sql
-- Simple region effects table
CREATE TABLE region_resource_effects (
    region_vnum INT,
    resource_type INT,
    multiplier FLOAT DEFAULT 1.0,
    bonus FLOAT DEFAULT 0.0
);
```

---

## 🌳 **Resource Tracking: KD-Tree Storage**

A **KD-tree** holds harvest history and consumption data keyed by `(x, y)`.

- **Harvest tracking only**: Only stores data for locations that have been harvested
- **Memory efficient**: Automatic cleanup of old, fully-regenerated nodes  
- **Lazy regeneration**: Time-based regeneration calculated on access
- **No events required**: Pure mathematical regeneration

```c
struct resource_node {
    int x, y;
    float consumed_amount[NUM_RESOURCE_TYPES];  // How much harvested
    time_t last_harvest[NUM_RESOURCE_TYPES];    // When harvested
    int harvest_count[NUM_RESOURCE_TYPES];      // Frequency tracking
};
```

---

## 🔄 **Lazy Regeneration System**

Resources regenerate through pure calculation, no events needed:

- **Time-based recovery**: Based on hours since last harvest
- **Configurable rates**: Per resource type regeneration speed
- **Environmental factors**: Weather and seasonal modifiers
- **Depletion resistance**: Diminishing returns on over-harvesting

```c
// Regeneration calculation example
float hours_passed = (now - last_harvest) / 3600.0;
float regenerated = consumed * regen_rate_per_hour * hours_passed;
consumed = MAX(0.0, consumed - regenerated);
current_value = base_value * (1.0 - consumed);
```

---

## 🧭 **Enhanced Survey Command**

Complete replacement of existing survey with resource-focused functionality:

### **Survey Options**
- `survey resources` – Local resource availability overview
- `survey map <type>` – Visual resource density map with color coding
- `survey detail <type>` – Detailed analysis of specific resource
- `survey terrain` – Basic terrain information
- `survey debug` – Admin debugging information

### **Resource Maps**
- **Color-coded density**: Visual representation of resource distribution  
- **5x5 grid display**: Centered on player position
- **Symbol legend**: # = Very Rich, * = Rich, + = Moderate, . = Poor
- **Player position marker**: Clear indication of current location

### **Detailed Analysis**
- **Current vs base levels**: Show environmental effects
- **Modifier breakdown**: Seasonal, weather, and regional effects
- **Harvest history**: Previous harvests and regeneration status
- **Quality assessment**: Potential yield and quality indicators

---

## ⚙️ **Environmental Modifiers**

### **Seasonal Effects**
- **Vegetation/Herbs**: High in spring (180%), low in winter (30%)
- **Game**: Breeding seasons affect availability
- **Water**: Consistent year-round with weather dependency
- **Minerals/Stone/Crystal**: Unaffected by seasons

### **Weather Effects**  
- **Water resources**: Dramatically increased during rain/storms
- **Vegetation**: Moderate boost from precipitation
- **Game**: Minimal weather impact
- **Underground resources**: Weather-independent

---

## 🧪 **Administration & Debug Tools**

### **Resource Admin Commands**
```
resource cleanup          - Remove old, unused resource nodes
resource reset <x> <y>    - Clear harvest history at coordinate  
survey debug              - Show detailed calculation breakdown
```

### **Debug Information**
- **Base Perlin values**: Raw noise generation results
- **Region modifier chains**: Step-by-step modification process
- **Environmental factors**: Current seasonal and weather effects
- **Harvest impact**: Depletion levels and regeneration progress
- **Memory usage**: KD-tree node counts and cleanup status

---

## 🔗 **System Integration**

### **Wilderness Integration**
- **Coordinate system**: Uses existing wilderness coordinate space (-1024 to +1024)
- **Noise layers**: Extends current Perlin noise infrastructure  
- **Region system**: Leverages existing MySQL spatial queries
- **Terrain types**: Resource distribution varies by sector type

### **Harvest System Integration**
- **Skill dependency**: Resource yield based on character harvesting skills
- **Material conversion**: Resources convert to existing crafting materials
- **Quality tiers**: Higher resource levels yield better quality materials
- **Depletion tracking**: Harvest actions recorded in KD-tree

### **Dynamic Description Integration**
- **Context-aware descriptions**: Resource levels influence room descriptions
- **Sector-specific content**: Different descriptions per terrain type
- **Abundance indicators**: Subtle hints about resource availability
- **Environmental storytelling**: Rich descriptions based on resource state

---

## 📊 **Implementation Phases**

### **Phase 1: Core Infrastructure**
1. Resource type definitions and configuration system
2. Perlin noise layer extensions for resource types  
3. Basic KD-tree resource node structure
4. Core calculation functions with lazy evaluation

### **Phase 2: Region Integration**
1. Region resource modifier data structures
2. Simple database schema for region effects
3. Region modifier application in calculation chain
4. Database loading and caching system

### **Phase 3: Survey Command Replacement**
1. Complete survey command rewrite
2. Resource overview and mapping functionality
3. Detailed resource analysis features
4. Color-coded visual representation system

### **Phase 4: Environmental Systems**
1. Seasonal modifier calculations
2. Weather effect integration
3. Environmental factor application
4. Dynamic modifier system

### **Phase 5: Harvest Integration**
1. Resource-based harvest modifications
2. Quality tier determination system
3. Skill-based yield calculations
4. Material conversion and rewards

### **Phase 6: Dynamic Description Enhancement**
1. Resource-aware description generation
2. Context-sensitive environmental storytelling
3. Abundance-based description variants
4. Integration with existing description engine

### **Phase 7: Administration & Polish**
1. Admin command implementation
2. Debug and diagnostic tools
3. Performance optimization
4. Memory management and cleanup systems

---

## 🎯 **Success Metrics**

### **Technical Metrics**
- **Performance**: Resource calculations complete in <1ms per request
- **Memory**: KD-tree nodes automatically pruned to prevent memory leaks
- **Stability**: No event system dependencies, pure calculation-based
- **Scalability**: Handles full wilderness coordinate space efficiently

### **Gameplay Metrics**  
- **Resource Discovery**: Players can locate and map resource distributions
- **Meaningful Choices**: Resource quality varies significantly by location
- **Exploration Incentive**: Rich resource areas encourage wilderness exploration
- **Economic Impact**: Resource scarcity/abundance affects player behavior

### **Content Metrics**
- **Description Enhancement**: Dynamic descriptions reflect resource state
- **Environmental Storytelling**: Rich, context-aware environmental descriptions
- **Immersion**: Seamless integration with existing wilderness experience
- **Builder Tools**: Easy configuration of regional resource effects

---

## 🚀 **Future Enhancements**

### **Advanced Features**
- **Resource Competition**: Multiple resource types competing for space
- **Magical Influences**: Spells and artifacts affecting resource regeneration
- **Faction Control**: Player/NPC faction control affecting resource access
- **Trade Networks**: Resource transport and trade route systems

### **Quality of Life**
- **Resource Tracking**: Player maps showing discovered resource locations
- **Prediction System**: Weather forecasting affecting resource planning
- **Notification System**: Alerts for resource regeneration completion
- **Batch Operations**: Tools for mass resource area configuration

### **Integration Opportunities**
- **Quest System**: Dynamic quests based on resource availability
- **Economy**: Market prices fluctuating with resource scarcity
- **Crafting**: Advanced recipes requiring specific resource qualities
- **Events**: Staff events modifying global or regional resource levels

---

## 📋 **Implementation Checklist**

### **Phase 1 Tasks**
- [ ] Create `src/resource_system.h` with core data structures
- [ ] Extend `wilderness.h` with new Perlin noise layer definitions  
- [ ] Implement basic resource calculation functions
- [ ] Add KD-tree resource node management
- [ ] Create resource configuration system

### **Phase 2 Tasks**
- [ ] Extend region data structures for resource effects
- [ ] Create database schema for region resource modifiers
- [ ] Implement region modifier loading and application
- [ ] Add region effect calculation chain

### **Phase 3 Tasks**
- [ ] Replace existing survey command completely
- [ ] Implement resource overview functionality
- [ ] Create resource mapping with color coding
- [ ] Add detailed resource analysis features

### **Deployment Readiness**
- [ ] Code review and testing completed
- [ ] Database migration scripts prepared
- [ ] Admin documentation updated
- [ ] Player help files created
- [ ] Performance benchmarking completed

---

## 📝 **Notes and Considerations**

### **Design Decisions**
- **Lazy evaluation chosen** over event system for stability and simplicity
- **KD-tree storage** minimizes memory usage by only tracking harvested locations
- **Region integration** leverages existing spatial query infrastructure
- **Survey replacement** provides significantly enhanced functionality

### **Performance Considerations**
- **On-demand calculation** ensures predictable performance characteristics
- **Memory cleanup** prevents long-term memory growth from resource tracking
- **Database optimization** uses existing spatial indexes for region queries
- **Caching opportunities** exist for frequently accessed calculations

### **Maintenance Considerations**
- **Simple configuration** makes resource balancing straightforward
- **Debug tools** provide insight into calculation processes
- **Admin commands** offer direct control over resource state
- **Modular design** allows incremental feature addition

---

**Document End**

*This document serves as the master plan for implementing the wilderness resource system in LuminariMUD. All implementation should refer to this document for consistency and completeness.*
