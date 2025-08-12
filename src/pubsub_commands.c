/*************************************************************************
*   File: pubsub_commands.c                            Part of LuminariMUD *
*  Usage: Player commands for PubSub system                               *
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
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "screen.h"
#include "constants.h"
#include "wilderness.h"
#include "pubsub.h"

/*
 * Main pubsub command (Phase 1 - basic framework)
 */
ACMD(do_pubsub) {
    send_to_char(ch, "PubSub System v%d - Phase 1 Implementation\r\n", PUBSUB_VERSION);
    send_to_char(ch, "Available commands:\r\n");
    send_to_char(ch, "  subscribe <topic> [handler]  - Subscribe to a topic\r\n");
    send_to_char(ch, "  unsubscribe <topic>          - Unsubscribe from a topic\r\n");
    send_to_char(ch, "  topics                       - List available topics\r\n");
    
    if (GET_LEVEL(ch) >= LVL_IMMORT) {
        send_to_char(ch, "\r\nAdmin commands:\r\n");
        send_to_char(ch, "  pubsubstat                   - Show system statistics\r\n");
        send_to_char(ch, "  pubsubtopic create <name>    - Create a new topic\r\n");
    }
}

/*
 * Subscribe command (Phase 1 - basic functionality)
 */
ACMD(do_subscribe) {
    char topic_name[MAX_INPUT_LENGTH];
    char handler_name[MAX_INPUT_LENGTH];
    struct pubsub_topic *topic;
    int result;
    
    if (!pubsub_system_enabled) {
        send_to_char(ch, "The PubSub system is currently disabled.\r\n");
        return;
    }
    
    /* Parse arguments */
    two_arguments(argument, topic_name, sizeof(topic_name), handler_name, sizeof(handler_name));
    
    if (!*topic_name) {
        send_to_char(ch, "Subscribe to which topic?\r\n");
        send_to_char(ch, "Usage: subscribe <topic> [handler]\r\n");
        return;
    }
    
    /* Default handler if none specified */
    if (!*handler_name) {
        strcpy(handler_name, "send_text");
    }
    
    /* Find the topic */
    topic = pubsub_find_topic_by_name(topic_name);
    if (!topic) {
        send_to_char(ch, "Topic '%s' does not exist.\r\n", topic_name);
        send_to_char(ch, "Use 'topics' to see available topics.\r\n");
        return;
    }
    
    /* Attempt to subscribe */
    result = pubsub_subscribe(ch, topic->topic_id, handler_name);
    
    switch (result) {
        case PUBSUB_SUCCESS:
            send_to_char(ch, "Successfully subscribed to topic '%s' with handler '%s'.\r\n", 
                        topic_name, handler_name);
            break;
        case PUBSUB_ERROR_DUPLICATE:
            send_to_char(ch, "You are already subscribed to topic '%s'.\r\n", topic_name);
            break;
        case PUBSUB_ERROR_SUBSCRIPTION_LIMIT:
            /* Message already sent by pubsub_subscribe */
            break;
        case PUBSUB_ERROR_TOPIC_FULL:
            /* Message already sent by pubsub_subscribe */
            break;
        case PUBSUB_ERROR_HANDLER_NOT_FOUND:
            /* Message already sent by pubsub_subscribe */
            break;
        default:
            send_to_char(ch, "Failed to subscribe: %s\r\n", pubsub_error_string(result));
            break;
    }
}

/*
 * Unsubscribe command (Phase 1 - placeholder)
 */
ACMD(do_unsubscribe) {
    send_to_char(ch, "Unsubscribe functionality will be implemented in Phase 2.\r\n");
}

/*
 * Topics command (Phase 1 - basic listing)
 */
ACMD(do_topics) {
    struct pubsub_topic *topic;
    int count = 0;
    
    if (!pubsub_system_enabled) {
        send_to_char(ch, "The PubSub system is currently disabled.\r\n");
        return;
    }
    
    send_to_char(ch, "Available Topics:\r\n");
    send_to_char(ch, "=================\r\n");
    
    for (topic = topic_list; topic; topic = topic->next) {
        if (!topic->is_active) continue;
        
        /* Basic access control check */
        if (topic->access_type == PUBSUB_ACCESS_ADMIN_ONLY && GET_LEVEL(ch) < LVL_IMMORT) {
            continue;
        }
        
        if (topic->min_level > GET_LEVEL(ch)) {
            continue;
        }
        
        send_to_char(ch, "  %-20s - %s (%d subscribers)\r\n", 
                    topic->name, 
                    topic->description ? topic->description : "No description",
                    topic->subscriber_count);
        count++;
    }
    
    if (count == 0) {
        send_to_char(ch, "  No topics are currently available.\r\n");
    } else {
        send_to_char(ch, "\r\nTotal: %d topic%s\r\n", count, count == 1 ? "" : "s");
        send_to_char(ch, "Use 'subscribe <topic>' to subscribe to a topic.\r\n");
    }
}

/*
 * Publish command (Phase 1 - placeholder)
 */
ACMD(do_publish) {
    send_to_char(ch, "Publish functionality will be implemented in Phase 2.\r\n");
}

/*
 * Admin statistics command (Phase 1)
 */
ACMD(do_pubsubstat) {
    if (GET_LEVEL(ch) < LVL_IMMORT) {
        send_to_char(ch, "You do not have permission to use this command.\r\n");
        return;
    }
    
    if (!pubsub_system_enabled) {
        send_to_char(ch, "The PubSub system is currently disabled.\r\n");
        return;
    }
    
    send_to_char(ch, "PubSub System Statistics:\r\n");
    send_to_char(ch, "========================\r\n");
    send_to_char(ch, "System Status: %s\r\n", pubsub_system_enabled ? "ENABLED" : "DISABLED");
    send_to_char(ch, "Topics: %lld total, %lld active\r\n", 
                 pubsub_stats.total_topics, pubsub_stats.active_topics);
    send_to_char(ch, "Subscriptions: %lld total, %lld active\r\n",
                 pubsub_stats.total_subscriptions, pubsub_stats.active_subscriptions);
    send_to_char(ch, "Messages: %lld sent, %lld delivered, %lld failed\r\n",
                 pubsub_stats.total_messages_sent,
                 pubsub_stats.total_messages_delivered,
                 pubsub_stats.total_messages_failed);
    send_to_char(ch, "Memory: %d topics, %d messages, %d subscriptions allocated\r\n",
                 pubsub_stats.topics_allocated,
                 pubsub_stats.messages_allocated,
                 pubsub_stats.subscriptions_allocated);
}

/*
 * Admin topic management command (Phase 1 - create only)
 */
ACMD(do_pubsubtopic) {
    char subcommand[MAX_INPUT_LENGTH];
    char topic_name[MAX_INPUT_LENGTH];
    char description[MAX_INPUT_LENGTH];
    int result;
    
    if (GET_LEVEL(ch) < LVL_IMMORT) {
        send_to_char(ch, "You do not have permission to use this command.\r\n");
        return;
    }
    
    if (!pubsub_system_enabled) {
        send_to_char(ch, "The PubSub system is currently disabled.\r\n");
        return;
    }
    
    argument = one_argument(argument, subcommand, sizeof(subcommand));
    
    if (!*subcommand) {
        send_to_char(ch, "Usage: pubsubtopic create <name> [description]\r\n");
        return;
    }
    
    if (!strcasecmp(subcommand, "create")) {
        argument = one_argument(argument, topic_name, sizeof(topic_name));
        strcpy(description, argument); /* Rest of argument is description */
        
        if (!*topic_name) {
            send_to_char(ch, "Create topic with what name?\r\n");
            send_to_char(ch, "Usage: pubsubtopic create <name> [description]\r\n");
            return;
        }
        
        result = pubsub_create_topic(topic_name, *description ? description : NULL,
                                   PUBSUB_CATEGORY_GENERAL, PUBSUB_ACCESS_PUBLIC,
                                   GET_NAME(ch));
        
        if (result > 0) {
            send_to_char(ch, "Successfully created topic '%s' (ID: %d).\r\n", 
                        topic_name, result);
        } else {
            send_to_char(ch, "Failed to create topic: %s\r\n", pubsub_error_string(result));
        }
    } else {
        send_to_char(ch, "Unknown subcommand '%s'.\r\n", subcommand);
        send_to_char(ch, "Available subcommands: create\r\n");
    }
}

/*
 * Admin cache management command (Phase 1 - placeholder)
 */
ACMD(do_pubsubcache) {
    if (GET_LEVEL(ch) < LVL_IMMORT) {
        send_to_char(ch, "You do not have permission to use this command.\r\n");
        return;
    }
    
    send_to_char(ch, "Cache management will be implemented in Phase 2.\r\n");
}

/*
 * Admin command hub (Phase 1)
 */
ACMD(do_pubsub_admin) {
    if (GET_LEVEL(ch) < LVL_IMMORT) {
        send_to_char(ch, "You do not have permission to use this command.\r\n");
        return;
    }
    
    send_to_char(ch, "PubSub Admin Commands:\r\n");
    send_to_char(ch, "=====================\r\n");
    send_to_char(ch, "  pubsubstat           - Show system statistics\r\n");
    send_to_char(ch, "  pubsubtopic create   - Create a new topic\r\n");
    send_to_char(ch, "  pubsubcache          - Cache management (Phase 2)\r\n");
    send_to_char(ch, "  pubsubqueue          - Queue management (Phase 2A)\r\n");
}

/*
 * Queue management command (Phase 2A)
 */
ACMD(do_pubsubqueue) {
    char subcommand[MAX_INPUT_LENGTH];
    int processed;
    
    if (GET_LEVEL(ch) < LVL_IMMORT) {
        send_to_char(ch, "You do not have permission to use this command.\r\n");
        return;
    }
    
    if (!pubsub_system_enabled) {
        send_to_char(ch, "The PubSub system is currently disabled.\r\n");
        return;
    }
    
    one_argument(argument, subcommand, sizeof(subcommand));
    
    if (!*subcommand || !strcasecmp(subcommand, "status")) {
        send_to_char(ch, "Message Queue Status:\r\n");
        send_to_char(ch, "====================\r\n");
        send_to_char(ch, "Queue Processing: %s\r\n", 
                    pubsub_queue_processing ? "ENABLED" : "DISABLED");
        send_to_char(ch, "Total Queued: %d messages\r\n", pubsub_queue_get_size());
        send_to_char(ch, "Critical: %d, Urgent: %d, High: %d, Normal: %d, Low: %d\r\n",
                    pubsub_queue_get_priority_count(PUBSUB_PRIORITY_CRITICAL),
                    pubsub_queue_get_priority_count(PUBSUB_PRIORITY_URGENT),
                    pubsub_queue_get_priority_count(PUBSUB_PRIORITY_HIGH),
                    pubsub_queue_get_priority_count(PUBSUB_PRIORITY_NORMAL),
                    pubsub_queue_get_priority_count(PUBSUB_PRIORITY_LOW));
        send_to_char(ch, "Processed: C:%lld U:%lld H:%lld N:%lld L:%lld\r\n",
                    pubsub_stats.queue_critical_processed,
                    pubsub_stats.queue_urgent_processed,
                    pubsub_stats.queue_high_processed,
                    pubsub_stats.queue_normal_processed,
                    pubsub_stats.queue_low_processed);
        send_to_char(ch, "Batch Operations: %lld\r\n", pubsub_stats.queue_batch_operations);
        send_to_char(ch, "Avg Processing Time: %.2f ms\r\n", pubsub_stats.avg_processing_time_ms);
    } else if (!strcasecmp(subcommand, "process")) {
        processed = pubsub_queue_process_all();
        send_to_char(ch, "Processed %d messages from queue.\r\n", processed);
    } else if (!strcasecmp(subcommand, "start")) {
        pubsub_queue_start_processing();
        send_to_char(ch, "Queue processing started.\r\n");
    } else if (!strcasecmp(subcommand, "stop")) {
        pubsub_queue_stop_processing();
        send_to_char(ch, "Queue processing stopped.\r\n");
    } else if (!strcasecmp(subcommand, "test")) {
        /* Test command to create sample messages */
        if (pubsub_publish(1, GET_NAME(ch), "Test message for queue system", 
                          PUBSUB_MESSAGE_TEXT, PUBSUB_PRIORITY_NORMAL) == PUBSUB_SUCCESS) {
            send_to_char(ch, "Test message published and queued.\r\n");
        } else {
            send_to_char(ch, "Failed to publish test message.\r\n");
        }
    } else if (!strcasecmp(subcommand, "spatial")) {
        /* Test wilderness spatial audio */
        if (ZONE_FLAGGED(GET_ROOM_ZONE(IN_ROOM(ch)), ZONE_WILDERNESS)) {
            int result = pubsub_publish_wilderness_audio(X_LOC(ch), Y_LOC(ch), 
                                                        get_modified_elevation(X_LOC(ch), Y_LOC(ch)),
                                                        GET_NAME(ch), "A mysterious sound echoes across the wilderness",
                                                        25, PUBSUB_PRIORITY_NORMAL);
            if (result == PUBSUB_SUCCESS) {
                send_to_char(ch, "Wilderness spatial audio test sent.\r\n");
            } else {
                send_to_char(ch, "Failed to send wilderness spatial audio test.\r\n");
            }
        } else {
            send_to_char(ch, "You must be in the wilderness to test spatial audio.\r\n");
        }
    } else {
        send_to_char(ch, "Usage: pubsubqueue [status|process|start|stop|test|spatial]\r\n");
        send_to_char(ch, "  status  - Show queue status and statistics\r\n");
        send_to_char(ch, "  process - Process all queued messages\r\n");
        send_to_char(ch, "  start   - Start queue processing\r\n");
        send_to_char(ch, "  stop    - Stop queue processing\r\n");
        send_to_char(ch, "  test    - Send a test message to queue\r\n");
        send_to_char(ch, "  spatial - Test wilderness spatial audio (wilderness only)\r\n");
    }
}
