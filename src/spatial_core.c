/*************************************************************************
*   File: spatial_core.c                               Part of LuminariMUD *
*  Usage: Core spatial system implementation                               *
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
#include <time.h>

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "wilderness.h"
#include "spatial_core.h"

/* Global Variables */
bool spatial_system_enabled = FALSE;
bool spatial_debug_mode = FALSE;
struct spatial_system **registered_systems = NULL;
int registered_system_count = 0;
static int max_registered_systems = 10;

/* Strategy registries (temporarily unused - will be needed for strategy registration) */
/* static struct stimulus_strategy **stimulus_strategies = NULL;
static struct los_strategy **los_strategies = NULL;
static struct modifier_strategy **modifier_strategies = NULL;
static int stimulus_strategy_count = 0;
static int los_strategy_count = 0;
static int modifier_strategy_count = 0; */

/* Performance tracking */
static struct {
    long total_processed;
    long successful_transmissions;
    double total_processing_time_ms;
    time_t last_reset;
} spatial_stats;

/*
 * Initialize the spatial system
 */
int spatial_init_system(void) {
    spatial_log("Initializing spatial system...");
    
    /* Allocate system registry */
    CREATE(registered_systems, struct spatial_system *, max_registered_systems);
    if (!registered_systems) {
        spatial_log("ERROR: Failed to allocate system registry");
        return SPATIAL_ERROR_MEMORY;
    }
    
    /* Allocate strategy registries (future implementation) */
    /* CREATE(stimulus_strategies, struct stimulus_strategy *, 20);
    CREATE(los_strategies, struct los_strategy *, 20);
    CREATE(modifier_strategies, struct modifier_strategy *, 20);
    
    if (!stimulus_strategies || !los_strategies || !modifier_strategies) {
        spatial_log("ERROR: Failed to allocate strategy registries");
        return SPATIAL_ERROR_MEMORY;
    } */
    
    /* Initialize cache system */
    if (spatial_init_cache() != SPATIAL_SUCCESS) {
        spatial_log("WARNING: Failed to initialize spatial cache");
        /* Continue without cache */
    }
    
    /* Reset statistics */
    memset(&spatial_stats, 0, sizeof(spatial_stats));
    spatial_stats.last_reset = time(NULL);
    
    spatial_system_enabled = TRUE;
    spatial_log("Spatial system initialized successfully");
    
    return SPATIAL_SUCCESS;
}

/*
 * Shutdown the spatial system
 */
void spatial_shutdown_system(void) {
    int i;
    
    spatial_log("Shutting down spatial system...");
    
    spatial_system_enabled = FALSE;
    
    /* Free registered systems */
    if (registered_systems) {
        for (i = 0; i < registered_system_count; i++) {
            if (registered_systems[i]) {
                /* Don't free the systems themselves - they're static */
                registered_systems[i] = NULL;
            }
        }
        free(registered_systems);
        registered_systems = NULL;
    }
    
    /* Free strategy registries */
    /* TODO: Implement strategy registries
    if (stimulus_strategies) {
        free(stimulus_strategies);
        stimulus_strategies = NULL;
    }
    if (los_strategies) {
        free(los_strategies);
        los_strategies = NULL;
    }
    if (modifier_strategies) {
        free(modifier_strategies);
        modifier_strategies = NULL;
    }
    */
    
    /* Cleanup cache */
    spatial_cleanup_cache();
    
    spatial_log("Spatial system shutdown complete");
}

/*
 * Main processing function - uses all three strategies
 */
int spatial_process_stimulus(struct spatial_context *ctx, struct spatial_system *system) {
    float obstruction_factor = 0.0;
    float range_modifier = 1.0;
    float clarity_modifier = 1.0;
    clock_t start_time, end_time;
    
    if (!ctx || !system) {
        return SPATIAL_ERROR_INVALID_PARAM;
    }
    
    if (!spatial_system_enabled || !system->enabled) {
        return SPATIAL_ERROR_NOT_IMPLEMENTED;
    }
    
    start_time = clock();
    ctx->active_system = system;
    
    spatial_debug("Processing %s stimulus from (%d,%d,%d) to (%d,%d,%d)",
                 system->system_name, ctx->source_x, ctx->source_y, ctx->source_z,
                 ctx->observer_x, ctx->observer_y, ctx->observer_z);
    
    /* Step 1: Calculate base distance */
    ctx->distance = spatial_calculate_3d_distance(ctx->source_x, ctx->source_y, ctx->source_z,
                                                 ctx->observer_x, ctx->observer_y, ctx->observer_z);
    
    /* Quick range check */
    if (ctx->distance > (system->stimulus->base_range * system->global_range_multiplier)) {
        spatial_debug("Stimulus out of range: %.2f > %.2f", ctx->distance, 
                     system->stimulus->base_range * system->global_range_multiplier);
        return SPATIAL_ERROR_BELOW_THRESHOLD;
    }
    
    /* Step 2: Calculate base stimulus intensity */
    if (system->stimulus->calculate_intensity(ctx) != SPATIAL_SUCCESS) {
        spatial_debug("Failed to calculate stimulus intensity");
        return SPATIAL_ERROR_STIMULUS;
    }
    
    /* Step 3: Check line of sight using appropriate strategy */
    if (system->line_of_sight->calculate_obstruction(ctx, &obstruction_factor) != SPATIAL_SUCCESS) {
        spatial_debug("Failed to calculate line of sight obstruction");
        return SPATIAL_ERROR_LOS;
    }
    
    /* Step 4: Apply environmental modifiers */
    if (system->modifiers->apply_environmental_modifiers(ctx, &range_modifier, &clarity_modifier) != SPATIAL_SUCCESS) {
        spatial_debug("Failed to apply environmental modifiers");
        return SPATIAL_ERROR_MODIFIERS;
    }
    
    /* Step 5: Calculate final transmission strength */
    ctx->final_intensity = ctx->base_intensity * (1.0 - obstruction_factor) * range_modifier * system->global_intensity_multiplier;
    
    spatial_debug("Final intensity: %.3f (base: %.3f, obstruction: %.3f, range_mod: %.3f)",
                 ctx->final_intensity, ctx->base_intensity, obstruction_factor, range_modifier);
    
    spatial_log("SPATIAL: Final intensity calculation: %.6f = %.6f * (1.0 - %.6f) * %.6f * %.6f",
                ctx->final_intensity, ctx->base_intensity, obstruction_factor, range_modifier, 
                system->global_intensity_multiplier);
    spatial_log("SPATIAL: Threshold check: %.6f > %.6f = %s", 
                ctx->final_intensity, SPATIAL_MIN_THRESHOLD, 
                (ctx->final_intensity > SPATIAL_MIN_THRESHOLD) ? "PASS" : "FAIL");
    
    /* Step 6: Generate appropriate message if stimulus is strong enough */
    if (ctx->final_intensity > SPATIAL_MIN_THRESHOLD) {
        spatial_log("SPATIAL: Attempting message generation...");
        if (system->stimulus->generate_base_message(ctx, ctx->processed_message, SPATIAL_MAX_MESSAGE_LENGTH) == SPATIAL_SUCCESS) {
            spatial_log("SPATIAL: Base message generated successfully");
            /* Allow modifiers to adjust the message based on clarity */
            if (system->modifiers->modify_message(ctx, ctx->processed_message, SPATIAL_MAX_MESSAGE_LENGTH) == SPATIAL_SUCCESS) {
                spatial_log("SPATIAL: Message modification successful");
                /* Update statistics */
                end_time = clock();
                double processing_time = ((double)(end_time - start_time)) / CLOCKS_PER_SEC * 1000.0;
                
                spatial_stats.total_processed++;
                spatial_stats.successful_transmissions++;
                spatial_stats.total_processing_time_ms += processing_time;
                
                system->total_processed++;
                system->successful_transmissions++;
                system->avg_processing_time_ms = (system->avg_processing_time_ms * (system->total_processed - 1) + processing_time) / system->total_processed;
                
                spatial_debug("Stimulus successfully processed in %.3f ms", processing_time);
                return SPATIAL_SUCCESS;
            } else {
                spatial_log("SPATIAL: Message modification FAILED");
            }
        } else {
            spatial_log("SPATIAL: Base message generation FAILED");
        }
    }
    
    spatial_debug("Stimulus below threshold: %.3f < %.3f", ctx->final_intensity, SPATIAL_MIN_THRESHOLD);
    return SPATIAL_ERROR_BELOW_THRESHOLD;
}

/*
 * Process stimulus through multiple systems
 */
int spatial_process_all_systems(struct spatial_context *ctx, struct spatial_system **systems, int system_count) {
    int processed_count = 0;
    int i;
    
    if (!ctx || !systems || system_count <= 0) {
        return 0;
    }
    
    for (i = 0; i < system_count; i++) {
        if (systems[i] && systems[i]->enabled) {
            if (spatial_process_stimulus(ctx, systems[i]) == SPATIAL_SUCCESS) {
                processed_count++;
                /* Could deliver message here or queue for batch delivery */
                spatial_debug("Message processed by %s system: %s", 
                             systems[i]->system_name, ctx->processed_message);
            }
        }
    }
    
    return processed_count;
}

/*
 * Create and initialize a spatial context
 */
struct spatial_context *spatial_create_context(void) {
    struct spatial_context *ctx;
    
    CREATE(ctx, struct spatial_context, 1);
    if (!ctx) {
        return NULL;
    }
    
    /* Initialize obstacle list */
    CREATE(ctx->obstacles.obstacles, struct spatial_obstacle, SPATIAL_MAX_OBSTACLES);
    if (!ctx->obstacles.obstacles) {
        free(ctx);
        return NULL;
    }
    ctx->obstacles.capacity = SPATIAL_MAX_OBSTACLES;
    ctx->obstacles.count = 0;
    
    /* Initialize entity list */
    CREATE(ctx->nearby_entities.entities, struct nearby_entity, SPATIAL_MAX_NEARBY_ENTITIES);
    if (!ctx->nearby_entities.entities) {
        free(ctx->obstacles.obstacles);
        free(ctx);
        return NULL;
    }
    ctx->nearby_entities.capacity = SPATIAL_MAX_NEARBY_ENTITIES;
    ctx->nearby_entities.count = 0;
    
    /* Allocate message buffer */
    CREATE(ctx->processed_message, char, SPATIAL_MAX_MESSAGE_LENGTH);
    if (!ctx->processed_message) {
        free(ctx->nearby_entities.entities);
        free(ctx->obstacles.obstacles);
        free(ctx);
        return NULL;
    }
    
    /* Set defaults */
    ctx->use_cache = TRUE;
    ctx->last_calculated = time(NULL);
    
    return ctx;
}

/*
 * Free a spatial context
 */
void spatial_free_context(struct spatial_context *ctx) {
    if (!ctx) return;
    
    if (ctx->obstacles.obstacles) {
        free(ctx->obstacles.obstacles);
    }
    if (ctx->nearby_entities.entities) {
        free(ctx->nearby_entities.entities);
    }
    if (ctx->processed_message) {
        free(ctx->processed_message);
    }
    if (ctx->source_description) {
        /* Don't free - assumed to be managed elsewhere */
    }
    
    free(ctx);
}

/*
 * Get current weather type for spatial processing (0-4 scale)
 * This is a helper function that strategies can use to access weather data
 */
int spatial_get_weather_type(struct char_data *observer) {
    if (!observer) {
        return 0; /* Clear weather fallback */
    }
    
    room_rnum room = IN_ROOM(observer);
    
    /* Check if observer is in wilderness area */
    if (room != NOWHERE && IS_WILDERNESS_VNUM(GET_ROOM_VNUM(room))) {
        /* Wilderness: Use coordinate-based weather */
        int x = world[room].coords[0];
        int y = world[room].coords[1];
        int wilderness_weather = get_weather(x, y);
        
        /* Convert wilderness weather (0-255) to spatial weather types (0-4) */
        if (wilderness_weather >= 225) return 4;  /* Lightning */
        if (wilderness_weather >= 200) return 3;  /* Storm */
        if (wilderness_weather >= 178) return 2;  /* Rainy */
        if (wilderness_weather > 127) return 1;   /* Cloudy */
        return 0; /* Clear */
    } else {
        /* Non-wilderness: Use global weather system */
        switch (weather_info.sky) {
            case SKY_CLOUDLESS: return 0;  /* Clear */
            case SKY_CLOUDY:    return 1;  /* Cloudy */
            case SKY_RAINING:   return 2;  /* Rainy */
            case SKY_LIGHTNING: return 4;  /* Lightning */
            default:            return 0;  /* Clear fallback */
        }
    }
}

/*
 * Get raw weather value for strategies that need more granular data
 */
int spatial_get_raw_weather(struct char_data *observer) {
    if (!observer) {
        return 0;
    }
    
    room_rnum room = IN_ROOM(observer);
    
    if (room != NOWHERE && IS_WILDERNESS_VNUM(GET_ROOM_VNUM(room))) {
        /* Wilderness: Return raw Perlin noise value (0-255) */
        int x = world[room].coords[0];
        int y = world[room].coords[1];
        return get_weather(x, y);
    } else {
        /* Non-wilderness: Convert global weather to equivalent scale */
        switch (weather_info.sky) {
            case SKY_CLOUDLESS: return 100;   /* Clear equivalent */
            case SKY_CLOUDY:    return 150;   /* Cloudy equivalent */
            case SKY_RAINING:   return 190;   /* Rainy equivalent */
            case SKY_LIGHTNING: return 230;   /* Lightning equivalent */
            default:            return 100;   /* Clear fallback */
        }
    }
}

/*
 * Setup context with basic source and observer information
 */
int spatial_setup_context(struct spatial_context *ctx, int source_x, int source_y, int source_z,
                         struct char_data *observer, const char *description) {
    if (!ctx || !observer) {
        return SPATIAL_ERROR_INVALID_PARAM;
    }
    
    /* Source information */
    ctx->source_x = source_x;
    ctx->source_y = source_y;
    ctx->source_z = source_z;
    ctx->source_description = (char *)description;  /* Assume managed elsewhere */
    
    /* Observer information */
    ctx->observer = observer;
    if (IN_WILDERNESS(observer)) {
        ctx->observer_x = X_LOC(observer);
        ctx->observer_y = Y_LOC(observer);
        ctx->observer_z = get_elevation(0, X_LOC(observer), Y_LOC(observer));
    } else {
        /* Not in wilderness - use room coordinates if available */
        ctx->observer_x = 0;
        ctx->observer_y = 0;
        ctx->observer_z = 0;
    }
    
    /* Environmental factors - strategies decide how to use this data */
    ctx->weather_conditions = spatial_get_weather_type(observer);
    ctx->time_of_day = weather_info.sunlight;  /* Use sunlight state for better accuracy */
    ctx->magical_field_strength = 0.0;  /* TODO: Get from magic system */
    
    /* Reset calculated values */
    ctx->distance = 0.0;
    ctx->effective_range = 0.0;
    ctx->obstruction_factor = 0.0;
    ctx->environmental_modifier = 1.0;
    ctx->final_intensity = 0.0;
    
    /* Clear working data */
    ctx->obstacles.count = 0;
    ctx->nearby_entities.count = 0;
    
    return SPATIAL_SUCCESS;
}

/*
 * Calculate 3D distance between two points
 */
float spatial_calculate_3d_distance(int x1, int y1, int z1, int x2, int y2, int z2) {
    float dx = (float)(x2 - x1);
    float dy = (float)(y2 - y1);
    float dz = (float)(z2 - z1);
    
    return sqrt(dx*dx + dy*dy + dz*dz);
}

/*
 * Check if context is within range
 */
bool spatial_is_in_range(struct spatial_context *ctx, float max_range) {
    if (!ctx) return FALSE;
    
    return (ctx->distance <= max_range);
}

/*
 * Register a spatial system
 */
int spatial_register_system(struct spatial_system *system) {
    if (!system || !system->system_name) {
        return SPATIAL_ERROR_INVALID_PARAM;
    }
    
    if (registered_system_count >= max_registered_systems) {
        spatial_log("ERROR: Maximum number of spatial systems reached");
        return SPATIAL_ERROR_MEMORY;
    }
    
    /* Check for duplicate names */
    int i;
    for (i = 0; i < registered_system_count; i++) {
        if (registered_systems[i] && !strcmp(registered_systems[i]->system_name, system->system_name)) {
            spatial_log("ERROR: Spatial system '%s' already registered", system->system_name);
            return SPATIAL_ERROR_INVALID_PARAM;
        }
    }
    
    /* Initialize system stats */
    system->total_processed = 0;
    system->successful_transmissions = 0;
    system->avg_processing_time_ms = 0.0;
    
    registered_systems[registered_system_count] = system;
    registered_system_count++;
    
    spatial_log("Registered spatial system: %s", system->system_name);
    return SPATIAL_SUCCESS;
}

/*
 * Find a registered system by name
 */
struct spatial_system *spatial_find_system(const char *system_name) {
    int i;
    
    if (!system_name) return NULL;
    
    for (i = 0; i < registered_system_count; i++) {
        if (registered_systems[i] && !strcmp(registered_systems[i]->system_name, system_name)) {
            return registered_systems[i];
        }
    }
    
    return NULL;
}

/*
 * Get terrain type at coordinates (placeholder - integrate with wilderness system)
 */
int spatial_get_terrain_type(int x, int y) {
    /* TODO: Integrate with actual wilderness terrain system */
    return 0;  /* Default terrain */
}

/*
 * Stub cache functions - to be implemented
 */
int spatial_init_cache(void) {
    return SPATIAL_SUCCESS;
}

void spatial_cleanup_cache(void) {
    /* TODO: Implement cache cleanup */
}

int spatial_cache_result(struct spatial_context *ctx, float result) {
    /* TODO: Implement result caching */
    return SPATIAL_SUCCESS;
}

int spatial_get_cached_result(struct spatial_context *ctx, float *cached_result) {
    /* TODO: Implement cache lookup */
    return SPATIAL_ERROR_NOT_IMPLEMENTED;
}

/*
 * Logging functions
 */
void spatial_log(const char *format, ...) {
    va_list args;
    char buf[MAX_STRING_LENGTH];
    
    va_start(args, format);
    vsnprintf(buf, sizeof(buf), format, args);
    va_end(args);
    
    log("Spatial: %s", buf);
}

void spatial_debug(const char *format, ...) {
    va_list args;
    char buf[MAX_STRING_LENGTH];
    
    if (!spatial_debug_mode) return;
    
    va_start(args, format);
    vsnprintf(buf, sizeof(buf), format, args);
    va_end(args);
    
    log("SPATIAL DEBUG: %s", buf);
}

/*
 * Error code to string conversion
 */
const char *spatial_error_string(int error_code) {
    switch (error_code) {
        case SPATIAL_SUCCESS: return "Success";
        case SPATIAL_ERROR_STIMULUS: return "Stimulus calculation failed";
        case SPATIAL_ERROR_LOS: return "Line of sight calculation failed";
        case SPATIAL_ERROR_MODIFIERS: return "Environmental modifier calculation failed";
        case SPATIAL_ERROR_BELOW_THRESHOLD: return "Stimulus below detection threshold";
        case SPATIAL_ERROR_INVALID_PARAM: return "Invalid parameter";
        case SPATIAL_ERROR_MEMORY: return "Memory allocation failed";
        case SPATIAL_ERROR_NOT_IMPLEMENTED: return "Feature not implemented";
        default: return "Unknown error";
    }
}
