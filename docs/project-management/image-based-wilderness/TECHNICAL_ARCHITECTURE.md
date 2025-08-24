# Technical Architecture for Image-Based Wilderness

## System Architecture Overview

The image-based wilderness system introduces an alternative **base terrain generation method only** that coexists with the existing Perlin noise system through conditional compilation. 

**Phase 1 Implementation**: Developers manually update `WILD_X_SIZE` and `WILD_Y_SIZE` to match image dimensions before compilation. This ensures all existing scaling functions, resource calculations, and mathematical operations work unchanged.

**Phase 2 Future Enhancement**: Dynamic runtime coordinate system that automatically adapts to image dimensions without recompilation, including unified sizing for both image and Perlin noise modes.

Weather, resources, and other noise layers continue using Perlin noise but automatically scale with the updated size constants.

```
┌─────────────────────────────────────────────────────────────┐
│                    Campaign Configuration                    │
├─────────────────────────────────────────────────────────────┤
│  #ifdef USE_IMAGE_WILDERNESS                               │
│    └─ Image-based BASE TERRAIN ONLY                        │
│       • get_elevation(NOISE_MATERIAL_PLANE_ELEV, x, y)     │
│       • get_moisture(NOISE_MATERIAL_PLANE_MOISTURE, x, y)   │
│       • get_temperature(NOISE_MATERIAL_PLANE_ELEV, x, y)    │
│    └─ Perlin noise for ALL OTHER LAYERS                    │
│       • get_weather(x, y) - unchanged                      │
│       • Resource noise layers - unchanged                  │
│       • All other noise layers - unchanged                 │
│  #else                                                      │
│    └─ Perlin noise terrain generation (all layers)         │
│  #endif                                                     │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│                   Wilderness Interface                      │
├─────────────────────────────────────────────────────────────┤
│  BASE TERRAIN (IMAGE):                                     │
│  get_elevation(NOISE_MATERIAL_PLANE_ELEV, x, y) → [0-255]  │
│  get_moisture(NOISE_MATERIAL_PLANE_MOISTURE, x, y) → [0-255]│
│  get_temperature(NOISE_MATERIAL_PLANE_ELEV, x, y) → [-30,35]│
│  get_sector_type(e, t, m) → sector_type                    │
│                                                             │
│  OTHER LAYERS (PERLIN):                                    │
│  get_weather(x, y) → [0-255] - UNCHANGED                   │
│  get_elevation(NOISE_VEGETATION, x, y) → [0-255]           │
│  get_moisture(NOISE_HERBS, x, y) → [0-255]                 │
│  ... all other resource layers - UNCHANGED                 │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│                    Game Systems                             │
├─────────────────────────────────────────────────────────────┤
│  • Room Assignment (assign_wilderness_room)                │
│  • Map Generation (make_wild_map)                          │
│  • Resource System (continues using mixed layers)          │
│  • Weather System (continues using Perlin noise)           │
│  • Region System                                           │
│  • Navigation System                                       │
└─────────────────────────────────────────────────────────────┘
```

## Data Flow Architecture

### Image-Based System Data Flow

```
┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐
│   Wilderness    │    │   Color-to-     │    │    World        │
│   Image File    │───▶│   Sector Type   │───▶│  Coordinates    │
│   (PNG/BMP)     │    │   Mapping       │    │   (x, y)        │
└─────────────────┘    └─────────────────┘    └─────────────────┘
                                                      │
                                                      ▼
┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐
│   Terrain       │◄───│ Sector-to-Value │◄───│   Image Pixel   │
│   Properties    │    │   Mapping       │    │   Lookup        │
│ • Elevation     │    │ + Latitude      │    │  RGB(r,g,b)     │
│ • Moisture      │    │   Modifier      │    │      ↓          │
│ • Temperature   │    │                 │    │  Sector Type    │
│ • Sector Type   │    │                 │    │                 │
└─────────────────┘    └─────────────────┘    └─────────────────┘
```

### Coordinate Mapping System

```
Image-Based Wilderness:
World Coordinates          Image Coordinates         Terrain Properties
─────────────────          ──────────────────        ──────────────────

X: [-(width/2), (width/2)-1]    ═══▶   img_x: [0, width-1]       Convert: world_x + (width/2) → img_x
Y: [-(height/2), (height/2)-1]  ═══▶   img_y: [0, height-1]      Convert: world_y + (height/2) → img_y
(Center origin)                                       (Top-left origin)
                                                      
                           RGB: pixel(img_x, img_y)  
                                      ↓               
                           Sector Type (from color)   
                                      ↓               
                           Elevation = f(sector_type)
                           Moisture = f(sector_type)
                           Temperature = f(sector_type, latitude)

Traditional Perlin Noise:
World Coordinates          Noise Coordinates         Terrain Properties
─────────────────          ──────────────────        ──────────────────

X: [-(width/2), (width/2)-1]   ───▶   noise_x: scaled coords     Elevation = PerlinNoise(x,y)
Y: [-(height/2), (height/2)-1] ───▶   noise_y: scaled coords     Moisture = PerlinNoise(x,y)
(Center origin)                                                   Temperature = f(latitude, elevation)
```

## Core Data Structures

### Image Data Management

```c
/* Global image data structure */
struct image_wilderness_data {
    unsigned char *image_data;    /* Raw RGB/RGBA pixel data */
    int width;                    /* Image width in pixels */
    int height;                   /* Image height in pixels */
    int channels;                 /* 3=RGB, 4=RGBA */
    int bytes_per_pixel;         /* Usually 3 or 4 */
    int loaded;                   /* Loading status flag */
    char *filename;               /* Path to image file */
    time_t load_time;            /* When image was loaded */
    size_t file_size;            /* Original file size */
};

/* Global instance */
extern struct image_wilderness_data *wilderness_image;
```

### Color-Terrain Mapping

```c
/* Color to terrain type mapping */
struct terrain_color_map {
    unsigned char red;            /* RGB red component [0-255] */
    unsigned char green;          /* RGB green component [0-255] */
    unsigned char blue;           /* RGB blue component [0-255] */
    int sector_type;             /* Corresponding SECT_* constant */
    const char *name;            /* Human-readable name */
    const char *description;     /* Extended description */
    int elevation_hint;          /* Optional elevation override */
    int moisture_hint;           /* Optional moisture override */
    int temperature_hint;        /* Optional temperature override */
};

/* Predefined color mappings */
extern struct terrain_color_map terrain_colors[];
```

### Performance Optimization Structures

```c
/* Pixel cache for frequently accessed coordinates */
struct pixel_cache_entry {
    int world_x, world_y;        /* World coordinates (center-origin, need conversion) */
    unsigned char rgb[3];        /* Cached RGB values */
    int sector_type;             /* Cached sector type */
    time_t access_time;          /* Last access timestamp */
};

/* LRU cache for pixel lookups */
#define PIXEL_CACHE_SIZE 1024
struct pixel_cache {
    struct pixel_cache_entry entries[PIXEL_CACHE_SIZE];
    int next_slot;               /* Round-robin replacement */
    int hits;                    /* Cache hit counter */
    int misses;                  /* Cache miss counter */
};
```

## Function Interface Specifications

### Core API Functions

```c
/* Image loading and management */
int load_wilderness_image(const char *filename);
    /* Returns: 0 on success, -1 on error */
    /* Side effects: Allocates memory for image data */
    /* Thread safety: Not thread-safe, call during initialization */

void free_wilderness_image(void);
    /* Returns: void */
    /* Side effects: Frees all image memory */
    /* Thread safety: Not thread-safe */

int reload_wilderness_image(void);
    /* Returns: 0 on success, -1 on error */
    /* Side effects: Frees old image, loads new one */
    /* Thread safety: Not thread-safe */

/* Coordinate mapping - Convert center-origin world coords to top-left image coords */
int map_world_to_image_x(int world_x);
int map_world_to_image_y(int world_y);
    /* Input: World coordinates [-(width/2), (width/2)-1] with center origin */
    /* Returns: Image coordinates [0, width-1] with top-left origin */
    /* Algorithm: img_x = world_x + (width/2), with bounds checking */

int map_image_to_world_x(int img_x);
int map_image_to_world_y(int img_y);
    /* Input: Image coordinates [0, width/height-1] with top-left origin */
    /* Returns: World coordinates [-(width/2), (width/2)-1] with center origin */
    /* Algorithm: world_x = img_x - (width/2), with bounds checking */

/* Pixel access */
unsigned char *get_pixel_color(int world_x, int world_y);
    /* Returns: Pointer to RGB array [r,g,b] or NULL on error */
    /* Caching: Uses LRU cache for performance */
    /* Thread safety: Read-only after initialization */

int get_sector_from_color(unsigned char r, unsigned char g, unsigned char b);
    /* Returns: sector_type constant or -1 if no match */
    /* Algorithm: Euclidean distance in RGB space */
    /* Tolerance: Configurable color matching threshold */

/* Base terrain calculation functions - ONLY THESE USE IMAGE DATA */
int get_elevation_from_image(int x, int y);
int get_moisture_from_image(int x, int y);
int get_temperature_from_image(int x, int y);
int get_sector_from_image(int x, int y);
    /* Returns: Calculated values using image-based algorithms */
    /* Fallback: Returns Perlin noise values on image error */
    /* Scope: ONLY for base terrain layers */

/* Perlin noise scaling functions - for non-base layers */
double get_scaled_perlin_coordinate_x(int x, double frequency);
double get_scaled_perlin_coordinate_y(int y, double frequency);
    /* Returns: Scaled coordinates for consistent noise resolution */
    /* Purpose: Match noise layer scale to image dimensions */
    /* Used by: Weather, resources, and other non-base layers */
```

### Error Handling and Validation

```c
/* Error codes */
#define IMAGE_WILD_SUCCESS           0
#define IMAGE_WILD_FILE_NOT_FOUND   -1
#define IMAGE_WILD_CORRUPT_FILE     -2
#define IMAGE_WILD_UNSUPPORTED_FORMAT -3
#define IMAGE_WILD_MEMORY_ERROR     -4
#define IMAGE_WILD_INVALID_SIZE     -5

/* Validation functions */
int validate_image_dimensions(int width, int height);
int validate_pixel_coordinates(int x, int y);
int validate_color_mapping(void);

/* Diagnostic functions */
void image_wilderness_stats(struct char_data *ch);
void dump_color_histogram(const char *filename);
void test_coordinate_mapping(struct char_data *ch);
```

## Algorithm Specifications

### Coordinate Mapping Algorithm

```c
/* Direct coordinate mapping for image-based wilderness */
int map_world_to_image_x(int world_x) {
    if (!wilderness_image || !wilderness_image->loaded) {
        return -1;  /* Error: no image loaded */
    }
    
    /* Convert center-origin world coords to top-left image coords */
    /* world_x range: [-1024, 1024] → img_x range: [0, width-1] */
    int img_x = world_x + (wilderness_image->width / 2);
    
    /* Ensure image coordinates are within bounds */
    if (img_x < 0) return 0;
    if (img_x >= wilderness_image->width) return wilderness_image->width - 1;
    
    return img_x;
}

int map_world_to_image_y(int world_y) {
    if (!wilderness_image || !wilderness_image->loaded) {
        return -1;  /* Error: no image loaded */
    }
    
    /* Convert center-origin world coords to top-left image coords */
    /* world_y range: [-1024, 1024] → img_y range: [0, height-1] */
    int img_y = world_y + (wilderness_image->height / 2);
    
    /* Ensure image coordinates are within bounds */
    if (img_y < 0) return 0;
    if (img_y >= wilderness_image->height) return wilderness_image->height - 1;
    
    return img_y;
}
```

### Elevation Calculation Algorithm

```c
/* Sector-type-based elevation calculation */
int get_elevation_from_image(int x, int y) {
    if (!wilderness_image || !wilderness_image->loaded) {
        return get_elevation(NOISE_MATERIAL_PLANE_ELEV, x, y);  /* Fallback */
    }
    
    /* Get sector type from image color */
    int sector = get_sector_from_image_color(x, y);
    if (sector == -1) {
        return get_elevation(NOISE_MATERIAL_PLANE_ELEV, x, y);  /* Fallback */
    }
    
    /* Return elevation based on sector type from lookup table */
    if (sector >= 0 && sector < NUM_SECT_TYPES && sector_elevation_map[sector] > 0) {
        return sector_elevation_map[sector];
    }
    
    return 150;  /* Default elevation */
}

/* Example sector elevation mapping */
static const int sector_elevation_map[] = {
    [SECT_OCEAN] = 60,           /* Below waterline */
    [SECT_WATER_SWIM] = 120,     /* Near waterline */
    [SECT_BEACH] = 140,          /* Just above waterline */
    [SECT_FIELD] = 160,          /* Plains level */
    [SECT_HILLS] = 200,          /* Elevated terrain */
    [SECT_MOUNTAIN] = 230,       /* High elevations */
    [SECT_HIGH_MOUNTAIN] = 250,  /* Highest peaks */
    [SECT_DESERT] = 180,         /* Elevated arid regions */
    /* ... etc for all sector types ... */
};
```
```

### Color Matching Algorithm

```c
/* Euclidean distance color matching */
int get_sector_from_color(unsigned char r, unsigned char g, unsigned char b) {
    int best_match = SECT_INSIDE;  /* Default fallback */
    int min_distance = INT_MAX;
    
    for (int i = 0; terrain_colors[i].name != NULL; i++) {
        /* Calculate Euclidean distance in RGB space */
        int dr = (int)r - (int)terrain_colors[i].red;
        int dg = (int)g - (int)terrain_colors[i].green;
        int db = (int)b - (int)terrain_colors[i].blue;
        
        int distance = dr*dr + dg*dg + db*db;
        
        if (distance < min_distance) {
            min_distance = distance;
            best_match = terrain_colors[i].sector_type;
        }
        
        /* Exact match - can stop early */
        if (distance == 0) {
            break;
        }
    }
    
    /* Log warning for poor color matches */
    if (min_distance > COLOR_MATCH_THRESHOLD) {
        log("WARN: Poor color match for RGB(%d,%d,%d), distance=%d", 
            r, g, b, min_distance);
    }
    
    return best_match;
}
```

### Pixel Caching System

```c
/* LRU cache implementation for pixel access */
unsigned char *get_pixel_color_cached(int world_x, int world_y) {
    static struct pixel_cache cache = {0};
    
    /* Check cache first */
    for (int i = 0; i < PIXEL_CACHE_SIZE; i++) {
        if (cache.entries[i].world_x == world_x && 
            cache.entries[i].world_y == world_y) {
            cache.entries[i].access_time = time(NULL);
            cache.hits++;
            return cache.entries[i].rgb;
        }
    }
    
    /* Cache miss - load from image */
    cache.misses++;
    unsigned char *pixel = get_pixel_color_direct(world_x, world_y);
    if (pixel) {
        /* Store in cache */
        int slot = cache.next_slot;
        cache.entries[slot].world_x = world_x;
        cache.entries[slot].world_y = world_y;
        memcpy(cache.entries[slot].rgb, pixel, 3);
        cache.entries[slot].access_time = time(NULL);
        
        cache.next_slot = (cache.next_slot + 1) % PIXEL_CACHE_SIZE;
    }
    
    return pixel;
}
```

## Integration Points

### Campaign System Integration

```c
/* Campaign-specific image selection */
const char *get_wilderness_image_path(void) {
#if defined(CAMPAIGN_DL)
    return "lib/world/krynn_wilderness.png";
#elif defined(CAMPAIGN_FR)
    return "lib/world/faerun_wilderness.png";
#else
    return "lib/world/wilderness_map.png";
#endif
}

/* Campaign-specific color mappings */
struct terrain_color_map *get_terrain_colors(void) {
#if defined(CAMPAIGN_DL)
    return dragonlance_terrain_colors;
#elif defined(CAMPAIGN_FR)
    return forgotten_realms_terrain_colors;
#else
    return standard_terrain_colors;
#endif
}
```

### Configuration System Integration

```c
/* Configuration options */
#define CONFIG_IMAGE_WILDERNESS_ENABLED  config_info.extra.image_wilderness
#define CONFIG_WILDERNESS_IMAGE_PATH     config_info.extra.wilderness_image_path
#define CONFIG_COLOR_MATCH_THRESHOLD     config_info.extra.color_match_threshold
#define CONFIG_PIXEL_CACHE_SIZE          config_info.extra.pixel_cache_size

/* Runtime configuration validation */
int validate_wilderness_config(void) {
    if (CONFIG_IMAGE_WILDERNESS_ENABLED) {
        if (!file_exists(CONFIG_WILDERNESS_IMAGE_PATH)) {
            log("ERROR: Wilderness image not found: %s", CONFIG_WILDERNESS_IMAGE_PATH);
            return -1;
        }
        
        if (CONFIG_COLOR_MATCH_THRESHOLD < 0 || CONFIG_COLOR_MATCH_THRESHOLD > 1000) {
            log("ERROR: Invalid color match threshold: %d", CONFIG_COLOR_MATCH_THRESHOLD);
            return -1;
        }
    }
    return 0;
}
```

## Memory Management

### Memory Layout

```
Wilderness Image Memory Layout:
┌─────────────────────────────────────┐
│ struct image_wilderness_data        │  ← 64 bytes
├─────────────────────────────────────┤
│ Image pixel data                    │  ← width × height × channels
│ (2048 × 2048 × 3 = ~12MB for RGB)   │
├─────────────────────────────────────┤
│ Pixel cache                         │  ← PIXEL_CACHE_SIZE × cache_entry
│ (1024 × 32 bytes = 32KB)            │
├─────────────────────────────────────┤
│ Color mapping table                 │  ← terrain_colors array (~1KB)
└─────────────────────────────────────┘
Total: ~12MB + overhead
```

### Memory Management Functions

```c
/* Memory allocation and cleanup */
static void *image_malloc(size_t size) {
    void *ptr = malloc(size);
    if (!ptr) {
        log("SYSERR: Failed to allocate %zu bytes for image wilderness", size);
    }
    return ptr;
}

static void image_free(void *ptr) {
    if (ptr) {
        free(ptr);
    }
}

/* Cleanup on shutdown */
void cleanup_image_wilderness(void) {
    if (wilderness_image) {
        image_free(wilderness_image->image_data);
        image_free(wilderness_image->filename);
        image_free(wilderness_image);
        wilderness_image = NULL;
    }
    
    /* Clear pixel cache */
    memset(&pixel_cache, 0, sizeof(pixel_cache));
}
```

## Performance Considerations

### Benchmark Targets

| Operation | Target Time | Acceptable Range |
|-----------|-------------|------------------|
| Image loading | < 5 seconds | 1-10 seconds |
| Pixel lookup (cached) | < 1 microsecond | < 10 microseconds |
| Pixel lookup (uncached) | < 10 microseconds | < 100 microseconds |
| Color matching | < 1 microsecond | < 5 microseconds |
| Coordinate mapping | < 0.1 microseconds | < 1 microsecond |

### Optimization Strategies

```c
/* Compiler optimizations */
#ifdef __GNUC__
    #define INLINE __inline__ __attribute__((always_inline))
    #define HOT __attribute__((hot))
    #define PURE __attribute__((pure))
#else
    #define INLINE inline
    #define HOT
    #define PURE
#endif

/* Hot path functions */
INLINE HOT int map_world_to_image_x_fast(int world_x) {
    /* Optimized version for center-origin coordinate conversion */
    return world_x + (WILDERNESS_IMAGE_WIDTH / 2);
}

/* Cache-friendly memory access patterns */
PURE INLINE unsigned char *get_pixel_fast(int img_x, int img_y) {
    return &wilderness_image->image_data[(img_y * wilderness_image->width + img_x) * 3];
}
```

This technical architecture provides the foundation for implementing a robust, performant image-based wilderness system that integrates seamlessly with the existing codebase.
