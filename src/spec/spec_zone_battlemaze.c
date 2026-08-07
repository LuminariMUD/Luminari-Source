/**************************************************************************
 *  File: spec/spec_zone_battlemaze.c                  Part of LuminariMUD *
 *  Usage: Battlemaze zone procedures.                                    *
 *                                                                         *
 *  All rights reserved.  See license for complete information.            *
 **************************************************************************/

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "movement/movement.h"
#include "spec_zone_battlemaze.h"

/* from homeland */
SPECIAL(battlemaze_guard)
{
  const char *buf = "$N \tL tells you, 'You don't want to go any farther, young one. \tn\r\n"
                    "\tL You must be at least level ten to go into the more advanced\tn\r\n"
                    "\tL parts of the battlemaze.'\tn";
  const char *buf2 = "$N \tLsteps in front of $n\tL, blocking access the gate.\tn";

  if (!IS_MOVE(cmd))
    return FALSE;

  if (cmd == SCMD_NORTH && GET_LEVEL(ch) < 10)
  {
    act(buf, FALSE, ch, 0, (struct char_data *)me, TO_CHAR);
    act(buf2, FALSE, ch, 0, (struct char_data *)me, TO_ROOM);
    return TRUE;
  }

  return FALSE;
}
