# Calling Mechanics Research for LuminariMUD

## Overview
This document contains research on the various "call" mechanics for different character classes in LuminariMUD. This will inform the development of a more comprehensive calling system for the mob behavior system.

## Current Implementation Status

### Core System Architecture
- **Main Command**: `do_call` in act.other.c:1996
- **Core Function**: `perform_call()` in act.other.c:1657
- **Command Usage**: `call <companion/familiar/mount/shadow/eidolon/cohort/dragon>`
- **NPC Integration**: Rangers already use `perform_call(ch, MOB_C_ANIMAL, GET_LEVEL(ch))` in mob_class.c:221

## Class-Specific Calling Mechanics

### 1. Ranger/Druid - Animal Companion (MOB_C_ANIMAL)
**Implementation Status**: ✅ Fully Implemented
- **Classes**: Ranger (level 4+), Druid
- **Required Feat**: FEAT_ANIMAL_COMPANION
- **Selection Method**: Via 'study' command
- **Storage**: GET_ANIMAL_COMPANION(ch)
- **Mob Flag**: MOB_C_ANIMAL
- **Level Calculation**: 
  - Druid: CLASS_LEVEL(ch, CLASS_DRUID)
  - Ranger: CLASS_LEVEL(ch, CLASS_RANGER) - 3 (if level >= 4)
- **Max Level**: 20
- **Cooldown Event**: eC_ANIMAL (4 MUD days)
- **Special Features**:
  - FEAT_BOON_COMPANION adds +5 levels
  - +20 HP bonus
  - Autoroll stats based on level
- **NPC Behavior**: Already integrated in npc_ranger_behave()

### 2. Sorcerer/Wizard - Familiar (MOB_C_FAMILIAR)
**Implementation Status**: ✅ Fully Implemented
- **Classes**: Sorcerer, Wizard
- **Required Feat**: FEAT_SUMMON_FAMILIAR
- **Selection Method**: Via 'study' command
- **Storage**: GET_FAMILIAR(ch)
- **Mob Flag**: MOB_C_FAMILIAR
- **Level Calculation**: CLASS_LEVEL(ch, CLASS_SORCERER) + CLASS_LEVEL(ch, CLASS_WIZARD)
- **Max Level**: 20
- **Cooldown Event**: eC_FAMILIAR (4 MUD days)
- **Special Features**:
  - Base +10 HP
  - +2d4+1 HP per level
  - FEAT_IMPROVED_FAMILIAR provides bonuses:
    - +10 HP per feat rank
    - +10 AC per feat rank
    - +1 to STR/DEX/CON per feat rank

### 3. Paladin/Blackguard - Mount (MOB_C_MOUNT)
**Implementation Status**: ✅ Fully Implemented
- **Classes**: Paladin, Blackguard
- **Required Feat**: FEAT_CALL_MOUNT
- **Selection Method**: Automatic based on class/level/size
- **Storage**: GET_MOUNT(ch)
- **Mob Flag**: MOB_C_MOUNT
- **Level Calculation**: CLASS_LEVEL(ch, CLASS_PALADIN) or CLASS_LEVEL(ch, CLASS_BLACKGUARD)
- **Max Level**: 20 (27 for epic mounts)
- **Cooldown Event**: eC_MOUNT (4 MUD days)
- **Mount Types**:
  - Paladin: MOB_PALADIN_MOUNT / MOB_PALADIN_MOUNT_SMALL
  - Paladin Epic: MOB_EPIC_PALADIN_MOUNT / MOB_EPIC_PALADIN_MOUNT_SMALL
  - Blackguard: MOB_BLACKGUARD_MOUNT
  - Blackguard Advanced (level 12+): MOB_ADV_BLACKGUARD_MOUNT
  - Blackguard Epic: MOB_EPIC_BLACKGUARD_MOUNT
- **Special Features**:
  - Mount size = character size + 1
  - +20 HP bonus
  - 500 movement points
  - Autoroll stats

### 4. Summoner/Necromancer - Eidolon/Undead Cohort (MOB_EIDOLON)
**Implementation Status**: ✅ Fully Implemented
- **Classes**: Summoner (eidolon), Necromancer (undead cohort)
- **Required Feat**: FEAT_EIDOLON or FEAT_UNDEAD_COHORT
- **Commands**: Both 'call eidolon' and 'call cohort' work
- **Storage**: Custom eidolon data structures
- **Mob Flag**: MOB_EIDOLON
- **Mob Vnum**: MOB_NUM_EIDOLON
- **Level Calculation**: MIN(GET_LEVEL(ch), CLASS_LEVEL(ch, CLASS_SUMMONER) + CLASS_LEVEL(ch, CLASS_NECROMANCER))
- **Max Level**: 30
- **Cooldown Event**: eC_EIDOLON (4 MUD days)
- **Special Features**:
  - +20 HP bonus
  - Custom descriptions via GET_EIDOLON_SHORT_DESCRIPTION/GET_EIDOLON_LONG_DESCRIPTION
  - Evolution system via assign_eidolon_evolutions()
  - Autoroll stats

### 5. Dragonrider - Dragon Mount (MOB_C_DRAGON)
**Implementation Status**: ✅ Fully Implemented
- **Required Feat**: FEAT_DRAGON_BOND
- **Selection Method**: Via 'study' command
- **Storage**: GET_DRAGON_RIDER_DRAGON_TYPE(ch)
- **Mob Flag**: MOB_C_DRAGON
- **Mob Vnum**: GET_DRAGON_RIDER_DRAGON_TYPE(ch) + 40400
- **Level Calculation**: MIN(256, GET_LEVEL(ch))
- **Max Level**: 25
- **Cooldown Event**: eC_DRAGONMOUNT (4 MUD days)
- **Special Features**:
  - Autoroll stats based on level
  - Dragon type selection determines specific mob

### 6. Shadowdancer - Shadow Companion (MOB_SHADOW)
**Implementation Status**: ✅ Fully Implemented
- **Class**: Shadowdancer
- **Required Feat**: FEAT_SUMMON_SHADOW
- **Mob Flag**: MOB_SHADOW
- **Mob Vnum**: 60289 (fixed)
- **Level Calculation**: MIN(GET_LEVEL(ch), CLASS_LEVEL(ch, CLASS_SHADOWDANCER) + 8)
- **Max Level**: 25
- **Cooldown Event**: eSUMMONSHADOW (4 MUD days)
- **Special Features**:
  - FEAT_SHADOW_MASTER adds 1d4+1 levels
  - +20 HP bonus
  - Autoroll stats

## Common Mechanics Across All Call Types

### Shared Features:
1. **Charm Effect**: All called creatures get AFF_CHARM flag
2. **Follower System**: Added as follower to the caller
3. **Group Integration**: Auto-joins caller's group if applicable
4. **Pet Saving**: Saves via save_char_pets()
5. **Ultravision**: All get FEAT_ULTRAVISION
6. **Cooldown**: ~14 minutes (4 MUD days)
7. **Wilderness Support**: Coordinates set for wilderness zones
8. **Mob Loading**: Triggers load_mtrigger()

### Existing Check System:
1. Check if companion already summoned
2. If in different room, teleport to caller
3. If in same room, notify already present
4. Check for valid selection (study command)
5. Check for cooldown events
6. Level and feat requirements

### Dismiss Functionality:
- 'dismiss' command available to reduce cooldown
- Removes follower from group and world

## Integration Points for Mob Behavior System

### Current NPC Usage:
- Only Rangers currently use calling in mob behavior (mob_class.c)
- Called via: `perform_call(ch, MOB_C_ANIMAL, GET_LEVEL(ch))`
- No cooldown checking for NPCs

### Recommended Enhancements for Mob Behavior:

1. **Add Class-Specific Calling**:
   - Wizard/Sorcerer NPCs: Call familiars in combat
   - Paladin/Blackguard NPCs: Call mounts when needed
   - Summoner/Necromancer NPCs: Call eidolons/cohorts
   - Shadowdancer NPCs: Call shadows
   - Dragonrider NPCs: Call dragons

2. **Smart Calling Logic**:
   - Check if companion already exists before calling
   - Call at start of combat or when outnumbered
   - Different call priorities based on situation
   - Respect cooldowns even for NPCs (optional)

3. **NPC-Specific Considerations**:
   - NPCs don't need 'study' command selections
   - Default mob vnums for NPC calls already exist
   - May want shorter cooldowns for NPCs
   - Could bypass feat requirements for special NPCs

4. **Implementation Strategy**:
   - Add calling checks to respective class behavior functions
   - Create npc_should_call_companion() utility function
   - Add MOB_NO_CALL flag for NPCs that shouldn't call
   - Consider environmental factors (indoor/outdoor, room size)

## Technical Notes

### Key Files:
- act.other.c: Main call command and perform_call function
- mob_class.c: NPC class behaviors
- structs.h: Mob flags and feat definitions
- utils.h: Macros for checking feats and companion data
- mud_event.h: Cooldown event definitions

### Mob Flags:
- MOB_C_ANIMAL (26)
- MOB_C_FAMILIAR (27)
- MOB_C_MOUNT (28)
- MOB_SHADOW (43)
- MOB_EIDOLON (89)
- MOB_C_DRAGON (94)

### Event Types:
- eC_ANIMAL
- eC_FAMILIAR
- eC_MOUNT
- eSUMMONSHADOW
- eC_EIDOLON
- eC_DRAGONMOUNT

## Future Considerations

1. **Expanded Companion Types**:
   - Alchemist homunculus
   - Witch familiar (different from wizard)
   - Cavalier mount (different from paladin)
   - Beastmaster multiple companions

2. **Enhanced Features**:
   - Companion equipment system
   - Companion advancement/leveling
   - Companion special abilities based on type
   - Companion death/resurrection mechanics

3. **Mob Behavior Integration**:
   - Coordinate companion actions with master
   - Special combo attacks
   - Protective behaviors
   - Formation fighting

---
*Research compiled for mob behavior system enhancement*
*Last updated: 2025*