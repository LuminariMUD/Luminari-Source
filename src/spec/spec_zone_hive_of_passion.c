/**************************************************************************
 *  File: spec/spec_zone_hive_of_passion.c              Part of LuminariMUD *
 *  Usage: Hive of Passion zone procedures.                                *
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
#include "act.h"
#include "spec_zone_hive_of_passion.h"

/* from homeland */
SPECIAL(hive_death)
{
  if (cmd)
    return FALSE;
  if (!ch)
    return FALSE;

  send_to_char(ch, "\trAs you enter through the curtain, your body is ripped into two pieces, as "
                   "your link\tn\r\n"
                   "\trthrough the ethereal plane is severed.  You suddenly realise that your "
                   "physical body\tn\r\n"
                   "\tris at one place, and your mind in another part.\tn\r\n\r\n");
  char_from_room(ch);
  char_to_room(ch, real_room(129500));
  // make_corpse(ch, 0);
  send_to_char(
      ch, "\tWYou feel the link snap completely, leaving you body behind completely!\tn\r\n\r\n");
  look_at_room(ch, 0);
  char_from_room(ch);
  send_to_char(ch, "\tLYou focus your eyes back on the present.\tn\r\n\r\n");
  char_to_room(ch, real_room(139328));
  look_at_room(ch, 0);

  return TRUE;
}
