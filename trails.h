/* *************************************************************************
 *   File: trails.h                                    Part of LuminariMUD *
 *  Usage: Header file for trails (Scent, Foot, Blood, Magic, etc.)        *
 * Author: Ornir                                                           *
 ***************************************************************************
 *                                                                         *
 ***************************************************************************/
#pragma once
#include <time.h>

struct trail_data {
    struct trail_data *next;
    struct trail_data *prev;
    
    char *name;
    // race
    
    int from;
    int to;
    time_t when;
}
