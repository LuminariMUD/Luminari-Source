# NEXT_SESSION.md

## Session Recommendation

**Generated**: 2025-12-30
**Project State**: Phase 03 - Optimization & Polish
**Completed Sessions**: 23

---

## Recommended Next Session

**Session ID**: `phase03-session01-code-consolidation`
**Session Name**: Code Consolidation
**Estimated Duration**: 2-4 hours
**Estimated Tasks**: 15-20

---

## Why This Session Next?

### Prerequisites Met
- [x] Phase 02 complete and validated (7 sessions completed 2025-12-30)
- [x] All existing tests passing (159 unit tests, 100% pass rate)
- [x] Clean foundation from phases 00-02

### Dependencies
- **Builds on**: Phase 02 completion (vehicle system + unified transport)
- **Enables**: Session 02 (interior movement), Session 03 (command registration), and all subsequent Phase 03 sessions

### Project Progression
This is the foundational cleanup session for Phase 03. Code consolidation must happen first because:

1. **Technical Debt Resolution**: Directly addresses the #1 and #2 items in CONSIDERATIONS.md (duplicate structs and constants in vessels.h)
2. **Clean Foundation**: All subsequent sessions build on this cleanup - implementing interior movement or registering commands on top of duplicate definitions creates confusion and maintenance burden
3. **No External Dependencies**: Purely internal refactoring with no new features or external system changes
4. **Low Risk**: Consolidation is safer when done before adding new code rather than after

---

## Session Overview

### Objective
Eliminate duplicate struct definitions, constant declarations, and other code redundancies in vessels.h and related files to establish a clean, maintainable codebase foundation.

### Key Deliverables
1. Consolidated vessels.h with single struct definitions
2. Consolidated constant definitions (no duplicates)
3. Updated dependent source files (vessels.c, vessels_rooms.c, vessels_docking.c)
4. All existing tests still passing (159 tests)
5. Valgrind clean verification

### Scope Summary
- **In Scope (MVP)**: Remove duplicate greyhawk_ship_slot, greyhawk_ship_crew, greyhawk_ship_data structs; consolidate VESSEL_STATE_* and VESSEL_SIZE_* constants; update all references
- **Out of Scope**: New features, performance optimization, database changes, interior movement (Session 02)

---

## Technical Considerations

### Known Duplicates to Resolve
- `greyhawk_ship_slot` struct: lines 321-405 vs 498-612
- `greyhawk_ship_crew` struct: duplicate definition
- `greyhawk_ship_data` struct: duplicate definition
- `VESSEL_STATE_*` constants: lines 50-58 vs 89-97
- `VESSEL_SIZE_*` constants: duplicated

### Approach
1. Identify canonical location for each definition
2. Remove duplicate definitions
3. Update all references in dependent files
4. Verify compilation after each change
5. Run full test suite to confirm no regressions

### Potential Challenges
- Hidden dependencies on specific definition locations
- Subtle differences between duplicate definitions
- Ensuring all dependent files are updated

### Relevant Considerations
- **[P01]** **Duplicate struct definitions in vessels.h**: Primary target for this session
- **[P01]** **VESSEL_STATE constants duplicated**: Secondary target for this session
- **[P01]** **Standalone unit test files pattern**: Use existing test infrastructure for validation

---

## Alternative Sessions

If this session is blocked:
1. **phase03-session03-command-registration-wiring** - Could technically proceed but would build on unclean foundation
2. **phase03-session04-dynamic-wilderness-rooms** - Independent enough to start, but violates dependency order

**Recommendation**: Do not skip Session 01. Code consolidation is low-risk foundational work that de-risks all subsequent sessions.

---

## Next Steps

Run `/sessionspec` to generate the formal specification.
