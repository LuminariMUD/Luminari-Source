# Demand-Driven Gameplay Audit

**Date:** 2026-09-01
**Branch:** `event-driven-core-refactor`
**Status:** Controlling correction implemented and locally accepted

## Production Evidence

A copied production world loaded 61,004 NPCs. The rejected implementation
admitted about 60,500 recurring mobile owners and about 61,000 character-periodic
owners. Coalescing those into one handle per NPC bounded the queue near 63,000
but still produced about 20 million callbacks in eleven minutes and consumed
about 97% of one CPU core.

A debugger-owned, read-only sample of the copied world classified the NPCs:

| Capability or state | NPCs |
|---|---:|
| AI enabled and non-sentinel | 38,487 |
| Sentinel | 22,476 |
| Special-procedure flag | 2,382 |
| Echo definitions | 1,312 |
| Patrol paths | 101 |
| Scavenger | 11,876 |
| Aggression flags | 21,849 |
| Memory flag | 37,578 |
| Helper or guard | 11,017 |
| Listen | 8,951 |
| Hunter flag | 10,069 |
| Active hunt target | 0 |
| Active combat | 0 |
| Explicit `MOB_NO_AI` | 1,150 |

The large reactive populations do not justify permanent recurring events.
Wandering is the dominant genuinely periodic workload.

The corrected copied-world run admitted about 39,000 concrete autonomous
agendas, including about 37,000 wanderers, and settled at 2.6% of one core.
The timing wheel had no ready backlog, overdue work, or late callbacks. Roughly
193,000 agenda callbacks consumed 0.84 CPU-seconds, about 4 microseconds each.

The validation run caught and removed two disguised scans/cascades. Domain
character and object handles had resolved by walking their global population
lists; they now use lifecycle-maintained generation-keyed hash registries.
NPC movement had also woken unrelated room observers; it now wakes the moving
NPC's own arrival behavior, while only player or pet arrivals fan out bounded
local reactions.

## Mobile Activity Decomposition

| Legacy branch | Correct owner | Wake/admission | Completion/retirement |
|---|---|---|---|
| Gated-creature expiry | Exact expiry event | Gate creation/load with positive expiry | Expire, purge, extract, or gate removal |
| Umber-hulk equipment repair | Lifecycle reaction | Load and relevant equipment mutation | One-shot completion |
| Death retargeting | Encounter turn | Encounter membership and qualifying combat state | Encounter departure/end |
| Mobile special activity | Typed special event | Binding declares `SPEC_EVENT_MOBILE_ACTIVITY` | Binding/flag removal, disable, extraction |
| Spell-slot recovery | Resource deadline | Slot consumption below maximum | Full slots, invalid owner, extraction |
| Known-spell recovery | Resource deadline | Known slot consumption below two | Full slots, invalid owner, extraction |
| Combat race/class/spell AI | Encounter turn | Encounter membership | Encounter departure/end |
| Out-of-combat pre-buff | Local presence reaction | Eligible player enters room or caster becomes available | One-shot decision |
| Mobile echo | Echo deadline | Non-empty configured echo set | Echo removal or extraction |
| Scavenging | Scavenge decision | Eligible object enters a scavenger's room | No eligible objects, flag removal, extraction |
| Aggression | Room reaction | Character entry, visibility/hostility transition, boot reconciliation | One-shot decision or combat admission |
| Memory recognition | Room reaction | Remembered character entry/visibility transition | One-shot decision or combat admission |
| Guard/helper/mob assist | Combat reaction | `CombatStateChanged` in the room | One-shot decision or encounter join |
| Patrol movement | Patrol step | Non-empty path and eligible movement state | Path removal, combat suspension, extraction |
| Hunting | Hunt step | Valid generation-aware target | Target loss, encounter admission, cancellation |
| Adjacent archery | Local movement reaction | Eligible target enters adjacent room | One-shot decision or encounter admission |
| Nearby-fight listening | Adjacent combat reaction | Combat starts in an adjacent trackable room | One-shot movement decision |
| Random wandering | Wander decision | AI-enabled, non-sentinel, standing, uncontrolled NPC | State/control change, extraction |
| Sentinel posture restore | Position deadline | Position differs from configured default in load room | Position restored or state invalidated |

Room and adjacent-room reactions are bounded local traversal. They are not
global discovery scans. Initial world load may perform one explicit
reconciliation pass before the reactor starts; steady-state gameplay may not.

## Character Periodic Decomposition

| Legacy work | Correct owner |
|---|---|
| Falling, lava, drowning, environmental hazards | Movement/environment reaction plus hazard recurrence while exposed |
| Hit/move/PSP regeneration | Resource recovery while below maximum; cancel when full |
| Poison, acid coat, afflictions, damage-over-time | Applied effect owner or affected-character agenda |
| Weapon idle spells | Equipped effect deadline while such an item exists |
| Mount and grapple cleanup | Relationship mutation reaction, with a bounded repair event only when inconsistent |
| Combat round flags and action state | Encounter round owner |
| Six-second cooldown counters | Absolute expiry or explicit cooldown agenda |
| Encounter hostility/extraction timers | Encounter-owned deadlines |
| Connected player maintenance | Connection owner; small active population |
| Device recharge | Device/resource owner while below maximum |
| Timed quests | Player quest deadline while a timed quest exists |

Applying/removing affects, spending/recovering resources, changing position,
starting/stopping combat, movement, relationship changes, equipment changes,
script binding changes, and extraction are required synchronization boundaries.

## Acceptance Invariants

1. No owner is scheduled with an empty work-reason mask or agenda.
2. Removing the final reason cancels its handle in the same mutation boundary.
3. No normal callback traverses a global entity population to discover work.
4. Wanderers, patrols, hunts, scripts, and NPC encounters continue without
   players nearby.
5. Static idle NPCs own no recurring event.
6. Combat AI runs only on the encounter clock.
7. Full-world queue depth, ready backlog, overdue age, callbacks per second, and
   CPU are measured against the same copied-world legacy baseline.
8. `eventdebug` reports work reasons and keeps every line within 80 columns by
   default and 120 columns at maximum.

All eight invariants hold in focused tests and the copied-world live run. The
repository-wide four-mode matrix, authoritative `make test-all`, sanitizer,
Valgrind, and copied-world syntax gates also pass without an architectural
exception.

## Executable Architecture Lock

A 2026-09-01 follow-up audit confirmed that the normal source already matches
the architecture above. The recurring `service.mobile_activity_rollback`
callback is admitted only when the active-world subsystem is explicitly
disabled; it is not a scheduler for a gameplay class. Normal autonomous work
uses one `active_world_mobile_agenda` handle per owner with a non-empty reason
mask and dispatches only reasons whose deadlines are due.

Two regression layers now make this distinction durable:

1. `TestActiveWorldDormantPopulationDoesNotCreateScheduledWork` loads 512
   dormant sentinel NPCs and one off-screen wanderer. Queue depth remains one,
   exactly one callback executes at the wanderer's deadline, and removing the
   wander reason immediately leaves the queue empty.
2. `scripts/events/test_demand_driven_architecture.sh` is part of `make test`.
   It rejects global character/object list traversal, whole-mobile rollback
   dispatch, or reason-blind execution from the normal agenda callback and
   freezes the exclusive rollback gate in `runtime_service_needed()`.

This lock does not make empty zones inactive. Wanderers, patrols, hunts,
special procedures, resource recovery, and NPC conflicts continue off-screen
because their owners retain concrete work. Loaded entities without a current
responsibility remain dormant and cost no recurring callback.
