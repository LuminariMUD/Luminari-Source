# Druid Season's Herald Tree: Tier 1 & 2 Perks Implementation

## Overview
This document details the implementation of Tier 1 and Tier 2 perks from the Season's Herald druid perk tree. These perks focus on enhancing the druid's spellcasting abilities with bonuses to spell damage, save DCs, and spell slots.

## Implemented Perks

### Tier 1 Perks (Cost: 1 AP each)

#### 1. Spell Power I (ID: 615)
**Ranks:** 5  
**Prerequisite:** None  
**Benefits:**
- Rank 1-5: +1 damage per die per rank

**Total Possible:** +5 damage per die at max rank

**Implementation:**
- Created `get_druid_spell_power_bonus()` helper function in perks.c
- Integrated into `mag_damage()` function in magic.c
- Bonus applies to all druid spells that deal damage with dice
- Formula: `dam += num_dice * spell_power_bonus`

#### 2. Nature's Focus I (ID: 616)
**Ranks:** 3  
**Prerequisite:** None  
**Benefits:**
- Rank 1-3: +1 spell save DC per rank

**Total Possible:** +3 spell save DC at max rank

**Implementation:**
- Created `get_druid_spell_dc_bonus()` helper function in perks.c
- Integrated into `mag_damage()` function in magic.c via `dc_mod` variable
- Bonus applies to all druid spells that allow saving throws
- Makes spells harder for enemies to resist

#### 3. Elemental Manipulation I (ID: 617)
**Ranks:** 3  
**Prerequisite:** None  
**Benefits:**
- Rank 1-3: +2d6 damage per rank for fire, cold, and lightning spells

**Total Possible:** +6d6 elemental damage at max rank

**Implementation:**
- Created `get_druid_elemental_damage_dice()` helper function in perks.c
- Integrated into `mag_damage()` function in magic.c
- Only applies to spells with DAM_FIRE, DAM_COLD, or DAM_ELECTRIC damage types
- Adds extra dice rolls to elemental spell damage
- Formula: `dam += dice(elemental_dice, 6)`

#### 4. Efficient Caster (ID: 618)
**Ranks:** 1  
**Prerequisite:** None  
**Benefits:**
- +1 spell slot (applied to 1st circle)

**Implementation:**
- Created `get_druid_bonus_spell_slots()` helper function in perks.c
- Integrated into `compute_slots_by_circle()` function in spell_prep.c
- Grants one additional 1st circle spell slot
- Allows druids to prepare and cast one more spell per rest period

### Tier 2 Perks (Cost: 2 AP each)

#### 5. Spell Power II (ID: 619)
**Ranks:** 3  
**Prerequisite:** Spell Power I (max rank)  
**Benefits:**
- Rank 1-3: +2 damage per die per rank

**Total Possible:** +6 damage per die at max rank  
**Combined with Tier 1:** +11 damage per die maximum

**Implementation:**
- Integrated into existing `get_druid_spell_power_bonus()` helper function
- Stacks with Spell Power I bonuses
- Same integration point as Spell Power I in magic.c

#### 6. Nature's Focus II (ID: 620)
**Ranks:** 2  
**Prerequisite:** Nature's Focus I (max rank)  
**Benefits:**
- Rank 1-2: +1 spell save DC per rank

**Total Possible:** +2 spell save DC at max rank  
**Combined with Tier 1:** +5 spell save DC maximum

**Implementation:**
- Integrated into existing `get_druid_spell_dc_bonus()` helper function
- Stacks with Nature's Focus I bonuses
- Same integration point as Nature's Focus I in magic.c

#### 7. Elemental Manipulation II (ID: 621)
**Ranks:** 2  
**Prerequisite:** Elemental Manipulation I (max rank)  
**Benefits:**
- Rank 1-2: +3d6 damage per rank for fire, cold, and lightning spells

**Total Possible:** +6d6 elemental damage at max rank  
**Combined with Tier 1:** +12d6 elemental damage maximum

**Implementation:**
- Integrated into existing `get_druid_elemental_damage_dice()` helper function
- Stacks with Elemental Manipulation I bonuses
- Same integration point as Elemental Manipulation I in magic.c

#### 8. Spell Critical (ID: 622)
**Ranks:** 1  
**Prerequisite:** Spell Power I (3 ranks)  
**Benefits:**
- 5% chance for druid spells to critical hit for double damage

**Implementation:**
- Created `check_druid_spell_critical()` helper function in perks.c
- Integrated into `mag_damage()` function in magic.c
- Uses `rand_number(1, 100)` to check for 5% chance
- Doubles spell damage when critical occurs
- Displays "\tY[Spell Critical!]\tn" message to caster

## Modified Files

### src/perks.c
**Lines 11300-11386:** Added Season's Herald druid perk helper functions
- `get_druid_spell_power_bonus()`: Returns total damage per die bonus
- `get_druid_spell_dc_bonus()`: Returns total spell save DC bonus
- `get_druid_elemental_damage_dice()`: Returns total bonus elemental damage dice
- `check_druid_spell_critical()`: Checks for 5% spell critical chance
- `get_druid_bonus_spell_slots()`: Returns bonus spell slots from Efficient Caster

### src/perks.h
**Lines 318-322:** Added function declarations for Season's Herald helper functions

### src/magic.c
**Lines 2846-2885:** Modified `mag_damage()` function to add druid spell bonuses
- Added spell power bonus (damage per die)
- Added elemental manipulation bonus (extra dice for fire/cold/lightning)
- Added spell critical check (5% chance for double damage)

**Lines 2962-2973:** Modified `mag_damage()` function to add druid spell DC bonus
- Added Nature's Focus DC bonus via `dc_mod` variable
- Applies before saving throw checks

### src/spell_prep.c
**Lines 2725-2732:** Modified `compute_slots_by_circle()` function
- Added Efficient Caster bonus spell slot for druids
- Applied to 1st circle spells

## Damage Stacking Examples

### Spell Power (Damage per Die)
- Tier 1 only (5 ranks): +5 per die
- Tier 2 only (3 ranks): +6 per die
- **Maximum (Tier 1 + 2):** +11 damage per die

Example with Flame Strike (8d6):
- Base: 8d6 (28 average)
- With max Spell Power: 8d6 + 88 (116 average)

### Elemental Manipulation (Extra Dice)
- Tier 1 only (3 ranks): +6d6
- Tier 2 only (2 ranks): +6d6
- **Maximum (Tier 1 + 2):** +12d6

Example with Call Lightning (3d6):
- Base: 3d6 (10.5 average)
- With max Elemental Manipulation: 3d6 + 12d6 (52.5 average)
- With both maxed: 15d6 + 165 (217.5 average with Spell Power)

### Combined Effects
Example with Ice Storm (3d6 cold damage per round):
- Base: 3d6 (10.5 average)
- With Spell Power I+II (11 per die): 3d6 + 33 (43.5 average)
- With Elemental Manipulation I+II (+12d6): 15d6 + 33 (85.5 average)
- **With 5% Spell Critical:** 171 damage on critical!

### Nature's Focus (Spell Save DC)
- Tier 1 only (3 ranks): +3 DC
- Tier 2 only (2 ranks): +2 DC
- **Maximum (Tier 1 + 2):** +5 spell save DC

Makes spells significantly harder to resist, especially powerful for:
- Charm/dominate spells
- Hold/paralysis spells
- Damage spells with save for half

## Integration Points

All druid Season's Herald perks are checked in the `mag_damage()` function which is the central damage-dealing spell handler. This ensures consistent application across all druid damage spells.

### Damage Calculation Order:
1. Calculate base damage from spell dice (num_dice × size_dice)
2. Apply metamagic (maximize, empower)
3. Apply class-specific bonuses (wizard, cleric, druid)
4. **Apply druid spell power bonus** (damage per die)
5. **Apply druid elemental manipulation bonus** (extra dice)
6. Apply saving throw reductions
7. **Check for druid spell critical** (double damage on 5% chance)

### DC Calculation Order:
1. Calculate base DC from spell level and caster stats
2. Apply racial bonuses
3. **Apply druid Nature's Focus DC bonus**
4. Apply target's saving throw bonuses
5. Resolve saving throw

### Spell Slot Calculation:
1. Check class level and stat bonuses
2. Apply base spell slots from tables
3. **Apply druid Efficient Caster bonus**
4. Apply equipment/affect bonuses
5. Return total available slots

## Testing Recommendations

1. **Spell Power Testing**
   - Cast a druid spell that deals damage (e.g., Flame Strike)
   - Verify damage increases by expected amount per die
   - Test with spells of different damage dice (d4, d6, d8, d10)

2. **Elemental Manipulation Testing**
   - Cast fire spell (Flame Strike, Fireball)
   - Cast cold spell (Ice Storm, Cone of Cold)
   - Cast lightning spell (Call Lightning, Lightning Bolt)
   - Verify extra d6 dice are added
   - Verify non-elemental spells are NOT affected

3. **Nature's Focus Testing**
   - Cast spell with saving throw against various level enemies
   - Record success/failure rates
   - Compare with and without perk
   - Should see noticeably fewer successful saves

4. **Efficient Caster Testing**
   - Check spell preparation screen
   - Verify +1 spell slot appears in 1st circle
   - Verify can prepare one extra spell
   - Rest and verify slot refreshes

5. **Spell Critical Testing**
   - Cast many spells and watch for critical messages
   - Statistically should see ~1 critical per 20 spells
   - Verify damage doubles on critical
   - Check that critical stacks with other bonuses

6. **Combined Effects Testing**
   - Max all Season's Herald perks
   - Cast elemental damage spell
   - Should see all bonuses applied simultaneously
   - Verify spectacular damage output!

## Known Behaviors

1. **Spell Power applies to ALL dice:** If a spell does 8d6 base damage, +5 spell power adds 40 damage (8 × 5), not just 5.

2. **Elemental Manipulation only applies to fire/cold/lightning:** Acid, force, sonic, and other damage types are not affected.

3. **Spell Critical applies AFTER all other bonuses:** The critical multiplier applies to the final damage including all bonuses, making it very powerful.

4. **Efficient Caster applies to 1st circle only:** This matches the perk description of "one additional spell per rest period" by granting it at the lowest spell circle.

5. **All bonuses stack with each other:** Spell Power + Elemental Manipulation + Spell Critical can create extremely high damage.

## Completion Status

✅ All 8 perks in Season's Herald Tier 1 and 2 implemented  
✅ Tier 1 (4 perks): Spell Power I, Nature's Focus I, Elemental Manipulation I, Efficient Caster  
✅ Tier 2 (4 perks): Spell Power II, Nature's Focus II, Elemental Manipulation II, Spell Critical  
✅ Code compiles without errors or warnings  
✅ All helper functions created and integrated  
✅ Perks stack properly with their respective tiers  

## Next Steps

The Season's Herald Tier 1 and 2 are complete. Remaining work:
1. Implement Season's Herald Tier 3 and 4 perks
2. Implement Nature's Protector tree (12 perks)
3. In-game testing and balance adjustments
4. Player documentation and help files

## Balance Notes

With all Tier 1 and 2 perks maxed, a druid's elemental damage spells become extremely powerful:
- +11 damage per die (Spell Power I+II)
- +12d6 extra damage (Elemental Manipulation I+II)
- +5 spell DC (Nature's Focus I+II)
- 5% chance for double damage (Spell Critical)
- +1 spell slot (Efficient Caster)

This represents a significant investment in Action Points (requires 23 AP total) and should provide appropriate power increase for specialized spellcasting druids.
