# NEXT_SESSION.md

## Session Recommendation

**Generated**: 2025-12-30
**Project State**: Phase 01 - Automation Layer
**Completed Sessions**: 13 (9 Phase 00 + 4 Phase 01)

---

## Recommended Next Session

**Session ID**: `phase01-session05-npc-pilot-integration`
**Session Name**: NPC Pilot Integration
**Estimated Duration**: 2-4 hours
**Estimated Tasks**: 15-20

---

## Why This Session Next?

### Prerequisites Met
- [x] Session 01: Autopilot data structures complete
- [x] Session 02: Waypoint/route management complete
- [x] Session 03: Path-following logic complete
- [x] Session 04: Autopilot player commands complete

### Dependencies
- **Builds on**: Session 04 (autopilot player commands) - leverages the complete autopilot system
- **Enables**: Session 06 (scheduled routes - pilots announce departures/arrivals)

### Project Progression
NPC Pilot Integration is the natural next step after implementing player-controlled autopilot. The autopilot system (sessions 01-04) provides the foundation for automated vessel navigation. This session extends that by allowing NPCs to operate vessels autonomously, which is essential for ferry services, patrol routes, and transport NPCs. Session 06 (Scheduled Routes) explicitly depends on pilots for announcing departures.

---

## Session Overview

### Objective
Enable NPC characters to serve as vessel pilots, operating vessels along assigned routes with appropriate behavior and responses.

### Key Deliverables
1. NPC pilot assignment data structure and association
2. `assignpilot <npc>` and `unassignpilot` command handlers
3. Pilot behavior functions (arrival announcements, status reports)
4. Database persistence for pilot assignments (ship_crew_roster integration)
5. Integration with autopilot tick (pilot controls vessel automatically)
6. Commands registered in interpreter.c with help file entries

### Scope Summary
- **In Scope (MVP)**: Pilot assignment/unassignment, basic pilot behavior (announcements), persistence, autopilot integration
- **Out of Scope**: Complex NPC AI, crew beyond pilot role, training/skills, dynamic hiring/wages

---

## Technical Considerations

### Technologies/Patterns
- NPC (mob) data structures (`struct char_data`)
- Existing `ship_crew_roster` database table
- Autopilot tick processing from Session 03
- Command registration pattern from interpreter.c

### Potential Challenges
- NPC-vessel association must survive server reboots
- Pilot must remain with vessel during travel (not left behind)
- Need to validate NPC is suitable for pilot role
- Integration with existing crew roster table structure

### Relevant Considerations
- **[P00] Auto-create DB tables at startup**: Pattern works well - pilot data should follow same approach
- **[P00] Don't use C99/C11 features**: Ensure ANSI C90 compliance in new code
- **[P00] Don't skip NULL checks**: Critical for NPC pointer handling

---

## Alternative Sessions

If this session is blocked:
1. **Session 07 (Testing Validation)** - Could begin unit tests for sessions 01-04 while deferring pilot/schedule integration tests. Partial testing is better than no testing.
2. **Phase 02 Planning** - If Phase 01 sessions 05-06 are blocked by external dependencies, could begin planning Simple Vehicle Support phase.

---

## Next Steps

Run `/sessionspec` to generate the formal specification.
