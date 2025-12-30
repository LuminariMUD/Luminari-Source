# Task Checklist

**Session ID**: `phase02-session05-vehicle-in-vehicle-mechanics`
**Total Tasks**: 20
**Estimated Duration**: 6-8 hours
**Created**: 2025-12-30

---

## Legend

- `[x]` = Completed
- `[ ]` = Pending
- `[P]` = Parallelizable (can run with other [P] tasks)
- `[S0205]` = Session reference (Phase 02, Session 05)
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

- [x] T001 [S0205] Verify prerequisites met - confirm sessions 01-04 complete, review vessel docking patterns (`src/vessels_docking.c`)
- [x] T002 [S0205] Update CMakeLists.txt to include vehicles_transport.c in build (`src/CMakeLists.txt`)

---

## Foundation (5 tasks)

Core structures and base implementations.

- [x] T003 [S0205] Add parent_vessel_id field to struct vehicle_data (`src/vessels.h:1117`)
- [x] T004 [S0205] Add VSTATE_LOADED to vehicle_state enum or document parent_vessel_id usage (`src/vessels.h`)
- [x] T005 [S0205] [P] Add function prototypes for transport functions (`src/vessels.h`)
- [x] T006 [S0205] [P] Create vehicles_transport.c with file header and includes (`src/vehicles_transport.c`)
- [x] T007 [S0205] Update vehicle_save() to persist parent_vessel_id to MySQL (`src/vehicles.c`)

---

## Implementation (8 tasks)

Main feature implementation.

- [x] T008 [S0205] Implement check_vessel_vehicle_capacity() - weight/slot validation (`src/vehicles_transport.c`)
- [x] T009 [S0205] Implement load_vehicle_onto_vessel() - core loading logic (`src/vehicles_transport.c`)
- [x] T010 [S0205] Implement unload_vehicle_from_vessel() - core unloading logic (`src/vehicles_transport.c`)
- [x] T011 [S0205] Implement get_loaded_vehicles_list() - list vehicles on a vessel (`src/vehicles_transport.c`)
- [x] T012 [S0205] Implement vehicle_sync_with_vessel() - coordinate synchronization hook (`src/vehicles_transport.c`)
- [x] T013 [S0205] [P] Implement do_loadvehicle command handler (`src/vehicles_commands.c`)
- [x] T014 [S0205] [P] Implement do_unloadvehicle command handler (`src/vehicles_commands.c`)
- [x] T015 [S0205] Register loadvehicle and unloadvehicle in command table (`src/interpreter.c`)

---

## Testing (5 tasks)

Verification and quality assurance.

- [x] T016 [S0205] Create vehicle_transport_tests.c with test framework setup (`unittests/CuTest/vehicle_transport_tests.c`)
- [x] T017 [S0205] [P] Write unit tests for load/unload functions (6+ tests) (`unittests/CuTest/vehicle_transport_tests.c`)
- [x] T018 [S0205] [P] Write unit tests for capacity and coordinate sync (6+ tests) (`unittests/CuTest/vehicle_transport_tests.c`)
- [x] T019 [S0205] Update Makefile and run test suite - verify all passing (`unittests/CuTest/Makefile`)
- [x] T020 [S0205] Validate ASCII encoding and run Valgrind memory check

---

## Completion Checklist

Before marking session complete:

- [ ] All tasks marked `[x]`
- [ ] All tests passing (12+ tests)
- [ ] All files ASCII-encoded (0-127 only)
- [ ] Unix LF line endings verified
- [ ] No compiler warnings with -Wall -Wextra
- [ ] Valgrind clean (no memory leaks)
- [ ] implementation-notes.md updated
- [ ] Ready for `/validate`

---

## Notes

### Parallelization Opportunities
- T005 + T006: Prototypes and new file creation are independent
- T013 + T014: Command handlers are independent after core functions exist
- T017 + T018: Test categories can be written simultaneously

### Task Dependencies
```
T001 -> T002 -> T003 -> T004
                  |
                  v
            T005 + T006 (parallel)
                  |
                  v
                T007 -> T008 -> T009 -> T010 -> T011 -> T012
                                                         |
                                                         v
                                                  T013 + T014 (parallel)
                                                         |
                                                         v
                                                       T015 -> T016 -> T017 + T018 (parallel)
                                                                              |
                                                                              v
                                                                        T019 -> T020
```

### Key Implementation Patterns
- Follow `vessel_dock()` pattern from vessels_docking.c for state transitions
- Use `vehicle_can_add_weight()` pattern for capacity checks
- MySQL persistence uses REPLACE INTO pattern from vehicles.c

### Critical Validations
- Vessel must be docked or speed=0 to load/unload
- Player must dismount vehicle before loading
- Vehicle terrain_flags must be compatible with unload location
- Only one nesting level (vehicle -> vessel, not deeper)

---

## Next Steps

Run `/implement` to begin AI-led implementation.
