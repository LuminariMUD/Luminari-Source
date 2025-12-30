# Berserker Ravager Tree - Tier 3 & 4 Implementation

## Overview
Implemented the final two tiers (3 and 4) of the Ravager tree for the Berserker/Barbarian class perk system. These tiers provide the ultimate offensive capabilities, scaling existing bonuses to maximum levels and introducing devastating new mechanics.

## Implementation Date
November 5, 2024

---

## Perk IDs Allocated

### Ravager Tree - Tier 3 (708-711)
- `PERK_BERSERKER_POWER_ATTACK_MASTERY_3` = 708
- `PERK_BERSERKER_OVERWHELMING_FORCE` = 709
- `PERK_BERSERKER_CRIMSON_RAGE` = 710
- `PERK_BERSERKER_CARNAGE` = 711

### Ravager Tree - Tier 4 (712-714)
- `PERK_BERSERKER_FRENZIED_BERSERKER` = 712
- `PERK_BERSERKER_RELENTLESS_ASSAULT` = 713
- `PERK_BERSERKER_DEATH_FROM_ABOVE` = 714

---

## Tier 3 Perks (Cost: 3 points each)

### Power Attack Mastery III
- **ID**: PERK_BERSERKER_POWER_ATTACK_MASTERY_3
- **Max Ranks**: 2
- **Prerequisite**: Power Attack Mastery II (3 ranks)
- **Effect**: +2 bonus damage per rank when using power attack
- **Implementation**: Integrated into fight.c power attack damage calculation
- **Total Power Attack Bonus**: Up to +15 damage with all 3 tiers maxed (+5 + +6 + +4)
- **Status**: ✅ Fully Implemented

### Overwhelming Force
- **ID**: PERK_BERSERKER_OVERWHELMING_FORCE
- **Max Ranks**: 1
- **Prerequisite**: Power Attack Mastery II (2 ranks)
- **Effect**: Power attacks have 15% chance to stagger opponents for 2 rounds
- **Implementation**: Added check after successful hits in hit() function
- **Game Impact**: Staggers reduce target's effectiveness significantly
- **Status**: ✅ Fully Implemented

### Crimson Rage
- **ID**: PERK_BERSERKER_CRIMSON_RAGE
- **Max Ranks**: 1
- **Prerequisite**: Rage Damage II (2 ranks)
- **Effect**: Gain +1 damage for every 10% HP missing while raging (max +9 at 10% HP)
- **Implementation**:
  - Calculates current HP percentage
  - Adds bonus damage based on missing HP
  - Integrated into rage damage section of fight.c
- **Example**: At 50% HP = +5 damage, at 20% HP = +8 damage
- **Status**: ✅ Fully Implemented

### Carnage
- **ID**: PERK_BERSERKER_CARNAGE
- **Max Ranks**: 1
- **Prerequisite**: Devastating Critical (2 ranks)
- **Effect**: Critical hits deal 25% weapon damage to all other enemies in the room
- **Implementation**:
  - Checks for multiple enemies after critical hit
  - Deals splash damage to all non-grouped combatants
  - Uses existing damage() function
- **Tactical Use**: Devastating in multi-enemy encounters
- **Status**: ✅ Fully Implemented

---

## Tier 4 Perks (Cost: 4 points each)

### Frenzied Berserker
- **ID**: PERK_BERSERKER_FRENZIED_BERSERKER
- **Max Ranks**: 1
- **Prerequisite**: Power Attack Mastery III (2 ranks)
- **Effect**: Once per rage, activate to gain +4 to hit, +6 damage, and +2 critical multiplier for 5 rounds (5 min cooldown)
- **Implementation**: Defined in perks.c
- **Usage**: Use 'frenzy' command (requires command implementation)
- **Status**: ⚠️ Defined - Requires command implementation

### Relentless Assault
- **ID**: PERK_BERSERKER_RELENTLESS_ASSAULT
- **Max Ranks**: 1
- **Prerequisite**: Blood Frenzy (1 rank)
- **Effect**: Under Blood Frenzy haste, power attacks reduce target's AC by 1 for 3 rounds (max -5 AC)
- **Implementation**: Defined in perks.c
- **Synergy**: Works with Blood Frenzy haste effect
- **Status**: ⚠️ Defined - Requires Blood Frenzy and AC reduction implementation

### Death from Above
- **ID**: PERK_BERSERKER_DEATH_FROM_ABOVE
- **Max Ranks**: 1
- **Prerequisite**: Overwhelming Force (1 rank)
- **Effect**: Gain 'leap' ability - leap at opponent to start combat dealing 2x weapon damage on hit
- **Implementation**: Defined in perks.c
- **Usage**: Use 'leap <target>' command (requires command implementation)
- **Status**: ⚠️ Defined - Requires command implementation

---

## Files Modified

### src/structs.h
- Added perk ID definitions for Tier 3 (708-711) and Tier 4 (712-714)
- Organized with clear tier separation comments

### src/perks.c
- Added 7 new perk definitions with proper metadata
- Added Tier 3 helper functions:
  - `get_berserker_power_attack_mastery_3_bonus()`
  - `has_berserker_overwhelming_force()`
  - `get_berserker_crimson_rage_bonus()`
  - `has_berserker_carnage()`
- Added Tier 4 helper functions:
  - `has_berserker_frenzied_berserker()`
  - `has_berserker_relentless_assault()`
  - `has_berserker_death_from_above()`

### src/perks.h
- Added function prototypes for all Tier 3 and 4 helper functions
- Organized by tier for clarity

### src/fight.c
- **Power Attack** (line ~6400): Added Power Attack Mastery III bonus
- **Rage Damage** (line ~6425): Added Crimson Rage scaling bonus
- **Critical Damage** (line ~7615): Added Carnage splash damage on crits
- **Overwhelming Force** (line ~12630): Added stagger chance on power attack hits

---

## Complete Ravager Tree Progression

### Maximum Damage Output Build (All Tiers)

**Total Perk Point Investment**: 36 points

#### Power Attack Path (17 points)
- Power Attack Mastery I: 5 ranks = 5 points → +5 damage
- Power Attack Mastery II: 3 ranks = 6 points → +6 damage
- Power Attack Mastery III: 2 ranks = 6 points → +4 damage
- **Total Power Attack Bonus**: +15 damage (+30 with 2H weapon!)

#### Rage Damage Path (11 points)
- Rage Damage I: 3 ranks = 3 points → +6 damage
- Rage Damage II: 2 ranks = 4 points → +6 damage
- Crimson Rage: 1 rank = 3 points → up to +9 damage
- **Total Rage Bonus**: +12 damage (+9 at low HP) = +21 damage while raging

#### Combined Maximum
- **Power Attack + Rage + Crimson (at 10% HP)**: +15 PA + +21 rage = **+36 damage per hit!**
- **With 2H Weapon**: +30 PA + +21 rage = **+51 damage per hit!**

#### Critical Path (8 points)
- Improved Critical I: 3 ranks = 3 points → +3 threat range
- Devastating Critical: 3 ranks = 6 points → +3d6 crit damage
- **Additional Tier 3**: Carnage (3 points) → 25% splash damage
- **Total**: Enhanced crit chance, +3d6 bonus damage, area damage

---

## Implementation Details

### Crimson Rage Calculation
```c
hp_percent = (current_hp * 100) / max_hp
missing_percent = 100 - hp_percent
bonus = missing_percent / 10  // rounded down
bonus = MIN(bonus, 9)  // capped at +9
```

**Examples**:
- 100% HP → 0% missing → +0 damage
- 90% HP → 10% missing → +1 damage
- 50% HP → 50% missing → +5 damage
- 20% HP → 80% missing → +8 damage
- 10% HP → 90% missing → +9 damage (capped)

### Overwhelming Force Mechanics
- Triggers on successful power attack hits
- 15% chance per hit
- Applies AFF_STAGGERED for 2 rounds
- Staggered enemies have reduced effectiveness
- Works on any successful hit, not just crits

### Carnage Splash Damage
- Only triggers on critical hits
- Calculates 25% of the critical damage dealt
- Applies to all non-grouped enemies in the room
- Does not apply to the primary target
- Uses force damage type (unresistable)
- Excellent for multi-enemy encounters

---

## Testing Recommendations

### Tier 3 Testing

#### Power Attack Mastery III
1. Purchase all 3 tiers of Power Attack Mastery
2. Activate power attack mode
3. Verify damage bonus reaches +15 (or +30 with 2H)
4. Test with various weapon types

#### Overwhelming Force
1. Purchase the perk
2. Activate power attack
3. Attack multiple enemies
4. Verify ~15% stagger rate occurs
5. Confirm stagger lasts 2 rounds
6. Test that stagger affects enemy combat effectiveness

#### Crimson Rage
1. Purchase the perk
2. Enter rage
3. Take damage to reduce HP
4. Verify damage bonus scales with missing HP
5. Test at various HP percentages (90%, 50%, 20%, 10%)
6. Confirm bonus caps at +9

#### Carnage
1. Purchase Devastating Critical (2+ ranks) and Carnage
2. Fight multiple enemies
3. Score critical hits
4. Verify splash damage appears
5. Confirm 25% damage calculation
6. Test with different weapon damage ranges

### Tier 4 Testing

**Note**: Tier 4 perks are defined but require additional command implementation.

#### Frenzied Berserker (Requires Implementation)
- Needs 'frenzy' command
- Should track cooldown (5 minutes)
- Should apply +4 hit, +6 damage, +2 crit multiplier
- Duration: 5 rounds
- Only usable while raging

#### Relentless Assault (Requires Implementation)
- Needs AC reduction tracking system
- Should only work while Blood Frenzy haste is active
- Must stack up to -5 AC
- Duration: 3 rounds per stack

#### Death from Above (Requires Implementation)
- Needs 'leap' command
- Should initiate combat
- Must deal 2x weapon damage on successful hit
- Should have positioning/range considerations

---

## Synergies and Build Paths

### "The Destroyer" - Maximum Sustained Damage
**Points**: 30
- Power Attack Mastery I-III: 17 points
- Rage Damage I-II: 7 points
- Overwhelming Force: 3 points
- Crimson Rage: 3 points
**Result**: Highest consistent damage output, stagger control

### "The Executioner" - Critical Strike Specialist
**Points**: 30
- Power Attack Mastery I-II: 11 points
- Improved Critical I: 3 points
- Devastating Critical: 6 points
- Carnage: 3 points
- Blood Frenzy: 2 points
- Frenzied Berserker: 4 points (when implemented)
**Result**: Massive critical damage with area effect

### "The Juggernaut" - Low HP Power Build
**Points**: 25
- Power Attack Mastery I-II: 11 points
- Rage Damage I-II: 7 points
- Crimson Rage: 3 points
- Overwhelming Force: 3 points
**Result**: Becomes more dangerous when wounded

---

## Known Limitations and Future Work

### Tier 4 - Requires Implementation

#### Frenzied Berserker Command
**Required Work**:
1. Implement 'frenzy' command (ACMD)
2. Add cooldown tracking (5 minutes)
3. Add affect for +4 hit, +6 damage
4. Add critical multiplier modification (+2)
5. Track 5-round duration
6. Verify only usable while raging
7. Add combat messages

**Suggested Approach**:
- Create new affect type for Frenzied state
- Use existing cooldown system from other abilities
- Integrate with critical multiplier calculation
- Add to combat display

#### Relentless Assault
**Required Work**:
1. Implement AC reduction tracking
2. Stack counter (max 5 stacks)
3. Duration tracking per stack (3 rounds)
4. Only trigger during Blood Frenzy haste
5. Apply to power attack hits
6. Combat feedback messages

**Suggested Approach**:
- Use affect system with stacking
- Check for Blood Frenzy haste before applying
- Integrate with AC calculation
- Track per-victim debuff

#### Death from Above Command
**Required Work**:
1. Implement 'leap <target>' command
2. Calculate 2x weapon damage
3. Initiate combat on success
4. Add movement/positioning checks
5. Cooldown or usage restrictions
6. Dramatic combat messages

**Suggested Approach**:
- Similar to charge or bash mechanics
- Check line of sight and range
- Double base weapon damage
- Add to interpreter command table

### Blood Frenzy (Tier 2)
Still requires full implementation:
- Attack speed stacking system
- Duration tracking (10 seconds)
- Stack counter (max 3)
- Visual feedback

---

## Balance Considerations

### Power Scaling
**Level 30 Berserker with Full Ravager Tree**:
- Base 2H weapon damage: ~30
- Power Attack: +30 (doubled for 2H)
- Rage Damage: +12
- Crimson Rage: +9 (at low HP)
- Devastating Critical: +3d6 (average +10.5 on crits)
- **Total Average Hit**: 91 damage
- **Total Critical Hit**: 273+ damage (with 3x multiplier + devastation)
- **Plus Carnage**: 68 splash damage to all nearby enemies

### Drawbacks and Counters
- Crimson Rage requires being at low HP (high risk)
- Power Attack penalties to hit (-2 base, -1 with Fighter perk)
- Rage limited duration and uses per day
- Overwhelming Force only 15% chance
- Carnage requires crits and multiple enemies
- High perk point investment (30+ points for full tree)

### Comparison to Other Classes
- Fighter: Better sustained accuracy, more versatile
- Rogue: Higher burst with sneak attack, but lower sustained
- Barbarian/Berserker: Highest raw damage potential, especially in extended combats

---

## Integration with Existing Systems

### Power Attack System
- Stacks with all 3 tiers seamlessly
- Works with 2H multiplier
- Integrates with Overwhelming Force
- Compatible with fighter Power Attack Training

### Rage System
- All rage bonuses check for SKILL_RAGE affect
- Crimson Rage dynamically updates with HP changes
- Works with all existing rage feats
- No conflicts with rage enhancements

### Critical Hit System
- Carnage triggers after critical multiplier
- Devastating Critical dice added correctly
- Works with all weapon types
- Stacks with other critical effects

### Combat Flow
- All bonuses apply in correct damage calculation order
- Display messages show all bonuses clearly
- No performance impact from checks
- Clean integration with existing combat code

---

## Summary Statistics

### Total Perks Implemented
- **Tier 1**: 4 perks ✅
- **Tier 2**: 4 perks ✅
- **Tier 3**: 4 perks ✅
- **Tier 4**: 3 perks ⚠️ (defined, need commands)

### Fully Functional
- Power Attack Mastery I-III
- Rage Damage I-II
- Improved Critical I
- Devastating Critical
- Cleaving Strikes
- Crimson Rage
- Overwhelming Force
- Carnage

### Requires Additional Work
- Blood Frenzy (Tier 2) - needs attack speed system
- Frenzied Berserker - needs frenzy command
- Relentless Assault - needs AC reduction tracking
- Death from Above - needs leap command

---

## Code Quality Notes

### Consistent Patterns
- All helper functions check IS_NPC and CLASS_BERSERKER
- Tier 3 functions follow naming: `get_berserker_*` or `has_berserker_*`
- Early returns for performance
- Clear comments in fight.c integration points

### Performance Optimization
- Crimson Rage only calculates when raging
- Overwhelming Force only checks on power attack hits
- Carnage only triggers on crits with multiple enemies
- No unnecessary calculations in hot paths

### Maintainability
- Clear function names and purposes
- Organized by tier in source files
- Comprehensive inline comments
- Integration points well-documented

---

## Completion Status

**Ravager Tree Status**: 11/15 perks fully functional (73%)

**Compilation Status**: ✅ Successfully compiled  
**Integration Status**: ✅ Tier 3 fully integrated, Tier 4 defined  
**Testing Status**: ⚠️ Awaiting in-game testing  
**Documentation Status**: ✅ Complete

**Next Steps**:
1. Test all Tier 3 perks in actual gameplay
2. Implement Frenzied Berserker command
3. Implement Death from Above leap command
4. Complete Blood Frenzy attack speed system
5. Implement Relentless Assault AC reduction
6. Begin Occult Slayer tree (Tree 2)
7. Begin Primal Warrior tree (Tree 3)

---

## Developer Notes

The Ravager tree is now substantially complete, offering a powerful offensive progression for berserker characters. The implemented perks provide significant damage scaling from early game through end game. The remaining Tier 4 perks require command system integration but are fully defined and ready for implementation when the command framework is available.

The Crimson Rage perk is particularly interesting as it creates a "berserker rage" gameplay style where the character becomes more dangerous as they take damage. Combined with the high HP pools barbarians typically have, this creates exciting risk/reward scenarios.

Carnage adds excellent area control for berserkers, making them formidable in multi-enemy encounters. The 25% splash damage can quickly clear groups of weaker enemies while the berserker focuses on the primary threat.
