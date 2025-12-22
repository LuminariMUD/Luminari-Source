# Psychokinetic Arsenal Tier 3 & 4 Perks Implementation

## Overview
This document describes the implementation of Tier 3 and Tier 4 perks for the Psionicist's Psychokinetic Arsenal tree.

## Implemented Perks

### Tier 3 Perks (3 points each)

#### 1. Kinetic Edge III (PERK_PSIONICIST_KINETIC_EDGE_III - 1322)
**Prerequisites:** Kinetic Edge II
**Description:** Total +3 dice on Psychokinesis blasts; energy ray/energy push gain +2 DC.

**Implementation:**
- Added to perk definitions in [perks.c](perks.c#L2260)
- Helper functions: `has_kinetic_edge_iii()`, `get_kinetic_edge_iii_bonus()`, `get_kinetic_edge_iii_dc_bonus()`
- Integrated into damage calculations in [magic.c](magic.c):
  - PSIONIC_ENERGY_RAY: +1 die, +2 DC
  - PSIONIC_ENERGY_PUSH: +1 die, +2 DC  
  - PSIONIC_CONCUSSION_BLAST: +1 die
  - PSIONIC_CRYSTAL_SHARD: +1 die
  - PSIONIC_ENERGY_BURST: +1 die

**Mechanics:**
- Stacks with Kinetic Edge I and II for total +3 dice bonus
- Adds +2 DC specifically to energy ray and energy push powers
- Works with augmented powers

#### 2. Gravity Well (PERK_PSIONICIST_GRAVITY_WELL - 1323)
**Prerequisites:** None
**Description:** Once per combat, create an AoE effect that halves speed and prevents fleeing (Reflex negates each round); lasts 3 rounds.

**Implementation:**
- Added to perk definitions in [perks.c](perks.c#L2273)
- Helper functions: `has_gravity_well()`, `can_use_gravity_well()`, `use_gravity_well()`
- Uses combat cooldown event: eGRAVITY_WELL_USED

**Mechanics:**
- Once per combat ability
- Creates area-of-effect gravity field
- 50% speed reduction
- Prevents fleeing
- Reflex save each round to negate
- 3 round duration

**Note:** Requires command implementation in interpreter.c and full power logic for complete functionality.

#### 3. Force Aegis (PERK_PSIONICIST_FORCE_AEGIS - 1324)
**Prerequisites:** Deflective Screen
**Description:** +3 AC vs ranged/spells while force screen/inertial armor active; gain temp HP = manifester level on cast.

**Implementation:**
- Added to perk definitions in [perks.c](perks.c#L2286)
- Helper functions: `has_force_aegis()`, `get_force_aegis_ranged_ac_bonus()`, `get_force_aegis_temp_hp_bonus()`
- Integrated into PSIONIC_FORCE_SCREEN and PSIONIC_INERTIAL_ARMOR in [magic.c](magic.c)

**Mechanics:**
- +3 AC bonus vs ranged attacks and spells (adds to shield/armor bonus)
- Grants temporary hit points equal to manifester level when casting force screen or inertial armor
- Stacks with Deflective Screen bonuses
- Active only while force screen or inertial armor is active

#### 4. Kinetic Crush (PERK_PSIONICIST_KINETIC_CRUSH - 1325)
**Prerequisites:** Vector Shove
**Description:** Forced-movement powers add prone on failed Reflex; if target collides, take extra force damage = manifester level.

**Implementation:**
- Added to perk definitions in [perks.c](perks.c#L2299)
- Helper functions: `has_kinetic_crush()`, `should_apply_kinetic_crush_prone()`, `get_kinetic_crush_collision_damage()`
- Integrated into PSIONIC_ENERGY_PUSH forced movement logic in [magic.c](magic.c)

**Mechanics:**
- Enhanced prone effect (POS_RESTING instead of POS_SITTING) on failed save
- Adds extra collision damage equal to manifester level when target hits a wall
- Works with all forced-movement psychokinesis powers
- Stacks with Vector Shove bonuses

### Tier 4 Capstone Perks (5 points each)

#### 1. Singular Impact (PERK_PSIONICIST_SINGULAR_IMPACT - 1326)
**Prerequisites:** Kinetic Edge III
**Description:** 1/day Psychokinesis strike: heavy force damage, auto-bull rush, and stun 1 round (Fort partial: half damage, no stun).

**Implementation:**
- Added to perk definitions in [perks.c](perks.c#L2314)
- Helper functions: `has_singular_impact()`, `can_use_singular_impact()`, `use_singular_impact()`
- Uses daily cooldown event: eSINGULAR_IMPACT_USED (24 hours)

**Mechanics:**
- Once per day ability
- Heavy force damage: (manifester_level / 2)d10 + INT bonus
- Auto bull-rush: knocks target prone
- Stun effect: 1 round duration on failed Fort save
- Fortitude save negates stun (damage still applies)

**Note:** Requires command implementation for player activation.

#### 2. Perfect Deflection (PERK_PSIONICIST_PERFECT_DEFLECTION - 1327)
**Prerequisites:** Force Aegis
**Description:** 1/day reaction: negate one ranged/spell/psionic attack against you and reflect it using your casting stat vs the original attacker.

**Implementation:**
- Added to perk definitions in [perks.c](perks.c#L2327)
- Helper functions: `has_perfect_deflection()`, `can_use_perfect_deflection()`, `use_perfect_deflection()`
- Uses daily cooldown event: ePERFECT_DEFLECTION_USED (24 hours)

**Mechanics:**
- Once per day reaction ability
- Negates one incoming ranged/spell/psionic attack
- Reflects the attack back at the attacker
- Uses psionicist's casting stat for the reflected attack

**Note:** Requires integration into combat attack resolution code for full functionality.

## Files Modified

### Core Files
- **src/structs.h**: Added perk constant definitions (1322-1327)
- **src/perks.h**: Added helper function declarations
- **src/perks.c**: Added perk definitions and helper function implementations
- **src/magic.c**: Integrated perk bonuses into psionic power calculations
- **src/mud_event.h**: Added new event types (eGRAVITY_WELL_USED, eSINGULAR_IMPACT_USED, ePERFECT_DEFLECTION_USED)
- **src/spells.h**: Added PSIONIC_GRAVITY_WELL spell definition (1604)

## Testing Recommendations

1. **Kinetic Edge III**
   - Test damage dice progression with augmentation
   - Verify DC bonuses apply to energy ray and energy push
   - Confirm stacking with lower tiers

2. **Force Aegis**
   - Test AC bonus application vs ranged attacks
   - Verify temporary HP granted on spell cast
   - Check that bonuses apply only while armor/screen active

3. **Kinetic Crush**
   - Test prone effect application
   - Verify collision damage in indoor locations
   - Confirm it works with Vector Shove

4. **Singular Impact**
   - Test daily cooldown
   - Verify damage scaling
   - Test stun save mechanics

5. **Perfect Deflection**
   - Requires full implementation before testing

## Balance Notes

- Tier 3 perks provide significant power increases but require investment in prerequisite perks
- Force Aegis temp HP scales with level, providing increasing survivability
- Kinetic Edge III completes the damage progression tree (+1/+2/+3 dice)
- Capstone perks are powerful but limited to 1/day use
- Gravity Well still needs command implementation for full functionality

## Future Enhancements

1. Implement Gravity Well as a usable command
2. Implement Singular Impact as a usable command  
3. Complete Perfect Deflection integration into combat code
4. Add visual effects/messages for perk activations
5. Create help files for players
6. Add perk availability to character sheet display
