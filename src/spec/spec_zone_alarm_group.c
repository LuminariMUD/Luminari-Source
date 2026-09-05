/**************************************************************************
 *  File: spec/spec_zone_alarm_group.c                 Part of LuminariMUD *
 *  Usage: Private shared alarm behavior and its zone procedure owners.    *
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
#include "magic/spells.h"
#include "actions.h"
#include "graph.h"
#include "mob/mob_utils.h"
#include "combat/fight.h"
#include "movement/movement_position.h"
#include "spec_zone_fire_plane.h"
#include "spec_zone_snake_pit.h"
#include "spec_zone_water_plane.h"

/* locally defined functions of local (file) scope */
static void zone_yell(struct char_data *ch, const char *buf);

/* this function will cause basically all the mobiles in the same zone
   to hunt someone down */
static void zone_yell(struct char_data *ch, const char *buf)
{
  struct char_data *i = NULL;
  struct char_data *vict = NULL;
  int num_targets = 0;

  if (!ch)
    return;

  if (IN_ROOM(ch) == NOWHERE)
    return;

  for (i = character_list; i; i = i->next)
  {
    if (IN_ROOM(i) == NOWHERE)
      continue;

    if (world[ch->in_room].zone == world[i->in_room].zone)
    {
      if (PROC_FIRED(ch) == FALSE)
      {
        send_to_char(i, "%s", buf);
      }

      if (i == ch || !IS_NPC(i))
        continue;

      if (((IS_EVIL(ch) && IS_EVIL(i)) || (IS_GOOD(ch) && IS_GOOD(i))) &&
          MOB_FLAGGED(i, MOB_HELPER))
      {
        if (i->in_room == ch->in_room && !FIGHTING(i))
        {
          for (vict = world[i->in_room].people; vict; vict = vict->next_in_room)
            if (FIGHTING(vict) == ch)
            {
              act("$n jumps to the aid of $N!", FALSE, i, 0, ch, TO_ROOM);
              hit(i, vict, TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, FALSE);
              break;
            }
        }
        else
        {
          /* retrieve random valid target and number of targets */
          if (!(vict = npc_find_target(ch, &num_targets)))
          {
            /* currently nothing to process if we can't find a target */
          }
          else
          {
            set_hunting_target(i, vict);
            hunt_victim(i);
          }
        }
      }
    }
  }
  PROC_FIRED(ch) = TRUE;
}

/* imix fireplane procs */
SPECIAL(imix)
{
  if (!ch)
    return FALSE;

  if (IN_ROOM(ch) == NOWHERE)
    return FALSE;

  if (GET_HIT(ch) <= 1)
    return FALSE;

  if (cmd || GET_POS(ch) == POS_DEAD)
    return FALSE;

  if (!FIGHTING(ch))
    PROC_FIRED(ch) = FALSE;

  if (FIGHTING(ch))
  {
    zone_yell(ch,
              "\r\n\tMImix \tnshouts, '\tRYou DARE attack me?!? Minions... to me now!!!\tn'\r\n");
  }

  if (!rand_number(0, 3) && FIGHTING(ch))
  {
    call_magic(ch, FIGHTING(ch), 0, SPELL_FIRE_BREATHE, 0, GET_LEVEL(ch), CAST_WEAPON_SPELL);
    return TRUE;
  }

  return FALSE;
}

/* olhydra procs */
SPECIAL(olhydra)
{
  struct char_data *vict;
  struct char_data *next_vict;

  if (cmd || GET_POS(ch) == POS_DEAD)
    return FALSE;

  if (!FIGHTING(ch))
    PROC_FIRED(ch) = FALSE;

  if (FIGHTING(ch))
  {
    zone_yell(
        ch, "\r\n\tBOlhydra \tnshouts, '\tCYou DARE attack me?!? Minions... to me now!!!\tn'\r\n");
  }

  if (!rand_number(0, 3) && FIGHTING(ch))
  {
    act("$n \tLopens $s mouth and let stream forth a \tBwave of water.\tn", FALSE, ch, 0, 0,
        TO_ROOM);

    for (vict = world[ch->in_room].people; vict; vict = next_vict)
    {
      next_vict = vict->next_in_room;
      if (IS_NPC(vict) && !IS_PET(vict))
        continue;
      if (ch == vict)
        continue;

      if ((dice(1, 20) + 21) < GET_DEX(vict))
      {
        act("\tbThe wave hits \tCYOU\tb, knocking you backwards.\tn", FALSE, ch, 0, vict, TO_VICT);
        act("\tbThe wave hits $N\tb, knocking $M backwards.\tn", FALSE, ch, 0, vict, TO_NOTVICT);
        USE_MOVE_ACTION(vict);
      }
      else
      {
        act("\tbThe wave hits \tCYOU\tb with full \tBforce\tb, knocking you down.\tn", FALSE, ch, 0,
            vict, TO_VICT);
        act("\tbThe wave hits $N\tb with full \tBforce\tb, knocking $M down.\tn", FALSE, ch, 0,
            vict, TO_NOTVICT);
        change_position(vict, POS_SITTING);
        USE_FULL_ROUND_ACTION(ch);
      }
    }
    return TRUE;
  }
  return FALSE;
}

/* from homeland */
SPECIAL(naga_golem)
{
  if (cmd || GET_POS(ch) == POS_DEAD)
    return FALSE;

  if (!FIGHTING(ch))
    PROC_FIRED(ch) = FALSE;

  if (FIGHTING(ch))
  {
    zone_yell(ch, "\r\n\tLThe golem rings an alarm bell, which echoes through "
                  "the pit.\tn\r\n");
    return TRUE;
  }

  return FALSE;
}

/* from homeland */
SPECIAL(naga)
{
  struct char_data *tch = 0;
  struct char_data *vict = 0;
  int dam = 0;

  if (cmd || GET_POS(ch) == POS_DEAD)
    return FALSE;
  if (rand_number(0, 3))
    return FALSE;
  if (!FIGHTING(ch))
    return FALSE;

  for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room)
  {
    if ((!IS_NPC(tch) || IS_PET(tch)) && !MOB_FLAGGED(tch, MOB_NOSLEEP))
    {
      if (!vict || !rand_number(0, 3))
      {
        vict = tch;
      }
    }
  }
  if (!vict)
    return FALSE;

  if (MOB_FLAGGED(vict, MOB_NOSLEEP))
    return FALSE;

  act("$n\tL thrusts its powerful barbed tail-stinger into your flesh causing\tn\r\n"
      "\tLyou to scream in agony.  As it snaps back its tail, poison oozes into the large\tn\r\n"
      "\tLwound that is opened.  You begin to fall into a drug induced "
      "sleep.\tn\r\n",
      FALSE, ch, 0, vict, TO_VICT);

  act("$n\tL thrusts its powerful barbed tail-stinger into $N's flesh causing\tn\r\n"
      "\tL$M\tL to scream in agony.  As it snaps back its tail, poison oozes into the large\tn\r\n"
      "\tLwound that is opened.  \tn$N\tL begin to fall into a drug "
      "induced sleep.\tn\r\n",
      TRUE, ch, 0, vict, TO_NOTVICT);

  act("\tLYour poison stinger hits $N!\tn", TRUE, ch, 0, vict, TO_CHAR);
  dam = GET_LEVEL(ch) * 2 + dice(2, GET_LEVEL(ch));
  if (dam > GET_HIT(vict))
    dam = GET_HIT(vict);
  if (dam < 0)
    dam = 0;
  combat_apply_raw_damage(vict, ch, dam, DAM_POISON, INT_MIN);
  stop_fighting(vict);
  change_position(vict, POS_SLEEPING);
  /* Would be best to make this an affect that affects your ability to wake up, lasting a couple rounds. */
  USE_FULL_ROUND_ACTION(vict);
  return TRUE;
}

/* from homeland */
SPECIAL(fp_invoker)
{
  struct char_data *victim;

  if (!ch)
    return FALSE;

  if (FIGHTING(ch))
    return FALSE;

  if (!IS_NPC(ch) && cmd && CMD_IS("cast") && GET_POS(ch) >= POS_FIGHTING)
  {
    victim = ch;
    ch = (struct char_data *)me;
    act("$n screams in rage, 'How DARE you cast a spell in my tower'", FALSE, ch, 0, 0, TO_ROOM);
    call_magic(ch, victim, 0, SPELL_MISSILE_STORM, 0, 30, CAST_SPELL);
    return FALSE;
  }

  return FALSE;
}
