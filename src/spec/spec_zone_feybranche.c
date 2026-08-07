/**************************************************************************
 *  File: spec/spec_zone_feybranche.c                   Part of LuminariMUD *
 *  Usage: Fey-Branche zone procedures.                                   *
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
#include "spec_zone_feybranche.h"
#include "combat/fight.h"
#include "graph.h"

/* from homeland */
SPECIAL(feybranche)
{
  struct char_data *i = NULL;
  char buf[MAX_INPUT_LENGTH] = {'\0'};

  if (cmd || GET_POS(ch) == POS_DEAD)
    return FALSE;

  struct char_data *enemy = FIGHTING(ch);

  if (!enemy)
    PROC_FIRED(ch) = FALSE;

  if (FIGHTING(ch) && !ROOM_FLAGGED(ch->in_room, ROOM_SOUNDPROOF))
  {
    if (enemy->master && enemy->master->in_room == enemy->in_room)
      enemy = enemy->master;
    snprintf(buf, sizeof(buf),
             "%s\tL shouts, '\tmCome to me!!' Fey-Branche is under attack!\tn\r\n",
             ch->player.short_descr);
    for (i = character_list; i; i = i->next)
    {
      if (!FIGHTING(i) && IS_NPC(i) &&
          (GET_MOB_VNUM(i) == 135535 || GET_MOB_VNUM(i) == 135536 || GET_MOB_VNUM(i) == 135537 ||
           GET_MOB_VNUM(i) == 135538 || GET_MOB_VNUM(i) == 135539 || GET_MOB_VNUM(i) == 135540) &&
          ch != i)
      {
        if (FIGHTING(ch)->in_room != i->in_room)
        {
          if (GET_MOB_VNUM(i) != 135536)
          {
            HUNTING(i) = enemy;
            hunt_victim(i);
          }
          else
            cast_spell(i, enemy, 0, SPELL_TELEPORT, 0);
        }
        else
          hit(i, enemy, TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, FALSE);
      }

      if (world[ch->in_room].zone == world[i->in_room].zone && !PROC_FIRED(ch))
        send_to_char(i, "%s", buf);
    }
    PROC_FIRED(ch) = TRUE;
    return TRUE;
  }
  return FALSE;
}
