# NEXT_SESSION.md

## Session Recommendation

**Generated**: 2025-12-30
**Project State**: Phase 02 - Simple Vehicle Support
**Completed Sessions**: 18

---

## Recommended Next Session

**Session ID**: `phase02-session03-vehicle-movement-system`
**Session Name**: Vehicle Movement System
**Estimated Duration**: 2-4 hours
**Estimated Tasks**: 15-20

---

## Why This Session Next?

### Prerequisites Met
- [x] Session 01 complete (vehicle data structures)
- [x] Session 02 complete (vehicle creation system)
- [x] Wilderness coordinate system operational
- [x] Dynamic room allocation patterns established in Phase 00

### Dependencies
- **Builds on**: phase02-session02-vehicle-creation-system (vehicle lifecycle and persistence)
- **Enables**: phase02-session04-vehicle-player-commands (drive command needs movement)

### Project Progression
Session 03 is the natural next step in Phase 02's progression. Sessions 01-02 established the vehicle data structures and creation/persistence systems. Now vehicles need the ability to actually move across the wilderness grid. This unlocks Session 04's player commands which rely on movement functionality for the `drive` command.

---

## Session Overview

### Objective
Implement vehicle movement across the wilderness coordinate system with proper terrain restrictions and speed modifiers for land-based vehicles.

### Key Deliverables
1. `move_vehicle()` core movement function
2. `check_vehicle_terrain()` terrain traversability validation
3. `get_vehicle_speed_modifier()` terrain-based speed calculation
4. Vehicle position update logic with database persistence
5. Integration with wilderness coordinate system
6. Dynamic room allocation for vehicle destinations

### Scope Summary
- **In Scope (MVP)**: Vehicle movement, terrain checking, speed modifiers, 8-direction navigation, wilderness room allocation, position tracking
- **Out of Scope**: Autopilot/waypoints, combat during movement, weather effects, mount stamina/fatigue

---

## Technical Considerations

### Technologies/Patterns
- Wilderness coordinate system (-1024 to +1024 X/Y)
- `find_available_wilderness_room()` + `assign_wilderness_room()` pattern (from Phase 00)
- Terrain speed modifier tables (established for vessels)
- ANSI C90/C89 with GNU extensions

### Potential Challenges
- Ensuring terrain traversability differs correctly from vessels (land vs water)
- Implementing road bonus/rough terrain penalty appropriately
- Memory efficiency for movement state tracking
- Handling invalid movement attempts gracefully without crashes

### Relevant Considerations
- [P00] **Dynamic wilderness room allocation**: Vessel movement originally failed silently when destination had no allocated room. Fixed in Phase 00 Session 02 using find_available_wilderness_room() pattern - apply same approach to vehicles.
- [P01] **Memory-efficient structs**: Autopilot achieved 48 bytes per vessel; maintain efficiency for vehicle movement state.
- [P01] **Standalone unit tests**: Create self-contained tests without server dependencies.

---

## Alternative Sessions

If this session is blocked:
1. **phase02-session06-unified-command-interface** - Could theoretically start API design, but lacks movement implementation to test against
2. **phase02-session07-testing-validation** - Requires sessions 01-06 complete

Note: Sessions 04 and 05 both depend on Session 03's movement system, so no viable alternatives exist.

---

## Next Steps

Run `/sessionspec` to generate the formal specification.
