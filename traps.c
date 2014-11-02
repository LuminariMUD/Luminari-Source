/****************************************************************************
 *  Realms of Luminari
 *  File:     traps.c
 *  Usage:    system for traps!
 *  Header:   traps.h
 *  Authors:  Homeland (ported to Luminari by Zusuk)
 ****************************************************************************/
#ifndef  __TRAPS_C__
#define  __TRAPS_C__

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "handler.h"

#include "mud_event.h"
#include "actions.h"

#include "spells.h"

#include "traps.h"

/* A trap in this version of the code is a special item that is loaded into
   the room that will react to either entering rooms, opening doors, or
   grabbing stuff from a container */

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
void set_off_trap(struct char_data *ch, struct obj_data *trap) {
  struct trap_event *trap_event = NULL;

  if (IS_NPC(ch) && !IS_PET(ch))
    return;

  send_to_char(ch, "Ooops, you must have triggered something.\r\n");
  
  CREATE(trap_event, struct trap_event, 1);
  trap_event->ch = ch;
  trap_event->effect = GET_OBJ_VAL(trap, 2);
//  TRAP(ch) = event_create(perform_trap_effect, trap_event, 0);
}

/* checks the 5th value (4) to see if its set (which indicates detection) */
bool is_trap_detected(struct obj_data *trap) {
  return (GET_OBJ_VAL(trap, 4) > 0);
}

/* set the 5th value (4) to indicate it is detected */
void set_trap_detected(struct obj_data *trap) {
  GET_OBJ_VAL(trap, 4) = 1;
}

/* based on trap-type, see if it should fire or not! */
bool check_trap(struct char_data *ch, int trap_type, int room, struct obj_data *obj, int dir) {
  struct obj_data *trap = NULL;
  
  /* check the room for any traps */
  for (trap = world[room].contents; trap; trap = trap->next_content) {
    
    /* is this a trap? */
    if (GET_OBJ_TYPE(trap) == ITEM_TRAP && GET_OBJ_VAL(trap, 0) == trap_type) {
      switch (trap_type) {
        case TRAP_TYPE_ENTER_ROOM:
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


ACMD(do_disabletrap) {
  struct obj_data *trap = NULL;
  int result = 0;
  
  if (!GET_ABILITY(ch, ABILITY_DISABLE_DEVICE)) {
    send_to_char(ch, "But you do not know how.\r\n");
    return;
  }
  
  for (trap = world[ch->in_room].contents; trap; trap = trap->next_content) {
    if (GET_OBJ_TYPE(trap) == ITEM_TRAP && is_trap_detected(trap)) {
      act("$n is trying to disable a trap", FALSE, ch, 0, 0, TO_ROOM);
      act("You try to disable the trap", FALSE, ch, 0, 0, TO_CHAR);
      if ((result = skill_check(ch, ABILITY_DISABLE_DEVICE, (GET_OBJ_VAL(trap, 3) / 3)))) {
        act("And is Succesful!", FALSE, ch, 0, 0, TO_ROOM);
        act("And are Succesful!", FALSE, ch, 0, 0, TO_CHAR);
        extract_obj(trap);
      } else {
        act("But fails.", FALSE, ch, 0, 0, TO_ROOM);
        act("But fail.", FALSE, ch, 0, 0, TO_CHAR);
        if (result <= -5) /* fail your roll by 5 or more, BOOM! */
          set_off_trap(ch, trap);
      }
      USE_FULL_ROUND_ACTION(ch);
      return;
    }
  }
  send_to_char(ch, "But there are no traps here than you can see.\r\n");
}


ACMD(do_detecttrap) {
  struct obj_data *trap = NULL;
  
  if (!GET_ABILITY(ch, ABILITY_PERCEPTION)) {
    send_to_char(ch, "But you do not know how.\r\n");
    return;
  }

  USE_FULL_ROUND_ACTION(ch);
  for (trap = world[ch->in_room].contents; trap; trap = trap->next_content) {
    if (GET_OBJ_TYPE(trap) == ITEM_TRAP && !is_trap_detected(trap)) {
      if (skill_check(ch, ABILITY_PERCEPTION, (GET_OBJ_VAL(trap, 3) / 3))) {
        act("$n has detected a trap!", FALSE, ch, 0, 0, TO_ROOM);
        act("You have detected a trap!", FALSE, ch, 0, 0, TO_CHAR);
        set_trap_detected(trap);
        return;
      }
    }
  }
  act("$n is looking around for some traps, but can not find any.", FALSE, ch, 0, 0, TO_ROOM);
  act("You do not seem to detect any traps.", FALSE, ch, 0, 0, TO_CHAR);
}
*/

/*
EVENTFUNC(perform_trap_effect) {
  struct trap_event *trap_event = (struct trap_event *) event_obj;
  struct char_data *ch = trap_event->ch;
  int effect = trap_event->effect;
  int type = DAMBIT_PHYSICAL;
  struct affected_type af;
  char *to_char = 0;
  char *to_room = 0;
  int dam = 0;
  int count = 0;
  int i;

  af.type = 0;
  af.modifier = 0;
  TRAP(ch) = 0;

  if (effect < 1000)
    call_magic(ch, ch, 0, effect, 50, CAST_SPELL);
  else {
    switch (effect) {
      case 1000:
        to_char = "&cLThe air is sucked from your lungs as a wall of &cRflames&cL erupts at your feet&c0!";
        to_room = "&cLYou watch in horror as &c0$n&cL is engulfed in a &cRwall &crof &cRflames!&c0";
        dam = dice(20, 20);
        type = DAMBIT_FIRE;
        if (AFF_FLAGGED(ch, AFF_PROTECT_FIRE))
          dam /= 2;
        break;
      case 1001:
        to_char = "&cwA brilliant light suddenly blinds you and the smell of your own &cLscorched flesh&cw fills your nostrils.&c0";
        to_room = "&cwA bright flash blinds you, striking &c0$n&c and filling the room with the stench of &cLburnt flesh.&c0";
        dam = dice(20, 30);
        if (AFF_FLAGGED(ch, AFF_PROTECT_LIGHTNING))
          dam /= 2;
        type = DAMBIT_LIGHTNING;
        break;
      case 1002:
        af.type = effect;
        af.bitvector = 0;
        af.bitvector2 = AFF2_MAJOR_PARA;
        af.bitvector3 = 0;
        af.duration = 5;
        to_char = "&cLA large &cWspike&cL shoots up from the floor, and &crimpales&cL you upon it.&c0";
        to_room = "&cLSuddenly, a large &cWspike&cL impales &c0$n&cL as it shoots up from the floor.&c0";
        dam = dice(15, 20);
        break;
      case 1003:
        af.type = SPELL_FEEBLEMIND;
        af.bitvector = 0;
        af.bitvector2 = AFF2_FEEBLEMIND;
        af.bitvector3 = 0;
        af.duration = 25;
        to_char = "&cLA dark glyph &cYFLASHES&cL brightly as you walk through it, hurting your brain immensely.&c0";
        to_room = "&cLAs $n&cL walks through a dark glyph, it &cYflashes&cL brightly.&c0";
        dam = 300 + dice(15, 20);
        type = DAMBIT_MAGIC;
        break;
      case 1004:
        to_char = "&cLYou stumble into a shallow hole, screaming out in pain as small spikes in the bottom pierce your foot.&c0";
        to_room = "&cL$n&cL stumbles, screaming as $s foot is impaled on tiny spikes in a shallow hole.&c0";
        dam = dice(2, 10);
        break;

      case 1005:
        to_char = "&cLA tiny &cRdart&cL hits you with full force, piercing your skin.&c0";
        to_room = "&cL$n&cL shivers slightly as a tiny &cRdart&cL hits $m with full force.&c0";
        dam = 10 + dice(6, 6);
        break;
      case 1006:
        if (!AFF_FLAGGED(ch, AFF_PROTECT_GAS)) {
          af.type = SPELL_POISON;
          af.bitvector = AFF_POISON;
          af.bitvector2 = 0;
          af.bitvector3 = 0;
          af.duration = 25;
          to_char = "&cwAs you inhale the &cgacrid vapors&c0, you cough and choke and start to feel quite sick.&c0";
          to_room = "&cw$n&cw takes a breath, and starts to sputter and cough, looking a little pale.&c0";
          dam = 15 + dice(2, 15);
          type = DAMBIT_GAS;
        }
        break;

      case 1007:
        to_char = "&cCThere is a blinding flash of light which moves to surround you.  You feel all of your enchantments fade away.&c0";
        to_room = "&cCThere is a blinding flash of light which moves to surround &c0$n&cC.  It disappears as quickly as it came.&c0";
        break;

      case 1008:
        to_char = "&cRYou are under ambush!&c0\r\n";
        if (GET_LEVEL(ch) < 6)
          count = 1;
        else if (GET_LEVEL(ch) < 8)
          count = 2;
        else
          count = 3;
        for (i = 0; i < count; i++) {
          struct char_data *mob = read_mobile(35600, VIRTUAL);
          char_to_room(mob, ch->in_room);
          remember(mob, ch);
        }
        break;
      case 1009:
        dam = GET_HIT(ch) / 5;
        to_char = "A &cyboulder&c0 suddenly thunders down from somewhere high above, hitting you squarely.";
        to_room = "A &cyboulder&c0 falls from somewhere above, hitting $n squarely.";
        break;
      case 1010:
        dam = GET_HIT(ch) / 5;
        to_char = "&ccA nearby wall suddenly shifts, pressing you against the hard stone.&c0";
        to_room = "&cc$n is suddenly slammed against the stone when an adjacent wall moves inward.&c0";
        break;
      case 1011:
        dam = GET_HIT(ch) / 6;
        to_char = "A horde of &cmspiders&c0 drops onto your head from above, the tiny creatures biting any exposed skin.";
        to_room = "$n is suddenly covered in thousands of biting &cmspiders&c0.";
        break;
      case 1012:
        dam = GET_HIT(ch) / 4;
        to_char = "A cloud of &cggas&c0 surrounds you!";
        to_room = "A cloud of &cggas&c0 surrounds $n!";
        type = DAMBIT_GAS;
        break;
      case 1013:
        //cold damage..
        dam = dice(10, 20);
        type = DAMBIT_COLD;
        if (AFF_FLAGGED(ch, AFF_PROTECT_COLD))
          dam /= 2;
        to_char = "&cbThe bone-chilling cold bites deep into you, causing you to shudder uncontrollably.&c0";
        to_room = "&cb$n shudders as the icy cold bites deep into $s bones.&c0";
        break;
      case 1014:
        //skeletal stuff.
        if (dice(1, 101) > GET_LUCK(ch) && dice(1, 10) < 5) {
          dam = GET_MAX_HIT(ch)*2;
          to_char = "&cwYou feel a bone-chilling &cCcold&cw as you are raked by &cWskeletal claws&cw thrusting up from the cold waters below.&c0";
          to_room = "&cwA gout of icy water washes over $n as hands reach up from below and drag $m under.&c0";
        } else {
          type = DAMBIT_COLD | DAMBIT_PHYSICAL;
          dam = dice(10, 40);
          if (AFF_FLAGGED(ch, AFF_PROTECT_COLD))
            dam /= 2;
          to_char = "&cwSkeletal hands suddenly thrust up from the waters below, grasping at your feet in an effort to drag you under.&c0";
          to_room = "&cwSkeletal hands thrust up from the cold waters, slashing $n and causing $m to shudder with cold and pain.&c0";
        }
        break;
      case 1015:
        af.type = SPELL_ENTANGLE;
        af.bitvector = 0;
        af.bitvector2 = AFF2_ENTANGLED;
        af.bitvector3 = 0;
        af.duration = 10;
        to_char = "&cLYou are suddenly entangled in sticky strands of &cwspider silk&cL, held fast as spiders descend from above.&c0";
        to_room = "&cL$n is suddenly encased in a cocoon of silk, held fast as spiders descend on $m from all sides.&c0";

        //spiders loading..
        count = dice(1, 3);
        for (i = 0; i < count; i++) {
          struct char_data *mob = read_mobile(80437, VIRTUAL);
          char_to_room(mob, ch->in_room);
          remember(mob, ch);
        }
        break;

      default:
        return 0;
    }
    act(to_char, FALSE, ch, 0, 0, TO_CHAR);
    act(to_room, FALSE, ch, 0, 0, TO_ROOM);
    if (effect == 1007)
      spell_dispelmagic(100, ch, ch, 0);
    if (af.type)
      affect_join(ch, &af, 1, FALSE, FALSE, FALSE);
    if (dam)
      damage(ch, ch, dam, SKILL_DISABLE_TRAP, type);
  }
  FREE(event_obj);
  return 0;
}
 */

/******* end of file **********/

#endif	/* TRAPS_C */
