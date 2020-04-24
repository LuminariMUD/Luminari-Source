/**************************************************************************
 *  File: limits.c                                     Part of LuminariMUD *
 *  Usage: Limits & gain funcs for HMV, exp, hunger/thirst, idle time.     *
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
#include "spells.h"
#include "comm.h"
#include "db.h"
#include "handler.h"
#include "interpreter.h"
#include "dg_scripts.h"
#include "class.h"
#include "fight.h"
#include "screen.h"
#include "mud_event.h"
#include "mudlim.h"
#include "act.h"
#include "actions.h"
#include "domains_schools.h"
#include "grapple.h"
#include "constants.h"
#include "alchemy.h"

/* added this for falling event, general dummy check */
bool death_check(struct char_data *ch)
{
  /* we're just making sure damage() is called if he should be dead */

  if (HAS_FEAT(ch, FEAT_DEATHLESS_FRENZY) && affected_by_spell(ch, SKILL_RAGE))
  {
    if (GET_HIT(ch) <= -51)
    {
      damage(ch, ch, 999, TYPE_UNDEFINED, DAM_FORCE, FALSE);
      return TRUE; // dead for sure now!
    }
    else
      return FALSE;
  }

  if (GET_HIT(ch) <= -12)
  {
    damage(ch, ch, 999, TYPE_UNDEFINED, DAM_FORCE, FALSE);
    return TRUE; // dead for sure now!
  }

  return FALSE;
}

/* engine for checking a room-affect to see if it fires */
void room_aff_tick(struct raff_node *raff)
{
  struct room_data *caster_room = NULL;
  struct char_data *caster = NULL;
  int casttype = CAST_SPELL;
  int level = DG_SPELL_LEVEL;

  switch (raff->spell)
  {
  case SPELL_ACID_FOG:
    caster = read_mobile(DG_CASTER_PROXY, VIRTUAL);
    caster_room = &world[raff->room];
    if (!caster)
    {
      script_log("comm.c: Cannot load the caster mob (acid fog)!");
      return;
    }

    /* set the caster's name */
    caster->player.short_descr = strdup("The room");
    caster->next_in_room = caster_room->people;
    caster_room->people = caster;
    caster->in_room = real_room(caster_room->number);
    call_magic(caster, NULL, NULL, SPELL_ACID, 0, DG_SPELL_LEVEL, CAST_SPELL);
    extract_char(caster);
    break;
  case SPELL_BILLOWING_CLOUD:
    for (caster = world[raff->room].people; caster; caster = caster->next_in_room)
    {
      if (caster && GET_LEVEL(caster) < 13)
      {
        if (!mag_savingthrow(caster, caster, SAVING_FORT, 0, casttype, level, CONJURATION))
        {
          send_to_char(caster, "You are bogged down by the billowing cloud!\r\n");
          act("$n is bogged down by the billowing cloud.", TRUE, caster, 0, NULL, TO_ROOM);
          USE_MOVE_ACTION(caster);
        }
      }
    }
    break;
  case SPELL_BLADE_BARRIER:
    caster = read_mobile(DG_CASTER_PROXY, VIRTUAL);
    caster_room = &world[raff->room];
    if (!caster)
    {
      script_log("comm.c: Cannot load the caster mob (blade barrier)!");
      return;
    }

    /* set the caster's name */
    caster->player.short_descr = strdup("The room");
    caster->next_in_room = caster_room->people;
    caster_room->people = caster;
    caster->in_room = real_room(caster_room->number);
    call_magic(caster, NULL, NULL, SPELL_BLADES, 0, DG_SPELL_LEVEL, CAST_SPELL);
    extract_char(caster);
    break;
  case SPELL_STINKING_CLOUD:
    caster = read_mobile(DG_CASTER_PROXY, VIRTUAL);
    caster_room = &world[raff->room];
    if (!caster)
    {
      script_log("comm.c: Cannot load the caster mob!");
      return;
    }

    /* set the caster's name */
    caster->player.short_descr = strdup("The room");
    caster->next_in_room = caster_room->people;
    caster_room->people = caster;
    caster->in_room = real_room(caster_room->number);
    call_magic(caster, NULL, NULL, SPELL_STENCH, 0, DG_SPELL_LEVEL, CAST_SPELL);
    extract_char(caster);
    break;
  }
}

/* engine for 'afflictions' related to pulse_luminari */
void affliction_tick(struct char_data *ch)
{
  /* cloudkill */
  if (CLOUDKILL(ch))
  {
    call_magic(ch, NULL, NULL, SPELL_DEATHCLOUD, 0, MAGIC_LEVEL(ch), CAST_SPELL);
    CLOUDKILL(ch)
    --;
    if (CLOUDKILL(ch) <= 0)
    {
      send_to_char(ch, "Your cloud of death dissipates!\r\n");
      act("The cloud of death following $n dissipates!", TRUE, ch, 0, NULL,
          TO_ROOM);
    }
  } //end cloudkill

  /* creeping doom */
  else if (DOOM(ch))
  {
    call_magic(ch, NULL, NULL, SPELL_DOOM, 0, DIVINE_LEVEL(ch), CAST_SPELL);
    DOOM(ch)
    --;
    if (DOOM(ch) <= 0)
    {
      send_to_char(ch, "Your creeping swarm of centipedes dissipates!\r\n");
      act("The creeping swarm of centipedes following $n dissipates!", TRUE, ch, 0, NULL,
          TO_ROOM);
    }
  } //end creeping doom

  /* incendiary cloud */
  else if (INCENDIARY(ch))
  {
    call_magic(ch, NULL, NULL, SPELL_INCENDIARY, 0, MAGIC_LEVEL(ch), CAST_SPELL);
    INCENDIARY(ch)
    --;
    if (INCENDIARY(ch) <= 0)
    {
      send_to_char(ch, "Your incendiary cloud dissipates!\r\n");
      act("The incendiary cloud following $n dissipates!", TRUE, ch, 0, NULL,
          TO_ROOM);
    }
  }
  //end incendiary cloud

  /* disease */
  if (IS_AFFECTED(ch, AFF_DISEASE))
  {
    if (!IS_NPC(ch) && (HAS_FEAT(ch, FEAT_DIVINE_HEALTH) || HAS_FEAT(ch, FEAT_DIAMOND_BODY)))
    {
      if (affected_by_spell(ch, SPELL_EYEBITE))
        affect_from_char(ch, SPELL_EYEBITE);
      if (affected_by_spell(ch, SPELL_CONTAGION))
        affect_from_char(ch, SPELL_CONTAGION);
      if (IS_AFFECTED(ch, AFF_DISEASE))
        REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_DISEASE);
      send_to_char(ch, "The \tYdisease\tn you have fades away!\r\n");
      act("$n glows bright \tWwhite\tn and the \tYdisease\tn $e had "
          "fades away!",
          TRUE, ch, 0, NULL, TO_ROOM);
    }
    else if (GET_HIT(ch) > (GET_MAX_HIT(ch) * 3 / 5))
    {
      send_to_char(ch, "The \tYdisease\tn you have causes you to suffer!\r\n");
      act("$n suffers from a \tYdisease\tn!", TRUE, ch, 0, NULL,
          TO_ROOM);
      GET_HIT(ch) = GET_MAX_HIT(ch) * 3 / 5;
    }
  }

  remove_fear_affects(ch, TRUE);
}

/* dummy check mostly, checks to see if mount/rider got seperated */
void mount_cleanup(struct char_data *ch)
{
  if (RIDING(ch))
  {
    if (RIDDEN_BY(RIDING(ch)) != ch)
    {
      /* dismount both of these guys */
      dismount_char(ch);
      dismount_char(RIDDEN_BY(RIDING(ch)));
    }
    else if (IN_ROOM(RIDING(ch)) != IN_ROOM(ch))
    {
      /* not in same room?  dismount 'em */
      dismount_char(ch);
    }
  }
  else if (RIDDEN_BY(ch))
  {
    if (RIDING(RIDDEN_BY(ch)) != ch)
    {
      /* dismount both of these guys */
      dismount_char(ch);
      dismount_char(RIDING(RIDDEN_BY(ch)));
    }
    else if (IN_ROOM(RIDDEN_BY(ch)) != IN_ROOM(ch))
    {
      /* not in same room?  dismount 'em */
      dismount_char(ch);
    }
  }
}

/* a tick counter that checks for room-based hazards, like
 * falling/drowning/lava/etc */
void hazard_tick(struct char_data *ch)
{
  bool flying = FALSE;

  flying = FALSE;
  if (is_flying(ch))
    flying = TRUE;
  if (RIDING(ch) && is_flying(RIDING(ch)))
    flying = TRUE;

  /* falling */
  if (char_should_fall(ch, TRUE) && !char_has_mud_event(ch, eFALLING))
  {
    /* the svariable value of 20 is just a rough number for feet */
    attach_mud_event(new_mud_event(eFALLING, ch, "20"), 5);
    send_to_char(ch, "Suddenly your realize you are falling!\r\n");
    act("$n has just realized $e has no visible means of support!",
        FALSE, ch, 0, 0, TO_ROOM);
  }

  if (!IS_NPC(ch))
  {
    switch (SECT(IN_ROOM(ch)))
    {
    case SECT_LAVA:
      if (!AFF_FLAGGED(ch, AFF_ELEMENT_PROT))
        damage(ch, ch, rand_number(1, 50), TYPE_UNDEFINED, DAM_FIRE, FALSE);
      break;
    case SECT_UNDERWATER:
      if (IS_NPC(ch) && (GET_MOB_VNUM(ch) == 1260 || IS_UNDEAD(ch)))
        break;
      if (!AFF_FLAGGED(ch, AFF_WATER_BREATH) && !ROOM_FLAGGED(IN_ROOM(ch), ROOM_AIRY))
        damage(ch, ch, rand_number(1, 65), TYPE_UNDEFINED, DAM_WATER, FALSE);
      break;
    }
  }
}

/*  pulse_luminari was built to throw in customized Luminari
 *  procedures that we want called in a similar manner as the
 *  other pulses.  The whole concept was created before I had
 *  a full grasp on the event system, otherwise it would have
 *  been implemented differently.  -Zusuk
 *
 *  Also should be noted, its nice to keep this off-beat with
 *  PULSE_VIOLENCE, it has a little nicer feel to it
 */
void pulse_luminari()
{
  struct char_data *i = NULL;
  struct raff_node *raff = NULL, *next_raff = NULL;

  // room-affections, loop through em
  for (raff = raff_list; raff; raff = next_raff)
  {
    next_raff = raff->next;

    /* will check a room it has room affection to fire */
    room_aff_tick(raff);
  }

  // looping through char list, what needs to be done?
  for (i = character_list; i; i = i->next)
  {

    /* dummy check + added for falling event */
    if (death_check(i))
      continue; // i is dead

    /* 04/07/13 - added position check since pos_fighting is deprecated */
    if (GET_POS(i) == POS_FIGHTING && !FIGHTING(i))
      change_position(i, POS_STANDING);

    /* safety check to make sure you aren't firing when not fighting */
    if (!FIGHTING(i))
      FIRING(i) = 0;

    /* a function meant to check for room-based hazards, like
       falling, drowning, lava, etc */
    hazard_tick(i);

    /* mount clean-up */
    mount_cleanup(i);

    /* vitals regeneration */
    if (GET_HIT(i) == GET_MAX_HIT(i) &&
        GET_MOVE(i) == GET_MAX_MOVE(i) &&
        GET_PSP(i) == GET_MAX_PSP(i) &&
        !AFF_FLAGGED(i, AFF_POISON))
      ;
    else
      regen_update(i);

    /* weapon spells */
    // weapon spells call (in fight.c currently)
    idle_weapon_spells(i);

    /* an assortment of affliction types */
    affliction_tick(i);

    /* grapple cleanup */
    grapple_cleanup(i);

  } // end char list loop
}

/* When age < 15 return the value p0
 When age is 15..29 calculate the line between p1 & p2
 When age is 30..44 calculate the line between p2 & p3
 When age is 45..59 calculate the line between p3 & p4
 When age is 60..79 calculate the line between p4 & p5
 When age >= 80 return the value p6 */
int graf(int grafage, int p0, int p1, int p2, int p3, int p4, int p5, int p6)
{

  if (grafage < 15)
    return (p0); /* < 15   */
  else if (grafage <= 29)
    return (p1 + (((grafage - 15) * (p2 - p1)) / 15)); /* 15..29 */
  else if (grafage <= 44)
    return (p2 + (((grafage - 30) * (p3 - p2)) / 15)); /* 30..44 */
  else if (grafage <= 59)
    return (p3 + (((grafage - 45) * (p4 - p3)) / 15)); /* 45..59 */
  else if (grafage <= 79)
    return (p4 + (((grafage - 60) * (p5 - p4)) / 20)); /* 60..79 */
  else
    return (p6); /* >= 80 */
}

void regen_update(struct char_data *ch)
{
  struct char_data *tch = NULL;
  int hp = 0, found = 0;

  // poisoned, and dying people should suffer their damage from anyone they are
  // fighting in order that xp goes to the killer (who doesn't strike the last blow)
  // -zusuk
  if (AFF_FLAGGED(ch, AFF_POISON))
  {

    /*  */
    if (!IS_NPC(ch) && HAS_FEAT(ch, FEAT_VENOM_IMMUNITY))
    {
      send_to_char(ch, "Your venom immunity purges the poison!\r\n");
      act("$n appears better as their body purges away some poison.", TRUE, ch, 0, 0, TO_ROOM);
      if (affected_by_spell(ch, SPELL_POISON))
        affect_from_char(ch, SPELL_POISON);
      if (IS_AFFECTED(ch, AFF_POISON))
        REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_POISON);
      return;
    }

    /* purity of body feat */
    if (!IS_NPC(ch) && HAS_FEAT(ch, FEAT_PURITY_OF_BODY))
    {
      send_to_char(ch, "Your purity of body purges the poison!\r\n");
      act("$n appears better as their body purges away some poison.", TRUE, ch, 0, 0, TO_ROOM);
      if (affected_by_spell(ch, SPELL_POISON))
        affect_from_char(ch, SPELL_POISON);
      if (IS_AFFECTED(ch, AFF_POISON))
        REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_POISON);
      return;
    }

    /* poison immunity feat */
    if (!IS_NPC(ch) && HAS_FEAT(ch, FEAT_POISON_IMMUNITY))
    {
      send_to_char(ch, "Your poison immunity purges the poison!\r\n");
      act("$n appears better as their body purges away some poison.", TRUE, ch, 0, 0, TO_ROOM);
      if (affected_by_spell(ch, SPELL_POISON))
        affect_from_char(ch, SPELL_POISON);
      if (IS_AFFECTED(ch, AFF_POISON))
        REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_POISON);
      return;
    }

    if (FIGHTING(ch) || dice(1, 2) == 2)
    {
      for (tch = world[IN_ROOM(ch)].people; tch; tch = tch->next_in_room)
      {
        if (!IS_NPC(tch) && FIGHTING(tch) == ch)
        {
          damage(tch, ch, dice(1, 4), SPELL_POISON, KNOWS_DISCOVERY(tch, ALC_DISC_CELESTIAL_POISONS) ? DAM_CELESTIAL_POISON : DAM_POISON, FALSE);
          /* we use to have custom damage message here for this */
          //act("$N looks really \tgsick\tn and shivers uncomfortably.",
          //        FALSE, tch, NULL, ch, TO_CHAR);
          //act("You feel burning \tgpoison\tn in your blood, and suffer.",
          //        FALSE, tch, NULL, ch, TO_VICT | TO_SLEEP);
          //act("$N looks really \tgsick\tn and shivers uncomfortably.",
          //        FALSE, tch, NULL, ch, TO_NOTVICT);
          found = 1;
          break;
        }
      }

      if (!found)
        damage(ch, ch, 1, SPELL_POISON, DAM_POISON, FALSE);
      update_pos(ch);
      return;
    }

  } /* done dealing with poison */

  found = 0;
  tch = NULL;
  if (GET_POS(ch) == POS_MORTALLYW)
  {
    for (tch = world[IN_ROOM(ch)].people; tch; tch = tch->next_in_room)
    {
      if (!IS_NPC(tch) && FIGHTING(tch) == ch)
      {
        damage(tch, ch, 1, TYPE_SUFFERING, DAM_RESERVED_DBC, FALSE);
        found = 1;
        break;
      }
    }
    if (!found)
      damage(ch, ch, 1, TYPE_SUFFERING, DAM_RESERVED_DBC, FALSE);
    update_pos(ch);
    return;
  }

  //50% chance you'll continue dying when incapacitated
  found = 0;
  tch = NULL;
  if (GET_POS(ch) == POS_INCAP && dice(1, 2) == 2)
  {
    for (tch = world[IN_ROOM(ch)].people; tch; tch = tch->next_in_room)
    {
      if (!IS_NPC(tch) && FIGHTING(tch) == ch)
      {
        damage(tch, ch, 1, TYPE_SUFFERING, DAM_RESERVED_DBC, FALSE);
        found = 1;
        break;
      }
    }
    if (!found)
      damage(ch, ch, 1, TYPE_SUFFERING, DAM_RESERVED_DBC, FALSE);
    update_pos(ch);
    return;
  }

  if (IS_NPC(ch) && GET_LEVEL(ch) <= 6 && !AFF_FLAGGED(ch, AFF_CHARM))
  {
    update_pos(ch);
    return;
  }

  if (rand_number(0, 1))
    hp++;

  //position, other bonuses
  if (GET_POS(ch) == POS_SITTING && SITTING(ch) && GET_OBJ_TYPE(SITTING(ch)) == ITEM_FURNITURE)
    hp += dice(3, 2) + 1;
  else if (GET_POS(ch) == POS_RESTING)
    hp += dice(1, 2);
  else if (GET_POS(ch) == POS_RECLINING)
    hp += dice(1, 4);
  else if (GET_POS(ch) == POS_SLEEPING)
    hp += dice(3, 2);

  if (HAS_FEAT(ch, FEAT_FAST_HEALING))
  {
    hp += HAS_FEAT(ch, FEAT_FAST_HEALING) * 3;
  }

  // half-troll racial innate regeneration
  if (GET_RACE(ch) == RACE_HALF_TROLL)
  {
    hp += 3;
    if (FIGHTING(ch))
      hp += 3;
  }

  /* probably put these as last bonuses */
  if (ROOM_FLAGGED(ch->in_room, ROOM_REGEN))
    hp *= 2;
  if (AFF_FLAGGED(ch, AFF_REGEN))
    hp *= 2;

  if (rand_number(0, 3) && GET_LEVEL(ch) <= LVL_IMMORT && !IS_NPC(ch) &&
      (GET_COND(ch, THIRST) == 0 || GET_COND(ch, HUNGER) == 0))
    hp = 0;

  /* blackmantle stops natural regeneration */
  if (AFF_FLAGGED(ch, AFF_BLACKMANTLE) || ROOM_FLAGGED(IN_ROOM(ch), ROOM_NOHEAL))
    hp = 0;

  /* some mechanics put you over maximum hp (purposely), this slowly drains that bonus over time */
  if (GET_HIT(ch) > GET_MAX_HIT(ch) && !rand_number(0, 1))
  {
    GET_HIT(ch)
    --;
  }
  else
  {
    GET_HIT(ch) = MIN(GET_HIT(ch) + hp, GET_MAX_HIT(ch));
  }
  if (GET_MOVE(ch) > GET_MAX_MOVE(ch))
  {
    GET_MOVE(ch)
    --;
  }
  else if (!AFF_FLAGGED(ch, AFF_FATIGUED))
  {
    int move_regen = hp;
    if (!IS_NPC(ch) && HAS_FEAT(ch, FEAT_FAST_MOVEMENT))
      move_regen++;
    if (!IS_NPC(ch) && HAS_FEAT(ch, FEAT_ENDURANCE))
      move_regen += 2;
    move_regen *= 10;
    GET_MOVE(ch) = MIN(GET_MOVE(ch) + (move_regen * 3), GET_MAX_MOVE(ch));
  }

  if (GET_PSP(ch) > GET_MAX_PSP(ch))
  {
    GET_PSP(ch)
    --;
  }
  else
  {
    GET_PSP(ch) = MIN(GET_PSP(ch) + (hp * 2), GET_MAX_PSP(ch));
  }

  update_pos(ch);
  return;
}

/* The hit_limit, psp_limit, and move_limit functions are gone.  They added an
 * unnecessary level of complexity to the internal structure, weren't
 * particularly useful, and led to some annoying bugs.  From the players' point
 * of view, the only difference the removal of these functions will make is
 * that a character's age will now only affect the HMV gain per tick, and _not_
 * the HMV maximums. */

/* psppoint gain pr. game hour */
int psp_gain(struct char_data *ch)
{
  int gain;

  if (IS_NPC(ch))
  {
    /* Neat and fast */
    gain = GET_LEVEL(ch);
  }
  else
  {
    gain = graf(age(ch)->year, 4, 8, 12, 16, 12, 10, 8);

    /* Class calculations */

    /* Skill/Spell calculations */

    /* Position calculations    */
    switch (GET_POS(ch))
    {
    case POS_SLEEPING:
      gain *= 2;
      break;
    case POS_RECLINING:
      gain *= 3;
      gain /= 2;
      break;
    case POS_RESTING:
      gain += (gain / 2); /* Divide by 2 */
      break;
    case POS_SITTING:
      gain += (gain / 4); /* Divide by 4 */
      break;
    }

    if (IS_WIZARD(ch) || IS_CLERIC(ch) || IS_SORCERER(ch) || IS_BARD(ch) || IS_DRUID(ch) || IS_PALADIN(ch) || IS_RANGER(ch))
      gain *= 2;

    if ((GET_COND(ch, HUNGER) == 0) || (GET_COND(ch, THIRST) == 0))
      gain /= 4;
  }

  if (AFF_FLAGGED(ch, AFF_POISON))
    gain /= 4;

  return (gain);
}

/* Hitpoint gain pr. game hour */
int hit_gain(struct char_data *ch)
{
  int gain;

  if (IS_NPC(ch))
  {
    /* Neat and fast */
    gain = GET_LEVEL(ch);
  }
  else
  {

    gain = graf(age(ch)->year, 8, 12, 20, 32, 16, 10, 4);

    /* Class/Level calculations */
    /* Skill/Spell calculations */
    /* Position calculations    */

    switch (GET_POS(ch))
    {
    case POS_SLEEPING:
      gain += (gain / 2); /* Divide by 2 */
      break;
    case POS_RECLINING:
      gain += (gain / 3); /* Divide by 3 */
      break;
    case POS_RESTING:
      gain += (gain / 4); /* Divide by 4 */
      break;
    case POS_SITTING:
      gain += (gain / 8); /* Divide by 8 */
      break;
    }

    if (IS_WIZARD(ch) || IS_CLERIC(ch) || IS_DRUID(ch) ||
        IS_SORCERER(ch))
      gain /= 2; /* Ouch. */

    if ((GET_COND(ch, HUNGER) == 0) || (GET_COND(ch, THIRST) == 0))
      gain /= 4;
  }

  if (AFF_FLAGGED(ch, AFF_POISON))
    gain /= 4;

  return (gain);
}

/* move gain pr. game hour */
int move_gain(struct char_data *ch)
{
  int gain;

  if (IS_NPC(ch))
  {
    /* Neat and fast */
    gain = GET_LEVEL(ch);
  }
  else
  {
    gain = graf(age(ch)->year, 16, 20, 24, 20, 16, 12, 10);

    /* Class/Level calculations */
    /* Skill/Spell calculations */
    /* Position calculations    */
    switch (GET_POS(ch))
    {
    case POS_SLEEPING:
      gain += (gain / 2); /* Divide by 2 */
      break;
    case POS_RECLINING:
      gain += (gain / 3); /* Divide by 3 */
      break;
    case POS_RESTING:
      gain += (gain / 4); /* Divide by 4 */
      break;
    case POS_SITTING:
      gain += (gain / 8); /* Divide by 8 */
      break;
    }

    if ((GET_COND(ch, HUNGER) == 0) || (GET_COND(ch, THIRST) == 0))
      gain /= 4;
  }

  if (AFF_FLAGGED(ch, AFF_POISON))
    gain /= 4;

  gain *= 10;

  return (gain);
}

void set_title(struct char_data *ch, char *title)
{
  if (GET_TITLE(ch) != NULL)
    free(GET_TITLE(ch));

  //why are we checking sex?  old title system -zusuk
  //OK to remove sex check!
  if (title == NULL)
  {
    GET_TITLE(ch) = strdup(GET_SEX(ch) == SEX_FEMALE ? titles(GET_CLASS(ch), GET_LEVEL(ch)) : titles(GET_CLASS(ch), GET_LEVEL(ch)));
  }
  else
  {
    if (strlen(title) > MAX_TITLE_LENGTH)
      title[MAX_TITLE_LENGTH] = '\0';

    GET_TITLE(ch) = strdup(title);
  }
}

void run_autowiz(void)
{
#if defined(CIRCLE_UNIX) || defined(CIRCLE_WINDOWS)
  if (CONFIG_USE_AUTOWIZ)
  {
    size_t res;
    char buf[1024];
    int i;

#if defined(CIRCLE_UNIX)
    res = snprintf(buf, sizeof(buf), "nice ../bin/autowiz %d %s %d %s %d &",
                   CONFIG_MIN_WIZLIST_LEV, WIZLIST_FILE, LVL_IMMORT, IMMLIST_FILE, (int)getpid());
#elif defined(CIRCLE_WINDOWS)
    res = snprintf(buf, sizeof(buf), "autowiz %d %s %d %s",
                   CONFIG_MIN_WIZLIST_LEV, WIZLIST_FILE, LVL_IMMORT, IMMLIST_FILE);
#endif /* CIRCLE_WINDOWS */

    /* Abusing signed -> unsigned conversion to avoid '-1' check. */
    if (res < sizeof(buf))
    {
      mudlog(CMP, LVL_IMMORT, FALSE, "Initiating autowiz.");
      i = system(buf);
      reboot_wizlists();
    }
    else
      log("Cannot run autowiz: command-line doesn't fit in buffer.");
  }
#endif /* CIRCLE_UNIX || CIRCLE_WINDOWS */
}

/* changed to return gain */
#define NEWBIE_EXP 150
#define MIN_NUM_MOBS_TO_KILL_5 9
#define MIN_NUM_MOBS_TO_KILL_10 24
#define MIN_NUM_MOBS_TO_KILL_15 59
#define MIN_NUM_MOBS_TO_KILL_20 120
#define MIN_NUM_MOBS_TO_KILL_25 185
int gain_exp(struct char_data *ch, int gain, int mode)
{
  int xp_to_lvl = 0;
  int gain_cap = 0;

  if (!IS_NPC(ch) && ((GET_LEVEL(ch) < 1 || GET_LEVEL(ch) >= LVL_IMMORT)))
    return 0;

  /* discourage people from killing their pets at the end of the day */
  if (IS_NPC(ch) && AFF_FLAGGED(ch, AFF_CHARM) && ch->master)
  {
    return 0;
  }

  if (IS_NPC(ch))
  {
    GET_EXP(ch) += gain / 2;
    return 0;
  }

  if (gain > 0)
  {

    /* newbie bonus */
    if (GET_LEVEL(ch) <= NEWBIE_LEVEL)
      gain += (int)((float)gain * ((float)NEWBIE_EXP / (float)(100)));

    /* flat rate for now! (halfed the rate for testing purposes) */
    if (ch && ch->desc && ch->desc->account)
    {
      if (gain >= 2500 && ch->desc->account->experience < 33999)
      {
        if (gain / 1250 >= 7) /*reduce spam*/
          send_to_char(ch, "You gain %d account experience points!\r\n", gain / 1250);
        ch->desc->account->experience += gain / 1250;
      }
    }

    /* some limited xp cap conditions */
    switch (mode)
    {
      /* quest, script xp not limited here */
    case GAIN_EXP_MODE_QUEST:
    case GAIN_EXP_MODE_SCRIPT:
    case GAIN_EXP_MODE_DEATH: /* should be negative and not get here! */
      break;
    case GAIN_EXP_MODE_EDRAIN:
    case GAIN_EXP_MODE_CRAFT:
    case GAIN_EXP_MODE_DAMAGE:
    case GAIN_EXP_MODE_DUMP:
      /* further cap these */
      xp_to_lvl = level_exp(ch, GET_LEVEL(ch) + 1) - level_exp(ch, GET_LEVEL(ch));
      if (GET_LEVEL(ch) < 6)
      {
        gain_cap = gain; /* no cap */
      }
      else if (GET_LEVEL(ch) < 11)
      {
        gain_cap = xp_to_lvl / (MIN_NUM_MOBS_TO_KILL_5 * 4);
      }
      else if (GET_LEVEL(ch) < 16)
      {
        gain_cap = xp_to_lvl / (MIN_NUM_MOBS_TO_KILL_10 * 4);
      }
      else if (GET_LEVEL(ch) < 21)
      {
        gain_cap = xp_to_lvl / (MIN_NUM_MOBS_TO_KILL_15 * 4);
      }
      else if (GET_LEVEL(ch) < 26)
      {
        gain_cap = xp_to_lvl / (MIN_NUM_MOBS_TO_KILL_20 * 4);
      }
      else
      {
        gain_cap = xp_to_lvl / (MIN_NUM_MOBS_TO_KILL_25 * 4);
      }
      gain = MIN(gain_cap, gain);
      break;
    case GAIN_EXP_MODE_DEFAULT:
    case GAIN_EXP_MODE_GROUP:
    case GAIN_EXP_MODE_SOLO:
    case GAIN_EXP_MODE_TRAP:
    default:
      xp_to_lvl = level_exp(ch, GET_LEVEL(ch) + 1) - level_exp(ch, GET_LEVEL(ch));
      if (GET_LEVEL(ch) < 6)
      {
        gain_cap = gain; /* no cap */
      }
      else if (GET_LEVEL(ch) < 11)
      {
        gain_cap = xp_to_lvl / (MIN_NUM_MOBS_TO_KILL_5);
      }
      else if (GET_LEVEL(ch) < 16)
      {
        gain_cap = xp_to_lvl / (MIN_NUM_MOBS_TO_KILL_10);
      }
      else if (GET_LEVEL(ch) < 21)
      {
        gain_cap = xp_to_lvl / (MIN_NUM_MOBS_TO_KILL_15);
      }
      else if (GET_LEVEL(ch) < 26)
      {
        gain_cap = xp_to_lvl / (MIN_NUM_MOBS_TO_KILL_20);
      }
      else
      {
        gain_cap = xp_to_lvl / (MIN_NUM_MOBS_TO_KILL_25);
      }
      gain = MIN(gain_cap, gain);
      break;
    }

    /* happy hour bonus, purposely applied after above caps */
    if ((IS_HAPPYHOUR) && (IS_HAPPYEXP))
      gain += (int)((float)gain * ((float)HAPPY_EXP / (float)(100)));

    /* put an absolute cap on the max gain per kill */
    gain = MIN(CONFIG_MAX_EXP_GAIN, gain);

    /* new gain xp cap -zusuk */
    GET_EXP(ch) += gain;
  }
  else if (gain < 0)
  {

    gain = MAX(-CONFIG_MAX_EXP_LOSS, gain); /* Cap max exp lost per death */
    GET_EXP(ch) += gain;
    if (GET_EXP(ch) < 0)
      GET_EXP(ch) = 0;

    send_to_char(ch, "You lose %d experience points!", gain);
  }

  if (GET_LEVEL(ch) >= LVL_IMMORT && !PLR_FLAGGED(ch, PLR_NOWIZLIST))
    run_autowiz();

  if (GET_LEVEL(ch) < LVL_IMMORT - CONFIG_NO_MORT_TO_IMMORT &&
      GET_EXP(ch) >= level_exp(ch, GET_LEVEL(ch) + 1))
    send_to_char(ch, "\tDYou have gained enough xp to advance, type 'gain' to level.\tn\r\n");

  return gain;
}

void gain_exp_regardless(struct char_data *ch, int gain)
{
  int is_altered = FALSE;
  int num_levels = 0;

  if ((IS_HAPPYHOUR) && (IS_HAPPYEXP))
    gain += (int)((float)gain * ((float)HAPPY_EXP / (float)(100)));

  GET_EXP(ch) += gain;
  if (GET_EXP(ch) < 0)
    GET_EXP(ch) = 0;

  if (!IS_NPC(ch))
  {
    while (GET_LEVEL(ch) < LVL_IMPL &&
           GET_EXP(ch) >= level_exp(ch, GET_LEVEL(ch) + 1))
    {
      GET_LEVEL(ch) += 1;
      CLASS_LEVEL(ch, GET_CLASS(ch))
      ++;
      num_levels++;
      advance_level(ch, GET_CLASS(ch));
      is_altered = TRUE;
    }

    if (is_altered)
    {
      mudlog(BRF, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE, "%s advanced %d level%s to level %d.",
             GET_NAME(ch), num_levels, num_levels == 1 ? "" : "s", GET_LEVEL(ch));
      if (num_levels == 1)
        send_to_char(ch, "You rise a level!\r\n");
      else
        send_to_char(ch, "You rise %d levels!\r\n", num_levels);
      set_title(ch, NULL);
    }
  }
  if (GET_LEVEL(ch) >= LVL_IMMORT && !PLR_FLAGGED(ch, PLR_NOWIZLIST))
    run_autowiz();

  if (GET_LEVEL(ch) < LVL_IMMORT - CONFIG_NO_MORT_TO_IMMORT &&
      GET_EXP(ch) >= level_exp(ch, GET_LEVEL(ch) + 1))
    send_to_char(ch,
                 "\tDYou have gained enough xp to advance, type 'gain' to level.\tn\r\n");
}

void gain_condition(struct char_data *ch, int condition, int value)
{
  bool intoxicated;

  if (!ch)
    return;

  if (IS_NPC(ch) || GET_COND(ch, condition) == -1) /* No change */
    return;

  intoxicated = (GET_COND(ch, DRUNK) > 0);

  GET_COND(ch, condition) += value;

  GET_COND(ch, condition) = MAX(0, GET_COND(ch, condition));
  GET_COND(ch, condition) = MIN(24, GET_COND(ch, condition));

  if (GET_COND(ch, condition) || PLR_FLAGGED(ch, PLR_WRITING))
    return;

  switch (condition)
  {
  case HUNGER:
    send_to_char(ch, "You are hungry.\r\n");
    break;
  case THIRST:
    send_to_char(ch, "You are thirsty.\r\n");
    break;
  case DRUNK:
    if (intoxicated)
      send_to_char(ch, "You are now sober.\r\n");
    break;
  default:
    break;
  }
}

void check_idling(struct char_data *ch)
{
  if (ch->char_specials.timer > CONFIG_IDLE_VOID)
  {
    if (GET_WAS_IN(ch) == NOWHERE && IN_ROOM(ch) != NOWHERE)
    {
      GET_WAS_IN(ch) = IN_ROOM(ch);
      if (FIGHTING(ch))
      {
        stop_fighting(FIGHTING(ch));
        stop_fighting(ch);
      }
      act("$n disappears into the void.", TRUE, ch, 0, 0, TO_ROOM);
      send_to_char(ch, "You have been idle, and are pulled into a void.\r\n");
      save_char(ch, 0);
      Crash_crashsave(ch);
      char_from_room(ch);
      char_to_room(ch, 1);
    }
    else if (ch->char_specials.timer > CONFIG_IDLE_RENT_TIME)
    {
      if (IN_ROOM(ch) != NOWHERE)
        char_from_room(ch);
      char_to_room(ch, 3);
      if (ch->desc)
      {
        STATE(ch->desc) = CON_DISCONNECT;
        /*
         * For the 'if (d->character)' test in close_socket().
         * -gg 3/1/98 (Happy anniversary.)
         */
        ch->desc->character = NULL;
        ch->desc = NULL;
      }
      if (CONFIG_FREE_RENT)
        Crash_rentsave(ch, 0);
      else
        Crash_idlesave(ch);
      mudlog(CMP, LVL_STAFF, TRUE, "%s force-rented and extracted (idle).", GET_NAME(ch));
      add_llog_entry(ch, LAST_IDLEOUT);
      extract_char(ch);
    }
  }
}

/* Update PCs, NPCs, and objects */
void point_update(void)
{
  struct char_data *i = NULL, *next_char = NULL;
  struct obj_data *j = NULL, *next_thing, *jj = NULL, *next_thing2 = NULL;
  int counter = 0;

  /** general **/
  /* Take 1 from the happy-hour tick counter, and end happy-hour if zero */
  if (HAPPY_TIME > 1)
    HAPPY_TIME--;
  /* Last tick - set everything back to zero */
  else if (HAPPY_TIME == 1)
  {
    HAPPY_QP = 0;
    HAPPY_EXP = 0;
    HAPPY_GOLD = 0;
    HAPPY_TIME = 0;
    game_info("Happy hour has ended!");
  }

  /** characters **/
  for (i = character_list; i; i = next_char)
  {
    next_char = i->next;

    gain_condition(i, HUNGER, -1);
    gain_condition(i, DRUNK, -1);
    gain_condition(i, THIRST, -1);

    /* old tick regen code use to be here -zusuk */

    if (!IS_NPC(i))
    {
      update_char_objects(i);
      (i->char_specials.timer)++;
      if (GET_LEVEL(i) < CONFIG_IDLE_MAX_LEVEL)
        check_idling(i);
    }
  }

  /** objects **/
  /* Make sure there is only one way to decrement each object timer */
  for (j = object_list; j; j = next_thing)
  {
    next_thing = j->next; /* Next in object list */

    if (!j)
      continue;

    /* object spec timers, for old school object procs */
    for (counter = 0; counter < SPEC_TIMER_MAX; counter++)
    {
      if (GET_OBJ_SPECTIMER(j, counter) > 0)
      {
        GET_OBJ_SPECTIMER(j, counter)
        --;
      }
      if (GET_OBJ_SPECTIMER(j, counter) <= 0)
      {
        /*wear out messages*/
      }
    }

    /* decrement timer */
    if (GET_OBJ_TIMER(j) > 0)
      GET_OBJ_TIMER(j)
    --;

    /* timer counting down that doesn't result in extraction */

    /** Arrow (that is imbued) */
    if (GET_OBJ_TYPE(j) == ITEM_MISSILE && GET_OBJ_VAL(j, 1))
    {
      /* imbued arrow lost its spell! */
      if (GET_OBJ_TIMER(j) <= 0)
      {
        /* simple mechanic is reset obj-val 1 to 0 */
        GET_OBJ_VAL(j, 1) = 0;
        /* now send a message if appropriate */
        if (j->carried_by) /* carried in your inventory */
          act("$p briefly shudders as the imbued magic fades.", FALSE, j->carried_by, j, 0, TO_CHAR);
        if (j->in_obj && j->in_obj->carried_by) /* object carrying the missile */
          act("$p briefly shudders as the imbued magic fades.", FALSE, j->in_obj->carried_by, j, 0, TO_CHAR);
      }
    }

    /** for timed object triggers, make sure this is LAST before countdowns
     * that cause extraction!! **/
    if (GET_OBJ_TIMER(j) <= 0)
    {
      timer_otrigger(j);
    }

    /* END timer counting down that doesn't result in extraction */

    /* start timer counting down that results in extraction */

    /** portals that fade **/
    if (IS_DECAYING_PORTAL(j))
    {
      /* the portal fades */
      if (GET_OBJ_TIMER(j) <= 0)
      {
        /* send message if it makes sense */
        if ((IN_ROOM(j) != NOWHERE) && (world[IN_ROOM(j)].people))
        {
          act("\tnYou watch as $p \tCs\tMh\tCi\tMm\tCm\tMe\tCr\tMs\tn then "
              "fades, then disappears.",
              TRUE, world[IN_ROOM(j)].people,
              j, 0, TO_ROOM);
          act("\tnYou watch as $p \tCs\tMh\tCi\tMm\tCm\tMe\tCr\tMs\tn then "
              "fades, then disappears.",
              TRUE, world[IN_ROOM(j)].people,
              j, 0, TO_CHAR);
        }
        extract_obj(j);
        continue; /* object is gone */
      }
    } /* end portal fade */

    /** general item that fade **/
    if (OBJ_FLAGGED(j, ITEM_DECAY))
    {
      /* the object fades */
      if (GET_OBJ_TIMER(j) <= 0)
      {
        /* send message if it makes sense */
        if ((IN_ROOM(j) != NOWHERE) && (world[IN_ROOM(j)].people))
        {
          act("\tnYou watch as $p fades, then disappears.", TRUE, world[IN_ROOM(j)].people,
              j, 0, TO_ROOM);
          act("\tnYou watch as $p fades, then disappears.", TRUE, world[IN_ROOM(j)].people,
              j, 0, TO_CHAR);
        }
        extract_obj(j);
        continue; /* object is gone */
      }
    } /* end 'general' fade */

    /** If this is a corpse **/
    if (IS_CORPSE(j))
    {
      /* corpse decayed */
      if (GET_OBJ_TIMER(j) <= 0)
      {
        if (j->carried_by)
          act("$p decays in your hands.", FALSE, j->carried_by, j, 0, TO_CHAR);
        else if ((IN_ROOM(j) != NOWHERE) && (world[IN_ROOM(j)].people))
        {
          act("A quivering horde of maggots consumes $p.",
              TRUE, world[IN_ROOM(j)].people, j, 0, TO_ROOM);
          act("A quivering horde of maggots consumes $p.",
              TRUE, world[IN_ROOM(j)].people, j, 0, TO_CHAR);
        }

        for (jj = j->contains; jj; jj = next_thing2)
        {
          next_thing2 = jj->next_content; /* Next in inventory */
          obj_from_obj(jj);

          if (j->in_obj)
            obj_to_obj(jj, j->in_obj);
          else if (j->carried_by)
            obj_to_room(jj, IN_ROOM(j->carried_by));
          else if (IN_ROOM(j) != NOWHERE)
            obj_to_room(jj, IN_ROOM(j));
          else
            core_dump();
        }
        extract_obj(j);
        continue;
      }
    }

  } /* end object loop */
}

/* Note: amt may be negative */
int increase_gold(struct char_data *ch, int amt)
{
  int curr_gold = 0;

  curr_gold = GET_GOLD(ch);

  if (amt < 0)
  {
    GET_GOLD(ch) = MAX(0, curr_gold + amt);
    /* Validate to prevent overflow */
    if (GET_GOLD(ch) > curr_gold)
      GET_GOLD(ch) = 0;
  }
  else
  {
    GET_GOLD(ch) = MIN(MAX_GOLD, curr_gold + amt);
    /* Validate to prevent overflow */
    if (GET_GOLD(ch) < curr_gold)
      GET_GOLD(ch) = MAX_GOLD;
  }
  if (GET_GOLD(ch) == MAX_GOLD)
    send_to_char(ch, "%sYou have reached the maximum gold!\r\n%sYou must spend it or bank it before you can gain any more.\r\n", QBRED, QNRM);

  return (GET_GOLD(ch));
}

int decrease_gold(struct char_data *ch, int deduction)
{
  int amt;
  amt = (deduction * -1);
  increase_gold(ch, amt);
  return (GET_GOLD(ch));
}

int increase_bank(struct char_data *ch, int amt)
{
  int curr_bank;

  if (IS_NPC(ch))
    return 0;

  curr_bank = GET_BANK_GOLD(ch);

  if (amt < 0)
  {
    GET_BANK_GOLD(ch) = MAX(0, curr_bank + amt);
    /* Validate to prevent overflow */
    if (GET_BANK_GOLD(ch) > curr_bank)
      GET_BANK_GOLD(ch) = 0;
  }
  else
  {
    GET_BANK_GOLD(ch) = MIN(MAX_BANK, curr_bank + amt);
    /* Validate to prevent overflow */
    if (GET_BANK_GOLD(ch) < curr_bank)
      GET_BANK_GOLD(ch) = MAX_BANK;
  }
  if (GET_BANK_GOLD(ch) == MAX_BANK)
    send_to_char(ch, "%sYou have reached the maximum bank balance!\r\n%sYou cannot put more into your account unless you withdraw some first.\r\n", QBRED, QNRM);
  return (GET_BANK_GOLD(ch));
}

int decrease_bank(struct char_data *ch, int deduction)
{
  int amt;
  amt = (deduction * -1);
  increase_bank(ch, amt);
  return (GET_BANK_GOLD(ch));
}

void increase_anger(struct char_data *ch, float amount)
{
  if (IS_NPC(ch) && GET_ANGER(ch) <= MAX_ANGER)
    GET_ANGER(ch) = MIN(MAX(GET_ANGER(ch) + amount, 0), MAX_ANGER);
}

void update_damage_and_effects_over_time(void)
{
  int dam = 0;
  struct affected_type *affects = NULL;
  struct char_data *ch = NULL, *next_char = NULL;
  char buf[MAX_STRING_LENGTH];

  for (ch = character_list; ch; ch = next_char)
  {
    next_char = ch->next;

    /* dummy check */
    if (!ch)
      return;

    if (affected_by_spell(ch, BOMB_AFFECT_ACID))
    {
      for (affects = ch->affected; affects; affects = affects->next)
      {
        if (affects->spell == BOMB_AFFECT_ACID)
        {
          act("You suffer in pain as acid continues to burn you.", FALSE, ch, 0, 0, TO_CHAR);
          act("$n suffers in pain as acid continues to burn $m.", FALSE, ch, 0, 0, TO_ROOM);

          dam = damage(ch, ch, affects->modifier, SKILL_BOMB_TOSS, DAM_ACID, SKILL_BOMB_TOSS);

          if (dam <= 0)
          { /* they died */
            break;
          }

          affects->duration--;
          if (affects->duration <= 0)
            affect_from_char(ch, BOMB_AFFECT_ACID);

          break;
        }
      }
    } // end acid bombs

    if (affected_by_spell(ch, BOMB_AFFECT_BONESHARD))
    {
      for (affects = ch->affected; affects; affects = affects->next)
      {
        if (affects->spell == BOMB_AFFECT_BONESHARD)
        {
          act("You suffer in pain as shards of bone embed themselves in your flesh.", FALSE, ch, 0, 0, TO_CHAR);
          act("$n suffers in pain as shards of bone embed themselves in $s flesh.", FALSE, ch, 0, 0, TO_ROOM);
          dam = damage(ch, ch, dice(1, 4), SKILL_BOMB_TOSS, DAM_PUNCTURE, SKILL_BOMB_TOSS);
          if (dam <= 0)
          { /* they died */
            break;
          }
          affects->duration--;
          if (affects->duration <= 0)
            affect_from_char(ch, BOMB_AFFECT_BONESHARD);
          break;
        }
      }
    } // end boneshard bombs

    if (ch->player_specials->sticky_bomb[0] != BOMB_NONE)
    {
      if (ch->player_specials->sticky_bomb[0] != BOMB_FIRE_BRAND && ch->player_specials->sticky_bomb[0] != BOMB_HEALING && FIGHTING(ch)) {
        snprintf(buf, sizeof(buf), "A sticky %s bomb explodes again causing you %s damage.", bomb_types[ch->player_specials->sticky_bomb[0]], weapon_damage_types[ch->player_specials->sticky_bomb[1]]);
        act(buf, FALSE, ch, 0, FIGHTING(ch), TO_VICT);
        snprintf(buf, sizeof(buf), "A sticky %s bomb explodes on $N again causing $M %s damage.", bomb_types[ch->player_specials->sticky_bomb[0]], weapon_damage_types[ch->player_specials->sticky_bomb[1]]);
        act(buf, FALSE, ch, 0, FIGHTING(ch), TO_ROOM);
        dam = damage(ch, FIGHTING(ch), ch->player_specials->sticky_bomb[2], SKILL_BOMB_TOSS, ch->player_specials->sticky_bomb[1], SKILL_BOMB_TOSS);
        ch->player_specials->sticky_bomb[0] = ch->player_specials->sticky_bomb[1] = ch->player_specials->sticky_bomb[2] = 0;
      } else if (ch->player_specials->sticky_bomb[0] != BOMB_HEALING) {
        snprintf(buf, sizeof(buf), "A sticky %s bomb explodes again, healing you for more.", bomb_types[ch->player_specials->sticky_bomb[0]]);
        act(buf, FALSE, ch, 0, 0, TO_CHAR);
        snprintf(buf, sizeof(buf), "A sticky %s bomb explodes again, healing $n for more.", bomb_types[ch->player_specials->sticky_bomb[0]]);
        act(buf, FALSE, ch, 0, 0, TO_ROOM);
        perform_bomb_direct_healing(ch, ch, BOMB_HEALING);
      } else if (ch->player_specials->sticky_bomb[0] != BOMB_FIRE_BRAND) {
        snprintf(buf, sizeof(buf), "A sticky %s bomb explodes again, setting your weapons aflame anew.", bomb_types[ch->player_specials->sticky_bomb[0]]);
        act(buf, FALSE, ch, 0, 0, TO_CHAR);
        snprintf(buf, sizeof(buf), "A sticky %s bomb explodes again, setting $n's weapons aflame anew.", bomb_types[ch->player_specials->sticky_bomb[0]]);
        act(buf, FALSE, ch, 0, 0, TO_ROOM);
        perform_bomb_self_effect(ch, ch, BOMB_FIRE_BRAND);
      }

    } // sticky bomb effects

    // fast healing grand discovery affect
    if (GET_GRAND_DISCOVERY(ch) == GR_ALC_DISC_FAST_HEALING)
    {
      GET_HIT(ch) += 5;
      if (GET_HIT(ch) > GET_MAX_HIT(ch))
        GET_HIT(ch) = GET_MAX_HIT(ch);
    }

    if (affected_by_spell(ch, BOMB_AFFECT_IMMOLATION))
    {
      for (affects = ch->affected; affects; affects = affects->next)
      {
        if (affects->spell == BOMB_AFFECT_IMMOLATION)
        {
          act("You suffer in pain as liquid flames consume you.", FALSE, ch, 0, 0, TO_CHAR);
          act("$n suffers in pain as liquid flames consume $m.", FALSE, ch, 0, 0, TO_ROOM);
          dam = damage(ch, ch, affects->modifier, SKILL_BOMB_TOSS, DAM_FIRE, SKILL_BOMB_TOSS);
          if (dam <= 0)
          { /* they died */
            break;
          }
          affects->duration--;
          if (affects->duration <= 0)
            affect_from_char(ch, BOMB_AFFECT_IMMOLATION);
          break;
        }
      }
    } // end immolation bombs

  } // end character_list loop
}
