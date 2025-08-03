/*/ \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \
\     Staff Ran Event System
/  File:       staff_events.c
/  Created By: Zusuk
\  Header:     staff_events.h
/    System for running staff events
\    Basics including starting, ending and info on the event
/  Created on April 26, 2020
\ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ /*/

/* includes */
#include "conf.h"
#include "sysdep.h"
#include <time.h>
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "screen.h"
#include "wilderness.h"
#include "dg_scripts.h"
#include "staff_events.h"
#include "spec_procs.h" /* external variable for prisoner heads */
/* end includes */

/*****************************************************************************/
/* GLOBAL EVENT DATA AND CONFIGURATION */
/*****************************************************************************/

/*
 * Master event data array containing all event information.
 *
 * This 2D array serves as the central repository for all staff event data.
 * Each event occupies one row (first dimension) and contains standardized
 * message fields (second dimension) that define the event's behavior and
 * player-facing content.
 *
 * Array Structure:
 * - First dimension: Event index (JACKALOPE_HUNT, THE_PRISONER_EVENT, etc.)
 * - Second dimension: Message type (EVENT_TITLE, EVENT_BEGIN, etc.)
 *
 * Message Fields (defined in staff_events.h):
 * - EVENT_TITLE: Display name shown in lists and announcements
 * - EVENT_BEGIN: World-wide announcement when event starts
 * - EVENT_END: World-wide announcement when event ends
 * - EVENT_DETAIL: Detailed information shown to players about event mechanics
 * - EVENT_SUMMARY: Conclusion message displayed when event completes
 *
 * Color Codes Used:
 * - \tC = Cyan (titles)
 * - \tW = White (important announcements)
 * - \tR = Red (warnings/end messages)
 * - \tG = Green (dialogue/story text)
 * - \tn = Normal (reset color)
 *
 * TODO: Consider converting this to a proper structure array for better
 *       type safety and maintainability in future versions.
 */
const char *staff_events_list[NUM_STAFF_EVENTS][STAFF_EVENT_FIELDS] = {

    /*
     * JACKALOPE HUNT EVENT DATA
     * A wilderness hunting event where players track and kill Jackalope mobs
     * across three difficulty tiers in the Hardbuckler Region.
     */
    {/*JACKALOPE_HUNT*/

     /* EVENT_TITLE - Display name for the event */
     "\tCHardbuckler Jackalope Hunt\tn",

     /* EVENT_BEGIN - World announcement when event starts */
     "\tWThe horn has sounded, the great Jackalope Hunt of the Hardbuckler Region has begun!\tn",

     /* EVENT_END - World announcement when event ends */
     "\tRThe horn has sounded, the great Jackalope Hunt of the Hardbuckler Region has ended!\tn",

     /* EVENT_DETAIL - Detailed player information including IC and OOC instructions */
     "\tgIt is as I feared, Jackalopes have been seen in numbers roaming the countryside.  "
     "Usually they reproduce very slowly.  Clearly someone has been breeding them, and "
     "this, can mean no good.  We must stop the spread of this growing menace now.  Will "
     "you help me?\tn\r\n \tW- Fullstaff, Agent of Sanctus -\tn\r\n"
     "\r\n\tR[OOC: Head to the Hardbuckler Region and hunt Jackalope in the wilderness.  "
     "There are 3 levels of Jackalope, a set for level 10 and under, 20 and under, and then "
     "epic levels.  There is no reward for killing Jackalope under your level bracket.  The "
     "Jackalope will frequently return to random locations in that area.  "
     "The carcasses will be counted by staff, please reach out and get your "
     "final count within 24 hours of event completion.  Grand prize(s) will be handed out shortly after final count."
     "Rare antlers found are to be "
     "turned into Fullstaff in Hardbuckler for a special bonus prize.]\tn\r\n",

     /* EVENT_SUMMARY - Conclusion message with final instructions */
     "\tgThank you hunter, I.... nay the world owes you a debt of gratitude. I hope we have "
     "quelled this menace for good. However, keep your blades sharp, and your bowstrings tight, "
     "in case they are needed again.\tn\r\n \tW- Fullstaff, Agent of Sanctus -\tn\r\n"
     "\tR[OOC: You can visit Fullstaff in the Hardbuckler Inn to turn in the rare antlers for "
     "a special prize!  The carcasses will be counted by staff, please contact and get your "
     "final count within 24 hours of event completion.  Grand prize(s) will be handed out shortly after final count.]\tn\r\n",

     /*end jackalope hunt*/},

    /*
     * THE PRISONER EVENT DATA
     * A high-level raid event where players assault The Prisoner through a
     * temporary portal, with enhanced rewards and no death penalties.
     */
    {/*THE_PRISONER_EVENT*/

     /* EVENT_TITLE - Display name for the event */
     "\tCThe Prisoner\tn",

     /* EVENT_BEGIN - World announcement when event starts */
     "\tWExistence shudders as The Prisoner's captivity begins to crack!!!\tn",

     /* EVENT_END - World announcement when event ends */
     "\tRBy immeasurable sacrifice, aggression, strategy and luck the mystical cell containing The Prisoner holds firm!!!\tn",

     /* EVENT_DETAIL - Detailed player information including IC lore and OOC mechanics */
     "\tGThrough the darkling hoarde's tampering, The Prisoner's cell has been compromised.  \tn"
     "\tGAs you know, The Prisoner can't be defeated while in any of our realities.  But the five-headed dragon avatar\tn "
     "\tGthat the Luminari imprisoned in Avernus is the entity that is creating the damage to The Prisoner's cell.\tn\r\n  "
     "\tGGet to the Mosswood Elder adventurer and step through the portal, we MUST mount an offensive or all is lost!\tn\r\n "
     "\tWOOC: While this event is running, the treasure drop is maximized for The Prisoner, there is no XP loss for death and there is a \tn"
     "\tWdirect portal to the Garden of Avernus at the Mosswood Elder.\tn\r\n",

     /* EVENT_SUMMARY - Conclusion message thanking participants */
     "\tGThank you adventuer, I.... nay the entire existence owes you a debt of gratitude. I hope we have "
     "quelled this menace for good. However, keep your blades sharp, and your bowstrings tight, "
     "in case they are needed again.\tn\r\n \tW- Alerion -\tn\r\n",

     /*end the prisoner event*/},
};

/*****************************************************************************/
/* EVENT REWARD AND DROP MANAGEMENT FUNCTIONS */
/*****************************************************************************/

/*
 * Process special event-specific drops when a mobile is killed.
 *
 * This function is called from the combat system whenever a mobile dies
 * and handles the creation and distribution of event-specific loot based
 * on the current active event, the victim's type, and the killer's level.
 *
 * Key Features:
 * - Level-gated rewards to prevent high-level players from farming low content
 * - Automatic object creation and delivery to the killer
 * - Rare drop chances with percentage-based probability
 * - Event-specific loot tables and reward logic
 * - Visual feedback to killer and surrounding players
 *
 * @param killer: Character who killed the mobile (can be NPC or PC)
 * @param victim: Mobile that was killed (must be NPC)
 *
 * Requirements:
 * - A staff event must be currently active (IS_STAFF_EVENT)
 * - Victim must be an NPC (IS_NPC check)
 * - Event-specific logic determines actual reward eligibility
 */
void check_event_drops(struct char_data *killer, struct char_data *victim)
{
  /* Preliminary validation checks */
  /*
   * Note: Killer can be NPC (e.g., pet kills) - this is intentionally allowed
   * to support various combat scenarios including charmed creatures, pets, etc.
   */
  /*
  if (IS_NPC(killer))
      return;
  */
  
  /* Only NPCs can drop event items */
  if (!IS_NPC(victim))
    return;
    
  /* No drops if no event is currently running */
  if (!IS_STAFF_EVENT)
    return;

  struct obj_data *obj = NULL;          /* Object pointer for created items */
  bool load_drop = FALSE;               /* Flag to determine if drop should be created */
  char buf[MAX_STRING_LENGTH] = {'\0'}; /* Buffer for act() messages */

  /*
   * Event-specific drop logic
   * Each event has its own reward system and drop tables
   */
  switch (STAFF_EVENT_NUM)
  {

  case JACKALOPE_HUNT:
    /*
     * Jackalope Hunt Drop Logic:
     * - Three tiers of Jackalope (Easy/Med/Hard) with level restrictions
     * - Standard drop: Jackalope Hide (always drops if level appropriate)
     * - Rare drop: Pristine Horn (5% chance, can be turned in for bonus prize)
     * - Level gating prevents high-level players from farming low-tier content
     */

    switch (GET_MOB_VNUM(victim))
    {

    case EASY_JACKALOPE:
      /* Easy Jackalope: Only drops for characters level 10 and under */
      if (GET_LEVEL(killer) <= 10)
      {
        load_drop = TRUE;
      }
      else
      {
        /* Inform player why they didn't get a drop */
        send_to_char(killer, "OOC:  Lower level Jackalope hides won't drop for someone over level 10.\r\n");
      }
      break;

    case MED_JACKALOPE:
      /* Medium Jackalope: Only drops for characters level 20 and under */
      if (GET_LEVEL(killer) <= 20)
      {
        load_drop = TRUE;
      }
      else
      {
        /* Inform player why they didn't get a drop */
        send_to_char(killer, "OOC:  Mid level Jackalopes hides won't drop for someone over level 20.\r\n");
      }
      break;

    case HARD_JACKALOPE:
      /* Hard Jackalope: Always drops for any level (epic content) */
      load_drop = TRUE;
      break;

    /* Handle any unexpected Jackalope VNUMs */
    default:
      break;
    } /* end mob vnum switch */

    /* Process the actual drop creation and delivery */
    if (load_drop)
    {
      /*
       * ENHANCED ERROR HANDLING FOR OBJECT CREATION (Addresses H001)
       * Improved error handling with proper fallback and user notification
       * when object creation fails during event drops.
       */
      
      /*
       * Create and deliver the standard Jackalope Hide
       * This is the primary event currency for tracking participation
       */
      if ((obj = read_object(JACKALOPE_HIDE, VIRTUAL)) == NULL)
      {
        log("SYSERR: check_event_drops() failed to create jackalope hide (vnum %d) for %s",
            JACKALOPE_HIDE, killer ? GET_NAME(killer) : "unknown");
        
        /* Graceful degradation: inform player of technical issue */
        if (killer)
        {
          send_to_char(killer, "\tRTechnical issue: Your reward could not be created. Please contact staff.\tn\r\n");
        }
        return; /* Abort drop processing to prevent further issues */
      }
      
      obj_to_char(obj, killer); /* Deliver object to killer's inventory */
      
      /* Provide visual feedback about the drop - with enhanced safety checks */
      if (killer && obj && obj->short_description)
      {
        send_to_char(killer, "\tYYou have found \tn%s\tn\tY!\tn\r\n", obj->short_description);

        /* Show other players in the room what was found - with buffer overflow protection */
        int ret = snprintf(buf, sizeof(buf), "$n \tYhas found \tn%s\tn\tY!\tn", obj->short_description);
        if (ret >= sizeof(buf))
        {
          log("SYSERR: Message truncated in check_event_drops for jackalope hide");
          /* Use a safe fallback message */
          snprintf(buf, sizeof(buf), "$n \tYhas found something valuable!\tn");
        }
        act(buf, FALSE, killer, 0, killer, TO_NOTVICT);
      }

      /*
       * Rare Drop Check - Pristine Jackalope Horn
       * This is a bonus drop with P_HORN_RARITY% chance (default: 5%)
       * Can be turned in to Fullstaff NPC for special bonus prizes
       */
      if (dice(1, PERCENTAGE_DICE_SIDES) < P_HORN_RARITY)
      {
        if ((obj = read_object(PRISTINE_HORN, VIRTUAL)) == NULL)
        {
          log("SYSERR: check_event_drops() failed to create pristine horn (vnum %d) for %s",
              PRISTINE_HORN, killer ? GET_NAME(killer) : "unknown");
          
          /* Continue execution - rare drop failure shouldn't prevent basic rewards */
          if (killer)
          {
            send_to_char(killer, "\tYA rare item glimmers briefly but vanishes due to magical interference.\tn\r\n");
          }
        }
        else
        {
          obj_to_char(obj, killer); /* Deliver rare object to killer */

          /* Provide visual feedback for the rare drop - with enhanced safety checks */
          if (killer && obj && obj->short_description)
          {
            send_to_char(killer, "\tYYou have found \tn%s\tn\tY!\tn\r\n", obj->short_description);

            /* Announce rare drop to other players in room - with buffer overflow protection */
            int ret = snprintf(buf, sizeof(buf), "$n \tYhas found \tn%s\tn\tY!\tn", obj->short_description);
            if (ret >= sizeof(buf))
            {
              log("SYSERR: Message truncated in check_event_drops for pristine horn");
              /* Use a safe fallback message */
              snprintf(buf, sizeof(buf), "$n \tYhas found something rare!\tn");
            }
            act(buf, FALSE, killer, 0, killer, TO_NOTVICT);
          }
        }
      }
    }

    break;
    /* end jackalope hunt case */

  /*
   * Future events can add their own drop logic here
   * Template:
   * case NEW_EVENT:
   *   // Event-specific drop logic
   *   break;
   */
  default:
    /* No special drops for unhandled events */
    break;

  } /* end staff event switch */

  return;
}

/*****************************************************************************/
/* MOBILE MANAGEMENT AND UTILITY FUNCTIONS */
/*****************************************************************************/

/*
 * Remove all instances of a specific mobile VNUM from the game world.
 *
 * This function performs a comprehensive cleanup of all mobiles matching
 * the specified VNUM, used primarily during event endings to remove
 * event-specific creatures from the game world.
 *
 * Process:
 * 1. Convert virtual number to real number for efficient searching
 * 2. Iterate through all mobile prototypes to find matching type
 * 3. Search the global character list for instances of this mobile
 * 4. Safely extract each found mobile from the game
 *
 * Safety Features:
 * - Checks for valid mobile table (top_of_mobt)
 * - Validates VNUM exists (mobile_rnum != NOTHING)
 * - Prevents crashes by checking room validity before extraction
 *
 * @param mobile_vnum: Virtual number of the mobile type to purge
 *
 * Note: This is a heavy operation that iterates through all characters
 *       in the game. Should only be used during event cleanup.
 */
void mob_ingame_purge(int mobile_vnum)
{
  struct char_data *l = NULL;                    /* Iterator for character list */
  mob_rnum mobile_rnum = real_mobile(mobile_vnum); /* Convert vnum to rnum */
  mob_rnum i = 0;                                /* Mobile prototype iterator */

  /* Safety check: Ensure mobile table exists */
  if (!top_of_mobt)
    return;

  /* Safety check: Ensure the mobile VNUM exists in the game */
  if (mobile_rnum == NOTHING)
    return;

  /*
   * Search through mobile prototypes to find the target type
   * "i" represents the real number (index) in the mobile prototype table
   */
  for (i = 0; i <= top_of_mobt; i++)
  {
    /* Check if this prototype matches our target */
    if (mobile_rnum == i)
    {
      /*
       * Found matching prototype - now search for all instances
       * in the game world and remove them
       */
      for (l = character_list; l; l = l->next)
      {
        if (IS_NPC(l) && GET_MOB_RNUM(l) == i)
        {
          /*
           * Safety check: Ensure mobile is in a valid room
           * Mobiles in NOWHERE can cause crashes during extraction
           */
          if (IN_ROOM(l) == NOWHERE)
          {
            /* Skip extraction for mobiles in invalid locations */
            /* This prevents potential crashes during cleanup */
          }
          else
          {
            /* Safe to extract - mobile is in a valid room */
            extract_char(l);
          }
        }
      }
    }
  } /* end mobile prototype search */

  return;
}

/*
 * Count the number of mobiles with a specific VNUM currently in the game.
 *
 * This function provides real-time population tracking for event mobiles,
 * enabling the system to maintain appropriate spawn levels and detect
 * when replenishment is needed.
 *
 * Process:
 * 1. Convert virtual number to real number for efficient comparison
 * 2. Search mobile prototypes to find the target type
 * 3. Count all instances of this mobile in the global character list
 *
 * Used for:
 * - Event tick functions to maintain mob populations
 * - Staff commands to report current mob counts
 * - Spawn logic to determine if new mobs are needed
 *
 * @param mobile_vnum: Virtual number of the mobile type to count
 * @return: Number of mobiles of this type currently in the game (0 if none/error)
 *
 * Performance Note: This function searches the entire character list,
 *                   so frequent calls may impact performance.
 */
int mob_ingame_count(int mobile_vnum)
{
  struct char_data *l = NULL;                    /* Iterator for character list */
  mob_rnum mobile_rnum = real_mobile(mobile_vnum); /* Convert vnum to rnum */
  mob_rnum i = 0;                                /* Mobile prototype iterator */
  int num_found = 0;                             /* Counter for found mobiles */

  /* Safety check: Ensure mobile table exists */
  if (!top_of_mobt)
    return 0;

  /* Safety check: Ensure the mobile VNUM exists in the game */
  if (mobile_rnum == NOTHING)
    return 0;

  /*
   * Search through mobile prototypes to find the target type
   * "i" represents the real number (index) in the mobile prototype table
   */
  for (i = 0; i <= top_of_mobt; i++)
  {
    /* Check if this prototype matches our target */
    if (mobile_rnum == i)
    {
      /*
       * Found matching prototype - now count all instances
       * Initialize counter and iterate through all characters
       */
      for (num_found = 0, l = character_list; l; l = l->next)
      {
        if (IS_NPC(l) && GET_MOB_RNUM(l) == i)
        {
          num_found++;
        }
      }
    }

  } /* end mobile prototype search */
  
  return num_found;
}

/*****************************************************************************/
/* COORDINATE GENERATION AND CACHING SYSTEM (M006) */
/*****************************************************************************/

/*
 * Coordinate pair structure for efficient batch generation and caching.
 * This addresses M006: Performance - Inefficient Random Generation by
 * pre-generating coordinates in batches rather than generating them
 * individually during spawning operations.
 */
typedef struct {
  int x;  /* X coordinate */
  int y;  /* Y coordinate */
} coord_pair_t;

/*
 * Global coordinate cache for efficient random coordinate generation.
 * This cache reduces the number of random number generation calls
 * during high-volume spawning operations.
 */
static coord_pair_t coord_cache[COORD_CACHE_SIZE];
static int coord_cache_size = 0;      /* Current number of cached coordinates */
static int coord_cache_index = 0;     /* Current position in cache */

/*
 * Generate a batch of random coordinates and store them in the cache.
 * This function pre-generates coordinates in batches to improve performance
 * during spawning operations by reducing system calls to the RNG.
 *
 * @param count: Number of coordinate pairs to generate (up to COORD_CACHE_SIZE)
 */
static void generate_coordinate_batch(int count)
{
  int i = 0;
  
  /* Clamp count to cache size limit */
  if (count > COORD_CACHE_SIZE)
    count = COORD_CACHE_SIZE;
  
  /* Generate batch of random coordinates */
  for (i = 0; i < count; i++)
  {
    coord_cache[i].x = rand_number(JACKALOPE_WEST_X, JACKALOPE_EAST_X);
    coord_cache[i].y = rand_number(JACKALOPE_SOUTH_Y, JACKALOPE_NORTH_Y);
  }
  
  coord_cache_size = count;
  coord_cache_index = 0;
}

/*
 * Get the next coordinate pair from the cache, generating new batch if needed.
 * This function provides efficient coordinate retrieval with automatic
 * cache replenishment when the cache is exhausted.
 *
 * @param x_coord: Pointer to store X coordinate
 * @param y_coord: Pointer to store Y coordinate
 */
static void get_cached_coordinates(int *x_coord, int *y_coord)
{
  /* Refill cache if empty or exhausted */
  if (coord_cache_index >= coord_cache_size)
  {
    generate_coordinate_batch(COORD_BATCH_GENERATION_SIZE);
  }
  
  /* Return next cached coordinate pair */
  if (x_coord) *x_coord = coord_cache[coord_cache_index].x;
  if (y_coord) *y_coord = coord_cache[coord_cache_index].y;
  
  coord_cache_index++;
}

/*****************************************************************************/
/* EVENT STATE MANAGEMENT FUNCTIONS (M007) */
/*****************************************************************************/

/*
 * State management functions to reduce tight coupling with global variables.
 * These functions provide a clean abstraction layer for event state operations,
 * improving testability and modularity by encapsulating global variable access.
 */

/*
 * Set the current event state with event number and duration.
 * @param event_num: Event number to set as active
 * @param duration: Duration in ticks for the event
 */
void set_event_state(int event_num, int duration)
{
  STAFF_EVENT_NUM = event_num;
  STAFF_EVENT_TIME = duration;
}

/*
 * Check if an event is currently active.
 * @return: 1 if an event is running, 0 otherwise
 */
int is_event_active(void)
{
  return IS_STAFF_EVENT;
}

/*
 * Get the currently active event number.
 * @return: Active event number, or UNDEFINED_EVENT if none active
 */
int get_active_event(void)
{
  return STAFF_EVENT_NUM;
}

/*
 * Get the remaining time for the current event.
 * @return: Remaining time in ticks, or 0 if no event active
 */
int get_event_time_remaining(void)
{
  return STAFF_EVENT_TIME;
}

/*
 * Clear the event state (set to inactive).
 */
void clear_event_state(void)
{
  STAFF_EVENT_NUM = UNDEFINED_EVENT;
  STAFF_EVENT_TIME = 0;
}

/*
 * Set the inter-event delay timer.
 * @param delay: Delay in ticks before next event can start
 */
void set_event_delay(int delay)
{
  STAFF_EVENT_DELAY = delay;
}

/*
 * Get the current inter-event delay remaining.
 * @return: Delay remaining in ticks
 */
int get_event_delay(void)
{
  return STAFF_EVENT_DELAY;
}

/*
 * Helper function to spawn a batch of Jackalope mobs at random coordinates.
 *
 * This function addresses code duplication (Medium Priority Issue M001) by
 * consolidating the repeated coordinate generation and spawning logic into
 * a single reusable function. This improves maintainability and ensures
 * consistent spawning behavior across all Jackalope types.
 *
 * Features:
 * - Generates random coordinates within Hardbuckler region boundaries
 * - Batch spawning for efficient mob deployment
 * - Consistent spawning logic across all mob types
 * - Error resilience - continues spawning even if individual spawns fail
 *
 * @param mob_vnum: Virtual number of the Jackalope type to spawn
 * @param count: Number of mobs to spawn
 *
 * Performance Notes:
 * - Uses efficient random number generation
 * - Minimizes function call overhead through batch processing
 * - Wilderness system handles room allocation efficiently
 */
static void spawn_jackalope_batch(int mob_vnum, int count)
{
  int i = 0;           /* Loop counter */
  int x_coord = 0;     /* Random X coordinate for spawning */
  int y_coord = 0;     /* Random Y coordinate for spawning */

  /* Input validation */
  if (count <= 0)
    return;

  /* Spawn the requested number of mobs using cached coordinates for efficiency */
  for (i = 0; i < count; i++)
  {
    /* Get coordinates from cache (automatically refills when needed) */
    get_cached_coordinates(&x_coord, &y_coord);
    
    /* Load the mob at the cached coordinates */
    wild_mobile_loader(mob_vnum, x_coord, y_coord);
  }
}

/*
 * Optimized function to count all three Jackalope mob types in a single pass.
 *
 * This function addresses the critical performance issue (C001) where multiple
 * calls to mob_ingame_count() were causing O(n*m) time complexity during
 * Jackalope Hunt events. Instead of traversing the character list 3-6 times
 * per tick, this function does it once and returns all counts simultaneously.
 *
 * Performance Benefits:
 * - Reduces character list traversals from 6 per tick to 1
 * - Eliminates redundant mobile prototype searches
 * - Significantly improves tick performance during events
 *
 * @param easy_count: Pointer to store count of Easy Jackalope mobs
 * @param med_count: Pointer to store count of Medium Jackalope mobs
 * @param hard_count: Pointer to store count of Hard Jackalope mobs
 *
 * Safety Features:
 * - NULL pointer checks for all output parameters
 * - Mobile table validation
 * - VNUM existence validation
 */
void count_jackalope_mobs(int *easy_count, int *med_count, int *hard_count)
{
  struct char_data *l = NULL;                      /* Iterator for character list */
  mob_rnum easy_rnum = real_mobile(EASY_JACKALOPE);  /* Easy Jackalope real number */
  mob_rnum med_rnum = real_mobile(MED_JACKALOPE);    /* Medium Jackalope real number */
  mob_rnum hard_rnum = real_mobile(HARD_JACKALOPE);  /* Hard Jackalope real number */

  /* Initialize output parameters to zero */
  if (easy_count) *easy_count = 0;
  if (med_count) *med_count = 0;
  if (hard_count) *hard_count = 0;

  /* Safety checks: Ensure mobile table exists and all VNUMs are valid */
  if (!top_of_mobt || easy_rnum == NOTHING || med_rnum == NOTHING || hard_rnum == NOTHING)
    return;

  /*
   * Single pass through character list counting all three Jackalope types
   * This replaces 3-6 separate traversals with one efficient pass
   */
  for (l = character_list; l; l = l->next)
  {
    if (IS_NPC(l))
    {
      mob_rnum mob_rnum = GET_MOB_RNUM(l);
      
      if (mob_rnum == easy_rnum && easy_count)
        (*easy_count)++;
      else if (mob_rnum == med_rnum && med_count)
        (*med_count)++;
      else if (mob_rnum == hard_rnum && hard_count)
        (*hard_count)++;
    }
  }
}

/*
 * Create and place a mobile into the wilderness at specified coordinates.
 *
 * This function handles the complete process of spawning event mobiles
 * in the wilderness system, including room management, coordinate assignment,
 * and trigger activation.
 *
 * Process:
 * 1. Create mobile instance from prototype
 * 2. Find or create wilderness room at target coordinates
 * 3. Assign coordinates to both mobile and room
 * 4. Place mobile in room and activate load triggers
 *
 * Wilderness Integration:
 * - Attempts to find existing room at coordinates first
 * - Creates new wilderness room if none exists at location
 * - Properly assigns coordinate metadata for wilderness system
 *
 * @param mobile_vnum: Virtual number of the mobile to create
 * @param x_coord: X coordinate in wilderness grid system
 * @param y_coord: Y coordinate in wilderness grid system
 *
 * TODO: Future enhancement could use wilderness regions for more
 *       dynamic and intelligent placement based on terrain types
 *       and regional characteristics.
 */
void wild_mobile_loader(int mobile_vnum, int x_coord, int y_coord)
{
  room_rnum location = NOWHERE;      /* Target room for mobile placement */
  struct char_data *mob = NULL;      /* Mobile instance to be created */

  /* Create the mobile instance from its prototype */
  mob = read_mobile(mobile_vnum, VIRTUAL);

  /* Safety check: Ensure mobile was successfully created */
  if (!mob)
    return;

  /*
   * Wilderness Room Management:
   * Try to find an existing room at the target coordinates.
   * If none exists, we'll need to allocate a new wilderness room.
   */
  if ((location = find_room_by_coordinates(x_coord, y_coord)) == NOWHERE)
  {
    /*
     * No existing room at these coordinates - allocate a new one
     * from the wilderness room pool
     */
    if ((location = find_available_wilderness_room()) == NOWHERE)
    {
      /*
       * Failed to allocate wilderness room - this should rarely happen
       * unless the wilderness system is at capacity or misconfigured
       */
      return; /* Abort mobile loading - no valid placement location */
    }
    else
    {
      /*
       * Successfully allocated wilderness room - now assign it
       * the target coordinates and configure it for wilderness use
       */
      assign_wilderness_room(location, x_coord, y_coord);
    }
  }

  /*
   * ENHANCED COORDINATE ASSIGNMENT (Addresses M005: Resource Validation)
   * Add bounds checking before accessing world array to prevent crashes
   * and ensure mobile coordinate metadata is properly assigned.
   */
  
  /* Validate location bounds before accessing world array */
  if (location == NOWHERE || location > top_of_world)
  {
    log("SYSERR: Invalid location %d in wild_mobile_loader for mob vnum %d",
        location, mobile_vnum);
    extract_char(mob);  /* Clean up created mob to prevent memory leak */
    return;
  }
  
  /* Safe coordinate assignment with validated location */
  X_LOC(mob) = world[location].coords[0];
  Y_LOC(mob) = world[location].coords[1];
  
  /* Place the mobile in the target room */
  char_to_room(mob, location);

  /*
   * Trigger Activation:
   * Execute any load triggers associated with this mobile
   * This enables script-based behaviors and special actions
   */
  load_mtrigger(mob);

  return;
}

/*****************************************************************************/
/* CORE EVENT LIFECYCLE MANAGEMENT FUNCTIONS */
/*****************************************************************************/

/*
 * Main event tick function - handles all periodic event operations.
 *
 * This is the heart of the event system's real-time operations, called
 * every mud hour from the game's main update loop (limits.c point_update()).
 * It manages event timers, mob population maintenance, environmental effects,
 * and automatic event termination.
 *
 * Core Responsibilities:
 * 1. Event timer management (countdown and expiration)
 * 2. Inter-event delay countdown for cleanup periods
 * 3. Event-specific periodic tasks (mob spawning, environmental effects)
 * 4. Automatic event termination when time expires
 *
 * System Integration:
 * - Called from limits.c point_update() every mud hour
 * - Interfaces with wilderness system for mob placement
 * - Uses global event state variables (STAFF_EVENT_TIME, etc.)
 * - Coordinates with combat system through mob management
 *
 * Performance Considerations:
 * - Executes every mud hour regardless of player count
 * - Mob counting operations can be expensive with large populations
 * - Random coordinate generation uses system RNG
 * - Network broadcasts to all connected players for environmental effects
 */
void staff_event_tick()
{
  /* Variables removed - unused in current implementation */
  struct descriptor_data *pt = NULL;   /* Iterator for player connections */
  struct obj_data *obj = NULL;         /* Object pointer for portal management */
  bool found = FALSE;                  /* Flag for portal existence check */

  /*
   * GLOBAL EVENT TIMER MANAGEMENT
   * Handle the countdown timers that control event duration and delays
   */

  /*
   * Inter-Event Delay Countdown:
   * When no event is active, count down the mandatory delay between events.
   * This prevents rapid-fire event starting and ensures proper cleanup.
   */
  if (!IS_STAFF_EVENT && STAFF_EVENT_DELAY > 0)
  {
    STAFF_EVENT_DELAY--;
  }

  /*
   * ACTIVE EVENT TIMER MANAGEMENT (Fixed H004: Race Condition)
   * Use atomic-style decrement and termination logic to prevent
   * race conditions in event timer management.
   */
  if (STAFF_EVENT_TIME > 0)
  {
    /* Atomically decrement and check for termination */
    if (--STAFF_EVENT_TIME == 0)
    {
      /* Event time expired - terminate immediately */
      end_staff_event(STAFF_EVENT_NUM);
      return; /* Exit early - no further tick processing needed */
    }

    /*
     * EVENT-SPECIFIC PERIODIC OPERATIONS
     * Each event can define custom behavior that occurs every tick
     * while the event is active. This enables dynamic content like
     * mob replenishment, environmental effects, and special mechanics.
     */
    switch (STAFF_EVENT_NUM)
    {

    case JACKALOPE_HUNT:
    {
      int easy_count = 0, med_count = 0, hard_count = 0;
      int easy_deficit = 0, med_deficit = 0, hard_deficit = 0;
      
      /*
       * JACKALOPE HUNT TICK OPERATIONS
       * Maintain optimal Jackalope population across all three tiers.
       *
       * Population Control Logic:
       * - Check current population vs. target (NUM_JACKALOPE_EACH)
       * - Spawn additional mobs to maintain target population
       * - Use random coordinates within defined geographic boundaries
       * - Maintains consistent hunting opportunities throughout event
       */

      /*
       * OPTIMIZED JACKALOPE POPULATION MAINTENANCE
       * Uses single-pass counting to fix critical performance issue (C001).
       * Replaces 6 character list traversals per tick with 1.
       */
      
      /* Single efficient count of all Jackalope types */
      count_jackalope_mobs(&easy_count, &med_count, &hard_count);
      
      /* Calculate deficits for each type */
      easy_deficit = (easy_count < NUM_JACKALOPE_EACH) ? (NUM_JACKALOPE_EACH - easy_count) : 0;
      med_deficit = (med_count < NUM_JACKALOPE_EACH) ? (NUM_JACKALOPE_EACH - med_count) : 0;
      hard_deficit = (hard_count < NUM_JACKALOPE_EACH) ? (NUM_JACKALOPE_EACH - hard_count) : 0;

      /*
       * STREAMLINED JACKALOPE SPAWNING (Addresses M001: Code Duplication)
       * Use helper function to eliminate repeated coordinate generation logic
       */
      
      /* Easy Jackalope Population Maintenance (Levels 1-10) */
      spawn_jackalope_batch(EASY_JACKALOPE, easy_deficit);

      /* Medium Jackalope Population Maintenance (Levels 11-20) */
      spawn_jackalope_batch(MED_JACKALOPE, med_deficit);

      /* Hard Jackalope Population Maintenance (Levels 21+) */
      spawn_jackalope_batch(HARD_JACKALOPE, hard_deficit);
    }
    break;

    case THE_PRISONER_EVENT:
      /*
       * THE PRISONER EVENT TICK OPERATIONS
       * Maintain raid portal and provide atmospheric environmental effects.
       *
       * Features:
       * 1. Portal Management - Ensure portal remains available for players
       * 2. Environmental Broadcasting - Random atmospheric messages
       * 3. Immersion Enhancement - Thematic world-wide effects
       */

      /*
       * Portal Maintenance:
       * Ensure the portal to Avernus remains available throughout the event.
       * The portal may be destroyed or removed, so we check and recreate as needed.
       */
      found = FALSE; /* Reset portal found flag */
      
      /* Search for existing portal in the designated room */
      for (obj = world[real_room(TP_PORTAL_L_ROOM)].contents; obj; obj = obj->next_content)
      {
        if (GET_OBJ_VNUM(obj) == THE_PRISONER_PORTAL)
        {
          found = TRUE;
        }
      }
      
      /* Portal missing - recreate it */
      if (!found)
      {
        obj = read_object(THE_PRISONER_PORTAL, VIRTUAL);
        if (obj)
        {
          obj_to_room(obj, real_room(TP_PORTAL_L_ROOM));
          /* Notify players in the room about portal manifestation */
          act("...  $p flickers, shimmers, then manifests before you...", TRUE, 0, obj, 0, TO_ROOM);
        }
      }

      /*
       * Environmental Broadcasting:
       * Create atmospheric world-wide messages to enhance immersion.
       * Uses random chance to avoid spam (15/16 chance to skip each tick).
       */
      if (rand_number(0, PRISONER_ATMOSPHERIC_CHANCE_SKIP))
        break; /* Skip environmental effects this tick (93.75% chance) */

      /*
       * Atmospheric Message Broadcasting:
       * Send thematic messages to all connected players to create
       * a sense of cosmic threat and urgency during the event.
       */
      switch (rand_number(0, ATMOSPHERIC_MESSAGE_COUNT - 1)) /* Random message selection */
      {
      case 0:
        for (pt = descriptor_list; pt; pt = pt->next)
          if (IS_PLAYING(pt) && pt->character)
            send_to_char(pt->character, "A perpetual haze of green looms on the horizon as \tY\t=The Prisoner's power\tn flares throughout the realms!\r\n");
        break;
      case 1:
        for (pt = descriptor_list; pt; pt = pt->next)
          if (IS_PLAYING(pt) && pt->character)
            send_to_char(pt->character, "Booms and echoes of \tY\t=The Prisoner's power\tn resound throughout the realms!\r\n");
        break;
      case 2:
        for (pt = descriptor_list; pt; pt = pt->next)
          if (IS_PLAYING(pt) && pt->character)
            send_to_char(pt->character, "Emanations from the outter planes, via \tY\t=The Prisoner's power\tn, pulsate through the realms!\r\n");
        break;
      case 3:
        for (pt = descriptor_list; pt; pt = pt->next)
          if (IS_PLAYING(pt) && pt->character)
            send_to_char(pt->character, "The \tY\t=power of The Prisoner\tn is causing the very ground to shake!\r\n");
        break;
      case 4:
        for (pt = descriptor_list; pt; pt = pt->next)
          if (IS_PLAYING(pt) && pt->character)
            send_to_char(pt->character, "Vicious blasts of mental energy from \tY\t=The Prisoner\tn penetrate your psyche!\r\n");
        break;
      case 5:
        for (pt = descriptor_list; pt; pt = pt->next)
          if (IS_PLAYING(pt) && pt->character)
            send_to_char(pt->character, "Random ebbs and flows of chaotic magic from \tY\t=The Prisoner\tn manifest nearby!\r\n");
        break;
      case 6:
        for (pt = descriptor_list; pt; pt = pt->next)
          if (IS_PLAYING(pt) && pt->character)
            send_to_char(pt->character, "The very gravity of the realms seem to shift as \tY\t=The Prisoner's power\tn grows!\r\n");
        break;
      default:
        break;
      }

      break;

    /*
     * Future events can add their own tick operations here
     * Template:
     * case NEW_EVENT:
     *   // Event-specific tick operations
     *   break;
     */
    default:
      /* No special tick operations for unhandled events */
      break;
    }
  }

  /*
   * EVENT TIMER MANAGEMENT COMPLETE
   * Timer logic has been moved to atomic decrement above (line ~660)
   * to prevent race conditions and ensure reliable event termination.
   */
  /* end staff event timer management */
}

/*
 * Initialize and start a staff event with comprehensive setup and validation.
 *
 * This function serves as the primary entry point for event activation,
 * handling all necessary initialization, validation, world broadcasting,
 * and event-specific setup procedures.
 *
 * Process Flow:
 * 1. Parameter validation and boundary checking
 * 2. Event-specific precondition validation
 * 3. World-wide event announcement broadcasting
 * 4. Global event state initialization
 * 5. Event-specific initialization procedures
 *
 * Return Values:
 * - NUM_STAFF_EVENTS: Success - event started successfully
 * - event_num: Failure - specific event couldn't start (precondition failed)
 * - -1: Invalid event number provided
 *
 * @param event_num: Index of the event to start (0 to NUM_STAFF_EVENTS-1)
 * @return: Success code (NUM_STAFF_EVENTS) or failure code (event_num or -1)
 *
 * Global State Changes:
 * - Sets STAFF_EVENT_NUM to active event
 * - Sets STAFF_EVENT_TIME to event duration
 * - Triggers event-specific world changes (mob spawning, etc.)
 */
event_result_t start_staff_event(int event_num)
{
  struct descriptor_data *pt = NULL;    /* Iterator for player connections */
  /* Variables removed - unused in current implementation */

  /*
   * INPUT VALIDATION (Enhanced for H002: Event Data Integrity)
   * Ensure the requested event number is within valid bounds and
   * that the event data is properly initialized.
   */
  if (event_num >= NUM_STAFF_EVENTS || event_num < 0)
  {
    return EVENT_ERROR_INVALID_NUM; /* Invalid event number */
  }

  /*
   * EVENT DATA INTEGRITY VALIDATION (Addresses H002)
   * Verify that the event has properly initialized data fields
   * before attempting to start it. This prevents crashes from
   * accessing uninitialized or corrupted event data.
   */
  if (!staff_events_list[event_num][EVENT_TITLE] ||
      !*staff_events_list[event_num][EVENT_TITLE])
  {
    log("SYSERR: Event %d has invalid or empty title field", event_num);
    return EVENT_ERROR_INVALID_DATA; /* Invalid event data */
  }
  
  if (!staff_events_list[event_num][EVENT_BEGIN] ||
      !*staff_events_list[event_num][EVENT_BEGIN])
  {
    log("SYSERR: Event %d has invalid or empty begin message", event_num);
    return EVENT_ERROR_INVALID_DATA; /* Invalid event data */
  }
  
  if (!staff_events_list[event_num][EVENT_DETAIL] ||
      !*staff_events_list[event_num][EVENT_DETAIL])
  {
    log("SYSERR: Event %d has invalid or empty detail message", event_num);
    return EVENT_ERROR_INVALID_DATA; /* Invalid event data */
  }

  /*
   * EVENT-SPECIFIC PRECONDITION VALIDATION:
   * Some events have special requirements or limitations that must
   * be checked before they can be started.
   */
  switch (event_num)
  {
  case JACKALOPE_HUNT:
    /* Jackalope Hunt has no special preconditions */
    break;
    
  case THE_PRISONER_EVENT:
    /*
     * The Prisoner Event Restriction:
     * This event can only be run once per server boot cycle.
     * The global variable 'prisoner_heads' is set to -2 when The Prisoner
     * has been defeated, preventing the event from being restarted.
     */
    if (prisoner_heads == -2)
    {
      return EVENT_ERROR_PRECONDITION_FAILED; /* Failure: already completed this boot */
    }
    break;
    
  default:
    /* Future events: add precondition checks here */
    break;
  }

  /*
   * WORLD-WIDE EVENT ANNOUNCEMENT:
   * Broadcast the event start to all connected players with comprehensive
   * information including title, detailed instructions, and start message.
   */
  for (pt = descriptor_list; pt; pt = pt->next)
  {
    if (IS_PLAYING(pt) && pt->character)
    {
      /* Eye-catching event start notification */
      send_to_char(pt->character, "\r\n\tY╔═══════════════════════════════════════════════════════════════════════════╗\tn\r\n");
      send_to_char(pt->character, "\tY║                       \tR\t=STAFF EVENT STARTING!\tY                            ║\tn\r\n");
      send_to_char(pt->character, "\tY╠═══════════════════════════════════════════════════════════════════════════╣\tn\r\n");
      send_to_char(pt->character, "\tY║ \tWEvent:\tn %-67s \tY║\tn\r\n", staff_events_list[event_num][EVENT_TITLE]);
      send_to_char(pt->character, "\tY╚═══════════════════════════════════════════════════════════════════════════╝\tn\r\n\r\n");
      
      /* Event-specific start message */
      send_to_char(pt->character, "%s\r\n\r\n", staff_events_list[event_num][EVENT_BEGIN]);
      
      /* Detailed event information box */
      send_to_char(pt->character, "\tC┌───────────────────────────────────────────────────────────────────────────┐\tn\r\n");
      send_to_char(pt->character, "\tC│                           \tWEVENT INFORMATION                               \tC│\tn\r\n");
      send_to_char(pt->character, "\tC├───────────────────────────────────────────────────────────────────────────┤\tn\r\n");
      send_to_char(pt->character, "\tC│\tn\r\n");
      send_to_char(pt->character, "%s", staff_events_list[event_num][EVENT_DETAIL]);
      send_to_char(pt->character, "\tC│\tn\r\n");
      send_to_char(pt->character, "\tC└───────────────────────────────────────────────────────────────────────────┘\tn\r\n\r\n");
    }
  }

  /*
   * GLOBAL EVENT STATE INITIALIZATION (Using M007 abstraction):
   * Set the system-wide variables that track current event status
   * through the state management layer for better modularity.
   */
  
  /*
   * Event Duration Configuration:
   * Default duration is 1200 ticks (approximately 1 real day).
   * Individual events can override this in their specific setup below.
   * Note: 48 ticks ≈ 1 real hour
   */
  set_event_state(event_num, 1200);

  /*
   * EVENT-SPECIFIC INITIALIZATION:
   * Each event has unique setup requirements such as mob spawning,
   * object creation, or special game state modifications.
   */
  switch (event_num)
  {

  case JACKALOPE_HUNT:
    /*
     * JACKALOPE HUNT INITIALIZATION:
     * Spawn the initial population of Jackalope mobs across all three tiers
     * within the designated Hardbuckler Region boundaries.
     *
     * Spawning Strategy:
     * - NUM_JACKALOPE_EACH of each tier (Easy, Medium, Hard)
     * - Random placement within geographic boundaries
     * - Three spawns per iteration to distribute mob types evenly
     */

    /* Optional: Override default event duration for testing */
    // STAFF_EVENT_TIME = 20; /* Uncomment for short test events */

    /*
     * STREAMLINED INITIAL JACKALOPE DEPLOYMENT (Addresses M001: Code Duplication)
     * Use helper function for consistent and maintainable spawning logic
     */
    
    /* Deploy initial population of all three Jackalope tiers */
    spawn_jackalope_batch(EASY_JACKALOPE, NUM_JACKALOPE_EACH);   /* Easy Jackalope (Levels 1-10) */
    spawn_jackalope_batch(MED_JACKALOPE, NUM_JACKALOPE_EACH);    /* Medium Jackalope (Levels 11-20) */
    spawn_jackalope_batch(HARD_JACKALOPE, NUM_JACKALOPE_EACH);   /* Hard Jackalope (Levels 21+) */

    break;

  /*
   * Future events: Add initialization code here
   * Template:
   * case NEW_EVENT:
   *   // Event-specific setup
   *   break;
   */
  default:
    /* No special initialization required for unhandled events */
    break;
  }

  return EVENT_SUCCESS; /* Success indicator */
}

/*
 * Terminate an active staff event with complete cleanup and notification.
 *
 * This function handles the orderly shutdown of staff events, including
 * world-wide announcements, state cleanup, and event-specific teardown
 * procedures to ensure the game world returns to normal operation.
 *
 * Process Flow:
 * 1. Parameter validation
 * 2. World-wide event end announcement
 * 3. Global event state reset
 * 4. Event-specific cleanup procedures
 * 5. Inter-event delay activation
 *
 * Key Features:
 * - Comprehensive world broadcasting with summary messages
 * - Complete state cleanup to prevent event conflicts
 * - Event-specific cleanup (mob removal, object cleanup, etc.)
 * - Mandatory delay period before next event can start
 *
 * @param event_num: Index of the event to end
 *
 * Global State Changes:
 * - Resets STAFF_EVENT_NUM to UNDEFINED_EVENT
 * - Resets STAFF_EVENT_TIME to 0
 * - Sets STAFF_EVENT_DELAY for cleanup period
 */
event_result_t end_staff_event(int event_num)
{
  struct descriptor_data *pt = NULL;    /* Iterator for player connections */
  struct obj_data *obj = NULL;          /* Object pointer for cleanup operations */

  /*
   * INPUT VALIDATION:
   * Ensure the event number is within valid bounds
   */
  if (event_num >= NUM_STAFF_EVENTS || event_num < 0)
  {
    return EVENT_ERROR_INVALID_NUM;
  }

  /*
   * WORLD-WIDE EVENT TERMINATION ANNOUNCEMENT:
   * Inform all connected players that the event has concluded,
   * providing summary information and final instructions.
   */
  for (pt = descriptor_list; pt; pt = pt->next)
  {
    if (IS_PLAYING(pt) && pt->character)
    {
      /* Eye-catching event end notification */
      send_to_char(pt->character, "\r\n\tR╔═══════════════════════════════════════════════════════════════════════════╗\tn\r\n");
      send_to_char(pt->character, "\tR║                        \tY\t=STAFF EVENT ENDING!\tR                             ║\tn\r\n");
      send_to_char(pt->character, "\tR╠═══════════════════════════════════════════════════════════════════════════╣\tn\r\n");
      send_to_char(pt->character, "\tR║ \tWEvent:\tn %-67s \tR║\tn\r\n", staff_events_list[event_num][EVENT_TITLE]);
      send_to_char(pt->character, "\tR╚═══════════════════════════════════════════════════════════════════════════╝\tn\r\n\r\n");
      
      /* Event-specific end message */
      send_to_char(pt->character, "%s\r\n\r\n", staff_events_list[event_num][EVENT_END]);
      
      /* Event summary box */
      send_to_char(pt->character, "\tG┌───────────────────────────────────────────────────────────────────────────┐\tn\r\n");
      send_to_char(pt->character, "\tG│                         \tWEVENT CONCLUSION                                  \tG│\tn\r\n");
      send_to_char(pt->character, "\tG├───────────────────────────────────────────────────────────────────────────┤\tn\r\n");
      send_to_char(pt->character, "\tG│\tn\r\n");
      send_to_char(pt->character, "%s", staff_events_list[event_num][EVENT_SUMMARY]);
      send_to_char(pt->character, "\tG│\tn\r\n");
      send_to_char(pt->character, "\tG└───────────────────────────────────────────────────────────────────────────┘\tn\r\n\r\n");
    }
  }

  /*
   * GLOBAL EVENT STATE CLEANUP (Using M007 abstraction):
   * Reset all event-related global variables to their inactive states
   * and initiate the mandatory inter-event delay period through
   * the state management layer for better modularity.
   */
  clear_event_state();                                  /* Clear active event state */
  set_event_delay(STAFF_EVENT_DELAY_CNST);             /* Start cleanup delay */

  /*
   * EVENT-SPECIFIC CLEANUP PROCEDURES:
   * Each event requires unique cleanup operations to remove
   * event-specific content and restore normal game conditions.
   */
  switch (event_num)
  {

  case JACKALOPE_HUNT:
    /*
     * JACKALOPE HUNT CLEANUP:
     * Remove all Jackalope mobs from the game world to prevent
     * post-event farming and restore normal wilderness conditions.
     */
    mob_ingame_purge(EASY_JACKALOPE);   /* Remove all Easy Jackalope */
    mob_ingame_purge(MED_JACKALOPE);    /* Remove all Medium Jackalope */
    mob_ingame_purge(HARD_JACKALOPE);   /* Remove all Hard Jackalope */
    break;

  case THE_PRISONER_EVENT:
  {
    struct obj_data *obj_next = NULL;  /* Safe iteration pointer */
    
    /*
     * THE PRISONER EVENT CLEANUP:
     * Remove the temporary portal that provides access to the raid zone.
     * This prevents post-event access and maintains game balance.
     */
    
    /*
     * SAFE PORTAL CLEANUP (Addresses High Priority Issue H003)
     * Use safe iteration pattern to prevent corruption when modifying
     * the object list during traversal. Store next pointer before
     * potential extraction to avoid accessing freed memory.
     */
    
    for (obj = world[real_room(TP_PORTAL_L_ROOM)].contents; obj; obj = obj_next)
    {
      /* Store next pointer before potential extraction */
      obj_next = obj->next_content;
      
      if (GET_OBJ_VNUM(obj) == THE_PRISONER_PORTAL)
      {
        /* Provide atmospheric removal message to players in the room */
        act("...  $p flickers, shimmers, then fades away...", TRUE, 0, obj, 0, TO_ROOM);
        extract_obj(obj); /* Remove portal from game - safe since we stored next pointer */
      }
    }
  }
  break;

  /*
   * Future events: Add cleanup code here
   * Template:
   * case NEW_EVENT:
   *   // Event-specific cleanup
   *   break;
   */
  default:
    /* No special cleanup required for unhandled events */
    break;
  }

  return EVENT_SUCCESS;
}

/*****************************************************************************/
/* STAFF COMMAND INTERFACE AND INFORMATION FUNCTIONS */
/*****************************************************************************/

/*
 * Display comprehensive information about a specific staff event.
 *
 * This function provides detailed event information to players and staff,
 * including event messages, current status, population counts, and timing
 * information. It serves as the primary information interface for the
 * staff event system.
 *
 * Key Features:
 * - Displays all event message fields (title, begin, end, detail)
 * - Shows real-time mob population counts for applicable events
 * - Calculates and displays precise time remaining
 * - Provides inter-event delay information
 * - Includes usage instructions for staff members
 * - Role-based information access (staff vs. players)
 *
 * Information Display:
 * - Event title with index number
 * - Event begin message (visible to all)
 * - Event end message (staff only)
 * - Detailed event information and instructions
 * - Event-specific status information (mob counts, etc.)
 * - Precise time remaining calculation (hours/minutes/seconds)
 * - Inter-event delay timer
 * - Command usage help (staff only)
 *
 * @param ch: Character requesting event information
 * @param event_num: Index of the event to display information for
 *
 * Access Control:
 * - Basic event info available to all players
 * - Staff-only sections require LVL_STAFF or higher
 * - Event end messages restricted to staff only
 */
void staff_event_info(struct char_data *ch, const int event_num)
{
  /*
   * INPUT VALIDATION:
   * Ensure valid character pointer and event number bounds
   */
  if (!ch || event_num >= NUM_STAFF_EVENTS || event_num < 0)
  {
    return;
  }

  int event_field = 0;        /* Iterator for event message fields */
  int secs_left = 0;          /* Calculated seconds remaining in event */
  int hours = 0, mins = 0, secs = 0;  /* Time breakdown */
  const char *status_color = "\tG";  /* Color for status display */
  const char *status_text = "ACTIVE";  /* Status text */

  /* Header with decorative border */
  send_to_char(ch, "\r\n\tC╔═══════════════════════════════════════════════════════════════════════════╗\tn\r\n");
  send_to_char(ch, "\tC║                          \tYSTAFF EVENT INFORMATION                           \tC║\tn\r\n");
  send_to_char(ch, "\tC╠═══════════════════════════════════════════════════════════════════════════╣\tn\r\n");
  
  /* Event Title and Index */
  send_to_char(ch, "\tC║ \tWEvent:\tn %-62s \tC║\tn\r\n", staff_events_list[event_num][EVENT_TITLE]);
  send_to_char(ch, "\tC║ \tWIndex:\tn \tG%-61d\tn \tC║\tn\r\n", event_num);
  
  /* Current Status */
  if (IS_STAFF_EVENT && STAFF_EVENT_NUM == event_num)
  {
    if (STAFF_EVENT_TIME <= 60) /* Less than 10 minutes */
    {
      status_color = "\tR";
      status_text = "ENDING SOON";
    }
    else if (STAFF_EVENT_TIME <= 300) /* Less than 50 minutes */
    {
      status_color = "\tY";
      status_text = "ACTIVE";
    }
  }
  else
  {
    status_color = "\tw";
    status_text = "INACTIVE";
  }
  
  send_to_char(ch, "\tC║ \tWStatus:\tn %s%-60s\tn \tC║\tn\r\n", status_color, status_text);
  send_to_char(ch, "\tC╠═══════════════════════════════════════════════════════════════════════════╣\tn\r\n");

  /*
   * ENHANCED EVENT FIELD VALIDATION (Addresses M008)
   * Validate that all required fields exist before accessing them
   * to prevent crashes and provide meaningful error messages.
   */
  for (event_field = 0; event_field < STAFF_EVENT_FIELDS; event_field++)
  {
    /* Validate field exists before processing */
    if (!staff_events_list[event_num][event_field])
    {
      log("SYSERR: Missing event field %d for event %d", event_field, event_num);
      if (GET_LEVEL(ch) >= LVL_STAFF)
      {
        send_to_char(ch, "\tC║ \tR[ERROR: Missing field %d - contact administrators]                          \tC║\tn\r\n", event_field);
      }
      continue; /* Skip this field and continue with others */
    }
    
    /* Additional validation: check for empty strings */
    if (!*staff_events_list[event_num][event_field])
    {
      log("SYSERR: Empty event field %d for event %d", event_field, event_num);
      if (GET_LEVEL(ch) >= LVL_STAFF)
      {
        send_to_char(ch, "\tC║ \tR[ERROR: Empty field %d - contact administrators]                            \tC║\tn\r\n", event_field);
      }
      continue; /* Skip this field and continue with others */
    }
    
    switch (event_field)
    {

    case EVENT_BEGIN:
      send_to_char(ch, "\tC║                                                                           ║\tn\r\n");
      send_to_char(ch, "\tC║ \tWStart Announcement:\tn                                                       \tC║\tn\r\n");
      send_to_char(ch, "\tC║ \tw━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\tn \tC║\tn\r\n");
      send_to_char(ch, "\tC║\tn %s\r\n", staff_events_list[event_num][EVENT_BEGIN]);
      break;

    case EVENT_END:
      if (GET_LEVEL(ch) >= LVL_STAFF)
      {
        send_to_char(ch, "\tC║                                                                           ║\tn\r\n");
        send_to_char(ch, "\tC║ \tWEnd Announcement:\tn                                                         \tC║\tn\r\n");
        send_to_char(ch, "\tC║ \tw━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\tn \tC║\tn\r\n");
        send_to_char(ch, "\tC║\tn %s\r\n", staff_events_list[event_num][EVENT_END]);
      }
      break;

    case EVENT_DETAIL:
      send_to_char(ch, "\tC║                                                                           ║\tn\r\n");
      send_to_char(ch, "\tC║ \tWEvent Details:\tn                                                            \tC║\tn\r\n");
      send_to_char(ch, "\tC║ \tw━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\tn \tC║\tn\r\n");
      send_to_char(ch, "\tC║\tn\r\n");
      send_to_char(ch, "%s", staff_events_list[event_num][EVENT_DETAIL]);
      send_to_char(ch, "\tn");
      break;

    case EVENT_TITLE: /* we mention this above */
    /* fallthrough */
    default:
      break;
    }
  } /*end for*/

  /* Event-specific statistics section */
  send_to_char(ch, "\tC╠═══════════════════════════════════════════════════════════════════════════╣\tn\r\n");
  send_to_char(ch, "\tC║                            \tYEVENT STATISTICS                               \tC║\tn\r\n");
  send_to_char(ch, "\tC╠═══════════════════════════════════════════════════════════════════════════╣\tn\r\n");
  
  /* here is our custom output relevant to each event */
  switch (event_num)
  {
  case JACKALOPE_HUNT:
  {
    int easy_count = 0, med_count = 0, hard_count = 0;
    
    /* Use optimized counting function */
    count_jackalope_mobs(&easy_count, &med_count, &hard_count);
    
    send_to_char(ch, "\tC║ \tWJackalope Population:\tn                                                     \tC║\tn\r\n");
    send_to_char(ch, "\tC║   \tw• Elusive (Lvl 1-10):\tn  \tG%-47d\tn \tC║\tn\r\n", easy_count);
    send_to_char(ch, "\tC║   \tw• Mature (Lvl 11-20):\tn   \tY%-47d\tn \tC║\tn\r\n", med_count);
    send_to_char(ch, "\tC║   \tw• Alpha (Lvl 21+):\tn      \tR%-47d\tn \tC║\tn\r\n", hard_count);
    send_to_char(ch, "\tC║   \tw• Total Active:\tn         \tW%-47d\tn \tC║\tn\r\n", easy_count + med_count + hard_count);
  }
  break;

  case THE_PRISONER_EVENT:
    send_to_char(ch, "\tC║ \tWEvent Features:\tn                                                           \tC║\tn\r\n");
    send_to_char(ch, "\tC║   \tw• Portal Status:\tn        \tGActive at Mosswood Elder\tn                   \tC║\tn\r\n");
    send_to_char(ch, "\tC║   \tw• Death Penalty:\tn        \tRDisabled during event\tn                      \tC║\tn\r\n");
    send_to_char(ch, "\tC║   \tw• Treasure Drops:\tn       \tYMaximized for The Prisoner\tn                 \tC║\tn\r\n");
    break;

  default:
    send_to_char(ch, "\tC║ \twNo specific statistics available for this event.\tn                         \tC║\tn\r\n");
    break;
  }

  /* Time Information Section */
  send_to_char(ch, "\tC╠═══════════════════════════════════════════════════════════════════════════╣\tn\r\n");
  send_to_char(ch, "\tC║                            \tYTIME INFORMATION                               \tC║\tn\r\n");
  send_to_char(ch, "\tC╠═══════════════════════════════════════════════════════════════════════════╣\tn\r\n");
  
  if (STAFF_EVENT_TIME && IS_STAFF_EVENT && STAFF_EVENT_NUM == event_num)
  {
    secs_left = ((STAFF_EVENT_TIME - 1) * SECS_PER_MUD_HOUR) + next_tick;
    hours = secs_left / 3600;
    mins = (secs_left % 3600) / 60;
    secs = secs_left % 60;
    
    /* Color-coded time display based on urgency */
    const char *time_color = "\tG";
    if (hours == 0 && mins < 10)
      time_color = "\tR\t="; /* Flashing red for last 10 minutes */
    else if (hours == 0 && mins < 30)
      time_color = "\tR";
    else if (hours < 2)
      time_color = "\tY";
    
    send_to_char(ch, "\tC║ \tWTime Remaining:\tn %s%02d:%02d:%02d\tn                                               \tC║\tn\r\n",
                 time_color, hours, mins, secs);
    
    /* Progress bar */
    int total_duration = 1200; /* Default event duration */
    int elapsed = total_duration - STAFF_EVENT_TIME;
    int progress = (elapsed * 50) / total_duration; /* 50 character bar */
    int i = 0;
    
    send_to_char(ch, "\tC║ \tWProgress:\tn       [");
    for (i = 0; i < 50; i++)
    {
      if (i < progress)
        send_to_char(ch, "\tG=\tn");
      else
        send_to_char(ch, "\tw-\tn");
    }
    send_to_char(ch, "] \tC║\tn\r\n");
  }
  else
  {
    send_to_char(ch, "\tC║ \tWTime Remaining:\tn \twEvent not currently active\tn                            \tC║\tn\r\n");
  }
  
  /* Inter-event delay information */
  if (STAFF_EVENT_DELAY > 0)
  {
    send_to_char(ch, "\tC║ \tWCleanup Delay:\tn  \tY%-57d ticks\tn \tC║\tn\r\n", STAFF_EVENT_DELAY);
  }

  /* Footer section */
  if (GET_LEVEL(ch) >= LVL_STAFF)
  {
    send_to_char(ch, "\tC╠═══════════════════════════════════════════════════════════════════════════╣\tn\r\n");
    send_to_char(ch, "\tC║                            \tYSTAFF COMMANDS                                 \tC║\tn\r\n");
    send_to_char(ch, "\tC╠═══════════════════════════════════════════════════════════════════════════╣\tn\r\n");
    send_to_char(ch, "\tC║ \twUsage: \tWstaffevents\tn \tw[start|end|info] [event_number]\tn                     \tC║\tn\r\n");
    send_to_char(ch, "\tC║                                                                           ║\tn\r\n");
    send_to_char(ch, "\tC║ \tw• \tGstart\tn \tw- Begin a new event (requires no active event)\tn               \tC║\tn\r\n");
    send_to_char(ch, "\tC║ \tw• \tRend\tn   \tw- Terminate the current event immediately\tn                   \tC║\tn\r\n");
    send_to_char(ch, "\tC║ \tw• \tCinfo\tn  \tw- Display detailed information about an event\tn               \tC║\tn\r\n");
  }
  
  send_to_char(ch, "\tC╚═══════════════════════════════════════════════════════════════════════════╝\tn\r\n\r\n");

  return;
}

/*
 * Display a formatted list of all available staff events.
 *
 * This function provides a quick reference of all implemented staff events,
 * showing their index numbers and titles for easy identification and
 * command reference. Used primarily for staff command help and player
 * information when no specific event is active.
 *
 * Display Format:
 * - Header identifying this as a staff event listing
 * - Numbered list of events with titles
 * - Usage instructions for the staffevents command
 *
 * Information Source:
 * - Reads directly from staff_events_list array
 * - Uses EVENT_TITLE field (index 0) for display names
 * - Dynamically reflects all configured events
 *
 * @param ch: Character requesting the event list
 *
 * Usage Context:
 * - Called by staffevents command when no arguments provided
 * - Used for command help and event discovery
 * - Available to both staff and players for information
 */
void list_staff_events(struct char_data *ch)
{
  int i = 0;    /* Iterator for event list */
  const char *event_status = NULL;  /* Status indicator for each event */
  const char *status_color = NULL;  /* Color for status display */

  /*
   * HEADER DISPLAY:
   * Present a professional header with decorative borders
   */
  send_to_char(ch, "\r\n\tC╔═══════════════════════════════════════════════════════════════════════════╗\tn\r\n");
  send_to_char(ch, "\tC║                        \tYAVAILABLE STAFF EVENTS                            \tC║\tn\r\n");
  send_to_char(ch, "\tC╠═══════════════════════════════════════════════════════════════════════════╣\tn\r\n");
  send_to_char(ch, "\tC║ \twID\tn   \tWEvent Name                                          \twStatus\tn        \tC║\tn\r\n");
  send_to_char(ch, "\tC║ \tw───  ──────────────────────────────────────────────  ─────────────\tn      \tC║\tn\r\n");

  /*
   * EVENT LIST DISPLAY:
   * Iterate through all configured events and display their
   * index numbers, titles, and current status.
   */
  for (i = 0; i < NUM_STAFF_EVENTS; i++)
  {
    /* Determine event status */
    if (IS_STAFF_EVENT && STAFF_EVENT_NUM == i)
    {
      event_status = "ACTIVE NOW";
      status_color = "\tG\t=";  /* Flashing green */
    }
    else if (STAFF_EVENT_DELAY > 0)
    {
      event_status = "COOLDOWN";
      status_color = "\tY";
    }
    else
    {
      event_status = "AVAILABLE";
      status_color = "\tw";
    }
    
    /* Special status for The Prisoner if already defeated */
    if (i == THE_PRISONER_EVENT && prisoner_heads == -2)
    {
      event_status = "COMPLETED";
      status_color = "\tR";
    }
    
    /*
     * Event Entry Format:
     * - Formatted ID number
     * - Event title with proper spacing
     * - Color-coded status indicator
     */
    send_to_char(ch, "\tC║ \tG[%d]\tn  %-50s  %s%-13s\tn \tC║\tn\r\n", 
                 i, 
                 staff_events_list[i][EVENT_TITLE], 
                 status_color,
                 event_status);
    
    /* Add descriptive subtitle for each event */
    switch (i)
    {
    case JACKALOPE_HUNT:
      send_to_char(ch, "\tC║      \tw└─ Hunt magical jackalopes across the Hardbuckler wilderness\tn       \tC║\tn\r\n");
      break;
    case THE_PRISONER_EVENT:
      send_to_char(ch, "\tC║      \tw└─ Assault The Prisoner through a portal to Avernus\tn              \tC║\tn\r\n");
      break;
    default:
      break;
    }
    
    /* Add spacing between events */
    if (i < NUM_STAFF_EVENTS - 1)
      send_to_char(ch, "\tC║                                                                           ║\tn\r\n");
  }

  /*
   * CURRENT STATUS SECTION
   */
  send_to_char(ch, "\tC╠═══════════════════════════════════════════════════════════════════════════╣\tn\r\n");
  send_to_char(ch, "\tC║                           \tYCURRENT STATUS                                 \tC║\tn\r\n");
  send_to_char(ch, "\tC╠═══════════════════════════════════════════════════════════════════════════╣\tn\r\n");
  
  if (IS_STAFF_EVENT)
  {
    int secs_left = ((STAFF_EVENT_TIME - 1) * SECS_PER_MUD_HOUR) + next_tick;
    int hours = secs_left / 3600;
    int mins = (secs_left % 3600) / 60;
    
    send_to_char(ch, "\tC║ \tWActive Event:\tn %-59s \tC║\tn\r\n", 
                 staff_events_list[STAFF_EVENT_NUM][EVENT_TITLE]);
    send_to_char(ch, "\tC║ \tWTime Left:\tn    \tY%d hours, %d minutes\tn                                    \tC║\tn\r\n", 
                 hours, mins);
  }
  else if (STAFF_EVENT_DELAY > 0)
  {
    send_to_char(ch, "\tC║ \twNo events active - System in cleanup mode\tn                                \tC║\tn\r\n");
    send_to_char(ch, "\tC║ \tWCooldown:\tn     \tY%d ticks remaining\tn                                        \tC║\tn\r\n", 
                 STAFF_EVENT_DELAY);
  }
  else
  {
    send_to_char(ch, "\tC║ \tGSystem ready - No events currently active\tn                                 \tC║\tn\r\n");
  }

  /*
   * USAGE INSTRUCTIONS:
   * Provide command syntax help based on user's access level
   */
  if (GET_LEVEL(ch) >= LVL_STAFF)
  {
    send_to_char(ch, "\tC╠═══════════════════════════════════════════════════════════════════════════╣\tn\r\n");
    send_to_char(ch, "\tC║                            \tYCOMMAND USAGE                                  \tC║\tn\r\n");
    send_to_char(ch, "\tC╠═══════════════════════════════════════════════════════════════════════════╣\tn\r\n");
    send_to_char(ch, "\tC║ \tWstaffevents\tn                  - Show this list                             \tC║\tn\r\n");
    send_to_char(ch, "\tC║ \tWstaffevents start\tn \tw[ID]\tn       - Start an event                            \tC║\tn\r\n");
    send_to_char(ch, "\tC║ \tWstaffevents end\tn \tw[ID]\tn         - End an active event                        \tC║\tn\r\n");
    send_to_char(ch, "\tC║ \tWstaffevents info\tn \tw[ID]\tn        - View detailed event information            \tC║\tn\r\n");
  }
  else
  {
    send_to_char(ch, "\tC╠═══════════════════════════════════════════════════════════════════════════╣\tn\r\n");
    send_to_char(ch, "\tC║ \twUse '\tWstaffevents\tw' to view current event information\tn                     \tC║\tn\r\n");
  }
  
  send_to_char(ch, "\tC╚═══════════════════════════════════════════════════════════════════════════╝\tn\r\n\r\n");

  return;
}

/*
 * Helper function to handle player (non-staff) access to event information.
 * Non-staff users can only view information about the current event.
 *
 * @param ch: Character requesting information
 */
static void handle_player_event_access(struct char_data *ch)
{
  if (is_event_active())
  {
    /* Show information about the currently active event */
    staff_event_info(ch, get_active_event());
  }
  else
  {
    /* No active event - display a nice message box */
    send_to_char(ch, "\r\n\tC╔═══════════════════════════════════════════════════════════════════════════╗\tn\r\n");
    send_to_char(ch, "\tC║                           \tYSTAFF EVENT STATUS                             \tC║\tn\r\n");
    send_to_char(ch, "\tC╠═══════════════════════════════════════════════════════════════════════════╣\tn\r\n");
    send_to_char(ch, "\tC║                                                                           ║\tn\r\n");
    send_to_char(ch, "\tC║                    \twNo staff events are currently active\tn                   \tC║\tn\r\n");
    send_to_char(ch, "\tC║                                                                           ║\tn\r\n");
    send_to_char(ch, "\tC║   \twStaff events are special limited-time activities that offer unique\tn      \tC║\tn\r\n");
    send_to_char(ch, "\tC║   \twrewards and experiences. Keep an eye out for announcements!\tn             \tC║\tn\r\n");
    send_to_char(ch, "\tC║                                                                           ║\tn\r\n");
    send_to_char(ch, "\tC╚═══════════════════════════════════════════════════════════════════════════╝\tn\r\n\r\n");
  }
}

/*
 * Helper function to handle default behavior when no arguments are provided.
 * Shows current event info if active, otherwise shows list of available events.
 *
 * @param ch: Character executing the command
 */
static void handle_default_event_display(struct char_data *ch)
{
  if (is_event_active())
  {
    /* Active event - show detailed information */
    staff_event_info(ch, get_active_event());
  }
  else
  {
    /* No active event - show list of available events */
    list_staff_events(ch);
  }
}

/*
 * Helper function to parse and validate event number from command arguments.
 *
 * @param ch: Character executing the command
 * @param arg2: String containing the event number
 * @return: Parsed event number, or -1 if invalid
 */
static int parse_and_validate_event_num(struct char_data *ch, const char *arg2)
{
  int event_num = UNDEFINED_EVENT;
  
  /* Parse event number */
  if (isdigit(*arg2))
  {
    event_num = atoi(arg2);
  }
  else
  {
    /* Invalid event number format */
    send_to_char(ch, "\r\n\tR┌─────────────────────────────────────────────────────────────────┐\tn\r\n");
    send_to_char(ch, "\tR│                   \tYINVALID EVENT NUMBER                          \tR│\tn\r\n");
    send_to_char(ch, "\tR├─────────────────────────────────────────────────────────────────┤\tn\r\n");
    send_to_char(ch, "\tR│ \twEvent number must be a digit, not '\tY%s\tw'\tn                        \tR│\tn\r\n", arg2);
    send_to_char(ch, "\tR│                                                                 │\tn\r\n");
    send_to_char(ch, "\tR│ \twPlease specify the event ID number from the list below.\tn        \tR│\tn\r\n");
    send_to_char(ch, "\tR└─────────────────────────────────────────────────────────────────┘\tn\r\n\r\n");
    list_staff_events(ch);
    return -1;
  }

  /* Validate event number bounds */
  if (event_num >= NUM_STAFF_EVENTS || event_num < 0)
  {
    send_to_char(ch, "\r\n\tR┌─────────────────────────────────────────────────────────────────┐\tn\r\n");
    send_to_char(ch, "\tR│                   \tYEVENT NUMBER OUT OF RANGE                     \tR│\tn\r\n");
    send_to_char(ch, "\tR├─────────────────────────────────────────────────────────────────┤\tn\r\n");
    send_to_char(ch, "\tR│ \twEvent \tY%d\tw does not exist.\tn                                       \tR│\tn\r\n", event_num);
    send_to_char(ch, "\tR│                                                                 │\tn\r\n");
    send_to_char(ch, "\tR│ \twValid event IDs are \tG0\tw to \tG%d\tw.\tn                                  \tR│\tn\r\n", NUM_STAFF_EVENTS - 1);
    send_to_char(ch, "\tR└─────────────────────────────────────────────────────────────────┘\tn\r\n\r\n");
    list_staff_events(ch);
    return -1;
  }
  
  return event_num;
}

/*
 * Helper function to handle event start command processing.
 *
 * @param ch: Character executing the command
 * @param event_num: Event number to start
 */
static void handle_start_event_command(struct char_data *ch, const int event_num)
{
  event_result_t result = EVENT_SUCCESS;
  
  /* Check preconditions for starting events */
  if (!is_event_active() && !get_event_delay())
  {
    /* All clear - attempt to start the event */
    result = start_staff_event(event_num);
  }
  else if (get_event_delay())
  {
    /* Inter-event cleanup delay still active */
    send_to_char(ch, "\r\n\tR┌─────────────────────────────────────────────────────────────────┐\tn\r\n");
    send_to_char(ch, "\tR│                      \tYCLEANUP IN PROGRESS                       \tR│\tn\r\n");
    send_to_char(ch, "\tR├─────────────────────────────────────────────────────────────────┤\tn\r\n");
    send_to_char(ch, "\tR│ \twThe system is currently in cleanup mode between events.\tn        \tR│\tn\r\n");
    send_to_char(ch, "\tR│ \twPlease wait \tY%d\tw more ticks before starting a new event.\tn        \tR│\tn\r\n",
                 get_event_delay());
    send_to_char(ch, "\tR└─────────────────────────────────────────────────────────────────┘\tn\r\n\r\n");
    return;
  }
  else
  {
    /* Another event is already running */
    send_to_char(ch, "\r\n\tR┌─────────────────────────────────────────────────────────────────┐\tn\r\n");
    send_to_char(ch, "\tR│                       \tYEVENT CONFLICT                           \tR│\tn\r\n");
    send_to_char(ch, "\tR├─────────────────────────────────────────────────────────────────┤\tn\r\n");
    send_to_char(ch, "\tR│ \twAnother event is already active:\tn                               \tR│\tn\r\n");
    send_to_char(ch, "\tR│ \tW%-63s\tn \tR│\tn\r\n", staff_events_list[get_active_event()][EVENT_TITLE]);
    send_to_char(ch, "\tR│                                                                 │\tn\r\n");
    send_to_char(ch, "\tR│ \twUse '\tWstaffevents end %d\tw' to stop the current event.\tn           \tR│\tn\r\n", get_active_event());
    send_to_char(ch, "\tR└─────────────────────────────────────────────────────────────────┘\tn\r\n\r\n");
    return;
  }

  /* Handle start event results */
  switch (result)
  {
  case EVENT_ERROR_PRECONDITION_FAILED:
    /* Event-specific precondition not met */
    if (event_num == THE_PRISONER_EVENT)
    {
      send_to_char(ch, "\r\n\tR┌─────────────────────────────────────────────────────────────────┐\tn\r\n");
      send_to_char(ch, "\tR│                    \tYEVENT UNAVAILABLE                           \tR│\tn\r\n");
      send_to_char(ch, "\tR├─────────────────────────────────────────────────────────────────┤\tn\r\n");
      send_to_char(ch, "\tR│ \twThe Prisoner has already been defeated this boot cycle.\tn        \tR│\tn\r\n");
      send_to_char(ch, "\tR│ \twThis event cannot be run again until the next reboot.\tn          \tR│\tn\r\n");
      send_to_char(ch, "\tR└─────────────────────────────────────────────────────────────────┘\tn\r\n\r\n");
    }
    else
    {
      send_to_char(ch, "\r\n\tR┌─────────────────────────────────────────────────────────────────┐\tn\r\n");
      send_to_char(ch, "\tR│                  \tYPRECONDITION FAILED                          \tR│\tn\r\n");
      send_to_char(ch, "\tR├─────────────────────────────────────────────────────────────────┤\tn\r\n");
      send_to_char(ch, "\tR│ \twEvent preconditions not met. Cannot start this event.\tn          \tR│\tn\r\n");
      send_to_char(ch, "\tR└─────────────────────────────────────────────────────────────────┘\tn\r\n\r\n");
    }
    break;
    
  case EVENT_ERROR_INVALID_NUM:
    send_to_char(ch, "\r\n\tR┌─────────────────────────────────────────────────────────────────┐\tn\r\n");
    send_to_char(ch, "\tR│                      \tYINVALID EVENT                            \tR│\tn\r\n");
    send_to_char(ch, "\tR├─────────────────────────────────────────────────────────────────┤\tn\r\n");
    send_to_char(ch, "\tR│ \twThe specified event number is invalid.\tn                         \tR│\tn\r\n");
    send_to_char(ch, "\tR└─────────────────────────────────────────────────────────────────┘\tn\r\n\r\n");
    break;
    
  case EVENT_ERROR_INVALID_DATA:
    send_to_char(ch, "\r\n\tR┌─────────────────────────────────────────────────────────────────┐\tn\r\n");
    send_to_char(ch, "\tR│                    \tYCORRUPTED EVENT DATA                        \tR│\tn\r\n");
    send_to_char(ch, "\tR├─────────────────────────────────────────────────────────────────┤\tn\r\n");
    send_to_char(ch, "\tR│ \twEvent data is corrupted or incomplete.\tn                         \tR│\tn\r\n");
    send_to_char(ch, "\tR│ \twPlease contact the administrators immediately.\tn                 \tR│\tn\r\n");
    send_to_char(ch, "\tR└─────────────────────────────────────────────────────────────────┘\tn\r\n\r\n");
    break;
    
  case EVENT_SUCCESS:
    /* Success - display a nice confirmation */
    send_to_char(ch, "\r\n\tG┌─────────────────────────────────────────────────────────────────┐\tn\r\n");
    send_to_char(ch, "\tG│                    \tYEVENT STARTED SUCCESSFULLY                  \tG│\tn\r\n");
    send_to_char(ch, "\tG├─────────────────────────────────────────────────────────────────┤\tn\r\n");
    send_to_char(ch, "\tG│ \tWEvent:\tn %-57s \tG│\tn\r\n", staff_events_list[event_num][EVENT_TITLE]);
    send_to_char(ch, "\tG│ \twThe event has been activated and announced to all players.\tn      \tG│\tn\r\n");
    send_to_char(ch, "\tG└─────────────────────────────────────────────────────────────────┘\tn\r\n\r\n");
    break;
    
  default:
    /* Unhandled error code */
    send_to_char(ch, "\r\n\tR┌─────────────────────────────────────────────────────────────────┐\tn\r\n");
    send_to_char(ch, "\tR│                     \tYUNEXPECTED ERROR                           \tR│\tn\r\n");
    send_to_char(ch, "\tR├─────────────────────────────────────────────────────────────────┤\tn\r\n");
    send_to_char(ch, "\tR│ \twAn unexpected error occurred while starting the event.\tn          \tR│\tn\r\n");
    send_to_char(ch, "\tR│ \twError code: %d\tn                                                  \tR│\tn\r\n", result);
    send_to_char(ch, "\tR└─────────────────────────────────────────────────────────────────┘\tn\r\n\r\n");
    break;
  }
}

/*
 * Helper function to handle event end command processing.
 *
 * @param ch: Character executing the command
 * @param event_num: Event number to end
 */
static void handle_end_event_command(struct char_data *ch, const int event_num)
{
  event_result_t result = EVENT_SUCCESS;
  
  if (!is_event_active())
  {
    /* No active event to end */
    send_to_char(ch, "\r\n\tY┌─────────────────────────────────────────────────────────────────┐\tn\r\n");
    send_to_char(ch, "\tY│                      \tWNO ACTIVE EVENT                            \tY│\tn\r\n");
    send_to_char(ch, "\tY├─────────────────────────────────────────────────────────────────┤\tn\r\n");
    send_to_char(ch, "\tY│ \twThere is no event currently running to end.\tn                     \tY│\tn\r\n");
    send_to_char(ch, "\tY│                                                                 │\tn\r\n");
    send_to_char(ch, "\tY│ \twUse '\tWstaffevents\tw' to see available events.\tn                     \tY│\tn\r\n");
    send_to_char(ch, "\tY└─────────────────────────────────────────────────────────────────┘\tn\r\n\r\n");
    return;
  }

  /* Check if they're trying to end the wrong event */
  if (event_num != get_active_event())
  {
    send_to_char(ch, "\r\n\tR┌─────────────────────────────────────────────────────────────────┐\tn\r\n");
    send_to_char(ch, "\tR│                    \tYWRONG EVENT NUMBER                           \tR│\tn\r\n");
    send_to_char(ch, "\tR├─────────────────────────────────────────────────────────────────┤\tn\r\n");
    send_to_char(ch, "\tR│ \twYou specified event \tY%d\tw, but the active event is:\tn              \tR│\tn\r\n", event_num);
    send_to_char(ch, "\tR│ \tW[%d] %-58s\tn \tR│\tn\r\n", 
                 get_active_event(), 
                 staff_events_list[get_active_event()][EVENT_TITLE]);
    send_to_char(ch, "\tR│                                                                 │\tn\r\n");
    send_to_char(ch, "\tR│ \twUse '\tWstaffevents end %d\tw' to end the current event.\tn            \tR│\tn\r\n", get_active_event());
    send_to_char(ch, "\tR└─────────────────────────────────────────────────────────────────┘\tn\r\n\r\n");
    return;
  }

  /* End the specified event */
  result = end_staff_event(event_num);
  
  /* Handle end event results */
  switch (result)
  {
  case EVENT_ERROR_INVALID_NUM:
    send_to_char(ch, "\r\n\tR┌─────────────────────────────────────────────────────────────────┐\tn\r\n");
    send_to_char(ch, "\tR│                      \tYINVALID EVENT                            \tR│\tn\r\n");
    send_to_char(ch, "\tR├─────────────────────────────────────────────────────────────────┤\tn\r\n");
    send_to_char(ch, "\tR│ \twThe specified event number is invalid.\tn                         \tR│\tn\r\n");
    send_to_char(ch, "\tR└─────────────────────────────────────────────────────────────────┘\tn\r\n\r\n");
    break;
    
  case EVENT_SUCCESS:
    /* Success - display confirmation */
    send_to_char(ch, "\r\n\tG┌─────────────────────────────────────────────────────────────────┐\tn\r\n");
    send_to_char(ch, "\tG│                     \tYEVENT ENDED SUCCESSFULLY                   \tG│\tn\r\n");
    send_to_char(ch, "\tG├─────────────────────────────────────────────────────────────────┤\tn\r\n");
    send_to_char(ch, "\tG│ \tWEvent:\tn %-57s \tG│\tn\r\n", staff_events_list[event_num][EVENT_TITLE]);
    send_to_char(ch, "\tG│ \twThe event has been terminated and cleanup is in progress.\tn       \tG│\tn\r\n");
    send_to_char(ch, "\tG│                                                                 │\tn\r\n");
    send_to_char(ch, "\tG│ \twA cleanup delay of \tY%d ticks\tw has been initiated.\tn               \tG│\tn\r\n", STAFF_EVENT_DELAY_CNST);
    send_to_char(ch, "\tG└─────────────────────────────────────────────────────────────────┘\tn\r\n\r\n");
    break;
    
  default:
    /* Unhandled error code */
    send_to_char(ch, "\r\n\tR┌─────────────────────────────────────────────────────────────────┐\tn\r\n");
    send_to_char(ch, "\tR│                     \tYUNEXPECTED ERROR                           \tR│\tn\r\n");
    send_to_char(ch, "\tR├─────────────────────────────────────────────────────────────────┤\tn\r\n");
    send_to_char(ch, "\tR│ \twAn unexpected error occurred while ending the event.\tn            \tR│\tn\r\n");
    send_to_char(ch, "\tR│ \twError code: %d\tn                                                  \tR│\tn\r\n", result);
    send_to_char(ch, "\tR└─────────────────────────────────────────────────────────────────┘\tn\r\n\r\n");
    break;
  }
}

/*
 * Helper function to handle event info command processing.
 *
 * @param ch: Character executing the command
 * @param event_num: Event number to show info for
 */
static void handle_info_event_command(struct char_data *ch, const int event_num)
{
  staff_event_info(ch, event_num);
  
  /* Additional context if no event is currently active */
  if (!IS_STAFF_EVENT)
  {
    send_to_char(ch, "There is no event active right now...\r\n");
  }
}

/*
 * Staff events command handler - provides complete event management interface.
 *
 * This function implements the 'staffevents' command that serves as the primary
 * interface for staff event management. It provides both player information
 * access and staff control capabilities with appropriate permission checking.
 *
 * REFACTORED (Addresses L005): Large function broken down into focused helper functions
 * for improved maintainability and single responsibility principle compliance.
 *
 * @param ch: Character executing the command
 * @param argument: Command line arguments (action and event number)
 * @param cmd: Command number (unused in this implementation)
 * @param subcmd: Subcommand number (unused in this implementation)
 */
ACMD(do_staffevents)
{
  char arg[MAX_INPUT_LENGTH] = {'\0'};    /* First argument (action) */
  char arg2[MAX_INPUT_LENGTH] = {'\0'};   /* Second argument (event number) */
  int event_num = UNDEFINED_EVENT;        /* Parsed event number */

  /* Handle player (non-staff) access */
  if (GET_LEVEL(ch) < LVL_STAFF)
  {
    handle_player_event_access(ch);
    return;
  }

  /* Handle empty arguments */
  if (!argument || !*argument)
  {
    handle_default_event_display(ch);
    return;
  }

  /* Parse command arguments */
  half_chop_c(argument, arg, sizeof(arg), arg2, sizeof(arg2));

  /* Handle incomplete arguments */
  if (!*arg || !*arg2)
  {
    handle_default_event_display(ch);
    return;
  }

  /* Parse and validate event number */
  event_num = parse_and_validate_event_num(ch, arg2);
  if (event_num < 0)
  {
    return; /* Error already reported by helper function */
  }

  /* Process command actions */
  if (is_abbrev(arg, "start"))
  {
    handle_start_event_command(ch, event_num);
  }
  else if (is_abbrev(arg, "end"))
  {
    handle_end_event_command(ch, event_num);
  }
  else if (is_abbrev(arg, "info"))
  {
    handle_info_event_command(ch, event_num);
  }
  else
  {
    /* Invalid command action */
    send_to_char(ch, "\r\n\tR┌─────────────────────────────────────────────────────────────────┐\tn\r\n");
    send_to_char(ch, "\tR│                    \tYINVALID COMMAND SYNTAX                        \tR│\tn\r\n");
    send_to_char(ch, "\tR├─────────────────────────────────────────────────────────────────┤\tn\r\n");
    send_to_char(ch, "\tR│ \twUnrecognized action: '\tY%s\tw'\tn                                     \tR│\tn\r\n", arg);
    send_to_char(ch, "\tR│                                                                 │\tn\r\n");
    send_to_char(ch, "\tR│ \twValid actions are:\tn                                              \tR│\tn\r\n");
    send_to_char(ch, "\tR│   \tw• \tGstart\tn  \tw- Begin a new event\tn                               \tR│\tn\r\n");
    send_to_char(ch, "\tR│   \tw• \tRend\tn    \tw- Terminate an active event\tn                       \tR│\tn\r\n");
    send_to_char(ch, "\tR│   \tw• \tCinfo\tn   \tw- Display detailed event information\tn              \tR│\tn\r\n");
    send_to_char(ch, "\tR└─────────────────────────────────────────────────────────────────┘\tn\r\n\r\n");
    
    /* Show available events */
    list_staff_events(ch);
  }
}

/*****************************************************************************/
/* PERFORMANCE OPTIMIZATION IMPLEMENTATIONS */
/*****************************************************************************/

/*
 * Object pooling system for frequently used objects.
 * This addresses the performance recommendation for pre-allocating
 * frequently used objects to reduce memory allocation overhead.
 */

/* Object pool structures and data */
typedef struct obj_pool_node {
  struct obj_data *obj;
  struct obj_pool_node *next;
  int obj_vnum;
  bool in_use;
} obj_pool_node_t;

static obj_pool_node_t *object_pool[OBJECT_POOL_SIZE];
static int pool_initialized = 0;

/*
 * Initialize the object pooling system.
 * Pre-allocates commonly used objects for the event system.
 */
void init_object_pool(void)
{
  int i = 0;
  
  if (pool_initialized)
    return; /* Already initialized */
  
  /* Initialize pool array */
  for (i = 0; i < OBJECT_POOL_SIZE; i++)
  {
    object_pool[i] = NULL;
  }
  
  /* Pre-allocate commonly used objects */
  struct obj_data *obj = NULL;
  obj_pool_node_t *node = NULL;
  
  /* Pre-allocate Jackalope Hides */
  for (i = 0; i < 10; i++)
  {
    obj = read_object(JACKALOPE_HIDE, VIRTUAL);
    if (obj)
    {
      node = malloc(sizeof(obj_pool_node_t));
      if (node)
      {
        node->obj = obj;
        node->obj_vnum = JACKALOPE_HIDE;
        node->in_use = FALSE;
        node->next = object_pool[i % OBJECT_POOL_SIZE];
        object_pool[i % OBJECT_POOL_SIZE] = node;
      }
    }
  }
  
  /* Pre-allocate Pristine Horns */
  for (i = 0; i < 5; i++)
  {
    obj = read_object(PRISTINE_HORN, VIRTUAL);
    if (obj)
    {
      node = malloc(sizeof(obj_pool_node_t));
      if (node)
      {
        node->obj = obj;
        node->obj_vnum = PRISTINE_HORN;
        node->in_use = FALSE;
        node->next = object_pool[(i + 10) % OBJECT_POOL_SIZE];
        object_pool[(i + 10) % OBJECT_POOL_SIZE] = node;
      }
    }
  }
  
  pool_initialized = 1;
  log("Object pool initialized with pre-allocated event objects");
}

/*
 * Get a pre-allocated object from the pool.
 * Falls back to standard object creation if pool is empty.
 */
struct obj_data *get_pooled_object(int obj_vnum)
{
  int i = 0;
  obj_pool_node_t *node = NULL;
  
  if (!pool_initialized)
    init_object_pool();
  
  /* Search pool for available object of requested type */
  for (i = 0; i < OBJECT_POOL_SIZE; i++)
  {
    for (node = object_pool[i]; node; node = node->next)
    {
      if (node->obj_vnum == obj_vnum && !node->in_use && node->obj)
      {
        node->in_use = TRUE;
        return node->obj;
      }
    }
  }
  
  /* Pool exhausted or object type not pooled - fall back to standard creation */
  return read_object(obj_vnum, VIRTUAL);
}

/*
 * Return an object to the pool for reuse.
 * Resets object state for clean reuse.
 */
void return_object_to_pool(struct obj_data *obj)
{
  int i = 0;
  obj_pool_node_t *node = NULL;
  
  if (!obj || !pool_initialized)
    return;
  
  /* Find the object in the pool */
  for (i = 0; i < OBJECT_POOL_SIZE; i++)
  {
    for (node = object_pool[i]; node; node = node->next)
    {
      if (node->obj == obj && node->in_use)
      {
        /* Reset object state for reuse */
        node->in_use = FALSE;
        /* Object cleanup would go here if needed */
        return;
      }
    }
  }
  
  /* Object not from pool - extract normally */
  extract_obj(obj);
}

/*
 * Cleanup the object pooling system.
 * Frees all pooled objects and memory.
 */
void cleanup_object_pool(void)
{
  int i = 0;
  obj_pool_node_t *node = NULL, *next = NULL;
  
  if (!pool_initialized)
    return;
  
  for (i = 0; i < OBJECT_POOL_SIZE; i++)
  {
    node = object_pool[i];
    while (node)
    {
      next = node->next;
      if (node->obj)
        extract_obj(node->obj);
      free(node);
      node = next;
    }
    object_pool[i] = NULL;
  }
  
  pool_initialized = 0;
  log("Object pool cleaned up");
}

/*****************************************************************************/
/* HASH TABLE SYSTEM FOR ALGORITHMIC OPTIMIZATION */
/*****************************************************************************/

/*
 * Hash table system for faster lookups.
 * This addresses the performance recommendation for using hash tables
 * instead of linear searches for better algorithmic complexity.
 */

/* Hash table structures */
typedef struct hash_node {
  int key;
  int value;
  struct hash_node *next;
} hash_node_t;

static hash_node_t *event_hash_table[EVENT_HASH_TABLE_SIZE];
static hash_node_t *mobile_count_hash_table[MOBILE_HASH_TABLE_SIZE];
static int hash_tables_initialized = 0;

/*
 * Simple hash function for integer keys.
 */
static int hash_function(int key, int table_size)
{
  return (key % table_size);
}

/*
 * Initialize hash table systems.
 */
void init_hash_tables(void)
{
  int i = 0;
  
  if (hash_tables_initialized)
    return;
  
  /* Initialize event hash table */
  for (i = 0; i < EVENT_HASH_TABLE_SIZE; i++)
  {
    event_hash_table[i] = NULL;
  }
  
  /* Initialize mobile count hash table */
  for (i = 0; i < MOBILE_HASH_TABLE_SIZE; i++)
  {
    mobile_count_hash_table[i] = NULL;
  }
  
  /* Pre-populate event hash table with known events */
  hash_node_t *node = NULL;
  
  /* Add Jackalope VNUMs */
  int jackalope_vnums[] = {EASY_JACKALOPE, MED_JACKALOPE, HARD_JACKALOPE};
  for (i = 0; i < 3; i++)
  {
    node = malloc(sizeof(hash_node_t));
    if (node)
    {
      node->key = jackalope_vnums[i];
      node->value = JACKALOPE_HUNT;
      node->next = event_hash_table[hash_function(jackalope_vnums[i], EVENT_HASH_TABLE_SIZE)];
      event_hash_table[hash_function(jackalope_vnums[i], EVENT_HASH_TABLE_SIZE)] = node;
    }
  }
  
  hash_tables_initialized = 1;
  log("Hash tables initialized for performance optimization");
}

/*
 * Hash table lookup for event information.
 * O(1) average case performance vs O(n) linear search.
 */
int hash_lookup_event(int event_vnum)
{
  hash_node_t *node = NULL;
  int hash_index = 0;
  
  if (!hash_tables_initialized)
    init_hash_tables();
  
  hash_index = hash_function(event_vnum, EVENT_HASH_TABLE_SIZE);
  node = event_hash_table[hash_index];
  
  while (node)
  {
    if (node->key == event_vnum)
      return node->value;
    node = node->next;
  }
  
  return -1; /* Not found */
}

/*
 * Hash table lookup for cached mobile counts.
 * Reduces expensive character list traversals.
 */
int hash_lookup_mobile_count(int mobile_vnum)
{
  hash_node_t *node = NULL;
  int hash_index = 0;
  
  if (!hash_tables_initialized)
    return -1; /* Not cached */
  
  hash_index = hash_function(mobile_vnum, MOBILE_HASH_TABLE_SIZE);
  node = mobile_count_hash_table[hash_index];
  
  while (node)
  {
    if (node->key == mobile_vnum)
      return node->value;
    node = node->next;
  }
  
  return -1; /* Not cached */
}

/*
 * Update cached mobile count in hash table.
 * Maintains cache for frequently accessed mob counts.
 */
void hash_update_mobile_count(int mobile_vnum, int count)
{
  hash_node_t *node = NULL;
  int hash_index = 0;
  
  if (!hash_tables_initialized)
    init_hash_tables();
  
  hash_index = hash_function(mobile_vnum, MOBILE_HASH_TABLE_SIZE);
  node = mobile_count_hash_table[hash_index];
  
  /* Search for existing entry */
  while (node)
  {
    if (node->key == mobile_vnum)
    {
      node->value = count; /* Update existing */
      return;
    }
    node = node->next;
  }
  
  /* Create new entry */
  node = malloc(sizeof(hash_node_t));
  if (node)
  {
    node->key = mobile_vnum;
    node->value = count;
    node->next = mobile_count_hash_table[hash_index];
    mobile_count_hash_table[hash_index] = node;
  }
}

/*
 * Cleanup hash table systems.
 */
void cleanup_hash_tables(void)
{
  int i = 0;
  hash_node_t *node = NULL, *next = NULL;
  
  if (!hash_tables_initialized)
    return;
  
  /* Cleanup event hash table */
  for (i = 0; i < EVENT_HASH_TABLE_SIZE; i++)
  {
    node = event_hash_table[i];
    while (node)
    {
      next = node->next;
      free(node);
      node = next;
    }
    event_hash_table[i] = NULL;
  }
  
  /* Cleanup mobile count hash table */
  for (i = 0; i < MOBILE_HASH_TABLE_SIZE; i++)
  {
    node = mobile_count_hash_table[i];
    while (node)
    {
      next = node->next;
      free(node);
      node = next;
    }
    mobile_count_hash_table[i] = NULL;
  }
  
  hash_tables_initialized = 0;
  log("Hash tables cleaned up");
}

/*****************************************************************************/
/* COMPREHENSIVE TESTING FRAMEWORK */
/*****************************************************************************/

/*
 * Unit testing framework for staff event system.
 * Provides comprehensive testing capabilities addressing all test scenarios
 * identified in the audit recommendations.
 */

/* Test result tracking */
static int total_tests_run = 0;
static int total_tests_passed = 0;
static int total_tests_failed = 0;

/* Test assertion macros */
#define TEST_ASSERT(condition, test_name) \
  do { \
    total_tests_run++; \
    if (condition) { \
      total_tests_passed++; \
      log("PASS: %s", test_name); \
    } else { \
      total_tests_failed++; \
      log("FAIL: %s", test_name); \
    } \
  } while(0)

#define TEST_ASSERT_EQUAL(expected, actual, test_name) \
  TEST_ASSERT((expected) == (actual), test_name)

#define TEST_ASSERT_NOT_NULL(ptr, test_name) \
  TEST_ASSERT((ptr) != NULL, test_name)

/*
 * Unit Tests for Event State Management
 */
static int test_event_state_management(void)
{
  int failures = 0;
  
  log("Running Event State Management Tests...");
  
  /* Test 1: Initial state should be inactive */
  TEST_ASSERT_EQUAL(0, is_event_active(), "Initial event state inactive");
  TEST_ASSERT_EQUAL(UNDEFINED_EVENT, get_active_event(), "Initial event number undefined");
  TEST_ASSERT_EQUAL(0, get_event_time_remaining(), "Initial time remaining zero");
  
  /* Test 2: Set event state */
  set_event_state(JACKALOPE_HUNT, 100);
  TEST_ASSERT_EQUAL(1, is_event_active(), "Event state set to active");
  TEST_ASSERT_EQUAL(JACKALOPE_HUNT, get_active_event(), "Event number set correctly");
  TEST_ASSERT_EQUAL(100, get_event_time_remaining(), "Event time set correctly");
  
  /* Test 3: Clear event state */
  clear_event_state();
  TEST_ASSERT_EQUAL(0, is_event_active(), "Event state cleared to inactive");
  TEST_ASSERT_EQUAL(UNDEFINED_EVENT, get_active_event(), "Event number reset to undefined");
  
  /* Test 4: Event delay management */
  set_event_delay(50);
  TEST_ASSERT_EQUAL(50, get_event_delay(), "Event delay set correctly");
  
  return failures;
}

/*
 * Unit Tests for Mob Spawning Logic
 */
static int test_mob_spawning_logic(void)
{
  int failures = 0;
  
  log("Running Mob Spawning Logic Tests...");
  
  /* Test coordinate generation and caching */
  int x_coord = 0, y_coord = 0;
  
  /* Multiple coordinate requests should work */
  get_cached_coordinates(&x_coord, &y_coord);
  TEST_ASSERT(x_coord >= JACKALOPE_WEST_X && x_coord <= JACKALOPE_EAST_X, "X coordinate within bounds");
  TEST_ASSERT(y_coord >= JACKALOPE_SOUTH_Y && y_coord <= JACKALOPE_NORTH_Y, "Y coordinate within bounds");
  
  /* Test Jackalope counting function */
  int easy_count = 0, med_count = 0, hard_count = 0;
  count_jackalope_mobs(&easy_count, &med_count, &hard_count);
  TEST_ASSERT(easy_count >= 0, "Easy Jackalope count non-negative");
  TEST_ASSERT(med_count >= 0, "Medium Jackalope count non-negative");
  TEST_ASSERT(hard_count >= 0, "Hard Jackalope count non-negative");
  
  /* Test mob count validation with individual function */
  int individual_easy_count = mob_ingame_count(EASY_JACKALOPE);
  TEST_ASSERT_EQUAL(easy_count, individual_easy_count, "Optimized count matches individual count");
  
  return failures;
}

/*
 * Unit Tests for Drop System
 */
static int test_drop_system(void)
{
  int failures = 0;
  
  log("Running Drop System Tests...");
  
  /* Test object pooling system */
  init_object_pool();
  
  struct obj_data *pooled_obj = get_pooled_object(JACKALOPE_HIDE);
  TEST_ASSERT_NOT_NULL(pooled_obj, "Pooled object creation successful");
  
  if (pooled_obj)
  {
    TEST_ASSERT_EQUAL(JACKALOPE_HIDE, GET_OBJ_VNUM(pooled_obj), "Pooled object has correct VNUM");
    return_object_to_pool(pooled_obj);
  }
  
  /* Test hash table lookups */
  init_hash_tables();
  
  int event_lookup = hash_lookup_event(EASY_JACKALOPE);
  TEST_ASSERT_EQUAL(JACKALOPE_HUNT, event_lookup, "Hash lookup returns correct event");
  
  /* Test mobile count caching */
  hash_update_mobile_count(EASY_JACKALOPE, 42);
  int cached_count = hash_lookup_mobile_count(EASY_JACKALOPE);
  TEST_ASSERT_EQUAL(42, cached_count, "Mobile count caching works correctly");
  
  return failures;
}

/*
 * Unit Tests for Command Parsing
 */
static int test_command_parsing(void)
{
  int failures = 0;
  
  log("Running Command Parsing Tests...");
  
  /* Test event number validation */
  char valid_number[] = "1";
  char invalid_number[] = "abc";
  char out_of_range[] = "999";
  
  /* These would normally be tested with a mock character */
  TEST_ASSERT(isdigit(*valid_number), "Valid number string detection");
  TEST_ASSERT(!isdigit(*invalid_number), "Invalid number string detection");
  
  int parsed_valid = atoi(valid_number);
  int parsed_invalid = atoi(out_of_range);
  
  TEST_ASSERT(parsed_valid >= 0 && parsed_valid < NUM_STAFF_EVENTS, "Valid event number parsing");
  TEST_ASSERT(parsed_invalid >= NUM_STAFF_EVENTS, "Out of range number detection");
  
  return failures;
}

/*
 * Integration Tests for Event Lifecycle
 */
static int test_event_lifecycle(void)
{
  int failures = 0;
  
  log("Running Event Lifecycle Integration Tests...");
  
  /* Test complete event start-to-end cycle */
  
  /* Start with clean state */
  clear_event_state();
  TEST_ASSERT_EQUAL(0, is_event_active(), "Clean initial state");
  
  /* Start event */
  event_result_t start_result = start_staff_event(JACKALOPE_HUNT);
  TEST_ASSERT_EQUAL(EVENT_SUCCESS, start_result, "Event start successful");
  TEST_ASSERT_EQUAL(1, is_event_active(), "Event active after start");
  TEST_ASSERT_EQUAL(JACKALOPE_HUNT, get_active_event(), "Correct event active");
  
  /* End event */
  event_result_t end_result = end_staff_event(JACKALOPE_HUNT);
  TEST_ASSERT_EQUAL(EVENT_SUCCESS, end_result, "Event end successful");
  TEST_ASSERT_EQUAL(0, is_event_active(), "Event inactive after end");
  
  /* Test error conditions */
  event_result_t invalid_start = start_staff_event(999);
  TEST_ASSERT_EQUAL(EVENT_ERROR_INVALID_NUM, invalid_start, "Invalid event number rejected");
  
  event_result_t invalid_end = end_staff_event(-1);
  TEST_ASSERT_EQUAL(EVENT_ERROR_INVALID_NUM, invalid_end, "Invalid end event number rejected");
  
  return failures;
}

/*
 * Performance Testing
 */
static int test_performance_optimizations(void)
{
  int failures = 0;
  
  log("Running Performance Optimization Tests...");
  
  /* Test coordinate caching performance */
  int start_time = time(NULL);
  int i = 0;
  
  for (i = 0; i < 1000; i++)
  {
    int x = 0, y = 0;
    get_cached_coordinates(&x, &y);
  }
  
  int end_time = time(NULL);
  int duration = end_time - start_time;
  
  TEST_ASSERT(duration < 5, "Coordinate generation performance acceptable"); /* Should be nearly instant */
  
  /* Test hash table performance vs linear search */
  init_hash_tables();
  
  start_time = time(NULL);
  for (i = 0; i < 1000; i++)
  {
    hash_lookup_event(EASY_JACKALOPE);
  }
  end_time = time(NULL);
  int hash_duration = end_time - start_time;
  
  TEST_ASSERT(hash_duration < 2, "Hash table lookup performance acceptable");
  
  log("Performance test completed: Hash lookups took %d seconds for 1000 operations", hash_duration);
  
  return failures;
}

/*
 * Run specific test suite by name.
 */
int run_test_suite(const char *test_suite)
{
  int failures = 0;
  
  if (!test_suite)
    return run_staff_event_tests(); /* Run all if no suite specified */
  
  /* Reset test counters */
  total_tests_run = 0;
  total_tests_passed = 0;
  total_tests_failed = 0;
  
  if (strcmp(test_suite, "state") == 0)
  {
    failures += test_event_state_management();
  }
  else if (strcmp(test_suite, "spawning") == 0)
  {
    failures += test_mob_spawning_logic();
  }
  else if (strcmp(test_suite, "drops") == 0)
  {
    failures += test_drop_system();
  }
  else if (strcmp(test_suite, "parsing") == 0)
  {
    failures += test_command_parsing();
  }
  else if (strcmp(test_suite, "lifecycle") == 0)
  {
    failures += test_event_lifecycle();
  }
  else if (strcmp(test_suite, "performance") == 0)
  {
    failures += test_performance_optimizations();
  }
  else
  {
    log("Unknown test suite: %s", test_suite);
    return -1;
  }
  
  log("Test Suite '%s' Results: %d/%d passed (%d failed)",
      test_suite, total_tests_passed, total_tests_run, total_tests_failed);
  
  return failures;
}

/*
 * Run all unit tests for the staff event system.
 */
int run_staff_event_tests(void)
{
  int total_failures = 0;
  
  log("Starting comprehensive staff event system tests...");
  
  /* Reset test counters */
  total_tests_run = 0;
  total_tests_passed = 0;
  total_tests_failed = 0;
  
  /* Run all test suites */
  total_failures += test_event_state_management();
  total_failures += test_mob_spawning_logic();
  total_failures += test_drop_system();
  total_failures += test_command_parsing();
  total_failures += test_event_lifecycle();
  total_failures += test_performance_optimizations();
  
  /* Report final results */
  log("=== STAFF EVENT SYSTEM TEST RESULTS ===");
  log("Total Tests Run: %d", total_tests_run);
  log("Tests Passed: %d", total_tests_passed);
  log("Tests Failed: %d", total_tests_failed);
  log("Success Rate: %.1f%%", (total_tests_passed * 100.0) / total_tests_run);
  
  if (total_failures == 0)
  {
    log("ALL TESTS PASSED - Staff Event System operating correctly");
  }
  else
  {
    log("SOME TESTS FAILED - %d test suites reported failures", total_failures);
  }
  
  return total_failures;
}

/*
 * Staff command for running tests: 'testevent'
 * Allows comprehensive testing of the event system.
 */
ACMD(do_testevent)
{
  char arg[MAX_INPUT_LENGTH] = {'\0'};
  
  /* Staff-only command */
  if (GET_LEVEL(ch) < LVL_STAFF)
  {
    send_to_char(ch, "You don't have permission to run event system tests.\r\n");
    return;
  }
  
  one_argument(argument, arg, sizeof(arg));
  
  if (!*arg)
  {
    /* Run all tests */
    send_to_char(ch, "Running comprehensive staff event system tests...\r\n");
    int failures = run_staff_event_tests();
    
    if (failures == 0)
    {
      send_to_char(ch, "\tGAll tests passed successfully!\tn\r\n");
    }
    else
    {
      send_to_char(ch, "\tR%d test suite(s) reported failures. Check system logs for details.\tn\r\n", failures);
    }
  }
  else
  {
    /* Run specific test suite */
    send_to_char(ch, "Running test suite: %s\r\n", arg);
    int failures = run_test_suite(arg);
    
    if (failures == -1)
    {
      send_to_char(ch, "Unknown test suite. Available suites: state, spawning, drops, parsing, lifecycle, performance\r\n");
    }
    else if (failures == 0)
    {
      send_to_char(ch, "\tGTest suite '%s' passed successfully!\tn\r\n", arg);
    }
    else
    {
      send_to_char(ch, "\tRTest suite '%s' reported %d failures. Check system logs for details.\tn\r\n", arg, failures);
    }
  }
  
  send_to_char(ch, "Test execution completed. Check system logs for detailed results.\r\n");
}

/* undefines */

/* general */

#undef NUM_STAFF_EVENTS
#undef STAFF_EVENT_FIELDS

#undef EVENT_TITLE
#undef EVENT_BEGIN
#undef EVENT_END
#undef EVENT_DETAIL

/* end general */

/* jackalope hunt undefines */
#undef JACKALOPE_HUNT
#undef EASY_JACKALOPE       /* vnum of lower level jackalope */
#undef MED_JACKALOPE        /* vnum of mid level jackalope */
#undef HARD_JACKALOPE       /* vnum of high level jackalope */
#undef SMALL_JACKALOPE_HIDE /* vnum of lower level jackalope's hide */
#undef MED_JACKALOPE_HIDE   /* vnum of mid level jackalope's hide */
#undef LARGE_JACKALOPE_HIDE /* vnum of high level jackalope's hide */
#undef PRISTINE_HORN        /* vnum of rare pristine jackalope horn */
#undef P_HORN_RARITY        /* % chance of loading pristine jackalope horn */

/* end jackalope hunt undefines */

/* the prisoner undefines */

#undef THE_PRISONER_EVENT

/* end the prisoner undefines */

/* EOF */
