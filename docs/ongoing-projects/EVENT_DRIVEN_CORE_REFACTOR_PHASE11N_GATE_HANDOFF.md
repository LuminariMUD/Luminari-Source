# Event-Driven Core Refactor Phase 11n Gate Handoff

**Status:** Development acceptance complete; external release gate remains closed

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

At audited implementation source
`85504e5afec35b073d9d401a32821225354e4fa1`:

- `struct event *` appears 60 times in three files;
- three occurrences are libevent declarations in `src/reactor.c`;
- 57 occurrences are private facade details in
  `src/dgscript/dg_event.c` and `src/dgscript/dg_event_internal.h`;
- no production source outside the facade calls raw creation, cancellation,
  query, or queue APIs;
- the private header has no production include outside the facade; and
- 18 opaque adapter schedules across 13 production files are frozen by the
  legacy-event admission contract.

The normal autonomous-world path is also frozen by a source contract and a
production-linked 512-dormant-plus-one-wanderer test. The agenda callback is
one-owner and reason-aware, while the whole-mobile dispatcher remains
conditional rollback code. Dormant loaded population therefore contributes no
queue entries or callback work.

The opaque callers can survive deletion of the physical legacy queue because
the facade defaults to the timing wheel. Native migration may continue as a
burn-down exercise, but raw-pointer migration is no longer an uncompleted gate
requirement.

## 3. External Evidence

Repository and GitHub inspection on 2026-09-01 confirms the audited commit:

- is contained only by `origin/event-driven-core-refactor`;
- has no containing tag;
- has no GitHub release;
- has no GitHub deployment record;
- has no open, closed, or merged pull request for the branch; and
- is not contained by the default `master` branch.

Build & Test run `33456813692` and Security run `33456854683` are green for the
exact audited SHA, but those are development acceptance and do not substitute
for a production release period. No production database inventory, retention
decision, protected backup, restore rehearsal, or physical-removal approval was
supplied or inferred.

## 4. Validation

- `scripts/events/test_legacy_event_admission.sh` passes.
- `scripts/events/test_pubsub_retirement.sh` passes.
- `scripts/events/test_demand_driven_architecture.sh` passes.
- `make check-world-docs` passes with zero findings.
- `git diff --check` passes.
- An isolated MariaDB 10.11 rehearsal validates the documented discovery SQL,
  consistent dump, SHA-256 verification, disposable restore, equal source and
  restore row counts, routine execution, object checks, and zero foreign-key
  orphans. No existing database was read or modified.
- The current source counts and GitHub gate queries are recorded as reproducible
  commands in the removal inventory.
- The audited source passed 1,048 tests in every backend/driver mode,
  authoritative `make test-all`, sanitizer, strict Valgrind, copied-world
  syntax boot, live-MUD telemetry, Build & Test, and Security.
- A fresh immortal report on the retained copied-world server showed all 14
  default named services live, 40,881 concrete autonomous agendas, and zero
  ready work, overdue age, registry mismatches, or stale-owner outcomes.

## 5. Decision

All currently specified safe development-side preparation is complete.
Deleting rollback code or archival data now would violate the controlling
specification. The next gate-closing state change must come from a
maintainer-approved merge, tagged deployment, observed stable period, and
database rehearsal using
[`EVENT_DRIVEN_CORE_RELEASE_GATE.md`](../deployment/EVENT_DRIVEN_CORE_RELEASE_GATE.md).
