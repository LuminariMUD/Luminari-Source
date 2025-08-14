# LuminariMUD Publish/Subscribe System - Design and Implementation Plan

## Document Overview

**Author:** AI Assistant  
**Date:** August 12, 2025  
**Status:** Draft - Design Phase  
**Version:** 1.0  

## Table of Contents

1. [Executive Summary](#executive-summary)
2. [System Architecture](#system-architecture)
3. [Core Components](#core-components)
4. [Data Structures](#data-structures)
5. [API Design](#api-design)
6. [Integration Points](#integration-points)
7. [Use Cases and Examples](#use-cases-and-examples)
8. [Implementation Phases](#implementation-phases)
9. [Performance Considerations](#performance-considerations)
10. [Security and Safety](#security-and-safety)
11. [Testing Strategy](#testing-strategy)
12. [Future Extensions](#future-extensions)
13. [Code Architecture Guidelines](#code-architecture-guidelines)
14. [File Structure and Organization](#file-structure-and-organization)
15. [Memory Management Strategy](#memory-management-strategy)
16. [Error Handling Patterns](#error-handling-patterns)
17. [Integration Requirements](#integration-requirements)
18. [Configuration and Tuning](#configuration-and-tuning)
19. [Debugging and Monitoring](#debugging-and-monitoring)
20. [Migration and Deployment](#migration-and-deployment)

---

## Executive Summary

The LuminariMUD Publish/Subscribe (PubSub) system is a general-purpose, event-driven messaging framework designed to decouple game systems and enable flexible, scalable communication between game components. This system will allow for topics to be created, players and systems to subscribe to those topics, and messages to be published that trigger actions for subscribers.

### Key Goals
- **Decoupling**: Separate message producers from consumers
- **Flexibility**: Support multiple message types and handler actions
- **Scalability**: Handle hundreds of concurrent subscriptions efficiently
- **Integration**: Work seamlessly with existing LuminariMUD systems
- **Extensibility**: Easy to add new topics, message types, and handlers

### Core Benefits
- **Event-Driven Gameplay**: Weather changes, realm announcements, faction updates
- **Notification System**: Tell system replacement, guild communications, alerts
- **System Integration**: Connect different mud subsystems without tight coupling
- **Player Experience**: Rich, dynamic content delivery based on preferences
- **Administrative Tools**: Broadcast capabilities, monitoring, debugging

---

## System Architecture

### High-Level Design

```
┌─────────────┐    ┌─────────────┐    ┌─────────────┐
│  Publishers │───▶│   PubSub    │───▶│ Subscribers │
│             │    │   Engine    │    │             │
└─────────────┘    └─────────────┘    └─────────────┘
                           │
                           ▼
                   ┌─────────────┐
                   │   Topics    │
                   │ Management  │
                   └─────────────┘
```

### Core Principles

1. **Topic-Based Messaging**: All communication flows through named topics
2. **Asynchronous Processing**: Messages are queued and processed during game pulses
3. **Type Safety**: Strongly typed messages with validation
4. **Priority System**: Critical messages get priority handling
5. **Persistence**: Topic subscriptions survive logout/login cycles
6. **Filtering**: Subscribers can filter messages based on criteria

### Spatial Event Processing

The PubSub system supports sophisticated spatial events that affect players differently based on their location, distance from the event, and environmental factors between them and the event source.

**Key Concepts:**

1. **Distance-Based Variants**: Different messages based on how far the player is from the event
2. **Terrain Propagation**: Sound/effects are modified by terrain between source and listener
3. **Environmental Factors**: Weather, elevation, and obstacles affect transmission
4. **Efficient Processing**: Only process events if players are within range ("tree falling in forest" principle)
5. **Realistic Effects**: Close events have physical effects, distant ones are just audio

**Implementation Details:**

```c
/* Example: Tree falling event at coordinates (0,0) */
void wilderness_tree_fall_event(int x, int y, int tree_size) {
    /* Calculate max hearing distance based on tree size */
    int max_distance = 3 + (tree_size / 10); // Larger trees heard farther
    int base_volume = 50 + tree_size;        // Larger trees are louder
    
    /* Only publish if players are nearby */
    if (!pubsub_has_listeners_in_radius(x, y, max_distance)) {
        return; /* No one around to hear it */
    }
    
    /* Set up distance-based messages */
    const char *distance_messages[MAX_DISTANCE_VARIANTS] = {
        "A massive tree crashes down right next to you with earth-shaking force!",
        "A large tree falls nearby with a thunderous crash and splintering wood.",
        "You hear the loud crack and crash of a falling tree in the distance.",
        "A distant rumbling crash echoes through the forest.",
        "You barely make out the faint sound of something falling far away."
    };
    
    /* Publish spatial event */
    pubsub_publish_spatial(
        pubsub_find_topic("world.wilderness.audio")->topic_id,
        x, y, distance_messages, max_distance, base_volume,
        SPATIAL_FLAG_TERRAIN_AFFECTS | SPATIAL_FLAG_WEATHER_AFFECTS,
        NULL
    );
}
```

**Terrain Effect Calculations:**

The system models how different terrain types affect sound propagation:

- **Forest**: Muffles sound (+1 distance per forest tile)
- **Mountains**: Blocks sound significantly (+2 distance per mountain tile)  
- **Hills**: Partially blocks sound (+1 distance per hill tile)
- **Water**: Carries sound better (-1 distance per water tile)
- **Plains**: No modification (open terrain)

**Distance Processing Algorithm:**

1. Calculate actual geometric distance between event and player
2. Trace path between event and player coordinates
3. Sum terrain effects along the path to get "effective distance"
4. Select appropriate message variant based on effective distance
5. Calculate volume based on distance falloff
6. Apply visual color coding based on distance/intensity

This approach allows for very realistic and immersive environmental events that feel natural and location-appropriate.

- **Event System**: Leverage the existing MUD event framework for timing
- **Communication**: Use existing `send_to_char()`, `act()` functions for delivery
- **Player Data**: Store subscriptions in player special data
- **Command System**: Add commands for topic management
- **Persistence**: Save/load subscriptions with player files

### 6. Spatial Event Processing Engine

**Purpose**: Handle location-based events with distance and terrain calculations

**Key Features**:
- Distance-based message variants and volume calculation
- Terrain-aware sound propagation modeling
- Elevation and weather effects on audio transmission
- Efficient spatial queries to find nearby players
- "Tree falls in forest" optimization - only process if listeners exist
- Line-of-sight calculations for visual events

---

## Core Components

### 1. Topic Management System

**Purpose**: Manage topic lifecycle, metadata, and access control

**Key Features**:
- Topic creation and deletion
- Access control (public, private, permission-based)
- Topic metadata (description, category, TTL)
- Hierarchical topic naming (e.g., `world.weather.storms`)
- Topic statistics and monitoring

### 2. Subscription Manager

**Purpose**: Handle player and system subscriptions to topics

**Key Features**:
- Subscribe/unsubscribe operations
- Subscription filtering and preferences
- Batch subscription management
- Temporary vs. persistent subscriptions
- Priority-based subscription handling

### 3. Message Queue Engine

**Purpose**: Queue, route, and deliver messages efficiently

**Key Features**:
- Message queuing with priority levels
- Batched message processing
- Message persistence for offline players
- Delivery confirmation and retry logic
- Dead letter handling for failed deliveries

### 4. Handler Dispatch System

**Purpose**: Execute appropriate actions when messages are received

**Key Features**:
- Pluggable handler system
- Handler registration and discovery
- Context-aware message processing
- Error handling and recovery
- Performance monitoring

### 5. Admin Interface

**Purpose**: Administrative tools for managing the PubSub system

**Key Features**:
- Topic management commands
- Subscription monitoring
- Message broadcasting tools
- System health monitoring
- Configuration management

---

## Data Structures

### Topic Structure

```c
/* Topic priority levels */
#define TOPIC_PRIORITY_LOW       1
#define TOPIC_PRIORITY_NORMAL    2  
#define TOPIC_PRIORITY_HIGH      3
#define TOPIC_PRIORITY_CRITICAL  4

/* Topic access types */
#define TOPIC_ACCESS_PUBLIC      0
#define TOPIC_ACCESS_PRIVATE     1
#define TOPIC_ACCESS_RESTRICTED  2

/* Topic categories */
#define TOPIC_CAT_SYSTEM         0
#define TOPIC_CAT_WORLD          1
#define TOPIC_CAT_PLAYER         2
#define TOPIC_CAT_GUILD          3
#define TOPIC_CAT_ZONE           4
#define TOPIC_CAT_ADMIN          5

struct pubsub_topic {
    int topic_id;                          /* Unique topic identifier */
    char *name;                            /* Topic name (e.g., "world.weather") */
    char *description;                     /* Human readable description */
    int category;                          /* Topic category */
    int access_type;                       /* Access control type */
    int min_level;                         /* Minimum level to subscribe */
    int creator_id;                        /* Character ID who created topic */
    time_t created;                        /* Creation timestamp */
    time_t last_message;                   /* Last message timestamp */
    int total_messages;                    /* Total messages sent */
    int subscriber_count;                  /* Current subscriber count */
    bool persistent;                       /* Survive reboots? */
    int max_subscribers;                   /* Subscriber limit (-1 = unlimited) */
    int message_ttl;                       /* Message time-to-live in seconds */
    
    struct list_data *subscribers;         /* List of subscribers */
    struct list_data *moderators;          /* List of topic moderators */
    
    struct pubsub_topic *next;             /* For linked list */
};
```

### Subscription Structure

```c
/* Subscription types */
#define SUB_TYPE_PLAYER          0
#define SUB_TYPE_SYSTEM          1
#define SUB_TYPE_TEMP            2

/* Subscription status */
#define SUB_STATUS_ACTIVE        0
#define SUB_STATUS_PAUSED        1
#define SUB_STATUS_DISABLED      2

struct pubsub_subscription {
    int subscription_id;                   /* Unique subscription ID */
    int topic_id;                          /* Topic being subscribed to */
    int subscriber_type;                   /* Player, system, etc. */
    long subscriber_id;                    /* Character ID or system ID */
    int status;                            /* Active, paused, disabled */
    int priority;                          /* Delivery priority */
    time_t subscribed;                     /* Subscription timestamp */
    time_t last_delivered;                 /* Last successful delivery */
    int messages_received;                 /* Total messages received */
    
    /* Filtering options */
    char *filter_keywords;                 /* Keyword filters */
    int min_priority;                      /* Minimum message priority */
    bool offline_delivery;                 /* Deliver when offline? */
    
    /* Handler configuration */
    char *handler_name;                    /* Handler function name */
    char *handler_data;                    /* Handler-specific data */
    
    struct pubsub_subscription *next;      /* For linked list */
};
```

### Message Structure

```c
/* Message types */
#define MSG_TYPE_TEXT            0
#define MSG_TYPE_STRUCTURED      1
#define MSG_TYPE_COMMAND         2
#define MSG_TYPE_EVENT           3

/* Message priority levels */
#define MSG_PRIORITY_LOW         1
#define MSG_PRIORITY_NORMAL      2
#define MSG_PRIORITY_HIGH        3
#define MSG_PRIORITY_URGENT      4

### Enhanced Message Structure for Spatial Events

```c
/* Enhanced message types for spatial events */
#define MSG_TYPE_TEXT            0
#define MSG_TYPE_STRUCTURED      1
#define MSG_TYPE_COMMAND         2
#define MSG_TYPE_EVENT           3
#define MSG_TYPE_SPATIAL         4  /* New: Spatial/location-based messages */

/* Spatial message processing flags */
#define SPATIAL_FLAG_TERRAIN_AFFECTS    0x01
#define SPATIAL_FLAG_WEATHER_AFFECTS    0x02
#define SPATIAL_FLAG_ELEVATION_AFFECTS  0x04
#define SPATIAL_FLAG_LINE_OF_SIGHT      0x08

/* Maximum distance variants for spatial messages */
#define MAX_DISTANCE_VARIANTS    5

/* Extended message structure for spatial events */
struct pubsub_spatial_data {
    /* Spatial coordinates */
    int origin_x, origin_y;            /* Event origin coordinates */
    int origin_elevation;              /* Elevation of event origin */
    
    /* Audio properties */
    int max_distance;                  /* Maximum hearing distance */
    int base_volume;                   /* Base volume level (0-100) */
    int frequency;                     /* Sound frequency (affects propagation) */
    
    /* Environmental factors */
    int spatial_flags;                 /* Spatial processing flags */
    int weather_modifier;              /* Weather effect on sound (-50 to +50) */
    
    /* Distance-based message variants */
    char *distance_messages[MAX_DISTANCE_VARIANTS];
    int distance_thresholds[MAX_DISTANCE_VARIANTS]; /* Distance breakpoints */
    
    /* Color codes for different distances */
    char *distance_colors[MAX_DISTANCE_VARIANTS];
};

struct pubsub_message {
    int message_id;                        /* Unique message ID */
    int topic_id;                          /* Target topic */
    int message_type;                      /* Text, structured, spatial, etc. */
    int priority;                          /* Message priority */
    time_t timestamp;                      /* Message creation time */
    time_t expires;                        /* Message expiration time */
    long sender_id;                        /* Sender character ID */
    
    /* Message content */
    char *content;                         /* Primary message content */
    char *metadata;                        /* JSON or key-value metadata */
    size_t content_length;                 /* Content size */
    
    /* Spatial data (only for MSG_TYPE_SPATIAL) */
    struct pubsub_spatial_data *spatial;   /* Spatial event information */
    
    /* Delivery tracking */
    int delivery_attempts;                 /* Number of delivery attempts */
    int successful_deliveries;             /* Successful deliveries count */
    int failed_deliveries;                 /* Failed deliveries count */
    
    /* Processing state */
    bool processed;                        /* Has been processed? */
    time_t processed_time;                 /* When it was processed */
    
    struct pubsub_message *next;           /* For queue linked list */
};
```
```

### Player Subscription Data

**Note**: All PubSub data will be stored in the MySQL database, not in player files.

```c
/* Remove from player_special_data_saved - no longer needed */
/* PubSub data will be fully database-driven */

/* Runtime subscription cache structure (in memory only) */
struct pubsub_player_cache {
    long player_id;                        /* Player unique ID (GET_IDNUM) */
    int subscription_count;                /* Cached subscription count */
    int *subscribed_topics;                /* Array of topic IDs */
    time_t last_cache_update;              /* When cache was last refreshed */
    
    struct pubsub_player_cache *next;      /* For hash table */
};

/* Global subscription cache for online players */
extern struct pubsub_player_cache *subscription_cache[SUBSCRIPTION_CACHE_SIZE];
```

---

## API Design

### Core PubSub Functions

```c
/* Topic Management */
int pubsub_create_topic(const char *name, const char *description, 
                       int category, int access_type, struct char_data *creator);
int pubsub_delete_topic(int topic_id, struct char_data *deleter);
struct pubsub_topic *pubsub_find_topic(const char *name);
struct pubsub_topic *pubsub_get_topic(int topic_id);
int pubsub_list_topics(struct char_data *ch, int category);

/* Subscription Management */
int pubsub_subscribe(struct char_data *ch, int topic_id, const char *handler);
int pubsub_unsubscribe(struct char_data *ch, int topic_id);
int pubsub_list_subscriptions(struct char_data *ch);
int pubsub_pause_subscription(struct char_data *ch, int topic_id);
int pubsub_resume_subscription(struct char_data *ch, int topic_id);

/* Message Publishing */
int pubsub_publish(int topic_id, const char *content, int priority, 
                   struct char_data *sender);
int pubsub_publish_structured(int topic_id, const char *content, 
                              const char *metadata, int priority, 
                              struct char_data *sender);
int pubsub_publish_spatial(int topic_id, int origin_x, int origin_y, 
                          const char **distance_messages, int max_distance,
                          int volume, int spatial_flags, struct char_data *sender);
int pubsub_broadcast(const char *topic_pattern, const char *content, 
                     int priority, struct char_data *sender);

/* Spatial Event Utilities */
bool pubsub_has_listeners_in_radius(int center_x, int center_y, int radius);
int pubsub_calculate_terrain_modifier(int from_x, int from_y, int to_x, int to_y);
int pubsub_calculate_distance_modifier(int distance, int max_distance, 
                                      int base_volume);
int pubsub_get_wilderness_distance(int x1, int y1, int x2, int y2);

/* Enhanced Subscription with Spatial Filtering */
int pubsub_subscribe_spatial(struct char_data *ch, int topic_id, 
                            const char *handler, int max_distance);
int pubsub_set_spatial_filter(struct char_data *ch, int topic_id, 
                             int max_distance, bool terrain_sensitive);

/* Message Processing */
void pubsub_process_messages(void);
void pubsub_process_topic_messages(int topic_id);
int pubsub_deliver_message(struct pubsub_message *msg, long player_id, 
                           const char *handler_name, const char *handler_data);

/* Database Operations */
int pubsub_db_init(void);
int pubsub_db_create_tables(void);
int pubsub_db_load_topics(void);
int pubsub_db_save_topic(struct pubsub_topic *topic);
int pubsub_db_load_subscriptions_for_player(long player_id);
int pubsub_db_cleanup_expired_messages(void);

/* Cache Management */
void pubsub_cache_player_subscriptions(long player_id);
void pubsub_invalidate_player_cache(long player_id);
void pubsub_refresh_subscription_cache(void);

/* Database-Driven Subscription Operations */
int pubsub_load_player_subscriptions(long player_id);
int pubsub_save_subscription(long player_id, int topic_id, const char *handler, 
                            const char *handler_data);
int pubsub_remove_subscription(long player_id, int topic_id);
int pubsub_get_player_subscription_count(long player_id);
bool pubsub_is_subscribed(long player_id, int topic_id);

/* Player Settings Management */
int pubsub_get_player_settings(long player_id, struct pubsub_player_settings *settings);
int pubsub_set_player_settings(long player_id, struct pubsub_player_settings *settings);
bool pubsub_is_player_enabled(long player_id);
int pubsub_get_max_subscriptions(long player_id);

/* Handler System */
int pubsub_register_handler(const char *name, pubsub_handler_func func);
int pubsub_unregister_handler(const char *name);
pubsub_handler_func pubsub_get_handler(const char *name);

/* Utility Functions */
int pubsub_cleanup_expired_messages(void);
int pubsub_cleanup_old_subscriptions(void);
```

### Handler Function Signature

```c
/* Handler function pointer type */
typedef int (*pubsub_handler_func)(struct char_data *subscriber, 
                                   struct pubsub_message *message,
                                   const char *handler_data);

/* Built-in handlers */
int pubsub_handler_send_text(struct char_data *ch, struct pubsub_message *msg, 
                            const char *data);
int pubsub_handler_send_formatted(struct char_data *ch, struct pubsub_message *msg, 
                                 const char *data);
int pubsub_handler_execute_command(struct char_data *ch, struct pubsub_message *msg, 
                                  const char *data);
int pubsub_handler_trigger_event(struct char_data *ch, struct pubsub_message *msg, 
                                const char *data);
int pubsub_handler_log_message(struct char_data *ch, struct pubsub_message *msg, 
                              const char *data);
```

---

## Integration Points

### 1. Game Loop Integration

**Integration Point**: `comm.c` - main game loop  
**Function**: `game_loop()` or `heartbeat()`

```c
/* Add to main game loop, called every pulse */
void game_loop(void) {
    // ... existing game loop code ...
    
    /* Process PubSub messages */
    if (pulse % PULSE_PUBSUB == 0) {
        pubsub_process_messages();
    }
    
    /* Cleanup expired messages periodically */
    if (pulse % (PULSE_PUBSUB * 60) == 0) {
        pubsub_cleanup_expired_messages();
    }
}
```

### 1. Database Initialization

**Integration Point**: `db_init.c` - database table creation

```c
/* Add to init_database_tables() function */
void init_pubsub_tables(void) {
    if (!mysql_available || !conn) {
        log("MySQL not available, skipping PubSub tables initialization");
        return;
    }

    log("Initializing PubSub system tables...");
    
    if (pubsub_db_create_tables() != PUBSUB_SUCCESS) {
        log("SYSERR: Failed to initialize PubSub database tables");
        return;
    }
    
    /* Load existing topics into memory */
    if (pubsub_db_load_topics() != PUBSUB_SUCCESS) {
        log("SYSERR: Failed to load PubSub topics from database");
        return;
    }
    
    log("PubSub system tables initialized successfully");
}

/* Call during startup in boot_db() */
void boot_db_pubsub_init(void) {
    init_pubsub_tables();
    pubsub_init_default_topics();
    pubsub_init_handler_registry();
}
```

### 2. Player Management Integration

**Integration Point**: Player login/logout events

```c
/* In login process - load player's subscription cache */
void pubsub_player_login(struct char_data *ch) {
    long player_id = GET_IDNUM(ch);
    
    /* Load player's subscription data into memory cache */
    pubsub_cache_player_subscriptions(player_id);
    
    /* Check for any offline messages */
    pubsub_deliver_offline_messages(player_id);
    
    /* Update player's last online time */
    pubsub_update_player_last_online(player_id);
}

/* In logout process - clean up player's cache */
void pubsub_player_logout(struct char_data *ch) {
    long player_id = GET_IDNUM(ch);
    
    /* Save any pending subscription changes */
    pubsub_flush_subscription_cache(player_id);
    
    /* Remove from memory cache */
    pubsub_invalidate_player_cache(player_id);
}

/* No longer needed - all data is database-driven */
// void load_char_pubsub_data(struct char_data *ch);
// void save_char_pubsub_data(struct char_data *ch);
```

### 3. Command System Integration

**Integration Point**: `interpreter.c` - command table

```c
/* Add PubSub commands to command table */
{ "subscribe"   , do_subscribe   , 0, 0, 0 },
{ "unsubscribe" , do_unsubscribe , 0, 0, 0 },
{ "topics"      , do_topics      , 0, 0, 0 },
{ "publish"     , do_publish     , LVL_IMMORT, 0, 0 },
{ "topicadmin"  , do_topicadmin  , LVL_GRGOD, 0, 0 },
```

### 4. Event System Integration

**Integration Point**: `mud_event.c` - integrate with existing events

```c
/* Use existing event system for delayed message delivery */
EVENTFUNC(event_pubsub_delayed_delivery);

/* Trigger PubSub messages from existing events */
void trigger_weather_change_event(int weather_type, int zone) {
    char content[MAX_STRING_LENGTH];
    snprintf(content, sizeof(content), "weather_change:%d:%d", weather_type, zone);
    pubsub_publish(pubsub_find_topic("world.weather"), content, 
                   MSG_PRIORITY_NORMAL, NULL);
}
```

### 5. Communication System Integration

**Integration Point**: `act.comm.c` - existing communication commands

```c
/* Enhanced tell system using PubSub */
void perform_tell_via_pubsub(struct char_data *ch, struct char_data *vict, 
                             const char *message) {
    char topic_name[MAX_STRING_LENGTH];
    snprintf(topic_name, sizeof(topic_name), "player.tell.%ld", GET_IDNUM(vict));
    
    /* Create private topic if it doesn't exist */
    int topic_id = pubsub_find_or_create_private_topic(topic_name, vict);
    
    /* Publish the tell message */
    pubsub_publish(topic_id, message, MSG_PRIORITY_HIGH, ch);
}
```

---

## Use Cases and Examples

### 1. Weather System Integration

**Scenario**: Notify players about weather changes in their current zone

```c
/* Weather change publisher */
void notify_weather_change(int zone, int old_weather, int new_weather) {
    struct pubsub_topic *topic = pubsub_find_topic("world.weather.changes");
    char content[MAX_STRING_LENGTH];
    char metadata[MAX_STRING_LENGTH];
    
    snprintf(content, sizeof(content), 
             "The weather changes from %s to %s",
             weather_types[old_weather], weather_types[new_weather]);
             
    snprintf(metadata, sizeof(metadata), 
             "{\"zone\":%d,\"old_weather\":%d,\"new_weather\":%d}",
             zone, old_weather, new_weather);
    
    pubsub_publish_structured(topic->topic_id, content, metadata, 
                             MSG_PRIORITY_NORMAL, NULL);
}

/* Weather subscription handler */
int pubsub_handler_weather_change(struct char_data *ch, struct pubsub_message *msg, 
                                 const char *data) {
    /* Parse metadata to check if player is in affected zone */
    cJSON *json = cJSON_Parse(msg->metadata);
    int zone = cJSON_GetObjectItem(json, "zone")->valueint;
    
    if (world[IN_ROOM(ch)].zone == zone) {
        send_to_char(ch, "\tY%s\tn\r\n", msg->content);
        
        /* Optional: Add weather-specific effects */
        int new_weather = cJSON_GetObjectItem(json, "new_weather")->valueint;
        if (new_weather == WEATHER_LIGHTNING) {
            send_to_char(ch, "\tWLightning illuminates the area!\tn\r\n");
        }
    }
    
    cJSON_Delete(json);
    return PUBSUB_DELIVERY_SUCCESS;
}
```

### 2. Guild Communication System

**Scenario**: Guild-specific communication channels

```c
/* Guild message broadcasting */
void guild_broadcast(int guild_id, const char *message, struct char_data *sender) {
    char topic_name[MAX_STRING_LENGTH];
    snprintf(topic_name, sizeof(topic_name), "guild.%d.broadcast", guild_id);
    
    struct pubsub_topic *topic = pubsub_find_topic(topic_name);
    if (!topic) {
        /* Create guild topic automatically */
        topic_id = pubsub_create_topic(topic_name, "Guild broadcast channel", 
                                      TOPIC_CAT_GUILD, TOPIC_ACCESS_RESTRICTED, sender);
    }
    
    pubsub_publish(topic->topic_id, message, MSG_PRIORITY_NORMAL, sender);
}

/* Auto-subscribe guild members */
void auto_subscribe_guild_member(struct char_data *ch, int guild_id) {
    char topic_name[MAX_STRING_LENGTH];
    snprintf(topic_name, sizeof(topic_name), "guild.%d.broadcast", guild_id);
    
    struct pubsub_topic *topic = pubsub_find_topic(topic_name);
    if (topic) {
        pubsub_subscribe(ch, topic->topic_id, "send_formatted");
    }
}
```

### 3. Quest System Integration

**Scenario**: Notify players about quest updates and completions

```c
/* Quest completion publisher */
void notify_quest_completion(struct char_data *ch, int quest_id) {
    char content[MAX_STRING_LENGTH];
    char metadata[MAX_STRING_LENGTH];
    
    snprintf(content, sizeof(content), 
             "%s has completed the quest '%s'!",
             GET_NAME(ch), quest_table[quest_id].name);
             
    snprintf(metadata, sizeof(metadata),
             "{\"player_id\":%ld,\"quest_id\":%d,\"quest_name\":\"%s\"}",
             GET_IDNUM(ch), quest_id, quest_table[quest_id].name);
    
    /* Publish to zone-specific topic */
    char topic_name[MAX_STRING_LENGTH];
    snprintf(topic_name, sizeof(topic_name), "zone.%d.quests", 
             world[IN_ROOM(ch)].zone);
    
    struct pubsub_topic *topic = pubsub_find_topic(topic_name);
    if (topic) {
        pubsub_publish_structured(topic->topic_id, content, metadata,
                                 MSG_PRIORITY_NORMAL, ch);
    }
}
```

### 4. Administrative Notifications

**Scenario**: System-wide administrative announcements

```c
/* System shutdown notification */
void notify_system_shutdown(int minutes_remaining) {
    char content[MAX_STRING_LENGTH];
    
    if (minutes_remaining > 1) {
        snprintf(content, sizeof(content),
                "\tR[SYSTEM]\tn The MUD will shut down in %d minutes for maintenance.\r\n"
                "Please finish your current activities and find a safe place to quit.",
                minutes_remaining);
    } else {
        snprintf(content, sizeof(content),
                "\tR[SYSTEM]\tn \tYSHUTDOWN IMMINENT!\tn\r\n"
                "The MUD will shut down in less than 1 minute!");
    }
    
    pubsub_publish(pubsub_find_topic("system.announcements")->topic_id, 
                   content, MSG_PRIORITY_URGENT, NULL);
}
```

### 5. Wilderness Audio Events with Distance-Based Effects

**Scenario**: Environmental audio events that affect players differently based on distance and terrain

```c
/* Enhanced message structure for spatial events */
struct pubsub_spatial_message {
    struct pubsub_message base;        /* Base message structure */
    int origin_x, origin_y;            /* Event coordinates */
    int max_distance;                  /* Maximum hearing distance */
    int base_volume;                   /* Base volume level (0-100) */
    bool terrain_affects;              /* Does terrain affect transmission? */
    char *distance_variants[MAX_DISTANCE_VARIANTS]; /* Different messages by distance */
};

/* Spatial event publisher */
void publish_wilderness_audio_event(int x, int y, const char *base_message, 
                                   int max_distance, int volume, 
                                   bool terrain_sensitive) {
    /* Only publish if there are potential listeners nearby */
    if (!has_players_in_radius(x, y, max_distance)) {
        /* Tree falls in forest with no one to hear - no event */
        return;
    }
    
    struct pubsub_spatial_message *spatial_msg = create_spatial_message();
    spatial_msg->origin_x = x;
    spatial_msg->origin_y = y;
    spatial_msg->max_distance = max_distance;
    spatial_msg->base_volume = volume;
    spatial_msg->terrain_affects = terrain_sensitive;
    
    /* Set up distance-based message variants */
    spatial_msg->distance_variants[0] = strdup("A massive tree crashes to the ground with thunderous force!");
    spatial_msg->distance_variants[1] = strdup("You hear the loud crack and crash of a falling tree nearby.");
    spatial_msg->distance_variants[2] = strdup("A distant rumbling crash echoes from somewhere in the forest.");
    spatial_msg->distance_variants[3] = strdup("You hear a faint crashing sound carried on the wind.");
    
    /* Create metadata with spatial information */
    char metadata[MAX_STRING_LENGTH];
    snprintf(metadata, sizeof(metadata),
             "{\"type\":\"spatial_audio\",\"origin\":[%d,%d],\"max_distance\":%d,"
             "\"volume\":%d,\"terrain_sensitive\":%s,\"event_type\":\"tree_fall\"}",
             x, y, max_distance, volume, terrain_sensitive ? "true" : "false");
    
    /* Publish to wilderness audio topic */
    struct pubsub_topic *topic = pubsub_find_topic("world.wilderness.audio");
    pubsub_publish_spatial(topic->topic_id, (struct pubsub_message*)spatial_msg, 
                          metadata, MSG_PRIORITY_NORMAL, NULL);
}

/* Spatial audio handler with distance and terrain calculations */
int pubsub_handler_spatial_audio(struct char_data *ch, struct pubsub_message *msg, 
                                const char *data) {
    struct pubsub_spatial_message *spatial = (struct pubsub_spatial_message*)msg;
    
    /* Check if player is in wilderness */
    if (!IS_WILDERNESS(IN_ROOM(ch))) {
        return PUBSUB_DELIVERY_SUCCESS; /* Skip non-wilderness players */
    }
    
    /* Calculate distance from event */
    int player_x = GET_COORD_X(ch);
    int player_y = GET_COORD_Y(ch);
    int distance = calculate_wilderness_distance(player_x, player_y, 
                                                spatial->origin_x, spatial->origin_y);
    
    /* Check if within hearing range */
    if (distance > spatial->max_distance) {
        return PUBSUB_DELIVERY_SUCCESS; /* Too far to hear */
    }
    
    /* Calculate terrain-based sound modification */
    int terrain_modifier = 0;
    if (spatial->terrain_affects) {
        terrain_modifier = calculate_terrain_sound_modifier(
            player_x, player_y, spatial->origin_x, spatial->origin_y);
    }
    
    /* Determine effective distance with terrain modification */
    int effective_distance = distance + terrain_modifier;
    
    /* Select appropriate message variant based on effective distance */
    int variant_index = MIN(effective_distance / 2, MAX_DISTANCE_VARIANTS - 1);
    const char *message = spatial->distance_variants[variant_index];
    
    /* Calculate volume with distance falloff */
    int volume = spatial->base_volume * (spatial->max_distance - effective_distance) / 
                 spatial->max_distance;
    
    /* Apply additional effects based on distance and terrain */
    char final_message[MAX_STRING_LENGTH];
    if (effective_distance <= 1) {
        /* Very close - full impact with possible ground shake */
        snprintf(final_message, sizeof(final_message), 
                "%s\r\nThe ground trembles beneath your feet!", message);
        send_to_char(ch, "\tR%s\tn\r\n", final_message);
        
        /* Possible minor effects */
        if (GET_POS(ch) < POS_STANDING) {
            send_to_char(ch, "The impact nearly knocks you over!\r\n");
        }
    } else if (effective_distance <= 3) {
        /* Close - clear sound */
        send_to_char(ch, "\tY%s\tn\r\n", message);
    } else if (effective_distance <= 6) {
        /* Moderate distance - muffled */
        send_to_char(ch, "\tG%s\tn\r\n", message);
    } else {
        /* Far - very faint */
        send_to_char(ch, "\tg%s\tn\r\n", message);
    }
    
    /* Log the event for debugging/analysis */
    mudlog(BRF, LVL_IMMORT, TRUE, "SPATIAL_AUDIO: %s at (%d,%d) heard tree fall "
           "from (%d,%d), distance=%d, effective=%d, volume=%d",
           GET_NAME(ch), player_x, player_y, spatial->origin_x, spatial->origin_y,
           distance, effective_distance, volume);
    
    return PUBSUB_DELIVERY_SUCCESS;
}

/* Helper function to check for nearby players before publishing */
bool has_players_in_radius(int center_x, int center_y, int radius) {
    struct descriptor_data *d;
    
    for (d = descriptor_list; d; d = d->next) {
        if (STATE(d) != CON_PLAYING || !d->character) continue;
        if (!IS_WILDERNESS(IN_ROOM(d->character))) continue;
        
        int dist = calculate_wilderness_distance(GET_COORD_X(d->character),
                                               GET_COORD_Y(d->character),
                                               center_x, center_y);
        if (dist <= radius) {
            return TRUE;
        }
    }
    return FALSE;
}

/* Terrain-based sound modification */
int calculate_terrain_sound_modifier(int from_x, int from_y, int to_x, int to_y) {
    int modifier = 0;
    
    /* Trace path between points and accumulate terrain effects */
    int dx = abs(to_x - from_x);
    int dy = abs(to_y - from_y);
    int steps = MAX(dx, dy);
    
    for (int i = 0; i <= steps; i++) {
        int x = from_x + (i * (to_x - from_x)) / steps;
        int y = from_y + (i * (to_y - from_y)) / steps;
        
        int terrain = get_wilderness_terrain(x, y);
        
        switch (terrain) {
            case TERRAIN_FOREST:
                modifier += 1; /* Trees muffle sound */
                break;
            case TERRAIN_MOUNTAIN:
                modifier += 2; /* Mountains block sound significantly */
                break;
            case TERRAIN_HILLS:
                modifier += 1; /* Hills partially block sound */
                break;
            case TERRAIN_WATER:
                modifier -= 1; /* Water carries sound better */
                break;
            case TERRAIN_PLAINS:
                modifier += 0; /* No modification for open plains */
                break;
            default:
                break;
        }
    }
    
    return modifier / (steps + 1); /* Average the modifier */
}
```

### 6. Player-to-Player Messaging Enhancement

**Scenario**: Enhanced tell system with offline message delivery

```c
/* Enhanced tell with PubSub backend */
void perform_enhanced_tell(struct char_data *ch, const char *target_name, 
                          const char *message) {
    struct char_data *vict = get_player_vis(ch, target_name, NULL, FIND_CHAR_WORLD);
    long target_id;
    
    if (vict) {
        target_id = GET_IDNUM(vict);
    } else {
        /* Look up offline player */
        target_id = get_id_by_name(target_name);
        if (target_id == NOBODY) {
            send_to_char(ch, "No such player exists.\r\n");
            return;
        }
    }
    
    /* Create or find personal message topic */
    char topic_name[MAX_STRING_LENGTH];
    snprintf(topic_name, sizeof(topic_name), "player.messages.%ld", target_id);
    
    struct pubsub_topic *topic = pubsub_find_topic(topic_name);
    if (!topic) {
        topic_id = pubsub_create_topic(topic_name, "Personal messages", 
                                      TOPIC_CAT_PLAYER, TOPIC_ACCESS_PRIVATE, ch);
        /* Auto-subscribe the target player */
        pubsub_subscribe_by_id(target_id, topic_id, "handler_personal_message");
    }
    
    /* Format the message */
    char formatted_msg[MAX_STRING_LENGTH];
    snprintf(formatted_msg, sizeof(formatted_msg), "%s tells you, '%s'", 
             GET_NAME(ch), message);
    
    /* Publish the message */
    pubsub_publish(topic->topic_id, formatted_msg, MSG_PRIORITY_HIGH, ch);
    
    /* Confirmation to sender */
    if (vict) {
        send_to_char(ch, "You tell %s, '%s'\r\n", GET_NAME(vict), message);
    } else {
        send_to_char(ch, "You leave a message for %s: '%s'\r\n", 
                     target_name, message);
    }
}
```

---

## Implementation Phases

### Phase 1: Core Infrastructure (Weeks 1-3)

**Deliverables:**
- Basic data structures (topic, subscription, message)
- Core API functions (create, subscribe, publish)
- Memory management and basic safety checks
- Simple message queue implementation
- File-based persistence for topics and subscriptions

**Key Files to Create:**
- `src/pubsub.c` - Core PubSub implementation
- `src/pubsub.h` - Header file with structures and prototypes
- `src/pubsub_handlers.c` - Built-in message handlers
- `src/pubsub_persist.c` - Persistence functions

**Integration Points:**
- Add PubSub data to player structure
- Integrate with game loop for message processing
- Basic command implementations

### Phase 2: Basic Functionality (Weeks 4-5)

**Deliverables:**
- Complete command interface (subscribe, unsubscribe, topics, publish)
- Basic message handlers (send_text, send_formatted)
- Topic management (create, delete, list)
- Player subscription management
- Basic error handling and validation

**Key Commands:**
- `subscribe <topic> [handler]` - Subscribe to a topic
- `unsubscribe <topic>` - Unsubscribe from a topic  
- `topics [category]` - List available topics
- `publish <topic> <message>` - Publish message (admin only)
- `mysubs` - List personal subscriptions

### Phase 3: Advanced Features (Weeks 6-8)

**Deliverables:**
- Message filtering and priority system
- Structured messages with metadata
- Handler registration system
- Topic access control and permissions
- Message persistence and offline delivery
- Performance optimizations

**Enhanced Features:**
- Message expiration and cleanup
- Subscription priorities and preferences
- Bulk operations (batch subscribe/unsubscribe)
- Topic categories and hierarchical naming
- Advanced filtering options

### Phase 4: System Integration (Weeks 9-11)

**Deliverables:**
- Weather system integration
- Enhanced communication system
- Quest system notifications
- Administrative announcement system
- Guild/clan integration
- Zone-specific messaging

**Integration Examples:**
- Weather changes trigger topic messages
- Quest completions notify zone subscribers
- Guild broadcasts use PubSub backend
- System announcements via topics
- Enhanced tell system with offline delivery

### Phase 5: Polish and Optimization (Weeks 12-13)

**Deliverables:**
- Performance profiling and optimization
- Comprehensive error handling
- Administrative tools and monitoring
- Documentation and help files
- Unit and integration testing
- Security audit and hardening

**Quality Assurance:**
- Load testing with high message volume
- Memory leak detection and fixes
- Edge case handling
- Administrative interfaces
- Player experience optimization

---

## Performance Considerations

### Message Processing Optimization

1. **Batched Processing**: Process multiple messages per pulse to reduce overhead
2. **Priority Queues**: Critical messages get immediate processing
3. **Lazy Evaluation**: Only process messages when subscribers are online
4. **Memory Pooling**: Reuse message structures to reduce allocation overhead
5. **Index Optimization**: Hash tables for quick topic and subscription lookups

### Scalability Metrics

**Target Performance:**
- Support 1000+ concurrent subscriptions
- Process 100+ messages per second
- < 1ms average message delivery time
- < 10MB memory overhead for full system
- 99.9% message delivery success rate

### Memory Management

```c
/* Memory pool for message structures */
struct pubsub_message_pool {
    struct pubsub_message *free_messages;
    int pool_size;
    int messages_allocated;
    int peak_usage;
};

/* Efficient message allocation */
struct pubsub_message *pubsub_alloc_message(void) {
    if (message_pool.free_messages) {
        struct pubsub_message *msg = message_pool.free_messages;
        message_pool.free_messages = msg->next;
        memset(msg, 0, sizeof(struct pubsub_message));
        return msg;
    }
    
    /* Allocate new message if pool is empty */
    return (struct pubsub_message *)calloc(1, sizeof(struct pubsub_message));
}
```

### Database Considerations

**Topic Storage:**
- Topics stored in flat files (ASCII format like player files)
- In-memory hash table for fast lookups
- Periodic persistence to prevent data loss

**Subscription Storage:**
- Subscriptions saved with player data
- In-memory subscription lists per topic
- Lazy loading of offline player subscriptions

---

## Security and Safety

### Access Control

1. **Topic Permissions**: Public, private, and restricted topics
2. **Subscription Limits**: Prevent subscription spam
3. **Message Rate Limiting**: Prevent message flooding
4. **Content Filtering**: Basic spam and abuse detection
5. **Administrative Override**: Staff can manage all topics and subscriptions

### Safety Mechanisms

```c
/* Subscription limits per player */
#define MAX_PLAYER_SUBSCRIPTIONS     50
#define MAX_MESSAGES_PER_HOUR       1000
#define MAX_TOPIC_NAME_LENGTH        80
#define MAX_MESSAGE_CONTENT_LENGTH   4096

/* Rate limiting structure */
struct pubsub_rate_limit {
    long player_id;
    int messages_this_hour;
    time_t hour_start;
    int warning_count;
};

/* Safety checks */
bool pubsub_check_subscription_limit(struct char_data *ch) {
    if (ch->player_specials->saved.pubsub_data.subscription_count >= 
        MAX_PLAYER_SUBSCRIPTIONS) {
        send_to_char(ch, "You have reached your subscription limit (%d).\r\n",
                     MAX_PLAYER_SUBSCRIPTIONS);
        return FALSE;
    }
    return TRUE;
}
```

### Error Recovery

1. **Graceful Degradation**: System continues if individual components fail
2. **Message Retry Logic**: Failed deliveries are retried with backoff
3. **Dead Letter Queue**: Messages that can't be delivered are logged
4. **Circuit Breaker**: Temporarily disable problematic topics
5. **Rollback Capability**: Can revert to previous topic/subscription state

---

## Testing Strategy

### Unit Testing

**Core Function Tests:**
- Topic creation, deletion, and lookup
- Subscription management (add, remove, modify)
- Message publishing and queuing
- Handler registration and execution
- Persistence (save/load operations)

### Integration Testing

**System Integration Tests:**
- Game loop integration (message processing during pulses)
- Player login/logout (subscription persistence)
- Command system integration (all PubSub commands)
- Communication system integration (enhanced tell, broadcasts)
- Event system integration (triggering from game events)

### Load Testing

**Performance Tests:**
- 1000 concurrent subscriptions
- 1000 messages published simultaneously
- Player with maximum subscriptions
- Topic with maximum subscribers
- Message queue overflow scenarios

### User Acceptance Testing

**Player Experience Tests:**
- New player subscribes to weather updates
- Guild leader sends broadcast to all members
- Player receives offline tell messages
- Administrative announcement reaches all players
- Player manages subscription preferences

---

## Future Extensions

### Advanced Features (Post-MVP)

1. **Message Templates**: Predefined message formats with variables
2. **Conditional Subscriptions**: Subscribe based on player state/location
3. **Message Aggregation**: Combine similar messages to reduce spam
4. **Topic Aliases**: Multiple names for the same topic
5. **Message Encryption**: Secure private communications
6. **API Integration**: External systems can publish to topics
7. **Message History**: Players can review recent topic messages
8. **Subscription Groups**: Manage multiple subscriptions as a group
9. **Advanced Spatial Effects**: 3D audio simulation with elevation
10. **Weather-Based Propagation**: Dynamic sound modification based on current weather
11. **Visual Spatial Events**: Line-of-sight calculations for visual effects
12. **Scent-Based Propagation**: Wind direction affects scent/smell events

### Integration Opportunities

1. **Mobile App Integration**: Push notifications to external apps
2. **Web Interface**: Manage subscriptions via website
3. **Discord Integration**: Bridge topics to Discord channels
4. **Database Backend**: Move from files to database storage
5. **Clustering Support**: Distribute across multiple servers
6. **REST API**: External access to PubSub system
7. **Webhook Support**: HTTP callbacks for message delivery

### Advanced Use Cases

1. **Dynamic Questing**: Quest updates delivered via topics
2. **Player Matching**: Find players with similar interests
3. **Economic Notifications**: Market price changes, auctions
4. **Combat Logs**: Detailed combat information for groups
5. **Social Features**: Friend activity feeds, status updates
6. **World Events**: Large-scale world changes and consequences
7. **Roleplay Support**: Character background and story updates

---

## Conclusion

The LuminariMUD Publish/Subscribe system represents a significant architectural enhancement that will enable more dynamic, responsive, and interconnected gameplay experiences. By implementing this system in phases, we can ensure robust, well-tested functionality while maintaining backward compatibility with existing systems.

The modular design allows for future extensions and integrations, making this system a solid foundation for years of feature development. The focus on performance, security, and player experience ensures that the system will scale with the MUD's growth and provide lasting value to both players and administrators.

**Next Steps:**
1. Review and refine this design document
2. Create detailed technical specifications for Phase 1
3. Set up development environment and file structure
4. Begin implementation of core data structures
5. Establish testing framework and procedures

---

## Appendices

### A. Command Reference

```
subscribe <topic> [handler] - Subscribe to a topic with optional handler
unsubscribe <topic>         - Unsubscribe from a topic
unsubscribe all             - Unsubscribe from all topics
topics                      - List all available topics
topics <category>           - List topics in specific category
mysubs                      - List your current subscriptions
pubsub pause <topic>        - Pause subscription temporarily
pubsub resume <topic>       - Resume paused subscription
pubsub status               - Show PubSub system status
pubsub history <topic>      - Show recent messages from topic (if enabled)

Administrative Commands:
publish <topic> <message>   - Publish message to topic
topicadmin create <name> <description> <category> <access>
topicadmin delete <topic>
topicadmin list [category]
topicadmin stats <topic>
topicadmin subscribers <topic>
topicadmin moderate <topic> <action>
pubsubadmin status          - Show system-wide statistics
pubsubadmin cleanup         - Force cleanup of expired messages
pubsubadmin reload          - Reload topic configuration
```

### B. Topic Naming Conventions

```
system.*           - System-wide announcements and notifications
world.*            - World events (weather, time, global events)
world.weather.*    - Weather-related events
world.time.*       - Time-based events (dawn, dusk, seasons)
zone.*             - Zone-specific events
zone.<id>.*        - Specific zone events
player.*           - Player-related topics
player.messages.*  - Personal messaging
guild.*            - Guild and clan communications
guild.<id>.*       - Specific guild topics
admin.*            - Administrative topics
quest.*            - Quest-related notifications
combat.*           - Combat-related events
trade.*            - Economic and trading events
rp.*               - Roleplay-related topics
```

### C. Error Codes

```c
#define PUBSUB_SUCCESS                  0
#define PUBSUB_ERROR_INVALID_TOPIC     -1
#define PUBSUB_ERROR_ACCESS_DENIED     -2
#define PUBSUB_ERROR_SUBSCRIPTION_EXISTS -3
#define PUBSUB_ERROR_SUBSCRIPTION_LIMIT -4
#define PUBSUB_ERROR_MESSAGE_TOO_LARGE -5
#define PUBSUB_ERROR_INVALID_HANDLER   -6
#define PUBSUB_ERROR_TOPIC_NOT_FOUND   -7
#define PUBSUB_ERROR_INSUFFICIENT_LEVEL -8
#define PUBSUB_ERROR_SYSTEM_OVERLOAD   -9
#define PUBSUB_ERROR_RATE_LIMITED      -10

#define PUBSUB_DELIVERY_SUCCESS         0
#define PUBSUB_DELIVERY_FAILED         -1
#define PUBSUB_DELIVERY_RETRY          -2
#define PUBSUB_DELIVERY_DEAD_LETTER    -3
```

---

## Code Architecture Guidelines

### Design Principles

1. **Separation of Concerns**: Each module has a single, well-defined responsibility
2. **Fail-Safe Design**: System continues operating even if individual components fail
3. **Memory Safety**: Consistent allocation/deallocation patterns with safety checks
4. **Thread Safety**: Prepared for future multi-threading (though current MUD is single-threaded)
5. **Backward Compatibility**: No breaking changes to existing LuminariMUD systems

### Module Dependencies

```
pubsub.c (core)
├── pubsub_topics.c (topic management)
├── pubsub_subscriptions.c (subscription management)  
├── pubsub_queue.c (message queuing)
├── pubsub_spatial.c (spatial event processing)
├── pubsub_handlers.c (built-in message handlers)
├── pubsub_persist.c (save/load functionality)
└── pubsub_admin.c (administrative functions)

Integration points:
├── comm.c (game loop integration)
├── players.c (character save/load)
├── interpreter.c (command integration)
├── act.comm.c (communication enhancement)
└── utils.c (utility functions)
```

### Coding Standards

```c
/* Function naming conventions */
pubsub_*()           // Public API functions
pubsub_internal_*()  // Internal helper functions
do_pubsub_*()        // Player command functions

/* Error handling pattern */
int function_name(params) {
    /* Input validation */
    if (!param) {
        log("SYSERR: function_name called with NULL param");
        return PUBSUB_ERROR_INVALID_PARAM;
    }
    
    /* Main logic */
    // ... implementation ...
    
    /* Cleanup and return */
    return PUBSUB_SUCCESS;
}

/* Memory allocation pattern */
struct pubsub_topic *topic = NULL;
CREATE(topic, struct pubsub_topic, 1);
if (!topic) {
    log("SYSERR: Failed to allocate memory for topic");
    return PUBSUB_ERROR_MEMORY;
}
// ... use topic ...
// Always paired with cleanup
```

---

## File Structure and Organization

### Core Files to Create

**Primary Implementation:**
```
src/pubsub/
├── pubsub.c              // Core pub/sub engine
├── pubsub.h              // Main header file  
├── pubsub_internal.h     // Internal structures and prototypes
├── pubsub_db.c           // Database operations (MySQL)
├── pubsub_topics.c       // Topic lifecycle management
├── pubsub_subscriptions.c // Subscription management
├── pubsub_queue.c        // Message queuing and processing
├── pubsub_spatial.c      // Spatial event processing
├── pubsub_handlers.c     // Built-in message handlers
├── pubsub_cache.c        // Memory caching for performance
├── pubsub_admin.c        // Administrative commands and utilities
└── pubsub_commands.c     // Player-facing commands (if any)
```

**Configuration Files:**
```
lib/etc/
├── pubsub.conf           // System configuration
└── handlers.conf         // Handler registration
```

**Database Schema:**
```
All PubSub data stored in MySQL database:
- pubsub_topics
- pubsub_subscriptions  
- pubsub_messages
- pubsub_player_settings
- pubsub_statistics
- pubsub_handlers
```

**Log Files:**
```
log/
└── pubsub.log            // PubSub-specific debug and error logs
```

### Header File Organization

**pubsub.h** (public interface):
```c
#ifndef _PUBSUB_H_
#define _PUBSUB_H_

/* Public API prototypes */
/* Data structure definitions for external use */
/* Constants and enums */
/* Error codes */

#endif /* _PUBSUB_H_ */
```

**pubsub_internal.h** (private interface):
```c
#ifndef _PUBSUB_INTERNAL_H_
#define _PUBSUB_INTERNAL_H_

#include "pubsub.h"

/* Internal data structures */
/* Internal function prototypes */
/* Private constants */
/* Debug macros */

#endif /* _PUBSUB_INTERNAL_H_ */
```

---

## Memory Management Strategy

### Allocation Patterns

**Topic Management:**
```c
/* Topic creation with proper cleanup */
struct pubsub_topic *pubsub_create_topic_internal(const char *name) {
    struct pubsub_topic *topic = NULL;
    
    CREATE(topic, struct pubsub_topic, 1);
    if (!topic) return NULL;
    
    topic->name = str_dup(name);
    topic->subscribers = create_list();
    topic->moderators = create_list();
    
    if (!topic->name || !topic->subscribers || !topic->moderators) {
        pubsub_free_topic(topic); // Handles partial cleanup
        return NULL;
    }
    
    return topic;
}

void pubsub_free_topic(struct pubsub_topic *topic) {
    if (!topic) return;
    
    if (topic->name) free(topic->name);
    if (topic->description) free(topic->description);
    if (topic->subscribers) free_list(topic->subscribers);
    if (topic->moderators) free_list(topic->moderators);
    
    free(topic);
}
```

**Message Pooling:**
```c
/* Message pool for performance */
#define PUBSUB_MESSAGE_POOL_SIZE 100

static struct pubsub_message *message_pool = NULL;
static int pool_count = 0;

struct pubsub_message *pubsub_alloc_message(void) {
    if (message_pool) {
        struct pubsub_message *msg = message_pool;
        message_pool = msg->next;
        pool_count--;
        memset(msg, 0, sizeof(struct pubsub_message));
        return msg;
    }
    
    /* Pool empty, allocate new */
    struct pubsub_message *msg;
    CREATE(msg, struct pubsub_message, 1);
    return msg;
}

void pubsub_free_message(struct pubsub_message *msg) {
    if (!msg) return;
    
    /* Free message content */
    if (msg->content) free(msg->content);
    if (msg->metadata) free(msg->metadata);
    if (msg->spatial) pubsub_free_spatial_data(msg->spatial);
    
    /* Return to pool if not full */
    if (pool_count < PUBSUB_MESSAGE_POOL_SIZE) {
        msg->next = message_pool;
        message_pool = msg;
        pool_count++;
    } else {
        free(msg);
    }
}
```

### Memory Safety Checks

```c
/* Macro for safe string duplication */
#define SAFE_STRDUP(dest, src) \
    do { \
        if (src) { \
            dest = str_dup(src); \
            if (!dest) { \
                log("SYSERR: str_dup failed in %s:%d", __FILE__, __LINE__); \
                return PUBSUB_ERROR_MEMORY; \
            } \
        } else { \
            dest = NULL; \
        } \
    } while(0)

/* Memory leak detection in debug mode */
#ifdef DEBUG_PUBSUB
static int total_allocations = 0;
static int total_frees = 0;

#define PUBSUB_MALLOC(size) pubsub_debug_malloc(size, __FILE__, __LINE__)
#define PUBSUB_FREE(ptr) pubsub_debug_free(ptr, __FILE__, __LINE__)
#else
#define PUBSUB_MALLOC(size) malloc(size)
#define PUBSUB_FREE(ptr) free(ptr)
#endif
```

---

## Error Handling Patterns

### Error Code Definitions

```c
/* Success codes */
#define PUBSUB_SUCCESS                    0

/* General errors */
#define PUBSUB_ERROR_INVALID_PARAM       -1
#define PUBSUB_ERROR_MEMORY              -2
#define PUBSUB_ERROR_NOT_FOUND           -3
#define PUBSUB_ERROR_ALREADY_EXISTS      -4
#define PUBSUB_ERROR_ACCESS_DENIED       -5
#define PUBSUB_ERROR_SYSTEM_OVERLOAD     -6

/* Topic-specific errors */
#define PUBSUB_ERROR_TOPIC_NOT_FOUND     -10
#define PUBSUB_ERROR_TOPIC_EXISTS        -11
#define PUBSUB_ERROR_TOPIC_FULL          -12
#define PUBSUB_ERROR_INVALID_TOPIC_NAME  -13

/* Subscription errors */
#define PUBSUB_ERROR_ALREADY_SUBSCRIBED  -20
#define PUBSUB_ERROR_NOT_SUBSCRIBED      -21
#define PUBSUB_ERROR_SUBSCRIPTION_LIMIT  -22
#define PUBSUB_ERROR_INVALID_HANDLER     -23

/* Message errors */
#define PUBSUB_ERROR_MESSAGE_TOO_LARGE   -30
#define PUBSUB_ERROR_MESSAGE_EXPIRED     -31
#define PUBSUB_ERROR_DELIVERY_FAILED     -32
#define PUBSUB_ERROR_RATE_LIMITED        -33

/* Persistence errors */
#define PUBSUB_ERROR_FILE_READ           -40
#define PUBSUB_ERROR_FILE_WRITE          -41
#define PUBSUB_ERROR_CORRUPT_DATA        -42
```

### Error Handling Functions

```c
/* Convert error codes to human-readable messages */
const char *pubsub_strerror(int error_code) {
    static const char *error_messages[] = {
        [0] = "Success",
        [-PUBSUB_ERROR_INVALID_PARAM] = "Invalid parameter",
        [-PUBSUB_ERROR_MEMORY] = "Memory allocation failed",
        [-PUBSUB_ERROR_NOT_FOUND] = "Item not found",
        // ... etc
    };
    
    if (error_code > 0 || error_code < -50) {
        return "Unknown error";
    }
    
    return error_messages[-error_code];
}

/* Centralized error logging */
void pubsub_log_error(int error_code, const char *function, 
                      const char *details) {
    mudlog(BRF, LVL_IMMORT, TRUE, 
           "PUBSUB ERROR in %s: %s (%s)", 
           function, pubsub_strerror(error_code), 
           details ? details : "no details");
}

/* Error handling macro */
#define PUBSUB_CHECK_ERROR(result, function, details) \
    do { \
        if ((result) < 0) { \
            pubsub_log_error((result), (function), (details)); \
            return (result); \
        } \
    } while(0)
```

---

## Integration Requirements

### Required LuminariMUD Knowledge

**Key Files to Understand:**
```c
// structs.h - Character and game structures
struct char_data          // Player/NPC data structure
struct descriptor_data    // Connection information
struct player_special_data_saved // Persistent player data

// comm.c - Game loop and communication
void game_loop()          // Main game loop
void send_to_char()       // Send message to player
char *act()               // Formatted message system

// players.c - Character persistence  
void save_char()          // Save character data
int load_char()           // Load character data

// utils.c - Utility functions
CREATE()                  // Memory allocation macro
str_dup()                 // Safe string duplication
```

**Game Loop Integration Points:**
```c
/* In comm.c heartbeat() function */
void heartbeat(void) {
    // ... existing code ...
    
    /* Process PubSub messages every pulse */
    if (pulse % PULSE_PUBSUB == 0) {
        pubsub_process_messages();
    }
    
    /* Cleanup expired messages every minute */
    if (pulse % (60 * PASSES_PER_SEC) == 0) {
        pubsub_cleanup_expired();
    }
}
```

**Character Data Integration:**
```c
/* Add to player_special_data_saved in structs.h */
struct player_special_data_saved {
    // ... existing fields ...
    
    /* PubSub player data */
    int pubsub_subscription_count;
    bool pubsub_enabled;
    int pubsub_max_subscriptions;
    time_t pubsub_last_cleanup;
    // Note: actual subscriptions stored separately
};
```

### Command System Integration

**Add to interpreter.c command table:**
```c
const struct command_info cmd_info[] = {
    // ... existing commands ...
    
    /* PubSub administrative commands */
    { "pubsub"      , do_pubsub      , LVL_IMMORT, 0, 0 },
    { "topicadmin"  , do_topicadmin  , LVL_GRGOD , 0, 0 },
    { "pubsubstat"  , do_pubsubstat  , LVL_GRGOD , 0, 0 },
    
    // ... rest of commands ...
};
```

**Command Implementation Template:**
```c
ACMD(do_pubsub) {
    char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
    
    two_arguments(argument, arg1, arg2);
    
    if (!*arg1) {
        send_to_char(ch, "Usage: pubsub <command> [arguments]\r\n"
                         "Commands: status, create, delete, list, publish\r\n");
        return;
    }
    
    if (!str_cmp(arg1, "status")) {
        pubsub_show_status(ch);
    } else if (!str_cmp(arg1, "create")) {
        pubsub_admin_create_topic(ch, arg2);
    } else {
        send_to_char(ch, "Unknown pubsub command: %s\r\n", arg1);
    }
}
```

---

## Configuration and Tuning

### System Configuration

**pubsub.conf format:**
```ini
# PubSub System Configuration

[general]
max_topics = 1000
max_subscriptions_per_player = 50
max_message_size = 4096
message_queue_size = 10000
cleanup_interval = 3600

[performance]
message_pool_size = 100
topic_hash_buckets = 256
subscription_hash_buckets = 512
spatial_grid_size = 10

[spatial]
max_spatial_distance = 20
terrain_sound_multiplier = 1.5
weather_sound_modifier = 0.8
elevation_sound_factor = 0.1

[persistence]
auto_save_interval = 300
backup_count = 3
compression_enabled = true

[debug]
log_level = 2
debug_spatial = false
trace_messages = false
```

### Runtime Tuning Parameters

```c
/* Tunable parameters structure */
struct pubsub_config {
    /* General limits */
    int max_topics;
    int max_subscriptions_per_player;
    int max_message_size;
    int message_queue_size;
    
    /* Performance settings */
    int message_pool_size;
    int topic_hash_buckets;
    int subscription_hash_buckets;
    int cleanup_interval;
    
    /* Spatial settings */
    int max_spatial_distance;
    float terrain_sound_multiplier;
    float weather_sound_modifier;
    float elevation_sound_factor;
    
    /* Debug settings */
    int log_level;
    bool debug_spatial;
    bool trace_messages;
};

extern struct pubsub_config pubsub_config;

/* Configuration loading */
int pubsub_load_config(const char *config_file);
int pubsub_reload_config(void);
void pubsub_save_config(const char *config_file);
```

---

## Debugging and Monitoring

### Debug Output System

```c
/* Debug levels */
#define PUBSUB_DEBUG_NONE    0
#define PUBSUB_DEBUG_ERROR   1  
#define PUBSUB_DEBUG_WARN    2
#define PUBSUB_DEBUG_INFO    3
#define PUBSUB_DEBUG_VERBOSE 4

/* Debug macros */
#define pubsub_debug(level, fmt, ...) \
    do { \
        if (pubsub_config.log_level >= (level)) { \
            mudlog(BRF, LVL_IMMORT, TRUE, \
                   "PUBSUB[%s]: " fmt, \
                   pubsub_debug_level_name(level), ##__VA_ARGS__); \
        } \
    } while(0)

#define pubsub_error(fmt, ...)   pubsub_debug(PUBSUB_DEBUG_ERROR, fmt, ##__VA_ARGS__)
#define pubsub_warn(fmt, ...)    pubsub_debug(PUBSUB_DEBUG_WARN, fmt, ##__VA_ARGS__)  
#define pubsub_info(fmt, ...)    pubsub_debug(PUBSUB_DEBUG_INFO, fmt, ##__VA_ARGS__)
#define pubsub_verbose(fmt, ...) pubsub_debug(PUBSUB_DEBUG_VERBOSE, fmt, ##__VA_ARGS__)
```

### System Statistics

```c
/* Statistics structure */
struct pubsub_stats {
    /* Topic statistics */
    int total_topics;
    int active_topics;
    int private_topics;
    
    /* Subscription statistics */
    int total_subscriptions;
    int active_subscriptions;
    int player_subscriptions;
    int system_subscriptions;
    
    /* Message statistics */
    long long total_messages_sent;
    long long total_messages_delivered;
    long long total_messages_failed;
    long long messages_sent_this_hour;
    
    /* Performance statistics */
    int avg_delivery_time_ms;
    int peak_queue_size;
    int current_queue_size;
    
    /* Spatial statistics */
    int spatial_messages_sent;
    int spatial_calculations_performed;
    int players_in_range_checks;
    
    /* Memory statistics */
    int topics_allocated;
    int messages_allocated;
    int subscriptions_allocated;
    int memory_pool_usage;
};

extern struct pubsub_stats pubsub_stats;

/* Statistics functions */
void pubsub_update_stats(void);
void pubsub_reset_stats(void);
void pubsub_show_stats(struct char_data *ch);
```

### Administrative Commands

```c
/* System monitoring commands */
ACMD(do_pubsubstat) {
    send_to_char(ch, "PubSub System Statistics:\r\n");
    send_to_char(ch, "========================\r\n");
    send_to_char(ch, "Topics: %d total, %d active\r\n", 
                 pubsub_stats.total_topics, pubsub_stats.active_topics);
    send_to_char(ch, "Subscriptions: %d total, %d active\r\n",
                 pubsub_stats.total_subscriptions, pubsub_stats.active_subscriptions);
    send_to_char(ch, "Messages: %lld sent, %lld delivered, %lld failed\r\n",
                 pubsub_stats.total_messages_sent,
                 pubsub_stats.total_messages_delivered,
                 pubsub_stats.total_messages_failed);
    send_to_char(ch, "Queue: %d current, %d peak\r\n",
                 pubsub_stats.current_queue_size, pubsub_stats.peak_queue_size);
    send_to_char(ch, "Memory: %d topics, %d messages, %d subscriptions allocated\r\n",
                 pubsub_stats.topics_allocated,
                 pubsub_stats.messages_allocated,
                 pubsub_stats.subscriptions_allocated);
}
```

---

## Migration and Deployment

### Database Schema Migration

**MySQL Schema for PubSub System:**

```sql
-- PubSub Topics table
CREATE TABLE IF NOT EXISTS pubsub_topics (
    topic_id INT AUTO_INCREMENT PRIMARY KEY,
    name VARCHAR(255) UNIQUE NOT NULL,
    description TEXT,
    category INT DEFAULT 0,
    access_type INT DEFAULT 0,
    min_level INT DEFAULT 1,
    creator_id BIGINT,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    last_message_at TIMESTAMP NULL,
    total_messages INT DEFAULT 0,
    subscriber_count INT DEFAULT 0,
    max_subscribers INT DEFAULT -1,
    message_ttl INT DEFAULT 3600,
    is_persistent BOOLEAN DEFAULT TRUE,
    is_active BOOLEAN DEFAULT TRUE,
    
    INDEX idx_name (name),
    INDEX idx_category (category),
    INDEX idx_access_type (access_type),
    INDEX idx_creator (creator_id),
    INDEX idx_active (is_active),
    FOREIGN KEY (creator_id) REFERENCES player_data(id) ON DELETE SET NULL
);

-- PubSub Subscriptions table  
CREATE TABLE IF NOT EXISTS pubsub_subscriptions (
    subscription_id INT AUTO_INCREMENT PRIMARY KEY,
    topic_id INT NOT NULL,
    player_id BIGINT NOT NULL,
    handler_name VARCHAR(64) DEFAULT 'send_text',
    handler_data TEXT,
    status INT DEFAULT 0,
    priority INT DEFAULT 2,
    min_message_priority INT DEFAULT 1,
    offline_delivery BOOLEAN DEFAULT TRUE,
    spatial_max_distance INT DEFAULT -1,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    last_delivered_at TIMESTAMP NULL,
    messages_received INT DEFAULT 0,
    
    UNIQUE KEY unique_player_topic (player_id, topic_id),
    INDEX idx_topic (topic_id),
    INDEX idx_player (player_id),
    INDEX idx_status (status),
    INDEX idx_priority (priority),
    FOREIGN KEY (topic_id) REFERENCES pubsub_topics(topic_id) ON DELETE CASCADE,
    FOREIGN KEY (player_id) REFERENCES player_data(id) ON DELETE CASCADE
);

-- PubSub Messages queue table (for persistent messages)
CREATE TABLE IF NOT EXISTS pubsub_messages (
    message_id INT AUTO_INCREMENT PRIMARY KEY,
    topic_id INT NOT NULL,
    sender_id BIGINT,
    message_type INT DEFAULT 0,
    priority INT DEFAULT 2,
    content TEXT NOT NULL,
    metadata JSON,
    spatial_data JSON,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    expires_at TIMESTAMP NULL,
    delivery_attempts INT DEFAULT 0,
    successful_deliveries INT DEFAULT 0,
    failed_deliveries INT DEFAULT 0,
    is_processed BOOLEAN DEFAULT FALSE,
    processed_at TIMESTAMP NULL,
    
    INDEX idx_topic (topic_id),
    INDEX idx_sender (sender_id),
    INDEX idx_priority (priority),
    INDEX idx_expires (expires_at),
    INDEX idx_processed (is_processed),
    INDEX idx_created (created_at),
    FOREIGN KEY (topic_id) REFERENCES pubsub_topics(topic_id) ON DELETE CASCADE,
    FOREIGN KEY (sender_id) REFERENCES player_data(id) ON DELETE SET NULL
);

-- PubSub Player Settings table
CREATE TABLE IF NOT EXISTS pubsub_player_settings (
    player_id BIGINT PRIMARY KEY,
    is_enabled BOOLEAN DEFAULT TRUE,
    max_subscriptions INT DEFAULT 50,
    default_priority INT DEFAULT 2,
    offline_delivery BOOLEAN DEFAULT TRUE,
    last_cleanup_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    total_messages_received INT DEFAULT 0,
    last_message_at TIMESTAMP NULL,
    
    FOREIGN KEY (player_id) REFERENCES player_data(id) ON DELETE CASCADE
);

-- PubSub Statistics table
CREATE TABLE IF NOT EXISTS pubsub_statistics (
    stat_id INT AUTO_INCREMENT PRIMARY KEY,
    stat_type VARCHAR(64) NOT NULL,
    stat_value BIGINT DEFAULT 0,
    last_updated TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    
    UNIQUE KEY unique_stat_type (stat_type)
);

-- PubSub Handler Registry table
CREATE TABLE IF NOT EXISTS pubsub_handlers (
    handler_id INT AUTO_INCREMENT PRIMARY KEY,
    handler_name VARCHAR(64) UNIQUE NOT NULL,
    description TEXT,
    is_enabled BOOLEAN DEFAULT TRUE,
    usage_count INT DEFAULT 0,
    
    INDEX idx_name (handler_name),
    INDEX idx_enabled (is_enabled)
);

-- Initialize default handlers
INSERT IGNORE INTO pubsub_handlers (handler_name, description) VALUES
('send_text', 'Send plain text message to player'),
('send_formatted', 'Send formatted message with color codes'),
('spatial_audio', 'Process spatial audio events with distance'),
('personal_message', 'Handle personal tell-style messages'),
('system_announcement', 'Handle system-wide announcements'),
('guild_broadcast', 'Handle guild communication messages'),
('quest_notification', 'Handle quest-related notifications');

-- Initialize default statistics
INSERT IGNORE INTO pubsub_statistics (stat_type, stat_value) VALUES
('total_topics_created', 0),
('total_subscriptions_created', 0),
('total_messages_published', 0),
('total_messages_delivered', 0),
('total_delivery_failures', 0);
```

**Index Strategy:**
- Primary keys for fast lookups
- Foreign key constraints to maintain data integrity
- Composite indexes on frequently queried combinations
- Separate indexes for different query patterns

**Data Integrity:**
- Foreign key constraints link to existing `player_data` table
- Cascading deletes for topics/subscriptions
- SET NULL for optional references (creator, sender)
- JSON columns for flexible metadata storage

**File-based Migration:**
```c
/* Convert old data formats if needed */
int pubsub_migrate_data_v1_to_v2(void) {
    /* Migration logic for data format changes */
    pubsub_info("Starting data migration from v1 to v2...");
    
    // Read old format
    // Convert to new format  
    // Write new format
    // Backup old format
    
    pubsub_info("Data migration completed successfully");
    return PUBSUB_SUCCESS;
}
```

### Deployment Checklist

**Pre-deployment:**
- [ ] All files compiled without warnings
- [ ] Unit tests pass
- [ ] Integration tests pass  
- [ ] Memory leak tests pass
- [ ] Performance benchmarks meet requirements
- [ ] Configuration files created
- [ ] Backup of existing system created

**Deployment Steps:**
1. **Stop MUD gracefully** with advance warning to players
2. **Backup existing codebase** and player files
3. **Compile new code** with PubSub system
4. **Run migration scripts** if needed
5. **Start MUD** with new system
6. **Monitor logs** for any issues
7. **Test basic functionality** with admin commands
8. **Announce new features** to development team

**Post-deployment Monitoring:**
- [ ] Check system logs for errors
- [ ] Monitor memory usage
- [ ] Verify message delivery rates
- [ ] Check performance metrics
- [ ] Validate persistence working correctly

**Rollback Plan:**
```bash
#!/bin/bash
# Emergency rollback script
echo "Rolling back PubSub deployment..."
cp -r /backup/luminari-pre-pubsub/* /home/jamie/Luminari-Source/
make clean && make
systemctl restart luminari
echo "Rollback completed"
```

---

## Implementation Readiness Assessment

### ✅ **READY FOR IMPLEMENTATION**

This plan is now complete and implementation-ready. The system provides:

**Core Features:**
- ✅ Database-driven persistence with MySQL integration  
- ✅ Industry-standard PubSub API with player-friendly commands
- ✅ Spatial messaging for wilderness/environmental events
- ✅ Extensible handler system for different message types
- ✅ Performance optimizations with caching and indexes
- ✅ Complete integration with existing LuminariMUD systems

**Architecture Benefits:**
- **Scalable**: Database backend handles thousands of subscriptions efficiently
- **Reliable**: ACID compliance ensures data consistency
- **Maintainable**: Clean separation between API and implementation
- **Extensible**: Plugin-style handlers for custom message processing
- **Performant**: Cached lookups and optimized database queries

**Implementation Priority:**
1. **Phase 1**: Core database schema and basic API functions
2. **Phase 2**: Message processing and delivery system  
3. **Phase 3**: Player commands and administrative tools
4. **Phase 4**: Spatial messaging and advanced features

**Next Steps:**
- Ready to begin database table creation (`pubsub_db_create_tables()`)
- Ready to implement core API functions (`pubsub_init()`, `pubsub_subscribe()`)
- Ready to integrate with existing MUD systems (`db_init.c`, `comm.c`)

**Developer Notes:**
- All function signatures and database schemas are fully specified
- Integration points with existing LuminariMUD code are documented
- Error handling, memory management, and cleanup patterns defined
- Performance considerations and optimization strategies included

**For AI Implementation:**
- Complete technical specification with concrete code examples
- Database-first approach eliminates file-based complexity
- Clear integration patterns with existing MUD architecture
- Comprehensive error handling and edge case coverage

---

*This comprehensive documentation provides everything needed for any developer or AI assistant to understand, implement, maintain, and extend the PubSub system in LuminariMUD. All technical decisions, patterns, and integration points are clearly documented with concrete examples and ready-to-implement code.*
