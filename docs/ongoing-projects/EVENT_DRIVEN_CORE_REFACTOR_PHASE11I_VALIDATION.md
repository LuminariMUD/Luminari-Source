# Event-Driven Core Refactor Phase 11i Validation

**Status:** Accepted 2026-08-31
**Date:** 2026-08-31
**Branch:** `event-driven-core-refactor`
**Scope:** Raw-event zero-caller and public-boundary closure

## 1. Delivered Slice

The final raw gameplay producers were the AI response-delivery and request-
retry jobs. Both now use `event_schedule_with_cleanup()` and retain no pointer
to the compatibility record. Their payload destructors release every nested
allocation and the outer payload on cancellation, shutdown, failed admission,
invalid-character exit, and normal completion.

The public `dg_event.h` header now exposes only callback signatures, opaque
handle scheduling/control, backend lifecycle, and payload-free diagnostics.
The compatibility record, raw pointer functions, queue records, and queue
operations moved to `dg_event_internal.h`. Only `dg_event.c` and the two low-
level adapter/syntax test files include that private header.

The legacy physical queue and backend selector are unchanged. This slice closes
the gameplay caller boundary without bypassing the specification's stable-
release requirement for irreversible rollback removal.

## 2. Source Contract

- Raw compatibility creation/control calls outside `dg_event.c` and
  `dg_event_internal.h`: zero.
- `struct event *` declarations outside the private facade, its two low-level
  tests, and libevent's unrelated `reactor.c` type: zero.
- `dg_event_internal.h` production includes outside `dg_event.c`: zero.
- Public `dg_event.h` raw record, pointer-function, and queue declarations:
  zero.

## 3. Lifecycle Review

AI callbacks continue to run on their established timing and preserve their
response delivery, character-presence, same-room, logging, retry, and backend
selection behavior. Cleanup is now one shared path per payload type. In
particular, response exits caused by missing or extracted characters also free
the backend string, which the prior ad hoc exits omitted.

Admission failure remains caller-owned: if handle scheduling returns
`EVENT_HANDLE_NONE`, the producer invokes its destructor immediately and logs
the failed queue operation. Once admitted, the selected timed backend owns the
payload and invokes the same destructor during cancellation or shutdown.

## 4. Validation Evidence

- Production Autotools build passed after the public/private header split.
- Production-linked CuTest passed 1,038/1,038 in every supported runtime
  combination:
  - scheduler/libevent;
  - scheduler/`select()`;
  - legacy queue/libevent; and
  - legacy queue/`select()`.
- The new focused test schedules both AI payload types, shuts each timed
  backend down, and proves both destructors run exactly once with an empty
  queue afterward.
- `make test-all` passed: 1,038 C tests, 504 world-tool tests, 29 protocol
  tests, 36 help-system tests with 8 intentional skips, process-memory tests,
  player-rename tests, and release install verification.
- The production-linked 1,038-test binary passed with AddressSanitizer and
  UndefinedBehaviorSanitizer enabled, with no sanitizer report.
- The same 1,038 tests passed under Valgrind with zero definite, indirect, or
  possible leaks and zero errors. The remaining 1,721,233 reachable bytes are
  process-lifetime test fixtures and globals, not lost allocations.
- Syntax-check boot passed in all four normal backend/driver runs and in the
  complete suite. It was intentionally skipped only inside the sanitizer and
  Valgrind parent processes because it launches a separately instrumented
  child.
- A separate live-MUD session was not required for this boundary-only change:
  no command, timing, persistence, network, or gameplay behavior changed, and
  the affected scheduling/cancellation paths are exercised through the
  production-linked binary on both timed backends. Phase 11h already supplied
  the adjacent login/logout and real-copyover live evidence.
- Remote Build & Test run `33411294759` and Security run `33411294824` both
  passed on the pushed branch commit.

## 5. Deferred Removal

The old queue, backend selector, `select()` driver, legacy persistence writer,
and archival PubSub schema remain. Their removal still requires the stable
release, no-rollback-dependency, maintainer-approval, and database backup/
restore evidence defined by Phase 11. This tranche does not claim those
external gates have occurred.
