# Wilderness Resource System Testing Guide

**Document Version:** 1.0  
**Date:** August 8, 2025  
**System Status:** ✅ Phases 1-3 Implemented  
**Testing Priority:** HIGH - Ready for Production Testing  

---

## 📋 **Quick Start Testing Checklist**

### **Prerequisites**
- [ ] MUD server compiled with latest resource system code
- [ ] Character with wilderness access (any level)
- [ ] Immortal character for admin commands (optional)

### **5-Minute Smoke Test**
1. [ ] Enter wilderness area
2. [ ] Run `survey resources` - verify 10 resource types display
3. [ ] Run `survey map vegetation` - verify colorful ASCII minimap appears
4. [ ] Move to different location and repeat - verify values change
5. [ ] Run `resourceadmin status` (immortal) - verify cache statistics

---

## 🎯 **Comprehensive Testing Scenarios**

### **Scenario 1: Basic Resource Discovery**

**Objective:** Verify players can discover and view resources in wilderness

**Steps:**
1. Navigate to wilderness zone using `goto` or walking
2. Confirm wilderness status: `whereis` should show wilderness zone
3. Test resource overview: `survey resources`
4. Test specific resource detail: `survey detail vegetation`
5. Test terrain analysis: `survey terrain`

**Expected Results:**
- All 10 resource types display with 0.0% to 100.0% values
- Resource percentages vary by location and terrain type
- Terrain command shows elevation, sector type, environmental factors
- No error messages or crashes

**Pass Criteria:** ✅ All commands work, reasonable resource values displayed

---

### **Scenario 2: Visual Resource Mapping**

**Objective:** Test ASCII minimap functionality and visual resource representation

**Steps:**
1. In wilderness, run `survey map vegetation`
2. Try different radii: `survey map water 5`, `survey map minerals 15`
3. Test all resource types: vegetation, minerals, water, herbs, game, wood, stone, crystal, clay, salt
4. Move to different terrain and repeat mapping

**Expected Results:**
- Colored ASCII maps with symbols representing resource density
- Maps centered on player position (@ symbol)
- Different colors for different resource levels (green=high, red=low)
- Map size changes with radius parameter
- Resources vary across different terrain types

**Visual Reference:**
```
Resource Map: vegetation (radius: 10)
========================================
..·.·.▪▪▫▫▪▪·..··.▪.·..
·.·.▪▪▫▫▫▫▫▫▪▪·.·.▪▪·.·
.·▪▪▫▫▫▫▫▫▫▫▫▫▪▪.▪▪▫·..
▪▪▫▫▫▫▫█████▫▫▫▫▪▪▫▫▪▪·
▪▫▫▫▫█████████▫▫▫▫▫▪▪·.
▫▫▫███████@███████▫▫▪▪·
▪▫▫▫█████████▫▫▫▫▫▪▪·..
▪▪▫▫▫▫▫█████▫▫▫▫▪▪▫▫▪▪·
.·▪▪▫▫▫▫▫▫▫▫▫▫▪▪.▪▪▫·..
```

**Pass Criteria:** ✅ Maps display correctly, colors vary, no crashes

---

### **Scenario 3: Performance and Caching**

**Objective:** Verify spatial caching improves performance and works correctly

**Steps:**
1. First visit to location: `resourceadmin debug`
   - Note cache status should show "MISS"
2. Immediate second visit: `resourceadmin debug`
   - Note cache status should show "HIT"
3. Check cache statistics: `resourceadmin cache`
4. Move around nearby area (within 10 coordinates) and return
5. Run `resourceadmin debug` again - should still show cache HIT
6. Test cache management: `resourceadmin cache clear`
7. Verify next access shows cache MISS again

**Expected Results:**
- First access shows cache MISS, subsequent accesses show cache HIT
- Cache statistics increase with more locations visited
- Resource values identical between cached accesses
- Cache clear command resets cache to 0 nodes

**Performance Indicators:**
- Cache hit ratio should improve with repeated visits
- Total cached nodes should increase as you explore
- Expired nodes should show 0 unless cache lifetime exceeded

**Pass Criteria:** ✅ Caching works, performance improves on subsequent visits

---

### **Scenario 4: Multi-Location Consistency**

**Objective:** Ensure resource values are consistent and realistic across different locations

**Testing Matrix:**

| Terrain Type | Expected High Resources | Expected Low Resources |
|--------------|------------------------|----------------------|
| Forest       | vegetation, wood, herbs | minerals, stone, crystal |
| Mountains    | stone, minerals, crystal | vegetation, wood, herbs |
| Plains       | vegetation, game        | wood, crystal, stone |
| Desert       | crystal, minerals       | vegetation, water, wood |
| Swamp        | water, herbs, clay      | stone, crystal, game |

**Steps:**
1. Visit each terrain type in wilderness
2. Record resource values: `survey resources`
3. Verify values match expected patterns
4. Test several locations of same terrain type
5. Confirm resource values vary but stay within expected ranges

**Pass Criteria:** ✅ Resource patterns match terrain types, values are realistic

---

### **Scenario 5: Admin Command Functionality**

**Objective:** Test all administrative commands for resource management

**Admin Command Test Matrix:**

| Command | Purpose | Expected Output |
|---------|---------|----------------|
| `resourceadmin status` | System overview | Resource counts, cache stats, system info |
| `resourceadmin here` | Current location resources | All 10 resource percentages |
| `resourceadmin coords -100 50` | Specific coordinate resources | Resources at exact coordinates |
| `resourceadmin map vegetation 10` | Admin minimap | Same as survey map |
| `resourceadmin debug` | Comprehensive debugging | Cache stats, calculations, raw values |
| `resourceadmin cache` | Cache management | Cache statistics and commands |
| `resourceadmin cache cleanup` | Remove expired cache | "Removed X expired entries" |
| `resourceadmin cache clear` | Clear all cache | "Cleared X entries" |
| `resourceadmin cleanup` | Force node cleanup | "Cleanup complete" |

**Pass Criteria:** ✅ All commands work without errors, provide expected information

---

## 🐛 **Known Issues and Troubleshooting**

### **Common Issues**

1. **"Resource maps can only be viewed in the wilderness"**
   - **Cause:** Not in a wilderness zone
   - **Solution:** Use `goto` to navigate to wilderness area, check with `whereis`

2. **"Invalid resource type" for survey commands**
   - **Cause:** Typo in resource name or unsupported resource
   - **Solution:** Use exact names: vegetation, minerals, water, herbs, game, wood, stone, crystal, clay, salt

3. **All resource values showing 0.000**
   - **Cause:** Perlin noise system not initialized or LIMIT macro issues
   - **Solution:** Check server logs, restart MUD, verify resource system initialization

4. **Cache not showing HIT status**
   - **Cause:** Coordinates not aligned to cache grid or cache expired
   - **Solution:** Cache uses 10x10 grid, wait for cache expiration, check cache settings

### **Debug Information**

Use `resourceadmin debug` to see detailed information:
- Raw Perlin noise values (should be -1.0 to 1.0 range)
- Normalized resource values (should be 0.0 to 1.0 range)
- Cache grid coordinates and hit/miss status
- Environmental modifiers and calculations

---

## 📊 **Performance Benchmarks**

### **Expected Performance**

| Metric | Target | Measurement Method |
|--------|--------|--------------------|
| Cache Hit Ratio | >80% after exploration | `resourceadmin cache` statistics |
| Resource Calculation Time | <1ms per location | Server performance monitoring |
| Memory Usage | <1MB for cache | System monitoring |
| Cache Expiration | 5 minutes | Test with wait time |

### **Load Testing**

1. **Multiple Player Test:**
   - Have 5+ players explore different wilderness areas
   - Monitor cache statistics and performance
   - Verify no conflicts or data corruption

2. **Rapid Movement Test:**
   - Move quickly through wilderness coordinates
   - Verify cache builds appropriately
   - Check for memory leaks or performance degradation

---

## ✅ **Test Result Documentation**

### **Test Session Template**

```
Date: ___________
Tester: ___________
MUD Version: ___________
Test Duration: ___________

Basic Functionality:
[ ] Resource survey commands work
[ ] Visual maps display correctly  
[ ] Admin commands accessible
[ ] No crashes or errors

Performance:
[ ] Cache hit ratio >80% after exploration
[ ] Resource values consistent on repeated access
[ ] Memory usage acceptable

Issues Found:
- Issue 1: ________________
- Issue 2: ________________

Overall Status: [ ] PASS [ ] FAIL [ ] NEEDS WORK

Notes: ____________________
```

---

## 🚀 **Next Phase Testing Preparation**

### **Phase 4: Region Integration (Planned)**
- Test biome-specific resource modifiers
- Verify region boundaries affect resources
- Check climate and season impacts

### **Phase 5: Harvesting Mechanics (Planned)**
- Test actual resource collection
- Verify depletion and regeneration
- Check skill integration and tools

---

**Testing Contact:** Development Team  
**Bug Reports:** Submit via normal channels with "RESOURCE SYSTEM" tag  
**Performance Issues:** Include `resourceadmin debug` output in reports
