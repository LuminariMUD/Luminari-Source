# Skill work, guarded rest, and expedition ownership

Assigned issue: #109. Decision and first pilot: 2026-09-06 on
`fix/open-issue-repairs`.

## Decision

Use the existing primary activity manager only for work whose result is delayed
and can be meaningfully interrupted. Keep immediate rules and committed world
operations synchronous. A command must not gain a timer merely because it uses
a skill.

The first pilot is a full-room search. Player searches now own one six-second,
room-targeted `PRIMARY_ACTIVITY_SEARCH`. The result is atomic: treasure, hidden
exit, and trap checks happen only at completion. Committed movement, damage,
combat, loss of sight, grapple, entanglement, target-room loss, logout, death,
or extraction cancels the work and grants no result. The work claims attention,
vision, a standard action, and a move action; it has no hand or material claim.
The existing NPC special procedure keeps its immediate scan so a periodic mobile
cannot accumulate or repeatedly attempt player work.

The activity manager remains the only owner of this timer. `activity` exposes the
semantic name and claims, and `eventdebug` exposes the native
`activity.primary.step` deadline. An idle character has no search event.

## Skill classification

| Work | Decision | Claims | Progress and interruption | Resource boundary |
| --- | --- | --- | --- | --- |
| Full-room search | Owned atomic activity, implemented | Attention, vision, standard and move actions; stationary, distracted, obvious | One six-second interval; movement, damage, combat, invalid room or lost sight cancels; no partial result survives | No materials; rolls and discoveries commit only on completion |
| Lockpicking | Keep immediate until a multi-stage lock interaction is authored | Hands, attention, fine manipulation, move action | The current single check and door mutation are one synchronous transaction. If later staged, progress belongs to the exact door/container generation and movement, damage, combat or target change cancels it | A future consumable pick must be reserved after admission and consumed at an explicitly committed attempt, never before admission |
| Trap disarming | Keep immediate until traps expose stable instance identity and staged work | Hands, attention, vision, fine manipulation, full-round action | Current success/failure/trigger is one authoritative transaction. A timer without a stable trap generation could complete against a replacement | A future kit charge is consumed when the attempt result commits, including an authored trigger-on-failure result |
| Climbing | Keep as committed movement | Movement, hands where the edge requires them, attention | Each climb check decides one edge traversal and possible fall. Primary activity ownership would compete with the movement operation. Multi-segment climbs should be an expedition route whose individual crossings still commit synchronously | Movement points and fall consequences commit with each accepted crossing |
| Treat injury / emergency bandage | Keep immediate in combat rules; candidate for an out-of-combat treatment activity | Hands, attention, standard action; stationary and target-watched when timed | Current healing and long cooldown commit together. A later timed treatment must watch the exact patient and cancel on separation, combat, damage, death or extraction; no healing occurs before completion | Reserve a future kit after activity admission. Consume it and start cooldown in the same completion transaction; cancellation releases the reservation |
| Rituals | Timed casting already owns actual cast work; add only per authored ritual | Spell-specific hands, speech, attention and focus; usually stationary and obvious | Use progressive or continuous progress only when authored phases matter. Movement, silence, component loss, countering and target loss use the existing activity responses | Validate components before admission, reserve after admission, consume at the authored irreversible phase, and release unconsumed reservations exactly once |

Progress retention is feature-owned policy, not a manager default. Search is
atomic and retains none. Craft projects persist their existing project progress.
A future treatment is atomic. A ritual may persist completed preparation only if
its authored state is durable and safe to resume; raw runtime handles and pulse
values are never saved.

## Guarded rest

A camp site and a rest session are separate objects. `RAFF_CAMP` describes room
shelter and its world-time lifetime. A guarded-rest session describes a group's
current attempt to recover there. The session has a stable persisted ID, camp
site identity, participant IDs, an ordered watch roster, current stage, supplies,
exposure score, and the wall-clock deadline that advances the stage. Runtime
event handles are reconstructed from that state and are never persisted.

Use explicit stages:

1. **Settle** validates the live camp, participants, roster, weather, and supplies.
   Admission reserves supplies but grants no recovery.
2. **Watch periods** assign one awake watcher and sleeping/resting members.
   Perception consumes typed `PhenomenonPerceived` facts for the watcher. Weather,
   exposure, hostile entry, damage, movement, or a script-directed interruption
   records a typed session interruption before any recovery decision.
3. **Recovery** commits one staged recovery award and the matching supply
   consumption. Awards use a monotonically increasing stage number so restart or
   a duplicate callback cannot grant one stage twice.
4. **Completion/abort** releases unused reservations and removes the deadline.

A watcher may perform informational commands but cannot sleep or begin another
primary activity. Resting participants use a continuous rest activity only while
online; the session remains the persisted authority. Logout removes that member
from active recovery for the stage rather than advancing them offline. Loss of
all eligible participants aborts. A missed deadline processes at most one
interaction-bearing stage per dispatch and schedules the next deadline from the
persisted wall-clock state; it never replays a burst of encounters.

Weather and terrain produce exposure inputs, while camp quality, gear, skills,
and temporary wilderness features mitigate them. Typed perception decides what
the watcher notices. Scripts may request an interruption or add a bounded
encounter candidate through validated commands, but cannot directly grant
recovery or mutate an event handle. This makes watches useful without requiring
a global camp or player scan.

This contract does not replace rolling daily-use recovery. Changing daily uses
to rest-based recovery is a separate balance change with its own migration.

## Expeditions and world events

An expedition is persisted domain state with a small authored state machine. It
is not a long primary activity and does not occupy every passenger. Its identity,
route/site references, participants, state version, deterministic seed, current
stage, completed-stage sequence, and absolute UTC deadlines are authoritative.
Use stable wilderness coordinates/site IDs and database IDs; never persist room
pool indices, pointers, owner generations, scheduler handles, or pulse counts.

The standard deadlines are:

- **departure**: closes admission and commits the participant/resource manifest;
- **warning**: publishes an authored warning to eligible participants/watchers;
- **encounter**: offers or materializes one encounter decision for the current
  stage, with an idempotency key derived from expedition ID and stage sequence;
- **arrival**: commits destination/site arrival after the route state validates;
- **expiry**: retires abandoned offers, temporary world events, or completed
  expedition state after its retention period.

Each feature stores one `next_deadline_kind` and `next_deadline_at`. On boot or
load, reconstruct one native owned event from persisted state. If overdue,
advance through non-interactive bookkeeping in a bounded loop, but process at
most one warning, encounter, arrival, or reward per callback. Persist the new
state and completed-stage sequence before publishing consequences. A duplicate
or stale callback compares expedition ID, state version, kind, and sequence and
does nothing.

Character logout does not stop world-time expedition travel. A character who is
absent at an interactive stage follows the authored absentee policy and receives
no inferred participation credit. Target loss, route/site deletion, insufficient
reserved resources, or failed native admission moves the expedition to an
explicit suspended or aborted state; none silently completes. Operations that
move characters still use the committed relocation contract.

Implement a concrete expedition repository and deadline owner only with the
first authored expedition or world-event consumer. Creating generic database
tables, domain facts, and callbacks before that consumer would leave an unused
second workflow system. The contracts above define the schema and ordering that
consumer must satisfy.

## Verification

Production-linked coverage for the search pilot proves that the hidden result
is absent during work, committed once at the native deadline, and remains hidden
when committed movement cancels the activity. Existing activity-manager coverage
proves damage/combat responses, target movement/death, owner extraction, logout
cleanup, timer admission failure, stale callbacks, pause/resume policy, and
exactly-once completion. Craft coverage proves reservation/progress behavior and
logout/restart reconstruction for the existing durable activity.

Guarded-rest and expedition integration tests must be added with their first
concrete implementations. Required cases are: interruption before recovery,
exactly-once supply consumption and stage reward, watcher target loss, all-member
logout, restart before/at/after every deadline, duplicate callback, overdue
bounded catch-up, removed route/site, failed event admission, and stale owner or
state version. A design-only generic implementation would not make those tests
meaningful.

Plan ablation: reuse `primary_activity`, the native scheduler, committed
movement, room affects, typed perception, and feature-specific persistence. Add
no global activity/rest/expedition scan, second timer service, generic workflow
engine, raw script access to handles, or speculative event types without a
consumer.
