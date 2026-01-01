# Session 04: Phase 2 Command Registration

**Session ID**: `phase00-session04-phase2-command-registration`
**Status**: Not Started
**Estimated Tasks**: ~12-15
**Estimated Duration**: 2-3 hours

---

## Objective

Register Phase 2 commands (dock, undock, board_hostile, look_outside, ship_rooms) in interpreter.c to make them accessible to players.

---

## Scope

### In Scope (MVP)
- Add dock command to interpreter.c cmd_info[]
- Add undock command to interpreter.c cmd_info[]
- Add board_hostile command to interpreter.c cmd_info[]
- Add look_outside command to interpreter.c cmd_info[]
- Add ship_rooms command to interpreter.c cmd_info[]
- Verify extern declarations for command handlers
- Test each command is recognized
- Add appropriate help file entries

### Out of Scope
- Implementing command functionality (already exists in vessels_docking.c)
- Modifying command behavior
- Adding new commands beyond the 5 specified

---

## Prerequisites

- [ ] Session 01 completed (clean codebase)
- [ ] Handlers exist in vessels_docking.c (verified)
- [ ] Understanding of interpreter.c cmd_info structure

---

## Deliverables

1. All 5 Phase 2 commands registered in interpreter.c
2. Commands recognized when typed in-game
3. Help files for each command
4. No conflicts with existing commands

---

## Success Criteria

- [ ] `dock` command recognized and calls do_dock
- [ ] `undock` command recognized and calls do_undock
- [ ] `board_hostile` command recognized and calls do_board_hostile
- [ ] `look_outside` command recognized and calls do_look_outside
- [ ] `ship_rooms` command recognized and calls do_ship_rooms
- [ ] All commands have help entries
- [ ] No duplicate command registration warnings
- [ ] Build compiles cleanly

---

## Technical Notes

### Command Handlers (from PRD Section 5)

| Command | Handler | Description |
|---------|---------|-------------|
| dock [ship] | do_dock | Create gangway to adjacent vessel |
| undock | do_undock | Remove docking connection |
| board_hostile <ship> | do_board_hostile | Forced boarding attempt |
| look_outside | do_look_outside | View exterior from interior |
| ship_rooms | do_ship_rooms | List interior rooms |

### Registration Format

In interpreter.c cmd_info[]:
```c
{ "commandname", POS_STANDING, do_handler, 0, 0 },
```

### Existing Phase 1 Commands (for reference)

- board (functional)
- shipstatus (functional)
- speed (functional)
- heading (functional)
- tactical (placeholder)
- contacts (placeholder)
- disembark (placeholder)
- shipload (placeholder)
- setsail (placeholder)

### Known Issue

From PRD Section 11:
> Duplicate `disembark` registration at interpreter.c:385 and interpreter.c:1165

Be careful not to create similar duplicate registrations.

### Files to Modify

- src/interpreter.c - Add command entries
- lib/text/help/ - Add help files
