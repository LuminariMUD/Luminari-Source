# Session Specification

**Session ID**: `phase02-session06-unified-command-interface`
**Phase**: 02 - Simple Vehicle Support
**Status**: Not Started
**Created**: 2025-12-30

---

## 1. Session Overview

This session creates a unified command interface that provides consistent player commands across all transport types (vehicles and vessels). Currently, players must learn separate command sets for different transport types: `mount`/`dismount`/`drive` for vehicles, and `board`/`disembark`/`pilot` for vessels. This creates an inconsistent user experience and duplicates command handling logic.

The unified interface introduces an abstraction layer with transport-agnostic commands: `enter`, `exit`, `go`, and `tstatus`. These commands automatically detect whether the target is a vehicle or vessel and delegate to the appropriate underlying implementation. This approach provides a consistent player experience regardless of transport type, reduces documentation complexity, and enables future transport types to integrate seamlessly.

This session builds on the complete vehicle implementation (sessions 01-05) and the vessel system from phases 00-01. It is the natural culmination of the transport system work before final phase validation.

---

## 2. Objectives

1. Create a transport abstraction structure (`transport_data`) with type detection
2. Implement unified `do_enter` command that handles both vehicle and vessel boarding
3. Implement unified `do_exit_transport` command for dismounting/disembarking
4. Implement unified `do_transport_go` movement command
5. Implement unified `do_transportstatus` status display command
6. Maintain full backward compatibility with existing transport-specific commands

---

## 3. Prerequisites

### Required Sessions
- [x] `phase02-session05-vehicle-in-vehicle-mechanics` - Vehicle loading/unloading onto vessels
- [x] `phase02-session04-vehicle-player-commands` - Vehicle mount/dismount/drive/vstatus
- [x] `phase02-session03-vehicle-movement-system` - Vehicle movement and terrain navigation
- [x] `phase02-session02-vehicle-creation-system` - Vehicle lifecycle and persistence
- [x] `phase02-session01-vehicle-data-structures` - Vehicle data foundations
- [x] `phase01-session*` - Complete vessel automation layer
- [x] `phase00-session*` - Core vessel system

### Required Tools/Knowledge
- Understanding of existing vessel command structure in vessels_src.c
- Understanding of vehicle command structure in vehicles_commands.c
- Familiarity with creature mount system in act.other.c (to avoid conflicts)

### Environment Requirements
- CuTest framework for unit testing
- Valgrind for memory leak detection

---

## 4. Scope

### In Scope (MVP)
- Transport type enumeration (`TRANSPORT_VEHICLE`, `TRANSPORT_VESSEL`, `TRANSPORT_NONE`)
- `transport_data` abstraction structure with union for vehicle/vessel
- `get_transport_type()` - Detect transport in room or character's current transport
- `do_enter` command - Unified entry (replaces mount/board for new usage)
- `do_exit_transport` command - Unified exit (replaces dismount/disembark for new usage)
- `do_transport_go` command - Unified movement (replaces drive/sail)
- `do_transportstatus` command - Unified status display
- Unit tests for all new functions
- Backward compatibility with existing `mount`, `dismount`, `drive`, `vstatus`, `board`, `disembark`, `pilot`, `vessel_status` commands

### Out of Scope (Deferred)
- Unified autopilot commands - *Reason: Session 07 scope*
- Unified combat interface - *Reason: Future phase*
- Transport switching while moving - *Reason: Complex edge case*
- GUI/client protocol extensions - *Reason: Not required for CLI*
- Creature mount integration - *Reason: Separate system, would conflict*

---

## 5. Technical Approach

### Architecture
The unified interface uses a thin abstraction layer that wraps existing implementations. Rather than refactoring the existing vehicle and vessel code, we create dispatcher functions that:

1. Detect the transport type in the current context
2. Validate the operation is appropriate for that transport type
3. Delegate to the existing implementation
4. Provide consistent messaging to the player

This approach minimizes risk by preserving all existing, tested code while adding a convenience layer on top.

### Design Patterns
- **Adapter Pattern**: Unified commands adapt to underlying vehicle/vessel APIs
- **Type Dispatch**: Switch on transport type enum to call appropriate handler
- **Wrapper Functions**: Thin wrappers provide unified interface without modifying existing code

### Technology Stack
- ANSI C90/C89 (strict compliance, no C99/C11 features)
- CuTest for unit testing
- Existing vessel/vehicle infrastructure

---

## 6. Deliverables

### Files to Create
| File | Purpose | Est. Lines |
|------|---------|------------|
| `src/transport.c` | Unified transport command implementations | ~400 |
| `src/transport.h` | Transport abstraction types and prototypes | ~80 |
| `unittests/CuTest/test_transport.c` | Unit tests for transport abstraction | ~250 |

### Files to Modify
| File | Changes | Est. Lines |
|------|---------|------------|
| `src/interpreter.c` | Register new unified commands | ~10 |
| `src/act.h` | Add ACMD declarations for new commands | ~5 |
| `src/Makefile.am` | Add transport.c to build | ~2 |
| `CMakeLists.txt` | Add transport.c to CMake build | ~2 |
| `unittests/CuTest/Makefile` | Add test_transport.c to tests | ~3 |

---

## 7. Success Criteria

### Functional Requirements
- [ ] `enter <target>` works for both vehicles and vessels in room
- [ ] `exit` (while in transport) works for both vehicle dismount and vessel disembark
- [ ] `go <direction>` works for both vehicle driving and vessel sailing
- [ ] `tstatus` shows status for current transport (vehicle or vessel)
- [ ] All existing commands (`mount`, `board`, etc.) continue to work unchanged
- [ ] Transport type detection correctly distinguishes vehicles from vessels

### Testing Requirements
- [ ] Unit tests for transport type detection
- [ ] Unit tests for enter/exit logic
- [ ] Unit tests for movement delegation
- [ ] Unit tests for status display
- [ ] Manual testing of all commands in-game
- [ ] Valgrind clean (no memory leaks)

### Quality Gates
- [ ] All files ASCII-encoded (0-127 characters only)
- [ ] Unix LF line endings
- [ ] Code follows ANSI C90 standard (no // comments, no declarations after statements)
- [ ] All compiler warnings resolved (-Wall -Wextra)
- [ ] CuTest tests pass
- [ ] clang-format applied

---

## 8. Implementation Notes

### Key Considerations
- The `enter` command must distinguish between vehicles, vessels, and bulletin boards (which currently use `board`)
- Creature mounts use `mount`/`dismount` in act.other.c - we must NOT interfere with this system
- The unified commands should work transparently whether player is already in a transport or not
- Room scanning for transports should be O(1) or O(n) with small n (check vehicle first, then vessel)

### Potential Challenges
- **Board command collision**: The `board` command already exists for bulletin boards (mysql_boards.c) and vessels (vessels_src.c). The unified `enter` command avoids this by using a distinct name.
- **Mount command collision**: Creature mounts use `mount`/`dismount`. We use `enter`/`exit` to avoid conflicts.
- **Transport detection order**: When both vehicle and vessel are in room, need consistent priority (vehicle first, as more common case).
- **Exit ambiguity**: If player is both mounted on creature and in vehicle, which exit applies? Solution: Only handle transport exits, not creature dismounts.

### Relevant Considerations
- [P01] **Don't use C99/C11 features**: All code must be strictly ANSI C90/C89 compliant
- [P01] **Standalone unit test files**: Tests must not require server initialization
- [P01] **Incremental session approach**: Build cleanly on sessions 01-05 without modifying their code
- [P00] **NULL checks critical**: Every pointer must be validated before dereferencing

### ASCII Reminder
All output files must use ASCII-only characters (0-127). No Unicode, no extended ASCII.

---

## 9. Testing Strategy

### Unit Tests
- `test_get_transport_type_null` - NULL room returns TRANSPORT_NONE
- `test_get_transport_type_empty` - Empty room returns TRANSPORT_NONE
- `test_get_transport_type_vehicle` - Room with vehicle returns TRANSPORT_VEHICLE
- `test_get_transport_type_vessel` - Room with vessel returns TRANSPORT_VESSEL
- `test_transport_enter_validation` - Entry validation logic
- `test_transport_exit_validation` - Exit validation logic
- `test_transport_movement_validation` - Movement validation logic
- `test_transport_data_init` - Abstraction structure initialization

### Integration Tests
- All unified commands work with mock transport data
- Type detection correctly identifies transport types
- Delegation to existing functions works correctly

### Manual Testing
- Create vehicle, use `enter` to board, `go` to move, `tstatus` to view, `exit` to leave
- Board vessel, use `go` to sail, `tstatus` to view, `exit` to disembark
- Verify `mount`, `dismount`, `drive`, `board`, `disembark` still work unchanged
- Test error cases (no transport in room, wrong transport type, etc.)

### Edge Cases
- No transport in room (clear error message)
- Already in transport trying to enter another
- Attempting to move in non-operational transport
- NULL pointer handling at all boundaries
- Room with both vehicle AND vessel (priority handling)

---

## 10. Dependencies

### External Libraries
- None (uses existing infrastructure)

### Other Sessions
- **Depends on**: phase02-session01 through phase02-session05 (vehicle system)
- **Depends on**: phase00-* and phase01-* (vessel system)
- **Depended by**: phase02-session07-testing-validation (final phase validation)

---

## Next Steps

Run `/tasks` to generate the implementation task checklist.
