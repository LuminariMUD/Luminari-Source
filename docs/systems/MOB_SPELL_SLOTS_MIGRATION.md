# Mob Spell Slot System - Change Summary & Migration Guide

## Date: October 19, 2025

## ⚠️ BREAKING CHANGES

### Changes Made

1. **Default Behavior Reversed**
   - **OLD**: Mobs had unlimited spell casting by default
   - **NEW**: All mobs use spell slots by default
   
2. **Flag Changed**
   - **REMOVED**: `MOB_USES_SPELLSLOTS` (flag 100) - enabled spell slots
   - **ADDED**: `MOB_UNLIMITED_SPELL_SLOTS` (flag 100) - disables spell slots (opt-out)
   
3. **Regeneration Time Increased**
   - **OLD**: 1 minute (60 seconds) per slot
   - **NEW**: 2 minutes (120 seconds) per slot

### Why These Changes?

**Default Spell Slots (Opt-Out Design)**:
- Encourages tactical, resource-based gameplay across the entire game world
- Makes spellcasting encounters more strategic and challenging
- Prevents trivial encounters where casters spam high-level spells indefinitely
- Allows special/boss mobs to be explicitly flagged for unlimited power

**Slower Regeneration**:
- Makes slot depletion more meaningful
- Requires players to make decisive tactical choices
- Prevents rapid recovery between pulls
- Increases value of sustained combat pressure

## Impact Assessment

### What Will Change

**All Mobs (Without Flag)**:
- ✅ Will now have limited spell slots based on class/level
- ✅ Will deplete slots when casting
- ✅ Will regenerate 1 slot per 2 minutes (out of combat)
- ✅ Will become easier to defeat in extended combat

**Flagged Mobs (With MOB_UNLIMITED_SPELL_SLOTS)**:
- ✅ Will cast unlimited spells (old behavior)
- ✅ No slot tracking or regeneration
- ✅ Suitable for bosses, deities, or special encounters

### Affected Content

**High-Impact Areas**:
- Spellcaster boss fights (may become easier without flag)
- Dungeon caster mobs (will run out of spells)
- NPC healers (limited healing slots)
- Zone gauntlets with multiple caster encounters

**Low-Impact Areas**:
- Melee-only mobs (unaffected)
- Non-spellcasting encounters (unaffected)
- Player characters (unaffected)

## Migration Strategy

### Phase 1: Immediate Actions (Required)

1. **Identify Critical Mobs**
   Review and flag mobs that MUST have unlimited casting:
   - Major boss encounters
   - Deity/immortal NPCs
   - Quest-critical spellcasters
   - Special event mobs

2. **Add MOB_UNLIMITED_SPELL_SLOTS Flag**
   For each critical mob:
   ```
   medit <vnum>
   mob flags
   [Toggle "Unlimited-Spell-Slots"]
   save internally
   ```

3. **Test Core Content**
   - Test major boss fights
   - Test critical quest mobs
   - Verify flagged mobs work correctly

### Phase 2: Content Review (Recommended)

1. **Zone-by-Zone Review**
   For each zone, consider:
   - Are caster mobs too weak with limited slots?
   - Are encounters now more interesting with slot management?
   - Do any standard mobs need the unlimited flag?

2. **Difficulty Rebalancing**
   Options if encounters are too easy:
   - Leave as-is (tactical challenge)
   - Flag select mobs for unlimited slots
   - Increase mob levels/HP
   - Add more mobs to encounters

3. **Special Encounters**
   Consider unlimited slots for:
   - Legendary/mythic creatures
   - Powerful undead (liches, vampires)
   - Elemental lords
   - Demon princes
   - Quest villains

### Phase 3: Documentation (Recommended)

1. **Update Zone Documentation**
   Document which mobs have unlimited slots and why

2. **Builder Guidelines**
   Create standards for when to use unlimited slots flag

3. **Player Communication**
   Inform players that mob casters now have limited resources

## Quick Reference: Flagging Mobs

### Mobs That Should Get UNLIMITED Flag

✅ **Always Flag**:
- Boss encounters (raid-level threats)
- Deities and immortals
- Epic/legendary creatures
- Quest-critical casters (if failure would break quest)

✅ **Consider Flagging**:
- High-level dungeon bosses
- Zone end-bosses
- Elite caster mobs
- Named/unique spellcasters

❌ **Don't Flag (Use Default Slots)**:
- Regular dungeon mobs
- Random encounter casters
- Common NPC casters
- Trash mobs
- Low-level content

### How to Flag a Mob

```
medit <mob vnum>
mob flags
[Find and toggle "Unlimited-Spell-Slots"]
save internally
```

Verify with:
```
mstat <mob name>
[Look for "Unlimited-Spell-Slots" in flags]
```

## Code Changes Summary

### Files Modified

1. **src/structs.h**
   - Changed: `MOB_USES_SPELLSLOTS` → `MOB_UNLIMITED_SPELL_SLOTS`
   - Meaning: Reversed - now disables slot system instead of enabling it

2. **src/constants.c**
   - Changed: `"Uses-Spell-Slots"` → `"Unlimited-Spell-Slots"`

3. **src/mob_spellslots.c**
   - Reversed all flag checks: `!MOB_FLAGGED(ch, MOB_USES_SPELLSLOTS)` → `MOB_FLAGGED(ch, MOB_UNLIMITED_SPELL_SLOTS)`
   - Changed regeneration: 60 seconds → 120 seconds
   - Updated comments to reflect new behavior

4. **src/spell_parser.c**
   - Reversed flag logic in `npc_can_cast()` and `call_magic()`
   - Now checks for unlimited flag to bypass, rather than slot flag to enable

5. **Documentation Files**
   - All docs updated to reflect new default-enabled behavior
   - Examples updated to show opt-out pattern
   - Migration guide created (this file)

### Compilation Status

✅ **Successfully compiled** with no errors or warnings

## Testing Checklist

### Pre-Deployment Testing

- [ ] Compile succeeds without errors
- [ ] Regular mob (no flag) has limited slots
- [ ] Regular mob depletes slots when casting
- [ ] Regular mob cannot cast when depleted
- [ ] Regular mob regenerates 1 slot per 2 minutes
- [ ] Flagged mob (unlimited) casts without limit
- [ ] Flagged mob shows unlimited in mstat/logs
- [ ] Combat prevents regeneration
- [ ] Out-of-combat allows regeneration

### Post-Deployment Monitoring

- [ ] Monitor boss encounter difficulty
- [ ] Track player feedback on caster mobs
- [ ] Identify mobs that need unlimited flag
- [ ] Check for unintended difficulty spikes/drops
- [ ] Verify no crashes or errors in logs

## Rollback Plan

If issues arise and rollback is needed:

1. **Revert Code Changes**
   ```bash
   cd /home/krynn/code
   git diff src/structs.h src/constants.c src/mob_spellslots.c src/spell_parser.c
   git checkout HEAD -- src/structs.h src/constants.c src/mob_spellslots.c src/spell_parser.c
   make
   ```

2. **Alternative: Add Universal Flag**
   Quick fix without code revert:
   - Add MOB_UNLIMITED_SPELL_SLOTS to mob prototypes globally
   - This restores unlimited casting for all mobs
   - Then selectively remove flag from mobs that should use slots

## Communication Plan

### To Builders

**Subject**: Mob Spell Slot System Now Default - Action Required

**Message**:
```
The mob spell slot system is now ENABLED BY DEFAULT for all mobs.

ACTION REQUIRED:
1. Review your zones for caster mobs
2. Add "Unlimited-Spell-Slots" flag to any mobs that MUST have unlimited casting
3. Test your boss fights and critical encounters

Most regular mobs should work fine with the default limited slots.
Only flag bosses, deities, and special encounters for unlimited casting.

Command: medit <vnum> -> mob flags -> toggle "Unlimited-Spell-Slots"
```

### To Players

**Subject**: Mob Spellcasters Now Have Limited Resources

**Message**:
```
Exciting tactical update: Mob spellcasters now have limited spell slots!

What this means:
- Caster mobs will run out of high-level spells during combat
- Sustained pressure is more effective against casters
- Giving casters time to rest allows them to regenerate (1 slot per 2 minutes)
- Boss encounters may have unlimited slots for epic challenges

This adds strategic depth to combat. Focus fire on casters early, 
or use sustained pressure to deplete their magical resources!
```

## FAQ

**Q: Will this make the game too easy?**
A: Most encounters should remain challenging. Mobs still have multiple slots per circle, and regenerate out of combat. Boss mobs can be flagged for unlimited power.

**Q: How do I restore old behavior for a specific mob?**
A: `medit <vnum>`, toggle "Unlimited-Spell-Slots" flag, save.

**Q: What about player pets/summons?**
A: They follow the same rules - limited slots unless flagged.

**Q: Can players see mob spell slots?**
A: Not by default. This is intended information asymmetry. Admin commands can view slots for testing.

**Q: What if I want faster regeneration?**
A: Future enhancement. Currently hardcoded to 2 minutes for all mobs.

**Q: Does this affect player spellcasting?**
A: No. Players have their own spell slot system. This only affects NPCs.

## Future Enhancements

Planned improvements to the system:

1. **Variable Regen Rates**
   - Per-mob configurable regeneration speed
   - Race/type-based defaults (liches slower, fey faster)

2. **Admin Commands**
   - View mob spell slots with `mstat`
   - Force regeneration with admin command
   - Set custom slot counts per mob

3. **Advanced Mechanics**
   - Slot trading (higher for multiple lower)
   - Mob AI to manage slots intelligently
   - Retreat behavior when slots low

---

**Document Version**: 1.0  
**Last Updated**: October 19, 2025  
**Status**: Changes Implemented and Compiled  
**Deployment**: Ready for testing
