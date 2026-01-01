# LuminariMUD Vessel System - Product Requirements Document

**Document Version:** 2.3
**Created:** December 29, 2025
**Last Updated:** December 30, 2025
**Project Code:** VESSELS-UNIFIED-2025
**Status:** Complete - All Phases Delivered (2025-12-30)

---

## 1. Executive Summary

### Vision

Create a unified vessel system for LuminariMUD that leverages the wilderness coordinate system, enabling vessels to navigate freely across the 2048x2048 game world with terrain-aware movement, elevation support (airships/submarines), and multi-room ship interiors.

### Strategic Decision

Use Greyhawk naval system as foundation with wilderness integration, replacing three separate legacy vessel systems (CWG, Outcast, Greyhawk) with one maintainable solution.

### Key Outcomes

- Free-roaming vessels across entire wilderness map
- Terrain-based movement restrictions and speed modifiers
- Z-axis navigation for aerial and submersible vessels
- Dynamic interior room generation per vessel type
- Persistence of vessel state, cargo, and crew

---

## 2. Project Goals

1. **Full Wilderness Integration** - Vessels navigate using wilderness coordinates (-1024 to +1024) without track restrictions
2. **Terrain-Aware Navigation** - Different vessel types interact realistically with terrain (water depth, altitude)
3. **Elevation Support** - Enable airships (above terrain) and submarines (below waterline)
4. **Multi-Room Interiors** - Ships contain navigable interior rooms (bridge, quarters, cargo, etc.)
5. **Environmental Integration** - Weather affects movement speed and visibility
6. **Ship-to-Ship Interaction** - Docking, boarding, and combat between vessels

---

## 3. System Architecture

### High-Level Architecture

```
UNIFIED VESSEL SYSTEM
    |
    +-- Wilderness Coordinate System (X, Y, Z navigation)
    +-- Greyhawk Foundation (command interface, ship systems)
    +-- Multi-Room Interiors (Outcast-derived room generation)
    +-- Terrain Integration (sector types, elevation, weather)
    +-- Database Persistence (MySQL tables for state)
```

### Source File Layout

| File | Purpose |
|------|---------|
| `src/vessels.h` | Shared structures, constants, prototypes |
| `src/vessels.c` | Core Greyhawk commands, wilderness movement |
| `src/vessels_rooms.c` | Interior room generation and templates |
| `src/vessels_docking.c` | Ship-to-ship docking and boarding |
| `src/vessels_db.c` | MySQL persistence helpers |
| `src/db_init.c` | Auto-creates vessel tables at startup |
| `src/db_init_data.c` | Populates default room templates |

### Key Constants

| Constant | Value | Description |
|----------|-------|-------------|
| `GREYHAWK_MAXSHIPS` | 500 | Maximum concurrent vessels |
| `MAX_SHIP_ROOMS` | 20 | Maximum interior rooms per vessel |
| `MAX_DOCKING_RANGE` | 2.0 | Maximum distance for docking |
| `BOARDING_DIFFICULTY` | 15 | DC for hostile boarding attempts |
| Interior VNUM base | 30000 | Reserved range for generated rooms |

---

## 4. Vessel Classifications

### Vessel Types (enum vessel_class)

| Type | Description | Terrain Capabilities |
|------|-------------|---------------------|
| `VESSEL_RAFT` | Small, basic | Rivers, shallow water only |
| `VESSEL_BOAT` | Medium craft | Coastal waters |
| `VESSEL_SHIP` | Large vessel | Ocean-capable |
| `VESSEL_WARSHIP` | Combat vessel | Ocean, heavily armed |
| `VESSEL_AIRSHIP` | Flying vessel | Ignores terrain, altitude navigation |
| `VESSEL_SUBMARINE` | Underwater | Depth navigation below waterline |
| `VESSEL_TRANSPORT` | Cargo/passenger | Ocean, high capacity |
| `VESSEL_MAGICAL` | Special | Unique navigation capabilities |

### Terrain Capabilities Structure

```c
struct vessel_terrain_caps {
  bool can_traverse_ocean;       /* Deep water navigation */
  bool can_traverse_shallow;     /* Shallow water/rivers */
  bool can_traverse_air;         /* Airship flight */
  bool can_traverse_underwater;  /* Submarine diving */
  int min_water_depth;           /* Minimum depth required */
  int max_altitude;              /* Maximum flight altitude */
  float terrain_speed_mod[40];   /* Speed modifier by sector type */
};
```

### Terrain Speed Modifiers (Surface Vessels)

| Terrain | Speed Modifier |
|---------|---------------|
| Ocean/Deep Water | 100% |
| Shallow Water | 75% |
| Land/Mountains | 0% (impassable) |
| Storm conditions | -25% |

---

## 5. Command Interface

### Phase 1 Commands (Registered)

| Command | Status | Description |
|---------|--------|-------------|
| `board` | Functional | Enter a vessel via ITEM_GREYHAWK_SHIP object |
| `shipstatus` | Functional | Display coordinates, terrain, weather |
| `speed <0-10>` | Functional | Set vessel speed with terrain modifiers |
| `heading <dir>` | Functional | Set direction (8 compass + up/down) |
| `tactical` | Placeholder | Display tactical map (not implemented) |
| `contacts` | Placeholder | Show nearby vessels (not implemented) |
| `disembark` | Placeholder | Exit vessel (not implemented) |
| `shipload` | Placeholder | Load cargo (not implemented) |
| `setsail` | Placeholder | Initiate travel (not implemented) |

### Phase 2 Commands (Implemented, Not Registered)

| Command | Handler | Description |
|---------|---------|-------------|
| `dock [ship]` | `do_dock` | Create gangway to adjacent vessel |
| `undock` | `do_undock` | Remove docking connection |
| `board_hostile <ship>` | `do_board_hostile` | Forced boarding attempt |
| `look_outside` | `do_look_outside` | View exterior from interior |
| `ship_rooms` | `do_ship_rooms` | List interior rooms |

---

## 6. Database Schema

### Tables (Auto-created at startup)

| Table | Primary Key | Purpose |
|-------|-------------|---------|
| `ship_interiors` | `ship_id VARCHAR(8)` | Vessel configuration, room data |
| `ship_docking` | `dock_id INT AUTO_INCREMENT` | Active/historical docking records |
| `ship_room_templates` | `template_id INT AUTO_INCREMENT` | 19 pre-configured room types |
| `ship_cargo_manifest` | `manifest_id INT AUTO_INCREMENT` | Cargo tracking (FK to ship_interiors) |
| `ship_crew_roster` | `crew_id INT AUTO_INCREMENT` | NPC crew assignments (FK to ship_interiors) |

### Room Templates (19 default types)

- **Control:** bridge, helm
- **Quarters:** quarters_captain, quarters_crew, quarters_officer
- **Cargo:** cargo_main, cargo_secure
- **Engineering:** engineering, weapons, armory
- **Common:** mess_hall, galley, infirmary
- **Connectivity:** corridor, deck_main, deck_lower
- **Special:** airlock, observation, brig

### Interior VNUM Allocation

```
Formula: 30000 + (ship->shipnum * MAX_SHIP_ROOMS) + room_index
Range:   30000 - 40019 (reserved, do not use for builder zones)
```

---

## 7. Implementation Phases

### Phases Overview

| Phase | Name | Sessions | Status |
|-------|------|----------|--------|
| 00 | Core Vessel System | 9 | Complete (2025-12-30) |
| 01 | Automation Layer | 7 | Complete (2025-12-30) |
| 02 | Simple Vehicle Support | 7 | Complete (2025-12-30) |
| 03 | Optimization & Polish | 6 | Complete (2025-12-30) |

### Phase 00: Core Vessel System

Complete wiring of existing Phase 1 and Phase 2 code.

#### Phase 1 Foundation Status (Partially Complete)

| Component | Status | Notes |
|-----------|--------|-------|
| Build integration | Complete | CMake and autotools compile all vessel sources |
| Boot initialization | Complete | `greyhawk_initialize_ships()` called from db.c |
| Command registration | Complete | 9 commands registered in interpreter.c |
| Wilderness coordinates | Complete | X/Y/Z navigation with -1024 to +1024 range |
| Terrain restrictions | Complete | Speed modifiers by sector type |
| Weather effects | Complete | Storms reduce movement 25% |
| Boarding mechanic | Complete | Via ITEM_GREYHAWK_SHIP + spec proc |

**Known Limitations:**
- Vessel movement does not allocate new dynamic wilderness rooms. Movement fails if destination coordinate has no pre-allocated room. Fix requires using `find_available_wilderness_room()` + `assign_wilderness_room()` pattern from `src/movement.c`.
- Wilderness movement currently uses placeholder `VESSEL_TYPE_SAILING_SHIP`; per-ship vessel_class mapping not yet implemented.

#### Phase 2 Multi-Room Status (Code exists, wiring incomplete)

| Component | Status | Notes |
|-----------|--------|-------|
| Room generation code | Complete | `generate_ship_interior()` in vessels_rooms.c |
| Docking handlers | Complete | do_dock, do_undock, etc. in vessels_docking.c |
| DB table creation | Complete | Auto-creates 5 tables at startup |
| Reference data | Complete | 19 room templates auto-populated |
| Command registration | **Not Done** | Phase 2 commands not in interpreter.c |
| Runtime wiring | **Not Done** | generate_ship_interior() not called |
| Interior movement | **Not Done** | Movement helpers declared but not implemented |
| Persistence | **Not Done** | Save/load calls not wired into runtime |

**Implementation Notes:**
- Room generation in `vessels_rooms.c` uses 10 hard-coded templates; does not yet query `ship_room_templates` database table.
- Interior movement helpers declared in `vessels.h:671-674` (`do_move_ship_interior()`, `get_ship_exit()`, `is_passage_blocked()`) have no implementation.
- `do_look_outside()` prints placeholder; does not call `get_weather()` or render wilderness view.

### Phase 01: Automation Layer (Complete)

**Sessions**: 7 | **PRD**: [phase_01/PRD_phase_01.md](phase_01/PRD_phase_01.md) | **Completed**: 2025-12-30

Vessel automation capabilities:
- Session 01: Autopilot data structures and constants (Complete)
- Session 02: Waypoint and route management with DB persistence (Complete)
- Session 03: Path-following movement logic (Complete)
- Session 04: Autopilot player commands (Complete)
- Session 05: NPC pilot integration (Complete)
- Session 06: Scheduled route system (Complete)
- Session 07: Testing and validation (Complete)

**Key Deliverables:**
- 84 unit tests (100% pass rate, Valgrind clean)
- Memory usage: 1016 bytes/vessel (target: <1KB)
- Stress tested: 100/250/500 concurrent vessels

### Phase 02: Simple Vehicle Support (Complete)

**Sessions**: 7 | **PRD**: [phase_02/PRD_phase_02.md](phase_02/PRD_phase_02.md) | **Completed**: 2025-12-30

Lightweight vehicle tier and transport unification:
- Session 01: Vehicle data structures and constants (Complete)
- Session 02: Vehicle creation and initialization system (Complete)
- Session 03: Vehicle movement with terrain handling (Complete)
- Session 04: Vehicle player commands (mount, dismount, drive) (Complete)
- Session 05: Vehicle-in-vehicle mechanics (loading onto vessels) (Complete)
- Session 06: Unified command interface for all transport types (Complete)
- Session 07: Testing and validation (Complete)

**Key Deliverables:**
- 159 unit tests (100% pass rate, Valgrind clean)
- Memory usage: 148 bytes/vehicle (target: <512)
- Stress tested: 100/500/1000 concurrent vehicles
- Unified transport interface across vehicles and vessels

### Phase 03: Optimization & Polish (Complete)

**Sessions**: 6 | **PRD**: [phase_03/PRD_phase_03.md](phase_03/PRD_phase_03.md) | **Completed**: 2025-12-30

Code quality, feature completion, and production readiness:
- Session 01: Code Consolidation (Complete)
- Session 02: Interior Movement Implementation (Complete)
- Session 03: Command Registration & Wiring (Complete)
- Session 04: Dynamic Wilderness Rooms (Complete)
- Session 05: Vessel Type System (Complete)
- Session 06: Final Testing & Documentation (Complete)

**Key Deliverables:**
- 353 total unit tests (100% pass rate, Valgrind clean)
- Code consolidation and duplicate elimination
- Full interior movement implementation
- All Phase 2 commands registered
- Dynamic wilderness room allocation working
- Per-vessel type system complete
- Documentation complete

---

## 8. Remaining Work (Priority Order)

### Critical Path Items

1. **Dynamic wilderness room allocation** - Vessel movement must allocate rooms at destination coordinates using `find_available_wilderness_room()` + `assign_wilderness_room()` pattern
2. **Per-vessel type mapping** - Replace hardcoded `VESSEL_TYPE_SAILING_SHIP` placeholder with actual vessel_class lookup
3. **Register Phase 2 commands** - Add dock, undock, board_hostile, look_outside, ship_rooms to interpreter.c
4. **Wire interior generation** - Call `generate_ship_interior()` during ship creation/loading
5. **Implement interior movement** - Complete `do_move_ship_interior()`, `get_ship_exit()`, `is_passage_blocked()`

### Secondary Items

6. Connect persistence calls (save/load interiors, docking records, cargo, crew)
7. Integrate `look_outside` with `get_weather()` and wilderness view
8. Implement tactical display rendering
9. Implement contact detection and display
10. Complete disembark mechanics

### Testing & Validation

11. Create unit tests in unittests/CuTest
12. Performance profiling for room generation and docking
13. Stress testing with 100/250/500 concurrent vessels
14. Memory usage validation against targets

---

## 9. Success Metrics

### Technical Requirements

| Metric | Target | Status |
|--------|--------|--------|
| Max concurrent vessels | 500 | Validated (stress test passed) |
| Command response time | <100ms | Validated (<1ms achieved) |
| Memory per simple vessel | <1KB | Validated (~1016 bytes) |
| Memory (100 ships) | <5MB | Validated (99.2 KB) |
| Memory (500 ships) | <25MB | Validated (496.1 KB) |
| Code test coverage | >90% | 91 tests, 100% pass rate |
| Critical production bugs | 0 | Validated (Valgrind clean) |

### Functional Requirements

| Requirement | Status |
|-------------|--------|
| Greyhawk system compiles and loads | Complete |
| Players can board vessels | Complete |
| Basic movement across wilderness | Complete (Phase 00 Session 02) |
| Terrain-based speed modifiers | Complete |
| Weather affects movement | Complete |
| Multi-room ship interiors | Complete (Phase 00 Session 05-06) |
| Ship-to-ship docking | Complete (Phase 00 Session 04) |
| Tactical display | Complete (Phase 00 Session 08) |
| Contact detection | Complete (Phase 00 Session 08) |

---

## 10. Risk Assessment

| Risk | Probability | Impact | Mitigation |
|------|-------------|--------|------------|
| Integration complexity | High | High | Incremental development, extensive testing |
| Performance degradation | Medium | High | Tiered complexity, feature toggles |
| Data migration issues | Medium | Medium | Phased migration, rollback scripts ready |
| Scope creep | Medium | Medium | Strict change control, clear priorities |
| Memory overhead | Medium | High | Ship pooling, bit fields, compressed coordinates |
| Data corruption | Low | Critical | Bounds checking, null guards, transaction safety |

---

## 11. Known Issues

| Issue | Location | Description | Workaround |
|-------|----------|-------------|------------|
| Duplicate `disembark` registration | `interpreter.c:385` and `interpreter.c:1165` | Old and Greyhawk versions both registered; second (`do_greyhawk_disembark`) takes precedence | None needed; works correctly |
| Room templates hard-coded | `vessels_rooms.c:27-104` | 10 hard-coded templates used instead of querying `ship_room_templates` table | Future: wire DB lookup |
| Interior movement unimplemented | `vessels.h:671-674` | `do_move_ship_interior()`, `get_ship_exit()`, `is_passage_blocked()` declared but not implemented | Blocks interior navigation |

---

## 12. Troubleshooting

### Common Issues

**Issue: Foreign key constraint errors when inserting cargo/crew**
- **Cause:** `ship_interiors` record must exist first (parent table)
- **Solution:** Ensure vessel is saved to `ship_interiors` before adding cargo or crew records
- **Note:** CASCADE DELETE is implemented; deleting ship_interiors removes related records automatically

**Issue: Stored procedures not working**
- **Cause:** User lacks EXECUTE privilege
- **Solution:**
```sql
GRANT EXECUTE ON luminari_mudprod.* TO 'luminari_mud'@'localhost';
```

**Issue: Vessel movement fails silently**
- **Cause:** Destination wilderness coordinate has no allocated room
- **Solution:** Implement dynamic room allocation using `find_available_wilderness_room()` + `assign_wilderness_room()` pattern

**Issue: Phase 2 commands not recognized**
- **Cause:** Commands not registered in `interpreter.c`
- **Solution:** Add command entries for `dock`, `undock`, `board_hostile`, `look_outside`, `ship_rooms`

**Issue: Performance degradation with many vessels**
- **Diagnosis:** Check index usage:
```sql
EXPLAIN SELECT * FROM ship_docking WHERE ship1_id = '123';
```
- **Solution:** Verify indexes exist on frequently queried columns

---

## 13. Verification & Monitoring

### Post-Deployment Verification

Run these queries to verify successful deployment:

```sql
-- Check all 5 tables exist
SELECT COUNT(*) as table_count FROM information_schema.TABLES
WHERE TABLE_SCHEMA = DATABASE() AND TABLE_NAME LIKE 'ship_%';
-- Expected: 5

-- Check room templates loaded
SELECT COUNT(*) as template_count FROM ship_room_templates;
-- Expected: 19

-- Check stored procedures exist
SHOW PROCEDURE STATUS WHERE Db = DATABASE()
AND Name IN ('cleanup_orphaned_dockings', 'get_active_dockings');
-- Expected: 2 procedures

-- Verify table structure
DESCRIBE ship_interiors;
DESCRIBE ship_docking;
```

### Ongoing Monitoring

```sql
-- Check table sizes periodically
SELECT TABLE_NAME, TABLE_ROWS, DATA_LENGTH/1024/1024 as size_mb
FROM information_schema.TABLES
WHERE TABLE_SCHEMA = DATABASE() AND TABLE_NAME LIKE 'ship_%';

-- Monitor active dockings
SELECT COUNT(*) as active_docks FROM ship_docking WHERE dock_status = 'active';
```

### Scheduled Maintenance

Run weekly to clean orphaned docking records:
```sql
CALL cleanup_orphaned_dockings();
```

---

## 14. Dependencies

### External Dependencies

- MySQL/MariaDB 5.7+ (required)
- Wilderness system operational
- Zone 213 test area configured

### Internal Dependencies

- `src/wilderness.c` - Coordinate system and room allocation
- `src/weather.c` - Weather integration via `get_weather()`
- `src/spec_procs.c` - ITEM_GREYHAWK_SHIP special procedure
- `src/interpreter.c` - Command registration
- `src/db.c` - Boot sequence integration

### Reserved Resources

- **VNUM Range 30000-40019:** Reserved for dynamic ship interior rooms
- **Item Type 56:** ITEM_GREYHAWK_SHIP
- **Room Flags:** ROOM_VEHICLE (40), ROOM_DOCKABLE (41)

### Test Zone (Zone 213)

- Room 21300: Dock room with DOCKABLE flag
- Room 21398: Ship interior (control room)
- Room 21399: Additional ship interior
- Object 21300: Test ship (ITEM_GREYHAWK_SHIP, boarding functional)

---

## 15. File Inventory

### Core Implementation

- `src/vessels.h` - Structures, constants, prototypes
- `src/vessels.c` - Core commands, wilderness movement
- `src/vessels_rooms.c` - Interior room generation
- `src/vessels_docking.c` - Docking and boarding
- `src/vessels_db.c` - Database persistence

### Database

- `src/db_init.c` - Table creation (init_vessel_system_tables)
- `src/db_init_data.c` - Template population
- `sql/components/vessels_phase2_schema.sql` - Manual schema script
- `sql/components/vessels_phase2_rollback.sql` - Rollback script
- `sql/components/verify_vessels_schema.sql` - Verification script

### Legacy (Disabled)

- `src/vessels_src.c` - Old CWG/Outcast code (#if 0)
- `src/vessels_src.h` - Old headers (#if 0)

---

## 16. Technical Stack

- **Language:** ANSI C90/C89 with GNU extensions
- **Database:** MySQL/MariaDB (required)
- **Build System:** CMake (preferred) or Autotools
- **Testing:** CuTest unit test framework
- **Codebase:** tbaMUD/CircleMUD foundation

---

## 17. Deployment Notes

### Recommended Approach

Let the server auto-create vessel tables on first startup with MySQL available. This ensures schema matches runtime code exactly.

### Manual Deployment (Production)

1. Backup database: `mysqldump -u root -p luminari_mudprod > backup.sql`
2. Validate `sql/components/vessels_phase2_schema.sql` matches `init_vessel_system_tables()`
3. Execute schema script
4. Run verification script
5. Test basic vessel commands

### Rollback

Execute `sql/components/vessels_phase2_rollback.sql` to remove all vessel tables.

---

## 18. Approval & Sign-Off

| Role | Name | Date |
|------|------|------|
| Project Lead | | |
| Technical Lead | | |
| QA Lead | | |

---

*End of Technical PRD*
