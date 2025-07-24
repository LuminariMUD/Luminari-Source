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
#include "staff_events.h"
#include "missions.h"
#include "account.h"
#include "psionics.h"
#include "evolutions.h"
#include "spell_prep.h"

// external functions
void save_char_pets(struct char_data *ch);
void rem_room_aff(struct raff_node *raff);

/* added this for falling event, general dummy check */
bool death_check(struct char_data *ch)
{
  /* we're just making sure damage() is called if he should be dead */

  if (HAS_FEAT(ch, FEAT_DEATHLESS_FRENZY) && affected_by_spell(ch, SKILL_RAGE))
  {
    if (GET_HIT(ch) <= -(GET_MAX_HIT(ch) / 2))
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
  case ABILITY_KAPAK_DRACONIAN_DEATH_THROES:
    caster = read_mobile(DG_CASTER_PROXY, VIRTUAL);
    caster_room = &world[raff->room];
    if (!caster)
    {
      script_log("comm.c: Cannot load the caster mob (kapak acid)!");
      return;
    }

    /* set the caster's name */
    caster->player.short_descr = strdup("The room");
    caster->next_in_room = caster_room->people;
    caster_room->people = caster;
    caster->in_room = real_room(caster_room->number);
    call_magic(caster, NULL, NULL, ABILITY_KAPAK_DRACONIAN_DEATH_THROES, 0, DG_SPELL_LEVEL, CAST_SPELL);
    extract_char(caster);
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
    CLOUDKILL(ch)--;
    if (CLOUDKILL(ch) <= 0)
    {
      send_to_char(ch, "Your cloud of death dissipates!\r\n");
      act("The cloud of death following $n dissipates!", TRUE, ch, 0, NULL,
          TO_ROOM);
    }
  } // end cloudkill

  /* creeping doom */
  else if (DOOM(ch))
  {
    call_magic(ch, NULL, NULL, SPELL_AFFECT_CREEPING_DOOM_BITE, 0, DIVINE_LEVEL(ch) + GET_CALL_EIDOLON_LEVEL(ch), CAST_SPELL);
    call_magic(ch, NULL, NULL, POISON_TYPE_CENTIPEDE_STRONG, 0, DIVINE_LEVEL(ch) + GET_CALL_EIDOLON_LEVEL(ch), CAST_SPELL);
    DOOM(ch)--;
    if (DOOM(ch) <= 0)
    {
      send_to_char(ch, "Your creeping swarm of centipedes dissipates!\r\n");
      act("The creeping swarm of centipedes following $n dissipates!", TRUE, ch, 0, NULL, TO_ROOM);
    }
  } // end creeping doom

  /* tenacious plague */
  else if (TENACIOUS_PLAGUE(ch))
  {
    mag_areas(GET_WARLOCK_LEVEL(ch), ch, NULL, WARLOCK_TENACIOUS_PLAGUE, 0, SAVING_REFL, CAST_INNATE);
    TENACIOUS_PLAGUE(ch)--;
    if (TENACIOUS_PLAGUE(ch) <= 0)
    {
      send_to_char(ch, "Your swarm of biting and stinging insects dissipates!\r\n");
      act("The swarm of biting and stinging insects following $n dissipates!", TRUE, ch, 0, NULL, TO_ROOM);
    }
  } // end tenacious plague

  /* incendiary cloud */
  else if (INCENDIARY(ch))
  {
    call_magic(ch, NULL, NULL, SPELL_INCENDIARY, 0, MAGIC_LEVEL(ch), CAST_SPELL);
    INCENDIARY(ch)--;
    if (INCENDIARY(ch) <= 0)
    {
      send_to_char(ch, "Your incendiary cloud dissipates!\r\n");
      act("The incendiary cloud following $n dissipates!", TRUE, ch, 0, NULL, TO_ROOM);
    }
  }
  // end incendiary cloud

  if (affected_by_spell(ch, WARLOCK_CHILLING_TENTACLES))
  {
    damage(FIGHTING(ch) ? FIGHTING(ch): ch, ch, dice(4, 6) + 13, WARLOCK_CHILLING_TENTACLES, DAM_FORCE, FALSE);
    damage(FIGHTING(ch) ? FIGHTING(ch): ch, ch, dice(2, 6), WARLOCK_CHILLING_TENTACLES, DAM_COLD, FALSE);
  }
  else if (affected_by_spell(ch, SPELL_GREATER_BLACK_TENTACLES))
  {
    damage(FIGHTING(ch) ? FIGHTING(ch): ch, ch, dice(4, 6) + 13, SPELL_GREATER_BLACK_TENTACLES, DAM_FORCE, FALSE);
  }
  else if (affected_by_spell(ch, SPELL_BLACK_TENTACLES))
  {
    damage(FIGHTING(ch) ? FIGHTING(ch) : ch, ch, dice(1, 6) + 4, SPELL_BLACK_TENTACLES, DAM_FORCE, FALSE);
  }


  /* disease */
  if (IS_AFFECTED(ch, AFF_DISEASE))
  {
    if (!IS_NPC(ch) && (HAS_FEAT(ch, FEAT_DIVINE_HEALTH) || HAS_FEAT(ch, FEAT_DIAMOND_BODY) || HAS_FEAT(ch, FEAT_PLAGUE_BRINGER)))
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
    else if (GET_HIT(ch) > MAX(GET_MAX_HIT(ch) - 1000, GET_MAX_HIT(ch) * 3 / 5))
    {
      send_to_char(ch, "The \tYdisease\tn you have causes you to suffer!\r\n");
      act("$n suffers from a \tYdisease\tn!", TRUE, ch, 0, NULL, TO_ROOM);
      GET_HIT(ch) = MAX(GET_MAX_HIT(ch) - 1000, GET_MAX_HIT(ch) * 3 / 5);
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
        damage(ch, ch, rand_number(1, 50), TYPE_LAVA_DAMAGE, DAM_FIRE, FALSE);
      break;
    case SECT_UNDERWATER:
      if (IS_NPC(ch) && (GET_MOB_VNUM(ch) == 1260 || IS_UNDEAD(ch)))
        break;
      if (!IS_UNDEAD(ch) && !AFF_FLAGGED(ch, AFF_WATER_BREATH) && !ROOM_FLAGGED(IN_ROOM(ch), ROOM_AIRY))
        damage(ch, ch, rand_number(1, 65), TYPE_DROWNING, DAM_WATER, FALSE);
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
      FIRING(i) = FALSE;

    /* a function meant to check for room-based hazards, like
       falling, drowning, lava, etc */
    hazard_tick(i);

    /* mount clean-up */
    mount_cleanup(i);

    /* vitals regeneration */
    if (GET_HIT(i) == GET_MAX_HIT(i) &&
        GET_MOVE(i) == GET_MAX_MOVE(i) &&
        GET_PSP(i) == GET_MAX_PSP(i) &&
        !AFF_FLAGGED(i, AFF_POISON) &&
        !AFF_FLAGGED(i, AFF_ACID_COAT))
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

/* we do the math for our hps regen per tick here -zusuk */
int regen_hps(struct char_data *ch)
{
  int hp = 0;

  /* base regen rate */
  if (rand_number(0, 1))
    hp++;

  /* position bonus */
  else if (GET_POS(ch) == POS_RESTING)
    hp += dice(1, 2);
  else if (GET_POS(ch) == POS_RECLINING)
    hp += dice(1, 4);
  else if (GET_POS(ch) == POS_SLEEPING)
    hp += dice(3, 2);
  if (GET_POS(ch) == POS_SITTING && SITTING(ch) && GET_OBJ_TYPE(SITTING(ch)) == ITEM_FURNITURE)
    hp += dice(3, 2) + 1;

  if (HAS_FEAT(ch, FEAT_FAST_HEALING))
    hp += HAS_FEAT(ch, FEAT_FAST_HEALING) * 3;
  else if (HAS_FEAT(ch, FEAT_WARLOCK_FIENDISH_RESILIENCE))
    hp += HAS_FEAT(ch, FEAT_WARLOCK_FIENDISH_RESILIENCE) * 3;

  if (HAS_FEAT(ch, FEAT_VAMPIRE_FAST_HEALING) && !ch->player.exploit_weaknesses)
  {
    if (!((IN_SUNLIGHT(ch)) || (IN_MOVING_WATER(ch))))
      hp += 5;
    if (FIGHTING(ch))
      hp += 3;
  }

  // half-troll racial innate regeneration
  if (HAS_FEAT(ch, FEAT_TROLL_REGENERATION))
  {
    hp += 3;
    if (FIGHTING(ch))
      hp += 3;
  }

  if (FIGHTING(ch) && affected_by_spell(ch, SKILL_RAGE) && HAS_FEAT(ch, FEAT_DEATHLESS_FRENZY))
  {
    hp += 3;
  }

  if (affected_by_spell(ch, SKILL_DEFENSIVE_STANCE) && HAS_FEAT(ch, FEAT_RENEWED_DEFENSE))
  {
    hp += 3;
    if (FIGHTING(ch))
      hp += 3;
  }

  // shadow master feat
  if (IS_SHADOW_CONDITIONS(ch) && HAS_REAL_FEAT(ch, FEAT_SHADOW_MASTER))
  {
    hp += 3;
    if (FIGHTING(ch))
      hp += 3;
  }

  if (!FIGHTING(ch))
    hp += get_hp_regen_amount(ch);

  hp += get_fast_healing_amount(ch);

  /* these are last bonuses (outside of exceptions) because of multiplier */
  if (ROOM_FLAGGED(ch->in_room, ROOM_REGEN))
  {
    if (hp < 2)
      hp = 2;
    hp *= 2;
  }

  if (AFF_FLAGGED(ch, AFF_REGEN))
  {
    if (hp < 2)
      hp = 2;
    hp *= 2;
  }

  /* exception bonuses */
  if (affected_by_spell(ch, PSIONIC_TRUE_METABOLISM))
    hp += 10;

  /* penalties */

  /* blackmantle stops natural regeneration */
  if (AFF_FLAGGED(ch, AFF_BLACKMANTLE) || ROOM_FLAGGED(IN_ROOM(ch), ROOM_NOHEAL))
    hp = 0;

  return hp;
}

/* this function handles poison, entry point for hps rege, and movement regen */
void regen_update(struct char_data *ch)
{
  struct char_data *tch = NULL;
  int hp = 0, found = 0;

  /* poisoned, and dying people should suffer their damage from anyone they are
     fighting in order that xp goes to the killer (who doesn't strike the last blow)
     -zusuk */
  if (AFF_FLAGGED(ch, AFF_POISON))
  {

    /* venom immunity  */
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
    if (!IS_NPC(ch) && (HAS_FEAT(ch, FEAT_POISON_IMMUNITY) || HAS_FEAT(ch, FEAT_SOUL_OF_THE_FEY)))
    {
      send_to_char(ch, "Your poison immunity purges the poison!\r\n");
      act("$n appears better as their body purges away some poison.", TRUE, ch, 0, 0, TO_ROOM);
      if (affected_by_spell(ch, SPELL_POISON))
        affect_from_char(ch, SPELL_POISON);
      if (IS_AFFECTED(ch, AFF_POISON))
        REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_POISON);
      return;
    }

#if !defined(CAMPAIGN_DL)
    if (FIGHTING(ch) || dice(1, 2) == 2)
    {
      for (tch = world[IN_ROOM(ch)].people; tch; tch = tch->next_in_room)
      {
        if (!IS_NPC(tch) && FIGHTING(tch) == ch)
        {
          damage(tch, ch, dice(1, 4), SPELL_POISON, KNOWS_DISCOVERY(tch, ALC_DISC_CELESTIAL_POISONS) ? DAM_CELESTIAL_POISON : DAM_POISON, FALSE);
          /* we use to have custom damage message here for this */
          /* act("$N looks really \tgsick\tn and shivers uncomfortably.",
                     FALSE, tch, NULL, ch, TO_CHAR);
             act("You feel burning \tgpoison\tn in your blood, and suffer.",
                     FALSE, tch, NULL, ch, TO_VICT | TO_SLEEP);
             act("$N looks really \tgsick\tn and shivers uncomfortably.",
                    FALSE, tch, NULL, ch, TO_NOTVICT); */
          found = 1;
          break;
        }
      }

      if (!found)
        damage(ch, ch, 1, SPELL_POISON, DAM_POISON, FALSE);
      update_pos(ch);
      return;
    }
#endif

  } /* done dealing with poison */

  // Similarly people coated in acid will just continue to be hurt.
  if (AFF_FLAGGED(ch, AFF_ACID_COAT))
  {
    if (FIGHTING(ch) || dice(1, 2) == 2)
    {
      for (tch = world[IN_ROOM(ch)].people; tch; tch = tch->next_in_room)
      {
        if (!IS_NPC(tch) && FIGHTING(tch) == ch)
        {
          damage(tch, ch, dice(2, 6), WARLOCK_VITRIOLIC_BLAST, DAM_ACID, FALSE);
          found = 1;
          break;
        }
      }

      if (!found)
        damage(ch, ch, 3, WARLOCK_VITRIOLIC_BLAST, DAM_ACID, FALSE);
      update_pos(ch);
      return;
    }
  } /* done dealing with acid */

  /* mortally wounded, you will die if not aided! */
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

  // 50% chance you'll continue dying when incapacitated
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

  /* we turn off regen for low level npcs */
  if (IS_NPC(ch) && GET_LEVEL(ch) <= 6 && !AFF_FLAGGED(ch, AFF_CHARM))
  {
    update_pos(ch);
    return;
  }

  /****/

  // we don't have hunger and thirst here.
  /*
  if (rand_number(0, 3) && GET_LEVEL(ch) <= LVL_IMMORT && !IS_NPC(ch) &&
      (GET_COND(ch, THIRST) == 0 || GET_COND(ch, HUNGER) == 0))
    hp = 0;
  */

  /* we moved the math of hp regen into a separate function to make it easier to find/ manipulate */
  hp = regen_hps(ch);

  /* some mechanics put you over maximum hp (purposely), this slowly drains that bonus over time */
  if (GET_HIT(ch) > GET_MAX_HIT(ch))
  {
    if (GET_MAX_HIT(ch) - GET_HIT(ch) <= 15)
    {
      GET_HIT(ch)--;
    }
    else if (GET_MAX_HIT(ch) - GET_HIT(ch) <= 45)
    {
      GET_HIT(ch) -= 3;
    }
    else if (GET_MAX_HIT(ch) - GET_HIT(ch) <= 100)
    {
      GET_HIT(ch) -= 10;
    }
    else
    {
      GET_HIT(ch) -= 20;
    }
  }
  else
  {
    GET_HIT(ch) = MIN(GET_HIT(ch) + hp, GET_MAX_HIT(ch));
  }

  /* handle move regen here */
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

    if (!FIGHTING(ch))
      move_regen += get_psp_regen_amount(ch);

    move_regen *= 10; /* conversion to gicker's new system */

    GET_MOVE(ch) = MIN(GET_MOVE(ch) + (move_regen * 3), GET_MAX_MOVE(ch));
  }

  /* this is an extra over-stack drain for PSP, another one exists in the regen_psp() function */
  if (GET_PSP(ch) > GET_MAX_PSP(ch))
  {
    GET_PSP(ch)--;
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

void regen_psp(void)
{
  struct descriptor_data *d = NULL;

  for (d = descriptor_list; d; d = d->next)
  {

    if (STATE(d) != CON_PLAYING)
      continue;
    if (!d->character)
      continue;
    if (IN_ROOM(d->character) == NOWHERE)
      continue;
    if (FIGHTING(d->character))
      continue;

    if (GET_PSP(d->character) < GET_MAX_PSP(d->character))
      GET_PSP(d->character)++;

    if (!FIGHTING(d->character))
      GET_PSP(d->character) += get_psp_regen_amount(d->character);

    if (GET_PSP(d->character) < GET_MAX_PSP(d->character))
      if (HAS_FEAT(d->character, FEAT_PSIONIC_RECOVERY))
        GET_PSP(d->character) += (HAS_FEAT(d->character, FEAT_PSIONIC_RECOVERY) * 2);

    switch (GET_POS(d->character))
    {
    case POS_SLEEPING:
    case POS_RECLINING:
    /*case POS_CRAWLING:*/
    case POS_RESTING:
    case POS_SITTING:
      if (GET_PSP(d->character) < GET_MAX_PSP(d->character))
        GET_PSP(d->character) += 2 + (GET_PSIONIC_LEVEL(d->character) / 7);
      break;
    default:
      break;
    }

    /* we also have a de-regen if over max in another function */
    if (GET_PSP(d->character) > GET_MAX_PSP(d->character))
      GET_PSP(d->character)--;

    if (GET_PSP(d->character) > GET_MAX_PSP(d->character))
      GET_PSP(d->character) = GET_MAX_PSP(d->character);
  }
}

/* psppoint gain pr. game hour */
/* this isn't used anymore -- Gicker */
int psp_gain(struct char_data *ch)
{

  return 0;

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

  if (AFF_FLAGGED(ch, AFF_POISON) || AFF_FLAGGED(ch, AFF_ACID_COAT))
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

    if (IS_WIZARD(ch) || IS_CLERIC(ch) || IS_DRUID(ch) || IS_SORCERER(ch))
      gain /= 2; /* Ouch. */

    if ((GET_COND(ch, HUNGER) == 0) || (GET_COND(ch, THIRST) == 0))
      gain /= 4;
  }

  if (AFF_FLAGGED(ch, AFF_POISON) || AFF_FLAGGED(ch, AFF_ACID_COAT))
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

  if (AFF_FLAGGED(ch, AFF_POISON) || AFF_FLAGGED(ch, AFF_ACID_COAT))
    gain /= 4;

  gain *= 10;

  return (gain);
}

void set_title(struct char_data *ch, char *title)
{
  if (GET_TITLE(ch) != NULL)
    free(GET_TITLE(ch));

  // why are we checking sex?  old title system -zusuk
  // OK to remove sex check!
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

void set_imm_title(struct char_data *ch, char *title)
{
  if (GET_LEVEL(ch) < LVL_IMMORT) return;

  if (GET_IMM_TITLE(ch) != NULL)
    free(GET_IMM_TITLE(ch));

  // why are we checking sex?  old title system -zusuk
  // OK to remove sex check!
  if (title == NULL)
  {
    GET_IMM_TITLE(ch) = strdup(admin_level_names[GET_LEVEL(ch)-LVL_IMMORT]);
  }
  else
  {
    if (strlen(title) > MAX_IMM_TITLE_LENGTH)
      title[MAX_IMM_TITLE_LENGTH] = '\0';

    GET_IMM_TITLE(ch) = strdup(title);
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
  int xp_to_lvl_cap = 0;
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
    return (int)(gain / 2);
  }

  xp_to_lvl_cap = level_exp(ch, GET_LEVEL(ch) + 2);

  if (gain > 0)
  {

    if (GET_EXP(ch) > xp_to_lvl_cap && gain > 0 && GET_LEVEL(ch) < 30)
    {
      send_to_char(ch, "Your experience has been capped.  You must gain a level before you can begin earning experience again.\r\n");
      return 0;
    }

    // leadership bonus
    gain = (int)((float)gain * ((float) leadership_exp_multiplier(ch) / (float)(100)));
    /* newbie bonus */
    if (GET_LEVEL(ch) <= NEWBIE_LEVEL)
      gain += (int)((float)gain * ((float)NEWBIE_EXP / (float)(100)));

    if (HAS_FEAT(ch, FEAT_ADAPTABILITY))
      gain += (int)((float)gain * .05);

    if (HAS_FEAT(ch, FEAT_BG_HERMIT) && get_party_size_same_room(ch))
      gain += (int)((float)gain * .05);

#if defined(CAMPAIGN_DL)
/* flat rate for now! (halfed the rate for testing purposes) */
    if (rand_number(0, 1) && ch && ch->desc && ch->desc->account)
    {
      if (gain >= 1000 && GET_ACCEXP_DESC(ch) <= 99999999)
      {
        if (!ch->char_specials.post_combat_messages)
          send_to_char(ch, "You gain %d account experience points!\r\n", (gain / 1000));
        else
          ch->char_specials.post_combat_account_exp = gain / 1000;
        change_account_xp(ch, (gain / 1000));
      }
    }
#else
    /* flat rate for now! (halfed the rate for testing purposes) */
    if (rand_number(0, 1) && ch && ch->desc && ch->desc->account)
    {
      if (gain >= 3000 && GET_ACCEXP_DESC(ch) <= 99999999)
      {
        
        if (!ch->char_specials.post_combat_messages)
        {
          if (gain / 3000 >= 4)
          {
            send_to_char(ch, "You gain %d account experience points!\r\n", (gain / 3000));
          }
        }
        else
        {
          ch->char_specials.post_combat_account_exp = gain / 3000;
        }
        change_account_xp(ch, (gain / 3000));
      }
    }
#endif

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
    case GAIN_EXP_MODE_GROUP:
    case GAIN_EXP_MODE_SOLO:
    case GAIN_EXP_MODE_DEFAULT:
    case GAIN_EXP_MODE_TRAP:
    default:
      xp_to_lvl = level_exp(ch, GET_LEVEL(ch) + 1) - level_exp(ch, GET_LEVEL(ch));

      /* The no cap for below level 6 was causing serious power levelling issues. -- Gicker May 28, 2020
      if (GET_LEVEL(ch) < 6)
      {
        gain_cap = gain; // no cap
      }
      else
      */

      if (GET_LEVEL(ch) < 11)
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

    /* end game characters get hit much harder */
    if (GET_LEVEL(ch) >= 30)
    {
      gain -= 300000;
    }

    /* bam - hit 'em! */
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

  if (mode == GAIN_EXP_MODE_GROUP || mode == GAIN_EXP_MODE_SOLO)
    ch->char_specials.post_combat_exp = gain;

  return gain;
}

/* */
int gain_exp_regardless(struct char_data *ch, int gain, bool is_ress)
{
  int is_altered = FALSE;
  int num_levels = 0;

  if (!is_ress)
  {
    if ((IS_HAPPYHOUR) && (IS_HAPPYEXP))
      gain += (int)((float)gain * ((float)HAPPY_EXP / (float)(100)));
  }

  GET_EXP(ch) += gain;

  if (GET_EXP(ch) < 0)
    GET_EXP(ch) = 0;

  if (!is_ress)
  {

    if (!IS_NPC(ch))
    {

      while (GET_LEVEL(ch) < LVL_IMPL &&
             GET_EXP(ch) >= level_exp(ch, GET_LEVEL(ch) + 1))
      {
        GET_LEVEL(ch) += 1;
        if (CLASS_LEVEL(ch, GET_CLASS(ch)) < (LVL_STAFF - 1))
          CLASS_LEVEL(ch, GET_CLASS(ch))
        ++;
        num_levels++;
        /* our function for leveling up, takes in class that is being advanced */
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
#if  !defined(CAMPAIGN_FR) && !defined(CAMPAIGN_DL)
        set_title(ch, NULL);
#endif
      }
    }
  }

  if (!is_ress)
  {
    if (GET_LEVEL(ch) >= LVL_IMMORT && !PLR_FLAGGED(ch, PLR_NOWIZLIST))
      run_autowiz();
  }

  if (GET_LEVEL(ch) < LVL_IMMORT - CONFIG_NO_MORT_TO_IMMORT &&
      GET_EXP(ch) >= level_exp(ch, GET_LEVEL(ch) + 1))
    send_to_char(ch,
                 "\tDYou have gained enough xp to advance, type 'gain' to level.\tn\r\n");

  return gain;
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
      save_char_pets(ch);
      dismiss_all_followers(ch);

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

void recharge_activated_items(void)
{
  struct descriptor_data *d = NULL;
  struct char_data *ch = NULL;
  struct obj_data *obj = NULL;
  int i = 0, j = 0;
  char buf[200], where_name[200];

  for (d = descriptor_list; d; d = d->next)
  {
    ch = d->character;
    if (!ch)
      continue;

    if (IS_NPC(ch))
      continue;

    if (STATE(d) != CON_PLAYING)
      continue;

    for (i = 0; i < NUM_WEARS; i++)
    {
      if ((obj = GET_EQ(ch, i)))
      {
        if (obj->activate_spell[ACT_SPELL_SPELLNUM] > 0)
        {
          if (obj->activate_spell[ACT_SPELL_COOLDOWN] > 0)
          {
            obj->activate_spell[ACT_SPELL_COOLDOWN]--;
            if (obj->activate_spell[ACT_SPELL_COOLDOWN] == 0)
            {
              if (obj->activate_spell[ACT_SPELL_MAX_USES] > obj->activate_spell[ACT_SPELL_CURRENT_USES])
              {
                obj->activate_spell[ACT_SPELL_CURRENT_USES]++;
                snprintf(where_name, sizeof(where_name), "%s", equipment_types[i]);
                for (j = 0; j < strlen(where_name); j++)
                {
                  where_name[j] = tolower(where_name[j]);
                }
                snprintf(buf, sizeof(buf), "$p, %s, regains 1 charge of '%s'.",
                  where_name, spell_info[obj->activate_spell[ACT_SPELL_SPELLNUM]].name);
                act(buf, TRUE, ch, obj, 0, TO_CHAR);
                obj->activate_spell[ACT_SPELL_COOLDOWN] = ACT_SPELL_COOLDOWN_TIME;
              }
            }
          }
          else if (obj->activate_spell[ACT_SPELL_MAX_USES] > obj->activate_spell[ACT_SPELL_CURRENT_USES])
          {
            obj->activate_spell[ACT_SPELL_COOLDOWN] = ACT_SPELL_COOLDOWN_TIME;
          }
        }
      }
    }

    for (obj = ch->carrying; obj; obj = obj->next_content)
    {
      if (obj->activate_spell[ACT_SPELL_SPELLNUM] > 0)
        {
          if (obj->activate_spell[ACT_SPELL_COOLDOWN] > 0)
          {
            obj->activate_spell[ACT_SPELL_COOLDOWN]--;
            if (obj->activate_spell[ACT_SPELL_COOLDOWN] == 0)
            {
              if (obj->activate_spell[ACT_SPELL_MAX_USES] > obj->activate_spell[ACT_SPELL_CURRENT_USES])
              {
                obj->activate_spell[ACT_SPELL_CURRENT_USES]++;
                snprintf(buf, sizeof(buf), "$p, in your inventory, regains 1 charge of '%s'.",
                  spell_info[obj->activate_spell[ACT_SPELL_SPELLNUM]].name);
                act(buf, TRUE, ch, obj, 0, TO_CHAR);
              }
            }
          }
        }
    }

    for (obj = ch->bags->bag1; obj; obj = obj->next_content)
    {
      if (obj->activate_spell[ACT_SPELL_SPELLNUM] > 0)
      {
        if (obj->activate_spell[ACT_SPELL_MAX_USES] > obj->activate_spell[ACT_SPELL_CURRENT_USES])
        {
          obj->activate_spell[ACT_SPELL_CURRENT_USES]++;
          snprintf(buf, sizeof(buf), "$p, in your bag #1, regains 1 charge of '%s'.",
            spell_info[obj->activate_spell[ACT_SPELL_SPELLNUM]].name);
          act(buf, TRUE, ch, obj, 0, TO_CHAR);
        }
      }
    }

    for (obj = ch->bags->bag2; obj; obj = obj->next_content)
    {
      if (obj->activate_spell[ACT_SPELL_SPELLNUM] > 0)
      {
        if (obj->activate_spell[ACT_SPELL_MAX_USES] > obj->activate_spell[ACT_SPELL_CURRENT_USES])
        {
          obj->activate_spell[ACT_SPELL_CURRENT_USES]++;
          snprintf(buf, sizeof(buf), "$p, in your bag #2, regains 1 charge of '%s'.",
            spell_info[obj->activate_spell[ACT_SPELL_SPELLNUM]].name);
          act(buf, TRUE, ch, obj, 0, TO_CHAR);
        }
      }
    }

    for (obj = ch->bags->bag3; obj; obj = obj->next_content)
    {
      if (obj->activate_spell[ACT_SPELL_SPELLNUM] > 0)
      {
        if (obj->activate_spell[ACT_SPELL_MAX_USES] > obj->activate_spell[ACT_SPELL_CURRENT_USES])
        {
          obj->activate_spell[ACT_SPELL_CURRENT_USES]++;
          snprintf(buf, sizeof(buf), "$p, in your bag #3, regains 1 charge of '%s'.",
            spell_info[obj->activate_spell[ACT_SPELL_SPELLNUM]].name);
          act(buf, TRUE, ch, obj, 0, TO_CHAR);
        }
      }
    }

    for (obj = ch->bags->bag4; obj; obj = obj->next_content)
    {
      if (obj->activate_spell[ACT_SPELL_SPELLNUM] > 0)
      {
        if (obj->activate_spell[ACT_SPELL_MAX_USES] > obj->activate_spell[ACT_SPELL_CURRENT_USES])
        {
          obj->activate_spell[ACT_SPELL_CURRENT_USES]++;
          snprintf(buf, sizeof(buf), "$p, in your bag #4, regains 1 charge of '%s'.",
            spell_info[obj->activate_spell[ACT_SPELL_SPELLNUM]].name);
          act(buf, TRUE, ch, obj, 0, TO_CHAR);
        }
      }
    }

    for (obj = ch->bags->bag5; obj; obj = obj->next_content)
    {
      if (obj->activate_spell[ACT_SPELL_SPELLNUM] > 0)
      {
        if (obj->activate_spell[ACT_SPELL_MAX_USES] > obj->activate_spell[ACT_SPELL_CURRENT_USES])
        {
          obj->activate_spell[ACT_SPELL_CURRENT_USES]++;
          snprintf(buf, sizeof(buf), "$p, in your bag #5, regains 1 charge of '%s'.",
            spell_info[obj->activate_spell[ACT_SPELL_SPELLNUM]].name);
          act(buf, TRUE, ch, obj, 0, TO_CHAR);
        }
      }
    }

    for (obj = ch->bags->bag6; obj; obj = obj->next_content)
    {
      if (obj->activate_spell[ACT_SPELL_SPELLNUM] > 0)
      {
        if (obj->activate_spell[ACT_SPELL_MAX_USES] > obj->activate_spell[ACT_SPELL_CURRENT_USES])
        {
          obj->activate_spell[ACT_SPELL_CURRENT_USES]++;
          snprintf(buf, sizeof(buf), "$p, in your bag #6, regains 1 charge of '%s'.",
            spell_info[obj->activate_spell[ACT_SPELL_SPELLNUM]].name);
          act(buf, TRUE, ch, obj, 0, TO_CHAR);
        }
      }
    }

    for (obj = ch->bags->bag7; obj; obj = obj->next_content)
    {
      if (obj->activate_spell[ACT_SPELL_SPELLNUM] > 0)
      {
        if (obj->activate_spell[ACT_SPELL_MAX_USES] > obj->activate_spell[ACT_SPELL_CURRENT_USES])
        {
          obj->activate_spell[ACT_SPELL_CURRENT_USES]++;
          snprintf(buf, sizeof(buf), "$p, in your bag #7, regains 1 charge of '%s'.",
            spell_info[obj->activate_spell[ACT_SPELL_SPELLNUM]].name);
          act(buf, TRUE, ch, obj, 0, TO_CHAR);
        }
      }
    }

    for (obj = ch->bags->bag8; obj; obj = obj->next_content)
    {
      if (obj->activate_spell[ACT_SPELL_SPELLNUM] > 0)
      {
        if (obj->activate_spell[ACT_SPELL_MAX_USES] > obj->activate_spell[ACT_SPELL_CURRENT_USES])
        {
          obj->activate_spell[ACT_SPELL_CURRENT_USES]++;
          snprintf(buf, sizeof(buf), "$p, in your bag #8, regains 1 charge of '%s'.",
            spell_info[obj->activate_spell[ACT_SPELL_SPELLNUM]].name);
          act(buf, TRUE, ch, obj, 0, TO_CHAR);
        }
      }
    }

    for (obj = ch->bags->bag9; obj; obj = obj->next_content)
    {
      if (obj->activate_spell[ACT_SPELL_SPELLNUM] > 0)
      {
        if (obj->activate_spell[ACT_SPELL_MAX_USES] > obj->activate_spell[ACT_SPELL_CURRENT_USES])
        {
          obj->activate_spell[ACT_SPELL_CURRENT_USES]++;
          snprintf(buf, sizeof(buf), "$p, in your bag #9, regains 1 charge of '%s'.",
            spell_info[obj->activate_spell[ACT_SPELL_SPELLNUM]].name);
          act(buf, TRUE, ch, obj, 0, TO_CHAR);
        }
      }
    }

    for (obj = ch->bags->bag10; obj; obj = obj->next_content)
    {
      if (obj->activate_spell[ACT_SPELL_SPELLNUM] > 0)
      {
        if (obj->activate_spell[ACT_SPELL_MAX_USES] > obj->activate_spell[ACT_SPELL_CURRENT_USES])
        {
          obj->activate_spell[ACT_SPELL_CURRENT_USES]++;
          snprintf(buf, sizeof(buf), "$p, in your bag #10, regains 1 charge of '%s'.",
            spell_info[obj->activate_spell[ACT_SPELL_SPELLNUM]].name);
          act(buf, TRUE, ch, obj, 0, TO_CHAR);
        }
      }
    }
  }
}

void update_player_misc(void)
{
  struct descriptor_data *d = NULL;
  struct char_data *ch = NULL;
  int i = 0;

  for (d = descriptor_list; d; d = d->next)
  {
    ch = d->character;
    if (!ch)
      continue;

    if (STATE(d) != CON_PLAYING)
      continue;

    save_char_pets(ch);
    affect_total(ch);

    if (GET_MISSION_COOLDOWN(ch) > 0)
      GET_MISSION_COOLDOWN(ch)--;

    if (GET_FORAGE_COOLDOWN(ch) > 0)
    {
      GET_FORAGE_COOLDOWN(ch)--;
      if (GET_FORAGE_COOLDOWN(ch) == 0)
      {
        send_to_char(ch, "You can now forage for food again.\r\n");
      }
    }

    if (GET_SCROUNGE_COOLDOWN(ch) > 0)
    {
      GET_SCROUNGE_COOLDOWN(ch)--;
      if (GET_SCROUNGE_COOLDOWN(ch) == 0)
      {
        send_to_char(ch, "You can now scrounge for supplies again.\r\n");
      }
    }

    if (IN_ROOM(ch) == 0 || IN_ROOM(ch) == NOWHERE || GET_ROOM_VNUM(IN_ROOM(ch)) == CONFIG_MORTAL_START || GET_ROOM_VNUM(IN_ROOM(ch)) == CONFIG_IMMORTAL_START)
      ;
    else
      GET_LAST_ROOM(ch) = GET_ROOM_VNUM(IN_ROOM(ch));

    if (GET_RETAINER_COOLDOWN(ch) > 0)
    {
      GET_RETAINER_COOLDOWN(ch)--;
      if (GET_RETAINER_COOLDOWN(ch) == 0)
      {
        send_to_char(ch, "You can now call your retainer again.\r\n");
      }
    }

    if (HAS_FEAT(ch, FEAT_DETECT_ALIGNMENT))
      SET_BIT_AR(AFF_FLAGS(ch), AFF_DETECT_ALIGN);

    if (!are_mission_mobs_loaded(ch))
    {
      apply_mission_rewards(ch);
      clear_mission(ch);
    }

    if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_AUTO_PREP))
    {
      for (i = 0; i < NUM_CLASSES; i++)
      {
        if (is_spellcasting_class(i))
        {
          if (CLASS_LEVEL(ch, i) > 0)
          {
            if (SPELL_PREP_QUEUE(ch, i))
            {
              begin_preparing(ch, i);
              break;
            }
          }
        }
      }
    }

    if (ch->player_specials->concussive_onslaught_duration > 0)
    {
      ch->player_specials->concussive_onslaught_duration--;
      if (ch->player_specials->concussive_onslaught_duration <= 0)
      {
        send_to_char(ch, "Your concussive onslaught ends.\r\n");
        act("Waves of concussive force stop emenating from $n.", FALSE, ch, 0, 0, TO_ROOM);
      }
    }

    if (IS_VAMPIRE(ch) && GET_SETCLOAK_TIMER(ch) > 0)
    {
      GET_SETCLOAK_TIMER(ch)--;
      if (GET_SETCLOAK_TIMER(ch) == 0)
      {
        send_to_char(ch, "You can now set your vampire cloak bonuses again. (setcloak command)\r\n");
      }
    }

    if (HAS_FEAT(ch, FEAT_EFREETI_MAGIC) && IS_EFREETI(ch) && EFREETI_MAGIC_TIMER(ch) > 0)
    {
      EFREETI_MAGIC_TIMER(ch)--;
      if (EFREETI_MAGIC_TIMER(ch) <= 0)
      {
        EFREETI_MAGIC_TIMER(ch) = 0;
        EFREETI_MAGIC_USES(ch) = EFREETI_MAGIC_USES_PER_DAY;
        send_to_char(ch, "Your efreeti magic uses have been refreshed.\r\n");
      }
    }
    if (HAS_FEAT(ch, FEAT_DRAGON_MAGIC) && IS_DRAGON(ch) && DRAGON_MAGIC_TIMER(ch) > 0)
    {
      DRAGON_MAGIC_TIMER(ch)--;
      if (DRAGON_MAGIC_TIMER(ch) <= 0)
      {
        DRAGON_MAGIC_TIMER(ch) = 0;
        DRAGON_MAGIC_USES(ch) = DRAGON_MAGIC_USES_PER_DAY;
        send_to_char(ch, "Your dragon magic uses have been refreshed.\r\n");
      }
    }
    if (HAS_FEAT(ch, FEAT_PIXIE_DUST) && IS_PIXIE(ch) && PIXIE_DUST_TIMER(ch) > 0)
    {
      PIXIE_DUST_TIMER(ch)--;
      if (PIXIE_DUST_TIMER(ch) <= 0)
      {
        PIXIE_DUST_TIMER(ch) = 0;
        PIXIE_DUST_USES(ch) = PIXIE_DUST_USES_PER_DAY(ch);
        send_to_char(ch, "Your pixie dust uses have been refreshed.\r\n");
      }
    }
    if (HAS_FEAT(ch, FEAT_LAUGHING_TOUCH) && LAUGHING_TOUCH_TIMER(ch) > 0)
    {
      LAUGHING_TOUCH_TIMER(ch)--;
      if (LAUGHING_TOUCH_TIMER(ch) <= 0)
      {
        LAUGHING_TOUCH_TIMER(ch) = 0;
        LAUGHING_TOUCH_USES(ch) = LAUGHING_TOUCH_USES_PER_DAY(ch);
        send_to_char(ch, "Your laughing touch uses have been refreshed.\r\n");
      }
    }
    if (HAS_FEAT(ch, FEAT_FLEETING_GLANCE) && FLEETING_GLANCE_TIMER(ch) > 0)
    {
      FLEETING_GLANCE_TIMER(ch)--;
      if (FLEETING_GLANCE_TIMER(ch) <= 0)
      {
        FLEETING_GLANCE_TIMER(ch) = 0;
        FLEETING_GLANCE_USES(ch) = FLEETING_GLANCE_USES_PER_DAY;
        send_to_char(ch, "Your fleeting glance uses have been refreshed.\r\n");
      }
    }
    if (HAS_FEAT(ch, FEAT_SOUL_OF_THE_FEY) && FEY_SHADOW_WALK_TIMER(ch) > 0)
    {
      FEY_SHADOW_WALK_TIMER(ch)--;
      if (FEY_SHADOW_WALK_TIMER(ch) <= 0)
      {
        FEY_SHADOW_WALK_TIMER(ch) = 0;
        FEY_SHADOW_WALK_USES(ch) = FEY_SHADOW_WALK_USES_PER_DAY;
        send_to_char(ch, "Your fey shadow walk uses have been refreshed.\r\n");
      }
    }
    if (HAS_FEAT(ch, FEAT_GRAVE_TOUCH) && GRAVE_TOUCH_TIMER(ch) > 0)
    {
      GRAVE_TOUCH_TIMER(ch)--;
      if (GRAVE_TOUCH_TIMER(ch) <= 0)
      {
        GRAVE_TOUCH_TIMER(ch) = 0;
        GRAVE_TOUCH_USES(ch) = GRAVE_TOUCH_USES_PER_DAY(ch);
        send_to_char(ch, "Your grave touch uses have been refreshed.\r\n");
      }
    }
    if (HAS_FEAT(ch, FEAT_GRASP_OF_THE_DEAD) && GRASP_OF_THE_DEAD_TIMER(ch) > 0)
    {
      GRASP_OF_THE_DEAD_TIMER(ch)--;
      if (GRASP_OF_THE_DEAD_TIMER(ch) <= 0)
      {
        GRASP_OF_THE_DEAD_TIMER(ch) = 0;
        GRASP_OF_THE_DEAD_USES(ch) = GRASP_OF_THE_DEAD_USES_PER_DAY(ch);
        send_to_char(ch, "Your grasp of the dead uses have been refreshed.\r\n");
      }
    }
    if (HAS_FEAT(ch, FEAT_INCORPOREAL_FORM) && INCORPOREAL_FORM_TIMER(ch) > 0)
    {
      INCORPOREAL_FORM_TIMER(ch)--;
      if (INCORPOREAL_FORM_TIMER(ch) <= 0)
      {
        INCORPOREAL_FORM_TIMER(ch) = 0;
        INCORPOREAL_FORM_USES(ch) = INCORPOREAL_FORM_USES_PER_DAY(ch);
        send_to_char(ch, "Your incorporeal form (undead bloodline) uses have been refreshed.\r\n");
      }
    }

    if (GET_MARK(ch) && GET_MARK_ROUNDS(ch) < 3)
    {
      GET_MARK_ROUNDS(ch) += 1;
      if (GET_MARK_ROUNDS(ch) == 3 || HAS_FEAT(ch, FEAT_ANGEL_OF_DEATH))
      {
        send_to_char(ch, "You have finished marking your target.\r\n");
      }
      else
        send_to_char(ch, "You continue to mark your target.\r\n");
    }
  }
}

// every 6 seconds
void proc_d20_round(void)
{

  struct char_data *i = NULL, *tch = NULL;
  struct raff_node *raff, *next_raff;
  struct affected_type af;
  int x = 0;

  for (i = character_list; i; i = i->next)
  {

    if (GET_KAPAK_SALIVA_HEALING_COOLDOWN(i) > 0)
    {
      GET_KAPAK_SALIVA_HEALING_COOLDOWN(i)--;
      if (GET_KAPAK_SALIVA_HEALING_COOLDOWN(i) == 0)
      {
        send_to_char(i, "You can now be healed with kapak saliva again.\r\n");
      }
    }

    if (i->char_specials.terror_cooldown > 0)
    {
      i->char_specials.terror_cooldown--;
      if (i->char_specials.terror_cooldown == 0)
      {
        send_to_char(i, "You are no longer immune to auras of terror.\r\n");
      }
    }

    if (GET_PUSHED_TIMER(i) > 0)
    {
      GET_PUSHED_TIMER(i)--;
    }
    if (GET_SICKENING_AURA_TIMER(i) > 0)
    {
      GET_SICKENING_AURA_TIMER(i)--;
    }
    if (GET_FRIGHTFUL_PRESENCE_TIMER(i) > 0)
    {
      GET_FRIGHTFUL_PRESENCE_TIMER(i)--;
    }
    if (CALL_EIDOLON_COOLDOWN(i) > 0)
    {
      CALL_EIDOLON_COOLDOWN(i)--;
      if (CALL_EIDOLON_COOLDOWN(i) <= 0)
      {
        send_to_char(i, "You can now summon your eidolon again.\r\n");
      }
    }
    if (MERGE_FORMS_TIMER(i) > 0)
    {
      MERGE_FORMS_TIMER(i)--;
      if (MERGE_FORMS_TIMER(i) <= 0)
      {
        act("Your eidolon's form departs from your own.", FALSE, i, 0, 0, TO_CHAR);
        for (x = 0; x < NUM_EVOLUTIONS; x++)
        {
          if (HAS_TEMP_EVOLUTION(i, x))
            HAS_TEMP_EVOLUTION(i, x) = 0;
        }
      }
    }

    if (i->char_specials.swindle_cooldown > 0)
      i->char_specials.swindle_cooldown--;
    if (i->char_specials.entertain_cooldown > 0)
      i->char_specials.entertain_cooldown--;
    if (i->char_specials.tribute_cooldown > 0)
      i->char_specials.tribute_cooldown--;
      
    if (i->char_specials.recently_slammed > 0)
      i->char_specials.recently_slammed--;
    if (i->char_specials.recently_kicked > 0)
      i->char_specials.recently_kicked--;

    if (AFF_FLAGGED(i, AFF_WIND_WALL))
    {
      if (IN_ROOM(i) != NOWHERE)
      {
        for (raff = raff_list; raff; raff = next_raff)
        {
          next_raff = raff->next;

          if (raff->room == IN_ROOM(i))
          {
            if (raff->affection == RAFF_OBSCURING_MIST)
            {
              rem_room_aff(raff);
              act("Your wall of wind dissipates the obscuring mist.", FALSE, i, 0, 0, TO_CHAR);
              act("$n's wall of wind dissipates the obscuring mist.", FALSE, i, 0, 0, TO_ROOM);
            }
            else if (raff->affection == RAFF_ACID_FOG)
            {
              rem_room_aff(raff);
              act("Your wall of wind dissipates the acid fog.", FALSE, i, 0, 0, TO_CHAR);
              act("$n's wall of wind dissipates the acid fog.", FALSE, i, 0, 0, TO_ROOM);
            }
            else if (raff->affection == RAFF_BILLOWING)
            {
              rem_room_aff(raff);
              act("Your wall of wind dissipates the billowing cloud.", FALSE, i, 0, 0, TO_CHAR);
              act("$n's wall of wind dissipates the billowing cloud.", FALSE, i, 0, 0, TO_ROOM);
            }
            else if (raff->affection == RAFF_STINK)
            {
              rem_room_aff(raff);
              act("Your wall of wind dissipates the stinking cloud.", FALSE, i, 0, 0, TO_CHAR);
              act("$n's wall of wind dissipates the stinking cloud.", FALSE, i, 0, 0, TO_ROOM);
            }
            else if (raff->affection == RAFF_FOG)
            {
              rem_room_aff(raff);
              act("Your wall of wind dissipates the wall of fog.", FALSE, i, 0, 0, TO_CHAR);
              act("$n's wall of wind dissipates the wall of fog.", FALSE, i, 0, 0, TO_ROOM);
            }
          }
        }
      }
      if (GET_SICKENING_AURA_TIMER(i) <= 0)
      for (tch = world[IN_ROOM(i)].people; tch; tch = tch->next_in_room)
      {
        if (AFF_FLAGGED(tch, AFF_SICKENING_AURA) && aoeOK(tch, i, EVOLUTION_SICKENING_EFFECT))
        {
          if (mag_savingthrow(tch, i, SAVING_FORT, 0, CAST_INNATE, GET_CALL_EIDOLON_LEVEL(tch), NOSCHOOL))
          {
            act("$N is unaffected by your sickening aura.", TRUE, tch, 0, i, TO_CHAR);
            act("You are unaffected by $n's sickening aura.", TRUE, tch, 0, i, TO_VICT);
            act("$N is unaffected by $n's sickening aura.", TRUE, tch, 0, i, TO_NOTVICT);
          }
          else
          {
            act("$N succumbs to your sickening aura.", TRUE, tch, 0, i, TO_CHAR);
            act("You succumb to $n's sickening aura.", TRUE, tch, 0, i, TO_VICT);
            act("$N succumbs to $n's sickening aura.", TRUE, tch, 0, i, TO_NOTVICT);

            new_affect(&af);
            af.spell = EVOLUTION_SICKENING_EFFECT;
            af.location = APPLY_CON;
            af.modifier = -2;
            af.duration = 1;
            SET_BIT_AR(af.bitvector, AFF_SICKENED);
            affect_to_char(i, &af);
          }
          GET_SICKENING_AURA_TIMER(i) = 10;
        }
      }
    }

    if (!IS_NPC(i)) // players only
    {
    }
    else // mobs only
    {
      if (MOB_FLAGGED(i, MOB_HUNTS_TARGET))
      {
        if (i->mob_specials.hunt_cooldown > 0)
        {
          i->mob_specials.hunt_cooldown--;
          if (i->mob_specials.hunt_cooldown == 0)
          {
            extract_char(i);
          }
        }
      }
      if (MOB_FLAGGED(i, MOB_ENCOUNTER))
      {

        if (i->mob_specials.extract_timer > 0)
        {
          i->mob_specials.extract_timer--;
          if (i->mob_specials.extract_timer == 0)
          {
            extract_char(i);
          }
        }

        if (i->mob_specials.peaceful_timer > 0)
        {
          i->mob_specials.peaceful_timer--;
          if (i->mob_specials.peaceful_timer == 0)
          {
            i->mob_specials.peaceful_timer = -1;
            act("$n is no longer peaceful and will have to be dealt with again in some manner. (HELP ENCOUNTERS)\r\n", false, i, 0, 0, TO_ROOM);
          }
        }

        if (!FIGHTING(i) && i->mob_specials.aggro_timer > 0 && i->mob_specials.peaceful_timer == -1)
        {
          switch (i->mob_specials.aggro_timer)
          {
          case 5:
            act("\tR$n looks very hostile towards you.\tn", true, i, 0, 0, TO_ROOM);
            break;
          case 4:
            act("\tR$n seems to be getting even more hostile.\tN", true, i, 0, 0, TO_ROOM);
            break;
          case 3:
            act("\tR$n looks to be losing $s patience.\tN", true, i, 0, 0, TO_ROOM);
            break;
          case 2:
            act("\tR$n looks like $e may attack you.\tN", true, i, 0, 0, TO_ROOM);
            break;
          case 1:
            act("\tR$n is preparing to attack you.\tN", true, i, 0, 0, TO_ROOM);
            break;
          }
          i->mob_specials.aggro_timer--;
          if (i->mob_specials.aggro_timer == 0)
          {
            SET_BIT_AR(MOB_FLAGS(i), MOB_AGGRESSIVE);
            REMOVE_BIT_AR(MOB_FLAGS(i), MOB_HELPER); // helper and aggro flags conflict
          }
        }
      }
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
    set_db_happy_hour(2);
  }

  /* this is the staff event code for regular maintenance */
  staff_event_tick();

  /* end general */

  /** characters **/
  for (i = character_list; i; i = next_char)
  {
    next_char = i->next;

    gain_condition(i, HUNGER, -1);
    gain_condition(i, DRUNK, -1);
    gain_condition(i, THIRST, -1);

    /* old tick regen code use to be here -zusuk */

    if (!IS_NPC(i)) // players only
    {
      update_char_objects(i);
      (i->char_specials.timer)++;
      if (GET_LEVEL(i) < CONFIG_IDLE_MAX_LEVEL)
        check_idling(i);
      // eldritch knight spell crit expires after combat ends if not used.
      if (!FIGHTING(i) && HAS_ELDRITCH_SPELL_CRIT(i))
        HAS_ELDRITCH_SPELL_CRIT(i) = false;
    }
    else // mobs only
    {
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

        if (GET_OBJ_SPECTIMER(j, counter) <= 0)
        {
          /*obj timer is back to 0*/

          if (j->carried_by) /* carried in your inventory */
            act("$p briefly flares as the imbued magic returns.", FALSE, j->carried_by, j, 0, TO_CHAR);
          else if (j->in_obj && j->in_obj->carried_by) /* object carrying the missile */
            act("$p briefly flares as the imbued magic returns.", FALSE, j->in_obj->carried_by, j, 0, TO_CHAR);
          else if (j->worn_by)
            act("$p briefly flares as the imbued magic returns.", FALSE, j->worn_by, j, 0, TO_CHAR);
        }
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
        if (j)
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

// function that performs the "meat" of the vampiric blood drain mechanic!
void vamp_blood_drain(struct char_data *ch, struct char_data *vict)
{

  struct affected_type af;
  
  // struct affected_type *af2;

  // for (af2 = ch->affected; af2; af2 = af2->next)
  // {
  //   if (af2->spell == ABILITY_BLOOD_DRAIN)
  //   {
  //     af2->duration--;
  //     if (af2->duration <= 0)
  //     {
  //       affect_from_char(ch, ABILITY_BLOOD_DRAIN);
  //       send_to_char(ch, "You finish feasting on the blood of your opponent.\r\n");
  //       break;
  //     }
  //   }
  // }

  if (!ch || !vict) return;
  if (IN_ROOM(ch) == NOWHERE || IN_ROOM(vict) == NOWHERE) return;

  if (IN_SUNLIGHT(ch) || IN_MOVING_WATER(ch))
  {
    send_to_char(ch, "You cannot drain blood in sunlight or moving water, even if wearing a vampire cloak.\r\n");
    return;
  }

  if (!can_blood_drain_target(ch, vict))
  {
    
    return;
  }

  act("You lean into $N's neck and drain the blood from $S body.", FALSE, ch, 0, vict, TO_CHAR);
  act("$n leans into your neck and drains the blood from your body.", FALSE, ch, 0, vict, TO_VICT);
  act("$n leans into $N's neck and drains the blood from $S body.", FALSE, ch, 0, vict, TO_NOTVICT);

  if (!IS_NPC(ch))
  {
    TIME_SINCE_LAST_FEEDING(ch) -= 25;

    if (TIME_SINCE_LAST_FEEDING(ch) < 0)
      TIME_SINCE_LAST_FEEDING(ch) = 0;
  }

  if (vict && GET_CON(vict) > 0)
  {
    if (!mag_savingthrow(ch, vict, SAVING_FORT, 0, CAST_INNATE, GET_LEVEL(ch), NECROMANCY))
    {
      new_affect(&af);
      af.spell = ABILITY_SCORE_DAMAGE;
      af.location = APPLY_CON;
      af.modifier = -dice(1, 4);
      af.duration = 50; // approx five minutes
      if ((GET_CON(vict) + af.modifier) < 0) // we're adding a negative number so it's + not -
        af.modifier = GET_CON(vict);
      affect_join(vict, &af, FALSE, FALSE, TRUE, FALSE);
      act("You drain some of $N's constitution.", FALSE, ch, 0, vict, TO_CHAR);
      act("You feel your constitution being drained.", FALSE, ch, 0, vict, TO_VICT);
    }
  }

  GET_HIT(ch) += 5;
  GET_HIT(ch) = MIN(GET_MAX_HIT(ch) * 2, GET_HIT(ch));
  act("The blood bolsters your strength.", FALSE, ch, 0, vict, TO_CHAR);

  // damage goes last in case it kills the vict, preventing potential
  // crashes from a now, non-existent vict.
  damage(ch, vict, 5, ABILITY_BLOOD_DRAIN, DAM_BLOOD_DRAIN, FALSE);

  return;
}

void update_damage_and_effects_over_time(void)
{
  int dam = 0, x = 0;
  struct affected_type *affects = NULL;
  struct char_data *ch = NULL, *next_char = NULL;
  char buf[MAX_STRING_LENGTH] = {'\0'};

  for (ch = character_list; ch; ch = next_char)
  {
    next_char = ch->next;

    /* dummy check */
    if (!ch)
      return;

    if (HAS_EVOLUTION(ch, EVOLUTION_GILLS))
      SET_BIT_AR(AFF_FLAGS(ch), AFF_WATER_BREATH);

    // Disabled as causes issues with different things, such as wildshape
    // This code handles ability score damage which can be healed with various 'restoration' spells
    // if (GET_STR(ch) <= 0 || GET_DEX(ch) <= 0 || GET_INT(ch) <= 0 || GET_WIS(ch) <= 0 || 
    //     GET_CHA(ch) <= 0 || GET_CON(ch) <= 0)
    // {
    //   struct affected_type af;
    //   new_affect(&af);
    //   af.spell = ABILITY_SCORE_DAMAGE;
    //   af.duration = 5;
    //   SET_BIT_AR(af.bitvector, AFF_PARALYZED);
    //   affect_to_char(ch, &af);

    //   if (GET_STR(ch) <= 0)
    //     act("Your strength has sapped completely, rendering you immoble.", FALSE, ch, 0, 0, TO_CHAR);
    //   if (GET_CON(ch) <= 0)
    //     act("Your constitution has sapped completely, rendering you immoble.", FALSE, ch, 0, 0, TO_CHAR);
    //   if (GET_DEX(ch) <= 0)
    //     act("Your dexterity has sapped completely, rendering you immoble.", FALSE, ch, 0, 0, TO_CHAR);
    //   if (GET_INT(ch) <= 0)
    //     act("Your intelligence has sapped completely, rendering you immoble.", FALSE, ch, 0, 0, TO_CHAR);
    //   if (GET_WIS(ch) <= 0)
    //     act("Your wisdom has sapped completely, rendering you immoble.", FALSE, ch, 0, 0, TO_CHAR);
    //   if (GET_CHA(ch) <= 0)
    //     act("Your charisma has sapped completely, rendering you immoble.", FALSE, ch, 0, 0, TO_CHAR);

    //   act("$n collapses into a helpless heap, looking completely drained.", TRUE, ch, 0, 0, TO_ROOM);
    // }

    if (GET_NODAZE_COOLDOWN(ch) > 0)
    {
      GET_NODAZE_COOLDOWN(ch)--;
    }

    if (affected_by_spell(ch, ABILITY_BLOOD_DRAIN))
    {
      vamp_blood_drain(ch, FIGHTING(ch));
    }

    if (IS_VAMPIRE(ch) && TIME_SINCE_LAST_FEEDING(ch) <= 100)
    {
      TIME_SINCE_LAST_FEEDING(ch)++;
    }

    if (AFF_FLAGGED(ch, AFF_ON_FIRE))
    {
      damage(ch, ch, dice(2, 6), TYPE_ON_FIRE, DAM_FIRE, FALSE);
    }

    // set this to false every round so banishing blade can be attempted again
    if (ch->char_specials.banishing_blade_procced_this_round)
      ch->char_specials.banishing_blade_procced_this_round = FALSE;

    if (HAS_FEAT(ch, FEAT_VAMPIRE_WEAKNESSES) && GET_LEVEL(ch) < LVL_IMMORT &&
        !affected_by_spell(ch, AFFECT_RECENTLY_DIED) && !affected_by_spell(ch, AFFECT_RECENTLY_RESPECED))
    {
      if (IN_SUNLIGHT(ch) && !is_covered(ch))
      {
        damage(ch, ch, dice(2, 6), TYPE_SUN_DAMAGE, DAM_SUNLIGHT, FALSE);
      }
      if (IN_MOVING_WATER(ch))
      {
        damage(ch, ch, GET_MAX_HIT(ch) / 3, TYPE_MOVING_WATER, DAM_WATER, FALSE);
      }
    }

    for (x = 0; x < NUM_ELDRITCH_BLAST_COOLDOWNS; x++)
    {
      if (ch->char_specials.eldritch_blast_cooldowns[x] > 0)
      {
        ch->char_specials.eldritch_blast_cooldowns[x]--;
      }
    }

    if (AFF_FLAGGED(ch, AFF_BLEED))
    {
      for (affects = ch->affected; affects; affects = affects->next)
      {
        if (IS_SET_AR(affects->bitvector, AFF_BLEED))
        {
          dam = damage(ch, ch, affects->modifier, TYPE_SUFFERING, DAM_BLEEDING, TYPE_SPECAB_BLEEDING);

          if (dam <= 0)
          { /* they died */
            break;
          }
        }
      }
    }

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

    if (affected_by_spell(ch, AFFECT_CAUSTIC_BLOOD_DAMAGE))
    {
      for (affects = ch->affected; affects; affects = affects->next)
      {
        if (affects->spell == AFFECT_CAUSTIC_BLOOD_DAMAGE)
        {
          dam = damage(ch, ch, dice(affects->modifier, 6), AFFECT_CAUSTIC_BLOOD_DAMAGE, DAM_ACID, 0);

          if (dam <= 0)
          { /* they died */
            break;
          }
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

    if (GET_STICKY_BOMB(ch, 0) != BOMB_NONE)
    {
      if (GET_STICKY_BOMB(ch, 0) != BOMB_FIRE_BRAND && GET_STICKY_BOMB(ch, 0) != BOMB_HEALING)
      {
        snprintf(buf, sizeof(buf), "A sticky %s bomb explodes again causing you %s damage.", bomb_types[GET_STICKY_BOMB(ch, 0)], damtypes[GET_STICKY_BOMB(ch, 1)]);
        act(buf, FALSE, ch, 0, 0, TO_CHAR);
        snprintf(buf, sizeof(buf), "A sticky %s bomb explodes on $n again causing $m %s damage.", bomb_types[GET_STICKY_BOMB(ch, 0)], damtypes[GET_STICKY_BOMB(ch, 1)]);
        act(buf, FALSE, ch, 0, 0, TO_ROOM);
        dam = damage(ch, ch, GET_STICKY_BOMB(ch, 2), SKILL_BOMB_TOSS, GET_STICKY_BOMB(ch, 1), SKILL_BOMB_TOSS);
        GET_STICKY_BOMB(ch, 0) = GET_STICKY_BOMB(ch, 1) = GET_STICKY_BOMB(ch, 2) = 0;
      }
      else if (GET_STICKY_BOMB(ch, 0) == BOMB_HEALING)
      {
        snprintf(buf, sizeof(buf), "A sticky %s bomb explodes again, healing you for more.", bomb_types[GET_STICKY_BOMB(ch, 0)]);
        act(buf, FALSE, ch, 0, 0, TO_CHAR);
        snprintf(buf, sizeof(buf), "A sticky %s bomb explodes again, healing $n for more.", bomb_types[GET_STICKY_BOMB(ch, 0)]);
        act(buf, FALSE, ch, 0, 0, TO_ROOM);
        perform_bomb_direct_healing(ch, ch, BOMB_HEALING);
        GET_STICKY_BOMB(ch, 0) = GET_STICKY_BOMB(ch, 1) = GET_STICKY_BOMB(ch, 2) = 0;
      }
      else if (GET_STICKY_BOMB(ch, 0) == BOMB_FIRE_BRAND)
      {
        snprintf(buf, sizeof(buf), "A sticky %s bomb explodes again, setting your weapons aflame anew.", bomb_types[GET_STICKY_BOMB(ch, 0)]);
        act(buf, FALSE, ch, 0, 0, TO_CHAR);
        snprintf(buf, sizeof(buf), "A sticky %s bomb explodes again, setting $n's weapons aflame anew.", bomb_types[GET_STICKY_BOMB(ch, 0)]);
        act(buf, FALSE, ch, 0, 0, TO_ROOM);
        perform_bomb_self_effect(ch, ch, BOMB_FIRE_BRAND);
        GET_STICKY_BOMB(ch, 0) = GET_STICKY_BOMB(ch, 1) = GET_STICKY_BOMB(ch, 2) = 0;
      }

    } // sticky bomb effects

    // fast healing grand discovery affect
    if (GET_GRAND_DISCOVERY(ch) == GR_ALC_DISC_FAST_HEALING && GET_HIT(ch) < GET_MAX_HIT(ch))
    {
      GET_HIT(ch) += 5;
      if (GET_HIT(ch) > GET_MAX_HIT(ch))
        GET_HIT(ch)--;
    }

    // judgement of healing
    if (is_judgement_possible(ch, FIGHTING(ch), INQ_JUDGEMENT_HEALING) && !ch->player.exploit_weaknesses && GET_HIT(ch) < GET_MAX_HIT(ch))
      GET_HIT(ch) += get_judgement_bonus(ch, INQ_JUDGEMENT_HEALING);
    if (GET_HIT(ch) > GET_MAX_HIT(ch))
      GET_HIT(ch)--;

    // paladin fast healing mercy effect
    if (affected_by_spell(ch, PALADIN_MERCY_INJURED_FAST_HEALING) && GET_HIT(ch) < GET_MAX_HIT(ch))
    {
      GET_HIT(ch) += get_char_affect_modifier(ch, PALADIN_MERCY_INJURED_FAST_HEALING, APPLY_SPECIAL);
      if (GET_HIT(ch) > GET_MAX_HIT(ch))
        GET_HIT(ch)--;
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

void check_auto_happy_hour(void)
{
  if (IS_HAPPYHOUR)
    return;

  time_t mytime;
  int m;

  mytime = time(0);

  m = (mytime / 60) % 60;

  if (m == 0)
  {
    if (rand_number(1, 100) <= CONFIG_HAPPY_HOUR_CHANCE)
    {
      HAPPY_EXP = CONFIG_HAPPY_HOUR_EXP;
      HAPPY_GOLD = CONFIG_HAPPY_HOUR_GOLD;
      HAPPY_QP = CONFIG_HAPPY_HOUR_QP;
      HAPPY_TREASURE = CONFIG_HAPPY_HOUR_TREASURE;
      HAPPY_TIME = 47;

      game_info("An automated happy hour has started!");
      set_db_happy_hour(1);
    }
  }
}

void self_buffing(void)
{
  struct char_data *ch = NULL;
  struct descriptor_data *d = NULL;
  int is_spell = false;
  int spellnum = 0, i = 0;
  char spellname[200];
  char buf1[MAX_STRING_LENGTH] = {'\0'};
  char buf2[MAX_STRING_LENGTH] = {'\0'};

  for (d = descriptor_list; d; d = d->next)
  {
    ch = d->character;

    if (!ch)
      continue;

    if (!IS_BUFFING(ch))
      continue;

    if (GET_FIGHT_TO_THE_DEATH_COOLDOWN(ch) > 0)
    {
      GET_FIGHT_TO_THE_DEATH_COOLDOWN(ch)--;
      if (GET_FIGHT_TO_THE_DEATH_COOLDOWN(ch) == 0)
      {
        send_to_char(ch, "You can now fight to the death again.\r\n");
      }
    }

    while (GET_BUFF(ch, GET_CURRENT_BUFF_SLOT(ch), 0) == 0 && GET_CURRENT_BUFF_SLOT(ch) < (MAX_BUFFS + 1))
    {
      GET_CURRENT_BUFF_SLOT(ch)++;
    }
    if (GET_CURRENT_BUFF_SLOT(ch) >= MAX_BUFFS)
    {
      send_to_char(ch, "You finish buffing yourself.\r\n");
      GET_CURRENT_BUFF_SLOT(ch) = 0;
      GET_BUFF_TIMER(ch) = 0;
      IS_BUFFING(ch) = false;
      affect_from_char(ch, SPELL_MINOR_RAPID_BUFF);
      affect_from_char(ch, SPELL_RAPID_BUFF);
      affect_from_char(ch, SPELL_GREATER_RAPID_BUFF);
    }

    if (GET_BUFF_TIMER(ch) > 0)
    {
      if (--GET_BUFF_TIMER(ch) == 0)
      {
        spellnum = GET_BUFF(ch, GET_CURRENT_BUFF_SLOT(ch), 0);
        is_spell = is_spell_or_power(spellnum);
        send_to_char(ch, "You continue buffing... (buff cancel to stop)\r\n");

        if (IS_AFFECTED(ch, AFF_TIME_STOPPED))
          GET_BUFF_TIMER(ch) = 1;
        else if (IS_AFFECTED(ch, AFF_RAPID_BUFF))
          GET_BUFF_TIMER(ch) = 1;
        else
        {
#if defined(CAMPAIGN_FR) || defined(CAMPAIGN_DL)
          if (spell_info[spellnum].ritual_spell)
            GET_BUFF_TIMER(ch) = MAX(6, spell_info[spellnum].time + 1);
          else
            GET_BUFF_TIMER(ch) = 6;
#else
          GET_BUFF_TIMER(ch) = spell_info[spellnum].time + 1;
#endif
        }

        if (is_spell >= 2) // spell or warlock power
        {
          snprintf(spellname, sizeof(spellname), " '%s'", spell_info[spellnum].name);
          if (GET_BUFF_TARGET(ch) && IN_ROOM(GET_BUFF_TARGET(ch)) == IN_ROOM(ch))
          {
            snprintf(buf1, sizeof(buf1), "%s", GET_NAME(GET_BUFF_TARGET(ch)));
            for (i = 0; i < strlen(buf1); i++)
              if (buf1[i] == ' ')
                buf1[i] = '-';
            snprintf(buf2, sizeof(buf2), "%s %s", spellname, buf1);
            do_gen_cast(ch, (const char *)buf2, 0, SCMD_CAST_SPELL);
          }
          else
          {
            do_gen_cast(ch, (const char *)spellname, 0, SCMD_CAST_SPELL);
          }
        }
        else
        {
          int augment = 0;
          if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_AUGMENT_BUFFS))
            augment = max_augment_psp_allowed(ch, spellnum);
          snprintf(spellname, sizeof(spellname), " %d '%s'", augment, spell_info[spellnum].name);
          do_manifest(ch, (const char *)spellname, 0, SCMD_CAST_PSIONIC);
        }

        GET_CURRENT_BUFF_SLOT(ch)++;
      }
    }
    else
    {
      GET_CURRENT_BUFF_SLOT(ch) = 0;
      GET_BUFF_TIMER(ch) = 0;
      IS_BUFFING(ch) = false;
    }
  }
}