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
8. [Performance Characteristics](#performance-characteristics)

---

## System Overview

The LuminariMUD Vessel System provides a comprehensive transport framework for water-based vessels (ships, boats, submarines, airships) and land-based vehicles (carts, wagons, mounts, carriages). The system was developed across 4 phases totaling 28 sessions.

### Design Goals

- **Scalability**: Support 500+ concurrent vessels with <1KB memory per vessel
- **Flexibility**: Multiple vessel types with distinct terrain capabilities
- **Automation**: Autopilot, waypoint-based navigation, and NPC pilot support
- **Integration**: Seamless wilderness coordinate system integration
- **Unified Interface**: Common commands work across vessel and vehicle types

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

### Vehicle Types

| Type | Passengers | Weight | Speed | Terrain |
|------|------------|--------|-------|---------|
| CART | 2 | 500 lbs | 2 | Road, Plains |
| WAGON | 6 | 2000 lbs | 1 | Road, Plains |
| MOUNT | 1 | 200 lbs | 4 | Road, Plains, Forest, Hills |
| CARRIAGE | 4 | 800 lbs | 2 | Road only |

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

---

## Related Documentation

- [VESSEL_TROUBLESHOOTING.md](guides/VESSEL_TROUBLESHOOTING.md) - Common issues and solutions
- [VESSEL_BENCHMARKS.md](VESSEL_BENCHMARKS.md) - Detailed performance data
- [TECHNICAL_DOCUMENTATION_MASTER_INDEX.md](TECHNICAL_DOCUMENTATION_MASTER_INDEX.md) - Complete docs index

---

*Generated as part of Phase 03, Session 06 - Final Testing and Documentation*
