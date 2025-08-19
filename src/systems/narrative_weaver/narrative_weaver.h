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
 * Main function to create unified wilderness description
 * @param x World X coordinate
 * @param y World Y coordinate
 * @return Newly allocated unified description, or NULL on failure
 */
char *create_unified_wilderness_description(int x, int y);

/**
 * Enhanced wilderness description function that replaces hint-based approach
 * @param ch Character viewing the description
 * @param x World X coordinate  
 * @param y World Y coordinate
 * @return Newly allocated unified description
 */
char *enhanced_wilderness_description_unified(struct char_data *ch, int x, int y);

/**
 * Validate that text uses proper third-person observational voice
 * @param text The text to validate
 * @return TRUE if voice is appropriate, FALSE if contains "You" references
 */
int validate_narrative_voice(const char *text);

/**
 * Enhanced wilderness description function using unified narrative weaving
 * This serves as the main entry point for the description engine
 * @param ch Character requesting the description
 * @param x World X coordinate
 * @param y World Y coordinate
 * @return Newly allocated unified description, or NULL on failure
 */
char *enhanced_wilderness_description_unified(struct char_data *ch, int x, int y);

/**
 * Initialize the narrative weaving system
 * Called during mud startup
 */
void init_narrative_weaver(void);

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
