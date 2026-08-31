# Event-Driven Core Refactor Phase 11l Validation

**Status:** Implementation and local acceptance complete
**Date:** 2026-09-01
**Branch:** `event-driven-core-refactor`
**Scope:** Behavior-preserving gameplay intent naming

## 1. Delivered Slice

Gameplay callback names now describe the responsibility being performed rather
than the historical heartbeat cadence that happened to invoke it. This makes
the event-owned architecture visible at call sites and removes an attractive
path for new gameplay code to grow another population pulse.

| Historical vocabulary | Intent vocabulary |
|-----------------------|-------------------|
| Luminari room and character pulses | room-affect activity, character environment and recovery |
| Bardic pulses and verse pulses | performance advancement and named verse effects |
| `mobile_activity()` and its pulse slice | explicitly legacy mobile cycle and slice rollback |
| object auto-pulse special events | object automatic activity |
| RoL/Avernus/Tarrasque pulse callbacks | the concrete sustain, restore, enforce-owner, boon, invocation, garden, or acid behavior |

The whole-population mobile and Luminari entry points remain only for explicit
rollback and carry `legacy` in their names. Normal gameplay scheduling invokes
owner-local responsibility functions. The object special-event flag retains
its existing numeric bit, so world files and persisted data do not change.

## 2. Retained Timing Vocabulary

Not every use of `pulse` is misleading. The following terms remain where a
pulse is the actual infrastructure concept:

- `PULSE_*` constants express durations in legacy ticks.
- `event_process_compatibility_pulse()` advances the retained physical legacy
  timed-event backend.
- `capture_slow_pulse()`, `PERF_log_pulse()`, and `PERF_prof_repr_pulse()` are
  reactor/performance measurements of real pulse timing.
- Runtime-service cadence labels describe global services that genuinely run
  at a fixed interval; renaming or decomposing them would be a behavioral and
  ordering change, not an intent-name cleanup.

The admission policy now rejects new gameplay function definitions named
`pulse_*` or `*_pulse`, with the four infrastructure functions above as an
exact allowlist. This is a source contract, not a style suggestion.

## 3. Behavioral Boundaries

- No schedule, recurrence, lateness, random-roll, callback-order, or player
  output rule changed.
- No new event category, owner, queue, scan, or capacity demand was introduced.
- The normal autonomous-mobile path remains concrete owner agendas. The
  renamed whole-population mobile cycle exists only for rollback.
- Establish Camp and its historical Survival/Nature decision are unchanged and
  remain explicitly deferred for human rules approval.
- Compatibility backend and driver removal remain separately gated by stable
  release and maintainer evidence.

## 4. Validation Evidence

- The optimized production build and production-linked CuTest binary build.
- All four supported backend/driver combinations pass 1,046/1,046 tests:
  scheduler/libevent, scheduler/`select()`, legacy queue/libevent, and legacy
  queue/`select()`.
- Authoritative `make test-all` passes the 1,046 C tests, 504 world-tool tests
  with 35 intentional corpus skips, 29 protocol tests, documentation and source
  policy checks, process-memory tests, rename checks, and immutable install
  verification.
- AddressSanitizer, UndefinedBehaviorSanitizer, and leak detection pass all
  1,046 tests with syntax-child boot disabled as in CI.
- Strict child-tracing Valgrind passes all 1,046 tests with zero errors and zero
  definite, indirect, or possible leaks.
- The copied production world passes syntax boot with all 762 zones, 91,735
  rooms, 27,067 mobile prototypes, wilderness indexing, MySQL regions and
  paths, Perlin generators, and resource initialization.
- The legacy-event admission contract passes and rejects new cadence-oriented
  gameplay function definitions.
- GitHub Build & Test and Security workflows will provide the independent
  post-push acceptance result.
