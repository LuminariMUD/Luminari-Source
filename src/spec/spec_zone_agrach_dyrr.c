/**************************************************************************
 *  File: spec/spec_zone_agrach_dyrr.c                  Part of LuminariMUD *
 *  Usage: House Agrach-Dyrr zone procedures.                              *
 *                                                                         *
 *  All rights reserved.  See license for complete information.            *
 **************************************************************************/

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "magic/spells.h"
#include "spec_zone_agrach_dyrr.h"
#include "combat/fight.h"
#include "graph.h"

/* from homeland */
SPECIAL(agrachdyrr)
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
    if (!rand_number(0, 4) && !ch->followers)
    {
      act("$n\tL looks to be extremely disspleased at being\r\n"
          "forced to fight such inferior beings in her own mansion. She raises her\r\n"
          "arms and cries out, '\tmAid me Lloth!\tL'",
          FALSE, ch, 0, 0, TO_ROOM);

      struct char_data *mob = read_mobile(135523, VIRTUAL);
      if (!mob)
        return FALSE;
      char_to_room(mob, ch->in_room);
      add_follower(mob, ch);
      return TRUE;
    }

    if (enemy->master && enemy->master->in_room == enemy->in_room)
      enemy = enemy->master;

    snprintf(buf, sizeof(buf),
             "%s\tL shouts, '\twTo me, \tcAgrach-Dyrr\tw is under attack!'\tn\r\n",
             ch->player.short_descr);
    for (i = character_list; i; i = i->next)
    {
      if (!FIGHTING(i) && IS_NPC(i) &&
          (GET_MOB_VNUM(i) == 135521 || GET_MOB_VNUM(i) == 135522 || GET_MOB_VNUM(i) == 135510 ||
           GET_MOB_VNUM(i) == 135524 || GET_MOB_VNUM(i) == 135525 || GET_MOB_VNUM(i) == 135512) &&
          ch != i)
      {
        if (FIGHTING(ch)->in_room != i->in_room)
        {
          if (GET_MOB_VNUM(i) != 135522)
          {
            set_hunting_target(i, enemy);
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
  } // for loop
  return FALSE;
}
