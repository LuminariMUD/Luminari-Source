# Task Checklist

**Session ID**: `phase02-session06-unified-command-interface`
**Total Tasks**: 20
**Estimated Duration**: 6-8 hours
**Created**: 2025-12-30

---

## Legend

- `[x]` = Completed
- `[ ]` = Pending
- `[P]` = Parallelizable (can run with other [P] tasks)
- `[S0206]` = Session reference (Phase 02, Session 06)
- `TNNN` = Task ID

---

## Progress Summary

| Category | Total | Done | Remaining |
|----------|-------|------|-----------|
| Setup | 3 | 3 | 0 |
| Foundation | 5 | 5 | 0 |
| Implementation | 8 | 8 | 0 |
| Testing | 4 | 4 | 0 |
| **Total** | **20** | **20** | **0** |

---

## Setup (3 tasks)

Initial configuration and environment preparation.

- [x] T001 [S0206] Verify prerequisites met - confirm vehicles.c, vehicles_commands.c, vessels_src.c exist and build cleanly
- [x] T002 [S0206] [P] Add transport_unified.c to build system (Note: No Makefile.am - CMake only)
- [x] T003 [S0206] [P] Add transport_unified.c to CMake build (`CMakeLists.txt`)

---

## Foundation (5 tasks)

Core structures and base implementations.

- [x] T004 [S0206] Create transport_unified.h header with include guards and standard includes (`src/transport_unified.h`)
- [x] T005 [S0206] Define transport_type enum (TRANSPORT_NONE, TRANSPORT_VEHICLE, TRANSPORT_VESSEL) (`src/transport_unified.h`)
- [x] T006 [S0206] Define transport_data abstraction structure with union for vehicle/vessel pointers (`src/transport_unified.h`)
- [x] T007 [S0206] Add function prototypes for transport abstraction layer (`src/transport_unified.h`)
- [x] T008 [S0206] Add ACMD declarations for new commands in vessels.h (`src/vessels.h`)

---

## Implementation (8 tasks)

Main feature implementation.

- [x] T009 [S0206] Create transport_unified.c with standard boilerplate and includes (`src/transport_unified.c`)
- [x] T010 [S0206] Implement get_transport_type() - detect transport type in room (`src/transport_unified.c`)
- [x] T011 [S0206] Implement get_character_transport() - get character's current transport (`src/transport_unified.c`)
- [x] T012 [S0206] Implement do_transport_enter ACMD - unified entry for vehicles/vessels (`src/transport_unified.c`)
- [x] T013 [S0206] Implement do_exit_transport ACMD - unified exit command (`src/transport_unified.c`)
- [x] T014 [S0206] Implement do_transport_go ACMD - unified movement command (`src/transport_unified.c`)
- [x] T015 [S0206] Implement do_transportstatus ACMD - unified status display (`src/transport_unified.c`)
- [x] T016 [S0206] Register new commands in interpreter.c cmd_info array (`src/interpreter.c`)

---

## Testing (4 tasks)

Verification and quality assurance.

- [x] T017 [S0206] Create test_transport_unified.c with CuTest framework setup (`unittests/CuTest/test_transport_unified.c`)
- [x] T018 [S0206] [P] Implement unit tests for transport type detection and abstraction (15 tests passing)
- [x] T019 [S0206] Add test_transport_unified.c to test runner (standalone executable)
- [x] T020 [S0206] Run test suite, verify ASCII encoding - all files ASCII text confirmed

---

## Completion Checklist

Before marking session complete:

- [x] All tasks marked `[x]`
- [x] All tests passing (15/15)
- [x] All files ASCII-encoded (0-127 characters only)
- [x] Unix LF line endings on all files
- [x] implementation-notes.md updated
- [x] Ready for `/validate`

---

## Notes

### Parallelization
- T002/T003: Build system updates can be done in parallel
- T018: Unit test writing can parallel with other testing tasks

### Task Timing
Target ~20-25 minutes per task.

### Dependencies
- T004-T008 (Foundation) must complete before T009-T016 (Implementation)
- T009 must complete before T010-T015
- T010-T011 must complete before T012-T015 (commands need detection functions)
- T016 depends on T012-T015 (commands must exist before registration)
- T017 must complete before T018

### Key Integration Points
- `do_enter`: Must detect vehicle vs vessel vs bulletin board context
- `do_exit_transport`: Must NOT interfere with creature mount dismount
- Transport detection: Check vehicle first (more common), then vessel
- All commands: Must validate NULL pointers at boundaries

### ASCII Reminder
All output files must use ASCII-only characters (0-127). No Unicode, no extended ASCII.

### Existing Command Compatibility
- `mount`/`dismount` in act.other.c - creature mounts, do NOT modify
- `board`/`disembark` in vessels_src.c - vessel boarding, preserve
- `drive`/`vstatus` in vehicles_commands.c - vehicle commands, preserve
- `board` in mysql_boards.c - bulletin boards, avoid collision

---

## Next Steps

Run `/implement` to begin AI-led implementation.
