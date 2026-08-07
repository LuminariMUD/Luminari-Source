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
#include "spec/spec_combat.h"
#include "spec/spec_context.h"
#include "spec/spec_cooldown.h"
#include "spec/spec_phrase.h"

/*****************************************/
/****  object procs general functions ****/
/*****************************************/

/* this function will check the basic parameters for whether an item is ready to proc in combat */
bool obj_proc_ready(struct char_data *ch, struct obj_data *obj, int cmd)
{
  /* Require the invoking instance, not merely another copy of its VNUM. */
  if (spec_context_validate_worn_object(ch, obj) != SPEC_CONTEXT_VALID)
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
  struct char_data *vict;

  if (!ch)
    return FALSE;

  if (!cmd && !strcmp(argument, "identify"))
  {
    send_to_char(ch, "Proc: Shock damage.\r\n");
    return TRUE;
  }

  vict = FIGHTING(ch);

  if (cmd || !vict || rand_number(0, 15))
    return FALSE;
  if (spec_context_validate_worn_object(ch, (struct obj_data *)me) != SPEC_CONTEXT_VALID ||
      spec_context_validate_combat_target(ch, vict, true) != SPEC_CONTEXT_VALID)
    return FALSE;

  weapons_spells("\twYour $p\tw \tWsparks\tw as you hit $N causing $M to shudder violently from "
                 "the \tYshock\tw!\tn",
                 "$n\tw's $p\tw \tWsparks\tw as $e hits you causing you to shudder violently from "
                 "the \tYshock\tw!\tn",
                 "$n\tw's $p\tw \tWsparks\tw as $e hits $N causing $M to shudder violently from "
                 "the \tYshock\tw!\tn",
                 ch, vict, (struct obj_data *)me, 0);
  (void)spec_damage_current_target(ch, vict, dice(2, 8), -1, DAM_ELECTRIC, FALSE);

  return TRUE;
}

SPECIAL(monk_glove_cold)
{
  struct char_data *vict;

  if (!ch)
    return FALSE;

  if (!cmd && !strcmp(argument, "identify"))
  {
    send_to_char(ch, "Proc: Cold damage.\r\n");
    return TRUE;
  }

  vict = FIGHTING(ch);

  if (cmd || !vict || rand_number(0, 15))
    return FALSE;
  if (spec_context_validate_worn_object(ch, (struct obj_data *)me) != SPEC_CONTEXT_VALID ||
      spec_context_validate_combat_target(ch, vict, true) != SPEC_CONTEXT_VALID)
    return FALSE;

  weapons_spells("\twYour $p\tw \tWfrosts\tw as you hit $N causing $M to shudder violently from "
                 "the \tBcold\tw!\tn",
                 "$n\tw's $p\tw \tWfrosts\tw as $e hits you causing you to shudder violently from "
                 "the \tBcold\tw!\tn",
                 "$n\tw's $p\tw \tWfrosts\tw as $e hits $N causing $M to shudder violently from "
                 "the \tBcold\tw!\tn",
                 ch, vict, (struct obj_data *)me, 0);
  (void)spec_damage_current_target(ch, vict, dice(2, 8), -1, DAM_COLD, FALSE);

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

/* Toggle for legacy object-procedure debug messages.
 * TRUE is intentionally noisy; FALSE is normal gameplay. */
#define DEBUGMODE FALSE

SPECIAL(warbow)
{
  if (!cmd && argument && !strcmp(argument, "identify"))
  {
    send_to_char(ch, "This is a special warbow.\r\n");
    return TRUE;
  }
  return 0;
}

/* from homeland */
SPECIAL(xvim_normal)
{
  int dam, i, num = dice(1, 4);

  if (!ch)
    return FALSE;

  if (!cmd && !strcmp(argument, "identify"))
  {
    send_to_char(ch, "???");
    return TRUE;
  }

  struct char_data *tch = NULL, *vict = FIGHTING(ch);

  if (!cmd && vict)
    switch (rand_number(0, 40))
    {
    case 0:
    case 1:
      weapons_spells(
          "\tLThe \tGUNHOLY\tL power of \tgIy\tGach\tgtu X\tGvi\tgm \tLtakes hold of your body\r\n"
          "in a flash of \tmhatred\tL. Your arms begin to move with blinding\r\n"
          "speed as your accursed weapon begins to rend and tear apart\r\n"
          "the \tyflesh\tL of $N\tL in a spray of \trBLOOD\tL.\tn",
          "\tLThe \tGUNHOLY\tL power of \tgIy\tGach\tgtu X\tGvi\tgm \tLtakes hold of \tg$n's\tL "
          "body\r\n"
          "in a flash of \tmhatred\tL. $s arms begin to move with blinding\r\n"
          "speed as $s accursed weapon begins to rend and tear apart\r\n"
          "YOUR \tyflesh\tL in a spray of \trBLOOD\tL.\tn",
          "\tLThe \tGUNHOLY\tL power of \tgIy\tGach\tgtu X\tGvi\tgm \tLtakes hold of \tg$n's\tL "
          "body\r\n"
          "in a flash of \tmhatred\tL. $s arms begin to move with blinding\r\n"
          "speed as $s accursed weapon begins to rend and tear apart\r\n"
          "the \tyflesh\tL of $N\tL in a spray of \trBLOOD\tL.\tn",
          ch, vict, (struct obj_data *)me, 0);
      for (i = 0; i < num; i++)
        if (vict)
          hit(ch, vict, TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, FALSE);
      return TRUE;
    case 2:
      if (!rand_number(0, 100))
      {
        weapons_spells("\tf\tWBOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOM!!!\tn\r\n"
                       "\r\n\r\n\tmAfter a blinding flash the entire area becomes enveloped\r\n"
                       "in \tLdarkness\tm. With your limited vision you see an extremely\r\n"
                       "tall man plunges a large scimitar into $N's\tm chest and cackles\r\n"
                       "as $S \tMlife force\tm is stolen away. As the \tLdarkness\tm fades the\r\n"
                       "dark figure fades into nothingness, laughing all the while.\tn\r\n",
                       "\tf\tWBOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOM!!!\tn\r\n"
                       "\r\n\r\n\tmAfter a blinding flash the entire area becomes enveloped\r\n"
                       "in \tLdarkness\tm. With your limited vision you see an extremely\r\n"
                       "tall man plunges a large scimitar into $N's\tm chest and cackles\r\n"
                       "as $S \tMlife force\tm is stolen away. As the \tLdarkness\tm fades the\r\n"
                       "dark figure fades into nothingness, laughing all the while.\tn\r\n",
                       "\tf\tWBOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOM!!!\tn\r\n"
                       "\r\n\r\n\tmAfter a blinding flash the entire area becomes enveloped\r\n"
                       "in \tLdarkness\tm. With your limited vision you see an extremely\r\n"
                       "tall man plunges a large scimitar into $N's\tm chest and cackles\r\n"
                       "as $S \tMlife force\tm is stolen away. As the \tLdarkness\tm fades the\r\n"
                       "dark figure fades into nothingness, laughing all the while.\tn\r\n",
                       ch, vict, (struct obj_data *)me, 0);
        dam = rand_number(100, 200);
        if (dam > GET_HIT(vict))
        {
          GET_HIT(vict) = -13;
          change_position(vict, POS_DEAD);
          update_pos(vict);
        }
        else
        {
          GET_HIT(vict) -= dam;
          change_position(vict, POS_SITTING);
        }
        for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room)
          USE_MOVE_ACTION(tch);
        return TRUE;
      }
      return FALSE;
    case 3:
    case 4:
    case 5:
    case 6:
      if (GET_HIT(ch) < GET_MAX_HIT(ch))
      {
        act("\tLYour avenger \tgglows\tL in your hands, and your \trblood\tL seems to flow "
            "back\tn\r\n"
            "\tLinto your wounds, \tWhealing\tL them by the unholy power of \tGXvim\tL.\tn",
            FALSE, ch, 0, 0, TO_CHAR);
        act("\tw$n\tL's avenger \tgglows\tL in $s hands, and $s \trblood\tL seems to flow "
            "back\tn\r\n"
            "\tLinto $s wounds, \tWhealing\tL them by the unholy power of \tGXvim\tL.\tn",
            FALSE, ch, 0, 0, TO_ROOM);
        GET_HIT(ch) = MIN(GET_MAX_HIT(ch), GET_HIT(ch) + dice(10, 10));
        return TRUE;
      }
      return FALSE;
      break;
    default:
      return FALSE;
    }
  return FALSE;
}

/* from homeland */
SPECIAL(xvim_artifact)
{
  int num = (dice(1, 4) + 2), dam, i = 0;

  if (!ch)
    return FALSE;

  if (!cmd && !strcmp(argument, "identify"))
  {
    send_to_char(ch, "???");
    return TRUE;
  }

  struct char_data *tch = NULL, *vict = FIGHTING(ch), *pet = NULL;

  if (!cmd && vict)
  {
    switch (rand_number(0, 35))
    {
    case 0:
    case 1:
      weapons_spells(
          "\tLThe \tGUNHOLY\tL power of \tgIy\tGach\tgtu X\tGvi\tgm \tLtakes hold of your body\r\n"
          "in a flash of \tmhatred\tL. Your arms begin to move with blinding\r\n"
          "speed as your accursed weapon begins to rend and tear apart\r\n"
          "the \tyflesh\tL of $N\tL in a spray of \trBLOOD\tL.\tn",
          "\tLThe \tGUNHOLY\tL power of \tgIy\tGach\tgtu X\tGvi\tgm \tLtakes hold of \tg$n's\tL "
          "body\r\n"
          "in a flash of \tmhatred\tL. $s arms begin to move with blinding\r\n"
          "speed as $s accursed weapon begins to rend and tear apart\r\n"
          "YOUR \tyflesh\tL in a spray of \trBLOOD\tL.\tn",
          "\tLThe \tGUNHOLY\tL power of \tgIy\tGach\tgtu X\tGvi\tgm \tLtakes hold of \tg$n's\tL "
          "body\r\n"
          "in a flash of \tmhatred\tL. $s arms begin to move with blinding\r\n"
          "speed as $s accursed weapon begins to rend and tear apart\r\n"
          "the \tyflesh\tL of $N\tL in a spray of \trBLOOD\tL.\tn",
          ch, vict, (struct obj_data *)me, 0);
      for (i = 0; i < num; i++)
        if (vict)
          hit(ch, vict, TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, FALSE);
      return TRUE;
    case 2:
      if (!rand_number(0, 100))
      {
        weapons_spells("\tf\tWBOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOM!!!\tn\r\n"
                       "\r\n\r\n\tmAfter a blinding flash the entire area becomes enveloped\r\n"
                       "in \tLdarkness\tm. With your limited vision you see an extremely\r\n"
                       "tall man plunges a large scimitar into $N's\tm chest and cackles\r\n"
                       "as $S \tMlife force\tm is stolen away. As the \tLdarkness\tm fades the\r\n"
                       "dark figure fades into nothingness, laughing all the while.\tn\r\n",
                       "\tf\tWBOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOM!!!\tn\r\n"
                       "\r\n\r\n\tmAfter a blinding flash the entire area becomes enveloped\r\n"
                       "in \tLdarkness\tm. With your limited vision you see an extremely\r\n"
                       "tall man plunges a large scimitar into $N's\tm chest and cackles\r\n"
                       "as $S \tMlife force\tm is stolen away. As the \tLdarkness\tm fades the\r\n"
                       "dark figure fades into nothingness, laughing all the while.\tn\r\n",
                       "\tf\tWBOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOM!!!\tn\r\n"
                       "\r\n\r\n\tmAfter a blinding flash the entire area becomes enveloped\r\n"
                       "in \tLdarkness\tm. With your limited vision you see an extremely\r\n"
                       "tall man plunges a large scimitar into $N's\tm chest and cackles\r\n"
                       "as $S \tMlife force\tm is stolen away. As the \tLdarkness\tm fades the\r\n"
                       "dark figure fades into nothingness, laughing all the while.\tn\r\n",
                       ch, vict, (struct obj_data *)me, 0);
        dam = rand_number(100, 200) + 50;
        if (dam > GET_HIT(vict))
        {
          GET_HIT(vict) = -13;
          change_position(vict, POS_DEAD);
        }
        else
        {
          GET_HIT(vict) -= dam;
          change_position(vict, POS_SITTING);
        }
        for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room)
          USE_MOVE_ACTION(tch);
        return TRUE;
      }
      return FALSE;
    case 3:
    case 4:
    case 5:
      if (GET_HIT(ch) < GET_MAX_HIT(ch))
      {
        act("\tLYour avenger \tgglows\tL in your hands, and your \trblood\tL seems to flow "
            "back\tn\r\n"
            "\tLinto your wounds, \tWhealing\tL them by the unholy power of \tGXvim\tL.\tn",
            FALSE, ch, 0, 0, TO_CHAR);
        act("\tw$n\tL's avenger \tgglows\tL in $s hands, and $s \trblood\tL seems to flow "
            "back\tn\r\n"
            "\tLinto $s wounds, \tWhealing\tL them by the unholy power of \tGXvim\tL.\tn",
            FALSE, ch, 0, 0, TO_ROOM);
        GET_HIT(ch) = MIN(GET_MAX_HIT(ch), GET_HIT(ch) + dice(11, 10));
        return TRUE;
      }
      return FALSE;
      break;
    default:
      return FALSE;
    }
  }

  skip_spaces(&argument);

  if (!is_wearing(ch, 100501))
    return FALSE;

  if (!strcmp(argument, "nightmare") && CMD_IS("whisper"))
  {
    if (mob_index[real_mobile(100505)].number < 1)
    {
      act("\tLAs you whisper '\tmnightmare\tL' to your \trsword\tL, a \twthick\tW fog"
          "\tLforms in the area\r\naround you.  When it finally fades, the "
          "horrid visage of a \tmNightmare\tL\r\nstands before you.\tn",
          1, ch, 0, FIGHTING(ch), TO_CHAR);
      act("\tLAs $n whispers something to $s \trsword\tL, a \twthick\tW fog\r\n"
          "\tLforms in the area around you.  When it finally fades, the "
          "horrid visage\r\nof a \tmNightmare\tL stands before you.\tn",
          1, ch, 0, FIGHTING(ch), TO_ROOM);
      pet = read_mobile(real_mobile(100505), REAL);

      if (ZONE_FLAGGED(GET_ROOM_ZONE(ch->in_room), ZONE_WILDERNESS))
      {
        X_LOC(pet) = world[ch->in_room].coords[0];
        Y_LOC(pet) = world[ch->in_room].coords[1];
      }

      char_to_room(pet, ch->in_room);
      add_follower(pet, ch);
      GET_MAX_HIT(pet) = GET_HIT(ch) = GET_LEVEL(ch) * 10 + dice(GET_LEVEL(ch), 6);
      SET_BIT_AR(AFF_FLAGS(pet), AFF_CHARM);
      return TRUE;
    }
    else
    {
      send_to_char(ch, "\tLAs you whisper '\tmnightmare\tL' to your \trsword\tL, nothing seems to "
                       "happen.\tn\r\n");
      return TRUE;
    }
  }
  return FALSE;
}

/* from homeland */
SPECIAL(dragonbone_hammer)
{
  if (!ch)
    return FALSE;

  if (!cmd && !strcmp(argument, "identify"))
  {
    send_to_char(ch, "Proc: Ice Dagger.\r\n");
    return TRUE;
  }

  struct char_data *vict = FIGHTING(ch);

  if (cmd || !vict || rand_number(0, 10))
    return FALSE;

  weapons_spells("Your $p \tCvibrates violently!\tn", "$n's $p \tCvibrates violently!\tn",
                 "$n's $p \tCvibrates violently!\tn", ch, vict, (struct obj_data *)me,
                 SPELL_ICE_DAGGER);
  return TRUE;
}

/* from homeland */
SPECIAL(prismorb)
{
  if (!ch)
    return FALSE;

  if (!cmd && !strcmp(argument, "identify"))
  {
    send_to_char(ch, "Proc: Prismatic Spray.\r\n");
    return TRUE;
  }

  struct char_data *vict = FIGHTING(ch);

  if (cmd || !vict || rand_number(0, 25))
    return FALSE;

  weapons_spells("\tWYour \tn$p \tWpulsates violently.\tn",
                 "\tW$n\tW's \tn$p \tWpulsates violently.\tn",
                 "\tW$n\tW's \tn$p \tWpulsates violently.\tn", ch, vict, (struct obj_data *)me,
                 SPELL_PRISMATIC_SPRAY);

  return TRUE;
}

/* from homeland */
SPECIAL(dorfaxe)
{
  int num = 18;
  int dam = 0;

  if (!ch)
    return FALSE;

  if (!cmd && !strcmp(argument, "identify"))
  {
    send_to_char(ch, "Proc vs Evil: Clangeddins Wrath.\r\n");
    return TRUE;
  }

  struct char_data *vict = FIGHTING(ch);

  if (cmd || !vict)
    return FALSE;

  if (GET_RACE(ch) == RACE_DWARF)
    num = 12;

  if (!IS_GOOD(ch))
    return FALSE;
  if (!IS_EVIL(vict))
    return FALSE;
  if (rand_number(0, num))
    return FALSE;

  dam = rand_number(6, 12);

  if (dam > GET_HIT(vict))
    dam = GET_HIT(vict);

  weapons_spells("\tWAs $p impacts with \tn$N\tW, a mortal enemy of\r\n"
                 "\tWany righteous dwarf, the great god \tYClangeddin\tW infuses it,\r\n"
                 "\tWand strikes with great power into \tn$M.\tn",

                 "\tWAs $p impacts with YOU, a mortal enemy of\r\n"
                 "\tWany righteous dwarf, the great god \tYClangeddin\tW infuses it,\r\n"
                 "\tWand strikes with great power into YOU!\tn",

                 "\tWAs $p impacts with \tn$N\tW, a mortal enemy of\r\n"
                 "\tWany righteous dwarf, the great god \tYClangeddin\tW infuses it,\r\n"
                 "\tWand strikes with great power into \tn$M.\tn",
                 ch, vict, (struct obj_data *)me, 0);

  damage(ch, vict, dam, -1, DAM_HOLY, FALSE); // type -1 = no dam message
  return TRUE;
}

/* from homeland */
SPECIAL(acidstaff)
{
  struct char_data *victim = NULL;

  if (!cmd && !strcmp(argument, "identify"))
  {
    send_to_char(ch, "Proc: Acid Arrow.\r\n");
    return TRUE;
  }

  if (!ch)
    return FALSE;

  victim = FIGHTING(ch);

  if (!victim || cmd)
    return FALSE;

  if (rand_number(0, 15))
    return FALSE;

  weapons_spells("\tLYour staff vibrates and hums then glows \tGbright green\tL.\tn\r\n"
                 "\tLThe tiny black dragons on your staff come alive and roar loudly\tn\r\n"
                 "\tLthen spew forth vile \tgacid\tL at $N.\tn",

                 "\tL$n\tL's staff vibrates and hums then glows \tGbright green\tL.\tn\r\n"
                 "\tLThe tiny black dragons on $s staff come alive and roar loudly\tn\r\n"
                 "\tLthen spew forth vile \tgacid\tL at you.\tn",

                 "\tL$n\tL's staff vibrates and hums then glows \tGbright green\tL.\tn\r\n"
                 "\tLThe tiny black dragons on $s staff come alive and roar loudly\tn\r\n"
                 "\tLthen spew forth vile \tgacid\tL at \tn$N.\tn",
                 ch, victim, (struct obj_data *)me, SPELL_ACID_ARROW);
  return TRUE;
}

/* from homeland */
SPECIAL(sarn)
{
  if (!ch)
    return FALSE;

  int num = 18;

  if (!cmd && !strcmp(argument, "identify"))
  {
    send_to_char(ch, "Proc: Harm, more effective for Duergar.\r\n");
    return TRUE;
  }

  struct char_data *vict = FIGHTING(ch);

  if (cmd || !vict)
    return FALSE;

  if (GET_RACE(ch) == RACE_DUERGAR)
    num = 12;

  if (!IS_EVIL(ch))
    return FALSE;
  if (rand_number(0, num))
    return FALSE;

  weapons_spells(
      "\tLThe power of \twLad\tWu\twgu\tWe\twr\tL guides thine hand and "
      "\trstr\tRe\trngth\tRe\trns\tL it.\tn\r\n"
      "\tLAs the \traxe\tL impacts with \tn$N\tL, \twd\tWi\twv\tWi\twne\tL power is unleashed.\tn",

      "\tLThe power of \twLad\tWu\twgu\tWe\twr\tL guides $n's hand and "
      "\trstr\tRe\trngth\tRe\trns\tL it.\tn\r\n"
      "\tLAs the \traxe\tL impacts with YOU, \twd\tWi\twv\tWi\twne\tL power is unleashed.\tn",

      "\tLThe power of \twLad\tWu\twgu\tWe\twr\tL guides \tn$n\tL's hand and "
      "\trstr\tRe\trngth\tRe\trns\tL it.\tn\r\n"
      "\tLAs the \traxe\tL impacts with \tn$N\tL, \twd\tWi\twv\tWi\twne\tL power is unleashed.\tn",
      ch, vict, (struct obj_data *)me, SPELL_HARM);

  return TRUE;
}

/* from homeland */
SPECIAL(purity)
{
  if (!ch)
    return FALSE;

  int dam = 0;

  if (!cmd && !strcmp(argument, "identify"))
  {
    send_to_char(ch, "Proc:  Holy Light - in combat randomly inflict 2d24 holy damage.\r\n");
    return TRUE;
  }

  struct char_data *vict = FIGHTING(ch);

  if (cmd || !vict || rand_number(0, 20))
    return FALSE;

  dam = dice(2, 24);
  if (dam < GET_HIT(vict) + 10)
  {
    if (PRF_FLAGGED(ch, PRF_CONDENSED))
    {
    }
    else
    {
      act("\twThe head of your $p starts to \tYglow \twwith a \tWbright white light\tw.\r\n"
          "A beam of concetrated \tWholiness \twshoots towards $N.\r\n"
          "The \tWlightbeam \twsurrounds $N who howls in pain and fear.\tn",
          FALSE, ch, (struct obj_data *)me, vict, TO_CHAR);
    }

    if (!IS_NPC(vict) && PRF_FLAGGED(vict, PRF_CONDENSED))
    {
    }
    else
    {
      act("$n's $p \twstarts to \tYglow \twwith a \tWbright white light\tw.\r\n"
          "A beam of concentrated \tWholiness \twshoots towards you.\r\n"
          "The \tWlightbeam \twsurrounds you and you howl in pain and fear.\tn",
          FALSE, ch, (struct obj_data *)me, vict, TO_VICT);
    }

    act("$n's $p \twstarts to \tYglow \twwith a \tWbright white light\tw.\r\n"
        "A beam of concentrated \tWholiness \twshoots towards $N.\r\n"
        "The \tWlightbeam \twsurrounds $N who howls in pain and fear.\tn",
        ACT_CONDENSE_VALUE, ch, (struct obj_data *)me, vict, TO_NOTVICT);
  }
  else
  {
    if (PRF_FLAGGED(ch, PRF_CONDENSED))
    {
    }
    else
    {
      act("\twThe head of your $p starts to \tYglow \twwith a \tWbright white light\t.w\r\n"
          "A beam of concentrated \tWholiness \twshoots towards $N.\r\n"
          "The \tWlightbeam \twburns a hole right through $N who falls lifeless to the ground.\tn",
          FALSE, ch, (struct obj_data *)me, vict, TO_CHAR);
    }

    if (!IS_NPC(vict) && PRF_FLAGGED(vict, PRF_CONDENSED))
    {
    }
    else
    {
      act("$n's $p \twstarts to \tYglow \twwith a \tWbright white light\tw.\r\n"
          "A beam of concentrated \tWholiness \twshoots towards you.\r\n"
          "The \tWlightbeam \twburns a hole right through you and you fall lifeless to the "
          "ground.\tn",
          FALSE, ch, (struct obj_data *)me, vict, TO_VICT);
    }

    act("$n's $p \twstarts to \tYglow \twwith a \tWbright white light\tw.\r\n"
        "A beam of concentrated \tWholiness \twshoots towards $N.\r\n"
        "The \tWlightbeam \twburns a hole right through $N who falls lifeless to the ground.\tn",
        ACT_CONDENSE_VALUE, ch, (struct obj_data *)me, vict, TO_NOTVICT);

    call_magic(ch, vict, 0, SPELL_BLINDNESS, 0, GET_LEVEL(ch), CAST_WEAPON_SPELL);
  }

  damage(ch, vict, dam, -1, DAM_HOLY, FALSE); // type -1 = no dam message

  return TRUE;
}

/* from homeland */
SPECIAL(etherealness)
{
  if (!ch)
    return FALSE;

  if (!cmd && !strcmp(argument, "identify"))
  {
    send_to_char(ch, "Proc:  Slow.\r\n");
    return TRUE;
  }

  struct char_data *vict = FIGHTING(ch);

  if (cmd || !vict || rand_number(0, 15))
    return FALSE;

  weapons_spells("\twWaves of \tWghostly \twenergy starts to flow from your $p.",
                 "\twWaves of \tWghostly \twenergy starts to flow from $n's $p.",
                 "\twWaves of \tWghostly \twenergy starts to flow from $n's $p.", ch, vict,
                 (struct obj_data *)me, SPELL_SLOW);

  return TRUE;
}

/* from homeland */
SPECIAL(star_circlet)
{
  struct obj_data *circlet = (struct obj_data *)me;

  if (!circlet)
    return FALSE;

  if (!ch)
    return FALSE;

  if (!cmd && !strcmp(argument, "identify"))
  {
    send_to_char(ch, "Proc in combat: Divine Arcane Recall\r\n");
    return TRUE;
  }

  if (DEBUGMODE)
    send_to_char(ch, "DEBUG MARK 1\r\n");

  if (!is_wearing(ch, 132104))
    return FALSE;

  if (DEBUGMODE)
    send_to_char(ch, "DEBUG MARK 2\r\n");

  if (!FIGHTING(ch))
    return FALSE;

  if (DEBUGMODE)
    send_to_char(ch, "DEBUG MARK 3\r\n");

  if (cmd)
    return FALSE;

  if (DEBUGMODE)
    send_to_char(ch, "DEBUG MARK 4\r\n");

  if (rand_number(0, 5))
    return FALSE;

  if (DEBUGMODE)
    send_to_char(ch, "DEBUG MARK 5\r\n");

  if (star_circlet_proc(ch, 0))
  {
    act("\twIn an instant \tYflare of power\tw, the \tBdisplaced stars\tw encircling $p draw in "
        "\tYarcane and divine energy\tw from the planes directly into your head!\tn",
        FALSE, ch, circlet, NULL, TO_CHAR);
    act("\twIn an instant \tYflare of power\tw, the \tBdisplaced stars\tw encircling \tn$n's\tn $p"
        "\tw draw in \tYarcane and divine energy\tw from the planes directly into $s head!\tn",
        FALSE, ch, circlet, NULL, TO_NOTVICT);
  }
  else
  {
    if (DEBUGMODE)
      send_to_char(ch, "DEBUG MARK 6\r\n");
  }

  return TRUE;
}

/* from homeland */
SPECIAL(greatsword)
{
  if (!ch)
    return FALSE;

  if (!cmd && !strcmp(argument, "identify"))
  {
    send_to_char(ch, "Proc:  Silver Flames.\r\n");
    return TRUE;
  }

  struct char_data *vict = FIGHTING(ch);

  if (cmd || !vict || rand_number(0, 20))
    return FALSE;

  int dam = 30 + dice(5, 5);

  if (dam > GET_HIT(vict))
    dam = GET_HIT(vict);

  if (dam < 21)
    return FALSE;

  weapons_spells("\tCSilvery flames shoots from your $p\tC towards $N\tC.\r\nThe flames sear and "
                 "burn $N\tC who screams in pain.\tn",
                 "\tCSilvery flames shoot from $n's $p\tC towards you\tC.\r\nThe flames sear and "
                 "burn you and you scream in pain.\tn",
                 "\tCSilvery flames shoot from $n's $p\tC towards $N\tC.\r\nThe flames sear and "
                 "burn $N\tC who screams in pain.\tn",
                 ch, vict, (struct obj_data *)me, 0);

  damage(ch, vict, dam, -1, DAM_ENERGY, FALSE); // type -1 = no dam message

  return TRUE;
}

/* from homeland */
SPECIAL(fog_dagger)
{
  struct char_data *i, *vict;
  struct affected_type af;
  struct affected_type af2;

  if (!cmd && !strcmp(argument, "identify"))
  {
    send_to_char(ch, "Procs paralysis on backstab, whisper 'haze' for foggy"
                     " cloud.\r\n");
    return TRUE;
  }

  if (!is_wearing(ch, 115003))
    return FALSE;

  if (!ch || !cmd)
    return FALSE;

  skip_spaces(&argument);

  // First check if they whispered haze
  if (!strcmp(argument, "haze") && CMD_IS("whisper") && (vict = FIGHTING(ch)))
  {
    if ((GET_OBJ_SPECTIMER((struct obj_data *)me, 0) > 0))
    {
      act("\tLYou whisper a word to your\tn $p,\tL and nothing happens.\tn", FALSE, ch,
          (struct obj_data *)me, 0, TO_CHAR);
      return TRUE;
    }
    else
    {
      weapons_spells(
          "\tLA hazy cloud is emitted from your\tn $p\tL, and enshrouds \tn$N \tLin a dark "
          "mist!\tn",
          "\tLA hazy cloud is emitted from $n's\tn $p\tL, and enshrouds \tn$N \tLin a dark "
          "mist!\tn",
          "\tLA hazy cloud is emitted from $n's\tn $p\tL, and enshrouds you in a dark mist!\tn", ch,
          vict, (struct obj_data *)me, 0);

      // Sets the vict blind for 1-3 rounds
      if (!AFF_FLAGGED(vict, AFF_BLIND) && can_blind(vict))
      {
        new_affect(&af);
        af.spell = SPELL_BLINDNESS;
        SET_BIT_AR(af.bitvector, AFF_BLIND);
        af.duration = dice(1, 3);
        affect_join(vict, &af, TRUE, FALSE, FALSE, FALSE);
      }
      for (i = world[vict->in_room].people; i; i = i->next_in_room)
      {
        if (FIGHTING(i) == vict)
        {
          stop_fighting(i);
          act("\tLThe haze around \tn$N \tLprevents you from touching \tn$M", FALSE, i, 0, vict,
              TO_CHAR);
        }
      }

      stop_fighting(vict);
      clearMemory(vict);

      GET_OBJ_SPECTIMER((struct obj_data *)me, 0) = 24;
      return TRUE;
    }
    // Now check if they are trying to backstab
  }
  else if (CMD_IS("backstab") && (vict = get_char_vis(ch, argument, NULL, FIND_CHAR_ROOM)))
  {
    if (perform_backstab(ch, vict))
    {
      if (FIGHTING(ch) == vict && !AFF_FLAGGED(vict, AFF_PARALYZED) && !paralysis_immunity(vict) &&
          !rand_number(0, 9))
      {
        new_affect(&af2);
        af2.spell = SPELL_HOLD_PERSON;
        SET_BIT_AR(af2.bitvector, AFF_PARALYZED);
        af2.duration = dice(1, 2);
        affect_join(vict, &af2, TRUE, FALSE, FALSE, FALSE);
      }
    }
    return TRUE;
  }

  return FALSE;
}

/* a NPC only item */
SPECIAL(tyrantseye)
{
  if (!ch)
    return FALSE;

  if (!cmd && !strcmp(argument, "identify"))
  {
    send_to_char(ch, "???\r\n");
    return TRUE;
  }

  struct char_data *vict = FIGHTING(ch), *i = NULL, *in = NULL;

  if (cmd || !vict)
    return FALSE;

  if (!IS_NPC(ch) && !rand_number(0, 1))
  {
    act("\tLA \tWbolt \tLof \tGgreen \tLLighting slams into $n from above!\tn", FALSE, ch, 0, 0,
        TO_ROOM);
    act("\tLA \tWbolt \tLof \tGgreen \tLLighting slams into you from above!\tn", FALSE, ch, 0, 0,
        TO_CHAR);
    die(ch, ch);
  }

  switch (rand_number(0, 35))
  {
  case 0:
  case 1:
    weapons_spells("IF YOU SEE THIS, TALK TO A STAFF MEMBER",
                   "\tgFzoul \tLturns his wicked gaze toward's you and utters arcane "
                   "words to his \tgscepter\tL. You are blinded by a brilliant \tWFLASH\tn "
                   "\tLas a \tpbolt\tL of crackling \tGgreen energy\tL is hurled toward you!\tn",
                   "\tgFzoul \tLturns his wicked gaze toward's $N \tLand utters arcane "
                   "words to his \tgscepter\tL. $N \tLis blinded by a brilliant \tWFLASH\tn "
                   "\tLas a \tpbolt\tL of crackling \tGgreen energy\tL is hurled toward $M!\tn",
                   ch, vict, (struct obj_data *)me, 0);
    call_magic(ch, vict, 0, SPELL_MISSILE_STORM, 0, 30, CAST_WEAPON_SPELL);
    call_magic(ch, vict, 0, SPELL_BLINDNESS, 0, 30, CAST_WEAPON_SPELL);
    call_magic(ch, vict, 0, SPELL_SLOW, 0, 30, CAST_WEAPON_SPELL);
    return TRUE;
  case 10:
    weapons_spells("\tGIF YOU SEE THIS TALK TO A STAFF MEMBER",
                   "\tGFzoul's \tLscepter springs to life in a \tWFLASH\tL, bathing your "
                   "party in a misty \tGgreen glow! \tLYou scream in agony as you "
                   "begin to lose control of your body!\tn",
                   "\tGFzoul's \tLscepter springs to life in a \tWFLASH\tL, bathing the "
                   "room in a misty \tGgreen glow! \tLYou scream in agony as you "
                   "begin to lose control of your body!\tn",
                   ch, vict, (struct obj_data *)me, 0);

    for (i = character_list; i; i = in)
    {
      in = i->next;
      if (!IS_NPC(i) || IS_PET(i))
      {
        call_magic(ch, i, 0, SPELL_CURSE, 0, 30, CAST_WEAPON_SPELL);
        call_magic(ch, i, 0, SPELL_POISON, 0, 30, CAST_WEAPON_SPELL);
      }
    }
    return TRUE;
  default:
    return FALSE;
  }
  return FALSE;
}

/* from homeland */
SPECIAL(spiderdagger)
{
  if (!ch)
    return FALSE;

  if (!cmd && !strcmp(argument, "identify"))
  {
    send_to_char(ch, "Procs darkfire in combat and by Invoking Lloth she protects any drow.\r\n");
    return TRUE;
  }

  struct char_data *vict;

  vict = FIGHTING(ch);

  if (!cmd && vict && !rand_number(0, 9))
  {
    // proc darkfire
    weapons_spells("\tLYour $p\tL starts to \tcglow\tL as it pierces \tn$N!",
                   "$n\tL's $p\tL starts to \tcglow\tL as it pierces YOU!",
                   "$n\tL's $p\tL starts to \tcglow\tL as it pierces \tn$N!", ch, vict,
                   (struct obj_data *)me, SPELL_NEGATIVE_ENERGY_RAY);
    return TRUE;
  }
  // cloak of dark power once day on command
  if (cmd && argument && cmd_info[cmd].command_pointer == do_say)
  {
    if (!is_wearing(ch, 135535))
      return FALSE;

    skip_spaces(&argument);
    if (!strcmp(argument, "lloth"))
    {
      if (GET_OBJ_SPECTIMER((struct obj_data *)me, 0) > 0)
      {
        send_to_char(ch, "Nothing happens.\r\n");
        return TRUE;
      }
      if (GET_RACE(ch) != RACE_DROW)
      {
        send_to_char(ch, "Nothing happens.\r\n");
        return TRUE;
      }
      send_to_char(ch, "\tLYou invoke \tmLloth\tw.\tn\r\n");
      act("\tw$n raises $s $p \tw high and calls on \tmLloth.\tn", FALSE, ch, (struct obj_data *)me,
          0, TO_ROOM);
      call_magic(ch, ch, 0, SPELL_NON_DETECTION, 0, 30, CAST_WEAPON_SPELL);
      call_magic(ch, ch, 0, SPELL_CIRCLE_A_GOOD, 0, 30, CAST_WEAPON_SPELL);

      GET_OBJ_SPECTIMER((struct obj_data *)me, 0) = 24;
      return TRUE;
    }
  }

  return FALSE;
}

/* from homeland */
SPECIAL(sparksword)
{
  if (!ch)
    return FALSE;

  if (!cmd && !strcmp(argument, "identify"))
  {
    send_to_char(ch, "Proc: Shock damage.\r\n");
    return TRUE;
  }

  struct char_data *vict = FIGHTING(ch);

  if (cmd || !vict || rand_number(0, 20))
    return FALSE;

  weapons_spells("\twYour $p\tw's blade \tWsparks\tw as you hit $N "
                 "\twwith your slash, causing $M to shudder violently from the \tYshock\tw!\tn",
                 "$n\tw's $p\tw's blade \tWsparks\tw as $e hits you "
                 "with $s slash, causing you to shudder violently from the \tYshock\tw!\tn",
                 "$n\tw's $p\tw's blade \tWsparks\tw as $e hits $N "
                 "\twwith $s slash, causing $M to shudder violently from the \tYshock\tw!\tn",
                 ch, vict, (struct obj_data *)me, 0);
  damage(ch, vict, dice(9, 3), -1, DAM_ELECTRIC, FALSE);

  return TRUE;
}

/* from homeland */
SPECIAL(nutty_bracer)
{
  struct char_data *vict = NULL, *victim = NULL;

  if (!ch)
    return FALSE;

  if (!cmd && !strcmp(argument, "identify"))
  {
    send_to_char(ch, "Randomly lash out...\r\n");
    return TRUE;
  }

  if (cmd)
    return FALSE;

  for (vict = world[ch->in_room].people; vict; vict = vict->next_in_room)
    if (IS_NPC(vict) && !IS_PET(vict) && (!victim || GET_LEVEL(vict) > GET_LEVEL(victim)))
      victim = vict;

  if (!FIGHTING(ch) && is_wearing(ch, 113803) && !rand_number(0, 1000) && victim)
  {
    act("\tLThe bracer on your arm begins to \tpvibrate\tL, sending a horrible pain\r\n"
        "up the back of your neck. You feel unable to control yourself as you\r\n"
        "lunge toward $N\tL with \tpi\tPn\tps\tpa\tPn\tpi\tPt\tpy\tL!!\tn",
        FALSE, ch, 0, victim, TO_CHAR);
    act("\tLThe bracer on $n\tL's arm begins to \tpvibrate\tL, sending a horrible shriek\r\n"
        "from his gut. You watch as $e lunges toward $N\tL with "
        "\tpi\tPn\tps\tpa\tPn\tpi\tPt\tpy\tL!!\tn",
        FALSE, ch, 0, victim, TO_ROOM);
    hit(ch, victim, TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, FALSE);
    return TRUE;
  }
  return FALSE;
}

/* pet from moonblade below */
#define SPIRIT_EAGLE 101225
/* moonblade 109802 */
SPECIAL(whisperwind)
{
  if (!ch)
    return FALSE;

  int s = 0, i = 0;
  struct char_data *pet = NULL;

  if (!cmd && !strcmp(argument, "identify"))
  {
    send_to_char(ch, "Procs cyclone, whisper 'blur' for attack blur, whisper"
                     " 'wind' to summon weapon spirit, whisper 'smite' for harm and "
                     "dispel evil.\r\n");
    return TRUE;
  }

  if (!is_wearing(ch, 109802))
    return FALSE;

  struct obj_data *whisperwind = (struct obj_data *)me;

  skip_spaces(&argument);

  if (!strcmp(argument, "wind") && CMD_IS("whisper"))
  {
    if (GET_OBJ_SPECTIMER(whisperwind, 1) > 0)
    {
      send_to_char(
          ch, "\tcAs you whisper '\tCwind\tc' to your \tWmoon\tCblade\tc, nothing happens.\tn\r\n");
      return TRUE;
    }

    if (check_npc_followers(ch, NPC_MODE_SPECIFIC, SPIRIT_EAGLE) <= 0)
    {
      act("\tcAs you whisper '\tCwind\tc' to your \tWmoon\tCblade\tc, "
          "a \tWghostly mist \tcswirls\r\n"
          "in the area around you.  When it finally dissipates, the "
          "spirit of the\r\nblade has come to your calling in the "
          "form of a majestic \tBeagle\tc.",
          1, ch, whisperwind, NULL, TO_CHAR);
      act("\tcAs $n whispers something to $s \tWmoon\tCblade\tc, "
          "a \tWghostly mist \tcswirls\r\n"
          "in the area around $m.  When it finally dissipates, the "
          "spirit of the \r\nblade has come to $s calling in the "
          "form of a majestic \tBeagle\tc.",
          1, ch, whisperwind, NULL, TO_ROOM);

      pet = read_mobile(real_mobile(SPIRIT_EAGLE), REAL);
      if (pet)
      {
        if (ZONE_FLAGGED(GET_ROOM_ZONE(ch->in_room), ZONE_WILDERNESS))
        {
          X_LOC(pet) = world[ch->in_room].coords[0];
          Y_LOC(pet) = world[ch->in_room].coords[1];
        }

        char_to_room(pet, ch->in_room);
        add_follower(pet, ch);
        SET_BIT_AR(AFF_FLAGS(pet), AFF_CHARM);

        GET_LEVEL(pet) = GET_LEVEL(ch);
        GET_MAX_HIT(pet) = GET_MAX_HIT(ch);
        GET_HIT(pet) = GET_MAX_HIT(pet);

        GET_OBJ_SPECTIMER(whisperwind, 1) = 72;

        return TRUE;
      }
      else
        return FALSE;
    }
    else
    {
      act("\tcAs you whisper '\tCwind\tc' to your \tWmoon\tCblade\tc, "
          "nothing seems to happen.\r\n"
          "The spirit of the blade is still somewhere in the realms!",
          1, ch, whisperwind, NULL, TO_CHAR);
      return TRUE;
    }
  }

  struct char_data *vict = FIGHTING(ch);

  /* random cyclone proc */
  if (!cmd && !rand_number(0, 39) && vict)
  {
    weapons_spells("\tcYour \tWmoon\tCblade \tcbegins to gyrate violently in your hands, almost "
                   "causing you to fumble.  As soon as you regain control, the area is "
                   "suddenly overwhelmed with vicious northern \tWgales\tc!\tn",
                   "\tc$n's \tWmoon\tCblade \tcbegins to gyrate violently in $s hands, causing "
                   "$m to almost fumble.  As soon as $e regains control, the area is "
                   "suddenly overwhelmed with vicious northern \tWgales\tc!\tn",
                   "\tc$n's \tWmoon\tCblade \tcbegins to gyrate violently in $s hands, causing "
                   "$m to almost fumble.  As soon as $e regains control, the area is "
                   "suddenly overwhelmed with vicious northern \tWgales\tc!\tn",
                   ch, vict, whisperwind, SPELL_WHIRLWIND);
    return TRUE;
  }

  /* whisper blur for 'blur attacks' */
  if (!strcmp(argument, "blur") && CMD_IS("whisper"))
  {
    if (vict && (vict->in_room == ch->in_room))
    {
      if (GET_OBJ_SPECTIMER(whisperwind, 0) > 0)
      {
        send_to_char(
            ch,
            "\tcAs you whisper '\tCblur\tc' to your \tWmoon\tCblade\tc, nothing happens.\tn\r\n");
        return TRUE;
      }

      act("\tcAs you whisper '\tCblur\tc' to your "
          "\tWmoon\tCblade\tc, it calls upon the northern \tWgale\r\n"
          "\tcand envelops you in a sw\tCir\tWl\tCin\tcg cyclone making "
          "you move like the wind!\tn",
          FALSE, ch, whisperwind, vict, TO_CHAR);
      act("\tcA northern \tWgale \tcblows in and envelops $n in a "
          "sw\tCir\tWl\tCin\tcg cyclone \r\nas $e invokes the power of "
          "$s \tWmoon\tCblade\tc, making $m move like the wind!\r\n"
          "$n \tCBLURS \tcas $e strikes $N \tcin rapid succession!\tn",
          1, ch, whisperwind, vict, TO_NOTVICT);
      act("\tcA northern \tWgale \tcblows in and envelops $n in a"
          "sw\tCir\tWl\tCin\tcg cyclone \r\nas $e invokes the power of "
          "$s \tWmoon\tCblade\tc, making $m move like the wind!\r\n"
          "$n \tCBLURS \tcas $e strikes YOU in rapid succession!\tn",
          1, ch, whisperwind, vict, TO_VICT);

      s = rand_number(8, 12);
      for (i = 0; i <= s; i++)
      {
        if (valid_fight_cond(ch, FALSE))
          hit(ch, vict, TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, FALSE);
        if (GET_POS(vict) == POS_DEAD)
          break;
      }

      GET_OBJ_SPECTIMER(whisperwind, 0) = 24;

      return TRUE; /* end for */
    } /* end if-fighting */
    else
      return FALSE;
  } /* end if-strcmp */

  else if (!strcmp(argument, "smite") && CMD_IS("whisper"))
  {
    if (GET_OBJ_SPECTIMER(whisperwind, 2) > 0)
    {
      send_to_char(
          ch,
          "\tcAs you whisper '\tCsmite\tc' to your \tWmoon\tCblade\tc, nothing happens.\tn\r\n");
      return TRUE;
    }

    if (vict && (vict->in_room == ch->in_room))
    {
      if (!IS_EVIL(vict))
      {
        act("\tcYour \tWmoon\tCblade \tctells you '\tWI will not harm "
            "non-evil beings with my power!\tc'\r\n"
            "You feel a seering burst of pain as you are \tCzapped \tcby "
            "your blade!\tn",
            1, ch, whisperwind, vict, TO_CHAR);
        act("\tc$n is \tCzapped \tcby his \tWmoon\tCblade \tcafter "
            "muttering something to it!",
            1, ch, whisperwind, vict, TO_ROOM);
        damage(ch, ch, rand_number(4, 12), -1, DAM_ENERGY, FALSE); // type -1 = no message

        return TRUE;
      }
      else
      {
        act("\tcAs you whisper '\tCsmite\tc' to your \tWmoon\tCblade\tc, "
            "it suddenly bursts into \trfl\tRam\tres\tc!\r\nYour blade flares "
            "angrily at $N \tcas it tries to smite $M \tcmightily!\tn",
            1, ch, whisperwind, vict, TO_CHAR);
        act("\tc$n mutters something to $s \tWmoon\tCblade \tcand it suddenly "
            "bursts into \tcfl\tRam\tres\tc!\r\n$n's blade seems to flare "
            "angrily at $N \tcas it tries to smite $M \tcmightily!\tn",
            1, ch, whisperwind, vict, TO_NOTVICT);
        act("\tc$n mutters something to $s \tWmoon\tCblade \tcand it suddenly "
            "bursts into \tcfl\tRam\tres\tc!\r\n$n's blade seems to flare "
            "angrily at you as it tries to smite you mightily!\tn",
            1, ch, whisperwind, vict, TO_VICT);

        /* harm spell */
        call_magic(ch, vict, 0, SPELL_HARM, 0, 30, CAST_WEAPON_SPELL);
        /* up to 3 dispel evils */
        for (i = 0; i < 3; i++)
        {
          if (valid_fight_cond(ch, TRUE))
            call_magic(ch, vict, 0, SPELL_DISPEL_EVIL, 0, 30, CAST_WEAPON_SPELL);
          if (GET_POS(vict) == POS_DEAD)
            break;
        }

        GET_OBJ_SPECTIMER(whisperwind, 2) = 6;

        return TRUE;
      }
      return FALSE;
    }
  }
  else
  {
    return FALSE;
  }

  /* failed! */
  return FALSE;
}
#undef SPIRIT_EAGLE

/* pet from moonblade below */
#define LARGE_SPIRIT_EAGLE 132131
/* moonblade 132118 */
SPECIAL(ancient_moonblade)
{
  if (!ch)
    return FALSE;

  int s = 0, i = 0;
  struct char_data *pet = NULL;

  if (!cmd && !strcmp(argument, "identify"))
  {
    send_to_char(ch, "Procs cyclone, whisper 'blur' for attack blur, whisper"
                     " 'wind' to summon weapon spirit, whisper 'smite' for harm and "
                     "dispel evil.\r\n");
    return TRUE;
  }

  if (!is_wearing(ch, 132118))
    return FALSE;

  struct obj_data *whisperwind = (struct obj_data *)me;

  skip_spaces(&argument);

  if (!strcmp(argument, "wind") && CMD_IS("whisper"))
  {
    if (GET_OBJ_SPECTIMER(whisperwind, 1) > 0)
    {
      send_to_char(
          ch, "\tcAs you whisper '\tCwind\tc' to your \tWmoon\tCblade\tc, nothing happens.\tn\r\n");
      return TRUE;
    }

    if (check_npc_followers(ch, NPC_MODE_SPECIFIC, LARGE_SPIRIT_EAGLE) <= 0)
    {
      act("\tcAs you whisper '\tCwind\tc' to your \tWmoon\tCblade\tc, "
          "a \tWghostly mist \tcswirls\r\n"
          "in the area around you.  When it finally dissipates, the "
          "spirit of the\r\nblade has come to your calling in the "
          "form of a majestic \tBeagle\tc.",
          1, ch, whisperwind, NULL, TO_CHAR);
      act("\tcAs $n whispers something to $s \tWmoon\tCblade\tc, "
          "a \tWghostly mist \tcswirls\r\n"
          "in the area around $m.  When it finally dissipates, the "
          "spirit of the \r\nblade has come to $s calling in the "
          "form of a majestic \tBeagle\tc.",
          1, ch, whisperwind, NULL, TO_ROOM);

      pet = read_mobile(real_mobile(LARGE_SPIRIT_EAGLE), REAL);
      if (pet)
      {
        if (ZONE_FLAGGED(GET_ROOM_ZONE(ch->in_room), ZONE_WILDERNESS))
        {
          X_LOC(pet) = world[ch->in_room].coords[0];
          Y_LOC(pet) = world[ch->in_room].coords[1];
        }

        char_to_room(pet, ch->in_room);
        add_follower(pet, ch);
        SET_BIT_AR(AFF_FLAGS(pet), AFF_CHARM);

        GET_LEVEL(pet) = GET_LEVEL(ch);
        GET_MAX_HIT(pet) = GET_MAX_HIT(ch);
        GET_HIT(pet) = GET_MAX_HIT(pet);

        GET_OBJ_SPECTIMER(whisperwind, 1) = 72;

        return TRUE;
      }
      else
        return FALSE;
    }
    else
    {
      act("\tcAs you whisper '\tCwind\tc' to your \tWmoon\tCblade\tc, "
          "nothing seems to happen.\r\n"
          "The spirit of the blade is still somewhere in the realms!",
          1, ch, whisperwind, NULL, TO_CHAR);
      return TRUE;
    }
  }

  struct char_data *vict = FIGHTING(ch);

  /* random cyclone proc */
  if (!cmd && !rand_number(0, 19) && vict)
  {
    weapons_spells("\tcYour \tWmoon\tCblade \tcbegins to gyrate violently in your hands, almost "
                   "causing you to fumble.  As soon as you regain control, the area is "
                   "suddenly overwhelmed with vicious northern \tWgales\tc!\tn",
                   "\tc$n's \tWmoon\tCblade \tcbegins to gyrate violently in $s hands, causing "
                   "$m to almost fumble.  As soon as $e regains control, the area is "
                   "suddenly overwhelmed with vicious northern \tWgales\tc!\tn",
                   "\tc$n's \tWmoon\tCblade \tcbegins to gyrate violently in $s hands, causing "
                   "$m to almost fumble.  As soon as $e regains control, the area is "
                   "suddenly overwhelmed with vicious northern \tWgales\tc!\tn",
                   ch, vict, whisperwind, SPELL_WHIRLWIND);
    return TRUE;
  }

  /* whisper blur for 'blur attacks' */
  if (!strcmp(argument, "blur") && CMD_IS("whisper"))
  {
    if (vict && (vict->in_room == ch->in_room))
    {
      if (GET_OBJ_SPECTIMER(whisperwind, 0) > 0)
      {
        send_to_char(
            ch,
            "\tcAs you whisper '\tCblur\tc' to your \tWmoon\tCblade\tc, nothing happens.\tn\r\n");
        return TRUE;
      }

      act("\tcAs you whisper '\tCblur\tc' to your "
          "\tWmoon\tCblade\tc, it calls upon the northern \tWgale\r\n"
          "\tcand envelops you in a sw\tCir\tWl\tCin\tcg cyclone making "
          "you move like the wind!\tn",
          FALSE, ch, whisperwind, vict, TO_CHAR);
      act("\tcA northern \tWgale \tcblows in and envelops $n in a "
          "sw\tCir\tWl\tCin\tcg cyclone \r\nas $e invokes the power of "
          "$s \tWmoon\tCblade\tc, making $m move like the wind!\r\n"
          "$n \tCBLURS \tcas $e strikes $N \tcin rapid succession!\tn",
          1, ch, whisperwind, vict, TO_NOTVICT);
      act("\tcA northern \tWgale \tcblows in and envelops $n in a"
          "sw\tCir\tWl\tCin\tcg cyclone \r\nas $e invokes the power of "
          "$s \tWmoon\tCblade\tc, making $m move like the wind!\r\n"
          "$n \tCBLURS \tcas $e strikes YOU in rapid succession!\tn",
          1, ch, whisperwind, vict, TO_VICT);

      s = rand_number(9, 13);
      for (i = 0; i <= s; i++)
      {
        if (valid_fight_cond(ch, FALSE))
          hit(ch, vict, TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, FALSE);
        if (GET_POS(vict) == POS_DEAD)
          break;
      }

      GET_OBJ_SPECTIMER(whisperwind, 0) = 24;

      return TRUE; /* end for */
    } /* end if-fighting */
    else
      return FALSE;
  } /* end if-strcmp */

  else if (!strcmp(argument, "smite") && CMD_IS("whisper"))
  {
    if (GET_OBJ_SPECTIMER(whisperwind, 2) > 0)
    {
      send_to_char(
          ch,
          "\tcAs you whisper '\tCsmite\tc' to your \tWmoon\tCblade\tc, nothing happens.\tn\r\n");
      return TRUE;
    }

    if (vict && (vict->in_room == ch->in_room))
    {
      if (!IS_EVIL(vict))
      {
        act("\tcYour \tWmoon\tCblade \tctells you '\tWI will not harm "
            "non-evil beings with my power!\tc'\r\n"
            "You feel a seering burst of pain as you are \tCzapped \tcby "
            "your blade!\tn",
            1, ch, whisperwind, vict, TO_CHAR);
        act("\tc$n is \tCzapped \tcby his \tWmoon\tCblade \tcafter "
            "muttering something to it!",
            1, ch, whisperwind, vict, TO_ROOM);
        damage(ch, ch, rand_number(4, 12), -1, DAM_ENERGY, FALSE); // type -1 = no message

        return TRUE;
      }
      else
      {
        act("\tcAs you whisper '\tCsmite\tc' to your \tWmoon\tCblade\tc, "
            "it suddenly bursts into \trfl\tRam\tres\tc!\r\nYour blade flares "
            "angrily at $N \tcas it tries to smite $M \tcmightily!\tn",
            1, ch, whisperwind, vict, TO_CHAR);
        act("\tc$n mutters something to $s \tWmoon\tCblade \tcand it suddenly "
            "bursts into \tcfl\tRam\tres\tc!\r\n$n's blade seems to flare "
            "angrily at $N \tcas it tries to smite $M \tcmightily!\tn",
            1, ch, whisperwind, vict, TO_NOTVICT);
        act("\tc$n mutters something to $s \tWmoon\tCblade \tcand it suddenly "
            "bursts into \tcfl\tRam\tres\tc!\r\n$n's blade seems to flare "
            "angrily at you as it tries to smite you mightily!\tn",
            1, ch, whisperwind, vict, TO_VICT);

        /* harm spell */
        call_magic(ch, vict, 0, SPELL_HARM, 0, 30, CAST_WEAPON_SPELL);
        /* up to 3 dispel evils */
        for (i = 0; i < 3; i++)
        {
          if (valid_fight_cond(ch, TRUE))
            call_magic(ch, vict, 0, SPELL_DISPEL_EVIL, 0, 30, CAST_WEAPON_SPELL);
          if (GET_POS(vict) == POS_DEAD)
            break;
        }

        GET_OBJ_SPECTIMER(whisperwind, 2) = 6;

        return TRUE;
      }
      return FALSE;
    }
  }
  else
  {
    return FALSE;
  }

  /* failed! */
  return FALSE;
}
#undef LARGE_SPIRIT_EAGLE

SPECIAL(celestial_sword)
{
  if (!ch)
    return FALSE;

  if (!cmd && !strcmp(argument, "identify"))
  {
    send_to_char(ch, "Whisper 'revive' to resurrect from your last corpse every 2 days (don't have "
                     "to be in the same room).\r\n"
                     "Whisper 'messiah' to cast a strong group heal every 12 hours\r\n");
    return TRUE;
  }

  if (!is_wearing(ch, 132300))
    return FALSE;

  struct obj_data *celestial = (struct obj_data *)me, *obj = NULL;
  // bool found = FALSE;

  skip_spaces(&argument);

  if (!strcmp(argument, "revive") && CMD_IS("whisper"))
  {
    /* still on cooldown */
    if (GET_OBJ_SPECTIMER(celestial, 0) > 0)
    {
      send_to_char(ch, "\tcAs you whisper '\tWrevive\tc' to %s, nothing happens.\tn\r\n",
                   GET_OBJ_SHORT(celestial));
      return TRUE;
    }

    /* lets try to find your corpse.. */
    for (obj = object_list; obj; obj = obj->next)
    {
      /* dummychecks */
      if (!obj || !ch)
        continue;
      if (obj->in_room == NOWHERE)
        continue;
      if (ch->in_room == NOWHERE)
        continue;

      if (!isname_obj(GET_NAME(ch), obj->name))
        continue;

      /* found a name match at least! */

      /* is the item a corpse? */
      if (!IS_CORPSE(obj) || !GET_OBJ_VAL(obj, 4))
        continue;

      /* is this our corpse? */
      if (GET_OBJ_VAL(obj, 4) != GET_IDNUM(ch))
        continue;

      /* think we're good, lets fire! */
      if (call_magic(ch, ch, obj, SPELL_RESURRECT, 0, 30, CAST_WEAPON_SPELL))
      {
        GET_OBJ_SPECTIMER(celestial, 0) = 48;
        return TRUE;
      }
    } /* end for loop */

    send_to_char(ch, "Your corpse can't be found...\r\n");
    return FALSE;
  } /* end self revive proc */

  if (!strcmp(argument, "messiah") && CMD_IS("whisper"))
  {
    /* still on cooldown */
    if (GET_OBJ_SPECTIMER(celestial, 0) > 0)
    {
      send_to_char(ch, "\tcAs you whisper '\tWmessiah\tc' to %s, nothing happens.\tn\r\n",
                   GET_OBJ_SHORT(celestial));
      return TRUE;
    }

    /* i couldn't get this to stop crashing, disabled for now -zusuk */
#if 0
    /* lets try to find your corpse.. */
    for (obj = object_list; obj; obj = obj->next)
    {
      /* dummychecks */
      if (!obj || !ch)
        continue;
      if (obj->in_room == NOWHERE)
        continue;
      if (ch->in_room == NOWHERE)
        continue;

      /* is the item a corpse? */
      if (!IS_CORPSE(obj))
        continue;

      if (!GET_OBJ_VAL(obj, 4))
        continue;

      /* corpse should be on the floor somewhere */
      if (obj->in_room != IN_ROOM(ch))
        continue;

      /* think we're good, lets try! */
      if (call_magic(ch, ch, obj, SPELL_RESURRECT, 0, 30, CAST_WEAPON_SPELL))
      {
        found = TRUE;
      }
    } /* end for loop */

    if (found)
    {
      GET_OBJ_SPECTIMER(celestial, 0) = 96;
      return TRUE;
    }

    send_to_char(ch, "No corpses were found...\r\n");
    return FALSE;
  } /* end room revive proc */
#endif

    call_magic(ch, NULL, NULL, SPELL_GROUP_HEAL, 0, 30, CAST_WEAPON_SPELL);
    call_magic(ch, NULL, NULL, SPELL_GROUP_HEAL, 0, 30, CAST_WEAPON_SPELL);
    call_magic(ch, NULL, NULL, SPELL_GROUP_HEAL, 0, 30, CAST_WEAPON_SPELL);
    GET_OBJ_SPECTIMER(celestial, 0) = 12;

    return TRUE; /* made it! */
  }

  /* failed? */
  return FALSE;
}


/* from homeland */
SPECIAL(floating_teleport)
{
  int door;
  struct obj_data *obj = (struct obj_data *)me;
  room_rnum roomnum;

  skip_spaces(&argument);

  if (cmd)
    return FALSE;

  if (!cmd && !strcmp(argument, "identify"))
  {
    send_to_char(ch, "This appears to be a floating (moving) portal.\r\n");
    return TRUE;
  }

  if (((door = rand_number(0, 30)) < NUM_OF_DIRS) && CAN_GO(obj, door) &&
      (world[EXIT(obj, door)->to_room].zone == world[obj->in_room].zone))
  {
    roomnum = EXIT(obj, door)->to_room;
    act("$p floats away.", FALSE, 0, obj, 0, TO_ROOM);
    obj_from_room(obj);
    obj_to_room(obj, roomnum);
    act("$p floats in.", FALSE, 0, obj, 0, TO_ROOM);
    return TRUE;
  }
  return FALSE;
}

/* from homeland */
SPECIAL(vengeance)
{
  if (!ch)
    return FALSE;

  if (!cmd && !strcmp(argument, "identify"))
  {
    send_to_char(ch, "Procs mass cure light and word of faith.\r\n");
    return TRUE;
  }

  struct char_data *vict = FIGHTING(ch);

  if (cmd || !vict)
    return FALSE;

  int power = 10;
  if (GET_OBJ_VNUM(((struct obj_data *)me)) == 101199)
    power = 5;
  if (rand_number(0, power))
    return FALSE;

  if (GET_HIT(ch) < GET_MAX_HIT(ch) && rand_number(0, 4))
  {
    weapons_spells("\tWYour sword begins to \tphum \tWloudly and then \tCglows\tW as it pours its "
                   "healing powers into you.\tn",
                   "\tWYour sword begins to \tphum \tWloudly and then \tCglows\tW as it pours its "
                   "healing powers into you.\tn",
                   "$n's \tWsword begings to \tphum \tWloudly and then \tCglow\tW as it pours its "
                   "healing powers into $m\tW.\tn",
                   ch, vict, (struct obj_data *)me, 0);
    call_magic(ch, 0, 0, SPELL_MASS_CURE_LIGHT, 0, GET_LEVEL(ch), CAST_WEAPON_SPELL);
    return TRUE;
  }
  weapons_spells(
      "\tWYour blade starts to shake violently, nearly tearing itself from your grip,\tn\r\n"
      "\tWas it begins to \tCglow\tW with a \tcholy light\tW.  Suddenly a \tYblinding "
      "\tfflash\tn\tW of pure\tn\r\n"
      "\tWgoodness is released from the sword striking down any \trevil\tW in the area.\tn",

      "\tW$n's\tW blade starts to shake violently, nearly tearing itself from $s grip,\tn\r\n"
      "\tWas it begins to \tCglow\tW with a \tcholy light\tW.  Suddenly a \tYblinding "
      "\tfflash\tn\tW of pure\tn\r\n"
      "\tWgoodness is released from the sword striking down any \trevil\tW in the area.\tn",

      "\tW$n's\tW blade starts to shake violently, nearly tearing itself from $s grip,\tn\r\n"
      "\tWas it begins to \tCglow\tW with a \tcholy light\tW.  Suddenly a \tYblinding "
      "\tfflash\tn\tW of pure\tn\r\n"
      "\tWgoodness is released from the sword striking down any \trevil\tW in the area.\tn",
      ch, vict, (struct obj_data *)me, SPELL_WORD_OF_FAITH);
  return TRUE;
}

/* from homeland */
/* crashing!! */
SPECIAL(bloodaxe)
{
  if (!ch)
    return FALSE;

  if (!cmd && !strcmp(argument, "identify"))
  {
    send_to_char(ch, "Proc:  Bite.\r\n");
    return TRUE;
  }

  if (!is_wearing(ch, 117014))
    return FALSE;

  struct char_data *vict = FIGHTING(ch);

  if (cmd || !vict || rand_number(0, 16))
    return FALSE;

  int dam = rand_number(8, 8);

  GET_HIT(vict) -= dam;

  if (dam < GET_HIT(vict))
  {
    weapons_spells(
        "\tLYour $p \tLstarts \trhumming \tLlouder and louder. Suddenly "
        "the axehead reshapes into a powerful maw and bites $N\tL in the throat.\tRBlood \tLflows "
        "between the the canine jaws and spills to the ground and $N\tL howls in pain. With "
        "a satisfied grin the maw reverts back to an axehead.\tn",
        "$n's $p \tLstarts \trhumming \tLlouder and louder. Suddenly "
        "the axehead reshapes into a powerful maw and bites you in the throat. \tRBlood \tLflows "
        "between the canine jaws and spills to the ground and you howl in pain. With "
        "a satisfied grin the maw reverts back to an axehead.\tn",
        "$n's $p \tLstarts \trhumming \tLlouder and louder. Suddenly "
        "the axehead reshapes into a powerful maw and bites $N\tL in the throat. \tRBlood \tLflows "
        "between the the canine jaws and spills to the ground and $N\tL howls in pain. With "
        "a satisfied grin the maw reverts back to an axehead.\tn",
        ch, vict, (struct obj_data *)me, 0);
  }
  else
  {
    weapons_spells("\tLYour $p \tLstarts \trhumming \tLlouder and louder. Suddenly the "
                   "axehead reshapes into a powerful maw and bites $N\tL in the throat. $N\tL "
                   "looks at the \tRblood \tLflowing freely from the wound, then $S\tL eyes "
                   "\twglazes \tLover and $E falls to the ground, \trDEAD\tL. With a "
                   "satisfied grin the maw reverts back to an axehead.\tn",
                   "$n's $p \tLstarts \trhumming \tLlouder and louder. Suddenly the axehead "
                   "reshapes into a powerful maw and bites'you in the throat. You look at "
                   "the \tRblood \tLflowing freely from the wound, then your eyes \twglazes "
                   "\tLover and you fall to the ground, \trDEAD\tL.\tn",
                   "$n's $p \tLstarts \trhumming \tLlouder and louder. Suddenly the axehead "
                   "reshapes into a powerful maw and bites $N\tL in the throat. $N\tL looks at the "
                   "\tRblood \tLflowing freely from the wound, then $S eyes \twglazes "
                   "\tLover and $E falls to the ground, \trDEAD\tL. With a satisfied grin "
                   "the maw reverts back to an axehead.\tn",
                   ch, vict, (struct obj_data *)me, 0);
    GET_HIT(vict) = -100;
  }
  return TRUE;
}

/* from homeland */
SPECIAL(skullsmasher)
{
  if (!ch)
    return FALSE;

  if (!cmd && !strcmp(argument, "identify"))
  {
    send_to_char(ch, "Proc:  Knockdown.\r\n");
    return TRUE;
  }

  struct char_data *vict = FIGHTING(ch);

  if (cmd || !vict)
    return FALSE;

  int power = 25;

  if (GET_OBJ_VNUM(((struct obj_data *)me)) == 101850)
    power = 15;
  if (rand_number(0, power))
    return FALSE;
  if (MOB_FLAGGED(vict, MOB_NOBASH))
    return FALSE;
  if (AFF_FLAGGED(vict, AFF_IMMATERIAL))
    return FALSE;

  weapons_spells("\tLAs you swing your maul at $N \tLit connects with $S head\tn\r\n"
                 "\tLand suddenly \tWgl\two\tWws brigh\twt\tWly\tL.  A look of overwhelming "
                 "\trpain\tL shows on\tn\r\n"
                 "\tL$S face as $E slowly slumps to the ground.\tn",

                 "\tLAs $n \tLswings $s maul at you it connects with your head\tn\r\n"
                 "\tLand suddenly \tWgl\two\tWws brigh\twt\tWly\tL.  A feeling of overwhelming "
                 "\trpain\tL courses\tn\r\n"
                 "\tLthrough your body, and you feel yourself slump to the ground.\tn",

                 "\tLAs $n \tLswings $s maul at $N \tLit connects with $S head\tn\r\n"
                 "\tLand suddenly \tWgl\two\tWws brigh\twt\tWly\tL.  A look of overwhelming "
                 "\trpain \tLshows on\tn\r\n"
                 "\tL$S face as $E slowly slumps to the ground.\tn",
                 ch, vict, (struct obj_data *)me, 0);
  change_position(vict, POS_SITTING);
  USE_FULL_ROUND_ACTION(vict);
  return TRUE;
}

/* from homeland */
SPECIAL(acidsword)
{
  int dam;
  struct char_data *vict = NULL;

  if (!ch)
    return FALSE;

  if (!cmd && !strcmp(argument, "identify"))
  {
    send_to_char(ch, "Proc: Acid corrosion.\r\n");
    return TRUE;
  }

  if (!is_wearing(ch, 135199))
    return FALSE;

  vict = FIGHTING(ch);

  if (cmd || !vict || rand_number(0, 16))
    return FALSE;

  dam = dice(4, 3);

  GET_HIT(vict) -= dam;

  if (GET_HIT(vict) > -9)
  {
    weapons_spells("\tLYour\tn $p \tLstarts to \tGglow \tLwith a \tgd\tGi\tgm gr\tGe\tgen\r\n"
                   "sh\tGe\tgen \tLand suddenly a \tgth\tGi\tgn str\tGea\tgm of ac\tGi\tgd\r\n"
                   "sp\tGe\tgws fo\tGr\tgth \tLfrom the tip of the blade and strikes\tn\r\n"
                   "$N\tL, hissing as it starts to corrode.\tn",
                   "$n's $p \tLstarts to \tGglow \tLwith a \tgd\tGi\tgm gr\tGe\tgen\r\n"
                   "sh\tGe\tgen \tLand suddenly a \tgth\tGi\tgn str\tGea\tgm of ac\tGi\tgd\r\n"
                   "sp\tGe\tgws fo\tGr\tgth \tLfrom the tip of the blade and strikes\tn\r\n"
                   "you\tL, hissing as it starts to corrode.\tn",
                   "$n's $p \tLstarts to \tGglow \tLwith a \tgd\tGi\tgm gr\tGe\tgen\r\n"
                   "sh\tGe\tgen \tLand suddenly a \tgth\tGi\tgn str\tGea\tgm of ac\tGi\tgd\r\n"
                   "sp\tGe\tgws fo\tGr\tgth \tLfrom the tip of the blade and strikes\r\n"
                   "$N, hissing as it starts to corrode.\tn",
                   ch, vict, (struct obj_data *)me, 0);
  }
  else
  {
    weapons_spells("\tLYour\tn $p \tLstarts to \tGglow \tLwith a \tgd\tGi\tgm gr\tGe\tgen\r\n"
                   "sh\tGe\tgen \tLand suddenly a \tgth\tGi\tgn str\tGea\tgm of ac\tGi\tgd\r\n"
                   "sp\tGe\tgws fo\tGr\tgth \tLfrom the tip of the blade and strikes\tn\r\n"
                   "$N\tL, hissing as it melts\tn $N \tLto o\twoz\tLing pulp.\tn",
                   "$n's $p \tLstarts to \tGglow \tLwith a \tgd\tGi\tgm gr\tGe\tgen\r\n"
                   "sh\tGe\tgen \tLand suddenly a \tgth\tGi\tgn str\tGea\tgm of ac\tGi\tgd\r\n"
                   "sp\tGe\tgws fo\tGr\tgth \tLfrom the tip of the blade and strikes\tn\r\n"
                   "$N\tL, hissing as it melts\tn $N \tLto o\twoz\tLing pulp.\tn",
                   "$n's $p \tLstarts to \tGglow \tLwith a \tgd\tGi\tgm gr\tGe\tgen\r\n"
                   "sh\tGe\tgen \tLand suddenly a \tgth\tGi\tgn str\tGea\tgm of ac\tGi\tgd\r\n"
                   "sp\tGe\tgws fo\tGr\tgth \tLfrom the tip of the blade and strikes\r\n"
                   "you, hissing as it melts you to o\twoz\tLing pulp.\tn",
                   ch, vict, (struct obj_data *)me, 0);
    GET_HIT(vict) = -50;
  }
  return TRUE;
}

/* malevolence - this has a spec abillity vamp AND a blur attack -zusuk */
SPECIAL(malevolence)
{
  struct char_data *vict = NULL;
  int num_hits = 0, i = 0;
  struct obj_data *malevolence = (struct obj_data *)me;

  if (!ch)
    return FALSE;

  if (!cmd && !strcmp(argument, "identify"))
  {
    send_to_char(ch, "Proc: Attack Blur (3-5 bonus attacks on proc)\r\n");
    return TRUE;
  }

  if (!is_wearing(ch, 132101))
    return FALSE;

  vict = FIGHTING(ch);

  if (cmd || !vict || rand_number(0, 15))
    return FALSE;

  /* this will not proc more than once per 6 seconds */
  if (!char_has_mud_event(ch, eBLUR_ATTACK_DELAY))
  {
    attach_mud_event(new_mud_event(eBLUR_ATTACK_DELAY, ch, NULL), 6 * PASSES_PER_SEC);

    act("$p\tn glows with a bright \tYyellow\tn sheen before pulsing with \tRblood red malevolent "
        "light\tn as your attacks begin to speed up!",
        TRUE, ch, malevolence, vict, TO_CHAR);
    act("$p\tn glows with a bright \tYyellow\tn sheen before pulsing with \tRblood red malevolent "
        "light\tn as $n's\tn attacks begin to speed up!",
        TRUE, ch, malevolence, vict, TO_VICT);
    act("$p\tn glows with a bright \tYyellow\tn sheen before pulsing with \tRblood red malevolent "
        "light\tn as $n's\tn attacks begin to speed up!",
        TRUE, ch, malevolence, vict, TO_NOTVICT);

    num_hits = rand_number(3, 5);

    for (i = 0; i <= num_hits; i++)
    {
      if (valid_fight_cond(ch, TRUE))
        hit(ch, vict, TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, FALSE);
    }
  }

  return TRUE;
}

SPECIAL(rune_scimitar)
{
  struct char_data *vict = NULL;
  int num_hits = 0, i = 0;
  struct obj_data *scimitar = (struct obj_data *)me;

  if (!ch)
    return FALSE;

  if (!cmd && !strcmp(argument, "identify"))
  {
    send_to_char(ch, "Proc: Attack Blur (4-7 bonus attacks on proc)\r\n");
    send_to_char(
        ch, "Proc: Deft Parry - on parry will do a light vamp attack (not negative energy)\r\n");
    send_to_char(
        ch, "Proc: Deft Dodge - on dodge will do a light vamp attack (not negative energy)\r\n");
    return TRUE;
  }

  if (!is_wearing(ch, 132126))
    return FALSE;

  vict = FIGHTING(ch);

  if (cmd || !vict)
    return FALSE;

  /* blur attack proc */
  if (!rand_number(0, 15))
  {
    /* this will not proc more than once per 6 seconds */
    if (!char_has_mud_event(ch, eBLUR_ATTACK_DELAY))
    {
      attach_mud_event(new_mud_event(eBLUR_ATTACK_DELAY, ch, NULL), 6 * PASSES_PER_SEC);
      act("$p\tY glows with a \tLdark sheen\tY before pulsing with \tBblue arcane light\tY as your "
          "attacks begin to speed up!\tn",
          TRUE, ch, scimitar, vict, TO_CHAR);
      act("$p\tY glows with a \tLdark sheen\tY before pulsing with \tBblue arcane light\tn as "
          "$n's\tY attacks begin to speed up!\tn",
          TRUE, ch, scimitar, vict, TO_VICT);
      act("$p\tY glows with a \tLdark sheen\tY before pulsing with \tBblue arcane light\tn as "
          "$n's\tY attacks begin to speed up!\tn",
          TRUE, ch, scimitar, vict, TO_NOTVICT);

      num_hits = rand_number(4, 7);

      for (i = 0; i <= num_hits; i++)
      {
        if (valid_fight_cond(ch, TRUE))
          hit(ch, vict, TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, FALSE);
      }
      return TRUE;
    }
  }

  /* parry proc */
  else if (!strcmp(argument, "parry") && rand_number(0, 3))
  {
    act("\tLAs you parry the attack, \tn$p \tCglows brightly\tL as it steals some \trlifeforce\tn "
        "\tLfrom $N \tLand transfers it back to you.\tn",
        FALSE, ch, scimitar, vict, TO_CHAR);
    act("\tLAs \tn$n\tL parries your attack, \tn$p \tCglows brightly\tL as it steals some "
        "\trlifeforce\tn "
        "\tLfrom you and transfers it back to $m.\tn",
        FALSE, ch, scimitar, vict, TO_VICT);
    act("\tLAs \tn$n\tL parries \tn$N's\tL attack, \tn$p \tCglows brightly\tL as it steals some "
        "\trlifeforce\tn "
        "\tLfrom $N\tL.\tn",
        FALSE, ch, scimitar, vict, TO_NOTVICT);
    damage(ch, vict, dice(10, 5), -1, DAM_ENERGY, FALSE); // type -1 = no dam message
    call_magic(ch, ch, 0, SPELL_CURE_CRITIC, 0, 1, CAST_WEAPON_SPELL);
    return TRUE;
  }

  /* dodge proc */
  else if (!strcmp(argument, "dodge") && !rand_number(0, 3))
  {
    act("\tLAs you dodge the attack, \tn$p \tCglows brightly\tL as it steals some \trlifeforce\tn "
        "\tLfrom $N \tLand transfers it back to you.\tn",
        FALSE, ch, scimitar, vict, TO_CHAR);
    act("\tLAs \tn$n\tL dodges your attack, \tn$p \tCglows brightly\tL as it steals some "
        "\trlifeforce\tn "
        "\tLfrom you and transfers it back to $m.\tn",
        FALSE, ch, scimitar, vict, TO_VICT);
    act("\tLAs \tn$n\tL dodges \tn$N's\tL attack, \tn$p \tCglows brightly\tL as it steals some "
        "\trlifeforce\tn "
        "\tLfrom $N\tL.\tn",
        FALSE, ch, scimitar, vict, TO_NOTVICT);
    damage(ch, vict, dice(10, 5), -1, DAM_ENERGY, FALSE); // type -1 = no dam message
    call_magic(ch, ch, 0, SPELL_CURE_CRITIC, 0, 1, CAST_WEAPON_SPELL);
    return TRUE;
  }

  /* didn't do anything! */
  return FALSE;
}

/* from homeland */
SPECIAL(snakewhip)
{
  // struct affected_type af;
  struct obj_data *weepan = (struct obj_data *)me;
  int dam;

  if (!ch)
    return FALSE;

  if (!cmd && !strcmp(argument, "identify"))
  {
    send_to_char(ch, "Proc:  Drow-only, Snake-Bite.\r\n");
    return TRUE;
  }

  struct char_data *vict = FIGHTING(ch);

  if (cmd || !vict || GET_POS(ch) == POS_DEAD)
    return FALSE;

  if ((GET_RACE(ch) != RACE_DROW || GET_SEX(ch) != SEX_FEMALE) && is_wearing(ch, 135500))
  {
    if (GET_HIT(ch) > 0)
    {
      act("\tLYour $p \tLh\tYi\tLss\tYe\tLs angrily as it turns against you.\r\n"
          "All three snakeheads suddenly lunges forwardand sink their fangs in you throat. \r\n"
          "You barely have time to feel the terrible pain before you fall over with "
          "\tRbl\tro\tRod\r\n"
          "\tLflowing freely from the wounds in your neck.\tn",
          FALSE, ch, weepan, 0, TO_CHAR);

      act("$n's $p \tLh\tYi\tLss\tYe\tLs angrily as it turns against $n\tL. \r\n"
          "All three snakeheads suddenly lunges forward and sink their fangs in $n's \tLthroat.\r\n"
          "$n \tLbarely have time to feel the terrible pain before falling over with "
          "\tRbl\tro\tRod\r\n"
          "\tLflowing freely from the wounds in the neck.\tn",
          FALSE, ch, weepan, 0, TO_ROOM);
      GET_HIT(ch) = -5;
      change_position(ch, POS_INCAP);
    }
    return TRUE;
  }
  if (rand_number(0, 15))
    return FALSE;

  dam = dice(GET_LEVEL(ch), 3);

  GET_HIT(vict) -= dam;

  if (GET_HIT(vict) > -9)
  {
    weapons_spells(
        "\tLYour $p \tLh\tYi\tLss\tYe\tLs with fury as all three snakeheads suddenly lunges for "
        "$N\tL.\r\n"
        "Their fangs sink deep into the \tRfl\tre\tRsh \tLand $N \tLcries out in pain.\tn",
        "$n's $p \tLh\tYi\tLss\tYe\tLs\r\nwith fury as all three snakeheads suddenly lunges for "
        "you.\r\n"
        "Their fangs sink deep into the \tRfl\tre\tRsh \tLand you cry out in pain.\tn",
        "$n's $p \tLh\tYi\tLss\tYe\tLs\r\n with fury as all three snakeheads suddenly lunges for "
        "$N\tL.\r\n"
        "Their fangs sink deep into the \tRfl\tre\tRsh \tLand $N \tLcries out in pain.\tn",
        ch, vict, (struct obj_data *)me, 0);
  }
  else
  {
    weapons_spells("\tLYour $p \tLh\tYi\tLss\tYe\tLs with fury as all three snakeheads suddenly "
                   "lunges for $N\tL.\r\n"
                   "Their fangs sink deep into the \tRfl\tre\tRsh\tL, draining away the remaining "
                   "life of $N \tLwho\r\n"
                   "falls over dead.\tn",
                   "$n's $p \tLh\tYi\tLss\tYe\tLs\r\n with fury as all three snakeheads suddenly "
                   "lunges for you.\r\n"
                   "Their fangs sink deep into the \tRfl\tre\tRsh\tL, draining away your remaining "
                   "life and\r\n"
                   "you fall over dead.\tn",
                   "$n's $p \tLh\tYi\tLss\tYe\tLs\r\n with fury as all three snakeheads suddenly "
                   "lunges for $N\tL.\r\n"
                   "Their fangs sink deep into the \tRfl\tre\tRsh\tL, draining away the remaining "
                   "life of $N \tLwho\r\n"
                   "falls over dead.\tn",
                   ch, vict, (struct obj_data *)me, 0);
    GET_HIT(vict) = -50;
  }
  return TRUE;
}

/* from homeland */
SPECIAL(tormblade)
{
  if (!ch)
    return FALSE;

  if (!cmd && !strcmp(argument, "identify"))
  {
    send_to_char(ch, "Proc only vs Evil:  Dispel Magic randomly on hit.\r\n"
                     "                    Torms Protection of Evil on critical hits.\r\n");
    return TRUE;
  }

  struct char_data *vict;
  struct affected_type af;

  if (cmd)
    return FALSE;

  vict = FIGHTING(ch);
  if (!vict)
    return FALSE;
  if (!IS_EVIL(vict))
    return FALSE;

  if (argument)
  {
    skip_spaces(&argument);
    if (!strcmp(argument, "critical"))
    {
      // okies, we assume its a crit then.
      act("$n's $p shines as it protects $m.", FALSE, ch, (struct obj_data *)me, 0, TO_ROOM);
      act("Your $p shines as it protects you.", FALSE, ch, (struct obj_data *)me, 0, TO_CHAR);
      new_affect(&af);
      af.spell = SPELL_PROT_FROM_EVIL;
      af.modifier = 2;
      af.location = APPLY_AC_NEW;
      af.duration = dice(1, 4);
      affect_join(ch, &af, TRUE, FALSE, FALSE, FALSE);
      return TRUE;
    }
  }
  if (!rand_number(0, 30))
  {
    act("$n's $p hums loudly.", FALSE, ch, (struct obj_data *)me, 0, TO_ROOM);
    act("Your $p hums loudly.", FALSE, ch, (struct obj_data *)me, 0, TO_CHAR);
    call_magic(ch, vict, 0, SPELL_DISPEL_MAGIC, 0, GET_LEVEL(ch), CAST_WEAPON_SPELL);
    return TRUE;
  }

  return FALSE;
}

/* from homeland */
SPECIAL(witherdirk)
{
  if (!ch)
    return FALSE;

  if (!cmd && !strcmp(argument, "identify"))
  {
    send_to_char(ch, "Proc: Contagion\r\n");
    return TRUE;
  }

  struct char_data *vict;

  if (cmd)
    return FALSE;

  vict = FIGHTING(ch);
  if (!vict)
    return FALSE;

  if (rand_number(0, 30))
    return FALSE;

  weapons_spells(
      "\tLThe dirk \trwrithes\tL and \twtwists\tL as it bites deep into $N\tL's skin,\tn\r\n"
      "\tgputrid\tr blo\tro\trd\tL wells up in $S eyes, causing great pain and discomfort.\tn",
      "\tLThe dirk \trwrithes\tL and \twtwists\tL as it bites deep into YOUR skin,\tn\r\n"
      "\tgputrid\tr blo\tro\trd\tL wells up in YOUR eyes, causing great pain and discomfort.\tn",
      "\tLThe dirk \trwrithes\tL and \twtwists\tL as it bites deep into $N\tL's skin,\tn\r\n"
      "\tgputrid\tr blo\tro\trd\tL wells up in $S eyes, causing great pain and discomfort.\tn",
      ch, vict, (struct obj_data *)me, SPELL_CONTAGION);

  return TRUE;
}

/* from homeland */
SPECIAL(air_sphere)
{
  int dam = 0;
  struct char_data *vict;
  struct affected_type af;

  if (!ch)
    return FALSE;

  if (!cmd && !strcmp(argument, "identify"))
  {
    send_to_char(ch, "Proc: Electric Shock, on saying 'storm', haste and "
                     "chain lightning.\r\n");
    return TRUE;
  }

  if (!cmd && FIGHTING(ch) && !rand_number(0, 25))
  {
    vict = FIGHTING(ch);
    dam = 20 + dice(2, 8);
    act("\tbYour \tBsphere of lighting \tYglows bright\tb as electricity\r\n"
        "\tbc\tBr\tbackl\tBe\tbs \tball about its surface.\tn\r\n"
        "\tbSuddenly the \tYglow \tWintensifies\tb and a \tB\tfbolt of lightning\tn\r\n"
        "\tbshoots forth from the sphere band strikes $N \tbdead on!\tn",
        FALSE, ch, 0, vict, TO_CHAR);
    act("$n's \tBsphere of lighting \tYglows bright\tb as electricity\tn\r\n"
        "\tbc\tBr\tbackl\tBe\tbs \tball about its surface.\tn\r\n"
        "\tbSuddenly the \tYglow \tWintensifies\tb and a \tB\tfbolt of lightning\r\n"
        "\tbshoots forth from the sphere band strikes $N \tbdead on!\tn",
        FALSE, ch, 0, vict, TO_ROOM);
    damage(ch, vict, dam, -1, DAM_ELECTRIC, FALSE); // type -1 = no message
    return TRUE;
  }

  // haste/chain once a day on command
  if (cmd && argument && CMD_IS("say"))
  {
    if (!is_wearing(ch, 136100))
      return FALSE;

    skip_spaces(&argument);
    if (!strcmp(argument, "storm"))
    {
      if (GET_OBJ_SPECTIMER((struct obj_data *)me, 0) > 0)
      {
        send_to_char(ch, "Nothing happens.\r\n");
        return TRUE;
      }

      act("\tcAs you speak to your \tbsphere of lightning\tc, it begins to \tWglow\tc and fill "
          "with violent\tn\r\n"
          "\tcenergy.  The energy builds until it \tba\twrc\tbs and \tBc\tbrackle\tBs\tc all over "
          "the sphere before\tn\r\n"
          "\tcit lets loose in a violent \tblightning storm\tc.  As the energy from the storm "
          "begins\tn\r\n"
          "\tcto fade, a jolt of \tYelectricity\tc flows up through your arms, causing your heart "
          "to\tn\r\n"
          "\tcrace really fast!\tn",
          FALSE, ch, 0, 0, TO_CHAR);
      act("\tcAs $n \tcspeaks a word of power to $s \tbsphere of lightning\tc,\tn\r\n"
          "\tcit \tWglows brightly\tc and violent energy begins to fill it. The sphere\tn\r\n"
          "\tba\twrc\tbs\tc and \tBc\tbrackle\tBs\tc before it lets loose a violent "
          "\tblightning\tn\r\n"
          "\tbstorm\tc.  The energy begins to fade, but before this can happen a jolt of "
          "\tYelectricity\tn\r\n"
          "\tcflows up $n's\tc arms and causes $s heart to race really fast!",
          FALSE, ch, 0, 0, TO_ROOM);

      new_affect(&af);
      af.spell = SPELL_HASTE;
      af.duration = 100;
      SET_BIT_AR(af.bitvector, AFF_HASTE);
      affect_join(ch, &af, TRUE, FALSE, FALSE, FALSE);

      call_magic(ch, 0, 0, SPELL_CHAIN_LIGHTNING, 0, 20, CAST_WEAPON_SPELL);

      GET_OBJ_SPECTIMER((struct obj_data *)me, 0) = 24;
      return TRUE;
    }
  }
  return FALSE;
}

/* from homeland */
SPECIAL(bolthammer)
{
  int dam;

  if (!ch)
    return FALSE;

  if (!cmd && !strcmp(argument, "identify"))
  {
    send_to_char(ch, "Proc:  Lightning bolt.\r\n");
    return TRUE;
  }

  struct char_data *vict = FIGHTING(ch);

  if (cmd || !vict || rand_number(0, 18))
    return FALSE;

  dam = 25 + dice(1, 30);

  if (dam < GET_HIT(vict))
  {
    weapons_spells("\tLYour\tn $p \tLstarts to \twth\tWr\twob \tLviolently and\tn\r\n"
                   "\tLthe sound of th\tYun\tLder can be heard. Suddenly a bolt of "
                   "\tclig\tChtn\tcing \tLleaps\tn\r\n"
                   "\tLfrom the head of the warhammer and strikes\tn $N \tLwith full force.\tn",

                   "$n's $p \tLstarts to \twth\tWr\twob \tLviolently and the soundof th\tYun\tLder "
                   "can be heard.\tn\r\n"
                   "\tLSuddenly a bolt of \tclig\tChtn\tcing \tLleaps from the head of the "
                   "warhammer and strikes you\tn\r\n"
                   "\tlwith full force.\tn",

                   "$n's $p \tLstarts to \twth\tWr\twob \tLviolently and\tn\r\n"
                   "\tLthe sound of th\tYun\tLder can be heard. Suddenly a bolt of "
                   "\tclig\tChtn\tcing \tLleaps\tn\r\n"
                   "\tLfrom the head of the warhammer and strikes $N \tLwith full force.\tn",
                   ch, vict, (struct obj_data *)me, 0);
  }
  else
  {
    dam += 20;
    weapons_spells(
        "\tLYour\tn $p \tLstarts to \twth\tWr\twob \tLviolently and the sound of th\tYun\tLder can "
        "be\tn\r\n"
        "\tLheard. Suddenly a bolt of \tclig\tChtn\tcing \tLleaps from the head of the warhammer "
        "and strikes\tn\r\n"
        "$N \tLwith full force. When the flash is gone\r\n"

        "\tL you see the corpse of\tn $N \tLstill twitching on the ground.\tn",
        "$n's $p \tLstarts to \twth\tWr\twob \tLviolently and the soundof th\tYun\tLder can be "
        "heard.\tn\r\n"
        "\tLSuddenly a bolt of \tclig\tChtn\tcing \tLleaps from the head of the warhammer and "
        "strikes\tn\r\n"
        "\tLyou with full force. You twitch a few times before your body goes still forever.\tn",

        "$n's $p \tLstarts to \twth\tWr\twob \tLviolently and the soundof th\tYun\tLder can be "
        "heard.\tn\r\n"
        "\tLSuddenly a bolt of \tclig\tChtn\tcing \tLleaps from the head of the warhammer and "
        "strikes\tn \tn\r\n"
        "$N \tLwith full force. When the flash is gone you see\r\n"
        "\tLthe corpse of\tn $N \tLstill twitching on the ground.\tn",
        ch, vict, (struct obj_data *)me, 0);
  }

  damage(ch, vict, dam, -1, DAM_ELECTRIC, FALSE); // type -1 = no message
  return TRUE;
}

/* from jot i think */
SPECIAL(rughnark)
{
  if (!ch)
    return FALSE;

  if (!cmd && !strcmp(argument, "identify"))
  {
    send_to_char(ch, "Proc: magical damage 25+10d4 for high level monks.  Will work better for "
                     "non-good monks\r\n");
    return TRUE;
  }

  if (!is_wearing(ch, 114838))
    return FALSE;

  if (cmd)
    return FALSE;

  if (MONK_TYPE(ch) < 20 && !IS_NPC(ch))
    return FALSE;

  int dam = 0;
  struct char_data *vict = FIGHTING(ch);

  if (!vict)
    return FALSE;

  if (dice(1, 40) < 38)
    return FALSE;

  if (IS_GOOD(ch) && dice(1, 10) > 5)
    return FALSE;

  dam = 25 + dice(10, 4);

  if (dam > GET_HIT(vict))
    dam = GET_HIT(vict);

  weapons_spells(
      "\tLAs you make contact with your opponent, the twin \tWmithril\tL blades rip apart\tn\r\n"
      "\tLthe flesh in a gory display of blood, tearing huge chunks of meat out of your\tn\r\n"
      "\tLopponent as $E screams in agony and pain.\tn",

      "\tLAs $n make contact with you, the twin \tWmithril\tL blades rip apart\tn\r\n"
      "\tLyour flesh in a gory display of blood, tearing huge chunks of meat out of your\tn\r\n"
      "\tLown body as you scream in agony.\tn",

      "\tLAs $n makes contact with $s opponent, the twin \tWmithril\tL blades rip apart\tn\r\n"
      "\tLthe flesh in a gory display of blood, tearing huge chunks of meat out of $s\tn\r\n"
      "\tLopponent as $E screams in agony and pain.\tn",
      ch, vict, (struct obj_data *)me, 0);
  damage(ch, vict, dam, -1, DAM_SLICE, FALSE); // type -1 = no message
  return TRUE;
}

/* from the prisoner trove 132128 */
SPECIAL(speed_gaunts)
{
  if (!ch)
    return FALSE;

  if (!cmd && !strcmp(argument, "identify"))
  {
    send_to_char(
        ch, "Proc: Extra attacks & chance to do a powerful vamp attack (powerful monks only)\r\n");
    return TRUE;
  }

  if (!is_wearing(ch, 132128))
    return FALSE;

  if (cmd)
    return FALSE;

  if (MONK_TYPE(ch) < 20 && !IS_NPC(ch))
    return FALSE;

  int dam = 0, num_hits = 0, i = 0;
  struct char_data *vict = FIGHTING(ch);

  if (!vict)
    return FALSE;

  struct obj_data *gaunts = (struct obj_data *)me;

  /* odds to succeed on vamp proc */
  if (!rand_number(0, 37))
  {
    dam = rand_number(175, 300);

    weapons_spells("\tWSuddenly, $p\tW glows brightly as your body starts to \tCflicker\tW in and "
                   "out of reality!  "
                   "You \tCphase away\tW, dodging \tn$N's\tW attack and quickly \tCphase back\tW "
                   "\twslamming your "
                   "gauntlets powerfully\tW into $N where upon impact they flare with \tRvampiric "
                   "power\tW drawing "
                   "the lifeforce out of $M !\tn",

                   "\tWSuddenly, $p\tW glows brightly as $n's\tW body starts to \tCflicker\tW in "
                   "and out of reality!  "
                   "$n \tCphases away\tW, dodging your attack and quickly \tCphases back\tW "
                   "slamming $s gauntlets "
                   "powerfully into you where upon impact they flare with \tRvampiric power\tW "
                   "drawing the lifeforce "
                   "out of you!\tn",

                   "\tWSuddenly, $p\tW glows brightly as $n's\tW body starts to \tCflicker\tW in "
                   "and out of reality!  "
                   "$n \tCphases away\tW, dodging $N's\tW attack and quickly \tCphases back\tW "
                   "slamming $s gauntlets "
                   "powerfully into $N\tW where upon impact they flare with \tRvampiric power\tW "
                   "drawing the lifeforce "
                   "out of you!\tn",
                   ch, vict, gaunts, 0);
    damage(ch, vict, dam, -1, DAM_SLICE, FALSE); // type -1 = no message
    process_healing(ch, ch, -1, dam, 0, 0);

    return TRUE;
  }

  /* we failed vamp, lets try for the attack-blur proc */
  else if (!rand_number(0, 26))
  {
    act("$p\tn flares a \tGgreen\tn sheen before pulsing with \tWwhite light\tn as your strikes "
        "begin to speed up!",
        TRUE, ch, gaunts, vict, TO_CHAR);
    act("$p\tn flares a \tGgreen\tn sheen before pulsing with \tWwhite light\tn as $n's\tn strikes "
        "begin to speed up!",
        TRUE, ch, gaunts, vict, TO_VICT);
    act("$p\tn flares a \tGgreen\tn sheen before pulsing with \tWwhite light\tn as $n's\tn strikes "
        "begin to speed up!",
        TRUE, ch, gaunts, vict, TO_NOTVICT);

    num_hits = rand_number(4, 7);

    for (i = 0; i <= num_hits; i++)
    {
      if (valid_fight_cond(ch, TRUE))
        hit(ch, vict, TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, FALSE);
    }

    return TRUE;
  }

  return FALSE;
}

/* from homeland */
SPECIAL(magma)
{
  if (!ch)
    return FALSE;

  if (!cmd && !strcmp(argument, "identify"))
  {
    send_to_char(ch, "Proc: magmaburst (fire damage for monk.)\r\n");
    return TRUE;
  }

  int dam = 0;
  struct char_data *vict = 0;

  if (cmd)
    return FALSE;

  if (!FIGHTING(ch))
    return FALSE;

  if (dice(1, 40) < 39)
    return FALSE;

  if (MONK_TYPE(ch) < 20 && !IS_NPC(ch))
    return FALSE;

  vict = FIGHTING(ch);
  dam = 50 + dice(30, 5);
  if (dam > GET_HIT(vict))
    dam = GET_HIT(vict);
  if (dam < 50)
    return FALSE;
  weapons_spells(
      "\tLAs your hands \twimpact\tL with your opponent, the \trflowing m\tRagm\tra\tn\r\n"
      "\trignites\tL and emits a \twburst\tL of \tRf\tYi\tRr\tre\tL and \tyr\tLo\tyck\tL.\tn",
      "\tLAs $n\tL's hands \twimpact\tL with YOU, the \trflowing m\tRagm\tra\tn\r\n"
      "\trignites\tL and emits a \twburst\tL of \tRf\tYi\tRr\tre\tL and \tyr\tLo\tyck\tL.\tn",
      "\tLAs $n\tL's hands \twimpact\tL with $s opponent, the \trflowing m\tRagm\tra\tn\r\n"
      "\trignites\tL and emits a \twburst\tL of \tRf\tYi\tRr\tre\tL and \tyr\tLo\tyck\tL.\tn",
      ch, vict, (struct obj_data *)me, 0);
  damage(ch, vict, dam, -1, DAM_FIRE, FALSE); // type -1 = no message
  return TRUE;
}

/* from homeland */
SPECIAL(halberd)
{
  struct affected_type af;

  if (!ch)
    return FALSE;

  if (!cmd && !strcmp(argument, "identify"))
  {
    send_to_char(ch, "Proc:  blur, stun, slow.\r\n");
    return TRUE;
  }

  struct char_data *vict = FIGHTING(ch);

  if (cmd || !vict)
    return FALSE;

  switch (rand_number(0, 30))
  {
  case 27:
    if (!can_stun(vict))
      return FALSE;
    // A slight chance to stun
    weapons_spells(
        "\tcYour\tn $p \tcreverberates loudly as it sends\tn\r\n"
        "\tcforth a \tWthunderous blast \tcat \tn$N\tc.  $E is knocked backwards as the\tn\r\n"
        "\tcfull brunt of the blast hits $M squarely.\tn",
        "\tc$n\tn $p \tcreverberates loudly as it sends\tn\r\n"
        "\tcforth a \tWthunderous blast \tcat YOU.  You are knocked backwards as the\tn\r\n"
        "\tcfull brunt of the blast hits YOU squarely.\tn",
        "\tc$n\tn $p \tcreverberates loudly as it sends\tn\r\n"
        "\tcforth a \tWthunderous blast \tcat $N.  $E is knocked backwards as the\tn\r\n"
        "\tcfull brunt of the blast hits $M squarely.\tn",
        ch, vict, (struct obj_data *)me, 0);
    new_affect(&af);
    af.spell = SKILL_CHARGE;
    SET_BIT_AR(af.bitvector, AFF_STUN);
    af.duration = dice(1, 4);
    affect_join(vict, &af, TRUE, FALSE, FALSE, FALSE);
    return TRUE;
    break;

  case 21:
    // blur
    weapons_spells(
        "\tcAs your\tn $p \tctumultuously resonates, a strange \twm\tWi\twst \tcemanates "
        "from\tn\r\n"
        "\tcit quickly enshrouding you.  The \twm\tWi\twst \tcinduces you into a deep trance as "
        "your\tn\r\n"
        "\tctrance as your body melds with your\tn $p. \r\n\tcYou begin to \tCmove "
        "\tCat a \tcrapid\tCly in\tccreas\tCing sp\tceed \tCblurring out of focus.\tn\tn",

        "\tcAs $n\tn $p \tctumultuously resonates, a strange \twm\tWi\twst \tcemanates from\tn\r\n"
        "\tcit quickly enshrouding $m.  The \twm\tWi\twst \tcinduces $m into a deep\tn\r\n"
        "\tctrance as $m body melds with $s\tn $p.  \r\n\tc$e begins to \tCmove \tn"
        "\tCat a \tcrapid\tCly in\tccreas\tCing sp\tceed then $e \tCblurs out of focus.\tn",

        "\tcAs $n\tn $p \tctumultuously resonates, a strange \twm\tWi\twst \tcemanates from\tn\r\n"
        "\tcit quickly enshrouding $m.  The \twm\tWi\twst \tcinduces $m into a deep\tn\r\n"
        "\tctrance as $m body melds with $s\tn $p.  \r\n\tc$e begins to \tCmove \tn"
        "\tCat a \tcrapid\tCly in\tccreas\tCing sp\tceed then $e \tCblurs out of focus.\tn",
        ch, vict, (struct obj_data *)me, 0);
    return TRUE;
    break;

  case 17:
  case 16:
    // damage, and a chance to slow.
    weapons_spells(
        "\tcYour\tn $p \tcravenously slashes deep into \tn$N\tn\r\n"
        "\tcinflicting life-threatening wounds causing $M to convulse and \tRbleed profusely.\tn",
        "\tc$n\tn $p \tMravenously slashes deep into YOU inflicting\tn\r\n"
        "\tclife-threatening wounds causing YOU to convulse and \tRbleed profusely.\tn",
        "\tc$n\tn $p \tcravenously slashes deep into \tn$N\tc\r\n"
        "\tcinflicting life-threatening wounds causing $M to convulse and \tRbleed profusely.\tn",
        ch, vict, (struct obj_data *)me, SPELL_SLOW);
    damage(ch, vict, 10 + dice(2, 4), -1, DAM_POISON, FALSE); // type -1 = no message
    return TRUE;
    break;

  default:
    return FALSE;
  }
  return FALSE;
}

/*
   Portal that will jump to a player's clanhall
   Exit depends on which clan player belongs to
   Created by Jamdog - 4th July 2006
 */
SPECIAL(clanportal)
{
  int iPlayerClan = -1;
  struct obj_data *obj = (struct obj_data *)me;
  struct obj_data *port;
  zone_vnum z;
  room_vnum r;
  char obj_name[MAX_INPUT_LENGTH] = {'\0'};
  room_rnum was_in = IN_ROOM(ch);
  struct follow_type *k;

  if (!cmd && !strcmp(argument, "identify"))
  {
    send_to_char(ch, "This appears to be a clan portal.\r\n");
    return TRUE;
  }

  if (!CMD_IS("enter"))
    return FALSE;

  argument = one_argument_u(argument, obj_name);

  /* Check that the player is trying to enter THIS portal */
  if (!(port = get_obj_in_list_vis(ch, obj_name, NULL, world[(IN_ROOM(ch))].contents)))
  {
    return (FALSE);
  }

  if (port != obj)
    return (FALSE);

  iPlayerClan = GET_CLAN(ch);

  if (iPlayerClan == (int)NO_CLAN)
  {
    send_to_char(ch, "You try to enter the portal, but it returns you back to the same room!\n\r");
    return TRUE;
  }

  if ((z = get_clanhall_by_char(ch)) == NOWHERE)
  {
    send_to_char(ch, "Your clan does not have a clanhall!\n\r");
    log("Warning: Clan Portal - No clanhall (Player: %s, Clan ID: %d)", GET_NAME(ch), iPlayerClan);
    return TRUE;
  }

  //  r = (z * 100) + 1;    /* Get room xxx01 in zone xxx */
  /* for now lets have the exit room be 3000, until we get hometowns in, etc */
  r = 3000;

  if (!(real_room(r)))
  {
    send_to_char(ch, "Your clanhall is currently broken - contact an Imm!\n\r");
    log("Warning: Clan Portal failed (Player: %s, Clan ID: %d)", GET_NAME(ch), iPlayerClan);
    return TRUE;
  }

  /* First, move the player */
  if (!(House_can_enter(ch, r)))
  {
    send_to_char(ch, "That's private property -- no trespassing!\r\n");
    return TRUE;
  }

  act("$n enters $p, and vanishes!", FALSE, ch, port, 0, TO_ROOM);
  act("You enter $p, and you are transported elsewhere", FALSE, ch, port, 0, TO_CHAR);
  char_from_room(ch);

  if (ZONE_FLAGGED(GET_ROOM_ZONE(real_room(r)), ZONE_WILDERNESS))
  {
    X_LOC(ch) = world[real_room(r)].coords[0];
    Y_LOC(ch) = world[real_room(r)].coords[1];
  }

  char_to_room(ch, real_room(r));
  look_at_room(ch, 0);
  act("$n appears from thin air!", FALSE, ch, 0, 0, TO_ROOM);

  /* Then, any followers should auto-follow (Jamdog 19th June 2006) */
  for (k = ch->followers; k; k = k->next)
  {
    if ((IN_ROOM(k->follower) == was_in) && (GET_POS(k->follower) >= POS_STANDING))
    {
      act("You follow $N.\r\n", FALSE, k->follower, 0, ch, TO_CHAR);
      char_from_room(k->follower);

      if (ZONE_FLAGGED(GET_ROOM_ZONE(real_room(r)), ZONE_WILDERNESS))
      {
        X_LOC(k->follower) = world[real_room(r)].coords[0];
        Y_LOC(k->follower) = world[real_room(r)].coords[1];
      }

      char_to_room(k->follower, real_room(r));
      look_at_room(k->follower, 0);
      act("$n appears from thin air!", FALSE, k->follower, 0, 0, TO_ROOM);
    }
  }
  return TRUE;
}

static const struct spec_phrase_rule stability_boots_phrase = {"say", "whirlwind",
                                                               SPEC_PHRASE_SKIP_LEADING_SPACES};
static const struct spec_phrase_rule hellfire_phrase = {"say", "hellfire",
                                                        SPEC_PHRASE_SKIP_LEADING_SPACES};

/* re-written...  will now give fireshield/haste -zusuk */
SPECIAL(stability_boots)
{
  struct obj_data *obj = (struct obj_data *)me;
  struct spec_object_cooldown_state cooldown;
  char *normalized_argument;

  if (ch == NULL || argument == NULL)
    return FALSE;

  normalized_argument = argument;
  skip_spaces(&normalized_argument);
  if (!cmd && !strcmp(normalized_argument, "identify"))
  {
    send_to_char(ch, "Flurry of buffs by saying 'whirlwind'.\r\n");
    return TRUE;
  }

  if (!cmd)
    return FALSE;
  if (spec_context_validate_worn_object(ch, obj) != SPEC_CONTEXT_VALID)
    return FALSE;

  if (spec_phrase_match(CMD_NAME, argument, &stability_boots_phrase) == SPEC_PHRASE_MATCHED)
  {
    cooldown = spec_object_cooldown_read(obj, 0);
    if (cooldown.status == SPEC_OBJECT_COOLDOWN_ACTIVE)
    {
      send_to_char(ch, "Nothing happens (recharge in %d hours).\r\n", cooldown.remaining_mud_hours);
      return TRUE;
    }
    if (cooldown.status != SPEC_OBJECT_COOLDOWN_READY)
      return FALSE;

    act("\twSmall eddies of wind begin to form around the edges of the area, \tn\r\n"
        "\twswirling about in tiny patterns focused at $p.  Gradually, the wind picks \tn\r\n"
        "\twup in its intensity, lifting up from the ground to build into a howling whirlwind "
        "\tn\r\n"
        "\twbefore the eddies are drawn inward around your body.\tn\r\n",
        FALSE, ch, (struct obj_data *)me, 0, TO_CHAR);
    act("\tw$n utters a word towards $p...   Small eddies of wind begin to form around the edges "
        "of the area, \tn\r\n"
        "\twswirling about in tiny patterns focused at $p.  Gradually, the wind picks \tn\r\n"
        "\twup in its intensity, lifting up from the ground to build into a howling whirlwind "
        "\tn\r\n"
        "\twbefore the eddies are drawn inward around your body.\tn\r\n",
        FALSE, ch, (struct obj_data *)me, 0, TO_ROOM);

    call_magic(ch, ch, 0, SPELL_FIRE_SHIELD, 0, 30, CAST_WEAPON_SPELL);
    call_magic(ch, ch, 0, SPELL_HASTE, 0, 30, CAST_WEAPON_SPELL);
    call_magic(ch, ch, 0, SPELL_SHADOW_SHIELD, 0, 30, CAST_WEAPON_SPELL);

    (void)spec_object_cooldown_commit(obj, 0, 12);
    return TRUE;
  }
  return FALSE;
}

/* re-written...  will now give fireshield/haste -zusuk */
SPECIAL(hellfire)
{
  struct obj_data *obj = (struct obj_data *)me;
  struct spec_object_cooldown_state cooldown;
  char *normalized_argument;

  if (ch == NULL || argument == NULL)
    return FALSE;

  normalized_argument = argument;
  skip_spaces(&normalized_argument);
  if (!cmd && !strcmp(normalized_argument, "identify"))
  {
    send_to_char(ch, "Invoke haste and fireshield on armor by saying 'Hellfire'.\r\n");
    return TRUE;
  }

  if (!cmd)
    return FALSE;
  if (spec_context_validate_worn_object(ch, obj) != SPEC_CONTEXT_VALID)
    return FALSE;

  if (spec_phrase_match(CMD_NAME, argument, &hellfire_phrase) == SPEC_PHRASE_MATCHED)
  {
    cooldown = spec_object_cooldown_read(obj, 0);
    if (cooldown.status == SPEC_OBJECT_COOLDOWN_ACTIVE)
    {
      send_to_char(ch, "Nothing happens (recharge in %d hours).\r\n", cooldown.remaining_mud_hours);
      return TRUE;
    }
    if (cooldown.status != SPEC_OBJECT_COOLDOWN_READY)
      return FALSE;

    act("\tLThe pure flames of your $p\tL is invoked.\tn\r\n"
        "\tLThe flames rise and protects YOU!\tn\r\n",
        FALSE, ch, (struct obj_data *)me, 0, TO_CHAR);

    act("\tLThe pure flames of $n\tL's $p\tL is invoked.\tn\r\n"
        "\tLThe flames rise and protects $m!\tn\r\n",
        FALSE, ch, (struct obj_data *)me, 0, TO_ROOM);

    call_magic(ch, ch, 0, SPELL_FIRE_SHIELD, 0, 26, CAST_WEAPON_SPELL);
    call_magic(ch, ch, 0, SPELL_HASTE, 0, 26, CAST_WEAPON_SPELL);

    (void)spec_object_cooldown_commit(obj, 0, 12);
    return TRUE;
  }
  return FALSE;
}

/* from homeland */
SPECIAL(angel_leggings)
{
  if (DEBUGMODE)
    send_to_char(ch, "Debug - Mark 1\r\n");

  skip_spaces(&argument);

  if (!ch)
    return FALSE;

  if (DEBUGMODE)
    send_to_char(ch, "Debug - Mark 2\r\n");

  if (!cmd && !strcmp(argument, "identify"))
  {
    send_to_char(ch, "Invoke fly by keyword 'Elysium'.\r\n");
    return TRUE;
  }

  if (DEBUGMODE)
    send_to_char(ch, "Debug - Mark 3\r\n");

  if (!cmd)
    return FALSE;

  if (DEBUGMODE)
    send_to_char(ch, "Debug - Mark 4\r\n");

  if (!argument)
    return FALSE;

  if (DEBUGMODE)
    send_to_char(ch, "Debug - Mark 5\r\n");

  if (!is_wearing(ch, 106021))
    return FALSE;

  if (DEBUGMODE)
    send_to_char(ch, "Debug 7 - Argument: %s\r\n", argument);

  skip_spaces(&argument);

  if (cmd && CMD_IS("say") && !strcmp(argument, "elysium"))
  {
    if (GET_OBJ_SPECTIMER((struct obj_data *)me, 0) > 0)
    {
      if (DEBUGMODE)
        send_to_char(ch, "Debug - Mark 8\r\n");

      send_to_char(ch, "Nothing happens.\r\n");
      return TRUE;
    }

    act("\tWThe power of $p\tW is invoked.\tn\r\n"
        "\tcYour feet slowly raise off the ground.\tn\r\n",
        FALSE, ch, (struct obj_data *)me, 0, TO_CHAR);
    act("\tWThe power of $n\tW's $p\tW is invoked.\tn\r\n"
        "\tw$s feet slowly raise of the ground!\tn\r\n",
        FALSE, ch, (struct obj_data *)me, 0, TO_ROOM);

    if (DEBUGMODE)
      send_to_char(ch, "Debug - Mark 9\r\n");

    call_magic(ch, ch, 0, SPELL_FLY, 0, 30, CAST_WEAPON_SPELL);

    GET_OBJ_SPECTIMER((struct obj_data *)me, 0) = 48;

    return TRUE;
  }
  return FALSE;
}

/* zusuk's epic robes from cloud realms */
SPECIAL(dragon_robes)
{
  if (DEBUGMODE)
    send_to_char(ch, "Debug - Mark 1\r\n");

  skip_spaces(&argument);

  if (!ch)
    return FALSE;

  if (DEBUGMODE)
    send_to_char(ch, "Debug - Mark 2\r\n");

  if (!cmd && !strcmp(argument, "identify"))
  {
    send_to_char(ch, "Invoke displace by keyword 'Power'.\r\n");
    return TRUE;
  }

  if (DEBUGMODE)
    send_to_char(ch, "Debug - Mark 3\r\n");

  if (!cmd)
    return FALSE;

  if (DEBUGMODE)
    send_to_char(ch, "Debug - Mark 4\r\n");

  if (!argument)
    return FALSE;

  if (DEBUGMODE)
    send_to_char(ch, "Debug - Mark 5\r\n");

  /*[144669]*/
  if (!is_wearing(ch, 144669))
    return FALSE;

  if (DEBUGMODE)
    send_to_char(ch, "Debug 7 - Argument: %s\r\n", argument);

  skip_spaces(&argument);

  if (cmd && CMD_IS("say") && !strcmp(argument, "power"))
  {
    if (GET_OBJ_SPECTIMER((struct obj_data *)me, 0) > 0)
    {
      if (DEBUGMODE)
        send_to_char(ch, "Debug - Mark 8\r\n");

      send_to_char(ch, "Nothing happens.\r\n");
      return TRUE;
    }

    act("\tWThe power of $p\tW is invoked.\tn\r\n"
        "\tcYour form begins to shimmer in and out of reality!\tn\r\n",
        FALSE, ch, (struct obj_data *)me, 0, TO_CHAR);
    act("\tWThe power of $n\tW's $p\tW is invoked.\tn\r\n"
        "\tw$s begins to shimmer in and out of reality!\tn\r\n",
        FALSE, ch, (struct obj_data *)me, 0, TO_ROOM);

    if (DEBUGMODE)
      send_to_char(ch, "Debug - Mark 9\r\n");

    call_magic(ch, ch, 0, SPELL_DISPLACEMENT, 0, 30, CAST_WEAPON_SPELL);

    GET_OBJ_SPECTIMER((struct obj_data *)me, 0) = 48;

    return TRUE;
  }
  return FALSE;
}

/* from homeland, i doubt we are going to port this, houses replace these */
/*
SPECIAL(storage_chest) {
  if (cmd)
    return FALSE;
  if (!cmd && !strcmp(argument, "identify"))
  {
    send_to_char(ch, "This is a player storage chest.\r\n");
    return TRUE;
  }

  struct obj_data *chest = (struct obj_data*) me;
  if (GET_OBJ_VAL(chest, 3) != 0)
    return FALSE;
  ch = chest->carried_by;
  if (!ch) {
    REMOVE_BIT(GET_OBJ_EXTRA(chest), ITEM_INVISIBLE);
    return FALSE;
  }
  if (IS_NPC(ch) || IS_PET(ch))
    return FALSE;

  snprintf(buf2, sizeof(buf2), "chest storage %s", GET_NAME(ch));
  chest->name = str_dup(buf2);

  if (GET_OBJ_VNUM(chest) == 1291) {
    snprintf(buf2, sizeof(buf2), "\tLAn ornate \tcmithril\tL chest owned by \tw%s\tL rests here.\tn", GET_NAME(ch));
    chest->description = str_dup(buf2);
    snprintf(buf2, sizeof(buf2), "\tLan ornate \tcmithril\tL chest owned by \tw%s\tn", GET_NAME(ch));
    chest->short_description = str_dup(buf2);

  } else {
    snprintf(buf2, sizeof(buf2), "\tLA storage chest owned by \tW%s\tL is standing here.\tn", GET_NAME(ch));
    chest->description = str_dup(buf2);
    snprintf(buf2, sizeof(buf2), "\tLa chest owned by \tW%s\tn", GET_NAME(ch));
    chest->short_description = str_dup(buf2);
  }
  GET_OBJ_VAL(chest, 3) = -GET_IDNUM(ch);
  SET_BIT(GET_OBJ_SAVED(chest), SAVE_OBJ_VALUES);
  SET_BIT(GET_OBJ_SAVED(chest), SAVE_OBJ_NAME);
  SET_BIT(GET_OBJ_SAVED(chest), SAVE_OBJ_DESC);
  SET_BIT(GET_OBJ_SAVED(chest), SAVE_OBJ_SHORT);
  SET_BIT(GET_OBJ_SAVED(chest), SAVE_OBJ_TYPE);
  SET_BIT(GET_OBJ_SAVED(chest), SAVE_OBJ_WEAR);
  SET_BIT(GET_OBJ_SAVED(chest), SAVE_OBJ_EXTRA);
  SET_BIT(GET_OBJ_SAVED(chest), SAVE_OBJ_TIMER);
  SET_BIT(GET_OBJ_SAVED(chest), SAVE_OBJ_WEIGHT);
  SET_BIT(GET_OBJ_SAVED(chest), SAVE_OBJ_COST);
  save_chests();
  return TRUE;
}
 */

/* from homeland */
SPECIAL(clang_bracer)
{
  if (!cmd && !strcmp(argument, "identify"))
  {
    send_to_char(ch, "Dwarf and Group Only.  Invoke battle prowess by saying 'argenoth'.\r\n");
    return TRUE;
  }

  if (ch && is_wearing(ch, 121456))
  {
    if (!cmd && GET_RACE(ch) != RACE_DWARF)
    {
      act("\tLThe bracer begins to glow on your arm, clenching tighter and "
          "tighter until you rip it off in agony.\tn",
          FALSE, ch, 0, 0, TO_CHAR);
      act("\tLA bracer on $n\tL's arm begins to glow brightly and a look of "
          "intense pain crosses $s face as $e rips the bracer free.\tn",
          FALSE, ch, 0, 0, TO_ROOM);

      if (GET_EQ(ch, WEAR_WRIST_R) == (obj_data *)me)
        obj_to_char(unequip_char(ch, WEAR_WRIST_R), ch);
      else if (GET_EQ(ch, WEAR_WRIST_L) == (obj_data *)me)
        obj_to_char(unequip_char(ch, WEAR_WRIST_L), ch);
      return TRUE;
    }

    // invoke it!
    if (!cmd)
      return FALSE;
    if (!argument)
      return FALSE;
    if (!CMD_IS("say"))
      return FALSE;
    skip_spaces(&argument);

    if (!strcmp(argument, "argenoth"))
    {
      if (GET_OBJ_SPECTIMER((struct obj_data *)me, 0) > 0)
      {
        send_to_char(ch, "You attempt to invoke your bracer, but nothing happens.\r\n");
        return TRUE;
      }

      struct group_data *group;

      if ((group = GROUP(ch)) == NULL)
      {
        send_to_char(ch, "You recall from lore this item will not work unless in a group.\r\n");
        return FALSE;
      }

      /* success! */
      send_to_group(NULL, group,
                    "The memories of ancient battles fills your mind, each "
                    "blow clear as if it were yesterday.  You feel your muscles tighten "
                    "then relax as the skill of ancient warriors is merged with your own.\r\n");
      call_magic(ch, ch, 0, SPELL_MASS_ENHANCE, 0, 30, CAST_WEAPON_SPELL);
      GET_OBJ_SPECTIMER((struct obj_data *)me, 0) = 24;
      return TRUE;
    }
  }
  return FALSE;
}

/* from homeland */
SPECIAL(menzo_chokers)
{
  struct affected_type *af2;
  struct affected_type af;

  if (!ch)
    return FALSE;

  if (!cmd && !strcmp(argument, "identify"))
  {
    send_to_char(ch, "For Drow, finding pair will give +1 to hitroll.\r\n");
    return TRUE;
  }

  for (af2 = ch->affected; af2; af2 = af2->next)
  {
    if (af2->spell == AFF_MENZOCHOKER)
    {
      if (!is_wearing(ch, 135626) || !is_wearing(ch, 135627))
      {
        send_to_char(ch, "\tLYou suddenly feel bereft of your \tmgoddess's\tL"
                         " touch.\tn\r\n");
        affect_from_char(ch, AFF_MENZOCHOKER);
      }
      return FALSE;
    }
  }

  if (is_wearing(ch, 135626) && is_wearing(ch, 135627))
  {
    if (GET_RACE(ch) == RACE_DROW)
    {
      send_to_char(ch, "\tLYour blood quickens, as if your soul has been touched "
                       "by a higher power.\tn\r\n");
      af.location = APPLY_HITROLL;
      af.duration = 5;
      af.modifier = 1;
      SET_BIT_AR(af.bitvector, AFF_MENZOCHOKER);
      affect_join(ch, &af, FALSE, FALSE, TRUE, FALSE);
      return FALSE;
    }
  }
  return FALSE;
}

#undef DEBUGMODE
