# Event-Driven Core Refactor Phase 11b Validation

**Date:** 2026-08-31

**Scope:** First focused compatibility-record caller migration

**Decision:** Implemented; irreversible Phase 11 removal gate remains pending

## 1. Delivered Slice

Phase 11b migrates two private owners that were already accepted in Phases 8-10:

- the one recurring round clock owned by each live combat encounter; and
- the wall-time step timer owned by each active primary activity.

Both owners now store `event_handle_t`, schedule through `event_schedule*()`,
cancel through `event_handle_cancel()`, and query remaining delay through
`event_handle_time()`. No public gameplay structure or command contract changes.

## 2. Lifecycle Review

Encounter events preserve one payload for their recurring lifetime. Normal
terminal dispatch frees that payload and destroys the encounter without
self-cancelling; external teardown and encounter merge cancel by handle, with
the facade retaining generic payload cleanup. Event-count diagnostics retain
the existing one-event-per-encounter accounting.

Primary activities use handle-aware cancellation cleanup. A queued cancel or
shutdown detaches only a matching handle and frees its payload. During normal
dispatch the owner temporarily clears the handle, preventing callback-driven
activity completion from cancelling its in-flight record, then reattaches the
same handle only when recurrence is accepted. Stale-owner and terminal paths
retain callback-owned payload cleanup.

Both paths continue to work on the timing-wheel scheduler and retained legacy
queue. Combat cadence, semantic action economy, activity progression,
interruption policy, diagnostics, and rollback selectors are unchanged.

## 3. Source Inventory

Phase 11a's additive facade left 129 `struct event *` occurrences across 23
source/header files: three libevent declarations, 54 private facade references,
and 72 external compatibility references across 20 files. This slice reduces
the total to 119 across 21 files and external references to 62 across 18 files.
Neither `src/activity_manager.c` nor `src/combat/combat_encounters.c` owns a raw
compatibility-event pointer afterward. The remaining `event_time()` call in the
combat file reads a legacy MUD action event and belongs to the later MUD-event
migration, not the encounter round owner.

## 4. Validation

- Optimized production-linked CuTest build: pass without a new warning.
- Scheduler-default C suite with authoritative world fixture: 1,030 tests.
- Explicit legacy-backend C suite with authoritative world fixture: 1,030 tests.
- Existing activity coverage exercises progression, delay, pause/resume,
  cancellation, re-entry, extraction, and timer snapshots.
- Existing encounter coverage exercises recurrence, deadline replacement,
  joins, merges, callback mutation, complete teardown, ID generation reuse,
  semantic rounds, and rollback exclusivity.
- CMake Debug AddressSanitizer and UndefinedBehaviorSanitizer suite with leak
  detection: 1,030 tests.
- Strict Valgrind with child tracing: 1,030 tests across 33 process logs, zero
  errors, and no definite, indirect, or possible leaks.
- Authoritative `make test-all`: 1,030 production-linked C tests, 504 world
  tool tests, 29 protocol tests, 36 help tests, process-memory and character-
  rename checks, and release installation.
- Isolated live combat: one scheduled event owned a two-participant encounter,
  semantic rounds and turns recurred across two boundaries, `peace` removed
  the encounter and event, and the server shut down with its port closed.
- Isolated live activity: pause preserved the deadline, resume progressed,
  manual cancel and movement detached the timer, normal completion removed the
  activity, event diagnostics remained at or below 80 columns, and shutdown
  closed the server port.

The live activity harness exercises the currently implemented camp behavior as
a regression test only. Its existing Nature/Survival setup is not accepted here
as a gameplay-design decision.

GitHub Build & Test and Security workflow results are recorded after push.

## 5. Explicit Deferral

This slice does not change Establish Camp's skill or ability mapping. The
existing `ABILITY_SURVIVAL` behavior remains intact. Whether camp should use a
different Survival/Nature model requires a separate human gameplay decision.

## 6. Next Slice

Continue with the already-event-driven Phase 7 owner managers in focused
categories. Shared gameplay structures need a lightweight opaque-handle type
boundary before their stored pointers move. DG wait/random records, the MUD
event layer, residual heartbeat services, and irreversible compatibility
deletion remain later work.
