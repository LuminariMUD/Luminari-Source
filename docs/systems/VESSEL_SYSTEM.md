# LuminariMUD Vessel System Documentation

**Release Status**: Gameplay layer, initial campaign shipping, initial
data/DG-driven derelict, wilderness frontier package, Phase 16 showcase
events, and Phase 17 exterior customization implemented; wilderness tactical
chart, lookout view, dynamic at-sea narrative, and cosmetics accepted;
development preflight and schema rehearsal pass; player-data balance, human
beta, and staged production rollout remain
**Last Updated**: 2026-10-02
**Scope**: Current behavior reference. For the durable product contract see
[Vessel System Product Requirements](../product-requirements/VESSEL_SYSTEM_REQUIREMENTS.md),
including its
[release-gate state](../product-requirements/VESSEL_SYSTEM_REQUIREMENTS.md#release-gate-state);
for what shipped when see
[the archived changelogs](../previous_changelogs/); for the player's view, with
screenshots, see the [Vessel Player Guide](../guides/VESSEL_PLAYER_GUIDE.md).

---

## Table of Contents

01. [System Overview](#system-overview)
02. [Architecture](#architecture)
03. [Data Structures](#data-structures)
04. [Vessel Types and Capabilities](#vessel-types-and-capabilities)
05. [API Reference](#api-reference)
06. [Player Commands](#player-commands)
07. [Integration Testing Workflows](#integration-testing-workflows)
08. [Vehicle-in-Vessel Mechanics](#vehicle-in-vessel-mechanics)
09. [Performance Characteristics](#performance-characteristics)
10. [Key Constants](#key-constants)
11. [Database Schema](#database-schema)
12. [File Inventory](#file-inventory)
13. [Dependencies](#dependencies)
14. [Troubleshooting](#troubleshooting)
15. [Operations](#operations)
16. [Risk Assessment](#risk-assessment)
17. [Release gates](../product-requirements/VESSEL_SYSTEM_REQUIREMENTS.md#release-gate-state)
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
| -- | -- | -- | -- | -- |
| **Vessel** | Ships, airships, submarines | 5,120-byte base struct | Multi-room | Exploration, cargo, combat |
| **Vehicle** | Carts, wagons, mounts | 152-byte base struct | None | Land travel, cargo, transport |

### System Components

| Component | Description | Source Files |
| -- | -- | -- |
| Core Vessels | Ship management, coordinates, movement | vessels.c, vessels.h |
| Periodic Ownership | Per-vessel deadlines, global service deadline, rollback | vessel_periodic.c |
| Tactical Chart | Wilderness terrain, regions, range rings, contacts | vessels_tactical.c |
| Lookout View | Eight-bearing wilderness samples and visible contacts | vessels_lookout.c |
| At-Sea Narrative | Contextual descriptions and occupied-hull ambience | vessels_narrative.c |
| Movement | Momentum sailing, per-room checks, berths, anchor, maneuvers | vessels_movement.c |
| Autopilot | Waypoint navigation, route following | vessels_autopilot.c |
| Interior Rooms | Multi-room ship interiors | vessels_rooms.c |
| Docking | Ship-to-ship docking mechanics | vessels_docking.c |
| Persistence | Database save/load operations | vessels_db.c |
| Builder and Shipyard | Prototypes, spawning, hull purchase | vessels_edit.c |
| Combat | Damage, weapons, sinking | vessels_combat.c |
| Ownership and Crew | Owners, permits, one-time crew hires, experience, stamina | vessels_ownership.c, vessels_crew.c |
| Upgrades | Refits, wear, insurance settlement | vessels_upgrades.c |
| Repair | Repair stores, crew and character repairs, dock repairs | vessels_repair.c |
| Loss and Recovery | Wreck registry, automatic insurance, summons, trade-in rebuild | vessels_loss.c |
| Economy | Cargo, markets, freight, piracy | vessels_trade.c, vessels_contracts.c, vessels_piracy.c |
| NPC Merchant Fleet | Durable definitions, assembly, consequences, respawn | vessels_merchants.c |
| Bounty Hunters | HUNTED encounter policy, pursuit, durable lifecycle | vessels_hunters.c |
| NPC Raiders | Ambushes, raider tiers, raider AI, boarding and looting, running merchants | vessels_raiders.c |
| Ramming | `shipram`, the ram impact, cooldowns | vessels_ramming.c |
| Living World | Weather hazards and region encounters | vessels_hazards.c |
| Operations | Fleet tools, room-pool monitoring, MSDP | vessels_admin.c |
| Vehicles | Land-based transport | vehicles.c |
| Vehicle Commands | Player vehicle interactions | vehicles_commands.c |
| Vehicle Transport | Vehicle-on-vessel mechanics | vehicles_transport.c |

---

## Architecture

### Memory Layout

- **Vessel** (`greyhawk_ship_data`): 2,672 bytes, max 500 = about 1.27 MiB
- **Autopilot** (`autopilot_data`): 80 bytes (optional, attached to vessel)
- **Schedule** (`vessel_schedule`): ~32 bytes (optional, attached to vessel)
- **Vehicle** (`vehicle_data`): 152 bytes, max 1000 = about 148 KB

### Periodic Scheduling

The default `LUMINARI_VESSEL_EVENTS=scheduled` mode gives every valid
Greyhawk vessel one generation-aware event. It wakes on the next aligned
0.5-second boundary for autopilot, hunter, raider (ambush rolls, raider AI,
merchants running), movement, combat (with ramming), upkeep, crew stamina, crew
repair, narrative, weather, and encounter work, and also carries
that vessel's aligned 75-second schedule deadline. One service-owned event
retains genuinely global vessel event, trade-restock, MSDP, summons arrival,
and merchant work. A stowed hull (in the wreck registry or under summons) is
not active and has no owner event. Fixed-interior RoL hulls
receive their own 2.5-second events through direct object lifecycle hooks.

Vessels remain eligible without players aboard. Spawn, persistence load,
replacement, relinking, sinking, purge, and extraction synchronize ownership
directly. Saving a changed `cedit` vessel-system setting immediately cancels
or recreates the service deadline and owner registry. If the service deadline
cannot be restored, vessel work falls back to the legacy heartbeat.
The owner registry admits all 501 fleet slots, records capacity
rejections, and refills a released slot without scanning the world. Set
`LUMINARI_VESSEL_EVENTS=legacy` before restart to restore all former vessel
heartbeat paths as one exclusive rollback mode. A failed mandatory service
event also selects that complete legacy mode; partial scheduled operation is
not allowed.

### Movement and Pacing

`src/vessels/vessels_movement.c` implements DurisMUD's momentum sailing at
decision D1's pacing (vessels-ships study, section 3.3.2). Orders set targets
and every 0.5-second vessel tick converges on them:

- `speed` sets `setspeed` and `heading` sets `setheading`. Each tick
  `vessel_movement_tick_one()` changes `speed` by the class acceleration and
  `heading` by the class turn rate the short way round, both times the
  sailmaster multiplier (1.1, 1.2, or 1.3 by tier). The turn rate also scales
  from three quarters at speed 3 to the full rate at the design speed, and by
  the share of rudder left (`turnrate / maxturnrate`). A smashed rudder cannot
  turn; a hull with no way possible turns one degree a tick.
- A hull covers `speed / 90` rooms per tick along its heading: speed 30
  crosses a room in 1.5 seconds and speed 12 in 3.75 seconds. `dx` and `dy`
  hold the position inside the current room (-0.5 to 0.5 from its centre).
  The hull crosses room edges in the order its track meets them, entering
  the diagonal room only when the track runs through the corner (DurisMUD
  steps diagonally whenever both edges fall in one tick, which lets a hull
  slip past the corner of a land room unchecked). Each room is entered
  through `update_ship_wilderness_position()`, so every room entered is
  checked for class terrain, altitude or depth, dock-fee clearance, and
  room-pool capacity. A refused room stops the hull where its track met the
  edge, cancels the speed order, and pauses a travelling autopilot. Water depth does not stop a hull:
  seaports sit on water one unit deep, so a draft barrier would close every
  port to ship-class hulls. Shallows refuse a hull only at battle stations,
  where a refused room also rolls the crash check (Weapons and Gunnery (S4)
  below).
- `vessel_max_speed()` is the design speed (`maxspeed`, the prototype's speed)
  times the sailmaster multiplier, the load factor, the sail fraction
  (`mainsail / maxmainsail`), and the terrain, weather, and altitude-lane
  percentage, at least 1, plus 1 with a seadog helmsman on the bridge
  (`vessel_helm_speed_bonus()`), at most `VESSEL_SPEED_LIMIT` (30). With the
  sail shot away it is 0. The load factor is 1 minus the fit-out weight (the
  weights of installed slots) above the class allowance plus the bulk-cargo
  weight (its share of the class hold capacity times the weight of a full
  hold) above the class allowance, divided by the class weight budget. The
  terrain, weather, and lane percentage is cached per room and refreshed on
  every room entered and every hazard check.
- A hull that comes to rest in a port room is berthed: `dock` holds the port
  room vnum. A hull coming to rest also saves its runtime row, so the berth and
  the position a paused autopilot holds survive a restart. Public and NPC hulls
  have no owner to repair or rearm them, so the harbor restores their sail and
  rudder and refills their weapons whenever they berth. A berthed hull, a hull
  at anchor (`anchored`, runtime
  only), and a hull made fast alongside another hold position and take no
  speed order.
  `undock` casts off in 30 seconds (60 ticks) or weighs anchor in 13 (26
  ticks); casting off needs a whole sail, settled dock fees, the shipwrights
  finished, a legal fit-out (Weapons and Gunnery (S4) below), and a captain of the hull's
  minimum level. `anchor` needs a stopped hull on the surface and
  disengages the autopilot. `vessel_sync_berth()` reconciles the berth after a
  spawn or reboot.
- `setsail <direction>` is the harbor maneuver (`vessel_maneuver()`): one room
  at speed 6 or less, leaving the hull stopped on the new heading and berthed
  if the room is a port. `setsail up` and `down` change altitude or depth by
  10 under way. The crew needs 5 seconds between maneuvers.
- Every automated mover sails through the same tick. The autopilot
  (`vessel_autopilot_steer()`) orders the bearing to its waypoint and caps
  speed through `autopilot_data.speed_limit`: zero while the bow is more than
  90 degrees off, so she comes about where she lies, steerage speed 2 while it
  is more than 45 degrees off, and `sqrt(180 * accel * distance)` approaching a
  waypoint where the hull stops, so it comes to rest inside a 0.5-room
  tolerance. A waypoint counts as reached when the hull enters its room:
  `setwaypoint` stores `AUTOPILOT_ARRIVAL_TOLERANCE` (0.5), and the boot
  migration 2026092901 moved rows at the old five-room default to 0.5. It cruises at the ordered speed, or at full speed when none is
  ordered, casts off first from a berth or anchorage, holds the hull while
  waiting or paused, and stops the hull at the end of a one-way route. Altitude
  or depth follows the straight line to the waypoint, at least one unit a
  tick. Hunters steer for their target at their pursuit speed and match its
  speed within two rooms. Merchants cruise at their design speed from a
  standing start. `setschedule` and every scheduled departure validate the
  route by sailing a copy of the hull over it through the same steering and
  `vessel_sail_tick()`, at her present maximum speed with rigging and rudder
  whole, checking each room she would enter with `vessel_chart_cell()`
  (no room allocation); a loop route is sailed on to its second waypoint so
  the turn after the closing leg is checked too.

Per-class handling (`vessel_class_handling()`; Duris analog values, speeds
times 0.3, weights in Duris units):

| Class | Speed | Accel / tick | Turn / tick | Weight budget | Free fit-out | Full hold | Free hold |
| -- | -: | -: | -: | -: | -: | -: | -: |
| Raft | 5 | 5.0 | 25.0 | 5 | 0 | 2 | 0 |
| Boat | 30 | 4.0 | 22.0 | 12 | 2 | 6 | 0 |
| Ship | 20 | 2.0 | 6.5 | 100 | 13 | 70 | 12 |
| Warship | 17 | 1.5 | 4.0 | 142 | 20 | 56 | 0 |
| Airship | 22 | 3.0 | 10.0 | 82 | 13 | 32 | 0 |
| Submarine | 12 | 2.0 | 6.5 | 110 | 16 | 44 | 0 |
| Transport | 15 | 1.2 | 3.0 | 165 | 19 | 140 | 40 |
| Magical | 14 | 1.2 | 2.5 | 200 | 25 | 80 | 0 |

`vedit new` uses the class speed as the prototype default. Voyage times follow
from the pacing: the 368-room Vailand Iron Passage takes about 23 minutes of
sailing at the cog's speed 12, and the harbor ferry's 24-room loop about 2
minutes at speed 10, before departures, waits, and turns.

### Wilderness Coordinates

X/Y: -1024 to +1024; Z: altitude (airships) or depth (submarines)

The class Z contract is enforced before wilderness-room allocation. Surface
hulls remain at Z 0; air-capable hulls may rise only to their configured
ceiling; submersible hulls may use negative Z only in a water column. Submarine
crush depth remains anchored to local bathymetry instead of a fixed class
floor. The autopilot rejects an invalid waypoint Z before steering toward it.
Every room a hull enters is resolved and validated once inside
`update_ship_wilderness_position()`; movement does not run the allocating
`can_vessel_traverse_terrain()` probe beforehand. If that central move rejects
terrain or Z, the hull stops at the room's edge, and a travelling
autopilot enters `PAUSED`, persists the runtime state, and tells occupants
which waypoint is unreachable. It does not retry the same invalid room every
heartbeat. Correct the route and resume autopilot.

### Wilderness Integration Contract

Vessels extend the wilderness system; they do not create a separate geography.

| Wilderness signal | Vessel behavior |
| -- | -- |
| Dynamic room pool | Characters and exterior hulls keep their coordinate room occupied; co-located hulls share it |
| Generated sector | The central position update gates every room entered; maximum speed consumes the resulting sector |
| Bathymetry | Submarine crush depth; shallows refuse a hull at battle stations (S4) |
| Weather field | Maximum speed, visibility, helm risk, and storm damage |
| `REGION_ENCOUNTER` | Builder-authored encounter selection |
| Sector regions | Magical or transformed waters through the generated sector |
| Paths | Roads for vehicles; `PATH_RIVER` digitalizes canonical River travel cells for rafts and boats |
| Geographic regions | Canonical source for named seas and territorial waters |
| Bathymetric regions | Thresholded natural-depth trenches reported through `seastate` |
| Altitude-lane regions | Thresholded high currents that multiply eligible airship maximum speed by 125 percent |
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
threshold. Builders can use `reglist type 5`, `reglist type 6`, `reglist type 7`, and `pathlist type 5` without paging unrelated records.

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

    /* Position and navigation (vessels_movement.c) */
    double x, y, z;           /* Wilderness room coordinates */
    double dx, dy;            /* Position inside the room, -0.5..0.5 */
    double heading;           /* Current heading, 0 <= heading < 360 */
    double speed;             /* Current speed; speed / 90 rooms per tick */
    short int setheading;     /* Ordered heading */
    short int setspeed;       /* Ordered speed */
    short int maxspeed;       /* Design speed */
    int dock;                 /* Berth: port room vnum, 0 when not berthed */
    bool anchored;            /* At anchor (runtime only) */

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

**Critical Linkages** (established during boarding in `src/vessels/vessels_legacy.c`):

| Linkage | Purpose |
| -- | -- |
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
    double speed_limit;          /* Steering cap on speed, set each tick */
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
| -- | -- | -- | -- | -- |
| RAFT | Rivers, shallow | Slow | 1-2 | Bridge |
| BOAT | Coastal | Moderate | 2-4 | Bridge, Quarters |
| SHIP | Ocean | Moderate | 3-8 | Bridge, Quarters, Cargo, Deck |
| WARSHIP | Ocean | Fast | 5-15 | Bridge, Armory, Weapons, Quarters, Brig |
| AIRSHIP | Air | Fast | 4-12 | Bridge, Observation, Engineering, Quarters |
| SUBMARINE | Underwater | Slow | 4-10 | Bridge, Helm, Engineering, Quarters |
| TRANSPORT | Ocean | Slow | 6-20 | Bridge, Large Cargo, Passenger Quarters |
| MAGICAL | Any | Variable | 1-5 | Custom configuration |

Rooms beyond these are discovered at random (`vessel_discovered_room_type()`),
never one the class is too small for (the template's `min_vessel_size`): a
raft's only extra room is a hold, and medical bays begin at ship size.

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
| -- | -- | -- | -- |
| Ocean/Deep Water | 100% | 100% | 100% |
| Shallow Water | 75% | 100% | 0% (blocked) |
| Rivers | 50-100% (by type) | 100% | 0% (blocked) |
| Land/Mountains | 0% (blocked) | 75-100% | 0% (blocked) |
| Weather band (squall, storm, gale) | -5% per band | -15% per band | 0% submerged |

### Vehicle System

| Type | Passengers | Cargo | Base speed | Terrain |
| -- | -: | -: | -: | -- |
| `VEHICLE_CART` | 2 | 500 lbs | 2 | Road, plains |
| `VEHICLE_WAGON` | 6 | 2,000 lbs | 1 | Road, plains |
| `VEHICLE_MOUNT` | 1 | 200 lbs | 4 | Road, plains, forest, hills |
| `VEHICLE_CARRIAGE` | 4 | 800 lbs | 2 | Road |

**States**: `IDLE`, `MOVING`, `LOADED`, `HITCHED`, `DAMAGED`, `ON_VESSEL`

Vehicles are not objects: `look_at_room()` lists those standing in the room
through `vehicle_list_to_char()` ("River Cart, a cart, stands here."). A vehicle
loaded aboard a hull is in no room (`location` is `NOWHERE`).
`load_vehicle_onto_vessel()` and `unload_vehicle_from_vessel()` need the hull
stopped or docked and at the surface (`z` 0): aloft or submerged she has no
ground beside her.

**Terrain Flags** (`VTERRAIN_*`): `ROAD`, `PLAINS`, `FOREST`, `HILLS`, `MOUNTAIN`, `DESERT`,
`SWAMP`; water is impassable to every vehicle.

**Speed Modifiers by Terrain** (`get_vehicle_speed_modifier()`, the same for every type on the
terrain it can enter): road 150%, plains 100%, forest, hills and desert 75%, mountain and swamp
50%. `vehicle_get_speed()` takes 50% in poor condition and 75% when the load passes three
quarters of capacity or every seat is taken.

---

## API Reference

### Vessel Functions

```c
/* Lifecycle */
void vessel_init_all(void);                           // Initialize at boot
void load_vessels(void);                              // Load from database
void save_vessels(void);                              // Save to database
struct vessel_data *find_vessel_by_id(int id);        // Find by ID

/* Movement (vessels_movement.c) */
bool update_ship_wilderness_position(int ship, int x, int y, int z); /* enters one room */
bool can_vessel_traverse_terrain(enum vessel_class type, int x, int y, int z);
int get_terrain_speed_modifier(enum vessel_class type, int sector, int storm_band);
const struct vessel_class_handling *vessel_class_handling(enum vessel_class type);
double vessel_max_speed(struct greyhawk_ship_data *ship);
double vessel_turn_rate(const struct greyhawk_ship_data *ship, double max_speed);
void vessel_movement_tick_one(struct greyhawk_ship_data *ship);  /* one 0.5 s tick */
bool vessel_maneuver(struct greyhawk_ship_data *ship, struct char_data *ch, int dir);
bool vessel_begin_departure(struct greyhawk_ship_data *ship, struct char_data *ch);
bool vessel_autopilot_steer(struct greyhawk_ship_data *ship);
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
bool autopilot_tick_one(struct greyhawk_ship_data *ship); // Scheduled owner work
void autopilot_tick(void);                            // Legacy rollback wrapper
```

A timed waypoint is a physical stop, not only an autopilot state. Entering its
wait caps speed at 0, so the hull loses way at its class rate and comes to
rest at the waypoint, while the requested cruise speed stays ordered. A boot or
copyover during the wait reconstructs the vessel held with the remaining wait
intact; expiry lifts the cap and advances the route. A paused autopilot holds
the hull the same way; `autopilot off` leaves it on its ordered speed and
heading. An assigned NPC pilot engages a route the autopilot is off on
(`autopilot_tick_one()`), so `autopilot off` is refused while one is assigned;
`autopilot pause` or `unassignpilot`, which keeps the route set, holds her.

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
| -- | -- | -- |
| board | Board a vessel | `board <ship>` |
| greyhawk_tactical | Display tactical map | `tactical` |
| greyhawk_status | Show ship status: position, navigation, armor and structure by side, sails, rudder, holes, sink timer, struck colors, weapons (`vessel_show_condition()`) | `shipstatus` |
| shiptalk | Speak across all rooms of the current vessel | `shiptalk <message>` |
| greyhawk_speed | Order a speed; the hull gathers or loses way at its class rate | `speed <0-30>` |
| greyhawk_heading | Order a heading; the hull comes about at its turn rate | `heading <0-360>` |
| greyhawk_setsail | Harbor maneuver: one room at speed 6 or less, or climb or dive 10 | `setsail <direction>` |
| dock | Dock with vessel | `dock <ship>` |
| undock | Remove a gangway, cast off from a berth (30 s), or weigh anchor (13 s) | `undock` |
| vessel_anchor | Anchor a stopped surface hull | `anchor` |
| lookout | View canonical surroundings from a bridge or deck | `lookout` (`look_outside` legacy alias) |
| board_hostile | Grapple and cross to an enemy vessel | `board_hostile <vessel>` |

System-generated vessel messages use independent per-vessel cooldown classes.
Repeated depth and weather messages are limited to one copy per class every
120 seconds; a change from squall to storm or gale remains immediately
visible. High-volume damage, NPC return-fire, miss, and reload messages are
limited to one copy per class per half-second vessel tick. Sinking, a stop at
the edge of a refused room, rigging-collapse, and rudder-loss warnings remain
immediate. Suppressed copies
increment the process-wide `vessel_messages_throttled` performance counter.

#### Wilderness Tactical Chart

`tactical` renders a 21-by-21, north-up chart centered on the current vessel.
Every base cell comes from the canonical `get_map()` wilderness renderer:
`~` is deep water, `.` is shoal or shallow water, `=` is River, `u` is
underwater, `:` is Beach, `D` is a seaport, `#` is coastal land, `^` is other
land, and `?` is an unknown sector. The chart therefore changes with the same
terrain, path digitalization, and sector transforms that govern movement.

Overlays are applied in gameplay order: terrain, five-unit `o` and ten-unit
`O` range rings, public region edge `+`, visible contact, then the player's
`@` vessel. Region edges and the accompanying region list include geographic,
bathymetric, altitude-lane, and sky-island polygons. Encounter and sector
regions remain hidden so tactical presentation does not reveal private spawn
or transform metadata.

Contacts come from `vessel_collect_contacts()`, the one contact list that
`contacts`, `tactical`, and `shipfire` targeting share: every other vessel
within `vessel_sight_range()` (production weather penalty and posted-lookout
bonus included), nearest first, ties in fleet-slot order. `V`, `B`, `C`, and
`X` report sound, battered, crippled, and sinking vessels; `M` marks multiple
contacts in one cell. A nearest-first roster below the chart includes the
two-letter vessel ID, vessel name, condition, three-dimensional range,
bearing, compass direction, and relative Z. `contacts` prints the nearest 20
of the same list with their IDs. The header reports position, heading,
weather, visibility, and the current vessel's aggregate internal hull
condition.

#### Wilderness Lookout View

`lookout` is the player-facing spelling; `look_outside` remains compatible.
The command resolves the current hull through its registered generated room,
then permits bridges and outside-view decks. It samples canonical modified
wilderness terrain in eight compass directions at bounded near, middle, and
horizon distances. Static coordinate rooms take precedence where present, and
all other samples use the same sector transforms and path overlays as travel.

The header reports X/Y/Z, heading, weather, production visibility, current
terrain, natural elevation, and water column. Each bearing compresses
consecutive equal samples into readable terrain bands. A nearest-first contact
list uses the shared `vessel_sight_range()`, three-dimensional range, compass
bearing, `vessel_status()`, and relative Z; it does not maintain a separate
lookout contact model.

Owners use `shipcustomize` to set or clear optional paint and figurehead text,
each limited to 80 printable characters. The current hull's appearance follows
the lookout header, and visible contacts show their appearance below the
nearest-first roster row. The same values build the exterior object's room
description and persist in `ship_interiors` through Phase 17. Look builds a
managed hull's room line as she lies at that moment
(`vessel_hull_room_description()`): "is moored here" at a berth or alongside,
"lies at anchor here", "is sinking here", "hovers overhead" aloft, and "is here"
otherwise.

Development acceptance run
`/tmp/luminari-vessel-lookout-check-1000/runs/20260802T131015Z-1845762`
passed in 41 seconds on source `302c8b87` and installed SHA-256
`75a62d7c17ed93c3cfc7c4e74db458b59745dd4e993ea075bec3cfb7616f0bf3`.
Actual Kohdee read both help aliases, set and cleared both cosmetic fields,
observed them in `lookout` and exterior room text, saw an open-water sound
contact, and read real coastal sectors. The gate purged its hulls,
byte-restored Kohdee, and left no acceptance runtime rows. Seven
production-linked tests cover cosmetic formatting, sample selection,
terrain-band compression, capacity/input boundaries, and compass boundaries.

#### Dynamic At-Sea Narrative

The `At sea:` line shown by `lookout` passes the current hull through the
compact narrative-weaver API. It combines vessel class, stopped or moving
state, speed band, depth, and the raw wilderness weather value with one
deterministically selected `region_hint`. Geographic prose therefore follows
the same `region_data` polygons as travel and piracy law; it does not add a
vessel-only map or random description state.

`src/vessels/vessels_narrative.c` also formats class-, speed-, weather-, and
depth-aware ambient messages. The normal heartbeat broadcasts one every 240
vessel ticks, or 120 seconds, but only for a moving hull with a player aboard.
Stopped, empty, and invalid hulls add no ambient traffic. Staff may run
`vesseldebug ambient` aboard a hull to invoke the same formatter immediately;
it is an acceptance hook rather than an independent message implementation.

`vessels_narrative_content.sql` owns eight idempotent Vailand hints: one
geographic and one severe-weather variant for each of the four canonical
water regions. Its verifier requires the exact 8/4/4 inventory and rejects
ownership or metadata drift; its guarded rollback deletes only rows owned by
`vessel_narrative_v1`.

Development run
`/tmp/luminari-vessel-narrative-check-1000/runs/20260802T115413Z-1685068`
passed in 34 seconds on source `547e54b3` and installed SHA-256
`908e809acf0941624d4ce301dc4deaadb14f627d1e9fd140718147ada068079e`.
Actual Kohdee observed overcast 167/255 conditions, a steady-warship at-sea
line with Vailand Passage prose, and matching forced ambience. The gate
byte-restored Kohdee, removed every temporary hull, and left no acceptance
runtime row. Five production-linked tests cover exact weather boundaries,
all eight classes, speed bands, submarine depth, and invalid inputs.

#### Hostile Boarding

`board_hostile <vessel>` requires the attacker to be aboard a different hull,
within normal docking range, with neither hull already docked. It and `dock`
find the target as `shiplock` does (`vessel_find_contact()`: contact ID, then
a word of the name, nearest first). Player-owned
targets pass through the shared PvP-consent gate before defenses or rolls are
resolved. The attempt alerts the target and moves idle NPC crew from other
interior rooms to its entrance and bridge chokepoints.

Boarding is ability 27 and is a class ability for every class. `train` displays
both its invested rank and effective total. `compute_ability()` adds the better
of the character's Strength or Dexterity modifier, applies the equipped armor
penalty, and adds 2 for Minotaur Seafaring. The player-file `BrdV` marker
distinguishes current saves from legacy saves where slot 27 held the retired
Jump ability. Loading an unmarked save clears that slot once; marked saves
preserve legitimate Boarding ranks.

The target's strongest conscious, PvP-consenting occupant anywhere in its
generated interior supplies the defending Boarding total. Each stage rolls
d20 + attacker Boarding against d20 + defender Boarding + vessel modifier;
ties favor the defender. The first stage secures grappling lines. Only a
successful grapple reaches the crossing stage.

The vessel modifier includes hull class (-4 raft, -2 boat or transport, +2
airship, +3 submarine or magical vessel, and +4 warship) and penalties for
internal structure below 75, 50, or 25 percent. Grappling adds up to 6 from
target speed plus twice its sailmaster tier. Crossing adds up to 3 from speed
plus twice its bosun tier. The final modifier is clamped from -8 through +15.

A failed grapple leaves the attacker aboard the original hull. A failed
crossing does the same unless the attacker rolled a natural 1 or lost by at
least 10; that critical failure drops the attacker into the exterior water
room and resolves a d20 + Athletics swim check. A successful crossing moves
the attacker to the target entrance, falling back to its bridge if necessary,
and starts combat with eligible defenders in that room. Non-consenting player
passengers are not forced into combat.

Development run
`/tmp/luminari-vessel-boarding-check-1000/runs/20260802T124631Z-1797834`
passed in 74 seconds on source `e8377caa` and installed SHA-256
`b01e8610325dc40445c8550a8b93752bfc979512145efbe357e48c22db04ed8a`.
Actual Kohdee lost a 26-to-56 grapple to Vesselmate, then won grapple 56-to-14
and crossing 56-to-28 after their trained ranks were reversed. Both target
warnings arrived, both temporary hulls were purged, and both player files were
restored exactly. The production-linked suite passes 302 tests.

### Autopilot Commands

| Command | Description | Usage |
| -- | -- | -- |
| autopilot | Engage, pause, disengage, or read the autopilot | `autopilot [on\|off\|pause\|status]` |
| setwaypoint | Create waypoint | `setwaypoint <name>` |
| listwaypoints | List one's own and the harbors' waypoints | `listwaypoints` |
| delwaypoint | Delete one's own waypoint that no route sails through | `delwaypoint <name>` |
| createroute | Create route | `createroute <name>` |
| addtoroute | Add a waypoint to one's own route | `addtoroute <route> <waypoint>` |
| listroutes | List one's own and the harbors' routes | `listroutes` |
| delroute | Delete one's own route that no hull runs on a schedule or is sailing | `delroute <name>` |
| setroute | Assign one's own or a harbor route | `setroute <route>` |

Each waypoint and route records its creator (`creator_id`, the player's ID;
Phase 24, which boot adds in `vessel_ownership_ensure_schema()` and
`init_vessel_system_tables()` creates). Rows no player made (content, and rows
made before Phase 24) have creator 0: every captain may use them and only
immortals change them. A captain's own rows are theirs alone: only the creator
or an immortal may `delwaypoint`, `delroute`, or `addtoroute` them, and only
they may sail (`setroute`), schedule (`setschedule`), list, or route through
them. A scheduled hull rebuilds her route from the cache at each departure, so
if other captains could use a route, its creator could redirect their hulls,
and their use could hold the creator's rows in place; keeping use to the
creator closes both. A waypoint that any route sails through, and a route a hull
runs on a schedule or is sailing, stay even for their creator. Where names
repeat, the commands take the caller's own row of that name, then the harbors',
otherwise the first (which they then refuse). Permanent player removal
(`vessel_handle_player_removal()`) gives the player's rows to the staff
(creator 0): a removed player's ID is handed out again when it was the highest.

### Operator Commands (Phases 09, 14, 15, and 16)

| Command | Description | Usage |
| -- | -- | -- |
| shiplist | Fleet overview + room pool health | `shiplist [summary]` |
| shipgoto | Teleport aboard a vessel | `shipgoto <slot>` |
| shipfix | Restore a vessel to full condition | `shipfix <slot>` |
| vmerchant | Inspect or reconcile NPC merchants; force a confirmed loss | `vmerchant [list\|sync\|sink <id> confirm]` |
| vesseldebug | Inspect debug state, run balance diagnostics, force ambience, or advance encounters | `vesseldebug [status\|balance [duels]\|on ...\|off ...\|ambient\|encounter]` |
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
Telnet option 69 and uses `REPORT` for any of the vessel variables below.
`src/vessels/vessels_admin.c` refreshes them on the vessel tick, and the normal
MSDP update sends each reported value when it changes, so a client can draw
gauges, a weapons panel, and a contact plot without polling.

| Variable | Value |
| -- | -- |
| `SHIP_NAME`, `SHIP_ID` | The vessel's name and two-letter contact ID |
| `SHIP_X`, `SHIP_Y`, `SHIP_Z` | Wilderness coordinates and altitude or depth |
| `SHIP_HEADING`, `SHIP_SPEED` | Heading in degrees and speed, as `shipstatus` shows them |
| `SHIP_HULL`, `SHIP_HULL_MAX` | The sums of the four internal-structure sections |
| `SHIP_STATUS` | `sound`, `battered`, `crippled`, or `sinking` |
| `SHIP_ARMOR`, `SHIP_INTERNAL` | Tables keyed `fore`, `port`, `rear`, and `starboard`, each holding `CURRENT` and `MAX` |
| `SHIP_SAIL`, `SHIP_SAIL_MAX`, `SHIP_RUDDER`, `SHIP_RUDDER_MAX` | Sail and rudder condition |
| `SHIP_STAMINA`, `SHIP_STAMINA_MAX` | Crew stamina left (negative in deficit) and when rested |
| `SHIP_TARGET` | The ID of the contact the guns are locked on, or empty |
| `SHIP_WEAPONS` | One table per mounted weapon, in slot order: `SLOT`, `NAME`, `ARC`, `AMMO`, `READY` (1 when `shipstatus` says ready: undamaged, rounds left, reloaded), and `DAMAGE` (0-100: disabled from 1, destroyed at 100) |
| `SHIP_CONTACTS` | The `contacts` list, the nearest 20 first: one table each with `ID`, `NAME`, `RANGE` (rooms, one decimal), `BEARING` (degrees), and `ARC` |

An `ARC` value is one of the arc tables' keys, so a client can show the armor
a weapon or contact faces. An empty array (an unarmed hull, an empty sea) is
an empty string, as `GROUP` is. When a character leaves a vessel, the server
sends an explicit empty state: every string, table, and array becomes empty
and every number zero, so a client does not go on displaying a stale vessel.

A client that negotiates GMCP but not native MSDP receives the same values as
strict JSON in the `MSDP` GMCP package (see
[MSDP Variables](MSDP_VARIABLES.md#wire-encodings)): tables become objects and
arrays become arrays. The old unquoted `MSDP.<variable> <value>` fallback is
not accepted as vessel support, and a separate GMCP ship package is not part
of this release contract.

`vesseldebug encounter` is an acceptance hook, not a parallel spawner. It
advances the cadence counter and immediately invokes the same production
region, class, depth, chance, HUNTED eligibility, spawn, and lifecycle path
used by the heartbeat. Staff can use it in a normal build even though runtime
debug categories remain compiled out.

`vesseldebug ambient` resolves the operator's current generated vessel room
and invokes the production narrative broadcaster once. It does not advance or
reset the normal 120-second cadence.

`vesseldebug balance [duels]` is read-only and remains available when debug
logging is compiled out. It runs a deterministic equal-warship duel sample
(`vessel_balance_run_duels()`, default 200, at most 1,000) without creating
fleet hulls: two default warships with Duris's frigate combat fit (three large
ballistae on each beam, a heavy beamcannon on the bow, NPC-crew gunnery at +5)
start 8 rooms apart on parallel courses and sail through the production
`vessel_sail_tick()`, `vessel_fire_weapon()`, damage, and reload code under a
simple captain who holds the healthier beam at about 7.5 rooms, until one is
holed on a second side. Their crews reload, tire, rest, and repair through the
production `vessel_reload_tick()`, `vessel_crew_tick_one()`, and
`vessel_repair_tick_one()` (S5). The duels draw on the live random stream from
a fixed seed and restore it afterward. A duel with no kill in an hour is a
draw (a smashed rudder cannot come about until the crew mends it). The report also invokes the production
1,000-trade simulation, reports class cost and crew-hire anchors, and reads
only anonymized aggregate persistence totals. Its mechanical verdict uses
decision D2: a 3-8 minute median, a 12 minute p95, nothing under 90 seconds,
and at most 2% drawn; weapon reload times are the tuning lever. The final line
always requires human beta feedback; the command cannot manufacture a fun
rating or authorize rollout.

### Living World Commands (Phase 08)

| Command | Description | Usage |
| -- | -- | -- |
| seastate | Weather, depth, visibility, hull state | `seastate` |

Hazards and encounters (`src/vessels/vessels_hazards.c`) read only wilderness
signals - no vessel-private geography:

- **Weather**: raw 0..255 bands from `get_weather(x,y)` exactly match the
  field a coastal walker sees: 0..127 clear, 128..177 cloudy, 178..199
  rain/squall, 200..224 storm, and 225..255 gale/thunder. Squall, storm, and
  gale degrade rigging; a gale with neither a sailmaster nor the assigned
  pilot at the bridge damages the hull. Narrative, visibility, lookout,
  tactical, hazard, and maximum-speed logic share these thresholds: the
  maximum speed reads `vessel_storm_severity()` and loses 5% per band (15% for
  airships). Submerged submarines are sheltered.
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

Phase 15 hunter policy (`src/vessels/vessels_hunters.c`) optionally extends one of
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
| -- | -- | -- |
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

Completion writes the terminal event status and every leaderboard row in one
database transaction. The status is written first, and only to an event that
has not ended; the scores are written only when that changed the event's row,
so finishing an event twice adds them once. When the COMMIT gets no reply (a
connection lost while the reply is on its way), the event's row is read back:
the terminal status there means the scores are recorded and the event ends.
If the row cannot be read either, the staff retry `vevent end`, which finds
the row as the first attempt left it. A failed cleanup or score commit keeps
the event in `recovery_failed`, a mark that never replaces a terminal status,
and blocks another start instead of repeating work each tick. Every event has a one-hour ceiling. Events do not resume after process
restart: boot retires tracked ghost hulls and closes interrupted rows as
`recovered`; a cleanup failure remains explicit for `vevent recover`. Captain
IDs are gameplay player-file IDs, and leaderboard display resolves the current
name through the authoritative player index rather than the unrelated
`player_data.player_idnum` key.

### Cargo & Trade Commands (Phase 07)

| Command | Description | Usage |
| -- | -- | -- |
| market | List a port's commodity prices | `market` |
| cargobuy | Load bulk goods (dock only) | `cargobuy <commodity> <qty>` |
| cargosell | Sell bulk goods (dock only) | `cargosell <commodity> [qty\|all]` |
| cargomanifest | Show bulk cargo aboard | `cargomanifest` |
| contracts | Freight board + your active jobs | `contracts` |
| contractaccept | Take a freight job (posts a bond, loads cargo) | `contractaccept <id>` |
| contractdeliver | Deliver at destination, collect | `contractdeliver <id>` |
| contractabandon | Return a job to the board | `contractabandon <id>` |
| plunder | Take cargo from a ship you've cleared | `plunder` |
| bounty | Check a price on someone's head, or pay yours off at a lawful port | `bounty [<player>\|pay]` |
| marque | Buy a letter of marque (dock only) | `marque` |
| dockfees | Inspect or pay the current berth charge | `dockfees [pay]` |

Economy model (`src/vessels/vessels_trade.c`): commodities live in
`trade_commodities` (seeded with 9 goods, builder-editable); per-port stock
lives in `port_commodities`, seeded deterministically from the port vnum so
ports differ without randomness (lawful goods only: contraband stock comes from
content, S7). Price = base scaled by scarcity, clamped to
+/- `TRADE_MAX_DRIFT` (60%) - the anti-arbitrage bound, unit-tested across
the whole supply domain. A batch is priced one unit at a time across every
supply level it moves through; quoting the whole batch at its first unit's
price would let an oversized shipment flip two markets and profit again in
reverse. Buying drains local stock (price up), selling floods it (price down);
inventory is clamped to 10-400, and `vessel_trade_restock_tick()` drifts all
ports back toward baseline. Ports buy at 85% of ask, so same-port round trips
lose money. Bulk lots persist in `ship_cargo_manifest` with
`cargo_room = 0`. `cargobuy` and `cargosell` record the port's new supply and
the manifest in one transaction before any gold moves, then save the gold
with `save_char_checked()`; if that save fails the old supply and manifest
are recorded again and the gold and the hold restored. If even that undo
cannot be recorded, the trade stands as recorded, in memory too, and the
persistence service's minute save stores the gold once it can; only a crash
before then leaves cargo recorded without its price or a price without its
cargo (work item #12). A refused write moves no gold, and without a database
no trade is made. A COMMIT the server does not answer (a connection lost
while its reply is on the way) may have taken effect or not; both writes set
absolute values, so the trade or its undo is written again on a reconnected
session, and once that commits it stands whichever way the first went. A
connection lost inside the transaction loses the whole of it: the database
layer refuses the statements that follow until the transaction is rolled
back, so none of them commits on its own on a new session
(`docs/systems/DATABASE_INTEGRATION.md`, Transaction Management).

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
`ship_schedules.next_departure` is an absolute MUD hour (`schedule_mud_hour()`,
the hour of the day modulo 24), so a departure past midnight, or an interval of
24, is not taken for one already due; a row saved as an hour of the day before
this reads as overdue and departs once.

Freight contracts (`src/vessels/vessels_contracts.c`): each port's board offers runs
to other *known trading* ports (any with `port_commodities` rows that is a port
room), with quantity and payout scaled from real wilderness distance between the
dock rooms. The payout is the goods' base worth plus a distance premium.
Accepting takes the goods' base worth as a bond (refused without the gold),
loads the cargo (capacity-checked), and claims the row with a conditional
UPDATE, so two captains racing for the same job cannot both win it. The claim
and the manifest commit in one transaction before the bond is debited, and the
debit is saved with `save_char_checked()`; if that save fails the gold is
restored, the job reopened and the freight unloaded, so the record never keeps
the freight without the bond or the bond without the freight. Abandoning
returns the job to the board and leaves the bought freight aboard, so taking
and dropping a job gains nothing. Delivering requires the freight still aboard.
Boards refresh on a TTL; accepted contracts are never cleared by a refresh.
Market, cargo and freight commands key the port by `vessel_port_room()`: the
hull object's room when that is a port, else the port at her coordinates.

Piracy (`src/vessels/vessels_piracy.c`): `plunder` moves cargo from a cleared prize
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

Bounties decay and can be paid off. `vessel_bounties.last_offense_at` (Phase
18\) records the latest offense; `vessel_get_bounty()` returns
`vessel_bounty_after_decay()`, which holds the bounty for one full day and then
removes 5% of it per further day, clearing it after 21 quiet days. Every
offense path (plunder and NPC-merchant consequences) goes through
`vessel_bounty_record_offense()`, which folds the decay into the stored amount
before adding and restarts the clock. The row holds the whole bounty, so the
offense is not recorded when the standing bounty cannot be read: read as
none, the offense alone would replace it. `vessel_add_bounty()`, the form
`plunder` uses outside a transaction, tries once more and reports the result:
the raider is told of a bounty only when it was recorded, and otherwise the
staff are told the amount to apply. `bounty pay` in any port room outside a
pirate cove clears the bounty for `vessel_bounty_payoff_cost()`, 125% rounded
up; WANTED captains may pay. WANTED, HUNTED, port refusal, and hunter
eligibility all read the decayed amount.

NPC merchant shipping (`src/vessels/vessels_merchants.c`) is definition-driven rather
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
| -- | -- | -- |
| shipbrowse | Shipyard catalog: for-sale hulls with price and level | `shipbrowse` |
| shipbuy | Buy a listed hull at a dock, become owner; or trade in your hull berthed there | `shipbuy <id> [trade]` |
| shipchristen | Owner: rename the ship (first christening free, then 10% of her value) | `shipchristen <name>` |
| shipsummon | Owner at a shipyard: list your hulls, or call one (or her wreck) here | `shipsummon [<number \| name>]` |
| shiprenown | The ten players' hulls with the most renown (S7) | `shiprenown` |
| shipcustomize | Owner: review, set, or clear exterior details | `shipcustomize [show]` or `shipcustomize <paint\|figurehead> <description\|clear>` |
| shipdeed | Owner: transfer ownership | `shipdeed <player>` |
| shippermit / shiprevoke | Owner: manage helm clearances | `shippermit <player>` |
| shipcrew | List owner, pilot, permits, crew | `shipcrew` |
| shiphire / shipdismiss | Hire or release crew (dock only) | `shiphire <position> <tier>` |
| shipupgrade | List/install refits (dock only) | `shipupgrade [<refit>]` |
| shipweapon | List, buy, sell, or rearrange weapons (dock only) | `shipweapon [list \| buy <weapon> <arc> \| sell <slot> \| swap <slot> <slot>]` |
| shipequip | Fit or remove the ram and neutral colors (dock only) | `shipequip [list \| buy <ram\|colors> \| sell <ram\|colors>]` |
| shiprearm | Refill ammunition (dock only) | `shiprearm [<slot> \| all]` |

Only prototypes with `for_sale = 1` appear in `shipbrowse` or can be bought;
merchant, hunter, derelict, event, harbor, and new `vedit` prototypes are
unlisted. A player owns at most `CONFIG_VESSEL_OWNER_CAP` hulls, stowed ones
(wrecks and hulls under summons) included,
(`cedit` "Vessel Hulls Per Owner", 1-10, default 3; `vessel_owner_at_cap()`),
enforced at `shipbuy`, `claimship`, and for the recipient of `shipdeed`.
Immortals are exempt, ownerless public and NPC hulls never count, and owners
above a lowered cap keep their hulls. `vessel_helm_level_refused()` holds a
hull's departures to its level (`vessel_ship_min_level()`: the prototype's
`min_level`, or the class minimum 1/1/16/22/24/23/21/25 for raft, boat, ship,
warship, airship, submarine, transport, magical): `undock` from a berth,
`autopilot on`, `assignpilot`, and `setschedule`. Immortals and NPC pilots are
exempt. When a prototype-backed hull's level cannot be read, the departure is
refused rather than held to the lower class minimum. These checks only read
`ship_prototypes`; `vessel_prototype_ensure_schema()` creates and migrates the
table at boot, never on a command.

Owned ships restrict the helm (`is_pilot()`) to owner + permits + immortals
(`src/vessels/vessels_ownership.c`). An unowned hull with an NPC pilot (a
public ferry or merchant) restricts it to NPCs and immortals, so passengers
cannot steer, stop, anchor, or reroute her or dismiss her pilot; other unowned
hulls stay open to anyone. Owner persists in `ship_interiors.owner`
(auto-migrated); permits persist in `ship_crew_roster` (crew_role
'captain', npc_vnum -1). Capture via `claimship` transfers ownership and
voids old permits. A deed or capture writes the new owner and the reset of
the old owner's PvP consent in one transaction (`vessel_transfer_owner()`);
when its COMMIT gets no reply, the owner is read back from `ship_interiors`
before the hull changes hands in memory, so memory and the database name
the same owner.

Soft-deleted characters retain their deeds so staff restoration is lossless.
Before permanent player-file removal, one transaction makes their ships
unowned, removes their helm permits, voids pending insurance and merchant
consequences, clears current merchant-attack attribution, and removes their
vessel bounty and durable hunter lifecycle. Any matching live hunter is then
retired through the normal vessel cleanup path. If that transaction cannot
commit, player removal is deferred instead of orphaning property.

Crew (`src/vessels/vessels_crew.c`): four positions (sailmaster, gunner, bosun,
quartermaster) at three tiers (green/able/veteran). The gunner's tier is
mirrored into the legacy `guncrew` field; movement and repair read the
sailmaster and bosun tiers directly. Hiring is a one-time price
(`vessel_crew_hire_cost()`: sailmaster 1,600/6,000/15,000, gunner
2,400/8,000/18,000, bosun 2,000/7,000/16,000, quartermaster 1,200/4,500/11,000
gold by tier); crew draw no wages and never walk off. The retired
`ship_interiors.wages_owed` and `ship_runtime_state.wage_ticks` columns remain
in the schema, unread, so a rollback needs no data migration. Crew rows live
in `ship_crew_roster` with `npc_vnum <= -100`; the tier is `loyalty_rating`
and the experience `experience` (Phase 21). Experience, promotion, casualties,
and stamina are under Crew, Repair and Loss (S5) below.

Upgrades, wear, insurance (`src/vessels/vessels_upgrades.c`): four one-time refits
raise hull ceilings at install time (study 3.3.1): plating +20% armor and
reinforcement +20% internal structure on every arc (at most 255), rigging +10%
design speed (at least 1, at most 30, `vessel_rigged_speed()`), hold +25%
cargo; each costs a fifth of the class price. A plating or reinforcement refit
adds its points to the arc's current value as well as its ceiling, so it
repairs nothing (L10). `vessel_upkeep_tick()` grinds armor and subsystems down
while under way (never below 1 structure per section). Insurance is automatic
(S5): a lost owned hull's payout (`vessel_insurance_payout()`) becomes one
durable `vessel_insurance_claims` row plus a system-mail receipt
(`vessel_pay_insurance()`). Online owners receive the gold immediately;
offline owners receive pending settlements on their next login. A player-file
high-water mark prevents duplicate credit if recovery occurs between saving
the character and closing the database claim. `shipinsure` is retired:
`vessel_refund_insurance_premiums()` (boot, and the Phase 21 SQL) queues a
fifth of every remaining `ship_interiors.insured_for`, at least 1 gold, as a
claim for the owner and zeroes the column, which is otherwise unread.

### Naval Combat Commands (Phase 05)

| Command | Description | Usage |
| -- | -- | -- |
| shiplock | Lock the guns onto a contact (battle stations) or clear the lock | `shiplock [<contact ID or name> \| off]` |
| shipfire | Fire a weapon slot, or every weapon on an arc that can, at the locked contact | `shipfire <slot \| fore \| port \| rear \| starboard> [<contact>]` |
| shipsight | Each weapon's DC and chance to hit against the locked contact | `shipsight [<slot>]` |
| shipscan | Armor, structure, weapons, condition, and the owner's law standing of a contact within 20 rooms | `shipscan <contact>` |
| shiprepair | At sea: one Craft (woodworking) patch from the stores; at a shipyard: the owner buys dock repairs | `shiprepair [armor \| structure \| sails \| rudder \| weapons \| all]` |
| shipsalvage | Haul floating salvage crates into the hold (helm, stopped) | `shipsalvage` |
| claimship | Capture a beaten prize from an uncontested bridge | `claimship` |
| strikecolors | Yield: make a stopped hull a prize for ten minutes | `strikecolors` |

Combat model (`src/vessels/vessels_combat.c`, gunnery in
`src/vessels/vessels_gunnery.c`): a shot resolves through the gunnery model
(Weapons and Gunnery (S4) below) and a hit through the damage model (Damage Model (S3)
below). Weapon arcs derive from the heading-relative bearing between exact
positions (`vessel_arc_toward()`), reloads tick on the heartbeat
(`vessel_combat_tick()`, `vessel_gunnery_tick_one()`), and NPC-piloted ships
return fire automatically. At battle stations a hull keeps off harbors and
shallows and may run aground (Weapons and Gunnery (S4) below).

Every player-driven hostile entry point uses `vessel_pvp_permitted()`. A
consented engagement records a persisted, opponent-specific five-minute
window. If an owner logs out, only the original still-PvP-enabled aggressor may
continue during that window; other players and expired snapshots fail closed.
Ownership changes and permanent owner removal clear inherited consent.

`shiplock` and `shipfire` target only contacts (`vessel_find_contact()`: exact
two-letter ID first, then the nearest hull whose name, or a word of it,
starts with the argument). `vessel_gunnery_permitted()` limits the
guns to the owner, helm permit holders, members of the online owner's group,
and immortals; unowned hulls fire only through NPC return fire.
`vessel_fire_permitted()` adds the firing hull owner's own consent whenever a
non-owner fires on another player's hull, so retaliation is always lawful.
The gunner and owner consent checks have no side effects. Only a shot that
clears range, arc, and consent records the engagement, once and for the
actual gunner, so a refused shot leaves no grace behind. Recording it also
takes the aggressor out of the target owner's group, as attacking a groupmate
in person does (`leave_group()` in `fight.c`). If the target's owner
logs out, that gunner may keep firing while the hull owner stays online with
PvP enabled.
Harbors are neutral: `vessel_ship_is_in_port()` refuses player and NPC fire
into or out of a port. Every volley, hit or miss, costs `PULSE_VIOLENCE` of
command lag.

### Damage Model (S3)

`src/vessels/vessels_damage.c` holds the DurisMUD damage model (vessels-ships
study 3.3.1, 3.3.3).

- Class condition profiles (`vessel_class_condition()`): each class takes its
  Duris analog's per-arc armor and internal structure at the class beam armor,
  its sail hit points, and its price. A prototype's armor is its beam armor
  (0-229, `VESSEL_MAX_PROTOTYPE_ARMOR`) and scales the eight numbers in
  proportion (`vessel_initialize_condition()`, at least 1 structure per arc);
  the rudder is 20 (LuminariMUD-only). A warship at 109 has armor 87/109/65/109
  and structure 38/47/23/47 (fore/port/rear/starboard) and 140 sail.

| Class | Beam armor | Armor F/P/R/S | Internal F/P/R/S | Sail | Price |
| -- | -: | -- | -- | -: | -: |
| Raft | 3 | 2/3/1/3 | 1/1/1/1 | 20 | 200 |
| Boat | 8 | 6/8/4/8 | 3/4/2/4 | 40 | 600 |
| Ship | 66 | 53/66/33/66 | 26/33/16/33 | 110 | 8,000 |
| Warship | 109 | 87/109/65/109 | 38/47/23/47 | 140 | 44,000 |
| Airship | 63 | 50/63/37/63 | 22/27/13/27 | 120 | 72,000 |
| Submarine | 84 | 67/84/50/84 | 29/36/18/36 | 130 | 60,000 |
| Transport | 110 | 88/110/55/110 | 44/55/27/55 | 130 | 24,000 |
| Magical | 153 | 122/153/91/153 | 53/66/33/66 | 160 | 144,000 |

- Arcs (`vessel_arc_for_relative_bearing()`, used by `vessel_arc_toward()`) are
  relative to the heading, as in DurisMUD: fore 320-40 degrees, starboard
  40-140, rear 140-220, port 220-320.
- Shipyard price (`vessel_prototype_price()`): class price times
  `0.5 + 0.25 * armor / class armor + 0.25 * speed / class speed`, so a
  default hull costs the class price.
- A hit (`vessel_resolve_hit()`, from `shipfire` and NPC return fire) runs the
  catalogue weapon's fragments (Weapons and Gunnery (S4) below). Each fragment rolls the
  weapon's damage, or for a beam weapon takes it from the range, and strikes
  the sails at the weapon's sail-hit chance and sail share
  (`vessel_damage_sail()`; warships take 85%) or the arc facing the shooter,
  scattered across the weapon's spread, at its hull share
  (`vessel_damage_hull()`).
- Hull damage (Duris `damage_hull()`): armor absorbs first. A hit it holds
  stops there unless the shot is a confirmed critical, which carries half the
  damage into the structure with a 50% weapon-damage chance; overkill spills
  into the structure with a 15% chance. On a gutted arc one hit in three
  deflects into another arc that still has structure, and every hit there
  damages a weapon. Stern structure hits also foul the rudder. Every hit lands
  at least one point. Hazards use the same path (`vessel_apply_damage()`).
- Criticals: the natural d20 must reach the weapon's threat
  (`vessel_critical_threat()`: 20 for 2-3% pierce, 19-20 for 10%, 18-20 for
  15%, never for 0%) and a second roll with the same bonus must meet the same
  target number.
- Weapon damage (`vessel_damage_weapon()`, a random surviving weapon on the
  struck arc, five times the structural damage) accumulates in the slot's
  `damage`, persisted in `ship_weapons.weapon_damage`: 1 or more disables the
  weapon (`vessel_weapon_ready()`), 100 destroys it. `shiprepair` mends
  damaged weapons, and restores a destroyed one while berthed, until S5
  prices repairs; `shipfix` clears all damage.
- Knockdown (`vessel_knockdown_aboard()`): one structural hit in nine makes
  everyone aboard but staff roll Reflex (d20 plus their Reflex save) against
  DC 15 or fall prone (reclining) with two combat rounds of lag.
- Breaches (`vessel_breached_arcs()`): an arc with neither armor nor structure
  is holed. One holed arc makes `vessel_max_speed()` 0, or half for a hull
  aloft (z above 0); two start the sink timer (`vessel_update_condition()`,
  `vessel_begin_sinking()`). A hull holed afloat, or sinking, stops dead at
  once: `vessel_update_condition()` zeroes her speed. Deflected hits reach only another arc's structure,
  so a hull shot from one side is holed once and cannot sink until a second
  side is holed: maneuvering decides fights.
- Sinking (`sink_ticks`, saved in `ship_runtime_state` since Phase 19, so a
  restart resumes the countdown): 150-300 ticks (75-150 s) for a player-owned hull,
  2000-3000 ticks (1000-1500 s) for an unowned hull so it can be boarded and
  looted. A sinking hull has no maximum speed, drops her autopilot, and
  cannot fire, maneuver, or `shiprepair`; she can still be boarded and
  plundered. `vessel_damage_tick_one()`, from the combat tick, counts down;
  at zero half of each bulk cargo lot floats off as salvage crates
  (`vessel_spill_cargo()`) and `vessel_sink()` evacuates the hull as before.
  `vessel_status()` reports SINKING only for a sinking hull; a gutted hull
  whose armor holds is crippled, and so is a holed one whatever structure she
  has left.
- Prizes (decision D6, `vessel_prize_disabled()`): a hull is beaten when she
  has a holed arc, cannot move (`vessel_max_speed()` 0), has struck her colors,
  or is abandoned at sea (`vessel_abandoned_at_sea()`: not in port, nobody
  awake aboard but the claimant; hired crew positions are abstract).
  `claimship` and `plunder` take only a beaten prize from a bridge where
  nobody else is awake, and `claimship` refuses a sinking one; hostile boarding (`can_attempt_boarding()`) holds only on a
  hull at speed `VESSEL_BOARDING_MAX_SPEED` (3) or less or a beaten one.
- `strikecolors`: the owner or a helm permit holder of an owned, stopped hull
  strikes her colors (`colors_struck_ticks`, runtime only) for
  `VESSEL_COLORS_STRUCK_TICKS` (1200, ten minutes); they fly again when she
  gets under way.
- Salvage crates are prototype-less `ITEM_OTHER` objects (value 0 the
  commodity, 1 the units, 2 `VESSEL_SALVAGE_CRATE_MARK`), not takeable, that
  decay after `VESSEL_SALVAGE_CRATE_HOURS` (24) MUD hours. `shipsalvage` from
  the helm of a stopped hull stows them through `vessel_stow_cargo()`, the
  capacity-bounded stowage that `plunder` also uses.
- Migration (Phase 19, 3.3.10): `vessel_prototype_ensure_schema()` rescales
  prototype armor once (`armor_scale` 0 to 1) by the class beam armor over the
  old `vedit new` default (`vessel_rescale_legacy_armor()`: raft 2, boat 5,
  ship 20, warship 40, airship 15, submarine 25, transport 20, magical 20); a
  warship of 40 becomes 109. A runtime snapshot saved before S3
  (`condition_model` 0) is converted once at load
  (`vessel_convert_legacy_condition()`): the old model gave every arc the
  prototype armor and structure of half that plus 10, each half again with
  plating or reinforcement, 20 sail and 20 rudder, and rigging added 5 speed.
  The hull takes the class profile at its rescaled armor, refits recomputed at
  a fifth (`vessel_refit_arcs()`, `vessel_rigged_speed()`), and each arc, the
  sails, and the rudder keep their damage fraction; the old model had no holes,
  so every arc keeps at least 1 structure. Saves write `condition_model` 1.

### Weapons and Gunnery (S4)

`src/vessels/vessels_weapons.c` holds the DurisMUD weapon and equipment
catalogue (vessels-ships study 3.3.4), static tables like the class profiles.
Prices are 2 gold per Duris platinum. Reloads are in 0.5 s vessel ticks: Duris's
30 s and 45 s, tuned to decision D2 with the duel harness, became 20 s and
30 s in S4, and 17 s (34 ticks) and 25.5 s (51 ticks) in S5 once crews tire.

| Weapon | Price | Weight | Ammo | Range | Damage | Fragments | Spread | Sail hit | Hull/sail % | Pierce | Reload | Arcs |
| -- | -: | -: | -: | -- | -- | -: | -: | -: | -- | -: | -: | -- |
| Small Ballista | 100 | 3 | 60 | 0-8 | 2-4 | 1 | 10 | 12% | 100/50 | 10% | 34 | all |
| Medium Ballista | 200 | 6 | 50 | 0-10 | 4-6 | 1 | 10 | 14% | 100/50 | 10% | 34 | all |
| Large Ballista | 1,000 | 10 | 30 | 0-12 | 6-9 | 1 | 10 | 16% | 100/50 | 10% | 34 | all |
| Small Catapult | 1,000 | 10 | 30 | 4-15 | 2-3 | 4 | 160 | 20% | 100/100 | 2% | 34 | fore, rear |
| Medium Catapult | 1,600 | 13 | 20 | 5-20 | 2-4 | 5 | 260 | 20% | 100/100 | 2% | 34 | fore, rear |
| Large Catapult | 2,400 | 17 | 12 | 6-25 | 2-5 | 6 | 360 | 20% | 100/100 | 2% | 34 | fore, rear |
| Heavy Ballista | 2,000 | 15 | 6 | 0-4 | 15-22 | 1 | 10 | 0% | 100/0 | 15% | 34 | port, starboard |
| Light Beamcannon | 8,000 | 7 | 40 | 0-20 | 16 to 4 | 1 | 10 | 10% | 100/30 | 15% | 51 | all |
| Heavy Beamcannon | 10,000 | 9 | 40 | 0-23 | 22 to 5 | 1 | 10 | 10% | 100/30 | 15% | 51 | all |
| Mind Blast Cannon | 8,000 | 5 | 50 | 0-20 | crew stun | 1 | 360 | - | - | - | 51 | all |
| Fragmentation Cannon | 10,000 | 7 | 20 | 0-16 | 4-6 | 5 | 90 | 50% | 50/100 | 0% | 51 | fore, rear |
| Long Tom Catapult | 10,000 | 9 | 6 | 12-32 | 3-6 | 8 | 360 | 20% | 100/100 | 3% | 51 | fore, rear |

- The beam cannons' damage falls from its maximum at minimum range to its
  minimum at maximum range (`VESSEL_WEAPON_RANGE_DAMAGE`); the Mind Blast
  Cannon does no damage (`VESSEL_WEAPON_CREW_STUN`). Catapults and the Long Tom
  are ballistic (`VESSEL_WEAPON_BALLISTIC`). The beam, blast, fragmentation,
  and Long Tom weapons are capital (`VESSEL_WEAPON_CAPITAL`).
- Equipment: a ram (weight `(hull weight + 10) / 24`) and neutral colors
  (weight 0).
- Slots (`struct greyhawk_ship_slot`, `GREYHAWK_MAXSLOTS` 16): a slot holds
  its type (`VESSEL_SLOT_WEAPON` or `VESSEL_SLOT_EQUIPMENT`), its catalogue row
  (`item`), a weapon's arc, rounds left, damage, and reload timer; the
  catalogue supplies the rest (`vessel_slot_weapon()`, `vessel_slot_name()`,
  `vessel_slot_weight()`). Slot weights count toward the load factor
  (`vessel_load_factor()`).
- A new hull (`vessel_fit_default_weapons()`) carries its class armament with
  full ammunition: large ballistae on a warship's bow and both beams, one
  medium ballista on the bow of the other armed classes, none on a raft or
  boat.
- Firing spends a round and starts the weapon's reload; a weapon fires only at
  a target inside its range band.
- Class fitting (`class_fitting[]`, 3.3.1): the Duris analog's mounts and
  weapon weight cap per arc and its allowed weapons (`ship_allowed_weapons[]`).

| Class | Mounts F/P/R/S | Arc weight caps F/P/R/S | Weapons | Weight budget |
| -- | -- | -- | -- | -: |
| Raft | 0/0/0/0 | 0/0/0/0 | none | 5 |
| Boat | 1/1/1/1 | 3/5/3/5 | small ballista | 12 |
| Ship | 1/3/1/3 | 17/26/17/26 | all but the heavy beamcannon and Long Tom | 100 |
| Warship | 2/3/2/3 | 31/44/31/44 | all | 142 |
| Airship | 1/3/1/3 | 13/32/13/32 | small, medium, and large ballistae; small and medium catapults; light beamcannon; mind blast; fragmentation cannon | 82 |
| Submarine | 2/0/1/0 | 27/0/27/0 | all but the Long Tom | 110 |
| Transport | 2/3/1/3 | 26/35/26/35 | all | 165 |
| Magical | 2/4/2/4 | 35/50/35/50 | all | 200 |

- A fit-out is legal (`vessel_fitout_problem()`) when every weapon is allowed
  on the class and on its arc, no arc exceeds its mounts or weight cap, at
  most one weapon is capital, rafts and boats carry no ram, each equipment
  item is fitted once, and the whole fit-out (a ram included) weighs no more
  than the class weight budget. The same check refuses a purchase and a
  departure from a berth (`vessel_begin_departure()`).
- Shipyard (`vessel_refit_ship()`: the owner, berthed in port with no
  departure under way, not refused by the port): `shipweapon buy` mounts a loaded weapon in the first free slot; a
  capital weapon also needs a veteran gunner (renown arrives in S7).
  `shipweapon sell` pays 90%, or 10% for a damaged weapon; `swap` exchanges
  two slots whole. `shipequip` fits one ram (2 gold per hull weight) or
  neutral colors (free), sold back at 90%; colors stay while cargo is aboard.
  `shiprearm` refills at `VESSEL_ROUND_PRICE` (2) gold a round and skips
  destroyed weapons.
- Maintenance (`maintenance_ticks`, runtime only, like the departure timers):
  installing takes `VESSEL_INSTALL_TICKS_PER_WEIGHT` (75 s) per weight point
  and rearming `VESSEL_REARM_TICKS` (75 s) per weapon
  (`vessel_add_maintenance()`; immortals skip it). It counts down in the
  movement tick, shows in `shipstatus`, and blocks departure from a berth.
- Gunnery (S4, `vessels_gunnery.c`): a shot hits on `d20 + bonus >= DC`, a
  natural 1 missing and a natural 20 hitting (`vessel_hit_percent()`). The DC
  (`vessel_gunnery_dc()`) is `21 - round(20 * h)`, where `h` is Duris's
  `weaponsight()` chance for an untrained crew with full stamina after its
  2d50 roll (`vessel_volley_chance()`): a motion term from the target's
  crossing speed (LuminariMUD speed / 0.3 in Duris units), the swing of the
  relative bearing, and the closing speed, both hulls projected one second
  ahead by sailing copies through `vessel_sail_tick()` (direct fire weighs
  `crossing + 4 * swing + closing / 4`, ballistic weapons
  `crossing / 2 + 3 * swing + closing`); the base chance 0.5 divided by
  `1 + motion / 50`, plus `(sqrt(hull weight) - 3) / 100` for target size;
  the miss chance shrinking from its maximum at maximum range to
  `(miss - 0.05)^4` inside a quarter of it; and x1.5 miss against a hull
  aloft. A stopped frigate is DC 1 inside a quarter of the large ballista's
  range and DC 6 at its maximum.
- The gunnery bonus (`vessel_gunnery_bonus()`) is the gunner tier (+2/+4/+6)
  plus the firing character's Dexterity modifier, or Intelligence for a
  ballistic weapon, at most `VESSEL_GUNNERY_BONUS_MAX` (7); NPC crews fire at
  the gunner tier plus `VESSEL_NPC_GUNNERY_BONUS` (5). Criticals threaten by
  the weapon's pierce and confirm against the same DC. Reload is the
  catalogue reload times `1 - 0.15 * gunner mod` (0.15 a tier).
- A weapon fires (`vessel_weapon_fire_problem()`) only when sound, loaded,
  reloaded, on the arc facing the target, and with the target inside its
  band; the hull may not be in port, anchored, submerged, or sinking
  (`vessel_hull_fire_problem()`), and the target may not be in port or
  submerged. Locks (`lock_target`) and battle stations (`battle_ticks`,
  `VESSEL_BATTLE_STATIONS_TICKS` 360, 180 s) are runtime only: a lock or a
  shot, fired or received, puts a crew at battle stations; the lock drops
  when the contact leaves sight, enters port, dives, or sinks, checked each
  tick and at every shot and sighting (`vessel_locked_target()`); the crew
  stands down 180 s after the lock clears. `shipfire <arc>` fires every weapon on
  the arc that can. NPC return fire (`vessel_npc_return_fire()`) uses the same
  rules.
- Battle stations bar harbors and shallows (`vessel_enter_cell_default()`,
  L9): a hull at battle stations is refused a port room and water shallower
  than her class `min_water_depth` (surface hulls; the pre-S2 grounding test),
  so seaports stay open to every other hull. Barred from a harbor she lies
  off it, her orders kept, until the crew stands down; an autopilot keeps its
  route. Any other refused room at battle stations, or with a stunned crew,
  rolls Duris's crash check (`vessel_crash_check()`): 2d50 against
  `(speed / 0.3 + 50) / (1 + 2 * sail mod)` (sail mod 0.1 per sailmaster
  tier; 100 when stunned), and a grounding lands `hull weight / 25 + 1` hits
  of 1-9, the first on the bow and each later one half the time on a random
  side or the sails. `setsail` into a harbor at battle stations is refused
  without a roll.
- Crew stun (`vessel_mental_blast()`, the Mind Blast Cannon): a hit sets the
  target's `stun_ticks` (runtime only) to `2 * (5 + 15 * closeness)`, where
  closeness runs from 0 at the weapon's maximum range to 1 at its minimum, so
  5-20 s. A stunned crew (`vessel_crew_stunned()`) cannot steer
  (`vessel_sail_tick()` holds speed and heading), maneuver
  (`vessel_maneuver()`, `setsail`), fire, reload (the gunnery tick freezes
  the timers), or `shiprepair`; the schedule route check sails her unstunned. Inside mid-range everyone aboard
  saves (Will, `VESSEL_KNOCKDOWN_DC` 15) or falls prone for two rounds
  (`vessel_knockdown_aboard()`, which takes the save type).
- Flight: `greyhawk_range()` counts one room per 10 Z, so every vessel range
  (weapons, contacts, docking, boarding, sight) does; `vessel_range_between()`
  and `vessel_bearing_between()` use exact positions (room plus offset). A
  hull aloft is boarded only from within `VESSEL_BOARDING_MAX_ALTITUDE` (10) Z.
- `shipsight` prints each weapon's DC and chance against the locked contact;
  `shipscan` reads a contact within `VESSEL_SCAN_RANGE` (20) rooms, 22 with a
  posted lookout; `contacts` shows the arc each contact lies off and marks the
  lock.
- Persistence (Phase 20): every weapon and equipment slot is a `ship_weapons`
  row with its `catalog_id` and `ammo` (`vessel_db_save_weapons()`,
  `vessel_db_load_weapons()`); `ship_runtime_state.slot_data` is no longer
  written or read. A weapon row saved before S4 (`catalog_id` 0) loads as the
  class default weapon on the same arc with full ammunition, keeping its
  damage.

### Crew, Repair and Loss (S5)

Study sections 3.3.5-3.3.7 and the summons of 3.3.9; renown, rewards, and the
renown gates came in S7 (Rewards, Renown and Contraband below).

- Crew experience (`vessels_crew.c`): each hired position carries experience
  in Duris skill points, starting at its tier floor (`vessel_crew_floor()`:
  sailmaster and quartermaster 200/800/2,000, gunner 250/1,000/2,500, bosun
  220/900/2,200). `vessel_crew_gain()` trains a filled position and promotes it
  on reaching the next floor: the sailmaster 0.003 a room sailed (not a raft or
  boat, `vessel_movement_tick_one()`), the gunner 0.0015 a reload tick and 0.1
  a shot with a contact locked, the bosun 0.1 a repair at battle stations with
  a lock and 0.01 otherwise, and the sailmaster 1.5, bosun 0.5, and
  quartermaster 1.5 per 2,000 gold of `cargosell` revenue
  (`vessel_crew_sale_gain()`). The hull that sank another
  (`vessel_crew_credit_kill()`, her `last_attacker`) trains every hand by the
  target's hull weight, a tenth of it for an unowned target. A lost hull's
  crew gives up 10% of its experience to a player's hull (plus 1% per 30
  renown lost, S7), else 5% plus 1% per 100 hull weight
  (`vessel_crew_casualties()`), dropping a tier below its floor but never
  below green. Able and veteran hires need the hull's renown (S7); staff may
  hire them freely. `shipcrew` shows each hand's experience and next floor.
- Stamina (runtime only, `stamina_spent`, 0 = rested): the maximum is 500 plus
  100 a tier (`vessel_stamina_max()`); `vessel_crew_tick_one()` returns 1.5 a
  tick, 6 berthed or anchored. The helm (`vessel_sail_tick()`) spends a
  change's share of the class accel times (2 + hull effort) and of the class
  turn times (3 + hull effort), five times that warping an immobile hull
  round, divided by the sailmaster multiplier and halved per tick; each shot
  the weapon's weight over the hull effort; each reload tick
  (`vessel_reload_tick()`) a twentieth of that; repairs 1-3. The hull effort
  (`vessel_hull_effort()`) is the square root of the class hull weight over 10.
  Past empty, `vessel_stamina_modifier()` (`1 / (1 + deficit / max / 3)`)
  scales accel, turn, the reload, the hit chance, the crash save, and repair
  odds. The route preflight sails its copy with a rested crew. `shipstatus`
  shows it.
- Repair (`vessels_repair.c`): the stores are the class hull weight less
  `repair_used` (runtime only), refilled by `vessel_berth()`.
  `vessel_repair_tick_one()` makes crew repairs at Duris's per-mille odds
  halved per tick, times `(1 + bosun mod) * stamina modifier` (bosun mod 0.15
  a tier): sails and rudder below `max * (bosun mod + 0.4)` (anchored 250,
  shot away 50 or 15 at battle stations, under way 15 and nothing shot away),
  damaged weapons (100, one store in five), and structure below
  `max * (bosun mod + 0.1)` (anchored 125, holed 50, at battle stations 50 or
  5, under way 15), one point a success and never armor, never while sinking
  or stunned, all below 90% of the maximum. `shiprepair` away from a berth is
  a character repair: Craft (woodworking) DC 15, one point of stores, one point
  on the weakest structure below the cap, else the sails, rudder, or a damaged
  weapon, 12 s lag. Berthed, the owner buys dock repairs through
  `vessel_refit_ship()`: armor and structure 2 gold a point, sails and rudder 4,
  a damaged weapon 2 a damage point, a destroyed one half its price; each
  order `75 + points` s (75 s a weapon, 150 s rebuilt) of maintenance.
- Stowed hulls (`vessels_loss.c`): a hull in the wreck registry or under
  summons keeps her fleet slot, interior, and persistence but is not active
  (so `is_valid_ship()` is FALSE and contacts, ticks, targeting, and commands
  pass her by), has no exterior object and nobody aboard, and is saved with
  `stowed = 1`. `vedit_find_free_slot()`, `vessel_owned_hull_count()`,
  `shiplist`, and `shippurge` still see her; permanent removal of her owner
  purges her, and removes the player's helm permits from other owners' stowed
  hulls. Boot restores her in the world at her saved location and
  `vessel_restow()` takes her out again. The runtime and weapon saves accept
  her, and `save_all_vessels()` saves stowed hulls with the fleet, so a
  failed save when she is stowed is retried at the next full save.
- Loss (decision D3): `vessel_sink()` credits the victor's crew, settles
  `vessel_insurance_payout()` (the class share of `vessel_hull_price()`, 75%
  for ship, transport, and boat and 50% for the rest, 90% when an unowned
  hull made the kill, nothing for a raft or a `wreck_hull`; the claim sets
  `ship_runtime_state.wreck_hull` in its transaction, so a hull restored
  mid-sink by a crash is not paid twice), and for an owned
  hull calls `vessel_wreck_hull()`: casualties, then
  `vessel_rebuild_hull()` from `vessel_wreck_prototype()` (the cheapest boat
  for sale, else the cheapest hull for sale, else a boat to the `vedit`
  defaults) with no weapons, equipment, refits, or cargo, sails only when an
  unowned hull sank a heavier one, `wreck_hull` set, and stowed at the wreck
  site with `summon_due` 0. Legacy cargo objects and NPC crew rows go; name,
  owner, display ID, cosmetics, permits, and crew stay.
- Summons: `shipsummon` at a port room lists the owner's hulls and orders one
  for `vessel_summon_fee()` (a tenth of a gold per hull weight point) after
  `vessel_summon_seconds()`: Duris's 50 (raft, boat) or 70 mud hours over the
  empty-hold maximum speed in Duris units (less 20 for the rest, at least 2),
  doubled from the registry, at most 60 mud hours; staff take a second. It is
  refused while sinking, at battle stations, or already summoned. The hull
  empties her hold, puts everyone aboard into her exterior room, casts off
  anything alongside, releases vehicles, stops her autopilot, saves the
  shipyard as her location with `summon_due`, and stows. `vessel_summon_tick()`
  (service event) brings a due hull in: `vessel_create_runtime_hull()` at the
  shipyard, berthed, saved, and scheduled again. `vessel_summon_announce()`
  tells the dock, and sends word to her owner if online elsewhere.
- Trade-in and rename: `shipbuy <id> trade` rebuilds the owner's hull berthed
  at that dock (empty hold, not casting off or alongside) in place as the new
  prototype for its price less 90% of her `vessel_hull_price()` (nothing for a
  `wreck_hull`), a credit above the price paid out. The new hull gets her class armament
  (`vessel_fit_default_weapons()`), then `vessel_carry_fitout()` takes aboard
  each old weapon and equipment piece the fit-out can legally hold and pays
  `vessel_slot_sale_value()` for the rest. `shipchristen` is free while the
  hull bears her prototype's name and for staff, else 10% of her value.
- Persistence (Phase 21): `ship_crew_roster.experience`, and
  `ship_runtime_state.stowed`, `wreck_hull`, and `summon_due`.

### Raiders and Ramming (S6)

Study section 3.3.8, ramming (1.6), and the sailmaster's ram training gain of
3.3.5. Renown (the tier roll's second term and what a raider carries) and the
neutral-colors sale penalty came in S7 (below).

- Ambushes (`vessels_raiders.c`): `vessel_raider_tick_one()` rolls one in
  `vessel_raider_ambush_odds()` each tick for a player's hull under way at the
  surface, bigger than a boat, out of port, with no lock set: 2002, halved in a
  pirate cove, doubled in territorial waters (`vessel_piracy_law_for_ship()`),
  times 60 under neutral colors. A merchant class (raft, boat, ship, transport,
  `vessel_merchant_class()`) is ambushed once a voyage: `raided` (runtime) is
  set by the roll and cleared by `vessel_berth()`. `vessel_raider_pick_tier()`
  draws `n = random(0, hull weight) + renown`: merchant classes tier 0 below 250, 1
  below 1,200, 2 three times in four (a hunter one time in three), else a tier
  3 hunter; the rest are noticed when `n >= random(1, 1000)`, and draw a tier
  2 hunter, or tier 3 one time in three.
- Launch: `vessel_raider_spawn()` takes one of the tier's prototypes
  (`vessel_raider_tiers`; `vessel_raider_pick_prototype()` prefers those at
  least as fast as the quarry's design speed less 3) and spawns it through
  `vessel_spawn_public_from_prototype_at()` her sight range plus 10 rooms off
  the quarry's bow within 45 degrees, headed at her at full speed. The tier
  table in code (`raider_tiers[]`) gives the crew (8-12, 9-12, 12-15, 12-18
  mobiles, the captain included), the advanced-AI chance (0, 20, 50, 100%),
  the crew tier (green, green, able, veteran), the fit-out (each weapon
  mounted only while the fit-out stays legal), the chest's gold (800-1,600
  up to 3,000-6,000), and her renown (150-300, 500-600, 700-1,000,
  2,000-3,000, S7). The captain (mobile 70020 plus the tier) is her NPC
  pilot (`vessel_assign_npc_pilot()`), the crew (70024 plus the tier) stand in
  random rooms, and the strongbox (object 70020) lies in the hold, or on the
  bridge, with its key (object 70021) on the captain.
- AI: engaging, a raider marks her quarry as `last_attacker`, so NPC return
  fire works her guns. The basic brain works round a quarry whose facing side
  is shot away, else turns the arc ready within 4 s onto her (the least turn),
  opens the range when a ready gun is too close, turns the arc ready soonest,
  or leads her (`vessel_raider_intercept()`). A weapon's good band is its
  minimum range to a quarter of its maximum, at least a room past the minimum.
  The advanced brain chases beyond 10 rooms; closer it projects both hulls 3 s
  ahead (`vessel_project()`), takes the quarry's weakest side still standing
  and its own arc that reloads soonest (beams first), and turns that arc on
  her once off that side, else steers for a point off it at the arc's range.
  `vessel_raider_set_course()` swings a heading that meets land within the
  lookout (2 rooms engaging, 5 cruising, 10 running) 30 degrees at a time to
  open water, and brakes for land within 2 rooms (speed 1, 6, or 12). Out of
  ammunition or holed she runs; she rams when `vessel_raider_rams()` allows
  (Duris's worth and check tests; a basic brain rams a quarry it cannot board
  one time in three); and she boards a quarry at speed 3 or less (a merchant
  class) or stopped, once: her captain leads the grapple and crossing against
  `vessel_best_boarding_defender()`, and boarders (fresh crew mobiles) take
  the bridge and three quarters (tiers 0-1) or half (tiers 2-3) of the rooms.
  A pirate loots (`vessel_raider_loot()`: of each lot a 40-60% share of what
  is taken is lost and a 40-60% share left, her hold taking what fits) and
  leaves, repelled or not; a hunter fights on. A raider keeps her quarry
  within her sight plus 10 rooms while it is a player's hull afloat, out of
  port, and above water; cruising, she takes the player's hull that last
  fired on her, else the nearest in sight not under neutral colors.
- Leaving: 600 ticks after she loses her quarry (and while leaving after a
  boarding), 20 more at a time while a player is aboard or a player's hull
  is in sight, she takes her crew and everything aboard out of the world and
  `vessel_retire_npc_hull()` removes her. Killing her captain (no pilot on
  the bridge) stops the AI: she heaves to, clears her pilot so return fire
  stops, and counts down. `vessel_raider_handle_sink()` takes her crew down
  with her. Raiders are persisted like any public hull, and
  `vessel_raider_boot()` (from `vessel_hunter_boot()`) retires every unowned
  hull restored from a raider prototype; `claimship` refuses a raider.
- NPC merchants (`merchant_id`) at battle stations with their attacker in
  sight run from it at design speed; the autopilot takes over again when the
  crew stands down.
- Ramming (`vessels_ramming.c`): `shipram` (gunnery permission, a lock, speed
  6, the ram cooldown clear, the consent gate) braces the crew and records who
  gave the order (`ram_order`); `vessel_ram_tick_one()` (combat tick) rams the
  locked contact within a room and stands the crew down when the lock drops,
  she slows to 3, or at the impact the order's giver is offline or fails the
  consent gate against the hull then locked (a lock may change after the
  order). `vessel_ram()`
  is Duris's `try_ram_ship()`: a 120-degree bow cone at one altitude, speed
  above 3, the sailmaster's 1-3 gain, the target's speed along her heading
  taken off the blow (refused if she outruns it), `vessel_ram_chance()`, then
  `(hull + 100) / 10` crash damage each way scaled by the speeds and 1.2 on a
  bow, a fitted ram's own weight first (80-120%, 60-100% back from a rammed
  ram), 2-6 point hits shared in proportion (a tenth on the sails of the
  lighter hull, half on a ram-fitted bow), the lighter hull slewed and both
  slowed to 3 (a lighter rammer stops), Reflex knockdowns on both, and
  cooldowns of 100 ticks after a hit, 50 after a miss, less 15% of the crew
  mod, with guns silent 50 ticks after a hit. A braced or reeling crew does
  not reload (`vessel_reload_tick()`).
- Staff: `vesseldebug raider <0-3> [hunter]` launches a raider against the
  player's hull the staff member is aboard.
- Persistence (Phase 22): `vessel_raider_tiers (tier, prototype_id)`, created
  by `vessel_raider_ensure_schema()` from `vessel_hunter_ensure_schema()`; all
  raider and ram state is runtime only.

### Rewards, Renown and Contraband (S7)

Study sections 3.3.7 (rewards, renown, Ship Damage Control) and 3.3.9
(contraband, customs, cargo sales), with the renown carry-overs of S4-S6.

- Renown (`vessels_rewards.c`): `renown` on the hull, persisted in
  `ship_runtime_state` and untouched by `vessel_rebuild_hull()`, so it survives
  the wreck registry and a trade-in. `shipcrew` shows it; `shiprenown` lists
  the ten players' hulls with the most, afloat or stowed.
- Sinking rewards: `vessel_sink()` calls `vessel_settle_sinking()` with her
  victor (`last_attacker`) before evacuating her. A victor that is a player's
  hull of another owner shares with every player's hull afloat and out of
  port (`vessel_ship_is_in_port()`, as Duris passes over docked ships) within
  the sinking hull's `vessel_sight_range()` whose owner is online and in the
  victor's online owner's group; hulls of the target's owner never share.
  Every player enters the game in a group of one, so the victor's owner's
  other hulls at sea in sight share too. The sharers split, equally, salvage
  (`vessel_salvage_value()`: `vessel_hull_price()` times armor and structure
  left over their maximum, plus half the price of each weapon below
  `VESSEL_WEAPON_DESTROYED`, divided by 8), the renown bounty (2.5 gold a
  point above 100 renown, raiders included), and the target owner's bounty
  (`vessel_get_bounty()` of 500 or more, when the owner is aboard;
  `vessel_clear_bounty()` collects it). Each share goes to the sharing hull's
  owner as a claim (`vessel_queue_claim()`: `vessel_insurance_claims` with a
  mail receipt), delivered at once to an online owner or at login. When the
  target is a player's hull, each sharer gains her class hull weight divided
  among them and she loses it, floored at 0; `vessel_wreck_hull()` then takes
  10% plus 1% per 30 of it from her crew. NPC kills move no renown. One
  transaction records the claims, the bounty's collection, the renown, and
  her `last_attacker` set to 0, so a hull saved while sinking and restored
  before her wreck was saved sinks again with no victor and pays nothing
  twice; if it fails, nothing is paid. For the same reason `vessel_sink()`
  settles before `vessel_crew_credit_kill()` trains the victor's crew.
- Renown gates: `vessel_crew_hire_renown()` (able 540/700/640/540, veteran
  1,350/1,640/1,480/1,350 for sailmaster, gunner, bosun, quartermaster) in
  `shiphire`; a capital weapon's `renown` (1,600 light beam, 1,700 mind blast,
  1,800 heavy beam, 1,900 fragmentation, 2,000 Long Tom) or a veteran gunner
  in `shipweapon buy`.
- Raiders carry a random renown of their tier and the tier roll adds the
  quarry's renown (Raiders and Ramming above).
- Ship Damage Control: `FEAT_SHIP_DAMAGE_CONTROL`, an epic general feat of 5
  ranks. `vessel_damage_hull()` and `vessel_damage_sail()` take
  `4 + 4 * rank` percent off each blow while the owner is aboard with it
  (`vessel_owner_aboard()`), the fraction as the chance of one more point,
  never below 1 (Duris `epic_ship_damage_control()`).
- Contraband (`vessels_trade.c`): `trade_commodities.contraband_renown` above
  0 marks a good and is the renown needed to buy it. A port stocks it only
  with a `port_commodities` row, which only content creates: `port_supply()`
  answers `TRADE_SUPPLY_MIN` (the scarce price) without seeding a row, and
  `cargosell` pays that price for every unit, however the hold is split,
  leaving no row. `market` shows `-` to buy and `none (contraband)`
  where it is not stocked; `cargobuy` sells it only where stocked
  (`port_stocks()`), and, staff aside, only to a hull of its renown or with an
  able sailmaster and quartermaster, never to a warship or a buyer at
  alignment `VESSEL_CONTRABAND_ALIGNMENT` (1,000).
- Customs: `vessel_update_port_berth()` calls `vessel_customs_inspection()`
  when a hull sails into a port room from outside one. At a lawful port (not
  `vessel_piracy_wanted_port_is_open()`), for a player's hull, each lot of
  contraband the port is known not to stock (a failed `port_stocks()` lookup
  lets the lot pass) loses each unit with
  `vessel_customs_chance()` (`35 + units / 2 - sqrt(renown) / 5`, raised by
  `(100 - c) * (1 - load)`, at most 100, never under 5), load being cargo
  weight over capacity; the hold is then saved.
- Cargo sales: `vessel_cargo_sale_factor()` multiplies `cargosell` revenue by
  1.1 for a seller with SEADOG, 0.9 under neutral colors, and 0.6 for a
  warship, on every good.
- Persistence (Phase 23): `ship_runtime_state.renown` and
  `trade_commodities.contraband_renown`. Contraband content: see Contraband
  Content below.

### Builder Commands (Phase 04)

| Command | Description | Usage |
| -- | -- | -- |
| vedit | Ship prototype editor (LVL_BUILDER) | `vedit list/new/show/set/delete/spawn/spawnpublic` |

`vedit new <class 0-7> <name>` creates a prototype in `ship_prototypes` with
class defaults; `vedit set <id> name/class/speed/armor <value>` tunes it,
`vedit set <id> forsale yes|no` lists it in the shipyard, and
`vedit set <id> minlevel <0-30>` sets its departure level (0 keeps the class
minimum);
`vedit spawn <id>` instantiates a live, boardable ship at the builder's
location and assigns the builder as owner. `vedit spawnpublic <id>` uses the
same atomic spawn path but leaves the ship unclaimed for an NPC or public
route, so it has no player-owner dock fees. Both paths allocate a free slot,
generate the interior, link the exterior object, and persist the complete
instance before reporting success.

On local development, the complete builder gate is:

```bash
./scripts/development/dev_kohdee_login_smoke.sh --vessel-builder-check
```

It uses one actual Kohdee session, parses the generated IDs from in-game
output, verifies a one-cell sail, and removes the disposable hull and
prototype. The July 30, 2026 run took 2.7 seconds in game and 8 seconds
including login and clean account logout, well inside the 15-minute builder
independence budget.

Interior room text comes from `ship_room_templates`. DG trigger attachments
come from `ship_room_template_triggers`, keyed by generated room type. Changes
to either table take effect on the next boot; compiled-in room templates
remain the MySQL-unavailable fallback. The text carries the hull's name, and
`shipchristen` renders it again with the new one.

A hull of up to three rooms is a line running north from the bridge. A larger
hull puts each room on one of eight level rays out from the bridge (north,
east, south, west, then the diagonals): the first eight beside it, any more one
room further out along the same rays. Side passages join only rooms that lie
next to each other, so every exit agrees with where its rooms lie and the
minimap draws the interior as built. Interiors persisted before this layout
keep their stored passages.

Generated-room trigger mappings are shared by room type, so content-specific
DG programs must prove that the generated room belongs to their intended hull
before changing player state. Blackwake's bridge trigger checks the bridge
name directly; its quarters and cargo triggers resolve the linked bridge and
check that identity. This keeps the globally mapped VNUMs inert on unrelated
ship-class interiors.

### NPC Pilot Commands

| Command | Description | Usage |
| -- | -- | -- |
| assignpilot | Assign NPC pilot | `assignpilot <npc>` |
| unassignpilot | Remove NPC pilot | `unassignpilot` |

### Schedule Commands

| Command | Description | Usage |
| -- | -- | -- |
| setschedule | Set schedule and optional public fare | `setschedule <route> <interval> [fare]` |
| clearschedule | Clear schedule | `clearschedule` |
| showschedule | Display schedule | `showschedule` |

### Vehicle Commands

| Command | Description | Usage |
| -- | -- | -- |
| vmount | Mount vehicle | `vmount <vehicle>` |
| vdismount | Dismount vehicle | `vdismount` |
| drive | Drive vehicle | `drive <direction>` |
| vstatus | Vehicle status | `vstatus` |
| loadvehicle | Load onto vessel | `loadvehicle <vehicle>` |
| unloadvehicle | List the vehicles aboard, or unload one by its list number | `unloadvehicle [<number>]` |

### Unified Transport Commands

| Command | Description | Usage |
| -- | -- | -- |
| transport_enter | Enter the vehicle named, or the transport here | `tenter [<vehicle>]` |
| exit_transport | Exit transport | `texit` |
| transport_go | Move transport | `tgo <direction>` |
| transportstatus | Transport status | `tstatus` |

On a land vehicle `tgo` is `drive` (`do_transport_go()` calls `do_drive()`), so
it carries every rider along with the vehicle.

---

## Integration Testing Workflows

### Complete Vessel Workflow

1. **Create** - Load ship via OLC or admin command
2. **Board** - Player boards vessel (`board ship`)
3. **Navigate** - Cast off (`undock`), then order heading and speed
   (`heading 90`, `speed 15`)
4. **Move** - Ship gathers way and sails its heading across the wilderness grid
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
3. Bring the vessel to rest at the surface beside the empty vehicle
4. Load vehicle onto vessel from aboard (`loadvehicle wagon`)
5. Sail to destination and stop at the surface
6. Unload vehicle by its list number (`unloadvehicle`, then `unloadvehicle 1`)
7. Mount and drive the vehicle ashore (`vmount wagon`, `drive north`)

### Autopilot Workflow

1. Create waypoints at key locations
2. Create route with ordered waypoints
3. Assign route to vessel
4. Enable autopilot
5. Vessel follows route automatically
6. If terrain rejects a room, correct the route and resume
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
| -- | -- | -- | -- |
| Vessel | 2,672 bytes | 500 | About 1.27 MiB |
| Vehicle | 152 bytes | 1,000 | About 148 KB |
| Autopilot | 80 bytes | Optional per vessel | Up to about 40 KB |
| Schedule | About 32 bytes | Optional per vessel | Up to about 16 KB |

### Structure Sizes

| Structure | Size |
| -- | -- |
| `struct greyhawk_ship_data` | 2,672 bytes |
| `struct vehicle_data` | 152 bytes |
| `struct waypoint` | 88 bytes |
| `struct ship_route` | 1840 bytes |
| `struct autopilot_data` | 80 bytes |
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
| -- | -- | -- |
| `GREYHAWK_MAXSHIPS` | 501 | Fleet-slot array entries, including reserved slot 0 |
| `GREYHAWK_ACTIVE_SHIP_CAPACITY` | 500 | Maximum concurrent active vessels |
| `MAX_SHIP_ROOMS` | 20 | Maximum interior rooms per vessel |
| `MAX_DOCKING_RANGE` | 2.0 | Maximum distance for docking |
| `VESSEL_SPEED_PER_ROOM` | 90.0 | A hull covers speed / 90 rooms per 0.5 s tick |
| `VESSEL_SPEED_LIMIT` | 30 | Highest design or maximum speed |
| `VESSEL_MANEUVER_MAX_SPEED` | 6 | Highest speed for a `setsail` maneuver across the map |
| `VESSEL_MANEUVER_COOLDOWN_TICKS` | 10 | Ticks (5 s) between maneuvers |
| `VESSEL_UNDOCK_TICKS` | 60 | Ticks (30 s) to cast off from a berth |
| `VESSEL_WEIGH_ANCHOR_TICKS` | 26 | Ticks (13 s) to weigh anchor |
| `ABILITY_BOARDING` | 27 | Dedicated trained ability used on both sides of hostile boarding |
| `BOARDING_CRITICAL_MARGIN` | 10 | Defeat margin that makes a failed crossing critical |
| `BOARDING_DEFENSE_MIN` | -8 | Minimum target-vessel defense modifier |
| `BOARDING_DEFENSE_MAX` | 15 | Maximum target-vessel defense modifier |
| `SHIP_INTERIOR_VNUM_BASE` | 70000 | Start of interior room VNUMs |
| `SHIP_INTERIOR_VNUM_MAX` | 80019 | End of interior room VNUMs |

---

## Database Schema

### Tables (Auto-created at startup)

| Table | Purpose |
| -- | -- |
| `ship_prototypes` | Builder-authored hull definitions used by `vedit` and shipyards; `for_sale` and `min_level` since Phase 18, `armor_scale` since Phase 19 |
| `ship_interiors` | Vessel identity, rooms, cosmetics, owner, and upgrades (retired `wages_owed` and `insured_for` columns unread) |
| `ship_runtime_state` | Live hull, position, condition (`condition_model` and `sink_ticks` since Phase 19), stowed state (`stowed`, `wreck_hull`, `summon_due` since Phase 21), `renown` (Phase 23), room type, autopilot, PvP grace, and dock-fee snapshot |
| `ship_weapons` | Every weapon and equipment slot: type, arc, reload state, `weapon_damage` (Phase 19), and `catalog_id` and `ammo` (Phase 20) |
| `ship_docking` | Active and historical docking relationships |
| `ship_room_templates` | Builder-editable generated interior text |
| `ship_room_template_triggers` | DG trigger VNUMs attached to generated room types |
| `ship_cargo_manifest` | Object cargo and bulk commodity lots |
| `ship_crew_roster` | Hired crew (with `experience` since Phase 21) and helm permits |
| `ship_waypoints` | Persistent named navigation points; `creator_id` (Phase 24) |
| `ship_routes` | Persistent route identities; `creator_id` (Phase 24) |
| `ship_route_waypoints` | Ordered waypoint membership for routes |
| `ship_schedules` | NPC-pilot and ferry schedule state, including passenger fare |
| `trade_commodities` | Commodity definitions and base values; `contraband_renown` (Phase 23) marks contraband |
| `port_commodities` | Per-port supply and local price state; a contraband row is the port's stock |
| `freight_contracts` | Freight offer and acceptance lifecycle |
| `vessel_bounties` | Piracy bounty, decay clock (`last_offense_at`), and marque state |
| `vessel_region_law` | Legal-water metadata keyed to canonical geographic regions |
| `vessel_encounters` | Region-keyed encounter definitions |
| `vessel_insurance_claims` | Pending, paid, or void settlements: insurance, S5 premium refunds, and S7 prize money |
| `vessel_npc_merchants` | NPC merchant prototype, route, cargo, faction, schedule, and live generation |
| `vessel_merchant_consequences` | Deduplicated faction and bounty events with delivery state |
| `vessel_hunter_encounters` | Hunter warship, pilot, bounty, pursuit, duration, grace, and cooldown policy |
| `vessel_bounty_hunts` | One durable hunter generation and terminal cooldown per target player |
| `vessel_showcase_events` | Event type, course, staff owner, lifecycle, timing, and terminal reason |
| `vessel_event_participants` | Per-event hull, captain, team, score, finish, placement, and status |
| `vessel_event_leaderboards` | Durable entries, wins, points, and best regatta time per captain and type |
| `vessel_event_runtimes` | Temporary ghost-hull ownership used by cleanup and boot recovery |
| `vessel_raider_tiers` | The raider prototypes each raider tier sails (Phase 22) |

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
./scripts/vessels/provision_vessel_harbor.sh
```

The command refuses to run unless `lib/.env` contains
`APP_ENV=development`. It merges only missing records into the ignored live
world files, extends the reserved zone 700 upper bound from 79999 to 80019
when needed, applies Phases 11-15 and 19 and the development seed, restarts the
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
./scripts/vessels/provision_vessel_campaign.sh
```

It anchors North Vailand Sea Port 1000360 at `(-599, 455)` and Central Vailand
Sea Port 1000362 at `(-467, 204)`. Geographic regions 1000013-1000016 define
North and Central Vailand territorial waters, the Vailand Passage free seas,
and the Blackwake Anchorage pirate cove. Their law rows apply bounty rates of
150, 150, 100, and 0 percent with overlap priorities that keep the port and
pirate identities authoritative inside the wider passage.

The accompanying world package places a conspicuous free waystone in Mosswood
room 145200 to North Vailand, return waystones at the scheduled ports, and
passage boards that identify the Trader and work passage. This makes the real
campaign route discoverable and affordable to a new zero-gold character
without a staff movement command or the development harbor fixture.

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
./scripts/vessels/test_vessel_merchant_in_game.sh \
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
./scripts/vessels/provision_vessel_derelict.sh
./scripts/vessels/test_vessel_derelict_in_game.sh
```

`lib/world/vessel_derelict/700.obj` defines an ash-stained captain log, a
salt-stiff chart, and a bronze tidefinder salvage object. Trigger VNUMs
70010-70012 attach to the generated bridge, crew quarters, and main cargo
hold; object triggers 70013-70014 make the recovered log and chart readable.
The chain requires the player to recover and read the log before finding the
chart, study the chart before opening the cargo panel, and can award each
object only once. It begins with a plain `search` on the derelict's bridge
(trigger 70010 answers `search` and anything it begins, such as
`searchashlog`; elsewhere it returns 0 and the ordinary search runs); the log
and chart then name the next commands. Five player DG variables persist
discovery state in the ASCII player file. The ordinary `salvage` command values the tidefinder at 180
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
./scripts/vessels/provision_vessel_frontier.sh
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
| -- | -- | -- |
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

The supervised ferry gate takes about ten minutes, including its final restart
and cleanup. Its defaults observe for 450 seconds, which covers the four-minute
ferry loop with a margin:

```bash
./scripts/vessels/run_vessel_ferry_soak.sh start
./scripts/vessels/run_vessel_ferry_soak.sh status
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
database and process invariants every 30 seconds and serializes its Kohdee
checks through the shared login-helper lock. It also fails on a PID change,
route/room/pilot/schedule drift, structure loss, out-of-corridor coordinates,
an installed-binary fingerprint change, or a ferry-specific
movement/persistence error. Launch metadata records the source commit and
`bin/luminari` SHA-256. A successful run ends with a controlled local restart
that proves exact paused-coordinate recovery and launches the identical
executable hash, then resumes the ferry. Artifacts live in the run directory
printed by `start`. A failure writes terminal status before cleanup so an
interrupted cleanup cannot leave a stale `RUNNING` result.

### NPC Raider Content

Raiders (S6) sail six prototypes after Duris's raider hulls, none for sale:
the Corsair Clipper, Ketch, and Caravel (ship class) and the Corsair Corvette,
Destroyer, and Frigate (warship class). `vessel_raider_tiers` sets the tiers
each sails: the clipper, ketch, and caravel tier 0; the ketch, caravel, and
corvette tier 1; the corvette and destroyer tier 2; the destroyer and frigate
tier 3. Their captains, crews, strongbox, and key are zone 700 records in
`lib/world/vessel_raiders/`. A server needs all three before any raider sails:
apply `sql/components/vessels_phase22_schema.sql` (boot also creates the
table) and `sql/components/vessels_raider_content.sql`, and merge the mobile and
object records into the live zone 700 files, as `provision_vessel_harbor.sh`
does on a development server. Without them an ambush comes to
nothing (a missing captain is logged).

### Contraband Content

`sql/components/vessels_contraband_content.sql` (after Phase 23) seeds the
three contraband goods of study 3.3.9 and stocks each at one sea port of the
shipped world: forbidden tomes (190 gold, 4 lbs, 150 renown) at Selerish
Slateharbor (1000337), rare poisons (210, 1, 200) at Southwest Quechian
(1000351), and dragon eggs (310, 10, 250) at Koorvik (1000278). Builders may
stock them elsewhere with more `port_commodities` rows. Its rollback removes the
goods, their stock, and any lot of them in a hold; run it before the Phase 23
rollback. The development harbor fixture also stocks forbidden tomes at the
Harbor Sandbox East Dock (1000390) for the economy gate.

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
   stale or expired rows retire safely. The hunter boot then retires every
   raider the restart restored (S6), so raiders never outlive a restart or a
   copyover.
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
   or bounty-hunter lifecycle. A sunk player hull is rebuilt and stowed in the
   wreck registry instead of deleted (S5). Capturing a hunter removes its configured pilot
   and leaves the ordinary captured hull; a raider cannot be captured, and one
   that leaves the sea takes her crew and everything aboard with her.
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
| -- | -- |
| `src/vessels/vessels.h` | Structures, constants, prototypes (includes vehicle definitions) |
| `src/vessels/vessels.c` | Core commands, wilderness position updates, terrain system |
| `src/vessels/vessels_movement.c` | Movement and pacing: class handling, maximum speed, the movement tick, berths, departure, anchor, maneuvers |
| `src/vessels/vessel_periodic.c`, `vessel_periodic.h` | Bounded vessel owner/service deadlines and rollback selection |
| `src/vessels/vessels_tactical.c` | Canonical wilderness chart, range rings, regions, and damage-aware contacts |
| `src/vessels/vessels_lookout.c` | Eight-bearing canonical wilderness lookout and visible-contact roster |
| `src/vessels/vessels_narrative.c` | Class-, speed-, weather-, depth-, and region-aware at-sea prose |
| `src/vessels/vessels_rooms.c` | Interior room generation and movement |
| `src/vessels/vessels_docking.c` | Docking, boarding, and ship-to-ship interaction |
| `src/vessels/vessels_db.c` | MySQL persistence layer |
| `src/vessels/vessels_autopilot.c` | Autopilot, waypoints, routes, NPC pilots, schedules |
| `src/vessels/vessels_edit.c` | vedit ship prototype editor, spawner, shipyard (Phase 04/06) |
| `src/vessels/vessels_combat.c` | Naval combat: gunnery consent, firing, NPC return fire, sinking (Phase 05) |
| `src/vessels/vessels_damage.c` | Class condition profiles, arcs, hull and sail damage, breaches, sinking, salvage, prizes (S3) |
| `src/vessels/vessels_weapons.c` | Weapon and equipment catalogue, class fitting, and the shipyard weapon commands (S4) |
| `src/vessels/vessels_gunnery.c` | Geometry hit model, locks, battle stations, arc fire, sighting, scanning, NPC return fire (S4) |
| `src/vessels/vessels_ownership.c` | Ownership, helm permits, deed transfer (Phase 06) |
| `src/vessels/vessels_crew.c` | Hired crew positions, tiers, one-time hire prices (Phase 06); experience, promotion, casualties, stamina (S5) |
| `src/vessels/vessels_upgrades.c` | Refits, hull wear, the settlement queue for insurance and prize money (Phase 06, S7) |
| `src/vessels/vessels_repair.c` | Repair stores, crew repairs, character and dock repairs (S5) |
| `src/vessels/vessels_loss.c` | Hull value, automatic insurance, stowed hulls, wreck registry, summons, in-place rebuild (S5) |
| `src/vessels/vessels_trade.c` | Commodities, port pricing, bulk cargo (Phase 07); contraband, customs, sale modifiers (S7) |
| `src/vessels/vessels_rewards.c` | Renown, the rewards of a sinking, and the renown board (S7) |
| `src/vessels/vessels_contracts.c` | Freight boards and contract lifecycle (Phase 07) |
| `src/vessels/vessels_piracy.c` | Plunder, bounty, letters of marque (Phase 07) |
| `src/vessels/vessels_merchants.c` | NPC merchant definitions, assembly, consequences, and respawn (Phase 14) |
| `src/vessels/vessels_hunters.c` | HUNTED encounter policy, pursuit, lifecycle, and reconciliation (Phase 15); the shared NPC pilot and retire helpers |
| `src/vessels/vessels_raiders.c` | Ambushes, raider tiers and launch, raider AI, boarding and looting, running merchants, boot retirement (S6) |
| `src/vessels/vessels_ramming.c` | `shipram`, the ram impact, and its cooldowns (S6) |
| `src/vessels/vessels_events.c` | Regattas, skirmishes, ghost fleets, leaderboards, and recovery (Phase 16) |
| `src/vessels/vessels_hazards.c` | Weather hazards, encounters, seastate (Phase 08) |
| `src/vessels/vessels_admin.c` | Operator tooling, room pool monitor, MSDP (Phase 09) |
| `src/vessels/vessels_balance.c` | Read-only duel, economy, cost, and persisted-sample diagnostics |
| `src/vessels/vehicles.c` | Vehicle lifecycle, state management, persistence |
| `src/vessels/vehicles_commands.c` | Player commands (vmount, vdismount, drive, vstatus) |
| `src/vessels/vehicles_transport.c` | Vehicle-in-vessel mechanics (loading/unloading) |
| `src/vessels/transport_unified.c` | Unified transport interface across all transport types |
| `src/vessels/transport_unified.h` | Transport abstraction types and prototypes |

### Legacy, Converted, and Fast-Travel Code

| File | Purpose |
| -- | -- |
| `src/vessels/vessels_legacy.c`, `vessels_legacy.h` | Legacy route, ferry, and Greyhawk ship special procedures, including `greyhawk_ship_object` boarding |
| `src/vessels/vessels_moving_rooms.c`, `vessels_moving_rooms.h` | Legacy world `M` moving-room loading, scheduling, and relocation |
| `src/vessels/moving_room_events.c`, `moving_room_events.h` | Game-scheduler relocation events for moving rooms |
| `src/vessels/vessels_rol.c`, `vessels_rol.h` | Converted Realms of Luminari fixed-interior ship procedures and their periodic owner |
| `src/vessels/transport.c`, `transport.h` | Carriage, sailing, and overland-flight fast travel (`landmarks`) |
| `src/vessels/routing.c`, `routing.h` | Fast-travel locale name and destination lookups |
| `src/vessels/transport_jobs.c`, `transport_jobs.h` | Timed fast-travel trip arrival events and cancellation |

### Content and Development Acceptance

| File | Purpose |
| -- | -- |
| `lib/world/vessel_harbor/` | Shared development harbor zone, rooms, mobiles, and triggers |
| `scripts/vessels/provision_vessel_harbor.sh` | Development-only harbor provisioning and verification; also installs the raider package and content, Phase 23, and the contraband content |
| `lib/world/vessel_raiders/` | Raider captains and crews (mobiles 70020-70027), strongbox and key (objects 70020-70021) |
| `lib/world/vessel_campaign/` | Vailand campaign waystones, passage boards, and resets |
| `scripts/vessels/provision_vessel_campaign.sh` | Development-only campaign world/SQL provisioning and actual-Kohdee check |
| `lib/world/vessel_derelict/700.obj` | Blackwake log, chart, and tidefinder objects |
| `lib/world/vessel_derelict/700.trg` | Guarded room and object discovery-chain DG programs |
| `scripts/vessels/provision_vessel_derelict.sh` | Development-only world/SQL provisioning and restart proof |
| `scripts/vessels/test_vessel_derelict_in_game.sh` | Reversible actual-character discovery and persistence gate |
| `scripts/vessels/provision_vessel_frontier.sh` | Development-only trench, river, skyway, and sky-island provisioning plus piloted acceptance |
| `scripts/vessels/test_vessel_events_in_game.sh` | Reversible Kohdee regatta, skirmish, ghost-fleet, and leaderboard gate |
| `scripts/vessels/test_vessel_tactical_in_game.sh` | Reversible Kohdee wilderness-chart, live-contact, and coastal-symbol gate |
| `scripts/vessels/test_vessel_lookout_in_game.sh` | Reversible Kohdee lookout, cosmetics, contact, and coastal-sector gate |
| `scripts/vessels/test_vessel_narrative_in_game.sh` | Reversible Kohdee at-sea and forced-ambient narrative gate |
| `scripts/vessels/test_vessel_boarding_in_game.sh` | Boarding gate; delegates to the shared tactical acceptance harness |
| `scripts/vessels/test_vessel_rules_in_game.sh` | Two-character shipyard, contact-ID, gunnery, hull-level, route-ownership, hull-cap, and bounty gate; delegates to the shared tactical harness |
| `scripts/vessels/test_vessel_loss_in_game.sh` | Two-character crew hiring, rename fee, summons, and trade-in gate; delegates to the shared tactical harness |
| `scripts/vessels/test_vessel_economy_in_game.sh` | Two-character contraband, customs, sale modifier, prize money, and renown gate; delegates to the shared tactical harness |
| `scripts/vessels/test_vessel_client_in_game.sh` | Native MSDP client-data gate at sea and ashore; delegates to the shared tactical harness |
| `scripts/vessels/test_vessel_hunter_in_game.sh` | Reversible Kohdee HUNTED bounty-hunter encounter gate |
| `scripts/vessels/test_vessel_merchant_in_game.sh` | Reversible NPC merchant shipping gate |
| `scripts/vessels/run_vessel_ferry_soak.sh` | Development ferry soak runner with database, process, and Kohdee samples |
| `scripts/vessels/test_vessel_ferry_soak_preflight.sh` | Ferry soak parser and preflight regression |
| `scripts/vessels/run_vessel_scale_benchmark.sh` | Development-only 500-vessel scale workload and evidence runner |
| `scripts/vessels/test_vessel_scale_benchmark_parsers.sh` | Scale benchmark output-parser regression |
| `scripts/vessels/analyze_vessel_memory_samples.sh` | Process memory-sample regression analyzer |
| `scripts/vessels/test_vessel_memory_analyzer.sh` | Fixture-driven memory analyzer regression |

### Database

| File | Purpose |
| -- | -- |
| `src/database/db_init.c` | Table creation (init_vessel_system_tables) |
| `src/database/db_init_data.c` | Template population |
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
| `sql/components/vessels_phase17_*` | Exterior paint and figurehead persistence, verification, and rollback |
| `sql/components/vessels_phase18_*` | Prototype shipyard listing and hull level, wage-debt clearing, verification, and rollback |
| `sql/components/vessels_phase19_*` | S3 damage model: armor rescale flag, condition model, sink timer, weapon damage, verification, and rollback |
| `sql/components/vessels_phase20_*` | S4 weapons: catalogue row and ammunition per slot row, verification, and rollback |
| `sql/components/vessels_phase21_*` | S5 crew experience, stowed-hull state, insurance premium refund, verification, and rollback |
| `sql/components/vessels_phase22_*` | S6 raider tier table, verification, and rollback |
| `sql/components/vessels_phase23_*` | S7 hull renown and the contraband flag, verification, and rollback |
| `sql/components/vessels_phase24_*` | S12 waypoint and route creators, verification, and rollback |
| `sql/components/vessels_contraband_content.sql` | Three contraband goods, each stocked at one sea port |
| `sql/components/verify_vessels_contraband_content.sql` | Read-only contraband goods and stock checks |
| `sql/components/vessels_contraband_content_rollback.sql` | Contraband goods, stock, and hold lots removal |
| `sql/components/vessels_raider_content.sql` | Six Corsair raider prototypes (not for sale) and their ten tier rows |
| `sql/components/verify_vessels_raider_content.sql` | Read-only raider prototype and tier inventory |
| `sql/components/vessels_raider_content_rollback.sql` | Guarded raider content rollback; a prototype still sailing keeps its tier rows so the restart retires her; rerun after it |
| `sql/components/vessels_campaign_content.sql` | Initial Vailand regions, law, route, merchant, and iron markets |
| `sql/components/verify_vessels_campaign_content.sql` | Read-only campaign topology and identity checks |
| `sql/components/vessels_campaign_content_rollback.sql` | Guarded Vailand content rollback |
| `sql/components/vessels_derelict_content.sql` | Blackwake prototype and generated-room trigger mappings |
| `sql/components/verify_vessels_derelict_content.sql` | Read-only Blackwake identity and mapping checks |
| `sql/components/vessels_derelict_content_rollback.sql` | Dependency-aware Blackwake definition rollback |
| `sql/components/vessels_frontier_content.sql` | Starfall, Aetherwind, Shardspire, Sablebranch, and eight prototype definitions |
| `sql/components/verify_vessels_frontier_content.sql` | Read-only frontier geometry, index, and prototype inventory |
| `sql/components/vessels_frontier_content_rollback.sql` | Guarded frontier content rollback |
| `sql/components/vessels_narrative_content.sql` | Eight owned Vailand geographic and severe-weather hints |
| `sql/components/verify_vessels_narrative_content.sql` | Read-only narrative-hint inventory and metadata checks |
| `sql/components/vessels_narrative_content_rollback.sql` | Owner-scoped Vailand narrative rollback |
| `sql/components/help_vessel_entries.sql` | Idempotent authoritative help migration |
| `sql/components/verify_help_vessel_entries.sql` | Read-only help count, access, content, and duplicate checks |
| `sql/components/verify_vessels_schema.sql` | Phase 2 core schema verification |
| `sql/components/verify_vessels_phase*.sql` | Read-only verification for Phases 4 and 6-17 |
| `sql/components/test_vessels_integrity.sql` | Self-cleaning Phase 2 insert, update, delete, and foreign-key checks |
| `sql/components/vessels_harbor_sandbox.sql` | Development-only shared harbor fixture rows |

---

## Dependencies

### External Dependencies

- MySQL/MariaDB 5.7+ (required)
- Wilderness system operational
- Development test content available for manual verification

### Internal Dependencies

| File | Purpose |
| -- | -- |
| `src/wilderness/wilderness.c` | Coordinate system and room allocation |
| `src/core/weather.c` | Weather integration via `get_weather()` |
| `src/vessels/vessels_legacy.c` | Boarding special procedure (`greyhawk_ship_object`), establishes critical linkages |
| `src/core/interpreter.c` | Command registration |
| `src/core/db.c` | Boot sequence integration |
| `src/dgscript/dg_scripts.c/h` | Trigger integration for interior movement |
| `src/database/mysql.c` | Persistence layer (required) |

### Reserved Resources

- **VNUM Range 70000-80019:** Reserved for dynamic ship interior rooms; zone 700 must own
  the complete range
- **Item Type 56:** ITEM_GREYHAWK_SHIP
- **Room Flags:** ROOM_VEHICLE (40), ROOM_DOCKABLE (41)

### Test Zones

**Zone 213** (Legacy test zone):

| VNUM | Purpose |
| -- | -- |
| Room 21300 | Dock room with DOCKABLE flag |
| Room 21398 | Ship interior (control room) |
| Room 21399 | Additional ship interior |
| Object 21300 | Test ship (ITEM_GREYHAWK_SHIP, boarding functional) |

**Zone 700** (Current test zone - see [VESSEL_SYSTEM_TESTING.md](../testing/VESSEL_SYSTEM_TESTING.md)):

| VNUM | Purpose |
| -- | -- |
| Object 70002 | Generic hull prototype: every hull is an instance, given its name, descriptions and ITEM_GREYHAWK_SHIP type at spawn and restore, without the fixture's glow and hum |
| Room 70003 | Test vessel interior room |
| Room 1000389 | Wilderness dock location at (-66, 92) |

---

## Troubleshooting

### Quick Reference

| Issue | Check First | Solution |
| -- | -- | -- |
| Vessel not moving | Speed, dock status | `undock`, `speed 10`, `autopilot resume` |
| Cannot board | Room DOCKABLE flag | Move to dock room, check `entrance_room` |
| Interior nav fails | Room connections | `ship_rooms` to verify, regenerate if needed |
| Autopilot paused at blocked route | Route waypoints and hull speed | `listwaypoints`, correct unreachable terrain/Z, set speed, then `autopilot resume` |
| Vehicle terrain blocked | Vehicle type | Use MOUNT for hills/mountains |
| Coordinate desync | shipobj linkage | `greyhawk_shipload` (admin), check `src/vessels/vessels_legacy.c` |
| Disembark fails | Interior room link | Verify `world[room].ship` is set |
| Ship object doesn't move | shipobj not linked | Set `greyhawk_ships[idx].shipobj = obj` |

### Database Issues

| Issue | Cause | Solution |
| -- | -- | -- |
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
declared in `src/vessels/vessels.h`. `VESSEL_SYSTEM_DEBUG` defaults to `0`, so normal
builds compile out every diagnostic call site. An explicit development build
enables support, but its runtime category mask still starts empty.

```c
/* src/vessels/vessels.h */
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
| -- | -- |
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
grep "\[VESSEL_MOVE\]"   syslog   # movement, terrain, refused rooms
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
   maintained help entries and 81 command keywords to pass both SQL and in-game
   checks.
5. Verify reboot and copyover while under way, in combat, and carrying cargo.
6. Pass the 500-vessel, 25 ms tick measurement and supervised stability check;
   each complete validation must finish within one hour.
7. Rehearse schema migration and rollback against a production snapshot.

The current candidate passes these engineering and operator prerequisites.
They remain candidate-specific and must be repeated after relevant behavior or
schema changes. Human beta, player-data balance, and rollout state are tracked
in the
[Vessel System Product Requirements](../product-requirements/VESSEL_SYSTEM_REQUIREMENTS.md).

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
- `perfmon entities` shows the vessel periodic mode, owner registry,
  validation mismatches, capacity rejections, owner/service callbacks, fixed
  RoL ownership, and fast/mud-hour execution counts in an 80-column block.
- `vmerchant list` shows every configured merchant generation, lifecycle
  state, live hull, cargo mapping, loss count, and reconciliation error.
- `vessel_bounty_hunts` exposes each hunter's target, generation, active slot,
  expiry, cooldown, and terminal reason; investigate any long-lived
  `spawning` row or active row whose fleet identity no longer matches.
- `vevent status` exposes the live event. Investigate `recovery_failed` event
  rows or any `vessel_event_runtimes` row whose hull identity is missing.
- Vessel debug categories provide focused development diagnostics. Candidate
  and production builds must report that support is compiled out.
- Monitor database errors, orphan cleanup, trade ticks, encounter spawn
  volume, and game-loop latency.
- Treat room-pool pressure over 80%, tick time over 25 ms, or repeated
  persistence errors as rollout-stop conditions.

---

## Risk Assessment

| Risk | Impact | Mitigation |
| -- | -- | -- |
| Fleet-slot and object identity diverge | Commands address the wrong ship or lose the exterior object | Keep one canonical slot identity, validate every boundary, cover legacy and `vedit` paths |
| Shared wilderness rooms are exhausted | Vessels interfere with all wilderness travelers | Monitor pool pressure, reclaim rooms, and test graceful degradation |
| Due vessel work exceeds 25 ms | Game-loop latency at fleet scale | Benchmark all live subsystems together at 500 ships and inspect owner/service attribution |
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

# Install the tested server and remove the root-level luminari artifact
make install

# Full local gate, including focused protocol and schema checks
cd unittests/CuTest
make test-all
```

Vessel, autopilot, and vehicle behavior belongs in the production-linked root
suite. Do not recreate the removed standalone mirror implementations.

Primary automated coverage lives in
`unittests/CuTest/test_transport_production.c` and exercises production
functions linked with all game sources. `test_vessel_gunnery.c` covers gunnery
authorization, owner consent, harbor immunity, and the contact list;
`test_vessel_shipyard.c` covers hull levels, the owned-hull cap, and the
for-sale catalog; `test_vessel_bounty.c` covers bounty decay and pay-off. Their
database cases need `LUMINARI_TEST_MYSQL_ENABLE=1`. Manual world, command, persistence, OLC,
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

- [Vessel System Product Requirements](../product-requirements/VESSEL_SYSTEM_REQUIREMENTS.md) -
  Durable requirements, release criteria, and gate state
- [VESSEL_BENCHMARKS.md](../testing/VESSEL_BENCHMARKS.md) - Performance data and memory attribution
- [the archived changelogs](../previous_changelogs/) - What shipped when
- [VESSEL_SYSTEM_TESTING.md](../testing/VESSEL_SYSTEM_TESTING.md) - 30-step manual regression script
- [0001-unified-vessel-system.md](../adr/0001-unified-vessel-system.md) - Architecture decision and invariants
- [TECHNICAL_DOCUMENTATION_MASTER_INDEX.md](../TECHNICAL_DOCUMENTATION_MASTER_INDEX.md) - Complete docs index

---

*Behavior reference; update it whenever vessel behavior changes.*
