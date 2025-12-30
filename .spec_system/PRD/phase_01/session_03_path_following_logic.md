# Session 03: Path Following Logic

**Session ID**: `phase01-session03-path-following-logic`
**Status**: Not Started
**Estimated Tasks**: ~18-22
**Estimated Duration**: 3-4 hours

---

## Objective

Implement the core autopilot movement logic that enables vessels to follow routes by navigating between waypoints autonomously.

---

## Scope

### In Scope (MVP)
- Autopilot tick processing function (called from game loop)
- Calculate direction and distance to next waypoint
- Move vessel toward current waypoint at configured speed
- Detect waypoint arrival and advance to next
- Handle route completion (stop or loop)
- Respect waypoint wait times
- Terrain validation before movement
- Integration with existing vessel movement system

### Out of Scope
- Player commands to control autopilot (Session 04)
- NPC pilot behavior (Session 05)
- Scheduled route triggers (Session 06)
- Collision avoidance (future enhancement)

---

## Prerequisites

- [ ] Session 01 complete (data structures)
- [ ] Session 02 complete (waypoint/route management)
- [ ] Understanding of game tick/heartbeat system
- [ ] Understanding of existing vessel movement in vessels.c

---

## Deliverables

1. `autopilot_tick()` function for game loop integration
2. `calculate_heading_to_waypoint()` function
3. `check_waypoint_arrival()` function
4. `advance_to_next_waypoint()` function
5. Route completion handling (stop/loop)
6. Terrain validation before autopilot moves
7. Integration point in comm.c or heartbeat handler
8. Unit tests for path-following logic

---

## Success Criteria

- [ ] Vessels with autopilot enabled move toward waypoints
- [ ] Vessels stop at waypoints for configured wait time
- [ ] Vessels advance through all waypoints in route order
- [ ] Loop routes restart from first waypoint
- [ ] Non-loop routes stop at final waypoint
- [ ] Invalid terrain destinations prevent movement
- [ ] Autopilot respects vessel speed settings
- [ ] Performance acceptable with 100+ autopilot vessels
- [ ] Unit tests verify all path-following scenarios
