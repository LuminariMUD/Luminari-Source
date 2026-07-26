# LuminariMUD Vessel System Documentation

**Version**: 2.5008-beta (Phases 04-09 gameplay layer, code-complete)
**Last Updated**: 2026-07-26
**Scope**: current behavior reference. For requirements and outstanding work see
[VESSEL_PRD_FINAL.md](../project-management-zusuk/vessels/VESSEL_PRD_FINAL.md); for what shipped when see
[docs/CHANGELOG.md](../CHANGELOG.md).

---

## Table of Contents

1. [System Overview](#system-overview)
2. [Architecture](#architecture)
3. [Data Structures](#data-structures)
4. [Vessel Types and Capabilities](#vessel-types-and-capabilities)
5. [API Reference](#api-reference)
6. [Player Commands](#player-commands)
7. [Integration Testing Workflows](#integration-testing-workflows)
8. [Vehicle-in-Vessel Mechanics](#vehicle-in-vessel-mechanics)
9. [Performance Characteristics](#performance-characteristics)
10. [Key Constants](#key-constants)
11. [Database Schema](#database-schema)
12. [File Inventory](#file-inventory)
13. [Dependencies](#dependencies)
14. [Troubleshooting](#troubleshooting)
15. [Operations](#operations)
16. [Risk Assessment](#risk-assessment)
17. [Known Issues](#known-issues)
18. [Development](#development)

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

- **Vessel** (`greyhawk_ship_data`): ~1016 bytes, max 500 = ~496KB
- **Autopilot** (`autopilot_data`): ~64 bytes (optional, attached to vessel)
- **Schedule** (`vessel_schedule`): ~32 bytes (optional, attached to vessel)
- **Vehicle** (`vehicle_data`): ~148 bytes, max 1000 = ~145KB

### Wilderness Coordinates

X/Y: -1024 to +1024; Z: altitude (airships) or depth (submarines)

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
    struct obj_data *shipobj; /* Associated ship object (critical for coord sync) */

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
    enum vessel_class vessel_type; /* Type of vessel */
    int num_rooms;            /* Room count (1-20) */
    int room_vnums[20];       /* Interior room VNUMs */
    int entrance_room;        /* Boarding point */
    int bridge_room;          /* Control room */

    /* Automation */
    struct autopilot_data *autopilot;
    struct vessel_schedule *schedule;
};
```

**Critical Linkages** (established during boarding in `spec_procs.c`):

| Linkage | Purpose |
|---------|---------|
| `world[room].ship = &greyhawk_ships[idx]` | Interior room -> Ship data (enables disembark, ship commands) |
| `greyhawk_ships[idx].shipobj = obj` | Ship data -> Ship object (enables coordinate sync to move object) |
| `GET_OBJ_VAL(obj, 1) = idx` | Ship object -> Ship index (stored in object file) |

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

| Type | Terrain | Speed | Rooms | Generated Room Types |
|------|---------|-------|-------|----------------------|
| RAFT | Rivers, shallow | Slow | 1-2 | Bridge |
| BOAT | Coastal | Moderate | 2-4 | Bridge, Quarters |
| SHIP | Ocean | Moderate | 3-8 | Bridge, Quarters, Cargo, Deck |
| WARSHIP | Ocean | Fast | 5-15 | Bridge, Armory, Weapons, Quarters, Brig |
| AIRSHIP | Air | Fast | 4-12 | Bridge, Observation, Engineering, Quarters |
| SUBMARINE | Underwater | Slow | 4-10 | Bridge, Helm, Engineering, Quarters |
| TRANSPORT | Ocean | Slow | 6-20 | Bridge, Large Cargo, Passenger Quarters |
| MAGICAL | Any | Variable | 1-5 | Custom configuration |

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

### Vehicle System

| Type | Capacity | Base Speed | Terrain |
|------|----------|------------|---------|
| `VEHICLE_CART` | 1 pass, 200 lbs | 80% | Road, plains |
| `VEHICLE_WAGON` | 4 pass, 1000 lbs | 60% | Road, plains, forest |
| `VEHICLE_MOUNT` | 1 pass, 100 lbs | 120% | Most terrain |
| `VEHICLE_CARRIAGE` | 6 pass, 500 lbs | 70% | Road, plains |

**States**: `IDLE`, `MOVING`, `LOADED`, `HITCHED`, `DAMAGED`, `ON_VESSEL`

**Terrain Flags**: `ROAD`, `PLAINS`, `FOREST`, `HILLS`, `MOUNTAIN`, `DESERT`, `WATER_SHALLOW`

**Speed Modifiers by Terrain**:

| Terrain | Cart | Wagon | Mount | Carriage |
|---------|------|-------|-------|----------|
| Road | 150% | 150% | 150% | 150% |
| Plains | 100% | 100% | 100% | 100% |
| Forest | 50% | 75% | 100% | 50% |
| Hills | 50% | 50% | 75% | 50% |
| Mountain | - | - | 50% | - |
| Swamp | - | - | 50% | - |

---

## API Reference

### Vessel Functions

```c
/* Lifecycle */
void vessel_init_all(void);                           // Initialize at boot
void load_vessels(void);                              // Load from database
void save_vessels(void);                              // Save to database
struct vessel_data *find_vessel_by_id(int id);        // Find by ID

/* Movement */
bool update_ship_wilderness_position(int ship, int x, int y, int z);
bool move_ship_wilderness(int ship, int dir, struct char_data *ch);
bool can_vessel_traverse_terrain(enum vessel_class type, int x, int y, int z);
int get_terrain_speed_modifier(enum vessel_class type, int sector, int weather);
```

### Cargo and Template Functions (Phase 04)

```c
int get_vessel_cargo_capacity(enum vessel_class type); /* per-class lbs; drives loadvehicle */
void load_ship_room_templates_from_db(void);           /* boot-time template overrides */
```

`route_save()`/`route_load()` are now real: they round-trip `struct ship_route`
through the ship_routes/ship_waypoints tables (create-or-update semantics,
idempotent waypoint replacement).

### Autopilot Functions

```c
struct autopilot_data *autopilot_init(struct greyhawk_ship_data *ship);
void autopilot_cleanup(struct greyhawk_ship_data *ship);
int autopilot_start(struct greyhawk_ship_data *ship, struct ship_route *route);
int autopilot_stop/pause/resume(struct greyhawk_ship_data *ship);
int waypoint_add(struct ship_route *route, float x, float y, float z, const char *name);
struct waypoint *waypoint_get_current(struct greyhawk_ship_data *ship);
struct ship_route *route_create(const char *name);
int route_save(struct ship_route *route);
void autopilot_tick(void);                            // Called each game tick
```

### Vehicle Functions

```c
struct vehicle_data *vehicle_create(enum vehicle_type type, const char *name);
void vehicle_destroy(struct vehicle_data *vehicle);
int vehicle_set_state(struct vehicle_data *v, enum vehicle_state state);
int vehicle_can_move/move(struct vehicle_data *v, int direction);
int vehicle_can_traverse_terrain(struct vehicle_data *v, int sector);
int vehicle_add/remove_passenger(struct vehicle_data *v);
int vehicle_damage/repair(struct vehicle_data *v, int amount);
```

### Persistence Functions

```c
void save_all_vessels(void);      void load_all_ship_interiors(void);
void save_all_waypoints(void);    void load_all_waypoints(void);
void save_all_routes(void);       void load_all_routes(void);
void save_all_schedules(void);    void load_all_schedules(void);
void vehicle_save_all(void);      void vehicle_load_all(void);
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

### Operator Commands (Phase 09)

| Command | Description | Usage |
|---------|-------------|-------|
| shiplist | Fleet overview + room pool health | `shiplist` |
| shipgoto | Teleport aboard a vessel | `shipgoto <slot>` |
| shipfix | Restore a vessel to full condition | `shipfix <slot>` |

`shiplist` reports wilderness dynamic room pool utilization and flags
PRESSURE past 80% - the pool is shared with every wilderness traveller, so
this is the guard against vessels starving other systems (PRD Section 4,
ground rule 3).

MSDP ship variables (`src/vessels_admin.c`, pushed on the vessel tick to
anyone aboard): `SHIP_NAME`, `SHIP_X`, `SHIP_Y`, `SHIP_Z`, `SHIP_HEADING`,
`SHIP_SPEED`, `SHIP_HULL`, `SHIP_HULL_MAX`, `SHIP_STATUS`. Clients can
render gauges without polling.

### Living World Commands (Phase 08)

| Command | Description | Usage |
|---------|-------------|-------|
| seastate | Weather, depth, visibility, hull state | `seastate` |

Hazards and encounters (`src/vessels_hazards.c`) read only wilderness
signals - no vessel-private geography:

- **Weather**: severity bands from `get_weather(x,y)` (the same field a
  coastal walker sees). Squall/storm/gale degrade rigging; a gale with no
  sailmaster aboard damages the hull. Submerged submarines are sheltered.
- **Crush depth**: submarines diving past the seabed depth at their
  coordinate (`get_modified_elevation()` vs `wild_waterline`) take damage.
- **Visibility**: `vessel_sight_range()` shrinks in fog, extended by a
  posted lookout.
- **Encounters**: `vessel_encounters` rows key to `REGION_ENCOUNTER`
  wilderness region vnums (authored with existing region tooling). Rows are
  filtered by depth band and hull class, so submarine trenches and airship
  skies get their own content. Warned by lookouts, spawned into the ship's
  wilderness room so they fight/flee/get shot like anything else.

### Cargo & Trade Commands (Phase 07)

| Command | Description | Usage |
|---------|-------------|-------|
| market | List a port's commodity prices | `market` |
| cargobuy | Load bulk goods (dock only) | `cargobuy <commodity> <qty>` |
| cargosell | Sell bulk goods (dock only) | `cargosell <commodity> [qty\|all]` |
| cargomanifest | Show bulk cargo aboard | `cargomanifest` |
| contracts | Freight board + your active jobs | `contracts` |
| contractaccept | Take a freight job (loads cargo) | `contractaccept <id>` |
| contractdeliver | Deliver at destination, collect | `contractdeliver <id>` |
| contractabandon | Return a job to the board | `contractabandon <id>` |
| plunder | Take cargo from a ship you've cleared | `plunder` |
| bounty | Check a price on someone's head | `bounty [<player>]` |
| marque | Buy a letter of marque (dock only) | `marque` |

Economy model (`src/vessels_trade.c`): commodities live in
`trade_commodities` (seeded with 9 goods, builder-editable); per-port stock
lives in `port_commodities`, seeded deterministically from the port vnum so
ports differ without randomness. Price = base scaled by scarcity, clamped to
+/- `TRADE_MAX_DRIFT` (60%) - the anti-arbitrage bound, unit-tested across
the whole supply domain. Buying drains local stock (price up), selling
floods it (price down); `vessel_trade_restock_tick()` drifts all ports back
toward baseline. Ports buy at 85% of ask, so same-port round trips lose
money. Bulk lots persist in `ship_cargo_manifest` with `cargo_room = 0`.

Freight contracts (`src/vessels_contracts.c`): each port's board offers runs
to other *known trading* ports (any with `port_commodities` rows), with
quantity and payout scaled from real wilderness distance between the dock
rooms. Accepting loads the cargo (capacity-checked) and claims the row with
a conditional UPDATE, so two captains racing for the same job cannot both
win it. Delivering requires the freight still aboard. Boards refresh on a
TTL; accepted contracts are never cleared by a refresh.

Piracy (`src/vessels_piracy.c`): `plunder` moves cargo from a cleared prize
into an alongside raider, unit by unit so the weight limit stops it exactly
at capacity. Unlawful plunder accrues bounty in `vessel_bounties`;
`vessel_port_refuses()` is called from every port-service gate (market,
freight, crew hall, shipyard, hull purchase), so a WANTED pirate cannot sell
what they steal. A letter of marque (`marque`) exempts the holder for a real
day and is refused to captains already WANTED.

### Ownership & Shipyard Commands (Phase 06)

| Command | Description | Usage |
|---------|-------------|-------|
| shipbrowse | Shipyard catalog with prices | `shipbrowse` |
| shipbuy | Buy a hull at a dock, become owner | `shipbuy <id>` |
| shipchristen | Owner: rename the ship | `shipchristen <name>` |
| shipdeed | Owner: transfer ownership | `shipdeed <player>` |
| shippermit / shiprevoke | Owner: manage helm clearances | `shippermit <player>` |
| shipcrew | List owner, pilot, permits, crew | `shipcrew` |
| shiphire / shipdismiss | Hire or release crew (dock only) | `shiphire <position> <tier>` |
| shipwages | Review and settle payroll | `shipwages` |
| shipupgrade | List/install refits (dock only) | `shipupgrade [<refit>]` |
| shipinsure | Buy sinking insurance (dock only) | `shipinsure <value>` |

Owned ships restrict the helm (`is_pilot()`) to owner + permits + immortals
(`src/vessels_ownership.c`). Owner persists in `ship_interiors.owner`
(auto-migrated); permits persist in `ship_crew_roster` (crew_role
'captain', npc_vnum -1). Capture via `claimship` transfers ownership and
voids old permits.

Crew (`src/vessels_crew.c`): four positions (sailmaster, gunner, bosun,
quartermaster) at three tiers (green/able/veteran). Bonuses are mirrored
into the legacy `sailcrew`/`guncrew` fields so movement, gunnery, and
repair consume them without special cases. Wages accrue on the vessel tick
(`vessel_crew_wage_tick()`); three unpaid paydays and a crew member walks.
Crew rows live in `ship_crew_roster` with npc_vnum <= -100.

Upgrades, wear, insurance (`src/vessels_upgrades.c`): four one-time refits
(plating, rigging, hold, reinforcement) raise hull ceilings at install
time; `vessel_upkeep_tick()` grinds armor and subsystems down while under
way (never below 1 structure per section); insurance pays the owner on
sinking via `vessel_pay_insurance()` from `vessel_sink()`.

### Naval Combat Commands (Phase 05)

| Command | Description | Usage |
|---------|-------------|-------|
| shipfire | Fire a weapon slot at another ship | `shipfire <slot> <target>` |
| shiprepair | Slow at-sea repairs (stationary only) | `shiprepair` |
| claimship | Capture from an uncontested bridge | `claimship` |

Combat model (`src/vessels_combat.c`): per-side armor absorbs, spill hits
section internal structure and bleeds through destroyed sections; fore hits
degrade rigging (mainsail -> speed), stern hits degrade the rudder
(turnrate); zero total structure sinks the ship (crew ejected to the water,
object becomes wreckage, fleet slot freed). Weapon arcs derive from
heading-relative bearing (`greyhawk_getarc()`), reloads tick on the
heartbeat (`vessel_combat_tick()`), and NPC-piloted ships return fire
automatically. Deep-draft hulls ground on real wilderness bathymetry
(elevation vs waterline against class `min_water_depth`).

### Builder Commands (Phase 04)

| Command | Description | Usage |
|---------|-------------|-------|
| vedit | Ship prototype editor (LVL_BUILDER) | `vedit list/new/show/set/delete/spawn` |

`vedit new <class 0-7> <name>` creates a prototype in `ship_prototypes` with
class defaults; `vedit set <id> name/class/speed/armor <value>` tunes it;
`vedit spawn <id>` instantiates a live, boardable ship at the builder's
location (free ship slot, generated interior, object linkage, immediate DB
persist). Interior room text comes from `ship_room_templates` rows (edit the
DB rows to change generated interiors - no recompile needed; compiled-in
fallbacks apply when MySQL is down).

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

## Performance Characteristics

### Memory Usage

| Component | Per-Unit | Maximum | Total |
|-----------|----------|---------|-------|
| Vessel | 1016 bytes | 500 | 496 KB |
| Vehicle | 148 bytes | 1000 | 145 KB |
| Waypoint | ~80 bytes | 1000 | 78 KB |
| Route | ~200 bytes | 200 | 39 KB |

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

> See [VESSEL_BENCHMARKS.md](../testing/VESSEL_BENCHMARKS.md) for stress test results and quality metrics.

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
| `ship_prototypes` | `prototype_id INT AUTO_INCREMENT` | Builder-authored hull definitions (vedit) |
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
| `src/vessels_edit.c` | vedit ship prototype editor, spawner, shipyard (Phase 04/06) |
| `src/vessels_combat.c` | Naval combat: damage, weapons, sinking, groundings (Phase 05) |
| `src/vessels_ownership.c` | Ownership, helm permits, deed transfer (Phase 06) |
| `src/vessels_crew.c` | Hired crew positions, tiers, wages (Phase 06) |
| `src/vessels_upgrades.c` | Refits, hull wear, insurance (Phase 06) |
| `src/vessels_trade.c` | Commodities, port pricing, bulk cargo (Phase 07) |
| `src/vessels_contracts.c` | Freight boards and contract lifecycle (Phase 07) |
| `src/vessels_piracy.c` | Plunder, bounty, letters of marque (Phase 07) |
| `src/vessels_hazards.c` | Weather hazards, encounters, seastate (Phase 08) |
| `src/vessels_admin.c` | Operator tooling, room pool monitor, MSDP (Phase 09) |
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
| `src/spec_procs.c` | Boarding special procedure (`greyhawk_ship_object`), establishes critical linkages |
| `src/interpreter.c` | Command registration |
| `src/db.c` | Boot sequence integration |
| `src/dg_scripts.c/h` | Trigger integration for interior movement |
| `src/mysql.c` | Persistence layer (required) |

### Reserved Resources

- **VNUM Range 70000-79999:** Reserved for dynamic ship interior rooms (zones 700-799)
- **Item Type 56:** ITEM_GREYHAWK_SHIP
- **Room Flags:** ROOM_VEHICLE (40), ROOM_DOCKABLE (41)

### Test Zones

**Zone 213** (Legacy test zone):

| VNUM | Purpose |
|------|---------|
| Room 21300 | Dock room with DOCKABLE flag |
| Room 21398 | Ship interior (control room) |
| Room 21399 | Additional ship interior |
| Object 21300 | Test ship (ITEM_GREYHAWK_SHIP, boarding functional) |

**Zone 700** (Current test zone - see [VESSEL_SYSTEM_TESTING.md](../testing/VESSEL_SYSTEM_TESTING.md)):

| VNUM | Purpose |
|------|---------|
| Object 70002 | Test vessel (ITEM_GREYHAWK_SHIP, ship_index=0) |
| Room 70003 | Test vessel interior room |
| Room 1000389 | Wilderness dock location at (-66, 92) |

---

## Troubleshooting

### Quick Reference

| Issue | Check First | Solution |
|-------|-------------|----------|
| Vessel not moving | Speed, dock status | `undock`, `speed 10`, `autopilot resume` |
| Cannot board | Room DOCKABLE flag | Move to dock room, check `entrance_room` |
| Interior nav fails | Room connections | `ship_rooms` to verify, regenerate if needed |
| Autopilot stuck | Route waypoints | `listwaypoints`, verify terrain reachable |
| Vehicle terrain blocked | Vehicle type | Use MOUNT for hills/mountains |
| Coordinate desync | shipobj linkage | `greyhawk_shipload` (admin), check spec_procs.c |
| Disembark fails | Interior room link | Verify `world[room].ship` is set |
| Ship object doesn't move | shipobj not linked | Set `greyhawk_ships[idx].shipobj = obj` |

### Database Issues

| Issue | Cause | Solution |
|-------|-------|----------|
| FK constraint errors | Parent record missing | Save to `ship_interiors` before cargo/crew |
| Stored procedures fail | Missing EXECUTE privilege | `GRANT EXECUTE ON luminari_mudprod.* TO 'luminari_mud'@'localhost';` |
| Silent movement fail | No wilderness room | Use `find_available_wilderness_room()` + `assign_wilderness_room()` |
| Performance degradation | Missing indexes | Check with `EXPLAIN SELECT ...` |

### Gameplay Issues Detail

**Vessel Not Moving**: Check docking (`shipstatus`), speed > 0, autopilot state, terrain compatibility

**Vehicle Loading**: Vessel must be docked/stationary, have cargo capacity, vehicle in cargo hold room

**NPC Pilot Issues**: Verify pilot in ship interior, check `pilot_mob_vnum`, reassign with `assignpilot`

**Schedule Issues**: Check `showschedule`, verify `SCHEDULE_FLAG_ENABLED`, route active

### Debug Logging

The whole vessel and vehicle stack is instrumented behind compile-time macros
declared in `src/vessels.h`. There is a master switch plus eight category
switches, so you can turn on just the subsystem you are chasing.

> **Production**: `VESSEL_SYSTEM_DEBUG` is currently **1** (dev). Set it to
> **0** before any production build - at 1 it logs every ship movement,
> terrain check, and speed calculation.

```c
/* src/vessels.h */
#define VESSEL_SYSTEM_DEBUG 1  /* master: 0 disables all vessel debug output */
```

| Category toggle | Covers |
|-----------------|--------|
| `VESSEL_DEBUG_CORE` | General vessel operations, interior generation |
| `VESSEL_DEBUG_MOVE` | Position updates, terrain checks, speed modifiers, blocked moves, room allocation |
| `VESSEL_DEBUG_AUTO` | Autopilot state transitions, tick summary, travel steps |
| `VESSEL_DEBUG_DOCK` | Docking, boarding, defender positioning |
| `VESSEL_DEBUG_DB` | Per-ship save/load persistence |
| `VEHICLE_DEBUG_CORE` | Vehicle operations, state transitions, damage |
| `VEHICLE_DEBUG_MOVE` | Vehicle movement and terrain verdicts |
| `VEHICLE_DEBUG_XPORT` | Vehicle-on-vessel transport, capacity checks |

Macros: `VSSL_DEBUG`, `VSSL_DEBUG_MOVE`, `VSSL_DEBUG_AUTO`, `VSSL_DEBUG_DOCK`,
`VSSL_DEBUG_DB`, `VHCL_DEBUG`, `VHCL_DEBUG_MOVE`, `VHCL_DEBUG_XPORT`, plus
function tracing (`VSSL_DEBUG_ENTER`, `VSSL_DEBUG_EXIT`, `VSSL_DEBUG_EXIT_VAL`)
and state transitions (`VSSL_DEBUG_STATE`).

Filter the syslog by prefix:

```bash
grep "\[VESSEL_MOVE\]"   syslog   # movement, terrain, groundings
grep "\[VESSEL_AUTO\]"   syslog   # autopilot
grep "\[VESSEL_DOCK\]"   syslog   # docking and boarding
grep "\[VESSEL_DB\]"     syslog   # persistence
grep "\[VESSEL_STATE\]"  syslog   # state transitions
grep "\[VEHICLE_XPORT\]" syslog   # vehicle loading
```

---

## Operations

### Deployment

**Recommended**: Let server auto-create tables on first startup with MySQL. Manual:
1. Backup: `mysqldump -u root -p luminari_mudprod > backup.sql`
2. Execute schema, run verification, test vessel commands
3. **Rollback**: `sql/components/vessels_phase2_rollback.sql`

### Verification Queries

```sql
SELECT COUNT(*) FROM information_schema.TABLES WHERE TABLE_NAME LIKE 'ship_%';  -- Expect: 5
SELECT COUNT(*) FROM ship_room_templates;  -- Expect: 19
SHOW PROCEDURE STATUS WHERE Name IN ('cleanup_orphaned_dockings', 'get_active_dockings');  -- Expect: 2
```

### Monitoring & Maintenance

```sql
-- Weekly: clean orphaned dockings
CALL cleanup_orphaned_dockings();
-- Check table sizes
SELECT TABLE_NAME, TABLE_ROWS FROM information_schema.TABLES WHERE TABLE_NAME LIKE 'ship_%';
```

---

## Risk Assessment

| Risk | Probability | Impact | Mitigation |
|------|-------------|--------|------------|
| Integration complexity | High | High | Incremental development, extensive testing |
| Performance degradation | Medium | High | Tiered complexity, feature toggles |
| Data migration | Medium | Medium | Phased migration, rollback scripts |
| Memory overhead | Medium | High | Ship pooling, bit fields |
| Data corruption | Low | Critical | Bounds checking, null guards, transactions |

---

## Known Issues

| Issue | Location | Status |
|-------|----------|--------|
| Duplicate `disembark` registration | `interpreter.c:385,1165` | Working as intended (Greyhawk takes precedence) |
| Hard-coded room templates | `vessels_rooms.c` | RESOLVED (Phase 04): DB overrides from `ship_room_templates` with compiled-in fallback |
| Generated interior rooms persist until reboot after ship purge | `vessels_rooms.c` | Known limitation; runtime reclamation is future work |

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

- [VESSEL_BENCHMARKS.md](../testing/VESSEL_BENCHMARKS.md) - Performance data and memory attribution
- [VESSEL_PRD_FINAL.md](../project-management-zusuk/vessels/VESSEL_PRD_FINAL.md) - Requirements and outstanding work
- [CHANGELOG.md](../CHANGELOG.md) - What shipped when
- [VESSEL_SYSTEM_TESTING.md](../testing/VESSEL_SYSTEM_TESTING.md) - 30-step manual regression script
- [TECHNICAL_DOCUMENTATION_MASTER_INDEX.md](../TECHNICAL_DOCUMENTATION_MASTER_INDEX.md) - Complete docs index

---

*Generated as part of Phase 03, Session 06 - Final Testing and Documentation*
