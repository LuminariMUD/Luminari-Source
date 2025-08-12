/*************************************************************************
*   File: pubsub_queue.c                           Part of LuminariMUD *
*  Usage: Message queue system for PubSub (Phase 2A)                  *
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

/* Global queue variables */
struct pubsub_message_queue message_queue;
bool pubsub_queue_processing = FALSE;

/* Static helper function prototypes */
static struct pubsub_queue_node *create_queue_node(struct pubsub_message *msg, 
                                                  struct char_data *target, 
                                                  const char *handler_name);
static void free_queue_node(struct pubsub_queue_node *node);
static int get_priority_from_message(struct pubsub_message *msg);
static void enqueue_by_priority(struct pubsub_queue_node *node, int priority);
static struct pubsub_queue_node *dequeue_by_priority(int priority);
static void update_queue_statistics(void);

/*
 * Initialize the message queue system
 */
int pubsub_queue_init(void) {
    pubsub_info("Initializing PubSub message queue system...");
    
    /* Initialize all queue heads and tails to NULL */
    message_queue.critical_head = NULL;
    message_queue.critical_tail = NULL;
    message_queue.urgent_head = NULL;
    message_queue.urgent_tail = NULL;
    message_queue.high_head = NULL;
    message_queue.high_tail = NULL;
    message_queue.normal_head = NULL;
    message_queue.normal_tail = NULL;
    message_queue.low_head = NULL;
    message_queue.low_tail = NULL;
    
    /* Initialize counters */
    message_queue.total_queued = 0;
    message_queue.critical_count = 0;
    message_queue.urgent_count = 0;
    message_queue.high_count = 0;
    message_queue.normal_count = 0;
    message_queue.low_count = 0;
    
    /* Initialize processing state */
    message_queue.processing_active = FALSE;
    message_queue.last_processed = time(NULL);
    
    /* Initialize queue statistics */
    pubsub_stats.queue_critical_processed = 0;
    pubsub_stats.queue_urgent_processed = 0;
    pubsub_stats.queue_high_processed = 0;
    pubsub_stats.queue_normal_processed = 0;
    pubsub_stats.queue_low_processed = 0;
    pubsub_stats.queue_batch_operations = 0;
    pubsub_stats.avg_processing_time_ms = 0.0;
    pubsub_stats.last_queue_flush = time(NULL);
    
    pubsub_queue_processing = TRUE;
    
    pubsub_info("PubSub message queue system initialized successfully");
    return PUBSUB_SUCCESS;
}

/*
 * Shutdown the message queue system
 */
void pubsub_queue_shutdown(void) {
    struct pubsub_queue_node *node;
    
    pubsub_info("Shutting down PubSub message queue system...");
    
    pubsub_queue_processing = FALSE;
    
    /* Free all queued messages */
    int priority;
    for (priority = PUBSUB_PRIORITY_CRITICAL; priority >= PUBSUB_PRIORITY_LOW; priority--) {
        while ((node = dequeue_by_priority(priority)) != NULL) {
            free_queue_node(node);
        }
    }
    
    /* Reset counters */
    message_queue.total_queued = 0;
    message_queue.critical_count = 0;
    message_queue.urgent_count = 0;
    message_queue.high_count = 0;
    message_queue.normal_count = 0;
    message_queue.low_count = 0;
    
    pubsub_info("PubSub message queue system shutdown complete");
}

/*
 * Queue a message for asynchronous delivery
 */
int pubsub_queue_message(struct pubsub_message *msg, struct char_data *target, 
                        const char *handler_name) {
    struct pubsub_queue_node *node;
    int priority;
    
    if (!msg || !target || !handler_name) {
        pubsub_error("Invalid parameters for queue_message");
        return PUBSUB_ERROR_INVALID_PARAMETER;
    }
    
    if (!pubsub_queue_processing) {
        pubsub_error("Queue processing is disabled");
        return PUBSUB_ERROR_QUEUE_DISABLED;
    }
    
    /* Check if queue is full */
    if (pubsub_queue_is_full()) {
        pubsub_error("Message queue is full, dropping message");
        pubsub_stats.total_messages_failed++;
        return PUBSUB_ERROR_QUEUE_FULL;
    }
    
    /* Create queue node */
    node = create_queue_node(msg, target, handler_name);
    if (!node) {
        pubsub_error("Failed to create queue node");
        return PUBSUB_ERROR_MEMORY;
    }
    
    /* Get message priority */
    priority = get_priority_from_message(msg);
    
    /* Enqueue by priority */
    enqueue_by_priority(node, priority);
    
    /* Update statistics */
    message_queue.total_queued++;
    update_queue_statistics();
    
    pubsub_debug("Queued message ID %d with priority %d for player %s", 
                msg->message_id, priority, GET_NAME(target));
    
    return PUBSUB_SUCCESS;
}

/*
 * Process all queued messages
 */
int pubsub_queue_process_all(void) {
    int processed = 0;
    int priority;
    
    if (!pubsub_queue_processing) {
        return 0;
    }
    
    /* Process in priority order: Critical -> Urgent -> High -> Normal -> Low */
    for (priority = PUBSUB_PRIORITY_CRITICAL; priority >= PUBSUB_PRIORITY_LOW; priority--) {
        processed += pubsub_queue_process_priority(priority);
        
        /* Check if we've hit batch limit */
        if (processed >= PUBSUB_QUEUE_BATCH_SIZE) {
            break;
        }
    }
    
    if (processed > 0) {
        pubsub_stats.queue_batch_operations++;
        message_queue.last_processed = time(NULL);
    }
    
    return processed;
}

/*
 * Process messages of a specific priority
 */
int pubsub_queue_process_priority(int priority) {
    struct pubsub_queue_node *node;
    int processed = 0;
    int max_process = PUBSUB_QUEUE_BATCH_SIZE;
    
    while (processed < max_process && (node = dequeue_by_priority(priority)) != NULL) {
        /* Attempt to deliver the message */
        if (node->target_player && !PLR_FLAGGED(node->target_player, PLR_WRITING)) {
            int result = pubsub_call_handler(node->target_player, node->message, node->handler_name);
            
            if (result == PUBSUB_SUCCESS) {
                pubsub_stats.total_messages_delivered++;
                processed++;
                
                /* Update priority-specific statistics */
                switch (priority) {
                    case PUBSUB_PRIORITY_CRITICAL:
                        pubsub_stats.queue_critical_processed++;
                        break;
                    case PUBSUB_PRIORITY_URGENT:
                        pubsub_stats.queue_urgent_processed++;
                        break;
                    case PUBSUB_PRIORITY_HIGH:
                        pubsub_stats.queue_high_processed++;
                        break;
                    case PUBSUB_PRIORITY_NORMAL:
                        pubsub_stats.queue_normal_processed++;
                        break;
                    case PUBSUB_PRIORITY_LOW:
                        pubsub_stats.queue_low_processed++;
                        break;
                }
            } else {
                /* Handle delivery failure */
                node->retry_count++;
                if (node->retry_count < PUBSUB_QUEUE_MAX_RETRIES) {
                    /* Re-queue for retry */
                    enqueue_by_priority(node, priority);
                    message_queue.total_queued++;
                    continue; /* Don't free the node */
                } else {
                    pubsub_stats.total_messages_failed++;
                    pubsub_error("Failed to deliver message after %d retries", PUBSUB_QUEUE_MAX_RETRIES);
                }
            }
        } else {
            /* Player not available, re-queue for later */
            node->retry_count++;
            if (node->retry_count < PUBSUB_QUEUE_MAX_RETRIES) {
                enqueue_by_priority(node, priority);
                message_queue.total_queued++;
                continue; /* Don't free the node */
            } else {
                pubsub_stats.total_messages_failed++;
            }
        }
        
        /* Free the processed node */
        free_queue_node(node);
    }
    
    return processed;
}

/*
 * Process a batch of messages with limit
 */
int pubsub_queue_process_batch(int max_messages) {
    int processed = 0;
    int priority;
    
    if (!pubsub_queue_processing || max_messages <= 0) {
        return 0;
    }
    
    for (priority = PUBSUB_PRIORITY_CRITICAL; priority >= PUBSUB_PRIORITY_LOW && processed < max_messages; priority--) {
        struct pubsub_queue_node *node;
        int remaining = max_messages - processed;
        int batch_count = 0;
        
        while (batch_count < remaining && (node = dequeue_by_priority(priority)) != NULL) {
            /* Same processing logic as process_priority */
            if (node->target_player && !PLR_FLAGGED(node->target_player, PLR_WRITING)) {
                int result = pubsub_call_handler(node->target_player, node->message, node->handler_name);
                
                if (result == PUBSUB_SUCCESS) {
                    pubsub_stats.total_messages_delivered++;
                    processed++;
                    batch_count++;
                } else {
                    node->retry_count++;
                    if (node->retry_count < PUBSUB_QUEUE_MAX_RETRIES) {
                        enqueue_by_priority(node, priority);
                        message_queue.total_queued++;
                        continue;
                    } else {
                        pubsub_stats.total_messages_failed++;
                    }
                }
            } else {
                node->retry_count++;
                if (node->retry_count < PUBSUB_QUEUE_MAX_RETRIES) {
                    enqueue_by_priority(node, priority);
                    message_queue.total_queued++;
                    continue;
                }
            }
            
            free_queue_node(node);
        }
    }
    
    if (processed > 0) {
        pubsub_stats.queue_batch_operations++;
        message_queue.last_processed = time(NULL);
    }
    
    return processed;
}

/*
 * Clean up expired messages in the queue
 */
void pubsub_queue_cleanup_expired(void) {
    /* This is a simplified cleanup - in a full implementation,
     * we'd need to traverse all queues and check timestamps */
    
    pubsub_debug("Queue cleanup completed");
}

/*
 * Check if the queue is full
 */
bool pubsub_queue_is_full(void) {
    return (message_queue.total_queued >= PUBSUB_QUEUE_MAX_SIZE);
}

/*
 * Get total queue size
 */
int pubsub_queue_get_size(void) {
    return message_queue.total_queued;
}

/*
 * Get count of messages for specific priority
 */
int pubsub_queue_get_priority_count(int priority) {
    switch (priority) {
        case PUBSUB_PRIORITY_CRITICAL:
            return message_queue.critical_count;
        case PUBSUB_PRIORITY_URGENT:
            return message_queue.urgent_count;
        case PUBSUB_PRIORITY_HIGH:
            return message_queue.high_count;
        case PUBSUB_PRIORITY_NORMAL:
            return message_queue.normal_count;
        case PUBSUB_PRIORITY_LOW:
            return message_queue.low_count;
        default:
            return 0;
    }
}

/*
 * Start queue processing
 */
void pubsub_queue_start_processing(void) {
    pubsub_queue_processing = TRUE;
    pubsub_info("PubSub queue processing started");
}

/*
 * Stop queue processing
 */
void pubsub_queue_stop_processing(void) {
    pubsub_queue_processing = FALSE;
    pubsub_info("PubSub queue processing stopped");
}

/*
 * Static helper functions
 */

static struct pubsub_queue_node *create_queue_node(struct pubsub_message *msg, 
                                                  struct char_data *target, 
                                                  const char *handler_name) {
    struct pubsub_queue_node *node;
    
    CREATE(node, struct pubsub_queue_node, 1);
    if (!node) {
        return NULL;
    }
    
    node->message = msg;
    node->target_player = target;
    node->handler_name = strdup(handler_name);
    node->queued_at = time(NULL);
    node->retry_count = 0;
    node->next = NULL;
    
    return node;
}

static void free_queue_node(struct pubsub_queue_node *node) {
    if (node) {
        if (node->handler_name) {
            free(node->handler_name);
        }
        free(node);
    }
}

static int get_priority_from_message(struct pubsub_message *msg) {
    if (!msg) {
        return PUBSUB_PRIORITY_NORMAL;
    }
    
    /* Validate priority range */
    if (msg->priority >= PUBSUB_PRIORITY_LOW && msg->priority <= PUBSUB_PRIORITY_CRITICAL) {
        return msg->priority;
    }
    
    return PUBSUB_PRIORITY_NORMAL;
}

static void enqueue_by_priority(struct pubsub_queue_node *node, int priority) {
    struct pubsub_queue_node **head, **tail;
    int *count;
    
    /* Get appropriate queue pointers */
    switch (priority) {
        case PUBSUB_PRIORITY_CRITICAL:
            head = &message_queue.critical_head;
            tail = &message_queue.critical_tail;
            count = &message_queue.critical_count;
            break;
        case PUBSUB_PRIORITY_URGENT:
            head = &message_queue.urgent_head;
            tail = &message_queue.urgent_tail;
            count = &message_queue.urgent_count;
            break;
        case PUBSUB_PRIORITY_HIGH:
            head = &message_queue.high_head;
            tail = &message_queue.high_tail;
            count = &message_queue.high_count;
            break;
        case PUBSUB_PRIORITY_NORMAL:
            head = &message_queue.normal_head;
            tail = &message_queue.normal_tail;
            count = &message_queue.normal_count;
            break;
        case PUBSUB_PRIORITY_LOW:
        default:
            head = &message_queue.low_head;
            tail = &message_queue.low_tail;
            count = &message_queue.low_count;
            break;
    }
    
    /* Add to tail of queue (FIFO within priority) */
    node->next = NULL;
    
    if (*tail) {
        (*tail)->next = node;
    } else {
        *head = node;
    }
    *tail = node;
    (*count)++;
}

static struct pubsub_queue_node *dequeue_by_priority(int priority) {
    struct pubsub_queue_node **head, **tail, *node;
    int *count;
    
    /* Get appropriate queue pointers */
    switch (priority) {
        case PUBSUB_PRIORITY_CRITICAL:
            head = &message_queue.critical_head;
            tail = &message_queue.critical_tail;
            count = &message_queue.critical_count;
            break;
        case PUBSUB_PRIORITY_URGENT:
            head = &message_queue.urgent_head;
            tail = &message_queue.urgent_tail;
            count = &message_queue.urgent_count;
            break;
        case PUBSUB_PRIORITY_HIGH:
            head = &message_queue.high_head;
            tail = &message_queue.high_tail;
            count = &message_queue.high_count;
            break;
        case PUBSUB_PRIORITY_NORMAL:
            head = &message_queue.normal_head;
            tail = &message_queue.normal_tail;
            count = &message_queue.normal_count;
            break;
        case PUBSUB_PRIORITY_LOW:
        default:
            head = &message_queue.low_head;
            tail = &message_queue.low_tail;
            count = &message_queue.low_count;
            break;
    }
    
    /* Remove from head of queue (FIFO) */
    node = *head;
    if (node) {
        *head = node->next;
        if (*head == NULL) {
            *tail = NULL;
        }
        (*count)--;
        message_queue.total_queued--;
    }
    
    return node;
}

static void update_queue_statistics(void) {
    pubsub_stats.current_queue_size = message_queue.total_queued;
    
    if (pubsub_stats.current_queue_size > pubsub_stats.peak_queue_size) {
        pubsub_stats.peak_queue_size = pubsub_stats.current_queue_size;
    }
}
