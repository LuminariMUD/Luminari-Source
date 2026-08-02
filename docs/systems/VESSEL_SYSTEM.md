# LuminariMUD Vessel System Documentation

**Release Status**: Gameplay layer, initial campaign shipping, initial
data/DG-driven derelict, wilderness frontier package, and Phase 16 showcase
events implemented; production acceptance incomplete
**Last Updated**: 2026-08-02
**Scope**: Current behavior reference. For the durable product contract see
[PRD.md](../PRD.md); for outstanding work see
[VESSELS_TODO.md](../project-management-zusuk/vessels/VESSELS_TODO.md); for what
shipped when see [CHANGELOG.md](../CHANGELOG.md).

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

The LuminariMUD Vessel System provides a transport and gameplay framework for
water vessels, submarines, airships, and land vehicles. It combines wilderness
navigation, multi-room interiors, automation, combat, ownership, crew, refits,
cargo, trade, piracy, hazards, encounters, showcase events, builder tooling,
and operator controls in one system.

### Design Goals

- **One world**: Vessels consume wilderness coordinates, terrain, bathymetry,
  weather, paths, regions, and dynamic rooms rather than duplicating them.
- **Meaningful ships**: Vessels are persistent possessions that can be named,
  crewed, upgraded, fought, captured, insured, and lost.
- **Multiplayer depth**: Solo operation works; specialized crew roles make group
  play stronger.
- **Builder control**: Hulls, interiors, ports, prices, routes, and encounters
  are data-driven.
- **Operational safety**: Support 500 vessels with observable, recoverable
  behavior and a tested release path.
- **Unified interface**: Common commands work across vessel and vehicle types.

### Two-Tier Transport Architecture

| Tier | Type | Memory | Interior | Use Case |
|------|------|--------|----------|----------|
| **Vessel** | Ships, airships, submarines | 4,928-byte base struct | Multi-room | Exploration, cargo, combat |
| **Vehicle** | Carts, wagons, mounts | 152-byte base struct | None | Land travel, cargo, transport |

### System Components

| Component | Description | Source Files |
|-----------|-------------|--------------|
| Core Vessels | Ship management, coordinates, movement | vessels.c, vessels.h |
| Autopilot | Waypoint navigation, route following | vessels_autopilot.c |
| Interior Rooms | Multi-room ship interiors | vessels_rooms.c |
| Docking | Ship-to-ship docking mechanics | vessels_docking.c |
| Persistence | Database save/load operations | vessels_db.c |
| Builder and Shipyard | Prototypes, spawning, hull purchase | vessels_edit.c |
| Combat | Damage, weapons, grounding, sinking | vessels_combat.c |
| Ownership and Crew | Owners, permits, hires, wages | vessels_ownership.c, vessels_crew.c |
| Upgrades | Refits, wear, insurance | vessels_upgrades.c |
| Economy | Cargo, markets, freight, piracy | vessels_trade.c, vessels_contracts.c, vessels_piracy.c |
| NPC Merchant Fleet | Durable definitions, assembly, consequences, respawn | vessels_merchants.c |
| Bounty Hunters | HUNTED encounter policy, pursuit, durable lifecycle | vessels_hunters.c |
| Living World | Weather hazards and region encounters | vessels_hazards.c |
| Operations | Fleet tools, room-pool monitoring, MSDP | vessels_admin.c |
| Vehicles | Land-based transport | vehicles.c |
| Vehicle Commands | Player vehicle interactions | vehicles_commands.c |
| Vehicle Transport | Vehicle-on-vessel mechanics | vehicles_transport.c |

---

## Architecture

### Memory Layout

- **Vessel** (`greyhawk_ship_data`): 4,928 bytes, max 500 = about 2.35 MiB
- **Autopilot** (`autopilot_data`): 72 bytes (optional, attached to vessel)
- **Schedule** (`vessel_schedule`): ~32 bytes (optional, attached to vessel)
- **Vehicle** (`vehicle_data`): 152 bytes, max 1000 = about 148 KB

### Wilderness Coordinates

X/Y: -1024 to +1024; Z: altitude (airships) or depth (submarines)

The class Z contract is enforced before wilderness-room allocation. Surface
hulls remain at Z 0; air-capable hulls may rise only to their configured
ceiling; submersible hulls may use negative Z only in a water column. Submarine
crush depth remains anchored to local bathymetry instead of a fixed class
floor. Autopilot advances along X, Y, and Z together, clamps each step to the
remaining three-dimensional distance, and rejects an invalid waypoint Z before
moving toward it. Automated movement resolves and validates its target dynamic
room once inside `update_ship_wilderness_position()`; it does not run the
allocating `can_vessel_traverse_terrain()` probe immediately beforehand.
If that central move rejects terrain or Z, autopilot stops the hull, enters
`PAUSED`, persists the runtime state, and tells occupants which waypoint is
unreachable. It does not retry the same invalid step every heartbeat. Correct
the route, set the desired speed, and resume autopilot.

### Wilderness Integration Contract

Vessels extend the wilderness system; they do not create a separate geography.

| Wilderness signal | Vessel behavior |
|-------------------|-----------------|
| Dynamic room pool | Characters and exterior hulls keep their coordinate room occupied; co-located hulls share it |
| Generated sector | The central position update and direct-movement preflight gate traversal; speed rules consume the resulting sector |
| Bathymetry | Draft, grounding, and submarine crush depth |
| Weather field | Speed, visibility, helm risk, and storm damage |
| `REGION_ENCOUNTER` | Builder-authored encounter selection |
| Sector regions | Magical or transformed waters through the generated sector |
| Paths | Roads for vehicles; `PATH_RIVER` digitalizes canonical River travel cells for rafts and boats |
| Geographic regions | Canonical source for named seas and territorial waters |
| Bathymetric regions | Thresholded natural-depth trenches reported through `seastate` |
| Altitude-lane regions | Thresholded high currents that multiply eligible airship speed by 125 percent |
| Sky-island regions | Thresholded aerial destinations reported only inside their polygon and at altitude |

Permanent invariants:

1. Add missing environmental signals to wilderness first, then consume them
   from vessel code.
2. Author geographic names, legal waters, trade lanes, and encounter areas as
   regions rather than coordinate literals.
3. Treat the 6,000-room wilderness dynamic pool as shared infrastructure.
   `shiplist` reports utilization and flags pressure above 80%.
4. Anchor depth to bathymetry and altitude content to wilderness regions at the
   same `(x, y)` coordinate.
5. Keep core integration campaign-neutral and setting content in world or
   database data.
6. Treat X/Y as authoritative for a wilderness hull. A saved wilderness room
   VNUM may be recycled; recovery resolves the current room from coordinates
   and repairs the runtime snapshot.
7. Zone resets may remove stale hull objects, but never a hull currently owned
   by an active fleet slot.

Frontier feature regions use campaign-neutral types in `wilderness.h`.
`REGION_BATHYMETRIC` (5) treats `region_props` as the minimum natural water
column (`wild_waterline - elevation`). `REGION_ALTITUDE_LANE` (6) and
`REGION_SKY_ISLAND` (7) treat it as the minimum vessel Z. The resolver reads
the canonical in-memory polygons, rejects failed thresholds, and chooses the
lowest VNUM when equal types overlap. An altitude lane applies only to
airships and magical vessels; its multiplier remains capped by the normal
150-percent speed ceiling. `seastate` exposes each active feature and its
threshold. Builders can use `reglist type 5`, `reglist type 6`, `reglist type
7`, and `pathlist type 5` without paging unrelated records.

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
    |       +-- Multi-Room Interiors (VNUM Range: 70020-80019)
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
    uint64_t movement_steps;      /* Successful autonomous position updates */
    uint64_t waypoint_arrivals;
    uint64_t route_completions;
};
```

The three counters are monotonic for the lifetime of the in-memory autopilot
and reset at process reconstruction. `autopilot status` exposes them so
operators and soak monitors can prove route progress without enabling
per-movement logging.

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
| shiptalk | Speak across all rooms of the current vessel | `shiptalk <message>` |
| greyhawk_speed | Set ship speed | `speed <0-30>` |
| greyhawk_heading | Set ship heading | `heading <0-360>` |
| dock | Dock with vessel | `dock <ship>` |
| undock | Undock from vessel | `undock` |
| look_outside | View from interior | `lookout` |

System-generated vessel messages use independent per-vessel cooldown classes.
Repeated depth and weather messages are limited to one copy per class every
120 seconds; a change from squall to storm or gale remains immediately
visible. High-volume damage, NPC return-fire, miss, and reload messages are
limited to one copy per class per half-second vessel tick. Sinking, grounding,
rigging-collapse, and rudder-loss warnings remain immediate. Suppressed copies
increment the process-wide `vessel_messages_throttled` performance counter.

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

### Operator Commands (Phases 09, 14, 15, and 16)

| Command | Description | Usage |
|---------|-------------|-------|
| shiplist | Fleet overview + room pool health | `shiplist [summary]` |
| shipgoto | Teleport aboard a vessel | `shipgoto <slot>` |
| shipfix | Restore a vessel to full condition | `shipfix <slot>` |
| vmerchant | Inspect or reconcile NPC merchants; force a confirmed loss | `vmerchant [list\|sync\|sink <id> confirm]` |
| vesseldebug | Inspect debug state or advance the normal encounter cadence | `vesseldebug [status\|on ...\|off ...\|encounter]` |
| vevent | Start, enlist, end, cancel, or recover a showcase event | `vevent <action>` |

`shiplist` reports wilderness dynamic room pool utilization and flags
PRESSURE past 80% - the pool is shared with every wilderness traveller, so
this is the guard against vessels starving other systems. At fleet scale,
`shiplist summary` omits per-vessel rows so the count and pool warning fit in
one socket output buffer (see
[Wilderness Integration Contract](#wilderness-integration-contract)).
`shipfix` commits the repaired condition before reporting success. If that
runtime write fails, it restores the prior condition instead of presenting a
RAM-only repair.

Native MSDP is the vessel client contract for this release. A client enables
Telnet option 69 and uses `REPORT` for `SHIP_NAME`, `SHIP_X`, `SHIP_Y`,
`SHIP_Z`, `SHIP_HEADING`, `SHIP_SPEED`, `SHIP_HULL`, `SHIP_HULL_MAX`, and
`SHIP_STATUS`. `src/vessels_admin.c` refreshes them on the vessel tick, and
the normal MSDP update sends each reported value when it changes. Clients can
render gauges without polling.

When a character leaves a vessel, the server sends an explicit empty state:
the two strings become empty and all seven numbers become zero. This prevents
a client from continuing to display a stale vessel. `SHIP_HULL` and
`SHIP_HULL_MAX` are the sums of the four internal-structure sections, while
`SHIP_STATUS` is `sound`, `battered`, `crippled`, or `sinking`.

The general protocol layer contains experimental GMCP support, but it is not
an equivalent vessel interface and is not part of this release contract.
Future GMCP vessel work must define and test a valid JSON package or
standards-compliant MSDP-over-GMCP mapping; the old unquoted
`MSDP.<variable> <value>` fallback is not accepted as vessel support.

`vesseldebug encounter` is an acceptance hook, not a parallel spawner. It
advances the cadence counter and immediately invokes the same production
region, class, depth, chance, HUNTED eligibility, spawn, and lifecycle path
used by the heartbeat. Staff can use it in a normal build even though runtime
debug categories remain compiled out.

### Living World Commands (Phase 08)

| Command | Description | Usage |
|---------|-------------|-------|
| seastate | Weather, depth, visibility, hull state | `seastate` |

Hazards and encounters (`src/vessels_hazards.c`) read only wilderness
signals - no vessel-private geography:

- **Weather**: severity bands from `get_weather(x,y)` (the same field a
  coastal walker sees). Squall/storm/gale degrade rigging; a gale with neither
  a sailmaster nor the assigned pilot at the bridge damages the hull.
  Submerged submarines are sheltered.
- **Crush depth**: submarines diving past the seabed depth at their
  coordinate (`get_modified_elevation()` vs `wild_waterline`) take damage.
- **Visibility**: `vessel_sight_range()` shrinks in fog, extended by a
  posted lookout.
- **Encounters**: `vessel_encounters` rows key to `REGION_ENCOUNTER`
  wilderness region vnums (authored with existing region tooling). Rows are
  filtered by depth band and hull class, so submarine trenches and airship
  skies get their own content. Warned by lookouts, spawned into the ship's
  wilderness room so they fight/flee/get shot like anything else. Overlapping
  encounter regions resolve deterministically by containment position, then
  lowest region VNUM; equal-chance table rows resolve by encounter ID. When
  multiple hulls share one exterior wilderness room, a successful tick claims
  that room once, broadcasts the encounter to every co-located hull, and spawns
  at most one shared creature there. A bounded 1,024-row definition cache,
  loaded after schema boot, removes candidate and hunter-policy SQL from the
  recurring heartbeat. A staff-forced encounter reloads the cache first so
  development data can be iterated without a reboot.

Phase 15 hunter policy (`src/vessels_hunters.c`) optionally extends one of
those ordinary encounter rows. A target must be a moving, player-owned hull
whose exact owner is online aboard and currently has at least the configured
HUNTED bounty (never below 2,000). An atomic one-row-per-player lifecycle
claims the generation before an ownerless public warship is assembled through
the normal prototype/interior persistence path and assigned its real pilot.
The hunter uses the production autopilot position resolver to pursue the
target and the existing NPC combat path to open fire.

The lifecycle persists target ship, hunter slot, unique generation name,
expiry, cooldown, and terminal reason. Boot reattaches only an exact
name/prototype/slot/pilot match, preventing a recycled fleet slot from becoming
the hunter. Pardon is checked every 10 seconds; target logout or leaving the
hull starts the configured grace period. Pardon, expiry, grace, sinking, or
staff purge removes the hunter and all of its normal persistence. Capture
removes the Admiralty pilot and lifecycle but leaves the captured hull as an
ordinary player vessel.

### Showcase Event Commands (Phase 16)

| Command | Description | Usage |
|---------|-------------|-------|
| vevent status | Show the active event, participants, scores, and objectives | `vevent status` |
| vevent join | Enter the current vessel; a skirmish requires a team | `vevent join [red\|blue]` |
| vevent leaderboard | Show durable event rankings | `vevent leaderboard [regatta\|skirmish\|ghost]` |
| vevent start | Staff: open a regatta, skirmish, or ghost fleet | `vevent start <type> ...` |
| vevent enlist | Staff: add another fleet hull to a skirmish | `vevent enlist <ship-slot> <red\|blue>` |
| vevent end/cancel/recover | Staff: score, discard, or recover event state | `vevent <end\|cancel\|recover>` |

Only one showcase event may be open. A regatta begins at the staff member's
current wilderness coordinate and records a finish only when an entered hull
moves onto the exact finish coordinate. Placement awards 100 points for first,
90 for second, down to a minimum of 10. A skirmish awards the attacking fleet
one point per live damage point and 100 more for a sink; every participant on
the higher-scoring team receives a win. A ghost event creates one to five
public warships at the staff coordinate through the normal prototype,
interior, hull, weapon, and runtime persistence path. Damage and sinks score
against those contacts, and the unique highest-scoring captain wins.

Completion updates every leaderboard row and the terminal event status in one
database transaction. A failed cleanup or score commit keeps the event in
`recovery_failed` and blocks another start instead of repeating work each
tick. Every event has a one-hour ceiling. Events do not resume after process
restart: boot retires tracked ghost hulls and closes interrupted rows as
`recovered`; a cleanup failure remains explicit for `vevent recover`. Captain
IDs are gameplay player-file IDs, and leaderboard display resolves the current
name through the authoritative player index rather than the unrelated
`player_data.player_idnum` key.

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
| dockfees | Inspect or pay the current berth charge | `dockfees [pay]` |

Economy model (`src/vessels_trade.c`): commodities live in
`trade_commodities` (seeded with 9 goods, builder-editable); per-port stock
lives in `port_commodities`, seeded deterministically from the port vnum so
ports differ without randomness. Price = base scaled by scarcity, clamped to
+/- `TRADE_MAX_DRIFT` (60%) - the anti-arbitrage bound, unit-tested across
the whole supply domain. A batch is priced one unit at a time across every
supply level it moves through; quoting the whole batch at its first unit's
price would let an oversized shipment flip two markets and profit again in
reverse. Buying drains local stock (price up), selling floods it (price down);
inventory is clamped to 10-400, and `vessel_trade_restock_tick()` drifts all
ports back toward baseline. Ports buy at 85% of ask, so same-port round trips
lose money. Bulk lots persist in `ship_cargo_manifest` with
`cargo_room = 0`.

Staff can run `vtradecheck 1000` to execute the deterministic sustained-market
gate without changing live port or character state. It must report all 1,000
adversarial transfers inside the supply bounds, finite convergence of a real
profit gradient, non-positive oversized reversal profit, and restocking to
the 100-unit baseline.

Owned vessels receive one class-based dock fee on arrival at a port. Repeated
room updates within the same visit do not assess another fee. An unpaid balance
blocks manual departure and pauses autopilot; `dockfees pay` is limited to the
owner or a permitted helmsman and saves both vessel and player state before
confirming payment. Revenue assessed at a clan-owned port goes to that clan
even if control changes before settlement. Public-port revenue leaves the
economy. Unowned NPC and test hulls are exempt so public ferries cannot strand
themselves. Departure clears and persists berth state only when an actual fee
port or clan marker exists, avoiding false writes for public vessels.

Scheduled public vessels may set a 0-100,000-gold passenger fare with
`setschedule <route> <interval> [fare]`. Boarding collects and saves the fare
before moving the character; insufficient gold or a failed player save leaves
both the character and balance ashore. NPC crew are exempt. Privately owned
vessels do not collect this automatic fee because owner revenue and
player-to-player settlement are outside the public-ferry contract. The fare
lives in `ship_schedules`, appears in `showschedule`, and survives reboot.

Freight contracts (`src/vessels_contracts.c`): each port's board offers runs
to other *known trading* ports (any with `port_commodities` rows), with
quantity and payout scaled from real wilderness distance between the dock
rooms. Accepting loads the cargo (capacity-checked) and claims the row with
a conditional UPDATE, so two captains racing for the same job cannot both
win it. Delivering requires the freight still aboard. Boards refresh on a
TTL; accepted contracts are never cleared by a refresh.

Piracy (`src/vessels_piracy.c`): `plunder` moves cargo from a cleared prize
into an alongside raider, unit by unit so the weight limit stops it exactly
at capacity. Unlawful plunder accrues bounty in `vessel_bounties`. By default,
the rate is 15 gold per cargo unit. `vessel_region_law` may attach a 0-500%
multiplier, authority, water type, and overlap priority to a builder-authored
`REGION_GEOGRAPHIC` VNUM. At boot, law rows are cached and resolved against
the canonical wilderness polygons already loaded from
`region_data`/`region_index`; movement never runs a region query or creates a
vessel-private coordinate table. `reload regions` refreshes both sources.
`seastate` exposes the resolved named waters, authority, and rate. A vessel
announces a real named-water boundary crossing ship-wide and remembers its
current region so continued movement inside that polygon stays quiet. A
pirate-cove port permits WANTED captains;
`vessel_port_refuses()` remains active at every other port-service gate
(market, freight, crew hall, shipyard, hull purchase), so a WANTED pirate
cannot sell elsewhere. A letter of marque (`marque`) exempts the holder from
positive regional bounties for one real day and is refused to captains already
WANTED.

NPC merchant shipping (`src/vessels_merchants.c`) is definition-driven rather
than a special immortal hull. Each enabled `vessel_npc_merchants` row names a
builder prototype, route, pilot mobile, spawn coordinate, faction, commodity,
quantity, schedule interval, and bounded respawn delay. Boot and each MUD-hour
schedule tick reconcile those definitions with the ordinary public hulls.
Assembly uses the normal hull/interior persistence path, loads real bulk cargo,
assigns the configured pilot, creates the schedule, and departs on the real
route. A missing, captured, sunk, or staff-purged hull releases the definition;
after its delay, the next reconciliation creates a fresh generation.

Firing on a merchant records one fixed faction loss per player and generation.
Plunder adds a cargo-scaled loss and the ordinary regional bounty. Capture or
sinking adds a total-loss penalty and a bounty calculated from at least 34
cargo units. The most recent attacker is responsible for an otherwise
unattributed loss only for 300 seconds, so later weather or terrain damage is
not charged to an old attacker. `vessel_merchant_consequences` deduplicates
attack and loss events. Bounties commit with the event; faction losses are
saved to an online player immediately or delivered at the next login. A saved
per-player high-water mark closes an interrupted delivery without applying it
twice. Character rename updates active and pending merchant records, while
permanent removal voids pending rows, clears current attribution, and deletes
the removed name's bounty.

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

Soft-deleted characters retain their deeds so staff restoration is lossless.
Before permanent player-file removal, one transaction makes their ships
unowned, removes their helm permits, voids pending insurance and merchant
consequences, clears current merchant-attack attribution, and removes their
vessel bounty and durable hunter lifecycle. Any matching live hunter is then
retired through the normal vessel cleanup path. If that transaction cannot
commit, player removal is deferred instead of orphaning property.

Crew (`src/vessels_crew.c`): four positions (sailmaster, gunner, bosun,
quartermaster) at three tiers (green/able/veteran). Bonuses are mirrored
into the legacy `sailcrew`/`guncrew` fields so movement, gunnery, and
repair consume them without special cases. Wages accrue on the vessel tick
(`vessel_crew_wage_tick()`); three unpaid paydays and a crew member walks.
Crew rows live in `ship_crew_roster` with npc_vnum <= -100.

Upgrades, wear, insurance (`src/vessels_upgrades.c`): four one-time refits
(plating, rigging, hold, reinforcement) raise hull ceilings at install
time; `vessel_upkeep_tick()` grinds armor and subsystems down while under
way (never below 1 structure per section). Sinking consumes the policy and
creates one durable `vessel_insurance_claims` row plus a system-mail receipt in
the same settlement flow. Online owners receive the gold immediately; offline
owners receive pending settlements on their next login. A player-file
high-water mark prevents duplicate credit if recovery occurs between saving
the character and closing the database claim.

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

Every player-driven hostile entry point uses `vessel_pvp_permitted()`. A
consented engagement records a persisted, opponent-specific five-minute
window. If an owner logs out, only the original still-PvP-enabled aggressor may
continue during that window; other players and expired snapshots fail closed.
Ownership changes and permanent owner removal clear inherited consent.

### Builder Commands (Phase 04)

| Command | Description | Usage |
|---------|-------------|-------|
| vedit | Ship prototype editor (LVL_BUILDER) | `vedit list/new/show/set/delete/spawn/spawnpublic` |

`vedit new <class 0-7> <name>` creates a prototype in `ship_prototypes` with
class defaults; `vedit set <id> name/class/speed/armor <value>` tunes it;
`vedit spawn <id>` instantiates a live, boardable ship at the builder's
location and assigns the builder as owner. `vedit spawnpublic <id>` uses the
same atomic spawn path but leaves the ship unclaimed for an NPC or public
route, so it has no player-owner dock fees. Both paths allocate a free slot,
generate the interior, link the exterior object, and persist the complete
instance before reporting success.

On local development, the complete builder gate is:

```bash
./scripts/dev_kohdee_login_smoke.sh --vessel-builder-check
```

It uses one actual Kohdee session, parses the generated IDs from in-game
output, verifies a one-cell sail, and removes the disposable hull and
prototype. The July 30, 2026 run took 2.7 seconds in game and 8 seconds
including login and clean account logout, well inside the 15-minute builder
independence budget.

Interior room text comes from `ship_room_templates`. DG trigger attachments
come from `ship_room_template_triggers`, keyed by generated room type. Changes
to either table take effect on the next boot; compiled-in room templates
remain the MySQL-unavailable fallback.

Generated-room trigger mappings are shared by room type, so content-specific
DG programs must prove that the generated room belongs to their intended hull
before changing player state. Blackwake's bridge trigger checks the bridge
name directly; its quarters and cargo triggers resolve the linked bridge and
check that identity. This keeps the globally mapped VNUMs inert on unrelated
ship-class interiors.

### NPC Pilot Commands

| Command | Description | Usage |
|---------|-------------|-------|
| assignpilot | Assign NPC pilot | `assignpilot <npc>` |
| unassignpilot | Remove NPC pilot | `unassignpilot` |

### Schedule Commands

| Command | Description | Usage |
|---------|-------------|-------|
| setschedule | Set schedule and optional public fare | `setschedule <route> <interval> [fare]` |
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
6. If terrain rejects a step, correct the route, set speed, and resume
7. Optional: Assign NPC pilot for announcements

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

| Component | Per unit | Maximum | Base total |
|-----------|----------|---------|------------|
| Vessel | 4,928 bytes | 500 | About 2.35 MiB |
| Vehicle | 152 bytes | 1,000 | About 148 KB |
| Autopilot | 72 bytes | Optional per vessel | Up to about 36 KB |
| Schedule | About 32 bytes | Optional per vessel | Up to about 16 KB |

### Structure Sizes

| Structure | Size |
|-----------|------|
| `struct greyhawk_ship_data` | 4,928 bytes |
| `struct vehicle_data` | 152 bytes |
| `struct waypoint` | 88 bytes |
| `struct ship_route` | 1840 bytes |
| `struct autopilot_data` | 72 bytes |
| `struct waypoint_node` | 104 bytes |
| `struct transport_data` | 16 bytes |

The release budget is no more than 5 KB for the base ship structure, about
3 MB for the maximum base fleet, and 25 ms per game tick for all vessel
subsystems at 500 ships. Both budgets pass on local development. The required
1,862-second run sustained 500 vessels across all eight classes for 3,655
complete ticks: median 599 usec, p95 4,079 usec, p99 5,169.06 usec, and maximum
10,520 usec. Every measured subsystem maximum stayed below 25 ms. This accepts
the development performance gate, not production rollout or long-horizon
memory behavior. Phase 16 subsequently added the bounded `vessel_events`
profiler section, so final preflight must repeat the full scale gate on the
release candidate.

See [VESSEL_BENCHMARKS.md](../testing/VESSEL_BENCHMARKS.md) for attribution,
historical measurements, and the limits of the current evidence.

---

## Key Constants

| Constant | Value | Description |
|----------|-------|-------------|
| `GREYHAWK_MAXSHIPS` | 501 | Fleet-slot array entries, including reserved slot 0 |
| `GREYHAWK_ACTIVE_SHIP_CAPACITY` | 500 | Maximum concurrent active vessels |
| `MAX_SHIP_ROOMS` | 20 | Maximum interior rooms per vessel |
| `MAX_DOCKING_RANGE` | 2.0 | Maximum distance for docking |
| `BOARDING_DIFFICULTY` | 15 | DC for hostile boarding attempts |
| `SHIP_INTERIOR_VNUM_BASE` | 70000 | Start of interior room VNUMs |
| `SHIP_INTERIOR_VNUM_MAX` | 80019 | End of interior room VNUMs |

---

## Database Schema

### Tables (Auto-created at startup)

| Table | Purpose |
|-------|---------|
| `ship_prototypes` | Builder-authored hull definitions used by `vedit` and shipyards |
| `ship_interiors` | Vessel identity, rooms, owner, upgrades, insurance, and wage state |
| `ship_runtime_state` | Live hull, position, condition, room type, autopilot, PvP grace, and dock-fee snapshot |
| `ship_weapons` | Normalized installed weapon slots, values, position, and reload state |
| `ship_docking` | Active and historical docking relationships |
| `ship_room_templates` | Builder-editable generated interior text |
| `ship_room_template_triggers` | DG trigger VNUMs attached to generated room types |
| `ship_cargo_manifest` | Object cargo and bulk commodity lots |
| `ship_crew_roster` | Hired crew and helm permits |
| `ship_waypoints` | Persistent named navigation points |
| `ship_routes` | Persistent route identities |
| `ship_route_waypoints` | Ordered waypoint membership for routes |
| `ship_schedules` | NPC-pilot and ferry schedule state, including passenger fare |
| `trade_commodities` | Commodity definitions and base values |
| `port_commodities` | Per-port supply and local price state |
| `freight_contracts` | Freight offer and acceptance lifecycle |
| `vessel_bounties` | Piracy bounty and marque state |
| `vessel_region_law` | Legal-water metadata keyed to canonical geographic regions |
| `vessel_encounters` | Region-keyed encounter definitions |
| `vessel_insurance_claims` | Pending, paid, or void offline insurance settlements |
| `vessel_npc_merchants` | NPC merchant prototype, route, cargo, faction, schedule, and live generation |
| `vessel_merchant_consequences` | Deduplicated faction and bounty events with delivery state |
| `vessel_hunter_encounters` | Hunter warship, pilot, bounty, pursuit, duration, grace, and cooldown policy |
| `vessel_bounty_hunts` | One durable hunter generation and terminal cooldown per target player |
| `vessel_showcase_events` | Event type, course, staff owner, lifecycle, timing, and terminal reason |
| `vessel_event_participants` | Per-event hull, captain, team, score, finish, placement, and status |
| `vessel_event_leaderboards` | Durable entries, wins, points, and best regatta time per captain and type |
| `vessel_event_runtimes` | Temporary ghost-hull ownership used by cleanup and boot recovery |

### Room Templates (19 default types)

- **Control:** bridge, helm
- **Quarters:** quarters_captain, quarters_crew, quarters_officer
- **Cargo:** cargo_main, cargo_secure
- **Engineering:** engineering, weapons, armory
- **Common:** mess_hall, galley, infirmary
- **Connectivity:** corridor, deck_main, deck_lower
- **Special:** airlock, observation, brig

### Shared Development Harbor

The tracked source package in `lib/world/vessel_harbor/` and the explicit
development provisioner create the reusable harbor validation environment:

```bash
make install
./scripts/provision_vessel_harbor.sh
```

The command refuses to run unless `lib/.env` contains
`APP_ENV=development`. It merges only missing records into the ignored live
world files, extends the reserved zone 700 upper bound from 79999 to 80019
when needed, applies Phases 11-15 and the development seed, restarts the
supervised local MUD, creates the ferry only when absent, and verifies the
result through batched Kohdee sessions. It rejects conflicting zone or legal
water region reservations instead of overwriting them. It is intentionally not
part of `make install`, `setup.sh`, or `deploy.sh`.

The environment contains Testing Dock at room 1000389 and `(-66, 92)`, Harbor
Sandbox East Dock at room 1000390 and `(-62, 82)`, representative raft/ship/
airship prototypes, the looping `harbor_ferry_loop`, a public ship-class
ferry with a 10-gold fare, mobile 70001 as its persistent pilot, and
bridge/cargo DG diagnostics 70001/70002. Three development-only geographic
regions demonstrate territorial waters (150% bounty), nested free seas (100%),
and a pirate cove (0%) without replacing wilderness geometry. The same route
also drives `Harbor Sandbox Merchant`, a faction-1 public hull carrying 25
units of spice under the fixture pilot. The provisioner requires its durable
definition, positive generation, live hull, real cargo, pilot, enabled
schedule, route, in-game registry row, and ship-status identity.
The Phase 15 fixture adds `Harbor Sandbox Hunted Raft`, an Admiralty warship,
captain mobile 70002, encounter region 7000004, and a deterministic
raft-target policy. The provisioner validates the region, both prototypes,
warship class, pilot, HUNTED threshold, pursuit bounds, and lifecycle tables;
it does not create a hunt or alter Kohdee's bounty.

Re-running the command reuses the same ferry, merchant definition, and account
rather than duplicating any of them. An assigned pilot at the bridge is
excluded from ordinary mobile wandering; the fixture ferrymaster is also
authored Sentinel so it remains at its duty station. The two docks are joined
by four ordered route entries: west dock, channel turn at `(-64, 82)`, east
dock, and the same channel turn for the return leg. This keeps both
straight-line legs off the Beach cells. After a hard restart, the provisioner
checks the restored fare and named legal waters, boards as Kohdee through the
ordinary hull-object path, proves exactly 10 gold was collected, restores
Kohdee's starting gold, resumes the ferry, and validates the exact route
topology.

### Initial Luminari Campaign Shipping

The tracked campaign package uses existing Luminari wilderness content rather
than the development harbor fixture:

```bash
./scripts/provision_vessel_campaign.sh
```

It anchors North Vailand Sea Port 1000360 at `(-599, 455)` and Central Vailand
Sea Port 1000362 at `(-467, 204)`. Geographic regions 1000013-1000016 define
North and Central Vailand territorial waters, the Vailand Passage free seas,
and the Blackwake Anchorage pirate cove. Their law rows apply bounty rates of
150, 150, 100, and 0 percent with overlap priorities that keep the port and
pirate identities authoritative inside the wider passage.

`Vailand Iron Passage` is a looping 18-link route. Its five-point southern
coastal detour keeps the merchant on actual Water, Water (Swim), Ocean, and
Seaport sectors around the land west of Central Vailand. The package also
defines `Vailand Merchant Cog` as a ship-class hull with speed 12 and armor
30, plus faction-1 `Vailand Ironwind Trader`: pilot mobile 31810, 40 units of
iron, hourly scheduling, and a 3,600-second replacement delay. Iron supply is
320 at North Vailand and 80 at Central Vailand, creating a deterministic
trade gradient.

The provisioner is development-only, idempotent, and collision-sensitive. It
applies the Phase 13/14 prerequisites and campaign content, verifies the exact
topology and active assembly, and runs two actual Kohdee observation windows
around a hard restart. Live positions must change in both sessions; shutdown
positions, merchant slot/generation, route, and active autopilot must survive;
the outbound trip must reach `vailand_central_port`; and no campaign-related
`SYSERR` is allowed. It finishes with the same merchant generation reset to
North Vailand waters so destructive lifecycle acceptance begins under the
150-percent territorial bounty rate.

Use the selected-merchant form of the reversible lifecycle harness:

```bash
./scripts/test_vessel_merchant_in_game.sh \
  --merchant "Vailand Ironwind Trader" --temporary-respawn 5
```

The August 2, 2026 provision run
`/tmp/luminari-vessel-campaign-1000/runs/20260802T065410Z-1061371` passed in
167 seconds on source `923c8024`. The destructive run
`/tmp/luminari-vessel-merchant-check-1000/runs/20260802T065717Z-1068792`
passed in 22 seconds: merchant 18 moved from generation 1 to 2, Kohdee
observed 165 standing loss and a 900-gold bounty, and the replacement retained
40 iron, pilot 31810, the route, and schedule. Cleanup byte-restored Kohdee and
all snapshotted vessel/economy tables.

`sql/components/vessels_campaign_content_rollback.sql` is the content rollback,
not a substitute for a full database restore. Stop vessel writes and retire
the active merchant hull first; the script deliberately leaves an active
definition disabled when dependencies cannot be removed safely.

### Blackwake Derelict Content

The first tracked derelict combines generated vessel interiors with world-file
objects and DG programs rather than adding a compiled quest path:

```bash
./scripts/provision_vessel_derelict.sh
./scripts/test_vessel_derelict_in_game.sh
```

`lib/world/vessel_derelict/700.obj` defines an ash-stained captain log, a
salt-stiff chart, and a bronze tidefinder salvage object. Trigger VNUMs
70010-70012 attach to the generated bridge, crew quarters, and main cargo
hold; object triggers 70013-70014 make the recovered log and chart readable.
The chain requires the player to recover and read the log before finding the
chart, study the chart before opening the cargo panel, and can award each
object only once. Five player DG variables persist discovery state in the
ASCII player file. The ordinary `salvage` command values the tidefinder at 180
gold; the DG program does not implement a parallel reward path.

The SQL package owns the `Blackwake Derelict` ship-class prototype and the
three generated-room mappings. The development provisioner is idempotent and
collision-sensitive: it merges only the reserved world records, creates or
normalizes at most one ownerless hull at `(-533, 330)`, and verifies its
four-room interior and stable identity around a hard restart. Other vessels
receive the same shared room mappings at boot, but the exact-hull DG guards
return without effects. Optional first-finder naming is not enabled for this
initial hull.

The reversible acceptance harness snapshots both player object-save mirrors
before any login, temporarily makes the level-34 staff character a valid
level-30 DG command target, and executes the full clue chain around a hard
restart. It requires exactly one log and chart in both stores, all five DG
variables, one 180-gold tidefinder salvage, stable hull identity, and exact
cleanup. Run `20260802T075751Z-1199403` passed in 55 seconds on source
`a390a387`; provision run `20260802T072737Z-1135588` passed in 61 seconds.

`vessels_derelict_content_rollback.sql` removes the shared mappings and removes
the prototype only when no runtime still depends on it. It does not remove the
world object/trigger records or destroy a persistent hull. A full content
rollback must retire the hull safely and remove the reviewed world records and
index entries separately while application writes are stopped.

### Wilderness Frontier Content

The tracked frontier package connects the region/path contracts to all eight
actual vessel classes:

```bash
./scripts/provision_vessel_frontier.sh
```

`vessels_frontier_content.sql` owns Starfall Trench (region 7100101, minimum
natural depth 96), Aetherwind Skyway (region 7100102, minimum Z 100),
Shardspire Sky Island (region 7100103, minimum Z 200), and Sablebranch River
(path 7100104, `PATH_RIVER`, `SECT_RIVER`). The database path trigger expands
the three authored line vertices into 79 contiguous cells and mirrors them in
`path_index`. All eight prototypes remain inside the production builder speed
and armor limits.

The current package owns this acceptance matrix:

| Class | Prototype | Actual-character capability proof |
|-------|-----------|-----------------------------------|
| Raft | Sablebranch Raft | River movement, 300-pound hold, one-room interior |
| Boat | Sablebranch Riverboat | River movement, 2,000-pound hold, crew quarters |
| Ship | Starfall Survey Ship | Ocean movement, main deck, 12,000-pound hold |
| Warship | Starfall Bastion | Three weapon slots and two weapons decks; no shot fired |
| Airship | Aetherwind Courier | Z 100 speed lane and Z 200 sky island |
| Submarine | Starfall Bathyscaphe | Z -90 dive inside natural depth 104 |
| Transport | Sablebranch Grand Freighter | 40,000-pound hold and three cargo rooms |
| Magical Vessel | Liminal Wayfarer | Plains, River, submerged, and airborne traversal |

The development-only provisioner is atomic, idempotent, collision-sensitive,
and requires the installed binary to be newer than every source input. It
hard-restarts the supervised MUD, verifies database and spatial identity, and
uses actual Kohdee sessions for builder discovery and piloting. The piloted
gate executes every row above. It also proves the sky lane is gated until Z
100 and yields effective speed 12 from requested speed 10, then reaches
Shardspire at `(469, 0, 200)`. It purges every temporary runtime and returns
Kohdee to room 1204; its failure trap performs the same owned-runtime cleanup.
Run `20260802T091531Z-1364409` passed in 75 seconds on source `873171ae`.

`vessels_frontier_content_rollback.sql` removes only the eight owned prototypes,
path, and region identities after checking exact names. Retire any dependent
runtime hull before rollback and stop application writes. It does not undo
unrelated wilderness paths or regions.

The supervised ferry gate has a one-hour total execution budget, including its
final restart and cleanup. The runner retains its historical long default, so
always pass the bounded duration explicitly:

```bash
./scripts/run_vessel_ferry_soak.sh start 2700 60 900
./scripts/run_vessel_ferry_soak.sh status
```

The transient user service submits a generated, nonexistent account name but
does not confirm it. This leaves one non-character descriptor in the
non-expiring confirmation state without creating an account. The monitor
requires its socket to remain `ESTABLISHED` every 20 seconds and fails if the
descriptor-driven game loop reports that it slept. A normal copyover drops
non-playing descriptors by design; when the log proves copyover mode, the
monitor requires the same PID and installed binary, waits for boot, reconnects
the hold descriptor, and records the recovery. A missing socket without that
copyover evidence remains a hard failure. The bounded invocation samples
database and process invariants every minute and serializes Kohdee checks every
15 minutes through the shared login-helper lock. It also fails on a PID change,
route/room/pilot/schedule drift, structure loss, out-of-corridor coordinates,
an installed-binary fingerprint change, or a ferry-specific
movement/persistence error. Launch metadata records the source commit and
`bin/circle` SHA-256. A successful run ends with a controlled local restart
that proves exact paused-coordinate recovery and launches the identical
executable hash, then resumes the ferry. Artifacts live in the run directory
printed by `start`. A failure writes terminal status before cleanup so an
interrupted cleanup cannot leave a stale `RUNNING` result.

### Interior VNUM Allocation

```
Formula: 70000 + (ship_number * 20) + room_index
Active:  70020 - 80019 (ship slots 1-500)
Reserve: 70000 - 80019 (slot 0 remains unused)
Maximum: 500 active vessels * 20 rooms = 10,000 rooms
```

### Persistence Lifecycle

1. **Boot**: Schemas are created or migrated, templates and gameplay data load,
   and saved ship, route, schedule, crew, cargo, ownership, NPC merchant, and
   bounty-hunter state is restored. Every active slot, including the legacy
   fixture, reconstructs its exterior hull. World resets preserve managed
   hulls, then the boot pass relinks them to their fleet slots. Merchant
   reconciliation reattaches matching live generations and assembles
   definitions that are due. Hunter reconciliation accepts only the exact
   target, unique generation name, prototype, fleet slot, and active pilot;
   stale or expired rows retire safely.
2. **Create**: A spawned or purchased vessel receives a fleet slot, object,
   interior, and immediate database record.
3. **Operate**: Docking, route, cargo, trade, ownership, crew, upgrade, and
   insurance changes update their authoritative tables. Player autopilot
   `on`, `off`, `pause`, and `setroute` changes commit the runtime row before
   reporting success; pilot and schedule commands use the same durable
   boundary. A failed write restores the prior in-memory state and compensates
   any earlier write in the operation.
4. **Destroy**: Sinking or deletion evacuates occupants, clears live references,
   applies the applicable persistence policy, and closes any matching merchant
   or bounty-hunter lifecycle. Capturing a hunter removes its configured pilot
   and leaves the ordinary captured hull.
5. **Copyover**: Complete vessel state is committed before descriptor handoff.
   Boot reconstructs dynamic interiors and exterior hull objects before player
   descriptors return to their saved rooms.
6. **Shutdown**: Current vessel state is saved before termination.

The July 29, 2026 local lifecycle run used Kohdee to prove both a graceful full
restart and descriptor-preserving copyover. A dynamic warship recovered while
actively traveling, with route progress, position, heading, damage, combat
link, and schedule intact. A second dynamic transport retained ownership,
generated rooms, cargo, hired crew, a refit, insurance, combat damage, and four
normalized weapon rows. A separate owned-transport run proved one 35-gold dock
fee per port visit, departure and autopilot blocking, payment, and unpaid
balance recovery across copyover and full restart, including recycled dynamic
wilderness rooms.

Exterior-hull recovery has direct local evidence as well. Three hulls shared
the static Testing Dock, while two persisted hulls shared one dynamic room at
`(-62, 82)`. All five relinked after a hard restart, survived `zreset 10000`,
and remained independently boardable. After the player left and the recycle
interval elapsed, `shiplist` still reported one occupied dynamic room. The
temporary fifth hull then purged cleanly. Runtime rows converged on the generic
70002 hull prototype and current room VNUMs derived from their coordinates.

The offline-owner insurance path also has live evidence. Veska bought a
50-gold policy for 10 gold and logged out at 9,990 gold. Kohdee sank the raft
with actual gunfire; one pending claim and one receipt mail existed before
Veska returned. Her first login credited exactly 50 gold, marked the claim
paid, and saved claim high-water mark 1. Her second login retained 10,040 gold
without another credit. Production-snapshot rehearsal remains a release
prerequisite.

The normal player-removal paths have actual-character evidence. A reversible
deleted flag blocked Corven's login without changing the owned raft or Tern
permit, and clearing it restored Corven aboard the same ship. Corven then used
the real character-menu password/confirmation flow with fast wipe enabled.
The player file and database membership were removed, the raft became
unclaimed in memory and SQL, the permit was removed, and a controlled pending
claim became `void`. A second disposable owner, Elyra, then exercised the
failure path. A temporary MariaDB trigger rejected the ownership update inside
the removal transaction. The transaction rolled back, deletion was cancelled
at the real character menu, and account membership, player data, raft
ownership, and a separate Tern helm permit all remained intact. The trigger
was removed, and Elyra immediately logged back in aboard the same raft.

The opponent-specific logout grace also has actual-character and process
recovery evidence. Dorrin and Elyra enabled PvP and completed a consented
Tern-versus-raft attack. Both runtime rows stored the opposite owner for 300
seconds. With Elyra offline, Dorrin's connection survived copyover and his
next attack was permitted; PvP-enabled Veska was refused against the same
raft. After the full five minutes elapsed, Dorrin was refused too. A deed
transfer then exposed and fixed a stale-database defect: ownership and the
grace reset now commit together, and permanent player removal clears the
runtime row inside its cleanup transaction. A fresh boot plus live
Veska-to-Elyra deed left owner `Elyra`, grace timestamp `0`, and an empty
opponent in SQL.

Player autopilot control has the same immediate-durability evidence. A stale
Traveling snapshot first reproduced the defect after a hard local service
replacement even though Kohdee had previously issued `autopilot off`. With the
fix installed, Kohdee assigned `persistroute`, engaged, paused, resumed, and
disengaged the Goshawk; SQL reflected route/state values `3/0`, `3/3`, `3/1`,
and `0/0` immediately after the respective commands. Separate hard service
replacements restored both Off-with-route and Paused exactly. A temporary
MariaDB trigger then rejected only the Goshawk runtime write during resume;
the command reported failure, memory and SQL both remained Paused on route 3,
and the trigger was removed.

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
| `src/vessels_merchants.c` | NPC merchant definitions, assembly, consequences, and respawn (Phase 14) |
| `src/vessels_hunters.c` | HUNTED encounter policy, pursuit, lifecycle, and reconciliation (Phase 15) |
| `src/vessels_events.c` | Regattas, skirmishes, ghost fleets, leaderboards, and recovery (Phase 16) |
| `src/vessels_hazards.c` | Weather hazards, encounters, seastate (Phase 08) |
| `src/vessels_admin.c` | Operator tooling, room pool monitor, MSDP (Phase 09) |
| `src/vehicles.c` | Vehicle lifecycle, state management, persistence |
| `src/vehicles_commands.c` | Player commands (vmount, vdismount, drive, vstatus) |
| `src/vehicles_transport.c` | Vehicle-in-vessel mechanics (loading/unloading) |
| `src/transport_unified.c` | Unified transport interface across all transport types |
| `src/transport_unified.h` | Transport abstraction types and prototypes |

### Content and Development Acceptance

| File | Purpose |
|------|---------|
| `lib/world/vessel_derelict/700.obj` | Blackwake log, chart, and tidefinder objects |
| `lib/world/vessel_derelict/700.trg` | Guarded room and object discovery-chain DG programs |
| `scripts/provision_vessel_derelict.sh` | Development-only world/SQL provisioning and restart proof |
| `scripts/test_vessel_derelict_in_game.sh` | Reversible actual-character discovery and persistence gate |
| `scripts/provision_vessel_frontier.sh` | Development-only trench, river, skyway, and sky-island provisioning plus piloted acceptance |
| `scripts/test_vessel_events_in_game.sh` | Reversible Kohdee regatta, skirmish, ghost-fleet, and leaderboard gate |

### Database

| File | Purpose |
|------|---------|
| `src/db_init.c` | Table creation (init_vessel_system_tables) |
| `src/db_init_data.c` | Template population |
| `sql/components/vessels_phase2_*` | Core schema, rollback, and verification |
| `sql/components/vessels_phase4_*` | Prototype schema, rollback, and verification |
| `sql/components/vessels_phase6_*` | Ownership schema, rollback, and verification |
| `sql/components/vessels_phase7_*` | Economy schema, rollback, and verification |
| `sql/components/vessels_phase8_*` | Encounter schema, rollback, and verification |
| `sql/components/vessels_phase9_*` | Runtime-state schema, rollback, and verification |
| `sql/components/vessels_phase10_*` | Weapons and recovery schema, rollback, and verification |
| `sql/components/vessels_phase11_*` | Generated-room DG schema, rollback, and verification |
| `sql/components/vessels_phase12_*` | Passenger-fare schema, rollback, and verification |
| `sql/components/vessels_phase13_*` | Geographic piracy-law schema, rollback, and verification |
| `sql/components/vessels_phase14_*` | NPC merchant schema, rollback, and verification |
| `sql/components/vessels_phase15_*` | Bounty-hunter policy/lifecycle schema, rollback, and verification |
| `sql/components/vessels_phase16_*` | Showcase-event history, results, leaderboards, runtime ownership, and rollback |
| `sql/components/vessels_campaign_content.sql` | Initial Vailand regions, law, route, merchant, and iron markets |
| `sql/components/verify_vessels_campaign_content.sql` | Read-only campaign topology and identity checks |
| `sql/components/vessels_campaign_content_rollback.sql` | Guarded Vailand content rollback |
| `sql/components/vessels_derelict_content.sql` | Blackwake prototype and generated-room trigger mappings |
| `sql/components/verify_vessels_derelict_content.sql` | Read-only Blackwake identity and mapping checks |
| `sql/components/vessels_derelict_content_rollback.sql` | Dependency-aware Blackwake definition rollback |
| `sql/components/vessels_frontier_content.sql` | Starfall, Aetherwind, Shardspire, Sablebranch, and eight prototype definitions |
| `sql/components/verify_vessels_frontier_content.sql` | Read-only frontier geometry, index, and prototype inventory |
| `sql/components/vessels_frontier_content_rollback.sql` | Guarded frontier content rollback |
| `sql/components/help_vessel_entries.sql` | Idempotent authoritative help migration |
| `sql/components/verify_help_vessel_entries.sql` | Read-only help count, access, content, and duplicate checks |

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
- Development test content available for manual verification

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

- **VNUM Range 70000-80019:** Reserved for dynamic ship interior rooms; zone 700 must own
  the complete range
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
| Object 70002 | Current broken fixture (ITEM_GREYHAWK_SHIP, ship_index=0; see Known Issues) |
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
| Autopilot paused at blocked route | Route waypoints and hull speed | `listwaypoints`, correct unreachable terrain/Z, set speed, then `autopilot resume` |
| Vehicle terrain blocked | Vehicle type | Use MOUNT for hills/mountains |
| Coordinate desync | shipobj linkage | `greyhawk_shipload` (admin), check spec_procs.c |
| Disembark fails | Interior room link | Verify `world[room].ship` is set |
| Ship object doesn't move | shipobj not linked | Set `greyhawk_ships[idx].shipobj = obj` |

### Database Issues

| Issue | Cause | Solution |
|-------|-------|----------|
| FK constraint errors | Parent record missing | Save to `ship_interiors` before cargo/crew |
| Stored procedures fail | Missing EXECUTE privilege | `GRANT EXECUTE ON luminari_mudprod.* TO 'luminari_mud'@'localhost';` |
| Movement pause cannot persist | Database connection/runtime row | Check the one matching `SYSERR`, then repair persistence before resuming |
| Performance degradation | Missing indexes | Check with `EXPLAIN SELECT ...` |

### Gameplay Issues Detail

**Vessel Not Moving**: Check docking (`shipstatus`), speed > 0, autopilot state, terrain compatibility

**Vehicle Loading**: Vessel must be docked/stationary, have cargo capacity, vehicle in cargo hold room

**NPC Pilot Issues**: Verify pilot in ship interior, check `pilot_mob_vnum`, reassign with `assignpilot`

**Schedule Issues**: Check `showschedule`, verify `SCHEDULE_FLAG_ENABLED`, route active

### Debug Logging

The whole vessel and vehicle stack is instrumented behind compile-time macros
declared in `src/vessels.h`. `VESSEL_SYSTEM_DEBUG` defaults to `0`, so normal
builds compile out every diagnostic call site. An explicit development build
enables support, but its runtime category mask still starts empty.

```c
/* src/vessels.h */
#ifndef VESSEL_SYSTEM_DEBUG
#define VESSEL_SYSTEM_DEBUG 0
#endif
```

For a bounded local-development investigation:

```bash
make clean
make CPPFLAGS='-DVESSEL_SYSTEM_DEBUG=1' -j$(nproc)
make install

# In game as LVL_IMMORT+
vdebug status
vdebug on move
vdebug off move
vdebug off
```

`vdebug` and `vesseldebug` are aliases. `on all` enables every category;
`off` with no category clears the mask. Restore a clean default build after the
investigation and require `vdebug status` to report `compiled out`.

Normal builds do not write a line for every position update, waypoint arrival,
wait completion, route loop, wilderness region transform, elevation
adjustment, or matched path. Vessel movement details use the `move` and `auto`
debug categories. Runtime progress remains visible through the monotonic
counters in `autopilot status`, so long soaks do not trade log growth for route
evidence. A rejected automated step still emits one actionable message and
bounded failure diagnostics before the persisted pause prevents repeated
output.

| Runtime category | Covers |
|------------------|--------|
| `core` | General vessel operations and interior generation |
| `move` | Position updates, terrain checks, speed modifiers, blocked moves, and room allocation |
| `auto` | Autopilot state transitions, tick summaries, and travel steps |
| `dock` | Docking, boarding, and defender positioning |
| `db` | Per-ship save/load persistence |
| `func` | Function entry and exit tracing |
| `state` | State transitions |
| `vehicle` | Vehicle operations and damage |
| `vehicle_move` | Vehicle movement and terrain verdicts |
| `transport` | Vehicle-on-vessel transport and capacity checks |

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

### Release Prerequisites

The gameplay layer is not approved for production merely because it builds and
passes automated tests. Before rollout:

1. Repeat the numbered manual regression on the release candidate.
2. Exercise cedit `Off` with an active route: gated commands must refuse,
   coordinates must remain fixed, and recovery commands must remain available.
3. Require `vdebug status` to report that debug support is compiled out.
4. Apply and verify every vessel schema component, then require all 33
   maintained help entries and 79 command keywords to pass both SQL and in-game
   checks.
5. Verify reboot and copyover while under way, in combat, and carrying cargo.
6. Pass the 500-vessel, 25 ms tick measurement and supervised stability check;
   each complete validation must finish within one hour.
7. Rehearse schema migration and rollback against a production snapshot.

The live checklist is
[VESSELS_TODO.md](../project-management-zusuk/vessels/VESSELS_TODO.md).

### Deployment

The server auto-creates and auto-migrates vessel tables at boot. For a controlled
deployment, apply the phase components in ascending order, run each matching
verification script, apply the help migration, and retain the rollback scripts
used in the rehearsal.

See [VESSEL_SCHEMA_DEPLOYMENT.md](../deployment/VESSEL_SCHEMA_DEPLOYMENT.md) for
the DBA procedure. Do not deploy vessel code directly from a development
checkout.

### Verification Queries

```sql
SELECT TABLE_NAME
FROM information_schema.TABLES
WHERE TABLE_SCHEMA = DATABASE()
  AND (TABLE_NAME LIKE 'ship_%'
       OR TABLE_NAME IN ('trade_commodities', 'port_commodities',
                         'freight_contracts', 'vessel_bounties',
                         'vessel_region_law', 'vessel_encounters',
                         'vessel_insurance_claims',
                         'vessel_npc_merchants',
                         'vessel_merchant_consequences',
                         'vessel_hunter_encounters',
                         'vessel_bounty_hunts',
                         'vessel_showcase_events',
                         'vessel_event_participants',
                         'vessel_event_leaderboards',
                         'vessel_event_runtimes'))
ORDER BY TABLE_NAME;

SELECT COUNT(*) FROM ship_room_templates;
SELECT COUNT(*) FROM help_keywords WHERE keyword IN ('SHIPFIRE', 'SHIPBUY', 'SEASTATE');
```

Run the `verify_vessels_*.sql` scripts and
`verify_help_vessel_entries.sql` as the authoritative checks; a single table
or keyword count is insufficient once later phases extend the system.

### Monitoring & Maintenance

- `shiplist` shows fleet state and wilderness dynamic-room utilization;
  `shiplist summary` keeps the health totals available at fleet scale.
- `vmerchant list` shows every configured merchant generation, lifecycle
  state, live hull, cargo mapping, loss count, and reconciliation error.
- `vessel_bounty_hunts` exposes each hunter's target, generation, active slot,
  expiry, cooldown, and terminal reason; investigate any long-lived
  `spawning` row or active row whose fleet identity no longer matches.
- `vevent status` exposes the live event. Investigate `recovery_failed` event
  rows or any `vessel_event_runtimes` row whose hull identity is missing.
- Vessel debug categories provide focused development diagnostics. Candidate
  and production builds must report that support is compiled out.
- Monitor database errors, orphan cleanup, wage and trade ticks, encounter spawn
  volume, and game-loop latency.
- Treat room-pool pressure over 80%, tick time over 25 ms, or repeated
  persistence errors as rollout-stop conditions.

---

## Risk Assessment

| Risk | Impact | Mitigation |
|------|--------|------------|
| Fleet-slot and object identity diverge | Commands address the wrong ship or lose the exterior object | Keep one canonical slot identity, validate every boundary, cover legacy and `vedit` paths |
| Shared wilderness rooms are exhausted | Vessels interfere with all wilderness travelers | Monitor pool pressure, reclaim rooms, and test graceful degradation |
| Full tick exceeds 25 ms | Game-loop latency at fleet scale | Benchmark all live subsystems together at 500 ships |
| Persistence or schema migration loses property | Ships, cargo, ownership, or crew are corrupted | Auto-migration, verification and rollback SQL, snapshot rehearsal, lifecycle tests |
| PvP entry point bypasses consent | Non-consensual property loss | Central `vessel_pvp_permitted()` gate and entry-point coverage |
| Data-driven economy is exploitable | Infinite profit or cargo duplication | Hard price bounds, atomic claims, copyover tests, and sustained simulations |
| Debug or partial toggle behavior reaches production | Log flood or inability to stop faulty ticks | Release preflight, runtime-safe diagnostics, and a load-bearing kill switch |

---

## Development

### Adding New Vessel Types

1. Add enum value to `vessel_class` in `vessels.h`
2. Add terrain capabilities to `vessel_terrain_data[]` in `vessels.c`
3. Add room generation rules to `get_rooms_for_vessel_type()` in `vessels_rooms.c`
4. Update the authoritative help migration and verifier

### Adding New Commands

1. Implement handler in `vessels.c` or `vessels_docking.c`
2. Register in `interpreter.c` under vessel command section
3. Add the authoritative entry and exact keyword to
   `sql/components/help_vessel_entries.sql` and its verifier
4. Add production-linked coverage in
   `unittests/CuTest/test_transport_production.c`
5. Run the SQL verifier and `--vessel-help-check`

### Adding New Vehicle Types

1. Add enum value to `vehicle_type` in `vessels.h`
2. Add terrain capabilities to default capability arrays
3. Add capacity constants (passengers, weight)
4. Add speed modifier constant
5. Update `vehicle_type_name()` in `vehicles.c`
6. Update the authoritative vehicle help migration and verifier
7. Add production-linked tests in `test_transport_production.c`

### Testing

```bash
# Production-linked integration suite
make test

# Install the tested server and remove the root-level circle artifact
make install

# Full local gate, including focused protocol and schema checks
cd unittests/CuTest
make test-all
```

Vessel, autopilot, and vehicle behavior belongs in the production-linked root
suite. Do not recreate the removed standalone mirror implementations.

Primary automated coverage lives in
`unittests/CuTest/test_transport_production.c` and exercises production
functions linked with all game sources. Manual world, command, persistence, OLC,
and copyover behavior is covered by
[VESSEL_SYSTEM_TESTING.md](../testing/VESSEL_SYSTEM_TESTING.md).

For each vessel behavior change:

1. Add or update a production-linked `Test*` function.
2. Add the test source to both build systems if a new source file is introduced.
3. Run `make test`, then `make install`.
4. Run relevant Valgrind checks.
5. Run the numbered manual workflow on development.
6. Update this behavior reference in the same change.

---

## Related Documentation

- [PRD.md](../PRD.md) - Durable product requirements and release criteria
- [VESSEL_BENCHMARKS.md](../testing/VESSEL_BENCHMARKS.md) - Performance data and memory attribution
- [VESSELS_TODO.md](../project-management-zusuk/vessels/VESSELS_TODO.md) - Outstanding work only
- [CHANGELOG.md](../CHANGELOG.md) - What shipped when
- [VESSEL_SYSTEM_TESTING.md](../testing/VESSEL_SYSTEM_TESTING.md) - 30-step manual regression script
- [0001-unified-vessel-system.md](../adr/0001-unified-vessel-system.md) - Architecture decision and invariants
- [TECHNICAL_DOCUMENTATION_MASTER_INDEX.md](../TECHNICAL_DOCUMENTATION_MASTER_INDEX.md) - Complete docs index

---

*Behavior reference; update it whenever vessel behavior changes.*
