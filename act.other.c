/**************************************************************************
*  File: act.other.c                                       Part of tbaMUD *
*  Usage: Miscellaneous player-level commands.                             *
*                                                                         *
*  All rights reserved.  See license for complete information.            *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
**************************************************************************/

/* needed by sysdep.h to allow for definition of <sys/stat.h> */
#define __ACT_OTHER_C__

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "spells.h"
#include "screen.h"
#include "house.h"
#include "constants.h"
#include "dg_scripts.h"
#include "act.h"
#include "spec_procs.h"
#include "class.h"
#include "fight.h"
#include "mail.h"  /* for has_mail() */
#include "shop.h"
#include "quest.h"
#include "modify.h"
#include "race.h"
#include "clan.h"
/* Local defined utility functions */
/* do_group utility functions */
static void print_group(struct char_data *ch);
static void display_group_list(struct char_data * ch);



ACMD(do_recharge) {
  char buf[MAX_INPUT_LENGTH];
  struct obj_data *obj;
  int maxcharge, mincharge, chargeval;

  argument = one_argument(argument, buf);

  if (!(obj = get_obj_in_list_vis(ch, buf, NULL, ch->carrying))) {
      send_to_char(ch, "You don't have that!\r\n");
      return;
  }

  if (GET_OBJ_TYPE(obj) != ITEM_STAFF &&
        GET_OBJ_TYPE(obj) != ITEM_WAND) {
      send_to_char(ch, "Are you daft!  You can't recharge that!\r\n");
      return;
  }

  if (GET_GOLD(ch) < 5000) {
    send_to_char(ch, "You don't have enough gold!\r\n");
    return;
  }

  if (CASTER_LEVEL(ch) < 15) {
    send_to_char(ch, "You don't know enough about magicks!\r\n");
    return;
  }

  maxcharge = GET_OBJ_VAL(obj, 1);
  mincharge = GET_OBJ_VAL(obj, 2);
 
  if (mincharge < maxcharge) {
    chargeval = maxcharge - mincharge;
    GET_OBJ_VAL(obj, 2) += chargeval;
    GET_GOLD(ch) -= 10000;
    send_to_char(ch, "Shazzzaaaam!\r\n");
    sprintf(buf, "The item now has %d charges remaining.\r\n", maxcharge);
    send_to_char(ch, buf);
    act("$n with a flash recharges $p.",
          FALSE, ch, obj, 0, TO_ROOM);
  } else {
    send_to_char(ch, "The item does not need recharging.\r\n");
  }
  return;
}


ACMD(do_mount) {
  char arg[MAX_INPUT_LENGTH];
  struct char_data *vict;
  
  one_argument(argument, arg);
  
  if (!*arg) {
    send_to_char(ch, "Mount who?\r\n");
    return;
  } else if (!(vict = get_char_room_vis(ch, arg, NULL))) {
    send_to_char(ch, "There is no-one by that name here.\r\n");
    return;
  } else if (!IS_NPC(vict) && GET_LEVEL(ch) < LVL_IMMORT) {
    send_to_char(ch, "Ehh... no.\r\n");
    return;
  } else if (RIDING(ch) || RIDDEN_BY(ch)) {
    send_to_char(ch, "You are already mounted.\r\n");
    return;
  } else if (RIDING(vict) || RIDDEN_BY(vict)) {
    send_to_char(ch, "It is already mounted.\r\n");
    return;
  } else if (GET_LEVEL(ch) < LVL_IMMORT && IS_NPC(vict) && !MOB_FLAGGED(vict, MOB_MOUNTABLE)) {
    send_to_char(ch, "You can't mount that!\r\n");
    return;
  } else if (!GET_ABILITY(ch, ABILITY_MOUNT)) {
    send_to_char(ch, "First you need to learn *how* to mount.\r\n");
    return;
  } else if (GET_ABILITY(ch, ABILITY_MOUNT) <= rand_number(1, GET_LEVEL(vict))) {
    act("You try to mount $N, but slip and fall off.", FALSE, ch, 0, vict, TO_CHAR);
    act("$n tries to mount you, but slips and falls off.", FALSE, ch, 0, vict, TO_VICT);
    act("$n tries to mount $N, but slips and falls off.", TRUE, ch, 0, vict, TO_NOTVICT);
    damage(ch, ch, dice(1, 2), -1, -1, -1);
    return;
  }
  
  act("You mount $N.", FALSE, ch, 0, vict, TO_CHAR);
  act("$n mounts you.", FALSE, ch, 0, vict, TO_VICT);
  act("$n mounts $N.", TRUE, ch, 0, vict, TO_NOTVICT);
  mount_char(ch, vict);
  
  if (IS_NPC(vict) && !AFF_FLAGGED(vict, AFF_TAMED) &&
	GET_ABILITY(ch, ABILITY_MOUNT) <= rand_number(1, GET_LEVEL(vict))) {
    act("$N suddenly bucks upwards, throwing you violently to the ground!", FALSE, ch, 0, vict, TO_CHAR);
    act("$n is thrown to the ground as $N violently bucks!", TRUE, ch, 0, vict, TO_NOTVICT);
    act("You buck violently and throw $n to the ground.", FALSE, ch, 0, vict, TO_VICT);
    dismount_char(ch);
    damage(vict, ch, dice(1,3), -1, -1, -1);
  }
}


ACMD(do_dismount) {
  if (!RIDING(ch)) {
    send_to_char(ch, "You aren't even riding anything.\r\n");
    return;
  } else if (SECT(ch->in_room) == SECT_WATER_NOSWIM && !has_boat(ch)) {
    send_to_char(ch, "Yah, right, and then drown...\r\n");
    return;
  }
  
  act("You dismount $N.", FALSE, ch, 0, RIDING(ch), TO_CHAR);
  act("$n dismounts from you.", FALSE, ch, 0, RIDING(ch), TO_VICT);
  act("$n dismounts $N.", TRUE, ch, 0, RIDING(ch), TO_NOTVICT);
  dismount_char(ch);
}


ACMD(do_buck) {
  if (!RIDDEN_BY(ch)) {
    send_to_char(ch, "You're not even being ridden!\r\n");
    return;
  } else if (AFF_FLAGGED(ch, AFF_TAMED)) {
    send_to_char(ch, "But you're tamed!\r\n");
    return;
  }
  
  act("You quickly buck, throwing $N to the ground.", FALSE, ch, 0, RIDDEN_BY(ch), TO_CHAR);
  act("$n quickly bucks, throwing you to the ground.", FALSE, ch, 0, RIDDEN_BY(ch), TO_VICT);
  act("$n quickly bucks, throwing $N to the ground.", FALSE, ch, 0, RIDDEN_BY(ch), TO_NOTVICT);
  GET_POS(RIDDEN_BY(ch)) = POS_SITTING;
  if (rand_number(0, 4)) {
    send_to_char(RIDDEN_BY(ch), "You hit the ground hard!\r\n");
    damage(RIDDEN_BY(ch), RIDDEN_BY(ch), dice(2,4), -1, -1, -1);
  }
  dismount_char(ch);
  
  
  /*
   * you might want to call set_fighting() or some nonsense here if you
   * want the mount to attack the unseated rider or vice-versa.
   */
}


ACMD(do_tame) {
  char arg[MAX_INPUT_LENGTH];
  struct affected_type af;
  struct char_data *vict;
  
  one_argument(argument, arg);
  
  if (!*arg) {
    send_to_char(ch, "Tame who?\r\n");
    return;
  } else if (!(vict = get_char_room_vis(ch, arg, NULL))) {
    send_to_char(ch, "They're not here.\r\n");
    return;
  } else if (GET_LEVEL(ch) < LVL_IMMORT && IS_NPC(vict) && !MOB_FLAGGED(vict, MOB_MOUNTABLE)) {
    send_to_char(ch, "You can't do that to them.\r\n");
    return;
  } else if (!GET_ABILITY(ch, ABILITY_TAME)) {
    send_to_char(ch, "You don't even know how to tame something.\r\n");
    return;
  } else if (!IS_NPC(vict) && GET_LEVEL(ch) < LVL_IMMORT) {
    send_to_char(ch, "You can't do that.\r\n");
    return;
  } else if (GET_SKILL(ch, ABILITY_TAME) <= rand_number(1, GET_LEVEL(vict))) {
    send_to_char(ch, "You fail to tame it.\r\n");
    return;
  }

  new_affect(&af);  
  af.duration = 50 + GET_ABILITY(ch, ABILITY_TAME) * 4;
  SET_BIT_AR(af.bitvector, AFF_TAMED);
  affect_to_char(vict, &af);
  
  act("You tame $N.", FALSE, ch, 0, vict, TO_CHAR);
  act("$n tames you.", FALSE, ch, 0, vict, TO_VICT);
  act("$n tames $N.", FALSE, ch, 0, vict, TO_NOTVICT);
}

// if you meet the class pre-reqs, return 1, otherwise 0
int meet_class_reqs(struct char_data *ch, int class)
{
  switch (class) {
    case CLASS_MAGIC_USER:
      if (ch->real_abils.intel >= 11)
        return 1;
      
      break;
    case CLASS_CLERIC:
      if (ch->real_abils.wis >= 11)
        return 1;
      break;
    case CLASS_DRUID:
      if (ch->real_abils.wis >= 11)
        return 1;
      break;
    case CLASS_THIEF:
      if (ch->real_abils.dex >= 11)
        return 1;
      break;
    case CLASS_WARRIOR:
      if (ch->real_abils.str >= 11)
        return 1;
      break;
    case CLASS_MONK:
      if (ch->real_abils.dex >= 11 && ch->real_abils.wis >= 11)
        return 1;
      break;
  }
  return 0;
}


void list_valid_classes(struct char_data *ch)
{
  int i;

  for (i = 0; i < NUM_CLASSES; i++) {
    if (meet_class_reqs(ch, i)) {
      send_to_char(ch, "%d) %s\r\n", i, pc_class_types[i]);
    }
  }
  send_to_char(ch, "\r\n");
}


ACMD(do_respec)
{
  char arg[MAX_INPUT_LENGTH];
  int class = -1;

  if (IS_NPC(ch) || !ch->desc)
    return;

  one_argument(argument, arg);


  if (!*arg) {
    send_to_char(ch, "You need to select a starting class to respec to,"
                     " here are your options:\r\n");
    list_valid_classes(ch);
    return;
  } else {
    class = get_class_by_name(arg);
    if (class == -1) {
      send_to_char(ch, "Invalid class.\r\n");
      list_valid_classes(ch);
      return;
    }
    if (class >= NUM_CLASSES || !meet_class_reqs(ch, class)) {
      send_to_char(ch, "That is not a valid class!  These are valid choices:\r\n");
      list_valid_classes(ch);
      return;   
    }
    if (GET_LEVEL(ch) < 2) {
      send_to_char(ch, "You need to be at least 2nd level to respec...\r\n");
      return;
    }
    if (GET_LEVEL(ch) >= LVL_IMMORT) {
      send_to_char(ch, "Sorry staff can't respec...\r\n");
      return;
    }

    int tempXP = GET_EXP(ch);
    GET_CLASS(ch) = class;
    do_start(ch);
    GET_EXP(ch) = tempXP;
    send_to_char(ch, "\tMYou have respec'd!\tn\r\n");
    send_to_char(ch, "\tDType 'gain' to regain your level(s)...\tn\r\n");
  }
}


#define MULTICAP	3
ACMD(do_gain)
{
  char arg[MAX_INPUT_LENGTH];
  int is_altered = FALSE, num_levels = 0;
  int class = -1, i, classCount = 0;

  if (IS_NPC(ch) || !ch->desc)
    return;
    
  one_argument(argument, arg);
    
  if (!*arg) {
    send_to_char(ch, "You need to select a class, here are your options:\r\n");
    list_valid_classes(ch);
    return;
  } else {
    class = get_class_by_name(arg);
    if (class == -1) {
      send_to_char(ch, "Invalid class.\r\n");
      list_valid_classes(ch);
      return;
    }

    if (class < 0 || class >= NUM_CLASSES || !meet_class_reqs(ch, class)) {
      send_to_char(ch, "That is not a valid class!  These are valid choices:\r\n");
      list_valid_classes(ch);
      return;
    }

    //multi class cap
    for (i = 0; i < MAX_CLASSES; i++) {
      if (CLASS_LEVEL(ch, i) && i != class)
        classCount++;
    }
    if (classCount >= MULTICAP) {
      send_to_char(ch, "Current cap on multi-classing is %d.\r\n", MULTICAP);
      send_to_char(ch, "Please select one of the classes you already have!\r\n");
      return;
    }

    if (GET_LEVEL(ch) < LVL_IMMORT - CONFIG_NO_MORT_TO_IMMORT &&
	GET_EXP(ch) >= level_exp(ch, GET_LEVEL(ch) + 1)) {
      GET_LEVEL(ch) += 1;
      CLASS_LEVEL(ch, class)++;   
      GET_CLASS(ch) = class;
      num_levels++;
      advance_level(ch, class);
      is_altered = TRUE;
    } else {
      send_to_char(ch, "You are unable to gain a level.\r\n");
      return;
    }
 
    if (is_altered) {
      mudlog(BRF, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE,
	"%s advanced %d level%s to level %d.", GET_NAME(ch),
	num_levels, num_levels == 1 ? "" : "s", GET_LEVEL(ch));
      if (num_levels == 1)
        send_to_char(ch, "You rise a level!\r\n");
      else
        send_to_char(ch, "You rise %d levels!\r\n", num_levels);
      set_title(ch, NULL);
      if (GET_LEVEL(ch) >= LVL_IMMORT && !PLR_FLAGGED(ch, PLR_NOWIZLIST))
        run_autowiz();

      send_to_char(ch, "\tMDon't forget to \tmTRAIN\tM, \tmPRACtice\tM and "
                       "\tmBOOST\tM your stats and skills!\tn\r\n");
    }
  }
}
#undef MULTICAP


void list_forms(struct char_data *ch)
{
  send_to_char(ch, "%s\r\n", npc_race_menu);
}


ACMD(do_shapechange)
{
  char arg[MAX_INPUT_LENGTH];
  int form = -1;

  if (IS_NPC(ch) || !ch->desc)
    return;

  if (CLASS_LEVEL(ch, CLASS_DRUID) < 5) {
    send_to_char(ch, "You are not a high enough level druid to do this...\r\n");
    return;
  }

  one_argument(argument, arg);

  if (!*arg) {
    if (!IS_MORPHED(ch)) {
      send_to_char(ch, "You are already in your natural form!\r\n");
    } else {
      send_to_char(ch, "You shift back into your natural form...\r\n");
      act("$n shifts back to his natural form.", TRUE, ch, 0, 0, TO_ROOM);    
      IS_MORPHED(ch) = 0;
    }
    list_forms(ch);
  } else {
    form = atoi(arg);
    if (form < 1 || form > NUM_NPC_RACES - 1) {
      send_to_char(ch, "That is not a valid race!\r\n");
      list_forms(ch);
      return;
    }
    IS_MORPHED(ch) = form;
    send_to_char(ch, "You transform into a %s!\r\n", RACE_ABBR(ch));
    send_to_char(ch, "\tDType 'innates' to see your abilities.\tn\r\n"); 
    act("$n shapechanges!", TRUE, ch, 0, 0, TO_ROOM);    
  }

}


ACMD(do_quit)
{
  if (IS_NPC(ch) || !ch->desc)
    return;

  if (subcmd != SCMD_QUIT && GET_LEVEL(ch) < LVL_IMMORT)
    send_to_char(ch, "You have to type quit--no less, to quit!\r\n");
  else if (GET_POS(ch) == POS_FIGHTING)
    send_to_char(ch, "No way!  You're fighting for your life!\r\n");
  else if (GET_POS(ch) < POS_STUNNED) {
    send_to_char(ch, "You die before your time...\r\n");
    die(ch, NULL);
  } else {
    act("$n has left the game.", TRUE, ch, 0, 0, TO_ROOM);
    mudlog(NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE, "%s has quit the game.", GET_NAME(ch));

    if (GET_QUEST_TIME(ch) != -1)
      quest_timeout(ch);

    send_to_char(ch, "Goodbye, friend.. Come back soon!\r\n");

    /* We used to check here for duping attempts, but we may as well do it right
     * in extract_char(), since there is no check if a player rents out and it
     * can leave them in an equally screwy situation. */

    if (CONFIG_FREE_RENT)
      Crash_rentsave(ch, 0);

    GET_LOADROOM(ch) = GET_ROOM_VNUM(IN_ROOM(ch));

    /* Stop snooping so you can't see passwords during deletion or change. */
    if (ch->desc->snoop_by) {
      write_to_output(ch->desc->snoop_by, "Your victim is no longer among us.\r\n");
      ch->desc->snoop_by->snooping = NULL;
      ch->desc->snoop_by = NULL;
    }

    extract_char(ch);		/* Char is saved before extracting. */
  }
}

ACMD(do_save)
{
  if (IS_NPC(ch) || !ch->desc)
    return;

  send_to_char(ch, "Saving %s.\r\n", GET_NAME(ch));
  save_char(ch);
  Crash_crashsave(ch);
  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_HOUSE_CRASH))
    House_crashsave(GET_ROOM_VNUM(IN_ROOM(ch)));
  GET_LOADROOM(ch) = GET_ROOM_VNUM(IN_ROOM(ch));
}

/* Generic function for commands which are normally overridden by special
 * procedures - i.e., shop commands, mail commands, etc. */
ACMD(do_not_here)
{
  send_to_char(ch, "Sorry, but you cannot do that here!\r\n");
}


ACMD(do_lore)
{
  char arg[MAX_INPUT_LENGTH];
  struct char_data *tch = NULL;
  struct obj_data *tobj = NULL;
  int target;

  if (IS_NPC(ch))
    return;

  if (!IS_NPC(ch) && !GET_ABILITY(ch, ABILITY_LORE)) {
    send_to_char(ch, "You have no ability to do that!\r\n");
    return;
  }

  one_argument(argument, arg);

  target = generic_find(arg, FIND_CHAR_ROOM | FIND_OBJ_INV | FIND_OBJ_ROOM | 
	FIND_OBJ_EQUIP, ch, &tch, &tobj);

  if (*arg) {
    if (!target) {
      act("There is nothing to here to use your Lore ability on...", FALSE,
            ch, NULL, NULL, TO_CHAR);
      return;
    }
  } else {
    tch = ch;
  }

  send_to_char(ch, "You attempt to utilize your vast knowledge of lore...\r\n");
  WAIT_STATE(ch, PULSE_VIOLENCE);

  if (tobj && GET_OBJ_COST(tobj) > lore_app[compute_ability(ch, ABILITY_LORE)]) {
    send_to_char(ch, "Your knowledge is not extensive enough to know about this object!\r\n");
    return;
  }
  if (tch && GET_LEVEL(tch) > compute_ability(ch, ABILITY_LORE)) {
    send_to_char(ch, "Your knowledge is not extensive enough to know about this creature!\r\n");
    return;
  }

  //level ch tch tobj
  call_magic(ch, tch, tobj, SPELL_IDENTIFY, GET_LEVEL(ch), CAST_SPELL);
}


ACMD(do_sneak)
{
  if (AFF_FLAGGED(ch, AFF_GRAPPLED)) {
    send_to_char(ch, "You are unable to move to make your attempt!\r\n");
    return;
  }

  if (IS_NPC(ch) || !GET_ABILITY(ch, ABILITY_SNEAK)) {
    send_to_char(ch, "You have no idea how to do that.\r\n");
    return;
  }

  if (AFF_FLAGGED(ch, AFF_SNEAK)) {
    REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_SNEAK);
    send_to_char(ch, "You stop sneaking...\r\n");
    return;
  }

  send_to_char(ch, "Okay, you'll try to move silently for a while.\r\n");

  SET_BIT_AR(AFF_FLAGS(ch), AFF_SNEAK);
}


ACMD(do_hide)
{
  if (AFF_FLAGGED(ch, AFF_GRAPPLED)) {
    send_to_char(ch, "You are unable to move to make your attempt!\r\n");
    return;
  }

  if (IS_NPC(ch) || !GET_ABILITY(ch, ABILITY_HIDE)) {
    send_to_char(ch, "You have no idea how to do that.\r\n");
    return;
  }

  if (AFF_FLAGGED(ch, AFF_HIDE)) {
    REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_HIDE);
    send_to_char(ch, "You step out of the shadows...\r\n");
    return;
  }

  send_to_char(ch, "You attempt to hide yourself.\r\n");

  SET_BIT_AR(AFF_FLAGS(ch), AFF_HIDE);
}

ACMD(do_steal)
{
  struct char_data *vict;
  struct obj_data *obj;
  char vict_name[MAX_INPUT_LENGTH], obj_name[MAX_INPUT_LENGTH];
  int percent, gold, eq_pos, pcsteal = 0, ohoh = 0;

  if (IS_NPC(ch) || !GET_SKILL(ch, SKILL_STEAL)) {
    send_to_char(ch, "You have no idea how to do that.\r\n");
    return;
  }
  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL)) {
    send_to_char(ch, "This room just has such a peaceful, easy feeling...\r\n");
    return;
  }

  two_arguments(argument, obj_name, vict_name);

  if (!(vict = get_char_vis(ch, vict_name, NULL, FIND_CHAR_ROOM))) {
    send_to_char(ch, "Steal what from who?\r\n");
    return;
  } else if (vict == ch) {
    send_to_char(ch, "Come on now, that's rather stupid!\r\n");
    return;
  }

  /* 101% is a complete failure */
  percent = rand_number(1, 101) - GET_DEX_BONUS(ch);

  if (GET_POS(vict) < POS_SLEEPING)
    percent = -1;		/* ALWAYS SUCCESS, unless heavy object. */

  if (!CONFIG_PT_ALLOWED && !IS_NPC(vict))
    pcsteal = 1;

  if (!AWAKE(vict))	/* Easier to steal from sleeping people. */
    percent -= 50;

  /* No stealing if not allowed. If it is no stealing from Imm's or Shopkeepers. */
  if (GET_LEVEL(vict) >= LVL_IMMORT || pcsteal || GET_MOB_SPEC(vict) == shop_keeper)
    percent = 101;		/* Failure */

  if (str_cmp(obj_name, "coins") && str_cmp(obj_name, "gold")) {

    if (!(obj = get_obj_in_list_vis(ch, obj_name, NULL, vict->carrying))) {

      for (eq_pos = 0; eq_pos < NUM_WEARS; eq_pos++)
	if (GET_EQ(vict, eq_pos) &&
	    (isname(obj_name, GET_EQ(vict, eq_pos)->name)) &&
	    CAN_SEE_OBJ(ch, GET_EQ(vict, eq_pos))) {
	  obj = GET_EQ(vict, eq_pos);
	  break;
	}
      if (!obj) {
	act("$E hasn't got that item.", FALSE, ch, 0, vict, TO_CHAR);
	return;
      } else {			/* It is equipment */
	if ((GET_POS(vict) > POS_STUNNED)) {
	  send_to_char(ch, "Steal the equipment now?  Impossible!\r\n");
	  return;
	} else {
          if (!give_otrigger(obj, vict, ch) ||
              !receive_mtrigger(ch, vict, obj) ) {
            send_to_char(ch, "Impossible!\r\n");
            return;
          }
	  act("You unequip $p and steal it.", FALSE, ch, obj, 0, TO_CHAR);
	  act("$n steals $p from $N.", FALSE, ch, obj, vict, TO_NOTVICT);
	  obj_to_char(unequip_char(vict, eq_pos), ch);
	}
      }
    } else {			/* obj found in inventory */

      percent += GET_OBJ_WEIGHT(obj);	/* Make heavy harder */

      if (percent > GET_SKILL(ch, SKILL_STEAL)) {
	ohoh = TRUE;
	send_to_char(ch, "Oops..\r\n");
	act("$n tried to steal something from you!", FALSE, ch, 0, vict, TO_VICT);
	act("$n tries to steal something from $N.", TRUE, ch, 0, vict, TO_NOTVICT);
      } else {			/* Steal the item */
	if (IS_CARRYING_N(ch) + 1 < CAN_CARRY_N(ch)) {
          if (!give_otrigger(obj, vict, ch) ||
              !receive_mtrigger(ch, vict, obj) ) {
            send_to_char(ch, "Impossible!\r\n");
            return;
          }
	  if (IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(obj) < CAN_CARRY_W(ch)) {
	    obj_from_char(obj);
	    obj_to_char(obj, ch);
	    send_to_char(ch, "Got it!\r\n");
	  }
	} else
	  send_to_char(ch, "You cannot carry that much.\r\n");
      }
    }
  } else {			/* Steal some coins */
    if (AWAKE(vict) && (percent > GET_SKILL(ch, SKILL_STEAL))) {
      ohoh = TRUE;
      send_to_char(ch, "Oops..\r\n");
      act("You discover that $n has $s hands in your wallet.", FALSE, ch, 0, vict, TO_VICT);
      act("$n tries to steal gold from $N.", TRUE, ch, 0, vict, TO_NOTVICT);
    } else {
      /* Steal some gold coins */
      gold = (GET_GOLD(vict) * rand_number(1, 10)) / 100;
      gold = MIN(1782, gold);
      if (gold > 0) {
		increase_gold(ch, gold);
		decrease_gold(vict, gold);
        if (gold > 1)
	  send_to_char(ch, "Bingo!  You got %d gold coins.\r\n", gold);
	else
	  send_to_char(ch, "You manage to swipe a solitary gold coin.\r\n");
      } else {
	send_to_char(ch, "You couldn't get any gold...\r\n");
      }
    }
  }

  if (ohoh && IS_NPC(vict) && AWAKE(vict))
    hit(vict, ch, TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, FALSE);
}


ACMD(do_spells)
{
  char arg[MAX_INPUT_LENGTH];
  int class = -1;

  if (IS_NPC(ch))
    return;

  one_argument(argument, arg);

  if (!*arg) {
    send_to_char(ch, "The spells command requires a class name as an argument.\r\n");
    return;
  } else {
    class = get_class_by_name(arg);
    if (class < 0 || class >= NUM_CLASSES) {
      send_to_char(ch, "That is not a valid class!\r\n");
      return; 
    }
    if (CLASS_LEVEL(ch, class)) {
      list_spells(ch, 0, class);
    } else {
      send_to_char(ch, "You don't have any levels in that class.\r\n");
    }
  }

  send_to_char(ch, "\tDType 'practice' to see your skills\tn\r\n");
  send_to_char(ch, "\tDType 'train' to see your abilities\tn\r\n");
  send_to_char(ch, "\tDType 'spelllist <classname>' to see all your class spells\tn\r\n");
}


ACMD(do_spelllist)
{
  char arg[MAX_INPUT_LENGTH];
  int class = -1;

  if (IS_NPC(ch))
    return;

  one_argument(argument, arg);

  if (!*arg) {
    send_to_char(ch, "Usage:  spelllist <class name>\r\n");
    return; 
  } else {
    class = get_class_by_name(arg);
    if (class < 0 || class >= NUM_CLASSES) {
      send_to_char(ch, "That is not a valid class!\r\n");
      return; 
    }
    if (CLASS_LEVEL(ch, class)) {
      list_spells(ch, 1, class);
    } else {
      send_to_char(ch, "You don't have any levels in that class.\r\n");
    }
  }

  send_to_char(ch, "\tDType 'practice' to see your skills\tn\r\n");
  send_to_char(ch, "\tDType 'train' to see your abilities\tn\r\n");
  send_to_char(ch, "\tDType 'spells <classname>' to see your currently known spells\tn\r\n");
}


ACMD(do_boosts)
{
  char arg[MAX_INPUT_LENGTH];

  if (IS_NPC(ch))
    return;

  one_argument(argument, arg);

  if (*arg)
    send_to_char(ch, "You can only boost stats in your guild.\r\n");
  else
    send_to_char(ch, "\tCStat boost sessions remaining: %d\tn\r\n"
	"\tcStats:\tn\r\n"
        "Strength\r\n"
        "Constitution\r\n"
        "Dexterity\r\n" 
        "Intelligence\r\n"
        "Wisdom\r\n"
        "Charisma\r\n"
        "\tC*Reminder that you can only boost your stats in your guild.\tn\r\n"
        "\r\n",
	GET_BOOSTS(ch));
}


ACMD(do_practice)
{
  char arg[MAX_INPUT_LENGTH];

  if (IS_NPC(ch))
    return;

  one_argument(argument, arg);

  if (*arg)
    send_to_char(ch, "You can only practice skills in your guild.\r\n");
  else
    list_skills(ch);

  send_to_char(ch, "\tDType 'train' to see your abilities\tn\r\n");
  if (IS_CASTER(ch)) {
    send_to_char(ch, "\tDType 'spells' to see your spells\tn\r\n");
  }

}


ACMD(do_train)
{
  char arg[MAX_INPUT_LENGTH];

  if (IS_NPC(ch))
    return;

  one_argument(argument, arg);

  if (*arg)
    send_to_char(ch, "You can only train abilities in your guild.\r\n");
  else
    list_abilities(ch);

  send_to_char(ch, "\tDType 'practice' to see your skills\tn\r\n");
  if (IS_CASTER(ch)) {
    send_to_char(ch, "\tDType 'spells' to see your spells\tn\r\n");
  }
}


ACMD(do_visible)
{
  if (GET_LEVEL(ch) >= LVL_IMMORT) {
    perform_immort_vis(ch);
    return;
  }

  if AFF_FLAGGED(ch, AFF_INVISIBLE) {
    appear(ch);
    send_to_char(ch, "You break the spell of invisibility.\r\n");
  } else
    send_to_char(ch, "You are already visible.\r\n");
}

ACMD(do_title)
{
  skip_spaces(&argument);
  delete_doubledollar(argument);
  parse_at(argument);

  if (IS_NPC(ch))
    send_to_char(ch, "Your title is fine... go away.\r\n");
  else if (PLR_FLAGGED(ch, PLR_NOTITLE))
    send_to_char(ch, "You can't title yourself -- you shouldn't have abused it!\r\n");
  else if (strstr(argument, "(") || strstr(argument, ")"))
    send_to_char(ch, "Titles can't contain the ( or ) characters.\r\n");
  else if (strlen(argument) > MAX_TITLE_LENGTH)
    send_to_char(ch, "Sorry, titles can't be longer than %d characters.\r\n", MAX_TITLE_LENGTH);
  else {
    set_title(ch, argument);
    send_to_char(ch, "Okay, you're now %s%s%s.\r\n", GET_NAME(ch), *GET_TITLE(ch) ? " " : "", GET_TITLE(ch));
  }
}


static void print_group(struct char_data *ch)
{
  struct char_data *k;

  send_to_char(ch, "Your group consists of:\r\n");

  while ((k = (struct char_data *) simple_list(ch->group->members)) != NULL)
    send_to_char(ch, "%-*s: %s[%4d/%-4d]H [%4d/%-4d]M [%4d/%-4d]V%s\r\n",
	    count_color_chars(GET_NAME(k))+22, GET_NAME(k), 
	    GROUP_LEADER(GROUP(ch)) == k ? CBGRN(ch, C_NRM) : CCGRN(ch, C_NRM),
	    GET_HIT(k), GET_MAX_HIT(k),
	    GET_MANA(k), GET_MAX_MANA(k),
	    GET_MOVE(k), GET_MAX_MOVE(k),
	    CCNRM(ch, C_NRM));
}


static void display_group_list(struct char_data * ch)
{
  struct group_data * group;
  int count = 0;
	
  if (group_list->iSize) {
    send_to_char(ch, "#   Group Leader     # of Members    In Zone\r\n"
                     "---------------------------------------------------\r\n");
		
    while ((group = (struct group_data *) simple_list(group_list)) != NULL) {
			if (IS_SET(GROUP_FLAGS(group), GROUP_NPC))
			  continue;
      if (GROUP_LEADER(group) && !IS_SET(GROUP_FLAGS(group), GROUP_ANON))
        send_to_char(ch, "%-2d) %s%-12s     %-2d              %s%s\r\n", 
          ++count,
          IS_SET(GROUP_FLAGS(group), GROUP_OPEN) ? CCGRN(ch, C_NRM) : CCRED(ch, C_NRM), 
          GET_NAME(GROUP_LEADER(group)), group->members->iSize, 
		zone_table[world[IN_ROOM(GROUP_LEADER(group))].zone].name,
		CCNRM(ch, C_NRM));
      else
        send_to_char(ch, "%-2d) Hidden\r\n", ++count);
				
		}
  }
  if (count)
    send_to_char(ch, "\r\n"
                     "%sSeeking Members%s\r\n"
                     "%sClosed%s\r\n", 
                     CCGRN(ch, C_NRM), CCNRM(ch, C_NRM),
                     CCRED(ch, C_NRM), CCNRM(ch, C_NRM));
  else
    send_to_char(ch, "\r\n"
                     "Currently no groups formed.\r\n");
}


//vatiken's group system 1.2, installed 08/08/12
ACMD(do_group)
{
  char buf[MAX_STRING_LENGTH];
  struct char_data *vict;
  argument = one_argument(argument, buf);

  if (!*buf) {
    if (GROUP(ch))
      print_group(ch);
    else
      send_to_char(ch, "You must specify a group option, or type HELP GROUP for more info.\r\n");
    return;
  }
  
  if (is_abbrev(buf, "new")) {
    if (GROUP(ch))
      send_to_char(ch, "You are already in a group.\r\n");
    else
      create_group(ch);
  } else if (is_abbrev(buf, "list"))
    display_group_list(ch);
  else if (is_abbrev(buf, "join")) {
    skip_spaces(&argument);
    if (!(vict = get_char_vis(ch, argument, NULL, FIND_CHAR_ROOM))) {
      send_to_char(ch, "Join who?\r\n");
      return;
    } else if (vict == ch) {
      send_to_char(ch, "That would be one lonely grouping.\r\n");
      return;
    } else if (GROUP(ch)) {
      send_to_char(ch, "But you are already part of a group.\r\n");
      return;
    } else if (!GROUP(vict)) {
      send_to_char(ch, "They are not a part of a group!\r\n");
      return;
    } else if (!IS_SET(GROUP_FLAGS(GROUP(vict)), GROUP_OPEN)) {
      send_to_char(ch, "That group isn't accepting members.\r\n");
      return;
    }   
    join_group(ch, GROUP(vict)); 
  } else if (is_abbrev(buf, "kick")) {
    skip_spaces(&argument);
    if (!(vict = get_char_vis(ch, argument, NULL, FIND_CHAR_ROOM))) {
      send_to_char(ch, "Kick out who?\r\n");
      return;
    } else if (vict == ch) {
      send_to_char(ch, "There are easier ways to leave the group.\r\n");
      return;
    } else if (!GROUP(ch) ) {
      send_to_char(ch, "But you are not part of a group.\r\n");
      return;
    } else if (GROUP_LEADER(GROUP(ch)) != ch ) {
      send_to_char(ch, "Only the group's leader can kick members out.\r\n");
      return;
    } else if (GROUP(vict) != GROUP(ch)) {
      send_to_char(ch, "They are not a member of your group!\r\n");
      return;
    } 
    send_to_char(ch, "You have kicked %s out of the group.\r\n", GET_NAME(vict));
    send_to_char(vict, "You have been kicked out of the group.\r\n"); 
    leave_group(vict);
  } else if (is_abbrev(buf, "leave")) {
    if (!GROUP(ch)) {
      send_to_char(ch, "But you aren't apart of a group!\r\n");
      return;
    }
    leave_group(ch);
  } else if (is_abbrev(buf, "option")) {
    skip_spaces(&argument);
    if (!GROUP(ch)) {
      send_to_char(ch, "But you aren't part of a group!\r\n");
      return;
    } else if (GROUP_LEADER(GROUP(ch)) != ch) {
      send_to_char(ch, "Only the group leader can adjust the group flags.\r\n");
      return;
    }

    if (is_abbrev(argument, "open")) {
      TOGGLE_BIT(GROUP_FLAGS(GROUP(ch)), GROUP_OPEN);
      send_to_char(ch, "The group is now %s to new members.\r\n",
		IS_SET(GROUP_FLAGS(GROUP(ch)), GROUP_OPEN) ? "open" : "closed");
    } else if (is_abbrev(argument, "anonymous")) {
      TOGGLE_BIT(GROUP_FLAGS(GROUP(ch)), GROUP_ANON);
      send_to_char(ch, "The group location is now %s to other players.\r\n",
		IS_SET(GROUP_FLAGS(GROUP(ch)), GROUP_ANON) ? "invisible" : "visible");
    } else 
      send_to_char(ch, "The flag options are: Open, Anonymous\r\n");
  } else {
    send_to_char(ch, "You must specify a group option, or type HELP GROUP for more info.\r\n");	
  }

}

ACMD(do_report)
{
  struct group_data *group;

  if ((group = GROUP(ch)) == NULL) {
    send_to_char(ch, "But you are not a member of any group!\r\n");
    return;
  }

  send_to_group(NULL, group, "%s reports: %d/%dH, %d/%dM, %d/%dV\r\n",
	  GET_NAME(ch),	GET_HIT(ch), GET_MAX_HIT(ch),
	  GET_MANA(ch), GET_MAX_MANA(ch),
	  GET_MOVE(ch), GET_MAX_MOVE(ch));
}


ACMD(do_split)
{
  char buf[MAX_INPUT_LENGTH];
  int amount, num = 0, share, rest;
  size_t len;
  struct char_data *k;

  if (IS_NPC(ch))
    return;

  one_argument(argument, buf);

  if (is_number(buf)) {
    amount = atoi(buf);
    if (amount <= 0) {
      send_to_char(ch, "Sorry, you can't do that.\r\n");
      return;
    }
    if (amount > GET_GOLD(ch)) {
      send_to_char(ch, "You don't seem to have that much gold to split.\r\n");
      return;
    }

    if (GROUP(ch))
      while ((k = (struct char_data *) simple_list(GROUP(ch)->members)) != NULL)
        if (IN_ROOM(ch) == IN_ROOM(k) && !IS_NPC(k))
          num++;

    if (num && GROUP(ch)) {
      share = amount / num;
      rest = amount % num;
    } else {
      send_to_char(ch, "With whom do you wish to share your gold?\r\n");
      return;
    }

    decrease_gold(ch, share * (num - 1));

    /* Abusing signed/unsigned to make sizeof work. */
    len = snprintf(buf, sizeof(buf), "%s splits %d coins; you receive %d.\r\n",
		GET_NAME(ch), amount, share);
    if (rest && len < sizeof(buf)) {
      snprintf(buf + len, sizeof(buf) - len,
		"%d coin%s %s not splitable, so %s keeps the money.\r\n", rest,
		(rest == 1) ? "" : "s", (rest == 1) ? "was" : "were", GET_NAME(ch));
    }

    while ((k = (struct char_data *) simple_list(GROUP(ch)->members)) != NULL)
      if (k != ch && IN_ROOM(ch) == IN_ROOM(k) && !IS_NPC(k)) {
	      increase_gold(k, share);
	      send_to_char(k, "%s", buf);
			}
    send_to_char(ch, "You split %d coins among %d members -- %d coins each.\r\n",
	    amount, num, share);

    if (rest) {
      send_to_char(ch, "%d coin%s %s not splitable, so you keep the money.\r\n",
		rest, (rest == 1) ? "" : "s", (rest == 1) ? "was" : "were");
      increase_gold(ch, rest);
    }
  } else {
    send_to_char(ch, "How many coins do you wish to split with your group?\r\n");
    return;
  }
}


ACMD(do_use)
{
  char buf[MAX_INPUT_LENGTH], arg[MAX_INPUT_LENGTH];
  struct obj_data *mag_item;

  half_chop(argument, arg, buf);
  if (!*arg) {
    send_to_char(ch, "What do you want to %s?\r\n", CMD_NAME);
    return;
  }
  mag_item = GET_EQ(ch, WEAR_HOLD_1);

  if (!mag_item || !isname(arg, mag_item->name)) {
    switch (subcmd) {
    case SCMD_RECITE:
    case SCMD_QUAFF:
      if (!(mag_item = get_obj_in_list_vis(ch, arg, NULL, ch->carrying))) {
	send_to_char(ch, "You don't seem to have %s %s.\r\n", AN(arg), arg);
	return;
      }
      break;
    case SCMD_USE:
      send_to_char(ch, "You don't seem to be holding %s %s.\r\n", AN(arg), arg);
      return;
    default:
      log("SYSERR: Unknown subcmd %d passed to do_use.", subcmd);
      /* SYSERR_DESC: This is the same as the unhandled case in do_gen_ps(),
       * but in the function which handles 'quaff', 'recite', and 'use'. */
      return;
    }
  }
  switch (subcmd) {
  case SCMD_QUAFF:
    if (GET_OBJ_TYPE(mag_item) != ITEM_POTION) {
      send_to_char(ch, "You can only quaff potions.\r\n");
      return;
    }
    break;
  case SCMD_RECITE:
    if (GET_OBJ_TYPE(mag_item) != ITEM_SCROLL) {
      send_to_char(ch, "You can only recite scrolls.\r\n");
      return;
    }
    break;
  case SCMD_USE:
    if ((GET_OBJ_TYPE(mag_item) != ITEM_WAND) &&
	(GET_OBJ_TYPE(mag_item) != ITEM_STAFF)) {
      send_to_char(ch, "You can't seem to figure out how to use it.\r\n");
      return;
    }
    break;
  }

  mag_objectmagic(ch, mag_item, buf);
}


ACMD(do_display)
{
  size_t i;

  if (IS_NPC(ch)) {
    send_to_char(ch, "Monsters don't need displays.  Go away.\r\n");
    return;
  }
  skip_spaces(&argument);

  if (!*argument) {
    send_to_char(ch, "Usage: prompt { { H | M | V } | all | auto | none }\r\n");
    return;
  }

  if (!str_cmp(argument, "auto")) {
    TOGGLE_BIT_AR(PRF_FLAGS(ch), PRF_DISPAUTO);
    send_to_char(ch, "Auto prompt %sabled.\r\n", PRF_FLAGGED(ch, PRF_DISPAUTO) ? "en" : "dis");
    return;
  }

  if (!str_cmp(argument, "on") || !str_cmp(argument, "all")) {
    SET_BIT_AR(PRF_FLAGS(ch), PRF_DISPHP);
    SET_BIT_AR(PRF_FLAGS(ch), PRF_DISPMANA);
    SET_BIT_AR(PRF_FLAGS(ch), PRF_DISPMOVE);
  } else if (!str_cmp(argument, "off") || !str_cmp(argument, "none")) {
    REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_DISPHP);
    REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_DISPMANA);
    REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_DISPMOVE);
  } else {
    REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_DISPHP);
    REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_DISPMANA);
    REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_DISPMOVE);

    for (i = 0; i < strlen(argument); i++) {
      switch (LOWER(argument[i])) {
      case 'h':
        SET_BIT_AR(PRF_FLAGS(ch), PRF_DISPHP);
	break;
      case 'm':
        SET_BIT_AR(PRF_FLAGS(ch), PRF_DISPMANA);
	break;
      case 'v':
        SET_BIT_AR(PRF_FLAGS(ch), PRF_DISPMOVE);
	break;
      default:
	send_to_char(ch, "Usage: prompt { { H | M | V } | all | auto | none }\r\n");
	return;
      }
    }
  }

  send_to_char(ch, "%s", CONFIG_OK);
}

#define TOG_OFF 0
#define TOG_ON  1
ACMD(do_gen_tog)
{
  long result;
  int i;
  char arg[MAX_INPUT_LENGTH];

  const char *tog_messages[][2] = {
    {"You are now safe from summoning by other players.\r\n",
    "You may now be summoned by other players.\r\n"},
    {"Nohassle disabled.\r\n",
    "Nohassle enabled.\r\n"},
    {"Brief mode off.\r\n",
    "Brief mode on.\r\n"},
    {"Compact mode off.\r\n",
    "Compact mode on.\r\n"},
    {"You can now hear tells.\r\n",
    "You are now deaf to tells.\r\n"},
    {"You can now hear auctions.\r\n",
    "You are now deaf to auctions.\r\n"},
    {"You can now hear shouts.\r\n",
    "You are now deaf to shouts.\r\n"},
    {"You can now hear gossip.\r\n",
    "You are now deaf to gossip.\r\n"},
    {"You can now hear the congratulation messages.\r\n",
    "You are now deaf to the congratulation messages.\r\n"},
    {"You can now hear the Wiz-channel.\r\n",
    "You are now deaf to the Wiz-channel.\r\n"},
    {"You are no longer part of the Quest.\r\n",
    "Okay, you are part of the Quest!\r\n"},
    {"You will no longer see the room flags.\r\n",
    "You will now see the room flags.\r\n"},
    {"You will now have your communication repeated.\r\n",
    "You will no longer have your communication repeated.\r\n"},
    {"HolyLight mode off.\r\n",
    "HolyLight mode on.\r\n"},
    {"Nameserver_is_slow changed to NO; IP addresses will now be resolved.\r\n",
    "Nameserver_is_slow changed to YES; sitenames will no longer be resolved.\r\n"},
    {"Autoexits disabled.\r\n",
    "Autoexits enabled.\r\n"},
    {"Will no longer track through doors.\r\n",
    "Will now track through doors.\r\n"},
    {"Will no longer clear screen in OLC.\r\n",
    "Will now clear screen in OLC.\r\n"},
    {"Buildwalk Off.\r\n",
    "Buildwalk On.\r\n"},
    {"AFK flag is now off.\r\n",
    "AFK flag is now on.\r\n"},
    {"Autoloot disabled.\r\n",
    "Autoloot enabled.\r\n"},
    {"Autogold disabled.\r\n",
    "Autogold enabled.\r\n"},
    {"Autosplit disabled.\r\n",
    "Autosplit enabled.\r\n"},
    {"Autosacrifice disabled.\r\n",
    "Autosacrifice enabled.\r\n"},
    {"Autoassist disabled.\r\n",
    "Autoassist enabled.\r\n"},
    {"Automap disabled.\r\n",
    "Automap enabled.\r\n"},
    {"Autokey disabled.\r\n",
    "Autokey enabled.\r\n"},
    {"Autodoor disabled.\r\n",
    "Autodoor enabled.\r\n"},
    {"You are now able to see all clantalk.\r\n",
     "Clantalk channels disabled.\r\n"}
  };

  if (IS_NPC(ch))
    return;

  switch (subcmd) {
  case SCMD_NOSUMMON:
    result = PRF_TOG_CHK(ch, PRF_SUMMONABLE);
    break;
  case SCMD_NOHASSLE:
    result = PRF_TOG_CHK(ch, PRF_NOHASSLE);
    break;
  case SCMD_BRIEF:
    result = PRF_TOG_CHK(ch, PRF_BRIEF);
    break;
  case SCMD_COMPACT:
    result = PRF_TOG_CHK(ch, PRF_COMPACT);
    break;
  case SCMD_NOTELL:
    result = PRF_TOG_CHK(ch, PRF_NOTELL);
    break;
  case SCMD_NOAUCTION:
    result = PRF_TOG_CHK(ch, PRF_NOAUCT);
    break;
  case SCMD_NOSHOUT:
    result = PRF_TOG_CHK(ch, PRF_NOSHOUT);
    break;
  case SCMD_NOGOSSIP:
    result = PRF_TOG_CHK(ch, PRF_NOGOSS);
    break;
  case SCMD_NOGRATZ:
    result = PRF_TOG_CHK(ch, PRF_NOGRATZ);
    break;
  case SCMD_NOWIZ:
    result = PRF_TOG_CHK(ch, PRF_NOWIZ);
    break;
  case SCMD_QUEST:
    result = PRF_TOG_CHK(ch, PRF_QUEST);
    break;
  case SCMD_SHOWVNUMS:
    result = PRF_TOG_CHK(ch, PRF_SHOWVNUMS);
    break;
  case SCMD_NOREPEAT:
    result = PRF_TOG_CHK(ch, PRF_NOREPEAT);
    break;
  case SCMD_HOLYLIGHT:
    result = PRF_TOG_CHK(ch, PRF_HOLYLIGHT);
    break;
  case SCMD_AUTOEXIT:
    result = PRF_TOG_CHK(ch, PRF_AUTOEXIT);
    break;
  case SCMD_CLS:
    result = PRF_TOG_CHK(ch, PRF_CLS);
    break;
  case SCMD_BUILDWALK:
    if (GET_LEVEL(ch) < LVL_BUILDER) {
      send_to_char(ch, "Builders only, sorry.\r\n");
      return;
    }
    result = PRF_TOG_CHK(ch, PRF_BUILDWALK);
    if (PRF_FLAGGED(ch, PRF_BUILDWALK)) {
	  one_argument(argument, arg);
	  for (i=0; *arg && *(sector_types[i]) != '\n'; i++)
		if (is_abbrev(arg, sector_types[i]))
		  break;
	  if (*(sector_types[i]) == '\n') 
	    i=0;
	  GET_BUILDWALK_SECTOR(ch) = i;
	  send_to_char(ch, "Default sector type is %s\r\n", sector_types[i]);
	  
      mudlog(CMP, GET_LEVEL(ch), TRUE,
             "OLC: %s turned buildwalk on. Allowed zone %d", GET_NAME(ch), GET_OLC_ZONE(ch));
    } else
      mudlog(CMP, GET_LEVEL(ch), TRUE,
             "OLC: %s turned buildwalk off. Allowed zone %d", GET_NAME(ch), GET_OLC_ZONE(ch));
    break;
  case SCMD_AFK:
    result = PRF_TOG_CHK(ch, PRF_AFK);
    if (PRF_FLAGGED(ch, PRF_AFK))
      act("$n has gone AFK.", TRUE, ch, 0, 0, TO_ROOM);
    else {
      act("$n has come back from AFK.", TRUE, ch, 0, 0, TO_ROOM);
      if (has_mail(GET_IDNUM(ch)))
        send_to_char(ch, "You have mail waiting.\r\n");
    }
    break;
  case SCMD_AUTOLOOT:
    result = PRF_TOG_CHK(ch, PRF_AUTOLOOT);
    break;
  case SCMD_AUTOGOLD:
    result = PRF_TOG_CHK(ch, PRF_AUTOGOLD);
    break;
  case SCMD_AUTOSPLIT:
    result = PRF_TOG_CHK(ch, PRF_AUTOSPLIT);
    break;
  case SCMD_AUTOSAC:
    result = PRF_TOG_CHK(ch, PRF_AUTOSAC);
    break;
  case SCMD_AUTOASSIST:
    result = PRF_TOG_CHK(ch, PRF_AUTOASSIST);
    break;
  case SCMD_AUTOMAP:
    result = PRF_TOG_CHK(ch, PRF_AUTOMAP);
    break;
  case SCMD_AUTOKEY:
    result = PRF_TOG_CHK(ch, PRF_AUTOKEY);
    break;
  case SCMD_AUTODOOR:
    result = PRF_TOG_CHK(ch, PRF_AUTODOOR);
    break;
  case SCMD_NOCLANTALK:
    result = PRF_TOG_CHK(ch, PRF_NOCLANTALK);
    break;
  default:
    log("SYSERR: Unknown subcmd %d in do_gen_toggle.", subcmd);
    return;
  }

  if (result)
    send_to_char(ch, "%s", tog_messages[subcmd][TOG_ON]);
  else
    send_to_char(ch, "%s", tog_messages[subcmd][TOG_OFF]);

  return;
}

/*a general diplomacy skill - popularity increase is determined by SCMD */
struct diplomacy_data diplomacy_types[] = {
  { SCMD_MURMUR,     SKILL_MURMUR,     0.75, 1},  /**< Murmur skill, 0.75% increase, 1 tick wait  */
  { SCMD_PROPAGANDA, SKILL_PROPAGANDA, 2.32, 3},  /**< Propaganda skill, 2.32% increase, 3 tick wait */
  { SCMD_LOBBY,      SKILL_LOBBY,      10.0, 8},  /**< Lobby skill, 10% increase, 8 tick wait     */

  { 0, 0, 0.0, 0 }                                /**< This must be the last line */
};

ACMD(do_diplomacy)
{
  // need to make this do something Zusuk :P
}

void show_happyhour(struct char_data *ch)
{
  char happyexp[80], happygold[80], happyqp[80];
  int secs_left;

  if ((IS_HAPPYHOUR) || (GET_LEVEL(ch) >= LVL_GRGOD))
  {
      if (HAPPY_TIME)
        secs_left = ((HAPPY_TIME - 1) * SECS_PER_MUD_HOUR) + next_tick;
      else
        secs_left = 0;

      sprintf(happyqp,   "%s+%d%%%s to Questpoints per quest\r\n", CCYEL(ch, C_NRM), HAPPY_QP,   CCNRM(ch, C_NRM));
      sprintf(happygold, "%s+%d%%%s to Gold gained per kill\r\n",  CCYEL(ch, C_NRM), HAPPY_GOLD, CCNRM(ch, C_NRM));
      sprintf(happyexp,  "%s+%d%%%s to Experience per kill\r\n",   CCYEL(ch, C_NRM), HAPPY_EXP,  CCNRM(ch, C_NRM));

      send_to_char(ch, "LuminariMUD Happy Hour!\r\n"
                       "------------------\r\n"
                       "%s%s%sTime Remaining: %s%d%s hours %s%d%s mins %s%d%s secs\r\n",
                       (IS_HAPPYEXP || (GET_LEVEL(ch) >= LVL_GOD)) ? happyexp : "",
                       (IS_HAPPYGOLD || (GET_LEVEL(ch) >= LVL_GOD)) ? happygold : "",
                       (IS_HAPPYQP || (GET_LEVEL(ch) >= LVL_GOD)) ? happyqp : "",
                       CCYEL(ch, C_NRM), (secs_left / 3600), CCNRM(ch, C_NRM),
                       CCYEL(ch, C_NRM), (secs_left % 3600) / 60, CCNRM(ch, C_NRM),
                       CCYEL(ch, C_NRM), (secs_left % 60), CCNRM(ch, C_NRM) );
  }
  else
  {
      send_to_char(ch, "Sorry, there is currently no happy hour!\r\n");
  }
}

ACMD(do_happyhour)
{
  char arg[MAX_INPUT_LENGTH], val[MAX_INPUT_LENGTH];
  int num;

  if (GET_LEVEL(ch) < LVL_GOD)
  {
    show_happyhour(ch);
    return;
  }

  /* Only Imms get here, so check args */
  two_arguments(argument, arg, val);

  if (is_abbrev(arg, "experience"))
  {
    num = MIN(MAX((atoi(val)), 0), 1000);
    HAPPY_EXP = num;
    send_to_char(ch, "Happy Hour Exp rate set to +%d%%\r\n", HAPPY_EXP);
  }
  else if ((is_abbrev(arg, "gold")) || (is_abbrev(arg, "coins")))
  {
    num = MIN(MAX((atoi(val)), 0), 1000);
    HAPPY_GOLD = num;
    send_to_char(ch, "Happy Hour Gold rate set to +%d%%\r\n", HAPPY_GOLD);
  }
  else if ((is_abbrev(arg, "time")) || (is_abbrev(arg, "ticks")))
  {
    num = MIN(MAX((atoi(val)), 0), 1000);
    if (HAPPY_TIME && !num)
      game_info("Happyhour has been stopped!");
    else if (!HAPPY_TIME && num)
      game_info("A Happyhour has started!");

    HAPPY_TIME = num;
    send_to_char(ch, "Happy Hour Time set to %d ticks (%d hours %d mins and %d secs)\r\n",
                                HAPPY_TIME,
                                 (HAPPY_TIME*SECS_PER_MUD_HOUR)/3600,
                                ((HAPPY_TIME*SECS_PER_MUD_HOUR)%3600) / 60,
                                 (HAPPY_TIME*SECS_PER_MUD_HOUR)%60 );
  }
  else if ((is_abbrev(arg, "qp")) || (is_abbrev(arg, "questpoints")))
  {
    num = MIN(MAX((atoi(val)), 0), 1000);
    HAPPY_QP = num;
    send_to_char(ch, "Happy Hour Questpoints rate set to +%d%%\r\n", HAPPY_QP);
  }
  else if (is_abbrev(arg, "show"))
  {
    show_happyhour(ch);
  }
  else if (is_abbrev(arg, "default"))
  {
    HAPPY_EXP = 100;
    HAPPY_GOLD = 50;
    HAPPY_QP  = 50;
    HAPPY_TIME = 48;
    game_info("A Happyhour has started!");
  }
  else
  {
    send_to_char(ch, "Usage: %shappyhour              %s- show usage (this info)\r\n"
                     "       %shappyhour show         %s- display current settings (what mortals see)\r\n"
                     "       %shappyhour time <ticks> %s- set happyhour time and start timer\r\n"
                     "       %shappyhour qp <num>     %s- set qp percentage gain\r\n"
                     "       %shappyhour exp <num>    %s- set exp percentage gain\r\n"
                     "       %shappyhour gold <num>   %s- set gold percentage gain\r\n"
                     "       \tyhappyhour default      \tw- sets a default setting for happyhour\r\n\r\n"
                     "Configure the happyhour settings and start a happyhour.\r\n"
                     "Currently 1 hour IRL = %d ticks\r\n"
                     "If no number is specified, 0 (off) is assumed.\r\nThe command \tyhappyhour time\tn will therefore stop the happyhour timer.\r\n",
                     CCYEL(ch, C_NRM), CCNRM(ch, C_NRM),
                     CCYEL(ch, C_NRM), CCNRM(ch, C_NRM),
                     CCYEL(ch, C_NRM), CCNRM(ch, C_NRM),
                     CCYEL(ch, C_NRM), CCNRM(ch, C_NRM),
                     CCYEL(ch, C_NRM), CCNRM(ch, C_NRM),
                     CCYEL(ch, C_NRM), CCNRM(ch, C_NRM),
                     (3600 / SECS_PER_MUD_HOUR) );
  }
}
