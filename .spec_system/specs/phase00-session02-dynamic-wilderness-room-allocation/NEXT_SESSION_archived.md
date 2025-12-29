# NEXT_SESSION.md

## Session Recommendation

**Generated**: 2025-12-29
**Project State**: Phase 00 - Core Vessel System
**Completed Sessions**: 1

---

## Recommended Next Session

**Session ID**: `phase00-session02-dynamic-wilderness-room-allocation`
**Session Name**: Dynamic Wilderness Room Allocation
**Estimated Duration**: 3-4 hours
**Estimated Tasks**: 18-22

---

## Why This Session Next?

### Prerequisites Met
- [x] Session 01 completed (header cleanup foundation)
- [x] Understanding of wilderness coordinate system (documented in PRD)
- [x] Understanding of room allocation patterns (reference in movement.c)

### Dependencies
- **Builds on**: phase00-session01-header-cleanup-foundation (clean codebase)
- **Enables**: All subsequent vessel movement features (type system, interior rooms, persistence)

### Project Progression

This session addresses the **#1 Critical Path Item** from the PRD:

> Vessel movement does not allocate new dynamic wilderness rooms. Movement fails if destination coordinate has no pre-allocated room.

This is a blocking issue that prevents all vessel navigation. Without dynamic room allocation, vessels can only move to coordinates that happen to have pre-existing rooms. Fixing this unlocks the entire vessel system's core functionality and is prerequisite for:
- Session 03 (Vessel Type System) - needs working movement to test different vessel types
- Session 05-06 (Interior Rooms) - need vessel movement working first
- All subsequent features that depend on vessel navigation

---

## Session Overview

### Objective

Implement dynamic wilderness room allocation for vessel movement, fixing the critical bug where vessels fail silently when moving to coordinates without pre-allocated rooms.

### Key Deliverables
1. Updated vessels.c with dynamic room allocation using `find_available_wilderness_room()` + `assign_wilderness_room()` pattern
2. Vessel movement works to any valid wilderness coordinate (-1024 to +1024)
3. Proper error handling for allocation failures
4. No memory leaks from room management

### Scope Summary
- **In Scope (MVP)**: Study movement.c pattern, implement for vessels.c, handle edge cases, test across coordinates
- **Out of Scope**: Performance optimization, room pooling enhancements, multi-vessel coordination

---

## Technical Considerations

### Technologies/Patterns
- Wilderness coordinate system (X, Y, Z navigation)
- Room allocation pattern from src/movement.c
- Terrain restriction validation

### Potential Challenges
- Understanding room lifecycle (allocation vs deallocation timing)
- Edge cases: room pool exhaustion, invalid coordinates
- Ensuring rooms are released when vessels move away
- Thread safety considerations for room allocation

### Relevant Considerations

From CONSIDERATIONS.md:

- **[Architecture]** Vessel movement fails silently - This is the exact issue this session addresses
- **[Technical Debt]** Don't skip NULL checks - Critical when implementing room allocation
- **[External Dependencies]** Wilderness system required - Must understand wilderness.c thoroughly
- **[Lesson Learned]** Don't use C99/C11 features - Must use C90-compatible code

---

## Alternative Sessions

If this session is blocked:

1. **phase00-session04-phase2-command-registration** - Could register Phase 2 commands (dock, undock, etc.) in interpreter.c without movement working. Less impactful but unblocks UI.
2. **phase00-session03-vessel-type-system** - Could implement vessel type mapping theoretically, but testing would be limited without movement.

**Note**: Session 02 is not blocked. It has clear prerequisites met and clear implementation path via the movement.c reference pattern.

---

## Next Steps

Run `/sessionspec` to generate the formal specification.
