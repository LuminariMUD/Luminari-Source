# Task Checklist

**Session ID**: `phase00-session04-phase2-command-registration`
**Total Tasks**: 15
**Estimated Duration**: 3-4 hours
**Created**: 2025-12-29

---

## Legend

- `[x]` = Completed
- `[ ]` = Pending
- `[P]` = Parallelizable (can run with other [P] tasks)
- `[S0004]` = Session reference (Phase 00, Session 04)
- `TNNN` = Task ID

---

## Progress Summary

| Category | Total | Done | Remaining |
|----------|-------|------|-----------|
| Setup | 2 | 2 | 0 |
| Foundation | 3 | 3 | 0 |
| Implementation | 6 | 6 | 0 |
| Testing | 4 | 4 | 0 |
| **Total** | **15** | **15** | **0** |

---

## Setup (2 tasks)

Initial verification and environment preparation.

- [x] T001 [S0004] Verify source compiles cleanly before changes (`./cbuild.sh`)
- [x] T002 [S0004] Verify vessels.h include exists in interpreter.c (line 66)

---

## Foundation (3 tasks)

Analyze existing code and determine registration locations.

- [x] T003 [S0004] Identify exact insertion point for commands in cmd_info[] (`src/interpreter.c`)
- [x] T004 [S0004] Verify no existing registrations for dock/undock/board_hostile/look_outside/ship_rooms (`src/interpreter.c`)
- [x] T005 [S0004] Document duplicate disembark issue (lines 385, 1165) in implementation-notes.md

---

## Implementation (6 tasks)

Register commands and create help files.

- [x] T006 [S0004] Add dock command registration to cmd_info[] (`src/interpreter.c`)
- [x] T007 [S0004] Add undock command registration to cmd_info[] (`src/interpreter.c`)
- [x] T008 [S0004] Add board_hostile command registration to cmd_info[] (`src/interpreter.c`)
- [x] T009 [S0004] Add look_outside command registration to cmd_info[] (`src/interpreter.c`)
- [x] T010 [S0004] Add ship_rooms command registration to cmd_info[] (`src/interpreter.c`)
- [x] T011 [S0004] [P] Create help entries for all 5 commands (`lib/text/help/help.hlp`)

---

## Testing (4 tasks)

Verification and quality assurance.

- [x] T012 [S0004] Build project and verify no compilation errors (`./cbuild.sh`)
- [x] T013 [S0004] Verify no new warnings in build output
- [x] T014 [S0004] Validate ASCII encoding on all modified files
- [x] T015 [S0004] Update implementation-notes.md with completion summary

---

## Completion Checklist

Before marking session complete:

- [x] All tasks marked `[x]`
- [x] Build compiles with no errors
- [x] No new warnings introduced
- [x] All files ASCII-encoded
- [x] Unix LF line endings
- [x] implementation-notes.md updated
- [x] Ready for `/validate`

---

## Notes

### Command Registration Format
```c
{"command", "sort_as", minimum_position, do_handler, min_level, subcmd, ignore_wait, action_type, {cooldowns}, check_ptr},
```

### Position Requirements
| Command | Position | Rationale |
|---------|----------|-----------|
| dock | POS_STANDING | Physical boarding action |
| undock | POS_STANDING | Physical disconnection |
| board_hostile | POS_FIGHTING | Combat boarding action |
| look_outside | POS_RESTING | Passive observation |
| ship_rooms | POS_RESTING | Information query |

### Help File Format
Each entry in help.hlp:
```
KEYWORD1 KEYWORD2
<blank line>
Help text content...
<blank line>
See also: RELATED
#0
```

### Dependencies
All 5 command registrations (T006-T010) can technically be done in one edit, but are separated for tracking clarity. T011 (help entries) is parallelizable with any other task.

### Known Issues
- Duplicate disembark registration exists at lines 385 and 1165 - document but do not fix in this session

---

## Next Steps

Run `/implement` to begin AI-led implementation.
