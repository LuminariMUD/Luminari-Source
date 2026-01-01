# Session Specification

**Session ID**: `phase03-session04-dynamic-wilderness-rooms`
**Phase**: 03 - Optimization & Polish
**Status**: Not Started
**Created**: 2025-12-30

---

## 1. Session Overview

This session addresses the #1 Critical Path Item from the PRD: vessel movement fails silently when the destination wilderness coordinate has no pre-allocated room. While the core functions `find_available_wilderness_room()` and `assign_wilderness_room()` exist in `wilderness.c`, and a wrapper `get_or_allocate_wilderness_room()` exists in `vessels.c`, there is incomplete integration in the autopilot movement path.

Specifically, `move_vessel_toward_waypoint()` in `vessels_autopilot.c` allocates wilderness rooms but does not update the ship's location field or move the ship object. Additionally, there is no mechanism to release rooms when vessels depart a coordinate, which could exhaust the dynamic room pool over time. This session will complete the integration, add room release logic, handle multi-vessel scenarios, and validate with comprehensive unit tests.

Upon completion, vessels will be able to navigate freely to any valid wilderness coordinate without requiring pre-allocated rooms, and the system will properly manage room lifecycle.

---

## 2. Objectives

1. Fix autopilot movement to properly update ship location using `update_ship_wilderness_position()`
2. Implement room release mechanism when vessels depart coordinates (coordinate ROOM_OCCUPIED cleanup)
3. Ensure multiple vessels can safely occupy the same wilderness coordinate
4. Add comprehensive unit tests validating dynamic room allocation during movement (target: 12+ tests)

---

## 3. Prerequisites

### Required Sessions
- [x] `phase03-session03-command-registration-wiring` - Commands registered in interpreter.c

### Required Tools/Knowledge
- CuTest unit testing framework
- Valgrind for memory validation
- Understanding of wilderness room pool system (WILD_DYNAMIC_ROOM_VNUM_START to WILD_DYNAMIC_ROOM_VNUM_END)

### Environment Requirements
- Wilderness coordinate system operational (-1024 to +1024 range)
- `find_available_wilderness_room()` exists in wilderness.c (verified: line 839)
- `assign_wilderness_room()` exists in wilderness.c (verified: line 874)
- `event_check_occupied()` exists for room recycling (verified: line 1441)

---

## 4. Scope

### In Scope (MVP)
- Fix `move_vessel_toward_waypoint()` to call `update_ship_wilderness_position()` after movement
- Implement vessel departure tracking to trigger ROOM_OCCUPIED cleanup
- Validate multi-vessel coordinate handling (ships don't conflict at same coordinate)
- Add unit tests for dynamic allocation during movement
- Add unit tests for room release on departure
- Performance validation (<10ms per movement operation)

### Out of Scope (Deferred)
- Changes to wilderness.c core system - *Reason: Existing functions work correctly*
- New terrain types - *Reason: Not part of room allocation*
- Weather system modifications - *Reason: Independent subsystem*
- Z-axis (altitude/depth) room allocation - *Reason: Uses same X/Y allocation pattern*

---

## 5. Technical Approach

### Architecture

The wilderness system uses a pool of dynamic rooms (WILD_DYNAMIC_ROOM_VNUM_START to WILD_DYNAMIC_ROOM_VNUM_END). When a player or vessel moves to a wilderness coordinate:

1. `find_room_by_coordinates(x, y)` checks if a room exists at those coordinates
2. If NOWHERE, `find_available_wilderness_room()` gets an unoccupied room from the pool
3. `assign_wilderness_room(room, x, y)` configures the room for those coordinates and sets ROOM_OCCUPIED
4. The `event_check_occupied()` event clears ROOM_OCCUPIED when the room is empty (no people, no objects, no effects)

**Current Gap**: `move_vessel_toward_waypoint()` allocates rooms but:
- Does not call `update_ship_wilderness_position()` to update `ship->location`
- Does not move the ship object to the new room
- Vessel objects in rooms may prevent `event_check_occupied()` from releasing rooms

**Solution**:
1. Replace inline allocation in `move_vessel_toward_waypoint()` with call to existing `update_ship_wilderness_position()` from vessels.c
2. Ensure ship objects are properly moved, allowing old rooms to become empty
3. Validate that `event_check_occupied()` properly recycles rooms once vessels depart

### Design Patterns
- **Dynamic Pool Allocation**: Use existing wilderness room pool pattern
- **Event-Based Cleanup**: Leverage existing `event_check_occupied()` for automatic room release
- **Centralized Position Update**: Use single function (`update_ship_wilderness_position()`) for all vessel movement

### Technology Stack
- ANSI C90/C89 with GNU extensions
- CuTest unit testing framework
- Valgrind for memory leak detection

---

## 6. Deliverables

### Files to Create
| File | Purpose | Est. Lines |
|------|---------|------------|
| `unittests/CuTest/test_vessel_wilderness_rooms.c` | Unit tests for dynamic room allocation | ~300 |

### Files to Modify
| File | Changes | Est. Lines |
|------|---------|------------|
| `src/vessels_autopilot.c` | Fix move_vessel_toward_waypoint() to use update_ship_wilderness_position() | ~20 |
| `src/vessels.c` | Add departure tracking/notification for room cleanup | ~30 |
| `src/vessels.h` | Add prototype for any new helper functions | ~5 |
| `unittests/CuTest/Makefile` | Add new test file to build | ~3 |
| `unittests/CuTest/AllTests.c` | Register new test suite | ~5 |

---

## 7. Success Criteria

### Functional Requirements
- [ ] Vessels can move to any valid wilderness coordinate
- [ ] No silent failures on movement (proper error messages logged)
- [ ] Rooms properly released when vessels leave (ROOM_OCCUPIED cleared)
- [ ] Multiple vessels can occupy same coordinate simultaneously
- [ ] Ship objects properly moved between rooms during navigation

### Testing Requirements
- [ ] 12+ unit tests written and passing
- [ ] All unit tests Valgrind clean
- [ ] Manual testing with autopilot navigation completed

### Quality Gates
- [ ] All files ASCII-encoded (0-127 characters only)
- [ ] Unix LF line endings
- [ ] Code follows C90/C89 conventions (no // comments, no VLAs)
- [ ] Two-space indentation, Allman-style braces
- [ ] Performance: <10ms for movement operation

---

## 8. Implementation Notes

### Key Considerations
- The `update_ship_wilderness_position()` function already exists and correctly uses `get_or_allocate_wilderness_room()`
- Ship objects are moved via `obj_from_room()` / `obj_to_room()` in `update_ship_wilderness_position()`
- The `event_check_occupied()` event runs every 10 seconds and clears ROOM_OCCUPIED when room is empty
- Multiple ships at same coordinate: Each ship object will be in the same room, which is valid

### Potential Challenges
- **Room pool exhaustion**: If ships are constantly moving without old rooms being released
  - *Mitigation*: Ensure ship objects are properly moved so old rooms become empty
- **Concurrent vessel access**: Multiple ships moving to same coordinate simultaneously
  - *Mitigation*: The allocation is sequential (single-threaded game loop), no race conditions
- **Performance impact**: Dynamic allocation during rapid movement
  - *Mitigation*: Benchmark with 100+ vessels; allocation is O(n) pool scan, typically fast

### Relevant Considerations
- [P03] **Vessel movement fails silently**: This session directly resolves this Architecture concern by completing the dynamic room allocation integration
- [P03] **Wilderness system required**: Movement depends on wilderness coordinate system being operational (verified functional)
- [P02] **Standalone unit test files**: Continue pattern of self-contained tests without server dependencies
- [P02] **POSIX macro for C89 compatibility**: Use _POSIX_C_SOURCE 200809L for snprintf/strcasecmp in tests

### ASCII Reminder
All output files must use ASCII-only characters (0-127).

---

## 9. Testing Strategy

### Unit Tests
- Test `get_or_allocate_wilderness_room()` allocates new room when none exists
- Test `get_or_allocate_wilderness_room()` returns existing room when already allocated
- Test `update_ship_wilderness_position()` updates ship location correctly
- Test ship object is moved to new room after position update
- Test room ROOM_OCCUPIED flag is set after allocation
- Test room becomes available after ship departs
- Test multiple ships can be at same coordinate
- Test coordinate boundary validation (-1024 to +1024)
- Test room pool exhaustion handling (graceful failure)
- Test `move_vessel_toward_waypoint()` updates position correctly
- Test terrain validation during movement
- Test speed calculation affects movement distance

### Integration Tests
- Autopilot navigation through multiple waypoints
- Vessel round-trip (depart and return to same coordinate)

### Manual Testing
- Start vessel at one coordinate, navigate to another, verify arrival
- Check that departure coordinate room is eventually released
- Navigate multiple vessels to same destination
- Verify `shipstatus` shows correct coordinates after movement

### Edge Cases
- Movement to coordinate (-1024, -1024) and (1024, 1024) (boundaries)
- Movement when room pool is nearly exhausted
- Rapid movement (speed 10) across multiple coordinates
- Movement to coordinate that already has another vessel

---

## 10. Dependencies

### External Libraries
- CuTest: Unit testing framework (unittests/CuTest/)
- Valgrind: Memory leak detection

### Other Sessions
- **Depends on**: `phase03-session03-command-registration-wiring` (complete)
- **Depended by**: `phase03-session05-vessel-type-system`, `phase03-session06-final-testing-documentation`

---

## Next Steps

Run `/tasks` to generate the implementation task checklist.
