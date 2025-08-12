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

/* Constants for validation */
#define MAX_HELP_TAG_LENGTH 50      /* Maximum length for help tags */
#define MAX_HELP_KEYWORD_LENGTH 50  /* Maximum length for keywords */
#define MIN_TAG_LENGTH 2            /* Minimum length for help tags */
#define MIN_KEYWORD_LENGTH 2        /* Minimum length for keywords */

/* local functions */
static void hedit_disp_menu(struct descriptor_data *);
static void hedit_setup_new(struct descriptor_data *);
static void hedit_save_to_disk(struct descriptor_data *);
static void hedit_save_to_db(struct descriptor_data *);
static void hedit_save_internally(struct descriptor_data *);
static bool validate_help_tag(const char *tag, struct descriptor_data *d);
static bool validate_help_keyword(const char *keyword, struct descriptor_data *d);
static bool validate_help_content(const char *content, struct descriptor_data *d);
static bool validate_min_level(int level, struct descriptor_data *d);

/**
 * Validates a help tag for security and format requirements.
 * 
 * @param tag The tag to validate
 * @param d The descriptor for sending error messages
 * @return TRUE if valid, FALSE otherwise
 * 
 * Validation checks:
 * - Non-empty and within length limits (2-50 characters)
 * - Contains only alphanumeric, spaces, hyphens, underscores
 * - Does not contain SQL injection patterns
 * - Not a SQL reserved word
 */
static bool validate_help_tag(const char *tag, struct descriptor_data *d)
{
  int len;
  const char *p;
  
  /* Check for NULL or empty */
  if (!tag || !*tag) {
    write_to_output(d, "Help tag cannot be empty.\r\n");
    return FALSE;
  }
  
  /* Check length boundaries */
  len = strlen(tag);
  if (len < MIN_TAG_LENGTH) {
    write_to_output(d, "Help tag too short (minimum %d characters).\r\n", MIN_TAG_LENGTH);
    return FALSE;
  }
  if (len > MAX_HELP_TAG_LENGTH) {
    write_to_output(d, "Help tag too long (maximum %d characters).\r\n", MAX_HELP_TAG_LENGTH);
    return FALSE;
  }
  
  /* Validate characters - alphanumeric, spaces, hyphens, underscores only */
  for (p = tag; *p; p++) {
    if (!isalnum(*p) && *p != ' ' && *p != '-' && *p != '_') {
      write_to_output(d, "Invalid character '%c' in help tag. Use only letters, numbers, spaces, hyphens, and underscores.\r\n", *p);
      return FALSE;
    }
  }
  
  /* Check for SQL injection patterns */
  if (strstr(tag, "--") || strchr(tag, ';') || strchr(tag, '\'') || strchr(tag, '\"')) {
    write_to_output(d, "Invalid characters detected in help tag.\r\n");
    return FALSE;
  }
  
  /* Check for SQL reserved words (basic set) */
  if (!str_cmp(tag, "DROP") || !str_cmp(tag, "DELETE") || !str_cmp(tag, "INSERT") ||
      !str_cmp(tag, "UPDATE") || !str_cmp(tag, "SELECT") || !str_cmp(tag, "ALTER")) {
    write_to_output(d, "Cannot use SQL reserved words as help tags.\r\n");
    return FALSE;
  }
  
  return TRUE;
}

/**
 * Validates a help keyword for security and format requirements.
 * 
 * @param keyword The keyword to validate
 * @param d The descriptor for sending error messages  
 * @return TRUE if valid, FALSE otherwise
 * 
 * Similar validation to tags but allows for search-friendly keywords
 */
static bool validate_help_keyword(const char *keyword, struct descriptor_data *d)
{
  int len;
  const char *p;
  
  /* Check for NULL or empty */
  if (!keyword || !*keyword) {
    write_to_output(d, "Keyword cannot be empty.\r\n");
    return FALSE;
  }
  
  /* Check length boundaries */
  len = strlen(keyword);
  if (len < MIN_KEYWORD_LENGTH) {
    write_to_output(d, "Keyword too short (minimum %d characters).\r\n", MIN_KEYWORD_LENGTH);
    return FALSE;
  }
  if (len > MAX_HELP_KEYWORD_LENGTH) {
    write_to_output(d, "Keyword too long (maximum %d characters).\r\n", MAX_HELP_KEYWORD_LENGTH);
    return FALSE;
  }
  
  /* Validate characters - alphanumeric, spaces, hyphens, underscores, apostrophes (for contractions) */
  for (p = keyword; *p; p++) {
    if (!isalnum(*p) && *p != ' ' && *p != '-' && *p != '_' && *p != '\'') {
      write_to_output(d, "Invalid character '%c' in keyword.\r\n", *p);
      return FALSE;
    }
  }
  
  /* Check for SQL injection patterns (but allow single apostrophe for contractions) */
  if (strstr(keyword, "--") || strchr(keyword, ';') || strstr(keyword, "''")) {
    write_to_output(d, "Invalid character sequence in keyword.\r\n");
    return FALSE;
  }
  
  return TRUE;
}

/**
 * Validates help content for security and size requirements.
 * 
 * @param content The help content to validate
 * @param d The descriptor for sending error messages
 * @return TRUE if valid, FALSE otherwise
 * 
 * Checks for dangerous content and size limits
 */
static bool validate_help_content(const char *content, struct descriptor_data *d)
{
  int len;
  
  /* Allow empty content (will be filled later) */
  if (!content) {
    return TRUE;
  }
  
  /* Check length */
  len = strlen(content);
  if (len > MAX_STRING_LENGTH) {
    write_to_output(d, "Help content too long (maximum %d characters).\r\n", MAX_STRING_LENGTH);
    return FALSE;
  }
  
  /* Check for script/HTML injection attempts */
  if (strstr(content, "<script") || strstr(content, "<iframe") || 
      strstr(content, "javascript:") || strstr(content, "onclick")) {
    write_to_output(d, "HTML/Script tags are not allowed in help content.\r\n");
    return FALSE;
  }
  
  return TRUE;
}

/**
 * Validates minimum level requirement for help entries.
 * 
 * @param level The minimum level to validate
 * @param d The descriptor for sending error messages
 * @return TRUE if valid, FALSE otherwise
 */
static bool validate_min_level(int level, struct descriptor_data *d)
{
  /* Check range - 0 to LVL_IMPL (typically 36) */
  if (level < 0) {
    write_to_output(d, "Minimum level cannot be negative.\r\n");
    return FALSE;
  }
  if (level > LVL_IMPL) {
    write_to_output(d, "Minimum level cannot exceed %d.\r\n", LVL_IMPL);
    return FALSE;
  }
  
  return TRUE;
}

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
      send_to_char(ch, "Sorry, only one person can edit help files at a time.\r\n");
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
  char buf1[MAX_STRING_LENGTH] = {'\0'};
  struct help_keyword_list *keyword;
  PREPARED_STMT *pstmt;
  char tag_lower[MAX_HELP_TAG_LENGTH + 1];
  int i;

  if (OLC_HELP(d) == NULL)
    return;

  /* Validate all data before attempting to save to database */
  if (!validate_help_tag(OLC_HELP(d)->tag, d)) {
    write_to_output(d, "Cannot save: Invalid help tag.\r\n");
    return;
  }
  
  if (!validate_help_content(OLC_HELP(d)->entry, d)) {
    write_to_output(d, "Cannot save: Invalid help content.\r\n");
    return;
  }
  
  if (!validate_min_level(OLC_HELP(d)->min_level, d)) {
    write_to_output(d, "Cannot save: Invalid minimum level.\r\n");
    return;
  }
  
  /* Validate all keywords */
  for (keyword = OLC_HELP(d)->keyword_list; keyword != NULL; keyword = keyword->next) {
    if (!validate_help_keyword(keyword->keyword, d)) {
      write_to_output(d, "Cannot save: Invalid keyword '%s'.\r\n", keyword->keyword);
      return;
    }
  }

  /* Prepare help entry content */
  strncpy(buf1, OLC_HELP(d)->entry ? OLC_HELP(d)->entry : "Empty\r\n", sizeof(buf1) - 1);
  strip_cr(buf1);

  /* Convert tag to lowercase for storage */
  for (i = 0; OLC_HELP(d)->tag[i] && i < MAX_HELP_TAG_LENGTH; i++) {
    tag_lower[i] = LOWER(OLC_HELP(d)->tag[i]);
  }
  tag_lower[i] = '\0';

  /* === INSERT/UPDATE HELP ENTRY === */
  /* Use prepared statement for UPSERT - completely SQL injection safe */
  pstmt = mysql_stmt_create(conn);
  if (!pstmt) {
    mudlog(NRM, LVL_STAFF, TRUE, "SYSERR: Failed to create prepared statement for help entry save");
    return;
  }

  /* Prepare UPSERT query with parameters */
  if (!mysql_stmt_prepare_query(pstmt,
      "INSERT INTO help_entries (tag, entry, min_level) VALUES (?, ?, ?) "
      "ON DUPLICATE KEY UPDATE "
      "min_level = VALUES(min_level), "
      "entry = VALUES(entry)")) {
    mudlog(NRM, LVL_STAFF, TRUE, "SYSERR: Failed to prepare help entry UPSERT query");
    mysql_stmt_cleanup(pstmt);
    return;
  }

  /* Bind parameters for the UPSERT */
  if (!mysql_stmt_bind_param_string(pstmt, 0, tag_lower) ||
      !mysql_stmt_bind_param_string(pstmt, 1, buf1) ||
      !mysql_stmt_bind_param_int(pstmt, 2, OLC_HELP(d)->min_level)) {
    mudlog(NRM, LVL_STAFF, TRUE, "SYSERR: Failed to bind parameters for help entry UPSERT");
    mysql_stmt_cleanup(pstmt);
    return;
  }

  /* Execute the UPSERT */
  if (!mysql_stmt_execute_prepared(pstmt)) {
    mudlog(NRM, LVL_STAFF, TRUE, "SYSERR: Failed to execute help entry UPSERT");
  }

  mysql_stmt_cleanup(pstmt);

  /* === DELETE OLD KEYWORDS === */
  pstmt = mysql_stmt_create(conn);
  if (!pstmt) {
    mudlog(NRM, LVL_STAFF, TRUE, "SYSERR: Failed to create prepared statement for keyword deletion");
    return;
  }

  /* Prepare DELETE query with parameter */
  if (!mysql_stmt_prepare_query(pstmt,
      "DELETE FROM help_keywords WHERE LOWER(help_tag) = ?")) {
    mudlog(NRM, LVL_STAFF, TRUE, "SYSERR: Failed to prepare keyword deletion query");
    mysql_stmt_cleanup(pstmt);
    return;
  }

  /* Bind the tag parameter */
  if (!mysql_stmt_bind_param_string(pstmt, 0, tag_lower)) {
    mudlog(NRM, LVL_STAFF, TRUE, "SYSERR: Failed to bind parameter for keyword deletion");
    mysql_stmt_cleanup(pstmt);
    return;
  }

  /* Execute the DELETE */
  if (!mysql_stmt_execute_prepared(pstmt)) {
    mudlog(NRM, LVL_STAFF, TRUE, "SYSERR: Failed to execute keyword deletion");
  }

  mysql_stmt_cleanup(pstmt);

  /* === INSERT NEW KEYWORDS === */
  /* Create one prepared statement and reuse it for all keywords */
  pstmt = mysql_stmt_create(conn);
  if (!pstmt) {
    mudlog(NRM, LVL_STAFF, TRUE, "SYSERR: Failed to create prepared statement for keyword insertion");
    return;
  }

  /* Prepare INSERT query with parameters */
  if (!mysql_stmt_prepare_query(pstmt,
      "INSERT INTO help_keywords (help_tag, keyword) VALUES (?, ?)")) {
    mudlog(NRM, LVL_STAFF, TRUE, "SYSERR: Failed to prepare keyword insertion query");
    mysql_stmt_cleanup(pstmt);
    return;
  }

  /* Insert each keyword using the same prepared statement */
  for (keyword = OLC_HELP(d)->keyword_list; keyword != NULL; keyword = keyword->next) {
    /* Bind parameters for this keyword */
    if (!mysql_stmt_bind_param_string(pstmt, 0, tag_lower) ||
        !mysql_stmt_bind_param_string(pstmt, 1, keyword->keyword)) {
      mudlog(NRM, LVL_STAFF, TRUE, "SYSERR: Failed to bind parameters for keyword '%s'", keyword->keyword);
      continue;
    }

    /* Execute the INSERT */
    if (!mysql_stmt_execute_prepared(pstmt)) {
      mudlog(NRM, LVL_STAFF, TRUE, "SYSERR: Failed to insert keyword '%s'", keyword->keyword);
    }
  }

  mysql_stmt_cleanup(pstmt);
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

  char *escaped_tag = mysql_escape_string_alloc(conn, entry->tag);
  if (!escaped_tag) {
    log("SYSERR: Failed to escape help tag in hedit_delete_entry");
    return FALSE;
  }
  snprintf(buf, sizeof(buf), "delete from help_entries where lower(tag) = lower('%s')", escaped_tag);
  mudlog(NRM, LVL_STAFF, TRUE, "%s", buf);
  free(escaped_tag);

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
    strip_cr(arg);
    /* Validate the tag before accepting it */
    if (!validate_help_tag(arg, d)) {
      write_to_output(d, "Enter help entry tag : ");
      return;
    }
    if (OLC_HELP(d)->tag)
      free(OLC_HELP(d)->tag);
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
    strip_cr(arg);
    /* Validate the keyword before accepting it */
    if (!validate_help_keyword(arg, d)) {
      write_to_output(d, "Enter new keyword : ");
      return;
    }
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
    /* Use validation function for consistency and better error messages */
    if (!validate_min_level(number, d)) {
      write_to_output(d, "Enter min level : ");
      return;
    }
    OLC_HELP(d)->min_level = number;
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
  /* This command now uses MySQL database instead of legacy help_table
   * It queries help_keywords table to find all keywords matching the search pattern
   * Two types of matches: keywords beginning with pattern, and keywords containing pattern */
  
  char buf[MAX_STRING_LENGTH] = {'\0'}, buf2[MAX_STRING_LENGTH] = {'\0'};
  char query[MAX_STRING_LENGTH];
  char *escaped_arg;
  MYSQL_RES *result;
  MYSQL_ROW row;
  int count = 0, count2 = 0, len = 0, len2 = 0, total_entries = 0;

  skip_spaces_c(&argument);

  if (!*argument)
  {
    send_to_char(ch, "Usage: hindex <string>\r\n");
    return;
  }

  /* Escape the search argument to prevent SQL injection */
  escaped_arg = (char *)malloc(strlen(argument) * 2 + 1);
  if (!escaped_arg) {
    send_to_char(ch, "Memory allocation error.\r\n");
    return;
  }
  mysql_real_escape_string(conn, escaped_arg, argument, strlen(argument));

  len = snprintf(buf, sizeof(buf), "\t1Help index entries beginning with '%s':\t2\r\n", argument);
  len2 = snprintf(buf2, sizeof(buf2), "\t1Help index entries containing '%s':\t2\r\n", argument);

  /* Query 1: Find keywords beginning with the search pattern */
  snprintf(query, sizeof(query),
    "SELECT DISTINCT hk.keyword "
    "FROM help_keywords hk "
    "JOIN help_entries he ON hk.help_tag = he.tag "
    "WHERE LOWER(hk.keyword) LIKE LOWER('%s%%') "
    "AND he.min_level <= %d "
    "ORDER BY hk.keyword",
    escaped_arg, GET_LEVEL(ch));

  if (mysql_query(conn, query) == 0) {
    result = mysql_store_result(conn);
    if (result) {
      while ((row = mysql_fetch_row(result))) {
        if (row[0]) {
          len += snprintf(buf + len, sizeof(buf) - len, "%-20.20s%s", 
                         row[0], (++count % 3 ? "" : "\r\n"));
        }
      }
      mysql_free_result(result);
    }
  }

  /* Query 2: Find keywords containing the search pattern (but not beginning with it) */
  snprintf(query, sizeof(query),
    "SELECT DISTINCT hk.keyword "
    "FROM help_keywords hk "
    "JOIN help_entries he ON hk.help_tag = he.tag "
    "WHERE LOWER(hk.keyword) LIKE LOWER('%%_%s%%') "  /* Use %%_ to exclude beginning matches */
    "AND LOWER(hk.keyword) NOT LIKE LOWER('%s%%') "   /* Exclude already found entries */
    "AND he.min_level <= %d "
    "ORDER BY hk.keyword",
    escaped_arg, escaped_arg, GET_LEVEL(ch));

  if (mysql_query(conn, query) == 0) {
    result = mysql_store_result(conn);
    if (result) {
      while ((row = mysql_fetch_row(result))) {
        if (row[0]) {
          len2 += snprintf(buf2 + len2, sizeof(buf2) - len2, "%-20.20s%s",
                          row[0], (++count2 % 3 ? "" : "\r\n"));
        }
      }
      mysql_free_result(result);
    }
  }

  /* Get total count of help entries accessible to this player */
  snprintf(query, sizeof(query),
    "SELECT COUNT(DISTINCT he.tag) "
    "FROM help_entries he "
    "WHERE he.min_level <= %d",
    GET_LEVEL(ch));

  if (mysql_query(conn, query) == 0) {
    result = mysql_store_result(conn);
    if (result) {
      row = mysql_fetch_row(result);
      if (row && row[0]) {
        total_entries = atoi(row[0]);
      }
      mysql_free_result(result);
    }
  }

  free(escaped_arg);

  /* Format the output */
  if (count % 3)
    len += snprintf(buf + len, sizeof(buf) - len, "\r\n");
  if (count2 % 3)
    len2 += snprintf(buf2 + len2, sizeof(buf2) - len2, "\r\n");

  if (!count)
    len += snprintf(buf + len, sizeof(buf) - len, "  None.\r\n");
  if (!count2)
    len2 += snprintf(buf2 + len2, sizeof(buf2) - len2, "  None.\r\n");

  /* Join the two strings */
  len += snprintf(buf + len, sizeof(buf) - len, "%s", buf2);

  len += snprintf(buf + len, sizeof(buf) - len, "\t1Applicable Index Entries: \t3%d\r\n"
                                                "\t1Total Index Entries: \t3%d\tn\r\n",
                  count + count2, total_entries);

  page_string(ch->desc, buf, TRUE);
}
