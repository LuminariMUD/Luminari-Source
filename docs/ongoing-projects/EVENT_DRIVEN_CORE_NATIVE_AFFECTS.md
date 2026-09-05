# Event-Driven Core Native Affected Owners

**Status:** Accepted 2026-09-01
**Date:** 2026-09-01
**Branch:** `event-driven-core-refactor`
**Scope:** First production producer migration to native timed-event types

## Delivered Slice

Character-affect expiry now runs as `affected.character.duration`. Room-affect
expiry and room-affect behavior run as `affected.room.duration`. Both types
register during owner-runtime initialization, require a generation-aware owner,
admit at most one event per owner, and remain schedulable after the boot registry
is sealed.

Owners store opaque native runtime handles. Completion, lifecycle cancellation,
cancellation during callback dispatch, scheduler shutdown, subsystem capacity
refill, and room OLC/world-reindex replacement all converge on the native
scheduler lifecycle. Borrowed owner payloads have an explicit no-op destructor;
the owning character or room lifecycle remains responsible for the entity.

When the legacy timed backend is selected, affected-owner scheduling falls back
to the established affected heartbeat. It does not create compatibility-adapter
records for these native producers.

## Gameplay Behavior

- Character affect durations still advance on six-second combat-round
  boundaries.
- Room affect durations still advance on six-second boundaries.
- Room affect behavior still runs on its established five-second boundary.
- Coincident expiry runs before room behavior, preserving the prior ordering.
- Removing the last affect cancels that owner's deadline immediately and makes
  capacity available to another affected owner.
- Room OLC copies preserve the live handle; world-array insertion and deletion
  cancel and reconstruct it against the moved room identity.

## Validation

- The production Autotools binary builds successfully; the only compiler
  warning is the pre-existing suppressed-conversion warning in `players.c`.
- Production-linked CuTest passes 1,050/1,050.
- The affected-owner test seals the registry before scheduling and verifies one
  compatibility type plus both semantic native types on the same runtime.
- Owner inspection resolves the character event to
  `affected.character.duration` and the room event to
  `affected.room.duration`.
- Existing cadence, ordering, admission/refill, cancellation, and OLC/reindex
  tests pass unchanged apart from opaque-handle assertions.
- Physical legacy-queue coverage requests scheduled affects, verifies native
  scheduling remains disabled, and expires both character and room affects
  through the heartbeat rollback path.
- The legacy event admission contract passes with 16 remaining schedules across
  12 production files.
- The demand-driven architecture source contract passes.
- An isolated copied-world scheduler/libevent server reached the game loop on
  port 4104. The immortal diagnostic session reported `types: 3 (sealed)`, zero
  registry mismatch, zero failed events, and scheduled affected owners. It then
  logged out and shut down with the port closed. Redacted evidence is retained
  under `.ci-runtime/native-affected-live.DNbkB5/`.

## Next Slice

Completed by the native `character.maintenance` slice. The next producer group
is object automatic procedures and DG random triggers.
