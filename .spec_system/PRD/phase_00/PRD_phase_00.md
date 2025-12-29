# PRD Phase 00: Core Vessel System

**Status**: In Progress
**Sessions**: 9 (initial estimate)
**Estimated Duration**: 4-6 days

**Progress**: 4/9 sessions (44%)

---

## Overview

Complete the core vessel system implementation by wiring existing Phase 1 and Phase 2 code, implementing missing functionality, and ensuring all features are properly integrated and tested. This phase transforms partially-implemented code into a fully functional vessel navigation system.

### Current State Summary

**Working:**
- Build integration (CMake/autotools)
- Boot initialization (greyhawk_initialize_ships() in db.c)
- 9 commands registered in interpreter.c
- Wilderness coordinate navigation (X/Y/Z with -1024 to +1024 range)
- Terrain restrictions and weather effects
- Basic boarding via ITEM_GREYHAWK_SHIP

**Code Exists, Not Wired:**
- Room generation (vessels_rooms.c)
- Docking handlers (vessels_docking.c)
- DB tables (5 tables auto-created at startup)
- Room templates (19 types in ship_room_templates)

**Missing Implementation:**
- ~~Dynamic wilderness room allocation~~ (Session 02 Complete)
- ~~Per-vessel type mapping~~ (Session 03 Complete)
- ~~Phase 2 command registration~~ (Session 04 Complete)
- Interior movement functions
- Persistence integration

---

## Progress Tracker

| Session | Name | Status | Est. Tasks | Validated |
|---------|------|--------|------------|-----------|
| 01 | Header Cleanup & Foundation | Complete | 20 | 2025-12-29 |
| 02 | Dynamic Wilderness Room Allocation | Complete | 18 | 2025-12-29 |
| 03 | Vessel Type System | Complete | 20 | 2025-12-29 |
| 04 | Phase 2 Command Registration | Complete | 15 | 2025-12-29 |
| 05 | Interior Room Generation Wiring | Not Started | ~18-22 | - |
| 06 | Interior Movement Implementation | Not Started | ~20-25 | - |
| 07 | Persistence Integration | Not Started | ~18-22 | - |
| 08 | External View & Display Systems | Not Started | ~20-25 | - |
| 09 | Testing & Validation | Not Started | ~15-20 | - |

---

## Completed Sessions

### Session 01: Header Cleanup & Foundation
- **Completed**: 2025-12-29
- **Tasks**: 20/20
- **Summary**: Removed 123 lines of duplicate definitions from vessels.h, consolidated struct definitions, preserved Phase 2 multi-room fields, fixed pre-existing build issues (GOLEM_* constants, missing source files)

### Session 02: Dynamic Wilderness Room Allocation
- **Completed**: 2025-12-29
- **Tasks**: 18/18
- **Summary**: Implemented dynamic wilderness room allocation for vessels using the proven pattern from movement.c. Created helper function `get_or_allocate_wilderness_room()` and updated 3 vessel functions to use dynamic allocation with coordinate bounds validation and error logging.

### Session 03: Vessel Type System
- **Completed**: 2025-12-29
- **Tasks**: 20/20
- **Summary**: Implemented per-vessel type terrain capabilities using static lookup table. Created vessel_terrain_data array mapping all 8 vessel types to 40 sector types. Added get_vessel_terrain_caps(), get_vessel_type_from_ship(), and get_vessel_type_name() accessors. Replaced all hardcoded VESSEL_TYPE_SAILING_SHIP references with actual vessel type lookups.

### Session 04: Phase 2 Command Registration
- **Completed**: 2025-12-29
- **Tasks**: 15/15
- **Summary**: Registered 5 Phase 2 vessel commands (dock, undock, board_hostile, look_outside, ship_rooms) in interpreter.c cmd_info[] array. Created help entries for all commands in lib/text/help/help.hlp. Build compiles cleanly with no new warnings. Commands are now recognized by the parser and ready for testing.

---

## Upcoming Sessions

- Session 05: Interior Room Generation Wiring

---

## Objectives

1. **Fix Code Quality Issues** - Resolve duplicate definitions and establish clean foundation
2. **Complete Movement System** - Enable vessels to navigate wilderness with dynamic room allocation
3. **Wire Interior Systems** - Connect room generation, movement, and persistence
4. **Enable Ship Interaction** - Register and wire docking/boarding commands
5. **Ensure Reliability** - Comprehensive testing and validation

---

## Prerequisites

- MySQL/MariaDB database operational
- Wilderness system functional
- Zone 213 test area configured
- Build system working (CMake or autotools)

---

## Technical Considerations

### Architecture
- All vessel code follows ANSI C90/C89 standard
- Integration with existing wilderness coordinate system
- Database tables auto-created at server startup
- Interior VNUM range 30000-40019 reserved

### Technologies
- C90/C89 with GNU extensions
- MySQL/MariaDB for persistence
- CuTest for unit testing
- Valgrind for memory validation

### Risks
- **Movement Complexity**: Dynamic room allocation may have edge cases - Mitigation: Thorough testing with various coordinates
- **Performance**: Room generation overhead - Mitigation: Profile and optimize as needed
- **Integration**: Many touchpoints with existing systems - Mitigation: Incremental wiring with testing at each step

### Relevant Considerations
- **[P00] Duplicate struct definitions**: Must be resolved in Session 01 before further work
- **[P00] Silent movement failures**: Critical bug addressed in Session 02
- **[P00] Hard-coded room templates**: Should query DB table instead (Session 05)
- **[P00] Interior movement unimplemented**: Core functionality for Session 06

---

## Success Criteria

Phase complete when:
- [ ] All 9 sessions completed and validated
- [ ] No duplicate definitions in vessels.h
- [ ] Vessel movement works across entire wilderness grid
- [ ] All Phase 2 commands registered and functional
- [ ] Interior rooms generated and navigable
- [ ] Persistence working across server restarts
- [ ] Unit tests created and passing
- [ ] Memory validation clean (Valgrind)
- [ ] Performance targets met (<100ms response, <1KB per vessel)

---

## Dependencies

### Depends On
- Wilderness system (src/wilderness.c)
- Weather system (src/weather.c)
- Command interpreter (src/interpreter.c)
- Database layer (src/db.c, MySQL)

### Enables
- Phase 01: Automation Layer (autopilot, NPC pilots)
- Phase 02: Simple Vehicle Support
- Phase 03: Optimization & Polish

---

## Key Files

| File | Purpose |
|------|---------|
| src/vessels.h | Structures, constants, prototypes |
| src/vessels.c | Core commands, wilderness movement |
| src/vessels_rooms.c | Interior room generation |
| src/vessels_docking.c | Docking and boarding |
| src/vessels_db.c | Database persistence |
| src/db_init.c | Table creation at startup |
| src/db_init_data.c | Template population |
| src/interpreter.c | Command registration |
| src/db.c | Boot sequence integration |
| src/movement.c | Wilderness room allocation patterns |
