# NEXT_SESSION.md

## Session Recommendation

**Generated**: 2025-12-30
**Project State**: Phase 01 - Automation Layer
**Completed Sessions**: 11 (9 Phase 00 + 2 Phase 01)

---

## Recommended Next Session

**Session ID**: `phase01-session03-path-following-logic`
**Session Name**: Path Following Logic
**Estimated Duration**: 3-4 hours
**Estimated Tasks**: 18-22

---

## Why This Session Next?

### Prerequisites Met
- [x] Session 01 complete (autopilot data structures)
- [x] Session 02 complete (waypoint/route management with DB persistence)

### Dependencies
- **Builds on**: Session 01 (VESSEL_AUTOPILOT_DATA, AUTOPILOT_STATE_*) and Session 02 (waypoint/route CRUD functions, waypoint_cache)
- **Enables**: Session 04 (player commands to control autopilot), Session 05 (NPC pilot integration)

### Project Progression
This session implements the core autopilot movement engine - the brain that actually makes vessels move between waypoints. Sessions 01-02 provided the data structures and waypoint management; this session makes them functional. Without path-following logic, the autopilot system cannot operate, making this the critical next step before player commands or NPC pilots can be added.

---

## Session Overview

### Objective
Implement the core autopilot movement logic that enables vessels to follow routes by navigating between waypoints autonomously.

### Key Deliverables
1. `autopilot_tick()` function for game loop integration
2. `calculate_heading_to_waypoint()` function
3. `check_waypoint_arrival()` function
4. `advance_to_next_waypoint()` function
5. Route completion handling (stop/loop modes)
6. Terrain validation before autopilot moves
7. Integration point in comm.c or heartbeat handler
8. Unit tests for path-following logic

### Scope Summary
- **In Scope (MVP)**: Autopilot tick processing, direction/distance calculation, movement toward waypoints, waypoint arrival detection, route advancement, wait time handling, terrain validation, game loop integration
- **Out of Scope**: Player commands (Session 04), NPC pilots (Session 05), scheduled routes (Session 06), collision avoidance (future)

---

## Technical Considerations

### Technologies/Patterns
- Game tick/heartbeat integration (comm.c pattern)
- Wilderness coordinate navigation (existing vessels.c movement)
- Distance and heading calculations (Euclidean geometry)
- State machine pattern (AUTOPILOT_STATE_* transitions)

### Potential Challenges
- **Game loop integration**: Must integrate autopilot_tick() without performance impact on main loop
- **Terrain validation**: Must use find_available_wilderness_room() + assign_wilderness_room() pattern to avoid silent movement failures
- **Wait time handling**: Need to track per-vessel wait state without blocking other vessels
- **Performance at scale**: Target 100+ autopilot vessels without degradation

### Relevant Considerations
- [P00] **Vessel movement may fail silently**: Must use find_available_wilderness_room() + assign_wilderness_room() pattern for terrain validation
- [P00] **Don't use C99/C11 features**: ANSI C90/C89 only - no // comments, no declarations after statements
- [P00] **Always NULL check pointers**: Critical in C code before dereferencing

---

## Alternative Sessions

If this session is blocked:
1. **phase01-session04-autopilot-player-commands** - Not recommended; depends on Session 03 path-following logic being complete
2. **phase01-session05-npc-pilot-integration** - Not recommended; depends on Sessions 03 and 04

Note: There are no valid alternatives; Session 03 is a critical-path dependency for all subsequent Phase 01 sessions.

---

## Next Steps

Run `/sessionspec` to generate the formal specification.
