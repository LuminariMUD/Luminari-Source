# PRD Phase 03: Optimization & Polish

**Status**: In Progress
**Sessions**: 6 (initial estimate)
**Estimated Duration**: 2-3 days

**Progress**: 4/6 sessions (67%)

---

## Overview

Complete the vessel system by addressing remaining technical debt, implementing missing functionality, optimizing performance, and ensuring comprehensive test coverage. This phase focuses on code quality, stability, and production readiness.

---

## Progress Tracker

| Session | Name | Status | Est. Tasks | Validated |
|---------|------|--------|------------|-----------|
| 01 | Code Consolidation | Complete | 18 | 2025-12-30 |
| 02 | Interior Movement Implementation | Complete | 16 | 2025-12-30 |
| 03 | Command Registration & Wiring | Complete | 0 | 2025-12-30 |
| 04 | Dynamic Wilderness Rooms | Complete | 20 | 2025-12-30 |
| 05 | Vessel Type System | Not Started | ~14-18 | - |
| 06 | Final Testing & Documentation | Not Started | ~20-25 | - |

---

## Completed Sessions

### Session 01: Code Consolidation (2025-12-30)

Consolidated duplicate constant definitions in vessels.h. Resolved GREYHAWK_ITEM_SHIP conflict (standardized on value 56), removed 12 duplicate macro definitions from conditional blocks, and verified all 326 unit tests passing.

### Session 02: Interior Movement Implementation (2025-12-30)

Validation session confirming Phase 00 Session 06 implementation is complete. Verified all three core interior movement functions (do_move_ship_interior, get_ship_exit, is_passage_blocked) are fully implemented with comprehensive NULL checks and bidirectional connection matching. All 91 unit tests passing (18 movement-specific). Valgrind clean. Removed stale Technical Debt #4 from CONSIDERATIONS.md.

### Session 03: Command Registration & Wiring (2025-12-30)

Validation session confirming all Phase 2 command registration and wiring was completed in earlier Phase 00 sessions. Verified: dock (interpreter.c:4938), undock (interpreter.c:4939), board_hostile (interpreter.c:4940-4949), look_outside (interpreter.c:4950-4959), ship_rooms (interpreter.c:4960-4969). Interior generation wiring (vessels_src.c:2392-2401), persistence (vessels_db.c:580,612), and weather integration (vessels_docking.c:713) all confirmed functional. Zero tasks required - all objectives pre-implemented.

### Session 04: Dynamic Wilderness Rooms (2025-12-30)

Fixed vessel wilderness movement to use centralized room allocation. Modified `move_vessel_toward_waypoint()` to call `update_ship_wilderness_position()` instead of direct manipulation, ensuring proper room allocation via `find_available_wilderness_room()`. Added departure logging for debugging room cleanup. Created 14 comprehensive mock-based unit tests covering room pool initialization, allocation/release cycles, multi-vessel coordination, and boundary conditions. All tests Valgrind clean (32 allocs, 32 frees).

---

## Upcoming Sessions

- Session 05: Vessel Type System
- Session 06: Final Testing & Documentation

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
