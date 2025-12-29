# Task Checklist

**Session ID**: `phase00-session03-vessel-type-system`
**Total Tasks**: 20
**Estimated Duration**: 6-8 hours
**Created**: 2025-12-29

---

## Legend

- `[x]` = Completed
- `[ ]` = Pending
- `[P]` = Parallelizable (can run with other [P] tasks)
- `[S0003]` = Session reference (Phase 00, Session 03)
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

Initial verification and preparation.

- [x] T001 [S0003] Verify prerequisites met: sessions 01/02 completed, build passes
- [x] T002 [S0003] Review existing vessel_class enum and VESSEL_TYPE_* defines for mapping strategy

---

## Foundation (6 tasks)

Core data structures and accessor functions.

- [x] T003 [S0003] Define static terrain capability data for VESSEL_RAFT (`src/vessels.c`)
- [x] T004 [S0003] [P] Define static terrain capability data for VESSEL_BOAT (`src/vessels.c`)
- [x] T005 [S0003] [P] Define static terrain capability data for VESSEL_SHIP and VESSEL_WARSHIP (`src/vessels.c`)
- [x] T006 [S0003] [P] Define static terrain capability data for VESSEL_AIRSHIP (`src/vessels.c`)
- [x] T007 [S0003] [P] Define static terrain capability data for VESSEL_SUBMARINE (`src/vessels.c`)
- [x] T008 [S0003] [P] Define static terrain capability data for VESSEL_TRANSPORT and VESSEL_MAGICAL (`src/vessels.c`)

---

## Implementation (8 tasks)

Core function implementations and refactoring.

- [x] T009 [S0003] Implement get_vessel_terrain_caps() accessor function (`src/vessels.c`)
- [x] T010 [S0003] Implement get_vessel_type_from_ship() helper with bounds checking (`src/vessels.c`)
- [x] T011 [S0003] Add function prototypes to header file (`src/vessels.h`)
- [x] T012 [S0003] Expand can_vessel_traverse_terrain() switch for all 8 vessel types (`src/vessels.c`)
- [x] T013 [S0003] Expand get_terrain_speed_modifier() switch for all 8 vessel types (`src/vessels.c`)
- [x] T014 [S0003] Replace hardcoded VESSEL_TYPE_SAILING_SHIP in move_ship_wilderness() line 530 (`src/vessels.c`)
- [x] T015 [S0003] Replace hardcoded VESSEL_TYPE_SAILING_SHIP in do_greyhawk_speed() line 755 (`src/vessels.c`)
- [x] T016 [S0003] Add vessel-type-specific terrain denial messages (`src/vessels.c`)

---

## Testing (4 tasks)

Verification, validation, and quality assurance.

- [x] T017 [S0003] Verify clean compile with -Wall -Wextra (no warnings)
- [x] T018 [S0003] Code review: verify all switch statements have cases for all 8 vessel types
- [x] T019 [S0003] Validate ASCII encoding and Unix LF line endings on modified files
- [x] T020 [S0003] Create implementation-notes.md documenting decisions and edge cases

---

## Completion Checklist

Before marking session complete:

- [x] All tasks marked `[x]`
- [x] All files ASCII-encoded
- [x] Unix LF line endings verified
- [x] No compiler warnings with -Wall -Wextra (for new code)
- [x] implementation-notes.md updated
- [x] Ready for `/validate`

---

## Notes

### Parallelization
Tasks T004-T008 are parallelizable as they define independent data structures for different vessel types. Complete T003 first as a template.

### Task Timing
Target approximately 20-25 minutes per task.

### Dependencies
- T003 must complete before T004-T008 (establishes pattern)
- T009-T010 depend on T003-T008 (data tables must exist)
- T011 depends on T009-T010 (prototypes for new functions)
- T012-T016 depend on T009-T010 (use new accessor functions)
- T017-T020 depend on all implementation tasks

### Key Technical Notes
1. **Enum vs Define mapping**: vessel_class enum (VESSEL_RAFT=0) differs from VESSEL_TYPE_* defines (VESSEL_TYPE_SAILING_SHIP=1). Use vessel_class enum consistently.
2. **Default behavior**: Invalid/uninitialized vessel_type should default to VESSEL_SHIP behavior.
3. **Speed modifier 0**: Indicates impassable terrain, not stopped vessel.
4. **Airship altitude**: Must consider z-coordinate for mountain avoidance.
5. **Submarine depth**: Requires negative z for underwater, positive z blocked.

### Vessel Type Terrain Capabilities Summary
| Vessel Type | Ocean | Shallow | Underwater | Air |
|-------------|-------|---------|------------|-----|
| VESSEL_RAFT | No | Yes | No | No |
| VESSEL_BOAT | No | Yes | No | No |
| VESSEL_SHIP | Yes | Yes | No | No |
| VESSEL_WARSHIP | Yes | Yes | No | No |
| VESSEL_AIRSHIP | Yes* | Yes* | No | Yes |
| VESSEL_SUBMARINE | Yes | Yes | Yes | No |
| VESSEL_TRANSPORT | Yes | Yes | No | No |
| VESSEL_MAGICAL | Yes | Yes | Yes | Yes |

*Airships ignore water requirements when flying (z > 0)

---

## Next Steps

Run `/implement` to begin AI-led implementation.
