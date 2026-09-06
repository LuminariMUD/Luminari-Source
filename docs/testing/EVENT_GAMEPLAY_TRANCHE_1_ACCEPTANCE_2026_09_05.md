# Tranche 1: door readiness acceptance

Date: 2026-09-05. Branch: `refactor/fight-combat-safety`.
Baseline: `2596c9e464d04ddfc4bc9a863315b1ab34de22a8`.

## Delivered behavior

`ready <command> on door open <direction>` binds one visible closed door to
an existing native scoped subscription. A successful gameplay opening admits
one command for the next native pulse. The normal interpreter enforces action
availability, position and targeting. Entry readiness remains supported.

Door mutations publish final flags after commit, with gameplay/reset/edit
causes and room/exit identity. Verified reciprocal pairs commit together;
asymmetric exits retain their authored behavior. Traps and DG vetoes precede
mutation. Containers and no-op changes do not produce room-door facts.
Replacement, removal and retargeting invalidate the old readiness binding.
A close after triggering cancels execution even if the door reopens immediately.
Movement, death, extraction, cancellation, rearming, logout, copyover and runtime
shutdown use native lifecycle cleanup. An armed action has no periodic timer.

The unused `util/hl_events.c/.h` implementation is deleted. Both physical
scheduler ownership and retired-API checks cover `src/` and `util/`; an isolated
fixture verifies rejection of an alternative utility-tree event API. Current
architecture documentation supersedes the obsolete rollback instructions.

The [mechanism and writer inventory](../systems/EVENT_MECHANISM_INVENTORY.md)
records every audited writer family and the retained countdowns, direct
DG/special/quest gateways, I/O, supervision and data compatibility boundaries.
These retained mechanisms are explicit follow-up work, not claimed migrations.

## Automated verification

| Check | Result |
| --- | --- |
| `make test`, then `make install` | PASS; 1,095 production-linked CuTests plus the make target's script/tool checks. No root server binary left behind. |
| Four event architecture checks | PASS: demand-driven boundaries, retired API admission, singleton native ownership, PubSub retirement. |
| CMake Debug with `BUILD_TESTS=ON` | PASS: all 19 CTest targets. |
| CMake production-linked suite with `LUMINARI_IO_DRIVER=select` | PASS. Reactor tests also exercise both supported I/O drivers. |
| Full Valgrind suite | PASS: 1,095 tests; all 33 process logs report zero errors and zero definite/indirect leaks; no suppressions. |
| Changed C/H formatting | clang-format 18.1.8, matching the repository hook. |
| Patch/file checks | `git diff --check`; new/updated tranche documentation is ASCII with LF. |
| Player help | READY updated in `lib/text/help/help.hlp` and the local development DB's `ready-action` record, with version history and read-back verification. |

Fourteen new tests cover committed pairs, asymmetric/missing exits, lock flags,
no-op and failed commands, real player/NPC command paths, containers, a real DG
veto, all three DG door command families, resets, OLC replacement, retargeting,
visibility, one-tick execution, close/reopen cancellation, owner lifecycle,
extraction during notification, partial subscription admission failure, native
owner quota failure, exhausted actions, and deadline percentile calculation.
Existing suites continue to cover entry readiness, owner lifecycle, special
mechanics, reset/trap behavior, vessels and world reindexing.

Expanding the suite exposed two pre-existing fixture assumptions. One mixed
cadence test now fixes its NPC generation so its phase is independent of earlier
fixtures. Three affected-room cleanup checks now validate their own world before
restoring the surrounding world pointer; checking afterward could inspect an
expired stack fixture and skip cleanup on assertion failure. The final Valgrind
result is after these corrections. Temporary failure instrumentation was removed. A concurrent script-check run
also stalled in release-helper cleanup; its disposable helper was stopped,
the standalone release test passed, and the final make test run was serialized.

Final local logs: `/tmp/door-make-test.log`, `/tmp/door-install.log`,
`/tmp/door-cmake-build.log`, `/tmp/door-ctest.log`,
`/tmp/door-ctest-select.log`, `/tmp/door-verified-valgrind-tests.log`, and
`/tmp/door-verified-valgrind.*.log`. Logs and binaries are not committed.

## Isolated full-world gameplay

The runtime used copied world/player data and the separate local
`luminari_phase3_test` database, inside a private user/network namespace on
port 4103. It booted the same 762-zone, 91,735-room, 27,067-mobile-prototype
archive used by the earlier event-core acceptance. Only the isolated copy
received disposable door/trigger edits in rooms 2 and 3. Ordinary development
port 4101 was not restarted. No production deployment occurred.

| Scenario | Observed result |
| --- | --- |
| Aster waits in room 2; Mirel opens south from room 3 | One opening notification and one explicit readied SAY command; READY afterward reports no armed action. |
| Controlled NPC invokes the room's DG door command trigger | One watched opening and one command, using the same runtime bus and deferred callback. |
| Repeated normal and burst script openings | All 128 measured readied commands executed once. |
| Armed door readiness followed by full-world copyover | Connection recovered; READY reported no readied action; transient state was not restored. |
| Final executable copyover and repeat | Entry readiness cleared; opposite-side player, script and burst door checks each passed once on the final server binary. |
| Cleanup | Isolated MUD and its autorun watchdog stopped using the local supervisor's stop operation. |

Setup findings were isolated-fixture issues: serialized destination 0 means no
exit, so the paired doors were moved to rooms 2/3; level-34 staff are intentionally
excluded from DG command targeting, so the script was invoked through a controlled
NPC. Neither required weakening gameplay checks.

Private evidence: `.ci-runtime/acceptance-20260905/evidence/door-live-session.txt`,
`door-idle-latency.txt`, `door-burst-latency.txt`; final smoke summary in
`/tmp/door-live-final.log`. World/configuration/account data and raw transcripts
remain untracked and must not be published.

Final server ELF build ID: `b89fda1add748d6e619afa73b98c71a0b58f6fe3`.
SHA-256: `3e745f469357f0fd93db0b496b4577316d9ffe9be0356e60842ef7849cfce506`.
The 64-sample runs preceded the final Avernus caller revalidation hardening;
the final executable repeated the three door scenarios after copyover. The
readiness scheduler/measurement implementation was unchanged between these runs.

## Deadline measurements and limits

`eventdebug ready [reset]` keeps a bounded last-1024 sample window. Values are
`actual native dispatch pulse - scheduled deadline pulse`, clamped at zero.
Nearest-rank percentiles exclude the intentional one-pulse readiness delay.
They include entry and door callbacks and omit cancelled callbacks.

| Workload | Samples | p50 | p95 | p99 | Maximum |
| --- | ---: | ---: | ---: | ---: | ---: |
| Normal script opening in otherwise idle player sessions, full-world background work active | 64 | 0 pulses | 0 pulses | 0 pulses | 0 pulses |
| Burst: 200 adjacent-door state writes (including a no-op), then the watched opening, per sample | 64 | 0 pulses | 0 pulses | 0 pulses | 0 pulses |
| Final executable: opposite-side, script, burst smoke | 3 | 0 pulses | 0 pulses | 0 pulses | 0 pulses |

The intended delay is one pulse (nominally 100 ms), not included in these
lateness values. No pulse-level lateness was observed in these bounded samples.
This is not sub-pulse wall-clock or client round-trip latency, a sustained
many-player load test, or proof of a long-term memory trend. There is still no
agreed latency SLA. The broader event-core performance gate remains qualified;
these results do not claim unconditional production readiness.

## Follow-up

Keep the [mechanism inventory](../systems/EVENT_MECHANISM_INVENTORY.md)
open for crafting/self-buffing/transit/supply/mover/staff deadlines, atomic
inventory facts, richer movement causes, activity routing, casting decision
windows, perception and full tabletop Ready semantics. This tranche supplies a
tested fact-to-consumer pattern and removes the orphan scheduler source; it does
not silently absorb those design changes.
