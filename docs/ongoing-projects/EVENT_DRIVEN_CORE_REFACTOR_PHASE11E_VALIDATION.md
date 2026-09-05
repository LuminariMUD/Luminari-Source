# Event-Driven Core Refactor Phase 11e Validation

**Date:** 2026-08-31

**Scope:** DG trigger-wait owner-handle migration

**Decision:** Implemented; irreversible Phase 11 removal gate remains pending

## Delivered Slice

Phase 11e migrates DG script `wait` timers from public `struct event *`
records to generation-safe `event_handle_t` identities. Trigger creation,
remaining-time display, cancellation, normal completion, OLC replacement, and
room-owner relocation no longer inspect the scheduler compatibility record.

Each waiting trigger retains a private pointer to its wait payload solely so
room OLC can replace the room owner while a script is paused. This pointer is
not a scheduler record, is cleared together with the handle, and remains owned
by the callback or cancellation cleanup path.

## Lifecycle And Gameplay Review

The callback clears the trigger's handle and payload reference before
resuming the script. Cancellation cleanup clears them only when its handle is
still current, so stale cleanup cannot detach a replacement wait. Failed
admission releases the payload and leaves both fields empty.

The DG `wait` grammar, pulse conversion, callback-relative timing, current-line
resume behavior, synchronous damage-trigger result contract, OLC cancellation,
and room replacement behavior are unchanged. Random DG triggers were already
migrated in Phase 11c. Establish Camp and its Survival/Nature ability model are
outside this slice and remain unchanged pending a human gameplay decision.

## Source Inventory

Phase 11d left 86 `struct event *` occurrences across 10 source/header files:
three libevent declarations, 54 private facade references, and 29 external
compatibility references across seven files. Phase 11e leaves 83 occurrences
across eight files: three libevent declarations, 54 private facade references,
and 26 external compatibility references across five files.

All remaining external raw declarations belong to MUD-event storage and its
inspection or persistence paths. Some callers in additional files consume the
same public `pEvent` member without declaring the type; those are included in
the Phase 11f migration audit.

## Validation

- Production build and authoritative local test target: 1,030 C tests plus
  operational, help-sync, installation, healthcheck, PubSub-retirement, vessel,
  and process-memory checks.
- Explicit scheduler and legacy event-backend suites: 1,030 tests each.
- Focused DG damage-wait coverage proves both the handle and OLC payload
  reference are empty after normal completion and that trigger teardown
  cancels and invalidates an outstanding handle.
- AddressSanitizer, UndefinedBehaviorSanitizer, and leak detection: 1,030 tests.
- Strict child-tracing Valgrind: 1,030 tests across 33 logs with zero errors and
  no definite, indirect, or possible leaks.
- Isolated scheduler/libevent MUD on port 4102: real Ornir login, activity and
  event diagnostics, timing/cancellation paths, width checks, signal shutdown,
  and confirmed port closure.

## Next Slice

Phase 11f adds handle-native terminal-completion cleanup and migrates the MUD
event layer, entity event lists, remaining-time consumers, duration changes,
and versioned/legacy persistence inspection. It must preserve exactly-once
cleanup and every reboot, copyover, offline-pause, and rollback contract.
