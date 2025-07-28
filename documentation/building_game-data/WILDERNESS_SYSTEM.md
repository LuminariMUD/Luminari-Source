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
   - Creates more natural terrain features

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
    else if (elevation > 255 - HIGH_MOUNTAIN_THRESHOLD) // 200
        return SECT_HIGH_MOUNTAIN;

    // Mountain ranges
    else if (elevation > 255 - MOUNTAIN_THRESHOLD) // 185
        return SECT_MOUNTAIN;

    // Hills and elevated terrain
    else if (elevation > 255 - HILL_THRESHOLD) { // 175
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

### Wilderness Weather

The wilderness has its own weather system separate from zone-based weather:

```c
int get_weather(int x, int y) {
    double trans_x, trans_y, result, time_base;
    time_t now;

    // Get current time for dynamic weather changes
    now = time(NULL);
    time_base = (now % 100000) / 100000.0;

    // Transform coordinates for noise sampling
    trans_x = x / (double)(WILD_X_SIZE / 1.0);
    trans_y = y / (double)(WILD_Y_SIZE / 1.0);

    // Use 3D Perlin noise with time component for dynamic weather
    result = PerlinNoise3D(NOISE_WEATHER, trans_x * 50.0, trans_y * 50.0,
                          time_base * 100, 2.0, 2.0, 8);

    // Normalize result to 0-255 range
    result = (result + 1) / 2.0;
    return 255 * result;
}
```

### Weather Integration

- Each wilderness tile has its own weather value based on coordinates and time
- Weather changes dynamically over time using 3D Perlin noise
- Weather values range from 0-255 representing different conditions
- Time-based component ensures weather patterns evolve naturally
- Weather affects visibility and movement in some implementations
- Integrated with map display system for weather overlay mode
- Displayed in room descriptions and maps

### Weather Exclusion

```c
if (ZONE_FLAGGED(world[IN_ROOM(character)].zone, ZONE_WILDERNESS))
    continue; /* Wilderness weather is handled elsewhere */
```

Standard weather messages don't apply to wilderness zones.

### Campaign-Specific Behavior

**Forgotten Realms Campaign (`CAMPAIGN_FR`):**
- Wilderness system is completely disabled
- `find_room_by_coordinates()` returns `NOWHERE`
- `IS_DYNAMIC()` macro always returns `FALSE`
- No dynamic room allocation or wilderness movement

## Player Experience

### Movement

Players move through wilderness using standard directional commands:
- `north`, `south`, `east`, `west`
- Each movement changes coordinates by ±1
- Automatic room creation/assignment
- Seamless transition between static and dynamic areas

### Mapping

**Automatic Mapping:**
```c
if (PRF_FLAGGED(ch, PRF_AUTOMAP)) {
    show_wilderness_map(ch, 21, ch->coords[0], ch->coords[1]);
}
```

**Map Display:**
- 21x21 grid centered on player
- Color-coded terrain types
- Unicode symbols for terrain
- GUI mode support with XML tags

**Map Features:**
- Line-of-sight calculations using `line_vis()` function
- Terrain-appropriate symbols with Unicode support
- Player position indicator (\tM\t[u128946/*]\tn)
- Region and path overlays with color coding
- Weather overlay mode (`MAP_TYPE_WEATHER`)
- Circular or rectangular map shapes
- GUI mode support with XML tags

### Line-of-Sight System

```c
void line_vis(struct wild_map_tile **map, int x0, int y0, int x1, int y1) {
    // Bresenham's line algorithm implementation
    // Sets visibility flags for tiles along line of sight
    // Used to create realistic vision limitations
}
```

The system calculates visibility from the player's position to map edges, creating realistic sight lines that can be blocked by terrain features.

### Navigation

**Coordinate Display:**
Players can see their current coordinates and use them for navigation.

**Landmarks:**
Static rooms serve as permanent landmarks and reference points.

**Paths:**
Roads and rivers provide guided travel routes between locations.

## Builder Tools

### OLC Integration

**Wilderness Room Editing:**
```c
// Coordinates automatically set in OLC
OLC_ROOM(d)->coords[0] = x_coordinate;
OLC_ROOM(d)->coords[1] = y_coordinate;
```

**Buildwalk Support:**
- `PRF_BUILDWALK` flag enables building while walking
- Automatic coordinate calculation for wilderness zones
- Permission checking for zone editing with `can_edit_zone()`
- Integration with existing OLC commands
- Special wilderness buildwalk handling in `perform_move()`

### Wilderness Buildwalk Implementation

```c
int buildwalk(struct char_data *ch, int dir) {
    if (!PRF_FLAGGED(ch, PRF_BUILDWALK) || GET_LEVEL(ch) < LVL_BUILDER)
        return 0;

    // Special handling for wilderness zones
    if (ZONE_FLAGGED(world[IN_ROOM(ch)].zone, ZONE_WILDERNESS)) {
        int new_x = X_LOC(ch), new_y = Y_LOC(ch);

        // Calculate new coordinates based on direction
        switch (dir) {
            case NORTH: new_y++; break;
            case SOUTH: new_y--; break;
            case EAST:  new_x++; break;
            case WEST:  new_x--; break;
            default:
                send_to_char(ch, "Invalid direction for wilderness buildwalk.\r\n");
                return 0;
        }

        // Check build permissions
        if (!can_edit_zone(ch, world[IN_ROOM(ch)].zone)) {
            send_to_char(ch, "You do not have build permissions in this zone.\r\n");
            return 0;
        }

        // Create new room at coordinates
        room_rnum new_room = create_wilderness_room(new_x, new_y);
        if (new_room != NOWHERE) {
            // Set buildwalk properties if configured
            if (GET_BUILDWALK_SECTOR(ch) != SECT_INSIDE)
                world[new_room].sector_type = GET_BUILDWALK_SECTOR(ch);
            if (GET_BUILDWALK_NAME(ch))
                world[new_room].name = strdup(GET_BUILDWALK_NAME(ch));
            if (GET_BUILDWALK_DESC(ch))
                world[new_room].description = strdup(GET_BUILDWALK_DESC(ch));
        }
    }

    return 1;
}
```

**Buildwalk Configuration Commands:**
```
buildwalk                    // Toggle buildwalk on/off
buildwalk reset              // Reset all buildwalk settings
buildwalk sector <type>      // Set default sector type
buildwalk name <name>        // Set default room name
buildwalk desc <description> // Set default room description
```

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
    for (int i = 0; i < 500; i++) { // Max 500 segments
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

**KD-Tree Indexing:**
```c
struct kdtree *kd_wilderness_rooms;
// O(log n) lookup for static rooms
// Spatial indexing for coordinate-based queries
```

**Dynamic Room Pooling:**
- Fixed pool of 6000 dynamic rooms
- Automatic recycling of unused rooms
- `ROOM_OCCUPIED` flag management
- Memory-efficient room reuse

**Lazy Loading:**
- Rooms created only when accessed
- Terrain generated on-demand
- Database queries optimized with spatial indexes

### Memory Management

**Static Data:**
- Region and path data loaded at startup
- KD-Tree rebuilt when wilderness zones change
- Persistent coordinate storage

**Dynamic Data:**
- Room descriptions generated as needed
- Temporary room assignments
- Automatic cleanup of unused dynamic rooms

**Memory Allocation:**
```c
// Dynamic room pool allocation
for (i = WILD_DYNAMIC_ROOM_VNUM_START; i <= WILD_DYNAMIC_ROOM_VNUM_END; i++) {
    if (real_room(i) == NOWHERE) {
        // Room available for allocation
        return real_room(i);
    }
}
```

### Database Schema

**Region Data:**
```sql
CREATE TABLE region_data (
    vnum INT PRIMARY KEY,
    zone_vnum INT,
    name VARCHAR(255),
    region_type INT,
    region_polygon GEOMETRY,
    region_props INT,
    region_reset_data TEXT,
    region_reset_time INT,
    SPATIAL INDEX(region_polygon)
);

-- Optimized spatial index table for faster queries
CREATE TABLE region_index (
    vnum INT PRIMARY KEY,
    zone_vnum INT,
    region_polygon GEOMETRY,
    SPATIAL INDEX(region_polygon),
    FOREIGN KEY (vnum) REFERENCES region_data(vnum)
);
```

**Path Data:**
```sql
CREATE TABLE path_data (
    vnum INT PRIMARY KEY,
    zone_vnum INT,
    name VARCHAR(255),
    path_type INT,
    path_linestring GEOMETRY,
    path_props INT,
    SPATIAL INDEX(path_linestring)
);

-- Optimized spatial index table for faster path queries
CREATE TABLE path_index (
    vnum INT PRIMARY KEY,
    zone_vnum INT,
    path_linestring GEOMETRY,
    SPATIAL INDEX(path_linestring),
    FOREIGN KEY (vnum) REFERENCES path_data(vnum)
);

-- Path type definitions with display glyphs
CREATE TABLE path_types (
    path_type INT PRIMARY KEY,
    glyph_ns VARCHAR(50),    -- North-South glyph
    glyph_ew VARCHAR(50),    -- East-West glyph
    glyph_int VARCHAR(50)    -- Intersection glyph
);
```

**Spatial Queries:**
```sql
-- Find regions containing a point (actual query used)
SELECT vnum,
  CASE
    WHEN ST_Within(GeomFromText('POINT(x y)'), region_polygon) THEN
    CASE
      WHEN (GeomFromText('POINT(x y)') = Centroid(region_polygon)) THEN '1'
      WHEN (ST_Distance(GeomFromText('POINT(x y)'), ExteriorRing(region_polygon)) >
            ST_Distance(GeomFromText('POINT(x y)'), Centroid(region_polygon))/2) THEN '2'
      ELSE '3'
    END
    ELSE NULL
  END as location_type
FROM region_index
WHERE zone_vnum = ? AND ST_Within(GeomFromText('POINT(x y)'), region_polygon);

-- Find paths containing a point
SELECT vnum,
  CASE WHEN (ST_Within(GeomFromText('POINT(x y)'), path_linestring) AND
             ST_Within(GeomFromText('POINT(x y-1)'), path_linestring)) THEN 0  -- NS glyph
    WHEN (ST_Within(GeomFromText('POINT(x-1 y)'), path_linestring) AND
          ST_Within(GeomFromText('POINT(x+1 y)'), path_linestring)) THEN 1     -- EW glyph
    ELSE 2                                                                     -- Intersection glyph
  END AS glyph_type
FROM path_index
WHERE zone_vnum = ? AND ST_Within(GeomFromText('POINT(x y)'), path_linestring);
```

### Error Handling

**Coordinate Validation:**
```c
if (x < -WILD_X_SIZE/2 || x > WILD_X_SIZE/2 ||
    y < -WILD_Y_SIZE/2 || y > WILD_Y_SIZE/2) {
    log("SYSERR: Invalid wilderness coordinates (%d, %d)", x, y);
    return NOWHERE;
}
```

**Room Allocation:**
- Fallback when dynamic room pool exhausted
- Error recovery for failed room assignments
- Comprehensive error logging

**Database Error Handling:**
```c
if (mysql_query(conn, query)) {
    log("SYSERR: MySQL query failed: %s", mysql_error(conn));
    return NULL;
}
```

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
#define WATERLINE 138                      // Default waterline (can be modified via wild_waterline)
#define SHALLOW_WATER_THRESHOLD 20         // Shallow vs deep water
#define COASTLINE_THRESHOLD 5              // Beach/coastline threshold
#define PLAINS_THRESHOLD 35                // Plains elevation threshold
#define HIGH_MOUNTAIN_THRESHOLD 55         // High mountain threshold (used as 255 - 55 = 200)
#define MOUNTAIN_THRESHOLD 70              // Mountain threshold (used as 255 - 70 = 185)
#define HILL_THRESHOLD 80                  // Hill threshold (used as 255 - 80 = 175)

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

### Sector Mapping

Terrain generation can be customized by modifying sector thresholds:

```c
struct sector_limits sector_map[NUM_ROOM_SECTORS][3];
// [sector_type][elevation/moisture/temperature] = {min, max}
```

**Example Configuration:**
```c
// Mountains at high elevation
sector_map[SECT_MOUNTAIN][0] = {200, 255};  // elevation
sector_map[SECT_MOUNTAIN][1] = {0, 255};    // moisture (any)
sector_map[SECT_MOUNTAIN][2] = {0, 100};    // temperature (cold)

// Forests in moderate elevation with high moisture
sector_map[SECT_FOREST][0] = {50, 150};     // elevation
sector_map[SECT_FOREST][1] = {150, 255};    // moisture (high)
sector_map[SECT_FOREST][2] = {100, 200};    // temperature (moderate)
```

### Database Configuration

**MySQL Connection:**
- Configure connection parameters in mysql.c
- Ensure spatial extensions are enabled
- Create appropriate indexes for performance

**Required MySQL Extensions:**
- Spatial data types (GEOMETRY, POINT, POLYGON, LINESTRING)
- Spatial functions (ST_Within, ST_Distance, GeomFromText, AsText, NumPoints, Centroid, ExteriorRing)
- Spatial indexing support with SPATIAL INDEX
- MySQL version 5.7+ recommended for full spatial function support

### Perlin Noise Configuration

**Noise Parameters:**
```c
// Elevation noise - creates mountain ranges
result = PerlinNoise2D(NOISE_MATERIAL_PLANE_ELEV,
                       trans_x, trans_y,
                       2.0,    // frequency
                       2.0,    // lacunarity
                       16);    // octaves

// Compress data for better mountain formation
result = (result > .8 ? .8 : result);
result = (result < -.8 ? -.8 : result);

// Normalize between -1 and 1
result = result / .8;

// Use ridged multifractal for realistic mountains
result = 1 - (result < 0 ? -result : result);

// Apply attenuation for sharper peaks
result *= result;
result *= result;
```

**Customizable Parameters:**
- Frequency: Controls terrain feature size
- Lacunarity: Controls detail level
- Octaves: Controls noise complexity
- Attenuation: Controls mountain sharpness

## Troubleshooting

### Common Issues

**1. Wilderness Movement Fails**

*Symptoms:* Players can't move in wilderness, get "You can't go that way" messages

*Solutions:*
- Check zone has `ZONE_WILDERNESS` flag set
- Verify exits point to room 1000000 (navigation room)
- Ensure coordinates are properly set in room data
- Check that wilderness zone exists and is loaded

*Debug Commands:*
```
stat room        // Check current room flags and coordinates
goto 1000000     // Test navigation room accessibility
```

**2. Terrain Not Generating Properly**

*Symptoms:* All wilderness shows same terrain type, no variation

*Solutions:*
- Verify Perlin noise initialization with correct seeds
- Check noise seed values in wilderness.h
- Confirm sector mapping configuration
- Test noise generation functions

*Debug Steps:*
```c
// Test noise generation
int elev = get_elevation(NOISE_MATERIAL_PLANE_ELEV, x, y);
int moist = get_moisture(NOISE_MATERIAL_PLANE_MOISTURE, x, y);
int temp = get_temperature(NOISE_MATERIAL_PLANE_ELEV, x, y);
int sector = get_sector_type(elev, temp, moist);
log("Coordinates (%d,%d): elev=%d, temp=%d, moist=%d, sector=%d",
    x, y, elev, temp, moist, sector);
```

**3. Regions/Paths Not Working**

*Symptoms:* Regions don't affect terrain, paths don't display

*Solutions:*
- Check MySQL connection and database accessibility
- Verify spatial data exists in region_data/path_data tables
- Confirm geometry functions are available in MySQL
- Test spatial queries manually

*Database Tests:*
```sql
-- Test region data
SELECT COUNT(*) FROM region_data;
SELECT * FROM region_data WHERE zone_vnum = your_zone;

-- Test spatial functions
SELECT ST_Within(GeomFromText('POINT(0 0)'), region_polygon)
FROM region_data LIMIT 1;
```

**4. Performance Issues**

*Symptoms:* Slow wilderness movement, lag when exploring

*Solutions:*
- Monitor KD-Tree performance and rebuild if needed
- Check dynamic room pool usage and increase if necessary
- Optimize database queries with proper indexes
- Profile memory usage patterns

*Performance Monitoring:*
```c
// Check dynamic room pool usage
int used_rooms = 0;
for (i = WILD_DYNAMIC_ROOM_VNUM_START; i <= WILD_DYNAMIC_ROOM_VNUM_END; i++) {
    if (real_room(i) != NOWHERE && ROOM_FLAGGED(real_room(i), ROOM_OCCUPIED))
        used_rooms++;
}
log("Dynamic rooms in use: %d/%d", used_rooms,
    WILD_DYNAMIC_ROOM_VNUM_END - WILD_DYNAMIC_ROOM_VNUM_START + 1);
```

**5. Map Display Problems**

*Symptoms:* Maps show incorrectly, missing colors/symbols

*Solutions:*
- Verify Unicode support in client
- Check color code compatibility with client
- Confirm GUI mode settings for XML tags
- Test with different terminal/client software

*Map Debug:*
```c
// Test map generation
char *map_str = gen_ascii_wilderness_map(21, x, y, MAP_TYPE_NORMAL);
send_to_char(ch, "Raw map data:\r\n%s\r\n", map_str);
```

### Debug Commands

**Wilderness Information Commands:**
```
stat room                    // Show current room info including coordinates
goto 1000000                 // Go to wilderness navigation room
goto <room_vnum>             // Go to specific room by vnum
genmap <size> <filename>     // Generate PNG map file for debugging
genriver <dir> <vnum> <name> // Create procedural river
save_map_to_file <filename> <xsize> <ysize> // Save wilderness map as PNG
save_noise_to_file <idx> <filename> <xsize> <ysize> <zoom> // Save noise layer
```

**Wilderness-Specific Commands:**
```
buildwalk                    // Toggle wilderness buildwalk mode
buildwalk reset              // Reset buildwalk settings
buildwalk sector <type>      // Set default sector for new rooms
buildwalk name <name>        // Set default name for new rooms
buildwalk desc <description> // Set default description for new rooms
```

**Database Debugging:**
```sql
-- Check region coverage
SELECT r.name, COUNT(*) as rooms_affected
FROM region_data r
JOIN wilderness_rooms w ON ST_Within(w.point, r.region_polygon)
GROUP BY r.vnum;

-- Verify path data
SELECT name, path_type, ST_AsText(path_linestring)
FROM path_data
WHERE zone_vnum = your_zone;
```

### Log Messages

The system provides comprehensive logging for debugging:

**Room Management:**
```
INFO: Assigning wilderness room %d to coordinates (%d, %d)
SYSERR: Attempted to assign NOWHERE as wilderness location at (%d, %d)
SYSERR: Wilderness movement failed from (%d, %d) to (%d, %d)
SYSERR: Illegal value(s) passed to char_to_room. (Room: %d/%d Ch: %p)
```

**Database Operations:**
```
INFO: Loading wilderness data for zone: %d
INFO: Loading region data from MySQL
INFO: Loading path data from MySQL
SYSERR: Unable to SELECT from wilderness_data: %s
SYSERR: Unable to SELECT from region_data: %s
SYSERR: Unable to SELECT from path_data: %s
INFO: Deleting Path [%d] from MySQL
SYSERR: Unable to delete from path_data: %s
```

**Initialization Messages:**
```
Loading regions. (MySQL)
Loading paths. (MySQL)
Indexing wilderness rooms.
Writing wilderness map image.
```

**Performance Warnings:**
```
WARNING: Dynamic room pool nearly exhausted (%d/%d used)
WARNING: KD-Tree lookup taking longer than expected
```

### Performance Monitoring

**Key Metrics to Monitor:**

1. **Dynamic Room Pool Utilization**
   - Target: < 80% usage during peak times
   - Alert: > 90% usage indicates need for pool expansion

2. **KD-Tree Query Performance**
   - Target: < 1ms average lookup time
   - Alert: > 10ms indicates need for tree rebuild

3. **Database Response Times**
   - Target: < 100ms for region/path queries
   - Alert: > 500ms indicates indexing issues

4. **Memory Usage Patterns**
   - Monitor for memory leaks in dynamic room allocation
   - Check for proper cleanup of unused rooms

**Monitoring Commands:**
```c
// Add to periodic maintenance
void wilderness_performance_check() {
    // Check room pool usage
    // Monitor KD-tree performance
    // Test database connectivity
    // Report metrics to logs
}
```

### Maintenance Tasks

**Regular Maintenance:**
- Rebuild KD-Tree indexes when static rooms change
- Clean up orphaned dynamic rooms
- Optimize database indexes
- Monitor and rotate log files

**Database Maintenance:**
```sql
-- Optimize spatial indexes
OPTIMIZE TABLE region_data;
OPTIMIZE TABLE path_data;

-- Check for data integrity
SELECT COUNT(*) FROM region_data WHERE region_polygon IS NULL;
SELECT COUNT(*) FROM path_data WHERE path_linestring IS NULL;
```

---

## System Integration

The wilderness system integrates with multiple other game systems:

### Combat System Integration
- **Battlegrounds**: Wilderness provides diverse tactical environments for combat
- **Terrain Effects**: Different sector types affect movement, visibility, and combat mechanics
- **Cover and Concealment**: Forest and mountain terrain provide tactical advantages
- **Environmental Hazards**: Weather and terrain can affect combat outcomes

### Quest System Integration
- **Quest Locations**: Static wilderness rooms serve as quest destinations
- **Dynamic Objectives**: Procedural content can generate quest targets
- **Region-Based Quests**: Regions can trigger specific quest content
- **Exploration Rewards**: Discovery of new areas can provide quest completion

### Scripting System Integration
- **Room Triggers**: Wilderness rooms support DG Script triggers
- **Region Events**: Regions can execute scripts when players enter/exit
- **Path Interactions**: Roads and rivers can have associated script events
- **Weather Scripts**: Weather changes can trigger environmental scripts

### Player System Integration
- **Movement Handling**: Seamless integration with standard movement commands
- **Coordinate Tracking**: Player positions tracked in wilderness coordinate system
- **Mapping Integration**: Automatic map display and navigation assistance
- **Preference System**: Player preferences for automap, GUI mode, etc.

### Event System Integration
- **Timed Events**: Wilderness areas can host scheduled events
- **Seasonal Changes**: Environmental changes based on game calendar
- **Dynamic Spawning**: Event-driven NPC and object spawning
- **Weather Events**: Integration with global weather patterns

### Database Integration
- **Spatial Queries**: MySQL spatial functions for region and path management
- **Performance Optimization**: Indexed spatial data for fast lookups
- **Data Persistence**: Region and path data stored permanently
- **Real-time Updates**: Dynamic updates to wilderness features

This comprehensive integration creates a dynamic, living wilderness environment that responds to player actions while maintaining consistent game world rules and behaviors.

---

## Advanced Features

### Dynamic Content Generation

The wilderness system supports various forms of dynamic content beyond basic terrain:

**Environmental Spawning:**
- Random encounters based on terrain type and regions
- Dynamic weather effects that vary by location and time
- Seasonal changes affecting terrain appearance
- Time-based lighting and visibility modifications

**Resource Generation:**
- Harvestable resources that respawn based on biome
- Dynamic treasure spawning in appropriate terrain
- Region-specific loot tables and spawn rates
- Environmental hazards and special features

### Performance Optimization Details

**Memory Management:**
```c
// Dynamic room pool management
#define DYNAMIC_ROOM_POOL_SIZE (WILD_DYNAMIC_ROOM_VNUM_END - WILD_DYNAMIC_ROOM_VNUM_START + 1)

// Room recycling when pool approaches capacity
if (used_rooms > (DYNAMIC_ROOM_POOL_SIZE * 0.9)) {
    cleanup_unused_wilderness_rooms();
}
```

**KD-Tree Performance:**
- O(log n) lookup time for static room queries
- Automatic rebuilding when static rooms are added/removed
- Spatial indexing optimized for 2D coordinate queries
- Memory-efficient storage of room references

**Database Query Optimization:**
- Spatial indexes on region_polygon and path_linestring columns
- Prepared statements for frequent queries
- Connection pooling and automatic reconnection
- Batch operations for bulk data loading

### Integration with Other Systems

**Combat System Integration:**
```c
// Terrain affects combat mechanics
int get_terrain_combat_modifier(int sector_type) {
    switch (sector_type) {
        case SECT_FOREST:
            return COMBAT_COVER_BONUS;
        case SECT_MOUNTAIN:
            return COMBAT_HIGH_GROUND_BONUS;
        case SECT_WATER_SWIM:
            return COMBAT_MOVEMENT_PENALTY;
        default:
            return 0;
    }
}
```

**Quest System Integration:**
- Dynamic quest objectives based on wilderness exploration
- Region-based quest triggers and completion
- Procedural quest generation using terrain features
- Discovery rewards for finding new areas

**Scripting System (DG Scripts) Integration:**
- Room triggers that activate based on coordinates
- Region entry/exit script execution
- Weather-based script triggers
- Time-of-day wilderness events

### Advanced Configuration

**Custom Terrain Types:**
```c
// Adding new sector types requires:
// 1. Define new SECT_* constant
// 2. Add to wild_map_info[] array with display glyph
// 3. Update get_sector_type() logic
// 4. Add to sector_map configuration

struct wild_map_info_type custom_terrain = {
    SECT_CUSTOM_SWAMP,
    "\tg\t[u127807/S]\tn",  // Green swamp symbol
    {variant_glyphs}         // Optional variants
};
```

**Region Scripting:**
```c
// Regions can execute scripts on entry/exit
struct region_data {
    // ... existing fields ...
    struct list_data *events;  // Script events
    char *entry_script;        // Script on region entry
    char *exit_script;         // Script on region exit
    int script_flags;          // Script execution flags
};
```

**Path Customization:**
```c
// Custom path types with special properties
#define PATH_CUSTOM_BRIDGE 7
#define PATH_CUSTOM_TUNNEL 8

// Path glyphs for different orientations
char *path_glyphs[3] = {
    "\tY|\tn",    // North-South glyph
    "\tY-\tn",    // East-West glyph
    "\tY+\tn"     // Intersection glyph
};
```

## Additional Resources

### Related Documentation
- `documentation/building_game-data/attaching_zones_wilderness.md` - Builder guide for zone attachment
- `documentation/systems/COMBAT_SYSTEM.md` - Combat system integration
- `documentation/systems/SCRIPTING_SYSTEM_DG.md` - DG Scripts integration
- `documentation/systems/OLC_ONLINE_CREATION_SYSTEM.md` - OLC system documentation

### Source Code Files
- `wilderness.c/h` - Main wilderness implementation
- `perlin.c/h` - Noise generation algorithms
- `mysql.c/h` - Database integration
- `act.movement.c` - Movement handling and buildwalk
- `act.wizard.c` - Administrative commands (genriver, genmap)
- `oasis_copy.c` - Buildwalk implementation
- `handler.c` - Character placement and room management
- `db.c` - World loading and wilderness initialization

### Database Schema Files
```sql
-- Core wilderness tables
CREATE TABLE wilderness_data (
    id INT PRIMARY KEY,
    zone_vnum INT,
    nav_vnum INT,
    dynamic_vnum_pool_start INT,
    dynamic_vnum_pool_end INT,
    x_size INT,
    y_size INT,
    elevation_seed INT,
    distortion_seed INT,
    moisture_seed INT,
    min_temp INT,
    max_temp INT
);

CREATE TABLE region_data (
    vnum INT PRIMARY KEY,
    zone_vnum INT,
    name VARCHAR(255),
    region_type INT,
    region_polygon GEOMETRY,
    region_props INT,
    region_reset_data TEXT,
    region_reset_time INT,
    SPATIAL INDEX(region_polygon)
);

CREATE TABLE path_data (
    vnum INT PRIMARY KEY,
    zone_vnum INT,
    name VARCHAR(255),
    path_type INT,
    path_linestring GEOMETRY,
    path_props INT,
    SPATIAL INDEX(path_linestring)
);

CREATE TABLE path_types (
    path_type INT PRIMARY KEY,
    glyph_ns VARCHAR(50),
    glyph_ew VARCHAR(50),
    glyph_int VARCHAR(50)
);
```

### Development Guidelines

**Adding New Features:**
1. Consider impact on existing wilderness areas
2. Test with both static and dynamic rooms
3. Verify database schema changes are backward compatible
4. Update documentation and builder guides
5. Test performance impact with large coordinate ranges

**Debugging Workflow:**
1. Check log files for wilderness-related errors
2. Verify database connectivity and spatial functions
3. Test noise generation with known coordinates
4. Validate KD-tree indexing with stat commands
5. Monitor dynamic room pool usage

**Performance Monitoring:**
```c
// Add to periodic maintenance routine
void wilderness_health_check() {
    int static_rooms = count_static_wilderness_rooms();
    int dynamic_used = count_occupied_dynamic_rooms();
    double kd_tree_efficiency = measure_kd_tree_performance();

    log("Wilderness Health: Static=%d, Dynamic=%d/%d, KD-Tree=%.2fms",
        static_rooms, dynamic_used, DYNAMIC_ROOM_POOL_SIZE, kd_tree_efficiency);

    if (dynamic_used > (DYNAMIC_ROOM_POOL_SIZE * 0.8)) {
        log("WARNING: Dynamic room pool approaching capacity");
    }
}
```

---

*This comprehensive documentation covers all aspects of the Luminari MUD Wilderness System. The system is designed to be extensible and performant, supporting both small-scale wilderness areas and large procedural worlds. For specific implementation details, advanced customization, or troubleshooting complex issues, refer to the source code and consult with the development team.*
