/**************************************************************************
 *  File: spec/spec_zone_illithid_enclave.c             Part of LuminariMUD *
 *  Usage: Illithid Enclave zone procedures.                               *
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
#include "spec_zone_illithid_enclave.h"

/* from homeland */
SPECIAL(illithid_gguard)
{
  const char *buf = "$N \tLsteps in front of you, blocking you from accessing the gate.\tn";
  const char *buf2 = "$N \tLsteps in front of $n\tL, blocking access the gate.\tn";

  if (!IS_MOVE(cmd))
    return FALSE;

  // if (cmd == SCMD_EAST && GET_RACE(ch) != RACE_ILLITHID) {
  if (cmd == SCMD_EAST)
  {
    act(buf, FALSE, ch, 0, (struct char_data *)me, TO_CHAR);
    act(buf2, FALSE, ch, 0, (struct char_data *)me, TO_ROOM);
    return TRUE;
  }

  return FALSE;
}
