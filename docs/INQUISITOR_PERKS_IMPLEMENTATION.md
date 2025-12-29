# Inquisitor Perks Implementation - Tier 1 Summary

## Implementation Date
December 24, 2025

## Overview
Implemented the Tier 1 perks from the **Judgment & Spellcasting** tree for Inquisitors, as defined in the refined perks document.

## Changes Made

### 1. Added Perk Categories (structs.h)
Added four new inquisitor perk categories:
- `PERK_CATEGORY_JUDGMENT_SPELLCASTING` (42)
- `PERK_CATEGORY_HUNTERS_ARSENAL` (43)
- `PERK_CATEGORY_INVESTIGATION_PERCEPTION` (44)
- `PERK_CATEGORY_ADAPTABLE_TACTICS` (45)

### 2. Updated Category Names Array (perks.c)
Added human-readable names for the new categories in `perk_category_names[]`:
- "Judgment & Spellcasting"
- "Hunter's Arsenal"
- "Investigation & Perception"
- "Adaptable Tactics"

### 3. Added Perk IDs (structs.h)
Defined four new perk constants:
- `PERK_INQUISITOR_EMPOWERED_JUDGMENT` (1444)
- `PERK_INQUISITOR_SWIFT_SPELLCASTER` (1445)
- `PERK_INQUISITOR_SPELL_FOCUS_DIVINATION` (1446)
- `PERK_INQUISITOR_JUDGMENT_RECOVERY` (1447)

Updated `NUM_PERKS` from 1444 to 1448.

### 4. Function Declarations (perks.h)
Added:
- `define_inquisitor_perks()` - Main definition function
- Helper functions:
  - `get_inquisitor_empowered_judgment_bonus()`
  - `can_inquisitor_dual_judgment()`
  - `has_inquisitor_swift_spellcaster()`
  - `get_inquisitor_divination_dc_bonus()`
  - `has_inquisitor_divination_bonus_slot()`
  - `has_inquisitor_judgment_recovery()`

### 5. Perk Definitions (perks.c)
Implemented `define_inquisitor_perks()` with the following Tier 1 perks:

#### Empowered Judgment
- **Ranks**: 3 (1 point each)
- **Effect**: +1 to judgment bonuses per rank
- **Special**: At rank 3, can maintain two judgments simultaneously once per encounter for 1d4 rounds
- **Category**: Judgment & Spellcasting

#### Swift Spellcaster
- **Ranks**: 1 (1 point)
- **Effect**: Reduce casting time of inquisitor spells by one step once per encounter; +2 to concentration checks
- **Category**: Judgment & Spellcasting

#### Spell Focus: Divination
- **Ranks**: 2 (1 point each)
- **Effect**: +1 DC to divination spells per rank
- **Special**: At rank 2, gain one additional divination spell slot per spell level
- **Category**: Judgment & Spellcasting

#### Judgment Recovery
- **Ranks**: 1 (1 point)
- **Effect**: Regain one use of judgment ability when you reduce an enemy to 0 hit points or below
- **Special**: Once per encounter
- **Category**: Judgment & Spellcasting

### 6. Helper Function Implementations (perks.c)
Implemented all six helper functions for easy access to perk data:
- Bonus calculations for judgment and divination DCs
- Boolean checks for perk possession
- Rank queries for multi-rank perks

## Compilation Status
✅ **Successfully compiled** with no errors or warnings

## Files Modified
1. `/home/krynn/code/src/structs.h` - Added categories and perk IDs
2. `/home/krynn/code/src/perks.h` - Added function declarations
3. `/home/krynn/code/src/perks.c` - Added implementations and helper functions

## Next Steps
To complete the Inquisitor perk system, the following should be implemented:
1. **Tier 2 perks** from Judgment & Spellcasting tree (4 perks)
2. **Tier 3 perks** from Judgment & Spellcasting tree (4 perks)
3. **Tier 4 perks** from Judgment & Spellcasting tree (4 perks)
4. Complete implementation of the other three trees:
   - Hunter's Arsenal (16 perks across 4 tiers)
   - Investigation & Perception (16 perks across 4 tiers)
   - Adaptable Tactics (16 perks across 4 tiers)

## Integration Points
The following game systems will need to reference these perks:
- **Judgment system**: Apply bonuses from Empowered Judgment
- **Spellcasting system**: 
  - Handle Swift Spellcaster casting time reduction
  - Apply Spell Focus: Divination DC bonuses
  - Grant bonus spell slots at rank 2
  - Handle concentration bonus
- **Combat system**: Trigger Judgment Recovery on enemy death
- **Dual judgment system**: Enable at Empowered Judgment rank 3

## Notes
- All perks follow the established pattern from other classes
- Helper functions provide clean API for game systems to query perk effects
- Multi-rank perks use the standard `get_perk_rank()` function
- All perks properly associated with CLASS_INQUISITOR
