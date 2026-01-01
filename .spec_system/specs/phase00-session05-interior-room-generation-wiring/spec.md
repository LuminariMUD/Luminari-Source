# Session Specification

**Session ID**: `phase00-session05-interior-room-generation-wiring`
**Phase**: 00 - Core Vessel System
**Status**: Not Started
**Created**: 2025-12-29

---

## 1. Session Overview

This session wires the existing `generate_ship_interior()` function to ship creation and loading entry points, enabling vessels to have properly generated interior rooms when they are instantiated. Currently, the function exists in `vessels_rooms.c` but is never called, meaning ships have no interior rooms despite the infrastructure being in place.

The interior room generation is critical for the vessel system because it provides the navigable spaces that players interact with once aboard a ship. Without this wiring, commands like `ship_rooms` would return empty results, and subsequent features (interior movement in Session 06) would have nothing to work with.

This session builds directly on Session 04's command registration work and enables Session 06's interior movement implementation by ensuring rooms exist for navigation.

---

## 2. Objectives

1. Wire `generate_ship_interior()` to `greyhawk_loadship()` so new ships have interior rooms on creation
2. Integrate interior generation with vessel type system to ensure room layouts match vessel configuration
3. Verify VNUM allocation uses the reserved 30000-40019 range without conflicts
4. Ensure `ship_rooms` command correctly displays generated interior rooms
5. Handle edge cases: ship loading when rooms may already exist, NULL checks throughout

---

## 3. Prerequisites

### Required Sessions
- [x] `phase00-session01-header-cleanup-foundation` - Provides clean vessels.h structure
- [x] `phase00-session02-dynamic-wilderness-room-allocation` - Provides room allocation patterns
- [x] `phase00-session03-vessel-type-system` - Provides VESSEL_TYPE_* constants and terrain capabilities
- [x] `phase00-session04-phase2-command-registration` - Provides ship_rooms command registration

### Required Tools/Knowledge
- Understanding of LuminariMUD room system (world array, real_room(), room_vnum)
- ANSI C90/C89 coding conventions
- Greyhawk ship system structures (greyhawk_ship_data, greyhawk_ships array)

### Environment Requirements
- MySQL/MariaDB running with ship_room_templates table populated (19 templates)
- Build environment configured (cmake or autotools)

---

## 4. Scope

### In Scope (MVP)
- Call `generate_ship_interior()` at end of `greyhawk_loadship()` after ship initialization
- Verify `create_ship_room()` allocates VNUMs in 30000-40019 range
- Connect vessel_type to room generation switch statement
- Test `ship_rooms` command output for newly created ships
- Handle duplicate room prevention on ship reload scenarios
- Add appropriate NULL checks and error logging

### Out of Scope (Deferred)
- Interior movement between rooms - *Reason: Session 06 scope*
- Database persistence of interior rooms - *Reason: Session 07 scope*
- New room template types beyond existing 10 - *Reason: Feature expansion, not wiring*
- Replacing hard-coded templates with DB lookup - *Reason: Stretch goal, not MVP*

---

## 5. Technical Approach

### Architecture

The wiring follows this call flow:
```
greyhawk_loadship() [vessels_src.c:2277]
    |
    +-> Initialize ship data (existing code)
    +-> generate_ship_interior(ship) [NEW CALL]
            |
            +-> get_max_rooms_for_type(ship->vessel_type)
            +-> add_ship_room() [per room type]
                    |
                    +-> create_ship_room() [vessels_rooms.c:137]
                            |
                            +-> Allocate VNUM: 30000 + (shipnum * MAX_SHIP_ROOMS) + index
                            +-> Create room in world array
```

### Design Patterns
- **Entry Point Hook**: Add single function call at end of ship creation, keeping code modular
- **Idempotent Design**: Check if rooms already exist before generating to handle reload scenarios
- **Fail-Safe Defaults**: If vessel_type is invalid, default to minimal room set (bridge only)

### Technology Stack
- ANSI C90/C89 (no C99 features)
- LuminariMUD room management (world[], real_room(), CREATE macros)
- Existing vessels_rooms.c infrastructure

---

## 6. Deliverables

### Files to Create
| File | Purpose | Est. Lines |
|------|---------|------------|
| None | All functionality exists, only wiring needed | 0 |

### Files to Modify
| File | Changes | Est. Lines |
|------|---------|------------|
| `src/vessels_src.c` | Add generate_ship_interior() call in greyhawk_loadship() | ~5-10 |
| `src/vessels_rooms.c` | Add room existence check, fix any VNUM calculation issues | ~10-20 |
| `src/vessels_commands.c` | Verify/fix ship_rooms command to display generated rooms | ~5-15 |

---

## 7. Success Criteria

### Functional Requirements
- [ ] `greyhawk_loadship()` calls `generate_ship_interior()` after ship initialization
- [ ] Interior rooms created with VNUMs in 30000-40019 range
- [ ] Room count matches vessel type expectations (raft=1, boat=2, ship=4, etc.)
- [ ] `ship_rooms` command lists all interior rooms with correct details
- [ ] No VNUM conflicts with existing builder zones

### Testing Requirements
- [ ] Manual test: Create ship via shipload, verify rooms exist
- [ ] Manual test: Run ship_rooms command, verify output
- [ ] Code review: Confirm NULL checks on all pointer dereferences
- [ ] Build verification: Compile cleanly with -Wall -Wextra

### Quality Gates
- [ ] All files ASCII-encoded
- [ ] Unix LF line endings
- [ ] Code follows project conventions (two-space indent, Allman braces, /* */ comments)
- [ ] No new compiler warnings introduced

---

## 8. Implementation Notes

### Key Considerations
- `greyhawk_loadship()` already initializes `ship->vessel_type` implicitly via template - need to verify or explicitly set
- `create_ship_room()` at line 137 of vessels_rooms.c handles VNUM allocation
- The VNUM formula is: `30000 + (ship->shipnum * MAX_SHIP_ROOMS) + room_index`
- MAX_SHIP_ROOMS is likely 20 based on range 30000-40019 (1000 ships * 20 rooms)

### Potential Challenges
- **vessel_type not set**: greyhawk_loadship() may not set vessel_type correctly; need to verify or derive from template
- **Reload conflicts**: If ship data persists but rooms don't, reloading could cause duplicate room attempts
- **VNUM exhaustion**: With 1000 potential ships * 20 rooms, ensure bounds checking exists

### Relevant Considerations
- [P00] **Room templates hard-coded**: The 10 hard-coded templates in vessels_rooms.c are acceptable for MVP; DB lookup is stretch goal
- [P00] **Don't hardcode VNUMs**: Use SHIP_INTERIOR_VNUM_BASE (30000) constant, not magic numbers
- [P00] **Don't skip NULL checks**: Critical for ship pointer and room creation results

### ASCII Reminder
All output files must use ASCII-only characters (0-127).

---

## 9. Testing Strategy

### Unit Tests
- Test `get_max_rooms_for_type()` returns correct room counts per vessel type
- Test VNUM calculation formula returns expected values

### Integration Tests
- Ship creation to room generation flow
- ship_rooms command integration with generated rooms

### Manual Testing
1. Connect to running server as implementor
2. Use `shipload <template>` to create a new ship
3. Board ship and verify room exists
4. Run `ship_rooms` command, verify output lists rooms
5. Repeat for different vessel types (raft, boat, ship, warship)

### Edge Cases
- Ship creation with invalid vessel_type (should default to minimal layout)
- Ship creation when VNUM range exhausted (should log error, not crash)
- ship_rooms on ship with no interior (should handle gracefully)

---

## 10. Dependencies

### External Libraries
- MySQL/MariaDB: For future DB template lookup (stretch goal only)

### Other Sessions
- **Depends on**: session01 (header), session02 (room allocation), session03 (vessel types), session04 (commands)
- **Depended by**: session06 (interior movement needs rooms to navigate)

---

## Next Steps

Run `/tasks` to generate the implementation task checklist.
