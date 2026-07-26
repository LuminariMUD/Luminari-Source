# Changelog

## [Unreleased] - July 26, 2026

### Documentation - Zusuk workspace audit

Audited every document in `docs/project-management-zusuk/`, which is developer
scratch space rather than a home for official documentation. Each file was
classified by verifying its claims against the code, not by trusting its own
status headers.

#### Changed

- **Finished work with enduring value moved into the formal documentation tree:**
  - `current-casting-visuals.md` -> `docs/systems/CASTING_VISUALS_SYSTEM.md`
  - `client_capabilities_and_player_preferences.md` -> `docs/systems/CLIENT_CAPABILITIES_AND_PREFERENCES.md`
  - `MOUNT_AUDIT.md` -> `docs/systems/MOUNT_SYSTEM.md`
  - `char-rename-fixes.md` -> `docs/systems/CHARACTER_RENAME_SYSTEM.md`
  - `INTEGRATION_GUIDE.md` -> `docs/systems/INTERMUD3_GATEWAY_API.md`
  - `CIRCLEMUD_CLIENT_AUDIT.md` -> `docs/systems/INTERMUD3_SECURITY_AUDIT.md`
  - `casting-visuals-testing.md` -> `docs/testing/CASTING_VISUALS_TESTING.md`
  - `overview.md` -> `docs/guides/LUMINARI_OVERVIEW.md`
  - `PRODUCTION_DEPLOYMENT_STEPS.md` -> `docs/deployment/VESSEL_SCHEMA_DEPLOYMENT.md`,
    generalized from Phase 2 to all vessel phases with corrected
    `sql/components/` paths
- **Unfinished work consolidated** in `project-management-zusuk/ongoing-projects/`
  with a README stating each item's real status: AI conversation history (not
  started), SKORE phases 3-4, protocol security follow-ups (4 of 6 open), the
  event-system merge (not started), 76 outstanding CMake format warnings, and the
  player/staff idea backlog.
- **`docs/systems/CASTING_VISUALS_SYSTEM.md` brought up to date.** It documented
  the pre-enhancement system: no mention of schools, class styles, or
  environmental reactions, and no reference to `casting_visuals.c` at all, even
  though the seven-phase enhancement shipped 2025-11-26. It now documents all five
  delivered feature families, their entry points, target-type and message-slot
  model, and the fallback behavior when a message table entry is missing.
- **`docs/systems/INTERMUD3_GATEWAY_API.md` gained a scope note.** Its paths
  (`clients/python/i3_client.py`, `docs/API_REFERENCE.md`) refer to the external
  gateway project's repository, not this codebase, and never resolved here.
- **`docs/TECHNICAL_DOCUMENTATION_MASTER_INDEX.md`** updated for every move, with
  new entries for the protocol, mount, casting-visuals, rename, and I3 documents.

#### Removed

Three documents whose value was fully consumed, verified before deletion:

- `DEPLOYMENT_LOG.md` - a dated session log from 2025-11-20. Every build error it
  recorded is fixed (the `mariadb/mysql.h` include, the undeclared `REGION_*`
  constants), and its top recommendation - a CI pipeline to catch build errors -
  now exists as `.github/workflows/`.
- `casting-visuals/ideas-casting-visuals.md` and
  `casting-visuals/improving-casting-visuals.md` - all five ideas and all seven
  implementation phases shipped, confirmed present in `src/casting_visuals.c`.
  Their content was folded into the system documentation first.

#### Noted

- `docs/testing/vessel_test_results.md` describes eight test files
  (`test_vessels.c`, `test_vessel_coords.c`, `vessel_stress_test.c`,
  `test_runner.c`, and others) that **no longer exist** - they were standalone
  mirror suites, since replaced by the production-linked CuTest suite. Marked as a
  historical Phase 00 record rather than deleted, since it is not this project's
  document to remove.

### Vessel System - Phases 04 through 09 (gameplay layer)

Turns the vessel system from working transport infrastructure into a gameplay
system: ships can be authored by builders, bought and owned by players, crewed,
upgraded, insured, fought, sunk, plundered, and used to run cargo and freight
across the wilderness. Every environmental signal comes from the existing
wilderness system - no vessel-private geography, weather, or terrain.

Documentation: `docs/systems/VESSEL_SYSTEM.md` (behavior reference),
`docs/testing/VESSEL_SYSTEM_TESTING.md` (30-step manual regression script),
`docs/testing/VESSEL_BENCHMARKS.md` (memory attribution and test figures), and
`docs/project-management-zusuk/vessels/VESSEL_PRD_FINAL.md` (requirements,
wilderness contract, remaining work).

#### Added

- **Builder tooling** (`src/vessels_edit.c`) - `vedit` ship prototype editor
  (`list`/`new`/`show`/`set`/`delete`/`spawn`, LVL_BUILDER) backed by a new
  `ship_prototypes` table. `vedit spawn` produces a live boardable ship:
  allocates a fleet slot, applies per-prototype class/speed/armor, generates the
  interior, wires the object linkage, and persists immediately.
- **Data-driven ship interiors** - `ship_room_templates` rows override the
  compiled-in room template array at boot (`load_ship_room_templates_from_db()`),
  so builders change generated interiors without recompiling. Compiled-in
  templates remain as the MySQL-unavailable fallback.
- **Naval combat** (`src/vessels_combat.c`) - per-side armor absorption with
  spill into section internal structure and bleed-through from destroyed
  sections; damage bands (sound/battered/crippled/sinking); subsystem
  degradation (bow hits tear rigging and cut speed, stern hits foul the rudder);
  `vessel_sink()` evacuates all interior rooms into the water, converts the ship
  object to salvageable wreckage, and frees the fleet slot.
  - `shipfire <slot> <target>` with range gating, firing-arc gating, d20 +
    gunnery attack rolls against a speed-derived defense, damage dice from slot
    data, and reload timers on the heartbeat (`vessel_combat_tick()`).
  - `greyhawk_getarc()` implemented (it was previously declared but never
    defined) - computes the facing arc from heading-relative bearing.
  - NPC auto-defense doctrine: NPC-piloted ships track their last attacker and
    return fire with every bearing, loaded weapon.
  - `shiprepair` (at-sea repairs, stationary only) and `claimship` (capture from
    an uncontested bridge).
  - Groundings: sailing into water shallower than the hull's draft, measured
    against real wilderness bathymetry, halts the ship and damages the bow.
- **Ownership and shipyards** (`src/vessels_ownership.c`, `src/vessels_edit.c`) -
  player ownership persisted on `ship_interiors.owner`; helm permits (up to 10
  names) persisted in `ship_crew_roster`; `shipbrowse`/`shipbuy`/`shipchristen`/
  `shipdeed`/`shippermit`/`shiprevoke`/`shipcrew`.
- **Hired crew** (`src/vessels_crew.c`) - four positions (sailmaster, gunner,
  bosun, quartermaster) at three quality tiers, with signing costs and recurring
  wages; `shiphire`/`shipdismiss`/`shipwages`. Bonuses are written into the
  legacy `sailcrew`/`guncrew` fields so they feed existing systems: speed in
  movement, accuracy in gunnery, repair rate, and cargo capacity. Unpaid wages
  accrue and eventually cost you a crew member.
- **Refits, wear, and insurance** (`src/vessels_upgrades.c`) - `shipupgrade`
  installs plating (+50% armor), rigging (+5 speed), hold (+25% cargo), or
  reinforcement (+50% structure); `vessel_upkeep_tick()` wears armor and
  subsystems while under way; `shipinsure` buys coverage capped at hull value,
  paid out from `vessel_sink()`.
- **Bulk cargo and port trading** (`src/vessels_trade.c`) - `trade_commodities`
  (9 seeded goods, builder-editable) and `port_commodities` per-port supply
  seeded deterministically from port vnum so ports differ without randomness.
  `market`/`cargobuy`/`cargosell`/`cargomanifest`. Price scales with local
  scarcity, hard-clamped to +/-60% of base; buying drains local stock and selling
  floods it; ports buy at 85% of ask so same-port round trips lose money.
- **Freight contracts** (`src/vessels_contracts.c`) - per-port boards with TTL
  refresh; `contracts`/`contractaccept`/`contractdeliver`/`contractabandon`.
  Offers are generated from live commodity prices and real wilderness distance
  between dock rooms, and only to ports that actually trade.
- **Piracy and bounty** (`src/vessels_piracy.c`) - `plunder` transfers cargo
  from a cleared prize to an alongside raider; `bounty` and `marque`. Unlawful
  plunder accrues bounty, and `vessel_port_refuses()` is enforced at every
  port-service entry point, so a WANTED pirate cannot sell what they steal. A
  letter of marque legalizes prizes for a real day.
- **Weather hazards and encounters** (`src/vessels_hazards.c`) - storm severity
  bands read from the shared wilderness weather field (squall/storm/gale degrade
  rigging; a gale with no sailmaster aboard damages the hull); fog closes
  visibility, a posted lookout reopens it; submarine crush depth checked against
  real bathymetry; submerged submarines sheltered from surface weather;
  `seastate` reports water, depth, weather, visibility, and hull state.
  - Encounter engine: `vessel_encounters` rows key to `REGION_ENCOUNTER`
    wilderness regions via `get_enclosing_regions()`, filtered by depth band and
    hull class, with lookout warnings. Creatures spawn into the ship's
    wilderness room so they fight, flee, and take fire like anything else.
- **Operator tooling and client protocol** (`src/vessels_admin.c`) - `shiplist`
  (fleet overview plus wilderness dynamic room pool utilization, flagged past
  80%), `shipgoto <slot>`, `shipfix <slot>`. New MSDP variables pushed to anyone
  aboard a ship: `SHIP_NAME`, `SHIP_X`, `SHIP_Y`, `SHIP_Z`, `SHIP_HEADING`,
  `SHIP_SPEED`, `SHIP_HULL`, `SHIP_HULL_MAX`, `SHIP_STATUS`.
- **Debug instrumentation across the vessel and vehicle stack** - a master
  compile-time switch (`VESSEL_SYSTEM_DEBUG` in `src/vessels.h`) plus eight
  category switches (`VESSEL_DEBUG_CORE`/`MOVE`/`AUTO`/`DOCK`/`DB`,
  `VEHICLE_DEBUG_CORE`/`MOVE`/`XPORT`), so a single subsystem can be traced
  without noise from the rest. Macros cover plain logging, function entry/exit
  tracing, and state transitions; every line carries a greppable `[VESSEL_*]` or
  `[VEHICLE_*]` prefix.
  - Instrumentation completed across all nine files: `vessels_docking.c` (~30
    calls), `vehicles_transport.c` (all 8 functions), `vessels.c` (position
    updates, terrain checks, speed modifiers, blocked moves, room allocation),
    `vessels_autopilot.c` (start/stop/pause/resume, tick summary, travel steps),
    `vehicles.c` (state transitions, damage, terrain verdicts), plus
    `vessels_rooms.c`, `vessels_db.c`, `vehicles_commands.c`, and
    `transport_unified.c`.
  - Reference (categories, macro names, grep recipes) is documented in
    `docs/systems/VESSEL_SYSTEM.md` under Troubleshooting -> Debug Logging.
  - NOTE: the master switch currently ships at `1` for dev. It must be set to
    `0` before a production build - see the PRD's Remaining Work section.
- **Per-class cargo capacity** - `get_vessel_cargo_capacity()` data table plus
  `vessel_effective_cargo_capacity()`, which folds in the hold refit and the
  quartermaster's stowage bonus.
- **Help files** - `vedit.hlp`, `shipcombat.hlp`, `shipowner.hlp`,
  `shipcrew.hlp`, `shiptrade.hlp`, `shipfreight.hlp`, `shippiracy.hlp`,
  `seastate.hlp`, `shipadmin.hlp`. All 31 new commands have entries (verified by
  scripted audit).
- **SQL components** - schema, rollback, and verify scripts for each phase:
  `vessels_phase4_*`, `vessels_phase6_*`, `vessels_phase7_*`, `vessels_phase8_*`
  in `sql/components/`. The Phase 08 verify script flags any encounter row whose
  region is missing or is not actually a `REGION_ENCOUNTER` region.
- **Tests** - production-linked CuTest coverage in
  `unittests/CuTest/test_transport_production.c`: cargo capacity table, combat
  status bands, firing arcs including heading offset, armor absorption and
  sinking, an NPC-vs-NPC duel harness, helm permission matrix, shipyard pricing,
  crew costs and bonuses, upgrade effects, trade price bounds swept across the
  entire supply domain, cargo weight accounting, lookout/weather behavior, the
  PvP consent gate, and stale attacker-reference cleanup on sinking.
  Suite grew from 60 to 74 tests, all passing.

#### Changed

- **Helm access on owned ships** - `is_pilot()` now requires owner, helm permit,
  or immortal on an owned vessel. Unowned vessels (test hulls, unclaimed
  ferries) remain open to anyone.
- **Vessel tick** - `comm.c` now drives `vessel_combat_tick()`,
  `vessel_crew_wage_tick()`, `vessel_upkeep_tick()`,
  `vessel_trade_restock_tick()`, `vessel_weather_tick()`,
  `vessel_encounter_tick()`, and `vessel_msdp_tick()` alongside the existing
  autopilot tick.
- **Boot sequence** (`db.c`) - loads DB room templates and ensures the
  ownership, trade, contract, piracy, and encounter schemas.
- **Ship movement** - speed now credits the sailmaster's handling bonus (capped
  at the hull's maximum) and runs a grounding check after each move.
- **`NUM_VESSEL_TYPES`** moved from `vessels.c` to `vessels.h` next to the
  `vessel_class` enum so other modules can bounds-check against it.
- **Vessel documentation consolidated and relocated out of the developer
  workspace.** `docs/project-management-zusuk/` is scratch space, not a home for
  official documentation, so the enduring vessel docs moved into the formal tree:
  - `VESSEL_SYSTEM.md` -> `docs/systems/` (alongside COMBAT_SYSTEM.md,
    CLAN_SYSTEM.md, and the other system references), updated with every new
    command and subsystem. Its Debug Logging section was rewritten to document
    the actual macro system - it previously showed hand-rolled `log()` calls that
    did not reflect the code.
  - `VESSEL_MANUAL_TEST.md` -> `docs/testing/VESSEL_SYSTEM_TESTING.md`, renamed
    to match the directory's convention (RESOURCE_SYSTEM_TESTING.md,
    WEATHER_INTEGRATION_TESTING.md) and rewritten as a 30-step numbered
    regression script.
  - `VESSEL_BENCHMARKS.md` -> `docs/testing/`, with the corrected memory
    attribution.
  - Only `VESSEL_PRD_FINAL.md` remains in the workspace folder, which is the
    appropriate home for in-flight planning.
  - Two working documents were retired entirely once their content landed in
    permanent homes: `VESSEL_CHECKLIST.md` and `todo.md` (the debug logging
    tracker) - completed work to this changelog, outstanding work to the PRD's
    Remaining Work section.
  - Inbound links updated in TECHNICAL_DOCUMENTATION_MASTER_INDEX.md,
    adr/0001-unified-vessel-system.md, and CONSIDERATIONS.md.

#### Fixed

- **Cargo capacity was never enforced** - `check_vessel_vehicle_capacity()` had
  a stubbed weight check, so `loadvehicle` accepted any load. It now totals
  loaded vehicle weight against the vessel's effective capacity.
- **23 stale "TODO Session 02/03" comments** in `vessels_autopilot.c` sat above
  working code and misrepresented the file's state. Resolved: routes now get
  unique session-local ids (negative, so they cannot collide with database
  AUTO_INCREMENT ids), and `route_save()`/`route_load()` genuinely round-trip
  through the `ship_routes`/`ship_waypoints` tables with idempotent waypoint
  replacement. Vessel sources are now TODO-free.
- **Boarding gaps** - `setup_boarding_defenses()` now repositions idle NPC crew
  to the entrance and bridge chokepoints; a failed hostile boarding drops the
  character into the actual wilderness water room with a d20 + Athletics swim
  check following the `movement_validation.c` convention; the boarding roll now
  factors in Athletics rather than level alone.
- **Format-string risk in room generation** - builder-authored template strings
  were passed to `snprintf` as format strings. Replaced with an explicit
  single-substitution helper that treats them as literal text.
- **Damage could not sink an evenly-matched ship** - damage to a destroyed hull
  section was absorbed instead of bleeding through to the rest of the hull, so
  two comparable ships could pound each other indefinitely. Caught by the
  NPC-vs-NPC duel harness.
- **`vessel_sink()` leaked** the autopilot and schedule allocations attached to
  the ship. Also caught by the duel harness.
- **Vessel combat bypassed the MUD's PvP consent system** (the most serious
  defect found). `pk_allowed` is enabled in this installation, but `pvp_ok()`
  (`src/utils.c`) additionally requires *both* players to have `PRF_PVP` set,
  arena excepted - and no vessel code called it. Consequences, all now fixed via
  a new `vessel_pvp_permitted()` gate that resolves a ship's owner and routes
  them through `pvp_ok()`:
  - `shipfire` could sink a non-consenting player's ship, drowning her crew and
    destroying her cargo.
  - `plunder` could steal a non-consenting player's cargo.
  - `claimship` could seize a moored ship with no combat whatsoever - board a
    ship, walk to the bridge, take it while the owner was logged off.
  - `do_board_hostile` (pre-existing, Phase 02) forced combat via `set_fighting()`
    on every player in the boarded room; `set_fighting()` does not gate either.
    Defenders are now checked individually, so a passenger who has not enabled
    PVP is not dragged into a fight for standing on deck.
  - Unowned hulls (test vessels, unclaimed NPC ferries) remain fair game, staff
    can always act, and an owner who is not logged in cannot consent - so their
    ship is protected while they are away.
- **Sunk ships left stale attacker references** - `last_attacker` holds a fleet
  slot index, and `vessel_sink()` frees the slot for reuse without clearing other
  ships' references to it. The next hull created in that slot inherited the
  grudge and would be fired on unprovoked by NPC return fire. `vessel_sink()` now
  clears the index fleet-wide.
- **Memory budget documentation was wrong** - the benchmarks doc and the PRD
  recorded 1016 bytes per ship with a 2KB cap. The struct actually measures 4744
  bytes, and was already ~4400 before this work; Phases 04-09 added roughly 340
  bytes. The dominant cost is legacy (`desc[256]` inside each of ten equipment
  slots). At 500 ships the fleet costs 2.3 MB, which is negligible, so the
  documented budget was corrected to 5KB/ship rather than restructuring working
  Greyhawk display code. Full component attribution is in the benchmarks doc.

#### Technical Details

- **New files**: `vessels_admin.c`, `vessels_combat.c`, `vessels_contracts.c`,
  `vessels_crew.c`, `vessels_edit.c`, `vessels_hazards.c`,
  `vessels_ownership.c`, `vessels_piracy.c`, `vessels_trade.c`,
  `vessels_upgrades.c` - all registered in both `Makefile.am` and
  `CMakeLists.txt`.
- **New tables**: `ship_prototypes`, `trade_commodities`, `port_commodities`,
  `freight_contracts`, `vessel_bounties`, `vessel_encounters`. Extended
  `ship_interiors` with `owner`, `upgrades`, `insured_for`, `wages_owed`
  (auto-migrated at boot). Activated the previously unused `ship_crew_roster`
  and `ship_cargo_manifest` tables.
- **Wilderness integration** (the design constraint throughout): groundings and
  crush depth read `get_modified_elevation()` against `wild_waterline`; weather
  reads the shared `get_weather(x, y)` field, so a storm at sea is the same
  storm a coastal walker experiences; encounters key to wilderness regions via
  `get_enclosing_regions()`; freight distances derive from dock room
  coordinates. No new coordinate space, terrain table, or weather source was
  introduced. `shiplist` monitors the shared dynamic room pool, which vessels
  borrow from rather than own.
- **Quality gates**: build clean with no new warnings under `-Wall -Wextra`;
  74/74 production-linked tests pass; valgrind reports 0 definitely/indirectly/
  possibly lost bytes and 0 errors; the whole system remains behind the existing
  cedit vessel-system toggle.
- **Verification status**: all of the above is code-complete and test-verified,
  but has not been exercised on a running server. Live-server verification
  (harbor content, soak testing, tick budget measurement, player beta) remains
  outstanding - see the Remaining Work section of the PRD.

# [Unreleased] - October 10, 2025

### Minimal World Bootstrap & Database Hardening

#### Added
- **Starter area documentation** (`docs/world/STARTER_AREA.md`) covering the minimal three-room loop aligned with start VNUMs.
- **`pet_data` schema creation** in the bootstrap initializer so fresh databases persist companion stats.

#### Changed
- Replaced the stub world definition with a sorted, four-room minimal world (`lib/world/(minimal|wld)/0.wld`) including a defined `#0` fallback.
- Widened the minimal zone range to `0–3099` so start rooms resolve cleanly on boot.
- MSDP room updates now guard against invalid room indices to avoid formatting crashes during login/movement.

#### Fixed
- Movement and MSDP segfaults caused by empty start rooms and invalid wilderness lookups.
- Repeated pet load/save SQL warnings by provisioning the missing `pet_data` table during setup.

## [Unreleased] - August 28, 2025

### Deployment System - Complete Overhaul (100% Complete)

#### Added
- **Simple setup script** (`scripts/simple_setup.sh`) - Zero-interaction deployment in under 2 minutes
  - Automatic configuration file setup from examples
  - Build system execution with error handling
  - Symlink creation for world/, text/, and etc/ directories
  - World file initialization with proper index naming
  - Text file creation for all required game files
  - Directory structure setup for player files

#### Fixed
- **Deploy script path navigation** - Script now correctly navigates from scripts/ to project root
- **World file copying bugs** - Fixed incorrect wildcard usage that copied wrong index files
- **Index file naming** - Files are now properly renamed from `index.xxx` to `index`
- **Symlink creation** - Automatically creates required symlinks (world, text, etc)
- **HLQ directory** - Added missing Homeland Quest directory and index
- **Text file initialization** - All required text files now created automatically

#### Changed
- **Deployment workflow** - Simplified from complex manual process to single script execution
- **Error handling** - Graceful MySQL bypass when database not configured
- **Documentation** - Updated all deployment guides with working instructions

## [Unreleased] - August 26, 2025

### Intermud3 Integration - Complete Repair and Enhancement (100% Complete)

#### Added
- **Complete Intermud3 client implementation** - Full thread-safe inter-MUD communication system
  - `src/systems/intermud3/i3_client.c` - Core threaded client with event queuing (901 lines)
  - `src/systems/intermud3/i3_client.h` - Complete API definitions and data structures (215 lines)
  - `src/systems/intermud3/i3_commands.c` - All player and admin commands implemented (602 lines)
- **Thread-safe architecture** - Producer-consumer event queuing between I3 thread and main game thread
- **Complete command set** - All inter-MUD communication features:
  - `i3tell <user>@<mud> <message>` - Send tells across MUD network
  - `i3chat [channel] <message>` - Multi-MUD channel communication
  - `i3who <mud>` - Query remote MUD player lists
  - `i3finger <user>@<mud>` - Get remote player information
  - `i3locate <user>` - Search for users across network
  - `i3mudlist` - List all connected MUDs on network
  - `i3channels list|join|leave [channel]` - Channel subscription management
  - `i3config` - Toggle I3 features on/off
  - `i3admin status|stats|reconnect|reload|save` - Administrative functions
- **JSON-RPC 2.0 protocol compliance** - Full implementation of I3 Gateway protocol
- **Configuration system** - File-based configuration in `lib/i3_config`
- **Event processing integration** - Seamless integration with main game heartbeat

#### Fixed
- **All critical security vulnerabilities** identified in CircleMUD client audit:
  - Buffer overflow vulnerabilities in message processing
  - Use-after-free and memory corruption issues  
  - Format string vulnerabilities in logging
  - Threading safety violations and race conditions
  - Resource leaks in socket and memory management
- **Complete protocol implementation** - All previously stub functions now fully implemented:
  - `i3_request_who()` - Remote player list queries
  - `i3_request_finger()` - Remote player information
  - `i3_request_locate()` - Cross-network user location
  - `i3_request_mudlist()` - Network MUD directory
  - `i3_join_channel()` / `i3_leave_channel()` - Channel management
  - `i3_send_emoteto()` / `i3_send_channel_emote()` - Emote support

#### Changed
- **Thread safety implementation**:
  - Proper mutex usage for all shared data structures
  - Event queuing prevents cross-thread character_list access
  - Safe message passing between I3 thread and main thread
- **Memory management**:
  - Proper JSON object cleanup with `json_object_put()`
  - Bounds checking on all string operations using `strncpy()`
  - Resource tracking and cleanup on shutdown
- **Error handling**:
  - Comprehensive input validation and sanitization
  - Graceful handling of connection failures with auto-reconnect
  - Proper error propagation and logging
- **Build system integration**:
  - Added to both `Makefile.am` and `CMakeLists.txt`
  - All commands registered in `interpreter.c`
  - Main loop integration in `comm.c` heartbeat function

#### Technical Details
- **Architecture**: Event-driven design with thread-safe producer-consumer queues
- **Dependencies**: json-c library for JSON-RPC protocol, pthread for threading
- **Performance**: Non-blocking I/O, efficient queue operations, minimal main thread impact
- **Security**: Input validation, bounds checking, safe string operations throughout
- **Configuration**: `lib/i3_config` with gateway host, port, API key, and feature toggles

#### Testing and Documentation
- **Integration status document** created at `docs/systems/narrative-weaver/INTERMUD3_INTEGRATION_STATUS.md`
- **Updated audit report** reflects successful remediation of all security issues
- **Production readiness**: Full compliance with I3 Gateway specifications
- **Testing instructions**: Comprehensive guide for verifying functionality

### Vessel System - Phase 2 Progress (80% Complete)

#### Added
- **Multi-room vessel interiors** - Ships now support 1-20 dynamically generated interior rooms
  - `vessels_rooms.c` - New file implementing room generation, templates, and connections (573 lines)
  - `vessels_docking.c` - New file implementing docking and boarding mechanics (412 lines)
- **Room template system** - 10 different room types with dynamic descriptions:
  - Bridge, Quarters, Cargo Hold, Engineering, Weapons, Medical, Mess Hall, Corridor, Airlock
- **Vessel-specific room generation** - Different vessel types get appropriate room layouts:
  - Warships: Multiple weapon rooms, engineering
  - Transports: Multiple cargo holds
  - Smaller vessels: Fewer, more compact layouts
- **Ship-to-ship docking mechanics**:
  - `dock` command - Dock with nearby vessels
  - `undock` command - Separate from docked vessels
  - Automatic gangway creation between docked ships
  - Safety checks for proximity and speed
- **Combat boarding system**:
  - `board_hostile` command - Attempt hostile boarding
  - Skill-based success checks
  - Consequences for failure (damage, falling into water)
- **Interior viewing commands**:
  - `look_outside` - View wilderness from ship interior
  - `ship_rooms` - List all rooms in current vessel
- **Room connection algorithm** - Smart hub-and-spoke layout with cross-connections

#### Changed
- **Build system integration**:
  - Updated Makefile.am to include vessels_rooms.c and vessels_docking.c
  - Updated CMakeLists.txt with new source files
- **C89/C90 compatibility fixes**:
  - Changed all `number()` calls to `rand_number()`
  - Fixed room coordinate fields to use `coords[0]` and `coords[1]`
  - Corrected `damage()` function calls with proper parameters
  - Fixed room_flags array handling with SET_BIT_AR
  - Moved all variable declarations to block start (no C99 loop declarations)

#### Fixed
- Compilation errors related to undefined functions
- Type mismatches for room coordinate fields
- Incorrect damage() function parameters
- C99 loop variable declarations
- Room flags assignment for array-based flags

#### Technical Details
- **Files modified**: vessels.h, Makefile.am, CMakeLists.txt
- **New dependencies**: spells.h (for TYPE_UNDEFINED)
- **Functions implemented**:
  - `generate_ship_interior()` - Creates room layout based on vessel type
  - `complete_docking()` - Establishes connections between ships
  - `do_dock()`, `do_undock()` - Docking commands
  - `do_board_hostile()` - Combat boarding command
  - `do_look_outside()` - View external wilderness

#### Remaining Work
- Database persistence for ship configurations
- Interior movement integration with ship navigation
- NPC crew management
- Cargo transfer system completion
- Performance optimization
- Unit test suite creation
- Integration testing with live gameplay
