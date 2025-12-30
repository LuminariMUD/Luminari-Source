# Session 03: Vehicle Movement System

**Session ID**: `phase02-session03-vehicle-movement-system`
**Status**: Not Started
**Estimated Tasks**: ~15-20
**Estimated Duration**: 2-4 hours

---

## Objective

Implement vehicle movement across the wilderness coordinate system with proper terrain restrictions and speed modifiers for land-based vehicles.

---

## Scope

### In Scope (MVP)
- Vehicle movement function (move_vehicle)
- Terrain traversability checking for land vehicles
- Speed modifiers by terrain type (road faster, rough terrain slower)
- Direction handling (8 compass directions)
- Dynamic wilderness room allocation for vehicle destinations
- Vehicle position tracking and updates

### Out of Scope
- Autopilot/waypoint navigation (future phase)
- Combat during movement
- Weather effects on vehicles (future enhancement)
- Mount stamina/fatigue system

---

## Prerequisites

- [ ] Session 02 complete (creation system)
- [ ] Review wilderness.c for room allocation patterns
- [ ] Review vessels.c for movement implementation patterns

---

## Deliverables

1. move_vehicle() core function
2. check_vehicle_terrain() validation function
3. get_vehicle_speed_modifier() for terrain types
4. Vehicle position update logic
5. Integration with wilderness coordinate system
6. Dynamic room allocation for destinations

---

## Success Criteria

- [ ] Vehicles move across wilderness grid
- [ ] Land vehicles blocked by water terrain
- [ ] Road terrain provides speed bonus
- [ ] Rough terrain applies speed penalty
- [ ] Position updates correctly in database
- [ ] No crashes on invalid movement attempts
- [ ] Movement uses find_available_wilderness_room() pattern
