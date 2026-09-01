# Native Event Architecture Enforcement

**Date:** 2026-09-01

**Branch:** `event-driven-core-refactor`

## Result

The default product architecture is now an executable build contract, not only
a design rule. CMake and Autotools run a consolidated native-event gate beside
the existing demand-driven, rollback-admission, and retired-PubSub gates.

The contract rejects a change when:

- a production module calls the physical scheduler instead of `event_runtime`;
- any C translation unit outside `game_scheduler.c` and `event_runtime.c` owns
  a `struct game_scheduler *`;
- the game-facing runtime creates anything other than one physical wheel;
- default preprocessing exposes the rollback queue, facade, backend, or raw
  event records;
- a required gameplay, DG, MUD, AI, vessel, combat, activity, affect, mobile,
  or service semantic type loses its stable identity;
- boot no longer seals the semantic type registry;
- entity-focused or DG-script-focused `eventdebug` commands, generation-wide
  owner matching, or payload redaction disappear; or
- the linked cleanup, dormant-population, and width-bounded diagnostic
  regressions disappear.

## Immortal Diagnostics

The entity and script filters are part of this contract:

```text
eventdebug player <name> [limit]
eventdebug mob <name> [limit]
eventdebug object <name> [limit]
eventdebug room <here|vnum> [limit]
eventdebug scripts <player|mob|object|room> <target> [limit]
```

The first four forms show every scheduled event whose owner identity belongs
to the selected live entity. The `scripts` form applies the same owner filter
and then limits results to `dg.` semantic types, including DG waits and random
trigger agendas attached to that character, object, or room. Independent
subsystem generations are intentionally matched together. Payloads remain
redacted, the normal width remains 80 columns, and output remains capped at
120 columns.

## Deterministic Test Runtime

Both build systems now point the named-special-procedure inventory regression
at `unittests/CuTest/fixtures/spec_world_inventory`. Local tests therefore do
not change result according to which ignored development-world archive is
installed under `lib/world`.

## Validation

The following passed from the normal CMake test product:

- 1,051 production-linked CuTests;
- `native-event-architecture`;
- `legacy-event-admission`;
- `demand-driven-event-architecture`; and
- `event-pubsub-retirement`.

The next tranche is full product validation: normal and rollback builds,
sanitizers, Valgrind, syntax boot, copied-world boot and soak, live MUD and
copyover checks, and direct in-game `eventdebug` exercises. The final tranche
then repeats the adversarial source/spec/runtime audit and publishes the
maintainer-facing gameplay report and MUD test card.
