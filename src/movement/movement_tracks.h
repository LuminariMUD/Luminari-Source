/**************************************************************************
 *  File: movement_tracks.h                            Part of LuminariMUD *
 *  Usage: Header file for trail and tracking system.                     *
 *  Author: Ornir (original trails.h)                                     *
 *                                                                         *
 *  All rights reserved.  See license for complete information.           *
 *                                                                         *
 *  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 **************************************************************************/

#ifndef _MOVEMENT_TRACKS_H_
#define _MOVEMENT_TRACKS_H_

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>

/* Trail pruning threshold - 1 in-game week */
#define TRAIL_PRUNING_THRESHOLD 12600

/* Retain only the freshest distinct trail signatures in one room. */
#define TRAIL_MAX_PER_ROOM 16

/* Track type definitions */
#define TRACKS_UNDEFINED 0
#define TRACKS_IN 1
#define TRACKS_OUT 2
#define DIR_NONE -1

/* Trail data structures */
struct trail_data
{
  struct trail_data *next;
  struct trail_data *prev;

  char *name;
  char *race;

  int from;
  int to;
  time_t age;
};

struct trail_data_list
{
  struct trail_data *head;
  struct trail_data *tail;
};

/* Function prototypes for trail/tracking system */

/**
 * Create tracks in the current room
 * @param ch Character creating the tracks
 * @param dir Direction of movement
 * @param flag TRACKS_IN, TRACKS_OUT, or TRACKS_UNDEFINED
 */
void create_tracks(struct char_data *ch, int dir, int flag);

/**
 * Refresh or add one bounded movement-trail signature.
 *
 * Repeated movement by the same visible identity in the same direction
 * refreshes one record instead of allocating an unbounded duplicate.
 */
void movement_trail_record(struct trail_data_list *list, const char *name, const char *race,
                           int from, int to, time_t age);

/**
 * Record or query trails at one stable gameplay location.
 *
 * Ordinary locations use the room vnum. Wilderness locations use the zone
 * vnum and coordinates because dynamic wilderness room vnums are recyclable
 * allocator slots rather than place identities.
 */
void movement_trail_record_at_room(const struct room_data *room, const char *name,
                                   const char *race, int from, int to, time_t age);
const struct trail_data_list *movement_trails_at_room(const struct room_data *room);

typedef bool (*movement_trail_visitor)(struct trail_data *trail, void *context);
bool movement_trail_visit_all(movement_trail_visitor visitor, void *context);

/**
 * Clean up old trails in rooms in the active trail registry.
 * Called periodically from heartbeat or events
 */
void cleanup_all_trails(void);
void movement_trail_registry_forget(room_vnum vnum);
void movement_trail_registry_shutdown(void);
size_t movement_trail_active_location_count(void);
size_t movement_trail_registry_validate(void);
uint64_t movement_trail_cleanup_runs(void);
uint64_t movement_trail_locations_visited(void);
uint64_t movement_trail_entries_removed(void);
size_t movement_trail_last_cleanup_locations_visited(void);

/**
 * Count movement trails currently retained by the stable-location registry.
 * @return Number of live trail_data entries
 */
size_t count_live_movement_trails(void);

/**
 * Check if tracks should be created for this character
 * @param ch Character to check
 * @return TRUE if tracks should be created, FALSE otherwise
 */
bool should_create_tracks(struct char_data *ch);

/**
 * Create movement tracks for a character (wrapper function)
 * @param ch Character creating tracks
 * @param dir Direction of movement
 * @param track_type TRACKS_IN or TRACKS_OUT
 */
void create_movement_tracks(struct char_data *ch, int dir, int track_type);

#endif /* _MOVEMENT_TRACKS_H_ */
