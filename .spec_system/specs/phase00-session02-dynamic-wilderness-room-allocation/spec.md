# Session Specification

**Session ID**: `phase00-session02-dynamic-wilderness-room-allocation`
**Phase**: 00 - Core Vessel System
**Status**: Not Started
**Created**: 2025-12-29

---

## 1. Session Overview

This session addresses the **#1 Critical Path Item** from the Vessel System PRD: vessels fail silently when attempting to move to wilderness coordinates that lack pre-allocated rooms. Currently, vessel movement only works if the destination coordinate happens to have an existing room, which severely limits navigation across the 2048x2048 wilderness grid.

The fix follows the proven pattern already implemented in `src/movement.c` for character movement: using `find_available_wilderness_room()` to obtain a room from the pool, then `assign_wilderness_room()` to configure it for the destination coordinates. This pattern dynamically allocates rooms on-demand, enabling vessels to navigate to any valid wilderness coordinate.

Successfully completing this session unlocks all subsequent vessel features. Sessions 03-09 depend on working vessel movement, making this a blocking prerequisite. Without dynamic room allocation, vessel navigation is non-functional for practical use.

---

## 2. Objectives

1. Implement dynamic wilderness room allocation in vessel movement functions using the `find_available_wilderness_room()` + `assign_wilderness_room()` pattern
2. Enable vessel navigation to any valid wilderness coordinate (-1024 to +1024 on X/Y axes)
3. Replace silent failures with appropriate error messages when room allocation fails
4. Ensure proper room lifecycle management (rooms released when vessels move away)

---

## 3. Prerequisites

### Required Sessions
- [x] `phase00-session01-header-cleanup-foundation` - Clean codebase with consolidated vessel headers

### Required Tools/Knowledge
- Understanding of wilderness coordinate system (X, Y, Z navigation)
- Familiarity with room allocation pattern in movement.c
- Knowledge of room pool management in wilderness.c

### Environment Requirements
- MariaDB/MySQL database running
- Wilderness system operational
- Build environment configured (CMake or autotools)

---

## 4. Scope

### In Scope (MVP)
- Study and document the room allocation pattern from src/movement.c
- Implement `find_available_wilderness_room()` + `assign_wilderness_room()` pattern in vessel movement
- Add error handling for room pool exhaustion
- Add error handling for invalid coordinates
- Verify rooms are released when vessels move to new locations
- Test movement across various wilderness coordinates
- Ensure terrain restriction validation still applies

### Out of Scope (Deferred)
- Performance optimization of room allocation - *Reason: premature optimization*
- Room pooling enhancements - *Reason: current pool is sufficient for MVP*
- Multi-vessel coordination at same coordinate - *Reason: edge case for later session*
- Pathfinding improvements - *Reason: separate feature*

---

## 5. Technical Approach

### Architecture

The implementation follows a direct integration approach:

1. **Identify vessel movement entry points** - Find functions that attempt to move vessels to new coordinates
2. **Add room allocation logic** - Before moving a vessel, check if destination room exists; if not, allocate from pool
3. **Handle allocation failures** - Return meaningful errors instead of silent failures
4. **Ensure room release** - When vessel vacates a coordinate, the room should be available for reuse

### Design Patterns
- **Pool Pattern**: Wilderness rooms are pre-allocated and assigned/released as needed
- **Null Object Pattern**: Using `NOWHERE` constant to indicate invalid/unavailable rooms
- **Guard Clause Pattern**: Early return on NULL checks and allocation failures

### Technology Stack
- ANSI C90/C89 (no C99/C11 features)
- Wilderness coordinate system APIs
- Existing room pool infrastructure

---

## 6. Deliverables

### Files to Create
| File | Purpose | Est. Lines |
|------|---------|------------|
| None | All changes are modifications to existing files | - |

### Files to Modify
| File | Changes | Est. Lines |
|------|---------|------------|
| `src/vessels_src.c` | Add room allocation to `vessel_movement_tick()` and related movement functions | ~50-80 |
| `src/vessels_src.c` | Add helper function for vessel-specific room allocation | ~30-40 |
| `src/vessels.c` | Update any direct movement calls to use new allocation | ~20-30 |

---

## 7. Success Criteria

### Functional Requirements
- [ ] Vessels can move to wilderness coordinates that have no pre-existing room
- [ ] Movement to coordinates within valid range (-1024 to +1024) succeeds with room allocation
- [ ] Movement to coordinates outside valid range fails with appropriate error message
- [ ] Room pool exhaustion produces clear error message, not silent failure
- [ ] Vessels can traverse long distances across the wilderness grid
- [ ] Terrain restrictions are still enforced during movement

### Testing Requirements
- [ ] Manual testing: sail vessel to 5+ different wilderness coordinates without pre-existing rooms
- [ ] Manual testing: verify error message when attempting invalid coordinate
- [ ] Valgrind check for memory leaks after extended vessel movement

### Quality Gates
- [ ] All files ASCII-encoded (0-127 characters only)
- [ ] Unix LF line endings
- [ ] Code follows C90 conventions (no // comments, declarations at block start)
- [ ] NULL checks on all pointer operations
- [ ] No compiler warnings with -Wall -Wextra
- [ ] Code compiles cleanly with both CMake and autotools

---

## 8. Implementation Notes

### Key Considerations
- The pattern in movement.c (lines 203-216) is the reference implementation
- `find_available_wilderness_room()` returns `NOWHERE` if pool is exhausted
- `assign_wilderness_room()` configures room description, coordinates, and sector type
- Must understand when rooms are released back to pool (check wilderness.c for deallocation)

### Potential Challenges
- **Room lifecycle understanding**: Need to trace when rooms return to pool; may need to explicitly release rooms when vessel leaves
- **Terrain validation integration**: Must ensure terrain checks happen before room allocation, not after
- **Multiple movement paths**: `vessel_movement_tick()` handles automatic movement, but manual commands may have different paths

### Relevant Considerations
- [P00] **Vessel movement fails silently**: This is the exact issue this session addresses; the fix is dynamic room allocation
- [P00] **Don't skip NULL checks**: Critical when handling `find_available_wilderness_room()` return value; always check for `NOWHERE`
- [P00] **Wilderness system required**: Must include wilderness.h and ensure wilderness APIs are available
- [P00] **Don't use C99/C11 features**: All new code must use `/* */` comments, declarations at block start

### ASCII Reminder
All output files must use ASCII-only characters (0-127).

---

## 9. Testing Strategy

### Unit Tests
- Test room allocation function with valid coordinates
- Test room allocation function with invalid coordinates
- Test handling of NOWHERE return value

### Integration Tests
- Vessel moves from pre-existing room to coordinate without room
- Vessel moves between two coordinates that both require allocation
- Vessel moves back to previously visited coordinate

### Manual Testing
- Create test vessel at known wilderness location
- Issue sail command to distant coordinate (e.g., 500,500)
- Verify vessel arrives at new location
- Issue sail command to another distant coordinate
- Verify error message when pool is exhausted (if testable)
- Verify terrain restrictions still apply (e.g., landlocked vessel cannot sail over mountains)

### Edge Cases
- Room pool exhaustion (all wilderness rooms in use)
- Boundary coordinates (-1024, -1024) and (1024, 1024)
- Movement to current location (should be no-op or appropriate message)
- Rapid sequential movements

---

## 10. Dependencies

### External Libraries
- MySQL/MariaDB client library (existing dependency)
- Standard C library

### Other Sessions
- **Depends on**: `phase00-session01-header-cleanup-foundation` (completed)
- **Depended by**:
  - `phase00-session03-vessel-type-system` (needs working movement)
  - `phase00-session05-interior-room-generation-wiring`
  - `phase00-session06-interior-movement-implementation`
  - All subsequent sessions

---

## Next Steps

Run `/tasks` to generate the implementation task checklist.
