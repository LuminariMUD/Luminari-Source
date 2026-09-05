# Event-Driven Core Refactor Phase 7 Periodic-Owner Validation

**Status:** Pass
**Date:** 2026-08-30
**Branch:** `event-driven-core-refactor`
**Scope:** Second Phase 7 slice, automatic objects and DG random triggers

## Specification Audit

| Requirement | Disposition |
|---|---|
| Owner deadlines | Every admitted `ITEM_AUTOPROC` object and DG random-trigger script owner receives exactly one deadline spread across its established cadence. |
| Gameplay parity | Auto-procs still run every six seconds. DG random triggers still run every thirteen seconds with their existing percentage checks. |
| Off-screen semantics | Objects are never player-gated. DG mobile and room owners preserve the authored `GLOBAL` empty-zone rule; non-global owners retain empty-zone suppression. |
| Lifecycle | Object registry changes, OLC replacement, DG owner binding, trigger attach/detach, and owner or script extraction schedule or cancel directly. In-flight cancellation prevents recurrence. |
| Bounds | Auto-procs admit 16,384 owners and DG random triggers admit 32,768 combined owners under the 131,072-event queue ceiling. Rejections fail closed and are rate-limited. |
| Diagnostics | `perfmon entities` reports mode, members, scheduled owners, validation mismatches, limits, rejections, callbacks, and DG executions. |
| Rollback | `LUMINARI_AUTOPROC_EVENTS` and `LUMINARI_DG_RANDOM_EVENTS` independently select scheduled or legacy behavior for the entire boot. The two paths cannot execute together. |
| Domain boundary | Scheduler events decide when an owner runs. Typed domain events continue to describe what happened; no synthetic fact is published merely to invoke a periodic callback. |

## Behavioral Boundary

This slice changes how due owners are found, not what their procedures do.
Objects still invoke the same special-procedure gateway, including the inert
unfinished-weapon rule. DG scripts still invoke the same mobile, object, or
room random-trigger function and authored chance.

The existing DG `GLOBAL` flag remains significant. It allows mobile and room
random triggers to advance in an empty zone, while non-global triggers remain
suppressed there. Object random triggers remain independent of player
presence. The separate Avernus room pulse remains an explicit six-second
service in both modes.

## Focused Coverage

The production-linked suite proves:

- automatic object and DG mobile, object, and room owners each schedule once;
- all four callbacks become due and recur without a registry sweep;
- global mobile and room scripts execute in an empty-zone fixture;
- flag removal, owner unbinding, and extraction remove every queued event;
- per-subsystem admission limits reject excess owners without exceeding the
  configured bound;
- independent legacy selections schedule no owner events; and
- source-level heartbeat coverage keeps the legacy calls behind their gates
  while the Avernus room service remains unconditional at six seconds.

## Validation Evidence

- Fresh CMake production build and production-linked `cutest`: pass.
- Normal Autotools build and `make test`: pass, 972/972 C tests.
- Full `make test-all`: pass, including 972 C tests, 504 world-tooling tests,
  protocol, process, supervision, install, health, and character-rename checks.
- Database-enabled production suite against isolated MariaDB: 972/972.
- Rollback matrix: 972/972 in all 16 combinations of auto-proc
  scheduled/legacy, DG random scheduled/legacy, scheduler/legacy event backend,
  and libevent/select I/O driver.
- Syntax boots: all matching 16 combinations loaded the isolated world,
  selected only the requested paths, cleaned up, and reported `Done.`
- ASan and UBSan CMake suite: 972/972 with leak detection, strict string
  checks, stack traces, and halt-on-error enabled.
- Strict Valgrind: 972/972, zero errors and zero definite, indirect, or
  possible leaks.
- Live installed candidate on port 4101: scheduler/libevent boot selected both
  scheduled periodic-owner paths. After a real staff login and 15-second
  observation, all four legacy sweep counters remained zero and
  `perfmon entities` reported both scheduled modes, limits, and zero registry
  mismatches. The reduced local world loads no authored auto-proc or random
  owner; callback execution is therefore established by the production-linked
  owner fixtures rather than persistent test world content.
- Runtime cleanup: the disposable staff fixture was restored to level 1 and
  its prior account credential after logout. Ornir remains level 34. The
  installed candidate remains running on local port 4101.

Local logs are retained under `.ci-runtime/phase7b-*` and are intentionally
untracked.

## Rollback and Next Slice

Restart with either `LUMINARI_AUTOPROC_EVENTS=legacy` or
`LUMINARI_DG_RANDOM_EVENTS=legacy` to restore only that subsystem's heartbeat
scan. Scheduled mode does not execute the corresponding compatibility call.

The next Phase 7 slice should separate connected-player MSDP refresh from
effect-duration expiry, then convert affected character and room owners to
their own deadlines. In game terms, buffs, debuffs, damage-over-time effects,
and room effects should wake when they have work due instead of being found by
a six-second affected-owner scan.
