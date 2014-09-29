/**************************************************************************
*  File: hedit.c                                           Part of tbaMUD *
*  Usage: Oasis OLC Help Editor.                                          *
* Author: Steve Wolfe, Scott Meisenholder, Rhade                          *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
**************************************************************************/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "db.h"
#include "boards.h"
#include "oasis.h"
#include "genolc.h"
#include "genzon.h"
#include "handler.h"
#include "improved-edit.h"
#include "act.h"
#include "hedit.h"
#include "modify.h"
#include "help.h"
#include "mysql.h"

/* local functions */
static void hedit_disp_menu(struct descriptor_data *);
static void hedit_setup_new(struct descriptor_data *);
static void hedit_save_to_disk(struct descriptor_data *);
static void hedit_save_to_db(struct descriptor_data *);
static void hedit_save_internally(struct descriptor_data *);


ACMD(do_oasis_hedit)
{
  char arg[MAX_INPUT_LENGTH];
  struct descriptor_data *d;

  /* No building as a mob or while being forced. */
  if (IS_NPC(ch) || !ch->desc || STATE(ch->desc) != CON_PLAYING)
    return;

  if (!can_edit_zone(ch, HEDIT_PERMISSION)) {
    send_to_char(ch, "You don't have access to editing help files.\r\n");
    return;
  }

  for (d = descriptor_list; d; d = d->next) {
    if (STATE(d) == CON_HEDIT) {
      send_to_char(ch, "Sorry, only one can person can edit help files at a time.\r\n");
      return;
    }
  }

  one_argument(argument, arg);

  if (!*arg) {
    send_to_char(ch, "Please specify a help entry to edit.\r\n");
    return;
  }

  d = ch->desc;
/*  
  if (!str_cmp("save", argument)) {
    mudlog(CMP, MAX(LVL_BUILDER, GET_INVIS_LEV(ch)), TRUE, "OLC: %s saves help files.",
           GET_NAME(ch));
    hedit_save_to_disk(d);
    send_to_char(ch, "Saving help files.\r\n");
    return;
  }
*/
  /* Give descriptor an OLC structure. */
  if (d->olc) {
    mudlog(BRF, LVL_IMMORT, TRUE, "SYSERR: do_oasis: Player already had olc structure.");
    free(d->olc);
  }

  CREATE(d->olc, struct oasis_olc_data, 1);
  OLC_NUM(d) = 0;
  OLC_STORAGE(d) = strdup(arg);
  
  OLC_HELP(d) = search_help(OLC_STORAGE(d), LVL_IMPL);

  /*  
  if ((OLC_ZNUM(d) != NOWHERE) && help_table[OLC_ZNUM(d)].duplicate) {
    for (i = 0; i < top_of_helpt; i++)
      if (help_table[i].duplicate == 0 && help_table[i].entry == help_table[OLC_ZNUM(d)].entry) {
        OLC_ZNUM(d) = i;
        break;
      }
  }  
  */

  if (OLC_HELP(d) == NULL) {
    send_to_char(ch, "Do you wish to add the '%s' help file? ", OLC_STORAGE(d));
    OLC_MODE(d) = HEDIT_CONFIRM_ADD;
  } else {
    send_to_char(ch, "Do you wish to edit the '%s' help file? ",
            OLC_HELP(d)->keyword);
    OLC_MODE(d) = HEDIT_CONFIRM_EDIT;
  }

  STATE(d) = CON_HEDIT;
  act("$n starts editing help files.", TRUE, d->character, 0, 0, TO_ROOM);
  SET_BIT_AR(PLR_FLAGS(ch), PLR_WRITING);
  mudlog(CMP, LVL_IMMORT, TRUE, "OLC: %s starts editing help files.", GET_NAME(d->character));
}

static void hedit_setup_new(struct descriptor_data *d)
{
  CREATE(OLC_HELP(d), struct help_entry_list, 1);

  OLC_HELP(d)->keyword		  = strdup(OLC_STORAGE(d));
  OLC_HELP(d)->alternate_keywords = NULL;
  OLC_HELP(d)->entry		  = strdup("This help file is unfinished.\r\n");
  OLC_HELP(d)->min_level	  = 0;
  OLC_HELP(d)->last_updated       = NULL;
  OLC_VAL(d) = 0;

  hedit_disp_menu(d);
}
/*  
static void hedit_setup_existing(struct descriptor_data *d, int rnum)
{
  CREATE(OLC_HELP(d), struct help_index_element, 1);

  OLC_HELP(d)->keywords		= str_udup(help_table[rnum].keywords);
  OLC_HELP(d)->entry		= str_udup(help_table[rnum].entry);
  OLC_HELP(d)->duplicate	= help_table[rnum].duplicate;
  OLC_HELP(d)->min_level	= help_table[rnum].min_level;
  OLC_VAL(d) = 0;

  hedit_disp_menu(d);
}
*/

static void hedit_save_internally(struct descriptor_data *d)
{
  hedit_save_to_disk(d);
}

static void hedit_save_to_disk(struct descriptor_data *d)
{
  /* Save to the database as well.  */
  hedit_save_to_db(d);
}

static void hedit_save_to_db(struct descriptor_data *d)
{
  char buf1[MAX_STRING_LENGTH], buf2[MAX_STRING_LENGTH], buf[MAX_STRING_LENGTH]; /* Buffers for DML query. */
  int i = 0;
 
  if( OLC_HELP(d) == NULL) 
    return;

  strncpy(buf1, OLC_HELP(d)->entry ? OLC_HELP(d)->entry : "Empty\r\n", sizeof(buf1) -1);
  strip_cr(buf1);
  mysql_real_escape_string(conn, buf2, buf1, strlen(buf1));
  mysql_real_escape_string(conn, buf1, OLC_HELP(d)->alternate_keywords, strlen(OLC_HELP(d)->alternate_keywords));

  sprintf(buf, "INSERT INTO help_entries (keyword, alternate_keywords, entry, min_level) VALUES (upper('%s'), upper('%s'), '%s', %d)"
               " on duplicate key update"
               "  alternate_keywords = values(alternate_keywords),"
               "  entry = values(entry);",
      OLC_HELP(d)->keyword, buf1, buf2, help_table[i].min_level);

  if (mysql_query(conn, buf)) {
    mudlog(NRM, LVL_STAFF, TRUE, "SYSERR: Unable to INSERT in help_topics: %s", mysql_error(conn));
  }

  /* help_keywords insert goes here */
}

/* The main menu. */
static void hedit_disp_menu(struct descriptor_data *d)
{
  get_char_colors(d->character);

  write_to_output(d,
      "%s-- Help file editor\r\n"
      "%s1%s) Keyword       : %s%s\r\n"
      "%s2%s) Alt. Keywords : %s%s\r\n"
      "%s3%s) Entry         :\r\n%s"
      "%s4%s) Min Level     : %s%d\r\n"
      "%sQ%s) Quit\r\n"
      "Enter choice : ",
       nrm,
       grn, nrm, yel, OLC_HELP(d)->keyword,
       grn, nrm, yel, OLC_HELP(d)->alternate_keywords,
       grn, nrm, OLC_HELP(d)->entry,
       grn, nrm, yel, OLC_HELP(d)->min_level,
       grn, nrm
  );
  OLC_MODE(d) = HEDIT_MAIN_MENU;
}

void hedit_parse(struct descriptor_data *d, char *arg)
{
  char buf[MAX_STRING_LENGTH];
  char *oldtext = '\0';
  int number;
  struct help_entry_list *tmp;

  switch (OLC_MODE(d)) {
  case HEDIT_CONFIRM_SAVESTRING:
    switch (*arg) {
    case 'y':
    case 'Y':
      snprintf(buf, sizeof(buf), "OLC: %s edits help for %s.", GET_NAME(d->character),
               OLC_HELP(d)->keyword);
      mudlog(TRUE, MAX(LVL_BUILDER, GET_INVIS_LEV(d->character)), CMP, "%s", buf);
      write_to_output(d, "Help saved to disk.\r\n");
      hedit_save_internally(d);

      /* Do not free strings, just the help structure. */
      cleanup_olc(d, CLEANUP_STRUCTS);
      break;
    case 'n':
    case 'N':
      /* Free everything up, including strings, etc. */
      cleanup_olc(d, CLEANUP_ALL);
      break;
    default:
      write_to_output(d, "Invalid choice!\r\nDo you wish to save your changes? : \r\n");
      break;
    }
    return;

  case HEDIT_CONFIRM_EDIT:
    switch (*arg)  {
    case 'y': case 'Y':
      hedit_disp_menu(d);
      break;
    case 'q': case 'Q': 
      cleanup_olc(d, CLEANUP_ALL);
      break;       
    case 'n': case 'N':
      if(OLC_HELP(d)->next != NULL) {
        tmp = OLC_HELP(d);
        OLC_HELP(d) = OLC_HELP(d)->next;
        free(tmp);
        
        write_to_output(d, "Do you wish to edit the '%s' help file? ",
            OLC_HELP(d)->keyword);
        OLC_MODE(d) = HEDIT_CONFIRM_EDIT;

      } else {
        write_to_output(d, "Do you wish to add the '%s' help file? ",
            OLC_STORAGE(d));
        OLC_MODE(d) = HEDIT_CONFIRM_ADD;
      }
      break;
    default:
      write_to_output(d, "Invalid choice!\r\n"
                         "Do you wish to edit the '%s' help file? ",
                         OLC_HELP(d)->keyword);
      break;
    }
    return;

  case HEDIT_CONFIRM_ADD:
    switch (*arg)  {
      case 'y': case 'Y':
      hedit_setup_new(d);
      break;
    case 'n': case 'N': case 'q': case 'Q':
      cleanup_olc(d, CLEANUP_ALL);
      break;
    default:
      write_to_output(d, "Invalid choice!\r\n"
                         "Do you wish to add the '%s' help file? ",
                         OLC_STORAGE(d));
      break;
    }
    return;

  case HEDIT_MAIN_MENU:
    switch (*arg) {
    case 'q':
    case 'Q':
      if (OLC_VAL(d)) {
        /* Something has been modified. */
        write_to_output(d, "Do you wish to save your changes? : ");
        OLC_MODE(d) = HEDIT_CONFIRM_SAVESTRING;
      } else {
        write_to_output(d, "No changes made.\r\n");
        cleanup_olc(d, CLEANUP_ALL);
      }
      break;
    case '1': 
      write_to_output(d, "Enter help entry keyword : ");
      OLC_MODE(d) = HEDIT_KEYWORD;
      break;
    case '2':
      write_to_output(d, "Enter alternate keywords : ");
      OLC_MODE(d) = HEDIT_ALT_KEYWORDS;
      break;      
    case '3':
      OLC_MODE(d) = HEDIT_ENTRY;
      clear_screen(d);
      send_editor_help(d);
      write_to_output(d, "Enter help entry: (/s saves /h for help)\r\n");
      if (OLC_HELP(d)->entry) {
        write_to_output(d, "%s", OLC_HELP(d)->entry);
        oldtext = strdup(OLC_HELP(d)->entry);
      }
      string_write(d, &OLC_HELP(d)->entry, MAX_MESSAGE_LENGTH, 0, oldtext);
      OLC_VAL(d) = 1;
      break;
    case '4':
      write_to_output(d, "Enter min level : ");
      OLC_MODE(d) = HEDIT_MIN_LEVEL;
      break;
    default:
      write_to_output(d, "Invalid choice!\r\n");
      hedit_disp_menu(d);
      break;
    }
    return;

  case HEDIT_KEYWORD:
    if (OLC_HELP(d)->keyword)
      free(OLC_HELP(d)->keyword);
    strip_cr(arg);
    OLC_HELP(d)->keyword = str_udup(arg);
    break;

  case HEDIT_ALT_KEYWORDS:
    if (OLC_HELP(d)->alternate_keywords)
      free(OLC_HELP(d)->alternate_keywords);
    if (strlen(arg) > MAX_HELP_KEYWORDS)
      arg[MAX_HELP_KEYWORDS - 1] = '\0';
    strip_cr(arg);
    OLC_HELP(d)->alternate_keywords = str_udup(arg);
    break;

  case HEDIT_ENTRY:
    /* We will NEVER get here, we hope. */
    mudlog(TRUE, LVL_BUILDER, BRF, "SYSERR: Reached HEDIT_ENTRY case in parse_hedit");
    break;

  case HEDIT_MIN_LEVEL:
    number = atoi(arg);
    if ((number < 0) || (number > LVL_IMPL))
      write_to_output(d, "That is not a valid choice!\r\nEnter min level:-\r\n] ");
    else {
      OLC_HELP(d)->min_level = number;
      break;
    }
    return;

  default:
    /* We should never get here. */
    mudlog(TRUE, LVL_BUILDER, BRF, "SYSERR: Reached default case in parse_hedit");
    break;
  }

  /* If we get this far, something has been changed. */
  OLC_VAL(d) = 1;
  hedit_disp_menu(d);
}

void hedit_string_cleanup(struct descriptor_data *d, int terminator)
{
  switch (OLC_MODE(d)) {
  case HEDIT_ENTRY:
    hedit_disp_menu(d);
    break;
  }
}

ACMD(do_helpcheck)
{

  char buf[MAX_STRING_LENGTH];
  int i, count = 0;
  size_t len = 0, nlen;

  for (i = 1; *(complete_cmd_info[i].command) != '\n'; i++) {
    if (complete_cmd_info[i].command_pointer != do_action && complete_cmd_info[i].minimum_level >= 0) {
      if (search_help(complete_cmd_info[i].command, LVL_IMPL) == NULL) {
        nlen = snprintf(buf + len, sizeof(buf) - len, "%-20.20s%s", complete_cmd_info[i].command,
                        (++count % 3 ? "" : "\r\n"));
        if (len + nlen >= sizeof(buf))
          break;
        len += nlen;
      }
    }
  }
  if (count % 3 && len < sizeof(buf))
    nlen = snprintf(buf + len, sizeof(buf) - len, "\r\n");

  if (ch->desc) {
	if (len == 0)
	 send_to_char(ch, "All commands have help entries.\r\n");
	else {
	 send_to_char(ch, "Commands without help entries:\r\n");
	 page_string(ch->desc, buf, TRUE);
	}
  }
}

ACMD(do_hindex)
{
  int len, len2, count = 0, count2=0, i;
  char buf[MAX_STRING_LENGTH], buf2[MAX_STRING_LENGTH];

  skip_spaces(&argument);

  if (!*argument) {
    send_to_char(ch, "Usage: hindex <string>\r\n");
    return;
  }

  len = sprintf(buf, "\t1Help index entries beginning with '%s':\t2\r\n", argument);
  len2 = sprintf(buf2, "\t1Help index entries containing '%s':\t2\r\n", argument);
  for (i = 0; i < top_of_helpt; i++) {
    if (is_abbrev(argument, help_table[i].keywords)
        && (GET_LEVEL(ch) >= help_table[i].min_level))
      len +=
          snprintf(buf + len, sizeof(buf) - len, "%-20.20s%s", help_table[i].keywords,
                   (++count % 3 ? "" : "\r\n"));
    else if (strstr(help_table[i].keywords, argument)
        && (GET_LEVEL(ch) >= help_table[i].min_level))
      len2 +=
          snprintf(buf2 + len2, sizeof(buf2) - len2, "%-20.20s%s", help_table[i].keywords,
                   (++count2 % 3 ? "" : "\r\n"));
  }
  if (count % 3)
    len += snprintf(buf + len, sizeof(buf) - len, "\r\n");
  if (count2 % 3)
    len2 += snprintf(buf2 + len2, sizeof(buf2) - len2, "\r\n");

  if (!count)
    len += snprintf(buf + len, sizeof(buf) - len, "  None.\r\n");
  if (!count2)
    len2 += snprintf(buf2 + len2, sizeof(buf2) - len2, "  None.\r\n");

  // Join the two strings
  len += snprintf(buf + len, sizeof(buf) - len, "%s", buf2);

  len += snprintf(buf + len, sizeof(buf) - len, "\t1Applicable Index Entries: \t3%d\r\n"
                                                 "\t1Total Index Entries: \t3%d\tn\r\n", count + count2, top_of_helpt);

  page_string(ch->desc, buf, TRUE);
}
