# Session Specification

**Session ID**: `phase01-session01-autopilot-data-structures`
**Phase**: 01 - Automation Layer
**Status**: Not Started
**Created**: 2025-12-30

---

## 1. Session Overview

This session establishes the foundational data structures for the vessel autopilot system. As the first session of Phase 01 (Automation Layer), it follows the proven "headers-first" pattern from Phase 00 Session 01, defining all types, constants, and function prototypes before any implementation logic.

The autopilot system will enable vessels to navigate autonomously along predefined waypoint routes. This session creates the structural foundation: waypoint definitions, route containers, autopilot state machines, and the integration point with the existing `greyhawk_ship_data` structure. Without these structures, no subsequent autopilot functionality (movement logic, persistence, commands, NPC pilots) can be implemented.

The design prioritizes memory efficiency (targeting <1KB overhead per vessel for autopilot data) and clean integration with the existing Phase 00 vessel system. All structures will be ANSI C90 compliant, use block comments only, and follow the established naming conventions from the codebase.

---

## 2. Objectives

1. Define `struct autopilot_data` containing state machine, waypoint tracking, and timing fields
2. Define `struct waypoint` for individual navigation points with coordinates and metadata
3. Define `struct ship_route` for ordered collections of waypoints with route metadata
4. Extend `greyhawk_ship_data` with autopilot integration pointer and status fields
5. Create `vessels_autopilot.c` skeleton with stub implementations for all prototypes
6. Update CMakeLists.txt to compile the new source file

---

## 3. Prerequisites

### Required Sessions
- [x] `phase00-session01-header-cleanup-foundation` - Clean vessels.h structure
- [x] `phase00-session02-dynamic-wilderness-room-allocation` - Wilderness coordinate system
- [x] `phase00-session03-vessel-type-system` - Vessel type enumeration
- [x] `phase00-session07-persistence-integration` - Database patterns for vessels
- [x] `phase00-session09-testing-validation` - Phase 00 validation complete

### Required Tools/Knowledge
- ANSI C90/C89 structure syntax and conventions
- Understanding of greyhawk_ship_data structure (vessels.h:431-499)
- Wilderness coordinate system (2048x2048 grid, X/Y/Z)

### Environment Requirements
- GCC or Clang with -Wall -Wextra
- CMake build system operational
- CuTest framework for unit testing

---

## 4. Scope

### In Scope (MVP)
- `struct autopilot_data` with state enum, current waypoint index, timing fields
- `struct waypoint` with X/Y/Z coordinates, name, arrival tolerance, wait time
- `struct ship_route` with waypoint array, route name, loop flag, active flag
- Autopilot state constants: AUTOPILOT_OFF, AUTOPILOT_TRAVELING, AUTOPILOT_WAITING, AUTOPILOT_PAUSED, AUTOPILOT_COMPLETE
- Configuration constants: MAX_WAYPOINTS_PER_ROUTE, MAX_ROUTES_PER_SHIP, AUTOPILOT_TICK_INTERVAL
- Extend greyhawk_ship_data with `struct autopilot_data *autopilot` pointer
- Function prototypes for autopilot lifecycle (init, cleanup, start, stop, pause, resume)
- Function prototypes for waypoint management (add, remove, clear, get_current, get_next)
- Function prototypes for route management (create, destroy, load, save, activate)
- Source file skeleton `vessels_autopilot.c` with stub implementations
- Unit test skeleton for structure validation

### Out of Scope (Deferred)
- Actual movement logic - *Reason: Session 03 (Path Following Logic)*
- Database persistence schemas - *Reason: Session 02 (Waypoint Route Management)*
- Player autopilot commands - *Reason: Session 04 (Autopilot Player Commands)*
- NPC pilot behavior - *Reason: Session 05 (NPC Pilot Integration)*
- Scheduled route execution - *Reason: Session 06 (Scheduled Route System)*

---

## 5. Technical Approach

### Architecture

The autopilot system uses a three-layer architecture:

1. **Data Layer** (this session): Structures and constants
2. **Logic Layer** (Session 03): Movement and pathfinding
3. **Interface Layer** (Session 04): Player commands

The `autopilot_data` structure attaches to `greyhawk_ship_data` via pointer, allowing optional autopilot capability per vessel. Routes are stored separately and referenced by ID, enabling route sharing between vessels.

```
greyhawk_ship_data
    |
    +-> autopilot_data (pointer, NULL if no autopilot)
            |
            +-> current_route (pointer to active ship_route)
            +-> state (enum autopilot_state)
            +-> current_waypoint_index
            +-> timing fields
```

### Design Patterns
- **Pointer attachment**: Autopilot data optional via NULL pointer (memory efficient)
- **State machine**: Clear autopilot states for predictable behavior
- **Index-based navigation**: Waypoints accessed by index within route array

### Technology Stack
- ANSI C90/C89 (no C99 features)
- GCC/Clang with -Wall -Wextra
- CuTest for unit testing

---

## 6. Deliverables

### Files to Create
| File | Purpose | Est. Lines |
|------|---------|------------|
| `src/vessels_autopilot.c` | Autopilot function stubs and initialization | ~150 |
| `unittests/CuTest/autopilot_struct_test.c` | Structure validation tests | ~100 |

### Files to Modify
| File | Changes | Est. Lines |
|------|---------|------------|
| `src/vessels.h` | Add autopilot structures, constants, prototypes | ~120 |
| `CMakeLists.txt` | Add vessels_autopilot.c to source list | ~2 |
| `unittests/CuTest/Makefile` | Add autopilot test compilation | ~5 |

---

## 7. Success Criteria

### Functional Requirements
- [ ] `struct autopilot_data` defined with all required fields
- [ ] `struct waypoint` defined with X/Y/Z, name, tolerance, wait_time
- [ ] `struct ship_route` defined with waypoint array and metadata
- [ ] `enum autopilot_state` defined with 5 states
- [ ] Constants defined: MAX_WAYPOINTS_PER_ROUTE (20), MAX_ROUTES_PER_SHIP (5), AUTOPILOT_TICK_INTERVAL
- [ ] `greyhawk_ship_data` extended with autopilot pointer
- [ ] All function prototypes declared in vessels.h
- [ ] `vessels_autopilot.c` compiles without errors or warnings

### Testing Requirements
- [ ] Unit tests validate structure sizes and field offsets
- [ ] Unit tests verify constant values
- [ ] All tests pass with zero failures
- [ ] Valgrind reports zero memory leaks in tests

### Quality Gates
- [ ] All files ASCII-encoded (0-127 characters only)
- [ ] Unix LF line endings throughout
- [ ] Code follows ANSI C90 (no // comments, no declarations after statements)
- [ ] Two-space indentation, Allman-style braces
- [ ] No compiler warnings with -Wall -Wextra
- [ ] No duplicate struct definitions introduced

---

## 8. Implementation Notes

### Key Considerations
- Place autopilot structures AFTER greyhawk_ship_data in vessels.h to avoid forward declaration issues
- Use fixed-size arrays for waypoint names to avoid dynamic allocation complexity
- Autopilot state should be separate from vessel state (VESSEL_STATE_TRAVELING)
- Coordinate tolerance should account for floating-point comparison issues

### Potential Challenges
- **Header ordering**: Autopilot structures reference greyhawk_ship_data - place definitions carefully
- **Memory alignment**: Ensure struct packing is efficient for 500 vessel target
- **Duplicate definitions**: vessels.h already has duplicate constants (lines 50-58 vs 89-97) - avoid adding more

### Relevant Considerations
- [P00] **Duplicate struct definitions**: Be careful not to introduce more duplicates - verify no existing autopilot-related definitions exist
- [P00] **ANSI C90/C89 codebase**: No // comments, no declarations after statements, no inline functions
- [P00] **Max 500 concurrent vessels**: Autopilot structures must be memory-efficient - use compact types

### ASCII Reminder
All output files must use ASCII-only characters (0-127). No Unicode quotes, dashes, or special characters.

---

## 9. Testing Strategy

### Unit Tests
- Test struct autopilot_data field accessibility and sizes
- Test struct waypoint can store full coordinate range (-1024 to +1024)
- Test struct ship_route can hold MAX_WAYPOINTS_PER_ROUTE entries
- Test enum autopilot_state has expected values
- Test constant values are within expected bounds

### Integration Tests
- Verify vessels.h compiles cleanly with autopilot additions
- Verify vessels_autopilot.c links with existing vessel code
- Verify full project builds without errors

### Manual Testing
- None required for data structure session

### Edge Cases
- Waypoint coordinates at boundaries (-1024, +1024)
- Empty routes (0 waypoints)
- Maximum routes per ship (5)
- Maximum waypoints per route (20)
- NULL autopilot pointer handling

---

## 10. Dependencies

### External Libraries
- None (pure C structures)

### Other Sessions
- **Depends on**: Phase 00 complete (all 9 sessions)
- **Depended by**:
  - Session 02 (Waypoint Route Management) - needs structures for DB schema
  - Session 03 (Path Following Logic) - needs structures for movement implementation
  - Session 04 (Autopilot Player Commands) - needs structures for command handlers
  - Session 05 (NPC Pilot Integration) - needs autopilot state for NPC behavior
  - Session 06 (Scheduled Route System) - needs route structures for scheduling

---

## Structure Definitions Reference

### autopilot_state enum
```c
enum autopilot_state {
  AUTOPILOT_OFF,        /* Autopilot disabled */
  AUTOPILOT_TRAVELING,  /* Moving toward waypoint */
  AUTOPILOT_WAITING,    /* At waypoint, waiting */
  AUTOPILOT_PAUSED,     /* Temporarily suspended */
  AUTOPILOT_COMPLETE    /* Route finished */
};
```

### waypoint struct
```c
struct waypoint {
  float x, y, z;          /* Target coordinates */
  char name[64];          /* Waypoint name */
  float tolerance;        /* Arrival distance threshold */
  int wait_time;          /* Seconds to wait at waypoint */
  int flags;              /* Waypoint flags (future use) */
};
```

### ship_route struct
```c
struct ship_route {
  int route_id;                                    /* Unique route identifier */
  char name[64];                                   /* Route name */
  struct waypoint waypoints[MAX_WAYPOINTS_PER_ROUTE];
  int num_waypoints;                               /* Actual waypoint count */
  bool loop;                                       /* Repeat route when complete */
  bool active;                                     /* Route is available */
};
```

### autopilot_data struct
```c
struct autopilot_data {
  enum autopilot_state state;          /* Current autopilot state */
  struct ship_route *current_route;    /* Active route (NULL if none) */
  int current_waypoint_index;          /* Index in route waypoints array */
  int tick_counter;                    /* Ticks since last update */
  int wait_remaining;                  /* Seconds left at current waypoint */
  time_t last_update;                  /* Timestamp of last state update */
  int pilot_mob_vnum;                  /* VNUM of NPC pilot (-1 if none) */
};
```

---

## Next Steps

Run `/tasks` to generate the implementation task checklist.
