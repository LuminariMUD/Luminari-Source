# Wilderness Automatic Subscription Management System

**System:** LuminariMUD Publish/Subscribe Messaging System  
**Feature:** Automatic wilderness topic subscription based on player location  
**Purpose:** Implementation guide for location-aware messaging  
**Audience:** System developers, content creators  
**Last Updated:** August 13, 2025

---

## 🎯 **Overview**

This guide provides comprehensive solutions for automatically subscribing players to different PubSub topics as they move around the wilderness map. The system enables location-aware messaging, environmental immersion, and region-specific content delivery based on terrain, coordinates, elevation, and other wilderness attributes.

---

## 🏗️ **System Architecture**

### Core Components

#### 1. Movement Hook Integration
```c
/* Integration point in movement.c */
/* Located after char_to_room() call around line 812 */

int do_simple_move(struct char_data *ch, int dir, int need_specials_check) {
    /* ... existing movement code ... */
    
    /* The actual technical moving of the char */
    char_from_room(ch);
    
    X_LOC(ch) = new_x;
    Y_LOC(ch) = new_y;
    
    char_to_room(ch, going_to);
    /* end the actual technical moving of the char */
    
    /* ===== PUBSUB WILDERNESS SUBSCRIPTION HOOK ===== */
    /* Check if player moved to wilderness and update subscriptions */
    if (ZONE_FLAGGED(GET_ROOM_ZONE(going_to), ZONE_WILDERNESS)) {
        pubsub_wilderness_subscription_update(ch, going_to);
    }
    /* ================================================ */
    
    /* ... rest of movement code ... */
}
```

#### 2. Subscription Management Data Structures
```c
/* Wilderness subscription tracking structures */
struct wilderness_subscription_data {
    char *player_name;                      // Player identifier
    int current_x, current_y;               // Current coordinates
    int current_terrain_type;               // Current terrain (SECT_*)
    int current_region_id;                  // Current region identifier
    int current_elevation_band;             // Elevation category (0-4)
    char **active_topics;                   // Array of currently subscribed topics
    int topic_count;                        // Number of active subscriptions
    time_t last_update;                     // Last subscription update time
    struct wilderness_subscription_data *next;
};

/* Global wilderness subscription tracking */
static struct wilderness_subscription_data *wilderness_subscriptions = NULL;

/* Terrain-based topic categories */
#define TERRAIN_TOPIC_FOREST        "wilderness_forest"
#define TERRAIN_TOPIC_MOUNTAIN      "wilderness_mountain"
#define TERRAIN_TOPIC_PLAINS        "wilderness_plains"
#define TERRAIN_TOPIC_WATER         "wilderness_water"
#define TERRAIN_TOPIC_COASTAL       "wilderness_coastal"
#define TERRAIN_TOPIC_HILLS         "wilderness_hills"

/* Regional topic categories */
#define REGION_TOPIC_PREFIX         "region_"
#define WEATHER_TOPIC_PREFIX        "weather_"
#define ELEVATION_TOPIC_PREFIX      "elevation_"

/* Elevation bands for subscription management */
#define ELEVATION_BAND_DEEP_WATER   0  // < WATERLINE - 20
#define ELEVATION_BAND_SHALLOW      1  // WATERLINE - 20 to WATERLINE
#define ELEVATION_BAND_LOWLAND      2  // WATERLINE to PLAINS_THRESHOLD
#define ELEVATION_BAND_HIGHLAND     3  // PLAINS_THRESHOLD to MOUNTAIN_THRESHOLD
#define ELEVATION_BAND_MOUNTAIN     4  // > MOUNTAIN_THRESHOLD
```

---

## 🌍 **Implementation Solutions**

### Solution 1: Terrain-Based Automatic Subscriptions

#### Core Implementation
```c
/* Main wilderness subscription update function */
void pubsub_wilderness_subscription_update(struct char_data *ch, room_rnum room) {
    if (!ch || !IS_PC(ch) || room == NOWHERE) {
        return;
    }
    
    /* Skip if not in wilderness */
    if (!ZONE_FLAGGED(GET_ROOM_ZONE(room), ZONE_WILDERNESS)) {
        return;
    }
    
    int player_x = X_LOC(ch);
    int player_y = Y_LOC(ch);
    
    /* Get current wilderness attributes */
    int terrain_type = get_terrain_type_at_coordinates(player_x, player_y);
    int region_id = get_region_id(player_x, player_y);
    int elevation = get_modified_elevation(player_x, player_y);
    int elevation_band = calculate_elevation_band(elevation);
    
    /* Update player's wilderness subscription data */
    struct wilderness_subscription_data *ws_data = 
        get_or_create_wilderness_data(GET_NAME(ch));
    
    /* Check if player has moved to different terrain/region */
    bool terrain_changed = (ws_data->current_terrain_type != terrain_type);
    bool region_changed = (ws_data->current_region_id != region_id);
    bool elevation_changed = (ws_data->current_elevation_band != elevation_band);
    bool coordinates_changed = (ws_data->current_x != player_x || 
                               ws_data->current_y != player_y);
    
    if (terrain_changed || region_changed || elevation_changed || coordinates_changed) {
        /* Unsubscribe from old location-based topics */
        unsubscribe_old_wilderness_topics(ch, ws_data);
        
        /* Subscribe to new location-based topics */
        subscribe_new_wilderness_topics(ch, terrain_type, region_id, 
                                       elevation_band, player_x, player_y);
        
        /* Update tracking data */
        update_wilderness_subscription_data(ws_data, terrain_type, region_id,
                                           elevation_band, player_x, player_y);
        
        /* Optional: Notify player of environment change */
        send_wilderness_environment_message(ch, terrain_type, region_id, elevation_band);
    }
}

/* Terrain type determination function */
int get_terrain_type_at_coordinates(int x, int y) {
    int elevation = get_modified_elevation(x, y);
    
    /* Use existing LuminariMUD terrain thresholds */
    if (elevation < WATERLINE - SHALLOW_WATER_THRESHOLD) {
        return SECT_WATER_NOSWIM;  // Deep water
    } else if (elevation < WATERLINE) {
        return SECT_WATER_SWIM;    // Shallow water
    } else if (elevation < WATERLINE + COASTLINE_THRESHOLD) {
        return SECT_BEACH;         // Coastline
    } else if (elevation < PLAINS_THRESHOLD) {
        return SECT_FIELD;         // Plains
    } else if (elevation < HILL_THRESHOLD) {
        return SECT_HILLS;         // Hills
    } else if (elevation < MOUNTAIN_THRESHOLD) {
        return SECT_MOUNTAIN;      // Mountains
    } else {
        return SECT_FLYING;        // High peaks
    }
}

/* Calculate elevation band for subscription purposes */
int calculate_elevation_band(int elevation) {
    if (elevation < WATERLINE - SHALLOW_WATER_THRESHOLD) {
        return ELEVATION_BAND_DEEP_WATER;
    } else if (elevation < WATERLINE) {
        return ELEVATION_BAND_SHALLOW;
    } else if (elevation < PLAINS_THRESHOLD) {
        return ELEVATION_BAND_LOWLAND;
    } else if (elevation < MOUNTAIN_THRESHOLD) {
        return ELEVATION_BAND_HIGHLAND;
    } else {
        return ELEVATION_BAND_MOUNTAIN;
    }
}
```

#### Terrain-Specific Subscription Management
```c
/* Subscribe to terrain-based topics */
void subscribe_terrain_topics(struct char_data *ch, int terrain_type) {
    char *terrain_topic = NULL;
    char *ambient_topic = NULL;
    char *weather_topic = NULL;
    
    switch (terrain_type) {
        case SECT_FOREST:
            terrain_topic = TERRAIN_TOPIC_FOREST;
            ambient_topic = "forest_ambience";
            weather_topic = "forest_weather";
            
            /* Forest-specific subscriptions */
            pubsub_subscribe_player(ch, terrain_topic);
            pubsub_subscribe_player(ch, ambient_topic);
            pubsub_subscribe_player(ch, weather_topic);
            pubsub_subscribe_player(ch, "wildlife_sounds");
            pubsub_subscribe_player(ch, "druid_activities");
            break;
            
        case SECT_MOUNTAIN:
            terrain_topic = TERRAIN_TOPIC_MOUNTAIN;
            ambient_topic = "mountain_ambience";
            weather_topic = "mountain_weather";
            
            /* Mountain-specific subscriptions */
            pubsub_subscribe_player(ch, terrain_topic);
            pubsub_subscribe_player(ch, ambient_topic);
            pubsub_subscribe_player(ch, weather_topic);
            pubsub_subscribe_player(ch, "avalanche_warnings");
            pubsub_subscribe_player(ch, "mining_activities");
            pubsub_subscribe_player(ch, "high_altitude_effects");
            break;
            
        case SECT_WATER_SWIM:
        case SECT_WATER_NOSWIM:
            terrain_topic = TERRAIN_TOPIC_WATER;
            ambient_topic = "water_ambience";
            weather_topic = "marine_weather";
            
            /* Water-specific subscriptions */
            pubsub_subscribe_player(ch, terrain_topic);
            pubsub_subscribe_player(ch, ambient_topic);
            pubsub_subscribe_player(ch, weather_topic);
            pubsub_subscribe_player(ch, "sea_life_sounds");
            pubsub_subscribe_player(ch, "naval_activities");
            pubsub_subscribe_player(ch, "storm_warnings");
            break;
            
        case SECT_FIELD:
            terrain_topic = TERRAIN_TOPIC_PLAINS;
            ambient_topic = "plains_ambience";
            weather_topic = "plains_weather";
            
            /* Plains-specific subscriptions */
            pubsub_subscribe_player(ch, terrain_topic);
            pubsub_subscribe_player(ch, ambient_topic);
            pubsub_subscribe_player(ch, weather_topic);
            pubsub_subscribe_player(ch, "farming_activities");
            pubsub_subscribe_player(ch, "caravan_routes");
            break;
            
        case SECT_BEACH:
            terrain_topic = TERRAIN_TOPIC_COASTAL;
            ambient_topic = "coastal_ambience";
            weather_topic = "coastal_weather";
            
            /* Coastal-specific subscriptions */
            pubsub_subscribe_player(ch, terrain_topic);
            pubsub_subscribe_player(ch, ambient_topic);
            pubsub_subscribe_player(ch, weather_topic);
            pubsub_subscribe_player(ch, "tide_changes");
            pubsub_subscribe_player(ch, "port_activities");
            break;
            
        case SECT_HILLS:
            terrain_topic = TERRAIN_TOPIC_HILLS;
            ambient_topic = "hills_ambience";
            weather_topic = "hills_weather";
            
            /* Hills-specific subscriptions */
            pubsub_subscribe_player(ch, terrain_topic);
            pubsub_subscribe_player(ch, ambient_topic);
            pubsub_subscribe_player(ch, weather_topic);
            pubsub_subscribe_player(ch, "shepherd_activities");
            break;
    }
    
    /* Log subscription for debugging */
    pubsub_debug("Player %s subscribed to terrain type %d topics", 
                 GET_NAME(ch), terrain_type);
}

/* Unsubscribe from terrain-based topics */
void unsubscribe_terrain_topics(struct char_data *ch, int old_terrain_type) {
    /* Unsubscribe from previous terrain topics */
    switch (old_terrain_type) {
        case SECT_FOREST:
            pubsub_unsubscribe_player(ch, TERRAIN_TOPIC_FOREST);
            pubsub_unsubscribe_player(ch, "forest_ambience");
            pubsub_unsubscribe_player(ch, "forest_weather");
            pubsub_unsubscribe_player(ch, "wildlife_sounds");
            pubsub_unsubscribe_player(ch, "druid_activities");
            break;
            
        case SECT_MOUNTAIN:
            pubsub_unsubscribe_player(ch, TERRAIN_TOPIC_MOUNTAIN);
            pubsub_unsubscribe_player(ch, "mountain_ambience");
            pubsub_unsubscribe_player(ch, "mountain_weather");
            pubsub_unsubscribe_player(ch, "avalanche_warnings");
            pubsub_unsubscribe_player(ch, "mining_activities");
            pubsub_unsubscribe_player(ch, "high_altitude_effects");
            break;
            
        /* ... similar unsubscribe patterns for other terrain types ... */
    }
    
    pubsub_debug("Player %s unsubscribed from terrain type %d topics", 
                 GET_NAME(ch), old_terrain_type);
}
```

### Solution 2: Region-Based Subscription System

#### Regional Topic Management
```c
/* Region-based subscription management */
void subscribe_regional_topics(struct char_data *ch, int region_id, int x, int y) {
    char region_topic[256];
    char weather_topic[256];
    char events_topic[256];
    char resources_topic[256];
    
    /* Create region-specific topic names */
    snprintf(region_topic, sizeof(region_topic), "%s%d", REGION_TOPIC_PREFIX, region_id);
    snprintf(weather_topic, sizeof(weather_topic), "%s%s%d", 
             WEATHER_TOPIC_PREFIX, REGION_TOPIC_PREFIX, region_id);
    snprintf(events_topic, sizeof(events_topic), "%s%d_events", REGION_TOPIC_PREFIX, region_id);
    snprintf(resources_topic, sizeof(resources_topic), "%s%d_resources", REGION_TOPIC_PREFIX, region_id);
    
    /* Subscribe to region topics */
    pubsub_subscribe_player(ch, region_topic);
    pubsub_subscribe_player(ch, weather_topic);
    pubsub_subscribe_player(ch, events_topic);
    pubsub_subscribe_player(ch, resources_topic);
    
    /* Special regional features based on region properties */
    if (region_has_special_features(region_id)) {
        char special_topic[256];
        snprintf(special_topic, sizeof(special_topic), "%s%d_special", 
                REGION_TOPIC_PREFIX, region_id);
        pubsub_subscribe_player(ch, special_topic);
    }
    
    /* Proximity-based subscriptions for neighboring regions */
    subscribe_neighboring_region_topics(ch, region_id, x, y);
    
    pubsub_debug("Player %s subscribed to region %d topics", GET_NAME(ch), region_id);
}

/* Subscribe to neighboring region topics for border effects */
void subscribe_neighboring_region_topics(struct char_data *ch, int region_id, int x, int y) {
    /* Check for nearby regions within subscription range */
    int search_radius = 10;  // Coordinate radius for neighbor checking
    
    for (int dx = -search_radius; dx <= search_radius; dx += 5) {
        for (int dy = -search_radius; dy <= search_radius; dy += 5) {
            int check_x = x + dx;
            int check_y = y + dy;
            
            /* Skip center region (already subscribed) */
            if (dx == 0 && dy == 0) continue;
            
            int neighbor_region = get_region_id(check_x, check_y);
            
            if (neighbor_region != region_id && neighbor_region > 0) {
                char neighbor_topic[256];
                snprintf(neighbor_topic, sizeof(neighbor_topic), 
                        "%s%d_border_effects", REGION_TOPIC_PREFIX, neighbor_region);
                
                /* Subscribe with lower priority for neighbor effects */
                pubsub_subscribe_player_with_options(ch, neighbor_topic, 
                    "send_text", PUBSUB_PRIORITY_LOW, FALSE, 0);
            }
        }
    }
}

/* Check if region has special features that warrant additional topics */
bool region_has_special_features(int region_id) {
    /* This would check against region table or special region database */
    /* Example implementation checking region properties */
    
    /* Placeholder: Check if region is marked as having special features */
    return (region_id % 10 == 0);  // Every 10th region has special features
}
```

### Solution 3: Coordinate-Based Grid Subscription

#### Grid-Based Topic Management
```c
/* Coordinate grid-based subscription system */
#define GRID_SIZE 100  // 100x100 coordinate grid cells

/* Calculate grid cell from coordinates */
void get_grid_cell(int x, int y, int *grid_x, int *grid_y) {
    *grid_x = x / GRID_SIZE;
    *grid_y = y / GRID_SIZE;
}

/* Subscribe to grid-based topics */
void subscribe_grid_topics(struct char_data *ch, int x, int y) {
    int grid_x, grid_y;
    get_grid_cell(x, y, &grid_x, &grid_y);
    
    char grid_topic[256];
    char grid_events_topic[256];
    char grid_weather_topic[256];
    
    /* Create grid-specific topic names */
    snprintf(grid_topic, sizeof(grid_topic), "grid_%d_%d", grid_x, grid_y);
    snprintf(grid_events_topic, sizeof(grid_events_topic), "grid_%d_%d_events", grid_x, grid_y);
    snprintf(grid_weather_topic, sizeof(grid_weather_topic), "grid_%d_%d_weather", grid_x, grid_y);
    
    /* Subscribe to current grid topics */
    pubsub_subscribe_player(ch, grid_topic);
    pubsub_subscribe_player(ch, grid_events_topic);
    pubsub_subscribe_player(ch, grid_weather_topic);
    
    /* Subscribe to adjacent grid cells for seamless transitions */
    for (int dx = -1; dx <= 1; dx++) {
        for (int dy = -1; dy <= 1; dy++) {
            if (dx == 0 && dy == 0) continue;  // Skip center cell
            
            char adjacent_topic[256];
            snprintf(adjacent_topic, sizeof(adjacent_topic), 
                    "grid_%d_%d_border", grid_x + dx, grid_y + dy);
            
            /* Subscribe with low priority for adjacent cell effects */
            pubsub_subscribe_player_with_options(ch, adjacent_topic, 
                "send_text", PUBSUB_PRIORITY_LOW, FALSE, 0);
        }
    }
    
    pubsub_debug("Player %s subscribed to grid cell (%d, %d) topics", 
                 GET_NAME(ch), grid_x, grid_y);
}

/* Unsubscribe from grid-based topics */
void unsubscribe_grid_topics(struct char_data *ch, int old_x, int old_y) {
    int old_grid_x, old_grid_y;
    get_grid_cell(old_x, old_y, &old_grid_x, &old_grid_y);
    
    char grid_topic[256];
    char grid_events_topic[256];
    char grid_weather_topic[256];
    
    /* Unsubscribe from old grid topics */
    snprintf(grid_topic, sizeof(grid_topic), "grid_%d_%d", old_grid_x, old_grid_y);
    snprintf(grid_events_topic, sizeof(grid_events_topic), "grid_%d_%d_events", old_grid_x, old_grid_y);
    snprintf(grid_weather_topic, sizeof(grid_weather_topic), "grid_%d_%d_weather", old_grid_x, old_grid_y);
    
    pubsub_unsubscribe_player(ch, grid_topic);
    pubsub_unsubscribe_player(ch, grid_events_topic);
    pubsub_unsubscribe_player(ch, grid_weather_topic);
    
    /* Unsubscribe from adjacent grid cells */
    for (int dx = -1; dx <= 1; dx++) {
        for (int dy = -1; dy <= 1; dy++) {
            if (dx == 0 && dy == 0) continue;
            
            char adjacent_topic[256];
            snprintf(adjacent_topic, sizeof(adjacent_topic), 
                    "grid_%d_%d_border", old_grid_x + dx, old_grid_y + dy);
            
            pubsub_unsubscribe_player(ch, adjacent_topic);
        }
    }
    
    pubsub_debug("Player %s unsubscribed from grid cell (%d, %d) topics", 
                 GET_NAME(ch), old_grid_x, old_grid_y);
}
```

### Solution 4: Elevation-Based Subscription

#### Elevation Band Management
```c
/* Elevation-based topic subscription */
void subscribe_elevation_topics(struct char_data *ch, int elevation_band) {
    char elevation_topic[256];
    char altitude_effects_topic[256];
    
    switch (elevation_band) {
        case ELEVATION_BAND_DEEP_WATER:
            snprintf(elevation_topic, sizeof(elevation_topic), "%sdeep_water", ELEVATION_TOPIC_PREFIX);
            pubsub_subscribe_player(ch, elevation_topic);
            pubsub_subscribe_player(ch, "underwater_effects");
            pubsub_subscribe_player(ch, "deep_sea_events");
            pubsub_subscribe_player(ch, "pressure_effects");
            break;
            
        case ELEVATION_BAND_SHALLOW:
            snprintf(elevation_topic, sizeof(elevation_topic), "%sshallow_water", ELEVATION_TOPIC_PREFIX);
            pubsub_subscribe_player(ch, elevation_topic);
            pubsub_subscribe_player(ch, "shallow_water_effects");
            pubsub_subscribe_player(ch, "marine_life");
            break;
            
        case ELEVATION_BAND_LOWLAND:
            snprintf(elevation_topic, sizeof(elevation_topic), "%slowland", ELEVATION_TOPIC_PREFIX);
            pubsub_subscribe_player(ch, elevation_topic);
            pubsub_subscribe_player(ch, "lowland_weather");
            pubsub_subscribe_player(ch, "river_effects");
            break;
            
        case ELEVATION_BAND_HIGHLAND:
            snprintf(elevation_topic, sizeof(elevation_topic), "%shighland", ELEVATION_TOPIC_PREFIX);
            pubsub_subscribe_player(ch, elevation_topic);
            pubsub_subscribe_player(ch, "highland_weather");
            pubsub_subscribe_player(ch, "hill_effects");
            pubsub_subscribe_player(ch, "moderate_altitude");
            break;
            
        case ELEVATION_BAND_MOUNTAIN:
            snprintf(elevation_topic, sizeof(elevation_topic), "%smountain", ELEVATION_TOPIC_PREFIX);
            pubsub_subscribe_player(ch, elevation_topic);
            pubsub_subscribe_player(ch, "mountain_weather");
            pubsub_subscribe_player(ch, "high_altitude_effects");
            pubsub_subscribe_player(ch, "thin_air");
            pubsub_subscribe_player(ch, "mountain_dangers");
            break;
    }
    
    pubsub_debug("Player %s subscribed to elevation band %d topics", 
                 GET_NAME(ch), elevation_band);
}

/* Unsubscribe from elevation-based topics */
void unsubscribe_elevation_topics(struct char_data *ch, int old_elevation_band) {
    switch (old_elevation_band) {
        case ELEVATION_BAND_DEEP_WATER:
            pubsub_unsubscribe_player(ch, "elevation_deep_water");
            pubsub_unsubscribe_player(ch, "underwater_effects");
            pubsub_unsubscribe_player(ch, "deep_sea_events");
            pubsub_unsubscribe_player(ch, "pressure_effects");
            break;
            
        case ELEVATION_BAND_SHALLOW:
            pubsub_unsubscribe_player(ch, "elevation_shallow_water");
            pubsub_unsubscribe_player(ch, "shallow_water_effects");
            pubsub_unsubscribe_player(ch, "marine_life");
            break;
            
        /* ... similar patterns for other elevation bands ... */
    }
    
    pubsub_debug("Player %s unsubscribed from elevation band %d topics", 
                 GET_NAME(ch), old_elevation_band);
}
```

---

## 🔧 **Advanced Features**

### Solution 5: Dynamic Distance-Based Subscriptions

#### Proximity-Based Topic Management
```c
/* Distance-based subscription system for points of interest */
struct wilderness_poi {
    int x, y;                              // Point of interest coordinates
    char *topic_name;                      // Associated topic
    int subscription_radius;               // Auto-subscription radius
    int priority;                          // Topic priority
    bool active;                           // Whether POI is active
    struct wilderness_poi *next;
};

static struct wilderness_poi *wilderness_pois = NULL;

/* Check proximity to points of interest and manage subscriptions */
void check_poi_subscriptions(struct char_data *ch, int x, int y) {
    struct wilderness_poi *poi = wilderness_pois;
    
    while (poi) {
        if (!poi->active) {
            poi = poi->next;
            continue;
        }
        
        /* Calculate distance to POI */
        int distance = calculate_wilderness_distance(x, y, poi->x, poi->y);
        
        if (distance <= poi->subscription_radius) {
            /* Within range - ensure subscription */
            if (!is_player_subscribed(GET_NAME(ch), poi->topic_name)) {
                pubsub_subscribe_player_with_options(ch, poi->topic_name,
                    "send_text", poi->priority, FALSE, 0);
                
                pubsub_debug("Player %s auto-subscribed to POI topic %s (distance: %d)",
                           GET_NAME(ch), poi->topic_name, distance);
            }
        } else {
            /* Outside range - ensure unsubscription */
            if (is_player_subscribed(GET_NAME(ch), poi->topic_name)) {
                pubsub_unsubscribe_player(ch, poi->topic_name);
                
                pubsub_debug("Player %s auto-unsubscribed from POI topic %s (distance: %d)",
                           GET_NAME(ch), poi->topic_name, distance);
            }
        }
        
        poi = poi->next;
    }
}

/* Calculate wilderness distance between two points */
int calculate_wilderness_distance(int x1, int y1, int x2, int y2) {
    int dx = x2 - x1;
    int dy = y2 - y1;
    return (int)sqrt(dx * dx + dy * dy);
}

/* Add a point of interest for automatic subscription management */
void add_wilderness_poi(int x, int y, char *topic_name, int radius, int priority) {
    struct wilderness_poi *new_poi = calloc(1, sizeof(struct wilderness_poi));
    
    new_poi->x = x;
    new_poi->y = y;
    new_poi->topic_name = strdup(topic_name);
    new_poi->subscription_radius = radius;
    new_poi->priority = priority;
    new_poi->active = TRUE;
    
    /* Add to list */
    new_poi->next = wilderness_pois;
    wilderness_pois = new_poi;
    
    pubsub_info("Added wilderness POI: %s at (%d, %d) with radius %d",
               topic_name, x, y, radius);
}

/* Example POI initialization */
void initialize_wilderness_pois(void) {
    /* Major cities */
    add_wilderness_poi(500, 600, "city_luminar_events", 50, PUBSUB_PRIORITY_HIGH);
    add_wilderness_poi(300, 400, "city_palanthas_events", 50, PUBSUB_PRIORITY_HIGH);
    
    /* Dungeons and landmarks */
    add_wilderness_poi(100, 200, "dragon_lair_warnings", 30, PUBSUB_PRIORITY_URGENT);
    add_wilderness_poi(800, 900, "ancient_ruins_mysteries", 25, PUBSUB_PRIORITY_NORMAL);
    
    /* Resource locations */
    add_wilderness_poi(150, 350, "mining_area_activities", 20, PUBSUB_PRIORITY_NORMAL);
    add_wilderness_poi(700, 750, "herb_gathering_spots", 15, PUBSUB_PRIORITY_LOW);
    
    /* Weather phenomena locations */
    add_wilderness_poi(400, 500, "storm_center_warnings", 40, PUBSUB_PRIORITY_HIGH);
    add_wilderness_poi(650, 800, "magical_weather_zone", 30, PUBSUB_PRIORITY_NORMAL);
}
```

### Solution 6: Time-Based Subscription Management

#### Temporal Topic Control
```c
/* Time-based subscription management */
void update_time_based_subscriptions(struct char_data *ch) {
    time_t current_time = time(NULL);
    struct tm *time_info = localtime(&current_time);
    
    int hour = time_info->tm_hour;
    int season = get_current_season();  // Assuming MUD has season system
    
    /* Time-of-day subscriptions */
    unsubscribe_all_time_topics(ch);
    
    if (hour >= 6 && hour < 12) {
        /* Morning subscriptions */
        pubsub_subscribe_player(ch, "morning_activities");
        pubsub_subscribe_player(ch, "dawn_events");
        pubsub_subscribe_player(ch, "early_wildlife");
    } else if (hour >= 12 && hour < 18) {
        /* Afternoon subscriptions */
        pubsub_subscribe_player(ch, "afternoon_activities");
        pubsub_subscribe_player(ch, "midday_weather");
        pubsub_subscribe_player(ch, "trade_routes");
    } else if (hour >= 18 && hour < 22) {
        /* Evening subscriptions */
        pubsub_subscribe_player(ch, "evening_activities");
        pubsub_subscribe_player(ch, "sunset_events");
        pubsub_subscribe_player(ch, "tavern_activities");
    } else {
        /* Night subscriptions */
        pubsub_subscribe_player(ch, "night_activities");
        pubsub_subscribe_player(ch, "nocturnal_creatures");
        pubsub_subscribe_player(ch, "dark_mysteries");
        pubsub_subscribe_player(ch, "night_dangers");
    }
    
    /* Seasonal subscriptions */
    char seasonal_topic[256];
    snprintf(seasonal_topic, sizeof(seasonal_topic), "season_%d_events", season);
    pubsub_subscribe_player(ch, seasonal_topic);
    
    switch (season) {
        case SEASON_SPRING:
            pubsub_subscribe_player(ch, "spring_weather");
            pubsub_subscribe_player(ch, "plant_growth");
            pubsub_subscribe_player(ch, "mating_season");
            break;
            
        case SEASON_SUMMER:
            pubsub_subscribe_player(ch, "summer_weather");
            pubsub_subscribe_player(ch, "hot_weather_effects");
            pubsub_subscribe_player(ch, "harvest_activities");
            break;
            
        case SEASON_AUTUMN:
            pubsub_subscribe_player(ch, "autumn_weather");
            pubsub_subscribe_player(ch, "falling_leaves");
            pubsub_subscribe_player(ch, "harvest_festivals");
            break;
            
        case SEASON_WINTER:
            pubsub_subscribe_player(ch, "winter_weather");
            pubsub_subscribe_player(ch, "cold_weather_effects");
            pubsub_subscribe_player(ch, "winter_survival");
            break;
    }
}

/* Unsubscribe from all time-based topics */
void unsubscribe_all_time_topics(struct char_data *ch) {
    /* Time of day topics */
    pubsub_unsubscribe_player(ch, "morning_activities");
    pubsub_unsubscribe_player(ch, "dawn_events");
    pubsub_unsubscribe_player(ch, "early_wildlife");
    pubsub_unsubscribe_player(ch, "afternoon_activities");
    pubsub_unsubscribe_player(ch, "midday_weather");
    pubsub_unsubscribe_player(ch, "trade_routes");
    pubsub_unsubscribe_player(ch, "evening_activities");
    pubsub_unsubscribe_player(ch, "sunset_events");
    pubsub_unsubscribe_player(ch, "tavern_activities");
    pubsub_unsubscribe_player(ch, "night_activities");
    pubsub_unsubscribe_player(ch, "nocturnal_creatures");
    pubsub_unsubscribe_player(ch, "dark_mysteries");
    pubsub_unsubscribe_player(ch, "night_dangers");
    
    /* Seasonal topics */
    pubsub_unsubscribe_player(ch, "spring_weather");
    pubsub_unsubscribe_player(ch, "plant_growth");
    pubsub_unsubscribe_player(ch, "mating_season");
    pubsub_unsubscribe_player(ch, "summer_weather");
    pubsub_unsubscribe_player(ch, "hot_weather_effects");
    pubsub_unsubscribe_player(ch, "harvest_activities");
    pubsub_unsubscribe_player(ch, "autumn_weather");
    pubsub_unsubscribe_player(ch, "falling_leaves");
    pubsub_unsubscribe_player(ch, "harvest_festivals");
    pubsub_unsubscribe_player(ch, "winter_weather");
    pubsub_unsubscribe_player(ch, "cold_weather_effects");
    pubsub_unsubscribe_player(ch, "winter_survival");
}
```

---

## 📊 **Data Management Functions**

### Wilderness Subscription Data Management
```c
/* Get or create wilderness subscription data for player */
struct wilderness_subscription_data *get_or_create_wilderness_data(char *player_name) {
    struct wilderness_subscription_data *ws_data = wilderness_subscriptions;
    
    /* Search for existing data */
    while (ws_data) {
        if (!strcmp(ws_data->player_name, player_name)) {
            return ws_data;
        }
        ws_data = ws_data->next;
    }
    
    /* Create new data structure */
    ws_data = calloc(1, sizeof(struct wilderness_subscription_data));
    ws_data->player_name = strdup(player_name);
    ws_data->current_x = -1;
    ws_data->current_y = -1;
    ws_data->current_terrain_type = -1;
    ws_data->current_region_id = -1;
    ws_data->current_elevation_band = -1;
    ws_data->active_topics = NULL;
    ws_data->topic_count = 0;
    ws_data->last_update = time(NULL);
    
    /* Add to list */
    ws_data->next = wilderness_subscriptions;
    wilderness_subscriptions = ws_data;
    
    return ws_data;
}

/* Update wilderness subscription data */
void update_wilderness_subscription_data(struct wilderness_subscription_data *ws_data,
                                        int terrain_type, int region_id, 
                                        int elevation_band, int x, int y) {
    ws_data->current_x = x;
    ws_data->current_y = y;
    ws_data->current_terrain_type = terrain_type;
    ws_data->current_region_id = region_id;
    ws_data->current_elevation_band = elevation_band;
    ws_data->last_update = time(NULL);
}

/* Clean up wilderness subscription data for disconnected players */
void cleanup_wilderness_subscription_data(void) {
    struct wilderness_subscription_data *ws_data = wilderness_subscriptions;
    struct wilderness_subscription_data *prev = NULL;
    time_t current_time = time(NULL);
    
    while (ws_data) {
        /* Remove data for players offline for more than 1 hour */
        if (current_time - ws_data->last_update > 3600) {
            struct char_data *ch = get_player_by_name(ws_data->player_name);
            
            if (!ch || !IS_PLAYING(ch)) {
                /* Player is offline, clean up data */
                if (prev) {
                    prev->next = ws_data->next;
                } else {
                    wilderness_subscriptions = ws_data->next;
                }
                
                /* Free memory */
                free(ws_data->player_name);
                if (ws_data->active_topics) {
                    for (int i = 0; i < ws_data->topic_count; i++) {
                        free(ws_data->active_topics[i]);
                    }
                    free(ws_data->active_topics);
                }
                
                struct wilderness_subscription_data *to_free = ws_data;
                ws_data = ws_data->next;
                free(to_free);
                continue;
            }
        }
        
        prev = ws_data;
        ws_data = ws_data->next;
    }
}
```

---

## 🎮 **Integration Examples**

### Complete Integration Example
```c
/* Complete wilderness subscription update function */
void pubsub_wilderness_subscription_update(struct char_data *ch, room_rnum room) {
    if (!ch || !IS_PC(ch) || room == NOWHERE) {
        return;
    }
    
    /* Skip if not in wilderness */
    if (!ZONE_FLAGGED(GET_ROOM_ZONE(room), ZONE_WILDERNESS)) {
        /* If leaving wilderness, clean up wilderness subscriptions */
        cleanup_player_wilderness_subscriptions(ch);
        return;
    }
    
    int player_x = X_LOC(ch);
    int player_y = Y_LOC(ch);
    
    /* Get current wilderness attributes */
    int terrain_type = get_terrain_type_at_coordinates(player_x, player_y);
    int region_id = get_region_id(player_x, player_y);
    int elevation = get_modified_elevation(player_x, player_y);
    int elevation_band = calculate_elevation_band(elevation);
    
    /* Get or create player's wilderness subscription data */
    struct wilderness_subscription_data *ws_data = 
        get_or_create_wilderness_data(GET_NAME(ch));
    
    /* Check what has changed */
    bool terrain_changed = (ws_data->current_terrain_type != terrain_type);
    bool region_changed = (ws_data->current_region_id != region_id);
    bool elevation_changed = (ws_data->current_elevation_band != elevation_band);
    bool coordinates_changed = (ws_data->current_x != player_x || 
                               ws_data->current_y != player_y);
    bool significant_move = (abs(ws_data->current_x - player_x) > 5 || 
                            abs(ws_data->current_y - player_y) > 5);
    
    /* Update subscriptions if significant changes occurred */
    if (terrain_changed || region_changed || elevation_changed || 
        coordinates_changed || significant_move) {
        
        /* 1. Unsubscribe from old location-based topics */
        if (terrain_changed && ws_data->current_terrain_type != -1) {
            unsubscribe_terrain_topics(ch, ws_data->current_terrain_type);
        }
        
        if (region_changed && ws_data->current_region_id != -1) {
            unsubscribe_regional_topics(ch, ws_data->current_region_id);
        }
        
        if (elevation_changed && ws_data->current_elevation_band != -1) {
            unsubscribe_elevation_topics(ch, ws_data->current_elevation_band);
        }
        
        if (significant_move && ws_data->current_x != -1) {
            unsubscribe_grid_topics(ch, ws_data->current_x, ws_data->current_y);
        }
        
        /* 2. Subscribe to new location-based topics */
        subscribe_terrain_topics(ch, terrain_type);
        subscribe_regional_topics(ch, region_id, player_x, player_y);
        subscribe_elevation_topics(ch, elevation_band);
        subscribe_grid_topics(ch, player_x, player_y);
        
        /* 3. Check proximity-based subscriptions */
        check_poi_subscriptions(ch, player_x, player_y);
        
        /* 4. Update time-based subscriptions */
        update_time_based_subscriptions(ch);
        
        /* 5. Update tracking data */
        update_wilderness_subscription_data(ws_data, terrain_type, region_id,
                                           elevation_band, player_x, player_y);
        
        /* 6. Optional: Send environment notification */
        if (terrain_changed || region_changed || elevation_changed) {
            send_wilderness_environment_message(ch, terrain_type, region_id, elevation_band);
        }
        
        pubsub_debug("Updated wilderness subscriptions for %s at (%d, %d): "
                    "terrain=%d, region=%d, elevation_band=%d",
                    GET_NAME(ch), player_x, player_y, 
                    terrain_type, region_id, elevation_band);
    }
}

/* Send environment change notification to player */
void send_wilderness_environment_message(struct char_data *ch, int terrain_type, 
                                        int region_id, int elevation_band) {
    char env_message[MAX_STRING_LENGTH];
    char terrain_desc[256];
    char elevation_desc[256];
    
    /* Get terrain description */
    switch (terrain_type) {
        case SECT_FOREST:
            strcpy(terrain_desc, "dense forest");
            break;
        case SECT_MOUNTAIN:
            strcpy(terrain_desc, "rocky mountains");
            break;
        case SECT_FIELD:
            strcpy(terrain_desc, "open plains");
            break;
        case SECT_WATER_SWIM:
            strcpy(terrain_desc, "shallow waters");
            break;
        case SECT_WATER_NOSWIM:
            strcpy(terrain_desc, "deep waters");
            break;
        case SECT_BEACH:
            strcpy(terrain_desc, "sandy coastline");
            break;
        case SECT_HILLS:
            strcpy(terrain_desc, "rolling hills");
            break;
        default:
            strcpy(terrain_desc, "unfamiliar terrain");
            break;
    }
    
    /* Get elevation description */
    switch (elevation_band) {
        case ELEVATION_BAND_DEEP_WATER:
            strcpy(elevation_desc, "the depths");
            break;
        case ELEVATION_BAND_SHALLOW:
            strcpy(elevation_desc, "water level");
            break;
        case ELEVATION_BAND_LOWLAND:
            strcpy(elevation_desc, "low-lying areas");
            break;
        case ELEVATION_BAND_HIGHLAND:
            strcpy(elevation_desc, "elevated terrain");
            break;
        case ELEVATION_BAND_MOUNTAIN:
            strcpy(elevation_desc, "high altitude");
            break;
        default:
            strcpy(elevation_desc, "");
            break;
    }
    
    /* Compose environment message */
    if (strlen(elevation_desc) > 0) {
        snprintf(env_message, sizeof(env_message),
                "You find yourself in %s at %s. The environment feels different here.",
                terrain_desc, elevation_desc);
    } else {
        snprintf(env_message, sizeof(env_message),
                "You find yourself in %s. The environment feels different here.",
                terrain_desc);
    }
    
    send_to_char(ch, "%s\r\n", env_message);
    
    /* Publish environment change to wilderness topics */
    char env_topic[256];
    snprintf(env_topic, sizeof(env_topic), "player_environment_changes");
    
    char pub_message[MAX_STRING_LENGTH];
    snprintf(pub_message, sizeof(pub_message),
            "%s has entered %s in region %d",
            GET_NAME(ch), terrain_desc, region_id);
    
    pubsub_publish_message(env_topic, pub_message, 
                          PUBSUB_PRIORITY_LOW, "Environment System");
}
```

### Administrative Commands for Management

#### Debugging and Administration
```c
/* Administrative command for wilderness subscription management */
ACMD(do_wilderness_subscriptions) {
    char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
    
    two_arguments(argument, arg1, arg2);
    
    if (!*arg1) {
        send_to_char(ch, "Usage: wildersub <list|player|cleanup|poi|debug>\r\n");
        return;
    }
    
    if (!strcmp(arg1, "list")) {
        /* List all players with wilderness subscription data */
        struct wilderness_subscription_data *ws_data = wilderness_subscriptions;
        int count = 0;
        
        send_to_char(ch, "&WActive Wilderness Subscriptions:&n\r\n");
        send_to_char(ch, "&C========================================&n\r\n");
        
        while (ws_data) {
            send_to_char(ch, "&Y%s&n: (%d, %d) terrain=%d region=%d elevation=%d\r\n",
                        ws_data->player_name, ws_data->current_x, ws_data->current_y,
                        ws_data->current_terrain_type, ws_data->current_region_id,
                        ws_data->current_elevation_band);
            count++;
            ws_data = ws_data->next;
        }
        
        send_to_char(ch, "\r\nTotal: %d active wilderness subscriptions\r\n", count);
        
    } else if (!strcmp(arg1, "player") && *arg2) {
        /* Show detailed info for specific player */
        struct wilderness_subscription_data *ws_data = 
            get_or_create_wilderness_data(arg2);
            
        if (ws_data->current_x == -1) {
            send_to_char(ch, "No wilderness data found for player '%s'.\r\n", arg2);
            return;
        }
        
        send_to_char(ch, "&WDetailed Wilderness Data for %s:&n\r\n", arg2);
        send_to_char(ch, "&C=====================================&n\r\n");
        send_to_char(ch, "Coordinates: (%d, %d)\r\n", ws_data->current_x, ws_data->current_y);
        send_to_char(ch, "Terrain Type: %d\r\n", ws_data->current_terrain_type);
        send_to_char(ch, "Region ID: %d\r\n", ws_data->current_region_id);
        send_to_char(ch, "Elevation Band: %d\r\n", ws_data->current_elevation_band);
        send_to_char(ch, "Last Update: %ld seconds ago\r\n", 
                    time(NULL) - ws_data->last_update);
        
        /* Show active topics if available */
        if (ws_data->topic_count > 0) {
            send_to_char(ch, "Active Topics: %d\r\n", ws_data->topic_count);
            for (int i = 0; i < ws_data->topic_count; i++) {
                send_to_char(ch, "  - %s\r\n", ws_data->active_topics[i]);
            }
        }
        
    } else if (!strcmp(arg1, "cleanup")) {
        /* Force cleanup of old wilderness subscription data */
        int count_before = 0, count_after = 0;
        struct wilderness_subscription_data *ws_data = wilderness_subscriptions;
        
        /* Count before cleanup */
        while (ws_data) {
            count_before++;
            ws_data = ws_data->next;
        }
        
        cleanup_wilderness_subscription_data();
        
        /* Count after cleanup */
        ws_data = wilderness_subscriptions;
        while (ws_data) {
            count_after++;
            ws_data = ws_data->next;
        }
        
        send_to_char(ch, "Cleaned up %d old wilderness subscription records.\r\n",
                    count_before - count_after);
        
    } else if (!strcmp(arg1, "poi")) {
        /* List points of interest */
        struct wilderness_poi *poi = wilderness_pois;
        int count = 0;
        
        send_to_char(ch, "&WWilderness Points of Interest:&n\r\n");
        send_to_char(ch, "&C==================================&n\r\n");
        
        while (poi) {
            send_to_char(ch, "&Y%s&n: (%d, %d) radius=%d priority=%d %s\r\n",
                        poi->topic_name, poi->x, poi->y, 
                        poi->subscription_radius, poi->priority,
                        poi->active ? "&G[ACTIVE]&n" : "&R[INACTIVE]&n");
            count++;
            poi = poi->next;
        }
        
        send_to_char(ch, "\r\nTotal: %d wilderness POIs\r\n", count);
        
    } else if (!strcmp(arg1, "debug")) {
        /* Enable/disable debug mode */
        static bool debug_mode = FALSE;
        debug_mode = !debug_mode;
        
        send_to_char(ch, "Wilderness subscription debug mode: %s\r\n",
                    debug_mode ? "&GENABLED&n" : "&RDISABLED&n");
    }
}
```

---

## 🚀 **Performance Considerations**

### Optimization Strategies
```c
/* Optimize subscription updates to avoid spam */
void optimize_subscription_updates(struct char_data *ch) {
    /* Rate limiting for subscription updates */
    static time_t last_update_time[MAX_PLAYERS];
    static int player_update_count[MAX_PLAYERS];
    
    int player_idx = GET_PLAYER_INDEX(ch);
    time_t current_time = time(NULL);
    
    /* Reset counter every minute */
    if (current_time > last_update_time[player_idx] + 60) {
        player_update_count[player_idx] = 0;
        last_update_time[player_idx] = current_time;
    }
    
    /* Limit to 10 subscription updates per minute per player */
    if (player_update_count[player_idx] >= 10) {
        pubsub_debug("Rate limit exceeded for player %s wilderness subscriptions",
                    GET_NAME(ch));
        return;
    }
    
    player_update_count[player_idx]++;
}

/* Batch subscription operations for efficiency */
void batch_subscription_update(struct char_data *ch, char **topics_to_add, 
                              int add_count, char **topics_to_remove, int remove_count) {
    /* Remove old subscriptions first */
    for (int i = 0; i < remove_count; i++) {
        pubsub_unsubscribe_player(ch, topics_to_remove[i]);
    }
    
    /* Add new subscriptions */
    for (int i = 0; i < add_count; i++) {
        pubsub_subscribe_player(ch, topics_to_add[i]);
    }
    
    pubsub_debug("Batch updated subscriptions for %s: +%d -%d topics",
                GET_NAME(ch), add_count, remove_count);
}
```

---

*Comprehensive implementation guide for wilderness automatic subscription management*  
*Enabling immersive, location-aware messaging in LuminariMUD*
