/**
* @file interpreter.h                         LuminariMUD
* Public procs, macro defs, subcommand defines for the command intepreter.
*
* Part of the core tbaMUD source code distribution, which is a derivative
* of, and continuation of, CircleMUD.
*
* All rights reserved.  See license for complete information.
* Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University
* CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.
*/
#ifndef _INTERPRETER_H_
#define _INTERPRETER_H_

/* List of external function prototypes.
 * @todo Organize the functions into modules. */

#define CMD_NAME (complete_cmd_info[cmd].command)
#define CMD_IS(cmd_name) (!strcmp(cmd_name, complete_cmd_info[cmd].command))
#define IS_MOVE(cmdnum) (complete_cmd_info[cmdnum].command_pointer == do_move)

void sort_commands(void);
void command_interpreter(struct char_data *ch, char *argument);
char *one_argument_u(char *argument, char *first_arg);
const char *one_argument(const char *argument, char *first_arg, size_t n);
char *one_word(char *argument, char *first_arg);
char *any_one_arg(char *argument, char *first_arg);
const char *any_one_arg_c(const char *argument, char *first_arg, size_t n);
char *two_arguments_u(char *argument, char *first_arg, char *second_arg);
char *three_arguments_u(char *argument, char *first_arg, char *second_arg,
                       char *third_arg);
const char *three_arguments(const char *argument,
                            char *first_arg, size_t n1,
                            char *second_arg, size_t n2,
                            char *third_arg, size_t n3);
int fill_word(char *argument);
int reserved_word(char *argument);
void half_chop(char *string, char *arg1, char *arg2);
void half_chop_c(const char *string, char *arg1, size_t n1, char *arg2, size_t n2);
void nanny(struct descriptor_data *d, char *arg);
int is_abbrev(const char *arg1, const char *arg2);
int is_number(const char *str);
int find_command(const char *command);
void skip_spaces(char **string);
void skip_spaces_c(const char **string);
char *delete_doubledollar(char *string);
int special(struct char_data *ch, int cmd, char *arg);
void free_alias(struct alias_data *a);
int perform_alias(struct descriptor_data *d, char *orig, size_t maxlen);
int enter_player_game(struct descriptor_data *d);

int load_account(char *name, struct account_data *account);
void save_account(struct account_data *account);
void show_account_menu(struct descriptor_data *d);
void remove_char_from_account(struct char_data *ch, struct account_data *account);
char *get_char_account_name(char *name);

ACMD_DECL(do_account);

/* ACMDs available through interpreter.c */
ACMD_DECL(do_alias);

/* for compatibility with 2.20: */
#define argument_interpreter(a, b, c) two_arguments_u(a, b, c)

/* WARNING: if you have added diagonal directions and have them at the
 * beginning of the command list.. change this value to 11 or 15 (depending)
 * reserve these commands to come straight from the cmd list then start
 * sorting */
#define RESERVE_CMDS 7

struct command_info
{
  const char *command;
  const char *sort_as;
  byte minimum_position;
  void (*command_pointer)(struct char_data *ch, const char *argument, int cmd, int subcmd);
  sh_int minimum_level;
  int subcmd;
  sh_int ignore_wait; // set to TRUE if the command can be performed during wait event
  int actions_required;
  int action_cooldowns[NUM_ACTIONS];
  int (*command_check_pointer)(struct char_data *ch, bool show_error);
};

struct mob_script_command_t
{
  const char *command_name;
  void (*command_pointer)(struct char_data *ch, const char *argument, int cmd, int subcmd);
  int subcmd;
};

struct alias_data
{
  char *alias;
  char *replacement;
  int type;
  struct alias_data *next;
};

#define ALIAS_SIMPLE 0
#define ALIAS_COMPLEX 1

#define ALIAS_SEP_CHAR ';'
#define ALIAS_VAR_CHAR '$'
#define ALIAS_GLOB_CHAR '*'

/* SUBCOMMANDS: You can define these however you want to, and the definitions
 * of the subcommands are independent from function to function.*/
/* directions */

/* do_move
 *
 * Make sure the SCMD_XX directions are mapped
 * to the cardinal directions.
 */
#define SCMD_NORTH NORTH
#define SCMD_EAST EAST
#define SCMD_SOUTH SOUTH
#define SCMD_WEST WEST
#define SCMD_UP UP
#define SCMD_DOWN DOWN
#define SCMD_NW NORTHWEST
#define SCMD_NE NORTHEAST
#define SCMD_SE SOUTHEAST
#define SCMD_SW SOUTHWEST

#ifdef CAMPAIGN_FR

#define SCMD_IN IN
#define SCMD_OUT OUT

#endif
/** @deprecated all old do_poof stuff is deprecated and unused. */
#define SCMD_POOFIN 0
/** @deprecated all old do_poof stuff is deprecated and unused. */
#define SCMD_POOFOUT 1

/* do_oasis_Xlist */
#define SCMD_OASIS_RLIST 0
#define SCMD_OASIS_MLIST 1
#define SCMD_OASIS_OLIST 2
#define SCMD_OASIS_SLIST 3
#define SCMD_OASIS_ZLIST 4
#define SCMD_OASIS_TLIST 5
#define SCMD_OASIS_QLIST 6
#define SCMD_OASIS_REGLIST 7
#define SCMD_OASIS_PATHLIST 8
#define SCMD_OASIS_OLIST_APPLIES 9

#define SCMD_PILFER 1

#define SCMD_QUICK_CHANT 1
#define SCMD_QUICK_MIND  2

#define SCMD_DIALOGUE_DIPLOMACY  1
#define SCMD_DIALOGUE_INTIMIDATE 2
#define SCMD_DIALOGUE_BLUFF      3

/* Necessary for CMD_IS macro.  Borland needs the structure defined first
 * so it has been moved down here. */
/* Global buffering system */
#ifndef __INTERPRETER_C__

extern int *cmd_sort_info;
extern struct command_info *complete_cmd_info;
extern const struct command_info cmd_info[];

#endif /* __INTERPRETER_C__ */

#endif /* _INTERPRETER_H_ */
