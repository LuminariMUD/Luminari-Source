# Spatial Visual System Implementation Plan
## LuminariMUD Wilderness Visual Stimulus System

**Date:** August 13, 2025  
**Author:** Luminari Development Team  
**Status:** Planning Phase  

---

## 🎯 **Project Overview**

Implement a comprehensive spatial visual system for the wilderness that allows players to see dynamic events based on line of sight, distance, and terrain interference. This system will use the PubSub architecture for event-driven visual stimulus delivery.

### **Core Concept**
Visual stimuli (ships, flying objects, cities, etc.) are published as spatial events and delivered to players based on:
- **Line of Sight calculations** (existing wilderness map system)
- **Distance calculations** with realistic falloff
- **Terrain interference** (mountains block view, hills reduce visibility)
- **Weather conditions** (fog, storms affect visibility)
- **Time of day** (darkness limits vision range)

---

## 🏗️ **Architecture Design**

### **Triple Strategy Pattern Implementation**
The system uses three independent strategy patterns for maximum flexibility:

```c
/* 1. STIMULUS STRATEGY - What is being transmitted */
struct stimulus_strategy {
    char *name;
    int stimulus_type;
    float base_range;
    int (*calculate_intensity)(struct spatial_context *ctx);
    int (*generate_base_message)(struct spatial_context *ctx, char *output, size_t max_len);
    int (*apply_stimulus_effects)(struct spatial_context *ctx);
};

/* 2. LINE OF SIGHT STRATEGY - How transmission is blocked */
struct los_strategy {
    char *name;
    int (*calculate_obstruction)(struct spatial_context *ctx, float *obstruction_factor);
    int (*get_blocking_elements)(struct spatial_context *ctx, struct obstacle_list *obstacles);
    bool (*can_transmit_through)(int terrain_type, int stimulus_type);
};

/* 3. MODIFIER STRATEGY - Environmental effects on transmission */
struct modifier_strategy {
    char *name;
    int (*apply_environmental_modifiers)(struct spatial_context *ctx, float *range_mod, float *clarity_mod);
    int (*calculate_interference)(struct spatial_context *ctx, float *interference);
    int (*modify_message)(struct spatial_context *ctx, char *message, size_t max_len);
};

/* UNIFIED SPATIAL SYSTEM */
struct spatial_system {
    struct stimulus_strategy *stimulus;
    struct los_strategy *line_of_sight;
    struct modifier_strategy *modifiers;
    char *system_name;
    bool enabled;
};
```

### **Example System Configurations**
```c
/* VISUAL SYSTEM */
struct spatial_system visual_system = {
    .stimulus = &visual_stimulus_strategy,
    .line_of_sight = &physical_los_strategy,
    .modifiers = &weather_terrain_modifier_strategy,
    .system_name = "Visual",
    .enabled = true
};

/* AUDIO SYSTEM */
struct spatial_system audio_system = {
    .stimulus = &audio_stimulus_strategy,
    .line_of_sight = &acoustic_los_strategy,        /* Sound travels around corners */
    .modifiers = &atmospheric_modifier_strategy,    /* Wind, air density */
    .system_name = "Audio",
    .enabled = true
};

/* EMPATHY SYSTEM (Future) */
struct spatial_system empathy_system = {
    .stimulus = &empathy_stimulus_strategy,
    .line_of_sight = &mental_los_strategy,          /* Blocked by living minds */
    .modifiers = &emotional_modifier_strategy,       /* Affected by emotions */
    .system_name = "Empathy",
    .enabled = false  /* Not implemented yet */
};

/* MAGICAL SYSTEM (Future) */
struct spatial_system magical_system = {
    .stimulus = &magical_stimulus_strategy,
    .line_of_sight = &ethereal_los_strategy,        /* Ignores physical barriers */
    .modifiers = &mana_field_modifier_strategy,      /* Affected by magical fields */
    .system_name = "Magical",
    .enabled = false
};
```

### **Unified Spatial Context**
```c
struct spatial_context {
    /* Source Information */
    int source_x, source_y, source_z;
    char *source_description;
    int stimulus_type;
    float base_intensity;
    
    /* Observer Information */
    struct char_data *observer;
    int observer_x, observer_y, observer_z;
    
    /* Environmental Factors */
    int weather_conditions;
    int time_of_day;
    struct char_data **nearby_entities;    /* For empathy system */
    int entity_count;
    float magical_field_strength;          /* For magical systems */
    
    /* Calculated Values */
    float distance;
    float effective_range;
    float obstruction_factor;
    float environmental_modifier;
    float final_intensity;
    char *processed_message;
    
    /* System Configuration */
    struct spatial_system *active_system;
};
```

### **Strategy Implementations**

#### **Stimulus Strategies**
```c
/* Visual Stimulus - Light-based transmission */
struct stimulus_strategy visual_stimulus_strategy = {
    .name = "Visual",
    .stimulus_type = STIMULUS_VISUAL,
    .base_range = 1000.0,  /* Base visual range in wilderness units */
    .calculate_intensity = visual_calculate_intensity,
    .generate_base_message = visual_generate_message,
    .apply_stimulus_effects = visual_apply_effects
};

/* Audio Stimulus - Sound-based transmission */
struct stimulus_strategy audio_stimulus_strategy = {
    .name = "Audio", 
    .stimulus_type = STIMULUS_AUDIO,
    .base_range = 500.0,   /* Sound travels less far than sight */
    .calculate_intensity = audio_calculate_intensity,
    .generate_base_message = audio_generate_message,
    .apply_stimulus_effects = audio_apply_effects
};

/* Empathy Stimulus - Emotion-based transmission */
struct stimulus_strategy empathy_stimulus_strategy = {
    .name = "Empathy",
    .stimulus_type = STIMULUS_EMPATHY,
    .base_range = 100.0,   /* Close range only */
    .calculate_intensity = empathy_calculate_intensity,
    .generate_base_message = empathy_generate_message,
    .apply_stimulus_effects = empathy_apply_effects
};
```

#### **Line of Sight Strategies**
```c
/* Physical LOS - Blocked by solid terrain */
struct los_strategy physical_los_strategy = {
    .name = "Physical",
    .calculate_obstruction = physical_calculate_obstruction,
    .get_blocking_elements = physical_get_obstacles,
    .can_transmit_through = physical_can_transmit
};

/* Acoustic LOS - Sound travels around corners, through some materials */
struct los_strategy acoustic_los_strategy = {
    .name = "Acoustic",
    .calculate_obstruction = acoustic_calculate_obstruction,
    .get_blocking_elements = acoustic_get_obstacles,
    .can_transmit_through = acoustic_can_transmit
};

/* Mental LOS - Blocked by living minds, not physical barriers */
struct los_strategy mental_los_strategy = {
    .name = "Mental",
    .calculate_obstruction = mental_calculate_obstruction,
    .get_blocking_elements = mental_get_obstacles,      /* Returns nearby entities */
    .can_transmit_through = mental_can_transmit         /* Always true for terrain */
};

/* Ethereal LOS - Ignores physical barriers entirely */
struct los_strategy ethereal_los_strategy = {
    .name = "Ethereal",
    .calculate_obstruction = ethereal_calculate_obstruction, /* Always 0 */
    .get_blocking_elements = ethereal_get_obstacles,         /* Returns empty list */
    .can_transmit_through = ethereal_can_transmit            /* Always true */
};
```

#### **Modifier Strategies**
```c
/* Weather/Terrain Modifiers - Affects visual and some audio */
struct modifier_strategy weather_terrain_modifier_strategy = {
    .name = "Weather_Terrain",
    .apply_environmental_modifiers = weather_terrain_apply_modifiers,
    .calculate_interference = weather_terrain_calculate_interference,
    .modify_message = weather_terrain_modify_message
};

/* Atmospheric Modifiers - Affects audio transmission */
struct modifier_strategy atmospheric_modifier_strategy = {
    .name = "Atmospheric", 
    .apply_environmental_modifiers = atmospheric_apply_modifiers,
    .calculate_interference = atmospheric_calculate_interference,
    .modify_message = atmospheric_modify_message
};

/* Emotional Modifiers - Affects empathy transmission */
struct modifier_strategy emotional_modifier_strategy = {
    .name = "Emotional",
    .apply_environmental_modifiers = emotional_apply_modifiers,
    .calculate_interference = emotional_calculate_interference, /* Crowd dampening */
    .modify_message = emotional_modify_message
};

/* Mana Field Modifiers - Affects magical transmission */
struct modifier_strategy mana_field_modifier_strategy = {
    .name = "Mana_Field",
    .apply_environmental_modifiers = mana_field_apply_modifiers,
    .calculate_interference = mana_field_calculate_interference,
    .modify_message = mana_field_modify_message
};
```

### **Core Processing Engine**
```c
/* Main processing function that uses all three strategies */
int spatial_process_stimulus(struct spatial_context *ctx, struct spatial_system *system) {
    float obstruction_factor = 0.0;
    float range_modifier = 1.0;
    float clarity_modifier = 1.0;
    
    /* Step 1: Calculate base stimulus intensity */
    if (system->stimulus->calculate_intensity(ctx) != SPATIAL_SUCCESS) {
        return SPATIAL_ERROR_STIMULUS;
    }
    
    /* Step 2: Check line of sight using appropriate strategy */
    if (system->line_of_sight->calculate_obstruction(ctx, &obstruction_factor) != SPATIAL_SUCCESS) {
        return SPATIAL_ERROR_LOS;
    }
    
    /* Step 3: Apply environmental modifiers */
    if (system->modifiers->apply_environmental_modifiers(ctx, &range_modifier, &clarity_modifier) != SPATIAL_SUCCESS) {
        return SPATIAL_ERROR_MODIFIERS;
    }
    
    /* Step 4: Calculate final transmission */
    ctx->final_intensity = ctx->base_intensity * (1.0 - obstruction_factor) * range_modifier;
    
    /* Step 5: Generate appropriate message if stimulus is strong enough */
    if (ctx->final_intensity > MINIMUM_STIMULUS_THRESHOLD) {
        if (system->stimulus->generate_base_message(ctx, ctx->processed_message, MAX_MESSAGE_LENGTH) == SPATIAL_SUCCESS) {
            /* Allow modifiers to adjust the message */
            system->modifiers->modify_message(ctx, ctx->processed_message, MAX_MESSAGE_LENGTH);
            return SPATIAL_SUCCESS;
        }
    }
    
    return SPATIAL_ERROR_BELOW_THRESHOLD;
}

/* Convenience function for processing multiple systems */
int spatial_process_all_systems(struct spatial_context *ctx, struct spatial_system **systems, int system_count) {
    int processed_count = 0;
    
    for (int i = 0; i < system_count; i++) {
        if (systems[i]->enabled) {
            ctx->active_system = systems[i];
            if (spatial_process_stimulus(ctx, systems[i]) == SPATIAL_SUCCESS) {
                processed_count++;
                /* Could deliver message here or queue for batch delivery */
            }
        }
    }
    
    return processed_count;
}
```

### **Example Usage Scenarios**

#### **Ship Passing - Visual System**
```c
void ship_passes_coast(int ship_x, int ship_y, char *ship_description) {
    struct spatial_context ctx = {0};
    
    ctx.source_x = ship_x;
    ctx.source_y = ship_y;
    ctx.source_z = 0;  /* Sea level */
    ctx.source_description = ship_description;
    ctx.base_intensity = 1.0;
    
    /* Process for all nearby players using visual system */
    struct spatial_system *systems[] = { &visual_system };
    
    for (struct char_data *ch = character_list; ch; ch = ch->next) {
        if (IS_NPC(ch) || !IN_WILDERNESS(ch)) continue;
        
        ctx.observer = ch;
        ctx.observer_x = GET_WILDERNESS_X(ch);
        ctx.observer_y = GET_WILDERNESS_Y(ch);
        ctx.observer_z = GET_WILDERNESS_Z(ch);
        
        if (spatial_process_all_systems(&ctx, systems, 1) > 0) {
            /* Deliver visual message to player */
            send_to_char(ch, "%s\r\n", ctx.processed_message);
        }
    }
}
```

#### **Empathy System Example (Future)**
```c
void empathic_event(struct char_data *source, char *emotion_description, float emotion_intensity) {
    struct spatial_context ctx = {0};
    
    ctx.source_x = GET_WILDERNESS_X(source);
    ctx.source_y = GET_WILDERNESS_Y(source);
    ctx.source_z = GET_WILDERNESS_Z(source);
    ctx.source_description = emotion_description;
    ctx.base_intensity = emotion_intensity;
    
    /* Get nearby entities that could interfere */
    ctx.nearby_entities = get_nearby_living_entities(source, EMPATHY_INTERFERENCE_RANGE);
    ctx.entity_count = count_nearby_entities(ctx.nearby_entities);
    
    struct spatial_system *systems[] = { &empathy_system };
    
    for (struct char_data *ch = character_list; ch; ch = ch->next) {
        if (IS_NPC(ch) || ch == source) continue;
        if (!HAS_EMPATHY_ABILITY(ch)) continue;  /* Only empaths can sense */
        
        ctx.observer = ch;
        ctx.observer_x = GET_WILDERNESS_X(ch);
        ctx.observer_y = GET_WILDERNESS_Y(ch);
        ctx.observer_z = GET_WILDERNESS_Z(ch);
        
        if (spatial_process_all_systems(&ctx, systems, 1) > 0) {
            send_to_char(ch, "%s\r\n", ctx.processed_message);
        }
    }
}
```

### **1. Wilderness Line of Sight System**
**Current System:** `src/wilderness.c` - `wilderness_can_see()`
**Enhancement Needed:** 
- Extract into reusable `spatial_line_of_sight.c` module
- Add granular terrain interference calculations
- Support 3D calculations (height/elevation)

### **2. Spatial Audio System** 
**Current System:** `src/pubsub_spatial.c`
**Refactoring Plan:**
- Extract common distance/interference calculations
- Create shared `spatial_core.c` module
- Implement strategy pattern for audio vs visual processing

### **3. PubSub Integration**
**Topics:**
- `visual_wilderness` - General visual events
- `visual_ships` - Maritime traffic
- `visual_flying` - Aerial objects
- `visual_weather` - Weather-related visual effects
- `visual_structures` - Moving structures (cities, caravans)

---

## 📋 **Implementation Phases**

### **Phase 1: Core Infrastructure (Week 1)**
1. **Spatial Core Module** (`src/spatial_core.c`)
   - Unified distance calculations
   - Terrain interference algorithms
   - Line of sight abstraction layer
   
2. **Visual Processor Module** (`src/spatial_visual.c`)
   - Visual-specific range calculations
   - Message generation for visual stimuli
   - Integration with line of sight system

3. **Enhanced Line of Sight** (`src/spatial_los.c`)
   - Refactored from wilderness system
   - 3D support with elevation
   - Granular terrain interference

### **Phase 2: PubSub Integration (Week 1)**
1. **Visual Event Publishers**
   - Ship movement events
   - Flying object tracking
   - Weather visual effects
   
2. **Auto-Subscription System**
   - Subscribe players to relevant visual topics based on location
   - Dynamic subscription management as players move
   - Efficient topic cleanup

3. **Message Delivery Optimization**
   - Batch processing for multiple observers
   - Priority-based delivery (important events first)
   - Rate limiting to prevent spam

### **Phase 3: Visual Event Types (Week 2)**
1. **Maritime Events**
   - Ships passing along coastlines
   - Naval battles visible from shore
   - Lighthouse beacons and signals
   
2. **Aerial Events**
   - Flying creatures/objects
   - Flying cities/structures
   - Weather phenomena (storms, aurora)
   
3. **Terrestrial Events**
   - Distant caravans
   - Smoke from settlements
   - Magical phenomena

### **Phase 4: Environmental Factors (Week 2)**
1. **Weather Integration**
   - Fog/mist reduces visibility
   - Rain affects visual clarity
   - Storms create dramatic visual effects
   
2. **Time of Day Effects**
   - Darkness limits vision range
   - Dawn/dusk lighting effects
   - Celestial events (meteor showers)
   
3. **Seasonal Variations**
   - Snow affects visibility
   - Vegetation changes impact line of sight

---

## 🔧 **Technical Specifications**

### **Distance Calculations**
```c
/* Enhanced distance calculation with 3D support */
float calculate_3d_distance(int x1, int y1, int z1, int x2, int y2, int z2) {
    return sqrt(pow(x2-x1, 2) + pow(y2-y1, 2) + pow(z2-z1, 2));
}

/* Visual range with environmental modifiers */
float calculate_visual_range(struct spatial_stimulus_context *ctx) {
    float base_range = ctx->base_range;
    float weather_mod = get_weather_visibility_modifier(ctx);
    float time_mod = get_time_visibility_modifier(ctx);
    float terrain_mod = get_terrain_visibility_modifier(ctx);
    
    return base_range * weather_mod * time_mod * terrain_mod;
}
```

### **Line of Sight Enhancement**
```c
/* Enhanced LOS with terrain interference levels */
typedef enum {
    LOS_CLEAR = 0,          /* Perfect line of sight */
    LOS_PARTIAL = 1,        /* Some obstruction (reduces clarity) */
    LOS_HEAVILY_OBSCURED = 2, /* Major obstruction (vague shapes only) */
    LOS_BLOCKED = 3         /* Complete obstruction */
} los_result_t;

los_result_t calculate_line_of_sight_detailed(
    int source_x, int source_y, int source_z,
    int target_x, int target_y, int target_z,
    float *interference_factor
);
```

### **Message Generation Strategy**
```c
/* Different message types based on visibility conditions */
typedef enum {
    VISUAL_MSG_CLEAR,       /* "You see a large ship sailing north along the coast" */
    VISUAL_MSG_DISTANT,     /* "You glimpse something moving on the distant horizon" */
    VISUAL_MSG_OBSCURED,    /* "Through the fog, you make out a dark shape moving" */
    VISUAL_MSG_SILHOUETTE   /* "A silhouette passes against the sunset sky" */
} visual_message_type_t;

int generate_visual_message(struct spatial_stimulus_context *ctx, 
                           visual_message_type_t msg_type,
                           char *output, size_t max_len);
```

---

## 🎮 **Game Integration Examples**

### **Ship Passing Example**
```c
/* When a ship moves in the wilderness */
void ship_movement_event(int ship_x, int ship_y, int direction, char *ship_desc) {
    struct spatial_stimulus_context ctx;
    
    /* Set up stimulus context */
    ctx.source_x = ship_x;
    ctx.source_y = ship_y;
    ctx.source_z = 0; /* Sea level */
    ctx.source_description = ship_desc;
    ctx.stimulus_type = STIMULUS_VISUAL;
    ctx.intensity = 1.0; /* Full visibility under good conditions */
    
    /* Process for all potential observers */
    spatial_process_visual_stimulus(&ctx, "visual_ships");
}
```

### **Flying Object Example**
```c
/* Dragon flying overhead */
void flying_object_event(int x, int y, int altitude, char *description) {
    struct spatial_stimulus_context ctx;
    
    ctx.source_x = x;
    ctx.source_y = y;
    ctx.source_z = altitude;
    ctx.source_description = description;
    ctx.stimulus_type = STIMULUS_VISUAL;
    ctx.intensity = 0.8; /* Slightly harder to see due to altitude */
    
    spatial_process_visual_stimulus(&ctx, "visual_flying");
}
```

---

## 🔄 **Auto-Subscription System**

### **Location-Based Subscriptions**
```c
/* When player moves in wilderness */
void update_wilderness_visual_subscriptions(struct char_data *ch, int new_x, int new_y) {
    /* Unsubscribe from irrelevant topics */
    if (!is_near_coast(new_x, new_y)) {
        pubsub_unsubscribe_if_subscribed(ch, "visual_ships");
    }
    
    /* Subscribe to relevant topics */
    if (is_near_coast(new_x, new_y)) {
        pubsub_auto_subscribe(ch, "visual_ships", "visual_stimulus_handler");
    }
    
    if (is_open_terrain(new_x, new_y)) {
        pubsub_auto_subscribe(ch, "visual_flying", "visual_stimulus_handler");
    }
    
    /* Always subscribe to general wilderness visuals */
    pubsub_auto_subscribe(ch, "visual_wilderness", "visual_stimulus_handler");
}
```

---

## 📊 **Performance Considerations**

### **Optimization Strategies**
1. **Spatial Indexing**
   - Grid-based partitioning for quick distance calculations
   - Only process stimuli for players within maximum range
   
2. **Caching**
   - Cache line of sight calculations for frequently used paths
   - Cache terrain interference factors
   
3. **Batch Processing**
   - Process multiple visual events in single pass
   - Group nearby observers for efficient delivery

### **Resource Management**
```c
/* Efficient observer finding */
struct observer_list {
    struct char_data **observers;
    int count;
    int capacity;
};

/* Pre-allocated context pools */
struct spatial_context_pool {
    struct spatial_stimulus_context *contexts;
    int pool_size;
    int next_available;
};
```

---

## 🧪 **Testing Strategy**

### **Unit Tests**
1. **Distance Calculations**
   - 2D and 3D distance accuracy
   - Edge cases (same location, extreme distances)
   
2. **Line of Sight**
   - Various terrain configurations
   - Elevation differences
   - Performance benchmarks

### **Integration Tests**
1. **Visual Event Flow**
   - Event publication → subscription → delivery
   - Auto-subscription behavior
   - Message generation accuracy

### **Performance Tests**
1. **Load Testing**
   - Multiple simultaneous visual events
   - Large numbers of observers
   - Memory usage patterns

---

## 📖 **Documentation Plan**

### **Developer Documentation**
1. **API Reference** - Function signatures and usage
2. **Integration Guide** - How to add new visual event types
3. **Performance Guide** - Optimization best practices

### **Design Documentation**
1. **Architecture Overview** - System design and relationships
2. **Algorithm Documentation** - Line of sight and distance calculations
3. **Extension Points** - How to add new stimulus types

---

## 🚀 **Deployment Strategy**

### **Rollout Plan**
1. **Phase 1 Deploy** - Core infrastructure (invisible to players)
2. **Phase 2 Deploy** - Basic ship visual events
3. **Phase 3 Deploy** - Full visual event suite
4. **Phase 4 Deploy** - Environmental effects and polish

### **Configuration**
```c
/* Runtime configuration options */
struct visual_system_config {
    bool enabled;
    float global_visual_range_multiplier;
    int max_visual_events_per_tick;
    bool debug_los_calculations;
    bool enable_weather_effects;
};
```

---

## 🔮 **Future Enhancements**

### **Advanced Features**
1. **Dynamic Weather Systems** - Real-time weather affecting visibility
2. **Magical Sight** - Spells that enhance or modify vision
3. **Racial Vision Differences** - Different races see different distances
4. **Interactive Visuals** - Players can interact with distant objects

### **Performance Improvements**
1. **GPU Acceleration** - Parallel processing for complex calculations
2. **Predictive Caching** - Pre-calculate common scenarios
3. **Adaptive Quality** - Reduce detail for distant/obscured objects

---

## ✅ **Success Criteria**

### **Functional Requirements**
- [ ] Players can see ships passing along coasts
- [ ] Flying objects are visible from appropriate distances
- [ ] Line of sight calculations are accurate and performant
- [ ] Auto-subscription system works seamlessly
- [ ] Weather and time affect visibility appropriately

### **Performance Requirements**
- [ ] System handles 100+ simultaneous visual events
- [ ] No noticeable lag when processing visual stimuli
- [ ] Memory usage remains within acceptable bounds
- [ ] Integration doesn't impact existing wilderness performance

### **Quality Requirements**
- [ ] Code is well-documented and maintainable
- [ ] System is easily extensible for new event types
- [ ] Error handling is robust and informative
- [ ] Testing coverage is comprehensive

---

*This plan provides a comprehensive roadmap for implementing a sophisticated spatial visual system that leverages the PubSub architecture while maintaining high performance and extensibility.*
