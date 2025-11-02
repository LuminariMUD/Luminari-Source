# Phase 7: Ecological Resource Interdependencies

## 🌱 **Overview: Resource Ecosystem Interactions**

**Goal**: Implement realistic ecological relationships where harvesting one resource affects the availability of related resources, creating a complex and dynamic resource ecosystem.

**Core Concept**: Resources don't exist in isolation. Harvesting vegetation affects herbs, mining can destroy crystals, and heavy animal hunting reduces vegetation through ecological pressure.

---

## 🔗 **Ecological Relationship Matrix**

### **Primary Resource Interactions**

```
Harvesting This:     Affects These Resources:
===================  ========================
VEGETATION    →      HERBS (-), GAME (-), CLAY (+)
HERBS         →      VEGETATION (-), GAME (-)
MINERALS      →      CRYSTAL (--), WATER (-), STONE (+)
CRYSTAL       →      MINERALS (-), STONE (-)
WOOD          →      VEGETATION (-), HERBS (-), GAME (-)
GAME          →      VEGETATION (+), HERBS (+)
STONE         →      MINERALS (-), CRYSTAL (-), CLAY (+)
WATER         →      CLAY (+), VEGETATION (+), HERBS (+)
CLAY          →      WATER (-), VEGETATION (-)
SALT          →      WATER (-), VEGETATION (--)
```

**Legend:**
- `(+)` = Positive effect (increases availability over time)
- `(-)` = Negative effect (decreases availability)
- `(--)` = Strong negative effect (major depletion)

---

## 🧬 **Detailed Ecological Relationships**

### **7.1: Vegetation ↔ Herbs Symbiosis**

**Concept**: Herbs and vegetation share root systems and soil nutrients.

**Harvesting Vegetation Effects:**
- **Herbs**: -0.03 per vegetation harvest (trampling, soil disturbance)
- **Game**: -0.02 per harvest (habitat disruption)
- **Clay**: +0.01 per harvest (exposed soil from plant removal)

**Harvesting Herbs Effects:**
- **Vegetation**: -0.02 per herb harvest (root system damage)
- **Game**: -0.01 per harvest (food source reduction)

**Recovery Mechanics:**
- Vegetation and herbs recover 25% faster when both are above 70% availability
- Joint depletion penalty: When both below 30%, recovery 50% slower

### **7.2: Mining ↔ Crystal Competition**

**Concept**: Mining operations physically destroy crystal formations and vice versa.

**Harvesting Minerals Effects:**
- **Crystal**: -0.08 per mineral harvest (heavy mining destroys formations)
- **Water**: -0.03 per harvest (groundwater disruption)
- **Stone**: +0.02 per harvest (exposed stone from mining)

**Harvesting Crystal Effects:**
- **Minerals**: -0.04 per crystal harvest (delicate extraction affects ore veins)
- **Stone**: -0.02 per harvest (precision mining weakens stone)

**Recovery Mechanics:**
- Crystal formations take 3x longer to recover in heavily mined areas
- Mineral deposits recover 2x slower where crystals are being extracted

### **7.3: Forestry ↔ Ecosystem Impact**

**Concept**: Trees provide habitat and soil stability for entire ecosystems.

**Harvesting Wood Effects:**
- **Vegetation**: -0.05 per wood harvest (canopy loss, sunlight changes)
- **Herbs**: -0.04 per harvest (microclimate disruption)
- **Game**: -0.06 per harvest (habitat destruction)
- **Water**: -0.02 per harvest (reduced water retention)

**Recovery Mechanics:**
- Wood recovery enables accelerated vegetation/herb recovery
- Deforested areas (wood <20%) suffer ecosystem collapse penalties

### **7.4: Wildlife ↔ Vegetation Balance**

**Concept**: Animal populations both consume and fertilize vegetation.

**Harvesting Game Effects:**
- **Vegetation**: +0.03 per game harvest (reduced grazing pressure)
- **Herbs**: +0.02 per harvest (reduced trampling)

**Hunting Pressure Thresholds:**
- **Light Hunting** (game >60%): Minimal vegetation impact
- **Moderate Hunting** (game 30-60%): Balanced ecosystem
- **Overhunting** (game <30%): Vegetation boom (+15% growth rate)
- **Extinction** (game <5%): Vegetation overgrowth, then collapse

### **7.5: Water ↔ Life Support Systems**

**Concept**: Water availability affects all biological resources.

**Harvesting Water Effects:**
- **Clay**: +0.04 per water harvest (exposed lakebed/riverbed)
- **Vegetation**: +0.02 per harvest (irrigation effect)
- **Herbs**: +0.03 per harvest (medicinal plants need water)

**Water Depletion Cascades:**
- When water <30%: All biological resources regenerate 30% slower
- When water <10%: Vegetation/herbs start dying off (-0.01/hour)

### **7.6: Geological Resource Interactions**

**Concept**: Geological extraction activities interfere with each other.

**Stone Quarrying Effects:**
- **Minerals**: -0.03 per stone harvest (quarrying disrupts ore seams)
- **Crystal**: -0.05 per harvest (vibrations shatter formations)
- **Clay**: +0.03 per harvest (exposed sediment layers)

**Salt Harvesting Effects:**
- **Water**: -0.06 per salt harvest (brine pool depletion)
- **Vegetation**: -0.08 per harvest (soil salinization)
- **Clay**: +0.02 per harvest (salt flat exposure)

---

## ⚙️ **Implementation Architecture**

### **7.1: Enhanced Depletion System**

**New Database Schema Extension:**
```sql
-- Add cascade tracking to existing resource_depletion table
ALTER TABLE resource_depletion ADD COLUMN cascade_effects TEXT;

-- New table for relationship definitions
CREATE TABLE resource_relationships (
    id INT PRIMARY KEY AUTO_INCREMENT,
    source_resource INT NOT NULL,
    target_resource INT NOT NULL,
    effect_type ENUM('depletion', 'enhancement', 'threshold') NOT NULL,
    effect_magnitude DECIMAL(5,3) NOT NULL,
    threshold_min DECIMAL(4,3) DEFAULT NULL,
    threshold_max DECIMAL(4,3) DEFAULT NULL,
    description VARCHAR(255)
);
```

**Core Function Enhancements:**
```c
/* Enhanced depletion with cascade effects */
void apply_harvest_depletion_with_cascades(room_rnum room, int resource_type, int quantity);

/* Calculate cascade effects for a harvest action */
void apply_cascade_effects(room_rnum room, int source_resource, int quantity);

/* Get relationship strength between two resources */
float get_resource_relationship_strength(int source_resource, int target_resource);

/* Check for threshold-based ecosystem effects */
void check_ecosystem_thresholds(room_rnum room);
```

### **7.2: Ecosystem State Tracking**

**Ecosystem Health Metrics:**
```c
enum ecosystem_states {
    ECOSYSTEM_PRISTINE,      /* All resources >80% */
    ECOSYSTEM_HEALTHY,       /* Most resources >60% */
    ECOSYSTEM_STRESSED,      /* Several resources <40% */
    ECOSYSTEM_DEGRADED,      /* Multiple resources <20% */
    ECOSYSTEM_COLLAPSED      /* Core resources <10% */
};

/* Calculate overall ecosystem health */
int get_ecosystem_state(room_rnum room);

/* Apply ecosystem-wide effects based on health */
void apply_ecosystem_modifiers(room_rnum room, int resource_type, float *base_value);
```

### **7.3: Enhanced Survey System**

**New Survey Commands:**
```
survey ecosystem         - Show ecosystem health and relationships
survey cascade <type>    - Show what harvesting this resource will affect
survey relationships     - Display resource interaction matrix for area
survey impact           - Show your harvesting impact on ecosystem
```

**Ecosystem Survey Output:**
```
Ecosystem Health Analysis for Mountain Ridge (-15, -8):
=====================================================
Overall Health: STRESSED (3 resources critically low)

Resource Interactions:
  Mineral extraction affecting crystal formations (severe)
  Heavy stone quarrying disrupting ore veins (moderate)
  Water depletion affecting vegetation recovery (moderate)

Critical Thresholds:
  Crystal availability: 12% (below sustainable levels)
  Water availability: 28% (ecosystem stress threshold)
  
Recommended Actions:
  • Cease crystal harvesting for 2-3 days
  • Reduce stone quarrying frequency
  • Allow water sources to recover
```

---

## 🎮 **Gameplay Mechanics**

### **7.1: Strategic Resource Management**

**Player Decision Points:**
- **Specialization vs Diversification**: Focus on one resource type vs balanced harvesting
- **Short-term vs Long-term**: Quick gains vs sustainable ecosystem management
- **Competition vs Conservation**: Individual profit vs community resource health

**Resource Synergies:**
- **Sustainable Harvesting**: Taking small amounts of multiple resources maintains ecosystem
- **Cascade Planning**: Understanding which harvests will help/hurt other resources
- **Ecosystem Restoration**: Actively improving degraded areas through selective harvesting

### **7.2: Conservation Incentives**

**Ecosystem Stewardship Rewards:**
```c
/* Conservation bonuses for maintaining ecosystem health */
if (ecosystem_state == ECOSYSTEM_PRISTINE) {
    quality_bonus += 0.2;   /* 20% quality improvement */
    quantity_bonus += 0.1;  /* 10% quantity improvement */
}

if (ecosystem_state == ECOSYSTEM_COLLAPSED) {
    quality_penalty -= 0.4;  /* 40% quality reduction */
    harvest_failure_rate += 0.3; /* 30% more failures */
}
```

**Long-term Player Tracking:**
- **Ecosystem Impact Score**: Track player's cumulative effect on areas
- **Restoration Achievements**: Rewards for improving degraded ecosystems
- **Sustainable Harvester**: Benefits for maintaining ecosystem balance

### **7.3: Competitive Resource Dynamics**

**Resource Competition Events:**
- **Mining Boom**: High mineral demand affects crystal availability server-wide
- **Deforestation Crisis**: Excessive wood harvesting triggers ecosystem warnings
- **Conservation Mandates**: Temporary restrictions on over-exploited resources

**Player vs Player Impact:**
- **Shared Ecosystem**: Multiple players affect same resource areas
- **Conservation vs Exploitation**: Different player strategies compete
- **Restoration Cooperation**: Players can work together to restore areas

---

## 🧪 **Testing & Validation**

### **7.1: Ecosystem Simulation Testing**

**Test Scenarios:**
1. **Heavy Mining Impact**: Mine intensively, verify crystal/water depletion
2. **Deforestation Chain**: Clear wood, track vegetation/herb/game effects
3. **Overhunting Recovery**: Reduce game to <10%, verify vegetation response
4. **Ecosystem Collapse**: Deplete multiple core resources, verify cascade effects
5. **Recovery Patterns**: Stop harvesting, verify restoration timelines

**Expected Behaviors:**
- Cascades should be gradual, not instant ecosystem collapse
- Recovery should follow realistic timelines (hours to days)
- Players should have clear feedback about their impact

### **7.2: Balance Validation**

**Metrics to Monitor:**
- **Harvest Success Rates**: Ensure viable gameplay despite interactions
- **Resource Availability**: Prevent permanent ecosystem dead zones
- **Player Progression**: Conservation shouldn't completely halt advancement
- **Server Performance**: Cascade calculations shouldn't impact performance

**Adjustment Mechanisms:**
- **Cascade Dampening**: Reduce effect strengths if too punitive
- **Recovery Acceleration**: Speed up regeneration if too slow
- **Threshold Tuning**: Adjust ecosystem state boundaries

---

## 📋 **Implementation Timeline**

### **Week 1: Core Cascade System**
- Implement `apply_cascade_effects()` function
- Add relationship matrix to database
- Basic cascade depletion calculations

### **Week 2: Ecosystem Health Tracking**
- Implement ecosystem state calculation
- Add ecosystem-wide modifiers
- Database schema for cascade tracking

### **Week 3: Enhanced Survey & Feedback**
- New survey commands for ecosystem analysis
- Player impact tracking and display
- Cascade preview functionality

### **Week 4: Balance Testing & Refinement**
- Extensive ecosystem simulation testing
- Player feedback integration
- Performance optimization

### **Week 5: Advanced Features**
- Conservation incentive systems
- Ecosystem restoration mechanics
- Competition/cooperation systems

---

## 🎯 **Success Metrics**

### **Technical Metrics**
- **Performance**: Cascade calculations <2ms per harvest
- **Database Efficiency**: Relationship queries optimized
- **Memory Usage**: Ecosystem state caching effective

### **Gameplay Metrics**
- **Strategic Depth**: Players make meaningful ecosystem decisions
- **Conservation Engagement**: 60%+ players maintain ecosystem health
- **Resource Diversity**: Players harvest variety of resources, not just optimals
- **Long-term Viability**: Sustainable areas show 5x+ longevity

### **Player Experience Metrics**
- **Educational Value**: Players understand ecological relationships
- **Strategic Planning**: Harvest decisions consider ecosystem impact
- **Social Cooperation**: Players coordinate for ecosystem management
- **Progression Balance**: Conservation doesn't halt character advancement

---

**Phase 7 transforms the wilderness resource system from individual resource management into a complex, interconnected ecosystem where every harvesting decision has far-reaching ecological consequences.**

---

*Document Version: 1.0*  
*Last Updated: August 11, 2025*  
*Status: READY FOR IMPLEMENTATION*
