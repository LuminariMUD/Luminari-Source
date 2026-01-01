# Task Checklist

**Session ID**: `phase02-session03-vehicle-movement-system`
**Total Tasks**: 20
**Estimated Duration**: 6-8 hours
**Created**: 2025-12-30
**Completed**: 2025-12-30

---

## Legend

- `[x]` = Completed
- `[ ]` = Pending
- `[P]` = Parallelizable (can run with other [P] tasks)
- `[S0203]` = Session reference (Phase 02, Session 03)
- `TNNN` = Task ID

---

## Progress Summary

| Category | Total | Done | Remaining |
|----------|-------|------|-----------|
| Setup | 2 | 2 | 0 |
| Foundation | 6 | 6 | 0 |
| Implementation | 8 | 8 | 0 |
| Testing | 4 | 4 | 0 |
| **Total** | **20** | **20** | **0** |

---

## Setup (2 tasks)

Initial configuration and environment preparation.

- [x] T001 [S0203] Verify prerequisites met - check vehicles.c, vessels.h, VTERRAIN_* flags exist
- [x] T002 [S0203] Read existing vehicle movement patterns in vessels_autopilot.c for reference

---

## Foundation (6 tasks)

Core structures and base implementations.

- [x] T003 [S0203] [P] Define direction delta constants for 8-direction movement (`src/vessels.h`)
- [x] T004 [S0203] [P] Add vehicle movement function declarations to header (`src/vessels.h`)
- [x] T005 [S0203] Implement `vehicle_get_direction_delta()` helper function (`src/vehicles.c`)
- [x] T006 [S0203] Implement SECT_* to VTERRAIN_* mapping function (`src/vehicles.c`)
- [x] T007 [S0203] Implement `vehicle_can_traverse_terrain()` terrain validation (`src/vehicles.c`)
- [x] T008 [S0203] Implement `get_vehicle_speed_modifier()` terrain speed calculation (`src/vehicles.c`)

---

## Implementation (8 tasks)

Main feature implementation.

- [x] T009 [S0203] Implement `move_vehicle()` core movement function - coordinate calculation (`src/vehicles.c`)
- [x] T010 [S0203] Add wilderness boundary validation to `move_vehicle()` (`src/vehicles.c`)
- [x] T011 [S0203] Add terrain validation check to `move_vehicle()` (`src/vehicles.c`)
- [x] T012 [S0203] Add wilderness room allocation pattern to `move_vehicle()` (`src/vehicles.c`)
- [x] T013 [S0203] Implement state transitions in `move_vehicle()` - IDLE/LOADED to MOVING (`src/vehicles.c`)
- [x] T014 [S0203] Add speed modifier application to movement (`src/vehicles.c`)
- [x] T015 [S0203] Implement database position persistence after movement (`src/vehicles.c`)
- [x] T016 [S0203] Add NULL checks and error handling to all movement functions (`src/vehicles.c`)

---

## Testing (4 tasks)

Verification and quality assurance.

- [x] T017 [S0203] Create test file with unit tests for direction, terrain, and speed functions (`unittests/CuTest/test_vehicle_movement.c`)
- [x] T018 [S0203] Add `move_vehicle()` tests - valid movement, boundaries, invalid terrain (`unittests/CuTest/test_vehicle_movement.c`)
- [x] T019 [S0203] Update Makefile with vehicle movement test targets (`unittests/CuTest/Makefile`)
- [x] T020 [S0203] Run test suite, validate ASCII encoding, verify no compiler warnings

---

## Completion Checklist

Before marking session complete:

- [x] All tasks marked `[x]`
- [x] All tests passing (45 tests, 0 failures)
- [x] All files ASCII-encoded
- [x] implementation-notes.md updated
- [x] Ready for `/validate`

---

## Notes

### Parallelization
Tasks marked `[P]` can be worked on simultaneously.
- T003 and T004 can be done in parallel (both header modifications)

### Task Timing
Target ~20-25 minutes per task.

### Dependencies
- T005-T008 depend on T003-T004 (header declarations)
- T009-T016 depend on T005-T008 (foundation functions)
- T017-T018 depend on T009-T016 (need implementation to test)
- T019 depends on T017-T018 (need test files to add to Makefile)

### Key Implementation Patterns

1. **Wilderness Room Allocation** (from Phase 00):
   ```c
   room = find_room_by_coordinates(x, y);
   if (room == NOWHERE)
   {
     room = find_available_wilderness_room();
     if (room == NOWHERE)
       return 0; /* Pool exhausted */
     assign_wilderness_room(room, x, y);
   }
   ```

2. **Terrain Mapping** - Map SECT_* to VTERRAIN_*:
   - SECT_ROAD_NS/EW/INT -> VTERRAIN_ROAD
   - SECT_FIELD -> VTERRAIN_PLAINS
   - SECT_FOREST -> VTERRAIN_FOREST
   - SECT_HILLS -> VTERRAIN_HILLS
   - SECT_MOUNTAIN -> VTERRAIN_MOUNTAIN
   - SECT_WATER_* -> Blocked for land vehicles

3. **Speed Modifiers**:
   - Road: 1.5x (150%)
   - Plains/Field: 1.0x (100%)
   - Forest/Hills: 0.75x (75%)
   - Mountain/Swamp: 0.5x (50%)

### Direction Constants
Standard 8-direction mapping (uses existing MUD direction constants):
- NORTH=0, EAST=1, SOUTH=2, WEST=3
- NORTHWEST=6, NORTHEAST=7, SOUTHEAST=8, SOUTHWEST=9

---

## Implementation Summary

**Files Modified**:
- `src/vessels.h` - Added terrain speed constants, wilderness bounds, x/y coords to struct, function declarations
- `src/vehicles.c` - Added wilderness.h include, x/y init, 450+ lines of movement functions, updated DB schema

**Functions Implemented**:
- `vehicle_get_direction_delta()` - 8-direction to X/Y delta conversion
- `sector_to_vterrain()` - SECT_* to VTERRAIN_* mapping (37 sector types)
- `vehicle_can_traverse_terrain()` - Terrain capability validation
- `get_vehicle_speed_modifier()` - Terrain-based speed calculation
- `vehicle_get_speed()` - Effective speed with damage/load modifiers
- `vehicle_can_move()` - Complete movement validation
- `move_vehicle()` - Core movement with room allocation and persistence
- `vehicle_move()` - API wrapper

**Test Results**:
- 45 unit tests created and passing
- Tests cover: direction deltas, terrain mapping, traversal, speed modifiers, movement validation

---
