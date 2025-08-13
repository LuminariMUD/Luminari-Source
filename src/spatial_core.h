/*************************************************************************
*   File: spatial_core.h                               Part of LuminariMUD *
*  Usage: Core spatial system interfaces and definitions                   *
*  Author: Luminari Development Team                                       *
*                                                                          *
*  All rights reserved.  See license for complete information.            *
*                                                                          *
*  LuminariMUD is based on CircleMUD, Copyright (C) 1993, 94 by the       *
*  Department of Computer Science at the Johns Hopkins University         *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#ifndef _SPATIAL_CORE_H_
#define _SPATIAL_CORE_H_

#include "structs.h"

/* Spatial System Configuration */
#define SPATIAL_MAX_RANGE                 2000.0f
#define SPATIAL_MIN_THRESHOLD             0.1f
#define SPATIAL_MAX_MESSAGE_LENGTH        1024
#define SPATIAL_MAX_OBSTACLES             100
#define SPATIAL_MAX_NEARBY_ENTITIES       50
#define SPATIAL_CACHE_SIZE                256

/* Stimulus Types */
#define STIMULUS_VISUAL                   1
#define STIMULUS_AUDIO                    2
#define STIMULUS_EMPATHY                  3
#define STIMULUS_MAGICAL                  4
#define STIMULUS_SCENT                    5
#define STIMULUS_VIBRATION                6

/* Return Codes */
#define SPATIAL_SUCCESS                   0
#define SPATIAL_ERROR_STIMULUS            -1
#define SPATIAL_ERROR_LOS                 -2
#define SPATIAL_ERROR_MODIFIERS           -3
#define SPATIAL_ERROR_BELOW_THRESHOLD     -4
#define SPATIAL_ERROR_INVALID_PARAM       -5
#define SPATIAL_ERROR_MEMORY              -6
#define SPATIAL_ERROR_NOT_IMPLEMENTED     -7

/* Forward Declarations */
struct spatial_context;
struct spatial_system;
struct stimulus_strategy;
struct los_strategy;
struct modifier_strategy;

/* Obstacle Information */
struct spatial_obstacle {
    int x, y, z;
    int terrain_type;
    float obstruction_factor;  /* 0.0 = no obstruction, 1.0 = complete block */
    char *description;
};

/* List of obstacles */
struct obstacle_list {
    struct spatial_obstacle *obstacles;
    int count;
    int capacity;
};

/* Nearby entity information (for empathy/mental systems) */
struct nearby_entity {
    struct char_data *entity;
    float distance;
    int entity_type;  /* PC, NPC, etc. */
    float interference_factor;
};

/* List of nearby entities */
struct entity_list {
    struct nearby_entity *entities;
    int count;
    int capacity;
};

/* Core spatial context - passed between all strategies */
struct spatial_context {
    /* Source Information */
    int source_x, source_y, source_z;
    char *source_description;
    int stimulus_type;
    float base_intensity;
    void *source_data;  /* Additional stimulus-specific data */
    
    /* Observer Information */
    struct char_data *observer;
    int observer_x, observer_y, observer_z;
    void *observer_data;  /* Additional observer-specific data */
    
    /* Environmental Factors */
    int weather_conditions;
    int time_of_day;
    float magical_field_strength;
    int terrain_difficulty;
    
    /* Audio-specific data */
    int audio_frequency;    /* Audio frequency band for sound calculations */
    
    /* Calculated Values */
    float distance;
    float effective_range;
    float obstruction_factor;
    float environmental_modifier;
    float final_intensity;
    
    /* Working Data */
    struct obstacle_list obstacles;
    struct entity_list nearby_entities;
    char *processed_message;
    
    /* System Configuration */
    struct spatial_system *active_system;
    
    /* Cache and Performance */
    bool use_cache;
    int cache_key;
    time_t last_calculated;
};

/* STRATEGY 1: STIMULUS STRATEGY - How the event is generated/processed */
struct stimulus_strategy {
    char *name;
    int stimulus_type;
    float base_range;
    
    /* Core Functions */
    int (*calculate_intensity)(struct spatial_context *ctx);
    int (*generate_base_message)(struct spatial_context *ctx, char *output, size_t max_len);
    int (*apply_stimulus_effects)(struct spatial_context *ctx);
    
    /* Optional Functions */
    int (*initialize_stimulus_data)(struct spatial_context *ctx);
    int (*cleanup_stimulus_data)(struct spatial_context *ctx);
    bool (*should_process_observer)(struct spatial_context *ctx);
    
    /* Strategy metadata */
    bool enabled;
    int usage_count;
    float performance_factor;  /* For optimization */
};

/* STRATEGY 2: LINE OF SIGHT STRATEGY - How transmission is blocked */
struct los_strategy {
    char *name;
    int supported_stimulus_types;  /* Bitmask of supported types */
    
    /* Core Functions */
    int (*calculate_obstruction)(struct spatial_context *ctx, float *obstruction_factor);
    int (*get_blocking_elements)(struct spatial_context *ctx, struct obstacle_list *obstacles);
    bool (*can_transmit_through)(int terrain_type, int stimulus_type);
    
    /* Optional Functions */
    int (*precompute_los_data)(struct spatial_context *ctx);
    int (*cache_los_result)(struct spatial_context *ctx);
    int (*get_cached_los_result)(struct spatial_context *ctx, float *cached_obstruction);
    
    /* Strategy metadata */
    bool enabled;
    bool use_caching;
    int cache_hits;
    int cache_misses;
};

/* STRATEGY 3: MODIFIER STRATEGY - Environmental effects on transmission */
struct modifier_strategy {
    char *name;
    int applicable_stimulus_types;  /* Bitmask of applicable types */
    
    /* Core Functions */
    int (*apply_environmental_modifiers)(struct spatial_context *ctx, float *range_mod, float *clarity_mod);
    int (*calculate_interference)(struct spatial_context *ctx, float *interference);
    int (*modify_message)(struct spatial_context *ctx, char *message, size_t max_len);
    
    /* Optional Functions */
    int (*get_environmental_factors)(struct spatial_context *ctx);
    bool (*should_apply_modifier)(struct spatial_context *ctx);
    
    /* Strategy metadata */
    bool enabled;
    float modifier_strength;  /* 0.0-1.0 for variable strength */
};

/* UNIFIED SPATIAL SYSTEM */
struct spatial_system {
    char *system_name;
    int system_id;
    
    /* The three strategies */
    struct stimulus_strategy *stimulus;
    struct los_strategy *line_of_sight;
    struct modifier_strategy *modifiers;
    
    /* System configuration */
    bool enabled;
    float global_range_multiplier;
    float global_intensity_multiplier;
    
    /* Performance tracking */
    int total_processed;
    int successful_transmissions;
    float avg_processing_time_ms;
    
    /* Integration with PubSub */
    char *pubsub_topic;
    char *pubsub_handler;
};

/* Core Processing Functions */
int spatial_init_system(void);
void spatial_shutdown_system(void);

int spatial_process_stimulus(struct spatial_context *ctx, struct spatial_system *system);
int spatial_process_all_systems(struct spatial_context *ctx, struct spatial_system **systems, int system_count);

/* Context Management */
struct spatial_context *spatial_create_context(void);
void spatial_free_context(struct spatial_context *ctx);
int spatial_setup_context(struct spatial_context *ctx, int source_x, int source_y, int source_z,
                         struct char_data *observer, const char *description);

/* System Management */
int spatial_register_system(struct spatial_system *system);
int spatial_unregister_system(const char *system_name);
struct spatial_system *spatial_find_system(const char *system_name);
struct spatial_system **spatial_get_all_systems(int *count);

/* Strategy Registration */
int spatial_register_stimulus_strategy(struct stimulus_strategy *strategy);
int spatial_register_los_strategy(struct los_strategy *strategy);
int spatial_register_modifier_strategy(struct modifier_strategy *strategy);

/* Utility Functions */
float spatial_calculate_3d_distance(int x1, int y1, int z1, int x2, int y2, int z2);
bool spatial_is_in_range(struct spatial_context *ctx, float max_range);
int spatial_get_terrain_type(int x, int y);

/* Cache Management */
int spatial_init_cache(void);
void spatial_cleanup_cache(void);
int spatial_cache_result(struct spatial_context *ctx, float result);
int spatial_get_cached_result(struct spatial_context *ctx, float *cached_result);

/* Debug and Logging */
void spatial_log(const char *format, ...);
void spatial_debug(const char *format, ...);
const char *spatial_error_string(int error_code);

/* Global Variables */
extern bool spatial_system_enabled;
extern bool spatial_debug_mode;
extern struct spatial_system **registered_systems;
extern int registered_system_count;

#endif /* _SPATIAL_CORE_H_ */
