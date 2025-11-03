# Knight of Solamnia - Spell Progression System

## Overview

The Knight of Solamnia uses **SPELL PROGRESSION ADVANCEMENT** (not independent spell slots).

This is the same mechanic used by:
- Sacred Fist (advances divine casting every level)
- Mystic Theurge (advances both divine and arcane)
- Other prestige classes that enhance existing spellcasting

## How It Works

### Prerequisites
To take Knight of Solamnia, you must already be a divine spellcaster (Cleric, Druid, Inquisitor, etc.) with at least 1st level spells.

### Spell Progression by Level

**Levels 1-5 (Crown Phase):**
- Pure martial training
- **NO** spell progression advancement
- Focus on combat feats and knightly virtues

**Levels 6-20 (Sword & Rose Phase):**
- **EVERY level** adds +1 to your divine caster level
- You cast spells as if you had continued advancing in your divine class
- Total advancement: +15 caster levels (from level 6 to level 20)

## Examples

### Example 1: Pure Path
- Cleric 5 → Take Knight of Solamnia
- Cleric 5 / Knight 1 = 5th level caster (Crown training, no advancement)
- Cleric 5 / Knight 5 = 5th level caster (completed Crown, no spell advancement yet)
- Cleric 5 / Knight 6 = 6th level caster (Sword begins, spells advance!)
- Cleric 5 / Knight 10 = 10th level caster (Sword complete, +5 levels)
- Cleric 5 / Knight 20 = 20th level caster (Rose complete, +15 levels total)

### Example 2: Multiclass Divine Caster
- Cleric 3 / Druid 2 → Take Knight of Solamnia
- Cleric 3 / Druid 2 / Knight 6 = 6th level divine caster (advances both classes)
- Cleric 3 / Druid 2 / Knight 20 = 20th level divine caster

### Example 3: With Other Prestige Classes
- Cleric 10 / Knight 10 = 15th level caster (10 + 5 from Knight levels 6-10)
- Cleric 10 / Sacred Fist 5 / Knight 15 = 25th level caster
  * Cleric 10 = 10 levels
  * Sacred Fist 5 = +5 levels (full advancement)
  * Knight 1-5 = +0 levels (Crown only)
  * Knight 6-15 = +10 levels (Sword 6-10 + Rose 11-15)

## Spell Slots Gained

The Knight does NOT have its own spell slot table. Instead:

1. Your base divine class determines spell slots (Cleric, Druid, etc.)
2. Knight levels 6-20 increase your effective caster level
3. You gain spell slots as if you had leveled up in your base class

**Example:**
- Cleric 5 normally gets: 5/4/3/2 slots (1st/2nd/3rd/4th circle)
- Cleric 5 / Knight 6 (= 6th level caster) gets: 5/4/3/2/1 slots (adds 5th circle!)
- Cleric 5 / Knight 10 (= 10th level caster) gets: 5/4/4/3/3/2 slots

## Spell Access

You can only cast spells from your base divine class spell list(s).

- Cleric/Knight → Cleric spells only
- Druid/Knight → Druid spells only
- Inquisitor/Knight → Inquisitor spells only
- Cleric/Druid/Knight → Both Cleric and Druid spells

The Knight class itself does NOT grant additional spells. It just makes you better at casting the spells you already know.

## Code Implementation

### In `src/utils.c` - `compute_divine_level()`:
```c
divine_level += MAX(0, CLASS_LEVEL(ch, CLASS_KNIGHT_OF_SOLAMNIA) - 5);
```

This formula:
- Knight level 1-5: MAX(0, 1-5) = 0 (no advancement)
- Knight level 6: MAX(0, 6-5) = 1 (adds 1 caster level)
- Knight level 7: MAX(0, 7-5) = 2 (adds 2 caster levels)
- Knight level 20: MAX(0, 20-5) = 15 (adds 15 caster levels)

### In `src/class.c` - Class Definition:
```c
/*prestige spell progression*/ "divine advancement after level 5",
```

This description tells players how the spell progression works.

## Comparison to Paladin

**Paladin:**
- Has its own spell slot table
- Starts casting at level 4 (using paladin_slots array)
- Gets paladin-specific spell list
- Half-caster (max 4th circle spells)

**Knight of Solamnia:**
- NO independent spell slots
- Advances existing divine caster class starting at level 6
- Uses base class spell list
- Full caster advancement (can reach 9th circle via base class + advancement)

## Balance Notes

The 5-level delay (no spell advancement until level 6) is intentional:

1. **Flavor**: Crown knights are pure warriors, not spellcasters
2. **Balance**: Prevents easy "dip" for quick spell progression
3. **Investment**: Rewards players who commit to the full prestige path
4. **Tradeoff**: Those 5 Crown levels give strong combat feats instead of spells

A character who takes Knight early (e.g., Cleric 5 / Knight 20) will have:
- Strong combat abilities (20 Knight levels of feats)
- 20th level divine casting (5 base + 15 from Knight 6-20)

A character who delays (e.g., Cleric 15 / Knight 10) will have:
- Strong divine casting baseline
- 20th level divine casting (15 base + 5 from Knight 6-10)
- Fewer Knight-specific combat feats (only 10 Knight levels)

## Summary

**Knight of Solamnia = DIVINE CASTER LEVEL BOOSTER**

- Not an independent spellcaster
- Requires existing divine casting
- Levels 1-5: Combat only
- Levels 6-20: +1 caster level per Knight level
- Uses your base class spell slots and spell list
- Makes you a better divine caster while maintaining martial prowess
