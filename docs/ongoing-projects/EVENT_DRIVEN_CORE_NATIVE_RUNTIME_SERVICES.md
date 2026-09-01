# Event-Driven Core Native Runtime Services

**Status:** Accepted 2026-09-01  
**Date:** 2026-09-01  
**Branch:** `event-driven-core-refactor`  
**Scope:** Native named cadences, persistence batches, and native diagnostics

## Delivered Slice

The 24 named runtime cadences in `src/comm.c` now register and schedule
distinct native service-owner event types. Each type allows one live event for
its singleton owner. The callback performs one service responsibility at its
established boundary and returns that service's next delay.

Incremental player persistence is a separate `service.persistence_batch`
owner event. It performs one bounded save step and recurs one tick later only
while the admitted persistence cycle still has work. Shutdown and terminal
completion invalidate the stored generation-safe handle exactly once.

The scheduler inspection API now exposes all direct native records.
`eventdebug queue`, `eventdebug id`, and filtered views merge those records
with compatibility records while omitting duplicate `legacy_event` wrappers.
Native rows show their semantic type, typed owner, state, and deadline.

## Validation

- The complete 1,050-test CuTest suite passes against the specification-world
  fixture.
- Production-linked tests verify native service ownership, one-per-type
  admission, persistence recurrence and terminal cleanup, shutdown cleanup,
  and legacy-heartbeat rollback.
- Event diagnostics resolve native service, persistence, and encounter types.
- The compatibility inventory is reduced to four calls in three production
  files.
- Architecture, compatibility-admission, and retired-PubSub source gates pass.
- The production server builds successfully.

## Next Slice

Migrate the remaining DG trigger wait to a native owner event, followed by MUD
events and durable reconstruction, then AI response and retry work.
