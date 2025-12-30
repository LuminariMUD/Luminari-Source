# NEXT_SESSION.md

## Session Recommendation

**Generated**: 2025-12-30
**Project State**: Phase 01 - Automation Layer
**Completed Sessions**: 10 (9 from Phase 00, 1 from Phase 01)

---

## Recommended Next Session

**Session ID**: `phase01-session02-waypoint-route-management`
**Session Name**: Waypoint Route Management
**Estimated Duration**: 2-4 hours
**Estimated Tasks**: 15-20

---

## Why This Session Next?

### Prerequisites Met
- [x] Session 01 complete (autopilot data structures defined in vessels.h)
- [x] MySQL/MariaDB connection functional
- [x] Understanding of existing vessel DB patterns (vessels_db.c established in Phase 00)

### Dependencies
- **Builds on**: phase01-session01-autopilot-data-structures (waypoint_data, route_data, autopilot_state structs)
- **Enables**: phase01-session03-path-following-logic (needs waypoints/routes to follow)

### Project Progression
Session 02 is the natural next step in the Automation Layer. Session 01 defined the data structures (`waypoint_data`, `route_data`, `route_waypoint`, `autopilot_state`), and now Session 02 implements the persistence and management layer for waypoints and routes. This creates the foundation that all subsequent autopilot sessions depend on:

- Session 03 (path-following) needs waypoints to navigate between
- Session 04 (player commands) needs CRUD operations to manage waypoints/routes
- Session 05 (NPC pilots) needs routes to assign to NPCs
- Session 06 (scheduled routes) needs route persistence

---

## Session Overview

### Objective
Implement waypoint and route data management including database schema, CRUD operations, and in-memory caching.

### Key Deliverables
1. Database schema for `ship_waypoints`, `ship_routes`, `ship_route_waypoints` tables
2. Waypoint CRUD functions (create, read, update, delete) in `vessels_autopilot.c`
3. Route CRUD functions with ordered waypoint sequence management
4. Boot-time loading of waypoints and routes from database
5. Database init code in `db_init.c` for new tables (auto-create pattern)
6. Unit tests for all CRUD operations

### Scope Summary
- **In Scope (MVP)**: DB tables, CRUD functions, boot-time loading, in-memory caching, persistence, unit tests
- **Out of Scope**: Autopilot state persistence (later session), movement logic (Session 03), player commands (Session 04)

---

## Technical Considerations

### Technologies/Patterns
- MySQL/MariaDB for persistence (existing pattern from vessels_db.c)
- Auto-create tables at startup (proven pattern from Phase 00)
- In-memory caching for performance (avoid DB lookups during gameplay)
- Foreign key constraints for data integrity
- ANSI C90/C89 coding standards

### Potential Challenges
- Ensuring proper memory management for waypoint/route linked lists
- Maintaining referential integrity between routes and waypoints
- Handling concurrent access to waypoint/route caches (if relevant)
- Matching existing vessel DB patterns for consistency

### Relevant Considerations
- [P00] **Auto-create DB tables at startup**: Continue this proven pattern for new waypoint/route tables
- [P00] **MySQL/MariaDB required**: Ensure database connection is validated before CRUD operations
- [P00] **Don't use C99/C11 features**: Stick to ANSI C90/C89 for all new code
- [P00] **CuTest for unit testing**: Add comprehensive tests in unittests/CuTest/

---

## Alternative Sessions

If this session is blocked:
1. **phase01-session04-autopilot-player-commands** - Could implement command stubs with placeholder data (not recommended - breaks dependency chain)
2. **Return to Phase 00 tech debt** - Address duplicate struct definitions in vessels.h if DB access is blocked

---

## Success Criteria

- [ ] All three new tables created at server startup
- [ ] Waypoints can be created, listed, updated, deleted
- [ ] Routes can be created with ordered waypoint sequences
- [ ] Waypoints/routes persist across server restarts
- [ ] Foreign key constraints enforce data integrity
- [ ] Unit tests pass for all CRUD operations
- [ ] No memory leaks in waypoint/route management (Valgrind clean)

---

## Next Steps

Run `/sessionspec` to generate the formal specification.
