# Session 04: Vehicle Player Commands

**Session ID**: `phase02-session04-vehicle-player-commands`
**Status**: Not Started
**Estimated Tasks**: ~15-20
**Estimated Duration**: 2-4 hours

---

## Objective

Implement player-facing commands for interacting with vehicles including mounting, dismounting, driving, and status checking.

---

## Scope

### In Scope (MVP)
- `mount` command - Enter/mount a vehicle
- `dismount` command - Exit/dismount from vehicle
- `drive <direction>` command - Move vehicle in specified direction
- `vehiclestatus` command - Display vehicle state, position, cargo
- Command registration in interpreter.c
- Input validation and error messages
- Help file entries for all commands

### Out of Scope
- Autopilot commands (future phase)
- Combat commands from vehicles
- Vehicle-in-vehicle commands (Session 05)
- Unified interface abstraction (Session 06)

---

## Prerequisites

- [ ] Session 03 complete (movement system)
- [ ] Review interpreter.c for command registration patterns
- [ ] Review act.*.c files for command implementation patterns

---

## Deliverables

1. do_mount() command handler
2. do_dismount() command handler
3. do_drive() command handler
4. do_vehiclestatus() command handler
5. Command entries in interpreter.c
6. Help files in lib/text/help/

---

## Success Criteria

- [ ] All 4 commands registered and functional
- [ ] Players can mount vehicles in same room
- [ ] Players can dismount to current location
- [ ] Drive command moves vehicle with terrain checks
- [ ] Status shows position, speed, cargo capacity
- [ ] Appropriate error messages for invalid actions
- [ ] Help entries accessible via `help <command>`
