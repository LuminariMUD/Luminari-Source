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
#include "handler.h"
#include "screen.h"
#include "wilderness.h"
#include "dg_scripts.h"
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
     "you help me?\tn\r\n \tW- Fullstaff, Agent of Sanctus -\tn\r\n"
     "\r\n\tR[OOC: Head to the Hardbuckler Region and hunt Jackalope in the wilderness.  "
     "There are 3 levels of Jackalope, a set for level 10 and under, 20 and under, and then "
     "epic levels.  There is no reward for killing Jackalope under your level bracket.  The "
     "Jackalope will frequently return to random locations in that area.  "
     "The carcasses will be counted by staff, please reach out and get your "
     "final count within 24 hours of event completion.  Grand prize(s) will be handed out shortly after final count."
     "Rare antlers found are to be "
     "turned into Fullstaff in Hardbuckler for a special bonus prize.]\tn\r\n",

     /* event summary/conclusion - EVENT_SUMMARY */
     "\tgThank you hunter, I.... nay the world owes you a debt of gratitude. I hope we have "
     "quelled this menace for good. However, keep your blades sharp, and your bowstrings tight, "
     "in case they are needed again.\tn\r\n \tW- Fullstaff, Agent of Sanctus -\tn\r\n"
     "\tR[OOC: You can visit Fullstaff in the Hardbuckler Inn to turn in the rare antlers for "
     "a special prize!  The carcasses will be counted by staff, please contact and get your "
     "final count within 24 hours of event completion.  Grand prize(s) will be handed out shortly after final count.]\tn\r\n",

     /*end jackalope hunt*/},

    {/*THE_PRISONER_EVENT*/

     /* title - EVENT_TITLE */
     "\tCThe Prisoner\tn",

     /* event start message - EVENT_BEGIN */
     "\tWExistence shudders as The Prisoner's captivity begins to crack!!!\tn",

     /* event end message - EVENT_END */
     "\tRBy immeasurable sacrifice, aggression, strategy and luck the mystical cell containing The Prisoner holds firm!!!\tn",

     /* event info message - EVENT_DETAIL */
     "\tGThrough the darkling hoarde's tampering, The Prisoner's cell has been compromised.  \tn"
     "\tGAs you know, The Prisoner can't be defeated while in any of our realities.  But the five-headed dragon avatar\tn "
     "\tGthat the Luminari imprisoned in Avernus is the entity that is creating the damage to The Prisoner's cell.\tn\r\n  "
     "\tGGet to the Mosswood Elder adventurer and step through the portal, we MUST mount an offensive or all is lost!\tn\r\n "
     "\tWOOC: While this event is running, the treasure drop is maximized for The Prisoner, there is no XP loss for death and there is a \tn"
     "\tWdirect portal to the Garden of Avernus at the Mosswood Elder.\tn\r\n",

     /* event summary/conclusion - EVENT_SUMMARY */
     "\tGThank you adventuer, I.... nay the entire existence owes you a debt of gratitude. I hope we have "
     "quelled this menace for good. However, keep your blades sharp, and your bowstrings tight, "
     "in case they are needed again.\tn\r\n \tW- Alerion -\tn\r\n",

     /*end the prisoner */},
};

/* drops from staff event mobiles */
void check_event_drops(struct char_data *killer, struct char_data *victim)
{
  /* get some dummy checks out of the way */
  /*
  if (IS_NPC(killer))
      return;
  */
  if (!IS_NPC(victim))
    return;
  if (!IS_STAFF_EVENT)
    return;

  struct obj_data *obj = NULL;
  bool load_drop = FALSE;
  char buf[MAX_STRING_LENGTH] = {'\0'};

  switch (STAFF_EVENT_NUM)
  {

  case JACKALOPE_HUNT:

    switch (GET_MOB_VNUM(victim))
    {

    case EASY_JACKALOPE:
      if (GET_LEVEL(killer) <= 10)
      {
        load_drop = TRUE;
      }
      else
      {
        send_to_char(killer, "OOC:  Lower level Jackalope hides won't drop for someone over level 10.\r\n");
      }

      break;

    case MED_JACKALOPE:
      if (GET_LEVEL(killer) <= 20)
      {
        load_drop = TRUE;
      }
      else
      {
        send_to_char(killer, "OOC:  Mid level Jackalopes hides won't drop for someone over level 20.\r\n");
      }
      break;

    case HARD_JACKALOPE:
      load_drop = TRUE;
      break;

    /* fallthrough */
    default:
      break;
    } /* end mob vnum switch */

    /* should it be loaded? ok load up the object */
    if (load_drop)
    {
      if ((obj = read_object(JACKALOPE_HIDE, VIRTUAL)) == NULL)
      {
        log("SYSERR: check_event_drops() created NULL object for jackalope hide");
        return;
      }
      obj_to_char(obj, killer); // deliver object
      if (killer && obj && obj->short_description)
      {
        send_to_char(killer, "\tYYou have found \tn%s\tn\tY!\tn\r\n", obj->short_description);

        snprintf(buf, MAX_STRING_LENGTH, "$n \tYhas found \tn%s\tn\tY!\tn", obj->short_description);
        act(buf, FALSE, killer, 0, killer, TO_NOTVICT);
      }

      /* checking for bonus? */
      if (dice(1, 100) < P_HORN_RARITY)
      {
        if ((obj = read_object(PRISTINE_HORN, VIRTUAL)) == NULL)
        {
          log("SYSERR: check_event_drops() created NULL object for pristine horn");
          return;
        }
        obj_to_char(obj, killer); // deliver object
        buf[MAX_STRING_LENGTH];
        if (killer && obj && obj->short_description)
        {
          send_to_char(killer, "\tYYou have found \tn%s\tn\tY!\tn\r\n", obj->short_description);

          snprintf(buf, MAX_STRING_LENGTH, "$n \tYhas found \tn%s\tn\tY!\tn", obj->short_description);
          act(buf, FALSE, killer, 0, killer, TO_NOTVICT);
        }
      }
    }

    break;
    /* end jackalope case */

  default:
    break;

  } /* end staff event switch */

  return;
}

/* find the given mobile by vnum and clear it out of the game */
void mob_ingame_purge(int mobile_vnum)
{
  struct char_data *l = NULL;
  mob_rnum mobile_rnum = real_mobile(mobile_vnum);
  mob_rnum i = 0;

  if (!top_of_mobt)
    return;

  if (mobile_rnum == NOTHING)
    return;

  /* "i" will be the real-number */
  for (i = 0; i <= top_of_mobt; i++)
  {
    /* this the mob? */
    if (mobile_rnum == i)
    {
      /* find how many of the same mobiles are in the game currently */
      for (l = character_list; l; l = l->next)
      {
        if (IS_NPC(l) && GET_MOB_RNUM(l) == i)
        {
          if (IN_ROOM(l) == NOWHERE)
          {
            /* this is to prevent crash */
          }
          else
          {
            extract_char(l);
          }
        }
      }
    }
  } /* end for */

  return;
}

/* count the # of mobiles of given vnum in the game */
int mob_ingame_count(int mobile_vnum)
{
  struct char_data *l = NULL;
  mob_rnum mobile_rnum = real_mobile(mobile_vnum);
  mob_rnum i = 0;
  int num_found = 0;

  if (!top_of_mobt)
    return 0;

  if (mobile_rnum == NOTHING)
    return 0;

  /* "i" will be the real-number */
  for (i = 0; i <= top_of_mobt; i++)
  {

    /* this the mob? */
    if (mobile_rnum == i)
    {
      /* find how many of the same mobiles are in the game currently */
      for (num_found = 0, l = character_list; l; l = l->next)
      {
        if (IS_NPC(l) && GET_MOB_RNUM(l) == i)
        {
          num_found++;
        }
      }
    }

  } /* end for */
  return num_found;
}

/* load and place mobile into the wilderness */
/* todo use wilderness-regions to make this more dynamic */
void wild_mobile_loader(int mobile_vnum, int x_coord, int y_coord)
{
  room_rnum location = NOWHERE;
  struct char_data *mob = NULL;

  mob = read_mobile(mobile_vnum, VIRTUAL);

  /* dummy check! */
  if (!mob)
    return;

  if ((location = find_room_by_coordinates(x_coord, y_coord)) == NOWHERE)
  {
    if ((location = find_available_wilderness_room()) == NOWHERE)
    {
      return; /* we failed, should not get here if things are working correctly */
    }
    else
    {
      /* Must set the coords, etc in the going_to room. */
      assign_wilderness_room(location, x_coord, y_coord);
    }
  }

  X_LOC(mob) = world[location].coords[0];
  Y_LOC(mob) = world[location].coords[1];
  char_to_room(mob, location);

  load_mtrigger(mob);

  return;
}

/* used to check/run things frequenly related to the event; called by limits.c point_update() */
void staff_event_tick()
{
  int x_coord = 0;
  int y_coord = 0;
  int mob_count = 0;
  struct descriptor_data *pt = NULL;
  struct obj_data *obj = NULL;
  bool found = FALSE;

  /*********/
  /* staff event related updates */

  if (!IS_STAFF_EVENT && STAFF_EVENT_DELAY > 0)
  {
    STAFF_EVENT_DELAY--;
  }

  if (STAFF_EVENT_TIME > 1)
  {
    STAFF_EVENT_TIME--;

    /* if we want things to happen at given ticks for the event, we do it here! */
    switch (STAFF_EVENT_NUM)
    {

    case JACKALOPE_HUNT:

      /* place some more jackalope if needed! */
      if (mob_ingame_count(EASY_JACKALOPE) < NUM_JACKALOPE_EACH)
      {
        for (mob_count = 0; mob_count < (NUM_JACKALOPE_EACH - mob_ingame_count(EASY_JACKALOPE)); mob_count++)
        {
          x_coord = rand_number(JACKALOPE_WEST_X, JACKALOPE_EAST_X);
          y_coord = rand_number(JACKALOPE_SOUTH_Y, JACKALOPE_NORTH_Y);
          wild_mobile_loader(EASY_JACKALOPE, x_coord, y_coord);
        }
      }
      if (mob_ingame_count(MED_JACKALOPE) < NUM_JACKALOPE_EACH)
      {
        for (mob_count = 0; mob_count < (NUM_JACKALOPE_EACH - mob_ingame_count(MED_JACKALOPE)); mob_count++)
        {
          x_coord = rand_number(JACKALOPE_WEST_X, JACKALOPE_EAST_X);
          y_coord = rand_number(JACKALOPE_SOUTH_Y, JACKALOPE_NORTH_Y);
          wild_mobile_loader(MED_JACKALOPE, x_coord, y_coord);
        }
      }
      if (mob_ingame_count(HARD_JACKALOPE) < NUM_JACKALOPE_EACH)
      {
        for (mob_count = 0; mob_count < (NUM_JACKALOPE_EACH - mob_ingame_count(HARD_JACKALOPE)); mob_count++)
        {
          x_coord = rand_number(JACKALOPE_WEST_X, JACKALOPE_EAST_X);
          y_coord = rand_number(JACKALOPE_SOUTH_Y, JACKALOPE_NORTH_Y);
          wild_mobile_loader(HARD_JACKALOPE, x_coord, y_coord);
        }
      }

      break;

    case THE_PRISONER_EVENT:

      /* check to make sure the portal is up */
      for (obj = world[real_room(TP_PORTAL_L_ROOM)].contents; obj; obj = obj->next_content)
      {
        if (GET_OBJ_VNUM(obj) == THE_PRISONER_PORTAL)
        {
          found = TRUE;
        }
      }
      if (!found)
      {
        obj = read_object(THE_PRISONER_PORTAL, VIRTUAL);
        if (obj)
        {
          obj_to_room(obj, real_room(TP_PORTAL_L_ROOM));
          act("...  $p flickers, shimmers, then manifests before you...", TRUE, 0, obj, 0, TO_ROOM);
        }
      }
      /* end portal check */

      /* exit now? */
      if (rand_number(0, 15))
        break;

      /* we are doing some fun broadcasts for environment */
      switch (rand_number(0, 6))
      {
      case 0:
        for (pt = descriptor_list; pt; pt = pt->next)
          if (IS_PLAYING(pt) && pt->character)
            send_to_char(pt->character, "A perpetual haze of green looms on the horizon as \tY\t=The Prisoner's power\tn flares throughout the realms!\r\n");
        break;
      case 1:
        for (pt = descriptor_list; pt; pt = pt->next)
          if (IS_PLAYING(pt) && pt->character)
            send_to_char(pt->character, "Booms and echoes of \tY\t=The Prisoner's power\tn resound throughout the realms!\r\n");
        break;
      case 2:
        for (pt = descriptor_list; pt; pt = pt->next)
          if (IS_PLAYING(pt) && pt->character)
            send_to_char(pt->character, "Emanations from the outter planes, via \tY\t=The Prisoner's power\tn, pulsate through the realms!\r\n");
        break;
      case 3:
        for (pt = descriptor_list; pt; pt = pt->next)
          if (IS_PLAYING(pt) && pt->character)
            send_to_char(pt->character, "The \tY\t=power of The Prisoner\tn is causing the very ground to shake!\r\n");
        break;
      case 4:
        for (pt = descriptor_list; pt; pt = pt->next)
          if (IS_PLAYING(pt) && pt->character)
            send_to_char(pt->character, "Vicious blasts of mental energy from \tY\t=The Prisoner\tn penetrate your psyche!\r\n");
        break;
      case 5:
        for (pt = descriptor_list; pt; pt = pt->next)
          if (IS_PLAYING(pt) && pt->character)
            send_to_char(pt->character, "Random ebbs and flows of chaotic magic from \tY\t=The Prisoner\tn manifest nearby!\r\n");
        break;
      case 6:
        for (pt = descriptor_list; pt; pt = pt->next)
          if (IS_PLAYING(pt) && pt->character)
            send_to_char(pt->character, "The very gravity of the realms seem to shift as \tY\t=The Prisoner's power\tn grows!\r\n");
        break;
      default:
        break;
      }

      break;

    default:
      break;
    }
  }

  /* Last tick - set everything back to zero */
  else if (STAFF_EVENT_TIME == 1)
  {
    end_staff_event(STAFF_EVENT_NUM);
  }
  /* end staff event section */
}

/* start a staff event! */
void start_staff_event(int event_num)
{
  struct descriptor_data *pt = NULL;
  int counter = 0;
  /* variables used in specific events */
  int x_coord = 0;
  int y_coord = 0;

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
      send_to_char(pt->character, "\tR[\tWInfo\tR]\tn A Staff Ran Event (\tn%s\tn) is Starting!\r\n\r\n",
                   staff_events_list[event_num][EVENT_TITLE]);
      send_to_char(pt->character, "\tn%s\tn\r\n\r\n",
                   staff_events_list[event_num][EVENT_DETAIL]);
      send_to_char(pt->character, "\r\n\tn%s\tn\r\n\r\n",
                   staff_events_list[event_num][EVENT_BEGIN]);
    }
  }

  /* set the event number in the global struct */
  STAFF_EVENT_NUM = event_num;

  /* default length for event, override in case below, 48 ticks = about 1 real hour */
  STAFF_EVENT_TIME = 1200; /* this is approximately one real day */

  /* what are we going to do for each event? */
  switch (event_num)
  {

  case JACKALOPE_HUNT:

    /* set event duration */
    // STAFF_EVENT_TIME = 20;

    /* load the jackalopes! */
    for (counter = 0; counter < NUM_JACKALOPE_EACH; counter++)
    {
      x_coord = rand_number(JACKALOPE_WEST_X, JACKALOPE_EAST_X);
      y_coord = rand_number(JACKALOPE_SOUTH_Y, JACKALOPE_NORTH_Y);
      wild_mobile_loader(EASY_JACKALOPE, x_coord, y_coord);

      x_coord = rand_number(JACKALOPE_WEST_X, JACKALOPE_EAST_X);
      y_coord = rand_number(JACKALOPE_SOUTH_Y, JACKALOPE_NORTH_Y);
      wild_mobile_loader(MED_JACKALOPE, x_coord, y_coord);

      x_coord = rand_number(JACKALOPE_WEST_X, JACKALOPE_EAST_X);
      y_coord = rand_number(JACKALOPE_SOUTH_Y, JACKALOPE_NORTH_Y);
      wild_mobile_loader(HARD_JACKALOPE, x_coord, y_coord);
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
      send_to_char(pt->character, "\tR[\tWInfo\tR]\tn A Staff Ran Event (\tn%s\tn) has ended.\r\n\r\n",
                   staff_events_list[event_num][EVENT_TITLE]);
      send_to_char(pt->character, "\tn%s\tn\r\n\r\n",
                   staff_events_list[event_num][EVENT_SUMMARY]);
      send_to_char(pt->character, "\r\n\tn%s\tn\r\n\r\n",
                   staff_events_list[event_num][EVENT_END]);
    }
  }

  /* make sure the event is turned off */
  STAFF_EVENT_NUM = UNDEFINED_EVENT;
  STAFF_EVENT_TIME = 0;
  STAFF_EVENT_DELAY = STAFF_EVENT_DELAY_CNST; /* this is a delay before next event for cleanup */

  switch (event_num)
  {

  case JACKALOPE_HUNT:
    /* jackalope cleanup crew */
    mob_ingame_purge(EASY_JACKALOPE);
    mob_ingame_purge(MED_JACKALOPE);
    mob_ingame_purge(HARD_JACKALOPE);
    break;

  case THE_PRISONER_EVENT:
    /* check to make sure the portal is gone */
    struct obj_data *obj = NULL;

    for (obj = world[real_room(TP_PORTAL_L_ROOM)].contents; obj; obj = obj->next_content)
    {
      if (GET_OBJ_VNUM(obj) == THE_PRISONER_PORTAL)
      {
        act("...  $p flickers, shimmers, then fades away...", TRUE, 0, obj, 0, TO_ROOM);
        extract_obj(obj);
      }
    }
    /* end portal check */

    break;

  default:
    break;
  }

  return;
}

/* details about a specific event */
void staff_event_info(struct char_data *ch, int event_num)
{

  /* dummy checks */
  if (!ch || event_num >= NUM_STAFF_EVENTS || event_num < 0)
  {
    return;
  }

  int event_field = 0;
  int secs_left = 0;

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
      if (GET_LEVEL(ch) >= LVL_STAFF)
      {

        send_to_char(ch, "Event end message to world: \tn%s\tn\r\n", staff_events_list[event_num][EVENT_END]);
      }
      break;

    case EVENT_DETAIL:
      send_to_char(ch, "Event info:\r\n\tn");
      send_to_char(ch, staff_events_list[event_num][EVENT_DETAIL]);
      send_to_char(ch, "\tn");
      break;

    case EVENT_TITLE: /* we mention this above */
    /* fallthrough */
    default:
      break;
    }
  } /*end for*/

  /* here is our custom output relevant to each event */
  switch (event_num)
  {
  case JACKALOPE_HUNT:
    send_to_char(ch, "Number of elusive Jackalope: %d, mature Jackalope: %d and alpha Jackalope: %d.\r\n",
                 mob_ingame_count(EASY_JACKALOPE),
                 mob_ingame_count(MED_JACKALOPE),
                 mob_ingame_count(HARD_JACKALOPE));
    break;

  default:
    break;
  }

  if (STAFF_EVENT_TIME)
    secs_left = ((STAFF_EVENT_TIME - 1) * SECS_PER_MUD_HOUR) + next_tick;
  else
    secs_left = 0;

  send_to_char(ch, "Event Time Remaining: %s%d%s hours %s%d%s mins %s%d%s secs\r\n",
               CCYEL(ch, C_NRM), (secs_left / 3600), CCNRM(ch, C_NRM),
               CCYEL(ch, C_NRM), (secs_left % 3600) / 60, CCNRM(ch, C_NRM),
               CCYEL(ch, C_NRM), (secs_left % 60), CCNRM(ch, C_NRM));

  send_to_char(ch, "Delay Timer Remaining Between Events: %s%d%s ticks.\r\n",
               CCYEL(ch, C_NRM), STAFF_EVENT_DELAY, CCNRM(ch, C_NRM));

  if (GET_LEVEL(ch) >= LVL_STAFF)
  {
    send_to_char(ch, "\r\nUsage: staffevents [start|end|info] [index # above]\r\n\r\n");
  }

  return;
}

/* list the events */
void list_staff_events(struct char_data *ch)
{
  int i = 0;

  send_to_char(ch, "\r\n\tCA Listing of Staff Ran Events:\tn\r\n\r\n");

  for (i = 0; i < NUM_STAFF_EVENTS; i++)
  {
    send_to_char(ch, "\tG%d)\tn %s\r\n", i, staff_events_list[i][0]);
  }

  send_to_char(ch, "\r\n\r\nUsage: staffevents [start|end|info] [index # above]\r\n");

  return;
}

/* command to start/end/list staff events */
ACMD(do_staffevents)
{
  char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
  int event_num = UNDEFINED_EVENT;

  if (GET_LEVEL(ch) < LVL_STAFF)
  {
    if (IS_STAFF_EVENT)
    {
      staff_event_info(ch, STAFF_EVENT_NUM);
    }
    else
    {
      send_to_char(ch, "There is no staff ran event currently!\r\n");
    }
    return;
  }

  /* should only be staff from this point onward */

  half_chop_c(argument, arg, sizeof(arg), arg2, sizeof(arg2));

  if (!*arg || !*arg2)
  {
    if (IS_STAFF_EVENT)
    {
      staff_event_info(ch, STAFF_EVENT_NUM);
    }
    else
    {
      list_staff_events(ch);
    }
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
    if (!IS_STAFF_EVENT && !STAFF_EVENT_DELAY)
    {
      start_staff_event(event_num);
    }
    else if (STAFF_EVENT_DELAY)
    {
      send_to_char(ch, "There is a delay of approximately %d more ticks between events for cleanup.\r\n",
                   STAFF_EVENT_DELAY);
    }
    else
    {
      send_to_char(ch, "There is already an event running!\r\n");
    }
  }
  else if (is_abbrev(arg, "end"))
  {
    if (!IS_STAFF_EVENT)
    {
      send_to_char(ch, "There is no event active right now...\r\n");
    }
    else
    {
      end_staff_event(event_num);
    }
  }
  else if (is_abbrev(arg, "info"))
  {
    staff_event_info(ch, event_num);
    if (!IS_STAFF_EVENT)
    {
      send_to_char(ch, "There is no event active right now...\r\n");
    }
  }
  else
  {
    list_staff_events(ch);
    send_to_char(ch, "Invalid argument!\r\n");
    return;
  }

  return;
}

/* undefines */

/* general */

#undef NUM_STAFF_EVENTS
#undef STAFF_EVENT_FIELDS

#undef EVENT_TITLE
#undef EVENT_BEGIN
#undef EVENT_END
#undef EVENT_DETAIL

/* end general */

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

/* end jackalope hunt undefines */

/* the prisoner undefines */

#undef THE_PRISONER_EVENT

/* end the prisoner undefines */

/* EOF */
