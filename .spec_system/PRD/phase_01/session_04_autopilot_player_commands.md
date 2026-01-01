# Session 04: Autopilot Player Commands

**Session ID**: `phase01-session04-autopilot-player-commands`
**Status**: Not Started
**Estimated Tasks**: ~15-18
**Estimated Duration**: 2-4 hours

---

## Objective

Implement player-facing commands for controlling autopilot, managing waypoints, and configuring routes.

---

## Scope

### In Scope (MVP)
- `autopilot on/off` - Enable/disable autopilot
- `autopilot status` - Show current autopilot state
- `setwaypoint <name>` - Create waypoint at vessel location
- `listwaypoints` - Display all waypoints
- `delwaypoint <id/name>` - Remove a waypoint
- `createroute <name>` - Create new route
- `addtoroute <route> <waypoint>` - Add waypoint to route
- `listroutes` - Display all routes with waypoints
- `setroute <route>` - Assign route to current vessel
- Register all commands in interpreter.c
- Help file entries for all commands

### Out of Scope
- NPC pilot commands (Session 05)
- Scheduled route commands (Session 06)
- Route editing (reorder waypoints, remove from route)

---

## Prerequisites

- [ ] Session 01-03 complete (structures, management, logic)
- [ ] Understanding of command registration in interpreter.c
- [ ] Understanding of command argument parsing

---

## Deliverables

1. Command handlers in `vessels_autopilot.c`
2. Command registration in `interpreter.c`
3. Help files in `lib/text/help/`
4. Argument parsing and validation
5. User feedback messages
6. Permission checks (captain/crew only)

---

## Success Criteria

- [ ] All commands registered and recognized
- [ ] Commands only work on vessels (appropriate context)
- [ ] Only authorized crew can use autopilot commands
- [ ] Waypoints created with correct coordinates
- [ ] Routes display with waypoint sequence
- [ ] Setting route enables autopilot navigation
- [ ] Help files accessible via `help <command>`
- [ ] Invalid arguments produce helpful error messages
- [ ] No crashes on malformed input
