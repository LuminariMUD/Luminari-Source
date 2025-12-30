# Session Specification

**Session ID**: `phase02-session05-vehicle-in-vehicle-mechanics`
**Phase**: 02 - Simple Vehicle Support
**Status**: Not Started
**Created**: 2025-12-30

---

## 1. Session Overview

This session implements the mechanics for loading land vehicles onto water vessels, enabling key gameplay scenarios such as wagons loaded onto ferries, carriages transported on cargo ships, and mounts stored in vessel holds. This feature bridges the land-based vehicle system (sessions 01-04) with the existing vessel/ship system from Phase 00.

The implementation follows established patterns from `vessels_docking.c` for boarding mechanics and reuses the existing capacity/weight infrastructure. When a vehicle is loaded onto a vessel, it becomes "cargo" that travels with the parent vessel - its wilderness coordinates update automatically when the vessel moves. This nested transport relationship requires careful state management to handle edge cases like unloading in invalid terrain or managing the relationship when a vessel is destroyed.

Vehicle-in-vehicle mechanics are a prerequisite for the unified command interface (session 06), as that session will need to understand when a character is on a vehicle that is itself loaded onto a vessel. This session focuses on the core loading/unloading mechanics and does not implement multi-level nesting or vehicle-on-vehicle loading.

---

## 2. Objectives

1. Implement vehicle loading/unloading functions that integrate with the vessel cargo system
2. Add capacity and weight validation to prevent overloading vessels with vehicles
3. Create player commands (`loadvehicle`, `unloadvehicle`) for interacting with vehicle transport
4. Ensure loaded vehicles persist across server restarts via MySQL

---

## 3. Prerequisites

### Required Sessions
- [x] `phase02-session04-vehicle-player-commands` - Player command infrastructure (mount, dismount, drive)
- [x] `phase02-session03-vehicle-movement-system` - Vehicle movement and terrain handling
- [x] `phase02-session02-vehicle-creation-system` - Vehicle lifecycle and persistence
- [x] `phase02-session01-vehicle-data-structures` - Core vehicle_data struct with capacity fields
- [x] `phase00-session*` - Complete vessel system with docking mechanics

### Required Tools/Knowledge
- Understanding of `struct vehicle_data` (src/vessels.h:1117)
- Understanding of `struct greyhawk_ship_data` for vessel cargo
- Familiarity with vessels_docking.c boarding patterns
- MySQL table structure for vehicle_data persistence

### Environment Requirements
- MySQL/MariaDB database running with vessel tables
- Test zone (zone 213) available for manual testing

---

## 4. Scope

### In Scope (MVP)
- `load_vehicle_onto_vessel()` core loading function
- `unload_vehicle_from_vessel()` core unloading function
- `check_vessel_vehicle_capacity()` weight/slot validation
- Vehicle state tracking (new VSTATE_LOADED state or parent_vessel_id field)
- `do_loadvehicle` command handler for players
- `do_unloadvehicle` command handler for players
- Automatic coordinate updates when parent vessel moves
- MySQL persistence for loaded vehicle state
- Unit tests for loading/unloading logic

### Out of Scope (Deferred)
- Loading vehicles onto other vehicles - *Reason: Keep first iteration simple; vehicles only load onto vessels*
- Multi-level nesting - *Reason: Complexity; vehicle cannot be on a vehicle that is on a vessel*
- Automated loading/unloading - *Reason: Future automation session*
- Loading while vessel is moving - *Reason: Requires docking check; vessel must be stationary or docked*

---

## 5. Technical Approach

### Architecture
The vehicle-in-vessel relationship uses a parent tracking field on `struct vehicle_data`:
- Add `int parent_vessel_id` field to track which vessel (if any) the vehicle is loaded onto
- When `parent_vessel_id > 0`, the vehicle is "loaded" and inherits location from parent
- The vessel tracks total cargo weight including loaded vehicles

Loading flow:
1. Player uses `loadvehicle <vehicle>` while standing in a vessel docking room
2. System validates: vessel docked/stationary, vehicle in same location, capacity available
3. Vehicle state changes, parent_vessel_id set, vehicle weight added to vessel cargo
4. Vehicle is "removed" from the world room (location = NOWHERE or vessel interior VNUM)

Unloading flow:
1. Player uses `unloadvehicle <vehicle>` while on vessel at valid terrain
2. System validates: terrain suitable for vehicle type, player has access
3. Vehicle location set to vessel's current wilderness room, parent_vessel_id cleared
4. Vehicle weight removed from vessel cargo

### Design Patterns
- **Capacity Validation Pattern**: Reuse `vehicle_can_add_weight()` style from vehicles.c
- **State Machine Pattern**: Extend vehicle states or use parent_vessel_id as state indicator
- **Observer Pattern**: Hook into vessel movement to update loaded vehicle coordinates

### Technology Stack
- ANSI C90/C89 (no C99 features)
- MySQL for persistence (REPLACE INTO pattern)
- CuTest for unit testing

---

## 6. Deliverables

### Files to Create
| File | Purpose | Est. Lines |
|------|---------|------------|
| `src/vehicles_transport.c` | Core vehicle-in-vessel logic | ~400 |
| `unittests/CuTest/vehicle_transport_tests.c` | Unit tests for transport functions | ~250 |

### Files to Modify
| File | Changes | Est. Lines Changed |
|------|---------|------------|
| `src/vessels.h` | Add parent_vessel_id to vehicle_data, function prototypes | ~20 |
| `src/vehicles.c` | Update save/load for parent_vessel_id, coordinate sync hook | ~40 |
| `src/vehicles_commands.c` | Add do_loadvehicle, do_unloadvehicle commands | ~150 |
| `src/interpreter.c` | Register loadvehicle and unloadvehicle commands | ~4 |
| `src/CMakeLists.txt` | Add vehicles_transport.c to build | ~2 |
| `unittests/CuTest/Makefile` | Add transport test compilation | ~5 |

---

## 7. Success Criteria

### Functional Requirements
- [ ] Vehicles can be loaded onto docked/stationary vessels
- [ ] Vehicles can be unloaded at valid terrain locations
- [ ] Weight/capacity limits enforced (vessel cargo capacity)
- [ ] Loaded vehicles move with parent vessel (coordinates update)
- [ ] Players can view loaded vehicles on a vessel
- [ ] State persists across server restarts
- [ ] Clear error messages for invalid operations

### Testing Requirements
- [ ] Unit tests written for all transport functions (12+ tests)
- [ ] Unit tests pass with zero failures
- [ ] Valgrind clean (no memory leaks in new code)
- [ ] Manual testing completed (load, unload, vessel movement)

### Quality Gates
- [ ] All files ASCII-encoded (0-127 characters only)
- [ ] Unix LF line endings
- [ ] Code follows ANSI C90 conventions (/* */ comments, no C99 features)
- [ ] Two-space indentation, Allman braces
- [ ] All functions have NULL checks on pointer parameters
- [ ] No compiler warnings with -Wall -Wextra

---

## 8. Implementation Notes

### Key Considerations
- Vessel must be docked or speed=0 to allow loading/unloading
- Vehicle terrain_flags must be compatible with unload location
- When vessel is destroyed, loaded vehicles should be unloaded to current location
- Player must dismount vehicle before loading it onto vessel
- Only one level of nesting (vehicle -> vessel, not vehicle -> vehicle -> vessel)

### Potential Challenges
- **Coordinate synchronization**: When vessel moves, all loaded vehicles need coordinate updates. Solution: Hook into vessel movement function or update lazily on vehicle access.
- **Vessel destruction handling**: If vessel sinks/is destroyed, loaded vehicles need graceful handling. Solution: Unload to vessel's last valid location or destroy with salvage chance.
- **Terrain validation on unload**: Vehicle might not be able to traverse vessel's current location (e.g., deep water). Solution: Check terrain compatibility before allowing unload.
- **Memory management**: Vehicle struct modification requires careful update of save/load. Solution: Follow existing vehicle_save/vehicle_load patterns exactly.

### Relevant Considerations
- **[P01] Standalone unit test files**: Tests will be self-contained without server dependencies, using mock vessel/vehicle data structures
- **[P01] Memory-efficient structures**: Adding parent_vessel_id is only 4 bytes, maintaining sub-512 byte target
- **[P00] Auto-create DB tables at startup**: Will add parent_vessel_id column to vehicle_data table schema
- **Avoid C99/C11 features**: All code uses ANSI C90 only
- **Don't skip NULL checks**: All functions will validate pointers before use

### ASCII Reminder
All output files must use ASCII-only characters (0-127).

---

## 9. Testing Strategy

### Unit Tests
- `test_load_vehicle_success` - Basic loading onto vessel
- `test_load_vehicle_capacity_exceeded` - Weight limit enforcement
- `test_load_vehicle_not_docked` - Reject loading on moving vessel
- `test_load_vehicle_already_loaded` - Reject double loading
- `test_unload_vehicle_success` - Basic unloading
- `test_unload_vehicle_invalid_terrain` - Reject unload to water for cart
- `test_unload_vehicle_not_loaded` - Reject unload when not on vessel
- `test_vehicle_coordinate_sync` - Coordinates match parent vessel
- `test_check_capacity_edge_cases` - Boundary weight conditions
- `test_persistence_loaded_state` - Save/load preserves parent_vessel_id

### Integration Tests
- Load vehicle, move vessel, verify vehicle coordinates updated
- Save loaded vehicle, reload server, verify relationship intact
- Multiple vehicles on one vessel respects total capacity

### Manual Testing
- Create vehicle in zone 213, dock vessel, load vehicle, move vessel, unload vehicle
- Attempt invalid operations (load on moving vessel, unload in water)
- Verify `vehiclestatus` shows loaded state correctly

### Edge Cases
- Loading vehicle that is at max_weight itself
- Unloading when vessel is at wilderness edge (coordinate boundary)
- Loading when character is mounted on the vehicle (should fail)
- Vessel with multiple loaded vehicles - list all, unload specific one

---

## 10. Dependencies

### External Libraries
- MySQL/MariaDB client library (existing dependency)
- CuTest framework (existing in unittests/CuTest)

### Other Sessions
- **Depends on**: phase02-session01 through phase02-session04 (all complete)
- **Depended by**: phase02-session06-unified-command-interface (needs to detect loaded state)

---

## Next Steps

Run `/tasks` to generate the implementation task checklist.
