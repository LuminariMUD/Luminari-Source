# Task Checklist

**Session ID**: `phase01-session01-autopilot-data-structures`
**Total Tasks**: 21
**Estimated Duration**: 6-8 hours
**Created**: 2025-12-30
**Completed**: 2025-12-30

---

## Legend

- `[x]` = Completed
- `[ ]` = Pending
- `[P]` = Parallelizable (can run with other [P] tasks)
- `[S0101]` = Session reference (Phase 01, Session 01)
- `TNNN` = Task ID

---

## Progress Summary

| Category | Total | Done | Remaining |
|----------|-------|------|-----------|
| Setup | 3 | 3 | 0 |
| Foundation | 6 | 6 | 0 |
| Implementation | 7 | 7 | 0 |
| Testing | 5 | 5 | 0 |
| **Total** | **21** | **21** | **0** |

---

## Setup (3 tasks)

Initial configuration and environment preparation.

- [x] T001 [S0101] Verify CMake builds cleanly with existing code (`cmake --build build/`)
- [x] T002 [S0101] Review greyhawk_ship_data structure for autopilot integration point (`src/vessels.h:431-499`)
- [x] T003 [S0101] Check for existing autopilot-related definitions to avoid duplicates (`src/vessels.h`)

---

## Foundation (6 tasks)

Core structures and base implementations.

- [x] T004 [S0101] Define autopilot configuration constants in vessels.h (`src/vessels.h`)
  - MAX_WAYPOINTS_PER_ROUTE (20)
  - MAX_ROUTES_PER_SHIP (5)
  - AUTOPILOT_TICK_INTERVAL
  - AUTOPILOT_NAME_LENGTH (64)

- [x] T005 [S0101] Define enum autopilot_state with 5 states (`src/vessels.h`)
  - AUTOPILOT_OFF
  - AUTOPILOT_TRAVELING
  - AUTOPILOT_WAITING
  - AUTOPILOT_PAUSED
  - AUTOPILOT_COMPLETE

- [x] T006 [S0101] Define struct waypoint with coordinates and metadata (`src/vessels.h`)
  - x, y, z coordinates (float)
  - name[64] (fixed char array)
  - tolerance (float)
  - wait_time (int)
  - flags (int)

- [x] T007 [S0101] Define struct ship_route with waypoint array (`src/vessels.h`)
  - route_id (int)
  - name[64] (fixed char array)
  - waypoints[MAX_WAYPOINTS_PER_ROUTE]
  - num_waypoints (int)
  - loop (bool)
  - active (bool)

- [x] T008 [S0101] Define struct autopilot_data with state machine fields (`src/vessels.h`)
  - state (enum autopilot_state)
  - current_route pointer
  - current_waypoint_index (int)
  - tick_counter (int)
  - wait_remaining (int)
  - last_update (time_t)
  - pilot_mob_vnum (int)

- [x] T009 [S0101] Extend greyhawk_ship_data with autopilot pointer (`src/vessels.h:567`)
  - Add struct autopilot_data *autopilot field

---

## Implementation (7 tasks)

Main feature implementation.

- [x] T010 [S0101] Declare autopilot lifecycle function prototypes (`src/vessels.h`)
  - autopilot_init()
  - autopilot_cleanup()
  - autopilot_start()
  - autopilot_stop()
  - autopilot_pause()
  - autopilot_resume()

- [x] T011 [S0101] Declare waypoint management function prototypes (`src/vessels.h`)
  - waypoint_add()
  - waypoint_remove()
  - waypoint_clear_all()
  - waypoint_get_current()
  - waypoint_get_next()

- [x] T012 [S0101] Declare route management function prototypes (`src/vessels.h`)
  - route_create()
  - route_destroy()
  - route_load()
  - route_save()
  - route_activate()
  - route_deactivate()

- [x] T013 [S0101] Create vessels_autopilot.c skeleton with includes (`src/vessels_autopilot.c`)
  - File header comment
  - Include vessels.h and required headers
  - Forward declarations if needed

- [x] T014 [S0101] [P] Implement autopilot lifecycle stub functions (`src/vessels_autopilot.c`)
  - All functions return placeholder values
  - Include TODO comments for future implementation

- [x] T015 [S0101] [P] Implement waypoint management stub functions (`src/vessels_autopilot.c`)
  - All functions return placeholder values
  - Include TODO comments for future implementation

- [x] T016 [S0101] [P] Implement route management stub functions (`src/vessels_autopilot.c`)
  - All functions return placeholder values
  - Include TODO comments for future implementation

---

## Testing (5 tasks)

Verification and quality assurance.

- [x] T017 [S0101] Update CMakeLists.txt with vessels_autopilot.c (`CMakeLists.txt`)
  - Add src/vessels_autopilot.c to CIRCLE_SRCS list

- [x] T018 [S0101] Create autopilot_struct_test.c test file (`unittests/CuTest/test_autopilot_structs.c`)
  - Structure size tests
  - Field accessibility tests
  - Constant value verification tests
  - Enum value tests

- [x] T019 [S0101] Update CuTest Makefile for new test (`unittests/CuTest/Makefile`)
  - Add test_autopilot_structs.c to TEST_SOURCES
  - Add compilation rules

- [x] T020 [S0101] Run full build and fix any compilation issues (`cmake --build build/`)
  - Verify zero errors
  - Verify zero warnings with -Wall -Wextra
  - Fix any issues found

- [x] T021 [S0101] Run tests and validate with Valgrind (`unittests/CuTest/`)
  - Run test_runner
  - Run valgrind --leak-check=full
  - Verify ASCII encoding on all new files
  - Verify Unix LF line endings

---

## Completion Checklist

Before marking session complete:

- [x] All tasks marked `[x]`
- [x] All tests passing (14/14)
- [x] All files ASCII-encoded
- [x] implementation-notes.md updated
- [x] Ready for `/validate`

---

## Notes

### Parallelization
Tasks T014, T015, T016 were marked `[P]` and implemented together after T013.

### Key Metrics
- struct waypoint: 88 bytes
- struct ship_route: 1840 bytes
- struct autopilot_data: 48 bytes
- Per-ship overhead: 48 bytes (well under 1KB target)

### Files Created
- `src/vessels_autopilot.c` - Autopilot function implementations
- `unittests/CuTest/test_autopilot_structs.c` - Structure validation tests

### Files Modified
- `src/vessels.h` - Added autopilot structures, constants, prototypes
- `CMakeLists.txt` - Added vessels_autopilot.c to build
- `unittests/CuTest/Makefile` - Added autopilot test targets

---

## Next Steps

Run `/validate` to verify session completeness.
