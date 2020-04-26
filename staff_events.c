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
#include "staff_events.h"
/* end includes */

const char *staff_events_list[NUM_STAFF_EVENTS][STAFF_EVENT_FIELDS] = {

    {/*JACKALOPE_HUNT*/
     /* title */
     "Hardbuckler Jackalope Hunt",
     /* event start message */
     "The horn has sounded, the great Jackalope Hunt of the Hardbuckler Region has begun!",
     /* event end message */
     "The horn has sounded, the great Jackalope Hunt of the Hardbuckler Region has ended!",
     /* event info message */
     "It is as I feared, Jackalopes have been seen in numbers roaming the countryside.  "
     "Usually they reproduce very slowly.  Clearly someone has been breeding them, and "
     "this, can mean no good.  We must stop the spread of this growing menace now.  Will "
     "you help me?\r\n - Fullstaff, Agent of Sanctus -"},

};

/* start a staff event! */
void start_staff_event(int event_num)
{
    struct descriptor_data *pt;

    switch (event_num)
    {

    case JACKALOPE_HUNT:
        for (pt = descriptor_list; pt; pt = pt->next)
        {
            if (IS_PLAYING(pt) && pt->character)
            {
                send_to_char(pt->character, "\tR[\tWInfo\tR]\tn %s of %s's group has defeated %s!\r\n",
                             GET_NAME(killer), GET_NAME(killer->group->leader), GET_NAME(ch));
            }
        }
        break;

    default:
        break;
    }

    return;
}

/* end a staff event! */
void end_staff_event(int event_num)
{
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
void staff_event_info(int event_num)
{
    int event_field = 0;

    send_to_char(ch, "\r\n\tgDetails about \tW%s\tg:\tn\r\n", staff_events_list[event_num][EVENT_TITLE]);

    for (event_field = 1; event_field < STAFF_EVENT_FIELDS; event_field++)
    {
        switch (event_field)
        {

        case EVENT_BEGIN:
            send_to_char(ch, "Event begin message to world: \tW%s\tn\r\n", staff_events_list[event_num][EVENT_BEGIN]);
            break;

        case EVENT_END:
            send_to_char(ch, "Event end message to world: \tW%s\tn\r\n", staff_events_list[event_num][EVENT_END]);
            break;

        case EVENT_DETAIL:
            send_to_char(ch, "Event info:\r\n");
            send_to_char(ch, staff_events_list[event_num][EVENT_DETAIL]);
            send_to_char(ch, "\r\n");
            break;

        default: /* should not get here */
            break;
        }

        send_to_char(ch, "%d) %s\r\n", event_field, staff_events_list[event_field][0]);
    }

    send_to_char(ch, "Usage: staffevent [start|end|info] [index # above]\r\n\r\n");

    return;
}

/* list the events */
void list_staff_events()
{
    int i = 0;

    send_to_char(ch, "\r\nA Listing of Staff Ran Events:\r\n");

    for (i = 0; i < NUM_STAFF_EVENTS; i++)
    {
        send_to_char(ch, "%d) %s\r\n", i, staff_events_list[i][0]);
    }

    send_to_char(ch, "Usage: staffevent [start|end|info] [index # above]\r\n\r\n");

    return;
}

/* command to start/end/list staff events */
ACMD(do_staffevent)
{
    char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
    int event_num = -1;

    half_chop(argument, arg, arg2);

    if (!*arg || !*arg2)
    {
        list_staff_events();
        return;
    }

    if (isdigit(*arg2))
    {
        event_num = atoi(arg2);
    }
    else
    {
        send_to_char(ch, "Requires a digit for the second argument!\r\n");
        list_staff_events();
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
        staff_event_info(event_num);
    }
    else
    {
        list_staff_events();
        return;
    }

    return;
}

#undef NUM_STAFF_EVENTS
#undef STAFF_EVENT_FIELDS

#undef EVENT_TITLE
#undef EVENT_BEGIN
#undef EVENT_END
#undef EVENT_DETAIL

/* jackalope hunt defines */
#undef JACKALOPE_HUNT
#undef EASY_JACKALOPE       /* vnum of lower level jackalope */
#undef MED_JACKALOPE        /* vnum of mid level jackalope */
#undef HARD_JACKALOPE       /* vnum of high level jackalope */
#undef SMALL_JACKALOPE_HIDE /* vnum of lower level jackalope's hide */
#undef MED_JACKALOPE_HIDE   /* vnum of mid level jackalope's hide */
#undef LARGE_JACKALOPE_HIDE /* vnum of high level jackalope's hide */
#undef PRISTINE_HORN        /* vnum of rare pristine jackalope horn */
#undef P_HORN_RARITY        /* % chance of loading pristine jackalope horn */
/* end jackalope hunt defines */

/* end staff event code */
