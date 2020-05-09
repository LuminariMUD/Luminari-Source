/* *************************************************************************
 *   File: hlqedit.c                                   Part of LuminariMUD *
 *  Usage: alternate quest system                                          *
 *  Author: Homeland (Vhaerun), ported to luminari by Zusuk                *
 ************************************************************************* */

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "comm.h"
#include "utils.h"
#include "db.h"
#include "boards.h"
#include "handler.h"
#include "oasis.h"
#include "interpreter.h"
#include "constants.h"
#include "hlquest.h"
#include "spells.h"
#include "class.h"
#include "genzon.h"
#include "genolc.h"
#include "genmob.h"
#include "improved-edit.h"
#include "modify.h"

/*---------------------------------------------*/
/*. Function prototypes / Globals / Externals. */
/*---------------------------------------------*/

const char * const hlqedit_command = "CIOMADTXFKUS";

/*****************************************************************************/

/*---------------------------------------------*/
/*. local functions, utility                            */
/*---------------------------------------------*/

/* this is ported from old circlemud 3.0 for exclusive usage
   for the questing system */
void zedit_create_index(int znum)
{
  FILE *newfile, *oldfile;
  char new_name[32], old_name[32];
  const char *prefix;
  int num, found = FALSE;
  char buf1[MAX_INPUT_LENGTH] = {'\0'};
  char buf[MAX_INPUT_LENGTH] = {'\0'};

  prefix = HLQST_PREFIX;

  snprintf(old_name, sizeof(old_name), "%s/index", prefix);
  snprintf(new_name, sizeof(new_name), "%s/newindex", prefix);

  if (!(oldfile = fopen(old_name, "r")))
  {
    snprintf(buf, sizeof(buf), "SYSERR: OLC: Failed to open %s", old_name);
    log(buf);
    return;
  }
  else if (!(newfile = fopen(new_name, "w")))
  {
    snprintf(buf, sizeof(buf), "SYSERR: OLC: Failed to open %s", new_name);
    log(buf);
    return;
  }

  /*
   * Index contents must be in order: search through the old file for the
   * right place, insert the new file, then copy the rest over. 
   */
  snprintf(buf1, sizeof(buf1), "%d.%s", znum, "hlq");
  while (get_line(oldfile, buf))
  {
    if (*buf == '$')
    {
      fprintf(newfile, "%s\n$\n", (!found ? buf1 : ""));
      break;
    }
    else if (!found)
    {
      sscanf(buf, "%d", &num);
      if (num == znum)
        found = TRUE;
      if (num > znum)
      {
        found = TRUE;
        fprintf(newfile, "%s\n", buf1);
      }
    }
    fprintf(newfile, "%s\n", buf);
  }

  fclose(newfile);
  fclose(oldfile);
  /*
   * Out with the old, in with the new.
   */
  remove(old_name);
  rename(new_name, old_name);
}

/* lists classes appropriate for hlqedit
   UNFINISHED for Luminari usage */
void hlqedit_show_classes(struct descriptor_data *d)
{
  char buf[MAX_INPUT_LENGTH] = {'\0'};
  int i;

  for (i = 0; i < NUM_CLASSES; i++)
  {
    snprintf(buf, sizeof(buf), "%d) %s\r\n", i, CLSLIST_NAME(i));
    send_to_char(d->character, buf);
  }
}

/*
 *  this is the first step in editing an existing or new quest
 */
void hlqedit_setup(struct descriptor_data *d, mob_rnum mob)
{
  struct quest_entry *quest = NULL;
  struct quest_entry *qexist = NULL;
  struct quest_entry *qtmp = NULL;
  struct quest_command *qcom = NULL;
  struct quest_command *qlast = NULL;
  struct quest_command *qcomexist = NULL;
  struct char_data *ch = NULL;

  ch = &mob_proto[mob];

  /*
     Okies copy what already exist
   */
  if (ch && ch->mob_specials.quest)
  {

    for (qexist = ch->mob_specials.quest; qexist; qexist = qexist->next)
    {
      if (ch && qexist)
      {

        CREATE(qtmp, struct quest_entry, 1);
        clear_hlquest(qtmp);
        qtmp->type = qexist->type;
        qtmp->approved = qexist->approved;
        if (qexist->reply_msg)
          qtmp->reply_msg = strdup(qexist->reply_msg);
        else
          qtmp->reply_msg = strdup("Nothing\r\n");
        if (qtmp->type == QUEST_ASK && qexist->keywords)
          qtmp->keywords = strdup(qexist->keywords);
        else if (qtmp->type == QUEST_ASK)
          qtmp->keywords = strdup("hi hello quest");
        qtmp->room = qexist->room;

        qtmp->next = quest;
        quest = qtmp;

        /* in chain */
        for (qcomexist = qexist->in; qcomexist; qcomexist = qcomexist->next)
        {
          CREATE(qcom, struct quest_command, 1);
          qcom->type = qcomexist->type;
          qcom->value = qcomexist->value;
          qcom->location = qcomexist->location;
          qcom->next = quest->in;
          quest->in = qcom;
        }

        /* out chain */
        for (qcomexist = qexist->out; qcomexist; qcomexist = qcomexist->next)
        {
          CREATE(qcom, struct quest_command, 1);
          qcom->type = qcomexist->type;
          qcom->value = qcomexist->value;
          qcom->location = qcomexist->location;

          if (quest->out == 0)
            quest->out = qcom;
          else
          {
            qlast = quest->out;
            while (qlast->next)
              qlast = qlast->next;
            qlast->next = qcom;
          }
        } /* end out chain */

      } /* end if qexist */
    }   /* for loop */
  }
  /*
   * Attach reference to quest to player's struct descriptor_data.
   */
  OLC_HLQUEST(d) = quest;
  OLC_QCOM(d) = 0;
  OLC_QUESTENTRY(d) = 0;
  OLC_VAL(d) = 0;
  OLC_MOB(d) = ch;
  hlqedit_disp_menu(d);
}

/* utility function that finds quest entry with given num */
struct quest_entry *getquest(struct descriptor_data *d, int num)
{
  struct quest_entry *quest;
  int a = 1;

  for (quest = OLC_HLQUEST(d); quest; quest = quest->next)
  {
    if (a == num)
      return quest;
    a++;
  }
  return NULL;
}

/* utility function, links quest chain*/
void hlqedit_addtoout(struct descriptor_data *d, struct quest_command *qcom)
{
  struct quest_command *qlast;

  if (!OLC_QUESTENTRY(d)->out)
    OLC_QUESTENTRY(d)->out = qcom;
  else
  {
    qlast = OLC_QUESTENTRY(d)->out;
    while (qlast->next)
      qlast = qlast->next;
    qlast->next = qcom;
  }
  OLC_QCOM(d) = qcom;
}
/*------------------------------------------------------------------------*/
/*  saving/loading related functions */
/*------------------------------------------------------------------------*/

/* note loading the quests from file is in hlquest.c */

/* our saving function, comes from a deprecated system that added
   a list of zone info that needed to be saved, which you then would
   save with a seperate command - it is now changed to autosave to
   disk whenever its saved internally
 */
void hlqedit_save_internally(struct descriptor_data *d)
{
  struct char_data *i = NULL;
  struct char_data *ch = NULL;

  ch = OLC_MOB(d);

  /* quest mobs have to be wiped off the planet */
  for (i = character_list; i; i = i->next)
  {
    if (!IS_NPC(i))
      continue;
    if (GET_MOB_VNUM(i) == GET_MOB_VNUM(ch))
      extract_char(i);
  }

  free_hlquest(ch);
  ch->mob_specials.quest = OLC_HLQUEST(d);

  /* going ahead and saving to disk now -zusuk */
  hlqedit_save_to_disk(OLC_ZNUM(d));
}

/* writing the hlq file to disk
 */
void hlqedit_save_to_disk(int zone_num)
{
  FILE *fp;
  struct char_data *ch;
  struct quest_entry *quest;
  struct quest_command *qcom;
  char command = 'Q';
  int zone;
  int top;
  int i;
  int rmob_num;
  char buf[MAX_INPUT_LENGTH] = {'\0'};
  char buf2[MAX_INPUT_LENGTH] = {'\0'};

  if (zone_num < 0 || zone_num > top_of_zone_table)
  {
    log("SYSERR: hlqedit_save_to_disk: Invalid real zone passed!");
    return;
  }

  snprintf(buf, sizeof(buf), "%s/%d.new", HLQST_PREFIX, zone_table[zone_num].number);
  if (!(fp = fopen(buf, "w+")))
  {
    log("SYSERR: OLC: Cannot open hl quest file!");
    return;
  }

  zone = zone_table[zone_num].number;
  //zone = zone_table[zone_num].bot;
  top = zone_table[zone_num].top;

  /*
   * Search the database for mobs with quests in this zone and save them.
   */
  //for (i = zone; i <= top; i++) {
  for (i = zone * 100; i <= top; i++)
  {
    if ((rmob_num = real_mobile(i)) != NOWHERE)
    {
      ch = &mob_proto[rmob_num];
      if (ch->mob_specials.quest)
      {
        if (fprintf(fp, "#%d\n", i) < 0)
        {
          log("SYSERR: OLC: Cannot write hl quest file!\r\n");
          fclose(fp);
          return;
        }
        for (quest = ch->mob_specials.quest; quest; quest = quest->next)
        {
          switch (quest->type)
          {
          case QUEST_ASK:
            command = 'A';
            break;
          case QUEST_GIVE:
            command = 'Q';
            break;
          case QUEST_ROOM:
            command = 'R';
            break;
          }
          if (quest->approved)
            fprintf(fp, "%c!\n", command);
          else
            fprintf(fp, "%c\n", command);
          if (quest->type == QUEST_ASK)
            fprintf(fp, "%s~\n", quest->keywords);
          if (quest->type == QUEST_ROOM)
            fprintf(fp, "%d\n", quest->room);
          fprintf(fp, "%s~\n", quest->reply_msg);
          if (quest->type == QUEST_GIVE || quest->type == QUEST_ROOM)
          {
            for (qcom = quest->in; qcom; qcom = qcom->next)
              fprintf(fp, "I %c %d %d\n", hlqedit_command[qcom->type], qcom->value,
                      qcom->location);
            for (qcom = quest->out; qcom; qcom = qcom->next)
              fprintf(fp, "O %c %d %d\n", hlqedit_command[qcom->type],
                      qcom->value, qcom->location);
            fprintf(fp, "S\n");
          }
        }
      }
    }
  }

  fprintf(fp, "$~\n");
  fclose(fp);
  snprintf(buf2, sizeof(buf2), "%s/%d.hlq", HLQST_PREFIX, zone_table[zone_num].number);
  /*
   * We're fubar'd if we crash between the two lines below.
   */
  remove(buf2);
  rename(buf, buf2);

  /*
   * Since quests were not in from start, make sure that they are in index file
   * index files now do not add duplicates (Vhaerun)
   */
  zedit_create_index(zone_table[zone_num].number);
}
/*------------------------------------------------------------------------*/

/**************************************************************************
 Menu functions 
 **************************************************************************/

/*
 * For extra descriptions.
 */
void hlqedit_disp_incommand_menu(struct descriptor_data *d)
{
  char buf[MAX_INPUT_LENGTH] = {'\0'};

  snprintf(buf, sizeof(buf),
          "\r\nQuest-Give Menu\r\n"
          "%sC%s) Give Coins to Mob\r\n"
          "%sI%s) Give Item to Mob\r\n",
          grn, nrm,
          grn, nrm);

  strlcat(buf, "Enter choice (0 to end/quit):  ", sizeof(buf));
  send_to_char(d->character, buf);
  OLC_MODE(d) = HLQEDIT_INCOMMAND;
}

void hlqedit_disp_outcommand_menu(struct descriptor_data *d)
{
  char buf[MAX_INPUT_LENGTH] = {'\0'};

  snprintf(buf, sizeof(buf),
          "\r\nQuest-Out Menu (Quest Rewards)\r\n"
          "%sC%s) Give coins\r\n"
          "%sI%s) Give item\r\n"
          "%sO%s) Load object in a room\r\n"
          "%sM%s) Load mob in a room\r\n"
          "%sA%s) Attack questor\r\n"
          "%sD%s) Disappear\r\n"
          "%sT%s) Teach spell/skill (not yet implemented)\r\n"
          "%sX%s) Open door in a room\r\n"
          "%sF%s) Follow questor\r\n"
          "%sU%s) Set Church (not yet implemented)\r\n"
          "%sK%s) Change Kit (not yet implemented)\r\n"
          "%sS%s) Cast Spell\r\n",
          grn, nrm,
          grn, nrm,
          grn, nrm,
          grn, nrm,
          grn, nrm,
          grn, nrm,
          grn, nrm,
          grn, nrm,
          grn, nrm,
          grn, nrm,
          grn, nrm,
          grn, nrm);

  strlcat(buf, "Enter choice (0 to end/quit):  ", sizeof(buf));
  send_to_char(d->character, buf);
  OLC_MODE(d) = HLQEDIT_OUTCOMMANDMENU;
}

void hlqedit_disp_spells(struct descriptor_data *d)
{
  char buf[MAX_INPUT_LENGTH] = {'\0'};

  int counter, columns = 0;

  get_char_colors(d->character);
  clear_screen(d);

  for (counter = 0; counter < NUM_SPELLS; counter++)
  {
    snprintf(buf, sizeof(buf), "%s%2d%s) %s%-20.20s %s", grn, counter, nrm, yel,
            spell_info[counter].name, !(++columns % 3) ? "\r\n" : "");
    send_to_char(d->character, buf);
  }
  snprintf(buf, sizeof(buf), "\r\n%sEnter spell choice (0 for none):  ", nrm);
  send_to_char(d->character, buf);
}

/*
 * The main menu.
 */
void hlqedit_disp_menu(struct descriptor_data *d)
{
  char buf[MAX_INPUT_LENGTH] = {'\0'};
  int num = 1;
  struct quest_entry *quest;

  get_char_colors(d->character);

  /* If a new entry has been writtem, add it to quest chain*/
  if (OLC_QUESTENTRY(d))
  {
    OLC_QUESTENTRY(d)->next = OLC_HLQUEST(d);
    OLC_HLQUEST(d) = OLC_QUESTENTRY(d);
    OLC_QUESTENTRY(d) = 0;
  }

  snprintf(buf, sizeof(buf), "\r\n---- Quests for %s (vnum: %d)\r\n", GET_NAME(OLC_MOB(d)),
          GET_MOB_VNUM(OLC_MOB(d)));
  send_to_char(d->character, buf);

  for (quest = OLC_HLQUEST(d); quest; quest = quest->next)
  {
    if (quest->type == QUEST_ASK)
      snprintf(buf, sizeof(buf), "%d) (%s) ASK %s", num, quest->approved ? "OK" : "-", quest->keywords);
    else if (quest->type == QUEST_ROOM)
      snprintf(buf, sizeof(buf), "%d) (%s) ROOM %d", num, quest->approved ? "OK" : "-", quest->room);
    else
    {
      if (quest->in > 0)
      {
        if (quest->in->type == QUEST_COMMAND_ITEM)
          snprintf(buf, sizeof(buf), "%d) (%s) GIVE %s", num,
                  quest->approved ? "OK" : "-",
                  obj_proto[real_object(quest->in->value)].short_description);
        else
          snprintf(buf, sizeof(buf), "%d) (%s) GIVE %d coins", num,
                  quest->approved ? "OK" : "-",
                  quest->in->value);

        if (quest->in->next)
          strlcat(buf, " etc..", sizeof(buf));
      }
    }
    strlcat(buf, "\r\n", sizeof(buf));
    send_to_char(d->character, buf);

    num++;
  }

  snprintf(buf, sizeof(buf),
          "\r\nMain Menu\r\n"
          "%sA%s) Approve quest\r\n"
          "%sN%s) Add new quest for the mob\r\n"
          "%sD%s) Delete quest\r\n"
          "%sV%s) View quest details\r\n"
          "%sQ%s) Quit\r\n"
          "Enter choice : ",

          grn, nrm,
          grn, nrm,
          grn, nrm,
          grn, nrm,
          grn, nrm);
  send_to_char(d->character, buf);

  OLC_MODE(d) = HLQEDIT_MAIN_MENU;
}

/* message displayed upon finishing a quest's step
 */
void hlqedit_init_replymsg(struct descriptor_data *d)
{
  char *msg = NULL;

  OLC_MODE(d) = HLQEDIT_REPLYMSG;
  write_to_output(d, "Enter reply message on quest:\r\n");
  send_editor_help(d);

  if (OLC_QUESTENTRY(d)->reply_msg)
  {
    write_to_output(d, "%s", OLC_QUESTENTRY(d)->reply_msg);
    msg = strdup(OLC_QUESTENTRY(d)->reply_msg);
  }
  string_write(d, &OLC_QUESTENTRY(d)->reply_msg, MAX_ROOM_DESC, 0, msg);
  OLC_VAL(d) = 1;
}

/**************************************************************************
  The main loop
 **************************************************************************/

void hlqedit_parse(struct descriptor_data *d, char *arg)
{
  int i;
  struct quest_entry *quest;
  struct quest_entry *qtmp;
  struct quest_command *qcom;
  int number = 1;
  char buf[MAX_INPUT_LENGTH] = {'\0'};

  switch (OLC_MODE(d))
  {

  case HLQEDIT_CONFIRM_HLSAVESTRING:
    d->str = 0;
    switch (*arg)
    {
    case 'y':
    case 'Y':
      hlqedit_save_internally(d);
      hlqedit_save_to_disk(real_zone_by_thing(OLC_NUM(d)));
      snprintf(buf, sizeof(buf), "OLC: %s edits hl quest %d.", GET_NAME(d->character), OLC_NUM(d));
      log(buf);
      OLC_MOB(d) = 0;
      cleanup_olc(d, CLEANUP_STRUCTS);
      send_to_char(d->character, "HL Quest saved to memory and disk.\r\n");
      break;
    case 'n':
    case 'N':
      /*
           * Free everything up, including strings, etc.
           */
      OLC_MOB(d) = 0;
      cleanup_olc(d, CLEANUP_ALL);
      break;
    default:
      send_to_char(d->character, "Invalid choice!\r\nDo you wish to "
                                 "save this hl quest internally?:  ");
      break;
    }
    return;

  case HLQEDIT_NEWCOMMAND:
    OLC_VAL(d) = 1;
    switch (*arg)
    {
    case 'g':
    case 'G':
      OLC_QUESTENTRY(d)->type = QUEST_GIVE;
      hlqedit_disp_incommand_menu(d);
      return;
    case 'r':
    case 'R':
      OLC_QUESTENTRY(d)->type = QUEST_ROOM;
      OLC_MODE(d) = HLQEDIT_ROOM;
      send_to_char(d->character, "Room Quest is a quest that requires the"
                                 " questor to bring the quest-mobile to a given room.\r\n");
      send_to_char(d->character, "Which room to trigger in (vnum)?:  ");
      return;
      break;
    case 'a':
    case 'A':
      OLC_QUESTENTRY(d)->type = QUEST_ASK;
      OLC_MODE(d) = HLQEDIT_KEYWORDS;
      send_to_char(d->character, "Enter Keywords >");
      return;
    }
    send_to_char(d->character, "\r\nInvalid choice!\r\nWhat type of quest"
                               " entry (G)ive, (R)oom or (A)sk?  ");
    break;
  case HLQEDIT_KEYWORDS:
    OLC_VAL(d) = 1;
    if (OLC_QUESTENTRY(d)->keywords)
      free(OLC_QUESTENTRY(d)->keywords);
    OLC_QUESTENTRY(d)->keywords = strdup((arg && *arg) ? arg : "hi hello");
    OLC_MODE(d) = HLQEDIT_REPLYMSG;
    hlqedit_init_replymsg(d);
    return;

    break;

  case HLQEDIT_REPLYMSG:
    /*
       * We will NEVER get here, we hope.
       */
    log("SYSERR: Reached HLQEDIT_REPLYMSG case in parse_hlqedit");
    break;

  case HLQEDIT_ROOM:
    number = atoi(arg);
    if (number && real_room(number) != NOWHERE)
    {
      OLC_QUESTENTRY(d)->room = number;
      hlqedit_disp_outcommand_menu(d);
      return;
    }
    break;

  case HLQEDIT_INCOMMAND:
  {
    switch (*arg)
    {
    case 'c':
    case 'C':
      CREATE(qcom, struct quest_command, 1);
      qcom->next = OLC_QUESTENTRY(d)->in;
      OLC_QUESTENTRY(d)->in = qcom;
      OLC_MODE(d) = HLQEDIT_IN_COIN;
      qcom->type = QUEST_COMMAND_COINS;
      send_to_char(d->character, "How many coins?  ");
      return;
    case 'i':
    case 'I':
      CREATE(qcom, struct quest_command, 1);
      qcom->next = OLC_QUESTENTRY(d)->in;
      OLC_QUESTENTRY(d)->in = qcom;
      qcom->type = QUEST_COMMAND_ITEM;
      OLC_MODE(d) = HLQEDIT_IN_ITEM;
      send_to_char(d->character, "Select item (vnum)?  ");
      return;
    case '0':
    case 'E':
    case 'e':
      hlqedit_disp_outcommand_menu(d);
      return;
    }
  }
  break;

  case HLQEDIT_IN_COIN:
    number = atoi(arg);
    if (number < 0 || number > 100000)
      send_to_char(d->character, "Invalid choice! (0-100000)\r\n");
    else
    {
      OLC_QUESTENTRY(d)->in->value = number;
      hlqedit_disp_incommand_menu(d);
    }
    return;
    break;
  case HLQEDIT_IN_ITEM:
    if ((number = real_object(atoi(arg))) != NOWHERE)
    {
      OLC_QUESTENTRY(d)->in->value = atoi(arg);
      hlqedit_disp_incommand_menu(d);
    }
    else
      send_to_char(d->character, "That object does not exist, try again:  ");
    return;
    break;

  case HLQEDIT_OUTCOMMANDMENU:
    switch (*arg)
    {
    case 'c':
    case 'C':
      CREATE(qcom, struct quest_command, 1);
      hlqedit_addtoout(d, qcom);
      OLC_MODE(d) = HLQEDIT_OUT_COIN;
      qcom->type = QUEST_COMMAND_COINS;
      send_to_char(d->character, "How many coins?  ");
      return;
      break;
    case 'i':
    case 'I':
      CREATE(qcom, struct quest_command, 1);
      hlqedit_addtoout(d, qcom);
      qcom->type = QUEST_COMMAND_ITEM;
      OLC_MODE(d) = HLQEDIT_OUT_ITEM;
      send_to_char(d->character, "Select item (vnum)?  ");
      return;
      break;
    case 'o':
    case 'O':
      CREATE(qcom, struct quest_command, 1);
      hlqedit_addtoout(d, qcom);
      qcom->type = QUEST_COMMAND_LOAD_OBJECT_INROOM;
      OLC_MODE(d) = HLQEDIT_OUT_LOAD_OBJECT;
      send_to_char(d->character, "Select item (vnum)?  ");
      return;
      break;
    case 'm':
    case 'M':
      CREATE(qcom, struct quest_command, 1);
      hlqedit_addtoout(d, qcom);
      qcom->type = QUEST_COMMAND_LOAD_MOB_INROOM;
      OLC_MODE(d) = HLQEDIT_OUT_LOAD_MOB;
      send_to_char(d->character, "Select mob (vnum)?  ");
      return;
    case 't':
    case 'T':
      CREATE(qcom, struct quest_command, 1);
      hlqedit_addtoout(d, qcom);
      qcom->type = QUEST_COMMAND_TEACH_SPELL;
      OLC_MODE(d) = HLQEDIT_OUT_TEACH_SPELL;
      hlqedit_disp_spells(d);
      return;

    case 'x':
    case 'X':
      CREATE(qcom, struct quest_command, 1);
      hlqedit_addtoout(d, qcom);
      qcom->type = QUEST_COMMAND_OPEN_DOOR;
      OLC_MODE(d) = HLQEDIT_OUT_OPEN_DOOR;
      send_to_char(d->character, "Select room (vnum)?  ");
      return;

      break;

    case 'a':
    case 'A':
      CREATE(qcom, struct quest_command, 1);
      hlqedit_addtoout(d, qcom);
      qcom->type = QUEST_COMMAND_ATTACK_QUESTOR;
      hlqedit_init_replymsg(d);
      return;

      break;

    case 'd':
    case 'D':
      CREATE(qcom, struct quest_command, 1);
      hlqedit_addtoout(d, qcom);
      qcom->type = QUEST_COMMAND_DISAPPEAR;
      hlqedit_init_replymsg(d);
      return;

      break;

    case 'f':
    case 'F':
      CREATE(qcom, struct quest_command, 1);
      hlqedit_addtoout(d, qcom);
      qcom->type = QUEST_COMMAND_FOLLOW;
      send_to_char(d->character, "\r\nQuest-mobile now set to follow"
                                 " questor.\r\n");
      hlqedit_disp_outcommand_menu(d);
      return;

      break;

    case 'k':
    case 'K':
      CREATE(qcom, struct quest_command, 1);
      hlqedit_addtoout(d, qcom);
      qcom->type = QUEST_COMMAND_KIT;
      OLC_MODE(d) = HLQEDIT_OUT_KIT_SELECT;
      hlqedit_show_classes(d);
      send_to_char(d->character, "Select Kit:  ");
      return;

    case 'u':
    case 'U':
      CREATE(qcom, struct quest_command, 1);
      hlqedit_addtoout(d, qcom);
      qcom->type = QUEST_COMMAND_CHURCH;
      OLC_MODE(d) = HLQEDIT_OUT_CHURCH;
      for (i = 0; i < NUM_CHURCHES; i++)
      {
        snprintf(buf, sizeof(buf), "%3d)%20s \r\n", i, church_types[i]);
        send_to_char(d->character, buf);
      }
      send_to_char(d->character, "Select church:  ");
      return;
    case 's':
    case 'S':
      CREATE(qcom, struct quest_command, 1);
      hlqedit_addtoout(d, qcom);
      qcom->type = QUEST_COMMAND_CAST_SPELL;
      OLC_MODE(d) = HLQEDIT_OUT_TEACH_SPELL; //same no need for new.
      hlqedit_disp_spells(d);
      return;

      break;

    case '0':
    case 'E':
    case 'e':
      hlqedit_init_replymsg(d);
      return;

      break;
    } /* end out command arg switch */
    break;

  case HLQEDIT_OUT_COIN:
    number = atoi(arg);
    if (number < 0 || number > 100000)
      send_to_char(d->character, "That is not a valid choice! (0 - 100000)\r\n");
    else
    {
      OLC_QCOM(d)->value = number;
      hlqedit_disp_outcommand_menu(d);
    }
    return;

    break;

  case HLQEDIT_OUT_ITEM:
    if ((number = real_object(atoi(arg))) != NOTHING)
    {
      OLC_QCOM(d)->value = atoi(arg);
      hlqedit_disp_outcommand_menu(d);
    }
    else
      send_to_char(d->character, "That object does not exist, try again:  ");
    return;

    break;

  case HLQEDIT_OUT_LOAD_OBJECT:
    if ((number = real_object(atoi(arg))) != NOTHING)
    {
      OLC_QCOM(d)->value = atoi(arg);
      OLC_MODE(d) = HLQEDIT_OUT_LOAD_OBJECT_ROOM;
      send_to_char(d->character, "Which room to load it (vnum). (0 for current room):\r\n");
    }
    else
      send_to_char(d->character, "That object does not exist, try again:  ");
    return;

    break;

  case HLQEDIT_OUT_LOAD_MOB:
    if ((number = real_mobile(atoi(arg))) != NOBODY)
    {
      OLC_QCOM(d)->value = atoi(arg);
      OLC_MODE(d) = HLQEDIT_OUT_LOAD_MOB_ROOM;
      send_to_char(d->character, "Which room to load it (vnum). (0 for current room): ");
    }
    else
      send_to_char(d->character, "That mob does not exist, try again:  ");
    return;

    break;

  case HLQEDIT_OUT_TEACH_SPELL:
    number = atoi(arg);
    if (number > 0 && number < MAX_SKILLS)
    {
      OLC_QCOM(d)->value = atoi(arg);
      hlqedit_disp_outcommand_menu(d);
    }
    else
      send_to_char(d->character, "That spell/skill does not exist, try again:  ");
    return;
  case HLQEDIT_OUT_LOAD_OBJECT_ROOM:
  case HLQEDIT_OUT_LOAD_MOB_ROOM:
    if ((number = real_room(atoi(arg))) != NOWHERE)
      OLC_QCOM(d)->location = atoi(arg);
    else
    {
      OLC_QCOM(d)->location = 0;
      send_to_char(d->character, "Invalid room, defaulting to room 0"
                                 " (which is current mobile room).\r\n");
    }
    hlqedit_disp_outcommand_menu(d);
    return;

    break;

  case HLQEDIT_OUT_CHURCH:
    number = atoi(arg);
    if (number >= 0 && number < NUM_CHURCHES)
    {
      OLC_QCOM(d)->value = number;
      hlqedit_disp_outcommand_menu(d);
      return;
    }
    break;

  case HLQEDIT_OUT_KIT_SELECT:
    number = atoi(arg);
    if (number >= 0 && number < NUM_CLASSES)
    {
      OLC_QCOM(d)->value = number;
      OLC_MODE(d) = HLQEDIT_OUT_KIT_PREREQ;
      hlqedit_show_classes(d);
      send_to_char(d->character, "Select Prerequisite:  ");
      return;
    }
    break;
  case HLQEDIT_OUT_KIT_PREREQ:
    number = atoi(arg);
    if (number >= 0 && number < NUM_CLASSES)
    {
      OLC_QCOM(d)->location = number;
      hlqedit_disp_outcommand_menu(d);
      return;
    }
    break;

  case HLQEDIT_OUT_OPEN_DOOR:
    if ((number = real_room(atoi(arg))) != NOWHERE)
    {
      OLC_QCOM(d)->location = atoi(arg);
      send_to_char(d->character, "Which direction? (0 = North, 1 = East, "
                                 "2 = South, 3 = West, 4 = Up, 5 = Down):  ");

      OLC_MODE(d) = HLQEDIT_OUT_OPEN_DOOR_DIR;
    }
    else
    {
      send_to_char(d->character, "Which room to open door in (vnum)?:  ");
    }
    return;

    break;

  case HLQEDIT_OUT_OPEN_DOOR_DIR:
    if (atoi(arg) > -1 && atoi(arg) < 6)
    {
      OLC_QCOM(d)->value = atoi(arg);
      hlqedit_disp_outcommand_menu(d);
    }
    else
    {
      send_to_char(d->character, "Which direction? (0 = North, 1 = East, "
                                 "2 = South, 3 = West, 4 = Up, 5 = Down): ");
    }
    return;

    break;

  case HLQEDIT_DELETE_QUEST:
  {
    OLC_VAL(d) = 1;
    number = atoi(arg);
    if (number < 1 || NULL == (quest = getquest(d, number)))
      send_to_char(d->character, "No such quest!\r\n");
    else
    {
      if (number == 1)
      {
        OLC_HLQUEST(d) = OLC_HLQUEST(d)->next;
      }
      else
      {
        for (qtmp = OLC_HLQUEST(d); qtmp; qtmp = qtmp->next)
        {
          if (qtmp->next == quest)
            qtmp->next = quest->next;
        }
      }
      quest->next = NULL;
      free_hlquests(quest);
    }
    hlqedit_disp_menu(d);
    return;
  }
  break;

  case HLQEDIT_APPROVE_QUEST:
    number = atoi(arg);
    if (number < 1 || NULL == (quest = getquest(d, number)))
      send_to_char(d->character, "No such quest!\r\n");
    else if (quest->approved == TRUE)
    {
      quest->approved = FALSE;
      send_to_char(d->character, "\r\nQUEST UN-APPROVED!\r\n");
    }
    else
    {
      quest->approved = TRUE;
      send_to_char(d->character, "\r\nQUEST APPROVED!\r\n");
    }
    hlqedit_disp_menu(d);
    return;

    break;

  case HLQEDIT_VIEW_QUEST:
  {
    number = atoi(arg);
    if (number < 1 || NULL == (quest = getquest(d, number)))
      send_to_char(d->character, "No such quest!\r\n");
    else
      show_quest_to_player(d->character, quest);
    break;
  }

  case HLQEDIT_MAIN_MENU:
    switch (*arg)
    {
    case 'q':
    case 'Q':
      if (OLC_VAL(d))
      { /* Something has been modified. */
        send_to_char(d->character, "Do you wish to save this quest internally?:  ");
        OLC_MODE(d) = HLQEDIT_CONFIRM_HLSAVESTRING;
      }
      else
      {
        OLC_MOB(d) = 0;
        cleanup_olc(d, CLEANUP_ALL);
      }
      return;
    case 'a':
    case 'A':
      if (GET_LEVEL(d->character) < LVL_GRSTAFF)
        send_to_char(d->character, "You are not high enough level to do"
                                   " that!\r\n");
      else
      {
        OLC_VAL(d) = 1;
        OLC_MODE(d) = HLQEDIT_APPROVE_QUEST;
        send_to_char(d->character, "Select which quest to approve\r\n");
      }
      return;
    case 'n':
    case 'N':
      CREATE(OLC_QUESTENTRY(d), struct quest_entry, 1);
      clear_hlquest(OLC_QUESTENTRY(d));
      OLC_MODE(d) = HLQEDIT_NEWCOMMAND;
      send_to_char(d->character, "What type of quest entry (G)ive, (R)oom or (A)sk?  ");
      return;
    case 'd':
    case 'D':
      OLC_MODE(d) = HLQEDIT_DELETE_QUEST;
      send_to_char(d->character, "Select which quest to delete:\r\n");
      return;
    case 'v':
    case 'V':
      OLC_MODE(d) = HLQEDIT_VIEW_QUEST;
      send_to_char(d->character, "Select which quest to view:\r\n");
      return;
    }
    break;

  default:
    /*
       * We should never get here.
       */
    log("SYSERR: Reached default case in parse_hlqedit");
    break;
  }
  /*
   * If we get this far, something has been changed.
   */
  OLC_VAL(d) = 1;
  hlqedit_disp_menu(d);
}

/* end main loop */

/*-----------------------------------------------------*/
/* commands code */
/*-----------------------------------------------------*/

/* entry point for hl quest editor */
ACMD(do_hlqedit)
{
  int number = NOBODY, save = 0, real_num;
  struct descriptor_data *d;
  const char *buf3;
  char buf2[MAX_INPUT_LENGTH];
  char buf1[MAX_INPUT_LENGTH];

  //No screwing around as a mobile.
  if (IS_NPC(ch) || !ch->desc || STATE(ch->desc) != CON_PLAYING)
    return;

  // parse arguments
  buf3 = two_arguments(argument, buf1, sizeof(buf1), buf2, sizeof(buf2));

  // no argument
  if (!*buf1)
  {
    send_to_char(ch, "Specify a hlquest VNUM to edit.\r\n");
    return;
  }
  else if (!isdigit(*buf1))
  {
    if (str_cmp("save", buf1) != 0)
    {
      send_to_char(ch, "Yikes!  Stop that, someone will get hurt!\r\n");
      return;
    }

    save = TRUE;

    if (is_number(buf2))
      number = atoi(buf2);
    else if (GET_OLC_ZONE(ch) > 0)
    {
      zone_rnum zlok;

      if ((zlok = real_zone(GET_OLC_ZONE(ch))) == NOWHERE)
        number = NOWHERE;
      else
        number = genolc_zone_bottom(zlok);
    }

    if (number == NOWHERE)
    {
      send_to_char(ch, "Save which zone?\r\n");
      return;
    }
  }

  // if numberic arg was given, get it
  if (number == NOBODY)
    number = atoi(buf1);

  /* debug */
  send_to_char(ch, "Number inputed: %d\r\n", number);

  // make sure not already being editted
  for (d = descriptor_list; d; d = d->next)
  {
    if (STATE(d) == CON_HLQEDIT || STATE(d) == CON_MEDIT)
    {
      if (d->olc && OLC_NUM(d) == number)
      {
        send_to_char(ch, "That hlquest/mob is currently being edited by %s.\r\n",
                     PERS(d->character, ch));
        return;
      }
    }
  }
  d = ch->desc;

  // give descriptor olc struct
  if (d->olc)
  {
    mudlog(BRF, LVL_IMMORT, TRUE,
           "SYSERR: do_oasis_hlquest: Player already had olc structure.");
    free(d->olc);
  }
  CREATE(d->olc, struct oasis_olc_data, 1);

  /* Find the zone. */
  OLC_ZNUM(d) = save ? real_zone(number) : real_zone_by_thing(number);

  /* debug */
  send_to_char(ch, "OLC Zone Number: %d\r\n", OLC_ZNUM(d));

  if (OLC_ZNUM(d) == NOWHERE)
  {
    send_to_char(ch, "Sorry, there is no zone for that number!\r\n");
    free(d->olc);
    d->olc = NULL;
    return;
  }

  // everyone except imps can only edit zones they have been assigned
  if (!can_edit_zone(ch, OLC_ZNUM(d)))
  {
    send_to_char(ch, "You do not have permission to edit this zone.\r\n");

    free(d->olc);
    d->olc = NULL;
    return;
  }

  if (save)
  {
    send_to_char(ch, "Saving all hlquest info in zone %d.\r\n",
                 zone_table[OLC_ZNUM(d)].number);
    log("OLC: %s saves hlquest info for zone %d.", GET_NAME(ch),
        zone_table[OLC_ZNUM(d)].number);

    send_to_char(ch, "Saving to disk...");
    hlqedit_save_to_disk(OLC_ZNUM(d));
    /* Save the mobiles. */
    send_to_char(ch, "Saving mobiles...");
    save_mobiles(OLC_ZNUM(d));

    free(d->olc);
    d->olc = NULL;
    return;
  }

  OLC_NUM(d) = number;

  // take descriptor and start up subcommands
  if ((real_num = real_mobile(number)) == NOBODY)
  {
    send_to_char(ch, "No such mob to make a quest for!\r\n");

    free(d->olc);
    d->olc = NULL;
    return;
  }
  hlqedit_setup(d, real_num);
  STATE(d) = CON_HLQEDIT;

  act("$n starts using OLC (hlqedit).", TRUE, d->character, 0, 0, TO_ROOM);
  SET_BIT_AR(PLR_FLAGS(ch), PLR_WRITING);

  mudlog(BRF, LVL_IMMORT, TRUE,
         "OLC: %s starts editing zone %d allowed zone %d",
         GET_NAME(ch), zone_table[OLC_ZNUM(d)].number, GET_OLC_ZONE(ch));
}
