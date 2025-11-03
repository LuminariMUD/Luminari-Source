/*************************************************************************
*   File: pubsub.c                                     Part of LuminariMUD *
*  Usage: Publish/Subscribe messaging system implementation               *
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
#include <math.h>

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "screen.h"
#include "constants.h"
#include "mysql.h"
#include "wilderness.h"
#include "pubsub.h"

/* Global Variables */
struct pubsub_topic *topic_list = NULL;
struct pubsub_handler *handler_list = NULL;
struct pubsub_player_cache *subscription_cache[SUBSCRIPTION_CACHE_SIZE];
struct pubsub_statistics pubsub_stats;
bool pubsub_system_enabled = FALSE;

/* External variables */
extern MYSQL *conn;

/*
 * Initialize the PubSub system
 */
int pubsub_init(void) {
    int i;
    
    pubsub_info("Initializing PubSub system v%d...", PUBSUB_VERSION);
    
    /* Initialize global variables */
    topic_list = NULL;
    handler_list = NULL;
    memset(&pubsub_stats, 0, sizeof(struct pubsub_statistics));
    
    /* Initialize subscription cache */
    for (i = 0; i < SUBSCRIPTION_CACHE_SIZE; i++) {
        subscription_cache[i] = NULL;
    }
    
#if PUBSUB_DEVELOPMENT_MODE
    /* Drop existing tables to ensure clean schema (development mode only) */
    pubsub_info("Development mode: Dropping existing tables for clean schema");
    pubsub_db_drop_tables();
#endif
    
    /* Create database tables with updated schema */
    if (pubsub_db_create_tables() != PUBSUB_SUCCESS) {
        pubsub_error("Failed to create database tables");
        return PUBSUB_ERROR_DATABASE;
    }
    
    /* Register built-in message handlers */
    pubsub_register_handler("send_text", "Send plain text message to player", 
                           pubsub_handler_send_text);
    pubsub_register_handler("send_formatted", "Send formatted message with color codes",
                           pubsub_handler_send_formatted);
    pubsub_register_handler("spatial_audio", "Process spatial audio events with distance",
                           pubsub_handler_spatial_audio);
    pubsub_register_handler("wilderness_spatial", "Enhanced 3D spatial audio for wilderness",
                           pubsub_handler_wilderness_spatial_audio);
    pubsub_register_handler("audio_mixing", "Multiple simultaneous audio source mixing",
                           pubsub_handler_audio_mixing);
    pubsub_register_handler("personal_message", "Handle personal tell-style messages",
                           pubsub_handler_personal_message);
    pubsub_register_handler("system_announcement", "Handle system-wide announcements",
                           pubsub_handler_system_announcement);
    
    /* Initialize event-driven handlers */
    pubsub_init_event_handlers();
    
    /* Load existing topics from database */
    if (pubsub_db_load_topics() != PUBSUB_SUCCESS) {
        pubsub_error("Failed to load topics from database");
        return PUBSUB_ERROR_DATABASE;
    }
    
    /* Initialize message queue system */
    if (pubsub_queue_init() != PUBSUB_SUCCESS) {
        pubsub_error("Failed to initialize message queue system");
        return PUBSUB_ERROR_QUEUE_INIT;
    }
    
    /* Initialize enhanced database schema */
    if (pubsub_db_populate_data() != PUBSUB_SUCCESS) {
        pubsub_error("Failed to populate database data");
        /* Continue with basic functionality - enhanced features are optional */
    }
    
    /* System is now ready */
    pubsub_system_enabled = TRUE;
    pubsub_info("PubSub system initialized successfully");
    
    return PUBSUB_SUCCESS;
}

/*
 * Shutdown the PubSub system
 */
void pubsub_shutdown(void) {
    struct pubsub_topic *topic, *next_topic;
    struct pubsub_handler *handler, *next_handler;
    
    pubsub_info("Shutting down PubSub system...");
    
    pubsub_system_enabled = FALSE;
    
    /* Shutdown message queue system */
    pubsub_queue_shutdown();
    
    /* Cleanup spatial audio system */
    pubsub_spatial_cleanup();
    
    /* Free topic list */
    for (topic = topic_list; topic; topic = next_topic) {
        next_topic = topic->next;
        PUBSUB_FREE_TOPIC(topic);
    }
    topic_list = NULL;
    
    /* Free handler list */
    for (handler = handler_list; handler; handler = next_handler) {
        next_handler = handler->next;
        if (handler->name) free(handler->name);
        if (handler->description) free(handler->description);
        free(handler);
    }
    handler_list = NULL;
    
    /* Clean up subscription cache */
    pubsub_cleanup_cache();
    
    pubsub_info("PubSub system shutdown complete");
}

/*
 * Drop existing PubSub tables (for schema updates)
 * WARNING: This function destroys all PubSub data!
 * Only used in development mode when PUBSUB_DEVELOPMENT_MODE is enabled.
 */
int pubsub_db_drop_tables(void) {
    char query[MAX_STRING_LENGTH];
    
    pubsub_info("Dropping existing PubSub database tables...");
    
    if (!conn) {
        pubsub_error("Database connection not available");
        return PUBSUB_ERROR_DATABASE;
    }
    
    /* Drop tables in reverse order due to foreign key constraints */
    snprintf(query, sizeof(query), "DROP TABLE IF EXISTS pubsub_messages");
    if (mysql_query(conn, query)) {
        pubsub_error("Failed to drop pubsub_messages table: %s", mysql_error(conn));
    }
    
    snprintf(query, sizeof(query), "DROP TABLE IF EXISTS pubsub_subscriptions");
    if (mysql_query(conn, query)) {
        pubsub_error("Failed to drop pubsub_subscriptions table: %s", mysql_error(conn));
    }
    
    snprintf(query, sizeof(query), "DROP TABLE IF EXISTS pubsub_topics");
    if (mysql_query(conn, query)) {
        pubsub_error("Failed to drop pubsub_topics table: %s", mysql_error(conn));
    }
    
    pubsub_info("Existing PubSub tables dropped successfully");
    return PUBSUB_SUCCESS;
}

/*
 * Create a new topic
 */
int pubsub_create_topic(const char *name, const char *description, 
                       int category, int access_type, const char *creator_name) {
    char query[MAX_STRING_LENGTH];
    char escaped_name[512];
    char escaped_desc[1024];
    char escaped_creator[64];
    struct pubsub_topic *topic;
    
    if (!name || !pubsub_validate_topic_name(name)) {
        return PUBSUB_ERROR_INVALID_PARAM;
    }
    
    if (!pubsub_system_enabled) {
        return PUBSUB_ERROR_DATABASE;
    }
    
    /* Check if topic already exists */
    if (pubsub_find_topic_by_name(name)) {
        return PUBSUB_ERROR_DUPLICATE;
    }
    
    /* Escape strings for SQL safety */
    mysql_real_escape_string(conn, escaped_name, name, strlen(name));
    if (description) {
        mysql_real_escape_string(conn, escaped_desc, description, strlen(description));
    }
    if (creator_name) {
        mysql_real_escape_string(conn, escaped_creator, creator_name, strlen(creator_name));
    }
    
    /* Insert into database */
    if (description && creator_name) {
        snprintf(query, sizeof(query),
            "INSERT INTO pubsub_topics "
            "(topic_name, description, created_by) "
            "VALUES ('%s', '%s', '%s')",
            escaped_name, escaped_desc, escaped_creator);
    } else if (description) {
        snprintf(query, sizeof(query),
            "INSERT INTO pubsub_topics "
            "(topic_name, description, created_by) "
            "VALUES ('%s', '%s', NULL)",
            escaped_name, escaped_desc);
    } else if (creator_name) {
        snprintf(query, sizeof(query),
            "INSERT INTO pubsub_topics "
            "(topic_name, description, created_by) "
            "VALUES ('%s', NULL, '%s')",
            escaped_name, escaped_creator);
    } else {
        snprintf(query, sizeof(query),
            "INSERT INTO pubsub_topics "
            "(topic_name, description, created_by) "
            "VALUES ('%s', NULL, NULL)",
            escaped_name);
    }
    
    if (mysql_query(conn, query)) {
        pubsub_error("Failed to create topic '%s': %s", name, mysql_error(conn));
        return PUBSUB_ERROR_DATABASE;
    }
    
    /* Create topic structure and add to list */
    topic = PUBSUB_CREATE_TOPIC();
    if (!topic) {
        return PUBSUB_ERROR_MEMORY;
    }
    
    topic->topic_id = mysql_insert_id(conn);
    topic->name = strdup(name);
    topic->description = description ? strdup(description) : NULL;
    topic->category = category;
    topic->access_type = access_type;
    topic->creator_name = creator_name ? strdup(creator_name) : NULL;
    topic->created_at = time(NULL);
    topic->is_active = TRUE;
    topic->is_persistent = TRUE;
    topic->message_ttl = PUBSUB_DEFAULT_MESSAGE_TTL;
    topic->max_subscribers = -1; /* unlimited */
    
    /* Add to topic list */
    topic->next = topic_list;
    topic_list = topic;
    
    pubsub_stats.total_topics++;
    pubsub_stats.active_topics++;
    
    pubsub_info("Created topic '%s' (ID: %d) by player %s", 
                name, topic->topic_id, creator_name ? creator_name : "system");
    
    return PUBSUB_SUCCESS;
}

/*
 * Find topic by name
 */
struct pubsub_topic *pubsub_find_topic_by_name(const char *name) {
    struct pubsub_topic *topic;
    
    if (!name) return NULL;
    
    for (topic = topic_list; topic; topic = topic->next) {
        if (topic->name && !strcasecmp(topic->name, name)) {
            return topic;
        }
    }
    
    return NULL;
}

/*
 * Find topic by ID
 */
struct pubsub_topic *pubsub_find_topic_by_id(int topic_id) {
    struct pubsub_topic *topic;
    
    if (topic_id <= 0) return NULL;
    
    for (topic = topic_list; topic; topic = topic->next) {
        if (topic->topic_id == topic_id) {
            return topic;
        }
    }
    
    return NULL;
}

/*
 * Delete a topic by name
 */
int pubsub_delete_topic_by_name(const char *name, const char *deleter_name) {
    struct pubsub_topic *topic, *prev_topic;
    char query[MAX_STRING_LENGTH];
    char escaped_name[256];
    
    /* Input validation */
    if (!name || !deleter_name) {
        return PUBSUB_ERROR_INVALID_PARAM;
    }
    
    if (!pubsub_system_enabled) {
        return PUBSUB_ERROR_DATABASE;
    }
    
    if (!conn) {
        pubsub_error("Database connection not available");
        return PUBSUB_ERROR_DATABASE;
    }
    
    /* Find the topic */
    topic = pubsub_find_topic_by_name(name);
    if (!topic) {
        return PUBSUB_ERROR_NOT_FOUND;
    }
    
    /* Check permissions - only creator or immortals can delete */
    if (topic->creator_name && strcmp(topic->creator_name, deleter_name) != 0) {
        /* Additional permission check would go here for immortals */
        /* For now, only the creator can delete */
        return PUBSUB_ERROR_PERMISSION;
    }
    
    /* Escape the topic name for SQL */
    mysql_real_escape_string(conn, escaped_name, name, strlen(name));
    
    /* Delete from database first */
    snprintf(query, sizeof(query),
        "DELETE FROM pubsub_topics WHERE topic_name = '%s'", escaped_name);
    
    if (mysql_query(conn, query)) {
        pubsub_error("Failed to delete topic '%s' from database: %s", 
                    name, mysql_error(conn));
        return PUBSUB_ERROR_DATABASE;
    }
    
    /* Remove from memory (linked list) */
    prev_topic = NULL;
    struct pubsub_topic *current;
    for (current = topic_list; current; current = current->next) {
        if (current == topic) {
            if (prev_topic) {
                prev_topic->next = current->next;
            } else {
                topic_list = current->next;
            }
            break;
        }
        prev_topic = current;
    }
    
    /* Free memory */
    if (topic->name) free(topic->name);
    if (topic->description) free(topic->description);
    if (topic->creator_name) free(topic->creator_name);
    free(topic);
    
    /* Update statistics */
    pubsub_stats.total_topics--;
    pubsub_stats.active_topics--;
    
    pubsub_info("Topic '%s' deleted by %s", name, deleter_name);
    return PUBSUB_SUCCESS;
}

/*
 * Subscribe a player to a topic
 */
int pubsub_subscribe(struct char_data *ch, int topic_id, const char *handler) {
    const char *player_name = GET_NAME(ch);
    char query[MAX_STRING_LENGTH];
    char escaped_handler[128];
    char escaped_player[64];
    struct pubsub_topic *topic;
    
    /* Input validation */
    if (!ch || topic_id <= 0 || !handler || !player_name) {
        return PUBSUB_ERROR_INVALID_PARAM;
    }
    
    if (!pubsub_system_enabled) {
        return PUBSUB_ERROR_DATABASE;
    }
    
    /* Check if topic exists */
    topic = pubsub_find_topic_by_id(topic_id);
    if (!topic) {
        return PUBSUB_ERROR_NOT_FOUND;
    }
    
    /* Check if already subscribed */
    if (pubsub_is_subscribed(player_name, topic_id)) {
        return PUBSUB_ERROR_DUPLICATE;
    }
    
    /* Check subscription limits */
    if (pubsub_get_player_subscription_count(player_name) >= 
        pubsub_get_max_subscriptions(player_name)) {
        send_to_char(ch, "You have reached your subscription limit.\r\n");
        return PUBSUB_ERROR_SUBSCRIPTION_LIMIT;
    }
    
    /* Check topic subscriber limit */
    if (topic->max_subscribers > 0 && 
        topic->subscriber_count >= topic->max_subscribers) {
        send_to_char(ch, "This topic has reached its subscriber limit.\r\n");
        return PUBSUB_ERROR_TOPIC_FULL;
    }
    
    /* Validate handler exists */
    if (!pubsub_find_handler(handler)) {
        send_to_char(ch, "Unknown message handler '%s'.\r\n", handler);
        return PUBSUB_ERROR_HANDLER_NOT_FOUND;
    }
    
    /* Escape strings for SQL safety */
    mysql_real_escape_string(conn, escaped_handler, handler, strlen(handler));
    mysql_real_escape_string(conn, escaped_player, player_name, strlen(player_name));
    
    /* Insert subscription record */
    snprintf(query, sizeof(query),
        "INSERT INTO pubsub_subscriptions "
        "(topic_id, player_name, handler_name, created_at) "
        "VALUES (%d, '%s', '%s', NOW()) "
        "ON DUPLICATE KEY UPDATE "
        "handler_name = '%s', created_at = NOW()",
        topic_id, escaped_player, escaped_handler, escaped_handler);
    
    if (mysql_query(conn, query)) {
        pubsub_error("PubSub subscribe failed: %s", mysql_error(conn));
        return PUBSUB_ERROR_DATABASE;
    }
    
    /* Update topic subscriber count */
    snprintf(query, sizeof(query),
        "UPDATE pubsub_topics SET subscriber_count = subscriber_count + 1 "
        "WHERE topic_id = %d", topic_id);
    mysql_query(conn, query);
    
    /* Update in-memory topic */
    topic->subscriber_count++;
    
    /* Update player's subscription cache */
    pubsub_cache_player_subscriptions(player_name);
    
    /* Update statistics */
    pubsub_stats.total_subscriptions++;
    pubsub_stats.active_subscriptions++;
    
    pubsub_info("Player %s subscribed to topic '%s' (ID: %d)", 
                GET_NAME(ch), topic->name, topic_id);
    
    return PUBSUB_SUCCESS;
}

/*
 * Publish a message to a topic (Phase 2A - uses message queue)
 */
int pubsub_publish(int topic_id, const char *sender_name, const char *content, 
                  int message_type, int priority) {
    struct pubsub_topic *topic;
    struct pubsub_message *msg;
    struct char_data *target;
    int processed = 0;
    
    if (!sender_name || !content || topic_id <= 0) {
        return PUBSUB_ERROR_INVALID_PARAM;
    }
    
    if (!pubsub_system_enabled) {
        return PUBSUB_ERROR_PERMISSION;
    }
    
    /* Find the topic */
    topic = pubsub_find_topic_by_id(topic_id);
    if (!topic || !topic->is_active) {
        return PUBSUB_ERROR_NOT_FOUND;
    }
    
    /* Create message structure */
    msg = PUBSUB_CREATE_MESSAGE();
    if (!msg) {
        return PUBSUB_ERROR_MEMORY;
    }
    
    msg->message_id = 0; /* Will be assigned by database if needed */
    msg->topic_id = topic_id;
    msg->sender_name = strdup(sender_name);
    msg->content = strdup(content);
    msg->message_type = message_type;
    msg->priority = priority;
    msg->created_at = time(NULL);
    msg->expires_at = msg->created_at + topic->message_ttl;
    msg->delivery_attempts = 0;
    msg->metadata = NULL;
    msg->spatial_data = NULL;
    
    /* Queue message for all subscribers */
    /* Note: In a full implementation, we'd query the database for subscribers
     * For now, we'll iterate through online players and check subscriptions */
    
    for (target = character_list; target; target = target->next) {
        if (IS_NPC(target) || !target->desc) continue;
        
        /* Check if player is subscribed to this topic */
        if (pubsub_is_subscribed(GET_NAME(target), topic_id)) {
            /* Get the player's preferred handler for this topic
             * For now, we'll use the default "send_text" handler */
            int result = pubsub_queue_message(msg, target, "send_text");
            if (result == PUBSUB_SUCCESS) {
                processed++;
            }
        }
    }
    
    /* Update statistics */
    pubsub_stats.total_messages_published++;
    topic->total_messages++;
    topic->last_message_at = time(NULL);
    
    pubsub_info("Published message to topic '%s' (ID: %d), queued for %d players", 
                topic->name, topic_id, processed);
    
    /* Clean up message structure (queue makes copies) */
    PUBSUB_FREE_MESSAGE(msg);
    
    return processed > 0 ? PUBSUB_SUCCESS : PUBSUB_ERROR_NOT_FOUND;
}

/*
 * Publish a spatial audio message to wilderness (Phase 2B)
 */
int pubsub_publish_wilderness_audio(int source_x, int source_y, int source_z,
                                   const char *sender_name, const char *content,
                                   int max_distance, int priority) {
    struct pubsub_message *msg;
    struct char_data *target;
    char spatial_data[256];
    int processed = 0;
    float distance;
    
    if (!sender_name || !content) {
        return PUBSUB_ERROR_INVALID_PARAM;
    }
    
    if (!pubsub_system_enabled) {
        return PUBSUB_ERROR_PERMISSION;
    }
    
    /* Create spatial data string */
    snprintf(spatial_data, sizeof(spatial_data), "%d,%d,%d,%d",
             source_x, source_y, source_z, max_distance);
    
    /* Create message structure */
    msg = PUBSUB_CREATE_MESSAGE();
    if (!msg) {
        return PUBSUB_ERROR_MEMORY;
    }
    
    msg->message_id = 0;
    msg->topic_id = 0; /* Special topic for spatial audio */
    msg->sender_name = strdup(sender_name);
    if (!msg->sender_name) {
        PUBSUB_FREE_MESSAGE(msg);
        return PUBSUB_ERROR_MEMORY;
    }
    msg->content = strdup(content);
    if (!msg->content) {
        PUBSUB_FREE_MESSAGE(msg);
        return PUBSUB_ERROR_MEMORY;
    }
    msg->message_type = PUBSUB_MESSAGE_TYPE_SPATIAL;
    msg->priority = priority;
    msg->spatial_data = strdup(spatial_data);
    if (!msg->spatial_data) {
        PUBSUB_FREE_MESSAGE(msg);
        return PUBSUB_ERROR_MEMORY;
    }
    msg->created_at = time(NULL);
    msg->expires_at = msg->created_at + 300; /* 5 minute TTL */
    msg->delivery_attempts = 0;
    msg->metadata = NULL;
    
    /* Find all players in wilderness within range */
    for (target = character_list; target; target = target->next) {
        if (IS_NPC(target) || !target->desc) continue;
        
        /* Check if player is in wilderness */
        if (!ZONE_FLAGGED(GET_ROOM_ZONE(IN_ROOM(target)), ZONE_WILDERNESS)) {
            continue;
        }
        
        /* Calculate distance to audio source (3D) */
        int target_z = get_modified_elevation(X_LOC(target), Y_LOC(target));
        distance = sqrt(pow(X_LOC(target) - source_x, 2) + 
                       pow(Y_LOC(target) - source_y, 2) +
                       pow((target_z - source_z) / 4.0, 2)); /* Weight elevation less */
        
        /* Check if within hearing range */
        if (distance <= max_distance) {
            /* Queue message with appropriate handler */
            int result = pubsub_queue_message(msg, target, "wilderness_spatial");
            if (result == PUBSUB_SUCCESS) {
                processed++;
                pubsub_debug("Queued spatial audio for %s at distance %.1f", GET_NAME(target), distance);
            } else {
                pubsub_error("Failed to queue spatial audio for %s: error %d", GET_NAME(target), result);
            }
        } else {
            pubsub_debug("Player %s at distance %.1f (too far from max %d)", GET_NAME(target), distance, max_distance);
        }
    }
    
    /* Update statistics */
    pubsub_stats.total_messages_published++;
    
    pubsub_info("Published wilderness audio from (%d,%d,%d), delivered to %d players",
                source_x, source_y, source_z, processed);
    
    /* NOTE: Don't free message here - it's still referenced by queued nodes
     * Message will be freed when queue processing completes */
    
    return PUBSUB_SUCCESS;
}

/*
 * Register a message handler
 */
int pubsub_register_handler(const char *name, const char *description, 
                           pubsub_handler_func func) {
    struct pubsub_handler *handler;
    
    if (!name || !func || !pubsub_validate_handler_name(name)) {
        return PUBSUB_ERROR_INVALID_PARAM;
    }
    
    /* Check if handler already exists */
    if (pubsub_find_handler(name)) {
        return PUBSUB_ERROR_DUPLICATE;
    }
    
    /* Create new handler */
    handler = PUBSUB_CREATE_HANDLER();
    if (!handler) {
        return PUBSUB_ERROR_MEMORY;
    }
    
    handler->name = strdup(name);
    handler->description = description ? strdup(description) : NULL;
    handler->func = func;
    handler->is_enabled = TRUE;
    handler->usage_count = 0;
    
    /* Add to handler list */
    handler->next = handler_list;
    handler_list = handler;
    
    pubsub_debug("Registered handler '%s'", name);
    
    return PUBSUB_SUCCESS;
}

/*
 * Find handler by name
 */
struct pubsub_handler *pubsub_find_handler(const char *name) {
    struct pubsub_handler *handler;
    
    if (!name) return NULL;
    
    for (handler = handler_list; handler; handler = handler->next) {
        if (handler->name && !strcasecmp(handler->name, name)) {
            return handler;
        }
    }
    
    return NULL;
}

/*
 * Call a message handler by name
 */
int pubsub_call_handler(struct char_data *ch, struct pubsub_message *msg, 
                       const char *handler_name) {
    struct pubsub_handler *handler;
    int result;
    
    if (!ch || !msg || !handler_name) {
        return PUBSUB_ERROR_INVALID_PARAM;
    }
    
    /* Find the handler */
    handler = pubsub_find_handler(handler_name);
    if (!handler) {
        pubsub_error("Handler '%s' not found", handler_name);
        return PUBSUB_ERROR_HANDLER_NOT_FOUND;
    }
    
    if (!handler->is_enabled) {
        pubsub_debug("Handler '%s' is disabled", handler_name);
        return PUBSUB_ERROR_HANDLER_NOT_FOUND;
    }
    
    /* Call the handler function */
    result = handler->func(ch, msg);
    
    /* Update handler statistics */
    if (result == PUBSUB_SUCCESS) {
        handler->usage_count++;
        pubsub_debug("Handler '%s' executed successfully for player %s", 
                    handler_name, GET_NAME(ch));
    } else {
        pubsub_error("Handler '%s' failed for player %s: %s", 
                    handler_name, GET_NAME(ch), pubsub_error_string(result));
    }
    
    return result;
}

/*
 * Logging functions
 */
void pubsub_log(const char *format, ...) {
    va_list args;
    char buf[MAX_STRING_LENGTH];
    
    va_start(args, format);
    vsnprintf(buf, sizeof(buf), format, args);
    va_end(args);
    
    log("PUBSUB: %s", buf);
}

void pubsub_info(const char *format, ...) {
    va_list args;
    char buf[MAX_STRING_LENGTH];
    
    va_start(args, format);
    vsnprintf(buf, sizeof(buf), format, args);
    va_end(args);
    
    log("PubSub Info: %s", buf);
}

void pubsub_error(const char *format, ...) {
    va_list args;
    char buf[MAX_STRING_LENGTH];
    
    va_start(args, format);
    vsnprintf(buf, sizeof(buf), format, args);
    va_end(args);
    
    log("PUBSUB ERROR: %s", buf);
}

/*
 * Validation functions
 */
bool pubsub_validate_topic_name(const char *name) {
    int len, i;
    
    if (!name) return FALSE;
    
    len = strlen(name);
    if (len == 0 || len > PUBSUB_MAX_TOPIC_NAME_LENGTH) {
        return FALSE;
    }
    
    /* Check for valid characters (alphanumeric, underscore, dash) */
    for (i = 0; i < len; i++) {
        if (!isalnum(name[i]) && name[i] != '_' && name[i] != '-') {
            return FALSE;
        }
    }
    
    return TRUE;
}

bool pubsub_validate_handler_name(const char *name) {
    int len, i;
    
    if (!name) return FALSE;
    
    len = strlen(name);
    if (len == 0 || len > PUBSUB_MAX_HANDLER_NAME_LENGTH) {
        return FALSE;
    }
    
    /* Check for valid characters (alphanumeric, underscore) */
    for (i = 0; i < len; i++) {
        if (!isalnum(name[i]) && name[i] != '_') {
            return FALSE;
        }
    }
    
    return TRUE;
}

/*
 * Get error message string
 */
const char *pubsub_error_string(int error_code) {
    switch (error_code) {
        case PUBSUB_SUCCESS:                  return "Success";
        case PUBSUB_ERROR_INVALID_PARAM:      return "Invalid parameter";
        case PUBSUB_ERROR_DATABASE:           return "Database error";
        case PUBSUB_ERROR_MEMORY:             return "Memory allocation error";
        case PUBSUB_ERROR_NOT_FOUND:          return "Not found";
        case PUBSUB_ERROR_DUPLICATE:          return "Already exists";
        case PUBSUB_ERROR_PERMISSION:         return "Permission denied";
        case PUBSUB_ERROR_SUBSCRIPTION_LIMIT: return "Subscription limit reached";
        case PUBSUB_ERROR_TOPIC_FULL:         return "Topic at subscriber limit";
        case PUBSUB_ERROR_HANDLER_NOT_FOUND:  return "Handler not found";
        case PUBSUB_ERROR_INVALID_MESSAGE:    return "Invalid message";
        case PUBSUB_ERROR_QUEUE_INIT:         return "Queue initialization failed";
        case PUBSUB_ERROR_QUEUE_FULL:         return "Message queue is full";
        case PUBSUB_ERROR_QUEUE_DISABLED:     return "Queue processing disabled";
        case PUBSUB_ERROR_INVALID_PARAMETER:  return "Invalid parameter";
        default:                              return "Unknown error";
    }
}

/*
 * Automatic message queue processing (called from heartbeat)
 */
void pubsub_process_message_queue(void) {
    static int pulse_count = 0;
    int processed = 0;
    
    /* Only process if system is enabled and queue processing is active */
    if (!pubsub_system_enabled || !pubsub_queue_processing) {
        return;
    }
    
    /* Process queue every few pulses to avoid overloading */
    pulse_count++;
    if (pulse_count < 3) {  /* Process every 3 pulses (~0.75 seconds) */
        return;
    }
    pulse_count = 0;
    
    /* Process a batch of messages */
    processed = pubsub_queue_process_batch(PUBSUB_QUEUE_BATCH_SIZE);
    
    /* Update statistics */
    if (processed > 0) {
        pubsub_debug("Auto-processed %d messages from queue", processed);
    }
}

/*
 * Message Validation Functions
 */

/* Validate message type */
bool pubsub_is_valid_message_type(int message_type)
{
    return (message_type >= PUBSUB_MESSAGE_TYPE_SIMPLE && 
            message_type <= PUBSUB_MESSAGE_TYPE_STATUS);
}

/* Validate message category */
bool pubsub_is_valid_message_category(int message_category)
{
    return (message_category >= PUBSUB_MESSAGE_CATEGORY_COMMUNICATION && 
            message_category <= PUBSUB_MESSAGE_CATEGORY_GUILD);
}

/* Validate message type and category combination */
bool pubsub_is_valid_type_category_combo(int message_type, int message_category)
{
    if (!pubsub_is_valid_message_type(message_type) || 
        !pubsub_is_valid_message_category(message_category)) {
        return false;
    }
    
    /* Define valid combinations - some types naturally fit certain categories */
    switch (message_type) {
        case PUBSUB_MESSAGE_TYPE_SPATIAL:
            /* Spatial messages can be in environmental or communication */
            return (message_category == PUBSUB_MESSAGE_CATEGORY_ENVIRONMENTAL || 
                    message_category == PUBSUB_MESSAGE_CATEGORY_COMMUNICATION);
            
        case PUBSUB_MESSAGE_TYPE_SYSTEM:
            /* System messages should be in system category */
            return (message_category == PUBSUB_MESSAGE_CATEGORY_SYSTEM_EVENT);
            
        case PUBSUB_MESSAGE_TYPE_ALERT:
        case PUBSUB_MESSAGE_TYPE_NOTIFICATION:
            /* Alerts and notifications are usually system events */
            return (message_category == PUBSUB_MESSAGE_CATEGORY_SYSTEM_EVENT ||
                    message_category == PUBSUB_MESSAGE_CATEGORY_GAME_EVENT);
            
        case PUBSUB_MESSAGE_TYPE_COMMAND:
        case PUBSUB_MESSAGE_TYPE_STATUS:
            /* Commands and status are user actions */
            return (message_category == PUBSUB_MESSAGE_CATEGORY_USER_ACTION ||
                    message_category == PUBSUB_MESSAGE_CATEGORY_COMMUNICATION);
            
        default:
            /* Most basic types are compatible with most categories */
            return true;
    }
}

/* Get recommended category for a message type */
int pubsub_get_recommended_category(int message_type)
{
    switch (message_type) {
        case PUBSUB_MESSAGE_TYPE_SIMPLE:
        case PUBSUB_MESSAGE_TYPE_FORMATTED:
        case PUBSUB_MESSAGE_TYPE_PERSONAL:
            return PUBSUB_MESSAGE_CATEGORY_COMMUNICATION;
            
        case PUBSUB_MESSAGE_TYPE_SPATIAL:
            return PUBSUB_MESSAGE_CATEGORY_ENVIRONMENTAL;
            
        case PUBSUB_MESSAGE_TYPE_SYSTEM:
        case PUBSUB_MESSAGE_TYPE_ALERT:
        case PUBSUB_MESSAGE_TYPE_NOTIFICATION:
            return PUBSUB_MESSAGE_CATEGORY_SYSTEM_EVENT;
            
        case PUBSUB_MESSAGE_TYPE_BROADCAST:
            return PUBSUB_MESSAGE_CATEGORY_GAME_EVENT;
            
        case PUBSUB_MESSAGE_TYPE_COMMAND:
        case PUBSUB_MESSAGE_TYPE_STATUS:
            return PUBSUB_MESSAGE_CATEGORY_USER_ACTION;
            
        default:
            return PUBSUB_MESSAGE_CATEGORY_COMMUNICATION; /* Default to communication */
    }
}

/* Create a new enhanced message with validation */
struct pubsub_message *pubsub_create_message(int topic_id, const char *sender_name, 
                                            const char *content, int message_type, 
                                            int message_category, int priority)
{
    struct pubsub_message *msg;
    
    /* Validate parameters */
    if (!sender_name || !content) {
        pubsub_error("pubsub_create_message: NULL sender_name or content");
        return NULL;
    }
    
    if (topic_id <= 0) {
        pubsub_error("pubsub_create_message: Invalid topic_id %d", topic_id);
        return NULL;
    }
    
    /* Validate message type and category */
    if (!pubsub_is_valid_message_type(message_type)) {
        pubsub_error("pubsub_create_message: Invalid message_type %d", message_type);
        return NULL;
    }
    
    if (!pubsub_is_valid_message_category(message_category)) {
        pubsub_error("pubsub_create_message: Invalid message_category %d", message_category);
        return NULL;
    }
    
    if (!pubsub_is_valid_type_category_combo(message_type, message_category)) {
        pubsub_error("pubsub_create_message: Invalid type/category combination %d/%d", 
                    message_type, message_category);
        return NULL;
    }
    
    /* Allocate message structure */
    CREATE(msg, struct pubsub_message, 1);
    if (!msg) {
        pubsub_error("pubsub_create_message: Failed to allocate memory");
        return NULL;
    }
    
    /* Initialize core fields */
    msg->message_id = 0; /* Will be set when saved to database */
    msg->topic_id = topic_id;
    msg->sender_name = strdup(sender_name);
    msg->sender_id = 0; /* Will be set by caller if needed */
    msg->message_type = message_type;
    msg->message_category = message_category;
    msg->priority = priority;
    msg->content = strdup(content);
    
    /* Initialize enhanced fields */
    msg->content_type = strdup("text/plain");
    msg->content_encoding = strdup("utf-8");
    msg->content_version = 1;
    msg->spatial_data = NULL;
    msg->metadata_v3 = NULL;
    msg->fields = NULL;
    msg->metadata = NULL; /* Legacy compatibility */
    
    /* Initialize timing */
    msg->created_at = time(NULL);
    msg->expires_at = msg->created_at + PUBSUB_DEFAULT_MESSAGE_TTL;
    
    /* Initialize threading */
    msg->parent_message_id = 0;
    msg->thread_id = 0;
    msg->sequence_number = 1;
    
    /* Initialize delivery tracking */
    msg->delivery_attempts = 0;
    msg->successful_deliveries = 0;
    msg->failed_deliveries = 0;
    msg->is_processed = false;
    msg->processed_at = 0;
    msg->reference_count = 1;
    
    /* Initialize linked list */
    msg->next = NULL;
    
    return msg;
}

/* Free a message and all associated data */
void pubsub_free_message(struct pubsub_message *msg)
{
    if (!msg) return;
    
    /* Free string fields */
    if (msg->sender_name) free(msg->sender_name);
    if (msg->content) free(msg->content);
    if (msg->content_type) free(msg->content_type);
    if (msg->content_encoding) free(msg->content_encoding);
    if (msg->spatial_data) free(msg->spatial_data);
    if (msg->metadata) free(msg->metadata);
    
    /* Free enhanced structures */
    if (msg->metadata_v3) {
        /* TODO: Implement pubsub_free_metadata when we need it */
        free(msg->metadata_v3);
    }
    if (msg->fields) {
        /* TODO: Implement pubsub_free_fields when we need it */
        free(msg->fields);
    }
    
    /* Free the message itself */
    free(msg);
}

/*
 * List topics for a player (for pubsub list command)
 */
void pubsub_list_topics_for_player(struct char_data *ch) {
    struct pubsub_topic *topic;
    int count = 0;
    
    if (!pubsub_system_enabled) {
        send_to_char(ch, "PubSub system is disabled.\r\n");
        return;
    }
    
    send_to_char(ch, "Available Topics:\r\n");
    send_to_char(ch, "%-20s %-40s %-8s\r\n", "Name", "Description", "Messages");
    send_to_char(ch, "------------------------------------------------------------------------\r\n");
    
    /* Iterate through topic list */
    for (topic = topic_list; topic; topic = topic->next) {
        if (topic->is_active) {
            char desc_display[45];
            const char *desc = topic->description ? topic->description : "No description";
            
            /* Truncate description if too long and add ... */
            if (strlen(desc) > 40) {
                strncpy(desc_display, desc, 37);
                desc_display[37] = '\0';
                strcat(desc_display, "...");
            } else {
                strcpy(desc_display, desc);
            }
            
            send_to_char(ch, "%-20s %-40s %-8d\r\n", 
                        topic->name, 
                        desc_display,
                        topic->total_messages);
            count++;
        }
    }
    
    if (count == 0) {
        send_to_char(ch, "No topics available.\r\n");
    } else {
        send_to_char(ch, "\r\nTotal: %d topic%s\r\n", count, count == 1 ? "" : "s");
    }
}

/*
 * Send a message to a topic
 */
int pubsub_send_message(const char *topic_name, const char *sender_name, 
                       const char *content, int message_type, int category) {
    struct pubsub_topic *topic;
    struct pubsub_message *msg;
    
    if (!pubsub_system_enabled) {
        return PUBSUB_ERROR_DATABASE;
    }
    
    if (!topic_name || !sender_name || !content) {
        return PUBSUB_ERROR_INVALID_PARAM;
    }
    
    /* Find the topic */
    topic = pubsub_find_topic_by_name(topic_name);
    if (!topic) {
        return PUBSUB_ERROR_NOT_FOUND;  /* Use existing error constant */
    }
    
    /* Create and populate message */
    msg = pubsub_create_message(topic->topic_id, sender_name, content, message_type, 
                               category, PUBSUB_PRIORITY_NORMAL);
    if (!msg) {
        return PUBSUB_ERROR_MEMORY;
    }
    
    /* Save to database */
    int result = pubsub_db_save_message(msg);
    if (result != PUBSUB_SUCCESS) {
        pubsub_free_message(msg);
        return result;
    }
    
    /* Trigger event handlers based on topic name */
    struct char_data *dummy_ch = NULL;  /* Create a dummy character for event processing */
    
    /* Find any online character to use as context (needed for some handlers) */
    for (dummy_ch = character_list; dummy_ch; dummy_ch = dummy_ch->next) {
        if (!IS_NPC(dummy_ch) && dummy_ch->desc) {
            break;  /* Found an online player to use as context */
        }
    }
    
    /* Trigger event handlers based on topic name */
    if (!strcasecmp(topic_name, "combat_log") || !strcasecmp(topic_name, "combat")) {
        if (dummy_ch) {
            pubsub_handler_combat_logger(dummy_ch, msg);
        }
    } else if (!strcasecmp(topic_name, "death_events") || !strcasecmp(topic_name, "death")) {
        if (dummy_ch) {
            pubsub_handler_death_processor(dummy_ch, msg);
        }
    } else if (!strcasecmp(topic_name, "levelup") || !strcasecmp(topic_name, "level_events")) {
        if (dummy_ch) {
            pubsub_handler_levelup_processor(dummy_ch, msg);
        }
    }
    
    /* Update topic statistics */
    topic->total_messages++;
    
    pubsub_free_message(msg);
    return PUBSUB_SUCCESS;
}
