# Crafting Stations Implementation

## Overview
This document describes the implementation of crafting station requirements for the crafting system. Players must now be in a room with the appropriate crafting station object to perform crafting activities.

## New Item Flags

The following item flags have been added to `src/structs.h`:

- `ITEM_CRAFTING_FORGE` (108) - For armorsmithing, metalworking, and weaponsmithing
- `ITEM_CRAFTING_ALCHEMY_LAB` (109) - For alchemy
- `ITEM_CRAFTING_JEWELCRAFTING_STATION` (110) - For jewelcrafting
- `ITEM_CRAFTING_TANNERY` (111) - For leatherworking
- `ITEM_CRAFTING_CARPENTRY_TABLE` (112) - For woodworking

Note: `ITEM_CRAFTING_LOOM` (107) already existed for tailoring

## Crafting Skill to Station Mapping

| Crafting Skill | Required Station | Flag |
|----------------|------------------|------|
| Alchemy | Alchemy Lab | ITEM_CRAFTING_ALCHEMY_LAB |
| Armorsmithing | Forge | ITEM_CRAFTING_FORGE |
| Jewelcrafting | Jewelcrafting Station | ITEM_CRAFTING_JEWELCRAFTING_STATION |
| Leatherworking | Tannery | ITEM_CRAFTING_TANNERY |
| Metalworking | Forge | ITEM_CRAFTING_FORGE |
| Tailoring | Loom | ITEM_CRAFTING_LOOM |
| Weaponsmithing | Forge | ITEM_CRAFTING_FORGE |
| Woodworking | Carpentry Table | ITEM_CRAFTING_CARPENTRY_TABLE |

## Modified Files

### src/structs.h
- Added new ITEM_CRAFTING_* flags (108-112)
- Updated NUM_ITEM_FLAGS to 113

### src/constants.c
- Added string names for new flags in extra_bits array:
  - "Crafting-Forge"
  - "Crafting-Alchemy-Lab"
  - "Crafting-Jewelcrafting-Station"
  - "Crafting-Tannery"
  - "Crafting-Carpentry-Table"

### src/crafting_new.h
- Added function prototypes:
  - `bool has_crafting_station_in_room(struct char_data *ch, int skill)`
  - `int get_required_crafting_station(int skill)`
  - `const char *get_crafting_station_name(int skill)`

### src/crafting_new.c

#### New Helper Functions

1. **get_required_crafting_station(int skill)**
   - Returns the item flag required for a given crafting skill
   - Returns -1 if no station is required

2. **get_crafting_station_name(int skill)**
   - Returns a human-readable name of the required crafting station
   - Used in error messages to players

3. **has_crafting_station_in_room(struct char_data *ch, int skill)**
   - Checks if the player is in a room with the required crafting station
   - Iterates through room contents looking for objects with the required flag
   - Returns TRUE if station is found or no station is required
   - Returns FALSE if station is required but not found

#### Modified Functions

1. **begin_current_craft(struct char_data *ch)**
   - Added crafting station check before allowing player to start crafting
   - Shows error message if required station is not present
   - Uses skill_type directly (contains ABILITY_CRAFT_* values from material_to_craft_skill())

2. **newcraft_refine (begin refine section)**
   - Added crafting station check before allowing player to start refining
   - Shows error message if required station is not present
   - Uses skill_type directly (refining_recipes use ABILITY_CRAFT_* values)

3. **start_supply_order(struct char_data *ch)**
   - Added crafting station check before allowing player to start supply order work
   - Shows error message if required station is not present
   - Converts skill_type from CRAFT_SKILL_* to ABILITY_CRAFT_* using recipe_skill_to_actual_crafting_skill()
   - Note: Supply orders use crafting_recipes which store CRAFT_SKILL_* values, requiring conversion

## Player-Facing Changes

### Error Messages
When a player tries to craft without the required station, they will see:
```
You need to be in a room with <station name> to craft this item.
```

Where `<station name>` is one of:
- "a forge"
- "an alchemy lab"
- "a jewelcrafting station"
- "a tannery"
- "a loom"
- "a carpentry table"

### Affected Commands
The following crafting actions now require appropriate stations:
- `craft start` - Creating new items
- `refine start` - Refining materials
- `supplyorder start` - Working on supply orders

### Unaffected Activities
The following activities do NOT require crafting stations:
- Harvesting (mining, hunting, forestry, gathering)
- Surveying
- Resizing items (uses harvesting skills, not crafting skills)

## Setting Up Crafting Stations

### In-Game Setup
Builders can create crafting station objects and set the appropriate flags using:

```
oset <object> flags <flag-name>
```

Flag names:
- `crafting-forge`
- `crafting-alchemy-lab`
- `crafting-jewelcrafting-station`
- `crafting-tannery`
- `crafting-carpentry-table`
- `crafting-loom`

### Example
```
oset anvil flags crafting-forge
```

### World Files
In world files, the flags can be set using their numeric values (108-112) or string names in the object definition.

## Technical Notes

### Design Decisions

1. **Station Check Timing**: Checks are performed when the player begins the crafting activity, not during setup. This allows players to prepare their crafting project anywhere, but they must be at the appropriate station to actually start work.

2. **Shared Stations**: Multiple crafting skills can share the same station (e.g., armorsmithing, metalworking, and weaponsmithing all use a forge).

3. **No Station Required**: Skills that return -1 from `get_required_crafting_station()` do not require any station. This is primarily for harvesting skills.

4. **Room Contents**: The system checks objects in the room's contents list, not in player inventory or equipment. Stations must be placed in the room.

5. **Skill Value Types**: The codebase uses two different skill constant systems:
   - **CRAFT_SKILL_*** (stored in crafting_recipes): CRAFT_SKILL_WEAPONSMITH, CRAFT_SKILL_TAILOR, etc.
   - **ABILITY_CRAFT_*** (used by abilities): ABILITY_CRAFT_WEAPONSMITHING, ABILITY_CRAFT_TAILORING, etc.
   
   Regular crafting and refining use ABILITY_CRAFT_* values directly. Supply orders use CRAFT_SKILL_* values and require conversion via `recipe_skill_to_actual_crafting_skill()` before checking station requirements.

### Future Enhancements

Potential future improvements:
- Personal portable crafting stations (lower quality/slower?)
- Station quality affecting crafting success or speed
- Station durability/maintenance requirements
- Multiple stations in one room for different skills
- Station bonuses to crafting skill checks

## Testing

### Test Cases

1. **Positive Tests**:
   - Player in room with forge can start armorsmithing
   - Player in room with alchemy lab can start alchemy
   - Player in room with loom can start tailoring

2. **Negative Tests**:
   - Player without forge cannot start weaponsmithing
   - Player without tannery cannot start leatherworking
   - Error messages display correctly

3. **Edge Cases**:
   - Multiple stations in one room work correctly
   - Harvesting activities don't require stations
   - Supply orders require appropriate stations

### Testing Commands
```
craft itemtype weapon
craft specifictype longsword
craft variant iron
craft start  (should fail without forge)
```

## Integration with Quartermaster System

The crafting station system works alongside the quartermaster system:
- **Quartermaster mobs** (MOB_QUARTERMASTER flag) are required for:
  - Requesting supply orders (`supplyorder request`)
  - Completing supply orders (`supplyorder complete`)
  - Viewing available orders (`supplyorder list`)
  - Selecting orders (`supplyorder select`)

- **Crafting stations** are required for:
  - Starting crafting work (`craft start`)
  - Starting refining work (`refine start`)
  - Starting supply order work (`supplyorder start`)

Both systems are independent - you need a quartermaster to get a supply order, but you need a crafting station to work on it.

## Version History

- **Initial Implementation**: October 17, 2025
  - Added 5 new item flags for crafting stations
  - Implemented station checking for craft, refine, and supply order systems
  - Created helper functions for station validation
