# PRD Phase 01: Automation Layer

**Status**: In Progress
**Sessions**: 7 (initial estimate)
**Estimated Duration**: 3-5 days

**Progress**: 6/7 sessions (86%)

---

## Overview

Implement vessel automation capabilities including autopilot systems, waypoint-based navigation, NPC pilot integration, and scheduled route execution. This phase transforms vessels from player-only controlled craft into autonomous navigation platforms that can follow predefined paths and schedules.

---

## Progress Tracker

| Session | Name | Status | Est. Tasks | Validated |
|---------|------|--------|------------|-----------|
| 01 | Autopilot Data Structures | Complete | 21 | 2025-12-30 |
| 02 | Waypoint Route Management | Complete | 20 | 2025-12-30 |
| 03 | Path Following Logic | Complete | 20 | 2025-12-30 |
| 04 | Autopilot Player Commands | Complete | 22 | 2025-12-30 |
| 05 | NPC Pilot Integration | Complete | 20 | 2025-12-30 |
| 06 | Scheduled Route System | Complete | 20 | 2025-12-30 |
| 07 | Testing Validation | Not Started | ~12-15 | - |

---

## Completed Sessions

### Session 01: Autopilot Data Structures
- **Completed**: 2025-12-30
- **Tasks**: 21/21
- **Key Deliverables**: Autopilot structures (waypoint, ship_route, autopilot_data), constants, function prototypes, vessels_autopilot.c skeleton, unit tests (14 tests passing)

### Session 02: Waypoint Route Management
- **Completed**: 2025-12-30
- **Tasks**: 20/20
- **Key Deliverables**: Database schema (3 tables), waypoint CRUD operations, route CRUD operations, cache layer (waypoint_list, route_list), boot/shutdown loading, unit tests (11 tests passing)

### Session 03: Path Following Logic
- **Completed**: 2025-12-30
- **Tasks**: 20/20
- **Key Deliverables**: Distance/heading calculations, tolerance-based arrival detection, waypoint advancement with loop support, wait time handling, terrain-validated movement, heartbeat integration, unit tests (30 tests passing)

### Session 04: Autopilot Player Commands
- **Completed**: 2025-12-30
- **Tasks**: 21/22 (T020 deferred by design)
- **Key Deliverables**: 8 ACMD command handlers (autopilot, setwaypoint, listwaypoints, delwaypoint, createroute, addtoroute, listroutes, setroute), interpreter.c registration, autopilot.hlp help file, unit tests (44 tests passing)

### Session 05: NPC Pilot Integration
- **Completed**: 2025-12-30
- **Tasks**: 20/20
- **Key Deliverables**: NPC pilot assignment commands (assignpilot, unassignpilot), pilot validation functions, pilot announcements on waypoint arrival, pilot persistence via ship_crew_roster, auto-engage autopilot for piloted vessels, unit tests (12 tests passing)

### Session 06: Scheduled Route System
- **Completed**: 2025-12-30
- **Tasks**: 20/20
- **Key Deliverables**: Schedule data structure (vessel_schedule), ship_schedules database table, schedule management commands (setschedule, clearschedule, showschedule), timer integration with MUD hour heartbeat, automatic route triggering, schedule persistence, unit tests (17 tests passing, 164 total)

---

## Upcoming Sessions

- Session 07: Testing Validation

---

## Objectives

1. **Autopilot System** - Enable vessels to navigate autonomously between waypoints without continuous player input
2. **Waypoint Management** - Provide data structures and commands for defining, storing, and managing navigation waypoints and routes
3. **NPC Pilot Integration** - Allow NPC characters to operate as vessel pilots, following routes and responding to events
4. **Scheduled Operations** - Implement timer-based route execution for ferry services and patrol routes
5. **Persistence** - Save and load autopilot state, waypoints, and routes via database

---

## Prerequisites

- Phase 00 completed (Core Vessel System validated)
- MySQL/MariaDB operational with vessel tables
- Wilderness coordinate system functional
- Basic vessel movement working

---

## Technical Considerations

### Architecture

The automation layer builds on the existing vessel movement system:
- Autopilot state stored in `greyhawk_ship_data` structure extension
- Waypoints stored in new `ship_waypoints` database table
- Routes stored in new `ship_routes` database table
- Event hooks into existing movement tick system
- NPC pilot assignments tracked via `ship_crew_roster` table

### Technologies
- ANSI C90/C89 (no C99 features)
- MySQL/MariaDB for persistence
- CuTest for unit testing
- Existing wilderness coordinate system

### Data Structures (Planned)

```c
/* Autopilot state */
struct autopilot_data {
  bool enabled;
  int current_waypoint_idx;
  int route_id;
  int speed_setting;
  bool loop_route;
  time_t last_move_time;
};

/* Waypoint definition */
struct waypoint {
  int id;
  int x, y, z;
  char name[64];
  int wait_time;  /* seconds to pause at waypoint */
};

/* Route definition */
struct ship_route {
  int route_id;
  char name[128];
  int waypoint_count;
  struct waypoint *waypoints;
  bool is_loop;
};
```

### Risks
- **Timer integration complexity**: Autopilot requires reliable tick-based movement; may need careful integration with game loop
- **Path validation**: Routes must validate terrain traversability for vessel type before starting
- **State synchronization**: Autopilot state must persist across reboots and handle interrupted journeys
- **NPC behavior conflicts**: NPC pilots may conflict with existing NPC AI systems

### Relevant Considerations
<!-- From CONSIDERATIONS.md -->
- [P00] **MySQL/MariaDB required**: All autopilot data must use database persistence
- [P00] **Wilderness system required**: Autopilot navigation depends on coordinate system
- [P00] **ANSI C90/C89 codebase**: No modern C features allowed
- [P00] **Max 500 concurrent vessels**: Autopilot tick processing must scale efficiently

---

## Success Criteria

Phase complete when:
- [ ] All 7 sessions completed
- [ ] Autopilot data structures defined and integrated
- [ ] Waypoint/route CRUD operations functional
- [ ] Vessels can follow multi-waypoint routes autonomously
- [ ] Player commands for autopilot control registered and working
- [ ] NPC pilots can operate vessels on routes
- [ ] Scheduled routes execute on timers
- [ ] All autopilot state persists across server restarts
- [ ] Unit tests pass with >90% coverage
- [ ] Valgrind clean (no memory leaks)
- [ ] Performance acceptable with 100+ autopilot vessels

---

## Dependencies

### Depends On
- Phase 00: Core Vessel System (complete)

### Enables
- Phase 02: Simple Vehicle Support
- Phase 03: Optimization & Polish

---

## Database Schema (Planned)

### New Tables

```sql
/* Waypoints table */
CREATE TABLE IF NOT EXISTS ship_waypoints (
  waypoint_id INT AUTO_INCREMENT PRIMARY KEY,
  name VARCHAR(64) NOT NULL,
  x_coord INT NOT NULL,
  y_coord INT NOT NULL,
  z_coord INT DEFAULT 0,
  wait_time INT DEFAULT 0,
  created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

/* Routes table */
CREATE TABLE IF NOT EXISTS ship_routes (
  route_id INT AUTO_INCREMENT PRIMARY KEY,
  name VARCHAR(128) NOT NULL,
  is_loop BOOLEAN DEFAULT FALSE,
  created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

/* Route-waypoint mapping */
CREATE TABLE IF NOT EXISTS ship_route_waypoints (
  id INT AUTO_INCREMENT PRIMARY KEY,
  route_id INT NOT NULL,
  waypoint_id INT NOT NULL,
  sequence_order INT NOT NULL,
  FOREIGN KEY (route_id) REFERENCES ship_routes(route_id) ON DELETE CASCADE,
  FOREIGN KEY (waypoint_id) REFERENCES ship_waypoints(waypoint_id) ON DELETE CASCADE
);

/* Autopilot state */
CREATE TABLE IF NOT EXISTS ship_autopilot (
  ship_id VARCHAR(8) PRIMARY KEY,
  route_id INT,
  current_waypoint_idx INT DEFAULT 0,
  enabled BOOLEAN DEFAULT FALSE,
  speed_setting INT DEFAULT 5,
  last_move_time TIMESTAMP,
  FOREIGN KEY (ship_id) REFERENCES ship_interiors(ship_id) ON DELETE CASCADE,
  FOREIGN KEY (route_id) REFERENCES ship_routes(route_id) ON DELETE SET NULL
);
```

---

## Command Interface (Planned)

| Command | Description |
|---------|-------------|
| `autopilot on/off` | Enable/disable autopilot |
| `autopilot status` | Show current autopilot state |
| `setwaypoint <name>` | Create waypoint at current location |
| `listwaypoints` | Show all waypoints |
| `delwaypoint <id>` | Delete a waypoint |
| `createroute <name>` | Create a new route |
| `addtroute <route> <waypoint>` | Add waypoint to route |
| `listroutes` | Show all routes |
| `setroute <route>` | Assign route to current vessel |
| `assignpilot <npc>` | Assign NPC as vessel pilot |

---

*Phase 01 of LuminariMUD Vessel System*
