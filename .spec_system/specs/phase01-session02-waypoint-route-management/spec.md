# Session Specification

**Session ID**: `phase01-session02-waypoint-route-management`
**Phase**: 01 - Automation Layer
**Status**: Not Started
**Created**: 2025-12-30

---

## 1. Session Overview

This session implements the waypoint and route data management layer for the vessel autopilot system. Building on the data structures defined in Session 01 (`struct waypoint`, `struct ship_route`, `struct autopilot_data`), this session creates the database schema, CRUD operations, and in-memory caching infrastructure that all subsequent autopilot sessions depend on.

The waypoint/route management system enables vessels to store named navigation points with coordinates and tolerance settings, organize waypoints into ordered routes with loop/one-way behavior, and persist this data across server restarts. This foundational layer is critical because Session 03 (path-following) needs waypoints to navigate between, Session 04 (player commands) needs CRUD operations to manage them, Session 05 (NPC pilots) needs routes to assign, and Session 06 (scheduled routes) needs route persistence.

The implementation follows established patterns from `vessels_db.c` including auto-create tables at startup, MySQL query patterns with proper escaping, and the existing save/load lifecycle hooks.

---

## 2. Objectives

1. Implement database schema with three tables (`ship_waypoints`, `ship_routes`, `ship_route_waypoints`) that auto-create at startup
2. Implement complete waypoint CRUD operations with database persistence in `vessels_autopilot.c`
3. Implement complete route CRUD operations with ordered waypoint sequence management
4. Implement boot-time loading and shutdown saving of all waypoints and routes
5. Create comprehensive unit tests for all CRUD operations with Valgrind-clean memory management

---

## 3. Prerequisites

### Required Sessions
- [x] `phase01-session01-autopilot-data-structures` - Provides `struct waypoint`, `struct ship_route`, `struct autopilot_data` definitions in vessels.h

### Required Tools/Knowledge
- MySQL/MariaDB database operations (see `vessels_db.c` patterns)
- CuTest unit testing framework (see `unittests/CuTest/`)
- C90/C89 coding standards with `/* */` comments only

### Environment Requirements
- MySQL/MariaDB connection functional (`mysql_available` external variable)
- Build system operational (CMake or Autotools)
- Valgrind available for memory leak testing

---

## 4. Scope

### In Scope (MVP)
- Database table creation with foreign key constraints
- Waypoint CRUD: `waypoint_db_create()`, `waypoint_db_load()`, `waypoint_db_update()`, `waypoint_db_delete()`
- Route CRUD: `route_db_create()`, `route_db_load()`, `route_db_update()`, `route_db_delete()`
- Route-waypoint association: `route_add_waypoint()`, `route_remove_waypoint()`, `route_reorder_waypoints()`
- In-memory cache with global waypoint and route lists
- Boot-time loading: `load_all_waypoints()`, `load_all_routes()`
- Shutdown saving: `save_all_waypoints()`, `save_all_routes()`
- Unit tests for all CRUD operations

### Out of Scope (Deferred)
- Autopilot state persistence - *Reason: Later session will handle real-time state*
- Movement/path-following logic - *Reason: Session 03 scope*
- Player-facing commands (waypoint, route, autopilot) - *Reason: Session 04 scope*
- NPC pilot assignment to routes - *Reason: Session 05 scope*
- Scheduled/timed route execution - *Reason: Session 06 scope*

---

## 5. Technical Approach

### Architecture
The waypoint/route system uses a three-tier architecture:
1. **Database Layer**: MySQL tables with foreign key constraints for referential integrity
2. **CRUD Layer**: Functions that translate between database rows and in-memory structures
3. **Cache Layer**: Global linked lists for O(1) lookups during gameplay

```
+------------------+     +------------------+     +------------------+
| ship_waypoints   |     | ship_routes      |     | ship_route_      |
| (DB table)       |<--->| (DB table)       |<--->| waypoints (DB)   |
+------------------+     +------------------+     +------------------+
         |                       |                       |
         v                       v                       v
+------------------+     +------------------+     +------------------+
| waypoint_db_*()  |     | route_db_*()     |     | route_*_waypoint |
| CRUD functions   |     | CRUD functions   |     | functions        |
+------------------+     +------------------+     +------------------+
         |                       |
         v                       v
+------------------+     +------------------+
| waypoint_list    |     | route_list       |
| (global cache)   |     | (global cache)   |
+------------------+     +------------------+
```

### Design Patterns
- **Repository Pattern**: Separate DB operations from business logic (waypoint_db_* vs waypoint_*)
- **Lazy Loading**: Routes load waypoint references, not full waypoint copies
- **Auto-increment IDs**: Database generates unique IDs, returned to caller

### Technology Stack
- MySQL/MariaDB 5.7+ (existing project requirement)
- ANSI C90/C89 (project standard)
- CuTest (existing test framework)

---

## 6. Deliverables

### Files to Create
| File | Purpose | Est. Lines |
|------|---------|------------|
| `sql/waypoint_tables.sql` | Table creation SQL for reference/manual setup | ~60 |
| `unittests/CuTest/test_waypoint_route.c` | Unit tests for all CRUD operations | ~400 |

### Files to Modify
| File | Changes | Est. Lines |
|------|---------|------------|
| `src/vessels_autopilot.c` | Add DB CRUD functions, cache management, boot/shutdown hooks | ~450 |
| `src/vessels.h` | Add new function prototypes, cache globals | ~40 |
| `src/db_init.c` | Add waypoint/route table auto-creation | ~80 |
| `unittests/CuTest/Makefile` | Add test_waypoint_route to build | ~5 |
| `unittests/CuTest/AllTests.c` | Register waypoint/route test suite | ~10 |

---

## 7. Success Criteria

### Functional Requirements
- [ ] All three database tables created automatically at server startup
- [ ] Waypoints can be created with coordinates, name, tolerance, wait_time
- [ ] Waypoints can be listed, updated by ID, and deleted
- [ ] Routes can be created with name and loop flag
- [ ] Routes can have waypoints added in order, removed, and reordered
- [ ] Waypoints and routes persist across server restarts
- [ ] Foreign key constraints prevent orphaned route_waypoint records

### Testing Requirements
- [ ] Unit tests written for waypoint CRUD (create, load, update, delete)
- [ ] Unit tests written for route CRUD (create, load, update, delete)
- [ ] Unit tests written for route-waypoint association management
- [ ] All unit tests passing
- [ ] Manual testing: create waypoint, restart server, verify waypoint exists

### Quality Gates
- [ ] All files ASCII-encoded (0-127 only)
- [ ] Unix LF line endings
- [ ] Code follows C90 conventions (no // comments, declarations at block start)
- [ ] No memory leaks (Valgrind clean)
- [ ] No compiler warnings with -Wall -Wextra
- [ ] Allman-style braces, 2-space indentation

---

## 8. Implementation Notes

### Key Considerations
- Use `mysql_real_escape_string()` for all string values to prevent SQL injection
- Always check `mysql_available` before database operations
- Use `snprintf()` not `sprintf()` for buffer safety
- Return database-generated IDs from create functions
- Cache invalidation: Update cache when DB changes

### Potential Challenges
- **Waypoint ordering in routes**: Use `sequence_num` column to maintain order; implement swap/reorder carefully
- **Memory management**: Each route holds waypoint IDs (not copies); free caches on shutdown
- **Concurrent access**: Single-threaded game loop makes this simpler, but add NULL checks everywhere
- **ID generation**: Let MySQL auto-increment handle this; retrieve with `mysql_insert_id()`

### Relevant Considerations
- [P00] **Auto-create DB tables at startup**: Continue this proven pattern; add table creation to `db_init.c` boot sequence
- [P00] **MySQL/MariaDB required**: All CRUD functions must check `mysql_available` and return gracefully if unavailable
- [P00] **Don't use C99/C11 features**: Stick to C90; declare all variables at block start; use /* */ comments only
- [P00] **CuTest for unit testing**: Add test file to `unittests/CuTest/`; follow existing Arrange-Act-Assert pattern

### ASCII Reminder
All output files must use ASCII-only characters (0-127). No smart quotes, em-dashes, or non-ASCII symbols.

---

## 9. Testing Strategy

### Unit Tests
- `test_waypoint_create`: Create waypoint, verify all fields stored correctly
- `test_waypoint_load`: Create, load by ID, verify match
- `test_waypoint_update`: Create, modify, update, reload, verify changes
- `test_waypoint_delete`: Create, delete, verify load fails
- `test_route_create`: Create route, verify fields
- `test_route_load`: Create, load by ID, verify match
- `test_route_update`: Modify name/loop flag, verify persistence
- `test_route_delete`: Create, delete, verify load fails
- `test_route_add_waypoint`: Add waypoints, verify order preserved
- `test_route_remove_waypoint`: Remove middle waypoint, verify resequencing
- `test_route_cascade_delete`: Delete route, verify route_waypoints removed
- `test_memory_cleanup`: Create/delete many items, run Valgrind

### Integration Tests
- Boot server with existing waypoints/routes in DB, verify loaded correctly
- Create waypoint via code, restart server, verify persistence
- Create route with waypoints, restart, verify route and waypoint associations intact

### Manual Testing
- Connect to running server
- Use future Session 04 commands (or direct DB queries) to create waypoints
- Restart server, verify data persists
- Delete waypoint that's part of a route, verify route integrity

### Edge Cases
- Create waypoint with empty name (should work, name optional)
- Create waypoint with MAX coordinates (2048, 2048, max_z)
- Create route with 0 waypoints (valid empty route)
- Create route with MAX_WAYPOINTS_PER_ROUTE waypoints
- Delete waypoint that's referenced by multiple routes
- Load non-existent waypoint ID (should return NULL/0)

---

## 10. Dependencies

### External Libraries
- MySQL/MariaDB C connector (libmysqlclient/libmariadb)
- CuTest (bundled in unittests/CuTest/)

### Other Sessions
- **Depends on**: `phase01-session01-autopilot-data-structures` (structs defined)
- **Depended by**:
  - `phase01-session03-path-following-logic` (needs waypoints to navigate)
  - `phase01-session04-autopilot-player-commands` (needs CRUD to expose)
  - `phase01-session05-npc-pilot-integration` (needs routes to assign)
  - `phase01-session06-scheduled-route-system` (needs route persistence)

---

## Database Schema Reference

```sql
-- Waypoints: Individual navigation points
CREATE TABLE IF NOT EXISTS ship_waypoints (
  waypoint_id INT AUTO_INCREMENT PRIMARY KEY,
  name VARCHAR(64) DEFAULT '',
  x FLOAT NOT NULL,
  y FLOAT NOT NULL,
  z FLOAT NOT NULL DEFAULT 0,
  tolerance FLOAT NOT NULL DEFAULT 5.0,
  wait_time INT NOT NULL DEFAULT 0,
  flags INT NOT NULL DEFAULT 0,
  created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
  updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- Routes: Named collections of waypoints
CREATE TABLE IF NOT EXISTS ship_routes (
  route_id INT AUTO_INCREMENT PRIMARY KEY,
  name VARCHAR(64) NOT NULL,
  loop_route TINYINT(1) NOT NULL DEFAULT 0,
  active TINYINT(1) NOT NULL DEFAULT 1,
  created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
  updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- Route-Waypoint associations with ordering
CREATE TABLE IF NOT EXISTS ship_route_waypoints (
  id INT AUTO_INCREMENT PRIMARY KEY,
  route_id INT NOT NULL,
  waypoint_id INT NOT NULL,
  sequence_num INT NOT NULL,
  FOREIGN KEY (route_id) REFERENCES ship_routes(route_id) ON DELETE CASCADE,
  FOREIGN KEY (waypoint_id) REFERENCES ship_waypoints(waypoint_id) ON DELETE CASCADE,
  UNIQUE KEY route_sequence (route_id, sequence_num)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
```

---

## Next Steps

Run `/tasks` to generate the implementation task checklist.
