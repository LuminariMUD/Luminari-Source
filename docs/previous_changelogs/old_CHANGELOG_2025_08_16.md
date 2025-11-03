# Changelog

## January 2025 - Vessel System Phase 2 In Progress

### Phase 2: Multi-Room Vessel Interiors (IN PROGRESS)

#### New Features Implemented
- **Multi-Room Ship Interiors**
  - Ships now have 1-20 interior rooms based on vessel type
  - Dynamic room generation with discovery algorithm (30% chance for extra rooms)
  - 10 room types: Bridge, Quarters, Cargo, Engineering, Weapons, Medical, Mess Hall, Corridor, Airlock, Deck
  - Smart room connections: hub-and-spoke pattern with cross-connections
  - Room descriptions dynamically include ship name

- **Vessel-Specific Room Allocation**
  - Rafts: 1-2 rooms (minimal)
  - Boats: 2-4 rooms (basic quarters)
  - Ships: 3-8 rooms (full facilities)
  - Warships: 5-15 rooms (weapons bays, engineering)
  - Transports: 6-20 rooms (multiple cargo holds)
  - Airships: 4-10 rooms (deck access)
  - Submarines: 4-12 rooms (airlocks)

- **Ship-to-Ship Docking System**
  - Range-based docking (max 2.0 units distance)
  - Speed restrictions for safe docking (max speed 2)
  - Dynamic gangway creation between vessels
  - Bidirectional connections for crew transfer
  - Undocking with automatic vessel separation

- **Combat Boarding Mechanics**
  - Hostile boarding with skill checks
  - Difficulty based on vessel type and damage
  - Failure consequences (fall damage, water hazard)
  - Automatic combat initiation on successful boarding
  - Defensive measures (sealed hatches, alerts)

- **New Commands**
  - `dock [ship]` - Dock with nearby vessel
  - `undock` - Separate from docked vessel
  - `board_hostile <ship>` - Attempt combat boarding
  - `look_outside` - View surroundings from ship interior
  - `ship_rooms` - List vessel's interior layout

#### Technical Implementation
- **New Files Created:**
  - `src/vessels_rooms.c` - Room generation and management (600+ lines)
  - `src/vessels_docking.c` - Docking and boarding mechanics (700+ lines)
  
- **Modified Files:**
  - `src/vessels.h` - Extended data structures for multi-room support
  - Added room types, connections, docking fields
  - New function prototypes for Phase 2 features

- **Data Structure Enhancements:**
  - MAX_SHIP_ROOMS (20) and MAX_SHIP_CONNECTIONS (40)
  - Room connection structure with hatch/lock support
  - Docking state tracking and ship-to-ship links
  - Room template system for dynamic generation

#### Remaining Phase 2 Tasks
- [ ] Interior movement integration with main navigation
- [ ] Database persistence for ship configurations
- [ ] NPC crew placement and management
- [ ] Full cargo transfer system
- [ ] Performance optimization
- [ ] Unit test implementation

---

## January 2025 - Vessel System Phase 1 Complete

### Major Features
- **Wilderness Coordinate Integration for Vessels**
  - Ships now navigate using wilderness X/Y coordinates (-1024 to +1024)
  - Free movement across entire 2048x2048 wilderness grid
  - No artificial track restrictions - vessels move freely
  - Integration with `find_room_by_coordinates()` for dynamic room management

- **Terrain-Based Movement System**  
  - Different vessel types have appropriate terrain restrictions
  - Sailing ships: Can only traverse water sectors
  - Airships: Can fly over any terrain at altitude > 100
  - Submarines: Can navigate underwater sectors
  - Dynamic speed modifiers based on terrain type

- **Vessel Classifications (8 Types)**
  - VESSEL_RAFT - Small river craft
  - VESSEL_BOAT - Coastal vessels  
  - VESSEL_SHIP - Ocean-capable ships
  - VESSEL_WARSHIP - Combat vessels
  - VESSEL_AIRSHIP - Flying vessels
  - VESSEL_SUBMARINE - Underwater vessels
  - VESSEL_TRANSPORT - Cargo ships
  - VESSEL_MAGICAL - Special vessels

- **3D Movement Support**
  - Z-coordinate tracking for elevation/depth
  - UP/DOWN commands for airships and submarines
  - Altitude restrictions for terrain traversal

- **Weather Integration**
  - Weather conditions affect movement speed and distance
  - Storm conditions reduce movement by 25%
  - Dynamic weather messages for immersion
  - Airships more affected by weather than surface vessels

### Enhanced Commands
- **status** - Shows vessel position, coordinates, terrain, weather
- **speed <0-max>** - Sets vessel speed with terrain/weather feedback  
- **heading <direction>** - Moves vessel in 8 cardinal/intercardinal directions + up/down

### Technical Implementation
- Added 326 lines of wilderness integration code to vessels.c
- New data structures in vessels.h for terrain capabilities
- Functions implemented:
  - `update_ship_wilderness_position()` - Core position update
  - `get_ship_terrain_type()` - Terrain detection
  - `can_vessel_traverse_terrain()` - Movement validation
  - `get_terrain_speed_modifier()` - Speed calculation
  - `move_ship_wilderness()` - Complete movement handler

### Files Modified
- src/vessels.c - Wilderness integration functions
- src/vessels.h - Vessel classifications and capability structures
- Both files compile without warnings

### Testing Status
- ✅ Compilation successful without errors
- ✅ Movement in all 8 directions + up/down
- ✅ Speed control with modifiers  
- ✅ Terrain restrictions enforced
- ✅ Weather effects operational
- ✅ Status display functional

### Next Phase Preview
- Multi-room ship interiors (Outcast integration)
- Ship-to-ship combat with spatial positioning
- NPC automation and pathfinding
- Resource gathering from vessels
- Advanced collision detection

