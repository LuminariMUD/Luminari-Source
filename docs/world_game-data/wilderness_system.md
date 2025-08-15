# Luminari MUD Wilderness System Documentation

## Table of Contents
1. [Overview](#overview)
2. [System Architecture](#system-architecture)
3. [Coordinate System](#coordinate-system)
4. [Terrain Generation](#terrain-generation)
5. [Room Management](#room-management)
6. [Regions and Paths](#regions-and-paths)
7. [Weather System](#weather-system)
8. [Resource System](#resource-system)
9. [PubSub Integration](#pubsub-integration)
10. [Spatial Audio System](#spatial-audio-system)
11. [Player Experience](#player-experience)
12. [Builder Tools](#builder-tools)
13. [Technical Implementation](#technical-implementation)
14. [Configuration](#configuration)
15. [Troubleshooting](#troubleshooting)

## Overview

The Luminari MUD Wilderness System is a sophisticated procedural terrain generation and management system that creates vast, explorable wilderness areas. The system combines:

- **Procedural Generation**: Uses Perlin noise algorithms to generate realistic terrain
- **Dynamic Room Creation**: Creates rooms on-demand as players explore
- **Static Room Support**: Allows builders to create permanent wilderness locations
- **Region System**: Defines special areas with unique properties
- **Path System**: Creates roads, rivers, and other linear features
- **Weather Integration**: Location-specific weather patterns
- **Resource System**: Comprehensive natural resource discovery and management
- **PubSub Messaging**: Event-driven communication and spatial audio
- **Spatial Audio**: 3D positional audio for wilderness events
- **Coordinate-Based Navigation**: X,Y coordinate system for precise positioning

### Key Features

- **Infinite Exploration**: 2048x2048 coordinate grid (4+ million possible locations)
- **Realistic Terrain**: Elevation, moisture, and temperature-based biome generation
- **Natural Resources**: 10 resource types with procedural distribution and depletion
- **Spatial Communication**: 3D audio and visual effects with distance-based delivery
- **Event-Driven Architecture**: PubSub system for real-time wilderness events
- **Performance Optimized**: KD-Tree indexing and dynamic room pooling
- **Builder Friendly**: OLC integration and buildwalk support
- **Player Immersive**: Integrated mapping, weather, resource discovery, and spatial audio

## System Architecture

### Core Components

```
wilderness.c/h              - Main wilderness engine
perlin.c/h                  - Noise generation algorithms
mysql.c/h                   - Database integration for regions/paths
weather.c                   - Weather system integration
act.movement.c              - Movement handling
redit.c                     - OLC wilderness room editing
resource_system.c/h         - Natural resource management
resource_descriptions.c/h   - Resource discovery and mapping
pubsub.c/h                  - Event-driven messaging system
spatial_core.c/h            - 3D spatial audio/visual systems
spatial_audio.c/h           - Wilderness audio positioning
spatial_visual.c/h          - Visual event transmission
systems/pubsub/*            - PubSub subsystem components
systems/spatial/*           - Spatial system components
```

### Data Flow

```
Player Movement → Coordinate Calculation → Room Lookup/Creation → 
Terrain Generation → Resource Calculation → Region/Path Application → 
Room Assignment → Weather/Description Generation → Spatial Event Processing → 
PubSub Event Distribution → Player Display/Audio Delivery
```

### System Integration

The wilderness system integrates multiple subsystems:

1. **Terrain Engine**: Procedural generation using Perlin noise
2. **Resource Engine**: Natural resource distribution and tracking
3. **Event Engine**: PubSub messaging for real-time communication
4. **Spatial Engine**: 3D audio/visual positioning and delivery
5. **Database Engine**: Persistent storage for regions, paths, and resources
6. **Weather Engine**: Environmental conditions affecting gameplay

## Coordinate System

### Coordinate Space
- **Range**: -1024 to +1024 on both X and Y axes
- **Origin**: (0,0) at the center of the wilderness
- **Directions**: 
  - North: +Y
  - South: -Y  
  - East: +X
  - West: -X

### Room VNUM Allocation

```c
#define WILD_ROOM_VNUM_START 1000000        // Static wilderness rooms
#define WILD_ROOM_VNUM_END 1003999          // End of static range
#define WILD_DYNAMIC_ROOM_VNUM_START 1004000 // Dynamic room pool
#define WILD_DYNAMIC_ROOM_VNUM_END 1009999   // End of dynamic range
```

### Coordinate Storage
Each wilderness room stores its coordinates in `world[room].coords[]`:
- `coords[0]` = X coordinate
- `coords[1]` = Y coordinate

## Terrain Generation

### Noise Layers
The system uses multiple Perlin noise layers:

1. **Elevation** (`NOISE_MATERIAL_PLANE_ELEV`)
   - Seed: 822344
   - Creates mountains, hills, valleys
   - Ridged multifractal for realistic mountain ranges
   - Function: `get_elevation(NOISE_MATERIAL_PLANE_ELEV, x, y)`

2. **Moisture** (`NOISE_MATERIAL_PLANE_MOISTURE`)
   - Seed: 834
   - Determines wetness/dryness
   - Affects vegetation and water features
   - Function: `get_moisture(NOISE_MATERIAL_PLANE_MOISTURE, x, y)`

3. **Elevation Distortion** (`NOISE_MATERIAL_PLANE_ELEV_DIST`)
   - Seed: 74233
   - Adds variation to elevation patterns
   - Creates island-like terrain features
   - Used to modulate base elevation with radial gradient

4. **Temperature** (calculated from latitude and elevation)
   - Gradient based on distance from equator (Y=0)
   - Modified by elevation: `temp = base_temp - (MAX(1.5 * elevation - WATERLINE, 0)) / 10`
   - Range: -30°C to +35°C
   - Function: `get_temperature(NOISE_MATERIAL_PLANE_ELEV, x, y)`
   - Affects biome selection

5. **Weather** (`NOISE_WEATHER`)
   - Seed: 43425
   - Local weather pattern variations
   - Uses 3D noise with time component for dynamic weather
   - Function: `get_weather(x, y)`

### Temperature System Details

The temperature system provides realistic climate variation across the wilderness:

```c
int get_temperature(int map, int x, int y) {
    int max_temp = 35;      // Maximum temperature at equator
    int min_temp = -30;     // Minimum temperature at poles
    int equator = 0;        // Y coordinate of equator
    int dist = abs(y - equator);

    // Calculate gradient percentage
    double pct = (double)(dist / (double)(WILD_Y_SIZE - equator));

    // Apply temperature gradient and elevation modifier
    int temp = (max_temp - (max_temp - min_temp) * pct) -
               (MAX(1.5 * get_elevation(map, x, y) - WATERLINE, 0)) / 10;

    return temp;
}
```

**Temperature Features:**
- **Latitude Gradient**: Temperature decreases with distance from equator (Y=0)
- **Elevation Cooling**: Higher elevations are colder using 1.5x elevation multiplier
- **Range**: -30°C to +35°C across the entire wilderness
- **Critical Thresholds**:
  - 8°C: Tundra/marshland boundary
  - 10°C: Taiga formation threshold
  - 18°C: Jungle formation threshold
  - 25°C: Desert formation threshold
- **Integration**: Used in `get_sector_type()` for biome determination

### Sector Type Determination

The system combines elevation, temperature, and moisture to determine terrain types:

```c
int get_sector_type(int elevation, int temperature, int moisture) {
    int waterline = wild_waterline; // Default: 128

    // Water sectors based on elevation
    if (elevation < waterline) {
        if (elevation > waterline - SHALLOW_WATER_THRESHOLD) // 20
            return SECT_WATER_SWIM;
        else
            return SECT_OCEAN;
    }

    // Coastal and low elevation sectors
    if (elevation < waterline + COASTLINE_THRESHOLD) { // 5
        if ((moisture > 180) && (temperature > 8))
            return SECT_MARSHLAND;
        else
            return SECT_BEACH;
    }

    // Plains elevation range
    else if (elevation < waterline + PLAINS_THRESHOLD) { // 35
        if ((moisture > 180) && (temperature > 8))
            return SECT_MARSHLAND;
        else if (temperature < 8)
            return SECT_TUNDRA;
        else if ((temperature > 25) && (moisture < 80))
            return SECT_DESERT;
        else
            return SECT_FIELD;
    }

    // High mountain peaks (impassable)
    else if (elevation > 255 - HIGH_MOUNTAIN_THRESHOLD) // > 200
        return SECT_HIGH_MOUNTAIN;

    // Mountain ranges
    else if (elevation > 255 - MOUNTAIN_THRESHOLD) // > 185
        return SECT_MOUNTAIN;

    // Hills and elevated terrain
    else if (elevation > 255 - HILL_THRESHOLD) { // > 175
        if ((temperature < 10) && (moisture > 128))
            return SECT_TAIGA;
        else
            return SECT_HILLS;
    }

    // Moderate elevation forested areas
    else {
        if (temperature < 10)
            return SECT_TAIGA;
        else if ((temperature > 18) && (moisture > 180))
            return SECT_JUNGLE;
        else
            return SECT_FOREST;
    }
}

**Sector Types Include:**
- `SECT_FIELD` - Grasslands and plains (moderate elevation, moderate conditions)
- `SECT_FOREST` - Wooded areas (moderate elevation, default forest type)
- `SECT_JUNGLE` - Tropical forests (temperature > 18°C, moisture > 180)
- `SECT_TAIGA` - Coniferous forests (temperature < 10°C, moisture > 128 for hills)
- `SECT_HILLS` - Rolling hills (elevation > 175, moderate temperature)
- `SECT_MOUNTAIN` - Mountain peaks (elevation > 185)
- `SECT_HIGH_MOUNTAIN` - Impassable peaks (elevation > 200)
- `SECT_WATER_SWIM` - Shallow water (elevation < waterline, > waterline - 20)
- `SECT_OCEAN` - Deep water (elevation < waterline - 20)
- `SECT_RIVER` - Flowing water (path override)
- `SECT_BEACH` - Coastal areas (elevation < waterline + 5, dry conditions)
- `SECT_MARSHLAND` - Wetlands (low elevation, moisture > 180, temperature > 8°C)
- `SECT_DESERT` - Arid regions (plains elevation, temperature > 25°C, moisture < 80)
- `SECT_TUNDRA` - Cold plains (plains elevation, temperature < 8°C)

### Terrain Visualization

Each sector type has associated map symbols and colors:

```c
struct wild_map_info_type wild_map_info[] = {
    {SECT_INSIDE, "\tn\t[u65294/.]\tn", {NULL}},
    {SECT_CITY, "\tw\t[u127984/C]\tn", {NULL}},
    {SECT_FIELD, "\tg\t[u65292],\tn", {variant_glyphs}},      // Green comma
    {SECT_FOREST, "\tG\t[u127795/Y]\tn", {variant_glyphs}},   // Green tree
    {SECT_HILLS, "\ty\t[u65358]\t[u65358/n]\tn", {NULL}},
    {SECT_MOUNTAIN, "\tw\t[u127956/^]\tn", {NULL}},           // White caret
    {SECT_WATER_SWIM, "\tB\t[u65374/~]\tn", {NULL}},
    {SECT_WATER_NOSWIM, "\tb\t[u65309/=]\tn", {NULL}},
    {SECT_OCEAN, "\tb\t[u65309/=]\tn", {variant_glyphs}},
    {SECT_RIVER, "\tB\t[u65374/~]\tn", {NULL}},
    {SECT_BEACH, "\ty\t[u65306]\t[u65306/:]\tn", {NULL}},
    // ... more terrain types
};
```

### Variant Glyphs System

Some terrain types support variant glyphs for visual diversity:

```c
#define NUM_VARIANT_GLYPHS 4

// Fields have 4 different grass variations
{SECT_FIELD, "\tg\t[u65292],\tn", {
    "\t[F120]\t[u65292],\tn",  // Variant 1
    "\t[F121]\t[u65292],\tn",  // Variant 2
    "\t[F130]\t[u65292],\tn",  // Variant 3
    "\t[F131]\t[u65292],\tn"   // Variant 4
}},

// Forests have 4 different tree variations
{SECT_FOREST, "\tG\t[u127795/Y]\tn", {
    "\t[f020]\t[u127795/Y]\tn", // Variant 1
    "\t[f030]\t[u127795/Y]\tn", // Variant 2
    "\t[f040]\t[u127795/Y]\tn", // Variant 3
    "\t[f050]\t[u127795/Y]\tn"  // Variant 4
}}
```

Variant selection is based on coordinate-based noise values to ensure consistent patterns across server restarts.

## Room Management

### Static vs Dynamic Rooms

**Static Rooms (1000000-1003999)**
- Pre-built by builders using OLC
- Permanent locations (cities, dungeons, landmarks)
- Indexed in KD-Tree for fast lookup
- Survive server reboots

**Dynamic Rooms (1004000-1009999)**
- Created on-demand as players explore
- Temporary - recycled when not in use
- Generated using terrain algorithms
- Flagged with `ROOM_OCCUPIED` when active

### Room Lookup Process

```c
room_rnum find_room_by_coordinates(int x, int y) {
    #ifdef CAMPAIGN_FR
        return NOWHERE; // Wilderness disabled in Forgotten Realms
    #endif

    // 1. Check static rooms first (KD-Tree lookup)
    room = find_static_room_by_coordinates(x, y);
    if (room != NOWHERE) return room;

    // 2. Check dynamic room pool
    for (i = WILD_DYNAMIC_ROOM_VNUM_START; i <= WILD_DYNAMIC_ROOM_VNUM_END; i++) {
        if (ROOM_FLAGGED(real_room(i), ROOM_OCCUPIED) &&
            world[real_room(i)].coords[X_COORD] == x &&
            world[real_room(i)].coords[Y_COORD] == y) {
            return real_room(i); // Found existing dynamic room
        }
    }

    // 3. Room not found - caller must allocate if needed
    return NOWHERE;
}
```

### Dynamic Room Allocation

```c
room_rnum find_available_wilderness_room() {
    // Search for unused room in dynamic pool
    for (i = WILD_DYNAMIC_ROOM_VNUM_START; i <= WILD_DYNAMIC_ROOM_VNUM_END; i++) {
        if (real_room(i) != NOWHERE && !ROOM_FLAGGED(real_room(i), ROOM_OCCUPIED)) {
            SET_BIT_AR(ROOM_FLAGS(real_room(i)), ROOM_OCCUPIED);
            return real_room(i);
        }
    }
    return NOWHERE; // Pool exhausted
}
```

### Room Assignment

When a new wilderness location is accessed:

```c
void assign_wilderness_room(room_rnum room, int x, int y) {
    if (room == NOWHERE) {
        log("SYSERR: Attempted to assign NOWHERE as wilderness location at (%d, %d)", x, y);
        return;
    }

    // Set default properties
    world[room].name = "The Wilderness of Luminari";
    world[room].description = "The wilderness extends in all directions.";

    // Set coordinates
    world[room].coords[0] = x;
    world[room].coords[1] = y;

    // Generate terrain-based properties
    sector_type = get_modified_sector_type(zone, x, y);
    world[room].sector_type = sector_type;

    // Apply region modifications
    regions = get_enclosing_regions(zone, x, y);
    for (curr_region = regions; curr_region != NULL; curr_region = curr_region->next) {
        switch (region_table[curr_region->rnum].region_type) {
            case REGION_SECTOR:
                world[room].sector_type = region_table[curr_region->rnum].region_props;
                break;
            // Handle other region types...
        }
    }

    // Apply path modifications
    paths = get_enclosing_paths(zone, x, y);
    for (curr_path = paths; curr_path != NULL; curr_path = curr_path->next) {
        switch (path_table[curr_path->rnum].path_type) {
            case PATH_ROAD:
            case PATH_RIVER:
                world[room].sector_type = path_table[curr_path->rnum].path_props;
                break;
        }
    }

    // Set room flags
    SET_BIT_AR(ROOM_FLAGS(room), ROOM_OCCUPIED);

    // Generate exits to navigation room for wilderness movement
    create_wilderness_exits(room);
}
```

## Regions and Paths

### Regions

Regions are polygonal areas that modify wilderness properties:

```c
struct region_data {
    region_vnum vnum;           // Unique identifier
    zone_rnum zone;             // Associated zone
    char *name;                 // Region name
    int region_type;            // Type of region
    int region_props;           // Properties/effects
    struct vertex *vertices;    // Polygon boundary
    int num_vertices;           // Number of vertices
};
```

**Region Types:**
- `REGION_GEOGRAPHIC` (1) - Named areas (forests, mountains)
- `REGION_ENCOUNTER` (2) - Special encounter zones
- `REGION_SECTOR_TRANSFORM` (3) - Changes terrain type
- `REGION_SECTOR` (4) - Overrides sector completely

**Region Position Constants:**
- `REGION_POS_UNDEFINED` (0) - Position not determined
- `REGION_POS_CENTER` (1) - At region center
- `REGION_POS_INSIDE` (2) - Inside region boundary
- `REGION_POS_EDGE` (3) - At region edge

### Paths

Paths are linear features like roads and rivers:

```c
struct path_data {
    region_vnum vnum;           // Unique identifier
    zone_rnum zone;             // Associated zone  
    char *name;                 // Path name
    int path_type;              // Type of path
    int path_props;             // Sector type override
    char *glyphs[3];            // Map display symbols
    struct vertex *vertices;    // Path route
    int num_vertices;           // Number of vertices
};
```

**Path Types:**
- `PATH_ROAD` (1) - Paved roads
- `PATH_DIRT_ROAD` (2) - Dirt roads
- `PATH_GEOGRAPHIC` (3) - Geographic features
- `PATH_RIVER` (5) - Rivers and streams
- `PATH_STREAM` (6) - Smaller streams

### Database Integration

Regions and paths are stored in MySQL database:
- `region_data` table - Region definitions with polygon geometry
- `region_index` table - Optimized spatial index for region queries
- `path_data` table - Path definitions with linestring geometry
- `path_index` table - Optimized spatial index for path queries
- `path_types` table - Path type definitions with display glyphs
- Spatial queries using MySQL's geometry functions
- `ST_Within()` for point-in-polygon testing
- `GeomFromText()` for creating geometry objects from coordinates
- `AsText()` and `NumPoints()` for extracting geometry data
- `ST_Distance()` and `Centroid()` for advanced spatial calculations

## Weather System

**3D Perlin:** Location + time component for dynamic weather (0-255 scale)

**Integration:** Per-tile weather, map overlay mode, excluded from zone weather

**Campaign FR:** Wilderness completely disabled

## Resource System

The Wilderness Resource System provides comprehensive natural resource discovery, mapping, and management integrated with the terrain generation system.

### Resource Types

The system supports 10 distinct resource types, each with unique distribution patterns:

1. **Vegetation** - General plant life and foliage
2. **Minerals** - Ores, metals, and mineral deposits  
3. **Water** - Fresh water sources and springs
4. **Herbs** - Medicinal and magical plants
5. **Game** - Wildlife for hunting and tracking
6. **Wood** - Trees suitable for lumber and crafting
7. **Stone** - Building stone and quarry materials
8. **Crystal** - Magical crystals and gems
9. **Clay** - Pottery clay and construction materials
10. **Salt** - Salt deposits and mineral salts

### Resource Distribution

Resources are distributed using sophisticated algorithms that consider multiple environmental factors:

**Terrain-Based Distribution:**
- **Forest Areas**: High vegetation (60-80%), wood (50-70%), herbs (30-50%)
- **Mountain Areas**: High stone (50-70%), minerals (40-60%), crystal (5-15%)
- **Plains/Fields**: High vegetation (50-70%), game (40-60%)
- **Desert Areas**: High crystal (10-20%), minerals (30-50%), low water (5-15%)

**Environmental Factors:**
- **Elevation**: Higher elevations favor minerals and stone
- **Moisture**: Affects vegetation, herbs, and water availability
- **Temperature**: Influences resource quality and accessibility
- **Coordinates**: Natural variation across the landscape

### Player Commands

**Basic Resource Discovery:**
```
survey resources              # Show all resource percentages at current location
survey detail <resource>      # Detailed info about specific resource
survey terrain               # Environmental factors affecting resources
```

**Visual Resource Mapping:**
```
survey map <resource>         # Show ASCII minimap (default radius 7)
survey map <resource> <radius> # Custom radius (3-15)
```

**Advanced Analysis:**
```
survey conservation          # Resource depletion and conservation status
survey regeneration         # Resource regeneration analysis  
survey ecosystem            # Ecosystem health analysis
survey impact               # Personal conservation impact
survey cascade <resource>   # Preview ecological impact of harvesting
```

### Resource Visualization

The system provides rich visual feedback through ASCII maps:

| Symbol | Density | Color | Meaning |
|--------|---------|-------|---------|
| `█` | 90%+ | 🟢 Bright Green | Very High |
| `▓` | 70-89% | 🟢 Green | High |
| `▒` | 50-69% | 🟡 Yellow | Medium-High |
| `░` | 30-49% | 🟠 Orange | Medium |
| `▪` | 10-29% | ⚫ Gray | Low |
| `·` | 5-9% | ⚫ Dark Gray | Very Low |
| ` ` | 0-4% | ⚫ Black | None |
| `@` | - | ⚪ White | Your Position |

### Administrative Tools

**System Status:**
```
resourceadmin status         # Overall system status and cache statistics
resourceadmin here          # Resources at current location
resourceadmin coords <x> <y> # Resources at specific coordinates
```

**Debug and Management:**
```
resourceadmin debug         # Comprehensive debug information
resourceadmin map <type> [radius] # Admin resource minimap
resourceadmin cache         # Cache management functions
resourceadmin cleanup       # Force cleanup of old resource nodes
```

### Technical Implementation

**Core Functions:**
- `calculate_current_resource_level(resource_type, x, y)` - Main resource calculation
- `get_resource_base_level(resource_type, sector_type)` - Terrain-based base levels
- `get_abundance_description(resource_level)` - Human-readable descriptions
- `show_resource_map(ch, resource_type, radius)` - Visual mapping

**Database Integration:**
- `resource_types` table - Resource type definitions
- `resource_depletion` table - Location-based depletion tracking
- Cache system for performance optimization
- Regeneration tracking and persistence

**Performance Features:**
- Coordinate-based caching system
- Efficient map generation algorithms
- Database-backed persistence for depletion
- Optimized queries for large-scale resource analysis

## PubSub Integration

The Wilderness System integrates with the PubSub (Publish/Subscribe) messaging system to provide real-time event-driven communication and spatial audio/visual effects.

### System Overview

The PubSub system provides:
- **Topic-Based Messaging**: Players can subscribe to various wilderness topics
- **Spatial Audio**: 3D positional audio for wilderness events
- **Event Broadcasting**: Real-time distribution of wilderness events
- **Message Queuing**: Reliable delivery with queue management
- **Multiple Handlers**: Different message processing strategies

### Wilderness Integration

**Spatial Audio Broadcasting:**
```c
// Publish spatial audio to wilderness area
pubsub_publish_wilderness_audio(source_x, source_y, source_z,
                               sender_name, content, 
                               max_distance, priority);
```

**Coordinate-Based Delivery:**
- Messages delivered based on 3D distance calculations
- Elevation affects audio transmission (Z-axis positioning)
- Range-based filtering ensures appropriate audience
- Terrain and weather can modify transmission

### Player Commands

**Basic PubSub Commands:**
```
pubsub status                - Show system status
pubsub list                  - List available topics  
pubsub create <name> <desc>  - Create a new topic
pubsub send <topic> <msg>    - Send message to topic
pubsub subscribe <topic>     - Subscribe to a topic
pubsub unsubscribe <topic>   - Unsubscribe from topic
```

**Wilderness-Specific Features:**
```
pubsub spatial              - Test spatial systems (wilderness only, admin)
pubsubqueue spatial         - Test wilderness spatial audio
```

### Message Handlers

The system includes specialized handlers for wilderness:

1. **spatial_audio**: Basic spatial audio processing with distance
2. **wilderness_spatial**: Enhanced 3D spatial audio for wilderness
3. **audio_mixing**: Multiple simultaneous audio source mixing
4. **send_text**: Plain text message delivery
5. **send_formatted**: Formatted message with color codes

### Technical Implementation

**Core Integration Points:**
- `pubsub_publish_wilderness_audio()` - Main wilderness audio function
- Distance calculation using `sqrt(pow(x_diff, 2) + pow(y_diff, 2) + pow(z_diff/4, 2))`
- Range checking with configurable maximum distances
- Player filtering based on wilderness zone flags

**Database Tables:**
- `pubsub_topics` - Topic definitions and metadata
- `pubsub_subscriptions` - Player subscription tracking
- `pubsub_messages` - Message history and delivery tracking

**Event Processing:**
- Automatic queue processing every 3 pulses (~0.75 seconds)
- Message TTL (Time To Live) management
- Delivery attempt tracking and retry logic
- Statistics collection for performance monitoring

## Spatial Audio System

The Spatial Audio System provides 3D positional audio effects specifically designed for wilderness environments.

### Core Features

**3D Positioning:**
- X, Y coordinate-based horizontal positioning
- Z-axis (elevation) affects audio transmission
- Distance-based volume and clarity calculations
- Direction-based audio panning (future enhancement)

**Environmental Factors:**
- Terrain type affects audio transmission
- Weather conditions modify sound propagation
- Elevation differences create realistic audio shadows
- Maximum transmission distances based on content type

### Integration with Wilderness

**Coordinate Integration:**
```c
// Get player wilderness coordinates
int target_x = X_LOC(target);
int target_y = Y_LOC(target);
int target_z = get_modified_elevation(X_LOC(target), Y_LOC(target));
```

**Distance Calculation:**
```c
// 3D distance with elevation weighting
float distance = sqrt(pow(X_LOC(target) - source_x, 2) + 
                     pow(Y_LOC(target) - source_y, 2) +
                     pow((target_z - source_z) / 4.0, 2));
```

**Range-Based Delivery:**
- Audio events delivered only to players within range
- Configurable maximum distances (typically 10-50 wilderness units)
- Automatic filtering based on zone wilderness flags

### Audio Types and Uses

**Environmental Audio:**
- Thunder and weather effects
- Wildlife sounds and movement
- Combat and spell effects
- Player actions and movement

**Communication Audio:**
- Distant shouts and calls
- Horn and signal sounds
- Musical instruments
- Emergency alerts

### Administrative Tools

**Testing Commands:**
```
pubsub spatial              # Test both visual and audio systems
pubsubqueue spatial         # Test wilderness spatial audio specifically
```

**Performance Monitoring:**
```
pubsub stats               # System-wide statistics
pubsubqueue status         # Queue processing statistics
```

### Technical Architecture

**Core Components:**
- `spatial_core.c/h` - Core spatial calculation engine
- `spatial_audio.c/h` - Audio-specific processing
- `spatial_visual.c/h` - Visual event processing  
- `systems/spatial/` - Modular spatial subsystems

**Integration Points:**
- PubSub message queue for reliable delivery
- Wilderness coordinate system for positioning
- Character filtering based on online status and location
- Dynamic range calculation based on content and environment

**Performance Optimizations:**
- Efficient distance calculations
- Player filtering before expensive operations
- Queue-based processing to avoid lag spikes
- Configurable processing intervals

## Player Experience

### Movement and Navigation
- **Movement**: Standard directions (±1 coordinate), auto room creation
- **Map**: 21x21 automap, ASCII-only symbols (2025 fix for alignment), line-of-sight, weather overlay
- **Navigation**: Coordinate display, static landmarks, path guidance
- **Resource Discovery**: Visual resource mapping and detailed surveys

### Enhanced Exploration Features

**Resource Discovery:**
- `survey resources` - Comprehensive resource analysis at current location
- `survey map <resource> [radius]` - Visual ASCII maps showing resource density
- `survey terrain` - Environmental factor analysis
- Real-time resource percentage calculations based on coordinates

**Spatial Audio Experience:**
- 3D positional audio for wilderness events
- Distance-based volume and clarity
- Environmental audio (thunder, wildlife, weather)
- Player-generated spatial audio events

**Communication Systems:**
- PubSub topic subscription for real-time wilderness events
- Spatial audio communication with realistic range limitations
- Event-driven notifications for wilderness activities
- Personal message delivery with location awareness

### Advanced Features

**Conservation and Ecology:**
- `survey conservation` - Resource depletion tracking
- `survey ecosystem` - Ecosystem health monitoring
- `survey impact` - Personal environmental impact assessment
- `survey cascade <resource>` - Ecological impact preview

**Interactive Mapping:**
- Resource density visualization with color-coded symbols
- Customizable map radius (3-15 units)
- Terrain-based resource distribution patterns
- Real-time coordinate tracking and display

**Administrative Tools (Immortal+):**
- `resourceadmin` suite for resource system management
- `pubsub admin` commands for system administration
- Spatial audio testing and debugging tools
- Performance monitoring and statistics

## Builder Tools

### OLC Integration

**Wilderness Room Editing:**
```c
// Coordinates automatically set in OLC
OLC_ROOM(d)->coords[0] = x_coordinate;
OLC_ROOM(d)->coords[1] = y_coordinate;
```

**Buildwalk Support:** `PRF_BUILDWALK` flag, auto coordinates, permission checks, OLC integration

**Commands:** `buildwalk` (toggle), `buildwalk reset`, `buildwalk sector/name/desc <value>`

### Attaching Zones to Wilderness

**Process Overview:**
1. Create zone with `ZONE_WILDERNESS` flag
2. Build rooms with coordinate assignments
3. Set up exits to wilderness navigation room (vnum 1000000)
4. Update external documentation

**Key Requirements:**
- Zone must be flagged as `ZONE_WILDERNESS`
- Rooms must have valid coordinates set
- Exits must point to navigation room for wilderness movement
- Static rooms should be in VNUM range 1000000-1003999

#### Builder's Quick Guide

> **Note:** This system is primitive but functional for current wilderness attachment needs.

**Step 1: Access Trello Resources**
- **Builder's Board:** [https://trello.com/b/xOjCl0hC/luminari-builders](https://trello.com/b/xOjCl0hC/luminari-builders)
- **Wilderness Map Card:** [https://trello.com/c/5sbBrktg](https://trello.com/c/5sbBrktg)
  - Open the most recent date folder for the up-to-date map
  - Download either `.pdn` (Paint.NET source) or `.png` (web-friendly image)

**Step 2: Get Pixel Coordinates**
- Open the `.png` in a basic image editor (e.g., Paint)
- Bottom-left corner shows pixel location
- Pick your desired location and **record its pixel coordinates**

**Step 3: Convert Coordinates (Optional)**
- **Main Zone Document:** [https://trello.com/c/cGNoX1Ea](https://trello.com/c/cGNoX1Ea)
  - Use the coordinate conversion cells to go from pixel location to wilderness coordinates
  - Or use wilderness coordinates directly if you already know them

**Step 4: In-Game Attachment Process**
1. Find the **entrance room** to your zone
2. Move **one space** in any cardinal direction
3. Turn **Buildwalk ON**
4. Move **back** to the original room (this "paints" a single room over the wilderness)
5. Turn **Buildwalk OFF**
6. Edit that created room:
   - Connect it to your zone
   - Ensure there is a path back to the wilderness

**Step 5: Update Documentation**
- Add your zone to the Wilderness Map (images on Trello): [https://trello.com/c/5sbBrktg](https://trello.com/c/5sbBrktg)
- Add your zone to the **Main Zone Document**: [https://trello.com/c/cGNoX1Ea](https://trello.com/c/cGNoX1Ea)

**Step 6: Announce In-Game**
Type in-game:
```
Change <insert your message about your zone here>
```

#### Technical Implementation Steps

**Detailed Technical Process:**
1. **Zone Creation**: Use `zedit` to create new zone with wilderness flag
2. **Room Building**: Use `redit` to create rooms with proper coordinates
3. **Exit Setup**: Create exits pointing to room 1000000 for wilderness movement
4. **Testing**: Use buildwalk to test movement and coordinate assignment
5. **Documentation**: Update builder documentation with new zone information

### River Generation

**Command:** `genriver <direction> <vnum> <name>`

```c
ACMD(do_genriver) {
    char arg1[MAX_STRING_LENGTH], arg2[MAX_STRING_LENGTH];
    const char *name;
    int dir;
    region_vnum vnum;

    name = two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

    // Parse direction (north, south, east, west or numeric)
    if (is_abbrev(arg1, "north")) dir = NORTH;
    else if (is_abbrev(arg1, "east")) dir = EAST;
    // ... other directions

    vnum = atoi(arg2);

    // Generate river path following terrain
    generate_river(ch, dir, vnum, name);
    load_paths(); // Reload path data
}
```

**River Generation Algorithm:**
```c
void generate_river(struct char_data *ch, int dir, region_vnum vnum, const char *name) {
    int x = X_LOC(ch), y = Y_LOC(ch);
    struct vertex vertices[1024];
    int num_vertices = 0;
    int move_dir = dir;

    // Generate meandering path based on elevation
    int i;
    for (i = 0; i < 500; i++) { // Max 500 segments
        vertices[num_vertices].x = x;
        vertices[num_vertices].y = y;
        num_vertices++;

        // Choose next direction based on elevation gradient
        int best_dir = find_downhill_direction(x, y, move_dir);

        // Add some randomness for natural meandering
        if (rand() % 4 == 0) {
            best_dir = (best_dir + (rand() % 3) - 1 + 4) % 4;
        }

        // Move to next position
        switch (best_dir) {
            case NORTH: y++; break;
            case SOUTH: y--; break;
            case EAST:  x++; break;
            case WEST:  x--; break;
        }

        // Stop if we hit water or go too far
        int elevation = get_elevation(NOISE_MATERIAL_PLANE_ELEV, x, y);
        if (elevation < wild_waterline) break;
    }

    // Create path_data structure and insert into database
    struct path_data river;
    river.vnum = vnum;
    river.zone = world[IN_ROOM(ch)].zone;
    river.name = STRALLOC(name);
    river.path_type = PATH_RIVER;
    river.path_props = SECT_RIVER;
    river.num_vertices = num_vertices;
    river.vertices = vertices;

    insert_path(&river);
}
```

**Features:**
- Follows natural terrain flow using elevation gradients
- Automatic meandering based on elevation and randomness
- Database integration with MySQL spatial data
- Visual map representation with river glyphs
- Stops when reaching water bodies
- Maximum length limit to prevent infinite rivers

**Usage Examples:**
```
genriver north 100011 "Silverflow River"
genriver south 100012 "Muddy Creek"
genriver east 100013 "Crystal Stream"
```

**Requirements:**
- Must be used in wilderness areas only
- Requires unique vnum for each river
- Name is required and stored in database
- Automatically reloads path data after creation

### Region and Path Management

**Database Tools:**
- Direct MySQL manipulation for complex regions
- Spatial geometry support
- Polygon and linestring definitions
- Automatic indexing for performance

**Region Creation Process:**
1. Define polygon boundaries in MySQL
2. Set region type and properties
3. Associate with appropriate zone
4. Test in-game functionality

**Path Creation Process:**
1. Define linestring route in MySQL
2. Set path type and sector override
3. Configure display glyphs
4. Test visual representation

## Technical Implementation

### World Loading Integration

The wilderness system is integrated into the world loading process during server startup:

```c
void boot_world(void) {
    // 1. Database connection
    connect_to_mysql();

    // 2. Core world structures
    log("Loading zone table.");
    index_boot(DB_BOOT_ZON);        // Zone definitions

    log("Loading rooms.");
    index_boot(DB_BOOT_WLD);        // Room data (includes wilderness rooms)

    // 3. Geographic systems
    log("Loading regions. (MySQL)");
    load_regions();                  // Geographic regions for wilderness

    log("Loading paths. (MySQL)");
    load_paths();                    // Path data for wilderness

    // 4. Initialize wilderness systems
    initialize_wilderness_lists();   // KD-Tree indexing for static rooms
    
    // 5. Initialize enhanced systems
    log("Initializing PubSub system...");
    pubsub_init();                   // Event-driven messaging system
    
    log("Initializing Resource System...");
    init_wilderness_resource_tables(); // Resource system database
    
    log("Initializing Spatial Systems...");
    spatial_init();                  // 3D spatial audio/visual systems
}
```

**Loading Sequence:**
1. **Zone Data**: Loads zone definitions including `ZONE_WILDERNESS` flags
2. **Room Data**: Loads static wilderness rooms with coordinate assignments
3. **Region Data**: Loads polygonal regions from MySQL database
4. **Path Data**: Loads linear paths (roads, rivers) from MySQL database
5. **Wilderness Initialization**: Builds KD-Tree indexes for performance
6. **PubSub Initialization**: Sets up event-driven messaging and handlers
7. **Resource System**: Initializes resource tables and caching
8. **Spatial Systems**: Configures 3D audio/visual positioning systems

### Dynamic Content Generation

The wilderness system supports various forms of dynamic content generation:

**Room Generation:**
- Wilderness rooms created on-demand as players explore
- Procedural terrain and descriptions based on coordinate algorithms
- Dynamic exit creation for seamless exploration
- Automatic sector type assignment using noise functions
- Resource levels calculated in real-time based on coordinates

**Resource Generation:**
- Dynamic resource distribution using terrain-based algorithms
- Coordinate-dependent resource calculations for consistency
- Depletion tracking with database persistence
- Regeneration patterns based on ecological factors
- Real-time resource mapping and visualization

**Event Generation:**
- PubSub event distribution based on player locations
- Spatial audio events with 3D positioning
- Weather-based environmental audio
- Player action broadcasts with range limitations
- Automated ecological event notifications

**Object Spawning:**
- Random treasure generation based on terrain type
- Harvestable resource respawning in appropriate biomes
- Dynamic item creation based on environmental conditions
- Region-specific loot tables and spawn rates

**NPC Spawning:**
- Random encounters in wilderness based on terrain and regions
- Dynamic population based on time of day and weather conditions
- Seasonal or event-based spawning patterns
- Encounter tables specific to different wilderness areas

**Environmental Features:**
- Weather patterns that vary by location and terrain
- Seasonal changes affecting terrain appearance and properties
- Dynamic water levels and river flow patterns
- Time-based lighting and visibility changes
- Spatial audio events for environmental immersion

### Performance Optimizations

**KD-Tree:** O(log n) static room lookup
**Room Pool:** 6000 dynamic rooms, auto-recycling, `ROOM_OCCUPIED` flag
**Lazy Loading:** On-demand creation, spatial DB indexes

### Memory Management

#### Critical Memory Safety Pattern

The wilderness system implements a sophisticated memory management pattern that prevents both memory leaks and double-free crashes. This pattern was developed after resolving critical memory issues in the `goto` wilderness navigation system.

**Core Memory Safety Principles:**

1. **Static String Defaults**: Room names and descriptions start as pointers to static strings
2. **Dynamic Overrides**: Regions and paths can replace these with dynamically allocated strings
3. **Safe Free Checks**: Always verify pointer != static_string before calling `free()`
4. **Leak Prevention**: Previous dynamic strings are freed before replacement
5. **Crash Prevention**: Static strings are never freed
6. **Recycling Safety**: Room cleanup doesn't free strings (handled on reuse)

**Implementation Pattern:**

```c
/* Static string constants - NEVER free these */
static char wilderness_name[] = "The Wilderness of Luminari";
static char wilderness_desc[] = "The wilderness extends in all directions.";

/* Safe memory management in assign_wilderness_room() */
void assign_wilderness_room(room_rnum room, int x, int y) {
    /* Step 1: Safe cleanup of existing strings */
    if (world[room].name && world[room].name != wilderness_name)
        free(world[room].name);
    if (world[room].description && world[room].description != wilderness_desc)
        free(world[room].description);
    
    /* Step 2: Assign static defaults */
    world[room].name = wilderness_name;
    world[room].description = wilderness_desc;
    
    /* Step 3: Dynamic overrides (regions/paths) */
    if (region_overrides_name) {
        if (world[room].name && world[room].name != wilderness_name)
            free(world[room].name);  // Safe: only frees dynamic strings
        world[room].name = strdup(region_name);  // New allocation
    }
}
```

**Memory Lifecycle:**

1. **Room Creation**: Pointers set to static strings (no allocation)
2. **Region Override**: Static pointer replaced with `strdup()` allocation
3. **Path Override**: Previous allocation freed, new `strdup()` allocation
4. **Room Recycling**: Strings remain (cleaned up on next assignment)
5. **Room Reuse**: Safe cleanup pattern repeated

**Critical Safety Checks:**

```c
/* CORRECT - Safe pattern used throughout wilderness system */
if (world[room].name && world[room].name != wilderness_name)
    free(world[room].name);

/* INCORRECT - Would crash when freeing static strings */
if (world[room].name)
    free(world[room].name);  // CRASH: Cannot free static memory
```

**Historical Context:**

The wilderness system previously crashed during `goto` coordinate navigation due to attempts to free static strings. Memory audit fixes implemented this safety pattern to prevent such crashes while maintaining proper cleanup of dynamic allocations.

**Static Data Management:**
- Region and path data loaded at startup
- KD-Tree rebuilt when wilderness zones change
- Persistent coordinate storage
- Static string constants never freed

**Dynamic Data Management:**
- Room strings managed with safe allocation/deallocation pattern
- Temporary room assignments with automatic recycling
- Dynamic room pool with occupation flag management

**Memory Pool Allocation:**
```c
// Dynamic room pool management
for (i = WILD_DYNAMIC_ROOM_VNUM_START; i <= WILD_DYNAMIC_ROOM_VNUM_END; i++) {
    if (real_room(i) != NOWHERE && !ROOM_FLAGGED(real_room(i), ROOM_OCCUPIED)) {
        SET_BIT_AR(ROOM_FLAGS(real_room(i)), ROOM_OCCUPIED);
        return real_room(i);  // Room allocated and marked occupied
    }
}
```

**Memory Audit Guidelines:**

For future memory leak audits, follow these guidelines to avoid breaking the wilderness system:

1. **Never modify** the static string pointer checks
2. **Always preserve** the pattern: `if (ptr && ptr != static_string) free(ptr)`
3. **Understand** that room recycling intentionally doesn't free strings
4. **Recognize** that static pointers are not memory leaks
5. **Test thoroughly** any memory management changes with wilderness navigation

**Debug Memory Issues:**

```c
// Memory debugging for wilderness rooms
void debug_wilderness_memory(room_rnum room) {
    log("DEBUG: Room %d name ptr: %p (static: %p)", 
        room, world[room].name, wilderness_name);
    log("DEBUG: Room %d desc ptr: %p (static: %p)", 
        room, world[room].description, wilderness_desc);
    log("DEBUG: Name is %s: '%s'", 
        (world[room].name == wilderness_name) ? "STATIC" : "DYNAMIC",
        world[room].name ? world[room].name : "NULL");
}
```

This memory management pattern ensures system stability while maintaining performance and preventing resource leaks.

### Database Schema

**Core Wilderness Tables:**
- `region_data`, `region_index` - Polygonal regions with spatial geometry
- `path_data`, `path_index`, `path_types` - Linear paths and roads with spatial data

**Resource System Tables:**
- `resource_types` - Resource type definitions and properties
- `resource_depletion` - Location-based resource depletion tracking
- Resource cache tables for performance optimization

**PubSub System Tables:**
- `pubsub_topics` - Topic definitions and metadata
- `pubsub_subscriptions` - Player subscription tracking
- `pubsub_messages` - Message history and delivery tracking

**Key Features:**
- GEOMETRY columns with SPATIAL INDEX for efficient queries
- ST_Within() spatial queries for region/path detection
- Glyph definitions for visual map representation
- Foreign key relationships for data integrity
- Optimized indexes for coordinate-based lookups

### Error Handling

**Validation:** Coordinate bounds checking, room allocation fallback, MySQL error logging

## Configuration

### Wilderness Constants

```c
#define WILD_X_SIZE 2048                    // Wilderness width
#define WILD_Y_SIZE 2048                    // Wilderness height
#define WILD_ZONE_VNUM 10000               // Default wilderness zone

// Coordinate system
#define X_COORD 0                          // X coordinate array index
#define Y_COORD 1                          // Y coordinate array index

// Room VNUM ranges
#define WILD_ROOM_VNUM_START 1000000       // Static wilderness rooms start
#define WILD_ROOM_VNUM_END 1003999         // Static wilderness rooms end
#define WILD_DYNAMIC_ROOM_VNUM_START 1004000 // Dynamic room pool start
#define WILD_DYNAMIC_ROOM_VNUM_END 1009999   // Dynamic room pool end

// Terrain thresholds
#define WATERLINE 138                      // Used in temperature calculation only
int wild_waterline = 128;                  // Runtime waterline for actual terrain generation
#define SHALLOW_WATER_THRESHOLD 20         // Shallow vs deep water
#define COASTLINE_THRESHOLD 5              // Beach/coastline threshold
#define PLAINS_THRESHOLD 35                // Plains elevation threshold
#define HIGH_MOUNTAIN_THRESHOLD 55         // High mountain threshold (elevation > 255 - 55 = 200)
#define MOUNTAIN_THRESHOLD 70              // Mountain threshold (elevation > 255 - 70 = 185)
#define HILL_THRESHOLD 80                  // Hill threshold (elevation > 255 - 80 = 175)

// Map display
#define MAP_TYPE_NORMAL 0                  // Normal terrain map
#define MAP_TYPE_WEATHER 1                 // Weather overlay map
#define WILD_MAP_SHAPE_CIRCLE 1            // Circular map shape
#define WILD_MAP_SHAPE_RECT 2              // Rectangular map shape
#define NUM_VARIANT_GLYPHS 4               // Number of terrain variants

// Utility macro
#define IS_WILDERNESS_VNUM(room_vnum) \
    ((room_vnum >= WILD_ROOM_VNUM_START && room_vnum <= WILD_ROOM_VNUM_END) || \
     (room_vnum >= WILD_DYNAMIC_ROOM_VNUM_START && room_vnum <= WILD_DYNAMIC_ROOM_VNUM_END))
```

### Noise Seeds and Indices

```c
// Noise layer indices
#define NOISE_MATERIAL_PLANE_ELEV 0        // Elevation noise layer
#define NOISE_MATERIAL_PLANE_ELEV_DIST 1   // Elevation distortion layer
#define NOISE_MATERIAL_PLANE_MOISTURE 2    // Moisture noise layer
#define NOISE_WEATHER 3                    // Weather noise layer
#define NUM_NOISE 4                        // Total number of noise layers

// Noise seeds (must be < MAX_GENERATED_NOISE = 24)
#define NOISE_MATERIAL_PLANE_ELEV_SEED 822344     // Elevation seed
#define NOISE_MATERIAL_PLANE_MOISTURE_SEED 834    // Moisture seed
#define NOISE_MATERIAL_PLANE_ELEV_DIST_SEED 74233 // Elevation distortion seed
#define NOISE_WEATHER_SEED 43425                  // Weather seed
```

**Customization:**
- Change seeds to generate different terrain patterns
- Modify size constants for larger/smaller wilderness
- Adjust noise parameters for different terrain characteristics

### Resource System Configuration

```c
// Resource types and counts
#define NUM_RESOURCE_TYPES 10         // Total number of resource types

// Resource density ranges
#define RESOURCE_DENSITY_MIN 0.0      // Minimum resource density (0%)
#define RESOURCE_DENSITY_MAX 1.0      // Maximum resource density (100%)

// Map visualization
#define DEFAULT_RESOURCE_MAP_RADIUS 7  // Default map radius
#define MAX_RESOURCE_MAP_RADIUS 15     // Maximum allowed map radius
#define MIN_RESOURCE_MAP_RADIUS 3      // Minimum allowed map radius
```

### PubSub System Configuration

```c
// System versioning and features
#define PUBSUB_VERSION 3               // Current PubSub system version
#define PUBSUB_DEVELOPMENT_MODE 0      // Enable development mode features

// Message and queue limits
#define PUBSUB_DEFAULT_MESSAGE_TTL 3600    // Default message TTL (1 hour)
#define PUBSUB_QUEUE_BATCH_SIZE 10         // Messages processed per batch
#define SUBSCRIPTION_CACHE_SIZE 256        // Player subscription cache size

// Topic and handler limits
#define PUBSUB_MAX_TOPIC_NAME_LENGTH 64    // Maximum topic name length
#define PUBSUB_MAX_HANDLER_NAME_LENGTH 32  // Maximum handler name length

// Spatial audio defaults
#define PUBSUB_DEFAULT_SPATIAL_RANGE 25    // Default spatial audio range
#define PUBSUB_PRIORITY_NORMAL 5           // Normal message priority
```

### Spatial System Configuration

```c
// Distance calculation parameters
#define SPATIAL_ELEVATION_WEIGHT 4.0       // Z-axis distance weighting factor
#define SPATIAL_MAX_TRANSMISSION_RANGE 50   // Maximum audio transmission range

// Processing intervals
#define SPATIAL_PROCESSING_INTERVAL 3       // Process every N pulses
#define SPATIAL_CACHE_CLEANUP_INTERVAL 100  // Cache cleanup interval

// System limits
#define MAX_SPATIAL_SYSTEMS 16              // Maximum registered systems
#define SPATIAL_MAX_SIMULTANEOUS_EVENTS 32  // Maximum concurrent events
```

### Database Configuration

**MySQL Requirements:** 5.7+, spatial extensions (GEOMETRY, ST_Within, GeomFromText), SPATIAL INDEX support

### Perlin Noise Configuration

**Parameters:** Frequency(2.0), Lacunarity(2.0), Octaves(16)
**Techniques:** Ridged multifractal, attenuation for sharp peaks

## Troubleshooting

### Core Wilderness Issues

**Movement Fails:** Check `ZONE_WILDERNESS` flag, exits to room 1000000, coordinates set

**No Terrain Variation:** Verify noise seeds, test with `get_elevation()`, `get_moisture()`, `get_temperature()`

**Regions/Paths Missing:** Check MySQL spatial data, test `ST_Within()` queries

**Performance:** Monitor room pool usage (<80%), rebuild KD-Tree if slow

**Map Misalignment:** Fixed 2025 - ASCII-only symbols now (Y,^,~,=)

**Memory Crashes:** Use pattern `if (ptr && ptr != static_string) free(ptr)` - never free static strings

### Resource System Issues

**Resources Always 0%:** 
- Check if you're in wilderness zone (`ZONE_WILDERNESS` flag)
- Verify resource system initialization in server startup
- Test with `resourceadmin debug` for detailed diagnostics

**Resource Maps Not Displaying:**
- Ensure client supports ANSI color codes
- Check map radius is within bounds (3-15)
- Verify resource type name spelling (use exact names)

**Resource Commands Fail:**
- Command: `"Resource maps can only be viewed in the wilderness"`
- Solution: Navigate to wilderness zone using `goto` or walking
- Verify with `whereis` command

### PubSub System Issues

**PubSub Commands Not Working:**
- Check if PubSub system initialized successfully during startup
- Verify MySQL database connectivity
- Test with `pubsub status` to check system state

**Spatial Audio Not Working:**
- Must be in wilderness zone for spatial audio tests
- Check if other players are within range for testing
- Verify PubSub message queue is processing (`pubsubqueue status`)

**Subscription Issues:**
- Check subscription limits and topic availability
- Verify handler names are correct (case-sensitive)
- Test basic functionality with `pubsub list` and `pubsub info <topic>`

### Spatial System Issues

**No Spatial Audio Effects:**
- Ensure both source and receiver are in wilderness
- Check distance calculations - audio has maximum range limits
- Verify spatial system initialization in startup logs

**Audio Distance Problems:**
- Audio uses 3D distance calculation including elevation
- Elevation differences affect transmission (Z-axis weighted by 4.0)
- Check if wilderness coordinates are properly set for rooms

### Performance Issues

**System Lag with New Features:**
- Monitor PubSub queue processing intervals (default: every 3 pulses)
- Check resource system cache performance
- Use `pubsub stats` and `resourceadmin cache` for diagnostics

**Database Performance:**
- Ensure spatial indexes are properly created
- Monitor MySQL query performance for resource and PubSub queries
- Check database connection stability

**Memory Usage:**
- Resource system uses coordinate-based caching
- PubSub maintains message queues and subscription caches
- Monitor memory usage with administrative commands

### Debug Commands

**Core Wilderness:**
- `stat room` - Room information and coordinates
- `goto 1000000` - Navigate to wilderness navigation room
- `genriver <dir> <vnum> <name>` - Generate rivers with terrain flow
- `genmap` - Generate terrain maps

**Resource System:**
- `survey debug` - Comprehensive resource system debug info
- `resourceadmin status` - System status and statistics
- `resourceadmin debug` - Advanced debug information
- `resourceadmin cache` - Cache statistics and management

**PubSub System:**
- `pubsub status` - System status and configuration
- `pubsub stats` - Performance statistics and metrics
- `pubsub admin status` - Administrative system information
- `pubsubqueue status` - Message queue diagnostics

**Spatial Systems:**
- `pubsub spatial` - Test spatial audio and visual systems
- `pubsubqueue spatial` - Test wilderness spatial audio
- Spatial system debug logging in server logs

**Build Tools:**
- `buildwalk` (toggle/reset/sector/name/desc)
- Resource system integration with OLC
- PubSub topic creation and management

### Maintenance

**Monitor:** Room pool <80%, KD-Tree <1ms, DB queries <100ms
**Tasks:** Rebuild KD-Tree, clean orphaned rooms, optimize DB indexes

---

## System Integration

### Core Game Systems

**Combat:** Terrain modifiers, cover/concealment, environmental hazards
**Quests:** Static destinations, region triggers, exploration rewards, resource-based objectives
**Scripts:** DG triggers, region events, weather scripts, PubSub event handlers
**Players:** Movement, coordinate tracking, automap, resource discovery, spatial audio
**Events:** Timed events, seasonal changes, dynamic spawning, ecological notifications

### Enhanced Integrations

**Resource Integration:**
- Terrain-based resource distribution affects gameplay mechanics
- Resource depletion influences long-term area development
- Conservation systems impact player environmental scores
- Ecological cascades create realistic resource interdependencies

**Communication Integration:**
- PubSub topics for real-time wilderness event notifications
- Spatial audio for immersive environmental experiences
- Event-driven messaging for player coordination
- Distance-based communication with realistic limitations

**Database Integration:**
- MySQL spatial queries for region and path detection
- Resource tracking with persistent depletion data
- PubSub message history and subscription management
- Indexed lookups for performance optimization
- Coordinate-based caching for efficient resource calculations

### Advanced Features

**Event-Driven Architecture:**
- PubSub system enables real-time wilderness event distribution
- Spatial events with 3D positioning and range-based delivery
- Automated ecological notifications and conservation alerts
- Player action broadcasting with environmental context

**Performance Monitoring:**
- Resource system cache statistics and optimization
- PubSub queue processing and message delivery metrics
- Spatial system performance tracking and diagnostics
- Database query optimization and index maintenance

---

## Advanced Features

### Dynamic Content Systems

**Terrain-Based Features:**
- Resource distribution algorithms based on elevation, moisture, and temperature
- Terrain-dependent encounter tables and spawn rates
- Weather effects that influence resource availability and audio transmission
- Elevation-based audio transmission calculations with realistic sound propagation

**Event-Driven Features:**
- Real-time PubSub event distribution for wilderness activities
- Spatial audio events with 3D positioning and environmental factors
- Automated ecological notifications and conservation alerts
- Player action broadcasting with coordinate-based filtering

**Interactive Features:**
- Resource mapping with customizable visualization radius
- Conservation tracking with ecological impact previews
- Spatial communication with distance-based limitations
- Real-time environmental monitoring and feedback

### Performance Optimizations

**Core Systems:**
- Room pool recycling at 90% capacity with dynamic allocation
- KD-Tree O(log n) lookups for static room positioning
- Spatial database indexes for efficient region/path queries
- Coordinate-based resource caching for consistent calculations

**Enhanced Systems:**
- PubSub message queue processing with configurable batch sizes
- Resource system caching with automatic cache cleanup
- Spatial system optimization with distance-based filtering
- Database query optimization with prepared statements and indexes

### Customization Options

**Terrain Customization:**
- Add new sector types via `wild_map_info[]` configuration
- Custom region scripts and special area behaviors
- Configurable path types with visual glyph definitions
- Noise seed modification for different terrain patterns

**System Customization:**
- Resource type definitions with custom distribution algorithms
- PubSub handler registration for specialized message processing
- Spatial system configuration with custom distance calculations
- Database schema extensions for additional functionality

## Resources

### Core Source Files

**Wilderness Engine:**
- `wilderness.c/h` - Main wilderness system and coordinate management
- `perlin.c/h` - Noise generation algorithms for terrain
- `mysql.c/h` - Database integration and spatial queries
- `act.movement.c` - Movement handling and room transitions
- `act.wizard.c` - Administrative commands and debugging tools

**Resource System:**
- `resource_system.c/h` - Core resource management and calculations
- `resource_descriptions.c/h` - Resource discovery and mapping functionality
- `resource_*.c/h` - Specialized resource subsystems

**PubSub System:**
- `pubsub.c/h` - Core PubSub messaging and topic management
- `systems/pubsub/pubsub_commands.c` - Player command interface
- `systems/pubsub/pubsub_handlers.c` - Message handler implementations
- `systems/pubsub/pubsub_spatial.c` - Spatial audio integration
- `systems/pubsub/pubsub_*.c` - Additional subsystem components

**Spatial Systems:**
- `spatial_core.c/h` - 3D positioning and distance calculations
- `spatial_audio.c/h` - Audio-specific processing and transmission
- `spatial_visual.c/h` - Visual event processing and delivery
- `systems/spatial/` - Modular spatial subsystem components

### Database Tables

**Core Wilderness:**
- `region_data`, `region_index` - Polygonal regions with spatial geometry
- `path_data`, `path_index`, `path_types` - Linear features and roads
- All tables include spatial indexes for performance

**Resource Management:**
- `resource_types` - Resource type definitions and properties
- `resource_depletion` - Location-based depletion tracking
- Resource cache tables for performance optimization

**Communication Systems:**
- `pubsub_topics` - Topic definitions and metadata
- `pubsub_subscriptions` - Player subscription tracking  
- `pubsub_messages` - Message history and delivery tracking

### Documentation References

**System Guides:**
- `RESOURCE_SYSTEM_REFERENCE.md` - Complete resource system documentation
- `WILDERNESS_BUILDER_GUIDE.md` - Builder tools and procedures
- `PUBSUB_API_REFERENCE.md` - PubSub system API documentation

**Technical Documentation:**
- Database schema documentation for spatial features
- Performance tuning guides for large-scale deployments
- Integration examples for custom system development

---

## Region Reload System Architecture

### Overview
The `reloadimm regions` command reloads wilderness regions from the MySQL database without requiring a server restart. This involves multiple interconnected systems that must be carefully coordinated to prevent memory corruption and crashes.

### Systems Involved

#### 1. MUD Event System (mud_event.c/h)
- **new_mud_event()** - Creates event data structures with strdup'd variables
- **attach_mud_event()** - Attaches events to the global event queue
- **free_mud_event()** - Frees events and removes from associated lists
- **clear_region_event_list()** - Safely cancels all events for a region
- **EVENT_REGION** type events for encounter resets

#### 2. DG Event Queue System (dg_event.c/h)
- **event_create()** - Creates events and enqueues them
- **event_cancel()** - Dequeues and frees events (calls cleanup_event_obj)
- **queue_enq() / queue_deq()** - Queue management functions
- **event_q** - The global event queue (static queue pointer)
- **cleanup_event_obj()** - Calls free_mud_event for mud events

#### 3. Custom List System (lists.c/h)
- **create_list()** - Creates linked lists for event management
- **add_to_list()** - Adds items to lists
- **remove_from_list()** - Removes items from lists  
- **free_list()** - Frees entire lists
- **simple_list()** - Safe iterator for lists
- Each region has an `events` list field

#### 4. MySQL Database System (mysql.c)
- **load_regions()** - Main function that loads region data
- Queries `region_data` table for polygon geometry
- Handles encounter region reset data and timers
- Creates encounter reset events for regions

#### 5. Region/Wilderness System (wilderness.h, db.c)
- **region_table** - Global array of struct region_data
- **struct region_data** - Contains events list, reset_data, reset_time
- **real_region()** - Converts vnum to rnum (returns NOWHERE if not found)
- **REGION_ENCOUNTER** type regions with reset functionality

#### 6. Memory Management
- **CREATE macro** - Wrapper around calloc for zeroed allocation
- **strdup()** - String duplication for dynamic strings
- Manual **free()** calls for cleanup sequences
- Critical initialization of NULL pointers

### Critical Issues and Fixes

#### Issue 1: Uninitialized Events Field
**Problem:** The `events` field in struct region_data was not explicitly initialized to NULL after CREATE allocation, leading to heap corruption.

**Fix:** Explicitly initialize all pointer fields:
```c
region_table[i].events = NULL;  /* CRITICAL: Initialize events list to NULL */
```

#### Issue 2: Double Free in Event Cleanup
**Problem:** The original clear_region_event_list used a temp_list approach that caused double-free errors because event_cancel removes events from the region's list.

**Fix:** Process events directly from the region's list one at a time:
```c
while (reg->events && reg->events->iSize > 0) {
    pEvent = (struct event *)reg->events->pFirstItem->pContent;
    if (pEvent && event_is_queued(pEvent))
        event_cancel(pEvent);  /* This removes event from reg->events */
}
```

#### Issue 3: NOWHERE Check in free_mud_event
**Problem:** During reload, real_region() may return NOWHERE (-1) causing buffer underflow when accessing region_table[-1].

**Fix:** Check for NOWHERE before accessing region_table:
```c
region_rnum rnum = real_region(*regvnum);
if (rnum != NOWHERE) {
    region = &region_table[rnum];
    remove_from_list(pMudEvent->pEvent, region->events);
}
```

### Event Lifecycle During Reload

1. **Clear Phase:** All existing region events are cancelled
   - clear_region_event_list() called for each region
   - event_cancel() → cleanup_event_obj() → free_mud_event()
   - Events removed from both event_q and region->events

2. **Free Phase:** Region memory is freed
   - Region names, vertices, reset_data freed
   - Old region_table freed

3. **Load Phase:** New regions loaded from database
   - New region_table allocated with CREATE
   - All fields explicitly initialized including events = NULL
   - Region data populated from MySQL

4. **Event Creation Phase:** New events created
   - For REGION_ENCOUNTER types with reset_time > 0
   - NEW_EVENT macro creates and attaches events
   - Events added to both event_q and region->events

### Memory Safety Patterns

- Always check pointers before freeing
- Initialize all pointer fields explicitly
- Process lists carefully when items self-remove
- Separate event cleanup from memory cleanup phases
- Use NOWHERE checks when converting vnums to rnums

---

## Summary

The Luminari MUD Wilderness System is a comprehensive, integrated environment that combines:

- **Procedural Terrain**: 2048x2048 coordinate grid with Perlin noise-based generation
- **Resource Management**: 10 resource types with real-time discovery and conservation tracking  
- **Event Communication**: PubSub messaging system with spatial audio and real-time event distribution
- **3D Audio**: Spatial positioning system with elevation-aware distance calculations
- **Performance Optimization**: KD-Tree indexing, caching systems, and efficient database queries
- **Administrative Tools**: Comprehensive debugging and management interfaces

**Current Status**: Fully operational with all major subsystems integrated and tested.

**Key Features for Players**:
- Enhanced exploration with resource discovery and mapping
- Immersive spatial audio effects and environmental communication
- Real-time wilderness event notifications and ecological feedback
- Conservation systems with environmental impact tracking

**Key Features for Builders**:
- OLC integration with coordinate-based room creation
- Resource system integration with terrain types
- PubSub event handlers for custom wilderness behaviors
- Administrative tools for system monitoring and debugging

**Key Features for Developers**:
- Modular architecture with clear separation of concerns
- Event-driven design with extensible handler registration
- Comprehensive API for custom system integration
- Performance monitoring and optimization tools

*See individual system documentation files for detailed implementation information and API references.*
