# Wilderness Resource System - Quick Reference

**Version:** 1.0 | **Status:** [OK] Active | **Date:** August 8, 2025

---

## **Player Commands**

### **Basic Resource Discovery**
```
survey resources              # Show all resource percentages at current location
survey detail <resource>      # Detailed info about specific resource
survey terrain               # Environmental factors affecting resources
```

### **Visual Resource Mapping**
```
survey map <resource>         # Show ASCII minimap (default 10 radius)
survey map <resource> <radius> # Custom radius (5-20)
```

**Resource Types:** `vegetation`, `minerals`, `water`, `herbs`, `game`, `wood`, `stone`, `crystal`, `clay`, `salt`

**Example Usage:**
```
survey map vegetation        # Vegetation density map
survey map water 15         # Large water source map  
survey detail minerals      # Detailed mineral information
```

---

## **Map Symbols & Colors**

| Symbol | Density | Color | Meaning |
|--------|---------|-------|---------|
| `#` | 75%+ | Green | Very rich |
| `*` | 55-74% | Yellow | Rich |
| `+` | 35-54% | Light yellow | Moderate |
| `.` | 15-34% | Red | Poor |
| `,` | 3-14% | Dark | Trace |
| space | 0-2% | - | None |
| `@` | - | White | Your Position |

---

## **Resource by Terrain Type**

### **Forest Areas**
- **High:** Vegetation (60-80%), Wood (50-70%), Herbs (30-50%)
- **Medium:** Game (30-50%), Water (20-40%)
- **Low:** Minerals (10-20%), Stone (10-20%), Crystal (2-8%)

### **Mountain Areas**  
- **High:** Stone (50-70%), Minerals (40-60%), Crystal (5-15%)
- **Medium:** Water (20-40%), Vegetation (20-40%)
- **Low:** Herbs (10-20%), Wood (10-20%), Game (10-20%)

### **Plains/Fields**
- **High:** Vegetation (50-70%), Game (40-60%)  
- **Medium:** Herbs (30-50%), Water (30-50%)
- **Low:** Wood (10-30%), Minerals (10-20%), Stone (10-20%)

### **Desert Areas**
- **High:** Crystal (10-20%), Minerals (30-50%)
- **Medium:** Stone (20-30%)
- **Low:** Vegetation (5-15%), Water (5-15%), Wood (0-10%)

---

## **Admin Commands** *(Immortal Only)*

### **System Status**
```
resourceadmin status         # Overall system status and cache statistics
resourceadmin here          # Resources at current location
resourceadmin coords <x> <y> # Resources at specific coordinates
```

### **Debug and Analysis**
```
resourceadmin debug         # Comprehensive debug information
resourceadmin map <type> [radius] # Admin resource minimap
```

### **Cache Management**
```
resourceadmin cache         # Show cache statistics
resourceadmin cache cleanup # Remove expired cache entries
resourceadmin cache clear   # Clear all cache entries
resourceadmin cleanup       # Force cleanup of old resource nodes
```

---

## **Understanding Resource Values**

### **Percentage Ranges**
- **90-100%:** Incredibly abundant - rich resource deposits
- **70-89%:** Very abundant - excellent gathering opportunities  
- **50-69%:** Abundant - good resource availability
- **30-49%:** Moderate - average resource presence
- **10-29%:** Sparse - limited resources available
- **5-9%:** Very sparse - minimal resources present
- **0-4%:** Depleted/None - no significant resources

### **Factors Affecting Resources**
- **Terrain Type:** Different terrains favor different resources
- **Elevation:** Higher elevations affect water and mineral availability
- **Coordinates:** Resources vary naturally across the landscape
- **Environment:** Seasonal and weather effects (future enhancement)
- **Harvesting:** Player harvesting reduces availability (future enhancement)

---

## **Getting Started Guide**

### **Step 1: Enter Wilderness**
- Use `goto` or walk to any wilderness zone
- Verify with `whereis` - should show wilderness area

### **Step 2: Survey Resources**
- Start with `survey resources` for overview
- Try `survey terrain` to understand environment
- Use `survey map vegetation` for visual representation

### **Step 3: Explore and Compare**
- Move to different terrain types
- Compare resource availability
- Notice how resources change with location

### **Step 4: Advanced Features**
- Try different map radii: `survey map water 5`
- Check specific resources: `survey detail herbs`
- Experiment with all 10 resource types

---

## [?] **Troubleshooting**

### **"Resource maps can only be viewed in the wilderness"**
- **Solution:** Navigate to a wilderness zone using `goto` or walking

### **"Invalid resource type"**
- **Solution:** Use exact resource names (see list above), check spelling

### **All resources showing 0%**
- **Solution:** Contact admin - may indicate system issue

### **Map not displaying colors**
- **Solution:** Ensure your client supports ANSI colors

---

## **Tips for Best Results**

1. **Exploration:** Different areas have different resource patterns
2. **Terrain Matters:** Check terrain type for expected resources  
3. **Mapping:** Use larger radii (15-20) to find resource hotspots
4. **Detail Views:** Use `survey detail` for comprehensive resource info
5. **Movement:** Resources are consistent - revisiting areas shows same values

---

## **Support**

**Bug Reports:** Submit with "RESOURCE SYSTEM" tag  
**Questions:** Ask any immortal or development team member  
**Feature Requests:** Submit via normal suggestion channels  

**System Status:** [OK] Phases 1-3 Complete and Active  
**Last Updated:** August 8, 2025
