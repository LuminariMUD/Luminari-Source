# Mob AI Group Logic Enhancement

## Overview
Enhanced mob AI to recognize and support allies through a new `are_grouped()` function that handles both formal groups and mob following relationships.

## Changes Made

### 1. New Function: `are_grouped()` (src/utils.c, src/utils.h)

**Purpose**: Determines if two characters should be considered "grouped" for AI decisions.

**Logic**:
- Returns TRUE if both characters are in the same formal GROUP()
- For NPCs in the same room:
  - Returns TRUE if one is following the other
  - Returns TRUE if both follow the same master
- Enables pack behavior for mobs (e.g., wolves following an alpha)

**Use Cases**:
- Group heal spell targeting
- Buff spell targeting decisions
- Coordinated mob tactics
- Pack behavior for predator mobs

### 2. Enhanced Healing/Buff Targeting (src/mob_spells.c)

**Function Modified**: `npc_spellup()`

**Before**: Only checked formal GROUP() members for healing/buffing
```c
if (GROUP(ch) && GROUP(ch)->members->iSize)
{
  victim = (struct char_data *)random_from_list(GROUP(ch)->members);
}
```

**After**: Also checks for allied mobs using following logic
```c
if (GROUP(ch) && GROUP(ch)->members->iSize)
{
  victim = (struct char_data *)random_from_list(GROUP(ch)->members);
}
else if (IS_NPC(ch))
{
  /* Check all NPCs in room for allies that need healing */
  for (tch = world[IN_ROOM(ch)].people; tch; tch = tch->next_in_room)
  {
    if (!are_grouped(ch, tch))
      continue;
    /* Find most injured ally */
  }
}
```

**Behavior**:
- Mobs now heal/buff their followers
- Pack leaders can support their pack
- Summoned creatures are supported by summoner
- Prioritizes most injured ally

## Examples

### Example 1: Wolf Pack
```
Alpha Wolf (level 15)
  └─ Wolf 1 (level 12, following alpha)
  └─ Wolf 2 (level 12, following alpha)
  └─ Wolf 3 (level 12, following alpha)
```

**Before**: Alpha would only buff/heal itself
**After**: Alpha can heal injured pack members, cast group buffs

### Example 2: Necromancer with Undead
```
Necromancer Mob (level 20)
  └─ Zombie 1 (following necromancer)
  └─ Zombie 2 (following necromancer)
  └─ Skeleton (following necromancer)
```

**Before**: Necromancer ignores minions' health
**After**: Necromancer can heal undead minions if capable

### Example 3: Guard Captain with Guards
```
Guard Captain (level 18)
  └─ Guard 1 (level 15, following captain)
  └─ Guard 2 (level 15, following captain)
```

**Before**: Guards fought independently
**After**: Captain can cast group heals, buff guards

## Technical Details

### Function Signature
```c
bool are_grouped(struct char_data *ch, struct char_data *target);
```

### Performance Considerations
- Function includes NULL checks and early returns
- Same room check prevents long-distance searches
- Only iterates room occupants (typically small list)
- No recursive following checks (prevents infinite loops)

### Integration Points
Current usage:
- `npc_spellup()` - healing and buff targeting

Potential future usage:
- `wizard_combat_ai()` - coordinated offensive spells
- `cleric_combat_ai()` - group healing priority (TBD)
- `druid_combat_ai()` - group support spells (TBD)
- General mob tactics for coordinated attacks

## Testing Recommendations

1. **Basic Following**:
   - Create two mobs, make one follow the other
   - Damage the follower
   - Verify leader attempts to heal follower

2. **Pack Behavior**:
   - Create 3-4 mobs following a leader
   - Damage pack members to varying degrees
   - Verify leader prioritizes most injured

3. **Formal Groups**:
   - Verify existing GROUP() behavior unchanged
   - Player-led groups with mob followers should work

4. **Edge Cases**:
   - Mobs in different rooms (should not help)
   - Hostile mobs (should not help)
   - Self-targeting still works

## Future Enhancements

### Short Term
- Add coordinated combat tactics (focus fire)
- Group buff casting (buff all allies at once)
- Flanking behavior for rogues with allies

### Long Term
- Formation-based tactics
- Role-based AI (tank/healer/DPS coordination)
- Dynamic pack leadership (alpha dies, beta takes over)
- Morale system (pack flees if leader dies)

## Related Systems
- Mob following system (act.movement.c)
- Group system (structs.h, GROUP macro)
- Spell targeting (magic.c)
- Mob AI (mob_act.c, mob_spells.c)

## Compilation Status
✅ Compiles cleanly with no errors or warnings
✅ Function properly declared in utils.h
✅ Integrated into mob_spells.c successfully

## Author Notes
This enhancement lays the groundwork for sophisticated pack/group mob behaviors. The simple following relationship can now drive complex coordinated AI decisions without requiring formal group membership.
