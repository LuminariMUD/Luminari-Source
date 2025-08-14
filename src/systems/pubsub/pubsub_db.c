/*************************************************************************
*   File: pubsub_db.c                                  Part of LuminariMUD *
*  Usage: Database operations for PubSub system                           *
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

/* External variables */
extern MYSQL *conn;
extern struct pubsub_topic *topic_list;
extern struct pubsub_player_cache *subscription_cache[];
extern struct pubsub_statistics pubsub_stats;

/*
 * Load all topics from database into memory
 */
int pubsub_db_load_topics(void) {
    MYSQL_RES *result;
    MYSQL_ROW row;
    char query[MAX_STRING_LENGTH];
    struct pubsub_topic *topic;
    int count = 0;
    
    pubsub_info("Loading topics from database...");
    
    if (!conn) {
        pubsub_error("Database connection not available");
        return PUBSUB_ERROR_DATABASE;
    }
    
    /* Query all active topics */
    snprintf(query, sizeof(query),
        "SELECT topic_id, topic_name, description, created_by, "
        "UNIX_TIMESTAMP(created_at), subscriber_count, message_count, is_active "
        "FROM pubsub_topics WHERE is_active = 1 ORDER BY topic_id");
    
    if (mysql_query(conn, query)) {
        pubsub_error("Failed to load topics: %s", mysql_error(conn));
        return PUBSUB_ERROR_DATABASE;
    }
    
    result = mysql_store_result(conn);
    if (!result) {
        pubsub_error("No result for topics query: %s", mysql_error(conn));
        return PUBSUB_ERROR_DATABASE;
    }
    
    /* Process each topic row */
    while ((row = mysql_fetch_row(result))) {
        topic = PUBSUB_CREATE_TOPIC();
        if (!topic) {
            pubsub_error("Failed to allocate memory for topic");
            mysql_free_result(result);
            return PUBSUB_ERROR_MEMORY;
        }
        
        /* Parse topic data */
        topic->topic_id = atoi(row[0]);
        topic->name = strdup(row[1]);
        topic->description = row[2] ? strdup(row[2]) : NULL;
        topic->creator_name = row[3] ? strdup(row[3]) : NULL;
        topic->created_at = row[4] ? atol(row[4]) : 0;
        topic->subscriber_count = atoi(row[5]);
        topic->total_messages = atoi(row[6]);
        topic->is_active = atoi(row[7]) ? TRUE : FALSE;
        
        /* Set default values for fields not in database */
        topic->category = PUBSUB_CATEGORY_GENERAL;
        topic->access_type = PUBSUB_ACCESS_PUBLIC;
        topic->min_level = 1;
        topic->last_message_at = 0;
        topic->max_subscribers = 0; /* unlimited */
        topic->message_ttl = 0; /* no TTL */
        topic->is_persistent = TRUE;
        
        /* Add to topic list */
        topic->next = topic_list;
        topic_list = topic;
        
        count++;
        pubsub_debug("Loaded topic: %s (ID: %d)", topic->name, topic->topic_id);
    }
    
    mysql_free_result(result);
    
    /* Update statistics */
    pubsub_stats.total_topics = count;
    pubsub_stats.active_topics = count;
    
    pubsub_info("Loaded %d topics from database", count);
    return PUBSUB_SUCCESS;
}

/*
 * Save topic to database
 */
int pubsub_db_save_topic(struct pubsub_topic *topic) {
    char query[MAX_STRING_LENGTH];
    char escaped_name[512];
    char escaped_desc[1024];
    
    if (!topic || !topic->name) {
        return PUBSUB_ERROR_INVALID_PARAM;
    }
    
    if (!conn) {
        return PUBSUB_ERROR_DATABASE;
    }
    
    /* Escape strings for SQL safety */
    mysql_real_escape_string(conn, escaped_name, topic->name, strlen(topic->name));
    if (topic->description) {
        mysql_real_escape_string(conn, escaped_desc, topic->description, strlen(topic->description));
    }
    
    /* Update existing topic */
    snprintf(query, sizeof(query),
        "UPDATE pubsub_topics SET "
        "name = '%s', description = %s, category = %d, access_type = %d, "
        "min_level = %d, total_messages = %d, subscriber_count = %d, "
        "max_subscribers = %d, message_ttl = %d, is_persistent = %d, "
        "is_active = %d WHERE topic_id = %d",
        escaped_name,
        topic->description ? escaped_desc : "NULL",
        topic->category, topic->access_type, topic->min_level,
        topic->total_messages, topic->subscriber_count, topic->max_subscribers,
        topic->message_ttl, topic->is_persistent ? 1 : 0,
        topic->is_active ? 1 : 0, topic->topic_id);
    
    if (mysql_query(conn, query)) {
        pubsub_error("Failed to save topic %d: %s", topic->topic_id, mysql_error(conn));
        return PUBSUB_ERROR_DATABASE;
    }
    
    pubsub_debug("Saved topic: %s (ID: %d)", topic->name, topic->topic_id);
    return PUBSUB_SUCCESS;
}

/*
 * Load player subscriptions and cache them
 */
void pubsub_cache_player_subscriptions(const char *player_name) {
    MYSQL_RES *result;
    MYSQL_ROW row;
    char query[MAX_STRING_LENGTH];
    char escaped_name[64];
    struct pubsub_player_cache *cache;
    int hash, i;
    
    if (!player_name) {
        pubsub_error("Invalid player name for caching");
        return;
    }
    
    /* Remove existing cache entry */
    pubsub_invalidate_player_cache(player_name);
    
    if (!conn) {
        pubsub_error("Database connection not available for caching");
        return;
    }
    
    mysql_real_escape_string(conn, escaped_name, player_name, strlen(player_name));
    
    /* Query player's active subscriptions */
    snprintf(query, sizeof(query),
        "SELECT s.topic_id, s.handler_name, s.handler_data, s.status, "
        "s.priority, t.name "
        "FROM pubsub_subscriptions s "
        "JOIN pubsub_topics t ON s.topic_id = t.topic_id "
        "WHERE s.player_name = '%s' AND s.status = 0 AND t.is_active = 1 "
        "ORDER BY s.topic_id", escaped_name);
    
    if (mysql_query(conn, query)) {
        pubsub_error("Failed to load subscriptions for player %s: %s", 
                    player_name, mysql_error(conn));
        return;
    }
    
    result = mysql_store_result(conn);
    if (!result) {
        pubsub_error("No result for subscription query: %s", mysql_error(conn));
        return;
    }
    
    /* Create cache entry */
    CREATE(cache, struct pubsub_player_cache, 1);
    cache->player_name = strdup(player_name);
    cache->subscription_count = mysql_num_rows(result);
    cache->last_cache_update = time(NULL);
    cache->subscribed_topics = NULL;
    
    /* Allocate and populate topic array */
    if (cache->subscription_count > 0) {
        CREATE(cache->subscribed_topics, int, cache->subscription_count);
        
        i = 0;
        while ((row = mysql_fetch_row(result))) {
            cache->subscribed_topics[i] = atoi(row[0]);
            i++;
            pubsub_debug("Cached subscription: player %s -> topic %s", 
                        player_name, row[5]);
        }
    }
    
    mysql_free_result(result);
    
    /* Add to cache hash table - use string hash */
    hash = 0;
    for (i = 0; player_name[i]; i++) {
        hash = (hash * 31 + player_name[i]) % SUBSCRIPTION_CACHE_SIZE;
    }
    cache->next = subscription_cache[hash];
    subscription_cache[hash] = cache;
    
    pubsub_debug("Cached %d subscriptions for player %s", 
                cache->subscription_count, player_name);
}

/*
 * Check if player is subscribed to topic (with caching)
 */
bool pubsub_is_subscribed(const char *player_name, int topic_id) {
    struct pubsub_player_cache *cache;
    int hash, i;
    
    if (!player_name || topic_id <= 0) {
        return FALSE;
    }
    
    /* Calculate hash for player name */
    hash = 0;
    for (i = 0; player_name[i]; i++) {
        hash = (hash * 31 + player_name[i]) % SUBSCRIPTION_CACHE_SIZE;
    }
    
    /* Find player in cache */
    for (cache = subscription_cache[hash]; cache; cache = cache->next) {
        if (cache->player_name && strcmp(cache->player_name, player_name) == 0) {
            /* Check if cache is stale */
            if (time(NULL) - cache->last_cache_update > CACHE_TIMEOUT) {
                pubsub_cache_player_subscriptions(player_name);
                return pubsub_is_subscribed(player_name, topic_id);
            }
            
            /* Search cached topic list */
            for (i = 0; i < cache->subscription_count; i++) {
                if (cache->subscribed_topics[i] == topic_id) {
                    return TRUE;
                }
            }
            return FALSE;
        }
    }
    
    /* Not in cache, load from database */
    pubsub_cache_player_subscriptions(player_name);
    
    /* Try again with fresh cache */
    for (cache = subscription_cache[hash]; cache; cache = cache->next) {
        if (cache->player_name && strcmp(cache->player_name, player_name) == 0) {
            for (i = 0; i < cache->subscription_count; i++) {
                if (cache->subscribed_topics[i] == topic_id) {
                    return TRUE;
                }
            }
            return FALSE;
        }
    }
    
    return FALSE; /* Player has no subscriptions */
}

/*
 * Get player's subscription count
 */
int pubsub_get_player_subscription_count(const char *player_name) {
    struct pubsub_player_cache *cache;
    int hash, i;
    char query[MAX_STRING_LENGTH];
    char escaped_name[64];
    MYSQL_RES *result;
    MYSQL_ROW row;
    int count = 0;
    
    if (!player_name) {
        return 0;
    }
    
    /* Calculate hash for player name */
    hash = 0;
    for (i = 0; player_name[i]; i++) {
        hash = (hash * 31 + player_name[i]) % SUBSCRIPTION_CACHE_SIZE;
    }
    
    /* Check cache first */
    for (cache = subscription_cache[hash]; cache; cache = cache->next) {
        if (cache->player_name && strcmp(cache->player_name, player_name) == 0) {
            /* Check if cache is fresh */
            if (time(NULL) - cache->last_cache_update <= CACHE_TIMEOUT) {
                return cache->subscription_count;
            }
            break;
        }
    }
    
    /* Cache miss or stale, query database */
    if (!conn) {
        return 0;
    }
    
    mysql_real_escape_string(conn, escaped_name, player_name, strlen(player_name));
    
    snprintf(query, sizeof(query),
        "SELECT COUNT(*) FROM pubsub_subscriptions "
        "WHERE player_name = '%s' AND status = 0", escaped_name);
    
    if (mysql_query(conn, query)) {
        pubsub_error("Failed to get subscription count for player %s: %s",
                    player_name, mysql_error(conn));
        return 0;
    }
    
    result = mysql_store_result(conn);
    if (result && (row = mysql_fetch_row(result))) {
        count = atoi(row[0]);
    }
    
    if (result) {
        mysql_free_result(result);
    }
    
    return count;
}

/*
 * Get player's maximum allowed subscriptions
 */
int pubsub_get_max_subscriptions(const char *player_name) {
    char query[MAX_STRING_LENGTH];
    char escaped_name[64];
    MYSQL_RES *result;
    MYSQL_ROW row;
    int max_subs = PUBSUB_MAX_SUBSCRIPTIONS_DEFAULT;
    
    if (!player_name || !conn) {
        return PUBSUB_MAX_SUBSCRIPTIONS_DEFAULT;
    }
    
    mysql_real_escape_string(conn, escaped_name, player_name, strlen(player_name));
    
    /* Check player settings table */
    snprintf(query, sizeof(query),
        "SELECT max_subscriptions FROM pubsub_player_settings "
        "WHERE player_name = '%s'", escaped_name);
    
    if (mysql_query(conn, query)) {
        pubsub_debug("Failed to get max subscriptions for player %s: %s",
                    player_name, mysql_error(conn));
        return PUBSUB_MAX_SUBSCRIPTIONS_DEFAULT;
    }
    
    result = mysql_store_result(conn);
    if (result && (row = mysql_fetch_row(result))) {
        max_subs = atoi(row[0]);
    }
    
    if (result) {
        mysql_free_result(result);
    }
    
    return max_subs > 0 ? max_subs : PUBSUB_MAX_SUBSCRIPTIONS_DEFAULT;
}

/*
 * Invalidate player's subscription cache
 */
void pubsub_invalidate_player_cache(const char *player_name) {
    struct pubsub_player_cache *cache, *prev;
    int hash, i;
    
    if (!player_name) {
        return;
    }
    
    /* Calculate hash for player name */
    hash = 0;
    for (i = 0; player_name[i]; i++) {
        hash = (hash * 31 + player_name[i]) % SUBSCRIPTION_CACHE_SIZE;
    }
    
    prev = NULL;
    
    for (cache = subscription_cache[hash]; cache; cache = cache->next) {
        if (cache->player_name && strcmp(cache->player_name, player_name) == 0) {
            /* Remove from cache */
            if (prev) {
                prev->next = cache->next;
            } else {
                subscription_cache[hash] = cache->next;
            }
            
            /* Free memory */
            if (cache->subscribed_topics) {
                free(cache->subscribed_topics);
            }
            if (cache->player_name) {
                free(cache->player_name);
            }
            free(cache);
            
            pubsub_debug("Invalidated cache for player %s", player_name);
            return;
        }
        prev = cache;
    }
}

/*
 * Clean up entire subscription cache
 */
void pubsub_cleanup_cache(void) {
    struct pubsub_player_cache *cache, *next;
    int i, cleaned = 0;
    
    pubsub_info("Cleaning up subscription cache...");
    
    for (i = 0; i < SUBSCRIPTION_CACHE_SIZE; i++) {
        for (cache = subscription_cache[i]; cache; cache = next) {
            next = cache->next;
            
            if (cache->subscribed_topics) {
                free(cache->subscribed_topics);
            }
            free(cache);
            cleaned++;
        }
        subscription_cache[i] = NULL;
    }
    
    pubsub_info("Cleaned up %d cache entries", cleaned);
}

/*
 * V3 Database Schema Creation and Initialization (Phase 3.1.4)
 */

/*
 * Clean up expired messages from database
 */
int pubsub_db_cleanup_expired_messages(void) {
    char query[MAX_STRING_LENGTH];
    int affected;
    
    if (!conn) {
        return PUBSUB_ERROR_DATABASE;
    }
    
    /* Delete expired and processed messages */
    snprintf(query, sizeof(query),
        "DELETE FROM pubsub_messages "
        "WHERE (expires_at IS NOT NULL AND expires_at < NOW()) "
        "OR (is_processed = 1 AND processed_at < DATE_SUB(NOW(), INTERVAL 1 DAY))");
    
    if (mysql_query(conn, query)) {
        pubsub_error("Failed to cleanup expired messages: %s", mysql_error(conn));
        return PUBSUB_ERROR_DATABASE;
    }
    
    affected = mysql_affected_rows(conn);
    if (affected > 0) {
        pubsub_info("Cleaned up %d expired messages", affected);
    }
    
    return affected;
}

/*
 * V3 Database Schema Creation and Initialization (Phase 3.1.4)
 */

/* Create database tables for PubSub system */
int pubsub_db_create_tables(void)
{
    char query[MAX_STRING_LENGTH];
    
    if (!conn) {
        pubsub_error("Database connection not available");
        return PUBSUB_ERROR_DATABASE;
    }
    
    pubsub_info("Creating PubSub database tables...");
    
    /* Create legacy pubsub_topics table */
    snprintf(query, sizeof(query),
        "CREATE TABLE IF NOT EXISTS pubsub_topics ("
        "topic_id INT PRIMARY KEY AUTO_INCREMENT, "
        "topic_name VARCHAR(128) NOT NULL UNIQUE, "
        "description TEXT, "
        "created_by VARCHAR(80), "
        "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP, "
        "is_active BOOLEAN DEFAULT TRUE, "
        "subscriber_count INT DEFAULT 0, "
        "message_count INT DEFAULT 0, "
        "INDEX idx_active (is_active), "
        "INDEX idx_created_at (created_at)"
        ")");
    
    if (mysql_query(conn, query)) {
        pubsub_error("Failed to create pubsub_topics table: %s", mysql_error(conn));
        return PUBSUB_ERROR_DATABASE;
    }
    
    /* Create legacy pubsub_subscriptions table */
    snprintf(query, sizeof(query),
        "CREATE TABLE IF NOT EXISTS pubsub_subscriptions ("
        "subscription_id INT PRIMARY KEY AUTO_INCREMENT, "
        "topic_id INT NOT NULL, "
        "player_name VARCHAR(80) NOT NULL, "
        "subscribed_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP, "
        "is_active BOOLEAN DEFAULT TRUE, "
        "UNIQUE KEY unique_subscription (topic_id, player_name), "
        "FOREIGN KEY (topic_id) REFERENCES pubsub_topics(topic_id) ON DELETE CASCADE, "
        "INDEX idx_player (player_name), "
        "INDEX idx_active (is_active)"
        ")");
    
    if (mysql_query(conn, query)) {
        pubsub_error("Failed to create pubsub_subscriptions table: %s", mysql_error(conn));
        return PUBSUB_ERROR_DATABASE;
    }
    
    /* Create enhanced pubsub_messages table */
    snprintf(query, sizeof(query),
        "CREATE TABLE IF NOT EXISTS pubsub_messages ("
        "message_id INT PRIMARY KEY AUTO_INCREMENT, "
        "topic_id INT NOT NULL, "
        "sender_name VARCHAR(80) NOT NULL, "
        "sender_id BIGINT, "
        "message_type INT NOT NULL DEFAULT 0, "
        "message_category INT NOT NULL DEFAULT 1, "
        "priority INT NOT NULL DEFAULT 2, "
        "content TEXT, "
        "content_type VARCHAR(64) DEFAULT 'text/plain', "
        "content_encoding VARCHAR(32) DEFAULT 'utf-8', "
        "content_version INT DEFAULT 1, "
        "spatial_data TEXT, "
        "legacy_metadata TEXT, "
        "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP, "
        "expires_at TIMESTAMP NULL, "
        "last_modified_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP, "
        "parent_message_id INT NULL, "
        "thread_id INT NULL, "
        "sequence_number INT DEFAULT 0, "
        "delivery_attempts INT DEFAULT 0, "
        "successful_deliveries INT DEFAULT 0, "
        "failed_deliveries INT DEFAULT 0, "
        "is_processed BOOLEAN DEFAULT FALSE, "
        "processed_at TIMESTAMP NULL, "
        "reference_count INT DEFAULT 1, "
        "FOREIGN KEY (topic_id) REFERENCES pubsub_topics(topic_id) ON DELETE CASCADE, "
        "FOREIGN KEY (parent_message_id) REFERENCES pubsub_messages(message_id) ON DELETE SET NULL, "
        "INDEX idx_topic_created (topic_id, created_at), "
        "INDEX idx_type_category (message_type, message_category), "
        "INDEX idx_thread (thread_id, sequence_number), "
        "INDEX idx_parent (parent_message_id), "
        "INDEX idx_processed (is_processed), "
        "INDEX idx_expires (expires_at)"
        ")");

    if (mysql_query(conn, query)) {
        pubsub_error("Failed to create pubsub_messages table: %s", mysql_error(conn));
        return PUBSUB_ERROR_DATABASE;
    }    /* Create enhanced message metadata table */
    snprintf(query, sizeof(query),
        "CREATE TABLE IF NOT EXISTS pubsub_message_metadata ("
        "metadata_id INT AUTO_INCREMENT PRIMARY KEY, "
        "message_id INT NOT NULL, "
        "sender_real_name VARCHAR(100), "
        "sender_title VARCHAR(255), "
        "sender_level INT, "
        "sender_class VARCHAR(50), "
        "sender_race VARCHAR(50), "
        "origin_room INT, "
        "origin_zone INT, "
        "origin_area_name VARCHAR(255), "
        "origin_x INT DEFAULT 0, "
        "origin_y INT DEFAULT 0, "
        "origin_z INT DEFAULT 0, "
        "context_type VARCHAR(100), "
        "trigger_event VARCHAR(100), "
        "related_object VARCHAR(100), "
        "related_object_id INT, "
        "handler_chain TEXT, "
        "processing_time_ms BIGINT DEFAULT 0, "
        "processing_notes TEXT, "
        "FOREIGN KEY (message_id) REFERENCES pubsub_messages(message_id) ON DELETE CASCADE, "
        "INDEX idx_message_id (message_id), "
        "INDEX idx_origin_location (origin_x, origin_y, origin_z)"
        ") ENGINE=InnoDB");

    if (mysql_query(conn, query)) {
        pubsub_error("Failed to create pubsub_message_metadata table: %s", mysql_error(conn));
        return PUBSUB_ERROR_DATABASE;
    }

    /* Create enhanced message fields table */
    snprintf(query, sizeof(query),
        "CREATE TABLE IF NOT EXISTS pubsub_message_fields ("
        "field_id INT AUTO_INCREMENT PRIMARY KEY, "
        "message_id INT NOT NULL, "
        "field_name VARCHAR(128) NOT NULL, "
        "field_value TEXT, "
        "field_type VARCHAR(32) DEFAULT 'string', "
        "field_order INT DEFAULT 0, "
        "FOREIGN KEY (message_id) REFERENCES pubsub_messages(message_id) ON DELETE CASCADE, "
        "INDEX idx_message_field (message_id, field_name), "
        "INDEX idx_field_type (field_type)"
        ") ENGINE=InnoDB");

    if (mysql_query(conn, query)) {
        pubsub_error("Failed to create pubsub_message_fields table: %s", mysql_error(conn));
        return PUBSUB_ERROR_DATABASE;
    }

    pubsub_info("Enhanced database tables created successfully");
    return PUBSUB_SUCCESS;
}

/* Create V3 enhanced database tables */
int pubsub_db_create_v3_tables(void)
{
    char query[MAX_STRING_LENGTH];
    
    if (!conn) {
        pubsub_error("Database connection not available for V3 tables");
        return PUBSUB_ERROR_DATABASE;
    }
    
    pubsub_info("Creating PubSub V3 enhanced database tables...");
    
    /* Create V3 enhanced messages table */
    snprintf(query, sizeof(query),
        "CREATE TABLE IF NOT EXISTS pubsub_messages_v3 ("
        "message_id INT PRIMARY KEY AUTO_INCREMENT, "
        "topic_id INT NOT NULL, "
        "sender_name VARCHAR(80) NOT NULL, "
        "sender_id BIGINT, "
        "message_type INT NOT NULL DEFAULT 0, "
        "message_category INT NOT NULL DEFAULT 1, "
        "priority INT NOT NULL DEFAULT 2, "
        "content TEXT, "
        "content_type VARCHAR(64) DEFAULT 'text/plain', "
        "content_encoding VARCHAR(32) DEFAULT 'utf-8', "
        "content_version INT DEFAULT 1, "
        "spatial_data TEXT, "
        "legacy_metadata TEXT, "
        "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP, "
        "expires_at TIMESTAMP NULL, "
        "last_modified_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP, "
        "parent_message_id INT NULL, "
        "thread_id INT NULL, "
        "sequence_number INT DEFAULT 0, "
        "delivery_attempts INT DEFAULT 0, "
        "successful_deliveries INT DEFAULT 0, "
        "failed_deliveries INT DEFAULT 0, "
        "is_processed BOOLEAN DEFAULT FALSE, "
        "processed_at TIMESTAMP NULL, "
        "reference_count INT DEFAULT 1, "
        "FOREIGN KEY (topic_id) REFERENCES pubsub_topics(topic_id) ON DELETE CASCADE, "
        "FOREIGN KEY (parent_message_id) REFERENCES pubsub_messages_v3(message_id) ON DELETE SET NULL, "
        "INDEX idx_topic_created (topic_id, created_at), "
        "INDEX idx_type_category (message_type, message_category), "
        "INDEX idx_thread (thread_id, sequence_number), "
        "INDEX idx_parent (parent_message_id), "
        "INDEX idx_processed (is_processed), "
        "INDEX idx_expires (expires_at)"
        ")");
    
    if (mysql_query(conn, query)) {
        pubsub_error("Failed to create pubsub_messages_v3 table: %s", mysql_error(conn));
        return PUBSUB_ERROR_DATABASE;
    }
    
    /* Create V3 message metadata table */
    snprintf(query, sizeof(query),
        "CREATE TABLE IF NOT EXISTS pubsub_message_metadata_v3 ("
        "metadata_id INT PRIMARY KEY AUTO_INCREMENT, "
        "message_id INT NOT NULL, "
        "sender_real_name VARCHAR(80), "
        "sender_title VARCHAR(255), "
        "sender_level INT, "
        "sender_class VARCHAR(64), "
        "sender_race VARCHAR(64), "
        "origin_room INT, "
        "origin_zone INT, "
        "origin_area_name VARCHAR(128), "
        "origin_x INT, "
        "origin_y INT, "
        "origin_z INT, "
        "context_type VARCHAR(64), "
        "trigger_event VARCHAR(128), "
        "related_object VARCHAR(128), "
        "related_object_id INT, "
        "handler_chain TEXT, "
        "processing_time_ms INT DEFAULT 0, "
        "processing_notes TEXT, "
        "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP, "
        "last_updated TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP, "
        "FOREIGN KEY (message_id) REFERENCES pubsub_messages_v3(message_id) ON DELETE CASCADE, "
        "UNIQUE KEY unique_message_metadata (message_id), "
        "INDEX idx_sender (sender_real_name), "
        "INDEX idx_origin (origin_room, origin_zone), "
        "INDEX idx_context (context_type)"
        ")");
    
    if (mysql_query(conn, query)) {
        pubsub_error("Failed to create pubsub_message_metadata_v3 table: %s", mysql_error(conn));
        return PUBSUB_ERROR_DATABASE;
    }
    
    /* Create V3 custom fields table */
    snprintf(query, sizeof(query),
        "CREATE TABLE IF NOT EXISTS pubsub_message_fields_v3 ("
        "field_id INT PRIMARY KEY AUTO_INCREMENT, "
        "message_id INT NOT NULL, "
        "field_name VARCHAR(128) NOT NULL, "
        "field_type INT NOT NULL, "
        "string_value TEXT, "
        "integer_value BIGINT, "
        "float_value DOUBLE, "
        "boolean_value BOOLEAN, "
        "timestamp_value TIMESTAMP NULL, "
        "json_value JSON, "
        "player_ref_value BIGINT, "
        "location_ref_room INT, "
        "location_ref_zone INT, "
        "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP, "
        "FOREIGN KEY (message_id) REFERENCES pubsub_messages_v3(message_id) ON DELETE CASCADE, "
        "INDEX idx_message_field (message_id, field_name), "
        "INDEX idx_field_type (field_type), "
        "INDEX idx_string_value (string_value(255)), "
        "INDEX idx_integer_value (integer_value)"
        ")");
    
    if (mysql_query(conn, query)) {
        pubsub_error("Failed to create pubsub_message_fields_v3 table: %s", mysql_error(conn));
        return PUBSUB_ERROR_DATABASE;
    }
    
    /* Create V3 tags table */
    snprintf(query, sizeof(query),
        "CREATE TABLE IF NOT EXISTS pubsub_message_tags_v3 ("
        "tag_id INT PRIMARY KEY AUTO_INCREMENT, "
        "message_id INT NOT NULL, "
        "tag_category VARCHAR(64) NOT NULL, "
        "tag_name VARCHAR(128) NOT NULL, "
        "tag_value VARCHAR(255), "
        "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP, "
        "FOREIGN KEY (message_id) REFERENCES pubsub_messages_v3(message_id) ON DELETE CASCADE, "
        "INDEX idx_message_tag (message_id, tag_name), "
        "INDEX idx_category_tag (tag_category, tag_name), "
        "INDEX idx_tag_value (tag_value)"
        ")");
    
    if (mysql_query(conn, query)) {
        pubsub_error("Failed to create pubsub_message_tags_v3 table: %s", mysql_error(conn));
        return PUBSUB_ERROR_DATABASE;
    }
    
    pubsub_info("PubSub V3 database tables created successfully");
    
    /* Populate base data */
    return pubsub_db_populate_data();
}

/* Populate base data for unified system */
int pubsub_db_populate_data(void)
{
    char query[MAX_STRING_LENGTH];
    
    pubsub_info("Populating PubSub base data...");
    
    /* Insert default topics if they don't exist */
    const char *default_topics[][3] = {
        {"system", "System announcements and notifications", "System"},
        {"general", "General discussion and communication", "System"},
        {"spatial", "Spatial audio and environmental sounds", "System"},
        {"debug", "Debug and development messages", "System"},
        {"combat", "Combat notifications and events", "System"},
        {"quest", "Quest and adventure notifications", "System"},
        {"weather", "Weather and environmental updates", "System"},
        {"economy", "Trading and economic information", "System"},
        {"social", "Social interactions and group activities", "System"},
        {NULL, NULL, NULL}
    };
    int i;
    
    for (i = 0; default_topics[i][0] != NULL; i++) {
        snprintf(query, sizeof(query),
            "INSERT IGNORE INTO pubsub_topics "
            "(topic_name, description, created_by, is_active) "
            "VALUES ('%s', '%s', '%s', TRUE)",
            default_topics[i][0], default_topics[i][1], default_topics[i][2]);
        
        if (mysql_query(conn, query)) {
            pubsub_error("Failed to insert default topic '%s': %s", 
                        default_topics[i][0], mysql_error(conn));
            /* Continue with other topics */
        }
    }
    
    pubsub_info("PubSub base data populated successfully");
    return PUBSUB_SUCCESS;
}

/* Save V3 message to database */
int pubsub_db_save_message(struct pubsub_message *msg)
{
    char query[4096];
    char escaped_sender[512];
    char escaped_content[2048] = "";
    
    if (!msg || !conn) {
        return PUBSUB_ERROR_INVALID_PARAM;
    }
    
    /* Escape strings */
    mysql_real_escape_string(conn, escaped_sender, msg->sender_name, strlen(msg->sender_name));
    if (msg->content) {
        mysql_real_escape_string(conn, escaped_content, msg->content, strlen(msg->content));
    }
    
    /* Build query with proper NULL handling */
    char parent_id_str[32], thread_id_str[32];
    
    if (msg->parent_message_id > 0) {
        snprintf(parent_id_str, sizeof(parent_id_str), "%d", msg->parent_message_id);
    } else {
        strcpy(parent_id_str, "NULL");
    }
    
    if (msg->thread_id > 0) {
        snprintf(thread_id_str, sizeof(thread_id_str), "%d", msg->thread_id);
    } else {
        strcpy(thread_id_str, "NULL");
    }
    
    snprintf(query, sizeof(query),
        "INSERT INTO pubsub_messages "
        "(topic_id, sender_name, sender_id, message_type, message_category, "
        "priority, content, content_type, content_encoding, content_version, "
        "spatial_data, parent_message_id, "
        "thread_id, sequence_number) "
        "VALUES (%d, '%s', %d, %d, %d, %d, '%s', '%s', '%s', %d, %s, %s, %s, %d)",
        msg->topic_id, escaped_sender, msg->sender_id, msg->message_type, 
        msg->message_category, msg->priority, 
        msg->content ? escaped_content : "",
        msg->content_type ? msg->content_type : "text/plain",
        msg->content_encoding ? msg->content_encoding : "utf-8",
        msg->content_version,
        msg->spatial_data ? msg->spatial_data : "NULL",
        parent_id_str,
        thread_id_str,
        msg->sequence_number);
    
    if (mysql_query(conn, query)) {
        pubsub_error("Failed to save message: %s", mysql_error(conn));
        return PUBSUB_ERROR_DATABASE;
    }
    
    msg->message_id = mysql_insert_id(conn);
    
    /* Save metadata if present */
    if (msg->metadata_v3) {
        if (pubsub_db_save_metadata(msg->message_id, msg->metadata_v3) != PUBSUB_SUCCESS) {
            pubsub_error("Failed to save V3 metadata for message %d", msg->message_id);
            return PUBSUB_ERROR_DATABASE;
        }
    }
    
    /* Save fields if present */
    if (msg->fields) {
        if (pubsub_db_save_fields(msg->message_id, msg->fields) != PUBSUB_SUCCESS) {
            pubsub_error("Failed to save V3 fields for message %d", msg->message_id);
            return PUBSUB_ERROR_DATABASE;
        }
    }
    
    return PUBSUB_SUCCESS;
}

/* Save V3 metadata to database */
int pubsub_db_save_metadata(int message_id, struct pubsub_message_metadata *metadata)
{
    char query[4096];
    char temp_str[512];
    
    if (!metadata || !conn) {
        return PUBSUB_ERROR_INVALID_PARAM;
    }
    
    /* Build the INSERT query dynamically */
    snprintf(query, sizeof(query),
        "INSERT INTO pubsub_message_metadata "
        "(message_id, sender_real_name, sender_title, sender_level, "
        "sender_class, sender_race, origin_room, origin_zone, origin_area_name, "
        "origin_x, origin_y, origin_z, context_type, trigger_event, "
        "related_object, related_object_id, handler_chain, processing_time_ms, "
        "processing_notes) VALUES (%d, ", message_id);
    
    /* sender_real_name */
    if (metadata->sender_real_name) {
        mysql_real_escape_string(conn, temp_str, metadata->sender_real_name, strlen(metadata->sender_real_name));
        strcat(query, "'");
        strcat(query, temp_str);
        strcat(query, "'");
    } else {
        strcat(query, "NULL");
    }
    strcat(query, ", ");
    
    /* sender_title */
    if (metadata->sender_title) {
        mysql_real_escape_string(conn, temp_str, metadata->sender_title, strlen(metadata->sender_title));
        strcat(query, "'");
        strcat(query, temp_str);
        strcat(query, "'");
    } else {
        strcat(query, "NULL");
    }
    
    /* Complete the rest of the query */
    snprintf(temp_str, sizeof(temp_str), ", %d, ", metadata->sender_level);
    strcat(query, temp_str);
    
    /* sender_class */
    if (metadata->sender_class) {
        char escaped_class[256];
        mysql_real_escape_string(conn, escaped_class, metadata->sender_class, strlen(metadata->sender_class));
        strcat(query, "'");
        strcat(query, escaped_class);
        strcat(query, "'");
    } else {
        strcat(query, "NULL");
    }
    strcat(query, ", ");
    
    /* sender_race */
    if (metadata->sender_race) {
        char escaped_race[256];
        mysql_real_escape_string(conn, escaped_race, metadata->sender_race, strlen(metadata->sender_race));
        strcat(query, "'");
        strcat(query, escaped_race);
        strcat(query, "'");
    } else {
        strcat(query, "NULL");
    }
    
    /* Add numeric fields */
    snprintf(temp_str, sizeof(temp_str), ", %d, %d, ", metadata->origin_room, metadata->origin_zone);
    strcat(query, temp_str);
    
    /* origin_area_name */
    if (metadata->origin_area_name) {
        char escaped_area[256];
        mysql_real_escape_string(conn, escaped_area, metadata->origin_area_name, strlen(metadata->origin_area_name));
        strcat(query, "'");
        strcat(query, escaped_area);
        strcat(query, "'");
    } else {
        strcat(query, "NULL");
    }
    
    /* Add coordinates and remaining fields */
    snprintf(temp_str, sizeof(temp_str), ", %d, %d, %d, ", metadata->origin_x, metadata->origin_y, metadata->origin_z);
    strcat(query, temp_str);
    
    /* context_type */
    if (metadata->context_type) {
        char escaped_context[256];
        mysql_real_escape_string(conn, escaped_context, metadata->context_type, strlen(metadata->context_type));
        strcat(query, "'");
        strcat(query, escaped_context);
        strcat(query, "'");
    } else {
        strcat(query, "NULL");
    }
    strcat(query, ", ");
    
    /* trigger_event */
    if (metadata->trigger_event) {
        char escaped_trigger[256];
        mysql_real_escape_string(conn, escaped_trigger, metadata->trigger_event, strlen(metadata->trigger_event));
        strcat(query, "'");
        strcat(query, escaped_trigger);
        strcat(query, "'");
    } else {
        strcat(query, "NULL");
    }
    strcat(query, ", ");
    
    /* related_object */
    if (metadata->related_object) {
        char escaped_object[256];
        mysql_real_escape_string(conn, escaped_object, metadata->related_object, strlen(metadata->related_object));
        strcat(query, "'");
        strcat(query, escaped_object);
        strcat(query, "'");
    } else {
        strcat(query, "NULL");
    }
    
    /* Add remaining numeric fields */
    snprintf(temp_str, sizeof(temp_str), ", %d, ", metadata->related_object_id);
    strcat(query, temp_str);
    
    /* handler_chain */
    if (metadata->handler_chain) {
        char escaped_chain[256];
        mysql_real_escape_string(conn, escaped_chain, metadata->handler_chain, strlen(metadata->handler_chain));
        strcat(query, "'");
        strcat(query, escaped_chain);
        strcat(query, "'");
    } else {
        strcat(query, "NULL");
    }
    
    snprintf(temp_str, sizeof(temp_str), ", %ld, ", metadata->processing_time_ms);
    strcat(query, temp_str);
    
    /* processing_notes */
    if (metadata->processing_notes) {
        char escaped_notes[256];
        mysql_real_escape_string(conn, escaped_notes, metadata->processing_notes, strlen(metadata->processing_notes));
        strcat(query, "'");
        strcat(query, escaped_notes);
        strcat(query, "'");
    } else {
        strcat(query, "NULL");
    }
    
    strcat(query, ")");
    
    if (mysql_query(conn, query)) {
        pubsub_error("Failed to save V3 metadata: %s", mysql_error(conn));
        return PUBSUB_ERROR_DATABASE;
    }
    
    return PUBSUB_SUCCESS;
}

/* Save V3 custom fields to database */
int pubsub_db_save_fields(int message_id, struct pubsub_message_fields *fields)
{
    /* TODO: Implement custom fields saving */
    /* This is a placeholder for the full fields implementation */
    return PUBSUB_SUCCESS;
}
