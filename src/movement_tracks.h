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

#include <time.h>

/* Trail pruning threshold - 1 in-game week */
#define TRAIL_PRUNING_THRESHOLD 12600

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
 * Clean up old trails in all rooms
 * Called periodically from heartbeat or events
 */
void cleanup_all_trails(void);

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