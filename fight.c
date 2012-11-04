/**************************************************************************
*  File: fight.c                                           Part of tbaMUD *
*  Usage: Combat system.                                                  *
*                                                                         *
*  All rights reserved.  See license for complete information.            *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
**************************************************************************/

#define __FIGHT_C__

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "handler.h"
#include "interpreter.h"
#include "db.h"
#include "spells.h"
#include "screen.h"
#include "constants.h"
#include "dg_scripts.h"
#include "act.h"
#include "class.h"
#include "fight.h"
#include "shop.h"
#include "quest.h"
#include "mud_event.h"
#include "spec_procs.h"
#include "clan.h"

/* locally defined global variables, used externally */

/* head of l-list of fighting chars */
struct char_data *combat_list = NULL;

/* Weapon attack texts */
struct attack_hit_type attack_hit_text[] =
{
  {"hit", "hits"},    /* 0 */
  {"sting", "stings"},
  {"whip", "whips"},
  {"slash", "slashes"},
  {"bite", "bites"},
  {"bludgeon", "bludgeons"},  /* 5 */
  {"crush", "crushes"},
  {"pound", "pounds"},
  {"claw", "claws"},
  {"maul", "mauls"},
  {"thrash", "thrashes"}, /* 10 */
  {"pierce", "pierces"},
  {"blast", "blasts"},
  {"punch", "punches"},
  {"stab", "stabs"}
};

/* local (file scope only) variables */
static struct char_data *next_combat_list = NULL;

/* local file scope utility functions */
static void perform_group_gain(struct char_data *ch, int base, struct char_data *victim);
static void dam_message(int dam, struct char_data *ch, struct char_data *victim,
	int w_type, int offhand);
static void free_messages_type(struct msg_type *msg);
static void make_corpse(struct char_data *ch);
static void change_alignment(struct char_data *ch, struct char_data *victim);
static void group_gain(struct char_data *ch, struct char_data *victim);
static void solo_gain(struct char_data *ch, struct char_data *victim);
/** @todo refactor this function name */
static char *replace_string(const char *str, const char *weapon_singular, const char *weapon_plural);

#define IS_WEAPON(type) (((type) >= TYPE_HIT) && ((type) < TYPE_SUFFERING))



/*********************************/


void perform_flee(struct char_data *ch)
{
  int i, found = 0, fleeOptions[DIR_COUNT];
  struct char_data *was_fighting; 
    
  /* disqualifications? */
  if (AFF_FLAGGED(ch, AFF_STUN) ||
          AFF_FLAGGED(ch, AFF_PARALYZED) || char_has_mud_event(ch, eSTUNNED)) {
    send_to_char(ch, "You try to flee, but you are unable to move!\r\n");  
    act("$n attemps to flee, but is unable to move!", TRUE, ch, 0, 0, TO_ROOM);
    return;
  }

    //first find which directions are fleeable
  for (i = 0; i < DIR_COUNT; i++) {
    if (CAN_GO(ch, i)) {
      fleeOptions[found] = i;
      found++;
    }
  }
    
    //no actual fleeable directions
  if (!found) {
    send_to_char(ch, "You have no route of escape!\r\n");
    return;
  }
        
  //not fighting?  no problems
  if (!FIGHTING(ch)) {
    send_to_char(ch, "You quickly flee the area...\r\n");
    act("$n quickly flees the area!", TRUE, ch, 0, 0, TO_ROOM);

    //pick a random direction
    do_simple_move(ch, fleeOptions[rand_number(0, found - 1)], 3);

  } else {

    send_to_char(ch, "You attempt to flee:  ");
    act("$n attemps to flee...", TRUE, ch, 0, 0, TO_ROOM);
    
    //ok beat all odds, fleeing
    was_fighting = FIGHTING(ch);

    //pick a random direction
    if (do_simple_move(ch, fleeOptions[rand_number(0, found - 1)], 3)) {
      send_to_char(ch, "You quickly flee from combat...\r\n");
      act("$n quickly flees the battle!", TRUE, ch, 0, 0, TO_ROOM);
      stop_fighting(ch);
      if (was_fighting && ch == FIGHTING(was_fighting))
        stop_fighting(was_fighting);
      WAIT_STATE(ch, PULSE_VIOLENCE);
    } else {  //failure
      send_to_char(ch, "You failed to flee the battle...\r\n");
      act("$n failed to flee the battle!", TRUE, ch, 0, 0, TO_ROOM);
    }
  }

}

void appear(struct char_data *ch)
{

  if (affected_by_spell(ch, SPELL_INVISIBLE))
    affect_from_char(ch, SPELL_INVISIBLE);

  if (AFF_FLAGGED(ch, AFF_SNEAK)) {  
    REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_SNEAK);
    send_to_char(ch, "You stop sneaking...\r\n");
    act("$n stops moving silently...", FALSE, ch, 0, 0, TO_ROOM);
  }
  if (AFF_FLAGGED(ch, AFF_HIDE)) {
    REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_HIDE);
    send_to_char(ch, "You step out of the shadows...\r\n");
    act("$n steps out of the shadows...", FALSE, ch, 0, 0, TO_ROOM);
  }
  if (AFF_FLAGGED(ch, AFF_INVISIBLE)) {
    REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_INVISIBLE);
    send_to_char(ch, "You snap into visibility...\r\n");
    act("$n slowly fades into existence.", FALSE, ch, 0, 0, TO_ROOM);
  }

}


// A attacking B
int compute_size_bonus(int sizeA, int sizeB)
{
  int bonus = 0;

  // attacker
  switch (sizeA) {
    case SIZE_FINE:
      bonus += 8;
      break;
    case SIZE_DIMINUTIVE:
      bonus += 4;
      break;
    case SIZE_TINY:
      bonus += 2;
      break;
    case SIZE_SMALL:
      bonus += 1;
      break;
    case SIZE_LARGE:
      bonus -= 1;
      break;
    case SIZE_HUGE:
      bonus -= 2;
      break;
    case SIZE_GARGANTUAN:
      bonus -= 4;
      break;
    case SIZE_COLOSSAL:
      bonus -= 8;
      break;
    default:
      //medium
      break;
  }
  // defender
  switch (sizeB) {
    case SIZE_FINE:
      bonus -= 8;
      break;
    case SIZE_DIMINUTIVE:
      bonus -= 4;
      break;
    case SIZE_TINY:
      bonus -= 2;
      break;
    case SIZE_SMALL:
      bonus -= 1;
      break;
    case SIZE_LARGE:
      bonus += 1;
      break;
    case SIZE_HUGE:
      bonus += 2;
      break;
    case SIZE_GARGANTUAN:
      bonus += 4;
      break;
    case SIZE_COLOSSAL:
      bonus += 8;
      break;
    default:
      //medium
      break;
  }

  return bonus;
}


int compute_armor_class(struct char_data *attacker, struct char_data *ch)
{
  int armorclass = GET_AC(ch) / (-10);

  //hack to translate old D&D to 3.5 Edition
  armorclass += 20;

  if (AWAKE(ch))
    armorclass += GET_DEX_BONUS(ch);

  if (attacker)
    armorclass += compute_size_bonus(GET_SIZE(attacker), GET_SIZE(ch));

  if (!IS_NPC(ch) && GET_ABILITY(ch, ABILITY_TUMBLE)) //caps at 5
    armorclass += MIN(5, (int)(compute_ability(ch, ABILITY_TUMBLE)/5));

  if (AFF_FLAGGED(ch, AFF_EXPERTISE))
    armorclass += 5;

  if (!IS_NPC(ch) && GET_SKILL(ch, SKILL_DODGE))
    armorclass += 1;
  if (!IS_NPC(ch) && GET_SKILL(ch, SKILL_ARMOR_SKIN))
    armorclass += 2;

  if (CLASS_LEVEL(ch, CLASS_MONK)) {
    armorclass += GET_WIS_BONUS(ch);
    if (CLASS_LEVEL(ch, CLASS_MONK) >= 5)
      armorclass++;
    if (CLASS_LEVEL(ch, CLASS_MONK) >= 10)
      armorclass++;
    if (CLASS_LEVEL(ch, CLASS_MONK) >= 15)
      armorclass++;
    if (CLASS_LEVEL(ch, CLASS_MONK) >= 20)
      armorclass++;
    if (CLASS_LEVEL(ch, CLASS_MONK) >= 25)
      armorclass++;
    if (CLASS_LEVEL(ch, CLASS_MONK) >= 30)
      armorclass++;
  }

  switch (GET_POS(ch)) {  //position penalty
    case POS_SITTING:
    case POS_RESTING:
    case POS_SLEEPING:
    case POS_STUNNED:
    case POS_INCAP:
    case POS_MORTALLYW:
    case POS_DEAD:
      armorclass -= 2;
      break;
    case POS_FIGHTING:
    case POS_STANDING:
    default:  break;
  }
  if (char_has_mud_event(ch, eTAUNTED))
    armorclass -= 6;
  if (char_has_mud_event(ch, eSTUNNED))
    armorclass -= 2;

  return (MIN(MAX_AC, armorclass));
}


static void free_messages_type(struct msg_type *msg)
{
  if (msg->attacker_msg)	free(msg->attacker_msg);
  if (msg->victim_msg)		free(msg->victim_msg);
  if (msg->room_msg)		free(msg->room_msg);
}


void free_messages(void)
{
  int i;

  for (i = 0; i < MAX_MESSAGES; i++)
    while (fight_messages[i].msg) {
      struct message_type *former = fight_messages[i].msg;

      free_messages_type(&former->die_msg);
      free_messages_type(&former->miss_msg);
      free_messages_type(&former->hit_msg);
      free_messages_type(&former->god_msg);

      fight_messages[i].msg = fight_messages[i].msg->next;
      free(former);
    }
}

void load_messages(void)
{
  FILE *fl;
  int i, type;
  struct message_type *messages;
  char chk[128], *buf;

  if (!(fl = fopen(MESS_FILE, "r"))) {
    log("SYSERR: Error reading combat message file %s: %s", MESS_FILE, strerror(errno));
    exit(1);
  }

  for (i = 0; i < MAX_MESSAGES; i++) {
    fight_messages[i].a_type = 0;
    fight_messages[i].number_of_attacks = 0;
    fight_messages[i].msg = NULL;
  }

  while (!feof(fl)) {
    buf = fgets(chk, 128, fl);
    while (!feof(fl) && (*chk == '\n' || *chk == '*'))
      buf = fgets(chk, 128, fl);

    while (*chk == 'M') {
      buf = fgets(chk, 128, fl);
      sscanf(chk, " %d\n", &type);
      for (i = 0; (i < MAX_MESSAGES) && (fight_messages[i].a_type != type) &&
         (fight_messages[i].a_type); i++);
      if (i >= MAX_MESSAGES) {
        log("SYSERR: Too many combat messages.  Increase MAX_MESSAGES and recompile.");
        exit(1);
      }
      CREATE(messages, struct message_type, 1);
      fight_messages[i].number_of_attacks++;
      fight_messages[i].a_type = type;
      messages->next = fight_messages[i].msg;
      fight_messages[i].msg = messages;

      messages->die_msg.attacker_msg = fread_action(fl, i);
      messages->die_msg.victim_msg = fread_action(fl, i);
      messages->die_msg.room_msg = fread_action(fl, i);
      messages->miss_msg.attacker_msg = fread_action(fl, i);
      messages->miss_msg.victim_msg = fread_action(fl, i);
      messages->miss_msg.room_msg = fread_action(fl, i);
      messages->hit_msg.attacker_msg = fread_action(fl, i);
      messages->hit_msg.victim_msg = fread_action(fl, i);
      messages->hit_msg.room_msg = fread_action(fl, i);
      messages->god_msg.attacker_msg = fread_action(fl, i);
      messages->god_msg.victim_msg = fread_action(fl, i);
      messages->god_msg.room_msg = fread_action(fl, i);
      buf  = fgets(chk, 128, fl);
      while (!feof(fl) && (*chk == '\n' || *chk == '*'))
        buf  = fgets(chk, 128, fl);
    }
  }
  fclose(fl);
}


// the whole update_pos system probably needs to be rethought -zusuk
void update_pos_dam(struct char_data *victim)
{
  if (GET_HIT(victim) <= -11)
    GET_POS(victim) = POS_DEAD;
  else if (GET_HIT(victim) <= -6)
    GET_POS(victim) = POS_MORTALLYW;
  else if (GET_HIT(victim) <= -3)
    GET_POS(victim) = POS_INCAP;
  else if (GET_HIT(victim) == 0)
    GET_POS(victim) = POS_STUNNED;

  else {  // hp > 0
    if (GET_POS(victim) < POS_RESTING) {
      if (!AWAKE(victim))
        send_to_char(victim, "\tRYour sleep is disturbed!!\tn  ");
      GET_POS(victim) = POS_SITTING;
      send_to_char(victim,
	"You instinctively shift from dangerous positioning to sitting...\r\n");
    }
  }

}


void update_pos(struct char_data *victim)
{

  if ((GET_HIT(victim) > 0) && (GET_POS(victim) > POS_STUNNED))
    return;

  if (GET_HIT(victim) <= -11)
    GET_POS(victim) = POS_DEAD;
  else if (GET_HIT(victim) <= -6)
    GET_POS(victim) = POS_MORTALLYW;
  else if (GET_HIT(victim) <= -3)
    GET_POS(victim) = POS_INCAP;
  else if (GET_HIT(victim) == 0 )
    GET_POS(victim) = POS_STUNNED;

  // hp > 0 , pos <= stunned
  else {
    GET_POS(victim) = POS_RESTING;
    send_to_char(victim,
	"You find yourself in a resting position...\r\n");    
  }
}


void check_killer(struct char_data *ch, struct char_data *vict)
{
  if (PLR_FLAGGED(vict, PLR_KILLER) || PLR_FLAGGED(vict, PLR_THIEF))
    return;
  if (PLR_FLAGGED(ch, PLR_KILLER) || IS_NPC(ch) || IS_NPC(vict) || ch == vict)
    return;
  if (ROOM_FLAGGED(IN_ROOM(vict), ROOM_PEACEFUL)) {
    send_to_char(ch, "You will not be flagged as a killer for attempting to attack in a peaceful room...\r\n");
    return;
  }
  if (GET_LEVEL(ch) > LVL_IMMORT) {
    send_to_char(ch, "Normally you would've been flagged a PKILLER for this action...\r\n");
    return;
  }
  if (GET_LEVEL(vict) > LVL_IMMORT && !IS_NPC(vict)) {
    send_to_char(ch, "You will not be flagged as a killer for attacking an Immortal...\r\n");
    return;
  }

  SET_BIT_AR(PLR_FLAGS(ch), PLR_KILLER);
  send_to_char(ch, "If you want to be a PLAYER KILLER, so be it...\r\n");
  mudlog(BRF, LVL_IMMORT, TRUE, "PC Killer bit set on %s for initiating attack on %s at %s.",
	    GET_NAME(ch), GET_NAME(vict), world[IN_ROOM(vict)].name);
}


void set_fighting(struct char_data *ch, struct char_data *vict)
{
  if (ch == vict)
    return;

  if (FIGHTING(ch)) {
    core_dump();
    return;
  }

  ch->next_fighting = combat_list;
  combat_list = ch;

  if (AFF_FLAGGED(ch, AFF_SLEEP))
    affect_from_char(ch, SPELL_SLEEP);

  FIGHTING(ch) = vict;
//  GET_POS(ch) = POS_FIGHTING;

  if (!CONFIG_PK_ALLOWED)
    check_killer(ch, vict);
}


/* remove a char from the list of fighting chars */
void stop_fighting(struct char_data *ch)
{
  struct char_data *temp;

  if (ch == next_combat_list)
    next_combat_list = ch->next_fighting;

  REMOVE_FROM_LIST(ch, combat_list, next_fighting);
  ch->next_fighting = NULL;
  FIGHTING(ch) = NULL;
  if (GET_POS(ch) != POS_SITTING)
    GET_POS(ch) = POS_STANDING;
  update_pos(ch);
}

static void make_corpse(struct char_data *ch)
{
  char buf2[MAX_NAME_LENGTH + 64];
  struct obj_data *corpse, *o;
  struct obj_data *money;
  int i, x, y;

  corpse = create_obj();

  corpse->item_number = NOTHING;
  IN_ROOM(corpse) = NOWHERE;
  corpse->name = strdup("corpse");

  snprintf(buf2, sizeof(buf2), "The corpse of %s is lying here.", GET_NAME(ch));
  corpse->description = strdup(buf2);

  snprintf(buf2, sizeof(buf2), "the corpse of %s", GET_NAME(ch));
  corpse->short_description = strdup(buf2);

  GET_OBJ_TYPE(corpse) = ITEM_CONTAINER;
  for(x = y = 0; x < EF_ARRAY_MAX || y < TW_ARRAY_MAX; x++, y++) {
    if (x < EF_ARRAY_MAX)
      GET_OBJ_EXTRA_AR(corpse, x) = 0;
    if (y < TW_ARRAY_MAX)
      corpse->obj_flags.wear_flags[y] = 0;
  }
  SET_BIT_AR(GET_OBJ_WEAR(corpse), ITEM_WEAR_TAKE);
  SET_BIT_AR(GET_OBJ_EXTRA(corpse), ITEM_NODONATE);
  GET_OBJ_VAL(corpse, 0) = 0;	/* You can't store stuff in a corpse */
  GET_OBJ_VAL(corpse, 3) = 1;	/* corpse identifier */
  GET_OBJ_WEIGHT(corpse) = GET_WEIGHT(ch) + IS_CARRYING_W(ch);
  GET_OBJ_RENT(corpse) = 100000;
  if (IS_NPC(ch))
    GET_OBJ_TIMER(corpse) = CONFIG_MAX_NPC_CORPSE_TIME;
  else
    GET_OBJ_TIMER(corpse) = CONFIG_MAX_PC_CORPSE_TIME;

  /* transfer character's inventory to the corpse */
  corpse->contains = ch->carrying;
  for (o = corpse->contains; o != NULL; o = o->next_content)
    o->in_obj = corpse;
  object_list_new_owner(corpse, NULL);

  /* transfer character's equipment to the corpse */
  for (i = 0; i < NUM_WEARS; i++)
    if (GET_EQ(ch, i)) {
      remove_otrigger(GET_EQ(ch, i), ch);
      obj_to_obj(unequip_char(ch, i), corpse);
    }

  /* transfer gold */
  if (GET_GOLD(ch) > 0) {
    /* following 'if' clause added to fix gold duplication loophole. The above
     * line apparently refers to the old "partially log in, kill the game
     * character, then finish login sequence" duping bug. The duplication has
     * been fixed (knock on wood) but the test below shall live on, for a
     * while. -gg 3/3/2002 */
    if (IS_NPC(ch) || ch->desc) {
      money = create_money(GET_GOLD(ch));
      obj_to_obj(money, corpse);
    }
    GET_GOLD(ch) = 0;
  }
  ch->carrying = NULL;
  IS_CARRYING_N(ch) = 0;
  IS_CARRYING_W(ch) = 0;

  obj_to_room(corpse, IN_ROOM(ch));
}

/* When ch kills victim */
static void change_alignment(struct char_data *ch, struct char_data *victim)
{
  /* new alignment change algorithm: if you kill a monster with alignment A,
   * you move 1/16th of the way to having alignment -A.  Simple and fast. */
  GET_ALIGNMENT(ch) += (-GET_ALIGNMENT(victim) - GET_ALIGNMENT(ch)) / 16;
}

void death_cry(struct char_data *ch)
{
  int door;

  act("Your blood freezes as you hear $n's death cry.", FALSE, ch, 0, 0, TO_ROOM);

  for (door = 0; door < DIR_COUNT; door++)
    if (CAN_GO(ch, door))
      send_to_room(world[IN_ROOM(ch)].dir_option[door]->to_room, "Your blood freezes as you hear someone's death cry.\r\n");
}


void death_message(struct char_data *ch)
{
  send_to_char(ch, "\r\n");
  send_to_char(ch, "\r\n");
  send_to_char(ch, "\r\n");
  send_to_char(ch, "\r\n");
  send_to_char(ch, "\r\n");
  send_to_char(ch, "\r\n");
  send_to_char(ch, "\r\n");
  send_to_char(ch, "\r\n");
  send_to_char(ch, "\r\n");
  send_to_char(ch, "\r\n");
  send_to_char(ch, "\r\n");
  send_to_char(ch, "\r\n");
  send_to_char(ch, "\tD'||''|.   '||''''|      |     |''||''| '||'  '||' \r\n");
  send_to_char(ch, " ||   ||   ||  .       |||       ||     ||    ||  \r\n");
  send_to_char(ch, " ||    ||  ||''|      |  ||      ||     ||''''||  \r\n");
  send_to_char(ch, " ||    ||  ||        .''''|.     ||     ||    ||  \r\n");
  send_to_char(ch, ".||...|'  .||.....| .|.  .||.   .||.   .||.  .||. \r\n\tn");
  send_to_char(ch, "\r\n");
  send_to_char(ch, "\r\n");
  send_to_char(ch, "\r\n");
  send_to_char(ch, "\r\n");
  send_to_char(ch, "\r\n");
  send_to_char(ch, "\r\n");
  send_to_char(ch, "You start awake...  you realize someone has resurrected you...\r\n");
  send_to_char(ch, "\r\n");
  send_to_char(ch, "\r\n");
}


// we're not extracting anybody anymore, just penalize them xp
// and move them back to the starting room -zusuk
void raw_kill(struct char_data *ch, struct char_data *killer)
{
  struct char_data *k, *temp;

  if (FIGHTING(ch))
    stop_fighting(ch);
 
  while (ch->affected)
    affect_remove(ch, ch->affected);
  
  GET_POS(ch) = POS_STANDING;  // ordinary commands work in scripts -welcor

  if (killer) {
    if (death_mtrigger(ch, killer))
      death_cry(ch);
  } else
    death_cry(ch);
  
  if (killer)
    autoquest_trigger_check(killer, ch, NULL, AQ_MOB_KILL);
 
  if (GROUP(ch)) {
    send_to_group(ch, GROUP(ch), "%s has died.\r\n", GET_NAME(ch));
    leave_group(ch);
  }

  update_pos(ch);

  //this replaces extraction
  if (ch->followers || ch->master)
    die_follower(ch);

  for (k = combat_list; k; k = temp) {
    temp = k->next_fighting;
    if (FIGHTING(k) == ch)
      stop_fighting(k);
  }  

  /* Whipe character from the memory of hunters and other intelligent NPCs... */
  for (temp = character_list; temp; temp = temp->next) {
    /* PCs can't use MEMORY, and don't use HUNTING() */
    if (!IS_NPC(temp))
      continue;
    /* If "temp" is hunting our extracted char, stop the hunt. */
    if (HUNTING(temp) == ch)
      HUNTING(temp) = NULL;
    /* If "temp" has allocated memory data and our ch is a PC, forget the
     * extracted character (if he/she is remembered) */
    if (!IS_NPC(ch) && MEMORY(temp))
      forget(temp, ch); /* forget() is safe to use without a check. */
  }

  char_from_room(ch);

  death_message(ch);

  char_to_room(ch, r_mortal_start_room);
  act("$n appears in the middle of the room.", TRUE, ch, 0, 0, TO_ROOM);
  look_at_room(ch, 0);
  entry_memory_mtrigger(ch);
  greet_mtrigger(ch, -1);
  greet_memory_mtrigger(ch);
  save_char(ch); 
  Crash_delete_crashfile(ch);
  //end extraction replacement 

  if (killer) {
    autoquest_trigger_check(killer, NULL, NULL, AQ_MOB_SAVE);
    autoquest_trigger_check(killer, NULL, NULL, AQ_ROOM_CLEAR);
  }

  WAIT_STATE(ch, PULSE_VIOLENCE * 4);
  GET_HIT(ch) = 1;
  update_pos(ch);
}


void raw_kill_old(struct char_data * ch, struct char_data * killer)
{
  if (FIGHTING(ch))
    stop_fighting(ch);

  while (ch->affected)
    affect_remove(ch, ch->affected);

  GET_POS(ch) = POS_STANDING;

  if (killer) {
    if (death_mtrigger(ch, killer))
      death_cry(ch);
  } else
    death_cry(ch);

  if (killer)
    autoquest_trigger_check(killer, ch, NULL, AQ_MOB_KILL);

  update_pos(ch);

  make_corpse(ch);
  extract_char(ch);

  if (killer) {
    autoquest_trigger_check(killer, NULL, NULL, AQ_MOB_SAVE);
    autoquest_trigger_check(killer, NULL, NULL, AQ_ROOM_CLEAR);
  }
}


void die(struct char_data * ch, struct char_data * killer)
{
  gain_exp(ch, -(GET_EXP(ch) / 2));
  if (!IS_NPC(ch)) {
    REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_KILLER);
    REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_THIEF);
  }

  if (!IS_NPC(ch))
    raw_kill(ch, killer);
  else
    raw_kill_old(ch, killer);
}

static void perform_group_gain(struct char_data *ch, int base,
			     struct char_data *victim)
{
  int share, hap_share;

  share = MIN(CONFIG_MAX_EXP_GAIN, MAX(1, base));

  if ((IS_HAPPYHOUR) && (IS_HAPPYEXP))
  {
    /* This only reports the correct amount - the calc is done in gain_exp */
    hap_share = share + (int)((float)share * ((float)HAPPY_EXP / (float)(100)));
    share = MIN(CONFIG_MAX_EXP_GAIN, MAX(1, hap_share));
  }
  if (share > 1)
    send_to_char(ch, "You receive your share of experience -- %d points.\r\n", share);
  else
    send_to_char(ch, "You receive your share of experience -- one measly little point!\r\n");

  gain_exp(ch, share);
  change_alignment(ch, victim);
}

static void group_gain(struct char_data *ch, struct char_data *victim)
{
  int tot_members = 0, base, tot_gain;
  struct char_data *k;

  while ((k = (struct char_data *) simple_list(GROUP(ch)->members)) != NULL)
    if (IN_ROOM(ch) == IN_ROOM(k))
      tot_members++;

  /* round up to the nearest tot_members */
  tot_gain = (GET_EXP(victim) / 3) + tot_members - 1;

  /* prevent illegal xp creation when killing players */
  if (!IS_NPC(victim))
    tot_gain = MIN(CONFIG_MAX_EXP_LOSS * 2 / 3, tot_gain);

  if (tot_members >= 1)
    base = MAX(1, tot_gain / tot_members);
  else
    base = 0;

  while ((k = (struct char_data *) simple_list(GROUP(ch)->members)) != NULL)
    if (IN_ROOM(k) == IN_ROOM(ch))
      perform_group_gain(k, base, victim);
}

static void solo_gain(struct char_data *ch, struct char_data *victim)
{
  int exp, happy_exp;

  exp = MIN(CONFIG_MAX_EXP_GAIN, GET_EXP(victim) / 3);

  /* Calculate level-difference bonus */
  if (IS_NPC(ch))
    exp += MAX(0, (exp * MIN(4, (GET_LEVEL(victim) - GET_LEVEL(ch)))) / 8);
  else
    exp += MAX(0, (exp * MIN(8, (GET_LEVEL(victim) - GET_LEVEL(ch)))) / 8);

  exp = MAX(exp, 1);

  if (IS_HAPPYHOUR && IS_HAPPYEXP) {
    happy_exp = exp + (int)((float)exp * ((float)HAPPY_EXP / (float)(100)));
    exp = MAX(happy_exp, 1);
  }

  if (exp > 1)
    send_to_char(ch, "You receive %d experience points.\r\n", exp);
  else
    send_to_char(ch, "You receive one lousy experience point.\r\n");

  gain_exp(ch, exp);
  change_alignment(ch, victim);

  if (GET_LEVEL(ch) < LVL_IMMORT - CONFIG_NO_MORT_TO_IMMORT &&
      GET_EXP(ch) >= level_exp(ch, GET_LEVEL(ch) + 1))
    send_to_char(ch,
	"\tDYou have gained enough xp to advance, type 'gain' to level.\tn\r\n");
}

static char *replace_string(const char *str, const char *weapon_singular, const char *weapon_plural)
{
  static char buf[256];
  char *cp = buf;

  for (; *str; str++) {
    if (*str == '#') {
      switch (*(++str)) {
      case 'W':
	for (; *weapon_plural; *(cp++) = *(weapon_plural++));
	break;
      case 'w':
	for (; *weapon_singular; *(cp++) = *(weapon_singular++));
	break;
      default:
	*(cp++) = '#';
	break;
      }
    } else
      *(cp++) = *str;

    *cp = 0;
  }				/* For */

  return (buf);
}

/* message for doing damage with a weapon */
static void dam_message(int dam, struct char_data *ch, struct char_data *victim,
		      int w_type, int offhand)
{
  char *buf;
  int msgnum, hp = 0, pct;

  hp = GET_HIT(victim);
  if (GET_HIT(victim) < 1)
    hp = 1;
  
  pct = 100 * dam / hp;

  if (dam && pct <= 0)
    pct = 1;

  static struct dam_weapon_type {
    const char *to_room;
    const char *to_char;
    const char *to_victim;
  } dam_weapons[] = {

    /* use #w for singular (i.e. "slash") and #W for plural (i.e. "slashes") */

    {
      "$n tries to #w $N, but misses.",	/* 0: 0     */
      "You try to #w $N, but miss.",
      "$n tries to #w you, but misses."
    },

    {
      "$n barely grazes $N as $e #W $M.",	/* 1: 2  */
      "You barely graze $N as you #w $M.",
      "$n barely grazes you as $e #W you."
    },

    {
      "$n barely #W $N.",		/* 2: 4  */
      "You barely #w $N.",
      "$n barely #W you."
    },

    {
      "$n #W $N.",			/* 3: 6  */
      "You #w $N.",
      "$n #W you."
    },

    {
      "$n #W $N hard.",			/* 4: 10  */
      "You #w $N hard.",
      "$n #W you hard."
    },

    {
      "$n #W $N very hard.",		/* 5: 14  */
      "You #w $N very hard.",
      "$n #W you very hard."
    },

    {
      "$n #W $N extremely hard.",	/* 6: 19  */
      "You #w $N extremely hard.",
      "$n #W you extremely hard."
    },
    {
      "$n severely wounds $N with $s #w.",	/* 7: 23 */
      "You severely wound $N with your #w.",
      "$n severely wounds you with $s #w."
    },

    {
      "$n nearly kills $N with $s deadly #w!!",	/* 8: > 23   */
      "You nearly kill $N with your deadly #w!!",
      "$n nearly kills you with $s deadly #w!!"
    }
  };

  w_type -= TYPE_HIT;		/* Change to base of table with text */

  if (pct == 0)		msgnum = 0;
  else if (pct <= 2)    msgnum = 1;
  else if (pct <= 4)    msgnum = 2;
  else if (pct <= 6)    msgnum = 3;
  else if (pct <= 8)    msgnum = 4;
  else if (pct <= 11)   msgnum = 5;
  else if (pct <= 14)   msgnum = 6;
  else if (pct <= 18)   msgnum = 7;
  else			msgnum = 8;

  /* damage message to onlookers */
  // note, we may have to add more info if we have some way to attack
  // someone that isn't in your room - zusuk
  buf = replace_string(dam_weapons[msgnum].to_room,
	  attack_hit_text[w_type].singular, attack_hit_text[w_type].plural), dam;
  act(buf, FALSE, ch, NULL, victim, TO_NOTVICT);

  /* damage message to damager */
  buf = replace_string(dam_weapons[msgnum].to_char,
	  attack_hit_text[w_type].singular, attack_hit_text[w_type].plural);
  act(buf, FALSE, ch, NULL, victim, TO_CHAR);
  send_to_char(ch, CCNRM(ch, C_CMP));

  /* damage message to damagee */
  buf = replace_string(dam_weapons[msgnum].to_victim,
	  attack_hit_text[w_type].singular, attack_hit_text[w_type].plural);
  act(buf, FALSE, ch, NULL, victim, TO_VICT | TO_SLEEP);
  send_to_char(victim, CCNRM(victim, C_CMP));
}

/*  message for doing damage with a spell or skill. Also used for weapon
 *  damage on miss and death blows. */
int skill_message(int dam, struct char_data *ch, struct char_data *vict,
		      int attacktype, int dualing)
{
  int i, j, nr;
  struct message_type *msg;

  struct obj_data *weap = GET_EQ(ch, WEAR_WIELD_1);

  if (GET_EQ(ch, WEAR_WIELD_2H))
    weap = GET_EQ(ch, WEAR_WIELD_2H);
  else if (dualing)
    weap = GET_EQ(ch, WEAR_WIELD_2);

  for (i = 0; i < MAX_MESSAGES; i++) {
    if (fight_messages[i].a_type == attacktype) {
      nr = dice(1, fight_messages[i].number_of_attacks);
      for (j = 1, msg = fight_messages[i].msg; (j < nr) && msg; j++)
	msg = msg->next;

//      if (!IS_NPC(vict) && (GET_LEVEL(vict) >= LVL_IMPL)) {
//	act(msg->god_msg.attacker_msg, FALSE, ch, weap, vict, TO_CHAR);
//	act(msg->god_msg.victim_msg, FALSE, ch, weap, vict, TO_VICT);
//	act(msg->god_msg.room_msg, FALSE, ch, weap, vict, TO_NOTVICT);} else
      if (dam != 0) {
        /*
         * Don't send redundant color codes for TYPE_SUFFERING & other types
         * of damage without attacker_msg.
         */
	if (GET_POS(vict) == POS_DEAD) {
          if (msg->die_msg.attacker_msg) {
            send_to_char(ch, CCYEL(ch, C_CMP));
            act(msg->die_msg.attacker_msg, FALSE, ch, weap, vict, TO_CHAR);
            send_to_char(ch, CCNRM(ch, C_CMP));
          }

	  send_to_char(vict, CCRED(vict, C_CMP));
	  act(msg->die_msg.victim_msg, FALSE, ch, weap, vict, TO_VICT | TO_SLEEP);
	  send_to_char(vict, CCNRM(vict, C_CMP));

	  act(msg->die_msg.room_msg, FALSE, ch, weap, vict, TO_NOTVICT);
	} else {
          if (msg->hit_msg.attacker_msg) {
	    send_to_char(ch, CCYEL(ch, C_CMP));
	    act(msg->hit_msg.attacker_msg, FALSE, ch, weap, vict, TO_CHAR);
	    send_to_char(ch, CCNRM(ch, C_CMP));
          }

	  send_to_char(vict, CCRED(vict, C_CMP));
	  act(msg->hit_msg.victim_msg, FALSE, ch, weap, vict, TO_VICT | TO_SLEEP);
	  send_to_char(vict, CCNRM(vict, C_CMP));

	  act(msg->hit_msg.room_msg, FALSE, ch, weap, vict, TO_NOTVICT);
	}
      } else if (ch != vict) {	/* Dam == 0 */
        if (msg->miss_msg.attacker_msg) {
	  send_to_char(ch, CCYEL(ch, C_CMP));
	  act(msg->miss_msg.attacker_msg, FALSE, ch, weap, vict, TO_CHAR);
	  send_to_char(ch, CCNRM(ch, C_CMP));
        }

	send_to_char(vict, CCRED(vict, C_CMP));
	act(msg->miss_msg.victim_msg, FALSE, ch, weap, vict, TO_VICT | TO_SLEEP);
	send_to_char(vict, CCNRM(vict, C_CMP));

	act(msg->miss_msg.room_msg, FALSE, ch, weap, vict, TO_NOTVICT);
      }
      return (1);
    }
  }
  return (0);
}


// this is just like damage reduction, except applies to certain type
int compute_energy_absorb(struct char_data *ch, int dam_type)
{
  int dam_reduction = 0;

  switch (dam_type) {
    case DAM_FIRE:
      if (affected_by_spell(ch, SPELL_RESIST_ENERGY))
        dam_reduction += 3;
      break;
    case DAM_COLD:
      if (affected_by_spell(ch, SPELL_RESIST_ENERGY))
        dam_reduction += 3;
      break;
    case DAM_AIR:
      if (affected_by_spell(ch, SPELL_RESIST_ENERGY))
        dam_reduction += 3;
      break;
    case DAM_EARTH:
      if (affected_by_spell(ch, SPELL_RESIST_ENERGY))
        dam_reduction += 3;
      break;
    case DAM_ACID:
      if (affected_by_spell(ch, SPELL_RESIST_ENERGY))
        dam_reduction += 3;
      break;
    case DAM_HOLY:
      break;
    case DAM_ELECTRIC:
      if (affected_by_spell(ch, SPELL_RESIST_ENERGY))
        dam_reduction += 3;
      break;
    case DAM_UNHOLY:
      break;
    case DAM_SLICE:
      break;
    case DAM_PUNCTURE:
      break;
    case DAM_FORCE:
      break;
    case DAM_SOUND:
      break;
    case DAM_POISON:
      break;
    case DAM_DISEASE:
      break;
    case DAM_NEGATIVE:
      break;
    case DAM_ILLUSION:
      break;
    case DAM_MENTAL:
      break;
    case DAM_LIGHT:
      break;
    case DAM_ENERGY:
      if (affected_by_spell(ch, SPELL_RESIST_ENERGY))
        dam_reduction += 3;
      break;
    default: break;
  }

  return (MIN(MAX_ENERGY_ABSORB, dam_reduction));
}


// can return negative values, which indicates vulnerability
// dam_ defines are in spells.h
int compute_damtype_reduction(struct char_data *ch, int dam_type)
{
  int damtype_reduction = 0;

  switch (dam_type) {
    case DAM_FIRE:
      if (GET_RACE(ch) == RACE_TROLL)
        damtype_reduction += -50;
      if (affected_by_spell(ch, SPELL_ENDURE_ELEMENTS))
        damtype_reduction += 10;
      break;
    case DAM_COLD:
      if (affected_by_spell(ch, SPELL_ENDURE_ELEMENTS))
        damtype_reduction += 10;
      break;
    case DAM_AIR:
      if (affected_by_spell(ch, SPELL_ENDURE_ELEMENTS))
        damtype_reduction += 10;
      break;
    case DAM_EARTH:
      if (affected_by_spell(ch, SPELL_ENDURE_ELEMENTS))
        damtype_reduction += 10;
      break;
    case DAM_ACID:
      if (affected_by_spell(ch, SPELL_ENDURE_ELEMENTS))
        damtype_reduction += 10;
      break;
    case DAM_HOLY:
      break;
    case DAM_ELECTRIC:
      if (affected_by_spell(ch, SPELL_ENDURE_ELEMENTS))
        damtype_reduction += 10;
      break;
    case DAM_UNHOLY:
      break;
    case DAM_SLICE:
      break;
    case DAM_PUNCTURE:
      break;
    case DAM_FORCE:
      break;
    case DAM_SOUND:
      break;
    case DAM_POISON:
      break;
    case DAM_DISEASE:
      break;
    case DAM_NEGATIVE:
      break;
    case DAM_ILLUSION:
      break;
    case DAM_MENTAL:
      break;
    case DAM_LIGHT:
      break;
    case DAM_ENERGY:
      if (affected_by_spell(ch, SPELL_ENDURE_ELEMENTS))
        damtype_reduction += 10;
      break;
    default: break;
  }

  return damtype_reduction;  //no cap as of yet
}


int compute_damage_reduction(struct char_data *ch)
{
  int damage_reduction = 0;

  if (!IS_NPC(ch) && GET_SKILL(ch, SKILL_DAMAGE_REDUC_1))
    damage_reduction += 3;
  if (!IS_NPC(ch) && GET_SKILL(ch, SKILL_DAMAGE_REDUC_2))
    damage_reduction += 3;
  if (!IS_NPC(ch) && GET_SKILL(ch, SKILL_DAMAGE_REDUC_3))
    damage_reduction += 3;

  //damage reduction cap is 20
  return (MIN(MAX_DAM_REDUC, damage_reduction));
}


int compute_concealment(struct char_data *ch)
{
  int concealment = 0;

  if (!IS_NPC(ch) && GET_SKILL(ch, SKILL_SELF_CONCEAL_1))
    concealment += 10;
  if (!IS_NPC(ch) && GET_SKILL(ch, SKILL_SELF_CONCEAL_2))
    concealment += 10;
  if (!IS_NPC(ch) && GET_SKILL(ch, SKILL_SELF_CONCEAL_3))
    concealment += 10;
  if (AFF_FLAGGED(ch, AFF_BLUR))
    concealment += 20;

    // concealment cap is 50%
  return (MIN(MAX_CONCEAL, concealment));

}


// returns modified damage, process elements/resistance/avoidance
// -1 means we're gonna go ahead and exit damage()
int damage_handling(struct char_data *ch, struct char_data *victim,
	int dam, int attacktype, int dam_type)
{

  if (dam > 0 && attacktype != TYPE_SUFFERING &&
	attacktype != SKILL_BASH && attacktype != SKILL_TRIP) {
 
    int concealment = compute_concealment(victim);
        if (dice(1, 100) <= compute_concealment(victim)) {
      send_to_char(victim, "\tW<conceal:%d>\tn", concealment);
      send_to_char(ch, "\tR<oconceal:%d>\tn", concealment);
      return 0;
    }

    /* mirror image gives (1 / (# of image + 1)) chance of hitting */
    if (affected_by_spell(victim, SPELL_MIRROR_IMAGE) && dam > 0) {
      if (GET_IMAGES(victim) > 0) {
        if (rand_number(0, GET_IMAGES(victim))) {
          send_to_char(victim, "\tWOne of your images is destroyed!\tn\r\n");
          send_to_char(ch, "\tRYou have struck an illusionary image of %s!\tn\r\n",
		GET_NAME(victim));
          act("$n struck an illusionary image of $N!", FALSE, ch, 0, victim,
                TO_NOTVICT);
          GET_IMAGES(victim)--;
          if (GET_IMAGES(victim) <= 0) {
            send_to_char(victim, "\t2All of your illusionary images are gone!\tn\r\n");
            affect_from_char(victim, SPELL_MIRROR_IMAGE);
          }
          return -1;
        } 
      } else {
        //dummy check
        send_to_char(victim, "\t2All of your illusionary images are gone!\tn\r\n");
        affect_from_char(victim, SPELL_MIRROR_IMAGE);
      }
    }

    int damage_reduction = compute_energy_absorb(ch, dam_type);
    dam -= compute_energy_absorb(ch, dam_type);
    if (dam <= 0) {
      send_to_char(victim, "\tWYou absorbe all the damage!\tn\r\n");
      send_to_char(ch, "\tRYou fail to cause %s any harm!\tn\r\n",
	GET_NAME(victim));
      act("$n fails to do any harm to $N!", FALSE, ch, 0, victim,
	TO_NOTVICT);
      return -1;
    } else if (damage_reduction) {
      send_to_char(victim, "\tW<EA:%d>\tn", damage_reduction);
      send_to_char(ch, "\tR<oEA:%d>\tn", damage_reduction);
    }

    float damtype_reduction = (float)compute_damtype_reduction(victim, dam_type);
    damtype_reduction = (((float)(damtype_reduction/100)) * dam);
    dam -= damtype_reduction;
    if (dam <= 0) {
      send_to_char(victim, "\tWYou absorbe all the damage!\tn\r\n");
      send_to_char(ch, "\tRYou fail to cause %s any harm!\tn\r\n",
	GET_NAME(victim));
      act("$n fails to do any harm to $N!", FALSE, ch, 0, victim,
	TO_NOTVICT);
      return -1;
    } else if (damtype_reduction < 0) { // no reduction, vulnerability
      send_to_char(victim, "\tR<TR:%d>\tn", (int)damtype_reduction);
      send_to_char(ch, "\tW<oTR:%d>\tn", (int)damtype_reduction);
    } else if (damtype_reduction > 0) {
      send_to_char(victim, "\tW<TR:%d>\tn", (int)damtype_reduction);
      send_to_char(ch, "\tR<oTR:%d>\tn", (int)damtype_reduction);
    }

    damage_reduction = compute_damage_reduction(victim);
    dam -= MIN(dam, damage_reduction);
    if (!dam) {
      send_to_char(victim, "\tWYou absorbe all the damage!\tn\r\n");
      send_to_char(ch, "\tRYou fail to cause %s any harm!\tn\r\n",
	GET_NAME(victim));
      act("$n fails to do any harm to $N!", FALSE, ch, 0, victim,
	TO_NOTVICT);
      return -1;
    } else if (damage_reduction) {
      send_to_char(victim, "\tW<DR:%d>\tn", damage_reduction);
      send_to_char(ch, "\tR<oDR:%d>\tn", damage_reduction);
    }
      
    /* Cut damage in half if victim has sanct, to a minimum 1 */
    if (AFF_FLAGGED(victim, AFF_SANCTUARY) && dam >= 2) {
      dam /= 2;
      send_to_char(victim, "\tW<sanc:%d>\tn", dam);
      send_to_char(ch, "\tR<oSanc:%d>\tn", dam);
    }
  }
  return dam;
}


int dam_killed_vict(struct char_data *ch, struct char_data *victim,
	int dam, int attacktype, int dam_type)
{
  char local_buf[256];
  long local_gold = 0, happy_gold = 0;
  struct char_data *tmp_char;
  struct obj_data *corpse_obj;

  if (ch != victim && (IS_NPC(victim) || victim->desc)) {  //xp gain
    if (GROUP(ch))
      group_gain(ch, victim);
    else
      solo_gain(ch, victim);
  }

  resetCastingData(victim);  //stop casting

  if (!IS_NPC(victim)) {  //forget victim, log
    mudlog(BRF, LVL_IMMORT, TRUE, "%s killed by %s at %s", GET_NAME(victim), GET_NAME(ch), world[IN_ROOM(victim)].name);
    if (MOB_FLAGGED(ch, MOB_MEMORY))
      forget(ch, victim);
  }

  if (IS_NPC(victim)) {  // determine gold before corpse created
    if ((IS_HAPPYHOUR) && (IS_HAPPYGOLD)) {
      happy_gold = (long)(GET_GOLD(victim) * (((float)(HAPPY_GOLD))/(float)100));
      happy_gold = MAX(0, happy_gold);
      increase_gold(victim, happy_gold);
    }
    local_gold = GET_GOLD(victim);
    sprintf(local_buf,"%ld", (long)local_gold);
  }

  die(victim, ch);

  //handle dead mob and PRF_
  if (GROUP(ch) && (local_gold > 0) && PRF_FLAGGED(ch, PRF_AUTOSPLIT) ) {
    generic_find("corpse", FIND_OBJ_ROOM, ch, &tmp_char, &corpse_obj);
    if (corpse_obj) {
      do_get(ch, "all.coin corpse", 0, 0);
      do_split(ch, local_buf, 0, 0);
    }
  } else if (!IS_NPC(ch) && (ch != victim) && PRF_FLAGGED(ch, PRF_AUTOGOLD))
    do_get(ch, "all.coin corpse", 0, 0);
  if (!IS_NPC(ch) && (ch != victim) && PRF_FLAGGED(ch, PRF_AUTOLOOT))
    do_get(ch, "all corpse", 0, 0);
  if (IS_NPC(victim) && !IS_NPC(ch) && PRF_FLAGGED(ch, PRF_AUTOSAC))
    do_sac(ch,"corpse",0,0);

  return (-1);
}


// death < 0, no dam = 0, damage done > 0
int damage(struct char_data *ch, struct char_data *victim,
	int dam, int attacktype, int dam_type, int offhand)
{

  if (GET_POS(victim) <= POS_DEAD) {  //delayed extraction
    if (PLR_FLAGGED(victim, PLR_NOTDEADYET) || MOB_FLAGGED(victim, MOB_NOTDEADYET))
      return (-1);
    log("SYSERR: Attempt to damage corpse '%s' in room #%d by '%s'.",
		GET_NAME(victim), GET_ROOM_VNUM(IN_ROOM(victim)), GET_NAME(ch));
    die(victim, ch);
    return (-1);
  }
  if (ch->nr != real_mobile(DG_CASTER_PROXY) &&
      ch != victim && ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL)) {
    send_to_char(ch, "This room just has such a peaceful, easy feeling...\r\n");
    return (0);
  }
  if (!ok_damage_shopkeeper(ch, victim) || MOB_FLAGGED(victim, MOB_NOKILL)) {
    send_to_char(ch, "This mob is protected.\r\n");
    return (0);
  }
  if (!IS_NPC(victim) && ((GET_LEVEL(victim) >= LVL_IMMORT) && PRF_FLAGGED(victim, PRF_NOHASSLE)))
    dam = 0;  // immort protection
  if (victim != ch) {
    if (GET_POS(ch) > POS_STUNNED && (FIGHTING(ch) == NULL))  // ch -> vict
      set_fighting(ch, victim);
    if (GET_POS(victim) > POS_STUNNED && (FIGHTING(victim) == NULL)) {  // vict -> ch
      set_fighting(victim, ch);
      if (MOB_FLAGGED(victim, MOB_MEMORY) && !IS_NPC(ch))
	remember(victim, ch);
    }
  }
  if (victim->master == ch)  // pet leaves you
    stop_follower(victim);
  if (AFF_FLAGGED(ch, AFF_INVISIBLE) || AFF_FLAGGED(ch, AFF_HIDE)) // lose hide/invis
    appear(ch);
  if (!CONFIG_PK_ALLOWED) {  // PK check
    check_killer(ch, victim);
    if (PLR_FLAGGED(ch, PLR_KILLER) && (ch != victim))
      dam = 0;
  }

  dam = damage_handling(ch, victim, dam, attacktype, dam_type); //modify damage
  if (dam == -1)  // make sure message handling has been done!
    return 0;

  dam = MAX(MIN(dam, 999), 0);  //damage cap
  GET_HIT(victim) -= dam;

  if (ch != victim)  //xp gain
    gain_exp(ch, GET_LEVEL(victim) * dam);

  if (!dam)
    update_pos(victim);
  else
    update_pos_dam(victim);

  if (dam) {  //display damage done
    send_to_char(ch, "\tW[%d]\tn", dam);
    send_to_char(victim, "\tR[%d]\tn", dam);  
  }

  if (attacktype != -1) {	//added for mount
    if (!IS_WEAPON(attacktype))  //non weapons use skill_message
      skill_message(dam, ch, victim, attacktype, offhand);
    else {
      if (GET_POS(victim) == POS_DEAD || dam == 0) {  //miss and death = skill_message
        if (!skill_message(dam, ch, victim, attacktype, offhand))
          dam_message(dam, ch, victim, attacktype, offhand);  //default if no skill_message
      } else {
        dam_message(dam, ch, victim, attacktype, offhand); //default landed-hit
      }
    }
  }

  switch (GET_POS(victim)) {  //act() used in case someone is dead
  case POS_MORTALLYW:
    act("$n is mortally wounded, and will die soon, if not aided.",
	TRUE, victim, 0, 0, TO_ROOM);
    send_to_char(victim, "You are mortally wounded, and will die soon, if not aided.\r\n");
    break;
  case POS_INCAP:
    act("$n is incapacitated and will slowly die, if not aided.", TRUE, victim, 0, 0, TO_ROOM);
    send_to_char(victim, "You are incapacitated and will slowly die, if not aided.\r\n");
    break;
  case POS_STUNNED:
    act("$n is stunned, but will probably regain consciousness again.", TRUE, victim, 0, 0, TO_ROOM);
    send_to_char(victim, "You're stunned, but will probably regain consciousness again.\r\n");
    break;
  case POS_DEAD:
    act("$n is dead!  R.I.P.", FALSE, victim, 0, 0, TO_ROOM);
    send_to_char(victim, "You are dead!  Sorry...\r\n");
    break;
  default:
    if (dam > (GET_MAX_HIT(victim) / 4))
      send_to_char(victim, "That really did HURT!\r\n");
    if (GET_HIT(victim) < (GET_MAX_HIT(victim) / 4)) {
      send_to_char(victim, "%sYou wish that your wounds would stop BLEEDING so much!%s\r\n",
		CCRED(victim, C_SPR), CCNRM(victim, C_SPR));

      if (ch != victim && MOB_FLAGGED(victim, MOB_WIMPY))  //wimpy mobs
        if (GET_HIT(victim) > 0)
          if (!IS_CASTING(victim) && GET_POS(victim) >= POS_FIGHTING)
            perform_flee(victim);
    }
    if (!IS_NPC(victim) && GET_WIMP_LEV(victim) && (victim != ch) &&  //pc wimpy
	GET_HIT(victim) < GET_WIMP_LEV(victim) && GET_HIT(victim) > 0 &&
        IN_ROOM(ch) == IN_ROOM(victim) && !IS_CASTING(victim) &&
	GET_POS(victim) >= POS_FIGHTING) {
      send_to_char(victim, "You wimp out, and attempt to flee!\r\n");
      perform_flee(victim);
    }
    break;
  }  //end SWITCH

  if (!IS_NPC(victim) && !(victim->desc) && GET_POS(victim) > POS_STUNNED) { //linkless
    perform_flee(victim);
    if (!FIGHTING(victim)) {
      act("$n is rescued by divine forces.", FALSE, victim, 0, 0, TO_ROOM);
      GET_WAS_IN(victim) = IN_ROOM(victim);
      char_from_room(victim);
      char_to_room(victim, 0);
    }
  }

  if (GET_POS(victim) <= POS_STUNNED && FIGHTING(victim) != NULL) //too hurt to continue
    stop_fighting(victim);

  if (GET_POS(victim) == POS_DEAD)  // victim died
    return (dam_killed_vict(ch, victim, dam, attacktype, dam_type));

  return (dam);
}


// old skool thaco replaced by bab (base attack bonus)
int compute_bab(struct char_data *ch, struct char_data *victim, int type)
{
  int calc_bab = BAB(ch);  //base attack bonus

  // strength (or dex) bonus
  if (!IS_NPC(ch) && GET_SKILL(ch, SKILL_FINESSE))
    calc_bab += GET_DEX_BONUS(ch);
  else
    calc_bab += GET_STR_BONUS(ch);

  // position penalty
  switch (GET_POS(ch)) {
    case POS_SITTING:
    case POS_RESTING:
    case POS_SLEEPING:
    case POS_STUNNED:
    case POS_INCAP:
    case POS_MORTALLYW:
    case POS_DEAD:
      calc_bab -= 2;
      break;
    case POS_FIGHTING:
    case POS_STANDING:
    default:  break;
  }

  // hitroll bonus
  calc_bab += GET_HITROLL(ch);

  // all other bonuses (penalties)
  if (!IS_NPC(ch) && GET_SKILL(ch, SKILL_PROWESS))
    calc_bab++;
  if (!IS_NPC(ch) && GET_SKILL(ch, SKILL_EPIC_PROWESS))
    calc_bab += 2;
  if (AFF_FLAGGED(ch, AFF_POWER_ATTACK))
    calc_bab -= 5;
  if (AFF_FLAGGED(ch, AFF_EXPERTISE))
    calc_bab -= 5;

  return (MIN(MAX_BAB, calc_bab));
}


// ch: focus person, vict: whoever hit()'s vict is,
// type:  weapon type, mod: bonus/penalty to damage
// mode:  whether should display or not; 2=mainhand 3=offhand
int compute_damage_bonus(struct char_data *ch, struct char_data *vict,
	int type, int mod, int mode)
{
  int dambonus = mod;

  //strength
  if (GET_EQ(ch, WEAR_WIELD_2H))
    dambonus += GET_STR_BONUS(ch) * 3 / 2;
  else
    dambonus += GET_STR_BONUS(ch);
  
  //damroll (should be mostly just gear)
  dambonus += GET_DAMROLL(ch);

  // other bonuses
  if (AFF_FLAGGED(ch, AFF_POWER_ATTACK))
    dambonus += 5;

  if (mode == 2 || mode ==3) {
    send_to_char(ch, "Dam Bonus:  %d, ", dambonus);
  }
  return dambonus;
}


int compute_dam_dice(struct char_data *ch, struct char_data *victim, 
	struct obj_data *wielded, int mode)
{
  int diceOne = 0, diceTwo = 0;
  int monkLevel = CLASS_LEVEL(ch, CLASS_MONK);

  //just information mode
  if (mode == 2) {
    if (!GET_EQ(ch, WEAR_WIELD_1) && !GET_EQ(ch, WEAR_WIELD_2H)) {
      send_to_char(ch, "Bare-hands\r\n");
    } else {
      if (GET_EQ(ch, WEAR_WIELD_2H))
        wielded = GET_EQ(ch, WEAR_WIELD_2H);
      else
        wielded = GET_EQ(ch, WEAR_WIELD_1);
      show_obj_to_char(wielded, ch, SHOW_OBJ_SHORT);
    }
  } else if (mode == 3) {
    wielded = GET_EQ(ch, WEAR_WIELD_2);
    show_obj_to_char(GET_EQ(ch, WEAR_WIELD_2), ch, SHOW_OBJ_SHORT);
  }

  if (wielded && GET_OBJ_TYPE(wielded) == ITEM_WEAPON) {  //weapon
    diceOne = GET_OBJ_VAL(wielded, 1);
    diceTwo = GET_OBJ_VAL(wielded, 2);
  } else {  //barehand
    if (IS_NPC(ch)) {
      diceOne = ch->mob_specials.damnodice;
      diceTwo = ch->mob_specials.damsizedice;
    } else {
      if (monkLevel && !GET_EQ(ch, WEAR_HOLD_1) &&
		!GET_EQ(ch, WEAR_WIELD_1) && !GET_EQ(ch, WEAR_HOLD_2) &&
		!GET_EQ(ch, WEAR_WIELD_2) && !GET_EQ(ch, WEAR_SHIELD) &&
		!GET_EQ(ch, WEAR_WIELD_2H)
	) {
        if (monkLevel < 4) {	diceOne = 1;	diceTwo = 6;	}
        else if (monkLevel < 8) {	diceOne = 1;	diceTwo = 8;	}
        else if (monkLevel < 12) {	diceOne = 1;	diceTwo = 10;	}
        else if (monkLevel < 16) {	diceOne = 2;	diceTwo = 6;	}
        else if (monkLevel < 20) {	diceOne = 2;	diceTwo = 8;	}
        else if (monkLevel < 25) {	diceOne = 4;	diceTwo = 5;	}
        else {				diceOne = 6;	diceTwo = 4;	}
      } else { // non-monk bare-hand damage
        diceOne = 1;
        diceTwo = 2;
      }
    }
  }
  if (mode == 2 || mode == 3) {
    send_to_char(ch, "Damage Dice:  %dD%d, ", diceOne, diceTwo);
  }

  return dice(diceOne,diceTwo);
}


int isCriticalHit(struct char_data *ch, int diceroll)
{
  if ((!IS_NPC(ch) && GET_SKILL(ch, SKILL_EPIC_CRIT) && diceroll >= 18) ||
	(!IS_NPC(ch) && GET_SKILL(ch, SKILL_IMPROVED_CRITICAL) && diceroll >= 19) ||
	diceroll == 20)
    return 1;
  return 0;
}


// since dam is potentially adjusted percentile, we needed to bring it
// (same reason we bring diceroll)
// mode is for info purposes, 2 = mainhand, 3 = offhand
int hit_dam_bonus(struct char_data *ch, struct char_data *victim,
	int dam, int diceroll, int mode)
{

  if (mode == 0) {
    //dirty fighting bonus damage
    if (!IS_NPC(ch) && GET_SKILL(ch, SKILL_DIRTY_FIGHTING) >= dice(1, 106)) {
      send_to_char(ch, "\tW[DF]\tn");
      send_to_char(victim, "\tR[oDF]\tn");      
      dam += dice(2, 3);
    }      

    //critical hit!  improved crit increases crit chance by 5%, epic 10%
    if (isCriticalHit(ch, diceroll)) {

      //overwhelming crit addes 3d2 damage before doubling for normal crit
      if (!IS_NPC(ch) && GET_SKILL(ch, SKILL_OVERWHELMING_CRIT) && !IS_NPC(ch)) {
        send_to_char(ch, "\tW[OW]\tn");
        send_to_char(victim, "\tR[oOW]\tn");      
        dam += dice(3, 2); 
      }
      send_to_char(ch, "\tW[crit]\tn");
      send_to_char(victim, "\tR[oCrit]\tn");      
      dam *= 2;
    }
  } else if (mode == 2 || mode == 3) {
    send_to_char(ch, "Other Bonuses:  ");
    if (!IS_NPC(ch) && GET_SKILL(ch, SKILL_DIRTY_FIGHTING))
      send_to_char(ch, "[DF] ");
    if (!IS_NPC(ch) && GET_SKILL(ch, SKILL_EPIC_CRIT))
      send_to_char(ch, "[EC] ");
    if (!IS_NPC(ch) && GET_SKILL(ch, SKILL_IMPROVED_CRITICAL))
      send_to_char(ch, "[IC] ");
    if (!IS_NPC(ch) && GET_SKILL(ch, SKILL_OVERWHELMING_CRIT))
      send_to_char(ch, "[OC] ");
    send_to_char(ch, "\r\n\r\n");
  }
  return dam;
}


//mode = 0	Normal damage calculating in hit()
//mode = 2	Display damage info primary
//mode = 3	Display damage info offhand
int calculate_hit_damage(struct char_data *ch, struct char_data *victim,
	struct obj_data *wielded, int w_type, int diceroll, int mode)
{
  int dam = 0;

  if (mode == 0) {
    dam = compute_dam_dice(ch, victim, wielded, mode);
    dam += compute_damage_bonus(ch, victim, w_type, 0, mode);
    switch (GET_POS(victim)) {
      case POS_SITTING:
        dam += 4;
        break;
      case POS_RESTING:
        dam += 6;
        break;
      case POS_SLEEPING:
        dam *= 2;
        break;
      case POS_STUNNED:
        dam *= 1.25;
        break;
      case POS_INCAP:
        dam *= 1.5;
        break;
      case POS_MORTALLYW:
      case POS_DEAD:
        dam *= 1.75;
        break;
      case POS_STANDING:
      case POS_FIGHTING:
      default: break;
    }
    dam = hit_dam_bonus(ch, victim, dam, diceroll, mode);
  } else if (mode == 2 || mode == 3) {
    dam = compute_dam_dice(ch, ch, NULL, mode);
    dam += compute_damage_bonus(ch, ch, 0, 0, mode);
    hit_dam_bonus(ch, ch, dam, 0, mode);
  }

  return MAX(1, dam);  //min damage of 1
}


#define STONESKIN_ABSORB	16
#define EPIC_WARDING_ABSORB	76
int handle_warding(struct char_data *ch, struct char_data *victim, int dam)
{
  int warding = 0;

  if (affected_by_spell(victim, SPELL_STONESKIN)) {
    if (GET_STONESKIN(victim) <= 0) {
      send_to_char(victim, "\tDYour skin has reverted from stone!\tn\r\n");
      affect_from_char(victim, SPELL_STONESKIN);
      GET_STONESKIN(victim) = 0;
      return dam;
    }
    warding = MIN(MIN(STONESKIN_ABSORB, GET_STONESKIN(victim)), dam);

    GET_STONESKIN(victim) -= warding;
    dam -= warding;
    if (GET_STONESKIN(victim) <= 0) {
      send_to_char(victim, "\tDYour skin has reverted from stone!\tn\r\n");
      affect_from_char(victim, SPELL_STONESKIN);
      GET_STONESKIN(victim) = 0;
    }
    if (dam <= 0) {
      send_to_char(victim, "\tWYour skin of stone absorbs the attack!\tn\r\n");
      send_to_char(ch,
	"\tRYou have failed to penetrate the stony skin of %s!\tn\r\n",GET_NAME(victim));
      act("$n fails to penetrate the stony skin of $N!", FALSE, ch, 0, victim,
	TO_NOTVICT);        
      return -1;
    } else {
      send_to_char(victim, "\tW<stone:%d>\tn", warding);
      send_to_char(ch, "\tR<oStone:%d>\tn", warding);
    }

  } else if (affected_by_spell(victim, SPELL_EPIC_WARDING)) {
    if (GET_STONESKIN(victim) <= 0) {
      send_to_char(victim, "\tDYour ward has fallen!\tn\r\n");
      affect_from_char(victim, SPELL_EPIC_WARDING);
      GET_STONESKIN(victim) = 0;
      return dam;
    }
    warding = MIN(EPIC_WARDING_ABSORB, GET_STONESKIN(victim));

    GET_STONESKIN(victim) -= warding;
    dam -= warding;
    if (GET_STONESKIN(victim) <= 0) {
      send_to_char(victim, "\tDYour ward has fallen!\tn\r\n");
      affect_from_char(victim, SPELL_EPIC_WARDING);
      GET_STONESKIN(victim) = 0;
    }
    if (dam <= 0) {
      send_to_char(victim, "\tWYour ward absorbs the attack!\tn\r\n");
      send_to_char(ch,
		"\tRYou have failed to penetrate the ward of %s!\tn\r\n",
                GET_NAME(victim));
      act("$n fails to penetrate the ward of $N!", FALSE, ch, 0, victim,
                TO_NOTVICT);        
      return -1;
    } else {
      send_to_char(victim, "\tW<ward:%d>\tn", warding);
      send_to_char(ch, "\tR<oWard:%d>\tn", warding);
    }

  } else {  // has no warding
    return dam;
  }

  return dam;
}
#undef STONESKIN_ABSORB
#undef EPIC_WARDING_ABSORB


void weapon_spells(struct char_data *ch, struct char_data *vict, struct obj_data *wpn)
{
  int i = 0, random;

  if(wpn && HAS_SPELLS(wpn)) {

    for (i = 0; i < MAX_WEAPON_SPELLS; i++) {
      if (GET_WEAPON_SPELL(wpn, i) && GET_WEAPON_SPELL_AGG(wpn, i)) {
        if (ch->in_room != vict->in_room) {
	  if (FIGHTING(ch) && FIGHTING(ch) == vict)
	    stop_fighting(ch);
	  return;
        }
        random = rand_number(1,100);
        if(GET_WEAPON_SPELL_PCT(wpn, i) >= random) {
	  act("$p leaps to action with an attack of its own.",
		TRUE, ch, wpn, 0, TO_CHAR);
	  act("$p leaps to action with an attack of its own.",
		TRUE, ch, wpn, 0, TO_ROOM);
          if(call_magic(ch, vict, NULL, GET_WEAPON_SPELL(wpn,i), GET_WEAPON_SPELL_LVL(wpn, i),
		CAST_WAND) < 0) return;
        }
      }
    }

  }

}


void idle_weapon_spells(struct char_data *ch)
{
      int random = 0, j = 0;
      struct obj_data *wielded = GET_EQ(ch, WEAR_WIELD_1);
      struct obj_data *offWield = GET_EQ(ch, WEAR_WIELD_2);
                        
      if (GET_EQ(ch, WEAR_WIELD_2H))
        wielded = GET_EQ(ch, WEAR_WIELD_2H);

      if (wielded && HAS_SPELLS(wielded)) {
        for (j = 0; j < MAX_WEAPON_SPELLS; j++) {
          if (!GET_WEAPON_SPELL_AGG(wielded, j) && GET_WEAPON_SPELL(wielded, j)) {
            random = rand_number(1,100);
            if(GET_WEAPON_SPELL_PCT(wielded, j) >= random) {
              act("$p leaps to action.",
                        TRUE, ch, wielded, 0, TO_CHAR);
              act("$p leaps to action.",
                        TRUE, ch, wielded, 0, TO_ROOM);
              call_magic(ch, ch, NULL, GET_WEAPON_SPELL(wielded, j),
                        GET_WEAPON_SPELL_LVL(wielded, j), CAST_WAND);
            }
          }
        }
      }

      if (offWield && HAS_SPELLS(offWield)) {
        for (j = 0; j < MAX_WEAPON_SPELLS; j++) {
          if (!GET_WEAPON_SPELL_AGG(offWield, j) && GET_WEAPON_SPELL(offWield, j)) {
            random = rand_number(1,100);
            if(GET_WEAPON_SPELL_PCT(offWield, j) >= random) {
              act("$p leaps to action.",
                        TRUE, ch, offWield, 0, TO_CHAR);
              act("$p leaps to action.",
                        TRUE, ch, offWield, 0, TO_ROOM);
              call_magic(ch, ch, NULL, GET_WEAPON_SPELL(offWield, j),
                        GET_WEAPON_SPELL_LVL(offWield, j), CAST_WAND);
            }
          }
        }
      }
}


void hit(struct char_data *ch, struct char_data *victim,
	int type, int dam_type, int penalty, int offhand)
{
  struct obj_data *wielded = GET_EQ(ch, WEAR_WIELD_1);
  int w_type, victim_ac, calc_bab, dam, diceroll;

  // primary hand setting
  if (GET_EQ(ch, WEAR_WIELD_2H))
    wielded = GET_EQ(ch, WEAR_WIELD_2H);

  if (!ch || !victim) return;  //ch and victim exist?
  if (IN_ROOM(ch) != IN_ROOM(victim)) {  //same room?
    if (FIGHTING(ch) && FIGHTING(ch) == victim)
      stop_fighting(ch);
    return;  }
  update_pos(ch);update_pos(victim);  //valid positions?
  if (GET_POS(ch) <= POS_DEAD || GET_POS(victim) <= POS_DEAD)    return;
  fight_mtrigger(ch);  //fight trig?

  // added these two checks in case parry is successful on opening attack -zusuk
  if (ch->nr != real_mobile(DG_CASTER_PROXY) &&
      ch != victim && ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL)) {
    send_to_char(ch, "This room just has such a peaceful, easy feeling...\r\n");
    stop_fighting(ch);
    return;
  }
  if (victim != ch) {
    if (GET_POS(ch) > POS_STUNNED && (FIGHTING(ch) == NULL))  // ch -> vict
      set_fighting(ch, victim);
    if (GET_POS(victim) > POS_STUNNED && (FIGHTING(victim) == NULL)) {  // vict -> ch
      set_fighting(victim, ch);
      if (MOB_FLAGGED(victim, MOB_MEMORY) && !IS_NPC(ch))
        remember(victim, ch);
    }   
  }

  // primary or offhand attack?
  if (offhand)
    wielded = GET_EQ(ch, WEAR_WIELD_2);
  if (wielded && GET_OBJ_TYPE(wielded) == ITEM_WEAPON)
    w_type = GET_OBJ_VAL(wielded, 3) + TYPE_HIT;
  else {
    if (IS_NPC(ch) && ch->mob_specials.attack_type != 0)
      w_type = ch->mob_specials.attack_type + TYPE_HIT;
    else
      w_type = TYPE_HIT;
  }

  // attack rolls:  1 = stumble, 20 = crit
  calc_bab = compute_bab(ch, victim, w_type) + penalty;
  victim_ac = compute_armor_class(ch, victim);
  diceroll = rand_number(1, 20);
  if (isCriticalHit(ch, diceroll)) {
    dam = TRUE;
  } else if (!AWAKE(victim)) {
    send_to_char(ch, "Can't miss a downed target...  ");
    dam = TRUE;
  } else if (diceroll == 1) {
    send_to_char(ch, "You stumble your attack...  ");
    dam = FALSE;
  } else {
    send_to_char(ch, "\tc{T:%d+", calc_bab);
    send_to_char(ch, "D:%d>=", diceroll);
    send_to_char(ch, "AC:%d}\tn", victim_ac);
    dam = (calc_bab + diceroll >= victim_ac);
  }

  //check parry attempt
  int parryDC = calc_bab + diceroll;
  if (!IS_NPC(victim) && compute_ability(victim, ABILITY_PARRY) &&
	PARRY_LEFT(victim) && AFF_FLAGGED(victim, AFF_PARRY)) {
    int parryAttempt = compute_ability(victim, ABILITY_PARRY) + dice(1,20);
    if (parryDC > parryAttempt) {
      send_to_char(victim, "You failed to \tcparry\tn the attack from %s!  ",
		GET_NAME(ch));
    } else if ((parryAttempt - parryDC) >= 10) {
      send_to_char(victim, "You deftly \tcriposte the attack\tn from %s!  ", 
		GET_NAME(ch));
      send_to_char(ch, "%s \tCparries\tn your attack and strikes back!  ",
		GET_NAME(victim));
      act("$N \tDripostes\tn an attack from $n!", FALSE, ch, 0, victim,
                TO_NOTVICT);
      hit(victim, ch, TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, FALSE);
      PARRY_LEFT(victim)--;
      return;
    } else {
      send_to_char(victim, "You \tcparry\tn the attack from %s!\r\n", GET_NAME(ch));
      send_to_char(ch, "%s \tCparries\tn your attack!\r\n", GET_NAME(victim));
      act("$N \tDparries\tn an attack from $n!", FALSE, ch, 0, victim,
                TO_NOTVICT);
      PARRY_LEFT(victim)--;
      return;
    }
  }

  if (!dam)  //miss
    damage(ch, victim, 0, type == SKILL_BACKSTAB ? SKILL_BACKSTAB : w_type, dam_type, offhand);
  else {
    if (affected_by_spell(ch, SPELL_TRUE_STRIKE)) {
      send_to_char(ch, "\tWTRUE-STRIKE\tn  ");
      affect_from_char(ch, SPELL_TRUE_STRIKE);
    }

    //calculate damage
    dam = calculate_hit_damage(ch, victim, wielded, w_type, diceroll, 0);
    if ((dam = handle_warding(ch, victim, dam)) == -1)
      return;

    if (type == SKILL_BACKSTAB)
      damage(ch, victim, dam * backstab_mult(GET_LEVEL(ch)),
		SKILL_BACKSTAB, dam_type, offhand);
    else
      damage(ch, victim, dam, w_type, dam_type, offhand);

    if (ch && victim)
      weapon_spells(ch, victim, wielded);

  }

  hitprcnt_mtrigger(victim);  //hitprcnt trigger
}



// now returns # of attacks and has mode functionality -zusuk
// mode = 0	normal attack routine
// mode = 1	return # of attacks, nothing else
// mode = 2	display attack routine potential

#define ATTACK_CAP	4
#define MONK_CAP	ATTACK_CAP + 2
#define TWO_WPN_PNLTY	-5
#define EPIC_TWO_PNLY	-7
int perform_attacks(struct char_data *ch, int mode)
{
  int i = 0, penalty = 0, numAttacks = 0, bonusAttacks = 0;
  bool dual = FALSE;

  //now lets determine base attack(s) and resulting possible penalty
  if (GET_EQ(ch, WEAR_WIELD_2))
    dual = TRUE;
  //default of one offhand attack for everyone
  if (dual) {
    numAttacks += 2;
    if (GET_SKILL(ch, SKILL_AMBIDEXTERITY))
      penalty = -1;
    else
      penalty = -4;
    if (mode == 0) {
      hit(ch, FIGHTING(ch), TYPE_UNDEFINED, DAM_RESERVED_DBC,
	penalty, FALSE);
      hit(ch, FIGHTING(ch), TYPE_UNDEFINED, DAM_RESERVED_DBC,
	penalty * 2, TRUE);
    } else if (mode == 2) {
      send_to_char(ch, "Mainhand, Attack Bonus:  %d; ",
	compute_bab(ch, ch, 0) + penalty);
      calculate_hit_damage(ch, ch, NULL, 0, 0, 2);
      send_to_char(ch, "Offhand, Attack Bonus:  %d; ",
	compute_bab(ch, ch, 0) + penalty * 2);
      calculate_hit_damage(ch, ch, NULL, 0, 0, 3);
    }
  } else {
    //default of one attack for everyone
    numAttacks++;
    if (mode == 0) {
      hit(ch, FIGHTING(ch), TYPE_UNDEFINED, DAM_RESERVED_DBC, penalty, FALSE);
    } else if (mode == 2) {
      send_to_char(ch, "Mainhand, Attack Bonus:  %d; ",
	compute_bab(ch, ch, 0) + penalty);
      calculate_hit_damage(ch, ch, NULL, 0, 0, 2);
    }
  }
  if (AFF_FLAGGED(ch, AFF_HASTE) ||
	(!IS_NPC(ch) && GET_SKILL(ch, SKILL_BLINDING_SPEED))) {
    numAttacks++;
    if (mode == 0) {
      hit(ch, FIGHTING(ch), TYPE_UNDEFINED, DAM_RESERVED_DBC, penalty, FALSE);
    } else if (mode == 2) {
      send_to_char(ch, "Mainhand (Haste), Attack Bonus:  %d; ",
	compute_bab(ch, ch, 0) + penalty);
      calculate_hit_damage(ch, ch, NULL, 0, 0, 2);
    }
  }

  //now level based bonus attacks
  int monkMode = FALSE;
  bonusAttacks = MIN((BAB(ch) - 1) / 5, ATTACK_CAP);
  if (CLASS_LEVEL(ch, CLASS_MONK)) {
    if(!GET_EQ(ch, WEAR_HOLD_1)
        && !GET_EQ(ch, WEAR_WIELD_1)
        && !GET_EQ(ch, WEAR_HOLD_2) 
        && !GET_EQ(ch, WEAR_WIELD_2)
        && !GET_EQ(ch, WEAR_SHIELD)
	&& !GET_EQ(ch, WEAR_WIELD_2H)
	) {
      bonusAttacks = MIN((BAB(ch) - 1) / 3, MONK_CAP);
      monkMode = TRUE;
    }
  }

  //execute
  for (i = 0; i < bonusAttacks; i++) {
    if (monkMode)
      penalty -= 3;
    else
      penalty -= 5;
    numAttacks++;
    if (FIGHTING(ch) && mode == 0) {
      update_pos(FIGHTING(ch));
      if (GET_POS(FIGHTING(ch)) != POS_DEAD &&
	IN_ROOM(FIGHTING(ch)) == IN_ROOM(ch)) {
        hit(ch, FIGHTING(ch), TYPE_UNDEFINED, DAM_RESERVED_DBC, penalty, FALSE);
      }
    } else if (mode == 2) {
      send_to_char(ch, "Mainhand Bonus %d, Attack Bonus:  %d; ",
	i + 1, compute_bab(ch, ch, 0) + penalty);
      calculate_hit_damage(ch, ch, NULL, 0, 0, 2);
    }
  }

  //additional off-hand attacks
  if (dual) {
    if (!IS_NPC(ch) && GET_SKILL(ch, SKILL_TWO_WEAPON_FIGHT)) {
      numAttacks++;
      if (mode == 0) {
        hit(ch, FIGHTING(ch), TYPE_UNDEFINED, DAM_RESERVED_DBC, TWO_WPN_PNLTY, TRUE);
      } else if (mode == 2) {
        send_to_char(ch, "Offhand (2 Weapon Fighting), Attack Bonus:  %d; ",
		compute_bab(ch, ch, 0) + TWO_WPN_PNLTY);
        calculate_hit_damage(ch, ch, NULL, 0, 0, 3);
      }
    }
    if (!IS_NPC(ch) && GET_SKILL(ch, SKILL_EPIC_2_WEAPON)) {
      numAttacks++;
      if (mode == 0) {
        hit(ch, FIGHTING(ch), TYPE_UNDEFINED, DAM_RESERVED_DBC, EPIC_TWO_PNLY, TRUE);
      } else if (mode == 2) {
        send_to_char(ch, "Offhand (Epic 2 Weapon Fighting), Attack Bonus:  %d; ",
		compute_bab(ch, ch, 0) + EPIC_TWO_PNLY);
        calculate_hit_damage(ch, ch, NULL, 0, 0, 3);
      }
    }
  }

  return numAttacks++;
}
#undef ATTACK_CAP
#undef MONK_CAP
#undef TWO_WPN_PNLTY
#undef EPIC_TWO_PNLY


void autoDiagnose(struct char_data *ch)
{
  struct char_data *char_fighting = NULL, *tank = NULL;
  int percent;

  char_fighting = FIGHTING(ch);
  if (char_fighting && (ch->in_room == char_fighting->in_room) &&
	GET_HIT(char_fighting) > 0) {
                
    if ((tank = char_fighting->char_specials.fighting) &&
	(ch->in_room == tank->in_room)) {
       
      send_to_char(ch, "\r\n%s|T:%s%s",
                CCCYN(ch,C_NRM), CCNRM(ch,C_NRM),
                (CAN_SEE(ch, tank)) ? GET_NAME(tank) : "someone");
      send_to_char(ch, "%s-TC:",
                CCCYN(ch,C_NRM));
        
      if (GET_MAX_HIT(tank) > 0)
        percent = (100 * GET_HIT(tank)) / GET_MAX_HIT(tank);
      else
        percent = -1;
      if (percent >= 100) {
        send_to_char(ch, CBWHT(ch, C_NRM));
        send_to_char(ch, "excellent");
      } else if (percent >= 95) {
        send_to_char(ch, CCNRM(ch, C_NRM));
        send_to_char(ch, "few scratches");
      } else if (percent >= 75) {
        send_to_char(ch, CBGRN(ch, C_NRM));
        send_to_char(ch, "small wounds");
      } else if (percent >= 55) {
        send_to_char(ch, CBBLK(ch, C_NRM));
        send_to_char(ch, "few wounds");
      } else if (percent >= 35) {
        send_to_char(ch, CBMAG(ch, C_NRM));
        send_to_char(ch, "nasty wounds");
      } else if (percent >= 15) {
        send_to_char(ch, CBBLU(ch, C_NRM));
        send_to_char(ch, "pretty hurt");
      } else if (percent >= 1) {
        send_to_char(ch, CBRED(ch, C_NRM));
        send_to_char(ch, "awful");
      } else {
        send_to_char(ch, CBFRED(ch, C_NRM));
        send_to_char(ch, "bleeding, close to death");
        send_to_char(ch, CCNRM(ch, C_NRM));
      }
    } // end tank
          
    send_to_char(ch, "%s|E:%s%s",
        CBCYN(ch,C_NRM), CCNRM(ch,C_NRM),
        (CAN_SEE(ch, char_fighting)) ? GET_NAME(char_fighting) : "someone");
        
    send_to_char(ch, "%s-EC:",
        CBCYN(ch,C_NRM));
        
    if (GET_MAX_HIT(char_fighting) > 0)
      percent = (100 * GET_HIT(char_fighting)) / GET_MAX_HIT(char_fighting);
    else
      percent = -1;
    if (percent >= 100) {
      send_to_char(ch, CBWHT(ch, C_NRM));
      send_to_char(ch, "excellent");
    } else if (percent >= 95) {
      send_to_char(ch, CCNRM(ch, C_NRM));
      send_to_char(ch, "few scratches");
    } else if (percent >= 75) {
      send_to_char(ch, CBGRN(ch, C_NRM));
      send_to_char(ch, "small wounds");
    } else if (percent >= 55) {
      send_to_char(ch, CBBLK(ch, C_NRM));
      send_to_char(ch, "few wounds");
    } else if (percent >= 35) {
      send_to_char(ch, CBMAG(ch, C_NRM));
      send_to_char(ch, "nasty wounds");
    } else if (percent >= 15) {
      send_to_char(ch, CBBLU(ch, C_NRM));
      send_to_char(ch, "pretty hurt");
    } else if (percent >= 1) {
      send_to_char(ch, CBRED(ch, C_NRM));
      send_to_char(ch, "awful");
    } else {
      send_to_char(ch, CBFRED(ch, C_NRM));
      send_to_char(ch, "bleeding, close to death");
      send_to_char(ch, CCNRM(ch, C_NRM));
    }
    send_to_char(ch, "\tn|\r\n\r\n");
  }
}


/* control the fights going on.  Called every 2 seconds from comm.c. */
void perform_violence(void)
{
  struct char_data *ch, *tch, *charmee;

  for (ch = combat_list; ch; ch = next_combat_list) {
    next_combat_list = ch->next_fighting;
    PARRY_LEFT(ch) = perform_attacks(ch, 1);

    if (FIGHTING(ch) == NULL || IN_ROOM(ch) != IN_ROOM(FIGHTING(ch))) {
      stop_fighting(ch);
      continue;
    }

    if (AFF_FLAGGED(ch, AFF_PARALYZED)) {
      send_to_char(ch, "You are paralyzed and unable to react!\r\n");
      act("$n seems to be paralyzed and unable to react!", TRUE, ch, 0, 0, TO_ROOM);
      continue;
    }
    if (char_has_mud_event(ch, eSTUNNED)) {
      send_to_char(ch, "You are stunned and unable to react!\r\n");
      act("$n seems to be stunned and unable to react!", TRUE, ch, 0, 0, TO_ROOM);
      continue;
    }

    if (IS_NPC(ch)) {
      if (GET_MOB_WAIT(ch) > 0) {
	GET_MOB_WAIT(ch) -= PULSE_VIOLENCE;
      } else {
        GET_MOB_WAIT(ch) = 0;
        if (GET_POS(ch) < POS_FIGHTING) {
          GET_POS(ch) = POS_FIGHTING;
          send_to_char(ch, "You scramble to your feet!\r\n");
	  act("$n scrambles to $s feet!", TRUE, ch, 0, 0, TO_ROOM);
        }
      }
    }

    /* Positions
	POS_DEAD	0
	POS_MORTALLYW	1
	POS_INCAP	2
	POS_STUNNED	3
	POS_SLEEPING	4
	POS_RESTING	5
	POS_SITTING	6
	POS_FIGHTING	7
	POS_STANDING	8	*/
    if (GET_POS(ch) < POS_SITTING) {
      send_to_char(ch, "You are in no position to fight!!\r\n");
      continue;
    }

    if (GROUP(ch)) {
      while ((tch = (struct char_data *) simple_list(GROUP(ch)->members)) != NULL) {
        if (tch == ch)
          continue;
        if (!IS_NPC(tch) && !PRF_FLAGGED(tch, PRF_AUTOASSIST))
          continue;
        if (IN_ROOM(ch) != IN_ROOM(tch))
          continue;
        if (FIGHTING(tch))
          continue;
        if (GET_POS(tch) != POS_STANDING)
          continue;
        if (!CAN_SEE(tch, ch))
          continue;
      
        do_assist(tch, GET_NAME(ch), 0, 0);				  
      }
    }

    //your charmee, even if not groupped, should assist
    for (charmee = world[IN_ROOM(ch)].people; charmee; charmee = charmee->next_in_room)
      if (AFF_FLAGGED(charmee, AFF_CHARM) && charmee->master == ch && !FIGHTING(charmee) &&
		GET_POS(charmee) == POS_STANDING && CAN_SEE(charmee, ch))
        do_assist(charmee, GET_NAME(ch), 0, 0);

    if (AFF_FLAGGED(ch, AFF_PARRY))
      send_to_char(ch, "You continue the battle in defensive positioning!\r\n");

    if (!IS_CASTING(ch) && !AFF_FLAGGED(ch, AFF_PARRY))
      perform_attacks(ch, 0);

    autoDiagnose(ch);

    if (MOB_FLAGGED(ch, MOB_SPEC) && GET_MOB_SPEC(ch) && !MOB_FLAGGED(ch, MOB_NOTDEADYET)) {
      char actbuf[MAX_INPUT_LENGTH] = "";
      (GET_MOB_SPEC(ch)) (ch, ch, 0, actbuf);
    }

    if (AFF_FLAGGED(ch, AFF_FEAR) && !rand_number(0,2)) {
      send_to_char(ch, "\tDFear\tc overcomes you!\tn  ");
      act("$n \tcis overcome with \tDfear\tc!\tn",
		TRUE, ch, 0, 0, TO_ROOM);
      perform_flee(ch);
    }

  }  //end for loop
}
