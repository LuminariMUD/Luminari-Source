/**************************************************************************
 *  File: boards.c                                     Part of LuminariMUD *
 *  Usage: Handling of multiple bulletin boards.                           *
 *                                                                         *
 *  All rights reserved.  See license for complete information.            *
 *                                                                         *
 *  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 **************************************************************************/

#define __BOARDS_C__

/* FEATURES & INSTALLATION INSTRUCTIONS
 * - Arbitrary number of boards handled by one set of generalized routines.
 *   Adding a new board is as easy as adding another entry to an array.
 * - Safe removal of messages while other messages are being written.
 *
 * TO ADD A NEW BOARD, simply follow our easy 4-step program:
 * 1 - Create a new board object in the object files.
 * 2 - Increase the NUM_OF_BOARDS constant in boards.h.
 * 3 - Add a new line to the board_info array below.  The fields are:
 * 	Board's virtual number.
 * 	Min level one must be to look at this board or read messages on it.
 * 	Min level one must be to post a message to the board.
 * 	Min level one must be to remove other people's messages from this
 * 	  board (but you can always remove your own message).
 * 	Filename of this board, in quotes.
 * 	Last field must always be 0.
 * 4 - In spec/spec_assign_objects.c, find the board assignment section
 *     gen_board to the other bulletin boards, and add your new one in a
 *     similar fashion. */

#include "conf.h"
#include "core/sysdep.h"
#include <time.h>
#include "core/structs.h"
#include "core/utils.h"
#include "core/comm.h"
#include "core/db.h"
#include "boards.h"
#include "core/interpreter.h"
#include "core/handler.h"
#include "olc/improved-edit.h"
#include "core/modify.h"
#include "core/binary_formats.h"

/* Board appearance order. */
#define NEWEST_AT_TOP FALSE

/* Format: vnum, read lvl, write lvl, remove lvl, filename, 0 at end. Be sure
 * to also change NUM_OF_BOARDS in board.h*/
struct board_info_type board_info[NUM_OF_BOARDS] = {
    {2201, 0, 1, LVL_IMPL, LIB_ETC "board.general", NOTHING},
    {3098, LVL_IMMORT, LVL_IMMORT, LVL_IMPL, LIB_ETC "board.immortal", NOTHING}};

/* local (file scope) global variables */
static char *msg_storage[INDEX_SIZE];
static int msg_storage_taken[INDEX_SIZE];
static int num_of_msgs[NUM_OF_BOARDS];
static struct board_msginfo msg_index[NUM_OF_BOARDS][MAX_BOARD_MESSAGES];

/* local static utility functions */
static int find_slot(void);
static int find_board(struct obj_data *board);
static void init_boards(void);
static void board_clear_board(int board_type);

static int find_slot(void)
{
  int i;

  for (i = 0; i < INDEX_SIZE; i++)
    if (!msg_storage_taken[i])
    {
      msg_storage_taken[i] = 1;
      return (i);
    }
  return (-1);
}

/* Find the legacy board entry for the exact object whose special procedure
 * was invoked. MySQL-managed board objects intentionally have no entry. */
static int find_board(struct obj_data *board)
{
  int i;

  if (!board)
    return (-1);

  for (i = 0; i < NUM_OF_BOARDS; i++)
    if (BOARD_RNUM(i) == GET_OBJ_RNUM(board))
      return (i);

  return (-1);
}

static void init_boards(void)
{
  int i, j, fatal_error = 0;

  for (i = 0; i < INDEX_SIZE; i++)
  {
    msg_storage[i] = 0;
    msg_storage_taken[i] = 0;
  }

  /* Verify board array size matches NUM_OF_BOARDS */
  int actual_boards = sizeof(board_info) / sizeof(board_info[0]);
  if (actual_boards != NUM_OF_BOARDS)
  {
    log("SYSERR: Board count mismatch! NUM_OF_BOARDS=%d but board_info has %d entries",
        NUM_OF_BOARDS, actual_boards);
    fatal_error = 1;
  }

  for (i = 0; i < NUM_OF_BOARDS; i++)
  {
    if ((BOARD_RNUM(i) = real_object(BOARD_VNUM(i))) == NOTHING)
    {
      log("SYSERR: Fatal board error: board vnum %" PRI_IDX " does not exist!", BOARD_VNUM(i));
      fatal_error = 1;
    }
    else
    {
      log("Board %d initialized: vnum=%" PRI_IDX ", rnum=%" PRI_IDX, i, BOARD_VNUM(i),
          BOARD_RNUM(i));
    }
    num_of_msgs[i] = 0;
    for (j = 0; j < MAX_BOARD_MESSAGES; j++)
    {
      memset((char *)&(msg_index[i][j]), 0, sizeof(struct board_msginfo));
      msg_index[i][j].slot_num = -1;
    }
    board_load_board(i);
  }

  if (fatal_error)
    exit(1);
}

SPECIAL(gen_board)
{
  int board_type;
  static int loaded = 0;
  struct obj_data *board = (struct obj_data *)me;

  /* These were originally globals for some unknown reason. */
  int ACMD_READ, ACMD_LOOK, ACMD_EXAMINE, ACMD_WRITE, ACMD_REMOVE;

  if (!cmd && argument && !strcmp(argument, "identify"))
  {
    send_to_char(ch, "This is a bulletin board. You can use the following commands:\r\n");
    send_to_char(ch, "  read <num>    - Read a specific message\r\n");
    send_to_char(ch, "  write         - Write a new message\r\n");
    send_to_char(ch, "  remove <num>  - Remove a message (if allowed)\r\n");
    send_to_char(ch, "  look board    - List all messages\r\n");
    return TRUE;
  }

  if (!loaded)
  {
    init_boards();
    loaded = 1;
  }
  if (!ch->desc)
    return (0);

  ACMD_READ = find_command("read");
  ACMD_WRITE = find_command("write");
  ACMD_REMOVE = find_command("remove");
  ACMD_LOOK = find_command("look");
  ACMD_EXAMINE = find_command("examine");

  if (cmd != ACMD_WRITE && cmd != ACMD_LOOK && cmd != ACMD_EXAMINE && cmd != ACMD_READ &&
      cmd != ACMD_REMOVE)
    return (0);

  if ((board_type = find_board(board)) == -1)
    return (0);
  if (cmd == ACMD_WRITE)
    return (board_write_message(board_type, ch, argument, board));
  else if (cmd == ACMD_LOOK || cmd == ACMD_EXAMINE)
    return (board_show_board(board_type, ch, argument, board));
  else if (cmd == ACMD_READ)
    return (board_display_msg(board_type, ch, argument, board));
  else if (cmd == ACMD_REMOVE)
    return (board_remove_msg(board_type, ch, argument, board));
  else
    return (0);
}

int board_write_message(int board_type, struct char_data *ch, char *arg,
                        struct obj_data *board __attribute__((unused)))
{
  time_t ct;
  char buf[MAX_INPUT_LENGTH] = {'\0'}, buf2[MAX_NAME_LENGTH + 3], tmstr[32] = {'\0'};

  if (GET_LEVEL(ch) < WRITE_LVL(board_type))
  {
    send_to_char(ch, "You are not holy enough to write on this board.\r\n");
    return (1);
  }
  if (num_of_msgs[board_type] >= MAX_BOARD_MESSAGES)
  {
    send_to_char(ch, "The board is full.\r\n");
    return (1);
  }
  if ((NEW_MSG_INDEX(board_type).slot_num = find_slot()) == -1)
  {
    send_to_char(ch, "The board is malfunctioning - sorry.\r\n");
    log("SYSERR: Board: failed to find empty slot on write.");
    return (1);
  }
  /* skip blanks */
  skip_spaces(&arg);
  delete_doubledollar(arg);

  /* JE Truncate headline at 80 chars if it's longer than that. */
  arg[80] = '\0';

  if (!*arg)
  {
    send_to_char(ch, "We must have a headline!\r\n");
    return (1);
  }
  ct = time(0);
  format_time_string(ct, "%a %b %d %Y", tmstr, sizeof(tmstr));

  snprintf(buf2, sizeof(buf2), "(%s)", GET_NAME(ch));
  snprintf(buf, sizeof(buf), "%s %-12s :: %s", tmstr, buf2, arg);
  NEW_MSG_INDEX(board_type).heading = strdup(buf);
  NEW_MSG_INDEX(board_type).level = GET_LEVEL(ch);

  send_to_char(ch, "Write your message.\r\n");
  send_editor_help(ch->desc);
  act("$n starts to write a message.", TRUE, ch, 0, 0, TO_ROOM);

  string_write(ch->desc, &(msg_storage[NEW_MSG_INDEX(board_type).slot_num]), MAX_MESSAGE_LENGTH,
               board_type + BOARD_MAGIC, NULL);

  num_of_msgs[board_type]++;
  return (1);
}

int board_show_board(int board_type, struct char_data *ch, char *arg, struct obj_data *board)
{
  int i;
  char tmp[MAX_STRING_LENGTH] = {'\0'}, buf[MAX_STRING_LENGTH] = {'\0'};

  if (!ch->desc)
    return (0);

  one_argument(arg, tmp, sizeof(tmp));

  if (!*tmp || !isname(tmp, board->name))
    return (0);

  if (GET_LEVEL(ch) < READ_LVL(board_type))
  {
    send_to_char(ch, "You try but fail to understand the holy words.\r\n");
    return (1);
  }
  act("$n studies the board.", TRUE, ch, 0, 0, TO_ROOM);

  if (!num_of_msgs[board_type])
    send_to_char(ch, "This is a bulletin board.  Usage: READ/REMOVE <messg #>, WRITE "
                     "<header>.\r\nThe board is empty.\r\n");
  else
  {
    size_t len = 0;
    int nlen;

    len = snprintf(buf, sizeof(buf),
                   "This is a bulletin board.  Usage: READ/REMOVE <messg #>, WRITE <header>.\r\n"
                   "You will need to look at the board to save your message.\r\n"
                   "There are %d messages on the board.\r\n",
                   num_of_msgs[board_type]);
#if NEWEST_AT_TOP
    for (i = num_of_msgs[board_type] - 1; i >= 0; i--)
    {
      if (!MSG_HEADING(board_type, i))
        goto fubar;

      nlen = snprintf(buf + len, sizeof(buf) - len, "%-2d : %s\r\n", num_of_msgs[board_type] - i,
                      MSG_HEADING(board_type, i));
      if (len + nlen >= sizeof(buf) || nlen < 0)
        break;
      len += nlen;
    }
#else
    for (i = 0; i < num_of_msgs[board_type]; i++)
    {
      if (!MSG_HEADING(board_type, i))
        goto fubar;

      nlen = snprintf(buf + len, sizeof(buf) - len, "%-2d : %s\r\n", i + 1,
                      MSG_HEADING(board_type, i));
      if (len + nlen >= sizeof(buf) || nlen < 0)
        break;
      len += nlen;
    }
#endif
    page_string(ch->desc, buf, TRUE);
  }
  return (1);

fubar:
  log("SYSERR: Board %d is fubar'd.", board_type);
  send_to_char(ch, "Sorry, the board isn't working.\r\n");
  return (1);
}

int board_display_msg(int board_type, struct char_data *ch, char *arg, struct obj_data *board)
{
  char number[MAX_INPUT_LENGTH] = {'\0'}, buffer[MAX_STRING_LENGTH] = {'\0'};
  int msg, ind;

  one_argument(arg, number, sizeof(number));
  if (!*number)
  {
    send_to_char(ch, "Read which message? Use 'look board' to list messages.\r\n");
    return (1);
  }
  if (isname(number, board->name)) /* so "read board" works */
    return (board_show_board(board_type, ch, arg, board));
  if (!is_number(number)) /* read 2.mail, look 2.sword */
    return (0);
  if (!(msg = parse_int(number)))
    return (0);

  if (GET_LEVEL(ch) < READ_LVL(board_type))
  {
    send_to_char(ch, "You try but fail to understand the holy words.\r\n");
    return (1);
  }
  if (!num_of_msgs[board_type])
  {
    send_to_char(ch, "The board is empty!\r\n");
    return (1);
  }
  if (msg < 1 || msg > num_of_msgs[board_type])
  {
    send_to_char(ch, "That message exists only in your imagination.\r\n");
    return (1);
  }
#if NEWEST_AT_TOP
  ind = num_of_msgs[board_type] - msg;
#else
  ind = msg - 1;
#endif
  if (MSG_SLOTNUM(board_type, ind) < 0 || MSG_SLOTNUM(board_type, ind) >= INDEX_SIZE)
  {
    send_to_char(ch, "Sorry, the board is not working.\r\n");
    log("SYSERR: Board is screwed up. (Room #%u)", GET_ROOM_VNUM(IN_ROOM(ch)));
    return (1);
  }
  if (!(MSG_HEADING(board_type, ind)))
  {
    send_to_char(ch, "That message appears to be screwed up.\r\n");
    return (1);
  }
  if (!(msg_storage[MSG_SLOTNUM(board_type, ind)]))
  {
    send_to_char(ch, "That message seems to be empty.\r\n");
    return (1);
  }
  snprintf(buffer, sizeof(buffer), "Message %d : %s\r\n\r\n%s\r\n", msg,
           MSG_HEADING(board_type, ind), msg_storage[MSG_SLOTNUM(board_type, ind)]);

  page_string(ch->desc, buffer, TRUE);

  return (1);
}

int board_remove_msg(int board_type, struct char_data *ch, char *arg,
                     struct obj_data *board __attribute__((unused)))
{
  int ind, msg, slot_num;
  char number[MAX_INPUT_LENGTH] = {'\0'}, buf[MAX_INPUT_LENGTH] = {'\0'};
  struct descriptor_data *d;

  one_argument(arg, number, sizeof(number));

  if (!*number || !is_number(number))
    return (0);
  if (!(msg = parse_int(number)))
    return (0);

  if (!num_of_msgs[board_type])
  {
    send_to_char(ch, "The board is empty!\r\n");
    return (1);
  }
  if (msg < 1 || msg > num_of_msgs[board_type])
  {
    send_to_char(ch, "That message exists only in your imagination.\r\n");
    return (1);
  }
#if NEWEST_AT_TOP
  ind = num_of_msgs[board_type] - msg;
#else
  ind = msg - 1;
#endif
  if (!MSG_HEADING(board_type, ind))
  {
    send_to_char(ch, "That message appears to be screwed up.\r\n");
    return (1);
  }
  snprintf(buf, sizeof(buf), "(%s)", GET_NAME(ch));
  if (GET_LEVEL(ch) < REMOVE_LVL(board_type) && !(strstr(MSG_HEADING(board_type, ind), buf)))
  {
    send_to_char(ch, "You are not holy enough to remove other people's messages.\r\n");
    return (1);
  }
  if (GET_LEVEL(ch) < MSG_LEVEL(board_type, ind))
  {
    send_to_char(ch, "You can't remove a message holier than yourself.\r\n");
    return (1);
  }
  slot_num = MSG_SLOTNUM(board_type, ind);
  if (slot_num < 0 || slot_num >= INDEX_SIZE)
  {
    send_to_char(ch, "That message is majorly screwed up.\r\n");
    log("SYSERR: The board is seriously screwed up. (Room #%u)", GET_ROOM_VNUM(IN_ROOM(ch)));
    return (1);
  }
  for (d = descriptor_list; d; d = d->next)
    if (STATE(d) == CON_PLAYING && d->str == &(msg_storage[slot_num]))
    {
      send_to_char(ch, "At least wait until the author is finished before removing it!\r\n");
      return (1);
    }
  if (msg_storage[slot_num])
    free(msg_storage[slot_num]);
  msg_storage[slot_num] = 0;
  msg_storage_taken[slot_num] = 0;
  if (MSG_HEADING(board_type, ind))
    free(MSG_HEADING(board_type, ind));

  for (; ind < num_of_msgs[board_type] - 1; ind++)
  {
    MSG_HEADING(board_type, ind) = MSG_HEADING(board_type, ind + 1);
    MSG_SLOTNUM(board_type, ind) = MSG_SLOTNUM(board_type, ind + 1);
    MSG_LEVEL(board_type, ind) = MSG_LEVEL(board_type, ind + 1);
  }
  /* The vacated last entry still names the moved post; board_clear_board()
   * would free that post twice. */
  memset(&(msg_index[board_type][ind]), 0, sizeof(struct board_msginfo));
  msg_index[board_type][ind].slot_num = -1;
  num_of_msgs[board_type]--;

  send_to_char(ch, "Message removed.\r\n");
  snprintf(buf, sizeof(buf), "$n just removed message %d.", msg);
  act(buf, FALSE, ch, 0, 0, TO_ROOM);
  board_save_board(board_type);

  return (1);
}

void board_save_board(int board_type)
{
  struct board_file_message messages[MAX_BOARD_MESSAGES];
  enum binary_format_status status;
  unsigned char *data = NULL;
  size_t size = 0;
  int i, slot;

  for (i = 0; i < num_of_msgs[board_type]; i++)
  {
    slot = MSG_SLOTNUM(board_type, i);
    messages[i].level = MSG_LEVEL(board_type, i);
    messages[i].heading = MSG_HEADING(board_type, i);
    messages[i].message = slot >= 0 && slot < INDEX_SIZE ? msg_storage[slot] : NULL;
  }

  status = board_file_encode(messages, (size_t)num_of_msgs[board_type], &data, &size);
  if (status != BINARY_FORMAT_OK)
  {
    log("SYSERR: Unable to encode board %d: %s.", board_type, binary_format_status_name(status));
    return;
  }
  if (!replace_durable_file(FILENAME(board_type), BOARD_FILE_MAGIC,
                            board_file_max_size(MAX_BOARD_MESSAGES), data, size))
    log("SYSERR: Unable to save board %d to %s.", board_type, FILENAME(board_type));
  free(data);
}

/* Loads a board file all or nothing. A file that cannot be loaded is moved
 * aside, never deleted, so a later save cannot destroy it. */
void board_load_board(int board_type)
{
  struct board_file_message *messages = NULL;
  enum binary_format_status status;
  unsigned char *data = NULL;
  size_t size = 0, count = 0, i;
  int version = BINARY_FORMAT_LEGACY, slot;
  size_t free_slots = 0;

  switch (read_durable_file(FILENAME(board_type), board_file_max_size(MAX_BOARD_MESSAGES), &data,
                            &size))
  {
  case DURABLE_FILE_ABSENT:
    return;
  case DURABLE_FILE_UNREADABLE:
    quarantine_durable_file(FILENAME(board_type));
    return;
  case DURABLE_FILE_READ:
    break;
  }

  status = board_file_decode(data, size, MAX_BOARD_MESSAGES, &messages, &count, &version);
  free(data);
  if (status != BINARY_FORMAT_OK)
  {
    log("SYSERR: Rejected board file %s: %s.", FILENAME(board_type),
        binary_format_status_name(status));
    quarantine_durable_file(FILENAME(board_type));
    return;
  }

  for (slot = 0; slot < INDEX_SIZE; slot++)
    if (!msg_storage_taken[slot])
      free_slots++;
  if (free_slots < count)
  {
    log("SYSERR: Out of message slots loading board %d.", board_type);
    board_file_free(messages, count);
    quarantine_durable_file(FILENAME(board_type));
    return;
  }

  /* The board now owns the decoded strings. */
  for (i = 0; i < count; i++)
  {
    slot = find_slot();
    msg_index[board_type][i].slot_num = slot;
    msg_index[board_type][i].heading = messages[i].heading;
    msg_index[board_type][i].level = messages[i].level;
    msg_storage[slot] = messages[i].message;
  }
  num_of_msgs[board_type] = (int)count;
  free(messages);

  if (version == BINARY_FORMAT_LEGACY && size > 0)
    log("Board file %s uses the legacy native layout; its next save upgrades it and keeps a "
        "backup.",
        FILENAME(board_type));
}

/* When shutting down, clear all boards. */
void board_clear_all(void)
{
  int i;

  for (i = 0; i < NUM_OF_BOARDS; i++)
    board_clear_board(i);
}

/* Clear the in-memory structures. */
void board_clear_board(int board_type)
{
  int i;

  for (i = 0; i < MAX_BOARD_MESSAGES; i++)
  {
    if (MSG_SLOTNUM(board_type, i) == -1)
      continue; /* don't try to free non-existant slots */
    if (MSG_HEADING(board_type, i))
      free(MSG_HEADING(board_type, i));
    if (msg_storage[MSG_SLOTNUM(board_type, i)])
      free(msg_storage[MSG_SLOTNUM(board_type, i)]);
    msg_storage_taken[MSG_SLOTNUM(board_type, i)] = 0;
    memset((char *)&(msg_index[board_type][i]), 0, sizeof(struct board_msginfo));
    msg_index[board_type][i].slot_num = -1;
  }
  num_of_msgs[board_type] = 0;
}
