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
#include "movement_tracks.h" /* includes trail data structures */

static void movement_trail_free(struct trail_data *trail)
{
  if (trail == NULL)
  {
    return;
  }

  free(trail->name);
  free(trail->race);
  free(trail);
}

static void movement_trail_unlink(struct trail_data_list *list,
                                  struct trail_data *trail)
{
  if (list == NULL || trail == NULL)
  {
    return;
  }

  if (trail->prev != NULL)
  {
    trail->prev->next = trail->next;
  }
  else
  {
    list->head = trail->next;
  }

  if (trail->next != NULL)
  {
    trail->next->prev = trail->prev;
  }
  else
  {
    list->tail = trail->prev;
  }
}

static void movement_trail_prepend(struct trail_data_list *list,
                                   struct trail_data *trail)
{
  trail->prev = NULL;
  trail->next = list->head;
  if (list->head != NULL)
  {
    list->head->prev = trail;
  }
  else
  {
    list->tail = trail;
  }
  list->head = trail;
}

/**
 * Retain one freshest record for each visible trail signature.
 *
 * The legacy trail list has no reader that benefits from duplicate nodes.
 * Refreshing a match preserves its age while the per-room cap prevents
 * ordinary mobile wandering from retaining process-lifetime heap growth.
 */
void movement_trail_record(struct trail_data_list *list, const char *name,
                           const char *race, int from, int to, time_t age)
{
  struct trail_data *trail;
  struct trail_data *match;
  int count;

  if (list == NULL)
  {
    return;
  }

  if (name == NULL)
  {
    name = "unknown";
  }
  if (race == NULL)
  {
    race = "unknown";
  }

  match = NULL;
  count = 0;
  for (trail = list->head; trail != NULL; trail = trail->next)
  {
    count++;
    if (match == NULL && trail->from == from && trail->to == to &&
        !strcmp(trail->name ? trail->name : "", name) &&
        !strcmp(trail->race ? trail->race : "", race))
    {
      match = trail;
    }
  }

  if (match != NULL)
  {
    match->age = age;
    if (match != list->head)
    {
      movement_trail_unlink(list, match);
      movement_trail_prepend(list, match);
    }
    return;
  }

  while (count >= TRAIL_MAX_PER_ROOM && list->tail != NULL)
  {
    trail = list->tail;
    movement_trail_unlink(list, trail);
    movement_trail_free(trail);
    count--;
  }

  CREATE(trail, struct trail_data, 1);
  trail->name = strdup(name);
  trail->race = strdup(race);
  trail->from = from;
  trail->to = to;
  trail->age = age;
  movement_trail_prepend(list, trail);
}

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
  struct trail_data *next = NULL;
  const char *name;
  const char *race;
  time_t now;
  int race_idx;

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
  if (!room->trail_tracks)
  {
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
  now = time(NULL);
  for (cur = room->trail_tracks->head; cur != NULL;)
  {
    next = cur->next;
    if (now - cur->age >= TRAIL_PRUNING_THRESHOLD)
    {
      movement_trail_unlink(room->trail_tracks, cur);
      movement_trail_free(cur);
    }
    /* Always advance to next, whether we removed the current node or not */
    cur = next;
  }

  name = GET_NAME(ch) ? GET_NAME(ch) : "unknown";
  race = "unknown";
  if (IS_NPC(ch))
  {
    race_idx = GET_NPC_RACE(ch);
    if (race_idx >= 0 && race_idx <= NUM_RACE_TYPES && race_family_types[race_idx])
      race = race_family_types[race_idx];
  }
  else
  {
    race_idx = GET_RACE(ch);
    if (race_idx >= 0 && race_idx < NUM_RACES && race_list[race_idx].name)
      race = race_list[race_idx].name;
  }

  movement_trail_record(room->trail_tracks, name, race,
                        flag == TRACKS_IN ? dir : DIR_NONE,
                        flag == TRACKS_OUT ? dir : DIR_NONE, now);
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
      log("SYSERR: Room %d has corrupted trail_tracks->head pointer (%p), clearing trails", room,
          world[room].trail_tracks->head);
      world[room].trail_tracks->head = NULL;
      world[room].trail_tracks->tail = NULL;
      continue;
    }

    for (cur = world[room].trail_tracks->head; cur != NULL;)
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
 * Count movement trails currently retained by all world rooms.
 *
 * This scan is intended for infrequent staff/runtime checkpoints. Keeping the
 * count derived from the lists avoids a second mutable counter that can drift
 * when rooms are replaced or destroyed.
 */
size_t count_live_movement_trails(void)
{
  room_rnum room;
  struct trail_data *trail;
  size_t trail_count = 0;

  if (world == NULL)
    return 0;

  for (room = 0; room <= top_of_world; room++)
  {
    if (world[room].trail_tracks == NULL)
      continue;

    for (trail = world[room].trail_tracks->head; trail != NULL; trail = trail->next)
      trail_count++;
  }

  return trail_count;
}

/**
 * Check if tracks should be created for this character
 *
 * @param ch Character to check
 * @return TRUE if tracks should be created, FALSE otherwise
 */
bool should_create_tracks(struct char_data *ch)
{
  /* The retained trail lists describe player footprints. Awake-world NPC
   * wandering can otherwise allocate a new named record in every room it
   * crosses even when no player is present to leave a trail. */
  if (ch == NULL || IS_NPC(ch))
    return FALSE;

  /* Don't create tracks for immortals with nohassle */
  if (PRF_FLAGGED(ch, PRF_NOHASSLE))
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
