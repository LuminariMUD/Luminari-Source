# Event-Driven Core Refactor Phase 11c Validation

**Date:** 2026-08-31

**Scope:** Shared Phase 7 owner-handle migration

**Decision:** Implemented; irreversible Phase 11 removal gate remains pending

## 1. Delivered Slice

Phase 11c moves the shared, high-cardinality owners introduced during Phase 7
off public compatibility-record pointers:

- autonomous mobile activity;
- character and room affect processing;
- nearest-deadline character periodic work;
- object automatic procedures; and
- DG mobile, object, and room random triggers.

The runtime fields now store `event_handle_t`. A lightweight
`event_handle.h` boundary lets shared game structures name that identity
without importing the DG compatibility record. Owners schedule, cancel, and
query through the opaque handle API on both retained backends.

This is an identity and ownership migration only. Cadence, admission limits,
callback-relative recurrence, owner registries, domain lifecycle wakeups,
off-screen mobile simulation, NPC wars, affected behavior, automatic
procedures, DG `GLOBAL` semantics, and every rollback selector are unchanged.

## 2. Lifecycle Review

Every cancellation path clears the owner's handle before asking the facade to
cancel. A callback that extracts or otherwise forgets its own owner therefore
marks the in-flight record terminal, and cancellation still wins over a
positive recurrence result. Terminal callbacks clear their handle and update
the existing scheduled-owner accounting before capacity refill.

Character, room, object, and script payloads are borrowed game objects. Their
handle-aware cleanup callbacks intentionally perform no payload release; the
facade then nulls its internal payload field. This preserves the old custom
cleanup contract without dereferencing an owner that may already have been
destroyed. Generation-aware scheduler ownership remains the stale-owner
boundary.

Room and object OLC copies preserve runtime-only handles. World reindex first
cancels affected-room handles, moves the ownership lists, and then schedules
fresh handles against the new room generations. Direct tests cover these
transitions and registry consistency.

## 3. Source Inventory

The Phase 11b tree contained 119 `struct event *` occurrences across 21
source/header files, including 62 external compatibility references across 18
files. Phase 11c reduces that inventory to 99 occurrences across 14 files:
three libevent declarations, 54 private facade references, and 42 external
compatibility references across 11 files.

No raw compatibility pointer remains in `active_world.c`,
`affected_owners.c`, `character_periodic.c`, `periodic_owners.c`, or the
associated character, room, object, and random-script fields. The remaining
external categories are vessel owners, the point-update service, DG waits,
MUD events, and their diagnostics/persistence callers.

## 4. Validation

- Optimized production-linked build: pass without a new warning.
- Scheduler-default suite with authoritative world fixture: 1,030 tests.
- Explicit legacy-backend suite with the same fixture: 1,030 tests.
- Focused coverage includes autonomous off-screen mobiles, owner admission and
  refill, exact mixed periodic deadlines, affects and room behavior, DG random
  triggers, object automatic procedures, extraction, OLC copy, world reindex,
  shutdown, and rollback exclusivity.
- CMake Debug AddressSanitizer and UndefinedBehaviorSanitizer run with leak
  detection: 1,030 tests.
- Strict Valgrind with child tracing: 1,030 tests across 33 process logs, zero
  errors, and no definite, indirect, or possible leaks.
- Authoritative `make test-all` with the tracked world-inventory fixture:
  1,030 C tests, 504 world-tool tests, 29 protocol tests, 36 help tests,
  process-memory and character-rename checks, and release installation.
- Isolated live server on port 4102: real Ornir login, scheduled character and
  activity recurrence, pause/resume, cancellation, movement teardown, normal
  completion, zero registry mismatches and stale-owner outcomes, 80-column
  event diagnostics, signal shutdown, and confirmed port closure.

The live activity path is regression coverage only. It does not approve the
historical Survival/Nature model or alter the Establish Camp command.

## 5. Explicit Deferral

`ABILITY_SURVIVAL`, its Nature alias, and Establish Camp's existing skill rule
remain untouched. Resolving that gameplay design requires a separate human
decision.

## 6. Next Slice

Phase 11d migrates vessel action/periodic owners and the point-update/global
service owners. It remains a reversible handle migration; legacy queue,
heartbeat, I/O driver, persistence writer, and schema retirement stay gated.
