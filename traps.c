/****************************************************************************
 *  Realms of Luminari
 *  File:     traps.c
 *  Usage:    system for traps!
 *  Header:   traps.h
 *  Authors:  Homeland (ported to Luminari by Zusuk)
 ****************************************************************************/
#ifndef __TRAPS_C__
#define __TRAPS_C__

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "handler.h"

#include "mud_event.h"
#include "actions.h"
#include "mudlim.h"

#include "fight.h"
#include "spells.h"

#include "traps.h"

/* A trap in this version of the code is a special item that is loaded into
   the room that will react to either entering rooms, opening doors, or
   grabbing stuff from a container, etc */

/* side note, if the GET_OBJ_RENT(trap) > 0, then the trap is detected in the original version */
/* object value (0) is the trap-type */
/* object value (1) is the direction of the trap (TRAP_TYPE_OPEN_DOOR and TRAP_TYPE_UNLOCK_DOOR)
     or the object-vnum (TRAP_TYPE_OPEN_CONTAINER and TRAP_TYPE_UNLOCK_CONTAINER and TRAP_TYPE_GET_OBJECT) */
/* object value (2) is the effect */
/* object value (3) is the trap difficulty */
/* object value (4) is whether this trap has been "detected" yet */

/* start code */

/* this function is ran to set off a trap, it creates and attaches the
 event to the victim*/
void set_off_trap(struct char_data *ch, struct obj_data *trap)
{
  char buf[128];

  if (IS_NPC(ch) && !IS_PET(ch))
    return;

  send_to_char(ch, "Ooops, you must have triggered something.\r\n");

  /* Build the effect string */
  snprintf(buf, sizeof(buf), "%d", GET_OBJ_VAL(trap, 2));

  /* Add the event to the character.*/
  NEW_EVENT(eTRAPTRIGGERED, ch, strdup(buf), 1);
}

/* checks the 5th value (4) to see if its set (which indicates detection) */
bool is_trap_detected(struct obj_data *trap)
{
  return (GET_OBJ_VAL(trap, 4) > 0);
}

/* set the 5th value (4) to indicate it is detected */
void set_trap_detected(struct obj_data *trap)
{
  GET_OBJ_VAL(trap, 4) = 1;
}

/* based on trap-type, see if it should fire or not! */
bool check_trap(struct char_data *ch, int trap_type, int room, struct obj_data *obj, int dir)
{
  struct obj_data *trap = NULL;

  /* check the room for any traps */
  for (trap = world[room].contents; trap; trap = trap->next_content)
  {

    /* is this a trap? */
    if (GET_OBJ_TYPE(trap) == ITEM_TRAP && GET_OBJ_VAL(trap, 0) == trap_type)
    {
      switch (trap_type)
      {
      case TRAP_TYPE_ENTER_ROOM:
        break;
      case TRAP_TYPE_LEAVE_ROOM:
        break;
      case TRAP_TYPE_OPEN_DOOR:
      case TRAP_TYPE_UNLOCK_DOOR:
        if (dir != GET_OBJ_VAL(trap, 1))
          continue;
        break;
      case TRAP_TYPE_OPEN_CONTAINER:
      case TRAP_TYPE_UNLOCK_CONTAINER:
      case TRAP_TYPE_GET_OBJECT:
        if (GET_OBJ_VNUM(obj) != GET_OBJ_VAL(trap, 1))
          continue;
        break;
      }

      /* bingo!  FIRE! */
      set_off_trap(ch, trap);

      return TRUE;
    }
  }
  return FALSE;
}

ACMDC(do_disabletrap)
{
  struct obj_data *trap = NULL;
  int result = 0, exp = 1, dc = 0;

  if (!GET_ABILITY(ch, ABILITY_DISABLE_DEVICE))
  {
    send_to_char(ch, "But you do not know how.\r\n");
    return;
  }

  for (trap = world[ch->in_room].contents; trap; trap = trap->next_content)
  {
    if (GET_OBJ_TYPE(trap) == ITEM_TRAP && is_trap_detected(trap))
    {
      act("$n is trying to disable a trap...", FALSE, ch, 0, 0, TO_ROOM);
      act("You try to disable the trap...", FALSE, ch, 0, 0, TO_CHAR);
      dc = GET_OBJ_VAL(trap, 3);
      if ((result = skill_check(ch, ABILITY_DISABLE_DEVICE, dc)))
      {
        act("...and is successful!", FALSE, ch, 0, 0, TO_ROOM);
        act("...and are successful!", FALSE, ch, 0, 0, TO_CHAR);
        exp = dc * dc * 100;
        send_to_char(ch, "You receive %d experience points.\r\n", gain_exp(ch, exp, GAIN_EXP_MODE_TRAP));
        extract_obj(trap);
      }
      else
      {
        act("...but fails.", FALSE, ch, 0, 0, TO_ROOM);
        act("...but fail.", FALSE, ch, 0, 0, TO_CHAR);
        if (result <= -5) /* fail your roll by 5 or more, BOOM! */
          set_off_trap(ch, trap);
      }
      USE_FULL_ROUND_ACTION(ch);
      return;
    }
  }
  send_to_char(ch, "But there are no traps here than you can see.\r\n");
}

/* engine for detecting traps, extracted it for trap-sense feat */

/* included a "silent" mode for the trap-sense feat */
int perform_detecttrap(struct char_data *ch, bool silent)
{
  struct obj_data *trap = NULL;
  int exp = 1, dc = 0;

  if (!silent)
  {
    USE_FULL_ROUND_ACTION(ch);
  }

  for (trap = world[ch->in_room].contents; trap; trap = trap->next_content)
  {
    if (GET_OBJ_TYPE(trap) == ITEM_TRAP && !is_trap_detected(trap))
    {
      dc = GET_OBJ_VAL(trap, 3);
      if (!IS_NPC(ch) && HAS_FEAT(ch, FEAT_TRAPFINDING))
        dc -= 4;
      if (skill_check(ch, ABILITY_PERCEPTION, dc))
      {
        act("$n has detected a \tRtrap\tn!", FALSE, ch, 0, 0, TO_ROOM);
        act("You have detected a \tRtrap\tn!", FALSE, ch, 0, 0, TO_CHAR);
        set_trap_detected(trap);
        exp = dc * 100;
        send_to_char(ch, "You receive %d experience points.\r\n", gain_exp(ch, exp, GAIN_EXP_MODE_TRAP));
        return 1;
      }
    }
  }

  if (!silent)
  {
    act("$n is looking around for some traps, but can not find any.", FALSE, ch, 0, 0, TO_ROOM);
    act("You do not seem to detect any traps.", FALSE, ch, 0, 0, TO_CHAR);
  }

  return 0;
}

ACMDC(do_detecttrap)
{

  if (!GET_ABILITY(ch, ABILITY_PERCEPTION))
  {
    send_to_char(ch, "But you do not know how.\r\n");
    return;
  }

  perform_detecttrap(ch, FALSE);
}

/* a reminder: int call_magic(struct char_data *caster, struct char_data *cvict,
        struct obj_data *ovict, int spellnum, int level, int casttype);
 another reminder: int damage(struct char_data *ch, struct char_data *victim,
     int dam, int attacktype, int dam_type, int dualwield);
 another reminder: #define ASPELL(spellname) \
void	spellname(int level, struct char_data *ch, \
            struct char_data *victim, struct obj_data *obj)
 *  */
EVENTFUNC(event_trap_triggered)
{
  struct mud_event_data *pMudEvent = NULL;
  struct char_data *ch = NULL;
  struct room_data *room = NULL;
  room_vnum *rvnum;
  room_rnum rnum = NOWHERE;

  /* Non-event related variables.*/
  int effect;
  int dam_type = DAM_FORCE;
  struct affected_type af;
  const char *to_char = NULL;
  const char *to_room = NULL;
  int dam = 0;
  int count = 0;
  int i;
  int casttype = CAST_TRAP;
  int level = (LVL_STAFF - 1);

  pMudEvent = (struct mud_event_data *)event_obj;

  if (!pMudEvent)
    return 0;

  if (!pMudEvent->iId)
    return 0;

  switch (mud_event_index[pMudEvent->iId].iEvent_Type)
  {
  case EVENT_CHAR:
    ch = (struct char_data *)pMudEvent->pStruct;
    break;
  case EVENT_ROOM:
    rvnum = (room_vnum *)pMudEvent->pStruct;
    rnum = real_room(*rvnum);
    room = &world[real_room(rnum)];
    break;
  default:
    break;
  }

  if (pMudEvent->sVariables == NULL)
  {
    /* This is odd - This field should always be populated for traps. */
    log("SYSERR: sVariables field is NULL for event_trap_triggered: %d", pMudEvent->iId);
    return 0;
  }
  else
  {
    effect = atoi(pMudEvent->sVariables);
  }

  switch (pMudEvent->iId)
  {

  case eTRAPTRIGGERED:
    /* init the af-struct */
    af.spell = TYPE_UNDEFINED;
    af.duration = 0;
    af.modifier = 0;
    af.location = APPLY_NONE;
    af.bonus_type = BONUS_TYPE_UNDEFINED;

    for (i = 0; i < AF_ARRAY_MAX; i++)
      af.bitvector[i] = AFF_DONTUSE;

    /* check for valid effect, spellnum?  then call spell... */
    if (effect < TRAP_EFFECT_FIRST_VALUE)
    {
      if (effect >= LAST_SPELL_DEFINE)
      {
        log("SYSERR: perform_trap_effect event called with invalid spell effect!\r\n");
      }
      else
      {
        call_magic(ch, ch, NULL, effect, 0, level, casttype);
      }
    }
    else
    {
      /* ok so its not a spell and should be a valid value, lets handle it */
      switch (effect)
      {

      case TRAP_EFFECT_WALL_OF_FLAMES:
        to_char = "\tLThe air is sucked from your lungs as a wall of \tRflames\tL erupts at your feet\tn!";
        to_room = "\tLYou watch in horror as \tn$n\tL is engulfed in a \tRwall \trof \tRflames!\tn";
        dam = dice(20, 20);
        dam_type = DAM_FIRE;
        break;

      case TRAP_EFFECT_LIGHTNING_STRIKE:
        to_char = "\twA brilliant light suddenly blinds you and the smell of your own \tLscorched flesh\tw fills your nostrils.\tn";
        to_room = "\twA bright flash blinds you, striking \tn$n\tw and filling the room with the stench of \tLburnt flesh.\tn";
        dam = dice(20, 30);
        dam_type = DAM_ELECTRIC;
        break;

      case TRAP_EFFECT_IMPALING_SPIKE:
        af.spell = effect;
        SET_BIT_AR(af.bitvector, AFF_PARALYZED);
        af.duration = 5;
        to_char = "\tLA large \tWspike\tL shoots up from the floor, and \trimpales\tL you upon it.\tn";
        to_room = "\tLSuddenly, a large \tWspike\tL impales \tn$n\tL as it shoots up from the floor.\tn";
        dam = dice(15, 20);
        dam_type = DAM_PUNCTURE;
        break;

      case TRAP_EFFECT_DARK_GLYPH:
        af.spell = SPELL_FEEBLEMIND;
        af.modifier = -10;
        af.location = APPLY_INT;
        af.duration = 25;
        to_char = "\tLA dark glyph \tYFLASHES\tL brightly as you walk through it, sending searing pain through your brain.\tn";
        to_room = "\tLAs \tn$n\tL walks through a dark glyph, it \tYflashes\tL brightly.\tn";
        dam = 300 + dice(15, 20);
        dam_type = DAM_MENTAL;
        break;

      case TRAP_EFFECT_SPIKE_PIT:
        to_char = "\tLYou stumble into a shallow hole, screaming out in pain as small spikes in the bottom pierce your foot.\tn";
        to_room = "\tn$n\tL stumbles, screaming as $s foot is impaled on tiny spikes in a shallow hole.\tn";
        dam_type = DAM_PUNCTURE;
        dam = dice(2, 10);
        break;

      case TRAP_EFFECT_DAMAGE_DART:
        to_char = "\tLA tiny \tRdart\tL hits you with full force, piercing your skin.\tn";
        to_room = "\tn$n\tL shivers slightly as a tiny \tRdart\tL hits $m with full force.\tn";
        dam_type = DAM_PUNCTURE;
        dam = 10 + dice(6, 6);
        break;

      case TRAP_EFFECT_POISON_GAS:
        af.duration = 10;
        to_char = "\tgPoisonous gas seeps out entering your lungs!  You feel ill!\tn";
        to_room = "\tgPoisonous gas seeps out into the area!!!\tn";
        af.spell = SPELL_POISON;
        SET_BIT_AR(af.bitvector, AFF_POISON);
        break;

      case TRAP_EFFECT_DISPEL_MAGIC:
        /* special handling, done below */
        to_char = "\tCThere is a blinding flash of light which moves to surround you.  You feel all of your enchantments fade away.\tn";
        to_room = "\tCThere is a blinding flash of light which moves to surround \tn$n\tC.  It disappears as quickly as it came.\tn";

        break;

      case TRAP_EFFECT_DARK_WARRIOR_AMBUSH:
        to_char = "\tRYou are under ambush!\tn\r\n";
        if (GET_LEVEL(ch) < 6)
          count = 1;
        else if (GET_LEVEL(ch) < 8)
          count = 2;
        else
          count = 3;
        for (i = 0; i < count; i++)
        {
          struct char_data *mob = read_mobile(TRAP_DARK_WARRIOR_MOBILE, VIRTUAL);
          if (mob)
          {
            char_to_room(mob, ch->in_room);
            remember(mob, ch);
          }
          else
          {
            log("SYSERR: perform_trap_effect event called with invalid dark warrior mobile!\r\n");
          }
        }
        break;

      case TRAP_EFFECT_BOULDER_DROP:
        dam = GET_HIT(ch) / 5;
        to_char = "A \tyboulder\tn suddenly thunders down from somewhere high above, striking you squarely.";
        to_room = "A \tyboulder\tn falls from somewhere above, hitting $n squarely.";
        break;

      case TRAP_EFFECT_WALL_SMASH:
        dam = GET_HIT(ch) / 5;
        to_char = "\tcA nearby wall suddenly shifts, pressing you against the hard stone.\tn";
        to_room = "\tn$n \tcis suddenly slammed against the stone when an adjacent wall moves inward.\tn";
        break;

      case TRAP_EFFECT_SPIDER_HORDE:
        dam = GET_HIT(ch) / 6;
        to_char = "A horde of \tmspiders\tn drops onto your head from above, the tiny creatures biting any exposed skin.";
        to_room = "$n is suddenly covered in thousands of biting \tmspiders\tn.";
        break;

      case TRAP_EFFECT_DAMAGE_GAS:
        dam = GET_HIT(ch) / 4;
        to_char = "A cloud of \tggas\tn surrounds you!";
        to_room = "A cloud of \tggas\tn surrounds $n!";
        dam_type = DAM_POISON;
        break;

      case TRAP_EFFECT_FREEZING_CONDITIONS:
        //cold damage..
        dam = dice(10, 20);
        dam_type = DAM_COLD;
        to_char = "\tbThe bone-chilling cold bites deep into you, causing you to shudder uncontrollably.\tn";
        to_room = "\tn$n \tbshudders as the icy cold bites deep into $s bones.\tn";
        break;

      case TRAP_EFFECT_SKELETAL_HANDS:
        //skeletal stuff.
        if (dice(1, 10) < 5)
        {
          dam = GET_MAX_HIT(ch) * 2;
          to_char = "\twYou feel a bone-chilling \tCcold\tw as you are raked by \tWskeletal claws\tw thrusting up from the cold waters below.\tn";
          to_room = "\twA gout of icy water washes over \tn$n\tw as hands reach up from below and drag $m under.\tn";
        }
        else
        {
          dam_type = DAM_COLD;
          dam = dice(10, 40);
          to_char = "\twSkeletal hands suddenly thrust up from the waters below, grasping at your feet in an effort to drag you under.\tn";
          to_room = "\twSkeletal hands thrust up from the cold waters, slashing \tn$n\tw and causing $m to shudder with cold and pain.\tn";
        }
        break;

      case TRAP_EFFECT_SPIDER_WEBS:
        af.spell = SPELL_WEB;
        SET_BIT_AR(af.bitvector, AFF_ENTANGLED);
        af.duration = 20;
        to_char = "\tLYou are suddenly entangled in sticky strands of \twspider silk\tL, held fast as spiders descend from above.\tn";
        to_room = "\tn$n \tLis suddenly encased in a cocoon of silk, held fast as spiders descend on $m from all sides.\tn";

        //spiders loading..
        count = dice(1, 3);
        for (i = 0; i < count; i++)
        {
          struct char_data *mob = read_mobile(TRAP_SPIDER_MOBILE, VIRTUAL);
          if (mob)
          {
            char_to_room(mob, ch->in_room);
            remember(mob, ch);
            /* popular demand asks that we add this -zusuk */
            attach_mud_event(new_mud_event(ePURGEMOB, mob, NULL), (180 * PASSES_PER_SEC));
          }
          else
          {
            log("SYSERR: perform_trap_effect event called with invalid spider mobile!\r\n");
          }
        }
        break;

      default:
        log("SYSERR: perform_trap_effect event called with invalid trap-effect!\r\n");
        return 0;
      }

      /* send messages */
      act(to_char, FALSE, ch, 0, 0, TO_CHAR);
      act(to_room, FALSE, ch, 0, 0, TO_ROOM);

      /* handle anything left over */
      if (effect == TRAP_EFFECT_DISPEL_MAGIC) /* special handling */
        spell_dispel_magic(LVL_IMPL, ch, ch, NULL, casttype);
      if (af.spell != TYPE_UNDEFINED) /* has an affection to add? */
        affect_join(ch, &af, TRUE, FALSE, FALSE, FALSE);
      if (dam) /* has damage to process? */
        damage(ch, ch, dam, -1 /*attacktype*/, dam_type, -1 /*offhand*/);
    }
    break;
  default:
    log("SYSERR: event_trap_triggered called with invalid event id!\r\n");
    break;
  }
  return 0;
}

/******* end of file **********/

#endif /* TRAPS_C */
