/* *************************************************************************
 *   File: trails.h                                    Part of LuminariMUD *
 *  Usage: Header file for trails (Scent, Foot, Blood, Magic, etc.)        *
 * Author: Ornir                                                           *
 ***************************************************************************
 *                                                                         *
 ***************************************************************************/
#pragma once
#include <time.h>

#define TRAIL_PRUNING_THRESHOLD 12600 /* 1 in-game week. */

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