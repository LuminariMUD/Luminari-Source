# Session Specification

**Session ID**: `phase00-session02-command-and-pulse-characterization`
**Phase**: 00 - Registry Safety and Observability
**Status**: Complete
**Created**: 2026-08-06
**Completed**: 2026-08-06
**Base Commit**: c17acfad37d4877ba9a2bde72592ee0349056c87

---

## 1. Session Overview

This session establishes executable compatibility evidence for command-owner traversal and the
scheduled non-combat special-procedure paths. Later Phase 00 sessions will introduce event gateways
and safer dispatch, so the present ordering, payload, activation, return, and `no_specials` behavior
must first be frozen against the production implementation.

The session adds a production-linked CuTest suite with isolated world, character, object, moving
room, and command-table fixtures. It invokes `special()`, `command_interpreter()`,
`mobile_activity()`, `proc_update()`, and `moving_rooms_update()` directly, and verifies heartbeat
scheduling against the production source. It does not change dispatch behavior or application code.

---

## 2. Objectives

1. Freeze command-owner traversal order and first-nonzero stop behavior.
2. Freeze mobile activity callback activation, payload, return handling, and default-AI fallback.
3. Freeze equipped and carried object auto-proc actor/fallback behavior and activation gates.
4. Freeze moving-room timing and callback payload, plus heartbeat placement and ordering.
5. Make applicable normal versus `no_specials` differences explicit.

---

## 3. Prerequisites

### Required Sessions

- Session 01 is complete and its production-linked fixture/tests remain passing.

### Required Tools Or Knowledge

- Root CuTest harness and generated `AllTests.c` registry.
- Current command dispatch in `src/interpreter.c`.
- Current mobile AI in `src/mob/mob_act.c`, object auto-procs in `src/comm.c`, and moving-room
  callbacks in `src/db.c`.
- Current heartbeat schedule in `src/comm.c`.

### Environment Requirements

- Development checkout; `lib/.env` and `lib/mysql_config` remain read-only.
- All mutable production globals used by a scenario must be restored before the test returns.
- `make test` must be followed by `make install`.

---

## 4. Scope

### In Scope (MVP)

- Engine maintainer can verify room, equipment, inventory, room-mobile, and room-object command
  owners execute in that order and stop after the first nonzero result.
- Test maintainer can verify exact command callback actor, owner, command, and argument payloads,
  including `NOWHERE` rejection and pending-extraction mobile skipping.
- Engine maintainer can verify mobile callbacks require `MOB_SPEC`, use `(ch, ch, 0, "")`, and use
  their return to suppress or permit current default AI.
- Engine maintainer can verify a missing mobile callback clears `MOB_SPEC` and that `no_specials`
  suppresses mobile callbacks while permitting current default AI.
- Engine maintainer can verify `ITEM_AUTOPROC` object callbacks, worn-actor calls, carried-object
  null-actor first calls, conditional carrier fallback, and current weapon-value gating.
- Engine maintainer can verify moving-room countdown/reset behavior, `(NULL, moving_room, 0, NULL)`
  payload, ignored callback return, and independence from `no_specials`.
- Engine maintainer can verify moving rooms run on the ten-second heartbeat and that
  `mobile_activity()` precedes `proc_update()` on the mobile pulse.

### Out Of Scope (Deferred)

- Combat, identification, maneuver, shop, and quest invocation categories - Session 03 owns these
  characterization paths.
- Successor caching, extraction safety, and event gateway changes - later Phase 00 implementation
  sessions own intentional lifetime and dispatch changes.
- Registry metadata, OLC owner filtering, authored names, and binding provenance - Sessions 04
  through 08 own these changes.

---

## 5. Technical Approach

### Architecture

Add one focused CuTest source. A fixture snapshots and restores `world`, object and mobile index
tables, object and character lists, moving-room state, `no_specials`, and the minimal command table.
It builds stack-owned owners with deterministic linked-list order and uses one recorder callback to
capture every actor, owner, command, argument, and configured return.

Runtime tests invoke production entry points for all dispatch and pulse behavior. The heartbeat test
uses a bounded source-contract helper rooted at `LUMINARI_TEST_ROOT`; calling the entire heartbeat
would execute unrelated zone, combat, persistence, and network systems and would not isolate the two
scheduling requirements. Runtime tests still exercise both scheduled callees and their payloads.

### Design Patterns

- Snapshot/restore fixture: prevents process-global state from leaking into the existing suite.
- Table-driven owner recorder: proves traversal and stop positions without duplicating callbacks.
- Deterministic fallback state: distinguishes a handled mobile callback from default AI by current
  position correction.
- Production source contract: narrowly freezes heartbeat interval and relative call order while
  runtime tests freeze the callees themselves.

---

## 6. Deliverables

### Files To Create

| File | Purpose | Est. Lines |
|------|---------|------------|
| `unittests/CuTest/test_spec_command_pulse.c` | Command and non-combat pulse characterization | ~650 |

### Files To Modify

| File | Changes | Est. Lines |
|------|---------|------------|
| `Makefile.am` | Add the suite to CuTest compile and generated-test inputs | ~2 |
| `CMakeLists.txt` | Add the suite to `CUTEST_TEST_SOURCES` | ~1 |

---

## 7. Success Criteria

### Functional Requirements

- [x] Every command owner executes in verified order, receives the exact payload, and a handled
  result prevents all later owners from running.
- [x] Command dispatch demonstrates current `NOWHERE`, pending-mobile, direct mobile-pointer, and
  `no_specials` behavior.
- [x] Mobile activity demonstrates the exact `MOB_SPEC`, missing-pointer, `no_specials`, return, and
  default-AI contracts.
- [x] Object auto-procs demonstrate worn and carried actor selection, fallback returns,
  `ITEM_AUTOPROC`, inert-weapon, callback-pointer, and `no_specials` contracts.
- [x] Moving rooms demonstrate countdown/reset, exact null payload, ignored return, and
  `no_specials` independence.
- [x] Heartbeat source evidence demonstrates the ten-second moving-room cadence and
  `mobile_activity()` before `proc_update()`.

### Testing Requirements

- [x] New tests run from the generated root `cutest` executable.
- [x] Root `make test` passes, followed by a passing `make install`.
- [x] Test teardown leaves no mutated global state or root-level `circle` artifact.

### Non-Functional Requirements

- [x] No production dispatch behavior, callback ABI, activation flag, or schedule changes.
- [x] Automake and CMake test membership remains synchronized.
- [x] All bounded string and source reads report setup failure instead of overrunning buffers.

### Quality Gates

- [x] All files ASCII-encoded with Unix LF endings.
- [x] Code follows project conventions.
- [x] Zero new `-Wall -Wextra` warnings.

---

## 8. Implementation Notes

### Working Assumptions

- `special()` traverses room, equipment slots, inventory, room mobiles, and room contents in that
  exact order and calls mobile pointers without requiring `MOB_SPEC`.
- `mobile_activity()` invokes enabled mobile callbacks before default AI and treats nonzero as
  handled; `no_specials` suppresses this path only.
- `proc_update()` calls an eligible carried object first with a null actor, then with `carried_by`
  only when the first call returns zero.
- `moving_rooms_update()` ignores callback returns and is not gated by `no_specials`.

### Key Considerations

- Linked-list insertion order is explicit in every fixture so traversal assertions are stable.
- Recorder arguments are copied immediately because production callbacks may receive mutable or
  transient buffers.
- Source-contract matching includes the surrounding pulse conditions, not merely unrelated symbol
  occurrences.

### Potential Challenges

- Legacy AI touches broad character state after a zero callback: initialize the exact sentinel
  branch prerequisites and keep random behaviors disabled with mobile flags.
- `command_interpreter()` depends on the command table: create only the minimum valid command row
  when the global table is absent, then restore ownership exactly.
- Global object and character lists are shared by the full test executable: fixture teardown must
  restore original heads and index tables on every path.

### Behavioral Quality Focus

Checklist active: Yes
Top behavioral risks for this session:
- A fixture can leave linked-list or index pointers aimed at expired stack storage.
- A callback recorder can mistake fallback calls when the actor is intentionally null.
- A broad heartbeat invocation can produce false failures through unrelated systems, so the
  schedule assertion must stay narrowly source-bound while callee behavior is tested at runtime.

---

## 9. Testing Strategy

### Production-Linked Tests

- Build an all-zero command graph and compare the complete owner and payload sequence.
- Configure every command owner in turn as handled and assert the exact traversal prefix.
- Exercise `NOWHERE`, pending-extraction mobiles, direct mobile pointer dispatch, and
  `command_interpreter()` with normal and `no_specials` modes.
- Exercise mobile callback handled, unhandled, disabled, and missing-pointer paths with deterministic
  default-AI evidence.
- Exercise worn, carried, unowned, ineligible, inert-weapon, and missing-pointer object paths.
- Exercise moving-room ticks, reset, return variation, and `no_specials` behavior.
- Read `src/comm.c` and assert the precise ten-second moving-room condition and mobile pulse call
  order.

### Regression And Build Gates

- Run the generated production-linked suite through `make test`.
- Run CMake build and CTest if the new source exposes generator-specific membership differences.
- Run targeted formatting/static checks for the new test source.
- Run `make install` and verify no root-level `circle` remains.

---

## 10. Security And Reliability

- No credential-bearing file is modified or emitted.
- Source reads are repository-rooted and bounded.
- Test callbacks do not retain pointers beyond a scenario.
- All global pointers and ownership flags are restored even after assertion collection.

---

## 11. Completion Definition

The session is complete when all tasks are checked, review has no unresolved blocker, validation and
security gates pass, the PRD/session state is updated, `make test` and `make install` pass, and the
committed result is published on the existing development branch.
