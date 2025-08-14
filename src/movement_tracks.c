/**************************************************************************
 *  File: movement_tracks.c                            Part of LuminariMUD *
 *  Usage: Trail and tracking system for movement.                        *
 *                                                                         *
 *  All rights reserved.  See license for complete information.           *
 *                                                                         *
 *  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 **************************************************************************/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "spells.h"
#include "constants.h"
#include <stdint.h>
#include "act.h"
#include "class.h"
#include "race.h"
/* Include movement system header */
#include "movement.h"
#include "movement_tracks.h"  /* includes trail data structures */

/**
 * Create tracks in the current room
 * 
 * @param ch Character creating the tracks
 * @param dir Direction of movement
 * @param flag TRACKS_IN, TRACKS_OUT, or TRACKS_UNDEFINED
 */
void create_tracks(struct char_data *ch, int dir, int flag)
{
  struct room_data *room = NULL;
  struct trail_data *cur = NULL;
  struct trail_data *new_trail = NULL;

  if (IN_ROOM(ch) != NOWHERE)
  {
    room = &world[ch->in_room];
  }
  else
  {
    log("SYSERR: Char at location NOWHERE trying to create tracks.");
    return;
  }

  /* Safety check for trail_tracks */
  if (!room->trail_tracks) {
    log("SYSERR: Room %d has NULL trail_tracks, initializing.", room->number);
    CREATE(room->trail_tracks, struct trail_data_list, 1);
    room->trail_tracks->head = NULL;
    room->trail_tracks->tail = NULL;
  }

  /*
    Here we create the track structure, set the values and assign it to the room.
    At the same time, we can prune off any really old trails.  Threshold is set,
    in seconds, in trails.h.  Eventually this can be adjusted based on weather -
    rain/snow/wind can all obscure trails.
  */

  /* First, prune old trails from the list BEFORE adding new ones to avoid corruption */
  for (cur = room->trail_tracks->head; cur != NULL; )
  {
    struct trail_data *next = cur->next;
    if (time(NULL) - cur->age >= TRAIL_PRUNING_THRESHOLD)
    {
      /* Free the trail data */
      if (cur->name)
        free(cur->name);
      if (cur->race)
        free(cur->race);
      
      /* Unlink from list */
      if (cur->prev != NULL)
      {
        cur->prev->next = cur->next;
      }
      else
      {
        room->trail_tracks->head = cur->next;
      }
      
      if (cur->next != NULL)
      {
        cur->next->prev = cur->prev;
      }
      else
      {
        room->trail_tracks->tail = cur->prev;
      }
      
      /* Free the structure */
      free(cur);
    }
    /* Always advance to next, whether we removed the current node or not */
    cur = next;
  }

  /* Now create and add the new trail to the cleaned list */
  CREATE(new_trail, struct trail_data, 1);
  
  /* Safely set name with NULL check */
  if (GET_NAME(ch))
    new_trail->name = strdup(GET_NAME(ch));
  else
    new_trail->name = strdup("unknown");
  
  /* Safely set race with bounds and NULL checks */
  if (IS_NPC(ch))
  {
    int race_idx = GET_NPC_RACE(ch);
    if (race_idx >= 0 && race_idx <= NUM_RACE_TYPES && race_family_types[race_idx])
      new_trail->race = strdup(race_family_types[race_idx]);
    else
      new_trail->race = strdup("unknown");
  }
  else
  {
    int race_idx = GET_RACE(ch);
    if (race_idx >= 0 && race_idx < NUM_RACES && race_list[race_idx].name)
      new_trail->race = strdup(race_list[race_idx].name);
    else  
      new_trail->race = strdup("unknown");
  }
  
  new_trail->from = (flag == TRACKS_IN ? dir : DIR_NONE);
  new_trail->to = (flag == TRACKS_OUT ? dir : DIR_NONE);
  new_trail->age = time(NULL);

  /* Insert at head of list with proper NULL checks */
  new_trail->next = room->trail_tracks->head;
  new_trail->prev = NULL;

  if (room->trail_tracks->head != NULL)
  {
    /* Only access head->prev if head is not NULL */
    room->trail_tracks->head->prev = new_trail;
  }
  else
  {
    /* If this is the first node, set tail as well */
    room->trail_tracks->tail = new_trail;
  }

  room->trail_tracks->head = new_trail;
}

/**
 * Clean up old trails in all rooms - called periodically
 * This is typically called from the heartbeat or a periodic event
 */
void cleanup_all_trails(void)
{
  room_rnum room;
  struct trail_data *cur, *next;
  time_t current_time = time(NULL);
  int cleaned = 0;
  
  for (room = 0; room <= top_of_world; room++)
  {
    if (!world[room].trail_tracks || !world[room].trail_tracks->head)
      continue;
    
    /* Validate the head pointer is in a reasonable memory range */
    if ((uintptr_t)world[room].trail_tracks->head < 0x1000 || 
        (uintptr_t)world[room].trail_tracks->head > (uintptr_t)-0x1000)
    {
      log("SYSERR: Room %d has corrupted trail_tracks->head pointer (%p), clearing trails", 
          room, world[room].trail_tracks->head);
      world[room].trail_tracks->head = NULL;
      world[room].trail_tracks->tail = NULL;
      continue;
    }
      
    for (cur = world[room].trail_tracks->head; cur != NULL; )
    {
      /* Validate cur pointer before dereferencing */
      if ((uintptr_t)cur < 0x1000 || (uintptr_t)cur > (uintptr_t)-0x1000)
      {
        log("SYSERR: Room %d has corrupted trail node pointer (%p), stopping cleanup", room, cur);
        /* Clear the entire list as it's corrupted */
        world[room].trail_tracks->head = NULL;
        world[room].trail_tracks->tail = NULL;
        break;
      }
      
      next = cur->next;
      if (current_time - cur->age >= TRAIL_PRUNING_THRESHOLD)
      {
        /* Free the trail data */
        if (cur->name)
          free(cur->name);
        if (cur->race)
          free(cur->race);
        
        /* Unlink from list using doubly-linked list operations */
        if (cur->prev != NULL)
        {
          cur->prev->next = cur->next;
        }
        else
        {
          world[room].trail_tracks->head = cur->next;
        }
        
        if (cur->next != NULL)
        {
          cur->next->prev = cur->prev;
        }
        else
        {
          world[room].trail_tracks->tail = cur->prev;
        }
        
        /* Free the structure */
        free(cur);
        cleaned++;
        /* Node was removed, use next which was saved before freeing */
        cur = next;
      }
      else
      {
        /* Node was not removed, advance to next node */
        cur = next;
      }
    }
  }
  
  if (cleaned > 0)
    log("Trail cleanup: Removed %d old trail entries.", cleaned);
}

/**
 * Check if tracks should be created for this character
 * 
 * @param ch Character to check
 * @return TRUE if tracks should be created, FALSE otherwise
 */
bool should_create_tracks(struct char_data *ch)
{
  /* Don't create tracks for immortals with nohassle */
  if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_NOHASSLE))
    return FALSE;
  
  /* Don't create tracks if riding (mount creates tracks instead) */
  if (RIDING(ch))
    return FALSE;
  
  /* Don't create tracks in rooms flagged as NOTRACK */
  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_NOTRACK))
    return FALSE;
  
  /* Check campaign settings */
#if defined(CAMPAIGN_DL) || defined(CAMPAIGN_FR)
  /* Tracks disabled for DragonLance and Forgotten Realms campaigns */
  return FALSE;
#else
  /* Tracks enabled for default LuminariMUD campaign */
  return TRUE;
#endif
}

/**
 * Create movement tracks for a character
 * This is a wrapper that checks if tracks should be created
 * 
 * @param ch Character creating tracks
 * @param dir Direction of movement
 * @param track_type TRACKS_IN or TRACKS_OUT
 */
void create_movement_tracks(struct char_data *ch, int dir, int track_type)
{
  if (should_create_tracks(ch))
  {
    create_tracks(ch, dir, track_type);
  }
}