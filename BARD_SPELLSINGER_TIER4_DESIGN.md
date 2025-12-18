# Tier 4 Spellsinger Bard Perks - Design Document

## Perk IDs
- PERK_BARD_SPELLSONG_MAESTRA (1112)
- PERK_BARD_ARIA_OF_STASIS (1113)
- PERK_BARD_SYMPHONIC_RESONANCE (1114)
- PERK_BARD_ENDLESS_REFRAIN (1115)

## Tier 4 Perks

### 1. Spellsong Maestra (1112)
**Cost:** 5 points
**Prerequisites:** Master of Motifs, Heightened Harmony
**Description:** Your songs amplify spellcasting to legendary power. While performing, your bard spells gain +2 effective caster level and +2 to spell DCs. Additionally, you may spend metamagic freely on bard spells without increasing their spell circle cost.

**Mechanics:**
- `has_bard_spellsong_maestra()` - Check if character has perk
- `get_bard_spellsong_maestra_caster_bonus()` - Returns +2 caster level while performing
- `get_bard_spellsong_maestra_dc_bonus()` - Returns +2 spell DC for bard spells while performing
- `has_bard_spellsong_maestra_metamagic_free()` - Check if metamagic is free (only applies to bard spells)
- **Integration:** Modify `compute_spell_circle()` to allow free metamagic on bard spells when perks active; add caster level/DC bonuses to spell calculations

### 2. Aria of Stasis (1113)
**Cost:** 5 points
**Prerequisites:** Protective Chorus, Dirge of Dissonance
**Description:** Allies under your song gain +4 all saves and immunity to movement-impairing effects. Enemies in your performance's radius suffer -2 to hit and are slowed 10% (movement speed reduced).

**Mechanics:**
- `has_bard_aria_of_stasis()` - Check if character has perk
- `get_bard_aria_stasis_ally_saves()` - Returns +4 to all saves for allies in group/room
- `get_bard_aria_stasis_enemy_penalty()` - Returns -2 to hit for enemies
- `get_bard_aria_stasis_slow_amount()` - Returns movement penalty (10%)
- **Integration:** Modify `savingthrow_full()` for allies; add tohit penalty when computing attacks vs. affected enemies; apply movement penalty in movement system

### 3. Symphonic Resonance (1114)
**Cost:** 5 points
**Prerequisites:** Crescendo, Enchanter's Guile II
**Description:** Every round while performing, you and your allies gain +1d6 temporary HP (stacking, max 30 rounds). When you cast an Enchantment or Illusion spell during a song, all affected enemies within 20 feet are dazed for 1 round.

**Mechanics:**
- `has_bard_symphonic_resonance()` - Check if character has perk
- `get_bard_symphonic_resonance_temp_hp()` - Returns 1d6 temp HP per round
- `get_bard_symphonic_resonance_daze_duration()` - Returns 1 round daze on Enchantment/Illusion spell cast
- `get_bard_symphonic_resonance_daze_range()` - Returns 20 feet range
- **Integration:** Add temp HP bonus in periodic update system; apply daze effect in spell casting system for Enchantment/Illusion spells cast during performance

### 4. Endless Refrain (1115)
**Cost:** 5 points
**Prerequisites:** Sustaining Melody, Spellsong Maestra (optional but thematic)
**Description:** Your performance pool does not deplete. Instead, each round while performing costs 0 moves, 0 actions, and regenerates 1 spell slot. Songs last indefinitely until manually stopped.

**Mechanics:**
- `has_bard_endless_refrain()` - Check if character has perk
- `get_bard_endless_refrain_slot_regen()` - Returns 1 spell slot regenerated per round
- `should_endless_refrain_consume_performance()` - Returns FALSE (performance is free)
- **Integration:** Modify `pulse_bardic_performance()` to skip performance cost when active; add spell slot regen each round; modify performance reset logic to never auto-stop

## Implementation Plan

1. Add 4 perk IDs to structs.h (1112-1115)
2. Add 15+ helper function declarations to perks.h
3. Add perk definitions to perks.c define_bard_perks()
4. Implement all 15+ helper functions in perks.c
5. Integrate into:
   - `call_magic()` or spell DC calculation for Spellsong Maestra caster bonus/DC
   - `savingthrow_full()` for Aria of Stasis ally saves
   - `compute_tohit()` for Aria of Stasis enemy penalty
   - Spell casting system for Symphonic Resonance daze
   - `pulse_bardic_performance()` for Endless Refrain slot regen
   - Movement system for Aria of Stasis slow
   - Periodic update for Symphonic Resonance temp HP

## Testing Checklist

- [ ] Spellsong Maestra: Bard spells gain +2 caster and +2 DC while performing
- [ ] Spellsong Maestra: Metamagic on bard spells doesn't increase spell circle
- [ ] Aria of Stasis: Grouped allies gain +4 all saves
- [ ] Aria of Stasis: Enemies suffer -2 to hit and 10% slow
- [ ] Symphonic Resonance: Allies gain 1d6 temp HP per round (stacks to 30 rounds)
- [ ] Symphonic Resonance: Enchantment/Illusion spells cause 1-round daze in 20 ft radius
- [ ] Endless Refrain: Performance is free (no cost) and regenerates 1 spell slot per round
- [ ] Endless Refrain: Songs last indefinitely until manually stopped
