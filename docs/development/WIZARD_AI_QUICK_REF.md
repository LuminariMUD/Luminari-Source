# Wizard/Sorcerer AI Enhancement Summary

## Quick Reference

### New Spell Lists

#### Group Buffs (MAG_GROUPS) - 10 Spells
Used when wizard has allies (formal group or followers):
- Mass Wisdom/Charisma/Cunning/Strength/Grace/Endurance (stat buffs)
- Mass Haste (combat buff)
- Mass Fly/Invisibility (utility buffs)
- Circle Against Evil (protection)

#### AoE Spells (MAG_AREAS) - 15 Spells
Used when fighting 2+ enemies:
- **High Level**: Meteor Swarm, Horrid Wilting, Wail of the Banshee, Incendiary Cloud
- **Mid Level**: Chain Lightning, Ice Storm, Prismatic Spray, Mass Hold Person
- **Low Level**: Fireball, Lightning Bolt, Burning Hands, Color Spray, Thunderclap
- **Debuffs**: Waves of Exhaustion, Waves of Fatigue

### Behavior Changes

**Pre-Combat Buffing** (`wizard_cast_prebuff`):
```
IF has_allies:
  30% chance → Cast group buff (Mass Haste, Mass Strength, etc.)
  70% chance → Cast self buff (Stoneskin, Mirror Image, etc.)
ELSE:
  100% chance → Cast self buff
```

**Combat Casting** (`wizard_combat_ai`):
```
IF fighting 2+ enemies:
  Cast AoE spell from wizard_aoe_spells[]
ELSE:
  Cast single-target spell from valid_offensive_spell[]
```

## Code Locations

- **Header**: `src/mob_spells.h` (lines defining WIZARD_GROUP_BUFFS, WIZARD_AOE_SPELLS)
- **Arrays**: `src/mob_spells.c` (wizard_group_buff_spells[], wizard_aoe_spells[])
- **Logic**: `src/mob_spells.c` (wizard_cast_prebuff(), wizard_combat_ai())

## Testing Commands

```bash
# Create wizard with followers
load mob <wizard_vnum>
load mob <apprentice_vnum>
force <apprentice> follow <wizard>

# Trigger prebuff (should see group buff ~30% of time)
force <wizard> cast 'mass haste'

# Combat test with multiple enemies
# (wizard should use AoE spells)
```

## Integration Points

Works with:
- ✅ `are_grouped()` function (detects allied mobs)
- ✅ Spell slot conservation (50% threshold)
- ✅ Level requirements (SINFO.min_level checks)
- ✅ Existing wizard AI (wizard_is_long_duration_buff, etc.)

## Statistics

- **10** new group buff spells for tactical support
- **15** wizard-specific AoE spells for multi-target combat
- **30%** chance to use group buff when allies present
- **100%** chance to use AoE when fighting 2+ enemies

## Impact

### Before
- Wizards cast single-target spells regardless of enemy count
- No group support for allied mobs
- Generic AoE spell selection (including non-wizard spells)

### After
- Wizards buff entire group when allies present
- Strategic AoE usage against multiple foes
- Class-appropriate spell selection

## Status
✅ **COMPLETE** - Compiled and ready for testing
