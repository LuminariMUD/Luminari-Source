/*************************************************************************
*   File: pubsub.h                                     Part of LuminariMUD *
*  Usage: Header file for publish/subscribe messaging system              *
*  Author: Luminari Development Team                                       *
*                                                                          *
*  All rights reserved.  See license for complete information.            *
*                                                                          *
*  LuminariMUD is based on CircleMUD, Copyright (C) 1993, 94 by the       *
*  Department of Computer Science at the Johns Hopkins University         *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#ifndef _PUBSUB_H_
#define _PUBSUB_H_

#include "interpreter.h"

/* PubSub System Configuration */
#define PUBSUB_VERSION                  1
#define PUBSUB_DEVELOPMENT_MODE         0   /* Set to 0 for production to disable table drops */
#define PUBSUB_MAX_TOPIC_NAME_LENGTH    255
#define PUBSUB_MAX_HANDLER_NAME_LENGTH  64
#define PUBSUB_MAX_MESSAGE_LENGTH       8192
#define PUBSUB_MAX_PLAYER_NAME_LENGTH   30
#define PUBSUB_MAX_SUBSCRIPTIONS_DEFAULT 50
#define PUBSUB_DEFAULT_MESSAGE_TTL      3600
#define SUBSCRIPTION_CACHE_SIZE         1024
#define CACHE_TIMEOUT                   300

/* Message Queue Configuration */
#define PUBSUB_QUEUE_MAX_SIZE           10000
#define PUBSUB_QUEUE_BATCH_SIZE         50
#define PUBSUB_QUEUE_PROCESS_INTERVAL   100    /* milliseconds */
#define PUBSUB_QUEUE_MAX_RETRIES        3
#define PUBSUB_QUEUE_RETRY_DELAY        1000   /* milliseconds */
#define PUBSUB_QUEUE_THROTTLE_LIMIT     100    /* messages per second per player */

/* PubSub Error Codes */
#define PUBSUB_SUCCESS                  0
#define PUBSUB_ERROR_INVALID_PARAM      -1
#define PUBSUB_ERROR_DATABASE           -2
#define PUBSUB_ERROR_MEMORY             -3
#define PUBSUB_ERROR_NOT_FOUND          -4
#define PUBSUB_ERROR_DUPLICATE          -5
#define PUBSUB_ERROR_PERMISSION         -6
#define PUBSUB_ERROR_SUBSCRIPTION_LIMIT -7
#define PUBSUB_ERROR_TOPIC_FULL         -8
#define PUBSUB_ERROR_HANDLER_NOT_FOUND  -9
#define PUBSUB_ERROR_INVALID_MESSAGE    -10
#define PUBSUB_ERROR_QUEUE_INIT         -11
#define PUBSUB_ERROR_QUEUE_FULL         -12
#define PUBSUB_ERROR_QUEUE_DISABLED     -13
#define PUBSUB_ERROR_INVALID_PARAMETER  -14
#define PUBSUB_ERROR_QUEUE_INIT         -11
#define PUBSUB_ERROR_QUEUE_FULL         -12
#define PUBSUB_ERROR_QUEUE_DISABLED     -13
#define PUBSUB_ERROR_INVALID_PARAMETER  -14

/* Topic Categories */
#define PUBSUB_CATEGORY_GENERAL         0
#define PUBSUB_CATEGORY_GUILD           1
#define PUBSUB_CATEGORY_WILDERNESS      2
#define PUBSUB_CATEGORY_SYSTEM          3
#define PUBSUB_CATEGORY_PERSONAL        4
#define PUBSUB_CATEGORY_QUEST           5
#define PUBSUB_CATEGORY_AUCTION         6
#define PUBSUB_CATEGORY_CHAT            7

/* Access Types */
#define PUBSUB_ACCESS_PUBLIC            0
#define PUBSUB_ACCESS_GUILD_ONLY        1
#define PUBSUB_ACCESS_ADMIN_ONLY        2
#define PUBSUB_ACCESS_SUBSCRIBER_ONLY   3
#define PUBSUB_ACCESS_CREATOR_ONLY      4

/* Message Types (Enhanced V3) */
#define PUBSUB_MESSAGE_TYPE_SIMPLE         0   /* Basic text message */
#define PUBSUB_MESSAGE_TYPE_FORMATTED      1   /* Formatted content */
#define PUBSUB_MESSAGE_TYPE_SPATIAL        2   /* Spatial/location-based */
#define PUBSUB_MESSAGE_TYPE_SYSTEM         3   /* System notifications */
#define PUBSUB_MESSAGE_TYPE_PERSONAL       4   /* Personal messages */
#define PUBSUB_MESSAGE_TYPE_BROADCAST      5   /* Server-wide broadcasts */
#define PUBSUB_MESSAGE_TYPE_NOTIFICATION   6   /* Event notifications */
#define PUBSUB_MESSAGE_TYPE_ALERT          7   /* Important alerts */
#define PUBSUB_MESSAGE_TYPE_COMMAND        8   /* Command responses */
#define PUBSUB_MESSAGE_TYPE_STATUS         9   /* Status updates */

/* Message Categories (Enhanced V3) */
#define PUBSUB_MESSAGE_CATEGORY_COMMUNICATION  0   /* General communication */
#define PUBSUB_MESSAGE_CATEGORY_GAME_EVENT     1   /* Game events */
#define PUBSUB_MESSAGE_CATEGORY_SYSTEM_EVENT   2   /* System events */
#define PUBSUB_MESSAGE_CATEGORY_USER_ACTION    3   /* User actions */
#define PUBSUB_MESSAGE_CATEGORY_ENVIRONMENTAL  4   /* Environmental events */
#define PUBSUB_MESSAGE_CATEGORY_COMBAT         5   /* Combat-related */
#define PUBSUB_MESSAGE_CATEGORY_SOCIAL         6   /* Social interactions */
#define PUBSUB_MESSAGE_CATEGORY_ECONOMY        7   /* Economic activities */
#define PUBSUB_MESSAGE_CATEGORY_QUEST          8   /* Quest-related */
#define PUBSUB_MESSAGE_CATEGORY_GUILD          9   /* Guild activities */

/* Message Priorities */
#define PUBSUB_PRIORITY_LOW             1
#define PUBSUB_PRIORITY_NORMAL          2
#define PUBSUB_PRIORITY_HIGH            3
#define PUBSUB_PRIORITY_URGENT          4
#define PUBSUB_PRIORITY_CRITICAL        5

/* Subscription Status */
#define PUBSUB_STATUS_ACTIVE            0
#define PUBSUB_STATUS_PAUSED            1
#define PUBSUB_STATUS_DISABLED          2

/* Forward Declarations */
struct char_data;

/* PubSub Topic Structure */
struct pubsub_topic {
    int topic_id;
    char *name;
    char *description;
    int category;
    int access_type;
    int min_level;
    char *creator_name;
    time_t created_at;
    time_t last_message_at;
    int total_messages;
    int subscriber_count;
    int max_subscribers;
    int message_ttl;
    bool is_persistent;
    bool is_active;
    struct pubsub_topic *next;
};

/* PubSub Enhanced Metadata Structure */
struct pubsub_message_metadata {
    char *sender_real_name;     /* Real character name */
    char *sender_title;         /* Character title */
    int sender_level;           /* Character level */
    char *sender_class;         /* Character class */
    char *sender_race;          /* Character race */
    int origin_room;            /* Room where message originated */
    int origin_zone;            /* Zone where message originated */
    char *origin_area_name;     /* Area name where message originated */
    int origin_x, origin_y, origin_z; /* Coordinates */
    char *context_type;         /* Context (e.g., "combat", "roleplay") */
    char *trigger_event;        /* Event that triggered this message */
    char *related_object;       /* Related object type */
    int related_object_id;      /* Related object ID */
    char *handler_chain;        /* Handler processing chain */
    long processing_time_ms;    /* Processing time in milliseconds */
    char *processing_notes;     /* Processing notes or debug info */
};

/* PubSub Dynamic Fields Structure */
struct pubsub_message_fields {
    char **field_names;         /* Array of field names */
    char **field_values;        /* Array of field values */
    char **field_types;         /* Array of field types */
    int field_count;            /* Number of fields */
    int capacity;               /* Allocated capacity */
};

/* PubSub Subscription Structure */
struct pubsub_subscription {
    int subscription_id;
    int topic_id;
    char *player_name;
    char *handler_name;
    char *handler_data;
    int status;
    int priority;
    int min_message_priority;
    bool offline_delivery;
    int spatial_max_distance;
    time_t created_at;
    time_t last_delivered_at;
    int messages_received;
    struct pubsub_subscription *next;
};

/* PubSub Message Structure (V3 - Primary) */
struct pubsub_message {
    int message_id;
    int topic_id;
    char *sender_name;
    int sender_id;
    int message_type;        /* PUBSUB_MESSAGE_TYPE_* */
    int message_category;    /* PUBSUB_MESSAGE_CATEGORY_* */
    int priority;
    char *content;
    char *content_type;      /* MIME type */
    char *content_encoding;  /* encoding format */
    int content_version;     /* content format version */
    char *spatial_data;      /* JSON spatial information */
    struct pubsub_message_metadata *metadata_v3;  /* Enhanced metadata */
    struct pubsub_message_fields *fields;         /* Dynamic fields */
    char *metadata;          /* Legacy metadata for backward compatibility */
    time_t created_at;
    time_t expires_at;
    int parent_message_id;   /* For threaded conversations */
    int thread_id;           /* Thread identifier */
    int sequence_number;     /* Message sequence in thread */
    int delivery_attempts;
    int successful_deliveries;
    int failed_deliveries;
    bool is_processed;
    time_t processed_at;
    int reference_count;     /* Number of queue nodes referencing this message */
    struct pubsub_message *next;
};

/* PubSub Player Cache Structure */
struct pubsub_player_cache {
    char *player_name;
    int *subscribed_topics;
    int subscription_count;
    time_t last_cache_update;
    struct pubsub_player_cache *next;
};

/* PubSub Statistics Structure */
struct pubsub_statistics {
    long long total_topics;
    long long active_topics;
    long long total_subscriptions;
    long long active_subscriptions;
    long long total_messages_sent;
    long long total_messages_published;
    long long total_messages_delivered;
    long long total_messages_failed;
    int current_queue_size;
    int peak_queue_size;
    int topics_allocated;
    int messages_allocated;
    int subscriptions_allocated;
    /* Queue-specific statistics */
    long long queue_critical_processed;
    long long queue_urgent_processed;
    long long queue_high_processed;
    long long queue_normal_processed;
    long long queue_low_processed;
    long long queue_batch_operations;
    double avg_processing_time_ms;
    time_t last_queue_flush;
};

/* Message Queue Node Structure */
struct pubsub_queue_node {
    struct pubsub_message *message;
    struct char_data *target_player;
    char *handler_name;
    time_t queued_at;
    int retry_count;
    struct pubsub_queue_node *next;
};

/* Priority Message Queue Structure */
struct pubsub_message_queue {
    struct pubsub_queue_node *critical_head;
    struct pubsub_queue_node *critical_tail;
    struct pubsub_queue_node *urgent_head;
    struct pubsub_queue_node *urgent_tail;
    struct pubsub_queue_node *high_head;
    struct pubsub_queue_node *high_tail;
    struct pubsub_queue_node *normal_head;
    struct pubsub_queue_node *normal_tail;
    struct pubsub_queue_node *low_head;
    struct pubsub_queue_node *low_tail;
    int total_queued;
    int critical_count;
    int urgent_count;
    int high_count;
    int normal_count;
    int low_count;
    bool processing_active;
    time_t last_processed;
};

/* PubSub Handler Function Type */
typedef int (*pubsub_handler_func)(struct char_data *ch, struct pubsub_message *msg);

/* PubSub Handler Structure */
struct pubsub_handler {
    char *name;
    char *description;
    pubsub_handler_func func;
    bool is_enabled;
    int usage_count;
    struct pubsub_handler *next;
};

/* Spatial Message Data Structure */
struct pubsub_spatial_data {
    int world_x;
    int world_y;
    int world_z;
    int max_distance;
    int room_vnum;
    int zone_vnum;
};

/* Global Variables */
extern struct pubsub_topic *topic_list;
extern struct pubsub_handler *handler_list;
extern struct pubsub_player_cache *subscription_cache[];
extern struct pubsub_statistics pubsub_stats;
extern struct pubsub_message_queue message_queue;
extern bool pubsub_system_enabled;
extern bool pubsub_queue_processing;

/* Core API Functions */
int pubsub_init(void);
void pubsub_shutdown(void);
int pubsub_db_drop_tables(void);
int pubsub_db_create_tables(void);

/* Database function declarations */
int pubsub_db_create_tables(void);
int pubsub_db_populate_data(void);
int pubsub_db_save_message(struct pubsub_message *msg);
int pubsub_db_save_metadata(int message_id, struct pubsub_message_metadata *metadata);
int pubsub_db_save_fields(int message_id, struct pubsub_message_fields *fields);

/* Message Management & Validation */
bool pubsub_is_valid_message_type(int message_type);
bool pubsub_is_valid_message_category(int message_category);
bool pubsub_is_valid_type_category_combo(int message_type, int message_category);
int pubsub_get_recommended_category(int message_type);
struct pubsub_message *pubsub_create_message(int topic_id, const char *sender_name, 
                                            const char *content, int message_type, 
                                            int message_category, int priority);
void pubsub_free_message(struct pubsub_message *msg);

/* Topic Management */
int pubsub_create_topic(const char *name, const char *description, 
                       int category, int access_type, const char *creator_name);
int pubsub_delete_topic(int topic_id, const char *deleter_name);
int pubsub_delete_topic_by_name(const char *name, const char *deleter_name);
struct pubsub_topic *pubsub_find_topic_by_name(const char *name);
struct pubsub_topic *pubsub_find_topic_by_id(int topic_id);
int pubsub_topic_set_property(int topic_id, const char *property, const char *value);

/* Subscription Management */
int pubsub_subscribe(struct char_data *ch, int topic_id, const char *handler);
int pubsub_unsubscribe(struct char_data *ch, int topic_id);
int pubsub_unsubscribe_all(struct char_data *ch);
bool pubsub_is_subscribed(const char *player_name, int topic_id);
int pubsub_get_player_subscription_count(const char *player_name);
int pubsub_get_max_subscriptions(const char *player_name);

/* Message Publishing */
int pubsub_publish(int topic_id, const char *sender_name, const char *content, 
                  int message_type, int priority);
int pubsub_publish_spatial(int topic_id, const char *sender_name, const char *content,
                          int world_x, int world_y, int max_distance);
int pubsub_publish_wilderness_audio(int source_x, int source_y, int source_z,
                                   const char *sender_name, const char *content,
                                   int max_distance, int priority);
int pubsub_publish_to_subscribers(struct pubsub_message *msg);

/* Message Processing */
void pubsub_process_message_queue(void);
int pubsub_deliver_message(struct char_data *ch, struct pubsub_message *msg);

/* Message Queue Management */
int pubsub_queue_init(void);
void pubsub_queue_shutdown(void);
int pubsub_queue_message(struct pubsub_message *msg, struct char_data *target, 
                        const char *handler_name);
int pubsub_queue_process_all(void);
int pubsub_queue_process_priority(int priority);
int pubsub_queue_process_batch(int max_messages);
void pubsub_queue_cleanup_expired(void);
bool pubsub_queue_is_full(void);
int pubsub_queue_get_size(void);
int pubsub_queue_get_priority_count(int priority);
void pubsub_queue_start_processing(void);
void pubsub_queue_stop_processing(void);

/* Handler Management */
int pubsub_register_handler(const char *name, const char *description, 
                           pubsub_handler_func func);
struct pubsub_handler *pubsub_find_handler(const char *name);
int pubsub_call_handler(struct char_data *ch, struct pubsub_message *msg, 
                       const char *handler_name);

/* Cache Management */
void pubsub_cache_player_subscriptions(const char *player_name);
void pubsub_invalidate_player_cache(const char *player_name);
void pubsub_cleanup_cache(void);

/* Database Operations */
int pubsub_db_save_topic(struct pubsub_topic *topic);
int pubsub_db_load_topics(void);
int pubsub_db_save_subscription(struct pubsub_subscription *sub);
int pubsub_db_load_player_subscriptions(const char *player_name);
int pubsub_db_save_message(struct pubsub_message *msg);
int pubsub_db_cleanup_expired_messages(void);

/* Utility Functions */
void pubsub_log(const char *format, ...);
void pubsub_info(const char *format, ...);
void pubsub_error(const char *format, ...);
const char *pubsub_error_string(int error_code);
bool pubsub_validate_topic_name(const char *name);
bool pubsub_validate_handler_name(const char *name);

/* Player Commands - declared where implemented */

/* Admin Commands - declared where implemented */

/* Player Interface Functions */
void pubsub_list_topics_for_player(struct char_data *ch);
int pubsub_send_message(const char *topic_name, const char *sender_name, 
                       const char *content, int message_type, int category);

/* Built-in Message Handlers */
int pubsub_handler_send_text(struct char_data *ch, struct pubsub_message *msg);
int pubsub_handler_send_formatted(struct char_data *ch, struct pubsub_message *msg);
int pubsub_handler_spatial_audio(struct char_data *ch, struct pubsub_message *msg);
int pubsub_handler_personal_message(struct char_data *ch, struct pubsub_message *msg);
int pubsub_handler_system_announcement(struct char_data *ch, struct pubsub_message *msg);

/* Enhanced Spatial Audio Handlers (Phase 2B) */
int pubsub_handler_wilderness_spatial_audio(struct char_data *ch, struct pubsub_message *msg);
int pubsub_handler_audio_mixing(struct char_data *ch, struct pubsub_message *msg);
void pubsub_spatial_cleanup(void);

/* Event-Driven Code Handlers */
int pubsub_handler_combat_logger(struct char_data *ch, struct pubsub_message *msg);
int pubsub_handler_death_processor(struct char_data *ch, struct pubsub_message *msg);
int pubsub_handler_levelup_processor(struct char_data *ch, struct pubsub_message *msg);
int pubsub_handler_resource_monitor(struct char_data *ch, struct pubsub_message *msg);
int pubsub_handler_spell_effects(struct char_data *ch, struct pubsub_message *msg);
int pubsub_handler_guild_monitor(struct char_data *ch, struct pubsub_message *msg);

/* Event System Functions */
void pubsub_init_event_handlers(void);
void pubsub_trigger_event(const char *event_topic, struct char_data *ch, 
                         const char *event_data, int priority);

/* Memory Management Macros */
#define PUBSUB_CREATE_TOPIC()        ((struct pubsub_topic *)calloc(1, sizeof(struct pubsub_topic)))
#define PUBSUB_CREATE_SUBSCRIPTION() ((struct pubsub_subscription *)calloc(1, sizeof(struct pubsub_subscription)))
#define PUBSUB_CREATE_MESSAGE()      ((struct pubsub_message *)calloc(1, sizeof(struct pubsub_message)))
#define PUBSUB_CREATE_HANDLER()      ((struct pubsub_handler *)calloc(1, sizeof(struct pubsub_handler)))

#define PUBSUB_FREE_TOPIC(t)         do { if (t) { \
                                        if ((t)->name) free((t)->name); \
                                        if ((t)->description) free((t)->description); \
                                        free(t); \
                                     } } while(0)

#define PUBSUB_FREE_MESSAGE(m)       do { if (m) { \
                                        if ((m)->sender_name) free((m)->sender_name); \
                                        if ((m)->content) free((m)->content); \
                                        if ((m)->metadata) free((m)->metadata); \
                                        if ((m)->spatial_data) free((m)->spatial_data); \
                                        free(m); \
                                     } } while(0)

/* Debug Macros */
#ifdef PUBSUB_DEBUG
#define pubsub_debug(fmt, ...) pubsub_log("DEBUG: " fmt, ##__VA_ARGS__)
#else
#define pubsub_debug(fmt, ...)
#endif

/* Command Function Declarations */
ACMD_DECL(do_pubsub);
ACMD_DECL(do_subscribe);
ACMD_DECL(do_topics);
ACMD_DECL(do_pubsubtopic);
ACMD_DECL(do_pubsubqueue);

#endif /* _PUBSUB_H_ */
