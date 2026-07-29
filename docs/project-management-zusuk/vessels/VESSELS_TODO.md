# Vessel System Remaining Work

**Last audited:** July 29, 2026

**Status:** Gameplay code is implemented; live validation and release work
remain.

This is the only vessel planning document in the temporary Zusuk workspace. It
contains outstanding work only. Durable requirements live in
[PRD.md](../../PRD.md), current behavior and operations in
[VESSEL_SYSTEM.md](../../systems/VESSEL_SYSTEM.md), measured evidence in
[VESSEL_BENCHMARKS.md](../../testing/VESSEL_BENCHMARKS.md), and completed work
in [CHANGELOG.md](../../CHANGELOG.md).

Do not treat code completion as production approval. Work through the
dependencies below in order and remove completed items from this file after
recording enduring behavior or evidence in the permanent documentation.

## 1. Unblock the First Live Regression

- [ ] Fix the legacy zone-700 ship identity mismatch.
  - Object 70002 currently carries interior room 70003 and ship index 0.
  - Initialization places the data in `greyhawk_ships[0]` but assigns
    `shipnum = 1`.
  - The same initialization binds `shiproom`, `room_vnums[0]`, and
    `world[room].ship` to room 1403, while boarding enters room 70003.
  - Disembark, speed, heading, status, and sailing callers use `shipnum` as an
    array index and therefore read the wrong slot.
  - The short-term recommended repair is to put the fixture consistently in
    slot 1, set object 70002's ship index to 1, and bind every interior field to
    room 70003. Verify all affected callers by tracing them before editing.
- [ ] Remove the underlying dual meaning of `shipnum`. Use one canonical fleet
  identity and a separate occupancy test so slot/index correctness no longer
  depends on a nonzero sentinel convention.
- [ ] Restart the 30-step
  [manual regression](../../testing/VESSEL_SYSTEM_TESTING.md) at step 1. The
  July 26 run passed steps 1 and 2 and failed step 3; every section A through G
  must pass on development after the repair.

## 2. Make the Development Release Boundary Real

- [ ] Make the cedit `CONFIG_VESSEL_SYSTEM` toggle gate vessel command dispatch
  and every vessel tick. It currently affects interior detection only and
  cannot stop a faulty live subsystem.
- [ ] Set `VESSEL_SYSTEM_DEBUG` to 0 before any production build. Retain focused
  diagnostics for development without allowing movement-level syslog flooding
  in production.
- [ ] Apply `sql/components/help_vessel_entries.sql` to the authoritative
  database and verify all vessel and vehicle command keywords in game.
- [ ] Decide whether to delete the tracked `autopilot.hlp` and `schedule.hlp`
  copies after the database migration so duplicate help sources cannot drift.
- [ ] Promote the master and category debug switches to runtime controls where
  practical. This is polish, not a substitute for the production-off default.

## 3. Build the Shared Validation Environment

- [ ] Expand zone 700 into a harbor sandbox with two docks, three representative
  ship prototypes, a persistent scheduled NPC ferry, and DG triggers in vessel
  interiors.
- [ ] Time a builder with no C knowledge creating and spawning a sailable vessel
  using only `vedit`; target less than 15 minutes.
- [ ] Run the scheduled ferry continuously for 24 hours without route,
  coordinate, room, or persistence desynchronization.

The harbor is required before meaningful ferry, merchant, builder, copyover,
and multiplayer encounter testing.

## 4. Close Persistence and Lifecycle Gaps

- [ ] Test copyover and full reboot while a vessel is mid-voyage, mid-combat,
  and carrying cargo. Confirm route, position, ownership, combat, cargo, crew,
  upgrade, and schedule state after recovery.
- [ ] Add safe runtime reclamation for generated vessel interior rooms after a
  vessel is purged or destroyed; currently they remain allocated until reboot.
- [ ] Deliver insurance payouts to offline owners through mail instead of
  requiring staff reconciliation.
- [ ] Decide and implement the player-deletion orphan policy: make ships
  unowned, transfer them, or scuttle them. Cover deletion and restoration.
- [ ] Persist installed weapons in a `ship_weapons` table with install,
  rollback, verification, save/load, and lifecycle coverage.
- [ ] Implement dock fees and define their relationship to port ownership and
  the vessel economy.
- [ ] Close the PvP logout escape: an owner logging out during combat currently
  makes the ship untouchable. Define and test a bounded recent-combat grace
  period without weakening the shared PvP consent gate.

## 5. Prove Scale, Stability, and Economy

- [ ] Benchmark 500 active ships on the production tick path with autopilot,
  schedules, combat, encounters, weather, economy, wear, persistence, and MSDP
  enabled. The complete vessel work must remain within 25 ms per tick; record
  median, p95, p99, maximum, memory, query volume, and subsystem attribution in
  [VESSEL_BENCHMARKS.md](../../testing/VESSEL_BENCHMARKS.md).
- [ ] Run a 72-hour development soak with NPC fleets active after the benchmark
  passes. Require zero crashes, leaks, unbounded growth, corrupt records, or
  schedule desynchronization, with the tick budget held.
- [ ] Run a scripted 1,000-trade economy simulation. Confirm prices stay inside
  their hard bounds, inventory converges sensibly, and no route yields
  unbounded profit.
- [ ] Add encounter determinism, shared-region multi-ship, and Z-axis boundary
  tests.

## 6. Add Living-World Content

- [ ] Add scheduled, killable NPC merchant ships carrying real cargo on real
  routes, with faction and bounty consequences.
- [ ] Add passenger ferries that collect fares and recover safely after reboot.
- [ ] Add bounty-hunter warships delivered through the encounter/spawn engine
  for players with the HUNTED state.
- [ ] Author territorial waters, free seas, and pirate coves as
  `REGION_GEOGRAPHIC` regions. Piracy legality must use shared wilderness
  geography rather than private coordinate tables.
- [ ] Add data- and DG-driven derelicts with explorable interiors, salvage,
  logs, maps, discovery chains, and optional first-finder naming.
- [ ] Add bathymetry-anchored trenches, sky islands, high-altitude lanes, and
  `path_data` river travel for rafts and boats.
- [ ] Give each of the eight vessel classes at least one unique destination or
  capability.
- [ ] Add regattas, staff-triggered fleet skirmishes, a ghost-fleet event, and
  leaderboards. Optional showcase events may be deferred behind release safety.

## 7. Finish Player Experience and Presentation

- [ ] Replace the legacy tactical grid with a wilderness-renderer tactical map
  showing coastline, shoals, region boundaries, contacts, range rings, and
  damage state.
- [ ] Build lookout view v2 from actual surrounding wilderness sectors.
- [ ] Add dynamic at-sea descriptions through `narrative_weaver` and
  `region_hints`, plus class-, weather-, and speed-aware ambient messages.
- [ ] Announce named-sea boundary crossings from `REGION_GEOGRAPHIC`, add a
  ship-wide captain channel, and throttle repeated ambient or combat messages.
- [ ] Refine hostile boarding with a grapple step, contested rolls, and a
  dedicated boarding skill instead of the current level-plus-Athletics blend.
- [ ] Add optional figurehead and paint customization to ship and lookout
  descriptions.

## 8. Balance, Beta, and Roll Out

- [ ] Tune combat time-to-kill, crew wages, freight margins, refit costs,
  insurance, and dock fees using the simulation, duel tests, and player data.
- [ ] Run a structured player beta against the release scorecard in
  [PRD.md](../../PRD.md). In particular, validate first-hour discovery,
  multiplayer roles, builder independence, week-long NPC shipping, and at
  least 70 percent "fun" combat feedback.
- [ ] Rehearse all schema migrations and rollbacks against a production
  snapshot with no data loss.
- [ ] Confirm production preflight: complete regression, load-bearing toggle,
  debug off, authoritative help loaded, lifecycle recovery passed, benchmark
  passed, soak passed, and documented operator recovery paths.
- [ ] Roll out in stages: staff, beta cohort, then all players. Prepare the
  announcement, monitor each stage, retain rollback authority, and write the
  postmortem. Update the permanent evidence and behavior references, and only
  then mark the vessel system 3.0.

## 9. Open Decisions

- [ ] Confirm that shared encounters are the final model by testing multiple
  ships entering the same regional encounter. Change the model only if the
  shared-world behavior fails the multiplayer requirement.
- [ ] Audit GMCP vessel-state support. Decide whether MSDP alone is the release
  contract or whether equivalent GMCP variables are required.

## Completion Rule

The backlog is complete only when all Must-have release criteria in
[PRD.md](../../PRD.md) have evidence, all production gates above pass, and no
release-blocking item remains here. Optional cosmetics and showcase events may
be explicitly deferred to the general project backlog; they must not be
silently reported as complete.
