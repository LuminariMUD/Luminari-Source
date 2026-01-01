# Session 06: Interior Movement Implementation

**Session ID**: `phase00-session06-interior-movement-implementation`
**Status**: Not Started
**Estimated Tasks**: ~20-25
**Estimated Duration**: 3-4 hours

---

## Objective

Implement the interior movement functions declared in vessels.h but not yet implemented: do_move_ship_interior(), get_ship_exit(), and is_passage_blocked().

---

## Scope

### In Scope (MVP)
- Implement do_move_ship_interior() - Handle movement between interior rooms
- Implement get_ship_exit() - Get exit information for a direction
- Implement is_passage_blocked() - Check if passage is blocked
- Wire interior movement to standard movement system
- Handle room transitions within ship
- Test movement in all directions within ship

### Out of Scope
- Exterior movement (already working)
- Boarding/disembarking (separate functionality)
- Combat in ship interiors

---

## Prerequisites

- [ ] Session 05 completed (interior rooms generated)
- [ ] Interior rooms exist and are navigable
- [ ] Understanding of standard movement system

---

## Deliverables

1. Functional do_move_ship_interior()
2. Functional get_ship_exit()
3. Functional is_passage_blocked()
4. Players can walk between ship interior rooms
5. Proper exit display in "look" command

---

## Success Criteria

- [ ] do_move_ship_interior() moves characters between interior rooms
- [ ] get_ship_exit() returns correct exit for given direction
- [ ] is_passage_blocked() correctly identifies blocked passages
- [ ] Standard direction commands work inside ship (n, s, e, w, u, d)
- [ ] Invalid directions properly rejected
- [ ] Movement messages displayed correctly
- [ ] No crashes on edge cases

---

## Technical Notes

### Declared Functions (from vessels.h:671-674)

```c
void do_move_ship_interior(struct char_data *ch, int direction);
struct room_direction_data *get_ship_exit(room_rnum room, int direction);
bool is_passage_blocked(room_rnum from, room_rnum to);
```

These are declared but have no implementation.

### Interior Room Connectivity

Rooms should be connected based on ship layout:
- Bridge connects to main deck
- Main deck connects to lower deck, quarters
- Corridors connect multiple rooms
- Some rooms may have restricted access

### Integration Points

Interior movement needs to:
1. Detect when character is in ship interior room
2. Use ship-specific movement rather than wilderness
3. Handle exits defined by room generation
4. Respect any access restrictions

### Files to Modify

- src/vessels.c or src/vessels_rooms.c - Implement the functions
- src/movement.c - Possibly add hooks for interior detection
