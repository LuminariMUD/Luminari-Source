# Event-Driven Core Native Runtime Foundation

**Status:** Accepted 2026-09-01
**Date:** 2026-09-01
**Branch:** `event-driven-core-refactor`
**Scope:** Single-wheel ownership and boot-sealed native type API

## Delivered Slice

`event_runtime` now exclusively owns the process timing wheel. It exposes
semantic type registration, opaque native handles, owned and unowned
scheduling, cancellation, owner cancellation, rescheduling, remaining time,
advancement, nearest deadline, inspection, and aggregate statistics without
exposing the process scheduler singleton for modules to own or store.

The scheduler type registry can be sealed and inspected by stable type name.
Normal boot, syntax-check boot, and copyover boot seal it after world and
runtime-service initialization. Scheduling registered types remains legal;
late registration is rejected explicitly.

The DG facade remains temporarily available for rollback and caller migration,
but it is now a client of the runtime. It registers `legacy_event` on the same
wheel and no longer creates, owns, advances, inspects, or destroys a scheduler.
The foundation initially retained 18 compatibility schedules in 13 production
files, frozen by the admission test. The first producer slice has since moved
the two affected-owner schedules to native semantic types.

## Lifecycle Guarantees

- One runtime and one wheel exist per process generation.
- Native event identities are monotonically allocated and never reused, so a
  stale handle cannot alias a later event.
- Payload ownership transfers only after successful admission.
- Completion, cancellation, owner cancellation, shutdown, and failed
  recurrence converge on the scheduler's exactly-once cleanup path.
- Copyover constructs a fresh runtime through the normal boot sequence and
  rebuilds persisted gameplay timers from their owning subsystems.

## Validation

- Production Autotools binary built successfully.
- Production-linked CuTest passed 1,050/1,050 with the authoritative special-
  procedure fixture root.
- New mixed-runtime coverage dispatched two semantic native types and one
  compatibility callback from one wheel in deadline/FIFO order, including a
  native recurrence and exactly-once cleanup.
- New owner coverage cancelled an owned native event, invalidated its stale
  handle, and ran cleanup exactly once.
- Late type registration after sealing was rejected.
- Legacy event admission and demand-driven architecture source gates passed.
- An isolated copied-world scheduler/libevent server reached the game loop on
  port 4104. Codexadmin logged in at level 34 and captured two 80-column
  `eventdebug` reports: about 41,000 bounded live events, about 38,800 off-screen
  autonomous agendas, zero registry mismatches, and zero failed events. The
  server then shut down cleanly. Evidence is retained under
  `.ci-runtime/native-runtime-live.Xm1uf7/`.

## First Producer Slice

`affected.character.duration` and `affected.room.duration` are now native
owner-required types. Their owner-local callbacks preserve established affect
expiry, room behavior, cancellation, capacity refill, and OLC/reindex behavior.
The frozen compatibility inventory is now four schedules across three files
after the subsequent character-maintenance, automatic-procedure, DG random-
trigger, mud-hour, vessel, autonomous-mobile, primary-activity, and encounter
migrations plus native named services and persistence batches. Thirty-eight
gameplay/service semantic types now schedule directly on the native runtime.
The subsequent DG-wait slice raises that total to 39 with native
`dg.trigger.wait`; one localized adapter remains solely for physical legacy
rollback, so the frozen compatibility-call count does not change in that slice.

Remaining callbacks continue to migrate in behavior-preserving groups. Each
must retain its cadence, owner lifecycle, cleanup, and recurrence policy while
becoming visible by semantic scheduler type rather than only by compatibility
profile.
