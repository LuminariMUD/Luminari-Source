# Session 01: Header Cleanup & Foundation

**Session ID**: `phase00-session01-header-cleanup-foundation`
**Status**: Not Started
**Estimated Tasks**: ~15-20
**Estimated Duration**: 2-3 hours

---

## Objective

Resolve duplicate struct definitions and constants in vessels.h to establish a clean, maintainable codebase foundation for subsequent sessions.

---

## Scope

### In Scope (MVP)
- Fix duplicate greyhawk_ship_slot struct definition (lines 321-405 vs 498-612)
- Fix duplicate greyhawk_ship_crew struct definition
- Fix duplicate greyhawk_ship_data struct definition
- Fix duplicate VESSEL_STATE_* constants (lines 50-58 vs 89-97)
- Fix duplicate VESSEL_SIZE_* constants
- Verify build compiles cleanly after changes
- Verify existing functionality still works

### Out of Scope
- Adding new functionality
- Refactoring beyond duplicate removal
- Performance optimization

---

## Prerequisites

- [ ] Build system working (CMake or autotools)
- [ ] Ability to compile and test changes

---

## Deliverables

1. Clean vessels.h with no duplicate definitions
2. All existing code compiles without errors
3. No new warnings introduced
4. Basic smoke test passes (server starts)

---

## Success Criteria

- [ ] No duplicate struct definitions in vessels.h
- [ ] No duplicate constant definitions in vessels.h
- [ ] Code compiles with zero errors
- [ ] No new compiler warnings
- [ ] Server starts successfully
- [ ] Existing vessel commands still functional

---

## Technical Notes

### Duplicate Locations (from CONSIDERATIONS.md)

**Struct duplicates:**
- greyhawk_ship_slot: lines 321-405 AND 498-612
- greyhawk_ship_crew: same regions
- greyhawk_ship_data: same regions

**Constant duplicates:**
- VESSEL_STATE_TRAVELING, VESSEL_STATE_COMBAT, VESSEL_STATE_DAMAGED: lines 50-58 AND 89-97
- VESSEL_SIZE_*: same regions

### Resolution Strategy

1. Identify which definition is more complete/correct
2. Remove the duplicate, keeping the better one
3. Update any code that may reference specific line numbers
4. Test compilation
5. Smoke test runtime
