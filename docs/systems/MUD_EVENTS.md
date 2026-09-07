# LuminariMUD event systems

Updated: 2026-09-07, committed outcome and perception-backed quest contracts.

The game has one process-owned timing wheel. The legacy DG queue, scheduling
facade, heartbeat fallback, rollback build switches, and old save writer were
physically removed. The unused `util/hl_events.c/.h` implementation is also
retired. Neither libevent nor select I/O driver selects another gameplay clock.

## Boundaries

- [event_runtime](../../src/event_runtime.h) is the game-facing API for delayed
  and recurring work, semantic type registration, inspection, and cancellation.
- [game_scheduler](../../src/game_scheduler.h) is the private physical wheel.
  Only event_runtime owns it; gameplay modules do not call it directly.
- [dg_event](../../src/dgscript/dg_event.h) bridges process initialization,
  shutdown, deadline inspection, and scheduler advancement. Its historical name
  does not imply a separate DG event engine.
- [reactor](../../src/reactor.c) waits for I/O, signals, or the next deadline.
  Gameplay remains on the main thread for deterministic mutation ordering.
- [domain_events](../../src/domain_events.h) synchronously publishes immutable,
  borrowed facts after committed state changes. Pre-operation vetoes and value
  changes remain typed decision hooks; notifications cannot undo mutations.

Monotonic runtime ticks are 100 ms (`PASSES_PER_SEC` is 10). Deadline-driven
waiting does not require waking on every tick. Runtime handles are process-local
identities, not persistence records. Never serialize a handle or raw pointer.

## Native scheduling and ownership

Register a semantic type during boot with a stable name, handler, cleanup,
owner requirement, lateness policy, and admission limits. Startup seals the
registry; scheduling continues after sealing, but registration does not.

Use `event_runtime_schedule_owned_after()` with a generation-aware owner.
Callbacks return a completion or rescheduling result. Owner cancellation removes
queued work and prevents in-flight recurrence; cleanup runs exactly once after
payload use ends. Re-resolve entity handles after callbacks that can mutate or
extract entities. Shutdown invalidates all remaining runtime handles.

Existing semantic owners include:

| Type | Implementation |
| --- | --- |
| `combat.encounter.round` | `combat/combat_encounters.c` |
| `activity.primary.step` | `activity_manager.c` |
| `mobile.autonomous.agenda` | `active_world.c` |
| `affected.character.duration`, `affected.room.duration` | `affected_owners.c` |
| `character.maintenance` | `character_periodic.c` |
| `object.automatic_procedure`, `dg.random_trigger` | `periodic_owners.c` |
| `dg.trigger.wait` | `dgscript/dg_scripts.c` |
| `world.mud_hour_update` | `point_update_periodic.c` |
| `vessel.greyhawk.agenda`, `vessel.shared.agenda` | `vessels/vessel_periodic.c` |
| `vessel.rol.agenda` | `vessels/vessels_rol.c` |
| `ai.response.delivery`, `ai.request.retry` | `ai_events.c` |
| `service.persistence_batch` and coarse world services | `comm.c` |
| `action.ready.execute`, `action.ready.expire` | `ready_action.c` |

Autonomous work is admitted on concrete state changes and retires when complete.
Some service-owned feature loops remain; see the explicit inventory below. A
native callback wrapping a scan does not make that scan owner-driven.

## Table-driven MUD events

[mud_event_list.c](../../src/mud_event_list.c) maps IDs to callbacks, owner kinds,
recovery messages, and feat metadata. Every usable ID registers a native type
named `mud.<three-digit-id>.<readable-name>`. Entity lists retain MUD payloads,
not scheduler internals.

- Create and attach through `new_mud_event()`, `attach_mud_event()`, or `NEW_EVENT`.
- Implement table callbacks with `MUD_EVENT_CALLBACK`. A positive pulse count
  recurs; zero terminates. Native cleanup detaches and releases the payload.
- Query with `char_has_mud_event()`, `mud_event_is_live()`, `mud_event_remaining()`.
- Cancel with `mud_event_cancel()` or an owner clear helper. Use
  `mud_event_detach_owner()` when extraction occurs during callback execution.
- `change_event_duration()` and `change_event_svariables()` recreate the owned
  work through the same native lifecycle. Respect owned string allocation.
- Room/region attachments validate and copy their VNUM identity as required by
  the MUD payload contract. Do not retain temporary room-pointer storage.

Before adding an ID, update the enum and registry together, assign an explicit
persistence policy, use correct time units, and cover admission, recurrence,
cancellation, extraction, and restore. Daily-use recovery uses the registry's
feat/use data and validated `uses:N` payload. Guard division and overflow.

## Persistence and copyover

The registry assigns each event a persisted, reconstructed, or transient policy.
Persisted player timers use `Evn2` records with durable player identity, event
schema, remaining ticks, save epoch, validated payload, and recovery cadence.
The cadence is captured before save bookkeeping temporarily removes equipment
and affects that change daily uses. Offline file edits preserve restored cadence.

Restore validates owner, type, schema, payload, duration, and duplicate IDs;
then it creates a fresh runtime event. Elapsed offline time reduces one-shot
cooldowns and recovers all due daily charges arithmetically. It never replays a
burst of world-dependent callbacks. Fully expired timers are not admitted.

Older `Evn2` records and `Evnt` sections remain readable migration inputs.
`Evnt` lacks an elapsed-time checkpoint and resumes its stored duration. There
is no legacy writer or persistence-format selector. Saved counters also use
`CkAt` recovery; files without it fall back to `Last` during migration.

Combat, casting, DG waits, AI requests, readied actions, and other transient work
are not reconstructed from raw handles. Readiness is cleared by logout,
copyover, and reboot. World-owned reconstructed timers use authoritative saved
world/database state. Archival PubSub SQL remains data, not executable dispatch.

## Domain facts and subscriptions

Foundation types are registered in [domain_event_types.c](../../src/domain_event_types.c).
Runtime publishers use typed payloads and entity-scoped topics. A topic combines
event type, role, and generation-safe entity identity. Publication uses indexes,
not a scan of the population or full subscription list.

Subscriptions have bounded admission, explicit owners, opaque cancellation
handles, and cleanup. Owner teardown removes its subscriptions. Cancellation
inside a callback is safe; new subscriptions are not admitted during synchronous
publication. Nested causality is bounded. Never retain a borrowed fact payload.

Movement, damage, death, combat changes, object transfers, activities, world
phenomena, perception results, doors, committed nonlethal resolutions, completed
skill checks, casting starts and committed attacks have distinct contracts. The
mechanism inventory records remaining coverage gaps; registration alone is not
proof of a complete publisher.

Quest consumers subscribe after commit. Location objectives consume
`CharacterMoved`; object discovery and delivery consume the single compound
`ObjectMoved`; rescue and negotiation consume `CharacterResolved`; successful
ability objectives consume `SkillResolved`; witnessed-phenomenon objectives
consume `PhenomenonPerceived`. Each consumer re-resolves handles and verifies
final room/holder state. Existing death, pet and group credit remains on its
established authoritative path.

## Door mutation boundary

[movement/door_state.h](../../src/movement/door_state.h) defines a caller-owned,
synchronous `door_state_operation`. It has no queue, clock, or global dispatcher.

1. `door_state_begin()` captures one exit or a verified reciprocal pair, including
   stable room identity, process-local exit identity, old flags, and cause.
2. `door_state_apply()` changes the captured sides together. Existing compound
   command/special gateways may perform their authored mutations within the same
   explicitly captured operation, preserving containers and asymmetric rules.
3. `door_state_finish()` snapshots final state before notifying and emits at most
   one `DoorStateChanged` fact per changed side, scoped to that room. Finish only
   after the operation no longer needs raw pointers used by notification handlers.

`door_state_update()` and `door_state_replace()` combine these steps for simple
mutations. Paired mode checks the destination and reciprocal return room; it
never changes an unrelated reverse exit. No-op writes emit nothing. Failed
pre-operation decisions do not begin a mutation. Loading occurs without runtime
notification. Reset and edit causes are distinct from gameplay.

DG removal/retarget and live room replacement invalidate the old exit identity.
A new exit never inherits the old process-local identity. Observers cannot use
room/direction alone to treat a replacement exit as the original watched door.
Containers sharing old door-command macros do not publish room-door facts.

## Casting activities

Timed PC and NPC casts use `PRIMARY_ACTIVITY_CASTING` and one
`activity.primary.step` deadline. `activity` inspects the cast; `abort` or
`activity cancel` cancels it. Casting cannot pause. Initial PC/NPC delays remain
one/two seconds, with subsequent ten-pulse steps and existing acceleration rules.
Combat entry does not change that timing or charge a new action per step.

The initial concentration check remains. Actual positive `CharacterDamaged`
facts request another check using the spell difficulty and existing modifiers,
plus 10 and the committed damage amount (saturating at INT_MAX). Progress alone
and zero damage do not reroll concentration. A successful damage check does not
restart or delay casting. Alchemist/shadowdancer exemptions and the initial
rather than repeated deafness check remain.

`ActivityTransitioned` is routed on the actor's SUBJECT topic and includes
`activity_id` and `end_reason`. Cancellation detaches the activity and cancels
its timer before clearing casting fields. Completion detaches before resolving
magic, so effects caused by the completed spell cannot interrupt that old cast.
Character/object target lifetimes use generation-safe handles and extraction
notifications before data is freed. Re-resolve identities after observers run.

Prepared spells, PSP, NPC slots and action costs retain their authoritative
consumption sites; cancellation does not refund spent resources. Admission
rejects an occupied primary activity before spending another cast's resources.
Instant spells, commanded pet/eidolon casts through `handle_npc_cast`, NPC
`manifest_power`, items, innate abilities and direct `call_magic` resolution
remain synchronous. They have no timed activity or interruption interval.
`eRETIRED_CASTING` reserves the old numeric ID without a handler or timer type.

`CastingStarted` publishes only after a timed casting activity is installed and
scheduled and still matches its ID after lifecycle observers. It carries caster,
target, source room, spell and class metadata; this does not grant player spell
identification. These committed facts are not counterspell decision windows.

## Readied actions

```text
ready attack <target> on entry [arrival-name]
ready attack <target> on door open <direction>
ready attack <caster> on casting
ready <noncombat-command> on entry [arrival-name]
ready <noncombat-command> on door open <direction>
ready
ready cancel
```

`attack`, `hit` and `kill` select one normal readied strike. Arming spends a
standard action; execution does not spend another, and cancellation/expiry does
not refund it. The strike uses normal hit, damage, defenses and concentration,
bypassing the special-attack queue and opening full-attack routines. Entry binds
the target on matching arrival; casting and door targets bind when armed.

Noncombat commands are limited to say, emote, look, rest, stand, sit, open and
close, with normal interpreter admission and costs. Arbitrary commands and
aliases cannot bypass combat action accounting. Entry's optional name filters
the arrival; it does not rewrite the prepared command's target.

Door readiness requires a visible closed room door and valid destination. Only
that exit incarnation's gameplay closed-to-open transition qualifies. Unlocks,
no-ops, reset/edit changes and other doors do not trigger it. Closing/replacing
the exit or moving away cancels pending execution, even if it reopens later.

A match queues one native `action.ready.execute` callback one pulse later.
Execution revalidates owner/target lifetime, room, reach, visibility, position,
incapacity, activity, weapon eligibility and relevant door state. The normal hit
path retains peaceful-room and PvP checks. Casting reactions additionally match
the exact still-live casting activity ID; a replacement cast cannot inherit one.
Damage can interrupt through concentration but does not automatically cancel.

The one-pulse reaction precedes the initial casting deadline of at least one
second, even when both are overdue: deadlines sort before insertion sequence.
This prevents timed magic from completing first solely because dispatch is late.
Full counterspelling, instant-cast interruption and ally-attacked triggers remain
separate design work.

Readied attacks expire before action recovery at the owner's next semantic turn.
For attacks outside semantic combat, `action.ready.expire` imposes a six-second deadline;
entering combat cancels it and leaving restores one. An owner has at most one
execution and one outside-combat expiry deadline; armed actors are not polled.
Cancellation, movement, death, extraction, logout, re-arming, copyover, shutdown
and admission failure clean up transient subscriptions and deadlines. Entry
readiness subscribes to departures when armed because bus dispatch forbids new
subscriptions; a match binds a handle without adding subscriptions in dispatch.

## Diagnostics and regression checks

Immortals use `eventdebug` for native types, live owners, remaining times, pending
work, and per-type profiles. Entity views support player/mobile/object/room
filters; script views select `dg.` types. Payloads are redacted. Use
`eventdebug subscriptions` for `ready.entry`, `ready.door-open`, casting, and owner-lifecycle
listeners. A pending ready execution is an ordinary native event.

Run the native architecture, retired API, PubSub retirement, and demand-driven
scripts under `scripts/events/`. The retired API guard includes a negative
utility-tree fixture; source ownership checks cover both `src/` and `util/`.
Production-linked CuTests exercise door state, real DG commands, readiness,
owner cancellation, and no-op/paired mutation behavior.

See the [retained mechanism inventory](EVENT_MECHANISM_INVENTORY.md), and
[ADR 0002](../adr/0002-event-driven-core-boundaries.md).

Readiness deadline diagnostics: `eventdebug ready [reset]` reports bounded
last-1024 callback p50/p95/p99/maximum lateness in native pulses, excluding
the intentional one-pulse delay. See the mechanism inventory for limitations.
