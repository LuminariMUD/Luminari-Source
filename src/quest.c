/* ***********************************************************************
 *    File:   quest.c                                Part of LuminariMUD  *
 * Version:   2.1 (December 2005) Written for CircleMud CWG / Suntzu      *
 * Purpose:   To provide special quest-related code.                      *
 * Copyright: Kenneth Ray                                                 *
 * Original Version Details:                                              *
 * Morgaelin - quest.c                                                    *
 * Copyright (C) 1997 MS                                                  *
 *********************************************************************** */
#define __QUEST_C__

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "comm.h"
#include "screen.h"
#include "quest.h"
#include "act.h" /* for do_tell */
#include "mudlim.h"
#include "mud_event.h"
#include "missions.h"
#include "house.h"
#include "dg_scripts.h" /* for load_mtrigger() */

/*-------------------------------------------------------------------*/
/* External data */

extern struct house_control_rec house_control[MAX_HOUSES]; /* house.c */
/*-------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* external function protos */

house_rnum find_house(room_vnum vnum); /* house.c */
/*-------------------------------------------------------------------*/

/*--------------------------------------------------------------------------
 * Exported global variables
 *--------------------------------------------------------------------------*/

const char *quest_types[NUM_AQ_TYPES + 1] = {
    "Acquire Object", /* 0 */
    "Find Room",
    "Find Mob",
    "Kill Mob",
    "Save Mob",
    "Return Object", /* 5 */
    "Clear Room",
    "Complete a Supplyorder",
    "Craft Item",
    "ReSize Item",
    "Divide Item", /* 10 */
    "Mine Crafting Mat",
    "Hunt for Crafting Mat",
    "Knit Crafting Mat",
    "Forest for Crafting Mat",
    "Disenchant Item", /* 15 */
    "Augment Item",
    "Convert Item",
    "ReString Item",
    "Complete a Mission",
    "Find a Player House",           /* 20 */
    "Get to Wilderness Coordinates", /* 21 */
    "Give Gold",                     /* 22 */
    "Kill Multi Mob (comma-separated vnums)",
    "Dialogue Quest",
    "\n"};

const char *aq_flags[] = {
    "REPEATABLE",
    "REPLACE-OBJ-REWARD",
    "\n"};

/*--------------------------------------------------------------------------
 * Local (file scope) global variables
 *--------------------------------------------------------------------------*/

static int cmd_tell;

static const char *quest_cmd[] = {
    "list",
    "history",
    "join",
    "leave",
    "progress",
    "status",
    "view",
    "assign",
    "\n"};

static const char *quest_mort_usage =
    "Usage: quest  list | history <optional nn> | progress <optional nn> | join <nn> | leave <nn>";

static const char *quest_imm_usage =
    "Usage: quest  list | history <optional nn> | progress <optional nn> | join <nn> | leave <nn> | status <vnum> | assign <target> <vnum>";

/*--------------------------------------------------------------------------*/
/* Utility Functions                                                        */
/*--------------------------------------------------------------------------*/

/* given a quest virtual number, return its real-number */
qst_rnum real_quest(qst_vnum vnum)
{
  int rnum;

  for (rnum = 0; rnum < total_quests; rnum++)
    if (QST_NUM(rnum) == vnum)
      return (rnum);
  return (NOTHING);
}

/* check if given player with given quest virtual-number, has completed it */
int is_complete(struct char_data *ch, qst_vnum vnum)
{
  int i;

  for (i = 0; i < GET_NUM_QUESTS(ch); i++)
    if (ch->player_specials->saved.completed_quests[i] == vnum)
      return TRUE;
  return FALSE;
}

/* given ch, quest-master, quest-number, return quest virtual-number */
qst_vnum find_quest_by_qmnum(struct char_data *ch, mob_vnum qm, int num)
{
  qst_rnum rnum;
  int found = 0;

  for (rnum = 0; rnum < total_quests; rnum++)
  {
    if (qm == QST_MASTER(rnum))
      if (++found == num)
        return (QST_NUM(rnum));
  }
  return NOTHING;
}

/*--------------------------------------------------------------------------*/
/* Quest Loading and Unloading Functions                                    */
/*--------------------------------------------------------------------------*/

/* completely wipe/free the aquest table */
void destroy_quests(void)
{
  qst_rnum rnum = 0;

  if (!aquest_table)
    return;

  for (rnum = 0; rnum < total_quests; rnum++)
  {
    free_quest_strings(&aquest_table[rnum]);
  }
  free(aquest_table);
  aquest_table = NULL;
  total_quests = 0;

  return;
}

/* count how many quests between two vnums */
int count_quests(qst_vnum low, qst_vnum high)
{
  int i, j;

  if (!aquest_table)
    return 0;

  for (i = j = 0; i < total_quests; i++)
    if (QST_NUM(i) >= low && QST_NUM(i) <= high)
      j++;

  return j;
}

/* read quest from file and load it into memory */
/* Returns 1 on success, 0 on error (but continues loading) */
int parse_quest(FILE *quest_f, int nr)
{
  static char line[MEDIUM_STRING] = {'\0'};
  static int i = 0, j;
  int retval = 0, t[7];
  char f1[128], buf2[MAX_STRING_LENGTH] = {'\0'};

  /* init some vars */
  aquest_table[i].vnum = nr;
  aquest_table[i].qm = NOBODY;
  aquest_table[i].name = NULL;
  aquest_table[i].desc = NULL;
  aquest_table[i].info = NULL;
  aquest_table[i].done = NULL;
  aquest_table[i].quit = NULL;
  aquest_table[i].kill_list = NULL;
  aquest_table[i].flags = 0;
  aquest_table[i].type = -1;
  aquest_table[i].target = -1;
  aquest_table[i].prereq = NOTHING;
  for (j = 0; j < 7; j++)
    aquest_table[i].value[j] = 0;
  aquest_table[i].prev_quest = NOTHING;
  aquest_table[i].next_quest = NOTHING;
  aquest_table[i].func = NULL;

  aquest_table[i].gold_reward = 0;
  aquest_table[i].exp_reward = 0;
  aquest_table[i].obj_reward = NOTHING;
  aquest_table[i].race_reward = RACE_UNDEFINED;
  aquest_table[i].follower_reward = NOBODY;

  aquest_table[i].coord_x = -1;
  aquest_table[i].coord_y = -1;

  aquest_table[i].diplomacy_dc = -1;
  aquest_table[i].intimidate_dc = -1;
  aquest_table[i].bluff_dc = -1;
  aquest_table[i].dialogue_alternative_quest = NOTHING;

  /* end init */

  /* begin to parse the data */
  aquest_table[i].name = fread_string(quest_f, buf2);
  aquest_table[i].desc = fread_string(quest_f, buf2);
  aquest_table[i].info = fread_string(quest_f, buf2);
  aquest_table[i].done = fread_string(quest_f, buf2);
  aquest_table[i].quit = fread_string(quest_f, buf2);
#if defined(CAMPAIGN_DL)  
  aquest_table[i].kill_list = fread_string(quest_f, buf2);
#endif

  /* parse the first line of ints */
  if (!get_line(quest_f, line) ||
      (retval = sscanf(line, " %d %d %s %d %d %d %d",
                       t, t + 1, f1, t + 2, t + 3, t + 4, t + 5)) != 7)
  {
    log("SYSERR: Quest #%d: Format error in numeric line 1 (expected 7, got %d), line: '%s'",
        nr, retval, line);
    log("SYSERR: Skipping malformed quest #%d", nr);
    /* Skip to end of quest entry */
    while (get_line(quest_f, line)) {
      if (!strcmp(line, "S"))
        break;
    }
    return 0;  /* Return error but don't exit */
  }
  aquest_table[i].type = t[0];
  aquest_table[i].qm = (real_mobile(t[1]) == NOBODY) ? NOBODY : t[1];
  aquest_table[i].flags = asciiflag_conv(f1);
  aquest_table[i].target = (t[2] == -1) ? NOTHING : t[2];
  aquest_table[i].prev_quest = (t[3] == -1) ? NOTHING : t[3];
  aquest_table[i].next_quest = (t[4] == -1) ? NOTHING : t[4];
  aquest_table[i].prereq = (t[5] == -1) ? NOTHING : t[5];

  /* parse the second line of ints */
  if (!get_line(quest_f, line) ||
      (retval = sscanf(line, " %d %d %d %d %d %d %d",
                       t, t + 1, t + 2, t + 3, t + 4, t + 5, t + 6)) != 7)
  {
    log("SYSERR: Quest #%d: Format error in numeric line 2 (expected 7, got %d), line: '%s'",
        nr, retval, line);
    log("SYSERR: Skipping malformed quest #%d", nr);
    /* Skip to end of quest entry */
    while (get_line(quest_f, line)) {
      if (!strcmp(line, "S"))
        break;
    }
    return 0;  /* Return error but don't exit */
  }
  for (j = 0; j < 7; j++)
    aquest_table[i].value[j] = t[j];

  /* parse the third line of ints
       re-wrote this to handle the old 3 values or new 7 values -zusuk */
  if (!get_line(quest_f, line) ||
      (retval = sscanf(line, " %d %d %d %d %d %d %d",
                       t, t + 1, t + 2, t + 3, t + 4, t + 5, t + 6)))
  {
    if (retval != 3 && retval != 7)
    {
      log("SYSERR: Quest #%d: Format error in numeric line 3 (expected 3 or 7, got %d), line: '%s'",
          nr, retval, line);
      log("SYSERR: Skipping malformed quest #%d", nr);
      /* Skip to end of quest entry */
      while (get_line(quest_f, line)) {
        if (!strcmp(line, "S"))
          break;
      }
      return 0;  /* Return error but don't exit */
    }
  }

  aquest_table[i].gold_reward = t[0];
  aquest_table[i].exp_reward = t[1];
  aquest_table[i].obj_reward = (t[2] == -1) ? NOTHING : t[2];

  if (retval == 7)
  {
    aquest_table[i].race_reward = t[3];

    aquest_table[i].coord_x = t[4];
    aquest_table[i].coord_y = t[5];

    aquest_table[i].follower_reward = t[6];
  }

  /* finish 'er up! */
  for (;;)
  {
    if (!get_line(quest_f, line))
    {
      log("Format error in %s\n", line);
      exit(1);
    }

    switch (*line)
    {
    case 'S':
      total_quests = ++i;
      return;
      break;
    case 'D':
      if (!get_line(quest_f, line))
      {
        log("SYSERR: Format error in 'D' field, %s\n"
            "...expecting numeric constant but file ended!", buf2);
        exit(1);
      }
      if (sscanf(line, "%d %d %d %d", t, t + 1, t + 2, t + 3) != 4)
      {
        log("SYSERR: Format error in 'D' field, %s\n"
            "...expecting numeric argument\n"
            "...offending line: '%s'", buf2, line);
        exit(1);
      }
      aquest_table[i].diplomacy_dc = t[0];
      aquest_table[i].intimidate_dc = t[1];
      aquest_table[i].bluff_dc = t[2];
      aquest_table[i].dialogue_alternative_quest = t[3];
      break;
    }
  }
  return 1;  /* Success */
} /* end parse_quest */

/* assign the quests to their questmasters */
void assign_the_quests(void)
{
  qst_rnum rnum;
  mob_rnum mrnum;

  cmd_tell = find_command("tell");

  for (rnum = 0; rnum < total_quests; rnum++)
  {
    if (QST_MASTER(rnum) == NOBODY ||
        QST_MASTER(rnum) <= 0)
    {
      log("SYSERR: Quest #%d has no questmaster specified.", QST_NUM(rnum));
      continue;
    }
    if ((mrnum = real_mobile(QST_MASTER(rnum))) == NOBODY)
    {
      log("SYSERR: Quest #%d has an invalid questmaster.", QST_NUM(rnum));
      continue;
    }
    if (mrnum <= 0)
    {
      log("SYSERR: Quest #%d has an invalid questmaster [2nd check].", QST_NUM(rnum));
      continue;
    }
    if (mob_index[(mrnum)].func &&
        mob_index[(mrnum)].func != questmaster)
      QST_FUNC(rnum) = mob_index[(mrnum)].func;
    mob_index[(mrnum)].func = questmaster;
  }
}

/*--------------------------------------------------------------------------*/
/* Quest Completion Functions                                               */
/*--------------------------------------------------------------------------*/

/* assign a quest to given ch */
void set_quest(struct char_data *ch, qst_rnum rnum, int index)
{
  GET_QUEST(ch, index) = QST_NUM(rnum);
  GET_QUEST_TIME(ch, index) = QST_TIME(rnum);
  GET_QUEST_COUNTER(ch, index) = QST_QUANTITY(rnum);
  // SET_BIT_AR(PRF_FLAGS(ch), PRF_QUEST);
  return;
}

/* clear a ch of his [index] quest */
void clear_quest(struct char_data *ch, int index)
{
  GET_QUEST(ch, index) = NOTHING;
  GET_QUEST_TIME(ch, index) = -1;
  GET_QUEST_COUNTER(ch, index) = 0;
  // REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_QUEST);
  return;
}

/* add a completed quest to the saved quest list of the ch (history) */
void add_completed_quest(struct char_data *ch, qst_vnum vnum)
{
  qst_vnum *temp;
  int i;

  CREATE(temp, qst_vnum, GET_NUM_QUESTS(ch) + 1);
  for (i = 0; i < GET_NUM_QUESTS(ch); i++)
    temp[i] = ch->player_specials->saved.completed_quests[i];

  temp[GET_NUM_QUESTS(ch)] = vnum;
  GET_NUM_QUESTS(ch)++;

  if (ch->player_specials->saved.completed_quests)
    free(ch->player_specials->saved.completed_quests);
  ch->player_specials->saved.completed_quests = temp;
}

/* */
void remove_completed_quest(struct char_data *ch, qst_vnum vnum)
{
  qst_vnum *temp;
  int i, j = 0;

  CREATE(temp, qst_vnum, GET_NUM_QUESTS(ch));
  for (i = 0; i < GET_NUM_QUESTS(ch); i++)
    if (ch->player_specials->saved.completed_quests[i] != vnum)
      temp[j++] = ch->player_specials->saved.completed_quests[i];

  GET_NUM_QUESTS(ch)--;

  if (ch->player_specials->saved.completed_quests)
    free(ch->player_specials->saved.completed_quests);
  ch->player_specials->saved.completed_quests = temp;
}

/* called when a quest is completed! */
void complete_quest(struct char_data *ch, int index)
{
  qst_rnum rnum = -1;
  qst_vnum vnum = GET_QUEST(ch, index);
  struct obj_data *new_obj = NULL;
  int happy_qp = 0, happy_gold = 0, happy_exp = 0;
  struct descriptor_data *pt = NULL;
  struct char_data *mob = NULL;

  /* dummy check */
  if (GET_QUEST(ch, index) == NOTHING)
  {
    log("UH OH: complete_quest() called without a quest VNUM!");
    return;
  }

  rnum = real_quest(vnum);

  /* we should NOT be getting this */
  if (GET_QUEST_COUNTER(ch, index) > 0 && rnum != NOWHERE && rnum != NOTHING)
  {
    send_to_char(ch, "You still have to achieve \tm%d\tn out of \tM%d\tn goals for the quest.\r\n\r\n",
                 --GET_QUEST_COUNTER(ch, index), QST_QUANTITY(rnum));
    save_char(ch, 0);
    log("UH OH: complete_quest() quest-counter is greater than zero!");
    return;
  }

  if (rnum == NOTHING)
  {
    send_to_char(ch, "Please 'bug submit': complete_quest() RNum is NOTHING\r\n");
    log("UH OH: complete_quest() rnum is NOTHING!");
    return;
  }

  /* does this race reward convert you into another race?  we have some important requirements! */
  if (QST_RACE(rnum) < NUM_EXTENDED_RACES && QST_RACE(rnum) > RACE_UNDEFINED)
  {
    if (GET_LEVEL(ch) < 30)
    {
      send_to_char(ch, "You must be level 30 to change races.\r\n");
      return;
    }

    /* these parameters break the game */
    if (GROUP(ch) || ch->master || ch->followers)
    {
      send_to_char(ch, "You cannot be part of a group, be following someone, or have followers of your own to change races.\r\n"
                       "You can dismiss npc followers with the 'dismiss' command.  You can leave your group with 'group leave.'\r\n");
      return;
    }
  }

  /* is the race reward follower (if there is one) valid?  the full processing is below
       this is "pre-flight" */
  if (QST_FOLLOWER(rnum) != NOBODY)
  {
    if (!(mob = read_mobile(QST_FOLLOWER(rnum), VIRTUAL)))
    {
      send_to_char(ch, "Report to staff:  quest follower invalid.\r\n");
      return;
    }
  }

  /* Quest complete! */

  /* any quest point reward for this quest? */
  if (IS_HAPPYHOUR && IS_HAPPYQP)
  {
    happy_qp = (int)(QST_POINTS(rnum) * (((float)(100 + HAPPY_QP)) / (float)100));
    happy_qp = MAX(happy_qp, 0);
    GET_QUESTPOINTS(ch) += happy_qp;
    send_to_char(ch,
                 "%s\r\nYou have been awarded %d \tCquest points\tn for your service.\r\n\r\n",
                 QST_DONE(rnum), happy_qp);
  }
  else
  { /* no happy hour bonus :( */
    GET_QUESTPOINTS(ch) += QST_POINTS(rnum);
    send_to_char(ch,
                 "%s\r\nYou have been awarded %d \tCquest points\tn for your service.\r\n\r\n",
                 QST_DONE(rnum), QST_POINTS(rnum));
  }

  /* any gold reward in this quest? */
  if (QST_GOLD(rnum))
  {
    if ((IS_HAPPYHOUR) && (IS_HAPPYGOLD))
    {
      happy_gold = (int)(QST_GOLD(rnum) * (((float)(100 + HAPPY_GOLD)) / (float)100));
      happy_gold = MAX(happy_gold, 0);
      increase_gold(ch, happy_gold);
      send_to_char(ch,
                   "You have been awarded %d \tYgold coins\tn for your service.\r\n\r\n",
                   happy_gold);
    }
    else
    {
      increase_gold(ch, QST_GOLD(rnum));
      send_to_char(ch,
                   "You have been awarded %d \tYgold coins\tn for your service.\r\n\r\n",
                   QST_GOLD(rnum));
    }
  }

  /* any xp points reward in this quest? */
  if (QST_EXP(rnum))
  {
    if ((IS_HAPPYHOUR) && (IS_HAPPYEXP))
    {
      happy_exp = (int)(QST_EXP(rnum) * (((float)(100 + HAPPY_EXP)) / (float)100));
      happy_exp = MAX(happy_exp, 0);
      send_to_char(ch,
                   "You have been awarded %d \tBexperience\tn for your service.\r\n\r\n",
                   happy_exp);
      gain_exp(ch, happy_exp, GAIN_EXP_MODE_QUEST);
    }
    else
    {
      send_to_char(ch,
                   "You have been awarded %d \tBexperience\tn points for your service.\r\n\r\n",
                   QST_EXP(rnum));
      gain_exp(ch, QST_EXP(rnum), GAIN_EXP_MODE_QUEST);
    }
  }

  /* any object reward from this quest? */
  if (QST_OBJ(rnum) && QST_OBJ(rnum) != NOTHING)
  {
    if (real_object(QST_OBJ(rnum)) != NOTHING)
    {
      if ((new_obj = read_object((QST_OBJ(rnum)), VIRTUAL)) != NULL)
      {
        obj_to_char(new_obj, ch);
        send_to_char(ch, "You have been presented with %s%s for your service.\r\n\r\n",
                     GET_OBJ_SHORT(new_obj), CCNRM(ch, C_NRM));
      }
    }
  }

  /* does this race reward convert you into another race? */
  if (QST_RACE(rnum) < NUM_EXTENDED_RACES && QST_RACE(rnum) > RACE_UNDEFINED)
  {
    switch (QST_RACE(rnum))
    {

    case RACE_VAMPIRE:

      GET_REAL_RACE(ch) = RACE_VAMPIRE;
      /* Zhentil Keep - hometwon system not implemented yet */
      // GET_HOMETOWN(ch) = 3;

      respec_engine(ch, CLASS_WARRIOR, NULL, TRUE);
      GET_EXP(ch) = 0;
      GET_ALIGNMENT(ch) = -1000;

      /* Messages */
      for (pt = descriptor_list; pt; pt = pt->next)
      {
        if (IS_PLAYING(pt) && pt->character && pt->character != ch)
        {
          send_to_char(pt->character, "\tL%s's \tWlifeforce\tL is ripped apart, who realizes\tn\r\n"
                                      "\tLthat death has arrived. %s's \tLbody is now merely a "
                                      "vessel for power.  %s \tLhas become a \tYVAMPIRE\tn\r\n",
                       GET_NAME(ch), GET_NAME(ch), GET_NAME(ch));
        }
      }

      send_to_char(ch, "\tLYour \tWlifeforce\tL is ripped apart of you,"
                       " and you realize\tn\r\n"
                       "\tLthat you are dieing. Your body is now merely a "
                       "vessel for your power.\tn\r\n");

      send_to_char(ch, "You are now a \tLVAMPIRE!\tn\r\n");
      log("Quest Log : %s has changed into to a VAMPIRE!", GET_NAME(ch));

      break;

    case RACE_LICH:

      GET_REAL_RACE(ch) = RACE_LICH;
      /* Zhentil Keep - hometwon system not implemented yet */
      // GET_HOMETOWN(ch) = 3;

      respec_engine(ch, CLASS_WIZARD, NULL, TRUE);
      GET_EXP(ch) = 0;
      GET_ALIGNMENT(ch) = -1000;

      /* Messages */
      for (pt = descriptor_list; pt; pt = pt->next)
      {
        if (IS_PLAYING(pt) && pt->character && pt->character != ch)
        {
          send_to_char(pt->character, "\tL%s's \tWlifeforce\tL is ripped apart, who realizes\tn\r\n"
                                      "\tLthat death has arrived. %s's \tLbody is now merely a "
                                      "vessel for power.  %s \tLhas become a \tYLICH\tn\r\n",
                       GET_NAME(ch), GET_NAME(ch), GET_NAME(ch));
        }
      }

      send_to_char(ch, "\tLYour \tWlifeforce\tL is ripped apart of you,"
                       " and you realize\tn\r\n"
                       "\tLthat you are dieing. Your body is now merely a "
                       "vessel for your power.\tn\r\n");

      send_to_char(ch, "You are now a \tLLICH!\tn\r\n");
      log("Quest Log : %s has changed into to a LICH!", GET_NAME(ch));

      break;

    default:
      log("Quest Log : %s reached default in race reward for quest!", GET_NAME(ch));
      break;
    }
  }

  /* is there a follower reward for this quest?  "pre-flight" was checked above */
  if (QST_FOLLOWER(rnum) != NOBODY)
  {
    if (!mob)
    {
      send_to_char(ch, "Report to staff:  quest follower invalid (2).\r\n");
      return;
    }

    /* should be good, do all the work! */
    if (ZONE_FLAGGED(GET_ROOM_ZONE(IN_ROOM(ch)), ZONE_WILDERNESS))
    {
      X_LOC(mob) = world[IN_ROOM(ch)].coords[0];
      Y_LOC(mob) = world[IN_ROOM(ch)].coords[1];
    }
    char_to_room(mob, IN_ROOM(ch));
    IS_CARRYING_W(mob) = 0;
    IS_CARRYING_N(mob) = 0;
    SET_BIT_AR(AFF_FLAGS(mob), AFF_CHARM);

    act("$N approaches you and quickly falls into line.", FALSE, ch, 0, mob, TO_CHAR);
    act("You approach $n and quickly fall into line.", FALSE, ch, 0, mob, TO_VICT);
    act("$N approaches $n and quickly falls into line.", FALSE, ch, 0, mob, TO_ROOM);

    load_mtrigger(mob);
    add_follower(mob, ch);
    if (!GROUP(mob) && GROUP(ch) && GROUP_LEADER(GROUP(ch)) == ch)
      join_group(mob, GROUP(ch));
  }

  /* end rewards */

  /* handle throwing quest in history and repeatable quests */
  if ((!IS_SET(QST_FLAGS(rnum), AQ_REPEATABLE)) ||
      (IS_SET(QST_FLAGS(rnum), AQ_REPEATABLE) &&
       !is_complete(ch, GET_QUEST(ch, index))))
    add_completed_quest(ch, vnum);

  /* clear the quest data from ch, clean slate */
  clear_quest(ch, index);

  /* does this quest have a next step built in? */
  if ((real_quest(QST_NEXT(rnum)) != NOTHING) &&
      (QST_NEXT(rnum) != vnum) &&
      !is_complete(ch, QST_NEXT(rnum)))
  {
    rnum = real_quest(QST_NEXT(rnum));
    /* we will just use the slot we completed to insert this quest */
    set_quest(ch, rnum, index);
    send_to_char(ch,
                 "\tW***The next stage of your quest awaits:\tn\r\n\r\n%s\r\n",
                 QST_INFO(rnum));
  }
}

/* this function is called upon completion of a quest
 * or completion of a quest-step
 * NOTE: We added the actual completion to an event that
 * will call: void complete_quest() above */
void generic_complete_quest(struct char_data *ch, int index)
{

  if (has_duplicate_quest(ch))
    remove_duplicate_quests(ch);

  /* more work to do on this quest! make sure to decrement counter  */
  if (GET_QUEST(ch, index) != NOTHING && --GET_QUEST_COUNTER(ch, index) > 0)
  {
    qst_rnum rnum = -1;
    qst_vnum vnum = GET_QUEST(ch, index);

    rnum = real_quest(vnum);

    send_to_char(ch, "You still have to achieve \tm%d\tn out of \tM%d\tn goals for the quest.\r\n\r\n",
                 GET_QUEST_COUNTER(ch, index), QST_QUANTITY(rnum));
    save_char(ch, 0);

    /* the quest is truly complete? */
  }
  else if (GET_QUEST(ch, index) != NOTHING)
  {
    struct mud_event_data *pMudEvent = NULL;
    char buf[128] = {'\0'};
    qst_vnum event_quest_num = NOTHING;

    if ((pMudEvent = char_has_mud_event(ch, eQUEST_COMPLETE)))
    {
      /* grab vnum of quest that is in event */
      event_quest_num = atoi((char *)pMudEvent->sVariables);

      /* make sure we do not already have an event for this quest! */
      if (event_quest_num == GET_QUEST(ch, index))
      {
        /* get out of here, we are already processing this particular
           quest completion */
        return;
      }
    }

    /* we should be in the clear to tag this player with a completed quest */
    snprintf(buf, sizeof(buf), "%d", GET_QUEST(ch, index)); /* sending vnum to event of quest */
    attach_mud_event(new_mud_event(eQUEST_COMPLETE, ch, buf), 1);
  }
}

void autoquest_trigger_check(struct char_data *ch, struct char_data *vict,
                             struct obj_data *object, int variable, int type)
{
  struct char_data *i = NULL;
  struct obj_data *obj = NULL;
  qst_rnum rnum = NOTHING;
  int found = TRUE, index = -1;
  house_rnum house_num = NOWHERE;

  if (IS_NPC(ch))
    return;

  for (index = 0; index < MAX_CURRENT_QUESTS; index++)
  {

    if (GET_QUEST(ch, index) == NOTHING) /* No current quest, skip this */
      continue;

    if (GET_QUEST_TYPE(ch, index) != type)
      continue;

    /* grab the rnum */
    if ((rnum = real_quest(GET_QUEST(ch, index))) == NOTHING)
      continue;

    switch (type)
    {

    case AQ_CRAFT_RESIZE:
    case AQ_CRAFT_DIVIDE:
    case AQ_CRAFT_MINE:
    case AQ_CRAFT_HUNT:
    case AQ_CRAFT_KNIT:
    case AQ_CRAFT_FOREST:
    case AQ_CRAFT_DISENCHANT:
    case AQ_CRAFT_AUGMENT:
    case AQ_CRAFT_CONVERT:
    case AQ_CRAFT_RESTRING:
    case AQ_AUTOCRAFT:
    case AQ_CRAFT:
      /* nothing to process */
      generic_complete_quest(ch, index);
      break;

    case AQ_COMPLETE_MISSION:

      /* variable here is the mission difficulty completed */
      if (variable >= QST_TARGET(rnum))
        generic_complete_quest(ch, index);

      break;

    case AQ_OBJ_FIND:
      if (QST_TARGET(rnum) == GET_OBJ_VNUM(object))
        generic_complete_quest(ch, index);
      break;

    case AQ_HOUSE_FIND:
      house_num = find_house(GET_ROOM_VNUM(IN_ROOM(ch)));

      /* debug */ /*send_to_char(ch, "DEBUG - House number: %d\r\n", house_num);*/

      if (house_num == NOWHERE)
      {
        /* debug */ /*send_to_char(ch, "DEBUG - House number is NOWHERE!\r\n");*/
        break;
      }

      if (house_control[house_num].mode != HOUSE_PRIVATE)
      {
        /* debug */ /*send_to_char(ch, "DEBUG - House number is not private! (%d)\r\n", house_control[house_num].mode);*/
        break;
      }

      /* debug */ /*send_to_char(ch, "DEBUG - Your IDNUM (%ld), House (%ld)\r\n", GET_IDNUM(ch), house_control[house_num].owner);*/
      if (GET_IDNUM(ch) == house_control[house_num].owner)
        generic_complete_quest(ch, index);

      break;

    case AQ_WILD_FIND:

      /* are we in a wilderness room? */
      if (ZONE_FLAGGED(GET_ROOM_ZONE(IN_ROOM(ch)), ZONE_WILDERNESS))
      {
        if (X_LOC(ch) == QST_COORD_X(rnum) &&
            Y_LOC(ch) == QST_COORD_Y(rnum))
        {
          /* made it! */
          generic_complete_quest(ch, index);
        }
      }

      break;

    case AQ_ROOM_FIND:
      if (QST_TARGET(rnum) == world[IN_ROOM(ch)].number)
        generic_complete_quest(ch, index);
      break;

    case AQ_MOB_FIND:
      for (i = world[IN_ROOM(ch)].people; i; i = i->next_in_room)
        if (IS_NPC(i))
          if (QST_TARGET(rnum) == GET_MOB_VNUM(i))
            generic_complete_quest(ch, index);
      break;

    case AQ_MOB_KILL:
      if (!IS_NPC(ch) && IS_NPC(vict) && (ch != vict))
        if (QST_TARGET(rnum) == GET_MOB_VNUM(vict))
          generic_complete_quest(ch, index);
      break;

    case AQ_MOB_MULTI_KILL:
      if (!IS_NPC(ch) && IS_NPC(vict) && (ch != vict))
      {
        char kill_list[MAX_STRING_LENGTH] = {'\0'};
        snprintf(kill_list, sizeof(kill_list), "%s", QST_KLIST(rnum));
        char *pt = strtok(kill_list, ",");
        while (pt != NULL)
        {
            if (atoi(pt) == GET_MOB_VNUM(vict))
            {
              generic_complete_quest(ch, index);
              break;
            }
            pt = strtok(NULL, ",");
        }
      }
      break;

    case AQ_MOB_SAVE:
      if (ch == vict)
        found = FALSE;
      for (i = world[IN_ROOM(ch)].people; i && found; i = i->next_in_room)
        if (i && IS_NPC(i) && !MOB_FLAGGED(i, MOB_NOTDEADYET))
          if ((GET_MOB_VNUM(i) != QST_TARGET(rnum)) &&
              !AFF_FLAGGED(i, AFF_CHARM))
            found = FALSE;
      if (found)
        generic_complete_quest(ch, index);
      break;

    case AQ_OBJ_RETURN:
      if (IS_NPC(vict) && (GET_MOB_VNUM(vict) == QST_RETURNMOB(rnum)))
        if (object && (GET_OBJ_VNUM(object) == QST_TARGET(rnum)))
        {
          generic_complete_quest(ch, index);

          /* we are now removing the object once returned so the mob isn't killed and robbed -zusuk */
          obj = get_obj_in_list_num(real_object(QST_TARGET(rnum)),
                                    vict->carrying);
          if (obj)
          {
            obj_from_char(obj);
            obj_to_room(obj, real_room(1));
          }
        }
      break;

    case AQ_GIVE_GOLD:
      if (IS_NPC(vict) && (GET_MOB_VNUM(vict) == QST_RETURNMOB(rnum)))
        if (GET_GOLD(vict) >= QST_TARGET(rnum))
        {
          generic_complete_quest(ch, index);

          /* we are now removing the gold once returned so the mob isn't killed and robbed -zusuk */
          GET_GOLD(vict) = 0;
        }
      break;

    case AQ_ROOM_CLEAR:
      if (QST_TARGET(rnum) == world[IN_ROOM(ch)].number)
      {
        for (i = world[IN_ROOM(ch)].people; i && found; i = i->next_in_room)
          if (i && IS_NPC(i) && !MOB_FLAGGED(i, MOB_NOTDEADYET))
            found = FALSE;
        if (found)
          generic_complete_quest(ch, index);
      }
      break;

    default:
      log("SYSERR: Invalid quest type passed to autoquest_trigger_check");
      break;
    }
  }
}

/* process (clear) a quest that has timed out */
void quest_timeout(struct char_data *ch, int index)
{
  if ((GET_QUEST(ch, index) != NOTHING) && (GET_QUEST_TIME(ch, index) != -1))
  {
    clear_quest(ch, index);
    send_to_char(ch, "You have run out of time to complete the quest index: %d.\r\n", index);
  }
}

/* tick processing - decrement timers on all the in-game quests, if it reaches zero (0) then call quest_timeout() */
void check_timed_quests(void)
{
  struct char_data *ch, *next_ch;
  int index = 0;

  for (ch = character_list; ch; ch = next_ch)
  {
    next_ch = ch->next; /* Cache next char before potential extraction */
    
    for (index = 0; index < MAX_CURRENT_QUESTS; index++)
    { /* loop through all the character's quest slots */
      if (!IS_NPC(ch) && (GET_QUEST(ch, index) != NOTHING) && (GET_QUEST_TIME(ch, index) != -1))
      {
        if (--GET_QUEST_TIME(ch, index) == 0)
        {
          quest_timeout(ch, index);
        }
      }
    } /* 2nd for-loop, index of char quests */
  }   /* 1st for-loop, character list */
}

/*--------------------------------------------------------------------------*/
/* Quest Command Helper Functions                                           */
/*--------------------------------------------------------------------------*/
void list_quests(struct char_data *ch, zone_rnum zone, qst_vnum vmin, qst_vnum vmax)
{
  qst_rnum rnum;
  qst_vnum bottom, top;
  int counter = 0;

  if (zone != NOWHERE)
  {
    bottom = zone_table[zone].bot;
    top = zone_table[zone].top;
  }
  else
  {
    bottom = vmin;
    top = vmax;
  }
  /* Print the header for the quest listing. */
  send_to_char(ch,
               "Index VNum    Description                                  Questmaster\r\n"
               "----- ------- -------------------------------------------- -----------\r\n");
  for (rnum = 0; rnum < total_quests; rnum++)
    if (QST_NUM(rnum) >= bottom && QST_NUM(rnum) <= top)
      send_to_char(ch, "\tg%4d\tn) [\tg%-5d\tn] \tc%-44.44s\tn \ty[%5d]\tn\r\n",
                   ++counter, QST_NUM(rnum), QST_DESC(rnum),
                   QST_MASTER(rnum) == NOBODY ? 0 : QST_MASTER(rnum));
  if (!counter)
    send_to_char(ch, "None found.\r\n");
}

void quest_hist(struct char_data *ch, char argument[MAX_STRING_LENGTH])
{
  int i = 0, counter = 0, num_arg = -1;
  qst_rnum rnum = NOTHING;

  /* no argument, just do a general listing of history */
  if (!*argument)
  {
    send_to_char(ch, "Quests that you have completed:\r\n"
                     "Index Description                                          Questmaster\r\n"
                     "----- ---------------------------------------------------- -----------\r\n");
    for (i = 0; i < GET_NUM_QUESTS(ch); i++)
    {
      if ((rnum = real_quest(ch->player_specials->saved.completed_quests[i])) != NOTHING)
        send_to_char(ch, "\tg%4d\tn) \tc%-52.52s\tn \ty%s\tn\r\n",
                     ++counter, QST_DESC(rnum), (real_mobile(QST_MASTER(rnum)) == NOBODY) ? "Unknown" : GET_NAME(&mob_proto[(real_mobile(QST_MASTER(rnum)))]));
      else
        send_to_char(ch,
                     "\tg%4d\tn) \tcUnknown Quest (it no longer exists)\tn\r\n", ++counter);
    }
    if (!counter)
      send_to_char(ch, "You haven't completed any quests yet.\r\n");

    return;
  }

  /* convert argument to a integer */
  num_arg = atoi(argument);
  num_arg--;

  if (num_arg >= GET_NUM_QUESTS(ch))
  {
    send_to_char(ch, "You haven't completed that many quests yet.  Try quest progress?\r\n");
    return;
  }

  /* this is a safeguard check */
  if (num_arg >= MAX_COMPLETED_QUESTS)
  {
    send_to_char(ch, "You have exceeded the maximum amount of completed quests.\r\n");
    log("ERROR: num_arg [%d] in quest_hist() is over MAX_COMPLETED_QUESTS [%d]",
        num_arg, MAX_COMPLETED_QUESTS);
    return;
  }
  /* this is a safeguard check */
  if (num_arg < 0)
  {
    send_to_char(ch, "Invalid argument!\r\n");
    log("ERROR: num_arg [%d] in quest_hist() is less than zero", num_arg);
    return;
  }

  /* argument equal to number in history */
  if ((rnum = real_quest(ch->player_specials->saved.completed_quests[num_arg])) != NOTHING)
  {

    send_to_char(ch,
                 "\tmName  :\tn \ty%s\tn\r\n"
                 "\tmDesc  :\tn \ty%s\tn\r\n"
                 "\tmAccept Message:\tn\r\n%s"
                 "\tmCompletion Message:\tn\r\n%s",
                 QST_NAME(rnum), QST_DESC(rnum),
                 QST_INFO(rnum), QST_DONE(rnum));
  }
  else
  {
    send_to_char(ch, "\r\nNot valid input, please either use no input to view "
                     "your complete history or type history <nn> to view the details of "
                     "a completed quest.\r\n");
  }
}

/* rewrote this so quest objects can be equipped -zusuk */
/* 2nd re-write to allow for taking multiple quests -z */
void quest_join(struct char_data *ch, struct char_data *qm, char argument[MAX_INPUT_LENGTH])
{
  qst_vnum vnum = NOTHING;
  qst_rnum rnum = NOWHERE;
  obj_rnum objrnum = NOTHING;
  char buf[MAX_INPUT_LENGTH] = {'\0'};
  bool has_quest_object = FALSE, found = FALSE;
  int i = 0, index = 0, sr_index = 0;

  if (!*argument)
  {
    snprintf(buf, sizeof(buf),
             "\r\n%s, what quest did you wish to join?\r\n", GET_NAME(ch));
    send_to_char(ch, "%s", buf);
    return;
  }

  /* got a spare slot to join a quest? */
  for (index = 0; index < MAX_CURRENT_QUESTS; index++)
  {
    if (GET_QUEST(ch, index) == NOTHING)
    {
      found = TRUE;
      break; /* we need index for later */
    }
  }

  if (!found)
  {
    snprintf(buf, sizeof(buf),
             "\r\n%s, but you don't have any spare slots to join a quest!\r\n", GET_NAME(ch));
    send_to_char(ch, "%s", buf);
    return;
  }

  /* assign vnum */
  if ((vnum = find_quest_by_qmnum(ch, GET_MOB_VNUM(qm), atoi(argument))) == NOTHING)
  {
    snprintf(buf, sizeof(buf),
             "\r\n%s, I don't know of such a quest!\r\n", GET_NAME(ch));
    send_to_char(ch, "%s", buf);
    return;
  }

  /*debug*/ // send_to_char(ch, "Vnum of this quest: %d.\r\n", vnum);

  /* assign rnum */
  if ((rnum = real_quest(vnum)) == NOTHING)
  {
    snprintf(buf, sizeof(buf),
             "\r\n%s, I don't know of such a quest!\r\n", GET_NAME(ch));
    send_to_char(ch, "%s", buf);
    return;
  }

  /*debug*/ // send_to_char(ch, "Rnum of this quest: %d.\r\n", rnum);

  /* already on this particular quest? */
  found = FALSE; /* reset variable */
  for (sr_index = 0; sr_index < MAX_CURRENT_QUESTS; sr_index++)
    if (GET_QUEST(ch, sr_index) == vnum)
      found = TRUE;
  if (found)
  {
    snprintf(buf, sizeof(buf),
             "\r\n%s, you already have accepted that quest!\r\n", GET_NAME(ch));
    send_to_char(ch, "%s", buf);
    return;
  }

  /* quest requirements section */

  if ((GET_LEVEL(ch) < QST_MINLEVEL(rnum)) && GET_LEVEL(ch) < LVL_IMMORT)
  {
    snprintf(buf, sizeof(buf),
             "\r\n%s, you are not high enough level for that quest!\r\n", GET_NAME(ch));
    send_to_char(ch, "%s", buf);
    return;
  }
  else if (GET_LEVEL(ch) >= LVL_IMMORT)
  {
    send_to_char(ch, "\tRNOTE: you are over-riding Min-Level\tn\r\n");
  }

  if ((GET_LEVEL(ch) > QST_MAXLEVEL(rnum)) && GET_LEVEL(ch) < LVL_IMMORT)
  {
    snprintf(buf, sizeof(buf),
             "\r\n%s, you are too experienced for that quest!\r\n", GET_NAME(ch));
    send_to_char(ch, "%s", buf);
    return;
  }
  else if (GET_LEVEL(ch) >= LVL_IMMORT)
  {
    send_to_char(ch, "\tRNOTE: you are over-riding Max-Level\tn\r\n");
  }

  /* repeatable quests are handled here */
  if (is_complete(ch, vnum) && !(IS_SET(QST_FLAGS(rnum), AQ_REPEATABLE)) &&
      GET_LEVEL(ch) < LVL_IMMORT)
  {
    snprintf(buf, sizeof(buf),
             "\r\n%s, you have already completed that quest!\r\n", GET_NAME(ch));
    send_to_char(ch, "%s", buf);
    return;
  }
  else if (GET_LEVEL(ch) >= LVL_IMMORT)
  {
    send_to_char(ch, "\tRNOTE: you are over-riding 'quest completed'\tn\r\n");
  }

  /* requires previous quest completed (quest chain) */
  if ((QST_PREV(rnum) != NOTHING) && !is_complete(ch, QST_PREV(rnum)))
  {
    snprintf(buf, sizeof(buf),
             "\r\n%s, that quest is not available to you yet! (quest chain)\r\n", GET_NAME(ch));
    send_to_char(ch, "%s", buf);
    return;
  }

  // for a dialogue quest, if there are future quests that depend on the dialogue quest being done we
  // need to check for either:
  // A: The dialogue quest was finished and was not failed
  // B: The dialogue quest was finished, but it failed, and the alternative dialogue quest was complete
  // do this here... >>>
  if ((QST_PREV(rnum) != NOTHING) && is_complete(ch, QST_PREV(rnum)))
  {
    if (is_dialogue_quest_failed(ch, QST_PREV(rnum)))
    {
      qst_rnum prev_quest = real_quest(QST_PREV(rnum));
      qst_vnum alt_quest = aquest_table[prev_quest].dialogue_alternative_quest;
      if (!is_complete(ch, alt_quest))
      {
        send_to_char(ch, "The previous quest was a dialogue quest which failed. You will need to complete its alternative quest in order to take this quest.\r\n");
        return;
      }
    }
  }


  // if this is a dialogue quest's alternative quest we need to check if the dialogue quest was finished
  // and failed. If not, we cannot take this quest
  if (is_dialogue_alternative_quest(QST_NUM(rnum)))
  {
    if (!is_complete(ch, QST_PREV(rnum)) || !is_dialogue_quest_failed(ch, QST_PREV(rnum)))
    {
      qst_rnum d_rnum = get_dialogue_alternative_quest_rnum(QST_PREV(rnum));
      if (d_rnum != NOTHING)
        send_to_char(ch, "You must complete %s in order to take this quest.\r\n", QST_NAME(d_rnum));
      else 
        send_to_char(ch, "Error getting dialogue alternative quest rnum. Please report to staff.\r\n");
      return;
    }
  }


  /* does this quest require an object? */
  if ((QST_PREREQ(rnum) != NOTHING) && ((objrnum = real_object(QST_PREREQ(rnum))) != NOTHING))
  {
    /* inventory will do it */
    if (get_obj_in_list_num(objrnum, ch->carrying) != NULL)
      has_quest_object = TRUE;

    /* and wearing will do it too */
    for (i = 0; i < NUM_WEARS; i++)
    {
      if (GET_EQ(ch, i) && GET_OBJ_RNUM(GET_EQ(ch, i)) == objrnum)
      {
        has_quest_object = TRUE;
        break;
      }
    }

    if (!has_quest_object)
    {
      snprintf(buf, sizeof(buf),
               "\r\n%s, you need to have %s first!\r\n", GET_NAME(ch),
               obj_proto[real_object(QST_PREREQ(rnum))].short_description);
      send_to_char(ch, "%s", buf);
      return;
    }
  }

  /* we made it! */
  act("You join the quest.", TRUE, ch, NULL, NULL, TO_CHAR);
  act("$n has joined a quest.", TRUE, ch, NULL, NULL, TO_ROOM);
  snprintf(buf, sizeof(buf),
           "\tW%s \tWlisten carefully to the instructions.\tn\r\n\r\n", GET_NAME(ch));
  send_to_char(ch, "%s", buf);
  set_quest(ch, rnum, index);
  send_to_char(ch, "%s", QST_INFO(rnum));
  if (QST_TYPE(rnum) == AQ_DIALOGUE)
  {
    send_to_char(ch, "\ty");
    draw_line(ch, 80, '-', '-');
    send_to_char(ch, "This is a dialogue quest. You must attempt to complete it using one of the\r\nfollowing dialogue commands:\r\n");
    if (aquest_table[rnum].diplomacy_dc > 0)
      send_to_char(ch, "Persuasion Skill  - 'convince' command.\r\n");
    if (aquest_table[rnum].intimidate_dc > 0)
      send_to_char(ch, "Intimidate Skill - 'threaten' command.\r\n");
    if (aquest_table[rnum].diplomacy_dc > 0)
      send_to_char(ch, "Deception Skill      - 'beguile' command.\r\n");
    draw_line(ch, 80, '-', '-');
    send_to_char(ch, "\tn");
  }
  if (QST_TIME(rnum) != -1)
  {
    snprintf(buf, sizeof(buf),
             "\r\n\tW%s, \tWyou have a time limit of %d turn%s to complete the quest.\tn\r\n\r\n",
             GET_NAME(ch), QST_TIME(rnum), QST_TIME(rnum) == 1 ? "" : "s");
  }
  else
  {
    snprintf(buf, sizeof(buf),
             "\r\n\tW%s, \tWyou can take however long you want to complete the quest.\tn\r\n",
             GET_NAME(ch));
  }
  send_to_char(ch, "%s", buf);
  save_char(ch, 0);
}

/* lists available quests, can also accept vnum or list-number to view
 details of a specific quest */
void quest_list(struct char_data *ch, struct char_data *qm, char argument[MAX_INPUT_LENGTH])
{
  qst_vnum vnum;
  qst_rnum rnum;

  if ((vnum = find_quest_by_qmnum(ch, GET_MOB_VNUM(qm), atoi(argument))) == NOTHING)
    send_to_char(ch, "That is not a valid quest!\r\n");
  else if ((rnum = real_quest(vnum)) == NOTHING)
    send_to_char(ch, "That is not a valid quest!\r\n");
  else if (QST_INFO(rnum))
  {
    send_to_char(ch, "Complete Details on Quest %d \tc%s\tn:\r\n%s",
                 vnum,
                 QST_DESC(rnum),
                 QST_INFO(rnum));
    if (QST_PREV(rnum) != NOTHING)
      send_to_char(ch, "You have to have completed quest %s first.\r\n",
                   QST_NAME(real_quest(QST_PREV(rnum))));
    if (QST_TIME(rnum) != -1)
      send_to_char(ch,
                   "There is a time limit of %d turn%s to complete the quest.\r\n",
                   QST_TIME(rnum),
                   QST_TIME(rnum) == 1 ? "" : "s");
  }
  else
    send_to_char(ch, "There is no further information on that quest.\r\n");
}

/* will drop the current quest index the player is pursuing */
void quest_quit(struct char_data *ch, char argument[MAX_STRING_LENGTH])
{
  qst_rnum rnum;
  int index = -1;

  if (!*argument)
  {
    send_to_char(ch, "You need to provide the quest index from your queue to leave.\r\n");
    return;
  }

  /* convert argument to a integer */
  index = atoi(argument);

  if (index >= MAX_CURRENT_QUESTS || index < 0)
  {
    send_to_char(ch, "Invalid index (%d)!\r\n", index);
    return;
  }

  if (GET_QUEST(ch, index) == NOTHING)
  {
    send_to_char(ch, "But you currently aren't on a quest in that index!\r\n");
  }
  else if ((rnum = real_quest(GET_QUEST(ch, index))) == NOTHING)
  {
    clear_quest(ch, index);
    send_to_char(ch, "You are now no longer part of the quest.\r\n");
    save_char(ch, 0);
  }
  else if (QST_PREV(rnum) != NOTHING && QST_TYPE(QST_PREV(rnum)) == AQ_DIALOGUE)
  {
    send_to_char(ch, "You cannot leave this quest. It must be completed.\r\n");
  }
  else
  {
    clear_quest(ch, index);
    if (QST_QUIT(rnum) && (str_cmp(QST_QUIT(rnum), "undefined") != 0))
      send_to_char(ch, "%s", QST_QUIT(rnum));
    else
      send_to_char(ch, "You are now no longer part of the quest.\r\n");
    if (QST_PENALTY(rnum))
    {
      GET_QUESTPOINTS(ch) -= QST_PENALTY(rnum);
      send_to_char(ch,
                   "You have lost %d quest points for your cowardice.\r\n",
                   QST_PENALTY(rnum));
    }
    save_char(ch, 0);
  }
}

/* will give player current status on their quest they are working on */
void quest_progress(struct char_data *ch, char argument[MAX_STRING_LENGTH])
{
  qst_rnum rnum;
  int index = -1;

  if (!*argument)
  {
    send_to_char(ch, "You are on the following quests:\r\n");
    for (index = 0; index < MAX_CURRENT_QUESTS; index++)
    {
      if ((rnum = real_quest(GET_QUEST(ch, index))) == NOTHING)
      {
        clear_quest(ch, index); /* safety clearing */
        send_to_char(ch, " (Index: %d) This quest slot is available.\r\n", index);
      }
      else
        send_to_char(ch, "(Index: %d) - %s [vnum %d]\r\n", index, QST_NAME(rnum), QST_NUM(rnum));
    }
    send_to_char(ch, "You can provide the quest index from your queue to check specific progress details (ex. quest progress <index # above>).\r\n");
    return;
  }

  /* convert argument to a integer */
  index = atoi(argument);

  if (index >= MAX_CURRENT_QUESTS || index < 0)
  {
    send_to_char(ch, "Invalid index (%d)!  This command can be used without an argument.\r\n", index);
    return;
  }

  if (GET_QUEST(ch, index) == NOTHING)
  {
    send_to_char(ch, "But you currently aren't on a quest in that slot!\r\n");
  }
  else if ((rnum = real_quest(GET_QUEST(ch, index))) == NOTHING)
  {
    clear_quest(ch, index);
    send_to_char(ch, "That quest seems to no longer exist.\r\n");
  }
  else
  {
    send_to_char(ch, "You are on the following quest:\r\n%s\r\n%s",
                 QST_NAME(rnum), QST_INFO(rnum));
    if (QST_QUANTITY(rnum) > 1)
      send_to_char(ch,
                   "You still have to achieve %d out of %d goals for the quest.\r\n",
                   GET_QUEST_COUNTER(ch, index), QST_QUANTITY(rnum));
  if (QST_TYPE(rnum) == AQ_DIALOGUE)
  {
    send_to_char(ch, "\ty");
    draw_line(ch, 80, '-', '-');
    send_to_char(ch, "This is a dialogue quest. You must find the mob attempt to persuade it using\r\none of the following dialogue commands:\r\n");
    if (aquest_table[rnum].diplomacy_dc > 0)
      send_to_char(ch, "Persuasion Skill  - 'convince' command.\r\n");
    if (aquest_table[rnum].intimidate_dc > 0)
      send_to_char(ch, "Intimidate Skill - 'threaten' command.\r\n");
    if (aquest_table[rnum].diplomacy_dc > 0)
      send_to_char(ch, "Deception Skill      - 'beguile' command.\r\n");
    draw_line(ch, 80, '-', '-');
    send_to_char(ch, "\tn");
  }
    if (GET_QUEST_TIME(ch, index) > 0)
      send_to_char(ch,
                   "You have %d turn%s remaining to complete the quest.\r\n",
                   GET_QUEST_TIME(ch, index),
                   GET_QUEST_TIME(ch, index) == 1 ? "" : "s");
  }
}

/* displays a list of quests available at given quest master */
void quest_show(struct char_data *ch, mob_vnum qm)
{
  qst_rnum rnum;
  int counter = 0;

  if (GET_LEVEL(ch) >= LVL_IMMORT)
  {
    send_to_char(ch,
                 "The following quests are available:\r\n"
                 "Index Quest Name                                           (VNum)   Done? Repeatable?\r\n"
                 "----- ---------------------------------------------------- -------- ----- -----------\r\n");
    for (rnum = 0; rnum < total_quests; rnum++)
      if (qm == QST_MASTER(rnum))
        send_to_char(ch, "\tg%4d\tn) \tc%-52.52s\tn \ty(%6d)\tn \ty(%3s)\tn \ty(%3s)\tn\r\n",
                     ++counter, QST_NAME(rnum), QST_NUM(rnum),
                     (is_complete(ch, QST_NUM(rnum)) ? "Yes" : "No "),
                     ((IS_SET(QST_FLAGS(rnum), AQ_REPEATABLE)) ? "Yes" : "No "));
  }
  else
  {
    send_to_char(ch,
                 "The following quests are available:                              Min Max\r\n"
                 "Index Quest Name                                           Done? Lvl Lvl Repeatable?\r\n"
                 "----- ---------------------------------------------------- ----- --- --- -----------\r\n");
    for (rnum = 0; rnum < total_quests; rnum++)
      if (qm == QST_MASTER(rnum))
        send_to_char(ch, "\tg%4d\tn) \tc%-52.52s\tn \ty(%3s)\tn %3d %3d \ty(%3s)\r\n",
                     ++counter, QST_NAME(rnum), (is_complete(ch, QST_NUM(rnum)) ? "Yes" : "No "), QST_MINLEVEL(rnum), QST_MAXLEVEL(rnum),
                     ((IS_SET(QST_FLAGS(rnum), AQ_REPEATABLE)) ? "Yes" : "No "));
  }

  if (!counter)
    send_to_char(ch, "There are no quests available here at the moment.\r\n");
}

/* allows staff to assign a quest as completed to given target */
void quest_assign(struct char_data *ch, char argument[MAX_STRING_LENGTH])
{
  char arg1[MAX_INPUT_LENGTH] = {'\0'}, arg2[MAX_INPUT_LENGTH] = {'\0'};
  struct char_data *victim = NULL;
  qst_rnum rnum = NOTHING;
  // qst_vnum vnum = NOTHING;
  int index = 0;
  bool found = FALSE;

  two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

  if (GET_LEVEL(ch) < LVL_IMMORT)
    send_to_char(ch, "Huh!?!\r\n");
  else if (!*arg1)
    send_to_char(ch, "Usage: quest assign <target> <quest vnum>\r\n");
  else if (!*arg2)
    send_to_char(ch, "Usage: quest assign <target> <quest vnum>\r\n");
  else if ((victim = get_player_vis(ch, arg1, NULL, FIND_CHAR_WORLD)) == NULL)
    send_to_char(ch, "Can not find that target!\r\n");
  else if ((rnum = real_quest(atoi(arg2))) == NOTHING)
    send_to_char(ch, "That quest does not exist.\r\n");
  else if (is_complete(victim, atoi(arg2)))
    send_to_char(ch, "That character already completed that quest.\r\n");

  /* got a spare slot to join a quest? */
  for (index = 0; index < MAX_CURRENT_QUESTS; index++)
  {
    if (GET_QUEST(victim, index) == NOTHING)
    {
      found = TRUE;
      break;
    }
  }

  if (!found)
  {
    send_to_char(ch, "That player doesn't have any spare quest slots!\r\n");
    return;
  }

  GET_QUEST(victim, index) = atoi(arg2);
  complete_quest(victim, index);
  send_to_char(ch, "Success! \r\n");
}

/* allows staff to view detailed info about any quest in game */
void quest_stat(struct char_data *ch, char argument[MAX_STRING_LENGTH])
{
  qst_rnum rnum = NOTHING;
  mob_rnum qmrnum = NOBODY;
  char buf[MAX_STRING_LENGTH] = {'\0'};
  char targetname[MAX_STRING_LENGTH] = {'\0'};
  char rewardname[MAX_STRING_LENGTH] = {'\0'};
  char racename[MAX_STRING_LENGTH] = {'\0'};
  char followername[MAX_STRING_LENGTH] = {'\0'};
  bool valid_race = FALSE, valid_follower = FALSE;

  if (GET_LEVEL(ch) < LVL_IMMORT)
    send_to_char(ch, "Huh!?!\r\n");
  else if (!*argument)
    send_to_char(ch, "%s\r\n", quest_imm_usage);
  else if ((rnum = real_quest(atoi(argument))) == NOTHING)
    send_to_char(ch, "That quest does not exist.\r\n");

  else
  {
    sprintbit(QST_FLAGS(rnum), aq_flags, buf, sizeof(buf));
    switch (QST_TYPE(rnum))
    {

    case AQ_COMPLETE_MISSION:
      switch (QST_TARGET(rnum))
      {
      case MISSION_DIFF_NORMAL:
        snprintf(targetname, sizeof(targetname), "Normal Mission Difficulty ");
        break;
      case MISSION_DIFF_TOUGH:
        snprintf(targetname, sizeof(targetname), "Tough Mission Difficulty ");
        break;
      case MISSION_DIFF_CHALLENGING:
        snprintf(targetname, sizeof(targetname), "Challenging Mission Difficulty ");
        break;
      case MISSION_DIFF_ARDUOUS:
        snprintf(targetname, sizeof(targetname), "Arduous Mission Difficulty ");
        break;
      case MISSION_DIFF_SEVERE:
        snprintf(targetname, sizeof(targetname), "Severe Mission Difficulty ");
        break;
      default:
        //#define MISSION_DIFF_EASY 0
        /* EASY or weird value */
        snprintf(targetname, sizeof(targetname), "Easy Mission Difficulty ");
        break;
      }

      break;

    case AQ_OBJ_FIND:
    case AQ_OBJ_RETURN:
      snprintf(targetname, sizeof(targetname), "%s",
               real_object(QST_TARGET(rnum)) == NOTHING ? "An unknown object" : obj_proto[real_object(QST_TARGET(rnum))].short_description);
      break;

    case AQ_GIVE_GOLD:
      snprintf(targetname, sizeof(targetname), "Minimum gold need to be given: %d ", QST_TARGET(rnum));
      break;

    case AQ_WILD_FIND:
      snprintf(targetname, sizeof(targetname), "Co-ords: %d, %d ", QST_COORD_X(rnum), QST_COORD_Y(rnum));
      break;

    case AQ_ROOM_FIND:
    case AQ_ROOM_CLEAR:
      snprintf(targetname, sizeof(targetname), "%s",
               real_room(QST_TARGET(rnum)) == NOWHERE ? "An unknown room" : world[real_room(QST_TARGET(rnum))].name);
      break;

    case AQ_MOB_FIND:
    case AQ_MOB_KILL:
    case AQ_MOB_SAVE:
      snprintf(targetname, sizeof(targetname), "%s",
               real_mobile(QST_TARGET(rnum)) == NOBODY ? "An unknown mobile" : GET_NAME(&mob_proto[real_mobile(QST_TARGET(rnum))]));
      break;

    default:
      snprintf(targetname, sizeof(targetname), "Unknown");
      break;
    }

    /* determine quest object reward name */
    snprintf(rewardname, sizeof(rewardname), "%s",
             real_object(QST_OBJ(rnum)) == NOTHING ? "An unknown object" : obj_proto[real_object(QST_OBJ(rnum))].short_description);

    qmrnum = real_mobile(QST_MASTER(rnum));

    /* race reward info */
    if (QST_RACE(rnum) < NUM_EXTENDED_RACES && QST_RACE(rnum) > RACE_UNDEFINED)
    {
      snprintf(racename, sizeof(racename), "%s", race_list[QST_RACE(rnum)].type_color);
      valid_race = TRUE;
    }

    /* follower reward info */
    if (QST_FOLLOWER(rnum) != NOBODY)
    {
      snprintf(followername, sizeof(followername), "%s",
               real_mobile(QST_FOLLOWER(rnum)) == NOBODY ? "An unknown mobile" : GET_NAME(&mob_proto[real_mobile(QST_FOLLOWER(rnum))]));
      valid_follower = TRUE;
    }

    /* display time! */
    send_to_char(ch,
                 "VNum  : [\ty%5d\tn], RNum: [\ty%5d\tn] -- Questmaster: [\ty%5d\tn] \ty%s\tn\r\n"
                 "Name  : \ty%s\tn\r\n"
                 "Desc  : \ty%s\tn\r\n"
                 "Accept Message:\r\n\tc%s\tn"
                 "Completion Message:\r\n\tc%s\tn"
                 "Quit Message:\r\n\tc%s\tn"
                 "Type  : \ty%s\tn\r\n"
                 "Target: \ty%d\tn \ty%s\tn, Quantity: \ty%d\tn\r\n"
                 "Value : \ty%d\tn, Penalty: \ty%d\tn, Min Level: \ty%2d\tn, Max Level: \ty%2d\tn\r\n"
                 "Gold Reward: \ty%d\tn, Exp Reward: \ty%d\tn, Obj Reward: \ty(%d)\tn %s\r\n"
                 "Quest Race Reward: %s (%d)\r\n"
                 "Quest Follower Reward: %s (%d)\r\n"
                 "Flags : \tc%s\tn\r\n",

                 QST_NUM(rnum), rnum,
                 QST_MASTER(rnum) == NOBODY ? -1 : QST_MASTER(rnum),
                 (qmrnum == NOBODY) ? "(Invalid vnum)" : GET_NAME(&mob_proto[(qmrnum)]),
                 QST_NAME(rnum), QST_DESC(rnum),
                 QST_INFO(rnum), QST_DONE(rnum),
                 (QST_QUIT(rnum) &&
                          (str_cmp(QST_QUIT(rnum), "undefined") != 0)
                      ? QST_QUIT(rnum)
                      : "Nothing\r\n"),
                 quest_types[QST_TYPE(rnum)],
                 QST_TARGET(rnum) == NOBODY ? -1 : QST_TARGET(rnum),
                 targetname,
                 QST_QUANTITY(rnum),
                 QST_POINTS(rnum) /*val0*/, QST_PENALTY(rnum) /*val1*/, QST_MINLEVEL(rnum) /*val2*/,
                 QST_MAXLEVEL(rnum) /*val3*/,
                 QST_GOLD(rnum), QST_EXP(rnum), QST_OBJ(rnum),
                 rewardname,
                 valid_race ? racename : "N/A", QST_RACE(rnum),
                 valid_follower ? followername : "N/A", QST_FOLLOWER(rnum),
                 buf);

    if (QST_PREREQ(rnum) != NOTHING)
      send_to_char(ch, "Preq  : [\ty%5d\tn] \ty%s\tn\r\n",
                   QST_PREREQ(rnum) == NOTHING ? -1 : QST_PREREQ(rnum),
                   QST_PREREQ(rnum) == NOTHING ? "" : real_object(QST_PREREQ(rnum)) == NOTHING ? "an unknown object"
                                                                                               : obj_proto[real_object(QST_PREREQ(rnum))].short_description);
    if (QST_TYPE(rnum) == AQ_OBJ_RETURN || QST_TYPE(rnum) == AQ_GIVE_GOLD)
      send_to_char(ch, "Mob   : [\ty%5d\tn] \ty%s\tn\r\n",
                   QST_RETURNMOB(rnum),
                   real_mobile(QST_RETURNMOB(rnum)) == NOBODY ? "an unknown mob" : mob_proto[real_mobile(QST_RETURNMOB(rnum))].player.short_descr);
    if (QST_TIME(rnum) != -1)
      send_to_char(ch, "Limit : There is a time limit of %d turn%s to complete.\r\n",
                   QST_TIME(rnum),
                   QST_TIME(rnum) == 1 ? "" : "s");
    else
      send_to_char(ch, "Limit : There is no time limit on this quest.\r\n");
    send_to_char(ch, "Prior :");
    if (QST_PREV(rnum) == NOTHING)
      send_to_char(ch, " \tyNone.\tn\r\n");
    else
      send_to_char(ch, " [\ty%5d\tn] \tc%s\tn\r\n",
                   QST_PREV(rnum), QST_DESC(real_quest(QST_PREV(rnum))));
    send_to_char(ch, "Next  :");
    if (QST_NEXT(rnum) == NOTHING)
      send_to_char(ch, " \tyNone.\tn\r\n");
    else
      send_to_char(ch, " [\ty%5d\tn] \tc%s\tn\r\n",
                   QST_NEXT(rnum), QST_DESC(real_quest(QST_NEXT(rnum))));
  }
}

/*--------------------------------------------------------------------------*/
/* Quest Command Processing Function and Questmaster Special                */

/*--------------------------------------------------------------------------*/
ACMD(do_quest)
{
  char arg1[MAX_INPUT_LENGTH] = {'\0'}, arg2[MAX_STRING_LENGTH] = {'\0'};
  int tp;

  two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));
  if (!*arg1)
    send_to_char(ch, "%s\r\n", GET_LEVEL(ch) < LVL_IMMORT ? quest_mort_usage : quest_imm_usage);
  else if (((tp = search_block(arg1, quest_cmd, FALSE)) == -1))
    send_to_char(ch, "%s\r\n", GET_LEVEL(ch) < LVL_IMMORT ? quest_mort_usage : quest_imm_usage);
  else
  {
    switch (tp)
    {
    case SCMD_QUEST_LIST:
    case SCMD_QUEST_JOIN:
      /* list, join should have been handled by
       * (SPECIAL(questmaster)) questmaster spec proc */
      send_to_char(ch, "Sorry, but you cannot do that here!\r\n");
      break;
    case SCMD_QUEST_HISTORY:
      quest_hist(ch, arg2);
      break;
    case SCMD_QUEST_LEAVE:
      quest_quit(ch, arg2);
      break;
    case SCMD_QUEST_PROGRESS:
      quest_progress(ch, arg2);
      break;
    case SCMD_QUEST_STATUS:
      if (GET_LEVEL(ch) < LVL_IMMORT)
        send_to_char(ch, "%s\r\n", quest_mort_usage);
      else
        quest_stat(ch, arg2);
      break;
    case SCMD_QUEST_ASSIGN:
      quest_assign(ch, arg2);
      break;
    default: /* Whe should never get here, but... */
      send_to_char(ch, "%s\r\n", GET_LEVEL(ch) < LVL_IMMORT ? quest_mort_usage : quest_imm_usage);
      break;
    } /* switch on subcmd number */
  }
}

/* with a given object vnum, finds references to quests in game */
ACMD(do_aqref)
{
  int i = 0;
  bool found = FALSE;
  obj_vnum vnum = 0;
  obj_rnum real_num = 0;
  char buf[MAX_INPUT_LENGTH] = {'\0'};

  one_argument(argument, buf, sizeof(buf));

  if (!*buf)
  {
    send_to_char(ch, "aqref what object?\r\n");
    return;
  }

  vnum = atoi(buf);
  real_num = real_object(vnum);

  if (real_num == NOTHING)
  {
    send_to_char(ch, "\tRNo such object!\tn\r\n");
    return;
  }

  if (GET_LEVEL(ch) < LVL_IMMORT)
  {
    snprintf(buf, sizeof(buf), "(GC) %s did a reference check for (%d).", GET_NAME(ch), vnum);
    log("%s", buf);
    return;
  }

  if (!aquest_table)
  {
    send_to_char(ch, "\tRNo aquest_table!\tn\r\n");
    return;
  }

  for (i = 0; i < total_quests; i++)
  {
    if (QST_OBJ(i) && QST_OBJ(i) == vnum)
    {
      found = TRUE;
      send_to_char(ch, "(%d) \tCREWARD\tn %s (\tW%d\tn) from %s (\tW%d\tn)\r\n", QST_NUM(i), obj_proto[real_num].short_description,
                   vnum, mob_proto[real_mobile(QST_MASTER(i))].player.short_descr, QST_MASTER(i));
    }

    if ((QST_TYPE(i) == AQ_OBJ_FIND) && QST_TARGET(i) && QST_TARGET(i) == vnum)
    {
      found = TRUE;
      send_to_char(ch, "(%d) \tCFIND\tn %s (\tW%d\tn) for %s (\tW%d\tn)\r\n", QST_NUM(i), obj_proto[real_num].short_description,
                   vnum, mob_proto[real_mobile(QST_MASTER(i))].player.short_descr, QST_MASTER(i));
    }

    if ((QST_TYPE(i) == AQ_OBJ_RETURN) && QST_TARGET(i) && QST_TARGET(i) == vnum)
    {
      found = TRUE;
      send_to_char(ch, "(%d) \tCRETURN\tn %s (\tW%d\tn) to %s (\tW%d\tn)\r\n", QST_NUM(i), obj_proto[real_num].short_description,
                   vnum, mob_proto[real_mobile(QST_MASTER(i))].player.short_descr, QST_MASTER(i));
    }
  }

  if (!found)
  {
    send_to_char(ch, "\tRThat object is not used in any quests!\tn\r\n");
  }

  return;
}

/* here is the mobile-spec proc for the quest master */
SPECIAL(questmaster)
{
  qst_rnum rnum;
  char arg1[MAX_INPUT_LENGTH] = {'\0'}, arg2[MAX_INPUT_LENGTH] = {'\0'};
  int tp;
  struct char_data *qm = (struct char_data *)me;

  /* check that qm mob has quests assigned */
  for (rnum = 0; (rnum < total_quests &&
                  QST_MASTER(rnum) != GET_MOB_VNUM(qm));
       rnum++)
    ;
  if (rnum >= total_quests)
    return FALSE; /* No quests for this mob */
  else if (QST_FUNC(rnum) && (QST_FUNC(rnum)(ch, me, cmd, argument)))
    return TRUE; /* The secondary spec proc handled this command */
  else if (CMD_IS("quest"))
  {
    two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));
    if (!*arg1)
      return FALSE;
    else if (((tp = search_block(arg1, quest_cmd, FALSE)) == -1))
      return FALSE;
    else
    {
      switch (tp)
      {
      case SCMD_QUEST_LIST:
        if (!*arg2)
          quest_show(ch, GET_MOB_VNUM(qm));
        else
          quest_list(ch, qm, arg2);
        break;
      case SCMD_QUEST_JOIN:
        quest_join(ch, qm, arg2);
        break;
      default:
        return FALSE; /* fall through to the do_quest command processor */
      }               /* switch on subcmd number */
      return TRUE;
    }
  }
  else
  {
    return FALSE; /* not a questmaster command */
  }
}

bool is_dialogue_quest_failed(struct char_data *ch, qst_vnum q_vnum)
{
  if (!ch)
  {
    return false;
  }
  
  if (q_vnum <= 0)
  {
    return false;
  }

  qst_rnum q_rnum;
  int i = 0;
  bool failed = false;

  if ((q_rnum = real_quest(q_vnum)) == NOTHING)
  {
    return false;
  }

  if (QST_TYPE(q_rnum) != AQ_DIALOGUE)
  {

     return false;
  }

  for (i = 0; i < 100; i++)
  {
    if (ch->player_specials->saved.failed_dialogue_quests[i] == q_vnum)
    {

      failed = true;
      break;
    }
  }

  return failed;
}

void set_dialogue_quest_failed(struct char_data *ch, qst_vnum q_vnum)
{
  if (!ch)
  {
    return;
  }
  if (q_vnum <= 0)
  {
    return;
  }

  qst_rnum q_rnum;
  int i = 0;

  if ((q_rnum = real_quest(q_vnum)) == NOTHING)
  {
    return;
  }

  if (QST_TYPE(q_rnum) != AQ_DIALOGUE)
  {
    return;
  }

  for (i = 0; i < 100; i++)
  {
    if (ch->player_specials->saved.failed_dialogue_quests[i] == 0)
    {
      ch->player_specials->saved.failed_dialogue_quests[i] = q_vnum;
      break;
    }
  }
}

void set_dialogue_quest_succeeded(struct char_data *ch, qst_vnum q_vnum)
{
  if (!ch) return;
  if (q_vnum <= 0) return;

  qst_rnum q_rnum;
  int i = 0;

  if ((q_rnum = real_quest(q_vnum)) == NOTHING)
    return;

  if (QST_TYPE(q_rnum) != AQ_DIALOGUE)
    return;

  for (i = 0; i < 100; i++)
  {
    if (ch->player_specials->saved.failed_dialogue_quests[i] == q_vnum)
    {
      ch->player_specials->saved.failed_dialogue_quests[i] = 0;
      break;
    }
  }
}

bool is_dialogue_alternative_quest(qst_vnum vnum)
{
  qst_rnum rnum = 0;

  if (!aquest_table)
    return false;

  for (rnum = 0; rnum < total_quests; rnum++)
  {
    if (aquest_table[rnum].dialogue_alternative_quest == vnum)
      return true;
  }
  return false;
}

qst_rnum get_dialogue_alternative_quest_rnum(qst_vnum dialogue_quest)
{

  qst_rnum rnum = 0;

  if (!aquest_table)
    return NOTHING;

  for (rnum = 0; rnum < total_quests; rnum++)
  {
    if (aquest_table[rnum].dialogue_alternative_quest == dialogue_quest)
    {
      return rnum;
    }
  }
  return NOTHING;
}

bool has_duplicate_quest(struct char_data *ch)
{
  int i = 0, j = 0;
  bool duplicate = false;

  for (i = 0; i < MAX_CURRENT_QUESTS; i++)
  {
    if (GET_QUEST(ch, i) == NOTHING) continue;
    for (j = i+1; j < MAX_CURRENT_QUESTS; j++)
    {
      if (real_quest(GET_QUEST(ch, i)) == NOTHING) continue;
      if (real_quest(GET_QUEST(ch, j)) == NOTHING) continue;
      if (real_quest(GET_QUEST(ch, i)) == real_quest(GET_QUEST(ch, j)))
      {
        duplicate = true;
      }
    }
  }
  return duplicate;
}

void remove_duplicate_quests(struct char_data *ch)
{
  int i = 0, j = 0;

  for (i = 0; i < MAX_CURRENT_QUESTS; i++)
  {
    if (GET_QUEST(ch, i) == NOTHING) continue;
    for (j = i+1; j < MAX_CURRENT_QUESTS; j++)
    {
      if (j >= MAX_CURRENT_QUESTS) break;
      if (real_quest(GET_QUEST(ch, i)) == NOTHING) continue;
      if (real_quest(GET_QUEST(ch, j)) == NOTHING) continue;
      if (real_quest(GET_QUEST(ch, i)) == real_quest(GET_QUEST(ch, j)))
      {
        clear_quest(ch, i);
      }
    }
  }
}

/* EOF */
