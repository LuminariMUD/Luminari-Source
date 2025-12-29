# Session Specification

**Session ID**: `phase00-session09-testing-validation`
**Phase**: 00 - Core Vessel System
**Status**: Not Started
**Created**: 2025-12-29

---

## 1. Session Overview

This is the final session of Phase 00, dedicated to comprehensive testing and validation of the vessel system implemented across sessions 01-08. The session focuses on creating a robust unit test suite, validating memory safety with Valgrind, profiling performance characteristics, and conducting stress tests to ensure the system meets PRD requirements.

The vessel system is now feature-complete with header cleanup, dynamic wilderness room allocation, vessel type definitions, command registration, interior room generation, interior movement, persistence integration, and external view displays. This session validates that all components work correctly together and meet the performance targets: <100ms command response, <1KB memory per vessel, and stable operation with 500 concurrent vessels.

Testing is critical before declaring Phase 00 complete and transitioning to Phase 01 (Automation Layer). Any issues discovered will be documented as technical debt for future remediation rather than fixed in this session, keeping scope bounded.

---

## 2. Objectives

1. Create comprehensive unit test suite covering all critical vessel functions (coordinate system, types, room generation, movement, persistence)
2. Validate memory safety with Valgrind (zero leaks, zero invalid accesses)
3. Profile performance of key operations (room generation <100ms, command response <100ms)
4. Conduct stress tests at 100/250/500 concurrent vessels validating stability and memory targets

---

## 3. Prerequisites

### Required Sessions
- [x] `phase00-session01-header-cleanup-foundation` - Clean header structure
- [x] `phase00-session02-dynamic-wilderness-room-allocation` - Room allocation system
- [x] `phase00-session03-vessel-type-system` - Type definitions and capabilities
- [x] `phase00-session04-phase2-command-registration` - Command infrastructure
- [x] `phase00-session05-interior-room-generation-wiring` - Interior room system
- [x] `phase00-session06-interior-movement-implementation` - Movement system
- [x] `phase00-session07-persistence-integration` - Save/load functionality
- [x] `phase00-session08-external-view-display-systems` - View and display systems

### Required Tools/Knowledge
- CuTest unit test framework (unittests/CuTest/)
- Valgrind memory analysis tool
- Performance profiling tools (gprof/perf)
- Understanding of vessel system architecture

### Environment Requirements
- GCC or Clang compiler with -Wall -Wextra
- Valgrind installed and functional
- MySQL/MariaDB running (for integration tests)
- Sufficient memory for 500-vessel stress tests

---

## 4. Scope

### In Scope (MVP)
- Unit tests for coordinate system boundary conditions
- Unit tests for vessel type capabilities and terrain validation
- Unit tests for room generation VNUM allocation
- Unit tests for interior and wilderness movement
- Unit tests for persistence save/load cycle
- Valgrind memory validation of unit tests
- Performance profiling of room generation
- Performance profiling of docking operations
- Stress test with 100 concurrent vessels
- Stress test with 250 concurrent vessels
- Stress test with 500 concurrent vessels
- Documentation of all test results and issues found

### Out of Scope (Deferred)
- Fixing issues found - *Reason: Creates unbounded scope; document as TODOs instead*
- CI/CD integration - *Reason: Phase 01+ task, requires infrastructure setup*
- Long-duration soak testing - *Reason: Requires hours/days of runtime; document procedure only*
- Full server Valgrind analysis - *Reason: Too large; focus on vessel-specific paths*

---

## 5. Technical Approach

### Architecture
Tests will be organized into logical groupings matching the vessel subsystems. Each test file focuses on a specific aspect of the vessel system, using mock objects where necessary to isolate functionality from database dependencies.

The stress testing approach creates vessels programmatically in memory, simulates command execution, and measures resource consumption at scale. This validates the system without requiring a full server deployment.

### Design Patterns
- **Arrange-Act-Assert**: Standard CuTest pattern for all unit tests
- **Test Fixtures**: Shared setup/teardown for vessel test data
- **Mock Objects**: Stub database calls for unit test isolation
- **Incremental Stress**: 100 -> 250 -> 500 vessel progression

### Technology Stack
- CuTest framework (existing in unittests/CuTest/)
- Valgrind 3.x with --leak-check=full --track-origins=yes
- gprof for function-level profiling
- Custom stress test harness in C

---

## 6. Deliverables

### Files to Create
| File | Purpose | Est. Lines |
|------|---------|------------|
| `unittests/CuTest/test_vessels.c` | Main vessel unit test suite | ~400 |
| `unittests/CuTest/test_vessel_coords.c` | Coordinate system tests | ~150 |
| `unittests/CuTest/test_vessel_types.c` | Vessel type validation tests | ~150 |
| `unittests/CuTest/test_vessel_rooms.c` | Room generation tests | ~150 |
| `unittests/CuTest/test_vessel_movement.c` | Movement system tests | ~150 |
| `unittests/CuTest/test_vessel_persistence.c` | Save/load cycle tests | ~150 |
| `unittests/CuTest/vessel_stress_test.c` | Stress test harness | ~200 |
| `docs/testing/vessel_test_results.md` | Test results documentation | ~200 |

### Files to Modify
| File | Changes | Est. Lines |
|------|---------|------------|
| `unittests/CuTest/Makefile` | Add vessel test targets | ~30 |
| `unittests/CuTest/AllTests.c` | Register vessel test suites | ~20 |

---

## 7. Success Criteria

### Functional Requirements
- [ ] Unit tests cover coordinate boundary conditions (-1024, 0, +1024)
- [ ] Unit tests validate vessel type terrain capabilities
- [ ] Unit tests verify VNUM allocation correctness
- [ ] Unit tests confirm room connectivity
- [ ] Unit tests validate interior movement
- [ ] Unit tests verify save/load data integrity
- [ ] All unit tests pass (100% pass rate)

### Testing Requirements
- [ ] Unit tests written and passing
- [ ] Valgrind reports zero memory leaks
- [ ] Valgrind reports zero invalid memory accesses
- [ ] Performance profiling completed for room generation
- [ ] Performance profiling completed for docking operations
- [ ] Stress tests completed at 100/250/500 vessels

### Quality Gates
- [ ] All files ASCII-encoded (0-127 characters only)
- [ ] Unix LF line endings
- [ ] Code follows ANSI C90 conventions (no // comments, declarations at block start)
- [ ] Two-space indentation, Allman-style braces
- [ ] All tests compile with -Wall -Wextra -Werror

---

## 8. Implementation Notes

### Key Considerations
- Tests must not require full server startup (isolate vessel functions)
- Mock database calls to avoid MySQL dependency in unit tests
- Stress tests can use simplified vessel structs for memory measurement
- Document any issues found rather than fixing them (scope control)

### Potential Challenges
- **Isolating vessel functions from server**: Use function pointers or stubs for database calls
- **Measuring accurate per-vessel memory**: Use Valgrind massif or custom tracking
- **Simulating 500 vessels without server**: Create minimal vessel structs in test harness
- **Test fixture complexity**: Keep fixtures minimal; test one thing at a time

### Relevant Considerations
- [P00] **Max 500 concurrent vessels target**: Primary validation target for stress tests; must achieve stable operation
- [P00] **Memory target: <1KB per vessel**: Validate with Valgrind massif; document actual usage
- [P00] **CuTest for unit testing**: Use established framework patterns from existing tests
- [P00] **Valgrind for memory leak detection**: Run with --leak-check=full --track-origins=yes on all test binaries

### ASCII Reminder
All output files must use ASCII-only characters (0-127). Use /* */ comments only, no Unicode characters in strings or comments.

---

## 9. Testing Strategy

### Unit Tests
- Coordinate system: boundary values, invalid inputs, overflow handling
- Vessel types: capability flags, terrain restrictions, speed modifiers
- Room generation: VNUM allocation, room linking, template application
- Movement: valid/invalid directions, blocked passages, elevation changes
- Persistence: save cycle, load cycle, data corruption handling

### Integration Tests
- Full vessel lifecycle: create -> move -> dock -> save -> load -> destroy
- Multi-vessel scenarios: collisions, shared docking locations

### Manual Testing
- Create vessel in-game, perform all commands, verify behavior
- Monitor syslog for errors during test execution
- Verify database state after persistence operations

### Edge Cases
- Vessel at coordinate boundaries (-1024, +1024)
- Maximum room count per vessel (16 rooms)
- Vessel with no crew
- Vessel at invalid terrain type
- Corrupted persistence data handling

---

## 10. Dependencies

### External Libraries
- CuTest: 1.5+ (bundled in unittests/CuTest/)
- Valgrind: 3.x
- MySQL/MariaDB: 10.x (for integration tests only)

### Other Sessions
- **Depends on**: Sessions 01-08 (all Phase 00 implementation complete)
- **Depended by**: Phase 01 sessions (must pass before Phase 01 begins)

---

## Performance Targets

| Metric | Target | Measurement Method |
|--------|--------|-------------------|
| Max concurrent vessels | 500 | Stress test stability |
| Command response time | <100ms | gprof timing |
| Memory per simple vessel | <1KB | Valgrind massif |
| Memory (100 ships) | <5MB | Process memory monitoring |
| Memory (500 ships) | <25MB | Process memory monitoring |
| Code test coverage | >90% | Line count analysis |

---

## Next Steps

Run `/tasks` to generate the implementation task checklist.
