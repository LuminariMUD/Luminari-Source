# Session Specification

**Session ID**: `phase01-session03-path-following-logic`
**Phase**: 01 - Automation Layer
**Status**: Not Started
**Created**: 2025-12-30

---

## 1. Session Overview

This session implements the core autopilot movement engine that enables vessels to navigate autonomously between waypoints along predefined routes. Building on the data structures from Session 01 and the waypoint/route persistence from Session 02, this session creates the "brain" that actually makes vessels move.

The path-following logic is the critical engine component of the autopilot system. It integrates with the game's heartbeat loop to periodically check all vessels with active autopilot, calculate heading and distance to their current waypoint, execute movement toward that waypoint at the configured speed, detect arrival and advance to the next waypoint, and handle route completion (either stopping or looping). Without this session, the autopilot system remains inert data structures with no actual movement capability.

The implementation must respect the existing wilderness coordinate system (2048x2048 grid), use the established `find_available_wilderness_room()` + `assign_wilderness_room()` pattern for terrain-validated movement, integrate seamlessly with the comm.c heartbeat without performance impact, and support 100+ concurrent autopilot vessels. All code will be ANSI C90 compliant.

---

## 2. Objectives

1. Implement `autopilot_tick()` as the main autopilot update function called from the game loop heartbeat
2. Implement direction/distance calculation to current waypoint using Euclidean geometry
3. Implement terrain-validated movement toward waypoints using the wilderness room allocation pattern
4. Implement waypoint arrival detection, wait time handling, and route advancement logic
5. Add autopilot heartbeat integration to comm.c using existing pulse patterns

---

## 3. Prerequisites

### Required Sessions
- [x] `phase01-session01-autopilot-data-structures` - Provides AUTOPILOT_STATE_*, autopilot_data, waypoint, ship_route structures
- [x] `phase01-session02-waypoint-route-management` - Provides waypoint/route CRUD, cache, and persistence

### Required Tools/Knowledge
- Wilderness coordinate system (2048x2048 grid with X/Y/Z)
- Game loop heartbeat integration pattern (comm.c:1368)
- Terrain-validated movement pattern (find_available_wilderness_room + assign_wilderness_room)
- Euclidean distance/heading calculations

### Environment Requirements
- GCC or Clang with -Wall -Wextra
- CMake build system operational
- CuTest framework for unit testing
- Valgrind for memory leak testing

---

## 4. Scope

### In Scope (MVP)
- `autopilot_tick()` function processing all vessels with active autopilot
- `calculate_heading_to_waypoint()` function returning direction vector
- `calculate_distance_to_waypoint()` function returning Euclidean distance
- `check_waypoint_arrival()` function testing against waypoint tolerance
- `advance_to_next_waypoint()` function handling route progression
- Wait time handling at waypoints (decrementing wait_remaining)
- Route completion logic for both loop and non-loop routes
- Terrain validation before movement using wilderness room allocation
- Game loop integration via heartbeat pulse timing
- Unit tests for all path-following logic

### Out of Scope (Deferred)
- Player commands (autopilot on/off/status) - *Reason: Session 04 scope*
- NPC pilot behavior and assignment - *Reason: Session 05 scope*
- Scheduled/timed route triggers - *Reason: Session 06 scope*
- Collision avoidance between vessels - *Reason: Future enhancement*
- Speed modifiers for terrain/weather - *Reason: Future enhancement*

---

## 5. Technical Approach

### Architecture
The autopilot tick system uses a polling architecture integrated with the game heartbeat:

```
heartbeat() [comm.c]
    |
    v
autopilot_tick() [every AUTOPILOT_TICK_INTERVAL pulses]
    |
    +-- For each vessel with autopilot->state == AUTOPILOT_TRAVELING
    |       |
    |       +-- calculate_distance_to_waypoint()
    |       +-- check_waypoint_arrival()
    |       |       |
    |       |       +-- Yes: handle_waypoint_arrival()
    |       |       |           |
    |       |       |           +-- wait_time > 0: state = WAITING
    |       |       |           +-- has next: advance_to_next_waypoint()
    |       |       |           +-- no next: state = COMPLETE (or loop)
    |       |       |
    |       |       +-- No: move_vessel_toward_waypoint()
    |       |                   |
    |       |                   +-- calculate_heading_to_waypoint()
    |       |                   +-- find_available_wilderness_room()
    |       |                   +-- assign_wilderness_room()
    |       |
    +-- For each vessel with autopilot->state == AUTOPILOT_WAITING
            |
            +-- Decrement wait_remaining
            +-- If zero: state = TRAVELING, advance_to_next_waypoint()
```

### Design Patterns
- **Iterator Pattern**: autopilot_tick iterates through vessel_list checking autopilot state
- **State Machine Pattern**: AUTOPILOT_OFF/TRAVELING/WAITING/PAUSED/COMPLETE transitions
- **Template Method Pattern**: Common autopilot tick with specialized arrival/movement handlers

### Technology Stack
- ANSI C90/C89 with GNU extensions
- MySQL/MariaDB for route/waypoint persistence (via Session 02)
- Wilderness coordinate system (wilderness.h/wilderness.c)

---

## 6. Deliverables

### Files to Create
| File | Purpose | Est. Lines |
|------|---------|------------|
| `unittests/CuTest/test_autopilot_pathfinding.c` | Unit tests for path-following logic | ~200 |

### Files to Modify
| File | Changes | Est. Lines |
|------|---------|------------|
| `src/vessels_autopilot.c` | Add autopilot_tick(), calculate/check/advance functions | ~300 |
| `src/vessels.h` | Add function prototypes for new path-following functions | ~15 |
| `src/comm.c` | Add autopilot_tick() call in heartbeat() | ~10 |
| `unittests/CuTest/Makefile` | Add test_autopilot_pathfinding to build | ~5 |
| `unittests/CuTest/AllTests.c` | Register autopilot tests | ~5 |

---

## 7. Success Criteria

### Functional Requirements
- [ ] Vessels with autopilot state TRAVELING move toward current waypoint each tick
- [ ] Vessels detect arrival when within waypoint tolerance distance
- [ ] Vessels wait at waypoints for configured wait_time seconds
- [ ] Vessels advance through all waypoints in route order
- [ ] Loop routes restart from first waypoint after completion
- [ ] Non-loop routes set state to COMPLETE at final waypoint
- [ ] Invalid terrain destinations prevent movement (no silent failures)
- [ ] Autopilot respects AUTOPILOT_TICK_INTERVAL timing

### Testing Requirements
- [ ] Unit tests for calculate_distance_to_waypoint() with various coordinates
- [ ] Unit tests for check_waypoint_arrival() boundary conditions
- [ ] Unit tests for advance_to_next_waypoint() loop/non-loop behavior
- [ ] Unit tests for state machine transitions
- [ ] Manual testing with 10+ simultaneous autopilot vessels
- [ ] Valgrind-clean execution of all tests

### Quality Gates
- [ ] All files ASCII-encoded (0-127 characters only)
- [ ] Unix LF line endings
- [ ] ANSI C90 compliant (no // comments, no declarations after statements)
- [ ] NULL checks on all pointer dereferences
- [ ] No memory leaks (Valgrind verified)
- [ ] Compiles clean with -Wall -Wextra

---

## 8. Implementation Notes

### Key Considerations
- Use existing vessel_list from vessels.c to iterate all vessels
- AUTOPILOT_TICK_INTERVAL is 5 pulses (~0.5 seconds at 10 pulses/sec)
- Ship speed is in greyhawk_ship_data.current_speed
- Waypoint tolerance is in struct waypoint.tolerance field
- Must handle NULL autopilot pointer on vessels without autopilot

### Potential Challenges
- **Game loop integration**: Must not block or slow heartbeat; O(n) iteration acceptable for 100-500 vessels
- **Terrain validation**: Use find_available_wilderness_room() before movement to avoid silent failures
- **Wait time tracking**: Decrement per-second, not per-tick; use time(0) or tick count conversion
- **Route pointer validity**: Verify current_route pointer is valid before accessing waypoints

### Relevant Considerations
- [P00] **Vessel movement fails silently**: MUST use find_available_wilderness_room() + assign_wilderness_room() pattern. If destination has no valid wilderness room, log warning and skip movement.
- [P00] **Don't use C99/C11 features**: ANSI C90/C89 only - use /* */ comments, declare all variables at block start
- [P00] **Always NULL check pointers**: Critical before dereferencing ship->autopilot, autopilot->current_route, etc.

### ASCII Reminder
All output files must use ASCII-only characters (0-127).

---

## 9. Testing Strategy

### Unit Tests
- `test_calculate_distance_zero()` - Same coordinates returns 0
- `test_calculate_distance_simple()` - Known coordinates return expected distance
- `test_calculate_heading_cardinal()` - Cardinal direction headings
- `test_check_arrival_within_tolerance()` - Returns TRUE when close enough
- `test_check_arrival_outside_tolerance()` - Returns FALSE when too far
- `test_advance_waypoint_middle()` - Advances to next waypoint in route
- `test_advance_waypoint_last_loop()` - Loop route returns to index 0
- `test_advance_waypoint_last_noloop()` - Non-loop route sets COMPLETE state
- `test_wait_time_decrement()` - Wait timer decrements correctly
- `test_state_transitions()` - TRAVELING -> WAITING -> TRAVELING -> COMPLETE

### Integration Tests
- Test autopilot_tick() with mock vessel list
- Test full route traversal from start to completion

### Manual Testing
- Create test route with 3-5 waypoints in Zone 213
- Assign route to test vessel and enable autopilot
- Observe vessel moving between waypoints
- Verify wait times are respected
- Verify loop/non-loop completion behavior

### Edge Cases
- Empty route (0 waypoints) - Should not crash, state stays OFF
- Single waypoint route - Should reach and complete/loop immediately
- Route with all waypoints at same location - Should advance rapidly
- Waypoint in impassable terrain - Should skip movement, log warning
- Very large tolerance (> route span) - Should immediately "arrive"
- Very small tolerance (< 1 unit) - Should require precise arrival

---

## 10. Dependencies

### External Libraries
- MySQL/MariaDB client library (libmariadb-dev)
- Standard C math library (libm) for sqrt()

### Other Sessions
- **Depends on**: phase01-session01-autopilot-data-structures, phase01-session02-waypoint-route-management
- **Depended by**: phase01-session04-autopilot-player-commands, phase01-session05-npc-pilot-integration

---

## Next Steps

Run `/tasks` to generate the implementation task checklist.
