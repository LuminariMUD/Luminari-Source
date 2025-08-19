/* *************************************************************************
 *   File: region_hints.h                              Part of LuminariMUD *
 *  Usage: Header file for region hints system.                           *
 * Author: Development Team                                                *
 ***************************************************************************
 * Region Hints System                                                    *
 * ===================                                                     *
 * This system provides descriptive hints with the dynamic description    *
 * engine to create rich, location-specific descriptions.                 *
 ***************************************************************************/

#ifndef REGION_HINTS_H
#define REGION_HINTS_H

#include "conf.h"
#include "sysdep.h"
#include "structs.h"

/* Hint Categories */
#define HINT_ATMOSPHERE        0
#define HINT_FAUNA            1
#define HINT_FLORA            2
#define HINT_GEOGRAPHY        3
#define HINT_WEATHER_INFLUENCE 4
#define HINT_RESOURCES        5
#define HINT_LANDMARKS        6
#define HINT_SOUNDS           7
#define HINT_SCENTS           8
#define HINT_SEASONAL_CHANGES 9
#define HINT_TIME_OF_DAY      10
#define HINT_MYSTICAL         11
#define NUM_HINT_CATEGORIES   12

/* Region hint structure */
struct region_hint {
    int id;
    int region_vnum;
    int hint_category;
    char *hint_text;
    int priority;
    char *weather_conditions;
    char *seasonal_weight;      /* JSON string */
    char *time_of_day_weight;   /* JSON string */
    char *resource_triggers;    /* JSON string */
    time_t created_at;
    bool is_active;
    double contextual_weight;   /* Calculated contextual relevance (0.0-1.0+) */
    struct region_hint *next;
};

/* Region profile structure */
struct region_profile {
    int region_vnum;
    char *overall_theme;
    char *dominant_mood;
    char *key_characteristics;  /* JSON string */
    int description_style;      /* 0=poetic, 1=practical, 2=mysterious, 3=dramatic, 4=pastoral */
    int complexity_level;       /* 1-5 */
    time_t created_at;
};

/* Description generation context */
struct description_context {
    room_rnum room;
    struct char_data *ch;
    int weather;
    int season;
    int time_of_day;
    double *resource_levels;    /* Array of resource levels for the location */
    int num_resources;
    struct region_profile *profile;
    struct region_hint *available_hints;
};

/* Function prototypes */

/* Main entry point - called from desc_engine.c */
char *enhance_wilderness_description_with_hints(struct char_data *ch, room_rnum room);

/* Core hint management */
struct region_hint *load_region_hints(int region_vnum);
struct region_profile *load_region_profile(int region_vnum);
void free_region_hints(struct region_hint *hints);
void free_region_profile(struct region_profile *profile);

/* Context building */
void build_description_context(room_rnum room, struct char_data *ch, struct description_context *context);

/* Hint selection and filtering */
struct region_hint *select_relevant_hints(struct region_hint *all_hints, struct description_context *context, int max_hints);
bool hint_matches_conditions(struct region_hint *hint, struct description_context *context);

/* Description generation */
char *generate_enhanced_description(struct description_context *context);
char *combine_base_with_hints(char *base_description, struct region_hint *selected_hints, struct description_context *context);

/* Weather and time integration */
double get_hint_weather_weight(struct region_hint *hint, int weather_condition);
double get_hint_time_weight(struct region_hint *hint, int time_of_day);
double get_hint_seasonal_weight(struct region_hint *hint, int season);
double get_hint_resource_weight(struct region_hint *hint, double *resource_levels);

/* Utility functions */
bool hint_matches_conditions(struct region_hint *hint, struct description_context *context);
char *get_hint_category_name(int category);
char *parse_json_string_value(const char *json, const char *key);
double parse_json_double_value(const char *json, const char *key);

/* Analytics and logging */
void log_hint_usage(int hint_id, room_rnum room, struct char_data *ch, 
                   struct description_context *context);
void update_hint_analytics(int region_vnum);

/* Integration with existing systems */
char *enhance_wilderness_description_with_hints(struct char_data *ch, room_rnum room);
void integrate_hints_with_resource_descriptions(void);

/* Configuration */
#define HINTS_MAX_PER_DESCRIPTION 3
#define HINTS_CACHE_TIMEOUT 300  /* 5 minutes */
#define HINTS_DEFAULT_PRIORITY 5

/* External variables */
extern struct region_hint *region_hint_cache;
extern time_t region_hint_cache_time;

#endif /* REGION_HINTS_H */
