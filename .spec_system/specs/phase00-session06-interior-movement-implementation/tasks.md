# Task Checklist

**Session ID**: `phase00-session06-interior-movement-implementation`
**Total Tasks**: 18
**Estimated Duration**: 6-8 hours
**Created**: 2025-12-29

---

## Legend

- `[x]` = Completed
- `[ ]` = Pending
- `[P]` = Parallelizable (can run with other [P] tasks)
- `[S0006]` = Session reference (Phase 00, Session 06)
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

Initial configuration and environment preparation.

- [x] T001 [S0006] Verify prerequisites met - confirm session05 complete and ship interior rooms exist
- [x] T002 [S0006] Review movement.c do_simple_move() structure for integration point (`src/movement.c:140-982`)

---

## Foundation (4 tasks)

Core structures and base implementations.

- [x] T003 [S0006] [P] Add include guards and required headers in vessels_rooms.c for movement integration (`src/vessels_rooms.c`)
- [x] T004 [S0006] [P] Define helper macro/function for is_in_ship_interior() detection (`src/vessels_rooms.c`)
- [x] T005 [S0006] [P] Add extern declaration for rev_dir[] if needed (`src/vessels_rooms.c`)
- [x] T006 [S0006] Document the three function signatures with Doxygen-style comments (`src/vessels_rooms.c`)

---

## Implementation (8 tasks)

Main feature implementation.

- [x] T007 [S0006] Implement get_ship_exit() - search connections for direction match (`src/vessels_rooms.c`)
- [x] T008 [S0006] Implement is_passage_blocked() - check is_locked flag on connection (`src/vessels_rooms.c`)
- [x] T009 [S0006] Implement do_move_ship_interior() skeleton with NULL checks and validation (`src/vessels_rooms.c`)
- [x] T010 [S0006] Add exit lookup logic to do_move_ship_interior() using get_ship_exit() (`src/vessels_rooms.c`)
- [x] T011 [S0006] Add passage blocked check to do_move_ship_interior() using is_passage_blocked() (`src/vessels_rooms.c`)
- [x] T012 [S0006] Add character movement logic with char_from_room/char_to_room (`src/vessels_rooms.c`)
- [x] T013 [S0006] Add movement messages for entering/leaving ship rooms (`src/vessels_rooms.c`)
- [x] T014 [S0006] Wire ship interior detection into do_simple_move() with delegation (`src/movement.c`)

---

## Testing (4 tasks)

Verification and quality assurance.

- [x] T015 [S0006] Compile and fix any errors or warnings
- [x] T016 [S0006] Validate ASCII encoding on all modified files
- [x] T017 [S0006] Manual testing - navigate ship interior rooms with direction commands
- [x] T018 [S0006] Verify existing wilderness/normal room movement still works

---

## Completion Checklist

Before marking session complete:

- [x] All tasks marked `[x]`
- [x] All code compiles without warnings
- [x] All files ASCII-encoded with Unix LF line endings
- [x] Code follows C90 conventions (no // comments, declarations at block start)
- [x] NULL pointer checks on all function entries
- [x] implementation-notes.md updated
- [x] Ready for `/validate`

---

## Notes

### Parallelization
Tasks T003, T004, T005, T006 are parallelizable as they set up independent components.

### Task Timing
Target ~20-25 minutes per task.

### Dependencies
- T007-T013 must be done sequentially (each builds on previous)
- T014 depends on T007-T013 being complete
- T015-T018 depend on all implementation tasks

### Key Code Locations
- `vessels.h:560-563` - Function declarations
- `vessels.h:421-428` - room_connection structure
- `vessels.h:488-489` - connections array and num_connections
- `vessels_rooms.c:620` - get_ship_from_room() implementation
- `movement.c:140` - do_simple_move() entry point

### Critical Implementation Details
1. **Bidirectional lookup**: Check both from_room->to_room and to_room->from_room
2. **VNUM vs RNUM**: Connections store vnums; use real_room() for comparison
3. **Direction reversal**: Use rev_dir[] for reverse direction lookup
4. **Integration point**: Add ship interior check early in do_simple_move()

---

## Next Steps

Run `/implement` to begin AI-led implementation.
