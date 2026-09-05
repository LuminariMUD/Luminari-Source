# Event-Driven Core Native Character Maintenance

**Status:** Accepted 2026-09-01
**Date:** 2026-09-01
**Branch:** `event-driven-core-refactor`
**Scope:** Native nearest-deadline character maintenance agenda

## Delivered Slice

The existing one-per-character maintenance agenda now runs directly as the
native semantic type `character.maintenance`. Eligible owners store opaque
native handles and use runtime remaining-time inspection, recurrence, and
cancellation. Each generation-aware character owner may hold at most one event.

This is deliberately an agenda rather than a collection of periodic timers.
The event wakes at the owner's nearest concrete deadline, runs only work due at
that tick, computes the next relevant deadline from current owner state, and
retires when no maintenance responsibility remains.

## Gameplay Behavior

- Walk-to actions retain their three-quarter-second cadence.
- Connected PSP recovery retains its five-second cadence.
- Environment/recovery and bardic verse work retain their established
  boundaries.
- Player hints retain their configured cadence.
- D20 cooldowns, damage/effects, and connected-player maintenance retain the
  six-second semantic-round boundary.
- Device recovery remains thirty seconds and timed quests remain one mud hour.
- NPCs with real maintenance state continue off-screen without player-proximity
  admission. Dormant NPCs do not acquire this agenda.
- State changes may move an owner to an earlier deadline by cancelling and
  replacing only that owner's event.

## Lifecycle And Rollback

Extraction, death, disconnection/state changes, completion, callback-time
cancellation, subsystem shutdown, and capacity refill all use native handle or
owner cancellation. A borrowed character payload has an explicit no-op cleanup;
the character lifecycle remains the entity owner.

When the physical legacy timed backend is selected, the native runtime is
absent and character maintenance uses its established heartbeat rollback. No
compatibility-adapter record is created for this producer.

## Validation

- Production-linked CuTest passes 1,050/1,050.
- Registry sealing precedes successful scheduling, and owner inspection reports
  `character.maintenance`.
- Exact nearest-deadline changes, mixed player/NPC work, typed movement
  admission, owner retirement, capacity rejection/refill, and registry health
  pass their existing focused tests.
- Physical legacy-queue coverage proves scheduled mode falls back to heartbeat
  with no queued event.
- The compatibility admission contract passes with 15 remaining schedules
  across 11 production files.
- The production binary and demand-driven architecture gate pass locally.
- A copied-world login session on isolated port 4104 reports the scheduler
  backend, four sealed native types, zero registry mismatches, and zero failed
  events. The server shut down cleanly and released the port after the test.
- The preceding native-affected-owners tranche is green in both remote Build &
  Test and Security workflows.

## Following Slice

Object automatic procedures and DG mobile/object/room random-trigger owners
subsequently migrated as separate semantic native types. Point-update and
vessel owner/service events are the next native owner slice.
