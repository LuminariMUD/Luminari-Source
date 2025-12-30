# Session 01: Code Consolidation

**Session ID**: `phase03-session01-code-consolidation`
**Status**: Not Started
**Estimated Tasks**: ~15-20
**Estimated Duration**: 2-4 hours

---

## Objective

Eliminate duplicate struct definitions, constant declarations, and other code redundancies in vessels.h and related files to establish a clean, maintainable codebase foundation.

---

## Scope

### In Scope (MVP)

- Remove duplicate greyhawk_ship_slot struct definition (lines 321-405 vs 498-612)
- Remove duplicate greyhawk_ship_crew struct definition
- Remove duplicate greyhawk_ship_data struct definition
- Consolidate VESSEL_STATE_* constants (lines 50-58 vs 89-97)
- Consolidate VESSEL_SIZE_* constants
- Update all references to use single canonical definitions
- Verify compilation after consolidation
- Update any dependent code in vessels.c, vessels_rooms.c, vessels_docking.c

### Out of Scope

- New feature implementation
- Performance optimization
- Database schema changes
- Interior movement implementation (Session 02)

---

## Prerequisites

- [ ] Phase 02 complete and validated
- [ ] All existing tests passing
- [ ] Clean git working state

---

## Deliverables

1. Consolidated vessels.h with single struct definitions
2. Consolidated constant definitions (no duplicates)
3. Updated dependent source files
4. All existing tests still passing
5. Valgrind clean verification

---

## Success Criteria

- [ ] No duplicate struct definitions in vessels.h
- [ ] No duplicate constant definitions in vessels.h
- [ ] All vessel-related code compiles without errors
- [ ] All 159 existing unit tests pass
- [ ] Valgrind reports no new memory issues
- [ ] Code follows project style guidelines
