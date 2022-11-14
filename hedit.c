/**************************************************************************
 *  File: hedit.c                                      Part of LuminariMUD *
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
  char arg[MAX_INPUT_LENGTH] = {'\0'};
  struct descriptor_data *d;

  /* No building as a mob or while being forced. */
  if (IS_NPC(ch) || !ch->desc || STATE(ch->desc) != CON_PLAYING)
    return;

  if (!can_edit_zone(ch, HEDIT_PERMISSION))
  {
    send_to_char(ch, "You have not been granted access to edit help files.\r\n");
    return;
  }

  for (d = descriptor_list; d; d = d->next)
  {
    if (STATE(d) == CON_HEDIT)
    {
      send_to_char(ch, "Sorry, only one can person can edit help files at a time.\r\n");
      return;
    }
  }

  one_argument(argument, arg, sizeof(arg));

  if (!*arg)
  {
    send_to_char(ch, "Please specify a help entry to edit.\r\n");
    return;
  }

  d = ch->desc;

  /* Give descriptor an OLC structure. */
  if (d->olc)
  {
    mudlog(BRF, LVL_IMMORT, TRUE, "SYSERR: do_oasis: Player already had olc structure.");
    free(d->olc);
  }

  CREATE(d->olc, struct oasis_olc_data, 1);
  OLC_NUM(d) = 0;
  OLC_STORAGE(d) = strdup(arg);

  OLC_HELP(d) = search_help(OLC_STORAGE(d), LVL_IMPL);

  if (OLC_HELP(d) == NULL)
  {
    send_to_char(ch, "Do you wish to add the '%s' help file? ", OLC_STORAGE(d));
    OLC_MODE(d) = HEDIT_CONFIRM_ADD;
  }
  else
  {
    send_to_char(ch, "Do you wish to edit the '%s' help file? ",
                 OLC_HELP(d)->tag);
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

  OLC_HELP(d)->tag = strdup(OLC_STORAGE(d));
  OLC_HELP(d)->keywords = NULL;
  OLC_HELP(d)->entry = strdup("This help file is unfinished.\r\n");
  OLC_HELP(d)->min_level = 0;
  OLC_HELP(d)->last_updated = NULL;
  OLC_VAL(d) = 0;

  hedit_disp_menu(d);
}

static void hedit_save_internally(struct descriptor_data *d)
{
  hedit_save_to_disk(d);
}

static void hedit_save_to_disk(struct descriptor_data *d)
{
  hedit_save_to_db(d);
}

static void hedit_save_to_db(struct descriptor_data *d)
{
  char buf1[MAX_STRING_LENGTH] = {'\0'}, buf2[MAX_STRING_LENGTH] = {'\0'}, buf[MAX_STRING_LENGTH] = {'\0'}; /* Buffers for DML query. */
  int i = 0;
  struct help_keyword_list *keyword;

  if (OLC_HELP(d) == NULL)
    return;

  strncpy(buf1, OLC_HELP(d)->entry ? OLC_HELP(d)->entry : "Empty\r\n", sizeof(buf1) - 1);
  strip_cr(buf1);
  mysql_real_escape_string(conn, buf2, buf1, strlen(buf1));
  //  mysql_real_escape_string(conn, buf1, OLC_HELP(d)->keywords, strlen(OLC_HELP(d)->keywords));

  snprintf(buf, sizeof(buf), "INSERT INTO help_entries (tag, entry, min_level) VALUES (lower('%s'), '%s', %d)"
                             " on duplicate key update"
                             "  min_level = values(min_level),"
                             "  entry = values(entry);",
           OLC_HELP(d)->tag, buf2, help_table[i].min_level);

  if (mysql_query(conn, buf))
  {
    mudlog(NRM, LVL_STAFF, TRUE, "SYSERR: Unable to UPSERT into help_entries: %s", mysql_error(conn));
  }
  /* Clear out the old keywords. */
  snprintf(buf, sizeof(buf), "DELETE from help_keywords where lower(help_tag) = lower('%s')", OLC_HELP(d)->tag);

  if (mysql_query(conn, buf))
  {
    mudlog(NRM, LVL_STAFF, TRUE, "SYSERR: Unable to DELETE from help_keywords: %s", mysql_error(conn));
  }

  /* Insert the new keywords.  */
  for (keyword = OLC_HELP(d)->keyword_list; keyword != NULL; keyword = keyword->next)
  {
    snprintf(buf, sizeof(buf), "INSERT INTO help_keywords (help_tag, keyword) VALUES (lower('%s'), '%s')", OLC_HELP(d)->tag, keyword->keyword);

    if (mysql_query(conn, buf))
    {
      mudlog(NRM, LVL_STAFF, TRUE, "SYSERR: Unable to INSERT into help_keywords: %s", mysql_error(conn));
    }
  }
}

/* The main menu. */
static void hedit_disp_menu(struct descriptor_data *d)
{
  get_char_colors(d->character);

  write_to_output(d,
                  "%s-- Help file editor\r\n"
                  "%s1%s) Tag           : %s%s\r\n"
                  "%s2%s) Keywords      : %s%s\r\n"
                  "%s3%s) Entry         :\r\n%s"
                  "%s4%s) Min Level     : %s%d\r\n"
                  "\r\n"
                  "%sD%s) Delete help entry\r\n"
                  "%sQ%s) Quit\r\n"
                  "Enter choice : ",
                  nrm,
                  grn, nrm, yel, OLC_HELP(d)->tag,
                  grn, nrm, yel, (OLC_HELP(d)->keyword_list == NULL ? "Not set." : "Set."),
                  grn, nrm, OLC_HELP(d)->entry,
                  grn, nrm, yel, OLC_HELP(d)->min_level,
                  grn, nrm,
                  grn, nrm);
  OLC_MODE(d) = HEDIT_MAIN_MENU;
}

static void hedit_disp_keywords_menu(struct descriptor_data *d)
{

  struct help_keyword_list *keyword;

  bool found = FALSE;
  int counter = 0;

  get_char_colors(d->character);
  clear_screen(d);
  write_to_output(d,
                  "Help entry keyword menu\r\n\r\n");

  for (keyword = OLC_HELP(d)->keyword_list; keyword != NULL; keyword = keyword->next)
  {
    counter++;
    found = TRUE;
    write_to_output(d,
                    "%s%d%s) %s%s%s\r\n",
                    grn, counter, nrm, yel, keyword->keyword, nrm);
  }
  if (!found)
    write_to_output(d, "No keywords assigned.\r\n");

  write_to_output(d,
                  "\r\n"
                  "%sN%s) Add a new keyword\r\n"
                  "%sD%s) Delete an existing keyword\r\n"
                  "%sQ%s) Quit\r\n"
                  "Enter choice : ",
                  grn, nrm,
                  grn, nrm,
                  grn, nrm);

  OLC_MODE(d) = HEDIT_KEYWORD_MENU;
}

bool hedit_delete_keyword(struct help_entry_list *entry, int num)
{
  int i;
  bool found = FALSE;
  struct help_keyword_list *keyword = NULL, *prev_keyword = NULL;

  keyword = entry->keyword_list;
  prev_keyword = NULL;

  for (i = 1; (i < num) && (keyword != NULL); i++)
  {
    prev_keyword = keyword;
    keyword = keyword->next;
  }
  /* Check to see if we found the keyword. */
  if ((i == num) && (keyword != NULL))
  {

    found = TRUE;

    /* Remove it from the list. */
    if (prev_keyword == NULL)
      entry->keyword_list = keyword->next;
    else
      prev_keyword->next = keyword->next;

    /* Free up the memory. */
    if (keyword->keyword != NULL)
      free(keyword->keyword);
    if (keyword->tag != NULL)
      free(keyword->tag);
    free(keyword);
    keyword = NULL;
  }

  return found;
}

bool hedit_delete_entry(struct help_entry_list *entry)
{
  char buf[MAX_STRING_LENGTH] = {'\0'}; /* Buffer for DML query. */
  bool retval = TRUE;

  if (entry == NULL)
    return FALSE;

  /* Clear out the old keywords. */
  while (hedit_delete_keyword(entry, 1))
    ;

  snprintf(buf, sizeof(buf), "delete from help_entries where lower(tag) = lower('%s')", entry->tag);
  mudlog(NRM, LVL_STAFF, TRUE, "%s", buf);

  if (mysql_query(conn, buf))
  {
    mudlog(NRM, LVL_STAFF, TRUE, "SYSERR: Unable to delete from help_entries: %s", mysql_error(conn));
    retval = FALSE;
  }
  return retval;
}

void hedit_parse(struct descriptor_data *d, char *arg)
{
  char buf[MAX_STRING_LENGTH] = {'\0'};
  char *oldtext = '\0';
  int number;
  struct help_entry_list *tmp;
  struct help_keyword_list *new_keyword;

  switch (OLC_MODE(d))
  {
  case HEDIT_CONFIRM_SAVESTRING:
    switch (*arg)
    {
    case 'y':
    case 'Y':
      if (OLC_HELP(d)->keyword_list == NULL)
      {
        /*  No keywords! */
        write_to_output(d, "Can not save a help entry with no keywords!  Add at least one keyword first.\r\n");
        hedit_disp_menu(d);
      }
      else
      {
        snprintf(buf, sizeof(buf), "OLC: %s edits help for %s.", GET_NAME(d->character),
                 OLC_HELP(d)->tag);
        mudlog(TRUE, MAX(LVL_BUILDER, GET_INVIS_LEV(d->character)), CMP, "%s", buf);
        write_to_output(d, "Help saved to disk.\r\n");
        hedit_save_internally(d);

        cleanup_olc(d, CLEANUP_ALL);
      }
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
    switch (*arg)
    {
    case 'y':
    case 'Y':
      hedit_disp_menu(d);
      break;
    case 'q':
    case 'Q':
      cleanup_olc(d, CLEANUP_ALL);
      break;
    case 'n':
    case 'N':
      if (OLC_HELP(d)->next != NULL)
      {
        tmp = OLC_HELP(d);
        OLC_HELP(d) = OLC_HELP(d)->next;
        free(tmp);

        write_to_output(d, "Do you wish to edit the '%s' help file? ",
                        OLC_HELP(d)->tag);
        OLC_MODE(d) = HEDIT_CONFIRM_EDIT;
      }
      else
      {
        write_to_output(d, "Do you wish to add the '%s' help file? ",
                        OLC_STORAGE(d));
        OLC_MODE(d) = HEDIT_CONFIRM_ADD;
      }
      break;
    default:
      write_to_output(d, "Invalid choice!\r\n"
                         "Do you wish to edit the '%s' help file? ",
                      OLC_HELP(d)->tag);
      break;
    }
    return;

  case HEDIT_CONFIRM_ADD:
    switch (*arg)
    {
    case 'y':
    case 'Y':
      hedit_setup_new(d);
      break;
    case 'n':
    case 'N':
    case 'q':
    case 'Q':
      cleanup_olc(d, CLEANUP_ALL);
      break;
    default:
      write_to_output(d, "Invalid choice!\r\n"
                         "Do you wish to add the '%s' help file? ",
                      OLC_STORAGE(d));
      break;
    }
    return;
  case HEDIT_CONFIRM_DELETE:
    switch (*arg)
    {
    case 'y':
    case 'Y':
      // Actually delete the help entry and the keywords.
      hedit_delete_entry(OLC_HELP(d));
      cleanup_olc(d, CLEANUP_ALL);
      write_to_output(d, "Help file deleted.\r\n");
      break;
    case 'n':
    case 'N':
    case 'q':
    case 'Q':
      hedit_disp_menu(d);
      break;
    default:
      write_to_output(d, "Invalid choice!\r\n"
                         "re you sure you wish to delete this help entry? : ");
      break;
    }
    return;
  case HEDIT_MAIN_MENU:
    switch (*arg)
    {
    case 'q':
    case 'Q':
      if (OLC_VAL(d))
      {
        /* Something has been modified. */
        write_to_output(d, "Do you wish to save your changes? : ");
        OLC_MODE(d) = HEDIT_CONFIRM_SAVESTRING;
      }
      else
      {
        write_to_output(d, "No changes made.\r\n");
        cleanup_olc(d, CLEANUP_ALL);
      }
      break;
    case 'd':
    case 'D':
      /* Delete this entry */
      write_to_output(d, "Are you sure you wish to delete this help entry? : ");
      OLC_MODE(d) = HEDIT_CONFIRM_DELETE;
      break;
    case '1':
      write_to_output(d, "Enter help entry tag : ");
      OLC_MODE(d) = HEDIT_TAG;
      break;
    case '2':
      hedit_disp_keywords_menu(d);
      break;
    case '3':
      OLC_MODE(d) = HEDIT_ENTRY;
      clear_screen(d);
      send_editor_help(d);
      write_to_output(d, "Enter help entry: (/s saves /h for help)\r\n");
      if (OLC_HELP(d)->entry)
      {
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

  case HEDIT_TAG:
    if (OLC_HELP(d)->tag)
      free(OLC_HELP(d)->tag);
    strip_cr(arg);
    OLC_HELP(d)->tag = str_udup(arg);
    break;

  case HEDIT_KEYWORD_MENU:
    switch (*arg)
    {
    case 'q':
    case 'Q':
      hedit_disp_menu(d);
      break;
    case 'n':
    case 'N':
      write_to_output(d, "Enter new keyword : ");
      OLC_MODE(d) = HEDIT_NEW_KEYWORD;
      break;
    case 'd':
    case 'D':
      write_to_output(d, "Enter keyword number to delete (-1 to cancel): ");
      OLC_MODE(d) = HEDIT_DEL_KEYWORD;
      break;
    default:
      write_to_output(d, "Invalid choice!\r\n");
      hedit_disp_keywords_menu(d);
      break;
    }
    return;

  case HEDIT_NEW_KEYWORD:
    CREATE(new_keyword, struct help_keyword_list, 1);
    new_keyword->tag = strdup(OLC_HELP(d)->tag);
    new_keyword->keyword = str_udup(arg);
    new_keyword->next = OLC_HELP(d)->keyword_list;
    OLC_HELP(d)->keyword_list = new_keyword;
    OLC_VAL(d) = 1;
    hedit_disp_keywords_menu(d);
    return;
    break;

  case HEDIT_DEL_KEYWORD:
    if ((number = atoi(arg)) == -1)
    {
      hedit_disp_keywords_menu(d);
      return;
    }

    if (hedit_delete_keyword(OLC_HELP(d), number))
      write_to_output(d, "Keyword deleted.\r\n");
    else
      write_to_output(d, "That keyword does not exist!\r\n");

    hedit_disp_keywords_menu(d);
    OLC_VAL(d) = 1;
    return;
    break;

  case HEDIT_ENTRY:
    /* We will NEVER get here, we hope. */
    mudlog(TRUE, LVL_BUILDER, BRF, "SYSERR: Reached HEDIT_ENTRY case in parse_hedit");
    break;

  case HEDIT_MIN_LEVEL:
    number = atoi(arg);
    if ((number < 0) || (number > LVL_IMPL))
      write_to_output(d, "That is not a valid choice!\r\nEnter min level:-\r\n] ");
    else
    {
      OLC_HELP(d)->min_level = number;
    }
    break;

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
  switch (OLC_MODE(d))
  {
  case HEDIT_ENTRY:
    hedit_disp_menu(d);
    break;
  }
}

ACMD(do_helpcheck)
{

  char buf[MAX_STRING_LENGTH] = {'\0'};
  int i, count = 0;
  size_t len = 0, nlen;

  for (i = 1; *(complete_cmd_info[i].command) != '\n'; i++)
  {
    if (complete_cmd_info[i].command_pointer != do_action && complete_cmd_info[i].minimum_level >= 0)
    {
      if (search_help(complete_cmd_info[i].command, LVL_IMPL) == NULL)
      {
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

  if (ch->desc)
  {
    if (len == 0)
      send_to_char(ch, "All commands have help entries.\r\n");
    else
    {
      send_to_char(ch, "Commands without help entries:\r\n");
      page_string(ch->desc, buf, TRUE);
    }
  }
}

ACMD(do_hindex)
{
  char buf[MAX_STRING_LENGTH] = {'\0'}, buf2[MAX_STRING_LENGTH] = {'\0'};
  int i = 0, count = 0, count2 = 0, len = 0, len2 = 0;

  skip_spaces_c(&argument);

  if (!*argument)
  {
    send_to_char(ch, "Usage: hindex <string>\r\n");
    return;
  }

  len = snprintf(buf, sizeof(buf), "\t1Help index entries beginning with '%s':\t2\r\n", argument);
  len2 = snprintf(buf2, sizeof(buf2), "\t1Help index entries containing '%s':\t2\r\n", argument);
  for (i = 0; i < top_of_helpt; i++)
  {
    if (is_abbrev(argument, help_table[i].keywords) && (GET_LEVEL(ch) >= help_table[i].min_level))
      len +=
          snprintf(buf + len, sizeof(buf) - len, "%-20.20s%s", help_table[i].keywords,
                   (++count % 3 ? "" : "\r\n"));
    else if (strstr(help_table[i].keywords, argument) && (GET_LEVEL(ch) >= help_table[i].min_level))
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
                                                "\t1Total Index Entries: \t3%d\tn\r\n",
                  count + count2, top_of_helpt);

  page_string(ch->desc, buf, TRUE);
}
