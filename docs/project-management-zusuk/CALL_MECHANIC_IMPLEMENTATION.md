# Call Mechanic Implementation for NPCs

## Overview
This document describes the implementation of companion calling mechanics for NPC behavior in LuminariMUD.

## Implementation Summary

### 1. Utility Function Added
- **File**: `src/mob_utils.c` and `src/mob_utils.h`
- **Function**: `npc_should_call_companion()`
- **Purpose**: Determines if an NPC should attempt to call a companion based on class, combat state, and existing companions

### 2. Class-Specific Implementations

#### Rangers (Already Existed, Modified)
- **File**: `src/mob_class.c`
- **Function**: `npc_ranger_behave()`
- **Change**: Now uses `npc_should_call_companion()` instead of always calling
- **Companion Type**: Animal (MOB_C_ANIMAL)

#### Druids (Added)
- **File**: `src/mob_spells.c`
- **Functions**: `npc_spellup()` and `npc_offensive_spells()`
- **Companion Type**: Animal (MOB_C_ANIMAL)

#### Paladins/Blackguards (Added)
- **File**: `src/mob_class.c`
- **Function**: `npc_paladin_behave()`
- **Change**: Added mount calling
- **Companion Type**: Mount (MOB_C_MOUNT)
- **Note**: Blackguards now use paladin behavior function

#### Wizards/Sorcerers (Added)
- **File**: `src/mob_spells.c`
- **Functions**: `npc_spellup()` and `npc_offensive_spells()`
- **Companion Type**: Familiar (MOB_C_FAMILIAR)

#### Summoners/Necromancers (Added)
- **File**: `src/mob_spells.c`
- **Functions**: `npc_spellup()` and `npc_offensive_spells()`
- **Companion Type**: Eidolon/Undead Cohort (MOB_EIDOLON)

#### Shadowdancers (Added)
- **File**: `src/mob_spells.c`
- **Functions**: `npc_spellup()` and `npc_offensive_spells()`
- **Companion Type**: Shadow (MOB_SHADOW)

#### Dragonriders (Added)
- **File**: `src/mob_spells.c`
- **Functions**: `npc_spellup()` and `npc_offensive_spells()`
- **Companion Type**: Dragon (MOB_C_DRAGON)

## Calling Logic

### When NPCs Call Companions
1. **In Combat**: 50% chance when fighting
2. **Out of Combat**: 10% chance during spellup
3. **Never Called If**:
   - Companion already exists
   - NPC health below 25%
   - Wrong class for companion type
   - Rangers below level 4

### Priority System
- Companions are called BEFORE other actions
- In spellup: Called before buffing
- In combat: Called before offensive spells
- For non-casters: Called at start of behavior routine

## Testing Instructions

### 1. Create Test NPCs
Use OLC to create NPCs with the following specifications:

#### Test Ranger
```
Class: RANGER
Level: 10
Flags: MOB_SENTINEL (to keep in one room for testing)
```

#### Test Wizard
```
Class: WIZARD
Level: 15
Flags: MOB_SENTINEL
```

#### Test Paladin
```
Class: PALADIN
Level: 12
Flags: MOB_SENTINEL
```

#### Test Summoner
```
Class: SUMMONER
Level: 20
Flags: MOB_SENTINEL
```

#### Test Shadowdancer
```
Class: SHADOWDANCER
Level: 18
Flags: MOB_SENTINEL
```

### 2. Testing Steps

1. **Spawn the test NPC**: Use wizard commands to load the mob
2. **Observe Non-Combat Behavior**: Wait and watch if companion is called (10% chance per tick)
3. **Test Combat Behavior**: Attack the NPC and observe if companion is called (50% chance)
4. **Verify No Duplicate Calls**: Ensure NPC doesn't try to call another companion if one exists
5. **Test Companion Persistence**: Leave room and return - companion should remain

### 3. Expected Behaviors

- **Rangers/Druids**: Should call an animal companion (vnum 63 for NPCs)
- **Paladins/Blackguards**: Should call a mount appropriate to their alignment
- **Wizards/Sorcerers**: Should call a familiar
- **Summoners/Necromancers**: Should call an eidolon (vnum MOB_NUM_EIDOLON)
- **Shadowdancers**: Should call a shadow (vnum 60289)
- **Dragonriders**: Should call a dragon mount

### 4. Debug Commands
If needed, use these commands to verify:
- `stat mob <name>` - Check mob's class and level
- `where <companion_name>` - Verify companion was created
- Check system logs for any errors during `perform_call()`

## Files Modified
1. `src/mob_utils.c` - Added `npc_should_call_companion()`
2. `src/mob_utils.h` - Added function declaration
3. `src/mob_class.c` - Modified ranger and paladin behaviors, added blackguard dispatch
4. `src/mob_spells.c` - Added calling for all spellcasting classes

## Future Enhancements
1. Add MOB_NO_CALL flag for NPCs that shouldn't call companions
2. Adjust calling percentages based on mob intelligence
3. Add formation fighting between NPCs and their companions
4. Consider environmental restrictions (indoor vs outdoor)
5. Add special combo attacks with companions