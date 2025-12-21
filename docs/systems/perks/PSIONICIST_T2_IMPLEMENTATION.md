# Psionicist Tier 2 Telepathic Control Perks - Implementation Guide

## Overview
The Tier 2 Telepathic Control perks have been fully implemented with supporting mechanics. These perks build upon Tier 1 perks and offer more powerful effects.

## Tier 2 Perks Summary

### 1. Mind Spike II (PERK_PSIONICIST_MIND_SPIKE_II - ID: 1304)
**Prerequisite:** Mind Spike I

**Mechanics:**
- Adds +1 DC to Telepathy powers (combined with Tier I = +2 total)
- Telepathy damage powers gain +1 damage die if augmented by ≥2 PSP

**Implementation:**
- `get_psionic_telepathy_dc_bonus()` - Returns cumulative DC bonus (combines Tier I and Tier II)
- `has_mind_spike_ii_bonus(ch, augment_spent)` - Helper function that checks if both perk is active AND augment spent is ≥2

**Integration Points:**
- DC calculation for Telepathy powers should use `get_psionic_telepathy_dc_bonus(caster)`
- Damage calculation for Telepathy damage powers should check `has_mind_spike_ii_bonus(caster, augment_psp)` and add +1 die

### 2. Overwhelm (PERK_PSIONICIST_OVERWHELM - ID: 1305)
**Prerequisite:** Suggestion Primer

**Mechanics:**
- First Telepathy power each encounter forces targets to save twice and take the worse result
- Can only be used once per combat

**Implementation:**
- `has_overwhelm(ch)` - Checks if character has the perk
- `overwhelm_used_this_combat(ch)` - Checks if Overwhelm has already been used (via affect tracking)
- `set_overwhelm_cooldown(ch)` - Sets a 60-tick affect to mark Overwhelm as used this combat

**Integration Points:**
- When manifesting a Telepathy power, check if caster has Overwhelm and hasn't used it yet
- If conditions met: target makes save twice using `worse` of the two results
- After applying Overwhelm effect, call `set_overwhelm_cooldown(caster)` to prevent re-use

**Note on Save Calculation:**
When targets save twice with Overwhelm:
```c
int save1 = saving_throw(target, spell_dc, 1);  // First save attempt
int save2 = saving_throw(target, spell_dc, 1);  // Second save attempt
int final_save = MAX(save1, save2);             // Take the WORSE (higher) result
if (final_save > 0) target_saves = TRUE;        // Higher number = better for target (save succeeds)
```

### 3. Psionic Disruptor II (PERK_PSIONICIST_PSIONIC_DISRUPTOR_II - ID: 1306)
**Prerequisite:** Psionic Disruptor I

**Mechanics:**
- Adds +1 manifester level vs power resistance for Telepathy powers (combined with Tier I = +2 total)

**Implementation:**
- `get_psionic_telepathy_penetration_bonus()` - Returns cumulative penetration bonus

**Integration Points:**
- Power resistance penetration checks should use `get_psionic_telepathy_penetration_bonus(caster)` as bonus

### 4. Linked Menace (PERK_PSIONICIST_LINKED_MENACE - ID: 1307)
**Prerequisite:** Focus Channeling

**Mechanics:**
- When landing a Telepathy debuff on a target, they take -2 penalty to AC for 2 rounds

**Implementation:**
- `has_linked_menace(ch)` - Checks if character has the perk
- `apply_linked_menace_ac_penalty(vict)` - Applies -2 AC affect for 12 ticks (2 rounds)

**Integration Points:**
- After successfully applying a Telepathy debuff (when save fails), check if caster has Linked Menace
- If yes, call `apply_linked_menace_ac_penalty(target)`

**AC Modifier Note:**
The affect uses `APPLY_AC` with modifier=2. In the AC system, higher values = worse AC, so +2 modifier means -2 in AC terms.

## Code Organization

### Files Modified:
1. **src/structs.h**
   - Added 4 new perk ID constants (1304-1307)

2. **src/perks.c**
   - Added Tier 2 perk definitions in `init_perks()`
   - Updated `get_psionic_telepathy_dc_bonus()` to include Tier II
   - Updated `get_psionic_telepathy_penetration_bonus()` to include Tier II
   - Added 6 new helper functions for Tier II mechanics

3. **src/perks.h**
   - Added declarations for all new helper functions

## Integration Checklist

For game mechanics to work, integrate these functions into:

- [ ] **Psionic Power DC Calculation** - Use `get_psionic_telepathy_dc_bonus()` when calculating save DCs
- [ ] **Power Resistance Penetration** - Use `get_psionic_telepathy_penetration_bonus()` in PR checks
- [ ] **Save System** - Implement double-save logic when Overwhelm is active
- [ ] **Telepathy Power Application** - Call `apply_linked_menace_ac_penalty()` after successful debuffs
- [ ] **Damage Calculation** - Check `has_mind_spike_ii_bonus()` for +1 die on damage powers
- [ ] **Combat Start** - Clear Overwhelm cooldown at start of new combat encounter

## Example Integration Code

### Power DC Bonus Integration:
```c
// In psionic power DC calculation
int spell_dc = base_dc + get_psionic_telepathy_dc_bonus(caster);
```

### Overwhelm Save Integration:
```c
if (has_overwhelm(caster) && !overwhelm_used_this_combat(caster)) {
  int save1 = saving_throw(target, spell_dc, SAVING_WILL);
  int save2 = saving_throw(target, spell_dc, SAVING_WILL);
  target_saves = MAX(save1, save2) > 0;  // Worse result (higher = save succeeds)
  
  if (!target_saves) {
    set_overwhelm_cooldown(caster);
  }
}
```

### Linked Menace Integration:
```c
// After a successful Telepathy debuff applies
if (has_linked_menace(caster)) {
  apply_linked_menace_ac_penalty(target);
}
```

### Mind Spike II Damage Bonus Integration:
```c
// In Telepathy damage power calculation
int damage = base_damage;
if (has_mind_spike_ii_bonus(caster, augment_spent)) {
  damage += roll_dice(1, damage_die);  // Add +1 die
}
```

## Testing Recommendations

1. **Mind Spike II DC:**
   - Verify Tier I grants +1 DC (total)
   - Verify Tier II adds +1 more (total +2)
   - Verify augment check works (≥2 PSP requirement)

2. **Overwhelm:**
   - Cast first Telepathy power in combat - should force double save
   - Attempt second cast - should NOT force double save (cooldown active)
   - Verify cooldown clears on new combat

3. **Psionic Disruptor II:**
   - Verify Tier I grants +1 penetration bonus
   - Verify Tier II adds +1 more (total +2)

4. **Linked Menace:**
   - Apply Telepathy debuff with perk active
   - Verify target gets -2 AC for 2 rounds
   - Verify affect expires after 2 rounds

## Future Integration

These Tier 2 perks are building blocks for future Tier 3 and Tier 4 capstones. The architecture allows for easy stacking and combination effects.
