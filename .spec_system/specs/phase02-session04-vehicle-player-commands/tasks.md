# Task Checklist

**Session ID**: `phase02-session04-vehicle-player-commands`
**Total Tasks**: 20
**Estimated Duration**: 6-8 hours
**Created**: 2025-12-30
**Completed**: 2025-12-30

---

## Legend

- `[x]` = Completed
- `[ ]` = Pending
- `[P]` = Parallelizable (can run with other [P] tasks)
- `[S0204]` = Session reference (Phase 02, Session 04)
- `TNNN` = Task ID

---

## Progress Summary

| Category | Total | Done | Remaining |
|----------|-------|------|-----------|
| Setup | 3 | 3 | 0 |
| Foundation | 5 | 5 | 0 |
| Implementation | 8 | 8 | 0 |
| Testing | 4 | 4 | 0 |
| **Total** | **20** | **20** | **0** |

---

## Setup (3 tasks)

Initial configuration and build system preparation.

- [x] T001 [S0204] Verify prerequisites: vehicles.c compiled, vehicle_move/add_passenger/remove_passenger functions exist
- [x] T002 [S0204] Create vehicles_commands.c file with header includes and license block (`src/vehicles_commands.c`)
- [x] T003 [S0204] [P] Add vehicles_commands.c to build systems (`CMakeLists.txt`)

---

## Foundation (5 tasks)

Core helper functions and direction parsing infrastructure.

- [x] T004 [S0204] Add ACMD_DECL prototypes for do_vmount, do_vdismount, do_drive, do_vstatus (`src/vessels.h`)
- [x] T005 [S0204] Implement parse_drive_direction() helper for direction string parsing (`src/vehicles_commands.c`)
- [x] T006 [S0204] Implement vehicle_find_in_room() helper - already existed in vehicles.c (`src/vehicles.c`)
- [x] T007 [S0204] Implement is_player_in_vehicle() helper to check mounted state (`src/vehicles_commands.c`)
- [x] T008 [S0204] Implement get_player_vehicle() helper to get current vehicle for mounted player (`src/vehicles_commands.c`)

---

## Implementation (8 tasks)

Main command handler implementations.

- [x] T009 [S0204] Implement do_vmount() command - argument parsing and vehicle lookup (`src/vehicles_commands.c`)
- [x] T010 [S0204] Add do_vmount() validation - capacity, damage state, already mounted checks (`src/vehicles_commands.c`)
- [x] T011 [S0204] Complete do_vmount() - call vehicle_add_passenger(), send success messages (`src/vehicles_commands.c`)
- [x] T012 [S0204] Implement do_vdismount() command with validation and messaging (`src/vehicles_commands.c`)
- [x] T013 [S0204] Implement do_drive() command - direction parsing and validation (`src/vehicles_commands.c`)
- [x] T014 [S0204] Complete do_drive() - call vehicle_move(), handle terrain errors, send messages (`src/vehicles_commands.c`)
- [x] T015 [S0204] Implement do_vstatus() command - display vehicle type, state, position, capacity (`src/vehicles_commands.c`)
- [x] T016 [S0204] Register all four commands in interpreter.c cmd_info[] array (`src/interpreter.c`)

---

## Testing (4 tasks)

Verification and quality assurance.

- [x] T017 [S0204] [P] Create test_vehicle_commands.c with test framework setup (`unittests/CuTest/test_vehicle_commands.c`)
- [x] T018 [S0204] [P] Write unit tests for mount/dismount commands - success and error cases (`unittests/CuTest/test_vehicle_commands.c`)
- [x] T019 [S0204] [P] Write unit tests for drive/vstatus commands - direction parsing, terrain errors (`unittests/CuTest/test_vehicle_commands.c`)
- [x] T020 [S0204] Create help file entries for vmount, vdismount, drive, vstatus (`lib/text/help/vehicles.hlp`)

---

## Completion Checklist

Before marking session complete:

- [x] All tasks marked `[x]`
- [ ] All tests passing (`cd unittests/CuTest && make && ./test_runner`)
- [x] All files ASCII-encoded (characters 0-127 only)
- [x] Unix LF line endings on all files
- [ ] No compiler warnings with `./cbuild.sh`
- [x] implementation-notes.md updated with decisions and learnings
- [ ] Ready for `/validate`

---

## Notes

### Implementation Changes from Spec

1. **Command Names Changed**: Used `vmount`/`vdismount` instead of `mount`/`dismount` to avoid conflict with existing creature mount commands in `act.other.c`.

2. **Function Names Changed**: Used `do_vmount()`/`do_vdismount()` instead of `do_mount()`/`do_dismount()` for the same reason.

3. **Existing Implementations**: `vehicle_find_in_room()` already existed in `vehicles.c`, so T006 was already complete.

### Key Design Decisions

- **Player-vehicle tracking**: Static array mapping player IDs to vehicle IDs
- **Direction parsing**: Supports full names ("north"), abbreviations ("n"), and numeric (0-7)
- **Error messages**: Player-friendly, consistent tone
- **Separation of concerns**: Vehicle commands separate from creature mount commands

---

## Files Created/Modified

### Created
- `src/vehicles_commands.c` - Command implementations
- `unittests/CuTest/test_vehicle_commands.c` - Unit tests
- `lib/text/help/vehicles.hlp` - Help file entries

### Modified
- `src/vessels.h` - Updated ACMD_DECL prototypes (vmount, vdismount)
- `src/interpreter.c` - Added command registrations
- `CMakeLists.txt` - Added vehicles_commands.c to build

---

## Next Steps

1. Run build to verify no compilation errors
2. Run tests to verify functionality
3. Run `/validate` to complete session
