# Luminari MUD Wilderness System Documentation

## Table of Contents
1. [Overview](#overview)
2. [System Architecture](#system-architecture)
3. [Coordinate System](#coordinate-system)
4. [Terrain Generation](#terrain-generation)
5. [Room Management](#room-management)
6. [Regions and Paths](#regions-and-paths)
7. [Weather System](#weather-system)
8. [Player Experience](#player-experience)
9. [Builder Tools](#builder-tools)
10. [Technical Implementation](#technical-implementation)
11. [Configuration](#configuration)
12. [Troubleshooting](#troubleshooting)

## Overview

The Luminari MUD Wilderness System is a sophisticated procedural terrain generation and management system that creates vast, explorable wilderness areas. The system combines:

- **Procedural Generation**: Uses Perlin noise algorithms to generate realistic terrain
- **Dynamic Room Creation**: Creates rooms on-demand as players explore
- **Static Room Support**: Allows builders to create permanent wilderness locations
- **Region System**: Defines special areas with unique properties
- **Path System**: Creates roads, rivers, and other linear features
- **Weather Integration**: Location-specific weather patterns
- **Coordinate-Based Navigation**: X,Y coordinate system for precise positioning

### Key Features

- **Infinite Exploration**: 2048x2048 coordinate grid (4+ million possible locations)
- **Realistic Terrain**: Elevation, moisture, and temperature-based biome generation
- **Performance Optimized**: KD-Tree indexing and dynamic room pooling
- **Builder Friendly**: OLC integration and buildwalk support
- **Player Immersive**: Integrated mapping, weather, and navigation

## System Architecture

### Core Components

```
wilderness.c/h     - Main wilderness engine
perlin.c/h         - Noise generation algorithms
mysql.c/h          - Database integration for regions/paths
weather.c          - Weather system integration
act.movement.c     - Movement handling
redit.c            - OLC wilderness room editing
```

### Data Flow

```
Player Movement → Coordinate Calculation → Room Lookup/Creation → 
Terrain Generation → Region/Path Application → Room Assignment → 
Weather/Description Generation → Player Display
```

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

## Player Experience

**Movement:** Standard directions (±1 coordinate), auto room creation

**Map:** 21x21 automap, ASCII-only symbols (2025 fix for alignment), line-of-sight, weather overlay

**Navigation:** Coordinate display, static landmarks, path guidance

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
}
```

**Loading Sequence:**
1. **Zone Data**: Loads zone definitions including `ZONE_WILDERNESS` flags
2. **Room Data**: Loads static wilderness rooms with coordinate assignments
3. **Region Data**: Loads polygonal regions from MySQL database
4. **Path Data**: Loads linear paths (roads, rivers) from MySQL database
5. **Wilderness Initialization**: Builds KD-Tree indexes for performance

### Dynamic Content Generation

The wilderness system supports various forms of dynamic content generation:

**Room Generation:**
- Wilderness rooms created on-demand as players explore
- Procedural terrain and descriptions based on coordinate algorithms
- Dynamic exit creation for seamless exploration
- Automatic sector type assignment using noise functions

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

**Tables:** `region_data`, `region_index`, `path_data`, `path_index`, `path_types`

**Key Features:** GEOMETRY columns, SPATIAL INDEX, ST_Within() queries, glyph definitions

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

### Database Configuration

**MySQL Requirements:** 5.7+, spatial extensions (GEOMETRY, ST_Within, GeomFromText), SPATIAL INDEX support

### Perlin Noise Configuration

**Parameters:** Frequency(2.0), Lacunarity(2.0), Octaves(16)
**Techniques:** Ridged multifractal, attenuation for sharp peaks

## Troubleshooting

**Movement Fails:** Check `ZONE_WILDERNESS` flag, exits to room 1000000, coordinates set

**No Terrain Variation:** Verify noise seeds, test with `get_elevation()`, `get_moisture()`, `get_temperature()`

**Regions/Paths Missing:** Check MySQL spatial data, test `ST_Within()` queries

**Performance:** Monitor room pool usage (<80%), rebuild KD-Tree if slow

**Map Misalignment:** Fixed 2025 - ASCII-only symbols now (Y,^,~,=)

**Memory Crashes:** Use pattern `if (ptr && ptr != static_string) free(ptr)` - never free static strings

### Debug Commands

**Info:** `stat room`, `goto 1000000`, `genriver <dir> <vnum> <name>`, `genmap`
**Build:** `buildwalk` (toggle/reset/sector/name/desc)

### Maintenance

**Monitor:** Room pool <80%, KD-Tree <1ms, DB queries <100ms
**Tasks:** Rebuild KD-Tree, clean orphaned rooms, optimize DB indexes

---

## System Integration

**Combat:** Terrain modifiers, cover/concealment, environmental hazards
**Quests:** Static destinations, region triggers, exploration rewards
**Scripts:** DG triggers, region events, weather scripts
**Players:** Movement, coordinate tracking, automap
**Events:** Timed events, seasonal changes, dynamic spawning
**Database:** MySQL spatial queries, indexed lookups, persistent data

---

## Advanced Features

**Dynamic Content:** Terrain-based encounters, weather effects, resource respawning, loot tables

**Performance:** Room pool recycling at 90%, KD-Tree O(log n) lookups, spatial DB indexes

**Customization:** Add sector types via wild_map_info[], region scripts, custom path types

## Resources

**Files:** wilderness.c/h, perlin.c/h, mysql.c/h, act.movement.c, act.wizard.c

**Tables:** wilderness_data, region_data, path_data, path_types (all with spatial indexes)

---

*Wilderness system supporting 2048x2048 procedural terrain with dynamic rooms, regions, paths, and weather. See source code for implementation details.*
