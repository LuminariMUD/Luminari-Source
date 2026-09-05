# Event-Driven Core Refactor Phase 6a Validation

**Status:** Pass
**Date:** 2026-08-30
**Branch:** `event-driven-core-refactor`
**Scope:** Typed domain-event introduction

## 1. Specification Audit

Phase 6a requires a typed synchronous fact bus, not another timer queue and not
the database-backed player pub/sub feature. The accepted implementation keeps
those ownership boundaries explicit:

| Requirement | Implementation |
|-------------|----------------|
| Stable typed registry | Boot registers stable IDs, names, and exact nonzero payload sizes, then seals all types, handlers, and resolvers. |
| Synchronous main-thread dispatch | The creating thread owns registration, publication, resolution, and destruction; cross-thread publication is rejected. |
| Deterministic handlers | Each type owns a list ordered by ascending priority and then monotonic registration sequence. Dispatch cost does not scan unrelated types. |
| Immutable borrowed payload | Callbacks receive `const` payload pointers for the publication call only. The bus neither copies, frees, logs, nor retains payload data. |
| Typed entity safety | `(kind, runtime_id, generation)` handles resolve through a boot-registered kind resolver immediately before mutation. |
| Bounded causality | Nested publication is depth-first, with defaults of 16 levels and 1,024 events per root chain. A limit aborts the remaining chain and is attributed to the rejected type. |
| Diagnostics | Bus, type, and handler statistics include publications, calls, depth, rejected chains, total/maximum time, and slow calls. |
| Notification semantics | Handlers return no decision and cannot roll back completed state. No migrated operation currently requires a pre-operation decision hook. |
| Production lifecycle | Normal and syntax-check boot create one eight-type sealed runtime; world teardown destroys it before entities. |

The foundational contracts are `CharacterMoved`, `CharacterDamaged`,
`CharacterDied`, `EntityExtracted`, `CombatStateChanged`, `ObjectMoved`,
`DoorStateChanged`, and `ActivityTransitioned`.

## 2. Migration and Rollback Boundary

This phase deliberately enables no gameplay publisher or subscriber. It cannot
duplicate movement, combat, scripting, wilderness, or activity side effects.
The existing `src/pubsub/` initialization, commands, database records, and
heartbeat queue are unchanged.

Reverting this phase removes the runtime initialization and the standalone
typed core without changing either timed-event backend or any gameplay path.
Future publishers must be reversible in independently reviewed owning-system
commits.

## 3. Focused Coverage

Twelve new production-linked tests cover:

- invalid, duplicate, capacity/freeze, unknown-type, and exact-size contracts;
- all eight foundational type identities and payload sizes;
- priority/registration ordering and depth-first nested order;
- independent nesting-depth and causal-event-count fail-closed behavior;
- extraction in an early handler followed by stale resolution in a later one;
- runtime-ID reuse with a new generation and kind mismatch;
- fake-clock total, maximum, and slow-handler attribution;
- cross-thread publication and destruction-during-dispatch rejection;
- borrowed stack payload preservation; and
- production runtime initialization, double-init rejection, sealing, and
  idempotent shutdown.

## 4. Validation Evidence

- Production-linked CuTest: pass, 967/967 in scheduler/libevent,
  scheduler/select, legacy/libevent, and legacy/select combinations.
- Isolated syntax-check boot: pass through the same matrix with the seeded
  encounter event; domain runtime initializes and tears down once.
- Normal Autotools production build: pass without a new compiler warning.
- CMake source/test manifest: configures and links the complete production
  test binary with the new modules.
- ASan and UBSan: pass, 967/967 with leak detection and halt-on-error enabled.
- Strict Valgrind: pass, zero errors and zero definite, indirect, or possible
  leaks. Existing test-process global registries remain reachable, not lost.
- Working-tree checks: `git diff --check` pass.

Logs are retained locally at `/tmp/phase6-foundation-scheduler-libevent.log`,
`/tmp/phase6-foundation-scheduler-select.log`,
`/tmp/phase6-foundation-legacy-libevent.log`,
`/tmp/phase6-foundation-legacy-select.log`,
`/tmp/phase6-foundation-asan.log`, and
`/tmp/phase6-foundation-valgrind.log`.

## 5. Player-Visible Result and Remaining Work

There is intentionally no player-visible gameplay change. The scheduler is the
alarm clock for owner-specific due work; this new bus is the immediate fact
channel that can wake affected owners. The old broad scans still run.

Phase 6b retired the unrelated database-backed pub/sub runtime and its
heartbeat queue under the schema-data rollback plan. Phase 7 starts the
player-relevant active-world migration: entities enter active, cooling-down, or
dormant states and only active owners schedule work. Combat, regeneration,
automatic actions, and activities follow in their specified phases.
