# Session Specification

**Session ID**: `phase02-session03-vehicle-movement-system`
**Phase**: 02 - Simple Vehicle Support
**Status**: Not Started
**Created**: 2025-12-30

---

## 1. Session Overview

This session implements the vehicle movement system, enabling land-based vehicles (carts, wagons, mounts, carriages) to traverse the wilderness coordinate system. Building on Sessions 01-02 which established vehicle data structures and creation/persistence systems, this session adds the core movement functionality that makes vehicles actually functional as transport.

The movement system mirrors the proven vessel movement pattern from Phase 00/01, adapting it for land-based terrain restrictions. Where vessels traverse water and airways, vehicles navigate roads, plains, forests, and other land terrain types. The implementation uses the dynamic wilderness room allocation pattern (find_available_wilderness_room + assign_wilderness_room) established in Phase 00 Session 02.

This session is critical for unlocking Session 04's player commands, as the `drive` command requires movement functionality to operate. Without this session, vehicles exist only as static objects without transport capability.

---

## 2. Objectives

1. Implement `move_vehicle()` core movement function with 8-direction navigation
2. Create `check_vehicle_terrain()` to validate terrain traversability for land vehicles
3. Implement `get_vehicle_speed_modifier()` for terrain-based speed calculation (road bonus, rough terrain penalty)
4. Integrate vehicle position updates with database persistence

---

## 3. Prerequisites

### Required Sessions
- [x] `phase02-session01-vehicle-data-structures` - Vehicle struct, terrain flags, type definitions
- [x] `phase02-session02-vehicle-creation-system` - Vehicle lifecycle, state management, persistence
- [x] `phase00-session02-dynamic-wilderness-room-allocation` - Room allocation patterns

### Required Tools/Knowledge
- Understanding of wilderness coordinate system (-1024 to +1024 X/Y)
- Familiarity with VTERRAIN_* flags in vessels.h
- Knowledge of SECT_* sector types in structs.h

### Environment Requirements
- MySQL/MariaDB running (vehicle_data table)
- Wilderness system operational

---

## 4. Scope

### In Scope (MVP)
- `move_vehicle()` core movement function
- 8-direction navigation (N, NE, E, SE, S, SW, W, NW)
- `check_vehicle_terrain()` terrain validation using VTERRAIN_* flags
- `get_vehicle_speed_modifier()` terrain speed modifiers
- Dynamic wilderness room allocation for destinations
- Vehicle position tracking (X/Y coordinates)
- Database persistence of position updates
- Integration with existing vehicle state machine (VSTATE_IDLE to VSTATE_MOVING)

### Out of Scope (Deferred)
- Autopilot/waypoint navigation - *Reason: Future phase feature*
- Combat during movement - *Reason: Complex system requiring separate session*
- Weather effects on vehicles - *Reason: Future enhancement*
- Mount stamina/fatigue system - *Reason: Complexity, future session*
- Player commands (drive, steer) - *Reason: Session 04 scope*

---

## 5. Technical Approach

### Architecture
The movement system follows a layered approach:
1. **Direction Layer**: Convert direction input (0-7) to X/Y coordinate deltas
2. **Validation Layer**: Check terrain traversability using vehicle terrain flags
3. **Speed Layer**: Calculate effective speed based on terrain type
4. **Execution Layer**: Allocate destination room, update position, persist to DB

### Design Patterns
- **Strategy Pattern**: Terrain validation per vehicle type using terrain_flags bitfield
- **Template Method**: Follow vessel movement pattern (can_vessel_traverse_terrain as template)
- **Coordinate Mapping**: Use existing direction_to_delta() pattern from movement.c

### Technology Stack
- ANSI C90/C89 with GNU extensions
- MySQL/MariaDB for persistence
- Wilderness coordinate system (wilderness.c/h)
- Dynamic room allocation (find_available_wilderness_room/assign_wilderness_room)

---

## 6. Deliverables

### Files to Create
| File | Purpose | Est. Lines |
|------|---------|------------|
| None | All code added to existing vehicles.c | - |

### Files to Modify
| File | Changes | Est. Lines |
|------|---------|------------|
| `src/vehicles.c` | Add movement functions, terrain checking, speed modifiers | ~250 |
| `src/vessels.h` | Add vehicle movement function declarations | ~15 |
| `unittests/CuTest/test_vehicle_movement.c` | Unit tests for movement system | ~200 |
| `unittests/CuTest/Makefile` | Add test_vehicle_movement to build | ~5 |

---

## 7. Success Criteria

### Functional Requirements
- [ ] Vehicles move across wilderness grid in 8 directions
- [ ] Land vehicles blocked by water terrain (SECT_WATER_SWIM, SECT_WATER_NOSWIM, SECT_OCEAN)
- [ ] Road terrain (SECT_ROAD_NS, SECT_ROAD_EW, SECT_ROAD_INT) provides speed bonus
- [ ] Rough terrain (SECT_MOUNTAIN, SECT_HIGH_MOUNTAIN, SECT_SWAMP) applies speed penalty
- [ ] Position updates correctly in database after movement
- [ ] No crashes on invalid movement attempts (NULL vehicle, out of bounds, impassable terrain)
- [ ] Movement uses find_available_wilderness_room() + assign_wilderness_room() pattern

### Testing Requirements
- [ ] Unit tests written and passing for all movement functions
- [ ] Manual testing completed (create vehicle, move in each direction)
- [ ] Valgrind clean (no memory leaks)

### Quality Gates
- [ ] All files ASCII-encoded
- [ ] Unix LF line endings
- [ ] Code follows project conventions (C90, two-space indent, Allman braces)
- [ ] No compiler warnings with -Wall -Wextra

---

## 8. Implementation Notes

### Key Considerations
- Vehicle terrain flags (VTERRAIN_*) differ from vessel terrain capabilities
- Carts/wagons restricted to roads and plains; mounts traverse more terrain
- Speed modifiers should be multiplicative (base_speed * terrain_modifier)
- State machine transition: IDLE/LOADED -> MOVING during movement, back after arrival

### Potential Challenges
- **Terrain mapping**: Mapping SECT_* to VTERRAIN_* requires careful switch statement
- **Room allocation exhaustion**: Must handle NOWHERE return gracefully
- **Coordinate bounds**: Wilderness is -1024 to +1024; must validate before movement
- **Concurrent access**: Multiple vehicles at same coordinates is valid (unlike rooms)

### Relevant Considerations
- [P00] **Dynamic wilderness room allocation**: Vessel movement originally failed silently when destination had no allocated room. Fixed using find_available_wilderness_room() + assign_wilderness_room() pattern - apply same approach to vehicles.
- [P01] **Memory-efficient structs**: Autopilot achieved 48 bytes per vessel; vehicle movement state should remain within existing struct size.
- [P01] **Standalone unit tests**: Create self-contained tests without server dependencies.

### ASCII Reminder
All output files must use ASCII-only characters (0-127).

---

## 9. Testing Strategy

### Unit Tests
- Test `check_vehicle_terrain()` with each vehicle type and terrain combination
- Test `get_vehicle_speed_modifier()` returns correct values for road/plains/rough terrain
- Test `move_vehicle()` with valid movement, invalid terrain, out-of-bounds
- Test direction-to-delta conversion for all 8 directions
- Test NULL handling in all functions

### Integration Tests
- Create vehicle, move multiple times, verify position updates in DB
- Move vehicle across terrain boundary (road to plains to forest)
- Verify state transitions (IDLE -> MOVING -> back to IDLE/LOADED)

### Manual Testing
- Create a cart via admin command, move in each direction
- Attempt to move cart into water (should be blocked)
- Move mount into forest (should succeed)
- Verify database has updated coordinates

### Edge Cases
- Movement at wilderness boundary (-1024, +1024)
- Movement when room pool is exhausted (NOWHERE returned)
- Movement of damaged vehicle (should be blocked - VSTATE_DAMAGED)
- Movement of hitched vehicle (should be blocked - VSTATE_HITCHED)
- Rapid successive movements

---

## 10. Dependencies

### External Libraries
- MySQL/MariaDB client library (libmariadb-dev)

### Other Sessions
- **Depends on**: phase02-session01, phase02-session02, phase00-session02
- **Depended by**: phase02-session04-vehicle-player-commands, phase02-session05-vehicle-in-vehicle-mechanics

---

## Next Steps

Run `/tasks` to generate the implementation task checklist.
