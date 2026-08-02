# Vessel System Remaining Work

**Last audited:** August 2, 2026

**Status:** Mechanics through Phase 15 are implemented. The GNU C23
production-linked suite passes 268 of 268 on current master, and a fresh
CMake build passes all six CTest targets, including the same production suite
and vessel tooling. The
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
ferry shakedown passes through same-PID recovery and the final exact-state hard
restart. The bounded 45-minute ferry gate also passes its actual-character
observation, exact persistence restart, and automatic resume within the
one-hour total budget. Long unattended ferry and fleet soaks are no longer
release gates. Every agent-run vessel validation must complete within a
one-hour total execution budget, including setup, terminal recovery checks,
and cleanup.

The former long-duration ferry attempts are retained only as historical
evidence in [VESSEL_BENCHMARKS.md](../../testing/VESSEL_BENCHMARKS.md). Do not
start or restart an unattended long-duration monitor. The ferry release gate
is closed; retain its explicit bounded invocation only for future regression
runs. Before running scale, hunter, or destructive merchant checks, confirm
that no legacy ferry monitor still owns the installed development server. The
500-ship run, actual shared-encounter and merchant-loss transcripts, bounded
fleet stability checks, campaign content, beta, and production rollout remain.

**Remaining checklist:** 22 top-level items: 4 performance validation,
6 living-world content, 6 player-experience/presentation, 5 balance/beta/
rollout, and 1 encounter-model decision.

**Active validation checkpoint (August 2, 2026, 03:29 IDT):** The bounded
ferry run is terminal `PASS` at
`/tmp/luminari-vessel-ferry-soak-1000/runs/20260801T230546Z-160058`. Its
2,700-second request produced 2,740 seconds of continuous observation and a
terminal result 2,779 seconds after launch. On source
`c539a6d59483f44da121260378287cb33094751e` and installed SHA-256
`6122ff1fbcac07a7a0188ee248bc6269dc4b5f3e0d18dc1764912cbafd24bccd`, ferry
5 completed 1,476 movement steps, 246 waypoint arrivals, and 62 route loops.
All 5 live, 46 database, and 46 process samples passed with fleet count 6,
dynamic rooms 6/13/2 of 2,000, zero buffer overflows, one PID, two threads,
and 12 descriptors. The final hard restart changed PID 160111 to 252880,
launched the identical executable, restored the exact paused coordinates and
route, and resumed the ferry. Preserve the stopped-MUD pre-fix failure at
`/tmp/luminari-vessel-ferry-soak-1000/runs/20260801T230025Z-148892` as harness
regression evidence.

The continuous RSS series rose from 767,396 to 866,944 KiB while ordinary
awake-world mobiles, objects, and movement trails also increased; its full
and trailing 1,797-second slopes are 128,565 and 118,710 KiB/hour. The
generated `memory-analysis.kv` result therefore remains `REPORT_ONLY`, not a
leak or plateau verdict. The current production-linked suite passes 268 of
268. Its first full Memcheck exposed a 24-byte purchased-perk node retained by
ordinary character teardown; `free_char()` now releases that owned list. The
exact rerun passes with zero errors and zero definite, indirect, or possible
loss, and the clean candidate is installed.

The reversible merchant gate passes in 41 seconds on source `1faa59b9` and
installed SHA-256
`63b73ed4ad3411fc5d00fcd8973c361d302335e973fa724858ab481de62c8019`.
Actual Kohdee sank merchant 1 generation 1, received 25 attack plus 125 total-
loss standing penalty and a 510-gold bounty, and inspected generation 2 with
25 spice, its pilot, route, schedule, and registry identity. Cleanup restored
byte-identical database and player-file snapshots; its no-login restart used
PID 299248 with the same installed hash. The reversible HUNTED gate then
passes in 57 seconds at
`/tmp/luminari-vessel-hunter-check-1000/runs/20260802T001910Z-302111`: one real
encounter created the exact hunter, PID 299248 changed to 302590 across the
hard restart, the hunter generation and target reattached, pardon retired it
into cooldown, and cleanup restored the original rows. A current one-session
builder run created prototype 13 and ship 7, sailed from `(-66, 92)` to
`(-67, 92)` at effective speed 1 in storm, and removed both records in 2.8
seconds of workflow and 8 seconds total. The development help database was
behind the tracked migration and initially lacked `SHIPTALK`; applying the
idempotent authoritative SQL and restarting produced five passing verifier
checks and an actual Kohdee sweep of all 78 command keywords. The focused
protocol parser passes 13 of 13, both character-rename gates pass, and a fresh
out-of-tree CMake build passes 6 of 6 CTest targets. `make install` restored
the proven Autotools SHA-256 and removed the root artifact after CMake; a clean
restart now maps that exact installed path on PID 324960. Next run the bounded
500-vessel scale gate.

This is the only vessel planning document in the temporary Zusuk workspace. It
contains outstanding work only. Durable requirements live in
[PRD.md](../../PRD.md), current behavior and operations in
[VESSEL_SYSTEM.md](../../systems/VESSEL_SYSTEM.md), measured evidence in
[VESSEL_BENCHMARKS.md](../../testing/VESSEL_BENCHMARKS.md), and completed work
in [CHANGELOG.md](../../CHANGELOG.md).

Do not treat code completion as production approval. Work through the
dependencies below in order and remove completed items from this file after
recording enduring behavior or evidence in the permanent documentation.

## 1. Prove Scale, Stability, and Economy

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
  percentiles or impossible stored/seen/call sample counts. The August 1
  distribution audit also found that the scale/ferry/login/memory scripts were
  incomplete in Automake's release manifest and that ordinary root and CMake
  tests skipped the safe memory and scale-parser regressions. All required
  scripts are now distributed, and both build systems run those three tooling
  tests as part of their normal test gates. A follow-up completeness scan added
  the missing harbor zone, vessel planning/behavior/testing/deployment docs,
  authoritative help SQL, master schema, and Phase 2-10 install/verify/rollback
  files. A packaged source tree now contains the complete documented local
  acceptance and schema-rehearsal inputs.
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
- [ ] Run a supervised post-benchmark stability check with NPC fleets active.
  The complete gate, including setup, evidence collection, restoration, and
  review, must finish within one hour. Limit the scale runner's steady
  measurement window to at most 1,800 seconds so setup and cleanup retain the
  other 30 minutes. Stop and clean up rather than exceeding the total budget.
  Require no crash, corrupt record, schedule desynchronization, buffer
  overflow, PID or executable drift, or tick-budget failure during the bounded
  window.

  Carry the correlated live-system and process series into this gate so RSS
  can be reviewed beside fleet count, dynamic wilderness occupancy, world
  objects/mobiles/rooms, allocation lists, movement trails, and buffer
  overflows. A one-hour-bounded observation cannot prove the absence of a
  long-horizon leak, so record the memory analysis and reject obvious runaway
  growth without presenting the result as a multi-day leak verdict. Repeat
  Memcheck for the current candidate as complementary evidence. The
  pre-Phase15 full-suite Memcheck reported zero errors and zero definite,
  indirect, or possible loss after fixing uninitialized secondary affect
  flags and gameplay-fixture teardown; its 301,630 reachable
  process-lifetime bytes remain context rather than runtime proof.

  The scale runner accepts longer measurements, but this plan does not permit
  lifting the 1,800-second measurement limit or launching a separate
  long-duration service. Three monotonic per-autopilot counters provide
  movement, arrival, and complete-route evidence through `autopilot status`;
  every active live interval must advance all three. The scale worker reports
  measured log bytes and fails on old unconditional or compiled-debug
  movement, arrival, wait, route-loop, wilderness-region, sector-transform,
  elevation, or path-progress rows. Its reconstruction check reads only the
  current 500-vessel boot's log slice. The live-system validator requires
  `system-0`, strictly increasing timestamps and labels, the exact terminal
  duration, and the calculated sample count.

  `scripts/analyze_vessel_memory_samples.sh` validates the process series and
  emits block means plus full, post-warmup, and trailing RSS/VSZ regressions.
  It remains `REPORT_ONLY`; review that report with the detailed-memory series
  and historical observations, but do not create a longer gate to derive a
  new threshold. The detailed-memory validator rejects PID drift,
  non-increasing timestamps, missing heap metrics, and impossible RSS
  relationships. Automated movement's persisted safe pause and the exact
  movement-trail counter retain their production-linked coverage.
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

## 2. Add Living-World Content

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
  are covered. `scripts/test_vessel_merchant_in_game.sh` now provides the
  reversible actual-Kohdee destruction/consequence/replacement path. It
  snapshots every mutable vessel/economy table and Kohdee's exact player file,
  then restores and byte-compares both before a no-login MUD restart. The
  current installed candidate passes that path in 41 seconds: generation 1 to
  2, 150 total standing loss, 510 bounty, and a fully assembled replacement,
  followed by exact rollback. Keep this item open only for production builders
  to author campaign merchant routes/cargo beyond the development fixture.
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

## 3. Finish Player Experience and Presentation

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

## 4. Balance, Beta, and Roll Out

- [ ] Tune combat time-to-kill, crew wages, freight margins, refit costs,
  insurance, and dock fees using the simulation, duel tests, and player data.
- [ ] Run a structured player beta against the release scorecard in
  [PRD.md](../../PRD.md). In particular, validate first-hour discovery,
  multiplayer roles, builder independence, a supervised NPC-shipping sample
  that fits the one-hour validation ceiling, and at least 70 percent "fun"
  combat feedback.
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

## 5. Open Decisions

- [ ] Confirm that shared encounters are the final model by testing multiple
  ships entering the same regional encounter. Change the model only if the
  shared-world behavior fails the multiplayer requirement.

## Completion Rule

The backlog is complete only when all Must-have release criteria in
[PRD.md](../../PRD.md) have evidence, all production gates above pass, and no
release-blocking item remains here. Optional cosmetics and showcase events may
be explicitly deferred to the general project backlog; they must not be
silently reported as complete.
