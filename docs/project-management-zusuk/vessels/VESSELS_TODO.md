# Vessel System Remaining Work

**Last audited:** August 1, 2026

**Status:** Mechanics through Phase 15 are implemented. The GNU C23
production-linked suite passes 268 of 268 on current
master, CMake passes the preceding
251-test candidate, the
complete disposable MariaDB chain through Phase 15 passes install/reapply/
verify/rollback, and the reversible bounty-hunter lifecycle passes with actual
level-34 Kohdee across a hard restart and pardon. The integrated candidate is
installed on local development. The shared harbor now passes restart
persistence, the exact passenger fare, named-water crossing, merchant
identity/cargo, and same-account captain-channel checks. Message throttling,
native MSDP, the 500-slot workload, and the prior restart/copyover state matrix
are automated. An untraversable automated step now stops the hull, pauses and
persists autopilot once, and tells occupants instead of retrying forever.
Routine wilderness region/path progress logging is removed, and the scale
runner treats its return as a bounded-log failure. A short forced-copyover
ferry shakedown now passes through same-PID recovery and the final exact-state
hard restart; it is not a substitute for the 24-hour duration.

The replacement 24-hour ferry gate is `RUNNING`. It started on August 1 at
22:48:15 IDT in
`/tmp/luminari-vessel-ferry-soak-1000/runs/20260801T194759Z-1807083`, under
`luminari-vessel-ferry-soak-20260801T194759Z-1807083.service`. The monitor
pinned source `4f3ba35511538594b6a86771e93301fb2c983388` and installed SHA-256
`6122ff1fbcac07a7a0188ee248bc6269dc4b5f3e0d18dc1764912cbafd24bccd` after a
clean 268-test pass, `make install`, and an actual Kohdee/Vesselmate harbor
provisioning pass. Its initial live and database samples passed with ferry
slot 5 and route 4. Do not run the scale, hunter, or destructive merchant
checks against the installed server until this monitor reaches a terminal
result. This item remains open until the full 86,400-second window and final
hard-restart recovery both report `PASS`.

The first pinned 24-hour ferry attempt is `ABANDONED`: it remained
vessel-healthy for 34,382 seconds, but the scheduled 11:00:30 IDT copyover
correctly dropped the monitor's non-playing keepalive. The old harness treated
that expected handoff as failure and did not finalize status. The current
harness reconnects only after a log-proven same-PID/same-binary copyover and
writes terminal failure state before cleanup. A full replacement window is
still required. The 500-ship run, actual shared-encounter and merchant-loss
transcripts, longer stability gates, campaign content, beta, and production
rollout remain.

**Remaining checklist:** 23 top-level items: 5 harbor/performance validation,
6 living-world content, 6 player-experience/presentation, 5 balance/beta/
rollout, and 1 encounter-model decision.

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

  The early idle-timeout, provenance, and interruption shakedowns are resolved.
  The pinned full window at
  `/tmp/luminari-vessel-ferry-soak-1000/runs/20260729T222703Z-4128760`
  reached 34,382 seconds before the scheduled 11:00:30 IDT copyover correctly
  dropped its non-playing hold descriptor. The process and ferry survived, but
  the old monitor treated that expected handoff as failure and did not finalize
  status. The run is `ABANDONED`; it does not satisfy the duration gate.
  The current monitor accepts only a log-proven same-PID/same-binary copyover,
  reconnects after boot, records the recovery count, and writes terminal
  failure status before cleanup. It also captures `shiplist summary`,
  `show stats`, `live-system-samples.tsv`, and
  `process-memory-details.tsv`.
  Because autopilot progress counters have process-executable lifetime, the
  monitor starts a new counter segment after each proven copyover, requires
  progress in that segment, and records the recovery number beside every live
  sample.
  The detailed-memory series deliberately remains on the one continuous MUD
  PID through the terminal pre-restart checkpoint. The separate hard-restart
  recovery phase verifies the replacement process's executable hash and exact
  gameplay state instead of corrupting that single-PID series.

  The forced-copyover shakedown at
  `/tmp/luminari-vessel-ferry-soak-1000/runs/20260730T092546Z-1844033`
  is `PASS` on source `823d48b9` and installed SHA-256
  `7237a57d92b0e701cf71e3f38993b869e8fb21c68c48745c9fc0fc77d9c6b4d1`.
  Its 240-second requested window observed 329 wall seconds, one same-PID
  copyover recovery, 132 movement steps, 22 waypoint arrivals, five route
  completions, four live samples, 25 database/process samples, and zero
  buffer overflows. The continuous process series stayed on PID 1817030; the
  final hard restart changed to PID 1859781, recovered the exact paused
  coordinates and route on the same binary, and resumed the ferry. This closes
  the forced-copyover and restart shakedown, not the 24-hour item.

  Partial evidence from the abandoned run remained healthy at the July 30
  08:28 IDT checkpoint
  after 25,202 seconds: 13,716 movement steps, 572/571 west/east arrivals,
  eight actual-character samples, and 421 database/process samples. The MUD
  PID, two threads, and 12 file descriptors remained constant. RSS rose from
  768,776 KiB to 1,134,288 KiB during world warmup. Its last 30-minute,
  1-hour, and 2-hour linear slopes were +5,836, +5,899, and +6,825 KiB/hour.
  Consecutive post-four-hour block slopes fell from +11,212 through +8,061 to
  +5,895 KiB/hour, versus about +120,000 KiB/hour in the first hour.
  `/proc` attributed 879,932 KiB RSS to an 880,116 KiB heap mapping, with
  1,113,812 KiB anonymous RSS, 20,576 KiB file RSS, and no swap.

  A separate actual Kohdee checkpoint completed and logged out cleanly in four
  seconds. It found five active ships, 13 of 2,000 dynamic rooms occupied,
  37,329 mobiles, 26,429 objects, 50,366 rooms, 897 allocation lists, 102
  buffer switches, and zero overflows. Two actual-character samples 55 minutes
  apart showed 201 more mobiles and 143 more objects while RSS rose 5,812 KiB.
  Their base `char_data` and `obj_data` structures alone account for about
  2,885 KiB of that change before per-instance allocations, so part of the
  remaining rise correlates with ordinary world population rather than ferry
  occupancy. A read-only standalone MariaDB client then repeated 20,000
  region/path assignment pairs, 40,000 result cycles total, against the ferry
  route. It remained flat after its first sample at 6,008 KiB RSS and 9,172
  KiB VSZ, excluding the direct spatial query/result cycle by itself as the
  source of the game process's rise. Neither correlation proves a root cause.
  The strong deceleration has not yet established a plateau, so this
  checkpoint is neither a leak verdict nor a substitute for the terminal
  result.

  At 08:42 IDT, another actual Kohdee checkpoint completed and logged out
  cleanly in six seconds. The fleet remained five, dynamic wilderness
  occupancy was 3 of 2,000, rooms remained 50,366, and buffer overflows
  remained zero. Since 08:28, mobiles rose by 25 to 37,354, objects rose by
  10 to 26,439, allocation lists fell by 13 to 884, and RSS rose 1,260 KiB to
  1,135,660 KiB. The added base mobile/object structures account for about
  354 KiB before their dynamic state. The contemporaneous heap mapping held
  881,204 KiB RSS/private-dirty, file RSS remained 20,576 KiB, and swap
  remained zero.

  At its last complete sample, the same pinned process had 574 samples over
  34,382 seconds and ended at 1,144,640 KiB RSS. Its post-four-hour regression
  was +5,923 KiB/hour, with trailing one- and two-hour slopes of +3,784 and
  +3,772 KiB/hour. The full measured details now live in
  [VESSEL_BENCHMARKS.md](../../testing/VESSEL_BENCHMARKS.md). They are partial
  warmup evidence, not a bounded-growth or duration verdict.

  The awake-world log exposes another material baseline: movement trails are
  retained for 12,600 seconds and cleaned every 75 seconds. By the checkpoint,
  the latest complete 168-cleanup retention window represented 1,314,302
  removed trail records, averaging 7,823 per cleanup. Their 48-byte structures
  alone represent at least 60.16 MiB before two duplicated strings and
  allocator metadata. Hour-sized removal-block means rose from 7,305 through
  7,774 and 7,916 to 8,044 as the mobile population warmed. This establishes
  a high-volume full-world retention confounder, not that trails explain the
  entire heap or that the ferry leaks.

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
  promotion, missed-heartbeat and vessel-message-throttling counters, and a
  process-wide direct/prepared SQL execution count. The fleet array now
  reserves slot 0 separately from active slots 1-500; the matching zone 700
  reservation reaches the final slot's VNUM 80019. The GNU C23
  production-linked suite covers the capacity, bounded fleet-summary,
  encounter, Z-axis, sustained-economy, and message-cooldown regressions, and
  isolated provisioner checks pass both extension and overlap rejection.
  Autopilot movement now resolves and validates each target dynamic room once
  through the central position update. The removed immediate traversal probe
  had configured the same room and executed the region/path spatial queries a
  second time whenever an automated step entered an otherwise unoccupied
  coordinate. The production-linked suite now passes 268 of 268 and isolated
  `make install` is clean. Only the current installed-candidate scale run can
  provide live evidence for this change.
  `scripts/run_vessel_scale_benchmark.sh` now constructs the reversible
  development-only workload through actual Kohdee/`vedit spawnpublic`
  sessions, covers all eight classes and periodic subsystems, captures the
  required evidence, and restores the pre-run database. It reuses one master
  account, with Kohdee for all fleet phases and at most one reusable
  `Vesselmate` for the channel proof, rather than creating a character per
  vessel. A paused reciprocal submarine pair keeps firing synchronized
  one-damage weapons after
  `perfmon reset`; its fixture defense speed prevents damaging hits, negative
  Z excludes surface weather, and the runner puts Kohdee aboard for an
  eight-second live observation. Its preserved transcript must contain
  return-fire text and a nonzero throttled-message count. The runner also
  negotiates native MSDP as Kohdee, verifies all nine `SHIP_*` frames aboard,
  and requires their empty state ashore. Its static checks and active-soak
  refusal pass. The quick guide now gives one installed-candidate sequence:
  `make test`, `make install`, and the scale runner; that runner invokes the
  harbor/fare/crossing/channel gates itself. Its exact profiler contract now
  contains all 11 production heartbeat rows, including `vessel_hunters`, and a
  parser regression compares that list with `src/comm.c`. The gate remains
  open until the current binary is installed and the runner records a terminal
  result. The August 1 preflight audit found that the documented stale-binary
  refusal only compared the running and installed hashes; it did not compare
  the installed file with current build inputs. The runner now rejects
  `bin/circle` when any C source, header, or primary build file is newer and
  directs the operator through `make test` and `make install`. A deterministic
  fixture proves both the stale and current cases before the long scale gate.
  The same audit found that only the tick call count and maximum were required
  to be numeric. The runner now validates the complete ten-field `vessel_tick`
  CSV row, requires ordered median/p95/p99/maximum values, and rejects missing
  percentiles or impossible stored/seen/call sample counts.
  The measured Kohdee session now records timestamped initial, hourly, and
  final `shiplist summary`/`show stats` checkpoints. The runner writes their
  fleet, dynamic-room, mobile, object, room, allocation-list, movement-trail,
  and buffer values to `live-system-samples.tsv`, rejects fleet/capacity drift
  or any buffer overflow, and records RSS, VSZ, threads, and file descriptors
  in a headered process series. `show stats` derives the exact live trail count
  from the room lists at each infrequent checkpoint; terminal summaries
  preserve its initial, maximum, and final values. Each process sample also
  rejects replacement of the installed executable. A separate sparse series
  captures anonymous,
  file-backed, and shared RSS, data, swap, and heap size/RSS/private-dirty at
  measurement start, each complete intermediate hour, and measurement end.
  It does not scan `smaps` in the 30-second process loop. An actual
  pinned-build Kohdee transcript verified the `show stats` grammar, and an
  exact current-output fixture verified both accepted and rejected parser
  paths. The ferry parser now accepts both the
  older full-list and current compact fleet-count wording.
  Instrumentation, capacity, and workload readiness do not themselves prove
  the 25 ms target.
- [ ] Run a 72-hour development soak with NPC fleets active after the benchmark
  passes. Require zero crashes, leaks, unbounded growth, corrupt records, or
  schedule desynchronization, with the tick budget held. Carry the supervised
  live-system series into this gate so process RSS can be correlated with
  fleet count, dynamic wilderness occupancy, world objects/mobiles/rooms,
  allocation lists, and buffer overflows. Define and enforce a post-warmup
  bounded-growth criterion before launch; initial/maximum/final RSS alone does
  not prove the absence of unbounded growth. The pre-Phase15 full-suite
  Memcheck pass reports zero errors and zero definite, indirect, or possible
  loss after fixing uninitialized secondary affect flags and gameplay-fixture
  teardown. Repeat Memcheck for the current candidate. Its 301,630 reachable
  process-lifetime bytes are not a substitute for the 72-hour runtime series.
  The scale runner now has the required
  correlated game/process sample format, but it still deliberately caps one
  measurement at 7,200 seconds. Before lifting that cap for the 72-hour gate,
  define the threshold from the completed 24-hour and default 500-ship
  observations. The current candidate has moved per-step position, waypoint,
  wait, and route-loop messages behind compiled development diagnostics.
  Three monotonic per-autopilot counters now provide movement, arrival, and
  complete-route evidence through `autopilot status`; the next ferry monitor
  requires every active live interval to advance all three. The counters add
  only 24 bytes per optional autopilot (72 bytes total), while the base ship
  remains 4,928 bytes. The production-linked suite passes 268 of 268. Confirm
  bounded actual server-log growth in the default installed 500-ship run
  before enabling a 72-hour window. The scale worker now reports measured log
  bytes and fails if any old unconditional or compiled-debug movement,
  arrival, wait, or route-loop progress row appears. Its reconstruction check
  reads only the log slice from the current 500-vessel boot, preventing stale
  success or error lines from changing the verdict. Deterministic clean/noisy
  log fixtures and complete/incomplete live-system parser fixtures pass. The
  live-system validator also requires `system-0`, strictly increasing
  timestamps and labels, hourly intermediate labels, the exact terminal
  duration, and the calculated sample count; duplicate-epoch and wrong-final
  fixtures are rejected.
  `scripts/analyze_vessel_memory_samples.sh` now validates both the legacy
  headerless and current headered process-series formats, rejects timestamp,
  PID, or metric corruption, and emits consecutive block means plus full,
  post-warmup, and configurable trailing RSS/VSZ regressions. Its stable
  `key=value` mode and deterministic plateau/rising-series tests pass. It
  deliberately reports `REPORT_ONLY`; use the completed 24-hour and default
  500-ship results to define and encode the threshold before starting the
  72-hour run. Future ferry and scale workers write this default report as
  `memory-analysis.kv` and fail if their terminal process series is invalid;
  the abandoned pinned ferry predates that automatic step and can be analyzed
  manually without changing it. The new detailed-memory sampler and validator
  likewise have deterministic status/`smaps` and live-process coverage. They
  reject PID drift, non-increasing timestamps, missing heap metrics, and
  impossible RSS relationships. Future workers validate
  `process-memory-details.tsv` terminally; the abandoned pinned ferry predates
  that artifact. The exact movement-trail counter has production-linked
  create/move coverage, and updated parser fixtures require it in every
  future live-system checkpoint. The abandoned pinned binary predates that
  field. Automated movement now turns a failed terrain or Z step into one
  persisted pause with speed zero, and production-linked coverage verifies
  that state transition. The benchmark's bounded-log detector additionally
  rejects the former unconditional wilderness region, sector-transform,
  elevation, and path progress signatures.
- [ ] Run a scripted 1,000-trade economy simulation. Confirm prices stay inside
  their hard bounds, inventory converges sensibly, and no route yields
  unbounded profit. The automated gate now passes all 1,000 adversarial
  transfers: marginal batch pricing charges each crossed supply level, the
  old oversized-shipment reversal cycle loses gold, legitimate arbitrage
  closes after a finite number of trips, supplies remain inside 10-400, and
  idle restocking returns both ports to 100. `vtradecheck 1000` exposes the
  same production calculation to one actual Kohdee session, and the
  500-vessel runner requires its PASS transcript. Keep this item open until
  that installed-candidate command is recorded in-game. A safe July 30
  Kohdee probe against the pinned ferry executable returned `Huh?!` and
  logged out cleanly in five seconds. Git history confirms this is expected:
  pinned source `0afad17b` predates the command's `ac418322` implementation.
  Retry only after the current candidate is installed.
- [ ] Add encounter determinism, shared-region multi-ship, and Z-axis boundary
  tests. The automated layer now passes: overlapping regions use containment
  position and then lowest VNUM regardless of query order; equal-chance rows
  use encounter ID; one successful encounter claims a shared exterior room
  once and broadcasts to every co-located hull; class Z limits reject surface
  flight, airship ceiling violations, and non-water submergence; and autopilot
  actually advances on Z without overshoot. The scale workload now varies
  airship altitude and requires distinct live Kohdee Z samples. Keep this item
  open until the installed candidate proves a two-ship shared encounter,
  an airship vertical route, and manual upper/lower boundary rejection through
  actual in-game commands.

## 3. Add Living-World Content

- [ ] Add scheduled, killable NPC merchant ships carrying real cargo on real
  routes, with faction and bounty consequences.
  Phase 14 now supplies the missing lifecycle. Durable definitions map a
  faction, prototype, route, pilot, spawn coordinate, commodity, quantity,
  schedule interval, and recovery delay to an ordinary public hull. Boot and
  MUD-hour reconciliation assemble real interiors, cargo, pilot, schedule, and
  route; sink, capture, purge, or a missing hull releases the definition for a
  fresh generation. A 300-second attribution window prevents an old attacker
  from receiving blame for a later environmental loss. Deduplicated durable
  rows apply attack, plunder, capture, and sink standing losses exactly once,
  commit regional bounties, survive logout, and follow character rename.
  Permanent character removal voids pending consequences, clears current
  attribution, and removes the bounty row.

  The development harbor seed and provisioner now require a live merchant
  generation with its real spice cargo, pilot, enabled schedule, route,
  in-game registry row, and ship-status identity. If boot leaves a delayed or
  stale definition, the provisioner performs two timed in-game reconciliation
  passes before failing, avoiding manual recovery between reruns. The ordinary
  PvP gate explicitly permits ownerless hulls, and regression coverage proves
  that attaching a merchant registry identity does not make the NPC ship
  immune to player attacks. The GNU C23
  production-linked suite passes 268 of 268 with constructor rejection,
  respawn gating, faction scaling, responsibility-window, faction persistence,
  consequence high-water, and merchant combat-consent coverage. The complete
  `make test` then isolated `make install` gate passes without a root build
  artifact. Phase 14 install/reapply/verify/rollback and the full schema chain
  pass in disposable MariaDB; character rename and permanent-removal mappings
  are covered. Keep this item open for two things: the installed-candidate
  Kohdee destruction/consequence/replacement transcript, and builder-authored
  campaign merchant routes/cargo beyond the development fixture.
- [ ] Author territorial waters, free seas, and pirate coves as
  `REGION_GEOGRAPHIC` regions. Piracy legality must use shared wilderness
  geography rather than private coordinate tables.
  The campaign-neutral mechanics and development content are complete:
  Phase 13 attaches water type, authority, overlap priority, and a bounded
  bounty multiplier to geographic region VNUMs; plunder and port refusal
  resolve the canonical `region_index` polygon; `seastate` exposes the result.
  The harbor seed authors territorial waters (150%), nested free seas (100%),
  and a pirate cove (0%), and its provisioner validates the spatial index and
  post-restart in-game display. The installed candidate now passes the actual
  Kohdee crossing and matching `seastate` transcript. Keep this open for
  production builders to author campaign regions.
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
  Named-water crossings now resolve the boot-loaded canonical polygons,
  announce once ship-wide, and remain quiet until the vessel changes regions;
  law metadata is cached so movement adds no SQL. `shiptalk` now carries an
  identified captain-channel message across every occupied room of one vessel
  without leaking ashore. Repeated weather/depth messages now use independent
  120-second severity cooldowns; repeated damage, return-fire, miss, and reload
  messages are limited per class to one copy per half-second vessel tick while
  critical failure warnings remain immediate. The login helper now owns both
  simultaneous character sockets for the channel proof, selecting Kohdee and
  the first other usable character from the same master account in one run;
  an explicit Name remains optional. If that account has no second usable
  character, the harbor provisioner adds reusable `Vesselmate` to the same
  account, then runs the channel gate on its discovered ferry slot. The
  named-water helper waits on the moving ferry's real crossing announcement
  and correlates its region, water type, authority, and bounty with an
  immediate `seastate`; the same provisioner runs that proof automatically.
  The installed candidate now passes the crossing transcript and a
  same-account Kohdee/Vesselmate two-character channel transcript. Keep this
  item open until the scale runner records the message-cooldown transcript
  through Kohdee's live observation and required nonzero suppression counter.
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
