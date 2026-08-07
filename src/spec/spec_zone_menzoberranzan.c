/**************************************************************************
 *  File: spec/spec_zone_menzoberranzan.c              Part of LuminariMUD *
 *  Usage: Menzoberranzan zone procedures.                                *
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
#include "graph.h"
#include "movement/movement.h"
#include "dgscript/dg_scripts.h"
#include "spec_zone_menzoberranzan.h"

/* from homeland */
static bool gr_stalled = FALSE;

SPECIAL(gromph)
{
  struct char_data *victim;
  int dir = -1;

  if (!ch)
    return FALSE;

  if (FIGHTING(ch))
    return FALSE;

  if (!IS_NPC(ch) && cmd && CMD_IS("cast"))
  {
    victim = ch;
    ch = (struct char_data *)me;
    act("$n sighs at YOU and mutters, 'You insolent worm!'", FALSE, ch, 0, victim, TO_VICT);
    act("$n sighs at $N, 'You insolent worm!'", FALSE, ch, 0, victim, TO_NOTVICT);
    call_magic(ch, victim, 0, SPELL_MISSILE_STORM, 0, 30, CAST_WEAPON_SPELL);
    return TRUE;
  }

  if (PATH_DELAY(ch) > 0)
    PATH_DELAY(ch)
  --;
  PATH_DELAY(ch) = 4;

  if (cmd)
    return FALSE;

  {
    switch (PROC_FIRED(ch))
    {
    case 0:
      // move to sorcere
      dir = find_first_step(ch->in_room, real_room(135250));
      if (dir < 0)
        PROC_FIRED(ch) = 1;
      break;
    case 1:
      // move to narbondel
      dir = find_first_step(ch->in_room, real_room(135353));

      if (dir < 0)
      {
        if (time_info.hours == 0 && gr_stalled == TRUE)
        {
          send_to_zone(
              "\tLSuddenly the base of the gigantic rockpillar known as \trNar\tRbon\trdel\tL\r\n"
              "\tLlights up with intense \trheat\tL, as Gromph Baenre uses his magic to relit it "
              "to\r\n"
              "\tLmark the start of a new day in the city.\tn\r\n",
              ch->in_room);
          gr_stalled = FALSE;
          PROC_FIRED(ch) = 0;
        }
        else
          gr_stalled = TRUE;
      }
      break;
    }
    if (dir >= 0)
      perform_move(ch, dir, 1);
    return TRUE;
  }
  return FALSE;
}
