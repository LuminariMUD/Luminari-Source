# vessels-ships.md improvement pass based on duris code

The goal is to improve the vessel/ship system implementation BASED on DurisMUD system.

## DurisMUD Vessel / Ship Code

### DurisMUD Location `/home/aiwithapex/projects/duris/`

### Location of DurisMUD Vessel / Ship Documentation + Code

Each C file in `/home/aiwithapex/projects/duris/src/ships/` carries an authored `OVERVIEW` block,
`/home/aiwithapex/projects/duris/src/ships/ships.h` opens with the module map, and
`/home/aiwithapex/projects/duris/docs/reference/SHIPS.md` organizes the module dependencies. Start
with the header, then the core utilities.

```
/home/aiwithapex/projects/duris/docs/reference/SHIPS.md
/home/aiwithapex/projects/duris/docs/reference/SHIP_GAMEPLAY.md
/home/aiwithapex/projects/duris/lib/information/helpships
/home/aiwithapex/projects/duris/lib/information/help_index
/home/aiwithapex/projects/duris/docs/reference/ARCHITECTURE.md
/home/aiwithapex/projects/duris/docs/reference/CODEBASE.md
/home/aiwithapex/projects/duris/docs/README_docs.md
```

#### Dedicated Documentation Files

- `/home/aiwithapex/projects/duris/docs/reference/SHIPS.md` - Engineering reference (lifecycle,
  combat, movement, crew, persistence, AI, GMCP)
- `/home/aiwithapex/projects/duris/docs/reference/SHIP_GAMEPLAY.md` - Player and staff reference
  (commands, hulls, weapons, equipment, crew, trade)
- `/home/aiwithapex/projects/duris/lib/information/helpships` - In-game player help manual (SHIP1
  through SHIP5)
- `/home/aiwithapex/projects/duris/lib/information/help_index` - Runtime help index entries for
  ship commands and systems (`SHIP SHIPS` through `SHIP_LOOKS`)

#### Subsystem Architecture References

- `/home/aiwithapex/projects/duris/docs/reference/ARCHITECTURE.md:299-308` - Section: Ships
- `/home/aiwithapex/projects/duris/docs/reference/CODEBASE.md:106-119` - Section: Ships subsystem
- `/home/aiwithapex/projects/duris/docs/README_docs.md:33-34` - Ship documentation catalog entries

#### In-Source Subsystem Overview Documentation

(Each `.c` file contains an authored `OVERVIEW` header explaining that module's place in the ship
subsystem; `/home/aiwithapex/projects/duris/src/ships/ships.h` holds the public data model and
module map.)

- `/home/aiwithapex/projects/duris/src/ships/ships.h`
- `/home/aiwithapex/projects/duris/src/ships/ship_base.c`
- `/home/aiwithapex/projects/duris/src/ships/ship_combat.c`
- `/home/aiwithapex/projects/duris/src/ships/ship_control.c`
- `/home/aiwithapex/projects/duris/src/ships/ship_cargo.c`
- `/home/aiwithapex/projects/duris/src/ships/ship_shop.c`
- `/home/aiwithapex/projects/duris/src/ships/ship_auto.c`
- `/home/aiwithapex/projects/duris/src/ships/ship_npc.c`
- `/home/aiwithapex/projects/duris/src/ships/ship_npc_ai.c`
- `/home/aiwithapex/projects/duris/src/ships/ship_utils.c`
- `/home/aiwithapex/projects/duris/src/ships/ship_identity.c`
- `/home/aiwithapex/projects/duris/src/ships/ship_variables.c`

## LuminariMUD Vessel / Ship Code

### LuminariMUD Location `/home/aiwithapex/projects/Luminari-Source/`

Paths below are relative to this repository root.

### Location of LuminariMUD Vessel / Ship Documentation + Code

One unified vessel system ([ADR 0001](../adr/0001-unified-vessel-system.md)) lives in
`src/vessels/`. It covers ships, airships, submarines, and land vehicles on the shared wilderness
map, plus the legacy Greyhawk ship, moving-room, and carriage travel code. `VESSEL_SYSTEM.md` is the
current-behavior reference; `VESSEL_SYSTEM_REQUIREMENTS.md` holds the authoritative release-gate
state (player-data balance and structured human beta are Open; staged production rollout is
Blocked). Start with `src/vessels/vessels.h`, then `src/vessels/vessels.c`.

```
docs/systems/VESSEL_SYSTEM.md
docs/product-requirements/VESSEL_SYSTEM_REQUIREMENTS.md
docs/adr/0001-unified-vessel-system.md
docs/deployment/VESSEL_SCHEMA_DEPLOYMENT.md
docs/testing/VESSEL_SYSTEM_TESTING.md
docs/testing/VESSEL_BENCHMARKS.md
docs/testing/vessel_test_results.md
lib/text/help/help.hlp
sql/components/help_vessel_entries.sql
```

#### Dedicated Documentation Files

- `docs/systems/VESSEL_SYSTEM.md` - Engineering and command reference (architecture, data
  structures, vessel types, API, commands, database schema, content packages, file inventory,
  troubleshooting, operations)
  - `:99-245` Architecture (scheduling, wilderness contract, state machine)
  - `:246-339` Data Structures (`greyhawk_ship_data`, `vehicle_data`, autopilot)
  - `:340-403` Vessel Types and Capabilities (classes, terrain, speed modifiers)
  - `:477-1042` Player Commands (navigation, autopilot, operator, living world, events, trade,
    ownership, combat, builder, pilots, schedules, vehicles, unified transport)
  - `:1086-1115` Vehicle-in-Vessel Mechanics
  - `:1172-1544` Database Schema (tables, room templates, harbor, campaign, derelict, frontier,
    persistence lifecycle)
  - `:1545-1660` File Inventory (source, content, acceptance scripts, SQL)
- `docs/product-requirements/VESSEL_SYSTEM_REQUIREMENTS.md` - Product contract: player and staff
  outcomes, functional requirements, wilderness contract, quality, scope, release acceptance, and
  the authoritative Release Gate State (`:263-276`)
- `docs/adr/0001-unified-vessel-system.md` - Decision to replace three vessel systems with one,
  wilderness integration and content invariants, rejected alternatives
- `docs/deployment/VESSEL_SCHEMA_DEPLOYMENT.md` - Schema install, verification, staged rollout, and
  rollback; production-snapshot rehearsal record
- `docs/testing/VESSEL_SYSTEM_TESTING.md` - Manual live command regression (30 steps)
- `docs/testing/VESSEL_BENCHMARKS.md` - Current and historical performance, memory, and test
  evidence
- `docs/testing/vessel_test_results.md` - Historical Phase 00 test record (superseded by
  `VESSEL_BENCHMARKS.md`)
- `docs/testing/event-acceptance-2026-09-05/native-vessel-autopilot.txt` and
  `native-vessel-builder-enabled.txt` - Event-driven core acceptance transcripts

#### Help Sources

- `sql/components/help_vessel_entries.sql` - Authoritative, idempotent database help for every
  vessel, vehicle, transport, autopilot, and vessel staff command;
  `sql/components/verify_help_vessel_entries.sql` checks it
- `lib/text/help/help.hlp` - File mirror of the same entries; keyword lines include
  `BOARD BOARDING ... VESSEL VESSELS`, `TACTICAL`, `AUTOPILOT`,
  `CARGO CARGOBUY CARGOMANIFEST CARGOSELL MARKET SHIP-TRADE`,
  `SEA-STATE SEASTATE SHIP-HAZARDS WEATHER-AT-SEA`, `SHIP-OWNERSHIP SHIPBROWSE ...`,
  `CLAIMSHIP NAVAL-COMBAT SHIP-COMBAT SHIPFIRE SHIPREPAIR`,
  `SHIP-ADMIN SHIPFIX SHIPGOTO SHIPLIST SHIPLOAD SHIPPURGE`, `VEDIT`,
  `VEHICLE-ADMIN VEHICLECREATE VEHICLEPURGE`, `LOADVEH ... VEHICLE-TRANSPORT`, and
  `LAND-VEHICLES VEHICLES VMOUNT`
- `lib/text/help/autopilot.hlp`, `schedule.hlp` (tracked) and `assignpilot.hlp`,
  `unassignpilot.hlp`, `vehicles.hlp` (ignored) - Standalone files; not indexed and not maintained
  sources

#### Subsystem Architecture References

- `docs/TECHNICAL_DOCUMENTATION_MASTER_INDEX.md:40, 87-89, 104-106, 168, 171` - Vessel
  documentation catalog entries
- `docs/systems/ARCHITECTURE.md:31` - Subsystem table row: Wilderness and transport
- `docs/systems/ARCHITECTURE.md:62-64` - Legacy route/ferry/Greyhawk ship and moving-room owners
- `docs/systems/CORE_SERVER_ARCHITECTURE.md:366-370` - Special-procedure ownership of legacy vessel
  and moving-room callbacks
- `docs/guides/DEVELOPER_GUIDE_AND_API.md:304-306` - Vessel callbacks under `src/vessels/`
- `docs/guides/LUMINARI_OVERVIEW.md:141, 573-580` - Player-facing summary: Vehicles and Transport
- `docs/development/CONSIDERATIONS.md:150-254` - Section: Vessel System (active concerns, enduring
  integration rules, resource budgets)
- `docs/docs-audit.md:186-269` - Vessel documentation consolidation audit: which document owns
  which fact
- `docs/guides/TESTING_GUIDE.md:11-12` - Removal of the legacy standalone vessel mirror suites

#### In-Source Subsystem Overview Documentation

(Most files open with a `Purpose:` or `Usage:` header block naming their phase and role;
`vessels.h` is the public data model and API. Commands are registered in `cmd_info[]` in
`src/core/interpreter.c`; the `board` command reaches `do_board_vessel()` through
`src/comms/mysql_boards.c`.)

Core, persistence, and scheduling:

- `src/vessels/vessels.h` - Structures, constants, and prototypes for vessels and vehicles
- `src/vessels/vessels.c` - Core commands, wilderness position updates, terrain (`shipstatus`,
  `speed`, `heading`, `contacts`, `disembark`, `setsail`)
- `src/vessels/vessels_movement.c` - Movement and pacing (S2): class handling, maximum speed, the
  movement tick, berths, departure, the `setsail` maneuver, and `anchor`
- `src/vessels/vessels_rooms.c` - Multi-room interior generation and movement (`shiptalk`)
- `src/vessels/vessels_docking.c` - Docking, boarding, ship-to-ship interaction (`dock`, `undock`,
  `board_hostile`, `ship_rooms`)
- `src/vessels/vessels_db.c` - MariaDB persistence
- `src/vessels/vessel_periodic.c`, `vessel_periodic.h` - Bounded vessel owner and service deadlines
  on the game scheduler

Navigation views and automation:

- `src/vessels/vessels_tactical.c` - Wilderness tactical chart (`tactical`)
- `src/vessels/vessels_lookout.c` - Eight-bearing lookout view (`lookout`, `look_outside`)
- `src/vessels/vessels_narrative.c` - Contextual at-sea prose and ambience
- `src/vessels/vessels_autopilot.c` - Autopilot, waypoints, routes, NPC pilots, schedules
  (`autopilot`, `setwaypoint`, `listwaypoints`, `delwaypoint`, `createroute`, `addtoroute`,
  `delroute`, `listroutes`, `setroute`, `assignpilot`, `unassignpilot`, `setschedule`,
  `clearschedule`, `showschedule`)

Ownership, crew, and shipyard:

- `src/vessels/vessels_edit.c` - `vedit` prototype editor, spawner, shipyard (`vedit`,
  `shipbrowse`, `shipbuy`, `shipchristen`, `shipcustomize`)
- `src/vessels/vessels_ownership.c` - Ownership and helm permits (`shippermit`, `shiprevoke`,
  `shipcrew`, `shipdeed`)
- `src/vessels/vessels_crew.c` - Hired crew positions, tiers, wages (`shiphire`, `shipdismiss`,
  `shipwages`)
- `src/vessels/vessels_upgrades.c` - Refits, hull wear, insurance (`shipupgrade`, `shipinsure`)

Combat, economy, and the living world:

- `src/vessels/vessels_combat.c` - Damage model, weapon fire, sinking, groundings, repair, capture
  (`shipfire`, `shiprepair`, `claimship`)
- `src/vessels/vessels_trade.c` - Commodities, port pricing, bulk cargo (`dockfees`, `market`,
  `vtradecheck`, `cargobuy`, `cargosell`, `cargomanifest`)
- `src/vessels/vessels_contracts.c` - Freight boards and contracts (`contracts`,
  `contractaccept`, `contractdeliver`, `contractabandon`)
- `src/vessels/vessels_piracy.c` - Plunder, bounty, letters of marque (`plunder`, `bounty`,
  `marque`)
- `src/vessels/vessels_hazards.c` - Weather hazards and encounters from wilderness data
  (`seastate`)
- `src/vessels/vessels_merchants.c` - Data-driven NPC merchant shipping (`vmerchant`)
- `src/vessels/vessels_hunters.c` - Bounty-hunter warship encounters
- `src/vessels/vessels_events.c` - Staff showcase events and leaderboards (`vevent`)

Operator tooling:

- `src/vessels/vessels_admin.c` - Fleet overview, teleport, forced maintenance, room pool monitor,
  MSDP ship variables (`shiplist`, `shipgoto`, `shipfix`, `vdebug`, `vesseldebug`, `shippurge`)
- `src/vessels/vessels_balance.c` - Read-only duel, economy, and cost diagnostics

Vehicles and unified transport:

- `src/vessels/vehicles.c` - Land vehicle lifecycle, state, capacity, persistence
- `src/vessels/vehicles_commands.c` - Vehicle commands (`vmount`, `vdismount`, `drive`, `vstatus`,
  `vehiclecreate`, `vehiclepurge`, `loadvehicle`, `unloadvehicle`)
- `src/vessels/vehicles_transport.c` - Vehicle-in-vessel loading and unloading
- `src/vessels/transport_unified.c`, `transport_unified.h` - Transport-agnostic commands (`tenter`,
  `texit`, `tgo`, `tstatus`)

Legacy and converted content:

- `src/vessels/vessels_legacy.c`, `vessels_legacy.h` - Legacy route, ferry, and Greyhawk ship
  special procedures (`shipdisembark`)
- `src/vessels/vessels_moving_rooms.c`, `vessels_moving_rooms.h` - Legacy world `M` moving-room
  loading, scheduling, relocation
- `src/vessels/moving_room_events.c`, `moving_room_events.h` - Scheduler events for moving rooms
- `src/vessels/vessels_rol.c`, `vessels_rol.h` - Converted Realms of Luminari fixed-interior ships
- `src/vessels/transport.c`, `transport.h`, `routing.c`, `routing.h`, `transport_jobs.c`,
  `transport_jobs.h` - Carriage, sailing, and overland-flight fast travel (`landmarks`, `lm`)

#### Database Schema and Content

- `src/database/db_init.c:1183` `init_vessel_system_tables()` and `:1534`
  `create_vessel_procedures()` - Startup table and stored-procedure creation
- `src/database/db_init_data.c:750` `populate_ship_room_templates_data()` and `:1536`
  `verify_vessel_system_tables()`
- `sql/components/vessels_phase{2,4,6,7,8,9,10,11,12,13,14,15,16,17}_schema.sql` with matching
  `_rollback.sql` and `verify_vessels_phase*.sql`; `verify_vessels_schema.sql`,
  `test_vessels_integrity.sql`, `vessels_harbor_sandbox.sql`
- `sql/components/vessels_{campaign,derelict,frontier,narrative}_content.sql` with matching
  `_rollback.sql` and `verify_vessels_*_content.sql`
- `lib/world/vessel_harbor/`, `lib/world/vessel_campaign/`, `lib/world/vessel_derelict/` -
  Development harbor, campaign, and Blackwake derelict world files
- `lib/rol-conversion/runs/phase6-special-20260812-ships/` - Realms of Luminari ship conversion
  ledgers

#### Tests and Tooling

- `unittests/CuTest/test_transport_production.c` - Main production-linked vessel, autopilot, and
  vehicle suite
- `unittests/CuTest/test_vessel_boarding.c`, `test_vessel_events.c`, `test_vessel_lookout.c`,
  `test_vessel_narrative.c`, `test_vessel_tactical.c`
- `scripts/vessels/` - Content provisioning (`provision_vessel_*.sh`), actual-character gates
  (`test_vessel_*_in_game.sh`), scale benchmark, ferry soak, and memory-sample analysis

## Study scope and baselines

- Studied 2026-09-28. DurisMUD `cca1d43dc71038600555a92bc47321be41b1c8ac`, LuminariMUD
  `fdc0eabe6b1f1f1e1baac7a7b2bf615cc6fa023e`. Line numbers refer to those revisions.
- DurisMUD read: `/home/aiwithapex/projects/duris/docs/reference/SHIPS.md`,
  `/home/aiwithapex/projects/duris/docs/reference/SHIP_GAMEPLAY.md`,
  `/home/aiwithapex/projects/duris/lib/information/helpships`, the hit, firing, volley and damage
  code in `/home/aiwithapex/projects/duris/src/ships/ship_combat.c`, `update_maxspeed()` in
  `/home/aiwithapex/projects/duris/src/ships/ship_utils.c`, the pirate spawn in
  `/home/aiwithapex/projects/duris/src/ships/ship_npc.c`, the AI overview in
  `/home/aiwithapex/projects/duris/src/ships/ship_npc_ai.c`, the tables in
  `/home/aiwithapex/projects/duris/src/ships/ship_variables.c`, and the `[ships]` properties in
  `/home/aiwithapex/projects/duris/lib/duris.properties:2271-2277`.
- LuminariMUD read: `docs/systems/VESSEL_SYSTEM.md`,
  `docs/product-requirements/VESSEL_SYSTEM_REQUIREMENTS.md`, all of
  `src/vessels/vessels_combat.c`, the movement paths in `src/vessels/vessels.c` and
  `src/vessels/vessels_autopilot.c`, prototypes, prices and the shipyard in
  `src/vessels/vessels_edit.c`, and the crew, upgrade, trade, balance, hazard, piracy, docking
  and scheduling code they call.
- Rules applied, from the earlier Duris conversion work: when an existing LuminariMUD mechanic
  already produces a Duris effect, the gap is closed in one line; when LuminariMUD lacks the
  mechanic, the recommendation is the real mechanic, never a cheaper stand-in. This is not an
  exact-parity port. Duris numbers are the player-tested prior for the Open "Player-data
  balance" gate in `VESSEL_SYSTEM_REQUIREMENTS.md`, translated into LuminariMUD's D20 rules, units
  and economy.
- Units. Both games measure range in map rooms (a Duris ocean map room and a LuminariMUD
  wilderness coordinate are both one room), so Duris ranges transfer unchanged. A Duris ship tick
  is 1 s; a LuminariMUD vessel tick is 0.5 s (`VESSEL_PERIODIC_FAST_CADENCE`, 5 pulses at 10
  pulses per second, `src/vessels/vessel_periodic.c:15`). Duris prices are platinum (pp). On the
  default hull prices, 1 pp is about 2 LuminariMUD gold for ship, transport and warship hulls
  (Part 2.2); that is a starting conversion, not an economy measurement.

## Part 0: Findings at a glance

- LuminariMUD is ahead on world integration and breadth: 3D wilderness navigation (sea, air,
  depth, bathymetry, weather), routes, schedules, fares and NPC pilots, regional piracy law,
  supply-driven markets and freight contracts, durable NPC merchant fleets, D20 player boarding,
  capture, plunder, showcase events, MSDP, operator tooling and persistence.
- DurisMUD is ahead on the naval combat game itself. Its combat is a real-time maneuvering duel:
  momentum sailing with per-class acceleration and turn rate, 80/100-degree arcs over asymmetric
  armor, a 12-weapon catalogue with range bands, fragments, sail hits, armor pierce and ammo, a
  hit model driven by range, relative motion, target size and crew, breach states (one breach
  immobilizes, two sink) with a sink timer, weapon damage, crew knockdowns, ramming, crew
  training, stamina and casualties, a repair stock, rewards (salvage, bounty, renown), and NPC
  pirates and hunters that maneuver, board and loot.
- LuminariMUD combat is a stand-in over the same frame: heading and speed change instantly, the
  rudder is never read, rigging damage is undone by the next `speed` order, the hit roll is d20
  plus half the firing character's level against 10 plus the target's speed setting divided by 5,
  armor is equal on all sides, one 90-degree arc (so one weapon on the default hulls) bears at a
  time, reload is 3 s, and a hull sinks the moment total structure reaches zero.
- The structural gap is pacing. Autopilot moves a hull `speed` rooms every 0.5 s, so a speed-12
  merchant sails the 359-room Vailand Iron Passage in about 15 s; a Duris galleon takes 3 s to
  cross one room. At the LuminariMUD scale maneuvering, ambushes, storms and encounters cannot
  matter, and combat only works between hulls that have stopped.
- Duris balance anchors (derived in 1.13): equal mid-tier warships need roughly 3-15 minutes
  (typically 5-8) to sink one another, against LuminariMUD's 45-120 s target; a hull at speed
  covers about one weapon range per reload; a combat fit costs 35-45% of the hull, a full
  dockside repair 2-3%; a sinking costs the owner a share of the investment, never all of it.
- Part 4 lists 14 verified LuminariMUD defects. Two violate requirement 4.4 ("a disabled
  subsystem changes behavior in an observable way"): rudder and rigging damage change nothing.
  Bounties can never be cleared, any passenger can fire a ship's weapons, and the shipyard sells
  every prototype to anyone without limit.
- Part 3.3 fixes every design value and Part 3.4 records the owner's decisions of 2026-09-28:
  Duris pacing, a 3-8 minute time to kill, the Duris loss model, one-time crew hire, a cap of
  3 hulls per owner, and capture of disabled prizes only.
- Part 5 orders the work: defect fixes, movement and pacing, damage model, weapons and gunnery,
  crew/repair/loss, NPC raiders and AI, rewards and economy, client data.

## Part 1: How DurisMUD does it

### 1.1 Time, pacing and movement

Source: `/home/aiwithapex/projects/duris/docs/reference/SHIPS.md` ("Movement model", "The
heartbeat"), `update_maxspeed()` at `/home/aiwithapex/projects/duris/src/ships/ship_utils.c:818`,
`get_next_heading_change()` at `:735`, `get_next_speed_change()` at `:777`, `ship_activity()` at
`/home/aiwithapex/projects/duris/src/ships/ship_base.c:2363`.

- Position is fractional inside the current map room. Each 1 s tick adds
  `speed * sin(heading) / 150` to x and `speed * cos(heading) / 150` to y: speed 100 crosses a
  room in 1.5 s, speed 40 in 3.75 s.

- Orders set targets (`order speed`, `order heading`); the heartbeat converges on them.
  Acceleration per tick is `class accel * (1 + sail_mod) * stamina_mod`. Turn per tick is
  `class turn * (0.75 + 0.25 * (speed - 9) / (class max - 9)) * (1 + sail_mod) * stamina_mod`,
  so a slow ship turns worse. Steering costs crew stamina.

- Maximum speed is 0 with a breached arc (surface) or no sail, otherwise:

  ```text
  maxspeed = class max (+0-2 crew) * (1 + sail_mod)
           * (1 - (fit-out and cargo weight above the free allowance) / max load)
           * mainsail / max sail
  ```

  Weight is the fitting trade-off: a frigate carrying 69 weight of guns (six large ballistae and
  a heavy beamcannon, 20 free) keeps 66% of its speed before crew bonuses; 105 weight keeps 40%.

- Only ocean map rooms are legal water. Leaving them stops the ship and, at battle stations, may
  crash it (`(speed + 50) / ((1 + 2 * sail_mod) * stamina_mod)` against 2d50) for
  `hull weight / 25 + 1` hits of 1-9, the first on the bow.

- In harbors `order maneuver <dir>` moves one room at a speed of at most 20, with a 5-tick
  cooldown, and docking requires the crew to stand down from battle stations first.

- Undocking takes 30 ticks, weighing anchor 13. Locking a target sets battle stations, which last
  180 ticks after the lock is cleared.

### 1.2 Hulls

From `/home/aiwithapex/projects/duris/docs/reference/SHIP_GAMEPLAY.md` ("Hulls", "Arcs, mounts
and armour"); `ship_type_data[]` and `ship_arc_properties[]` at
`/home/aiwithapex/projects/duris/src/ships/ship_variables.c:694` and `:719`. Turn and accel are
per 1 s tick; F/P/R/S is fore/port/rear/starboard.

| Hull | Kind | Price pp | Min lvl | Hull wt | Load (free) | Speed | Turn | Accel | Sail | Mounts | Armor | Internal |
| -- | -- | -: | -: | -: | -- | -: | -: | -: | -: | -- | -- | -- |
| Sloop | merchant | 100 | 0 | 10 | 5 (0) | 100 | 50 | 35 | 20 | 0/0/0/0 | 2/3/1/3 | 1/1/1/1 |
| Yacht | merchant | 300 | 0 | 25 | 12 (2) | 100 | 45 | 28 | 40 | 1/1/1/1 | 6/8/4/8 | 3/4/2/4 |
| Clipper | merchant | 1,500 | 20 | 110 | 55 (8) | 88 | 30 | 22 | 90 | 1/2/1/2 | 29/36/18/36 | 14/18/9/18 |
| Ketch | merchant | 2,500 | 25 | 150 | 75 (10) | 78 | 20 | 17 | 100 | 1/2/1/2 | 40/50/25/50 | 20/25/12/25 |
| Caravel | merchant | 4,000 | 30 | 200 | 100 (13) | 68 | 13 | 13 | 110 | 1/3/1/3 | 53/66/33/66 | 26/33/16/33 |
| Carrack | merchant | 8,000 | 35 | 260 | 130 (16) | 58 | 8 | 10 | 120 | 1/3/1/3 | 69/86/43/86 | 34/43/21/43 |
| Galleon | merchant | 12,000 | 40 | 330 | 165 (19) | 50 | 6 | 8 | 130 | 2/3/1/3 | 88/110/55/110 | 44/55/27/55 |
| Corvette | warship | 9,000 | 31 | 165 | 82 (13) | 74 | 20 | 20 | 120 | 1/3/1/3 | 50/63/37/63 | 22/27/13/27 |
| Destroyer | warship | 15,000 | 36 | 220 | 110 (16) | 64 | 13 | 14 | 130 | 2/3/1/3 | 67/84/50/84 | 29/36/18/36 |
| Frigate | warship | 22,000 | 41 | 285 | 142 (20) | 55 | 8 | 10 | 140 | 2/3/2/3 | 87/109/65/109 | 38/47/23/47 |
| Cruiser | warship | 36,000 | 46 | 400 | 200 (25) | 48 | 5 | 8 | 160 | 2/4/2/4 | 122/153/91/153 | 53/66/33/66 |
| Dreadnought | warship | none | 51 | 600 | 300 (32) | 40 | 4 | 6 | 200 | 3/5/2/5 | 183/229/138/229 | 79/99/49/99 |

Patterns worth keeping:

- Armor profile: fore about 0.8 and rear 0.6 (warships) or 0.5 (merchants) of the beam value;
  internal about 0.43 (warships) or 0.5 (merchants) of the armor on the same arc. The beams are
  the big targets and the stern is the weak spot, which is what makes positioning pay.
- Every armor and internal value fits an unsigned char (largest 229).
- Merchants carry cargo and contraband; warships carry none but get more armor and mounts per
  hull weight and cost 1.8-3.8x a merchant of similar weight.
- The minimum level is enforced on the captain at undock (`check_undocking_conditions()`,
  `/home/aiwithapex/projects/duris/src/ships/ship_base.c:2272`), not at purchase.
- One ship per owner (`ships.owner_name` is `UNIQUE`). A hull change credits 90% of the old list
  price; a rename costs 10%.

### 1.3 Weapons and fitting

`weapon_data[]` at `/home/aiwithapex/projects/duris/src/ships/ship_variables.c:492`; allowed arcs
and hulls in `ship_allowed_weapons[]` at `:744`. Damage is per fragment; reload in seconds.

| Weapon | Price pp | Wt | Ammo | Range | Damage | Spread | Sail hit | Hull/sail % | Pierce | Reload | Arcs |
| -- | -: | -: | -: | -- | -- | -: | -: | -- | -: | -: | -- |
| Small Ballista | 50 | 3 | 60 | 0-8 | 2-4 | 10 | 12% | 100/50 | 10% | 30 | all |
| Medium Ballista | 100 | 6 | 50 | 0-10 | 4-6 | 10 | 14% | 100/50 | 10% | 30 | all |
| Large Ballista | 500 | 10 | 30 | 0-12 | 6-9 | 10 | 16% | 100/50 | 10% | 30 | all |
| Small Catapult | 500 | 10 | 30 | 4-15 | 4x 2-3 | 160 | 20% | 100/100 | 2% | 30 | fore, rear |
| Medium Catapult | 800 | 13 | 20 | 5-20 | 5x 2-4 | 260 | 20% | 100/100 | 2% | 30 | fore, rear |
| Large Catapult | 1,200 | 17 | 12 | 6-25 | 6x 2-5 | 360 | 20% | 100/100 | 2% | 30 | fore, rear |
| Heavy Ballista | 1,000 | 15 | 6 | 0-4 | 15-22 | 10 | 0% | 100/0 | 15% | 30 | beams |
| Light Beamcannon | 4,000 | 7 | 40 | 0-20 | 16 to 4 by range | 10 | 10% | 100/30 | 15% | 45 | all |
| Heavy Beamcannon | 5,000 | 9 | 40 | 0-23 | 22 to 5 by range | 10 | 10% | 100/30 | 15% | 45 | all |
| Mind Blast Cannon | 4,000 | 5 | 50 | 0-20 | crew stun | 360 | - | - | - | 45 | all |
| Fragmentation Cannon | 5,000 | 7 | 20 | 0-16 | 5x 4-6 | 90 | 50% | 50/100 | 0% | 45 | fore, rear |
| Long Tom Catapult | 5,000 | 9 | 6 | 12-32 | 8x 3-6 | 360 | 20% | 100/100 | 3% | 45 | fore, rear |

- Each hull has a mount count and a weapon-weight cap per arc (1.2), and each weapon a list of
  allowed arcs and hulls. Catapults fire from the ends, heavy ballistae from the beams.
- The five beam, blast, fragmentation and Long Tom weapons and the levistone are "capital": one
  per ship, and each needs renown (ship frags) or gunnery skill of 1600-2000.
- A weapon installs in `weight * 75` ticks; ammo is bought per round.
- Design rule visible in the table: standard weapons deal about 1.5-2.5 hull damage per minute
  per weight point at 100% hits (large ballista 1.5, large catapult 2.5). The exotic ones buy
  more with tight range bands, 6-round magazines, the capital limit and renown gates.

### 1.4 Targeting and the hit model

`weaponsight()` at `/home/aiwithapex/projects/duris/src/ships/ship_combat.c:1661`,
`fire_weapon()` at `:1820`, `volley_hit_event()` at `:764`, `volley_hit_percent()` at `:733`,
`get_arc()` at `/home/aiwithapex/projects/duris/src/ships/ship_utils.c:1050`.

- `lock <id>` picks one contact (two-letter designation from `look contacts`) and sets battle
  stations; `fire <slot>` fires one weapon, `fire fore|port|starboard|rear` fires an arc. A weapon
  fires only when the target is inside its range band and its arc, and it is undamaged, reloaded
  and loaded. The firing ship must be undocked, not anchored and not mind-blasted.
- Arcs are relative to heading: fore +-40 degrees, starboard 40-140, rear 140-220, port 220-320.
- `weaponsight()` projects both ships one tick ahead and builds a motion term: direct fire uses
  `target crossing speed + 4 * bearing rate + closing speed / 4`; ballistic (catapults) uses
  `crossing / 2 + 3 * bearing rate + closing`. It starts at 0.5, divides by `1 + motion / 50`,
  adds `sqrt(hull) - 3` points for target size, shrinks the miss chance from maximum range down
  to a quarter of it, divides the miss chance by `1 + crew guns_mod` (x1.5 against a flyer) and
  multiplies by crew stamina.
- The result N resolves as `2d50 >= 100 - N`, and players are shown the true chance
  (`look sight`, and on every shot).
- The volley lands after `range / max_range * flight time` pulses with the chance frozen at
  firing; a volley at a ship that has docked or entered a zone room is dropped.
- Reload is `weapon reload * (1 - 0.15 * guns_mod) / stamina_mod`. Every shot costs stamina and
  trains gunnery.

### 1.5 Damage model

`damage_hull()` at `/home/aiwithapex/projects/duris/src/ships/ship_combat.c:969`,
`damage_sail()` at `:915`, `update_ship_status()` at `:1228`.

- Each fragment hits the sails with the weapon's sail-hit chance (damage times sail %),
  otherwise the target's arc facing the shooter, scattered by half the spread (damage times hull
  %).
- Armor absorbs first. If it holds, the armor-pierce chance makes a critical that carries half
  the damage into the internals (50% chance to damage a weapon); otherwise the hit stops. Overkill
  spills into the internals (15% weapon-damage chance). On a gutted arc one hit in three deflects
  into another arc with structure, and every hit damages a weapon. Weapon damage accumulates: 1 or
  more stops it firing, 100 destroys it.
- One internal hit in nine knocks everyone aboard down for two combat rounds.
- Warship sails take 85% of sail damage (`warship.sails.damage.reduction`); the Ship Damage
  Control epic skill cuts every hit by 4% plus a fifth of the skill while the owner is aboard.
- An arc is breached when its armor and internal are both zero. One breach immobilizes a surface
  ship (a flyer keeps half speed); two start sinking. No sail also immobilizes.

### 1.6 Ramming, crew-stun weapons and flight

- `order ram` needs a lock and speed >= 20. It connects inside a 120-degree bow cone at the same
  altitude; closing speed decides, heavier rammers and higher speeds lower the odds and deck skill
  raises them. Both ships take `(hull + 100) / 10` base damage scaled by speed (x1.2 on the bow)
  in 2-6 point hits; a fitted ram adds a bow hit of `(hull + 10) / 24` and halves bow crash damage;
  the lighter ship is spun to a random heading and both crews are knocked down. Cooldowns: 50
  ticks after a hit, 25 after a miss, guns locked 25 ticks after a hit.
- The Mind Blast Cannon does no damage: the target crew is stunned 5-20 ticks (no steering,
  firing, reloading or repair, certain crash at a coast) and passengers are knocked down inside
  mid-range.
- The levistone flies any hull above a yacht for 60 ticks (600 to recharge) at 4 squares of
  altitude. Ranges are 3D, so short-reach weapons cannot hit it; shots at flyers miss 50% more;
  flyers cannot be boarded and rams pass under them.

### 1.7 Crew

`ShipCrew::update()` at `/home/aiwithapex/projects/duris/src/ships/ship_utils.c:1766`,
`skill_raise()` at `:1697`, `replace_members()` at `:1610`; tables `ship_crew_data[]` and
`ship_chief_data[]` at `/home/aiwithapex/projects/duris/src/ships/ship_variables.c:49` and `:358`.

- One crew per ship with three skills (deck, guns, repair), a stamina pool and up to three
  chiefs. Each modifier is `sqrt(skill) / 150 + 0.03 * (crew level + crew mod + chief mod)`;
  deck and guns blend in 20% of repair.
- Skills grow with use: sailing, reloading and firing at a valid target, repairing, ramming,
  selling cargo, and kills. A chief adds 10-70% to training in its department and +1 to +3 to its
  modifier; a crew below the chief's minimum skill learns four times as fast.
- Steering, firing, reloading and repairing spend stamina. Below zero it scales acceleration,
  turning, hit chance, reload and repair by `1 / (1 + deficit / max / 3)`; docking or anchoring
  regenerates it four times as fast.
- A sinking pulls every skill back toward the crew's base by `10 + frags lost / 30` percent in
  PvP, `5 + hull weight / 100` percent when an NPC made the kill.
- 18 hireable crews beyond the starting amateurs (levels 1-4, base skills 0-3000, stamina 500-1500,
  1,000-40,000 pp) and 12 chiefs (800-9,000 pp). Each is gated by renown or by current crew skill,
  costs a one-time price and has no wages; a new crew keeps its training less 1-5%.

### 1.8 Repair and maintenance

- At sea the crew repairs from a repair stock equal to the hull weight, refilled on docking. Each
  tick it may repair with per-mille chances scaled by `(1 + repair mod) * stamina`; battle
  stations and movement cut the odds, anchoring raises them, and thresholds (internal only below
  `max * (repair mod + 0.1)`, armor only with repair mod above 0.5) mean a ship never fully
  repairs at sea.
- At a shipwright (`repair_*()` and `reload_ammo()` at
  `/home/aiwithapex/projects/duris/src/ships/ship_shop.c:1501-1895`): armor and internal 1 pp per
  point and sail 2 pp per point, each command `75 + points` ticks; a damaged weapon 1 pp per damage
  point (75 ticks), a destroyed one half its list price (150 ticks); ammo 1 pp per round (75 ticks
  per weapon). The maintenance timer keeps the ship in port.

### 1.9 Sinking, insurance and rewards

`sink_ship()` at `/home/aiwithapex/projects/duris/src/ships/ship_combat.c:536`, `calc_salvage()`
at `:323`, `calc_bounty()` at `:374`, `finish_sinking()` at
`/home/aiwithapex/projects/duris/src/ships/ship_base.c:3146`.

- A sink timer runs before the wreck goes down: 75-150 ticks for player ships, 1000-1500 for NPC
  ships so they can be boarded and looted.
- Rewards settle at the moment of sinking and go into the victor's ship coffers. Frags equal the
  target's hull weight (across a racewar boundary; NPC victims only train the crew). Salvage is
  the hull list price times the fraction of armor and internals left, plus surviving weapons at
  half list, divided by `ship.sinking.rewardDivider` (8 shipped). A target of the other side with
  more than 100 frags pays a bounty of `frags * 10000 / 8` copper. Shares are split among the
  ships in contact.
- The loser gives up a share of frags and some crew training, but the ship is never deleted: it
  becomes a sloop with every slot emptied at Davy Jones' Locker, keeping its name, frags and crew.
  Insurance pays 50% of the hull price for a warship, 75% for a merchant, 90% if an NPC made the
  kill, nothing for a sloop.
- A top-20 frag leaderboard; 20-frag PvP kills advance an epic skill.

### 1.10 NPC ships

`try_load_pirate_ship()` at `/home/aiwithapex/projects/duris/src/ships/ship_npc.c:1221`,
`NPCShipAI::activity()` at `/home/aiwithapex/projects/duris/src/ships/ship_npc_ai.c:261`,
`basic_combat_maneuver()` at `:1210`, `advanced_combat_maneuver()` at `:1777`.

- Every tick a moving, untargeted player ship rolls 1 in 1001
  (`ships.pirate.load.chance=1000`, `/home/aiwithapex/projects/duris/lib/duris.properties:2271`):
  one ambush roll succeeds per ~17 minutes of sailing. Merchant hulls are ambushed at most once
  per trip; sloops and yachts never.
- The threat scales with the victim. `n = random(0, hull weight) + frags` picks tier 0-3 at
  250/1200/2200 (the 1200 was tuned up from 800 in play); warships must first pass a 1-1000
  notice roll and always draw hunters.
- Five tiers with 45 fit-outs and crews of 8-25 mobs plus a treasure chest; the NPC appears 45
  rooms off the victim's bow and closes at full speed.
- The brain has modes: engaging (basic: rank the arcs, turn the best one onto the target, open or
  close range; advanced: predict both ships and choose a broadside), running (out of ammo),
  cruising, leaving (despawn 300 ticks after losing the target, delayed while players watch) and
  looting. It boards when the target is slow (a merchant at speed 9 or less, a warship stopped):
  grunts take the bridge and then half to three quarters of the rooms; pirates steal part of the
  hold (40-60% is lost in transfer) and leave, hunters fight on. Killing the captain disables the
  NPC ship.

### 1.11 Economy

- Ten ports, each producing one cargo and one contraband good. A buyer appetite matrix of
  178-406%, tuned to sailing distance, sets sale prices: buy at `base * mod`, sell at
  `1.5 * base * appetite * mod`. Market modifiers drift back to 0.6 and are held in bands (cargo
  0.54-0.69, contraband 0.85-1.3); players see prices an hour late.
- Warship hulls sell cargo at -40%, a diplomat's flag at -10%, the SEADOG innate at +10%.
- Contraband needs renown or crew skill. Customs confiscates each crate with chance
  `35 + crates / 2 - sqrt(frags) / 5`, rising as the hold empties, so contraband hides inside a full
  hold of ordinary cargo.
- Crates weigh 2 against the weight budget, so a full hold slows the ship. Jettisoned crates float
  half the time and can be salvaged by a stopped ship; a sinking ship jettisons its hold.

### 1.12 Services and information

- `summon ship` at any shipwright fetches the ship for its hull weight times 50 copper (14 pp for a
  frigate) and `50 / speed` (small hulls) or `70 / (speed - 20)` mud hours, capped at 75 minutes; it
  clears the cargo.
- `scan <id>` within 20 rooms reports per-arc armor and internals, weapons (and which are
  destroyed), status and flag. `look contacts` lists every ship within 35 rooms with ID, range,
  bearing, heading, speed and the arc it lies in. `look sight <slot>` shows the true hit chance.
- GMCP `Ship.Contacts` and `Ship.Info` carry contacts with arcs and targeting flags, per-arc
  condition, weapons with ammo, damage and readiness, and the crew sheet.

### 1.13 Derived balance anchors

(a) Hit chance against a frigate with a large ballista (range 0-12), from `weaponsight()` and the
2d50 roll. Crew modifiers: amateur 0.00, Winterhaven Seamans 0.30, Magical Automatons 0.53.

| Situation | Amateur | Trained | Elite |
| -- | -: | -: | -: |
| Target stopped, range 4 | 99.8% | 99.9% | 99.9% |
| Target stopped, range 8 | 92.4% | 95.8% | 96.9% |
| Parallel courses at speed 40, range 8 | 74.8% | 86.0% | 89.9% |
| Crossing at speed 55, range 10 | 47.0% | 68.8% | 77.6% |
| Maximum range 12, target at speed 40 | 36.1% | 62.2% | 71.9% |

Position and motion matter more than the crew: close, slow targets are near-certain hits and a
long shot at a mover is a coin flip even for an elite crew.

(b) Time to sink. A frigate broadside of three large ballistae delivers 14-41 hull damage per
minute over those situations. Sinking a frigate takes two breached arcs, at least 213 points
(fore 125 plus rear 88), 244 with a beam. With one broadside bearing throughout that is 5-15
minutes; a skilled captain alternating both broadsides at close range with a trained crew needs
about 3. LuminariMUD's current equal-warship duel resolves in 67 s (Part 2.6).

(c) Pacing. At speed a Duris hull covers 12-20 rooms per 30 s reload, about one weapon range, so
each volley follows a maneuver decision. A LuminariMUD speed-20 hull covers 120 rooms per 3 s
reload.

(d) Costs relative to the hull. A frigate combat fit of six large ballistae and a heavy
beamcannon costs 8,000 pp (36% of the hull); a heavier 10-weapon fit about 10,100 pp (46%). A
full armor and internal refit is 525 pp (2.4%), sails 280 pp, 30 rounds of ammo 30 pp. A hull
change credits 90%; insurance returns 50-90% of the hull.

(e) Threat cadence. One ambush roll success per ~17 minutes of sailing: a 22-minute voyage meets
one with about 73% probability, and a merchant hull at most once per trip.

## Part 2: What LuminariMUD has today

### 2.1 Pacing and movement

- One vessel tick is 0.5 s. Autopilot moves a hull `speed` rooms toward its waypoint every tick
  (`travel = speed`, `src/vessels/vessels_autopilot.c:2185`), validating only the destination
  cell. Hunters pursue through the same resolver (`src/vessels/vessels_hunters.c:987-1003`).
- Manual sailing is `setsail <direction>`: one jump of `max(1, speed / 10)` rooms, plus 1 with
  the Kuo-toa SEADOG feat, per command, with no command lag (`src/vessels/vessels.c:1927-1928`,
  `:2730-2768`). Between commands the hull does not move, whatever its speed setting.
- `speed` and `heading` apply instantly (`src/vessels/vessels.c:2316-2317`, `:2451-2452`); the
  helm needs `is_pilot()`. Terrain and weather modify speed; there is no acceleration, turn rate,
  sail or load effect.
- Airships use Z up to 500 (magical hulls 1000) and submarines dive below 0 in water; bathymetry
  grounds deep-draft hulls (`vessel_check_grounding()`,
  `src/vessels/vessels_combat.c:611`, 2d4 bow damage at any speed) and crushes deep submarines.

### 2.2 Hull classes, prototypes and prices

A prototype (`ship_prototypes`, `vedit`) has only a name, class, max speed (at most 30) and one
armor value (at most 100). `vessel_initialize_condition()` (`src/vessels/vessels.c:731`) gives all
four sides that armor, internals of `armor / 2 + 10` per side, sail 20 and rudder 20. Default
weapons come from the class (`src/vessels/vessels_edit.c:541-585`); the price is
`base * (1 + armor / 50 + speed / 60)` (`:395`).

| Class | Default speed / armor | Default weapons | Cargo lbs | Default price (gold) | Dock fee |
| -- | -- | -- | -: | -: | -: |
| Raft | 5 / 2 | none | 300 | 56 | 5 |
| Boat | 10 / 5 | none | 2,000 | 633 | 10 |
| Ship | 15 / 20 | fore ballista 1d8, range 40 | 12,000 | 8,250 | 25 |
| Warship | 20 / 40 | fore, port, starboard ballistae 2d8, range 50 | 6,000 | 42,666 | 40 |
| Airship | 25 / 15 | fore ballista 1d8, range 40 | 4,000 | 85,833 | 50 |
| Submarine | 8 / 25 | fore ballista 1d8, range 40 | 3,000 | 65,333 | 50 |
| Transport | 8 / 20 | fore ballista 1d8, range 40 | 40,000 | 23,000 | 35 |
| Magical | 15 / 20 | fore ballista 1d8, range 40 | 12,000 | 165,000 | 75 |

Compared with Duris: ship 8,250 against caravel 4,000 or carrack 8,000, transport 23,000 against
galleon 12,000, warship 42,666 against frigate 22,000, hence about 2 gold per pp; raft and boat
are relatively cheaper. The struct already carries unused Duris-shaped fields: `hullweight`,
`maxslots`, per-slot `weight`, and slot type 3 "ammo" (`src/vessels/vessels.h:1130-1139`).

### 2.3 Naval combat

- `shipfire <slot> <target>` (`src/vessels/vessels_combat.c:812`): the target must be within the
  weapon's range (3D) and the target must lie in the arc the weapon is mounted on. Arcs are 90
  degrees each (`greyhawk_getarc()`, `:305`), so on the default hulls one weapon bears at a time.
- Hit: `d20 + level / 2 + crew gunnery` against `10 + target speed / 5` (`:895-896`). NPC return
  fire uses `d20 + crew gunnery + 5` (`:711`) and needs an NPC pilot; hunters reuse it by marking
  their target as the last attacker.
- Damage `dice(val2, val3)` goes to the struck side: armor absorbs it entirely until empty, the
  rest hits that side's internal, and a destroyed side bleeds into the others
  (`vessel_apply_damage()`, `:474`). Bow spill reduces the sail and stern spill the rudder
  (`:570-587`). Reload is 6 ticks, 3 s (`VESSEL_WEAPON_RELOAD_TICKS`, `src/vessels/vessels.h:500`).
- The hull sinks at once when total internal reaches zero (`vessel_sink()`, `:338`): everyone
  aboard goes into the water room, loose objects float there, bulk cargo is lost, the exterior
  becomes inert wreckage, insurance settles and the slot and all persistence are deleted.
- Every player-driven hostile act passes `vessel_pvp_permitted()` (`:146`), with a five-minute
  opponent-specific logout grace. Unowned hulls are always fair game.

### 2.4 Crew, repair, wear, upgrades and insurance

- Crew: four positions (sailmaster, gunner, bosun, quartermaster) at three tiers. Hire costs
  400/600/350/300 gold times the tier; wages are a tenth of that every 300 s
  (`src/vessels/vessels_crew.c:123-148`, `:387`). Tiers give +2/+4/+6 to speed, gunnery and repair
  and +10/20/30% cargo (`:156`). Crew never trains.
- `shiprepair` (`src/vessels/vessels_combat.c:924`): any character aboard a stopped hull restores
  5 armor and 2 internal per side and 5 sail and 5 rudder per use, plus the bosun bonus, with a
  12 s lag and no materials or cost.
- Wear removes 1 armor per side, 1 sail and 1 rudder every 450 s under way
  (`src/vessels/vessels_upgrades.c:461`). Four one-time refits (plating +50% armor, rigging +5
  speed, hold +25% cargo, reinforcement +50% internal) cost `class price(10, 10) / 4`, 6,833 gold
  on a warship (`:229`).
- Insurance: a premium of a fifth of the chosen payout, up to the hull price, consumed by one
  sinking (`:24`, `:634`).

### 2.5 Where LuminariMUD is ahead (keep)

| Area | LuminariMUD | DurisMUD |
| -- | -- | -- |
| World | Shared wilderness coordinates, terrain, bathymetry, weather, regions; airships and submarines in 3D | One ocean map layer; levistone flight for 60 s |
| Automation | Waypoints, routes, schedules, public fares, NPC pilots, durable autopilot | `order sail` up to 35 rooms on one heading |
| Hazards | Weather bands, storms, gales, crush depth, grounding on real depth | Coastline crashes only |
| Law and piracy | Regional bounty rates, WANTED/HUNTED, port refusal, letters of marque, merchant faction consequences | None beyond frags and racewar |
| Economy | Supply-driven prices with a proven anti-arbitrage bound, freight contracts, dock fees to clan ports | Fixed appetite matrix with drift bands |
| Boarding | D20 Boarding ability contest for players, capture, plunder | Only NPCs board |
| Events | Regattas, skirmishes, ghost fleets, leaderboards | Frag leaderboard |
| Operations | Fleet tools, room-pool monitor, balance diagnostics, rollback mode | Staff objects and set commands |

### 2.6 Current balance numbers

- Equal default warships: 40 armor and 30 internal per side, one 2d8 ballista bearing, 80% hits
  (`d20 + 9 >= 14`), 3 s reload: 144 damage per minute against 160 needed on one side. The
  `vesseldebug balance` duel (`src/vessels/vessels_balance.c:15-25`) reports a 67 s median against
  the provisional 45-120 s target and 180 s p95.
- A level-30 gunner never misses (`d20 + 15 >= 16` at worst); a level-1 gunner without crew hits
  a speed-30 target 25% of the time. The character's level decides more than anything else.
- Voyage times at the current scale: the 359-room Vailand Iron Passage takes 15 s at the cog's
  speed 12; hazard checks run every 30 s and encounter rolls every 90 s
  (`src/vessels/vessels.h:670-671`), and both skip a stopped hull.
- A veteran crew costs 4,950 gold to hire and 5,940 gold per hour in wages.
- Trade: the widest LuminariMUD spread is about 3.3x (buy at 0.4 of base in a glutted port, sell
  at 85% of 1.54 of base in a starved one, `src/vessels/vessels_trade.c:544`), before per-unit
  batch pricing narrows it; Duris pays 2.7-6.1x at neutral markets (1.5 times the 178-406%
  appetite), with customs, capacity and ambushes as the brakes.

## Part 3: Mapping and decisions

### 3.1 Feature matrix

Status: Ahead (LuminariMUD better), Covered (an existing mechanic already delivers the effect),
Partial, Missing, Not needed (Duris-specific). Every Partial and Missing row has a decided
disposition; the values are in 3.3.

| Area | DurisMUD | LuminariMUD | Status | Disposition |
| -- | -- | -- | -- | -- |
| Momentum sailing | Fractional position, accel, turn rate | Instant speed/heading, step jumps | Missing | Build (S2) |
| Pacing | Room per 1.5-4 s | Room per 0.02-0.5 s | Missing | Duris pacing, D1 (S2) |
| Sail and load affect speed | Yes | No | Missing | Duris formula (S2) |
| Rudder | None | Field exists, never read | Partial | Scales turn rate (S2) |
| Harbor maneuvering | `order maneuver`, speed at most 20, 5-tick cooldown | `setsail` steps, no cooldown | Partial | `setsail` becomes the maneuver command (S2) |
| Departure and anchor | 30-tick undock, 13-tick weigh anchor, anchored repairs | Leave port at once, no anchor | Missing | `undock` departure and `anchor` (S2) |
| Asymmetric arcs and armor | 80/100 degrees, profiled armor | 90 degrees, equal armor | Partial | Duris arcs and profiles (S3) |
| Breach states and sink timer | 1 breach immobile, 2 sink, 75-1500 s | Sinks at 0 total, instantly | Missing | Build (S3) |
| Sails as a hit location | Sail HP 20-200, sail-hit per weapon | Bow spill only, sail 20 | Partial | Build (S3, S4) |
| Criticals, weapon damage, knockdown | Yes | No | Missing | Build with D20 rolls (S3) |
| Weapon catalogue and fitting | 12 weapons, mounts, weight caps, capital limit | Fixed class armament | Missing | Build as data (S4) |
| Equipment | Ram, levistone, diplomat's flag | None | Missing | Ram and neutral colors (S4); airships replace the levistone |
| Ammo and resupply | Per weapon, 1 pp a round | None | Missing | Build (S4); slot type 3 exists |
| Lock, battle stations, fire by arc | Yes | Name/slot per shot | Missing | Build (S4) |
| Hit model | Range, motion, size, crew, stamina | d20 + level/2 vs speed | Partial | Duris geometry sets a D20 DC (S4) |
| True hit chance shown | `look sight`, on firing | No | Missing | `sight` and fire message (S4) |
| Scan report | Arcs, weapons, status, flag | Status band only | Partial | `scan` (S4) |
| Ramming | Yes | No | Missing | Build (S6) |
| Crew-stun weapon | Mind Blast | No | Missing | Weapon flag (S4) |
| Flight rules | Altitude 4, reach limits, +50% miss | 3D range exists, no rules; flyers above 50 are out of every range | Partial | Altitude rules (S4) |
| Crew skills, training, casualties | Continuous skills, chiefs | Four positions, three tiers, no training | Partial | Tier experience, promotion, casualties (S5) |
| Crew stamina | Yes | No | Missing | Build (S5) |
| Crew economics | One-time hire, no wages | Wages every 300 s, even offline | Partial | One-time hire, D4 (S1) |
| Repair economy | Repair stock, thresholds, priced dock repairs | Free unlimited `shiprepair` | Partial | Build (S5) |
| Loss model | Hull kept as sloop, identity kept, 50-90% insurance | Hull and identity deleted, bought insurance | Missing | Duris model, D3 (S5) |
| Summon ship | Fee and travel time, clears cargo | None | Missing | `shipsummon` (S5) |
| Rewards and renown | Frags, salvage, bounty, leaderboard | None for a sinking | Missing | Build (S7) |
| Jettison and salvage | Crates float, salvage when stopped | Loose objects float, bulk cargo lost | Partial | Cargo spills as crates, `salvage` (S3) |
| NPC pirates and hunters | Ambush cadence, tiers, fit-outs, AI | Hunters for HUNTED only, return fire | Partial | Raider tiers on the hunter lifecycle (S6) |
| NPC combat AI | Arc ranking, range keeping, running, boarding | Pursue and return fire | Missing | Basic and advanced AI (S6) |
| NPC boarding and looting | Yes | No | Missing | Through the boarding contest (S6) |
| Level gates, ownership limit | Captain level at undock, one ship per owner | None | Missing | Level at departure, cap of 3, D5 (S1) |
| Hull trade-in, rename fee | 90% credit, 10% rename | None, free rename | Missing | Build (S5) |
| Contraband and customs | Yes | None | Missing | Build on the law regions (S7) |
| Ship Damage Control | Epic skill, up to 24% | None | Missing | Epic feat (S7) |
| SEADOG innate | +2 maximum speed at the helm, +10% cargo sales | Kuo-toa feat: +1 room per manual `setsail` only | Partial | +1 maximum speed and +10% sales (S2, S7) |
| Client data | GMCP contacts, weapons, crew | MSDP name, position, hull, status | Partial | MSDP tables (S8) |
| Racewar ocean PvP state, signal, ferries, autopilot, identity refs, save queue | Duris-specific or covered | PvP consent, `shiptalk`, schedules, autopilot, generation events, persistence | Covered or Not needed | None |

### 3.2 Combat parity checklist

Every Duris combat feature with the LuminariMUD form it takes. The requirement column is
`VESSEL_SYSTEM_REQUIREMENTS.md` section 4.4 ("range, bearing, firing arcs, reloads, armor
sections, subsystem damage, repair, sinking, wrecks, boarding, capture, and NPC combat doctrine";
"existing D20 and PvP-consent systems"; "a disabled subsystem changes behavior").

| Duris feature | LuminariMUD form | Requirement |
| -- | -- | -- |
| Lock and battle stations | `lock <id>`; battle stations until 180 s after the lock clears; blocks entering a port and summoning | NPC doctrine, safety |
| Contacts with IDs and arcs | One contact list within `vessel_sight_range()`, nearest first, two-letter IDs everywhere, showing the arc each contact lies in | Range, bearing, arcs |
| Fire slot or arc | `shipfire <slot or arc> [id]` against the locked target | Arcs, reloads |
| Range bands | Per-weapon minimum and maximum range in rooms (Duris values) | Range |
| Hit model | `d20 + gunnery bonus >= DC`, with the DC computed from Duris's `weaponsight()` geometry (3.3.4) | Existing D20 |
| True chance shown | `sight <slot>` and every fire message print the chance | Observable |
| Fragments, spread, sail hits | Weapon rows carry fragments, spread, sail-hit and hull/sail percentages | Armor sections |
| Armor pierce criticals | Threat range from the weapon's pierce value; a confirmed critical carries half damage past armor | Existing D20 |
| Weapon damage | Weapons disabled at 1 damage and destroyed at 100; repaired in port | Subsystem damage |
| Knockdown | One internal hit in nine: Reflex DC 15 or prone for two combat rounds, everyone aboard | Existing D20 |
| Sails | Per-class sail HP; maximum speed times sail fraction; zero is immobile | Subsystem damage |
| Rudder (LuminariMUD-only) | Turn rate times rudder fraction; zero cannot turn | Subsystem damage |
| Breach states | One breached arc immobile (airships half speed), two sinking on a timer | Sinking, wrecks |
| Ramming | Bow cone, closing speed, hull weights, ram equipment, cooldowns | Doctrine |
| Crew stun | Weapon flag: crew stunned 5-20 s; characters within mid-range Will DC 15 or prone two rounds | Existing D20 |
| At-sea repair limits | Repair stock, thresholds, battle-station and movement penalties; Craft (woodworking) DC 15 for character repairs | Repair |
| NPC doctrine | Arc ranking, range keeping, running, boarding slow targets | NPC doctrine |

### 3.3 Decided design values

Duris-derived unless marked LuminariMUD-only. They are the implementation targets;
`vesseldebug balance` and the human beta tune numbers later without reopening the design. Prices
use 2 gold per Duris platinum. Durations in seconds convert to 0.5 s ticks by doubling.

#### 3.3.1 Hull classes

Each class takes its Duris analog's per-arc armor and internal values as the defaults for its
prototype armor value (the beam armor); a prototype's armor scales all eight numbers in
proportion.

| Class | Duris analog | Hull wt | Max load (free) | Full hold wt (free) | Speed | Accel / turn per tick | Sail | Mounts F/P/R/S | Arc wt caps F/P/R/S | Beam armor | Min level | Price | Insurance |
| -- | -- | -: | -- | -- | -: | -- | -: | -- | -- | -: | -: | -: | -: |
| Raft | Sloop | 10 | 5 (0) | 2 (0) | 5 | 5 / 25 | 20 | 0/0/0/0 | 0/0/0/0 | 3 | 1 | 200 | 0% |
| Boat | Yacht | 25 | 12 (2) | 6 (0) | 30 | 4 / 22 | 40 | 1/1/1/1 | 3/5/3/5 | 8 | 1 | 600 | 75% |
| Ship | Caravel | 200 | 100 (13) | 70 (12) | 20 | 2 / 6.5 | 110 | 1/3/1/3 | 17/26/17/26 | 66 | 16 | 8,000 | 75% |
| Transport | Galleon | 330 | 165 (19) | 140 (40) | 15 | 1.2 / 3 | 130 | 2/3/1/3 | 26/35/26/35 | 110 | 21 | 24,000 | 75% |
| Warship | Frigate | 285 | 142 (20) | 56 (0) | 17 | 1.5 / 4 | 140 | 2/3/2/3 | 31/44/31/44 | 109 | 22 | 44,000 | 50% |
| Airship | Corvette | 165 | 82 (13) | 32 (0) | 22 | 3 / 10 | 120 | 1/3/1/3 | 13/32/13/32 | 63 | 24 | 72,000 | 50% |
| Submarine | Destroyer, ends only | 220 | 110 (16) | 44 (0) | 12 | 2 / 6.5 | 130 | 2/0/1/0 | 27/0/27/0 | 84 | 23 | 60,000 | 50% |
| Magical | Cruiser | 400 | 200 (25) | 80 (0) | 14 | 1.2 / 2.5 | 160 | 2/4/2/4 | 35/50/35/50 | 153 | 25 | 144,000 | 50% |

- Speeds are Duris speed times 0.3, except the raft (LuminariMUD river craft, 5) and the
  submarine (LuminariMUD-only, 12); all stay at or below `SHIP_MAX_SPEED` 30. Accel and turn are
  Duris values times 0.3 and halved per 0.5 s tick.
- "Full hold wt" is the Duris weight of a full hold: the analog's cargo rating times 2 for
  merchants, its salvage allowance (max load / 5 crates) for the others. Cargo counts toward the
  weight budget as `cargo lbs / capacity lbs * full hold wt`.
- A prototype's price is the class price times
  `0.5 + 0.25 * armor / class armor + 0.25 * speed / class speed`, so a default hull costs the
  class price.
- Minimum level is Duris's hull level times 30/56 (the mortal caps), checked for the character
  who gives the departure order. Airship 24 and submarine 23 are LuminariMUD-only placements.
- Insurance is a share of the hull price, 90% when an NPC made the kill, 0% for rafts and wreck
  replacement hulls (3.3.7).
- `VEDIT_MAX_ARMOR_LIMIT` rises from 100 to 229; any refit clamps armor and internal at 255.
- Refits are rescaled to Duris magnitudes: plating +20% armor, reinforcement +20% internal,
  rigging +10% maximum speed (at least +1, capped at 30), hold +25% cargo; each costs 20% of the
  class price.
- Only warships take Duris's warship rules for trade: cargo sells at -40% and contraband cannot
  be bought.

#### 3.3.2 Movement (D1: Duris pacing)

- A hull moves `speed / 90` rooms per 0.5 s tick along its heading: speed 30 crosses a room in
  1.5 s, 12 in 3.75 s, 5 in 9 s. Every cell entered is validated; entering illegal water or
  land stops the hull at the edge, and at battle stations rolls Duris's crash check
  (`(speed + 50) / ((1 + 2 * sail mod) * stamina mod)` against 2d50, with speed in Duris units),
  dealing `hull weight / 25 + 1` hits of 1-9, the first on the bow. Bathymetry grounding uses the
  same rule.
- `speed` and `heading` set targets. Each tick speed moves toward its target by the class accel
  times the sailmaster multiplier times the stamina modifier, and heading by the class turn times
  the same factors, times `(0.75 + 0.25 * (speed - 3) / (max speed - 3))` and the rudder fraction.
  A zero rudder cannot turn; an immobile hull turns 1 degree per tick.
- Maximum speed is `class speed * sailmaster multiplier * load factor * sail / max sail`, plus 1
  with SEADOG at the helm, never below 1, and 0 with a breached arc (airships: halved) or no
  sail. The load factor is 1 minus the fit-out and cargo weight above their free allowances,
  divided by the max load.
- `setsail <dir>` is the maneuver command: one room, only at speed 6 or less, 5 s cooldown;
  entering a port room docks the hull (not at battle stations).
- `undock` without a docked hull departs: 30 s from a port, 13 s from anchor. Departure checks
  unpaid dock fees, the captain's level against the class minimum and a legal fit-out.
- `anchor` at speed 0 anchors the hull: faster repairs and stamina regeneration (3.3.5, 3.3.6).
- Autopilot, NPC pilots, schedules, merchants, hunters and ferries all move through the same
  physics; schedule intervals are re-derived from the new voyage times.

#### 3.3.3 Damage

- Arcs: fore 320-40 degrees relative to heading, starboard 40-140, rear 140-220, port 220-320.
- Duris resolution per fragment (1.5): sail-hit chance, spread across arcs, armor then
  internals, a confirmed critical carries half the damage past armor (50% weapon-damage chance),
  15% weapon-damage chance on internal hits, one in three hits on a gutted arc deflects, weapon
  damage disables at 1 and destroys at 100. Warship sails take 85%.
- Knockdown: one internal hit in nine makes everyone aboard save (Reflex DC 15) or fall prone for
  two combat rounds.
- Breach: an arc with armor 0 and internal 0. One breach immobilizes (airships keep half speed);
  two start sinking: 75-150 s for player-owned hulls, 1000-1500 s for NPC hulls. A sinking hull
  cannot move or fire; it can still be boarded, plundered and abandoned.
- When it goes down, occupants enter the water room, loose objects float, and each bulk cargo lot
  spills half its units as salvage crates. `salvage` on a stopped hull hauls crates into its hold.
  NPC hulls are deleted; player-owned hulls follow 3.3.7.

#### 3.3.4 Weapons, fitting and gunnery

- The twelve Duris weapons (1.3) are seeded as data rows with Duris ranges, damage, fragments,
  spread, sail hit, hull/sail percentages, ammo and reloads (60 and 90 ticks). Prices are twice
  the platinum price; selling returns 90% (10% if damaged). Installing takes `weight * 75` s of
  maintenance, which blocks departure.
- Mounts and arc weight caps come from 3.3.1; each weapon keeps its Duris arcs; a class may mount
  what its analog may mount in `ship_allowed_weapons[]`. One capital weapon per hull, needing
  renown of 1,600 (light beam), 1,800 (heavy beam), 1,700 (mind blast), 1,900 (fragmentation) or
  2,000 (Long Tom), or a veteran gunner aboard.
- Equipment, one each: a ram (price 2 gold times hull weight, weight `(hull weight + 10) / 24`)
  and neutral colors (free, weight 0; 3.3.8). `GREYHAWK_MAXSLOTS` rises from 10 to 16 (Duris).
- Ammo costs 2 gold a round at a shipyard, 75 s per weapon; a weapon without ammo cannot fire.
- Commands: `shipweapon list|buy|sell|swap`, `shipequip buy|sell`, `shiprearm [slot|all]` at a
  shipyard; `lock <id>|off`, `shipfire <slot|fore|port|starboard|rear> [id]`, `sight <slot>` and
  `scan <id>` aboard.
- Who may lock and fire: the owner, helm permit holders, members of the online owner's group, and
  immortals. Unowned hulls fire only through their NPC crews.
- Hit: `d20 + gunnery bonus >= DC`. The DC is `21 - round(20 * h)`, where `h` is Duris's
  `weaponsight()` chance after the 2d50 transform for an untrained crew: range band, motion term
  (LuminariMUD speeds divided by 0.3 into Duris units, rates per second), target size from the
  class hull weight, x1.5 miss against a flyer, and crew stamina. The gunnery bonus is the gunner
  tier (+2/+4/+6) plus the firing character's Dexterity (direct fire) or Intelligence (ballistic)
  modifier, capped at +7, the Duris elite-crew ceiling. A natural 1 misses; a natural 20 hits.
- Criticals: threat on 20 for weapons with 2-3% pierce, 19-20 for 10%, 18-20 for 15%, none for
  0%; confirmed by a second roll against the same DC.
- Reload is the weapon reload times `1 - 0.15 * gunner mod` (0.15/0.30/0.45 by tier) divided by
  the stamina modifier.
- Firing needs: not in a port room, not anchored, not sinking, crew not stunned, weapon
  undamaged and loaded, target inside the range band and the weapon's arc. Hulls in a port room
  cannot be locked or fired on, and a lock on a hull that enters port drops; hulls docked to
  each other at sea stay targetable.
- Battle stations last until 180 s after the lock clears. They block entering a port and
  summoning, and cut repair odds (3.3.6).
- Flight: every vessel range (weapons, docking, boarding, sight) counts one room per 10 Z. A
  weapon cannot reach a hull whose vertical separation exceeds its maximum range; an airborne hull
  can only be boarded from within 10 Z; rams pass under it.
- Submerged hulls (Z below 0) cannot fire or be targeted (LuminariMUD-only).
- Crew stun weapon: the target crew is stunned for 5 to 20 seconds, `5 + 15 * closeness` where
  closeness runs from 0 at maximum range to 1 at minimum range (no steering, firing, reloading or
  repair); characters aboard inside mid-range save (Will DC 15) or fall prone for two rounds.
- Time to kill (D2): equal warships reach a 3-8 minute median with a p95 of 12 minutes and nothing
  under 90 s. The duel harness moves to these rules and bounds; weapon reload times are the
  tuning lever, armor and damage stay at Duris values.

Calibration reference, DC for an untrained crew against a warship (frigate) target:

| Duris motion term | Quarter range or less | Half | Two thirds | Five sixths | Maximum |
| -: | -: | -: | -: | -: | -: |
| 0 (stopped) | 1 | 2 | 3 | 4 | 6 |
| 20 | 1 | 2 | 4 | 7 | 11 |
| 40 | 1 | 3 | 6 | 10 | 14 |
| 70 | 2 | 4 | 8 | 12 | 16 |
| 100 | 2 | 5 | 9 | 13 | 17 |

#### 3.3.5 Crew (D4: one-time hire)

- Wages, paydays and walk-offs are removed and `shipwages` is retired; wages owed are cleared.
- Hire prices, paid once (Duris chief prices times 2):

| Position | Green | Able | Veteran | Able gate (renown) | Veteran gate (renown) |
| -- | -: | -: | -: | -: | -: |
| Sailmaster | 1,600 | 6,000 | 15,000 | 540 | 1,350 |
| Gunner | 2,400 | 8,000 | 18,000 | 700 | 1,640 |
| Bosun | 2,000 | 7,000 | 16,000 | 640 | 1,480 |
| Quartermaster | 1,200 | 4,500 | 11,000 | 540 | 1,350 |

- Experience per position in Duris skill points, starting at the tier floor when hired:
  sailmaster 200/800/2,000, gunner 250/1,000/2,500, bosun 220/900/2,200, quartermaster
  200/800/2,000 (green/able/veteran). Reaching a floor promotes the position without paying the
  gate.
- Gains (Duris rates): every position gains the target's hull weight when its hull sinks a
  player hull and a tenth of it for an NPC hull. Sailmaster +0.003 per room sailed (not raft or
  boat), +1 to +3 per ram attempt on a hostile, +1.5 per 2,000 gold of cargo sold. Gunner +0.0015
  per reload tick with a hostile locked, +0.1 per shot at a hostile. Bosun +0.1 per repair at
  battle stations against a hostile, +0.01 per other repair, +0.5 per 2,000 gold of cargo sold.
  Quartermaster +1.5 per 2,000 gold of cargo sold.
- Casualties when the hull sinks: every position loses `10 + renown lost / 30` percent of its
  experience to a player, `5 + hull weight / 100` percent to an NPC, and drops a tier if it falls
  below the floor.
- Effects: sailmaster multiplies maximum speed, accel and turn by 1.1/1.2/1.3; gunner +2/+4/+6
  gunnery and a gunner mod of 0.15/0.30/0.45; bosun a repair mod of 0.15/0.30/0.45; quartermaster
  +10/20/30% cargo capacity.
- Stamina: maximum `500 + 100 * sum of the four tiers`; regeneration 1.5 per tick, four times
  that docked or anchored; Duris costs for speed and heading changes, firing and repairs; below
  zero, `1 / (1 + deficit / max / 3)` scales accel, turn, reload, repair odds and the hit chance.

#### 3.3.6 Repair

- Repair stock equals the class hull weight and refills in port.
- Each tick the crew may repair at Duris's per-mille odds (halved per 0.5 s tick) times
  `(1 + bosun mod) * stamina modifier`, with Duris's anchored, underway and battle-station
  columns. Sails only below `max * (bosun mod + 0.4)`, internal only below
  `max * (bosun mod + 0.1)`, damaged weapons at any time; armor is never repaired at sea.
- `shiprepair` at sea is one character repair: Craft (woodworking) DC 15, one point of stock,
  same caps, 12 s lag. At a shipyard it buys dock repairs: armor and internal 2 gold a point, sail
  4, each order `75 + points` s of maintenance; a damaged weapon 2 gold per damage point (75 s), a
  destroyed one half its price (150 s).

#### 3.3.7 Loss, insurance and renown (D3: the Duris model)

- A player-owned hull that sinks becomes a wreck replacement hull: the cheapest for-sale boat
  prototype (else the cheapest for-sale prototype, else the `vedit` boat defaults), with every
  weapon, equipment slot and refit removed and no sail unless an NPC sank a larger hull. It waits
  out of the world in the wreck registry until summoned, and keeps its name, owner, crew (after
  casualties), renown (after the loss share), cosmetics and helm permits.
- Insurance (3.3.1) settles automatically through the existing settlement path: paid now to an
  online owner, at next login otherwise. `shipinsure` is retired and premiums on active policies
  are refunded.
- Rewards go to the owners of the victor and of every allied hull in sight whose owner is grouped
  with the victor's owner, split equally, through the same settlement path: salvage
  `(hull price * fraction of armor and internal left + surviving weapons at half price) / 8`, a
  renown bounty of `2.5 gold * target renown` when that renown exceeds 100, and the WANTED or
  HUNTED bounty of the target's owner when the owner was aboard.
- Renown lives on the hull. Sinking a consenting player's hull adds the target's hull weight,
  split among the allied hulls in sight; the loser drops that share, floored at zero. NPC kills
  give crew experience, not renown. `shiprenown` lists the top 10 of a top-20 board.
- Ship Damage Control is an epic feat with 5 ranks: `4 + 4 * rank` percent less hull and sail
  damage from other ships while its holder owns the hull and is aboard (24% at rank 5), minimum 1
  point.
- Trade-in: `shipbuy <id> trade` at a port where the owner's hull is berthed with an empty hold
  credits 90% of the old hull's price and carries over name, crew, renown and any weapons the new
  class accepts (the rest sell at 90%). Renaming costs 10% of the hull price.

#### 3.3.8 NPC raiders

- Eligible hulls: moving, no lock set, player-owned, not raft or boat, not in port.
- Roll 1 in 2002 per tick (one success per ~17 minutes of sailing), doubled in pirate-cove
  waters, halved in territorial waters, divided by 60 under neutral colors. Ships and
  transports are ambushed at most once per voyage; docking in port resets it.
- Tier: `n = random(0, hull weight) + renown`. Merchant classes: below 250 tier 0, below 1,200
  tier 1, below 2,200 or three times in four tier 2 (one in three a hunter), otherwise a tier 3
  hunter. Other classes must pass `n >= random(1, 1000)` to be noticed, then draw a tier 2 hunter,
  or tier 3 one time in three.
- Spawn: the current sight range plus 10 rooms off the target's bow within 45 degrees, headed at
  it at full speed, from the tier's raider prototypes, preferring one at least as fast as the
  target's maximum minus 3.
- Tiers (content rows in a raider tier table, with prototypes and zone 700 mobiles and objects):

| Tier | Hulls | Renown carried | Crew | Advanced AI |
| -: | -- | -- | -: | -- |
| 0 | Ship-class raiders (Duris clipper, ketch, caravel) | 150-300 | 8-12 | Never |
| 1 | Ship and light warship raiders | 500-600 | 9-12 | One in five |
| 2 | Warship raiders (corvette, destroyer) | 700-1,000 | 12-15 | One in two |
| 3 | Heavy warship raiders (destroyer, frigate) | 2,000-3,000 | 12-18 | Always |

- Each raider has a pilot (the captain), crew mobiles, and a chest in its hold with the key on
  the pilot. Killing the pilot stops its AI. Raiders never select NPC hulls, are never saved, and
  despawn 600 ticks after losing their target, 20 ticks later for as long as a player hull is in
  sight.
- AI modes: engaging (basic: rank the arcs, turn the best onto the target, open or close to the
  weapons' band; advanced: project both hulls several ticks ahead and choose the broadside),
  running (no ammo left or a breached arc), cruising, leaving and looting.
- Boarding when the target's speed is 3 or less (merchant classes) or 0 (others): crew mobiles
  take the bridge, then three quarters of the rooms (tiers 0-1) or half (tiers 2-3), through the
  boarding contest. Pirates take cargo up to their capacity, losing 40-60% in transfer and leaving
  40-60% behind, then leave; hunters fight on.
- NPC merchants under fire return fire with bearing weapons and run.
- Neutral colors: raiders do not pick the hull as a target and ambushes are 60 times rarer, but
  cargo sells for 10% less; they cannot be removed while cargo is aboard.

#### 3.3.9 Economy, services and client data

- Contraband: a flag on `trade_commodities`. Seed three goods: forbidden tomes (190 gold, renown
  150), rare poisons (210, renown 200) and dragon eggs (310, renown 250). Buying needs that renown
  or an able sailmaster and quartermaster; warships cannot buy it, nor can a captain at alignment
  1,000.
- Customs at every lawful port (not pirate coves), per contraband unit not stocked by that port:
  `c = 35 + units / 2 - sqrt(renown) / 5`, raised by `(100 - c) * (1 - load / capacity)`, capped
  at 100, 5 if negative.
- Cargo sales: SEADOG +10%, neutral colors -10%, warships -40%.
- Summon: `shipsummon` at a shipyard costs `0.1 gold * hull weight` and takes
  `50 / max(speed, 2)` mud hours for rafts and boats or `70 / max(speed - 20, 2)` for the rest,
  with the empty-hold maximum speed in Duris units, doubled from the wreck registry, capped at 60
  mud hours (75 minutes). It is refused while sinking or at battle stations, empties the hold,
  survives reboot and copyover, and docks the hull at that shipyard.
- Scan: `scan <id>` within 20 rooms, 22 with a posted lookout: per-arc armor and internal,
  weapons with destroyed ones marked, status, and whether the owner is WANTED, HUNTED or holds a
  letter of marque.
- MSDP adds `SHIP_ID`, `SHIP_TARGET`, `SHIP_ARMOR`, `SHIP_INTERNAL` (fore/port/rear/starboard,
  current and maximum), `SHIP_SAIL`, `SHIP_SAIL_MAX`, `SHIP_RUDDER`, `SHIP_RUDDER_MAX`,
  `SHIP_STAMINA`, `SHIP_STAMINA_MAX`, `SHIP_WEAPONS` (slot, name, arc, ammo, ready, damage) and
  `SHIP_CONTACTS` (the contact list fields), keeping the existing variables.

#### 3.3.10 Migration

- Prototype armor is rescaled by the class-default ratio (new default over old: warship 40 to
  109); the eight frontier class prototypes are for sale, while harbor, admiralty, merchant,
  derelict, hunter and event prototypes are not.
- Live hulls keep their damage fraction under the new condition values; default ballistae become
  large ballistae (warship bow, port and starboard) or a medium ballista (other armed classes)
  with full ammo; refits are recomputed at the new sizes; wages owed are cleared and active
  insurance premiums refunded.
- Owners above the ownership cap keep their hulls but cannot acquire more until below it.

### 3.4 Owner decisions (2026-09-28)

- D1 Pacing: the Duris scale, `speed / 90` rooms per 0.5 s tick.
- D2 Time to kill: 3-8 minute median for equal warships.
- D3 Loss model: Duris; the hull is lost, the ship's identity survives (3.3.7).
- D4 Crew economics: one-time hire, no wages (3.3.5).
- D5 Ownership: a small configurable cap, 3 hulls per owner by default, `cedit` range 1-10;
  public and NPC hulls do not count, and a capture at the cap is refused.
- D6 Capture: only disabled prizes: a breached arc, immobile, colors struck (`strikecolors`,
  owner or permit holder, until the hull moves or 10 minutes pass), or abandoned at sea (no
  conscious character or crew aboard). Hostile boarding needs the target at speed 3 or less or
  disabled.

## Part 4: Defects found in LuminariMUD

All verified in the source at the baseline revision.

| # | Defect | Evidence | Fix |
| -- | -- | -- | -- |
| L1 | Rudder damage does nothing. Heading changes are instant and `turnrate` is read only by repair, `shipfix`, persistence and wear; "The helm no longer answers" is false. Violates requirement 4.4. | `src/vessels/vessels.c:2451-2452`, `src/vessels/vessels_combat.c:580-587` | Rudder scales turn rate (S2) |
| L2 | Rigging damage is undone by the next `speed` order. A collapse zeroes speed once, but neither `speed` nor autopilot reads `mainsail`; storm and wear sail loss have no effect. Violates requirement 4.4. | `src/vessels/vessels_combat.c:570-578`, `src/vessels/vessels.c:2267-2330`, `src/vessels/vessels_hazards.c:662-671` | Sail fraction limits maximum speed (S2) |
| L3 | Movement ignores time. Autopilot moves `speed` rooms per 0.5 s; `setsail` jumps `speed / 10` rooms per command with no lag; both check only the destination, so a hull hops land narrower than its step. | `src/vessels/vessels_autopilot.c:2185`, `src/vessels/vessels.c:1927-1928`, `:2730-2768` | Duris physics and per-cell checks (S2) |
| L4 | Manual movement reads weather on the wrong scale. `get_weather()` is 0-255, but `move_ship_wilderness()` treats above 50 as a storm (25% shorter move), passes `weather / 25` to the speed modifier (5% or 10% off per unit) and prints storm text above 25, 50 and 75, so clear weather (0-127 elsewhere) slows and "storms" manual sailing. | `src/wilderness/wilderness.c:373`, `src/vessels/vessels.c:1930-1937`, `:2054`, `:2086-2103` | Use the `VESSEL_WEATHER_*` bands (S1) |
| L5 | Any passenger can fire the ship's weapons, while speed, heading and `setsail` need `is_pilot()`. Consent is checked for the firer, not the hull's owner, so a PvP-flagged passenger can make a non-consenting owner's hull attack while retaliation against that hull is refused. | `src/vessels/vessels_combat.c:812-921`, `:146-230`; `src/vessels/vessels.c:2282`, `:2425`, `:2743` | Owner, permits and the online owner's group only; the owner's consent is checked too (S1) |
| L6 | The hit roll is a level stand-in, and `speed` is a setting: a stopped hull with `speed 30` set gets +6 defense, and a level-30 gunner cannot miss. Range, size and motion are ignored. | `src/vessels/vessels_combat.c:711`, `:895-896` | Geometry DC and gunnery bonus (S4) |
| L7 | Misses have no command lag: `WAIT_STATE` runs only after a hit. | `src/vessels/vessels_combat.c:905-918` | Lag before resolving (S1) |
| L8 | Targeting. `shipfire` matches the two-letter ID or any name prefix across the whole fleet in slot order; the `tactical` roster prints the numeric slot as "ID", which `shipfire` rejects; `contacts` prints names only, uses a fixed 50-room range that ignores fog, and keeps the first 20 ships by slot rather than the nearest 20. | `src/vessels/vessels_combat.c:781-807`, `src/vessels/vessels_tactical.c:508-515`, `src/vessels/vessels.c:2118`, `:2479-2564` | One contact list within sight range, two-letter IDs everywhere, name prefixes only among contacts (S1) |
| L9 | No safe harbor and no combat lockout: a hull berthed in port can be shot (a consenting owner's hull, or any unowned public or NPC hull), and a hull under fire can dock or disembark at once. | `src/vessels/vessels_combat.c:812-921` | Hulls in port untargetable (S1); battle stations block entering port (S4) |
| L10 | Free unlimited repairs: any passenger, no cost or materials, mid-fight whenever speed is 0, to 100%. A refit also restores armor or internal to full. | `src/vessels/vessels_combat.c:924-986`, `src/vessels/vessels_upgrades.c:590-616` | Repair stock, caps and priced dock repairs; refits no longer repair (S5) |
| L11 | The shipyard lists and sells every prototype, including NPC merchant, hunter, derelict, ghost-fleet and test hulls, to anyone, with no level gate and no per-owner cap: one player can fill the 500-slot fleet and the shared room pool with 56-gold rafts. | `src/vessels/vessels_edit.c:726-835` | `for_sale` and `min_level` fields, departure level check, cap of 3 (S1) |
| L12 | Crew wages accrue every 300 s whether or not the owner is online or the hull is used; after three unpaid paydays one crew member leaves per payday, so a full crew is gone about 35 minutes after logout. | `src/vessels/vessels_crew.c:387-426` | Wages removed, one-time hire (S1) |
| L13 | Bounties can never be cleared. `vessel_clear_bounty()` has no callers and nothing decays or pays off `vessel_bounties`; a WANTED captain is refused by lawful ports forever and a HUNTED one stays eligible for bounty hunters forever, while the MARQUE help says "settle your affairs first". | `src/vessels/vessels_piracy.c:624-640`, `lib/text/help/help.hlp:25409-25410` | Pay-off at 125% at any lawful port, 5% decay per real day after 24 hours without a new offense (S1); collection by victors (S7) |
| L14 | Capture and plunder do not need a disabled prize: an uncontested bridge is enough, and hostile boarding has no target-speed limit (friendly docking needs speed 2 or less). An unattended, consenting owner's berthed ship can be claimed outright. | `src/vessels/vessels_combat.c:988-1044`, `src/vessels/vessels_piracy.c:824-954`, `src/vessels/vessels_docking.c:275`, `:565-625` | D6 disabled-prize rule and boarding speed limit (S3) |

## Part 5: Implementation sequence

Each step ships with production-linked CuTest coverage in `unittests/CuTest/`, help in both
`lib/text/help/help.hlp` and the database (`sql/components/help_vessel_entries.sql`), schema and
rollback SQL where tables change, `VESSEL_SYSTEM.md` updates, and, where play changes, an
actual-character gate in the `scripts/vessels/` pattern.

1. S1 Defects that do not wait for the redesign: L4 weather bands, L5 gunnery authorization and
   owner consent, L7 miss lag, L8 contact list and IDs, L9 port immunity, L11 `for_sale`,
   `min_level`, departure level check and the ownership cap (D5), L12 wage removal (D4), L13
   bounty pay-off and decay.
2. S2 Movement and pacing (D1): fractional movement with per-cell validation, class accel and
   turn with the sailmaster and rudder factors, the load and sail limits on maximum speed,
   `setsail` as the maneuver command, `undock` departure and `anchor`, SEADOG +1 speed, the class
   table's speeds, and every automated mover rebased. Ferry soak and scale benchmark are
   re-baselined in `docs/testing/VESSEL_BENCHMARKS.md`.
3. S3 Damage model: Duris arcs and armor profiles with the 229 armor limit, sail HP, breach
   states and sink timers, criticals, weapon damage, knockdown saves, cargo spill and `salvage`,
   and the D6 capture, plunder and boarding rules with `strikecolors`; also the 3.3.1 refit
   rescaling (plating and reinforcement +20%, rigging +10% speed), moved here from S2.
4. S4 Weapons and gunnery: the weapon and equipment tables, 16 slots, fitting, ammo and resupply
   commands, `lock`, battle stations, arc fire, the geometry DC hit model, `sight`, `scan`,
   crew-stun and flight rules; the duel harness moved to the D2 bounds; and, with battle
   stations, the 3.3.2 crash check for refused rooms and shallows, moved here from S2.
5. S5 Crew, repair and loss: one-time hire prices and gates, crew experience, promotion and
   casualties, stamina, the repair stock and dock repairs, the D3 wreck registry with automatic
   insurance, `shipsummon`, trade-in and the rename fee.
6. S6 NPC raiders and AI: the hunter lifecycle generalized into the raider tiers with the 3.3.8
   cadence, fit-outs from prototypes and weapon rows, basic and advanced AI, NPC boarding and
   looting, despawn rules, ramming with the ram, and neutral colors.
7. S7 Rewards and economy: renown, salvage and bounty payouts, the renown board, Ship Damage
   Control, contraband and customs, and the trade modifiers.
8. S8 Client: the MSDP additions.

### Phase 1 (S1) progress

Branch `feat/vessels-ships` (GitLab `gitlab/feat/vessels-ships`). Each row landed as its own
commit with tests, help in both places, and `VESSEL_SYSTEM.md`. S1 is complete; the next step is
S2 (movement and pacing). The vessel help SQL was applied to the development database on
2026-09-28; production help and the Phase 18 schema are not deployed.

| Defect | State | Where |
| -- | -- | -- |
| L4 weather bands | Done | `vessel_manual_move_distance()` and `vessel_storm_severity()` in `move_ship_wilderness()` |
| L5 gunnery authorization, owner consent | Done | `vessel_gunnery_permitted()`, `vessel_fire_permitted()` in `vessels_combat.c` |
| L7 miss lag | Done | `WAIT_STATE` before the hit roll in `do_shipfire()` |
| L8 contact list and IDs | Done | `vessel_collect_contacts()`, `vessel_find_contact()` in `vessels.c`; `contacts`, `tactical`, `shipfire` share them |
| L9 port immunity | Done | `vessel_ship_is_in_port()` gates player fire and `vessel_ai_return_fire()` |
| L12 wage removal (D4) | Done | Payroll, walk-offs, and `shipwages` removed; 3.3.5 one-time hire prices in `vessel_crew_hire_cost()` (renown gates stay in S5); `wages_owed`/`wage_ticks` columns kept unread, zeroed by the Phase 18 SQL |
| L11 `for_sale`, `min_level`, departure level, cap of 3 (D5) | Done | `ship_prototypes.for_sale`/`min_level` (Phase 18 SQL, `vessel_prototype_ensure_schema()` at boot); `vessel_helm_level_refused()` on `setsail` from port, `autopilot on`, `assignpilot`, `setschedule`; `vessel_owner_at_cap()` on `shipbuy`, `claimship`, `shipdeed`; `cedit` "Vessel Hulls Per Owner" (`CONFIG_VESSEL_OWNER_CAP`); frontier prototypes for sale |
| L13 bounty pay-off and decay | Done | `vessel_bounties.last_offense_at` (Phase 18); `vessel_bounty_after_decay()`, `vessel_bounty_record_offense()` (plunder and merchant paths), `bounty pay` (125%, `vessel_bounty_payoff_cost()`); collection by victors stays in S7 |
| Live gate in `scripts/vessels/` | Done | `test_vessel_rules_in_game.sh` (tactical harness `--rules`, login helper `--vessel-rules-check`); passed 2026-09-28 with the tactical, events, boarding, lookout, and narrative gates (see `VESSEL_SYSTEM_TESTING.md`) |

Notes for whoever continues:

- Tests: `unittests/CuTest/test_vessel_gunnery.c`, `test_vessel_shipyard.c`, and
  `test_vessel_bounty.c` (new); run
  `CUTEST_FILTER=vessel LUMINARI_TEST_ROOT="$PWD" LUMINARI_TEST_SPEC_WORLD_ROOT="$PWD/unittests/CuTest/fixtures/spec_world_inventory" ./cutest`.
  The suite has no booted world: tests that reach `vessel_ship_is_in_port()` install a one-room
  fake `world` (see the duel harness), and `find_static_room_by_coordinates()` now returns
  `NOWHERE` while the wilderness kd-tree is unbuilt.
- DB-backed cases run only with `LUMINARI_TEST_MYSQL_ENABLE=1` plus the `LUMINARI_TEST_MYSQL_*`
  connection variables. A disposable server:
  `docker run -d --rm --name luminari-vessels-testdb -p 127.0.0.3:3306:3306 -e MARIADB_ROOT_PASSWORD=root -e MARIADB_DATABASE=luminari_test -e MARIADB_USER=luminari_test -e MARIADB_PASSWORD=test_password mariadb:10.11`,
  then grant `luminari_test` all privileges and load `sql/master_schema.sql`.
- Help: every text change goes to `lib/text/help/help.hlp` and
  `sql/components/help_vessel_entries.sql` identically; `verify_help_vessel_entries.sql` guards
  key sentences (`content_contracts`).
- `scripts/development/dev_kohdee_login_smoke.sh` expects the new "No contact in sight matches"
  refusal.
- The live gates assume they own the development MUD on 4100 through a systemd user unit. When
  the main checkout's MUD holds 4100, run them in a private namespace:
  `unshare -r -n -m --pid --fork --mount-proc`, bring up `lo`, bind-mount a scratch directory over
  `/run/mysqld`, then `unshare --map-user=1000 --map-group=1000` into a script that starts a
  disposable `mariadbd` on that socket, loads a `mariadb-dump` of the development database
  (create the `mysql_config` user and the `luminari_mud` trigger definer first), puts
  `systemctl`/`systemd-run` stand-ins first on `PATH` (pid files; the launched server must not
  inherit the gate's lock descriptors), and runs the gates. The dump lacks stored routines; the
  server recreates them at boot.
- Phase 1 verification (2026-09-28): `make test-all` passed with the database cases enabled
  (1868 CuTest cases, isolated `.ci-runtime/lib` from `scripts/ci/prepare_test_runtime.sh`), and
  again with the MR !6 review fixes (1869 cases); all six live vessel gates passed on both.

### Phase 2 (S2) progress

Branch `feat/vessels-s2` from merge commit `34dcbb34e` (annotated tag `vessels-s2-base`, pushed
to `gitlab`). The review range is `vessels-s2-base..vessels-s2`; review fixes go on top
(`vessels-s2..feat/vessels-s2`). Follow the step workflow recorded for S1: one branch, a merge
commit at the end, never a squash. S2 is complete and handed to review as the annotated tag
`vessels-s2` with GitLab merge request !7 from `feat/vessels-s2`; the fixes for its first review
round are `vessels-s2..feat/vessels-s2` (see "MR !7 review fixes" below). MR !7 merged on
2026-09-29 as merge commit `89cfabcbe`; S3 continues on `feat/vessels-s3` (Phase 3 below).

| Item | State | Where |
| -- | -- | -- |
| Class handling table (3.3.1 speed, accel, turn, weight budget, allowances) | Done | `vessel_class_handling()` in the new `src/vessels/vessels_movement.c`; `vedit new` takes the class speed |
| Maximum speed: sailmaster, load, sail (L2), terrain/weather/lane, SEADOG +1 | Done | `vessel_max_speed()`, `vessel_max_speed_from()`, `vessel_load_factor()`, `vessel_helm_speed_bonus()` |
| Momentum: accel, turn with sailmaster and rudder (L1), speed / 90 rooms per tick (L3) | Done | `vessel_movement_tick_one()` from `vessel_owner_event()` after the autopilot and hunter ticks |
| Per-room validation; refused room stops the hull at its edge | Done | `vessel_cross_room_edges()` crosses edges in the order the track meets them, diagonally only through a corner (MR !7 review); `vessel_check_grounding()` is removed (see the grounding deferral below) |
| Berth at rest in port, `undock` departure (30 s / 13 s), `anchor` | Done | `vessel_berth()`, `vessel_sync_berth()`, `vessel_begin_departure()`, `do_vessel_anchor()`; `do_undock()` departs when no hull is alongside |
| `setsail` as the maneuver command | Done | `vessel_maneuver()`; the S1 departure level check moved from `setsail` to `undock` |
| Automated movers rebased | Done | Autopilot steering `vessel_autopilot_steer()` with `autopilot_data.speed_limit`; hunters steer and shadow; merchants cruise at design speed; scheduled routes validated by sailing a copy of the hull through the same steering and `vessel_sail_tick()` (MR !7 review) |
| Unit tests | Done | New `unittests/CuTest/test_vessel_movement.c` (20 cases with the MR !7 review); updates in `test_transport_production.c`, `test_vessel_gunnery.c`, `test_racial_innate_feats.c` |
| Help in both places, `VESSEL_SYSTEM.md` | Done | VESSELS (SPEED, HEADING, SETSAIL, UNDOCK, new ANCHOR keyword), AUTOPILOT, SEADOG; "Movement and Pacing" section |
| Existing live gates moved to the new model | Done | `scripts/development/dev_kohdee_login_smoke.sh` and the `scripts/vessels/` gates (list below); all pass (see Live gate results) |
| New actual-character movement gate | Done | `--vessel-movement-check <warship-id>` in the login helper, `scripts/vessels/test_vessel_movement_in_game.sh` (tactical harness `--movement` mode) |
| Ferry soak and scale re-baseline in `VESSEL_BENCHMARKS.md` | Done | "S2 Momentum Re-baseline": ferry soak PASS (2,736 s, 11 loops, exact restart); native 500-hull measurement (the fleet-heartbeat runner is retired), mean 500-hull tick 1.34 ms, no pulse over 100 ms |

Interpretations and deferrals decided while building S2:

- "Class speed" in the maximum-speed formula is the hull's design speed (`maxspeed`, the
  prototype's speed, which defaults to the class speed), so `vedit set speed` keeps its meaning.
- The terrain, weather, and altitude-lane percentage (LuminariMUD-only) multiplies the maximum
  speed; it is cached per room in `position_speed_percent` and refreshed on room entry and on
  every hazard check.
- Refit rescaling (rigging +10%, plating and reinforcement +20%) moves to S3 with the other
  refits: rigging still adds 5 to the design speed. Per-class sail HP also stays for S3.
- Water depth does not block movement in S2. 3.3.2 made shallows a barrier ("Bathymetry
  grounding uses the same rule"), but a live survey showed the Central Vailand Sea Port
  (-467,204) and its approach at elevation 127, one unit of water: a draft barrier closes every
  seaport to ship-class hulls, and the old autopilot never checked depth. The old manual
  `setsail` grounding (2d4 bow damage) is removed with it. S4 brings grounding back as the
  battle-stations crash check for land and shallows alike; until then a refused room (land,
  dock-fee clearance) only stops the hull.
- The anchor is runtime-only; a reboot drops it. The berth persists in the existing `dock_room`
  column, and `vessel_sync_berth()` berths any hull found at rest in port at load or spawn. No
  schema change was needed; `dx`/`dy` (already persisted) hold the position inside the room.
- `autopilot pause` holds the hull (speed cap 0); `autopilot off` leaves it on its ordered speed
  and heading, as in DurisMUD. With no ordered speed the autopilot cruises at full speed.
- `setsail up` and `down` are exempt from the speed-6 limit and keep the hull under way;
  horizontal maneuvers stop it, as DurisMUD's `order maneuver` does.
- Merchants cruise at their design speed instead of half of it; the study's 22-minute Vailand
  voyage assumes the cog's speed 12.
- Every scheduled route in the content loops, so schedule intervals only time the first
  departure; no interval needed re-deriving.
- `docked_to_ship > 0` marks a hull alongside in the new checks (fleet slot 0 is reserved and
  zeroed test fixtures carry 0).
- `speed` and `heading` are now `double`; `vessel_display_speed()` and `vessel_display_heading()`
  round them for output, persistence, and integer rules. The new fields fit struct padding, so
  `greyhawk_ship_data` stays at the 5 KiB budget (5,120 bytes); `autopilot_data` grew to 80.

Live gates that assume the old model (from a survey of `scripts/`): the builder, frontier,
regatta, narrative, and hunter checks in `dev_kohdee_login_smoke.sh` (instant speed, `speed`
then multi-room `setsail` at speed 10, exact coordinates), the harbor provisioner's fare and
crossing sessions (60 s and 45 s bounds at ferry speed 2), the campaign provisioner (arrival at
the central port inside a 45 s window), and the ferry soak (one loop per 15-minute sample at
speed 2, a paused ferry must not move). Hulls spawned at the (-66,92) seaport now start berthed.
The retired scale runner's text checks in `test_vessel_scale_benchmark_parsers.sh` still pass
unchanged.

Verification (2026-09-29): `make` clean with the strict warning set, also with
`-DVESSEL_SYSTEM_DEBUG=1` for the vessel sources; `make test-all` with the database cases on
(1887 CuTest cases); live gates, ferry soak, and scale measurement as below.

Live gate results (2026-09-28/29). The main checkout's development MUD holds port 4100, so every
gate ran inside a private user, network, mount, and PID namespace (`unshare -r -n -m --pid`)
with a disposable MariaDB loaded from a dump of the development database on a private
`/run/mysqld`, and process stand-ins for `systemctl --user` and `systemd-run`. All passed. On
the final binary (source `9a8757fce`, SHA-256 `62418bed...`): builder 50 s, tactical 99 s,
lookout 24 s, boarding 52 s, narrative 25 s, rules 39 s, events 46 s, movement 107 s. On the
preceding install from identical server sources (`92fc0b27f`): frontier 229 s, derelict 39 s,
campaign 141 s, merchant 31 s, hunter 77 s. `make test-all` passed with the database cases on
(1887 CuTest cases) and the `quality-clang-tidy` local CI job is clean.
The harbor provisioner cannot run in this checkout: the west Testing Dock (room 1000389) is
absent from the world files, so the builder and movement checks stage at the east dock
(1000390, at (-62,82)) instead. Fixes the gates forced, all committed on the branch:

- NPC and public hulls wear sail and rudder down to 1 of 20 over a long soak; with the new
  sail and rudder factors they barely moved. An unowned hull now has its rigging made good
  when it berths (the harbor service), until S5 crew repairs replace it.
- A per-axis room step refused diagonal courses; the crossing now enters the room the
  position lies in, diagonally when both edges are crossed in one tick (DurisMUD rule). The
  MR !7 review replaced this with time-ordered crossing (below).
- The draft barrier closed every seaport (see the water-depth deferral above).
- A waypoint astern made the autopilot circle; it now comes about in place (speed cap 0
  above 90 degrees of heading error, steerage 2 above 45).
- A hull coming to rest now saves its runtime row, so a restart finds it where it stopped.
- `reglist type 5` and `pathlist` had been broken since `500019db1` (zone/VNUM bound parsing
  ran for them); `src/olc/oasis_list.c` skips it for those two lists.
- The hunter, derelict, and campaign gates needed waits sized for undock and acceleration,
  `--skip-tz-utc` for the derelict snapshot, and the hunter's reattach comparison excludes
  the live `last_attacker` combat pointer (saved as 0 when the hull comes to rest).

MR !7 review fixes (2026-09-29), one commit each on `feat/vessels-s2`:

| Finding | Fix | Commit |
| -- | -- | -- |
| High: an off-axis tick could skip an impassable room (both edges past in one tick stepped diagonally) | Edges are crossed in the order the track meets them; only a track through the corner enters the diagonal room; a refused room leaves the hull where its track met the edge. This departs from DurisMUD, which steps diagonally whenever both edges fall in one tick | `f58e6083c` |
| Medium: schedule preflight ignored momentum (a hull carrying its way through a turn entered rooms the room-step check never saw) | `scheduled_route_is_traversable()` sails a copy of the hull through `vessel_sail_tick()` and the shared autopilot steering, checking rooms with `vessel_chart_cell()`; loops run on to their second waypoint; `vessel_autopilot_next_position()` is removed | `e74d18928` |
| Medium: `setwaypoint` stored a five-room arrival radius, so a created route stopped short of its port | `AUTOPILOT_ARRIVAL_TOLERANCE` (0.5) for new waypoints and the arrival fallback; legacy migration 2026092901 moves rows at 5.0 to 0.5 (the development dump had three, all July test rows) | `bcc9435e0` |
| Medium: boot reconciled the berth before loading the owner, so an owned hull at rest in port got the free harbor repair | `vessel_db_restore_berth()` loads the owner first | `f6eb99cc8` |

The corrected schedule check then rejected the Vailand Iron Passage. Sailed through the momentum
physics, its southwest leg clipped the beach at (-508,215), the approach to the central port
clipped the beach corner at (-468,204), and the northwest leg from the central offing turned into
the spit at (-501,192). The S2 gate runs had already logged the merchant refused at (-509,214)
without a gate noticing, because the campaign gate sails only the last leg. `808d1edc9` moves the
southwest turn to (-513,215), the central offing to (-504,191), and the harbor offing to
(-467,193), so the last leg runs due north into the port; `setschedule` on a copy of the loop from
the north port accepts it, and the passage is 368 rooms. Deploy: re-apply
`sql/components/vessels_campaign_content.sql` with this code; migration 2026092901 runs at boot.
`571ec69d0` gives the login helper's timed crew reports (cast off, weigh anchor) one ten-second
grace wait: the S2 movement pass caught the cast-off report at the end of its 33-second window, and
a review-round run missed it.

Review-round verification (2026-09-29): `make test-all` with the database cases on (1890 CuTest
cases); all 33 local CI jobs on `e74d18928`, and again on the final review head; the live gates on
the installed review-fix binary (SHA-256 `1ca7d59c...`) against a fresh copy of the development
database with the updated campaign content applied: campaign 153 s, builder 39 s, tactical 101 s,
lookout 25 s, boarding 53 s, narrative 25 s, rules 38 s, events 45 s, movement 109 s, frontier 232
s, derelict 42 s, merchant 31 s, hunter 87 s, and movement 111 s and builder 63 s again on the
helper fix `571ec69d0`. The ferry soak passed again (`run_vessel_ferry_soak.sh start 2700 60 900`,
source `808d1edc9`, same binary): 2,735 s, 10 route completions, 280 movement steps, 40 arrivals,
the paused position exact across the final restart, and no ferry errors; the three Vailand hulls
sailed the new route throughout with their schedules enabled. An earlier batch on the old Vailand
coordinates failed the campaign gate (the merchant's schedule had been disabled) and once the
movement gate (the report timing above).

Found during S2 and outside its scope (not fixed): the hub-and-spoke interior generator in
`src/vessels/vessels_rooms.c` cycles the bridge's spokes through only eight directions, so a
hull with ten interior rooms (nine spokes) overwrites the bridge's north exit and persists two
connections in that direction; at the next boot `restore_ship_connection()` logs
`SYSERR: Ship N persistence has conflicting connection` and drops one. One of the 500 hulls
spawned for the scale measurement (a 10-room Sablebranch Grand Freighter) hit it.

### Phase 3 (S3) progress

Branch `feat/vessels-s3` from the S2 merge commit `89cfabcbe` (MR !7). The annotated tag
`vessels-s3-base` marks `788f1ad67`, the reviewed S2 head, whose tree is identical to the merge,
so `git log vessels-s3-base..vessels-s3` also lists that merge commit (no changes). Hand-off:
annotated tag `vessels-s3` at the head given to review and a GitLab merge request from
`feat/vessels-s3`; review fixes go on top. Scope: 3.3.3, the S3 parts of 3.3.1 and 3.3.10, and
D6 (Part 5, step 3). The damage model lives in the new `src/vessels/vessels_damage.c`.

| Item | State | Where |
| -- | -- | -- |
| Class condition profiles (3.3.1 per-arc armor and internal at the beam armor, sail HP), `vedit` armor limit 229, class prices and the prototype price formula | Done | `vessel_class_condition()`, `vessel_initialize_condition()` (moved from `vessels.c`) in `vessels_damage.c`; `vessel_prototype_price()`; `vedit new` takes the class beam armor |
| Duris arcs (fore 320-40, starboard 40-140, rear 140-220, port 220-320) | Done | `vessel_arc_for_relative_bearing()` behind `greyhawk_getarc()` |
| Refit rescaling: plating and reinforcement +20%, rigging +10% maximum speed (at least 1, at most 30), hold +25%; each 20% of the class price | Done | `do_shipupgrade()`, `vessel_upgrade_cost()`, `vessel_rigged_speed()` in `vessels_upgrades.c` |
| Damage resolution per fragment: sail hits (warship sails take 85%), spread across arcs, armor then internals, confirmed criticals past armor, deflection on gutted arcs, weapon damage (disabled at 1, destroyed at 100), knockdown (Reflex DC 15) | Done | `vessel_resolve_hit()`, `vessel_damage_hull()`, `vessel_damage_sail()`, `vessel_damage_weapon()`, `vessel_knockdown_aboard()` in `vessels_damage.c`; `ship_weapons.weapon_damage`; `shipfire`, NPC return fire, and hazards call them |
| Breach states: one breached arc immobile (airborne hulls half speed), two sinking on a timer (150-300 ticks owned, 2000-3000 unowned); a sinking hull cannot move, fire, or be repaired | Done | `vessel_breached_arcs()`, `vessel_update_condition()`, `vessel_begin_sinking()`, `vessel_damage_tick_one()` (combat tick); `vessel_max_speed()`; `sink_ticks` |
| Going down: half of each bulk cargo lot spills as salvage crates; `shipsalvage` hauls crates into a stopped hull's hold | Done | `vessel_spill_cargo()`, `vessel_salvage_crates()`, `do_shipsalvage()`; `vessel_stow_cargo()` shared with `plunder` |
| D6: `strikecolors`; capture and plunder only of disabled prizes; hostile boarding only at speed 3 or less or disabled | Done | `vessel_prize_disabled()`, `vessel_abandoned_at_sea()`, `do_strikecolors()` in `vessels_damage.c`; `do_claimship()`, `do_plunder()`, `can_attempt_boarding()` (L14) |
| Migration: prototype armor rescaled once by class (armor-scale flag), live hulls converted keeping their damage fractions (condition-model flag), weapon damage column; Phase 19 SQL with rollback and verifier | Done | `vessel_prototype_ensure_schema()` (`armor_scale`), `vessel_rescale_legacy_armor()`, `vessel_convert_legacy_condition()` from `vessel_db_load_runtime()` (`condition_model`); `vessels_phase19_schema.sql`, `_rollback.sql`, `verify_vessels_phase19.sql`; content packages at S3 scale with `armor_scale = 1`, and their provisioners apply Phase 19 first |
| Status display (structure, sail, rudder, breaches, sinking, colors, weapons), help in both places, `VESSEL_SYSTEM.md` | Planned |  |
| Unit tests, actual-character damage gate, existing gates, local CI | Planned |  |

Interpretations decided while planning S3:

- Weapon rows arrive in S4. Until then every mounted weapon resolves as one Duris ballista
  fragment: spread 10, sail hit 14%, hull/sail 100/50%, pierce 10% (critical threat 19-20,
  confirmed by a second roll against the same target number). Damage stays the slot's dice.
- The hit roll stays the S1 rule (`d20 + level / 2 + gunnery` against `10 + speed / 5`) until
  the S4 geometry DC.
- "NPC hulls" for the sink timer are unowned hulls (public ferries, merchants, hunters,
  derelicts, events).
- Wear and weather keep their absolute sail and hull points, so against the larger Duris sails and
  hulls they matter proportionally less.
- The balance duel harness keeps its own constants until S4 moves it to the D2 bounds.
- The salvage command is `shipsalvage`: `salvage` is the item-salvage craft command.
- Until S4 sells weapons and S5 prices repairs, `shiprepair` also mends damaged weapons, and
  restores a destroyed one while berthed in port.
- A sinking hull can be boarded and plundered but not captured.
- "Abandoned" means no conscious character (player or mobile, the pilot included) aboard other
  than the claimant; hired crew positions are abstract and do not defend.
- Found while testing: a hull shot from one side only cannot sink. Deflected hits reach only
  another arc's structure, never its armor, so only the facing arc is ever holed (Duris behaves
  the same). S4's duel harness and S6's NPC AI must maneuver to bring a second arc to bear.
- The stale "Running aground" help paragraph (grounding was removed in S2) is dropped with the
  S3 help rewrite.
- Migration: the ratio is the class beam armor over the old `vedit new` default (raft 2, boat 5,
  ship 20, warship 40, airship 15, submarine 25, transport 20, magical 20), capped at 229. A
  legacy hull's prototype armor is read back from its saved arc maximum (undoing the old +50%
  plating), so hulls without a prototype convert too. The old model left shot-out sections
  afloat and had no holes, so a converted arc keeps at least 1 structure: no hull comes back
  holed or sinking. Default ballistae, wages and insurance refunds in 3.3.10 belong to S4 and S5.
- The content packages write S3-scale armor with `armor_scale = 1`, so they fail loudly on a
  database without Phase 19 instead of being rescaled twice.
- The Phase 19 rollback returns prototype armor to the old scale (rounded) so older code does not
  run S3-strength prototypes; converted hulls keep their S3 values.

### Estimate

Working days of focused implementation per step, each including its tests, help in both places,
schema and rollback SQL, documentation and live gate. The basis is this repository's recent pace:
vessel Phases 14, 15 and 16 each landed as one commit on 2026-07-30 or 2026-08-02, and the
four-arm mechanic went from study (2026-09-12) to merge (2026-09-14). Each step below is two to
five times the size of one of those phases.

| Step | What drives the size | Days |
| -- | -- | -: |
| S1 Defect fixes | Eight small fixes, three schema changes, help updates | 2 |
| S2 Movement and pacing | Rewrites every mover; ferry soak and 500-vessel benchmark re-baselines | 3-4 |
| S3 Damage model | Arc profiles, breach and sink-timer state, salvage, capture rules | 2-3 |
| S4 Weapons and gunnery | Weapon tables, 16-slot migration, shipyard commands, hit model, duel harness | 4-5 |
| S5 Crew, repair and loss | Wreck registry and summon persistence, insurance retirement, crew experience | 4 |
| S6 NPC raiders and AI | Four tiers of content, basic and advanced AI, boarding and looting, ramming | 5-6 |
| S7 Rewards and economy | Renown and payouts, contraband and customs, damage-control feat | 2 |
| S8 Client data | MSDP tables and protocol tests | 1 |

Total: 23-27 working days in sequence, about five weeks. S7 and S8 do not depend on S6 and can
run beside it, which brings the calendar to about four to five weeks. The largest risks are the
S2 re-baselines, the S5 persistence migration and the S6 content. The Open player-data balance
and human beta gates follow and depend on player availability, not engineering time.

### Ablation record

- Dropped as Duris-specific or already covered: racewar ocean-PvP state and `signal` (PvP consent
  and `shiptalk` cover them), the Redis and flat-file backends, Duris ferries and autopilot
  (schedules and routes cover them), runtime identity references (generation-aware events cover
  them), the levistone (airships cover flight), delayed market prices (per-unit batch pricing
  already stops quick flips), ship coffers (the settlement path pays owners), CTF speed penalty,
  the Trader achievement, the Sailor's Tattoo, and the unique Cyric's Revenge and automatons
  quest content.
- Simplified: continuous crew skills become experience on the existing four positions and three
  tiers, chiefs are those positions, NPC raiders extend the hunter lifecycle instead of a new
  spawner, the hit model reuses Duris's geometry as a D20 DC instead of a new table, and volley
  flight time is omitted (Duris freezes the chance at firing, so resolving at once changes only
  the delay).
- Kept: everything that satisfies requirement 4.4 (range, bearing, arcs, reloads, armor sections,
  subsystem damage, repair, sinking, wrecks, boarding, capture, NPC doctrine, observable disabled
  subsystems), the Duris features added for parity (stamina, anchor, contraband, damage control,
  equipment), and the pacing change without which none of it can matter.
