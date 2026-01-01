# Task Checklist

**Session ID**: `phase03-session04-dynamic-wilderness-rooms`
**Total Tasks**: 20
**Estimated Duration**: 6-8 hours
**Created**: 2025-12-30

---

## Legend

- `[x]` = Completed
- `[ ]` = Pending
- `[P]` = Parallelizable (can run with other [P] tasks)
- `[S0304]` = Session reference (Phase 03, Session 04)
- `TNNN` = Task ID

---

## Progress Summary

| Category | Total | Done | Remaining |
|----------|-------|------|-----------|
| Setup | 3 | 3 | 0 |
| Foundation | 5 | 5 | 0 |
| Implementation | 7 | 7 | 0 |
| Testing | 5 | 5 | 0 |
| **Total** | **20** | **20** | **0** |

---

## Setup (3 tasks)

Initial configuration and environment preparation.

- [x] T001 [S0304] Verify prerequisites - confirm session03 complete, wilderness.c functions exist
- [x] T002 [S0304] Review existing code paths in `move_vessel_toward_waypoint()` (`src/vessels_autopilot.c`)
- [x] T003 [S0304] Review existing `update_ship_wilderness_position()` implementation (`src/vessels.c`)

---

## Foundation (5 tasks)

Core analysis and header updates.

- [x] T004 [S0304] Analyze `find_available_wilderness_room()` in wilderness.c (line ~839)
- [x] T005 [S0304] Analyze `assign_wilderness_room()` in wilderness.c (line ~874)
- [x] T006 [S0304] Analyze `event_check_occupied()` room recycling logic (line ~1441)
- [x] T007 [S0304] [P] Create test file scaffold `test_vessel_wilderness_rooms.c` (`unittests/CuTest/test_vessel_wilderness_rooms.c`)
- [x] T008 [S0304] [P] Add any needed helper function prototypes (`src/vessels.h`) - Already declared

---

## Implementation (7 tasks)

Main feature implementation - autopilot movement fix and room lifecycle.

- [x] T009 [S0304] Fix `move_vessel_toward_waypoint()` to call `update_ship_wilderness_position()` (`src/vessels_autopilot.c`)
- [x] T010 [S0304] Verify ship object movement via `obj_from_room()`/`obj_to_room()` sequence (`src/vessels.c`)
- [x] T011 [S0304] Add departure logging/notification for debugging room cleanup (`src/vessels.c`)
- [x] T012 [S0304] Ensure ROOM_OCCUPIED flag lifecycle is correct for vessel movement (`src/vessels.c`)
- [x] T013 [S0304] [P] Add test suite registration to AllTests.c (`unittests/CuTest/AllTests.c`) - Using standalone exe
- [x] T014 [S0304] [P] Update Makefile for new test file (`unittests/CuTest/Makefile`)
- [x] T015 [S0304] Validate multi-vessel same-coordinate handling works correctly (`src/vessels.c`) - Already supported

---

## Testing (5 tasks)

Verification and quality assurance.

- [x] T016 [S0304] Write unit tests for room allocation (6 tests) (`unittests/CuTest/test_vessel_wilderness_rooms.c`)
- [x] T017 [S0304] Write unit tests for ship position updates (4 tests) (`unittests/CuTest/test_vessel_wilderness_rooms.c`)
- [x] T018 [S0304] Write unit tests for multi-vessel and edge cases (4 tests) (`unittests/CuTest/test_vessel_wilderness_rooms.c`)
- [x] T019 [S0304] Build and run test suite - verify all 14+ tests pass
- [x] T020 [S0304] Validate ASCII encoding and run Valgrind memory check

---

## Completion Checklist

Before marking session complete:

- [x] All tasks marked `[x]`
- [x] All 14+ unit tests passing
- [x] All files ASCII-encoded (0-127 characters only)
- [x] Valgrind clean (no memory leaks)
- [x] implementation-notes.md updated
- [x] Ready for `/validate`

---

## Notes

### Parallelization
Tasks marked `[P]` can be worked on simultaneously:
- T007/T008: Test scaffold and header updates are independent
- T013/T014: AllTests.c and Makefile updates are independent

### Task Timing
Target ~20-25 minutes per task.

### Key Code Locations
- `src/vessels_autopilot.c`: `move_vessel_toward_waypoint()` - main fix location
- `src/vessels.c`: `update_ship_wilderness_position()`, `get_or_allocate_wilderness_room()`
- `src/wilderness.c`: `find_available_wilderness_room()` (~line 839), `assign_wilderness_room()` (~line 874), `event_check_occupied()` (~line 1441)
- `unittests/CuTest/`: Test framework location

### Critical Integration Points
1. `move_vessel_toward_waypoint()` must call `update_ship_wilderness_position()` instead of inline allocation
2. Ship objects must be moved via `obj_from_room()`/`obj_to_room()` to allow room recycling
3. `event_check_occupied()` handles ROOM_OCCUPIED cleanup automatically when rooms are empty

### Dependencies
Complete tasks in order unless marked `[P]`:
- T001-T003: Setup must complete first
- T004-T006: Foundation analysis informs implementation
- T007/T008: Can run parallel during foundation
- T009-T015: Implementation is mostly sequential
- T016-T020: Testing requires implementation complete

---

## Next Steps

Run `/implement` to begin AI-led implementation.
