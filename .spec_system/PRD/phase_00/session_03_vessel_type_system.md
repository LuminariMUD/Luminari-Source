# Session 03: Vessel Type System

**Session ID**: `phase00-session03-vessel-type-system`
**Status**: Not Started
**Estimated Tasks**: ~15-18
**Estimated Duration**: 2-3 hours

---

## Objective

Implement per-vessel type mapping to replace the hardcoded VESSEL_TYPE_SAILING_SHIP placeholder, enabling different vessel types (raft, boat, ship, airship, submarine) to have appropriate terrain capabilities.

---

## Scope

### In Scope (MVP)
- Map vessel_class enum to terrain capabilities
- Implement vessel_terrain_caps structure usage
- Replace VESSEL_TYPE_SAILING_SHIP hardcoding
- Wire terrain speed modifiers per vessel type
- Implement can_traverse checks per vessel type
- Test different vessel types against terrain

### Out of Scope
- Adding new vessel types
- Complex terrain interactions
- Weather effects per vessel type (already working)

---

## Prerequisites

- [ ] Session 01 completed (clean headers)
- [ ] Session 02 completed (movement working)
- [ ] Understanding of vessel_class enum

---

## Deliverables

1. Working vessel_terrain_caps implementation
2. Per-vessel type terrain validation
3. Proper speed modifiers per vessel/terrain combination
4. Test coverage for each vessel type

---

## Success Criteria

- [ ] Each vessel type has defined terrain capabilities
- [ ] Rafts restricted to shallow water/rivers
- [ ] Boats restricted to coastal waters
- [ ] Ships can traverse deep ocean
- [ ] Airships ignore terrain (altitude navigation)
- [ ] Submarines can dive below waterline
- [ ] Speed modifiers applied correctly per terrain
- [ ] Invalid terrain blocked with appropriate message

---

## Technical Notes

### Vessel Types (from PRD Section 4)

| Type | Description | Terrain Capabilities |
|------|-------------|---------------------|
| VESSEL_RAFT | Small, basic | Rivers, shallow water only |
| VESSEL_BOAT | Medium craft | Coastal waters |
| VESSEL_SHIP | Large vessel | Ocean-capable |
| VESSEL_WARSHIP | Combat vessel | Ocean, heavily armed |
| VESSEL_AIRSHIP | Flying vessel | Ignores terrain, altitude navigation |
| VESSEL_SUBMARINE | Underwater | Depth navigation below waterline |
| VESSEL_TRANSPORT | Cargo/passenger | Ocean, high capacity |
| VESSEL_MAGICAL | Special | Unique navigation capabilities |

### Structure from PRD

```c
struct vessel_terrain_caps {
  bool can_traverse_ocean;       /* Deep water navigation */
  bool can_traverse_shallow;     /* Shallow water/rivers */
  bool can_traverse_air;         /* Airship flight */
  bool can_traverse_underwater;  /* Submarine diving */
  int min_water_depth;           /* Minimum depth required */
  int max_altitude;              /* Maximum flight altitude */
  float terrain_speed_mod[40];   /* Speed modifier by sector type */
};
```

### Current Placeholder

From PRD:
> Wilderness movement currently uses placeholder VESSEL_TYPE_SAILING_SHIP; per-ship vessel_class mapping not yet implemented.

### Files to Modify

- src/vessels.h - Ensure vessel_terrain_caps properly defined
- src/vessels.c - Implement terrain capability checks in movement
