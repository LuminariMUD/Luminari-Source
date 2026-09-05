# Event-Driven Core Refactor Phase 7E Mixed-Owner Validation

**Status:** Pass
**Date:** 2026-08-31
**Branch:** `event-driven-core-refactor`
**Scope:** Fifth Phase 7 slice, room behavior and mixed five/six-second work

## Specification Audit

| Requirement | Disposition |
|---|---|
| Room behavior | Each affected room uses its existing event for the nearer five-second behavior or six-second duration boundary. It does not gain a second timer. |
| Character work | Every in-world character uses its existing nearest-deadline event for five-second Luminari work and six-second damage/effect work. Connected in-world players also run player maintenance. |
| Autonomous world | Player proximity is not an eligibility condition. Off-screen mobiles remain scheduled, preserving patrols, hazards, wars, and other world simulation. |
| Gameplay parity | Owner callbacks invoke extracted one-room or one-character forms of the established routines. Legacy wrappers invoke those same forms. |
| Ordering and cadence | Five- and six-second deadlines remain aligned to the former shared pulse boundaries. Coincident room work expires durations before behavior; coincident character work retains walk, PSP, Luminari, bard, hint, damage/effect, then player-maintenance order. |
| Lifecycle | Boot reconstruction admits existing in-world characters. Typed `CharacterMoved` facts admit later entrants; extraction and existing character lifecycle hooks cancel them. Final room-affect removal cancels its room event. |
| Bounds | No new high-cardinality event class was added. Declared owner limits remain 196,608 beneath the 262,144 compatibility-event ceiling, leaving 65,536 slots for other work. |
| Diagnostics | `perfmon entities` reports room behavior and character Luminari, damage, and player-maintenance work on labeled rows. The entire affected/character owner block remains within 80 columns. |
| Rollback | `LUMINARI_AFFECT_EVENTS` owns affected-room behavior and duration work. `LUMINARI_CHARACTER_EVENTS` owns per-character mixed work. Either can be legacy while the other remains scheduled without skipped or duplicate execution. |

## Gameplay Boundary

This slice changes how the game finds due rooms and characters, not what those
routines do. Fog, blade barriers, environmental hazards, regeneration, damage
over time, cooldowns, and automatic player maintenance retain their existing
rules and messages. The scheduler now wakes the relevant owner instead of the
heartbeat walking every room affect, character, or connected descriptor to
discover work.

An NPC does not sleep because no player can see it. Any character with a valid
world room remains a character-periodic owner. Connection state only controls
connection-specific services such as PSP, hints, and player maintenance.

## Focused Coverage

The production-linked suite proves:

- affected-room behavior fires at the exact five-second boundary without
  decrementing six-second duration state;
- character and room durations then advance at the exact six-second boundary;
- a room affect expiring on a coincident five/six-second boundary is removed
  before behavior and cannot receive one extra final tick;
- an in-world NPC without a descriptor and a connected player each execute
  Luminari and damage/effect work from one owner event;
- only the connected player executes player maintenance, including a real
  mission-cooldown decrement;
- a typed movement publication admits a newly placed in-world mobile without a
  population scan;
- runtime initialization registers the fifth production domain handler;
- mixed heartbeat wrappers retain independent affected and character gates;
- registry counts, scheduled counts, event pointers, and rejection/refill
  invariants remain consistent; and
- every character-owner diagnostic row remains within 80 visible columns.

## Validation Evidence

The final Phase 7E tree passed all of the following gates:

- warning-clean CMake production and test builds, followed by all 983
  production-linked C tests;
- the authoritative Autotools `make test-all` gate with the isolated runtime,
  including help synchronization, supervision, deployment, health, world
  tooling, protocol, process-memory, rename, and install checks;
- all 983 tests against the isolated `luminari_test` MariaDB database;
- all 16 combinations of scheduled/legacy character work, scheduled/legacy
  affected work, scheduler/legacy event backend, and libevent/select I/O;
- four direct syntax-check world boots covering every affected/character mode
  combination;
- AddressSanitizer and UndefinedBehaviorSanitizer with all 983 tests;
- strict Valgrind with all 983 test entries, zero errors, and no definite,
  indirect, or possible leaks. The forked syntax-boot entry was skipped under
  Valgrind after the same four modes passed direct syntax boots, because its
  deliberate `_exit()` retains the child boot image by design;
- a live scheduler/libevent server on port 4101 using Ornir at level 34; and
- two logged live sessions exercising Mage Armor, Wall of Fog, a remote
  training dummy, callback telemetry, registry validation, and clean shutdown.

The first live session recorded one affected character and one affected room.
Wall of Fog produced two behavior runs and two processed room affects, while
room duration work and Mage Armor duration work advanced independently. The
second session loaded a training dummy in room 3001 while Ornir remained in the
Hall of Beginnings. `perfmon entities` then reported two scheduled character
owners, four Luminari executions, four damage/effect executions, two
player-maintenance executions, and zero registry mismatches. The affected and
character block's widest visible row was 44 characters.

Logs are retained under `.ci-runtime/phase7e-*`. Temporary local credentials
were restored after clean shutdown, and the remote test mobile was discarded
with process teardown.

## Rollback And Next Slice

Restart with either or both of these settings:

- `LUMINARI_AFFECT_EVENTS=legacy` restores affected duration scans and the
  affected-room half of the five-second Luminari wrapper.
- `LUMINARI_CHARACTER_EVENTS=legacy` restores all per-character periodic
  scans, including Luminari, damage/effects, and player maintenance.

No live mode switch is supported. The next Phase 7 slice decomposes the
half-second vessel service group, converted RoL ship movement, vessel schedule
work, and the mixed mud-hour `point_update()` pulse by real owner and service
state.
