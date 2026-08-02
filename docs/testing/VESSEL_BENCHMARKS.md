# Vessel System Benchmarks

**Version:** 3.17

**Evidence snapshot:** August 2, 2026

**Last updated:** August 2, 2026

This document records measured vessel-system evidence and the remaining release
performance gate. It intentionally separates completed foundation measurements
from the full live-game benchmark that still must be run.

## Evidence Summary

| Measure | Result | Status |
|---|---:|---|
| Configured fleet-array entries | 501 | Slot 0 is reserved; active maximum is 500 |
| Base `greyhawk_ship_data` size | 4,928 bytes | Within 5 KB budget |
| Base storage for 501 array entries | 2,468,928 bytes (about 2.35 MiB) | Within about 3 MB budget |
| Production-linked vessel test gate on July 26, 2026 | 74 of 74 passing | Historical snapshot |
| Valgrind result for that test gate | 0 errors, 0 leaks | Historical snapshot |
| Root suite on August 1, 2026 | 268 of 268 passing | Current production-linked gate |
| Pre-Phase15 suite Memcheck | 0 errors; 0 definite, indirect, or possible loss | Historical pre-soak gate; rerun current candidate |
| Current 268-test suite Memcheck on August 2, 2026 | 0 errors; 0 definite, indirect, or possible loss | Passing after character perk teardown fix |
| Bounded actual-character ferry gate on August 2, 2026 | 2,740-second observation; 62 route completions; exact restart | Passing |
| First current 500-ship attempt on August 2, 2026 | Reached slot 500; stopped before measurement | Harness race fixed; rerun required |
| Second current 500-ship attempt on August 2, 2026 | Corrected Z check and 500 live; stopped before measurement | Fresh-log slicing fixed; rerun required |
| Third current scale launch on August 2, 2026 | Harbor preflight stopped before spawn | Canonical west-dock fare path fixed; rerun required |
| Fourth current 500-ship attempt on August 2, 2026 | Reached reciprocal-combat proof; stopped before steady measurement | LF-CR parser fixed; performance warning retained |
| Fifth current 500-ship attempt on August 2, 2026 | Reached native MSDP proof; stopped before steady measurement | Raw Telnet client fixed and baseline contract passes |
| Sixth current 500-ship attempt on August 2, 2026 | Completed 1,800-second window; 3,676 ticks | Overflow and 25 ms performance gates failed |
| Post-sixth optimization candidate | 271/271 tests; actionable Memcheck clean | Installed and exercised by seventh launch |
| Seventh current 500-ship attempt on August 2, 2026 | Completed 1,800-second window; 3,665 ticks; zero overflows | Harness, route, memory, and 25 ms gates failed; merchant capacity deferral expected |
| Post-seventh repair candidate | 274/274 tests; actionable Memcheck clean | Installed; restart and scale rerun required |
| Eighth current 500-ship diagnostic on August 2, 2026 | Completed 600-second window; 1,217 ticks; all functional gates passed | Tick latency and memory gates failed |
| Complete current 500-ship live tick | 1,217 ticks; p95 66,429 usec | Release blocker; optimization and rerun required |

The release target is a complete vessel tick at or below 25 ms with 500 active
ships and the production gameplay workload enabled. Navigation-only
microbenchmarks do not satisfy this target.

## Memory Measurements

### Ship Structure

The measured `sizeof(struct greyhawk_ship_data)` is 4,928 bytes.

| Component | Approximate bytes |
|---|---:|
| Ten ship slots and description arrays | 2,680 |
| Connection data | 640 |
| Sail-crew and gun-crew data | 518 |
| Room arrays | 160 |
| Helm permits | 210 |
| Cargo data | 80 |
| Crew tiers | 16 |
| New counters and state fields | 219 |
| Other fields and padding | 405 |
| **Total** | **4,928** |

Recent gameplay and runtime-observability fields account for roughly 412
bytes, or about 8.4 percent of the structure. Older documentation that
reported a 1,016-byte ship structure is obsolete.

### Fleet Projection

| Ships | Base bytes | Approximate size |
|---:|---:|---:|
| 100 | 492,800 | 481.2 KiB |
| 250 | 1,232,000 | 1.17 MiB |
| 500 | 2,464,000 | 2.35 MiB |
| Fixed 501-entry array | 2,468,928 | 2.35 MiB |

The separate vehicle array has a measured element size of 152 bytes. At 1,000
vehicles, its base storage is 152,000 bytes, or about 148.4 KiB.

Optional autopilot state is 72 bytes, including three 64-bit runtime progress
counters. At 500 attached autopilots this is 36,000 bytes, about 35.2 KiB.

Optional allocations, strings, routes, encounter data, and database result
buffers add runtime memory beyond these base arrays. Those allocations must be
included in soak-test observation, but the fixed fleet array remains within its
budget.

### Supporting Structure Sizes

| Structure | Size |
|---|---:|
| `greyhawk_ship_data` | 4,928 bytes |
| `vehicle_data` | 152 bytes |
| Autopilot state | 72 bytes |
| Route data | 1,840 bytes |
| Waypoint data | 88 bytes |
| Route node | 104 bytes |
| Transport state | 16 bytes |
| Movement trail record, before two strings | 48 bytes |

## Performance Evidence

### Historical Foundation Microbenchmark

The original navigation foundation produced the following approximate movement
tick timings in 2025:

| Active ships | Approximate movement tick |
|---:|---:|
| 10 | Less than 1 ms |
| 100 | About 2 ms |
| 500 | About 10 ms |

These figures predate the complete combat, encounter, economy, wear, client,
and automation workload. They are useful only as foundation history and must
not be presented as evidence that the current 25 ms release gate passes.

### Required Release Benchmark

Run the production tick path with all of the following enabled:

- 500 active ships distributed across representative regions and elevations.
- Scheduled NPC fleets and ferries.
- Autopilot and route following.
- Encounters, weather, hazards, and Z-axis transitions.
- Combat, weapons, boarding state, damage, sinking, and insurance work.
- Cargo, trade, economy, crew wages, wear, and maintenance.
- MSDP updates and normal player-facing message production.
- Database persistence at realistic save intervals.

Record at minimum:

- Median, 95th percentile, 99th percentile, and maximum complete tick time.
- Total vessel time and per-subsystem time.
- Process resident memory at start, steady state, and end.
- Database query volume and slow queries.
- Missed heartbeats, delayed schedules, and message-throttling events.

The gate passes only when the complete vessel tick remains at or below 25 ms
under the agreed 500-ship workload. If rare outliers are accepted, their
threshold and operational effect must be documented before release.

### Instrumentation Readiness

The July 30 instrumentation prerequisites are implemented and covered by the
current production-linked root suite:

- Section timings use a monotonic clock. The eleven explicitly sampled vessel
  benchmark sections each retain up to 16,384 rolling microsecond samples.
  Sampling is opt-in so ordinary command and special-function sections cannot
  accumulate unbounded profiler memory. The eleven windows use about 1.38 MiB.
- The complete half-second vessel group is recorded as `vessel_tick`.
  Its separately attributed children are `vessel_autopilot`,
  `vessel_hunters`, `vessel_combat`, `vessel_crew_wages`, `vessel_upkeep`,
  `vessel_trade`, `vessel_weather`, `vessel_encounters`, and `vessel_msdp`.
- The 75-second schedule path is recorded separately as `vessel_schedules`.
- Automated movement now resolves and validates its target wilderness room
  once in the central position update. The previous immediate
  `can_vessel_traverse_terrain()` probe configured the same dynamic room first,
  doubling the region/path spatial queries when an autopilot step entered an
  otherwise unoccupied coordinate.
- `perfmon reset` starts a new pulse, section, missed-heartbeat,
  vessel-message-throttling, and process-wide SQL execution window. The SQL
  counter includes direct, safe, and pooled `mysql_query()` attempts plus
  prepared-statement executions.
- `perfmon csv` emits machine-readable rows for the sampled vessel sections,
  including calls, total, average, median, p95, p99, maximum, samples stored,
  and samples seen. It then emits `# missed_pulses=<count>` and
  `# vessel_messages_throttled=<count>` before the final
  `# database_queries=<count>`. `perfmon prof` remains the human-readable view
  of every section.
- Interval promotion now occurs once per completed lower-level buffer, and an
  hourly maximum is derived from the completed minute buffer instead of the
  prior hour buffer.

Use this collection sequence:

1. Start the representative workload and allow caches, sections, and database
   connections to warm up.
2. Run `perfmon reset`.
3. Hold the steady workload for at least 10 minutes so the 1,200-vessel-tick
   trade interval runs at least once. This also covers the 75-second schedule,
   60-tick weather, 180-tick encounter, 600-tick wage, and 900-tick wear
   intervals.
4. Run `perfmon csv` and capture the complete output with process memory,
   missed-heartbeat, slow-query, and message-throttling evidence.

At two complete vessel ticks per second, the 16,384-sample window covers about
2 hours 16 minutes. Longer runs remain valid rolling-window observations, but
the reported `samples_stored` and `samples_seen` values must be preserved so
the evidence is not mistaken for a full-run distribution.

The capacity prerequisite is corrected: the fleet array now contains 501
entries, reserving slot 0 while allowing active slots 1-500, and the final
slot's interior allocation extends through VNUM 80019. The development
provisioner extends zone 700 only from the former expected upper bound and
rejects overlaps.

### Reproducible Development Workload

`scripts/run_vessel_scale_benchmark.sh` now defines the development-only
workload and evidence contract. Its first current-candidate attempt reached the
complete 500-slot workload but stopped before measurement; a terminal rerun is
still required. Before launching it, confirm that no legacy ferry monitor
still owns the server. The runner's presence or premeasurement progress is not
evidence that the live 25 ms gate passes.

The retained attempt is
`/tmp/luminari-vessel-scale-benchmark-1000/runs/20260802T003029Z-328201`.
Actual Kohdee passed harbor fare/restoration, named-water and same-account
channel checks, spawned slots through 500 across all classes, ran the
1,000-trade simulation, and inspected reconstructed schedule, crew, cargo, and
Z state. It then failed the airship ceiling assertion before profiler reset.
The transcript showed the configured airship at Z 480 rather than the seeded
Z 500 because its active autopilot moved during the login delay; `setsail up`
validly reached 490. The corrected fixture persists that one airship in paused
state at Z 500 until Kohdee tests the rejection, then resumes the route for
live altitude variation. Cleanup restored the six-row baseline and restarted
the unchanged installed candidate. No timing or memory result from this run is
valid release evidence.

The retry is
`/tmp/luminari-vessel-scale-benchmark-1000/runs/20260802T004119Z-349856`.
It observed the corrected Z-500 ceiling rejection twice, reconstructed all 500
ships, and passed the economy and live workload transcript. It then stopped
before profiler reset because the reconstruction log slice began at the old
pre-restart byte offset. The development login helper truncates that log when
it starts the 500-ship process, so the slice began mid-record and omitted the
valid boot summary. The runner now uses byte zero for this known-fresh log.
The transcript also rendered `**OVERFLOW**` when Kohdee returned to a harbor
containing hundreds of hull objects. Generic ashore, native-MSDP, message, and
terminal transitions now use quiet staff room 1204, while the actual harbor
gates retain their dock rooms. Cleanup restored the baseline. This retry also
contains no valid performance or memory measurement.

Launch `20260802T005623Z-378533` stopped even earlier, during harbor preflight.
The ferry reached west-dock coordinates and stopped, but disembarkation placed
Kohdee in a room that did not contain the hull object; ordinary boarding
therefore refused proximity and no fare was charged. The corrected gate waits
specifically for `(-66, 92)`, pauses and stops, disembarks, resolves static
room 1000389, and then exercises normal boarding. The full standalone harbor
provisioner passed the 10-gold charge/restoration, named-water crossing, and
same-account channel checks immediately afterward. No fleet was created and
the cleanup-restored launch contains no benchmark sample.

Run `20260802T010309Z-392860` passed the repaired harbor, complete fleet,
economy, Z-boundary, and reconstruction gates. Its reciprocal-combat helper
also observed repeated return fire and a valid suppression count of 393. The
helper nevertheless stopped because LF-CR Telnet output placed a carriage
return before each CSV line, defeating a start-anchored regex. The common
output cleaner now strips that leading CR while retaining intended indentation.

That helper reset the profiler and captured only 18 vessel ticks over eight
seconds, so it is diagnostic rather than the required steady measurement. It
reported median 571 usec, p95 126,589.05 usec, p99 206,847.41 usec, maximum
226,912 usec, and 26 missed pulses. The largest sampled subsystem maxima were
autopilot at 226,835 usec and encounters at 107,698 usec. These values already
exceed the 25,000-usec release target and require investigation if reproduced
in the full run; they must not be diluted or presented as a passing result.
Cleanup restored the baseline.

Run `20260802T011448Z-414722` passed the repaired harbor and reciprocal-combat
paths, spawned all 500 ships through one 496-command Kohdee session, observed
the corrected Z-500 boundary, completed the economy and fresh-reconstruction
checks, and recorded 18 suppressed messages. It stopped before steady
measurement at the native MSDP gate. The in-game `whois` diagnostic proved
`MSDP: Yes`, but the `nc` client was attached to a cooked pseudo-terminal that
buffered and caret-echoed MSDP's binary VAR/VAL control bytes. The helper now
uses a raw no-echo connection, declines TTYPE before accepting option 69, and
validates the effective client cache after ashore updates. A missing clear
frame passes only when the prior aboard value was already the required neutral
value. An actual Kohdee rerun against baseline ship 1 received all nine aboard
variables and reached the complete empty ashore state in seven seconds. The
follow-up against actively navigating public ferry slot 5 paused it at
`(-63, 82)`, received the same complete state, resumed autopilot, and cleared
ashore in eight seconds. Last-frame selection prevents a late movement update
from hiding the subsequent clear. The fifth run cleaned back to six vessels
and restarted the exact candidate on PID 431693. Its 18-tick preflight sample
is diagnostic only and is not a release measurement.

Run `20260802T013644Z-457615` is the first complete current-candidate steady
window. It passed harbor, 500-slot construction, economy, Z, reconstruction,
reciprocal combat, and raw MSDP, then measured for the requested 1,800 seconds
with one process and all 500 vessels. Terminal validation stopped on a real
premeasurement buffer invariant: the reconstruction login inherited Kohdee's
post-spawn harbor location, rendered hundreds of hulls before `shiplist
summary`, and recorded `**OVERFLOW**`. Both game-side checkpoints therefore
reported one overflow. The spawn session must save Kohdee in quiet room 1204
before the workload restart.

The preserved `perfmon csv` nevertheless contains the complete diagnostic
distribution:

| Section | Calls | Median usec | p95 usec | p99 usec | Maximum usec |
|---|---:|---:|---:|---:|---:|
| `vessel_tick` | 3,676 | 764.50 | 130,928.50 | 166,398.50 | 1,027,228 |
| `vessel_autopilot` | 3,676 | 670.50 | 129,214.75 | 165,059.25 | 213,780 |
| `vessel_crew_wages` | 3,676 | 15.00 | 23.00 | 44.00 | 1,014,543 |
| `vessel_encounters` | 3,676 | 0.00 | 1.00 | 1.00 | 149,653 |
| `vessel_schedules` | 25 | 16,744.00 | 21,514.40 | 24,181.72 | 24,889 |

The window also recorded 6,157 missed pulses, 23,888 throttled messages, and
80,950 database queries. Median behavior is fast, but p95, p99, and maximum
all fail the 25,000-usec release budget. Synchronized wage due processing is
the one-second maximum; repeated dynamic-room region/path work drives the
autopilot distribution; per-ship encounter-region SQL drives the encounter
spike.

The 57 process samples span 1,861 seconds on PID 466495. RSS increased from
786,784 to 854,412 KiB and VSZ from 881,544 to 948,828 KiB. Threads remained
2 and descriptors 11-12. The sparse detailed samples attribute the increase
to anonymous/heap memory, while actual game checkpoints show movement trails
increasing from 30,426 to 289,000 and mobiles from 32,384 to 33,993. The
generated trend remains `REPORT_ONLY`; its full RSS slope is 130,282 KiB/hour
and its trailing-window slope is 129,104 KiB/hour. Cleanup restored the six-
vessel baseline and exact installed candidate on PID 522541.

The post-sixth candidate directly bounds all three measured spikes. Full-fleet
payroll is divided into 100 stable batches of at most five ships per tick, and
crew persistence now uses one multi-row insert after the roster delete.
Encounter containment is resolved once per shared exterior room during each
pass. Dynamic wilderness rooms retain reusable coordinate, region, path, and
terrain metadata after release, avoiding repeated spatial queries along warm
routes. The spawn character is saved in quiet room 1204 before reconstruction.
The production-linked suite passes 271 of 271 without warnings, vessel tooling
passes, and strict actionable Memcheck has zero errors and zero definite,
indirect, or possible loss. Installed SHA-256
`ade8d4db466ec5d2f49a5cd7f30ceda4a3e29af570921e8e6005797c7e8db12e`
runs on PID 565375; an actual Kohdee smoke confirms the six-vessel baseline.
Run `20260802T024352Z-573327` exercised that candidate for the full requested
1,800 seconds with 500 vessels on one PID. It passed harbor, construction,
economy, Z, reconstruction, reciprocal combat, and native MSDP. The initial
and final live samples each reported zero overflows, accepting the quiet-room
repair. The terminal script stopped first because its generic `@wait` path
discarded all asynchronous socket output before checking for the encounter
text. The server log independently proves 20 encounter deliveries from
Kohdee's moving airship and 20 shared deliveries to 60 co-located ships. This
is a harness defect, but the same log contains a later real workload failure:
225 scheduled autopilot moves attempted a non-navigable intermediate cell.
Six spawn attempts were also deferred after all 500 slots were occupied.
Production call tracing identifies those six as baseline NPC merchant
prototype 7 reconciliation, not hunter-vessel creation; capacity deferral is
expected.

The complete seventh profile is diagnostic evidence, not a pass:

| Section | Calls | Median usec | p95 usec | p99 usec | Maximum usec |
|---|---:|---:|---:|---:|---:|
| `vessel_tick` | 3,665 | 802.00 | 131,989.20 | 176,272.80 | 355,394 |
| `vessel_autopilot` | 3,665 | 626.00 | 130,774.00 | 170,540.04 | 218,707 |
| `vessel_crew_wages` | 3,665 | 16.00 | 9,146.80 | 12,005.16 | 353,062 |
| `vessel_encounters` | 3,665 | 0.00 | 1.00 | 1.00 | 60,540 |
| `vessel_schedules` | 25 | 16,081.00 | 20,237.60 | 20,515.40 | 20,579 |

Database executions fell from 80,950 to 67,052 and payroll p95 fell from 23
usec plus a one-second synchronized outlier to 9,146.80 usec with a 353,062-
usec maximum. Encounter p99 remained 1 usec, but one 60,540-usec synchronous
lookup remains. Autopilot p95 remains about 131 ms, so the complete tick still
misses the 25,000-usec target. The run recorded 6,217 missed pulses and 23,204
throttled messages.

The 59 process samples span 1,854 seconds on PID 582492. RSS increased from
786,296 to 853,480 KiB and VSZ from 881,084 to 947,988 KiB; both maxima equal
their final value. Threads stayed at 2 and descriptors at 11-12. Anonymous RSS
and the heap mapping each grew by about 65 MiB, while movement trails grew
from 29,608 to 287,220. The analyzer reports a 129,567-KiB/hour RSS slope and
remains `REPORT_ONLY`, so memory stability is still open. Cleanup restored six
vessels and restarted the exact installed SHA-256 on PID 640439.

The following repair candidate is installed as SHA-256
`0e79d6edb09be793d293ac31dee4aa42860368c4381659861309ce1d4bec3021` from
pushed commit `d610d58a`. Generic `@wait` now captures, displays, and
returns cleaned asynchronous data. The reciprocal water schedule and all
water-class runtime fixtures stay on the actual-Kohdee-verified `y = 82`
channel from `x = -66` through `x = -62`. A full fleet now reports merchant
prototype reconciliation as informational deferral. Movement-law polygon
resolution has a bounded 4,096-entry coordinate cache; encounter containment
uses the canonical in-memory region polygons instead of synchronous spatial
SQL; and a payroll batch persists up to five crew departures with one atomic
delete. Movement-trail signatures refresh in place and each room retains at
most 16. The warning-free server build, vessel tooling, 274 of 274 production-
linked tests, and strict actionable Memcheck across the same 274 tests all
pass. PID 714795 maps the exact installed hash; actual Kohdee passed a
17-second login smoke, then reported the six-ship baseline and saved in quiet
room 1204. These are baseline qualifications only; a fresh 500-vessel
measurement remains required.

Run `20260802T035823Z-718533` then completed the repaired 600-second diagnostic
on source `801c6b671ac99195b5d2ffc9296037ab2e1fbf13` and the same installed
SHA-256. It passed every harbor, construction, economy, reconstruction,
combat, MSDP, and terminal actual-Kohdee gate. Generic `@wait` retained and
displayed unsolicited scheduled crossing and encounter output. The measured
workload recorded six shared multi-ship encounters, ten scheduled departures,
nine distinct live airship Z values from 120 through 200, zero route failures,
zero workload errors, zero high-volume progress logs, and zero buffer
overflows. Initial and final live samples retained 500 ships. Cleanup restored
the six-vessel baseline and restarted local development. This accepts the
harness, route fixture, merchant deferral, containment, and payroll repairs.

The complete eighth profile remains diagnostic evidence, not a pass:

| Section | Calls | Median usec | p95 usec | p99 usec | Maximum usec |
|---|---:|---:|---:|---:|---:|
| `vessel_tick` | 1,217 | 659.00 | 66,429.00 | 86,597.80 | 103,801 |
| `vessel_autopilot` | 1,217 | 554.00 | 66,286.60 | 86,469.16 | 103,711 |
| `vessel_crew_wages` | 1,217 | 18.00 | 31.00 | 46.00 | 112 |
| `vessel_encounters` | 1,217 | 0.00 | 1.00 | 1.00 | 56,901 |
| `vessel_schedules` | 8 | 789.50 | 11,760.55 | 14,764.11 | 15,515 |

Payroll and schedules are now below 25 ms, and the median complete tick
improved, but autopilot still drives p95 above the release limit. The run
recorded 2,150 missed pulses, 7,803 throttled messages, and 14,942 database
executions. One synchronous encounter outlier also remains above budget.

The 22 process samples span 631 seconds on one PID. RSS increased from 786,304
to 807,504 KiB and VSZ from 881,224 to 902,344 KiB; threads stayed at two and
descriptors at 11-12. Movement trails increased from 21,472 to 68,895 while
world room count stayed constant, closely matching the 121,774-KiB/hour RSS
slope. The analyzer remains `REPORT_ONLY`, but repeated awake NPC movement is
the retained-growth source to remove before the full 1,800-second rerun.

The abandoned ferry run was pinned to an earlier executable, so its partial
observation cannot validate the single-pass target-resolution or Phase 15
hunter changes. The installed-candidate scale run is the first live
performance evidence for them. That pinned source `0afad17b` also predated
`vtradecheck` from `ac418322`; its safe Kohdee probe returned `Huh?!` and
logged out cleanly. The economy transcript remains an installed-candidate gate
rather than a pinned-run failure.

The runner:

- Refuses non-development configuration, an active ferry soak, a dirty source
  worktree, stale benchmark markers, an installed binary older than any C
  source, header, or primary build input, and an installed binary without both
  the 500-slot capacity and compact `shiplist summary` behavior.
- Atomically snapshots the 19 vessel, trade, freight, bounty, encounter, and
  insurance tables that the workload can change, then restores and verifies
  the baseline on success, failure, or interruption.
- Uses the existing master account and exact Kohdee character. The harbor
  preflight may add one reusable `Vesselmate` to that account for the channel
  proof. One in-game Kohdee builder session creates every missing public hull;
  no account or character is created per vessel.
- Populates active slots 1-500 across all eight vessel classes while
  preserving each generated class-specific interior. Slot 500 must reconstruct
  with all of its actual room VNUMs inside 80000-80019.
- Loads 500 NPC pilots, 2,000 veteran hired-crew rows, 500 enabled schedules,
  500 bulk-cargo lots, two synchronized weapons per combat hull, insured
  warships, safe surface, submerged, water, and altitude-changing air routes,
  and a message-producing regional airship encounter.
- Holds one reciprocal submarine pair paused at negative Z with synchronized
  one-damage weapons, a fixture defense speed above the NPC attack ceiling,
  and a departure beyond the measurement window. The pair keeps firing and
  reloading after `perfmon reset` but cannot land a damaging hit or receive
  surface weather, so suppression evidence cannot depend on initial timers or
  changing weather.
- Discovers the reciprocal fixture slot, puts Kohdee aboard it for an
  eight-second observation, requires live return-fire text and a nonzero
  suppression counter, and preserves the actual-character transcript as
  `vessel-message-throttling.log`.
- Uses actual Kohdee commands to prove the surface Z-0 boundary, airship
  ceiling, and submarine waterline boundary before timing. During timing,
  minute-by-minute airship status samples must observe at least two Z values
  inside 0-500. A live regional encounter must reach Kohdee and notify
  multiple ships sharing one exterior wilderness room.
- Requires Kohdee's `vtradecheck 1000` transcript to show all adversarial
  transfers inside supply 10-400, finite profitable-route convergence,
  negative oversized reversal profit, and restocking to baseline 100.
- Opens a raw, no-echo native MSDP connection as Kohdee, completes the
  TTYPE-first Telnet negotiation, requests all nine `SHIP_*`
  variables aboard the benchmark airship, compares position and navigation
  values with `shipstatus`, validates hull and status values, then goes ashore
  and requires the effective client state to be empty. An omitted dirty update
  is accepted only when the previously reported value was already neutral.
  The raw check result is preserved as `native-msdp-vessel-state.log`.
- Warms the production heartbeat, then pauses ten routed vessels after a known
  schedule departure. Each must trigger a new persisted departure during the
  measured window. Kohdee remains aboard an airship so native MSDP and normal
  player-facing message generation execute.
- Runs for at least 600 steady seconds and records RSS, VSZ, threads, file
  descriptors, PID, and installed-executable identity every 30 seconds. The
  held Kohdee session records timestamped fleet, dynamic-room,
  world-allocation, movement-trail, and buffer statistics at the start, every
  hour, and at the end. The runner rejects fleet or capacity drift, occupancy
  above capacity, buffer overflows, vessel errors, PID/binary drift, or a
  missing system sample. It also requires a `system-0` checkpoint, strictly
  increasing epochs and labels, hourly intermediate labels, and an exact
  terminal-duration label.
- Records anonymous, file-backed, and shared RSS, data, swap, and heap
  size/RSS/private-dirty in `process-memory-details.tsv` at measurement start,
  every complete intermediate hour, and measurement end. The sparse schedule
  avoids adding an approximately 0.01-second `smaps` scan to the 30-second
  process-sample loop.
- Uses monotonic per-vessel movement, waypoint-arrival, and route-completion
  counters for continuity evidence. Normal builds no longer emit a server-log
  line for every movement step, arrival, wait completion, or loop; those
  messages remain available only in focused development-debug builds. The
  runner preserves the measured log, reports its byte count, and fails if any
  old unconditional or compiled movement/autopilot progress row appears.
- Captures the fresh log created by the workload restart from byte zero and
  requires the 500-vessel boot success only in that file. Prior success or
  error lines in the truncated development log cannot create a false verdict.
- Captures all eleven sampled vessel profiler rows plus missed-pulse,
  message-throttling, and SQL counters, and requires every selected schedule
  to fire. The reciprocal submarine pair's synchronized reloads produce a nonzero
  `vessel_messages_throttled` count, proving the installed production tick
  suppressed repeated player-facing messages. The complete `vessel_tick`
  maximum must be no more than 25,000 microseconds. The parser regression
  derives the production profiler names from `src/comm.c` and compares them
  with the runner contract, so adding or removing a sampled heartbeat section
  cannot silently invalidate the row count.
- Validates the complete ten-field `vessel_tick` CSV row before applying the
  budget. Every metric must be numeric, median/p95/p99/maximum must be ordered,
  and the stored/seen/call counts must be possible. Deterministic fixtures
  reject missing percentiles, inverted distributions, impossible sample
  counts, and stale installed binaries.

The user-service interface is:

```bash
./scripts/run_vessel_scale_benchmark.sh start 1800
./scripts/run_vessel_scale_benchmark.sh status
./scripts/run_vessel_scale_benchmark.sh cleanup
```

Instrumentation, capacity, or workload-construction tests passing are not
evidence that the live 25 ms gate itself passes. Record only the terminal
runner result and preserved artifacts as performance evidence.

## Automated-Test Evidence

The July 26, 2026 snapshot recorded 74 of 74 production-linked vessel tests
passing. It also recorded a clean Valgrind run:

```text
ERROR SUMMARY: 0 errors from 0 contexts
All heap blocks were freed -- no leaks are possible
```

The current work was built with GNU C23 and `-Wall -Wextra` on local
development on August 1, 2026. The production-linked root suite passed 268 of 268
tests, including percentile interpolation, interval promotion, CSV/reset
behavior, truncation safety, stale-exit handling, the 500-active-slot and
slot-500 interior boundaries, bounded full-fleet `shiplist summary` output,
three-dimensional navigation, the safe pause after an untraversable automated
step, shared encounters, marginal batch pricing, the deterministic 1,000-trade
simulation, vessel MSDP clearing after going ashore,
exact movement-trail counting after production movement, hunter policy bounds,
HUNTED eligibility, and lifecycle cooldown semantics. The suite also checks
canonical polygon interiors and MariaDB-compatible edge exclusion for
named-water resolution. It poisons a complete affect structure before
initialization and proves that both flag arrays are cleared. The preceding
CMake candidate passed its 251-test production suite plus autorun supervision.
GCC `-fanalyzer` reports no diagnostic for the Phase 15 hunter implementation.
The preceding 246-test candidate passed Memcheck with zero errors and zero
definite, indirect, or possible loss. Its 301,630 still-reachable bytes belong
to process-lifetime spell, command, DG, and profiler registries and are not
presented as a long-horizon leak verdict; Memcheck must be repeated for Phase
15.
`make install` completed for the current local candidate and removed its
root-level `circle`. Isolated provisioner fixtures also passed
the idempotent zone-extension and overlap-rejection paths. Deterministic shell
fixtures cover the compact live-system parser, incomplete sample rejection, a
clean normal-build log, and detection of six representative vessel progress
rows plus five former unconditional wilderness progress rows. They also
reject duplicated epochs and a terminal checkpoint
whose label does not match the requested duration. Deterministic status and
`smaps` fixtures plus a live-process sample verify the detailed-memory sampler;
its validator rejects PID drift, duplicate epochs, impossible memory
relationships, and a missing heap mapping.

The normal Autotools `make test` gate now also runs the vessel memory analyzer,
detailed-memory sampler, and scale-parser regressions. CMake registers the same
three scripts as CTest cases; a clean CMake configuration ran all three
successfully. Automake's release manifest now includes the ferry, scale,
login, memory, and hunter tools plus the complete harbor, vessel documentation,
help SQL, master schema, and Phase 2-15 install/verify/rollback chain. An
August 1 `make dist` archive audit found zero missing documented vessel
acceptance or schema-rehearsal inputs.

After integration and installation on local development, the complete harbor
provisioner passed restart persistence, exact 10-gold fare collection and
restoration, canonical named-water crossing, merchant identity/cargo, and the
same-account captain channel. The Phase 15 Kohdee acceptance then passed in 64
seconds across hunter creation, a hard restart with exact identity
reattachment, pardon cleanup, target preservation, and exact baseline restore.
These are functional component results, not 500-ship performance evidence.

The older vessel-only result remains historical evidence, not a substitute for
rerunning the current root suite. The authoritative workflow is:

```bash
make test
valgrind --tool=memcheck --leak-check=full \
  --show-leak-kinds=definite,indirect \
  --errors-for-leak-kinds=definite,indirect \
  --error-exitcode=99 ./cutest
make install

cd unittests/CuTest
make test-all
```

The root CuTest binary links the production game sources. Older standalone
vessel mirror sources and claims about a separate `test_runner` are obsolete.

## Bounded Stability and Recovery Gate

The stability program uses one bounded ferry observation and one bounded
500-ship observation. Each full task, including setup, recovery checks,
evidence review, and cleanup, is capped at one hour. The ferry gate is
complete; limit the remaining steady scale measurement to 1,800 seconds. The
ferry monitor provides the reusable game-side observation contract:
actual-Kohdee samples capture fleet count, dynamic wilderness occupancy,
mobiles, objects, rooms, allocation lists, live movement trails, buffer
switches, and overflows in `live-system-samples.tsv`. The scale runner
preserves the same fields beside a headered process series. Review each memory
trend for obvious runaway growth, but do not present a bounded window as proof
that a long-horizon leak cannot exist. The candidate has moved high-volume
step/arrival/loop messages behind compiled development diagnostics and exposes
monotonic status counters instead. The installed 500-ship run must confirm
bounded actual log growth within the permitted window.

### August 2 Bounded Ferry Acceptance

Run `20260801T230546Z-160058` is terminal `PASS` at
`/tmp/luminari-vessel-ferry-soak-1000/runs/20260801T230546Z-160058`. It pinned
source `c539a6d59483f44da121260378287cb33094751e`, installed executable SHA-256
`6122ff1fbcac07a7a0188ee248bc6269dc4b5f3e0d18dc1764912cbafd24bccd`, ferry
5, and route 4. The request-to-result task took 2,779 seconds. Its requested
2,700-second window produced 2,740 seconds of observation, 1,476 movement
steps, 246 waypoint arrivals, 62 route completions, 5 actual-Kohdee samples,
46 database samples, and 46 process samples. Fleet count stayed at 6, dynamic
rooms were 6/13/2 of 2,000, rooms stayed at 50,370, and no live sample reported
a buffer overflow. World lists were 1,224/1,520/641 and movement trails were
1,896/388,841/63 across the initial, maximum, and post-restart final checks.

The continuous process series stayed on PID 160111 with two threads and 12
descriptors. RSS was 767,396/866,944/866,944 KiB. Its full-window slope was
128,565 KiB/hour; the trailing 1,797-second slope was 118,710 KiB/hour. Sparse
detailed samples localized the increase to anonymous and heap-backed pages:
anonymous RSS changed from 747,296 to 846,984 KiB and heap RSS from 513,244 to
612,524 KiB. During the same awake-world window mobiles rose from 31,632 to
34,188, objects from 24,201 to 24,856, and active movement trails from 1,896
to 388,841. The generated `memory-analysis.kv` therefore remains
`REPORT_ONLY`; this short correlated warmup is neither a leak attribution nor
a plateau verdict.

At the terminal checkpoint Kohdee paused ferry 5 at its exact coordinates and
route position. A deliberate hard restart changed the MUD to PID 252880,
launched the identical installed executable, restored the coordinates, route,
pilot, schedule, rooms, and structure, and left the ferry paused for
verification. Kohdee then resumed it. There were no copyovers during the
continuous window and no `ferry-errors.log`. This closes the bounded ferry
release gate; the bounded 500-ship profile remains separate.

The first pinned 24-hour ferry attempt is `ABANDONED`, not a failed vessel
continuity result. It remained healthy for 34,382 seconds with 18,720 movement
steps, 780 arrivals at each dock, 10 actual Kohdee checks, 574
database/process samples, one MUD PID, two threads, and 12 descriptors. At
11:00:30 IDT the normal automated copyover preserved the process and ferry but
discarded the unauthenticated monitor descriptor, as the server's copyover
contract requires for non-playing connections. The old monitor treated that
expected handoff as a dead keepalive and did not finalize status. The current
monitor accepts only a log-proven same-PID/same-binary copyover, reconnects
after boot, preserves the recovery log, and records terminal failure before
cleanup. No replacement long-duration window is required; the completed
replacement gate uses the bounded observation and exact-state restart recorded
above.

The August 1 replacement run has a shutdown checkpoint at 1,543 of 86,400
seconds. It retained PID 1803873, two threads, 12 descriptors, the pinned
binary hash, one actual-character sample, 26 database/process samples, and
continued ferry movement. Host shutdown invalidates the continuity window;
these partial observations remain historical rather than a terminal result.
The retired long-duration gate must not be restarted. During this partial
window, unrelated legacy ship 3 followed stale route `persistroute`
into an unoccupiable target, safely persisted speed zero/autopilot pause, and
emitted one `SYSERR`. That severity and development fixture require follow-up;
the monitored ferry was slot 5.

A July 30 forced-copyover shakedown is terminal `PASS` at
`/tmp/luminari-vessel-ferry-soak-1000/runs/20260730T092546Z-1844033`.
Source `823d48b9` ran a 240-second requested window with one same-PID copyover,
132 observed movement steps, 22 waypoint arrivals, five route completions,
four live samples, 25 database/process samples, and zero buffer overflows.
Raw autopilot counters reset across `exec` from 163/27/6 to 60/10/2; the
copyover-aware segment accumulator correctly retained progress. All continuous
process samples stayed on PID 1817030. The final hard restart changed to PID
1859781, launched the same installed SHA-256, recovered the exact paused
coordinates and route, and resumed the ferry. This proves the recovery
harnesses, not duration or bounded memory growth.

The 574 process samples span 34,382 seconds. RSS rose from 768,776 to 1,144,640
KiB and VSZ from 862,208 to 1,237,916 KiB. After excluding the first 14,400
seconds, the 334-sample RSS slope was +5,923 KiB/hour. Consecutive hour-sized
block slopes declined from +11,212 through +8,061, +5,895, +4,868, and +3,943
to +4,065 KiB/hour; trailing 30-minute, one-hour, and two-hour slopes were
+4,007, +3,784, and +3,772 KiB/hour. This is useful partial warmup evidence,
not a plateau, bounded-growth threshold, or duration pass.

`scripts/analyze_vessel_memory_samples.sh` validates either the legacy
headerless ferry process series or the newer headered scale series. It
requires strictly increasing epochs, one constant PID, and six valid numeric
metrics; reports consecutive block means plus full, post-warmup, and
configurable trailing linear RSS/VSZ regressions; and has a stable
`--format kv` mode. Its deterministic test proves an exact zero-slope plateau,
an exact +6,000 KiB/hour rising series, and rejection of PID, timestamp, and
metric corruption. It intentionally returns `REPORT_ONLY` until the two live
observations can be reviewed together. It is not used to create a longer
duration gate. Future ferry and scale runners create `memory-analysis.kv` at
the end of measurement and reject a series the analyzer cannot validate; the
abandoned pinned ferry predates this automatic artifact and remains available
for read-only manual analysis.

Current ferry and scale runs also create `process-memory-details.tsv`. It
records anonymous, file-backed, and shared RSS, data and swap sizes, and the
heap mapping's size, RSS, and private-dirty pages. Ferry samples align with
actual Kohdee checkpoints; scale samples occur at start, every complete
intermediate hour, and end. The terminal validator requires a constant PID,
strictly increasing timestamps, all metrics, and valid RSS/heap
relationships. A heap/status capture against the pinned 1.1 GiB process took
about 0.01 seconds, so scale collection remains sparse. The abandoned pinned
ferry predates this artifact.

For the ferry, that detailed series ends at the final active checkpoint before
the deliberate hard restart. This preserves the validator's one-process
contract. The recovery phase is a separate observation boundary: it requires
a replacement service process, the same installed executable hash, the exact
paused coordinates and route state, and successful resume. It does not append
the replacement PID to the continuous memory series.

At the July 30 08:28 IDT checkpoint, the pinned ferry run had completed 25,202
of 86,400 seconds with 13,716 movement steps, 572 west-dock and 571 east-dock
arrivals, eight actual-character checks, and 421 process samples. The MUD PID,
two threads, and 12 file descriptors were constant. RSS had risen from 768,776
KiB to 1,134,288 KiB. The last 30-minute, 1-hour, and 2-hour linear RSS slopes
were +5,836, +5,899, and +6,825 KiB/hour, respectively. Consecutive
post-four-hour block slopes were +11,212, +8,061, and +5,895 KiB/hour, versus
about +120,000 KiB/hour in the first hour. The rate is strongly decelerating
but is still positive.

A read-only `/proc` snapshot localized 879,932 KiB RSS to the 880,116 KiB heap
mapping. Total anonymous RSS was 1,113,812 KiB, file RSS was 20,576 KiB,
`VmData` was 1,122,932 KiB, and swap remained zero. This establishes where the
retained pages reside, not which subsystem owns them.

Two actual Kohdee `show stats` samples 55 minutes apart provide a useful
correlation. Mobiles increased from 37,128 to 37,329 and objects from 26,286 to
26,429 while rooms remained 50,366 and RSS increased from 1,128,588 to
1,134,400 KiB. At the pinned binary's measured structure sizes, the additional
201 `char_data` and 143 `obj_data` base structures account for about 2,885 KiB
before names, affects, scripts, events, equipment, and other per-instance
allocations. Allocation lists varied downward rather than accumulating, from
1,013 to 897. This makes ordinary world-population churn a material confounder
for the process slope; it does not prove that all retained memory belongs to
mobiles or objects.

At 08:42 IDT, another actual Kohdee checkpoint completed in six seconds. The
fleet remained five, dynamic-room occupancy was 3 of 2,000, rooms remained
50,366, and buffer overflows remained zero. Mobiles had risen by 25 to 37,354,
objects by 10 to 26,439, and RSS by 1,260 KiB to 1,135,660 KiB since 08:28,
while allocation lists fell by 13 to 884. The additional base structures
account for about 354 KiB before their dynamic allocations.

The pinned log also revealed a large awake-world retention set independent of
fleet size. Movement trails remain live for 12,600 seconds and are pruned
every 75 seconds. At the checkpoint, the most recent complete 168-cleanup
window contained 1,314,302 removals, averaging 7,823 per cleanup. The 48-byte
trail structures alone represent at least 60.16 MiB before two duplicated
strings and allocator metadata. Hour-sized cleanup means rose from 7,305
through 7,774 and 7,916 to 8,044 as the world population warmed. This is a
quantified full-world confounder, not proof that trails explain the entire
heap.

The candidate now derives the exact live movement-trail count from room lists
when an immortal issues `show stats`. Future ferry and scale checkpoints
require and preserve that field, and their summaries report its initial,
maximum, and final values. The runners invoke the scan only at infrequent
actual-character checkpoints. Production-linked create/move coverage and
updated shell fixtures pass; the abandoned pinned binary predates this field.

A separate read-only C client repeated the exact ferry-coordinate region and
path queries through `mysql_ping()`, `mysql_query()`, `mysql_store_result()`,
row iteration, and `mysql_free_result()`. Across 20,000 assignment pairs and
40,000 result cycles, RSS moved from 6,000 to 6,008 KiB in the first 1,000
pairs and then remained exactly 6,008 KiB; VSZ remained 9,172 KiB and glibc
allocated bytes stabilized at 130,544. This excludes the direct MariaDB
spatial result lifecycle by itself as the explanation for the game process's
growth. The candidate's single-pass target resolution remains a valid query
and CPU optimization, but is not claimed as a memory fix.

These observations remain a partial warmup investigation from an abandoned
run, not a terminal continuity, plateau, root-cause, or leak verdict. The
remaining bounded fleet gate must demonstrate:

- No crashes, corrupt vessel records, or observed runaway growth during the
  bounded window.
- Stable tick performance and schedule execution.
- Correct copyover and reboot recovery during voyages, combat, and cargo work.
- Safe cleanup after sinking, extraction, player deletion, and generated-room
  lifecycle events.
- Useful diagnostics without production debug spam.

## Current Verdict

The fixed-memory foundation and historical automated-test snapshot are within
their stated budgets. The one-hour-bounded actual-character ferry stability
gate passes. The vessel system is not performance-approved for broad release
until the complete 500-ship benchmark and its bounded stability checks pass.

Remaining benchmark and release work is tracked in
[`VESSELS_TODO.md`](../project-management-zusuk/vessels/VESSELS_TODO.md).

## Related Documentation

- [Vessel Product Requirements](../PRD.md)
- [Vessel System](../systems/VESSEL_SYSTEM.md)
- [Vessel System Testing](VESSEL_SYSTEM_TESTING.md)
- [Vessel Schema Deployment](../deployment/VESSEL_SCHEMA_DEPLOYMENT.md)
- [Unified Vessel Architecture Decision](../adr/0001-unified-vessel-system.md)
