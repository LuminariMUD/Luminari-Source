# Changelog

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

