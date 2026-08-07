/**************************************************************************
 *  File: spec/spec_zone_quicksand.c                    Part of LuminariMUD *
 *  Usage: Quicksand room procedures.                                      *
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
#include "magic/spells.h"
#include "spec_zone_quicksand.h"

/* marsh quicksand proc */
SPECIAL(quicksand)
{
  struct affected_type af;

  if (cmd)
    return FALSE;

  if (IS_NPC(ch) && !IS_PET(ch))
    return FALSE;

  if (is_flying(ch))
    return FALSE;
  if (paralysis_immunity(ch))
    return FALSE;
  if (GET_LEVEL(ch) > LVL_IMMORT)
    return FALSE;

  if (GET_DEX(ch) > dice(1, 20) + 12)
  {
    act("\tyYou avoid getting stuck in the quicksand.\tn", FALSE, ch, 0, 0, TO_CHAR);
    act("\tn$n\ty avoids getting stuck in the quicksand.\tn", FALSE, ch, 0, 0, TO_ROOM);
    return FALSE;
  }

  act("\tyThe marsh \tgla\tynd of the \twm\tye\tgr\tye opens up suddenly revealing "
      "quicksand!\tn\r\n"
      "\tnYou get sucked down.\tn",
      FALSE, ch, 0, 0, TO_CHAR);
  act("\tn$n\ty gets stuck in the quicksand of the marsh \tgla\tynd of the \twm\tye\tgr\tye.\tn",
      FALSE, ch, 0, 0, TO_ROOM);

  new_affect(&af);
  af.spell = SPELL_HOLD_PERSON;
  SET_BIT_AR(af.bitvector, AFF_PARALYZED);
  af.duration = 3;
  affect_join(ch, &af, TRUE, FALSE, FALSE, FALSE);

  return TRUE;
}
