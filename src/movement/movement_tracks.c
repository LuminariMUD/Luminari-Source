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
#include "magic/spells.h"
#include "constants.h"
#include <stdint.h>
#include "act.h"
#include "character/class.h"
#include "character/race.h"
#include "wilderness/wilderness.h"
/* Include movement system header */
#include "movement.h"
#include "movement_tracks.h" /* includes trail data structures */

enum movement_trail_location_kind
{
  TRAIL_LOCATION_ROOM = 0,
  TRAIL_LOCATION_WILDERNESS
};

struct movement_trail_location
{
  enum movement_trail_location_kind kind;
  room_vnum room_vnum;
  zone_vnum zone_vnum;
  int x;
  int y;
  struct trail_data_list trails;
  struct movement_trail_location *next;
};

#define MOVEMENT_TRAIL_LOCATION_BUCKETS 1024U

static struct movement_trail_location
    *movement_trail_location_buckets[MOVEMENT_TRAIL_LOCATION_BUCKETS];
static size_t movement_trail_location_count;
static uint64_t trail_cleanup_runs;
static uint64_t trail_locations_visited;
static uint64_t trail_entries_removed;
static size_t trail_last_cleanup_locations_visited;

static void movement_trail_free(struct trail_data *trail);

static bool movement_trail_room_is_wilderness(const struct room_data *room)
{
  if (room == NULL)
    return false;
  if (IS_WILDERNESS_VNUM(room->number))
    return true;
  return zone_table != NULL && room->zone != NOWHERE && room->zone <= top_of_zone_table &&
         ZONE_FLAGGED(room->zone, ZONE_WILDERNESS);
}

static zone_vnum movement_trail_wilderness_zone(const struct room_data *room)
{
  if (room != NULL && zone_table != NULL && room->zone != NOWHERE &&
      room->zone <= top_of_zone_table)
    return zone_table[room->zone].number;
  return WILD_ZONE_VNUM;
}

static uint32_t movement_trail_hash_mix(uint32_t value)
{
  value ^= value >> 16;
  value *= 0x7feb352dU;
  value ^= value >> 15;
  value *= 0x846ca68bU;
  return value ^ (value >> 16);
}

static size_t movement_trail_location_bucket(enum movement_trail_location_kind kind,
                                             room_vnum room_vnum, zone_vnum zone_vnum, int x,
                                             int y)
{
  uint32_t hash = movement_trail_hash_mix((uint32_t)kind + 1U);

  if (kind == TRAIL_LOCATION_ROOM || room_vnum != NOWHERE)
    hash ^= movement_trail_hash_mix((uint32_t)room_vnum);
  else
  {
    hash ^= movement_trail_hash_mix((uint32_t)zone_vnum);
    hash ^= movement_trail_hash_mix((uint32_t)x + 0x9e3779b9U);
    hash ^= movement_trail_hash_mix((uint32_t)y + 0x85ebca6bU);
  }
  return hash % MOVEMENT_TRAIL_LOCATION_BUCKETS;
}

static bool movement_trail_location_matches(const struct movement_trail_location *location,
                                            enum movement_trail_location_kind kind,
                                            room_vnum room_vnum, zone_vnum zone_vnum, int x,
                                            int y)
{
  if (location->kind != kind)
    return false;
  if (kind == TRAIL_LOCATION_ROOM)
    return location->room_vnum == room_vnum;
  if (location->room_vnum != room_vnum)
    return false;
  if (room_vnum != NOWHERE)
    return true;
  return location->zone_vnum == zone_vnum && location->x == x && location->y == y;
}

static void movement_trail_list_clear(struct trail_data_list *list)
{
  struct trail_data *trail;
  struct trail_data *next;

  if (list == NULL)
    return;
  for (trail = list->head; trail != NULL; trail = next)
  {
    next = trail->next;
    movement_trail_free(trail);
  }
  list->head = NULL;
  list->tail = NULL;
}

static struct movement_trail_location *
movement_trail_location_for_room(const struct room_data *room, bool create)
{
  struct movement_trail_location **head;
  struct movement_trail_location *location;
  enum movement_trail_location_kind kind;
  bool coordinates_set;
  room_vnum room_vnum;
  zone_vnum zone_vnum;
  int x;
  int y;

  if (room == NULL || room->number == NOWHERE)
    return NULL;
  kind = movement_trail_room_is_wilderness(room) ? TRAIL_LOCATION_WILDERNESS
                                                  : TRAIL_LOCATION_ROOM;
  coordinates_set = kind == TRAIL_LOCATION_WILDERNESS &&
                    (IS_WILDERNESS_VNUM(room->number) || room->wilderness_coordinates_set);
  room_vnum = kind == TRAIL_LOCATION_ROOM || !coordinates_set ? room->number : NOWHERE;
  zone_vnum = kind == TRAIL_LOCATION_WILDERNESS ? movement_trail_wilderness_zone(room) : NOWHERE;
  x = coordinates_set ? room->coords[X_COORD] : 0;
  y = coordinates_set ? room->coords[Y_COORD] : 0;
  head = &movement_trail_location_buckets[
      movement_trail_location_bucket(kind, room_vnum, zone_vnum, x, y)];
  for (location = *head; location != NULL; location = location->next)
    if (movement_trail_location_matches(location, kind, room_vnum, zone_vnum, x, y))
      return location;
  if (!create)
    return NULL;

  CREATE(location, struct movement_trail_location, 1);
  location->kind = kind;
  location->room_vnum = room_vnum;
  location->zone_vnum = zone_vnum;
  location->x = x;
  location->y = y;
  location->next = *head;
  *head = location;
  movement_trail_location_count++;
  return location;
}

void movement_trail_registry_forget(room_vnum vnum)
{
  struct movement_trail_location **link;
  struct movement_trail_location *location;
  enum movement_trail_location_kind kind;
  size_t bucket;

  if (vnum == NOWHERE || IS_WILDERNESS_VNUM(vnum))
    return;
  for (kind = TRAIL_LOCATION_ROOM; kind <= TRAIL_LOCATION_WILDERNESS; kind++)
  {
    bucket = movement_trail_location_bucket(kind, vnum, NOWHERE, 0, 0);
    for (link = &movement_trail_location_buckets[bucket]; (location = *link) != NULL;
         link = &location->next)
    {
      if (!movement_trail_location_matches(location, kind, vnum, NOWHERE, 0, 0))
        continue;
      *link = location->next;
      movement_trail_list_clear(&location->trails);
      free(location);
      if (movement_trail_location_count > 0U)
        movement_trail_location_count--;
      return;
    }
  }
}

void movement_trail_registry_shutdown(void)
{
  struct movement_trail_location *location;
  struct movement_trail_location *next;
  size_t bucket;

  for (bucket = 0U; bucket < MOVEMENT_TRAIL_LOCATION_BUCKETS; bucket++)
  {
    for (location = movement_trail_location_buckets[bucket]; location != NULL; location = next)
    {
      next = location->next;
      movement_trail_list_clear(&location->trails);
      free(location);
    }
    movement_trail_location_buckets[bucket] = NULL;
  }
  movement_trail_location_count = 0U;
  trail_cleanup_runs = 0U;
  trail_locations_visited = 0U;
  trail_entries_removed = 0U;
  trail_last_cleanup_locations_visited = 0U;
}

size_t movement_trail_active_location_count(void) { return movement_trail_location_count; }
uint64_t movement_trail_cleanup_runs(void) { return trail_cleanup_runs; }
uint64_t movement_trail_locations_visited(void) { return trail_locations_visited; }
uint64_t movement_trail_entries_removed(void) { return trail_entries_removed; }
size_t movement_trail_last_cleanup_locations_visited(void)
{
  return trail_last_cleanup_locations_visited;
}

size_t movement_trail_registry_validate(void)
{
  struct movement_trail_location *location;
  struct movement_trail_location *other;
  struct trail_data *trail;
  size_t actual = 0U;
  size_t invalid = 0U;
  size_t bucket;

  for (bucket = 0U; bucket < MOVEMENT_TRAIL_LOCATION_BUCKETS; bucket++)
    for (location = movement_trail_location_buckets[bucket]; location != NULL;
         location = location->next)
    {
      actual++;
      if (location->trails.head == NULL || location->trails.tail == NULL ||
          location->trails.head->prev != NULL || location->trails.tail->next != NULL)
        invalid++;
      for (trail = location->trails.head; trail != NULL; trail = trail->next)
        if ((trail->next != NULL && trail->next->prev != trail) ||
            (trail->next == NULL && location->trails.tail != trail))
          invalid++;
      for (other = location->next; other != NULL; other = other->next)
        if (movement_trail_location_matches(other, location->kind, location->room_vnum,
                                            location->zone_vnum, location->x, location->y))
          invalid++;
    }
  if (actual == movement_trail_location_count && invalid == 0U)
    return 0U;
  return invalid + (actual > movement_trail_location_count
                        ? actual - movement_trail_location_count
                        : movement_trail_location_count - actual);
}

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

static void movement_trail_unlink(struct trail_data_list *list, struct trail_data *trail)
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

static void movement_trail_prepend(struct trail_data_list *list, struct trail_data *trail)
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
void movement_trail_record(struct trail_data_list *list, const char *name, const char *race,
                           int from, int to, time_t age)
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

void movement_trail_record_at_room(const struct room_data *room, const char *name,
                                   const char *race, int from, int to, time_t age)
{
  struct movement_trail_location *location;
  struct trail_data *trail;
  struct trail_data *next;

  location = movement_trail_location_for_room(room, true);
  if (location == NULL)
    return;
  for (trail = location->trails.head; trail != NULL; trail = next)
  {
    next = trail->next;
    if (age - trail->age >= TRAIL_PRUNING_THRESHOLD)
    {
      movement_trail_unlink(&location->trails, trail);
      movement_trail_free(trail);
      trail_entries_removed++;
    }
  }
  movement_trail_record(&location->trails, name, race, from, to, age);
}

const struct trail_data_list *movement_trails_at_room(const struct room_data *room)
{
  struct movement_trail_location *location = movement_trail_location_for_room(room, false);

  return location != NULL ? &location->trails : NULL;
}

bool movement_trail_visit_all(movement_trail_visitor visitor, void *context)
{
  struct movement_trail_location *location;
  struct trail_data *trail;
  size_t bucket;

  if (visitor == NULL)
    return false;
  for (bucket = 0U; bucket < MOVEMENT_TRAIL_LOCATION_BUCKETS; bucket++)
    for (location = movement_trail_location_buckets[bucket]; location != NULL;
         location = location->next)
      for (trail = location->trails.head; trail != NULL; trail = trail->next)
        if (!visitor(trail, context))
          return false;
  return true;
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

  now = time(NULL);
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
    if (race_idx >= 0 && race_idx < NUM_EXTENDED_RACES && race_list[race_idx].name)
      race = race_list[race_idx].name;
  }

  movement_trail_record_at_room(room, name, race, flag == TRACKS_IN ? dir : DIR_NONE,
                                flag == TRACKS_OUT ? dir : DIR_NONE, now);
}

/**
 * Clean up old trails in rooms that currently contain trail data.
 * This is typically called from the heartbeat or a periodic event
 */
void cleanup_all_trails(void)
{
  struct movement_trail_location **link;
  struct movement_trail_location *location;
  struct trail_data *cur, *next;
  time_t current_time = time(NULL);
  int cleaned = 0;
  size_t bucket;

  trail_cleanup_runs++;
  trail_last_cleanup_locations_visited = 0U;
  for (bucket = 0U; bucket < MOVEMENT_TRAIL_LOCATION_BUCKETS; bucket++)
  {
    for (link = &movement_trail_location_buckets[bucket]; (location = *link) != NULL;)
    {
      trail_last_cleanup_locations_visited++;
      trail_locations_visited++;
      for (cur = location->trails.head; cur != NULL;)
      {
        next = cur->next;
        if (current_time - cur->age >= TRAIL_PRUNING_THRESHOLD)
        {
          movement_trail_unlink(&location->trails, cur);
          movement_trail_free(cur);
          cleaned++;
          cur = next;
        }
        else
        {
          /* Node was not removed, advance to next node */
          cur = next;
        }
      }
      if (location->trails.head == NULL)
      {
        *link = location->next;
        free(location);
        if (movement_trail_location_count > 0U)
          movement_trail_location_count--;
        continue;
      }
      link = &location->next;
    }
  }

  trail_entries_removed += (uint64_t)cleaned;
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
  struct movement_trail_location *location;
  struct trail_data *trail;
  size_t trail_count = 0;
  size_t bucket;

  for (bucket = 0U; bucket < MOVEMENT_TRAIL_LOCATION_BUCKETS; bucket++)
    for (location = movement_trail_location_buckets[bucket]; location != NULL;
         location = location->next)
      for (trail = location->trails.head; trail != NULL; trail = trail->next)
        trail_count++;

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

  return TRUE;
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
