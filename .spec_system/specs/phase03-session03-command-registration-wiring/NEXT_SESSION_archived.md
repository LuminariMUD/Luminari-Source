# NEXT_SESSION.md

## Session Recommendation

**Generated**: 2025-12-30
**Project State**: Phase 03 - Optimization & Polish
**Completed Sessions**: 25

---

## Recommended Next Session

**Session ID**: `phase03-session03-command-registration-wiring`
**Session Name**: Command Registration & Wiring
**Estimated Duration**: 2-3 hours
**Estimated Tasks**: 15-18

---

## Why This Session Next?

### Prerequisites Met
- [x] Session 02 complete (interior movement working) - Completed as `phase03-session02-interior-movement-implementation`
- [x] do_dock, do_undock handlers exist in vessels_docking.c
- [x] generate_ship_interior() implemented in vessels_rooms.c

### Dependencies
- **Builds on**: Session 02 (Interior Movement Implementation)
- **Enables**: Session 04 (Dynamic Wilderness Rooms)

### Project Progression
This is the logical next step in Phase 03's sequential dependency chain. Sessions 01-02 established clean code structure and interior movement functionality. Session 03 now wires up the Phase 2 commands that depend on this foundation, making docking, hostile boarding, and interior viewing accessible to players. This directly addresses the Active Concern in CONSIDERATIONS.md: "Phase 2 commands not registered - dock, undock, board_hostile, look_outside, ship_rooms need adding to interpreter.c."

---

## Session Overview

### Objective
Register all Phase 2 vessel commands in interpreter.c and wire up interior generation to be called at appropriate runtime points.

### Key Deliverables
1. All 5 Phase 2 commands registered in interpreter.c (dock, undock, board_hostile, look_outside, ship_rooms)
2. Interior generation wired to ship creation/loading
3. Persistence calls wired to runtime (save/load interiors)
4. look_outside connected to get_weather() and wilderness view systems
5. Unit tests for command registration
6. Manual testing verification

### Scope Summary
- **In Scope (MVP)**: Command registration, interior generation wiring, persistence wiring, weather integration for look_outside
- **Out of Scope**: New command implementation, tactical display, contact detection, ship combat

---

## Technical Considerations

### Technologies/Patterns
- interpreter.c command registration array (cmd_info[])
- ACMD macro pattern for command handlers
- Runtime wiring via initialization hooks in db.c or vessels.c
- get_weather() integration from weather.c

### Potential Challenges
- Finding correct insertion point in interpreter.c command array (alphabetical ordering)
- Ensuring command precedence over any legacy commands
- Wiring interior generation without causing startup delays
- Handling edge cases in look_outside when vessel is in unusual terrain

### Relevant Considerations
- [P03] **Phase 2 commands not registered**: This session directly resolves this Active Concern
- [P00] **Auto-create DB tables at startup**: Pattern to follow for wiring interior generation
- [Lesson] **Standalone unit test files**: Keep new tests self-contained without server dependencies

---

## Alternative Sessions

If this session is blocked:
1. **Session 05 (Vessel Type System)** - Could potentially be done out-of-order, but would leave commands unwired
2. **Return to documentation** - Update existing docs while investigating blockers

---

## Next Steps

Run `/sessionspec` to generate the formal specification.
