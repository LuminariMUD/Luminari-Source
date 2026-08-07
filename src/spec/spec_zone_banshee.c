/**************************************************************************
 *  File: spec/spec_zone_banshee.c                      Part of LuminariMUD *
 *  Usage: Banshee encounter procedures.                                   *
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
#include "magic/domains_schools.h"
#include "magic/spells.h"
#include "spec_zone_banshee.h"

/* banshee procs */
SPECIAL(banshee)
{
  struct char_data *vict;

  if (cmd || GET_POS(ch) == POS_DEAD)
    return FALSE;

  if (!FIGHTING(ch)) // heheh  && GET_HIT(ch) == GET_MAX_HIT(ch))
    PROC_FIRED(ch) = FALSE;

  if (FIGHTING(ch) && !rand_number(0, 40) && PROC_FIRED(ch) != TRUE)
  {
    act("\tW$n \tWlets out a piercing shriek so horrible that it makes your ears \trBLEED\tW!\tn",
        FALSE, ch, 0, 0, TO_ROOM);
    for (vict = world[ch->in_room].people; vict; vict = vict->next_in_room)
      if (!IS_NPC(vict) &&
          !savingthrow(ch, vict, SAVING_WILL, -4, CAST_INNATE, GET_LEVEL(ch), NOSCHOOL))
      {
        act("\tRThe brutal scream tears away at your life force,\r\n"
            "causing you to fall to your knees with pain!\tn",
            FALSE, vict, 0, 0, TO_CHAR);
        act("$n grabs $s ears and tumbles to the ground in pain!", FALSE, vict, 0, 0, TO_ROOM);
        GET_HIT(vict) = 1;
      }

    PROC_FIRED(ch) = TRUE;
    return TRUE;
  }
  return FALSE;
}
