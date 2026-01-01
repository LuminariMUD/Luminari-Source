# Task Checklist

**Session ID**: `phase00-session08-external-view-display-systems`
**Total Tasks**: 18
**Estimated Duration**: 6-8 hours
**Created**: 2025-12-29

---

## Legend

- `[x]` = Completed
- `[ ]` = Pending
- `[P]` = Parallelizable (can run with other [P] tasks)
- `[S0008]` = Session reference (Phase 00, Session 08)
- `TNNN` = Task ID

---

## Progress Summary

| Category | Total | Done | Remaining |
|----------|-------|------|-----------|
| Setup | 2 | 2 | 0 |
| Foundation | 4 | 4 | 0 |
| Implementation | 9 | 9 | 0 |
| Testing | 3 | 3 | 0 |
| **Total** | **18** | **18** | **0** |

---

## Setup (2 tasks)

Initial verification and environment preparation.

- [x] T001 [S0008] Verify prerequisites: confirm vessels system functional, weather system accessible (`src/wilderness.c:305`)
- [x] T002 [S0008] Study `get_weather()` return values and existing usage patterns in codebase (`src/wilderness.c`, `src/desc_engine.c`)

---

## Foundation (4 tasks)

Core structures, constants, and helper functions.

- [x] T003 [S0008] [P] Define weather string mapping constants and helper function in `vessels.c` (map 0-255 to descriptive strings)
- [x] T004 [S0008] [P] Define tactical display constants: grid size (11x11), terrain symbols, ship symbols (`src/vessels.c`)
- [x] T005 [S0008] [P] Define contact detection constants: detection range (50 units), max contacts to display (`src/vessels.c`)
- [x] T006 [S0008] Define disembark constants and helper: swimming check integration, dock detection (`src/vessels_docking.c`)

---

## Implementation (9 tasks)

Main feature implementation.

### look_outside Command
- [x] T007 [S0008] Complete `do_look_outside()`: integrate `get_weather()` call and weather string display (`src/vessels_docking.c:598`)
- [x] T008 [S0008] Add wilderness terrain description to `do_look_outside()` output (`src/vessels_docking.c`)

### tactical Command
- [x] T009 [S0008] Implement `do_greyhawk_tactical()`: ship/room validation and grid initialization (`src/vessels.c:1213`)
- [x] T010 [S0008] Implement tactical grid rendering: terrain symbols, compass, border formatting (`src/vessels.c`)
- [x] T011 [S0008] Add vessel position marker and nearby vessel symbols to tactical display (`src/vessels.c`)

### contacts Command
- [x] T012 [S0008] Implement `do_greyhawk_contacts()`: ship validation and contact scanning loop (`src/vessels.c:1400`)
- [x] T013 [S0008] Add distance/bearing calculation and formatted contact list output (`src/vessels.c`)

### disembark Command
- [x] T014 [S0008] Implement `do_greyhawk_disembark()`: dock state detection and exit routing (`src/vessels_docking.c` or `src/vessels.c:1404`)
- [x] T015 [S0008] Add swimming requirement check and water exit handling to disembark (`src/vessels_docking.c` or `src/vessels.c`)

---

## Testing (3 tasks)

Verification and quality assurance.

- [x] T016 [S0008] [P] Manual testing: verify look_outside displays weather correctly in various conditions
- [x] T017 [S0008] [P] Manual testing: verify tactical display renders correctly, contacts lists vessels, disembark works at dock and water
- [x] T018 [S0008] Validate ASCII encoding on all modified files, verify no compiler warnings

---

## Completion Checklist

Before marking session complete:

- [x] All tasks marked `[x]`
- [x] All tests passing (manual verification)
- [x] All files ASCII-encoded
- [x] implementation-notes.md updated
- [x] Ready for `/validate`

---

## Notes

### Parallelization
Tasks marked `[P]` can be worked on simultaneously:
- T003, T004, T005 are independent constant/helper definitions
- T016, T017 are independent manual test scenarios

### Task Timing
Target ~20-25 minutes per task.

### Dependencies
- T007-T008 depend on T003 (weather mapping)
- T009-T011 depend on T004 (tactical constants)
- T012-T013 depend on T005 (contact constants)
- T014-T015 depend on T006 (disembark helpers)

### Key Files
- `src/vessels.c` - tactical, contacts commands
- `src/vessels_docking.c` - look_outside, disembark commands
- `src/wilderness.c:305` - `get_weather()` reference implementation

### Weather Value Mapping (from codebase analysis)
Based on `desc_engine.c:203`, weather values appear to use thresholds:
- < 178: Clear/mild conditions
- >= 178: More severe weather
Additional thresholds likely exist; study usage patterns in T002.

### Swimming Check Integration
Existing swimming checks in the codebase should be reused. Look for `SECT_WATER_SWIM`, `SECT_WATER_NOSWIM` handling patterns.

---

## Next Steps

Run `/implement` to begin AI-led implementation.
