# Game Loop Catch-Up Recovery

## Status

Last updated: 2026-08-14.

The investigation, recovery guard, measured workload reductions, documentation, and validation are
complete on `game-loop-recommendations`. There is no remaining implementation item in this project
note.

The branch was synchronized with `master` before the completion work. Merge commit `db397c3f`
combined the game-loop event profiling with master's event cancellation cleanup contract. The
published pre-merge documentation checkpoint is `91500956`, and the safety reference is
`backup/pre-master-sync-2026-08-14-160309`.

The completed result includes:

1. A 100 ms wall-clock budget for missed-heartbeat recovery.
2. Bounded event callback identity, timing, queue-depth, extraction, and catch-up telemetry.
3. Safe event-specific cancellation cleanup, including DG Script wait events.
4. Stable mobile-activity sharding that preserves each NPC's six-second cadence without running the
   whole population on one pulse.
5. One-pass cross-character reference cleanup for pending extraction batches.
6. Five-second aggregation for catch-up log messages while retaining every pass in counters.
7. Explicit event registry capacity, report-limit, overflow, and catch-up maximum metadata.
8. Updated central and staff-facing PERFMON documentation.

## Confirmed failure mechanism

The original incident had two stages:

1. A large `extract_pending_chars()` batch created an initial multi-second stall.
2. The former recovery loop replayed up to 300 heartbeats in one outer-loop pass. Every heartbeat
   called `event_process()`, and the replay made thousands of NPC `Casting` callbacks due together.

The old loop could not converge. One heartbeat represents 100 ms of logical time, but affected
replayed heartbeats spent more than 100 ms doing the resulting work. Replay therefore created at
least as much new wall-clock delay as it removed.

### Original incident evidence

The ordinary 11:49 development run grew as follows:

| Time | Outer-loop time | Heartbeats | `event_process()` time |
| --- | ---: | ---: | ---: |
| 11:49:31 | 2.744 s | 1 | 0.026 s |
| 11:50:37 | 10.545 s | 80 | 10.220 s |
| 11:51:49 | 29.944 s | 161 | 29.047 s |
| 12:31:00 | 75.946 s | 300 | 73.401 s |

At the final high-water mark, `event_process()` consumed about 97 percent of the pass. Its 300
invocations averaged about 245 ms, more than twice the logical time recovered by each pulse.

A controlled normal-build run then identified callback volume rather than one anomalous callback:

| Measurement | Value |
| --- | ---: |
| Outer-loop time | 8.108416 s |
| `event_process()` | 7.789088 s |
| Event callbacks | 5,275 |
| Queue depth | 1,783 to 1,618 |
| `Casting` calls | 4,491 |
| `Casting` total | 7.530550 s |
| `Casting` average | 1,676.81 us |
| `Casting` maximum | 13,780 us |

Full-world Memcheck runs amplified the same behavior and were useful for correctness checks, but
their timings are not representative of normal operation.

## Implemented recovery and telemetry

### Bounded heartbeat recovery

`game_loop()` always executes the current heartbeat. It replays additional requested heartbeats only
while the batch remains inside `HEARTBEAT_CATCHUP_BUDGET_USEC`, currently one normal 100 ms pulse
interval. It then reports the actual replayed count and deliberately discards any remainder.

The remainder is called `remaining_backlog` for diagnosis, but it is not carried into the next pass.
The next pass derives its own request from elapsed wall-clock time. This prevents a slow subsystem
from monopolizing socket and command processing indefinitely.

Catch-up log output is aggregated into at most one line per five seconds. Each line includes window
pass count, budget exhaustions, maximum request and remainder, and the latest pass. PERFMON counters
continue to record every slow pass.

### Event and extraction telemetry

Events register a stable callback identity when created. Callback execution records two monotonic
clock readings without doing a per-call name lookup. MUD events use display identities such as
`Casting`, `Combat Round`, and `Spell Preparation`; direct callbacks use their function identity.

Reports include:

- event queue calls, callbacks, events created during processing, and queue depth;
- extraction calls, pending count, processed count, and maximum batch;
- catch-up requests, replays, discarded remainder, budget exhaustion, and maxima;
- the top 16 callback identities by total execution time; and
- callback registry use out of 512 entries plus unregistered overflow calls.

The registry is fixed and callback names are bounded, so memory cannot grow with uptime. The top-16
view remains sufficient for diagnosis because reports now show whether omitted identities or
overflow could affect interpretation.

### Event cancellation cleanup

The master merge introduced cleanup callbacks that receive the owning `struct event *`. The merged
implementation retains named callback profiling and this cleanup contract. DG Script wait-event
cleanup clears `GET_TRIG_WAIT()` only when it still points to the event being cancelled, preventing
a stale owner pointer and shutdown double free.

### Extraction batch optimization

Phase telemetry split `extract_char_final()` into:

- `extract.last_attacker`
- `extract.relationships`
- `extract.assets`
- `extract.combat_refs`
- `extract.world_remove`
- `extract.events`
- `extract.finalize`

In a 213-character full-world batch, `extract_pending_chars()` took 1.479350 seconds. Three repeated
cross-character scans accounted for almost all of it:

| Phase | Time |
| --- | ---: |
| `extract.last_attacker` | 553.854 ms |
| `extract.relationships` | 431.336 ms |
| `extract.combat_refs` | 457.973 ms |
| All other measured phases | 34.737 ms |

Those scans made a batch quadratic in the live character population. The final implementation
clears pending targets from `last_attacker`, guarding, hunting, and combat pointers in one prepass,
then finalizes each character. Dead-player NPC memory cleanup retains its established ID-based path.
Production-linked coverage verifies that two simultaneous targets are removed and an observer's
cross-character references and combat state are cleared.

### NPC casting burst reduction

The source trace found why thousands of `Casting` events became active together:

- `mobile_activity()` ran the entire mobile population on one `PULSE_MOBILE` boundary every six
  seconds.
- Idle NPC casters have a 1-in-16 prebuff opportunity during that pass; combat casters may also cast.
- NPC casting starts an `eCASTING` event after two seconds, and active casting requeues on pulse
  intervals.

This synchronized both cast creation and later callback deadlines. `mobile_activity_pulse()` now
hashes each stable character address into one of the 60 mobile pulses and processes only that shard.
Every continuously live NPC is still visited exactly once per six executed seconds. The legacy full
pass remains available to focused callers and tests.

## Gameplay semantics decision

The recovery policy advances `pulse` only for heartbeats that actually execute. This produces one
consistent rule across pulse-driven systems:

- `event_process()` compares all event deadlines with the same executed `pulse` value;
- casting and combat rounds remain ordinary queued events;
- action and daily-use cooldowns remain queued events;
- spell preparation continues to requeue at its established half-second or one-second delay; and
- vessel movement, combat, events, upkeep, trade, weather, encounter, and schedule functions retain
  their existing heartbeat gates.

Discarded logical time therefore delays all of these systems in wall-clock time without reordering
one callback type relative to another. This is accepted because keeping descriptors and commands
responsive is safer than trying to simulate an unbounded period while the server is already
overloaded. No callback-specific coalescing or skipping was added.

## Full-world validation

All active runs used a private copy of the development `lib` tree, isolated game and terrain ports,
normal special procedures, and the full world. Existing development credential files were read but
not modified. The primary development process and checkout were not stopped or used as the test
runtime.

### Post-merge baseline

The synchronized branch without mobile sharding ran an active heartbeat for about 100 seconds:

| Measurement | Value |
| --- | ---: |
| Catch-up log lines | 55 |
| Budget-exhausted passes | 42 |
| Maximum requested missed pulses | 47 |
| Maximum remaining backlog | 30 |
| Initial extraction | 177 characters, 3.941432 s |
| Process CPU while overloaded | about 73 percent |

The remainder repeatedly returned to zero, so the 100 ms guard preserved responsiveness, but the
synchronized casting workload continued to produce new slow passes.

### Sharded five-minute comparison

The first five-minute connected run with mobile sharding and final telemetry showed:

| Measurement | Value |
| --- | ---: |
| Initial extraction | 213 characters, 1.479350 s |
| Initial event queue | 78 to 76 |
| Callback registry | 7/512, zero overflow |
| Maximum requested missed pulses | 15 initial; 8 after extraction |
| Maximum remaining backlog | 8 |
| Last reported remaining backlog | 0 |
| Catch-up activity after 16:36:21 | none for the final four-plus minutes |
| Resident memory | stable near 1.43 million KiB (about 1.37 GiB) |

The initial report also showed two `trig_wait_event` callbacks and no event creation. The lack of
later catch-up reports, stable resident memory, and clean shutdown provide no evidence of a growing
event queue or registry.

### Final-source five-minute run

The final source, including the extraction batch prepass, received a second continuous five-minute
full-world run:

| Measurement | Value |
| --- | ---: |
| Highest reported pulse use | 864.930 ms |
| Highest observed `event_process()` total | 209.171 ms across two calls |
| Catch-up report windows | 16 |
| Slow passes represented by those windows | 338 |
| Budget-exhausted passes | 181 |
| Maximum requested missed pulses | 9 |
| Maximum remaining backlog | 9 |
| Last reported remaining backlog | 0 |
| Catch-up activity after 16:43:25 | none through shutdown at 16:47:58 |
| Callback registry | 12/512, zero overflow |
| Resident memory | stable at 1,433,160 KiB (about 1.37 GiB) |

No pending extraction batch occurred naturally during this second boot, so it is not used to claim
a new batch runtime. The earlier phase trace establishes which repeated scans dominated, while the
production-linked regression verifies the one-pass implementation's reference and combat cleanup.
The run instead validates final-source steady-state sharding, bounded recovery, telemetry, memory,
and shutdown behavior.

Both completed comparison runs stopped by `SIGTERM`, shut down vessel, Discord, Intermud3, terrain,
world, AI, and MySQL state, and reached `Done.`. No queue error, allocator abort, or listener remained.

## Review decisions

- Keep the game-loop aggregates in `perfmon.c`. They share the existing section registry, reset,
  reporting, and cleanup lifecycle; another source module would add ownership and manifest churn
  without separating an independent subsystem.
- Keep always-on callback clocks. Two monotonic reads were acceptable during the full-world soak,
  registry lookup is paid at creation rather than dispatch, and the data resolved the incident.
- Keep the fixed 512-entry registry and top-16 report. Runtime use was a small fraction of capacity,
  overflow stayed zero, and reports now expose both limits.
- Keep five-second catch-up aggregation. It reduced the baseline's one-line-per-pass noise while
  retaining exact cumulative counters.
- Do not add callback coalescing. Current event lifetime and ordering behavior remains intact.
- Do not run another broad Memcheck soak. Normal-build evidence is sufficient and no new memory or
  shutdown symptom requires a targeted Valgrind investigation.

## Adjacent findings resolved by master

The original incident also logged stale DG object UID lookups and invalid Command Sentinel mobile
activity dispatches. They were independent amplifiers rather than the measured event bottleneck.
The synchronized master code suppresses low-level missing-UID spam while retaining higher-level
context and accepts Command Sentinel mobile-activity dispatch. No additional branch-local change is
required.

## Verification

The synchronized master merge passed the complete repository validation path:

- clean GNU C23 build with `-Wall -Wextra`;
- production-linked CuTest suite;
- world tooling suite;
- protocol parser suite;
- character rename static and schema suites;
- install, root artifact cleanup, and clean full-world shutdown.

After the completion changes, the authoritative `make test-all` path passed:

- production-linked CuTest: `OK (713 tests)`;
- world tooling: `Ran 409 tests`, `OK`;
- protocol parser: `OK (29 tests)`;
- character rename static and schema suites: `PASS`; and
- installation and root-level `circle` artifact cleanup.

Worktree-only test prerequisites are documented rather than hidden:

```bash
LUMINARI_TEST_SKIP_SYNTAX_BOOT=1 \
LUMINARI_TEST_SPEC_WORLD_ROOT="$PWD/unittests/CuTest/fixtures/spec_world_inventory" \
make test
```

The complete world-tool path additionally needs the ignored reference corpora available to the
worktree, as described in `docs/guides/TESTING_GUIDE.md`.

## Files changed by the completion work

```text
docs/development/perfmon_help.txt
docs/ongoing-projects/game-loop.md
docs/systems/CORE_SERVER_ARCHITECTURE.md
src/comm.c
src/handler.c
src/handler.h
src/mob/mob_act.c
src/mob/mob_act.h
src/perfmon.c
unittests/CuTest/test_perfmon_production.c
unittests/CuTest/test_spec_command_pulse.c
```

No source file was added or removed, so `Makefile.am` and `CMakeLists.txt` did not require changes.
