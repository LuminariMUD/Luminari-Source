# LuminariMUD World Simulation System

## Overview

LuminariMUD implements a sophisticated world simulation system that manages the game environment, including rooms, zones, wilderness areas, mobile NPCs, and dynamic content. The system handles world loading, zone resets, mobile activity, and environmental changes to create a living, breathing game world.

## Core Components

### 1. World Loading System (`boot_world()`)

The world loading process follows a specific sequence to ensure proper initialization:

```c
void boot_world(void) {
    // 1. Database connection
    connect_to_mysql();
    
    // 2. Core world structures
    log("Loading zone table.");
    index_boot(DB_BOOT_ZON);        // Zone definitions
    
    log("Loading triggers and generating index.");
    index_boot(DB_BOOT_TRG);        // DG Script triggers
    
    log("Loading rooms.");
    index_boot(DB_BOOT_WLD);        // Room data
    
    // 3. Geographic systems
    log("Loading regions. (MySQL)");
    load_regions();                  // Geographic regions
    
    log("Loading paths. (MySQL)");
    load_paths();                    // Travel paths
    
    log("Renumbering rooms.");
    renum_world();                   // Assign real numbers
    
    // 4. Validation and setup
    log("Checking start rooms.");
    check_start_rooms();             // Validate starting locations
    
    // 5. Entities
    log("Loading mobs and generating index.");
    index_boot(DB_BOOT_MOB);        // NPCs and monsters
    
    log("Loading objects and generating index.");
    index_boot(DB_BOOT_OBJ);        // Items and objects
    
    // 6. Game systems
    load_class_list();               // Character classes
    assign_feats();                  // Feat system
    load_deities();                  // Religion system
    
    // 7. Finalization
    renum_zone_table();              // Final zone processing
    boot_social_messages();          // Social commands
    load_help();                     // Help system
}
```

### 2. Zone Management System

Zones are the primary organizational unit for world content:

```c
struct zone_data {
    char *name;                      // Zone name
    char *builders;                  // Builder credits
    int lifespan;                    // Reset timer (minutes)
    int age;                         // Current age
    room_vnum bot;                   // Bottom room vnum
    room_vnum top;                   // Top room vnum
    int reset_mode;                  // Reset behavior
    zone_vnum number;                // Zone number
    int zone_flags[ZF_ARRAY_MAX];    // Zone properties
    int min_level;                   // Minimum level
    int max_level;                   // Maximum level
    struct reset_com *cmd;           // Reset commands
    struct trig_proto_list *proto_script; // Zone triggers
};
```

**Zone Reset Modes:**
- `0` - Never reset
- `1` - Reset when empty of players
- `2` - Always reset when timer expires

**Zone Flags:**
- `ZONE_CLOSED` - Players cannot enter
- `ZONE_NOIMMORT` - Immortals below staff level cannot enter
- `ZONE_GRID` - Connected to main world, shows in area list
- `ZONE_WILDERNESS` - Uses wilderness coordinate system
- `ZONE_NOASTRAL` - No teleportation magic allowed

### 3. Zone Reset System (`zone_update()`)

The zone reset system manages automatic world refreshing:

```c
void zone_update(void) {
    static int timer = 0;
    
    // Every minute (60 seconds)
    if (((++timer * PULSE_ZONE) / PASSES_PER_SEC) >= 60) {
        timer = 0;
        
        // Age all zones
        for (i = 0; i <= top_of_zone_table; i++) {
            if (zone_table[i].age < zone_table[i].lifespan &&
                zone_table[i].reset_mode) {
                (zone_table[i].age)++;
            }
            
            // Queue zones for reset when they reach lifespan
            if (zone_table[i].age >= zone_table[i].lifespan &&
                zone_table[i].age < ZO_DEAD && 
                zone_table[i].reset_mode) {
                
                // Add to reset queue
                CREATE(update_u, struct reset_q_element, 1);
                update_u->zone_to_reset = i;
                // ... queue management
                zone_table[i].age = ZO_DEAD;
            }
        }
    }
    
    // Process reset queue
    for (update_u = reset_q.head; update_u; update_u = update_u->next) {
        if (zone_table[update_u->zone_to_reset].reset_mode == 2 ||
            is_empty(update_u->zone_to_reset)) {
            
            reset_zone(update_u->zone_to_reset);
            mudlog(CMP, LVL_IMPL, FALSE, 
                   "Auto zone reset: %s (Zone %d)",
                   zone_table[update_u->zone_to_reset].name,
                   zone_table[update_u->zone_to_reset].number);
            
            // Remove from queue
            // ... dequeue logic
        }
    }
}
```

### 4. Zone Reset Commands

Zone resets are controlled by commands that specify what to load and where:

**Reset Command Types:**
- `'M'` - Load mobile (NPC)
- `'O'` - Load object in room
- `'P'` - Put object in container
- `'G'` - Give object to mobile
- `'E'` - Equip object on mobile
- `'D'` - Set door state
- `'R'` - Remove object from room
- `'T'` - Attach trigger
- `'V'` - Set variable

**Example Reset Commands:**
```
M 0 3001 2 3001    ; Load mobile 3001, max 2, in room 3001
G 1 3010 -1        ; Give object 3010 to last loaded mobile
E 1 3011 -1 16     ; Equip object 3011 on last mobile, wear position 16
O 0 3020 1 3002    ; Load object 3020, max 1, in room 3002
D 0 3001 0 1       ; Set door in room 3001, direction north, state closed
```

### 5. Room System

Rooms are the fundamental spatial units of the game world:

```c
struct room_data {
    room_vnum number;                    // Virtual room number
    zone_rnum zone;                      // Zone assignment
    int coords[2];                       // Wilderness coordinates
    int sector_type;                     // Terrain type
    int room_flags[RF_ARRAY_MAX];        // Room properties
    long room_affections;                // Active spell effects
    
    // Descriptions
    char *name;                          // Room title
    char *description;                   // Room description
    struct extra_descr_data *ex_description; // Additional descriptions
    
    // Connections and contents
    struct room_direction_data *dir_option[NUM_OF_DIRS]; // Exits
    struct obj_data *contents;           // Objects in room
    struct char_data *people;            // Characters in room
    
    // Environmental factors
    byte light;                          // Light sources
    byte globe;                          // Darkness sources
    
    // Special features
    SPECIAL_DECL(*func);                 // Special procedure
    struct trig_proto_list *proto_script; // Default triggers
    struct script_data *script;          // Active scripts
    struct list_data *events;            // Room events
    
    // Advanced systems
    struct trail_data_list *trail_tracks; // Tracking system
    struct moving_room_data *mover;      // Moving room data
    int harvest_material;                // Harvestable resources
    int harvest_material_amount;         // Resource quantity
};
```

**Sector Types:**
- `SECT_INSIDE` - Indoor rooms
- `SECT_CITY` - Urban areas
- `SECT_FIELD` - Open fields
- `SECT_FOREST` - Wooded areas
- `SECT_HILLS` - Hilly terrain
- `SECT_MOUNTAIN` - Mountainous regions
- `SECT_WATER_SWIM` - Swimmable water
- `SECT_WATER_NOSWIM` - Deep water
- `SECT_UNDERWATER` - Submerged areas
- `SECT_FLYING` - Aerial regions

### 6. Wilderness System

The wilderness system provides procedurally generated outdoor areas:

```c
// Wilderness room management
room_rnum find_room_by_coordinates(int x, int y) {
    // Check static rooms first
    room_rnum room = find_static_room_by_coordinates(x, y);
    if (room != NOWHERE) return room;
    
    // Check dynamic rooms
    for (i = WILD_DYNAMIC_ROOM_VNUM_START; 
         i <= WILD_DYNAMIC_ROOM_VNUM_END && real_room(i) != NOWHERE; 
         i++) {
        if (ROOM_FLAGGED(real_room(i), ROOM_OCCUPIED) &&
            world[real_room(i)].coords[X_COORD] == x &&
            world[real_room(i)].coords[Y_COORD] == y) {
            return real_room(i);
        }
    }
    
    return NOWHERE;
}

void assign_wilderness_room(room_rnum room, int x, int y) {
    // Set coordinates
    world[room].coords[X_COORD] = x;
    world[room].coords[Y_COORD] = y;
    
    // Generate terrain based on coordinates
    world[room].sector_type = generate_wilderness_sector(x, y);
    
    // Set room properties
    SET_BIT_AR(ROOM_FLAGS(room), ROOM_OCCUPIED);
    
    // Generate description
    generate_wilderness_description(room, x, y);
}
```

**Wilderness Features:**
- **Coordinate System:** X,Y coordinates for positioning
- **Dynamic Rooms:** Rooms created on-demand for exploration
- **Terrain Generation:** Procedural sector type assignment
- **KD-Tree Indexing:** Efficient spatial lookups
- **Static Rooms:** Pre-built rooms at specific coordinates

### 7. Mobile Activity System (`mobile_activity()`)

NPCs are brought to life through the mobile activity system:

```c
void mobile_activity(void) {
    struct char_data *ch, *next_ch, *vict;
    
    for (ch = character_list; ch; ch = next_ch) {
        next_ch = ch->next;
        
        // Skip player characters
        if (!IS_NPC(ch)) continue;
        
        // Skip dead or extracted mobiles
        if (MOB_FLAGGED(ch, MOB_NOTDEADYET)) continue;
        
        // Combat actions
        if (FIGHTING(ch)) {
            mobile_combat_activity(ch);
            continue;
        }
        
        // Helper behavior - assist allies
        if (MOB_FLAGGED(ch, MOB_HELPER)) {
            for (vict = world[IN_ROOM(ch)].people; vict; vict = vict->next_in_room) {
                if (ch != vict && IS_NPC(vict) && FIGHTING(vict) &&
                    !IS_NPC(FIGHTING(vict)) && ch->master != FIGHTING(vict)) {
                    act("$n jumps to the aid of $N!", FALSE, ch, 0, vict, TO_ROOM);
                    hit(ch, FIGHTING(vict), TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, FALSE);
                    break;
                }
            }
        }
        
        // Movement behaviors
        
        // Follow set path (patrols)
        if (move_on_path(ch)) continue;
        
        // Hunt specific targets
        if (MOB_FLAGGED(ch, MOB_HUNTER)) {
            hunt_victim(ch);
        }
        
        // Scavenge items
        if (MOB_FLAGGED(ch, MOB_SCAVENGER)) {
            scavenge_items(ch);
        }
        
        // Random movement
        if (!rand_number(0, 2) && !MOB_FLAGGED(ch, MOB_SENTINEL) &&
            GET_POS(ch) == POS_STANDING) {
            
            int door = rand_number(0, 18);
            if (door < DIR_COUNT && CAN_GO(ch, door) &&
                !ROOM_FLAGGED(EXIT(ch, door)->to_room, ROOM_NOMOB) &&
                !ROOM_FLAGGED(EXIT(ch, door)->to_room, ROOM_DEATH) &&
                (!MOB_FLAGGED(ch, MOB_STAY_ZONE) ||
                 world[EXIT(ch, door)->to_room].zone == world[IN_ROOM(ch)].zone)) {
                
                if (ch->master == NULL) {
                    perform_move(ch, door, 1);
                }
            }
        }
        
        // Special procedures
        if (MOB_FLAGGED(ch, MOB_SPEC) && GET_MOB_SPEC(ch)) {
            if (GET_MOB_SPEC(ch)(ch, ch, 0, "")) continue;
        }
    }
}
```

**Mobile Behaviors:**
- **Combat AI:** Intelligent fighting and spell casting
- **Helper Mobs:** Assist other NPCs in combat
- **Hunters:** Track and pursue specific targets
- **Scavengers:** Collect items from the ground
- **Sentinels:** Stay in assigned locations
- **Wanderers:** Move randomly through areas
- **Zone Restricted:** Stay within assigned zones

### 8. Moving Rooms System

Special rooms that change location over time:

```c
struct moving_room_data {
    int resetZonePulse;          // Zone pulses per reset
    int remainingZonePulses;     // Pulses left until reset
    int currentInbound;          // Current connection room
    room_num destination;        // Target room
    int inbound_dir;             // Direction to/from target
    int randomMove;              // Random movement flag
    sh_int exitInfo;             // Door type
    obj_num keyInfo;             // Key required
    char *keywords;              // Door keywords
};

void moving_rooms_update(void) {
    struct moving_room_data *nextRoom = movingRoomList;
    
    while (nextRoom != NULL) {
        nextRoom->remainingZonePulses--;
        
        if (nextRoom->remainingZonePulses <= 0) {
            room_num mover = nextRoom->destination;
            
            if (real_room(mover) > 0) {
                // Execute room's special procedure for movement
                if (world[real_room(mover)].func != NULL) {
                    world[real_room(mover)].func(NULL, nextRoom, 0, NULL);
                }
            }
            
            // Reset timer
            nextRoom->remainingZonePulses = nextRoom->resetZonePulse;
        }
        
        nextRoom = nextRoom->next;
    }
}
```

### 9. Dynamic Content Generation

The system supports various forms of dynamic content:

**Room Generation:**
- Wilderness rooms created on-demand
- Procedural terrain and descriptions
- Dynamic exit creation for exploration

**Object Spawning:**
- Random treasure generation
- Harvestable resource respawning
- Dynamic item creation based on conditions

**NPC Spawning:**
- Random encounters in wilderness
- Dynamic population based on time/conditions
- Seasonal or event-based spawning

### 10. Environmental Systems

**Weather System:**
- Dynamic weather patterns
- Seasonal changes
- Weather effects on gameplay

**Time System:**
- Day/night cycles
- Seasonal progression
- Time-based events and triggers

**Lighting System:**
- Natural light from sun/moon
- Artificial light sources
- Darkness and vision effects

## Performance Optimizations

### 1. Spatial Indexing

**KD-Tree for Wilderness:**
- Efficient spatial queries for room lookup
- Fast nearest-neighbor searches
- Optimized for 2D coordinate systems

### 2. Zone Management

**Reset Queuing:**
- Zones queued for reset rather than immediate processing
- Load balancing across multiple game ticks
- Priority system for critical zones

### 3. Mobile Optimization

**Activity Throttling:**
- Not all mobiles act every tick
- Randomized activity to spread CPU load
- Inactive mobile detection and optimization

### 4. Memory Management

**Dynamic Room Allocation:**
- Wilderness rooms allocated on-demand
- Unused rooms deallocated after timeout
- Memory pool management for frequent allocations

## Integration Points

The world simulation system integrates with:

- **Combat System:** Provides battlegrounds and tactical environments
- **Quest System:** Manages quest locations and objectives
- **Scripting System:** Executes room, mobile, and object triggers
- **Player System:** Handles player movement and interaction
- **Event System:** Manages timed world events and changes

This comprehensive world simulation creates a dynamic, living environment that responds to player actions while maintaining consistent game world rules and behaviors.
