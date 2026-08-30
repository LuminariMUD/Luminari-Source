# Event-Driven Core Refactor Phase 7 Validation

**Status:** Local pass; remote CI pending
**Date:** 2026-08-30
**Branch:** `event-driven-core-refactor`
**Scope:** Scan inventory and first active-world migration, NPC thinking

## Specification Audit

| Requirement | Disposition |
|---|---|
| Inventory every remaining scan | Direct heartbeat consumers are classified by cadence, owner population, reason, and migration disposition in the Phase 7 scan inventory. |
| Active/cooling/dormant registries | Every in-world autonomous NPC is active, including NPCs in player-free rooms. Only out-of-world, extracting, and `MOB_NO_AI` owners become dormant. |
| Scheduled owners | Each admitted autonomous NPC owns exactly one generation-aware event distributed across the established six-second interval. |
| Domain-event wakeups | Character movement re-evaluates only two rooms; combat re-evaluates only its participants; extraction cancels the owner. |
| Genuine global work | Metrics, watchdogs, world clock/weather coordination, persistence admission, and maintenance remain explicitly classified global work. |
| Admission/cancellation bounds | The autonomous mobile registry is capped at 65,536 owners, the production queue at 131,072 events, rejection is rate-limited, and extraction/in-flight cancellation is idempotent. |
| Rollback | `LUMINARI_ACTIVE_WORLD=legacy` selects the old heartbeat scan for the entire boot. The two paths cannot run together. |

## Behavioral Boundary

Every in-world autonomous NPC receives the same `mobile_activity` logic and
six-second recurrence as before. The migration changes discovery and deadline
distribution, not simulation eligibility. Mobs continue to patrol, wander,
hunt, scavenge, invoke scripts and special procedures, become aggressive, and
fight other mobs in zones with no players present.

This boundary is required gameplay parity. A zone war may progress before a
player arrives, and a player may then avoid it or join either side. Player
presence can publish useful facts but never freezes the autonomous world.

## Focused Coverage

The production-linked suite proves:

- an NPC placement movement fact admits the owner without any player present;
- NPCs in both player-occupied and distant rooms retain exactly one event;
- player departure does not suspend either NPC's autonomous behavior;
- due callbacks execute for distant autonomous NPCs;
- combat and extraction facts use generation-aware character resolution;
- extraction cancels the queued owner event;
- a lowered admission ceiling rejects excess NPCs without exceeding the bound;
- legacy mode registers no active-world handlers or NPC events; and
- production runtime registration contains the sensory subscriber plus three
  active-world subscribers.

## Validation Evidence

- Normal Autotools `luminari` and production-linked `cutest` build: pass
  without a new warning.
- Full repository `make test-all`: pass, including 970 C tests, 504 world
  tooling tests, protocol and process-memory checks, isolated character-rename
  schema coverage, and the versioned install path.
- Database-enabled production suite against isolated MariaDB: 970/970.
- Expanded rollback matrix: 970/970 in all eight combinations of active-world
  scheduled/legacy mode, scheduler/legacy event backend, and libevent/select
  I/O driver.
- ASan and UBSan CMake build: 970/970 with leak detection, strict string
  checks, stack traces, and halt-on-error enabled.
- Strict Valgrind: 970/970; zero errors and zero definite, indirect, or
  possible leaks.
- Syntax boots: both `LUMINARI_ACTIVE_WORLD=active` and `legacy` initialize the
  scheduler backend, select only the requested mobile path, load the isolated
  world, clean up, and report `Done.`
- Live candidate on port 4101: a real staff login loaded mobile 3001, waited
  through its six-second cadence, and observed `active=1`, `callbacks=2`, and
  an `active_world_mobile` profiler row with two calls and two recurrences.
  Purge and logout exercised extraction cleanup. The installed candidate was
  then restarted successfully in scheduled/libevent mode.
- Diagnostics: `perfmon reset` now resets active-world callback and rejection
  totals together with the scheduler and entity telemetry it reports.

Local logs are retained under `.ci-runtime/phase7-*` and are intentionally
untracked. Remote workflow and security results will be appended after the
candidate commit is pushed.

## Rollback and Next Slice

Restart with `LUMINARI_ACTIVE_WORLD=legacy`. The compatibility heartbeat then
calls `mobile_activity_pulse()` and scheduled mode admits no NPC owners.

The next Phase 7 slice should convert `ITEM_AUTOPROC` objects and DG random
trigger owners. Both already have lifecycle-maintained eligible registries, so
the migration can replace their cadence scans with owner deadlines without
first discovering eligibility through a global list.
