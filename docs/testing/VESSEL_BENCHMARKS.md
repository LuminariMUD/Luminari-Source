# Vessel System Benchmarks

**Version:** 3.1

**Evidence snapshot:** July 30, 2026

**Last updated:** July 30, 2026

This document records measured vessel-system evidence and the remaining release
performance gate. It intentionally separates completed foundation measurements
from the full live-game benchmark that still must be run.

## Evidence Summary

| Measure | Result | Status |
|---|---:|---|
| Configured fleet-array slots | 500 | Slot 0 is reserved; current active maximum is 499 |
| Base `greyhawk_ship_data` size | 4,744 bytes | Within 5 KB budget |
| Base storage for 500 array entries | 2,372,000 bytes (about 2.26 MiB) | Within about 3 MB budget |
| Production-linked vessel test gate on July 26, 2026 | 74 of 74 passing | Historical snapshot |
| Valgrind result for that test gate | 0 errors, 0 leaks | Historical snapshot |
| Root suite on July 30, 2026 | 227 of 227 passing | Current instrumentation gate |
| Complete 500-ship live tick | Not yet measured | Release blocker |

The release target is a complete vessel tick at or below 25 ms with 500 active
ships and the production gameplay workload enabled. Navigation-only
microbenchmarks do not satisfy this target.

## Memory Measurements

### Ship Structure

The measured `sizeof(struct greyhawk_ship_data)` is 4,744 bytes.

| Component | Approximate bytes |
|---|---:|
| Ten ship slots and description arrays | 2,680 |
| Connection data | 640 |
| Sail-crew and gun-crew data | 518 |
| Room arrays | 160 |
| Helm permits | 210 |
| Cargo data | 80 |
| Crew tiers | 16 |
| New counters and state fields | 35 |
| Other fields and padding | 405 |
| **Total** | **4,744** |

Gameplay work added during phases 4 through 9 accounts for roughly 340 bytes,
or about 7.7 percent of the structure. Older documentation that reported a
1,016-byte ship structure is obsolete.

### Fleet Projection

| Ships | Base bytes | Approximate size |
|---:|---:|---:|
| 100 | 474,400 | 463.3 KiB |
| 250 | 1,186,000 | 1.13 MiB |
| 500 | 2,372,000 | 2.26 MiB |

The separate vehicle array has a measured element size of 152 bytes. At 1,000
vehicles, its base storage is 152,000 bytes, or about 148.4 KiB.

Optional allocations, strings, routes, encounter data, and database result
buffers add runtime memory beyond these base arrays. Those allocations must be
included in soak-test observation, but the fixed fleet array remains within its
budget.

### Supporting Structure Sizes

| Structure | Size |
|---|---:|
| `greyhawk_ship_data` | 4,744 bytes |
| `vehicle_data` | 152 bytes |
| Autopilot state | 48 bytes |
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
- `perfmon reset` starts a new pulse, section, and process-wide SQL execution
  window. The SQL counter includes direct, safe, and pooled `mysql_query()`
  attempts plus prepared-statement executions.
- `perfmon csv` emits machine-readable rows for the sampled vessel sections,
  including calls, total, average, median, p95, p99, maximum, samples stored,
  and samples seen. It ends with `# database_queries=<count>`. `perfmon prof`
  remains the human-readable view of every section.
- Interval promotion now occurs once per completed lower-level buffer, and an
  hourly maximum is derived from the completed minute buffer instead of the
  prior hour buffer.

Use this collection sequence:

1. Start the representative workload and allow caches, sections, and database
   connections to warm up.
2. Run `perfmon reset`.
3. Hold the steady workload for at least 75 seconds so `vessel_schedules` runs
   at least once.
4. Run `perfmon csv` and capture the complete output with process memory,
   missed-heartbeat, slow-query, and message-throttling evidence.

At two complete vessel ticks per second, the 16,384-sample window covers about
2 hours 16 minutes. Longer runs remain valid rolling-window observations, but
the reported `samples_stored` and `samples_seen` values must be preserved so
the evidence is not mistaken for a full-run distribution.

Two prerequisites remain before collecting the release result. The current
500-entry array reserves slot 0, leaving only 499 active slots, so capacity
must be corrected to support 500 simultaneously active ships. A reproducible
development-only workload must then populate all 500 ships with the complete
gameplay mix above. The instrumentation passing its tests is not evidence that
the live 25 ms gate itself passes.

## Automated-Test Evidence

The July 26, 2026 snapshot recorded 74 of 74 production-linked vessel tests
passing. It also recorded a clean Valgrind run:

```text
ERROR SUMMARY: 0 errors from 0 contexts
All heap blocks were freed -- no leaks are possible
```

The instrumentation work was built with GNU C23 and `-Wall -Wextra` in an
isolated worktree on July 30, 2026. The production-linked root suite passed
227 of 227 tests, including percentile interpolation, interval promotion,
CSV/reset behavior, truncation safety, and stale-exit handling. `make install`
completed in that isolated worktree and removed its root-level `circle`.

The older vessel-only result remains historical evidence, not a substitute for
rerunning the current root suite. The authoritative workflow is:

```bash
make test
make install

cd unittests/CuTest
make test-all
```

The root CuTest binary links the production game sources. Older standalone
vessel mirror sources and claims about a separate `test_runner` are obsolete.

## Soak and Recovery Gate

After the benchmark passes, run a 72-hour development soak with scheduled NPC
fleets and representative player activity. The soak must demonstrate:

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
