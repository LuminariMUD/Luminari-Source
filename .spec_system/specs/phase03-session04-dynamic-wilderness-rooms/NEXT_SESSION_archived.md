# NEXT_SESSION.md

## Session Recommendation

**Generated**: 2025-12-30
**Project State**: Phase 03 - Optimization & Polish
**Completed Sessions**: 26 (3 of 6 in current phase)

---

## Recommended Next Session

**Session ID**: `phase03-session04-dynamic-wilderness-rooms`
**Session Name**: Dynamic Wilderness Rooms
**Estimated Duration**: 3-4 hours
**Estimated Tasks**: 16-20

---

## Why This Session Next?

### Prerequisites Met
- [x] Session 03 complete (command registration wiring)
- [x] find_available_wilderness_room() exists in wilderness.c
- [x] assign_wilderness_room() exists in wilderness.c
- [x] Wilderness room pool system operational

### Dependencies
- **Builds on**: Phase 03 Session 03 (command registration)
- **Enables**: Session 05 (vessel type system) and Session 06 (final testing)

### Project Progression
This is the **#1 Critical Path Item** identified in the PRD. Currently, vessel movement fails silently when the destination wilderness coordinate has no pre-allocated room. This session implements the dynamic room allocation pattern (`find_available_wilderness_room()` + `assign_wilderness_room()`) already proven in `src/movement.c`, enabling vessels to navigate to any valid wilderness coordinate without requiring pre-allocated rooms.

---

## Session Overview

### Objective
Implement dynamic wilderness room allocation for vessel movement so vessels can navigate to any valid wilderness coordinate without requiring pre-allocated rooms.

### Key Deliverables
1. Modified vessel movement to use dynamic room allocation
2. Room release mechanism when vessels depart coordinates
3. Multi-vessel handling at single coordinates
4. Error handling for allocation failures
5. Unit tests (target: 12+ tests)
6. Performance benchmark results (<10ms per movement)

### Scope Summary
- **In Scope (MVP)**: Dynamic room allocation integration, room release on departure, concurrent vessel handling, error handling, unit tests, performance testing
- **Out of Scope**: Changes to wilderness.c core system, new terrain types, weather modifications, Z-axis room allocation

---

## Technical Considerations

### Technologies/Patterns
- Wilderness coordinate system (X/Y/Z with -1024 to +1024 range)
- Room pool allocation pattern from movement.c
- CuTest unit testing framework
- Valgrind memory validation

### Potential Challenges
- Ensuring room release happens correctly when vessels leave coordinates
- Handling multiple vessels at the same coordinate simultaneously
- Performance impact of allocation/deallocation during rapid movement
- Integration with existing vessel position tracking

### Relevant Considerations
- [P03] **Vessel movement fails silently**: This session directly addresses this Architecture concern by implementing proper room allocation
- [P03] **Wilderness system required**: Movement depends on wilderness coordinate system being operational
- [P02] **Standalone unit test files**: Continue pattern of self-contained tests without server dependencies

---

## Alternative Sessions

If this session is blocked:
1. **None available** - Sessions 05 and 06 both depend on Session 04 completing first
2. **Consider**: If wilderness room functions are unavailable, investigate wilderness.c first to understand the allocation API

---

## Next Steps

Run `/sessionspec` to generate the formal specification.
