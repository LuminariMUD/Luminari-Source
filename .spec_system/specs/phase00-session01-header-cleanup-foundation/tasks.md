# Task Checklist

**Session ID**: `phase00-session01-header-cleanup-foundation`
**Total Tasks**: 20
**Estimated Duration**: 6-8 hours
**Created**: 2025-12-29

---

## Legend

- `[x]` = Completed
- `[ ]` = Pending
- `[B]` = Blocked (external dependency)
- `[P]` = Parallelizable (can run with other [P] tasks)
- `[S0001]` = Session reference (Phase 00, Session 01)
- `TNNN` = Task ID

---

## Progress Summary

| Category | Total | Done | Blocked | Remaining |
|----------|-------|------|---------|-----------|
| Setup | 3 | 3 | 0 | 0 |
| Foundation | 4 | 4 | 0 | 0 |
| Implementation | 9 | 9 | 0 | 0 |
| Testing | 4 | 4 | 0 | 0 |
| **Total** | **20** | **20** | **0** | **0** |

---

## Setup (3 tasks)

Initial configuration and environment preparation.

- [x] T001 [S0001] Verify prerequisites met - build system working, config headers in place
- [x] T002 [S0001] Create backup of `src/vessels.h` before modifications
- [x] T003 [S0001] Read and document current vessels.h structure to confirm duplicate locations

---

## Foundation (4 tasks)

Analysis and preparation before making changes.

- [x] T004 [S0001] Verify duplicate constant locations match spec (lines 50-53 vs 89-91, lines 56-59 vs 94-97)
- [x] T005 [S0001] Verify duplicate struct locations match spec (lines 321-405 vs 498-626)
- [x] T006 [S0001] Document extended greyhawk_ship_data struct fields to preserve (Phase 2 multi-room)
- [x] T007 [S0001] Build project to establish baseline - verify current state compiles (`cmake --build build/`)

---

## Implementation (9 tasks)

Main duplicate removal - work incrementally with compile verification.

- [x] T008 [S0001] Remove duplicate VESSEL_STATE_* constants (lines 89-91) (`src/vessels.h`)
- [x] T009 [S0001] Remove duplicate VESSEL_SIZE_* constants (lines 94-97) (`src/vessels.h`)
- [x] T010 [S0001] Compile and verify no errors after constant removal (`cmake --build build/`)
- [x] T011 [S0001] [P] Remove duplicate greyhawk_ship_slot struct (lines 321-329) (`src/vessels.h`)
- [x] T012 [S0001] [P] Remove duplicate greyhawk_ship_crew struct (lines 331-337) (`src/vessels.h`)
- [x] T013 [S0001] Remove duplicate greyhawk_ship_data struct (lines 340-386) - verify Phase 2 fields retained (`src/vessels.h`)
- [x] T014 [S0001] [P] Remove duplicate greyhawk_contact_data struct (lines 388-395) (`src/vessels.h`)
- [x] T015 [S0001] [P] Remove duplicate greyhawk_ship_map struct (lines 403-405) (`src/vessels.h`)
- [x] T016 [S0001] Remove duplicate function prototypes (lines 451-477) and redundant forward decls (`src/vessels.h`)

---

## Testing (4 tasks)

Verification and quality assurance.

- [x] T017 [S0001] Full compile with cmake - zero errors required (`cmake --build build/`) - RESOLVED: Fixed missing GOLEM_* constants and CMakeLists.txt
- [x] T018 [S0001] Verify no new compiler warnings introduced (compare to baseline) - PASSED: 122 pre-existing warnings, none from vessels.h
- [x] T019 [S0001] Server startup smoke test (`./bin/circle -d lib`) - PASSED: Server boots, connects to MySQL, loads all zones
- [x] T020 [S0001] Validate ASCII encoding and Unix LF line endings on vessels.h

---

## Completion Checklist

Before marking session complete:

- [x] All tasks marked `[x]` (20/20 complete)
- [x] All duplicate definitions removed
- [x] Zero compile errors (build issues resolved)
- [x] No new warnings (122 pre-existing, none from vessels.h changes)
- [x] Server starts successfully (verified boot and MySQL connection)
- [x] All files ASCII-encoded with LF line endings
- [x] implementation-notes.md updated
- [x] Ready for `/validate`

---

## Notes

### Pre-existing Build Blocker - RESOLVED

The build initially failed due to missing GOLEM_* constants in `src/crafting_new.c` (lines 8918-8950).

**Resolution:**
- Added missing GOLEM_WOOD_SMALL through GOLEM_IRON_HUGE (12 constants) to `src/vnums.h`
- Added missing `src/moon_bonus_spells.c` to CMakeLists.txt
- Added missing `src/mob_known_spells.c` to CMakeLists.txt

All issues resolved and build now completes successfully.

### vessels.h Changes Summary

Successfully removed 123 lines:
- Original: 730 lines
- After cleanup: 607 lines

Removed:
- Duplicate VESSEL_STATE_* and VESSEL_SIZE_* constants
- Duplicate greyhawk_ship_slot, greyhawk_ship_crew, greyhawk_ship_data, greyhawk_contact_data, greyhawk_ship_map structs
- Duplicate function prototypes under #if VESSELS_ENABLE_GREYHAWK
- Redundant forward declarations

Preserved:
- greyhawk_ship_action_event struct (unique, not duplicated)
- All macros (GREYHAWK_SHIP* accessors)
- Extended greyhawk_ship_data with Phase 2 multi-room fields
- All unconditional function prototypes

### Parallelization

Tasks T011, T012, T014, T015 were completed together in a single edit.

---

## Next Steps

1. Fix pre-existing crafting_new.c build errors (separate issue)
2. Re-run T017-T019 after build fix
3. Run `/validate` to verify session completeness
