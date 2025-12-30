# Session Specification

**Session ID**: `phase01-session04-autopilot-player-commands`
**Phase**: 01 - Automation Layer
**Status**: Not Started
**Created**: 2025-12-30

---

## 1. Session Overview

This session implements the player-facing command interface for the vessel autopilot system. With all backend infrastructure now complete (data structures in Session 01, waypoint/route management in Session 02, and path-following logic in Session 03), we now expose these capabilities to players through intuitive commands.

Players will be able to control autopilot mode (on/off/status), manage waypoints (create, list, delete), and configure routes (create, add waypoints, list, assign to vessel). All commands will enforce captain/crew authorization and validate that the player is in a vessel context before executing.

This session follows the proven pattern from Phase 00 where infrastructure was built first, then wired up to user-facing commands. Upon completion, players will have full control over automated vessel navigation, enabling the advanced features in Sessions 05 (NPC pilots) and 06 (scheduled routes).

---

## 2. Objectives

1. Implement 3 autopilot control commands (autopilot on/off/status) with proper state management
2. Implement 3 waypoint management commands (setwaypoint, listwaypoints, delwaypoint) with coordinate capture
3. Implement 4 route management commands (createroute, addtoroute, listroutes, setroute) with persistence
4. Register all 10 commands in interpreter.c with appropriate help file entries

---

## 3. Prerequisites

### Required Sessions
- [x] `phase01-session01-autopilot-data-structures` - Provides autopilot_data, waypoint_data, route_data structs
- [x] `phase01-session02-waypoint-route-management` - Provides waypoint/route CRUD functions
- [x] `phase01-session03-path-following-logic` - Provides autopilot_update(), path calculation, state machine
- [x] `phase00-session04-phase2-command-registration` - Validated command registration pattern in interpreter.c

### Required Tools/Knowledge
- ACMD() macro usage pattern for command handlers
- one_argument() / two_arguments() functions for argument parsing
- send_to_char() for user feedback
- Understanding of vessel permission model (captain/crew roles)

### Environment Requirements
- Sessions 01-03 code merged and compiling
- Database tables for waypoints/routes accessible
- Test vessel available in Zone 213

---

## 4. Scope

### In Scope (MVP)
- `autopilot` command with on/off/status subcommands
- `setwaypoint` command capturing current vessel coordinates
- `listwaypoints` command showing all waypoints for vessel
- `delwaypoint` command removing waypoint by name
- `createroute` command creating empty named route
- `addtoroute` command appending waypoint to route
- `listroutes` command showing all routes for vessel
- `setroute` command assigning route to vessel autopilot
- Command registration in interpreter.c
- Help file entries for all 10 commands
- Captain/crew permission validation on all commands
- Vessel context validation (player must be on vessel)

### Out of Scope (Deferred)
- `removefromroute` command - *Reason: Route editing deferred to future session*
- `reorderroute` command - *Reason: Route editing deferred to future session*
- NPC pilot assignment commands - *Reason: Session 05 scope*
- Scheduled route commands - *Reason: Session 06 scope*
- Route import/export - *Reason: Not in Phase 01 PRD*

---

## 5. Technical Approach

### Architecture
Commands are implemented as ACMD() handlers in vessels_autopilot.c, following the established pattern from Phase 00. Each command validates context (vessel presence, permissions), parses arguments, calls the appropriate Session 02/03 API functions, and provides feedback.

Command handlers are thin wrappers that:
1. Validate player context (in vessel, has permission)
2. Parse and validate arguments
3. Call existing waypoint/route/autopilot API functions
4. Format and send feedback to player

### Design Patterns
- **ACMD Pattern**: Standard LuminariMUD command handler pattern with char_data *ch, argument parsing
- **Thin Controller**: Commands delegate to existing API; no business logic in command handlers
- **Defensive Programming**: NULL checks on all pointers, graceful error handling with user feedback

### Technology Stack
- ANSI C90/C89 (no C99 features)
- Existing vessels.h/vessels_autopilot.c/vessels_autopilot.h infrastructure
- interpreter.c command registration
- lib/text/help/ for help files

---

## 6. Deliverables

### Files to Create
| File | Purpose | Est. Lines |
|------|---------|------------|
| `lib/text/help/autopilot.hlp` | Help entries for all 10 commands | ~120 |

### Files to Modify
| File | Changes | Est. Lines Changed |
|------|---------|-------------------|
| `src/vessels_autopilot.c` | Add 10 ACMD() command handlers | ~400 |
| `src/vessels_autopilot.h` | Declare command handler prototypes | ~15 |
| `src/interpreter.c` | Register 10 commands in cmd_info[] | ~15 |
| `src/interpreter.h` | Add ACMD extern declarations | ~10 |

---

## 7. Success Criteria

### Functional Requirements
- [ ] `autopilot on` enables autopilot for current vessel (captain only)
- [ ] `autopilot off` disables autopilot for current vessel (captain only)
- [ ] `autopilot status` shows current autopilot state, route, progress
- [ ] `setwaypoint <name>` creates waypoint at vessel's current coordinates
- [ ] `listwaypoints` displays all waypoints owned by vessel/player
- [ ] `delwaypoint <name>` removes specified waypoint
- [ ] `createroute <name>` creates new empty route
- [ ] `addtoroute <route> <waypoint>` appends waypoint to route
- [ ] `listroutes` displays all routes with waypoint counts
- [ ] `setroute <route>` assigns route to vessel's autopilot

### Testing Requirements
- [ ] Unit tests for argument parsing functions
- [ ] Manual testing of all commands in-game
- [ ] Permission denial tested for non-captain players
- [ ] Context validation tested (commands fail gracefully outside vessel)

### Quality Gates
- [ ] All files ASCII-encoded (0-127 only)
- [ ] Unix LF line endings
- [ ] Code follows C90 conventions (no // comments, no declarations after statements)
- [ ] All commands have help file entries
- [ ] No compiler warnings with -Wall -Wextra
- [ ] No memory leaks (Valgrind clean on related tests)

---

## 8. Implementation Notes

### Key Considerations
- Player must be physically inside a vessel room to use these commands
- Permission checks use existing vessel ownership/crew role system
- Waypoints store wilderness coordinates (x, y, z)
- Routes are ordered lists of waypoint references
- Autopilot state machine integrates with vessel movement tick

### Potential Challenges
- **Vessel context detection**: Need reliable way to determine if player is on a vessel
  - Mitigation: Use existing get_vessel_from_room() or similar function from Sessions 01-03
- **Coordinate capture for waypoints**: Must get vessel's wilderness position
  - Mitigation: Use vessel's stored world_x, world_y, world_z coordinates
- **Route validation**: Route must contain valid, existing waypoints
  - Mitigation: Validate each waypoint exists before adding to route

### Relevant Considerations
- **[P00] C90 Standards**: Use /* */ comments only, declare all variables at block start
- **[P00] NULL Checks**: Critical for ch, vessel, waypoint, and route pointers
- **[P00] CuTest Testing**: Add tests for argument parsing edge cases
- **[P00] VNUMs**: Not directly applicable, but use #defines for any constants
- **Duplicate struct definitions in vessels.h**: Be aware when including headers

### ASCII Reminder
All output files must use ASCII-only characters (0-127). No smart quotes, em-dashes, or Unicode symbols in help files or source code.

---

## 9. Testing Strategy

### Unit Tests
- Test one_argument() parsing with various inputs (empty, single, multiple words)
- Test permission check helper function with mock character data
- Test waypoint name validation (length, characters, uniqueness)
- Test route name validation

### Integration Tests
- Verify commands call correct API functions from Sessions 02-03
- Verify database persistence of waypoints and routes created via commands

### Manual Testing
- Create waypoint at vessel location, verify coordinates match
- Create route, add waypoints, assign to vessel, enable autopilot
- Verify autopilot status shows correct route and progress
- Test command rejection when not on vessel
- Test command rejection for non-captain players
- Test malformed input handling (missing args, invalid names)

### Edge Cases
- Empty waypoint/route names
- Duplicate waypoint/route names
- Very long names (test truncation)
- Special characters in names
- Deleting waypoint that's part of active route
- Setting route that doesn't exist
- Enabling autopilot with no route set

---

## 10. Dependencies

### External Libraries
- None beyond standard LuminariMUD dependencies

### Internal Dependencies
- **vessels.h**: Vessel data structures, VESSEL_* constants
- **vessels_autopilot.h**: Autopilot API functions from Sessions 01-03
- **interpreter.h/c**: Command registration infrastructure
- **comm.h**: send_to_char() for player feedback
- **utils.h**: Argument parsing utilities

### Other Sessions
- **Depends on**: phase01-session01, phase01-session02, phase01-session03
- **Depended by**: phase01-session05 (NPC pilots need commands), phase01-session06 (scheduled routes need route assignment)

---

## Next Steps

Run `/tasks` to generate the implementation task checklist.
