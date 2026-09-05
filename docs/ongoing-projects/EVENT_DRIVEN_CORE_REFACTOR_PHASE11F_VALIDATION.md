# Event-Driven Core Refactor Phase 11f Validation

**Date:** 2026-08-31

**Scope:** MUD-event owner-handle and terminal-cleanup migration

**Decision:** Implemented; irreversible Phase 11 removal gate remains pending

## Delivered Slice

Phase 11f removes the last external `struct event *` declarations from the MUD
event layer. `mud_event_data` stores a generation-safe `event_handle_t`, and
character, descriptor, object, room, region, and world lists store MUD payload
pointers rather than scheduler records. Queries, cooldown displays, combat
handoff, duration and payload replacement, persistence, and cancellation use
opaque handle operations.

The facade now offers an owned terminal-cleanup scheduling form. Unlike the
ordinary cancellation cleanup contract, this destructor runs exactly once on
normal completion as well as queued cancellation, in-flight cancellation, and
shutdown. A positive callback return retains the payload and handle for the
next callback-relative deadline. The facade no longer has an `isMudEvent` flag,
includes no MUD header, and contains no MUD-specific cleanup branch.

## Lifecycle And Gameplay Review

Normal terminal cleanup clears the handle, detaches the payload from its owner,
releases room or region VNUM storage and `sVariables`, and frees the payload.
Bulk owner teardown invalidates the owner generation and detaches the whole
list before requesting cancellation, so an executing callback can finish
reading its payload without touching released owner memory.

Two scripted object callbacks must extract their owner while executing. They
now detach their MUD payload before extraction; scheduler terminal cleanup owns
destruction after the callback returns. Tests cover this in-flight pattern on
both timed backends. Ordinary recurring MUD events retain one list entry and
one handle between firings, then detach and clean up once after their terminal
return.

Registry IDs, callback functions, messages, pulse delays, daily-use recurrence,
cooldown display, action handoff, room and region ownership, and transient
versus durable policy are unchanged. Versioned and legacy player records still
serialize stable event data only and rehydrate a fresh process-local handle.
The existing Survival-based Establish Camp result is untouched; its
Survival/Nature model remains deferred for a human gameplay decision.

## Source Inventory

Phase 11e left 83 `struct event *` occurrences across eight source/header
files: three libevent declarations, 54 private facade references, and 26
external MUD-event references across five files.

Phase 11f leaves 58 occurrences across three files: three unrelated libevent
declarations in `src/reactor.c` and 55 private compatibility-facade references
in `src/dgscript/dg_event.[ch]`. There are no external raw compatibility-record
declarations. Two one-shot AI producers still call `event_create()` without
retaining the returned record; their handle conversion belongs to the
zero-caller release-candidate slice.

## Validation

- Production and test builds compile with no new warning; the historical
  player-file `sscanf` warning remains unchanged.
- The C suite passes 1,032 tests, including terminal completion, positive-return
  recurrence, queued cancellation, in-flight owner cancellation, shutdown,
  stale-handle invalidation, durable restore, and both scheduler and legacy
  timed backends.
- The authoritative `make test-all` operational, database, help, protocol,
  install, and source-policy targets pass.
- AddressSanitizer, UndefinedBehaviorSanitizer, and leak detection pass the full
  C suite.
- Strict child-tracing Valgrind reports zero errors and no definite, indirect,
  or possible leaks.
- An isolated scheduler/libevent MUD boots from installed data, accepts the
  Ornir test login, reports readable event diagnostics, and shuts down with the
  test port closed.

## Next Slice

Phase 11g decomposes residual heartbeat responsibilities into named deadlines,
elapsed-time state, and deferred safe-point drains. It must preserve gameplay
ordering and off-screen world activity rather than replacing the heartbeat
with one generic recurring event. The two ignored-return AI producers can move
to handles in this slice or the following zero-caller audit, but raw facade
removal remains gated on a stable release and maintainer approval.
