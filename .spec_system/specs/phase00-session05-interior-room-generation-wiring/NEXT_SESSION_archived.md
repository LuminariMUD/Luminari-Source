# NEXT_SESSION.md

## Session Recommendation

**Generated**: 2025-12-29
**Project State**: Phase 00 - Core Vessel System
**Completed Sessions**: 4

---

## Recommended Next Session

**Session ID**: `phase00-session05-interior-room-generation-wiring`
**Session Name**: Interior Room Generation Wiring
**Estimated Duration**: 3-4 hours
**Estimated Tasks**: 18-22

---

## Why This Session Next?

### Prerequisites Met
- [x] Session 01-03 completed (header cleanup, wilderness allocation, vessel types)
- [x] Session 04 completed (Phase 2 commands registered in interpreter.c)
- [x] Understanding of vessels_rooms.c available
- [x] DB tables created (ship_room_templates with 19 templates)

### Dependencies
- **Builds on**: Session 04 (command registration enables ship_rooms command)
- **Enables**: Session 06 (interior movement needs rooms to exist first)

### Project Progression
Session 05 is the natural continuation of the implementation sequence. With commands registered, the next logical step is to wire the room generation so that ships actually have interiors to navigate. The `generate_ship_interior()` function exists but is never called - this session connects it to ship creation/loading entry points.

---

## Session Overview

### Objective
Wire the existing `generate_ship_interior()` function to ship creation/loading, ensuring interior rooms are created in the reserved VNUM range (30000-40019) when vessels are instantiated.

### Key Deliverables
1. Interior rooms generated when ship is created/loaded
2. Rooms use correct VNUM range (30000-40019)
3. Room types match vessel configuration
4. `ship_rooms` command displays generated rooms correctly

### Scope Summary
- **In Scope (MVP)**: Identify ship creation/loading entry points, call `generate_ship_interior()` at appropriate times, connect room generation to vessel type, verify VNUM allocation, ensure rooms persist across vessel operations
- **Out of Scope**: Interior movement (Session 06), room persistence to database (Session 07), new room template types

---

## Technical Considerations

### Technologies/Patterns
- ANSI C90/C89 (no C99/C11 features)
- LuminariMUD room system (world array, real_room(), etc.)
- VNUM allocation formula: `30000 + (ship->shipnum * MAX_SHIP_ROOMS) + room_index`
- Greyhawk ship system structures

### Potential Challenges
- Identifying exact ship creation entry points in the codebase
- Ensuring rooms don't conflict with existing builder zones
- Handling ship loading when rooms may already exist
- Memory management for dynamically created rooms

### Relevant Considerations
- [P00] **Room templates hard-coded**: Currently 10 hard-coded templates in vessels_rooms.c; stretch goal to replace with DB lookup from ship_room_templates table
- [P00] **Don't hardcode VNUMs**: Use #defines or configuration (VNUM range 30000-40019 already reserved)
- [P00] **Don't skip NULL checks**: Critical for C code, especially when creating/accessing rooms

---

## Alternative Sessions

If this session is blocked:
1. **session08-external-view-display-systems** - Could implement `look_outside` and tactical display without interior rooms
2. **session09-testing-validation** - Could write tests for existing functionality while investigating blocking issues

---

## Next Steps

Run `/sessionspec` to generate the formal specification.
