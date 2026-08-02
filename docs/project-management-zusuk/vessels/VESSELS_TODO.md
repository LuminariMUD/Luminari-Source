# Vessel System Remaining Work

**Last audited:** August 2, 2026

**Status:** Mechanics through Phase 15 and the first Luminari campaign shipping
package are implemented. The core development release gates for build,
regression, Memcheck, bounded ferry recovery, 500-vessel
performance/stability, economy simulation, shared encounters, Z-axis
boundaries, native MSDP, named-water crossing, captain-channel isolation, and
message throttling pass. The installed development candidate is not approved
for production until the remaining living-world content, player experience,
balance, beta, production-snapshot rehearsal, preflight, and staged rollout
work below is complete.

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

Permanent evidence and behavior live in:

- [VESSEL_BENCHMARKS.md](../../testing/VESSEL_BENCHMARKS.md)
- [VESSEL_SYSTEM_TESTING.md](../../testing/VESSEL_SYSTEM_TESTING.md)
- [VESSEL_SYSTEM.md](../../systems/VESSEL_SYSTEM.md)
- [CHANGELOG.md](../../CHANGELOG.md)

Do not restart an unattended long-duration ferry or fleet monitor. Future
agent-run vessel gates must retain the one-hour total ceiling, including setup,
recovery, review, and cleanup. Before destructive merchant or hunter checks,
confirm no benchmark worker owns the development service.

**Remaining checklist:** 14 top-level items: 4 living-world content,
5 player-experience/presentation, and 5 balance/beta/rollout.

## 1. Add Living-World Content

- [ ] Add data- and DG-driven derelicts with explorable interiors, salvage,
  logs, maps, discovery chains, and optional first-finder naming.
- [ ] Add bathymetry-anchored trenches, sky islands, high-altitude lanes, and
  `path_data` river travel for rafts and boats.
- [ ] Give each of the eight vessel classes at least one unique destination or
  capability.
- [ ] Add regattas, staff-triggered fleet skirmishes, a ghost-fleet event, and
  leaderboards. Optional showcase events may be deferred behind release
  safety, but any deferral must be recorded in the general project backlog.

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
release-blocking item remains here. Optional cosmetics and showcase events may
be explicitly deferred to the general project backlog; they must not be
silently reported as complete.
