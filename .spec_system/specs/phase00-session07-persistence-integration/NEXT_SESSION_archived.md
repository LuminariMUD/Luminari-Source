# NEXT_SESSION.md

## Session Recommendation

**Generated**: 2025-12-29
**Project State**: Phase 00 - Core Vessel System
**Completed Sessions**: 6 of 9

---

## Recommended Next Session

**Session ID**: `phase00-session07-persistence-integration`
**Session Name**: Persistence Integration
**Estimated Duration**: 3-4 hours
**Estimated Tasks**: 18-22

---

## Why This Session Next?

### Prerequisites Met
- [x] Session 01: Header Cleanup Foundation
- [x] Session 02: Dynamic Wilderness Room Allocation
- [x] Session 03: Vessel Type System
- [x] Session 04: Phase 2 Command Registration
- [x] Session 05: Interior Room Generation Wiring
- [x] Session 06: Interior Movement Implementation
- [x] Database tables exist (auto-created at startup)

### Dependencies
- **Builds on**: Session 06 (Interior Movement) - movement and interiors must work before persisting state
- **Enables**: Session 08 (External View & Display Systems) - requires persistence for complete functionality

### Project Progression

Session 07 is the clear next step because:
1. All functional vessel features (movement, interiors, commands) are now implemented
2. Persistence is required before external view/display systems to ensure state survives restarts
3. Testing (Session 09) cannot be comprehensive without persistence working
4. This follows the natural pattern: implement functionality first, then persist it

---

## Session Overview

### Objective
Wire the persistence layer to save and load vessel state, including ship interiors, docking records, cargo manifests, and crew rosters.

### Key Deliverables
1. Ship interiors persist to database and restore on server restart
2. Docking records persist correctly with proper status tracking
3. Save/load cycle verified with no data loss on normal shutdown
4. Foreign key relationships maintained (CASCADE DELETE working)

### Scope Summary
- **In Scope (MVP)**: Wire save_ship_interior(), load_ship_interior(), docking persistence, save points, load points
- **Out of Scope**: Adding new persistence features, performance optimization, data compression

---

## Technical Considerations

### Technologies/Patterns
- MySQL/MariaDB persistence using existing vessels_db.c functions
- Integration with db.c boot sequence for load operations
- Integration with vessels.c for save trigger points
- Foreign key constraints (ship_interiors parent table)

### Potential Challenges
- Ensuring save_ship_interior() is called at all appropriate save points (creation, modification, shutdown)
- Coordinating load_ship_interior() with boot sequence timing
- Handling data migration for any existing ships
- Maintaining data integrity across server restarts

### Relevant Considerations
- **[P00] Technical Debt**: Room templates hard-coded in vessels_rooms.c - may affect persistence if template IDs don't match
- **[P00] External Dependencies**: MySQL/MariaDB required - ensure DB connection is validated before persistence operations
- **[P00] Lesson Learned**: Auto-create DB tables at startup ensures schema matches runtime code exactly

---

## Alternative Sessions

If this session is blocked:
1. **Session 08 (External View & Display Systems)** - Could implement display systems first, but persistence is prerequisite per session 08 definition
2. **Session 09 (Testing & Validation)** - Requires sessions 01-08 complete; cannot proceed before persistence

**Note**: No true alternatives exist - session 07 is the critical path.

---

## Next Steps

Run `/sessionspec` to generate the formal specification.
