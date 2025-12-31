# LuminariMUD Vessel System Documentation

**Version**: 2.4840 (Phase 03 Complete)
**Last Updated**: 2025-12-30

---

## Table of Contents

1. [System Overview](#system-overview)
2. [Architecture](#architecture)
3. [Data Structures](#data-structures)
4. [Vessel Types and Capabilities](#vessel-types-and-capabilities)
5. [API Reference](#api-reference)
6. [Player Commands](#player-commands)
7. [Integration Testing Workflows](#integration-testing-workflows)
8. [Automation Layer](#automation-layer)
9. [Vehicle-in-Vessel Mechanics](#vehicle-in-vessel-mechanics)
10. [Interior Room Generation](#interior-room-generation)
11. [Performance Characteristics](#performance-characteristics)
12. [Key Constants](#key-constants)
13. [Database Schema](#database-schema)
14. [File Inventory](#file-inventory)
15. [Dependencies](#dependencies)
16. [Troubleshooting](#troubleshooting)
17. [Verification & Monitoring](#verification--monitoring)
18. [Risk Assessment](#risk-assessment)
19. [Known Issues](#known-issues)
20. [Deployment Notes](#deployment-notes)
21. [Development](#development)

---

## System Overview

The LuminariMUD Vessel System provides a comprehensive transport framework for water-based vessels (ships, boats, submarines, airships) and land-based vehicles (carts, wagons, mounts, carriages). The system was developed across 4 phases totaling 28 sessions.

### Design Goals

- **Scalability**: Support 500+ concurrent vessels with <1KB memory per vessel
- **Flexibility**: Multiple vessel types with distinct terrain capabilities
- **Automation**: Autopilot, waypoint-based navigation, and NPC pilot support
- **Integration**: Seamless wilderness coordinate system integration
- **Unified Interface**: Common commands work across vessel and vehicle types

### Two-Tier Transport Architecture

| Tier | Type | Memory | Interior | Use Case |
|------|------|--------|----------|----------|
| **Vessel** | Ships, airships, submarines | ~1KB | Multi-room | Ocean travel, cargo transport, combat |
| **Vehicle** | Carts, wagons, mounts | 148 bytes | None | Land travel, cargo hauling, quick transport |

### System Components

| Component | Description | Source Files |
|-----------|-------------|--------------|
| Core Vessels | Ship management, coordinates, movement | vessels.c, vessels.h |
| Autopilot | Waypoint navigation, route following | vessels_autopilot.c |
| Interior Rooms | Multi-room ship interiors | vessels_rooms.c |
| Docking | Ship-to-ship docking mechanics | vessels_docking.c |
| Persistence | Database save/load operations | vessels_db.c |
| Vehicles | Land-based transport | vehicles.c |
| Vehicle Commands | Player vehicle interactions | vehicles_commands.c |
| Vehicle Transport | Vehicle-on-vessel mechanics | vehicles_transport.c |

---

## Architecture

### Memory Layout

```
+------------------+
|  greyhawk_ship_  |  ~1016 bytes per vessel
|      data        |  Maximum 500 vessels = ~496KB
+------------------+
        |
        v
+------------------+
|  autopilot_data  |  Optional, ~64 bytes
+------------------+
        |
        v
+------------------+
| vessel_schedule  |  Optional, ~32 bytes
+------------------+
```

### Wilderness Coordinate System

Vessels operate on a 2D wilderness grid:
- X coordinate: -1024 to +1024
- Y coordinate: -1024 to +1024
- Z coordinate: Altitude (airships) or depth (submarines)

### State Machine

```
DOCKED <--> TRAVELING <--> COMBAT
   |            |
   v            v
DAMAGED <-------+
```

Autopilot States:
```
OFF --> TRAVELING --> WAITING --> COMPLETE
          ^   |         |
          +---+---------+
               PAUSED
```

### System Diagram

```
UNIFIED TRANSPORT SYSTEM
    |
    +-- Wilderness Coordinate System (X, Y, Z navigation)
    |       Range: -1024 to +1024 on X/Y, -500 to +500 on Z
    |
    +-- VESSEL TIER (Heavy Transport)
    |       +-- Vessel Type System (8 vessel classes)
    |       |       RAFT, BOAT, SHIP, WARSHIP, AIRSHIP, SUBMARINE, TRANSPORT, MAGICAL
    |       +-- Multi-Room Interiors (VNUM Range: 70000-79999)
    |       +-- Automation Layer (autopilot, waypoints, NPC pilots)
    |       +-- Docking and Boarding Systems
    |
    +-- VEHICLE TIER (Light Transport)
    |       +-- Vehicle Type System (5 vehicle types)
    |       |       NONE, CART, WAGON, MOUNT, CARRIAGE
    |       +-- Land-based terrain navigation
    |       +-- Vehicle-in-Vessel mechanics (loading vehicles onto ships)
    |       +-- Lightweight persistence (148 bytes per vehicle)
    |
    +-- UNIFIED INTERFACE
    |       +-- Common commands: tenter, texit, tgo, tstatus
    |       +-- Transport type detection
    |       +-- Seamless vehicle/vessel interaction
    |
    +-- Terrain Integration (40 sector types, speed modifiers)
    |
    +-- Database Persistence (vessel and vehicle tables)
```

---

## Data Structures

### Vessel Data (greyhawk_ship_data)

Primary vessel structure containing all ship state:

```c
struct greyhawk_ship_data {
    /* Identification */
    char name[128];           /* Ship name */
    char id[3];               /* Ship ID (AA-ZZ) */
    char owner[64];           /* Owner name */
    int shipnum;              /* Ship index */

    /* Position */
    float x, y, z;            /* Wilderness coordinates */
    short int heading;        /* Direction 0-360 */
    short int speed;          /* Current speed */

    /* Armor (per side) */
    unsigned char farmor;     /* Fore armor */
    unsigned char rarmor;     /* Rear armor */
    unsigned char parmor;     /* Port armor */
    unsigned char sarmor;     /* Starboard armor */

    /* Interior */
    int num_rooms;            /* Room count (1-20) */
    int room_vnums[20];       /* Interior room VNUMs */
    int entrance_room;        /* Boarding point */
    int bridge_room;          /* Control room */

    /* Automation */
    struct autopilot_data *autopilot;
    struct vessel_schedule *schedule;
};
```

### Vehicle Data (vehicle_data)

Lightweight structure for land vehicles (~148 bytes):

```c
struct vehicle_data {
    int id;                   /* Unique ID */
    enum vehicle_type type;   /* CART, WAGON, MOUNT, CARRIAGE */
    enum vehicle_state state; /* IDLE, MOVING, LOADED, etc. */
    char name[64];            /* Vehicle name */

    room_rnum location;       /* Current room */
    int x_coord, y_coord;     /* Wilderness coordinates */

    int max_passengers;       /* Capacity */
    int current_passengers;   /* Current count */
    int max_weight;           /* Weight limit (lbs) */
    int current_weight;       /* Current load */

    int base_speed;           /* Rooms per tick */
    int terrain_flags;        /* VTERRAIN_* bitfield */
    int condition;            /* Durability 0-100 */
};
```

### Autopilot Data

```c
struct autopilot_data {
    enum autopilot_state state;  /* OFF, TRAVELING, WAITING, etc. */
    struct ship_route *current_route;
    int current_waypoint_index;
    int wait_remaining;          /* Seconds at waypoint */
    int pilot_mob_vnum;          /* NPC pilot VNUM (-1 if none) */
};
```

---

## Vessel Types and Capabilities

### Vessel Classifications

| Type | Terrain | Speed | Rooms | Use Case |
|------|---------|-------|-------|----------|
| RAFT | Rivers, shallow | Slow | 1 | Short river crossings |
| BOAT | Coastal | Moderate | 1-3 | Fishing, coastal travel |
| SHIP | Ocean | Moderate | 5-10 | Long-distance travel |
| WARSHIP | Ocean | Fast | 8-15 | Naval combat |
| AIRSHIP | Air | Fast | 5-12 | Flying transport |
| SUBMARINE | Underwater | Slow | 4-8 | Underwater exploration |
| TRANSPORT | Ocean | Slow | 10-20 | Cargo, passengers |
| MAGICAL | Any | Variable | 1-5 | Special magical vessels |

### Terrain Capabilities

```c
struct vessel_terrain_caps {
    bool can_traverse_ocean;      /* Deep water */
    bool can_traverse_shallow;    /* Rivers */
    bool can_traverse_air;        /* Flying */
    bool can_traverse_underwater; /* Diving */
    int min_water_depth;          /* Required depth */
    int max_altitude;             /* Max flight height */
};
```

### Terrain Speed Modifiers

| Terrain | Surface Vessels | Airships | Submarines |
|---------|----------------|----------|------------|
| Ocean/Deep Water | 100% | 100% | 100% |
| Shallow Water | 75% | 100% | 0% (blocked) |
| Rivers | 50-100% (by type) | 100% | 0% (blocked) |
| Land/Mountains | 0% (blocked) | 75-100% | 0% (blocked) |
| Storm conditions | -25% | -50% | 0% |

### Vehicle Types

| Type | Description | Capacity | Speed | Terrain |
|------|-------------|----------|-------|---------|
| `VEHICLE_NONE` | Invalid/unassigned | - | - | - |
| `VEHICLE_CART` | Small hand cart | 1 passenger, 200 lbs | 80% | Road, plains |
| `VEHICLE_WAGON` | Large cargo wagon | 4 passengers, 1000 lbs | 60% | Road, plains, forest |
| `VEHICLE_MOUNT` | Riding mount | 1 passenger, 100 lbs | 120% | Most terrain |
| `VEHICLE_CARRIAGE` | Passenger carriage | 6 passengers, 500 lbs | 70% | Road, plains |

### Vehicle States

| State | Description |
|-------|-------------|
| `VSTATE_IDLE` | Stationary, not in use |
| `VSTATE_MOVING` | Currently in motion |
| `VSTATE_LOADED` | Carrying cargo/passengers |
| `VSTATE_HITCHED` | Connected to another vehicle |
| `VSTATE_DAMAGED` | Requires repair before use |
| `VSTATE_ON_VESSEL` | Loaded onto a vessel |

### Vehicle Terrain Flags

| Flag | Description |
|------|-------------|
| `VTERRAIN_ROAD` | Paved roads, paths |
| `VTERRAIN_PLAINS` | Open grasslands |
| `VTERRAIN_FOREST` | Wooded areas |
| `VTERRAIN_HILLS` | Hilly terrain |
| `VTERRAIN_MOUNTAIN` | Mountain passes (mounts only) |
| `VTERRAIN_DESERT` | Desert terrain |
| `VTERRAIN_WATER_SHALLOW` | Shallow water crossings |

### Vehicle Speed Modifiers by Terrain

| Terrain | Cart | Wagon | Mount | Carriage |
|---------|------|-------|-------|----------|
| Road | 150% | 150% | 150% | 150% |
| Plains | 100% | 100% | 100% | 100% |
| Forest | 50% | 75% | 100% | 50% |
| Hills | 50% | 50% | 75% | 50% |
| Mountain | 0% | 0% | 50% | 0% |
| Swamp | 0% | 0% | 50% | 0% |

---

## API Reference

### Vessel Lifecycle

```c
/* Initialize vessel system at boot */
void vessel_init_all(void);

/* Load/save vessel data */
void load_vessels(void);
void save_vessels(void);

/* Find vessel by ID */
struct vessel_data *find_vessel_by_id(int vessel_id);
```

### Vessel Movement

```c
/* Update wilderness position */
bool update_ship_wilderness_position(int shipnum, int new_x, int new_y, int new_z);

/* Move ship in direction */
bool move_ship_wilderness(int shipnum, int direction, struct char_data *ch);

/* Check terrain traversability */
bool can_vessel_traverse_terrain(enum vessel_class vessel_type, int x, int y, int z);

/* Get speed modifier for terrain */
int get_terrain_speed_modifier(enum vessel_class vessel_type, int sector_type, int weather);
```

### Autopilot Functions

```c
/* Lifecycle */
struct autopilot_data *autopilot_init(struct greyhawk_ship_data *ship);
void autopilot_cleanup(struct greyhawk_ship_data *ship);

/* Control */
int autopilot_start(struct greyhawk_ship_data *ship, struct ship_route *route);
int autopilot_stop(struct greyhawk_ship_data *ship);
int autopilot_pause(struct greyhawk_ship_data *ship);
int autopilot_resume(struct greyhawk_ship_data *ship);

/* Waypoints */
int waypoint_add(struct ship_route *route, float x, float y, float z, const char *name);
struct waypoint *waypoint_get_current(struct greyhawk_ship_data *ship);

/* Routes */
struct ship_route *route_create(const char *name);
int route_save(struct ship_route *route);

/* Tick processing */
void autopilot_tick(void);
```

### Vehicle Functions

```c
/* Lifecycle */
struct vehicle_data *vehicle_create(enum vehicle_type type, const char *name);
void vehicle_destroy(struct vehicle_data *vehicle);

/* State */
int vehicle_set_state(struct vehicle_data *vehicle, enum vehicle_state new_state);
const char *vehicle_state_name(enum vehicle_state state);

/* Movement */
int vehicle_can_move(struct vehicle_data *vehicle, int direction);
int vehicle_move(struct vehicle_data *vehicle, int direction);
int vehicle_can_traverse_terrain(struct vehicle_data *vehicle, int sector_type);

/* Passengers */
int vehicle_add_passenger(struct vehicle_data *vehicle);
int vehicle_remove_passenger(struct vehicle_data *vehicle);

/* Condition */
int vehicle_damage(struct vehicle_data *vehicle, int amount);
int vehicle_repair(struct vehicle_data *vehicle, int amount);
```

### Persistence Functions

```c
/* Vessel persistence */
void save_all_vessels(void);
void load_all_ship_interiors(void);

/* Waypoint/route persistence */
void save_all_waypoints(void);
void save_all_routes(void);
void load_all_waypoints(void);
void load_all_routes(void);

/* Schedule persistence */
void save_all_schedules(void);
void load_all_schedules(void);

/* Vehicle persistence */
void vehicle_save_all(void);
void vehicle_load_all(void);
```

---

## Player Commands

### Vessel Commands

| Command | Description | Usage |
|---------|-------------|-------|
| board | Board a vessel | `board <ship>` |
| greyhawk_tactical | Display tactical map | `tactical` |
| greyhawk_status | Show ship status | `shipstatus` |
| greyhawk_speed | Set ship speed | `speed <0-30>` |
| greyhawk_heading | Set ship heading | `heading <0-360>` |
| dock | Dock with vessel | `dock <ship>` |
| undock | Undock from vessel | `undock` |
| look_outside | View from interior | `lookout` |

### Autopilot Commands

| Command | Description | Usage |
|---------|-------------|-------|
| autopilot | Toggle autopilot | `autopilot on/off/status` |
| setwaypoint | Create waypoint | `setwaypoint <name>` |
| listwaypoints | List waypoints | `listwaypoints` |
| delwaypoint | Delete waypoint | `delwaypoint <id>` |
| createroute | Create route | `createroute <name>` |
| addtoroute | Add waypoint to route | `addtoroute <route> <waypoint>` |
| listroutes | List routes | `listroutes` |
| setroute | Assign route | `setroute <route>` |

### NPC Pilot Commands

| Command | Description | Usage |
|---------|-------------|-------|
| assignpilot | Assign NPC pilot | `assignpilot <npc>` |
| unassignpilot | Remove NPC pilot | `unassignpilot` |

### Schedule Commands

| Command | Description | Usage |
|---------|-------------|-------|
| setschedule | Set departure schedule | `setschedule <route> <interval>` |
| clearschedule | Clear schedule | `clearschedule` |
| showschedule | Display schedule | `showschedule` |

### Vehicle Commands

| Command | Description | Usage |
|---------|-------------|-------|
| vmount | Mount vehicle | `vmount <vehicle>` |
| vdismount | Dismount vehicle | `vdismount` |
| drive | Drive vehicle | `drive <direction>` |
| vstatus | Vehicle status | `vstatus` |
| hitch | Hitch vehicles | `hitch <vehicle>` |
| unhitch | Unhitch vehicles | `unhitch` |
| loadvehicle | Load onto vessel | `loadvehicle <vehicle>` |
| unloadvehicle | Unload from vessel | `unloadvehicle <vehicle>` |

### Unified Transport Commands

| Command | Description | Usage |
|---------|-------------|-------|
| transport_enter | Enter any transport | `tenter <transport>` |
| exit_transport | Exit transport | `texit` |
| transport_go | Move transport | `tgo <direction>` |
| transportstatus | Transport status | `tstatus` |

---

## Integration Testing Workflows

### Complete Vessel Workflow

1. **Create** - Load ship via OLC or admin command
2. **Board** - Player boards vessel (`board ship`)
3. **Navigate** - Set heading and speed (`heading 90`, `speed 15`)
4. **Move** - Ship moves on wilderness grid
5. **Dock** - Approach and dock with target (`dock pier`)
6. **Interior** - Move through ship rooms
7. **Undock** - Depart from dock (`undock`)

### Complete Vehicle Workflow

1. **Create** - Spawn vehicle via creation system
2. **Mount** - Player mounts vehicle (`vmount wagon`)
3. **Load** - Add cargo/passengers
4. **Drive** - Move through rooms (`drive north`)
5. **Dismount** - Exit vehicle (`vdismount`)

### Vessel + Vehicle Combined Workflow

1. Create vessel and vehicle
2. Board vessel
3. Navigate vessel to port
4. Mount vehicle
5. Load vehicle onto vessel (`loadvehicle wagon`)
6. Sail to destination
7. Unload vehicle (`unloadvehicle wagon`)
8. Drive vehicle ashore

### Autopilot Workflow

1. Create waypoints at key locations
2. Create route with ordered waypoints
3. Assign route to vessel
4. Enable autopilot
5. Vessel follows route automatically
6. Optional: Assign NPC pilot for announcements

---

## Automation Layer

### Autopilot System

The autopilot system enables hands-free navigation along predefined waypoint routes.

**States:**
- `AUTOPILOT_INACTIVE` - System off
- `AUTOPILOT_ACTIVE` - Following route
- `AUTOPILOT_PAUSED` - Temporarily stopped
- `AUTOPILOT_COMPLETE` - Route finished

**Features:**
- Waypoint-based path following
- Automatic terrain avoidance
- Speed adjustment for conditions
- Pause/resume capability

### Route Management

Routes are named collections of waypoints that can be saved and reused.

**Structure:**
- Up to 20 waypoints per route
- X, Y, Z coordinates per waypoint
- Optional waypoint names
- Database persistence

### NPC Pilot Integration

NPC characters can be assigned as vessel pilots for automated operation.

**Capabilities:**
- Autonomous navigation
- Route following
- Alert generation for obstacles
- Crew management integration

### Scheduled Routes

Vessels can be scheduled to depart at specific times.

**Features:**
- Time-based departure scheduling
- Automatic route activation
- Repeat schedule support
- Integration with NPC pilots

---

## Vehicle-in-Vessel Mechanics

Vehicles can be loaded onto vessels for transport across water.

### Loading Requirements

- Vessel must be stationary (speed = 0) or docked
- Vehicle must be in same room as vessel boarding point
- Vehicle must not already be on a vessel
- Vessel must have available cargo capacity

### Unloading Requirements

- Vessel must be stationary or docked
- Vehicle must be on the vessel
- Must be at valid unload location (dock or shore)

### State Transitions

```
VSTATE_IDLE --> loadvehicle --> VSTATE_ON_VESSEL
VSTATE_ON_VESSEL --> unloadvehicle --> VSTATE_IDLE
```

### Coordinate Synchronization

When a vessel moves, all loaded vehicles automatically update their coordinates to match the vessel's position.

---

## Interior Room Generation

Ships automatically generate interior rooms based on vessel type when loaded:

| Vessel Type | Room Count | Rooms Generated |
|-------------|------------|-----------------|
| VESSEL_RAFT | 1-2 | Bridge only |
| VESSEL_BOAT | 2-4 | Bridge, Quarters |
| VESSEL_SHIP | 3-8 | Bridge, Quarters, Cargo, Deck |
| VESSEL_WARSHIP | 5-15 | Bridge, Armory, Weapons, Quarters, Brig |
| VESSEL_AIRSHIP | 4-12 | Bridge, Observation, Engineering, Quarters |
| VESSEL_SUBMARINE | 4-10 | Bridge, Helm, Engineering, Quarters |
| VESSEL_TRANSPORT | 6-20 | Bridge, Large Cargo, Passenger Quarters |
| VESSEL_MAGICAL | Variable | Custom configuration |

---

## Performance Characteristics

### Memory Usage

| Component | Per-Unit | Maximum | Total |
|-----------|----------|---------|-------|
| Vessel | 1016 bytes | 500 | 496 KB |
| Vehicle | 148 bytes | 1000 | 145 KB |
| Waypoint | ~80 bytes | 1000 | 78 KB |
| Route | ~200 bytes | 200 | 39 KB |

### Stress Test Results

| Level | Vessels | Memory | Create Time | Ops/sec |
|-------|---------|--------|-------------|---------|
| Low | 100 | 99.2 KB | 0.05 ms | 185M |
| Medium | 250 | 248 KB | 0.13 ms | 195M |
| High | 500 | 496 KB | 0.21 ms | 192M |

### Quality Metrics

- **Unit Tests**: 353 tests, 100% passing
- **Memory Leaks**: 0 (Valgrind verified)
- **Coverage**: All core modules tested
- **Stress**: 500 concurrent vessels stable

### Structure Sizes

| Structure | Size |
|-----------|------|
| `struct greyhawk_ship_data` | 1016 bytes |
| `struct vehicle_data` | 148 bytes |
| `struct waypoint` | 88 bytes |
| `struct ship_route` | 1840 bytes |
| `struct autopilot_data` | 48 bytes |
| `struct waypoint_node` | 104 bytes |
| `struct transport_data` | 16 bytes |

### Vehicle Stress Test Results

| Level | Vehicles | Memory Used | Per-Vehicle |
|-------|----------|-------------|-------------|
| Low | 100 | 14.5 KB | 148 bytes |
| Medium | 500 | 72.3 KB | 148 bytes |
| High | 1000 | 144.5 KB | 148 bytes |

---

## Key Constants

| Constant | Value | Description |
|----------|-------|-------------|
| `GREYHAWK_MAXSHIPS` | 500 | Maximum concurrent vessels |
| `MAX_SHIP_ROOMS` | 20 | Maximum interior rooms per vessel |
| `MAX_DOCKING_RANGE` | 2.0 | Maximum distance for docking |
| `BOARDING_DIFFICULTY` | 15 | DC for hostile boarding attempts |
| `SHIP_INTERIOR_VNUM_BASE` | 70000 | Start of interior room VNUMs |
| `SHIP_INTERIOR_VNUM_MAX` | 79999 | End of interior room VNUMs |

---

## Database Schema

### Tables (Auto-created at startup)

| Table | Primary Key | Purpose |
|-------|-------------|---------|
| `ship_interiors` | `ship_id VARCHAR(8)` | Vessel configuration, room data |
| `ship_docking` | `dock_id INT AUTO_INCREMENT` | Active/historical docking records |
| `ship_room_templates` | `template_id INT AUTO_INCREMENT` | 19 pre-configured room types |
| `ship_cargo_manifest` | `manifest_id INT AUTO_INCREMENT` | Cargo tracking (FK to ship_interiors) |
| `ship_crew_roster` | `crew_id INT AUTO_INCREMENT` | NPC crew assignments (FK to ship_interiors) |

### Room Templates (19 default types)

- **Control:** bridge, helm
- **Quarters:** quarters_captain, quarters_crew, quarters_officer
- **Cargo:** cargo_main, cargo_secure
- **Engineering:** engineering, weapons, armory
- **Common:** mess_hall, galley, infirmary
- **Connectivity:** corridor, deck_main, deck_lower
- **Special:** airlock, observation, brig

### Interior VNUM Allocation

```
Formula: 70000 + (ship_number * 20) + room_index
Range:   70000 - 79999 (reserved for vessel interiors, zones 700-799)
Maximum: 500 vessels * 20 rooms = 10,000 rooms
```

### Persistence Lifecycle

1. **Boot**: Ships initialized, then saved states loaded from database
2. **Create**: New ship interiors immediately saved to database
3. **Dock**: Docking records saved when ships dock
4. **Undock**: Docking records marked complete when ships undock
5. **Shutdown**: All vessel states saved before server terminates

---

## File Inventory

### Core Implementation

| File | Purpose |
|------|---------|
| `src/vessels.h` | Structures, constants, prototypes (includes vehicle definitions) |
| `src/vessels.c` | Core commands, wilderness movement, terrain system |
| `src/vessels_rooms.c` | Interior room generation and movement |
| `src/vessels_docking.c` | Docking, boarding, and ship-to-ship interaction |
| `src/vessels_db.c` | MySQL persistence layer |
| `src/vessels_autopilot.c` | Autopilot, waypoints, routes, NPC pilots, schedules |
| `src/vehicles.c` | Vehicle lifecycle, state management, persistence |
| `src/vehicles_commands.c` | Player commands (vmount, vdismount, drive, vstatus) |
| `src/vehicles_transport.c` | Vehicle-in-vessel mechanics (loading/unloading) |
| `src/transport_unified.c` | Unified transport interface across all transport types |
| `src/transport_unified.h` | Transport abstraction types and prototypes |
| `lib/text/help/vehicles.hlp` | Help file entries for vehicle commands |

### Database

| File | Purpose |
|------|---------|
| `src/db_init.c` | Table creation (init_vessel_system_tables) |
| `src/db_init_data.c` | Template population |
| `sql/components/vessels_phase2_schema.sql` | Manual schema script |
| `sql/components/vessels_phase2_rollback.sql` | Rollback script |
| `sql/components/verify_vessels_schema.sql` | Verification script |

### Legacy (Disabled)

| File | Purpose |
|------|---------|
| `src/vessels_src.c` | Old CWG/Outcast code (#if 0) |
| `src/vessels_src.h` | Old headers (#if 0) |

---

## Dependencies

### External Dependencies

- MySQL/MariaDB 5.7+ (required)
- Wilderness system operational
- Zone 213 test area configured

### Internal Dependencies

| File | Purpose |
|------|---------|
| `src/wilderness.c` | Coordinate system and room allocation |
| `src/weather.c` | Weather integration via `get_weather()` |
| `src/spec_procs.c` | ITEM_GREYHAWK_SHIP special procedure |
| `src/interpreter.c` | Command registration |
| `src/db.c` | Boot sequence integration |
| `src/dg_scripts.c/h` | Trigger integration for interior movement |
| `src/mysql.c` | Persistence layer (required) |

### Reserved Resources

- **VNUM Range 70000-79999:** Reserved for dynamic ship interior rooms (zones 700-799)
- **Item Type 56:** ITEM_GREYHAWK_SHIP
- **Room Flags:** ROOM_VEHICLE (40), ROOM_DOCKABLE (41)

### Test Zone (Zone 213)

| VNUM | Purpose |
|------|---------|
| Room 21300 | Dock room with DOCKABLE flag |
| Room 21398 | Ship interior (control room) |
| Room 21399 | Additional ship interior |
| Object 21300 | Test ship (ITEM_GREYHAWK_SHIP, boarding functional) |

---

## Troubleshooting

### Common Issues

**Issue: Foreign key constraint errors when inserting cargo/crew**
- **Cause:** `ship_interiors` record must exist first (parent table)
- **Solution:** Ensure vessel is saved to `ship_interiors` before adding cargo or crew records
- **Note:** CASCADE DELETE is implemented; deleting ship_interiors removes related records automatically

**Issue: Stored procedures not working**
- **Cause:** User lacks EXECUTE privilege
- **Solution:**
```sql
GRANT EXECUTE ON luminari_mudprod.* TO 'luminari_mud'@'localhost';
```

**Issue: Vessel movement fails silently**
- **Cause:** Destination wilderness coordinate has no allocated room
- **Solution:** Implement dynamic room allocation using `find_available_wilderness_room()` + `assign_wilderness_room()` pattern

**Issue: Phase 2 commands not recognized**
- **Cause:** Commands not registered in `interpreter.c`
- **Solution:** Add command entries for `dock`, `undock`, `board_hostile`, `look_outside`, `ship_rooms`

**Issue: Performance degradation with many vessels**
- **Diagnosis:** Check index usage:
```sql
EXPLAIN SELECT * FROM ship_docking WHERE ship1_id = '123';
```
- **Solution:** Verify indexes exist on frequently queried columns

---

## Verification & Monitoring

### Post-Deployment Verification

Run these queries to verify successful deployment:

```sql
-- Check all 5 tables exist
SELECT COUNT(*) as table_count FROM information_schema.TABLES
WHERE TABLE_SCHEMA = DATABASE() AND TABLE_NAME LIKE 'ship_%';
-- Expected: 5

-- Check room templates loaded
SELECT COUNT(*) as template_count FROM ship_room_templates;
-- Expected: 19

-- Check stored procedures exist
SHOW PROCEDURE STATUS WHERE Db = DATABASE()
AND Name IN ('cleanup_orphaned_dockings', 'get_active_dockings');
-- Expected: 2 procedures

-- Verify table structure
DESCRIBE ship_interiors;
DESCRIBE ship_docking;
```

### Ongoing Monitoring

```sql
-- Check table sizes periodically
SELECT TABLE_NAME, TABLE_ROWS, DATA_LENGTH/1024/1024 as size_mb
FROM information_schema.TABLES
WHERE TABLE_SCHEMA = DATABASE() AND TABLE_NAME LIKE 'ship_%';

-- Monitor active dockings
SELECT COUNT(*) as active_docks FROM ship_docking WHERE dock_status = 'active';
```

### Scheduled Maintenance

Run weekly to clean orphaned docking records:
```sql
CALL cleanup_orphaned_dockings();
```

---

## Risk Assessment

| Risk | Probability | Impact | Mitigation |
|------|-------------|--------|------------|
| Integration complexity | High | High | Incremental development, extensive testing |
| Performance degradation | Medium | High | Tiered complexity, feature toggles |
| Data migration issues | Medium | Medium | Phased migration, rollback scripts ready |
| Scope creep | Medium | Medium | Strict change control, clear priorities |
| Memory overhead | Medium | High | Ship pooling, bit fields, compressed coordinates |
| Data corruption | Low | Critical | Bounds checking, null guards, transaction safety |

---

## Known Issues

| Issue | Location | Description | Status |
|-------|----------|-------------|--------|
| Duplicate `disembark` registration | `interpreter.c:385` and `interpreter.c:1165` | Old and Greyhawk versions both registered; second (`do_greyhawk_disembark`) takes precedence | Working as intended |
| Room templates hard-coded | `vessels_rooms.c:27-104` | 10 hard-coded templates used instead of querying `ship_room_templates` table | Future enhancement |

---

## Deployment Notes

### Recommended Approach

Let the server auto-create vessel tables on first startup with MySQL available. This ensures schema matches runtime code exactly.

### Manual Deployment (Production)

1. Backup database: `mysqldump -u root -p luminari_mudprod > backup.sql`
2. Validate `sql/components/vessels_phase2_schema.sql` matches `init_vessel_system_tables()`
3. Execute schema script
4. Run verification script
5. Test basic vessel commands

### Rollback

Execute `sql/components/vessels_phase2_rollback.sql` to remove all vessel tables.

---

## Development

### Adding New Vessel Types

1. Add enum value to `vessel_class` in `vessels.h`
2. Add terrain capabilities to `vessel_terrain_data[]` in `vessels.c`
3. Add room generation rules to `get_rooms_for_vessel_type()` in `vessels_rooms.c`
4. Update help files

### Adding New Commands

1. Implement handler in `vessels.c` or `vessels_docking.c`
2. Register in `interpreter.c` under vessel command section
3. Add help entry in `lib/text/help/help.hlp`
4. Add tests in `unittests/CuTest/test_vessels.c`

### Adding New Vehicle Types

1. Add enum value to `vehicle_type` in `vessels.h`
2. Add terrain capabilities to default capability arrays
3. Add capacity constants (passengers, weight)
4. Add speed modifier constant
5. Update `vehicle_type_name()` in `vehicles.c`
6. Add help entry in `lib/text/help/vehicles.hlp`
7. Add tests in `unittests/CuTest/test_vehicle_structs.c`

### Testing

```bash
# Build and run vessel tests
cd unittests/CuTest
make all
make test

# Run vehicle tests
make phase02-tests

# Run stress tests
make stress

# Run with Valgrind
make valgrind
make valgrind-phase02
```

### Test Files

| Test File | Tests | Coverage |
|-----------|-------|----------|
| `test_vessels.c` | 91 | Core vessel system |
| `test_vessel_types.c` | 18 | Vessel type system |
| `test_vehicle_structs.c` | 19 | Enum values, struct sizes, constants |
| `test_vehicle_creation.c` | 27 | Lifecycle, state management, capacity |
| `test_vehicle_movement.c` | 45 | Direction, terrain, speed, movement |
| `test_vehicle_commands.c` | 31 | Player commands, parsing |
| `vehicle_transport_tests.c` | 14 | Vehicle-in-vessel mechanics |
| `test_transport_unified.c` | 15 | Unified transport interface |
| `vehicle_stress_test.c` | 8 | 100/500/1000 vehicle stress tests |
| **Total** | **353+** | 100% pass rate |

---

## Related Documentation

- [VESSEL_BENCHMARKS.md](VESSEL_BENCHMARKS.md) - Detailed performance data
- [TECHNICAL_DOCUMENTATION_MASTER_INDEX.md](TECHNICAL_DOCUMENTATION_MASTER_INDEX.md) - Complete docs index

---

*Generated as part of Phase 03, Session 06 - Final Testing and Documentation*
