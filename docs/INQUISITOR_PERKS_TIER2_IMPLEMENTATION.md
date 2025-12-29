# Inquisitor Perks Implementation - Tier 2 Summary

## Implementation Date
December 28, 2025

## Overview
Implemented **Tier 2 perks** from the **Judgment & Spellcasting** tree for Inquisitors with full game mechanic integration, as defined in the refined perks document.

## Tier 2 Perks Implemented

### 1. Enhanced Bane (4 ranks, 2 points each)
**Mechanics Integrated:**
- **Damage Bonus**: Added to `compute_damage_bonus()` in [fight.c](fight.c#L6500) - gains +1 damage per rank against judged target
- **Attack Bonus**: Added to `get_perk_weapon_tohit_bonus()` in [perks.c](perks.c#L13092) - gains +1 to-hit per 2 ranks (max +2 at rank 4)
- **AOE Capability**: Rank 4 extends bane to all creatures of same type as current target (via `has_inquisitor_enhanced_bane_aoe()`)
- **Helper Functions**:
  - `get_inquisitor_enhanced_bane_damage()` - Returns +1-4 damage
  - `get_inquisitor_enhanced_bane_attack()` - Returns +1-2 to-hit
  - `has_inquisitor_enhanced_bane_aoe()` - Boolean for rank 4 AOE

### 2. Divine Resilience (1 rank, 2 points)
**Mechanics Integrated:**
- **Temporary HP Trigger**: When judgment is toggled ON in [act.offensive.c](act.offensive.c#L12945)
- **Formula**: Inquisitor Level + Wisdom Modifier = temporary HP
- **Duration**: Persists while judgment is active; reapplies if judgment is toggled off/on
- **Implementation**: Uses `AFFECT_DIVINE_RESILIENCE` (1305) affect with `APPLY_HIT` location
- **Helper Function**: `has_inquisitor_divine_resilience()` - Boolean check

### 3. Spell Penetration (3 ranks, 2 points each)
**Mechanics Integrated:**
- **Challenge Roll Bonus**: Added to `mag_resistance()` in [magic.c](magic.c#L288) - gains +1 per rank to spell resistance check
- **SR Reduction (Rank 3)**: At rank 3, ignores first 5 points of enemy spell resistance
- **Implementation**: 
  - Bonus applied via `get_inquisitor_spell_penetration()` to challenge roll
  - SR reduction via `has_inquisitor_spell_penetration_ignore()` check in resistance calculation
- **Helper Functions**:
  - `get_inquisitor_spell_penetration()` - Returns +1-3 to spell penetration
  - `has_inquisitor_spell_penetration_ignore()` - Boolean for rank 3 special ability

### 4. Persistent Judgment (1 rank, 2 points)
**Mechanics Already Integrated:**
- **Effect**: When judgment is toggled OFF, the judgment bonus persists for 5 rounds as a morale bonus
- **Code Location**: [act.offensive.c](act.offensive.c#L12897) - Full implementation already in place
- **Cooldown**: 20 round cooldown per judgment type (prevents stacking)
- **Application**: Bonus applies to appropriate ability scores based on judgment type
- **Helper Function**: `has_inquisitor_persistent_judgment()` - Boolean check

## Files Modified

### 1. [src/structs.h](src/structs.h)
- Added Tier 2 perk ID constants (already present):
  - `PERK_INQUISITOR_ENHANCED_BANE` (1448)
  - `PERK_INQUISITOR_DIVINE_RESILIENCE` (1449)
  - `PERK_INQUISITOR_SPELL_PENETRATION` (1450)
  - `PERK_INQUISITOR_PERSISTENT_JUDGMENT` (1451)

### 2. [src/spells.h](src/spells.h)
- Added new affect ID: `AFFECT_DIVINE_RESILIENCE` (1305)

### 3. [src/perks.h](src/perks.h)
- All helper function declarations already present

### 4. [src/perks.c](src/perks.c)
- **Lines 849-1000**: Tier 2 perk definitions in `define_inquisitor_perks()`
- **Lines 1069-1120**: Helper functions for Tier 2 perks
- **Lines 13092-13100**: Enhanced Bane integrated into `get_perk_weapon_tohit_bonus()`

### 5. [src/fight.c](src/fight.c)
- **Lines 6500-6514**: Enhanced Bane damage bonus in `compute_damage_bonus()`

### 6. [src/act.offensive.c](act.offensive.c)
- **Lines 12945-12962**: Divine Resilience temporary HP on judgment activation
- **Lines 12897-12930**: Persistent Judgment already fully integrated

### 7. [src/magic.c](magic.c)
- **Lines 288-297**: Spell Penetration bonus in `mag_resistance()`
- Includes special handling for rank 3 SR reduction

## Game Systems Integration Points

### Combat System
- **Enhanced Bane**: Integrated into weapon damage and to-hit calculations
- **Divine Resilience**: Grants protective HP when leveraging judgment abilities
- **Persistent Judgment**: Already working - provides continuing bonuses after judgment toggle

### Spellcasting System
- **Spell Penetration**: Integrated into spell resistance checks
- **Rank 3 Special**: Ignores first 5 SR points for powerful spell penetration

### Judgment System
- **Divine Resilience**: Triggers on judgment activation in command
- **Persistent Judgment**: Triggers on judgment deactivation in command

## Compilation Status
✅ **Successfully compiled** with no errors or warnings (December 28, 2025)

## Testing Recommendations

1. **Enhanced Bane Testing**:
   - Equip weapon and activate judgment
   - Verify damage increases by +1 per rank
   - Verify to-hit increases by +1 per 2 ranks
   - At rank 4, test that bane applies to all creatures of same type

2. **Divine Resilience Testing**:
   - Toggle a judgment ON
   - Verify character gains temporary HP = level + WIS modifier
   - Toggle judgment OFF then ON - should reapply HP

3. **Spell Penetration Testing**:
   - Cast spells against enemies with spell resistance
   - Verify +1 per rank to penetration
   - At rank 3, verify 5 SR is ignored

4. **Persistent Judgment Testing**:
   - Toggle judgment ON then OFF
   - Verify judgment bonus persists for 5 rounds
   - Test 20 round cooldown prevents reapplication

## Next Steps

1. Implement Tier 3 perks (Greater Judgment, Spell Metamastery, Righteous Strike, Versatile Judgment)
2. Implement Tier 4 perks (Judgment Mastery, Divine Spellstrike, Inexorable Judgment, Supreme Spellcasting)
3. Implement other three perk trees:
   - Hunter's Arsenal (16 perks across 4 tiers)
   - Investigation & Perception (16 perks across 4 tiers)
   - Adaptable Tactics (16 perks across 4 tiers)

## Notes

- All Tier 2 perks follow established patterns from Tier 1
- Divine Resilience uses affect-based temporary HP (consistent with other temporary bonuses)
- Persistent Judgment was already implemented in previous update
- Helper functions provide clean API for game systems to query perk effects
- All perks properly associated with CLASS_INQUISITOR
