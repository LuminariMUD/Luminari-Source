/*
 * File:   bardic_performance.h
 * Author: Zusuk
 * Constants and function prototypes for the bardic performance system.
 */

#ifndef BARDIC_PERFORMANCE_H
#define BARDIC_PERFORMANCE_H

#ifdef __cplusplus
extern "C"
{
#endif
/*********************************************************/
/* includes */
#include "utils.h" /* for the ACMD macro */

    /* functions */
    extern struct room_data *world;
    extern void clearMemory(struct char_data *ch);
    extern const char *spells[];
    ACMD_DECL(do_perform);

/* defines */
#define VERSE_INTERVAL (11 RL_SEC)
#define MAX_PERFORMANCES 12
#define MAX_PRFM_EFFECT 60 /* maximum effectiveness of performance */
#define MAX_INSTRUMENT_EFFECT 20

/* lookup components for song_info */
#define PERFORMANCE_SKILLNUM 0
#define INSTRUMENT_NUM 1
#define INSTRUMENT_SKILLNUM 2
#define PERFORMANCE_DIFF 3
#define PERFORMANCE_TYPE 4
#define PERFORMANCE_AOE 5
#define PERFORMANCE_FEATNUM 6
    /**/ #define PERFORMANCE_INFO_FIELDS 7

/* types of performances */
#define PERFORMANCE_TYPE_UNDEFINED 0
#define PERFORMANCE_TYPE_ACT 1
#define PERFORMANCE_TYPE_COMEDY 2
#define PERFORMANCE_TYPE_DANCE 3
#define PERFORMANCE_TYPE_KEYBOARD 4
#define PERFORMANCE_TYPE_ORATORY 5
#define PERFORMANCE_TYPE_PERCUSSION 6
#define PERFORMANCE_TYPE_STRING 7
#define PERFORMANCE_TYPE_WIND 8
#define PERFORMANCE_TYPE_SING 9
        /**/ #define NUM_PERFORMANCE_TYPES 10

/* these are defines just made for fillers for lookup data song_info
 since they are currently unused in our feat system, could be expanded tho */
#define SKILL_LYRE 1
#define SKILL_DRUM 2
#define SKILL_HORN 3
#define SKILL_FLUTE 4
#define SKILL_HARP 5
#define SKILL_MANDOLIN 6

/* area of effect */
#define PERFORM_AOE_UNDEFINED 0
#define PERFORM_AOE_GROUP 1
#define PERFORM_AOE_ROOM 2
#define PERFORM_AOE_FOES 3
/**/
#define NUM_PERFORM_AOE 4

/*********************************************************/
#ifdef __cplusplus
}
#endif

#endif /* BARDIC_PERFORMANCE_H */
