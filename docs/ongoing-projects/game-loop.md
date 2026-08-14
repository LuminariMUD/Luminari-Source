# Game Loop Catch-Up Investigation and Handoff

## Current status

Last updated: 2026-08-14.

The investigation recommendations have been implemented on the published
`game-loop-recommendations` branch. The implementation checkpoint is commit
`d108d980` (`Instrument and bound game-loop recovery`), pushed to
`origin/game-loop-recommendations`. No pull request was opened.

The active worktree is:

```text
/home/aiwithapex/projects/Luminari-Source-game-loop
```

The branch started from development commit
`c3202a5dd80e5da9f07cc550ef509cc1fb9760b0`. The older incident logs reviewed
at the start of the investigation identify their binary as commit
`05c5fea3f30d734020c1fc974e67a2f9c845a51a`.

The work completed so far includes:

1. Bounded callback identity and timing telemetry inside `event_process()`.
2. Queue-depth and callback-creation telemetry for every event-processing pass.
3. Timing and processed-count telemetry for `extract_pending_chars()`.
4. Requested, replayed, discarded, and budget-exhaustion telemetry for every
   slow outer-loop pass.
5. A 100 ms wall-clock budget for heartbeat catch-up instead of the former
   hard limit of 300 heartbeat calls per outer-loop pass.
6. A DG Script wait-event cancellation cleanup required for safe global event
   queue shutdown.
7. Production-linked CuTest coverage for the new telemetry and cleanup paths.
8. Normal-build, full-world reproduction of the original spiral and a
   comparison run with the bounded recovery policy.

The main implementation and tests are committed and pushed. This document is
the handoff for finishing review, longer-duration validation, and any follow-up
optimization work.

If this handoff update has not been committed before the next session, the
expected working-tree change is only:

```text
M docs/ongoing-projects/game-loop.md
```

## Confirmed conclusions

The original failure has two distinct stages:

1. `extract_pending_chars()` creates the initial multi-second stall after the
   world begins ticking. This was previously an inference and is now measured.
2. Unbounded heartbeat replay causes thousands of due event callbacks to run.
   The sustained `event_process()` cost is dominated by a high volume of
   `Casting` callbacks, not by one unusually slow callback.

The old recovery loop could not converge. A replayed heartbeat represented
100 ms of logical time, but affected full-world passes could spend more than
100 ms executing the work associated with each replay. Replaying more pulses
therefore created at least as much new wall-clock delay as it removed.

The new recovery policy preserves the current heartbeat, performs additional
replays only while heartbeat execution remains inside one 100 ms budget, and
discards the unreplayed remainder. This bounds time spent away from socket and
command processing. Logical timers advance only for heartbeats that actually
run.

## Original incident evidence

### Scope of the initial review

The initial read-only investigation used:

- `log/syslog.3`
- `log/valgrind/live-20260814T093421Z/game.log`
- `log/valgrind/live-20260814T100137Z-mini/game.log`
- `src/comm.c`, `src/dgscript/dg_event.c`, and `src/handler.c`

The dedicated `log/performance` file was empty, so the initial conclusions came
from PERFMON reports embedded in the game logs.

### Ordinary non-Valgrind progression

The 11:49 copyover entered a self-reinforcing missed-pulse catch-up spiral. A
loop with a 100 ms target grew from 2.74 seconds to 10.54 seconds, then 29.94
seconds, and finally 75.95 seconds.

| Time | Outer-loop time | Heartbeats executed | `event_process()` time |
| --- | ---: | ---: | ---: |
| 11:49:31 | 2.744 s | 1 | 0.026 s |
| 11:50:37 | 10.545 s | 80 | 10.220 s |
| 11:51:49 | 29.944 s | 161 | 29.047 s |
| 12:31:00 | 75.946 s | 300 | 73.401 s |

At the final high-water mark, `event_process()` consumed about 97 percent of
the outer-loop pass. Its maximum single invocation was 3.705 seconds.

The first explicit missed-pulse warning appeared at 11:52:36. From then until
12:31:00 the log contained 47 warnings, normally reporting 30 to 46 missed
seconds and ending with 76 missed seconds. The reported values summed to 1,707
seconds. That sum indicates repeated severe stalls; it is not a measurement of
unique lost time because the old loop recalculated and clamped recovery on
each pass.

### Why the old loop could not recover

The old `game_loop()` converted elapsed processing time into `missed_pulses`,
clamped a large request to 30 real seconds, and called `heartbeat(++pulse)` for
every replay. At ten pulses per second, the clamp still permitted 300
heartbeat calls in one outer-loop pass.

`heartbeat()` calls `event_process()` on every pulse. In the 75.95-second pass,
300 `event_process()` calls took 73.40 seconds, or about 245 ms per replayed
pulse. Each replay therefore cost more than twice the 100 ms of logical time it
was intended to recover.

### Correlated error activity

Between 11:49 and 12:31, the original log also contained:

- 10,228 `find_obj_by_uid_in_lookup_table` failures for missing object UIDs;
- 6,171 invalid `RoL Command Sentinel` mobile-activity dispatches; and
- repeated Intermud3 failures against `127.0.0.1:8081`.

The missing object lookups suggest stale DG Script object references. The
typed-special errors are generated during mobile activity when a bound
procedure does not support the dispatched event. Both are avoidable work and
synchronous log noise, but neither was proven to be the dominant cost. During
the final 75.95-second pass there were only 86 missing-UID messages and 85
sentinel messages while `event_process()` consumed more than 73 seconds.

These remain separate defects and possible amplifiers.

### Valgrind comparison

The full-world Memcheck run amplified the spiral:

- outer-loop passes reached 122 to 127 seconds;
- `event_process()` consumed about 94 to 96 seconds per 300-heartbeat pass; and
- `mobile_activity()` consumed another 23 to 24 seconds across five calls.

The later minimized run, which also suppressed special-procedure assignment,
did not spiral. It changed both world size and special assignment behavior, so
it did not isolate the cause. The normal-build reproduction described below is
the authoritative confirmation; Valgrind timings are not representative of
normal operation.

## Controlled normal-build reproduction

### Isolation

The reproduction used a private copy of the development `lib` tree, an unused
port (`41999`), and the worktree binary. It did not use `-m` or `-s`, so the
full world and normal special procedures were active. Credential files were
read through the existing development configuration and were not modified.
The source checkout, its `.killscript`, its logs, and the already-running
development server were not used as the test data directory.

The main checkout's `lib/.env` identified the environment as development.
Production code and production configuration were not modified.

### Instrumented old-policy run

The first full-world heartbeat took 3.611186 seconds:

| Measurement | Value |
| --- | ---: |
| `extract_pending_chars()` | 3.597964 s |
| Pending characters before | 193 |
| Characters processed | 193 |
| Pending characters after | 0 |
| `event_process()` | 0.012963 s |
| Event callbacks | 2 `trig_wait_event` calls |

This confirms that pending-character extraction was the initial trigger.

The old replay policy then showed requested missed-pulse counts of 36, 46, 73,
74, and 86, with the trend still increasing. A later 75-heartbeat outer pass
reported:

| Measurement | Value |
| --- | ---: |
| Outer-loop time | 8.108416 s |
| `event_process()` | 7.789088 s |
| Event callbacks processed | 5,275 |
| Events created during processing | 35 |
| Event queue depth | 1,783 to 1,618 |
| `Casting` callback calls | 4,491 |
| `Casting` total time | 7.530550 s |
| `Casting` average time | 1,676.81 us |
| `Casting` maximum time | 13,780 us |

The callback identity data changes the diagnosis from "an unknown expensive
event" to "a very large volume of ordinary `Casting` callbacks." The callback
is `event_casting()` in `src/magic/spell_parser.c`. It normally requeues at a
ten-pulse interval while a cast remains active. Replaying logical time makes
many full-world NPC casting events due in rapid succession.

### Budgeted-policy run

A comparison run used the 100 ms heartbeat recovery budget. Its first
heartbeat independently reproduced the extraction stall:

| Measurement | Value |
| --- | ---: |
| Outer-loop time | 2.714963 s |
| `extract_pending_chars()` | 2.701732 s |
| Characters processed | 177 |
| `event_process()` | 0.012998 s |
| Event callbacks | 2 `trig_wait_event` calls |

The initial 27 missed pulses were cheap enough to replay inside the budget.
During the following approximately 64 seconds with a connected descriptor,
the requested missed count peaked at 20 instead of growing without bound.
When the budget was exhausted, a representative pass replayed 9 missed pulses
and reported 11 as remaining. No post-extraction high-water mark exceeded the
initial multi-second extraction pass.

This is evidence that the wall-clock budget breaks the runaway feedback loop.
It is not evidence that the underlying extraction and casting workloads are
cheap; they remain optimization targets.

## Implemented code

### `src/comm.c`

- Replaced the 300-heartbeat hard clamp with
  `HEARTBEAT_CATCHUP_BUDGET_USEC`, currently equal to `OPT_USEC` (100 ms).
- Always executes the current heartbeat.
- Executes additional requested heartbeats until the request is satisfied or
  the heartbeat budget is exhausted.
- Calculates actual replayed missed pulses only after execution.
- Reports `requested_missed`, `replayed_missed`, `remaining_backlog`, actual
  `heartbeat_calls`, and `budget_exhausted` for every slow pass.
- Records the same values in cumulative PERFMON telemetry.
- Adds sampled profiling sections for `event_process` and
  `extract_pending_chars`.

`remaining_backlog` is diagnostic wording. It is not carried into the next
outer-loop pass; it is deliberately discarded. The next pass calculates a new
missed-pulse request from its own elapsed wall-clock time.

### `src/dgscript/dg_event.c` and `src/dgscript/dg_event.h`

- Added `event_create_named()` and kept `event_create()` as a macro that uses
  the callback function name as its stable identity.
- Stores a PERFMON profile index on each event, avoiding a name lookup on every
  callback invocation.
- Times only the event callback, leaving queue management outside callback
  cost.
- Records callbacks processed, events created during processing, and queue
  depth before and after each `event_process()` call.
- Adds optional cancellation/shutdown cleanup for non-MUD events through
  `event_set_cancel_cleanup()`.

### `src/mud_event.c`

- Registers MUD events with their `mud_event_index` display name, so reports
  show identities such as `Casting`, `Combat Round`, and `Check Occupied`
  rather than the generic dispatch function.
- Handles event-creation failure without dereferencing a null event.

### `src/dgscript/dg_scripts.c`

- Registers specialized cancellation cleanup for `trig_wait_event`.
- The cleanup clears `GET_TRIG_WAIT(trigger)` before freeing the wait-event
  data during global queue teardown.

This was required after the full-world validation exposed a shutdown-only
double free. `event_free_all()` released a queued wait event, but the owning
trigger retained its cached event pointer. Later trigger extraction attempted
to cancel that freed event after the global queue was gone. A GDB backtrace
identified `cleanup_event_obj()` through `event_cancel()` and
`extract_trigger()`. The final full-world connect/shutdown run terminated
cleanly after the specialized cleanup was added.

### `src/handler.c`

- Records pending extraction count before the scan, actual characters
  processed, and unresolved pending count after the scan.
- Sends those counts to PERFMON on every call.

### `src/perfmon.c` and `src/perfmon.h`

- Adds monotonic microsecond timing for event callbacks and recovery budgeting.
- Uses a fixed registry of 512 callback identities with 64-byte names.
- Reports the top 16 callback identities by total time.
- Maintains pulse and cumulative aggregates for callbacks, event queue work,
  pending extractions, and catch-up passes.
- Uses saturating counter addition and a bounded overflow aggregate.
- Sanitizes identity text for CSV safety.
- Extends human-readable pulse and cumulative reports.
- Extends CSV reports with game-loop counters and callback rows.
- Resets the new statistics through the existing PERFMON reset and cleanup
  paths.

No source files were added, so `Makefile.am` and `CMakeLists.txt` did not need
changes.

## How to read the new telemetry

Automatic PERFMON high-water logs include a `game-loop telemetry` section.
The existing staff PERFMON reports also include these aggregates.

Important fields:

- `Event queue calls`: number of `event_process()` invocations.
- `callbacks`: total due callbacks executed.
- `created`: events created while callbacks were being processed.
- `depth`: initial queue depth to latest queue depth for the report interval.
- `max_before` and `max_after`: maximum observed queue depths.
- `Extractions pending_before`, `processed`, and `pending_after`: sums across
  calls in the report interval.
- `max_processed`: largest extraction batch in one call.
- `Catch-up requested_missed`: missed pulses requested by elapsed time.
- `replayed_missed`: missed pulses actually replayed, excluding the current
  heartbeat.
- `remaining_backlog`: requested missed pulses deliberately not replayed.
- `budget_exhausted`: passes stopped by the 100 ms heartbeat budget.
- `Event callbacks`: top identities with call count, total, average, and
  maximum callback time.

The CSV header for callback rows is:

```text
event_identity,calls,total_usec,average_usec,max_usec
```

When diagnosing another stall, compare callback call count and average time.
A high total with a low average indicates amplification through callback
volume, as happened with `Casting`. A high maximum with a low call count points
to an individually expensive callback.

## Recovery semantics and tradeoffs

The recovery change is intentionally a responsiveness policy, not a workload
optimization:

- The current heartbeat cannot be preempted. One expensive heartbeat may still
  exceed 100 ms.
- The budget is checked between heartbeat calls.
- Cheap heartbeats may all be replayed, even if the count is greater than the
  former numeric clamp, as long as they fit inside 100 ms.
- Unreplayed logical time is discarded. Cooldowns and periodic work advance
  only through heartbeat calls that actually execute.
- Event callbacks retain their normal per-pulse semantics. No callback type is
  currently coalesced or skipped selectively.
- Per-callback measurement adds two monotonic clock reads to every callback.
  Memory use remains bounded, but runtime overhead should be observed on long
  full-world runs.
- Every slow pass writes one catch-up log line. Monitor whether that diagnostic
  logging becomes significant during prolonged overload.

The prior policy already discarded requests above its 300-call clamp. The new
policy makes the discard decision based on measured work time so the server
returns to socket processing promptly.

## Validation completed

### Automated and build validation

The following completed successfully on the implementation worktree:

```bash
make -j$(nproc)

LUMINARI_TEST_SKIP_SYNTAX_BOOT=1 \
LUMINARI_TEST_SPEC_WORLD_ROOT="$PWD/unittests/CuTest/fixtures/spec_world_inventory" \
make test

make install
```

Results:

- GNU C23 production build passed with `-Wall -Wextra` and no new warnings.
- Root production-linked CuTest suite passed: `OK (709 tests)`.
- Infrastructure and fixture validation invoked by `make test` passed.
- `make install` installed `bin/circle` and removed the root-level `circle`
  artifact.
- Commit and push hooks passed formatting, whitespace, conflict, large-file,
  and compile checks.
- `git diff --check` passed before the implementation commit.

The root test command skipped its built-in syntax boot because the isolated
worktree intentionally does not contain `lib/.env` or `lib/mysql_config`.
Separately, a normal full-world syntax run in the private development data
copy exited successfully, and several normal full-world active runs booted,
accepted a connection, produced telemetry, and shut down.

### Added test coverage

`unittests/CuTest/test_perfmon_production.c` covers:

- callback identity sanitization and aggregation;
- event queue, extraction, and catch-up counters;
- budget-exhaustion reporting; and
- pulse reset versus cumulative reset behavior.

`unittests/CuTest/test_syntax_check_boot.c` covers:

- real event creation and processing through the production event queue;
- callback identity and queue depth in CSV output; and
- specialized cancellation cleanup during `event_free_all()`.

### Full-world shutdown validation

The final source was booted with a full normal world, connected briefly, and
then stopped by signal. Shutdown completed through `Done.` without the earlier
`queue_deq called with NULL queue` message or allocator abort.

## Worktree and environment notes

- Branch: `game-loop-recommendations`
- Published implementation commit: `d108d980`
- Remote tracking branch: `origin/game-loop-recommendations`
- No pull request exists.
- The primary checkout was not modified by this work.
- The primary development server was not stopped or reused for reproduction.
- The isolated full-world logs and data copy were temporary diagnostic
  artifacts and are not part of the repository.
- The worktree has ignored local copies of `src/campaign.h`,
  `src/mud_options.h`, and `src/vnums.h` created from their examples for the
  fresh build. Do not edit or commit those protected local headers.
- The worktree does not contain credential files. Do not copy or modify the
  primary checkout's `lib/.env` or `lib/mysql_config`; read-only use of the
  configured development credentials was sufficient for the isolated run.
- `lib/world/obj/1699.obj` was provisioned as an ignored runtime artifact so
  `make install` could complete. Do not commit it.
- Tracked shell-script symlinks were restored as symlinks in this worktree
  because the checkout initially materialized their targets as plain text.

Before starting another server, recheck current processes and listening ports.
Do not assume the transient port or process state recorded during this
investigation is still valid.

## Remaining work and recommended next session

### Immediate continuation checklist

1. Enter the existing worktree and inspect its state:

   ```bash
   cd /home/aiwithapex/projects/Luminari-Source-game-loop
   git status -sb
   git log -1 --oneline --decorate
   ```

2. Read this document before changing the recovery semantics. Do not repeat
   the initial blind log investigation; callback identity and extraction data
   have already resolved the diagnostic gap.
3. Run a longer full-world normal-build soak with the final source, ideally
   several minutes with a connected descriptor. Confirm:
   - requested missed pulses remain bounded;
   - budget exhaustion returns control to the outer loop;
   - `remaining_backlog` does not trend upward across passes;
   - queue depth does not grow continuously;
   - the callback registry remains useful and bounded; and
   - shutdown remains clean after the longer run.
4. Review the 100 ms discard policy as a gameplay decision. In particular,
   verify acceptable behavior for casting, combat rounds, action cooldowns,
   spell preparation, vessel events, and other pulse-based timers when logical
   time is discarded under overload.
5. Update the central PERFMON documentation if this branch is prepared for
   integration. The performance section in
   `docs/systems/CORE_SERVER_ARCHITECTURE.md` predates these counters. Also
   inspect `docs/development/perfmon_help.txt` for staff-facing output changes.
6. Rerun the production-linked suite and `make install` after any source
   change. Do not leave a root-level `circle` binary.

### Separate optimization tracks

The recovery guard prevents one overload from monopolizing the outer loop, but
it does not remove the expensive work. Keep these as separate, evidence-driven
tasks:

1. Profile `extract_char_final()` phases for a large initial batch. The current
   evidence proves that processing 177 to 193 pending characters costs about
   2.7 to 3.6 seconds, but it does not identify which extraction sub-operation
   dominates.
2. Determine why thousands of NPC `Casting` events are simultaneously active
   in the full world. The measured average callback is modest; reducing event
   count is more promising than micro-optimizing the callback wrapper.
3. Investigate stale DG object UID references and invalid typed-special
   dispatches as independent correctness and log-amplification defects.
4. If memory analysis is needed, run targeted Valgrind only after choosing one
   of the measured paths. Do not use Memcheck timings as normal performance
   data.

### Review prompts

- `src/perfmon.c` grew beyond 1,000 lines. The repository treats that as a
  review prompt, not a violation. Decide whether the game-loop aggregate code
  remains cohesive there or should become a separate module; adding a source
  file would require both `Makefile.am` and `CMakeLists.txt` updates.
- Confirm that always-on per-callback clock reads are acceptable at full-world
  event rates.
- Confirm that a fixed 512-identity registry and top-16 report are sufficient
  for long-running servers.
- Consider whether catch-up log lines need rate limiting after enough field
  evidence has been collected. The current implementation logs every slow pass
  because that was an explicit diagnostic requirement.
- Do not add callback-specific coalescing until the lifecycle and ordering
  requirements of that event type are documented and tested.

## Files changed by the implementation checkpoint

```text
src/comm.c
src/dgscript/dg_event.c
src/dgscript/dg_event.h
src/dgscript/dg_scripts.c
src/handler.c
src/mud_event.c
src/perfmon.c
src/perfmon.h
unittests/CuTest/test_perfmon_production.c
unittests/CuTest/test_syntax_check_boot.c
```

The original recommendation list is now implemented. Future work should focus
on validating the recovery semantics under realistic load and reducing the two
measured workloads rather than adding more broad instrumentation without a
specific question.
