# Changelog

## [Unreleased] - January 18, 2025

### Vessel System - Phase 2 Progress (85% Complete)

#### Added (January 18, 2025)
- **Database Persistence Layer** - Complete MySQL integration for vessel system
  - `vessels_db.c` - New file with full persistence implementation (450+ lines)
  - `vessels_phase2_schema.sql` - Database schema with 5 tables
  - Save/load ship interior configurations
  - Docking record persistence
  - Cargo manifest and crew roster tables
  - Stored procedures for cleanup operations
- **Interior Movement System** - Complete navigation within ship interiors
  - `do_move_ship_interior()` function for room-to-room movement
  - Passage blocking for sealed hatches
  - Ship exit navigation with connection tracking
  - Room coordinate synchronization with ship position
  
#### Added (January 17, 2025)
- **Multi-room vessel interiors** - Ships now support 1-20 dynamically generated interior rooms
  - `vessels_rooms.c` - New file implementing room generation, templates, and connections (660+ lines)
  - `vessels_docking.c` - New file implementing docking and boarding mechanics (500+ lines)
- **Room template system** - 10 different room types with dynamic descriptions:
  - Bridge, Quarters, Cargo Hold, Engineering, Weapons, Medical, Mess Hall, Corridor, Airlock
- **Vessel-specific room generation** - Different vessel types get appropriate room layouts:
  - Warships: Multiple weapon rooms, engineering
  - Transports: Multiple cargo holds
  - Smaller vessels: Fewer, more compact layouts
- **Ship-to-ship docking mechanics**:
  - `dock` command - Dock with nearby vessels
  - `undock` command - Separate from docked vessels
  - Automatic gangway creation between docked ships
  - Safety checks for proximity and speed
- **Combat boarding system**:
  - `board_hostile` command - Attempt hostile boarding
  - Skill-based success checks
  - Consequences for failure (damage, falling into water)
- **Interior viewing commands**:
  - `look_outside` - View wilderness from ship interior
  - `ship_rooms` - List all rooms in current vessel
- **Room connection algorithm** - Smart hub-and-spoke layout with cross-connections

#### Changed (January 18, 2025)
- **Build system integration**:
  - Updated Makefile.am to include vessels_rooms.c, vessels_docking.c, and vessels_db.c
  - Updated CMakeLists.txt with all new source files
  - Added database initialization to greyhawk_initialize_ships()
- **Function enhancements**:
  - complete_docking() now saves docking records to database
  - do_undock() updates database records on undocking
  - Added external function declarations for look_at_room and dirs[]
- **C89/C90 compatibility fixes**:
  - Changed all `number()` calls to `rand_number()`
  - Fixed room coordinate fields to use `coords[0]` and `coords[1]`
  - Corrected `damage()` function calls with proper parameters
  - Fixed room_flags array handling with SET_BIT_AR
  - Moved all variable declarations to block start (no C99 loop declarations)

#### Fixed
- Compilation errors related to undefined functions
- Type mismatches for room coordinate fields
- Incorrect damage() function parameters
- C99 loop variable declarations
- Room flags assignment for array-based flags

#### Technical Details
- **Files modified**: vessels.h, vessels.c, Makefile.am, CMakeLists.txt
- **New files created**: vessels_db.c, vessels_phase2_schema.sql
- **New dependencies**: mysql.h integration
- **New functions implemented (January 18)**:
  - `init_vessel_db()` - Initialize database persistence
  - `save_ship_interior()` / `load_ship_interior()` - Ship configuration persistence
  - `save_docking_record()` / `end_docking_record()` - Docking persistence
  - `save_cargo_manifest()` / `load_cargo_manifest()` - Cargo tracking
  - `save_crew_roster()` / `load_crew_roster()` - Crew management
  - `serialize_room_data()` / `deserialize_room_data()` - Room data encoding
  - `do_move_ship_interior()` - Interior navigation
  - `is_passage_blocked()` / `get_ship_exit()` - Movement validation
  - `update_ship_room_coordinates()` - Position synchronization
- **Previous functions implemented (January 17)**:
  - `generate_ship_interior()` - Creates room layout based on vessel type
  - `complete_docking()` - Establishes connections between ships
  - `do_dock()`, `do_undock()` - Docking commands
  - `do_board_hostile()` - Combat boarding command
  - `do_look_outside()` - View external wilderness

#### Remaining Work (15% to completion)
- NPC crew management and AI behavior
- Cargo transfer system completion with weight tracking
- Performance optimization and profiling
- Unit test suite creation (test_vessels_phase2.c)
- Integration testing with live gameplay
- Player and builder documentation

