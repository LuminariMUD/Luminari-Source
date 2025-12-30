# LuminariMUD Vessel System

Last Updated: 2025-12-30
Version: 2.0 (Phase 01 Complete - Automation Layer)

---

## Overview

The Vessel System provides a unified implementation for ships, airships, submarines, and other vehicles in LuminariMUD. It enables free-roaming navigation across the 2048x2048 wilderness coordinate system with terrain-aware movement, elevation support, and multi-room ship interiors.

## Architecture

### Source Files

| File | Purpose |
|------|---------|
| `src/vessels.h` | Structures, constants, prototypes |
| `src/vessels.c` | Core commands, wilderness movement, terrain system |
| `src/vessels_rooms.c` | Interior room generation and movement |
| `src/vessels_docking.c` | Docking, boarding, and ship-to-ship interaction |
| `src/vessels_db.c` | MySQL persistence layer |
| `src/vessels_autopilot.c` | Autopilot, waypoints, routes, NPC pilots, schedules |

### Dependencies

- **Wilderness System** (`wilderness.c/h`) - Coordinate system and room allocation
- **Weather System** (`weather.c`) - Weather effects on movement
- **DG Scripts** (`dg_scripts.c/h`) - Trigger integration for interior movement
- **MySQL** (`mysql.c`) - Persistence layer (required)

### System Diagram

```
UNIFIED VESSEL SYSTEM
    |
    +-- Wilderness Coordinate System (X, Y, Z navigation)
    |       Range: -1024 to +1024 on X/Y, -500 to +500 on Z
    |
    +-- Vessel Type System (8 vessel classes)
    |       RAFT, BOAT, SHIP, WARSHIP, AIRSHIP, SUBMARINE, TRANSPORT, MAGICAL
    |
    +-- Multi-Room Interiors (dynamic room generation)
    |       VNUM Range: 70000-79999
    |
    +-- Terrain Integration (40 sector types, speed modifiers)
    |
    +-- Automation Layer (Phase 01)
    |       +-- Autopilot system with waypoint navigation
    |       +-- Named route management
    |       +-- NPC pilot integration
    |       +-- Scheduled route execution
    |
    +-- Database Persistence (5 MySQL tables)
```

---

## Vessel Types

### Vessel Classes

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

### Terrain Speed Modifiers

| Terrain | Surface Vessels | Airships | Submarines |
|---------|----------------|----------|------------|
| Ocean/Deep Water | 100% | 100% | 100% |
| Shallow Water | 75% | 100% | 0% (blocked) |
| Rivers | 50-100% (by type) | 100% | 0% (blocked) |
| Land/Mountains | 0% (blocked) | 75-100% | 0% (blocked) |
| Storm conditions | -25% | -50% | 0% |

---

## Commands

### Navigation Commands

| Command | Description | Usage |
|---------|-------------|-------|
| `board` | Enter a vessel | `board <ship>` |
| `disembark` | Exit a vessel | `disembark` |
| `shipstatus` | Display current status | `shipstatus` |
| `speed` | Set vessel speed | `speed <0-10>` |
| `heading` | Set direction | `heading <dir>` |
| `tactical` | Display tactical map | `tactical` |
| `contacts` | Show nearby vessels | `contacts` |

### Ship Interior Commands

| Command | Description | Usage |
|---------|-------------|-------|
| `ship_rooms` | List interior rooms | `ship_rooms` |
| `look_outside` | View exterior | `look_outside` |

### Docking Commands

| Command | Description | Usage |
|---------|-------------|-------|
| `dock` | Create gangway to vessel | `dock [ship]` |
| `undock` | Remove docking connection | `undock` |
| `board_hostile` | Forced boarding attempt | `board_hostile <ship>` |

### Autopilot Commands (Phase 01)

| Command | Description | Usage |
|---------|-------------|-------|
| `autopilot` | Toggle autopilot on/off | `autopilot [on/off]` |
| `autopilot status` | Display autopilot state | `autopilot status` |
| `waypoint add` | Add waypoint to route | `waypoint add <x> <y> [z]` |
| `waypoint remove` | Remove waypoint | `waypoint remove <index>` |
| `waypoint list` | List all waypoints | `waypoint list` |
| `waypoint clear` | Clear all waypoints | `waypoint clear` |
| `route create` | Create named route | `route create <name>` |
| `route load` | Load saved route | `route load <name>` |
| `route save` | Save current route | `route save <name>` |
| `route list` | List saved routes | `route list` |
| `schedule` | Set scheduled departure | `schedule <time>` |
| `schedule clear` | Clear schedule | `schedule clear` |

### NPC Pilot Commands (Phase 01)

| Command | Description | Usage |
|---------|-------------|-------|
| `pilot assign` | Assign NPC as pilot | `pilot assign <npc>` |
| `pilot release` | Release current pilot | `pilot release` |
| `pilot status` | Show pilot information | `pilot status` |

---

## Automation Layer (Phase 01)

### Autopilot System

The autopilot system enables hands-free navigation along predefined waypoint routes.

**States:**
- `AUTOPILOT_INACTIVE` - System off
- `AUTOPILOT_ACTIVE` - Following route
- `AUTOPILOT_PAUSED` - Temporarily stopped
- `AUTOPILOT_COMPLETE` - Route finished

**Features:**
- Waypoint-based path following
- Automatic terrain avoidance
- Speed adjustment for conditions
- Pause/resume capability

### Route Management

Routes are named collections of waypoints that can be saved and reused.

**Structure:**
- Up to 20 waypoints per route
- X, Y, Z coordinates per waypoint
- Optional waypoint names
- Database persistence

### NPC Pilot Integration

NPC characters can be assigned as vessel pilots for automated operation.

**Capabilities:**
- Autonomous navigation
- Route following
- Alert generation for obstacles
- Crew management integration

### Scheduled Routes

Vessels can be scheduled to depart at specific times.

**Features:**
- Time-based departure scheduling
- Automatic route activation
- Repeat schedule support
- Integration with NPC pilots

---

## Interior Rooms

### Room Generation

Ships automatically generate interior rooms based on vessel type when loaded:

| Vessel Type | Room Count | Rooms Generated |
|-------------|------------|-----------------|
| VESSEL_RAFT | 1-2 | Bridge only |
| VESSEL_BOAT | 2-4 | Bridge, Quarters |
| VESSEL_SHIP | 3-8 | Bridge, Quarters, Cargo, Deck |
| VESSEL_WARSHIP | 5-15 | Bridge, Armory, Weapons, Quarters, Brig |
| VESSEL_AIRSHIP | 4-12 | Bridge, Observation, Engineering, Quarters |
| VESSEL_SUBMARINE | 4-10 | Bridge, Helm, Engineering, Quarters |
| VESSEL_TRANSPORT | 6-20 | Bridge, Large Cargo, Passenger Quarters |
| VESSEL_MAGICAL | Variable | Custom configuration |

### Room Templates (19 types)

- **Control**: bridge, helm
- **Quarters**: quarters_captain, quarters_crew, quarters_officer
- **Cargo**: cargo_main, cargo_secure
- **Engineering**: engineering, weapons, armory
- **Common**: mess_hall, galley, infirmary
- **Connectivity**: corridor, deck_main, deck_lower
- **Special**: airlock, observation, brig

### VNUM Allocation

```
Interior Room VNUM = 70000 + (ship_number * 20) + room_index

Range: 70000 - 79999 (reserved for vessel interiors)
Maximum: 500 vessels * 20 rooms = 10,000 rooms
```

---

## Database Schema

### Tables (Auto-created at startup)

| Table | Purpose |
|-------|---------|
| `ship_interiors` | Vessel configuration and room data |
| `ship_docking` | Active and historical docking records |
| `ship_room_templates` | 19 pre-configured room templates |
| `ship_cargo_manifest` | Cargo tracking |
| `ship_crew_roster` | NPC crew assignments |

### Persistence Lifecycle

1. **Boot**: Ships initialized, then saved states loaded from database
2. **Create**: New ship interiors immediately saved to database
3. **Dock**: Docking records saved when ships dock
4. **Undock**: Docking records marked complete when ships undock
5. **Shutdown**: All vessel states saved before server terminates

---

## Key Constants

| Constant | Value | Description |
|----------|-------|-------------|
| `GREYHAWK_MAXSHIPS` | 500 | Maximum concurrent vessels |
| `MAX_SHIP_ROOMS` | 20 | Maximum interior rooms per vessel |
| `SHIP_INTERIOR_VNUM_BASE` | 70000 | Start of interior room VNUMs |
| `SHIP_INTERIOR_VNUM_MAX` | 79999 | End of interior room VNUMs |
| `MAX_DOCKING_RANGE` | 2.0 | Maximum distance for docking |
| `BOARDING_DIFFICULTY` | 15 | DC for hostile boarding attempts |

---

## Performance Metrics

### Phase 00 (Core System)

| Metric | Target | Achieved |
|--------|--------|----------|
| Max concurrent vessels | 500 | 500 (passed) |
| Memory per vessel | <1KB | 1016 bytes |
| Command response time | <100ms | <1ms |
| Unit test coverage | >90% | 91 tests, 100% pass |
| Memory leaks | 0 | 0 (Valgrind clean) |

### Phase 01 (Automation Layer)

| Metric | Target | Achieved |
|--------|--------|----------|
| Unit tests | 84 | 84/84 pass |
| Memory per vessel (with autopilot) | <1KB | 1016 bytes |
| Stress test (100 vessels) | Stable | Pass |
| Stress test (250 vessels) | Stable | Pass |
| Stress test (500 vessels) | Stable | Pass |
| Memory leaks | 0 | 0 (Valgrind clean) |

### Autopilot Structure Sizes

| Structure | Size |
|-----------|------|
| `struct waypoint` | 88 bytes |
| `struct ship_route` | 1840 bytes |
| `struct autopilot_data` | 48 bytes |
| `struct waypoint_node` | 104 bytes |

---

## Development

### Adding New Vessel Types

1. Add enum value to `vessel_class` in `vessels.h`
2. Add terrain capabilities to `vessel_terrain_data[]` in `vessels.c`
3. Add room generation rules to `get_rooms_for_vessel_type()` in `vessels_rooms.c`
4. Update help files

### Adding New Commands

1. Implement handler in `vessels.c` or `vessels_docking.c`
2. Register in `interpreter.c` under vessel command section
3. Add help entry in `lib/text/help/help.hlp`
4. Add tests in `unittests/CuTest/test_vessels.c`

### Testing

```bash
# Build and run vessel tests
cd unittests/CuTest
make all
make test

# Run stress tests
make stress

# Run with Valgrind
make valgrind
```

---

## Troubleshooting

### Common Issues

**Issue: Vessel movement fails silently**
- **Cause**: Destination coordinate has no allocated room
- **Solution**: Dynamic allocation using `get_or_allocate_wilderness_room()` handles this automatically

**Issue: Interior rooms not generated**
- **Cause**: `generate_ship_interior()` not called or ship already has rooms
- **Solution**: Check idempotent flag; rooms generate once per ship

**Issue: Docking fails**
- **Cause**: Ships too far apart or terrain mismatch
- **Solution**: Move within `MAX_DOCKING_RANGE` (2.0 units)

### Verification Queries

```sql
-- Check vessel tables exist
SELECT COUNT(*) FROM information_schema.TABLES
WHERE TABLE_SCHEMA = DATABASE() AND TABLE_NAME LIKE 'ship_%';
-- Expected: 5

-- Check room templates loaded
SELECT COUNT(*) FROM ship_room_templates;
-- Expected: 19

-- Monitor active dockings
SELECT COUNT(*) FROM ship_docking WHERE dock_status = 'active';
```

---

## Related Documentation

- [PRD](../../.spec_system/PRD/PRD.md) - Product requirements and roadmap
- [Wilderness System](WILDERNESS_SYSTEM.md) - Coordinate system details
- [Phase 00 Test Results](../testing/vessel_test_results.md) - Core system validation
- [Phase 01 Test Results](../testing/phase01_test_results.md) - Automation layer validation

---

*Phase 00 "Core Vessel System" and Phase 01 "Automation Layer" completed 2025-12-30. See `.spec_system/` for implementation details.*
