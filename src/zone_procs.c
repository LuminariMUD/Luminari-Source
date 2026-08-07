/**************************************************************************
 *  File: zone_procs.c                                 Part of LuminariMUD *
 *  Usage: Special procedures for zones                                    *
 *  Author:  Zusuk                                                         *
 *                                                                         *
 *  Header File:  spec_procs.h                                             *
 **************************************************************************/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "constants.h"
#include "magic/spells.h"
#include "act.h"        /* for act related stuff, like act.offensive fuctions */
#include "spec_procs.h" /**< zone_procs.c is part of the spec_procs module */
#include "combat/fight.h"
#include "graph.h"
#include "mud_event.h"
#include "actions.h"
#include "magic/domains_schools.h"
#include "combat/spec_abilities.h"
#include "obj/treasure.h"
#include "mob/mob_utils.h"       /* for npc_find_target() */
#include "dgscript/dg_scripts.h" /* for load_mtrigger() */
#include "quest/staff_events.h"  /* for staff events!  prisoner treasury! */
#include "character/evolutions.h"
#include "spec/spec_effective_binding.h"
#include "spec/spec_registry.h"


/*****************/
/* Temple of Twisted Flesh (TTF) */

/*****************/

SPECIAL(ttf_monstrosity)
{
  struct char_data *vict;
  struct char_data *next_vict;
  int percent, prob;

  if (cmd || GET_POS(ch) == POS_DEAD)
    return 0;

  if (!FIGHTING(ch))
    return 0;

  if (dice(1, 10) > 2)
    return 0;

  act("\tLThe tentacled monstrosity rises up in the air and sends its full mass crashing into the "
      "floor!\tn",
      FALSE, ch, 0, 0, TO_ROOM);

  for (vict = world[ch->in_room].people; vict; vict = next_vict)
  {
    next_vict = vict->next_in_room;

    if (!aoeOK(ch, vict, -1))
      continue;

    percent = rand_number(1, 101); /* 101% is a complete failure */
    prob = GET_LEVEL(ch) / 5;
    if (percent < prob)
    {
      change_position(vict, POS_SITTING);
      WAIT_STATE(vict, 1 * PULSE_VIOLENCE);
      act("\trThe shockwave sends you crashing to the ground!\tn", FALSE, vict, 0, 0, TO_CHAR);
      act("\trThe shockwave sends \tn$n\tr crashing to the ground!\tn", FALSE, vict, 0, 0, TO_ROOM);
    }
  }
  return TRUE;
}

SPECIAL(ttf_abomination)
{
  struct char_data *vict;
  struct char_data *next_vict;
  int percent, prob;

  if (cmd || GET_POS(ch) == POS_DEAD)
    return 0;

  if (!FIGHTING(ch))
    return 0;

  if (dice(1, 16) > 2)
    return 0;

  act("\tLA gargantuan four-armed battle abomination lunges forward and swings one of his\r\n"
      "\tLenormous arms straight into your group!\tn",
      FALSE, ch, 0, 0, TO_ROOM);

  for (vict = world[ch->in_room].people; vict; vict = next_vict)
  {
    next_vict = vict->next_in_room;

    if (!aoeOK(ch, vict, -1))
      continue;

    percent = rand_number(1, 101); /* 101% is a complete failure */
    prob = GET_LEVEL(ch) / 5;
    if (percent < prob)
    {
      change_position(vict, POS_SITTING);
      WAIT_STATE(vict, 1 * PULSE_VIOLENCE);
      act("\trYou are unable to dodge the blow, and its force sends you crashing to the ground!\tn",
          FALSE, vict, 0, 0, TO_CHAR);
      act("$n \tris unable to dodge the blow, and its force sends $m crashing to the ground!\tn",
          FALSE, vict, 0, 0, TO_ROOM);
    }
  }
  return TRUE;
}

SPECIAL(ttf_rotbringer)
{
  int hp;
  struct char_data *mob;

  if (cmd || GET_POS(ch) == POS_DEAD)
    return 0;

  if (!FIGHTING(ch))
    PROC_FIRED(ch) = FALSE;
  if (!FIGHTING(ch))
    return 0;

  if (PROC_FIRED(ch))
    return FALSE;

  hp = GET_HIT(ch) * 100;
  hp /= GET_MAX_HIT(ch);
  if (hp < 40)
  {
    send_to_room(
        ch->in_room,
        "\tRThe Rot Bringer realizes the tide of the battle is turning against him, and he\tn\r\n"
        "\tRtakes a step towards the bloody basin. His face contorted in rage, he whispers\tn\r\n"
        "\tRsomething while clawing at the air over the floating bodies. Instantly, the red\tn\r\n"
        "\tRliquid starts swirling as the cadavers join together, forming a massive mound of\tn\r\n"
        "\tRmeat! A massive ball of flesh rises out of the basin, and follows its new "
        "master!\tn\r\n");

    mob = read_mobile(145193, VIRTUAL);
    char_to_room(mob, ch->in_room);
    add_follower(mob, ch);
    PROC_FIRED(ch) = TRUE;

    return TRUE;
  }
  return FALSE;
}

int ttf_path[] = {145185, 145184, 145183, 145184, 145186, 145187, 145186, 145188,
                  145189, 145188, 145186, 145187, 145186, 145184, 145185, -1};

SPECIAL(ttf_patrol)
{
  int dir = -1;
  // int next = 0;

  if (!ch)
    return 0;
  if (FIGHTING(ch))
    return 0;

  if (cmd)
    return 0;

  if (PATH_INDEX(ch) > 16 || PATH_INDEX(ch) < 0)
    PATH_INDEX(ch) = 0;

  // 8 second delay...
  if (PATH_DELAY(ch) > 0)
  {
    PATH_DELAY(ch)
    --;
    return 0;
  }
  PATH_DELAY(ch) = 8;

  PATH_INDEX(ch)
  ++;

  if (ttf_path[PATH_INDEX(ch)] == -1)
    PATH_INDEX(ch) = 0;

  dir = find_first_step(ch->in_room, real_room(ttf_path[PATH_INDEX(ch)]));
  if (dir >= 0)
    perform_move(ch, dir, 1);
  return 1;
}

/*****************/
/* End Temple of Twisted Flesh (TTF) */
/*****************/

/* put new zone procs here */
