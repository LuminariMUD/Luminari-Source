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
    
/* defines */
#define VERSE_INTERVAL       (7 RL_SEC)
#define MAX_PERFORMANCES     12
    
#define PERFORMANCE_SKILLNUM 0
#define INSTRUMENT_NUM       1
#define INSTRUMENT_SKILLNUM  2

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

