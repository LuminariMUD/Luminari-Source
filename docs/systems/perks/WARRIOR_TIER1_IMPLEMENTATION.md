# Warrior Tier I Perks Implementation

## Summary
Successfully implemented Tier I perks for the Warrior class based on the design in WARRIOR_PERKS.md. These are the foundational perks from Tree 1: Weapon Specialist.

## Implemented Perks

### 1. Weapon Focus I (ID: 1)
- **Cost**: 1 point
- **Max Rank**: 1
- **Prerequisite**: None
- **Effect**: +1 to hit with all weapons
- **Implementation**: Uses PERK_EFFECT_WEAPON_TOHIT, handled automatically by existing perk system

### 2. Power Attack Training (ID: 2)
- **Cost**: 1 point
- **Max Rank**: 1
- **Prerequisite**: None
- **Effect**: 
  - +2 damage when using power attack
  - Reduces to-hit penalty from -2 to -1
- **Implementation**: 
  - Uses PERK_EFFECT_SPECIAL
  - Custom handling in `fight.c`:
    - Line ~6083: Adds +2 damage bonus to power attack
    - Line ~8765: Reduces penalty by 1 (minimum penalty of 1)

### 3. Critical Awareness I (ID: 3)
- **Cost**: 1 point
- **Max Rank**: 1
- **Prerequisite**: None
- **Effect**: +1 to critical confirmation rolls
- **Implementation**:
  - Uses PERK_EFFECT_SPECIAL
  - Custom handling in `fight.c` line ~6900: Adds +1 to confirm_roll in `is_critical_hit()`

## File Changes

### src/structs.h
- Reorganized fighter perk IDs to accommodate new Tier I perks
- New perks use IDs 1-3
- Existing perks shifted: WEAPON_SPEC_1 (1→4), ARMOR_MASTERY_1 (4→7), TOUGHNESS (9→12), etc.

### src/perks.c
- Added 3 new perk definitions at start of `define_fighter_perks()`
- Each perk includes:
  - ID, name, description
  - Associated class (CLASS_WARRIOR)
  - Cost and max rank
  - Prerequisite (-1 for none)
  - Effect type and values
  - Special description

### src/fight.c
- **Power Attack Training** handling:
  - Modified damage calculation (~line 6083): Adds +2 bonus when perk is active
  - Modified to-hit penalty (~line 8765): Reduces penalty by 1 when perk is active
  
- **Critical Awareness I** handling:
  - Modified `is_critical_hit()` function (~line 6900): Adds +1 to confirmation roll

## Testing Recommendations

1. **Weapon Focus I**:
   - Create warrior character
   - Purchase Weapon Focus I perk
   - Verify +1 to hit in combat
   - Check 'perk' command shows perk active

2. **Power Attack Training**:
   - Purchase perk
   - Activate power attack mode
   - Verify damage increases by +2 more than normal power attack
   - Verify to-hit penalty is -1 instead of -2
   - Check with 2-handed weapons (should get bonus on doubled amount)

3. **Critical Awareness I**:
   - Purchase perk
   - Make attacks that threaten criticals
   - Verify confirmation rolls have +1 bonus
   - Can test with lower-level enemies to see more frequent confirmations

## Integration Notes

- All perks integrate with existing perk system infrastructure
- Weapon Focus I works automatically through generic perk effect handlers
- Power Attack Training and Critical Awareness I required custom code in fight.c
- Perk availability checked via `has_perk(ch, PERK_ID)` function
- All effects stack properly with other bonuses

## Future Work

From WARRIOR_PERKS.md Tree 1 (Weapon Specialist):

**Tier II** (Prerequisites require Tier I):
- Perks 4-8: Weapon Focus II, Power Attack Mastery, Critical Awareness II, Weapon Training, Combat Reflexes I

**Tier III** (Prerequisites require Tier II):
- Perks 9-14: Weapon Focus III, Power Attack Expertise, Critical Awareness III, Improved Critical I, Combat Reflexes II, Greater Weapon Training

Total Tree 1 Cost: 45 points across 14 perks

## Compilation Status

✅ Successfully compiled with no errors or warnings
✅ All perk IDs properly defined in structs.h
✅ All perk definitions added to perks.c
✅ Custom effect handlers implemented in fight.c
✅ perks.h already includes necessary function prototypes
