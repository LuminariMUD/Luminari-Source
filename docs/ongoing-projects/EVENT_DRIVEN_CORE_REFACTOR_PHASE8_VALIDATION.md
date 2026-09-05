# Event-Driven Core Refactor Phase 8 Encounter Validation

**Status:** Pass
**Date:** 2026-08-31
**Branch:** `event-driven-core-refactor`
**Scope:** Encounter-owned compatibility combat scheduling

## Specification Audit

| Requirement | Disposition |
|---|---|
| Runtime authority | A fixed 32,768-slot `combat_encounter_data` registry owns process-local IDs and generations. It is distinct from wilderness encounter content and is torn down before world entities. |
| One event per fight | Every active fight has one `combat_encounter_round` event with typed encounter ownership. Individual participants retain phase, deadline, and deterministic due order without owning combat events. |
| Join and merge | Hostility creates, extends, or merges encounters. In-dispatch additions and merges are deferred; absorbed participant deadlines are preserved and already-due members still act once in the same callback. |
| Fair eligibility | Ordinary joins preserve their existing initial initiative delay. A join created during encounter dispatch cannot act before six seconds have passed. Terminal encounters cannot be revived. |
| Leave and end | Participants are marked inactive immediately and compacted safely after dispatch. The API classifies stop, movement, flee, teleport, death, extraction, disconnect, and administrative departure. The shared event ends exactly once when no active hostility remains. |
| Lifecycle safety | Movement, committed death, and extraction facts resolve generation-aware character handles. Direct extraction cleanup is idempotent, DG mobile transformation preserves runtime links, and stale encounter or participant resolution is counted. |
| Gameplay compatibility | Both modes invoke the same extracted production combat-phase routine. Existing three-phase behavior, two-second phase cadence, six-second round, action queue, reactions, attacks, and effects remain authoritative. |
| Diagnostics | `eventdebug` reports mode, active encounters and participants, shared event count, create/end/merge totals, callbacks, phase and terminal outcomes, structural/outcome mismatches, admission failures, and stale callbacks. Queue filtering shows encounter owner ID and generation without payloads. |
| Rollback | `LUMINARI_COMBAT_EVENTS=legacy` exclusively restores per-character `eCOMBAT_ROUND` events for the entire boot. No active fight is converted between models. |

## Gameplay Boundary

This tranche changes who owns the combat clock, not the combat rules. A fight
between two or twenty combatants now wakes one encounter event at the nearest
participant deadline. That callback runs only due participants through the
same combat code that the old per-character events used.

Mobs continue to fight other mobs in empty zones, and linkdead participants
retain the existing continued-combat policy. Movement reactions still occur
before successful departure. Phase 9, not Phase 8, owns any proposed change to
initiative, action/reaction budgets, intent buffering, or the visible meaning
of a six-second round.

## Focused Coverage

Eight encounter tests prove:

- one shared event with exact participant cadence, phase order, and teardown;
- joins before, during, and after dispatch, with the guard applied only during
  dispatch;
- deferred encounter merge, preserved due times, same-callback due work, and
  bridge departure;
- callback removal of every participant without iterator invalidation;
- slot reuse with a new generation and no stale revival;
- terminal encounter replacement rather than revival;
- every declared departure reason plus exactly-once shared cancellation; and
- exclusive legacy selection with no encounter state or event.

The production runtime test also proves that the runtime registers eight
handlers in total, including all three encounter lifecycle handlers, without
test-order state leakage. It also proves that committed character death
publishes the foundational typed fact.

## Validation Evidence

The accepted tree passed:

- warning-clean production Autotools and CMake builds;
- the isolated authoritative `make test-all` gate, including 1,004
  production-linked C tests, 504 world-tool tests, 29 protocol tests,
  process-memory and character-rename checks, and release installation;
- all 1,004 C tests against a disposable `luminari_test` MariaDB runtime;
- four representative direct syntax boots: encounter/scheduler/libevent,
  character/scheduler/libevent, encounter/legacy/libevent, and
  encounter/scheduler/select;
- an AddressSanitizer and UndefinedBehaviorSanitizer production-linked CMake
  build with all 1,004 tests and no finding;
- strict child-tracing Valgrind with all 1,004 tests, zero errors, and no
  definite, indirect, or possible leaks; and
- a logged live scheduler/libevent MUD session on port 4101 as Ornir at level
  34. Two durable NPC fixtures entered combat, `eventdebug` showed one
  encounter, two participants, one shared event, encounter owner ID/generation,
  and successful callbacks; `peace` reduced the encounter, participant, and
  event counts to zero.

The first unisolated repository gate reproduced the two known mutable-world
fixture collisions. The authoritative rerun used the tracked spec inventory
and disposable syntax world and passed; this was a harness isolation correction,
not a product failure. Live credentials were redacted and restored byte for
byte, the player file and temporary target prototype were restored, the server
shut down normally, and port 4101 was closed.

Adversarial testing found and fixed a passive-membership/turn-eligibility
conflation, terminal encounter revival, dispatch-terminal event accounting,
generic payload cleanup, leaked test selection, missing death publication, and
missing stale-resolution telemetry before acceptance. Logs remain under
`.ci-runtime/phase8-*` and `.ci-runtime/eventdebug-live-*` and are untracked.

## Validation Policy

The encounter implementation is now the development focus. It receives full
gameplay, database, sanitizer, Valgrind, performance, and live-MUD coverage.
The character-event implementation remains only as a temporary rollback path,
so it receives compile, syntax-boot, and focused exclusivity smoke coverage
until the final dual-mode removal gate.

## Rollback And Next Tranche

Restart with `LUMINARI_COMBAT_EVENTS=legacy` to restore per-character combat
events. Live mode switching and conversion of active fights are unsupported.

Phase 9 is next: separately approve and implement semantic encounter rounds,
initiative ordering, explicit action and reaction budgets, intent buffering,
six-second D20 round behavior, and appropriate encounter-owned round flags.
Its gameplay and help changes require design and balance review rather than
being inferred from this compatibility migration.
