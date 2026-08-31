# Event-Driven Core Refactor Phase 5 Validation

**Status:** Pass; offline-pause decision superseded by Phase 11h on 2026-08-31
**Date:** 2026-08-30
**Branch:** `event-driven-core-refactor`
**Scope:** Persistent and reconstructable event ownership

> This document preserves the Phase 5 acceptance evidence. Its offline-pause
> statements describe that historical implementation, not current behavior.
> Phase 11h changed the 93 persisted character events to schema 2 elapsed
> wall-time recovery and added elapsed recovery for older saved counters. See
> `EVENT_DRIVEN_CORE_REFACTOR_PHASE11H_VALIDATION.md` for the current contract.

## 1. Specification Audit

The registry contains 232 usable MUD event types plus the null sentinel. Every
usable type now receives one policy at boot, and a production-linked test walks
the complete registry so a later enum addition cannot escape classification.

| Class | Count | Current members and behavior |
|-------|------:|------------------------------|
| Persisted | 93 | The exact unique set previously emitted by the `Evnt` player-file writer. Remaining game pulses pause while the character is offline. |
| Reconstructable | 1 | `eENCOUNTER_REG_RESET`; region reset work is rebuilt from database-backed region reset data during world boot. |
| Copyover-preserved only | 0 | No current timer has copyover-only durable semantics. Player timers use the persisted path for both copyover and full reboot. |
| Transient | 138 | Combat rounds, casting and preparation, action waits, descriptor protocol work, DG waits, AI requests, room/object effects, and other live-instance work are discarded with their runtime owners. |

The old writer contained 94 active clauses but named 93 unique event IDs because
`eC_DRAGONMOUNT` appeared twice. A mechanical comparison of the active legacy
clauses and the new policy table has no missing or additional IDs. Commented-out
`eQUEST_COMPLETE` and explicitly excluded `eSTRUGGLE` remain transient.

Generic events outside `mud_event_index` are also transient: DG script waits
belong to the running trigger instance and AI request events belong to the
running process. Neither is admitted to the durable player-record API.

## 2. Durable Contract

The new `Evn2` player-file section stores only:

- durable format version and per-event schema version;
- event type;
- stable player ID;
- remaining game pulses and wall-clock save time;
- one validated typed integer for daily-use recovery events.

It does not store a timing-wheel level or slot, scheduler event ID, pointer,
runtime owner ID, or owner generation. Loading validates the type, schema,
stable owner, duration, save time, payload bounds, and duplicates before making
a new event. The active backend therefore assigns a fresh process-local event
ID and the newly loaded character receives a fresh runtime owner generation.

All 93 migrated types retain their previous offline rule: their countdown pauses
while the character is not loaded. The timestamp is retained so a future
per-type migration to elapsed wall time can be explicit and versioned; none is
silently changed in this tranche.

The loader continues to accept legacy `Evnt` sections, but routes them through
the same validation and admission path. Set
`LUMINARI_EVENT_PERSISTENCE_FORMAT=legacy` before boot to restore the old writer.
Readers for both formats remain enabled during the migration window.

## 3. Requirement Alignment

| Phase 5 requirement | Implementation/evidence |
|---------------------|-------------------------|
| Classify every event | Exhaustive policy default plus explicit persisted and reconstructable tables; complete-registry test. |
| Per-type schema | Schema 1 for every persisted type; schema 0 for non-durable types. |
| Owner validation | Stable player ID must match the character being loaded. Runtime identity is never trusted from disk. |
| Offline policy | Explicit pause policy for all existing persisted timers; discard/reconstruct policies for the other classes. |
| Process-local rehydration | Restore creates a new backend event and generation-aware runtime owner. |
| Boot reconstruction | Encounter-region reset work remains rebuilt from database region data during boot. |
| Malformed/stale/duplicate safety | Unknown type, wrong class/schema/owner, future timestamp, invalid duration/payload, and duplicate type are rejected and logged. |
| Rollback | Legacy writer boot switch plus unconditional old-format reader. |
| Cleanup exactly once | Instrumented destruction test clears source and restored events twice and observes one cleanup per event. |

## 4. Validation Evidence

- Production-linked CuTest: pass, 955/955 in all four scheduler/legacy and
  libevent/select combinations.
- Normal Autotools production build and immutable install: pass without new
  compiler warnings.
- ASan and UBSan: pass, 955/955 with leak detection and halt-on-error enabled.
- Valgrind: pass, zero errors and no definite, indirect, or possible leaks.
- Legacy/policy inventory comparison: pass, 93 unique IDs on each side.
- Live disposable character: a 600-second Treat Injury cooldown loaded through
  `Evn2`, saved normally, survived full reboot, and survived logged-in copyover
  on the same descriptor. It showed 597 seconds immediately before copyover and
  596 seconds immediately after recovery.
- Live rollback: a default-format record loaded under a legacy-writer boot,
  emitted as `Evnt`, loaded under a normal boot, and was rewritten as `Evn2`.
- Cleanup: the disposable account, player/database rows, player/object files,
  player index entry, and local server unit were removed after the run.

The live audit exposed and fixed a pre-existing account-menu overwrite: world
extraction saved timers and cleared runtime events correctly, but selecting
character-menu option 0 immediately rewrote the player file from that cleared
menu copy. The redundant save is removed; the extraction save is authoritative.
The fixed path retained the same event line after world logout and account-menu
return.

Sanitized session and server transcripts are retained locally at
`/tmp/phase5-live-final-copyover.log`, `/tmp/phase5-live-final-server.log`,
`/tmp/phase5-live-legacy-writer.log`, and
`/tmp/phase5-live-legacy-reader.log`. The reusable development client expected
an account-logout confirmation that this local text/runtime combination did not
emit, so it timed out only after the tested character had quit and the durable
file assertion had passed; server logs and database/file cleanup confirm no
session or test identity remained.

## 5. Residual Boundaries

This phase makes existing durable cooldowns safer; it does not make transient
combat, spells, scripts, room effects, or activities survive reboot. Those
systems keep their current behavior until their owning migration phases define
their complete state, recurrence, and catch-up rules. The old writer must remain
available through a stable migration window and may not yet be deleted.
