# NEXT_SESSION.md

## Session Recommendation

**Generated**: 2025-12-29
**Project State**: Phase 00 - Core Vessel System
**Completed Sessions**: 5

---

## Recommended Next Session

**Session ID**: `phase00-session06-interior-movement-implementation`
**Session Name**: Interior Movement Implementation
**Estimated Duration**: 3-4 hours
**Estimated Tasks**: 20-25

---

## Why This Session Next?

### Prerequisites Met
- [x] Session 05 completed (interior room generation wiring)
- [x] Interior rooms exist and are navigable via generate_ship_interior()
- [x] Standard movement system understood (src/movement.c patterns)

### Dependencies
- **Builds on**: Session 05 (Interior Room Generation Wiring) - rooms now generated
- **Enables**: Session 07 (Persistence Integration) - movement must work before persisting state

### Project Progression

This is the natural next step because:
1. Sessions 01-05 established the foundation: cleaned headers, dynamic wilderness room allocation, vessel type system, Phase 2 command registration, and interior room generation
2. Interior rooms are now being created, but players cannot navigate between them
3. The three movement functions (`do_move_ship_interior()`, `get_ship_exit()`, `is_passage_blocked()`) are declared in `vessels.h:671-674` but have no implementation
4. This directly addresses the "Interior movement unimplemented" technical debt noted in CONSIDERATIONS.md
5. All downstream sessions (persistence, external view, testing) depend on functional interior navigation

---

## Session Overview

### Objective

Implement the interior movement functions declared in vessels.h but not yet implemented, enabling players to navigate between generated ship interior rooms using standard direction commands.

### Key Deliverables
1. Functional `do_move_ship_interior()` - Handle movement between interior rooms
2. Functional `get_ship_exit()` - Get exit information for a direction
3. Functional `is_passage_blocked()` - Check if passage is blocked
4. Integration with standard movement system for seamless direction commands
5. Proper exit display in "look" command within ship interiors

### Scope Summary
- **In Scope (MVP)**: Interior movement implementation, exit handling, passage blocking, movement messages
- **Out of Scope**: Exterior movement (already working), boarding/disembarking (separate), combat in interiors

---

## Technical Considerations

### Technologies/Patterns
- ANSI C90/C89 (no C99 features)
- Follow existing movement.c patterns for character movement
- Use room_direction_data structures for exits
- Match Greyhawk system command interface

### Potential Challenges
- Detecting when character is in ship interior vs. wilderness/normal room
- Ensuring exits defined by room generation are properly navigable
- Handling edge cases (room doesn't exist, invalid direction, blocked passage)
- Integration without breaking existing movement system

### Relevant Considerations
- **[P00]** **Interior movement unimplemented**: This session directly resolves this active concern
- **[P00]** **Don't use C99/C11 features**: Must use ANSI C90 variable declarations
- **[P00]** **Don't skip NULL checks**: Critical for all pointer operations in movement functions

---

## Alternative Sessions

If this session is blocked:
1. **phase00-session08-external-view-display-systems** - Could implement look_outside functionality independently
2. **phase00-session09-testing-validation** - Could begin test framework setup in parallel

However, neither alternative is recommended since Session 06 has no blockers and is on the critical path.

---

## Next Steps

Run `/sessionspec` to generate the formal specification with detailed task breakdown.
