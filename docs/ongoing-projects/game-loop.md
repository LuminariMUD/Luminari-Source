# game-loop.md - Game Loop Issues

## Investigation: 2026-08-14 slow-loop incident

### Scope

This note records a read-only review of the local development logs. The main
sources were:

- `log/syslog.3`
- `log/valgrind/live-20260814T093421Z/game.log`
- `log/valgrind/live-20260814T100137Z-mini/game.log`
- `src/comm.c`, `src/dgscript/dg_event.c`, and `src/handler.c`

All reviewed runs identify the binary as clean commit
`05c5fea3f30d734020c1fc974e67a2f9c845a51a`. The dedicated
`log/performance` file was empty, so the conclusions below come from the
PERFMON reports embedded in the game logs.

### Summary

The game loop entered a self-reinforcing missed-pulse catch-up spiral after
the 11:49 copyover. A loop with a 100 ms budget grew from 2.74 seconds to
10.54 seconds, then 29.94 seconds, and finally 75.95 seconds. Once the loop
started replaying missed heartbeats, `event_process()` became the dominant
cost and the server could no longer catch up to wall-clock time.

Valgrind made the problem substantially worse, but it was not the root cause.
The same failure was already present in the ordinary non-Valgrind run.

### Ordinary-run evidence

The copyover started at 11:49:04 and the server entered the game loop at
11:49:28. PERFMON then recorded the following progression:

| Time | Outer-loop time | Heartbeats replayed | `event_process()` time |
| --- | ---: | ---: | ---: |
| 11:49:31 | 2.744 s | 1 | 0.026 s |
| 11:50:37 | 10.545 s | 80 | 10.220 s |
| 11:51:49 | 29.944 s | 161 | 29.047 s |
| 12:31:00 | 75.946 s | 300 | 73.401 s |

At the final high-water mark, `event_process()` consumed about 97 percent of
the outer-loop pass. Its maximum single invocation was 3.705 seconds.

The first explicit missed-pulse warning appeared at 11:52:36. From then until
12:31:00 the log contains 47 warnings, normally reporting 30 to 46 missed
seconds and ending with 76 missed seconds. The reported values sum to 1,707
seconds. This sum is an indication of repeated severe stalls, not a precise
measurement of unique lost wall-clock time, because the recovery loop clamps
the amount replayed on each pass.

### Why the loop cannot recover

`game_loop()` converts elapsed processing time into `missed_pulses`, clamps a
large backlog to 30 real seconds, and then calls `heartbeat(++pulse)` once for
every pulse being replayed. With ten pulses per second, the clamp still allows
300 heartbeat calls in one outer-loop pass.

`heartbeat()` calls `event_process()` on every pulse. In the 75.95-second
pass, the 300 `event_process()` calls took 73.40 seconds, or approximately
245 ms per replayed pulse. That is already more than twice the 100 ms being
recovered by each pulse. Consequently, replaying the backlog creates another
backlog and the server remains trapped in catch-up work.

Relevant code locations:

- `src/comm.c`: missed-pulse calculation and the `while (missed_pulses--)`
  replay loop in `game_loop()`
- `src/comm.c`: unconditional `event_process()` call at the start of
  `heartbeat()`
- `src/dgscript/dg_event.c`: due-event loop inside `event_process()`

### Likely initial trigger

The first post-copyover pass took 2.744 seconds, but `event_process()` accounted
for only 25.9 ms. Almost all of the delay was exclusive, currently unlabelled
heartbeat time.

For heartbeat number 1, the periodic modulo-based jobs do not run. After the
profiled `event_process()` call, the remaining unconditional heartbeat work is
`extract_pending_chars()`. This makes `extract_pending_chars()` the leading
candidate for the initial approximately 2.7-second stall. This is a code-trace
inference rather than a directly labelled PERFMON result and should be
confirmed by adding timing and extraction-count telemetry around that call.

After this first stall caused heartbeat replay, `event_process()` became the
clear sustained bottleneck.

### Error activity observed during the incident

Between 11:49 and 12:31 the log also contains:

- 10,228 `find_obj_by_uid_in_lookup_table` failures for missing object UIDs
- 6,171 invalid `RoL Command Sentinel` mobile-activity dispatches
- repeated Intermud3 connection failures against `127.0.0.1:8081`

The missing object lookups strongly suggest stale DG Script object references.
The typed-special errors are generated during mobile activity because the
bound procedure does not support the dispatched event. Both create avoidable
work and substantial synchronous log noise.

These messages correlate with the affected full-world run, but the current
profiling is not detailed enough to prove that either message class is the
main cost inside `event_process()`. During the final 75.95-second pass there
were only 86 missing-UID messages and 85 sentinel messages, while
`event_process()` consumed more than 73 seconds. They should therefore be
treated as defects and possible amplifiers, not as the demonstrated primary
cause of that pass.

### Valgrind comparison

The full-world Valgrind run reproduced and amplified the spiral:

- Outer-loop passes reached 122 to 127 seconds.
- `event_process()` consumed roughly 94 to 96 seconds per 300-heartbeat pass.
- `mobile_activity()` consumed a further 23 to 24 seconds across five calls.

This overhead is expected under Memcheck, so those absolute timings are not
representative of production performance. They do reinforce the identification
of event processing and full-world mobile activity as the expensive paths.

The later minimized run, which also suppressed special-procedure assignment,
did not enter the catch-up spiral. Its only PERFMON warning was a 367.8 ms
command-processing pass while loading a character; heartbeat and event work
were negligible. Because that run changed both world size and special
assignment behavior, it does not isolate which change removed the problem.

### Current diagnostic gap

PERFMON measures `event_process()` as one inclusive section. It does not record:

- which event callbacks ran;
- callback invocation counts and individual durations;
- event queue depth before and after a pass;
- how many events were newly queued while catch-up was running; or
- which callback owns the stale object UID lookups.

The logs therefore identify the bottleneck boundary but cannot identify the
specific event type or callback responsible for most of the time.

### Recommended next investigation

1. Add bounded per-callback telemetry inside `event_process()`: callback/event
   identity, call count, total time, maximum time, and queue depth before and
   after processing.
2. Add a profiling section around `extract_pending_chars()` and record how many
   pending characters it processes per call.
3. Record requested missed pulses, replayed pulses, and remaining backlog for
   every slow outer-loop pass.
4. Reproduce with the full world in a normal build before using Valgrind, then
   use Valgrind only after the dominant callback is known.
5. Once event semantics are understood, evaluate a bounded recovery policy
   that coalesces eligible periodic work or limits heavyweight catch-up work by
   wall-clock budget instead of replaying as many as 300 heartbeats in a single
   pass.

No code or runtime configuration changes were made during this investigation.
