/* *************************************************************************
 *   File: hlquest.h                                   Part of LuminariMUD *
 *  Usage: alternate quest system                                          *
 *  Author: Homeland, ported to tba/luminari by Zusuk                      *
 ************************************************************************* */

#ifndef HLQUEST_H
#define	HLQUEST_H

#ifndef __QUEST_CODE_H
#define __QUEST_CODE_H

enum quest_command_type {
  QUEST_COMMAND_COINS = 0,
  QUEST_COMMAND_ITEM,
  QUEST_COMMAND_LOAD_OBJECT_INROOM,
  QUEST_COMMAND_LOAD_MOB_INROOM,
  QUEST_COMMAND_ATTACK_QUESTOR,
  QUEST_COMMAND_DISAPPEAR,
  QUEST_COMMAND_TEACH_SPELL,
  QUEST_COMMAND_OPEN_DOOR,
  QUEST_COMMAND_FOLLOW,
  QUEST_COMMAND_KIT,
  QUEST_COMMAND_CHURCH,
  QUEST_COMMAND_CAST_SPELL,
};

enum quest_type {
  QUEST_ASK = 0,
  QUEST_GIVE,
  QUEST_ROOM
};

struct quest_command {
  int type;
  int value;
  int location;
  struct quest_command *next;
};

struct quest_entry {
  int type;
  char *keywords;
  char *reply_msg;
  struct quest_command *in;
  struct quest_command *out;
  bool approved;
  int room;

  struct quest_entry *next;
};

/* functions */
void clear_hlquest(struct quest_entry *quest);
int quest_location_vnum(struct quest_command *qcom);
int quest_value_vnum(struct quest_command * qcom);
void free_hlquest(struct char_data * ch);
void free_hlquests(struct quest_entry *quest);
void clear_hlquest(struct quest_entry *quest);
void show_quest_to_player(struct char_data *ch, struct quest_entry *quest);
void hlqedit_parse(struct descriptor_data *d, char *arg);
void boot_the_quests(FILE * quest_f, char *filename, int rec_count);
void quest_ask(struct char_data * ch, struct char_data * victim, char *keyword);
void quest_give(struct char_data * ch, struct char_data * victim);
void quest_room(struct char_data * ch);
bool is_object_in_a_quest(struct obj_data *obj);
void hlqedit_save_to_disk(int zone_num);
void hlqedit_disp_menu(struct descriptor_data *d);
/* end functions */

/* commands */
ACMD(do_qinfo);
ACMD(do_checkapproved);
ACMD(do_kitquests);
ACMD(do_spellquests);
ACMD(do_qref);
ACMD(do_qview);
ACMD(do_hlqedit);
/* end commands */
  
/*
 * Submodes of QEDIT connectedness.
 */
#define QEDIT_MAIN_MENU             0
#define QEDIT_NEWCOMMAND            1
#define QEDIT_KEYWORDS              2
#define QEDIT_REPLYMSG              3
#define QEDIT_INCOMMAND             4
#define QEDIT_OUTCOMMANDMENU        5
#define QEDIT_OUT_COIN              6
#define QEDIT_OUT_ITEM              7
#define QEDIT_OUT_LOAD_OBJECT       8
#define QEDIT_OUT_LOAD_OBJECT_ROOM  9
#define QEDIT_OUT_LOAD_MOB         10
#define QEDIT_OUT_LOAD_MOB_ROOM    11
#define QEDIT_DELETE_QUEST         12
#define QEDIT_CONFIRM_HLSAVESTRING 13
#define QEDIT_APPROVE_QUEST        14
#define QEDIT_VIEW_QUEST           15
#define QEDIT_IN_COIN              16
#define QEDIT_IN_ITEM              17
#define QEDIT_OUT_TEACH_SPELL      18
#define QEDIT_OUT_OPEN_DOOR        19
#define QEDIT_OUT_OPEN_DOOR_DIR    20
#define QEDIT_OUT_CHURCH           21
#define QEDIT_OUT_KIT_PREREQ       22
#define QEDIT_OUT_KIT_SELECT       23
#define QEDIT_ROOM                 24
/* end qedit connectedness */

#endif

#endif	/* HLQUEST_H */

