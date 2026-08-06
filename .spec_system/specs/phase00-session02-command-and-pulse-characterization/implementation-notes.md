# Implementation Notes

**Session ID**: `phase00-session02-command-and-pulse-characterization`
**Started**: 2026-08-06
**Base Commit**: c17acfad37d4877ba9a2bde72592ee0349056c87
**Status**: Complete

---

## Planning And Prerequisites

- `check-prereqs.sh` passed for the development checkout, spec system, jq, and git.
- `analyze-project.sh --json` reported Session 01 complete, no active session, and Session 02 as the
  next incomplete candidate.
- The worktree was clean at the Session 02 base commit.
- Session 01 production-linked fixtures and all 482 tests were already passing.

## Retraced Compatibility Matrix

- `special()` returns immediately for `NOWHERE`; otherwise it visits room, equipment slots,
  carried objects, room mobiles, and room contents, stopping at the first nonzero callback.
- Command callbacks receive the command actor, owner pointer, parsed command index, and command
  argument. Room-mobile dispatch uses the index callback pointer directly and does not require
  `MOB_SPEC`; mobiles marked `MOB_NOTDEADYET` are skipped.
- `command_interpreter()` bypasses `special()` when `no_specials` is set and then runs the ordinary
  command handler.
- `mobile_activity()` requires `MOB_SPEC`, a callback pointer, and `no_specials == 0`. It calls the
  mobile callback as `(ch, ch, 0, "")`; nonzero skips default AI, while a null callback clears
  `MOB_SPEC` and continues.
- `proc_update()` requires `ITEM_AUTOPROC`; an uninitialized weapon (`value[0] == 0`) is skipped. It
  calls the callback with `worn_by` first and, only on zero, with `carried_by`.
- `moving_rooms_update()` decrements each active timer, calls a valid destination room callback as
  `(NULL, moving_room, 0, NULL)` on expiry, ignores the return, and resets the timer.
- Heartbeat calls moving rooms at `PASSES_PER_SEC * 10`; on `PULSE_MOBILE`, it calls
  `mobile_activity()` before `proc_update()`.
- `no_specials` suppresses command and mobile-activity dispatch, but does not gate `proc_update()`,
  `moving_rooms_update()`, or their callbacks.

## Behavioral Quality Focus

- Snapshot every replaced global and restore ownership exactly.
- Capture callback payload values during the call rather than retaining transient pointers.
- Keep random mobile behavior disabled and use the sentinel position-correction branch as the
  deterministic fallback observation.
- Verify heartbeat scheduling narrowly while executing each scheduled callee through production
  code in separate runtime tests.

## Implementation Log

- Created the Session 02 specification and 24-task checklist.
- Added `unittests/CuTest/test_spec_command_pulse.c` with a fixture that snapshots and restores the
  world, index tables, character and object lists, moving-room list, command table, and
  `no_specials`.
- Added a bounded recorder that captures actor, owner, command, nullness, argument text, and
  per-call return behavior without retaining transient argument pointers.
- Added command tests for the complete nine-owner order, every first-nonzero stop, exact payloads,
  `NOWHERE`, pending-extraction mobiles, mobile pointers without `MOB_SPEC`, and runtime
  `command_interpreter()` normal versus `no_specials` behavior.
- Added mobile-activity tests for the `(ch, ch, 0, "")` payload, handled/default-AI split,
  `MOB_SPEC`, `no_specials`, and automatic clearing of `MOB_SPEC` when its callback is absent.
- Added object auto-proc tests for worn actors, null-first carried calls, carrier fallback, unowned
  duplicate-null behavior, flag/weapon/pointer gates, and independence from `no_specials`.
- Added moving-room tests for countdown, reset, `(NULL, moving_room, 0, NULL)`, ignored returns, and
  independence from `no_specials`.
- Added a 1 MiB-bounded `src/comm.c` source-contract check for the ten-second moving-room cadence and
  `mobile_activity()` before `proc_update()` within the mobile pulse.
- Added the new source to both Automake memberships and CMake `CUTEST_TEST_SOURCES`.
- `make -j$(nproc) cutest` compiled the new source with `-Wall -Wextra` and no warning.
- Direct `./cutest` execution passed all 495 tests, including 13 Session 02 tests.
- Independent CMake compilation and `production-cutest` CTest passed.
- Formal review fixed one implicit-widening and two signed-character `clang-tidy` advisories. A
  repeat analysis reports no warning in the new file; only three inherited padding advisories from
  unchanged `src/structs.h` remain.
- `code-review.md` records a RESOLVED result with no open Critical, High, Medium, or Low finding.
- Full `make test` passed seven auxiliary checks and all 495 production-linked CuTests.
- Mandatory `make install` activated release `8fede4096f3ba418314e2bcba118880c286b4b92` and removed
  the root `circle` artifact.
- Checked-in world data retained digest
  `28d30cda73e9dd19e6ea1bf85260aefba0f621d6144401eda421a9fca2be2d98` across validation.
- Encoding, LF, formatting, manifest parity, protected-file, credential, security, and diff-hygiene
  checks passed. `validation.md` and `security-compliance.md` record PASS.
- No production code, world data, or credential-bearing file has been modified.
