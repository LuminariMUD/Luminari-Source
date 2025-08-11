/**************************************************************************
 *  File: movement_falling.c                          Part of LuminariMUD *
 *  Usage: Falling mechanics functions                                    *
 *                                                                         *
 *  All rights reserved.  See license complete information.               *
 *                                                                         *
 *  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.              *
 **************************************************************************/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "spells.h"
#include "constants.h"
#include "act.h"
#include "fight.h"
#include "mud_event.h"
#include "actions.h"
#include "feats.h"
#include "movement_falling.h"

/* External functions */
extern int do_simple_move(struct char_data *ch, int dir, int need_specials_check);
extern int change_position(struct char_data *ch, int new_position);

/* falling system */

/* TODO objects */
/* Actually in the original design of falling, I wanted to have objects
* fall and stack up at the bottom (if they float they don't fall) -zusuk
 * this obviously has not been expanded on at all :)
 */

bool obj_should_fall(struct obj_data *obj)
{
  int falling = FALSE;

  if (!obj)
    return FALSE;

  if (ROOM_FLAGGED(obj->in_room, ROOM_FLY_NEEDED) && EXIT_OBJ(obj, DOWN))
    falling = TRUE;

  if (OBJ_FLAGGED(obj, ITEM_FLOAT))
  {
    act("You watch as $p floats gracefully in the air!",
        FALSE, 0, obj, 0, TO_ROOM);
    return FALSE;
  }

  return falling;
}

/* this function will check whether a char should fall or not based on
   circumstances and whether the ch is flying / levitate */
bool char_should_fall(struct char_data *ch, bool silent)
{
  int falling = FALSE;

  if (!ch)
    return FALSE;

  if (IN_ROOM(ch) == NOWHERE)
    return FALSE;

  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_FLY_NEEDED) && EXIT(ch, DOWN))
    falling = TRUE;

  if (RIDING(ch) && is_flying(RIDING(ch)))
  {
    if (!silent)
      send_to_char(ch, "Your mount flies gracefully through the air...\r\n");
    return FALSE;
  }

  if (is_flying(ch))
  {
    if (!silent)
      send_to_char(ch, "You fly gracefully through the air...\r\n");
    return FALSE;
  }

  if (AFF_FLAGGED(ch, AFF_LEVITATE))
  {
    if (!silent)
      send_to_char(ch, "You levitate above the ground...\r\n");
    return FALSE;
  }

  return falling;
}

EVENTFUNC(event_falling)
{
  struct mud_event_data *pMudEvent = NULL;
  struct char_data *ch = NULL;
  int height_fallen = 0;
  char buf[50] = {'\0'};

  /* This is just a dummy check, but we'll do it anyway */
  if (event_obj == NULL)
    return 0;

  /* For the sake of simplicity, we will place the event data in easily
   * referenced pointers */
  pMudEvent = (struct mud_event_data *)event_obj;

  /* nab char data */
  ch = (struct char_data *)pMudEvent->pStruct;

  /* dummy checks */
  if (!ch)
    return 0;
  if (!IS_NPC(ch) && !IS_PLAYING(ch->desc))
    return 0;

  /* retrieve svariables and convert it */
  height_fallen += atoi((char *)pMudEvent->sVariables);
  send_to_char(ch, "AIYEE!!!  You have fallen %d feet!\r\n", height_fallen);

  /* already checked if there is a down exit, lets move the char down */
  do_simple_move(ch, DOWN, FALSE);
  send_to_char(ch, "You fall into a new area!\r\n");
  act("$n appears from above, arms flailing helplessly as $e falls...",
      FALSE, ch, 0, 0, TO_ROOM);
  height_fallen += 20; // 20 feet per room right now

  /* can we continue this fall? */
  if (!ROOM_FLAGGED(ch->in_room, ROOM_FLY_NEEDED) || !CAN_GO(ch, DOWN))
  {

    if (AFF_FLAGGED(ch, AFF_SAFEFALL))
    {
      send_to_char(ch, "Moments before slamming into the ground, a 'safefall'"
                       " enchantment stops you!\r\n");
      act("Moments before $n slams into the ground, some sort of magical force"
          " stops $s from the impact.",
          FALSE, ch, 0, 0, TO_ROOM);
      REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_SAFEFALL);
      return 0;
    }

    /* potential damage */
    int dam = dice((height_fallen / 5), 6) + 20;

    /* check for slow-fall! */
    if (!IS_NPC(ch) && HAS_FEAT(ch, FEAT_SLOW_FALL))
    {
      dam -= 21;
      dam -= dice((HAS_FEAT(ch, FEAT_SLOW_FALL) * 4), 6);
    }
    if (HAS_FEAT(ch, FEAT_DRACONIAN_CONTROLLED_FALL))
    {
      dam /= 2;
    }

    if (dam <= 0)
    { /* woo! avoided damage */
      send_to_char(ch, "You gracefully land on your feet from your perilous fall!\r\n");
      act("$n comes falling in from above, but at the last minute, pulls of an acrobatic flip and lands gracefully on $s feet!", FALSE, ch, 0, 0, TO_ROOM);
      return 0; // end event
    }
    else
    { /* ok we know damage is going to be suffered at this stage */
      send_to_char(ch, "You fall headfirst to the ground!  OUCH!\r\n");
      act("$n crashes into the ground headfirst, OUCH!", FALSE, ch, 0, 0, TO_ROOM);
      change_position(ch, POS_RECLINING);
      start_action_cooldown(ch, atSTANDARD, 12 RL_SEC);

      /* we have a special situation if you die, the event will get cleared */
      if (dam >= GET_HIT(ch) + 9)
      {
        GET_HIT(ch) = -999;
        send_to_char(ch, "You attempt to scream in horror as your skull slams "
                         "into the ground, the very brief sensation of absolute pain "
                         "strikes you as all your upper-body bones shatter and your "
                         "head splatters all over the area!\r\n");
        act("$n attempts to scream in horror as $s skull slams "
            "into the ground.  There is the sound like the cracking of a "
            "ripe melon.  You watch as all of $s upper-body bones shatter and $s "
            "head splatters all over the area!\r\n",
            FALSE, ch, 0, 0, TO_ROOM);
        return 0;
      }
      else
      {
        damage(ch, ch, dam, TYPE_UNDEFINED, DAM_FORCE, FALSE);
        return 0; // end event
      }
    }
  }

  /* hitting ground or fixing your falling situation is the only way to stop
   *  this event :P
   * theoritically the player now could try to cast a spell, use an item, hop
   * on a mount to fix his falling situation, so we gotta check if he's still
   * falling every event call
   *  */
  if (char_should_fall(ch, FALSE))
  {
    send_to_char(ch, "You fall tumbling down!\r\n");
    act("$n drops from sight.", FALSE, ch, 0, 0, TO_ROOM);

    /* are we falling more?  then we gotta increase the heigh fallen */
    snprintf(buf, sizeof(buf), "%d", height_fallen);
    /* Need to free the memory, if we are going to change it. */
    if (pMudEvent->sVariables)
      free(pMudEvent->sVariables);
    pMudEvent->sVariables = strdup(buf);
    return (1 * PASSES_PER_SEC);
  }
  else
  { // stop falling!
    send_to_char(ch, "You put a stop to your fall!\r\n");
    act("$n turns on the air-brakes from a plummet!", FALSE, ch, 0, 0, TO_ROOM);
    return 0;
  }

  return 0;
}
/*  END falling system */