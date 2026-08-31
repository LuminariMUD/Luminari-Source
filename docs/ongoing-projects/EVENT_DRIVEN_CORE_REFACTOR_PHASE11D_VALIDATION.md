# Event-Driven Core Refactor Phase 11d Validation

**Date:** 2026-08-31

**Scope:** Vessel and singleton-service owner-handle migration

**Decision:** Implemented; irreversible Phase 11 removal gate remains pending

## Delivered Slice

Phase 11d migrates Greyhawk vessel periodic owners, fixed RoL ship owners, the
global vessel service, and the mud-hour point-update service to
`event_handle_t`. Scheduling, cancellation, and cleanup now use the opaque
handle API on both physical backends.

The audit also proved that `greyhawk_ship_data.action` and
`greyhawk_ship_action_event` had no source caller, scheduler, callback, or
persistence use. That dead compatibility residue is removed. This does not
remove or alter any vessel command or gameplay action.

## Lifecycle And Gameplay Review

Owner handles are cleared before explicit cancellation. Greyhawk and fixed-RoL
cleanup compare the released handle with the owner's current handle, preventing
a stale cleanup from detaching a newer incarnation. Singleton services pass a
borrowed pointer to their handle slot so owner-wide or shutdown cancellation
can detach them even though their callbacks require no gameplay payload.

The half-second Greyhawk chain, 2.5-second fixed-RoL movement, 75-second
schedule/merchant and point-update boundaries, callback ordering, admission
refill, feature toggles, object extraction, ship retirement, and legacy
heartbeat selectors are unchanged. Off-screen and unoccupied vessels continue
to move, fight, trade, pay crews, and encounter hazards when eligible.

## Source Inventory

Phase 11c left 99 `struct event *` occurrences across 14 source/header files,
with 42 external references across 11 files. Phase 11d leaves 86 occurrences
across 10 files: three libevent declarations, 54 private facade references, and
29 external compatibility references across seven files. The only external
categories now are DG trigger waits, MUD-event storage, and MUD-event
inspection/persistence callers.

## Validation

- Production-linked scheduler and explicit legacy suites: 1,030 tests each.
- Focused tests cover exact vessel and point-update boundaries, service
  fallback, feature disable/re-enable, extraction, generation reuse, owner
  admission/refill, fixed-RoL direct lifecycle, registry validation, object
  decay mutation, and rollback exclusivity.
- AddressSanitizer, UndefinedBehaviorSanitizer, and leak detection: 1,030 tests.
- Strict child-tracing Valgrind: 1,030 tests across 33 logs with zero errors and
  no definite, indirect, or possible leaks.
- Authoritative `make test-all`: 1,030 C tests, 504 world-tool tests, 29
  protocol tests, 36 help tests, process-memory and rename checks, and release
  installation.
- Isolated scheduler/libevent MUD on port 4102: real Ornir login, recurring
  service/character activity, compact event diagnostics, timed cancellation
  and completion, signal shutdown, and confirmed port closure.

The reduced live world has no loaded vessel prototypes, so production-linked
tests construct real valid owners and execute their callbacks. The live path
proves integration and service scheduling. Its camp steps are regression only;
the deferred Survival/Nature gameplay decision remains untouched.

## Next Slice

Phase 11e migrates DG trigger waits and the MUD-event compatibility layer,
including list storage, duration changes, persistence inspection, and player
diagnostics. It must preserve versioned/legacy persistence rollback and every
entity lifecycle contract.
