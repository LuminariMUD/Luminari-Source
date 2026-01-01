# Session Specification

**Session ID**: `phase00-session03-vessel-type-system`
**Phase**: 00 - Core Vessel System
**Status**: Not Started
**Created**: 2025-12-29

---

## 1. Session Overview

This session implements per-vessel type mapping to replace the hardcoded `VESSEL_TYPE_SAILING_SHIP` placeholder that currently treats all vessels identically. The current implementation has working wilderness movement (from Session 02), but all vessels use the same terrain capabilities and speed modifiers regardless of their actual type.

The vessel system defines 8 distinct vessel types via the `vessel_class` enum (VESSEL_RAFT through VESSEL_MAGICAL), each with fundamentally different navigation capabilities. Rafts should only traverse rivers and shallow water, while airships should ignore terrain entirely. This session wires the existing `vessel_type` field in `greyhawk_ship_data` to the terrain validation and speed modifier systems.

Completing this session is foundational for Phase 2 commands (docking, boarding) since those features depend on vessels having correct terrain capabilities. A warship that behaves like a raft, or a submarine that cannot dive, would break the intended gameplay mechanics.

---

## 2. Objectives

1. Create terrain capability data tables mapping each of the 8 `vessel_class` enum values to their specific navigation abilities
2. Replace all hardcoded `VESSEL_TYPE_SAILING_SHIP` references with actual vessel type lookups from ship data
3. Implement complete terrain validation for all 8 vessel types with appropriate restrictions
4. Wire terrain speed modifiers for each vessel/terrain combination using the `vessel_terrain_caps` structure

---

## 3. Prerequisites

### Required Sessions
- [x] `phase00-session01-header-cleanup-foundation` - Clean header structure, vessel_class enum defined
- [x] `phase00-session02-dynamic-wilderness-room-allocation` - Working wilderness movement, get_or_allocate_wilderness_room() functional

### Required Tools/Knowledge
- Understanding of sector types (SECT_OCEAN, SECT_WATER_SWIM, SECT_WATER_NOSWIM, SECT_UNDERWATER, etc.)
- Familiarity with vessel_class enum values and intended capabilities per PRD Section 4
- Knowledge of greyhawk_ship_data structure (specifically vessel_type field at line 469)

### Environment Requirements
- Access to src/vessels.c and src/vessels.h
- Build environment functional (verified via Session 01/02)

---

## 4. Scope

### In Scope (MVP)
- Create static terrain capability definitions for all 8 vessel_class types
- Implement get_vessel_terrain_caps() function to return capabilities for a vessel type
- Implement get_vessel_type_from_ship() helper to read vessel_type from ship data
- Update can_vessel_traverse_terrain() to handle all 8 vessel types
- Update get_terrain_speed_modifier() to handle all 8 vessel types
- Replace hardcoded VESSEL_TYPE_SAILING_SHIP in move_ship_wilderness() (line 530)
- Replace hardcoded VESSEL_TYPE_SAILING_SHIP in do_greyhawk_speed() (line 755)
- Add terrain-specific denial messages (e.g., "Your raft cannot navigate deep ocean!")
- Validate vessel type bounds checking (default to VESSEL_SHIP for invalid types)

### Out of Scope (Deferred)
- Adding new vessel types beyond the existing 8 - *Reason: Not needed for MVP*
- Weather effects per vessel type - *Reason: Already working, not broken*
- Complex multi-terrain interactions (e.g., river-to-ocean transitions) - *Reason: Future enhancement*
- Dynamic capability modifications (enchantments, damage effects) - *Reason: Phase 2+*
- Interior room generation based on vessel type - *Reason: Session 05*

---

## 5. Technical Approach

### Architecture
Create a static data table mapping vessel_class enum values to vessel_terrain_caps structures. This lookup table pattern avoids runtime allocation and provides O(1) access to vessel capabilities. The existing switch statements in can_vessel_traverse_terrain() and get_terrain_speed_modifier() will be expanded to cover all 8 vessel types.

### Design Patterns
- **Lookup Table**: Static array indexed by vessel_class enum for terrain capabilities
- **Accessor Function**: get_vessel_terrain_caps() returns const pointer to capability struct
- **Helper Function**: get_vessel_type_from_ship() encapsulates vessel type extraction with bounds checking

### Technology Stack
- ANSI C90/C89 (no C99 features)
- Static const data tables (no dynamic allocation)
- Existing greyhawk_ship_data structure

---

## 6. Deliverables

### Files to Create
| File | Purpose | Est. Lines |
|------|---------|------------|
| None | All changes in existing files | - |

### Files to Modify
| File | Changes | Est. Lines |
|------|---------|------------|
| `src/vessels.c` | Add terrain capability table, get_vessel_terrain_caps(), get_vessel_type_from_ship(), expand switch statements, replace hardcoded placeholders | ~150 |
| `src/vessels.h` | Add function prototypes for new accessor functions | ~10 |

---

## 7. Success Criteria

### Functional Requirements
- [ ] VESSEL_RAFT restricted to SECT_WATER_SWIM (shallow water/rivers) only
- [ ] VESSEL_BOAT restricted to SECT_WATER_SWIM and SECT_WATER_NOSWIM (coastal waters)
- [ ] VESSEL_SHIP can traverse SECT_WATER_NOSWIM and SECT_OCEAN (deep water)
- [ ] VESSEL_WARSHIP has same capabilities as VESSEL_SHIP
- [ ] VESSEL_AIRSHIP ignores water requirements, altitude-based navigation
- [ ] VESSEL_SUBMARINE can traverse all water types including SECT_UNDERWATER
- [ ] VESSEL_TRANSPORT has same capabilities as VESSEL_SHIP
- [ ] VESSEL_MAGICAL has expanded capabilities (most terrain types)
- [ ] Speed modifiers vary correctly by vessel/terrain combination
- [ ] Invalid terrain attempts produce vessel-type-specific error messages
- [ ] No hardcoded VESSEL_TYPE_SAILING_SHIP references remain

### Testing Requirements
- [ ] Manual test: Create raft, verify blocked from ocean
- [ ] Manual test: Create ship, verify can traverse ocean
- [ ] Manual test: Create airship, verify ignores terrain
- [ ] Manual test: Create submarine, verify underwater navigation
- [ ] Code review: Verify all switch statements have cases for all 8 types

### Quality Gates
- [ ] All files ASCII-encoded
- [ ] Unix LF line endings
- [ ] Code follows project conventions (C90, /* */ comments, 2-space indent)
- [ ] No compiler warnings with -Wall -Wextra
- [ ] NULL checks on all pointer accesses

---

## 8. Implementation Notes

### Key Considerations
- The vessel_type field in greyhawk_ship_data uses enum vessel_class, but existing code uses VESSEL_TYPE_* defines - must bridge this gap
- Default to VESSEL_SHIP behavior for uninitialized or invalid vessel_type values
- Speed modifier of 0 indicates impassable terrain, not stopped vessel
- Airship terrain checks must consider z-coordinate (altitude) for mountain avoidance

### Potential Challenges
- **Enum vs Define mismatch**: vessel_class enum (VESSEL_RAFT=0) differs from VESSEL_TYPE_* defines (VESSEL_TYPE_SAILING_SHIP=1). Solution: Use vessel_class enum consistently, deprecate old defines.
- **Missing vessel_type initialization**: Ships may have vessel_type=0 (VESSEL_RAFT) by default. Solution: Add bounds check and default to VESSEL_SHIP for value 0 if ship is large.
- **Submarine depth validation**: Current z-coordinate handling is minimal. Solution: Ensure submarines require negative z for underwater, positive z blocked.

### Relevant Considerations
- [P00] **Per-vessel type mapping missing**: This session directly resolves this architectural concern
- [P00] **Don't use C99/C11 features**: All new code uses C90 syntax only
- [P00] **Don't skip NULL checks**: get_vessel_type_from_ship() will validate ship pointer

### ASCII Reminder
All output files must use ASCII-only characters (0-127).

---

## 9. Testing Strategy

### Unit Tests
- Test get_vessel_terrain_caps() returns valid capabilities for each vessel_class value
- Test get_vessel_type_from_ship() with valid ship, NULL ship, invalid shipnum
- Test can_vessel_traverse_terrain() for each vessel type against key sector types

### Integration Tests
- Test move_ship_wilderness() with different vessel types and terrain combinations
- Verify speed modifiers applied correctly after terrain transitions

### Manual Testing
- Create test ships of each type using builder commands
- Navigate each ship type through different terrain zones
- Verify raft blocked from ocean with appropriate message
- Verify airship can fly over mountains at altitude
- Verify submarine can dive (negative z) in deep water

### Edge Cases
- Vessel type out of bounds (negative or > VESSEL_MAGICAL)
- Ship data pointer is NULL
- Movement to boundary of wilderness coordinate system
- Altitude/depth boundary conditions (z = 0 transitions)

---

## 10. Dependencies

### External Libraries
- None (uses existing LuminariMUD infrastructure)

### Other Sessions
- **Depends on**: phase00-session01 (header cleanup), phase00-session02 (wilderness room allocation)
- **Depended by**: phase00-session04 (command registration), phase00-session05 (interior rooms), phase00-session06 (interior movement)

---

## Next Steps

Run `/tasks` to generate the implementation task checklist.
