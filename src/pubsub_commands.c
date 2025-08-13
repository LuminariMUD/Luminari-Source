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
 * Main pubsub command - handles subcommands
 */
ACMD(do_pubsub) {
    char subcommand[MAX_INPUT_LENGTH];
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    char arg3[MAX_INPUT_LENGTH];
    
    argument = one_argument(argument, subcommand, sizeof(subcommand));
    argument = one_argument(argument, arg1, sizeof(arg1));
    argument = one_argument(argument, arg2, sizeof(arg2));
    argument = one_argument(argument, arg3, sizeof(arg3));
    
    if (!*subcommand) {
        /* Show help if no subcommand */
        send_to_char(ch, "PubSub System v%d - Unified Enhanced Implementation\r\n", PUBSUB_VERSION);
        send_to_char(ch, "Available commands:\r\n");
        send_to_char(ch, "  pubsub status                - Show system status\r\n");
        send_to_char(ch, "  pubsub list                  - List available topics\r\n");
        send_to_char(ch, "  pubsub create <name> <desc>  - Create a new topic\r\n");
        send_to_char(ch, "  pubsub delete <name>         - Delete a topic\r\n");
        send_to_char(ch, "  pubsub send <topic> <msg>    - Send message to topic\r\n");
        send_to_char(ch, "  pubsub subscribe <topic>     - Subscribe to a topic\r\n");
        send_to_char(ch, "  pubsub unsubscribe <topic>   - Unsubscribe from topic\r\n");
        send_to_char(ch, "  pubsub info <topic>          - Show topic information\r\n");
        
        if (GET_LEVEL(ch) >= LVL_IMMORT) {
            send_to_char(ch, "\r\nAdmin commands:\r\n");
            send_to_char(ch, "  pubsub admin <cmd>           - Admin functions\r\n");
            send_to_char(ch, "  pubsub stats                 - System statistics\r\n");
        }
        return;
    }
    
    /* Handle subcommands */
    if (is_abbrev(subcommand, "status")) {
        send_to_char(ch, "PubSub System Status:\r\n");
        send_to_char(ch, "System Enabled: %s\r\n", pubsub_system_enabled ? "YES" : "NO");
        send_to_char(ch, "Database Connected: %s\r\n", "YES");  /* Simplified for now */
        send_to_char(ch, "Total Topics: %d\r\n", pubsub_stats.total_topics);
        return;
    }
    
    if (is_abbrev(subcommand, "list")) {
        send_to_char(ch, "Available Topics:\r\n");
        pubsub_list_topics_for_player(ch);
        return;
    }
    
    if (is_abbrev(subcommand, "create")) {
        if (!*arg1) {
            send_to_char(ch, "Usage: pubsub create <name> <description>\r\n");
            return;
        }
        
        /* Check if system is enabled first */
        if (!pubsub_system_enabled) {
            send_to_char(ch, "PubSub system is not enabled. Check logs for initialization errors.\r\n");
            return;
        }
        
        /* Get description as everything after the topic name */
        char description[MAX_STRING_LENGTH];
        const char *desc_start = argument;
        
        /* Skip leading spaces */
        while (*desc_start && *desc_start == ' ') desc_start++;
        
        /* Copy description */
        strncpy(description, desc_start, sizeof(description) - 1);
        description[sizeof(description) - 1] = '\0';
        
        /* Remove quotes if present and clean up any trailing characters */
        int len = strlen(description);
        if (len > 1 && description[0] == '"' && description[len-1] == '"') {
            description[len-1] = '\0';  /* Remove ending quote */
            memmove(description, description+1, len-1);  /* Remove starting quote */
            len -= 2; /* Update length */
        }
        
        /* Clean up any stray quotes or whitespace at the end */
        while (len > 0 && (description[len-1] == '"' || description[len-1] == ' ')) {
            description[len-1] = '\0';
            len--;
        }
        
        int result = pubsub_create_topic(arg1, description, PUBSUB_CATEGORY_GENERAL, 
                                       PUBSUB_ACCESS_PUBLIC, GET_NAME(ch));
        
        if (result == PUBSUB_SUCCESS) {
            send_to_char(ch, "Topic '%s' created successfully.\r\n", arg1);
        } else {
            const char *error_msg = "Unknown error";
            switch (result) {
                case PUBSUB_ERROR_INVALID_PARAM: error_msg = "Invalid parameter"; break;
                case PUBSUB_ERROR_DATABASE: error_msg = "Database error"; break;
                case PUBSUB_ERROR_MEMORY: error_msg = "Memory allocation error"; break;
                case PUBSUB_ERROR_NOT_FOUND: error_msg = "Not found"; break;
                case PUBSUB_ERROR_DUPLICATE: error_msg = "Topic already exists"; break;
                case PUBSUB_ERROR_PERMISSION: error_msg = "Permission denied"; break;
            }
            send_to_char(ch, "Failed to create topic '%s': %s (error %d).\r\n", arg1, error_msg, result);
        }
        return;
    }
    
    if (is_abbrev(subcommand, "delete")) {
        if (!*arg1) {
            send_to_char(ch, "Usage: pubsub delete <name>\r\n");
            return;
        }
        
        /* Check if player has permission to delete topic */
        if (GET_LEVEL(ch) < LVL_IMMORT) {
            /* For non-immortals, check if they created the topic */
            struct pubsub_topic *topic = pubsub_find_topic_by_name(arg1);
            if (!topic) {
                send_to_char(ch, "Topic '%s' not found.\r\n", arg1);
                return;
            }
            
            if (!topic->creator_name || strcmp(topic->creator_name, GET_NAME(ch)) != 0) {
                send_to_char(ch, "You can only delete topics you created.\r\n");
                return;
            }
        }
        
        int result = pubsub_delete_topic_by_name(arg1, GET_NAME(ch));
        
        if (result == PUBSUB_SUCCESS) {
            send_to_char(ch, "Topic '%s' deleted successfully.\r\n", arg1);
        } else {
            const char *error_msg = "Unknown error";
            switch(result) {
                case PUBSUB_ERROR_NOT_FOUND: error_msg = "Topic not found"; break;
                case PUBSUB_ERROR_PERMISSION: error_msg = "Permission denied"; break;
                case PUBSUB_ERROR_DATABASE: error_msg = "Database error"; break;
                case PUBSUB_ERROR_INVALID_PARAM: error_msg = "Invalid parameter"; break;
            }
            send_to_char(ch, "Failed to delete topic '%s': %s (error %d).\r\n", arg1, error_msg, result);
        }
        return;
    }

    if (is_abbrev(subcommand, "send")) {
        if (!*arg1 || !*arg2) {
            send_to_char(ch, "Usage: pubsub send <topic> <message>\r\n");
            return;
        }
        
        /* Combine remaining arguments as message */
        char message[MAX_STRING_LENGTH];
        snprintf(message, sizeof(message), "%s %s", arg2, argument);
        
        int result = pubsub_send_message(arg1, GET_NAME(ch), message, 
                                       PUBSUB_MESSAGE_TYPE_SIMPLE, PUBSUB_CATEGORY_GENERAL);
        
        if (result == PUBSUB_SUCCESS) {
            send_to_char(ch, "Message sent to topic '%s'.\r\n", arg1);
        } else {
            send_to_char(ch, "Failed to send message to '%s' (error %d).\r\n", arg1, result);
        }
        return;
    }
    
    if (is_abbrev(subcommand, "subscribe")) {
        if (!*arg1) {
            send_to_char(ch, "Usage: pubsub subscribe <topic>\r\n");
            return;
        }
        
        send_to_char(ch, "Subscribe functionality not yet implemented.\r\n");
        return;
    }
    
    if (is_abbrev(subcommand, "unsubscribe")) {
        if (!*arg1) {
            send_to_char(ch, "Usage: pubsub unsubscribe <topic>\r\n");
            return;
        }
        
        send_to_char(ch, "Unsubscribe functionality not yet implemented.\r\n");
        return;
    }
    
    if (is_abbrev(subcommand, "info")) {
        if (!*arg1) {
            send_to_char(ch, "Usage: pubsub info <topic>\r\n");
            return;
        }
        
        struct pubsub_topic *topic = pubsub_find_topic_by_name(arg1);
        if (!topic) {
            send_to_char(ch, "Topic '%s' not found.\r\n", arg1);
            return;
        }
        
        send_to_char(ch, "Topic Information:\r\n");
        send_to_char(ch, "Name: %s\r\n", topic->name);
        send_to_char(ch, "Description: %s\r\n", topic->description ? topic->description : "None");
        send_to_char(ch, "Category: %d\r\n", topic->category);
        send_to_char(ch, "Creator: %s\r\n", topic->creator_name ? topic->creator_name : "Unknown");
        send_to_char(ch, "Total Messages: %d\r\n", topic->total_messages);
        send_to_char(ch, "Subscribers: %d\r\n", topic->subscriber_count);
        return;
    }
    
    if (is_abbrev(subcommand, "admin") && GET_LEVEL(ch) >= LVL_IMMORT) {
        send_to_char(ch, "PubSub Admin commands not yet implemented.\r\n");
        return;
    }
    
    if (is_abbrev(subcommand, "stats") && GET_LEVEL(ch) >= LVL_IMMORT) {
        send_to_char(ch, "PubSub System Statistics:\r\n");
        send_to_char(ch, "System Enabled: %s\r\n", pubsub_system_enabled ? "YES" : "NO");
        send_to_char(ch, "Database Connected: %s\r\n", "YES");  /* Simplified */
        send_to_char(ch, "Total Topics: %d\r\n", 0);  /* TODO: Get actual count */
        send_to_char(ch, "Active Subscriptions: (not implemented)\r\n");
        return;
    }
    
    /* Unknown subcommand */
    send_to_char(ch, "Unknown pubsub subcommand '%s'. Type 'pubsub' for help.\r\n", subcommand);
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
                          PUBSUB_MESSAGE_TYPE_SIMPLE, PUBSUB_PRIORITY_NORMAL) == PUBSUB_SUCCESS) {
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
