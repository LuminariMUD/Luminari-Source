# Wilderness Resource System Implementation Plan

**Document Version:** 1.0  
**Date:** August 8, 2025  
**Author:** Implementation Planning Team  
**Status:** Planning Phase  

## 📋 **Executive Summary**

This document outlines the implementation plan for a dynamic resource system in the LuminariMUD wilderness. The system integrates Perlin noise, KD-trees, regions, and lazy evaluation to create a comprehensive resource gathering and description enhancement system.

### **Key Design Principles**
- **Lazy Evaluation**: Resources calculated on-demand, no event system dependencies
- **Stability Focus**: Avoid complex event-driven regeneration
- **Seamless Integration**: Build on existing wilderness, region, and harvest systems
- **Performance Optimized**: Memory-efficient with minimal database overhead

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
