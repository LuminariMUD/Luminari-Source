# NEXT_SESSION.md

## Session Recommendation

**Generated**: 2025-12-29
**Project State**: Phase 00 - Core Vessel System
**Completed Sessions**: 0

---

## Recommended Next Session

**Session ID**: `phase00-session01-header-cleanup-foundation`
**Session Name**: Header Cleanup & Foundation
**Estimated Duration**: 2-3 hours
**Estimated Tasks**: 15-20

---

## Why This Session Next?

### Prerequisites Met
- [x] Build system working (CMake or autotools available)
- [x] Ability to compile and test changes

### Dependencies
- **Builds on**: Nothing - this is the foundation session
- **Enables**: All subsequent sessions (sessions 02-09)

### Project Progression

This is the **mandatory first session** for Phase 00. The PRD documents significant technical debt in `vessels.h` - duplicate struct definitions and constant declarations that must be resolved before any new functionality can be safely added. Attempting to work on later sessions without this cleanup would:

1. Risk compilation conflicts when adding new code
2. Create confusion about which definitions are authoritative
3. Make subsequent debugging significantly harder

The header cleanup establishes a clean, maintainable foundation that all other sessions depend on.

---

## Session Overview

### Objective
Resolve duplicate struct definitions and constants in vessels.h to establish a clean, maintainable codebase foundation for subsequent sessions.

### Key Deliverables
1. Clean `vessels.h` with no duplicate definitions
2. All existing code compiles without errors
3. No new warnings introduced
4. Basic smoke test passes (server starts)

### Scope Summary
- **In Scope (MVP)**: Fix duplicate struct definitions (greyhawk_ship_slot, greyhawk_ship_crew, greyhawk_ship_data), fix duplicate constants (VESSEL_STATE_*, VESSEL_SIZE_*), verify build and runtime
- **Out of Scope**: Adding new functionality, refactoring beyond duplicate removal, performance optimization

---

## Technical Considerations

### Technologies/Patterns
- ANSI C90/C89 (no C99/C11 features)
- Header file organization best practices
- Incremental compilation verification

### Potential Challenges
- Determining which duplicate definition is more complete/correct
- Ensuring no code depends on specific line numbers in headers
- Maintaining binary compatibility with any existing saved data

### Relevant Considerations
- [P00] **Duplicate struct definitions in vessels.h**: Lines 321-405 vs 498-612 - greyhawk_ship_slot, greyhawk_ship_crew, greyhawk_ship_data need consolidation
- [P00] **VESSEL_STATE constants duplicated**: Lines 50-58 vs 89-97 - need to remove one set
- [P00] **Don't use C99/C11 features**: Keep changes compatible with ANSI C90/C89

---

## Alternative Sessions

If this session is blocked:
1. **Session 02 (Dynamic Wilderness Room Allocation)** - Could theoretically start in parallel, but would risk conflicts with header changes
2. **Session 03 (Vessel Type System)** - Depends on clean headers, not recommended before cleanup

**Strong recommendation**: Do not skip Session 01. Technical debt cleanup is foundational.

---

## Next Steps

Run `/sessionspec` to generate the formal specification.
