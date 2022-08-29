/* *************************************************************************
 *   File: hlquest.c                                   Part of LuminariMUD *
 *  Usage: alternate quest system                                          *
 *  Author: Homeland, ported to tba/luminari by Zusuk                      *
 ************************************************************************* */

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "hlquest.h"
#include "spells.h"
#include "race.h"
#include "class.h"
#include "fight.h"
#include "act.h"
#include "constants.h"
#include "mud_event.h"
#include "actions.h"
#include "spell_prep.h"

/* cheesy lich hack */
#define LICH_QUEST 9999

/*-----------------------------------*/
/* utility functions */
/*-----------------------------------*/

/* homeland-port this eventually can be used to have special class
   quests */
int has_race_kit(int race, int c)
{
  // return has_kit[race][c];
  return TRUE;
}

/* this function will have the quest mob open a specific door */
void quest_open_door(int room, int door)
{
  int other_room = 0;
  struct obj_data *dummy = 0;
  struct room_direction_data *back = 0;

  if ((other_room = EXITN(room, door)->to_room) != NOWHERE)
  {
    if ((back = world[other_room].dir_option[rev_dir[door]]))
    {
      if (back->to_room != room)
        back = 0;
    }
  }

  if (EXIT_FLAGGED(world[room].dir_option[door], EX_LOCKED))
    UNLOCK_DOOR(room, dummy, door);

  OPEN_DOOR(room, dummy, door);

  if (back)
  {
    if (EXIT_FLAGGED(world[other_room].dir_option[rev_dir[door]], EX_LOCKED))
      UNLOCK_DOOR(other_room, dummy, rev_dir[door]);
    OPEN_DOOR(other_room, dummy, rev_dir[door]);
  }
}

/* this utility function display quest info to a viewer in nice format */
void show_quest_to_player(struct char_data *ch, struct quest_entry *quest)
{
  struct quest_command *qcom = NULL;
  char buf[MAX_INPUT_LENGTH] = {'\0'};

  if (!quest)
    return;
  if (!ch)
    return;

  if (quest->approved)
    send_to_char(ch, "\tc*** \tCQuest is Approved\tc ***\tn\r\n");
  else
    send_to_char(ch, "\tr*** \tRQuest is NOT APPROVED!\tr ***\tn\r\n");
  if (quest->type == QUEST_ASK)
  {
    snprintf(buf, sizeof(buf), "\tCASK\tn %s\r\n", quest->keywords);
    send_to_char(ch, buf);
  }
  else
  {
    if (quest->type == QUEST_ROOM)
    {
      snprintf(buf, sizeof(buf), "\tCROOM\tn %d\r\n", quest->room);
      send_to_char(ch, buf);
    }
    else
    {

      for (qcom = quest->in; qcom; qcom = qcom->next)
      {
        switch (qcom->type)
        {
        case QUEST_COMMAND_ITEM:
          if (NOTHING == real_object(qcom->value))
          {
            send_to_char(ch, "\tCGIVE\tn <Missing Object> %d\r\n", qcom->value);
            break;
          }
          snprintf(buf, sizeof(buf), "\tCGIVE\tn %s (%d)\r\n",
                   obj_proto[real_object(qcom->value)].short_description,
                   qcom->value);
          send_to_char(ch, buf);
          break;
        case QUEST_COMMAND_COINS:
          snprintf(buf, sizeof(buf), "\tCGIVE\tn %d coins\r\n", qcom->value);
          send_to_char(ch, buf);
          break;
        }
      }
    }
    for (qcom = quest->out; qcom; qcom = qcom->next)
    {
      switch (qcom->type)
      {
      case QUEST_COMMAND_ITEM:
        if (NOTHING == real_object(qcom->value))
        {
          send_to_char(ch, "\tcRECEIVE\tn <Missing Object> %d\r\n", qcom->value);
          break;
        }
        snprintf(buf, sizeof(buf), "\tcRECEIVE\tn %s (%d)\r\n",
                 obj_proto[real_object(qcom->value)].short_description, qcom->value);
        send_to_char(ch, buf);
        break;
      case QUEST_COMMAND_COINS:
        snprintf(buf, sizeof(buf), "\tcRECEIVE\tn %d coins\r\n", qcom->value);
        send_to_char(ch, buf);
        break;
      case QUEST_COMMAND_LOAD_OBJECT_INROOM:
        if (NOTHING == real_object(qcom->value))
        {
          send_to_char(ch, "\tcLOADOBJECT\tn <Missing Object> %d\r\n", qcom->value);
          break;
        }
        if (NOWHERE == real_room(qcom->location))
        {
          send_to_char(ch, "\tcLOADOBJECT\tn <Missing Room>\r\n");
          break;
        }
        snprintf(buf, sizeof(buf), "\tcLOADOBJECT\tn %s in %s\r\n",
                 obj_proto[real_object(qcom->value)].short_description,
                 (qcom->location == 0 ? "CurrentRoom" : world[real_room(qcom->location)].name));
        send_to_char(ch, buf);
        break;
      case QUEST_COMMAND_OPEN_DOOR:
        if (NOWHERE == real_room(qcom->location))
        {
          send_to_char(ch, "\tcOPEN_DOOR\tn <Missing Room>\r\n");
          break;
        }
        snprintf(buf, sizeof(buf), "\tcOPEN_DOOR\tn %s in %s(%d)\r\n", dirs[qcom->value],
                 world[real_room(qcom->location)].name, qcom->location);
        send_to_char(ch, buf);
        break;
      case QUEST_COMMAND_FOLLOW:
        send_to_char(ch, "\tcFOLLOW\tn questmob following player\r\n");
        break;
      case QUEST_COMMAND_CHURCH:
        snprintf(buf, sizeof(buf), "\tcSET_CHURCH\tn of player to of %s.\r\n",
                 church_types[qcom->value]);
        send_to_char(ch, buf);
        break;
      case QUEST_COMMAND_KIT:
        if (qcom->location == LICH_QUEST || qcom->value == LICH_QUEST)
        {
          send_to_char(ch, "\tcSET_KIT\tn character will become a LICH (race).\r\n");
        }
        else if (qcom->location >= NUM_CLASSES || qcom->location >= NUM_CLASSES)
        {
          send_to_char(ch, "Invalid Class # set for this quest!\r\n");
        }
        else
        {
          snprintf(buf, sizeof(buf), "\tcSET_KIT\tn of a %s to become %s.\r\n",
                   CLSLIST_NAME(qcom->location),
                   CLSLIST_NAME(qcom->location));
          send_to_char(ch, buf);
        }
        break;
      case QUEST_COMMAND_LOAD_MOB_INROOM:
        if (NOBODY == real_mobile(qcom->value))
        {
          send_to_char(ch, "\tcLOADMOB\tn <Missing Mobile>\r\n");
          break;
        }
        snprintf(buf, sizeof(buf), "\tcLOADMOB\tn %s in %s\r\n",
                 mob_proto[real_mobile(qcom->value)].player.short_descr,
                 (qcom->location == 0 ? "CurrentRoom" : world[real_room(qcom->location)].name));
        send_to_char(ch, buf);
        break;
      case QUEST_COMMAND_ATTACK_QUESTOR:
        send_to_char(ch, "\tcMOB_ATTACK\tn player!\r\n");
        break;
      case QUEST_COMMAND_DISAPPEAR:
        send_to_char(ch, "\tcREMOVE_MOB\tn\r\n");
        break;
      case QUEST_COMMAND_TEACH_SPELL:
        if (qcom->value <= SPELL_RESERVED_DBC || qcom->value >= NUM_SPELLS)
        {
          send_to_char(ch, "\tcTEACH-SPELL\tn <Invalid Spellnum>\r\n");
          break;
        }
        snprintf(buf, sizeof(buf), "\tcTEACH_SPELL\tn %s\r\n",
                 spell_info[qcom->value].name);
        send_to_char(ch, buf);
        break;
      case QUEST_COMMAND_CAST_SPELL:
        if (qcom->value <= SPELL_RESERVED_DBC || qcom->value >= NUM_SPELLS)
        {
          send_to_char(ch, "\tcCAST-SPELL\tn <Invalid Spellnum>\r\n");
          break;
        }
        snprintf(buf, sizeof(buf), "\tcCAST_SPELL\tn %s\r\n",
                 spell_info[qcom->value].name);
        send_to_char(ch, buf);
        break;
      }
    }
  }
  send_to_char(ch, quest->reply_msg);
}

/* utility function to check if there is a spell reward for all quests */
bool has_spell_a_quest(int spell)
{
  int i;
  struct quest_entry *quest;
  struct quest_command *qcom;
  for (i = 0; i < top_of_mobt; i++)
  {
    if (mob_proto[i].mob_specials.quest)
    {
      for (quest = mob_proto[i].mob_specials.quest; quest;
           quest = quest->next)
      {
        for (qcom = quest->out; qcom; qcom = qcom->next)
        {
          if (qcom->type == QUEST_COMMAND_TEACH_SPELL && qcom->value == spell)
            return TRUE;
        }
      }
    }
  }
  return FALSE;
}

/* utility function used to return items given in quest */
void give_back_items(struct char_data *questor, struct char_data *player,
                     struct quest_entry *quest)
{
  struct quest_command *qcom;
  struct obj_data *obj;

  for (qcom = quest->in; qcom; qcom = qcom->next)
  {
    switch (qcom->type)
    {
    case QUEST_COMMAND_ITEM:
      obj = get_obj_in_list_num(real_object(qcom->value),
                                questor->carrying);
      if (obj)
      {
        obj_from_char(obj);
        obj_to_char(obj, player);
      }
      break;
    }
  }
}

/* utility function will identify if given item is involved in a quest */
bool is_object_in_a_quest(struct obj_data *obj)
{
  int i;
  int vnum = 0;
  struct quest_entry *quest;
  struct quest_command *qcom;

  if (!obj)
    return FALSE;

  vnum = GET_OBJ_VNUM(obj);

  for (i = 0; i < top_of_mobt; i++)
  {
    if (mob_proto[i].mob_specials.quest)
    {
      for (quest = mob_proto[i].mob_specials.quest; quest;
           quest = quest->next)
      {
        // check in.
        for (qcom = quest->in; qcom; qcom = qcom->next)
        {
          if (qcom->value == vnum && qcom->type == QUEST_COMMAND_ITEM)
            return TRUE;
        }
        // check out.
        for (qcom = quest->out; qcom; qcom = qcom->next)
        {
          if (qcom->value == vnum)
          {
            switch (qcom->type)
            {
            case QUEST_COMMAND_ITEM:
              return TRUE;
            case QUEST_COMMAND_LOAD_OBJECT_INROOM:
              return TRUE;
            }
          }
        }
      }
    }
  }
  return FALSE;
}

/* this is the main driver for the quest-out quest-reward system */

void perform_out_chain(struct char_data *ch, struct char_data *victim,
                       struct quest_entry *quest, char *name)
{
  struct char_data *mob = NULL;
  struct quest_command *qcom = NULL;
  struct char_data *homie = NULL, *nexth = NULL;
  struct obj_data *obj = NULL;
  char buf[MAX_INPUT_LENGTH] = {'\0'};
  int i = 0;

  // heh.. give stuff..
  act(quest->reply_msg, FALSE, ch, 0, victim, TO_CHAR);

  for (qcom = quest->out; qcom; qcom = qcom->next)
  {
    switch (qcom->type)
    {
    case QUEST_COMMAND_COINS:
      if (GET_GOLD(ch) + qcom->value <= MAX_GOLD)
        GET_GOLD(ch) += qcom->value;
      else
        GET_GOLD(ch) = MAX_GOLD;
      send_to_char(ch, "You receive %d \tYcoins\tn.\r\n", qcom->value);
      break;
    case QUEST_COMMAND_ITEM:
      obj = read_object(qcom->value, VIRTUAL);

      if (obj)
      {
        obj_to_char(obj, victim);
        if (FALSE == perform_give(victim, ch, obj))
        {
          act("$n drops $p at the ground.", TRUE, victim, obj, 0, TO_ROOM);
          obj_from_char(obj);
          obj_to_room(obj, ch->in_room);
        }
      }
      break;
    case QUEST_COMMAND_LOAD_OBJECT_INROOM:
      obj = read_object(qcom->value, VIRTUAL);
      if (obj && qcom->location == 0)
        obj_to_room(obj, victim->in_room);
      else if (obj)
        obj_to_room(obj, real_room(qcom->location));
      break;
    case QUEST_COMMAND_LOAD_MOB_INROOM:
      mob = read_mobile(qcom->value, VIRTUAL);
      if (mob && qcom->location == 0)
        char_to_room(mob, victim->in_room);
      else if (mob)
        char_to_room(mob, real_room(qcom->location));
      break;
    case QUEST_COMMAND_ATTACK_QUESTOR:
      hit(victim, ch, TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, FALSE);
      break;
    case QUEST_COMMAND_OPEN_DOOR:
      quest_open_door(real_room(qcom->location), qcom->value);
      break;
    case QUEST_COMMAND_DISAPPEAR:
      char_from_room(victim);
      char_to_room(victim, real_room(1));
      if (victim->master)
        stop_follower(victim);
      /* getting rid of his/her pets too */
      for (homie = world[victim->in_room].people; homie; homie = nexth)
      {
        nexth = homie->next_in_room;
        if (IS_NPC(homie) && homie->master == victim)
        {
          char_from_room(homie);
          char_to_room(homie, real_room(1));
        }
      }
      break;
    case QUEST_COMMAND_FOLLOW:
      if (circle_follow(victim, ch))
      {
        send_to_char(ch, "Sorry, following in circles can not be"
                         " allowed.\r\n");
        return;
      }
      if (victim->master)
      {
        send_to_char(ch, "Sorry, I am already following someone else.\r\n");
        return;
      }

      add_follower(victim, ch);
      SET_BIT_AR(AFF_FLAGS(victim), AFF_CHARM);

      break;

      /* unfinished for luminari port */
    case QUEST_COMMAND_CHURCH:
      GET_CHURCH(ch) = qcom->value;
      snprintf(buf, sizeof(buf), "You are now a servant of %s.\r\n",
               church_types[GET_CHURCH(ch)]);
      send_to_char(ch, buf);
      break;

      /* unfinished for luminari port */
    case QUEST_COMMAND_KIT:
      if (qcom->value == LICH_QUEST || qcom->location == LICH_QUEST)
      {
        // hack for lich remort..

        GET_REAL_RACE(ch) = RACE_LICH;
        // GET_HOMETOWN(ch) = 3; /*Zhentil Keep*/s

        respec_engine(ch, CLASS_WIZARD, NULL, TRUE);
        GET_EXP(ch) = 0;
        GET_ALIGNMENT(ch) = -1000;

        send_to_char(ch, "\tLYour \tWlifeforce\tL is ripped apart of you,"
                         " and you realize\tn\r\n"
                         "\tLthat you are dieing. Your body is now merely a "
                         "vessel for your power.\tn\r\n");

        send_to_char(ch, "You are now a \tLLICH!\tn\r\n");
        log("Quest Log : %s have changed into to a LICH!", GET_NAME(ch));

        return;
      }

      if (GET_CLASS(ch) != qcom->location)
      {
        if (qcom->location <= CLASS_UNDEFINED || qcom->location >= NUM_CLASSES)
        {
          snprintf(buf, sizeof(buf), "This quest is broken, please report it to the staff.\r\n");
          give_back_items(victim, ch, quest);
          send_to_char(ch, buf);
        }
        else
        {
          snprintf(buf, sizeof(buf), "You need to be a %s to learn how to be a %s.\r\n",
                   CLSLIST_NAME(qcom->location), CLSLIST_NAME(qcom->value));
          give_back_items(victim, ch, quest);
          send_to_char(ch, buf);
          log("quest_log : %s failed to do a kitquest.(Not right class)",
              GET_NAME(ch));
        }
      }
      else if (!has_race_kit(GET_RACE(ch), qcom->value))
      {
        if (qcom->location <= CLASS_UNDEFINED || qcom->location >= NUM_CLASSES)
        {
          snprintf(buf, sizeof(buf), "This quest is broken, please report it to the staff.\r\n");
          give_back_items(victim, ch, quest);
          send_to_char(ch, buf);
        }
        else
        {
          snprintf(buf, sizeof(buf), "Your race can NEVER learn how to become a %s.\r\n",
                   race_list[qcom->value].type_color);
          send_to_char(ch, buf);
          give_back_items(victim, ch, quest);
          log("quest_log : %s failed to do a kitquest.(Not right race)",
              GET_NAME(ch));
        }
      }
      else if (GET_LEVEL(ch) < 10)
      {
        send_to_char(ch, "You are too low level to do this now.\r\n");
        log("quest_log : %s failed to do a kitquest.(too low level)",
            GET_NAME(ch));
        give_back_items(victim, ch, quest);
      }
      else if (GET_LEVEL(ch) < (LVL_IMMORT - 1) &&
               (qcom->value == LICH_QUEST || qcom->location == LICH_QUEST))
      {
        send_to_char(ch, "You are too low level (min 30) to do this now.\r\n");
        log("quest_log : %s failed to do a kitquest.(too low level)",
            GET_NAME(ch));
        give_back_items(victim, ch, quest);
      }
      else if ((GROUP(ch) || ch->master || ch->followers) && (qcom->value == LICH_QUEST || qcom->location == LICH_QUEST))
      {
        send_to_char(ch, "You cannot be part of a group, be following someone, or have followers of your own to respec.\r\n"
                         "You can dismiss npc followers with the 'dismiss' command.\r\n");
        log("quest_log : %s failed to do a kitquest.(followers)",
            GET_NAME(ch));
        give_back_items(victim, ch, quest);
      }
      else
      {
        if (qcom->value != LICH_QUEST)
          ch->player.chclass = qcom->value;
        destroy_spell_prep_queue(ch);
        destroy_innate_magic_queue(ch);
        destroy_spell_collection(ch);
        destroy_known_spells(ch);

        for (i = 0; i < MAX_SKILLS; i++)
        {
          if (GET_SKILL(ch, i) &&
              spell_info[i].min_level[(int)GET_CLASS(ch)] >
                  GET_LEVEL(ch))
          {
            GET_SKILL(ch, i) = 0;
          }
        }

        snprintf(buf, sizeof(buf), "You are now a %s.\r\n", CLSLIST_NAME(GET_CLASS(ch)));
        send_to_char(ch, buf);
        log("quest_log : %s have changed to %s", GET_NAME(ch),
            CLSLIST_NAME(GET_CLASS(ch)));
      }
      break;

      /* unfinished for luminari port */
    case QUEST_COMMAND_TEACH_SPELL:
      if (GET_LEVEL(ch) <
          spell_info[qcom->value].min_level[(int)GET_CLASS(ch)])
        send_to_char(ch, "The teaching is way beyond your comprehension!\r\n");
      else if (GET_SKILL(ch, qcom->value) > 0)
        send_to_char(ch, "You realize that you already know this way too "
                         "well.\r\n");
      else
      {
        snprintf(buf, sizeof(buf), "$N teaches you '\tL%s\tn'",
                 spell_info[qcom->value].name);
        act(buf, FALSE, ch, 0, victim, TO_CHAR);
        GET_SKILL(ch, qcom->value) = 9;
        snprintf(buf, sizeof(buf), "quest_log: %s has quested %s", GET_NAME(ch),
                 spell_info[qcom->value].name);
        log(buf);
      }
      break;

      /* took out casting time for this */
    case QUEST_COMMAND_CAST_SPELL:
      call_magic(victim, ch, 0, qcom->value, 0, GET_LEVEL(victim), CAST_SPELL);
      break;
    }
  }
}

/* quest_room is the function that launches quest out if you bring in
   a specific mobile into the (given) room (the quest mobile) */
void quest_room(struct char_data *ch)
{
  struct quest_entry *quest;

  if (!IS_NPC(ch))
    return;
  if (!ch->master)
    return;

  for (quest = ch->mob_specials.quest; quest; quest = quest->next)
  {
    /* Mortals can only quest on approved quests */
    if (quest->type == QUEST_ROOM && ch->in_room == real_room(quest->room) &&
        ch->in_room == ch->master->in_room)
    {
      perform_out_chain(ch->master, ch, quest, GET_NAME(ch->master));
      return;
    }
  }
}

/* this function will determine whether the quest-out will fire given
 * a certain set of keywords
 */
void quest_ask(struct char_data *ch, struct char_data *victim, char *keyword)
{
  struct quest_entry *quest = NULL;
  char buf[MAX_INPUT_LENGTH] = {'\0'};

  if (IS_NPC(ch))
    return;
  if (!IS_NPC(victim))
    return;

  if (GET_LEVEL(ch) >= LVL_IMMORT && GET_LEVEL(ch) < LVL_IMPL)
  {
    snprintf(buf, sizeof(buf), "(GC) %s asked '%s' on %s (%d).", GET_NAME(ch), keyword,
             GET_NAME(victim), GET_MOB_VNUM(victim));
    log(buf);
  }

  for (quest = victim->mob_specials.quest; quest; quest = quest->next)
  {

    if (!quest || !ch)
      continue;

    if (!quest->keywords || !quest->reply_msg)
      continue;

    /* Mortals can only quest on approved quests */
    if (quest->approved == FALSE && GET_LEVEL(ch) < LVL_IMMORT)
      continue;

    if (quest->type == QUEST_ASK && isname(keyword, quest->keywords))
    {
      act(quest->reply_msg, FALSE, ch, 0, victim, TO_CHAR);
      return;
    }
  }
}

/* this function will determine whether the quest-out will fire when
 * the quest mob receives items/coins
 */
void quest_give(struct char_data *ch, struct char_data *victim)
{
  struct quest_entry *quest = NULL;
  struct quest_command *qcom = NULL;
  bool fullfilled = FALSE;
  struct obj_data *obj = NULL;

  if (!ch || !victim)
    return;
  if (IS_NPC(ch))
    return;
  if (!IS_NPC(victim))
    return;

  for (quest = victim->mob_specials.quest; quest; quest = quest->next)
  {
    /* Mortals can only quest on approved quests */
    if (victim && quest && ch)
      if (quest->approved == FALSE && GET_LEVEL(ch) < LVL_IMMORT)
        continue;

    if (quest->type == QUEST_GIVE)
    {
      fullfilled = TRUE;
      for (qcom = quest->in; qcom && fullfilled == TRUE; qcom = qcom->next)
      {
        if (qcom)
        {
          switch (qcom->type)
          {
          case QUEST_COMMAND_COINS:
            if (GET_GOLD(victim) < qcom->value)
              fullfilled = FALSE;
            break;
          case QUEST_COMMAND_ITEM:
            /* if object doesn't exist, we can't ask for it */
            if (NOTHING == real_object(qcom->value))
              continue;
            if (!get_obj_in_list_num(real_object(qcom->value),
                                     victim->carrying))
              fullfilled = FALSE;
            break;
          }
        }
      }
      if (fullfilled)
      {
        // remove given items from inventory...
        for (qcom = quest->in; qcom; qcom = qcom->next)
        {
          switch (qcom->type)
          {
          case QUEST_COMMAND_COINS:
            GET_GOLD(victim) = 0;
            break;
          case QUEST_COMMAND_ITEM:
            obj = get_obj_in_list_num(real_object(qcom->value),
                                      victim->carrying);
            if (obj)
            {
              obj_from_char(obj);
              obj_to_room(obj, real_room(1));
            }
            break;
          }
        }
        perform_out_chain(ch, victim, quest, GET_NAME(ch));
      }
    }
  }
}

/*
 * init a quest
 */
void clear_hlquest(struct quest_entry *quest)
{
  quest->type = -1;
  quest->keywords = strdup("hi hello quest");
  quest->reply_msg = strdup("Undefined Quest");
  quest->in = NULL;
  quest->out = NULL;
  quest->approved = FALSE;
  quest->room = NOWHERE;

  quest->next = NULL;
}

/*
 * free the quests (all of them)
 */
void free_hlquests(struct quest_entry *quest)
{
  struct quest_entry *next;
  struct quest_command *qcom;
  while (quest)
  {
    next = quest->next;
    free(quest->keywords);
    free(quest->reply_msg);
    while (quest->in)
    {
      qcom = quest->in;
      quest->in = qcom->next;
      free(qcom);
    }
    while (quest->out)
    {
      qcom = quest->out;
      quest->out = qcom->next;
      free(qcom);
    }
    free(quest);
    quest = next;
  }
}

/*
 * free a quest
 */
void free_hlquest(struct char_data *ch)
{
  if (!IS_NPC(ch))
    return;
  free_hlquests(ch->mob_specials.quest);
  ch->mob_specials.quest = 0;
}

/*
 *  this utility function will return vnum (if it applies)
 *  value for teach-spell and coins
 *  0 for attacking questor and disappearing
 */
int quest_value_vnum(struct quest_command *qcom)
{
  switch (qcom->type)
  {
  case QUEST_COMMAND_TEACH_SPELL:
  case QUEST_COMMAND_COINS:
    return qcom->value;
  case QUEST_COMMAND_ITEM:
  case QUEST_COMMAND_LOAD_OBJECT_INROOM:
    return GET_OBJ_VNUM(&obj_proto[qcom->value]);
    break;
  case QUEST_COMMAND_LOAD_MOB_INROOM:
    return GET_MOB_VNUM(&mob_proto[qcom->value]);
    break;
  case QUEST_COMMAND_ATTACK_QUESTOR:
  case QUEST_COMMAND_DISAPPEAR:
    return 0;
  }
  return 0;
}

/*
 *  this utility function returns room vnum if appropriate
 */
int quest_location_vnum(struct quest_command *qcom)
{
  if (qcom->location == -1)
    return -1;

  switch (qcom->type)
  {
  case QUEST_COMMAND_LOAD_OBJECT_INROOM:
  case QUEST_COMMAND_LOAD_MOB_INROOM:
    return GET_ROOM_VNUM(qcom->value);
  }
  return -1;
}

/*-----------------------------------*/
/* loading/saving functions */
/*-----------------------------------*/

/* loading the quests from disk */
void boot_the_quests(FILE *quest_f, char *filename, int rec_count)
{
  char str[256] = {'\0'};
  char line[256] = {'\0'};
  char inner[256] = {'\0'};
  int temp = 0;
  bool done = FALSE;
  bool approved = FALSE;
  struct char_data *mob = NULL;
  struct quest_command *qcom = NULL;
  struct quest_command *qlast = NULL;
  struct quest_entry *quest = NULL;
  char buf2[MAX_INPUT_LENGTH] = {'\0'};

  while (done == FALSE)
  {
    get_line(quest_f, line);
    approved = FALSE;
    if (strlen(line) > 1)
      approved = TRUE;

    switch (line[0])
    { /* New quest */
    case '#':
      sscanf(line, "#%d", &temp);
      mob = &mob_proto[real_mobile(temp)];
      break;
    case 'A':
      if (mob == 0)
      {
        log("ERROR: No mob defined in quest in %s", filename);
        return;
      }

      CREATE(quest, struct quest_entry, 1);
      clear_hlquest(quest);
      quest->type = QUEST_ASK;
      quest->approved = approved;
      quest->keywords = fread_string(quest_f, buf2);
      quest->reply_msg = fread_string(quest_f, buf2);
      quest->next = mob->mob_specials.quest;
      mob->mob_specials.quest = quest;
      break;
    case 'R':
      get_line(quest_f, inner);
      sscanf(inner, "%d", &temp);
    case 'Q':
      CREATE(quest, struct quest_entry, 1);
      clear_hlquest(quest);
      quest->room = temp;
      quest->type = line[0] == 'Q' ? QUEST_GIVE : QUEST_ROOM;
      quest->approved = approved;
      quest->keywords = 0;
      quest->reply_msg = fread_string(quest_f, buf2);
      quest->next = mob->mob_specials.quest;
      mob->mob_specials.quest = quest;
      do
      {
        get_line(quest_f, inner);
        CREATE(qcom, struct quest_command, 1);
        if (3 == sscanf(inner + 1, "%s%d%d", str, &qcom->value, &qcom->location))
        {
        }
        else
          qcom->location = -1;
        switch (str[0])
        {
        case 'C':
          qcom->type = QUEST_COMMAND_COINS;
          break;
        case 'I':
          qcom->type = QUEST_COMMAND_ITEM;
          break;
        case 'O':
          qcom->type = QUEST_COMMAND_LOAD_OBJECT_INROOM;
          break;
        case 'M':
          qcom->type = QUEST_COMMAND_LOAD_MOB_INROOM;
          break;
        case 'A':
          qcom->type = QUEST_COMMAND_ATTACK_QUESTOR;
          break;
        case 'D':
          qcom->type = QUEST_COMMAND_DISAPPEAR;
          break;
        case 'T':
          qcom->type = QUEST_COMMAND_TEACH_SPELL;
          break;
        case 'X':
          qcom->type = QUEST_COMMAND_OPEN_DOOR;
          break;
        case 'F':
          qcom->type = QUEST_COMMAND_FOLLOW;
          break;
        case 'U':
          qcom->type = QUEST_COMMAND_CHURCH;
          break;
        case 'K':
          qcom->type = QUEST_COMMAND_KIT;
          break;
        case 'S':
          qcom->type = QUEST_COMMAND_CAST_SPELL;
          break;
        }
        switch (inner[0])
        {
        case 'I':
          qcom->next = quest->in;
          quest->in = qcom;
          break;
        case 'O':
          if (quest->out == 0)
            quest->out = qcom;
          else
          {
            qlast = quest->out;
            while (qlast->next != 0)
              qlast = qlast->next;
            qlast->next = qcom;
          }
          break;
        }
      } while (*inner != 'S');
      break;
    case '$':
      done = TRUE;
      break;
    default:
      return;
      break;
    }
  }
}

/*-----------------------------------*/
/* hlquest commands ***/
/*-----------------------------------*/

/* quest info command
   CURRENTLY unfinished */
ACMD(do_qinfo)
{
  int i = 0, j = 0, start_num = 0, end_num = 0, number = 0, found = 0;
  int realnum = -1;
  struct quest_entry *quest = NULL;
  struct quest_command *qcmd = NULL;
  char arg[MAX_INPUT_LENGTH] = {'\0'};
  char buf[MAX_INPUT_LENGTH] = {'\0'};
  char buf2[MAX_INPUT_LENGTH] = {'\0'};

  one_argument(argument, arg, sizeof(arg));

  if (!*arg)
  {
    send_to_char(ch, "qinfo what object?\r\n");
    return;
  }
  if ((number = atoi(arg)) == NOTHING)
  {
    send_to_char(ch, "No such object.\r\n");
    return;
  }

  for (j = 0; j <= top_of_zone_table; j++)
  {
    start_num = zone_table[j].number * 100;
    end_num = zone_table[real_zone(start_num)].top;
    for (i = start_num; i <= end_num; i++)
    {
      if ((realnum = real_mobile(i)) != NOWHERE)
      {
        if (mob_proto[realnum].mob_specials.quest)
        {
          for (quest = mob_proto[realnum].mob_specials.quest; quest; quest =
                                                                         quest->next)
          {
            for (qcmd = quest->in; qcmd && !found; qcmd = qcmd->next)
            {
              if (qcmd->type == QUEST_COMMAND_ITEM && number == qcmd->value)
              {
                found = 1;
              }
            }
            for (qcmd = quest->out; qcmd && !found; qcmd = qcmd->next)
            {
              if (qcmd->type == QUEST_GIVE && number == qcmd->value)
              {
                found = 1;
              }
            }
            if (found)
            {
              snprintf(buf, sizeof(buf), "You");
              for (qcmd = quest->in; qcmd; qcmd = qcmd->next)
              {
                if (qcmd->type == QUEST_GIVE)
                {
                  snprintf(buf2, sizeof(buf2), " give %s (%d)",
                           obj_proto[real_object(qcmd->value)].short_description,
                           qcmd->value);
                  strlcat(buf, buf2, sizeof(buf));
                }
                else if (qcmd->type == QUEST_COMMAND_COINS)
                {
                  snprintf(buf2, sizeof(buf2), " give %d copper coins", qcmd->value);
                  strlcat(buf, buf2, sizeof(buf));
                }
                if (qcmd->next)
                {
                  strlcat(buf, " and", sizeof(buf));
                }
              }
              snprintf(buf2, sizeof(buf2), "\r\nTo %s (%d)\r\n", mob_proto[realnum].player.short_descr, i);
              strlcat(buf, buf2, sizeof(buf));
              for (qcmd = quest->out; qcmd; qcmd = qcmd->next)
              {
                if (qcmd->type == QUEST_GIVE)
                {
                  snprintf(buf2, sizeof(buf2), " and you receive %s (%d)",
                           obj_proto[real_object(qcmd->value)].short_description,
                           qcmd->value);
                  strlcat(buf, buf2, sizeof(buf));
                }
                else if (qcmd->type == QUEST_COMMAND_DISAPPEAR)
                {
                  strlcat(buf, " and the mob disappears", sizeof(buf));
                }
                else if (qcmd->type == QUEST_COMMAND_ATTACK_QUESTOR)
                {
                  strlcat(buf, " and the mob Attacks!", sizeof(buf));
                }
                else if (qcmd->type == QUEST_COMMAND_LOAD_OBJECT_INROOM)
                {
                  snprintf(buf2, sizeof(buf2), " and the mob loads %s in %s(%d)",
                           obj_proto[real_object(qcmd->value)].short_description,
                           (qcmd->location == -1 ? "CurrentRoom" : world[real_room(qcmd->location)].name), qcmd->location);
                  strlcat(buf, buf2, sizeof(buf));
                }
                else if (qcmd->type == QUEST_COMMAND_TEACH_SPELL)
                {
                  snprintf(buf2, sizeof(buf2), " and teaches you %s", spell_info[qcmd->value].name);
                  strlcat(buf, buf2, sizeof(buf));
                }
                else if (qcmd->type == QUEST_COMMAND_OPEN_DOOR)
                {
                  snprintf(buf2, sizeof(buf2), "and opens a door %s in %s(%d)", dirs[qcmd->value],
                           world[real_room(qcmd->location)].name, qcmd->location);
                  strlcat(buf, buf2, sizeof(buf));
                }
                else if (qcmd->type == QUEST_COMMAND_LOAD_MOB_INROOM)
                {
                  snprintf(buf2, sizeof(buf2), " and loads %s in %s (%d)",
                           mob_proto[real_mobile(qcmd->value)].player.short_descr,
                           qcmd->location == -1 ? "CurrentRoom" : world[real_room(qcmd->location)].name, qcmd->location);
                  strlcat(buf, buf2, sizeof(buf));
                }
                else if (qcmd->type == QUEST_COMMAND_FOLLOW)
                {
                  strlcat(buf, " and follows you", sizeof(buf));
                }
                else if (qcmd->type == QUEST_COMMAND_KIT)
                {
                  if (qcmd->value == LICH_QUEST || qcmd->location == LICH_QUEST)
                  {
                    snprintf(buf, sizeof(buf), "and changes your race to LICH");
                    strlcat(buf, buf2, sizeof(buf));
                  }
                  else
                  {
                    snprintf(buf, sizeof(buf), "and changes your kit to %s", CLSLIST_NAME(qcmd->value));
                    strlcat(buf, buf2, sizeof(buf));
                  }
                }
                else if (qcmd->type == QUEST_COMMAND_CHURCH)
                {
                  snprintf(buf2, sizeof(buf2), " and changes your religious affiliation to %s",
                           church_types[qcmd->value]);
                  strlcat(buf, buf2, sizeof(buf));
                }
                else
                {
                  strlcat(buf, " needs fixing (hlquest.c)", sizeof(buf));
                }
              } // end quest-> out loop

              strlcat(buf, ".\r\n\r\n", sizeof(buf));
              send_to_char(ch, buf);
              found = 0;
            } // end of if (found)
          }   // end quest loop
        }     // End if (mob has quest)
      }       // do we have a mob?
    }         // mobs in zone walk
  }           // zone table walk
}

/* command to check for any unapproved quests in given zone/mob vnum range */
ACMD(do_checkapproved)
{
  int i;
  int count;
  int total;
  struct quest_entry *quest;
  char buf[MAX_INPUT_LENGTH] = {'\0'};

  for (i = 0; i < top_of_mobt; i++)
  {
    if (mob_proto[i].mob_specials.quest)
    {
      count = 0;
      total = 0;
      for (quest = mob_proto[i].mob_specials.quest; quest; quest = quest->next)
      {
        if (quest->approved == FALSE)
          count++;
        total++;
      }
      if (count > 0)
      {
        snprintf(buf, sizeof(buf), "\tn[%5d] %-40s\tn  %d/%d\tn\r\n", mob_index[i].vnum, mob_proto[i].player.short_descr, total - count, total);
        send_to_char(ch, buf);
      }
    }
  }
}

/* displays all kitquests in the game */
ACMD(do_kitquests)
{
  char buf[MAX_INPUT_LENGTH] = {'\0'};

  if (GET_LEVEL(ch) < LVL_IMMORT)
  {
    snprintf(buf, sizeof(buf), "(GC) %s looked at kitquest list.", GET_NAME(ch));
    log(buf);
  }

  int i;
  struct quest_entry *quest;
  struct quest_command *qcom;

  for (i = 0; i < top_of_mobt; i++)
  {
    if (mob_proto[i].mob_specials.quest)
    {
      for (quest = mob_proto[i].mob_specials.quest; quest; quest = quest->next)
      {
        // check in.
        for (qcom = quest->out; qcom; qcom = qcom->next)
        {
          if (qcom->type == QUEST_COMMAND_KIT)
          {

            if (qcom->location == LICH_QUEST || qcom->value == LICH_QUEST)
            {
              snprintf(buf, sizeof(buf), "\tc%-32s\tn - %s(\tW%d\tn)\r\n", "LICH", mob_proto[i].player.short_descr, mob_index[i].vnum);
              send_to_char(ch, buf);
            }
            else
            {
              snprintf(buf, sizeof(buf), "\tc%-32s\tn - %s(\tW%d\tn)\r\n", CLSLIST_NAME(qcom->value), mob_proto[i].player.short_descr, mob_index[i].vnum);
              send_to_char(ch, buf);
            }
          }
        }
      }
    }
  }
}

/* displays all spell quests in the game */
ACMD(do_spellquests)
{
  char buf[MAX_INPUT_LENGTH] = {'\0'};
  int i;
  struct quest_entry *quest;
  struct quest_command *qcom;

  /********** under construction ***************/
  send_to_char(ch, "Currently not implemented, please tune in later!\r\n");
  return;
  /********** under construction ***************/

  if (GET_LEVEL(ch) < LVL_IMMORT)
  {
    snprintf(buf, sizeof(buf), "(GC) %s looked at spellquest list.", GET_NAME(ch));
    log(buf);
  }

  send_to_char(ch, "\tcSpells requiring quests:\tn\r\n\tc-------------------\tn\r\n");
  for (i = 0; i < MAX_SPELLS; i++)
  {
    if (spell_info[i].quest)
    {
      snprintf(buf, sizeof(buf), "\tc%-32s\tn  %s\r\n", spell_info[i].name, (has_spell_a_quest(i) ? "(\tCQuest\tn)" : ""));
      send_to_char(ch, buf);
    }
  }

  send_to_char(ch, "\r\n\tCCurrent quests:\tn\r\n\tc-------------------\tn\r\n");
  for (i = 0; i < top_of_mobt; i++)
  {
    if (mob_proto[i].mob_specials.quest)
    {
      for (quest = mob_proto[i].mob_specials.quest; quest; quest = quest->next)
      {
        // check in.
        for (qcom = quest->out; qcom; qcom = qcom->next)
        {
          if (qcom->type == QUEST_COMMAND_TEACH_SPELL)
          {
            snprintf(buf, sizeof(buf), "\tc%-32s\tn - %s(\tW%d\tn)\r\n", spell_info[qcom->value].name, mob_proto[i].player.short_descr, mob_index[i].vnum);
            send_to_char(ch, buf);
          }
        }
      }
    }
  }
}

/* with a given object vnum, finds references to quests in game */
ACMD(do_qref)
{
  int i;
  int count = 0;
  int vnum = 0;
  int real_num = 0;
  struct quest_entry *quest;
  struct quest_command *qcom;
  char buf[MAX_INPUT_LENGTH] = {'\0'};

  one_argument(argument, buf, sizeof(buf));

  if (!*buf)
  {
    send_to_char(ch, "qref what object?\r\n");
    return;
  }

  vnum = atoi(buf);
  real_num = real_object(vnum);

  if (real_num == NOWHERE)
  {
    send_to_char(ch, "\tRNo such object!\tn\r\n");
    return;
  }

  if (GET_LEVEL(ch) < LVL_IMMORT)
  {
    snprintf(buf, sizeof(buf), "(GC) %s did a reference check for (%d).", GET_NAME(ch), vnum);
    log(buf);
  }

  for (i = 0; i < top_of_mobt; i++)
  {
    if (mob_proto[i].mob_specials.quest)
    {
      for (quest = mob_proto[i].mob_specials.quest; quest; quest = quest->next)
      {
        // check in.
        for (qcom = quest->in; qcom; qcom = qcom->next)
        {
          if (qcom->value == vnum && qcom->type == QUEST_COMMAND_ITEM)
          {
            snprintf(buf, sizeof(buf), "\tCGIVE\tn %s to %s(\tW%d\tn)\r\n", obj_proto[real_num].short_description, mob_proto[i].player.short_descr, mob_index[i].vnum);
            send_to_char(ch, buf);
            count++;
          }
        }

        // check out.
        for (qcom = quest->out; qcom; qcom = qcom->next)
        {
          if (qcom->value == vnum)
          {
            switch (qcom->type)
            {
            case QUEST_COMMAND_ITEM:
              snprintf(buf, sizeof(buf), "\tCRECEIVE\tn %s from %s(\tW%d\tn)\r\n", obj_proto[real_num].short_description, mob_proto[i].player.short_descr, mob_index[i].vnum);
              send_to_char(ch, buf);
              count++;
              break;
            case QUEST_COMMAND_LOAD_OBJECT_INROOM:
              snprintf(buf, sizeof(buf), "\tcLOADOBJECT\tn %s in quest for %s (\tW%d\tn)\r\n",
                       obj_proto[real_num].short_description, mob_proto[i].player.short_descr, mob_index[i].vnum);
              send_to_char(ch, buf);
              count++;
              break;
            }
          }
        }
      }
    }
  }
  if (count == 0)
  {
    send_to_char(ch, "\tRThat object is not used in any quests!\tn\r\n");
    return;
  }
}

/* qview will view the quest details of a given mob's vnum in a nice format */
ACMD(do_qview)
{
  struct quest_entry *quest;
  int num;
  char buf[MAX_INPUT_LENGTH] = {'\0'};

  one_argument(argument, buf, sizeof(buf));

  if (!*buf)
  {
    send_to_char(ch, "Qview what mob?\r\n");
    return;
  }

  num = real_mobile(atoi(buf));
  if (num == NOWHERE)
  {
    send_to_char(ch, "\tRNo such mobile!\tn\r\n");
    return;
  }

  if (mob_proto[num].mob_specials.quest == 0)
  {
    send_to_char(ch, "\tRThat mob has no quests.\tn\r\n");
    return;
  }

  if (GET_LEVEL(ch) < LVL_IMPL)
  {
    snprintf(buf, sizeof(buf), "(GC) %s has peeked at quest for (%d).", GET_NAME(ch), atoi(buf));
    log(buf);
  }

  for (quest = mob_proto[num].mob_specials.quest; quest; quest = quest->next)
  {
    show_quest_to_player(ch, quest);
    if (quest->next)
      send_to_char(ch, "\r\n\tW-------------------------------------\tn"
                       "\r\n\r\n");
  }
}

#undef LICH_QUEST

/*-----------------------------------*/
/* end hlquest commands */
/*-----------------------------------*/
