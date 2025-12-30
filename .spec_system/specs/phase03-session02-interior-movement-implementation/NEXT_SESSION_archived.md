# NEXT_SESSION.md

## Session Recommendation

**Generated**: 2025-12-30
**Project State**: Phase 03 - Optimization & Polish
**Completed Sessions**: 24

---

## Recommended Next Session

**Session ID**: `phase03-session02-interior-movement-implementation`
**Session Name**: Interior Movement Implementation
**Estimated Duration**: 3-4 hours
**Estimated Tasks**: 18-22

---

## Why This Session Next?

### Prerequisites Met
- [x] Session 01 (Code Consolidation) complete
- [x] Interior room generation functional (from Phase 00)
- [x] Ship room templates defined (19 templates in DB)

### Dependencies
- **Builds on**: phase03-session01-code-consolidation (code cleanup complete)
- **Enables**: phase03-session03-command-registration-wiring

### Project Progression
Session 02 implements the missing interior movement functions that are already declared in vessels.h but have no implementation. This is explicitly flagged as Technical Debt item #4 in CONSIDERATIONS.md. The functions (`do_move_ship_interior()`, `get_ship_exit()`, `is_passage_blocked()`) are critical for players to navigate within ship interiors. Session 03 (Command Registration) depends on interior movement working before wiring up commands that rely on it.

---

## Session Overview

### Objective
Implement the interior movement functions declared in vessels.h but currently unimplemented: do_move_ship_interior(), get_ship_exit(), and is_passage_blocked().

### Key Deliverables
1. Implemented do_move_ship_interior() function
2. Implemented get_ship_exit() function
3. Implemented is_passage_blocked() function
4. Unit tests for interior movement (target: 15+ tests)
5. Integration with movement.c if needed
6. Updated documentation

### Scope Summary
- **In Scope (MVP)**: Interior movement between ship rooms, exit handling, passage blocking, edge cases, unit tests
- **Out of Scope**: New room types, database changes, docking/boarding, exterior vessel movement

---

## Technical Considerations

### Technologies/Patterns
- ANSI C90/C89 (no C99/C11 features)
- CuTest for unit testing
- Valgrind for memory validation
- Integration with existing room generation system

### Potential Challenges
- Coordinating with existing room exit data structures
- Handling all 10+ room types in ship interiors
- Edge cases: invalid directions, rooms at vessel boundaries
- Integration with standard movement system without regression

### Relevant Considerations
- [P02] **Interior movement unimplemented**: This session directly addresses Technical Debt #4
- [P02] **Standalone unit test files**: Use proven pattern of self-contained tests without server dependencies
- [P02] **NULL checks critical**: Ensure all pointer access is guarded

---

## Alternative Sessions

If this session is blocked:
1. **phase03-session04-dynamic-wilderness-rooms** - If wilderness room allocation is higher priority than interior movement (skip session 02-03 dependency chain)
2. **phase03-session05-vessel-type-system** - If per-vessel type mapping is critical path

---

## Next Steps

Run `/sessionspec` to generate the formal specification.
