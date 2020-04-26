/*/ \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \
\     Staff Ran Event System
/  File:       staff_events.c
/  Created By: Zusuk
\  Header:     staff_events.h
/    System for running staff events
\    Basics including starting, ending and info on the event
/  Created on April 26, 2020
\ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ /*/

/* includes */
#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "staff_events.h"
/* end includes */

/****************/
/** start code **/
/****************/

/* array with the data about the event, probably just change this to a structure later */
const char *staff_events_list[NUM_STAFF_EVENTS][STAFF_EVENT_FIELDS] = {

    {/*JACKALOPE_HUNT*/

     /* title - EVENT_TITLE */
     "\tCHardbuckler Jackalope Hunt\tn",

     /* event start message - EVENT_BEGIN */
     "\tWThe horn has sounded, the great Jackalope Hunt of the Hardbuckler Region has begun!\tn",

     /* event end message - EVENT_END */
     "\tRThe horn has sounded, the great Jackalope Hunt of the Hardbuckler Region has ended!\tn",

     /* event info message - EVENT_DETAIL */
     "\tgIt is as I feared, Jackalopes have been seen in numbers roaming the countryside.  "
     "Usually they reproduce very slowly.  Clearly someone has been breeding them, and "
     "this, can mean no good.  We must stop the spread of this growing menace now.  Will "
     "you help me?\tn\r\n \tW- Fullstaff, Agent of Sanctus -\tn\r\n",

     /* event summary/conclusion - EVENT_SUMMARY */
     "\tgGreat job cleaning up dood!\tn\r\n \tW- Fullstaff, Agent of Sanctus -\tn\r\n",

     /*end jackalope hunt*/},

};

/* start a staff event! */
void start_staff_event(int event_num)
{
    struct descriptor_data *pt = NULL;

    /* dummy checks */
    if (event_num >= NUM_STAFF_EVENTS || event_num < 0)
    {
        return;
    }
    /* announcement to game */
    for (pt = descriptor_list; pt; pt = pt->next)
    {
        if (IS_PLAYING(pt) && pt->character)
        {
            send_to_char(pt->character, "\tR[\tWInfo\tR]\tn A Staff Ran Event (\tn%s\tn) is Starting!\r\n",
                         staff_events_list[event_num][EVENT_TITLE]);
            send_to_char(pt->character, "\tn%s\tn\r\n",
                         staff_events_list[event_num][EVENT_DETAIL]);
            send_to_char(pt->character, "\r\n\tn%s\tn\r\n\r\n",
                         staff_events_list[event_num][EVENT_BEGIN]);
        }
    }

    switch (event_num)
    {

    case JACKALOPE_HUNT:
        break;

    default:
        break;
    }

    return;
}

/* end a staff event! */
void end_staff_event(int event_num)
{
    struct descriptor_data *pt = NULL;

    /* dummy checks */
    if (event_num >= NUM_STAFF_EVENTS || event_num < 0)
    {
        return;
    }
    /* announcement to game */
    for (pt = descriptor_list; pt; pt = pt->next)
    {
        if (IS_PLAYING(pt) && pt->character)
        {
            send_to_char(pt->character, "\tR[\tWInfo\tR]\tn A Staff Ran Event (\tn%s\tn) has ended.\r\n",
                         staff_events_list[event_num][EVENT_TITLE]);
            send_to_char(pt->character, "\tn%s\tn\r\n",
                         staff_events_list[event_num][EVENT_SUMMARY]);
            send_to_char(pt->character, "\r\n\tn%s\tn\r\n\r\n",
                         staff_events_list[event_num][EVENT_END]);
        }
    }

    switch (event_num)
    {

    case JACKALOPE_HUNT:
        break;

    default:
        break;
    }

    return;
}

/* details about a specific event */
void staff_event_info(struct char_data *ch, int event_num)
{
    int event_field = 0;

    /* dummy checks */
    if (!ch || event_num >= NUM_STAFF_EVENTS || event_num < 0)
    {
        return;
    }

    send_to_char(ch, "\r\n\tgDetails about \tn%s\tn (%d)\tg:\tn\r\n",
                 staff_events_list[event_num][EVENT_TITLE], event_num);

    for (event_field = 0; event_field < STAFF_EVENT_FIELDS; event_field++)
    {
        switch (event_field)
        {

        case EVENT_BEGIN:
            send_to_char(ch, "Event begin message to world: \tn%s\tn\r\n", staff_events_list[event_num][EVENT_BEGIN]);
            break;

        case EVENT_END:
            send_to_char(ch, "Event end message to world: \tn%s\tn\r\n", staff_events_list[event_num][EVENT_END]);
            break;

        case EVENT_DETAIL:
            send_to_char(ch, "Event info:\r\n\tn");
            send_to_char(ch, staff_events_list[event_num][EVENT_DETAIL]);
            send_to_char(ch, "\tn");
            break;

        case EVENT_TITLE: /* we mention this above */
        default:
            break;
        }
    }

    send_to_char(ch, "Usage: staffevents [start|end|info] [index # above]\r\n\r\n");

    return;
}

/* list the events */
void list_staff_events(struct char_data *ch)
{
    int i = 0;

    send_to_char(ch, "\r\n\tCA Listing of Staff Ran Events:\tn\r\n");

    for (i = 0; i < NUM_STAFF_EVENTS; i++)
    {
        send_to_char(ch, "\tc%d)\tn %s\r\n", i, staff_events_list[i][0]);
    }

    send_to_char(ch, "Usage: staffevents [start|end|info] [index # above]\r\n\r\n");

    return;
}

/* command to start/end/list staff events */
ACMD(do_staffevents)
{
    char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
    int event_num = -1;

    half_chop(argument, arg, arg2);

    if (!*arg || !*arg2)
    {
        list_staff_events(ch);
        return;
    }

    if (isdigit(*arg2))
    {
        event_num = atoi(arg2);
    }
    else
    {
        list_staff_events(ch);
        send_to_char(ch, "Requires a digit (the event number) for the second argument!\r\n");
        return;
    }

    /* not a valid event number */
    if (event_num >= NUM_STAFF_EVENTS || event_num < 0)
    {
        list_staff_events(ch);
        send_to_char(ch, "Invalid event #!\r\n");
        return;
    }

    if (is_abbrev(arg, "start"))
    {
        start_staff_event(event_num);
    }
    else if (is_abbrev(arg, "end"))
    {
        end_staff_event(event_num);
    }
    else if (is_abbrev(arg, "info"))
    {
        staff_event_info(ch, event_num);
    }
    else
    {
        list_staff_events(ch);
        return;
    }

    return;
}

/* undefines */

#undef NUM_STAFF_EVENTS
#undef STAFF_EVENT_FIELDS

#undef EVENT_TITLE
#undef EVENT_BEGIN
#undef EVENT_END
#undef EVENT_DETAIL

/* jackalope hunt undefines */
#undef JACKALOPE_HUNT
#undef EASY_JACKALOPE       /* vnum of lower level jackalope */
#undef MED_JACKALOPE        /* vnum of mid level jackalope */
#undef HARD_JACKALOPE       /* vnum of high level jackalope */
#undef SMALL_JACKALOPE_HIDE /* vnum of lower level jackalope's hide */
#undef MED_JACKALOPE_HIDE   /* vnum of mid level jackalope's hide */
#undef LARGE_JACKALOPE_HIDE /* vnum of high level jackalope's hide */
#undef PRISTINE_HORN        /* vnum of rare pristine jackalope horn */
#undef P_HORN_RARITY        /* % chance of loading pristine jackalope horn */

/* end jackalope hunt dundefines */

/* EOF */