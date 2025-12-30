# Task Checklist

**Session ID**: `phase02-session01-vehicle-data-structures`
**Total Tasks**: 18
**Estimated Duration**: 6-8 hours
**Created**: 2025-12-30

---

## Legend

- `[x]` = Completed
- `[ ]` = Pending
- `[P]` = Parallelizable (can run with other [P] tasks)
- `[S0201]` = Session reference (Phase 02, Session 01)
- `TNNN` = Task ID

---

## Progress Summary

| Category | Total | Done | Remaining |
|----------|-------|------|-----------|
| Setup | 2 | 2 | 0 |
| Foundation | 6 | 6 | 0 |
| Implementation | 6 | 6 | 0 |
| Testing | 4 | 4 | 0 |
| **Total** | **18** | **18** | **0** |

---

## Setup (2 tasks)

Initial configuration and environment preparation.

- [x] T001 [S0201] Verify prerequisites: existing vessels.h compiles cleanly (`src/vessels.h`)
- [x] T002 [S0201] Create test file scaffold for vehicle structs (`unittests/CuTest/test_vehicle_structs.c`)

---

## Foundation (6 tasks)

Core structures and base implementations.

- [x] T003 [S0201] Add VEHICLE SYSTEM section delimiter comments to vessels.h (`src/vessels.h`)
- [x] T004 [S0201] [P] Define vehicle_type enum with 4+ vehicle types (`src/vessels.h`)
- [x] T005 [S0201] [P] Define vehicle_state enum with lifecycle states (`src/vessels.h`)
- [x] T006 [S0201] [P] Define VTERRAIN_* terrain capability flags as bitfield macros (`src/vessels.h`)
- [x] T007 [S0201] [P] Define vehicle capacity constants (passengers, weight limits) (`src/vessels.h`)
- [x] T008 [S0201] [P] Define vehicle speed constants per vehicle type (`src/vessels.h`)

---

## Implementation (6 tasks)

Main feature implementation.

- [x] T009 [S0201] Implement core vehicle_data struct with identity fields (`src/vessels.h`)
- [x] T010 [S0201] Add location fields to vehicle_data struct (`src/vessels.h`)
- [x] T011 [S0201] Add capacity fields to vehicle_data struct (`src/vessels.h`)
- [x] T012 [S0201] Add movement fields to vehicle_data struct (`src/vessels.h`)
- [x] T013 [S0201] Add condition and ownership fields to vehicle_data struct (`src/vessels.h`)
- [x] T014 [S0201] Add forward declarations for vehicle functions (`src/vessels.h`)

---

## Testing (4 tasks)

Verification and quality assurance.

- [x] T015 [S0201] [P] Write unit tests for vehicle_data struct size validation (<512 bytes) (`unittests/CuTest/test_vehicle_structs.c`)
- [x] T016 [S0201] [P] Write unit tests for enum value uniqueness and integrity (`unittests/CuTest/test_vehicle_structs.c`)
- [x] T017 [S0201] [P] Write unit tests for terrain flag bit uniqueness (`unittests/CuTest/test_vehicle_structs.c`)
- [x] T018 [S0201] Compile verification with -Wall -Wextra, run tests, validate ASCII (`unittests/CuTest/`)

---

## Completion Checklist

Before marking session complete:

- [x] All tasks marked `[x]`
- [x] All tests passing
- [x] All files ASCII-encoded (0-127 characters only)
- [x] implementation-notes.md updated
- [x] Ready for `/validate`

---

## Notes

### Parallelization
Tasks marked `[P]` can be worked on simultaneously:
- T004-T008 (Foundation enums/constants) are independent
- T015-T017 (Test writing) are independent

### Task Timing
Target ~20-25 minutes per task.

### Dependencies
- T003 must complete before T004-T008 (section header needed first)
- T004-T008 must complete before T009-T014 (enums/constants needed by struct)
- T009-T014 are sequential (building up the struct incrementally)
- T015-T017 depend on T009-T014 (struct must exist before testing)
- T018 is final verification after all implementation complete

### Key Constraints
1. **Enum offset**: Start vehicle enums at values that don't conflict with VESSEL_* constants
2. **Struct size**: vehicle_data must remain under 512 bytes (target: 256-384 bytes)
3. **C90 compliance**: No // comments, no VLAs, no mixed declarations
4. **Namespace**: All vehicle items prefixed with VEHICLE_ or VSTATE_ or VTERRAIN_

### Expected Struct Size Breakdown
- Identity (id, type, state, name): ~76 bytes
- Location (room, direction): ~8 bytes
- Capacity (4 ints): ~16 bytes
- Movement (3 ints): ~12 bytes
- Condition (2 ints): ~8 bytes
- Ownership (long): ~8 bytes
- Object pointer: ~8 bytes
- **Estimated total**: ~136 bytes (well under 512 byte limit)

---

## Next Steps

Run `/implement` to begin AI-led implementation.
