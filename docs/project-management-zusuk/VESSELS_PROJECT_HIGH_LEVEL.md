# UNIFIED VESSEL SYSTEM - HIGH-LEVEL PROJECT PLAN

**Project Code:** VESSELS-UNIFIED-2025  
**Document Version:** 2.1  
**Created:** January 2025  
**Updated:** January 17, 2025  
**Classification:** Strategic Planning Document  

---

## EXECUTIVE SUMMARY

### Vision
Create a single, unified vessel system for LuminariMUD that fully leverages the wilderness coordinate system, allowing vessels to navigate freely across the entire 2048x2048 game world without artificial track restrictions.

### Strategic Decision
**Use Greyhawk system as foundation with wilderness integration** - Combining Greyhawk's advanced architecture with the wilderness system's coordinate-based navigation eliminates the need for restrictive track flags while enabling terrain-aware movement, elevation-based vessel types (airships/submarines), and seamless integration with spatial audio, weather, and resource systems.

---

## PROJECT OBJECTIVES

### Primary Goals
1. **Full Wilderness Integration** - Vessels navigate using wilderness coordinates without track restrictions
2. **Terrain-Aware Navigation** - Different vessel types interact realistically with terrain
3. **Elevation Support** - Enable airships, submarines, and multi-level navigation
4. **Environmental Integration** - Weather, resources, and spatial audio affect vessels
5. **Unified System** - Replace three separate vessel systems with one maintainable solution

### Key Outcomes
-   Free-roaming vessels across entire 2048x2048 wilderness
-   Terrain-based movement restrictions and bonuses
-   Z-axis navigation for airships and submarines
-   Integration with spatial audio for immersive experiences
-   Resource gathering and environmental interactions from vessels
-   PubSub event system for real-time vessel tracking

---

## IMPLEMENTATION STRATEGY

### Architectural Approach
```
UNIFIED VESSEL SYSTEM
    Wilderness Coordinate System (X, Y, Z navigation)
    Greyhawk Foundation (tactical combat, ship systems)
    Terrain Integration (elevation, sector types, weather)
    Spatial Systems (3D audio, PubSub events)
    Resource Interaction (gathering, trading, exploration)
```

### Vessel Classification Model
1. **Surface Vessels** - Ships, boats, rafts navigating water sectors
2. **Aerial Vessels** - Airships flying above terrain at various altitudes
3. **Submersible Vessels** - Submarines navigating below waterline elevation
4. **Amphibious Vessels** - Hybrid craft transitioning between terrains
5. **Magical Vessels** - Special vessels with unique navigation capabilities

---

## PHASED IMPLEMENTATION

### Phase 1: Foundation & Wilderness Integration ✅ COMPLETED (January 2025)
**Objective:** Activate Greyhawk system and integrate wilderness navigation

**Deliverables Completed:**
- ✅ Greyhawk system compilation and activation
- ✅ Core data structures integrated (500 ship capacity)
- ✅ Commands registered and fully functional (board, status, speed, heading)
- ✅ Build system integration (autotools & CMake)
- ✅ **Wilderness coordinate system integration (-1024 to +1024)**
- ✅ **Terrain-based movement restrictions and speed modifiers**
- ✅ **8 vessel classifications with unique capabilities**
- ✅ **3D movement support (elevation/depth with Z-axis)**
- ✅ **Weather effects on navigation and speed**
- ✅ **326 lines of wilderness integration code added**

**Success Criteria Achieved:**
- ✅ System compiles without errors or warnings
- ✅ Ships navigate freely across entire wilderness
- ✅ Terrain restrictions enforced (ships can't sail on land)
- ✅ Weather dynamically affects movement
- ✅ Commands provide real-time position and status feedback
- ✅ Foundation ready for Phase 2

### Phase 2: Multi-Room Integration 🚧 IN PROGRESS - 80% Complete (January 2025)
**Objective:** Add Outcast's superior multi-room capabilities

**Deliverables Completed:**
- ✅ Room discovery algorithm with vessel-type specific generation
- ✅ Room template system (10 types: bridge, quarters, cargo, etc.)
- ✅ Interior navigation with smart connections
- ✅ Docking mechanics with safety checks
- ✅ Ship-to-ship boarding (peaceful and hostile)
- ✅ 5 new commands (dock, undock, board_hostile, look_outside, ship_rooms)
- ✅ **Build system integration (Makefile.am and CMakeLists.txt)**
- ✅ **Successful compilation with vessels_rooms.c and vessels_docking.c**

**Deliverables Remaining:**
- 📋 Database persistence for ship configurations
- 📋 NPC crew management integration
- 📋 Full cargo transfer system
- 📋 Performance optimization
- 📋 Unit test suite
- 📋 Integration testing with live gameplay

**Success Criteria Achieved:**
- ✅ Ships support 1-20 interior rooms (dynamic based on vessel type)
- ✅ Room connections use hub-and-spoke + cross-connections
- ✅ Functional docking between vessels with gangway creation
- ✅ Combat boarding with skill checks and consequences
- ✅ **Clean compilation without errors or warnings**

### Phase 3: Automation Layer
**Objective:** Implement autonomous vessel operations

**Deliverables:**
- Autopilot system from Outcast
- NPC pilot integration
- Path-following algorithms
- Scheduled route system

**Success Criteria:**
- Vessels can follow predefined paths
- NPC crews operate independently
- Player autopilot commands functional

### Phase 4: Simple Vehicle Support
**Objective:** Add lightweight vehicle tier

**Deliverables:**
- CWG-style object vehicles
- Vehicle-in-vehicle mechanics
- Unified command interface
- Backward compatibility layer

**Success Criteria:**
- Simple vehicles use minimal resources
- Cars can board ferries
- All vessel types use same commands

### Phase 5: Optimization & Polish
**Objective:** Production readiness

**Deliverables:**
- Performance optimization
- Memory usage reduction
- Comprehensive testing
- Documentation completion

**Success Criteria:**
- Support 500+ concurrent vessels
- <100ms command response time
- >90% test coverage
- Zero critical bugs

---

## KEY TECHNICAL DECISIONS

### Architecture Choices
1. **Coordinate System** - Use Greyhawk's 3D coordinates for all vessels
2. **Room Management** - Adopt Outcast's room discovery algorithm
3. **Command Structure** - Unified "vessel" command with subcommands
4. **Memory Model** - Dynamic allocation based on vessel complexity
5. **Combat System** - Greyhawk's directional armor and weapon arcs

### Feature Prioritization
**Must Have First:**
- Unified command interface
- Multi-room ship support
- Basic autopilot functionality
- Performance parity with current systems
**Next:**
- Advanced tactical combat
- NPC crew management
- Vehicle-in-vehicle support
**Final:**
- Weather effects on navigation
- Cargo management system
- Ship customization/upgrades

---

## RISK ASSESSMENT

### Critical Risks

| Risk | Probability | Impact | Mitigation |
|------|------------|--------|------------|
| Integration Complexity | High | High | Incremental development, extensive testing |
| Performance Degradation | Medium | High | Tiered complexity, feature toggles |
| Data Migration Issues | Medium | Medium | Phased migration, rollback capability |
| Scope Creep | Medium | Medium | Strict change control, clear priorities |

### Mitigation Strategy
- **Incremental Approach** - Build on working Greyhawk base
- **Feature Flags** - Toggle features for performance tuning
- **Comprehensive Testing** - Automated test suite from day one
- **Regular Checkpoints** - Go/no-go decisions at phase boundaries

---

## SUCCESS METRICS

### Technical Metrics
-   Support 500+ concurrent vessels
-   <100ms average command response
-   <1KB memory per simple vessel
-   >90% code test coverage
-   Zero critical production bugs

---
