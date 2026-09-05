# Event-Driven Core Refactor Phase 11g Validation

**Status:** Accepted 2026-08-31
**Scope:** Residual heartbeat decomposition and deadline-driven runtime services

## 1. Delivered Architecture

The default runtime no longer wakes every 100 ms to execute the generic
`heartbeat()` dispatcher. A monotonic runtime tick remains available for pulse-
expressed gameplay deadlines, while 24 named service definitions schedule only
the services required by the selected subsystem modes at their real cadence.
The default live configuration admits 14 recurring service events; rollback-
only definitions are omitted when their owning scheduled subsystem is active.

The reactor now chooses the nearest of descriptor readiness, scheduler work,
an input or queued-action `WAIT_STATE` expiry, and required rollback timing.
Deferred character extraction runs at an explicit post-dispatch safe point.
Minute persistence owns a named dynamic batch-step event rather than relying on
an incidental outer-loop pulse.

Set `LUMINARI_RUNTIME_SERVICES=legacy` before boot to restore the complete
legacy heartbeat dispatcher. Selecting `LUMINARI_EVENT_BACKEND=legacy` also
retains the 100 ms adapter tick required to advance that physical queue. These
paths are mutually exclusive with named runtime-service callbacks.

## 2. Behavioral Contract

- Existing service cadence and ordering inside each cadence group are retained.
- `WAIT_STATE` consumes monotonic elapsed ticks, including after a quiet reactor
  sleep, and queued work contributes its exact expiry to the reactor deadline.
- Scheduled-service admission is all-or-nothing. A startup failure cancels any
  admitted service handles and restores the legacy heartbeat.
- Recurring service and persistence-step handles clear on every terminal path.
- The explicit extraction safe point runs independently of service cadence.
- Named services may call established bounded or global maintenance routines;
  naming a cadence is not evidence that every routine inside it is owner-local.

## 3. Observability

`eventdebug` reports the runtime-service mode, live/configured count, callback
count, and schedule failures on compact lines designed for an 80-column client.
Each service is visible through normal typed queue and profile filters, for
example:

```text
eventdebug type service.one_second 5
eventdebug profile service.one_second
```

PERFMON separately records monotonic runtime advances and legacy heartbeat
replay, so operations can distinguish deadline-driven work from rollback work.

## 4. Validation Evidence

- Production-linked CuTest: 1,034/1,034 passed.
- Complete `make test-all`: 1,034 C tests, 504 world-tool tests, 29 protocol
  tests, 36 help tests, process-memory checks, rename checks, and install passed.
- Four syntax boots covering scheduled/legacy runtime services and
  scheduler/legacy timed backends passed.
- ASan and UBSan: 1,034/1,034 passed with no sanitizer report.
- Valgrind: 1,034/1,034 passed with zero errors and no definite, indirect, or
  possible leaks.
- Isolated live MUD on port 4103: Ornir logged in at level 34; scheduled mode
  reported 14/14 live services, callbacks advanced, the typed service query
  succeeded, output fit 80 columns, and schedule failures remained zero.
- Real copyover: Ornir's descriptor survived the same-PID `exec`; the
  replacement rebuilt scheduler, libevent, and all 14 services; callbacks and
  typed queries worked after recovery; the handoff file and listening port
  cleaned up on shutdown.

## 5. Residual Boundaries

The legacy queue, `select()` driver, generic facade, persistence writer
rollback, and gameplay selectors remain until their stable-release gates are
satisfied. Two ignored-return AI producers still call the raw compatibility
creation API and belong to the zero-caller audit.

Some cadence callbacks intentionally retain broad maintenance work, including
connected-player protocol and supply updates, inventory recharge, time
triggers, timed quests, diplomacy, trails, and world maintenance. The final
zero-caller/adversarial audit must classify each as legitimate global work,
bounded active-owner work, or a remaining scan to migrate. Establish Camp's
historical Survival/Nature rule is unchanged and remains a human design choice.
