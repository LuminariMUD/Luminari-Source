# Task Checklist

**Session ID**: `phase01-session05-npc-pilot-integration`
**Total Tasks**: 20
**Estimated Duration**: 6-8 hours
**Created**: 2025-12-30

---

## Legend

- `[x]` = Completed
- `[ ]` = Pending
- `[P]` = Parallelizable (can run with other [P] tasks)
- `[S0105]` = Session reference (Phase 01, Session 05)
- `TNNN` = Task ID

---

## Progress Summary

| Category | Total | Done | Remaining |
|----------|-------|------|-----------|
| Setup | 2 | 2 | 0 |
| Foundation | 4 | 4 | 0 |
| Implementation | 10 | 10 | 0 |
| Testing | 4 | 4 | 0 |
| **Total** | **20** | **20** | **0** |

---

## Setup (2 tasks)

Initial verification and environment preparation.

- [x] T001 [S0105] Verify autopilot system functional from sessions 01-04 (`src/vessels_autopilot.c`)
- [x] T002 [S0105] Verify ship_crew_roster table exists and has correct schema (`src/db_init.c`)

---

## Foundation (4 tasks)

Core structures, constants, and function prototypes.

- [x] T003 [S0105] Add CREW_ROLE_PILOT constant to vessels.h (`src/vessels.h`)
- [x] T004 [S0105] [P] Add pilot function prototypes to vessels.h (`src/vessels.h`)
- [x] T005 [S0105] [P] Add do_assignpilot and do_unassignpilot ACMD declarations (`src/vessels.h`)
- [x] T006 [S0105] Create help file for assignpilot command (`lib/text/help/assignpilot.hlp`)

---

## Implementation (10 tasks)

Main feature implementation for NPC pilot system.

- [x] T007 [S0105] Implement is_valid_pilot_npc() validation function (`src/vessels_autopilot.c`)
- [x] T008 [S0105] Implement get_pilot_from_ship() helper function (`src/vessels_autopilot.c`)
- [x] T009 [S0105] Implement do_assignpilot() command handler (`src/vessels_autopilot.c`)
- [x] T010 [S0105] Implement do_unassignpilot() command handler (`src/vessels_autopilot.c`)
- [x] T011 [S0105] Register assignpilot and unassignpilot commands (`src/interpreter.c`)
- [x] T012 [S0105] Implement pilot_announce_waypoint() for arrival messages (`src/vessels_autopilot.c`)
- [x] T013 [S0105] Integrate pilot auto-engage in autopilot_tick() (`src/vessels_autopilot.c`)
- [x] T014 [S0105] Implement vessel_db_save_pilot() persistence function (`src/vessels_db.c`)
- [x] T015 [S0105] Implement vessel_db_load_pilot() persistence function (`src/vessels_db.c`)
- [x] T016 [S0105] [P] Create help file for unassignpilot command (`lib/text/help/unassignpilot.hlp`)

---

## Testing (4 tasks)

Verification and quality assurance.

- [x] T017 [S0105] Create test_npc_pilot.c unit test file (`unittests/CuTest/test_npc_pilot.c`)
- [x] T018 [S0105] Add test_npc_pilot.c to CuTest Makefile (`unittests/CuTest/Makefile`)
- [x] T019 [S0105] Run test suite and verify all tests passing (`unittests/CuTest/`)
- [x] T020 [S0105] Validate ASCII encoding and verify clean build (`src/`)

---

## Completion Checklist

Before marking session complete:

- [x] All tasks marked `[x]`
- [x] All tests passing
- [x] All files ASCII-encoded (0-127 characters only)
- [x] Unix LF line endings on all files
- [x] Code follows ANSI C90 conventions (no // comments)
- [x] All compiler warnings resolved
- [x] implementation-notes.md updated
- [x] Ready for `/validate`

---

## Notes

### Task Details

**T007 - is_valid_pilot_npc()**: Validates that an NPC can serve as pilot:
- Must be NPC (`IS_NPC()` check)
- Must be in helm/bridge room of the vessel
- Vessel must not already have a pilot assigned
- Returns TRUE/FALSE with error message to captain

**T008 - get_pilot_from_ship()**: Helper to find pilot NPC from VNUM:
- Searches helm room for mob matching `pilot_mob_vnum`
- Returns `struct char_data *` or NULL
- Used for announcements and validation

**T009-T010 - Command Handlers**: Follow existing patterns from `do_autopilot`:
- Use `get_vessel_for_command()` helper
- Use `check_vessel_captain()` for permission
- Parse NPC target with `get_char_room()`
- Store VNUM via `GET_MOB_VNUM()`

**T012 - pilot_announce_waypoint()**: Called from `handle_waypoint_arrival()`:
- Uses `send_to_ship()` for vessel-wide message
- Format: "Captain <name> announces: 'Arriving at <waypoint>!'"

**T013 - Autopilot Integration**: Modify `autopilot_tick()`:
- Check if `pilot_mob_vnum != -1` (pilot assigned)
- If pilot assigned + route set + autopilot OFF -> auto-engage
- Prevents need for manual `autopilot on` with NPC pilot

**T014-T015 - Persistence**: Use existing `ship_crew_roster` table:
- Save: INSERT with role='pilot', npc_vnum, ship_id
- Load: SELECT on ship load, restore `pilot_mob_vnum`

### Parallelization
Tasks marked `[P]` can be worked on simultaneously:
- T004, T005 (header additions)
- T016 (help file creation)

### Dependencies
- T007-T008 must complete before T009-T010 (validation before commands)
- T009-T010 must complete before T011 (implementation before registration)
- T014-T015 required for persistence testing in T017-T019

### Key Files Reference
- `src/vessels_autopilot.c`: ~2800 lines, main autopilot implementation
- `src/vessels.h`: ~820 lines, prototypes and structures
- `src/vessels_db.c`: ~400 lines, database persistence
- `src/interpreter.c:1175-1183`: Existing autopilot command registrations
- `src/db_init.c:1235-1257`: ship_crew_roster table schema

### Existing Patterns to Follow
- Command registration format: `{"cmd", "cmd", POS_RESTING, do_cmd, 0, 0, FALSE, ACTION_NONE, {0, 0}, NULL}`
- Use `one_argument()` for parsing, `get_char_room()` for NPC lookup
- All functions must have NULL parameter checks with `log("SYSERR: ...")`

---

## Next Steps

Run `/implement` to begin AI-led implementation.
