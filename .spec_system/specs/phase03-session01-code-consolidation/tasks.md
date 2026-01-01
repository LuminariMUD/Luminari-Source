# Task Checklist

**Session ID**: `phase03-session01-code-consolidation`
**Total Tasks**: 18
**Estimated Duration**: 6-8 hours
**Created**: 2025-12-30

---

## Legend

- `[x]` = Completed
- `[ ]` = Pending
- `[P]` = Parallelizable (can run with other [P] tasks)
- `[S0301]` = Session reference (Phase 03, Session 01)
- `TNNN` = Task ID

---

## Progress Summary

| Category | Total | Done | Remaining |
|----------|-------|------|-----------|
| Setup | 3 | 3 | 0 |
| Foundation | 4 | 4 | 0 |
| Implementation | 7 | 7 | 0 |
| Testing | 4 | 4 | 0 |
| **Total** | **18** | **18** | **0** |

---

## Setup (3 tasks)

Initial verification and baseline establishment.

- [x] T001 [S0301] Verify git working state is clean and tests pass (`unittests/CuTest/`)
- [x] T002 [S0301] Document current duplicate constants inventory in vessels.h (`src/vessels.h`)
- [x] T003 [S0301] Grep all usages of duplicate constants across codebase (`src/*.c`, `src/*.h`)

---

## Foundation (4 tasks)

Analyze and prepare consolidation strategy.

- [x] T004 [S0301] Analyze GREYHAWK_ITEM_SHIP conflict - verify value 56 is canonical (`src/vessels.h`)
- [x] T005 [S0301] Analyze DOCKABLE alias vs literal 41 usage patterns (`src/vessels.h`)
- [x] T006 [S0301] Identify conditional block dependencies and remaining content (`src/vessels.h`)
- [x] T007 [S0301] Create backup snapshot of vessels.h before modifications (`src/vessels.h`)

---

## Implementation (7 tasks)

Consolidate duplicate definitions - one category at a time.

- [x] T008 [S0301] Remove duplicate GREYHAWK_MAXSHIPS/MAXSLOTS from conditional block (`src/vessels.h`)
- [x] T009 [S0301] Remove duplicate position constants (FORE/PORT/REAR/STARBOARD) from conditional (`src/vessels.h`)
- [x] T010 [S0301] Remove duplicate range constants (SHRTRANGE/MEDRANGE/LNGRANGE) from conditional (`src/vessels.h`)
- [x] T011 [S0301] Resolve GREYHAWK_ITEM_SHIP conflict - remove line 358 (value 57) (`src/vessels.h`)
- [x] T012 [S0301] Consolidate DOCKABLE definition - remove duplicate at line 272 (`src/vessels.h`)
- [x] T013 [S0301] Evaluate and resolve ITEM_SHIP definition in OUTCAST section (`src/vessels.h`)
- [x] T014 [S0301] [P] Update any references in vessels.c/rooms.c/docking.c if needed (`src/vessels*.c`)

---

## Testing (4 tasks)

Verification and quality assurance.

- [x] T015 [S0301] Build project with cbuild.sh and verify no warnings (`./cbuild.sh`)
- [x] T016 [S0301] Run all 326 unit tests and confirm passing (`unittests/CuTest/`)
- [x] T017 [S0301] [P] Test compilation with VESSELS_ENABLE_GREYHAWK enabled/disabled (`src/vessels.h`)
- [x] T018 [S0301] [P] Run Valgrind on test suite to verify no memory issues (`unittests/CuTest/`)

---

## Completion Checklist

Before marking session complete:

- [x] All tasks marked `[x]`
- [x] All 326 tests passing (test count grew from 159 baseline)
- [x] All files ASCII-encoded
- [x] No duplicate #define for any constant in vessels.h
- [x] No conflicting values for any constant in vessels.h
- [x] No NEW compiler warnings with -Wall -Wextra (pre-existing warnings in structs.h/util/ remain)
- [x] implementation-notes.md updated
- [x] Ready for `/validate`

---

## Notes

### Parallelization
- T014 can run in parallel once T008-T013 are complete
- T017 and T018 can run in parallel after T016 confirms tests pass

### Task Timing
Target ~20-25 minutes per task.

### Dependencies
1. T001-T003 must complete before T004-T007 (baseline required)
2. T004-T007 must complete before T008-T014 (analysis informs implementation)
3. T008-T013 should be done sequentially (compile-verify after each)
4. T015 must pass before T016-T018

### Key Values to Preserve
- GREYHAWK_MAXSHIPS = 20
- GREYHAWK_MAXSLOTS = 10
- GREYHAWK_ITEM_SHIP = 56 (NOT 57)
- DOCKABLE = ROOM_DOCKABLE alias

### Risk Mitigation
- Compile after each constant category removal
- Keep backup of original vessels.h
- Run tests after each implementation task

---

## Next Steps

Run `/implement` to begin AI-led implementation.
