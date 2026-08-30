# Event-Driven Core Refactor Phase 4 Validation

**Status:** Pass
**Date:** 2026-08-30
**Branch:** `event-driven-core-refactor`
**Scope:** Scheduler/reactor bridge and production hardening

## 1. Result

Phase 4 is accepted. The reactor now chooses one monotonic wait from the next
compatibility heartbeat and `game_scheduler_next_deadline()`. Due scheduler work
runs after descriptor input, commands, and output, with a 256-callback and 5 ms
budget. A remaining ready backlog causes a zero-wait reactor turn so descriptor
service occurs between batches.

The 100 ms heartbeat remains only for unmigrated pulse work. Its adapter path
dispatches the legacy timed backend, while the reactor path alone dispatches the
scheduler backend. Regression coverage proves that a scheduler callback cannot
run through both paths.

## 2. Operational Bounds

| Control | Accepted value | Evidence |
|---------|----------------|----------|
| Callbacks per reactor turn | 256 maximum | Budgeted adapter and due-storm tests |
| Scheduler wall time per turn | 5 ms maximum, plus one non-preemptible callback | Injected-clock budget test and reactor integration |
| Compatibility cadence | 100 ms | Existing pulse behavior and 2x2 backend/driver matrix |
| Large-advance threshold | More than 4,096 ticks | Exact-boundary and 4,097-tick structural telemetry |
| Timing wheel | Five levels, 64 slots per level | Retained after the measurements below |

A 10,000-callback ready batch needs 40 turns when only the callback-count limit
governs. A turn may process fewer callbacks when the wall-time limit governs,
so total drain time remains workload-dependent. Descriptors are serviced before
every continuation, and uninterrupted scheduler occupancy in one turn is
bounded to 5 ms plus the callback currently running. Individual callback
runtime remains a handler obligation because callbacks are not preemptible.

## 3. Workload And Structural Evidence

The deterministic representative workload covers world quest completion, world
falling, spell preparation, cooldown expiry, DG wait resume, AI combat rounds,
resource regeneration, and a world midnight edict. Together these cases span
the seven passive delay buckets from immediate work through overflow placement.

The hardening workload completed 100 churn rounds containing 6,400 scheduled
events, followed by a 512-event simultaneous due storm under a 32-callback test
budget. The storm drained in exactly 16 reactor-style turns, with 15 backlog
continuations and a largest cascade of 512 events. Existing capacity, callback
cancellation, recurrence, owner lifecycle, stall, and cleanup-once tests remain
part of the same production-linked suite.

Structural telemetry at the policy boundary recorded:

- A 4,096-tick advance used the normal cascade path: one nonempty slot and one
  cascaded event in the controlled threshold case.
- A 4,097-tick advance used the large-advance path and reclassified one queued
  event without scanning each skipped tick.
- The 512-event storm produced the expected largest cascade of 512 while
  preserving stable callback order and the configured dispatch bound.

The measurements do not show a material reason to alter the private wheel
geometry. Five 64-slot levels and the greater-than-4,096 large-advance policy are
accepted for the next tranche.

## 4. Automated Evidence

The production-linked CuTest binary passed all 951 tests under each supported
combination:

| Timed backend | I/O driver | Result |
|---------------|------------|--------|
| scheduler | libevent | Pass, 951/951 |
| scheduler | select | Pass, 951/951 |
| legacy | libevent | Pass, 951/951 |
| legacy | select | Pass, 951/951 |

- Normal Autotools production build: pass without new warnings.
- ASan and UBSan: pass, 951/951, halt-on-error enabled.
- Valgrind: pass, 951/951, zero errors and no definite, indirect, or possible
  leaks.
- Copyover: a logged-in descriptor survived exec, scheduler and libevent state
  were reconstructed, post-copyover commands succeeded, and the disposable
  account, character, and database rows were removed afterward.

## 5. Active Session Sample

A privacy-safe active local session covered login, world entry, `look`, `score`,
`time`, PERFMON collection, logged-in copyover, post-exec commands, and logout.
Across a 9.51-second pre-copyover window it recorded 97 outer loops, 95 expected
and 95 executed heartbeats, no missed or catch-up pulses, a 3.21 ms maximum
pulse, and no pulse over 10 ms. Input, command, and output processing remained
active throughout the sample.

The live window had zero timed-event callbacks after its PERFMON reset. It is
therefore responsiveness and copyover evidence, not the representative event
distribution claim. The reproducible source-derived workload above supplies
that distribution without recording player data. This distinction preserves
the specification's prohibition on calling an idle local timed-event sample
representative.

## 6. Rollback And Residual Risk

Set `LUMINARI_EVENT_BACKEND=legacy` to restore legacy timed-event dispatch and
`LUMINARI_IO_DRIVER=select` to restore the compatibility I/O driver at boot.
Live backend switching and live scheduler conversion remain unsupported.

The bridge does not migrate heartbeat consumers or define persistence policy.
Phase 5 owns transient, reconstructable, copyover-preserved, and persisted event
classification. Runtime callback cost can still exceed the reactor budget when
a single handler blocks; aggregate callback telemetry remains the way to locate
such handlers before consumer migration.
