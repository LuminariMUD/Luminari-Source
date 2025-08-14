# PubSub System Technical Overview

**System:** LuminariMUD Publish/Subscribe Messaging System  
**Architecture:** Multi-component messaging system with spatial audio capabilities  
**Status:** Phase 2B Complete - Spatial Audio Integration  
**Last Updated:** August 13, 2025

---

## 🏗️ **System Architecture**

### Core Components

```
┌─────────────────────────────────────────────────────────────┐
│                    LuminariMUD PubSub System                │
├─────────────────────────────────────────────────────────────┤
│  Core Infrastructure (Phase 1)                             │
│  ├── pubsub.c/.h           # Core system & API             │
│  ├── pubsub_db.c/.h        # Database operations           │
│  └── pubsub_handlers.c/.h  # Message handler system        │
├─────────────────────────────────────────────────────────────┤
│  Message Processing (Phase 2A)                             │
│  ├── pubsub_queue.c/.h     # Message queue & processing    │
│  ├── pubsub_commands.c/.h  # Administrative commands       │
│  └── Automatic Processing  # Heartbeat integration         │
├─────────────────────────────────────────────────────────────┤
│  Spatial Audio (Phase 2B) ✅ COMPLETE                      │
│  ├── pubsub_spatial.c/.h   # 3D spatial audio engine       │
│  ├── Terrain Integration   # Wilderness system integration │
│  ├── Distance Calculations # 3D positioning & falloff      │
│  └── Multi-Source Mixing   # Simultaneous audio sources    │
└─────────────────────────────────────────────────────────────┘
```

---

## 📡 **Message Flow Architecture**

### Standard Message Flow
```
[Publisher] → [Topic] → [Subscription] → [Queue] → [Handler] → [Subscriber]
     ↓
  Content    Priority   Player List    Batch      Processing   Delivery
             Metadata   Filtering      Queue      Logic        Output
```

### Spatial Audio Flow (Phase 2B)
```
[Spatial Source] → [3D Positioning] → [Range Discovery] → [Terrain Calc] → [Queue] → [Delivery]
       ↓                 ↓                    ↓                ↓              ↓         ↓
   X,Y,Z coords     Distance calc      Find players     Apply modifiers   Auto-process  Spatial msg
   Max range        Elevation diff     Within range     Forest/Mountain   Heartbeat     + Direction
   Content          Direction calc     Wilderness only  Water/Plains      Every 0.75s   + Volume
```

---

## 🔧 **Core Data Structures**

### Message Structure
```c
struct pubsub_message {
    int message_id;
    int topic_id;
    char *sender_name;
    int message_type;           // Regular, Spatial, etc.
    int priority;               // Critical, Urgent, High, Normal, Low
    char *content;
    char *metadata;
    char *spatial_data;         // "x,y,z,range" for spatial messages
    time_t created_at;
    time_t expires_at;
    int reference_count;        // Memory management
    // ... delivery tracking fields
};
```

### Spatial Audio Data
```c
struct spatial_audio_source {
    int source_x, source_y, source_z;
    char *content;
    char *sender_name;
    time_t created_at;
    time_t expires_at;          // 30-second auto-cleanup
    int priority;
    float volume_modifier;
};
```

---

## ⚡ **Performance Characteristics**

### Computational Complexity
- **Distance Calculation:** O(1) per player per message
- **Terrain Lookup:** O(1) sector and elevation queries  
- **Player Discovery:** O(n) where n = online players
- **Queue Processing:** O(m) where m = queued messages (batched)

### Memory Usage
- **Per Message:** ~300 bytes (including spatial data)
- **Per Audio Source:** ~200 bytes (coordinates + metadata)
- **Maximum per Player:** ~1KB (5 audio sources max)
- **System Overhead:** Minimal impact on server resources

### Processing Speed
- **Message Processing:** < 1ms per message
- **Queue Processing:** Every 0.75 seconds (automatic)
- **Spatial Calculations:** < 0.1ms per player per audio event
- **Terrain Modifiers:** Cached for performance

---

## 🌍 **Spatial Audio Physics Engine**

### Distance Calculation
```c
// 3D distance with elevation weighting (Z-axis has less impact)
distance_3d = sqrt(dx² + dy² + (dz/4)²)
```

### Terrain Modifiers
| Terrain Type | Modifier | Effect | Audio Range Impact |
|--------------|----------|--------|-------------------|
| Forest | 0.7x | Trees absorb sound | Reduces by ~30% |
| Mountains/Hills | 1.3x | Echo enhancement | Extends by ~30% |
| Water Bodies | 1.2x | Sound carries over water | Extends by ~20% |
| Plains/Fields | 1.0x | Normal propagation | No change |
| Underground | 0.5x | Heavily muffled | Reduces by ~50% |

### Modifier Calculation
```c
// Uses BOTH source and listener terrain
listener_modifier = get_terrain_modifier(listener_terrain);
source_modifier = get_terrain_modifier(source_terrain);
final_modifier = (listener_modifier + source_modifier) / 2.0f;
```

### Volume Zones
| Distance | Volume % | Description | Color Code |
|----------|----------|-------------|------------|
| 0-5 units | 90-100% | "echoes powerfully" | Red |
| 5-15 units | 70-90% | "clearly hear nearby" | Yellow |
| 15-25 units | 50-70% | "carried on the wind" | Cyan |
| 25-35 units | 30-50% | "faintly in the distance" | Blue |
| 35-45 units | 10-30% | "barely make out" | Blue |
| 45+ units | 0% | No message (too far) | None |

---

## 🔄 **Integration Points**

### Game Loop Integration
```c
// In comm.c heartbeat() function
if (!(heart_pulse % PASSES_PER_SEC)) {
    // ... other game systems
    pubsub_process_message_queue();  // Process PubSub messages
}
```

### Wilderness System Integration
- **Coordinate System:** `X_LOC(ch)`, `Y_LOC(ch)` macros
- **Elevation Data:** `get_modified_elevation(x, y)`
- **Terrain Detection:** `get_modified_sector_type(zone, x, y)`
- **Zone Detection:** `ZONE_FLAGGED(zone, ZONE_WILDERNESS)`

### Memory Management
- **Reference Counting:** Messages freed when all queue nodes processed
- **Automatic Cleanup:** Audio sources expire after 30 seconds
- **Safe Allocation:** All allocations checked with error handling
- **Shutdown Integration:** Proper cleanup during system shutdown

---

## 📊 **Database Schema**

### Core Tables (Phase 1)
```sql
-- Topics (message categories)
pubsub_topics (
    topic_id, name, description, category, access_type,
    min_level, creator_name, created_at, last_message_at,
    total_messages, subscriber_count, max_subscribers,
    message_ttl, is_persistent, is_active
)

-- Subscriptions (player -> topic mapping)
pubsub_subscriptions (
    subscription_id, topic_id, player_name, handler_name,
    handler_data, status, subscribed_at
)

-- Messages (persistent storage - optional)
pubsub_messages (
    message_id, topic_id, sender_name, message_type,
    priority, content, spatial_data, metadata,
    created_at, expires_at, delivery_attempts
)
```

### Spatial Extensions (Phase 2B)
- **Spatial Data:** Stored in `spatial_data` field as "x,y,z,range"
- **Message Types:** `PUBSUB_MESSAGE_SPATIAL` for spatial audio
- **Handler Data:** Spatial-specific handler configurations

---

## 🛠️ **API Reference**

### Core Publishing API
```c
// Standard topic-based publishing
int pubsub_publish_message(int topic_id, const char *sender_name, 
                          const char *content, int priority);

// Spatial audio publishing (Phase 2B)
int pubsub_publish_wilderness_audio(int source_x, int source_y, int source_z,
                                   const char *sender_name, const char *content,
                                   int max_distance, int priority);
```

### Queue Management API
```c
// Automatic processing (called from heartbeat)
void pubsub_process_message_queue(void);

// Manual processing (administrative)
int pubsub_queue_process_all(void);
int pubsub_queue_process_batch(int max_messages);
```

### Spatial Audio API
```c
// Main spatial handler
int pubsub_handler_wilderness_spatial_audio(struct char_data *ch, 
                                           struct pubsub_message *msg);

// Multi-source audio mixing
int pubsub_handler_audio_mixing(struct char_data *ch, 
                               struct pubsub_message *msg);
```

---

## 🔧 **Configuration Constants**

### Spatial Audio Settings
```c
#define SPATIAL_AUDIO_MAX_DISTANCE      50    // Maximum hearing distance
#define SPATIAL_AUDIO_CLOSE_THRESHOLD   5     // Very close audio threshold
#define SPATIAL_AUDIO_MID_THRESHOLD     15    // Moderate distance threshold
#define SPATIAL_AUDIO_FAR_THRESHOLD     30    // Far distance threshold
#define MAX_CONCURRENT_AUDIO_SOURCES    5     // Max simultaneous sources
```

### Queue Processing Settings
```c
#define PUBSUB_QUEUE_BATCH_SIZE         20    // Messages per batch
#define PUBSUB_QUEUE_MAX_SIZE           1000  // Maximum queue size
#define PUBSUB_QUEUE_PROCESS_INTERVAL   100   // Milliseconds (unused)
```

### System Limits
```c
#define PUBSUB_MAX_TOPICS               100   // Maximum topics
#define PUBSUB_MAX_SUBSCRIPTIONS_PER_PLAYER 20 // Per-player limit
#define PUBSUB_MESSAGE_TTL_DEFAULT      3600  // 1 hour default TTL
```

---

## 🚀 **Performance Optimizations**

### Current Optimizations
- **Batch Processing:** Process multiple messages per heartbeat
- **Reference Counting:** Prevent premature message cleanup
- **Terrain Caching:** Cache terrain lookups for performance
- **Range Filtering:** Only process players within audio range
- **Memory Pooling:** Efficient allocation/deallocation patterns

### Future Optimization Opportunities (Phase 3+)
- **Spatial Indexing:** KD-trees for faster player discovery
- **Compression:** Message compression for large events
- **Threading:** Background processing for complex calculations
- **Caching:** Enhanced caching for frequently accessed data

---

## 🔍 **Debugging and Monitoring**

### Debug Logging
```c
pubsub_debug("Auto-processed %d messages from queue", processed);
pubsub_info("Published wilderness audio from (%d,%d,%d), delivered to %d players", 
            source_x, source_y, source_z, processed);
pubsub_error("Failed to parse spatial data: '%s'", msg->spatial_data);
```

### Performance Monitoring
- **Message Processing Time:** Tracked per batch
- **Queue Size Monitoring:** Track peak queue usage
- **Memory Usage:** Monitor audio source allocation
- **Error Rates:** Track failed deliveries and retries

### Administrative Commands
```
pubsub                 # System status
pubsubqueue status     # Queue statistics  
pubsubqueue spatial    # Test spatial audio
pubsubqueue process    # Manual processing (if needed)
pubsubstat            # Detailed system statistics
```

---

*Technical documentation for LuminariMUD PubSub System*  
*Maintained by: LuminariMUD Development Team*  
*Last Updated: August 13, 2025*
