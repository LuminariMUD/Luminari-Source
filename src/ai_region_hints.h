/* *************************************************************************
 *   File: ai_region_hints.h                           Part of LuminariMUD *
 *  Usage: Header file for AI-generated region hints system.              *
 * Author: Development Team                                                *
 ***************************************************************************
 * AI Region Hints System                                                 *
 * =====================                                                   *
 * This system integrates AI-generated descriptive hints with the dynamic *
 * description engine to create rich, location-specific descriptions.     *
 ***************************************************************************/

#ifndef AI_REGION_HINTS_H
#define AI_REGION_HINTS_H

#include "conf.h"
#include "sysdep.h"
#include "structs.h"

/* AI Hint Categories */
#define AI_HINT_ATMOSPHERE        0
#define AI_HINT_FAUNA            1
#define AI_HINT_FLORA            2
#define AI_HINT_GEOGRAPHY        3
#define AI_HINT_WEATHER_INFLUENCE 4
#define AI_HINT_RESOURCES        5
#define AI_HINT_LANDMARKS        6
#define AI_HINT_SOUNDS           7
#define AI_HINT_SCENTS           8
#define AI_HINT_SEASONAL_CHANGES 9
#define AI_HINT_TIME_OF_DAY      10
#define AI_HINT_MYSTICAL         11
#define NUM_AI_HINT_CATEGORIES   12

/* AI Hint structure */
struct ai_region_hint {
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
    struct ai_region_hint *next;
};

/* Region profile structure */
struct ai_region_profile {
    int region_vnum;
    char *overall_theme;
    char *dominant_mood;
    char *key_characteristics;  /* JSON string */
    int description_style;      /* 0=poetic, 1=practical, 2=mysterious, 3=dramatic, 4=pastoral */
    int complexity_level;       /* 1-5 */
    time_t created_at;
};

/* Description generation context */
struct ai_description_context {
    room_rnum room;
    struct char_data *ch;
    int weather;
    int season;
    int time_of_day;
    double *resource_levels;    /* Array of resource levels for the location */
    int num_resources;
    struct ai_region_profile *profile;
    struct ai_region_hint *available_hints;
};

/* Function prototypes */

/* Main entry point - called from desc_engine.c */
char *enhance_wilderness_description_with_ai(struct char_data *ch, room_rnum room);

/* Core hint management */
struct ai_region_hint *load_region_hints(int region_vnum);
struct ai_region_profile *load_region_profile(int region_vnum);
void free_region_hints(struct ai_region_hint *hints);
void free_region_profile(struct ai_region_profile *profile);

/* Hint selection and filtering */
struct ai_region_hint *filter_hints_by_conditions(struct ai_region_hint *hints, 
                                                  struct ai_description_context *context);
struct ai_region_hint *select_hints_for_description(struct ai_region_hint *filtered_hints, 
                                                   int max_hints, 
                                                   struct ai_description_context *context);

/* Description generation */
char *generate_ai_enhanced_description(struct ai_description_context *context);
char *combine_hints_with_base_description(char *base_description, 
                                         struct ai_region_hint *selected_hints,
                                         struct ai_description_context *context);

/* Weather and time integration */
double get_hint_weather_weight(struct ai_region_hint *hint, int weather_condition);
double get_hint_time_weight(struct ai_region_hint *hint, int time_of_day);
double get_hint_seasonal_weight(struct ai_region_hint *hint, int season);
double get_hint_resource_weight(struct ai_region_hint *hint, double *resource_levels);

/* Utility functions */
bool hint_matches_conditions(struct ai_region_hint *hint, struct ai_description_context *context);
char *get_hint_category_name(int category);
char *parse_json_string_value(const char *json, const char *key);
double parse_json_double_value(const char *json, const char *key);

/* Analytics and logging */
void log_hint_usage(int hint_id, room_rnum room, struct char_data *ch, 
                   struct ai_description_context *context);
void update_hint_analytics(int region_vnum);

/* Integration with existing systems */
char *enhance_wilderness_description_with_ai(struct char_data *ch, room_rnum room);
void integrate_ai_hints_with_resource_descriptions(void);

/* Configuration */
#define AI_HINTS_MAX_PER_DESCRIPTION 3
#define AI_HINTS_CACHE_TIMEOUT 300  /* 5 minutes */
#define AI_HINTS_DEFAULT_PRIORITY 5

/* External variables */
extern struct ai_region_hint *region_hint_cache;
extern time_t region_hint_cache_time;

#endif /* AI_REGION_HINTS_H */
