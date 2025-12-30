# NEXT_SESSION.md

## Session Recommendation

**Generated**: 2025-12-30
**Project State**: Phase 01 - Automation Layer
**Completed Sessions**: 12 (9 Phase 00 + 3 Phase 01)

---

## Recommended Next Session

**Session ID**: `phase01-session04-autopilot-player-commands`
**Session Name**: Autopilot Player Commands
**Estimated Duration**: 2-4 hours
**Estimated Tasks**: 15-18

---

## Why This Session Next?

### Prerequisites Met
- [x] Session 01: Autopilot data structures complete
- [x] Session 02: Waypoint and route management complete
- [x] Session 03: Path-following logic complete
- [x] Understanding of command registration in interpreter.c (validated in Phase 00)

### Dependencies
- **Builds on**: Sessions 01-03 (all autopilot infrastructure)
- **Enables**: Session 05 (NPC pilot integration needs commands), Session 06 (scheduled routes need route assignment)

### Project Progression
This is the natural next step in the Automation Layer. With all backend infrastructure complete (data structures, waypoint/route management, path-following logic), we now need to expose these capabilities to players through a command interface. This follows the proven pattern from Phase 00 where we built infrastructure first, then wired up commands.

---

## Session Overview

### Objective
Implement player-facing commands for controlling autopilot, managing waypoints, and configuring routes.

### Key Deliverables
1. **Autopilot control commands** - `autopilot on/off/status`
2. **Waypoint management commands** - `setwaypoint`, `listwaypoints`, `delwaypoint`
3. **Route management commands** - `createroute`, `addtoroute`, `listroutes`, `setroute`
4. **Command registration** in interpreter.c
5. **Help file entries** for all commands
6. **Permission checks** - captain/crew authorization

### Scope Summary
- **In Scope (MVP)**: 9 commands with argument parsing, validation, user feedback, help files
- **Out of Scope**: NPC pilot commands (Session 05), scheduled route commands (Session 06), route editing (reorder/remove waypoints)

---

## Technical Considerations

### Technologies/Patterns
- ACMD() macro for command handlers in vessels_autopilot.c
- one_argument() / two_arguments() for argument parsing
- Command registration in interpreter.c cmd_info array
- Help files in lib/text/help/ directory

### Potential Challenges
- Ensuring commands only work in vessel context
- Permission validation for captain/crew roles
- Handling malformed input without crashes
- Coordinate capture for waypoint creation

### Relevant Considerations
- **[P00] C90 Standards**: Use /* */ comments, no declarations after statements
- **[P00] NULL Checks**: Critical for all pointer operations in command handlers
- **[P00] CuTest Testing**: Add unit tests for command parsing logic
- **[P00] VNUMs**: Use #defines, not hardcoded values

---

## Alternative Sessions

If this session is blocked:
1. **Session 07: Testing and Validation** - Could run partial tests on sessions 01-03
2. **Session 05: NPC Pilot Integration** - Could proceed with basic NPC assignment, but would lack player commands to control autopilot

Neither alternative is recommended as they both depend on having player commands available.

---

## Next Steps

Run `/sessionspec` to generate the formal specification.
