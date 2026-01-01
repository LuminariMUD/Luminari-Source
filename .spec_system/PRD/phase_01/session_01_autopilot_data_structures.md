# Session 01: Autopilot Data Structures

**Session ID**: `phase01-session01-autopilot-data-structures`
**Status**: Not Started
**Estimated Tasks**: ~15-20
**Estimated Duration**: 2-4 hours

---

## Objective

Define and implement the core data structures, constants, and header declarations required for the vessel autopilot system.

---

## Scope

### In Scope (MVP)
- Define autopilot state structure (`struct autopilot_data`)
- Define waypoint structure (`struct waypoint`)
- Define route structure (`struct ship_route`)
- Add autopilot constants (max waypoints, timing intervals, etc.)
- Extend `greyhawk_ship_data` to include autopilot state
- Add function prototypes for autopilot operations
- Create `vessels_autopilot.c` source file skeleton

### Out of Scope
- Actual autopilot movement logic (Session 03)
- Database persistence (Session 02)
- Player commands (Session 04)
- NPC integration (Session 05)

---

## Prerequisites

- [ ] Phase 00 complete and validated
- [ ] vessels.h readable and modifiable
- [ ] Understanding of existing greyhawk_ship_data structure

---

## Deliverables

1. Updated `src/vessels.h` with autopilot structures and prototypes
2. New `src/vessels_autopilot.c` skeleton file
3. Constants defined for autopilot limits and timing
4. Build system updated to compile new source file
5. Unit test file skeleton for autopilot structures

---

## Success Criteria

- [ ] All autopilot structures defined in vessels.h
- [ ] No duplicate struct definitions introduced
- [ ] greyhawk_ship_data extended with autopilot_data pointer
- [ ] vessels_autopilot.c compiles without errors
- [ ] CMakeLists.txt updated to include new source
- [ ] No compiler warnings
- [ ] Header structure clean and well-documented
