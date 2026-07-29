# Vessel System Remaining Work

**Last audited:** July 29, 2026

**Status:** The local 30-step regression, development release-boundary checks,
complete reboot/copyover state matrix, installed-weapon persistence, and dock
fees pass. Offline insurance delivery also passes with an actual owner. Shared
validation, deletion and PvP lifecycle verification, scale, content, beta, and
production release work remain.

This is the only vessel planning document in the temporary Zusuk workspace. It
contains outstanding work only. Durable requirements live in
[PRD.md](../../PRD.md), current behavior and operations in
[VESSEL_SYSTEM.md](../../systems/VESSEL_SYSTEM.md), measured evidence in
[VESSEL_BENCHMARKS.md](../../testing/VESSEL_BENCHMARKS.md), and completed work
in [CHANGELOG.md](../../CHANGELOG.md).

Do not treat code completion as production approval. Work through the
dependencies below in order and remove completed items from this file after
recording enduring behavior or evidence in the permanent documentation.

## 1. Build the Shared Validation Environment

- [ ] Expand zone 700 into a harbor sandbox with two docks, three representative
  ship prototypes, a persistent scheduled NPC ferry, and DG triggers in vessel
  interiors.
- [ ] Time a builder with no C knowledge creating and spawning a sailable vessel
  using only `vedit`; target less than 15 minutes.
- [ ] Run the scheduled ferry continuously for 24 hours without route,
  coordinate, room, or persistence desynchronization.

The harbor is required before meaningful ferry, merchant, builder, copyover,
and multiplayer encounter testing.

## 2. Close Persistence and Lifecycle Gaps

- [ ] Live-test the implemented player-deletion policy with a disposable owner:
  soft deletion and restoration retain deeds; permanent removal makes ships
  unowned, removes helm permits, voids pending claims, and fails closed if the
  transaction cannot commit.
- [ ] With two actual PvP-enabled characters, live-test the persisted,
  opponent-specific five-minute combat logout grace. Prove the original
  aggressor can continue only inside the window, a third party cannot, expiry
  protects the vessel, and ownership change clears the snapshot.

## 3. Prove Scale, Stability, and Economy

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

## 4. Add Living-World Content

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

## 5. Finish Player Experience and Presentation

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

## 6. Balance, Beta, and Roll Out

- [ ] Tune combat time-to-kill, crew wages, freight margins, refit costs,
  insurance, and dock fees using the simulation, duel tests, and player data.
- [ ] Run a structured player beta against the release scorecard in
  [PRD.md](../../PRD.md). In particular, validate first-hour discovery,
  multiplayer roles, builder independence, week-long NPC shipping, and at
  least 70 percent "fun" combat feedback.
- [ ] Rehearse all schema migrations and rollbacks against a production
  snapshot with no data loss.
- [ ] Confirm production preflight on the release candidate: repeat the
  regression, load-bearing-toggle, debug-off, and authoritative-help checks;
  require lifecycle recovery, benchmark, soak, and documented operator
  recovery evidence.
- [ ] Roll out in stages: staff, beta cohort, then all players. Prepare the
  announcement, monitor each stage, retain rollback authority, and write the
  postmortem. Update the permanent evidence and behavior references, and only
  then mark the vessel system 3.0.

## 7. Open Decisions

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
