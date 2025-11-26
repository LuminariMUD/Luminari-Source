# Druid Nature's Warrior Tree: Tier 3 & 4 Perks Implementation

## Overview
This document details the implementation of Tier 3 and Tier 4 perks from the Nature's Warrior druid perk tree. These are the final 7 perks that complete the entire 15-perk tree.

## Implemented Perks

### Tier 3 Perks (Cost: 2 AP each)

#### 1. Wild Shape Enhancement III (ID: 608)
**Ranks:** 2  
**Prerequisite:** Wild Shape Enhancement II (2 ranks)  
**Benefits:**
- Rank 1: +1 attack, +1 damage
- Rank 2: +2 attack, +2 damage

**Implementation:**
- Modified `get_druid_wild_shape_attack_bonus()` in perks.c to include this perk
- Modified `get_druid_wild_shape_damage_bonus()` in perks.c to include this perk
- Bonuses apply in `compute_attack_bonus_full()` and `compute_damage_bonus()` in fight.c

#### 2. Natural Armor III (ID: 609)
**Ranks:** 2  
**Prerequisite:** Natural Armor II (2 ranks)  
**Benefits:**
- Rank 1: +1 AC
- Rank 2: +2 AC

**Implementation:**
- Modified `get_druid_natural_armor_bonus()` in perks.c to include this perk
- Bonus applies as BONUS_TYPE_NATURALARMOR in `compute_armor_class()` in fight.c

#### 3. Primal Instinct II (ID: 610)
**Ranks:** 2  
**Prerequisite:** Primal Instinct I (2 ranks)  
**Benefits:**
- Rank 1: +15 HP
- Rank 2: +30 HP

**Implementation:**
- Modified `get_druid_wild_shape_hp_bonus()` in perks.c to include this perk
- HP bonus applied through WILDSHAPE_AFFECTS array in `set_bonus_stats()` in act.other.c
- Used affect slot af[4] with BONUS_TYPE_RACIAL

#### 4. Mighty Wild Shape (ID: 611)
**Ranks:** 1  
**Prerequisite:** None  
**Benefits:**
- +4 Strength while wild shaped
- +4 Constitution while wild shaped

**Implementation:**
- Modified `wildshape_engine()` in act.other.c to add STR and CON bonuses
- Applied directly to ch->aff_abils before `set_bonus_stats()` call
- Only applies in mode 0 (main wild shape mode) for non-NPC characters

### Tier 4 Perks (Cost: 3 AP each)

#### 5. Elemental Wild Shape (ID: 612)
**Ranks:** 1  
**Prerequisite:** Mighty Wild Shape (1 rank)  
**Benefits:**
- +3 attack bonus when in elemental form
- +6 damage bonus when in elemental form
- +3 AC when in elemental form
- +50 HP when in elemental form

**Implementation:**
Created helper functions in perks.c:
- `is_druid_in_elemental_form()`: Checks if character is morphed as RACE_TYPE_ELEMENTAL and has the perk
- `get_druid_elemental_attack_bonus()`: Returns +3 if in elemental form
- `get_druid_elemental_damage_bonus()`: Returns +6 if in elemental form
- `get_druid_elemental_armor_bonus()`: Returns +3 if in elemental form
- `get_druid_elemental_hp_bonus()`: Returns +50 if in elemental form

Integrated into combat systems:
- Attack bonus: `compute_attack_bonus_full()` in fight.c (BONUS_TYPE_UNDEFINED)
- Damage bonus: `compute_damage_bonus()` in fight.c
- AC bonus: `compute_armor_class()` in fight.c (BONUS_TYPE_NATURALARMOR)
- HP bonus: `set_bonus_stats()` in act.other.c (added to hp_bonus calculation)

#### 6. Primal Avatar (ID: 613)
**Ranks:** 1  
**Prerequisite:** Elemental Wild Shape (1 rank)  
**Benefits:**
- +4 Dexterity while wild shaped
- One extra attack per round at maximum BAB

**Implementation:**
- DEX bonus: Modified `wildshape_engine()` in act.other.c to add +4 DEX
- Extra attack: Added check in fight.c combat loop (after haste check)
  - Created `has_druid_primal_avatar()` helper function in perks.c
  - Increments `bonus_mainhand_attacks` and `attacks_at_max_bab`
  - Attack only granted while wild shaped and not NPC

#### 7. Natural Fury (ID: 614)
**Ranks:** 1  
**Prerequisite:** Primal Avatar (1 rank)  
**Benefits:**
- Critical hits while wild shaped deal triple damage

**Implementation:**
- Modified `determine_critical_multiplier()` in fight.c
- Created `has_druid_natural_fury()` helper function in perks.c
- Sets crit_multi to at least 3 (x3) when wild shaped with this perk
- Uses MAX() to ensure it doesn't reduce weapon's natural multiplier

## Modified Files

### src/perks.c
**Lines 11130-11300:** Added/modified druid wild shape helper functions
- `get_druid_wild_shape_attack_bonus()`: Enhanced to include tier 3
- `get_druid_wild_shape_damage_bonus()`: Enhanced to include tier 3
- `get_druid_natural_armor_bonus()`: Enhanced to include tier 3
- `get_druid_wild_shape_hp_bonus()`: Enhanced to include tier 3
- `is_druid_in_elemental_form()`: New function for elemental form detection
- `get_druid_elemental_attack_bonus()`: New function (+3)
- `get_druid_elemental_damage_bonus()`: New function (+6)
- `get_druid_elemental_armor_bonus()`: New function (+3)
- `get_druid_elemental_hp_bonus()`: New function (+50)
- `has_druid_primal_avatar()`: New function for extra attack check
- `has_druid_natural_fury()`: New function for triple damage crit check

### src/perks.h
**Lines 300-322:** Added function declarations for all new helper functions

### src/act.other.c
**Line 71:** WILDSHAPE_AFFECTS changed from 4 to 5 (for HP affect)

**Lines 3628-3665:** Modified `set_bonus_stats()` function
- Calculate HP bonus from wild shape perks
- Include both base wild shape HP bonus and elemental HP bonus
- Apply through affect array af[4]

**Lines 4120-4185:** Modified `wildshape_engine()` function
- Added +4 STR and +4 CON for Mighty Wild Shape perk
- Added +4 DEX for Primal Avatar perk
- Both only apply in mode 0 (wild shape activation)
- Only apply to non-NPC characters

### src/fight.c
**Lines 833-845:** Modified `compute_armor_class()` function
- Added druid natural armor bonus (existing)
- Added elemental armor bonus (+3 in elemental form)

**Lines 6300-6315:** Modified `compute_damage_bonus()` function
- Added wild shape damage bonus (existing)
- Added elemental damage bonus (+6 in elemental form)

**Lines 6945-6955:** Modified `determine_critical_multiplier()` function
- Added Natural Fury check for triple damage crits
- Sets multiplier to at least 3 when wild shaped with perk

**Lines 9553-9575:** Modified `compute_attack_bonus_full()` function
- Added wild shape attack bonus (existing)
- Added elemental attack bonus (+3 in elemental form)

**Lines 12796-12807:** Modified combat loop (after haste)
- Added Primal Avatar extra attack check
- Grants one attack at maximum BAB when wild shaped

## Stacking Rules

### Attack Bonuses
- Wild Shape Enhancement I/II/III bonuses stack with each other
- Elemental Wild Shape bonus stacks with base wild shape bonuses
- All use BONUS_TYPE_UNDEFINED so they stack additively

### Damage Bonuses
- Same stacking as attack bonuses
- Base wild shape and elemental bonuses are calculated separately and added

### AC Bonuses
- Natural Armor I/II/III bonuses stack with each other
- Elemental Wild Shape AC bonus stacks with base natural armor bonuses
- All use BONUS_TYPE_NATURALARMOR

### HP Bonuses
- Primal Instinct I and II bonuses stack with each other
- Elemental Wild Shape HP bonus stacks with base HP bonuses
- All applied through single affect with BONUS_TYPE_RACIAL

### Stat Bonuses
- Mighty Wild Shape: +4 STR, +4 CON
- Primal Avatar: +4 DEX
- Applied directly to character stats during wild shape activation
- These are racial bonuses (wild shape changes race)

## Testing Recommendations

1. **Basic Wild Shape**
   - Test that stat bonuses (STR/CON/DEX) apply correctly
   - Verify HP bonus shows in character stats
   - Check that AC bonus applies to armor class

2. **Combat Testing**
   - Verify attack bonuses display in combat breakdown
   - Verify damage bonuses apply to damage rolls
   - Test critical hit multiplier with Natural Fury

3. **Elemental Form Testing**
   - Wild shape into an elemental form
   - Verify elemental bonuses only apply in elemental form
   - Check that both base and elemental bonuses stack

4. **Extra Attack Testing**
   - Verify Primal Avatar grants extra attack
   - Check that attack is at maximum BAB
   - Ensure it doesn't stack with multiple sources inappropriately

5. **Edge Cases**
   - Test with multiclass druids
   - Test with different elemental types
   - Test combination with other attack-granting abilities (haste, etc.)

## Completion Status

✅ All 15 perks in Nature's Warrior tree implemented  
✅ Tier 1 (4 perks): Wild Shape Enhancement I, Natural Armor I, Primal Instinct I, Natural Weapons  
✅ Tier 2 (4 perks): Wild Shape Enhancement II, Natural Armor II, Primal Instinct II, Savage Strikes  
✅ Tier 3 (4 perks): Wild Shape Enhancement III, Natural Armor III, Primal Instinct II, Mighty Wild Shape  
✅ Tier 4 (3 perks): Elemental Wild Shape, Primal Avatar, Natural Fury  
✅ Code compiles without errors or warnings  

## Next Steps

The Nature's Warrior tree is complete. Remaining work:
1. Implement Elemental Mastery tree (12 perks)
2. Implement Primal Restoration tree (12 perks)
3. In-game testing and balance adjustments
4. Player documentation and help files
