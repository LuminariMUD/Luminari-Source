# Event-Driven Core Refactor Phase 7C Affected-Owner Validation

**Status:** Pass
**Date:** 2026-08-31
**Branch:** `event-driven-core-refactor`
**Scope:** Third Phase 7 slice, character and room-affect duration owners

## Specification Audit

| Requirement | Disposition |
|---|---|
| Owner deadlines | Every admitted affected character and affected room receives exactly one deadline on the shared D20 round boundary. Multiple affects on one owner share that event. |
| Gameplay parity | Character duration decrement, wear-off messages and side effects, phantom-heal cleanup, death-pact behavior, room timers, shared room bits, and room wear-off text use the existing production routines. |
| Connected state | Affect mutation still refreshes MSDP immediately. The existing one-second connected-descriptor refresh remains; the redundant six-second refresh was removed from duration processing. |
| Lifecycle | First affect attachment schedules directly. Last-affect removal, `unaffect`, extraction, room-affect expiry, room deletion, and shutdown cancel directly. In-flight cancellation prevents recurrence. Room OLC preserves live state on edit and rebuilds ownership after insert/delete reindexing. |
| Bounds | Character admission is capped at 32,768 and room admission at 16,384. Released capacity refills a waiting owner without requiring a heartbeat scan. |
| Diagnostics | `perfmon entities` reports mode, members, scheduled owners, validation mismatches, limits, rejections, callbacks, and nodes processed on six short lines. |
| Rollback | `LUMINARI_AFFECT_EVENTS=legacy` restores the complete six-second compatibility update for both character and room durations. Scheduled and legacy paths cannot execute together. |
| Domain boundary | Scheduler events decide when duration work runs. No synthetic domain fact is published merely to invoke expiry behavior. |

## Behavioral Boundary

This slice changes how affected owners become due, not the meaning or duration
of an affect. Deadlines intentionally align to `PULSE_VIOLENCE`; spreading
initial deadlines would change the ordering between affect expiry and the
existing D20 round procedure. Room-affect behavior that runs on the mixed
five-second Luminari pulse remains for a later Phase 7 slice.

## Focused Coverage

The production-linked suite proves:

- character and room durations decrement and expire on exact round boundaries;
- one character event and one room event process every affect for that owner;
- duplicate room attachment does not duplicate registry membership or events;
- final removal cancels queued work and leaves no registry mismatch;
- ordinary room edits preserve live affects and their deadline, while room
  insertion and deletion relocate surviving affect owners and reschedule them
  on the shared boundary;
- zero-capacity admission fails closed, and released capacity refills waiting
  character and room owners, including during character-registry iteration;
- legacy selection creates no owner events and executes the compatibility
  duration routine; and
- the source-level heartbeat contract keeps the compatibility call behind its
  boot-sealed gate while preserving the independent D20 round procedure.

## Validation Evidence

- Fresh CMake production and test builds: pass; production-linked suite
  976/976 with the isolated specification-world fixture.
- Normal Autotools build and install: pass.
- Full `make test-all`: pass, including 976 C tests, 504 world-tooling tests,
  protocol, process, supervision, install, health, and rename checks.
- Database-enabled suite against the approved isolated `luminari_test`
  database: 976/976; no production database was mutated.
- Rollback matrix: 976/976 in all eight combinations of affected-owner
  scheduled/legacy, scheduler/legacy event backend, and libevent/select I/O.
- ASan and UBSan: 976/976 with leak detection, strict string checks, stack
  traces, and halt-on-error enabled.
- Strict Valgrind memory gate with syntax-child boot disabled: 976/976, zero
  errors and zero definite, indirect, or possible leaks. An initial diagnostic
  traversal exposed a stale test-world pointer; the registry validator now
  rejects pointers outside the current world before dereferencing them.
- Live installed candidate on port 4101: scheduler, scheduled affected owners,
  and libevent selected at boot. A real staff session cast `mage armor`; after
  seven seconds `perfmon entities` reported one member, one scheduled owner,
  one callback, one processed affect, zero mismatch, and zero legacy affect
  sweeps. `unaffect` then removed membership and the deadline immediately.
- In-game affected-owner diagnostics have a measured maximum visible width of
  44 characters, below the normal 80-column client width and the 120-column
  hard requirement.
- Runtime cleanup: disposable character `Phenochar` and its account credential
  were restored to their exact prior level-1 state after logout. Ornir was not
  modified. The installed candidate remains running on local port 4101.

Local logs are retained under `.ci-runtime/phase7c-*` and are intentionally
untracked.

## Rollback and Next Slice

Restart with `LUMINARI_AFFECT_EVENTS=legacy` to restore the complete affected
character and room-duration heartbeat. Scheduled mode does not execute that
compatibility call.

The next Phase 7 slice should migrate explicit character state, beginning with
walk-to progress, regeneration, bardic performance, and hint cadence. The
remaining room behavior and mixed Luminari/point pulses follow before Phase 8
encounter compatibility.
