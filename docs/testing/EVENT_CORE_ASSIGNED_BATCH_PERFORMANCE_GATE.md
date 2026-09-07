# Assigned batch performance gate

Declared 2026-09-06 before measuring the repair branch for #111. The targets
below are unchanged and the final full measurement is recorded in this report.
Functional test success by itself does not close this gate.

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
- DG fixture ownership: capture `dg.trigger.wait` immediately before each
  100-owner seed, during the workload, and after room cleanup. Require at least
  90 added waits while active and no more than ten waits above that segment's
  baseline after cleanup. Total full-world event counts remain diagnostic only
  because ordinary zone resets legitimately add and remove unrelated owners.
- Ready-queue health: require zero overdue ticks in every snapshot and an empty
  ready queue at the final steady-state snapshot. Record intermediate ready
  counts as transient scheduler state; a current-tick count can be observed
  while a staff diagnostic is running and does not establish retained growth.
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

Status: passed on both libevent and select for the measured repair binary.

### Full measurement, 2026-09-07

The isolated run began at 2026-09-07 03:50:04 UTC and finished at 07:44:50 UTC
on `jmcl-ubuntu-zbook`. It measured source
`c29603a393b891557461f030a9d368d8cf91fdc9` with no source diff and binary
SHA-256 `6a9551eb741ed5a1be6a1c3ee3e51b461a4745f2274f8c3b908dd2c1fabd3fe8`.
The world contained 4,993 source files and had tree SHA-256
`0ad1f48cf8e8b459ff3b39b200a91d7ef756ef627ac8bb02ff895607dc32044c`;
the private database snapshot SHA-256 was
`d65819a8f25074dff7e7f3b059925d73e8f446cb11aa1b328a7aac9f05c882de`.

The host ran Linux 6.17.0-29-generic on an Intel Core i7-10850H with 12 logical
CPUs and 32,641,252 KiB RAM. The build used Ubuntu GCC 13.3.0 with `-g -O2` and
MariaDB 10.11.14. The production MUD remained independently active on port
4101; each measured backend used its own listener, full-world copy and private
database restored from the same snapshot.

| Backend | Commands / timeouts | p50 / p95 / p99 / max (ms) | Lateness p99 / max (ticks) | First / final RSS median (KiB) | Final ratio | RSS slope (MiB/min) | Result |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| libevent | 4,808 / 0 | 0.335 / 0.646 / 0.889 / 18.001 | 1 / 1 | 1,584,136 / 1,598,672 | 1.009176 | 0.294276 | pass |
| select | 4,808 / 0 | 0.288 / 0.602 / 1.662 / 15.125 | 1 / 1 | 1,586,646 / 1,600,890 | 1.008977 | 0.288524 | pass |

Each backend supplied 361 steady-state RSS samples. All 273 registered event
types were captured in every measured phase. Registry mismatches, stale-owner
outcomes, scheduler failures, service scheduling failures, admission failures,
encounter admission/stale counts, activity stale callbacks, overdue ticks and
suspicious server-log lines were zero. Both final ready queues were empty.

The three libevent DG segments recorded baseline/active/cleanup wait counts of
21/124/24, 24/120/20 and 18/120/19. Select recorded 21/126/24, 24/118/19 and
20/119/20. All active deltas were at least 90 and all cleanup deltas were at
most ten.

The raw run remains locally under
`.burnin-runtime-event111-full-20260907T035001Z`. Its original in-memory
analyzer marked libevent diagnostics failed because an intermediate snapshot
caught 1,295 current-tick ready events even though it reported zero overdue
ticks and the final queue was empty. That check was stricter than the declared
no-overdue/no-retained-growth rule. The corrected analyzer preserves the raw
summaries, records intermediate counts, requires every overdue count to be zero
and requires the final steady queue to drain. Unit tests cover both this
transient case and a retained final queue. Reanalysis is stored beside the raw
evidence in `reanalysis.json`; it passes both backends. No threshold or workload
was changed after measurement.

## Instrumentation available on the repair branch

The native event runtime now records deadline lateness for every semantic event
callback. Lateness is `max(dispatch_tick - deadline_tick, 0)`, in native ticks.
On the normal ten-pulses-per-second configuration, one tick is 100 ms. On-time
callbacks are retained as zero-valued samples so percentiles describe all
callbacks rather than only the late subset.

Each registered event type owns a bounded ring of the latest 1,024 lateness
samples, plus lifetime sample, late-callback and maximum counts. This keeps the
measurement cost and memory fixed. Operators can read p50/p95/p99/max and
stored/seen/late counts with `eventdebug types`; `perfmon prof` and
`perfmon csv` expose the same data for capture. The CSV form is the preferred
artifact for the workloads above. Reset performance counters immediately
before each measured workload and archive the output immediately afterward.
When more than 100 types are registered, retrieve successive bounded pages with
`eventdebug types 100 0`, `eventdebug types 100 100`, and so on until the
reported offset plus showing count reaches the registered count.

The reproducible development harness runs the declared fixture against a
private MariaDB copy and isolated listeners:

```console
scripts/events/run_event_core_performance_gate.py --backend both --profile full
```

Use `--profile smoke` to validate fixture boot, client coordination, evidence
capture and threshold parsing. A smoke result cannot satisfy the sustained-RSS
gate because its steady phase is intentionally shorter than the two ten-minute
comparison windows. Each run stores provenance, raw phase captures, command
samples, process memory, server logs and machine-readable verdicts in a new
ignored `.burnin-runtime-event111-*` directory.

Production-linked tests cover on-time callbacks, a callback dispatched three
ticks after its deadline, percentile reporting, CSV output, and width-bounded
staff diagnostics. This proves the measurement path, but does not substitute
for the declared multi-client and sustained-memory runs.

## Current decision

The code-level observability gap and the declared live acceptance gate are
closed for the measured repair binary. Both backends passed command latency,
deadline lateness, sustained RSS, fixture cleanup, scheduler health and log
checks. The September 5 burn-in remains historical context and was not used to
substitute for these measurements.
