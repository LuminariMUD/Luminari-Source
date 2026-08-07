/**************************************************************************
 *  File: spec/spec_objects.c                          Part of LuminariMUD *
 *  Usage: General object special procedures.                              *
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
#include "dgscript/dg_scripts.h"
#include "mud_event.h"
#include "actions.h"
#include "combat/assign_wpn_armor.h"
#include "magic/domains_schools.h"
#include "character/feats.h"
#include "magic/spell_prep.h"
#include "obj/item.h"
#include "craft/alchemy.h"
#include "obj/treasure.h"
#include "mob/mob_utils.h"
#include "character/evolutions.h"
#include "olc/oasis.h"
#include "quest/quest.h"
#include "character/backgrounds.h"
#include "character/perks.h"

/*****************************************/
/****  object procs general functions ****/
/*****************************************/

/* this function will check the basic parameters for whether an item is ready to proc in combat */
bool obj_proc_ready(struct char_data *ch, struct obj_data *obj, int cmd)
{
  /* do we have this item equipped? */
  if (!is_wearing(ch, GET_OBJ_VNUM(obj)))
    return FALSE;

  /* valid conditions for combat? */
  if (!valid_fight_cond(ch, FALSE))
    return FALSE;

  /* was a command sent? */
  if (cmd)
    return FALSE;

  /* made it, item must be ready! */
  return TRUE;
}

/* NOT to be confused with the weapon-spells code used in OLC, etc */
/*  This was ported to accomodate the HL objects that were imported */
void weapons_spells(const char *to_ch, const char *to_vict, const char *to_room,
                    struct char_data *ch, struct char_data *vict, struct obj_data *obj, int spl)
{
  int level;

  level = GET_LEVEL(ch);

  if (level > 30)
    level = 30;

  if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_CONDENSED))
  {
  }
  else
  {
    act(to_ch, FALSE, ch, obj, vict, TO_CHAR);
  }

  if (!IS_NPC(vict) && PRF_FLAGGED(vict, PRF_CONDENSED))
  {
  }
  else
  {
    act(to_vict, FALSE, ch, obj, vict, TO_VICT);
  }

  act(to_room, ACT_CONDENSE_VALUE, ch, obj, vict, TO_NOTVICT);

  call_magic(ch, vict, 0, spl, 0, level, CAST_WEAPON_SPELL);
}

/******************************************/
/*** end object procs general functions ***/
/******************************************/

/* testing glove procs for monks, obj vnum 224 */
SPECIAL(monk_glove)
{
  if (!ch)
    return FALSE;

  if (!cmd && !strcmp(argument, "identify"))
  {
    send_to_char(ch, "Proc: Shock damage.\r\n");
    return TRUE;
  }

  struct char_data *vict = FIGHTING(ch);

  if (cmd || !vict || rand_number(0, 15))
    return FALSE;

  weapons_spells("\twYour $p\tw \tWsparks\tw as you hit $N causing $M to shudder violently from "
                 "the \tYshock\tw!\tn",
                 "$n\tw's $p\tw \tWsparks\tw as $e hits you causing you to shudder violently from "
                 "the \tYshock\tw!\tn",
                 "$n\tw's $p\tw \tWsparks\tw as $e hits $N causing $M to shudder violently from "
                 "the \tYshock\tw!\tn",
                 ch, vict, (struct obj_data *)me, 0);
  damage(ch, vict, dice(2, 8), -1, DAM_ELECTRIC, FALSE);

  return TRUE;
}

SPECIAL(monk_glove_cold)
{
  if (!ch)
    return FALSE;

  if (!cmd && !strcmp(argument, "identify"))
  {
    send_to_char(ch, "Proc: Cold damage.\r\n");
    return TRUE;
  }

  struct char_data *vict = FIGHTING(ch);

  if (cmd || !vict || rand_number(0, 15))
    return FALSE;

  weapons_spells("\twYour $p\tw \tWfrosts\tw as you hit $N causing $M to shudder violently from "
                 "the \tBcold\tw!\tn",
                 "$n\tw's $p\tw \tWfrosts\tw as $e hits you causing you to shudder violently from "
                 "the \tBcold\tw!\tn",
                 "$n\tw's $p\tw \tWfrosts\tw as $e hits $N causing $M to shudder violently from "
                 "the \tBcold\tw!\tn",
                 ch, vict, (struct obj_data *)me, 0);
  damage(ch, vict, dice(2, 8), -1, DAM_COLD, FALSE);

  return TRUE;
}

/* from homeland */
SPECIAL(spikeshield)
{
  if (!ch)
    return FALSE;

  if (!cmd && !strcmp(argument, "identify"))
  {
    send_to_char(ch, "On shieldpunch, procs 'spikes', on shieldblock procs "
                     "'life steal.'\r\n");
    return TRUE;
  }

  struct char_data *vict = FIGHTING(ch);

  if (!argument || cmd || !vict)
    return FALSE;

  // blocking
  if (!strcmp(argument, "shieldblock") && !rand_number(0, 2))
  {
    if (PRF_FLAGGED(ch, PRF_CONDENSED))
    {
    }
    else
    {
      act("\tLYour \tcshield \tCglows brightly\tL as it steals some \trlifeforce\tn "
          "\tLfrom $N \tLand transfers it back to you.\tn",
          FALSE, ch, (struct obj_data *)me, vict, TO_CHAR);
    }

    if (!IS_NPC(vict) && PRF_FLAGGED(vict, PRF_CONDENSED))
    {
    }
    else
    {
      act("$n's \tcshield \tCglows brightly\tL as it steals some \trlifeforce\tn "
          "\tLfrom you and transfers it back to $m.\tn",
          FALSE, ch, (struct obj_data *)me, vict, TO_VICT);
    }

    act("$n's \tcshield \tCglows brightly\tL as it steals some \trlifeforce\tn "
        "\tLfrom $N\tL.\tn",
        ACT_CONDENSE_VALUE, ch, (struct obj_data *)me, vict, TO_NOTVICT);

    damage(ch, vict, 15, -1, DAM_ENERGY, FALSE); // type -1 = no dam message
    call_magic(ch, ch, 0, SPELL_CURE_LIGHT, 0, 1, CAST_WEAPON_SPELL);
    return TRUE;
  }

  if (!strcmp(argument, "shieldpunch"))
  {
    if (PRF_FLAGGED(ch, PRF_CONDENSED))
    {
    }
    else
    {
      act("\tLYou slam your \tcshield \tLinto $N\tL\tn\r\n"
          "\tLcausing the rows of\tr spikes \tLto drive into $S body.\tn",
          FALSE, ch, (struct obj_data *)me, vict, TO_CHAR);
    }

    if (!IS_NPC(vict) && PRF_FLAGGED(vict, PRF_CONDENSED))
    {
    }
    else
    {
      act("$n \tLslams $s \tcshield\tL into you\tn\r\n"
          "\tLcausing the rows of \trspikes\tL to drive into your body.\tn",
          FALSE, ch, (struct obj_data *)me, vict, TO_VICT);
    }

    act("$n \tLslams $s \tcshield\tL into $N\tL\tn\r\n"
        "\tLcausing the rows of \trspikes\tL to drive into $S body.\tn",
        ACT_CONDENSE_VALUE, ch, (struct obj_data *)me, vict, TO_NOTVICT);

    damage(ch, vict, (dice(3, 8) + 4), -1, DAM_PUNCTURE,
           FALSE); // type -1 = no dam message

    return TRUE;
  }

  return FALSE;
}

/* from homeland */
SPECIAL(viperdagger)
{
  struct char_data *victim;
  int spellnum = SPELL_SLOW;

  if (!ch)
    return FALSE;

  if (!cmd && !strcmp(argument, "identify"))
  {
    send_to_char(ch, "Proc: Slowness or Harm.\r\n");
    return TRUE;
  }

  victim = FIGHTING(ch);
  if (!victim || cmd)
    return FALSE;

  if (AFF_FLAGGED(victim, AFF_SLOW))
    spellnum = SPELL_HARM;

  if (rand_number(0, 23))
    return FALSE;

  weapons_spells(
      "\tLThe jeweled eyes on your dagger glow \tRred \tLas it comes alive, writhes and\tn\r\n"
      "\tLforms into a \tYgolden viper\tL.  As it coils in your hand, its mouth opens wide\tn\r\n"
      "\tLas \tWhuge fangs \tLthrust out.  It strikes out violently and bites into \tn$N\tn\r\n"
      "\tLinjecting $M with venom then recoils and transforms back into a dagger.\tn",

      "\tLThe jeweled eyes on $n's dagger glow \tRred \tLas it comes alive, writhes and\tn\r\n"
      "\tLforms into a \tYgolden viper\tL.  As it coils in $s hand, its mouth opens wide\tn\r\n"
      "\tLas \tWhuge fangs \tLthrust out.  It strikes out violently and bites into you\tn\r\n"
      "\tLinjecting you with venom then recoils and transforms back into a dagger.\tn",

      "\tLThe jeweled eyes on $n\tL's dagger glow \tRred \tLas it comes alive, writhes and\tn\r\n"
      "\tLforms into a \tYgolden viper\tL.  As it coils in $s hand, its mouth opens wide\tn\r\n"
      "\tLas \tWhuge fangs \tLthrust out.  It strikes out violently and bites into \tn$N\tn\r\n"
      "\tLinjecting $M with venom then recoils and transforms back into a dagger.\tn",
      ch, victim, (struct obj_data *)me, spellnum);
  return TRUE;
}

/* from homeland */
SPECIAL(ches)
{
  struct char_data *vict;

  if (!ch)
    return FALSE;

  if (!cmd && !strcmp(argument, "identify"))
  {
    send_to_char(ch, "Invoke haste by keyword 'ches' once per day.  Procs shock "
                     "on critical.\r\n");
    return TRUE;
  }

  vict = FIGHTING(ch);

  // proc on critical
  if (!cmd && FIGHTING(ch) && argument)
  {
    skip_spaces(&argument);
    if (!strcmp(argument, "critical"))
    {
      if (PRF_FLAGGED(ch, PRF_CONDENSED))
      {
      }
      else
      {
        act("\tLAs your \tcstiletto \tLplunges deep into $N,\tn\r\n"
            "\tcs\tCp\tcar\tCk\tcs \tLbegin to \tYcrackle\tL about the hilt.  Suddenly a\tn\r\n"
            "\tBbolt of lightning \tLraces down from above to meet up\tn\r\n"
            "\tLwith the \tChilt\tL of the \tcstiletto\tL.  The enormous voltage\tn\r\n"
            "\tLflows through the weapon into $N\tn\r\n"
            "\tLcausing $S hair to stand on end.\tn",
            FALSE, ch, (struct obj_data *)me, vict, TO_CHAR);
      }

      if (!IS_NPC(vict) && PRF_FLAGGED(vict, PRF_CONDENSED))
      {
      }
      else
      {
        act("\tLAs $n's \tcstiletto \tLplunges deep into your body,\tn\r\n"
            "\tcs\tCp\tcar\tCk\tcs \tLbegin to \tYcrackle\tL about the hilt.  Suddenly a\tn\r\n"
            "\tBbolt of lightning \tLraces down from above to meet up\tn\r\n"
            "\tLwith the \tChilt\tL of the \tcstiletto\tL.  The enormous voltage\tn\r\n"
            "\tLflows through the weapon into you, \tLcausing your hair to stand on end.\tn\r\n",
            FALSE, ch, (struct obj_data *)me, vict, TO_VICT);
      }

      act("\tLAs $n's \tcstiletto \tLplunges deep into $N, \tn\r\n"
          "\tcs\tCp\tcar\tCk\tcs \tLbegin to \tYcrackle\tL about the hilt.  Suddenly a\tn\r\n"
          "\tBbolt of lightning \tLraces down from above to meet up\tn\r\n"
          "\tLwith the \tChilt\tL of the \tcstiletto\tL.  The enormous voltage\tn\r\n"
          "\tLflows through the weapon into $N \tn\r\n"
          "\tLcausing $S hair to stand on end.\tn\tn\r\n",
          ACT_CONDENSE_VALUE, ch, (struct obj_data *)me, vict, TO_NOTVICT);

      damage(ch, vict, 20 + dice(2, 8), -1, DAM_ELECTRIC, FALSE); // type -1 = no dam message

      return TRUE;
    }
  }

  // haste once a day on command
  if (cmd && argument && cmd_info[cmd].command_pointer == do_say)
  {
    if (!is_wearing(ch, 128106))
      return FALSE;
    skip_spaces(&argument);
    if (!strcmp(argument, "ches"))
    {
      if (GET_OBJ_SPECTIMER((struct obj_data *)me, 0) > 0)
      {
        send_to_char(ch, "Nothing happens.\r\n");
        return TRUE;
      }

      act("\tcAs you quietly speak the word of power to your stiletto\tn\r\n"
          "\tcthe aquamarine on the hilt begins to fizzle and pop. The\tn\r\n"
          "\tcnoise continues to culminate until there is a loud crack.\tn\r\n"
          "\tcThe hilt flashes bright for a split second before a sharp\tn\r\n"
          "\tcelectric shock flows up your hand into your body.  You\tn\r\n"
          "\tcsuddenly feel your heart begin to race REALLY fast!\tn",
          FALSE, ch, (struct obj_data *)me, 0, TO_CHAR);

      act("\tC$n whispers to $s $p", FALSE, ch, (struct obj_data *)me, 0, TO_ROOM);

      call_magic(ch, ch, 0, SPELL_HASTE, 0, 30, CAST_WEAPON_SPELL);
      GET_OBJ_SPECTIMER((struct obj_data *)me, 0) = 12;
      return TRUE;
    }
  }
  return FALSE;
}

/* two versionf of the mace:  139250 plus the less powerful 139251 */
#define COURAGE_MACE 139251
#define UPGRADED_COURAGE_MACE 139250
SPECIAL(courage)
{
  if (!ch)
    return FALSE;

  struct obj_data *courage = (struct obj_data *)me;
  int wpn_level = 0;

  if (!cmd && !strcmp(argument, "identify"))
  {
    if (GET_OBJ_VNUM(courage) == COURAGE_MACE)
      send_to_char(ch, "Invoke by whiper 'courage' once every 3 days: mass enhance\r\n");
    else if (GET_OBJ_VNUM(courage) == UPGRADED_COURAGE_MACE)
      send_to_char(ch, "Invoke by whiper 'courage' once every 3 days: prayer and mass enhance\r\n");
    else
      return FALSE;

    return TRUE;
  }

  if (!is_wearing(ch, COURAGE_MACE) && !is_wearing(ch, UPGRADED_COURAGE_MACE))
    return FALSE;

  if (is_wearing(ch, COURAGE_MACE))
    wpn_level = 25;
  if (is_wearing(ch, UPGRADED_COURAGE_MACE))
    wpn_level = 30;

  skip_spaces(&argument);

  if (!strcmp(argument, "courage") && CMD_IS("whisper"))
  {
    if (GET_OBJ_SPECTIMER(courage, 0) > 0)
    {
      send_to_char(ch, "Nothing happens.\r\n");
      return TRUE;
    }

    if (!GROUP(ch))
    {
      send_to_char(ch, "You have to be in a group for this power.\r\n");
      return TRUE;
    }

    /* should be good! */

    act("$n \tLinvokes $s $p!", ACT_CONDENSE_VALUE, ch, courage, 0, TO_ROOM);
    act("\tLYou invoke your $p!", FALSE, ch, courage, 0, TO_CHAR);

    call_magic(ch, ch, NULL, SPELL_MASS_ENHANCE, 0, wpn_level, CAST_WEAPON_SPELL);

    if (is_wearing(ch, UPGRADED_COURAGE_MACE))
      call_magic(ch, ch, NULL, SPELL_PRAYER, 0, wpn_level, CAST_WEAPON_SPELL);

    GET_OBJ_SPECTIMER(courage, 0) = 72;
    return TRUE;
  }

  return FALSE;
}
#undef COURAGE_MACE
#undef UPGRADED_COURAGE_MACE

/* from homeland */
SPECIAL(flamingwhip)
{
  if (!ch)
    return FALSE;

  if (!cmd && !strcmp(argument, "identify"))
  {
    send_to_char(ch, "Proc: Fire Damage.\r\n");
    return TRUE;
  }

  struct char_data *vict = FIGHTING(ch);

  if (cmd || !vict || rand_number(0, 16))
    return FALSE;

  weapons_spells("\trYour $p \trlashes out with infernal \tRfire\tr, burning $N\tr badly!\tn",
                 "\tr$n\tr's $p \trlashes out with infernal \tRfire\tr, burning YOU\tr badly!\tn",
                 "\tr$n\tr's $p \trlashes out with infernal \tRfire\tr, burning $N\tr badly!\tn",
                 ch, vict, (struct obj_data *)me, 0);

  damage(ch, vict, dice(6, 4), -1, DAM_FIRE, FALSE); // type -1 = no dam message

  return TRUE;
}

/* Helmblade vnum 121207
  only procs against evil,  a minor heal on wielder and a dispel_evil.
 */
SPECIAL(helmblade)
{
  if (!ch)
    return FALSE;

  if (!cmd && !strcmp(argument, "identify"))
  {
    send_to_char(ch, "Proc vs Evil: Cure Serious or Dispel Evil.\r\n");
    return TRUE;
  }

  struct char_data *vict = FIGHTING(ch);

  if (cmd || !vict)
    return FALSE;

  if (!IS_EVIL(vict))
    return FALSE;

  switch (rand_number(0, 40))
  {
  case 0:
    weapons_spells("\tWThe \tYHOLY\tW power of \tYHelm\tW flows through your body, cleaning you of "
                   "\tLevil\tW and nourishing you.\tn",
                   "\tWThe \tYHOLY\tW power of \tYHelm\tW flows through $n's\tW body, cleaning $m "
                   "of \tLevil\tW and nourishing $m.\tn",
                   "\tWThe \tWHOLY\tW power of \tYHelm\tW flows through $n's\tW body, cleaning $m "
                   "of \tLevil\tW and nourishing $m.\tn",
                   ch, vict, (struct obj_data *)me, 0);
    call_magic(ch, ch, 0, SPELL_CURE_SERIOUS, 0, GET_LEVEL(ch), CAST_WEAPON_SPELL);
    return TRUE;
  case 1:
    weapons_spells("\tWThe power of \tYHelm\tW guides and strengthens thy hand, dispelling the "
                   "\tLevil\tW of the world from $N.\tn",
                   "\tWThe power of \tYHelm\tW guides and strengthen the hand of $n, dispelling "
                   "the \tLevil\tW of the world from YOU!.\tn",
                   "\tWThe power of \tYHelm\tW guides and strengthen the hand of $n, dispelling "
                   "the \tLevil\tW of the world from $N.\tn",
                   ch, vict, (struct obj_data *)me, SPELL_DISPEL_EVIL);
    return TRUE;
  default:
    return FALSE;
  }
}

/* obj - 113898 has special proc when combined with 113897 */
SPECIAL(flaming_scimitar)
{
  struct obj_data *weepan = (struct obj_data *)me;

  if (!ch)
    return FALSE;

  if (!cmd && !strcmp(argument, "identify"))
  {
    send_to_char(ch, "???\r\n");
    return TRUE;
  }

  struct char_data *vict = FIGHTING(ch);

  if (cmd || !vict || GET_POS(ch) == POS_DEAD)
    return FALSE;

  if (CLASS_LEVEL(ch, CLASS_RANGER) < 10)
  {
    act("\tWA \trsearing hot\tW pain travels up your arm as $p \tWrips itself from your unworthy "
        "grasp!\tn",
        FALSE, ch, (struct obj_data *)me, NULL, TO_CHAR);
    act("\tW$n \tWscreams in pain as $p\tW rips itself from $s grasp!\tn", FALSE, ch,
        (struct obj_data *)me, NULL, TO_ROOM);
    obj_to_room(unequip_char(ch, weepan->worn_on), ch->in_room);
    GET_HIT(ch) = 0;
    return TRUE;
  }

  if (!rand_number(0, 22))
  {
    if (!is_wearing(ch, 113897))
    {
      weapons_spells("\trYour longsword begins to \tLvibrate violently\tr in your hands as it "
                     "forces your arm skyward and flashes with intense magical energy.\tn",

                     "\tr$n's \trlongsword begins to \tLvibrate violently\tr in $s hands as it "
                     "forces $s arm skyward and flashes with intense magical energy.\tn",

                     "\tr$n's \trlongsword begins to \tLvibrate violently\tr in $s hands as it "
                     "forces $s arm skyward and flashes with intense magical energy.\tn",
                     ch, vict, (struct obj_data *)me, SPELL_FLAME_STRIKE);
      return TRUE;
    }
    else
    {
      weapons_spells("\tLIn a \trflurry \tLof movement, your swords cross over one another, "
                     "resulting in a \tCbrilliant \tWflash \tLof \tbmagical energy \tLas their "
                     "powers combine, bringing down a violent storm of \trFIRE\tL upon all.\tn",

                     "\tLIn a \trflurry \tLof movement, \tn$n's\tL swords cross over one another, "
                     "resulting in a \tCbrilliant \tWflash \tLof \tbmagical energy \tLas their "
                     "powers combine, bringing down a violent storm of \trFIRE\tL upon all.\tn",

                     "\tLIn a \trflurry \tLof movement, \tn$n's\tL swords cross over one another, "
                     "resulting in a \tCbrilliant \tWflash \tLof \tbmagical energy \tLas their "
                     "powers combine, bringing down a violent storm of \trFIRE\tL upon all.\tn",
                     ch, vict, (struct obj_data *)me, SPELL_FIRE_STORM);
      return TRUE;
    }
  }
  return FALSE;
}

/* obj - 113897 has special proc when combined with 113898 */
SPECIAL(frosty_scimitar)
{
  struct obj_data *weepan = (struct obj_data *)me;

  if (!ch)
    return FALSE;

  if (!cmd && !strcmp(argument, "identify"))
  {
    send_to_char(ch, "???\r\n");
    return TRUE;
  }

  struct char_data *vict = FIGHTING(ch);

  if (cmd || !vict || GET_POS(ch) == POS_DEAD)
    return FALSE;

  if (CLASS_LEVEL(ch, CLASS_RANGER) < 10)
  {
    act("\tWA \trsearing hot\tW pain travels up your arm as $p \tWrips itself from your unworthy "
        "grasp!\tn",
        FALSE, ch, (struct obj_data *)me, NULL, TO_CHAR);
    act("\tW$n \tWscreams in pain as $p\tW rips itself from $s grasp!\tn", FALSE, ch,
        (struct obj_data *)me, NULL, TO_ROOM);
    obj_to_room(unequip_char(ch, weepan->worn_on), ch->in_room);
    GET_HIT(ch) = 0;
    return TRUE;
  }

  if (!rand_number(0, 18))
  {
    if (!is_wearing(ch, 113898))
    {
      weapons_spells("\tcYour scimitar begins to \tWvibrate violently\tc in your hands as it "
                     "forces your arm skyward and flashes with intense magical energy.\tn",
                     "\tc$n's \tcscimitar begins to \tWvibrate violently\tc in $s hands as it "
                     "forces $s arm skyward and flashes with intense magical energy.\tn",
                     "\tc$n's \tcscimitar begins to \tWvibrate violently\tc in $s hands as it "
                     "forces $s arm skyward and flashes with intense magical energy.\tn",
                     ch, vict, (struct obj_data *)me, SPELL_CONE_OF_COLD);
      return TRUE;
    }
    else
    {
      weapons_spells("\tLIn a \trflurry \tLof movement, your swords cross over one another, "
                     "resulting in a \tCbrilliant \tWflash \tLof \tbmagical energy \tLas their "
                     "powers combine, bringing down a violent storm of \tCICE\tL upon all.\tn",

                     "\tLIn a \trflurry \tLof movement, \tn$n\tL's swords cross over one another, "
                     "resulting in a \tCbrilliant \tWflash \tLof \tbmagical energy \tLas their "
                     "powers combine, bringing down a violent storm of \tCICE\tL upon all.\tn",

                     "\tLIn a \trflurry \tLof movement, \tn$n\tL's swords cross over one another, "
                     "resulting in a \tCbrilliant \tWflash \tLof \tbmagical energy \tLas their "
                     "powers combine, bringing down a violent storm of \tCICE\tL upon all.\tn",
                     ch, vict, (struct obj_data *)me, SPELL_ICE_STORM);
      return TRUE;
    }
  }
  return FALSE;
}

/* from homeland */
SPECIAL(disruption_mace)
{
  if (!ch)
    return FALSE;

  if (!cmd && !strcmp(argument, "identify"))
  {
    send_to_char(ch, "Proc: Flame Strike.\r\n");
    return TRUE;
  }

  struct char_data *vict = FIGHTING(ch);

  if (cmd || !vict || rand_number(0, 20))
    return FALSE;

  weapons_spells("\trYour\tn $p \tris engulfed in flames!\tn",
                 "$n's $p \tris engulfed in flames!\tn", "$n's $p \tris engulfed in flames!\tn", ch,
                 vict, (struct obj_data *)me, SPELL_FLAME_STRIKE);
  return TRUE;
}

/*
// Does not seem to be used yet, was attached to non existant objects
SPECIAL(forest_idol)
{
  if( cmd )
    return FALSE;
  if( !ch )
    return FALSE;

  struct obj_data *obj = (struct obj_data *)me;

    if (!cmd && !strcmp(argument, "identify"))
  {
    send_to_char(ch, "???.\r\n");
    return TRUE;
  }

  if( obj->carried_by != ch )
    return FALSE;

  if( GET_CLASS(ch) == CLASS_ANTIPALADIN )
    return FALSE;

  send_to_char("\tgYour idol melts in your hands.\tn\r\n", ch );
  obj_from_char(obj);
  obj_to_room(obj, ch->in_room );
}
 */

/* from homeland */
SPECIAL(haste_bracers)
{
  if (!ch)
    return FALSE;

  if (!cmd && !strcmp(argument, "identify"))
  {
    send_to_char(ch, "Proc: Haste once per week on keyword 'quicksilver.'\r\n");
    return TRUE;
  }

  if ((cmd && argument && CMD_IS("say")) || (cmd && argument && CMD_IS("'")))
  {
    if (!is_wearing(ch, 138415))
      return FALSE;

    skip_spaces(&argument);
    if (!strcmp(argument, "quicksilver"))
    {
      if (GET_OBJ_SPECTIMER((struct obj_data *)me, 0) > 0)
      {
        send_to_char(ch, "\tWYour \tcbracers\tW have not regained their \tcessence\tW.\tn\r\n");
        return TRUE;
      }
      act("\tWYou whisper softly to your bracers.\tn\r\n"
          "\tWYour $p \tcglow with a blue aura.\tn\r\n"
          "\tWThe world \tcslows \tWto a \tCc\tcr\tCa\tcw\tCl\tW.\tn\r\n",
          FALSE, ch, (struct obj_data *)me, 0, TO_CHAR);
      act("\tW$n whispers softly to $s bracers.\tn\r\n"
          "\tW$n's $p \tcglow with a blue aura.\tn\r\n"
          "\tW$n moves with \tCl\tci\tCg\tch\tCt\tW speed.\tn\r\n",
          FALSE, ch, (struct obj_data *)me, 0, TO_ROOM);
      call_magic(ch, ch, 0, SPELL_HASTE, 0, 30, CAST_WEAPON_SPELL);
      GET_OBJ_SPECTIMER((struct obj_data *)me, 0) = 84;
      return TRUE;
    }
  }
  return FALSE;
}
