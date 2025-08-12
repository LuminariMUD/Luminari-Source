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

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "screen.h"
#include "constants.h"
#include "mysql.h"
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
    
    /* Create database tables if they don't exist */
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
    pubsub_register_handler("personal_message", "Handle personal tell-style messages",
                           pubsub_handler_personal_message);
    pubsub_register_handler("system_announcement", "Handle system-wide announcements",
                           pubsub_handler_system_announcement);
    
    /* Load existing topics from database */
    if (pubsub_db_load_topics() != PUBSUB_SUCCESS) {
        pubsub_error("Failed to load topics from database");
        return PUBSUB_ERROR_DATABASE;
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
 * Create database tables for PubSub system
 */
int pubsub_db_create_tables(void) {
    char query[MAX_STRING_LENGTH * 4];
    
    pubsub_info("Creating PubSub database tables...");
    
    if (!conn) {
        pubsub_error("Database connection not available");
        return PUBSUB_ERROR_DATABASE;
    }
    
    /* Create topics table */
    snprintf(query, sizeof(query),
        "CREATE TABLE IF NOT EXISTS pubsub_topics ("
        "topic_id INT AUTO_INCREMENT PRIMARY KEY,"
        "name VARCHAR(255) UNIQUE NOT NULL,"
        "description TEXT,"
        "category INT DEFAULT 0,"
        "access_type INT DEFAULT 0,"
        "min_level INT DEFAULT 1,"
        "creator_name VARCHAR(30),"
        "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,"
        "last_message_at TIMESTAMP NULL,"
        "total_messages INT DEFAULT 0,"
        "subscriber_count INT DEFAULT 0,"
        "max_subscribers INT DEFAULT -1,"
        "message_ttl INT DEFAULT 3600,"
        "is_persistent BOOLEAN DEFAULT TRUE,"
        "is_active BOOLEAN DEFAULT TRUE,"
        "INDEX idx_name (name),"
        "INDEX idx_category (category),"
        "INDEX idx_access_type (access_type),"
        "INDEX idx_creator (creator_name),"
        "INDEX idx_active (is_active),"
        "FOREIGN KEY (creator_name) REFERENCES player_data(name) ON DELETE SET NULL"
        ")");
    
    if (mysql_query(conn, query)) {
        pubsub_error("Failed to create pubsub_topics table: %s", mysql_error(conn));
        return PUBSUB_ERROR_DATABASE;
    }
    
    /* Create subscriptions table */
    snprintf(query, sizeof(query),
        "CREATE TABLE IF NOT EXISTS pubsub_subscriptions ("
        "subscription_id INT AUTO_INCREMENT PRIMARY KEY,"
        "topic_id INT NOT NULL,"
        "player_name VARCHAR(30) NOT NULL,"
        "handler_name VARCHAR(64) DEFAULT 'send_text',"
        "handler_data TEXT,"
        "status INT DEFAULT 0,"
        "subscribed_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,"
        "INDEX idx_topic (topic_id),"
        "INDEX idx_player (player_name),"
        "INDEX idx_status (status),"
        "FOREIGN KEY (topic_id) REFERENCES pubsub_topics(topic_id) ON DELETE CASCADE,"
        "FOREIGN KEY (player_name) REFERENCES player_data(name) ON DELETE CASCADE"
        ")");
    
    if (mysql_query(conn, query)) {
        pubsub_error("Failed to create pubsub_subscriptions table: %s", mysql_error(conn));
        return PUBSUB_ERROR_DATABASE;
    }
    
    /* Create messages table */
    snprintf(query, sizeof(query),
        "CREATE TABLE IF NOT EXISTS pubsub_messages ("
        "message_id BIGINT AUTO_INCREMENT PRIMARY KEY,"
        "topic_id INT NOT NULL,"
        "sender_name VARCHAR(30),"
        "message_type VARCHAR(32) DEFAULT 'text',"
        "content TEXT,"
        "metadata TEXT,"
        "sent_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,"
        "expires_at TIMESTAMP NULL,"
        "INDEX idx_topic (topic_id),"
        "INDEX idx_sender (sender_name),"
        "INDEX idx_sent_at (sent_at),"
        "INDEX idx_expires_at (expires_at),"
        "FOREIGN KEY (topic_id) REFERENCES pubsub_topics(topic_id) ON DELETE CASCADE,"
        "FOREIGN KEY (sender_name) REFERENCES player_data(name) ON DELETE SET NULL"
        ")");
    
    if (mysql_query(conn, query)) {
        pubsub_error("Failed to create pubsub_messages table: %s", mysql_error(conn));
        return PUBSUB_ERROR_DATABASE;
    }
    
    /* Create player settings table */
    snprintf(query, sizeof(query),
        "CREATE TABLE IF NOT EXISTS pubsub_player_settings ("
        "setting_id INT AUTO_INCREMENT PRIMARY KEY,"
        "player_name VARCHAR(20) NOT NULL,"
        "max_subscriptions INT DEFAULT 50,"
        "default_handler VARCHAR(64) DEFAULT 'send_text',"
        "auto_subscribe_categories TEXT,"
        "notification_settings TEXT,"
        "INDEX idx_player (player_name),"
        "FOREIGN KEY (player_name) REFERENCES player_data(name) ON DELETE CASCADE"
        ")");
    
    if (mysql_query(conn, query)) {
        pubsub_error("Failed to create pubsub_player_settings table: %s", mysql_error(conn));
        return PUBSUB_ERROR_DATABASE;
    }
    
    /* Create handlers table */
    snprintf(query, sizeof(query),
        "CREATE TABLE IF NOT EXISTS pubsub_handlers ("
        "handler_id INT AUTO_INCREMENT PRIMARY KEY,"
        "handler_name VARCHAR(64) UNIQUE NOT NULL,"
        "description TEXT,"
        "is_enabled BOOLEAN DEFAULT TRUE,"
        "usage_count INT DEFAULT 0,"
        "INDEX idx_name (handler_name),"
        "INDEX idx_enabled (is_enabled)"
        ")");
    
    if (mysql_query(conn, query)) {
        pubsub_error("Failed to create pubsub_handlers table: %s", mysql_error(conn));
        return PUBSUB_ERROR_DATABASE;
    }
    
    /* Insert default handlers */
    snprintf(query, sizeof(query),
        "INSERT IGNORE INTO pubsub_handlers (handler_name, description) VALUES "
        "('send_text', 'Send plain text message to player'),"
        "('send_formatted', 'Send formatted message with color codes'),"
        "('spatial_audio', 'Process spatial audio events with distance'),"
        "('personal_message', 'Handle personal tell-style messages'),"
        "('system_announcement', 'Handle system-wide announcements')");
    
    if (mysql_query(conn, query)) {
        pubsub_error("Failed to insert default handlers: %s", mysql_error(conn));
        return PUBSUB_ERROR_DATABASE;
    }
    
    pubsub_info("PubSub database tables created successfully");
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
    snprintf(query, sizeof(query),
        "INSERT INTO pubsub_topics "
        "(name, description, category, access_type, creator_name) "
        "VALUES ('%s', %s, %d, %d, %s)",
        escaped_name,
        description ? escaped_desc : "NULL",
        category, access_type, creator_name ? escaped_creator : "NULL");
    
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
    
    return topic->topic_id;
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
    
    log("PUBSUB INFO: %s", buf);
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
        default:                              return "Unknown error";
    }
}
