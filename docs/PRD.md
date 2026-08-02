# LuminariMUD Vessel System - Product Requirements

**Version:** 3.0
**Last updated:** 2026-08-02
**Status:** Core development acceptance passes; campaign content, beta, and rollout remain
**Product owner:** Zusuk

This document is the durable product contract for the unified vessel system. It
defines why the system exists, the experience it must provide, its scope, and the
criteria for release. It intentionally does not contain session plans or a live
backlog.

Current behavior is documented in
[VESSEL_SYSTEM.md](systems/VESSEL_SYSTEM.md). Outstanding work is tracked only in
[VESSELS_TODO.md](project-management-zusuk/vessels/VESSELS_TODO.md). Completed
work is recorded in [CHANGELOG.md](CHANGELOG.md).

---

## 1. Product Vision

The ocean, sky, and depths are a living frontier: players buy, crew, sail, fight,
trade with, and lose ships that feel like characters of their own.

A new player should be able to work passage on an NPC ferry, save for a basic
raft, upgrade to a named brig with hired crew, run cargo past pirates through a
storm, fight a broadside duel, board the enemy ship, and tow home a prize. These
experiences must use LuminariMUD's existing D20 mechanics, DG scripts, wilderness
weather, regions, and builder content instead of forming an isolated minigame.

The unified implementation uses the Greyhawk vessel system as its foundation and
replaces the maintenance burden and inconsistent interfaces of the former CWG,
Outcast, and Greyhawk implementations.

## 2. Design Pillars

1. **Ships are characters.** A ship is named, owned, persistent, damageable,
   upgradeable, and meaningful to lose.
2. **The wilderness is the world.** Vessels use the same coordinates, terrain,
   elevation, bathymetry, weather, paths, and regions as every other wilderness
   system.
3. **Multiplayer is the best version.** Solo operation remains possible, but
   helm, lookout, gunner, repair, and boarding roles reward coordinated crews.
4. **Builders are first-class users.** Hulls, interiors, ports, routes,
   encounters, and tunable gameplay values are data-driven and scriptable.
5. **Operations are safe.** The system must be observable, recoverable,
   testable, and capable of being disabled without destabilizing the rest of the
   game.

## 3. Player and Staff Outcomes

### New player

A low-level character can afford passage on an NPC ferry and encounter vessel
gameplay during the first hour without staff assistance or prior system
knowledge.

### Captain and crew

A player or clan can acquire a named ship and divide meaningful work among a
helm operator, lookout, gunner, repair crew, cargo manager, and boarders. A solo
captain can hire NPC crew to cover essential roles.

### Merchant

A player can discover price differences, accept freight work, load persistent
cargo, choose a route, survive environmental and piracy risks, and earn a
bounded profit.

### Pirate or privateer

A player can hunt shipping, disable and board a target, plunder cargo, and face
geographic, legal, faction, and bounty consequences. Existing player-versus-player
consent rules apply to every hostile vessel action.

### Builder

A builder without C knowledge can author a hull prototype, compose its interior,
place a shipyard or port, create routes and encounters, and spawn a working
vessel through OLC and world data.

### Operator

Staff can inspect every live vessel, locate or repair one, monitor shared
wilderness-room use, diagnose subsystem behavior, and perform a rehearsed
rollback.

## 4. Functional Requirements

### 4.1 Navigation and environment

- Vessels navigate the shared wilderness coordinate system without a private
  map or track-only movement layer.
- Each hull class has explicit terrain, draft, altitude, depth, and speed
  capabilities.
- Surface vessels, airships, and submarines use the same wilderness column at
  a coordinate; altitude and depth remain anchored to the terrain below.
- Bathymetry governs draft, grounding, and crush depth.
- Wilderness weather governs speed, visibility, helm risk, and damage.
- Roads and rivers use the wilderness path system.
- Named seas, territorial waters, encounter waters, trade lanes, magical
  waters, and special travel areas use builder-authored wilderness regions.

### 4.2 Vessel lifecycle

- Ships can be created from builder-authored prototypes with generated,
  data-driven interiors.
- Ships can be bought, named, owned, transferred, permitted, upgraded,
  maintained, insured, damaged, sunk, salvaged, and captured.
- Ownership, interior state, cargo, crew, upgrades, and other durable state
  survive reboot and copyover.
- Deletion and loss paths must not leave orphaned database rows, interior
  rooms, fleet references, or player property.

### 4.3 Movement, automation, and transport

- Players can steer directly or use persistent waypoints, routes, schedules,
  autopilot, and NPC pilots.
- Ships can dock with ports and compatible vessels, and players can board and
  disembark safely.
- Land vehicles use a lightweight transport tier and can be loaded onto
  vessels within capacity and nesting limits.
- Movement preserves object, room, coordinate, and fleet-slot identity.

### 4.4 Naval combat

- Combat supports range, bearing, firing arcs, reloads, armor sections,
  subsystem damage, repair, sinking, wrecks, boarding, capture, and NPC combat
  doctrine.
- Combat uses existing D20 and PvP-consent systems.
- A disabled subsystem changes behavior in an observable way.
- Combat and boarding must clean up references and occupants safely when a
  vessel sinks or a fleet slot is reused.

### 4.5 Economy and living world

- Cargo has persistent weight and capacity constraints.
- Ports expose data-driven commodities, bounded local prices, and freight
  contracts.
- NPC ferries and merchant shipping use real routes and cargo.
- Piracy produces enforceable bounties and port consequences.
- Weather, encounters, derelicts, discoveries, depths, skies, rivers, and
  events make voyages differ without hardcoded private geography.
- Each vessel class must eventually offer at least one distinctive destination,
  hazard, or activity.

### 4.6 Builder, client, and operator support

- `vedit`, database rows, wilderness region tooling, OLC, and DG scripts expose
  content that builders need to vary.
- Player-facing commands have reachable help entries in the authoritative help
  database.
- Ship state is available through native MSDP without making a graphical
  client mandatory. Native MSDP is the release contract; a separate GMCP
  vessel package is not required for this release.
- Staff tooling exposes vessel state and shared wilderness-room pressure.
- Diagnostics are category-selectable and safe to use on a live server.

## 5. Wilderness Contract

The vessel system follows these invariants:

1. **One geography.** New environmental signals are added to the wilderness
   system first and then consumed by vessels. Vessel code does not create a
   second coordinate, terrain, weather, path, or region model.
2. **Regions belong to builders.** Geographic names, legal waters, encounter
   areas, trade lanes, and special environments are region data, not hardcoded
   coordinate rectangles.
3. **Dynamic rooms are shared.** Ships use the wilderness dynamic-room pool and
   must monitor pressure, release rooms when possible, and degrade safely before
   exhaustion.
4. **The Z axis is anchored.** Altitude and depth extend a wilderness coordinate;
   depth derives from bathymetry and altitude content derives from regions.
5. **Content is campaign-safe.** Core integration is campaign-neutral. Names,
   encounters, ports, and other setting-specific content live in campaign data.

The implementation-level mapping of these rules is maintained in
[VESSEL_SYSTEM.md](systems/VESSEL_SYSTEM.md#wilderness-integration-contract).

## 6. Quality Requirements

### Safety and correctness

- Validate indexes, VNUMs, pointers, array bounds, and allocation results before
  use.
- Avoid variable-length arrays and keep declarations at the top of blocks,
  following the repository's established GNU C23 style.
- Keep all tunable values in data or world files unless they are true engine
  constants.
- Apply existing PvP, authorization, and ownership checks at every entry point,
  not only in command handlers.

### Persistence

- Every schema change includes install, rollback, and verification components.
- Fresh databases auto-create the current schema.
- Copyover, reboot, sinking, transfer, deletion, and rollback paths are tested
  explicitly.

### Performance

- Support at most 500 concurrent vessels.
- Budget no more than 5 KB of base structure memory per vessel and about 3 MB
  for the maximum fleet before optional dynamic content.
- Keep the complete vessel subsystem within 25 ms per game tick at the
  500-vessel ceiling.
- Optional systems attach allocations only when needed.

### Verification

Every behavior-changing vessel update must:

1. Build without new `-Wall -Wextra` warnings.
2. Pass the production-linked root CuTest suite.
3. Add or update production-linked coverage for changed behavior.
4. Pass relevant Valgrind checks.
5. Complete the numbered dev-server regression workflow.
6. Update the behavior reference in the same change.

See [VESSEL_SYSTEM_TESTING.md](testing/VESSEL_SYSTEM_TESTING.md) and
[TESTING_GUIDE.md](guides/TESTING_GUIDE.md).

## 7. Scope Boundaries

The vessel system does not require:

- Real-time naval physics, wind vectors, or tacking simulation.
- A graphical or client-specific navigation experience.
- A second coordinate system.
- Campaign-specific ship content in core C.
- Ship-versus-fortress siege mechanics in the current release.
- Arbitrary player-built interiors; interiors remain template-composed.
- Cross-MUD vessel functionality.

## 8. Release Acceptance

The vessel system is ready for general production use only when all of the
following are demonstrated on the development server:

1. Each player and staff outcome in Section 3 works end-to-end without hidden
   staff intervention.
2. A new character can have a memorable ferry, event, or encounter experience
   within the first hour.
3. At least 70% of beta respondents rate ship combat "fun" or better.
4. NPC shipping completes a supervised sample within the one-hour validation
   ceiling, prices remain within bounds, and at least one player-discovered
   route is profitable.
5. A supervised stability gate, including setup, recovery checks, and cleanup,
   completes within one hour without a crash or observed runaway growth; the
   500-vessel tick budget is met, and all automated gates pass.
6. Every documented operational failure mode has a tested staff or command-line
   recovery procedure.
7. Schema migration and rollback are rehearsed against a production snapshot
   without data loss.
8. The vessel-system toggle is a real command-and-tick kill switch, production
   debug logging is disabled, and authoritative help entries are deployed.

## 9. Principal Risks

| Risk | Mitigation |
|------|------------|
| Gameplay scope expands without a releasable core | Preserve independent capability slices and cut optional events or cosmetics before safety, ownership, combat, or economy |
| Combat or economy balance changes repeatedly | Keep numbers data-driven and use duel, trade, soak, and beta simulations |
| PvP vessel actions bypass consent | Route every hostile action through the shared PvP gate and test each entry point |
| Shared wilderness rooms are exhausted | Monitor pool pressure, reclaim rooms, and test graceful degradation at fleet capacity |
| Tick work exceeds the game-loop budget | Measure the complete live subsystem at 500 vessels before rollout |
| Schema or lifecycle changes corrupt property | Provide rollback and verification SQL and test copyover, deletion, sinking, and transfer |
| Builders cannot ship content independently | Maintain OLC and data-driven workflows and run a timed builder usability test |

## 10. Delivery State

The core transport and gameplay layers are implemented: wilderness movement,
interiors, docking, autopilot, vehicles, builder prototypes, combat, ownership,
crew, upgrades, cargo, trade, freight, piracy, weather hazards, encounters,
operator tooling, and native MSDP ship state.

Implementation does not equal release acceptance. The local 30-step regression,
bounded ferry observation, complete 500-vessel performance gate, current
Memcheck, and development release-boundary checks pass. Campaign content,
human beta, production-snapshot rehearsal, balance, and staged rollout remain.
Consult [VESSELS_TODO.md](project-management-zusuk/vessels/VESSELS_TODO.md) for
the only current vessel backlog.
