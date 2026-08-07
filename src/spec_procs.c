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
static void npc_steal(struct char_data *ch, struct char_data *victim);
static void zone_yell(struct char_data *ch, const char *buf);

/********************************************************************/
/******************** Mobile Procs    *******************************/
/********************************************************************/

/*************************************************/
/**** General special procedures for mobiles. ****/
/*************************************************/

static void npc_steal(struct char_data *ch, struct char_data *victim)
{
  int gold;

  if (IS_NPC(victim))
    return;
  if (GET_LEVEL(victim) >= LVL_IMMORT)
    return;
  if (!CAN_SEE(ch, victim))
    return;

  if (AWAKE(victim) && (rand_number(0, GET_LEVEL(ch)) == 0))
  {
    act("You discover that $n has $s hands in your wallet.", FALSE, ch, 0, victim, TO_VICT);
    act("$n tries to steal gold from $N.", TRUE, ch, 0, victim, TO_NOTVICT);
  }
  else
  {
    /* Steal some gold coins */
    gold = (GET_GOLD(victim) * rand_number(1, 10)) / 100;
    if (gold > 0)
    {
      increase_gold(ch, gold);
      decrease_gold(victim, gold);
    }
  }
}

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

/* another hl port, checks if object with given vnum is being worn */
bool is_wearing(struct char_data *ch, obj_vnum vnum)
{
  int i;

  for (i = 0; i < NUM_WEARS; i++)
  {
    if (GET_EQ(ch, i))
      if (GET_OBJ_VNUM(GET_EQ(ch, i)) == vnum)
        return TRUE;
  }
  return FALSE;
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

/* kenjin proc */
SPECIAL(kt_kenjin)
{
  struct affected_type af;
  struct char_data *vict = 0;
  struct char_data *tch = 0;
  int val = 0;

  if (cmd)
    return FALSE;
  if (!FIGHTING(ch))
    return FALSE;

  if (GET_POS(ch) < POS_FIGHTING)
    return FALSE;

  for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room)
  {
    if (!IS_NPC(tch) || IS_PET(tch))
    {
      if (!vict || !rand_number(0, 2))
      {
        vict = tch;
      }
    }
  }

  val = dice(1, 3);

  // turns you into stone
  if (val == 1)
  {
    act("$n\tc whips out a \tWbone-white\tn wand, and waves it in the air.\tn\r\n"
        "\tcSuddenly $e points it at \tn$N\tc, who slowly turns to stone.",
        FALSE, ch, 0, vict, TO_ROOM);
    new_affect(&af);
    af.spell = SPELL_HOLD_PERSON;
    SET_BIT_AR(af.bitvector, AFF_PARALYZED);
    af.duration = rand_number(2, 3);
    affect_join(vict, &af, TRUE, FALSE, FALSE, FALSE);
    return TRUE;
  }

  // teleports you to the bottom of the shaft.
  if (val == 2)
  {
    act("$n\tc whips out a \trfiery red\tn wand, and waves it in the air.\tn\r\n"
        "\tcSuddenly $e points it at \tn$N\tc, who fades away suddenly.",
        FALSE, ch, 0, vict, TO_ROOM);
    char_from_room(vict);
    char_to_room(vict, real_room(132908));
    look_at_room(vict, 0);
    return TRUE;
  }

  // loads a new mob in 132919 :)
  if (val == 3)
  {
    act("$n\tc whips out a \tYgolden\tn wand, and waves it in the air.\tn\r\n"
        "\tcSuddenly $e taps in the air, and you see \tLshadow\tc coalesce behind you.",
        FALSE, ch, 0, vict, TO_ROOM);
    tch = read_mobile(132902, VIRTUAL);
    if (!tch)
      return FALSE;
    char_to_room(tch, real_room(132919));
    return TRUE;
  }

  return FALSE;
}

/* from homeland */
SPECIAL(kt_twister)
{
  struct char_data *mob;
  char l_name[256];
  char s_name[256];
  int temp;

  if (cmd)
    return FALSE;

  if (IS_NPC(ch) && !IS_PET(ch))
    return FALSE;

  temp = world[real_room(132901)].dir_option[0]->to_room;
  world[real_room(32901)].dir_option[0]->to_room =

      world[real_room(132901)].dir_option[1]->to_room;
  world[real_room(32901)].dir_option[1]->to_room =

      world[real_room(132901)].dir_option[2]->to_room;
  world[real_room(32901)].dir_option[2]->to_room =

      world[real_room(132901)].dir_option[3]->to_room;
  world[real_room(32901)].dir_option[3]->to_room = temp;

  send_to_room(real_room(132901), "\tCThe world seems to turn.\tn\r\n");

  mob = read_mobile(132901, VIRTUAL);

  if (!mob)
    return FALSE;

  char_to_room(mob, real_room(132906));

  snprintf(l_name, sizeof(l_name), "\tLThe shadow of \tw%s\tL stands here.\tn  ", GET_NAME(ch));
  snprintf(s_name, sizeof(s_name), "\tLa shadow of \tw%s\tn", GET_NAME(ch));

  mob->player.short_descr = strdup(s_name);
  mob->player.name = strdup("shadow");
  mob->player.long_descr = strdup(l_name);

  GET_LEVEL(mob) = GET_LEVEL(ch);
  GET_MAX_HIT(mob) = 1000 + GET_LEVEL(mob) * 50;
  GET_HIT(mob) = GET_MAX_HIT(mob);
  GET_CLASS(mob) = GET_CLASS(ch);

  send_to_char(ch, "You somehow feel \tWsplit\tn in half.\r\n");
  return TRUE;
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

SPECIAL(guild)
{
  int skill_num, percent;
  char arg[MAX_STRING_LENGTH] = {'\0'}, buf[MAX_STRING_LENGTH] = {'\0'};
  char *ability_name = NULL;

  if (IS_NPC(ch) || (!CMD_IS("practice") && !CMD_IS("train") && !CMD_IS("boosts")))
    return (FALSE);

  skip_spaces(&argument);

  // Practice code
  if (CMD_IS("practice"))
  {
    list_crafting_skills(ch);
    return (TRUE);

    /***************************************/
    /* everything below this is deprecated */
    /***************************************/

    if (!*argument)
    {
      list_skills(ch);
      return (TRUE);
    }
    if (GET_PRACTICES(ch) <= 0)
    {
      send_to_char(ch, "You do not seem to be able to practice now.\r\n");
      return (TRUE);
    }

    skill_num = find_skill_num(argument);

    if (skill_num < 1 || GET_LEVEL(ch) < spell_info[skill_num].min_level[(int)GET_CLASS(ch)])
    {
      send_to_char(ch, "You do not know of that skill.\r\n");
      return (TRUE);
    }

    /*
    if (GET_SKILL(ch, skill_num) >= LEARNED(ch)) {
      send_to_char(ch, "You are already learned in that area.\r\n");
      return (TRUE);
    }
     */

    if (skill_num > SPELL_RESERVED_DBC && skill_num < MAX_SPELLS)
    {
      send_to_char(ch, "You can't practice spells.\r\n");
      return (TRUE);
    }

    if (!meet_skill_reqs(ch, skill_num))
    {
      send_to_char(ch, "You haven't met the pre-requisites for that skill.\r\n");
      return (TRUE);
    }

    /* added with addition of crafting system so you can't use your
     'practice points' for training your crafting skills which have
     a much lower base value than 75 */

    if (GET_SKILL(ch, skill_num))
    {
      send_to_char(ch, "You already have this skill trained.\r\n");
      return TRUE;
    }

    send_to_char(ch, "You practice '%s' with your trainer...\r\n", spell_info[skill_num].name);
    GET_PRACTICES(ch)
    --;

    percent = GET_SKILL(ch, skill_num);
    percent += int_app[GET_INT(ch)].learn;

    SET_SKILL(ch, skill_num, percent);

    /*
    if (GET_SKILL(ch, skill_num) >= LEARNED(ch))
      send_to_char(ch, "You are now \tGlearned\tn in '%s.'\r\n",
            spell_info[skill_num].name);
     */

    // for further expansion - zusuk
    process_skill(ch, skill_num);

    return (TRUE);
  }
  else if (CMD_IS("train"))
  {
    // training code

    if (!*argument)
    {
      list_abilities(ch, ABILITY_TYPE_GENERAL);
      return (TRUE);
    }

    if (GET_TRAINS(ch) <= 0)
    {
      send_to_char(ch, "You do not seem to be able to train now.\r\n");
      return (TRUE);
    }

    /* Parse argument and check for 'knowledge' or 'craft' as a first arg- */
    ability_name = one_argument_u(argument, arg);
    skip_spaces(&ability_name);

    if (is_abbrev(arg, "craft"))
    {
      if (!strcmp(ability_name, ""))
      {
        list_abilities(ch, ABILITY_TYPE_CRAFT);
        return (TRUE);
      }
      /* Crafting skill */
      snprintf(buf, sizeof(buf), "Craft (%s", ability_name);
      skill_num = find_ability_num(buf);
    }
    else if (is_abbrev(arg, "knowledge"))
    {
      if (!strcmp(ability_name, ""))
      {
        list_abilities(ch, ABILITY_TYPE_KNOWLEDGE);
        return (TRUE);
      }
      /* Knowledge skill */
      snprintf(buf, sizeof(buf), "Knowledge (%s", ability_name);
      skill_num = find_ability_num(buf);
    }
    else
    {
      skill_num = find_ability_num(argument);
    }

    if (skill_num < 1)
    {
      send_to_char(ch, "You do not know of that ability.\r\n");
      return (TRUE);
    }

    // ability not available to this class
    if (modify_class_ability(ch, skill_num, GET_CLASS(ch)) == 0)
    {
      send_to_char(ch, "This ability is not available to your class...\r\n");
      return (TRUE);
    }

    // cross-class ability
    if (GET_TRAINS(ch) < 2 && modify_class_ability(ch, skill_num, GET_CLASS(ch)) == 1)
    {
      send_to_char(
          ch, "(Cross-Class) You don't have enough training sessions to train that ability...\r\n");
      return (TRUE);
    }
    if (GET_ABILITY(ch, skill_num) >= ((int)((GET_LEVEL(ch) + 3) / 2)) &&
        modify_class_ability(ch, skill_num, GET_CLASS(ch)) == 1)
    {
      send_to_char(ch, "You are already trained in that area.\r\n");
      return (TRUE);
    }

    // class ability
    if (GET_ABILITY(ch, skill_num) >= (GET_LEVEL(ch) + 3) &&
        modify_class_ability(ch, skill_num, GET_CLASS(ch)) == 2)
    {
      send_to_char(ch, "You are already trained in that area.\r\n");
      return (TRUE);
    }

    send_to_char(ch, "You train for a while...\r\n");
    GET_TRAINS(ch)
    --;
    if (modify_class_ability(ch, skill_num, GET_CLASS(ch)) == 1)
    {
      GET_TRAINS(ch)
      --;
      send_to_char(ch, "You used two training sessions to train a cross-class ability...\r\n");
    }
    GET_ABILITY(ch, skill_num)
    ++;

    if (GET_ABILITY(ch, skill_num) >= (GET_LEVEL(ch) + 3))
      send_to_char(ch, "You are now trained in that area.\r\n");
    if (GET_ABILITY(ch, skill_num) >= ((int)((GET_LEVEL(ch) + 3) / 2)) &&
        modify_class_ability(ch, skill_num, GET_CLASS(ch)) == 1)
      send_to_char(ch, "You are already trained in that area.\r\n");

    return (TRUE);
  }
  else if (CMD_IS("boosts"))
  {
    if (!argument || !*argument)
      send_to_char(ch,
                   "\tCStat boost sessions remaining: %d\tn\r\n"
                   "\tcStats:\tn\r\n"
                   "Strength\r\n"
                   "Constitution\r\n"
                   "Dexterity\r\n"
                   "Intelligence\r\n"
                   "Wisdom\r\n"
                   "Charisma\r\n"
                   "\r\n",
                   GET_BOOSTS(ch));
    else if (!GET_BOOSTS(ch))
      send_to_char(ch, "You have no ability training sessions.\r\n");
    else if (!strncasecmp("strength", argument, strlen(argument)))
    {
      send_to_char(ch, "%s", CONFIG_OK);
      send_to_char(ch, "%s", "\tMYour strength increases!\tn\r\n");
      GET_REAL_STR(ch) += 1;
      GET_BOOSTS(ch) -= 1;
    }
    else if (!strncasecmp("constitution", argument, strlen(argument)))
    {
      send_to_char(ch, "%s", CONFIG_OK);
      send_to_char(ch, "%s", "\tMYour constitution increases!\tn\r\n");
      GET_REAL_CON(ch) += 1;
      /* Give them retroactive hit points for constitution */
      if (!(GET_REAL_CON(ch) % 2))
      {
        GET_REAL_MAX_HIT(ch) += GET_LEVEL(ch);
        send_to_char(ch, "\tMYou gain %d hitpoints!\tn\r\n", GET_LEVEL(ch));
      }
      GET_BOOSTS(ch) -= 1;
    }
    else if (!strncasecmp("dexterity", argument, strlen(argument)))
    {
      send_to_char(ch, "%s", CONFIG_OK);
      send_to_char(ch, "%s", "\tMYour dexterity increases!\tn\r\n");
      GET_REAL_DEX(ch) += 1;
      GET_BOOSTS(ch) -= 1;
    }
    else if (!strncasecmp("intelligence", argument, strlen(argument)))
    {
      send_to_char(ch, "%s", CONFIG_OK);
      send_to_char(ch, "%s", "\tMYour intelligence increases!\tn\r\n");
      GET_REAL_INT(ch) += 1;
      GET_BOOSTS(ch) -= 1;
      /* Give them retroactive trains */
      if (!(GET_REAL_INT(ch) % 2))
      {
        GET_TRAINS(ch) += GET_LEVEL(ch);
        send_to_char(ch, "\tMYour intelligence increases!\tn\r\n");
        send_to_char(ch, "\tMYou gain %d trains!\tn\r\n", GET_LEVEL(ch));
      }
    }
    else if (!strncasecmp("wisdom", argument, strlen(argument)))
    {
      send_to_char(ch, "%s", CONFIG_OK);
      send_to_char(ch, "\tMYour wisdom increases!\tn\r\n");
      GET_REAL_WIS(ch) += 1;
      GET_BOOSTS(ch) -= 1;
    }
    else if (!strncasecmp("charisma", argument, strlen(argument)))
    {
      send_to_char(ch, "%s", CONFIG_OK);
      send_to_char(ch, "\tMYour charisma increases!\tn\r\n");
      GET_REAL_CHA(ch) += 1;
      GET_BOOSTS(ch) -= 1;
    }
    else
      send_to_char(ch,
                   "\tCStat boost sessions remaining: %d\tn\r\n"
                   "\tcStats:\tn\r\n"
                   "Strength\r\n"
                   "Constitution\r\n"
                   "Dexterity\r\n"
                   "Intelligence\r\n"
                   "Wisdom\r\n"
                   "Charisma\r\n"
                   "\r\n",
                   GET_BOOSTS(ch));
    affect_total(ch);
    send_to_char(ch, "\tDType 'feats' to see your feats\tn\r\n");
    send_to_char(ch, "\tDType 'train' to see your abilities\tn\r\n");
    send_to_char(ch, "\tDType 'boost' to adjust your stats\tn\r\n");
    send_to_char(ch, "\tDType 'craft' to see your crafting proficiency\tn\r\n");
    send_to_char(ch, "\tDType 'spells <classname>' to see your currently known spells\tn\r\n");
    return (TRUE);
  }

  // should not be able to get here
  log("Reached the unreachable in SPECIAL(guild) in spec_procs.c");
  return (FALSE);
}


int unlinkMovingRoom(struct moving_room_data *theRoom, struct oldNextMove *ONMdata, int cibIdx)
{
  char errStr[128];

  if ((ONMdata->oldRoom != NOWHERE) && (ONMdata->oldRoom != ENDMOVING))
  {
    /*  unlink old room - both sides  */

    /*  check if rooms sync up  */
    if (theRoom->from[cibIdx] != ONMdata->oldRoom)
    {
      snprintf(errStr, sizeof(errStr),
               "SPEC(move_room): [%d] from[cibIdx] != oldRoom (or <= 0) (%d/%d %d)",
               (int)ONMdata->moveRoom, theRoom->from[cibIdx], ONMdata->oldRoom, cibIdx);
      log("%s", errStr);
      return 0;
    }

    /*  check if dest room dir is clean...  */
    if (world[real_room(ONMdata->moveRoom)].dir_option[theRoom->inbound_dir] == NULL)
    {
      log("SPEC(move_room): moving room has not set a connecting room...");
      return 0;
    }

    /*  check if old conn room dir is clean...  */
    if (world[real_room(ONMdata->oldRoom)].dir_option[ONMdata->oldDir] == NULL)
    {
      sprintf(errStr, "SPEC(move_room): [%d] old conn room %d dir %d not set...",
              (int)ONMdata->moveRoom, ONMdata->oldRoom, ONMdata->oldDir);
      log("%s", errStr);
      return 0;
    }

    /* log("SPEC(move): all is Ok to unlink the old room..."); */

#ifdef DEBUGMEM
    freeusg(world[real_room(ONMdata->moveRoom)].dir_option[theRoom->inbound_dir], P9);
#else
    free(world[real_room(ONMdata->moveRoom)].dir_option[theRoom->inbound_dir]);
#endif

    world[real_room(ONMdata->moveRoom)].dir_option[theRoom->inbound_dir] = NULL;

#ifdef DEBUGMEM
    freeusg(world[real_room(ONMdata->oldRoom)].dir_option[ONMdata->oldDir], O9);
#else
    free(world[real_room(ONMdata->oldRoom)].dir_option[ONMdata->oldDir]);
#endif
    world[real_room(ONMdata->oldRoom)].dir_option[ONMdata->oldDir] = NULL;

    /* log("SPEC(move): all is Ok after unlinking the old room..."); */
    sprintf(errStr, "SPEC(moving_rooms): [%d] unlinked %d  FROM  %d", (int)ONMdata->moveRoom,
            ONMdata->oldRoom, theRoom->destination);
    /* mudlog(errStr, CMP, LVL_QUEST, TRUE); */
  }

  return 1;
}

int linkMovingRoom(struct moving_room_data *theRoom, struct oldNextMove *ONMdata, int cibIdx)
{
  struct room_direction_data *rdd;
  char errStr[100];

  if (ONMdata->nextRoom != NOWHERE)
  {
    /*  link up new room - both sides  */

    /*  check if dest room dir is clean...  */
    if (world[real_room(ONMdata->moveRoom)].dir_option[theRoom->inbound_dir] != NULL)
    {
      log("SPEC(move_room): moving room hasn't unset last connecting room...");

      rdd = world[real_room(ONMdata->moveRoom)].dir_option[theRoom->inbound_dir];
      sprintf(errStr, "SPEC(move): [%d] rdd - desc:%s:  key:%s:  ei:%d:  key:%d:  to:%d:",
              (int)ONMdata->moveRoom,
              (rdd->general_description == NULL) ? "" : rdd->general_description,
              (rdd->keyword == NULL) ? "" : rdd->keyword, rdd->exit_info, rdd->key, rdd->to_room);
      log("%s", errStr);

      return 0;
    }

    /*  check if conn room dir is clean...  */
    if (world[real_room(ONMdata->nextRoom)].dir_option[ONMdata->nextDir] != NULL)
    {
      sprintf(errStr, "SPEC(move_room): [%d] conn room has dir %d set...", (int)ONMdata->moveRoom,
              ONMdata->nextDir);
      log("%s", errStr);

      rdd = world[real_room(ONMdata->nextRoom)].dir_option[ONMdata->nextDir];
      sprintf(errStr, "SPEC(move): [%d] rdd - desc:%s:  key:%s:  ei:%d:  key:%d:  to:%d:",
              (int)ONMdata->moveRoom,
              (rdd->general_description == NULL) ? "" : rdd->general_description,
              (rdd->keyword == NULL) ? "" : rdd->keyword, rdd->exit_info, rdd->key, rdd->to_room);
      log("%s", errStr);

/*  pdh 5/3/01 - don't return - instead remove the offending exit
return 0;
*/
#ifdef DEBUGMEM
      freeusg(world[real_room(ONMdata->nextRoom)].dir_option[ONMdata->nextDir], N9);
#else
      free(world[real_room(ONMdata->nextRoom)].dir_option[ONMdata->nextDir]);
#endif
      world[real_room(ONMdata->nextRoom)].dir_option[ONMdata->nextDir] = NULL;
    }

    if (theRoom->from[cibIdx] != ENDMOVING)
    {
      log("SPEC(move_room): theRoom->from[cibIdx] != ENDMOVING");
      return 0;
    }

    /* log("SPEC(move): all is Ok to link the new room..."); */

#ifdef DEBUGMEM
    CREATE(world[real_room(ONMdata->nextRoom)].dir_option[ONMdata->nextDir],
           struct room_direction_data, 1, M9);
#else
    CREATE(world[real_room(ONMdata->nextRoom)].dir_option[ONMdata->nextDir],
           struct room_direction_data, 1);
#endif
    world[real_room(ONMdata->nextRoom)].dir_option[ONMdata->nextDir]->general_description = NULL;
    world[real_room(ONMdata->nextRoom)].dir_option[ONMdata->nextDir]->keyword = NULL;
    world[real_room(ONMdata->nextRoom)].dir_option[ONMdata->nextDir]->exit_info = 0;
    world[real_room(ONMdata->nextRoom)].dir_option[ONMdata->nextDir]->key = -1;
    world[real_room(ONMdata->nextRoom)].dir_option[ONMdata->nextDir]->to_room =
        real_room(theRoom->destination);

    /*  play 'docking' message  */
    if (theRoom->msg_docking)
    {
      struct char_data *cc;

      for (cc = world[real_room(ONMdata->moveRoom)].people; cc; cc = cc->next_in_room)
      {
        if (cc->desc)
        {
          send_to_char(cc, "%s\r\n", theRoom->msg_docking);
        }
      }
    }

#ifdef DEBUGMEM
    CREATE(world[real_room(ONMdata->moveRoom)].dir_option[theRoom->inbound_dir],
           struct room_direction_data, 1, L9);
#else
    CREATE(world[real_room(ONMdata->moveRoom)].dir_option[theRoom->inbound_dir],
           struct room_direction_data, 1);
#endif
    world[real_room(ONMdata->moveRoom)].dir_option[theRoom->inbound_dir]->general_description =
        NULL;
    world[real_room(ONMdata->moveRoom)].dir_option[theRoom->inbound_dir]->keyword = NULL;
    world[real_room(ONMdata->moveRoom)].dir_option[theRoom->inbound_dir]->exit_info = 0;
    world[real_room(ONMdata->moveRoom)].dir_option[theRoom->inbound_dir]->key = -1;
    world[real_room(ONMdata->moveRoom)].dir_option[theRoom->inbound_dir]->to_room =
        real_room(ONMdata->nextRoom);

    /*  play 'dest_docking' message  */
    if (theRoom->msg_dest_docking)
    {
      struct char_data *cc;

      for (cc = world[real_room(ONMdata->nextRoom)].people; cc; cc = cc->next_in_room)
      {
        if (cc->desc)
        {
          send_to_char(cc, "%s\r\n", theRoom->msg_dest_docking);
        }
      }
    }

    /* log("SPEC(move): all complete: linked the new room"); */
    sprintf(errStr, "SPEC(moving_rooms): [%d] unlinked %d and linked %d  TO  %d",
            (int)ONMdata->moveRoom, ONMdata->oldRoom, ONMdata->nextRoom, theRoom->destination);
    /* mudlog(errStr, CMP, LVL_QUEST, TRUE); */
  }
  else
  {
    /*  play 'transit' message  */
    if (theRoom->msg_transit)
    {
      struct char_data *cc;

      for (cc = world[real_room(ONMdata->moveRoom)].people; cc; cc = cc->next_in_room)
      {
        if (cc->desc)
        {
          send_to_char(cc, "%s\r\n", theRoom->msg_transit);
        }
      }
    }
  }

  return 1;
}

int prepMovingRoom(struct moving_room_data *theRoom, struct oldNextMove *ONMdata, int *cibIdx,
                   int *nextIdx)
{
  int fromRoomCnt = 0;

  /*  get count of rooms in array  */
  for (fromRoomCnt = 0; theRoom->from[fromRoomCnt] != ENDMOVING; fromRoomCnt++)
  {
    ;
  }

  if (fromRoomCnt <= 1)
  {
    log("SPECIAL(moving_rooms): 1 or smaller length room array");
    return 0;
  }

  if ((theRoom->currentInbound < 0) || (theRoom->currentInbound >= fromRoomCnt))
  {
    *cibIdx = fromRoomCnt;
  }
  else
  {
    *cibIdx = theRoom->currentInbound;
  }

  ONMdata->oldRoom = theRoom->from[*cibIdx];
  ONMdata->oldDir = theRoom->fromDir[*cibIdx];
  ONMdata->moveRoom = theRoom->destination;

  if (theRoom->randomMove)
  {
    /*  randomly select  */
    *nextIdx = dice(1, fromRoomCnt) - 1;
  }
  else
  {
    /*  next in line  */
    *nextIdx = *cibIdx + 1;

    if ((*nextIdx <= 0) || (*nextIdx >= fromRoomCnt) || (theRoom->from[*nextIdx] == ENDMOVING))
    {
      /*  start over  */
      *nextIdx = 0;
    }
  }

  ONMdata->nextRoom = theRoom->from[*nextIdx];
  ONMdata->nextDir = theRoom->fromDir[*nextIdx];

  /*
      sprintf(errStr, "SPECIAL(moving_rooms): [%d] old: %d (%d) next: %d (%d)",
      (int)moveRoom, oldRoom, oldDir, nextRoom, nextDir);
      log(errStr);
      */

  /*  PDH  5/1/98 - OLD
     *  if ( (ONMdata->nextRoom == NOWHERE) || (ONMdata->nextDir == -1) ) {
     *
     *  now, the next room info CAN be to nowhere
     *  (ie. disconnect the room)
     */
  if ((ONMdata->nextRoom == ENDMOVING) ||
      ((ONMdata->nextRoom != NOWHERE) && (ONMdata->nextDir == -1)))
  {
    /*  it's a bust  */
    log("SPEC(move): nextRoom or nextDir was bogus!");
    return 0;
  }

  return fromRoomCnt;
}

SPECIAL(moving_rooms)
{
  int fromRoomCnt = 0;
  struct oldNextMove ONM;
  int cibIdx = -1, nextIdx = -1;

  struct moving_room_data *theRoom;

  ONM.nextDir = -1;
  ONM.oldDir = -1;
  ONM.nextRoom = NOWHERE;
  ONM.oldRoom = NOWHERE;
  ONM.moveRoom = NOWHERE;

  if ((ch == NULL) && (me != NULL) && (cmd == 0) && (argument == NULL))
  {
    /* log("SPECIAL(moving_rooms)"); */
  }
  else
  {
    return 0;
  }

  /*  PDH 11/17/97  ugly cast... but needed  */
  theRoom = (struct moving_room_data *)me;

  if ((fromRoomCnt = prepMovingRoom(theRoom, &ONM, &cibIdx, &nextIdx)) < 1)
  {
    return 0;
  }

  /*  everything is set now, so make the changes  */

  if (!unlinkMovingRoom(theRoom, &ONM, cibIdx))
  {
    return 0;
  }

  cibIdx = fromRoomCnt; /* NOWHERE */
  theRoom->currentInbound = cibIdx;

  if (!linkMovingRoom(theRoom, &ONM, cibIdx))
  {
    return 0;
  }

  /*  set state of struct moving_room_data  */
  cibIdx = nextIdx;
  theRoom->currentInbound = cibIdx;

  return 1;
}

SPECIAL(mayor)
{
  char actbuf[MAX_INPUT_LENGTH] = {'\0'};

  static const char open_path[] = "W3a3003b33000c111d0d111Oe333333Oe22c222112212111a1S.";
  static const char close_path[] = "W3a3003b33000c111d0d111CE333333CE22c222112212111a1S.";

  static const char *path = NULL;
  static int path_index;
  static bool move = FALSE;

  if (!move)
  {
    if (time_info.hours == 6)
    {
      move = TRUE;
      path = open_path;
      path_index = 0;
    }
    else if (time_info.hours == 20)
    {
      move = TRUE;
      path = close_path;
      path_index = 0;
    }
  }
  if (cmd || !move || (GET_POS(ch) < POS_SLEEPING) || FIGHTING(ch))
    return (FALSE);

  switch (path[path_index])
  {
  case '0':
  case '1':
  case '2':
  case '3':
    perform_move(ch, path[path_index] - '0', 1);
    break;

  case 'W':
    change_position(ch, POS_STANDING);
    act("$n awakens and groans loudly.", FALSE, ch, 0, 0, TO_ROOM);
    break;

  case 'S':
    change_position(ch, POS_SLEEPING);
    act("$n lies down and instantly falls asleep.", FALSE, ch, 0, 0, TO_ROOM);
    break;

  case 'a':
    act("$n says 'Hello Honey!'", FALSE, ch, 0, 0, TO_ROOM);
    act("$n smirks.", FALSE, ch, 0, 0, TO_ROOM);
    break;

  case 'b':
    act("$n says 'What a view!  I must get something done about that dump!'", FALSE, ch, 0, 0,
        TO_ROOM);
    break;

  case 'c':
    act("$n says 'Vandals!  Youngsters nowadays have no respect for anything!'", FALSE, ch, 0, 0,
        TO_ROOM);
    break;

  case 'd':
    act("$n says 'Good day, citizens!'", FALSE, ch, 0, 0, TO_ROOM);
    break;

  case 'e':
    act("$n says 'I hereby declare the bazaar open!'", FALSE, ch, 0, 0, TO_ROOM);
    break;

  case 'E':
    act("$n says 'I hereby declare Midgen closed!'", FALSE, ch, 0, 0, TO_ROOM);
    break;

  case 'O':
    do_gen_door(ch, strcpy(actbuf, "gate"), 0, SCMD_UNLOCK); /* strcpy: OK */
    do_gen_door(ch, strcpy(actbuf, "gate"), 0, SCMD_OPEN);   /* strcpy: OK */
    break;

  case 'C':
    do_gen_door(ch, strcpy(actbuf, "gate"), 0, SCMD_CLOSE); /* strcpy: OK */
    do_gen_door(ch, strcpy(actbuf, "gate"), 0, SCMD_LOCK);  /* strcpy: OK */
    break;

  case '.':
    move = FALSE;
    break;
  }

  path_index++;
  return (FALSE);
}

/* Quite lethal to low-level characters. */
SPECIAL(snake)
{
  if (cmd || !FIGHTING(ch))
    return (FALSE);

  if (IN_ROOM(FIGHTING(ch)) != IN_ROOM(ch) || rand_number(0, GET_LEVEL(ch)) != 0)
    return (FALSE);

  act("$n bites $N!", 1, ch, 0, FIGHTING(ch), TO_NOTVICT);
  act("$n bites you!", 1, ch, 0, FIGHTING(ch), TO_VICT);
  call_magic(ch, FIGHTING(ch), 0, SPELL_POISON, 0, GET_LEVEL(ch), CAST_WEAPON_SPELL);
  return (TRUE);
}

SPECIAL(hound)
{
  struct char_data *i;
  int door;
  room_rnum room;

  if (cmd || GET_POS(ch) != POS_STANDING || FIGHTING(ch))
    return (FALSE);

  /* first go through all the directions */
  for (door = 0; door < DIR_COUNT; door++)
  {
    if (CAN_GO(ch, door))
    {
      room = world[IN_ROOM(ch)].dir_option[door]->to_room;

      /* ok found a neighboring room, now cycle through the peeps */
      for (i = world[room].people; i; i = i->next_in_room)
      {
        /* is this guy a hostile? */
        if (i && IS_NPC(i) && MOB_FLAGGED(i, MOB_AGGRESSIVE))
        {
          act("$n howls a warning!", FALSE, ch, 0, 0, TO_ROOM);
          return (TRUE);
        }
      } // end peeps cycle
    } // can_go
  } // end room cycle

  return (FALSE);
}

SPECIAL(thief)
{
  struct char_data *cons;

  if (cmd || GET_POS(ch) != POS_STANDING)
    return (FALSE);

  for (cons = world[IN_ROOM(ch)].people; cons; cons = cons->next_in_room)
    if (!IS_NPC(cons) && GET_LEVEL(cons) < LVL_IMMORT && !rand_number(0, 4))
    {
      npc_steal(ch, cons);
      return (TRUE);
    }

  return (FALSE);
}

SPECIAL(wizard)
{
  struct char_data *vict;

  if (cmd || !FIGHTING(ch))
    return (FALSE);

  /* pseudo-randomly choose someone in the room who is fighting me */
  for (vict = world[IN_ROOM(ch)].people; vict; vict = vict->next_in_room)
    if (FIGHTING(vict) == ch && !rand_number(0, 4))
      break;

  /* if I didn't pick any of those, then just slam the guy I'm fighting */
  if (vict == NULL && IN_ROOM(FIGHTING(ch)) == IN_ROOM(ch))
    vict = FIGHTING(ch);

  /* Hm...didn't pick anyone...I'll wait a round. */
  if (vict == NULL)
    return (TRUE);

  if (GET_LEVEL(ch) > 13 && rand_number(0, 10) == 0)
    cast_spell(ch, vict, NULL, SPELL_POISON, 0);

  if (GET_LEVEL(ch) > 7 && rand_number(0, 8) == 0)
    cast_spell(ch, vict, NULL, SPELL_BLINDNESS, 0);

  if (GET_LEVEL(ch) > 12 && rand_number(0, 12) == 0)
  {
    if (IS_EVIL(ch))
      cast_spell(ch, vict, NULL, SPELL_ENERGY_DRAIN, 0);
    else if (IS_GOOD(ch))
      cast_spell(ch, vict, NULL, SPELL_DISPEL_EVIL, 0);
  }

  if (rand_number(0, 4))
    return (TRUE);

  switch (GET_LEVEL(ch))
  {
  case 4:
  case 5:
    cast_spell(ch, vict, NULL, SPELL_MAGIC_MISSILE, 0);
    break;
  case 6:
  case 7:
    cast_spell(ch, vict, NULL, SPELL_CHILL_TOUCH, 0);
    break;
  case 8:
  case 9:
    cast_spell(ch, vict, NULL, SPELL_BURNING_HANDS, 0);
    break;
  case 10:
  case 11:
    cast_spell(ch, vict, NULL, SPELL_SHOCKING_GRASP, 0);
    break;
  case 12:
  case 13:
    cast_spell(ch, vict, NULL, SPELL_LIGHTNING_BOLT, 0);
    break;
  case 14:
  case 15:
  case 16:
  case 17:
    cast_spell(ch, vict, NULL, SPELL_COLOR_SPRAY, 0);
    break;
  default:
    cast_spell(ch, vict, NULL, SPELL_FIREBALL, 0);
    break;
  }
  return (TRUE);
}

SPECIAL(wall)
{
  if (!IS_MOVE(cmd))
    return (FALSE);

  /* acceptable ways to avoid the wall */
  /* */

  /* failed to get past wall */
  send_to_char(ch, "You can't get by the magical wall!\r\n");
  act("$n fails to get past the magical wall!", FALSE, ch, 0, 0, TO_ROOM);
  return (TRUE);
}

SPECIAL(guild_guard)
{
  int i, direction;
  struct char_data *guard = (struct char_data *)me;
  const char *buf = "The guard humiliates you, and blocks your way.\r\n";
  const char *buf2 = "The guard humiliates $n, and blocks $s way.";

  if (!IS_MOVE(cmd) || AFF_FLAGGED(guard, AFF_BLIND))
    return (FALSE);

  if (GET_LEVEL(ch) >= LVL_IMMORT)
    return (FALSE);

  /* find out what direction they are trying to go */
  for (direction = 0; direction < NUM_OF_DIRS; direction++)
    if (!strcmp(cmd_info[cmd].command, dirs[direction]))
      for (direction = 0; direction < DIR_COUNT; direction++)
        if (!strcmp(cmd_info[cmd].command, dirs[direction]) ||
            !strcmp(cmd_info[cmd].command, autoexits[direction]))
          break;

  for (i = 0; guild_info[i].guild_room != NOWHERE; i++)
  {
    /* Wrong guild. */
    if (GET_ROOM_VNUM(IN_ROOM(ch)) != guild_info[i].guild_room)
      continue;

    /* Wrong direction. */
    if (direction != guild_info[i].direction)
      continue;

    /* Allow the people of the guild through. */
    /* Can't use GET_CLASS anymore, need CLASS_LEVEL(ch, i)!!  - 04/08/2013 Ornir */
    if (!IS_NPC(ch) && (CLASS_LEVEL(ch, guild_info[i].pc_class) > 0))
      continue;

    send_to_char(ch, "%s", buf);
    act(buf2, FALSE, ch, 0, 0, TO_ROOM);
    return (TRUE);
  }
  return (FALSE);
}

SPECIAL(puff)
{
  char actbuf[MAX_INPUT_LENGTH] = {'\0'};

  if (cmd)
    return (FALSE);

  switch (rand_number(0, 60))
  {
  case 0:
    do_say(ch, strcpy(actbuf, "My god!  It's full of stars!"), 0, 0); /* strcpy: OK */
    return (TRUE);
  case 1:
    do_say(ch, strcpy(actbuf, "How'd all those fish get up here?"), 0, 0); /* strcpy: OK */
    return (TRUE);
  case 2:
    do_say(ch, strcpy(actbuf, "I'm a very female dragon."), 0, 0); /* strcpy: OK */
    return (TRUE);
  case 3:
    do_say(ch, strcpy(actbuf, "I've got a peaceful, easy feeling."), 0, 0); /* strcpy: OK */
    return (TRUE);
  default:
    return (FALSE);
  }
}

SPECIAL(fido)
{
  struct obj_data *i, *temp, *next_obj;

  if (cmd || !AWAKE(ch))
    return (FALSE);

  for (i = world[IN_ROOM(ch)].contents; i; i = i->next_content)
  {
    if (!IS_CORPSE(i))
      continue;

    act("$n savagely devours a corpse.", FALSE, ch, 0, 0, TO_ROOM);
    for (temp = i->contains; temp; temp = next_obj)
    {
      next_obj = temp->next_content;
      obj_from_obj(temp);
      obj_to_room(temp, IN_ROOM(ch));
    }
    extract_obj(i);
    return (TRUE);
  }
  return (FALSE);
}

SPECIAL(janitor)
{
  struct obj_data *i;

  if (cmd || !AWAKE(ch))
    return (FALSE);

  for (i = world[IN_ROOM(ch)].contents; i; i = i->next_content)
  {
    if (!CAN_WEAR(i, ITEM_WEAR_TAKE))
      continue;
    if (GET_OBJ_TYPE(i) != ITEM_DRINKCON && GET_OBJ_COST(i) >= 15)
      continue;
    act("$n picks up some trash.", FALSE, ch, 0, 0, TO_ROOM);
    obj_from_room(i);
    obj_to_char(i, ch);
    return (TRUE);
  }
  return (FALSE);
}

/* this is the generic dracolich procs -zusuk */
SPECIAL(dracolich_mob)
{
  struct char_data *vict = NULL;
  int hitpoints = 0, use_aoe = 0;

  if (!ch)
    return 0;

  /* note that the !vict is moved below */
  if (cmd)
    return 0;

  /* this is the offensive arsenal */
  if (FIGHTING(ch) && rand_number(0, 1))
  {
    if (!rand_number(0, 3) &&
        call_magic(ch, FIGHTING(ch), 0, SPELL_ACID_BREATHE, 0, GET_LEVEL(ch), CAST_INNATE))
    {
      /* looks like the breathe weapon worked */
      return 1;
    }
    else if (!rand_number(0, 3) && perform_tailsweep(ch))
    {
      /* looks like we did the tailsweeep successffully to at least one victim */
      return 1;
    }
    else if (!rand_number(0, 3) && perform_dragonfear(ch))
    {
      /* looks like we did the dragonfear to at least one victim */
      return 1;
    }
    else if (!rand_number(0, 4))
    {
      int i = 0;

      act("\tWWith power and determination you unleash an aggressive flurry of attacks!\tn", TRUE,
          ch, 0, FIGHTING(ch), TO_CHAR);
      act("$n\tL, with power and determination, unleashes an aggressive flurry of attacks!\tn",
          FALSE, ch, 0, FIGHTING(ch), TO_VICT);
      act("$n\tL, with power and determination, unleashes an aggressive flurry of attacks!\tn",
          TRUE, ch, 0, FIGHTING(ch), TO_NOTVICT);

      /* spam some attacks */
      for (i = 0; i <= rand_number(2, 4); i++)
      {
        if (valid_fight_cond(ch, TRUE))
          hit(ch, FIGHTING(ch), TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, FALSE);
      }
      return 1;
    }
  }
  /* special dracolich drain */
  else if (!rand_number(0, 6))
  {
    /* find random target, and num targets */
    if (!(vict = npc_find_target(ch, &use_aoe)))
      return 0;

    act("\tWWith a grin, you whisper, 'die' while touching $N, who keels over and falls over in "
        "excruciating pain!\tn",
        TRUE, ch, 0, vict, TO_CHAR);

    act("\tL$n cackles with glee at the fray, enjoying every second of the battle\r\n"
        "\tL $s sets her gaze upon you with the most wicked grin you have ever known.",
        FALSE, ch, 0, vict, TO_VICT);
    act("\tWAAAHHHH! You SCREAM in agony, a pain more intense than you have ever felt!\r\n"
        "\tWAs you flail in pain, you see a stream of your own life force flowing away from you..",
        FALSE, ch, 0, vict, TO_VICT);
    act("\tLAs the life drains from your body, you see $n's wicked grin staring into your "
        "soul..\tn",
        FALSE, ch, 0, vict, TO_VICT);

    act("$n \tLturns and gazes at \tn$N\tL, who freezes in place.\tn\r\n"
        "$n \tLreaches out with a skeletal claw and touches \tn$N\tL!\tn",
        TRUE, ch, 0, vict, TO_NOTVICT);
    act("\tL$N\tr SCREAMS\tL in agony, doubling over in pain so intense it makes you "
        "cringe!!\tn\r\n"
        "$n\tL literally sucks the life force from $N,\tn\r\n"
        "\tLwho crumples into a ball of unfathomable pain onto the ground...\tn",
        TRUE, ch, 0, vict, TO_NOTVICT);

    /* added a way to reduce the effectiveness of this attack -zusuk */
    if (AFF_FLAGGED(vict, AFF_DEATH_WARD) && !rand_number(0, 2))
    {
      hitpoints = damage(ch, vict, rand_number(100, MAX(100, GET_LEVEL(ch) * 20)), -1, DAM_UNHOLY,
                         FALSE); // type -1 = no dam message
    }
    else
    {
      if (GET_HIT(vict) <= 20)
      { /* try to finish the victim */
        hitpoints = damage(ch, vict, rand_number(100, MAX(100, GET_LEVEL(ch) * 20)), -1, DAM_UNHOLY,
                           FALSE); // type -1 = no dam message
      }
      else
      {
        hitpoints = GET_HIT(vict);
        GET_HIT(vict) = 21;
      }
    }

    /* heal/vamp effect from the attack */
    if (hitpoints < 120)
      hitpoints = 120;

    GET_HIT(ch) += hitpoints;

    return 1;
  }

  return 0;
}

/* custom mob code for vampire mobs -zusuk */
SPECIAL(vampire_mob)
{
  if (cmd)
    return 0;

  if (!ch)
    return 0;

  int rejuv = 0;
  struct char_data *vict = FIGHTING(ch);
  struct obj_data *corpse = NULL;

  /* this is the vampire's defensive arsenal */
  if (!rand_number(0, 6) && GET_HIT(ch) < GET_MAX_HIT(ch))
  {
    rejuv = GET_HIT(ch) + dice(10, GET_LEVEL(ch));

    if (rejuv > GET_MAX_HIT(ch))
      rejuv = GET_MAX_HIT(ch);

    GET_HIT(ch) = rejuv;

    if (vict) /* flavor messages */
    {
      if (rand_number(0, 1))
        act("\tr$n turns into gaseous form to escape the fray then turns back into vampire "
            "form!\tn",
            FALSE, ch, 0, 0, TO_ROOM);
      else
        act("\tr$n turns into a bat to escape the fray then turns back into vampire form!\tn",
            FALSE, ch, 0, 0, TO_ROOM);
    }

    act("\trThe wounds on $n's body begin to close as $e is regenerated!\tn", FALSE, ch, 0, 0,
        TO_ROOM);

    if (vict) /* flavor messages */
    {
      act("\tr$n returns to the fray!\tn", FALSE, ch, 0, 0, TO_ROOM);
    }

    /* removed the call to return here to make sure we can process an offensive proc */
    // return 1;
  }

  /* this is the vampire's regular form offensive arsenal */
  if (vict)
  {
    /* make sure we have our followers! */
    if (!PROC_FIRED(ch))
    {
      /* set up a group if we don't have one */
      if (!GROUP(ch))
      {
        create_group(ch);
      }

      /* get our children of the night first! */
      act("You reach out into the wilds to pull forth your children of the night.", FALSE, ch, 0, 0,
          TO_CHAR);
      act("$n reaches out into the wilds to pull forth children of the night.", FALSE, ch, 0, 0,
          TO_ROOM);
      call_magic(ch, ch, 0, VAMPIRE_ABILITY_CHILDREN_OF_THE_NIGHT, 0, GET_LEVEL(ch), CAST_INNATE);

      /* now create our vampire spawn */
      act("You turn to a nearby minion, grab him by the neck, and with a smile snap his neck.",
          FALSE, ch, 0, 0, TO_CHAR);
      act("$n turns to a nearby minion, grabs him by the neck, and with a smile snaps his neck.  "
          "The fresh corpse conveniently lays before $n.",
          FALSE, ch, 0, 0, TO_ROOM);

      /* this creates a generic corpse */
      corpse = make_a_corpse_4_npcs(ch);
      if (corpse)
      {
        /* messaging and actual call fo spell if we got a corpse */
        act("You draw upon your vampiric strength and attempt to convert $p into vampiric spawn",
            FALSE, ch, corpse, 0, TO_CHAR);
        act("$n draws upon vampiric strength and attempts to convert $p into vampiric spawn", FALSE,
            ch, corpse, 0, TO_ROOM);
        call_magic(ch, ch, corpse, ABILITY_CREATE_VAMPIRE_SPAWN, 0, GET_LEVEL(ch), CAST_INNATE);
      }

      /* done */
      PROC_FIRED(ch) = TRUE;
    }

    /* vampire bite */
    if (!rand_number(0, 3))
    {
      act("$n sinks $s fangs into $N!", 1, ch, 0, vict, TO_NOTVICT);
      act("$n sinks $s fangs into you!", 1, ch, 0, vict, TO_VICT);
      call_magic(ch, vict, 0, SPELL_POISON, 0, GET_LEVEL(ch), CAST_INNATE);
      damage(ch, vict, rand_number(6, MAX(6, GET_LEVEL(ch))), -1, DAM_POISON, FALSE);

      return 1;
    }
    /* blood drain */
    else if (!rand_number(0, 3))
    {
      act("You quickly pin $N.", FALSE, ch, 0, vict, TO_CHAR);
      act("$n briefly pins $N!", 1, ch, 0, vict, TO_NOTVICT);
      act("$n briefly pins you!", 1, ch, 0, vict, TO_VICT);
      vamp_blood_drain(ch, vict);

      return 1;
    }
    /* vicious attacks */
    else if (!rand_number(0, 3))
    {
      int i = 0;

      act("$n acts with inhuman speed!", 1, ch, 0, NULL, TO_ROOM);

      /* spam some attacks */
      for (i = 0; i <= rand_number(3, 6); i++)
      {
        if (valid_fight_cond(ch, TRUE))
          hit(ch, FIGHTING(ch), TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, FALSE);
      }

      return 1;
    }
  }

  return 0;
}

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

SPECIAL(cityguard)
{
  struct char_data *tch, *evil, *spittle;
  int max_evil, min_cha;

  if (cmd || !AWAKE(ch) || FIGHTING(ch))
    return (FALSE);

  max_evil = 1000;
  min_cha = 6;
  spittle = evil = NULL;

  for (tch = world[IN_ROOM(ch)].people; tch; tch = tch->next_in_room)
  {
    if (!CAN_SEE(ch, tch))
      continue;
    if (!IS_NPC(tch) && PLR_FLAGGED(tch, PLR_KILLER))
    {
      act("$n screams 'HEY!!!  You're one of those PLAYER KILLERS!!!!!!'", FALSE, ch, 0, 0,
          TO_ROOM);
      hit(ch, tch, TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, FALSE);
      return (TRUE);
    }

    if (!IS_NPC(tch) && PLR_FLAGGED(tch, PLR_THIEF))
    {
      act("$n screams 'HEY!!!  You're one of those PLAYER THIEVES!!!!!!'", FALSE, ch, 0, 0,
          TO_ROOM);
      hit(ch, tch, TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, FALSE);
      return (TRUE);
    }

    if (FIGHTING(tch) && GET_ALIGNMENT(tch) < max_evil && (IS_NPC(tch) || IS_NPC(FIGHTING(tch))))
    {
      max_evil = GET_ALIGNMENT(tch);
      evil = tch;
    }

    if (GET_CHA(tch) < min_cha)
    {
      spittle = tch;
      min_cha = GET_CHA(tch);
    }
  }

  /*
  if (evil && GET_ALIGNMENT(FIGHTING(evil)) >= 0) {
    act("$n screams 'PROTECT THE INNOCENT!  BANZAI!  CHARGE!  ARARARAGGGHH!'", FALSE, ch, 0, 0, TO_ROOM);
    hit(ch, evil, TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, FALSE);
    return (TRUE);
  }
   */

  /* Reward the socially inept. */
  if (spittle && !rand_number(0, 9))
  {
    static int spit_social;

    if (!spit_social)
      spit_social = find_command("spit");

    if (spit_social > 0)
    {
      char spitbuf[MAX_NAME_LENGTH + 1];
      strncpy(spitbuf, GET_NAME(spittle), sizeof(spitbuf)); /* strncpy: OK */
      spitbuf[sizeof(spitbuf) - 1] = '\0';
      do_action(ch, spitbuf, spit_social, 0);
      return (TRUE);
    }
  }
  return (FALSE);
}

SPECIAL(clan_cleric)
{
  int i;
  char buf[MAX_STRING_LENGTH] = {'\0'};
  zone_vnum clanhall;
  clan_vnum clan;
  struct char_data *this_mob = (struct char_data *)me;

  struct price_info
  {
    short int number;
    char name[25];
    short int price;
  } clan_prices[] = {/* Spell Num (defined)      Name shown        Price  */
                     {SPELL_SHIELD_OF_FAITH, "shield of faith  ", 75},
                     {SPELL_BLESS, "bless            ", 150},
                     {SPELL_REMOVE_POISON, "remove poison    ", 525},
                     {SPELL_CURE_BLIND, "cure blindness   ", 375},
                     {SPELL_CURE_CRITIC, "critical         ", 525},
                     {SPELL_SANCTUARY, "sanctuary       ", 3000},
                     {SPELL_HEAL, "heal            ", 3500},

                     /* The next line must be last, add new spells above. */
                     {-1, "\r\n", -1}};

  if (CMD_IS("buy") || CMD_IS("list"))
  {
    argument = one_argument_u(argument, buf);

    /* Which clanhall is this cleric in? */
    clanhall = zone_table[(GET_ROOM_ZONE(IN_ROOM(this_mob)))].number;
    if ((clan = zone_is_clanhall(clanhall)) == NO_CLAN)
    {
      log("SYSERR: clan_cleric spec (%s) not in a known clanhall (room %d)", GET_NAME(this_mob),
          world[(IN_ROOM(this_mob))].number);
      return FALSE;
    }
    if (clan != GET_CLAN(ch))
    {
      snprintf(buf, sizeof(buf), "$n will only serve members of %s", CLAN_NAME(real_clan(clan)));
      act(buf, TRUE, this_mob, 0, ch, TO_VICT);
      return TRUE;
    }

    if (FIGHTING(ch))
    {
      send_to_char(ch, "You can't do that while fighting!\r\n");
      return TRUE;
    }

    if (*buf)
    {
      for (i = 0; clan_prices[i].number > SPELL_RESERVED_DBC; i++)
      {
        if (is_abbrev(buf, clan_prices[i].name))
        {
          if (GET_GOLD(ch) < clan_prices[i].price)
          {
            act("$n tells you, 'You don't have enough gold for that spell!'", FALSE, this_mob, 0,
                ch, TO_VICT);
            return TRUE;
          }
          else
          {
            act("$N gives $n some money.", FALSE, this_mob, 0, ch, TO_NOTVICT);
            send_to_char(ch, "You give %s %d coins.\r\n", GET_NAME(this_mob), clan_prices[i].price);
            decrease_gold(ch, clan_prices[i].price);
            /* Uncomment the next line to make the mob get RICH! */
            /* increase_gold(this_mob, clan_prices[i].price); */

            cast_spell(this_mob, ch, NULL, clan_prices[i].number, 0);
            return TRUE;
          }
        }
      }
      act("$n tells you, 'I do not know of that spell!"
          "  Type 'buy' for a list.'",
          FALSE, this_mob, 0, ch, TO_VICT);

      return TRUE;
    }
    else
    {
      act("$n tells you, 'Here is a listing of the prices for my services.'", FALSE, this_mob, 0,
          ch, TO_VICT);
      for (i = 0; clan_prices[i].number > SPELL_RESERVED_DBC; i++)
      {
        send_to_char(ch, "%s%d\r\n", clan_prices[i].name, clan_prices[i].price);
      }
      return TRUE;
    }
  }
  return FALSE;
}

SPECIAL(clan_guard)
{
  zone_vnum clanhall, to_zone;
  clan_vnum clan;
  struct char_data *guard = (struct char_data *)me;
  const char *buf = "The guard humiliates you, and blocks your way.\r\n";
  const char *buf2 = "The guard humiliates $n, and blocks $s way.";

  if (!IS_MOVE(cmd) || IS_AFFECTED(guard, AFF_BLIND))
    return FALSE;

  if (GET_LEVEL(ch) >= LVL_IMMORT)
    return FALSE;

  /* Which clanhall is this cleric in? */
  clanhall = zone_table[(GET_ROOM_ZONE(IN_ROOM(guard)))].number;
  if ((clan = zone_is_clanhall(clanhall)) == NO_CLAN)
  {
    log("SYSERR: clan_guard spec (%s) not in a known clanhall (room %d)", GET_NAME(guard),
        world[(IN_ROOM(guard))].number);
    return FALSE;
  }

  /* This is the player's clanhall, allow them to pass */
  if (GET_CLAN(ch) == clan)
  {
    return FALSE;
  }

  /* If the exit leads to another clanhall room, block it */
  /* NOTE: cmd equals the direction for directional commands */
  if (EXIT(ch, cmd) && EXIT(ch, cmd)->to_room && EXIT(ch, cmd)->to_room != NOWHERE)
  {
    to_zone = zone_table[(GET_ROOM_ZONE(EXIT(ch, cmd)->to_room))].number;
    if (to_zone == clanhall)
    {
      act(buf, FALSE, ch, 0, 0, TO_CHAR);
      act(buf2, FALSE, ch, 0, 0, TO_ROOM);
      return TRUE;
    }
  }

  /* If we get here, player is allowed to leave */
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
SPECIAL(dog)
{
  int random = 0;
  struct affected_type af;
  struct char_data *pet = (struct char_data *)me;

  if (!argument)
    return FALSE;
  if (!cmd)
    return FALSE;

  skip_spaces(&argument);

  if (!isname(argument, GET_NAME(pet)))
    return FALSE;

  if (CMD_IS("pet") || CMD_IS("pat"))
  {
    random = dice(1, 3);
    switch (random)
    {
    case 3:
      act("$n tries to lick your hand as you pet $m.", FALSE, pet, 0, ch, TO_VICT);
      act("$n tries to lick the hand of $N as $E pet $m.", FALSE, pet, 0, ch, TO_NOTVICT);
      break;
    case 2:
      act("$n looks at you with adoring eyes as you pet $m.", FALSE, pet, 0, ch, TO_VICT);
      act("$n looks at $N with adoring eyes as $E pet $m.", FALSE, pet, 0, ch, TO_NOTVICT);
      break;
    case 1:
    default:
      act("$n wags $s tail happily, as you pet $m.", FALSE, pet, 0, ch, TO_VICT);
      act("$n wags $s tail happily, as $N pets $m.", FALSE, pet, 0, ch, TO_NOTVICT);
      break;
    }

    if (GET_LEVEL(pet) < 2 && ch->followers == 0 && ch->master == 0 && pet->master == 0 &&
        !circle_follow(pet, ch))
    {
      add_follower(pet, ch);
      af.spell = SPELL_CHARM;
      af.duration = 24000;
      af.modifier = 0;
      af.location = 0;
      SET_BIT_AR(af.bitvector, AFF_CHARM);
      affect_to_char(pet, &af);
    }
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

/* from Homeland */
// doesnt work properly if multiple instances.. :) -V

SPECIAL(practice_dummy)
{
  int rounddam = 0;
  static int round_count;
  static int max_hit;
  char buf[MAX_INPUT_LENGTH] = {'\0'};

  if (cmd)
    return FALSE;

  if (!FIGHTING(ch))
  {
    GET_MAX_HIT(ch) = 20000;
    GET_HIT(ch) = 20000;
    max_hit = 0;
    round_count = 0;
  }
  else
  {
    rounddam = GET_MAX_HIT(ch) - GET_HIT(ch);
    max_hit += rounddam;
    round_count++;

    snprintf(buf, sizeof(buf), "\tP%d damage last round!\tn  \tc(total: %d rounds: %d)\tn\r\n",
             rounddam, max_hit, round_count);
    send_to_room(ch->in_room, "%s", buf);
    GET_HIT(ch) = GET_MAX_HIT(ch);
    return TRUE;
  }
  return FALSE;
}

/* from Homeland */
SPECIAL(wraith)
{
  if (cmd)
    return FALSE;

  if (GET_POS(ch) == POS_DEAD || !ch->master)
  {
    act("With a loud shriek, $n crumbles into dust.", FALSE, ch, NULL, 0, TO_ROOM);
    extract_char(ch);
    return TRUE;
  }

  if (ch->master && ch->in_room == ch->master->in_room)
    if (FIGHTING(ch->master) && rand_number(0, 1))
    {
      perform_assist(ch, ch->master);
      return TRUE;
    }

  return FALSE;
}

/* from Homeland */
SPECIAL(skeleton_zombie)
{
  if (cmd)
    return FALSE;

  if (GET_POS(ch) == POS_DEAD || !ch->master)
  {
    act("With a loud shriek, $n crumbles into dust.", FALSE, ch, NULL, 0, TO_ROOM);
    extract_char(ch);
    return TRUE;
  }

  if (ch->master && ch->in_room == ch->master->in_room)
    if (FIGHTING(ch->master) && !rand_number(0, 2))
    {
      perform_assist(ch, ch->master);
      return TRUE;
    }

  return FALSE;
}

/* from Homeland */
SPECIAL(vampire)
{
  struct char_data *vict;

  if (cmd)
    return FALSE;

  if (GET_POS(ch) == POS_DEAD || !ch->master)
  {
    act("With a loud shriek, $n crumbles into dust.", FALSE, ch, NULL, 0, TO_ROOM);
    extract_char(ch);
    return TRUE;
  }

  if (ch->master && ch->in_room == ch->master->in_room)
  {
    for (vict = world[ch->in_room].people; vict; vict = vict->next_in_room)
    {
      if (FIGHTING(vict) == ch->master && !rand_number(0, 1))
      {
        perform_rescue(ch, ch->master);
        return TRUE;
      }
    }
  }

  return FALSE;
}

/* from Homeland */
SPECIAL(totemanimal)
{
  if (cmd)
    return FALSE;
  if (!ch->master)
    return FALSE;

  if (ch->master && ch->in_room == ch->master->in_room)
    if (FIGHTING(ch->master))
      perform_assist(ch, ch->master);
  return FALSE;
}

/* from Homeland */
SPECIAL(shades)
{
  if (cmd)
    return FALSE;

  if (GET_MAX_HIT(ch) > 1 && GET_HIT(ch) > 1)
  {
    GET_MAX_HIT(ch) = 1;
    GET_HIT(ch) = 1;
  }

  if (GET_POS(ch) == POS_DEAD)
    return FALSE;
  if (GET_HIT(ch) < GET_MAX_HIT(ch) || !ch->master)
  {
    act("A shade evaporates into thin air.", FALSE, ch, NULL, 0, TO_ROOM);
    extract_char(ch);
    return TRUE;
  }

  if (ch->in_room != ch->master->in_room)
  {
    HUNTING(ch) = ch->master;
    hunt_victim(ch);
    return TRUE;
  }
  return FALSE;
}

/* from Homeland */
SPECIAL(solid_elemental)
{
  struct char_data *vict;

  if (cmd)
    return FALSE;

  if (GET_POS(ch) == POS_DEAD || (!ch->master && !MOB_FLAGGED(ch, MOB_MEMORY)))
  {
    act("With a loud shriek, $n returns to $s home plane.", FALSE, ch, NULL, 0, TO_ROOM);
    extract_char(ch);
    return TRUE;
  }

  if (GET_HIT(ch) > 0)
  {
    if (ch->master && ch->in_room == ch->master->in_room && !rand_number(0, 1))
    {
      for (vict = world[ch->in_room].people; vict; vict = vict->next_in_room)
      {
        if (FIGHTING(vict) == ch->master)
        {
          perform_rescue(ch, ch->master);
          return TRUE;
        }
      }
    }

    if (!FIGHTING(ch) && ch->master && FIGHTING(ch->master) && ch->in_room == ch->master->in_room)
    {
      perform_assist(ch, ch->master);
      return TRUE;
    }
  }

  // auto stand if down
  if (GET_POS(ch) < POS_FIGHTING && GET_POS(ch) >= POS_STUNNED)
  {
    change_position(ch, POS_STANDING);
    act("$n clambers to $s feet.\r\n", FALSE, ch, 0, 0, TO_ROOM);
    return TRUE;
  }

  // we're fighting something we dont want to fight...
  if (!ch->master && FIGHTING(ch) && IS_NPC(FIGHTING(ch)) && !IS_PET(FIGHTING(ch)))
    do_flee(ch, 0, 0, 0);

  return FALSE;
}

/* from Homeland */
SPECIAL(wraith_elemental)
{
  struct char_data *vict;

  if (cmd)
    return FALSE;

  if (GET_POS(ch) == POS_DEAD || (!ch->master && !MOB_FLAGGED(ch, MOB_MEMORY)))
  {
    act("With a loud shriek, $n returns to $s home plane.", FALSE, ch, NULL, 0, TO_ROOM);
    extract_char(ch);
    return TRUE;
  }

  if (GET_HIT(ch) > 0)
  {
    if (ch->master && ch->in_room == ch->master->in_room && !rand_number(0, 1))
    {
      for (vict = world[ch->in_room].people; vict; vict = vict->next_in_room)
      {
        if (FIGHTING(vict) == ch->master)
        {
          perform_rescue(ch, ch->master);
          return TRUE;
        }
      }
    }

    if (!FIGHTING(ch) && ch->master && FIGHTING(ch->master) && ch->in_room == ch->master->in_room)
    {
      perform_assist(ch, ch->master);
      return TRUE;
    }
  }

  // auto stand if down
  if (GET_POS(ch) < POS_FIGHTING && GET_POS(ch) >= POS_STUNNED)
  {
    change_position(ch, POS_STANDING);
    act("$n clambers to $s feet.\r\n", FALSE, ch, 0, 0, TO_ROOM);
    return TRUE;
  }

  // we're fighting something we dont want to fight...
  if (!ch->master && FIGHTING(ch) && IS_NPC(FIGHTING(ch)) && !IS_PET(FIGHTING(ch)))
    do_flee(ch, 0, 0, 0);

  return FALSE;
}

/* from homeland */
SPECIAL(planewalker)
{
  if (cmd)
    return FALSE;

  if (ROOM_FLAGGED(ch->in_room, ROOM_SOUNDPROOF))
  {
    act("$n looks around in panic when he realizes that his spells\r\n"
        "would fizzle. He reaches down into his pockets and pulls out an ancient\r\n"
        "rod. He taps the rod and suddenly disappears!",
        FALSE, ch, 0, 0, TO_ROOM);
    call_magic(ch, 0, 0, SPELL_TELEPORT, 0, 30, CAST_WAND);
    return TRUE;
  }
  if (!FIGHTING(ch) && GET_HIT(ch) < GET_MAX_HIT(ch))
  {
    act("$n checks on his wounds, and grabs a potion from his pockets.", FALSE, ch, 0, 0, TO_ROOM);
    call_magic(ch, ch, 0, SPELL_HEAL, 0, 30, CAST_POTION);
    return TRUE;
  }
  return FALSE;
}

/* from homeland */
SPECIAL(phantom)
{
  struct char_data *vict;
  struct char_data *next_vict;
  int prob, percent;

  if (cmd)
    return FALSE;

  if (!FIGHTING(ch))
    return FALSE;
  if (rand_number(0, 4))
    return FALSE;

  act("$n \tLlets out a \trfrightening\tL wail\tn", FALSE, ch, 0, 0, TO_ROOM);

  for (vict = world[ch->in_room].people; vict; vict = next_vict)
  {
    next_vict = vict->next_in_room;

    if (vict == ch)
      continue;
    if (IS_NPC(vict) && !IS_PET(vict))
      continue;

    percent = rand_number(1, 111); /* 101% is a complete failure */
    prob = GET_WIS(vict) + 5;
    if (FIGHTING(vict))
      prob *= 2;
    if (prob > 100)
      prob = 100;

    if (percent > prob)
      do_flee(vict, NULL, 0, 0);
  }
  return TRUE;
}

/* this is the old lichdrain, don't think it works in its current
   implementation */
int perform_lichdrain(struct char_data *ch)
{
  if (!ch)
    return 0;

  struct char_data *tch = 0;
  struct char_data *vict = 0;
  int dam = 0;

  if (GET_POS(ch) == POS_DEAD)
    return FALSE;
  if (rand_number(0, 3))
    return FALSE;
  if (!FIGHTING(ch))
    return FALSE;

  if (AFF_FLAGGED(ch, AFF_PARALYZED))
    return FALSE;

  for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room)
  {
    if (!IS_NPC(tch) || IS_PET(tch))
    {
      if (!vict || !rand_number(0, 2))
      {
        vict = tch;
      }
    }
  }

  if (!vict)
    return FALSE;

  act("\tn$n\tL looks deep into your soul with $s horrid gaze.\tn\r\n"
      "\tLand $e simply leeches your \tWlifeforce\tL out of you.\r\n",
      FALSE, ch, 0, vict, TO_VICT);

  act("\tn$n\tL looks deep into the eyes of $N\tL with $s horrid gaze.\tn\r\n"
      "\tLand $e simply leeches $S \tWlifeforce\tL out of $M.\r\n",
      TRUE, ch, 0, vict, TO_NOTVICT);

  act("\tWYou reach out and suck the life force away from $N!", TRUE, ch, 0, vict, TO_CHAR);
  dam = GET_HIT(vict) + 5;
  if (GET_HIT(ch) + dam < GET_MAX_HIT(ch))
    GET_HIT(ch) += dam;
  GET_HIT(vict) -= dam;
  USE_FULL_ROUND_ACTION(vict);
  return TRUE;
}

/* threw this together so the experience of encountering lich isn't a pleasant one :P */
SPECIAL(lich_mob)
{
  struct char_data *vict = NULL;
  int use_aoe = 0;

  if (!ch)
    return 0;

  /* note that the !vict is moved below */
  if (cmd)
    return 0;

  /* find random target, and num targets */
  if (!(vict = npc_find_target(ch, &use_aoe)))
    return 0;

  /* this is the offensive arsenal */
  if (vict && rand_number(0, 1))
  {
    if (!rand_number(0, 5))
    {
      act("\tWWith power and determination you unleash an aggressive BURST of magic!\tn", TRUE, ch,
          0, FIGHTING(ch), TO_CHAR);
      act("$n\tL, with power and determination, unleashes an aggressive BURST of magic!\tn", FALSE,
          ch, 0, FIGHTING(ch), TO_VICT);
      act("$n\tL, with power and determination, unleashes an aggressive BURST of magic!\tn", TRUE,
          ch, 0, FIGHTING(ch), TO_NOTVICT);

      /* looks like the swarm worked */
      if (call_magic(ch, vict, 0, SPELL_METEOR_SWARM, 0, GET_LEVEL(ch), CAST_INNATE))
        return 1;
    }
    else if (!rand_number(0, 2) && (!IS_UNDEAD(vict) && !IS_LICH(vict)) &&
             perform_lichtouch(ch, vict))
    {
      /* looks like we did the lichtouch! */
      return 1;
    }
    else if (!rand_number(0, 2) && (IS_UNDEAD(ch) || IS_LICH(ch)) && perform_lichtouch(ch, ch))
    {
      /* looks like we did the self healing lichtouch */
      return 1;
    }
    else if (!rand_number(0, 4))
    {
      int i = 0;

      act("\tWWith power and determination you unleash an aggressive flurry of magic!\tn", TRUE, ch,
          0, FIGHTING(ch), TO_CHAR);
      act("$n\tL, with power and determination, unleashes an aggressive flurry of magic!\tn", FALSE,
          ch, 0, FIGHTING(ch), TO_VICT);
      act("$n\tL, with power and determination, unleashes an aggressive flurry of magic!\tn", TRUE,
          ch, 0, FIGHTING(ch), TO_NOTVICT);

      /* spam some nukes! */
      for (i = 0; i <= rand_number(1, 3); i++)
      {
        if (valid_fight_cond(ch, TRUE))
        {
          switch (rand_number(0, 2))
          {
          case 0:
            call_magic(ch, vict, 0, SPELL_PRISMATIC_SPRAY, 0, GET_LEVEL(ch), CAST_INNATE);
            break;
          case 1:
            call_magic(ch, vict, 0, SPELL_CHAIN_LIGHTNING, 0, GET_LEVEL(ch), CAST_INNATE);
            break;
          default:
            call_magic(ch, vict, 0, SPELL_THUNDERCLAP, 0, GET_LEVEL(ch), CAST_INNATE);
            break;
          }
        }
      }
      return 1;
    }
  }

  return 0;
}

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
SPECIAL(bonedancer)
{
  struct char_data *vict;
  struct char_data *next_vict;

  if (cmd)
    return FALSE;
  if (GET_POS(ch) == POS_DEAD || !ch->master)
  {
    act("With a loud shriek, $n crumbles into dust.", FALSE, ch, NULL, 0, TO_ROOM);
    extract_char(ch);
    return TRUE;
  }

  if (!FIGHTING(ch) && GET_HIT(ch) > 0)
  {
    for (vict = world[ch->in_room].people; vict; vict = next_vict)
    {
      next_vict = vict->next_in_room;
      if (vict != ch && CAN_SEE(ch, vict))
      {
        hit(ch, vict, TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, FALSE);
        return TRUE;
      }
    }
  }

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
SPECIAL(mercenary)
{
  int hit;
  int base = 1;

  if (!ch)
    return FALSE;
  if (cmd)
    return FALSE;

  // a recruited merc should get reasonable amounts of hp.
  if (PROC_FIRED(ch) == FALSE && IS_PET(ch))
  {
    switch (GET_CLASS(ch))
    {
    case CLASS_RANGER:
    case CLASS_PALADIN:
    case CLASS_BERSERKER:
    case CLASS_WARRIOR:
    case CLASS_WEAPON_MASTER:
    case CLASS_STALWART_DEFENDER:
    case CLASS_DUELIST:
      base = 8;
      break;
    case CLASS_ROGUE:
      //      case CLASS_SHADOW_DANCER:
      //      case CLASS_ASSASSIN:
    case CLASS_MONK:
    case CLASS_SACRED_FIST:
    case CLASS_SHIFTER:
      base = 5;
      break;

    default:
      base = 3;
      break;
    }

    hit = dice(GET_LEVEL(ch), (1 + GET_CON_BONUS(ch))) + GET_LEVEL(ch) * base;
    GET_MAX_HIT(ch) = hit;
    if (GET_HIT(ch) > hit)
      GET_HIT(ch) = hit;
    PROC_FIRED(ch) = TRUE;
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
SPECIAL(ethereal_pet)
{
  if (cmd || GET_POS(ch) == POS_DEAD)
    return FALSE;
  if (FIGHTING(ch))
    return FALSE;

  if (ch->desc == 0)
  {
    extract_char(ch);
    return TRUE;
  }
  return FALSE;
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

/* research spells for wizard - this is meant to fill the gap in the game we
   have for lack of scroll placement, special thanks to Stephen Squires for
   parts of the code */
SPECIAL(wizard_library)
{
  bool found = FALSE, full_spellbook = TRUE;
  struct obj_data *obj = NULL;
  int spellnum = SPELL_RESERVED_DBC, spell_level = 0, spell_circle = 0, cost = 100, i = 0;
  int class_level = 0;

  if (CMD_IS("research"))
  {
    if (!CLASS_LEVEL(ch, CLASS_WIZARD))
    {
      send_to_char(ch, "You are not a wizard!\r\n");
      return TRUE;
    }

    skip_spaces(&argument);

    if (!*argument)
    {
      send_to_char(ch, "You need to indicate which spell you want to research.\r\n");
      return TRUE;
    }

    spellnum = find_skill_num(argument);

    if (spellnum <= SPELL_RESERVED_DBC || spellnum >= NUM_SPELLS)
    {
      send_to_char(ch, "Invalid spell!\r\n");
      return TRUE;
    }

    spell_level = spell_info[spellnum].min_level[CLASS_WIZARD];
    spell_circle = (spell_level + 1) / 2;
    class_level = CLASS_LEVEL(ch, CLASS_WIZARD) + BONUS_CASTER_LEVEL(ch, CLASS_WIZARD);

    if (spell_level <= 0 || spell_level >= LVL_IMMORT || spell_circle <= 0 ||
        spell_circle > TOP_CIRCLE)
    {
      send_to_char(ch, "That spell is not available to wizards.\r\n");
      return TRUE;
    }

    /* Check if spell is flagged as no_player (not available to players) */
    if (spell_info[spellnum].no_player)
    {
      send_to_char(ch, "That spell cannot be researched by wizards.\r\n");
      return TRUE;
    }

    if (spell_circle < 7)
    {
      cost = (spell_circle * 50) * (spell_circle);
    }
    else
      cost = (spell_circle * 300) * (spell_circle);

    if (GET_GOLD(ch) < cost)
    {
      send_to_char(ch,
                   "You do not have enough coins to research this spell, you "
                   "need %d coins.\r\n",
                   cost);
      return TRUE;
    }

    if (class_level >= spell_level && GET_SKILL(ch, spellnum))
    {
      /* 1st make sure we have a spellbook handy */
      /* for-loop for inventory */
      for (obj = ch->carrying; obj && !found; obj = obj->next_content)
      {
        if (GET_OBJ_TYPE(obj) == ITEM_SPELLBOOK)
        {
          if (spell_in_book(obj, spellnum))
          {
            send_to_char(ch, "You already have the spell '%s' in this spellbook.\r\n",
                         spell_info[spellnum].name);
            return TRUE;
          }
          /* found a spellbook that doesn't have the spell! */
          found = TRUE;
          break; /* our obj variable is now pointing to this spellbook */
        }
      }

      /* for-loop for gear */
      if (!found)
      {
        for (i = 0; i < NUM_WEARS; i++)
        {
          if (GET_EQ(ch, i))
            obj = GET_EQ(ch, i);
          else
            continue;

          if (GET_OBJ_TYPE(obj) == ITEM_SPELLBOOK)
          {
            if (spell_in_book(obj, spellnum))
            {
              send_to_char(ch, "You already have the spell '%s' in this spellbook.\r\n",
                           spell_info[spellnum].name);
              return TRUE;
            }
            /* found a spellbook that doesn't have the spell! */
            found = TRUE;
            break; /* our obj variable is now pointing to this spellbook */
          }
        }
      }
    }
    else
    {
      send_to_char(ch, "You are not powerful enough to scribe that spell! (or this spell is from a "
                       "restricted school)\r\n");
      return TRUE;
    }

    if (!found)
    {
      send_to_char(ch, "No ready spellbook found in your inventory or equipped...\r\n");
      return TRUE;
    }

    /* ok obj variable pointing to spellbook, let's make sure it has space */
    if (!obj->sbinfo)
    { /* un-initialized spellbook, allocate memory */
      CREATE(obj->sbinfo, struct obj_spellbook_spell, SPELLBOOK_SIZE);
      memset((char *)obj->sbinfo, 0, SPELLBOOK_SIZE * sizeof(struct obj_spellbook_spell));
    }
    for (i = 0; i < SPELLBOOK_SIZE; i++)
    { /* check for space */
      if (obj->sbinfo[i].spellname == 0)
      {
        full_spellbook = FALSE;
        break;
      }
      else
      {
        continue;
      }
    } /* i = location in spellbook */

    if (full_spellbook)
    {
      send_to_char(ch, "There is not enough space in that spellbook!\r\n");
      return TRUE;
    }

    /* we made it! */
    GET_GOLD(ch) -= cost;
    obj->sbinfo[i].spellname = spellnum;
    obj->sbinfo[i].pages = MAX(1, lowest_spell_level(spellnum) / 2);
    send_to_char(ch,
                 "Your research is successful and you scribe the spell '%s' "
                 "into your spellbook, which takes up %d pages and cost %d coins.\r\n",
                 spell_info[spellnum].name, obj->sbinfo[i].pages, cost);

    USE_FULL_ROUND_ACTION(ch);
    return TRUE;
  }

  /* they did not type research */
  return FALSE;
}

SPECIAL(dump)
{
  struct obj_data *k;
  int value = 0;

  for (k = world[IN_ROOM(ch)].contents; k; k = world[IN_ROOM(ch)].contents)
  {
    act("$p vanishes in a puff of smoke!", FALSE, 0, k, 0, TO_ROOM);
    extract_obj(k);
  }

  if (!CMD_IS("drop"))
    return (FALSE);

  do_drop(ch, argument, cmd, SCMD_DROP);

  for (k = world[IN_ROOM(ch)].contents; k; k = world[IN_ROOM(ch)].contents)
  {
    act("$p vanishes in a puff of smoke!", FALSE, 0, k, 0, TO_ROOM);
    value += MAX(1, MIN(50, GET_OBJ_COST(k) / 10));
    extract_obj(k);
  }

  if (value)
  {
    send_to_char(ch, "You are awarded for outstanding performance.\r\n");
    act("$n has been awarded for being a good citizen.", TRUE, ch, 0, 0, TO_ROOM);

    if (GET_LEVEL(ch) < 3)
      gain_exp(ch, value, GAIN_EXP_MODE_DUMP);
    else
      increase_gold(ch, value);
  }
  return (TRUE);
}

#define PET_PRICE(pet) (GET_LEVEL(pet) * 300)

SPECIAL(pet_shops)
{
  char buf[MAX_STRING_LENGTH] = {'\0'}, pet_name[MEDIUM_STRING] = {'\0'};
  room_rnum pet_room;
  struct char_data *pet;

  /* Gross. */
  pet_room = IN_ROOM(ch) + 1;

  if (CMD_IS("list"))
  {
    send_to_char(ch, "Available pets are:\r\n");
    for (pet = world[pet_room].people; pet; pet = pet->next_in_room)
    {
      /* No, you can't have the Implementor as a pet if he's in there. */
      if (!IS_NPC(pet))
        continue;
      send_to_char(ch, "%8d - %s\r\n", PET_PRICE(pet), GET_NAME(pet));
    }
    return (TRUE);
  }
  else if (CMD_IS("buy"))
  {
    two_arguments(argument, buf, sizeof(buf), pet_name, sizeof(pet_name));

    /* disqualifiers */
    if (!(pet = get_char_room(buf, NULL, pet_room)) || !IS_NPC(pet))
    {
      send_to_char(ch, "There is no such pet!\r\n");
      return (TRUE);
    }
    if (GET_GOLD(ch) < PET_PRICE(pet))
    {
      send_to_char(ch, "You don't have enough gold!\r\n");
      return (TRUE);
    }
    // if (check_npc_followers(ch, NPC_MODE_SPARE, 0) <= 0)
    if (!can_add_follower(ch, GET_MOB_VNUM(pet)))
    {
      send_to_char(ch, "You can't have any more pets!\r\n");
      return (TRUE);
    }

    /* success! */
    decrease_gold(ch, PET_PRICE(pet));

    pet = read_mobile(GET_MOB_RNUM(pet), REAL);
    GET_EXP(pet) = 0;
    SET_BIT_AR(AFF_FLAGS(pet), AFF_CHARM);
    if (GET_LEVEL(pet) <= 10)
    {
      GET_REAL_MAX_HIT(pet) = GET_MAX_HIT(pet) =
          GET_MAX_HIT(pet) * CONFIG_SUMMON_LEVEL_1_10_HP / 100;
      GET_REAL_AC(pet) = GET_REAL_AC(pet) * CONFIG_SUMMON_LEVEL_1_10_AC / 100;
      GET_HITROLL(pet) = GET_HITROLL(pet) * CONFIG_SUMMON_LEVEL_1_10_HIT_DAM / 100;
      GET_DAMROLL(pet) = GET_DAMROLL(pet) * CONFIG_SUMMON_LEVEL_1_10_HIT_DAM / 100;
      pet->mob_specials.damnodice =
          pet->mob_specials.damnodice * CONFIG_SUMMON_LEVEL_1_10_HIT_DAM / 100;
      pet->mob_specials.damsizedice =
          pet->mob_specials.damsizedice * CONFIG_SUMMON_LEVEL_1_10_HIT_DAM / 100;
    }
    else if (GET_LEVEL(pet) <= 20)
    {
      GET_REAL_MAX_HIT(pet) = GET_MAX_HIT(pet) =
          GET_MAX_HIT(pet) * CONFIG_SUMMON_LEVEL_11_20_HP / 100;
      GET_REAL_AC(pet) = GET_REAL_AC(pet) * CONFIG_SUMMON_LEVEL_11_20_AC / 100;
      GET_HITROLL(pet) = GET_HITROLL(pet) * CONFIG_SUMMON_LEVEL_11_20_HIT_DAM / 100;
      GET_DAMROLL(pet) = GET_DAMROLL(pet) * CONFIG_SUMMON_LEVEL_11_20_HIT_DAM / 100;
      pet->mob_specials.damnodice =
          pet->mob_specials.damnodice * CONFIG_SUMMON_LEVEL_11_20_HIT_DAM / 100;
      pet->mob_specials.damsizedice =
          pet->mob_specials.damsizedice * CONFIG_SUMMON_LEVEL_11_20_HIT_DAM / 100;
    }
    else
    {
      GET_REAL_MAX_HIT(pet) = GET_MAX_HIT(pet) =
          GET_MAX_HIT(pet) * CONFIG_SUMMON_LEVEL_21_30_HP / 100;
      GET_REAL_AC(pet) = GET_REAL_AC(pet) * CONFIG_SUMMON_LEVEL_21_30_AC / 100;
      GET_HITROLL(pet) = GET_HITROLL(pet) * CONFIG_SUMMON_LEVEL_21_30_HIT_DAM / 100;
      GET_DAMROLL(pet) = GET_DAMROLL(pet) * CONFIG_SUMMON_LEVEL_21_30_HIT_DAM / 100;
      pet->mob_specials.damnodice =
          pet->mob_specials.damnodice * CONFIG_SUMMON_LEVEL_21_30_HIT_DAM / 100;
      pet->mob_specials.damsizedice =
          pet->mob_specials.damsizedice * CONFIG_SUMMON_LEVEL_21_30_HIT_DAM / 100;
    }
    GET_HIT(pet) = GET_MAX_HIT(pet);

    if (*pet_name)
    {
      snprintf(buf, sizeof(buf), "%s %s", pet->player.name, pet_name);
      /* free(pet->player.name); don't free the prototype! */
      pet->player.name = strdup(buf);

      snprintf(buf, sizeof(buf),
               "%sA small sign on a chain around the neck says 'My name is %s'\r\n",
               pet->player.description, pet_name);
      /* free(pet->player.description); don't free the prototype! */
      pet->player.description = strdup(buf);
    }

    if (ZONE_FLAGGED(GET_ROOM_ZONE(IN_ROOM(ch)), ZONE_WILDERNESS))
    {
      X_LOC(pet) = world[IN_ROOM(ch)].coords[0];
      Y_LOC(pet) = world[IN_ROOM(ch)].coords[1];
    }

    char_to_room(pet, IN_ROOM(ch));

    add_follower(pet, ch);

    /* Be certain that pets can't get/carry/use/wield/wear items */
    IS_CARRYING_W(pet) = 1000;
    IS_CARRYING_N(pet) = 100;

    send_to_char(ch, "May you enjoy your pet.\r\n");
    act("$n buys $N as a pet.", FALSE, ch, 0, pet, TO_ROOM);

    return (TRUE);
  }

  /* All commands except list and buy */
  return (FALSE);
}

/***********************/
/* end room procedures */
/***********************/

/********************************************************************/
/******************** Object Procs    *******************************/
/********************************************************************/

/* General object procedures are implemented in spec/spec_objects.c.
 * Commerce, crafting, vampire, quest, and zone procedures live with their owners.
 * The Celestial Leviathan stub remains here until its zone package is extracted. */

SPECIAL(celestial_leviathan)
{
  return 0;
}

/*************************/
/* end object procedures */
/*************************/

/* EoF */
