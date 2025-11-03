/* *************************************************************************
 *   File: hlquest.h                                   Part of LuminariMUD *
 *  Usage: alternate quest system                                          *
 *  Author: Homeland, ported to tba/luminari by Zusuk                      *
 ************************************************************************* */

#ifndef HLQUEST_H
#define HLQUEST_H

#ifndef __QUEST_CODE_H
#define __QUEST_CODE_H

enum quest_command_type
{
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

enum quest_type
{
  QUEST_ASK = 0,
  QUEST_GIVE,
  QUEST_ROOM
};

struct quest_command
{
  int type;
  int value;
  int location;
  struct quest_command *next;
};

struct quest_entry
{
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
int quest_value_vnum(struct quest_command *qcom);
void free_hlquest(struct char_data *ch);
void free_hlquests(struct quest_entry *quest);
void clear_hlquest(struct quest_entry *quest);
void show_quest_to_player(struct char_data *ch, struct quest_entry *quest);
void hlqedit_parse(struct descriptor_data *d, char *arg);
void boot_the_quests(FILE *quest_f, char *filename, int rec_count);
void quest_ask(struct char_data *ch, struct char_data *victim, char *keyword);
void quest_give(struct char_data *ch, struct char_data *victim);
void quest_room(struct char_data *ch);
bool is_object_in_a_quest(struct obj_data *obj);
void hlqedit_save_to_disk(int zone_num);
void hlqedit_disp_menu(struct descriptor_data *d);
/* end functions */

/* commands */
ACMD_DECL(do_qinfo);
ACMD_DECL(do_checkapproved);
ACMD_DECL(do_kitquests);
ACMD_DECL(do_spellquests);
ACMD_DECL(do_qref);
ACMD_DECL(do_qview);
ACMD_DECL(do_hlqedit);
/* end commands */

/*
 * Submodes of QEDIT connectedness.
 */
#define HLQEDIT_MAIN_MENU 0
#define HLQEDIT_NEWCOMMAND 1
#define HLQEDIT_KEYWORDS 2
#define HLQEDIT_REPLYMSG 3
#define HLQEDIT_INCOMMAND 4
#define HLQEDIT_OUTCOMMANDMENU 5
#define HLQEDIT_OUT_COIN 6
#define HLQEDIT_OUT_ITEM 7
#define HLQEDIT_OUT_LOAD_OBJECT 8
#define HLQEDIT_OUT_LOAD_OBJECT_ROOM 9
#define HLQEDIT_OUT_LOAD_MOB 10
#define HLQEDIT_OUT_LOAD_MOB_ROOM 11
#define HLQEDIT_DELETE_QUEST 12
#define HLQEDIT_CONFIRM_HLSAVESTRING 13
#define HLQEDIT_APPROVE_QUEST 14
#define HLQEDIT_VIEW_QUEST 15
#define HLQEDIT_IN_COIN 16
#define HLQEDIT_IN_ITEM 17
#define HLQEDIT_OUT_TEACH_SPELL 18
#define HLQEDIT_OUT_OPEN_DOOR 19
#define HLQEDIT_OUT_OPEN_DOOR_DIR 20
#define HLQEDIT_OUT_CHURCH 21
#define HLQEDIT_OUT_KIT_PREREQ 22
#define HLQEDIT_OUT_KIT_SELECT 23
#define HLQEDIT_ROOM 24
/* end qedit connectedness */

#endif

#endif /* HLQUEST_H */
