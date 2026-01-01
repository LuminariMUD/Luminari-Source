# NEXT_SESSION.md

## Session Recommendation

**Generated**: 2025-12-30
**Project State**: Phase 02 - Simple Vehicle Support
**Completed Sessions**: 17

---

## Recommended Next Session

**Session ID**: `phase02-session02-vehicle-creation-system`
**Session Name**: Vehicle Creation System
**Estimated Duration**: 2-4 hours
**Estimated Tasks**: 15-20

---

## Why This Session Next?

### Prerequisites Met
- [x] Session 01 complete (vehicle data structures defined in vehicles.h)
- [x] MySQL/MariaDB database available (project requirement)
- [x] db_init.c patterns available for reference (established in Phase 00)

### Dependencies
- **Builds on**: phase02-session01-vehicle-data-structures (completed)
- **Enables**: phase02-session03-vehicle-movement-system, phase02-session04-vehicle-player-commands

### Project Progression
Session 02 is the natural next step after defining vehicle data structures. Without a creation and persistence system, no vehicles can exist in the game world. This session establishes the foundation that all subsequent Phase 02 sessions depend on:
- Movement (Session 03) requires vehicles to exist
- Player commands (Session 04) require vehicles to mount/dismount
- Vehicle-in-vehicle (Session 05) requires vehicles to load

---

## Session Overview

### Objective
Implement the vehicle creation, initialization, and registration system including database persistence for vehicle state.

### Key Deliverables
1. `create_vehicle()` function - Programmatic vehicle creation
2. `initialize_vehicle()` function - Default value assignment
3. Vehicle tracking array/list management - Support up to 1000 concurrent vehicles
4. Database schema (`vehicle_data` table) - Auto-created at startup
5. `save_vehicle()` and `load_vehicle()` functions - Persistence layer
6. Boot-time vehicle loading integration - Restore state on server restart

### Scope Summary
- **In Scope (MVP)**: Creation functions, tracking array, DB schema, persistence (save/load), boot integration
- **Out of Scope**: Movement logic, player commands, vehicle-in-vehicle, builder/OLC integration

---

## Technical Considerations

### Technologies/Patterns
- ANSI C90/C89 (no C99/C11 features)
- MySQL/MariaDB for persistence
- Auto-create tables at startup (proven pattern from Phase 00/01)
- Follow `vessels_db.c` persistence patterns

### Potential Challenges
- Memory management for vehicle array (target: <512 bytes per simple vehicle)
- NULL pointer guards throughout creation/destruction paths
- Database transaction safety for concurrent vehicle operations
- Ensuring Valgrind-clean memory handling

### Relevant Considerations
- [P01] **Auto-create DB tables at startup**: Schema matches runtime code exactly - use this proven pattern
- [P01] **Memory-efficient struct design**: Phase 01 achieved 48 bytes per autopilot struct - apply similar discipline
- [P01] **Standalone unit test files**: Create self-contained tests without server dependencies

---

## Alternative Sessions

If this session is blocked:
1. **phase02-session03-vehicle-movement-system** - Could prototype movement logic with mock vehicles, but requires refactoring later
2. **phase02-session06-unified-command-interface** - Could design command abstraction layer, but lacks concrete implementation targets

Neither alternative is recommended - Session 02 is the critical path.

---

## Next Steps

Run `/sessionspec` to generate the formal specification.
