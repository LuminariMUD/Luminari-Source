# NEXT_SESSION.md

## Session Recommendation

**Generated**: 2025-12-30
**Project State**: Phase 01 - Automation Layer
**Completed Sessions**: 14 total (9 Phase 00 + 5 Phase 01)

---

## Recommended Next Session

**Session ID**: `phase01-session06-scheduled-route-system`
**Session Name**: Scheduled Route System
**Estimated Duration**: 2-4 hours
**Estimated Tasks**: 15-18

---

## Why This Session Next?

### Prerequisites Met
- [x] Session 01: Autopilot data structures complete
- [x] Session 02: Waypoint/route management complete
- [x] Session 03: Path-following logic complete
- [x] Session 04: Autopilot player commands complete
- [x] Session 05: NPC pilot integration complete

### Dependencies
- **Builds on**: NPC pilot integration (Session 05) - scheduled routes utilize NPC pilots for automated vessel operation
- **Enables**: Testing & validation (Session 07) - final phase validation requires all features implemented

### Project Progression
Session 06 is the penultimate session of Phase 01. It completes the automation feature set by adding timer-based scheduling, which allows vessels to operate as ferry services, patrol routes, and other timed operations. This is the natural progression after implementing manual autopilot controls and NPC pilots - now vessels can run autonomously on fixed schedules without player intervention.

---

## Session Overview

### Objective
Implement timer-based route scheduling that allows vessels to operate on fixed schedules, enabling ferry services, patrol routes, and other timed operations.

### Key Deliverables
1. Schedule data structure in vessels.h (start times, intervals)
2. `ship_schedules` database table with persistence
3. Schedule management commands (`setschedule`, `clearschedule`, `showschedule`)
4. Timer integration with game loop for automatic route execution
5. Help file entries for all schedule commands

### Scope Summary
- **In Scope (MVP)**: Schedule data structure, DB persistence, timer integration, player commands, automatic route start at scheduled times, departure announcements
- **Out of Scope**: Complex calendar scheduling, multiple schedules per vessel, schedule conflicts/collision detection, dynamic schedule adjustment

---

## Technical Considerations

### Technologies/Patterns
- Game timer/event system integration (`comm.c` heartbeat)
- MUD time vs real time conversion
- Database persistence pattern (following Session 02 waypoint storage)
- Command registration pattern (interpreter.c)

### Potential Challenges
- Timer precision with game loop tick rate
- Handling server restarts mid-schedule
- Schedule state synchronization with vessel state
- Performance impact of schedule checks in game loop

### Relevant Considerations
- **[Architecture]** Per-vessel type mapping still uses placeholder - schedules should respect vessel types
- **[Lesson]** Auto-create DB tables at startup - `ship_schedules` should follow this pattern
- **[Lesson]** Use #defines for constants, not hardcoded values

---

## Alternative Sessions

If this session is blocked:
1. **phase01-session07-testing-validation** - Could begin partial testing of Sessions 01-05 features, but full validation requires Session 06 completion
2. **Return to Phase 00 technical debt** - Address duplicate struct definitions or hard-coded room templates (not recommended - stay focused on Phase 01)

---

## Next Steps

Run `/sessionspec` to generate the formal specification.
