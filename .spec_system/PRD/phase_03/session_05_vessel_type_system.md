# Session 05: Vessel Type System

**Session ID**: `phase03-session05-vessel-type-system`
**Status**: Not Started
**Estimated Tasks**: ~14-18
**Estimated Duration**: 2-3 hours

---

## Objective

Replace the hardcoded VESSEL_TYPE_SAILING_SHIP placeholder with a proper per-vessel type mapping system that correctly applies terrain capabilities and speed modifiers based on vessel class.

---

## Scope

### In Scope (MVP)

- Implement vessel_class lookup from vessel data
- Map vessel_class to terrain capabilities
- Apply correct speed modifiers per vessel type
- Update terrain restriction checks per vessel type
- Support all 8 vessel types (RAFT, BOAT, SHIP, WARSHIP, AIRSHIP, SUBMARINE, TRANSPORT, MAGICAL)
- Wire type system into movement validation
- Wire type system into speed calculations
- Unit tests for each vessel type's capabilities

### Out of Scope

- New vessel types beyond existing 8
- Z-axis navigation implementation (if not already working)
- Ship combat modifications
- Database schema changes for vessel types

---

## Prerequisites

- [ ] Session 04 complete (dynamic rooms working)
- [ ] vessel_terrain_caps structure defined
- [ ] Terrain speed modifier table populated

---

## Deliverables

1. Per-vessel type lookup function
2. Terrain capability mapping for all 8 types
3. Speed modifier application per type
4. Movement validation per type
5. Unit tests for type system (target: 16+ tests)
6. Updated documentation of vessel type behaviors

---

## Success Criteria

- [ ] Each vessel type has unique terrain capabilities
- [ ] RAFT limited to rivers/shallow water
- [ ] BOAT works in coastal waters
- [ ] SHIP/WARSHIP/TRANSPORT work in ocean
- [ ] AIRSHIP ignores terrain (flight)
- [ ] SUBMARINE supports depth navigation
- [ ] Speed modifiers correctly applied per type
- [ ] No hardcoded VESSEL_TYPE_SAILING_SHIP references remain
- [ ] All tests pass
