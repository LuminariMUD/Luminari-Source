# Session 09: Testing & Validation

**Session ID**: `phase00-session09-testing-validation`
**Status**: Not Started
**Estimated Tasks**: ~15-20
**Estimated Duration**: 3-4 hours

---

## Objective

Create comprehensive unit tests, validate memory usage, profile performance, and conduct stress testing to ensure the vessel system meets all technical requirements.

---

## Scope

### In Scope (MVP)
- Create unit tests in unittests/CuTest for vessel functions
- Run Valgrind memory validation
- Profile room generation performance
- Profile docking operations performance
- Stress test with 100 concurrent vessels
- Stress test with 250 concurrent vessels
- Stress test with 500 concurrent vessels
- Document test results and any issues found

### Out of Scope
- Fixing issues found (create tickets/TODOs)
- Automated CI/CD integration
- Long-duration soak testing

---

## Prerequisites

- [ ] Sessions 01-08 completed
- [ ] All vessel features functional
- [ ] CuTest framework available
- [ ] Valgrind installed

---

## Deliverables

1. Unit test suite for vessel system
2. Memory validation report (Valgrind)
3. Performance profiling results
4. Stress test results (100/250/500 vessels)
5. List of any issues found

---

## Success Criteria

- [ ] Unit tests cover critical vessel functions
- [ ] All unit tests pass
- [ ] Valgrind reports no memory leaks
- [ ] Valgrind reports no invalid memory access
- [ ] Room generation < 100ms per ship
- [ ] Command response time < 100ms
- [ ] Memory per vessel < 1KB (simple)
- [ ] 100 vessels: stable operation
- [ ] 250 vessels: stable operation
- [ ] 500 vessels: stable operation (target max)
- [ ] Memory usage < 25MB at 500 vessels

---

## Technical Notes

### Test Targets (from PRD Section 9)

| Metric | Target |
|--------|--------|
| Max concurrent vessels | 500 |
| Command response time | <100ms |
| Memory per simple vessel | <1KB |
| Memory (100 ships) | <5MB |
| Memory (500 ships) | <25MB |
| Code test coverage | >90% |

### Unit Test Areas

1. **Coordinate System**
   - Boundary tests (-1024, 0, +1024)
   - Invalid coordinate handling

2. **Vessel Types**
   - Terrain capability validation
   - Speed modifier calculations

3. **Room Generation**
   - VNUM allocation correctness
   - Room connectivity verification

4. **Movement**
   - Interior movement validation
   - Wilderness movement validation

5. **Persistence**
   - Save/load cycle verification
   - Data integrity checks

### Valgrind Commands

```bash
# Basic memory check
valgrind --leak-check=full ./bin/circle -d lib

# Unit test memory check
cd unittests/CuTest
valgrind --leak-check=full --track-origins=yes ./test_runner
```

### Performance Profiling

```bash
# Use gprof or perf
# Profile specific operations:
# - Ship creation
# - Room generation
# - Movement operations
# - Docking operations
```

### Stress Test Procedure

1. Create N vessels programmatically
2. Issue movement commands to all
3. Monitor memory usage
4. Measure response times
5. Check for degradation over time

### Files to Create/Modify

- unittests/CuTest/test_vessels.c - New test file
- unittests/CuTest/Makefile - Add vessel tests
- docs/testing/ - Test result documentation
