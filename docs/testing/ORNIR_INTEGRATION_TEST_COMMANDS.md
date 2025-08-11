# Wilderness-Crafting Integration Testing Commands for `ornir`

**Character:** ornir  
**Date:** August 10, 2025  
**Testing:** Phase 5 Wilderness Harvesting Commands + Phase 4.5 Enhanced Integration  
**Status:** ✅ **READY FOR TESTING** - Harvesting commands implemented!

---

## 🎉 **Phase 5 Implementation Complete!**

**Status Update:**
- ✅ **Phase 4.5**: Enhanced integration system implemented and ready
- ✅ **Phase 5**: Material harvesting commands **NOW IMPLEMENTED**

**New Commands Available:**
- `harvest [resource_type]` - Primary harvesting command
- `gather [resource_type]` - Specialized for herbs, vegetation, game  
- `mine [resource_type]` - Specialized for minerals, crystal, stone

---

## 🚀 **Quick 5-Minute Test (Now Functional!)**

### **Step 1: Enter Wilderness Area**
```
goto <wilderness_zone_number>
```
**Expected:** You enter a wilderness area

### **Step 2: Survey Available Resources**
```
survey resources
```
**Expected:** Shows 10 resource types with availability percentages

### **Step 3: Test New Harvesting Commands**
```
harvest
```
**Expected:** Shows harvestable resources and usage instructions

```
harvest herbs
```
**Expected:** Attempts to harvest herb materials, success based on availability

```
gather herbs
```
**Expected:** Specialized herb gathering (same as harvest herbs)

```
mine minerals
```
**Expected:** Attempts to mine mineral resources

### **Step 4: Check Enhanced Integration**
```
materials
```
**Expected Basic Results:**
- Simple list of materials with quantities
- Script-friendly format
- Works in all campaigns

```
materials details
```
**Expected Enhanced Results (LuminariMUD only):**
- Materials show with enhanced descriptions
- Crafting applications mentioned for each material
- Quality levels (poor → legendary) show different crafting values
- Enhanced material IDs (1000+ range)
- **Proper color formatting** (white headers, yellow categories, normal text)

---

## 🔬 **Detailed Testing Commands**

### **Resource Surveying (Pre-Harvest)**

#### **Check Resource Availability:**
```
survey resources
```
**Expected:** List of 10 resource types with abundance levels

```
survey map vegetation 10
```
**Expected:** ASCII minimap showing vegetation distribution

```
survey detail herbs
```
**Expected:** Detailed herb resource analysis for current location

### **Harvesting Commands Testing**

#### **Primary Harvest Command:**
```
harvest
```
**Expected:** 
```
Harvestable resources at this location:
=====================================
  Vegetation  : abundant (harvest vegetation)
  Herbs       : moderate (harvest herbs)
  Wood        : scarce (harvest wood)
  ...

Usage: harvest <resource_type>
Specialized commands: gather <type>, mine <type>
```

#### **Test Each Resource Type:**
```
harvest herbs
harvest vegetation  
harvest minerals
harvest wood
harvest stone
harvest crystal
harvest game
harvest water
harvest clay
harvest salt
```
**Expected Results:**
- **Success:** "You successfully harvest X units of [quality] [material_name]."
- **Failure:** "You fail to harvest any usable [resource_type]."
- **Insufficient:** "There are insufficient [resource_type] resources here to harvest."

**Note:** All 10 resource types now fully supported (fixed water/clay/salt "storage full" bug)

#### **Specialized Commands:**
```
gather herbs
gather vegetation
gather game
```
**Expected:** Works for herbs, vegetation, game only

```
mine minerals
mine crystal  
mine stone
mine salt
```
**Expected:** Works for minerals, crystal, stone, salt only

```
mine herbs
```
**Expected:** "You can only mine: minerals, crystal, stone, or salt."

### **Integration Verification**

#### **After Each Harvest:**
```
materials
```
**Expected Basic Display:**
```
Your Wilderness Material Storage:
=====================================
  5 x legendary spring water
  4 x rare common clay
 13 x legendary sea salt
  4 x legendary starlily
  6 x legendary light bloom
  6 x legendary dragonhide
  6 x legendary star steel
  6 x legendary marble
Storage: 8/100 slots used
```

```
materials details
```
**Expected Enhanced Display (LuminariMUD only):**
```
=== Enhanced Wilderness Materials (LuminariMUD) ===
Your materials are preserved with their full hierarchy and quality.
These materials can be used in enhanced LuminariMUD crafting recipes.

Water Materials:
- Legendary Spring Water (ID: 1200) - Qty: 5
  Crafting Applications: Alchemy, food preparation
  Crafting Value: 800 (quality bonus: legendary quality (+500%))

Clay Materials:
- Rare Common Clay (ID: 1300) - Qty: 4
  Crafting Applications: Pottery, containers, building
  Crafting Value: 300 (quality bonus: rare quality (+200%))

Total Materials: 50 units
Enhanced Integration: ACTIVE
```

---

## 🧪 **Advanced Testing Scenarios**

### **Skill and Success Testing**

#### **Test Success Rates:**
```
# Try harvesting same resource multiple times
harvest herbs
harvest herbs  
harvest herbs
```
**Expected:** Success rate varies, skill affects outcomes

#### **Test Different Locations:**
```
# Move to different wilderness coordinates
survey resources  # Note differences
harvest herbs     # Compare success/materials
```

### **Quality and Material Variation**

#### **Test Quality Distribution:**
```
# Harvest same resource type multiple times
harvest herbs
harvest herbs
harvest herbs
materials list herbs
```
**Expected:** Different quality levels (poor, common, uncommon, rare, legendary)

#### **Test Material Subtypes:**
```
# Multiple harvests should yield different herb subtypes
harvest herbs (multiple times)
materials
```
**Expected:** Different herb types: marjoram, kingfoil, starlily, etc.

### **Error Condition Testing**

#### **Test Invalid Commands:**
```
harvest
harvest invalidtype
harvest
```
**Expected:** Appropriate error messages

#### **Test Outside Wilderness:**
```
# Go to non-wilderness room
goto <city_room>
harvest herbs
```
**Expected:** "You can only harvest materials in the wilderness."

#### **Test Resource Exhaustion:**
```
# Try harvesting in area with no resources
goto <barren_wilderness>
survey resources  # Should show low/no resources
harvest herbs
```
**Expected:** "There are insufficient herbs resources here to harvest."

---

## 📊 **Expected Success Indicators**

### ✅ **Harvesting Working Correctly:**
- `harvest` command available and functional
- `gather` and `mine` commands work with appropriate restrictions
- Success/failure messages appropriate to resource availability
- Materials automatically added to storage
- Enhanced integration triggers for each harvest

### ✅ **Integration Working Correctly:**
- Harvested materials appear in enhanced materials display
- Materials show enhanced descriptions with crafting info
- Quality levels affect crafting values appropriately
- Enhanced material IDs in 1000+ range
- Campaign safety maintained (basic display in DL/FR if available)

### ✅ **System Stability:**
- No crashes during harvesting
- Commands respond appropriately to invalid input
- Wilderness location checking works correctly
- Material storage limits respected

---

## 🐛 **Potential Issues to Watch For**

### **If Commands Don't Work:**
1. **Command not found:** Verify you're using LuminariMUD campaign (not DL/FR)
2. **Wrong location:** Ensure you're in a wilderness zone
3. **No resources:** Use `survey resources` to verify availability

### **If Integration Doesn't Show:**
1. **Basic display only:** Check campaign setting
2. **No enhanced features:** Verify Phase 4.5 integration compiled correctly
3. **Empty storage:** Ensure harvesting is actually adding materials

---

## 🎯 **Testing Summary**

**Phase 5 Implementation Status:**
- ✅ Basic harvesting commands (`harvest`, `gather`, `mine`)
- ✅ Resource availability checking
- ✅ Skill-based success calculation  
- ✅ Quality and subtype determination
- ✅ Automatic storage integration
- ✅ Enhanced crafting integration triggers
- ✅ Campaign-safe implementation
- ⏳ Skill improvement system (commented out for now)
- ⏳ Resource depletion/regeneration (future enhancement)

**Ready for full testing with `ornir`!** 🚀

---

## 🔬 **Detailed Testing Commands**

### **Material Harvesting Tests**

#### **Test All Resource Categories:**
```
survey resources
```
**Expected:** Shows 10 resource types with percentages for current location

```
gather herbs
gather herbs
gather herbs
```
**Expected:** Multiple herb harvests with varying quality levels

```
mine metals
mine stone
mine gems
```
**Expected:** Different metal/stone materials harvested

```
collect wood
hunt animals
hunt game
```
**Expected:** Wood and animal materials collected

```
forage food
collect vegetation
```
**Expected:** Food and vegetation materials gathered

#### **Check Storage After Each Harvest:**
```
materials
```
**Expected After Each Harvest:**
- Material count increases
- **Basic System:** Shows simple material list (all campaigns)
- **Enhanced System:** Use `materials details` for crafting integration info

```
materials details
```
**Expected Enhanced Results (LuminariMUD only):**
- Detailed crafting applications for each material
- Quality-based crafting values
- Enhanced material IDs

### **Enhanced Display Testing**

#### **Material List Commands:**
```
materials
```
**Expected:** Basic list format (script-friendly)

```
materials details
```
**Expected Enhanced (LuminariMUD only):** Detailed list with crafting applications

```
materials list herbs
```
**Note:** `list`, `search`, `detail` subcommands not yet implemented

#### **Material Search/Detail:**
```
materials search iron
```
**Note:** Search functionality not yet implemented

```
materials detail oak
```
**Note:** Individual material detail not yet implemented

### **Quality System Testing**

#### **Harvest Same Material Multiple Times:**
```
gather herbs
gather herbs
gather herbs
gather herbs
gather herbs
```
**Expected:** Different quality herbs (poor → legendary) with different crafting values

#### **Compare Quality Levels:**
```
materials details
```
**Expected Enhanced Results:**
- Poor materials: Lower crafting value (poor quality -25%)
- Common materials: Standard crafting value (common quality +0%)  
- Uncommon materials: Enhanced crafting value (uncommon quality +50%)
- Rare materials: High crafting value (rare quality +200%)
- Legendary materials: Maximum crafting value (legendary quality +500%)

---

## 🛡️ **Campaign Safety Testing**

### **Test Campaign Switching (If Available):**

#### **Switch to DragonLance (if available):**
```
campaign dl
materials
```
**Expected:** Basic materials display only (no enhanced features)

#### **Switch to Forgotten Realms (if available):**
```
campaign fr
materials
```
**Expected:** Basic materials display only (no enhanced features)

#### **Return to LuminariMUD:**
```
campaign luminari
materials
```
**Expected:** Enhanced materials display returns

---

## 📊 **Expected Results Summary**

### **Enhanced Integration Active (LuminariMUD Campaign):**
```
materials
```
**Should Show (Basic Format):**
```
Your Wilderness Material Storage:
=====================================
  3 x common silverleaf
  1 x rare bloodmoss
  5 x uncommon iron ore
  1 x legendary oak heartwood
Storage: 4/100 slots used
```

```
materials details
```
**Should Show (Enhanced Format):**
```
=== Enhanced Wilderness Materials (LuminariMUD) ===
Your materials are preserved with their full hierarchy and quality.
These materials can be used in enhanced LuminariMUD crafting recipes.

Herb Materials:
- Common Silverleaf (ID: 1001) - Qty: 3
  Crafting Applications: Alchemy potions, healing items
  Crafting Value: 150 (quality bonus: common quality (+0%))

- Rare Bloodmoss (ID: 1003) - Qty: 1  
  Crafting Applications: Alchemy potions, healing items
  Crafting Value: 400 (quality bonus: rare quality (+200%))

Mineral Materials:
- Uncommon Iron Ore (ID: 1021) - Qty: 5
  Crafting Applications: Weapons, armor, tools
  Crafting Value: 200 (quality bonus: uncommon quality (+50%))

Wood Materials:
- Legendary Oak Heartwood (ID: 1041) - Qty: 1
  Crafting Applications: Crafted items, magical staves
  Crafting Value: 800 (quality bonus: legendary quality (+500%))

Total Materials: 10 units
Enhanced Integration: ACTIVE
```

### **Basic System Only (DL/FR Campaigns):**
```
materials
```
**Should Show:**
```
=== Wilderness Materials Storage ===

You have the following materials stored:
- Common Silverleaf: 3 units
- Rare Bloodmoss: 1 unit
- Uncommon Iron Ore: 5 units
- Legendary Oak Heartwood: 1 unit

Total: 4 material types, 10 units
```

---

## 🐛 **Troubleshooting**

### **If Enhanced Features Don't Appear:**

1. **Check Compilation:**
   ```
   whereis
   ```
   Verify you're in default campaign, not DL/FR

2. **Test Basic Harvesting:**
   ```
   gather herbs
   materials
   ```
   If basic harvesting works but no enhanced display, integration may not be compiled

3. **Try Different Material Types:**
   ```
   mine iron
   collect oak
   hunt deer
   materials
   ```
   Test multiple categories to verify integration

### **If Nothing Works:**

1. **Verify Wilderness Location:**
   ```
   survey resources
   ```
   Should show resource percentages (confirms wilderness)

2. **Check Character Permissions:**
   ```
   score
   ```
   Verify character can access wilderness features

---

## ✅ **Success Criteria**

**Integration Working Correctly If:**
- ✅ Materials harvest successfully in wilderness
- ✅ Enhanced materials display shows crafting integration (LuminariMUD)
- ✅ Basic materials display only in DL/FR campaigns
- ✅ Quality levels affect crafting values appropriately
- ✅ Material hierarchy preserved (category → subtype → quality)
- ✅ No crashes or errors during testing

**Ready for Next Phase If:**
- ✅ All above criteria met
- ✅ Enhanced material IDs working (1000+ range)
- ✅ Integration functions trigger automatically
- ✅ Campaign safety verified

---

**Test Duration:** 10-15 minutes for complete testing  
**Focus:** Verify enhanced integration works for ornir in LuminariMUD campaign while maintaining safety for other campaigns
