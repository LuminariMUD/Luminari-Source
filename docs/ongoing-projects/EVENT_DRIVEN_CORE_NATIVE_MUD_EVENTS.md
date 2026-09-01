# Event-Driven Core Native MUD Events

**Status:** Accepted 2026-09-01
**Date:** 2026-09-01
**Branch:** `event-driven-core-refactor`
**Scope:** Native table-driven MUD timers and entity diagnostics

## Delivered Slice

Every usable entry in `mud_event_index` now registers as a distinct native,
owner-required timed type when the normal game scheduler is selected. The 232
names use `mud.<three-digit-id>.<readable-name>`, so an immortal can distinguish
Lay on Hands recovery (`mud.004.lay_on_hands`) from casting, crafting, status,
room, object, encounter-region, and other gameplay timers.

Admission schedules the existing table callback directly on the process timing
wheel. A positive callback return reschedules the same event relative to its
execution time; zero or a negative result completes it. Completion,
cancellation, owner extraction, shutdown, and in-flight self-cancellation all
converge on one cleanup that detaches the entity list and releases the payload
exactly once.

External callers no longer inspect compatibility handles. The backend-neutral
MUD helpers own live-state, remaining-time, and cancellation queries. Duration
or payload replacement and versioned offline cooldown reconstruction use those
same helpers. The physical legacy backend retains one localized adapter branch
until rollback is explicitly removed; the normal path does not enter it.

## Diagnostics

Existing entity-focused commands now show MUD timers naturally:

- `eventdebug player <name> [limit]`
- `eventdebug mob <name> [limit]`
- `eventdebug object <name> [limit]`
- `eventdebug room <here|vnum> [limit]`
- `eventdebug type mud.004 10`
- `eventdebug type lay_on_hands 10`

The entity commands intentionally match all subsystem generations for that
runtime entity, remain bounded and paged, fit the 80-column default and
120-column maximum, and never display payload contents. `eventdebug scripts`
continues to select only `dg.` events on the same entity.

## Validation Contract

- Runtime registration adds exactly 232 MUD semantic types.
- A character-owned Lay on Hands timer appears as
  `mud.004.lay_on_hands` under the character owner.
- Owner generation, recurrence, cancellation during dispatch, terminal cleanup,
  duration queries, durable reconstruction, and elapsed offline recovery retain
  dual-backend coverage.
- The default fixed type capacity is 512, above the current 271 native
  gameplay/service types without allocating per-event dynamic metadata.

The production binary and all 1,051 production-linked CuTests passed locally on
2026-09-01. Live-MUD validation was intentionally deferred because local test
servers remain down at maintainer request.

The next native migration slice owns the two AI response and retry jobs.
