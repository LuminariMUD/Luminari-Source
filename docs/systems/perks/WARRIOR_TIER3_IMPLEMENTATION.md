# Warrior Tier III Perks Implementation

## Summary
Successfully implemented 4 Tier III perks for the Warrior class from Tree 1: Weapon Specialist. These are advanced perks requiring significant investment in prerequisite perks.

## Implemented Perks

### 9. Weapon Focus III (ID: 9)
- **Cost**: 3 points
- **Max Rank**: 1
- **Prerequisite**: Weapon Focus II (rank 1)
- **Effect**: Additional +1 to hit with all weapons (+3 total with Focus I & II)
- **Implementation**: Uses PERK_EFFECT_WEAPON_TOHIT, handled automatically by existing perk system
- **Total Progression**: 
  - Focus I (1pt) → Focus II (2pt) → Focus III (3pt) = 6 points total for +3 to hit

### 10. Weapon Specialization II (ID: 10)
- **Cost**: 3 points per rank
- **Max Rank**: 3
- **Prerequisite**: Weapon Specialization I at max rank (rank 3)
- **Effect**: Additional +1 damage per rank with all weapons (max +3 additional damage)
- **Implementation**: Uses PERK_EFFECT_WEAPON_DAMAGE, stacking damage bonus
- **Total Progression**:
  - Specialization I Rank 1-3 (6pts) → Specialization II Rank 1-3 (9pts) = 15 points total for +6 damage

### 11. Great Cleave (ID: 11)
- **Cost**: 3 points
- **Max Rank**: 1
- **Prerequisite**: Cleaving Strike (rank 1)
- **Effect**: Cleaving Strike works on any kill, not just first (unlimited cleave attacks)
- **Implementation**: 
  - Uses PERK_EFFECT_SPECIAL
  - Modified `handle_cleave()` function in fight.c (~line 12741)
  - Added perk check alongside FEAT_GREAT_CLEAVE
  - Enables second cleave attack after first cleave
- **Total Progression**:
  - Power Attack Training (1pt) → Cleaving Strike (2pt) → Great Cleave (3pt) = 6 points total

### 12. Devastating Critical (ID: 12)
- **Cost**: 4 points
- **Max Rank**: 1
- **Prerequisite**: Improved Critical Threat (rank 1)
- **Effect**: Critical hits deal +1d6 additional damage
- **Implementation**:
  - Uses PERK_EFFECT_SPECIAL
  - Custom handling in critical damage section of fight.c (~line 7193)
  - Adds dice(1, 6) damage on confirmed critical hits
  - Works independently of devastating critical feat (simpler, no weapon family requirement)
- **Total Progression**:
  - Critical Awareness I (1pt) → Improved Critical Threat (2pt) → Devastating Critical (4pt) = 7 points total

## File Changes

### src/structs.h
- Added 4 new Tier III perk IDs (9-12)
- Shifted existing older perks down by 4 positions (WEAPON_SPEC_1 now 13, etc.)

```c
/* Tier III Perks */
#define PERK_FIGHTER_WEAPON_FOCUS_3 9
#define PERK_FIGHTER_WEAPON_SPECIALIZATION_2 10
#define PERK_FIGHTER_GREAT_CLEAVE 11
#define PERK_FIGHTER_DEVASTATING_CRITICAL 12
```

### src/perks.c
- Added 4 new perk definitions in `define_fighter_perks()` function
- Each perk includes proper prerequisites and higher costs
- Weapon Specialization II requires max rank (3) of Specialization I

### src/fight.c
- **Great Cleave** handling (~line 12741):
  - Modified `handle_cleave()` function
  - Added `has_perk(ch, PERK_FIGHTER_GREAT_CLEAVE)` check
  - Now triggers unlimited cleave with either feat or perk
  
- **Devastating Critical** handling (~line 7193):
  - Added perk check after savage attacks
  - Adds 1d6 damage on critical hits
  - Applied before critical multiplier for consistency

## Perk Progression Trees

### Complete To-Hit Progression (Tier I-III)
1. Weapon Focus I: +1 to hit, 1 point
2. Weapon Focus II: +1 to hit, 2 points [Requires Focus I]
3. Weapon Focus III: +1 to hit, 3 points [Requires Focus II]
   - **Total**: +3 to hit for 6 points

### Complete Damage Progression (Tier I-III)
1. Weapon Focus I: +1 to hit, 1 point (prerequisite)
2. Weapon Specialization I Rank 1-3: +3 damage, 6 points [Requires Focus I]
3. Weapon Specialization II Rank 1-3: +3 damage, 9 points [Requires Spec I max rank]
   - **Total**: +1 to hit, +6 damage for 16 points

### Complete Critical Progression (Tier I-III)
1. Critical Awareness I: +1 crit confirm, 1 point
2. Critical Awareness II: +1 crit confirm, 2 points [Requires Awareness I]
3. Improved Critical Threat: +1 threat range, 2 points [Requires Awareness I]
4. Devastating Critical: +1d6 crit damage, 4 points [Requires Improved Threat]
   - **Total**: +2 crit confirmation, +1 threat range, +1d6 crit damage for 9 points

### Complete Cleave Progression (Tier I-III)
1. Power Attack Training: improved power attack, 1 point
2. Cleaving Strike: extra attack on kill, 2 points [Requires Power Attack Training]
3. Great Cleave: unlimited cleave, 3 points [Requires Cleaving Strike]
   - **Total**: Better power attack + unlimited cleave for 6 points

## Cost Analysis

**Total Investment for Tier I + II + III Offense Tree:**

**To-Hit Path:**
- Weapon Focus I: 1 point
- Weapon Focus II: 2 points
- Weapon Focus III: 3 points
- Subtotal: 6 points for +3 to hit

**Damage Path:**
- Weapon Focus I: 1 point (prerequisite)
- Weapon Specialization I x3: 6 points
- Weapon Specialization II x3: 9 points
- Subtotal: 16 points for +1 to hit, +6 damage

**Critical Path:**
- Critical Awareness I: 1 point
- Critical Awareness II: 2 points
- Improved Critical Threat: 2 points
- Devastating Critical: 4 points
- Subtotal: 9 points for +2 crit confirm, +1 threat range, +1d6 crit damage

**Cleave Path:**
- Power Attack Training: 1 point
- Cleaving Strike: 2 points
- Great Cleave: 3 points
- Subtotal: 6 points for improved power attack + unlimited cleave

**Maximum Investment (all paths):**
Assuming no overlap except shared prerequisites: ~37 points

This provides:
- +3 to hit (from Focus tree)
- +6 damage (from Specialization tree)
- +2 to crit confirmation
- +1 to crit threat range
- +1d6 crit damage
- Improved power attack (-1 penalty instead of -2, +2 damage)
- Unlimited cleave on kills

## Testing Recommendations

1. **Weapon Focus III**:
   - Purchase entire Focus I → II → III chain
   - Verify cumulative +3 to hit in combat
   - Confirm prerequisite enforcement

2. **Weapon Specialization II**:
   - Must have Weapon Focus I first
   - Purchase all 3 ranks of Specialization I
   - Then purchase ranks of Specialization II
   - Verify total +6 damage (3 from Spec I, 3 from Spec II)
   - Test that cannot purchase Spec II without maxing Spec I

3. **Great Cleave**:
   - Purchase full chain: Power Attack Training → Cleaving Strike → Great Cleave
   - Kill multiple enemies in same room
   - Verify unlimited cleave attacks on each kill
   - Should work similar to FEAT_GREAT_CLEAVE

4. **Devastating Critical**:
   - Purchase chain: Critical Awareness I → Improved Critical Threat → Devastating Critical
   - Make attacks that confirm criticals
   - Verify +1d6 extra damage on critical hits (1-6 additional damage)
   - Check combat log for extra damage dice

## Integration Notes

- All perks integrate seamlessly with existing systems
- Weapon Focus III and Weapon Specialization II work through generic handlers
- Great Cleave and Devastating Critical use custom code in fight.c
- Prerequisites properly enforce progression paths
- Effects stack with feats and other bonuses
- No conflicts with existing devastating critical feat system (perk is simpler, no weapon family requirement)

## Tier Comparison

**Tier I (1 point each):**
- Entry-level perks
- No prerequisites
- Basic bonuses (+1 effects)

**Tier II (2 points each):**
- Intermediate perks
- Require Tier I prerequisites
- Enhanced bonuses (+1 additional effects, multi-rank)

**Tier III (3-4 points each):**
- Advanced perks
- Require Tier II prerequisites
- Powerful effects (+1 more, unlimited mechanics, dice damage)
- Higher costs reflect power level

## Future Work

From WARRIOR_PERKS.md Tree 1 (Weapon Specialist):

**Tier IV Capstones** (Not yet implemented):
- Perk 13: Master of Arms (5 points) - +1 attack per round
  - Requires: Weapon Focus III, Weapon Specialization II (max)
- Perk 14: Perfect Critical (5 points) - Auto-confirm crits, +1 multiplier
  - Requires: Devastating Critical, Critical Awareness II

**Tree 2: Defender** (Not yet implemented):
- 10+ defensive perks focused on AC, HP, saves, damage reduction

**Tree 3: Tactician** (Not yet implemented):
- 8+ tactical perks for battlefield control and group support

Total Tree 1 will require 45 points across all 14 perks when Tier IV is complete.

## Compilation Status

✅ Successfully compiled with no errors or warnings
✅ All perk definitions complete with proper prerequisites
✅ Effect handlers implemented in fight.c
✅ Prerequisite chains enforced (max rank requirements working)
✅ Ready for in-game testing

## Design Notes

The Tier III perks represent significant character investment and provide powerful benefits:

1. **Weapon Focus III** completes the to-hit progression for reliable combat
2. **Weapon Specialization II** doubles the damage investment for devastating strikes
3. **Great Cleave** transforms single kills into multi-target carnage
4. **Devastating Critical** makes critical hits truly devastating with bonus dice

These perks require 16-37 points total investment depending on path chosen, representing roughly 5-12 levels worth of perk points (at 3 points per level). This creates meaningful long-term character progression goals.
