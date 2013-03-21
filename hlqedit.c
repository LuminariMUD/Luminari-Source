/* *************************************************************************
 *   File: hlqedit.c                                   Part of LuminariMUD *
 *  Usage: alternate quest system                                          *
 *  Author: Homeland (Vhaerun), ported to tba/luminari by Zusuk            *
 ************************************************************************* */


#include "conf.h"
#include "sysdep.h"
#include "structs.h" 
#include "comm.h"
#include "utils.h"
#include "db.h"
#include "boards.h"
#include "oasis.h" 
#include "interpreter.h"
#include "constants.h"
#include "hlquest.h"
#include "spells.h"
#include "class.h"
#include "genzon.h"
#include "genolc.h"

/*---------------------------------------------*/
/*. Function prototypes / Globals / Externals. */
/*---------------------------------------------*/

extern struct room_data *world;
extern struct char_data *mob_proto;
extern struct zone_data *zone_table;
extern struct obj_data *obj_proto;
extern struct index_data *mob_index;

char *hlqedit_command = "CIOMADTXFKUS";

void hlqedit_disp_menu(struct descriptor_data *d);

/*****************************************************************************/

/*---------------------------------------------*/
/*. local functions                            */

/*---------------------------------------------*/


void zedit_create_index(int znum, char *type) {
  FILE *newfile, *oldfile;
  char new_name[32], old_name[32], *prefix;
  int num, found = FALSE;
  char buf1[MAX_INPUT_LENGTH] = {'\0'};
  char buf[MAX_INPUT_LENGTH] = {'\0'};

  switch (*type) {
    case 'z':
      prefix = ZON_PREFIX;
      break;
    case 'w':
      prefix = WLD_PREFIX;
      break;
    case 'o':
      prefix = OBJ_PREFIX;
      break;
    case 'm':
      prefix = MOB_PREFIX;
      break;
    case 's':
      prefix = SHP_PREFIX;
      break;
    case 'q':
      prefix = QST_PREFIX;
      break;
    default:
      /*
       * Caller messed up  
       */
      return;
  }

  sprintf(old_name, "%s/index", prefix);
  sprintf(new_name, "%s/newindex", prefix);

  if (!(oldfile = fopen(old_name, "r"))) {
    sprintf(buf, "SYSERR: OLC: Failed to open %s", buf);
    log(buf);
    return;
  } else if (!(newfile = fopen(new_name, "w"))) {
    sprintf(buf, "SYSERR: OLC: Failed to open %s", buf);
    log(buf);
    return;
  }

  /*
   * Index contents must be in order: search through the old file for the
   * right place, insert the new file, then copy the rest over. 
   */
  sprintf(buf1, "%d.%s", znum, type);
  while (get_line(oldfile, buf)) {
    if (*buf == '$') {
      fprintf(newfile, "%s\n$\n", (!found ? buf1 : ""));
      break;
    } else if (!found) {
      sscanf(buf, "%d", &num);
      if (num == znum)
        found = TRUE;
      if (num > znum) {
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

void hlqedit_show_classes(struct descriptor_data *d) {
  char buf[MAX_INPUT_LENGTH] = {'\0'};
  int i;

  for (i = 0; i < NUM_CLASSES; i++) {
    sprintf(buf, "%d) %s\r\n", i, pc_class_types[i]);
    send_to_char(d->character, buf);
  }
}

void hlqedit_setup(struct descriptor_data *d, int mob) {
  struct quest_entry *quest = 0;
  struct quest_entry *qexist;
  struct quest_entry *qtmp;
  struct quest_command *qcom;
  struct quest_command *qlast;
  struct quest_command *qcomexist;
  struct char_data *ch;

  ch = &mob_proto[mob];

  /*
     Okies copy what already exist
   */
  if (ch->mob_specials.quest) {

    for (qexist = ch->mob_specials.quest; qexist; qexist = qexist->next) {

      CREATE(qtmp, struct quest_entry, 1);
      clear_hlquest(qtmp);
      qtmp->type = qexist->type;
      qtmp->approved = qexist->approved;
      qtmp->reply_msg = strdup(qexist->reply_msg);
      if (qtmp->type == QUEST_ASK)
        qtmp->keywords = strdup(qexist->keywords);
      qtmp->room = qexist->room;

      qtmp->next = quest;
      quest = qtmp;
      /* in chain */
      for (qcomexist = qexist->in; qcomexist; qcomexist = qcomexist->next) {
        CREATE(qcom, struct quest_command, 1);
        qcom->type = qcomexist->type;
        qcom->value = qcomexist->value;
        qcom->location = qcomexist->location;
        qcom->next = quest->in;
        quest->in = qcom;
      }
      /* out chain */
      for (qcomexist = qexist->out; qcomexist; qcomexist = qcomexist->next) {
        CREATE(qcom, struct quest_command, 1);
        qcom->type = qcomexist->type;
        qcom->value = qcomexist->value;
        qcom->location = qcomexist->location;

        if (quest->out == 0)
          quest->out = qcom;
        else {
          qlast = quest->out;
          while (qlast->next != 0)
            qlast = qlast->next;
          qlast->next = qcom;
        }
      }
    }
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

/*------------------------------------------------------------------------*/


void hlqedit_save_internally(struct descriptor_data *d) {
  struct char_data *ch;
  ch = OLC_MOB(d);
  free_hlquest(ch);
  ch->mob_specials.quest = OLC_HLQUEST(d);
  /* homeland-port this has to be rewritten for luminari */
  //  olc_add_to_save_list(zone_table[OLC_ZNUM(d)].number, OLC_SAVE_QUEST);
}

/*------------------------------------------------------------------------*/

void hlqedit_save_to_disk(int zone_num) {
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

  if (zone_num < 0 || zone_num > top_of_zone_table) {
    log("SYSERR: hlqedit_save_to_disk: Invalid real zone passed!");
    return;
  }

  sprintf(buf, "%s/%d.new", QST_PREFIX, zone_table[zone_num].number);
  if (!(fp = fopen(buf, "w+"))) {
    log("SYSERR: OLC: Cannot open quest file!");
    return;
  }

  zone = zone_table[zone_num].number;
  top = zone_table[zone_num].top;

  /*
   * Search the database for mobs with quests in this zone and save them.
   */
  for (i = zone * 100; i <= top; i++) {
    if ((rmob_num = real_mobile(i)) != -1) {
      ch = &mob_proto[rmob_num];
      if (ch->mob_specials.quest) {
        if (fprintf(fp, "#%d\n", i) < 0) {
          log("SYSERR: OLC: Cannot write quest file!\r\n");
          fclose(fp);
          return;
        }
        for (quest = ch->mob_specials.quest; quest; quest = quest->next) {
          switch (quest->type) {
            case QUEST_ASK: command = 'A';
              break;
            case QUEST_GIVE: command = 'Q';
              break;
            case QUEST_ROOM: command = 'R';
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
          if (quest->type == QUEST_GIVE || quest->type == QUEST_ROOM) {
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
  sprintf(buf2, "%s/%d.hlq", QST_PREFIX, zone_table[zone_num].number);
  /*
   * We're fubar'd if we crash between the two lines below.
   */
  remove(buf2);
  rename(buf, buf2);

  /* homeland-port this has to be rewritten for luminari */
  //olc_remove_from_save_list(zone_table[zone_num].number, OLC_SAVE_QUEST);

  /*
   * Since quests were not in from start, make sure that they are in index file
   * index files now do not add duplicates (Vhaerun)
   */
  zedit_create_index(zone_table[zone_num].number, "hlq");
}

/**************************************************************************
 Menu functions 
 **************************************************************************/

/*
 * For extra descriptions.
 */
void hlqedit_disp_incommand_menu(struct descriptor_data *d) {
  char buf[MAX_INPUT_LENGTH] = {'\0'};

  sprintf(buf,
          "%sC%s) Give Coins to Mob\r\n"
          "%sI%s) Give Item to Mob\r\n",
          grn, nrm,
          grn, nrm);

  strcat(buf, "Enter choice (0 to end/quit) : ");
  send_to_char(d->character, buf);
  OLC_MODE(d) = QEDIT_INCOMMAND;
}

void hlqedit_disp_outcommand_menu(struct descriptor_data *d) {
  char buf[MAX_INPUT_LENGTH] = {'\0'};

  sprintf(buf,
          "%sC%s) Return Coin\r\n"
          "%sI%s) Return item\r\n"
          "%sO%s) Load object in a room\r\n"
          "%sM%s) Load mob in a room\r\n"
          "%sA%s) Attack questor\r\n"
          "%sD%s) Dissappear\r\n"
          "%sT%s) Teach spell/skill\r\n"
          "%sX%s) Open Door\r\n"
          "%sF%s) Follow questor\r\n"
          "%sU%s) Set Church\r\n"
          "%sK%s) Change Kit\r\n"
          "%sS%s) Cast Spell\r\n"
          ,
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

  strcat(buf, "Enter choice (0 to end/quit) : ");
  send_to_char(d->character, buf);
  OLC_MODE(d) = QEDIT_OUTCOMMANDMENU;
}

void hlqedit_disp_spells(struct descriptor_data *d) {
  char buf[MAX_INPUT_LENGTH] = {'\0'};

  int counter, columns = 0;

  get_char_colors(d->character);
  clear_screen(d);

  for (counter = 0; counter < NUM_SPELLS; counter++) {
    sprintf(buf, "%s%2d%s) %s%-20.20s %s", grn, counter, nrm, yel,
            spell_info[counter].name, !(++columns % 3) ? "\r\n" : "");
    send_to_char(d->character, buf);
  }
  sprintf(buf, "\r\n%sEnter spell choice (0 for none) : ", nrm);
  send_to_char(d->character, buf);
}

/*
 * The main menu.
 */
void hlqedit_disp_menu(struct descriptor_data *d) {
  char buf[MAX_INPUT_LENGTH] = {'\0'};
  int num = 1;

  struct quest_entry *quest;
  get_char_colors(d->character);

  /* If a new entry has been writtem, add it to quest chain*/
  if (OLC_QUESTENTRY(d)) {
    OLC_QUESTENTRY(d)->next = OLC_HLQUEST(d);
    OLC_HLQUEST(d) = OLC_QUESTENTRY(d);
    OLC_QUESTENTRY(d) = 0;
  }

  sprintf(buf, "\r\n----Quests for %s(%d)\r\n", GET_NAME(OLC_MOB(d)), GET_MOB_VNUM(OLC_MOB(d)));
  send_to_char(d->character, buf);

  for (quest = OLC_HLQUEST(d); quest; quest = quest->next) {
    if (quest->type == QUEST_ASK)
      sprintf(buf, "%d) (%s) ASK %s", num, quest->approved ? "OK" : "-", quest->keywords);
    else if (quest->type == QUEST_ROOM)
      sprintf(buf, "%d) (%s) ROOM %d", num, quest->approved ? "OK" : "-", quest->room);
    else {
      if (quest->in > 0) {
        if (quest->in->type == QUEST_COMMAND_ITEM)
          sprintf(buf, "%d) (%s) GIVE %s", num,
                quest->approved ? "OK" : "-",
                obj_proto[ real_object(quest->in->value)].short_description);
        else
          sprintf(buf, "%d) (%s) GIVE %d coins", num,
                quest->approved ? "OK" : "-",
                quest->in->value);

        if (quest->in->next)
          strcat(buf, " etc..");
      }
    }
    strcat(buf, "\r\n");
    send_to_char(d->character, buf);

    num++;
  }

  sprintf(buf,
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
          grn, nrm
          );
  send_to_char(d->character, buf);

  OLC_MODE(d) = QEDIT_MAIN_MENU;
}

void hlqedit_init_replymsg(struct descriptor_data *d) {
  char *msg;
  OLC_MODE(d) = QEDIT_REPLYMSG;
  write_to_output(d, "Enter reply message on quest: (/s saves /h for help)\r\n\r\n");
  d->backstr = NULL;
  if (OLC_QUESTENTRY(d)->reply_msg) {
    write_to_output(d, OLC_QUESTENTRY(d)->reply_msg);
    d->backstr = strdup(OLC_QUESTENTRY(d)->reply_msg);
  }

  CREATE(msg, char, MAX_ROOM_DESC);
  OLC_QUESTENTRY(d)->reply_msg = msg;
  d->str = &OLC_QUESTENTRY(d)->reply_msg;
  d->max_str = MAX_ROOM_DESC;
  d->mail_to = 0;
  OLC_VAL(d) = 1;
}

struct quest_entry *getquest(struct descriptor_data *d, int num) {
  struct quest_entry *quest;
  int a = 1;
  for (quest = OLC_HLQUEST(d); quest; quest = quest->next) {
    if (a == num)
      return quest;
    a++;
  }
  return NULL;
}

void hlqedit_addtoout(struct descriptor_data *d, struct quest_command *qcom) {
  struct quest_command *qlast;
  if (OLC_QUESTENTRY(d)->out == 0)
    OLC_QUESTENTRY(d)->out = qcom;
  else {
    qlast = OLC_QUESTENTRY(d)->out;
    while (qlast->next != 0)
      qlast = qlast->next;
    qlast->next = qcom;
  }
  OLC_QCOM(d) = qcom;
}

/**************************************************************************
  The main loop
 **************************************************************************/

void hlqedit_parse(struct descriptor_data *d, char *arg) {
  int i;
  struct quest_entry *quest;
  struct quest_entry *qtmp;
  struct quest_command *qcom;
  int number;
  char buf[MAX_INPUT_LENGTH] = {'\0'};

  switch (OLC_MODE(d)) {

    case QEDIT_CONFIRM_HLSAVESTRING:
      d->str = 0;
      switch (*arg) {
        case 'y':
        case 'Y':
          hlqedit_save_internally(d);
          sprintf(buf, "OLC: %s edits quest %d.", GET_NAME(d->character), OLC_NUM(d));
          log(buf);
          OLC_MOB(d) = 0;
          cleanup_olc(d, CLEANUP_STRUCTS);
          send_to_char(d->character, "Quest saved to memory.\r\n");
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
          send_to_char(d->character, "Invalid choice!\r\nDo you wish to save this quest internally? : ");
          break;
      }
      return;

    case QEDIT_NEWCOMMAND:
      OLC_VAL(d) = 1;
      switch (*arg) {
        case 'g':
        case 'G':
          OLC_QUESTENTRY(d)->type = QUEST_GIVE;
          hlqedit_disp_incommand_menu(d);
          return;
        case 'r':
        case 'R':
          OLC_QUESTENTRY(d)->type = QUEST_ROOM;
          OLC_MODE(d) = QEDIT_ROOM;
          send_to_char(d->character, "Which room to trigger in (num) ?");
          return;
          break;
        case 'a':
        case 'A':
          OLC_QUESTENTRY(d)->type = QUEST_ASK;
          OLC_MODE(d) = QEDIT_KEYWORDS;
          send_to_char(d->character, "Enter Keywords >");
          return;
      }
      send_to_char(d->character, "Invalid choice!\r\nWhat type of quest entry (G)ive, (R)oom or (A)sk?");
      break;
    case QEDIT_KEYWORDS:
      OLC_VAL(d) = 1;
      if (OLC_QUESTENTRY(d)->keywords)
        free(OLC_QUESTENTRY(d)->keywords);
      OLC_QUESTENTRY(d)->keywords = strdup((arg && *arg) ? arg : "hi hello");
      OLC_MODE(d) = QEDIT_REPLYMSG;
      hlqedit_init_replymsg(d);
      return;

    case QEDIT_REPLYMSG:
      /*
       * We will NEVER get here, we hope.
       */
      log("SYSERR: Reached QEDIT_REPLYMSG case in parse_hlqedit");
      break;

    case QEDIT_ROOM:
      number = atoi(arg);
      if (number) {
        OLC_QUESTENTRY(d)->room = number;
        hlqedit_disp_outcommand_menu(d);
        return;
      }
      break;

    case QEDIT_INCOMMAND:
      switch (*arg) {
        case 'c':
        case 'C':
          CREATE(qcom, struct quest_command, 1);
          qcom->next = OLC_QUESTENTRY(d)->in;
          OLC_QUESTENTRY(d)->in = qcom;
          OLC_MODE(d) = QEDIT_IN_COIN;
          qcom->type = QUEST_COMMAND_COINS;
          send_to_char(d->character, "How much coins (in copper)?");
          return;
        case 'i':
        case 'I':
          CREATE(qcom, struct quest_command, 1);
          qcom->next = OLC_QUESTENTRY(d)->in;
          OLC_QUESTENTRY(d)->in = qcom;
          qcom->type = QUEST_COMMAND_ITEM;
          OLC_MODE(d) = QEDIT_IN_ITEM;
          send_to_char(d->character, "Select item(vnum)?");
          return;
        case '0':
        case 'E':
        case 'e':
          hlqedit_disp_outcommand_menu(d);
          return;
      }
      break;

    case QEDIT_IN_COIN:
      number = atoi(arg);
      if (number < 0)
        send_to_char(d->character, "That is not a valid choice!\r\n");
      else {
        OLC_QUESTENTRY(d)->in->value = number;
        hlqedit_disp_incommand_menu(d);
      }
      return;
      break;
    case QEDIT_IN_ITEM:
      if ((number = real_object(atoi(arg))) >= 0) {
        OLC_QUESTENTRY(d)->in->value = atoi(arg);
        hlqedit_disp_incommand_menu(d);
      } else
        send_to_char(d->character, "That object does not exist, try again : ");
      return;
      break;


    case QEDIT_OUTCOMMANDMENU:
      switch (*arg) {
        case 'c':
        case 'C':
          CREATE(qcom, struct quest_command, 1);
          hlqedit_addtoout(d, qcom);
          OLC_MODE(d) = QEDIT_OUT_COIN;
          qcom->type = QUEST_COMMAND_COINS;
          send_to_char(d->character, "How much coins (in copper)?");
          return;
          break;
        case 'i':
        case 'I':
          CREATE(qcom, struct quest_command, 1);
          hlqedit_addtoout(d, qcom);
          qcom->type = QUEST_COMMAND_ITEM;
          OLC_MODE(d) = QEDIT_OUT_ITEM;
          send_to_char(d->character, "Select item(vnum)?");
          return;
          break;
        case 'o':
        case 'O':
          CREATE(qcom, struct quest_command, 1);
          hlqedit_addtoout(d, qcom);
          qcom->type = QUEST_COMMAND_LOAD_OBJECT_INROOM;
          OLC_MODE(d) = QEDIT_OUT_LOAD_OBJECT;
          send_to_char(d->character, "Select item(vnum)?");
          return;
          break;
        case 'm':
        case 'M':
          CREATE(qcom, struct quest_command, 1);
          hlqedit_addtoout(d, qcom);
          qcom->type = QUEST_COMMAND_LOAD_MOB_INROOM;
          OLC_MODE(d) = QEDIT_OUT_LOAD_MOB;
          send_to_char(d->character, "Select mob(vnum)?");
          return;
        case 't':
        case 'T':
          CREATE(qcom, struct quest_command, 1);
          hlqedit_addtoout(d, qcom);
          qcom->type = QUEST_COMMAND_TEACH_SPELL;
          OLC_MODE(d) = QEDIT_OUT_TEACH_SPELL;
          hlqedit_disp_spells(d);
          return;

        case 'x':
        case 'X':
          CREATE(qcom, struct quest_command, 1);
          hlqedit_addtoout(d, qcom);
          qcom->type = QUEST_COMMAND_OPEN_DOOR;
          OLC_MODE(d) = QEDIT_OUT_OPEN_DOOR;
          send_to_char(d->character, "Select room vnum?");
          return;

        case 'a':
        case 'A':
          CREATE(qcom, struct quest_command, 1);
          hlqedit_addtoout(d, qcom);
          qcom->type = QUEST_COMMAND_ATTACK_QUESTOR;
          hlqedit_init_replymsg(d);
          return;

        case 'd':
        case 'D':
          CREATE(qcom, struct quest_command, 1);
          hlqedit_addtoout(d, qcom);
          qcom->type = QUEST_COMMAND_DISAPPEAR;
          hlqedit_init_replymsg(d);
          return;

        case 'f':
        case 'F':
          CREATE(qcom, struct quest_command, 1);
          hlqedit_addtoout(d, qcom);
          qcom->type = QUEST_COMMAND_FOLLOW;
          hlqedit_disp_outcommand_menu(d);
          return;

        case 'k':
        case 'K':
          CREATE(qcom, struct quest_command, 1);
          hlqedit_addtoout(d, qcom);
          qcom->type = QUEST_COMMAND_KIT;
          OLC_MODE(d) = QEDIT_OUT_KIT_SELECT;
          hlqedit_show_classes(d);
          send_to_char(d->character, "Select Kit?");
          return;

        case 'u':
        case 'U':
          CREATE(qcom, struct quest_command, 1);
          hlqedit_addtoout(d, qcom);
          qcom->type = QUEST_COMMAND_CHURCH;
          OLC_MODE(d) = QEDIT_OUT_CHURCH;
          for (i = 0; i < NUM_CHURCHES; i++) {
            sprintf(buf, "%3d)%20s \r\n"
                    , i, church_types[i]);
            send_to_char(d->character, buf);
          }
          send_to_char(d->character, "Select church?");
          return;
        case 's':
        case 'S':
          CREATE(qcom, struct quest_command, 1);
          hlqedit_addtoout(d, qcom);
          qcom->type = QUEST_COMMAND_CAST_SPELL;
          OLC_MODE(d) = QEDIT_OUT_TEACH_SPELL; //same no need for new.
          hlqedit_disp_spells(d);
          return;

        case '0':
        case 'E':
        case 'e':
          hlqedit_init_replymsg(d);
          return;
      }
      break;

    case QEDIT_OUT_COIN:
      number = atoi(arg);
      if (number < 0)
        send_to_char(d->character, "That is not a valid choice!\r\n");
      else {
        OLC_QCOM(d)->value = number;
        hlqedit_disp_outcommand_menu(d);
      }
      return;
    case QEDIT_OUT_ITEM:
      if ((number = real_object(atoi(arg))) >= 0) {
        OLC_QCOM(d)->value = atoi(arg);
        hlqedit_disp_outcommand_menu(d);
      } else
        send_to_char(d->character, "That object does not exist, try again : ");
      return;

    case QEDIT_OUT_LOAD_OBJECT:
      if ((number = real_object(atoi(arg))) >= 0) {
        OLC_QCOM(d)->value = atoi(arg);
        OLC_MODE(d) = QEDIT_OUT_LOAD_OBJECT_ROOM;
        send_to_char(d->character, "Which room to load it. (-1 for current room)\r\n: ");
      } else
        send_to_char(d->character, "That object does not exist, try again : ");
      return;
    case QEDIT_OUT_LOAD_MOB:
      if ((number = real_mobile(atoi(arg))) >= 0) {
        OLC_QCOM(d)->value = atoi(arg);
        OLC_MODE(d) = QEDIT_OUT_LOAD_MOB_ROOM;
        send_to_char(d->character, "Which room to load it. (0 for current room)\r\n: ");
      } else
        send_to_char(d->character, "That mob does not exist, try again : ");
      return;

    case QEDIT_OUT_TEACH_SPELL:
      number = atoi(arg);
      if (number > 0 && number < MAX_SKILLS) {
        OLC_QCOM(d)->value = atoi(arg);
        hlqedit_disp_outcommand_menu(d);
      } else
        send_to_char(d->character, "That spell/skill does not exist, try again : ");
      return;
    case QEDIT_OUT_LOAD_OBJECT_ROOM:
    case QEDIT_OUT_LOAD_MOB_ROOM:
      if ((number = real_room(atoi(arg))) >= 0)
        OLC_QCOM(d)->location = atoi(arg);
      else
        OLC_QCOM(d)->location = -1;
      hlqedit_disp_outcommand_menu(d);
      return;

    case QEDIT_OUT_CHURCH:
      number = atoi(arg);
      if (number >= 0 && number < NUM_CHURCHES) {
        OLC_QCOM(d)->value = number;
        hlqedit_disp_outcommand_menu(d);
        return;
      }
      break;
    case QEDIT_OUT_KIT_SELECT:
      number = atoi(arg);
      if (number >= 0 && number < NUM_CLASSES) {
        OLC_QCOM(d)->value = number;
        OLC_MODE(d) = QEDIT_OUT_KIT_PREREQ;
        hlqedit_show_classes(d);
        send_to_char(d->character, "Select Prerequisit?");
        return;
      }
      break;
    case QEDIT_OUT_KIT_PREREQ:
      number = atoi(arg);
      if (number >= 0 && number < NUM_CLASSES) {
        OLC_QCOM(d)->location = number;
        hlqedit_disp_outcommand_menu(d);
        return;
      }

    case QEDIT_OUT_OPEN_DOOR:
      if ((number = real_room(atoi(arg))) >= 0) {
        OLC_QCOM(d)->location = atoi(arg);
        send_to_char(d->character, "Which direction? (0=n, 1=e, 2=s, 3=w, 4=u, 5=d): ");

        OLC_MODE(d) = QEDIT_OUT_OPEN_DOOR_DIR;
      } else {
        send_to_char(d->character, "Which room ?");
      }
      break;

    case QEDIT_OUT_OPEN_DOOR_DIR:
      if (atoi(arg) > -1 && atoi(arg) < 6) {
        OLC_QCOM(d)->value = atoi(arg);
        hlqedit_disp_outcommand_menu(d);
      } else {
        send_to_char(d->character, "Which direction? (0=n, 1=e, 2=s, 3=w, 4=u, 5=d): ");
      }
      return;

      break;
    case QEDIT_DELETE_QUEST:
      OLC_VAL(d) = 1;
      number = atoi(arg);
      if (number < 1 || NULL == (quest = getquest(d, number)))
        send_to_char(d->character, "No such quest!");
      else {
        if (number == 1) {
          OLC_HLQUEST(d) = OLC_HLQUEST(d)->next;
        } else {
          for (qtmp = OLC_HLQUEST(d); qtmp; qtmp = qtmp->next) {
            if (qtmp->next == quest)
              qtmp->next = quest->next;
          }
        }
        quest->next = NULL;
        free_hlquests(quest);

      }
      hlqedit_disp_menu(d);
      return;

    case QEDIT_APPROVE_QUEST:
      number = atoi(arg);
      if (number < 1 || NULL == (quest = getquest(d, number)))
        send_to_char(d->character, "No such quest!");
      else {
        quest->approved = TRUE;
        send_to_char(d->character, "QUEST APPROVED!");
      }
      hlqedit_disp_menu(d);
      return;
      break;

    case QEDIT_VIEW_QUEST:
      number = atoi(arg);
      if (number < 1 || NULL == (quest = getquest(d, number)))
        send_to_char(d->character, "No such quest!");
      else
        show_quest_to_player(d->character, quest);
      break;
    case QEDIT_MAIN_MENU:
      switch (*arg) {
        case 'q':
        case 'Q':
          if (OLC_VAL(d)) { /* Something has been modified. */
            send_to_char(d->character, "Do you wish to save this quest internally? : ");
            OLC_MODE(d) = QEDIT_CONFIRM_HLSAVESTRING;
          } else {
            OLC_MOB(d) = 0;
            cleanup_olc(d, CLEANUP_ALL);
          }
          return;
        case 'a':
        case 'A':
          if (GET_LEVEL(d->character) < LVL_IMPL)
            send_to_char(d->character, "You are definitely NOT a forger!\r\n");
          else {
            OLC_VAL(d) = 1;
            OLC_MODE(d) = QEDIT_APPROVE_QUEST;
            send_to_char(d->character, "Select which quest to approve\r\n");
          }
          return;
        case 'n':
        case 'N':
          CREATE(OLC_QUESTENTRY(d), struct quest_entry, 1);
          clear_hlquest(OLC_QUESTENTRY(d));
          OLC_MODE(d) = QEDIT_NEWCOMMAND;
          send_to_char(d->character, "What type of quest entry (G)ive, (R)oom or (A)sk?");
          return;
        case 'd':
        case 'D':
          OLC_MODE(d) = QEDIT_DELETE_QUEST;
          send_to_char(d->character, "Select which quest to delete\r\n");
          return;
        case 'v':
        case 'V':
          OLC_MODE(d) = QEDIT_VIEW_QUEST;
          send_to_char(d->character, "Select which quest to view\r\n");
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

ACMD(do_hlqedit) {
  int number = -1, save = 0, real_num;
  struct descriptor_data *d;
  char *buf3;
  char buf2[MAX_INPUT_LENGTH] = {'\0'};
  char buf1[MAX_INPUT_LENGTH] = {'\0'};

  //No screwing around as a mobile.
  if (IS_NPC(ch))
    return;

  // parse arguments
  buf3 = two_arguments(argument, buf1, buf2);

  // no argument  
  if (!*buf1) {
    send_to_char(ch, "Specify a hlquest VNUM to edit.\r\n");
    return;
  } else if (!isdigit(*buf1)) {
    if (str_cmp("save", buf1) != 0) {
      send_to_char(ch, "Yikes!  Stop that, someone will get hurt!\r\n");
      return;
    }

    save = TRUE;

    if (is_number(buf2))
      number = atoi(buf2);
    else if (GET_OLC_ZONE(ch) > 0) {
      zone_rnum zlok;

      if ((zlok = real_zone(GET_OLC_ZONE(ch))) == NOWHERE)
        number = NOWHERE;
      else
        number = genolc_zone_bottom(zlok);
    }

    if (number == NOWHERE) {
      send_to_char(ch, "Save which zone?\r\n");
      return;
    }
  }

  // if numberic arg was given, get it
  if (number == NOWHERE)
    number = atoi(buf1);

  // make sure not already being editted
  for (d = descriptor_list; d; d = d->next) {
    if (STATE(d) == CON_HLQEDIT) {
      if (d->olc && OLC_NUM(d) == number) {
        send_to_char(ch, "That hlquest is currently being edited by %s.\r\n",
          PERS(d->character, ch));
        return;
      }
    }
  }
  d = ch->desc;

  // give descriptor olc struct
  if (d->olc) {
    mudlog(BRF, LVL_IMMORT, TRUE,
      "SYSERR: do_oasis_hlquest: Player already had olc structure.");
    free(d->olc);
  }
  CREATE(d->olc, struct oasis_olc_data, 1);

  // find zone
  if ((OLC_ZNUM(d) = real_zone(number)) == NOWHERE) {
    send_to_char(ch, "Sorry, there is no zone for that number!\r\n");
    free(d->olc);
    d->olc = NULL;
    return;
  }

  // everyone except imps can only edit zones they have been assigned
  if (!can_edit_zone(ch, OLC_ZNUM(d))) {
    send_to_char(ch, "You do not have permission to edit this zone.\r\n");

    free(d->olc);
    d->olc = NULL;
    return;
  }

  if (save) {
    send_to_char(ch, "Saving all hlquest info in zone %d.\r\n",
            zone_table[OLC_ZNUM(d)].number);
    log( "OLC: %s saves hlquest info for zone %d.", GET_NAME(ch),
            zone_table[OLC_ZNUM(d)].number);

    hlqedit_save_to_disk(OLC_ZNUM(d));
    free(d->olc);
    d->olc = NULL;
    return;
  }

  OLC_NUM(d) = number;

  // take descriptor and start up subcommands
  if ((real_num = real_mobile(number)) != NOTHING) {
    send_to_char(ch, "No such mob to make a quest for!\r\n");
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

