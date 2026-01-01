# Task Checklist

**Session ID**: `phase00-session07-persistence-integration`
**Total Tasks**: 18
**Estimated Duration**: 4-6 hours
**Created**: 2025-12-29
**Completed**: 2025-12-29

---

## Legend

- `[x]` = Completed
- `[ ]` = Pending
- `[P]` = Parallelizable (can run with other [P] tasks)
- `[S0007]` = Session reference (Phase 00, Session 07)
- `TNNN` = Task ID

---

## Progress Summary

| Category | Total | Done | Remaining |
|----------|-------|------|-----------|
| Setup | 2 | 2 | 0 |
| Foundation | 4 | 4 | 0 |
| Implementation | 8 | 8 | 0 |
| Testing | 4 | 4 | 0 |
| **Total** | **18** | **18** | **0** |

---

## Setup (2 tasks)

Initial verification and environment preparation.

- [x] T001 [S0007] Verify existing DB functions compile and match expected signatures (`src/vessels_db.c`)
- [x] T002 [S0007] Verify greyhawk_ships[] array declaration and GREYHAWK_MAXSHIPS constant accessible (`src/vessels.c`, `src/vessels.h`)

---

## Foundation (4 tasks)

New functions and header declarations.

- [x] T003 [S0007] Implement load_all_ship_interiors() function - iterate greyhawk_ships[], call load_ship_interior() for each valid ship (`src/vessels_db.c`)
- [x] T004 [S0007] Implement save_all_vessels() function - iterate greyhawk_ships[], call save_ship_interior() for each valid ship (`src/vessels_db.c`)
- [x] T005 [S0007] [P] Add helper function is_valid_ship() to check if ship slot has real data (`src/vessels_db.c`)
- [x] T006 [S0007] Declare load_all_ship_interiors(), save_all_vessels(), is_valid_ship() in header (`src/vessels.h`)

---

## Implementation (8 tasks)

Wire persistence functions into server lifecycle.

- [x] T007 [S0007] Wire save_ship_interior() call after generate_ship_interior() in greyhawk_ship_create (`src/vessels_src.c:2400`)
- [x] T008 [S0007] Wire load_all_ship_interiors() into boot_world() after room loading complete (`src/db.c:1248`)
- [x] T009 [S0007] Find server shutdown sequence in comm.c and identify correct hook point (`src/comm.c`)
- [x] T010 [S0007] Wire save_all_vessels() into shutdown sequence before server terminates (`src/comm.c:680`)
- [x] T011 [S0007] Locate dock command handler in vessels_docking.c (`src/vessels_docking.c`)
- [x] T012 [S0007] Wire save_docking_record() call after successful ship docking (`src/vessels_docking.c:248`)
- [x] T013 [S0007] Locate undock command handler in vessels_docking.c (`src/vessels_docking.c`)
- [x] T014 [S0007] Wire end_docking_record() call after successful ship undocking (`src/vessels_docking.c:497`)

---

## Testing (4 tasks)

Verification and quality assurance.

- [x] T015 [S0007] Build project with autotools and verify zero errors, zero warnings (`make clean && make`)
- [x] T016 [S0007] Review all modified files for NULL checks on pointer dereferences
- [x] T017 [S0007] Validate all modified files use ASCII encoding (0-127 characters only)
- [x] T018 [S0007] Update implementation-notes.md with any decisions or challenges encountered

---

## Completion Checklist

Before marking session complete:

- [x] All tasks marked `[x]`
- [x] Project builds with zero errors
- [x] Project builds with zero warnings (in vessel files)
- [x] All files ASCII-encoded
- [x] implementation-notes.md updated
- [x] Ready for `/validate`

---

## Notes

### Parallelization
Task T005 can be worked on in parallel with T003/T004 as it's an independent helper.
Task T006 should wait until T003-T005 are complete to know exact signatures.

### Key Files
| File | Changes |
|------|---------|
| `src/vessels_db.c` | Add load_all_ship_interiors(), save_all_vessels(), is_valid_ship() |
| `src/vessels.h` | Declare new functions |
| `src/db.c` | Wire load_all_ship_interiors() to boot_world() |
| `src/vessels_src.c` | Wire save_ship_interior() after interior generation |
| `src/comm.c` | Wire save_all_vessels() to shutdown |
| `src/vessels_docking.c` | Wire save/end docking records |

### Critical Patterns
- Check `mysql_available` before any DB operation
- Check `shipnum > 0` to identify valid ship slots in greyhawk_ships[]
- Use C90 style: `/* */` comments, variables declared at block start
- Use Allman brace style with 2-space indentation

### Dependencies
- T003, T004, T005 must complete before T006 (need function signatures)
- T007 depends on T003, T004 being complete
- T008, T010, T012, T14 can proceed after T006

---

## Implementation Complete

Session implementation completed 2025-12-29.
Run `/validate` to verify session completeness.
