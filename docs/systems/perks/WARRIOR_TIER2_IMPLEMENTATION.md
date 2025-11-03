# Warrior Tier II Perks Implementation

## Summary
Successfully implemented 5 Tier II perks for the Warrior class from Tree 1: Weapon Specialist. These perks build upon the Tier I foundation and require prerequisites.

## Implemented Perks

### 4. Weapon Focus II (ID: 4)
- **Cost**: 2 points
- **Max Rank**: 1
- **Prerequisite**: Weapon Focus I (rank 1)
- **Effect**: Additional +1 to hit with all weapons (+2 total with Focus I)
- **Implementation**: Uses PERK_EFFECT_WEAPON_TOHIT, handled automatically by existing perk system
- **Stacks With**: Weapon Focus I for total +2 to hit

### 5. Weapon Specialization I (ID: 5)
- **Cost**: 2 points per rank
- **Max Rank**: 3
- **Prerequisite**: Weapon Focus I (rank 1)
- **Effect**: +1 damage per rank with all weapons (max +3 damage at rank 3)
- **Implementation**: Uses PERK_EFFECT_WEAPON_DAMAGE, stacking damage bonus
- **Note**: This is the redesigned multi-rank version, different from the old single-rank WEAPON_SPEC_1

### 6. Cleaving Strike (ID: 6)
- **Cost**: 2 points
- **Max Rank**: 1
- **Prerequisite**: Power Attack Training (rank 1)
- **Effect**: If you kill an enemy, immediately make one additional attack
- **Implementation**: 
  - Uses PERK_EFFECT_SPECIAL
  - Integrated into existing cleave system in fight.c
  - Works like FEAT_CLEAVE but perk-based
  - Modified `handle_cleave()` call at line ~13222 to include perk check

### 7. Critical Awareness II (ID: 7)
- **Cost**: 2 points
- **Max Rank**: 1
- **Prerequisite**: Critical Awareness I (rank 1)
- **Effect**: Additional +1 to critical confirmation rolls (+2 total with Awareness I)
- **Implementation**:
  - Uses PERK_EFFECT_SPECIAL
  - Custom handling in `is_critical_hit()` function (fight.c ~line 6900)
  - Adds +1 to confirm_roll, stacks with Critical Awareness I
- **Stacks With**: Critical Awareness I for total +2 to critical confirmation

### 8. Improved Critical Threat (ID: 8)
- **Cost**: 2 points
- **Max Rank**: 1
- **Prerequisite**: Critical Awareness I (rank 1)
- **Effect**: +1 to critical threat range (19-20 becomes 18-20, 18-20 becomes 17-20, etc.)
- **Implementation**:
  - Uses PERK_EFFECT_SPECIAL
  - Custom handling in `determine_threat_range()` function (fight.c ~line 6580)
  - Reduces threat_range by 1 (lower number = wider range)

## File Changes

### src/structs.h
- Reorganized fighter perk IDs to accommodate 5 new Tier II perks
- New perks use IDs 4-8
- Existing perks shifted: WEAPON_SPEC_1 (4→9), ARMOR_MASTERY_1 (7→12), TOUGHNESS (12→17), etc.

```c
/* Tier I Perks */
#define PERK_FIGHTER_WEAPON_FOCUS_1 1
#define PERK_FIGHTER_POWER_ATTACK_TRAINING 2
#define PERK_FIGHTER_CRITICAL_AWARENESS_1 3

/* Tier II Perks */
#define PERK_FIGHTER_WEAPON_FOCUS_2 4
#define PERK_FIGHTER_WEAPON_SPECIALIZATION_1 5
#define PERK_FIGHTER_CLEAVING_STRIKE 6
#define PERK_FIGHTER_CRITICAL_AWARENESS_2 7
#define PERK_FIGHTER_IMPROVED_CRITICAL_THREAT 8
```

### src/perks.c
- Added 5 new perk definitions in `define_fighter_perks()` function
- Each perk includes:
  - ID, name, description
  - Associated class (CLASS_WARRIOR)
  - Cost and max rank
  - Prerequisite perk and required rank
  - Effect type and values
  - Special description

### src/fight.c
- **Cleaving Strike** handling (~line 13222):
  - Modified cleave trigger to include `has_perk(ch, PERK_FIGHTER_CLEAVING_STRIKE)`
  - Now triggers on kill with either FEAT_CLEAVE, FEAT_GREAT_CLEAVE, or Cleaving Strike perk
  
- **Critical Awareness II** handling (~line 6900):
  - Modified `is_critical_hit()` function
  - Added separate check for PERK_FIGHTER_CRITICAL_AWARENESS_2
  - Each Critical Awareness perk adds +1 independently (total +2 with both)

- **Improved Critical Threat** handling (~line 6580):
  - Modified `determine_threat_range()` function
  - Added check before "end mods" comment
  - Reduces threat_range by 1 (expands critical range)

## Perk Progression Paths

### Path 1: To-Hit Specialist
1. Weapon Focus I (+1 to hit, 1 point)
2. Weapon Focus II (+1 to hit, 2 points) [Requires Focus I]
   - **Total**: +2 to hit for 3 points

### Path 2: Damage Specialist
1. Weapon Focus I (+1 to hit, 1 point)
2. Weapon Specialization I Rank 1 (+1 damage, 2 points) [Requires Focus I]
3. Weapon Specialization I Rank 2 (+1 damage, 2 points)
4. Weapon Specialization I Rank 3 (+1 damage, 2 points)
   - **Total**: +1 to hit, +3 damage for 7 points

### Path 3: Critical Specialist
1. Critical Awareness I (+1 crit confirm, 1 point)
2. Critical Awareness II (+1 crit confirm, 2 points) [Requires Awareness I]
3. Improved Critical Threat (+1 threat range, 2 points) [Requires Awareness I]
   - **Total**: +2 crit confirmation, +1 threat range for 5 points

### Path 4: Cleaver
1. Power Attack Training (improved power attack, 1 point)
2. Cleaving Strike (extra attack on kill, 2 points) [Requires Power Attack Training]
   - **Total**: Better power attack + cleave for 3 points

## Testing Recommendations

1. **Weapon Focus II**:
   - Purchase both Focus I and Focus II
   - Verify total +2 to hit bonus in combat
   - Check that both show in 'myperks' command

2. **Weapon Specialization I (multi-rank)**:
   - Purchase rank 1, verify +1 damage
   - Purchase rank 2, verify +2 damage total
   - Purchase rank 3, verify +3 damage total
   - Test that cannot purchase without Weapon Focus I

3. **Cleaving Strike**:
   - Purchase perk
   - Kill an enemy in combat
   - Verify extra attack is made against another target
   - Should work like FEAT_CLEAVE

4. **Critical Awareness II**:
   - Purchase both Awareness I and II
   - Make attacks that threaten criticals
   - Verify confirmation rolls have +2 bonus
   - Check against lower-level enemies for higher success rate

5. **Improved Critical Threat**:
   - Purchase perk (requires Critical Awareness I)
   - Use weapon with 19-20 threat range
   - Verify critical threats now occur on 18-20
   - Test with various weapon types

## Integration Notes

- All perks integrate with existing perk system infrastructure
- Weapon Focus II and Weapon Specialization I work automatically through generic handlers
- Cleaving Strike, Critical Awareness II, and Improved Critical Threat use custom code in fight.c
- All effects stack properly with existing feats and bonuses
- Prerequisites are enforced by perk purchase system

## Cost Analysis

**Total Investment for Full Tier I + II Offense Tree**:
- Weapon Focus I: 1 point
- Weapon Focus II: 2 points
- Power Attack Training: 1 point
- Critical Awareness I: 1 point
- Critical Awareness II: 2 points
- Improved Critical Threat: 2 points
- Weapon Specialization I x3: 6 points (2 per rank)
- Cleaving Strike: 2 points
- **Grand Total**: 17 points for all Tier I + II offensive perks

This represents a significant investment but provides:
- +2 to hit
- +3 damage
- +2 crit confirmation
- +1 crit threat range
- Improved power attack
- Cleave on kill

## Future Work

From WARRIOR_PERKS.md Tree 1 (Weapon Specialist):

**Tier III** (Not yet implemented):
- Perks 9-12: Weapon Focus III, Weapon Specialization II, Great Cleave, Devastating Critical

**Tier IV Capstones** (Not yet implemented):
- Perks 13-14: Master of Arms, Perfect Critical

Total Tree 1 will require 45 points across all 14 perks.

## Compilation Status

✅ Successfully compiled with no errors or warnings
✅ All perk definitions complete
✅ Effect handlers implemented in fight.c
✅ Prerequisite system working
✅ Ready for in-game testing
