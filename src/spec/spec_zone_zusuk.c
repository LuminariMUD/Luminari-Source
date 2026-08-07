/**************************************************************************
 *  File: spec/spec_zone_zusuk.c                       Part of LuminariMUD *
 *  Usage: Zusuk zone procedures.                                         *
 *                                                                         *
 *  All rights reserved.  See license for complete information.            *
 **************************************************************************/

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "spec_zone_zusuk.h"

/* from homeland */
SPECIAL(fzoul)
{
  if (!ch && !cmd)
    return FALSE;

  if (cmd && CMD_IS("kneel"))
  {
    send_to_char(ch, "\tLFzoul tells you, '\tgSee how easy it is to kneel before the beauty of our "
                     "god.\tL'\tn\r\n");
    return TRUE;
  }
  return FALSE;
}
