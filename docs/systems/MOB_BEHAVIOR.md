# LuminariMUD Mobile Behavior System Documentation

## Table of Contents
1. [Overview](#overview)
2. [System Architecture](#system-architecture)
3. [Module Organization](#module-organization)
4. [Main Activity Loop](#main-activity-loop)
5. [Combat Behaviors](#combat-behaviors)
6. [Non-Combat Behaviors](#non-combat-behaviors)
7. [Memory System](#memory-system)
8. [Spell and Psionic Systems](#spell-and-psionic-systems)
9. [Utility Functions](#utility-functions)
10. [Mob Flags](#mob-flags)
11. [Builder Guidelines](#builder-guidelines)
12. [Examples](#examples)

## Overview

The LuminariMUD mobile behavior system is a sophisticated AI framework that controls how non-player characters (NPCs/mobs) act and react in the game world. The system is modular, extensible, and provides rich behavioral patterns including combat tactics, spellcasting, movement patterns, and social interactions.

### Key Features
- **Modular Architecture**: Behaviors are organized into logical modules (combat, spells, race, class, etc.)
- **Class-Specific AI**: Each character class has unique combat behaviors and tactics
- **Companion Calling**: NPCs can summon class-appropriate companions (familiars, mounts, etc.)
- **Racial Behaviors**: Special abilities based on creature type (dragons, animals, etc.)
- **Intelligent Spellcasting**: Smart buff management and offensive spell selection
- **Memory System**: Mobs remember players who have wronged them
- **Dynamic Target Selection**: Intelligent opponent selection in combat
- **Path Following**: Support for patrol routes and movement patterns

## System Architecture

The mob behavior system follows a hierarchical execution model:

```
mobile_activity() [Main Loop]
    |-- Special Procedures (if MOB_SPEC flag)
    |-- Combat Behaviors (if fighting)
    |   |-- Racial Behaviors (25% chance)
    |   |-- Ability Behaviors (25% chance)
    |   |-- Assigned Spells (25% chance if knows spells)
    |   `-- Class/Spell Behaviors (remaining)
    |-- Non-Combat Behaviors
    |   |-- Spellup/Powerup (6.25% chance for casters/psionicists)
    |   |-- Mobile Echos
    |   |-- Scavenging
    |   |-- Aggression Checks
    |   |-- Memory Checks
    |   |-- Helper/Guard Behaviors
    |   `-- Movement (paths, hunting, random)
    `-- Position Management
```

## Module Organization

The system is divided into several specialized modules:

### Core Modules

#### mob_act.c/h - Main Coordination
- **Purpose**: Central activity loop and behavior coordination
- **Key Function**: `mobile_activity()` - called each game tick
- **Responsibilities**: 
  - Iterate through all mobs
  - Coordinate behavior execution
  - Handle special procedures
  - Manage aggression and helper behaviors

#### mob_utils.c/h - Utility Functions
- **Purpose**: Common utility functions used across modules
- **Key Functions**:
  - `npc_find_target()` - Intelligent target selection
  - `npc_switch_opponents()` - Tactical opponent switching
  - `npc_rescue()` - Rescue allies and masters
  - `npc_should_call_companion()` - Determine if NPC should summon companion
  - `move_on_path()` - Follow predefined patrol routes
  - `mobile_echos()` - Handle mob echo messages
  - `can_continue()` - Check if mob can perform actions

#### mob_class.c/h - Class Behaviors
- **Purpose**: Class-specific combat behaviors
- **Supported Classes**:
  - **Warrior**: Bash, shield punch, rescue tactics
  - **Rogue**: Backstab, trip, dirt kick, sneak attacks
  - **Monk**: Spring leap, stunning fist, quivering palm
  - **Ranger**: Call animal companion, rescue, ranged combat
  - **Paladin/Blackguard**: Call mount, mount management, lay on hands, smite evil, turn undead
  - **Dragonrider**: Call dragon mount, mount management, rescue tactics
  - **Berserker**: Rage, headbutt, aggressive tactics
  - **Bard**: Performance, trip, dirt kick

#### mob_race.c/h - Racial Behaviors
- **Purpose**: Race-specific special abilities
- **Supported Races**:
  - **Dragons**: Breath weapon, tail sweep, frightful presence
  - **Animals**: Rage ability
  - **Other races**: Extensible framework for additional abilities

#### mob_spells.c/h - Spell Casting
- **Purpose**: NPC spellcasting AI
- **Features**:
  - Companion calling for caster classes:
    - Druids: Animal companions
    - Wizards/Sorcerers: Familiars
    - Summoners/Necromancers: Eidolons/Undead cohorts
    - Shadowdancers: Shadow companions
  - Smart buff selection with saturation checks
  - Offensive spell selection (single-target and AoE)
  - Healing prioritization
  - Summoning (elementals and undead)
  - Class-appropriate spell restrictions

#### mob_psionic.c/h - Psionic Powers
- **Purpose**: Psionic power manifestation for psionicist NPCs
- **Features**:
  - Defensive power buffing
  - Offensive power selection
  - Power point management
  - Valid power filtering

#### mob_memory.c/h - Memory System
- **Purpose**: Allow mobs to remember and seek revenge
- **Functions**:
  - `remember()` - Add player to memory
  - `forget()` - Remove player from memory
  - `is_in_memory()` - Check if player is remembered
  - `clearMemory()` - Wipe all memories

## Main Activity Loop

The `mobile_activity()` function is the heart of the mob AI system, executed each game tick:

### Execution Flow

1. **Pre-checks**
   - Skip extracted mobs (MOB_NOTDEADYET flag)
   - Skip players
   - Skip mobs with MOB_NO_AI flag
   - Skip stunned/paralyzed/dazed mobs

2. **Special Procedures**
   - Execute if MOB_SPEC flag is set
   - If spec proc returns true, skip to next mob

3. **Combat Behaviors** (if fighting and level > NEWBIE_LEVEL)
   - Companion calling (50% chance if appropriate)
   - 25% chance: Racial behaviors
   - 25% chance: Ability behaviors
   - 25% chance: Assigned spells (if knows any)
   - Remaining: Class behaviors or offensive spells

4. **Non-Combat Behaviors**
   - Companion calling (10% chance if appropriate)
   - Spellup (6.25% chance for casters)
   - Mobile echos
   - Scavenging (10% chance if MOB_SCAVENGER)
   - Aggression checks
   - Memory revenge
   - Helper/guard assistance
   - Movement (paths, hunting, random)

5. **Position Management**
   - Return to default position if sentinel

## Combat Behaviors

### Class-Specific Tactics

Each class has unique combat behaviors that reflect their role:

#### Warriors
```c
- Rescue allies/master (priority)
- Switch opponents (33% chance)
- Bash or shield punch (50/50)
```

#### Rogues
```c
- Sneak attack scaling with level
- Trip attempt (50% chance)
- Dirt kick (if trip fails)
- Backstab (fallback)
```

#### Monks
```c
- Switch opponents (33% chance)
- Stunning fist (16.7% chance)
- Spring leap (66.7% chance)
- Quivering palm (rare)
```

#### Paladins/Blackguards
```c
- Call mount (if appropriate)
- Mount their mount if not already mounted
- Rescue allies (priority)
- Switch opponents (33% chance)
- Smite evil (if target is evil)
- Turn undead (if target is undead)
- Lay on hands (if health < 25%)
```

#### Dragonriders
```c
- Call dragon mount (if appropriate)
- Mount their dragon if not already mounted
- Rescue allies (priority)
- Switch opponents (33% chance)
- TODO: Dragon breath weapon usage
- TODO: Dragoon point spell usage
```

#### Rangers
```c
- Call animal companion (if appropriate)
- Rescue allies/master (priority)
- Switch opponents (33% chance)
- Kick
```

### Spell Selection Algorithm

The spell selection system uses intelligent prioritization:

1. **Buff Saturation Check**: Count existing buffs, reduce frequency if well-buffed (5+ buffs)
2. **Priority Buffs**: Try important combat buffs first (Stoneskin, Sanctuary, Haste, etc.)
3. **Random Selection**: Pick from valid spells based on level
4. **Special Restrictions**: 
   - No invisibility for shopkeepers
   - No invisibility for quest mobs
   - No invisibility for mounts

### Target Selection

The `npc_find_target()` function builds a target list based on:
- Mobs in memory (revenge targets)
- Current hunting target
- Attackers (those fighting the mob)
- Current combat target

## Non-Combat Behaviors

### Movement Patterns

#### Path Following
Mobs with defined paths follow waypoints:
```c
- Check PATH_SIZE for remaining waypoints
- Honor PATH_DELAY between movements
- Use pathfinding to reach next waypoint
- Increment PATH_INDEX after each move
```

#### Random Movement
- 50% chance to move if not sentinel
- Select random valid direction
- Respect zone boundaries (MOB_STAY_ZONE)
- Avoid death rooms and no-mob rooms

#### Hunting
- MOB_HUNTER flag enables victim tracking
- Uses pathfinding to pursue target
- Can hunt across zones if not restricted

### Scavenging
Mobs with MOB_SCAVENGER flag:
- 10% chance per tick to check for items
- Pick up most valuable item in room
- Automatically equip if possible

### Aggression System

Aggression triggers based on flags:
- **MOB_AGGRESSIVE**: Attack anyone
- **MOB_AGGR_EVIL**: Attack evil characters
- **MOB_AGGR_GOOD**: Attack good characters
- **MOB_AGGR_NEUTRAL**: Attack neutral characters

Special checks:
- Animals won't attack characters with FEAT_SOUL_OF_THE_FEY
- Undead won't attack characters with FEAT_ONE_OF_US
- Encounter mobs only attack appropriate level targets

### Helper/Guard Behaviors

**Helper Mobs** (MOB_HELPER):
- Assist any NPC being attacked by players
- Won't help animals
- Jump into combat to aid allies

**Guard Mobs** (MOB_GUARD):
- Only help citizens (MOB_CITIZEN flag)
- Protect mission-specific allies
- More selective than regular helpers

## Memory System

The memory system allows mobs to remember and seek revenge:

### How Memory Works
1. **Adding Memory**: When a player harms a mob with MOB_MEMORY flag
2. **Storage**: Player ID stored in linked list
3. **Recognition**: Check each room occupant against memory
4. **Revenge**: Attack remembered players on sight

### Memory Restrictions
- Only works on players (not other NPCs)
- Respects PRF_NOHASSLE flag
- Cleared on mob death/extraction
- Not saved across reboots

## Spell and Psionic Systems

### Spellup Logic

The spellup system provides intelligent buffing:

1. **Companion Calling** (Priority for casters)
   - Check if companion already exists
   - 10% chance out of combat, 50% in combat
   - Class-appropriate companion types

2. **Buff Saturation Prevention**
   - Count existing buffs
   - 75% skip chance if 5+ buffs active
   - Prevents over-buffing

3. **Priority System**
   ```c
   Priority buffs (always try first):
   - SPELL_STONESKIN
   - SPELL_SANCTUARY
   - SPELL_HASTE
   - SPELL_GLOBE_OF_INVULN
   - SPELL_MIRROR_IMAGE
   - SPELL_BLUR
   ```

4. **Summoning** (restricted to level 30+ mobs)
   - Animate dead from corpses (25% chance)
   - Summon elementals (10% chance)
   - Creates group if needed

5. **Healing**
   - Prioritize low health allies
   - Use group heal if available
   - Fall back to single-target heals

### Offensive Spell Selection

1. **AoE Decision**: Use AoE spells if 2+ targets
2. **Spell Level Check**: Only cast spells within level range
3. **Effect Stacking**: Avoid casting spells target already has
4. **Class Appropriateness**: Clerics attempt turn undead

### Psionic Powers

Similar to spells but using power points:
- Defensive powers for buffing
- Offensive powers for combat
- Valid power filtering based on level
- Direct manifestation (no command parsing)

## Utility Functions

### can_continue()
Checks if mob can perform actions:
- Not in combat with invalid target
- No wait state
- Position > sitting
- Not casting
- Has hit points
- Target not nearly dead

### npc_find_target()
Intelligent target selection:
- Creates target list from room
- Includes memory targets
- Includes current attackers
- Returns random valid target
- Sets num_targets for AoE decisions

### npc_switch_opponents()
Tactical target switching:
- Visibility check
- Position check
- Single-file room handling
- Stops current fight
- Initiates attack on new target

### npc_rescue()
Rescue priority system:
1. Rescue master (if charmed)
2. Rescue group members
3. Health threshold checks
4. Use perform_rescue()

### npc_should_call_companion()
Companion calling decision logic:
- Checks if companion already exists
- Verifies class compatibility
- Considers combat state (50% in combat, 10% out)
- Checks health threshold (won't call if < 25% HP)
- Returns true/false for calling decision

## Mob Flags

Key flags affecting behavior:

### Combat Flags
- **MOB_AGGRESSIVE**: Attack players on sight
- **MOB_AGGR_EVIL/GOOD/NEUTRAL**: Selective aggression
- **MOB_WIMPY**: Only attack sleeping targets
- **MOB_MEMORY**: Remember and revenge
- **MOB_HUNTER**: Track and pursue targets

### Movement Flags
- **MOB_SENTINEL**: Don't move from spawn room
- **MOB_STAY_ZONE**: Don't leave zone
- **MOB_LISTEN**: Move toward nearby fights

### Behavior Flags
- **MOB_SCAVENGER**: Pick up valuable items
- **MOB_HELPER**: Assist other NPCs
- **MOB_GUARD**: Protect citizens
- **MOB_SPEC**: Has special procedure
- **MOB_NO_AI**: Disable all AI behaviors
- **MOB_NOCLASS**: Disable class behaviors
- **MOB_BUFF_OUTSIDE_COMBAT**: Buff when not fighting (Campaign DL)

### Special Flags
- **MOB_NOKILL**: Protected, won't remember targets
- **MOB_ENCOUNTER**: Level-restricted aggression
- **MOB_C_MOUNT**: Can serve as mount
- **MOB_CITIZEN**: Protected by guards

## Builder Guidelines

### Creating Effective Mobs

#### 1. Choose Appropriate Class
- Match class to intended role
- Warriors for tanks
- Rogues for damage dealers
- Casters for magic support
- Paladins/Clerics for healing

#### 2. Set Proper Flags
```
Aggressive guard example:
- MOB_AGGRESSIVE
- MOB_GUARD
- MOB_MEMORY
- MOB_SENTINEL

Wandering merchant:
- MOB_SPEC (shop_keeper)
- MOB_SCAVENGER
- No aggressive flags

Patrol guard:
- MOB_MEMORY
- MOB_HELPER
- Path waypoints defined
```

#### 3. Level Considerations
- Mobs below NEWBIE_LEVEL have limited AI
- Level affects spell selection
- Higher level = more sophisticated tactics

#### 4. Avoid Common Pitfalls
- Don't set MOB_SPEC without assigning spec proc
- Don't make shopkeepers aggressive
- Be careful with MOB_MEMORY on frequently killed mobs
- Test path waypoints thoroughly

### Assigning Spells

Mobs can know specific spells via `spells_known` array:
```c
// In mob setup code:
MOB_KNOWS_SPELL(mob, SPELL_FIREBALL) = 1;
MOB_KNOWS_SPELL(mob, SPELL_LIGHTNING_BOLT) = 1;
```

This gives more control than relying on class defaults.

## Examples

### Example 1: City Guard
```
Class: WARRIOR
Level: 25
Flags: MOB_SENTINEL, MOB_GUARD, MOB_MEMORY, MOB_HELPER
Behavior:
- Stays at post (SENTINEL)
- Helps citizens under attack (GUARD)
- Remembers criminals (MEMORY)
- Assists other guards (HELPER)
- Uses warrior combat skills
```

### Example 2: Dragon Boss
```
Race: RACE_TYPE_DRAGON
Class: SORCERER
Level: 50
Flags: MOB_AGGRESSIVE, MOB_MEMORY, MOB_SENTINEL
Behavior:
- Breath weapon attack
- Tail sweep
- Frightful presence
- Offensive spellcasting
- Remembers enemies
```

### Example 3: Patrol Route
```c
// Setting up a patrol path:
PATH_SIZE(mob) = 4;
PATH_INDEX(mob) = 0;
PATH_DELAY(mob) = 2; // Wait 2 ticks between moves
PATH_RESET(mob) = 2;
GET_PATH(mob, 0) = 3001; // Room vnums
GET_PATH(mob, 1) = 3002;
GET_PATH(mob, 2) = 3003;
GET_PATH(mob, 3) = 3002;
```

### Example 4: Smart Caster
```
Class: WIZARD
Level: 30
Flags: MOB_MEMORY, MOB_BUFF_OUTSIDE_COMBAT
Assigned Spells: Custom spell list
Behavior:
- Calls familiar when appropriate
- Buffs self when not in combat
- Prioritizes defensive spells
- Uses AoE when multiple enemies
- Summons elementals for support
- Smart target selection
```

### Example 5: Mounted Paladin
```
Class: PALADIN
Level: 20
Flags: MOB_SENTINEL, MOB_MEMORY
Behavior:
- Calls mount at start of combat
- Automatically mounts when not mounted
- Uses mount for tactical advantage
- Smites evil opponents
- Heals self when low health
- Rescues allies in danger
```

### Example 6: Summoner with Eidolon
```
Class: SUMMONER
Level: 25
Flags: MOB_AGGRESSIVE, MOB_MEMORY
Behavior:
- Summons eidolon companion
- Coordinates attacks with eidolon
- Uses buff spells on self and companion
- Strategic spell selection
- Memory-based targeting
```

### Example 7: Dragon Rider
```
Class: DRAGONRIDER
Level: 30
Flags: MOB_MEMORY, MOB_AGGRESSIVE
Behavior:
- Calls dragon mount when needed
- Automatically mounts dragon when not mounted
- Rescues allies in danger
- Switches opponents tactically
- Future: Dragon breath weapon usage
- Future: Dragoon point spell casting
```

## Performance Considerations

### Optimization Tips
1. **Limit Active AI**: Use MOB_NO_AI for decoration mobs
2. **Zone Restrictions**: Use MOB_STAY_ZONE to limit pathfinding
3. **Sentinel Mobs**: Reduce movement calculations
4. **Spell Limits**: Don't assign too many spells to one mob
5. **Memory Management**: Clear memory regularly for long-lived mobs

### System Limits
- MAX_LOOPS (40): Maximum spell selection attempts
- RESCUE_LOOP (20): Maximum rescue target searches
- Spell arrays: Fixed sizes for spell lists
- Path size: Limited by PATH_SIZE per mob

## Debugging

### Common Issues and Solutions

#### Mob Not Attacking
- Check aggression flags
- Verify level restrictions
- Ensure mob can see target
- Check position (must be > sitting)

#### Spell Not Cast
- Verify mob level meets spell requirements
- Check class spell availability
- Ensure not in cooldown/wait state
- Verify target is valid

#### Path Not Working
- Verify room vnums are correct
- Check PATH_SIZE is set
- Ensure rooms are accessible
- Verify no NOMOB flags on rooms

#### Memory Not Working
- Confirm MOB_MEMORY flag is set
- Check target is player (not NPC)
- Verify mob can see target
- Ensure not NOHASSLE player

## Future Enhancements

Potential improvements to the system:

1. **Learning AI**: Mobs that adapt to player tactics
2. **Group Coordination**: Better team tactics for mob groups
3. **Emotional States**: Fear, anger, morale affecting behavior
4. **Advanced Pathfinding**: Dynamic obstacle avoidance
5. **Conversation AI**: More sophisticated dialog responses
6. **Tactical Positioning**: Formation fighting, flanking
7. **Resource Management**: Spell/ability cooldown tracking
8. **Environmental Awareness**: Using room features tactically

---

*Last Updated: 2025*
*LuminariMUD Mob Behavior System v2.2*
*Changes:*
*- v2.2: Added automatic mounting for Paladins, Blackguards, and Dragonriders*
*- v2.2: NPCs now use compute_ability() for ride checks (allows NPCs to mount)*
*- v2.2: Removed upper size limit for mounts (dragons can be any size)*
*- v2.1: Added Companion Calling System for NPCs*