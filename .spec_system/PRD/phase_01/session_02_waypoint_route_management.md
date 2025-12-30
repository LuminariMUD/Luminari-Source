# Session 02: Waypoint Route Management

**Session ID**: `phase01-session02-waypoint-route-management`
**Status**: Not Started
**Estimated Tasks**: ~15-20
**Estimated Duration**: 2-4 hours

---

## Objective

Implement waypoint and route data management including database schema, CRUD operations, and in-memory caching.

---

## Scope

### In Scope (MVP)
- Create database tables for waypoints, routes, and route-waypoint mappings
- Implement waypoint CRUD functions (create, read, update, delete)
- Implement route CRUD functions
- Implement route-waypoint association management
- Load waypoints/routes from database at boot
- Save waypoints/routes to database
- In-memory waypoint/route cache for performance

### Out of Scope
- Autopilot state persistence (deferred to later session)
- Movement logic using waypoints (Session 03)
- Player-facing commands (Session 04)

---

## Prerequisites

- [ ] Session 01 complete (data structures defined)
- [ ] MySQL/MariaDB connection functional
- [ ] Understanding of existing vessel DB patterns (vessels_db.c)

---

## Deliverables

1. Database schema for `ship_waypoints`, `ship_routes`, `ship_route_waypoints`
2. Waypoint management functions in `vessels_autopilot.c`
3. Route management functions in `vessels_autopilot.c`
4. Boot-time loading of waypoints and routes
5. Database init code in `db_init.c` for new tables
6. Unit tests for waypoint/route CRUD operations

---

## Success Criteria

- [ ] All three new tables created at server startup
- [ ] Waypoints can be created, listed, updated, deleted
- [ ] Routes can be created with ordered waypoint sequences
- [ ] Waypoints/routes persist across server restarts
- [ ] Foreign key constraints enforce data integrity
- [ ] Unit tests pass for all CRUD operations
- [ ] No memory leaks in waypoint/route management
