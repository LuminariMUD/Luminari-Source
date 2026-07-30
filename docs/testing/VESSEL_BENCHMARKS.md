# Vessel System Benchmarks

**Version:** 3.3

**Evidence snapshot:** July 30, 2026

**Last updated:** July 30, 2026

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
| Root suite on July 30, 2026 | 245 of 245 passing | Current production-linked gate |
| Current-suite Memcheck | 0 errors; 0 definite, indirect, or possible loss | Pre-soak gate |
| Complete 500-ship live tick | Not yet measured | Release blocker |

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

- Section timings use a monotonic clock. The ten explicitly sampled vessel
  benchmark sections each retain up to 16,384 rolling microsecond samples.
  Sampling is opt-in so ordinary command and special-function sections cannot
  accumulate unbounded profiler memory. The ten windows use about 1.25 MiB.
- The complete half-second vessel group is recorded as `vessel_tick`.
  Its separately attributed children are `vessel_autopilot`, `vessel_combat`,
  `vessel_crew_wages`, `vessel_upkeep`, `vessel_trade`, `vessel_weather`,
  `vessel_encounters`, and `vessel_msdp`.
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
workload and evidence contract. It remains unexecuted while the definitive
24-hour ferry soak owns the local development process, so its presence is not
evidence that the live 25 ms gate passes.

The active ferry run is pinned to an earlier executable, so its continuity
result cannot validate the single-pass target-resolution change. The
post-soak installed-build scale run is the first live performance evidence for
that optimization.

The runner:

- Refuses non-development configuration, an active ferry soak, a dirty source
  worktree, stale benchmark markers, and an installed binary without both the
  500-slot capacity and compact `shiplist summary` behavior.
- Atomically snapshots the 17 vessel, trade, freight, bounty, encounter, and
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
- Opens a native MSDP connection as Kohdee, requests all nine `SHIP_*`
  variables aboard the benchmark airship, compares position and navigation
  values with `shipstatus`, validates hull and status values, then goes ashore
  and requires the complete empty state. The raw check result is preserved as
  `native-msdp-vessel-state.log`.
- Warms the production heartbeat, then pauses ten routed vessels after a known
  schedule departure. Each must trigger a new persisted departure during the
  measured window. Kohdee remains aboard an airship so native MSDP and normal
  player-facing message generation execute.
- Runs for at least 600 steady seconds and records RSS, VSZ, threads, file
  descriptors, PID, and installed-executable identity every 30 seconds. The
  held Kohdee session records timestamped fleet, dynamic-room,
  world-allocation, and buffer statistics at the start, every hour, and at the
  end. The runner rejects fleet or capacity drift, occupancy above capacity,
  buffer overflows, vessel errors, PID/binary drift, or a missing system
  sample. It also requires a `system-0` checkpoint, strictly increasing epochs
  and labels, hourly intermediate labels, and an exact terminal-duration
  label.
- Uses monotonic per-vessel movement, waypoint-arrival, and route-completion
  counters for continuity evidence. Normal builds no longer emit a server-log
  line for every movement step, arrival, wait completion, or loop; those
  messages remain available only in focused development-debug builds. The
  runner preserves the measured log, reports its byte count, and fails if any
  old unconditional or compiled movement/autopilot progress row appears.
- Captures a byte-bounded log slice around the workload reconstruction and
  requires the 500-vessel boot success only in that slice. Prior success or
  error lines in the shared development log cannot create a false verdict.
- Captures all ten sampled vessel profiler rows plus missed-pulse,
  message-throttling, and SQL counters, and requires every selected schedule
  to fire. The reciprocal submarine pair's synchronized reloads produce a nonzero
  `vessel_messages_throttled` count, proving the installed production tick
  suppressed repeated player-facing messages. The complete `vessel_tick`
  maximum must be no more than 25,000 microseconds.

The user-service interface is:

```bash
./scripts/run_vessel_scale_benchmark.sh start
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

The current work was built with GNU C23 and `-Wall -Wextra` in an isolated
worktree on July 30, 2026. The production-linked root suite passed 245 of 245
tests, including percentile interpolation, interval promotion, CSV/reset
behavior, truncation safety, stale-exit handling, the 500-active-slot and
slot-500 interior boundaries, bounded full-fleet `shiplist summary` output,
three-dimensional navigation, shared encounters, marginal batch pricing, the
deterministic 1,000-trade simulation, and vessel MSDP clearing after going
ashore. The suite also checks canonical polygon interiors and MariaDB-compatible
edge exclusion for named-water resolution. It also poisons a complete affect
structure before initialization and proves that both flag arrays are cleared.
The same binary passes Memcheck with zero errors and zero definite, indirect,
or possible loss. Its 301,630 still-reachable bytes belong to process-lifetime
spell, command, DG, and profiler registries and are not presented as a
72-hour leak verdict. `make install` completed in that isolated worktree and
removed its root-level `circle`. Isolated provisioner fixtures also passed the
idempotent zone-extension and overlap-rejection paths. Deterministic shell
fixtures cover the compact live-system parser, incomplete sample rejection, a
clean normal-build log, and detection of all six representative high-volume
progress rows. They also reject duplicated epochs and a terminal checkpoint
whose label does not match the requested duration.

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

## Soak and Recovery Gate

After the benchmark passes, run a 72-hour development soak with scheduled NPC
fleets and representative player activity. The ferry monitor now provides a
reusable game-side observation contract: actual-Kohdee samples capture fleet
count, dynamic wilderness occupancy, mobiles, objects, rooms, allocation
lists, buffer switches, and overflows in `live-system-samples.tsv`. The
scale runner now preserves the same fields beside a headered process series,
but its supported window remains capped at 7,200 seconds. The 72-hour fleet
gate must retain equivalent samples and enforce a documented post-warmup
bounded-growth threshold; an initial/maximum/final RSS tuple alone is not a
leak verdict. The candidate has moved high-volume step/arrival/loop messages
behind compiled development diagnostics and exposes monotonic status counters
instead. The default installed 500-ship run must still confirm bounded actual
log growth before the 72-hour ceiling is lifted.

`scripts/analyze_vessel_memory_samples.sh` validates either the legacy
headerless ferry process series or the newer headered scale series. It
requires strictly increasing epochs, one constant PID, and six valid numeric
metrics; reports consecutive block means plus full, post-warmup, and
configurable trailing linear RSS/VSZ regressions; and has a stable
`--format kv` mode. Its deterministic test proves an exact zero-slope plateau,
an exact +6,000 KiB/hour rising series, and rejection of PID, timestamp, and
metric corruption. It intentionally returns `REPORT_ONLY` until the two live
observations support a defensible 72-hour criterion. Future ferry and scale
runners create `memory-analysis.kv` at the end of measurement and reject a
series the analyzer cannot validate; the active pinned ferry predates this
automatic artifact and remains available for read-only manual analysis.

At the July 30 07:55 IDT checkpoint, the pinned ferry run had completed 23,302
of 86,400 seconds with 12,671 movement steps, 528 west-dock and 527 east-dock
arrivals, seven actual-character checks, and 389 process samples. The MUD PID,
two threads, and 12 file descriptors were constant. RSS had risen from 768,776
KiB to 1,131,248 KiB. The last 30-minute, 1-hour, and 2-hour linear RSS slopes
were +6,891, +6,673, and +7,868 KiB/hour, respectively, versus +120,293
KiB/hour in the first block. The rate is strongly decelerating but is still
positive; this remains an in-progress warmup observation, not a terminal
continuity, plateau, or leak verdict. The soak must demonstrate:

- No crashes, leaks, unbounded growth, or corrupt vessel records.
- Stable tick performance and schedule execution.
- Correct copyover and reboot recovery during voyages, combat, and cargo work.
- Safe cleanup after sinking, extraction, player deletion, and generated-room
  lifecycle events.
- Useful diagnostics without production debug spam.

## Current Verdict

The fixed-memory foundation and historical automated-test snapshot are within
their stated budgets. The vessel system is not performance-approved for broad
release until the complete 500-ship benchmark and 72-hour soak both pass.

Remaining benchmark and release work is tracked in
[`VESSELS_TODO.md`](../project-management-zusuk/vessels/VESSELS_TODO.md).

## Related Documentation

- [Vessel Product Requirements](../PRD.md)
- [Vessel System](../systems/VESSEL_SYSTEM.md)
- [Vessel System Testing](VESSEL_SYSTEM_TESTING.md)
- [Vessel Schema Deployment](../deployment/VESSEL_SCHEMA_DEPLOYMENT.md)
- [Unified Vessel Architecture Decision](../adr/0001-unified-vessel-system.md)
