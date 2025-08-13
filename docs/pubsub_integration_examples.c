/*************************************************************************
*   File: pubsub_integration_example.c                 Part of LuminariMUD *
*  Usage: Example integration of PubSub events into game systems          *
*  Author: Luminari Development Team                                       *
*                                                                          *
*  This file shows how to integrate PubSub event publishing into          *
*  existing game mechanics. Copy these patterns into your actual          *
*  game code files (combat.c, spells.c, etc.)                            *
************************************************************************ */

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "pubsub.h"

/*
 * EXAMPLE 1: Integration into combat system
 * Add this to your combat hit() function or damage() function
 */
void integrate_combat_events_example(struct char_data *ch, struct char_data *victim, 
                                   int damage, int weapon_type) {
    char event_data[MAX_STRING_LENGTH];
    
    /* Publish detailed combat event */
    snprintf(event_data, sizeof(event_data),
            "Combat: %s attacks %s with %s for %d damage",
            GET_NAME(ch), GET_NAME(victim), 
            weapon_type >= 0 ? weapon_list[weapon_type].name : "bare hands",
            damage);
    
    pubsub_trigger_event("events.combat.action", ch, event_data, PUBSUB_PRIORITY_LOW);
    
    /* Critical hit events */
    if (damage > GET_MAX_HIT(victim) / 4) { /* More than 25% of max HP */
        snprintf(event_data, sizeof(event_data),
                "Critical hit: %d damage to %s", damage, GET_NAME(victim));
        pubsub_trigger_event("events.combat.critical", ch, event_data, PUBSUB_PRIORITY_NORMAL);
    }
    
    /* Death event */
    if (GET_HIT(victim) <= 0) {
        snprintf(event_data, sizeof(event_data),
                "Player death: %s killed by %s", GET_NAME(victim), GET_NAME(ch));
        pubsub_trigger_event("events.player.death", victim, event_data, PUBSUB_PRIORITY_HIGH);
    }
}

/*
 * EXAMPLE 2: Integration into spell casting system
 * Add this to your cast_spell() function
 */
void integrate_spell_events_example(struct char_data *ch, int spellnum, int level, 
                                  struct char_data *target) {
    char event_data[MAX_STRING_LENGTH];
    char spatial_data[256];
    
    /* Basic spell casting event */
    snprintf(event_data, sizeof(event_data),
            "Spell %d level %d", spellnum, level);
    pubsub_trigger_event("events.spell.cast", ch, event_data, PUBSUB_PRIORITY_NORMAL);
    
    /* Area effect spells get spatial events */
    if (spell_info[spellnum].targets & TAR_AREA_ATTACK) {
        /* Create spatial data for area effects */
        snprintf(spatial_data, sizeof(spatial_data), "%d,%d,%d,%d",
                X_LOC(ch), Y_LOC(ch), 0, 3); /* 3-room radius */
        
        struct pubsub_message *spatial_msg = pubsub_create_message(
            pubsub_find_topic_by_name("events.spell.area")->topic_id,
            GET_NAME(ch), event_data,
            PUBSUB_MESSAGE_TYPE_SPATIAL, PUBSUB_MESSAGE_CATEGORY_GAME_EVENT,
            PUBSUB_PRIORITY_NORMAL
        );
        
        if (spatial_msg) {
            spatial_msg->spatial_data = strdup(spatial_data);
            pubsub_db_save_message(spatial_msg);
            pubsub_publish_to_subscribers(spatial_msg);
        }
    }
    
    /* High-level magic creates wilderness audio events */
    if (level >= 6 && ZONE_FLAGGED(GET_ROOM_ZONE(IN_ROOM(ch)), ZONE_WILDERNESS)) {
        snprintf(event_data, sizeof(event_data),
                "Powerful magic energies ripple from %s's %s spell",
                GET_NAME(ch), spell_info[spellnum].name);
        
        pubsub_publish_wilderness_audio(X_LOC(ch), Y_LOC(ch), 0,
                                       "Magical Energies", event_data,
                                       10 + level, PUBSUB_PRIORITY_NORMAL);
    }
}

/*
 * EXAMPLE 3: Integration into leveling system
 * Add this to your advance_level() or gain_exp() function
 */
void integrate_levelup_events_example(struct char_data *ch, int old_level, int new_level) {
    char event_data[MAX_STRING_LENGTH];
    
    /* Level advancement event */
    snprintf(event_data, sizeof(event_data),
            "Level %d to %d", old_level, new_level);
    pubsub_trigger_event("events.player.levelup", ch, event_data, PUBSUB_PRIORITY_HIGH);
    
    /* Milestone level events */
    if (new_level == 10 || new_level == 20 || new_level == 30) {
        snprintf(event_data, sizeof(event_data),
                "Milestone level %d reached", new_level);
        pubsub_trigger_event("events.player.milestone", ch, event_data, PUBSUB_PRIORITY_HIGH);
    }
    
    /* Class-specific events */
    switch (GET_CLASS(ch)) {
        case CLASS_WIZARD:
            if (new_level % 2 == 0) { /* Every even level */
                snprintf(event_data, sizeof(event_data),
                        "Wizard spell learning opportunity: level %d", new_level);
                pubsub_trigger_event("events.class.wizard.spells", ch, event_data, PUBSUB_PRIORITY_NORMAL);
            }
            break;
            
        case CLASS_FIGHTER:
            if (new_level % 3 == 0) { /* Every 3rd level */
                snprintf(event_data, sizeof(event_data),
                        "Fighter feat selection: level %d", new_level);
                pubsub_trigger_event("events.class.fighter.feats", ch, event_data, PUBSUB_PRIORITY_NORMAL);
            }
            break;
    }
}

/*
 * EXAMPLE 4: Integration into resource system (wilderness)
 * Add this to your resource harvesting functions
 */
void integrate_resource_events_example(struct char_data *ch, int resource_type, 
                                     int amount_harvested, int remaining) {
    char event_data[MAX_STRING_LENGTH];
    char spatial_data[256];
    struct pubsub_message *msg;
    
    /* Resource harvesting event */
    snprintf(event_data, sizeof(event_data),
            "Harvested %d units of resource type %d, %d remaining",
            amount_harvested, resource_type, remaining);
    pubsub_trigger_event("events.resource.harvest", ch, event_data, PUBSUB_PRIORITY_LOW);
    
    /* Resource depletion warning */
    if (remaining <= 20 && remaining > 10) {
        snprintf(event_data, sizeof(event_data),
                "Resource %d depleted to %d", resource_type, remaining);
        
        /* Create spatial event for nearby players */
        snprintf(spatial_data, sizeof(spatial_data), "%d,%d,%d,%d",
                X_LOC(ch), Y_LOC(ch), 0, 5); /* 5-room radius */
        
        msg = pubsub_create_message(
            pubsub_find_topic_by_name("events.resource.depletion")->topic_id,
            GET_NAME(ch), event_data,
            PUBSUB_MESSAGE_TYPE_ENVIRONMENTAL, PUBSUB_MESSAGE_CATEGORY_ENVIRONMENTAL,
            PUBSUB_PRIORITY_NORMAL
        );
        
        if (msg) {
            msg->spatial_data = strdup(spatial_data);
            pubsub_db_save_message(msg);
            pubsub_publish_to_subscribers(msg);
        }
    }
    
    /* Critical depletion alert */
    if (remaining <= 10) {
        snprintf(event_data, sizeof(event_data),
                "CRITICAL: Resource %d depleted to %d at (%d,%d)",
                resource_type, remaining, X_LOC(ch), Y_LOC(ch));
        pubsub_trigger_event("events.resource.critical", ch, event_data, PUBSUB_PRIORITY_URGENT);
    }
}

/*
 * EXAMPLE 5: Integration into guild system
 * Add this to your guild promotion, donation, quest functions
 */
void integrate_guild_events_example(struct char_data *ch, const char *action, 
                                  const char *details) {
    char event_data[MAX_STRING_LENGTH];
    char topic_name[128];
    
    /* General guild event */
    snprintf(event_data, sizeof(event_data),
            "Guild action: %s - %s", action, details);
    pubsub_trigger_event("events.guild.activity", ch, event_data, PUBSUB_PRIORITY_NORMAL);
    
    /* Specific guild events */
    if (!strcasecmp(action, "promotion")) {
        snprintf(event_data, sizeof(event_data),
                "promotion to %s", details);
        pubsub_trigger_event("events.guild.promotion", ch, event_data, PUBSUB_PRIORITY_HIGH);
        
    } else if (!strcasecmp(action, "donation")) {
        snprintf(event_data, sizeof(event_data),
                "donation: %s", details);
        pubsub_trigger_event("events.guild.donation", ch, event_data, PUBSUB_PRIORITY_LOW);
        
    } else if (!strcasecmp(action, "quest_completed")) {
        snprintf(event_data, sizeof(event_data),
                "quest_completed: %s", details);
        pubsub_trigger_event("events.guild.quest", ch, event_data, PUBSUB_PRIORITY_NORMAL);
    }
    
    /* Guild-specific topic */
    snprintf(topic_name, sizeof(topic_name), "guild.%d.events", GET_GUILD(ch));
    pubsub_trigger_event(topic_name, ch, event_data, PUBSUB_PRIORITY_NORMAL);
}

/*
 * EXAMPLE 6: Integration into environmental system
 * Add this to weather, time, or environmental effect functions
 */
void integrate_environmental_events_example(int zone_vnum, const char *effect, 
                                          int severity) {
    char event_data[MAX_STRING_LENGTH];
    char topic_name[128];
    int priority;
    
    /* Determine priority based on severity */
    switch (severity) {
        case 1: priority = PUBSUB_PRIORITY_LOW; break;
        case 2: priority = PUBSUB_PRIORITY_NORMAL; break;
        case 3: priority = PUBSUB_PRIORITY_HIGH; break;
        case 4: priority = PUBSUB_PRIORITY_URGENT; break;
        default: priority = PUBSUB_PRIORITY_NORMAL; break;
    }
    
    /* General environmental event */
    snprintf(event_data, sizeof(event_data),
            "Environmental effect in zone %d: %s (severity %d)",
            zone_vnum, effect, severity);
    pubsub_trigger_event("events.environmental.global", NULL, event_data, priority);
    
    /* Zone-specific environmental events */
    snprintf(topic_name, sizeof(topic_name), "events.zone.%d.environment", zone_vnum);
    pubsub_trigger_event(topic_name, NULL, event_data, priority);
    
    /* Weather-specific events */
    if (strstr(effect, "storm") || strstr(effect, "rain") || strstr(effect, "snow")) {
        snprintf(event_data, sizeof(event_data),
                "Weather change: %s", effect);
        pubsub_trigger_event("events.weather.change", NULL, event_data, priority);
    }
    
    /* Fire events */
    if (strstr(effect, "fire")) {
        snprintf(event_data, sizeof(event_data),
                "Fire event: %s", effect);
        pubsub_trigger_event("events.environmental.fire", NULL, event_data, PUBSUB_PRIORITY_URGENT);
    }
}

/*
 * EXAMPLE 7: Setup event topics and subscriptions
 * Call this during mud startup to initialize event-driven topics
 */
void setup_event_driven_topics(void) {
    /* Create core event topics */
    pubsub_create_topic("events.combat.action", "Combat action events", 
                       PUBSUB_CATEGORY_SYSTEM, PUBSUB_ACCESS_PUBLIC, "System");
    pubsub_create_topic("events.combat.critical", "Critical hit events",
                       PUBSUB_CATEGORY_SYSTEM, PUBSUB_ACCESS_PUBLIC, "System");
    pubsub_create_topic("events.player.death", "Player death events",
                       PUBSUB_CATEGORY_SYSTEM, PUBSUB_ACCESS_PUBLIC, "System");
    pubsub_create_topic("events.player.levelup", "Player level advancement",
                       PUBSUB_CATEGORY_SYSTEM, PUBSUB_ACCESS_PUBLIC, "System");
    pubsub_create_topic("events.player.milestone", "Milestone level achievements",
                       PUBSUB_CATEGORY_SYSTEM, PUBSUB_ACCESS_PUBLIC, "System");
    pubsub_create_topic("events.spell.cast", "Spell casting events",
                       PUBSUB_CATEGORY_SYSTEM, PUBSUB_ACCESS_PUBLIC, "System");
    pubsub_create_topic("events.spell.area", "Area effect spell events",
                       PUBSUB_CATEGORY_SYSTEM, PUBSUB_ACCESS_PUBLIC, "System");
    pubsub_create_topic("events.resource.harvest", "Resource harvesting events",
                       PUBSUB_CATEGORY_WILDERNESS, PUBSUB_ACCESS_PUBLIC, "System");
    pubsub_create_topic("events.resource.depletion", "Resource depletion warnings",
                       PUBSUB_CATEGORY_WILDERNESS, PUBSUB_ACCESS_PUBLIC, "System");
    pubsub_create_topic("events.resource.critical", "Critical resource alerts",
                       PUBSUB_CATEGORY_WILDERNESS, PUBSUB_ACCESS_ADMIN_ONLY, "System");
    pubsub_create_topic("events.guild.activity", "General guild activities",
                       PUBSUB_CATEGORY_GUILD, PUBSUB_ACCESS_PUBLIC, "System");
    pubsub_create_topic("events.guild.promotion", "Guild promotions",
                       PUBSUB_CATEGORY_GUILD, PUBSUB_ACCESS_PUBLIC, "System");
    pubsub_create_topic("events.environmental.global", "Global environmental events",
                       PUBSUB_CATEGORY_SYSTEM, PUBSUB_ACCESS_PUBLIC, "System");
    pubsub_create_topic("events.weather.change", "Weather change events",
                       PUBSUB_CATEGORY_SYSTEM, PUBSUB_ACCESS_PUBLIC, "System");
    
    /* Set up code subscriptions (no actual players, just handlers) */
    struct pubsub_topic *topic;
    
    /* Combat logger subscribes to combat events */
    topic = pubsub_find_topic_by_name("events.combat.action");
    if (topic) {
        /* Create a system subscription - this would need special handling 
         * since there's no actual character, but the handler can still process events */
    }
    
    /* Death processor subscribes to death events */
    topic = pubsub_find_topic_by_name("events.player.death");
    if (topic) {
        /* System subscription for death processing */
    }
    
    /* Resource monitor subscribes to resource events */
    topic = pubsub_find_topic_by_name("events.resource.depletion");
    if (topic) {
        /* System subscription for resource monitoring */
    }
    
    pubsub_info("Set up %d event-driven topics", 14);
}

/*
 * EXAMPLE 8: Admin command to test event system
 * Add this as a new admin command: "testevent"
 */
ACMD(do_testevent) {
    char topic_name[128], event_data[MAX_STRING_LENGTH];
    
    two_arguments(argument, topic_name, event_data);
    
    if (!*topic_name || !*event_data) {
        send_to_char(ch, "Usage: testevent <topic> <event_data>\r\n");
        send_to_char(ch, "Example: testevent events.combat.action 'Test combat event'\r\n");
        return;
    }
    
    /* Trigger the test event */
    pubsub_trigger_event(topic_name, ch, event_data, PUBSUB_PRIORITY_NORMAL);
    
    send_to_char(ch, "Triggered event on topic '%s': %s\r\n", topic_name, event_data);
    
    /* Show who's subscribed to this topic */
    struct pubsub_topic *topic = pubsub_find_topic_by_name(topic_name);
    if (topic) {
        send_to_char(ch, "Topic has %d subscribers.\r\n", topic->subscriber_count);
    } else {
        send_to_char(ch, "Warning: Topic '%s' does not exist.\r\n", topic_name);
    }
}
