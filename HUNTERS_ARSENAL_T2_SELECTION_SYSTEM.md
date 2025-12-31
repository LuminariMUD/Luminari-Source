# Hunter's Arsenal Tier 2 - Favored Enemy & Terrain Selection System

## Overview
This implementation extends the Hunter's Arsenal Tier 2 perks for Inquisitors with a comprehensive selection system for multiple favored enemies and favored terrains based on perk ranks.

## Changes Made

### 1. Storage Structure Extensions

#### structs.h (Lines ~6700)
- **Added:** `favored_terrains[MAX_ENEMIES]` array to player_specials->saved structure
- **Purpose:** Supports multiple favored terrain selections (1 base + 1 per Terrain Mastery rank)
- **Backward Compatibility:** Legacy `inq_favored_terrain` field retained for compatibility

#### utils.h (Line ~2755)
- **Added:** `#define GET_FAVORED_TERRAINS(ch, slot)` macro for array access
- **Complements:** Existing `GET_FAVORED_TERRAIN()` macro for single-value legacy support

#### db.c (Lines ~6700-6710)
- **Modified:** `init_char()` function to initialize `favored_terrains` array to -1 for all slots

### 2. Perk Helper Functions

#### perks.c (Lines 1520-1545)
**Updated Functions:**
- `is_inquisitor_in_favored_terrain()` - Now checks all terrain array slots for any matching terrain

**New Functions:**
- `get_inquisitor_max_favored_enemies()` - Returns 1 (base) + number of Favored Enemy Enhancement ranks
- `get_inquisitor_max_favored_terrains()` - Returns 1 (base) + number of Terrain Mastery ranks
- `count_inquisitor_favored_enemies()` - Counts currently selected favored enemies
- `count_inquisitor_favored_terrains()` - Counts currently selected favored terrains

#### perks.h (Lines 73-82)
- **Added:** Function declarations for all new helper functions

### 3. Commands

#### do_favored_terrain (act.offensive.c, Lines 13084-13247)
**Enhanced Features:**
- **view**: Shows all selected terrains with slot numbers and counts (X/Y)
- **list**: Lists all 9 available terrain types with indices
- **clear**: Clear all terrains OR clear specific terrain from a slot (e.g., `clear 2`)
- **Add terrain**: Can select multiple terrains based on max available
- **Slot management**: Terrains are assigned to slots 0-9 (using MAX_ENEMIES array)
- **Validation**: Prevents duplicate selections, enforces maximum based on ranks

**Output Examples:**
```
favoredterrain view
Your Favored Terrains (2/4):
  [0] forest
  [1] mountain

favoredterrain forest
Favored terrain added to slot 2: forest. You gain +2 initiative and Stealth there.

favoredterrain clear 1
You clear your favored terrain from slot 1.
```

#### do_inquisitor_favored_enemy (NEW, act.offensive.c, Lines 13249-13393)
**Command: `inqenemy`**

**Features:**
- **Usage:** `inqenemy <creature|list|view|clear [#]>`
- **view**: Shows all selected creature types with slot numbers and counts (X/Y)
- **list**: Lists all 17 race family types with indices
- **clear**: Clear all enemies OR clear specific enemy from a slot
- **Add enemy**: Can select multiple creature types based on perk ranks
- **Slot management**: Uses existing favored_enemy[MAX_ENEMIES] array (10 slots)
- **Validation**: Prevents duplicates, enforces max based on Favored Enemy Enhancement ranks

**Output Examples:**
```
inqenemy view
Your Favored Enemies (2/3):
  [0] humanoid
  [1] giant

inqenemy dragon
Favored enemy added to slot 2: dragon. You gain +4 attack, damage, and +2 AC against them.

inqenemy clear 1
You clear your favored enemy from slot 1.
```

#### interpreter.c (Line 592)
- **Registered:** `inqenemy` command pointing to `do_inquisitor_favored_enemy`

#### act.h (Line 291)
- **Added:** `ACMD_DECL(do_inquisitor_favored_enemy)` declaration

### 4. Selection Limits

**Favored Enemy Enhancement:**
- Base: 1 selection (from Favored Terrain perk)
- Per Rank: +1 selection per rank of Favored Enemy Enhancement (up to 4 ranks)
- Maximum: 5 total selections

**Terrain Mastery:**
- Base: 1 selection (from Favored Terrain perk)
- Per Rank: +1 selection per rank of Terrain Mastery (up to 3 ranks)
- Maximum: 4 total selections

### 5. Bonus Calculations

**Favored Enemy Enhancement Bonuses:**
- Attack Bonus: +2 per perk rank
- Damage Bonus: +2 per perk rank
- AC Bonus: +1 per perk rank

**Terrain Mastery Bonuses:**
- Survival Skill: +2 per perk rank (in favored terrains)
- Initiative: +2 (in favored terrains)
- Stealth: Enhanced (in favored terrains)
- Leave No Trail: No tracks in favored terrains

**Ambush Predator:**
- First Strike Bonus: +3d6 damage (one-time per combat)

**Hunter's Endurance:**
- Move Regeneration: +2 per round
- Fatigue Removal: 5% chance per round

## Compilation Status
✅ **Successfully compiled** with no errors or warnings related to these changes.

## Testing Recommendations

1. **Slot Assignment:** Verify creatures/terrains assign to correct slots
2. **Maximum Limits:** Test that selections enforce perk rank limits
3. **Bonus Calculations:** Confirm attack/damage/AC/survival bonuses apply correctly
4. **Multiple Selections:** Verify bonuses apply to ALL selected creatures/terrains
5. **Clear Functions:** Test clearing individual slots and all selections
6. **List Functions:** Verify output formatting and completeness
7. **Backward Compatibility:** Ensure legacy single-value fields still function

## Usage Examples for Players

### Selecting Multiple Favored Enemies
```
inqenemy list                    # Show available types
inqenemy humanoid               # Add humanoid as favored enemy
inqenemy giant                  # Add giant as favored enemy
inqenemy dragon                 # Add dragon as favored enemy
inqenemy view                   # Show current selections
inqenemy clear 1                # Remove slot 1
inqenemy clear                  # Clear all
```

### Selecting Multiple Favored Terrains
```
favoredterrain list             # Show available terrains
favoredterrain forest           # Add forest as favored
favoredterrain mountain         # Add mountain as favored
favoredterrain view             # Show current selections
favoredterrain clear 0          # Remove slot 0
favoredterrain clear            # Clear all
```

## Database/Save Compatibility

**Existing Characters:**
- Legacy `inq_favored_terrain` field still functions
- New `favored_terrains` array initialized on first character load
- No character data corruption expected
- Supports seamless transition for existing players

## Files Modified

1. [src/structs.h](src/structs.h) - Added terrain array storage
2. [src/utils.h](src/utils.h) - Added macro for terrain array access
3. [src/db.c](src/db.c) - Initialize terrain array
4. [src/perks.c](src/perks.c) - Added helper functions, updated terrain checking
5. [src/perks.h](src/perks.h) - Added function declarations
6. [src/act.offensive.c](src/act.offensive.c) - Enhanced favored_terrain command, added inqenemy command
7. [src/act.h](src/act.h) - Added inqenemy command declaration
8. [src/interpreter.c](src/interpreter.c) - Registered inqenemy command

## Perk Integration

These selection systems work with the existing tier 2 perks:
- **Favored Enemy Enhancement** (PERK_INQUISITOR_FAVORED_ENEMY_ENHANCEMENT) - Unlocks multiple selections
- **Terrain Mastery** (PERK_INQUISITOR_TERRAIN_MASTERY) - Unlocks terrain selections + bonuses
- **Ambush Predator** (PERK_INQUISITOR_AMBUSH_PREDATOR) - First strike bonus
- **Hunter's Endurance** (PERK_INQUISITOR_HUNTERS_ENDURANCE) - Move regen + fatigue removal
