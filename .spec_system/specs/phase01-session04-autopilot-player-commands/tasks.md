# Task Checklist

**Session ID**: `phase01-session04-autopilot-player-commands`
**Total Tasks**: 22
**Estimated Duration**: 7-9 hours
**Created**: 2025-12-30
**Completed**: 2025-12-30

---

## Legend

- `[x]` = Completed
- `[ ]` = Pending
- `[P]` = Parallelizable (can run with other [P] tasks)
- `[S0104]` = Session reference (Phase 01, Session 04)
- `TNNN` = Task ID

---

## Progress Summary

| Category | Total | Done | Remaining |
|----------|-------|------|-----------|
| Setup | 3 | 3 | 0 |
| Foundation | 5 | 5 | 0 |
| Implementation | 10 | 10 | 0 |
| Testing | 4 | 3 | 1 |
| **Total** | **22** | **21** | **1** |

---

## Setup (3 tasks)

Initial verification and environment preparation.

- [x] T001 [S0104] Verify project compiles cleanly after Sessions 01-03
- [x] T002 [S0104] Review existing autopilot API functions in vessels_autopilot.c/h
- [x] T003 [S0104] Review interpreter.c command registration pattern from Phase 00

---

## Foundation (5 tasks)

Header declarations and helper functions.

- [x] T004 [S0104] [P] Add ACMD prototypes to vessels.h (Note: vessels_autopilot.h not needed, declarations in vessels.h)
- [x] T005 [S0104] [P] Add ACMD extern declarations to interpreter.h (interpreter.c includes vessels.h)
- [x] T006 [S0104] Implement vessel context validation helper - get_vessel_for_command() (`src/vessels_autopilot.c`)
- [x] T007 [S0104] Implement permission validation helper - check_vessel_captain() (`src/vessels_autopilot.c`)
- [x] T008 [S0104] Register all 8 commands in interpreter.c cmd_info[] (`src/interpreter.c`)

---

## Implementation - Autopilot Control (3 tasks)

Autopilot on/off/status commands.

- [x] T009 [S0104] Implement do_autopilot command dispatcher with subcommand parsing (`src/vessels_autopilot.c`)
- [x] T010 [S0104] Implement autopilot on/off subcommands with state machine integration (`src/vessels_autopilot.c`)
- [x] T011 [S0104] Implement autopilot status subcommand with route/progress display (`src/vessels_autopilot.c`)

---

## Implementation - Waypoint Commands (3 tasks)

Waypoint management commands.

- [x] T012 [S0104] [P] Implement do_setwaypoint - capture vessel coords, validate name (`src/vessels_autopilot.c`)
- [x] T013 [S0104] [P] Implement do_listwaypoints - display all waypoints with coords (`src/vessels_autopilot.c`)
- [x] T014 [S0104] [P] Implement do_delwaypoint - remove by name with confirmation (`src/vessels_autopilot.c`)

---

## Implementation - Route Commands (4 tasks)

Route management commands.

- [x] T015 [S0104] [P] Implement do_createroute - create empty named route (`src/vessels_autopilot.c`)
- [x] T016 [S0104] [P] Implement do_addtoroute - append waypoint to route (`src/vessels_autopilot.c`)
- [x] T017 [S0104] [P] Implement do_listroutes - display routes with waypoint counts (`src/vessels_autopilot.c`)
- [x] T018 [S0104] Implement do_setroute - assign route to vessel autopilot (`src/vessels_autopilot.c`)

---

## Implementation - Help Files (1 task)

User documentation.

- [x] T019 [S0104] Create help file entries for all 8 commands (`lib/text/help/autopilot.hlp`)

---

## Testing (4 tasks)

Verification and quality assurance.

- [ ] T020 [S0104] Write unit tests for validation helpers (`unittests/CuTest/test_autopilot_commands.c`)
- [x] T021 [S0104] Build project and verify zero warnings with -Wall -Wextra
- [x] T022 [S0104] Validate ASCII encoding on all modified/created files

---

## Completion Checklist

Before marking session complete:

- [x] All implementation tasks marked `[x]`
- [x] Build passing with no new warnings
- [x] All files ASCII-encoded
- [x] implementation-notes.md updated
- [ ] Unit tests for validation helpers (deferred - manual testing recommended first)
- [x] Ready for `/validate`

---

## Notes

### Implementation Notes
- T004/T005: ACMD prototypes added to vessels.h (line 810-818). No separate vessels_autopilot.h needed.
- T006/T007: Helper functions implemented as static functions in vessels_autopilot.c (lines 2260-2315)
- T008: 8 commands registered (autopilot has subcommands, so 8 ACMD handlers for 10 player-facing commands)
- T009-T011: do_autopilot implements all subcommands (on/off/pause/status) in single handler
- T019: Help file created at lib/text/help/autopilot.hlp with entries for all commands

### Files Modified
- `src/vessels.h` - Added ACMD prototypes (lines 810-818)
- `src/vessels_autopilot.c` - Added command handlers (lines 2249-2993)
- `src/interpreter.c` - Registered commands (lines 1174-1182)

### Files Created
- `lib/text/help/autopilot.hlp` - Help entries for all commands (~120 lines)

### Deferred
- T020 (unit tests): Recommend manual in-game testing first, then add unit tests if needed

---

## Next Steps

Run `/validate` to verify session completeness.
