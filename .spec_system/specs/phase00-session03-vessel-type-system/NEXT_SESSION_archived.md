# NEXT_SESSION.md

## Session Recommendation

**Generated**: 2025-12-29
**Project State**: Phase 00 - Core Vessel System
**Completed Sessions**: 2

---

## Recommended Next Session

**Session ID**: `phase00-session03-vessel-type-system`
**Session Name**: Vessel Type System
**Estimated Duration**: 2-3 hours
**Estimated Tasks**: 15-18

---

## Why This Session Next?

### Prerequisites Met
- [x] Session 01 completed (header cleanup foundation)
- [x] Session 02 completed (dynamic wilderness room allocation)
- [x] Understanding of vessel_class enum (defined in vessels.h)

### Dependencies
- **Builds on**: Session 02 (dynamic room allocation) - movement now works, but uses hardcoded vessel type
- **Enables**: Session 04+ (command registration, interior rooms) - proper vessel types needed before advanced features

### Project Progression

Session 03 is the clear next step in the critical path. With sessions 01 and 02 complete:
- Headers are clean and properly organized
- Vessel movement now correctly allocates wilderness rooms

The remaining gap is that all vessels are treated identically - using a hardcoded `VESSEL_TYPE_SAILING_SHIP` placeholder. This session replaces that with proper per-vessel type mapping, enabling:
- Rafts restricted to shallow water
- Ships navigating deep ocean
- Airships ignoring terrain entirely
- Submarines diving below waterline

This is foundational work that must be complete before Phase 2 commands (docking, boarding) make sense, since those features depend on vessels having correct terrain capabilities.

---

## Session Overview

### Objective
Implement per-vessel type mapping to replace the hardcoded VESSEL_TYPE_SAILING_SHIP placeholder, enabling different vessel types (raft, boat, ship, airship, submarine) to have appropriate terrain capabilities.

### Key Deliverables
1. Working vessel_terrain_caps implementation with per-type definitions
2. Per-vessel type terrain validation (can_traverse checks)
3. Proper speed modifiers per vessel/terrain combination
4. Test coverage for each vessel type

### Scope Summary
- **In Scope (MVP)**: Map vessel_class enum to terrain capabilities, implement vessel_terrain_caps structure usage, replace VESSEL_TYPE_SAILING_SHIP hardcoding, wire terrain speed modifiers per vessel type, implement can_traverse checks
- **Out of Scope**: Adding new vessel types, complex terrain interactions, weather effects per vessel type (already working)

---

## Technical Considerations

### Technologies/Patterns
- ANSI C90/C89 (no C99 features)
- vessel_terrain_caps structure from PRD Section 4
- Terrain sector types from wilderness system
- Speed modifier float arrays (40 sector types)

### Potential Challenges
- Ensuring all 8 vessel types have properly defined capabilities
- Correct terrain speed modifiers for each vessel/sector combination
- Airship altitude navigation may need special handling
- Submarine depth mechanics need careful bounds checking

### Relevant Considerations
- [P00] **Per-vessel type mapping missing**: This session directly addresses the architectural concern that wilderness movement uses placeholder VESSEL_TYPE_SAILING_SHIP
- [P00] **Don't use C99/C11 features**: Must use C90 syntax (no // comments, no declarations after statements)
- [P00] **Don't skip NULL checks**: Critical when accessing vessel structures

---

## Alternative Sessions

If this session is blocked:
1. **session04-phase2-command-registration** - Register Phase 2 commands (dock, undock, etc.) in interpreter.c. Can be done in parallel but vessel types should be correct first.
2. **session05-interior-room-generation-wiring** - Wire generate_ship_interior() calls. Independent but lower priority than type system.

---

## Next Steps

Run `/sessionspec` to generate the formal specification.
