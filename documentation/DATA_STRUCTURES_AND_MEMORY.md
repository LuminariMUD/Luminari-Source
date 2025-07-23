# LuminariMUD Data Structures and Memory Management

## Overview

LuminariMUD uses a complex system of interconnected data structures to represent the game world, characters, objects, and player connections. The primary data structures are defined in `structs.h` and follow a hierarchical organization with extensive use of linked lists, arrays, and pointer relationships.

## Core Data Structures

### 1. Character Data (`struct char_data`)

The `char_data` structure is the master structure for both Player Characters (PCs) and Non-Player Characters (NPCs):

```c
struct char_data {
    int pfilepos;              // PC playerfile position and ID
    mob_rnum nr;               // NPC real instance number
    int coords[2];             // Wilderness coordinates
    room_rnum in_room;         // Current room location
    room_rnum was_in_room;     // Previous location (for linkdead)
    int wait;                  // Action delay counter
    
    // Core character data
    struct char_player_data player;              // Name, class, race, etc.
    struct char_ability_data real_abils;         // Base abilities
    struct char_ability_data aff_abils;          // Modified abilities
    struct char_point_data points;               // HP, MP, movement
    struct char_special_data char_specials;      // Special flags/data
    struct player_special_data *player_specials; // PC-only data
    struct mob_special_data mob_specials;        // NPC-only data
    
    // Equipment and inventory
    struct obj_data *equipment[NUM_WEARS];       // Worn equipment
    struct obj_data *carrying;                   // Inventory list
    struct bag_data *bags;                       // Container organization
    
    // Relationships and connections
    struct descriptor_data *desc;                // Network connection (PCs only)
    struct char_data *next_in_room;              // Room occupant list
    struct char_data *next;                      // Global character list
    struct char_data *next_fighting;             // Combat participant list
    struct follow_type *followers;               // Followers list
    struct char_data *master;                    // Following target
    struct group_data *group;                    // Group membership
    
    // Scripting and events
    long id;                                     // Unique DG script ID
    struct trig_proto_list *proto_script;        // Default triggers
    struct script_data *script;                  // Active script data
    struct list_data *events;                    // Event queue
    
    // Combat and status
    struct affected_type *affected;              // Active spell effects
    struct char_data *last_attacker;             // Combat tracking
    bool dead;                                   // Death status
};
```

**Key Relationships:**
- **Room Occupancy:** `next_in_room` creates linked list of characters in same room
- **Global List:** `next` links all characters in the game
- **Equipment:** Array of worn items plus inventory list
- **Network:** `desc` pointer connects to player's network connection

### 2. Room Data (`struct room_data`)

Represents locations in the game world:

```c
struct room_data {
    room_vnum number;                            // Virtual room number
    zone_rnum zone;                              // Zone assignment
    int coords[2];                               // Wilderness coordinates
    int sector_type;                             // Terrain type
    int room_flags[RF_ARRAY_MAX];                // Room properties
    long room_affections;                        // Active spell effects
    
    // Descriptions
    char *name;                                  // Room title
    char *description;                           // Room description
    struct extra_descr_data *ex_description;     // Additional descriptions
    
    // Connections and contents
    struct room_direction_data *dir_option[NUM_OF_DIRS]; // Exits
    struct obj_data *contents;                   // Objects in room
    struct char_data *people;                    // Characters in room
    
    // Lighting and atmosphere
    byte light;                                  // Light sources
    byte globe;                                  // Darkness sources
    
    // Scripting and special functions
    SPECIAL_DECL(*func);                         // Special procedure
    struct trig_proto_list *proto_script;        // Default triggers
    struct script_data *script;                  // Active scripts
    struct list_data *events;                    // Room events
    
    // Advanced features
    struct trail_data_list *trail_tracks;        // Tracking system
    struct moving_room_data *mover;              // Moving room data
    int harvest_material;                        // Harvestable resources
};
```

**Key Features:**
- **Directional Exits:** Array of pointers to exit data
- **Contents Lists:** Separate lists for objects and characters
- **Coordinate System:** Support for wilderness positioning
- **Dynamic Features:** Moving rooms, trails, harvestable materials

### 3. Object Data (`struct obj_data`)

Represents all items, equipment, and objects in the game:

```c
struct obj_data {
    obj_rnum item_number;                        // Unique object ID
    room_rnum in_room;                           // Room location (-1 if carried)
    
    // Object properties
    struct obj_flag_data obj_flags;              // Type, wear flags, etc.
    struct obj_affected_type affected[MAX_OBJ_AFFECT]; // Stat bonuses
    struct obj_weapon_poison weapon_poison;      // Weapon poison data
    
    // Descriptions and identification
    char *name;                                  // Keywords
    char *description;                           // Room description
    char *short_description;                     // Inventory description
    char *action_description;                    // Action messages
    struct extra_descr_data *ex_description;     // Additional descriptions
    
    // Container relationships
    struct obj_data *in_obj;                     // Container holding this object
    struct obj_data *contains;                   // Objects inside this container
    struct obj_data *next_content;               // Next in container list
    struct obj_data *next;                       // Next in global object list
    
    // Character relationships
    struct char_data *carried_by;                // Character carrying object
    struct char_data *worn_by;                   // Character wearing object
    struct char_data *sitting_here;              // Character using furniture
    
    // Scripting and special features
    long id;                                     // DG script unique ID
    struct trig_proto_list *proto_script;        // Default triggers
    struct script_data *script;                  // Active script data
    
    // Advanced features
    int activate_spell[5];                       // Usable spells
    struct obj_data *sheath_primary;             // Weapon sheathing
    struct obj_data *sheath_secondary;           // Off-hand sheathing
    bool drainKilled;                            // Special corpse flag
    char *char_sdesc;                            // Corpse owner description
};
```

**Container System:**
- **Nested Containers:** Objects can contain other objects
- **Linked Lists:** `contains` and `next_content` manage container contents
- **Location Tracking:** Objects know their container or room location

### 4. Descriptor Data (`struct descriptor_data`)

Manages network connections and player sessions:

```c
struct descriptor_data {
    socket_t descriptor;                         // Network socket
    char host[HOST_LENGTH + 1];                  // Client hostname
    byte bad_pws;                                // Failed login attempts
    byte idle_tics;                              // Idle time counter
    int connected;                               // Connection state
    int desc_num;                                // Unique descriptor ID
    time_t login_time;                           // Connection timestamp
    
    // Input/Output buffers
    char *showstr_head;                          // Pager system
    char *showstr_point;                         // Current pager position
    char **str;                                  // String editor pointer
    size_t max_str;                              // String editor limit
    long mail_to;                                // Mail recipient
    
    // Input/Output queues
    struct txt_q input;                          // Input command queue
    char *output;                                // Output buffer
    char *history[HISTORY_SIZE];                 // Command history
    int history_pos;                             // History position
    
    // Character and session data
    struct char_data *character;                 // Associated character
    struct char_data *original;                  // Original character (for switch)
    
    // Administrative features
    struct descriptor_data *snooping;            // Snooping target
    struct descriptor_data *snoop_by;            // Snooped by whom
    struct descriptor_data *next;                // Next in descriptor list
    
    // Extended features
    struct oasis_olc_data *olc;                  // Online creation data
    protocol_t *pProtocol;                       // Protocol handler
    struct list_data *events;                    // Event system
    struct account_data *account;                // Account system
};
```

**Connection States:**
- `CON_PLAYING` - Normal gameplay
- `CON_GET_NAME` - Getting player name
- `CON_PASSWORD` - Password verification
- `CON_NEWPASSWD` - New password creation
- Various menu and editor states

### 5. Zone Data (`struct zone_data`)

Manages game world areas and reset systems:

```c
struct zone_data {
    char *name;                                  // Zone name
    char *builders;                              // Builder credits
    int lifespan;                                // Reset timer
    int age;                                     // Current age
    room_vnum bot;                               // Bottom room vnum
    room_vnum top;                               // Top room vnum
    int reset_mode;                              // Reset behavior
    zone_vnum number;                            // Zone number
    
    // Zone properties
    int zone_flags[ZF_ARRAY_MAX];                // Zone flags
    int min_level;                               // Minimum level
    int max_level;                               // Maximum level
    
    // Reset commands
    struct reset_com *cmd;                       // Reset command list
    
    // Scripting
    struct trig_proto_list *proto_script;        // Zone triggers
};
```

## Memory Management Patterns

### 1. Allocation Strategies

**Static Arrays:**
- World data uses pre-allocated arrays sized at boot time
- `struct room_data *world` - All rooms
- `struct char_data *mob_proto` - NPC prototypes
- `struct obj_data *obj_proto` - Object prototypes

**Dynamic Allocation:**
- Player characters allocated on login
- Temporary objects created as needed
- String data allocated dynamically

**Memory Pools:**
- Buffer pools for common operations
- Reduces malloc/free overhead
- Configurable pool sizes

### 2. Linked List Management

**Character Lists:**
```c
// Global character list
extern struct char_data *character_list;

// Room occupant lists
room->people = character;
character->next_in_room = next_character;

// Combat participant lists  
character->next_fighting = next_combatant;
```

**Object Lists:**
```c
// Global object list
extern struct obj_data *object_list;

// Container contents
container->contains = object;
object->next_content = next_object;

// Room contents
room->contents = object;
object->next = next_object;
```

**Descriptor Lists:**
```c
// Active connections
extern struct descriptor_data *descriptor_list;
descriptor->next = next_descriptor;
```

### 3. Reference Management

**Pointer Relationships:**
- Characters point to their room: `ch->in_room`
- Rooms maintain character lists: `room->people`
- Objects track their location: `obj->in_room` or `obj->carried_by`
- Descriptors link to characters: `desc->character`

**Cleanup Procedures:**
- `extract_char()` - Remove character and update all references
- `extract_obj()` - Remove object and update containers/carriers
- `close_socket()` - Clean up descriptor and associated data

### 4. Memory Debugging

**Debug Mode:**
```c
#ifdef MEMORY_DEBUG
#include "zmalloc.h"
#endif
```

**Features:**
- Memory leak detection
- Allocation tracking
- Corruption detection
- Usage statistics

**Debug Functions:**
- `zmalloc()` - Tracked malloc
- `zfree()` - Tracked free
- `zmalloc_init()` - Initialize tracking
- Memory usage reports

### 5. String Management

**Dynamic Strings:**
- All text data allocated dynamically
- Reference counting for shared strings
- Automatic cleanup on object destruction

**String Pools:**
- Common strings shared between objects
- Reduces memory fragmentation
- Improves cache performance

## Data Structure Relationships

### Character-Room-Object Triangle

```
Character ←→ Room ←→ Object
    ↑                  ↓
    └── Equipment ←────┘
```

**Character in Room:**
- `character->in_room` points to room
- `room->people` lists all characters
- `character->next_in_room` links room occupants

**Object Locations:**
- In room: `object->in_room` set, `room->contents` lists objects
- Carried: `object->carried_by` set, `character->carrying` lists objects
- Worn: `object->worn_by` set, `character->equipment[]` array
- In container: `object->in_obj` set, `container->contains` lists objects

### Network-Character Connection

```
Descriptor ←→ Character ←→ Account
     ↓            ↓
   Socket      Game Data
```

**Connection Flow:**
1. Socket accepts connection → `descriptor_data` created
2. Login process → `char_data` loaded/created
3. `descriptor->character` and `character->desc` linked
4. Account system provides persistent data

### Scripting Integration

```
Trigger ←→ Script ←→ Variables
   ↓         ↓         ↓
Room/Char/Obj → Events → Actions
```

**Script Attachment:**
- All major structures can have scripts attached
- `proto_script` for default triggers
- `script` for active script instances
- Event system for timed actions

## Performance Considerations

### 1. Cache Efficiency

**Data Locality:**
- Related data structures stored together
- Linked lists maintain spatial locality where possible
- Hot data paths optimized for cache performance

### 2. Search Optimization

**Hash Tables:**
- Character lookup by name/ID
- Object lookup by virtual number
- Room lookup by virtual number

**Indexing:**
- Pre-computed indices for world data
- Binary search for sorted arrays
- Efficient iteration patterns

### 3. Memory Usage

**Typical Memory Footprint:**
- Base world data: 50-200MB
- Active players: 1-5MB per player
- Dynamic objects: Variable based on activity
- Script data: 10-50MB depending on complexity

**Optimization Strategies:**
- Lazy loading of non-essential data
- Compression of text data
- Efficient data structure packing
- Memory pool reuse

This data structure system provides the foundation for all game operations while maintaining performance and flexibility for the complex interactions required in a MUD environment.
