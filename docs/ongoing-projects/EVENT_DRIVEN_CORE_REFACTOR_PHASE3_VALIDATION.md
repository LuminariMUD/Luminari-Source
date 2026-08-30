# Event-Driven Core Refactor Phase 3 Validation

**Status:** Pass
**Date:** 2026-08-30
**Branch:** `event-driven-core-refactor`
**Scope:** Phase 3 libevent compatibility reactor

## 1. Result

Phase 3 is accepted. Production I/O defaults to a private libevent 2.1.12+
reactor, while `LUMINARI_IO_DRIVER=select` provides boot-time rollback. Both
drivers preserve the existing 100 ms pulse cadence, descriptor queues, nanny
and command flow, scheduler-backend selection, and single-threaded game-state
ownership.

No `libevent` type escapes `src/reactor.c`. Autotools and CMake both require the
system `libevent_core >= 2.1.12` dependency and link it dynamically.

## 2. Ownership Inventory

| Resource | Runtime owner | Reactor interest | Copyover policy |
|----------|---------------|------------------|-----------------|
| Main game listener | Main thread / `comm.c` | Read | Inherited; nonblocking; `FD_CLOEXEC` clear |
| Player descriptors | Main thread / descriptor list | Read, queued write, error | Preserved descriptors inherit; reactor watches are rebuilt |
| I3 ingress wake pipe | Main thread read end; I3 worker write end | Read | I3 worker is joined and pipe/gateway resources close before exec |
| Discord listener and client | Main thread / Discord bridge | Read; client write only with queued output | Bridge shuts down before exec and is recreated by the new process |
| Terrain API listener | Main thread / terrain bridge | Read | Bridge shuts down before exec and is recreated by the new process |
| Terrain API clients | Main thread / terrain bridge client table | Read, error | Closed by bridge shutdown before exec |
| Compatibility pulse timer | Reactor | 100 ms monotonic deadline | Detached with reactor and recreated after exec |
| Scheduler and gameplay state | Main game thread | Not exposed to libevent | Existing copyover serialization policy remains authoritative |

Worker code does not call libevent or mutate game state. The I3 wake descriptor
is the bounded cross-thread ingress path. The terrain and Discord sockets are
nonblocking and remain processed by their existing subsystem entry points after
the reactor reports readiness.

## 3. Signal Inventory

| Signal | Libevent driver owner | Select driver owner | Behavior |
|--------|-----------------------|---------------------|----------|
| `SIGUSR1` | Persistent reactor signal event | `sigaction` handler | Request wizard-list reload on main loop |
| `SIGUSR2` | Persistent reactor signal event | `sigaction` handler | Request emergency unrestrict on main loop |
| `SIGCHLD` | Persistent reactor signal event | `sigaction` handler | Reap children |
| `SIGHUP`, `SIGINT`, `SIGTERM` | Persistent reactor signal events | `sigaction` handlers | Set shutdown request; logging and cleanup occur on main thread |
| `SIGVTALRM` | Compatibility `sigaction` handler | Compatibility `sigaction` handler | `ITIMER_VIRTUAL` checkpoint; disarmed before exec and restored exactly once |
| `SIGPIPE`, `SIGALRM` | Ignored by process setup | Ignored by process setup | Preserve established compatibility behavior |

## 4. Automated Evidence

The production-linked CuTest binary passed all 948 tests in the required
matrix:

| Timed backend | I/O driver | Result |
|---------------|------------|--------|
| scheduler | libevent | Pass, 948/948 |
| scheduler | select | Pass, 948/948 |
| legacy | libevent | Pass, 948/948 |
| legacy | select | Pass, 948/948 |

Dedicated reactor tests exercise driver selection, fd readiness, compatibility
timer cadence, monotonic clock progression, and libevent signal delivery while
fd watches are active. The GitHub Actions production-test job carries the same
2x2 matrix.

- ASan and UBSan: pass, 948/948, leak detection and halt-on-error enabled.
- Valgrind: pass, 948/948, full leak check with definite, indirect, and possible
  leaks treated as errors.
- Build: clean under the normal Autotools build after requiring
  `libevent_core >= 2.1.12`.

## 5. Live Session Evidence

Automated tests used an isolated installed runtime and dedicated test database.
The live protocol sessions used the repository's configured local development
runtime and database. The disposable account, character files, and dedicated
test database were removed after validation.

| Session | Driver | Result |
|---------|--------|--------|
| Account and character creation, human warrior premade path, world entry, `look`, `score`, `time`, logout | libevent | Pass |
| Logged-in copyover, descriptor continuity, post-exec `score`, `time`, logout | libevent | Pass |
| Logged-in copyover, descriptor continuity, post-exec command and logout | select | Pass |
| `SIGUSR1`, `SIGUSR2`, graceful `SIGTERM` | libevent | Pass; each action observed once |
| 64 parallel TCP connection attempts; server remains responsive | libevent | Pass |
| 64 parallel TCP connection attempts; server remains responsive | select | Pass |

Local raw transcripts were retained during validation as
`/tmp/phase3-live-character-creation.log`,
`/tmp/phase3-live-libevent-copyover.log`,
`/tmp/phase3-live-select-copyover.log`, and `/tmp/phase3-load-*.log`. They contain
terminal control sequences and disposable local credentials, so this sanitized
result table is the permanent repository record.

## 6. Defect Found During Validation

The first real libevent connection exposed an event-lifecycle double free. A
signal-event cleanup loop had been placed in fd lookup code, so registering a
descriptor after signals were installed freed live signal events. Valgrind
identified the ownership violation. Cleanup now occurs only during reactor
destruction, and the regression test combines signal registration with fd
readiness to cover the failed path.

## 7. Rollback And Residual Risk

Set `LUMINARI_IO_DRIVER=select` before boot to return to the compatibility
select driver. Live driver switching is intentionally unsupported. Set
`LUMINARI_EVENT_BACKEND=legacy` independently to select the legacy timed-event
backend while the migration matrix remains supported.

This phase does not remove the fixed 100 ms heartbeat, arm the reactor from the
scheduler's next deadline, or migrate gameplay work. Those are Phase 4 and later
obligations. The raw live transcripts and temporary isolated runtime are local
test artifacts rather than source-controlled fixtures.
