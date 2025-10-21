# Wizard Mob Buffing Troubleshooting Guide

## Issue Report
Wizard mobs don't seem to be buffing themselves.

## Root Causes & Solutions

### 1. ✅ FIXED: Spell Slot Initialization Bug

**Problem**: Spell slots were incorrectly initialized (showing only 1-2 slots regardless of level).

**Impact**: The `has_sufficient_slots_for_buff()` function would return FALSE because:
- Mobs had minimal spell slots (1-2 total)
- Function requires > 50% slots remaining
- With only 1 slot, casting anything would drop below 50%
- Result: Buffing blocked!

**Solution**: Fixed in `src/mob_spellslots.c` - now uses `GET_LEVEL(ch)` instead of `CLASS_LEVEL(ch, class)`

**Status**: ✅ Completed - but existing mobs need to be reloaded!

### 2. ⚠️ Existing Mobs Have Old Spell Slots

**Problem**: Mobs created before the fix still have incorrect spell slots in memory.

**How to Verify**:
```
stat <wizard_mob>
```

Look at spell slots section. If you see:
```
Circle   Current / Maximum
------   -----------------
  1        1 /   1
  2        1 /   1
```
This mob was created before the fix!

**Solutions**:

**Option A: Reload the Zone** (Recommended)
```
zreset <zone_number>
```
This destroys and recreates all mobs in the zone with proper spell slots.

**Option B: Copyover**
```
copyover
```
Reloads the entire MUD, recreating all mobs.

**Option C: Individual Mob Reload**
```
purge <mob>
load mob <vnum>
```
Removes the old mob and loads a fresh one with correct slots.

### 3. 📊 Buffing is Probabilistic

**How Buffing Works**:

**Out of Combat** (in `mob_act.c`):
```c
else if (!rand_number(0, 15) && IS_NPC_CASTER(ch))
{
  /* 6.25% chance per mobile activity pulse */
  if (GET_CLASS(ch) == CLASS_WIZARD || GET_CLASS(ch) == CLASS_SORCERER)
    wizard_cast_prebuff(ch);
  else
    npc_spellup(ch);
}
```

**Frequency**:
- 1/16 chance = 6.25% per pulse
- Mobile activity pulses are ~4 seconds
- Expected buffing: Once every ~64 seconds on average

**In Combat**:
```c
/* 20% chance to buff in combat */
if (!rand_number(0, 4))
{
  npc_spellup(ch);
  return;
}
```

**Important**: Buffing is **not guaranteed** - it's probabilistic! A wizard might stand around for several minutes without buffing, then suddenly cast multiple buffs in a row. This is normal behavior.

### 4. 🚫 Buffing Conditions & Blockers

Wizard buffing can be blocked by several conditions:

#### A. MOB_NOCLASS Flag
```c
if (MOB_FLAGGED(ch, MOB_NOCLASS))
  return;
```
**Check**: `stat <mob>` and look for `NOCLASS` in NPC flags.
**Solution**: Remove flag with `mset <mob> noclass`

#### B. Spell Slots Depleted
```c
!has_sufficient_slots_for_buff(ch, spellnum)
```
**Condition**: Mob must have > 50% spell slots remaining for that spell circle.
**Check**: `stat <mob>` and look at spell slots.
**Solution**: Wait for regeneration or use `set mob` command to restore slots.

#### C. Already Has the Buff
```c
affected_by_spell(ch, spellnum)
```
**Reason**: Won't recast buffs already active.
**Check**: `stat <mob>` shows active affects.
**Solution**: This is working as intended!

#### D. Spell Level Too High
```c
level < SINFO.min_level[char_class]
```
**Reason**: Mob level too low for that spell.
**Check**: Mob level vs spell requirements.
**Solution**: Increase mob level or adjust spell list.

#### E. No Slashing Weapon (for Keen Edge)
```c
(spellnum == SPELL_KEEN_EDGE && !has_slashing_weapon(ch))
```
**Reason**: Keen Edge only works on slashing weapons.
**Solution**: Give mob a slashing weapon or skip Keen Edge.

#### F. Can't Continue (paralyzed, stunned, etc.)
```c
if (!can_continue(ch, FALSE))
  return;
```
**Check**: Mob's position, affects, etc.

### 5. 🎲 Group Buff Logic

**If Mob Has Allies**:
```c
/* 30% chance to cast group buff if we have allies */
if (has_allies && !rand_number(0, 2))
{
  /* Try to cast Mass Haste, Mass Strength, etc. */
}
```

**Ally Detection**:
- Formal GROUP() members
- Mobs following this mob
- Mobs following the same master

**Implication**: A solo wizard is less likely to use group buffs (only self-buffs).

### 6. 🔍 Debugging Steps

**Step 1: Check Mob Class**
```
stat <mob>
```
Look for: `CrntClass: Wizard` or `Sorcerer`

**Step 2: Check IS_NPC_CASTER**
Wizard and Sorcerer are both included in `IS_NPC_CASTER`, so this should pass.

**Step 3: Check Spell Slots**
```
stat <mob>
```
Look at the "Spell Slots" section. Should show proper scaling:
```
Level 11 Wizard:
  Circle 1: 6/6
  Circle 2: 5/5
  Circle 3: 5/5
  Circle 4: 4/4
  Circle 5: 3/3
```

If you see 1/1 or 2/2, reload the mob!

**Step 4: Check MOB_NOCLASS Flag**
```
stat <mob>
```
Look in "NPC flags:" for `NOCLASS`. If present, remove it:
```
mset <mob> noclass
```

**Step 5: Watch for Buffing**
```
watch <mob>
```
Wait several minutes. You should see occasional buff casting.

**Step 6: Force a Test**
You can't directly force buffing, but you can:
1. Reduce mob's current spell slots: `set mob <name> slots <circle> 1`
2. Watch if regeneration works
3. See if mob buffs when slots regenerate

### 7. 📈 Expected Behavior

**Normal Wizard Buffing Pattern**:

**Pre-Combat** (Standing Around):
- Every ~64 seconds (on average): Cast a buff
- Prioritizes long-duration buffs (Stoneskin, Mirror Image, Haste, etc.)
- Skips buffs already active
- Won't buff if < 50% spell slots

**With Allies Present**:
- 30% of buffs will be group buffs (Mass Haste, Mass Strength, etc.)
- 70% will be self buffs

**In Combat**:
- 20% chance per round to buff instead of attack
- May cast defensive buffs mid-combat

**Slot Conservation**:
- Won't buff if spell slots < 50% for that circle
- Prioritizes keeping slots for combat

### 8. 🛠️ Manual Testing

**Test 1: Verify Spell Slots**
```
stat <wizard_mob>
```
Expected: Multiple circles with proper slot counts.

**Test 2: Check Current Buffs**
```
stat <wizard_mob>
```
Look for existing affects (Stoneskin, Haste, Mirror Image, etc.)

**Test 3: Remove All Buffs**
```
dispel <wizard_mob>
```
Wait and watch if mob rebuffs.

**Test 4: Watch Over Time**
```
watch <wizard_mob>
```
Leave mob alone for 5-10 minutes. Should see buffing activity.

**Test 5: Create Fresh Mob**
```
load mob <wizard_vnum>
stat <mob>
```
Fresh mob should have proper spell slots immediately.

## Campaign-Specific Notes

### Non-DL Campaign (Your Case)
```c
#else  /* Not CAMPAIGN_DL */
else if (!rand_number(0, 15) && IS_NPC_CASTER(ch))
{
  /* Buffing enabled by default, no MOB_BUFF_OUTSIDE_COMBAT flag needed */
  if (GET_CLASS(ch) == CLASS_WIZARD || GET_CLASS(ch) == CLASS_SORCERER)
    wizard_cast_prebuff(ch);
}
#endif
```

**Result**: Wizard/Sorcerer mobs should buff without any special flag!

### DL Campaign
```c
#if defined(CAMPAIGN_DL)
else if (!rand_number(0, 15) && MOB_FLAGGED(ch, MOB_BUFF_OUTSIDE_COMBAT) && IS_NPC_CASTER(ch))
{
  /* Requires MOB_BUFF_OUTSIDE_COMBAT flag */
  wizard_cast_prebuff(ch);
}
#endif
```

**Result**: Would require `MOB_BUFF_OUTSIDE_COMBAT` flag to buff.

## Quick Fix Checklist

- [ ] 1. Verify spell slot fix is compiled (`make`)
- [ ] 2. Check a wizard mob's spell slots with `stat`
- [ ] 3. If slots are wrong (1/1, 2/2), reload the zone with `zreset`
- [ ] 4. Check mob doesn't have `MOB_NOCLASS` flag
- [ ] 5. Verify mob is CLASS_WIZARD or CLASS_SORCERER
- [ ] 6. Wait 5-10 minutes and observe buffing
- [ ] 7. Remember: 6.25% chance = ~1 buff per minute on average

## Still Not Buffing?

If after following all steps the mob still doesn't buff:

1. **Check compilation**: `make 2>&1 | grep error`
2. **Check mob is loaded post-fix**: Kill and reload mob
3. **Check MOB_NOCLASS**: Remove if present
4. **Increase observation time**: Wait 10+ minutes
5. **Test with multiple mobs**: One might be unlucky with RNG
6. **Check combat state**: Mob shouldn't be fighting
7. **Review code**: `grep wizard_cast_prebuff src/mob_act.c`

## Related Files
- `src/mob_act.c` - Buffing trigger logic (lines 138-165)
- `src/mob_spells.c` - wizard_cast_prebuff() implementation
- `src/mob_spellslots.c` - Spell slot system
- `src/utils.h` - IS_NPC_CASTER macro definition

## Status Summary
✅ Code is correct
✅ Spell slots fixed
⚠️ Existing mobs need zone reload
✅ No MOB_BUFF_OUTSIDE_COMBAT flag required (non-DL)
✅ Buffing is probabilistic (6.25% per pulse)
