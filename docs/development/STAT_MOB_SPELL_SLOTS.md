# Mob Spell Slot Display in STAT Command

## Overview
Enhanced the `stat` command to automatically display spell slot information for mob spellcasters, making it easier for builders and admins to monitor mob spell resources.

## Change Summary

### Modified Files
- **src/act.wizard.c**
  - Added `#include "mob_spellslots.h"` 
  - Enhanced `do_stat_character()` to display spell slots for mobs

### How It Works

When using the `stat` command on a mob, the system now:

1. **Checks if target is a mob** - Only applies to NPCs
2. **Checks spell slot status** - Skips mobs with `MOB_UNLIMITED_SPELL_SLOTS` flag
3. **Detects spellcasters** - Only shows for mobs with configured spell slots
4. **Displays slot information** - Shows current/max slots per circle

### Display Format

```
Mob Spec-Proc: None, NPC Bare Hand Dam: 2d4

Spell Slots for ancient wizard:
Circle   Current / Maximum
------   -----------------
  0        4 / 4
  1        3 / 4
  2        2 / 3
  3        1 / 2
  4        0 / 1
  5        1 / 1

Last regeneration: 45 seconds ago
```

### Display Logic

The spell slot section only appears when:
- ✅ Target is a mob (`IS_MOB(k)`)
- ✅ Mob doesn't have unlimited slots flag
- ✅ Mob has at least one spell circle configured (max_spell_slots[i] > 0)

### Integration Points

**Uses existing function**: `show_mob_spell_slots(ch, mob)`
- Defined in: `src/mob_spellslots.c`
- Declared in: `src/mob_spellslots.h`
- Output format already perfect for stat display

**Placement in stat output**:
```
[Standard mob info: name, vnum, position, etc.]
Mob Spec-Proc: [spec name], NPC Bare Hand Dam: [damage dice]

[NEW: Spell Slot Display - if applicable]

Carried: weight: [weight], items: [count]
[Rest of stat output...]
```

## Example Usage

### Example 1: Wizard Mob with Spell Slots

```
> stat wizard

...
Mob Spec-Proc: None, NPC Bare Hand Dam: 1d4

Spell Slots for ancient wizard:
Circle   Current / Maximum
------   -----------------
  1        4 / 4
  2        3 / 4
  3        3 / 3
  4        2 / 2
  5        1 / 1
  6        1 / 1
  7        0 / 1

Last regeneration: 120 seconds ago

Carried: weight: 0, items: 0
...
```

**Interpretation**:
- Wizard has spell slots for circles 1-7
- Circle 7 is depleted (0/1)
- Most other circles are full or nearly full
- Last regen was 2 minutes ago

### Example 2: Non-Spellcaster Mob

```
> stat goblin

...
Mob Spec-Proc: None, NPC Bare Hand Dam: 1d6
Carried: weight: 0, items: 0
...
```

**Interpretation**:
- No spell slot section shown (goblin has no spell slots)
- Clean output without unnecessary information

### Example 3: Mob with Unlimited Slots

```
> stat archmage

...
Mob Spec-Proc: None, NPC Bare Hand Dam: 2d6
Carried: weight: 0, items: 0
...
```

**Interpretation**:
- Archmage has `MOB_UNLIMITED_SPELL_SLOTS` flag
- No spell slot tracking needed
- Section not displayed

## Benefits

### For Builders
- **Quick diagnostics**: Instantly see if mob is out of spell slots
- **Balance testing**: Monitor spell slot consumption during combat
- **Configuration check**: Verify spell circles are set up correctly

### For Admins
- **Debugging**: Identify why mobs aren't casting spells (depleted slots)
- **Monitoring**: Check spell slot regeneration is working
- **Balance**: Assess if mobs need more/fewer slots

### For Developers
- **Integration**: Seamlessly uses existing `show_mob_spell_slots()` function
- **Clean code**: Minimal changes, leverages existing infrastructure
- **Maintainable**: Clear separation of concerns

## Technical Details

### Code Addition

**Location**: `src/act.wizard.c`, in `do_stat_character()` function

```c
if (IS_MOB(k))
{
  send_to_char(ch, "\tCMob Spec-Proc: \tn%s\tC, NPC Bare Hand Dam: \tn%d\tCd\tn%d\r\n",
               (mob_index[GET_MOB_RNUM(k)].func ? get_spec_func_name(mob_index[GET_MOB_RNUM(k)].func) : "None"),
               k->mob_specials.damnodice, k->mob_specials.damsizedice);
  
  /* Display mob spell slots if they're a spellcaster */
  if (!MOB_FLAGGED(k, MOB_UNLIMITED_SPELL_SLOTS))
  {
    bool has_slots = FALSE;
    for (i = 0; i < 10; i++)
    {
      if (k->mob_specials.max_spell_slots[i] > 0)
      {
        has_slots = TRUE;
        break;
      }
    }
    
    if (has_slots)
    {
      send_to_char(ch, "\r\n");
      show_mob_spell_slots(ch, k);
    }
  }
}
```

### Performance Impact

**Minimal overhead**:
- Only executes for mob targets
- Simple loop check (max 10 iterations)
- Function call only if slots exist
- O(1) for non-spellcasters

**Memory**: No additional memory used

### Edge Cases Handled

1. **Non-mobs**: Skipped entirely (IS_MOB check)
2. **Unlimited slots**: Skipped via MOB_UNLIMITED_SPELL_SLOTS check
3. **No spell slots**: Skipped if no circles configured
4. **Partially configured**: Shows only configured circles

## Related Systems

**Works with**:
- ✅ Mob spell slot system (`src/mob_spellslots.c`)
- ✅ Wizard AI (`src/mob_spells.c`)
- ✅ Spell slot regeneration (pulse system)
- ✅ Spell casting system

**Does not interfere with**:
- Player stat display (only affects mobs)
- Object stat display
- Room stat display
- Other admin commands

## Future Enhancements

### Short Term
- Add color coding (red for depleted, yellow for low, green for full)
- Show percentage utilization per circle
- Add total slots used/available summary

### Long Term
- Interactive spell slot management (`set mob <name> slots <circle> <amount>`)
- Spell slot history tracking (last N casts)
- Spell casting statistics (most used spells)

## Testing Checklist

- [x] Stat on mob with spell slots shows slot info
- [x] Stat on mob without spell slots shows nothing
- [x] Stat on mob with unlimited slots shows nothing
- [x] Stat on player character unaffected
- [x] Compilation clean (no errors/warnings)
- [x] Display format consistent with existing stat output
- [x] All spell circles display correctly

## Compilation Status

✅ **COMPLETE** - Compiled cleanly with no errors or warnings

## Files Modified

1. **src/act.wizard.c** (2 changes)
   - Added include: `mob_spellslots.h`
   - Enhanced `do_stat_character()` with spell slot display

## Author Notes

This enhancement makes debugging and monitoring mob spellcasters much easier. Builders can now quickly see:
- Why a wizard stopped casting (out of slots)
- If slot regeneration is working
- Whether spell slot configuration is correct

The implementation is clean, efficient, and leverages existing code. The display only appears when relevant, keeping stat output clean for non-spellcaster mobs.
