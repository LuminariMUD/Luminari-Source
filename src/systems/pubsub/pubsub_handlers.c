/*************************************************************************
*   File: pubsub_handlers.c                            Part of LuminariMUD *
*  Usage: Built-in message handlers for PubSub system                     *
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
#include "pubsub.h"

/*
 * Built-in handler: Send plain text message
 */
int pubsub_handler_send_text(struct char_data *ch, struct pubsub_message *msg) {
    if (!ch || !msg || !msg->content) {
        return PUBSUB_ERROR_INVALID_PARAM;
    }
    
    send_to_char(ch, "%s\r\n", msg->content);
    return PUBSUB_SUCCESS;
}

/*
 * Built-in handler: Send formatted message with color codes
 */
int pubsub_handler_send_formatted(struct char_data *ch, struct pubsub_message *msg) {
    char formatted_msg[MAX_STRING_LENGTH];
    
    if (!ch || !msg || !msg->content) {
        return PUBSUB_ERROR_INVALID_PARAM;
    }
    
    /* Add priority-based color coding */
    switch (msg->priority) {
        case PUBSUB_PRIORITY_CRITICAL:
            snprintf(formatted_msg, sizeof(formatted_msg), 
                    "%s[CRITICAL]%s %s%s\r\n", 
                    KRED, KNRM, msg->content, KNRM);
            break;
        case PUBSUB_PRIORITY_URGENT:
            snprintf(formatted_msg, sizeof(formatted_msg),
                    "%s[URGENT]%s %s%s\r\n",
                    KYEL, KNRM, msg->content, KNRM);
            break;
        case PUBSUB_PRIORITY_HIGH:
            snprintf(formatted_msg, sizeof(formatted_msg),
                    "%s[HIGH]%s %s%s\r\n",
                    KCYN, KNRM, msg->content, KNRM);
            break;
        case PUBSUB_PRIORITY_NORMAL:
            snprintf(formatted_msg, sizeof(formatted_msg),
                    "%s%s%s\r\n",
                    KGRN, msg->content, KNRM);
            break;
        case PUBSUB_PRIORITY_LOW:
        default:
            snprintf(formatted_msg, sizeof(formatted_msg),
                    "%s%s%s\r\n",
                    KBLU, msg->content, KNRM);
            break;
    }
    
    send_to_char(ch, "%s", formatted_msg);
    return PUBSUB_SUCCESS;
}

/*
 * Built-in handler: Spatial audio with distance calculation
 */
int pubsub_handler_spatial_audio(struct char_data *ch, struct pubsub_message *msg) {
    struct pubsub_spatial_data spatial;
    int player_x, player_y, distance;
    float volume_modifier;
    char spatial_msg[MAX_STRING_LENGTH];
    
    if (!ch || !msg || !msg->content) {
        return PUBSUB_ERROR_INVALID_PARAM;
    }
    
    /* Parse spatial data if available */
    if (msg->spatial_data) {
        if (sscanf(msg->spatial_data, "%d,%d,%d,%d", 
                  &spatial.world_x, &spatial.world_y, 
                  &spatial.world_z, &spatial.max_distance) != 4) {
            /* Fall back to regular text if spatial data is invalid */
            return pubsub_handler_send_text(ch, msg);
        }
    } else {
        /* No spatial data, send as regular message */
        return pubsub_handler_send_text(ch, msg);
    }
    
    /* Get player's current coordinates */
    player_x = X_LOC(ch);
    player_y = Y_LOC(ch);
    
    /* Calculate distance */
    distance = (int)sqrt(pow(player_x - spatial.world_x, 2) + 
                        pow(player_y - spatial.world_y, 2));
    
    /* Check if within hearing range */
    if (spatial.max_distance > 0 && distance > spatial.max_distance) {
        /* Too far away to hear */
        return PUBSUB_SUCCESS;
    }
    
    /* Calculate volume modifier based on distance */
    if (spatial.max_distance > 0) {
        volume_modifier = 1.0f - ((float)distance / (float)spatial.max_distance);
    } else {
        volume_modifier = 1.0f;
    }
    
    /* Format message based on distance */
    if (distance == 0) {
        /* Right at the source */
        snprintf(spatial_msg, sizeof(spatial_msg), 
                "%s%s%s\r\n", KRED, msg->content, KNRM);
    } else if (volume_modifier > 0.8f) {
        /* Very close */
        snprintf(spatial_msg, sizeof(spatial_msg),
                "You hear %s%s%s clearly.\r\n", 
                KYEL, msg->content, KNRM);
    } else if (volume_modifier > 0.5f) {
        /* Moderate distance */
        snprintf(spatial_msg, sizeof(spatial_msg),
                "You hear %s%s%s in the distance.\r\n",
                KCYN, msg->content, KNRM);
    } else if (volume_modifier > 0.2f) {
        /* Far away */
        snprintf(spatial_msg, sizeof(spatial_msg),
                "You faintly hear %s%s%s from far away.\r\n",
                KBLU, msg->content, KNRM);
    } else {
        /* Very faint */
        snprintf(spatial_msg, sizeof(spatial_msg),
                "You barely make out the sound of %s%s%s.\r\n",
                KBLK, msg->content, KNRM);
    }
    
    send_to_char(ch, "%s", spatial_msg);
    return PUBSUB_SUCCESS;
}

/*
 * Built-in handler: Personal message (tell-style)
 */
int pubsub_handler_personal_message(struct char_data *ch, struct pubsub_message *msg) {
    char personal_msg[MAX_STRING_LENGTH];
    
    if (!ch || !msg || !msg->content) {
        return PUBSUB_ERROR_INVALID_PARAM;
    }
    
    /* Format as personal message */
    if (msg->sender_name && strlen(msg->sender_name) > 0) {
        snprintf(personal_msg, sizeof(personal_msg),
                "%s[Personal Message from %s]%s %s%s\r\n",
                KMAG, msg->sender_name, KNRM, msg->content, KNRM);
    } else {
        snprintf(personal_msg, sizeof(personal_msg),
                "%s[Personal Message]%s %s%s\r\n",
                KMAG, KNRM, msg->content, KNRM);
    }
    
    send_to_char(ch, "%s", personal_msg);
    
    /* Add to player's message history if desired */
    /* This would integrate with any existing mail/message system */
    
    return PUBSUB_SUCCESS;
}

/*
 * Built-in handler: System announcement
 */
int pubsub_handler_system_announcement(struct char_data *ch, struct pubsub_message *msg) {
    char announce_msg[MAX_STRING_LENGTH];
    
    if (!ch || !msg || !msg->content) {
        return PUBSUB_ERROR_INVALID_PARAM;
    }
    
    /* Format as system announcement with timestamp */
    snprintf(announce_msg, sizeof(announce_msg),
            "\r\n%s*** SYSTEM ANNOUNCEMENT ***%s\r\n"
            "%s%s%s\r\n"
            "%s*************************%s\r\n\r\n",
            KYEL, KNRM, KGRN, msg->content, KNRM, KYEL, KNRM);
    
    send_to_char(ch, "%s", announce_msg);
    
    /* Could also trigger other system behaviors like:
     * - Adding to system log
     * - Saving to announcement history
     * - Triggering visual/audio effects
     */
    
    return PUBSUB_SUCCESS;
}
