# Tier 4 Spellsinger Bard Perks - Full Implementation Guide

**Status:** ✅ Defined & Compiled  
**Date:** December 18, 2025  
**Version:** 1.0

---

## Overview

Tier 4 Spellsinger perks represent the capstone abilities of the Spellsinger tree. These are legendary powers that define the ultimate spellcasting bard. Each tier 4 perk costs 5 perk points and requires a specific Tier 3 prerequisite.

---

## Implementation Checklist

### Perk Definitions ✅
- [x] PERK_BARD_SPELLSONG_MAESTRA (1112) defined in structs.h
- [x] PERK_BARD_ARIA_OF_STASIS (1113) defined in structs.h
- [x] PERK_BARD_SYMPHONIC_RESONANCE (1114) defined in structs.h
- [x] PERK_BARD_ENDLESS_REFRAIN (1115) defined in structs.h

### Function Declarations ✅
- [x] 11 functions declared in perks.h:
  - `has_bard_spellsong_maestra()`
  - `get_bard_spellsong_maestra_caster_bonus()`
  - `get_bard_spellsong_maestra_dc_bonus()`
  - `has_bard_spellsong_maestra_metamagic_free()`
  - `has_bard_aria_of_stasis()`
  - `get_bard_aria_stasis_ally_saves_bonus()`
  - `get_bard_aria_stasis_enemy_tohit_penalty()`
  - `get_bard_aria_stasis_movement_penalty()`
  - `has_bard_symphonic_resonance()`
  - `get_bard_symphonic_resonance_temp_hp()`
  - `get_bard_symphonic_resonance_daze_duration()`
  - `get_bard_symphonic_resonance_daze_range()`
  - `has_bard_endless_refrain()`
  - `get_bard_endless_refrain_slot_regen()`
  - `should_endless_refrain_consume_performance()`

### Perk Initialization ✅
- [x] All 4 perks initialized in `define_bard_perks()` in perks.c

### Helper Functions ✅
- [x] All 15 helper functions implemented in perks.c with:
  - NPC checks (returns FALSE for NPCs)
  - Null pointer checks
  - Proper return types and values
  - Comprehensive documentation

### Build Status ✅
- [x] Clean compilation (no errors or warnings)

---

## Perk Details

### 1. SPELLSONG MAESTRA (1112)

**Prerequisites:** Master of Motifs (Tier 3)  
**Cost:** 5 points  
**Max Ranks:** 1  
**Category:** Spellsinger (30)

#### Mechanics

While performing a bardic song, this perk grants:

1. **+2 Caster Level** to bard spells
   - Function: `get_bard_spellsong_maestra_caster_bonus()` returns +2
   - Integration: Add to spell circle calculations and spell damage
   - Active only while IS_PERFORMING(ch)

2. **+2 Spell DC** to bard spells
   - Function: `get_bard_spellsong_maestra_dc_bonus()` returns +2
   - Integration: Add to DC calculations in `savingthrow_full()`
   - Active only for bard spells while IS_PERFORMING(ch)

3. **Free Metamagic** on bard spells
   - Function: `has_bard_spellsong_maestra_metamagic_free()` returns TRUE if active
   - Integration: Modify `spell_circle_to_spell_level()` to not add metamagic cost
   - Applies only to bard spells while IS_PERFORMING(ch)

#### Integration Points

- **Spell DC:** Add to `savingthrow_full()` in magic.c after spell preparation
- **Caster Level:** Add to spell damage/effect calculations in spells.c
- **Metamagic:** Modify spell circle calculation to check this perk

#### Testing

```c
// Test: Cast Enchantment spell during song with Heightened Harmony active
// Expected: Spell DC +2 from Maestra + normal boosts
// Expected: Caster level boosted by 2
// Expected: Any metamagic on the spell doesn't increase circle cost
```

---

### 2. ARIA OF STASIS (1113)

**Prerequisites:** Protective Chorus (Tier 3)  
**Cost:** 5 points  
**Max Ranks:** 1  
**Category:** Spellsinger (30)

#### Mechanics

While performing, this perk grants:

1. **Allies:** +4 to ALL saves and immunity to slow effects
   - Function: `get_bard_aria_stasis_ally_saves_bonus()` returns +4
   - Integration: Add to `savingthrow_full()` in magic.c for grouped members
   - Also set AFF_SLOWED flag to prevent movement penalties

2. **Enemies:** -2 to hit and 10% movement speed reduction
   - Function: `get_bard_aria_stasis_enemy_tohit_penalty()` returns -2
   - Integration: Apply penalty in `compute_tohit()` in fight.c
   - Function: `get_bard_aria_stasis_movement_penalty()` returns 10
   - Integration: Apply 10% movement penalty when enemies move

#### Integration Points

- **Ally Saves:** Modify `savingthrow_full()` to add +4 bonus when player has perk and is performing
- **Enemy To-Hit:** In `compute_tohit()`, subtract 2 from attacker's roll if target's group has performing bard with perk
- **Movement Penalty:** In movement system, apply 10% speed reduction to enemies affected by the perk

#### Buff Application

The perk itself should apply an affect buff to allies while active:

```c
struct affected_type aria_af;
new_affect(&aria_af);
aria_af.spell = PERK_BARD_ARIA_OF_STASIS;
aria_af.duration = 3; // 3 rounds while performing
aria_af.location = APPLY_NONE;
SET_BIT_AR(aria_af.bitvector, AFF_PROTECT_EVIL); // placeholder
aria_af.modifier = 0;
affect_to_char(ch, &aria_af);
```

#### Testing

```c
// Test: Start song with Aria, check allied saves
// Expected: +4 to all saves for group members in room
// Expected: Allies cannot be slowed
// Expected: Enemies in room get -2 to hit vs. allies
```

---

### 3. SYMPHONIC RESONANCE (1114)

**Prerequisites:** Crescendo (Tier 3)  
**Cost:** 5 points  
**Max Ranks:** 1  
**Category:** Spellsinger (30)

#### Mechanics

While performing, this perk grants:

1. **Temporary HP** per round
   - Function: `get_bard_symphonic_resonance_temp_hp()` returns 1 (roll 1d6)
   - Integration: In periodic update (e.g., `pulse_bardic_performance()`), grant 1d6 temp HP
   - Stacks up to 30 rounds worth (180 total temp HP)
   - Applies to bard and grouped allies

2. **Daze Effect** on Enchantment/Illusion spells
   - Function: `get_bard_symphonic_resonance_daze_duration()` returns 1 (1 round)
   - Function: `get_bard_symphonic_resonance_daze_range()` returns 20 (20 feet)
   - Integration: When casting Enchantment/Illusion spell during performance, apply 1-round daze to all enemies within 20 feet
   - Triggered in `finishCasting()` in spell_parser.c

#### Temporary HP Stacking

```c
// In periodic update (pulse_bardic_performance or similar):
if (has_bard_symphonic_resonance(ch) && IS_PERFORMING(ch))
{
  int temp_hp = dice(1, 6);
  
  // Apply to bard
  ch->char_specials.tempHitPoints += temp_hp;
  
  // Apply to allies
  struct char_data *tch = NULL;
  simple_list(NULL);
  while ((tch = (struct char_data *)simple_list(GROUP(ch)->members)) != NULL)
  {
    if (IN_ROOM(tch) == IN_ROOM(ch))
      tch->char_specials.tempHitPoints += temp_hp;
  }
  
  // Cap at 180 total
  if (ch->char_specials.tempHitPoints > 180)
    ch->char_specials.tempHitPoints = 180;
}
```

#### Daze Effect Trigger

```c
// In finishCasting() when spell is Enchantment/Illusion school:
if (!IS_NPC(ch) && IS_PERFORMING(ch) && has_bard_symphonic_resonance(ch))
{
  if (spell_info[CASTING_SPELLNUM(ch)].schoolOfMagic == ENCHANTMENT ||
      spell_info[CASTING_SPELLNUM(ch)].schoolOfMagic == ILLUSION)
  {
    // Daze all enemies within 20 feet
    int range = 20;
    for (tch = world[IN_ROOM(ch)].people; tch; tch = tch->next_in_room)
    {
      if (tch == ch || IS_NPC(tch))
        continue;
      if (!GROUP(ch) || GROUP(ch) != GROUP(tch))
      {
        // Apply daze
        struct affected_type daze_af;
        new_affect(&daze_af);
        daze_af.spell = PERK_BARD_SYMPHONIC_RESONANCE;
        daze_af.duration = 1; // 1 round
        SET_BIT_AR(daze_af.bitvector, AFF_DAZED);
        affect_to_char(tch, &daze_af);
      }
    }
  }
}
```

#### Testing

```c
// Test: During Symphonic Resonance performance, check temp HP growth
// Expected: +1d6 temp HP per round, capped at 180
// Test: Cast Enchantment spell during performance
// Expected: All enemies in 20 feet get dazed for 1 round
```

---

### 4. ENDLESS REFRAIN (1115)

**Prerequisites:** Sustaining Melody (Tier 3)  
**Cost:** 5 points  
**Max Ranks:** 1  
**Category:** Spellsinger (30)

#### Mechanics

This perk fundamentally changes how bardic performance works:

1. **Free Performance** - performance costs no actions, moves, or resources
   - Function: `should_endless_refrain_consume_performance()` returns FALSE
   - Integration: In `pulse_bardic_performance()`, skip the cost check
   - Normal performance would consume 1 round; Endless Refrain doesn't

2. **Spell Slot Regeneration** - 1 spell slot regenerated per round
   - Function: `get_bard_endless_refrain_slot_regen()` returns 1
   - Integration: In periodic update, grant 1 spell slot per round while performing
   - Checks class level to determine which circle to refund

3. **Indefinite Duration** - songs last until manually stopped
   - Integration: Modify performance auto-stop logic to never trigger
   - Check `has_bard_endless_refrain()` before stopping performance

#### Performance Cost Override

```c
// In pulse_bardic_performance():
if (!should_endless_refrain_consume_performance(ch))
{
  // Skip all performance cost checks
  // Song continues indefinitely
  continue; // Don't stop performance
}
```

#### Spell Slot Regeneration

```c
// In periodic update (pulse_bardic_performance or limits.c):
if (has_bard_endless_refrain(ch) && IS_PERFORMING(ch))
{
  int class_level = CLASS_LEVEL(ch, CLASS_BARD);
  int circle = MIN(6, class_level / 2); // Cap at 6th circle
  
  // Regenerate 1 spell slot of appropriate circle
  if (circle > 0 && circle <= NUM_CIRCLES)
  {
    ch->spells_prepared[CLASS_BARD][circle]++; // Restore a slot
    send_to_char(ch, "\tCYour song refills your magical reserves.\tn\r\n");
  }
}
```

#### Song Duration Override

```c
// In bardic_performance_engine() or check_performance_expiration():
if (has_bard_endless_refrain(ch) && IS_PERFORMING(ch))
{
  // Never stop the performance
  return TRUE; // Continue indefinitely
}
```

#### Testing

```c
// Test: Start performance with Endless Refrain
// Expected: No action/move cost; song never auto-stops
// Test: While performing Endless Refrain song for 100 rounds
// Expected: 100 spell slots regenerated (1 per round)
// Expected: Can perform indefinitely without resource drain
```

---

## Integration Summary

### Files to Modify

1. **perks.c** ✅
   - Define 4 perks in `define_bard_perks()`
   - Add 15 helper functions

2. **perks.h** ✅
   - Declare 15 helper functions

3. **structs.h** ✅
   - Add 4 perk IDs (1112-1115)

4. **spell_parser.c** (Integration Pending)
   - Modify `finishCasting()` to apply Symphonic Resonance daze
   - Check Spellsong Maestra metamagic costs
   - Add Heightened Harmony affect (already done for Tier 3)

5. **bardic_performance.c** (Integration Pending)
   - Modify `pulse_bardic_performance()` for:
     - Endless Refrain spell slot regen
     - Symphonic Resonance temp HP
     - Performance cost override

6. **magic.c** (Integration Pending)
   - Add Aria of Stasis +4 save bonus in `savingthrow_full()`
   - Add Spellsong Maestra +2 DC bonus

7. **fight.c** (Integration Pending)
   - Add Aria of Stasis -2 to-hit penalty in `compute_tohit()`

8. **limits.c** or similar (Integration Pending)
   - Alternative location for periodic updates if not in bardic_performance.c

---

## Code Locations Reference

| Function | File | Lines | Purpose |
|----------|------|-------|---------|
| `define_bard_perks()` | perks.c | ~4500 | Perk initialization |
| Helper functions | perks.c | ~13500+ | Getter/checker functions |
| `finishCasting()` | spell_parser.c | ~1650 | Spell casting completion |
| `pulse_bardic_performance()` | bardic_performance.c | ~1200 | Periodic performance tick |
| `savingthrow_full()` | magic.c | ~390 | Saving throw calculation |
| `compute_tohit()` | fight.c | ~10600+ | Attack roll calculation |

---

## Testing Checklist

### Spellsong Maestra
- [ ] Cast bard spell while performing: +2 caster level
- [ ] Check spell DC while performing: +2 bonus  
- [ ] Cast bard spell with metamagic: no circle increase
- [ ] Stop performing: bonuses disappear
- [ ] Non-bard spells: no bonus applied

### Aria of Stasis  
- [ ] Start performing with Aria active
- [ ] Check allied saves: +4 bonus
- [ ] Apply slow to allied character: immunity active
- [ ] Enemy attacks allies: -2 to their roll
- [ ] Enemy movement: 10% speed reduction
- [ ] Stop performing: all bonuses fade

### Symphonic Resonance
- [ ] Perform for 5 rounds: 5d6 temp HP accumulated
- [ ] Cap at 180 total temp HP (30 rounds)
- [ ] Cast Enchantment spell during performance: enemies dazed in 20 ft
- [ ] Cast Illusion spell during performance: enemies dazed in 20 ft
- [ ] Cast Evocation spell during performance: no daze effect
- [ ] Daze lasts 1 round, then expires

### Endless Refrain
- [ ] Start performance: no action cost
- [ ] Perform for 10 rounds: still performing without interruption
- [ ] Manually stop performance: stops as normal
- [ ] Check spell slots after 10 rounds of Endless Refrain: +10 slots
- [ ] Non-bard performance: normal costs apply
- [ ] Return to normal performance: costs resume

---

## Balance Notes

- **Spellsong Maestra:** Powerful but requires other Tier 3 perks. +2 caster and DC are significant but not game-breaking
- **Aria of Stasis:** Defensive capstone. +4 saves is major, but requires maintaining song
- **Symphonic Resonance:** Utility capstone. 1d6 temp HP/round = 6 average, 180 max is reasonable for sustained buff
- **Endless Refrain:** Resource generation capstone. 1 spell slot/round encourages performance usage

All perks scale with bard investment and require active song maintenance.

---

## Future Enhancements

- Consider Tier 5 capstone beyond these (would need new perk IDs 1116-1119)
- Integrate with Warchanter and Swashbuckler Tier 4 perks when implemented
- Balance adjustments based on player feedback after testing
