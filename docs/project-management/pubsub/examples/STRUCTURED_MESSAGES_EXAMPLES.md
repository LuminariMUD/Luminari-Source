# Structured Messages Usage Examples
**Project:** LuminariMUD PubSub System Enhancement  
**Phase:** 3 - Advanced Features  
**Component:** Structured Messages Examples  
**Date:** August 13, 2025  
**Status:** 📚 DOCUMENTATION

---

## Overview

This document provides **practical examples** of how the structured messages system will be used in LuminariMUD, demonstrating the power and flexibility of the enhanced PubSub architecture.

---

## 🎮 **Game Event Examples**

### **Example 1: Quest Completion with Rich Metadata**

```c
/* Scenario: Player completes "The Dragon's Lair" quest */
void handle_quest_completion(struct char_data *ch, int quest_id) {
    /* Create structured message for quest completion */
    struct pubsub_message_v3 *msg = pubsub_create_message_v3(
        TOPIC_QUEST_NOTIFICATIONS,          /* Topic: Quest notifications */
        "QuestSystem",                      /* Sender: System component */
        0,                                  /* System sender (no player ID) */
        PUBSUB_MESSAGE_QUEST,              /* Type: Quest message */
        PUBSUB_CATEGORY_GAMEPLAY,          /* Category: Gameplay event */
        PUBSUB_PRIORITY_HIGH,              /* Priority: High (important event) */
        "The ancient dragon has been slain! The realm celebrates your victory!"
    );

    /* Add quest-specific custom fields */
    pubsub_message_add_field_int(msg, "quest_id", 1001);
    pubsub_message_add_field_string(msg, "quest_name", "The Dragon's Lair");
    pubsub_message_add_field_string(msg, "quest_chapter", "Dragon Slayer");
    pubsub_message_add_field_int(msg, "experience_reward", 50000);
    pubsub_message_add_field_int(msg, "gold_reward", 10000);
    pubsub_message_add_field_string(msg, "completion_time", "2h 35m");
    pubsub_message_add_field_int(msg, "difficulty_level", 8);
    pubsub_message_add_field_bool(msg, "first_completion", true);

    /* Add descriptive tags for filtering and search */
    pubsub_message_add_tag(msg, "quest-completion", PUBSUB_TAG_CATEGORY_EVENT, 9);
    pubsub_message_add_tag(msg, "dragon", PUBSUB_TAG_CATEGORY_CONTENT, 8);
    pubsub_message_add_tag(msg, "high-level", PUBSUB_TAG_CATEGORY_CONTENT, 7);
    pubsub_message_add_tag(msg, "epic-victory", PUBSUB_TAG_CATEGORY_EVENT, 9);
    pubsub_message_add_tag(msg, "first-time", PUBSUB_TAG_CATEGORY_ACHIEVEMENT, 6);

    /* Set rich metadata */
    struct pubsub_message_metadata_v3 *metadata = pubsub_create_metadata_v3();
    
    /* Character information */
    pubsub_metadata_set_sender_info(metadata,
        GET_NAME(ch),                       /* Real character name */
        GET_TITLE(ch),                      /* Character title */
        GET_LEVEL(ch),                      /* Character level */
        CLASS_ABBR(ch),                     /* Character class */
        race_list[GET_RACE(ch)].name        /* Character race */
    );
    
    /* Location information */
    pubsub_metadata_set_origin(metadata,
        GET_ROOM_VNUM(IN_ROOM(ch)),        /* Room number */
        zone_table[world[IN_ROOM(ch)].zone].number,  /* Zone */
        zone_table[world[IN_ROOM(ch)].zone].name,    /* Zone name */
        world[IN_ROOM(ch)].coords[0],      /* X coordinate */
        world[IN_ROOM(ch)].coords[1],      /* Y coordinate */
        world[IN_ROOM(ch)].coords[2]       /* Z coordinate */
    );
    
    /* Context information */
    pubsub_metadata_set_context(metadata,
        "QUEST_COMPLETION",                 /* Context type */
        "dragon_death_trigger",             /* Trigger event */
        "ancient_red_dragon",               /* Related object */
        dragon_mob_id                       /* Object ID */
    );

    /* Add custom metadata fields */
    pubsub_message_add_field_string(metadata->custom_fields, "victory_type", "combat");
    pubsub_message_add_field_int(metadata->custom_fields, "party_size", 1);
    pubsub_message_add_field_float(metadata->custom_fields, "completion_percentage", 100.0);

    /* Attach metadata to message */
    pubsub_message_set_metadata(msg, metadata);

    /* Set content formatting */
    pubsub_message_v3_set_content(msg,
        msg->content,                       /* Content already set */
        "text/formatted",                   /* Content type: formatted text */
        "utf-8"                            /* Encoding */
    );

    /* Publish the structured message */
    pubsub_publish_structured_message(msg);
    
    /* Message will be delivered to all subscribers of quest notifications
     * with rich context and searchable metadata */
}
```

### **Example 2: Weather System with Environmental Data**

```c
/* Scenario: Weather changes in the wilderness */
void weather_system_update(int zone_id, int weather_type, int intensity) {
    struct pubsub_message_v3 *msg = pubsub_create_message_v3(
        get_weather_topic_for_zone(zone_id),  /* Zone-specific weather topic */
        "WeatherSystem",                      /* System sender */
        0,                                    /* No player sender */
        PUBSUB_MESSAGE_WEATHER,              /* Weather message type */
        PUBSUB_CATEGORY_ENVIRONMENT,         /* Environmental category */
        PUBSUB_PRIORITY_NORMAL,              /* Normal priority */
        generate_weather_description(weather_type, intensity)
    );

    /* Weather-specific fields */
    pubsub_message_add_field_int(msg, "weather_type", weather_type);
    pubsub_message_add_field_int(msg, "intensity", intensity);
    pubsub_message_add_field_int(msg, "temperature", get_temperature(zone_id));
    pubsub_message_add_field_int(msg, "humidity", get_humidity(zone_id));
    pubsub_message_add_field_int(msg, "wind_speed", get_wind_speed(zone_id));
    pubsub_message_add_field_string(msg, "wind_direction", get_wind_direction(zone_id));
    pubsub_message_add_field_int(msg, "visibility", get_visibility(zone_id));
    pubsub_message_add_field_bool(msg, "affects_travel", weather_affects_travel(weather_type));
    pubsub_message_add_field_bool(msg, "affects_combat", weather_affects_combat(weather_type));

    /* Environmental tags */
    pubsub_message_add_tag(msg, "weather-change", PUBSUB_TAG_CATEGORY_EVENT, 5);
    pubsub_message_add_tag(msg, get_weather_tag(weather_type), PUBSUB_TAG_CATEGORY_CONTENT, 7);
    if (intensity > 7) {
        pubsub_message_add_tag(msg, "severe-weather", PUBSUB_TAG_CATEGORY_PRIORITY, 8);
    }
    
    /* Zone and location tags */
    char zone_tag[64];
    snprintf(zone_tag, sizeof(zone_tag), "zone-%d", zone_id);
    pubsub_message_add_tag(msg, zone_tag, PUBSUB_TAG_CATEGORY_LOCATION, 6);

    /* Metadata for weather events */
    struct pubsub_message_metadata_v3 *metadata = pubsub_create_metadata_v3();
    metadata->origin_zone = zone_id;
    metadata->origin_area_name = strdup(zone_table[zone_id].name);
    metadata->context_type = strdup("WEATHER_UPDATE");
    metadata->trigger_event = strdup("scheduled_weather_check");
    
    /* Add weather-specific metadata */
    pubsub_message_add_field_string(metadata->custom_fields, "weather_pattern", 
                                   get_weather_pattern_name(zone_id));
    pubsub_message_add_field_int(metadata->custom_fields, "seasonal_modifier", 
                                get_seasonal_modifier());
    
    pubsub_message_set_metadata(msg, metadata);

    /* Publish weather update */
    pubsub_publish_structured_message(msg);
}
```

### **Example 3: Combat Event with Spatial Audio**

```c
/* Scenario: Combat action in wilderness with spatial audio */
void combat_action_with_spatial(struct char_data *attacker, struct char_data *victim, 
                               int damage, int attack_type) {
    struct pubsub_message_v3 *msg = pubsub_create_message_v3(
        TOPIC_WILDERNESS_COMBAT,            /* Wilderness combat topic */
        GET_NAME(attacker),                 /* Attacker as sender */
        GET_IDNUM(attacker),               /* Attacker's player ID */
        PUBSUB_MESSAGE_COMBAT,             /* Combat message type */
        PUBSUB_CATEGORY_GAMEPLAY,          /* Gameplay category */
        PUBSUB_PRIORITY_HIGH,              /* High priority for combat */
        generate_combat_message(attacker, victim, damage, attack_type)
    );

    /* Combat-specific fields */
    pubsub_message_add_field_string(msg, "attacker", GET_NAME(attacker));
    pubsub_message_add_field_string(msg, "victim", GET_NAME(victim));
    pubsub_message_add_field_int(msg, "damage", damage);
    pubsub_message_add_field_int(msg, "attack_type", attack_type);
    pubsub_message_add_field_string(msg, "weapon", GET_OBJ_SHORT(GET_EQ(attacker, WEAR_WIELD)));
    pubsub_message_add_field_int(msg, "attacker_hp", GET_HIT(attacker));
    pubsub_message_add_field_int(msg, "victim_hp", GET_HIT(victim));
    pubsub_message_add_field_bool(msg, "critical_hit", is_critical_hit(damage, attack_type));
    pubsub_message_add_field_bool(msg, "killing_blow", GET_HIT(victim) <= 0);

    /* Combat tags */
    pubsub_message_add_tag(msg, "combat-action", PUBSUB_TAG_CATEGORY_EVENT, 7);
    pubsub_message_add_tag(msg, get_attack_type_tag(attack_type), PUBSUB_TAG_CATEGORY_CONTENT, 6);
    if (damage > 100) {
        pubsub_message_add_tag(msg, "high-damage", PUBSUB_TAG_CATEGORY_PRIORITY, 7);
    }
    if (GET_HIT(victim) <= 0) {
        pubsub_message_add_tag(msg, "death-blow", PUBSUB_TAG_CATEGORY_EVENT, 9);
    }

    /* Spatial audio data for wilderness combat */
    if (ROOM_FLAGGED(IN_ROOM(attacker), ROOM_WILDERNESS)) {
        /* Add spatial positioning for audio */
        char spatial_data[256];
        snprintf(spatial_data, sizeof(spatial_data),
                "x=%d,y=%d,z=%d,range=25,type=combat",
                world[IN_ROOM(attacker)].coords[0],
                world[IN_ROOM(attacker)].coords[1],
                world[IN_ROOM(attacker)].coords[2]);
        
        if (msg->spatial_data) free(msg->spatial_data);
        msg->spatial_data = strdup(spatial_data);
        
        /* Add spatial-specific fields */
        pubsub_message_add_field_int(msg, "audio_range", 25);
        pubsub_message_add_field_string(msg, "audio_type", "combat_clash");
        pubsub_message_add_field_int(msg, "volume_level", calculate_volume_for_damage(damage));
    }

    /* Rich metadata for combat */
    struct pubsub_message_metadata_v3 *metadata = pubsub_create_metadata_v3();
    
    /* Attacker metadata */
    pubsub_metadata_set_sender_info(metadata,
        GET_NAME(attacker),
        GET_TITLE(attacker),
        GET_LEVEL(attacker),
        CLASS_ABBR(attacker),
        race_list[GET_RACE(attacker)].name
    );
    
    /* Combat location */
    pubsub_metadata_set_origin(metadata,
        GET_ROOM_VNUM(IN_ROOM(attacker)),
        zone_table[world[IN_ROOM(attacker)].zone].number,
        zone_table[world[IN_ROOM(attacker)].zone].name,
        world[IN_ROOM(attacker)].coords[0],
        world[IN_ROOM(attacker)].coords[1],
        world[IN_ROOM(attacker)].coords[2]
    );
    
    /* Combat context */
    pubsub_metadata_set_context(metadata,
        "COMBAT_ACTION",
        "player_attack",
        GET_NAME(victim),
        GET_IDNUM(victim)
    );

    /* Combat-specific metadata */
    pubsub_message_add_field_string(metadata->custom_fields, "combat_round", 
                                   itoa(get_combat_round(attacker)));
    pubsub_message_add_field_string(metadata->custom_fields, "terrain_type",
                                   get_terrain_type_name(IN_ROOM(attacker)));
    
    pubsub_message_set_metadata(msg, metadata);

    /* Publish combat event */
    pubsub_publish_structured_message(msg);
}
```

---

## 🔍 **Advanced Filtering Examples**

### **Example 4: Guild Leader's Combat Dashboard**

```c
/* Scenario: Guild leader wants to monitor guild members in combat */
void setup_guild_combat_filter(struct char_data *guild_leader) {
    struct pubsub_message_filter_v3 *filter = malloc(sizeof(struct pubsub_message_filter_v3));
    
    /* Initialize filter */
    memset(filter, 0, sizeof(struct pubsub_message_filter_v3));
    
    /* Filter for combat messages only */
    filter->allowed_types = malloc(sizeof(int) * 1);
    filter->allowed_types[0] = PUBSUB_MESSAGE_COMBAT;
    filter->allowed_type_count = 1;
    
    /* High and urgent priority only */
    filter->min_priority = PUBSUB_PRIORITY_HIGH;
    filter->max_priority = PUBSUB_PRIORITY_CRITICAL;
    
    /* Must have combat-related tags */
    filter->required_tags = malloc(sizeof(char*) * 2);
    filter->required_tags[0] = strdup("combat-action");
    filter->required_tags[1] = strdup("guild-member");
    filter->required_tag_count = 2;
    
    /* Exclude minor combat actions */
    filter->excluded_tags = malloc(sizeof(char*) * 1);
    filter->excluded_tags[0] = strdup("minor-damage");
    filter->excluded_tag_count = 1;
    
    /* Required fields for guild combat monitoring */
    filter->required_fields = malloc(sizeof(char*) * 3);
    filter->required_fields[0] = strdup("damage");
    filter->required_fields[1] = strdup("attacker");
    filter->required_fields[2] = strdup("victim");
    filter->required_field_count = 3;
    
    /* Only recent messages (last 10 minutes) */
    filter->created_after = time(NULL) - 600;
    
    /* Guild leader's level or higher */
    filter->min_sender_level = GET_LEVEL(guild_leader);
    
    /* Register this filter for the guild leader */
    pubsub_register_player_filter(guild_leader, "guild_combat_monitor", filter);
}
```

### **Example 5: Weather Alert System for Travelers**

```c
/* Scenario: Player wants alerts for severe weather affecting travel */
void setup_weather_travel_alerts(struct char_data *ch) {
    struct pubsub_message_filter_v3 *filter = malloc(sizeof(struct pubsub_message_filter_v3));
    memset(filter, 0, sizeof(struct pubsub_message_filter_v3));
    
    /* Weather messages only */
    filter->allowed_types = malloc(sizeof(int) * 1);
    filter->allowed_types[0] = PUBSUB_MESSAGE_WEATHER;
    filter->allowed_type_count = 1;
    
    /* Environmental category */
    filter->allowed_categories = malloc(sizeof(int) * 1);
    filter->allowed_categories[0] = PUBSUB_CATEGORY_ENVIRONMENT;
    filter->allowed_category_count = 1;
    
    /* Must have weather tags that affect travel */
    filter->required_tags = malloc(sizeof(char*) * 1);
    filter->required_tags[0] = strdup("affects-travel");
    filter->required_tag_count = 1;
    
    /* Must have affects_travel field set to true */
    filter->required_fields = malloc(sizeof(char*) * 1);
    filter->required_fields[0] = strdup("affects_travel");
    filter->required_field_count = 1;
    
    /* Urgent weather only */
    filter->min_priority = PUBSUB_PRIORITY_URGENT;
    
    /* Register weather travel filter */
    pubsub_register_player_filter(ch, "weather_travel_alerts", filter);
}
```

---

## 🔧 **Custom Field Usage Examples**

### **Example 6: Economic Trading System**

```c
/* Scenario: Market transaction notification */
void market_transaction_notification(struct char_data *buyer, struct char_data *seller,
                                   struct obj_data *item, int price) {
    struct pubsub_message_v3 *msg = pubsub_create_message_v3(
        TOPIC_MARKET_TRANSACTIONS,
        GET_NAME(seller),
        GET_IDNUM(seller),
        PUBSUB_MESSAGE_ECONOMY,
        PUBSUB_CATEGORY_ECONOMY,
        PUBSUB_PRIORITY_NORMAL,
        "A market transaction has been completed."
    );

    /* Transaction fields */
    pubsub_message_add_field_string(msg, "buyer_name", GET_NAME(buyer));
    pubsub_message_add_field_int(msg, "buyer_id", GET_IDNUM(buyer));
    pubsub_message_add_field_string(msg, "seller_name", GET_NAME(seller));
    pubsub_message_add_field_int(msg, "seller_id", GET_IDNUM(seller));
    pubsub_message_add_field_string(msg, "item_name", item->short_description);
    pubsub_message_add_field_int(msg, "item_vnum", GET_OBJ_VNUM(item));
    pubsub_message_add_field_int(msg, "price", price);
    pubsub_message_add_field_string(msg, "currency", "gold");
    
    /* Item properties */
    pubsub_message_add_field_int(msg, "item_type", GET_OBJ_TYPE(item));
    pubsub_message_add_field_int(msg, "item_level", GET_OBJ_LEVEL(item));
    pubsub_message_add_field_bool(msg, "item_magical", OBJ_FLAGGED(item, ITEM_MAGIC));
    pubsub_message_add_field_int(msg, "item_weight", GET_OBJ_WEIGHT(item));
    pubsub_message_add_field_int(msg, "item_value", GET_OBJ_COST(item));
    
    /* Market analysis fields */
    pubsub_message_add_field_float(msg, "price_ratio", (float)price / GET_OBJ_COST(item));
    pubsub_message_add_field_bool(msg, "good_deal", price < GET_OBJ_COST(item) * 0.8);
    pubsub_message_add_field_string(msg, "market_trend", get_market_trend_for_item(item));
    
    /* Economic tags */
    pubsub_message_add_tag(msg, "market-transaction", PUBSUB_TAG_CATEGORY_EVENT, 6);
    pubsub_message_add_tag(msg, get_item_category_tag(item), PUBSUB_TAG_CATEGORY_CONTENT, 5);
    if (price > 10000) {
        pubsub_message_add_tag(msg, "high-value", PUBSUB_TAG_CATEGORY_PRIORITY, 7);
    }

    /* Publish transaction */
    pubsub_publish_structured_message(msg);
}
```

### **Example 7: Social Event with Player Relationships**

```c
/* Scenario: Player marriage ceremony */
void marriage_ceremony_event(struct char_data *spouse1, struct char_data *spouse2,
                           struct char_data *officiant, room_rnum ceremony_room) {
    struct pubsub_message_v3 *msg = pubsub_create_message_v3(
        TOPIC_SOCIAL_EVENTS,
        "CeremonySystem",
        0,
        PUBSUB_MESSAGE_SOCIAL,
        PUBSUB_CATEGORY_SOCIAL,
        PUBSUB_PRIORITY_HIGH,
        "A marriage ceremony is taking place in the realm!"
    );

    /* Marriage ceremony fields */
    pubsub_message_add_field_string(msg, "spouse1_name", GET_NAME(spouse1));
    pubsub_message_add_field_int(msg, "spouse1_id", GET_IDNUM(spouse1));
    pubsub_message_add_field_string(msg, "spouse2_name", GET_NAME(spouse2));
    pubsub_message_add_field_int(msg, "spouse2_id", GET_IDNUM(spouse2));
    pubsub_message_add_field_string(msg, "officiant_name", GET_NAME(officiant));
    pubsub_message_add_field_int(msg, "officiant_id", GET_IDNUM(officiant));
    pubsub_message_add_field_string(msg, "ceremony_location", world[ceremony_room].name);
    pubsub_message_add_field_int(msg, "ceremony_room", world[ceremony_room].number);
    
    /* Relationship fields */
    pubsub_message_add_field_string(msg, "relationship_type", "marriage");
    pubsub_message_add_field_bool(msg, "first_marriage_spouse1", 
                                 is_first_marriage(spouse1));
    pubsub_message_add_field_bool(msg, "first_marriage_spouse2", 
                                 is_first_marriage(spouse2));
    pubsub_message_add_field_int(msg, "relationship_duration_days", 
                                get_courtship_duration(spouse1, spouse2));
    
    /* Ceremony details */
    pubsub_message_add_field_string(msg, "ceremony_style", "traditional");
    pubsub_message_add_field_int(msg, "guest_count", count_players_in_room(ceremony_room));
    pubsub_message_add_field_bool(msg, "public_ceremony", 
                                 ROOM_FLAGGED(ceremony_room, ROOM_PUBLIC));

    /* Social tags */
    pubsub_message_add_tag(msg, "marriage", PUBSUB_TAG_CATEGORY_EVENT, 9);
    pubsub_message_add_tag(msg, "ceremony", PUBSUB_TAG_CATEGORY_SOCIAL, 8);
    pubsub_message_add_tag(msg, "celebration", PUBSUB_TAG_CATEGORY_EVENT, 7);
    pubsub_message_add_tag(msg, "relationship", PUBSUB_TAG_CATEGORY_SOCIAL, 6);

    /* Publish marriage event */
    pubsub_publish_structured_message(msg);
}
```

---

## 📊 **Message Threading Examples**

### **Example 8: Quest Dialog System**

```c
/* Scenario: Multi-part quest conversation */
void quest_dialog_system(struct char_data *ch, struct char_data *npc, 
                        const char *dialog_text, int dialog_stage) {
    static int conversation_thread_id = 0;
    static int message_sequence = 1;
    
    /* Start new conversation thread if this is the first message */
    if (dialog_stage == 1) {
        conversation_thread_id = generate_thread_id();
        message_sequence = 1;
    }

    struct pubsub_message_v3 *msg = pubsub_create_message_v3(
        get_quest_dialog_topic(ch),
        GET_NAME(npc),
        GET_MOB_VNUM(npc),
        PUBSUB_MESSAGE_QUEST,
        PUBSUB_CATEGORY_SOCIAL,
        PUBSUB_PRIORITY_NORMAL,
        dialog_text
    );

    /* Dialog-specific fields */
    pubsub_message_add_field_string(msg, "speaker", GET_NAME(npc));
    pubsub_message_add_field_string(msg, "listener", GET_NAME(ch));
    pubsub_message_add_field_int(msg, "dialog_stage", dialog_stage);
    pubsub_message_add_field_string(msg, "dialog_type", "quest_conversation");
    pubsub_message_add_field_bool(msg, "requires_response", true);
    pubsub_message_add_field_int(msg, "timeout_seconds", 300);

    /* Set up message threading */
    if (message_sequence > 1) {
        /* Find the previous message ID for parent reference */
        int previous_message_id = get_last_message_in_thread(conversation_thread_id);
        pubsub_message_v3_set_parent(msg, previous_message_id, conversation_thread_id);
    } else {
        /* First message in thread */
        pubsub_message_v3_set_parent(msg, 0, conversation_thread_id);
    }
    
    msg->sequence_number = message_sequence++;

    /* Dialog tags */
    pubsub_message_add_tag(msg, "quest-dialog", PUBSUB_TAG_CATEGORY_EVENT, 6);
    pubsub_message_add_tag(msg, "conversation", PUBSUB_TAG_CATEGORY_SOCIAL, 5);
    pubsub_message_add_tag(msg, "npc-interaction", PUBSUB_TAG_CATEGORY_EVENT, 5);

    /* Publish dialog message */
    pubsub_publish_structured_message(msg);
}
```

---

## 🎵 **JSON Export/Import Examples**

### **Example 9: Message Archive and Analysis**

```c
/* Scenario: Export combat statistics for analysis */
void export_combat_statistics(time_t start_time, time_t end_time, const char *filename) {
    /* Create filter for combat messages in time range */
    struct pubsub_message_filter_v3 *filter = malloc(sizeof(struct pubsub_message_filter_v3));
    memset(filter, 0, sizeof(struct pubsub_message_filter_v3));
    
    filter->allowed_types = malloc(sizeof(int) * 1);
    filter->allowed_types[0] = PUBSUB_MESSAGE_COMBAT;
    filter->allowed_type_count = 1;
    
    filter->created_after = start_time;
    filter->created_before = end_time;
    
    /* Get all combat messages in range */
    struct pubsub_message_v3 **combat_messages;
    int message_count = pubsub_get_filtered_messages(filter, &combat_messages);
    
    /* Export to JSON file for analysis */
    pubsub_export_messages_to_file(combat_messages, message_count, filename);
    
    /* The exported JSON will include all structured data:
     * {
     *   "messages": [
     *     {
     *       "message_id": 12345,
     *       "topic_id": 100,
     *       "sender_name": "Thorin",
     *       "message_type": 7,
     *       "message_category": 3,
     *       "priority": 3,
     *       "content": "Thorin strikes the goblin with devastating force!",
     *       "fields": {
     *         "damage": 85,
     *         "attacker": "Thorin",
     *         "victim": "goblin_warrior",
     *         "weapon": "a gleaming silver sword",
     *         "critical_hit": true
     *       },
     *       "tags": ["combat-action", "high-damage", "critical-hit"],
     *       "metadata": {
     *         "sender_level": 25,
     *         "sender_class": "fighter",
     *         "origin_room": 3001,
     *         "origin_zone": 30
     *       },
     *       "created_at": "2025-08-13T14:30:15Z"
     *     }
     *   ]
     * }
     */
    
    free(combat_messages);
    free(filter);
}
```

---

## 🔧 **Performance Optimization Examples**

### **Example 10: Efficient Message Pooling**

```c
/* Scenario: High-frequency weather updates with memory pooling */
void optimized_weather_broadcast(void) {
    /* Use message pool for efficient allocation */
    struct pubsub_message_v3 *msg = pubsub_pool_acquire_message_v3();
    
    /* Initialize with weather data */
    pubsub_message_v3_init(msg,
        TOPIC_WEATHER_GLOBAL,
        "WeatherSystem",
        0,
        PUBSUB_MESSAGE_WEATHER,
        PUBSUB_CATEGORY_ENVIRONMENT,
        PUBSUB_PRIORITY_LOW,
        "The weather is changing across the realm."
    );

    /* Add weather fields efficiently */
    pubsub_message_add_field_int(msg, "global_temperature", get_global_temperature());
    pubsub_message_add_field_string(msg, "dominant_pattern", get_dominant_weather_pattern());
    
    /* Minimal tagging for performance */
    pubsub_message_add_tag(msg, "weather-update", PUBSUB_TAG_CATEGORY_EVENT, 3);
    
    /* Use cached metadata for weather system */
    static struct pubsub_message_metadata_v3 *weather_metadata = NULL;
    if (!weather_metadata) {
        weather_metadata = create_weather_system_metadata();
    }
    pubsub_message_set_metadata(msg, weather_metadata);

    /* Publish optimized message */
    pubsub_publish_structured_message(msg);
    
    /* Message automatically returned to pool after delivery */
}
```

---

## 📈 **Summary**

These examples demonstrate the power and flexibility of the structured messages system:

### **Key Benefits Shown:**

1. **Rich Data Structure**: Custom fields allow detailed game event tracking
2. **Advanced Filtering**: Players can create sophisticated filters for relevant content
3. **Metadata Context**: Rich context information for better understanding
4. **Message Threading**: Support for conversations and multi-part interactions
5. **Export/Analysis**: JSON support enables data analysis and archiving
6. **Performance**: Memory pooling and optimization for high-frequency events

### **Use Cases Covered:**

- ✅ **Quest System**: Completion tracking with rewards and context
- ✅ **Weather System**: Environmental data with travel impact
- ✅ **Combat System**: Detailed combat events with spatial audio
- ✅ **Economic System**: Market transactions with trend analysis
- ✅ **Social System**: Relationship events and ceremonies
- ✅ **Dialog System**: Multi-part conversations with threading
- ✅ **Administrative**: Message filtering and monitoring tools
- ✅ **Analytics**: Data export for analysis and reporting

This structured approach transforms the simple PubSub system into a comprehensive communication and event tracking platform that can support complex game mechanics while maintaining excellent performance.

---

**Status**: 📚 **DOCUMENTATION COMPLETE**
