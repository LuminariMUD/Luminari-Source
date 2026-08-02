# Vessel System Remaining Work

**Last audited:** August 2, 2026

**Status:** Mechanics through Phase 16, the first Luminari campaign shipping
package, the first data/DG-driven derelict, and the first wilderness frontier
package are implemented. Regattas, fleet skirmishes, ghost-fleet events, and
durable leaderboards also pass actual-character acceptance. The core
development release gates for build,
regression, Memcheck, bounded ferry recovery, 500-vessel
performance/stability, economy simulation, shared encounters, Z-axis
boundaries, native MSDP, named-water crossing, captain-channel isolation, and
message throttling pass. The installed development candidate is not approved
for production until the remaining player experience, balance, beta,
production-snapshot rehearsal, preflight, and staged rollout work below is
complete.

**Current release checkpoint (August 2, 2026, 09:04 IDT):** Full run
`/tmp/luminari-vessel-scale-benchmark-1000/runs/20260802T052407Z-896082`
is terminal `PASS`. Its complete request-to-cleanup task took 2,338 seconds,
within the one-hour ceiling, including 1,862 seconds of measurement with 500
ships and all eight classes on one PID. Across 3,655 production ticks,
median/p95/p99/maximum were 599/4,079/5,169.06/10,520 usec. Every subsystem
maximum remained below 25 ms. The run recorded 20 shared multi-ship
encounters, 10 scheduled departures, 12 airship Z values from 128 through 210,
23,181 suppressed messages, 4,247 database executions, zero workload errors,
zero high-volume progress rows, zero buffer overflows, and movement trails
0/0/0. Cleanup restored all six baseline vessel rows and restarted local
development.

The 63-sample process series retained two threads and 11-13 descriptors. RSS
rose 783,032 to 816,180 KiB while the awake world added 1,613 mobiles and 451
objects; rooms remained 52,418 and allocation lists fell from 1,490 to 1,112.
The bounded analyzer result remains `REPORT_ONLY`, not a long-horizon leak or
plateau claim. Complementary Memcheck across all 277 production-linked tests
reports zero errors and zero definite, indirect, or possible loss. The
368,451 reachable bytes remain owned by process-lifetime registries. The
focused protocol parser passes 13 of 13, the prior integrated CMake gate
passes 6 of 6, required `make install` removes the root artifact, and the
installed normal binary remains SHA-256
`281c7469702fbbeaa52f40a916a3911b121d3cfa9bd1050ed9feb4f1bad92075`.

**Campaign-content checkpoint (August 2, 2026, 09:59 IDT):** The tracked,
idempotent Vailand package now maps two territorial-water regions, the
Vailand Passage free seas, Blackwake Anchorage pirate cove, an 18-link
water-only route between the existing North and Central Vailand seaports, a
merchant-cog prototype, iron market gradient, and the scheduled, faction-1
`Vailand Ironwind Trader`. Development provision run
`/tmp/luminari-vessel-campaign-1000/runs/20260802T065410Z-1061371` passes in
167 seconds on source `923c8024`. Actual Kohdee observed territorial, free,
pirate, and Central territorial waters, real iron cargo, the route, movement,
the Central-port arrival, return movement after a hard restart, and two clean
shutdown checkpoints. Merchant lifecycle run
`/tmp/luminari-vessel-merchant-check-1000/runs/20260802T065717Z-1068792`
passes in 22 seconds: merchant 18 generation 1 was sunk, 165 standing and a
900-gold bounty were observed, generation 2 retained 40 iron, pilot 31810,
route, and schedule, and cleanup byte-restored Kohdee and every snapshotted
vessel/economy table.

**Derelict-content checkpoint (August 2, 2026, 10:59 IDT):** The tracked
Blackwake package supplies three object records, five DG triggers, an
idempotent prototype/mapping migration with verification and guarded rollback,
and a collision-sensitive development provisioner. Provision run
`/tmp/luminari-vessel-derelict-1000/runs/20260802T072737Z-1135588` passed in 61
seconds: actual Kohdee traversed all four generated rooms, and slot 8,
prototype 17, coordinates `(-533, 330)`, rooms 70160-70163, and all three room
trigger mappings remained stable across a hard restart. Reversible discovery
run `/tmp/luminari-vessel-derelict-check-1000/runs/20260802T075751Z-1199403`
passed in 55 seconds on source `a390a387`. Actual Kohdee followed the gated
log-to-chart-to-cargo chain across another hard restart, retained exactly one
log and chart in both object-save mirrors, salvaged one tidefinder for 180
gold, persisted all five DG variables, and then restored the original player,
index, object, optional legacy-variable, database-header, and database-object
state exactly. First-finder naming is intentionally not enabled for this
initial derelict; it remains an optional, non-release-blocking extension.

**Frontier-content checkpoint (August 2, 2026, 12:16 IDT):** The tracked,
idempotent frontier package owns the Starfall Trench bathymetric region,
Aetherwind Skyway altitude lane, Shardspire Sky Island, a 79-cell digitalized
Sablebranch River path, and one acceptance prototype for every vessel class.
Development run
`/tmp/luminari-vessel-frontier-1000/runs/20260802T091531Z-1364409` is terminal
`PASS` in 75 seconds on source `873171ae` and installed SHA-256
`9b329263602de6e1a655e68183389bbae73414bc9d21951e004603809856b6ec`.
Actual Kohdee sailed both river hulls from `(-810, 480)` to `(-809, 480)`,
crossed Starfall waters in a 12,000-pound survey ship, verified all three
warship weapon slots without firing, dived the Starfall Bathyscaphe to Z -90
inside natural depth 104, activated the 125-percent Aetherwind speed lane at Z
100, and entered Shardspire at `(469, 0, 200)`. The 40,000-pound transport
generated three cargo holds, while the magical vessel crossed Plains, River,
Z -10, and Z 10 in one continuous journey. The gate purged every temporary
hull and returned Kohdee to room 1204. End-to-end testing also repaired ignored
`reglist type` and `pathlist type` filters, missing River status output, and an
unspawnable airship speed. The production-linked suite passes 278 tests.

**Showcase-event checkpoint (August 2, 2026, 13:03 IDT):** Phase 16 adds one
staff-managed event at a time, a one-hour ceiling, movement-scored regattas,
damage- and sinking-scored team skirmishes, temporary persistent ghost fleets,
transactional result finalization, boot recovery, and public leaderboards.
Reversible run
`/tmp/luminari-vessel-event-check-1000/runs/20260802T100241Z-1463421`
is terminal `PASS` in 61 seconds on source `9ffe75d0` and installed SHA-256
`ace95edc41320918cd04ef0d6fa93effea9a65f06a04ae99d545a7e63fa0113a`.
Actual Kohdee placed first in a River regatta, dealt 12 live damage for the
winning red skirmish fleet, and hit one of three spawned ghost warships. All
three leaderboard rows advanced exactly once during the snapshot. Cleanup
restored the player file and all four event tables to identical hashes, left
zero event runtime or temporary hull rows, and restarted the exact candidate.
The warning-free production-linked suite passes 282 tests.

Permanent evidence and behavior live in:

- [VESSEL_BENCHMARKS.md](../../testing/VESSEL_BENCHMARKS.md)
- [VESSEL_SYSTEM_TESTING.md](../../testing/VESSEL_SYSTEM_TESTING.md)
- [VESSEL_SYSTEM.md](../../systems/VESSEL_SYSTEM.md)
- [CHANGELOG.md](../../CHANGELOG.md)

Do not restart an unattended long-duration ferry or fleet monitor. Future
agent-run vessel gates must retain the one-hour total ceiling, including setup,
recovery, review, and cleanup. Before destructive merchant or hunter checks,
confirm no benchmark worker owns the development service.

**Remaining checklist:** 10 top-level items: 5 player-experience/presentation
and 5 balance/beta/rollout.

## 1. Add Living-World Content

- [x] Add data- and DG-driven derelicts with explorable interiors, salvage,
  logs, maps, discovery chains, and optional first-finder naming.
- [x] Add bathymetry-anchored trenches, sky islands, high-altitude lanes, and
  `path_data` river travel for rafts and boats.
- [x] Give each of the eight vessel classes at least one unique destination or
  capability.
- [x] Add regattas, staff-triggered fleet skirmishes, a ghost-fleet event, and
  leaderboards.

## 2. Finish Player Experience and Presentation

- [ ] Replace the legacy tactical grid with a wilderness-renderer tactical map
  showing coastline, shoals, region boundaries, contacts, range rings, and
  damage state.
- [ ] Build lookout view v2 from actual surrounding wilderness sectors.
- [ ] Add dynamic at-sea descriptions through `narrative_weaver` and
  `region_hints`, plus class-, weather-, and speed-aware ambient messages.
- [ ] Refine hostile boarding with a grapple step, contested rolls, and a
  dedicated boarding skill instead of the current level-plus-Athletics blend.
- [ ] Add optional figurehead and paint customization to ship and lookout
  descriptions.

Named-water announcements, the ship-wide captain channel, and repeated-message
throttling are complete. The harbor provisioner passes a matching crossing/
`seastate` transcript and same-account Kohdee/Vesselmate cross-room isolation.
The full scale gate records 23,181 suppressed combat/ambient messages and 20
shared encounters, confirming the final shared-world encounter model.

## 3. Balance, Beta, and Roll Out

- [ ] Tune combat time-to-kill, crew wages, freight margins, refit costs,
  insurance, and dock fees using the simulation, duel tests, and player data.
- [ ] Run a structured player beta against the release scorecard in
  [PRD.md](../../PRD.md). Validate first-hour discovery, multiplayer roles,
  builder independence, a supervised NPC-shipping sample within one hour, and
  at least 70 percent "fun" combat feedback.
- [ ] Rehearse every schema migration and rollback against a production
  snapshot with no data loss. Do not modify the live production database.
- [ ] Confirm production preflight on the release candidate: repeat regression,
  load-bearing-toggle, debug-off, authoritative-help, lifecycle recovery,
  benchmark, bounded stability, and documented operator-recovery checks.
- [ ] Roll out in stages: staff, beta cohort, then all players. Prepare the
  announcement, monitor each stage, retain rollback authority, and write the
  postmortem. Update permanent evidence and behavior references before marking
  the vessel system 3.0.

## Completion Rule

The backlog is complete only when every Must-have release criterion in
[PRD.md](../../PRD.md) has evidence, all production gates pass, and no
release-blocking item remains here. Optional cosmetics may be explicitly
deferred to the general project backlog; they must not be silently reported as
complete.
