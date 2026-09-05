# Event-Driven Core Acceptance

**Date:** 2026-09-01

**Status:** Native implementation accepted; physical rollback deletion remains
subject to the external stable-release gate.

## Player-Facing Model

The normal game no longer wakes on a generic heartbeat and checks every mobile,
player, room, and object for possible work. It uses two related mechanisms:

- A timed event is an owner-specific alarm. A fight owns its next round, a
  mobile owns its next admitted autonomous action, an affected room owns its
  expiry, and a character owns cooldown or maintenance deadlines.
- A domain event is an immediate typed fact. Movement, damage, death, object
  movement, combat changes, and spatial phenomena notify already registered
  local consumers synchronously. A domain event can add or remove owner work;
  it is not a second delayed queue.

One mobile agenda may hold several known due reasons, such as wandering,
patrolling, hunting, a script, posture restoration, or resource recovery. The
event wakes that mobile, processes only due reasons, then schedules its next
meaningful deadline or retires. A loaded but inert mobile owns no event.
Player proximity is not a gate: off-screen movement, scripts, hunts, and NPC
wars continue.

Named singleton events remain for genuinely global clocks such as zone updates,
weather/time, persistence admission, and mud-hour work. Lifecycle-maintained
registries give those services their relevant owners without rediscovering a
whole population on every callback.

## Architecture Audit

| Requirement | Accepted implementation |
|-------------|-------------------------|
| Timed ownership | One process-owned hierarchical timing wheel behind `event_runtime` |
| Runtime identity | Opaque non-reused event IDs and typed generation-aware owners |
| Type identity | 272 boot-sealed semantic types on the copied world, including 232 MUD IDs |
| Event lifecycle | Queued and in-flight cancellation, owner cancellation, recurrence, and exactly-once cleanup |
| Main loop | `libevent` waits for descriptors, signals, queued waits, or the nearest scheduler deadline |
| Dispatch fairness | Count and wall-time budgets preserve descriptor service under due-event load |
| Autonomous world | Concrete owner agendas; no normal whole-mobile discovery or dispatch loop |
| Combat | One `combat.encounter.round` event per live encounter and six-second semantic rounds |
| Activities | One lifecycle-owned `activity.primary.step` event per active primary activity |
| Scripts | Native `dg.random_trigger` and `dg.trigger.wait` owner events |
| MUD timers | Native per-ID types with owner lists, mutation APIs, and terminal cleanup |
| Offline recovery | Versioned elapsed-wall-time cooldown/use recovery without callback bursts |
| Copyover | Process-local timers rebuild from durable state; connected descriptors survive exec |
| Spatial facts | Native `WorldPhenomenon` routes bounded wilderness and room-graph sights/sounds |
| Diagnostics | Payload-redacted, paginated `eventdebug` views at 80 columns by default and 120 maximum |
| Rollback isolation | Old queue, heartbeat body, loops, adapters, and selectors absent from ordinary preprocessing and binary |
| Enforcement | CMake and Autotools run source contracts for one wheel, native boundaries, demand-driven work, and retired PubSub |

Source and binary audits found no ordinary-build legacy scheduling API, queue,
heartbeat body or caller, population-loop symbol, runtime selector, or
`legacy_event` adapter identity. An empty `heartbeat()` ABI stub remains; all of
its former pulse work is inside the default-disabled rollback build. The only
physical `game_scheduler_create()` call is owned by `event_runtime.c`. Normal
subsystem admission is all-or-nothing and startup fails closed rather than
silently restoring a polling loop.

Bootstrap may inspect the loaded world once to admit concrete owners. Staff
validation may compare a registry with the world. Neither is recurring gameplay
dispatch. Bounded room contents, encounter participants, owner lists, local
graphs, and indexed due-owner registries are intentional local work.

## Runtime Evidence

The copied production world loaded 762 zones, 91,735 rooms, 27,067 mobile
prototypes, and 22,637 object prototypes. The final run held 41,960 live events,
including 38,743 autonomous agendas and 36,912 wander agendas, with zero ready
backlog, overdue work, callback failure, rejection, owner mismatch, stale owner,
late callback, skipped occurrence, missed occurrence, or coalesced occurrence.

A controlled optimized comparison found and removed two accidental full
diagnostic traversals from every dispatch pass. The corrected native product
used 2.70% of one CPU core versus 3.33% for the optimized rollback loop on the
same world. Native type profiling records schedules, callbacks, callback time,
explicit cancellation, owner cancellation, manual rescheduling, and callback
recurrence without scanning all events in the hot path.

A live immortal session selected Puff's exact `dg.random_trigger` through both
the mobile and script-only entity views. A real same-process copyover retained
the connected descriptor, resumed commands, rebuilt runtime services, preserved
event diagnostics, and wrote its durable copyover/performance snapshots.

Automated acceptance passed 19/19 CTest targets and 1,052 production-linked
CuTests, ASan plus UBSan, 33 strict child-tracing Valgrind process logs, normal
and explicit rollback builds, five syntax modes, and all four event architecture
source contracts.

## Immortal MUD Test Card

Use an `LVL_IMMORT` character. Set the client width to 80 for the default UX and
optionally repeat at 120. No command should reveal payload text.

### Baseline Health

```text
eventdebug
eventdebug types 10
eventdebug queue 10
eventdebug domain
```

Expect `Backend: scheduler`, a sealed type registry, named scheduled runtime
services, an online sealed domain bus, and readable paginated lines. Ready or
overdue values may be transient while inspecting, but they must not remain
nonzero or grow. Investigate any failed event, admission rejection, registry
mismatch, stale-owner outcome, or rejected domain chain.

### Entity Filters

```text
eventdebug player <online-player> 10
eventdebug mob <visible-mobile> 10
eventdebug object <visible-object> 10
eventdebug room here 10
eventdebug room <loaded-vnum> 10
```

Each result must contain only events owned by the selected live entity across
its subsystem generations. Player lookup is online-only; room lookup requires a
loaded room. A valid entity with no current timed responsibility should produce
an empty result rather than unrelated events.

### Script Filters

```text
eventdebug scripts player <online-player> 10
eventdebug scripts mob <visible-mobile> 10
eventdebug scripts object <visible-object> 10
eventdebug scripts room here 10
```

Only `dg.` types should appear. A random-trigger owner can show
`dg.random_trigger`; a script currently executing `wait` can show
`dg.trigger.wait`. Compare with the corresponding unfiltered entity command to
confirm that the script rows are the same owner events.

### Gameplay Timers

```text
eventdebug type mobile.autonomous.agenda 10
eventdebug type combat.encounter.round 10
eventdebug type activity.primary.step 10
eventdebug type mud. 10
```

Observe a wandering or patrolling mobile outside player proximity and confirm
its agenda remains scheduled. Start combat and confirm one encounter-round event
for the fight, not one event per combatant. Start a primary activity and confirm
one step event. Use an ability with a cooldown and locate its readable
`mud.<id>.<name>` player event.

Log out while that cooldown is active, wait beyond its recovery time, and log
back in. The expired cooldown or recovered uses should be available immediately
without a burst of delayed messages or callbacks. Repeat a shorter active timer
through copyover and confirm the descriptor survives and the remaining duration
continues from rebuilt state.

### Spatial Facts

```text
eventdebug domain WorldPhenomenon
```

Cast Meteor Swarm where another tester can receive its bounded distant sight or
sound. The publication and handler counters should advance. Wilderness delivery
uses source coordinates; ordinary rooms use bounded graph distance. This test
does not imply that the deferred airship, dragon, weather, or terrain publishers
have been implemented.

## Remaining External Work

The normal product is complete for this development scope. The repository still
retains a default-disabled rollback build because deleting the old queue,
heartbeat, `select()` driver, legacy persistence writer, and rollback branches
requires a maintainer-defined stable production period and approval. Deprecated
PubSub database objects also require a protected inventory, backup, reviewed
drop migration, and restore rehearsal. Follow
[`EVENT_DRIVEN_CORE_RELEASE_GATE.md`](../deployment/EVENT_DRIVEN_CORE_RELEASE_GATE.md).

Expansion of `WorldPhenomenon` publishers and the Establish Camp
Survival/Nature design decision were explicitly excluded from this completion.
