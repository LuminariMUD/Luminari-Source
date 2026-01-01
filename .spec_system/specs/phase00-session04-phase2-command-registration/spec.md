# Session Specification

**Session ID**: `phase00-session04-phase2-command-registration`
**Phase**: 00 - Core Vessel System
**Status**: Not Started
**Created**: 2025-12-29

---

## 1. Session Overview

This session registers the Phase 2 vessel commands (dock, undock, board_hostile, look_outside, ship_rooms) in interpreter.c to make them accessible to players. The command handlers already exist in vessels_docking.c and are properly declared in vessels.h; this session wires them into the game's command parser.

This is a critical infrastructure step that directly addresses the Active Concern "Phase 2 commands not registered" from CONSIDERATIONS.md. Without command registration, players cannot access the docking and interior viewing functionality even though the underlying code is implemented.

Completing this session enables Session 05 (Interior Room Generation Wiring) and Session 06 (Interior Movement Implementation), which depend on these commands being accessible for testing and validation.

---

## 2. Objectives

1. Register all 5 Phase 2 commands in interpreter.c cmd_info[] array
2. Ensure commands are recognized when typed in-game and route to correct handlers
3. Create help file entries for each command documenting usage and syntax
4. Verify no duplicate command registrations or conflicts with existing commands

---

## 3. Prerequisites

### Required Sessions
- [x] `phase00-session01-header-cleanup-foundation` - Clean vessels.h with proper declarations
- [x] `phase00-session02-dynamic-wilderness-room-allocation` - Wilderness room system working
- [x] `phase00-session03-vessel-type-system` - Vessel type capabilities defined

### Required Tools/Knowledge
- Understanding of interpreter.c cmd_info[] structure and registration format
- Familiarity with MUD command position requirements (POS_STANDING, POS_RESTING, etc.)
- Help file format in lib/text/help/

### Environment Requirements
- Source code compiles cleanly
- Command handlers exist in vessels_docking.c (verified: lines 389, 442, 501, 557, 621)
- ACMD_DECL declarations exist in vessels.h (verified: lines 606-611)

---

## 4. Scope

### In Scope (MVP)
- Add dock command registration to interpreter.c
- Add undock command registration to interpreter.c
- Add board_hostile command registration to interpreter.c
- Add look_outside command registration to interpreter.c
- Add ship_rooms command registration to interpreter.c
- Verify extern/ACMD_DECL declarations are properly included
- Create help file for each command
- Test that each command is recognized by the parser

### Out of Scope (Deferred)
- Modifying command handler functionality - *Reason: handlers already implemented in vessels_docking.c*
- Adding new commands beyond the 5 specified - *Reason: other commands are different sessions*
- Testing full command execution - *Reason: requires interior rooms from Session 05-06*

---

## 5. Technical Approach

### Architecture
Commands are registered in the `cmd_info[]` array in interpreter.c. Each entry maps a command string to a handler function pointer, position requirement, minimum level, and other metadata. The interpreter parses player input and dispatches to the appropriate handler.

### Design Patterns
- **Command Pattern**: Each command maps to an ACMD handler function
- **Table-Driven Design**: cmd_info[] array drives command recognition

### Technology Stack
- ANSI C90/C89 with GNU extensions
- interpreter.c command registration system
- lib/text/help/ help file system

### Registration Format
```c
{"commandname", "sort_as", minimum_position, do_handler, min_level, subcmd, ignore_wait, action_type, {cooldowns}, check_ptr},
```

---

## 6. Deliverables

### Files to Create
| File | Purpose | Est. Lines |
|------|---------|------------|
| `lib/text/help/dock.hlp` | Help entry for dock command | ~15 |
| `lib/text/help/undock.hlp` | Help entry for undock command | ~12 |
| `lib/text/help/board_hostile.hlp` | Help entry for board_hostile command | ~15 |
| `lib/text/help/look_outside.hlp` | Help entry for look_outside command | ~12 |
| `lib/text/help/ship_rooms.hlp` | Help entry for ship_rooms command | ~12 |

### Files to Modify
| File | Changes | Est. Lines |
|------|---------|------------|
| `src/interpreter.c` | Add 5 command registrations to cmd_info[] | ~10 |

---

## 7. Success Criteria

### Functional Requirements
- [ ] `dock` command recognized and calls do_dock handler
- [ ] `undock` command recognized and calls do_undock handler
- [ ] `board_hostile` command recognized and calls do_board_hostile handler
- [ ] `look_outside` command recognized and calls do_look_outside handler
- [ ] `ship_rooms` command recognized and calls do_ship_rooms handler
- [ ] All commands appear in help system

### Testing Requirements
- [ ] Build compiles cleanly with no errors
- [ ] Build produces no new warnings
- [ ] Commands registered without duplicate warnings at startup

### Quality Gates
- [ ] All files ASCII-encoded (characters 0-127 only)
- [ ] Unix LF line endings
- [ ] Code follows project conventions (two-space indent, Allman braces)
- [ ] No C99/C11 features (/* */ comments only)

---

## 8. Implementation Notes

### Key Considerations
- Place command registrations alphabetically or in a logical vessel command group
- Use appropriate position requirements: POS_STANDING for dock/undock/board_hostile, POS_RESTING for look_outside/ship_rooms
- Ensure vessels.h is included for ACMD_DECL declarations to be visible
- Set minimum level to 0 for player accessibility

### Potential Challenges
- **Duplicate registration**: PRD notes existing disembark duplication at lines 385 and 1165 - must verify no duplicate entries for new commands
- **Position requirements**: Must match expected usage context for each command
- **Include order**: vessels.h must be included in interpreter.c for declarations

### Relevant Considerations
- [P00] **Phase 2 commands not registered**: This session directly resolves this Active Concern
- [P00] **Don't use C99/C11 features**: All code must use /* */ comments, not //
- [P00] **Interior movement unimplemented**: Session 05-06 will implement the handlers these commands call

### ASCII Reminder
All output files must use ASCII-only characters (0-127). No curly quotes, em-dashes, or special characters.

---

## 9. Testing Strategy

### Unit Tests
- Not applicable - command registration tested via integration

### Integration Tests
- Build the project with `./cbuild.sh`
- Verify no compilation errors or new warnings
- Check startup logs for duplicate command warnings

### Manual Testing
- Connect to running server
- Type each command name (dock, undock, board_hostile, look_outside, ship_rooms)
- Verify command is recognized (not "Huh?!?")
- Type `help <command>` for each command
- Verify help text displays

### Edge Cases
- Verify partial command matching works (e.g., "doc" matching "dock")
- Verify no conflicts with existing commands
- Check that commands fail gracefully when not on a vessel

---

## 10. Dependencies

### External Libraries
- None required for this session

### Other Sessions
- **Depends on**: phase00-session01-header-cleanup-foundation (ACMD_DECL in vessels.h)
- **Depended by**: phase00-session05-interior-room-generation-wiring (commands must be registered first)
- **Depended by**: phase00-session06-interior-movement-implementation (commands must be registered first)

---

## Command Reference

| Command | Handler | Position | Description |
|---------|---------|----------|-------------|
| dock [ship] | do_dock | POS_STANDING | Create gangway to adjacent vessel |
| undock | do_undock | POS_STANDING | Remove docking connection |
| board_hostile <ship> | do_board_hostile | POS_FIGHTING | Forced boarding attempt |
| look_outside | do_look_outside | POS_RESTING | View exterior from interior |
| ship_rooms | do_ship_rooms | POS_RESTING | List interior rooms |

---

## Next Steps

Run `/tasks` to generate the implementation task checklist.
