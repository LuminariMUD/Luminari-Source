/**************************************************************************
 *  File: spec/spec_zone_celestial_leviathan.c         Part of LuminariMUD *
 *  Usage: Celestial Leviathan encounter procedures.                      *
 *                                                                         *
 *  All rights reserved.  See license for complete information.            *
 **************************************************************************/

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "magic/spells.h"
#include "act.h"
#include "spec_zone_celestial_leviathan.h"
#include "spec_zone_prisoner.h"
#include "combat/fight.h"
#include "combat/spec_abilities.h"

/***********************/
/* Celestial Leviathan */
/***********************/

int rejuv_celestial_leviathan(struct char_data *ch)
{
  int rejuv = 0;

  if (!rand_number(0, 7) && GET_HIT(ch) < GET_MAX_HIT(ch) && PROC_FIRED(ch) == FALSE &&
      !FIGHTING(ch))
  {
    rejuv = GET_HIT(ch) + 1500;

    if (rejuv >= GET_MAX_HIT(ch))
      rejuv = GET_MAX_HIT(ch);

    GET_HIT(ch) = rejuv;

    PROC_FIRED(ch) = TRUE;

    act("\trThe blood-red wounds on the Prisoner's body begin to close as she is partially "
        "revived!\tn",
        FALSE, ch, 0, 0, TO_ROOM);

    return 1;
  }
  else if (!rand_number(0, 4))
  {
    PROC_FIRED(ch) = FALSE;
  }

  if (!rand_number(0, 10) && FIGHTING(ch) && GET_HIT(ch) < GET_MAX_HIT(ch))
  {
    rejuv = GET_HIT(ch) + 2500;

    if (rejuv >= GET_MAX_HIT(ch))
      rejuv = GET_MAX_HIT(ch);

    GET_HIT(ch) = rejuv;

    act("\tLThe Prisoner ROARS in anger, and throws her talons to the sky furiously!\r\n"
        "\tWWhite tendrils of power crackle through the air, flowing into the Prisoner!",
        FALSE, ch, 0, 0, TO_ROOM);
    act("\trThe blood-red wounds on the Prisoner's body begin to close as she is partially "
        "revived!\tn",
        FALSE, ch, 0, 0, TO_ROOM);

    return 1;
  }

  return 0;
}

int celestial_leviathan_attacks(struct char_data *ch)
{
  if (!ch)
    return 0;

  if (!FIGHTING(ch))
    return 0;

  if (!rand_number(0, 2))
  {
    int breaths = 0;
    int breath[5];
    int selected = 0;

    if (prisoner_heads >= 1)
      breath[breaths++] = SPELL_FIRE_BREATHE;
    if (prisoner_heads >= 2)
      breath[breaths++] = SPELL_LIGHTNING_BREATHE;
    if (prisoner_heads >= 3)
      breath[breaths++] = SPELL_ACID_BREATHE;
    if (prisoner_heads >= 4)
      breath[breaths++] = SPELL_FROST_BREATHE;
    if (prisoner_heads >= 5)
      breath[breaths++] = SPELL_GAS_BREATHE;

    if (breaths < 1)
      return 0;

    selected = dice(1, breaths) - 1;
    selected = breath[selected];

    // do a breath..  level 40 breath since she breaths every round.
    call_magic(ch, FIGHTING(ch), 0, selected, 0, 34, CAST_INNATE);

    return 1;
  }
  else if (!rand_number(0, 2) && perform_tailsweep(ch))
  {
    /* looks like we did the tailsweeep successffully to at least one victim */
    return 1;
  }
  else if (!rand_number(0, 2) && perform_dragonbite(ch, FIGHTING(ch)))
  {
    /* looks like we did the dragonbite to at least one victim */
    return 1;
  }
  else if (!rand_number(0, 2))
  {
    int i = 0;

    /* spam some attacks */
    for (i = 0; i <= rand_number(4, 8); i++)
    {
      if (valid_fight_cond(ch, TRUE))
        hit(ch, FIGHTING(ch), TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, FALSE);
    }
    return 1;
  }

  return 0;
}

/***************************/
/* End Celestial Leviathan */
/***************************/

SPECIAL(celestial_leviathan)
{
  return 0;
}
