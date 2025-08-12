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
        "SELECT topic_id, name, description, category, access_type, "
        "min_level, creator_name, UNIX_TIMESTAMP(created_at), "
        "UNIX_TIMESTAMP(last_message_at), total_messages, subscriber_count, "
        "max_subscribers, message_ttl, is_persistent, is_active "
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
        topic->category = atoi(row[3]);
        topic->access_type = atoi(row[4]);
        topic->min_level = atoi(row[5]);
        topic->creator_name = row[6] ? strdup(row[6]) : NULL;
        topic->created_at = row[7] ? atol(row[7]) : 0;
        topic->last_message_at = row[8] ? atol(row[8]) : 0;
        topic->total_messages = atoi(row[9]);
        topic->subscriber_count = atoi(row[10]);
        topic->max_subscribers = atoi(row[11]);
        topic->message_ttl = atoi(row[12]);
        topic->is_persistent = atoi(row[13]) ? TRUE : FALSE;
        topic->is_active = atoi(row[14]) ? TRUE : FALSE;
        
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
 * Save message to database for persistent delivery
 */
int pubsub_db_save_message(struct pubsub_message *msg) {
    char query[MAX_STRING_LENGTH * 4];  /* Increased buffer size */
    char escaped_content[MAX_STRING_LENGTH * 2]; /* Increased for escaping */
    char escaped_metadata[MAX_STRING_LENGTH];
    char escaped_spatial[512];
    
    if (!msg || !msg->content || !conn) {
        return PUBSUB_ERROR_INVALID_PARAM;
    }
    
    /* Escape strings for SQL safety */
    mysql_real_escape_string(conn, escaped_content, msg->content, strlen(msg->content));
    
    if (msg->metadata) {
        mysql_real_escape_string(conn, escaped_metadata, msg->metadata, strlen(msg->metadata));
    }
    
    if (msg->spatial_data) {
        mysql_real_escape_string(conn, escaped_spatial, msg->spatial_data, strlen(msg->spatial_data));
    }
    
    /* Insert message record */
    snprintf(query, sizeof(query),
        "INSERT INTO pubsub_messages "
        "(topic_id, sender_name, message_type, priority, content, metadata, "
        "spatial_data, expires_at) "
        "VALUES (%d, %s, %d, %d, '%s', %s, %s, "
        "DATE_ADD(NOW(), INTERVAL %d SECOND))",
        msg->topic_id, msg->sender_name ? msg->sender_name : "NULL", msg->message_type, msg->priority,
        escaped_content,
        msg->metadata ? "NULL" : "NULL",
        msg->spatial_data ? "NULL" : "NULL",
        msg->expires_at > 0 ? (int)(msg->expires_at - time(NULL)) : PUBSUB_DEFAULT_MESSAGE_TTL);
    
    if (mysql_query(conn, query)) {
        pubsub_error("Failed to save message: %s", mysql_error(conn));
        return PUBSUB_ERROR_DATABASE;
    }
    
    msg->message_id = mysql_insert_id(conn);
    pubsub_debug("Saved message ID %d to database", msg->message_id);
    
    return PUBSUB_SUCCESS;
}

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
