/**************************************************************************
 *  File: spec/spec_zone_secomber.c                    Part of LuminariMUD *
 *  Usage: Secomber zone procedures.                                      *
 *                                                                         *
 *  All rights reserved.  See license for complete information.            *
 **************************************************************************/

#include "conf.h"
#include "core/sysdep.h"

#include "core/structs.h"
#include "core/utils.h"
#include "core/comm.h"
#include "core/interpreter.h"
#include "movement/movement.h"
#include "spec_zone_secomber.h"

/* from homeland */
SPECIAL(secomber_guard)
{
  const char *buf =
      "\tLThe doorguard steps before you, blocking your way with an upraised hand.\tn\r\n";
  const char *buf2 =
      "\tLThe doorguard blocks \tn$n\tL's way, placing one meaty hand on $s chest.\tn";

  if (!IS_MOVE(cmd))
    return FALSE;

  if (cmd == SCMD_EAST)
  {
    send_to_char(ch, "%s", buf);
    act(buf2, FALSE, ch, 0, 0, TO_ROOM);
    return TRUE;
  }

  return FALSE;
}
