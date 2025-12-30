# PRD Phase 03: Optimization & Polish

**Status**: In Progress
**Sessions**: 6 (initial estimate)
**Estimated Duration**: 2-3 days

**Progress**: 1/6 sessions (17%)

---

## Overview

Complete the vessel system by addressing remaining technical debt, implementing missing functionality, optimizing performance, and ensuring comprehensive test coverage. This phase focuses on code quality, stability, and production readiness.

---

## Progress Tracker

| Session | Name | Status | Est. Tasks | Validated |
|---------|------|--------|------------|-----------|
| 01 | Code Consolidation | Complete | 18 | 2025-12-30 |
| 02 | Interior Movement Implementation | Not Started | ~18-22 | - |
| 03 | Command Registration & Wiring | Not Started | ~15-18 | - |
| 04 | Dynamic Wilderness Rooms | Not Started | ~16-20 | - |
| 05 | Vessel Type System | Not Started | ~14-18 | - |
| 06 | Final Testing & Documentation | Not Started | ~20-25 | - |

---

## Completed Sessions

### Session 01: Code Consolidation (2025-12-30)

Consolidated duplicate constant definitions in vessels.h. Resolved GREYHAWK_ITEM_SHIP conflict (standardized on value 56), removed 12 duplicate macro definitions from conditional blocks, and verified all 326 unit tests passing.

---

## Upcoming Sessions

- Session 02: Interior Movement Implementation

---

## Objectives

1. **Code Quality** - Eliminate duplicate definitions, consolidate structures, clean up technical debt
2. **Feature Completion** - Implement interior movement, register Phase 2 commands, complete vessel type mapping
3. **Performance** - Optimize memory usage, query efficiency, and runtime performance for 500+ vessels
4. **Production Readiness** - Achieve >90% test coverage, complete documentation, validate all systems

---

## Prerequisites

- Phase 02 completed (Simple Vehicle Support validated)
- All 159 unit tests passing
- Valgrind clean memory state

---

## Technical Considerations

### Architecture

The vessel system is feature-complete at the API level but has several incomplete implementations and code quality issues that must be addressed before production deployment.

**Key Areas:**
- vessels.h has duplicate struct definitions that create maintenance burden
- Interior movement functions declared but not implemented
- Phase 2 commands exist but aren't registered in interpreter.c
- Dynamic room allocation pattern not integrated with vessel movement

### Technologies

- ANSI C90/C89 with GNU extensions
- MySQL/MariaDB for persistence
- CuTest for unit testing
- Valgrind for memory validation

### Risks

- **Struct consolidation may break dependent code**: Mitigation - careful refactoring with comprehensive testing
- **Interior movement complexity**: Mitigation - incremental implementation with unit tests
- **Performance regression during optimization**: Mitigation - benchmark before/after each change

### Relevant Considerations

From CONSIDERATIONS.md - items being addressed in this phase:

- [P00-P02] **Duplicate struct definitions in vessels.h**: Lines 321-405 and 498-612 have greyhawk_ship_slot, greyhawk_ship_crew, greyhawk_ship_data defined twice
- [P00-P02] **VESSEL_STATE constants duplicated**: Lines 50-58 and 89-97 define same constants
- [P00] **Room templates hard-coded**: vessels_rooms.c uses 10 hard-coded templates instead of DB
- [P00] **Interior movement unimplemented**: do_move_ship_interior(), get_ship_exit(), is_passage_blocked() declared but empty
- [P00] **Vessel movement fails silently**: Needs dynamic room allocation using find_available_wilderness_room() pattern
- [P00] **Per-vessel type mapping missing**: Hardcoded VESSEL_TYPE_SAILING_SHIP placeholder
- [P00] **Phase 2 commands not registered**: dock, undock, board_hostile, look_outside, ship_rooms

---

## Success Criteria

Phase complete when:
- [ ] All 6 sessions completed
- [ ] No duplicate struct/constant definitions remain
- [ ] All interior movement functions implemented and tested
- [ ] All Phase 2 commands registered and functional
- [ ] Dynamic wilderness room allocation working
- [ ] Per-vessel type system complete
- [ ] Test coverage >90% (target: 200+ tests)
- [ ] Valgrind clean across all test suites
- [ ] Documentation updated and complete

---

## Dependencies

### Depends On

- Phase 02: Simple Vehicle Support (complete)
- Wilderness system operational
- MySQL/MariaDB connection available

### Enables

- Production deployment of vessel system
- Ship-to-ship combat (future phase)
- Advanced vessel features (future phase)
