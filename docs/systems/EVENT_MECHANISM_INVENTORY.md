# Event mechanism and door writer inventory

Reviewed 2026-09-05 for the door-readiness tranche on
`refactor/fight-combat-safety`. This is a source inventory, not a claim that
all gameplay is demand-driven. See [MUD_EVENTS](MUD_EVENTS.md) for the runtime
contract. Remaining owner migrations are tracked in [#105](https://github.com/LuminariMUD/Luminari-Source/issues/105).

## Timing ownership and retained dispatch

| Mechanism and source | Owner, cadence and callers | Disposition and rationale |
| --- | --- | --- |
| `game_scheduler.c`, `event_runtime.c` | One process runtime; main-loop deadline service | Sole gameplay scheduler. Hierarchical wheel, owner handles and bounded dispatch. |
| `mud_event.c` | Per-character/object/room/world native types admitted by `NEW_EVENT` and related semantic APIs | Retain semantic API; no second queue. Cooldowns use native ownership; timed casting uses the activity manager. |
| `periodic_owners.c`, `character_periodic.c`, `affected_owners.c`, `point_update_periodic.c`, `vessel_periodic.c` | Registered active owners, native typed cadences | Retain. Cadence does not itself imply a legacy scheduler. |
| Encounter, activity and mobile agendas | Native encounter/activity/mobile owners, their next due reason | Retain. Decisions within an agenda are feature state, not another clock engine. |
| `craft/crafting_new.c:craft_update` | `service.one_second` in `comm.c`, descriptor scan, active `craft_duration` decrements | Retained migration debt. Follow-up: character-owned completion deadline with interruption/refund policy. Idle descriptors currently incur checks. |
| `limits.c:self_buffing` | Same one-second service and descriptor scan, advances active buff sequence | Retained migration debt. Follow-up: one owned next-cast continuation respecting actions and interruption. |
| `vessels/transport.c:travel_tickdown` | Same service, playing descriptors in transit rooms | Retained migration debt. Follow-up: owned arrival deadline with destination invalidation and copyover policy. |
| `craft/crafting_new.c:update_supply_slots_for_all_players` | Same service, playing descriptors and online-time refresh eligibility | Retained migration debt. Follow-up: active-player refresh deadline; preserve online-only accounting. |
| `vessels/vessels_moving_rooms.c:moving_rooms_update` | `service.moving_rooms`, every ten seconds; `movingRoomList` countdown | Retained local countdown list. Follow-up: per-mover native agenda. Exit disconnection is now observable; movement timing is unchanged. |
| `quest/staff_events.c:staff_event_tick` | `world.mud_hour_update` through `limits.c:point_update_global_one` | Retained active event/delay counters, portal and population management. Follow-up: explicit active staff-event owner/deadlines. Preserve mud-hour duration semantics. |
| `comm.c:runtime_service_table` | Named native service owners; zone, auction, hunts, maintenance, persistence, mud-hour/day cadences | Intentional shared services. Review each remaining scan by active population and cost before converting. No heartbeat fallback. |
| DG waits/random triggers | `dg.trigger.wait` and `dg.random_trigger`, native trigger/owner handles | Timing migrated. Immediate command/door/item/combat triggers remain synchronous decision gateways; a veto must precede mutation. Do not replace them with post-commit facts. |
| Special procedures | Typed `spec_gateway_*` calls from commands/combat/movement and native autoproc/mover agendas | Retain authored decisions and mechanics. Event-enabled door writes now publish through the common contract. Other post-operation notifications need individual contract reviews. |
| Autoquests and `quest/hlquest.c` | Direct command/item/room/combat gateways | Retain reward/decision ordering. Door-opening quest helper is migrated; wholesale reward rewiring is deferred. |
| `combat/combat_reactions.c` | Bounded FIFO drained within outer `damage()` operation, no clock | Intentional synchronous reaction ordering; do not add timers to this FIFO. |
| Reactor libevent/select drivers | Socket readiness and waiting until native deadline | Infrastructure, not competing gameplay schedulers. Both supported. |
| `comm.c` watchdog, autorun watchdog | CPU-stall/process supervision independent of gameplay progress | Intentional infrastructure. Must still work when gameplay stalls. |
| AI workers, I3 networking, Discord ingress | External I/O and worker queues; main-thread world handoff | Keep I/O ownership separate. Worker queues must not mutate the world or become gameplay clocks. |
| Persistence batching, help reload, MSDP | Native service and output boundaries; bounded work or dirty state | Retain infrastructure cadence; measure cost independently of gameplay readiness. |
| Lazy resource/cooldown timestamps | Evaluated on access, including offline recovery | Intentional when no autonomous action is needed. Document persistence/time semantics per feature. |
| Legacy event save readers | Old record decoding in persistence | Migration compatibility only. Native writer uses Evn2/cadence records. |
| Retired PubSub SQL | Archival schema/data only | No compiled runtime or command entry point. Do not mistake retained SQL for a running bus. |
| Old DG queue, rollback switches, `util/hl_events.c/.h` | Removed source | Physically retired. Architecture checks cover both `src/` and `util/`; fixture injection proves a utility-tree retired API fails admission. |

## Door writer completion checklist

A fact describes final flags for one room/direction. A verified reciprocal
pair commits both sides before either notification. The operation capture is
caller-owned stack data, never a deferred transaction or queue. Destination
handles and process-local exit identities detect removal, replacement and
retargeting, including a replacement with identical flags. No-op flag writes
are silent. Bootstrap initialization is silent.

| Writer family | Publication/invalidation boundary | Evidence |
| --- | --- | --- |
| `movement/movement_doors.c` | `do_gen_door` finishes after traps, DG vetoes, compound autokey operations, messages and costs. Containers do not enter the door contract. | Command success/failure/no-op, NPC close, container and DG veto tests. |
| `act.other.c` | Successful lockpick and hidden-door search capture before mutation and finish after command work. | Shared mutation tests; source trace preserves single-side lockpick/search semantics. |
| `movement/movement.c:do_pullswitch` | Captured verified pair; final flags published after messages. Reverse destination is validated against the switch's target room. | Shared pair/asymmetry tests and writer review. Commented-out doorbash is not compiled. |
| `quest/hlquest.c:quest_open_door` | Paired unlock/open completion. | Opposite-side ready execution test. |
| `dgscript/dg_mobcmd.c`, `dg_objcmd.c`, `dg_wldcmd.c` | Single-side field/purge operation. New exits initialize silently; live replacement/retarget invalidates old binding. | Real mobile/object/room command tests, purge and retarget tests. |
| `db.c` zone D reset and RoL door reset | RESET cause, after flag/trap changes; never triggers readiness. | Administrative filtering and existing reset/trap tests. `setup_dir` is bootstrap initialization. |
| `olc/genwld.c` | Live `copy_room` publishes EDIT after replacement; cloned exits get fresh identities. Deletion captures incoming exits and finishes after world reindex. Occupant movement cancels local readiness. | Real OLC replacement test; existing room deletion/reindex suites. |
| `olc/oasis_copy.c` | Dig deletion publishes EDIT after removal/messages; new dig/buildwalk exits initialize silently. | Removal contract tests and source review. |
| `olc/redit.c`, `oasis_delete.c`, `genwld.c:free_room_strings` | Draft editing/freeing and storage helpers; live commits are wrapped by genwld/dig. Shutdown frees after runtime teardown. | OLC replacement and existing persistence/editing suites. |
| Avernus, Darkhold, Lavatubes, Mad Drow, Neverwinter, RoL combat specials | Captured compound operations or common single/paired update at final mutation boundary. Avernus re-resolves the actor after notification before movement. | Existing special mechanics suites plus paired/extraction tests. Authored asymmetric flags are preserved. |
| Abyss, Abyssal Vortex, Kenjin Tower specials | Capture rotations and removals; EDIT facts invalidate bindings after authored work. | Retarget contract test and source review. Existing zone destination choices are unchanged. |
| `vessels/vessels_docking.c`, `vessels_moving_rooms.c` | Connection removal/replacement captured; notifications after disconnection/link completion. No borrowed exit pointer is retained by facts. | Existing vessel suites plus removal/retarget contract tests. |
| `vessels/vessels_rooms.c`, newly created ship connections | Fresh zeroed open passage allocation; no prior visible closed door can be bound. | Existing vessel allocation/docking suites and allocation-site review. |
| `utils.c:remove_locked_door_flags` and door macros | Low-level mutations only inside the above captured operations; intentionally do not publish per macro. | Full source search of assignments, SET/REMOVE/TOGGLE and macro callers. |
| Wilderness exit copies/room reindex | Open passage setup or index relocation preserving room identity; not a door opening. Real deleted destinations use genwld invalidation. | Existing wilderness/reindex tests and source review. |

These are the known live writer families after searches of flags, entire exit
assignments, direction pointers, frees, retargets and door macros. Contract
coverage is shared where indicated; the table does not claim each authored
zone procedure has a new bespoke test.

## Diagnostics

`eventdebug subscriptions` exposes `ready.door-open` and the owner lifecycle
listeners. An armed action adds no polling callback; readied attacks outside combat own an expiry deadline. One accepted transition
adds one `action.ready.execute` event for the next pulse.

`eventdebug ready [reset]` reports the last 1024 callback samples, total callbacks
since reset, and nearest-rank p50/p95/p99/maximum native deadline lateness in
pulses. This includes entry and door readiness. The intentional one-pulse delay
is excluded; cancelled callbacks have no execution sample. Reset/copyover clears
samples. Pulse resolution cannot establish sub-pulse or end-to-end network
latency, and these diagnostics are not a production responsiveness SLA.

## Tranche 2: casting ownership

Timed PC/NPC casting now uses `activity.primary.step`; the former casting MUD
handler and its registration are retired. Activity lifecycle facts are routed
by actor and include a stable activity ID and terminal reason. Positive damage
facts request concentration checks; progress does not reroll concentration.
Object disposal publishes extraction before releasing target data. Instant
magic remains synchronous and is not a separate scheduler.

Details: [casting activities](MUD_EVENTS.md#casting-activities).
The countdown migrations listed above remain open.

## Tactical readiness (tranche 3)

`CastingStarted` is a typed fact on the existing domain bus, published only for
committed timed casting activities. It carries stable caster/target handles
and the activity ID; it does not create a parallel casting queue. Readiness
uses the native `action.ready.execute` and `action.ready.expire` types, with
character owners. Semantic encounter turns expire combat readiness directly.
Entry, door and casting attacks share one action reservation and single-strike
path. See the [readied-action contract](MUD_EVENTS.md#readied-actions).

## Remaining publication boundaries

`ObjectMoved` currently covers room removal/placement, not an atomic inventory,
equipment or container transfer with source/destination holders and actor/cause.
`CharacterMoved` from low-level placement lacks relocation cause and direction.
Consumers must not infer complete delivery or provoking movement from these
facts. Track the contracts in [#103](https://github.com/LuminariMUD/Luminari-Source/issues/103) and [#104](https://github.com/LuminariMUD/Luminari-Source/issues/104).
DG/special/quest pre-operation decisions must stay before mutation; migrate
post-operation notifications only after identifying an authoritative publisher.

Runtime latency/RSS acceptance remains qualified; [#111](https://github.com/LuminariMUD/Luminari-Source/issues/111)
tracks representative measurements. Native pulse lateness is not an end-to-end
latency measure. Gameplay proposals live in the issue tracker, separately from
these implemented ownership contracts.
