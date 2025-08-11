# Phase 5: Material Harvesting Commands Implementation Plan

**Date:** August 10, 2025  
**Status:** Ready for Implementation  
**Dependencies:** Phase 4.5 Enhanced Integration (✅ Complete)

---

## 📋 **Implementation Overview**

Phase 5 adds the missing **player harvesting commands** that allow players to actively collect wilderness materials, triggering the enhanced integration system we implemented in Phase 4.5.

### **What We Have (Phase 4.5):**
- ✅ Enhanced integration system
- ✅ Material storage functions
- ✅ Quality-based crafting values
- ✅ Campaign-safe implementation
- ✅ Integration hooks ready in `add_material_to_storage()`

### **What We Need (Phase 5):**
- ❌ Harvesting commands (`harvest`, `gather`, `mine`, `collect`)
- ❌ Skill-based success system
- ❌ Resource depletion mechanics
- ❌ Tool requirements integration

---

## 🛠️ **Implementation Plan**

### **Step 1: Add Harvesting Commands to Interpreter**

**File: `src/interpreter.c`**

Add new commands to command table:
```c
// Around line 500 (near other wilderness commands)
#if !defined(CAMPAIGN_FR) && !defined(CAMPAIGN_DL)
    {"gather", "gather", POS_STANDING, do_wilderness_gather, 0, 0, FALSE, ACTION_STANDARD, {6, 0}, NULL},
    {"mine", "mine", POS_STANDING, do_wilderness_mine, 0, 0, FALSE, ACTION_STANDARD, {6, 0}, NULL},
    {"extract", "extract", POS_STANDING, do_wilderness_extract, 0, 0, FALSE, ACTION_STANDARD, {6, 0}, NULL},
    {"quarry", "quarry", POS_STANDING, do_wilderness_quarry, 0, 0, FALSE, ACTION_STANDARD, {6, 0}, NULL},
#endif

// Update existing harvest command for wilderness use
#if defined(CAMPAIGN_DL)
    {"harvest", "harvest", POS_STANDING, do_newcraft, 0, SCMD_NEWCRAFT_HARVEST, TRUE, ACTION_STANDARD, {0, 0}, NULL},
#else
    {"harvest", "harvest", POS_STANDING, do_wilderness_harvest, 0, 0, FALSE, ACTION_STANDARD, {6, 0}, NULL},
#endif
```

### **Step 2: Implement Harvesting Functions**

**File: `src/resource_system.c`**

Add primary harvesting function:
```c
/* Primary wilderness harvesting command */
ACMD(do_wilderness_harvest) {
    char arg[MAX_INPUT_LENGTH];
    int resource_type = -1;
    
    one_argument(argument, arg, sizeof(arg));
    
    // Validate wilderness location
    if (!ZONE_FLAGGED(world[IN_ROOM(ch)].zone, ZONE_WILDERNESS)) {
        send_to_char(ch, "You can only harvest materials in the wilderness.\r\n");
        return;
    }
    
    // Show available resources if no argument
    if (!*arg) {
        show_harvestable_resources(ch);
        return;
    }
    
    // Parse resource type
    resource_type = parse_resource_type(arg);
    if (resource_type < 0) {
        send_to_char(ch, "Invalid resource type. Use 'harvest' to see available options.\r\n");
        return;
    }
    
    // Attempt harvest
    attempt_wilderness_harvest(ch, resource_type);
}

/* Specialized gathering commands */
ACMD(do_wilderness_gather) {
    // Focuses on herbs, vegetation, game
    perform_specialized_harvest(ch, argument, HARVEST_TYPE_GATHER);
}

ACMD(do_wilderness_mine) {
    // Focuses on minerals, crystals, metals
    perform_specialized_harvest(ch, argument, HARVEST_TYPE_MINE);
}

ACMD(do_wilderness_extract) {
    // Focuses on stone, clay, salt
    perform_specialized_harvest(ch, argument, HARVEST_TYPE_EXTRACT);
}

ACMD(do_wilderness_quarry) {
    // Focuses on wood, large stone
    perform_specialized_harvest(ch, argument, HARVEST_TYPE_QUARRY);
}
```

### **Step 3: Core Harvesting Logic**

**Function: `attempt_wilderness_harvest()`**
```c
int attempt_wilderness_harvest(struct char_data *ch, int resource_type) {
    int x, y, skill_level, success_roll, base_yield;
    int category, subtype, quality, quantity;
    float resource_level;
    
    // Get location coordinates
    x = world[IN_ROOM(ch)].coords[0];
    y = world[IN_ROOM(ch)].coords[1];
    
    // Check resource availability
    resource_level = calculate_current_resource_level(resource_type, x, y);
    if (resource_level < 0.1) {
        send_to_char(ch, "There are insufficient %s resources here to harvest.\r\n", 
                     resource_names[resource_type]);
        return 0;
    }
    
    // Get relevant skill
    skill_level = get_harvest_skill_level(ch, resource_type);
    
    // Calculate success
    success_roll = dice(1, 100) + skill_level;
    int difficulty = get_harvest_difficulty(resource_type, resource_level);
    
    if (success_roll < difficulty) {
        send_to_char(ch, "You fail to harvest any usable %s.\r\n", 
                     resource_names[resource_type]);
        improve_skill(ch, get_harvest_skill(resource_type));
        return 0;
    }
    
    // Determine what was harvested
    category = resource_type;
    subtype = determine_harvested_material_subtype(resource_type, x, y, resource_level);
    quality = calculate_harvest_quality(ch, resource_type, success_roll, skill_level);
    quantity = calculate_harvest_quantity(ch, resource_type, success_roll, skill_level);
    
    // Add to storage (triggers Phase 4.5 integration)
    int added = add_material_to_storage(ch, category, subtype, quality, quantity);
    
    if (added > 0) {
        const char *material_name = get_full_material_name(category, subtype, quality);
        send_to_char(ch, "You successfully harvest %d units of %s.\r\n", 
                     added, material_name);
        
        // Skill improvement
        improve_skill(ch, get_harvest_skill(resource_type));
        
        // Resource depletion (Phase 5 future enhancement)
        // deplete_local_resource(x, y, resource_type, added);
    } else {
        send_to_char(ch, "Your material storage is full.\r\n");
    }
    
    return added;
}
```

### **Step 4: Support Functions**

**Helper functions needed:**
```c
int get_harvest_skill_level(struct char_data *ch, int resource_type);
int get_harvest_skill(int resource_type);
int get_harvest_difficulty(int resource_type, float resource_level);
int calculate_harvest_quality(struct char_data *ch, int resource_type, int success_roll, int skill_level);
int calculate_harvest_quantity(struct char_data *ch, int resource_type, int success_roll, int skill_level);
void show_harvestable_resources(struct char_data *ch);
int parse_resource_type(const char *arg);
```

---

## 🎯 **Implementation Priority**

### **Phase 5.1: Basic Harvesting (High Priority)**
1. ✅ Add `harvest` command to interpreter
2. ✅ Implement basic `do_wilderness_harvest()` function
3. ✅ Add resource type parsing
4. ✅ Connect to existing resource system
5. ✅ Trigger Phase 4.5 integration via `add_material_to_storage()`

### **Phase 5.2: Skill Integration (Medium Priority)**
1. ⏳ Map resource types to existing skills
2. ⏳ Implement skill-based success rates
3. ⏳ Add skill improvement on harvest attempts
4. ⏳ Implement difficulty scaling

### **Phase 5.3: Enhanced Commands (Low Priority)**
1. ⏳ Add specialized commands (`gather`, `mine`, `extract`, `quarry`)
2. ⏳ Add tool requirement checks
3. ⏳ Implement advanced success modifiers
4. ⏳ Add harvest efficiency bonuses

---

## 📝 **Testing Plan**

### **Phase 5.1 Testing:**
```bash
# Enter wilderness
goto <wilderness_zone>

# Check available resources
survey resources

# Test basic harvesting
harvest
harvest herbs
harvest minerals
harvest wood

# Verify integration
materials  # Should show enhanced display with harvested materials
```

### **Expected Results:**
- `harvest` command available in wilderness only
- Resource availability checked before harvest
- Materials added to storage trigger Phase 4.5 integration
- Enhanced materials display shows harvested items

---

## 🚀 **Implementation Benefits**

### **Immediate Benefits:**
- ✅ Players can finally harvest wilderness materials
- ✅ Phase 4.5 enhanced integration becomes functional
- ✅ Complete wilderness-to-crafting pipeline
- ✅ Campaign-safe implementation ready

### **Future Enhancements:**
- 🔄 Resource depletion and regeneration
- 🔄 Tool and equipment requirements
- 🔄 Advanced skill-based mechanics
- 🔄 Seasonal and weather effects

---

**Ready to implement Phase 5.1 Basic Harvesting!** The integration infrastructure from Phase 4.5 is complete and waiting for the harvesting commands.
