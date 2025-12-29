# Session 02: Dynamic Wilderness Room Allocation

**Session ID**: `phase00-session02-dynamic-wilderness-room-allocation`
**Status**: Not Started
**Estimated Tasks**: ~18-22
**Estimated Duration**: 3-4 hours

---

## Objective

Implement dynamic wilderness room allocation for vessel movement, fixing the critical bug where vessels fail silently when moving to coordinates without pre-allocated rooms.

---

## Scope

### In Scope (MVP)
- Study existing pattern in src/movement.c (find_available_wilderness_room + assign_wilderness_room)
- Implement same pattern for vessel movement in vessels.c
- Handle edge cases (room pool exhausted, invalid coordinates)
- Add appropriate error messages for failed allocations
- Test movement across various wilderness coordinates
- Verify rooms are properly deallocated when vessels leave

### Out of Scope
- Performance optimization
- Room pooling enhancements
- Multi-vessel coordination at same coordinate

---

## Prerequisites

- [ ] Session 01 completed (clean codebase)
- [ ] Understanding of wilderness coordinate system
- [ ] Understanding of room allocation patterns

---

## Deliverables

1. Updated vessels.c with dynamic room allocation
2. Vessel movement works to any valid wilderness coordinate
3. Proper error handling for allocation failures
4. No memory leaks from room management

---

## Success Criteria

- [ ] Vessels can move to any wilderness coordinate (-1024 to +1024)
- [ ] Movement no longer fails silently
- [ ] Appropriate error messages displayed on allocation failure
- [ ] Rooms properly released when vessel moves away
- [ ] No memory leaks (Valgrind clean)
- [ ] Movement respects terrain restrictions

---

## Technical Notes

### Current Issue

From PRD Section 7 (Phase 1 Known Limitations):
> Vessel movement does not allocate new dynamic wilderness rooms. Movement fails if destination coordinate has no pre-allocated room.

### Reference Pattern

From src/movement.c, the pattern is:
```c
room_rnum dest = find_available_wilderness_room();
if (dest != NOWHERE) {
    assign_wilderness_room(dest, x, y);
    /* move character/vessel to dest */
}
```

### Key Functions to Study

- find_available_wilderness_room() in wilderness.c
- assign_wilderness_room() in wilderness.c
- release_wilderness_room() in wilderness.c (if exists)
- Current vessel movement code in vessels.c

### Files to Modify

- src/vessels.c - Add room allocation to movement functions
- Possibly src/wilderness.c - If vessel-specific allocation needed
