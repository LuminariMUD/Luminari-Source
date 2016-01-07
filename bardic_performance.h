/*
 * File:   bardic_performance.h
 * Author: Zusuk
 * Constants and function prototypes for the bardic performance system.
 *
 */

#ifndef BARDIC_PERFORMANCE_H
#define	BARDIC_PERFORMANCE_H

#ifdef	__cplusplus
extern "C" {
#endif
/*********************************************************/
/* includes */
#include "utils.h" /* for the ACMD macro */

/* defines */
#define VERSE_INTERVAL       (7 RL_SEC)
#define MAX_PERFORMANCES     12
/* lookup components for song_info */    
#define PERFORMANCE_SKILLNUM             0
#define INSTRUMENT_NUM                   1
#define INSTRUMENT_SKILLNUM              2
#define PERFORMANCE_DIFF                 3
#define PERFORMANCE_TYPE                 4
/*last*/#define PERFORMANCE_INFO_FIELDS  5
/* types of performances */
#define PERFORMANCE_TYPE_UNDEFINED      0
#define PERFORMANCE_TYPE_ACT            1
#define PERFORMANCE_TYPE_COMEDY         2
#define PERFORMANCE_TYPE_DANCE          3
#define PERFORMANCE_TYPE_KEYBOARD       4
#define PERFORMANCE_TYPE_ORATORY        5
#define PERFORMANCE_TYPE_PERCUSSION     6
#define PERFORMANCE_TYPE_STRING         7
#define PERFORMANCE_TYPE_WIND           8
#define PERFORMANCE_TYPE_SING           9
/*last*/#define NUM_PERFORMANCE_TYPES  10
/* these are defines just made for fillers for lookup data song_info
 since they are currently unused in our feat system, could be expanded tho */
#define SKILL_LYRE     1
#define SKILL_DRUM     2
#define SKILL_HORN     3
#define SKILL_FLUTE    4
#define SKILL_HARP     5
#define SKILL_MANDOLIN 6

/* functions */
extern struct room_data *world;
extern void clearMemory(struct char_data * ch);
extern const char *spells[];
ACMD(do_play);

/* structs */
struct song_event_obj {
  struct char_data *ch;
  int song;
};

/*********************************************************/
#ifdef	__cplusplus
}
#endif

#endif	/* BARDIC_PERFORMANCE_H */

