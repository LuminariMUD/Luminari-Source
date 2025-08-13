# PubSub Event-Driven Architecture Guide

## Overview

The PubSub system in LuminariMUD provides a powerful event-driven architecture that allows code modules to communicate with each other through events without tight coupling. This document explains how to implement and use event-driven patterns.

## Core Concepts

### 1. Event Publishers
Any part of your code can **publish events** when something interesting happens:
- Combat actions (hits, misses, critical strikes)
- Player state changes (level up, death, login)
- Environmental changes (weather, resource depletion)
- System events (server startup, shutdown)

### 2. Event Handlers
**Event handlers** are C functions that automatically respond to events:
- Logging systems
- Game mechanics processors
- Database updaters
- Notification systems

### 3. Automatic Decoupling
The PubSub system acts as a **message broker** between publishers and handlers:
- Publishers don't need to know who will handle their events
- Handlers don't need to know where events come from
- You can add/remove handlers without changing publishers

## Implementation Patterns

### Pattern 1: Simple Event Triggering

```c
/* In your combat system (fight.c) */
void hit(struct char_data *ch, struct char_data *victim, int damage) {
    /* ... existing combat code ... */
    
    /* Publish combat event - no need to know who handles it */
    char event_data[MAX_STRING_LENGTH];
    snprintf(event_data, sizeof(event_data),
            "Combat: %s attacks %s for %d damage",
            GET_NAME(ch), GET_NAME(victim), damage);
    
    pubsub_trigger_event("events.combat.action", ch, event_data, PUBSUB_PRIORITY_LOW);
    
    /* Critical hit? */
    if (damage > GET_MAX_HIT(victim) / 4) {
        pubsub_trigger_event("events.combat.critical", ch, event_data, PUBSUB_PRIORITY_NORMAL);
    }
}
```

### Pattern 2: Spatial Event Publishing

```c
/* In your spell system (spells.c) */
void cast_fireball(struct char_data *ch, int level) {
    /* ... spell casting logic ... */
    
    /* Publish spatial audio event for nearby players */
    char event_data[MAX_STRING_LENGTH];
    snprintf(event_data, sizeof(event_data),
            "A massive fireball explodes with tremendous force!");
    
    pubsub_publish_wilderness_audio(X_LOC(ch), Y_LOC(ch), 0,
                                   GET_NAME(ch), event_data,
                                   5 + level, /* hearing distance */
                                   PUBSUB_PRIORITY_NORMAL);
}
```

### Pattern 3: Event Handler Implementation

```c
/* Event handler that automatically processes player deaths */
int pubsub_handler_death_processor(struct char_data *ch, struct pubsub_message *msg) {
    if (!ch || !msg) return PUBSUB_ERROR_INVALID_PARAM;
    
    /* Apply death penalties */
    int exp_loss = GET_EXP(ch) / 10; /* 10% experience loss */
    gain_exp(ch, -exp_loss);
    
    /* Create corpse */
    make_corpse(ch);
    
    /* Resurrect player at temple */
    char_from_room(ch);
    char_to_room(ch, find_temple_room());
    
    /* Announce death */
    if (GET_LEVEL(ch) >= 10) {
        char death_msg[MAX_STRING_LENGTH];
        snprintf(death_msg, sizeof(death_msg), 
                "%s has died and been resurrected", GET_NAME(ch));
        pubsub_trigger_event("announcements.death", NULL, death_msg, PUBSUB_PRIORITY_HIGH);
    }
    
    return PUBSUB_SUCCESS;
}
```

## Event Topic Hierarchy

Use a hierarchical naming scheme for events:

```
events.combat.action      - All combat actions
events.combat.critical    - Critical hits only
events.combat.death       - Combat deaths

events.player.login       - Player login
events.player.logout      - Player logout  
events.player.levelup     - Level advancement
events.player.death       - Player death

events.spell.cast         - Spell casting
events.spell.area         - Area effect spells
events.spell.dispel       - Dispel magic events

events.resource.harvest   - Resource harvesting
events.resource.depletion - Resource depletion
events.resource.critical  - Critical shortages

events.guild.promotion    - Guild promotions
events.guild.donation     - Guild donations
events.guild.quest        - Guild quest events

events.environmental.fire - Fire events
events.weather.change     - Weather changes
events.zone.123.events    - Zone-specific events
```

## Setting Up Event-Driven Topics

```c
/* During MUD initialization */
void setup_event_system(void) {
    /* Create event topics */
    pubsub_create_topic("events.combat.action", "Combat events", 
                       PUBSUB_CATEGORY_SYSTEM, PUBSUB_ACCESS_PUBLIC, "System");
    pubsub_create_topic("events.player.death", "Death events",
                       PUBSUB_CATEGORY_SYSTEM, PUBSUB_ACCESS_PUBLIC, "System");
    
    /* Initialize event handlers */
    pubsub_init_event_handlers();
    
    /* Set up "system subscriptions" - handlers that process events automatically */
    /* This would need special implementation since there's no "character" */
}
```

## Advanced Event Patterns

### Conditional Event Processing

```c
int pubsub_handler_guild_monitor(struct char_data *ch, struct pubsub_message *msg) {
    /* Only process events for guild members */
    if (!ch || GET_GUILD(ch) == GUILD_UNDEFINED) {
        return PUBSUB_SUCCESS; /* Skip non-guild players */
    }
    
    /* Process guild-specific logic */
    if (strstr(msg->content, "promotion")) {
        award_guild_experience(ch, 100);
        send_guild_announcement(GET_GUILD(ch), "Member promoted!");
    }
    
    return PUBSUB_SUCCESS;
}
```

### Event Chaining

```c
/* Death event triggers multiple other events */
int pubsub_handler_death_processor(struct char_data *ch, struct pubsub_message *msg) {
    /* Process death */
    apply_death_penalties(ch);
    
    /* Trigger secondary events */
    pubsub_trigger_event("events.economy.death_tax", ch, 
                        "Death tax collection", PUBSUB_PRIORITY_NORMAL);
    pubsub_trigger_event("events.quest.death_failure", ch,
                        "Quest failure due to death", PUBSUB_PRIORITY_HIGH);
    
    return PUBSUB_SUCCESS;
}
```

### Data-Rich Events

```c
/* Create detailed event with metadata */
void publish_detailed_combat_event(struct char_data *attacker, struct char_data *victim, 
                                  int damage, int weapon_type) {
    struct pubsub_message *msg;
    char event_data[MAX_STRING_LENGTH];
    char metadata[MAX_STRING_LENGTH];
    
    snprintf(event_data, sizeof(event_data),
            "Combat: %s -> %s, %d damage", 
            GET_NAME(attacker), GET_NAME(victim), damage);
    
    /* Create rich metadata */
    snprintf(metadata, sizeof(metadata),
            "{\"attacker_level\":%d,\"victim_level\":%d,\"weapon_type\":%d,"
            "\"damage\":%d,\"room\":%d,\"time\":%ld}",
            GET_LEVEL(attacker), GET_LEVEL(victim), weapon_type,
            damage, GET_ROOM_VNUM(IN_ROOM(attacker)), time(NULL));
    
    msg = pubsub_create_message(
        pubsub_find_topic_by_name("events.combat.detailed")->topic_id,
        GET_NAME(attacker), event_data,
        PUBSUB_MESSAGE_TYPE_SYSTEM, PUBSUB_MESSAGE_CATEGORY_COMBAT,
        PUBSUB_PRIORITY_LOW
    );
    
    if (msg) {
        msg->metadata = strdup(metadata);
        pubsub_db_save_message(msg);
        pubsub_publish_to_subscribers(msg);
    }
}
```

## Benefits of Event-Driven Architecture

### 1. **Modularity**
- Add new features without modifying existing code
- Each handler is independent and focused
- Easy to enable/disable specific features

### 2. **Logging and Analytics**
- Automatic logging of all game events
- Combat balance analysis
- Player behavior tracking
- Performance monitoring

### 3. **Dynamic Responses**
- Environmental effects based on player actions
- Adaptive difficulty systems
- Dynamic content generation
- Smart NPC reactions

### 4. **Integration Points**
- Database logging
- External APIs
- Web interfaces
- Real-time monitoring

## Testing Event Handlers

```c
/* Admin command to test event system */
ACMD(do_testevent) {
    char topic[128], data[MAX_STRING_LENGTH];
    two_arguments(argument, topic, data);
    
    if (!*topic || !*data) {
        send_to_char(ch, "Usage: testevent <topic> <data>\r\n");
        return;
    }
    
    pubsub_trigger_event(topic, ch, data, PUBSUB_PRIORITY_NORMAL);
    send_to_char(ch, "Event triggered: %s -> %s\r\n", topic, data);
}
```

## Performance Considerations

### 1. **Event Frequency**
- High-frequency events (combat actions) should use LOW priority
- Important events (deaths, level-ups) use NORMAL or HIGH priority
- Critical system events use URGENT priority

### 2. **Handler Efficiency**
- Keep handlers fast and focused
- Avoid complex processing in handlers
- Use background queues for heavy operations

### 3. **Topic Management**
- Don't create too many fine-grained topics
- Use wildcards for related events
- Clean up unused topics periodically

## Example: Complete Combat Analytics System

```c
/* 1. Publish events from combat system */
void hit(struct char_data *ch, struct char_data *victim, int damage) {
    /* ... combat logic ... */
    
    char event_data[MAX_STRING_LENGTH];
    snprintf(event_data, sizeof(event_data),
            "hit:%s:%s:%d:%d", GET_NAME(ch), GET_NAME(victim), 
            damage, weapon_type);
    pubsub_trigger_event("events.combat.action", ch, event_data, PUBSUB_PRIORITY_LOW);
}

/* 2. Handler automatically processes and logs */
int pubsub_handler_combat_analytics(struct char_data *ch, struct pubsub_message *msg) {
    /* Parse event data */
    char action[32], attacker[64], victim[64];
    int damage, weapon;
    
    sscanf(msg->content, "%[^:]:%[^:]:%[^:]:%d:%d", 
           action, attacker, victim, &damage, &weapon);
    
    /* Update analytics database */
    log_combat_action(attacker, victim, damage, weapon, time(NULL));
    
    /* Check for balance issues */
    if (damage > 500) { /* Extremely high damage */
        char alert[MAX_STRING_LENGTH];
        snprintf(alert, sizeof(alert),
                "BALANCE ALERT: %s dealt %d damage to %s",
                attacker, damage, victim);
        pubsub_trigger_event("admin.balance_alerts", NULL, alert, PUBSUB_PRIORITY_URGENT);
    }
    
    return PUBSUB_SUCCESS;
}

/* 3. Register during initialization */
void init_combat_analytics(void) {
    pubsub_create_topic("events.combat.action", "Combat actions",
                       PUBSUB_CATEGORY_SYSTEM, PUBSUB_ACCESS_PUBLIC, "System");
    pubsub_create_topic("admin.balance_alerts", "Game balance alerts",
                       PUBSUB_CATEGORY_SYSTEM, PUBSUB_ACCESS_ADMIN_ONLY, "System");
    
    pubsub_register_handler("combat_analytics", "Combat analytics processor",
                           pubsub_handler_combat_analytics);
}
```

This event-driven approach makes your MUD highly modular and extensible. You can add new features, monitoring, and integrations without modifying core game logic!
