/**************************************************************************
 *  File: spec/spec_zone_shadow_dragon.c                Part of LuminariMUD *
 *  Usage: Shadow Dragon encounter procedures.                             *
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
#include "spec_zone_shadow_dragon.h"

/* shadowdragon shadow breathe proc */
SPECIAL(shadowdragon)
{
  struct char_data *vict;
  struct char_data *next_vict;

  if (cmd)
    return FALSE;

  if (!FIGHTING(ch))
    return FALSE;

  if (rand_number(0, 4))
    return FALSE;

  act("$n \tLopens her mouth and let stream forth a black breath of de\tws\tWp\twa\tLir.\tn", FALSE,
      ch, 0, 0, TO_ROOM);

  for (vict = world[ch->in_room].people; vict; vict = next_vict)
  {
    next_vict = vict->next_in_room;
    if (IS_NPC(vict) && !IS_PET(vict))
      continue;

    act("\tLDarkness envelopes you and you feel the hopelessness of fighting against this all "
        "powerful foe.\tn",
        FALSE, ch, 0, vict, TO_VICT);
    act("$N \tLseems to loose the will for fighting against this awesome foe.\tn", FALSE, ch, 0,
        vict, TO_NOTVICT);
    GET_MOVE(vict) -= (10 + dice(5, 4));
  }

  call_magic(ch, FIGHTING(ch), 0, SPELL_DARKNESS, 0, GET_LEVEL(ch), CAST_WEAPON_SPELL);

  return TRUE;
}
