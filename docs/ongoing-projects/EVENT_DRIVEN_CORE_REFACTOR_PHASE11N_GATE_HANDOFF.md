# Event-Driven Core Refactor Phase 11n Gate Handoff

**Status:** Local acceptance complete; external release gate remains closed

**Date:** 2026-09-01

**Branch:** `event-driven-core-refactor`

**Scope:** Current removal inventory and operator evidence procedure

## 1. Delivered Slice

The post-audit removal inventory is now grounded in final source rather than the
Phase 11 baseline. It records the current private-facade state, completed
pointer migration, completed service decomposition, remaining physical rollback
implementations, and exact post-release deletion order.

Permanent deployment documentation now defines how operators must record:

- one tagged, merged, and deployed scheduler/libevent release;
- the maintainer-approved stable-period duration and source identity;
- startup mode, health, event telemetry, copyover, and gameplay evidence;
- explicit and automatic fallback use throughout the period;
- production PubSub object and row discovery;
- a protected full backup and checksum;
- a restore rehearsal in an isolated disposable database; and
- maintainer and production-database sign-off.

The runbook contains no drop migration and grants no production authorization.

## 2. Current Source Evidence

At audited source `43ed4062a4a5f6900c1b994b1c7e5ec12bea1e4f`:

- `struct event *` appears 60 times in three files;
- three occurrences are libevent declarations in `src/reactor.c`;
- 57 occurrences are private facade details in
  `src/dgscript/dg_event.c` and `src/dgscript/dg_event_internal.h`;
- no production source outside the facade calls raw creation, cancellation,
  query, or queue APIs;
- the private header has no production include outside the facade; and
- 18 opaque adapter schedules across 13 production files are frozen by the
  legacy-event admission contract.

The opaque callers can survive deletion of the physical legacy queue because
the facade defaults to the timing wheel. Native migration may continue as a
burn-down exercise, but raw-pointer migration is no longer an uncompleted gate
requirement.

## 3. External Evidence

Repository and GitHub inspection confirms the audited commit:

- is contained only by `origin/event-driven-core-refactor`;
- has no containing tag;
- has no GitHub release;
- has no GitHub deployment record;
- has no open, closed, or merged pull request for the branch; and
- is not contained by the default `master` branch.

Build & Test and Security are green for the commit, but those are development
acceptance and do not substitute for a production release period. No production
database inventory, retention decision, protected backup, restore rehearsal,
or physical-removal approval was supplied or inferred.

## 4. Validation

- `scripts/events/test_legacy_event_admission.sh` passes.
- `scripts/events/test_pubsub_retirement.sh` passes.
- `make check-world-docs` passes with zero findings.
- `git diff --check` passes.
- An isolated MariaDB 10.11 rehearsal validates the documented discovery SQL,
  consistent dump, SHA-256 verification, disposable restore, equal source and
  restore row counts, routine execution, object checks, and zero foreign-key
  orphans. No existing database was read or modified.
- The current source counts and GitHub gate queries are recorded as reproducible
  commands in the removal inventory.
- The preceding source commit passed 1,047 tests in every backend/driver mode,
  authoritative `make test-all`, sanitizer, Valgrind, copied-world syntax boot,
  live-MUD telemetry, Build & Test, and Security.

## 5. Decision

All safe development-side preparation is complete. Deleting rollback code or
archival data now would violate the controlling specification. The next state
change must come from a maintainer-approved merge, tagged deployment, observed
stable period, and database rehearsal using
[`EVENT_DRIVEN_CORE_RELEASE_GATE.md`](../deployment/EVENT_DRIVEN_CORE_RELEASE_GATE.md).
