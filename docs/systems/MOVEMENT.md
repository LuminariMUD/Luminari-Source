# LuminariMUD Movement System

## Overview
The movement system handles all character and object locomotion through the game world, including walking, climbing, swimming, flying, and falling. It's built as a modular system with nine components handling different aspects of movement.

## Architecture

### Module Structure
```
movement.c                  Core movement logic and commands (main module)
├── movement_validation.c   Movement capability checks  
├── movement_cost.c         Speed and cost calculations
├── movement_position.c     Position/stance management
├── movement_doors.c        Door and lock handling
├── movement_falling.c      Gravity and fall damage
├── movement_events.c       Post-movement event processing
├── movement_messages.c     Movement message display
└── movement_tracks.c       Trail/tracking system
```

### Core Files

| Module | Purpose | Key Functions |
|--------|---------|---------------|
| **movement.c** | Main movement execution | `do_simple_move()`, `perform_move()`, `do_enter()` |
| **movement_validation.c** | Terrain/ability checks | `has_boat()`, `has_flight()`, `can_climb()` |
| **movement_cost.c** | Movement point calculation | `get_speed()`, `calculate_movement_cost()` |
| **movement_position.c** | Stance changes | `change_position()`, `do_stand()`, `do_sit()` |
| **movement_doors.c** | Door operations | `do_gen_door()`, `has_key()`, `ok_pick()` |
| **movement_falling.c** | Fall mechanics | `event_falling()`, `char_should_fall()` |
| **movement_events.c** | Event processing | `process_movement_events()`, `process_room_damage()` |
| **movement_messages.c** | Message display | `display_leave_messages()`, `display_enter_messages()` |
| **movement_tracks.c** | Trail creation | `create_tracks()`, `should_create_tracks()` |

## Movement Flow

### Standard Movement Process
```
1. Command Input (n/s/e/w/etc)
   ↓
2. perform_move() → perform_move_full()
   ├── Check fighting/paralysis
   ├── Check exit exists
   ├── Handle closed doors (autodoor)
   └── Check encounters
   ↓
3. do_simple_move()
   ├── Validate terrain requirements
   ├── Check movement restrictions
   ├── Calculate movement cost
   ├── Deduct movement points
   └── Execute room transfer
   ↓
4. Post-Movement Processing
   ├── Display messages
   ├── Create tracks
   ├── Process events/triggers
   └── Move followers
```

## Movement Validation

### Terrain Requirements

| Terrain Type | Requirement | Check Function |
|--------------|-------------|----------------|
| Water (No Swim) | Boat or flight | `has_boat()` |
| Water (Swim) | Swimming skill check | `has_boat()` |
| Flying/Air | Flight capability | `has_flight()` |
| Underwater | Water breathing | `has_scuba()` |
| High Mountain | Climbing ability | `can_climb()` |
| Ocean | Ship (players blocked) | Hard check |

### Movement Restrictions
- **Room Flags**: TUNNEL, STAFFROOM, NOFLY, SIZE restrictions
- **Zone Flags**: CLOSED, NOIMMORT
- **Character State**: Paralyzed, grappled, entangled, sleeping
- **Special**: House ownership, alignment restrictions (holy/unholy)

## Movement Cost System

### Speed Calculation
Base speed is 30 feet, modified by:
- **Race**: Dwarves/Halflings (25), Fae flight (60)
- **Spells**: Haste (+30), Shadow Walk (400), Slow (÷2)
- **Class**: Monk bonus (+10-60), Fast Movement (+10)
- **Conditions**: Blind (÷2), Entangled (÷2)

### Movement Point Cost
```c
base_cost = (movement_loss[from_sector] + movement_loss[to_sector]) / 2
cost *= 10  // New system multiplier

// Modifiers
if (woodland_stride && outdoors) cost = 1
if (difficult_terrain) cost *= 2
if (spot_mode) cost *= 2
if (listen_mode) cost *= 2
if (reclining) cost *= 4

// Skill reduction
cost -= skill_roll(SURVIVAL)
cost -= (speed - 30)

// Minimum
cost = MAX(5, cost)  // or 1 with shadow walk
```

### Sector Movement Costs
| Sector | Base Cost | Description |
|--------|-----------|-------------|
| Inside/City | 1 | Normal indoor/urban |
| Field | 2 | Open terrain |
| Forest | 3 | Light woods |
| Hills | 4 | Rolling hills |
| Mountain | 7 | Steep terrain |
| High Mountain | 10 | Extreme altitude |
| Water (Swim) | 4 | Swimmable water |
| Desert | 3 | Sandy terrain |
| Ocean | 11 | Open ocean |
| Underwater | 5 | Submerged |

## Special Systems

### Door System
Handles both room exits and container objects with lock levels:
- **Lock Levels**: Easy (DC 15), Medium (DC 20), Hard (DC 25), Pickproof
- **Key Types**: Standard, Evaporating (vnums 1300-1399), Extract-on-use
- **Operations**: Open, close, lock, unlock, pick
- **Autodoor**: Automatic door opening with PRF_AUTODOOR

### Falling System
Event-driven system for gravity effects:
- **Triggers**: No flight in FLY_NEEDED rooms, failed climbing
- **Damage**: 1d6 per 10 feet + 20 base
- **Mitigation**: Slow Fall, Safefall, Feather Fall, Draconian wings

### Position System
Nine position states affecting movement:
| Position | Can Move | Special Effects |
|----------|----------|-----------------|
| Dead | No | Character is dead |
| Mortally Wounded | No | Dying |
| Incapacitated | No | Helpless |
| Stunned | No | Temporary paralysis |
| Sleeping | No | Unaware |
| Reclining | Crawl (4x cost) | Prone, AC penalties |
| Resting | No | Recovering |
| Sitting | No | Can use furniture |
| Fighting | Yes | In combat |
| Standing | Yes | Normal state |

### Track System
Creates persistent movement trails (disabled in DL/FR campaigns):
- Stores: Name, race, direction, timestamp
- Pruning: Automatic cleanup after threshold
- Creation: Only for non-immortals without nohassle

### Mount System
- Mount speed overrides rider speed
- Ride skill checks to avoid being thrown
- Movement points deducted from rider (mounts exempt)
- Shared movement with rider/mount

## Commands

### Basic Movement
- **n/s/e/w/u/d**: Cardinal directions
- **ne/nw/se/sw**: Diagonal directions (if enabled)

### Special Movement
- **enter** [portal/door]: Enter portals or buildings
- **leave**: Exit to outdoors
- **follow** [target]: Follow another character
- **flee** [direction]: Escape combat

### Position Changes
- **stand**: Stand up (uses move action)
- **sit** [furniture]: Sit down
- **rest**: Rest position
- **sleep**: Go to sleep
- **wake** [target]: Wake up
- **recline**: Lie prone

### Door Commands
- **open/close** [door/direction]: Door operations
- **lock/unlock** [door/direction]: Lock operations
- **pick** [door/direction]: Pick locks (Disable Device)

### Special Commands
- **pullswitch/push**: Activate switches
- **transposition**: Swap places with eidolon
- **unstuck**: Teleport to start room (cost: XP/gold)
- **lastroom**: Return to previous location

## Movement Events

### Pre-Movement
- Special procedure checks
- Leave triggers (mob/room/object)
- Combat checks (attacks of opportunity)
- Wall checks (Wall of Force, etc.)

### Post-Movement
- Entry triggers and memory
- Room damage (spike growth/stones)
- Trap checks (TRAP_TYPE_ENTER_ROOM)
- Vampire weaknesses (sunlight/water)
- Random encounters (wilderness)
- Trail creation

## Integration Points

### Combat System
- Attacks of opportunity on standing
- No Retreat feat (free AoO on flee)
- Position modifiers to AC
- Movement restrictions while fighting

### Skill System
| Skill | Usage |
|-------|-------|
| Athletics | Swimming, climbing |
| Acrobatics | Fall damage reduction, blind movement |
| Ride | Mount control |
| Survival | Movement cost reduction |
| Disable Device | Lock picking |
| Sleight of Hand | Treasure chest picking |

### Spell Integration
- **Movement Enhancement**: Fly, Spider Climb, Water Walk, Shadow Walk
- **Movement Prevention**: Web, Entangle, Grapple
- **Position Effects**: Sleep, Paralysis
- **Terrain Creation**: Spike Growth, Wall spells

## Configuration

### Key Constants
```c
#define NUM_OF_DIRS 10        // Total directions
#define CONFIG_TUNNEL_SIZE 2  // Max PCs in tunnel room
#define NEWBIE_LEVEL 5        // Level for zone warnings
#define TRAIL_PRUNING_THRESHOLD 3600  // 1 hour
```

### Campaign Differences
- **Default**: Full track system enabled
- **CAMPAIGN_DL/FR**: Tracks disabled, modified encounter checks

## API Reference

### Core Functions
```c
// Main movement execution
int do_simple_move(ch, dir, need_specials_check)
int perform_move(ch, dir, need_specials_check)
int perform_move_full(ch, dir, need_specials_check, recursive)

// Validation
int has_boat(ch, going_to)
int has_flight(ch)
int can_climb(ch)

// Speed/Cost
int get_speed(ch, to_display)
int calculate_movement_cost(ch, from, to, riding)

// Position
int change_position(ch, new_position)
bool can_stand(ch)

// Doors
int has_key(ch, key)
int ok_pick(ch, keynum, pickproof, scmd, door)
```

## Developer Notes

### Adding New Movement Types
1. Define new sector type in `structs.h`
2. Add movement cost to `movement_loss[]` array
3. Create validation function in `movement_validation.c`
4. Add checks to `do_simple_move()`

### Common Issues
- Mounts don't use movement points (intentional)
- Wilderness movement creates rooms dynamically
- Single-file rooms use special linked list manipulation
- Falling events must check for teleportation

### Performance Considerations
- Cache `get_speed()` results when used multiple times
- Batch follower movements
- Use bitwise operations for flag checks
- Minimize room searches in loops
