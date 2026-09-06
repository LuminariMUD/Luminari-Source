# Assigned batch performance gate

Declared 2026-09-06 before measuring the repair branch. Tracks #111. These are
acceptance targets, not measured results. Functional test success does not
close this gate.

## Workloads and provenance

Use an isolated development instance with the full retrieved world, the native
scheduler, and the same compiler, configuration and database snapshot for each
comparison. Record source SHA plus dirty-diff SHA, binary SHA, world archive
SHA, compiler flags, OS/kernel, CPU/RAM, I/O driver and UTC start/end. Redact
credentials and player data. Record competing host build/test load; comparisons
made while compiling cannot establish idle-server latency targets.

Measure both select and libevent. After five minutes of warmup, collect:

1. Ten minutes idle with the full loaded world and two connected test players.
2. Ten minutes with eight clients issuing a bounded mix of look, score and
   movement at one command per client per second, including offscreen combat,
   room scripts, casting interruptions and object transfers.
3. Thirty minutes of repeated spawn, DG wait, interruption and extraction
   cycles with a fixed script fixture and recorded seed. Use 100 active test
   owners, replacing extracted owners at a bounded rate of ten per second.
4. Sixty minutes steady state after workload three, recording memory every ten
   seconds. Repeat the DG/extraction workload three times from the same clean
   process state to distinguish retention from allocator warmup.

## Acceptance thresholds

- No crashes, registry mismatch, failed callback, unexpected admission failure,
  stale-owner execution or monotonic ready-queue growth.
- Scheduler deadline lateness: p99 at most one native tick; maximum at most ten
  ticks during steady state. Record lateness magnitude and the number of
  callbacks, not only an aggregate late-callback count.
- Local command round-trip latency: p95 <= 150 ms, p99 <= 300 ms, maximum <= 1 s
  for the bounded workload. Time from sending a complete command to its matched
  response/prompt with a monotonic clock; do not mix asynchronous output or
  authentication into command samples. Record every timeout separately.
- Steady-state RSS: after warmup, final ten-minute median no more than 2% above
  the first ten-minute median and fitted growth no more than 1 MiB/minute.
  Require owner/event/registry counts to return to their expected baseline.
  RSS alone cannot distinguish a live-object leak from allocator retention.
- Copyover and intentionally blocked diagnostics are separately labelled and
  excluded from steady-state percentiles; their actual pauses remain reported.

These local thresholds do not promise WAN latency. The ready-action diagnostic
holds at most 1,024 samples and measures only its own decision path. It is not
an end-to-end network SLA or a substitute for scheduler deadline measurements.

## Evidence and decision

Retain raw timestamped command samples, process memory and event snapshots,
per-workload summaries and a description of any exclusions. Any missed target
keeps performance approval qualified. Explain outliers; do not increase a
threshold after seeing a failure. A threshold revision requires a new declared
workload and another measurement.

Status: measurement pending on the final repair revision. The prior 2026-09-05
report remains historical evidence, not acceptance of this branch.
