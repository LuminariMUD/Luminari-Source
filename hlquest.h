/* *************************************************************************
 *   File: hlquest.h                                   Part of LuminariMUD *
 *  Usage: alternate quest system                                          *
 *  Author: Homeland, ported to tba/luminari by Zusuk                      *
 ************************************************************************* */

#ifndef HLQUEST_H
#define	HLQUEST_H

#ifndef __QUEST_CODE_H
#define __QUEST_CODE_H

enum quest_command_type
{
  QUEST_COMMAND_COINS=0,
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
 QUEST_ASK=0,
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
 int  type;
 char *keywords;
 char *reply_msg;
 struct quest_command  *in;
 struct quest_command  *out;
 bool	approved;
 int 	room;

 struct quest_entry *next;

};

#endif



#endif	/* HLQUEST_H */

