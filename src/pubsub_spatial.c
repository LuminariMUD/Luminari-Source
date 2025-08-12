/*************************************************************************
*   File: pubsub_spatial.c                         Part of LuminariMUD  *
*  Usage: Advanced spatial audio system for PubSub (Phase 2B)           *
*  Author: Luminari Development Team                                      *
*                                                                         *
*  All rights reserved.  See license for complete information.           *
*                                                                         *
*  LuminariMUD is based on CircleMUD, Copyright (C) 1993, 94 by the      *
*  Department of Computer Science at the Johns Hopkins University        *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.              *
************************************************************************ */

#include "conf.h"
#include "sysdep.h"
#include <math.h>
#include <time.h>

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

/* Constants for spatial audio calculations */
#define SPATIAL_AUDIO_MAX_DISTANCE      50    /* Maximum hearing distance in wilderness */
#define SPATIAL_AUDIO_CLOSE_THRESHOLD   5     /* Very close audio threshold */
#define SPATIAL_AUDIO_MID_THRESHOLD     15    /* Moderate distance threshold */
#define SPATIAL_AUDIO_FAR_THRESHOLD     30    /* Far distance threshold */
#define SPATIAL_AUDIO_ROOM_RANGE        3     /* Max room-based audio range */

/* Terrain-based audio modifiers */
#define TERRAIN_FOREST_DAMPING          0.7f  /* Forest reduces audio range */
#define TERRAIN_MOUNTAIN_ECHO           1.3f  /* Mountains create echoes */
#define TERRAIN_WATER_CARRY             1.2f  /* Water carries sound */
#define TERRAIN_PLAINS_NORMAL           1.0f  /* Plains normal propagation */
#define TERRAIN_UNDERGROUND_MUFFLED     0.5f  /* Underground muffles sound */

/* Audio mixing limits */
#define MAX_CONCURRENT_AUDIO_SOURCES    5     /* Maximum simultaneous audio sources */
#define AUDIO_MIXING_BLEND_DISTANCE     10    /* Distance for audio source blending */

/* Audio source tracking structure */
struct spatial_audio_source {
    int source_x;
    int source_y;
    int source_z;
    char *content;
    char *source_name;
    int priority;
    float volume;
    time_t start_time;
    int duration;
    struct spatial_audio_source *next;
};

/* Global audio source tracking */
static struct spatial_audio_source *active_audio_sources = NULL;

/* Static function prototypes */
static float calculate_distance_3d(int x1, int y1, int z1, int x2, int y2, int z2);
static float calculate_terrain_modifier(int listener_x, int listener_y, int source_x, int source_y);
static float calculate_elevation_effect(int listener_x, int listener_y, int source_x, int source_y);
static bool has_line_of_sound(int listener_x, int listener_y, int source_x, int source_y);
static void add_audio_source(int x, int y, int z, const char *content, const char *source_name, 
                           int priority, float volume, int duration);
static void remove_expired_audio_sources(void);
static struct spatial_audio_source *get_nearby_audio_sources(int x, int y, int max_distance);
static char *blend_audio_sources(struct char_data *ch, struct spatial_audio_source *sources);
static float get_sector_audio_modifier(int sector_type);

/*
 * Enhanced wilderness spatial audio handler
 */
int pubsub_handler_wilderness_spatial_audio(struct char_data *ch, struct pubsub_message *msg) {
    struct pubsub_spatial_data spatial;
    int listener_x, listener_y, listener_z = 0;
    float distance_3d, volume_modifier, terrain_modifier, elevation_effect;
    char spatial_msg[MAX_STRING_LENGTH];
    
    if (!ch || !msg || !msg->content) {
        return PUBSUB_ERROR_INVALID_PARAM;
    }
    
    /* Check if player is in wilderness */
    if (!ZONE_FLAGGED(GET_ROOM_ZONE(IN_ROOM(ch)), ZONE_WILDERNESS)) {
        /* Not in wilderness, use regular spatial audio */
        return pubsub_handler_spatial_audio(ch, msg);
    }
    
    /* Parse spatial data */
    if (msg->spatial_data) {
        if (sscanf(msg->spatial_data, "%d,%d,%d,%d", 
                  &spatial.world_x, &spatial.world_y, 
                  &spatial.world_z, &spatial.max_distance) != 4) {
            return pubsub_handler_send_text(ch, msg);
        }
    } else {
        return pubsub_handler_send_text(ch, msg);
    }
    
    /* Get listener's wilderness coordinates */
    listener_x = X_LOC(ch);
    listener_y = Y_LOC(ch);
    /* For now, wilderness Z is elevation-based */
    listener_z = get_elevation(NOISE_MATERIAL_PLANE_ELEV, listener_x, listener_y);
    
    /* Calculate 3D distance */
    distance_3d = calculate_distance_3d(listener_x, listener_y, listener_z,
                                       spatial.world_x, spatial.world_y, spatial.world_z);
    
    /* Check if within maximum hearing range */
    int max_range = (spatial.max_distance > 0) ? spatial.max_distance : SPATIAL_AUDIO_MAX_DISTANCE;
    if (distance_3d > max_range) {
        return PUBSUB_SUCCESS; /* Too far to hear */
    }
    
    /* Calculate terrain-based audio modification */
    terrain_modifier = calculate_terrain_modifier(listener_x, listener_y, 
                                                 spatial.world_x, spatial.world_y);
    
    /* Calculate elevation effects */
    elevation_effect = calculate_elevation_effect(listener_x, listener_y,
                                                 spatial.world_x, spatial.world_y);
    
    /* Check line of sound (simplified - no mountains blocking) */
    if (!has_line_of_sound(listener_x, listener_y, spatial.world_x, spatial.world_y)) {
        /* Sound is blocked by terrain */
        volume_modifier = 0.1f; /* Heavily muffled */
        snprintf(spatial_msg, sizeof(spatial_msg),
                "You hear the distant, muffled sound of %s%s%s.\r\n",
                KBLK, msg->content, KNRM);
    } else {
        /* Calculate base volume modifier */
        volume_modifier = 1.0f - (distance_3d / (float)max_range);
        
        /* Apply terrain and elevation modifiers */
        volume_modifier *= terrain_modifier * elevation_effect;
        
        /* Clamp to valid range */
        if (volume_modifier < 0.0f) volume_modifier = 0.0f;
        if (volume_modifier > 1.0f) volume_modifier = 1.0f;
        
        /* Format message based on effective volume */
        if (volume_modifier > 0.9f) {
            /* Right at source or very close */
            snprintf(spatial_msg, sizeof(spatial_msg),
                    "%s%s%s echoes powerfully across the wilderness.\r\n",
                    KRED, msg->content, KNRM);
        } else if (volume_modifier > 0.7f) {
            /* Close */
            snprintf(spatial_msg, sizeof(spatial_msg),
                    "You clearly hear %s%s%s nearby.\r\n",
                    KYEL, msg->content, KNRM);
        } else if (volume_modifier > 0.5f) {
            /* Moderate distance */
            snprintf(spatial_msg, sizeof(spatial_msg),
                    "You hear %s%s%s carried on the wind.\r\n",
                    KCYN, msg->content, KNRM);
        } else if (volume_modifier > 0.3f) {
            /* Far */
            snprintf(spatial_msg, sizeof(spatial_msg),
                    "You hear %s%s%s faintly in the distance.\r\n",
                    KBLU, msg->content, KNRM);
        } else if (volume_modifier > 0.1f) {
            /* Very far */
            snprintf(spatial_msg, sizeof(spatial_msg),
                    "You barely make out %s%s%s from far away.\r\n",
                    KBLU, msg->content, KNRM);
        } else {
            /* Almost inaudible */
            snprintf(spatial_msg, sizeof(spatial_msg),
                    "You think you hear something that might be %s%s%s.\r\n",
                    KBLK, msg->content, KNRM);
        }
    }
    
    /* Add directional information */
    char direction_info[256];
    int dx = spatial.world_x - listener_x;
    int dy = spatial.world_y - listener_y;
    
    if (abs(dx) > 2 || abs(dy) > 2) { /* Only show direction if reasonably far */
        char direction[64] = "somewhere";
        
        if (abs(dx) > abs(dy)) {
            if (dx > 0) strcpy(direction, "to the east");
            else strcpy(direction, "to the west");
        } else {
            if (dy > 0) strcpy(direction, "to the north");
            else strcpy(direction, "to the south");
        }
        
        /* Add diagonal directions for more precision */
        if (abs(dx) > 5 && abs(dy) > 5) {
            if (dx > 0 && dy > 0) strcpy(direction, "to the northeast");
            else if (dx > 0 && dy < 0) strcpy(direction, "to the southeast");
            else if (dx < 0 && dy > 0) strcpy(direction, "to the northwest");
            else strcpy(direction, "to the southwest");
        }
        
        snprintf(direction_info, sizeof(direction_info),
                " The sound comes from %s%s%s.",
                KWHT, direction, KNRM);
        
        /* Append direction if there's room */
        if (strlen(spatial_msg) + strlen(direction_info) < sizeof(spatial_msg) - 10) {
            /* Remove the \r\n from spatial_msg and add direction */
            char *end = strstr(spatial_msg, "\r\n");
            if (end) {
                strcpy(end, direction_info);
                strcat(spatial_msg, "\r\n");
            }
        }
    }
    
    /* Add to active audio sources for mixing */
    add_audio_source(spatial.world_x, spatial.world_y, spatial.world_z,
                    msg->content, msg->sender_name ? msg->sender_name : "unknown",
                    msg->priority, volume_modifier, 30); /* 30 second duration */
    
    send_to_char(ch, "%s", spatial_msg);
    return PUBSUB_SUCCESS;
}

/*
 * Audio mixing handler - processes multiple simultaneous audio sources
 */
int pubsub_handler_audio_mixing(struct char_data *ch, struct pubsub_message *msg) {
    struct spatial_audio_source *nearby_sources;
    char *mixed_audio;
    
    if (!ch || !ZONE_FLAGGED(GET_ROOM_ZONE(IN_ROOM(ch)), ZONE_WILDERNESS)) {
        return pubsub_handler_wilderness_spatial_audio(ch, msg);
    }
    
    /* Clean up expired audio sources */
    remove_expired_audio_sources();
    
    /* Get nearby audio sources */
    nearby_sources = get_nearby_audio_sources(X_LOC(ch), Y_LOC(ch), SPATIAL_AUDIO_MAX_DISTANCE);
    
    if (!nearby_sources) {
        /* No other audio sources, process normally */
        return pubsub_handler_wilderness_spatial_audio(ch, msg);
    }
    
    /* Blend multiple audio sources */
    mixed_audio = blend_audio_sources(ch, nearby_sources);
    
    if (mixed_audio) {
        send_to_char(ch, "%s", mixed_audio);
        free(mixed_audio);
    }
    
    return PUBSUB_SUCCESS;
}

/*
 * Static helper functions
 */

static float calculate_distance_3d(int x1, int y1, int z1, int x2, int y2, int z2) {
    /* Weight Z distance less than X/Y for wilderness */
    int dx = x2 - x1;
    int dy = y2 - y1;
    int dz = (z2 - z1) / 4; /* Elevation differences are less significant */
    
    return sqrt(dx*dx + dy*dy + dz*dz);
}

static float calculate_terrain_modifier(int listener_x, int listener_y, int source_x, int source_y) {
    int listener_sector = get_modified_sector_type(real_zone(WILD_ZONE_VNUM), listener_x, listener_y);
    int source_sector = get_modified_sector_type(real_zone(WILD_ZONE_VNUM), source_x, source_y);
    
    float listener_mod = get_sector_audio_modifier(listener_sector);
    float source_mod = get_sector_audio_modifier(source_sector);
    
    /* Average the modifiers */
    return (listener_mod + source_mod) / 2.0f;
}

static float get_sector_audio_modifier(int sector_type) {
    switch (sector_type) {
        case SECT_FOREST:
            return TERRAIN_FOREST_DAMPING;
        case SECT_MOUNTAIN:
        case SECT_HILLS:
            return TERRAIN_MOUNTAIN_ECHO;
        case SECT_WATER_SWIM:
        case SECT_WATER_NOSWIM:
        case SECT_OCEAN:
            return TERRAIN_WATER_CARRY;
        case SECT_FIELD:
            return TERRAIN_PLAINS_NORMAL;
        case SECT_UD_WILD:
        case SECT_UD_NOGROUND:
            return TERRAIN_UNDERGROUND_MUFFLED;
        default:
            return TERRAIN_PLAINS_NORMAL;
    }
}

static float calculate_elevation_effect(int listener_x, int listener_y, int source_x, int source_y) {
    int listener_elev = get_elevation(NOISE_MATERIAL_PLANE_ELEV, listener_x, listener_y);
    int source_elev = get_elevation(NOISE_MATERIAL_PLANE_ELEV, source_x, source_y);
    
    int elevation_diff = abs(source_elev - listener_elev);
    
    /* Sound carries better downhill, worse uphill */
    if (source_elev > listener_elev) {
        /* Sound coming from higher elevation - carries better */
        return 1.0f + (elevation_diff * 0.01f); /* 1% bonus per elevation unit */
    } else if (source_elev < listener_elev) {
        /* Sound coming from lower elevation - reduced */
        return 1.0f - (elevation_diff * 0.005f); /* 0.5% penalty per elevation unit */
    } else {
        /* Same elevation */
        return 1.0f;
    }
}

static bool has_line_of_sound(int listener_x, int listener_y, int source_x, int source_y) {
    /* Simplified line-of-sound check - check for major elevation barriers */
    int steps = MAX(abs(source_x - listener_x), abs(source_y - listener_y));
    
    if (steps <= 1) return true; /* Adjacent or same location */
    
    int step_x = (source_x - listener_x) / steps;
    int step_y = (source_y - listener_y) / steps;
    
    int max_elevation = 0;
    int x, y, i;
    
    /* Find the highest elevation along the path */
    for (i = 0; i <= steps; i++) {
        x = listener_x + (step_x * i);
        y = listener_y + (step_y * i);
        
        int elev = get_elevation(NOISE_MATERIAL_PLANE_ELEV, x, y);
        if (elev > max_elevation) {
            max_elevation = elev;
        }
    }
    
    /* Check if there's a significant elevation barrier */
    int listener_elev = get_elevation(NOISE_MATERIAL_PLANE_ELEV, listener_x, listener_y);
    int source_elev = get_elevation(NOISE_MATERIAL_PLANE_ELEV, source_x, source_y);
    
    int barrier_height = max_elevation - MAX(listener_elev, source_elev);
    
    /* If barrier is more than 50 elevation units higher, sound is blocked */
    return (barrier_height < 50);
}

static void add_audio_source(int x, int y, int z, const char *content, const char *source_name,
                           int priority, float volume, int duration) {
    struct spatial_audio_source *source;
    
    CREATE(source, struct spatial_audio_source, 1);
    source->source_x = x;
    source->source_y = y;
    source->source_z = z;
    source->content = strdup(content);
    source->source_name = strdup(source_name);
    source->priority = priority;
    source->volume = volume;
    source->start_time = time(NULL);
    source->duration = duration;
    source->next = active_audio_sources;
    
    active_audio_sources = source;
}

static void remove_expired_audio_sources(void) {
    struct spatial_audio_source *source, *next, *prev = NULL;
    time_t now = time(NULL);
    
    for (source = active_audio_sources; source; source = next) {
        next = source->next;
        
        if (now - source->start_time > source->duration) {
            /* Remove expired source */
            if (prev) {
                prev->next = next;
            } else {
                active_audio_sources = next;
            }
            
            if (source->content) free(source->content);
            if (source->source_name) free(source->source_name);
            free(source);
        } else {
            prev = source;
        }
    }
}

static struct spatial_audio_source *get_nearby_audio_sources(int x, int y, int max_distance) {
    struct spatial_audio_source *source, *nearby = NULL, *new_source;
    float distance;
    
    for (source = active_audio_sources; source; source = source->next) {
        distance = calculate_distance_3d(x, y, 0, source->source_x, source->source_y, source->source_z);
        
        if (distance <= max_distance) {
            /* Create a copy for the nearby list */
            CREATE(new_source, struct spatial_audio_source, 1);
            *new_source = *source; /* Copy all fields */
            new_source->content = strdup(source->content);
            new_source->source_name = strdup(source->source_name);
            new_source->next = nearby;
            nearby = new_source;
        }
    }
    
    return nearby;
}

static char *blend_audio_sources(struct char_data *ch, struct spatial_audio_source *sources) {
    char *result;
    struct spatial_audio_source *source;
    int count = 0;
    
    /* Count sources and limit to maximum */
    for (source = sources; source && count < MAX_CONCURRENT_AUDIO_SOURCES; source = source->next) {
        count++;
    }
    
    if (count == 0) return NULL;
    
    CREATE(result, char, MAX_STRING_LENGTH);
    
    if (count == 1) {
        /* Single source, no mixing needed */
        snprintf(result, MAX_STRING_LENGTH,
                "You hear %s from %s.\r\n",
                sources->content, sources->source_name);
    } else {
        /* Multiple sources - create a blended message */
        snprintf(result, MAX_STRING_LENGTH,
                "You hear a mixture of sounds: ");
        
        int pos = strlen(result);
        for (source = sources; source && count > 0; source = source->next, count--) {
            if (pos + strlen(source->content) + 20 < MAX_STRING_LENGTH) {
                if (count > 1) {
                    pos += snprintf(result + pos, MAX_STRING_LENGTH - pos,
                                  "%s, ", source->content);
                } else {
                    pos += snprintf(result + pos, MAX_STRING_LENGTH - pos,
                                  "and %s", source->content);
                }
            }
        }
        
        strcat(result, ".\r\n");
    }
    
    return result;
}

/*
 * Cleanup function for spatial audio system
 */
void pubsub_spatial_cleanup(void) {
    struct spatial_audio_source *source, *next;
    
    for (source = active_audio_sources; source; source = next) {
        next = source->next;
        if (source->content) free(source->content);
        if (source->source_name) free(source->source_name);
        free(source);
    }
    
    active_audio_sources = NULL;
}
