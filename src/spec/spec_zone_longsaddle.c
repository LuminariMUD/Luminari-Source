/**************************************************************************
 *  File: spec/spec_zone_longsaddle.c                  Part of LuminariMUD *
 *  Usage: Longsaddle zone procedures.                                    *
 *                                                                         *
 *  All rights reserved.  See license for complete information.            *
 **************************************************************************/

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "db.h"
#include "magic/spells.h"
#include "spec_zone_longsaddle.h"
#include "combat/fight.h"

/* from homeland */
SPECIAL(harpell)
{
  struct char_data *i = NULL;
  char buf[MAX_INPUT_LENGTH] = {'\0'};

  if (cmd || GET_POS(ch) == POS_DEAD)
    return FALSE;

  if (!FIGHTING(ch))
    PROC_FIRED(ch) = FALSE;

  if (FIGHTING(ch) && !ROOM_FLAGGED(ch->in_room, ROOM_SOUNDPROOF))
  {
    if (AFF_FLAGGED(FIGHTING(ch), AFF_CHARM) && FIGHTING(ch)->master)
      snprintf(buf, sizeof(buf),
               "%s shouts, 'HELP! %s has ordered his pets to kill "
               "me!!'\r\n",
               ch->player.short_descr, GET_NAME(FIGHTING(ch)->master));
    else
      snprintf(buf, sizeof(buf), "%s shouts, 'HELP! %s is trying to kill me!\r\n",
               ch->player.short_descr, GET_NAME(FIGHTING(ch)));
    for (i = character_list; i; i = i->next)
    {
      if (!FIGHTING(i) && IS_NPC(i) &&
          (GET_MOB_VNUM(i) == 106831 || GET_MOB_VNUM(i) == 106841 || GET_MOB_VNUM(i) == 106842 ||
           GET_MOB_VNUM(i) == 106844 || GET_MOB_VNUM(i) == 106845 || GET_MOB_VNUM(i) == 106846) &&
          ch != i && !rand_number(0, 2))
      {
        if (AFF_FLAGGED(FIGHTING(ch), AFF_CHARM) && FIGHTING(ch)->master &&
            (FIGHTING(ch)->master->in_room != FIGHTING(ch)->in_room))
        {
          if (FIGHTING(ch)->master->in_room != i->in_room)
            cast_spell(i, FIGHTING(ch)->master, NULL, SPELL_TELEPORT, 0);
          else
            hit(i, FIGHTING(ch)->master, TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, FALSE);
        }
        else
        {
          if (FIGHTING(ch)->in_room != i->in_room)
            cast_spell(i, FIGHTING(ch), NULL, SPELL_TELEPORT, 0);
          else
            hit(i, FIGHTING(ch), TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, FALSE);
        }
      }

      if (world[ch->in_room].zone == world[i->in_room].zone && !PROC_FIRED(ch))
        send_to_char(i, "%s", buf);
    }
    PROC_FIRED(ch) = TRUE;
    return TRUE;
  } // for loop

  return FALSE;
}
