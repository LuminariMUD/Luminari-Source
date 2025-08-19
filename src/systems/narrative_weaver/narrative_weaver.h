/**
 * @file narrative_weaver.h
 * @brief Header file for template-free narrative weaving system
 * @author GitHub Copilot Region Architect
 * @date August 18, 2025
 */

#ifndef _NARRATIVE_WEAVER_H_
#define _NARRATIVE_WEAVER_H_

/* Structure for holding narrative components */
struct narrative_components {
    char *base_description;     /* Comprehensive region description */
    struct region_hint *hints;  /* Array of contextual hints */
    int weather_influence;      /* Weather impact factor */
    int time_influence;         /* Time of day impact factor */
};

/* Function prototypes for narrative weaving system */

/**
 * Check if a JSON array contains a specific string value
 * @param json_array JSON array string like ["mystical", "tranquil", "ethereal"]
 * @param search_string String to search for
 * @return 1 if found, 0 if not found
 */
int json_array_contains_string(const char *json_array, const char *search_string);

/**
 * Get mood-based weight multiplier for hints based on regional AI characteristics
 * @param hint_category The category of hint being evaluated
 * @param hint_text The actual hint text
 * @param key_characteristics JSON string with regional AI characteristics
 * @return Weight multiplier (0.3 to 1.8)
 */
double get_mood_weight_for_hint(int hint_category, const char *hint_text, const char *key_characteristics);

/**
 * Select a hint using weighted probability based on contextual and mood weights
 * @param hints Array of available hints
 * @param hint_indices Array of indices to select from
 * @param count Number of hints to choose from
 * @param key_characteristics Regional AI characteristics for mood weighting
 * @return Index of selected hint from hint_indices array
 */
int select_weighted_hint(struct region_hint *hints, int *hint_indices, int count, const char *key_characteristics);

/**
 * Load regional AI characteristics for mood-based hint weighting
 * @param region_vnum The region vnum to load characteristics for
 * @return JSON string containing key_characteristics, or NULL if not found
 */
char *load_region_characteristics(int region_vnum);

/**
 * Main function to create unified wilderness description
 * @param zone Zone containing the coordinates
 * @param x World X coordinate
 * @param y World Y coordinate
 * @return Newly allocated unified description, or NULL on failure
 */
char *create_unified_wilderness_description(zone_rnum zone, int x, int y);

/**
 * Enhanced wilderness description function that replaces hint-based approach
 * @param ch Character viewing the description
 * @param room The wilderness room 
 * @param zone Zone containing the coordinates
 * @param x World X coordinate  
 * @param y World Y coordinate
 * @return Newly allocated unified description
 */
char *enhanced_wilderness_description_unified(struct char_data *ch, room_rnum room, zone_rnum zone, int x, int y);

/**
 * Enhance base resource-aware description with regional hints
 * @param base_description Existing resource-aware description to enhance
 * @param ch Character viewing the description
 * @param zone Zone containing the coordinates
 * @param x World X coordinate
 * @param y World Y coordinate
 * @return Enhanced description or NULL if enhancement fails
 */
char *enhance_base_description_with_hints(char *base_description, struct char_data *ch, 
                                         zone_rnum zone, int x, int y);

/**
 * Layer regional hints onto base description
 * @param base_description Foundation description from resource system
 * @param hints Array of contextual hints
 * @param weather_condition Current weather conditions
 * @param time_category Current time of day category
 * @param x World X coordinate
 * @param y World Y coordinate
 * @return Enhanced description with layered hints
 */
char *layer_hints_on_base_description(char *base_description, struct region_hint *hints,
                                     const char *weather_condition, const char *time_category,
                                     int x, int y);

/**
 * Validate that text uses proper third-person observational voice
 * @param text The text to validate
 * @return TRUE if voice is appropriate, FALSE if contains "You" references
 */
int validate_narrative_voice(const char *text);

/**
 * Initialize the narrative weaving system
 * Called during mud startup
 */
void init_narrative_weaver(void);

/**
 * Safe string copy with buffer bounds checking
 * @param dest Destination buffer
 * @param src Source string
 * @param dest_size Size of destination buffer
 * @return Number of characters copied, or -1 on error
 */
int safe_strcpy(char *dest, const char *src, size_t dest_size);

/**
 * Safe string concatenation with buffer bounds checking  
 * @param dest Destination buffer
 * @param src Source string
 * @param dest_size Size of destination buffer
 * @return Number of characters concatenated, or -1 on error
 */
int narrative_safe_strcat(char *dest, const char *src, size_t dest_size);

/* Constants for narrative weaving */
#define NARRATIVE_STYLE_POETIC      0
#define NARRATIVE_STYLE_PRACTICAL   1
#define NARRATIVE_STYLE_MYSTERIOUS  2
#define NARRATIVE_STYLE_DRAMATIC    3
#define NARRATIVE_STYLE_PASTORAL    4

#define NARRATIVE_LENGTH_BRIEF      0
#define NARRATIVE_LENGTH_MODERATE   1
#define NARRATIVE_LENGTH_DETAILED   2
#define NARRATIVE_LENGTH_EXTENSIVE  3

#define MAX_NARRATIVE_LENGTH        4096
#define MAX_TRANSITION_PHRASES      20
#define MAX_VOICE_PATTERNS          15

/* Structure definitions */
struct narrative_components;  /* Forward declaration - full definition in .c file */

#endif /* _NARRATIVE_WEAVER_H_ */
