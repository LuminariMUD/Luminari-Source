# PRD Phase 02: Simple Vehicle Support

**Status**: In Progress
**Sessions**: 7 (initial estimate)
**Estimated Duration**: 5-7 days

**Progress**: 3/7 sessions (43%)

---

## Overview

Implement a lightweight vehicle tier for LuminariMUD that provides simple, land-based transportation (CWG-style vehicles like carts, wagons, mounts) alongside the existing vessel system. This phase establishes vehicle-in-vehicle mechanics (loading vehicles onto ferries/transport ships) and creates a unified command interface that abstracts both vehicles and vessels under a common API.

---

## Progress Tracker

| Session | Name | Status | Est. Tasks | Validated |
|---------|------|--------|------------|-----------|
| 01 | Vehicle Data Structures | Complete | 18 | 2025-12-30 |
| 02 | Vehicle Creation System | Complete | 24 | 2025-12-30 |
| 03 | Vehicle Movement System | Complete | 20 | 2025-12-30 |
| 04 | Vehicle Player Commands | Not Started | ~15-20 | - |
| 05 | Vehicle-in-Vehicle Mechanics | Not Started | ~15-20 | - |
| 06 | Unified Command Interface | Not Started | ~15-20 | - |
| 07 | Testing and Validation | Not Started | ~20-25 | - |

---

## Completed Sessions

### Session 01: Vehicle Data Structures
- **Completed**: 2025-12-30
- **Tasks**: 18/18
- **Tests**: 19 passing, Valgrind clean
- **Summary**: Established foundational data structures for Simple Vehicle Support - vehicle_type enum (4 types), vehicle_state enum (5 states), VTERRAIN_* terrain flags (7 flags), capacity/speed constants, and memory-efficient vehicle_data struct (~136 bytes)

### Session 02: Vehicle Creation System
- **Completed**: 2025-12-30
- **Tasks**: 24/24
- **Tests**: 27 passing, Valgrind clean
- **Summary**: Implemented complete vehicle lifecycle system - vehicle_create/destroy/init, state management, passenger/weight capacity functions, condition tracking, lookup functions, MySQL persistence with auto-created tables, and server boot/shutdown integration

### Session 03: Vehicle Movement System
- **Completed**: 2025-12-30
- **Tasks**: 20/20
- **Tests**: 45 passing, Valgrind clean
- **Summary**: Implemented complete vehicle movement system - 8-direction navigation, SECT_* to VTERRAIN_* terrain mapping (37 sector types), terrain traversability validation, speed modifiers (road bonus, rough terrain penalty), wilderness room allocation, database persistence, and comprehensive movement validation

---

## Upcoming Sessions

- Session 04: Vehicle Player Commands

---

## Objectives

1. **Lightweight Vehicle Tier** - Create simple, low-overhead vehicle types for land-based transportation (carts, wagons, mounts, carriages)
2. **Vehicle-in-Vehicle Mechanics** - Enable loading smaller vehicles onto larger vessels (cars on ferries, wagons on transport ships)
3. **Unified Command Interface** - Abstract layer providing consistent commands across all transport types (vehicles and vessels)
4. **Memory Efficiency** - Maintain <512 bytes per simple vehicle to enable high concurrency
5. **Seamless Integration** - Vehicles work within existing wilderness coordinate system

---

## Prerequisites

- Phase 01 completed (Automation Layer)
- Wilderness coordinate system operational
- MySQL/MariaDB database available
- Vessel system functional

---

## Technical Considerations

### Architecture

The vehicle system will be implemented as a lightweight layer that shares infrastructure with the existing vessel system:

```
TRANSPORT ABSTRACTION LAYER
    |
    +-- Unified Command Interface (do_transport_*)
    |       |
    |       +-- Vehicle Commands (mount, dismount, drive)
    |       +-- Vessel Commands (board, disembark, sail)
    |
    +-- Vehicle System (vehicles.c/h)
    |       |
    |       +-- Simple vehicle types (cart, wagon, mount, carriage)
    |       +-- Single-room interiors (optional cargo space)
    |       +-- Land-based movement with terrain restrictions
    |
    +-- Vehicle-in-Vehicle (vehicle_loading.c)
            |
            +-- Loading/unloading mechanics
            +-- Capacity and weight limits
            +-- Nested transport tracking
```

### Technologies
- ANSI C90/C89 (no C99/C11 features)
- MySQL/MariaDB for persistence
- CuTest for unit testing
- Existing wilderness coordinate system

### Risks
- **Complexity creep**: Keep vehicles simple; defer advanced features to Phase 03
- **Memory overhead**: Target <512 bytes per vehicle; use bit fields and packed structures
- **Command conflicts**: Ensure vehicle commands don't conflict with existing vessel commands

### Relevant Considerations
<!-- From CONSIDERATIONS.md -->
- [P01] **Duplicate struct definitions in vessels.h**: Consider consolidating during vehicle struct design to avoid similar issues
- [P01] **Interior movement unimplemented**: Vehicles will use simpler single-room model, but may need shared movement helpers
- [P01] **Per-vessel type mapping missing**: Vehicle system should implement proper type mapping from start
- [P01] **Standalone unit test files**: Continue pattern of self-contained tests without server dependencies
- [P01] **Memory-efficient autopilot struct**: Apply same principles to vehicle structures

---

## Success Criteria

Phase complete when:
- [ ] All 7 sessions completed
- [ ] Vehicle types defined and functional (cart, wagon, mount, carriage minimum)
- [ ] Players can mount/dismount vehicles
- [ ] Vehicles navigate wilderness with terrain restrictions
- [ ] Vehicles can be loaded onto transport vessels
- [ ] Unified command interface operational
- [ ] Unit tests pass (target: 50+ tests, 100% pass rate)
- [ ] Memory usage <512 bytes per simple vehicle
- [ ] Valgrind clean (no memory leaks)

---

## Dependencies

### Depends On
- Phase 01: Automation Layer (complete)
- Wilderness coordinate system
- MySQL/MariaDB database
- Vessel system (vessels.c/h)

### Enables
- Phase 03: Optimization and Polish
- Future: Mount combat system
- Future: Caravan/trade route systems
