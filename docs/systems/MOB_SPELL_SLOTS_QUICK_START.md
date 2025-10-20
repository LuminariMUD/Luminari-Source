# Mob Spell Slot System - Quick Start Guide

## For Builders

### ⚠️ IMPORTANT: Spell Slots Are Now DEFAULT

**All mobs now use spell slots automatically!** You don't need to do anything.

### Giving a Mob Unlimited Spell Slots (Opt-Out)

If you want a special mob to have unlimited casting (like a powerful deity or immortal):

1. **Edit the mob:**
   ```
   medit <vnum>
   ```

2. **Toggle the unlimited spell slot flag:**
   ```
   mob flags
   ```
   Look for and toggle: **Unlimited-Spell-Slots**

3. **Save the mob:**
   ```
   save internally
   ```

4. **The mob will now:**
   - Cast spells without consuming spell slots (unlimited casting)
   - Bypass the spell slot system entirely

### Default Mob Behavior (No Flag)

All mobs without the Unlimited-Spell-Slots flag will:
- Have spell slots calculated automatically based on class and level
- Use spell slots when casting (one slot per spell)
- Regenerate ONE random spell slot every 2 minutes while NOT in combat
- Be unable to cast spells of a circle when all slots for that circle are depleted

### Viewing Mob Spell Slots (Future Feature)
Once admin commands are implemented:
```
mstat <mob name>
```
Will display current and maximum spell slots per circle.

## For Players

### What This Means
- **All mobs** will now run out of spell slots and be unable to cast
- Mobs regenerate slots slowly while not fighting (1 slot per 2 minutes)
- High-level spells deplete faster since mobs have fewer high-circle slots
- Sustained combat will exhaust mob casters
- Giving mobs time to rest allows them to recover spell slots

### Strategy Tips
1. **Sustained Pressure**: Keep mob casters in combat to prevent regeneration
2. **Force High-Level Spells**: Make them burn their powerful limited-use spells early
3. **Retreat and Re-engage**: Gives YOU time to heal, but also gives MOBS time to regenerate slots
4. **Focus Casters First**: Deplete their spell slots before they regenerate

## For Developers

### Checking if Mob Has Unlimited Spell Slots
```c
if (MOB_FLAGGED(mob, MOB_UNLIMITED_SPELL_SLOTS))
{
    /* This mob has unlimited casting (spell slots disabled) */
}
else
{
    /* This mob uses spell slot system (default) */
}
```

### Manually Checking Slot Availability
```c
if (has_spell_slot(mob, SPELL_FIREBALL))
{
    /* Mob has slots available for this spell */
}
```

### Getting Spell Circle
```c
int circle = get_spell_circle(SPELL_FIREBALL, CLASS_WIZARD);
/* Returns 0-9, or -1 if invalid */
```

### Initialization
Mobs are automatically initialized in `read_mobile()`:
```c
init_mob_spell_slots(mob);
```

### Regeneration
Automatically called in `mobile_activity()`:
```c
regenerate_mob_spell_slot(mob);
```

## Slot Allocation Examples

### Level 10 Wizard (Full Caster)
- Circle 0 (Cantrips): 100 (unlimited)
- Circle 1: 4 slots
- Circle 2: 3 slots
- Circle 3: 3 slots
- Circle 4: 2 slots
- Circle 5: 1 slot
- Circle 6-9: 0 slots

### Level 20 Cleric (Full Caster)
- Circle 0: 100 (unlimited)
- Circle 1-5: 4 slots each
- Circle 6-7: 3 slots each
- Circle 8-9: 2 slots each

### Level 10 Paladin (Half Caster)
- Circle 0: 100 (unlimited)
- Circle 1: 2 slots
- Circle 2: 1 slot
- Circle 3-9: 0 slots

## Regeneration Behavior

### When Regeneration Occurs
- ✅ Mob does NOT have MOB_UNLIMITED_SPELL_SLOTS flag
- ✅ Mob is NOT in combat
- ✅ 120 seconds (2 minutes) have passed since last regeneration
- ✅ At least one circle has less than maximum slots

### When Regeneration Does NOT Occur
- ❌ Mob has MOB_UNLIMITED_SPELL_SLOTS flag (unlimited casting)
- ❌ Mob is in combat (FIGHTING)
- ❌ Less than 120 seconds (2 minutes) since last regeneration
- ❌ All circles are at maximum

### What Gets Regenerated
- ONE random circle that has depleted slots
- ONE slot added to that circle
- Higher circles and lower circles have equal chance

## Testing Checklist

### Basic Functionality
- [ ] Mob (default) can cast spells
- [ ] Mob depletes slots when casting
- [ ] Mob cannot cast when circle depleted
- [ ] Mob regenerates slots out of combat (every 2 minutes)
- [ ] Mob does NOT regenerate in combat
- [ ] Mob with MOB_UNLIMITED_SPELL_SLOTS flag casts unlimited

### Edge Cases
- [ ] Cantrips (circle 0) effectively unlimited
- [ ] Multi-circle depletion
- [ ] Rapid successive casts
- [ ] Long combat (no regeneration)
- [ ] Combat ends (regeneration resumes)
- [ ] Mob death/reload resets slots

### Performance
- [ ] No lag during slot checks
- [ ] No lag during regeneration
- [ ] Multiple spell-slot mobs in same zone
- [ ] Heavy combat scenarios

## Troubleshooting

### Mob Casts Unlimited Spells
**Problem**: Mob ignores spell slots
**Solution**: Check if MOB_UNLIMITED_SPELL_SLOTS flag is set via `medit <vnum>`, "mob flags" - remove it to enable spell slots

### Mob Never Regenerates
**Problem**: Slots stay at 0 forever
**Check**:
1. Is mob in constant combat? (No regen during combat)
2. Is mob awake? (No regen while sleeping)
3. Has 2 minutes passed? (Regen takes time)
4. Does mob have MOB_UNLIMITED_SPELL_SLOTS flag? (Won't show regeneration)

### Mob Has Wrong Number of Slots
**Problem**: Slots don't match expected values
**Check**:
1. Mob's class (GET_CLASS)
2. Mob's level (GET_LEVEL)
3. Circle calculation based on class type

### Compilation Errors
**Problem**: Code won't compile
**Check**:
1. `src/mob_spellslots.c` in Makefile.am
2. All includes present in modified files
3. No syntax errors in new code

## Examples in Practice

### Encounter 1: Wizard Boss Fight
**Setup**: Level 15 Wizard (default - spell slots enabled)
**Spell Slots**:
- Circle 3 (Fireball, Lightning Bolt): 4 slots
- Circle 5 (Cone of Cold): 3 slots
- Circle 7 (Finger of Death): 2 slots

**Combat Flow**:
1. Wizard opens with high-level spells
2. After 4 fireballs, no more circle 3 spells
3. Falls back to lower circles or melee
4. If players retreat for 2-4 minutes, wizard regains 1-2 random slots
5. Players must finish quickly or face regenerated slots

### Encounter 2: Lich Battle
**Setup**: Level 25 Lich (default - spell slots enabled, or optionally with MOB_UNLIMITED_SPELL_SLOTS for epic boss)
**Spell Slots**: Full allocation across all 10 circles

**Combat Flow**:
1. Lich has substantial spell resources
2. Extended fight depletes even high-level undead
3. Strategic spell use by lich (AI permitting)
4. No regeneration during constant combat pressure
5. Defeat requires sustained assault

### Encounter 3: Cleric Healer
**Setup**: Level 12 Cleric (default - spell slots enabled)
**Spell Slots**:
- Circle 2 (Cure Moderate): 3 slots
- Circle 3 (Cure Serious): 3 slots
- Circle 4 (Cure Critical): 2 slots

**Combat Flow**:
1. Cleric heals allies in combat
2. After 3 serious wounds heals, must use lower circles
3. Eventually runs out of healing slots
4. Combat effectiveness drops dramatically
5. Allows players to wear down enemy group

## Future Features (Planned)

### Admin Commands
```
mstat <mob>
```
Will show spell slot information

### Variable Regen Rates
- Fast regen: 60 second intervals (faster than current 2 minutes)
- Slow regen: 300 second intervals (5 minutes)
- Type-based: Liches regen slower, nature spirits faster

### Slot Trading
- Use 1 higher circle slot = gain 2 lower circle slots
- Allows emergency casting flexibility

### Mob AI Improvements
- Conserve high-level slots
- Retreat when slots low
- Coordinate with other casters

---

**Document Version**: 1.0
**Last Updated**: October 19, 2025
**Status**: System Implemented, Admin Commands Pending
