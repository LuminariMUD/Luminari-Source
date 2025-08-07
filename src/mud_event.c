/**************************************************************************
 *  File: mud_event.c                                  Part of LuminariMUD *
 *  Usage: Handling of the mud event system                                *
 *                                                                         *
 *  By Vatiken. Copyright 2012 by Joseph Arnusch                           *
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
    rvnum = (room_vnum *)pMudEvent->pStruct;
    rnum = real_room(*rvnum);
    /* room = &world[real_room(rnum)]; */ /* Unused assignment */
    break;
  case EVENT_REGION:
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
      cooldown = (SECS_PER_MUD_DAY / nonfeat_daily_uses) RL_SEC;
    }
    else if (get_daily_uses(ch, featnum))
    { /* divide by 0! */
      cooldown = (SECS_PER_MUD_DAY / get_daily_uses(ch, featnum)) RL_SEC;
    }
  }

  return cooldown;
}

/*
 * Update: Events have been added to objects, rooms and characters.
 *         Region support has also been added for wilderness regions.
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

  pEvent = event_create(mud_event_index[pMudEvent->iId].func, pMudEvent, time);
  pEvent->isMudEvent = TRUE;
  pMudEvent->pEvent = pEvent;

  /* Add event to appropriate list based on entity type */
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

    CREATE(rvnum, room_vnum, 1);
    *rvnum = *((room_vnum *)pMudEvent->pStruct);
    pMudEvent->pStruct = rvnum;
    room = &world[real_room(*rvnum)];

    /* log("[DEBUG] Adding Event %s to room %d",mud_event_index[pMudEvent->iId].event_name, room->number); */

    if (room->events == NULL)
      room->events = create_list();

    add_to_list(pEvent, room->events);
    break;
  case EVENT_REGION:
    CREATE(regvnum, region_vnum, 1);
    *regvnum = *((region_vnum *)pMudEvent->pStruct);
    pMudEvent->pStruct = regvnum;

    /* Debug logging for region events - comment out when not debugging */
    /* log("Region event attached: vnum %d (rnum %d) for event %s", 
        *((region_vnum *)pMudEvent->pStruct), real_region(*regvnum), 
        event_name(pMudEvent->iId)); */

    if (real_region(*regvnum) == NOWHERE)
    {
      log("SYSERR: Attempt to add event to out-of-range region!");
      free(regvnum);
      break;
    }

    region = &region_table[real_region(*regvnum)];

    if (region->events == NULL)
      region->events = create_list();

    add_to_list(pEvent, region->events);
    break;
  }
}

struct mud_event_data *new_mud_event(event_id iId, void *pStruct, const char *sVariables)
{
  struct mud_event_data *pMudEvent = NULL;
  char *varString = NULL;

  CREATE(pMudEvent, struct mud_event_data, 1);
  varString = (sVariables != NULL) ? strdup(sVariables) : NULL;

  pMudEvent->iId = iId;
  pMudEvent->pStruct = pStruct;
  pMudEvent->sVariables = varString;
  pMudEvent->pEvent = NULL;

  return (pMudEvent);
}

void free_mud_event(struct mud_event_data *pMudEvent)
{
  struct descriptor_data *d = NULL;
  struct char_data *ch = NULL;
  struct room_data *room = NULL;
  struct region_data *region = NULL;
  struct obj_data *obj = NULL;

  room_vnum *rvnum = NULL;
  region_vnum *regvnum = NULL;

  /* Remove event from appropriate list based on entity type */
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
    /* Due to OLC changes, if rooms were deleted then the room we have in the event might be
     * invalid.  This entire system needs to be re-evaluated!  We should really use RNUM
     * and just get the room data ourselves.  Storing the room_data struct is asking for bad
     * news. */
    rvnum = (room_vnum *)pMudEvent->pStruct;

    room = &world[real_room(*rvnum)];

    /* log("[DEBUG] Removing Event %s from room %d, which has %d events.",mud_event_index[pMudEvent->iId].event_name, room->number, (room->events == NULL ? 0 : room->events->iSize)); */

    free(pMudEvent->pStruct);

    remove_from_list(pMudEvent->pEvent, room->events);

    if (room->events && room->events->iSize == 0)
    { /* Added the null check here. - Ornir*/
      free_list(room->events);
      room->events = NULL;
    }
    break;
  case EVENT_REGION:
    regvnum = (region_vnum *)pMudEvent->pStruct;
    
    /* CRITICAL: Check if region still exists before accessing region_table.
     * During reload, regions may be removed or reordered, causing real_region
     * to return NOWHERE (-1), which would cause memory corruption. */
    region_rnum rnum = real_region(*regvnum);
    
    free(pMudEvent->pStruct);
    
    if (rnum != NOWHERE)
    {
      region = &region_table[rnum];
      remove_from_list(pMudEvent->pEvent, region->events);

      if (region->events && region->events->iSize == 0)
      { /* Added the null check here. - Ornir*/
        free_list(region->events);
        region->events = NULL;
      }
    }
    else
    {
      /* Region no longer exists - this can happen during reload.
       * Just log it for debugging purposes. */
      log("INFO: Event for region vnum %d cancelled, but region no longer exists", *regvnum);
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

/* remove world event */
struct mud_event_data *world_has_mud_event(event_id iId)
{
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
  struct event *pEvent = NULL;
  struct item_data *pItem = NULL;
  struct item_data *pNextItem = NULL;
  struct list_data *temp_list = NULL;

  if (ch->events == NULL)
    return;

  if (ch->events->iSize == 0)
    return;

  /* Create a temporary list to collect events for cancellation.
   * This avoids modifying the list while iterating. */
  temp_list = create_list();

  /* First pass: collect all events that need cancelling using safe iteration */
  pItem = ch->events->pFirstItem;
  while (pItem)
  {
    pNextItem = pItem->pNextItem;  /* Cache next pointer before any modifications */
    pEvent = (struct event *)pItem->pContent;
    
    /* Here we have an issue - If we are currently executing an event, and it results in a char
     * having their events cleared (death) then we must be sure that we don't clear the executing
     * event!  Doing so will crash the event system. */
    if (pEvent && event_is_queued(pEvent))
      add_to_list(pEvent, temp_list);
      
    pItem = pNextItem;
  }

  /* Second pass: cancel the collected events using safe iteration */
  pItem = temp_list->pFirstItem;
  while (pItem)
  {
    pNextItem = pItem->pNextItem;  /* Cache next pointer before event_cancel */
    pEvent = (struct event *)pItem->pContent;
    
    if (pEvent)
      event_cancel(pEvent);
      
    pItem = pNextItem;
  }

  /* Clean up the temporary list */
  free_list(temp_list);
}

void clear_room_event_list(struct room_data *rm)
{
  struct event *pEvent = NULL;
  struct list_data *temp_list = NULL;

  if (rm->events == NULL)
    return;

  if (rm->events->iSize == 0)
    return;

  /* Create a temporary list to collect events for cancellation.
   * This avoids modifying the list while iterating. */
  temp_list = create_list();

  /* First pass: collect all events that need cancelling */
  simple_list(NULL);
  while ((pEvent = (struct event *)simple_list(rm->events)) != NULL)
  {
    if (event_is_queued(pEvent))
      add_to_list(pEvent, temp_list);
  }
  simple_list(NULL);

  /* Second pass: cancel the collected events */
  simple_list(NULL);
  while ((pEvent = (struct event *)simple_list(temp_list)) != NULL)
  {
    event_cancel(pEvent);
  }
  simple_list(NULL);

  /* Clean up the temporary list */
  free_list(temp_list);
}

void clear_region_event_list(struct region_data *reg)
{
  struct event *pEvent = NULL;

  if (reg->events == NULL)
    return;

  if (reg->events->iSize == 0)
    return;

  /* Cancel all events. Each cancellation will remove the event from reg->events,
   * so we keep taking the first item until the list is empty.
   * This avoids double-free issues that can occur when using a temp_list. */
  while (reg->events && reg->events->iSize > 0)
  {
    /* Get the first event from the list */
    pEvent = (struct event *)reg->events->pFirstItem->pContent;
    
    if (pEvent && event_is_queued(pEvent))
    {
      /* event_cancel will call free_mud_event which removes the event from reg->events */
      event_cancel(pEvent);
    }
    else
    {
      /* If event is not queued, we still need to remove it from the list */
      if (pEvent)
      {
        remove_from_list(pEvent, reg->events);
      }
      else
      {
        /* Corrupted list item - remove it manually */
        struct item_data *pItem = reg->events->pFirstItem;
        reg->events->pFirstItem = pItem->pNextItem;
        if (reg->events->pFirstItem)
          reg->events->pFirstItem->pPrevItem = NULL;
        else
          reg->events->pLastItem = NULL;
        reg->events->iSize--;
        free(pItem);
      }
    }
  }
  
  /* Clean up the empty list if it still exists */
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

  if (ch->events->iSize == 0)
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

  if (ch->events->iSize == 0)
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
