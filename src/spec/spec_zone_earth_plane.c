/**************************************************************************
 *  File: spec/spec_zone_earth_plane.c                  Part of LuminariMUD *
 *  Usage: Earth Plane zone procedures.                                    *
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
#include "spec_zone_earth_plane.h"
#include "combat/fight.h"
#include "graph.h"

/* from homeland */
SPECIAL(ogremoch)
{
  struct char_data *i;
  struct char_data *vict;
  struct descriptor_data *d;
  room_rnum room = 0;
  zone_rnum zone = world[ch->in_room].zone;
  room_rnum start = 0;
  room_rnum end = 0;

  if (cmd || GET_POS(ch) == POS_DEAD)
    return FALSE;

  if (!FIGHTING(ch))
    PROC_FIRED(ch) = FALSE;

  vict = FIGHTING(ch);
  if (!vict)
    return FALSE;

  // show yell message.
  if (PROC_FIRED(ch) == false)
  {
    for (d = descriptor_list; d; d = d->next)
    {
      if (STATE(d) == CON_PLAYING && d->character != NULL && IN_ROOM(d->character) != NOWHERE &&
          IN_ROOM(d->character) <= top_of_world && zone == world[d->character->in_room].zone)
      {
        send_to_char(d->character, "\tLOgremoch \tw shouts, '\tLI have been "
                                   "attacked! Come to me minions!\tw'\tn\r\n");
      }
    }
  }

  start = real_room(136700);
  end = real_room(136802);
  if (start == NOWHERE || end == NOWHERE || start > end)
  {
    log("SYSERR: ogremoch could not resolve its reinforcement room range.");
    return FALSE;
  }

  for (room = start; room <= end; room++)
  {
    for (i = world[room].people; i; i = i->next_in_room)
    {
      if (IS_NPC(i) && !FIGHTING(i))
      {
        switch (GET_MOB_VNUM(i))
        {
        case 136703:
        case 136704:
        case 136705:
        case 136706:
        case 136707:
        case 136708:
        case 136709:
          if (i->in_room == ch->in_room)
          {
            act("$n jumps to the aid of $N!", FALSE, i, 0, ch, TO_ROOM);
            hit(i, vict, TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, FALSE);
          }
          else
          {
            // either melt in directly or track
            if (dice(1, 10) < 2)
            {
              act("$n jumps into the pure rock, as $s lord calls for $m.", FALSE, i, 0, 0, TO_ROOM);
              char_from_room(i);
              char_to_room(i, ch->in_room);
              act("$n comes out from the rock, to help $s lord.", FALSE, i, 0, 0, TO_ROOM);
            }
            else
            {
              HUNTING(i) = ch;
              hunt_victim(i);
            }
          }
          break;
        }
      }
    }
  }
  PROC_FIRED(ch) = TRUE;

  return TRUE;
}
