# Comprehensive Trap System Implementation

## Overview
This document describes the new trap system for Luminari MUD, based on Neverwinter Nights mechanics.

## What Has Been Implemented

### 1. Data Structures (structs.h)

#### Trap Constants
- **20 Trap Types**: acid blob, acid splash, electrical, fire, frost, gas, holy, negative, sonic, spike, tangle, dart, pit, dispel, ambush, boulder, wall smash, spider horde, glyph, skeletal hands
- **5 Severity Levels**: minor, average, strong, deadly, epic
- **7 Trigger Types**: enter room, leave room, open door, unlock door, open container, unlock container, get object
- **4 Save Types**: none, reflex, fortitude, will
- **11 Special Effects**: paralysis, slow, stun, poison, ability drain, level drain, entangle, blind, feeblemind, summon creature
- **9 Trap Flags**: detected, disarmed, triggered, auto-generated, recoverable, area effect, one-shot, magical, mechanical

#### Trap Data Structure
```c
struct trap_data {
    int trap_type;               // Type of trap (TRAP_TYPE_*)
    int severity;                // Severity level (TRAP_SEVERITY_*)
    int trigger_type;            // How it triggers (TRAP_TRIGGER_*)
    int detect_dc;               // DC to detect
    int disarm_dc;               // DC to disarm
    int save_dc;                 // DC for saving throw
    int save_type;               // Type of save (TRAP_SAVE_*)
    int damage_dice_num;         // Number of dice
    int damage_dice_size;        // Die size
    int damage_type;             // Damage type (DAM_*)
    int special_effect;          // Special effect (TRAP_SPECIAL_*)
    int special_duration;        // Duration in rounds
    int area_radius;             // Area effect radius
    int max_targets;             // Max targets in area
    long flags;                  // Trap flags (TRAP_FLAG_*)
    int trigger_vnum;            // Triggering object vnum
    int trigger_direction;       // Door direction
    char *trap_name;             // Custom name
    char *trigger_message_char;  // Message to character
    char *trigger_message_room;  // Message to room
    struct trap_data *next;      // For linked lists
};
```

#### Integration with Existing Structures
- **obj_data**: Added `struct trap_data *trap` field for traps on containers/doors
- **room_data**: Added `struct trap_data *traps` field for room traps (linked list, supports multiple traps)

### 2. Zone/Room/Object Flags

#### Flags Used
- `ZONE_RANDOM_TRAPS` (13): Auto-generate traps randomly throughout the zone on reset
- `ROOM_RANDOM_TRAP` (36): Always auto-generate trap in this specific room on zone reset
- `ITEM_TRAPPED` (113): This object has a trap attached (informational flag)

#### Flag Behavior

**ZONE_RANDOM_TRAPS Flag:**
When set on a zone, traps will be randomly generated throughout the zone during zone reset. The frequency is controlled by `NUM_OF_ZONE_ROOMS_PER_RANDOM_TRAP` (currently 33), meaning approximately 1 trap per 33 rooms in the zone.

For example, a zone with 100 rooms and `ZONE_RANDOM_TRAPS` flag will get approximately 3 random traps placed in different rooms each reset.

**ROOM_RANDOM_TRAP Flag:**
When set on a specific room, that room will ALWAYS get a trap during zone reset, regardless of whether the zone has the `ZONE_RANDOM_TRAPS` flag. This allows builders to mark specific dangerous rooms (treasure rooms, boss lairs, etc.) as always trapped.

**Priority System:**
1. Rooms with `ROOM_RANDOM_TRAP` flag always get traps (100% chance)
2. Other rooms in zones with `ZONE_RANDOM_TRAPS` flag have ~3% chance (1 in 33)

**ITEM_TRAPPED Flag:**
Set automatically when a trap is attached to an object (container/door). This is informational only and helps identify trapped objects.

### 3. Trap System Functions (traps.h & traps_new.c)

#### Trap Creation and Management
```c
struct trap_data *create_trap(int trap_type, int severity, int trigger_type);
void free_trap(struct trap_data *trap);
struct trap_data *copy_trap(struct trap_data *source);
void attach_trap_to_room(struct trap_data *trap, room_rnum room);
void attach_trap_to_object(struct trap_data *trap, struct obj_data *obj);
void remove_trap_from_room(struct trap_data *trap, room_rnum room);
void remove_trap_from_object(struct obj_data *obj);
```

#### Trap Generation System
```c
struct trap_data *generate_random_trap(int zone_level);
int determine_trap_severity(int zone_level);
int get_random_trap_type(void);
void auto_generate_zone_traps(zone_rnum zone);
void auto_generate_room_trap(room_rnum room, int zone_level);
void auto_generate_object_trap(struct obj_data *obj, int zone_level);
```

#### Trap Data Tables
- `trap_severity_table[NUM_TRAP_SEVERITIES]`: DCs and damage multipliers by severity
- `trap_type_table[NUM_TRAP_TYPES]`: Templates for each trap type with:
  - Trap name
  - Damage type
  - Save type
  - Special effect
  - Trigger messages (char & room)
  - Detection message
  - Area effect flag
  - Default area radius

### 4. Trap Severity Scaling

The system automatically scales trap properties based on severity and zone level:

#### Minor (Level 1-5)
- Detect DC: 15
- Disarm DC: 15
- Save DC: 15
- Example: 3d6 acid blob, 2d8 acid splash

#### Average (Level 6-10)
- Detect DC: 20
- Disarm DC: 20
- Save DC: 20
- Example: 6d6 acid blob, 3d8 acid splash

#### Strong (Level 11-15)
- Detect DC: 25
- Disarm DC: 25
- Save DC: 25
- Example: 12d6 acid blob, 5d8 acid splash

#### Deadly (Level 16-20)
- Detect DC: 25
- Disarm DC: 30
- Save DC: 25
- Example: 18d6 acid blob, 8d8 acid splash

#### Epic (Level 21+)
- Detect DC: 35
- Disarm DC: 35
- Save DC: 35
- Example: 30d6 acid blob, 40d4 frost

## What Needs To Be Implemented

### 5. Trap Detection & Disarming (TODO)
- Detection with Perception skill + perk bonuses
- Disarming with Disable Device skill + perk bonuses
- Integration with PERK_ROGUE_TRAPFINDING_EXPERT
- Integration with PERK_ROGUE_TRAP_SENSE
- Visual indicators when traps are detected

### 6. Trap Triggering & Effects (TODO)
- Check for trigger conditions (enter room, open door, etc.)
- Apply damage with proper damage types
- Apply special effects (paralysis, stun, slow, etc.)
- Area effect handling for multi-target traps
- Saving throw mechanics
- Integration with existing damage system
- Integration with existing spell effect system

### 7. Trap Component Recovery (TODO)
- PERK_ROGUE_TRAP_SCAVENGER implementation
- Component value calculation based on trap severity
- Component item creation
- Recovery skill check

### 8. Player Commands (TODO)
- Update `do_detecttrap` to use new system
- Update `do_disabletrap` to use new system
- Add `do_trapinfo` for detailed trap information
- Update command help files

### 9. OLC Integration (TODO)
- Add trap editing to oedit for placing traps on objects
- Add trap editing to redit for placing traps in rooms
- Trap property editors (type, severity, trigger, DCs, etc.)
- Trap flag editors

### 10. Zone Reset Integration (TODO)
- Call `auto_generate_zone_traps()` during zone reset
- Handle ZONE_RANDOM_TRAPS flag
- Handle ROOM_RANDOM_TRAP flag
- Clean up auto-generated traps on reset

### 11. Legacy System Migration (TODO)
- Convert old ITEM_TRAP objects to new system
- Migrate existing trap objects in world files
- Update documentation
- Backward compatibility layer

### 12. Balance & Testing (TODO)
- Test all trap types and severities
- Balance damage values
- Balance DCs for different level ranges
- Test area effect traps
- Test special effects
- Test perk integration
- Performance testing with many traps

## Example Usage (When Complete)

### Builder Creating a Trap
```
> oedit <container_vnum>
> trap add
Trap Type: fire
Severity: strong
Trigger: open_container
> save
```

### Player Detecting a Trap
```
> detect trap
You carefully search for traps...
You notice a fire trap!
```

### Player Disarming a Trap
```
> disable trap
You attempt to disable the trap... SUCCESS!
You recover some trap components.
```

### Auto-Generated Traps
```
Zone reset occurs...
TRAP: Auto-generated fire trap (severity: strong) in room 12345
TRAP: Auto-generated acid splash trap (severity: average) on object 67890
TRAP: Auto-generated 5 traps in zone 123
```

## Files Modified/Created

### Modified
1. `/home/krynn/code/src/structs.h` - Added trap constants, struct trap_data, updated room_data and obj_data
2. `/home/krynn/code/src/traps.h` - Complete rewrite with new function prototypes and data structures

### Created
1. `/home/krynn/code/src/traps_new.c` - New trap system implementation (partial, needs completion)

### To Be Modified
1. `src/traps.c` - Needs update to use new system
2. `src/oedit.c` - Needs trap editing support
3. `src/redit.c` - Needs trap editing support
4. `src/db.c` - Needs zone reset integration
5. `src/act.movement.c` - Needs trap trigger checks
6. `src/act.item.c` - Needs trap trigger checks for containers/doors

## Integration Points

### Perk System
The trap system is designed to integrate with these rogue perks:
- `PERK_ROGUE_TRAPFINDING_EXPERT_1` (109): +3/rank to find/disable traps
- `PERK_ROGUE_TRAPFINDING_EXPERT_2` (113): +4/rank to find/disable traps
- `PERK_ROGUE_TRAP_SENSE_1` (116): +2/rank to saves/AC vs traps
- `PERK_ROGUE_TRAP_SCAVENGER` (118): Salvage trap components

### Skill System
- `ABILITY_PERCEPTION`: Used to detect traps
- `ABILITY_DISABLE_DEVICE`: Used to disarm traps

### Combat System
- Trap damage uses existing damage types (DAM_*)
- Trap effects use existing spell effects where applicable
- Special trap effects may need new affect types

## Benefits of This System

1. **Flexible**: Easy to add new trap types
2. **Scalable**: Automatically adjusts to zone level
3. **D&D-Compliant**: Based on NWN which follows D&D 3.5 rules
4. **Extensible**: Can attach traps to rooms, objects, doors
5. **Balanced**: Multiple severity levels with appropriate DCs
6. **Integrated**: Works with existing perk and skill systems
7. **Builder-Friendly**: Auto-generation reduces manual work
8. **Performance**: Efficient data structures, no unnecessary objects

## Next Steps

1. Complete the trap trigger and effect system
2. Implement trap detection and disarming
3. Add perk integration
4. Create player commands
5. Add OLC support
6. Integrate with zone reset system
7. Test and balance
8. Update documentation
9. Migrate legacy traps
10. Deploy to production
