# NEXT_SESSION.md

## Session Recommendation

**Generated**: 2025-12-30
**Project State**: Phase 01 - Automation Layer
**Completed Sessions**: 9 (Phase 00 complete)

---

## Recommended Next Session

**Session ID**: `phase01-session01-autopilot-data-structures`
**Session Name**: Autopilot Data Structures
**Estimated Duration**: 2-4 hours
**Estimated Tasks**: 15-20

---

## Why This Session Next?

### Prerequisites Met
- [x] Phase 00 complete and validated (9/9 sessions)
- [x] vessels.h readable and modifiable
- [x] greyhawk_ship_data structure exists and documented
- [x] MySQL/MariaDB operational with vessel tables
- [x] Wilderness coordinate system functional
- [x] Basic vessel movement working

### Dependencies
- **Builds on**: Phase 00 Core Vessel System (complete)
- **Enables**: Session 02 (Waypoint Route Management), Session 03 (Path Following Logic)

### Project Progression
This is the natural first session of Phase 01 (Automation Layer). It establishes the foundational data structures that all subsequent automation sessions depend on. Without these structures, no autopilot functionality can be implemented. This is a "headers-first" foundational session that follows the proven pattern from Phase 00 Session 01.

---

## Session Overview

### Objective
Define and implement the core data structures, constants, and header declarations required for the vessel autopilot system.

### Key Deliverables
1. Updated `src/vessels.h` with autopilot structures and prototypes
2. New `src/vessels_autopilot.c` skeleton file
3. Constants defined for autopilot limits and timing
4. Build system updated (CMakeLists.txt) to compile new source
5. Unit test file skeleton for autopilot structures

### Scope Summary
- **In Scope (MVP)**: autopilot_data struct, waypoint struct, ship_route struct, constants, greyhawk_ship_data extension, function prototypes, source file skeleton
- **Out of Scope**: Movement logic (Session 03), database persistence (Session 02), player commands (Session 04), NPC integration (Session 05)

---

## Technical Considerations

### Technologies/Patterns
- ANSI C90/C89 (no C99 features)
- Follow existing vessels.h structure and conventions
- Use header guards and proper documentation
- Integrate with existing greyhawk_ship_data via pointer

### Potential Challenges
- Avoiding duplicate struct definitions (Active Concern from Phase 00)
- Proper integration with existing complex vessels.h header
- Ensuring clean separation between autopilot and manual control states

### Relevant Considerations
- [P00] **Duplicate struct definitions**: Phase 00 left duplicate definitions in vessels.h - be careful not to introduce more
- [P00] **ANSI C90/C89 codebase**: No modern C features (no // comments, no declarations after statements)
- [P00] **Max 500 concurrent vessels**: Autopilot structures must be memory-efficient

---

## Alternative Sessions

If this session is blocked:
1. **phase01-session02-waypoint-route-management** - Could start DB schema first, but structures needed for proper table design
2. **None recommended** - Session 01 has no blockers and is the proper foundation

---

## Next Steps

Run `/sessionspec` to generate the formal specification.
