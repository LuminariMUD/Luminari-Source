# Event-Driven Core Refactor Phase 11a Validation

**Status:** Pass on 2026-08-31

**Scope:** Reversible opaque-handle migration foundation

## 1. Boundary

Phase 11a prepares raw compatibility-event callers for focused migration. It
does not migrate a gameplay owner, remove `struct event`, delete the legacy
queue or `select()` driver, alter persistence, touch the PubSub archival schema,
or change the Survival/Nature and Establish Camp decision.

Every event admitted through either compatibility backend now receives an
opaque `event_handle_t`. Existing pointer callers continue to use the same
record and behavior, while new `event_schedule*()` entry points allow later
slices to stop storing that pointer without reaching into the timing wheel.

## 2. Accepted Contract

| Contract | Result |
|---|---|
| Identity | Nonzero 64-bit handle with one-based 19-bit slot and 45-bit generation |
| Lookup | Fixed bounded registry; constant-time validation without dereferencing a stale event pointer |
| Capacity | Compile-time proof that all 262,144 compatibility events fit the slot field |
| Reuse | Terminal release advances generation before a slot enters the free list |
| Exhaustion | Maximum generation retires its slot permanently; generation never wraps |
| Scheduling | Pointer-free named and owner-aware entry points use the selected compatibility backend |
| Control | Live, queued, remaining-time, and idempotent stale-cancel operations accept only handles |
| Cleanup | Opaque cleanup receives handle plus payload and runs exactly once on queued cancel, in-flight cancel, and shutdown |
| Completion | Callback-owned normal completion invalidates the handle after the callback returns |
| Rollback | Scheduler and legacy queue remain selectable and obey the same handle lifecycle |
| Shutdown | Backend cleanup releases every handle before record destruction; stale process-local values do not revive |

Cleanup executes while its handle still resolves, allowing an owner to detach
the stored value safely. Record destruction then releases the registry slot.
An adversarial review found and corrected the older in-flight self-cancel path,
which had skipped custom cleanup after the callback returned. A positive
recurrence result can no longer override that cancellation, and both backends
converge on one terminal cleanup.

## 3. Focused Coverage

`Test_event_opaque_handles_are_generation_safe_on_both_backends` runs the same
matrix against the legacy queue and timing wheel. It proves:

- nonzero identity, liveness, queued state, and remaining-time queries;
- queued cancellation, cleanup visibility, and stale-operation rejection;
- slot reuse with a different generation;
- normal callback completion and immediate stale invalidation;
- forced maximum-generation retirement without slot reuse or stale aliasing;
- in-flight self-cancel winning over positive recurrence with cleanup once;
- shutdown cleanup once while the handle remains live; and
- zero retained compatibility events after terminal paths.

The generation-exhaustion accelerator exists only in CuTest builds. Production
cannot mutate or decode a handle through the public API.

## 4. Automated Evidence

The final tree passed:

- optimized Autotools production and CuTest builds; the only compiler warning
  is the pre-existing suppressed-`scanf` warning in `src/players.c`;
- scheduler-default C suite: 1,030 tests;
- explicit `LUMINARI_EVENT_BACKEND=legacy` C suite: 1,030 tests;
- CMake Debug AddressSanitizer and UndefinedBehaviorSanitizer suite with leak
  detection: 1,030 tests;
- strict Valgrind with child tracing: 1,030 tests across 33 process logs, zero
  errors, and no definite, indirect, or possible leaks; and
- authoritative `make test-all`: 1,030 production-linked C tests, 504 world
  tool tests, 29 protocol tests, 36 help tests, process-memory and character-
  rename checks, and release installation.

The first unqualified `make test-all` invocation selected this checkout's
partial development world and failed only the unrelated special-procedure
source-inventory assertion. The authoritative rerun supplied
`LUMINARI_TEST_SPEC_WORLD_ROOT=unittests/CuTest/fixtures/spec_world_inventory`
and passed. Evidence logs are retained under `.ci-runtime/phase11a-*` and are
intentionally untracked.

No database or isolated live-MUD mutation was warranted for this additive
infrastructure slice: it changes no schema, persisted format, help, command,
game rule, or active owner. The full suites include production syntax-check
boots and exercise startup, dispatch, cancellation, and shutdown under both
backends. Each owner migration that follows must add its own production-linked
and live gameplay evidence where behavior is exposed.

## 5. Remaining Work

The Phase 11 removal gate remains unsatisfied. Next, migrate event owners in
small reviewed categories to `event_handle_t`, beginning with the already
event-driven Phase 7-10 services and managers. Each slice must preserve owner
teardown, recurrence, diagnostics, and rollback. DG/MUD compatibility and
residual heartbeat services follow. Only a zero-caller audit plus the required
stable release evidence can authorize deletion of the public record, old
queue, compatibility heartbeat, `select()` driver, or archival schema.
