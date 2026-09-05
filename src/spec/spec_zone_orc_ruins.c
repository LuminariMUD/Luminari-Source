/**************************************************************************
 *  File: spec/spec_zone_orc_ruins.c                    Part of LuminariMUD *
 *  Usage: Orc Ruins zone procedures.                                     *
 *                                                                         *
 *  All rights reserved.  See license for complete information.            *
 **************************************************************************/

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "utils.h"
#include "combat/fight.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "magic/spells.h"
#include "spec_zone_orc_ruins.h"

/* from homeland */
SPECIAL(shar_heart)
{
  if (!ch)
    return FALSE;

  struct affected_type af;
  int dam = 0;

  struct char_data *vict = FIGHTING(ch);

  if (cmd || !vict)
    return FALSE;

  if (rand_number(0, 15))
    return FALSE;

  act("\tmThe \tMHeart of Shar \tn\tmpulses erratically in\r\n"
      "your hand before striking $N \tmwith a beam of\r\n"
      "\tLmalevolent light\tn\tm, bathing and filling $M with\r\n"
      "the virulence of the \tLL\tMady of \tLL\tMoss.\tn",
      FALSE, ch, 0, vict, TO_CHAR);

  act("\tmThe amethyst orb wielded by \tL$n \tn\tmpulses\r\n"
      "erratically before a beam of \tLmalevolent light\r\n"
      "\tn\tmshoots from it, striking you in the chest!\tn",
      FALSE, ch, 0, vict, TO_VICT);

  act("\tL$n \tn\tmis bathed in an amethyst radiance as $s\r\n"
      "\tMHeart of Shar \tn\tmpulses erratically.  Suddenly a\r\n"
      "sickly beam of \tLmalevolent light \tn\tmblazes\r\n"
      "towards $N\tm, filling $S body with the \tLvirulence\r\n"
      "\tn\tmof the \tLL\tMady of \tLL\tMoss.\tn",
      FALSE, ch, 0, vict, TO_ROOM);

  af.duration = 5;
  af.modifier = -4;
  af.location = APPLY_STR;
  af.spell = SPELL_POISON;
  affect_join(vict, &af, FALSE, FALSE, FALSE, FALSE);

  dam = dice(6, 3) + 4;
  combat_apply_raw_damage(vict, ch, dam, DAM_POISON, INT_MIN);
  return TRUE;
}

/* from homeland */
SPECIAL(shar_statue)
{
  struct char_data *mob;

  if (!FIGHTING(ch))
    return FALSE;
  if (cmd)
    return FALSE;

  if (!rand_number(0, 8) || !PROC_FIRED(ch))
  {
    PROC_FIRED(ch) = TRUE;
    send_to_room(ch->in_room, "\tLThe statue raises her ebon arms, screaming out to\r\n"
                              "her deity in a booming voice, '\tn\tmLady of loss,\r\n"
                              "mistress of the night, smite those who befoul your\r\n"
                              "house.  Send forth your faithful to quench the light\r\n"
                              "of their moon!\tL'\tn\r\n");

    if (dice(1, 100) < 50)
      mob = read_mobile(106241, VIRTUAL);
    else
      mob = read_mobile(106240, VIRTUAL);

    if (!mob)
      return FALSE;

    char_to_room(mob, ch->in_room);
    add_follower(mob, ch);

    return TRUE;
  }
  return FALSE;
}
