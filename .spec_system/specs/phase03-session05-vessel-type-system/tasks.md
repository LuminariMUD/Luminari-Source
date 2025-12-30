# Task Checklist

**Session ID**: `phase03-session05-vessel-type-system`
**Total Tasks**: 20
**Estimated Duration**: 6-8 hours
**Created**: 2025-12-30

---

## Legend

- `[x]` = Completed
- `[ ]` = Pending
- `[P]` = Parallelizable (can run with other [P] tasks)
- `[S0305]` = Session reference (Phase 03, Session 05)
- `TNNN` = Task ID

---

## Progress Summary

| Category | Total | Done | Remaining |
|----------|-------|------|-----------|
| Setup | 3 | 3 | 0 |
| Foundation | 5 | 5 | 0 |
| Implementation | 6 | 6 | 0 |
| Testing | 6 | 6 | 0 |
| **Total** | **20** | **20** | **0** |

---

## Setup (3 tasks)

Initial verification and environment preparation.

- [x] T001 [S0305] Verify build succeeds (`./cbuild.sh`)
- [x] T002 [S0305] Run existing tests to confirm baseline (`unittests/CuTest/test_runner`)
- [x] T003 [S0305] Read and audit production vessel type code (`src/vessels.c`, `src/vessels.h`)

---

## Foundation (5 tasks)

Code audit and gap identification for all 8 vessel types.

- [x] T004 [S0305] [P] Audit vessel_terrain_data[] table in vessels.c for all 8 types
- [x] T005 [S0305] [P] Audit derive_vessel_type_from_template() in vessels_src.c
- [x] T006 [S0305] [P] Audit get_vessel_type_from_ship() and get_vessel_terrain_caps()
- [x] T007 [S0305] Verify can_vessel_traverse_terrain() uses actual vessel type
- [x] T008 [S0305] Verify get_terrain_speed_modifier() uses actual vessel type

---

## Implementation (6 tasks)

Fix gaps and add missing integration points.

- [x] T009 [S0305] Document any gaps found between spec and production code
- [x] T010 [S0305] Fix any hardcoded VESSEL_TYPE_SAILING_SHIP placeholders in vessels.c
- [x] T011 [S0305] Ensure all ship creation paths populate vessel_type field
- [x] T012 [S0305] Verify database load/save preserves vessel_type (`src/vessels_db.c`)
- [x] T013 [S0305] Add warship capability tests to test_vessel_types.c (`unittests/CuTest/test_vessel_types.c`)
- [x] T014 [S0305] Add transport capability tests to test_vessel_types.c (`unittests/CuTest/test_vessel_types.c`)

---

## Testing (6 tasks)

Integration tests and comprehensive validation.

- [x] T015 [S0305] Create test_vessel_type_integration.c with end-to-end tests (`unittests/CuTest/test_vessel_type_integration.c`)
- [x] T016 [S0305] Add integration tests for movement validation with vessel types
- [x] T017 [S0305] Update CuTest Makefile to include new test file (`unittests/CuTest/Makefile`)
- [x] T018 [S0305] Run full test suite and verify all 20+ tests pass
- [x] T019 [S0305] Run Valgrind to verify no memory leaks
- [x] T020 [S0305] Update CONSIDERATIONS.md to resolve "Per-vessel type mapping missing" concern

---

## Completion Checklist

Before marking session complete:

- [x] All tasks marked `[x]`
- [x] All tests passing (104 vessel type tests)
- [x] Valgrind clean (no memory leaks)
- [x] All files ASCII-encoded
- [x] implementation-notes.md updated
- [x] Ready for `/validate`

---

## Notes

### Parallelization
Tasks T004-T006 can be worked on simultaneously during the audit phase.

### Key Files
| File | Purpose |
|------|---------|
| `src/vessels.h:127-137` | enum vessel_class definition |
| `src/vessels.c:62-447` | vessel_terrain_data[] table |
| `src/vessels.c:462-527` | accessor functions |
| `src/vessels_src.c:2390` | type derivation during creation |
| `src/vessels_db.c:144` | type loading from database |

### Current Test Count
- Existing tests in test_vessel_types.c: 19 tests
- Target: 20+ tests total
- Gap: Need warship/transport explicit tests + integration tests

### Active Concern to Resolve
From CONSIDERATIONS.md line 39:
> "Per-vessel type mapping missing - Currently hardcoded to VESSEL_TYPE_SAILING_SHIP placeholder."

This session must verify this is resolved and update the concern status.

### ASCII Reminder
All output files must use ASCII-only characters (0-127).

---

## Next Steps

Run `/implement` to begin AI-led implementation.
