# Session 01: Vehicle Data Structures

**Session ID**: `phase02-session01-vehicle-data-structures`
**Status**: Not Started
**Estimated Tasks**: ~15-20
**Estimated Duration**: 2-4 hours

---

## Objective

Define all vehicle data structures, constants, enums, and type definitions required for the simple vehicle system. Establish memory-efficient representations that support high concurrency.

---

## Scope

### In Scope (MVP)
- Vehicle type enum (VEHICLE_CART, VEHICLE_WAGON, VEHICLE_MOUNT, VEHICLE_CARRIAGE, etc.)
- Vehicle state enum (VEHICLE_STATE_IDLE, VEHICLE_STATE_MOVING, VEHICLE_STATE_LOADED)
- Core vehicle_data structure with essential fields
- Terrain capability flags for land vehicles
- Vehicle capacity/weight limit constants
- Header file organization (vehicles.h or extend vessels.h)

### Out of Scope
- Vehicle creation/initialization logic (Session 02)
- Movement implementation (Session 03)
- Player commands (Session 04)
- Database persistence schemas (Session 02)
- Vehicle-in-vehicle mechanics (Session 05)

---

## Prerequisites

- [ ] Phase 01 Automation Layer complete
- [ ] Review existing vessels.h for integration patterns
- [ ] Review CONSIDERATIONS.md for struct design lessons

---

## Deliverables

1. Vehicle type constants and enums in header file
2. Core vehicle_data structure (<512 bytes target)
3. Vehicle terrain capability structure
4. Vehicle state management constants
5. Capacity and weight limit definitions
6. Documentation comments for all structures

---

## Success Criteria

- [ ] All vehicle types defined (minimum: cart, wagon, mount, carriage)
- [ ] vehicle_data struct compiles without errors
- [ ] Struct size validated at <512 bytes
- [ ] No duplicate definitions (lesson from vessels.h)
- [ ] Header file follows ANSI C90/C89 standards
- [ ] Integration with existing vessel system considered
