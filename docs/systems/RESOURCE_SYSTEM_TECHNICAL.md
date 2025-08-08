# Wilderness Resource System - Technical Documentation

**Document Version:** 1.0  
**Date:** August 8, 2025  
**Implementation Status:** Phases 1-3 Complete  
**Target Audience:** Developers, System Administrators  

---

## 🏗️ **System Architecture**

### **Core Components**

```
Resource System Architecture
┌─────────────────────────────────────────────────┐
│                 User Interface                  │
├─────────────────────────────────────────────────┤
│  survey commands     │   resourceadmin commands │
│  (act.informative.c) │   (act.wizard.c)        │
├─────────────────────────────────────────────────┤
│              Resource Calculation Engine         │
│              (resource_system.c)                │
├─────────────────────────────────────────────────┤
│   Spatial Cache    │    Perlin Noise    │  KD-Tree │
│   (Phase 3)        │    (wilderness.c)  │  (kdtree.c)│
├─────────────────────────────────────────────────┤
│                  Data Storage                   │
│    Memory Cache    │    File System     │  MySQL   │
└─────────────────────────────────────────────────┘
```

### **File Structure**

```
src/
├── resource_system.h        # Main header with structures and declarations
├── resource_system.c        # Core implementation with caching
├── act.informative.c        # Enhanced survey commands
├── act.wizard.c            # resourceadmin command implementation
├── interpreter.c           # Command registration
├── wilderness.h            # Perlin noise layer definitions
├── wilderness.c            # Existing Perlin noise functions
└── kdtree.c               # KD-tree utilities for spatial indexing

docs/
├── project-management/
│   └── WILDERNESS-RESOURCE-PLAN.md    # Implementation plan
└── testing/
    └── RESOURCE_SYSTEM_TESTING.md     # Testing guide
```

---

## 💾 **Data Structures**

### **Resource Configuration**

```c
struct resource_config {
    int noise_layer;           /* Perlin noise layer (4-11) */
    float base_multiplier;     /* Global scaling (0.0-2.0) */
    float regen_rate_per_hour; /* Regeneration (0.0-1.0) */
    float depletion_threshold; /* Max harvest (0.0-1.0) */
    int quality_variance;      /* Quality variation (0-100) */
    bool seasonal_affected;    /* Season impact flag */
    bool weather_affected;     /* Weather impact flag */
    int harvest_skill;         /* Associated skill */
    const char *name;          /* Display name */
    const char *description;   /* Description text */
};
```

### **Spatial Cache Node**

```c
struct resource_cache_node {
    int x, y;                                      /* Coordinates */
    float cached_values[NUM_RESOURCE_TYPES];       /* Resource levels */
    time_t cache_time;                             /* Cache timestamp */
    struct resource_cache_node *left, *right;     /* KD-tree structure */
};
```

### **Resource Node (Harvest History)**

```c
struct resource_node {
    int x, y;                                      /* Coordinates */
    float consumed_amount[NUM_RESOURCE_TYPES];     /* Harvested amount */
    time_t last_harvest[NUM_RESOURCE_TYPES];       /* Last harvest time */
    int harvest_count[NUM_RESOURCE_TYPES];         /* Harvest frequency */
    struct resource_node *left, *right;           /* KD-tree structure */
};
```

---

## ⚙️ **Core Functions**

### **Primary API Functions**

```c
/* Main calculation function with caching */
float calculate_current_resource_level(int resource_type, int x, int y);

/* Base resource calculation from Perlin noise */
float get_base_resource_value(int resource_type, int x, int y);

/* Environmental modifiers */
float apply_region_resource_modifiers(int resource_type, int x, int y, float base);
float apply_environmental_modifiers(int resource_type, int x, int y, float base);

/* Harvest integration */
float apply_harvest_regeneration(int resource_type, float base, struct resource_node *node);
```

### **Cache Management Functions**

```c
/* Cache lookup and storage */
struct resource_cache_node *cache_find_resource_values(int x, int y);
void cache_store_resource_values(int x, int y, float values[NUM_RESOURCE_TYPES]);

/* Cache maintenance */
void cache_cleanup_expired(void);
void cache_clear_all(void);
int cache_get_stats(int *total_nodes, int *expired_nodes);
```

### **Display and Utility Functions**

```c
/* Resource visualization */
char get_resource_map_symbol(float level);
const char *get_resource_color(float level);
const char *get_abundance_description(float level);

/* Survey command implementations */
void show_resource_survey(struct char_data *ch);
void show_resource_map(struct char_data *ch, int resource_type, int radius);
void show_debug_survey(struct char_data *ch);
```

---

## 🔧 **Configuration Parameters**

### **Cache Settings**

```c
#define RESOURCE_CACHE_LIFETIME 300        /* Cache lifetime: 5 minutes */
#define RESOURCE_CACHE_MAX_NODES 1000      /* Maximum cached nodes */
#define RESOURCE_CACHE_GRID_SIZE 10        /* Cache grid: every 10 coords */
```

### **Resource Types and Noise Layers**

```c
#define NUM_RESOURCE_TYPES 10

/* Perlin noise layer assignments */
#define NOISE_VEGETATION 4      /* Layer 4: Vegetation */
#define NOISE_MINERALS 5        /* Layer 5: Minerals */
#define NOISE_WATER_RESOURCE 6  /* Layer 6: Water */
#define NOISE_HERBS 7           /* Layer 7: Herbs */
#define NOISE_GAME 8            /* Layer 8: Game animals */
#define NOISE_WOOD 9            /* Layer 9: Wood/timber */
#define NOISE_STONE 10          /* Layer 10: Stone */
#define NOISE_CRYSTAL 11        /* Layer 11: Crystal */
/* Minerals layer (5) shared by clay */
/* Water layer (6) shared by salt */
```

### **Resource Configuration Array**

```c
struct resource_config resource_configs[NUM_RESOURCE_TYPES] = {
    /* {layer, mult, regen, threshold, quality, seasonal, weather, skill, name, description} */
    {NOISE_VEGETATION, 1.0, 0.2, 0.8, 20, true, true, SKILL_FORESTING, "vegetation", "General plant life"},
    {NOISE_MINERALS, 0.3, 0.01, 0.9, 30, false, false, SKILL_MINING, "minerals", "Ores and metals"},
    {NOISE_WATER_RESOURCE, 1.2, 0.5, 0.6, 10, false, true, SKILL_FORESTING, "water", "Fresh water"},
    {NOISE_HERBS, 0.4, 0.1, 0.7, 40, true, true, SKILL_FORESTING, "herbs", "Medicinal plants"},
    {NOISE_GAME, 0.6, 0.15, 0.5, 25, true, false, SKILL_HUNTING, "game", "Huntable animals"},
    {NOISE_WOOD, 0.8, 0.05, 0.9, 15, true, false, SKILL_FORESTING, "wood", "Harvestable timber"},
    {NOISE_STONE, 0.5, 0.005, 0.95, 5, false, false, SKILL_MINING, "stone", "Building materials"},
    {NOISE_CRYSTAL, 0.1, 0.001, 0.99, 50, false, false, SKILL_MINING, "crystal", "Magical crystals"},
    {NOISE_MINERALS, 0.3, 0.02, 0.8, 15, false, true, SKILL_MINING, "clay", "Clay deposits"},
    {NOISE_WATER_RESOURCE, 0.2, 0.03, 0.7, 20, false, true, SKILL_MINING, "salt", "Salt deposits"}
};
```

---

## 🔄 **Calculation Flow**

### **Resource Level Calculation Process**

```
1. Input: (resource_type, x, y)
   ↓
2. Cache Check: cache_find_resource_values(x, y)
   ↓ [Cache MISS]
3. Base Calculation: PerlinNoise2D(layer, norm_x, norm_y, ...)
   ↓
4. Normalization: ((noise + 1.0) / 2.0) * base_multiplier
   ↓
5. Region Modifiers: apply_region_resource_modifiers(...)
   ↓
6. Environmental Modifiers: apply_environmental_modifiers(...)
   ↓
7. Harvest History: apply_harvest_regeneration(...)
   ↓
8. Range Limiting: LIMIT(value, 0.0, 1.0)
   ↓
9. Cache Storage: cache_store_resource_values(x, y, all_values)
   ↓
10. Return: final_resource_level
```

### **Coordinate Normalization**

```c
/* Convert world coordinates to Perlin noise coordinates */
double norm_x = x / (double)(WILD_X_SIZE / 4.0);    /* WILD_X_SIZE = 2048 */
double norm_y = y / (double)(WILD_Y_SIZE / 4.0);    /* WILD_Y_SIZE = 2048 */

/* Example: coordinate (-15, -6) becomes (-0.029, -0.012) */
```

### **Cache Grid System**

```c
/* Snap coordinates to cache grid */
static void get_cache_coordinates(int x, int y, int *cache_x, int *cache_y) {
    *cache_x = (x / RESOURCE_CACHE_GRID_SIZE) * RESOURCE_CACHE_GRID_SIZE;
    *cache_y = (y / RESOURCE_CACHE_GRID_SIZE) * RESOURCE_CACHE_GRID_SIZE;
}

/* Example: coordinates (-15, -6) snap to cache grid (-20, -10) */
```

---

## 🎨 **Visual System**

### **Resource Density Symbols**

```c
char get_resource_map_symbol(float level) {
    if (level >= 0.9) return '█';         /* Very high (90%+) */
    if (level >= 0.7) return '▓';         /* High (70-89%) */
    if (level >= 0.5) return '▒';         /* Medium-high (50-69%) */
    if (level >= 0.3) return '░';         /* Medium (30-49%) */
    if (level >= 0.1) return '▪';         /* Low (10-29%) */
    if (level >= 0.05) return '·';        /* Very low (5-9%) */
    return ' ';                           /* None (0-4%) */
}
```

### **Color Coding System**

```c
const char *get_resource_color(float level) {
    if (level >= 0.8) return "\t[f046]";      /* Bright green */
    if (level >= 0.6) return "\t[f034]";      /* Green */
    if (level >= 0.4) return "\t[f226]";      /* Yellow */
    if (level >= 0.2) return "\t[f208]";      /* Orange */
    if (level >= 0.05) return "\t[f240]";     /* Gray */
    return "\t[f236]";                        /* Dark gray */
}
```

---

## 🔍 **Debugging and Monitoring**

### **Debug Output Interpretation**

```
DEBUG Resource System Analysis for (-15, -6):
===================================================
Terrain: Field (Type: 2)                    # Sector type affecting resources
Elevation: 150                              # Height affecting water/minerals

Resource Details:
Resource     | Base     | Modified | Final    | Layer | Description
-------------|----------|----------|----------|-------|------------------
vegetation   |    0.573 |    0.573 |    1.032 |     4 | General plant life
             ^          ^          ^          ^       ^
             |          |          |          |       Perlin layer used
             |          |          |          Display description
             |          |          Final value (with env modifiers)
             |          After region modifiers
             Base Perlin noise calculation
```

### **Cache Debug Information**

```
Spatial Cache Statistics:
  Total cached nodes: 15                     # Active cache entries
  Expired nodes: 3                          # Entries past lifetime
  Cache grid size: 10 coordinates           # Grid resolution
  Cache lifetime: 300 seconds               # 5 minute expiration
  Current position cache grid: (-20, -10)   # Grid coordinates for this location
  Cache status: HIT                         # This lookup used cache
```

---

## 🛠️ **Integration Points**

### **Existing System Integration**

1. **Wilderness System:** Uses existing Perlin noise infrastructure
2. **Zone System:** Respects wilderness zone flags
3. **Skills System:** Ready for skill-based harvesting integration
4. **Weather System:** Framework for weather-based modifiers
5. **Region System:** Framework for biome-specific modifiers

### **Future Extension Points**

1. **Harvesting Commands:** `harvest`, `mine`, `forage` commands
2. **Tool Integration:** Pickaxes, axes, gathering baskets
3. **Inventory System:** Resource items and storage
4. **Crafting Integration:** Resource-based crafting recipes
5. **Economy System:** Resource trading and markets

---

## ⚡ **Performance Considerations**

### **Memory Usage**

- **Cache Nodes:** ~100 bytes per cached grid position
- **Maximum Memory:** ~100KB at 1000 cached nodes
- **Cleanup Strategy:** Automatic expiration and size limits

### **CPU Performance**

- **Cache Hit:** O(log n) lookup in KD-tree
- **Cache Miss:** O(1) Perlin noise calculation + O(log n) storage
- **Batch Calculation:** All 10 resource types calculated together

### **Scalability**

- **Concurrent Access:** Thread-safe with proper locking (future enhancement)
- **Database Impact:** Minimal - only KD-tree nodes for harvest history
- **Network Impact:** Efficient text output, minimal bandwidth

---

## 🚨 **Error Handling**

### **Common Error Conditions**

1. **Invalid Resource Type:** Range checking (0 to NUM_RESOURCE_TYPES-1)
2. **Non-Wilderness Access:** Zone flag checking before calculations
3. **Memory Allocation:** Graceful handling of CREATE() failures
4. **Cache Overflow:** Automatic cleanup when limits exceeded

### **Error Recovery**

- Cache failures fall back to direct calculation
- Invalid coordinates return 0.0 resource levels
- System continues operation even with cache errors

---

## 📈 **Future Development Roadmap**

### **Phase 4: Region Integration (Planned)**
- Biome-specific resource modifiers
- Climate and seasonal effects
- Region boundary impact on resources

### **Phase 5: Harvesting Mechanics (Planned)**
- Player resource collection commands
- Tool requirements and skill checks
- Resource depletion and regeneration

### **Phase 6: Advanced Features (Future)**
- Resource quality tiers (poor to legendary)
- Dynamic resource spawning events
- Player-driven resource economy

---

**Developer Contact:** Implementation Team  
**Code Reviews:** Submit via standard process  
**Architecture Questions:** Technical lead consultation  
**Performance Issues:** Include debug output and profiling data
