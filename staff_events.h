/*/ \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \
\     Staff Ran Event System
/  File:       staff_events.h
/  Created By: Zusuk
\  Main:     staff_events.c
/    System for running staff events
\    Basics including starting, ending and info on the event, etc
/  Created on April 26, 2020
\ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ /*/

/* includes */
#include "utils.h" /* for the ACMD macro */
/* end includes */

/***/
/******* defines */
#define NUM_STAFF_EVENTS 1

/* event fields */
#define EVENT_TITLE 0
#define EVENT_BEGIN 1
#define EVENT_END 2
#define EVENT_DETAIL 3
#define EVENT_SUMMARY 4
#define STAFF_EVENT_FIELDS 5

/* jackalope hunt defines */
#define JACKALOPE_HUNT 0
#define EASY_JACKALOPE 0       /* vnum of lower level jackalope */
#define MED_JACKALOPE 0        /* vnum of mid level jackalope */
#define HARD_JACKALOPE 0       /* vnum of high level jackalope */
#define SMALL_JACKALOPE_HIDE 0 /* vnum of lower level jackalope's hide */
#define MED_JACKALOPE_HIDE 0   /* vnum of mid level jackalope's hide */
#define LARGE_JACKALOPE_HIDE 0 /* vnum of high level jackalope's hide */
#define PRISTINE_HORN 0        /* vnum of rare pristine jackalope horn */
#define P_HORN_RARITY 0        /* % chance of loading pristine jackalope horn */
/* end jackalope hunt defines */

/******* end defines */
/***/

/* staff events functions, arrays, etc */
const char *staff_events_list[NUM_STAFF_EVENTS][STAFF_EVENT_FIELDS];

void start_staff_event(int event_num);
void end_staff_event(int event_num);
void staff_event_info(struct char_data *ch, int event_num);
void list_staff_events(struct char_data *ch);

ACMD(do_staffevent);

/* EOF */
