# Event-Driven Core Refactor Phase 7D Character-Owner Validation

**Status:** Pass
**Date:** 2026-08-31
**Branch:** `event-driven-core-refactor`
**Scope:** Fourth Phase 7 slice, explicit character periodic state

## Specification Audit

| Requirement | Disposition |
|---|---|
| Owner deadlines | A relevant character owns one event for the nearest walk-to, PSP, bardic verse, or hint boundary. Services do not create four independent timers. |
| Gameplay parity | The existing walk step, PSP calculation, bardic audience/effect engine, and hint filters are now callable for one character. Legacy wrappers use the same routines. |
| Exact cadence | Deadlines align to the former shared 0.7-second, 5-second, 11-second, and 300-second pulse boundaries. A new performance still produces its existing immediate opening verse. |
| Lifecycle | Login, reconnect, copyover, immortal switch/return, disconnect, extraction, walk start, performance start/stop, callback self-cancellation, and shutdown synchronize or forget owners directly. |
| Bounds | The combined active registry admits at most 32,768 scheduled owners. Waiting registered owners refill released capacity without scanning dormant characters. |
| Dormancy | Connected characters remain service owners. A disconnected nonperforming character and an inactive NPC own no event. NPC bardic performances remain active without player proximity. |
| Diagnostics | `perfmon entities` reports mode, registry state, validation, capacity, callbacks, and per-service execution counts on seven labeled lines. Every line remains within the normal 80-column client width even with maximum-width counters. |
| Rollback | `LUMINARI_CHARACTER_EVENTS=legacy` restores all four heartbeat calls. Scheduled and legacy paths cannot execute together. |

## Behavioral Boundary

This slice changes how characters become due, not what walking, PSP recovery,
bardic performances, or hints do. One owner event selects the nearest shared
boundary and invokes only services due on that pulse. Connected characters are
active service owners even when regeneration or hint preferences make a
particular callback a no-op. That cost is bounded by active connections, not
the instantiated world.

## Focused Coverage

The production-linked suite proves:

- one owner event selects and advances through walk, PSP, verse, and hint
  boundaries without duplicate timers;
- walk interruption and invalid bardic state use the existing production
  cleanup behavior;
- PSP and hint callbacks remain tied to connected owners;
- a one-owner admission cap leaves a second active owner registered, then
  schedules it immediately when capacity is released;
- disconnect and explicit forget remove registry membership and queued work;
- registry links, scheduled counts, and event pointers agree; and
- legacy selection creates no character-owner event while all four heartbeat
  calls remain behind the exclusive boot-time gate.

## Validation Evidence

The final Phase 7D tree passed all of the following gates:

- CMake production and test builds, followed by all 979 production-linked C
  tests;
- the authoritative Autotools `make test-all` gate with the isolated CI
  runtime, including help synchronization, supervision, deployment, health,
  world tooling, protocol, process-memory, character-rename, and install
  checks;
- all 979 tests against the isolated `luminari_test` MariaDB database;
- all eight character-mode, event-backend, and I/O-driver combinations:
  scheduled or legacy character work, scheduler or legacy event backend, and
  libevent or select I/O;
- AddressSanitizer and UndefinedBehaviorSanitizer with all 979 tests;
- Valgrind with all 979 tests, zero errors, and no definite, indirect, or
  possible leaks;
- a syntax-only world boot through normal initialization and cleanup; and
- a live port 4101 session using Ornir at level 34. The session observed PSP
  and hint owner callbacks, exercised immortal switch and return, and confirmed
  that a loaded NPC moved autonomously while no player occupied its room.

The seven-line character-owner report is production-tested line by line. Its
widest possible row is at most 72 visible characters with maximum-width
counters, below both the normal 80-column client width and the hard 120-column
UX limit.

The character registry limit is 32,768 owners. Declared high-cardinality owner
limits now total 196,608 under the shared 262,144-event ceiling, retaining
65,536 event slots for lower-cardinality and transient work. Wizard Eye and
eidolon Bond Senses descriptor transfers are included in the source-level
lifecycle contract, alongside switch and return.

Logs are retained under `.ci-runtime/phase7d-*`. The local account credential
used for scripted validation was restored after each session, and the temporary
runtime NPC is removed by the clean server restart.

## Rollback and Next Slice

Restart with `LUMINARI_CHARACTER_EVENTS=legacy` to restore the four former
heartbeat scans. No live mode switch is supported.

The next Phase 7 slice decomposes room behavior, six-second damage/effect and
player maintenance, and the remaining mixed five-second Luminari pulse before
vessel and old `point_update()` work.
