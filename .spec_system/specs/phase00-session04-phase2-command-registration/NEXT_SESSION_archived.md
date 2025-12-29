# NEXT_SESSION.md

## Session Recommendation

**Generated**: 2025-12-29
**Project State**: Phase 00 - Core Vessel System
**Completed Sessions**: 3 of 9

---

## Recommended Next Session

**Session ID**: `phase00-session04-phase2-command-registration`
**Session Name**: Phase 2 Command Registration
**Estimated Duration**: 2-3 hours
**Estimated Tasks**: 12-15

---

## Why This Session Next?

### Prerequisites Met
- [x] Session 01 completed (clean codebase with vessels.h cleanup)
- [x] Session 02 completed (dynamic wilderness room allocation)
- [x] Session 03 completed (vessel terrain capabilities)
- [x] Handlers exist in vessels_docking.c (verified per PRD)

### Dependencies
- **Builds on**: Session 01-03 foundation work
- **Enables**: Session 05 (Interior Room Generation Wiring) - which explicitly requires Session 04 to be complete

### Project Progression
This is the natural next step because:
1. It is the lowest-numbered incomplete session in Phase 00
2. Session 05+ explicitly lists Session 04 as a prerequisite
3. The commands (dock, undock, board_hostile, look_outside, ship_rooms) must be registered before interior room functionality can be tested
4. This directly addresses an Active Concern in CONSIDERATIONS.md

---

## Session Overview

### Objective
Register Phase 2 commands (dock, undock, board_hostile, look_outside, ship_rooms) in interpreter.c to make them accessible to players.

### Key Deliverables
1. All 5 Phase 2 commands registered in interpreter.c cmd_info[]
2. Commands recognized when typed in-game
3. Help files for each command in lib/text/help/
4. No conflicts with existing commands (watch for duplicate registration issues)

### Scope Summary
- **In Scope (MVP)**: Register 5 commands, verify extern declarations, add help files, test recognition
- **Out of Scope**: Modifying command functionality (already exists), adding new commands beyond the 5 specified

---

## Technical Considerations

### Technologies/Patterns
- interpreter.c cmd_info[] structure for command registration
- Help file format in lib/text/help/
- ANSI C90 coding standards (no // comments)

### Potential Challenges
- Duplicate command registration (PRD notes existing disembark duplication at lines 385 and 1165)
- Ensuring correct extern declarations for handlers in vessels_docking.c
- Proper position requirements for each command

### Relevant Considerations
- [P00] **Phase 2 commands not registered**: This session directly resolves this Active Concern
- [P00] **Don't use C99/C11 features**: Use /* */ comments, not //
- [P00] **Interior movement unimplemented**: Session 05-06 will address this after commands are registered

---

## Command Reference

| Command | Handler | Description | Position |
|---------|---------|-------------|----------|
| dock [ship] | do_dock | Create gangway to adjacent vessel | POS_STANDING |
| undock | do_undock | Remove docking connection | POS_STANDING |
| board_hostile <ship> | do_board_hostile | Forced boarding attempt | POS_FIGHTING |
| look_outside | do_look_outside | View exterior from interior | POS_RESTING |
| ship_rooms | do_ship_rooms | List interior rooms | POS_RESTING |

---

## Alternative Sessions

If this session is blocked:
1. **Session 05 (Interior Room Generation Wiring)** - Not recommended; explicitly requires Session 04
2. **Session 07 (Persistence Integration)** - Could potentially work independently but breaks logical flow

---

## Next Steps

Run `/sessionspec` to generate the formal specification.
