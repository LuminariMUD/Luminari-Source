# Wizard/Sorcerer AI Enhancement - Group Buffs & AoE Spells

## Overview
Enhanced wizard and sorcerer mob AI to intelligently use group buff spells when allies are present and wizard-specific AoE spells when fighting multiple enemies.

## Changes Made

### 1. New Spell Arrays

**Location**: `src/mob_spells.c` + `src/mob_spells.h`

#### Group Buff Spells (10 spells)
```c
int wizard_group_buff_spells[WIZARD_GROUP_BUFFS] = {
    SPELL_MASS_WISDOM,        // Mass stat buffs
    SPELL_MASS_CHARISMA,
    SPELL_MASS_CUNNING,
    SPELL_MASS_STRENGTH,
    SPELL_MASS_GRACE,
    SPELL_MASS_ENDURANCE,
    SPELL_MASS_HASTE,         // Mass combat buffs
    SPELL_MASS_FLY,
    SPELL_MASS_INVISIBILITY,
    SPELL_CIRCLE_A_EVIL       // Protection circles (MAG_GROUPS)
};
```

These spells use the `MAG_GROUPS` routine, meaning they affect all group members (or allied mobs using our new `are_grouped()` logic).

#### Wizard AoE Spells (15 spells)
```c
int wizard_aoe_spells[WIZARD_AOE_SPELLS] = {
    SPELL_ICE_STORM,          // Evocation damage
    SPELL_METEOR_SWARM,
    SPELL_CHAIN_LIGHTNING,
    SPELL_PRISMATIC_SPRAY,
    SPELL_INCENDIARY_CLOUD,
    SPELL_HORRID_WILTING,
    SPELL_WAIL_OF_THE_BANSHEE,
    SPELL_MASS_HOLD_PERSON,   // Crowd control
    SPELL_WAVES_OF_EXHAUSTION,
    SPELL_WAVES_OF_FATIGUE,
    SPELL_THUNDERCLAP,
    SPELL_COLOR_SPRAY,
    SPELL_BURNING_HANDS,
    SPELL_FIREBALL,           // Classic AoE
    SPELL_LIGHTNING_BOLT
};
```

These spells use the `MAG_AREAS` routine for area-of-effect damage and debuffs, specifically curated for wizard/sorcerer spell lists.

### 2. Enhanced `wizard_cast_prebuff()` Function

**New Behavior**:

1. **Ally Detection**: Checks for allies in two ways:
   - Formal groups: `GROUP(ch) && GROUP(ch)->members->iSize > 1`
   - Allied mobs: Uses `are_grouped(ch, tch)` to find followers/packmates

2. **Group Buff Priority**: 30% chance to cast group buff if allies present
   - Randomly selects from `wizard_group_buff_spells[]`
   - Checks spell level requirements
   - Avoids recasting if already affected
   - Respects spell slot conservation (50% threshold)

3. **Fallback to Self-Buffs**: If no group buff cast, uses standard long-duration buff logic

**Example Scenario**:
```
Wizard Mob (level 18)
  ├─ Apprentice 1 (following, level 12)
  ├─ Apprentice 2 (following, level 12)
  └─ Familiar (summoned)
```

The wizard will:
- 30% chance: Cast `SPELL_MASS_HASTE` on entire group
- 70% chance: Cast personal buff like `SPELL_STONESKIN`

### 3. Enhanced `wizard_combat_ai()` Function

**New Behavior**:

**AoE Spell Selection**: When `use_aoe >= 2` (fighting 2+ enemies)
- Uses `wizard_aoe_spells[]` instead of generic `valid_aoe_spell[]`
- Only checks level requirements (simplified logic)
- Wizard-specific spells appropriate for arcane casters

**Example Combat**:
```
Sorcerer Mob vs. 3 Goblins
```

Before:
- Might cast `SPELL_EARTHQUAKE` (druid spell)
- Might cast `SPELL_CALL_LIGHTNING_STORM` (druid spell)
- Generic AoE selection

After:
- Casts `SPELL_METEOR_SWARM` (wizard spell)
- Casts `SPELL_CHAIN_LIGHTNING` (wizard spell)
- Wizard-appropriate AoE spells

### 4. Header Updates

**src/mob_spells.h**:
```c
#define WIZARD_GROUP_BUFFS 10
#define WIZARD_AOE_SPELLS 15

extern int wizard_group_buff_spells[WIZARD_GROUP_BUFFS];
extern int wizard_aoe_spells[WIZARD_AOE_SPELLS];
```

## Technical Details

### Group Spell Mechanics (MAG_GROUPS)

From `spell_parser.c`:
```c
if (IS_SET(SINFO.routines, MAG_GROUPS))
  mag_groups(spell_level, caster, ovict, spellnum, savetype, casttype);
```

Group spells automatically affect:
- All members of caster's formal GROUP()
- With our new `are_grouped()` function, also allied mobs in same room

### AoE Spell Mechanics (MAG_AREAS)

From `spell_parser.c`:
```c
if (IS_SET(SINFO.routines, MAG_AREAS))
  mag_areas(spell_level, caster, ovict, spellnum, metamagic, savetype, casttype);
```

Area spells hit:
- All valid targets in the room
- Typically hostile targets only
- Subject to saving throws

## Strategic Benefits

### Group Buff Usage

**Before**:
- Wizard casts `SPELL_HASTE` on self only
- Allies fight un-hasted
- Inefficient spell slot usage

**After**:
- Wizard casts `SPELL_MASS_HASTE` on entire group
- All allies benefit from haste
- More efficient, stronger group combat

### AoE Spell Usage

**Before**:
- Wizard might cast cleric/druid AoE spells they shouldn't know
- Generic spell selection across all caster classes

**After**:
- Wizard uses wizard-appropriate AoE spells
- More thematic and lore-appropriate
- Better spell variety

## Spell Examples by Type

### Mass Stat Buffs
- **SPELL_MASS_STRENGTH**: +4 STR to entire group (level 5 spell)
- **SPELL_MASS_GRACE**: +4 DEX to entire group (level 5 spell)
- **SPELL_MASS_CUNNING**: +4 INT to entire group (level 5 spell)

### Mass Combat Buffs
- **SPELL_MASS_HASTE**: Extra attack, AC bonus for group (level 8 spell)
- **SPELL_MASS_FLY**: Flight for entire group (level 7 spell)
- **SPELL_MASS_INVISIBILITY**: Invisibility for group (level 7 spell)

### Wizard AoE Damage
- **SPELL_FIREBALL**: Classic 3rd level AoE
- **SPELL_ICE_STORM**: 4th level cold damage
- **SPELL_CHAIN_LIGHTNING**: 6th level lightning damage
- **SPELL_METEOR_SWARM**: 9th level massive fire damage

### Wizard AoE Control
- **SPELL_MASS_HOLD_PERSON**: Paralyze multiple humanoids (level 7)
- **SPELL_WAVES_OF_EXHAUSTION**: Exhaust multiple enemies (level 7)
- **SPELL_COLOR_SPRAY**: Stun/blind low-level enemies (level 1)

## Integration with `are_grouped()`

The group buff logic works seamlessly with our previously implemented `are_grouped()` function:

```c
/* Check for allied mobs in the room */
for (tch = world[IN_ROOM(ch)].people; tch; tch = tch->next_in_room)
{
  if (tch != ch && are_grouped(ch, tch))
  {
    has_allies = TRUE;
    break;
  }
}
```

This means:
- Mobs following each other count as allies
- Pack leaders will buff their pack
- Summoned creatures trigger group buffs
- Works without formal GROUP() structure

## Combat Example: Wizard Pack

```
Encounter Setup:
- Dark Wizard (level 20, wizard class)
  ├─ Apprentice Wizard 1 (level 12, following)
  ├─ Apprentice Wizard 2 (level 12, following)
  └─ Summoned Air Elemental (level 16, following)

vs.

- Party of 4 adventurers (level 15)
```

**Combat Flow**:

**Round 1 (Pre-combat)**:
- Dark Wizard uses `wizard_cast_prebuff()`
- Detects 3 allies in room
- 30% roll succeeds → casts `SPELL_MASS_HASTE`
- **All 4 mobs now hasted**

**Round 2**:
- Dark Wizard enters combat with party
- `use_aoe = 4` (4 party members)
- Casts `SPELL_METEOR_SWARM` (from wizard_aoe_spells)
- **All 4 party members take massive fire damage**

**Round 3**:
- Apprentice 1 is injured (50% HP)
- Dark Wizard's turn: 20% chance for `npc_spellup()`
- Rolls succeeds → calls `npc_spellup()`
- Detects Apprentice 1 needs healing
- Casts `SPELL_HEAL` on Apprentice 1

**Round 4**:
- Still fighting 4 enemies
- Casts `SPELL_CHAIN_LIGHTNING` (wizard AoE)
- **Lightning chains through party**

## Performance Considerations

### Memory Impact
- Added 2 static arrays: (10 + 15) × 4 bytes = 100 bytes
- Negligible memory footprint

### CPU Impact
- Ally detection: O(n) where n = room occupants (typically 3-8)
- Spell selection: O(1) random array access
- No performance concerns

### Spell Slot Conservation
- Still respects 50% threshold from previous implementation
- Won't spam group buffs if low on slots
- Prioritizes combat effectiveness

## Future Enhancements

### Short Term
- Add similar lists for other caster classes (cleric, druid)
- Implement smart buff selection (missing buffs first)
- Add buff priority based on combat situation

### Long Term
- Metamagic integration (maximize/empower AoE spells)
- Tactical positioning for AoE spell avoidance
- Coordinated casting between multiple wizard mobs

## Testing Checklist

- [ ] Wizard with followers casts group buffs
- [ ] Wizard vs. 2+ enemies uses wizard AoE spells
- [ ] Sorcerer gets same behavior as wizard
- [ ] Spell level requirements respected
- [ ] Spell slot conservation still works
- [ ] Group buffs don't recast on already-buffed targets
- [ ] AoE spells chosen from wizard list only
- [ ] Non-wizard classes unaffected

## Files Modified

1. **src/mob_spells.h**
   - Added `WIZARD_GROUP_BUFFS` define (10)
   - Added `WIZARD_AOE_SPELLS` define (15)
   - Added extern declarations for new arrays

2. **src/mob_spells.c**
   - Added `wizard_group_buff_spells[]` array (10 spells)
   - Added `wizard_aoe_spells[]` array (15 spells)
   - Enhanced `wizard_cast_prebuff()` with ally detection and group buff logic
   - Enhanced `wizard_combat_ai()` to use wizard-specific AoE spells

## Compilation Status
✅ Compiles cleanly with no errors or warnings
✅ New arrays properly declared and defined
✅ Functions properly updated

## Author Notes
This enhancement makes wizard/sorcerer mobs much more dangerous and tactically interesting:
- They support their allies with powerful group buffs
- They use appropriate arcane AoE spells against multiple foes
- Works seamlessly with the `are_grouped()` function for pack behavior
- Maintains spell slot conservation and level requirements

Next steps: Implement similar enhancements for clerics (group healing focus), druids (nature AoE), and other caster classes.
