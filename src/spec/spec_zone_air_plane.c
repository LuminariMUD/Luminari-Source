/**************************************************************************
 *  File: spec/spec_zone_air_plane.c                    Part of LuminariMUD *
 *  Usage: Air Plane zone procedures.                                      *
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
#include "spec_zone_air_plane.h"
#include "combat/fight.h"
#include "graph.h"

/* call allies to help yan */
bool yan_yell(struct char_data *ch)
{
  struct char_data *i;
  struct char_data *vict;
  struct descriptor_data *d;
  room_rnum room = 0;
  zone_rnum zone = world[ch->in_room].zone;
  room_rnum start = 0;
  room_rnum end = 0;
  vict = FIGHTING(ch);

  if (!vict)
    return FALSE;

  // show yan-s yell message.
  if (PROC_FIRED(ch) == false)
  {
    for (d = descriptor_list; d; d = d->next)
    {
      if (STATE(d) == CON_PLAYING && d->character != NULL && IN_ROOM(d->character) != NOWHERE &&
          IN_ROOM(d->character) <= top_of_world && zone == world[d->character->in_room].zone)
      {
        send_to_char(d->character, "\tcYan-C-Bin the Master of Evil Air\tw shouts, '\tcI "
                                   "have been attacked! Come to me minions!\tw'\tn\r\n");
      }
    }
  }

  start = real_room(136100);
  end = real_room(136224);
  if (start == NOWHERE || end == NOWHERE || start > end)
  {
    log("SYSERR: yan_yell could not resolve its reinforcement room range.");
    return FALSE;
  }

  for (room = start; room <= end; room++)
  {
    for (i = world[room].people; i; i = i->next_in_room)
    {
      if (IS_NPC(i) && !FIGHTING(i))
      {
        switch (GET_MOB_VNUM(i))
        {
        case 136110:
        case 136111:
        case 136112:
        case 136113:
          if (i->in_room == ch->in_room)
          {
            act("$n jumps to the aid of $N!", FALSE, i, 0, ch, TO_ROOM);
            hit(i, vict, TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, FALSE);
          }
          else
          {
            HUNTING(i) = ch;
            hunt_victim(i);
          }
          break;
        }
      }
    }
  }

  if (PROC_FIRED(ch) == FALSE)
  {
    PROC_FIRED(ch) = TRUE;
    return TRUE;
  }

  return FALSE;
}

/* yan damage proc */
void yan_maelstrom(struct char_data *ch)
{
  struct char_data *vict;
  struct char_data *next_vict;
  int dam = 0;

  act("$N \tcbegins to spin in a circular motion, gathering speed at an alarming pace.\tn\r\n"
      "\tcAs the pace quickens, $E begins to gain in height as well, until $E forms into\tn\r\n"
      "\tcan eighty-foot tall whirlwind \tcof \tCs\tcw\twi\tcr\tCl\tci\twn\tCg \tcchaos.\tn",
      FALSE, ch, 0, ch, TO_ROOM);

  for (vict = world[ch->in_room].people; vict; vict = next_vict)
  {
    next_vict = vict->next_in_room;
    if (IS_NPC(vict) && !IS_PET(vict))
      continue;
    dam = 150 + dice(10, 20);
    if (GET_LEVEL(vict) < 20)
      dam = GET_MAX_HIT(vict);
    if (dam >= GET_HIT(vict))
    {
      dam += 25;
      act("\twAs you are spun about by $n's \twmaelstrom, your body is damaged beyond repair.\tn",
          FALSE, ch, 0, vict, TO_VICT);
      act("\twAs $N is spun about by $n's \twmaelstrom, $M body is damaged beyond repair.\tn",
          FALSE, ch, 0, vict, TO_NOTVICT);
    }
    else
    {
      act("\twYou are enveloped in $n's \tCs\tcw\twi\tcr\tCl\tci\twn\tCg \tcmaelstrom\tw, your "
          "body pelted by \twgusts\tc of wind.\tn",
          FALSE, ch, 0, vict, TO_VICT);
      act("\tw$N is enveloped in $n's \tCs\tcw\twi\tcr\tCl\tci\twn\tCg \tcmaelstrom\tw, $S body "
          "pelted by \twgusts\tc of wind.\tn",
          FALSE, ch, 0, vict, TO_NOTVICT);
    }
    damage(ch, vict, dam, -1, DAM_AIR, FALSE); // type -1 = no dam msg
  }
}

/* yan windgust */
void yan_windgust(struct char_data *ch)
{
  struct char_data *vict;
  struct char_data *next_vict;
  int dam = 0;
  struct affected_type af;

  act("\tc$n\tc opens $s cavernous maw and sends forth a \tCpowerful \twgust\tc of air.\tn", FALSE,
      ch, 0, 0, TO_ROOM);

  for (vict = world[ch->in_room].people; vict; vict = next_vict)
  {
    next_vict = vict->next_in_room;
    if (IS_NPC(vict) && !IS_PET(vict))
      continue;

    dam = 30 + dice(3, 30);
    if (dam > GET_HIT(vict))
    {
      dam += 25;

      act("\twAs you are hit by the \tcgust\tw of wind sent by $n, \twyou feel your\r\n"
          "\twlife slip away.\tn",
          FALSE, ch, 0, vict, TO_VICT);
      act("\tw$N is blasted by $n's \tcgust\tw of wind, and suddenly keels over from\r\n"
          "\twthe damage.\tn",
          FALSE, ch, 0, vict, TO_NOTVICT);
      damage(ch, vict, dam, -1, DAM_AIR, FALSE); // type -1 = no dam msg
    }
    else
    {
      act("\twYou are blasted by a \tCf\tci\twer\tcc\tCe\tc gust\tw of wind hurled by $n.\tn",
          FALSE, ch, 0, vict, TO_VICT);
      act("\tw$N is blasted by a \tCf\tci\twer\tcc\tCe\tc gust\tw of wind hurled by $n.\tn", FALSE,
          ch, 0, vict, TO_NOTVICT);
      damage(ch, vict, dam, -1, DAM_AIR, FALSE); //-1 type = no dam mess
      if (dice(1, 40) > GET_CON(vict) && can_stun(vict))
      {
        new_affect(&af);
        af.spell = SKILL_CHARGE;
        SET_BIT_AR(af.bitvector, AFF_STUN);
        af.duration = dice(2, 4) + 1;
        affect_join(vict, &af, TRUE, FALSE, FALSE, FALSE);
      }
    }
  }
}

/* chan calls allies */
bool chan_yell(struct char_data *ch)
{
  struct char_data *i;
  struct char_data *vict;
  struct descriptor_data *d;
  room_rnum room = 0;
  zone_rnum zone = world[ch->in_room].zone;
  room_rnum start = 0;
  room_rnum end = 0;

  vict = FIGHTING(ch);
  if (!vict)
    return FALSE;

  // show yan-s yell message.
  if (PROC_FIRED(ch) == false)
  {
    for (d = descriptor_list; d; d = d->next)
    {
      if (STATE(d) == CON_PLAYING && d->character != NULL && IN_ROOM(d->character) != NOWHERE &&
          IN_ROOM(d->character) <= top_of_world && zone == world[d->character->in_room].zone)
      {
        send_to_char(d->character, "\tCChan, the Elemental Princess of Good Air\tw shouts, '\tcI "
                                   "have been attacked! Come to me my friends!\tw'\tn\r\n");
      }
    }
  }

  start = real_room(136100);
  end = real_room(136224);
  if (start == NOWHERE || end == NOWHERE || start > end)
  {
    log("SYSERR: chan_yell could not resolve its reinforcement room range.");
    return FALSE;
  }

  for (room = start; room <= end; room++)
  {
    for (i = world[room].people; i; i = i->next_in_room)
    {
      if (IS_NPC(i) && !FIGHTING(i))
      {
        switch (GET_MOB_VNUM(i))
        {
        case 136115:
        case 136116:
        case 136117:
        case 136118:
          if (i->in_room == ch->in_room)
          {
            act("$n jumps to the aid of $N!", FALSE, i, 0, ch, TO_ROOM);
            hit(i, vict, TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, FALSE);
          }
          else
          {
            HUNTING(i) = ch;
            hunt_victim(i);
          }
          break;
        }
      }
    }
  }

  if (PROC_FIRED(ch) == FALSE)
  {
    PROC_FIRED(ch) = TRUE;
    return TRUE;
  }

  return FALSE;
}

/* from homeland */
SPECIAL(yan)
{
  if (cmd || GET_POS(ch) == POS_DEAD)
    return FALSE;

  if (!FIGHTING(ch))
    PROC_FIRED(ch) = FALSE;

  if (FIGHTING(ch))
  {
    if (yan_yell(ch))
      return TRUE;
    if (!rand_number(0, 50))
    {
      yan_maelstrom(ch);
      return TRUE;
    }
    if (!rand_number(0, 3))
    {
      yan_windgust(ch);
      return TRUE;
    }
  }
  return FALSE;
}

/* from homeland */
SPECIAL(chan)
{
  if (cmd || GET_POS(ch) == POS_DEAD)
    return FALSE;

  if (!FIGHTING(ch))
    PROC_FIRED(ch) = FALSE;

  if (FIGHTING(ch))
  {
    if (chan_yell(ch))
      return TRUE;
    if (!rand_number(0, 3))
    {
      yan_windgust(ch);
      return TRUE;
    }
  }
  return FALSE;
}
