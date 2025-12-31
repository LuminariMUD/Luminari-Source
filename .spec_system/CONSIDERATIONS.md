# Considerations

> Institutional memory for AI assistants. Updated between phases via /carryforward.
> **Line budget**: 600 max | **Last updated**: Project Complete (2025-12-30)
> **Project Status**: COMPLETE - Vessel System Production Ready

---

## Active Concerns

Items requiring attention for future maintenance or enhancements.

### Technical Debt
<!-- Max 5 items -->

1. [P00] **Room templates hard-coded**: vessels_rooms.c uses 10 hard-coded templates instead of querying ship_room_templates DB table. Works correctly but limits runtime configurability.

### External Dependencies
<!-- Max 5 items -->

1. [P00] **MySQL/MariaDB required**: Server will not function without database connection. All vessel persistence depends on this.
2. [P00] **Wilderness system required**: Vessel navigation depends on wilderness coordinate system being operational.

### Performance / Security
<!-- Max 5 items -->

1. [P03] **Max 500 concurrent vessels validated**: Stress test passed at 100/250/500 vessels. Beyond this requires additional testing.
2. [P03] **Memory: 1016 bytes/vessel**: Within <1KB target. Monitor if adding features.
3. [P02] **Max 1000 concurrent vehicles validated**: Stress test passed at 100/500/1000 vehicles.
4. [P02] **Vehicle memory: 148 bytes/vehicle**: Well under 512-byte target.
5. [P00] **VNUM range 30000-40019 reserved**: Do not use for builder zones - reserved for dynamic wilderness rooms.

### Architecture
<!-- Max 5 items -->

1. [P03] **Test infrastructure uses Makefile**: CMake BUILD_TESTS disabled. Use `cd unittests/CuTest && make all && ./test_runner` for tests.
2. [P01] **Autopilot pulse frequency**: Currently runs every MUD pulse. May need throttling if server load increases with many active vessels.

---

## Lessons Learned

Proven patterns and anti-patterns. Reference during implementation.

### What Worked
<!-- Max 15 items -->

1. [P00] **Auto-create DB tables at startup**: Schema matches runtime code exactly. No migration headaches.
2. [P00] **Greyhawk system as foundation**: Most complete existing implementation provided solid base.
3. [P00] **Wilderness coordinate system**: Provides solid X/Y/Z navigation framework.
4. [P00] **Standalone unit test files**: Self-contained tests without server dependencies enable rapid iteration.
5. [P00] **Incremental session approach**: Each session builds cleanly on prior work. Small validated steps > big risky leaps.
6. [P01] **Valgrind with suppression file**: cutest.supp filters CuTest framework leaks, validates real code accurately.
7. [P01] **Memory-efficient structs**: 48-byte autopilot struct, 148-byte vehicle struct enable scaling. Design for minimal memory first.
8. [P01] **In-memory cache for waypoints/routes**: Fast lookups without DB round-trips. Cache frequently accessed data.
9. [P02] **Unified transport interface**: transport_data struct abstracts vessel/vehicle differences cleanly.
10. [P02] **Vehicle-in-vessel mechanics**: Clean layered transport with nesting limits (MAX_TRANSPORT_DEPTH=3).
11. [P02] **POSIX macro for C89 compatibility**: `_POSIX_C_SOURCE 200809L` enables snprintf/strcasecmp in C89.
12. [P03] **Validation sessions are valuable**: Sessions 02-03 discovered prior work was already complete. Always verify before implementing.
13. [P03] **Mock-based unit tests**: Created 14 mock tests for wilderness room allocation without full server dependencies.
14. [P03] **Integration tests catch gaps**: 11 end-to-end vessel type tests caught issues unit tests missed.
15. [P03] **Comprehensive documentation**: 353 tests, 3 docs (895 lines total), troubleshooting guide - invest in docs.

### What to Avoid
<!-- Max 10 items -->

1. [P00] **Don't use C99/C11 features**: This is ANSI C90/C89 codebase. No `//` comments, no declarations after statements.
2. [P00] **Don't hardcode VNUMs**: Use #defines or configuration. VNUMs in code = future breakage.
3. [P00] **Don't skip NULL checks**: Critical in C code. Every pointer dereference needs validation.
4. [P00] **Don't commit environment-specific files**: campaign.h, mud_options.h, vnums.h are local only.
5. [P03] **Don't assume code is unimplemented**: Phase 03 Sessions 02-03 found features already done. Verify state first.
6. [P03] **Don't duplicate definitions in header conditionals**: Caused Tech Debt #1-2. One canonical definition per struct/constant.

### Tool/Library Notes
<!-- Max 5 items -->

1. [P00] **CuTest for unit testing**: Located in unittests/CuTest/. Simple, reliable, C89 compatible.
2. [P00] **Valgrind for memory validation**: Run tests with `--leak-check=full`. Use cutest.supp suppression file.
3. [P03] **CMake vs Makefile tests**: CMake cutest target broken (legacy linking issues). Use Makefile for all tests.

---

## Resolved

Recently closed items (project completion - preserving final resolution state).

| Phase | Item | Resolution |
|-------|------|------------|
| P03 | Final testing/documentation | 353 tests pass (64% over target), Valgrind clean, docs complete |
| P03 | Per-vessel type mapping | 104 tests validate all 8 types fully implemented |
| P03 | Dynamic wilderness rooms | Centralized via get_or_allocate_wilderness_room() pattern |
| P03 | Interior movement | Verified complete in vessels_rooms.c (Phase 00 Session 06) |
| P03 | Code consolidation | GREYHAWK_ITEM_SHIP=56, 12 duplicates removed |
| P03 | Phase 2 commands registered | Verified in interpreter.c:4938-4969 |
| P02 | Unified transport interface | transport_data abstraction complete |
| P02 | Vehicle-in-vehicle mechanics | Nesting with MAX_TRANSPORT_DEPTH=3 |
| P02 | 151+ vehicle tests | All passing, Valgrind clean |
| P01 | Autopilot/waypoint system | In-memory cache, 84 tests passing |
| P01 | NPC pilot integration | Scheduled routes functional |
| P00 | Core vessel system | Foundation complete, all systems wired |

---

## Project Completion Summary

**LuminariMUD Vessel System** - Completed 2025-12-30

### Final Metrics
- **Total Phases**: 4 (Phase 00-03)
- **Total Sessions**: 29
- **Total Tests**: 353 (target was 215+)
- **Valgrind Status**: Clean (0 bytes, 0 errors)
- **Memory per Vessel**: 1016 bytes
- **Memory per Vehicle**: 148 bytes
- **Max Concurrent Vessels**: 500 (validated)
- **Max Concurrent Vehicles**: 1000 (validated)

### Documentation Created
- docs/project-management-zusuk/vessels/VESSEL_SYSTEM.md
- docs/project-management-zusuk/vessels/VESSEL_BENCHMARKS.md

### Future Enhancement Opportunities
1. Ship-to-ship combat system
2. Weather effects on vessel performance
3. Crew management and morale
4. Trade route economics
5. Naval warfare mechanics

---

*Auto-generated by /carryforward. Project complete - document preserved for future maintenance.*
