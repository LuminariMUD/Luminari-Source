# Session 02: Interior Movement Implementation

**Session ID**: `phase03-session02-interior-movement-implementation`
**Status**: Not Started
**Estimated Tasks**: ~18-22
**Estimated Duration**: 3-4 hours

---

## Objective

Implement the interior movement functions declared in vessels.h but currently unimplemented: do_move_ship_interior(), get_ship_exit(), and is_passage_blocked().

---

## Scope

### In Scope (MVP)

- Implement do_move_ship_interior() - handle movement between interior rooms
- Implement get_ship_exit() - return appropriate exit for direction within ship
- Implement is_passage_blocked() - check if passage between rooms is blocked
- Wire interior movement into standard movement system
- Handle edge cases (invalid directions, blocked passages, room boundaries)
- Support for all room types in ship interiors
- Unit tests for all three functions
- Integration with existing room generation system

### Out of Scope

- New room types
- Database schema changes
- Docking/boarding mechanics (already implemented)
- Exterior vessel movement

---

## Prerequisites

- [ ] Session 01 complete (code consolidated)
- [ ] Interior room generation functional
- [ ] Ship room templates defined

---

## Deliverables

1. Implemented do_move_ship_interior() function
2. Implemented get_ship_exit() function
3. Implemented is_passage_blocked() function
4. Unit tests for interior movement (target: 15+ tests)
5. Integration with movement.c if needed
6. Updated documentation

---

## Success Criteria

- [ ] Players can move between ship interior rooms
- [ ] Movement respects room exits and blocked passages
- [ ] Invalid movement attempts handled gracefully
- [ ] All new functions have unit tests
- [ ] Valgrind clean
- [ ] No regression in existing tests
