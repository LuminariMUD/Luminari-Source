# Event gameplay opportunities and migration audit

Date: 2026-09-05

Scope: `refactor/fight-combat-safety`, HEAD
`077ab249fdbf4c92a324060862081b849512d3cd`, including the existing local changes
to active-world, combat, handler, player persistence, and gameplay tests.
This is a source analysis and local verification report, not a gameplay migration.
Existing work was preserved. No production deployment or database changes were made.

## Assessment

The ordinary server uses one native timing engine. That does **not** mean all
old gameplay dispatch and countdown mechanisms have been replaced.

The new infrastructure already supplies generation-safe ownership, cancellation,
semantic timer types, scoped subscriptions, initiative-ordered encounter rounds,
action accounting, and bounded synchronous domain facts. The strongest next step
is to complete the facts and decision contracts that gameplay can depend upon,
then use them for player-visible tactical choices.

The project implements Pathfinder/D&D 3.5 mechanics. Recommendations below build
on that identity. A switch to another edition's action economy or rest model
would be a separate design decision.

## Migration inventory

| Mechanism | Current ownership and status | Required disposition |
| --- | --- | --- |
| Native timing wheel | `event_runtime.c` owns the scheduler; the normal main loop services its deadlines. | Keep as the only gameplay clock engine. |
| Table-driven MUD events | `mud_event.c` registers each usable ID as a native semantic type. Casting, cooldowns, and other `NEW_EVENT` callers use this layer. | Already superseded at the engine level; retain useful semantic APIs. |
| DG waits and random triggers | Native `dg.trigger.wait` and `dg.random_trigger` types, in `dgscript/dg_scripts.c` and `periodic_owners.c`. | Already migrated timing; script trigger dispatch is a separate concern. |
| Combat, activities, NPC agendas, affects, character maintenance | Native owner events in their respective modules. | Already migrated timing; some gameplay semantics remain cadence-based. |
| Vessels and point updates | Native vessel agendas and `world.mud_hour_update`; point updates use maintained owner registries. | Already integrated; shared world cadence is sometimes appropriate. |
| Legacy DG queue and heartbeat | Retained behind the default-disabled rollback build option. Four adapter admissions remain in AI, DG waits, and MUD events and disappear in normal preprocessing. | Explicitly retained fallback, not a hidden normal-runtime engine. Physical retirement is still separate work. |
| `util/hl_events.c` and `.h` | An entire alternative queue and scheduler remains outside `src/`. No caller/build inclusion found; CMake explicitly says it is excluded. | Remove this obsolete source and the CMake comment advertising it; extend architecture checks to `util/`. It was missed by source guards scoped to `src/`. |
| Old database PubSub | No `src/pubsub/*.[ch]` runtime sources; no normal build or command wiring. Archival SQL remains. Old object files may still exist locally. | Runtime retired. SQL retention and stale build products are not active event engines. |
| Crafting, self-buffing, travel, supply refresh | `service.one_second` still calls loops over `descriptor_list`. | Not separate engines, but incomplete owner-level migration. Give active jobs explicit owners and deadlines. |
| Moving rooms | `service.moving_rooms` invokes `moving_rooms_update()`, which walks `movingRoomList` and decrements counters. | Integrated clock, retained local countdown list. Replace with per-mover deadlines if complete owner-level migration is required. |
| Staff events | `point_update_global_one()` calls `staff_event_tick()`; staff duration/delay counters and replenishment remain. | Integrated mud-hour clock, retained feature state machine. Prefer a named active-event agenda with explicit reasons. |
| DG immediate triggers, special procedures, autoquests | Direct synchronous gateways remain in movement, item handling, and combat. | Not superseded by domain facts. Classify each as a pre-operation decision or post-operation notification before migration. |
| Combat reaction FIFO | `combat/combat_reactions.c`: at most 64 reactive damage admissions; drained by the outermost `damage()` call using entity handles. | Keep as explicit within-operation ordering. It has no clock and must not acquire a separate timer. |
| Reactor timer and watchdog | `reactor.c` uses libevent readiness/deadline waiting; `comm.c` retains a virtual-CPU watchdog. | Infrastructure, not gameplay timers. The watchdog must work when the game loop is stuck. |
| AI and I3 ingress | AI workers hand off native work; I3 has its own network thread/heartbeat and main-thread event consumption. | External I/O queues, not alternate gameplay clocks. Keep world mutation on the main thread. |
| Lazy timestamps | Resource regeneration, offline recovery, and some cooldown checks compare timestamps on access. | Not schedulers. Preserve lazy evaluation where no autonomous action is needed; document time and persistence policy. |

The repository's ADR 0002 and event-refactor specification deliberately retain
rollback code. The specification's removal gate includes a stable release
period, no rollback dependency, and maintainer approval. This audit did not
establish release/deployment evidence or perform retirement. The unused utility
scheduler is distinct from that deliberately supported rollback implementation.

## Contracts to complete before expanding consumers

1. **DoorStateChanged is registered but not published.**
   `domain_event_types.h:13` declares it; `domain_event_types.c` registers it.
   `movement/movement_doors.c:327` runs DG decision triggers and then mutates
   doors from line 354 onward. There is no production publication of the fact.
   Centralize successful door changes, preserve both sides of a door, and emit
   one logical change with old/new state. Audit scripted and spell-driven changes,
   not just player commands. Keep vetoes before mutation.

2. **ObjectMoved does not describe inventory transfers.**
   `domain_event_runtime.c:266` accepts room numbers and converts both ends to
   room handles. Its callers are `obj_to_room`/`obj_from_room` in `handler.c`.
   `obj_to_char`, `obj_from_char`, equipment, and containers have no corresponding
   complete transfer fact. Introduce an atomic transfer contract carrying actual
   source/destination holders and actor/cause where known. Do not award a quest
   twice from the removal and insertion halves of one transfer.

3. **CharacterMoved loses movement semantics.**
   `handler.c:2091` publishes direction `-1`. It does not distinguish walking,
   teleportation, forced movement, spawning, or scripted relocation. Add a
   relocation context and publish after the intended operation is committed.
   Direction alone cannot distinguish movement that provokes an opportunity
   attack from movement that should bypass it. Existing low-level placement
   can also precede higher-level script decisions, so document that boundary.

4. **ActivityTransitioned has no actor topic.**
   `activity_manager.c:145` publishes through `DOMAIN_EVENT_PUBLISH`, without the
   routed subject topic used by movement and damage. Add actor routing before
   features rely on exact subscriptions to another character's activity.

5. **The fact vocabulary is small.**
   The nine foundation types have no spell lifecycle, turn lifecycle, healing,
   condition-change, perception, or skill-outcome contracts. Add these only with
   a concrete consumer, authoritative publisher, ordering rules, and tests.
   Damage already has a publisher; its payload has amount/type/source, but no
   cast identity or attack outcome. Avoid interpreting it as an attack attempt.

## Recommended gameplay improvements, in implementation order

### 1. Expand ready into a tactical action

Existing foundation: `ready_action.c` supports `ready <command> on entry [target]`.
It listens on the destination room and schedules normal interpreter execution
one pulse later. Encounters already own initiative and action/reaction accounting.

Add selected triggers such as an enemy starting a spell, a designated ally being
attacked, or a door opening. Represent the chosen action and target with typed
intent and stable handles. Specify the action cost when arming, expiry, cancellation,
and the initiative consequences; revalidate visibility, range, and resources on
execution. Reuse existing encounter accounting instead of granting a free extra
action or adding a second reaction allowance.

Rationale: players can coordinate ambushes, protect allies, and prepare an answer
to an enemy tactic. The current delayed entry command is useful but is not a
complete tabletop Ready implementation. An interrupt that must precede a spell
or attack belongs in a typed decision window; a post-operation fact is too late.

### 2. Make casting respond to actual interruptions

Existing foundation: `magic/spell_parser.c:2288` already schedules casting through
the native MUD layer. Its progress callback repeatedly invokes
`concentration_check()`. The primary activity manager handles movement, damage,
combat, and target-loss responses, but the only production caller found for
`primary_activity_start()` is Establish Camp in `rol_feats.c`.

Model an in-progress cast as an activity with a cast identity and explicit
start/completion/interruption facts. Use actual damage and disruptive conditions
to request appropriate concentration checks. Extend the activity callback context
to provide the damage amount and spell details; its current generic recheck
does not receive the triggering damage payload. Add counterspell decisions
before spell resolution, with identification and resource costs.

Rationale: an archer can deliberately disrupt a caster, and an ally can protect
that caster. Interruptions become understandable consequences of play rather
than repeated checks simply because another progress callback occurred.

### 3. Align combat effects with semantic turns

Existing foundation: `combat/combat_encounters.c:943` prepares semantic rounds;
`actions.c` already delegates action queries/consumption to encounters.
Character affects in `affected_owners.c:104` retain global pulse-boundary timing.

Introduce explicit start/end-of-turn phases and duration policies for effects:
caster-relative, subject-relative, or real/game time. Start with a small set of
one-round effects, bleeding, and recurring saves. Carry remaining duration
correctly when encounters merge, end, or characters leave. Remove the previous
tick decrement for each migrated effect.

Rationale: a one-round defensive spell should give a predictable tactical window;
poison and recurring saves should resolve at a documented phase. Scheduling an
old decrement loop does not by itself establish that behavior. Keep exploration
and world-hour effects on their appropriate clocks.

### 4. Make hazards and persistent spells interact with movement

Existing foundation: room affects, `movement/movement_events.c` room damage and
trap handling, and spells such as walls, clouds, and tentacles already exist.

Use committed movement facts for entry/exit effects and semantic turns for
continued exposure. Add occupancy-scoped listeners and effect-source handles.
Define whether a particular spell triggers on crossing, entry, turn start, or
some combination; use per-effect exposure accounting to prevent unintended
double damage. Keep blocking traps and opportunity attacks in pre-move decisions.

Rationale: positioning, chokepoints, retreating, and pushing an enemy through a
hazard become meaningful. This extends existing spells rather than merely adding
more damage timers.

### 5. Give NPCs perception-driven responses

Existing foundation: `WorldPhenomenon`, the bounded spatial propagation code,
and demand-driven NPC agendas. The room-graph delivery path in
`wilderness/spatial_events.c:62` sends descriptions to connected players and
excludes NPCs; registered active-world handlers do not consume WorldPhenomenon.

Add a perception result separate from player-facing descriptive text. Let local
NPCs hear an alarm, investigate a blast, alert allies, seek cover, or lose interest
after an owner-scheduled deadline. Check senses, stealth, distance, obstacles,
and faction knowledge before waking behavior. Extend the emitter vocabulary with
source identity and phenomenon kind rather than parsing prose.

Rationale: noise becomes a cost, stealth becomes a strategy, and monsters react
to observable evidence. Preserve bounded propagation and avoid waking the entire
world. Surprise can then use encounter-local awareness; existing initiative and
flat-footed mechanics should be reused.

### 6. Unify extended skill work and exploration activities

Existing foundation: the primary activity manager has capabilities, traits,
progress, pause/delay/recheck policies, target ownership, and semantic-turn support.
Establish Camp demonstrates it. Crafting, transport, and buffing still have their
own polling loops.

Migrate active crafting and buff sequences first, preserving current timing and
offline behavior. Then implement interruptible lockpicking, trap disarming,
searching, climbing, first aid, and rituals where current mechanics warrant it.
Define what occupies hands, movement, or attention, whether progress survives
interruption, and when materials are consumed. Separate passenger transit from
the character's primary activity when passengers should be free to act.

Rationale: party members can cover a rogue disarming a trap or defend someone
performing a ritual. Shared interruption rules also close inconsistent cancellation
and resource-consumption behavior across individually implemented activities.

### 7. Use outcomes for quests, witnesses, and faction consequences

Existing foundation: autoquest checks are called directly from `handler.c:2106`,
item giving in `obj/act.item.c`, and death processing in `combat/fight.c:2382`.
The domain bus already reports death and movement, but object transfers need
the contract work described above.

Subscribe active objectives to relevant actors, targets, and locations. Add
explicit nonlethal resolution, delivery, skill outcome, and witnessed action
facts where a quest needs them. Preserve killer/pet/group credit, exactly-once
awards, and persistence. Migrate one authoritative reward path at a time.

Rationale: objectives can recognize rescue, surrender, negotiation, covert entry,
and keeping someone alive instead of relying predominantly on kills and pickups.
Witness-based reputation should depend on what an NPC could perceive, not an
omniscient global subscription.

### 8. Make expeditions and recovery stateful

Existing foundation: Establish Camp, recovery timers, weather/time services,
staff-event counters, and vessel agendas.

Build guarded rest with watches, interruptions, exposure, supplies, and staged
recovery. Give an expedition or active world event a named owner with only its
next deadline: departure, arrival, encounter, warning, or expiry. Keep persistent
state separate from process-local handles and rebuild work on restart.

Rationale: preparation and safe shelter become valuable, while travel and staff
events can tell multi-stage stories. Rest-based changes to daily abilities are
a balance proposal: the existing rolling daily-use recovery should not silently
be replaced during infrastructure migration.

## Verification and limits

- Passed `scripts/events/test_native_event_architecture.sh`.
- Passed `scripts/events/test_legacy_event_admission.sh`.
- Passed `scripts/events/test_pubsub_retirement.sh`.
- Passed `scripts/events/test_demand_driven_architecture.sh`.
- Existing production-linked `cutest` binary: **1,091 tests passed**, using
  `LUMINARI_TEST_ROOT` and `LUMINARI_TEST_SPEC_WORLD_ROOT` as configured by
  `Makefile.am`. An initial direct invocation without the fixture setting had
  one world-binding inventory failure; the correctly configured rerun passed.
- Checked the local build configuration: rollback is undefined in `src/conf.h`.
- Searched timer APIs, raw queue APIs, alternate event sources, time comparisons,
  threads, service callbacks, publishers, subscribers, and both build manifests.

No source changes were made and no fresh build, sanitizer run, production soak,
or rollback build was performed for this analysis. Passing architecture guards
does not prove publisher completeness: the orphan utility and retained descriptor
pollers illustrate their current coverage limits.

## Completion criteria for a future migration

1. Remove the unused utility scheduler and extend guards beyond `src/`.
2. Track every retained timer, dispatcher, local queue, and lazy timestamp in an
   explicit inventory with an owner and reason. Whitelist infrastructure only.
3. Replace retained gameplay discovery/countdown loops where owner-driven work
   is required; expose the resulting semantic owners through `eventdebug`.
4. Complete and test fact publishers across player, NPC, script, spell, and
   relocation paths. Test no-op and failed operations as well as success.
5. Classify DG/special/quest hooks by phase. Bridge post-operation notifications
   through one authoritative fact; preserve typed pre-operation decisions.
6. Test cancellation, extraction, target loss, visibility, duplicate publication,
   nested reactions, admission failure, logout, copyover, and encounter transitions
   for each migrated consumer.
7. Retire the explicitly supported rollback build through the project's release
   process if the goal includes deleting all historical engine implementations.

Until these distinctions are resolved, the accurate claim is "one normal-runtime
timer engine", not "every event system and gameplay timer has been superseded".

## References

- [ADR 0002](../adr/0002-event-driven-core-boundaries.md)
- [MUD event systems](../systems/MUD_EVENTS.md)
- [Event-driven core specification](EVENT_DRIVEN_CORE_REFACTOR_SPEC.md)
- [Pathfinder combat rules](https://legacy.aonprd.com/coreRulebook/combat.html):
  initiative, Ready, reactions, and duration relative to initiative.
- [Pathfinder magic rules](https://legacy.aonprd.com/coreRulebook/magic.html):
  concentration and counterspelling. These are edition references; the proposed
  MUD interactions and implementation ordering above are design recommendations.
