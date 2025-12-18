# Bard Spellsinger Tier 3 - Quick Reference

## Function Quick Reference

### Master of Motifs
```c
if (has_bard_master_of_motifs(ch)) {
    /* Allow 2 simultaneous songs */
}
```

### Dirge of Dissonance
```c
if (is_song_active(ch) && has_bard_dirge_of_dissonance(ch)) {
    int damage_dice = get_bard_dirge_sonic_damage(ch);  /* Returns 1 for 1d6 */
    int conc_penalty = get_bard_dirge_concentration_penalty(ch);  /* Returns -2 */
    /* Apply to enemies in room */
}
```

### Heightened Harmony
```c
/* On metamagic bard spell cast: */
if (has_bard_heightened_harmony(ch)) {
    /* Apply 60-second buff with PERK_BARD_HEIGHTENED_HARMONY as spell ID */
}

/* When calculating perform skill: */
bonus += get_bard_heightened_harmony_perform_bonus(ch);  /* +5 if active */
```

### Protective Chorus
```c
/* For saves vs. spells: */
if (is_under_bard_song(victim) && has_bard_protective_chorus(bard)) {
    save_bonus += get_bard_protective_chorus_save_bonus(bard);  /* +2 */
}

/* For AC vs. attacks of opportunity: */
if (is_aoo && is_under_bard_song(victim) && has_bard_protective_chorus(bard)) {
    ac_bonus += get_bard_protective_chorus_ac_bonus(bard);  /* +2 */
}
```

## Perk IDs
```c
PERK_BARD_MASTER_OF_MOTIFS      1108
PERK_BARD_DIRGE_OF_DISSONANCE   1109
PERK_BARD_HEIGHTENED_HARMONY    1110
PERK_BARD_PROTECTIVE_CHORUS     1111
```

## All Functions
```c
/* Master of Motifs */
bool has_bard_master_of_motifs(struct char_data *ch);

/* Dirge of Dissonance */
bool has_bard_dirge_of_dissonance(struct char_data *ch);
int get_bard_dirge_sonic_damage(struct char_data *ch);
int get_bard_dirge_concentration_penalty(struct char_data *ch);

/* Heightened Harmony */
bool has_bard_heightened_harmony(struct char_data *ch);
int get_bard_heightened_harmony_perform_bonus(struct char_data *ch);

/* Protective Chorus */
bool has_bard_protective_chorus(struct char_data *ch);
int get_bard_protective_chorus_save_bonus(struct char_data *ch);
int get_bard_protective_chorus_ac_bonus(struct char_data *ch);
```

## Integration Locations

| System | File(s) | What to Add |
|--------|---------|-------------|
| Bardic Performance | `bardic_performance.c` | Check Master of Motifs for dual songs |
| Combat Round | `fight.c` | Apply Dirge damage/penalties each round |
| Spell Casting | `magic.c`, `spell_parser.c` | Trigger Heightened Harmony on metamagic |
| Saving Throws | `magic.c` | Add Protective Chorus bonus |
| AC Calculation | `fight.c`, `combat.c` | Add Protective Chorus AC vs. AoO |
| Skill Checks | `skills.c` | Add Heightened Harmony perform bonus |

## Return Values

| Function | Active | Inactive/NPC |
|----------|--------|--------------|
| `has_bard_*` | TRUE | FALSE |
| `get_bard_dirge_sonic_damage` | 1 | 0 |
| `get_bard_dirge_concentration_penalty` | -2 | 0 |
| `get_bard_heightened_harmony_perform_bonus` | 5 | 0 |
| `get_bard_protective_chorus_save_bonus` | 2 | 0 |
| `get_bard_protective_chorus_ac_bonus` | 2 | 0 |
