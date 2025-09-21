/**************************************************************************
 *  File: mud_event.c                                  Part of LuminariMUD *
 *  Usage: Handling of the mud event system                                *
 *                                                                         *
 *  By Vatiken. Copyright 2012 by Joseph Arnusch                           *
 *  Re-written by LuminariMUD staff to fix the original code.              *
 **************************************************************************
 * 
 * BEGINNER'S GUIDE TO THE MUD EVENT SYSTEM:
 * 
 * The MUD event system handles timed events in the game. Think of it like
 * setting multiple timers that will trigger actions after a certain time.
 * 
 * KEY CONCEPTS:
 * 1. Events can be attached to different entities:
 *    - Characters (players/NPCs) 
 *    - Objects (items in the game)
 *    - Rooms (locations in the game world)
 *    - Regions (collections of rooms/areas)
 *    - The world itself (global events)
 * 
 * 2. Each event has:
 *    - An ID (what type of event it is)
 *    - A timer (when it will trigger)
 *    - Data (the entity it's attached to)
 *    - Variables (optional data specific to this event instance)
 * 
 * 3. Memory Management is CRITICAL:
 *    - When events are created, memory is allocated
 *    - When events complete or are cancelled, memory must be freed
 *    - Memory leaks will cause the game to crash over time
 * 
 * 4. Event Lifecycle:
 *    - new_mud_event(): Creates the event data structure
 *    - attach_mud_event(): Attaches event to an entity and starts timer
 *    - event_countdown(): Executes when the timer expires
 *    - free_mud_event(): Cleans up memory when event is done
 * 
 **************************************************************************/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "db.h"
#include "dg_event.h"
#include "constants.h"
#include "comm.h" /* For access to the game pulse */
#include "lists.h"
#include "mud_event.h"
#include "handler.h"
#include "wilderness.h"
#include "quest.h"
#include "mysql.h"
#include "act.h"
#include "brew.h"  /* Include for brewing events */

/* Global List */
struct list_data *world_events = NULL;

/* The mud_event_index[] is defined in mud_event_list.c */
extern struct mud_event_list mud_event_index[];

/* init_events() is the ideal function for starting global events. This
 * might be the case if you were to move the contents of heartbeat() into
 * the event system */
void init_events(void)
{
  /* Allocate Event List */
  world_events = create_list();
}

/* The bottom switch() is for any post-event actions, like telling the character they can
 * now access their skill again.
 */
EVENTFUNC(event_countdown)
{
  struct mud_event_data *pMudEvent = NULL;
  struct char_data *ch = NULL;
  /* struct room_data *room = NULL; */ /* Unused variable */
  /* struct obj_data *obj = NULL; */   /* Unused variable */
  room_vnum *rvnum = NULL;
  room_rnum rnum = NOWHERE;
  region_vnum *regvnum = NULL;
  region_rnum regrnum = NOWHERE;
  /* obj_vnum *obj_vnum = NULL; */
  /* obj_rnum obj_rnum = NOWHERE; */
  int index = 0, qvnum = NOTHING;

  char **tokens; /* Storage for tokenized encounter room vnums */
  char **it;     /* Token iterator */

  pMudEvent = (struct mud_event_data *)event_obj;

  if (!pMudEvent)
    return 0;

  if (!pMudEvent->iId)
    return 0;

  /* Determine what type of entity this event is attached to */
  switch (mud_event_index[pMudEvent->iId].iEvent_Type)
  {
  case EVENT_CHAR:
    ch = (struct char_data *)pMudEvent->pStruct;
    break;
  case EVENT_OBJECT:
    /* obj = (struct obj_data *)pMudEvent->pStruct; */ /* Unused assignment */
    /* obj_rnum = real_obj(*obj_vnum); */
    /* obj = &obj[real_obj(obj_rnum)]; */
    break;
  case EVENT_ROOM:
    /* SAFETY CHECK: Ensure pStruct is not NULL before dereferencing.
     * This prevents crashes if the event data is corrupted. */
    if (!pMudEvent->pStruct) {
      log("SYSERR: event_countdown() - ROOM event with NULL pStruct!");
      return 0;
    }
    rvnum = (room_vnum *)pMudEvent->pStruct;
    rnum = real_room(*rvnum);
    /* Verify the room exists before we use it later */
    if (rnum == NOWHERE) {
      log("SYSERR: event_countdown() - ROOM event for invalid vnum %d", *rvnum);
      return 0;
    }
    /* room = &world[real_room(rnum)]; */ /* Unused assignment */
    break;
  case EVENT_REGION:
    /* SAFETY CHECK: Ensure pStruct is not NULL before dereferencing.
     * This prevents crashes if the event data is corrupted. */
    if (!pMudEvent->pStruct) {
      log("SYSERR: event_countdown() - REGION event with NULL pStruct!");
      return 0;
    }
    regvnum = (region_vnum *)pMudEvent->pStruct;
    regrnum = real_region(*regvnum);
    /* log("LOG: EVENT_REGION case in EVENTFUNC(event_countdown): Region VNum %d, RNum %d", *regvnum, regrnum); */
    break;
  default:
    break;
  }

  /* First handle standard messages from the table */
  if (mud_event_index[pMudEvent->iId].completion_msg && ch)
  {
    /* Special case: eSTRUGGLE only shows message if grappled */
    if (pMudEvent->iId == eSTRUGGLE)
    {
      if (AFF_FLAGGED(ch, AFF_GRAPPLED))
        send_to_char(ch, "%s\r\n", mud_event_index[pMudEvent->iId].completion_msg);
    }
    else
    {
      send_to_char(ch, "%s\r\n", mud_event_index[pMudEvent->iId].completion_msg);
    }
  }

  /* Now handle special cases that need more than just a message */
  switch (pMudEvent->iId)
  {
  case eDARKNESS:
    /* SAFETY: Check that we have a valid room before accessing it.
     * The rnum should have been set in the EVENT_ROOM case above. */
    if (rnum == NOWHERE) {
      log("SYSERR: eDARKNESS event triggered but room is NOWHERE!");
      break;
    }
    /* Now safe to access the room flags and send messages */
    REMOVE_BIT_AR(ROOM_FLAGS(rnum), ROOM_DARK);
    send_to_room(rnum, "The dark shroud dissipates.\r\n");
    break;
    
  case ePURGEMOB:
    send_to_char(ch, "You must return to your home plane!\r\n");
    act("With a sigh of relief $n fades out of this plane!",
        FALSE, ch, NULL, NULL, TO_ROOM);
    extract_char(ch);
    break;
    
  case eCOLLECT_DELAY:
    perform_collect(ch, FALSE);
    break;
    
  case eBLUR_ATTACK_DELAY:
    /* No message needed */
    break;
    
  case eQUEST_COMPLETE:
    qvnum = atoi((char *)pMudEvent->sVariables);
    for (index = 0; index < MAX_CURRENT_QUESTS; index++)
      if (qvnum != NOTHING && qvnum == GET_QUEST(ch, index))
        complete_quest(ch, index);
    break;
    
  case eSPELLBATTLE:
    SPELLBATTLE(ch) = 0;
    break;
    
  case eENCOUNTER_REG_RESET:
    /* Testing */
    if (regrnum == NOWHERE)
    {
      log("SYSERR: event_countdown for eENCOUNTER_REG_RESET, region out of bounds.");
      break;
    }
    /* log("Encounter Region '%s' with vnum: %d reset.", region_table[regrnum].name, region_table[regrnum].vnum); */

    if (pMudEvent->sVariables == NULL)
    {
      /* This encounter region has no encounter rooms. */
      log("SYSERR: No encounter rooms set for encounter region vnum: %d", *regvnum);
    }
    else
    {
      /* Process all encounter rooms for this region */
      tokens = tokenize(pMudEvent->sVariables, ",");
      if (!tokens) {
        log("SYSERR: tokenize() failed in event_countdown for region %d", *regvnum);
        break;  /* Exit this case */
      }

      for (it = tokens; it && *it; ++it)
      {
        room_vnum eroom_vnum;
        room_rnum eroom_rnum = NOWHERE;
        int x, y;

        sscanf(*it, "%d", &eroom_vnum);
        eroom_rnum = real_room(eroom_vnum);
        /* This log is causing lots of spam in our syslog.  Removing it. */
        /* log("LOG: Processing encounter room vnum: %d", eroom_vnum); */

        if (eroom_rnum == NOWHERE)
        {
          log("  ERROR: Encounter room is NOWHERE");
          continue;
        }

        /* First check that the encounter room is empty of players */
        if (world[eroom_rnum].people != NULL)
        {
          /* Someone is in the room, so skip this one. */
          continue;
        }

        /* Find a location in the region where this room will be placed,
             it can not be the same coords as a static room and noone should be at those coordinates. */
        int ctr = 0;
        do
        {

          /* Generate the random point */
          get_random_region_location(*regvnum, &x, &y);

          /* Check for a static room at this location. */
          if (find_room_by_coordinates(x, y) == NOWHERE)
          {
            /* Make sure the sector types match. */
            if (world[eroom_rnum].sector_type == get_modified_sector_type(GET_ROOM_ZONE(eroom_rnum), x, y))
            {
              break;
            }
          }
        } while (++ctr < 128);

        /* Build the room. */
        /* assign_wilderness_room(eroom_rnum, x, y); */
        world[eroom_rnum].coords[0] = x;
        world[eroom_rnum].coords[1] = y;
      }
      initialize_wilderness_lists();
      free_tokens(tokens); /* Free the tokenized list */
    }

    return 60 RL_SEC;

    break;
  default:
    break;
  }

  return 0;
}

EVENTFUNC(event_daily_use_cooldown)
{
  struct mud_event_data *pMudEvent = NULL;
  struct char_data *ch = NULL;
  /* struct obj_data *obj = NULL; */ /* Unused variable */
  int cooldown = 0;
  int uses = 0;
  int nonfeat_daily_uses = 0;
  int featnum = 0;
  char buf[128];

  pMudEvent = (struct mud_event_data *)event_obj;

  if (!pMudEvent)
    return 0;

  if (!pMudEvent->iId)
    return 0;

  /* Get the entity this event is attached to */
  switch (mud_event_index[pMudEvent->iId].iEvent_Type)
  {
  case EVENT_CHAR:
    ch = (struct char_data *)pMudEvent->pStruct;
    break;
  case EVENT_OBJECT:
    /* obj = (struct obj_data *)pMudEvent->pStruct; */ /* Unused assignment */
    break;
  default:
    return 0;
  }

  if (pMudEvent->sVariables == NULL)
  {
    /* This is odd - This field should always be populated for daily-use abilities,
     * maybe some legacy code or bad id. */
    log("SYSERR: 1 sVariables field is NULL for daily-use-cooldown-event: %d", pMudEvent->iId);
  }
  else
  {
    if (sscanf(pMudEvent->sVariables, "uses:%d", &uses) != 1)
    {
      log("SYSERR: In event_daily_use_cooldown, bad sVariables for daily-use-cooldown-event: %d", pMudEvent->iId);
      uses = 0;
    }
  }

  /* Get feat and daily uses from the table */
  featnum = mud_event_index[pMudEvent->iId].feat_num;
  nonfeat_daily_uses = mud_event_index[pMudEvent->iId].daily_uses;

  /* Send recovery message from table if available */
  if (mud_event_index[pMudEvent->iId].recovery_msg && ch)
  {
    send_to_char(ch, "%s\r\n", mud_event_index[pMudEvent->iId].recovery_msg);
  }
  /* Fallback to completion message if no recovery message */
  else if (mud_event_index[pMudEvent->iId].completion_msg && ch)
  {
    send_to_char(ch, "%s\r\n", mud_event_index[pMudEvent->iId].completion_msg);
  }

  uses -= 1;
  if (uses > 0)
  {
    if (pMudEvent->sVariables != NULL)
      free(pMudEvent->sVariables);

    snprintf(buf, sizeof(buf), "uses:%d", uses);
    pMudEvent->sVariables = strdup(buf);

    if ((featnum == FEAT_UNDEFINED) && (nonfeat_daily_uses > 0))
    {
      /*
        This is a 'daily' feature that is not controlled by a feat - for example a weapon or armor special ability.
        In this case, the daily uses must be set above - variable nonfeat_daily_uses.
      */
      
      /* CRITICAL FIX: Integer Overflow Prevention
       * Before: cooldown = (SECS_PER_MUD_DAY / nonfeat_daily_uses) RL_SEC;
       * Problem: The multiplication could overflow if the division result is large.
       * Solution: Use long math to avoid overflow, then ensure result fits in int range.
       * 
       * IMPORTANT: RL_SEC is defined as *PASSES_PER_SEC (which equals *10)
       * So the original line expands to: (SECS_PER_MUD_DAY / nonfeat_daily_uses) * 10
       * 
       * Math explanation for beginners:
       * - SECS_PER_MUD_DAY = 24 * 75 = 1800 (MUD seconds in a MUD day)
       * - PASSES_PER_SEC = 10 (game ticks per real second)
       * - If nonfeat_daily_uses is 1: 1800 * 10 = 18000 pulses = 1800 real seconds (30 minutes)
       * - This converts MUD time to real-time ticks
       */
      long temp_cooldown = ((long)SECS_PER_MUD_DAY / nonfeat_daily_uses) RL_SEC;
      
      /* Clamp to reasonable maximum (1 real-time day) */
      if (temp_cooldown > 864000) {  /* 86400 seconds * 10 ticks/sec */
        log("WARNING: Cooldown overflow prevented for non-feat daily ability, clamping to 1 day");
        cooldown = 864000;
      } else {
        cooldown = (int)temp_cooldown;
      }
    }
    else if (get_daily_uses(ch, featnum) > 0)  /* Fixed: Check > 0 instead of just != 0 */
    { 
      /* CRITICAL FIX: Division by Zero Check Enhanced
       * Before: else if (get_daily_uses(ch, featnum))
       * Problem: Only checked for non-zero, but negative values would still crash
       * Solution: Explicitly check for positive values
       * 
       * Also applying the same overflow protection as above
       */
      int daily_uses = get_daily_uses(ch, featnum);
      
      /* Extra safety: Ensure daily_uses is positive */
      if (daily_uses <= 0) {
        log("SYSERR: Invalid daily_uses %d for feat %d on character %s", 
            daily_uses, featnum, GET_NAME(ch));
        cooldown = 0;  /* No cooldown if invalid */
      } else {
        /* RL_SEC expands to *PASSES_PER_SEC, so we need the parentheses */
        long temp_cooldown = ((long)SECS_PER_MUD_DAY / daily_uses) RL_SEC;
        
        /* Clamp to reasonable maximum (1 real-time day) */
        if (temp_cooldown > 864000) {  /* 86400 seconds * 10 ticks/sec */
          log("WARNING: Cooldown overflow prevented for feat %d, clamping to 1 day", featnum);
          cooldown = 864000;
        } else {
          cooldown = (int)temp_cooldown;
        }
      }
    }
  }

  return cooldown;
}

/*
 * BEGINNER'S GUIDE: attach_mud_event()
 * 
 * This function "attaches" an event to an entity and starts its timer.
 * Think of it like setting an alarm clock - after 'time' ticks, the
 * event will trigger and execute its associated function.
 * 
 * PARAMETERS:
 * - pMudEvent: The event data (what to do, what entity it's for)
 * - time: How many game ticks until the event triggers
 * 
 * CRITICAL MEMORY MANAGEMENT:
 * For ROOM and REGION events, we create our own copy of the vnum.
 * This is because the caller might pass temporary memory that gets
 * freed after this function returns. We need the vnum to persist
 * for the entire lifetime of the event!
 */
void attach_mud_event(struct mud_event_data *pMudEvent, long time)
{
  struct event *pEvent = NULL;

  struct descriptor_data *d = NULL;
  struct char_data *ch = NULL;
  struct room_data *room = NULL;
  struct region_data *region = NULL;
  struct obj_data *obj = NULL;

  room_vnum *rvnum = NULL;
  region_vnum *regvnum = NULL;

  /* Create the actual event and set its timer.
   * event_create() adds it to the global event queue. */
  pEvent = event_create(mud_event_index[pMudEvent->iId].func, pMudEvent, time);
  pEvent->isMudEvent = TRUE;
  pMudEvent->pEvent = pEvent;

  /* Add event to the appropriate entity's event list.
   * Each entity type maintains its own list of active events. */
  switch (mud_event_index[pMudEvent->iId].iEvent_Type)
  {
  case EVENT_WORLD:
    add_to_list(pEvent, world_events);
    break;
  case EVENT_DESC:
    d = (struct descriptor_data *)pMudEvent->pStruct;
    add_to_list(pEvent, d->events);
    break;
  case EVENT_CHAR:
    ch = (struct char_data *)pMudEvent->pStruct;

    if (ch->events == NULL)
      ch->events = create_list();

    add_to_list(pEvent, ch->events);
    break;
  case EVENT_OBJECT:
    obj = (struct obj_data *)pMudEvent->pStruct;

    if (obj->events == NULL)
      obj->events = create_list();

    add_to_list(pEvent, obj->events);
    break;
  case EVENT_ROOM:
    /* CRITICAL FIX: The original code had a memory leak here.
     * pMudEvent->pStruct initially points to a room_vnum passed from the caller.
     * We need to copy this value to our own allocated memory that we'll manage,
     * but we must NOT lose the original pointer if it was dynamically allocated.
     * 
     * For ROOM events, pStruct should contain a room_vnum that was passed in.
     * We create our own copy because the event system needs to own this memory
     * for the lifetime of the event. */
    
    /* Create new memory to store the room vnum for this event */
    CREATE(rvnum, room_vnum, 1);
    
    /* Copy the vnum value from the original pointer to our new memory */
    *rvnum = *((room_vnum *)pMudEvent->pStruct);
    
    /* Now update pStruct to point to our newly allocated memory.
     * The original pointer passed in is NOT freed here because we don't
     * own it - the caller is responsible for their own memory. */
    pMudEvent->pStruct = rvnum;
    
    /* BOUNDS CHECK: Ensure the room exists before accessing the world array.
     * real_room() returns NOWHERE (-1) if the vnum doesn't exist.
     * Accessing world[-1] would cause memory corruption! */
    room_rnum room_index = real_room(*rvnum);
    if (room_index == NOWHERE || room_index < 0) {
      log("SYSERR: Attempt to attach event to non-existent room vnum %d!", *rvnum);
      free(rvnum);  /* Clean up the memory we just allocated */
      /* CRITICAL: We must cancel the event that was already created above!
       * Otherwise it will fire with invalid data and could crash the game. */
      event_cancel(pEvent);
      /* Note: event_cancel will free pMudEvent, so we just return */
      return;
    }
    
    /* Now safe to get the room data */
    room = &world[room_index];

    /* log("[DEBUG] Adding Event %s to room %d",mud_event_index[pMudEvent->iId].event_name, room->number); */

    /* Create the event list for this room if it doesn't exist yet */
    if (room->events == NULL)
      room->events = create_list();

    /* Add this event to the room's event list */
    add_to_list(pEvent, room->events);
    break;
  case EVENT_REGION:
    /* CRITICAL FIX: Same memory management as ROOM events.
     * We need to copy the region vnum to our own allocated memory
     * that the event system will manage for the lifetime of the event.
     * 
     * IMPORTANT: Just like with ROOM events, we don't free the original
     * pointer because we don't own it - the caller owns that memory. */
    
    /* Allocate memory for storing the region vnum */
    CREATE(regvnum, region_vnum, 1);
    
    /* Copy the vnum value from the caller's pointer to our memory */
    *regvnum = *((region_vnum *)pMudEvent->pStruct);
    
    /* Update pStruct to point to our newly allocated memory */
    pMudEvent->pStruct = regvnum;

    /* Debug logging for region events - comment out when not debugging */
    /* log("Region event attached: vnum %d (rnum %d) for event %s", 
        *((region_vnum *)pMudEvent->pStruct), real_region(*regvnum), 
        event_name(pMudEvent->iId)); */

    /* SAFETY CHECK: Ensure the region vnum corresponds to a valid region.
     * real_region() returns NOWHERE (-1) if the vnum doesn't exist. */
    if (real_region(*regvnum) == NOWHERE)
    {
      log("SYSERR: Attempt to add event to out-of-range region!");
      /* Clean up the memory we just allocated since we can't use it */
      free(regvnum);
      /* CRITICAL: Cancel the already-created event to prevent it from
       * firing with invalid data. event_cancel will free pMudEvent. */
      event_cancel(pEvent);
      return;
    }

    /* Get the actual region data from the region_table array.
     * We've already verified the index is valid above. */
    region = &region_table[real_region(*regvnum)];

    /* Create the event list for this region if it doesn't exist yet */
    if (region->events == NULL)
      region->events = create_list();

    /* Add this event to the region's event list */
    add_to_list(pEvent, region->events);
    break;
  }
}

struct mud_event_data *new_mud_event(event_id iId, void *pStruct, const char *sVariables)
{
  struct mud_event_data *pMudEvent = NULL;
  char *varString = NULL;

  /* Allocate memory for the mud event data structure */
  CREATE(pMudEvent, struct mud_event_data, 1);
  
  /* If variables are provided, create our own copy of the string.
   * We use strdup() which allocates memory and copies the string.
   * This is important because the caller's string might be temporary. */
  varString = (sVariables != NULL) ? strdup(sVariables) : NULL;

  /* Initialize the event data:
   * - iId: The type of event (from mud_event_list.c)
   * - pStruct: Pointer to the entity (character, room, etc.)
   *   NOTE: For ROOM/REGION events, attach_mud_event() will create
   *   its own copy of this data to prevent memory leaks
   * - sVariables: Our copy of any event-specific data
   * - pEvent: Will be set when event is attached */
  pMudEvent->iId = iId;
  pMudEvent->pStruct = pStruct;
  pMudEvent->sVariables = varString;
  pMudEvent->pEvent = NULL;

  return (pMudEvent);
}

void free_mud_event(struct mud_event_data *pMudEvent)
{
  /* BEGINNER'S GUIDE: This is the cleanup function for MUD events.
   * It's called when an event completes or is cancelled.
   * 
   * CRITICAL RESPONSIBILITY: This function must:
   * 1. Remove the event from the entity's event list
   * 2. Free all dynamically allocated memory
   * 3. Handle special cases for ROOM and REGION events
   * 
   * Memory leaks here will cause the game to eventually crash! */
  
  struct descriptor_data *d = NULL;
  struct char_data *ch = NULL;
  struct room_data *room = NULL;
  struct region_data *region = NULL;
  struct obj_data *obj = NULL;

  room_vnum *rvnum = NULL;
  region_vnum *regvnum = NULL;

  /* Remove event from appropriate list based on entity type.
   * Each entity type (char, room, etc.) maintains its own event list. */
  switch (mud_event_index[pMudEvent->iId].iEvent_Type)
  {
  case EVENT_WORLD:
    remove_from_list(pMudEvent->pEvent, world_events);
    break;
  case EVENT_DESC:
    d = (struct descriptor_data *)pMudEvent->pStruct;
    remove_from_list(pMudEvent->pEvent, d->events);
    break;
  case EVENT_CHAR:
    ch = (struct char_data *)pMudEvent->pStruct;
    remove_from_list(pMudEvent->pEvent, ch->events);

    if (ch->events && ch->events->iSize == 0)
    {
      free_list(ch->events);
      ch->events = NULL;
    }
    break;
  case EVENT_OBJECT:
    obj = (struct obj_data *)pMudEvent->pStruct;
    remove_from_list(pMudEvent->pEvent, obj->events);

    if (obj->events && obj->events->iSize == 0)
    {
      free_list(obj->events);
      obj->events = NULL;
    }
    break;
  case EVENT_ROOM:
    /* CRITICAL SECTION: Room Event Cleanup
     * 
     * PROBLEM: Rooms can be deleted/modified by OLC (online creation) while
     * events are attached to them. We store room VNUMs (virtual numbers)
     * not pointers, because pointers can become invalid.
     * 
     * SOLUTION: Always verify the room still exists before accessing it! */
    
    rvnum = (room_vnum *)pMudEvent->pStruct;

    /* CRITICAL BOUNDS CHECK - THIS PREVENTS CRASHES!
     * Step 1: Save the vnum value BEFORE freeing the memory
     * Step 2: Check if the room still exists in the world
     * Step 3: Free the memory regardless (prevent memory leak)
     * Step 4: Only access the room if it exists
     * 
     * real_room() converts a VNUM to an array index (RNUM).
     * It returns NOWHERE (-1) if the room doesn't exist.
     * Accessing world[-1] would crash the game! */
    room_vnum vnum_copy = *rvnum;  /* Save vnum before freeing */
    room_rnum room_index = real_room(vnum_copy);
    
    /* Always free the allocated memory to prevent leaks */
    free(pMudEvent->pStruct);
    
    /* Safety: Only proceed if room exists */
    if (room_index == NOWHERE || room_index < 0) {
      log("Info: Event for room vnum %d cancelled, but room no longer exists", vnum_copy);
      break;  /* Exit early - can't remove from non-existent room's list */
    }
    
    /* Now safe to access the room via world array */
    room = &world[room_index];

    /* log("[DEBUG] Removing Event %s from room %d, which has %d events.",mud_event_index[pMudEvent->iId].event_name, room->number, (room->events == NULL ? 0 : room->events->iSize)); */

    remove_from_list(pMudEvent->pEvent, room->events);

    if (room->events && room->events->iSize == 0)
    { /* Added the null check here. - Ornir*/
      free_list(room->events);
      room->events = NULL;
    }
    break;
  case EVENT_REGION:
    /* CRITICAL SECTION: Region Event Cleanup
     * 
     * BUG HISTORY: The original code had a USE-AFTER-FREE bug!
     * It freed the memory first, then tried to use the freed memory
     * in a log message. This caused random crashes.
     * 
     * CORRECT ORDER:
     * 1. Get the pointer to the vnum
     * 2. Copy the vnum value to a local variable
     * 3. Free the memory
     * 4. Use the local copy (not the freed memory!)
     * 
     * This is a classic C programming pitfall that beginners often hit! */
    
    regvnum = (region_vnum *)pMudEvent->pStruct;
    
    /* CRITICAL: Copy the vnum BEFORE freeing!
     * After free(), the memory contents are undefined.
     * Accessing freed memory is undefined behavior (UB) in C. */
    region_vnum reg_vnum_copy = *regvnum;
    
    /* Verify region exists. Regions can be reloaded/deleted dynamically.
     * real_region() returns NOWHERE if the region doesn't exist.
     * Accessing region_table[NOWHERE] would be out of bounds! */
    region_rnum rnum = real_region(reg_vnum_copy);
    
    /* NOW safe to free the memory */
    free(pMudEvent->pStruct);
    
    if (rnum != NOWHERE)
    {
      /* Region exists - safe to remove event from its list */
      region = &region_table[rnum];
      remove_from_list(pMudEvent->pEvent, region->events);

      /* Clean up empty event list */
      if (region->events && region->events->iSize == 0)
      {
        free_list(region->events);
        region->events = NULL;
      }
    }
    else
    {
      /* Region no longer exists - this can happen during reload.
       * We use reg_vnum_copy here, which is safe because we copied it
       * before freeing the memory. */
      log("Info: Event for region vnum %d cancelled, but region no longer exists", reg_vnum_copy);
    }
    break;
  }

  if (pMudEvent->sVariables != NULL)
    free(pMudEvent->sVariables);

  pMudEvent->pEvent->event_obj = NULL;
  free(pMudEvent);
}

struct mud_event_data *char_has_mud_event(struct char_data *ch, event_id iId)
{
  struct event *pEvent = NULL;
  struct mud_event_data *pMudEvent = NULL;
  bool found = FALSE;
  struct iterator_data it;

  if (ch->events == NULL)
    return NULL;

  if (ch->events->iSize == 0)
    return NULL;


  for (pEvent = (struct event *)merge_iterator(&it, ch->events);
       pEvent != NULL;
       pEvent = next_in_list(&it))
  {
    if (!pEvent->isMudEvent)
      continue;
    pMudEvent = (struct mud_event_data *)pEvent->event_obj;
    if (pMudEvent->iId == iId)
    {
      found = TRUE;
      break;
    }
  }
  remove_iterator(&it);

  if (found)
    return (pMudEvent);

  return NULL;
}

struct mud_event_data *room_has_mud_event(struct room_data *rm, event_id iId)
{
  struct event *pEvent = NULL;
  struct mud_event_data *pMudEvent = NULL;
  bool found = FALSE;

  if (rm->events == NULL)
    return NULL;

  if (rm->events->iSize == 0)
    return NULL;

  simple_list(NULL);
  while ((pEvent = (struct event *)simple_list(rm->events)) != NULL)
  {
    if (!pEvent->isMudEvent)
      continue;
    pMudEvent = (struct mud_event_data *)pEvent->event_obj;
    if (pMudEvent->iId == iId)
    {
      found = TRUE;
      break;
    }
  }
  simple_list(NULL);

  if (found)
    return (pMudEvent);

  return NULL;
}

struct mud_event_data *obj_has_mud_event(struct obj_data *obj, event_id iId)
{
  struct event *pEvent = NULL;
  struct mud_event_data *pMudEvent = NULL;
  bool found = FALSE;

  if (obj->events == NULL)
    return NULL;

  if (obj->events->iSize == 0)
    return NULL;

  simple_list(NULL);
  while ((pEvent = (struct event *)simple_list(obj->events)) != NULL)
  {
    if (!pEvent->isMudEvent)
      continue;
    pMudEvent = (struct mud_event_data *)pEvent->event_obj;
    if (pMudEvent->iId == iId)
    {
      found = TRUE;
      break;
    }
  }
  simple_list(NULL);

  if (found)
    return (pMudEvent);

  return NULL;
}

struct mud_event_data *region_has_mud_event(struct region_data *reg, event_id iId)
{
  struct event *pEvent = NULL;
  struct mud_event_data *pMudEvent = NULL;
  bool found = FALSE;

  if (reg->events == NULL)
    return NULL;

  if (reg->events->iSize == 0)
    return NULL;

  simple_list(NULL);
  while ((pEvent = (struct event *)simple_list(reg->events)) != NULL)
  {
    if (!pEvent->isMudEvent)
      continue;
    pMudEvent = (struct mud_event_data *)pEvent->event_obj;
    if (pMudEvent->iId == iId)
    {
      found = TRUE;
      break;
    }
  }
  simple_list(NULL);

  if (found)
    return (pMudEvent);

  return NULL;
}

/* BEGINNER'S GUIDE: world_has_mud_event()
 * 
 * This function searches for a global event (one that affects the entire world).
 * World events are stored in the global world_events list.
 * 
 * RETURNS: Pointer to the mud_event_data if found, NULL if not found
 * 
 * IMPORTANT: This was previously a stub that always returned NULL!
 * Now it properly searches the world_events list for the requested event.
 */
struct mud_event_data *world_has_mud_event(event_id iId)
{
  struct event *pEvent = NULL;
  struct mud_event_data *pMudEvent = NULL;
  bool found = FALSE;

  /* Safety check: No world events list means nothing to search */
  if (world_events == NULL)
    return NULL;

  /* Safety check: Empty list means nothing to search */
  if (world_events->iSize == 0)
    return NULL;

  /* Search through all world events for one matching the requested ID.
   * We use simple_list() for safe iteration. */
  simple_list(NULL);  /* Reset the iterator */
  while ((pEvent = (struct event *)simple_list(world_events)) != NULL)
  {
    /* Skip non-MUD events (shouldn't be any in world_events, but be safe) */
    if (!pEvent->isMudEvent)
      continue;
    
    /* Get the MUD event data from this event */
    pMudEvent = (struct mud_event_data *)pEvent->event_obj;
    
    /* Check if this is the event we're looking for */
    if (pMudEvent->iId == iId)
    {
      found = TRUE;
      break;  /* Found it, stop searching */
    }
  }
  simple_list(NULL);  /* Clean up the iterator state */

  /* Return the found event or NULL if not found */
  if (found)
    return (pMudEvent);

  return NULL;
}

void event_cancel_specific(struct char_data *ch, event_id iId)
{
  struct event *pEvent;
  struct mud_event_data *pMudEvent = NULL;
  bool found = FALSE;

  if (ch->events == NULL)
  {
    /* act("ch->events == NULL, for $n.", FALSE, ch, NULL, NULL, TO_ROOM); */
    /* send_to_char(ch, "ch->events == NULL.\r\n"); */
    return;
  }

  if (ch->events->iSize == 0)
  {
    /* act("ch->events->iSize == 0, for $n.", FALSE, ch, NULL, NULL, TO_ROOM); */
    /* send_to_char(ch, "ch->events->iSize == 0.\r\n"); */
    return;
  }

  /* Use simple_list for safer iteration when we're going to modify the list */
  simple_list(NULL); /* Reset the simple list iterator */
  while ((pEvent = (struct event *)simple_list(ch->events)) != NULL)
  {
    if (!pEvent->isMudEvent)
      continue;
    pMudEvent = (struct mud_event_data *)pEvent->event_obj;
    if (pMudEvent->iId == iId)
    {
      found = TRUE;
      break;
    }
  }
  simple_list(NULL); /* Clear the simple list state */

  if (found)
  {
    /* act("event found for $n, attempting to cancel", FALSE, ch, NULL, NULL, TO_ROOM); */
    /* send_to_char(ch, "Event found: %d.\r\n", iId); */
    if (event_is_queued(pEvent))
      event_cancel(pEvent);
  }
  else
  {
    /* act("event_cancel_specific did not find an event for $n.", FALSE, ch, NULL, NULL, TO_ROOM); */
    /* send_to_char(ch, "event_cancel_specific did not find an event.\r\n"); */
  }

  return;
}

void clear_char_event_list(struct char_data *ch)
{
  /* BEGINNER'S GUIDE: This function clears all events from a character.
   * This is typically called when a character dies or leaves the game.
   * 
   * CRITICAL SAFETY: We use a TWO-PASS algorithm to prevent crashes:
   * Pass 1: Collect all events that need to be cancelled
   * Pass 2: Cancel the collected events
   * 
   * Why two passes? If we cancel events while iterating the original list,
   * the cancellation might modify the list we're iterating, causing crashes! */
  
  struct event *pEvent = NULL;
  struct item_data *pItem = NULL;
  struct item_data *pNextItem = NULL;
  struct list_data *temp_list = NULL;

  /* Safety check: If character has no event list, nothing to do */
  if (ch->events == NULL)
    return;

  /* Double-check: Ensure events list exists and has items */
  if (!ch->events || ch->events->iSize == 0)
    return;

  /* Create a temporary list to safely collect events for cancellation.
   * This is our "staging area" - we'll copy references to events here,
   * then cancel them all at once. This avoids the dreaded "modifying
   * a list while iterating it" problem. */
  temp_list = create_list();

  /* PASS 1: Collect all events that need cancelling
   * We iterate through the character's events and copy REFERENCES
   * (not the events themselves) to our temporary list. */
  pItem = ch->events->pFirstItem;
  while (pItem)
  {
    /* CRITICAL: Cache the next pointer BEFORE doing anything!
     * Why? Because if something modifies the list, pItem->pNextItem
     * might become invalid. By caching it now, we're safe. */
    pNextItem = pItem->pNextItem;
    pEvent = (struct event *)pItem->pContent;
    
    /* IMPORTANT EDGE CASE: Death during event execution
     * If a character dies WHILE one of their events is executing,
     * we must NOT cancel the currently-executing event or the game crashes!
     * event_is_queued() returns FALSE for currently-executing events,
     * so we only collect events that are queued (waiting to execute). */
    if (pEvent && event_is_queued(pEvent))
      add_to_list(pEvent, temp_list);
      
    pItem = pNextItem;
  }

  /* PASS 2: Cancel all the collected events
   * Now that we have a stable list of events to cancel,
   * we can safely cancel them without worrying about list corruption. */
  pItem = temp_list->pFirstItem;
  while (pItem)
  {
    /* Again, cache next pointer before cancellation */
    pNextItem = pItem->pNextItem;
    pEvent = (struct event *)pItem->pContent;
    
    if (pEvent)
      event_cancel(pEvent);  /* This will free memory and remove from original list */
      
    pItem = pNextItem;
  }

  /* Clean up our temporary list (the events themselves are already freed) */
  free_list(temp_list);
}

void clear_room_event_list(struct room_data *rm)
{
  /* BEGINNER'S GUIDE: This function clears all events from a room.
   * Rooms can have events like darkness timers, respawn timers, etc.
   * This is called when a room is being deleted or reset.
   * 
   * Like clear_char_event_list, we use a TWO-PASS algorithm for safety. */
  
  struct event *pEvent = NULL;
  struct list_data *temp_list = NULL;

  /* Safety check: No event list means nothing to do */
  if (rm->events == NULL)
    return;

  /* Safety check: Empty list means nothing to do */
  if (rm->events->iSize == 0)
    return;

  /* Create a temporary "staging area" for events to be cancelled.
   * This prevents list corruption during iteration. */
  temp_list = create_list();

  /* PASS 1: Collect all events that need cancelling
   * simple_list() is a special iterator that's safe for list traversal.
   * Calling it with NULL resets the iterator. */
  simple_list(NULL);  /* Reset the iterator */
  while ((pEvent = (struct event *)simple_list(rm->events)) != NULL)
  {
    /* Only collect events that are queued (not currently executing) */
    if (event_is_queued(pEvent))
      add_to_list(pEvent, temp_list);
  }
  simple_list(NULL);  /* Clean up the iterator state */

  /* PASS 2: Cancel all the collected events
   * Now we iterate through our temporary list and cancel each event. */
  simple_list(NULL);  /* Reset for the temp list */
  while ((pEvent = (struct event *)simple_list(temp_list)) != NULL)
  {
    event_cancel(pEvent);  /* Cancel and free the event */
  }
  simple_list(NULL);  /* Clean up the iterator state */

  /* Free our temporary list (events are already freed by event_cancel) */
  free_list(temp_list);
}

void clear_region_event_list(struct region_data *reg)
{
  struct event *pEvent = NULL;

  /* BEGINNER'S GUIDE: This function clears all events from a region.
   * A region is a collection of rooms/areas in the game world.
   * This is called when a region is being deleted or reset.
   * 
   * CRITICAL: We must be very careful here to avoid double-free bugs!
   * A double-free happens when we try to free the same memory twice,
   * which causes the game to crash. */

  /* Safety check: If there's no event list, nothing to do */
  if (reg->events == NULL)
    return;

  /* Safety check: If the list exists but is empty, nothing to do */
  if (reg->events->iSize == 0)
    return;

  /* IMPORTANT ALGORITHM: Process-First-Until-Empty Pattern
   * Instead of iterating through all items (which can cause problems when
   * the list is modified during iteration), we repeatedly process just the
   * FIRST item until the list is empty.
   * 
   * Why this works:
   * 1. We always get the first item from the list
   * 2. When we cancel an event, it removes itself from the list
   * 3. The "next" first item is what was previously the second item
   * 4. We continue until no items remain
   * 
   * This pattern PREVENTS double-free because we never hold pointers
   * to items that might get freed during the cancellation process. */
  while (reg->events && reg->events->iSize > 0)
  {
    /* Always work with the first event in the list.
     * After we process it, it will be removed, and the next event
     * will become the new first event. */
    pEvent = (struct event *)reg->events->pFirstItem->pContent;
    
    /* Check if this event is valid and queued (scheduled to run) */
    if (pEvent && event_is_queued(pEvent))
    {
      /* Cancel the event. This does THREE important things:
       * 1. Stops the event from executing
       * 2. Calls free_mud_event() to clean up memory
       * 3. Removes the event from reg->events list
       * 
       * After this call, pEvent is no longer valid! */
      event_cancel(pEvent);
    }
    else
    {
      /* Event exists but is not queued (maybe already executed?) */
      if (pEvent)
      {
        /* Just remove it from the list without canceling */
        remove_from_list(pEvent, reg->events);
      }
      else
      {
        /* CORRUPTION HANDLING: The list item exists but has no content.
         * This shouldn't happen, but we handle it gracefully to prevent crashes.
         * We manually remove the corrupted item from the linked list. */
        struct item_data *pItem = reg->events->pFirstItem;
        
        /* Update the list's first item pointer to skip the corrupted item */
        reg->events->pFirstItem = pItem->pNextItem;
        
        /* Fix the backward link of the new first item (if it exists) */
        if (reg->events->pFirstItem)
          reg->events->pFirstItem->pPrevItem = NULL;
        else
          reg->events->pLastItem = NULL;  /* List is now empty */
        
        /* Update the list size counter */
        reg->events->iSize--;
        
        /* Free the corrupted list item structure */
        free(pItem);
      }
    }
  }
  
  /* Final cleanup: If the list still exists but is empty, free it.
   * This saves memory and ensures reg->events is NULL when no events exist. */
  if (reg->events && reg->events->iSize == 0)
  {
    free_list(reg->events);
    reg->events = NULL;
  }
}

/* ripley's version of change_event_duration
 * a function to adjust the event time of a given event
 */
void change_event_duration(struct char_data *ch, event_id iId, long time)
{
  struct event *pEvent = NULL;
  struct mud_event_data *pMudEvent = NULL;
  struct mud_event_data *pNewMudEvent = NULL;
  bool found = FALSE;
  char *sVarCopy = NULL;

  /* Safety check: Ensure events list exists and has items */
  if (!ch->events || ch->events->iSize == 0)
    return;

  simple_list(NULL);
  while ((pEvent = (struct event *)simple_list(ch->events)) != NULL)
  {

    if (!pEvent->isMudEvent)
      continue;

    pMudEvent = (struct mud_event_data *)pEvent->event_obj;

    if (pMudEvent->iId == iId)
    {
      found = TRUE;
      /* Make a copy of the variables before we cancel the event */
      if (pMudEvent->sVariables)
        sVarCopy = strdup(pMudEvent->sVariables);
      break;
    }
  }
  simple_list(NULL);

  if (found)
  {
    /* Create the new event first */
    pNewMudEvent = new_mud_event(iId, ch, sVarCopy);
    
    /* Cancel the old event */
    if (event_is_queued(pEvent))
      event_cancel(pEvent);
    
    /* Now attach the new event */
    attach_mud_event(pNewMudEvent, time);
    
    /* Free our temporary copy */
    if (sVarCopy)
      free(sVarCopy);
  }
}

/* zusuk: change an event's svariables value */
void change_event_svariables(struct char_data *ch, event_id iId, char *sVariables)
{
  struct event *pEvent = NULL;
  struct mud_event_data *pMudEvent = NULL;
  struct mud_event_data *pNewMudEvent = NULL;
  bool found = FALSE;
  long time = 0;

  /* Safety check: Ensure events list exists and has items */
  if (!ch->events || ch->events->iSize == 0)
    return;

  simple_list(NULL);
  while ((pEvent = (struct event *)simple_list(ch->events)) != NULL)
  {

    if (!pEvent->isMudEvent)
      continue;

    pMudEvent = (struct mud_event_data *)pEvent->event_obj;

    if (pMudEvent->iId == iId)
    {
      time = event_time(pMudEvent->pEvent);
      found = TRUE;
      break;
    }
  }
  simple_list(NULL);

  if (found)
  {
    /* Create the new event first */
    pNewMudEvent = new_mud_event(iId, ch, sVariables);
    
    /* Cancel the old event */
    if (event_is_queued(pEvent))
      event_cancel(pEvent);
    
    /* Now attach the new event */
    attach_mud_event(pNewMudEvent, time);
  }
}
