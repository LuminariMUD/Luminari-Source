# Event-Driven Core Native Periodic Owners

**Status:** Accepted 2026-09-01
**Date:** 2026-09-01
**Branch:** `event-driven-core-refactor`
**Scope:** Native automatic-procedure and DG random-trigger owner deadlines

## Delivered Slice

Automatic object procedures now run as `object.automatic_procedure`. DG mobile,
object, and room random triggers run as `dg.random_trigger`. Both are boot-time
registered, owner-required native types with one event per type and owner.
Objects and scripts store opaque runtime handles and schedule, recur, inspect,
and cancel without entering the compatibility facade.

These are two concrete gameplay responsibilities, not class-wide schedulers.
Each callback receives one already-selected owner and performs only that
owner's automatic procedure or random-trigger check. Neither callback walks an
object, mobile, room, zone, or script population to discover work.

## Gameplay Behavior

- Automatic objects retain their callback-relative six-second cadence and the
  unfinished-weapon suppression rule.
- DG random triggers retain their callback-relative thirteen-second cadence
  and authored percentage checks.
- Automatic objects and DG object triggers continue regardless of player
  presence.
- DG mobile and room triggers retain their existing behavior: `GLOBAL` owners
  run in empty zones, while non-global owners wait until the zone is occupied.
- Lifecycle registry changes, extraction, OLC replacement, trigger attach or
  detach, owner rebinding, callback-time cancellation, and shutdown cancel the
  corresponding native event.

## Rollback

`LUMINARI_AUTOPROC_EVENTS` and `LUMINARI_DG_RANDOM_EVENTS` remain independent
boot-time selectors during the temporary rollback period. When the physical
legacy timed backend is selected, native registration is unavailable and each
requested subsystem automatically uses its established heartbeat path. No
compatibility-adapter event is created for either producer.

## Validation

- The production-linked suite passes 1,050/1,050.
- Registry tests seal three types in isolation (`legacy_event` plus the two
  native types), inspect one automatic-object event and all three DG owner
  kinds, and verify their semantic names.
- Existing coverage verifies distributed initial deadlines, recurrence,
  callbacks and executions, owner unbinding, flag removal, complete queue
  teardown, admission bounds, and physical legacy-backend fallback.
- The compatibility inventory gate passes with 13 schedules across 10
  production files, and the demand-driven architecture gate remains green.
- The production binary builds successfully.
- A copied-world staff login on isolated port 4104 reports the scheduler
  backend, six sealed native types, zero registry mismatches, and zero failed
  events. The server shut down cleanly and released the port.
- The preceding character-maintenance commit is green in both remote Build &
  Test and Security workflows.

## Next Slice

Migrate point-update and vessel owner/service events to native semantic types,
preserving their owner-local cadences, singleton responsibilities, and legacy
heartbeat fallback.
