# Tier 4 Bomb Craftsman Perks - Implementation Summary

## Overview
Successfully implemented both tier 4 (capstone) bomb craftsman perks for alchemists: **Bombardier Savant** and **Volatile Catalyst**. All code has been compiled successfully with no errors.

## Perks Implemented

### 1. Bombardier Savant (ID: 1226)
**Cost:** 5 points  
**Max Rank:** 1  
**Prerequisites:** Bomb Mastery + Calculated Throw  

**Mechanics:**
- **+3 to Ranged Touch Attacks:** All bomb throws gain a +3 bonus to hit
- **+6d6 Damage Bonus:** All bombs deal an additional 6d6 damage when thrown
- **Passive Effect:** No toggle required, automatically active

**Implementation Files:**
- `src/structs.h` - Added PERK_ALCHEMIST_BOMBARDIER_SAVANT (1226)
- `src/perks.c` - Perk initialization, helper functions, prerequisite validation
- `src/perks.h` - Helper function declarations
- `src/alchemy.c` - Attack bonus applied in `do_bombs()`, damage bonus in `perform_bomb_direct_damage()`

### 2. Volatile Catalyst (ID: 1227)
**Cost:** 5 points  
**Max Rank:** 1  
**Prerequisites:** Inferno Bomb + Cluster Bomb  

**Mechanics:**
- **Chain Bomb Triggers:** When enabled, bombs have a 1% chance per prepared bomb to trigger an automatic bonus bomb throw
  - Example: With 5 bombs prepared, there's a 5% chance per bomb throw to trigger a chain reaction
- **Automatic Chain Reaction:** Chain bombs are thrown without consuming an action, creating a true chain reaction feel
- **Recursion Prevention:** Chain bombs don't trigger additional chains (removed from inventory before throwing)
- **Toggle Command:** Use `volatilecatalyst` command to toggle on/off

**Implementation Files:**
- `src/structs.h` - Added PERK_ALCHEMIST_VOLATILE_CATALYST (1227)
- `src/perks.c` - Perk initialization, helper functions, prerequisite validation
- `src/perks.h` - Helper function declarations
- `src/alchemy.h` - Added ACMD_DECL(do_volatilecatalyst)
- `src/alchemy.c` - Command implementation, chain bomb mechanics in `perform_bomb_effect()`
- `src/interpreter.c` - Command registration in player command list

## Helper Functions Implemented

### Bombardier Savant Helpers
```c
int has_alchemist_bombardier_savant(struct char_data *ch)
// Returns TRUE if character owns the Bombardier Savant perk

int get_bombardier_savant_attack_bonus(struct char_data *ch)
// Returns +3 if owned, 0 otherwise

int get_bombardier_savant_damage_bonus(struct char_data *ch)
// Returns rolled 6d6 if owned, 0 otherwise
```

### Volatile Catalyst Helpers
```c
int has_alchemist_volatile_catalyst(struct char_data *ch)
// Returns TRUE if character owns the Volatile Catalyst perk

int is_volatile_catalyst_on(struct char_data *ch)
// Returns TRUE if perk is owned AND toggled on
```

## Integration Points

### 1. Bomb Attack Calculation (`do_bombs`)
- Added `bombardier_attack_bonus` calculation
- Applied bonus to `bomb_hit_result` before attack roll

### 2. Bomb Damage Calculation (`perform_bomb_direct_damage`)
- Added `get_bombardier_savant_damage_bonus(ch)` to damage calculation
- Stacks with existing bonuses (Bomb Mastery, etc.)

### 3. Chain Bomb Mechanics (`perform_bomb_effect`)
- Checks `is_volatile_catalyst_on(ch)` for active perk
- Calculates probability: 1% × number of prepared bombs
- Selects random bomb slot with wrap-around iteration
- Removes bomb before throwing to prevent recursion
- Initiates combat if needed
- Provides immersive action text for chain reaction

### 4. Toggle Command (`do_volatilecatalyst`)
- Mirroring the pattern of `do_unstablemutagen`
- NPC safety checks included
- Perk ownership validation
- Character auto-save on toggle
- Colorized feedback: GREEN for ON, RED for OFF

## Prerequisite Validation

Added special prerequisite checks in `can_purchase_perk()`:

**Bombardier Savant:**
- Requires PERK_ALCHEMIST_CALCULATED_THROW to purchase
- Follows existing perk chain validation pattern

**Volatile Catalyst:**
- Requires PERK_ALCHEMIST_CLUSTER_BOMB to purchase
- Follows existing perk chain validation pattern

## Compilation Status
✅ **SUCCESSFUL** - All code compiles without errors
- Modified files successfully compiled
- Command registration verified
- All function declarations properly linked

## Testing Checklist

The following should be verified through gameplay testing:

- [ ] Bombardier Savant +3 attack bonus applies to all bomb throws
- [ ] Bombardier Savant +6d6 damage bonus applies to all bombs
- [ ] Volatile Catalyst toggle command works (`volatilecatalyst` turns on/off)
- [ ] Chain bombs trigger with 1% per prepared bomb probability
- [ ] Chain bombs don't consume actions (feel automatic)
- [ ] Chain bombs don't recursively trigger more chains
- [ ] Chain bombs select from available prepared bombs correctly
- [ ] Prerequisite validation prevents purchasing without requirements
- [ ] Both perks properly cost 5 points each
- [ ] Max rank of 1 enforced for both capstone perks

## Code Statistics

**Total Lines Added:**
- `src/structs.h`: 2 lines (perk definitions)
- `src/perks.h`: 6 lines (declarations)
- `src/perks.c`: ~100 lines (initialization, prerequisites, helpers)
- `src/alchemy.h`: 1 line (command declaration)
- `src/alchemy.c`: ~135 lines (command, mechanics, integration)
- `src/interpreter.c`: 1 line (command registration)

**Total: ~245 lines of code**

## Tier System Completion

### Tier 1 (Preparation)
- ✅ Preparation
- ✅ Bomb Coating
- ✅ Efficient Bomb Making
- ✅ Volatile Preparation

### Tier 2 (Combat Application)
- ✅ Precise Throw
- ✅ Point Blank Bomb
- ✅ Explosive Cascade
- ✅ Focused Blast

### Tier 3 (Advanced Techniques)
- ✅ Inferno Bomb
- ✅ Cluster Bomb
- ✅ Calculated Throw
- ✅ Bomb Mastery

### Tier 4 (Capstone)
- ✅ Bombardier Savant
- ✅ Volatile Catalyst

## Known Limitations

None at this time. All requested features have been fully implemented and compiled successfully.

## Related Documentation

See [ALCHEMIST_PERKS.md](docs/systems/perks/ALCHEMIST_PERKS.md) for complete perk descriptions and flavor text.

