# Vessel System Remaining Work

**Last audited:** July 30, 2026

**Status:** The local 30-step regression, development release-boundary checks,
complete reboot/copyover state matrix, installed-weapon persistence, dock fees,
offline insurance delivery, player-removal recovery, and the multi-character
PvP logout lifecycle pass with actual characters are proven. Immediate
autopilot command durability, write-failure rollback, and static/dynamic
exterior-hull co-location through restart and zone reset are also proven. The
shared two-dock harbor, representative prototypes, persistent scheduled ferry,
and generated-room DG triggers are available. Monotonic per-subsystem timing,
rolling percentiles, CSV output, correct interval promotion, and a
process-wide SQL execution counter are now available for the scale gate. The
fleet now has 500 usable nonzero slots and a matching interior VNUM
reservation. Automated encounter ordering, shared-room deduplication, and
class Z boundaries now pass; their actual-character confirmation remains
queued. Native MSDP is now the explicit vessel release contract, stale client
state is cleared after disembarkation, and the scale runner includes a real
Kohdee Telnet-option-69 exchange; its installed-build transcript remains
queued. Public schedule fares now have fail-closed boarding collection,
persistence, schema migration, and a reversible Kohdee harbor check; its
installed-build transcript is also queued behind the pinned soak. Regional
piracy law now keys territorial, free-sea, and pirate-cove policy to canonical
wilderness geography; its installed-build transcript is queued with the fare
check. Continuous validation, execution of the reproducible 500-ship workload,
content, beta, and production release work remain. Pre-soak testing repaired
signed-coordinate movement and both unsafe legs of the sample ferry route; an
actual Kohdee session now completes the full four-waypoint loop. The first
monitor shakedown was rejected when its idle pre-login descriptor expired and
let the game loop sleep; the corrected confirmation-state keepalive then passed
a 150-second replacement with 84 continuous movement steps and exact-state
restart recovery. The continuous 24-hour result is still unverified.

This is the only vessel planning document in the temporary Zusuk workspace. It
contains outstanding work only. Durable requirements live in
[PRD.md](../../PRD.md), current behavior and operations in
[VESSEL_SYSTEM.md](../../systems/VESSEL_SYSTEM.md), measured evidence in
[VESSEL_BENCHMARKS.md](../../testing/VESSEL_BENCHMARKS.md), and completed work
in [CHANGELOG.md](../../CHANGELOG.md).

Do not treat code completion as production approval. Work through the
dependencies below in order and remove completed items from this file after
recording enduring behavior or evidence in the permanent documentation.

## 1. Validate the Shared Harbor

- [ ] Run the scheduled ferry continuously for 24 hours without route,
  coordinate, room, or persistence desynchronization.

  The July 30 run starting at 01:04:38 IDT is invalid: its idle account-name
  connection expired at 01:05:16 and the game loop slept. Its artifact is
  `ABANDONED`. The corrected 150-second replacement shakedown passed. A second
  full window started at 01:16:31 IDT and ran correctly, but was intentionally
  superseded after source audit showed the monitor did not pin the installed
  executable across its final restart. Provenance and interruption
  shakedowns now pass. The definitive local-development window started July
  30 at 01:27:19 IDT and reaches 24 hours on July 31 at 01:27:19 IDT. Its run
  directory is
  `/tmp/luminari-vessel-ferry-soak-1000/runs/20260729T222703Z-4128760`;
  check it with `./scripts/run_vessel_ferry_soak.sh status`. It pins source
  `0afad17bdb8fd67a78a58fa1af9e41d6ccc79efc` and executable SHA-256
  `ae7c6414bc934f4ddf09f6c35a3d97b15a9a5fa1845c13a109142eaf9b5ca2a2`.
  Leave this item open until it passes the final exact-state restart.

Use the provisioned harbor for the continuous ferry run before meaningful
merchant, copyover, and multiplayer encounter testing.

## 2. Prove Scale, Stability, and Economy

- [ ] Benchmark 500 active ships on the production tick path with autopilot,
  schedules, combat, encounters, weather, economy, wear, persistence, and MSDP
  enabled. The complete vessel work must remain within 25 ms per tick; record
  median, p95, p99, maximum, memory, query volume, and subsystem attribution in
  [VESSEL_BENCHMARKS.md](../../testing/VESSEL_BENCHMARKS.md).
  The July 30 instrumentation prerequisite is complete: `perfmon reset` and
  `perfmon csv` provide monotonic aggregate and per-subsystem timing,
  median/p95/p99/max, rolling-sample counts, correct one-time hierarchy
  promotion, and a process-wide direct/prepared SQL execution count. The
  fleet array now reserves slot 0 separately from active slots 1-500; the
  matching zone 700 reservation reaches the final slot's VNUM 80019. The GNU
  C23 production-linked suite passes 237 tests with the capacity, bounded
  fleet-summary, encounter, Z-axis, and sustained-economy regressions, and
  isolated provisioner checks pass both extension and overlap rejection.
  `scripts/run_vessel_scale_benchmark.sh` now constructs the reversible
  development-only workload through actual Kohdee/`vedit spawnpublic`
  sessions, covers all eight classes and periodic subsystems, captures the
  required evidence, and restores the pre-run database. It reuses one account
  and character rather than creating one per vessel. The runner also
  negotiates native MSDP as Kohdee, verifies all nine `SHIP_*` frames aboard,
  and requires their empty state ashore. Its static checks and active-soak
  refusal pass, but the gate itself must remain open until the definitive
  ferry soak finishes, the current binary is installed, and the runner records
  a terminal result. Instrumentation, capacity, and workload readiness do not
  themselves prove the 25 ms target.
- [ ] Run a 72-hour development soak with NPC fleets active after the benchmark
  passes. Require zero crashes, leaks, unbounded growth, corrupt records, or
  schedule desynchronization, with the tick budget held.
- [ ] Run a scripted 1,000-trade economy simulation. Confirm prices stay inside
  their hard bounds, inventory converges sensibly, and no route yields
  unbounded profit. The automated gate now passes all 1,000 adversarial
  transfers: marginal batch pricing charges each crossed supply level, the
  old oversized-shipment reversal cycle loses gold, legitimate arbitrage
  closes after a finite number of trips, supplies remain inside 10-400, and
  idle restocking returns both ports to 100. `vtradecheck 1000` exposes the
  same production calculation to one actual Kohdee session, and the
  500-vessel runner requires its PASS transcript. Keep this item open until
  that post-soak installed-build command is recorded in-game.
- [ ] Add encounter determinism, shared-region multi-ship, and Z-axis boundary
  tests. The automated layer now passes: overlapping regions use containment
  position and then lowest VNUM regardless of query order; equal-chance rows
  use encounter ID; one successful encounter claims a shared exterior room
  once and broadcasts to every co-located hull; class Z limits reject surface
  flight, airship ceiling violations, and non-water submergence; and autopilot
  actually advances on Z without overshoot. The scale workload now varies
  airship altitude and requires distinct live Kohdee Z samples. Keep this item
  open until the post-soak installed build proves a two-ship shared encounter,
  an airship vertical route, and manual upper/lower boundary rejection through
  actual in-game commands.

## 3. Add Living-World Content

- [ ] Add scheduled, killable NPC merchant ships carrying real cargo on real
  routes, with faction and bounty consequences.
  The public-hull, pilot, schedule, persistent-cargo, combat, and geographic
  bounty primitives now exist. Still add a durable NPC merchant lifecycle,
  merchant identity/faction consequences, authored production routes and
  cargo, and actual-character destruction/recovery evidence.
- [ ] Add passenger ferries that collect fares and recover safely after reboot.
  The code and data layer is complete: an optional fare is part of the
  persistent schedule, public boarding saves the deduction before moving the
  passenger, and failure rolls back gold and denies entry. Phase 12,
  production-linked policy/rollback coverage, authoritative help, and the
  development ferry's 10-gold seed are present. The harbor provisioner now
  checks the fare after a hard restart, boards through the ordinary object
  special as Kohdee, proves exactly one deduction, restores Kohdee's gold, and
  resumes the route. Keep this item open until that installed-build run passes
  after the active 24-hour soak.
- [ ] Add bounty-hunter warships delivered through the encounter/spawn engine
  for players with the HUNTED state.
- [ ] Author territorial waters, free seas, and pirate coves as
  `REGION_GEOGRAPHIC` regions. Piracy legality must use shared wilderness
  geography rather than private coordinate tables.
  The campaign-neutral mechanics and development content are complete:
  Phase 13 attaches water type, authority, overlap priority, and a bounded
  bounty multiplier to geographic region VNUMs; plunder and port refusal
  resolve the canonical `region_index` polygon; `seastate` exposes the result.
  The harbor seed authors territorial waters (150%), nested free seas (100%),
  and a pirate cove (0%), and its provisioner validates the spatial index and
  post-restart in-game display. Keep this open until the post-soak Kohdee run
  passes and production builders author campaign regions.
- [ ] Add data- and DG-driven derelicts with explorable interiors, salvage,
  logs, maps, discovery chains, and optional first-finder naming.
- [ ] Add bathymetry-anchored trenches, sky islands, high-altitude lanes, and
  `path_data` river travel for rafts and boats.
- [ ] Give each of the eight vessel classes at least one unique destination or
  capability.
- [ ] Add regattas, staff-triggered fleet skirmishes, a ghost-fleet event, and
  leaderboards. Optional showcase events may be deferred behind release safety.

## 4. Finish Player Experience and Presentation

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

## 5. Balance, Beta, and Roll Out

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

## 6. Open Decisions

- [ ] Confirm that shared encounters are the final model by testing multiple
  ships entering the same regional encounter. Change the model only if the
  shared-world behavior fails the multiplayer requirement.

## Completion Rule

The backlog is complete only when all Must-have release criteria in
[PRD.md](../../PRD.md) have evidence, all production gates above pass, and no
release-blocking item remains here. Optional cosmetics and showcase events may
be explicitly deferred to the general project backlog; they must not be
silently reported as complete.
