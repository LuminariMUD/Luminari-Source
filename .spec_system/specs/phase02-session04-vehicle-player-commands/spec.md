# Session Specification

**Session ID**: `phase02-session04-vehicle-player-commands`
**Phase**: 02 - Simple Vehicle Support
**Status**: Not Started
**Created**: 2025-12-30

---

## 1. Session Overview

This session implements player-facing commands for interacting with the vehicle system established in Sessions 01-03. While the vehicle data structures, creation system, and movement mechanics are fully functional internally, players currently have no way to interact with vehicles. This session bridges that gap by exposing the vehicle system through intuitive player commands.

The session delivers four core commands: `mount` (enter a vehicle), `dismount` (exit a vehicle), `drive` (move a vehicle in a direction), and `vstatus` (display vehicle information). These commands leverage the existing vehicle functions from vehicles.c (vehicle_add_passenger, vehicle_remove_passenger, vehicle_move, etc.) and follow established MUD command patterns using the ACMD() macro.

Upon completion, players will be able to find vehicles in the world, mount them, drive across terrain using the wilderness coordinate system with terrain validation, check vehicle status, and dismount. This enables testing of the entire vehicle system and prepares the foundation for Session 05 (vehicle-in-vehicle mechanics) and Session 06 (unified command interface).

---

## 2. Objectives

1. Implement do_mount() command handler that allows players to enter/mount vehicles present in their current room
2. Implement do_dismount() command handler that allows players to exit vehicles they are currently riding
3. Implement do_drive() command handler that moves mounted vehicles using existing vehicle_move() with terrain validation
4. Implement do_vstatus() command handler that displays comprehensive vehicle status information
5. Register all four commands in interpreter.c cmd_info[] array
6. Create help file entries for each command in lib/text/help/

---

## 3. Prerequisites

### Required Sessions
- [x] `phase02-session01-vehicle-data-structures` - Provides struct vehicle_data, vehicle types, states
- [x] `phase02-session02-vehicle-creation-system` - Provides vehicle_create(), lifecycle functions
- [x] `phase02-session03-vehicle-movement-system` - Provides vehicle_move(), terrain validation

### Required Tools/Knowledge
- ACMD() macro pattern for command handlers
- interpreter.c cmd_info[] registration format
- send_to_char() messaging pattern
- Understanding of struct char_data for player state

### Environment Requirements
- Vehicles.c compiled and linked
- vessels.h with ACMD_DECL() prototypes already present

---

## 4. Scope

### In Scope (MVP)
- `mount <target>` command - Mount a vehicle in the same room
- `dismount` command - Exit current vehicle to room
- `drive <direction>` command - Move vehicle (north, south, east, west, ne, nw, se, sw)
- `vstatus` command - Display vehicle state, position, capacity, condition
- Command registration in interpreter.c
- Input validation and error messaging
- Help file entries for all four commands

### Out of Scope (Deferred)
- Mounting animals/mobs - *Reason: Different system, potential conflict*
- Vehicle combat commands - *Reason: Session 07 scope*
- Vehicle-in-vehicle mounting - *Reason: Session 05 scope*
- Unified mount/board abstraction - *Reason: Session 06 scope*
- Multiple vehicle types per room selection - *Reason: Use first vehicle for MVP*

---

## 5. Technical Approach

### Architecture
Commands will be implemented in a new file `src/vehicles_commands.c` to maintain separation of concerns. Each command follows the standard ACMD pattern and calls existing vehicle functions from vehicles.c. Player-vehicle association will be tracked through a simple mechanism using the vehicle's current_passengers count and player room matching.

### Design Patterns
- **ACMD Pattern**: Standard LuminariMUD command handler pattern with argument parsing
- **Delegation**: Commands delegate to vehicle_* functions rather than duplicating logic
- **Fail-Fast Validation**: Check preconditions early with descriptive error messages

### Technology Stack
- ANSI C90/C89 (no // comments, no declarations after statements)
- Standard MUD messaging via send_to_char()
- Existing wilderness coordinate system for drive command
- CuTest for unit testing

---

## 6. Deliverables

### Files to Create
| File | Purpose | Est. Lines |
|------|---------|------------|
| `src/vehicles_commands.c` | All four command implementations (do_mount, do_dismount, do_drive, do_vstatus) | ~350 |
| `lib/text/help/vehicles.hlp` | Help entries for mount, dismount, drive, vstatus commands | ~80 |
| `unittests/CuTest/test_vehicle_commands.c` | Unit tests for command handlers | ~200 |

### Files to Modify
| File | Changes | Est. Lines |
|------|---------|------------|
| `src/interpreter.c` | Add cmd_info[] entries for mount, dismount, drive, vstatus | ~15 |
| `src/Makefile.am` | Add vehicles_commands.c to source list | ~2 |
| `CMakeLists.txt` | Add vehicles_commands.c to source list | ~2 |
| `src/vessels.h` | Add player-vehicle tracking helpers if needed | ~10 |

---

## 7. Success Criteria

### Functional Requirements
- [ ] mount command successfully adds player to vehicle in same room
- [ ] mount command fails with error if no vehicle in room
- [ ] mount command fails with error if vehicle is full
- [ ] mount command fails with error if vehicle is damaged
- [ ] dismount command removes player from vehicle
- [ ] dismount command fails if player not in a vehicle
- [ ] drive command moves vehicle in specified direction
- [ ] drive command fails with terrain error for impassable terrain
- [ ] drive command fails if vehicle damaged or player not driver
- [ ] vstatus displays vehicle type, state, position, capacity, condition

### Testing Requirements
- [ ] Unit tests written and passing for all command handlers
- [ ] Unit tests cover edge cases (NULL checks, invalid states)
- [ ] Manual testing completed with actual vehicle in-game

### Quality Gates
- [ ] All files ASCII-encoded (characters 0-127 only)
- [ ] Unix LF line endings
- [ ] Code follows project conventions (2-space indent, Allman braces)
- [ ] No compiler warnings with -Wall -Wextra
- [ ] Valgrind clean (no memory leaks in tests)

---

## 8. Implementation Notes

### Key Considerations
- Direction parsing: Must handle "north", "n", "1" formats consistently
- Vehicle lookup: Use vehicle_find_in_room() for mount, track current vehicle for drive/dismount
- Player state: Need mechanism to track which vehicle a player is in (consider char_data field or room-based)
- Error messages: Use consistent, player-friendly language

### Potential Challenges
- **Player-vehicle association**: The vehicle_data struct tracks current_passengers count but not WHO is a passenger. For MVP, use room-based logic: if player is in a "vehicle room" created by the vehicle, they are aboard.
- **Drive vs Move**: drive command is for player input; internally calls vehicle_move() which handles terrain
- **Already mounted check**: Prevent mounting multiple vehicles; dismount from current first

### Relevant Considerations
- [P01] **C90/C89 compliance**: Use /* */ comments only, declare all variables at block start
- [P01] **NULL checks critical**: Every pointer must be checked before dereferencing
- [P01] **CuTest for unit testing**: Follow existing test patterns in unittests/CuTest/
- [P00] **Interior movement unimplemented**: Commands operate at vehicle level, not room-to-room within vehicle

### ASCII Reminder
All output files must use ASCII-only characters (0-127). No Unicode quotes, dashes, or special characters.

---

## 9. Testing Strategy

### Unit Tests
- test_do_mount_no_vehicle_in_room - Returns error, no crash
- test_do_mount_vehicle_full - Returns capacity error
- test_do_mount_vehicle_damaged - Returns damaged error
- test_do_mount_success - Increases passenger count
- test_do_dismount_not_mounted - Returns error
- test_do_dismount_success - Decreases passenger count
- test_do_drive_not_mounted - Returns error
- test_do_drive_invalid_direction - Returns direction error
- test_do_drive_impassable_terrain - Returns terrain error
- test_do_drive_success - Updates vehicle coordinates
- test_do_vstatus_no_vehicle - Returns error or no output
- test_do_vstatus_success - Outputs vehicle information

### Integration Tests
- Mount vehicle, drive in circle, dismount - Full workflow
- Mount full vehicle fails, another player dismounts, mount succeeds

### Manual Testing
- Create vehicle in test zone, login as player, run through all commands
- Test error messages are player-friendly and informative
- Test drive command with various terrain types

### Edge Cases
- NULL character pointer passed to commands
- Vehicle destroyed while player is mounted
- Multiple players trying to mount simultaneously (race condition unlikely in MUD)
- Direction abbreviations: n, s, e, w, ne, nw, se, sw

---

## 10. Dependencies

### External Libraries
- None (uses standard MUD systems)

### Other Sessions
- **Depends on**: phase02-session03-vehicle-movement-system (provides vehicle_move, terrain functions)
- **Depended by**: phase02-session05-vehicle-in-vehicle-mechanics, phase02-session06-unified-command-interface

---

## Next Steps

Run `/tasks` to generate the implementation task checklist.
