# NEXT_SESSION.md

## Session Recommendation

**Generated**: 2025-12-30
**Project State**: Phase 02 - Simple Vehicle Support
**Completed Sessions**: 19 (3 in current phase)

---

## Recommended Next Session

**Session ID**: `phase02-session04-vehicle-player-commands`
**Session Name**: Vehicle Player Commands
**Estimated Duration**: 2-4 hours
**Estimated Tasks**: 15-20

---

## Why This Session Next?

### Prerequisites Met
- [x] Session 03 complete (vehicle movement system)
- [x] Vehicle data structures implemented (Session 01)
- [x] Vehicle creation system functional (Session 02)
- [x] Vehicle movement with terrain handling (Session 03)

### Dependencies
- **Builds on**: Session 03 (movement system provides the backend for drive command)
- **Enables**: Session 05 (vehicle-in-vehicle mechanics require mount/dismount)

### Project Progression
This is the natural next step in the Phase 02 sequence. Sessions 01-03 established the vehicle data layer, creation system, and movement mechanics. Session 04 exposes these capabilities to players through commands. Without player commands, the vehicle system cannot be tested or used. This session bridges the gap between internal systems and player interaction.

---

## Session Overview

### Objective
Implement player-facing commands for interacting with vehicles including mounting, dismounting, driving, and status checking.

### Key Deliverables
1. `do_mount()` command handler - Enter/mount a vehicle
2. `do_dismount()` command handler - Exit/dismount from vehicle
3. `do_drive()` command handler - Move vehicle in specified direction
4. `do_vehiclestatus()` command handler - Display vehicle state, position, cargo
5. Command entries in interpreter.c
6. Help files in lib/text/help/

### Scope Summary
- **In Scope (MVP)**: mount, dismount, drive, vehiclestatus commands; command registration; input validation; help entries
- **Out of Scope**: Autopilot commands, combat commands from vehicles, vehicle-in-vehicle commands (Session 05), unified interface (Session 06)

---

## Technical Considerations

### Technologies/Patterns
- ACMD() macro pattern for command handlers
- interpreter.c command registration (cmd_info[] array)
- act.*.c command implementation patterns
- send_to_char() for player messaging
- Standard NULL checks and ANSI C90 compliance

### Potential Challenges
- Ensuring mount/dismount properly updates character location state
- Drive command must validate terrain using existing vehicle movement functions
- Status display formatting for terminal output
- Handling edge cases (already mounted, vehicle not present, etc.)

### Relevant Considerations
- **[P01] C90/C89 compliance**: No C99/C11 features allowed (no // comments, no declarations after statements)
- **[P01] NULL checks critical**: Always check pointers before dereferencing
- **[P01] CuTest for unit testing**: Located in unittests/CuTest/
- **[P00] Interior movement unimplemented**: Note that interior movement helpers are not yet complete; vehicle commands operate at vehicle level

---

## Alternative Sessions

If this session is blocked:
1. **phase02-session06-unified-command-interface** - Could start interface design while commands are being implemented (but would require rework)
2. **phase02-session07-testing-validation** - Could begin test framework setup, but most tests depend on commands existing

---

## Next Steps

Run `/sessionspec` to generate the formal specification.
