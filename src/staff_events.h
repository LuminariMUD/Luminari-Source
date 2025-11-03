/*/ \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \
\     Staff Ran Event System
/  File:       staff_events.h
/  Created By: Zusuk
\  Main:     staff_events.c
/    System for running staff events
\    Basics including starting, ending and info on the event, etc
/  Created on April 26, 2020
\ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ /*/

/* includes */
#include "utils.h" /* for the ACMD macro */
#include <stdbool.h> /* for bool type in state management functions */
/* end includes */

/***/
/******* CONFIGURATION DEFINES AND CONSTANTS */

/*
 * Player variable index for staff-ran event related data storage.
 * This is the index in the pfile array for staff-ran event related stuff
 * up to STAFF_RAN_EVENTS_VAR array boundary.
 */
#define STAFFRAN_PVAR_JACKALOPE 0

/*
 * Mandatory delay between staff events for cleanup and system stability.
 * This prevents rapid-fire event starting which could cause memory issues
 * or incomplete cleanup from previous events.
 * DO NOT CHANGE: don't decrease this below 6 ticks for system stability.
 */
#define STAFF_EVENT_DELAY_CNST 6

/*
 * Event data field indices for the staff_events_list array.
 * Each event has 5 predefined message/data fields that define its behavior:
 * - TITLE: Display name of the event
 * - BEGIN: Message broadcast when event starts
 * - END: Message broadcast when event ends
 * - DETAIL: Detailed information shown to players about the event
 * - SUMMARY: Conclusion message shown when event completes
 */
#define EVENT_TITLE 0    /* Event title/name for display */
#define EVENT_BEGIN 1    /* Event start announcement message */
#define EVENT_END 2      /* Event end announcement message */
#define EVENT_DETAIL 3   /* Detailed event information for players */
#define EVENT_SUMMARY 4  /* Event completion summary message */
#define STAFF_EVENT_FIELDS 5  /* Total number of event data fields */

/*****************************************************************************/
/*
 * EVENT INDEX DEFINITIONS
 * These constants define the unique identifiers for each staff event.
 * They serve as array indices and event identifiers throughout the system.
 */
#define UNDEFINED_EVENT -1   /* Marker indicating no active event */
#define JACKALOPE_HUNT 0     /* Event ID for the Hardbuckler Jackalope Hunt */
#define THE_PRISONER_EVENT 1 /* Event ID for The Prisoner raid event */

/*
 * Total number of implemented staff events.
 * This MUST be updated whenever new events are added to the system.
 */
#define NUM_STAFF_EVENTS 2
/*****************************************************************************/

/*
 * EVENT-SPECIFIC CONFIGURATION DEFINES
 * Each event has its own section of defines for customization and tuning.
 */

/*****************************************************************************/
/* JACKALOPE HUNT EVENT CONFIGURATION */
/*****************************************************************************/

/*
 * Mobile VNUMs for the three tiers of Jackalope mobs.
 * Each tier is designed for different level ranges to ensure appropriate
 * challenge scaling across the player base.
 */
#define EASY_JACKALOPE 11391  /* VNUM: Lower level jackalope (levels 1-10) */
#define MED_JACKALOPE 11392   /* VNUM: Mid level jackalope (levels 11-20) */
#define HARD_JACKALOPE 11393  /* VNUM: High level jackalope (levels 21+) */

/*
 * Population control for Jackalope spawning.
 * CRITICAL: Due to wilderness memory pool limitations, keep total mob count
 * under 900 (300 each * 3 types). Higher values may cause memory issues.
 * DO NOT CHANGE without consulting wilderness system capacity.
 */
#define NUM_JACKALOPE_EACH 300

/*
 * Geographic boundaries for Jackalope spawning in the Hardbuckler Region.
 * These coordinates define a rectangular area in the wilderness where
 * Jackalope mobs will be randomly distributed during the event.
 * Coordinates are in wilderness grid system (x,y format).
 */
#define JACKALOPE_NORTH_Y 185  /* Northern boundary (y-coordinate) */
#define JACKALOPE_SOUTH_Y -63  /* Southern boundary (y-coordinate) */
#define JACKALOPE_WEST_X 597   /* Western boundary (x-coordinate) */
#define JACKALOPE_EAST_X 703   /* Eastern boundary (x-coordinate) */

/*
 * Reward and quest item VNUMs for the Jackalope Hunt event.
 * These objects are created and distributed based on event participation
 * and achievement levels.
 */
#define JACKALOPE_HIDE 11366       /* VNUM: Standard jackalope hide drop */
#define PRISTINE_HORN 11368        /* VNUM: Rare pristine jackalope horn */
#define P_HORN_RARITY 5            /* Percentage chance (1-100) for pristine horn drop */
#define PRISTINEHORN_PRIZE 11363   /* VNUM: Prize token for turning in pristine horn */
#define FULLSTAFF_NPC 11449        /* VNUM: Fullstaff NPC, quest giver and prize distributor */
#define JACKALOPE_HIGH_PRIZE 11365 /* VNUM: Grand prize for highest scorer (hunting horn) */
#define GREAT_HUNT_RIBBON 11369    /* VNUM: Participation ribbon for all hunters */

/*****************************************************************************/
/* THE PRISONER EVENT CONFIGURATION */
/*****************************************************************************/

/*
 * Portal and room configuration for The Prisoner raid event.
 * This event creates a temporary portal to allow access to the raid zone.
 */
#define THE_PRISONER_PORTAL 132399  /* VNUM: Portal object for raid access */
#define TP_PORTAL_L_ROOM 145202     /* VNUM: Room where portal is created */

/*****************************************************************************/
/* GENERAL EVENT SYSTEM CONSTANTS */
/*****************************************************************************/

/*
 * Magic number replacements for improved maintainability (M002).
 * These constants replace hardcoded values throughout the event system
 * to make configuration changes easier and code more readable.
 */

/* Random chance and probability constants */
#define PRISONER_ATMOSPHERIC_CHANCE_SKIP 15    /* Skip probability for atmospheric messages (15/16 chance) */
#define PERCENTAGE_DICE_SIDES 100              /* Standard percentage dice roll (1-100) */
#define ATMOSPHERIC_MESSAGE_COUNT 7            /* Total number of atmospheric messages (0-6) */

/* Buffer and string operation constants */
#define EVENT_MESSAGE_BUFFER_SIZE MAX_STRING_LENGTH  /* Buffer size for event message formatting */

/* Coordinate generation and caching constants (M006) */
#define COORD_CACHE_SIZE 1000                      /* Maximum cached coordinate pairs */
#define COORD_BATCH_GENERATION_SIZE 100            /* Coordinates generated per batch */

/* Object pooling constants for performance optimization */
#define OBJECT_POOL_SIZE 50                        /* Maximum pre-allocated objects in pool */
#define JACKALOPE_POOL_SIZE 20                     /* Pool size for Jackalope mobs */
#define PORTAL_POOL_SIZE 5                         /* Pool size for portal objects */

/* Hash table constants for algorithmic optimization */
#define EVENT_HASH_TABLE_SIZE 32                   /* Hash table size for event lookups */
#define MOBILE_HASH_TABLE_SIZE 128                 /* Hash table size for mobile lookups */

/*****************************************************************************/
/* ERROR HANDLING AND RETURN CODES */
/*****************************************************************************/

/*
 * Standardized error codes for event system operations (M003).
 * These provide consistent error reporting across all event functions
 * to improve debugging and error handling reliability.
 */
typedef enum {
  EVENT_SUCCESS = 0,                    /* Operation completed successfully */
  EVENT_ERROR_INVALID_NUM = -1,         /* Invalid event number provided */
  EVENT_ERROR_ALREADY_RUNNING = -2,     /* Event already active, cannot start another */
  EVENT_ERROR_PRECONDITION_FAILED = -3, /* Event-specific precondition not met */
  EVENT_ERROR_DELAY_ACTIVE = -4,        /* Inter-event delay still active */
  EVENT_ERROR_INVALID_DATA = -5,        /* Event data corrupted or missing */
  EVENT_ERROR_NO_ACTIVE_EVENT = -6      /* No event currently active to operate on */
} event_result_t;

/******* end defines */
/***/

/*****************************************************************************/
/* EXTERNAL DATA STRUCTURES AND FUNCTION DECLARATIONS */
/*****************************************************************************/

/*
 * Global event data array containing all event information.
 * This 2D array stores the message strings and data for each event.
 * First dimension: event index (0 to NUM_STAFF_EVENTS-1)
 * Second dimension: event field (EVENT_TITLE, EVENT_BEGIN, etc.)
 */
extern const char *staff_events_list[NUM_STAFF_EVENTS][STAFF_EVENT_FIELDS];

/*****************************************************************************/
/* CORE EVENT MANAGEMENT FUNCTIONS */
/*****************************************************************************/

/*
 * Start a staff event with the specified event number.
 * Performs initialization, broadcasts messages, and sets up event state.
 *
 * @param event_num: Index of the event to start (0 to NUM_STAFF_EVENTS-1)
 *                   Must be a valid event index, otherwise EVENT_ERROR_INVALID_NUM is returned.
 *                   Event data must be properly initialized, otherwise EVENT_ERROR_INVALID_DATA is returned.
 * @return: EVENT_SUCCESS on success, or specific error code on failure
 */
event_result_t start_staff_event(int event_num);

/*
 * End the currently running staff event.
 * Performs cleanup, broadcasts end messages, and resets event state.
 *
 * @param event_num: Index of the event to end
 *                   Must be a valid event index (0 to NUM_STAFF_EVENTS-1),
 *                   otherwise EVENT_ERROR_INVALID_NUM is returned.
 * @return: EVENT_SUCCESS on success, or specific error code on failure
 */
event_result_t end_staff_event(int event_num);

/*
 * Display detailed information about a specific event to a character.
 * Shows event messages, current status, and time remaining.
 *
 * @param ch: Character to display information to (must not be NULL)
 * @param event_num: Index of the event to show info for (0 to NUM_STAFF_EVENTS-1)
 *                   Function returns silently if ch is NULL or event_num is invalid.
 */
void staff_event_info(struct char_data *ch, int event_num);

/*
 * Display a list of all available staff events to a character.
 * Shows event indices and titles for staff command reference.
 *
 * @param ch: Character to display the list to
 */
void list_staff_events(struct char_data *ch);

/*****************************************************************************/
/* EVENT STATE MANAGEMENT FUNCTIONS (M007) */
/*****************************************************************************/

/*
 * State management functions to reduce tight coupling with global variables.
 * These functions provide a clean abstraction layer for event state operations,
 * improving testability and modularity.
 */

/*
 * Set the current event state with event number and duration.
 * @param event_num: Event number to set as active
 * @param duration: Duration in ticks for the event
 */
void set_event_state(int event_num, int duration);

/*
 * Check if an event is currently active.
 * @return: 1 if an event is running, 0 otherwise
 */
int is_event_active(void);

/*
 * Get the currently active event number.
 * @return: Active event number, or UNDEFINED_EVENT if none active
 */
int get_active_event(void);

/*
 * Get the remaining time for the current event.
 * @return: Remaining time in ticks, or 0 if no event active
 */
int get_event_time_remaining(void);

/*
 * Clear the event state (set to inactive).
 */
void clear_event_state(void);

/*
 * Set the inter-event delay timer.
 * @param delay: Delay in ticks before next event can start
 */
void set_event_delay(int delay);

/*
 * Get the current inter-event delay remaining.
 * @return: Delay remaining in ticks
 */
int get_event_delay(void);

/*****************************************************************************/
/* WILDERNESS AND MOBILE MANAGEMENT FUNCTIONS */
/*****************************************************************************/

/*
 * Load a mobile into the wilderness at specified coordinates.
 * Creates the mobile and places it at the given x,y coordinates,
 * handling wilderness room assignment as needed.
 *
 * @param mobile_vnum: Virtual number of the mobile to load (must be valid VNUM)
 * @param x_coord: X coordinate in wilderness grid (no bounds checking performed)
 * @param y_coord: Y coordinate in wilderness grid (no bounds checking performed)
 *                 Function returns silently if mobile creation fails or room allocation fails.
 */
void wild_mobile_loader(int mobile_vnum, int x_coord, int y_coord);

/*
 * Count the number of mobiles with a specific VNUM currently in the game.
 * Used for population control and event mob management.
 *
 * @param mobile_vnum: Virtual number of the mobile to count
 * @return: Number of mobiles found in the game
 */
int mob_ingame_count(int mobile_vnum);

/*
 * Optimized function to count all Jackalope mob types in a single pass.
 * This addresses the critical performance issue where multiple mob_ingame_count()
 * calls were causing excessive character list traversals during events.
 *
 * @param easy_count: Pointer to store Easy Jackalope count (can be NULL)
 * @param med_count: Pointer to store Medium Jackalope count (can be NULL)
 * @param hard_count: Pointer to store Hard Jackalope count (can be NULL)
 *                    NULL pointers are safely handled - corresponding counts are skipped.
 */
void count_jackalope_mobs(int *easy_count, int *med_count, int *hard_count);

/*
 * Remove all mobiles with a specific VNUM from the game.
 * Used for event cleanup and mob population control.
 *
 * @param mobile_vnum: Virtual number of the mobiles to purge
 */
void mob_ingame_purge(int mobile_vnum);

/*****************************************************************************/
/* EVENT MECHANICS AND REWARD FUNCTIONS */
/*****************************************************************************/

/*
 * Check and handle special event drops when a mobile is killed.
 * Called from combat system to process event-specific loot drops
 * based on killer level, victim type, and current event state.
 *
 * @param killer: Character who killed the mobile (can be NPC or PC, NULL checked)
 * @param victim: Mobile that was killed (must be NPC, otherwise function returns)
 *                Function returns early if victim is NULL, not NPC, or no event active.
 */
void check_event_drops(struct char_data *killer, struct char_data *victim);

/*
 * Main event tick function called from the game loop.
 * Handles event timers, periodic spawning, environmental effects,
 * and automatic event ending. Called once per mud hour.
 */
void staff_event_tick();

/*****************************************************************************/
/* PERFORMANCE OPTIMIZATION FUNCTIONS */
/*****************************************************************************/

/*
 * Object pooling system for frequently used objects.
 * Pre-allocates commonly used objects to reduce memory allocation overhead.
 */

/*
 * Initialize the object pooling system.
 * Should be called during system startup.
 */
void init_object_pool(void);

/*
 * Get a pre-allocated object from the pool, or create new if pool empty.
 * @param obj_vnum: Virtual number of the object to get
 * @return: Pointer to object, or NULL if creation fails
 */
struct obj_data *get_pooled_object(int obj_vnum);

/*
 * Return an object to the pool for reuse.
 * @param obj: Object to return to pool (will be reset for reuse)
 */
void return_object_to_pool(struct obj_data *obj);

/*
 * Cleanup the object pooling system.
 * Should be called during system shutdown.
 */
void cleanup_object_pool(void);

/*
 * Hash table system for faster event and mobile lookups.
 * Provides O(1) average case lookup performance.
 */

/*
 * Initialize the hash table systems.
 * Should be called during system startup.
 */
void init_hash_tables(void);

/*
 * Hash table lookup for event information by VNUM.
 * @param event_vnum: Virtual number to look up
 * @return: Event index, or -1 if not found
 */
int hash_lookup_event(int event_vnum);

/*
 * Hash table lookup for mobile count by VNUM.
 * @param mobile_vnum: Virtual number to look up
 * @return: Cached mobile count, or -1 if not cached
 */
int hash_lookup_mobile_count(int mobile_vnum);

/*
 * Update cached mobile count in hash table.
 * @param mobile_vnum: Virtual number of mobile
 * @param count: New count to cache
 */
void hash_update_mobile_count(int mobile_vnum, int count);

/*
 * Cleanup the hash table systems.
 * Should be called during system shutdown.
 */
void cleanup_hash_tables(void);

/*****************************************************************************/
/* TESTING FRAMEWORK */
/*****************************************************************************/

/*
 * Unit testing framework for staff event system.
 * Provides comprehensive testing capabilities for all system components.
 */

/*
 * Run all unit tests for the staff event system.
 * @return: Number of failed tests (0 = all passed)
 */
int run_staff_event_tests(void);

/*
 * Run specific test suite.
 * @param test_suite: Test suite identifier
 * @return: Number of failed tests in suite
 */
int run_test_suite(const char *test_suite);

/*
 * Staff command for running tests: 'testevent'
 * Allows staff to run comprehensive tests on the event system.
 */
ACMD_DECL(do_testevent);

/*****************************************************************************/
/* STAFF COMMAND INTERFACE */
/*****************************************************************************/

/*
 * Staff command for managing events: 'staffevents'
 * Allows staff to start, end, and get info about events.
 * Regular players can only view current event information.
 *
 * Usage: staffevents [start|end|info] [event_number]
 */
ACMD_DECL(do_staffevents);

/****** notes *******/
//#define STAFFRAN_PVAR(ch, variable)
// CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.staff_ran_events[variable]))
/**** end notes *****/

/* EOF */
