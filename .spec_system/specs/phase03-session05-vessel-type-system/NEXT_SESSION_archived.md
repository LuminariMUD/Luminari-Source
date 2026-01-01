# NEXT_SESSION.md

## Session Recommendation

**Generated**: 2025-12-30
**Project State**: Phase 03 - Optimization & Polish
**Completed Sessions**: 27

---

## Recommended Next Session

**Session ID**: `phase03-session05-vessel-type-system`
**Session Name**: Vessel Type System
**Estimated Duration**: 2-3 hours
**Estimated Tasks**: 14-18

---

## Why This Session Next?

### Prerequisites Met
- [x] Session 04 complete (dynamic wilderness rooms working)
- [x] vessel_terrain_caps structure defined (in vessels.h)
- [x] Terrain speed modifier table populated
- [x] Code consolidation complete (duplicate structs cleaned up)
- [x] Command registration complete (Phase 2 commands wired)

### Dependencies
- **Builds on**: phase03-session04-dynamic-wilderness-rooms (room allocation for vessel movement)
- **Enables**: phase03-session06-final-testing-documentation (final testing requires all features)

### Project Progression
This is the natural next step in the Phase 03 sequence. Sessions 01-04 completed the foundational cleanup (code consolidation, interior movement, command registration, dynamic rooms). Session 05 addresses the remaining hardcoded placeholder (`VESSEL_TYPE_SAILING_SHIP`) that prevents proper per-vessel type behavior. Session 06 (final testing) explicitly requires all features implemented before comprehensive testing can begin.

---

## Session Overview

### Objective
Replace the hardcoded VESSEL_TYPE_SAILING_SHIP placeholder with a proper per-vessel type mapping system that correctly applies terrain capabilities and speed modifiers based on vessel class.

### Key Deliverables
1. Per-vessel type lookup function
2. Terrain capability mapping for all 8 vessel types
3. Speed modifier application per vessel type
4. Movement validation per vessel type
5. Unit tests for type system (target: 16+ tests)

### Scope Summary
- **In Scope (MVP)**: Implement vessel_class lookup, map 8 vessel types to terrain capabilities, apply correct speed modifiers, wire into movement validation
- **Out of Scope**: New vessel types, Z-axis navigation changes, ship combat modifications, DB schema changes

---

## Technical Considerations

### Technologies/Patterns
- C90/C89 code (no C99/C11 features)
- Existing vessel_terrain_caps structure from vessels.h
- Terrain sector types from wilderness.c
- CuTest unit testing framework

### Potential Challenges
- Ensuring all 8 vessel types have distinct and balanced capabilities
- Proper speed modifier calculation across varied terrain
- Maintaining backward compatibility with existing vessel data

### Relevant Considerations
- [P03] **Per-vessel type mapping missing**: This session directly addresses this Active Concern (vessels.h currently uses VESSEL_TYPE_SAILING_SHIP placeholder)
- [P01] **Greyhawk system as foundation**: Leverage existing speed/terrain framework
- [P02] **Unified transport interface**: Consider how vessel types integrate with transport_data abstraction

---

## Alternative Sessions

If this session is blocked:
1. **phase03-session06-final-testing-documentation** - Can begin testing/documentation on completed features, though full coverage requires Session 05
2. **Additional Phase 02 polish** - If type system dependencies are not ready

---

## Next Steps

Run `/sessionspec` to generate the formal specification.
