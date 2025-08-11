# Phase 7: Implementation Quick Start Guide

## 🚀 **Getting Started with Ecological Resource Interdependencies**

This guide shows how to implement the Phase 7 cascade system that makes harvesting one resource affect related resources, creating realistic ecological interactions.

---

## 📋 **Implementation Checklist**

### **Step 1: Database Setup**
```bash
# Run the cascade database schema
mysql luminari < lib/resource_cascade_db.sql
```

### **Step 2: Add Files to Build System**
Add to `src/Makefile.am`:
```makefile
resource_cascade.c resource_cascade.h \
resource_cascade_integration.c
```

### **Step 3: Core Integration Points**

**Replace existing harvest calls:**
```c
// OLD: 
attempt_wilderness_harvest(ch, resource_type);

// NEW:
attempt_wilderness_harvest_with_cascades(ch, resource_type);
```

**Replace existing survey command:**
```c
// In interpreter.c, replace survey command handler:
{ "survey"     , POS_STANDING, do_enhanced_survey    , 0, 0 },
```

### **Step 4: Key Function Integration**

**In resource_system.c, modify harvest completion:**
```c
// After successful harvest in attempt_wilderness_harvest():
if (added > 0) {
    apply_harvest_depletion_with_cascades(IN_ROOM(ch), resource_type, added);
    // ... existing code
}
```

---

## 🎮 **New Player Commands**

### **Enhanced Survey Options**
```
survey ecosystem         # Show ecosystem health and balance
survey cascade herbs     # Show what harvesting herbs will affect
survey relationships     # Display resource interaction matrix
survey impact           # Show your conservation impact
```

### **Example Output**
```
> survey ecosystem

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

## ⚙️ **Key Relationships Implemented**

### **Symbiotic (Help Each Other)**
- **Vegetation ↔ Herbs**: Share root systems and soil nutrients
- **Water → All Biological**: Water supports vegetation/herbs/game

### **Competitive (Hurt Each Other)**
- **Minerals ↔ Crystals**: Mining destroys crystal formations
- **Stone ↔ Minerals**: Quarrying disrupts ore veins

### **Ecological Cascades**
- **Wood → Everything**: Deforestation affects entire ecosystem
- **Game → Vegetation**: Hunting changes grazing pressure
- **Salt → Vegetation**: Salt extraction causes soil salinization

---

## 🔧 **Configuration Options**

### **Cascade Effect Strengths** (in database)
```sql
-- Adjust effect magnitudes
UPDATE resource_relationships 
SET effect_magnitude = -0.040  -- Reduce from -0.080
WHERE source_resource = 1 AND target_resource = 7;  -- Minerals affecting crystals
```

### **Ecosystem Health Thresholds** (in resource_cascade.h)
```c
#define ECOSYSTEM_PRISTINE_THRESHOLD    0.80  // All resources >80%
#define ECOSYSTEM_HEALTHY_THRESHOLD     0.60  // Most resources >60%
#define ECOSYSTEM_STRESSED_THRESHOLD    0.40  // Several resources <40%
```

### **Conservation Scoring** 
```c
#define CONSERVATION_PERFECT            1.0   // Sustainable harvesting
#define CONSERVATION_DESTRUCTIVE        0.2   // Ecosystem damage
```

---

## 🧪 **Testing Scenarios**

### **Test 1: Mining Impact**
1. Find area with high minerals AND crystals
2. Mine extensively (`mine minerals` repeatedly)
3. Check crystal levels (`survey detail crystal`)
4. Verify crystals decreased due to cascade

### **Test 2: Ecosystem Recovery**
1. Deplete multiple resources in an area
2. Stop harvesting completely  
3. Wait 2-3 game days
4. Check ecosystem health (`survey ecosystem`)
5. Verify regeneration and health improvement

### **Test 3: Conservation Scoring**
1. Harvest sustainably (1-2 units at a time)
2. Check conservation score (`survey impact`)
3. Over-harvest in different area (5+ units repeatedly)
4. Compare conservation scores

---

## 🎯 **Expected Gameplay Impact**

### **Strategic Decisions**
- **Specialization vs Balance**: Focus on one resource vs varied harvesting
- **Location Planning**: Understanding which areas to preserve vs exploit
- **Time Management**: Allowing areas to recover between harvests

### **Conservation Incentives**
- **Quality Bonuses**: Pristine ecosystems yield better materials
- **Sustainability Rewards**: Good conservationists get server-wide recognition
- **Ecosystem Collapse Penalties**: Degraded areas have poor yields

### **Social Dynamics**
- **Shared Impact**: Multiple players affect same areas
- **Conservation vs Profit**: Different player strategies compete
- **Restoration Cooperation**: Players work together to heal damaged areas

---

## 🚨 **Troubleshooting**

### **Performance Issues**
- Monitor cascade calculation times with `survey debug`
- Adjust `MAX_CASCADE_EFFECT` if calculations too slow
- Use database indexing for relationship queries

### **Balance Problems**
- If cascades too strong: Reduce effect magnitudes in database
- If too weak: Increase effect magnitudes or add new relationships
- Monitor ecosystem collapse rates server-wide

### **Player Confusion**
- Add help files explaining ecological relationships
- Use `survey cascade` to preview effects before harvesting
- Provide clear feedback about conservation scoring

---

**Phase 7 transforms individual resource gathering into complex ecosystem management, where every harvesting decision has ecological consequences that ripple through the entire resource web.**

---

*Implementation Guide Version: 1.0*  
*Last Updated: August 11, 2025*  
*Estimated Implementation Time: 1-2 weeks*
