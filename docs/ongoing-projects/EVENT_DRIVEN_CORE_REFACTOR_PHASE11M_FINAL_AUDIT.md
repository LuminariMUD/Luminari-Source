# Event-Driven Core Refactor Phase 11m Final Audit

**Status:** Reversible implementation and local acceptance complete

**Date:** 2026-09-01

**Branch:** `event-driven-core-refactor`

**Scope:** Adversarial specification audit, residual correction, and acceptance

## 1. Architecture Verdict

The normal game path matches the controlling architecture:

- one global hierarchical timing wheel owns future work;
- an NPC has at most one autonomous agenda handle;
- existence, zone population, and player proximity do not admit an agenda;
- concrete behavior or spent resources add typed work with a meaningful
  deadline;
- callbacks execute only due reasons for that owner;
- local facts inspect one room or a bounded adjacent-room graph;
- completion, state loss, extraction, and generation change remove work; and
- off-screen wandering, patrols, hunts, scripts, and NPC wars remain active.

The rejected design, one unconditional recurring callback per loaded NPC, is
absent. Normal dispatch does not contain a coordinator that walks
`character_list`, `object_list`, or all rooms to rediscover autonomous work.
Population traversal remains limited to final boot reconciliation, explicit
staff validation, and boot-selected rollback code.

## 2. Gap Closed

The audit found one real specification miss. NPC class spell slots and
known-spell uses were still regenerated only by the whole-mobile rollback
dispatcher, so scheduler-mode NPCs could spend a use without ever recovering
it.

The correction makes expenditure the lifecycle trigger:

- the first class-slot deficit creates a five-minute owner deadline;
- the first known-spell deficit creates a one-minute owner deadline;
- both deadlines coalesce behind the NPC's existing agenda handle;
- a due callback restores one random eligible unit from each due pool;
- combat, casting, sleep, stun, paralysis, daze, and nausea defer only that
  owner's due recovery;
- additional deficits retain the existing recovery stream instead of creating
  duplicate events; and
- reaching full capacity removes the resource reason and retires an otherwise
  empty agenda.

The legacy dispatcher retains its old recovery calls and is exclusive with the
active-world scheduler.

## 3. Runtime-Service Audit

A default boot admits 14 of the 24 named definitions. None is an autonomous
population-discovery loop.

| Service | Normal responsibility | Work shape |
|---------|-----------------------|------------|
| moving rooms | update registered moving rooms | active list |
| one second | protocol, travel, self-buff, craft | connected/bounded owners |
| minute maintenance | global state, active items, memory sample | fixed plus active owners |
| zone | zone reset scheduler | indexed zone work |
| idle password | incomplete login timeout | descriptors |
| automatic procedures | fixed Avernus garden | fixed locations |
| hunt clock/round rollback | hunt countdown | singleton |
| auction/device recovery | auction; device rollback disabled | singleton |
| minute persistence | admit bounded persistence batches | bounded queue |
| hunt creation | create scheduled hunts | hunt table |
| mud hour | clock, active registries, diplomacy, clans | indexed/global rules |
| mud day | clan investments | clan table |
| usage | usage accounting | connected descriptors |
| time save | persist game clock | singleton |

The ten conditional definitions contain subsystem rollback work. Under default
selectors, the DG owner scan, whole-mobile cycle, character sweeps, affected
owner sweep, vessel sweeps, and point-update population scans are not admitted.

## 4. Ownership and Safety Audit

- Character and object domain handles resolve through generation-keyed hash
  registries in expected constant time.
- Autonomous callbacks carry immutable entity handles, not borrowed character
  pointers.
- Extraction removes registry membership and cancels every event for the exact
  owner generation.
- Dynamic wilderness rooms do not own stable trail identity. Wilderness trails
  use zone vnum plus coordinates and are restored to reusable room allocations;
  ordinary rooms use stable room vnums.
- DG time triggers and trail cleanup visit lifecycle-maintained owner/location
  registries rather than whole populations.
- Durable player cooldowns use wall-clock checkpoints and arithmetic catch-up,
  so logout and copyover do not freeze recoverable uses or replay gameplay
  loops.

## 5. Naming and Operational Corrections

Ambiguous residual service names now state their intent: automatic procedures,
hunt-clock plus round rollback, and auction plus device rollback. Rollback
mobile profiling is explicitly named `legacy_mobile_activity`. Permanent docs
now describe due-work dispatch, the actual default service set, and the correct
CLI flags: `-c` performs syntax checking while `-s` suppresses special
procedure assignment.

## 6. Validation

The focused production-linked suite covers both NPC resource systems, exact
admission deadlines, combat deferral, restoration, reason removal, and agenda
retirement. Acceptance evidence:

- the optimized production build passes with only the pre-existing suppressed
  `scanf` format warning in `src/players.c`;
- authoritative `make test-all` passes 1,047 C tests, 504 world-tool tests with
  35 intentional corpus skips, 29 protocol tests, source-policy checks,
  process-memory checks, schema checks, and immutable install verification;
- scheduler/libevent, scheduler/`select()`, legacy queue/libevent, and legacy
  queue/`select()` each pass all 1,047 production-linked tests;
- AddressSanitizer, UndefinedBehaviorSanitizer, and leak detection pass all
  1,047 tests with syntax-child boot disabled as in CI;
- strict child-tracing Valgrind passes all 1,047 tests with zero errors, zero
  definitely, indirectly, or possibly lost allocations, and only standard
  descriptors open;
- the copied production world syntax boot passes with 762 zones, 91,735 rooms,
  27,067 mobile prototypes, MySQL wilderness regions and paths, wilderness
  indexing, Perlin generators, and resource initialization; and
- the copied-world live server settles with 41,839 events, zero ready events,
  zero overdue pulses, zero late callbacks, zero registry mismatches, zero
  stale-owner outcomes, and no agenda-capacity rejection. The 80-column
  immortal display exposes autonomous reason counts, including resource
  recovery, and intent-named callback telemetry.

GitHub Build & Test and Security were green for both preceding architecture
commits. The same workflows are required to pass for this final audit commit
after push.

## 7. Remaining External Gate

The reversible source implementation is complete. The following physical
deletions are deliberately not performed:

- the legacy timed-event queue and backend selector;
- the `select()` I/O rollback driver;
- the compatibility heartbeat and subsystem rollback branches;
- the legacy durable-event writer/read migration; and
- deprecated PubSub database tables.

The specification requires a stable release period, evidence that rollback is
no longer needed, explicit maintainer approval, and an approved PubSub
backup/restore migration. Development tests and CI cannot manufacture that
operator evidence. Once the gate closes, the removal inventory defines the
mechanical deletion sequence; no new gameplay code may enter the compatibility
facade meanwhile.
