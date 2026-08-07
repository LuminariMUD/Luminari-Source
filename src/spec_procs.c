/**************************************************************************
 *  File: spec_procs.c                                 Part of LuminariMUD *
 *  Usage: Implementation of special procedures for mobiles/objects/rooms. *
 *                                                                         *
 *  All rights reserved.  See license for complete information.            *
 *                                                                         *
 *  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
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
#include "constants.h"
#include "act.h"
#include "spec_procs.h"
#include "character/class.h"
#include "combat/fight.h"
#include "modify.h"
#include "obj/house.h"
#include "clan.h"
#include "mudlim.h"
#include "graph.h"
#include "dgscript/dg_scripts.h" /* for send_to_zone() */
#include "mud_event.h"
#include "actions.h"
#include "combat/assign_wpn_armor.h"
#include "magic/domains_schools.h"
#include "character/feats.h"
#include "magic/spell_prep.h"
#include "obj/item.h" /* do_stat_object */
#include "craft/alchemy.h"
#include "obj/treasure.h"     /* for set_armor_object */
#include "mob/mob_utils.h"    /* npc_find_target() */
#include "magic/spell_prep.h" /* for star circlet proc */
#include "handler.h"          /* for is_name() */
#include "character/evolutions.h"
#include "olc/oasis.h"
#include "quest/quest.h"
#include "character/backgrounds.h"
#include "character/perks.h"
#include "vessels/vessels.h"


/* locally defined functions of local (file) scope */
static void zone_yell(struct char_data *ch, const char *buf);

/********************************************************************/
/******************** Mobile Procs    *******************************/
/********************************************************************/

/*************************************************/
/**** General special procedures for mobiles. ****/
/*************************************************/

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
            HUNTING(i) = vict;
            hunt_victim(i);
          }
        }
      }
    }
  }
  PROC_FIRED(ch) = TRUE;
}

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

/****************************************************/
/******* end general procedures for mobile procs ****/
/****************************************************/

/****************************/
/** begin actual mob procs **/
/****************************/

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

/* from homeland */
SPECIAL(feybranche)
{
  struct char_data *i = NULL;
  char buf[MAX_INPUT_LENGTH] = {'\0'};

  if (cmd || GET_POS(ch) == POS_DEAD)
    return FALSE;

  struct char_data *enemy = FIGHTING(ch);

  if (!enemy)
    PROC_FIRED(ch) = FALSE;

  if (FIGHTING(ch) && !ROOM_FLAGGED(ch->in_room, ROOM_SOUNDPROOF))
  {
    if (enemy->master && enemy->master->in_room == enemy->in_room)
      enemy = enemy->master;
    snprintf(buf, sizeof(buf),
             "%s\tL shouts, '\tmCome to me!!' Fey-Branche is under attack!\tn\r\n",
             ch->player.short_descr);
    for (i = character_list; i; i = i->next)
    {
      if (!FIGHTING(i) && IS_NPC(i) &&
          (GET_MOB_VNUM(i) == 135535 || GET_MOB_VNUM(i) == 135536 || GET_MOB_VNUM(i) == 135537 ||
           GET_MOB_VNUM(i) == 135538 || GET_MOB_VNUM(i) == 135539 || GET_MOB_VNUM(i) == 135540) &&
          ch != i)
      {
        if (FIGHTING(ch)->in_room != i->in_room)
        {
          if (GET_MOB_VNUM(i) != 135536)
          {
            HUNTING(i) = enemy;
            hunt_victim(i);
          }
          else
            cast_spell(i, enemy, 0, SPELL_TELEPORT, 0);
        }
        else
          hit(i, enemy, TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, FALSE);
      }

      if (world[ch->in_room].zone == world[i->in_room].zone && !PROC_FIRED(ch))
        send_to_char(i, "%s", buf);
    }
    PROC_FIRED(ch) = TRUE;
    return TRUE;
  }
  return FALSE;
}

/* from homeland */
SPECIAL(abyssal_vortex)
{
  int temp;

  if (cmd)
    return FALSE;

  if (IS_NPC(ch) && !IS_PET(ch))
    return FALSE;

  if (!rand_number(0, 7))
  {
    temp = world[ch->in_room].dir_option[0]->to_room;
    world[ch->in_room].dir_option[0]->to_room = world[ch->in_room].dir_option[1]->to_room;
    world[ch->in_room].dir_option[1]->to_room = world[ch->in_room].dir_option[4]->to_room;
    world[ch->in_room].dir_option[4]->to_room = world[ch->in_room].dir_option[3]->to_room;
    world[ch->in_room].dir_option[3]->to_room = world[ch->in_room].dir_option[5]->to_room;
    world[ch->in_room].dir_option[5]->to_room = world[ch->in_room].dir_option[2]->to_room;
    world[ch->in_room].dir_option[2]->to_room = temp;

    send_to_room(ch->in_room,
                 "\tLThe reality seems to \tCshift\tL as madness descends in the \tcvortex\tn\r\n");

    return TRUE;
  }
  return FALSE;
}

/* from homeland */
SPECIAL(agrachdyrr)
{
  struct char_data *i = NULL;
  char buf[MAX_INPUT_LENGTH] = {'\0'};

  if (cmd || GET_POS(ch) == POS_DEAD)
    return FALSE;

  struct char_data *enemy = FIGHTING(ch);

  if (!enemy)
    PROC_FIRED(ch) = FALSE;

  if (FIGHTING(ch) && !ROOM_FLAGGED(ch->in_room, ROOM_SOUNDPROOF))
  {
    if (!rand_number(0, 4) && !ch->followers)
    {
      act("$n\tL looks to be extremely disspleased at being\r\n"
          "forced to fight such inferior beings in her own mansion. She raises her\r\n"
          "arms and cries out, '\tmAid me Lloth!\tL'",
          FALSE, ch, 0, 0, TO_ROOM);

      struct char_data *mob = read_mobile(135523, VIRTUAL);
      if (!mob)
        return FALSE;
      char_to_room(mob, ch->in_room);
      add_follower(mob, ch);
      return TRUE;
    }

    if (enemy->master && enemy->master->in_room == enemy->in_room)
      enemy = enemy->master;

    snprintf(buf, sizeof(buf),
             "%s\tL shouts, '\twTo me, \tcAgrach-Dyrr\tw is under attack!'\tn\r\n",
             ch->player.short_descr);
    for (i = character_list; i; i = i->next)
    {
      if (!FIGHTING(i) && IS_NPC(i) &&
          (GET_MOB_VNUM(i) == 135521 || GET_MOB_VNUM(i) == 135522 || GET_MOB_VNUM(i) == 135510 ||
           GET_MOB_VNUM(i) == 135524 || GET_MOB_VNUM(i) == 135525 || GET_MOB_VNUM(i) == 135512) &&
          ch != i)
      {
        if (FIGHTING(ch)->in_room != i->in_room)
        {
          if (GET_MOB_VNUM(i) != 135522)
          {
            HUNTING(i) = enemy;
            hunt_victim(i);
          }
          else
            cast_spell(i, enemy, 0, SPELL_TELEPORT, 0);
        }
        else
          hit(i, enemy, TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, FALSE);
      }

      if (world[ch->in_room].zone == world[i->in_room].zone && !PROC_FIRED(ch))
        send_to_char(i, "%s", buf);
    }
    PROC_FIRED(ch) = TRUE;
    return TRUE;
  } // for loop
  return FALSE;
}

/* from homeland */
SPECIAL(shobalar)
{
  struct char_data *i = NULL;
  char buf[MAX_INPUT_LENGTH] = {'\0'};

  if (cmd || GET_POS(ch) == POS_DEAD)
    return FALSE;

  struct char_data *enemy = FIGHTING(ch);

  if (!enemy)
    PROC_FIRED(ch) = FALSE;

  if (FIGHTING(ch) && !ROOM_FLAGGED(ch->in_room, ROOM_SOUNDPROOF))
  {
    if (enemy->master && enemy->master->in_room == enemy->in_room)
      enemy = enemy->master;
    snprintf(buf, sizeof(buf), "%s\tL shouts, '\twTo me, \tmShobalar\tw is under attack!'\tn\r\n",
             ch->player.short_descr);
    for (i = character_list; i; i = i->next)
    {
      if (!FIGHTING(i) && IS_NPC(i) &&
          (GET_MOB_VNUM(i) == 135506 || GET_MOB_VNUM(i) == 135500 || GET_MOB_VNUM(i) == 135504 ||
           GET_MOB_VNUM(i) == 135507) &&
          ch != i)
      {
        if (FIGHTING(ch)->in_room != i->in_room)
        {
          if (GET_MOB_VNUM(i) != 135506)
          {
            HUNTING(i) = enemy;
            hunt_victim(i);
          }
          else
            cast_spell(i, enemy, NULL, SPELL_TELEPORT, 0);
        }
        else
          hit(i, enemy, TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, FALSE);
      }

      if (world[ch->in_room].zone == world[i->in_room].zone && !PROC_FIRED(ch))
        send_to_char(i, "%s", buf);
    }
    PROC_FIRED(ch) = TRUE;
    return TRUE;
  } // for loop

  return FALSE;
}

/* from homeland */
SPECIAL(ogremoch)
{
  struct char_data *i;
  struct char_data *vict;
  struct descriptor_data *d;
  room_rnum room = 0;
  zone_rnum zone = world[ch->in_room].zone;
  room_rnum start = 0;
  room_rnum end = 0;

  if (cmd || GET_POS(ch) == POS_DEAD)
    return FALSE;

  if (!FIGHTING(ch))
    PROC_FIRED(ch) = FALSE;

  vict = FIGHTING(ch);
  if (!vict)
    return FALSE;

  // show yell message.
  if (PROC_FIRED(ch) == false)
  {
    for (d = descriptor_list; d; d = d->next)
    {
      if (STATE(d) == CON_PLAYING && d->character != NULL && IN_ROOM(d->character) != NOWHERE &&
          IN_ROOM(d->character) <= top_of_world && zone == world[d->character->in_room].zone)
      {
        send_to_char(d->character, "\tLOgremoch \tw shouts, '\tLI have been "
                                   "attacked! Come to me minions!\tw'\tn\r\n");
      }
    }
  }

  start = real_room(136700);
  end = real_room(136802);
  if (start == NOWHERE || end == NOWHERE || start > end)
  {
    log("SYSERR: ogremoch could not resolve its reinforcement room range.");
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
        case 136703:
        case 136704:
        case 136705:
        case 136706:
        case 136707:
        case 136708:
        case 136709:
          if (i->in_room == ch->in_room)
          {
            act("$n jumps to the aid of $N!", FALSE, i, 0, ch, TO_ROOM);
            hit(i, vict, TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, FALSE);
          }
          else
          {
            // either melt in directly or track
            if (dice(1, 10) < 2)
            {
              act("$n jumps into the pure rock, as $s lord calls for $m.", FALSE, i, 0, 0, TO_ROOM);
              char_from_room(i);
              char_to_room(i, ch->in_room);
              act("$n comes out from the rock, to help $s lord.", FALSE, i, 0, 0, TO_ROOM);
            }
            else
            {
              HUNTING(i) = ch;
              hunt_victim(i);
            }
          }
          break;
        }
      }
    }
  }
  PROC_FIRED(ch) = TRUE;

  return TRUE;
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

/* General guild and mobile procedures are implemented by their feature owners. */

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
  GET_HIT(vict) -= dam;
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

/* from homeland */
SPECIAL(illithid_gguard)
{
  const char *buf = "$N \tLsteps in front of you, blocking you from accessing the gate.\tn";
  const char *buf2 = "$N \tLsteps in front of $n\tL, blocking access the gate.\tn";

  if (!IS_MOVE(cmd))
    return FALSE;

  // if (cmd == SCMD_EAST && GET_RACE(ch) != RACE_ILLITHID) {
  if (cmd == SCMD_EAST)
  {
    act(buf, FALSE, ch, 0, (struct char_data *)me, TO_CHAR);
    act(buf2, FALSE, ch, 0, (struct char_data *)me, TO_ROOM);
    return TRUE;
  }

  return FALSE;
}

/* from homeland */
SPECIAL(duergar_guard)
{
  const char *buf = "$N steps into the opening and blocks your path.\r\n";
  const char *buf2 = "$N steps into the opening blocking it.";

  if (!IS_MOVE(cmd))
    return FALSE;

  if (cmd == SCMD_DOWN)
  {
    act(buf, FALSE, ch, 0, (struct char_data *)me, TO_CHAR);
    act(buf2, FALSE, ch, 0, (struct char_data *)me, TO_ROOM);
    return TRUE;
  }

  return FALSE;
}

/* from homeland */
SPECIAL(bandit_guard)
{
  const char *buf = "$N blocks your access into the castle.\r\n";
  const char *buf2 = "$N blocks $n's access into the castle..";

  if (!IS_MOVE(cmd))
    return FALSE;

  if (GET_LEVEL(ch) < 12)
    return FALSE;

  if (cmd == SCMD_EAST || cmd == SCMD_SOUTH || cmd == SCMD_WEST)
  {
    act(buf, FALSE, ch, 0, (struct char_data *)me, TO_CHAR);
    act(buf2, FALSE, ch, 0, (struct char_data *)me, TO_ROOM);
    return TRUE;
  }

  return FALSE;
}

/* from homeland */
SPECIAL(secomber_guard)
{
  const char *buf =
      "\tLThe doorguard steps before you, blocking your way with an upraised hand.\tn\r\n";
  const char *buf2 =
      "\tLThe doorguard blocks \tn$n\tL's way, placing one meaty hand on $s chest.\tn";

  if (!IS_MOVE(cmd))
    return FALSE;

  if (cmd == SCMD_EAST)
  {
    send_to_char(ch, "%s", buf);
    act(buf2, FALSE, ch, 0, 0, TO_ROOM);
    return TRUE;
  }

  return FALSE;
}

/* from homeland */
/*
SPECIAL(guild_golem) {
  bool found = TRUE;
  const char *msg1 = "The golem humiliates you, and blocks your way.\r\n";
  const char *msg2 = "The golem humiliates $n, and blocks $s way.";

  if (!IS_MOVE(cmd))
    return FALSE;

  int i = cmd - 1;

  if (i < 0) {
    send_to_char("Index error in guild golem\r\n", ch);
    return FALSE;
  }

  if (!EXIT(ch, i))
    found = FALSE;
  else {
    int room_number = world[ch->in_room].dir_option[i]->to_room;
    if (world[room_number].guild_index) {
      if (GET_GUILD(ch) != world[room_number].guild_index && GET_ALT(ch) != world[room_number].guild_index)
        found = FALSE;
    }
  }

  if (!found) {
    send_to_char(msg1, ch);
    act(msg2, FALSE, ch, 0, 0, TO_ROOM);
    return TRUE;
  }

  return FALSE;
}
 */

/* from Homeland */
/*
SPECIAL(guild_guard) {
  int i;
  bool found = TRUE;
  const char *buf = "The guard humiliates you, and blocks your way.\r\n";
  const char *buf2 = "The guard humiliates $n, and blocks $s way.";

  if (!IS_MOVE(cmd))
    return FALSE;

  if (GET_LEVEL(ch) >= LVL_IMMORT)
    return FALSE;

  for (i = 0; guild_info[i][0] != -1; i++) {
    if (GET_ROOM_VNUM(IN_ROOM(ch)) == guild_info[i][1] &&
            cmd == guild_info[i][2]) {
      if (IS_NPC(ch) || GET_CLASS(ch) != guild_info[i][0]) {
        found = FALSE;
      } else {
        found = TRUE;
        break;
      }
    }
  }

  if (!found) {
    send_to_char(buf, ch);
    act(buf2, FALSE, ch, 0, 0, TO_ROOM);
    return TRUE;
  }

  return FALSE;
}
 */

/* from homeland */
SPECIAL(harpell)
{
  struct char_data *i = NULL;
  char buf[MAX_INPUT_LENGTH] = {'\0'};

  if (cmd || GET_POS(ch) == POS_DEAD)
    return FALSE;

  if (!FIGHTING(ch))
    PROC_FIRED(ch) = FALSE;

  if (FIGHTING(ch) && !ROOM_FLAGGED(ch->in_room, ROOM_SOUNDPROOF))
  {
    if (AFF_FLAGGED(FIGHTING(ch), AFF_CHARM) && FIGHTING(ch)->master)
      snprintf(buf, sizeof(buf),
               "%s shouts, 'HELP! %s has ordered his pets to kill "
               "me!!'\r\n",
               ch->player.short_descr, GET_NAME(FIGHTING(ch)->master));
    else
      snprintf(buf, sizeof(buf), "%s shouts, 'HELP! %s is trying to kill me!\r\n",
               ch->player.short_descr, GET_NAME(FIGHTING(ch)));
    for (i = character_list; i; i = i->next)
    {
      if (!FIGHTING(i) && IS_NPC(i) &&
          (GET_MOB_VNUM(i) == 106831 || GET_MOB_VNUM(i) == 106841 || GET_MOB_VNUM(i) == 106842 ||
           GET_MOB_VNUM(i) == 106844 || GET_MOB_VNUM(i) == 106845 || GET_MOB_VNUM(i) == 106846) &&
          ch != i && !rand_number(0, 2))
      {
        if (AFF_FLAGGED(FIGHTING(ch), AFF_CHARM) && FIGHTING(ch)->master &&
            (FIGHTING(ch)->master->in_room != FIGHTING(ch)->in_room))
        {
          if (FIGHTING(ch)->master->in_room != i->in_room)
            cast_spell(i, FIGHTING(ch)->master, NULL, SPELL_TELEPORT, 0);
          else
            hit(i, FIGHTING(ch)->master, TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, FALSE);
        }
        else
        {
          if (FIGHTING(ch)->in_room != i->in_room)
            cast_spell(i, FIGHTING(ch), NULL, SPELL_TELEPORT, 0);
          else
            hit(i, FIGHTING(ch), TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, FALSE);
        }
      }

      if (world[ch->in_room].zone == world[i->in_room].zone && !PROC_FIRED(ch))
        send_to_char(i, "%s", buf);
    }
    PROC_FIRED(ch) = TRUE;
    return TRUE;
  } // for loop

  return FALSE;
}


/* from homeland */
SPECIAL(wallach)
{
  if (cmd || GET_POS(ch) == POS_DEAD)
    return FALSE;

  if (GET_ROOM_VNUM(GET_MOB_LOADROOM(ch)) != 112638)
    GET_MOB_LOADROOM(ch) = real_room(112638);

  return FALSE;
}

/* from homeland */
SPECIAL(beltush)
{
  struct char_data *i;

  if (cmd || GET_POS(ch) == POS_DEAD || GET_ROOM_VNUM(ch->in_room) != 112648)
    return FALSE;

  for (i = character_list; i; i = i->next)
    if (!IS_NPC(i) && GET_ROOM_VNUM(i->in_room) == 112602)
    {
      do_enter(ch, "mirror", 0, 0);
      act("Beltush says, 'FOOLS!! How dare you attempt to enter the flaming "
          "tower!!",
          FALSE, ch, 0, 0, TO_ROOM);
      return TRUE;
    }

  return FALSE;
}

/* from homeland */
SPECIAL(mereshaman)
{
  if (cmd)
    return FALSE;

  if (FIGHTING(ch) && !PROC_FIRED(ch))
  {
    PROC_FIRED(ch) = TRUE;
    send_to_room(ch->in_room,
                 "\tLThe \tglizardman \tLshaman chants loudly, '\tGUktha slithiss "
                 "Semuanya! Ssithlarss sunggar uk!\tL'\tn\r\n"
                 "\tLThe monitor lizard statues shudder and vibrate then take on \tn\r\n"
                 "\tLa \tGbright green glow\tL. Each opens up like a cocoon releasing the\tn\r\n"
                 "\tLreptilian beast contained within.\tn\r\n");

    char_to_room(read_mobile(126725, VIRTUAL), ch->in_room);
    char_to_room(read_mobile(126725, VIRTUAL), ch->in_room);
    char_to_room(read_mobile(126725, VIRTUAL), ch->in_room);
    char_to_room(read_mobile(126725, VIRTUAL), ch->in_room);
    return TRUE;
  }
  return FALSE;
}


/* from homeland */
SPECIAL(battlemaze_guard)
{
  const char *buf = "$N \tL tells you, 'You don't want to go any farther, young one. \tn\r\n"
                    "\tL You must be at least level ten to go into the more advanced\tn\r\n"
                    "\tL parts of the battlemaze.'\tn";
  const char *buf2 = "$N \tLsteps in front of $n\tL, blocking access the gate.\tn";

  if (!IS_MOVE(cmd))
    return FALSE;

  if (cmd == SCMD_NORTH && GET_LEVEL(ch) < 10)
  {
    act(buf, FALSE, ch, 0, (struct char_data *)me, TO_CHAR);
    act(buf2, FALSE, ch, 0, (struct char_data *)me, TO_ROOM);
    return TRUE;
  }

  return FALSE;
}

/* from homeland */
SPECIAL(willowisp)
{
  room_rnum room = real_room(126899);

  if (cmd)
    return FALSE;

  if (FIGHTING(ch))
    return FALSE;

  if (ch->in_room != room && weather_info.sunlight == SUN_LIGHT)
  {
    act("$n fades away in the sunlight!", FALSE, ch, 0, 0, TO_ROOM);
    ch->mob_specials.temp_room_data = ch->in_room;
    char_from_room(ch);
    char_to_room(ch, room);

    return TRUE;
  }

  if (ch->in_room == room && weather_info.sunlight != SUN_LIGHT)
  {
    char_from_room(ch);
    char_to_room(ch, ch->mob_specials.temp_room_data);
    act("$n appears with the dark of the night!", FALSE, ch, 0, 0, TO_ROOM);
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
  GET_HIT(vict) -= dam;
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

/* from homeland */
static bool gr_stalled = FALSE;

SPECIAL(gromph)
{
  struct char_data *victim;
  int dir = -1;

  if (!ch)
    return FALSE;

  if (FIGHTING(ch))
    return FALSE;

  if (!IS_NPC(ch) && cmd && CMD_IS("cast"))
  {
    victim = ch;
    ch = (struct char_data *)me;
    act("$n sighs at YOU and mutters, 'You insolent worm!'", FALSE, ch, 0, victim, TO_VICT);
    act("$n sighs at $N, 'You insolent worm!'", FALSE, ch, 0, victim, TO_NOTVICT);
    call_magic(ch, victim, 0, SPELL_MISSILE_STORM, 0, 30, CAST_WEAPON_SPELL);
    return TRUE;
  }

  if (PATH_DELAY(ch) > 0)
    PATH_DELAY(ch)
  --;
  PATH_DELAY(ch) = 4;

  if (cmd)
    return FALSE;

  {
    switch (PROC_FIRED(ch))
    {
    case 0:
      // move to sorcere
      dir = find_first_step(ch->in_room, real_room(135250));
      if (dir < 0)
        PROC_FIRED(ch) = 1;
      break;
    case 1:
      // move to narbondel
      dir = find_first_step(ch->in_room, real_room(135353));

      if (dir < 0)
      {
        if (time_info.hours == 0 && gr_stalled == TRUE)
        {
          send_to_zone(
              "\tLSuddenly the base of the gigantic rockpillar known as \trNar\tRbon\trdel\tL\r\n"
              "\tLlights up with intense \trheat\tL, as Gromph Baenre uses his magic to relit it "
              "to\r\n"
              "\tLmark the start of a new day in the city.\tn\r\n",
              ch->in_room);
          gr_stalled = FALSE;
          PROC_FIRED(ch) = 0;
        }
        else
          gr_stalled = TRUE;
      }
      break;
    }
    if (dir >= 0)
      perform_move(ch, dir, 1);
    return TRUE;
  }
  return FALSE;
}

/*************************/
/* end mobile procedures */
/*************************/

/********************************************************************/
/******************** Room Procs      *******************************/
/********************************************************************/

/*
SPECIAL(emporium) {

  if (!CMD_IS("emporium"))
    return FALSE;

  char arg[200] = {'\0'}, arg2[200] = {'\0'}, arg3[200] = {'\0'}, arg4[200] = {'\0'};
  int bt = 0, bonus = 0;

  one_argument(one_argument(one_argument(one_argument(argument, arg), arg2), arg3), arg4);

  if (!*arg) {
    send_to_char(ch, "The syntax for this command is: 'item buy <vnum> <bonustype> "
            "<bonus>' or 'item list <weapons|armor|other> <bonustype> <bonus>'.\r\n");
    send_to_char(ch, "\tYPlease note that bonuses of the same type \tRDO NOT\tY "
            "stack in d20 rules.\tn\r\n");
    return TRUE;
  }

  if (is_abbrev(arg, "buy")) {
    if (!*arg2) {
      send_to_char(ch, "Please specify the vnum of the item you wish to buy.  "
              "You may obtain the vnum from the 'item list' command.\r\n");
      send_to_char(ch, "\tYPlease note that bonuses of the same type \tRDO NOT\tY "
              "stack in d20 rules.\tn\r\n");
      return TRUE;
    }

    int vnum = atoi(arg2);

    if (!((vnum >= 30000 && vnum <= 30083) || (vnum >= 30085 && vnum <= 30092) ||
            vnum == 30095 || (vnum >= 30100 && vnum <= 30105))) {
      send_to_char(ch, "That is not a valid vnum.  Please select again.\r\n");
      return TRUE;
    }

    struct obj_data *obj = read_object(vnum, VIRTUAL);

    if (!obj) {
      send_to_char(ch, "There was an error buying your item.  Please inform a staff "
              "member with error code ITM_BUY_001.\r\n");
      return TRUE;
    }

    if (GET_OBJ_TYPE(obj) != ITEM_WEAPON && GET_OBJ_TYPE(obj) != ITEM_ARMOR &&
            GET_OBJ_TYPE(obj) != ITEM_WORN) {
      send_to_char(ch, "That is not a valid vnum.  Please select again.\r\n");
      send_to_char(ch, "\tYPlease note that bonuses of the same type \tRDO NOT\tY stack "
              "in d20 rules.\tn\r\n");
      return TRUE;
    }

    if (!*arg3) {
      send_to_char(ch, list_bonus_types());
      send_to_char(ch, "\tYPlease note that bonuses of the same type \tRDO NOT\tY "
              "stack in d20 rules.\tn\r\n");
      return TRUE;
    }

    if ((bt = get_bonus_type_int(arg3)) == 0) {
      send_to_char(ch, "That's an invalid bonus type.\r\n\r\n");
      send_to_char(ch, list_bonus_types());
      send_to_char(ch, "\tYPlease note that bonuses of the same type \tRDO NOT\tY "
              "stack in d20 rules.\tn\r\n");
      return TRUE;
    }

    if (bt == APPLY_ACCURACY && GET_OBJ_TYPE(obj) != ITEM_WEAPON) {
      send_to_char(ch, "Only weapons can be given that bonus.\r\n");
      send_to_char(ch, "\tYPlease note that bonuses of the same type \tRDO NOT\tY "
              "stack in d20 rules.\tn\r\n");
      return TRUE;
    }

    if (bt == APPLY_AC_ARMOR && !CAN_WEAR(obj, ITEM_WEAR_BODY)) {
      send_to_char(ch, "Only body armor can be given that bonus.\r\n");
      send_to_char(ch, "\tYPlease note that bonuses of the same type \tRDO NOT\tY "
              "stack in d20 rules.\tn\r\n");
      return TRUE;
    }

    if (bt == APPLY_AC_SHIELD && !CAN_WEAR(obj, ITEM_WEAR_SHIELD)) {
      send_to_char(ch, "Only shields can be given that bonus.\r\n");
      send_to_char(ch, "\tYPlease note that bonuses of the same type \tRDO NOT\tY "
              "stack in d20 rules.\tn\r\n");
      return TRUE;
    }

    if (!*arg4) {
      send_to_char(ch, "How much would you like the bonus to be?\r\n");
      send_to_char(ch, "\tYPlease note that bonuses of the same type \tRDO NOT\tY "
              "stack in d20 rules.\tn\r\n");
      return TRUE;
    }

    if ((bonus = atoi(arg4)) <= 0) {
      send_to_char(ch, "The bonus must be greater than 0.\r\n");
      send_to_char(ch, "\tYPlease note that bonuses of the same type \tRDO NOT\tY "
              "stack in d20 rules.\tn\r\n");
      return TRUE;
    }

    if ((bonus = atoi(arg4)) > 100) {
      send_to_char(ch, "The bonus must be less than 100.\r\n");
      send_to_char(ch, "\tYPlease note that bonuses of the same type \tRDO NOT\tY "
              "stack in d20 rules.\tn\r\n");
      return TRUE;
    }

    obj->affected[0].location = bt;
    obj->affected[0].modifier = atoi(arg4);

    if ((bt == APPLY_AC_SHIELD || bt == APPLY_AC_DEFLECTION || bt == APPLY_AC_NATURAL ||
            bt == APPLY_AC_ARMOR))
      obj->affected[0].modifier *= 10;

    if (bt == APPLY_ACCURACY) {
      obj->affected[1].location = APPLY_DAMAGE;
      obj->affected[1].modifier = atoi(arg4);
    }

    GET_OBJ_LEVEL(obj) = set_object_level(obj);
    GET_OBJ_COST(obj) = MAX(10, 100 + GET_OBJ_LEVEL(obj) * 50 *
            MAX(1, GET_OBJ_LEVEL(obj) - 1) + GET_OBJ_COST(obj));

    int cost = MAX(10, GET_OBJ_LEVEL(obj) * (GET_OBJ_LEVEL(obj) / 2) * 3);

    spell_identify(20, ch, ch, obj, NULL);

    if (GET_OBJ_LEVEL(obj) > GET_CLASS_LEVEL(ch)) {
      send_to_char(ch, "You cannot buy an item whose level is greater than yours.\r\n");
      send_to_char(ch, "\tYPlease note that bonuses of the same type \tRDO NOT\tY stack "
              "in d20 rules.\tn\r\n");
      return TRUE;
    }

    if (GET_QUESTPOINTS(ch) < cost) {
      send_to_char(ch, "That item costs %d reputation points and you only have "
              "%d.\r\n", cost, GET_QUESTPOINTS(ch));
      send_to_char(ch, "\tYPlease note that bonuses of the same type \tRDO NOT\tY "
              "stack in d20 rules.\tn\r\n");
      return TRUE;
    }

    GET_QUESTPOINTS(ch) -= cost;

    SET_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_UNIQUE_SAVE);
    char buf[200] = {'\0'};
    snprintf(buf, sizeof(buf), "%s +%d %s", obj->short_description, bonus, get_bonus_type(arg3));
    if (obj->short_description) free(obj->short_description);
    obj->short_description = strdup(buf);
    if (obj->name) free(obj->name);
    obj->name = strdup(buf);
    snprintf(buf, sizeof(buf), "%s +%d %s lies here.", CAP(obj->short_description), bonus,
            get_bonus_type(arg3));
    if (obj->description) free(obj->description);
    obj->description = strdup(buf);

    obj_to_char(obj, ch);

    send_to_char(ch, "You purchase %s for %d reputation points.\r\n",
            obj->short_description, cost);
    return TRUE;
  } else if (is_abbrev(arg, "list")) {

    if (!*arg2) {
      send_to_char(ch, "Please specify either armor, weapon or other.\r\n");
      send_to_char(ch, "\tYPlease note that bonuses of the same type \tRDO NOT\tY "
              "stack in d20 rules.\tn\r\n");
      return TRUE;
    }

    int type = ITEM_OTHER;

    if (is_abbrev(arg2, "armor"))
      type = ITEM_ARMOR;
    else if (is_abbrev(arg2, "weapon"))
      type = ITEM_WEAPON;
    else if (is_abbrev(arg2, "other"))
      type = ITEM_WORN;
    else {
      send_to_char(ch, "Please specify either armor, weapon or other.\r\n");
      send_to_char(ch, "\tYPlease note that bonuses of the same type \tRDO NOT\tY "
              "stack in d20 rules.\tn\r\n");
      return TRUE;
    }

    if (!*arg3) {
      send_to_char(ch, list_bonus_types());
      send_to_char(ch, "\tYPlease note that bonuses of the same type \tRDO NOT\tY "
              "stack in d20 rules.\tn\r\n");
      return TRUE;
    }

    if ((bt = get_bonus_type_int(arg3)) == 0) {
      send_to_char(ch, "That's an invalid bonus type.\r\n\r\n");
      send_to_char(ch, list_bonus_types());
      send_to_char(ch, "\tYPlease note that bonuses of the same type \tRDO NOT\tY "
              "stack in d20 rules.\tn\r\n");
      return TRUE;
    }

    if (!*arg4) {
      send_to_char(ch, "How much would you like the bonus to be?\r\n");
      send_to_char(ch, "\tYPlease note that bonuses of the same type \tRDO NOT\tY "
              "stack in d20 rules.\tn\r\n");
      return TRUE;
    }

    if ((bonus = atoi(arg4)) <= 0) {
      send_to_char(ch, "The bonus must be greater than 0.\r\n");
      send_to_char(ch, "\tYPlease note that bonuses of the same type \tRDO NOT\tY "
              "stack in d20 rules.\tn\r\n");
      return TRUE;
    }

    char buf[100] = {'\0'};
    int cost = 0;

    struct obj_data *obj = NULL;
    int i = 0;
    int vnum = 0;

    send_to_char(ch, "%-5s %-6s %-7s %-35s\r\n----- ------ -------------------------\r\n",
            "VNUM", "COST", "MIN-LVL", "ITEM");

    for (i = 30000; i < 30299; i++) {
      vnum = i;
      if (!((vnum >= 30000 && vnum <= 30083) || (vnum >= 30085 && vnum <= 30092) ||
              vnum == 30095 || (vnum >= 30100 && vnum <= 30105))) {
        continue;
      }
      if (obj)
        extract_obj(obj);
      obj = read_object(i, VIRTUAL);
      if (!obj)
        continue;
      if (GET_OBJ_TYPE(obj) != type)
        continue;
      obj->affected[0].location = bt;
      obj->affected[0].modifier = atoi(arg4);

      if ((bt == APPLY_AC_SHIELD || bt == APPLY_AC_DEFLECTION ||
              bt == APPLY_AC_NATURAL || bt == APPLY_AC_ARMOR))
        obj->affected[0].modifier *= 10;

      if (bt == APPLY_ACCURACY) {
        obj->affected[1].location = APPLY_DAMAGE;
        obj->affected[1].modifier = atoi(arg4);
      }

      GET_OBJ_LEVEL(obj) = set_object_level(obj);
      GET_OBJ_COST(obj) = MAX(10, 100 + GET_OBJ_LEVEL(obj) * 50 *
              MAX(1, GET_OBJ_LEVEL(obj) - 1) + GET_OBJ_COST(obj));

      cost = MAX(10, GET_OBJ_LEVEL(obj) * (GET_OBJ_LEVEL(obj) / 2) * 3);

      snprintf(buf, sizeof(buf), "%s +%d %s", obj->short_description, bonus, get_bonus_type(arg3));
      send_to_char(ch, "%-5d %-6d %d %-35s\r\n", i, cost, GET_OBJ_LEVEL(obj), buf);
    }
    send_to_char(ch, "\r\n");
    send_to_char(ch, "\tYPlease note that bonuses of the same type \tRDO NOT\tY stack "
            "in d20 rules.\tn\r\n");
    return TRUE;
  } else {
    send_to_char(ch, "The syntax for this command is: 'item buy <vnum>' or 'item "
            "list <weapons|armor|other>'.\r\n");
    send_to_char(ch, "\tYPlease note that bonuses of the same type \tRDO NOT\tY "
            "stack in d20 rules.\tn\r\n");
    return TRUE;
  }

  return TRUE;
}
 */

/* General and feature-owned room procedures are implemented by their owners. */
/***********************/
/* end room procedures */
/***********************/

/* EoF */
