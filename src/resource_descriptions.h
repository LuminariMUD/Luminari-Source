/* *************************************************************************
 *   File: resource_descriptions.h             Part of LuminariMUD        *
 *  Usage: Header for dynamic resource-based descriptions                  *
 * Author: Dynamic Descriptions Implementation                             *
 ***************************************************************************
 * Resource-aware description generation for creating beautiful,           *
 * immersive wilderness descriptions that change based on ecological state *
 ***************************************************************************/

#ifndef RESOURCE_DESCRIPTIONS_H
#define RESOURCE_DESCRIPTIONS_H

#include "campaign.h"

/* Only compile if dynamic descriptions are enabled for this campaign */
#ifdef ENABLE_DYNAMIC_RESOURCE_DESCRIPTIONS

/* ===== CONSTANTS ===== */

/* Resource level thresholds for description variations */
#define RESOURCE_ABUNDANT_THRESHOLD    0.75f
#define RESOURCE_MODERATE_THRESHOLD    0.40f
#define RESOURCE_SPARSE_THRESHOLD      0.15f

/* Description detail levels */
#define DESC_BRIEF      1
#define DESC_STANDARD   2
#define DESC_DETAILED   3
#define DESC_ELABORATE  4
#define DESC_VERBOSE    5

/* Season definitions for description variation */
#define SEASON_SPRING   0
#define SEASON_SUMMER   1
#define SEASON_AUTUMN   2
#define SEASON_WINTER   3

/* Note: Time of day uses game's SUN_* constants from structs.h:
 * SUN_DARK (0) - Night time (hours 22-5)
 * SUN_RISE (1) - Dawn (hours 5-6) 
 * SUN_LIGHT (2) - Day time (hours 6-21)
 * SUN_SET (3) - Dusk (hours 21-22)
 */

/* Weather intensity thresholds for wilderness weather (0-255 scale) */
#define WEATHER_CLEAR_MAX       177  /* Clear/light clouds */
#define WEATHER_RAIN_MIN        178  /* Light rain starts */
#define WEATHER_STORM_MIN       200  /* Heavy rain/storms */
#define WEATHER_LIGHTNING_MIN   225  /* Lightning storms */

/* Weather types for description variation */
#define WEATHER_CLEAR       0
#define WEATHER_CLOUDY      1
#define WEATHER_RAINY       2
#define WEATHER_STORMY      3
#define WEATHER_LIGHTNING   4

/* ===== STRUCTURES ===== */

/* Resource state for description generation */
struct resource_state {
    float vegetation_level;
    float mineral_level;
    float water_level;
    float herb_level;
    float game_level;
    float wood_level;
    float stone_level;
    float clay_level;
    float salt_level;
};

/* Environmental context for descriptions */
struct environmental_context {
    int season;
    int time_of_day;        /* SUN_DARK, SUN_RISE, SUN_LIGHT, SUN_SET */
    int weather;            /* Wilderness weather for coord-based rooms */
    int light_level;        /* Total light level including all sources */
    int artificial_light;   /* Light from non-natural sources (torches, spells, etc.) */
    int natural_light;      /* Light from sun/moon only */
    int terrain_type;
    float elevation;
    bool near_water;
    bool in_forest;
    bool in_mountains;
    bool has_light_sources; /* True if artificial light sources are present */
};

/* ===== FUNCTION PROTOTYPES ===== */

/* Main description generation function */
char *generate_resource_aware_description(struct char_data *ch, room_rnum room);

/* Helper functions for description building */
/* Function prototypes */
void get_resource_state(room_rnum room, struct resource_state *state);
void get_environmental_context(room_rnum room, struct environmental_context *context);
int calculate_total_light_level(room_rnum room);

/* Description component generators */
char *get_terrain_base_description(room_rnum room, struct resource_state *state, 
                                  struct environmental_context *context);
void add_vegetation_details(char *desc, struct resource_state *state, 
                           struct environmental_context *context);
void add_geological_details(char *desc, struct resource_state *state, 
                           struct environmental_context *context);
void add_water_features(char *desc, struct resource_state *state, 
                       struct environmental_context *context);
void add_temporal_atmosphere(char *desc, struct environmental_context *context);
void add_wildlife_presence(char *desc, struct resource_state *state, 
                          struct environmental_context *context);

/* Resource level categorization */
const char *get_resource_abundance_category(float level);
const char *get_vegetation_description(float level, int season);
const char *get_mineral_description(float level, int terrain_type);
const char *get_water_description(float level, int season, int weather);

/* Utility functions */
int get_current_season(void);
int get_terrain_type(room_rnum room);
int calculate_total_light_level(room_rnum room);
int calculate_artificial_light_level(room_rnum room);
int calculate_natural_light_level(room_rnum room);

#endif /* ENABLE_DYNAMIC_RESOURCE_DESCRIPTIONS */

#endif /* RESOURCE_DESCRIPTIONS_H */
