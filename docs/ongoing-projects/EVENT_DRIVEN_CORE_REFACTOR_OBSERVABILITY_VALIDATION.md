# Event-Driven Core Refactor Immortal Observability Validation

**Status:** Pass
**Date:** 2026-08-31
**Branch:** `event-driven-core-refactor`
**Scope:** Dedicated observability gate between Phase 7 owner migration and
Phase 8 encounter compatibility

## Specification Audit

| Requirement | Disposition |
|---|---|
| Readable immortal command | `eventdebug` is an exact-match `LVL_IMMORT` command using the normal MUD pager. Invalid client widths default to 80 columns and every rendered line is hard-capped at 120 columns. |
| Safe live inspection | A backend-neutral intrusive registry records diagnostic identity, callback identity, typed owner identity, deadline, state, recurrence, and lifecycle only. It never stores or renders event payloads. |
| Queue filters | Staff can inspect by diagnostic ID, callback type text, typed owner and optional generation, maximum or bounded deadline, and queued/ready/running/cancel-pending state. Results are deadline ordered and limit bounded. |
| Scheduler health | Summary output includes timing-wheel occupancy, ready backlog, oldest overdue age, dispatch budget, queue high-water, admission rejection, scheduled/cancelled/rescheduled totals, and stale-owner outcomes. |
| Callback timing | `eventdebug types` reports per-callback live count, calls, total and maximum execution time, schedule/cancel/reschedule totals, and recurrence. Existing PERFMON samples retain callback identity for detailed latency analysis. |
| Typed domain events | `eventdebug domain` lists registered domain types and handlers, publication/delivery/drop totals, callback timing, and slow-handler counts without payload inspection. |
| Worker ingress | Bounded I3 game-thread ingress exposes depth, capacity, high-water, rejected items, and wake failures. No unbounded producer queue was added. |
| Stale-owner accounting | Compatibility owners cancel before release. Typed generation-aware consumers introduced from Phase 8 onward call `event_note_stale_owner_outcome()` when owner resolution fails. |
| Backend independence | The live registry and queue views are available under scheduler and legacy timed-event backends. Timing-wheel internals report only when the scheduler owns the queue. |
| Read-only behavior | The command cannot create, cancel, reschedule, execute, or reveal an event. Diagnostic collection does not alter scheduler ordering or dispatch. |

## Staff Workflow

`eventdebug` opens the compact health summary. The queue and profile views are
bounded by default and accept an explicit result limit. `eventdebug help`
documents the supported filters without requiring a web console or filesystem
access. Diagnostic IDs are local to the current server process.

The live acceptance session exercised summary, help, queue, ID, type, owner,
deadline, state, callback-profile, and domain views. It also changed Ornir's
client between 80 and 120 columns and used a ten-line page length to prove that
the normal pager, rather than a single oversized screen, owns long output.

## Focused Coverage

The production-linked C suite proves:

- exact immortal command registration and access level;
- scheduler and legacy registry lifecycle, recurrence, cancellation, and
  teardown behavior;
- deadline ordering plus ID, type, owner, generation, range, and state filters;
- payload redaction and output bounds at invalid, 80-column, and 120-column
  client widths;
- wheel, ready, overdue, lifecycle, capacity, and stale-owner counters;
- callback-profile and typed domain-handler enumeration; and
- bounded I3 ingress depth, high-water, rejection, and wake-failure telemetry.

## Validation Evidence

The final observability tree passed all of the following gates:

- the authoritative isolated Autotools `make test-all` gate, including all 996
  production-linked C tests, 504 world-tool tests, 29 protocol-parser tests,
  deployment and supervision checks, memory tooling, character rename checks,
  and production install validation;
- all 996 C tests with the disposable MariaDB integration path enabled;
- all 996 tests in scheduler/libevent, scheduler/select, legacy/libevent, and
  legacy/select combinations;
- a fresh AddressSanitizer and UndefinedBehaviorSanitizer CMake build and all
  996 C tests with leak detection enabled and no sanitizer finding;
- strict Valgrind execution of all 996 C tests with zero errors and no
  definite, indirect, or possible leaks; and
- a logged live scheduler/libevent MUD session on port 4101 as Ornir at level
  34, with pagination, all command views, payload redaction, zero registry
  mismatch, zero stale-owner outcomes, and programmatic line-width assertions.

The live test restored Ornir's 80-column width and normal page length, logged
out cleanly, shut the server down with normal subsystem cleanup, and restored
the original local account credential byte-for-byte. Ports 4101, 8181, and
8182 were closed afterward. Logs are retained under `.ci-runtime/eventdebug-*`.

## Rollback And Next Slice

Restart with `LUMINARI_EVENT_BACKEND=legacy` to retain the old timed-event
queue while preserving the same read-only staff inspection surface. Individual
Phase 7 subsystem rollback switches remain exclusive and were exercised by the
legacy matrix; no live event conversion is attempted.

Phase 8 introduces encounter-level combat scheduling. Its generation-aware
encounter owner lookup must feed this tranche's stale-owner counter, and its
single encounter event must appear naturally in `eventdebug` without adding a
second diagnostic path.
