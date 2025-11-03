# PubSub System API Reference

**System:** LuminariMUD Publish/Subscribe Messaging System  
**Version:** Phase 2B Complete - Spatial Audio Integration  
**Last Updated:** August 13, 2025

---

## 📚 **API Overview**

The LuminariMUD PubSub system provides a comprehensive API for publish/subscribe messaging with advanced spatial audio capabilities. This reference covers all public functions, data structures, and usage patterns.

---

## 🔧 **Core System API**

### System Initialization and Management

#### `int pubsub_init(void)`
**Purpose:** Initialize the PubSub system  
**Returns:** `PUBSUB_SUCCESS` on success, error code on failure  
**Usage:**
```c
if (pubsub_init() != PUBSUB_SUCCESS) {
    log("Failed to initialize PubSub system");
}
```

#### `void pubsub_cleanup(void)`
**Purpose:** Clean up PubSub system resources  
**Returns:** void  
**Usage:**
```c
pubsub_cleanup(); // Called during shutdown
```

#### `int pubsub_enable_system(bool enabled)`
**Purpose:** Enable or disable the PubSub system  
**Parameters:**
- `enabled` - true to enable, false to disable
**Returns:** `PUBSUB_SUCCESS` on success  

---

## 📡 **Message Publishing API**

### Standard Topic Publishing

#### `int pubsub_publish_message(int topic_id, const char *sender_name, const char *content, int priority)`
**Purpose:** Publish a message to a specific topic  
**Parameters:**
- `topic_id` - Target topic ID
- `sender_name` - Name of the sender (player name)
- `content` - Message content (max length varies)
- `priority` - Message priority (see Priority Constants)
**Returns:** `PUBSUB_SUCCESS` on success, error code on failure  
**Usage:**
```c
int result = pubsub_publish_message(
    TOPIC_GENERAL_ANNOUNCEMENTS,
    GET_NAME(ch),
    "Server maintenance in 5 minutes",
    PUBSUB_PRIORITY_HIGH
);
```

### Spatial Audio Publishing (Phase 2B)

#### `int pubsub_publish_wilderness_audio(int source_x, int source_y, int source_z, const char *sender_name, const char *content, int max_distance, int priority)`
**Purpose:** Publish spatial audio in wilderness areas  
**Parameters:**
- `source_x, source_y, source_z` - 3D coordinates of audio source
- `sender_name` - Name of the audio source
- `content` - Audio description (what players hear)
- `max_distance` - Maximum hearing distance (0 = use default)
- `priority` - Message priority
**Returns:** `PUBSUB_SUCCESS` on success  
**Usage:**
```c
int result = pubsub_publish_wilderness_audio(
    X_LOC(ch), Y_LOC(ch), get_modified_elevation(X_LOC(ch), Y_LOC(ch)),
    GET_NAME(ch),
    "A thunderous roar echoes across the mountains",
    40,  // 40-unit hearing range
    PUBSUB_PRIORITY_NORMAL
);
```

---

## 📬 **Subscription Management API**

### Topic Subscription

#### `int pubsub_subscribe_player(const char *player_name, int topic_id, const char *handler_name)`
**Purpose:** Subscribe a player to a topic  
**Parameters:**
- `player_name` - Player to subscribe
- `topic_id` - Topic to subscribe to
- `handler_name` - Message handler to use (e.g., "send_text")
**Returns:** `PUBSUB_SUCCESS` on success  

#### `int pubsub_unsubscribe_player(const char *player_name, int topic_id)`
**Purpose:** Unsubscribe a player from a topic  
**Parameters:**
- `player_name` - Player to unsubscribe
- `topic_id` - Topic to unsubscribe from
**Returns:** `PUBSUB_SUCCESS` on success  

---

## 🗂️ **Topic Management API**

### Topic Creation and Management

#### `int pubsub_create_topic(const char *name, const char *description, int category, int access_type, int min_level)`
**Purpose:** Create a new topic  
**Parameters:**
- `name` - Topic name (unique identifier)
- `description` - Human-readable description
- `category` - Topic category (see Category Constants)
- `access_type` - Access control type
- `min_level` - Minimum level to subscribe
**Returns:** Topic ID on success, negative error code on failure  

---

## 🎵 **Spatial Audio API (Phase 2B)**

### Spatial Message Handlers

#### `int pubsub_handler_wilderness_spatial_audio(struct char_data *ch, struct pubsub_message *msg)`
**Purpose:** Process 3D spatial audio in wilderness areas  
**Parameters:**
- `ch` - Target character (listener)
- `msg` - Message containing spatial data
**Returns:** `PUBSUB_SUCCESS` on successful processing  
**Features:**
- 3D distance calculations with elevation
- Terrain-based audio modification
- Directional information ("sound comes from the north")
- Volume-based message variations

#### `int pubsub_handler_audio_mixing(struct char_data *ch, struct pubsub_message *msg)`
**Purpose:** Handle multiple simultaneous audio sources  
**Parameters:**
- `ch` - Target character
- `msg` - Audio message to mix
**Returns:** `PUBSUB_SUCCESS` on success  
**Features:**
- Multi-source audio blending
- Automatic source expiration (30 seconds)
- Priority-based source management
- Mixing descriptions ("You hear a mixture of sounds...")

---

## 🔄 **Queue Management API**

### Automatic Processing

#### `void pubsub_process_message_queue(void)`
**Purpose:** Process queued messages (called automatically from heartbeat)  
**Returns:** void  
**Notes:** Processes messages every ~0.75 seconds automatically  

### Manual Processing (Administrative)

#### `int pubsub_queue_process_all(void)`
**Purpose:** Process all queued messages immediately  
**Returns:** Number of messages processed  
**Usage:**
```c
int processed = pubsub_queue_process_all();
send_to_char(ch, "Processed %d messages.\r\n", processed);
```

#### `int pubsub_queue_process_batch(int max_messages)`
**Purpose:** Process a specific number of messages  
**Parameters:**
- `max_messages` - Maximum messages to process
**Returns:** Number of messages actually processed  

### Queue Status

#### `bool pubsub_queue_is_enabled(void)`
**Purpose:** Check if queue processing is enabled  
**Returns:** true if enabled, false if disabled  

#### `int pubsub_queue_get_size(void)`
**Purpose:** Get current queue size  
**Returns:** Number of messages in queue  

---

## 📊 **Statistics and Monitoring API**

### System Statistics

#### `struct pubsub_statistics pubsub_get_stats(void)`
**Purpose:** Get comprehensive system statistics  
**Returns:** Statistics structure with performance data  
**Structure:**
```c
struct pubsub_statistics {
    long long total_messages_published;
    long long total_messages_delivered;
    long long total_messages_failed;
    int current_queue_size;
    int peak_queue_size;
    double avg_processing_time_ms;
    // ... additional metrics
};
```

---

## 🎮 **Player Commands API**

### Administrative Commands

#### `ACMD(do_pubsub)`
**Purpose:** Administrative PubSub management command  
**Syntax:** `pubsub [subcommand]`  
**Subcommands:**
- `status` - Show system status
- `stats` - Show detailed statistics
- `enable/disable` - Control system state

#### `ACMD(do_pubsubqueue)`
**Purpose:** Queue management and testing command  
**Syntax:** `pubsubqueue [subcommand]`  
**Subcommands:**
- `status` - Show queue status
- `process` - Manually process queue
- `spatial` - Test spatial audio
- `start/stop` - Control queue processing

---

## 🔧 **Data Structures**

### Core Message Structure

```c
struct pubsub_message {
    int message_id;             // Unique message identifier
    int topic_id;               // Target topic (0 for spatial)
    char *sender_name;          // Sender identification
    int message_type;           // Message type constant
    int priority;               // Priority level
    char *content;              // Message content
    char *metadata;             // Additional metadata
    char *spatial_data;         // Spatial coordinates "x,y,z,range"
    time_t created_at;          // Creation timestamp
    time_t expires_at;          // Expiration timestamp
    int delivery_attempts;      // Delivery attempt count
    int successful_deliveries;  // Successful delivery count
    int failed_deliveries;      // Failed delivery count
    bool is_processed;          // Processing status
    time_t processed_at;        // Processing timestamp
    int reference_count;        // Memory management counter
    struct pubsub_message *next; // Linked list pointer
};
```

### Spatial Audio Source

```c
struct spatial_audio_source {
    int source_x, source_y, source_z;  // 3D coordinates
    char *content;                     // Audio description
    char *sender_name;                 // Source identifier
    time_t created_at;                 // Creation time
    time_t expires_at;                 // Auto-cleanup time
    int priority;                      // Source priority
    float volume_modifier;             // Calculated volume
};
```

---

## 📋 **Constants and Enums**

### Return Codes

```c
#define PUBSUB_SUCCESS                  0
#define PUBSUB_ERROR_GENERAL           -1
#define PUBSUB_ERROR_DATABASE          -2
#define PUBSUB_ERROR_MEMORY            -3
#define PUBSUB_ERROR_NOT_FOUND         -4
#define PUBSUB_ERROR_DUPLICATE         -5
#define PUBSUB_ERROR_PERMISSION        -6
#define PUBSUB_ERROR_SUBSCRIPTION_LIMIT -7
#define PUBSUB_ERROR_TOPIC_FULL        -8
#define PUBSUB_ERROR_HANDLER_NOT_FOUND -9
#define PUBSUB_ERROR_INVALID_MESSAGE   -10
#define PUBSUB_ERROR_QUEUE_FULL        -11
#define PUBSUB_ERROR_QUEUE_DISABLED    -12
#define PUBSUB_ERROR_INVALID_PARAMETER -13
```

### Message Priorities

```c
#define PUBSUB_PRIORITY_CRITICAL    4    // Highest priority
#define PUBSUB_PRIORITY_URGENT      3    // High priority
#define PUBSUB_PRIORITY_HIGH        2    // Above normal
#define PUBSUB_PRIORITY_NORMAL      1    // Standard priority
#define PUBSUB_PRIORITY_LOW         0    // Lowest priority
```

### Message Types

```c
#define PUBSUB_MESSAGE_STANDARD     0    // Regular topic message
#define PUBSUB_MESSAGE_SPATIAL      1    // Spatial audio message
#define PUBSUB_MESSAGE_SYSTEM       2    // System announcement
#define PUBSUB_MESSAGE_PERSONAL     3    // Personal message
```

### Spatial Audio Constants

```c
#define SPATIAL_AUDIO_MAX_DISTANCE      50   // Maximum hearing range
#define SPATIAL_AUDIO_CLOSE_THRESHOLD   5    // Very close range
#define SPATIAL_AUDIO_MID_THRESHOLD     15   // Medium range
#define SPATIAL_AUDIO_FAR_THRESHOLD     30   // Far range
#define MAX_CONCURRENT_AUDIO_SOURCES    5    // Max simultaneous sources
```

---

## 🎯 **Usage Patterns**

### Basic Message Publishing
```c
// Simple announcement
pubsub_publish_message(
    TOPIC_ANNOUNCEMENTS,
    "System",
    "Server restart in 10 minutes",
    PUBSUB_PRIORITY_HIGH
);
```

### Spatial Audio in Game Events
```c
// Combat sound
if (ZONE_FLAGGED(GET_ROOM_ZONE(IN_ROOM(ch)), ZONE_WILDERNESS)) {
    pubsub_publish_wilderness_audio(
        X_LOC(ch), Y_LOC(ch), get_modified_elevation(X_LOC(ch), Y_LOC(ch)),
        GET_NAME(ch),
        "The clash of steel rings across the battlefield",
        30,  // 30-unit range
        PUBSUB_PRIORITY_NORMAL
    );
}
```

### Environmental Audio
```c
// Weather effects
pubsub_publish_wilderness_audio(
    weather_x, weather_y, weather_z,
    "Weather System",
    "Thunder rumbles ominously overhead",
    50,  // Maximum range
    PUBSUB_PRIORITY_LOW
);
```

### Multi-Character Interaction
```c
// Group spell casting
for (int i = 0; i < group_size; i++) {
    pubsub_publish_wilderness_audio(
        X_LOC(group[i]), Y_LOC(group[i]), get_elevation(group[i]),
        GET_NAME(group[i]),
        "Magical energy crackles through the air",
        25,
        PUBSUB_PRIORITY_NORMAL
    );
}
```

---

## 🔍 **Error Handling**

### Checking Return Values
```c
int result = pubsub_publish_wilderness_audio(...);
switch (result) {
    case PUBSUB_SUCCESS:
        // Success - message sent
        break;
    case PUBSUB_ERROR_MEMORY:
        log("PubSub: Memory allocation failed");
        break;
    case PUBSUB_ERROR_PERMISSION:
        log("PubSub: System disabled");
        break;
    default:
        log("PubSub: Unknown error %d", result);
        break;
}
```

### Debug Logging
```c
// Enable debug logging
#define PUBSUB_DEBUG 1

// Debug messages will appear in logs
pubsub_debug("Processed spatial audio for %s at (%d,%d,%d)", 
            GET_NAME(ch), x, y, z);
```

---

## 🚀 **Performance Considerations**

### Memory Management
- Messages use reference counting for safe cleanup
- Audio sources auto-expire after 30 seconds
- Queue processing is batched for efficiency

### Optimization Tips
- Use appropriate message priorities
- Set reasonable spatial audio ranges
- Consider terrain effects in range calculations
- Monitor queue size during high-traffic periods

### Scalability Guidelines
- System tested with 100+ concurrent players
- Memory usage: ~1KB per active player
- Processing time: < 1ms per message
- Queue processing: Every 0.75 seconds automatically

---

*Complete API reference for LuminariMUD PubSub System*  
*For implementation details, see: [TECHNICAL_OVERVIEW.md](TECHNICAL_OVERVIEW.md)*  
*For testing procedures, see: [../testing/PHASE_2B_TESTING_GUIDE.md](../testing/PHASE_2B_TESTING_GUIDE.md)*
