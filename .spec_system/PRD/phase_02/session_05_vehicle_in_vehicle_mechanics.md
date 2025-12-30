# Session 05: Vehicle-in-Vehicle Mechanics

**Session ID**: `phase02-session05-vehicle-in-vehicle-mechanics`
**Status**: Not Started
**Estimated Tasks**: ~15-20
**Estimated Duration**: 2-4 hours

---

## Objective

Implement mechanics for loading smaller vehicles onto larger transport vessels, enabling scenarios like cars on ferries, wagons on transport ships, and mounts in cargo holds.

---

## Scope

### In Scope (MVP)
- Vehicle loading onto vessels (load_vehicle_onto_vessel)
- Vehicle unloading from vessels (unload_vehicle_from_vessel)
- Capacity and weight limit checking
- Nested transport tracking (which vehicle is on which vessel)
- Player commands: `loadvehicle`, `unloadvehicle`
- State persistence for loaded vehicles

### Out of Scope
- Loading vehicles onto other vehicles (keep simple)
- Multi-level nesting (vehicle on vehicle on vessel)
- Automated loading/unloading
- Loading while vessel is moving

---

## Prerequisites

- [ ] Session 04 complete (player commands)
- [ ] Vessel docking system functional
- [ ] Review vessels_docking.c for boarding patterns

---

## Deliverables

1. load_vehicle_onto_vessel() function
2. unload_vehicle_from_vessel() function
3. check_vessel_vehicle_capacity() validation
4. Loaded vehicle tracking structure
5. do_loadvehicle() command handler
6. do_unloadvehicle() command handler
7. Database persistence for loaded state

---

## Success Criteria

- [ ] Vehicles can be loaded onto docked vessels
- [ ] Vehicles can be unloaded at valid locations
- [ ] Weight/capacity limits enforced
- [ ] Loaded vehicles move with parent vessel
- [ ] Players can access loaded vehicles
- [ ] State persists across server restarts
- [ ] Clear error messages for invalid operations
