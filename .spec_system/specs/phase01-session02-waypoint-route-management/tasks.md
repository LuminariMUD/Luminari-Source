# Task Checklist

**Session ID**: `phase01-session02-waypoint-route-management`
**Total Tasks**: 20
**Estimated Duration**: 6-8 hours
**Created**: 2025-12-30
**Completed**: 2025-12-30

---

## Legend

- `[x]` = Completed
- `[ ]` = Pending
- `[P]` = Parallelizable (can run with other [P] tasks)
- `[S0102]` = Session reference (Phase 01, Session 02)
- `TNNN` = Task ID

---

## Progress Summary

| Category | Total | Done | Remaining |
|----------|-------|------|-----------|
| Setup | 2 | 2 | 0 |
| Foundation | 5 | 5 | 0 |
| Implementation | 9 | 9 | 0 |
| Testing | 4 | 4 | 0 |
| **Total** | **20** | **20** | **0** |

---

## Setup (2 tasks)

Initial configuration and environment preparation.

- [x] T001 [S0102] Verify prerequisites: build system works, MySQL connection available
- [x] T002 [S0102] Create SQL reference file with table schemas (`sql/waypoint_tables.sql`)

---

## Foundation (5 tasks)

Core structures and base implementations.

- [x] T003 [S0102] [P] Add global cache declarations to vessels.h (`src/vessels.h`)
- [x] T004 [S0102] [P] Add function prototypes for waypoint CRUD to vessels.h (`src/vessels.h`)
- [x] T005 [S0102] [P] Add function prototypes for route CRUD to vessels.h (`src/vessels.h`)
- [x] T006 [S0102] Add table creation SQL to init_vessel_system_tables() (`src/db_init.c`)
- [x] T007 [S0102] Create in-memory cache infrastructure: waypoint_list, route_list globals (`src/vessels_autopilot.c`)

---

## Implementation (9 tasks)

Main feature implementation.

- [x] T008 [S0102] Implement waypoint_db_create() with MySQL INSERT (`src/vessels_autopilot.c`)
- [x] T009 [S0102] Implement waypoint_db_load() and waypoint_db_load_by_id() (`src/vessels_autopilot.c`)
- [x] T010 [S0102] Implement waypoint_db_update() and waypoint_db_delete() (`src/vessels_autopilot.c`)
- [x] T011 [S0102] Implement route_db_create() with MySQL INSERT (`src/vessels_autopilot.c`)
- [x] T012 [S0102] Implement route_db_load() and route_db_load_by_id() (`src/vessels_autopilot.c`)
- [x] T013 [S0102] Implement route_db_update() and route_db_delete() (`src/vessels_autopilot.c`)
- [x] T014 [S0102] Implement route_add_waypoint_db(), route_remove_waypoint_db(), route_reorder_waypoints_db() (`src/vessels_autopilot.c`)
- [x] T015 [S0102] Implement load_all_waypoints() and load_all_routes() for boot-time loading (`src/vessels_autopilot.c`)
- [x] T016 [S0102] Implement save_all_waypoints() and save_all_routes() for shutdown saving (`src/vessels_autopilot.c`)

---

## Testing (4 tasks)

Verification and quality assurance.

- [x] T017 [S0102] Create test file structure with mock setup (`unittests/CuTest/test_waypoint_cache.c`)
- [x] T018 [S0102] Write unit tests for waypoint CRUD operations (`unittests/CuTest/test_waypoint_cache.c`)
- [x] T019 [S0102] Write unit tests for route CRUD and route-waypoint associations (`unittests/CuTest/test_waypoint_cache.c`)
- [x] T020 [S0102] Run full test suite - 11 tests passing (`unittests/CuTest/test_waypoint_cache`)

---

## Completion Checklist

Before marking session complete:

- [x] All tasks marked `[x]`
- [x] All tests passing (11/11)
- [x] All files ASCII-encoded (0-127 only)
- [x] Unix LF line endings verified
- [x] No compiler warnings with -Wall -Wextra
- [ ] No memory leaks (Valgrind clean) - pending full Valgrind test
- [x] implementation-notes.md updated
- [ ] Ready for `/validate`

---

## Notes

### Parallelization
Tasks T003, T004, T005 can be worked on simultaneously as they modify different sections of vessels.h.

### Task Timing
Target approximately 20-25 minutes per task.

### Dependencies
- T006 depends on T002 (table schemas must be defined first)
- T007 depends on T003 (cache declarations needed before implementation)
- T008-T016 depend on T006, T007 (infrastructure must be in place)
- T017-T20 depend on T008-T016 (CRUD functions must exist to test)

### Key Implementation Patterns
Follow existing patterns from `vessels_db.c`:
- Use `mysql_available` check before all DB operations
- Use `mysql_real_escape_string()` for all string values
- Use `snprintf()` for buffer safety
- Return database-generated IDs using `mysql_insert_id()`
- Use `mysql_store_result()` and `mysql_fetch_row()` for SELECT queries
- Always `mysql_free_result()` after query completion

### Database Schema Reference
```sql
ship_waypoints: waypoint_id, name, x, y, z, tolerance, wait_time, flags
ship_routes: route_id, name, loop_route, active
ship_route_waypoints: id, route_id, waypoint_id, sequence_num
```

### ASCII Reminder
All output files must use ASCII-only characters (0-127). No smart quotes, em-dashes, or non-ASCII symbols.

---

## Session Complete

All 20 tasks completed. Run `/validate` to verify session completeness.
