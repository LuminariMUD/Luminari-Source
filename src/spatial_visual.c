/*************************************************************************
*   File: spatial_visual.c                             Part of LuminariMUD *
*  Usage: Visual stimulus strategy implementation                          *
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
#include "wilderness.h"
#include "spatial_core.h"

/* Visual System Constants */
#define VISUAL_BASE_RANGE                 1000.0f
#define VISUAL_CLARITY_EXCELLENT          1.0f
#define VISUAL_CLARITY_GOOD               0.8f
#define VISUAL_CLARITY_POOR               0.5f
#define VISUAL_CLARITY_TERRIBLE           0.2f

/* Visual message types */
typedef enum {
    VISUAL_MSG_CLEAR,       /* Perfect visibility */
    VISUAL_MSG_DISTANT,     /* Far away but clear */
    VISUAL_MSG_OBSCURED,    /* Partially blocked */
    VISUAL_MSG_SILHOUETTE,  /* Only outline visible */
    VISUAL_MSG_GLIMPSE      /* Barely visible */
} visual_message_type_t;

/* Forward Declarations */
static int visual_calculate_intensity(struct spatial_context *ctx);
static int visual_generate_message(struct spatial_context *ctx, char *output, size_t max_len);
static int visual_apply_effects(struct spatial_context *ctx);
static bool visual_should_process_observer(struct spatial_context *ctx);

static int physical_calculate_obstruction(struct spatial_context *ctx, float *obstruction_factor);
static int physical_get_obstacles(struct spatial_context *ctx, struct obstacle_list *obstacles);
static bool physical_can_transmit(int terrain_type, int stimulus_type);

static int weather_terrain_apply_modifiers(struct spatial_context *ctx, float *range_mod, float *clarity_mod);
static int weather_terrain_calculate_interference(struct spatial_context *ctx, float *interference);
static int weather_terrain_modify_message(struct spatial_context *ctx, char *message, size_t max_len);

/* VISUAL STIMULUS STRATEGY */
struct stimulus_strategy visual_stimulus_strategy = {
    .name = "Visual",
    .stimulus_type = STIMULUS_VISUAL,
    .base_range = VISUAL_BASE_RANGE,
    .calculate_intensity = visual_calculate_intensity,
    .generate_base_message = visual_generate_message,
    .apply_stimulus_effects = visual_apply_effects,
    .should_process_observer = visual_should_process_observer,
    .enabled = TRUE,
    .usage_count = 0,
    .performance_factor = 1.0f
};

/* PHYSICAL LINE OF SIGHT STRATEGY */
struct los_strategy physical_los_strategy = {
    .name = "Physical",
    .supported_stimulus_types = (1 << STIMULUS_VISUAL) | (1 << STIMULUS_AUDIO),
    .calculate_obstruction = physical_calculate_obstruction,
    .get_blocking_elements = physical_get_obstacles,
    .can_transmit_through = physical_can_transmit,
    .enabled = TRUE,
    .use_caching = TRUE,
    .cache_hits = 0,
    .cache_misses = 0
};

/* WEATHER/TERRAIN MODIFIER STRATEGY */
struct modifier_strategy weather_terrain_modifier_strategy = {
    .name = "Weather_Terrain",
    .applicable_stimulus_types = (1 << STIMULUS_VISUAL) | (1 << STIMULUS_AUDIO),
    .apply_environmental_modifiers = weather_terrain_apply_modifiers,
    .calculate_interference = weather_terrain_calculate_interference,
    .modify_message = weather_terrain_modify_message,
    .enabled = TRUE,
    .modifier_strength = 1.0f
};

/* VISUAL SYSTEM CONFIGURATION */
struct spatial_system visual_system = {
    .system_name = "Visual",
    .system_id = 1,
    .stimulus = &visual_stimulus_strategy,
    .line_of_sight = &physical_los_strategy,
    .modifiers = &weather_terrain_modifier_strategy,
    .enabled = TRUE,
    .global_range_multiplier = 1.0f,
    .global_intensity_multiplier = 1.0f,
    .pubsub_topic = "visual_wilderness",
    .pubsub_handler = "visual_stimulus_handler"
};

/*
 * Visual Stimulus Strategy Implementation
 */

/*
 * Calculate visual intensity based on distance and base factors
 */
static int visual_calculate_intensity(struct spatial_context *ctx) {
    float distance_factor;
    
    if (!ctx) {
        return SPATIAL_ERROR_INVALID_PARAM;
    }
    
    /* Calculate distance falloff - visual intensity decreases with distance */
    if (ctx->distance <= 0.0) {
        distance_factor = 1.0f;  /* Same location */
    } else {
        /* Use inverse square law modified for gameplay */
        distance_factor = 1.0f / (1.0f + (ctx->distance / 100.0f));
    }
    
    /* Base intensity starts at 1.0 and is modified by distance */
    ctx->base_intensity = distance_factor;
    
    spatial_debug("Visual intensity calculated: %.3f (distance: %.1f, factor: %.3f)",
                 ctx->base_intensity, ctx->distance, distance_factor);
    
    spatial_log("SPATIAL: Base intensity for visual: %.6f at distance %.2f", 
                ctx->base_intensity, ctx->distance);
    
    return SPATIAL_SUCCESS;
}

/*
 * Generate visual message based on clarity and distance
 */
static int visual_generate_message(struct spatial_context *ctx, char *output, size_t max_len) {
    visual_message_type_t msg_type;
    float clarity = ctx->final_intensity;
    
    spatial_log("SPATIAL: visual_generate_message called - ctx=%p, output=%p, source_desc=%p", 
                ctx, output, ctx ? ctx->source_description : NULL);
    
    if (!ctx || !output || !ctx->source_description) {
        spatial_log("SPATIAL: visual_generate_message validation failed - ctx=%p, output=%p, source_desc=%p",
                   ctx, output, ctx ? ctx->source_description : NULL);
        return SPATIAL_ERROR_INVALID_PARAM;
    }
    
    /* Determine message type based on final intensity */
    if (clarity >= 0.8f) {
        msg_type = VISUAL_MSG_CLEAR;
    } else if (clarity >= 0.6f) {
        msg_type = VISUAL_MSG_DISTANT;
    } else if (clarity >= 0.4f) {
        msg_type = VISUAL_MSG_OBSCURED;
    } else if (clarity >= 0.2f) {
        msg_type = VISUAL_MSG_SILHOUETTE;
    } else {
        msg_type = VISUAL_MSG_GLIMPSE;
    }
    
    /* Generate message based on type */
    switch (msg_type) {
        case VISUAL_MSG_CLEAR:
            snprintf(output, max_len, "You clearly see %s.", ctx->source_description);
            break;
            
        case VISUAL_MSG_DISTANT:
            snprintf(output, max_len, "In the distance, you see %s.", ctx->source_description);
            break;
            
        case VISUAL_MSG_OBSCURED:
            snprintf(output, max_len, "Through the haze, you make out %s.", ctx->source_description);
            break;
            
        case VISUAL_MSG_SILHOUETTE:
            snprintf(output, max_len, "You glimpse the silhouette of %s.", ctx->source_description);
            break;
            
        case VISUAL_MSG_GLIMPSE:
            snprintf(output, max_len, "You catch a brief glimpse of something moving in the distance.");
            break;
    }
    
    spatial_debug("Generated visual message (clarity %.3f, type %d): %s", 
                 clarity, msg_type, output);
    
    spatial_log("SPATIAL: visual_generate_message returning SUCCESS");
    return SPATIAL_SUCCESS;
}

/*
 * Apply visual-specific effects (placeholder for future features)
 */
static int visual_apply_effects(struct spatial_context *ctx) {
    if (!ctx) {
        return SPATIAL_ERROR_INVALID_PARAM;
    }
    
    /* Future: Could add visual-specific effects like:
     * - Color adjustments based on lighting
     * - Size perception based on distance
     * - Motion blur effects
     */
    
    return SPATIAL_SUCCESS;
}

/*
 * Check if observer should process visual stimuli
 */
static bool visual_should_process_observer(struct spatial_context *ctx) {
    if (!ctx || !ctx->observer) {
        return FALSE;
    }
    
    /* Skip NPCs unless specifically flagged */
    if (IS_NPC(ctx->observer)) {
        return FALSE;
    }
    
    /* Skip players not in wilderness for now */
    if (!IN_WILDERNESS(ctx->observer)) {
        return FALSE;
    }
    
    /* Future: Check for blindness, magical sight, etc. */
    
    return TRUE;
}

/*
 * Physical Line of Sight Strategy Implementation
 */

/*
 * Calculate physical obstruction based on terrain
 */
static int physical_calculate_obstruction(struct spatial_context *ctx, float *obstruction_factor) {
    float total_obstruction = 0.0f;
    int steps, i;
    int dx, dy, step_x, step_y;
    float step_size;
    
    if (!ctx || !obstruction_factor) {
        return SPATIAL_ERROR_INVALID_PARAM;
    }
    
    *obstruction_factor = 0.0f;
    
    /* Simple line-of-sight calculation using Bresenham-like algorithm */
    dx = abs(ctx->observer_x - ctx->source_x);
    dy = abs(ctx->observer_y - ctx->source_y);
    steps = MAX(dx, dy);
    
    if (steps == 0) {
        /* Same location - no obstruction */
        return SPATIAL_SUCCESS;
    }
    
    step_size = 1.0f / steps;
    
    /* Check terrain along the line */
    for (i = 1; i < steps; i++) {
        float progress = i * step_size;
        step_x = ctx->source_x + (int)(progress * (ctx->observer_x - ctx->source_x));
        step_y = ctx->source_y + (int)(progress * (ctx->observer_y - ctx->source_y));
        
        int terrain_type = spatial_get_terrain_type(step_x, step_y);
        
        /* Add obstruction based on terrain type */
        switch (terrain_type) {
            case 0:  /* Open terrain */
                /* No obstruction */
                break;
            case 1:  /* Light forest */
                total_obstruction += 0.1f;
                break;
            case 2:  /* Dense forest */
                total_obstruction += 0.3f;
                break;
            case 3:  /* Mountains */
                total_obstruction += 0.8f;
                break;
            case 4:  /* Hills */
                total_obstruction += 0.2f;
                break;
            default:
                total_obstruction += 0.1f;
                break;
        }
    }
    
    /* Cap obstruction at 100% */
    *obstruction_factor = MIN(total_obstruction, 1.0f);
    
    spatial_debug("Physical obstruction calculated: %.3f over %d steps", 
                 *obstruction_factor, steps);
    
    spatial_log("SPATIAL: Physical obstruction: %.6f over %d steps from (%d,%d) to (%d,%d)", 
                *obstruction_factor, steps, ctx->source_x, ctx->source_y, 
                ctx->observer_x, ctx->observer_y);
    
    return SPATIAL_SUCCESS;
}

/*
 * Get blocking elements along line of sight
 */
static int physical_get_obstacles(struct spatial_context *ctx, struct obstacle_list *obstacles) {
    if (!ctx || !obstacles) {
        return SPATIAL_ERROR_INVALID_PARAM;
    }
    
    /* Reset obstacle list */
    obstacles->count = 0;
    
    /* TODO: Implement detailed obstacle detection
     * For now, we'll use the simple terrain-based approach in calculate_obstruction
     */
    
    return SPATIAL_SUCCESS;
}

/*
 * Check if stimulus can transmit through terrain type
 */
static bool physical_can_transmit(int terrain_type, int stimulus_type) {
    /* Visual stimuli are blocked by solid terrain */
    if (stimulus_type == STIMULUS_VISUAL) {
        switch (terrain_type) {
            case 3:  /* Mountains - block completely */
                return FALSE;
            default:
                return TRUE;  /* All other terrain allows some transmission */
        }
    }
    
    /* Audio can travel through most terrain */
    if (stimulus_type == STIMULUS_AUDIO) {
        return TRUE;  /* Audio is harder to block completely */
    }
    
    return TRUE;
}

/*
 * Weather/Terrain Modifier Strategy Implementation
 */

/*
 * Apply weather and terrain modifiers to range and clarity
 */
static int weather_terrain_apply_modifiers(struct spatial_context *ctx, float *range_mod, float *clarity_mod) {
    float weather_range_mod = 1.0f;
    float weather_clarity_mod = 1.0f;
    float time_range_mod = 1.0f;
    float time_clarity_mod = 1.0f;
    
    if (!ctx || !range_mod || !clarity_mod) {
        return SPATIAL_ERROR_INVALID_PARAM;
    }
    
    /* Weather effects - now using real weather data from spatial_get_weather_type() */
    switch (ctx->weather_conditions) {
        case 0:  /* Clear */
            weather_range_mod = 1.0f;
            weather_clarity_mod = 1.0f;
            break;
        case 1:  /* Cloudy */
            weather_range_mod = 0.9f;
            weather_clarity_mod = 0.9f;
            break;
        case 2:  /* Rainy */
            weather_range_mod = 0.6f;
            weather_clarity_mod = 0.7f;
            break;
        case 3:  /* Foggy */
            weather_range_mod = 0.3f;
            weather_clarity_mod = 0.4f;
            break;
        case 4:  /* Storm */
            weather_range_mod = 0.2f;
            weather_clarity_mod = 0.3f;
            break;
    }
    
    /* Time of day effects - now using SUN_* constants */
    switch (ctx->time_of_day) {
        case SUN_LIGHT:  /* Daytime - normal visibility */
            time_range_mod = 1.0f;
            time_clarity_mod = 1.0f;
            break;
        case SUN_RISE:   /* Dawn - slightly reduced visibility */
        case SUN_SET:    /* Dusk - slightly reduced visibility */
            time_range_mod = 0.8f;
            time_clarity_mod = 0.9f;
            break;
        case SUN_DARK:   /* Night - greatly reduced visibility */
        default:
            time_range_mod = 0.3f;
            time_clarity_mod = 0.5f;
            break;
    }
    
    *range_mod = weather_range_mod * time_range_mod;
    *clarity_mod = weather_clarity_mod * time_clarity_mod;
    
    spatial_debug("Environmental modifiers: range %.3f, clarity %.3f (weather: %.3f/%.3f, time: %.3f/%.3f)",
                 *range_mod, *clarity_mod, weather_range_mod, weather_clarity_mod, 
                 time_range_mod, time_clarity_mod);
    
    spatial_log("SPATIAL: Weather/terrain modifiers - range: %.6f, clarity: %.6f, weather: %d, time: %d",
                *range_mod, *clarity_mod, ctx->weather_conditions, ctx->time_of_day);
    
    return SPATIAL_SUCCESS;
}

/*
 * Calculate environmental interference
 */
static int weather_terrain_calculate_interference(struct spatial_context *ctx, float *interference) {
    if (!ctx || !interference) {
        return SPATIAL_ERROR_INVALID_PARAM;
    }
    
    /* Basic interference calculation - can be expanded */
    *interference = 0.0f;
    
    /* Add interference based on weather */
    switch (ctx->weather_conditions) {
        case 2:  /* Rain */
            *interference += 0.2f;
            break;
        case 3:  /* Fog */
            *interference += 0.5f;
            break;
        case 4:  /* Storm */
            *interference += 0.7f;
            break;
    }
    
    return SPATIAL_SUCCESS;
}

/*
 * Modify message based on environmental conditions
 */
static int weather_terrain_modify_message(struct spatial_context *ctx, char *message, size_t max_len) {
    char temp_msg[SPATIAL_MAX_MESSAGE_LENGTH];
    
    if (!ctx || !message) {
        return SPATIAL_ERROR_INVALID_PARAM;
    }
    
    /* Store original message */
    strncpy(temp_msg, message, sizeof(temp_msg) - 1);
    temp_msg[sizeof(temp_msg) - 1] = '\0';
    
    /* Add environmental context based on conditions */
    if (ctx->weather_conditions == 3) {  /* Foggy */
        snprintf(message, max_len, "Through the thick fog, %s", temp_msg + 4);  /* Remove "You " */
    } else if (ctx->weather_conditions == 2) {  /* Rainy */
        snprintf(message, max_len, "Through the rain, %s", temp_msg + 4);
    } else if (ctx->time_of_day == SUN_DARK) {  /* Night */
        snprintf(message, max_len, "In the darkness, %s", temp_msg + 4);
    }
    /* Otherwise keep original message */
    
    return SPATIAL_SUCCESS;
}

/*
 * Initialize visual system
 */
int spatial_visual_init(void) {
    spatial_log("Initializing visual spatial system...");
    
    /* Register the visual system */
    if (spatial_register_system(&visual_system) != SPATIAL_SUCCESS) {
        spatial_log("ERROR: Failed to register visual system");
        return SPATIAL_ERROR_MEMORY;
    }
    
    spatial_log("Visual spatial system initialized successfully");
    return SPATIAL_SUCCESS;
}

/*
 * Test function - simulate a ship passing by
 */
int spatial_visual_test_ship_passing(int ship_x, int ship_y, const char *ship_desc) {
    struct spatial_context *ctx;
    struct char_data *ch;
    int processed_count = 0;
    
    if (!ship_desc) {
        return SPATIAL_ERROR_INVALID_PARAM;
    }
    
    spatial_log("Testing ship passing at (%d, %d): %s", ship_x, ship_y, ship_desc);
    
    /* Create properly initialized context */
    ctx = spatial_create_context();
    if (!ctx) {
        spatial_log("ERROR: Failed to create spatial context");
        return SPATIAL_ERROR_MEMORY;
    }
    
    /* Set source information */
    ctx->source_x = ship_x;
    ctx->source_y = ship_y;
    ctx->source_z = 0; /* Sea level */
    ctx->source_description = strdup(ship_desc);
    ctx->base_intensity = 1.0;
    
    /* Process for all wilderness players */
    for (ch = character_list; ch; ch = ch->next) {
        if (IS_NPC(ch) || !ch->desc) continue;
        if (!ZONE_FLAGGED(GET_ROOM_ZONE(IN_ROOM(ch)), ZONE_WILDERNESS)) continue;
        
        /* Set observer information */
        ctx->observer = ch;
        ctx->observer_x = X_LOC(ch);
        ctx->observer_y = Y_LOC(ch);
        ctx->observer_z = get_modified_elevation(X_LOC(ch), Y_LOC(ch));
        ctx->active_system = &visual_system;
        
        spatial_log("Testing visual for player %s at (%d, %d, %d)", 
                   GET_NAME(ch), ctx->observer_x, ctx->observer_y, ctx->observer_z);
        
        /* Process visual stimulus */
        int result = spatial_process_stimulus(ctx, &visual_system);
        spatial_log("Spatial process result for %s: %d", GET_NAME(ch), result);
        
        if (result == SPATIAL_SUCCESS) {
            send_to_char(ch, "\r\n%s\r\n", ctx->processed_message);
            processed_count++;
            spatial_log("Visual message delivered to %s", GET_NAME(ch));
        } else {
            spatial_log("Visual processing failed for %s with error %d", GET_NAME(ch), result);
        }
    }
    
    /* Cleanup */
    spatial_free_context(ctx);
    
    spatial_log("Ship passing test processed for %d players", processed_count);
    return SPATIAL_SUCCESS;
}
