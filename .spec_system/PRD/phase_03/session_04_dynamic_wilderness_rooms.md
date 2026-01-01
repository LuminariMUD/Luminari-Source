# Session 04: Dynamic Wilderness Rooms

**Session ID**: `phase03-session04-dynamic-wilderness-rooms`
**Status**: Not Started
**Estimated Tasks**: ~16-20
**Estimated Duration**: 3-4 hours

---

## Objective

Implement dynamic wilderness room allocation for vessel movement so vessels can navigate to any valid wilderness coordinate without requiring pre-allocated rooms.

---

## Scope

### In Scope (MVP)

- Integrate find_available_wilderness_room() into vessel movement
- Integrate assign_wilderness_room() into vessel movement
- Handle room allocation failures gracefully
- Release rooms when vessels leave coordinates
- Ensure room allocation respects terrain type
- Update vessel position tracking after room allocation
- Handle concurrent vessel movements (multiple vessels at same coordinate)
- Unit tests for dynamic allocation during movement
- Performance testing with 100+ vessels moving

### Out of Scope

- Changes to wilderness.c core system
- New terrain types
- Weather system modifications
- Z-axis (altitude/depth) room allocation (if not needed)

---

## Prerequisites

- [ ] Session 03 complete (commands registered)
- [ ] find_available_wilderness_room() exists in wilderness.c
- [ ] assign_wilderness_room() exists in wilderness.c
- [ ] Understanding of wilderness room pool system

---

## Deliverables

1. Modified vessel movement to use dynamic room allocation
2. Room release mechanism when vessels depart
3. Multi-vessel handling at single coordinates
4. Error handling for allocation failures
5. Unit tests (target: 12+ tests)
6. Performance benchmark results

---

## Success Criteria

- [ ] Vessels can move to any valid wilderness coordinate
- [ ] No silent failures on movement
- [ ] Rooms properly released when vessels leave
- [ ] Multiple vessels can occupy same coordinate
- [ ] Performance: <10ms for movement operation
- [ ] Valgrind clean
- [ ] Stress test: 100 concurrent vessels navigating
