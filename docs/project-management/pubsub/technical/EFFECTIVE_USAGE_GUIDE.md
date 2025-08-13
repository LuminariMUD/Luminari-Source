# PubSub System Best Practices & Effective Usage Guide

**System:** LuminariMUD Publish/Subscribe Messaging System  
**Purpose:** Maximize effectiveness and optimize usage patterns  
**Audience:** Game developers, system administrators, content creators  
**Last Updated:** August 13, 2025

---

## 🎯 **Overview**

This guide explains how to use the LuminariMUD PubSub system to maximum effect, covering optimal usage patterns, advanced features, and real-world implementation strategies for creating immersive player experiences.

---

## 🏗️ **System Architecture Quick Reference**

### Core Components
- **Topics:** Named message channels (e.g., `weather_alerts`, `guild_chat`, `auction_house`)
- **Subscriptions:** Player registrations to receive topic messages
- **Messages:** Priority-based content with metadata and spatial data
- **Handlers:** Processing functions that deliver messages to players
- **Queue:** High-performance priority-based message processing system

### Message Flow
```
Publisher → Message → Topic → Queue → Handler → Subscriber
    ↓         ↓        ↓       ↓        ↓         ↓
   NPC     Content   Channel Priority Delivery Player
```

---

## 📊 **Usage Categories & Examples**

### 1. Environmental Immersion
**Purpose:** Create dynamic, living world atmosphere

#### Weather Systems
```c
// Dynamic weather announcements based on location
pubsub_publish_wilderness_audio(
    weather_npc,                              // Source
    "Thunder rumbles ominously overhead",     // Content
    PUBSUB_PRIORITY_NORMAL,                   // Priority
    SPATIAL_AUDIO_MAX_DISTANCE               // 50 room range
);

// Region-specific weather topics
pubsub_topic_create("weather_northern_mountains", 
                   "Weather alerts for northern mountain region");
pubsub_topic_create("weather_coastal_storms", 
                   "Coastal storm warnings and updates");
```

#### Ambient Audio
```c
// Forest sounds with terrain awareness
pubsub_publish_spatial_message(
    "forest_ambience",                        // Topic
    "Birds chirp melodically in the canopy above",
    forest_x, forest_y, forest_z,            // 3D position
    25,                                       // Range
    PUBSUB_PRIORITY_LOW                       // Background priority
);

// Cave echoes with underground modifier
pubsub_publish_spatial_message(
    "cave_ambience",
    "Water drips echo through the cavern",
    cave_x, cave_y, cave_z,
    15,  // Reduced range underground
    PUBSUB_PRIORITY_LOW
);
```

### 2. Social Communication Systems
**Purpose:** Enhanced player interaction and community building

#### Guild Communication
```c
// Guild announcements with role-based access
struct pubsub_topic *guild_topic = pubsub_topic_create(
    "guild_silver_dragons",
    "Silver Dragons guild communication"
);
guild_topic->access_type = PUBSUB_ACCESS_GUILD_ONLY;
guild_topic->min_level = 1;
guild_topic->max_subscribers = 100;

// Guild event notifications
pubsub_publish_message_with_metadata(
    "guild_silver_dragons",
    "Guild raid scheduled: Dragon's Lair - Sunday 8PM EST",
    PUBSUB_PRIORITY_HIGH,
    "event_type:raid,location:dragon_lair,time:sunday_8pm"
);
```

#### Cross-Zone Chat
```c
// OOC (Out of Character) chat system
pubsub_subscribe_player_with_options(ch, "ooc_chat", 
    "send_formatted",                         // Handler
    PUBSUB_PRIORITY_NORMAL,                   // Min priority
    FALSE,                                    // No offline delivery
    0                                         // No spatial limit
);

// Auction house with structured data
pubsub_publish_message_with_metadata(
    "auction_house",
    "Flaming sword up for auction - Starting bid: 5000 gold",
    PUBSUB_PRIORITY_HIGH,
    "item_type:weapon,item_id:12345,starting_bid:5000,duration:24h"
);
```

### 3. Quest & Story Systems
**Purpose:** Dynamic storytelling and quest progression

#### Dynamic Quest Updates
```c
// Quest progression notifications
void quest_update_progress(struct char_data *ch, int quest_id, char *update) {
    char topic_name[256];
    snprintf(topic_name, sizeof(topic_name), "quest_%d_updates", quest_id);
    
    struct pubsub_message msg = {
        .topic = topic_name,
        .content = update,
        .priority = PUBSUB_PRIORITY_HIGH,
        .sender_name = "Quest System",
        .message_type = PUBSUB_MESSAGE_SYSTEM
    };
    
    pubsub_publish_message(&msg);
}

// Example usage:
quest_update_progress(ch, 101, 
    "The ancient tome glows as you approach the mystical altar...");
```

#### Story Events
```c
// Regional story events affecting multiple players
void trigger_regional_event(int region_id, char *event_description) {
    char topic_name[256];
    snprintf(topic_name, sizeof(topic_name), "region_%d_events", region_id);
    
    // High priority for major story events
    pubsub_publish_message_with_priority(
        topic_name,
        event_description,
        PUBSUB_PRIORITY_URGENT,
        "Story System"
    );
}

// Example: Dragon attack event
trigger_regional_event(5, 
    "A massive shadow passes overhead as an ancient red dragon "
    "circles the village, its roar shaking the very foundations!");
```

### 4. Combat & Action Systems
**Purpose:** Real-time tactical information and combat feedback

#### Spatial Combat Audio
```c
// Combat sounds with directional information
void combat_spatial_audio(struct char_data *attacker, struct char_data *victim, 
                         char *combat_sound) {
    if (IS_IN_WILDERNESS(attacker)) {
        pubsub_publish_wilderness_audio(
            attacker,
            combat_sound,
            PUBSUB_PRIORITY_HIGH,
            SPATIAL_AUDIO_MID_THRESHOLD  // 15 room range
        );
    }
}

// Usage in combat system:
combat_spatial_audio(ch, victim, 
    "The clash of steel rings out as weapons meet in fierce combat!");
```

#### Group Coordination
```c
// Party communication system
struct pubsub_topic *party_topic = pubsub_topic_create(
    "party_alpha_team",
    "Alpha team tactical communication"
);
party_topic->access_type = PUBSUB_ACCESS_SUBSCRIBER_ONLY;
party_topic->message_ttl = 300;  // 5 minute message expiry

// Tactical updates
pubsub_publish_message_with_metadata(
    "party_alpha_team",
    "Enemy mage casting fireball - take cover!",
    PUBSUB_PRIORITY_URGENT,
    "message_type:tactical,threat_level:high,action_required:immediate"
);
```

---

## 🎮 **Advanced Features & Techniques**

### 1. Message Filtering & Smart Delivery

#### Priority-Based Filtering
```c
// Subscribe with minimum priority filtering
pubsub_subscription *sub = pubsub_subscribe_player_advanced(
    ch,
    "world_events",
    "send_formatted",
    PUBSUB_PRIORITY_HIGH,     // Only receive HIGH, URGENT, CRITICAL
    TRUE,                     // Enable offline delivery
    0                         // No spatial limit
);

// Critical alerts bypass all filtering
pubsub_publish_message_with_priority(
    "world_events",
    "SERVER RESTART: 5 minutes remaining!",
    PUBSUB_PRIORITY_CRITICAL,  // Always delivered
    "System Administrator"
);
```

#### Conditional Subscriptions
```c
// Level-based topic access
int subscribe_with_level_check(struct char_data *ch, char *topic_name) {
    struct pubsub_topic *topic = pubsub_topic_find(topic_name);
    
    if (!topic) {
        return PUBSUB_ERROR_NOT_FOUND;
    }
    
    if (GET_LEVEL(ch) < topic->min_level) {
        send_to_char(ch, "You need to be level %d to access '%s'.\r\n",
                     topic->min_level, topic_name);
        return PUBSUB_ERROR_PERMISSION;
    }
    
    return pubsub_subscribe_player(ch, topic_name);
}
```

### 2. Spatial Audio Optimization

#### Distance-Based Volume Control
```c
// Varying message intensity by distance
void publish_distance_scaled_audio(struct char_data *source, char *base_message, 
                                  int max_distance) {
    char close_msg[MAX_STRING_LENGTH];
    char far_msg[MAX_STRING_LENGTH];
    
    // Close range (< 5 rooms) - full intensity
    snprintf(close_msg, sizeof(close_msg), 
        "%s The sound is crystal clear and immediate.", base_message);
    
    // Far range (> 25 rooms) - distant echo
    snprintf(far_msg, sizeof(far_msg), 
        "You hear the faint echo of %s", base_message);
    
    // Publish with spatial data for distance-based processing
    pubsub_publish_wilderness_audio(source, base_message, 
                                   PUBSUB_PRIORITY_NORMAL, max_distance);
}
```

#### Terrain-Aware Messaging
```c
// Terrain-specific audio modifications
void publish_terrain_aware_audio(int x, int y, char *message) {
    int terrain_type = get_terrain_type(x, y);
    char modified_message[MAX_STRING_LENGTH];
    
    switch (terrain_type) {
        case SECT_FOREST:
            snprintf(modified_message, sizeof(modified_message),
                "%s The forest canopy muffles the sound.", message);
            break;
            
        case SECT_MOUNTAIN:
            snprintf(modified_message, sizeof(modified_message),
                "%s The mountains create a haunting echo.", message);
            break;
            
        case SECT_WATER:
            snprintf(modified_message, sizeof(modified_message),
                "%s The sound carries clearly across the water.", message);
            break;
            
        default:
            strncpy(modified_message, message, sizeof(modified_message));
            break;
    }
    
    pubsub_publish_spatial_message("terrain_audio", modified_message,
                                  x, y, 0, SPATIAL_AUDIO_MAX_DISTANCE,
                                  PUBSUB_PRIORITY_NORMAL);
}
```

### 3. Performance Optimization Patterns

#### Batch Message Publishing
```c
// Efficient batch publishing for multiple recipients
void publish_to_zone_batch(int zone_vnum, char *message, int priority) {
    char topic_name[256];
    snprintf(topic_name, sizeof(topic_name), "zone_%d_events", zone_vnum);
    
    // Single publish reaches all zone subscribers
    pubsub_publish_message_with_priority(topic_name, message, priority, "Zone System");
    
    // More efficient than individual sends to each player in zone
}

// Usage for zone-wide events
publish_to_zone_batch(30, 
    "The ground trembles as an earthquake shakes the caverns!", 
    PUBSUB_PRIORITY_HIGH);
```

#### Smart Subscription Management
```c
// Temporary subscriptions for events
void create_temporary_subscription(struct char_data *ch, char *topic, int duration) {
    pubsub_subscribe_player(ch, topic);
    
    // Schedule automatic unsubscribe
    struct event_data *unsub_event = create_event(duration);
    unsub_event->func = auto_unsubscribe_player;
    unsub_event->arg1 = strdup(GET_NAME(ch));
    unsub_event->arg2 = strdup(topic);
    
    add_event(unsub_event);
}

// Auto-subscribe players entering specific areas
void on_player_enter_zone(struct char_data *ch, int zone_vnum) {
    char topic_name[256];
    snprintf(topic_name, sizeof(topic_name), "zone_%d_events", zone_vnum);
    
    // Temporary subscription while in zone
    create_temporary_subscription(ch, topic_name, -1);  // Until they leave
}
```

---

## 📈 **Performance Best Practices**

### 1. Message Design Principles

#### Optimal Message Size
```c
// Good: Concise, impactful messages
pubsub_publish_message("weather_alerts", 
    "Storm approaching from the north!", 
    PUBSUB_PRIORITY_HIGH, "Weather System");

// Avoid: Overly long messages that slow processing
// Bad example (don't do this):
// "A tremendously large and quite threatening storm system..."
// (500+ character descriptions slow down queue processing)
```

#### Priority Usage Guidelines
```c
// CRITICAL: Server shutdowns, critical errors only
pubsub_publish_message("system_alerts", 
    "EMERGENCY: Server restart in 30 seconds!", 
    PUBSUB_PRIORITY_CRITICAL, "System");

// URGENT: Important events requiring immediate attention
pubsub_publish_message("combat_alerts", 
    "Player killer spotted in newbie area!", 
    PUBSUB_PRIORITY_URGENT, "Security");

// HIGH: Significant events, quest updates
pubsub_publish_message("quest_updates", 
    "Ancient dragon awakens in mountain lair!", 
    PUBSUB_PRIORITY_HIGH, "Quest System");

// NORMAL: Regular gameplay messages (80% of traffic)
pubsub_publish_message("general_chat", 
    "Player: Anyone know where to find rare herbs?", 
    PUBSUB_PRIORITY_NORMAL, GET_NAME(ch));

// LOW: Ambient atmosphere, background events
pubsub_publish_message("ambience", 
    "A gentle breeze rustles the leaves", 
    PUBSUB_PRIORITY_LOW, "Environment");
```

### 2. Subscription Strategies

#### Smart Topic Organization
```c
// Hierarchical topic structure
/*
 * world_events           - Global server events
 * ├── world_events_major - Server restarts, patches
 * ├── world_events_minor - Maintenance, updates
 * └── world_events_story - Roleplay storylines
 *
 * zone_<id>_events       - Zone-specific events
 * ├── zone_30_weather    - Weather in zone 30
 * ├── zone_30_combat     - Combat alerts in zone 30
 * └── zone_30_ambience   - Atmospheric sounds
 *
 * guild_<name>           - Guild communications
 * ├── guild_<name>_chat  - General guild chat
 * ├── guild_<name>_raids - Raid coordination
 * └── guild_<name>_news  - Guild announcements
 */

// Implementation example:
void setup_zone_topics(int zone_vnum) {
    char base_topic[256], weather_topic[256], combat_topic[256];
    
    snprintf(base_topic, sizeof(base_topic), "zone_%d_events", zone_vnum);
    snprintf(weather_topic, sizeof(weather_topic), "zone_%d_weather", zone_vnum);
    snprintf(combat_topic, sizeof(combat_topic), "zone_%d_combat", zone_vnum);
    
    pubsub_topic_create(base_topic, "General zone events");
    pubsub_topic_create(weather_topic, "Zone weather updates");
    pubsub_topic_create(combat_topic, "Zone combat alerts");
}
```

#### Conditional Auto-Subscriptions
```c
// Auto-subscribe based on player attributes
void auto_subscribe_by_class(struct char_data *ch) {
    switch (GET_CLASS(ch)) {
        case CLASS_WIZARD:
            pubsub_subscribe_player(ch, "mage_guild_research");
            pubsub_subscribe_player(ch, "arcane_discoveries");
            break;
            
        case CLASS_RANGER:
            pubsub_subscribe_player(ch, "wilderness_alerts");
            pubsub_subscribe_player(ch, "nature_events");
            break;
            
        case CLASS_PALADIN:
            pubsub_subscribe_player(ch, "holy_missions");
            pubsub_subscribe_player(ch, "evil_presence_alerts");
            break;
    }
}

// Auto-subscribe based on location
void auto_subscribe_by_location(struct char_data *ch) {
    if (IS_IN_WILDERNESS(ch)) {
        pubsub_subscribe_player(ch, "wilderness_weather");
        pubsub_subscribe_player(ch, "wilderness_events");
    }
    
    if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_GUILD)) {
        char guild_topic[256];
        snprintf(guild_topic, sizeof(guild_topic), 
                "guild_%s_announcements", GET_GUILD_NAME(ch));
        pubsub_subscribe_player(ch, guild_topic);
    }
}
```

---

## 🔧 **Integration Patterns**

### 1. Weather System Integration
```c
// Complete weather system using PubSub
void weather_system_update(void) {
    int zone;
    
    for (zone = 0; zone <= top_of_zone_table; zone++) {
        struct weather_data *weather = get_zone_weather(zone);
        char topic[256], message[512];
        
        snprintf(topic, sizeof(topic), "zone_%d_weather", 
                zone_table[zone].number);
        
        switch (weather->change) {
            case WEATHER_CHANGE_STORM:
                snprintf(message, sizeof(message),
                    "Dark clouds gather as a storm approaches!");
                pubsub_publish_message(topic, message, 
                    PUBSUB_PRIORITY_HIGH, "Weather System");
                break;
                
            case WEATHER_CHANGE_CLEAR:
                snprintf(message, sizeof(message),
                    "The clouds part, revealing clear skies!");
                pubsub_publish_message(topic, message, 
                    PUBSUB_PRIORITY_NORMAL, "Weather System");
                break;
                
            case WEATHER_CHANGE_RAIN:
                // Use spatial audio for rain sounds
                publish_zone_spatial_audio(zone, 
                    "Raindrops patter softly against the ground",
                    PUBSUB_PRIORITY_LOW);
                break;
        }
    }
}
```

### 2. Combat System Integration
```c
// Enhanced combat with PubSub feedback
void combat_round_pubsub(struct char_data *ch, struct char_data *victim) {
    char combat_topic[256], message[512];
    
    // Zone combat alerts for significant fights
    if (GET_LEVEL(ch) > 30 || GET_LEVEL(victim) > 30) {
        snprintf(combat_topic, sizeof(combat_topic), 
                "zone_%d_combat", GET_ROOM_ZONE(IN_ROOM(ch)));
        snprintf(message, sizeof(message),
                "A fierce battle erupts between %s and %s!",
                GET_NAME(ch), GET_NAME(victim));
        
        pubsub_publish_message(combat_topic, message, 
                              PUBSUB_PRIORITY_HIGH, "Combat System");
    }
    
    // Spatial audio for nearby players
    if (IS_IN_WILDERNESS(ch)) {
        combat_spatial_audio(ch, victim, 
            "The sound of clashing weapons echoes across the landscape!");
    }
}
```

### 3. Economic System Integration
```c
// Auction house with PubSub notifications
void auction_system_integration(void) {
    // New auction notifications
    void announce_new_auction(struct obj_data *obj, int starting_bid) {
        char message[512], metadata[256];
        
        snprintf(message, sizeof(message),
                "%s up for auction! Starting bid: %d gold",
                obj->short_description, starting_bid);
        
        snprintf(metadata, sizeof(metadata),
                "item_id:%d,starting_bid:%d,item_type:%d",
                GET_OBJ_VNUM(obj), starting_bid, GET_OBJ_TYPE(obj));
        
        pubsub_publish_message_with_metadata("auction_house", message,
                                            PUBSUB_PRIORITY_HIGH, 
                                            metadata);
    }
    
    // Bid update notifications
    void announce_bid_update(struct obj_data *obj, int new_bid, char *bidder) {
        char message[512];
        
        snprintf(message, sizeof(message),
                "New bid on %s: %d gold by %s",
                obj->short_description, new_bid, bidder);
        
        pubsub_publish_message("auction_house", message,
                              PUBSUB_PRIORITY_NORMAL, "Auction System");
    }
}
```

---

## 🎯 **Real-World Implementation Examples**

### Example 1: Dynamic Dungeon Events
```c
// Multi-stage dungeon event system
void dungeon_event_system(int dungeon_id) {
    char topic[256];
    snprintf(topic, sizeof(topic), "dungeon_%d_events", dungeon_id);
    
    // Stage 1: Entry warning
    pubsub_publish_message(topic,
        "The ancient seals weaken as you enter the forgotten tomb...",
        PUBSUB_PRIORITY_HIGH, "Dungeon Guardian");
    
    // Stage 2: Environmental changes
    pubsub_publish_spatial_message(topic,
        "Mysterious whispers echo through the corridors",
        dungeon_x, dungeon_y, dungeon_z, 30,
        PUBSUB_PRIORITY_NORMAL);
    
    // Stage 3: Boss awakening
    pubsub_publish_message(topic,
        "A deep, rumbling roar shakes the very foundations!",
        PUBSUB_PRIORITY_URGENT, "Ancient Lich");
}
```

### Example 2: Player-Driven News System
```c
// Player news and rumor system
void player_news_system(struct char_data *reporter, char *news_text) {
    char metadata[512];
    
    // Add reporter credibility and location data
    snprintf(metadata, sizeof(metadata),
            "reporter:%s,credibility:%d,location:%d,timestamp:%ld",
            GET_NAME(reporter), 
            calculate_player_credibility(reporter),
            GET_ROOM_VNUM(IN_ROOM(reporter)),
            time(NULL));
    
    // Publish to news network
    pubsub_publish_message_with_metadata("player_news", news_text,
                                        PUBSUB_PRIORITY_NORMAL, metadata);
    
    // Regional news gets local distribution too
    char local_topic[256];
    snprintf(local_topic, sizeof(local_topic), 
            "zone_%d_news", GET_ROOM_ZONE(IN_ROOM(reporter)));
    
    pubsub_publish_message(local_topic, news_text,
                          PUBSUB_PRIORITY_HIGH, GET_NAME(reporter));
}
```

### Example 3: Event-Driven Crafting System
```c
// Crafting system with environmental feedback
void crafting_with_pubsub(struct char_data *ch, struct obj_data *item) {
    char message[512], topic[256];
    
    // Different topics based on crafting location
    if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_FORGE)) {
        strcpy(topic, "forge_activities");
        snprintf(message, sizeof(message),
                "The forge roars as %s works to create %s",
                GET_NAME(ch), item->short_description);
        
        // Spatial audio for forge work
        pubsub_publish_wilderness_audio(ch, 
            "The rhythmic hammering of metal on anvil rings out",
            PUBSUB_PRIORITY_NORMAL, SPATIAL_AUDIO_MID_THRESHOLD);
    } 
    else if (room_has_flag(IN_ROOM(ch), ROOM_LABORATORY)) {
        strcpy(topic, "alchemy_activities");
        snprintf(message, sizeof(message),
                "Mysterious vapors rise as %s brews %s",
                GET_NAME(ch), item->short_description);
        
        pubsub_publish_spatial_message(topic, message,
            GET_WILDERNESS_X(ch), GET_WILDERNESS_Y(ch), 0,
            SPATIAL_AUDIO_CLOSE_THRESHOLD, PUBSUB_PRIORITY_NORMAL);
    }
    
    // Success/failure notifications
    pubsub_publish_message(topic, message, PUBSUB_PRIORITY_NORMAL, GET_NAME(ch));
}
```

---

## 📝 **Content Creation Guidelines**

### Message Writing Best Practices

#### Effective Atmospheric Messages
```c
// Good: Evocative, concise, immersive
"Lightning illuminates the storm-darkened sky for a brief moment"
"The wind carries the scent of rain and distant wildflowers" 
"Ancient stones whisper forgotten secrets to those who listen"

// Avoid: Too verbose or breaking immersion
"You notice that there appears to be some weather happening..."
"*SYSTEM MESSAGE* Weather change detected in sector 7-G"
```

#### Spatial Audio Guidelines
```c
// Close range (0-5 rooms): High detail, immediate
"Steel clashes against steel as blades meet in deadly combat"

// Medium range (6-15 rooms): Moderate detail, contextual  
"The sound of battle echoes from somewhere nearby"

// Far range (16+ rooms): Subtle, atmospheric
"A distant clash of weapons carries on the wind"
```

### Topic Naming Conventions
- **System Topics:** `system_*` (e.g., `system_announcements`)
- **Zone Topics:** `zone_<id>_*` (e.g., `zone_30_weather`)  
- **Guild Topics:** `guild_<name>_*` (e.g., `guild_dragons_chat`)
- **Player Topics:** `player_<name>_*` (e.g., `player_gandalf_tells`)
- **Event Topics:** `event_<name>_*` (e.g., `event_dragon_invasion`)

---

## 🚀 **Advanced Customization**

### Creating Custom Handlers
```c
// Example: Custom spell effect handler
int pubsub_handler_spell_effects(struct char_data *ch, struct pubsub_message *msg) {
    // Parse spell effect metadata
    if (strstr(msg->metadata, "spell_type:fireball")) {
        send_to_char(ch, "&R%s&n\r\n", msg->content);  // Red text for fire
        
        // Add visual effect
        act("$n is surrounded by flickering flames!", FALSE, ch, 0, 0, TO_ROOM);
        
    } else if (strstr(msg->metadata, "spell_type:heal")) {
        send_to_char(ch, "&G%s&n\r\n", msg->content);  // Green text for healing
        
        // Add healing glow effect
        act("$n glows with a soft, healing light!", FALSE, ch, 0, 0, TO_ROOM);
    }
    
    return PUBSUB_SUCCESS;
}

// Register the custom handler
pubsub_register_handler("spell_effects", 
                       "Process magical spell effect messages", 
                       pubsub_handler_spell_effects);
```

### Dynamic Topic Creation
```c
// Event-based topic creation and cleanup
void create_event_topic(char *event_name, int duration) {
    char topic_name[256];
    snprintf(topic_name, sizeof(topic_name), "event_%s", event_name);
    
    struct pubsub_topic *topic = pubsub_topic_create(topic_name, 
        "Temporary event topic");
    topic->message_ttl = duration;
    topic->is_persistent = FALSE;  // Auto-cleanup after event
    
    // Schedule topic cleanup
    struct event_data *cleanup = create_event(duration);
    cleanup->func = cleanup_event_topic;
    cleanup->arg1 = strdup(topic_name);
    add_event(cleanup);
}
```

---

*Comprehensive guide for maximizing LuminariMUD PubSub system effectiveness*  
*Creating immersive, dynamic, and engaging player experiences through intelligent messaging*
