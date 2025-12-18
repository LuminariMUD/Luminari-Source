# Bard Spellsinger Tier 3 - Full Implementation Guide

## Overview

This document provides the complete implementation details for the Tier 3 Spellsinger perks for the Bard class. All perks have been fully defined in the codebase with supporting functions.

---

## Tier 3 Perks (Cost: 3 points each)

### 1. Master of Motifs

**Description:** Maintain up to two distinct bard songs simultaneously (shared performance pool).

**Game Mechanics:**
- Allows the bard to have two different bardic performance effects active at once
- Both songs consume from the same performance round pool
- Only distinct songs can be maintained (can't have Inspire Courage twice)
- Resource consumption rate may increase when dual-singing

**Implementation Details:**
- **Perk ID:** `PERK_BARD_MASTER_OF_MOTIFS` (1108)
- **Prerequisites:** 
  - Sustaining Melody (1 rank)
  - Songweaver II (1 rank)
- **Associated Class:** CLASS_BARD
- **Category:** PERK_CATEGORY_SPELLSINGER
- **Max Rank:** 1
- **Cost:** 3 points

**Functions:**
```c
bool has_bard_master_of_motifs(struct char_data *ch);
```

**Integration Points:**
1. **Bardic Performance System:** Check `has_bard_master_of_motifs()` when starting a new song
2. **Song Tracking:** Modify song tracking to allow 2 active performance IDs instead of 1
3. **Performance Pool:** Ensure both songs consume from the same resource pool
4. **Song Stacking Rules:** Prevent duplicate songs, ensure effects stack appropriately

**Example Usage:**
```c
if (has_bard_master_of_motifs(ch)) {
    /* Allow starting second song if not duplicate */
    if (GET_ACTIVE_SONG_COUNT(ch) < 2 && !is_duplicate_song(ch, song_id)) {
        start_bardic_performance(ch, song_id);
    }
}
```

---

### 2. Dirge of Dissonance

**Description:** Enemies in the room take 1d6 sonic damage per round and -2 penalty to concentration checks while your song persists.

**Game Mechanics:**
- Passive area effect while any bard song is active
- Affects all enemies in the same room as the bard
- 1d6 sonic damage per combat round
- -2 penalty to concentration checks for affected enemies
- Respects sonic resistance/immunity

**Implementation Details:**
- **Perk ID:** `PERK_BARD_DIRGE_OF_DISSONANCE` (1109)
- **Prerequisites:** Crescendo (1 rank)
- **Associated Class:** CLASS_BARD
- **Category:** PERK_CATEGORY_SPELLSINGER
- **Max Rank:** 1
- **Cost:** 3 points
- **Effect Value:** 6 (damage die size)
- **Effect Modifier:** -2 (concentration penalty)

**Functions:**
```c
bool has_bard_dirge_of_dissonance(struct char_data *ch);
int get_bard_dirge_sonic_damage(struct char_data *ch);      /* Returns 1 (for 1d6) */
int get_bard_dirge_concentration_penalty(struct char_data *ch); /* Returns -2 */
```

**Integration Points:**
1. **Combat Round Processing:** Add check in combat round handler
2. **Room-wide Effect:** Apply to all enemies in room when song is active
3. **Damage Application:** Roll 1d6 sonic damage per round
4. **Concentration System:** Apply penalty to spellcasting concentration checks
5. **Resistance/Immunity:** Check for sonic resistance/immunity before applying damage

**Example Usage:**
```c
/* In combat round processing */
if (is_bardic_performance_active(ch) && has_bard_dirge_of_dissonance(ch)) {
    for (each enemy in room) {
        if (!check_sonic_immunity(enemy)) {
            int damage = dice(get_bard_dirge_sonic_damage(ch), 6);
            damage_character(enemy, ch, damage, DAMAGE_TYPE_SONIC);
        }
        
        /* Apply concentration penalty */
        int penalty = get_bard_dirge_concentration_penalty(ch);
        apply_concentration_modifier(enemy, penalty);
    }
}
```

---

### 3. Heightened Harmony

**Description:** When you spend metamagic on a bard spell, you gain +5 to your perform skill for one minute.

**Game Mechanics:**
- Triggers when casting a bard spell with any metamagic feat applied
- Grants +5 circumstance bonus to Perform skill
- Duration: 60 seconds (1 minute)
- Stacks with other perform bonuses
- Multiple metamagic casts refresh the duration

**Implementation Details:**
- **Perk ID:** `PERK_BARD_HEIGHTENED_HARMONY` (1110)
- **Prerequisites:** Enchanter's Guile II (1 rank)
- **Associated Class:** CLASS_BARD
- **Category:** PERK_CATEGORY_SPELLSINGER
- **Max Rank:** 1
- **Cost:** 3 points
- **Effect Value:** 5 (perform bonus)
- **Effect Modifier:** 60 (duration in seconds)

**Functions:**
```c
bool has_bard_heightened_harmony(struct char_data *ch);
int get_bard_heightened_harmony_perform_bonus(struct char_data *ch); /* Returns +5 if active */
```

**Integration Points:**
1. **Spell Casting:** Check metamagic usage in spell casting system
2. **Affect Application:** Apply timed affect when triggered
3. **Perform Skill:** Add bonus to perform skill calculations
4. **Duration Tracking:** Use standard affect system with perk ID as spell number

**Example Usage:**
```c
/* In spell casting function */
if (is_bard_spell(spellnum) && used_metamagic && has_bard_heightened_harmony(ch)) {
    struct affected_type af;
    new_affect(&af);
    af.spell = PERK_BARD_HEIGHTENED_HARMONY;
    af.duration = 60; /* 60 seconds */
    af.location = APPLY_NONE;
    af.modifier = 0;
    affect_join(ch, &af, FALSE, FALSE, FALSE, FALSE);
    
    send_to_char(ch, "Your metamagic weaving heightens your harmony!\r\n");
}

/* In perform skill calculation */
if (has_bard_heightened_harmony(ch)) {
    bonus += get_bard_heightened_harmony_perform_bonus(ch);
}
```

---

### 4. Protective Chorus

**Description:** Allies under your song gain +2 to saves vs. spells and +2 to AC vs. attacks of opportunity.

**Game Mechanics:**
- Passive benefit to all allies affected by bard's songs
- +2 bonus to saving throws against spells only
- +2 dodge bonus to AC specifically vs. attacks of opportunity
- Does not apply to saves vs. non-spell effects
- Does not apply to AC vs. regular attacks

**Implementation Details:**
- **Perk ID:** `PERK_BARD_PROTECTIVE_CHORUS` (1111)
- **Prerequisites:** Resonant Voice I (2 ranks)
- **Associated Class:** CLASS_BARD
- **Category:** PERK_CATEGORY_SPELLSINGER
- **Max Rank:** 1
- **Cost:** 3 points
- **Effect Value:** 2 (save bonus)
- **Effect Modifier:** 2 (AC bonus)

**Functions:**
```c
bool has_bard_protective_chorus(struct char_data *ch);
int get_bard_protective_chorus_save_bonus(struct char_data *ch);  /* Returns +2 */
int get_bard_protective_chorus_ac_bonus(struct char_data *ch);    /* Returns +2 */
```

**Integration Points:**
1. **Saving Throw System:** Check when calculating saves vs. spells
2. **AC Calculation:** Check when calculating AC vs. attacks of opportunity
3. **Song Effect Tracking:** Verify target is under bard's song effect
4. **Ally Detection:** Ensure bard and target are friendly

**Example Usage:**
```c
/* In saving throw calculation */
if (save_type == SAVE_TYPE_SPELL && is_under_bard_song(victim)) {
    struct char_data *bard = get_bard_performer(victim);
    if (bard && has_bard_protective_chorus(bard)) {
        save_bonus += get_bard_protective_chorus_save_bonus(bard);
    }
}

/* In AC calculation vs. AoO */
if (attack_type == ATTACK_TYPE_OPPORTUNITY && is_under_bard_song(victim)) {
    struct char_data *bard = get_bard_performer(victim);
    if (bard && has_bard_protective_chorus(bard)) {
        ac_bonus += get_bard_protective_chorus_ac_bonus(bard);
    }
}
```

---

## Integration Checklist

### Bardic Performance System
- [ ] Modify song tracking to support dual songs (Master of Motifs)
- [ ] Add per-round damage processing (Dirge of Dissonance)
- [ ] Implement concentration penalty tracking (Dirge of Dissonance)

### Spell Casting System
- [ ] Add metamagic trigger detection (Heightened Harmony)
- [ ] Apply perform bonus affect on trigger (Heightened Harmony)
- [ ] Check for Crescendo first-spell-after-song bonus (existing Tier 2)

### Combat System
- [ ] Add Dirge damage to combat round processing
- [ ] Check sonic resistance/immunity
- [ ] Apply concentration penalties to enemies

### Saving Throw System
- [ ] Add Protective Chorus bonus to spell saves
- [ ] Verify ally is under bard's song effect

### AC Calculation
- [ ] Add Protective Chorus bonus vs. attacks of opportunity
- [ ] Ensure it only applies to AoO, not regular attacks

### Skill System
- [ ] Add Heightened Harmony bonus to perform skill checks
- [ ] Check for active affect before applying

---

## Testing Guidelines

### Master of Motifs
1. Start first song - should work normally
2. Start second distinct song with perk - should succeed
3. Try starting duplicate song - should fail
4. Verify both songs consume from same performance pool
5. End one song - other should persist
6. Verify without perk, second song attempt fails

### Dirge of Dissonance
1. Start any song with perk active
2. Verify enemies in room take 1d6 sonic per round
3. Check damage log shows "sonic" type
4. Verify enemies with sonic immunity take no damage
5. Cast spell as enemy - verify concentration penalty applied
6. Move to different room - verify damage stops
7. Verify without perk, no damage occurs

### Heightened Harmony
1. Cast bard spell without metamagic - no bonus
2. Cast bard spell with metamagic (e.g., Empower) - verify +5 perform
3. Check affect duration (60 seconds)
4. Cast another metamagic spell - verify duration refreshes
5. Wait for expiration - verify bonus removed
6. Cast non-bard spell with metamagic - verify no bonus

### Protective Chorus
1. Start song affecting ally
2. Ally targeted by spell - verify +2 save bonus
3. Ally targeted by weapon attack - verify no save bonus
4. Ally provokes AoO - verify +2 AC bonus
5. Ally in normal melee - verify no AC bonus
6. Ally not under song - verify no bonuses

---

## Performance Considerations

- **Dirge of Dissonance:** May impact performance with many enemies. Consider limiting checks to once per round.
- **Master of Motifs:** Ensure song effect stacking doesn't create exponential checks.
- **Protective Chorus:** Cache bard performer reference for allies to avoid repeated lookups.

---

## Balance Notes

- **Master of Motifs:** While powerful, limited by shared performance pool and positioning requirements
- **Dirge of Dissonance:** 1d6 per round is significant but manageable at higher levels
- **Heightened Harmony:** Conditional trigger limits power; metamagic slots are resource
- **Protective Chorus:** Specific situational bonuses prevent it from being too strong

---

## Future Enhancements

1. Add visual/audio cues for Dirge of Dissonance damage
2. Create special messaging for Master of Motifs dual-song state
3. Consider adding resist sonic check to Dirge damage
4. Track Heightened Harmony triggers for statistics
5. Add specific AoO messaging when Protective Chorus applies

---

## File Locations

- **Perk Definitions:** `/home/krynn/code/src/structs.h` (lines 3873-3883)
- **Perk Initialization:** `/home/krynn/code/src/perks.c` (define_bard_perks function)
- **Helper Functions:** `/home/krynn/code/src/perks.c` (Tier III functions section)
- **Function Declarations:** `/home/krynn/code/src/perks.h` (Bard functions section)
- **Documentation:** `/home/krynn/code/docs/systems/perks/BARD_PERKS.md`

---

## Revision History

- **Version 1.0** - Initial implementation (December 2025)
  - All four Tier 3 Spellsinger perks fully defined
  - Helper functions implemented
  - Prerequisites properly chained
  - Ready for integration into game systems
