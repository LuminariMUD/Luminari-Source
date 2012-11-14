/**************************************************************************
*  File: act.offensive.c                                   Part of tbaMUD *
*  Usage: Player-level commands of an offensive nature.                   *
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
#include "spells.h"
#include "act.h"
#include "fight.h"
#include "mud_event.h"
#include "constants.h"
#include "spec_procs.h"




ACMD(do_rage)
{
  struct affected_type af, aftwo, afthree, affour;

  if (affected_by_spell(ch, SKILL_RAGE)) {
    send_to_char(ch, "You are already raging!\r\n");
    return;
  }

  if (!IS_BADGER(ch)) {
    send_to_char(ch, "You don't know how to rage.\r\n");
    return;
  }

  send_to_char(ch, "You go into a \tRR\trA\tRG\trE\tn!.\r\n");
  act("$n goes into a \tRR\trA\tRG\trE\tn!", FALSE, ch, 0, 0, TO_ROOM);

  new_affect(&af);
  new_affect(&aftwo);
  new_affect(&afthree);
  new_affect(&affour);

  af.spell = SKILL_RAGE;
  af.duration = 6 + GET_CON_BONUS(ch) * 2;
  af.location = APPLY_STR;
  af.modifier = 4;

  aftwo.spell = SKILL_RAGE;
  aftwo.duration = 6 + GET_CON_BONUS(ch) * 2;
  aftwo.location = APPLY_CON;
  aftwo.modifier = 4;
  GET_HIT(ch) += GET_LEVEL(ch) * 2 + 10;

  afthree.spell = SKILL_RAGE;
  afthree.duration = 6 + GET_CON_BONUS(ch) * 2;
  afthree.location = APPLY_SAVING_WILL;
  afthree.modifier = 2;

  affour.spell = SKILL_RAGE;
  affour.duration = 6 + GET_CON_BONUS(ch) * 2;
  affour.location = APPLY_AC;
  affour.modifier = 20;

  affect_to_char(ch, &af);
  affect_to_char(ch, &aftwo);
  affect_to_char(ch, &afthree);
  affect_to_char(ch, &affour);
  
}


ACMD(do_assist)
{
  char arg[MAX_INPUT_LENGTH];
  struct char_data *helpee, *opponent;

  if (FIGHTING(ch)) {
    send_to_char(ch, "You're already fighting!  How can you assist someone else?\r\n");
    return;
  }
  one_argument(argument, arg);

  if (!*arg)
    send_to_char(ch, "Whom do you wish to assist?\r\n");
  else if (!(helpee = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM)))
    send_to_char(ch, "%s", CONFIG_NOPERSON);
  else if (helpee == ch)
    send_to_char(ch, "You can't help yourself any more than this!\r\n");
  else {
    /*
     * Hit the same enemy the person you're helping is.
     */
    if (FIGHTING(helpee))
      opponent = FIGHTING(helpee);
    else
      for (opponent = world[IN_ROOM(ch)].people;
	   opponent && (FIGHTING(opponent) != helpee);
	   opponent = opponent->next_in_room);

    if (!opponent)
      act("But nobody is fighting $M!", FALSE, ch, 0, helpee, TO_CHAR);
    else if (!CAN_SEE(ch, opponent))
      act("You can't see who is fighting $M!", FALSE, ch, 0, helpee, TO_CHAR);
         /* prevent accidental pkill */
    else if (!CONFIG_PK_ALLOWED && !IS_NPC(opponent))
      send_to_char(ch, "You cannot kill other players.\r\n");
    else {
      send_to_char(ch, "You join the fight!\r\n");
      act("$N assists you!", 0, helpee, 0, ch, TO_CHAR);
      act("$n assists $N.", FALSE, ch, 0, helpee, TO_NOTVICT);
      hit(ch, opponent, TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, FALSE);
    }
  }
}

ACMD(do_hit)
{
  char arg[MAX_INPUT_LENGTH];
  struct char_data *vict;
  int chInitiative = dice(1,20), victInitiative = dice(1,20);

  one_argument(argument, arg);

  if (!*arg)
    send_to_char(ch, "Hit who?\r\n");
  else if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM)))
    send_to_char(ch, "They don't seem to be here.\r\n");
  else if (vict == ch) {
    send_to_char(ch, "You hit yourself...OUCH!.\r\n");
    act("$n hits $mself, and says OUCH!", FALSE, ch, 0, vict, TO_ROOM);
  } else if (FIGHTING(ch) == vict) {
    send_to_char(ch, "You are already fighting that one!\r\n");
  } else if (AFF_FLAGGED(ch, AFF_CHARM) && (ch->master == vict))
    act("$N is just such a good friend, you simply can't hit $M.", FALSE, ch, 0, vict, TO_CHAR);
  else {
    if (!CONFIG_PK_ALLOWED && !IS_NPC(vict) && !IS_NPC(ch)) 
	check_killer(ch, vict);
    if (GET_POS(ch) == POS_STANDING) {
      if (!IS_NPC(ch) && GET_SKILL(ch, SKILL_INITIATIVE))
        chInitiative += 8;
      chInitiative += GET_DEX(ch);
      if (!IS_NPC(vict) && GET_SKILL(vict, SKILL_INITIATIVE))
        victInitiative += 8;
      victInitiative += GET_DEX(vict);
      if (chInitiative >= victInitiative || GET_POS(vict) < POS_FIGHTING)
        hit(ch, vict, TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, FALSE);  /* ch first */
      else {
        send_to_char(vict, "\tYYour superior initiative grants the first strike!\tn\r\n");
        send_to_char(ch,
"\tyYour opponents superior \tYinitiative\ty grants the first strike!\tn\r\n");
        hit(vict, ch, TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, FALSE);  // victim is first 
        update_pos(ch);
        if (!IS_NPC(vict) && GET_SKILL(vict, SKILL_INITIATIVE) &&
		GET_POS(ch) > POS_DEAD) {
          send_to_char(vict, "\tYYour superior initiative grants another attack!\tn\r\n");
          hit(vict, ch, TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, FALSE);
        }
      }
      WAIT_STATE(ch, PULSE_VIOLENCE + 2);
    } else {
      stop_fighting(ch);
      send_to_char(ch, "You switch opponents!\r\n");
      act("$n switches opponents!", FALSE, ch, 0, vict, TO_ROOM);      
      hit(ch, vict, TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, FALSE);
      WAIT_STATE(ch, PULSE_VIOLENCE * 2);

      //everyone gets a free shot at you unless you make a tumble check
        //15 is DC
      vict = FIGHTING(ch);
      if (FIGHTING(ch) && FIGHTING(vict)) {
        if (!IS_NPC(ch) && dice(1, 20) + compute_ability(ch, ABILITY_TUMBLE) <= 15) {
          send_to_char(ch, "\tR*Opponent Attack Opportunity*\tn");
          send_to_char(vict, "\tW*Attack Opportunity*\tn");
          hit(vict, ch, TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, FALSE);
          update_pos(ch);
          update_pos(vict);
        } else {
          send_to_char(ch, "\tW*Tumble Success*\tn");
          send_to_char(vict, "\tR*Tumbled vs AoO*\tn");
        }
      }

    }
  } 
}

ACMD(do_kill)
{
  char arg[MAX_INPUT_LENGTH];
  struct char_data *vict;

  if (GET_LEVEL(ch) < LVL_IMMORT || IS_NPC(ch) || !PRF_FLAGGED(ch, PRF_NOHASSLE)) {
    do_hit(ch, argument, cmd, subcmd);
    return;
  }
  one_argument(argument, arg);

  if (!*arg) {
    send_to_char(ch, "Kill who?\r\n");
  } else {
    if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM)))
      send_to_char(ch, "They aren't here.\r\n");
    else if (ch == vict)
      send_to_char(ch, "Your mother would be so sad.. :(\r\n");
    else {
      act("You chop $M to pieces!  Ah!  The blood!", FALSE, ch, 0, vict, TO_CHAR);
      act("$N chops you to pieces!", FALSE, vict, 0, ch, TO_CHAR);
      act("$n brutally slays $N!", FALSE, ch, 0, vict, TO_NOTVICT);
      if (!IS_NPC(vict))
        raw_kill(vict, ch);
      else
        raw_kill_old(vict, ch);
    }
  }
}

ACMD(do_backstab)
{
  char buf[MAX_INPUT_LENGTH];
  struct char_data *vict;
  int percent, percent2, prob;

  if (IS_NPC(ch) || !GET_SKILL(ch, SKILL_BACKSTAB)) {
    send_to_char(ch, "You have no idea how to do that.\r\n");
    return;
  }

  one_argument(argument, buf);

  if (!(vict = get_char_vis(ch, buf, NULL, FIND_CHAR_ROOM))) {
    send_to_char(ch, "Backstab who?\r\n");
    return;
  }
  if (vict == ch) {
    send_to_char(ch, "How can you sneak up on yourself?\r\n");
    return;
  }

  if (!GET_EQ(ch, WEAR_WIELD_1) && !GET_EQ(ch, WEAR_WIELD_2) && !GET_EQ(ch, WEAR_WIELD_2H)) {
    send_to_char(ch, "You need to wield a weapon to make it a success.\r\n");
    return;
  }

  if (FIGHTING(vict)) {
    send_to_char(ch, "You can't backstab a fighting person -- they're too alert!\r\n");
    return;
  }

  if (MOB_FLAGGED(vict, MOB_AWARE) && AWAKE(vict)) {
    act("You notice $N lunging at you!", FALSE, vict, 0, ch, TO_CHAR);
    act("$e notices you lunging at $m!", FALSE, vict, 0, ch, TO_VICT);
    act("$n notices $N lunging at $m!", FALSE, vict, 0, ch, TO_NOTVICT);
    hit(vict, ch, TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, FALSE);
    return;
  }

  percent = rand_number(1, 101);	/* 101% is a complete failure */
  percent2 = rand_number(1, 101);	/* 101% is a complete failure */
  prob = GET_SKILL(ch, SKILL_BACKSTAB);

  int successful = 0;

  if (GET_EQ(ch, WEAR_WIELD_1) &&
	(GET_OBJ_VAL(GET_EQ(ch, WEAR_WIELD_1), 3) == TYPE_PIERCE - TYPE_HIT)) {
    if (AWAKE(vict) && (percent > prob)) {
      damage(ch, vict, 0, SKILL_BACKSTAB, DAM_PUNCTURE, FALSE);
    } else {
      hit(ch, vict, SKILL_BACKSTAB, DAM_PUNCTURE, 0, FALSE);
    }
    successful++;
  }

  update_pos(vict);

  if (vict) {
    if (GET_EQ(ch, WEAR_WIELD_2) &&
	GET_OBJ_VAL(GET_EQ(ch, WEAR_WIELD_2), 3) == TYPE_PIERCE - TYPE_HIT &&
 	GET_POS(vict) >= POS_DEAD) {
      if (AWAKE(vict) && (percent2 > prob)) {
        damage(ch, vict, 0, SKILL_BACKSTAB, DAM_PUNCTURE, TRUE);
      } else {
        hit(ch, vict, SKILL_BACKSTAB, DAM_PUNCTURE, 0, TRUE);
      }
      successful++;
    }
  }

  update_pos(vict);

  if (vict) {
    if (GET_EQ(ch, WEAR_WIELD_2H) &&
	GET_OBJ_VAL(GET_EQ(ch, WEAR_WIELD_2H), 3) == TYPE_PIERCE - TYPE_HIT &&
 	GET_POS(vict) >= POS_DEAD) {
      if (AWAKE(vict) && (percent2 > prob)) {
        damage(ch, vict, 0, SKILL_BACKSTAB, DAM_PUNCTURE, TRUE);
      } else {
        hit(ch, vict, SKILL_BACKSTAB, DAM_PUNCTURE, 0, TRUE);
      }
      successful++;
    }
  }

  if (successful)
    WAIT_STATE(ch, 2 * PULSE_VIOLENCE);
  else
    send_to_char(ch, "You have no piercing weapon equipped.\r\n");
}

ACMD(do_order)
{
  char name[MAX_INPUT_LENGTH], message[MAX_INPUT_LENGTH];
  bool found = FALSE;
  struct char_data *vict;
  struct follow_type *k;

  half_chop(argument, name, message);

  if (!*name || !*message)
    send_to_char(ch, "Order who to do what?\r\n");
  else if (!(vict = get_char_vis(ch, name, NULL, FIND_CHAR_ROOM)) && !is_abbrev(name, "followers"))
    send_to_char(ch, "That person isn't here.\r\n");
  else if (ch == vict)
    send_to_char(ch, "You obviously suffer from skitzofrenia.\r\n");
  else {
    if (AFF_FLAGGED(ch, AFF_CHARM)) {
      send_to_char(ch, "Your superior would not aprove of you giving orders.\r\n");
      return;
    }
    if (vict) {
      char buf[MAX_STRING_LENGTH];

      snprintf(buf, sizeof(buf), "$N orders you to '%s'", message);
      act(buf, FALSE, vict, 0, ch, TO_CHAR);
      act("$n gives $N an order.", FALSE, ch, 0, vict, TO_ROOM);

      if ((vict->master != ch) || !AFF_FLAGGED(vict, AFF_CHARM))
	act("$n has an indifferent look.", FALSE, vict, 0, 0, TO_ROOM);
      else {
	send_to_char(ch, "%s", CONFIG_OK);
	command_interpreter(vict, message);
      }
    } else {			/* This is order "followers" */
      char buf[MAX_STRING_LENGTH];

      snprintf(buf, sizeof(buf), "$n issues the order '%s'.", message);
      act(buf, FALSE, ch, 0, 0, TO_ROOM);

      for (k = ch->followers; k; k = k->next) {
	if (IN_ROOM(ch) == IN_ROOM(k->follower))
	  if (AFF_FLAGGED(k->follower, AFF_CHARM)) {
	    found = TRUE;
	    command_interpreter(k->follower, message);
	  }
      }
      if (found)
	send_to_char(ch, "%s", CONFIG_OK);
      else
	send_to_char(ch, "Nobody here is a loyal subject of yours!\r\n");
    }
  }
}


ACMD(do_flee)
{
  char arg[MAX_INPUT_LENGTH];
  int i;

  if (GET_POS(ch) < POS_FIGHTING || GET_HIT(ch) <= 0) {
    send_to_char(ch, "You are in pretty bad shape, unable to flee!\r\n");
    return;
  }

  if (argument)
    one_argument(argument, arg);

  if (!*arg) {
    perform_flee(ch);
  } else if ((*arg && !IS_NPC(ch) && !GET_SKILL(ch, SKILL_SPRING_ATTACK))) {
    perform_flee(ch);
  } else {// there is an argument, check if its valid
    if (!GET_SKILL(ch, SKILL_SPRING_ATTACK)) {
      send_to_char(ch, "You don't have the option to choose which way to flee!\r\n");
      return;
    }
    //actualy direction?
    if ((i = search_block(arg, dirs, FALSE)) >= 0) {
      if (CAN_GO(ch, i)) {
        if (do_simple_move(ch, i, 3)) {
          send_to_char(ch, "You make a tactic retreat from battle!\r\n");
          act("$n makes a tactical retreat from the battle!",
		TRUE, ch, 0, 0, TO_ROOM);
        } else {
          send_to_char(ch, "You can't escape that direction!\r\n");
          return;
        }
      } else {
        send_to_char(ch, "You can't escape that direction!\r\n");
        return;
      }      
    } else {
      send_to_char(ch, "That isn't a valid direction!\r\n");
      return;
    }
  }
}


ACMD(do_parry)
{
  if (IS_NPC(ch) || !GET_ABILITY(ch, ABILITY_PARRY)) {
    send_to_char(ch, "You have no idea how to do that.\r\n");
    return;
  }
  
  if (AFF_FLAGGED(ch, AFF_PARRY)) {
    REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_PARRY);
    send_to_char(ch, "You leave 'parry' mode.\r\n");
    return;
  }
 
  send_to_char(ch, "You are now in 'parry' mode.\r\n");
 
  SET_BIT_AR(AFF_FLAGS(ch), AFF_PARRY);
}


ACMD(do_powerattack)
{
  if (IS_NPC(ch) || !GET_SKILL(ch, SKILL_POWER_ATTACK)) {
    send_to_char(ch, "You have no idea how to do that.\r\n");
    return;
  }
  
  if (AFF_FLAGGED(ch, AFF_POWER_ATTACK)) {
    REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_POWER_ATTACK);
    send_to_char(ch, "You leave 'power attack' mode.\r\n");
    return;
  }
 
  send_to_char(ch, "You are now in 'power attack' mode.\r\n");
 
  SET_BIT_AR(AFF_FLAGS(ch), AFF_POWER_ATTACK);
}


ACMD(do_expertise)
{
  if (IS_NPC(ch) || !GET_SKILL(ch, SKILL_EXPERTISE)) {
    send_to_char(ch, "You have no idea how to do that.\r\n");
    return;
  }
  
  if (AFF_FLAGGED(ch, AFF_EXPERTISE)) {
    REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_EXPERTISE);
    send_to_char(ch, "You leave 'expertise' mode.\r\n");
    return;
  }
 
  send_to_char(ch, "You are now in 'expertise' mode.\r\n");
 
  SET_BIT_AR(AFF_FLAGS(ch), AFF_EXPERTISE);
}


ACMD(do_taunt)
{
  char arg[MAX_INPUT_LENGTH];
  struct char_data *vict;
  int attempt = dice(1,20), resist = dice(1,20);

  one_argument(argument, arg);

  if (IS_NPC(ch) || !GET_ABILITY(ch, ABILITY_TAUNT)) {
    send_to_char(ch, "You have no idea how.\r\n");
    return;
  }
  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL)) {
    send_to_char(ch, "This room just has such a peaceful, easy feeling...\r\n");
    return;
  }
  if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM))) {
    if (FIGHTING(ch) && IN_ROOM(ch) == IN_ROOM(FIGHTING(ch))) {
      vict = FIGHTING(ch);
    } else {
      send_to_char(ch, "Taunt who?\r\n");
      return;
    }
  }
  if (vict == ch) {
    send_to_char(ch, "Aren't we funny today...\r\n");
    return;
  }
  if (MOB_FLAGGED(vict, MOB_NOKILL)) {
    send_to_char(ch, "This mob is protected.\r\n");
    return;
  }
  if (char_has_mud_event(ch, eTAUNTED)) {
    send_to_char(ch, "Your target is already taunted...\r\n");
    return;
  }
  if (char_has_mud_event(ch, eTAUNT)) {
    send_to_char(ch, "You must wait longer before you can use this ability again.\r\n");
    send_to_char(ch, "OOC:  The cooldown is approximately 7 seconds.\r\n");
    return;
  }

  attempt += compute_ability(ch, ABILITY_TAUNT);
  if (!IS_NPC(vict))
    resist += compute_ability(vict, ABILITY_CONCENTRATION);
  else
    resist += (GET_LEVEL(vict) / 2);

  if (attempt >= resist) {
    send_to_char(ch, "You taunt your opponent!\r\n");
    act("You are \tRtaunted\tn by $N!", FALSE, vict, 0, ch, TO_CHAR);
    act("$n \tWtaunts\tn $N!", FALSE, ch, 0, vict, TO_NOTVICT);
    attach_mud_event(new_mud_event(eTAUNTED, vict, NULL), 4 * PASSES_PER_SEC);
  } else {
    send_to_char(ch, "You fail to taunt your opponent!\r\n");
    act("$N fails to \tRtaunt\tn you...", FALSE, vict, 0, ch, TO_CHAR);
    act("$n fails to \tWtaunt\tn $N!", FALSE, ch, 0, vict, TO_NOTVICT);
  }
  if (!FIGHTING(vict))
    hit(vict, ch, TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, FALSE);
  attach_mud_event(new_mud_event(eTAUNT, ch, NULL), 8 * PASSES_PER_SEC);
}


ACMD(do_frightful)
{
  struct char_data *vict, *next_vict;

  if (!IS_RED_DRAGON(ch)) {
    send_to_char(ch, "You have no idea how.\r\n");
    return;
  }
  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL)) {
    send_to_char(ch, "This room just has such a peaceful, easy feeling...\r\n");
    return;
  }

  send_to_char(ch, "You ROAR!\r\n");
  act("$n lets out a mighty ROAR!", FALSE, ch, 0, 0, TO_ROOM);

  for (vict = world[IN_ROOM(ch)].people; vict; vict = next_vict) {
    next_vict = vict->next_in_room;
  
    if (aoeOK(ch, vict, -1)) {
      send_to_char(ch, "You roar at %s.\r\n", GET_NAME(vict));
      send_to_char(vict, "A mighty roar from %s is directed at you!\r\n", 
		GET_NAME(ch));
      act("$n roars at $N!", FALSE, ch, 0, vict, 
		TO_NOTVICT);

      perform_flee(vict);
      perform_flee(vict);
      perform_flee(vict);
      WAIT_STATE(vict, PULSE_VIOLENCE * 4);
    }
  }

  WAIT_STATE(ch, PULSE_VIOLENCE * 3);
}


ACMD(do_breathe)
{
  struct char_data *vict, *next_vict;

  if (!IS_RED_DRAGON(ch)) {
    send_to_char(ch, "You have no idea how.\r\n");
    return;
  }
  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL)) {
    send_to_char(ch, "This room just has such a peaceful, easy feeling...\r\n");
    return;
  }

  send_to_char(ch, "You exhale breathing out fire!\r\n");
  act("$n exhales breathing fire!", FALSE, ch, 0, 0, TO_ROOM);

  for (vict = world[IN_ROOM(ch)].people; vict; vict = next_vict) {
    next_vict = vict->next_in_room;

    if (aoeOK(ch, vict, SPELL_FIRE_BREATHE)) {  
      WAIT_STATE(vict, PULSE_VIOLENCE * 1);
      damage(ch, vict, dice(GET_LEVEL(ch), 14), SPELL_FIRE_BREATHE, DAM_FIRE, FALSE);
    }
  }

  WAIT_STATE(ch, PULSE_VIOLENCE * 3);
}


ACMD(do_tailsweep)
{
  struct char_data *vict, *next_vict;
  int percent = 0, prob = 0;

  if (!IS_RED_DRAGON(ch)) {
    send_to_char(ch, "You have no idea how.\r\n");
    return;
  }
  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL)) {
    send_to_char(ch, "This room just has such a peaceful, easy feeling...\r\n");
    return;
  }

  send_to_char(ch, "You lash out with your mighty tail!\r\n");
  act("$n lashes out with $s mighty tail!", FALSE, ch, 0, 0, TO_ROOM);

  for (vict = world[IN_ROOM(ch)].people; vict; vict = next_vict) {
    next_vict = vict->next_in_room;

    if (vict == ch)
      continue;

    // pass -2 as spellnum to handle tailsweep
    if (aoeOK(ch, vict, -2)) {  
      percent = rand_number(1, 101);
      prob = rand_number(75, 100);

      if (percent > prob) {
        send_to_char(ch, "You failed to knock over %s.\r\n", GET_NAME(vict));
        send_to_char(vict, "You were able to dodge a tailsweep from %s.\r\n", 
		GET_NAME(ch));
        act("$N dodges a tailsweep from $n.", FALSE, ch, 0, vict, 
		TO_NOTVICT);
      } else {
        GET_POS(vict) = POS_SITTING;
        WAIT_STATE(vict, PULSE_VIOLENCE * 2);

        send_to_char(ch, "You knock over %s.\r\n", GET_NAME(vict));
        send_to_char(vict, "You were knocked down by a tailsweep from %s.\r\n", 
		GET_NAME(ch));
        act("$N is knocked down by a tailsweep from $n.", FALSE, ch, 0, vict, 
		TO_NOTVICT);
      }

      if (GET_POS(ch) > POS_STUNNED && (FIGHTING(ch) == NULL))  // ch -> vict
        set_fighting(ch, vict);
      if (GET_POS(vict) > POS_STUNNED && (FIGHTING(vict) == NULL)) {  // vict -> ch
        set_fighting(vict, ch);
        if (MOB_FLAGGED(vict, MOB_MEMORY) && !IS_NPC(ch))
          remember(vict, ch);
      }

    }
  }

  WAIT_STATE(ch, PULSE_VIOLENCE * 2);
}


ACMD(do_bash)
{
  char arg[MAX_INPUT_LENGTH];
  struct char_data *vict;
  int percent, prob;

  one_argument(argument, arg);

  if (IS_NPC(ch) || !GET_SKILL(ch, SKILL_BASH)) {
    send_to_char(ch, "You have no idea how.\r\n");
    return;
  }
  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL)) {
    send_to_char(ch, "This room just has such a peaceful, easy feeling...\r\n");
    return;
  }
  if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM))) {
    if (FIGHTING(ch) && IN_ROOM(ch) == IN_ROOM(FIGHTING(ch))) {
      vict = FIGHTING(ch);
    } else {
      send_to_char(ch, "Bash who?\r\n");
      return;
    }
  }
  if (vict == ch) {
    send_to_char(ch, "Aren't we funny today...\r\n");
    return;
  }
  if (MOB_FLAGGED(vict, MOB_NOKILL)) {
    send_to_char(ch, "This mob is protected.\r\n");
    return;
  }
  if ((GET_SIZE(ch) - GET_SIZE(vict)) >= 2) {
    send_to_char(ch, "Your target is too small!\r\n");
    return;
  }
  if ((GET_SIZE(vict) - GET_SIZE(ch)) >= 2) {
    send_to_char(ch, "Your target is too big!\r\n");
    return;
  }  

  percent = rand_number(1, 101);	/* 101% is a complete failure */
  prob = GET_SKILL(ch, SKILL_BASH);

  if (MOB_FLAGGED(vict, MOB_NOBASH) && !GET_SKILL(ch, SKILL_IMPROVED_BASH)) {
    send_to_char(ch, "You realize you will probably not succeed:  ");
    percent = 101;
  }
  if (GET_POS(vict) == POS_SITTING) {
    send_to_char(ch, "It is difficult to knock down something already down:  ");
    percent += 75;
  }
   
  if (!IS_NPC(ch) && GET_SKILL(ch, SKILL_IMPROVED_BASH))
    prob += GET_SKILL(ch, SKILL_IMPROVED_BASH) / 5;
  
  if (!IS_NPC(vict) && compute_ability(vict, ABILITY_DISCIPLINE))
    percent += compute_ability(vict, ABILITY_DISCIPLINE);
  
  if (GET_RACE(ch) == RACE_DWARF ||
          GET_RACE(ch) == RACE_CRYSTAL_DWARF) // dwarf dwarven stability
    percent += 4;

  if (percent > prob) {
    GET_POS(ch) = POS_SITTING;
    damage(ch, vict, 0, SKILL_BASH, DAM_FORCE, FALSE);
  } else {
    GET_POS(vict) = POS_SITTING;
    if (GET_SKILL(ch, SKILL_IMPROVED_BASH)) {
      if (damage(ch, vict, GET_LEVEL(ch), SKILL_BASH, DAM_FORCE, FALSE) > 0)
        WAIT_STATE(vict, PULSE_VIOLENCE * 2);
    } else if (damage(ch, vict, 1, SKILL_BASH, DAM_FORCE, FALSE) > 0)
      WAIT_STATE(vict, PULSE_VIOLENCE * 2);
  }

  WAIT_STATE(ch, PULSE_VIOLENCE * 2);
}


ACMD(do_trip)
{
  char arg[MAX_INPUT_LENGTH];
  struct char_data *vict;
  int percent, prob;

  one_argument(argument, arg);

  if (IS_NPC(ch) || !GET_SKILL(ch, SKILL_TRIP)) {
    send_to_char(ch, "You have no idea how.\r\n");
    return;
  }
  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL)) {
    send_to_char(ch, "This room just has such a peaceful, easy feeling...\r\n");
    return;
  }
  if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM))) {
    if (FIGHTING(ch) && IN_ROOM(ch) == IN_ROOM(FIGHTING(ch))) {
      vict = FIGHTING(ch);
    } else {
      send_to_char(ch, "Trip who?\r\n");
      return;
    }
  }
  if (vict == ch) {
    send_to_char(ch, "Aren't we funny today...\r\n");
    return;
  }
  if (MOB_FLAGGED(vict, MOB_NOKILL)) {
    send_to_char(ch, "This mob is protected.\r\n");
    return;
  }
  if (GET_POS(vict) == POS_SITTING) {
    send_to_char(ch, "Your target is already prone!\r\n");
    return;
  }
  if ((GET_SIZE(ch) - GET_SIZE(vict)) >= 2) {
    send_to_char(ch, "Your target is too small!\r\n");
    return;
  }
  if ((GET_SIZE(vict) - GET_SIZE(ch)) >= 2) {
    send_to_char(ch, "Your target is too big!\r\n");
    return;
  }
  if (AFF_FLAGGED(vict, AFF_FLYING)) {
    send_to_char(ch, "Impossible, your target is flying!\r\n");
    return;
  }

  percent = rand_number(1, 101);	/* 101% is a complete failure */
  prob = GET_SKILL(ch, SKILL_TRIP);

  if (MOB_FLAGGED(vict, MOB_NOBASH) && !GET_SKILL(ch, SKILL_IMPROVED_TRIP)) {
    send_to_char(ch, "You realize too late that you will probably not succeed:  ");
    percent = 101;
  }

  if (!IS_NPC(ch) && GET_SKILL(ch, SKILL_IMPROVED_TRIP))
    prob += GET_SKILL(ch, SKILL_IMPROVED_TRIP) / 5;

  if (!IS_NPC(vict) && compute_ability(vict, ABILITY_DISCIPLINE))
    percent += compute_ability(vict, ABILITY_DISCIPLINE);
  if (GET_RACE(ch) == RACE_DWARF ||
          GET_RACE(ch) == RACE_CRYSTAL_DWARF) // dwarf dwarven stability
    percent += 4;
  
  if (percent > prob) {
    GET_POS(ch) = POS_SITTING;
    damage(ch, vict, 0, SKILL_TRIP, DAM_FORCE, FALSE);
  } else {
    GET_POS(vict) = POS_SITTING;
    if (GET_SKILL(ch, SKILL_IMPROVED_TRIP)) {
      if (damage(ch, vict, GET_LEVEL(ch), SKILL_TRIP, DAM_FORCE, FALSE) > 0)
        WAIT_STATE(vict, PULSE_VIOLENCE * 2);
    } else if (damage(ch, vict, 1, SKILL_TRIP, DAM_FORCE, FALSE) > 0)
      WAIT_STATE(vict, PULSE_VIOLENCE * 2);
  }

  WAIT_STATE(ch, PULSE_VIOLENCE * 2);
}


ACMD(do_layonhands)
{
  char arg[MAX_INPUT_LENGTH];
  struct char_data *vict;

  if (IS_NPC(ch) || GET_LEVEL(ch) < LVL_IMMORT) {
    send_to_char(ch, "You have no idea how to do that.\r\n");
    return;
  }
  one_argument(argument, arg);

  if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM))) {
    send_to_char(ch, "Whom do you want to lay hands on?\r\n");
    return;
  }

  if (char_has_mud_event(ch, eLAYONHANDS)) {
    send_to_char(ch, "You must wait longer before you can use this ability again.\r\n");
    send_to_char(ch, "OOC:  The cooldown is approximately 6 minutes.\r\n");
    return;
  }

  send_to_char(ch, "Your hands flash \tWbright white\tn as you reach out...\r\n");
  act("You are \tWhealed\tn by $N!", FALSE, vict, 0, ch, TO_CHAR);
  act("$n \tWheals\tn $N!", FALSE, ch, 0, vict, TO_NOTVICT);
  attach_mud_event(new_mud_event(eLAYONHANDS, ch, NULL), 2 * SECS_PER_MUD_DAY);
  GET_HIT(vict) = GET_MAX_HIT(vict);
  update_pos(vict);
}


ACMD(do_crystalfist)
{
  if (GET_RACE(ch) != RACE_CRYSTAL_DWARF) {
    send_to_char(ch, "How do you plan on doing that?\r\n");
    return;
  }

  if (char_has_mud_event(ch, eCRYSTALFIST)) {
    send_to_char(ch, "You must wait longer before you can use "
            "this ability again.\r\n");
    return;
  }

  send_to_char(ch, "\tCYour hands and harms grow LARGE crystals!\tn\r\n");
  act("\tCYou watch as $n's arms and hands grow LARGE crystals!\tn",
          FALSE, ch, 0, 0, TO_NOTVICT);
  attach_mud_event(new_mud_event(eCRYSTALFIST, ch, NULL),
          (8 * SECS_PER_MUD_HOUR));
}


ACMD(do_crystalbody)
{
  if (GET_RACE(ch) != RACE_CRYSTAL_DWARF) {
    send_to_char(ch, "How do you plan on doing that?\r\n");
    return;
  }

  if (char_has_mud_event(ch, eCRYSTALBODY)) {
    send_to_char(ch, "You must wait longer before you can use "
            "this ability again.\r\n");
    return;
  }

  send_to_char(ch, "\tCYour crystal-like body becomes harder!\tn\r\n");
  act("\tCYou watch as $n's crystal-like body becomes harder!\tn",
          FALSE, ch, 0, 0, TO_NOTVICT);
  attach_mud_event(new_mud_event(eCRYSTALBODY, ch, NULL),
          (8 * SECS_PER_MUD_HOUR));
}



ACMD(do_treatinjury)
{
  char arg[MAX_INPUT_LENGTH];
  struct char_data *vict;

  if (IS_NPC(ch)) {
    send_to_char(ch, "You have no idea how to do that.\r\n");
    return;
  }
  one_argument(argument, arg);

  if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM))) {
    send_to_char(ch, "Whom do you want to treat?\r\n");
    return;
  }

  if (char_has_mud_event(ch, eTREATINJURY)) {
    send_to_char(ch, "You must wait longer before you can use this "
            "ability again.\r\n");
    return;
  }

  send_to_char(ch, "You skillfully dress the wounds...\r\n");
  act("Your injuries are \tWtreated\tn by $N!", FALSE, vict, 0, ch, TO_CHAR);
  act("$n \tWtreats\tn $N's injuries!", FALSE, ch, 0, vict, TO_NOTVICT);
  attach_mud_event(new_mud_event(eTREATINJURY, ch, NULL),
          (6 * SECS_PER_MUD_HOUR));
  GET_HIT(vict) += 10 + (compute_ability(ch, ABILITY_TREAT_INJURY) * 2);
  GET_MANA(vict) += 10 + (compute_ability(ch, ABILITY_TREAT_INJURY) * 2);
  update_pos(vict);
}


ACMD(do_rescue)
{
  char arg[MAX_INPUT_LENGTH];
  struct char_data *vict, *tmp_ch;
  int percent, prob;

  if (IS_NPC(ch) || !GET_SKILL(ch, SKILL_RESCUE)) {
    send_to_char(ch, "You have no idea how to do that.\r\n");
    return;
  }

  one_argument(argument, arg);

  if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM))) {
    send_to_char(ch, "Whom do you want to rescue?\r\n");
    return;
  }
  if (vict == ch) {
    send_to_char(ch, "What about fleeing instead?\r\n");
    return;
  }
  if (FIGHTING(ch) == vict) {
    send_to_char(ch, "How can you rescue someone you are trying to kill?\r\n");
    return;
  }
  for (tmp_ch = world[IN_ROOM(ch)].people; tmp_ch &&
       (FIGHTING(tmp_ch) != vict); tmp_ch = tmp_ch->next_in_room);

  if ((FIGHTING(vict) != NULL) && (FIGHTING(ch) == FIGHTING(vict)) && (tmp_ch == NULL)) {
     tmp_ch = FIGHTING(vict);
     if (FIGHTING(tmp_ch) == ch) {
     send_to_char(ch, "You have already rescued %s from %s.\r\n", GET_NAME(vict), GET_NAME(FIGHTING(ch)));
     return;
  }
  }

  if (!tmp_ch) {
    act("But nobody is fighting $M!", FALSE, ch, 0, vict, TO_CHAR);
    return;
  }
  percent = rand_number(1, 101);	/* 101% is a complete failure */
  prob = GET_SKILL(ch, SKILL_RESCUE);

  if (percent > prob) {
    send_to_char(ch, "You fail the rescue!\r\n");
    return;
  }
  send_to_char(ch, "Banzai!  To the rescue...\r\n");
  act("You are rescued by $N, you are confused!", FALSE, vict, 0, ch, TO_CHAR);
  act("$n heroically rescues $N!", FALSE, ch, 0, vict, TO_NOTVICT);

  if (FIGHTING(vict) == tmp_ch)
    stop_fighting(vict);
  if (FIGHTING(tmp_ch))
    stop_fighting(tmp_ch);
  if (FIGHTING(ch))
    stop_fighting(ch);

  set_fighting(ch, tmp_ch);
  set_fighting(tmp_ch, ch);

  WAIT_STATE(vict, 2 * PULSE_VIOLENCE);
}


  /* The "return" of the event function is the time until the event is called
   * again. If we return 0, then the event is freed and removed from the list, but
   * any other numerical response will be the delay until the next call */
EVENTFUNC(event_regen)
{
  struct char_data *ch, *tch = NULL;
  struct mud_event_data *pMudEvent;
  int hp = 1, found = 0;
	
  /* This is just a dummy check, but we'll do it anyway */
  if (event_obj == NULL)
    return 0;
	  
  /* For the sake of simplicity, we will place the event data in easily
   * referenced pointers */  
  pMudEvent = (struct mud_event_data *) event_obj;
  ch = (struct char_data *) pMudEvent->pStruct;    
  
  // poisoned, and dying people should suffer their damage from anyone they are
  // fighting in order that xp goes to the killer (who doesn't strike the last blow)
  // -zusuk
  if (AFF_FLAGGED(ch, AFF_POISON)) {
    if (FIGHTING(ch) || dice(1, 2) == 2) {
      for (tch = world[IN_ROOM(ch)].people; tch; tch = tch->next_in_room) {
        if (!IS_NPC(tch) && FIGHTING(tch) == ch) {
          damage(tch, ch, 1, SPELL_POISON, DAM_POISON, FALSE);
          found = 1;
          break;
        }
      }
      if (!found)
        damage(ch, ch, 1, SPELL_POISON, DAM_POISON, FALSE);
      update_pos(ch);
      return 0;
    }
  }
  found = 0; tch = NULL;
  if (GET_POS(ch) == POS_MORTALLYW) {
    for (tch = world[IN_ROOM(ch)].people; tch; tch = tch->next_in_room) {
      if (!IS_NPC(tch) && FIGHTING(tch) == ch) {
        damage(tch, ch, 1, TYPE_SUFFERING, DAM_RESERVED_DBC, FALSE);
        found = 1;
        break;
      }
    }
    if (!found)
      damage(ch, ch, 1, TYPE_SUFFERING, DAM_RESERVED_DBC, FALSE);
    update_pos(ch);
    return 0;
  }
  //50% chance you'll continue dying when incapacitated
  found = 0; tch = NULL;
  if (GET_POS(ch) == POS_INCAP && dice(1,2) == 2) {
    for (tch = world[IN_ROOM(ch)].people; tch; tch = tch->next_in_room) {
      if (!IS_NPC(tch) && FIGHTING(tch) == ch) {
        damage(tch, ch, 1, TYPE_SUFFERING, DAM_RESERVED_DBC, FALSE);
        found = 1;
        break;
      }
    }
    if (!found)
      damage(ch, ch, 1, TYPE_SUFFERING, DAM_RESERVED_DBC, FALSE);
    update_pos(ch);
    return 0;
  }

  //position, other bonuses
  if (GET_POS(ch) == POS_RESTING)
    hp += dice(1, 2);
  if (GET_POS(ch) == POS_SLEEPING)
    hp += dice(3, 2);

//      if (ROOM_FLAGGED(ch->in_room, ROOM_REGEN))
//        hp *= 2;
//      if (AFF_FLAGGED(ch, AFF_REGEN))
//        hp *= 2;

  // troll racial innate regeneration
  if (GET_RACE(ch) == RACE_TROLL) {
    hp *= 2;
    if (FIGHTING(ch))
      hp *= 2;
  }

  if (rand_number(0, 3) && GET_LEVEL(ch) <= LVL_IMMORT && !IS_NPC(ch) &&
	(GET_COND(ch, THIRST) == 0 || GET_COND(ch, HUNGER) == 0))
    hp = 0;

  if (IS_NPC(ch) && GET_LEVEL(ch) <= 5)
    hp = 0;

  if (GET_HIT(ch) > GET_MAX_HIT(ch)) {
    GET_HIT(ch)--;
  } else {
    GET_HIT(ch) = MIN(GET_HIT(ch) + hp, GET_MAX_HIT(ch));
  }
  if (GET_MOVE(ch) > GET_MAX_MOVE(ch)) {
    GET_MOVE(ch)--;
  } else {
    GET_MOVE(ch) = MIN(GET_MOVE(ch) + (hp*3), GET_MAX_MOVE(ch));
  }
  if (GET_MANA(ch) > GET_MAX_MANA(ch)) {
    GET_MANA(ch)--;
  } else {
    GET_MANA(ch) = MIN(GET_MANA(ch) + (hp*2), GET_MAX_MANA(ch));
  }

  update_pos(ch);  
  return 0;
}


EVENTFUNC(event_whirlwind)
{
  struct char_data *ch, *tch;
  struct mud_event_data *pMudEvent;
  struct list_data *room_list;
  int count;
	
  /* This is just a dummy check, but we'll do it anyway */
  if (event_obj == NULL)
    return 0;
	  
  /* For the sake of simplicity, we will place the event data in easily
   * referenced pointers */  
  pMudEvent = (struct mud_event_data *) event_obj;
  ch = (struct char_data *) pMudEvent->pStruct;    
  if (!IS_PLAYING(ch->desc))
    return 0;
  
  /* When using a list, we have to make sure to allocate the list as it
   * uses dynamic memory */
  room_list = create_list();
  
  /* We search through the "next_in_room", and grab all NPCs and add them
   * to our list */
  if (!IN_ROOM(ch))
    return 0;
  for (tch = world[IN_ROOM(ch)].people; tch; tch = tch->next_in_room)  
    if (IS_NPC(tch))
      add_to_list(tch, room_list);
      
  /* If our list is empty or has "0" entries, we free it from memory and
   * close off our event */    
  if (room_list->iSize == 0) {
    free_list(room_list);
    send_to_char(ch, "There is no one in the room to whirlwind!\r\n");
    return 0;
  }
  
  if (GET_HIT(ch) < 1)
    return 0;

  /* We spit out some ugly colour, making use of the new colour options,
   * to let the player know they are performing their whirlwind strike */
  send_to_char(ch, "\t[f313]You deliver a vicious \t[f014]\t[b451]WHIRLWIND!!!\tn\r\n");
  
  /* Lets grab some a random NPC from the list, and hit() them up */
  for (count = dice(1, 4); count > 0; count--) {
    tch = random_from_list(room_list);
    hit(ch, tch, TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, FALSE);
  }
  
  /* Now that our attack is done, let's free out list */
  free_list(room_list);
  
  /* The "return" of the event function is the time until the event is called
   * again. If we return 0, then the event is freed and removed from the list, but
   * any other numerical response will be the delay until the next call */
  if (GET_SKILL(ch, SKILL_WHIRLWIND) < rand_number(1, 101)) {
    send_to_char(ch, "You stop spinning.\r\n");
    return 0;
  } else if (GET_SKILL(ch, SKILL_IMPROVED_WHIRL))
      return 1.5 * PASSES_PER_SEC;
    else
      return 4 * PASSES_PER_SEC;
}

/* The "Whirlwind" skill is designed to provide a basic understanding of the
 * mud event and list systems. This is in NO WAY a balanced skill. */
ACMD(do_whirlwind)
{
  
  if (IS_NPC(ch) || !GET_SKILL(ch, SKILL_WHIRLWIND)) {
    send_to_char(ch, "You have no idea how.\r\n");
    return;
  }
  
  if (GET_POS(ch) < POS_FIGHTING) {
    send_to_char(ch, "You must be on your feet to perform a whirlwind.\r\n");
    return;    
  }

  /* First thing we do is check to make sure the character is not in the middle
   * of a whirl wind attack.
   * 
   * "char_had_mud_event() will sift through the character's event list to see if
   * an event of type "eWHIRLWIND" currently exists. */
  if (char_has_mud_event(ch, eWHIRLWIND)) {
    send_to_char(ch, "You are already attempting that!\r\n");
    return;   
  }

  send_to_char(ch, "You begin to spin rapidly in circles.\r\n");
  act("$N begins to rapidly spin in a circle!", FALSE, ch, 0, 0, TO_ROOM);
  
  /* NEW_EVENT() will add a new mud event to the event list of the character.
   * This function below adds a new event of "eWHIRLWIND", to "ch", and passes "NULL" as
   * additional data. The event will be called in "3 * PASSES_PER_SEC" or 3 seconds */
  NEW_EVENT(eWHIRLWIND, ch, NULL, 3 * PASSES_PER_SEC);
  WAIT_STATE(ch, PULSE_VIOLENCE * 3);
}


ACMD(do_stunningfist)
{
  char arg[MAX_INPUT_LENGTH];
  struct char_data *vict;
  int percent, prob;

  if (IS_NPC(ch) || !GET_SKILL(ch, SKILL_STUNNING_FIST)) {
    send_to_char(ch, "You have no idea how.\r\n");
    return;
  }

  one_argument(argument, arg);

  if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM))) {
    if (FIGHTING(ch) && IN_ROOM(ch) == IN_ROOM(FIGHTING(ch))) {
      vict = FIGHTING(ch);
    } else {
      send_to_char(ch, "Use your stunning fist on who?\r\n");
      return;
    }
  }
  if (vict == ch) {
    send_to_char(ch, "Aren't we funny today...\r\n");
    return;
  }
  if (char_has_mud_event(ch, eSTUNNED)) {
    send_to_char(ch, "Your target is already stunned...\r\n");
    return;
  }
  if (char_has_mud_event(ch, eSTUNNINGFIST)) {
    send_to_char(ch, "You must wait longer before you can use this ability again.\r\n");
    send_to_char(ch, "OOC:  The cooldown is approximately 5 minutes.\r\n");
    return;
  }
  
  /* 101% is a complete failure */
  percent = rand_number(1, 101);
  prob = GET_SKILL(ch, SKILL_STUNNING_FIST);

  if (!IS_NPC(vict) && compute_ability(vict, ABILITY_DISCIPLINE))
    percent += compute_ability(vict, ABILITY_DISCIPLINE);

  if (percent > prob) {
    damage(ch, vict, 0, SKILL_STUNNING_FIST, DAM_FORCE, FALSE);
  } else {
    damage(ch, vict, (dice(1,8)+GET_DAMROLL(ch)), SKILL_STUNNING_FIST,
           DAM_FORCE, FALSE);
    attach_mud_event(new_mud_event(eSTUNNED, vict, NULL), 4 * PASSES_PER_SEC);
  }
  attach_mud_event(new_mud_event(eSTUNNINGFIST, ch, NULL), 300 * PASSES_PER_SEC);

  WAIT_STATE(ch, PULSE_VIOLENCE * 3);
}


ACMD(do_kick)
{
  char arg[MAX_INPUT_LENGTH];
  struct char_data *vict;
  int percent, prob;

  if (IS_NPC(ch) || !GET_SKILL(ch, SKILL_KICK)) {
    send_to_char(ch, "You have no idea how.\r\n");
    return;
  }

  one_argument(argument, arg);

  if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM))) {
    if (FIGHTING(ch) && IN_ROOM(ch) == IN_ROOM(FIGHTING(ch))) {
      vict = FIGHTING(ch);
    } else {
      send_to_char(ch, "Kick who?\r\n");
      return;
    }
  }
  if (vict == ch) {
    send_to_char(ch, "Aren't we funny today...\r\n");
    return;
  }
  /* 101% is a complete failure */
  percent = rand_number(1, 101);
  prob = GET_SKILL(ch, SKILL_KICK);

  if (!IS_NPC(vict) && compute_ability(vict, ABILITY_DISCIPLINE))
    percent += compute_ability(vict, ABILITY_DISCIPLINE);

  if (percent > prob) {
    damage(ch, vict, 0, SKILL_KICK, DAM_FORCE, FALSE);
  } else
    damage(ch, vict, GET_LEVEL(ch) * 2, SKILL_KICK, DAM_FORCE, FALSE);

  WAIT_STATE(ch, PULSE_VIOLENCE * 3);
}
