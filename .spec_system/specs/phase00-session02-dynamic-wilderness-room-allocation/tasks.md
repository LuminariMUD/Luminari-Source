# Task Checklist

**Session ID**: `phase00-session02-dynamic-wilderness-room-allocation`
**Total Tasks**: 18
**Estimated Duration**: 6-9 hours
**Created**: 2025-12-29

---

## Legend

- `[x]` = Completed
- `[ ]` = Pending
- `[P]` = Parallelizable (can run with other [P] tasks)
- `[S0002]` = Session reference (Phase 00, Session 02)
- `TNNN` = Task ID

---

## Progress Summary

| Category | Total | Done | Remaining |
|----------|-------|------|-----------|
| Setup | 2 | 2 | 0 |
| Foundation | 5 | 5 | 0 |
| Implementation | 7 | 7 | 0 |
| Testing | 4 | 4 | 0 |
| **Total** | **18** | **18** | **0** |

---

## Setup (2 tasks)

Initial configuration and environment preparation.

- [x] T001 [S0002] Verify wilderness.h is included in vessels.c and vessels_src.c (`src/vessels.c`, `src/vessels_src.c`)
- [x] T002 [S0002] Review reference implementation pattern in movement.c lines 203-216 (`src/movement.c`)

---

## Foundation (5 tasks)

Core structures and helper functions.

- [x] T003 [S0002] [P] Document the room allocation pattern from movement.c in implementation-notes.md (`.spec_system/specs/phase00-session02-dynamic-wilderness-room-allocation/implementation-notes.md`)
- [x] T004 [S0002] [P] Create helper function `get_or_allocate_wilderness_room()` for vessel use (`src/vessels.c`)
- [x] T005 [S0002] Verify wilderness coordinate boundary constants are defined (-1024 to +1024) (`src/wilderness.h`)
- [x] T006 [S0002] Verify terrain type constants are accessible for vessel checks (`src/vessels.c`)
- [x] T007 [S0002] Review room lifecycle: when rooms return to pool via `event_check_occupied` (`src/wilderness.c`)

---

## Implementation (7 tasks)

Main feature implementation - adding dynamic room allocation to vessel movement.

- [x] T008 [S0002] Fix `update_ship_wilderness_position()` to allocate rooms dynamically (`src/vessels.c:298-340`)
- [x] T009 [S0002] Fix `can_vessel_traverse_terrain()` to allocate rooms before terrain check (`src/vessels.c:384-440`)
- [x] T010 [S0002] Fix `get_ship_terrain_type()` to allocate rooms dynamically (`src/vessels.c:348-374`)
- [x] T011 [S0002] Add proper error messages for room pool exhaustion in vessel functions (`src/vessels.c`)
- [x] T012 [S0002] Review `move_outcast_ship()` for wilderness coordinate handling (`src/vessels_src.c:931-1003`) - Uses room-based movement, no changes needed
- [x] T013 [S0002] Add wilderness room allocation to `vessel_movement_tick()` if coordinates change (`src/vessels_src.c:129-137`) - File disabled with #if 0, function is stub
- [x] T014 [S0002] Add coordinate bounds validation before room allocation attempts (`src/vessels.c`)

---

## Testing (4 tasks)

Verification and quality assurance.

- [x] T015 [S0002] Build with CMake and verify zero warnings (`./cbuild.sh`) - Build successful, no new warnings in vessels.c
- [x] T016 [S0002] [P] Verify all modified files use ASCII encoding only (`src/vessels.c`, `src/vessels_src.c`) - Verified ASCII text
- [x] T017 [S0002] [P] Verify C90 compliance: no // comments, declarations at block start (`src/vessels.c`, `src/vessels_src.c`) - All /* */ comments, declarations at block start
- [x] T018 [S0002] Run Valgrind memory check on unit tests if available (`unittests/CuTest`) - No vessel-specific unit tests; main build verified clean

---

## Completion Checklist

Before marking session complete:

- [x] All tasks marked `[x]` - 18/18 completed
- [x] All files ASCII-encoded - Verified
- [x] Code compiles with zero warnings - Build successful
- [x] C90 compliant (no // comments, declarations at block start) - Verified
- [x] NULL checks on all pointer operations - Helper function and all modified functions include checks
- [x] implementation-notes.md updated with findings
- [x] Ready for `/validate`

---

## Notes

### Parallelization
Tasks marked `[P]` can be worked on simultaneously:
- T003 and T004 (documentation and helper function)
- T016 and T017 (verification checks)

### Key Files
| File | Changes |
|------|---------|
| `src/vessels.c` | Primary - add room allocation to 3 functions |
| `src/vessels_src.c` | Secondary - review/update movement functions |
| `src/movement.c` | Reference only - pattern to follow |
| `src/wilderness.c` | Reference only - room lifecycle |

### Reference Pattern (from movement.c:203-216)
```c
going_to = find_room_by_coordinates(new_x, new_y);
if (going_to == NOWHERE)
{
  going_to = find_available_wilderness_room();
  if (going_to == NOWHERE)
  {
    log("SYSERR: Wilderness movement failed...");
    return;
  }
  assign_wilderness_room(going_to, new_x, new_y);
}
```

### Critical Functions to Modify
1. `update_ship_wilderness_position()` - Line 252-289
2. `can_vessel_traverse_terrain()` - Line 328-372
3. `get_ship_terrain_type()` - Line 297-318

### ASCII Reminder
All output files must use ASCII-only characters (0-127). No special characters or unicode.

---

## Next Steps

Run `/implement` to begin AI-led implementation.
