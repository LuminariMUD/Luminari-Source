# Event-Driven Core Native Concrete Owners

**Status:** Accepted 2026-09-01
**Date:** 2026-09-01
**Branch:** `event-driven-core-refactor`
**Scope:** Native autonomous-mobile, primary-activity, and encounter timing

## Delivered Slice

Three existing concrete-owner schedulers now use native semantic types:

- `mobile.autonomous.agenda` owns one NPC with explicit autonomous work;
- `activity.primary.step` owns one character's active wall-clock activity; and
- `combat.encounter.round` owns one active combat encounter.

Each type requires a generation-aware owner and permits at most one event of
that type per owner. Payload ownership transfers only after admission and the
native scheduler performs exactly-once cleanup on completion, cancellation,
shutdown, or failed recurrence.

## Work Model

The mobile callback resolves one already-admitted owner, computes only that
owner's due reason mask, dispatches it once, and returns the next concrete
deadline. Dormant NPCs own no event. Player proximity is not an admission rule,
so off-screen wandering, patrols, hunts, scripted behavior, resource recovery,
and NPC wars continue.

The activity callback advances one active progressive activity. Pause,
interruption, combat-clock transfer, remaining-time preservation, completion,
and callback re-entry retain their existing policy. The deferred Establish
Camp Survival/Nature design decision is unchanged.

The encounter callback advances one encounter's indexed due participants. It
preserves merge and mutation deferral, stable owner generation, deterministic
participant ordering, and one shared six-second semantic round. An earlier
deadline reschedules the existing native event rather than admitting a second
event for the encounter.

No callback discovers work by walking `character_list`, `object_list`, rooms,
or another general population. Recurrence is expressed by returning the next
delay to the single timing wheel.

## Validation

- Production-linked CuTest passes 1,050/1,050.
- Existing mobile tests prove 512 dormant NPCs add no queue work while one
  off-screen wanderer owns and recurs exactly one event.
- Activity tests preserve exact step timing, pause/resume, interruption,
  cancellation during recheck, progress-callback re-entry, and exactly-once
  completion.
- Encounter tests preserve one event per encounter, semantic round cadence,
  participant joins/leaves, callback mutation, generation reuse, and merges.
- Tests assert all three native semantic names are registered and mobile
  deadline queries use native handles.
- The compatibility admission, demand-driven architecture, and PubSub
  retirement source gates pass.
- The production binary builds successfully.
- An isolated copied-world login session on port 4104 reported 13 sealed native
  scheduler types, 38,930 admitted autonomous-mobile owners, 2,318 agenda
  callbacks, zero capacity rejections, zero owner-registry mismatches, and zero
  scheduler failures. Both the isolated instance and the user test instance
  were shut down after validation.

At this slice boundary the compatibility inventory was six schedules across
four production files. The following native service slice reduced it again.

## Next Slice

Named runtime services and persistence work have now migrated. Next migrate DG
waits, MUD events and durable reconstruction, and AI response/retry jobs in
behavior-preserving slices.
