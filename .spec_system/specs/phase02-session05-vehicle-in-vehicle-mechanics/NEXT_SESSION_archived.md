# NEXT_SESSION.md

## Session Recommendation

**Generated**: 2025-12-30
**Project State**: Phase 02 - Simple Vehicle Support
**Completed Sessions**: 20

---

## Recommended Next Session

**Session ID**: `phase02-session05-vehicle-in-vehicle-mechanics`
**Session Name**: Vehicle-in-Vehicle Mechanics
**Estimated Duration**: 2-4 hours
**Estimated Tasks**: 15-20

---

## Why This Session Next?

### Prerequisites Met
- [x] Session 04 complete (player commands - mount, dismount, drive, vehiclestatus)
- [x] Vehicle data structures implemented (session 01)
- [x] Vehicle creation/initialization system (session 02)
- [x] Vehicle movement with terrain handling (session 03)
- [x] Vessel docking system functional (Phase 00)

### Dependencies
- **Builds on**: phase02-session04-vehicle-player-commands
- **Enables**: phase02-session06-unified-command-interface

### Project Progression
This session is the natural next step because:
1. All foundational vehicle systems are now complete (data structures, creation, movement, commands)
2. Vehicle-in-vehicle mechanics are a prerequisite for the unified command interface
3. This enables the key use case of loading land vehicles onto water vessels (wagons on ferries, carriages on transport ships)
4. Follows the PRD's logical build order for Phase 02

---

## Session Overview

### Objective
Implement mechanics for loading smaller vehicles onto larger transport vessels, enabling scenarios like cars on ferries, wagons on transport ships, and mounts in cargo holds.

### Key Deliverables
1. `load_vehicle_onto_vessel()` - Core loading function
2. `unload_vehicle_from_vessel()` - Core unloading function
3. `check_vessel_vehicle_capacity()` - Weight/capacity validation
4. Loaded vehicle tracking structure
5. `do_loadvehicle()` command handler
6. `do_unloadvehicle()` command handler
7. Database persistence for loaded state

### Scope Summary
- **In Scope (MVP)**: Vehicle loading/unloading, capacity checking, nested transport tracking, player commands (`loadvehicle`, `unloadvehicle`), state persistence
- **Out of Scope**: Vehicle-on-vehicle loading, multi-level nesting, automated loading, loading while moving

---

## Technical Considerations

### Technologies/Patterns
- Follow vessels_docking.c boarding patterns for loading mechanics
- Reuse existing capacity/weight structures from vessel system
- MySQL persistence following ship_cargo_manifest pattern
- ANSI C90 compliance (no C99 features)

### Potential Challenges
- Ensuring loaded vehicles move correctly with parent vessel coordinates
- Managing state when vessel moves to different wilderness coordinates
- Handling edge cases (loading onto moving vessel, unloading in invalid terrain)
- Memory management for nested transport relationships

### Relevant Considerations
- **[P01] Standalone unit test files**: Create self-contained tests without server dependencies
- **[P01] Memory-efficient structures**: Target <512 bytes per vehicle (existing constraint)
- **[P00] Auto-create DB tables at startup**: Schema matches runtime code exactly
- **Avoid C99/C11 features**: Use ANSI C90/C89 only
- **Don't skip NULL checks**: Critical for pointer handling in nested structures

---

## Alternative Sessions

If this session is blocked:
1. **phase02-session06-unified-command-interface** - Could start partial abstraction layer, but vehicle-in-vehicle should come first for complete integration
2. **phase02-session07-testing-validation** - Could write tests for existing sessions 01-04, but better to complete all features first

---

## Next Steps

Run `/sessionspec` to generate the formal specification.
