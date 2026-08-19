/**************************************************************************
 *  File: spec/spec_zone_jot.c                         Part of LuminariMUD *
 *  Usage: Jot invasion, encounter, and object procedures.                *
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
#include "constants.h"
#include "magic/spells.h"
#include "act.h"
#include "spec_objects.h"
#include "spec_zone_jot.h"
#include "combat/fight.h"
#include "actions.h"
#include "magic/domains_schools.h"
#include "character/evolutions.h"

/*****************/
/* Jot           */
/*****************/

#define JOT_VNUM 1960

#define MAX_FG 60    // fire giants
#define MAX_SB 20    // smoking beard batallion
#define MAX_EM 20    // efreeti mercenaries
#define MAX_FROST 65 // frost giants

bool jot_inv_check = false;

ACMD_DECL(do_say);

/* just made this to help facilitate switching of zone vnums if needed */
int jot_converter(int value)
{
  return (JOT_VNUM * 100) + value;
}

/* currently unused */
void jot_invasion()
{
  if (jot_inv_check)
    return;

  jot_inv_check = true;

  if (rand_number(0, 99) <= 2)
    return;
}

/* load rooms for fire giants */
int fg_pos[MAX_FG] = {295, 295, 295, 295, 295, 295, 295, 295, 295, 295, 295, 295, 295, 295, 295,
                      295, 295, 295, 295, 295, 295, 295, 295, 295, 295, 295, 295, 295, 295, 295,
                      295, 295, 295, 295, 295, 295, 295, 295, 295, 295, 215, 215, 215, 215, 215,
                      212, 218, 222, 207, 188, 204, 204, 204, 204, 196, 204, 204, 204, 204, 196};

/* load rooms for smoking beard batallion */
int sb_pos[MAX_SB] = {295, 295, 295, 295, 295, 295, 295, 295, 295, 295,
                      295, 295, 295, 215, 215, 188, 188, 217, 206, 206};

/* load rooms for frost giants */
int frost_pos[MAX_FROST] = {286, 286, 282, 283, 284, 285, 285, 285, 286, 286, 273, 273, 270,
                            270, 269, 273, 273, 270, 270, 269, 266, 266, 267, 264, 264, 266,
                            266, 267, 264, 264, 265, 272, 272, 271, 271, 228, 240, 240, 233,
                            233, 233, 235, 235, 235, 251, 251, 252, 252, 253, 253, 251, 252,
                            252, 253, 253, 244, 244, 255, 255, 254, 254, 256, 256, 243, 243};

/* spec proc for loading the jot invasion */
SPECIAL(jot_invasion_loader)
{
  struct char_data *tch = NULL, *chmove = NULL, *glammad = NULL, *leader = NULL, *mob = NULL;
  int i = 0;
  int where = -1;
  struct obj_data *obj = NULL, *obj2 = NULL;
  obj_rnum objrnum = NOTHING;
  room_rnum roomrnum = NOWHERE;
  mob_rnum mobrnum = NOWHERE;

  if (cmd || PROC_FIRED(ch) == TRUE)
    return 0;

  /* moving these special mobiles from their storage room to jot */
  for (tch = world[ch->in_room].people; tch; tch = chmove)
  {
    chmove = tch->next_in_room;
    /* glammad */
    if (GET_MOB_VNUM(tch) == (mob_vnum)jot_converter(80))
    {
      if ((roomrnum = real_room(jot_converter(204))) != NOWHERE)
      {
        glammad = tch; /* going to use this to form a group */
        char_from_room(glammad);
        char_to_room(glammad, roomrnum);
        if (!GROUP(glammad))
          create_group(glammad);
      }
    }
    /* fire giant captain(s) */
    if (GET_MOB_VNUM(tch) == (mob_vnum)jot_converter(81))
    {
      if ((roomrnum = real_room(jot_converter(204))) != NOWHERE)
      {
        char_from_room(tch);
        char_to_room(tch, roomrnum);
      }
    }
    /* sirthon quilen */
    if (GET_MOB_VNUM(tch) == (mob_vnum)jot_converter(83))
    {
      if ((roomrnum = real_room(jot_converter(115))) != NOWHERE)
      {
        char_from_room(tch);
        char_to_room(tch, roomrnum);
      }
    }
  }

  /* soldiers to glammad.  Resolve the destination before creating anything so a
   * missing room cannot strand a freshly loaded mobile outside the world. */
  if ((roomrnum = real_room(jot_converter(204))) != NOWHERE)
  {
    for (i = 0; i < 2; i++)
    {
      if ((mob = read_mobile(jot_converter(78), VIRTUAL)) == NULL)
        continue;

      char_to_room(mob, roomrnum);
      if ((obj = read_object(jot_converter(17), VIRTUAL)) != NULL)
      {
        obj_to_char(obj, mob);
        perform_wield(mob, obj, TRUE);
      }
      SET_BIT_AR(MOB_FLAGS(mob), MOB_SENTINEL);
      REMOVE_BIT_AR(MOB_FLAGS(mob), MOB_LISTEN);
      if (glammad)
      {
        add_follower(mob, glammad);
        if (!GROUP(mob))
          join_group(mob, GROUP(glammad));
      }
    }
  }

  /* twilight to treasure room */
  if ((objrnum = real_object(jot_converter(90))) != NOWHERE)
  {
    if ((obj = read_object(objrnum, REAL)) != NULL)
    {
      if ((roomrnum = real_room(jot_converter(296))) != NOWHERE)
      {
        obj_to_room(obj, roomrnum);
      }
    }
  }
  /* fire giant crown to treasure room */
  if ((objrnum = real_object(jot_converter(82))) != NOWHERE)
  {
    if ((obj = read_object(objrnum, REAL)) != NULL)
    {
      if ((roomrnum = real_room(jot_converter(296))) != NOWHERE)
      {
        obj_to_room(obj, roomrnum);
      }
    }
  }

  /* extra jarls to deal with */
  for (i = 0; i < 2; i++)
  { /* treasure room */
    if ((mobrnum = real_mobile(jot_converter(39))) != NOBODY)
    {
      if ((mob = read_mobile(mobrnum, REAL)) != NULL)
      {
        if ((roomrnum = real_room(jot_converter(296))) != NOWHERE)
        {
          char_to_room(mob, roomrnum);
        }
      }
    }
  }
  for (i = 0; i < 3; i++)
  { /* uthgard loki throne room */
    if ((mobrnum = real_mobile(jot_converter(39))) != NOBODY)
    {
      if ((mob = read_mobile(mobrnum, REAL)) != NULL)
      {
        if ((roomrnum = real_room(jot_converter(287))) != NOWHERE)
        {
          char_to_room(mob, roomrnum);
        }
      }
    }
  }

  /* heavily guarded gatehouse, frost giant mage is leading this group */
  if ((mobrnum = real_mobile(jot_converter(90))) != NOBODY)
  {
    if ((leader = read_mobile(mobrnum, REAL)) != NULL)
    {
      if ((roomrnum = real_room(jot_converter(266))) != NOWHERE)
      {
        char_to_room(leader, roomrnum);
        if (!GROUP(leader))
          create_group(leader);
      }
    }
  }
  /* 2nd mage in group */
  if ((mobrnum = real_mobile(jot_converter(90))) != NOBODY)
  {
    if ((mob = read_mobile(mobrnum, REAL)) != NULL)
    {
      if ((roomrnum = real_room(jot_converter(266))) != NOWHERE)
      {
        char_to_room(mob, roomrnum);
        if (leader)
        {
          add_follower(mob, leader);
          if (!GROUP(mob))
            join_group(mob, GROUP(leader));
        }
      }
    }
  }
  /* citadel guards join the group */
  if ((roomrnum = real_room(jot_converter(266))) != NOWHERE)
  {
    for (i = 0; i < 8; i++)
    {
      if ((mob = read_mobile(jot_converter(33), VIRTUAL)) == NULL)
        continue;

      char_to_room(mob, roomrnum);
      if ((obj = read_object(jot_converter(28), VIRTUAL)) != NULL)
      {
        obj_to_char(obj, mob);
        perform_wield(mob, obj, TRUE);
      }
      if ((obj2 = read_object(jot_converter(41), VIRTUAL)) != NULL)
      {
        obj_to_char(obj2, mob);
        where = find_eq_pos(mob, obj2, 0);
        perform_wear(mob, obj2, where);
      }
      if (leader)
      {
        add_follower(mob, leader);
        if (!GROUP(mob))
          join_group(mob, GROUP(leader));
      }
    }
  }

  /* large gatehouse group, led by a mage again */
  if ((mobrnum = real_mobile(jot_converter(90))) != NOBODY)
  {
    if ((leader = read_mobile(mobrnum, REAL)) != NULL)
    {
      if ((roomrnum = real_room(jot_converter(252))) != NOWHERE)
      {
        char_to_room(leader, roomrnum);
        if (!GROUP(leader))
          create_group(leader);
      }
    }
  }
  /* 2nd mage in group */
  if ((mobrnum = real_mobile(jot_converter(90))) != NOBODY)
  {
    if ((mob = read_mobile(mobrnum, REAL)) != NULL)
    {
      if ((roomrnum = real_room(jot_converter(252))) != NOWHERE)
      {
        char_to_room(mob, roomrnum);
        if (leader)
        {
          add_follower(mob, leader);
          if (!GROUP(mob))
            join_group(mob, GROUP(leader));
        }
      }
    }
  }
  /* citadel guards join the group */
  if ((roomrnum = real_room(jot_converter(252))) != NOWHERE)
  {
    for (i = 0; i < 5; i++)
    {
      if ((mob = read_mobile(jot_converter(33), VIRTUAL)) == NULL)
        continue;

      char_to_room(mob, roomrnum);
      if ((obj = read_object(jot_converter(28), VIRTUAL)) != NULL)
      {
        obj_to_char(obj, mob);
        perform_wield(mob, obj, TRUE);
      }
      if ((obj2 = read_object(jot_converter(40), VIRTUAL)) != NULL)
      {
        obj_to_char(obj2, mob);
        where = find_eq_pos(mob, obj2, 0);
        perform_wear(mob, obj2, where);
      }
      if (leader)
      {
        add_follower(mob, leader);
        if (!GROUP(mob))
          join_group(mob, GROUP(leader));
      }
    }
  }

  /* load up some firegiants, then equip them */
  for (i = 0; i < MAX_FG; i++)
  {
    if ((roomrnum = real_room(jot_converter(fg_pos[i]))) != NOWHERE)
    {
      if ((mob = read_mobile(jot_converter(78), VIRTUAL)) != NULL)
      {
        char_to_room(mob, roomrnum);
        if ((obj = read_object(jot_converter(17), VIRTUAL)) != NULL)
        {
          obj_to_char(obj, mob);
          perform_wield(mob, obj, TRUE);
        }
      }
    }
  }

  /* load up smoking beard batallion */
  for (i = 0; i < MAX_SB; i++)
  {
    if ((roomrnum = real_room(jot_converter(sb_pos[i]))) != NOWHERE)
    {
      if ((mob = read_mobile(jot_converter(79), VIRTUAL)) != NULL)
      {
        char_to_room(mob, roomrnum);
      }
    }
  }

  /* efreeti mercenary */
  for (i = 0; i < MAX_EM; i++)
  {
    if ((roomrnum = real_room(jot_converter(295))) != NOWHERE)
    {
      if ((mob = read_mobile(jot_converter(84), VIRTUAL)) != NULL)
      {
        char_to_room(mob, roomrnum);
      }
    }
  }

  /* Extra frost giants */
  for (i = 0; i < MAX_FROST; i++)
  {
    if ((roomrnum = real_room(jot_converter(frost_pos[i]))) != NOWHERE)
    {
      if ((mob = read_mobile(jot_converter(85), VIRTUAL)) != NULL)
      {
        char_to_room(mob, roomrnum);
        if ((obj = read_object(jot_converter(28), VIRTUAL)) != NULL)
        {
          obj_to_char(obj, mob);
          perform_wield(mob, obj, TRUE);
        }
      }
    }
  }

  /* Valkyrie */
  if ((roomrnum = real_room(jot_converter(4))) != NOWHERE)
  {
    if ((mob = read_mobile(jot_converter(82), VIRTUAL)) != NULL)
    {
      char_to_room(mob, roomrnum);
    }
  }

  /* Remove Brunnhilde */
  for (mob = character_list; mob; mob = mob->next)
    if (GET_MOB_VNUM(mob) == (mob_vnum)jot_converter(68))
      extract_char(mob);

  PROC_FIRED(ch) = TRUE;
  return 1;
}

/* thrym jot fight spec */
SPECIAL(thrym)
{
  if (!ch)
    return 0;

  struct char_data *vict = FIGHTING(ch);
  struct affected_type af;
  int bonus = 0;

  if (cmd || !vict || rand_number(0, 8))
    return 0;

  if (paralysis_immunity(vict))
  {
    send_to_char(ch, "Your target is unfazed.\r\n");
    return 1;
  }

  if (HAS_EVOLUTION(vict, EVOLUTION_UNDEAD_APPEARANCE))
    bonus += get_evolution_appearance_save_bonus(vict);

  // no save, unless have special feat
  if (HAS_FEAT(vict, FEAT_PARALYSIS_RESIST) ||
      savingthrow(ch, vict, SAVING_FORT, 4 + bonus, CAST_INNATE, 30, ENCHANTMENT))
  {
    send_to_char(ch, "Your target is unfazed.\r\n");
    return 1;
  }

  act("\tCThrym touches you with a chilling hand, freezing you in place.\tn", FALSE, vict, 0, ch,
      TO_CHAR);
  act("\tCThrym touches $n\tC, freezing $m in place.\tn", FALSE, vict, 0, ch, TO_ROOM);

  new_affect(&af);
  af.spell = SPELL_HOLD_PERSON;
  SET_BIT_AR(af.bitvector, AFF_PARALYZED);
  af.duration = 8;
  affect_join(vict, &af, TRUE, FALSE, FALSE, FALSE);

  return 1;
}

SPECIAL(ymir)
{
  if (!ch || cmd)
    return 0;

  if (FIGHTING(ch) && !rand_number(0, 4))
  {
    call_magic(ch, FIGHTING(ch), 0, SPELL_FROST_BREATHE, 0, GET_LEVEL(ch), CAST_INNATE);
    return 1;
  }

  return 0;
}

SPECIAL(planetar)
{
  if (!ch || cmd)
    return 0;

  if (FIGHTING(ch) && !rand_number(0, 5))
  {
    call_magic(ch, FIGHTING(ch), 0, SPELL_LIGHTNING_BREATHE, 0, GET_LEVEL(ch), CAST_INNATE);
    return 1;
  }

  return 0;
}

SPECIAL(gatehouse_guard)
{
  struct char_data *mob = (struct char_data *)me;

  if (!IS_MOVE(cmd) || AFF_FLAGGED(mob, AFF_BLIND) || AFF_FLAGGED(mob, AFF_SLEEP) ||
      AFF_FLAGGED(mob, AFF_PARALYZED) || AFF_FLAGGED(mob, AFF_GRAPPLED) ||
      AFF_FLAGGED(mob, AFF_ENTANGLED) || HAS_WAIT(mob))
    return FALSE;

  if (cmd == SCMD_EAST && (!IS_NPC(ch) || IS_PET(ch)) && GET_LEVEL(ch) < 31)
  {
    act("$N \twblocks your way!\tn\r\n", FALSE, ch, 0, mob, TO_CHAR);
    act("$N \twblocks $n's\tw way!\tn\r\n", FALSE, ch, 0, mob, TO_ROOM);
    return TRUE;
  }

  return 0;
}

/************************************************/
/* end mobile specs, start object specs for jot */
/************************************************/

/* special cloak object proc */
SPECIAL(ymir_cloak)
{
  if (!ch)
    return 0;

  if (!cmd && !strcmp(argument, "identify"))
  {
    send_to_char(ch, "Invoke ice storm by saying 'icicle storm'.\r\nOnce per day.\r\n");
    return 1;
  }

  struct obj_data *obj = (struct obj_data *)me;

  if (cmd && argument && CMD_IS("say"))
  {
    if (!is_wearing(ch, jot_converter(59)))
      return 0;

    skip_spaces(&argument);

    if (!strcmp(argument, "icicle storm"))
    {
      if (GET_OBJ_SPECTIMER(obj, 0) > 0)
      {
        send_to_char(ch, "\tcAs you say '\tCicicle storm\tc' to your \tWa cloak of glittering "
                         "icicles\tc, nothing happens.\tn\r\n");
        return 1;
      }

      weapons_spells("\tBAs you say '\twicicle storm\tB' to $p \tBit flashes bright blue and sends "
                     "forth a storm of razor sharp icicles in all directions.\tn",
                     "\tBAs $n \tBmutters something under his breath  to $p \tBit flashes bright "
                     "blue and sends forth a storm of razor sharp icicles in all directions.\tn",
                     "\tBAs $n \tBmutters something under his breath  to $p \tBit flashes bright "
                     "blue and sends forth a storm of razor sharp icicles in all directions.\tn",
                     ch, 0, (struct obj_data *)me, SPELL_ICE_STORM);
      GET_OBJ_SPECTIMER(obj, 0) = 6;
      return 1;
    }
  }
  return 0;
}

/* mistweave mace object proc */
SPECIAL(mistweave)
{
  if (!ch)
    return 0;

  if (!cmd && !strcmp(argument, "identify"))
  {
    send_to_char(ch, "Invoke blindness by saying 'mistweave'. Once per day.\r\n");
    return 1;
  }

  struct obj_data *obj = (struct obj_data *)me;
  struct char_data *vict = FIGHTING(ch);

  if (cmd && argument && CMD_IS("say"))
  {
    if (!is_wearing(ch, jot_converter(12)))
      return 0;

    skip_spaces(&argument);

    if (FIGHTING(ch) && (FIGHTING(ch)->in_room == ch->in_room) && !strcmp(argument, "mistweave"))
    {
      if (GET_OBJ_SPECTIMER(obj, 0) > 0)
      {
        send_to_char(ch, "\tpAs you say '\twmistweave\tp' to your a huge adamantium mace "
                         "enshrouded with \tWmist\tp, nothing happens.\tn\r\n");
        return 1;
      }
      act("\tLAs you say, '\tnmistweave\tL', "
          "\tLa thick vapor issues forth from $p\tL, "
          "\tLenshrouding the eyes of $N\tL.\tn",
          FALSE, ch, obj, vict, TO_CHAR);
      act("\tLAs $n \tLmutters something under his breath, "
          "\tLa thick vapor issues forth from $p\tL, "
          "\tLenshrouding the eyes of $N.",
          FALSE, ch, obj, vict, TO_ROOM);

      call_magic(ch, FIGHTING(ch), 0, SPELL_BLINDNESS, 0, 30, CAST_WEAPON_SPELL);
      GET_OBJ_SPECTIMER(obj, 0) = 24;
      return 1;
    }
    else
      return 0;
  }
  return 0;
}

/* frostbite axe proc */
SPECIAL(frostbite)
{
  if (!ch)
    return 0;

  if (!cmd && !strcmp(argument, "identify"))
  {
    send_to_char(ch, "Invoke cone of cold  by saying 'frostbite'. Once per day.\r\n");
    return 1;
  }

  struct obj_data *obj = (struct obj_data *)me;
  struct char_data *vict = FIGHTING(ch);
  int pct;
  struct affected_type af;

  if (cmd && argument && CMD_IS("say"))
  {
    if (!is_wearing(ch, jot_converter(0)))
      return 0;

    skip_spaces(&argument);

    if (FIGHTING(ch) && (FIGHTING(ch)->in_room == ch->in_room) && !strcmp(argument, "frostbite"))
    {
      if (GET_OBJ_SPECTIMER(obj, 0) > 0)
      {
        send_to_char(ch, "\tcAs you say '\twfrostbite\tc' to your a \tLa great iron axe \tCrimmed "
                         "\tLwith \tWfrost\tc, nothing happens.\tn\r\n");
        return 1;
      }
      act("\tCAs you say, '\twfrostbite\tC',\n\r"
          "\tCa swirling gale of pounding ice emanates forth from\n\r"
          "$p \tCpelting your foes.\tn",
          FALSE, ch, obj, 0, TO_CHAR);
      act("\tCAs $n \tCmutters something under his breath,\n\r"
          "\tCa swirling gale of pounding ice emanates forth from\n\r"
          "$p \tCpelting $n's \tCfoes.\tn",
          FALSE, ch, obj, 0, TO_ROOM);

      pct = rand_number(0, 99);
      if (pct < 55)
        call_magic(ch, vict, 0, SPELL_CONE_OF_COLD, 0, 20, CAST_WEAPON_SPELL);
      else if (pct < 85)
        call_magic(ch, vict, 0, SPELL_CONE_OF_COLD, 0, 30, CAST_WEAPON_SPELL);
      else
      {
        call_magic(ch, vict, 0, SPELL_CONE_OF_COLD, 0, 30, CAST_WEAPON_SPELL);
        if (!paralysis_immunity(vict))
        {
          new_affect(&af);
          af.spell = SPELL_HOLD_PERSON;
          SET_BIT_AR(af.bitvector, AFF_PARALYZED);
          af.duration = dice(2, 4);
          affect_join(vict, &af, TRUE, FALSE, FALSE, FALSE);
        }
      }

      GET_OBJ_SPECTIMER(obj, 0) = 24;
      return 1;
    }
    else
      return 0;
  }
  return 0;
}

/* special claws gear with proc */
#define VAP_AFFECTS 3

SPECIAL(vaprak_claws)
{
  struct affected_type af[VAP_AFFECTS];
  int duration = 0, i = 0;
  struct obj_data *obj = (struct obj_data *)me;

  skip_spaces(&argument);

  if (!ch)
    return 0;

  if (!cmd && !strcmp(argument, "identify"))
  {
    send_to_char(ch, "Invoke Fury of Vaprak by saying 'vaprak'. Once per day.\r\nWorks only for "
                     "Trolls and Ogres.\r\n");
    return 1;
  }

  if (!argument)
    return 0;

  /*
  if (GET_RACE(ch) != RACE_OGRE && GET_RACE(ch) != RACE_HALF_TROLL)
    return 0;
   */
  if (GET_RACE(ch) != RACE_HALF_TROLL)
    return 0;

  if (!is_wearing(ch, jot_converter(62)))
    return 0;

  skip_spaces(&argument);

  if (!strcmp(argument, "vaprak") && CMD_IS("say"))
  {
    // if (FIGHTING(ch) && (FIGHTING(ch)->in_room == ch->in_room)) {
    if (GET_OBJ_SPECTIMER(obj, 0) > 0)
    {
      send_to_char(ch, "\trAs you say '\twvaprak\tr' to your claws \tLof the destroyer\tr, nothing "
                       "happens.\tn\r\n");
      return 1;
    }

    if (affected_by_spell(ch, SKILL_RAGE) || affected_by_spell(ch, SKILL_DEFENSIVE_STANCE))
    {
      send_to_char(ch, "You are already raging or in a defensive stance!\r\n");
      return 1;
    }

    weapons_spells("\tLAs you say '\twvaprak\tL' to $p\tL, an evil warmth fills your body.\tn", 0,
                   "\tr$n \trmutters something under his breath.\tn", ch, ch, (struct obj_data *)me,
                   0);

    duration = GET_LEVEL(ch);
    /* init affect array */
    for (i = 0; i < VAP_AFFECTS; i++)
    {
      new_affect(&(af[i]));
      af[i].spell = SKILL_RAGE;
      af[i].duration = duration;
    }

    af[0].location = APPLY_HITROLL;
    af[0].modifier = 3;

    af[1].location = APPLY_DAMROLL;
    af[1].modifier = 3;

    af[2].location = APPLY_SAVING_WILL;
    af[2].modifier = 3;
    SET_BIT_AR(af[2].bitvector, AFF_HASTE);

    for (i = 0; i < VAP_AFFECTS; i++)
      affect_join(ch, af + i, FALSE, FALSE, FALSE, FALSE);
    GET_OBJ_SPECTIMER(obj, 0) = 24;
    return 1; /* success! */
  }

  return 0;
}
#undef VAP_AFFECTS

/* a fake twilight proc (large sword) */
#define TWI_AFFECTS 2

SPECIAL(fake_twilight)
{
  struct affected_type af[TWI_AFFECTS];
  struct char_data *vict;
  int duration = 0, i = 0;

  if (!ch)
    return 0;

  if (!cmd && !strcmp(argument, "identify"))
  {
    send_to_char(ch, "Twilight Rage.\r\n");
    return 1;
  }

  vict = FIGHTING(ch);

  if (affected_by_spell(ch, SPELL_BATTLETIDE))
  {
    return 0;
  }

  if (cmd || !vict || rand_number(0, 16))
    return 0;

  weapons_spells("\tLA glimmer of insanity crosses your face as your\r\n"
                 "\tLblade starts glowing with a strong \tpmagenta\tL sheen.\tn",
                 "\tLA glimmer of insanity crosses $n\tL's face as $s\r\n"
                 "\tLblade starts glowing with a strong \tpmagenta\tL sheen.\tn",
                 "\tLA glimmer of insanity crosses $n\tL's face as $s\r\n"
                 "\tLblade starts glowing with a strong \tpmagenta\tL sheen.\tn",
                 ch, vict, (struct obj_data *)me, 0);

  duration = GET_LEVEL(ch) / 5;
  /* init affect array */
  for (i = 0; i < TWI_AFFECTS; i++)
  {
    new_affect(&(af[i]));
    af[i].spell = SPELL_BATTLETIDE;
    af[i].duration = duration;
  }

  af[0].location = APPLY_HITROLL;
  af[0].modifier = GET_STR_BONUS(ch);

  af[1].location = APPLY_DAMROLL;
  af[1].modifier = GET_STR_BONUS(ch);

  for (i = 0; i < TWI_AFFECTS; i++)
    affect_join(ch, af + i, FALSE, FALSE, FALSE, FALSE);

  return 1;
}

/* a twilight proc (large sword) */
SPECIAL(twilight)
{
  struct affected_type af[TWI_AFFECTS];
  struct char_data *vict;
  int duration = 0, i = 0;

  if (!ch)
    return 0;

  if (!cmd && !strcmp(argument, "identify"))
  {
    send_to_char(ch, "Twilight Rage!\r\n");
    return 1;
  }

  vict = FIGHTING(ch);

  if (affected_by_spell(ch, SPELL_BATTLETIDE))
  {
    return 0;
  }

  if (cmd || !vict || rand_number(0, 12))
    return 0;

  weapons_spells("\tLA glimmer of insanity crosses your face as your\r\n"
                 "\tLblade starts glowing with a strong \tpmagenta\tL sheen.\tn",
                 "\tLA glimmer of insanity crosses $n\tL's face as $s\r\n"
                 "\tLblade starts glowing with a strong \tpmagenta\tL sheen.\tn",
                 "\tLA glimmer of insanity crosses $n\tL's face as $s\r\n"
                 "\tLblade starts glowing with a strong \tpmagenta\tL sheen.\tn",
                 ch, vict, (struct obj_data *)me, 0);

  duration = GET_LEVEL(ch) / 5 + 1;
  /* init affect array */
  for (i = 0; i < TWI_AFFECTS; i++)
  {
    new_affect(&(af[i]));
    af[i].spell = SPELL_BATTLETIDE;
    af[i].duration = duration;
  }

  af[0].location = APPLY_HITROLL;
  af[0].modifier = GET_STR_BONUS(ch);
  af[0].bonus_type = BONUS_TYPE_CIRCUMSTANCE;

  af[1].location = APPLY_DAMROLL;
  af[1].modifier = GET_STR_BONUS(ch);
  af[1].bonus_type = BONUS_TYPE_CIRCUMSTANCE;

  for (i = 0; i < TWI_AFFECTS; i++)
    affect_join(ch, af + i, FALSE, FALSE, FALSE, FALSE);

  return 1;
}
#undef TWI_AFFECTS

SPECIAL(valkyrie_sword)
{
  if (!ch || cmd)
    return 0;

  if (!cmd && !strcmp(argument, "identify"))
  {
    send_to_char(ch, "Female Only - Proc Burning Hands\r\n");
    return 1;
  }

  if (GET_SEX(ch) != SEX_FEMALE && !IS_NPC(ch))
  {
    damage(ch, ch, dice(5, 4), -1, DAM_HOLY, FALSE);
    send_to_char(ch, "\twYou are \tYburned \twby holy light.\tn\r\n");
    act("\tw$n is \tYburned \twby holy light.\tn", FALSE, ch, 0, ch, TO_ROOM);
    return 1;
  }

  struct char_data *vict = FIGHTING(ch);

  if (!is_wearing(ch, jot_converter(56)) || !vict || rand_number(0, 20))
    return 0;

  weapons_spells("\tYStreaks of flames issue forth from $p\n\r"
                 "\tYengulfing your foe.\tn",
                 "\tYYou are engulfed by the flames issuing forth from $p.",
                 "\tYStreaks of flames issue forth from $p\n\r"
                 "\tYengulfing $n's \tYfoe.",
                 ch, vict, (struct obj_data *)me, 0);

  call_magic(ch, vict, 0, SPELL_BURNING_HANDS, 0, 30, CAST_WEAPON_SPELL);

  return 1;
}

SPECIAL(planetar_sword)
{
  if (!ch)
    return 0;

  if (!cmd && !strcmp(argument, "identify"))
  {
    send_to_char(ch, "Proc Cure Critical and Dispel Evil\r\n");
    return 1;
  }

  struct char_data *vict = FIGHTING(ch);

  if (cmd || !vict || rand_number(0, 27))
    return 0;

  switch (rand_number(0, 1))
  {
  case 1:
    weapons_spells("\tWA nimbus of holy light surrounds your sword, bathing you in its radiance\tn",
                   0,
                   "\tWA nimbus of holy light surrounds $n's\tW sword, bathing $m in its radiance.",
                   ch, ch, (struct obj_data *)me, SPELL_CURE_CRITIC);
    call_magic(ch, ch, 0, SPELL_CURE_CRITIC, 0, GET_LEVEL(ch), CAST_WEAPON_SPELL);
    return 1;
  case 2:
    weapons_spells(
        "\tWA glowing nimbus of light emanates forth blasting the foul evil in its presence.\tn",
        "\tWA glowing nimbus of light emanates forth from $n, blasting the foul evil in its "
        "presence.\tn",
        "\tWA glowing nimbus of light emanates forth from $n, blasting the foul evil in its "
        "presence.\tn",
        ch, vict, (struct obj_data *)me, SPELL_DISPEL_EVIL);
    return 1;
  default:
    return 0;
  }

  return 1;
}

SPECIAL(giantslayer)
{
  if (!ch)
    return 0;

  if (!cmd && !strcmp(argument, "identify"))
  {
    send_to_char(ch, "Invoke giant hamstring attack by saying 'hamstring'. Once per day.\r\nWorks "
                     "only for Dwarves.\r\n");
    return 1;
  }

  switch (GET_RACE(ch))
  {
  case RACE_DWARF:
    break;

  case RACE_DUERGAR:
    break;

  default:
    return 0;
    break;
  }

  struct obj_data *obj = (struct obj_data *)me;
  struct char_data *vict = FIGHTING(ch);

  if (!vict)
    return 0;

  skip_spaces(&argument);
  if (!is_wearing(ch, jot_converter(66)))
    return 0;
  if (!strcmp(argument, "hamstring"))
  {
    if (IS_NPC(vict) && GET_RACE(vict) == RACE_TYPE_GIANT && (vict->in_room == ch->in_room))
    {
      if (GET_OBJ_SPECTIMER(obj, 0) > 0)
      {
        send_to_char(ch, "\tYAs you say '\twhamstring\tY' to your \tLa double-bladed dwarvish axe "
                         "of \tYgiantslaying, nothing happens.\tn\r\n");
        return 1;
      }

      act("\tyAs you say, '\tLhamstring\ty' to $p\ty,\n\r"
          "\tyit twirls forth from your hand, arcing through the air to "
          "hamstring\n\r$N \tybefore returning to your grasp.\tn",
          FALSE, ch, obj, vict, TO_CHAR);
      act("\tyAs $n \tymutters something under his breath to $p\ty,\n\r"
          "\tyit twirls forth from $s hand, arcing through the air to "
          "hamstring\n\r$N \tybefore returning to your grasp.\tn",
          FALSE, ch, obj, vict, TO_ROOM);
      // We hamstring the foe
      act("$N falls to $S knees before you!", FALSE, ch, obj, vict, TO_CHAR);
      act("$N falls to $S knees before $n!", FALSE, ch, obj, vict, TO_NOTVICT);
      act("You fall to your knees in agony!", FALSE, ch, obj, vict, TO_VICT);
      USE_MOVE_ACTION(vict);
      change_position(vict, POS_SITTING);
      GET_HIT(vict) -= 100;

      GET_OBJ_SPECTIMER(obj, 0) = 24;
      return 1; // end for
    }
    else
    {
      send_to_char(ch, "\tYAs you say '\twhamstring\tY' to your \tLa double-bladed dwarvish axe of "
                       "\tYgiantslaying, nothing happens.\tn\r\n");
      return 1;
    }
    return 0;
  }
  return 0;
}

#undef JOT_VNUM
#undef MAX_FG    // fire giants
#undef MAX_SB    // smoking beard batallion
#undef MAX_EM    // efreeti mercenaries
#undef MAX_FROST // frost giants

/*****************/
/* End Jot       */
/*****************/
