# Task Checklist

**Session ID**: `phase01-session06-scheduled-route-system`
**Total Tasks**: 20
**Estimated Duration**: 6-8 hours
**Created**: 2025-12-30

---

## Legend

- `[x]` = Completed
- `[ ]` = Pending
- `[P]` = Parallelizable (can run with other [P] tasks)
- `[S0106]` = Session reference (Phase 01, Session 06)
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

Initial verification and environment preparation.

- [x] T001 [S0106] Verify prerequisites met (autopilot system complete, MySQL running)
- [x] T002 [S0106] Create SQL and help file directories if needed

---

## Foundation (5 tasks)

Core structures, constants, and database schema.

- [x] T003 [S0106] Add schedule constants to vessels.h (interval limits, flags)
- [x] T004 [S0106] Define vessel_schedule struct in vessels.h
- [x] T005 [S0106] Add schedule function prototypes to vessels.h
- [x] T006 [S0106] [P] Create ship_schedules table SQL definition (`sql/schedule_tables.sql`)
- [x] T007 [S0106] Implement ensure_schedule_table_exists() in vessels_db.c

---

## Implementation (9 tasks)

Main feature implementation - schedule management and timer logic.

- [x] T008 [S0106] [P] Implement schedule_create() - create schedule for vessel (`src/vessels_autopilot.c`)
- [x] T009 [S0106] [P] Implement schedule_clear() - remove vessel schedule (`src/vessels_autopilot.c`)
- [x] T010 [S0106] Implement schedule_save() - persist to database (`src/vessels_db.c`)
- [x] T011 [S0106] Implement schedule_load() - load single schedule from DB (`src/vessels_db.c`)
- [x] T012 [S0106] Implement load_all_schedules() - boot-time loading (`src/vessels_db.c`)
- [x] T013 [S0106] Implement schedule_tick() - timer check function (`src/vessels_autopilot.c`)
- [x] T014 [S0106] [P] Implement ACMD(do_setschedule) command handler (`src/vessels_autopilot.c`)
- [x] T015 [S0106] [P] Implement ACMD(do_clearschedule) command handler (`src/vessels_autopilot.c`)
- [x] T016 [S0106] [P] Implement ACMD(do_showschedule) command handler (`src/vessels_autopilot.c`)

---

## Integration (2 tasks)

Wire up commands and timer hook.

- [x] T017 [S0106] Register setschedule, clearschedule, showschedule in interpreter.c
- [x] T018 [S0106] Add schedule_tick() call in comm.c heartbeat (MUD hour pulse)

---

## Testing (4 tasks)

Verification, help files, and quality assurance.

- [x] T019 [S0106] [P] Create help file entries for schedule commands (`lib/text/help/schedule.hlp`)
- [x] T020 [S0106] Create unit tests for schedule functions (`unittests/CuTest/test_schedule.c`)
- [x] T021 [S0106] Update Makefile and run test suite (`unittests/CuTest/Makefile`)
- [x] T022 [S0106] Validate ASCII encoding, compile with -Wall -Wextra, verify no warnings

---

## Completion Checklist

Before marking session complete:

- [x] All tasks marked `[x]`
- [x] All tests passing
- [x] All files ASCII-encoded (0-127 characters only)
- [x] Unix LF line endings verified
- [x] No compiler warnings with -Wall -Wextra
- [x] implementation-notes.md updated
- [x] Ready for `/validate`

---

## Notes

### Parallelization
Tasks marked `[P]` can be worked on simultaneously:
- T006 (SQL) can be done alongside foundation work
- T008, T009 (create/clear) are independent functions
- T014, T015, T016 (command handlers) are independent
- T019 (help file) is independent of code tasks

### Task Timing
Target ~20-25 minutes per task.

### Dependencies
- T007 depends on T006 (SQL schema)
- T010-T12 depend on T07 (table exists)
- T13 depends on T08, T11 (schedule creation and loading)
- T17-T18 depend on T13-T16 (functions must exist)
- T20-T21 depend on all implementation tasks

### Key Implementation Details
- MUD hour timing: SECS_PER_MUD_HOUR = 75 real seconds
- Schedule interval: 1-24 MUD hours
- next_departure: Stored as MUD time value
- Timer precision: Use >= comparison, not ==
- Vessel state: Must be docked with valid route to trigger

### Database Table Schema
```sql
CREATE TABLE ship_schedules (
  schedule_id INT AUTO_INCREMENT PRIMARY KEY,
  ship_id INT NOT NULL,
  route_id INT NOT NULL,
  interval_hours INT NOT NULL,
  next_departure INT NOT NULL,
  enabled TINYINT DEFAULT 1,
  created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
  UNIQUE KEY (ship_id)
);
```

---

## Next Steps

Run `/implement` to begin AI-led implementation.
