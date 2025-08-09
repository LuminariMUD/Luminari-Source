# Wilderness-Crafting Integration Testing Commands for `ornir`

**Character:** ornir  
**Date:** August 10, 2025  
**Testing:** Phase 4.5 Enhanced Wilderness-Crafting Integration  

---

## 🎯 **Quick Integration Test (5 minutes)**

### **Step 1: Verify Current Campaign**
```
whereis
```
**Expected:** Should show you're in default LuminariMUD campaign (not DL/FR)

### **Step 2: Check Enhanced Materials Display**
```
materials
```
**Expected Results:**
- **If Enhanced Integration Active:** Shows enhanced display with crafting integration info
- **If Basic System Only:** Shows simple material list without crafting details

### **Step 3: Enter Wilderness for Testing**
```
goto 12000
```
**(Replace 12000 with your wilderness zone number)**
**Expected:** You enter a wilderness area

### **Step 4: Test Material Harvesting**
```
gather herbs
mine metals
collect wood
```
**Expected Results:**
- Each command harvests materials based on location
- Messages show material type, quality (poor/common/uncommon/rare/legendary)
- Materials automatically added to storage

### **Step 5: Verify Enhanced Integration**
```
materials
```
**Expected Enhanced Results:**
- Materials show with enhanced descriptions
- Crafting applications mentioned for each material
- Quality levels show crafting values
- Enhanced material IDs (1000+ range) if debug info available

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
- **Enhanced System:** Shows crafting integration info
- **Basic System:** Shows simple material list

### **Enhanced Display Testing**

#### **Material List Commands:**
```
materials list
```
**Expected Enhanced:** Detailed list with crafting applications

```
materials list herbs
```
**Expected Enhanced:** Herb materials with enhanced descriptions

```
materials list metals
```
**Expected Enhanced:** Metal materials with crafting values

#### **Material Search/Detail:**
```
materials search iron
```
**Expected Enhanced:** Iron materials with quality-based crafting info

```
materials detail oak
```
**Expected Enhanced:** Oak wood details with crafting applications

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
materials list herbs
```
**Expected Enhanced Results:**
- Poor herbs: Lower crafting value
- Common herbs: Standard crafting value  
- Uncommon herbs: Enhanced crafting value
- Rare herbs: High crafting value
- Legendary herbs: Maximum crafting value

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
**Should Show:**
```
=== Enhanced Wilderness Materials Storage ===

Herb Materials:
- Common Silverleaf (ID: 1001) - Qty: 3
  Crafting Applications: Alchemy potions, healing items
  Crafting Value: 150 (quality bonus: +50%)

- Rare Bloodmoss (ID: 1003) - Qty: 1  
  Crafting Applications: High-grade alchemy, enchanting
  Crafting Value: 400 (quality bonus: +300%)

Metal Materials:
- Uncommon Iron Ore (ID: 1021) - Qty: 5
  Crafting Applications: Weapons, armor, tools
  Crafting Value: 200 (quality bonus: +100%)

Wood Materials:
- Legendary Oak Heartwood (ID: 1041) - Qty: 1
  Crafting Applications: Masterwork items, magical staves
  Crafting Value: 800 (quality bonus: +700%)

Total Materials: 10 types, 45 units
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
