# Task Checklist

**Session ID**: `phase01-session03-path-following-logic`
**Total Tasks**: 20
**Estimated Duration**: 6-8 hours
**Created**: 2025-12-30
**Completed**: 2025-12-30

---

## Legend

- `[x]` = Completed
- `[ ]` = Pending
- `[P]` = Parallelizable (can run with other [P] tasks)
- `[S0103]` = Session reference (Phase 01, Session 03)
- `TNNN` = Task ID

---

## Progress Summary

| Category | Total | Done | Remaining |
|----------|-------|------|-----------|
| Setup | 2 | 2 | 0 |
| Foundation | 5 | 5 | 0 |
| Implementation | 8 | 8 | 0 |
| Testing | 5 | 5 | 0 |
| **Total** | **20** | **20** | **0** |

---

## Setup (2 tasks)

Initial configuration and environment preparation.

- [x] T001 [S0103] Verify prerequisites met - confirm Session 01 and 02 code exists and compiles
- [x] T002 [S0103] Review existing vessel_list iteration pattern in vessels.c for autopilot_tick design

---

## Foundation (5 tasks)

Core structures and base function signatures.

- [x] T003 [S0103] Add function prototypes to vessels.h for path-following functions (`src/vessels.h`)
- [x] T004 [S0103] [P] Implement calculate_distance_to_waypoint() - Euclidean distance calculation (`src/vessels_autopilot.c`)
- [x] T005 [S0103] [P] Implement calculate_heading_to_waypoint() - direction vector to waypoint (`src/vessels_autopilot.c`)
- [x] T006 [S0103] [P] Implement check_waypoint_arrival() - tolerance-based arrival detection (`src/vessels_autopilot.c`)
- [x] T007 [S0103] Implement advance_to_next_waypoint() - route progression with loop handling (`src/vessels_autopilot.c`)

---

## Implementation (8 tasks)

Main feature implementation.

- [x] T008 [S0103] Implement handle_waypoint_arrival() - wait time initiation and state transitions (`src/vessels_autopilot.c`)
- [x] T009 [S0103] Implement move_vessel_toward_waypoint() - terrain-validated movement execution (`src/vessels_autopilot.c`)
- [x] T010 [S0103] Implement process_waiting_vessel() - wait time countdown and state transition (`src/vessels_autopilot.c`)
- [x] T011 [S0103] Implement process_traveling_vessel() - main movement loop per vessel (`src/vessels_autopilot.c`)
- [x] T012 [S0103] Implement autopilot_tick() - main entry point iterating all vessels (`src/vessels_autopilot.c`)
- [x] T013 [S0103] Add autopilot_tick() call to heartbeat() in comm.c with AUTOPILOT_TICK_INTERVAL timing (`src/comm.c`)
- [x] T014 [S0103] Add error handling and logging for terrain validation failures (`src/vessels_autopilot.c`)
- [x] T015 [S0103] Add route completion logic - set COMPLETE state or restart for loop routes (`src/vessels_autopilot.c`)

---

## Testing (5 tasks)

Verification and quality assurance.

- [x] T016 [S0103] Create test_autopilot_pathfinding.c with test scaffolding (`unittests/CuTest/test_autopilot_pathfinding.c`)
- [x] T017 [S0103] [P] Write unit tests for distance and heading calculations (`unittests/CuTest/test_autopilot_pathfinding.c`)
- [x] T018 [S0103] [P] Write unit tests for arrival detection and waypoint advancement (`unittests/CuTest/test_autopilot_pathfinding.c`)
- [x] T019 [S0103] Update Makefile to build pathfinding tests and register in test suite (`unittests/CuTest/Makefile`)
- [x] T020 [S0103] Run full test suite with Valgrind and validate ASCII encoding

---

## Completion Checklist

Before marking session complete:

- [x] All tasks marked `[x]`
- [x] All tests passing (make pathfinding)
- [x] All files ASCII-encoded (0-127 characters only)
- [x] Unix LF line endings
- [x] ANSI C90 compliant (no // comments, declarations at block start)
- [x] NULL checks on all pointer dereferences
- [x] Valgrind-clean execution
- [x] Compiles clean with -Wall -Wextra
- [x] implementation-notes.md updated
- [x] Ready for `/validate`

---

## Notes

### Parallelization
Tasks marked `[P]` can be worked on simultaneously:
- T004, T005, T006: Independent calculation functions
- T017, T018: Independent test categories

### Task Dependencies
```
T001 -> T002 -> T003 -> T004/T005/T006 -> T007 -> T008/T009/T010
                                                      |
                                                      v
                                                    T011 -> T012 -> T013
                                                      |
                                                      v
                                                T014 -> T015 -> T016 -> T017/T018 -> T019 -> T020
```

### Key Implementation Details
- **AUTOPILOT_TICK_INTERVAL**: 5 pulses (~0.5 seconds at 10 pulses/sec)
- **Movement pattern**: Use find_available_wilderness_room() + assign_wilderness_room()
- **Tolerance check**: Euclidean distance <= waypoint.tolerance
- **Wait time**: Decrement per second (not per tick) using time(0) delta
- **Vessel iteration**: Use greyhawk_ships[] array from vessels.c

### Critical Considerations
- Always NULL check ship->autopilot before access
- Always NULL check autopilot->current_route before waypoint access
- Log warnings on terrain validation failures (don't crash)
- Use /* */ comments only (C90)
- Declare variables at block start

### Math Functions
- sqrt() from math.h for distance calculation
- Link with -lm (already in Makefile)

---

## Session Complete

All 20 tasks completed successfully on 2025-12-30.

Run `/validate` to verify session completeness.
