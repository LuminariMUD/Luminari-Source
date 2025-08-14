/*************************************************************************
*   File: pubsub_event_handlers.c                      Part of LuminariMUD *
*  Usage: Event-driven message handlers for PubSub system                 *
*  Author: Luminari Development Team                                       *
*                                                                          *
*  All rights reserved.  See license for complete information.            *
*                                                                          *
*  LuminariMUD is based on CircleMUD, Copyright (C) 1993, 94 by the       *
*  Department of Computer Science at the Johns Hopkins University         *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include "conf.h"
#include "sysdep.h"
#include <time.h>
#include <math.h>
#include <unistd.h>
#include <errno.h>

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "screen.h"
#include "constants.h"
#include "dg_scripts.h"
#include "mud_event.h"
#include "act.h"
#include "race.h"
#include "fight.h"
#include "spells.h"
#include "feats.h"
#include "pubsub.h"

/*
 * Combat Analysis Logger Handler
 * Logs detailed combat events for analysis and balancing
 */
int pubsub_handler_combat_logger(struct char_data *ch, struct pubsub_message *msg) {
    FILE *logfile;
    time_t now;
    char timestamp[32];
    
    if (!ch || !msg) {
        return PUBSUB_ERROR_INVALID_PARAM;
    }
    
    /* Only log for players, not NPCs */
    if (IS_NPC(ch)) {
        return PUBSUB_SUCCESS;
    }
    
    /* Open combat log file - game runs from lib/ directory */
    logfile = fopen("../log/pubsub/combat_events.log", "a");
    if (!logfile) {
        pubsub_error("Cannot open combat events log at ../log/pubsub/combat_events.log - errno: %d", errno);
        return PUBSUB_ERROR_DATABASE;
    }
    
    /* Get timestamp */
    now = time(NULL);
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", localtime(&now));
    
    /* Log the combat event with context */
    fprintf(logfile, "[%s] Player: %s | Level: %d | Class: %s | Event: %s | Room: %d\n",
            timestamp, GET_NAME(ch), GET_LEVEL(ch), 
            class_names[GET_CLASS(ch)], msg->content, GET_ROOM_VNUM(IN_ROOM(ch)));
    
    fclose(logfile);
    
    return PUBSUB_SUCCESS;
}

/*
 * Death Event Processor Handler
 * Handles player death events with automatic processing
 */
int pubsub_handler_death_processor(struct char_data *ch, struct pubsub_message *msg) {
    char death_msg[MAX_STRING_LENGTH];
    
    if (!ch || !msg) {
        return PUBSUB_ERROR_INVALID_PARAM;
    }
    
    /* Only process player deaths */
    if (IS_NPC(ch)) {
        return PUBSUB_SUCCESS;
    }
    
    pubsub_info("Processing death event for %s at level %d", GET_NAME(ch), GET_LEVEL(ch));
    
    /* Basic death message */
    snprintf(death_msg, sizeof(death_msg), 
             "%s has died and been resurrected!", GET_NAME(ch));
    
    /* Send notification to player */
    send_to_char(ch, "\r\n%sYou have died but been brought back to life!%s\r\n\r\n",
                 CCRED(ch, C_NRM), CCNRM(ch, C_NRM));
    
    /* Save character state */
    save_char(ch, 0);
    
    /* Log the event */
    mudlog(BRF, LVL_IMMORT, TRUE, "DEATH: %s died and was auto-resurrected", GET_NAME(ch));
    
    return PUBSUB_SUCCESS;
}

/*
 * Level Up Processor Handler
 * Handles automatic level-up processing and rewards
 */
int pubsub_handler_levelup_processor(struct char_data *ch, struct pubsub_message *msg) {
    char level_msg[MAX_STRING_LENGTH];
    int hit_bonus, move_bonus;
    
    if (!ch || !msg) {
        return PUBSUB_ERROR_INVALID_PARAM;
    }
    
    /* Only process for players */
    if (IS_NPC(ch)) {
        return PUBSUB_SUCCESS;
    }
    
    pubsub_info("Processing level up for %s to level %d", GET_NAME(ch), GET_LEVEL(ch));
    
    /* Calculate bonuses */
    hit_bonus = dice(8, 1) + 1;  /* Simple hit bonus instead of class-specific */
    move_bonus = dice(2, 6) + GET_CON_BONUS(ch);
    
    /* Apply bonuses */
    GET_MAX_HIT(ch) += hit_bonus;
    GET_MAX_MOVE(ch) += move_bonus;
    GET_HIT(ch) = GET_MAX_HIT(ch);
    GET_MOVE(ch) = GET_MAX_MOVE(ch);
    
    /* Level up message */
    snprintf(level_msg, sizeof(level_msg),
             "Congratulations! You've gained %d hit points and %d movement!",
             hit_bonus, move_bonus);
    
    send_to_char(ch, "\r\n%s%s%s\r\n\r\n", 
                 CCYEL(ch, C_NRM), level_msg, CCNRM(ch, C_NRM));
    
    /* Save character */
    save_char(ch, 0);
    
    /* Log the event */
    mudlog(BRF, LVL_IMMORT, TRUE, "LEVELUP: %s reached level %d (+%d hp, +%d mv)", 
           GET_NAME(ch), GET_LEVEL(ch), hit_bonus, move_bonus);
    
    return PUBSUB_SUCCESS;
}

/*
 * Initialize all event handlers
 */
void pubsub_init_event_handlers(void) {
    pubsub_register_handler("combat_logger", "Combat analysis logger", 
                           pubsub_handler_combat_logger);
    
    pubsub_register_handler("death_processor", "Automatic death processing", 
                           pubsub_handler_death_processor);
    
    pubsub_register_handler("levelup_processor", "Automatic level-up processing", 
                           pubsub_handler_levelup_processor);
    
    pubsub_info("Event handlers initialized successfully");
}

/*
 * Trigger an event programmatically
 * This allows code to publish events that handlers can process
 */
/*
 * Event Trigger Function
 * Publishes an event to a specific topic
 */
void pubsub_trigger_event(const char *event_topic, struct char_data *ch, 
                         const char *event_data, int priority) {
    struct pubsub_topic *topic;
    
    if (!event_topic || !event_data) {
        return;
    }
    
    topic = pubsub_find_topic_by_name(event_topic);
    if (!topic) {
        /* Create topic if it doesn't exist */
        int topic_id = pubsub_create_topic(event_topic, "Auto-created event topic",
                                          PUBSUB_CATEGORY_SYSTEM, PUBSUB_ACCESS_PUBLIC,
                                          ch ? GET_NAME(ch) : "System");
        if (topic_id <= 0) {
            return;
        }
        topic = pubsub_find_topic_by_id(topic_id);
    }
    
    /* Publish the event */
    pubsub_publish(topic->topic_id, 
                  ch ? GET_NAME(ch) : "System",
                  event_data,
                  PUBSUB_MESSAGE_TYPE_SYSTEM,
                  priority);
}

