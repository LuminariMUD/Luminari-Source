# Session Specification

**Session ID**: `phase00-session06-interior-movement-implementation`
**Phase**: 00 - Core Vessel System
**Status**: Not Started
**Created**: 2025-12-29

---

## 1. Session Overview

This session implements the interior movement system for vessels, enabling players to navigate between generated ship interior rooms using standard direction commands. Sessions 01-05 established the foundation: cleaned headers, dynamic wilderness room allocation, vessel type system, Phase 2 command registration, and interior room generation with connectivity. Interior rooms now exist and have connections defined, but the actual movement functions remain unimplemented.

The three core movement functions declared in `vessels.h:560-563` (`do_move_ship_interior()`, `get_ship_exit()`, `is_passage_blocked()`) are the primary deliverables. These functions must integrate seamlessly with the existing movement system in `movement.c` so that standard direction commands (north, south, east, west, up, down) work inside ship interiors without breaking existing wilderness or normal room movement.

This is a critical path session - all downstream sessions (persistence, external view, testing) depend on functional interior navigation. The session directly resolves the "Interior movement unimplemented" technical debt noted in CONSIDERATIONS.md.

---

## 2. Objectives

1. Implement `get_ship_exit()` to retrieve exit information for a given direction from ship interior rooms
2. Implement `is_passage_blocked()` to check if a passage/hatch is blocked or locked
3. Implement `do_move_ship_interior()` to handle character movement between ship interior rooms
4. Wire interior movement detection into the standard movement system for seamless command handling

---

## 3. Prerequisites

### Required Sessions
- [x] `phase00-session01-header-cleanup-foundation` - Clean vessels.h structure
- [x] `phase00-session02-dynamic-wilderness-room-allocation` - Room allocation patterns
- [x] `phase00-session03-vessel-type-system` - Vessel classification system
- [x] `phase00-session04-phase2-command-registration` - Command infrastructure
- [x] `phase00-session05-interior-room-generation-wiring` - Interior rooms and connections exist

### Required Tools/Knowledge
- Understanding of `movement.c` `do_simple_move()` and `perform_move()` patterns
- Understanding of `room_direction_data` structure for exits
- Understanding of `room_connection` structure in `vessels.h:421-428`
- Understanding of `greyhawk_ship_data` multi-room fields (lines 478-499)

### Environment Requirements
- Compiled working server binary
- Test ship with generated interior rooms (from Session 05)

---

## 4. Scope

### In Scope (MVP)
- Implement `get_ship_exit()` - query ship connections to find exit for direction
- Implement `is_passage_blocked()` - check `is_locked` flag on connections
- Implement `do_move_ship_interior()` - move character between interior rooms
- Detect when character is in ship interior vs normal room
- Display proper exit information in room descriptions
- Movement messages for entering/leaving ship rooms
- Handle all 6 cardinal directions (N/S/E/W/U/D)

### Out of Scope (Deferred)
- Diagonal directions (NE/NW/SE/SW) - *Reason: Ships use simple 6-direction layout*
- Exterior ship movement - *Reason: Already working via wilderness system*
- Boarding/disembarking - *Reason: Separate session scope*
- Combat in ship interiors - *Reason: Separate combat integration session*
- Hatch opening/closing commands - *Reason: Future enhancement*

---

## 5. Technical Approach

### Architecture

The interior movement system operates alongside the existing movement system, not replacing it. Detection occurs early in `do_simple_move()` to determine if a character is in a ship interior room. If so, the movement is delegated to `do_move_ship_interior()` which handles ship-specific logic.

```
Standard Movement Flow:
  do_move (ACMD) -> perform_move() -> do_simple_move() -> move character

Ship Interior Detection:
  do_simple_move() -> is_in_ship_interior() -> do_move_ship_interior()
                   -> get_ship_exit() to find destination
                   -> is_passage_blocked() to check access
                   -> char_from_room() / char_to_room() to move
```

### Design Patterns
- **Delegation Pattern**: Ship interior detection delegates to specialized handler
- **Lookup Pattern**: `get_ship_exit()` searches ship connections for direction match
- **Guard Pattern**: `is_passage_blocked()` as precondition check before movement

### Technology Stack
- ANSI C90/C89 (no C99 features)
- Existing LuminariMUD macros (IN_ROOM, char_from_room, char_to_room, send_to_char)
- `room_connection` structure for connectivity data
- `greyhawk_ship_data` structure for ship state

---

## 6. Deliverables

### Files to Create
| File | Purpose | Est. Lines |
|------|---------|------------|
| None | All implementation in existing files | - |

### Files to Modify
| File | Changes | Est. Lines |
|------|---------|------------|
| `src/vessels_rooms.c` | Implement `get_ship_exit()`, `is_passage_blocked()`, `do_move_ship_interior()` | ~120-150 |
| `src/movement.c` | Add ship interior detection hook in `do_simple_move()` | ~20-30 |

---

## 7. Success Criteria

### Functional Requirements
- [ ] `get_ship_exit()` returns correct destination room for valid direction
- [ ] `get_ship_exit()` returns NOWHERE for invalid/non-existent direction
- [ ] `is_passage_blocked()` returns TRUE for locked hatches
- [ ] `is_passage_blocked()` returns FALSE for open passages
- [ ] `do_move_ship_interior()` moves character to destination room
- [ ] `do_move_ship_interior()` displays proper movement messages
- [ ] `do_move_ship_interior()` rejects blocked passages with message
- [ ] Standard direction commands (n, s, e, w, u, d) work inside ship
- [ ] Movement between ship rooms does not break existing movement system
- [ ] Characters can navigate entire ship interior via directions

### Testing Requirements
- [ ] Manual testing: move between all ship interior rooms
- [ ] Manual testing: attempt movement in invalid direction (no exit)
- [ ] Manual testing: verify messages display correctly
- [ ] Verify existing wilderness/normal room movement still works

### Quality Gates
- [ ] All files ASCII-encoded
- [ ] Unix LF line endings
- [ ] Code follows C90 conventions (no // comments, declarations at block start)
- [ ] No compiler warnings
- [ ] NULL pointer checks on all function entries
- [ ] Memory safe (no allocations that aren't freed)

---

## 8. Implementation Notes

### Key Considerations
- `get_ship_from_room()` already implemented - use it to detect ship context
- Ship connections stored in `ship->connections[]` array with `num_connections` count
- Each connection has `from_room`, `to_room`, `direction`, `is_hatch`, `is_locked`
- `rev_dir[]` array maps directions to reverse directions
- Use `real_room()` to convert vnums to rnums for comparison

### Potential Challenges
- **Bidirectional lookup**: Connections may be stored one-way; must check both directions
- **Room identification**: Connections store vnums; rooms identified by rnum in movement code
- **Exit display**: May need to update room exit display to show ship exits correctly
- **Movement integration**: Must not break existing movement for non-ship rooms

### Relevant Considerations
- **[P00] Interior movement unimplemented**: This session directly resolves this active concern
- **[P00] Don't use C99/C11 features**: All declarations at block start, use /* */ comments
- **[P00] Don't skip NULL checks**: Check ch, ship, room pointers before dereferencing

### ASCII Reminder
All output files must use ASCII-only characters (0-127).

---

## 9. Testing Strategy

### Unit Tests
- Not applicable for this session (integration with game loop)

### Integration Tests
- Verify ship interior movement doesn't break normal movement
- Verify wilderness movement still functions
- Verify room zone changes work correctly

### Manual Testing
1. Load a test ship with generated interior rooms
2. Enter the ship (board command)
3. Use `look` to see available exits
4. Move north/south/east/west/up/down
5. Verify arrival in correct rooms
6. Attempt invalid direction (should fail gracefully)
7. Test locked hatch blocking (if any exist)
8. Exit ship and verify normal movement works

### Edge Cases
- Character in room with no ship (should fall through to normal movement)
- Direction with no exit defined
- Room vnum not found in world table
- Ship with zero connections
- Locked hatch (should block with message)
- Movement to NOWHERE (should fail gracefully)

---

## 10. Dependencies

### External Libraries
- None (uses existing LuminariMUD infrastructure)

### Other Sessions
- **Depends on**: `phase00-session05-interior-room-generation-wiring` (rooms must exist)
- **Depended by**: `phase00-session07-persistence-integration`, `phase00-session08-external-view-display-systems`, `phase00-session09-testing-validation`

---

## Next Steps

Run `/tasks` to generate the implementation task checklist.
