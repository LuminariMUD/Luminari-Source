# Vessel System - Final PRD

**Product**: LuminariMUD Unified Transport System ("Vessels")
**Document**: Final Product Requirements - the road from working infrastructure to a
flagship gameplay system
**Version**: 2.0
**Date**: 2026-07-26
**Status**: IN PROGRESS - Phases 04-09 code-complete, live verification outstanding
**Owner**: Zusuk
**Prior art**: [VESSEL_SYSTEM.md](../../systems/VESSEL_SYSTEM.md), [VESSEL_BENCHMARKS.md](../../testing/VESSEL_BENCHMARKS.md),
[VESSEL_SYSTEM_TESTING.md](../../testing/VESSEL_SYSTEM_TESTING.md)

> **How to read this document.** Sections 1-4 are the enduring requirements and
> the wilderness contract. Section 5 is the phase plan, annotated with what is
> built. **Section 6 is the live work list** - everything still outstanding,
> which is where to look if you are picking this up. Completed work is recorded
> in [docs/CHANGELOG.md](../../CHANGELOG.md) (entry dated 2026-07-26) and
> described behaviorally in VESSEL_SYSTEM.md; this PRD no longer re-lists it.

**Resources**: [VESSEL_SYSTEM.md](../../systems/VESSEL_SYSTEM.md) (behavior reference) |
[VESSEL_BENCHMARKS.md](../../testing/VESSEL_BENCHMARKS.md) (memory, tests) |
[VESSEL_SYSTEM_TESTING.md](../../testing/VESSEL_SYSTEM_TESTING.md) (regression script) |
[CHANGELOG.md](../../CHANGELOG.md) (what shipped) |
[TESTING_GUIDE.md](../../guides/TESTING_GUIDE.md) |
[sql/components/](../../../sql/components/) |
vessel sources: [src/vessels*.c](../../../src/) |
wilderness: [wilderness.c](../../../src/wilderness.c), [wilderness.h](../../../src/wilderness.h)

---

## 1. Vision

When this PRD was written the vessel system was excellent *plumbing*: ships moved
on the wilderness grid, had generated interiors, docked, followed autopilot routes,
and persisted to MySQL - but it was not yet a *reason to log in*. Phases 04-09 have
since built the gameplay layer on top of it (Section 2).

The vision this document serves, unchanged:

> **The ocean, sky, and depths become a living frontier: players buy, crew, sail,
> fight, trade with, and lose ships that feel like characters of their own.**

"Mind-blowingly awesome" is defined concretely in Section 11 (Release Criteria), but
the spirit is: a new player should be able to work passage on an NPC ferry, save up
for a leaky raft, upgrade to a named brig with a hired crew, run cargo past pirates
through a storm, win a broadside duel, board the enemy ship, and tow home a prize -
all with mechanics that already exist in this codebase (D20 checks, DG scripts,
wilderness weather, zone content), not a parallel mini-game bolted on the side.

### Design pillars

1. **Ships are characters.** Named, owned, persistent, damageable, upgradeable,
   mournable when lost.
2. **The wilderness IS the ocean.** Vessels are not a parallel world; they are the
   way players experience the wilderness system's water, sky, and depths. Every
   voyage reads the same elevation, weather, sector, and region data as a walker
   on shore - weather, terrain, depth, altitude, and encounters make every voyage
   a set of decisions, not a travel timer.
3. **Multiplayer by default.** The best moments involve a crew: helm, lookout,
   gunner, boarder. Solo play works; group play shines.
4. **Builders are first-class users.** Everything shippable by OLC and DG scripts,
   nothing hardcoded that a builder will want to vary.
5. **Never break the MUD.** The cedit vessel-system toggle stays; every phase ships
   behind it, production-safe, with the existing test discipline (production-linked
   CuTest, valgrind-clean, CI gates).

---

## 2. Where We Are

### Foundation (Phases 01-03, complete before this PRD)

| Area | Evidence |
|------|----------|
| Wilderness movement, 8 vessel classes, terrain/speed rules | vessels.c |
| Multi-room interiors, room templates, dynamic wilderness rooms | vessels_rooms.c |
| Docking and boarding | vessels_docking.c |
| Autopilot, waypoints, routes, schedules, NPC pilots | vessels_autopilot.c |
| Vehicles (cart/wagon/mount/carriage), vehicle-in-vessel | vehicles*.c |
| Unified command layer (tenter/texit/tgo/tstatus) | transport_unified.c |
| MySQL persistence, cedit system toggle | vessels_db.c, db_init.c |

### Gameplay layer (Phases 04-09, code-complete 2026-07-26)

Ten new modules add builder tooling, naval combat, ownership, crew, refits,
bulk cargo, port trading, freight contracts, piracy, weather hazards, an
encounter engine, operator tooling, and MSDP ship variables - 31 new commands,
6 new tables, 74 passing production-linked tests, valgrind clean.

Full detail is in [docs/CHANGELOG.md](../../CHANGELOG.md) (entry dated
2026-07-26). Behavior reference is [VESSEL_SYSTEM.md](../../systems/VESSEL_SYSTEM.md).
Outstanding work is Section 6 of this document.

All Phase 04 debt from the original audit is retired: cargo capacity is
enforced, the 23 stale autopilot TODOs are resolved (vessel sources are
TODO-free), boarding gaps are closed, room templates are database-driven, and
the shallow wilderness integration is now the backbone described in Section 4.

### What is verified, and what is not

| Verified | Not yet verified |
|----------|------------------|
| Build clean, no new warnings (`-Wall -Wextra`) | Anything on a running server |
| 74/74 production-linked tests pass | Manual regression script (VESSEL_SYSTEM_TESTING.md) |
| Valgrind: 0 leaks, 0 errors | Tick budget at 500 ships with all subsystems live |
| Zero TODOs in vessel sources | Multi-day soak stability |
| Help entries for all 31 commands | Player-facing balance (TTK, wages, freight margins) |

Treat the gameplay layer as ready for a dev-server shakedown, not for
production. It remains behind the cedit vessel-system toggle.

### Explicit non-goals (for the final version)

- Real-time naval physics (wind vectors, tacking). Heading/speed abstraction stays.
- Client-side graphics. We enhance the ASCII tactical map and (optionally) publish
  MSDP variables; we do not build a map client.
- A second coordinate system. Everything stays on the wilderness grid.
- Campaign-specific ship content in core C. Campaign variation goes through
  `#ifdef CAMPAIGN_*` content tables and world files, per existing convention.

---

## 3. Personas and Player Stories

- **Deckhand Dana (new player, level 5)**: "I paid 10 gold for ferry passage, watched
  the coastline scroll by from the deck, and fought a stowaway rat. I want a boat."
- **Captain Corr (mid-level, small guild)**: "Our guild pooled money for a brig. I
  steer, Mira watches the tactical map, Tolan mans the ballista. We named it."
- **Merchant Mekk (economy player)**: "Salt is cheap in Ashenport and dear in
  Sanctus. My transport makes the run twice a week unless pirates or weather say
  otherwise. Insurance exists for a reason."
- **Pirate Vex (PvP/PvE raider)**: "I hunt the trade lane, disable rudders, board,
  and take cargo. Bounties on my head make port visits spicy."
- **Builder Bel (staff)**: "I can define a new vessel class, its interior layout,
  its shop listing, and its route schedule entirely from OLC and world files."
- **Imm Zusuk (operator)**: "I can see every ship, teleport to any, freeze the
  system with one cedit flag, and the nightly CI tells me nothing regressed."

---

## 4. Wilderness Integration - The Backbone

The wilderness system (wilderness.c/h, 2048x2048 grid, zone 10000) is where the
oceans, seas, skies, and depths live. The vessel system is, by design intent, the
primary way players will ever experience the water and air portions of that grid.
This section is the contract between the two systems; every phase in Section 5
builds on it.

### What is already wired (keep, do not duplicate)

| Integration | Mechanism | Used by |
|-------------|-----------|---------|
| Position = wilderness coordinate | ships occupy rooms from the dynamic pool via `get_or_allocate_wilderness_room()` -> `find_available_wilderness_room()` / `assign_wilderness_room()` (vessels.c:553-588) | all movement |
| Terrain gating | `world[room].sector_type` (produced by the wilderness generator from elevation/moisture/temperature) checked in `can_vessel_traverse_terrain()` | movement, speed |
| Weather | `get_weather(x, y)` per-coordinate Perlin weather | speed modifier, status display |

### Wilderness assets and their vessel use (status)

| Wilderness asset | Vessel use | Status |
|------------------|------------|--------|
| Bathymetry (`get_modified_elevation()` vs `wild_waterline`) | Draft enforced against real depth: shoal groundings, submarine crush depth | WIRED (Phase 05/08) |
| `REGION_ENCOUNTER` regions | Keying mechanism for the encounter engine, via `get_enclosing_regions()` | WIRED (Phase 08) |
| Weather field (`get_weather(x, y)`) | Storm severity, rigging damage, helm risk, fog visibility | WIRED (Phase 08) |
| `REGION_SECTOR_TRANSFORM` / `REGION_SECTOR` | Magical waters, becalmed zones, blessed lanes | NO WORK NEEDED - the generator already honours these regions for sector output; vessels read the resulting sector |
| Path system: roads (`PATH_ROAD`) | Vehicle road speed rules | WIRED (audited Phase 04 - `PATH_ROAD` stamps `SECT_ROAD_*`, which vehicles already map to `VTERRAIN_ROAD`) |
| Dynamic room pool | Ship positions borrow from the shared pool | WIRED + MONITORED (`shiplist` reports utilization, flags past 80%) |
| `REGION_GEOGRAPHIC` regions | Named seas and straits; territorial waters for piracy legality | OPEN (Section 6: piracy legality zones, named-sea announcements) |
| Path system: rivers (`PATH_RIVER`) | Navigable inland waterways for RAFT/BOAT, follow-the-river autopilot | OPEN (Section 6: river travel) |
| Wilderness map renderer (`WILD_MAP_SHAPE_*`) | Tactical map v2 with contact overlays; lookout view from real surrounding terrain | OPEN (Section 6: tactical map v2, lookout view v2) |
| `narrative_weaver` / `region_hints` | At-sea room descriptions from the same engines as land wilderness | OPEN (Section 6: immersion pass) |

### Ground rules

1. **One geography.** No vessel feature may introduce a coordinate space, terrain
   table, or weather source separate from wilderness. If the wilderness lacks a
   needed signal (e.g., ocean current), extend wilderness.c so walkers, spells,
   and future systems get it too - then consume it.
2. **Regions are builder territory.** Named seas, encounter waters, trade lanes,
   and territorial waters are authored as wilderness regions (existing MySQL
   region tooling), never hardcoded coordinate boxes in C.
3. **Respect the room pools.** The dynamic pool (vnums 1004000-1009999, 6,000
   rooms) is shared with every walker in the wilderness. 500 ships + wakes +
   encounters must not exhaust it: Phase 09 adds pool-utilization monitoring and
   graceful degradation (nearest-room reuse) before launch.
4. **Z-axis is vessel-owned but wilderness-anchored.** Altitude and depth extend
   the wilderness column at (x, y); depth limits derive from bathymetry at that
   coordinate, altitude content anchors to regions, so the third dimension stays
   consistent with the 2D map beneath it.
5. **Campaign safety applies here too.** Region content (sea names, encounter
   tables) is world data per campaign; the integration code is campaign-neutral.

---

## 5. The Plan: Phases 04-09

Numbering continues the established `phaseNN-sessionNN` convention (Phase 03 is
complete). Each phase is shippable and independently valuable; each ends with the
standard gate (Section 10). Sessions are sized to the same granularity as Phases
01-03 (one focused work session each).

Dependency graph (phases may overlap only where noted):

```
04 Foundation --> 05 Combat --> 06 Ownership & Crew --> 07 Economy --> 09 Launch
      |                                   |                  |            ^
      +-----------------------------------+--> 08 Living World ----------+
```

---

### Phase 04 - Foundation Hardening & Builder Tooling

**Status: COMPLETE** (debt retired, `vedit` shipped, templates database-driven). Outstanding: harbor sandbox content and the builder timing run - Section 6.

**Goal**: Retire all known debt, make builders self-sufficient, and finish the
observability work so Phases 05+ build on clean ground.

**Sessions**

1. **Debt sweep.** Implement the real cargo capacity check
   (vehicles_transport.c:149) backed by per-class capacity in
   `vessel_terrain_caps`/ship data; delete or resolve the 23 stale TODO comments in
   vessels_autopilot.c (implement what is genuinely missing - notably
   `route_id` generation and route DB round-trip verification); close the
   boarding TODOs at vessels_docking.c:651-652 and 842 minimally (defender
   positioning stub -> real placement; overboard -> move to water room with swim
   check via existing D20 utilities). Audit vehicle road/terrain rules against
   wilderness `path_data` roads so vehicles and vessels consume the same path
   source (Section 4 ground rule 1).
2. **Debug logging completion.** Instrument the remaining files (vessels.c
   movement, vessels_autopilot.c tick/waypoints, vehicles.c state, plus rooms,
   db, commands, unified). *Done, across all nine files. The master switch is
   currently 1 for dev rather than the planned 0 default - resetting it before
   production is tracked in Section 6.1.*
3. **Ship OLC part 1 - "vedit".** In-game editor (genolc pattern, like medit/oedit)
   for vessel prototypes: class, name, armor, speed caps, room count/layout choice,
   entrance/bridge rooms. Writes to a new `ship_prototypes` DB table (or world-file
   `.shp`-adjacent format - decide in-session; DB preferred since interiors already
   live there).
4. **Ship OLC part 2 - interior templates from data.** Move the hardcoded room
   templates (vessels_rooms.c:27-104) into `ship_room_templates` (table already
   exists with 19 rows) and let vedit compose layouts from them. Builders can add
   new templates without recompiling.
5. **Test zone build-out.** Expand zone 700 into a real harbor sandbox: two docks,
   three prototype ships (raft, ship, airship), an NPC ferry on a schedule between
   the docks, and DG-script triggers on interior rooms. Rewrite VESSEL_SYSTEM_TESTING.md
   as a numbered regression script covering board/disembark/sail/dock/autopilot/
   vehicle-load - every step with expected output.
6. **Docs & gate.** Update VESSEL_SYSTEM.md; run the full gate.

**Acceptance criteria**

- `grep -rn "TODO" src/vessels* src/vehicles* src/transport_unified*` returns only
  deliberate, ticketed items (target: zero).
- A builder with no C knowledge creates a new sailable vessel prototype and spawns
  one, using only vedit + existing OLC, in under 15 minutes.
- loadvehicle correctly refuses when capacity would be exceeded (unit-tested).
- NPC ferry demonstrably loops its schedule for 24h of uptime without desync.

---

### Phase 05 - Naval Combat

**Status: COMPLETE** (damage model, weapons, sinking, groundings, boarding v2, repair, capture, NPC doctrine). Outstanding: tactical map v2 - Section 6.

**Goal**: Make the COMBAT state real. Ship duels that use the existing per-side
armor model, positional tactics on the wilderness grid, and boarding as the climax.

**Sessions**

1. **Ship health & damage model.** Extend `greyhawk_ship_data`: hull points,
   per-section damage (uses existing farmor/rarmor/parmor/sarmor as mitigation),
   critical subsystems (rudder, sails/engine, weapon mounts) with disabled states.
   States: sound -> battered -> crippled -> sinking. Sinking ejects crew to water
   rooms (Phase 04 overboard work) and creates a salvageable wreck object.
   Groundings: sailing into water shallower than `min_water_depth` - checked
   against real bathymetry via `get_elevation_relative_sea_level()` - deals hull
   damage and strands the ship (Section 4).
2. **Weapons & firing.** Weapon mounts as data on the prototype (ballista, catapult,
   ram; MAGICAL class gets spell-mount). Commands: `fire <mount> <target>`,
   `aim`, `reload`. Range/arc from heading + relative bearing (the tactical map
   already computes bearings). D20 attack rolls using crew skill (Session 4).
3. **Combat loop & AI.** Ship combat ticks alongside the existing autopilot tick.
   NPC captains get simple doctrines (flee, broadside, close-and-board) selectable
   in vedit. Tactical map v2: rendered on the wilderness map renderer (real
   coastline, shoals, and region boundaries from sector data) with contact state
   (hostile/neutral), range rings, and damage readouts overlaid.
4. **Boarding actions v2.** Finish `perform_combat_boarding`: grapple check ->
   contested roll (new boarding skill; falls back to level per existing TODO note),
   defender NPC positioning at chokepoints (entrance room, bridge), capture
   condition = hold the bridge N rounds. Captured ships transfer control (hooks for
   Phase 06 ownership).
5. **Repair & aftermath.** `repair` command at sea (slow, material-consuming) and at
   dock (fast, gold). Wreck salvage. Death/respawn rules reviewed so ship combat is
   dangerous but not rage-quit lethal.
6. **Combat tests & balance pass.** Production-linked CuTest for damage math, arcs,
   state transitions; scripted NPC-vs-NPC duel harness for balance smoke tests.
   Docs + gate.

**Acceptance criteria**

- Two crewed ships can fight from first shot to sinking or boarding, PvE and
  (flag-gated) PvP, with no crash and no orphaned interior rooms.
- Every combat number (weapon damage, hull, DCs) lives in data, not literals.
- A disabled rudder observably changes handling (turn rate penalty).
- Zero-regression: all pre-existing 353 tests still pass.

---

### Phase 06 - Ownership, Crew & Shipyards

**Status: COMPLETE** (ownership, permits, shipyard, crew, refits, wear, insurance). Outstanding: offline insurance payout and deletion-orphan policy - Section 6.

**Goal**: Ships become possessions with lifecycle: buy, name, customize, crew,
maintain, insure, lose, replace.

**Sessions**

1. **Ownership model.** Owner fields already exist (`owner[64]`); promote to real
   ownership: player and clan ownership, permissions (helm, guests, gunners),
   `deed` item for transfer/sale. Persists via ship_interiors table extension.
2. **Shipyards & purchase.** Shipyard as a special shop room type: browse hulls
   (from vedit prototypes), buy, christen (name filter reuses existing profanity
   checks), take delivery at the adjacent dock. Buy-back/scrap pricing.
3. **Crew hiring & roles.** Hireable crew NPCs (extends existing `ship_crew_roster`
   table and NPC pilot code): pilot, gunner, lookout, bosun (repairs), quartermaster
   (cargo). Crew quality tiers affect the D20 rolls from Phase 05. Wages on a
   real-time cadence; unpaid crew walk at next dock.
4. **Customization & upgrades.** Upgrade slots per hull: armor plating, improved
   sails/engine (speed), extra cargo hold, extended quarters, figurehead/paint
   (cosmetic, shows in room descs and lookout view). Installed at shipyards.
5. **Maintenance, insurance, loss.** Slow wear on hull/rigging; dock fees;
   optional insurance contract (payout on verified sinking - uses combat/wreck
   events). Losing an uninsured ship should sting; recovery paths must exist
   (salvage your own wreck within N hours).
6. **Ownership tests & docs.** Permission-matrix tests, transfer edge cases,
   crew wage ticks under time-warp tests. Docs + gate.

**Acceptance criteria**

- A player can go from gold to sailing a named, crewed, upgraded ship with zero
  staff involvement.
- Ownership survives reboot, copyover, and player deletion (orphan policy defined
  and tested).
- Two-clan shared-ownership scenario passes the permission matrix tests.

---

### Phase 07 - Cargo, Trade & Ports

**Status: MOSTLY COMPLETE** (bulk cargo, port pricing, freight contracts, piracy and bounty). Outstanding: NPC merchant fleet, economy simulation, legality zones - Section 6.

**Goal**: An economic reason to sail. Freight with mass and value, port-to-port
price differentials, and NPC shipping lines that make the lanes feel alive.

**Sessions**

1. **Real cargo.** `ship_cargo_manifest` (table exists) becomes live: bulk goods as
   cargo lots with weight/volume against hull capacity; loading/unloading at docks
   via quartermaster or commands. Cargo survives persistence and is lootable via
   Phase 05 boarding.
2. **Port trade goods.** Per-port commodity tables (data-driven, builder-editable):
   base prices, supply/demand drift, restock ticks. Buy low here, sell high there;
   prices react mildly to player volume (bounded, exploit-tested).
3. **Trade routes & contracts.** Freight contract board at ports: "Deliver 20 crates
   of salted fish to Sanctus by Thursday" - generated from the port tables. Failure/
   piracy outcomes defined. Reuses schedule/route infra for contract routing hints.
4. **NPC shipping & ferries.** Promote the Phase 04 ferry into a fleet: scheduled
   NPC merchants sailing real cargo on real routes (killable, lootable, with
   consequences - faction/bounty), passenger ferries with fare collection.
5. **Piracy & bounty loop.** Attacking flagged shipping accrues bounty; bounty
   hunters (NPC warships) spawn against high-bounty players; ports may refuse
   docking. Letters of marque as the sanctioned variant. Legality is geographic:
   territorial waters, free seas, and pirate coves are builder-authored
   `REGION_GEOGRAPHIC` wilderness regions, not hardcoded zones (Section 4).
6. **Economy tests & docs.** Price-drift bounds tests, contract lifecycle tests,
   exploit pass (infinite-arbitrage, dupe-on-copyover). Docs + gate.

**Acceptance criteria**

- Merchant Mekk's story is playable end-to-end: contract, load, sail, risk, sell,
  profit - and the numbers are staff-tunable without recompile.
- Piracy has real risk/reward on both sides and cannot be trivially laundered.
- Economy sim run (scripted 1,000 trades) stays within configured price bounds.

---

### Phase 08 - The Living World (may overlap Phases 06-07)

**Status: PARTIAL** (weather hazards and the region-keyed encounter engine are in). Outstanding: derelicts, depths/sky content, river travel, immersion pass - Section 6.

**Goal**: Voyages become stories. Weather that matters, three travel layers with
distinct character, and encounter content that makes lookouts useful.

**Sessions**

1. **Weather & hazards.** Deepen the `get_weather(x, y)` integration beyond speed
   modifiers: storms are coherent areas of the existing wilderness weather field
   (so a walker on the coast sees the same storm the ship is fighting) dealing
   rigging damage and forcing helm checks; fog cuts tactical/contact range;
   favorable winds boost speed. Airships get altitude weather; submarines get
   crush depth enforced against bathymetry with consequences, not just blocks.
   If the field needs more signal (storm movement, currents), extend
   wilderness.c itself per Section 4 ground rule 1.
2. **Encounter engine.** Encounter tables keyed to `REGION_ENCOUNTER` wilderness
   regions for sea/air/depth (builder-editable via existing region tooling,
   DG-script hooked): sea serpents, ghost ships, floating derelicts, sirens,
   sargasso fields, aerial rocs, deep trenches. Lookout crew/players get advance
   warning rolls. Magical waters via `REGION_SECTOR_TRANSFORM` regions.
3. **Derelicts & discovery.** Explorable derelict interiors (generated from room
   templates + DG scripts), salvage, logs/maps that chain into further sites.
   Chartable discoveries: first-finder naming rights recorded and displayed.
4. **The depths & the sky.** Content pass making SUBMARINE and AIRSHIP genuinely
   different: underwater landmarks and hazards placed on the Z axis anchored to
   real bathymetry (trenches where `get_elevation_relative_sea_level()` is
   deepest), sky islands and high-altitude lanes anchored to regions; river
   travel for RAFT/BOAT along wilderness `path_data` rivers (follow-the-river
   autopilot); class-gated content so the hulls matter.
5. **Sensory & immersion pass.** Ambient messaging (creaking hull, wake, gull
   cries) per class/weather/speed; lookout view v2 composited from actual
   surrounding wilderness sectors via the map renderer; at-sea room descriptions
   through `narrative_weaver`/`region_hints` so the ocean gets the same dynamic
   description quality as land wilderness; ship-wide announce channel; named-sea
   entry announcements from `REGION_GEOGRAPHIC` boundaries.
6. **Living-world tests & docs.** Encounter table determinism tests, Z-axis
   boundary tests, message-spam throttling review. Docs + gate.

**Acceptance criteria**

- No two identical medium-length voyages: weather or encounter variance manifests
  observably in a scripted 10-voyage sample.
- Each of the 8 vessel classes has at least one thing only it can reach or do.
- Encounters are entirely data/DG-driven; zero encounter-specific C content.
- Zero new geography: every hazard, encounter area, named sea, and depth feature
  is expressed through wilderness regions, paths, bathymetry, or weather - no
  vessel-private coordinate tables (grep-auditable).

---

### Phase 09 - Events, Polish & Launch

**Status: PARTIAL** (operator tooling, MSDP variables, help audit, valgrind, memory audit done). Outstanding: events, soak, tick budget, beta, rollout - Section 6.

**Goal**: The showcase layer, protocol polish, ops hardening, and the production
rollout that earns the word "final".

**Sessions**

1. **Events.** Scheduled regattas (checkpoint racing on routes - infra exists),
   naval skirmish events (staff-triggered fleet battles), ghost-fleet world event.
   Leaderboards via existing score infrastructure.
2. **Protocol & UX polish.** MSDP/GMCP variables for ship state (position, heading,
   speed, hull, contacts) so modern clients can render gauges; color/layout pass on
   tactical map and all vessel output; command ergonomics review (aliases,
   abbreviation conflicts, help files for every command - audit against
   interpreter.c registrations).
3. **Admin & ops tooling.** `shiplist` overview (all ships, state, owner, position),
   admin teleport-to-ship, force-dock/force-repair, per-subsystem debug toggles at
   runtime (promote compile-time VESSEL_DEBUG_* to runtime where cheap);
   wilderness dynamic-room-pool utilization monitoring with graceful degradation
   before exhaustion (Section 4 ground rule 3); ops runbook update in
   VESSEL_SYSTEM.md.
4. **Performance & soak.** Re-run VESSEL_BENCHMARKS.md suite at final scope: 500
   ships with combat + encounters + economy ticks; 72-hour soak on dev with NPC
   fleets active; valgrind full pass; fix regressions to stay within a 25ms tick
   budget at maximum load.
5. **Balance & beta.** Structured player beta on dev port using the Section 11
   scorecard; balance iteration on combat TTK, wages, freight margins.
6. **Launch.** Production migration scripts + rollback verified (extends
   vessels_phase2 SQL component pattern); staged rollout behind the cedit toggle
   (staff -> beta cohort -> all); announcement content; postmortem doc; final
   VESSEL_SYSTEM.md revision declared 3.0.

**Acceptance criteria**

- Full benchmark + soak pass at final feature scope with zero leaks and tick
  budget met.
- Every player-facing command has a help entry (scripted audit passes).
- Rollback rehearsal performed on a production snapshot without data loss.

---

## 6. Remaining Work

The live work list. Everything here is outstanding as of 2026-07-26; completed
work lives in [docs/CHANGELOG.md](../../CHANGELOG.md). Grouped by what unblocks
what, not by phase - several items were deferred precisely because they depend
on each other.

### 6.1 Blocked on a live server (do these first)

Nothing below can be finished from source alone. They are the gate between
"code-complete" and "trustworthy".

- **Run the manual regression script.** [VESSEL_SYSTEM_TESTING.md](../../testing/VESSEL_SYSTEM_TESTING.md),
  30 steps, sections A-G. This is the single highest-value next action: it
  exercises boarding, sailing, autopilot, vedit, the vehicle loop, and docking
  against a real world. Nothing in the gameplay layer has run in a game yet.
- **Build the harbor sandbox** (was Phase 04 S5): expand zone 700 into two docks,
  three prototype ships, and a persistent NPC ferry on a schedule between them,
  with DG triggers on interior rooms. Several deferred items below need this
  content to exist before they can be built or tested.
- **Builder usability run**: time a builder with no C knowledge creating and
  spawning a sailable vessel using only `vedit`. Acceptance target is under 15
  minutes.
- **NPC ferry 24-hour loop**: confirm a scheduled ferry runs a full day without
  route or coordinate desync.
- **Tick budget measurement**: 500 ships with combat, encounters, economy, wear,
  and MSDP ticks all active, against the 25ms budget in Section 8. The per-tick
  cost of the new subsystems has never been measured on a running server.
- **72-hour soak** on dev with NPC fleets active: zero crashes, zero leaks, tick
  budget held.
- **Copyover and reboot survival**: mid-voyage, mid-combat, and cargo-laden.
  Persistence is implemented and reloads at boot, but the copyover paths have not
  been exercised.
- **Set `VESSEL_SYSTEM_DEBUG` to 0 before any production build.** It ships at
  `1` in `src/vessels.h` for dev work, which logs every ship movement, terrain
  check, and speed calculation - useful on dev, a syslog flood in production.
  This is a hard gate on the Section 6.5 rollout step, not a nice-to-have.
- **Load the help entries into the database.** Help content for all 31 commands is
  in `lib/text/help/help.hlp` and loads at boot (verified: keyword count 3260 ->
  3324), so in-game lookups work. But `lib/text/help/*` is gitignored
  (`.gitignore:322`) because the database is the authoritative store, which means
  the `help.hlp` edit is local-only and will not reach another machine or
  production. Apply `sql/components/help_vessel_entries.sql` to load the same 26
  entries into `help_entries`/`help_keywords` (idempotent; validated in a
  rolled-back transaction). Equivalent to running `hedit import` in-game.
  - The migration also covers 17 pre-existing vessel/vehicle topics (autopilot,
    waypoints, routes, schedules, vehicle commands, NPC pilots) that had never
    been reachable because their standalone `.hlp` files were not listed in
    `lib/text/help/index`.
  - Two of those orphan files, `autopilot.hlp` and `schedule.hlp`, are tracked in
    git and now duplicate content that lives in `help.hlp`. Worth deciding whether
    to delete them so the two copies cannot drift.

### 6.2 Content and gameplay depth

- **NPC merchant fleet and ferries** (Phase 07 S4): scheduled NPC merchants
  sailing real cargo on real routes - killable, lootable, with faction and bounty
  consequences - plus passenger ferries that collect fares. Needs the harbor
  sandbox (6.1) first, since the ferries need somewhere to sail between.
- **NPC bounty-hunter warships** (Phase 07 S5): navy hulls that spawn against
  HUNTED players. Deferred because it wants the encounter/spawn engine as its
  delivery mechanism now that one exists.
- **Piracy legality zones** (Phase 07 S5): territorial waters, free seas, and
  pirate coves authored as `REGION_GEOGRAPHIC` wilderness regions, so where you
  raid matters as much as whom. Currently bounty is global.
- **Derelicts and discovery** (Phase 08 S3): explorable derelict interiors
  generated from room templates plus DG scripts, salvage, logs and maps that
  chain into further sites, first-finder naming rights.
- **The depths and the sky** (Phase 08 S4): bathymetry-anchored trenches, sky
  islands and high-altitude lanes, and river travel for RAFT/BOAT along
  `path_data` rivers (follow-the-river autopilot). The acceptance criterion that
  each of the 8 hull classes can reach or do something unique is not yet met.
- **Events** (Phase 09 S1): regattas on checkpoint routes, staff-triggered fleet
  skirmishes, a ghost-fleet world event, leaderboards.

### 6.3 Polish and immersion

- **Tactical map v2** (Phase 05 S3): render on the wilderness map renderer so the
  map shows real coastline, shoals, and region boundaries, with contact state,
  range rings, and damage readouts overlaid. Currently the tactical display is
  the legacy Greyhawk grid.
- **Lookout view v2 and sensory pass** (Phase 08 S5): composite the view from
  actual surrounding wilderness sectors; at-sea room descriptions through
  `narrative_weaver`/`region_hints`; ambient messaging per class, weather, and
  speed; named-sea entry announcements from `REGION_GEOGRAPHIC` boundaries;
  ship-wide captain's announce channel. Include a message-throttling review.
- **Boarding polish** (Phase 05 S4): grapple step and contested-roll refinement;
  a dedicated boarding skill rather than the current level + Athletics blend.
- **Cosmetic customization** (Phase 06 S4): figurehead and paint options that
  show in room descriptions and the lookout view. Explicitly a Could-have.
- **Runtime debug toggles** (Phase 09 S3): promote the compile-time
  `VESSEL_SYSTEM_DEBUG` master and eight category switches
  (`VESSEL_DEBUG_CORE`/`MOVE`/`AUTO`/`DOCK`/`DB`,
  `VEHICLE_DEBUG_CORE`/`MOVE`/`XPORT`) to runtime where cheap, so a live
  problem can be traced without a rebuild and restart. The instrumentation
  itself is complete across all nine vessel and vehicle files; only the
  switching mechanism is compile-time. See VESSEL_SYSTEM.md
  (Troubleshooting -> Debug Logging) for the category reference.

### 6.4 Correctness and edge cases

- **Offline-owner insurance payout** (Phase 06 S5): a sinking currently pays a
  logged-in owner immediately and otherwise writes a log line for staff
  reconciliation. Should deliver by mail instead.
- **Player-deletion orphan policy** (Phase 06 S1): decide and implement what
  happens to a ship whose owner is deleted (revert to unowned, transfer to clan,
  scuttle), then test it.
- **Dock fees** (Phase 06 S5): deferred into the Phase 07 economy and not yet
  built; mooring is currently free.
- **PvP consent** - RESOLVED. All vessel aggression (`shipfire`, `plunder`,
  `claimship`, hostile boarding, and per-defender boarding combat) now routes
  through `vessel_pvp_permitted()` / `pvp_ok()`. Remaining nuance worth a look
  during the live pass: an owner who logs off mid-battle currently makes their
  ship untouchable, which is the safe failure but could be exploited to escape a
  losing fight. A grace period keyed on recent combat would close that.
- **`ship_weapons` table** (Phase 05): weapon mounts currently live in the
  in-memory `slot[]` array seeded at spawn, so player-installed armament does not
  survive a reboot. Needs a table plus rollback and verify scripts.

### 6.5 Verification and balance

- **Scripted 1,000-trade economy simulation** (Phase 07 S6): confirm prices stay
  inside their bounds under sustained volume and that no route pays without
  limit. The per-call price bound is unit-tested across the whole supply domain;
  the aggregate behavior over time is not.
- **Encounter determinism and Z-axis boundary tests** (Phase 08 S6).
- **Balance pass** (Phase 09 S5): combat time-to-kill, crew wages, freight
  margins, and refit costs are all first-draft numbers chosen to be plausible,
  not tuned against play. All of them are data, so tuning needs no recompile.
- **Player beta** against the Section 11 release scorecard (below).
- **Staged production rollout** (Phase 09 S6): rehearse migration and rollback on
  a production snapshot, then roll out behind the cedit toggle (staff, then a
  beta cohort, then everyone), with announcement content and a postmortem.
  Pre-flight: `VESSEL_SYSTEM_DEBUG` set to 0 and the help migration applied
  (both Section 6.1). Only after this does VESSEL_SYSTEM.md become 3.0.

### 6.6 Decisions still open

Section 13 carries the full list with recommendations. Resolved so far:

| Q | Question | Resolution |
|---|----------|------------|
| Q2 | Clan ownership model | Owner plus permission list, as recommended. Clan shares remain a Could-have. |
| Q3 | Soft vs static port prices | Soft, with hard bounds (+/-60%), as recommended. |
| Q6 | Storm cells and currents: extend wilderness or layer vessel-side | Extended nothing - the existing weather field carried enough signal. Weather stays shared. |
| Q1 | PvP ship combat gating | Mis-framed; the MUD's existing `pvp_ok()` consent model is now honoured by all vessel aggression. No launch decision needed. |

Still to decide: **Q4** (shared vs instanced encounters - currently shared,
matching the recommendation, but untested with multiple ships in one region) and
**Q5** (MSDP only vs MSDP plus GMCP - MSDP variables are in; GMCP was not
audited).

---

## 7. Feature Requirements Summary (MoSCoW)

**Must have (definition of "final")**: Phase 04 debt-zero + vedit; Phase 05 combat
loop incl. boarding v2; Phase 06 ownership/shipyards/crew; Phase 07 cargo +
port trading + NPC shipping; Phase 08 weather hazards + encounter engine; Phase 09
ops tooling, benchmarks, staged launch.

**Should have**: insurance, bounty/marque loop, derelict discovery chains,
MSDP/GMCP variables, regattas.

**Could have**: cosmetic upgrades, first-finder naming rights, ghost-fleet world
event, runtime debug toggles.

**Won't have (this cycle)**: naval physics, client graphics, ship-vs-fortress
siege, player-built custom interiors (template-composed only), cross-MUD anything.

---

## 8. Technical Ground Rules

- **Language/style**: GNU C23, existing conventions (CLAUDE.md): 2-space Allman,
  declarations at top of block, no VLAs, `/* */` comments, safe string functions.
- **Data over code**: every tunable in DB tables or world files; every new table
  gets schema + rollback + verify SQL components like vessels_phase2.
- **VNUMs**: interiors stay in 70000-79999; any new reserved ranges documented in
  vnums.example.h, never hardcoded.
- **Memory**: ship struct growth budgeted - cap 5KB per ship (measured 4744 bytes
  as of 2026-07-26); 500-ship ceiling retained (~2.3MB fleet); new per-ship
  allocations optional-attach like autopilot_data. NOTE: the original 2KB/1016-byte
  figure in this PRD was inherited from stale Phase 03 docs and was already wrong
  before Phase 04 began - see VESSEL_BENCHMARKS.md for the component attribution.
  The dominant cost is legacy (`desc[256]` inside each of ten equipment slots),
  not new work.
- **Ticks**: combat/encounter/economy processing joins existing tick scheduling;
  total vessel subsystem budget 25ms at 500 ships (benchmarked, CI-guarded where
  feasible).
- **Campaign safety**: all content tables campaign-aware; core C stays
  campaign-neutral.
- **Toggle**: everything behind the cedit vessel-system enable; combat and PvP
  additionally flag-gated per zone.

## 9. Data & Persistence Plan

Tables, with actual naming as built. Each shipped phase has schema, rollback,
and verify scripts in [sql/components/](../../../sql/components/)
(`vessels_phase4_*`, `vessels_phase6_*`, `vessels_phase7_*`, `vessels_phase8_*`).
Every table auto-creates or auto-migrates at boot, so a fresh database works
without manual schema steps.

| Table | Purpose | Status |
|-------|---------|--------|
| `ship_prototypes` | vedit-authored hull definitions | BUILT (Phase 04) |
| `ship_interiors` (extended) | + `owner`, `upgrades`, `insured_for`, `wages_owed` | BUILT (Phase 06) |
| `ship_room_templates` (activated) | builder-editable interior room text | BUILT (Phase 04) |
| `ship_crew_roster` (activated) | helm permits (npc_vnum -1) and hired crew (npc_vnum <= -100) | BUILT (Phase 06) |
| `ship_cargo_manifest` (activated) | bulk cargo lots (cargo_room 0) | BUILT (Phase 07) |
| `trade_commodities` | goods, base prices, unit weights | BUILT (Phase 07) |
| `port_commodities` | per-port supply driving local prices | BUILT (Phase 07) |
| `freight_contracts` | contract board lifecycle | BUILT (Phase 07) |
| `vessel_bounties` | bounty and letters of marque | BUILT (Phase 07) |
| `vessel_encounters` | encounters keyed by wilderness `REGION_ENCOUNTER` vnum | BUILT (Phase 08) |
| `ship_weapons` | weapon mounts, ammo, state | OPEN (Section 6.4) - armament is currently in-memory only and does not survive reboot |
| `vessel_events` / leaderboards | regatta and event results | OPEN (Section 6.2) |

Copyover/reboot safety is an acceptance criterion in every phase that touches
state. Persistence is implemented and reloads at boot, but the explicit copyover
tests (mid-combat, mid-voyage, cargo-laden) have not been run - see Section 6.1.

## 10. Quality Gates (every phase, non-negotiable)

1. `make -j$(nproc) && make test && make install` green; no new warnings
   (-Wall -Wextra).
2. New logic covered by production-linked CuTest suites (added to Makefile.am +
   CMakeLists.txt per TESTING guide); total suite stays 100% pass.
3. Valgrind clean on the touched suites.
4. Manual regression script (VESSEL_SYSTEM_TESTING.md, kept current) executed on dev.
5. VESSEL_SYSTEM.md updated same-session as behavior changes.
6. No production deploys mid-phase; production only at Phase 09 staged rollout.

## 11. Release Criteria - the "Awesome" Scorecard

None of these are met yet - all six require a running server and, for three of
them, real players. This is the final sign-off gate, tracked here rather than in
a separate checklist.

The final version ships when all of these are demonstrably true on dev:

1. **The five personas' stories** (Section 3) are each playable end-to-end without
   staff intervention.
2. **A first-hour hook exists**: a brand-new character can have a memorable vessel
   experience (ferry ride + event or encounter) within 60 minutes of creation.
3. **A ship fight is a story**: post-beta survey of testers rates ship combat
   "fun" or better at >=70%.
4. **The economy breathes**: NPC shipping runs for a week unattended; prices stay
   in bounds; at least one player-discovered trade route is profitable.
5. **Nothing burned down**: 72h soak, zero crashes, zero leaks, tick budget met,
   all CI gates green.
6. **Ops can sleep**: every failure mode in the troubleshooting table has a
   command-line remedy that works.

## 12. Risks

| Risk | P | I | Mitigation |
|------|---|---|------------|
| Scope creep (this PRD is large) | High | High | Phases are independently shippable; cut from the tail (09 events) never the spine (04-07) |
| Combat balance churn | High | Med | All numbers in data; NPC-vs-NPC duel harness for fast iteration |
| Economy exploits | Med | High | Bounded price drift, exploit test session in Phase 07, audit logging on trades |
| Tick budget blown at full scope | Med | High | Budget benchmarked per phase, not only at the end (Phase 09 session 4 is a re-check, not first check) |
| PvP griefing via piracy | Med | Med | Zone flags, bounty consequences, insurance floor for victims |
| Wilderness dynamic room pool exhaustion (6,000 rooms shared with walkers) | Med | High | Pool monitoring + nearest-room reuse degradation (Phase 09); soak test includes 500 ships + player load |
| Struct/schema migrations corrupt live data | Low | Critical | Rollback scripts per table, copyover tests per phase, staged rollout |
| Builder adoption fails (vedit unused) | Med | Med | Builder in the loop from Phase 04 session 5; 15-minute usability criterion |

## 13. Open Questions (status and recommendations)

| Q | Question | Status |
|---|----------|--------|
| Q1 | PvP ship combat at launch, or PvE-first with PvP behind a zone flag? | **RESOLVED - the question was mis-framed.** The MUD already has a two-layer PvP model: `pk_allowed` (on in this install) plus a per-player `PRF_PVP` opt-in enforced by `pvp_ok()`, arena excepted. Vessel combat now honours it via `vessel_pvp_permitted()`, so PvP is consensual by the same rule as every other hostile action - no separate zone flag or launch decision needed. |
| Q2 | Clan ownership shares/voting, or single owner with permission lists? | **RESOLVED**: owner plus permission list (up to 10 names), as recommended. Clan shares remain a Could-have. |
| Q3 | Do port prices react to player volume, or stay static? | **RESOLVED**: soft, with hard bounds. Buying and selling shift local supply; prices are clamped to +/-60% of base and unit-tested across the whole supply domain. |
| Q4 | Encounters instanced per ship, or shared world objects? | **RESOLVED in code** (shared - creatures spawn into the ship's wilderness room, so anyone there interacts with them), matching the recommendation. **Untested** with two or more ships in one encounter region. |
| Q5 | MSDP only, or GMCP too? | **PARTIALLY RESOLVED**: nine MSDP ship variables are implemented and pushed on the vessel tick. GMCP support in protocol.c was never audited, so the GMCP half of the question is still open. |
| Q6 | Extend the wilderness weather field, or layer a vessel-only weather model? | **RESOLVED**: neither was necessary. The existing `get_weather(x, y)` field carried enough signal for storm severity, fog, and helm risk. Weather stays shared with land players; no fork, no extension. |

---

*This document supersedes the roadmap implications of earlier phase docs. When it
conflicts with VESSEL_SYSTEM.md on future work, this PRD wins; on current behavior,
the code wins.*
