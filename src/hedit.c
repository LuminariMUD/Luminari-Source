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
#include <sys/stat.h>  /* For file stat */
#include <time.h>      /* For time functions */
#include <ctype.h>     /* For isspace */

/* Constants for validation */
#define MAX_HELP_TAG_LENGTH 50      /* Maximum length for help tags */
#define MAX_HELP_KEYWORD_LENGTH 50  /* Maximum length for keywords */
#define MIN_TAG_LENGTH 2            /* Minimum length for help tags */
#define MIN_KEYWORD_LENGTH 2        /* Minimum length for keywords */
#define MAX_KEYWORDS_PER_ENTRY 20   /* Maximum keywords per help entry to prevent memory exhaustion */

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

/* Import functionality */
static int import_help_hlp_file(struct char_data *ch, const char *mode);
static struct help_entry_list *parse_help_entry(FILE *fp, int *min_level);

/* Export functionality */
static int export_help_to_hlp(struct char_data *ch, const char *options);
static int write_help_entry_to_file(FILE *fp, const char *keywords, const char *content, int min_level);
static int import_entry_with_resolution(struct char_data *ch, struct help_entry_list *entry, int min_level, const char *mode, char *msg_buf, size_t msg_size);
static void free_help_entry(struct help_entry_list *entry);

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
  
  /* Immediately set state to CON_HEDIT to claim the lock atomically */
  struct descriptor_data *other_d;
  
  STATE(d) = CON_HEDIT;
  
  /* Now check if another editor is already active (besides us) */
  for (other_d = descriptor_list; other_d; other_d = other_d->next)
  {
    if (other_d != d && STATE(other_d) == CON_HEDIT)
    {
      /* Another editor is active, release our lock and cleanup */
      STATE(d) = CON_PLAYING;
      free(d->olc);
      d->olc = NULL;
      send_to_char(ch, "Sorry, only one person can edit help files at a time.\r\n");
      return;
    }
  }
  
  OLC_STORAGE(d) = strdup(arg);
  if (!OLC_STORAGE(d)) {
    log("SYSERR: do_oasis_hedit: strdup failed for OLC_STORAGE");
    STATE(d) = CON_PLAYING;  /* Release lock on error */
    free(d->olc);
    d->olc = NULL;
    send_to_char(ch, "Memory allocation error. Please try again.\r\n");
    return;
  }

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

  /* STATE already set to CON_HEDIT above for atomic lock acquisition */
  act("$n starts editing help files.", TRUE, d->character, 0, 0, TO_ROOM);
  SET_BIT_AR(PLR_FLAGS(ch), PLR_WRITING);
  mudlog(CMP, LVL_IMMORT, TRUE, "OLC: %s starts editing help files.", GET_NAME(d->character));
}

static void hedit_setup_new(struct descriptor_data *d)
{
  CREATE(OLC_HELP(d), struct help_entry_list, 1);

  OLC_HELP(d)->tag = strdup(OLC_STORAGE(d));
  if (!OLC_HELP(d)->tag) {
    log("SYSERR: hedit_setup_new: strdup failed for tag");
    cleanup_olc(d, CLEANUP_ALL);
    return;
  }
  
  OLC_HELP(d)->keywords = NULL;
  OLC_HELP(d)->entry = strdup("This help file is unfinished.\r\n");
  if (!OLC_HELP(d)->entry) {
    log("SYSERR: hedit_setup_new: strdup failed for entry");
    free(OLC_HELP(d)->tag);
    cleanup_olc(d, CLEANUP_ALL);
    return;
  }
  
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
  struct help_keyword_list *keyword, *existing_keywords = NULL, *temp_keyword;
  PREPARED_STMT *pstmt;
  char tag_lower[MAX_HELP_TAG_LENGTH + 1];
  int i, transaction_started = 0, error_occurred = 0;
  int keyword_count = 0;

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
  
  /* Validate and count all keywords */
  for (keyword = OLC_HELP(d)->keyword_list; keyword != NULL; keyword = keyword->next) {
    if (!validate_help_keyword(keyword->keyword, d)) {
      write_to_output(d, "Cannot save: Invalid keyword '%s'.\r\n", keyword->keyword);
      return;
    }
    keyword_count++;
    if (keyword_count > MAX_KEYWORDS_PER_ENTRY) {
      write_to_output(d, "Cannot save: Too many keywords (maximum %d).\r\n", MAX_KEYWORDS_PER_ENTRY);
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

  /* START TRANSACTION - All database operations must succeed or all will be rolled back */
  if (mysql_query(conn, "START TRANSACTION")) {
    mudlog(NRM, LVL_STAFF, TRUE, "SYSERR: Failed to start transaction for help save: %s", mysql_error(conn));
    write_to_output(d, "Database error: Failed to start transaction. Help entry not saved.\r\n");
    return;
  }
  transaction_started = 1;

  /* === SAVE CURRENT VERSION TO HISTORY (if entry exists) === */
  /* First, check if the entry exists and save current version to history */
  pstmt = mysql_stmt_create(conn);
  if (!pstmt) {
    mudlog(NRM, LVL_STAFF, TRUE, "SYSERR: Failed to create prepared statement for version history");
    write_to_output(d, "Database error: Failed to prepare version history.\r\n");
    error_occurred = 1;
    goto cleanup;
  }
  
  /* Archive current version if it exists */
  if (!mysql_stmt_prepare_query(pstmt,
      "INSERT INTO help_versions (tag, entry, min_level, saved_by, version_date) "
      "SELECT tag, entry, min_level, ?, last_updated "
      "FROM help_entries WHERE tag = ?")) {
    mudlog(NRM, LVL_STAFF, TRUE, "SYSERR: Failed to prepare version history query");
    /* Don't fail the save, just log the error - versioning is optional */
  } else {
    char *editor_name = GET_NAME(d->character) ? GET_NAME(d->character) : "Unknown";
    if (mysql_stmt_bind_param_string(pstmt, 0, editor_name) &&
        mysql_stmt_bind_param_string(pstmt, 1, tag_lower)) {
      mysql_stmt_execute_prepared(pstmt);
      /* Don't check for errors - it's ok if versioning fails */
    }
  }
  mysql_stmt_cleanup(pstmt);
  
  /* === INSERT/UPDATE HELP ENTRY === */
  /* Use prepared statement for UPSERT - completely SQL injection safe */
  pstmt = mysql_stmt_create(conn);
  if (!pstmt) {
    mudlog(NRM, LVL_STAFF, TRUE, "SYSERR: Failed to create prepared statement for help entry save");
    write_to_output(d, "Database error: Failed to prepare save operation.\r\n");
    error_occurred = 1;
    goto cleanup;
  }

  /* Prepare UPSERT query with parameters */
  if (!mysql_stmt_prepare_query(pstmt,
      "INSERT INTO help_entries (tag, entry, min_level) VALUES (?, ?, ?) "
      "ON DUPLICATE KEY UPDATE "
      "min_level = VALUES(min_level), "
      "entry = VALUES(entry)")) {
    mudlog(NRM, LVL_STAFF, TRUE, "SYSERR: Failed to prepare help entry UPSERT query");
    write_to_output(d, "Database error: Failed to prepare help entry update.\r\n");
    mysql_stmt_cleanup(pstmt);
    error_occurred = 1;
    goto cleanup;
  }

  /* Bind parameters for the UPSERT */
  if (!mysql_stmt_bind_param_string(pstmt, 0, tag_lower) ||
      !mysql_stmt_bind_param_string(pstmt, 1, buf1) ||
      !mysql_stmt_bind_param_int(pstmt, 2, OLC_HELP(d)->min_level)) {
    mudlog(NRM, LVL_STAFF, TRUE, "SYSERR: Failed to bind parameters for help entry UPSERT");
    write_to_output(d, "Database error: Failed to bind parameters.\r\n");
    mysql_stmt_cleanup(pstmt);
    error_occurred = 1;
    goto cleanup;
  }

  /* Execute the UPSERT */
  if (!mysql_stmt_execute_prepared(pstmt)) {
    mudlog(NRM, LVL_STAFF, TRUE, "SYSERR: Failed to execute help entry UPSERT");
    write_to_output(d, "Database error: Failed to save help entry.\r\n");
    mysql_stmt_cleanup(pstmt);
    error_occurred = 1;
    goto cleanup;
  }

  mysql_stmt_cleanup(pstmt);

  /* === DIFFERENTIAL KEYWORD UPDATE === */
  /* First, get existing keywords for this help entry using prepared statement */
  MYSQL_RES *result;
  MYSQL_ROW row;
  
  pstmt = mysql_stmt_create(conn);
  if (!pstmt) {
    mudlog(NRM, LVL_STAFF, TRUE, "SYSERR: Failed to create prepared statement for keyword fetch");
    error_occurred = 1;
    goto cleanup;
  }
  
  if (!mysql_stmt_prepare_query(pstmt, "SELECT keyword FROM help_keywords WHERE LOWER(help_tag) = ?")) {
    mudlog(NRM, LVL_STAFF, TRUE, "SYSERR: Failed to prepare keyword fetch query");
    mysql_stmt_cleanup(pstmt);
    error_occurred = 1;
    goto cleanup;
  }
  
  if (!mysql_stmt_bind_param_string(pstmt, 0, tag_lower)) {
    mudlog(NRM, LVL_STAFF, TRUE, "SYSERR: Failed to bind parameter for keyword fetch");
    mysql_stmt_cleanup(pstmt);
    error_occurred = 1;
    goto cleanup;
  }
  
  if (!mysql_stmt_execute_prepared(pstmt)) {
    mudlog(NRM, LVL_STAFF, TRUE, "SYSERR: Failed to execute keyword fetch");
    mysql_stmt_cleanup(pstmt);
    error_occurred = 1;
    goto cleanup;
  }
  
  mysql_stmt_cleanup(pstmt);
  
  result = mysql_store_result(conn);
  if (result) {
    while ((row = mysql_fetch_row(result))) {
      CREATE(temp_keyword, struct help_keyword_list, 1);
      CREATE(temp_keyword->keyword, char, strlen(row[0]) + 1);
      strcpy(temp_keyword->keyword, row[0]);
      temp_keyword->next = existing_keywords;
      existing_keywords = temp_keyword;
    }
    mysql_free_result(result);
  }

  /* Delete keywords that are no longer in the list */
  pstmt = mysql_stmt_create(conn);
  if (pstmt) {
    if (mysql_stmt_prepare_query(pstmt,
        "DELETE FROM help_keywords WHERE LOWER(help_tag) = ? AND keyword = ?")) {
      mysql_stmt_cleanup(pstmt);
      pstmt = NULL;
    }
  }
  
  if (pstmt) {
    for (temp_keyword = existing_keywords; temp_keyword; temp_keyword = temp_keyword->next) {
      int found = 0;
      for (keyword = OLC_HELP(d)->keyword_list; keyword; keyword = keyword->next) {
        if (!strcmp(temp_keyword->keyword, keyword->keyword)) {
          found = 1;
          break;
        }
      }
      if (!found) {
        /* This keyword should be deleted */
        if (mysql_stmt_bind_param_string(pstmt, 0, tag_lower) &&
            mysql_stmt_bind_param_string(pstmt, 1, temp_keyword->keyword)) {
          mysql_stmt_execute_prepared(pstmt);
        }
      }
    }
    mysql_stmt_cleanup(pstmt);
  }

  /* Insert only new keywords */
  pstmt = mysql_stmt_create(conn);
  if (!pstmt) {
    mudlog(NRM, LVL_STAFF, TRUE, "SYSERR: Failed to create prepared statement for keyword insertion");
    write_to_output(d, "Database error: Failed to prepare keyword update.\r\n");
    error_occurred = 1;
    goto cleanup;
  }

  /* Use INSERT IGNORE to avoid duplicates efficiently */
  if (!mysql_stmt_prepare_query(pstmt,
      "INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES (?, ?)")) {
    mudlog(NRM, LVL_STAFF, TRUE, "SYSERR: Failed to prepare keyword insertion query");
    write_to_output(d, "Database error: Failed to prepare keyword insertion.\r\n");
    mysql_stmt_cleanup(pstmt);
    error_occurred = 1;
    goto cleanup;
  }

  /* Insert each keyword using the same prepared statement */
  for (keyword = OLC_HELP(d)->keyword_list; keyword != NULL; keyword = keyword->next) {
    /* Bind parameters for this keyword */
    if (!mysql_stmt_bind_param_string(pstmt, 0, tag_lower) ||
        !mysql_stmt_bind_param_string(pstmt, 1, keyword->keyword)) {
      mudlog(NRM, LVL_STAFF, TRUE, "SYSERR: Failed to bind parameters for keyword '%s'", keyword->keyword);
      write_to_output(d, "Warning: Failed to save keyword '%s'.\r\n", keyword->keyword);
      error_occurred = 1;
      mysql_stmt_cleanup(pstmt);
      goto cleanup;
    }

    /* Execute the INSERT */
    if (!mysql_stmt_execute_prepared(pstmt)) {
      mudlog(NRM, LVL_STAFF, TRUE, "SYSERR: Failed to insert keyword '%s'", keyword->keyword);
      write_to_output(d, "Warning: Failed to save keyword '%s'.\r\n", keyword->keyword);
      error_occurred = 1;
      mysql_stmt_cleanup(pstmt);
      goto cleanup;
    }
  }

  mysql_stmt_cleanup(pstmt);

cleanup:
  /* Free existing keywords list */
  while (existing_keywords) {
    temp_keyword = existing_keywords;
    existing_keywords = existing_keywords->next;
    if (temp_keyword->keyword)
      free(temp_keyword->keyword);
    free(temp_keyword);
  }

  /* COMMIT or ROLLBACK transaction based on error status */
  if (transaction_started) {
    if (error_occurred) {
      if (mysql_query(conn, "ROLLBACK")) {
        mudlog(NRM, LVL_STAFF, TRUE, "SYSERR: Failed to rollback transaction: %s", mysql_error(conn));
      }
      write_to_output(d, "Database transaction failed. All changes have been rolled back.\r\n");
    } else {
      if (mysql_query(conn, "COMMIT")) {
        mudlog(NRM, LVL_STAFF, TRUE, "SYSERR: Failed to commit transaction: %s", mysql_error(conn));
        write_to_output(d, "Database error: Failed to commit changes.\r\n");
        /* Try to rollback */
        mysql_query(conn, "ROLLBACK");
      } else {
        write_to_output(d, "Help entry '%s' saved successfully to database.\r\n", OLC_HELP(d)->tag);
      }
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
  bool retval = TRUE;

  if (entry == NULL)
    return FALSE;

  /* Clear out the old keywords. */
  while (hedit_delete_keyword(entry, 1))
    ;

  /* Use prepared statement for safe deletion */
  PREPARED_STMT *pstmt = mysql_stmt_create(conn);
  if (!pstmt) {
    log("SYSERR: Failed to create prepared statement for help entry deletion");
    return FALSE;
  }

  /* Prepare DELETE query with parameter */
  if (!mysql_stmt_prepare_query(pstmt,
      "DELETE FROM help_entries WHERE LOWER(tag) = LOWER(?)")) {
    log("SYSERR: Failed to prepare help entry deletion query");
    mysql_stmt_cleanup(pstmt);
    return FALSE;
  }

  /* Bind the tag parameter */
  if (!mysql_stmt_bind_param_string(pstmt, 0, entry->tag)) {
    log("SYSERR: Failed to bind tag parameter for help entry deletion");
    mysql_stmt_cleanup(pstmt);
    return FALSE;
  }

  /* Execute the delete */
  if (!mysql_stmt_execute_prepared(pstmt)) {
    mudlog(NRM, LVL_STAFF, TRUE, "SYSERR: Unable to delete from help_entries");
    mysql_stmt_cleanup(pstmt);
    retval = FALSE;
  } else {
    mudlog(NRM, LVL_STAFF, TRUE, "Deleted help entry: %s", entry->tag);
  }

  mysql_stmt_cleanup(pstmt);
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
        if (!oldtext) {
          write_to_output(d, "Memory allocation error. Aborting.\r\n");
          cleanup_olc(d, CLEANUP_ALL);
          return;
        }
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
    
    /* Count existing keywords to prevent memory exhaustion */
    {
      int keyword_count = 0;
      struct help_keyword_list *temp_kw;
      for (temp_kw = OLC_HELP(d)->keyword_list; temp_kw; temp_kw = temp_kw->next) {
        keyword_count++;
      }
      if (keyword_count >= MAX_KEYWORDS_PER_ENTRY) {
        write_to_output(d, "Maximum number of keywords (%d) reached. Delete some keywords first.\r\n", 
                        MAX_KEYWORDS_PER_ENTRY);
        hedit_disp_keywords_menu(d);
        return;
      }
    }
    
    CREATE(new_keyword, struct help_keyword_list, 1);
    new_keyword->tag = strdup(OLC_HELP(d)->tag);
    if (!new_keyword->tag) {
      write_to_output(d, "Memory allocation error. Keyword not added.\r\n");
      free(new_keyword);
      hedit_disp_menu(d);
      return;
    }
    new_keyword->keyword = str_udup(arg);
    if (!new_keyword->keyword) {
      write_to_output(d, "Memory allocation error. Keyword not added.\r\n");
      free(new_keyword->tag);
      free(new_keyword);
      hedit_disp_menu(d);
      return;
    }
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
  char search_pattern[MAX_INPUT_LENGTH + 2];
  PREPARED_STMT *pstmt;
  int count = 0, count2 = 0, len = 0, len2 = 0, total_entries = 0;

  skip_spaces_c(&argument);

  if (!*argument)
  {
    send_to_char(ch, "Usage: hindex <string>\r\n");
    return;
  }

  /* No need for manual escaping - will use prepared statements */

  len = snprintf(buf, sizeof(buf), "\t1Help index entries beginning with '%s':\t2\r\n", argument);
  len2 = snprintf(buf2, sizeof(buf2), "\t1Help index entries containing '%s':\t2\r\n", argument);

  /* Query 1: Find keywords beginning with the search pattern - use prepared statement */
  pstmt = mysql_stmt_create(conn);
  if (!pstmt) {
    send_to_char(ch, "Database error - unable to create statement.\r\n");
    return;
  }

  /* Prepare query with LIKE pattern */
  if (!mysql_stmt_prepare_query(pstmt,
      "SELECT DISTINCT hk.keyword "
      "FROM help_keywords hk "
      "JOIN help_entries he ON hk.help_tag = he.tag "
      "WHERE LOWER(hk.keyword) LIKE LOWER(?) "
      "AND he.min_level <= ? "
      "ORDER BY hk.keyword")) {
    send_to_char(ch, "Database error - unable to prepare query.\r\n");
    mysql_stmt_cleanup(pstmt);
    return;
  }

  /* Create search pattern with % for LIKE */
  snprintf(search_pattern, sizeof(search_pattern), "%s%%", argument);

  /* Bind parameters */
  if (!mysql_stmt_bind_param_string(pstmt, 0, search_pattern) ||
      !mysql_stmt_bind_param_int(pstmt, 1, GET_LEVEL(ch))) {
    send_to_char(ch, "Database error - unable to bind parameters.\r\n");
    mysql_stmt_cleanup(pstmt);
    return;
  }

  /* Execute and fetch results */
  if (!mysql_stmt_execute_prepared(pstmt)) {
    send_to_char(ch, "Database error - unable to execute query.\r\n");
    mysql_stmt_cleanup(pstmt);
    return;
  }
  
  /* Fetch rows using prepared statement API */
  while (mysql_stmt_fetch_row(pstmt)) {
    char *tag = mysql_stmt_get_string(pstmt, 0);
    if (tag) {
      len += snprintf(buf + len, sizeof(buf) - len, "%-20.20s%s", 
                     tag, (++count % 3 ? "" : "\r\n"));
    }
  }

  mysql_stmt_cleanup(pstmt);

  /* Query 2: Find keywords containing the search pattern (but not beginning with it) */
  pstmt = mysql_stmt_create(conn);
  if (pstmt) {  /* Don't fail entirely if second query fails */
    if (mysql_stmt_prepare_query(pstmt,
        "SELECT DISTINCT hk.keyword "
        "FROM help_keywords hk "
        "JOIN help_entries he ON hk.help_tag = he.tag "
        "WHERE LOWER(hk.keyword) LIKE LOWER(?) "
        "AND LOWER(hk.keyword) NOT LIKE LOWER(?) "
        "AND he.min_level <= ? "
        "ORDER BY hk.keyword")) {
      
      /* Create search patterns for contains and not-begins-with */
      char contains_pattern[MAX_INPUT_LENGTH + 4];
      snprintf(contains_pattern, sizeof(contains_pattern), "%%_%s%%", argument);
      snprintf(search_pattern, sizeof(search_pattern), "%s%%", argument);
      
      /* Bind parameters */
      if (mysql_stmt_bind_param_string(pstmt, 0, contains_pattern) &&
          mysql_stmt_bind_param_string(pstmt, 1, search_pattern) &&
          mysql_stmt_bind_param_int(pstmt, 2, GET_LEVEL(ch))) {
        
        /* Execute and fetch results */
        if (mysql_stmt_execute_prepared(pstmt)) {
          /* Fetch rows using prepared statement API */
          while (mysql_stmt_fetch_row(pstmt)) {
            char *tag = mysql_stmt_get_string(pstmt, 0);
            if (tag) {
              len2 += snprintf(buf2 + len2, sizeof(buf2) - len2, "%-20.20s%s",
                              tag, (++count2 % 3 ? "" : "\r\n"));
            }
          }
        }
      }
    }
    mysql_stmt_cleanup(pstmt);
  }

  /* Get total count of help entries accessible to this player */
  pstmt = mysql_stmt_create(conn);
  if (pstmt) {
    if (mysql_stmt_prepare_query(pstmt,
        "SELECT COUNT(DISTINCT he.tag) "
        "FROM help_entries he "
        "WHERE he.min_level <= ?")) {
      
      if (mysql_stmt_bind_param_int(pstmt, 0, GET_LEVEL(ch))) {
        
        /* Execute and fetch count */
        if (mysql_stmt_execute_prepared(pstmt)) {
          /* Fetch single row using prepared statement API */
          if (mysql_stmt_fetch_row(pstmt)) {
            total_entries = mysql_stmt_get_int(pstmt, 0);
          }
        }
      }
    }
    mysql_stmt_cleanup(pstmt);
  }

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

/* Helper function to get position name */
static const char *get_position_name(int position) {
  switch (position) {
    case POS_DEAD:      return "Dead";
    case POS_MORTALLYW: return "Mortally Wounded";
    case POS_INCAP:     return "Incapacitated";
    case POS_STUNNED:   return "Stunned";
    case POS_SLEEPING:  return "Sleeping";
    case POS_RECLINING: return "Reclining";
    case POS_RESTING:   return "Resting";
    case POS_SITTING:   return "Sitting";
    case POS_FIGHTING:  return "Fighting";
    case POS_STANDING:  return "Standing";
    default:            return "Any";
  }
}

/* Helper function to get level name */
static const char *get_level_name(int level) {
  static char buf[32];
  
  if (level <= 0)
    return "Everyone";
  else if (level == LVL_IMMORT)
    return "Immortal";
  else if (level == LVL_STAFF)
    return "Staff";
  else if (level == LVL_GRSTAFF)
    return "Greater Staff";
  else if (level == LVL_GRSTAFF)
    return "Greater Staff";
  else if (level == LVL_IMPL)
    return "Implementor";
  else if (level > 0 && level < LVL_IMMORT) {
    snprintf(buf, sizeof(buf), "Level %d", level);
    return buf;
  }
  else {
    return "Unknown";
  }
}

/* Forward declarations for command functions used in categorization */
ACMD_DECL(do_move);
ACMD_DECL(do_action);
ACMD_DECL(do_gen_cast);
ACMD_DECL(do_gen_comm);
ACMD_DECL(do_gen_door);
ACMD_DECL(do_gen_ps);
ACMD_DECL(do_gen_tog);
ACMD_DECL(do_write);
ACMD_DECL(do_activate);

/* Get command category based on function pointer and name patterns */
static const char *get_command_category(struct command_info *cmd) {
  /* Check by function pointer first */
  if (cmd->command_pointer == do_move) return "Movement";
  if (cmd->command_pointer == do_action) return "Social";
  if (cmd->command_pointer == do_gen_cast) return "Magic";
  if (cmd->command_pointer == do_gen_comm) return "Communication";
  if (cmd->command_pointer == do_gen_door) return "Interaction";
  if (cmd->command_pointer == do_gen_ps) return "Communication";
  if (cmd->command_pointer == do_gen_tog) return "Settings";
  if (cmd->command_pointer == do_write) return "Communication";
  if (cmd->command_pointer == do_activate) return "Magic";
  
  /* Check by command name patterns */
  if (strstr(cmd->command, "cast") || strstr(cmd->command, "spell") || 
      strstr(cmd->command, "memorize") || strstr(cmd->command, "forget") ||
      strstr(cmd->command, "activate")) return "Magic";
  if (strstr(cmd->command, "attack") || strstr(cmd->command, "kill") || 
      strstr(cmd->command, "hit") || strstr(cmd->command, "bash") ||
      strstr(cmd->command, "kick") || strstr(cmd->command, "trip")) return "Combat";
  if (strstr(cmd->command, "get") || strstr(cmd->command, "drop") || 
      strstr(cmd->command, "put") || strstr(cmd->command, "wear") ||
      strstr(cmd->command, "remove") || strstr(cmd->command, "wield")) return "Items";
  if (strstr(cmd->command, "say") || strstr(cmd->command, "tell") || 
      strstr(cmd->command, "whisper") || strstr(cmd->command, "shout")) return "Communication";
  if (strstr(cmd->command, "look") || strstr(cmd->command, "examine") || 
      strstr(cmd->command, "score") || strstr(cmd->command, "who") ||
      strstr(cmd->command, "where")) return "Information";
  if (strstr(cmd->command, "save") || strstr(cmd->command, "quit") || 
      strstr(cmd->command, "config") || strstr(cmd->command, "toggle")) return "System";
  
  /* Admin commands */
  if (cmd->minimum_level >= LVL_IMMORT) return "Administration";
  
  return "General";
}

/* Get action type description */
static const char *get_action_type_desc(int action_type) {
  /* Handle combined action flags */
  if (action_type & ACTION_STANDARD && action_type & ACTION_MOVE)
    return "Requires both standard and move actions";
  if (action_type & ACTION_STANDARD)
    return "Requires a standard action";
  if (action_type & ACTION_MOVE)
    return "Requires a move action";
  if (action_type & ACTION_SWIFT)
    return "Requires a swift action";
  if (action_type == ACTION_NONE)
    return "No action required";
  
  return "";
}

/* Generate contextual description based on command properties */
static void generate_command_description(struct command_info *cmd, char *desc, size_t desc_size) {
  const char *category = get_command_category(cmd);
  
  /* Special handling for specific commands */
  if (strcmp(cmd->command, "activate") == 0) {
    snprintf(desc, desc_size,
      "Activates a spell stored in a magic item you are wearing or holding.\r\n"
      "\r\n"
      "Magic items such as wands, staves, and some equipment can store spells "
      "that can be activated during combat. This uses the item's charges rather "
      "than your own spell slots.");
    return;
  }
  
  if (cmd->command_pointer == do_move) {
    snprintf(desc, desc_size,
      "Moves you %s from your current location.\r\n"
      "\r\n"
      "Movement commands allow you to explore the world. You must be at least "
      "reclining to move. Some exits may be hidden or require special conditions "
      "to use.", cmd->command);
    return;
  }
  
  /* Generate generic description based on category */
  if (strcmp(category, "Combat") == 0) {
    snprintf(desc, desc_size,
      "A combat command used during battle.\r\n"
      "\r\n"
      "This command can be used to attack enemies or perform combat maneuvers. "
      "Most combat commands require you to be in a fighting position.");
  } else if (strcmp(category, "Magic") == 0) {
    snprintf(desc, desc_size,
      "A magic-related command for spellcasting or magical abilities.\r\n"
      "\r\n"
      "This command interacts with the magic system, allowing you to cast spells, "
      "manage magical abilities, or use magical items.");
  } else if (strcmp(category, "Items") == 0) {
    snprintf(desc, desc_size,
      "Manages items in your inventory or equipment.\r\n"
      "\r\n"
      "Use this command to interact with objects, manage your inventory, "
      "or change your equipment.");
  } else if (strcmp(category, "Communication") == 0) {
    snprintf(desc, desc_size,
      "Allows communication with other players or NPCs.\r\n"
      "\r\n"
      "Communication commands let you interact with other characters in the game "
      "through various channels.");
  } else if (strcmp(category, "Information") == 0) {
    snprintf(desc, desc_size,
      "Provides information about the game world or your character.\r\n"
      "\r\n"
      "Use this command to examine your surroundings, check your status, "
      "or learn about game mechanics.");
  } else if (strcmp(category, "Administration") == 0) {
    snprintf(desc, desc_size,
      "An administrative command for game management.\r\n"
      "\r\n"
      "This command is restricted to staff members and is used for "
      "world building, player assistance, or game administration.");
  } else {
    snprintf(desc, desc_size,
      "A general game command.\r\n"
      "\r\n"
      "This command provides various game functionality. Use without arguments "
      "to see available options, or check related commands for more information.");
  }
}

/* Generate usage examples based on command type */
static void generate_usage_examples(struct command_info *cmd, char *examples, size_t examples_size) {
  examples[0] = '\0';
  
  if (strcmp(cmd->command, "activate") == 0) {
    snprintf(examples, examples_size,
      "\r\nExamples:\r\n"
      "  activate 'cure light'      - Activates cure light wounds from an item\r\n"
      "  activate 'fireball' troll  - Activates fireball targeting a troll\r\n"
      "  activate 'shield'          - Activates shield spell on yourself\r\n");
    return;
  }
  
  if (cmd->subcmd) {
    /* Has subcommands */
    snprintf(examples, examples_size,
      "\r\nThis command has multiple forms. Type '%s' alone to see options.\r\n",
      cmd->command);
  }
}

/* Generate help entry for a command */
static int generate_help_entry(struct char_data *ch, int cmd_index, bool force_overwrite) {
  struct command_info *cmd;
  char tag[MAX_INPUT_LENGTH];
  char keywords[MAX_INPUT_LENGTH];
  char help_text[MAX_STRING_LENGTH * 2];  /* Doubled to prevent truncation */
  char query[MAX_STRING_LENGTH];
  char *escaped_tag, *escaped_keywords, *escaped_help;
  MYSQL_RES *result;
  int exists = 0;
  
  cmd = &complete_cmd_info[cmd_index];
  
  /* Skip special cases */
  if (cmd->command_pointer == do_action) {
    /* This is a social, skip it */
    return 0;
  }
  
  /* Generate tag - use command name directly for better searchability */
  snprintf(tag, sizeof(tag), "%s", cmd->command);
  
  /* Check if entry already exists */
  escaped_tag = (char *)malloc(strlen(tag) * 2 + 1);
  if (!escaped_tag) {
    log("SYSERR: generate_help_entry: malloc failed for escaped_tag");
    return -1;
  }
  mysql_real_escape_string(conn, escaped_tag, tag, strlen(tag));
  
  snprintf(query, sizeof(query),
    "SELECT tag FROM help_entries WHERE tag = '%s'",
    escaped_tag);
  
  if (mysql_query(conn, query) == 0) {
    result = mysql_store_result(conn);
    if (result) {
      if (mysql_num_rows(result) > 0) {
        exists = 1;
      }
      mysql_free_result(result);
    }
  }
  
  /* Skip if exists and not forcing */
  if (exists && !force_overwrite) {
    free(escaped_tag);
    return 0;
  }
  
  /* Build keywords list - include multiple variations for better searchability */
  const char *category = get_command_category(cmd);
  snprintf(keywords, sizeof(keywords), "%s", cmd->command);
  
  /* Add the sort_as variant if different */
  if (cmd->sort_as && strcmp(cmd->sort_as, cmd->command) != 0) {
    strncat(keywords, " ", sizeof(keywords) - strlen(keywords) - 1);
    strncat(keywords, cmd->sort_as, sizeof(keywords) - strlen(keywords) - 1);
  }
  
  /* Add category as a keyword for grouped searches */
  strncat(keywords, " ", sizeof(keywords) - strlen(keywords) - 1);
  strncat(keywords, category, sizeof(keywords) - strlen(keywords) - 1);
  
  /* Add common variations and abbreviations */
  if (strcmp(cmd->command, "activate") == 0) {
    strncat(keywords, " magic item spell wand staff", sizeof(keywords) - strlen(keywords) - 1);
  } else if (strstr(cmd->command, "attack")) {
    strncat(keywords, " combat fight", sizeof(keywords) - strlen(keywords) - 1);
  } else if (cmd->command_pointer == do_move) {
    strncat(keywords, " go walk run travel direction", sizeof(keywords) - strlen(keywords) - 1);
  }
  
  /* Add "command" and "cmd" as generic keywords */
  strncat(keywords, " command cmd", sizeof(keywords) - strlen(keywords) - 1);
  
  /* Build help text with enhanced information */
  char description[MAX_STRING_LENGTH];
  char examples[MAX_STRING_LENGTH];
  char usage_str[MAX_INPUT_LENGTH];
  /* category already retrieved above for keywords */
  const char *action_desc = get_action_type_desc(cmd->actions_required);
  
  /* Generate contextual description */
  generate_command_description(cmd, description, sizeof(description));
  
  /* Generate usage examples */
  generate_usage_examples(cmd, examples, sizeof(examples));
  
  /* Build usage string */
  if (strcmp(cmd->command, "activate") == 0) {
    snprintf(usage_str, sizeof(usage_str), "activate '<spell name>' [target]");
  } else if (cmd->subcmd) {
    snprintf(usage_str, sizeof(usage_str), "%s <option> [arguments]", cmd->command);
  } else {
    snprintf(usage_str, sizeof(usage_str), "%s [arguments]", cmd->command);
  }
  
  /* Build complete help text */
  snprintf(help_text, sizeof(help_text),
    "%s\r\n"
    "\r\n"
    "Category: %s\r\n"
    "Usage: %s\r\n"
    "\r\n"
    "%s\r\n"
    "\r\n"
    "Requirements:\r\n"
    "  Minimum Level: %s\r\n"
    "  Position: %s\r\n"
    "%s%s%s"
    "%s"
    "%s%s%s"
    "\r\n"
    "---\r\n"
    "This help entry was automatically generated.\r\n"
    "For more detailed information, use 'help %s manual' or ask a staff member.\r\n",
    cmd->command,
    category,
    usage_str,
    description,
    get_level_name(cmd->minimum_level),
    get_position_name(cmd->minimum_position),
    action_desc[0] ? "  Action Type: " : "",
    action_desc,
    action_desc[0] ? "\r\n" : "",
    examples,
    cmd->subcmd ? "\r\nNote: This command has subcommands or options.\r\n" : "",
    cmd->ignore_wait ? "Note: This command can be used while performing other actions.\r\n" : "",
    cmd->actions_required == ACTION_SWIFT ? "Note: Swift actions can be used once per round.\r\n" : "",
    cmd->command
  );
  
  /* Prepare escaped strings */
  escaped_keywords = (char *)malloc(strlen(keywords) * 2 + 1);
  if (!escaped_keywords) {
    log("SYSERR: generate_help_entry: malloc failed for escaped_keywords");
    free(escaped_tag);
    return -1;
  }
  
  escaped_help = (char *)malloc(strlen(help_text) * 2 + 1);
  if (!escaped_help) {
    log("SYSERR: generate_help_entry: malloc failed for escaped_help");
    free(escaped_tag);
    free(escaped_keywords);
    return -1;
  }
  
  mysql_real_escape_string(conn, escaped_keywords, keywords, strlen(keywords));
  mysql_real_escape_string(conn, escaped_help, help_text, strlen(help_text));
  
  /* Insert or update help entry with auto_generated flag */
  if (exists && force_overwrite) {
    snprintf(query, sizeof(query),
      "UPDATE help_entries SET entry = '%s', min_level = %d, "
      "auto_generated = TRUE, last_updated = CURRENT_TIMESTAMP WHERE tag = '%s'",
      escaped_help, cmd->minimum_level, escaped_tag);
  } else {
    snprintf(query, sizeof(query),
      "INSERT INTO help_entries (tag, entry, min_level, auto_generated) "
      "VALUES ('%s', '%s', %d, TRUE)",
      escaped_tag, escaped_help, cmd->minimum_level);
  }
  
  if (mysql_query(conn, query) != 0) {
    if (ch) {
      send_to_char(ch, "Database error creating help for '%s': %s\r\n", 
                   cmd->command, mysql_error(conn));
    }
    free(escaped_tag);
    free(escaped_keywords);
    free(escaped_help);
    return -1;
  }
  
  /* Update keywords - delete old ones if updating, then add new ones */
  if (exists && force_overwrite) {
    /* Delete existing keywords for this help entry */
    snprintf(query, sizeof(query),
      "DELETE FROM help_keywords WHERE help_tag = '%s'",
      escaped_tag);
    mysql_query(conn, query);
  }
  
  /* Add keywords for new or updated entry */
  if (!exists || force_overwrite) {
    char *token, *rest, *keyword_copy;
    keyword_copy = strdup(keywords);
    if (!keyword_copy) {
      /* Log error but continue - keywords are optional */
      log("SYSERR: generate_help_entry: strdup failed for keywords");
    } else {
      token = strtok_r(keyword_copy, " ", &rest);
      
      while (token) {
        /* Skip very short keywords unless they're the command itself */
        if (strlen(token) < 2 && strcmp(token, cmd->command) != 0) {
          token = strtok_r(NULL, " ", &rest);
          continue;
        }
        
        char *escaped_keyword = (char *)malloc(strlen(token) * 2 + 1);
        if (!escaped_keyword) {
          log("SYSERR: generate_help_entry: malloc failed for escaped_keyword");
          /* Continue with other keywords */
          token = strtok_r(NULL, " ", &rest);
          continue;
        }
        mysql_real_escape_string(conn, escaped_keyword, token, strlen(token));
        
        snprintf(query, sizeof(query),
          "INSERT IGNORE INTO help_keywords (help_tag, keyword) "
          "VALUES ('%s', '%s')",
          escaped_tag, escaped_keyword);
        
        mysql_query(conn, query);
        free(escaped_keyword);
        token = strtok_r(NULL, " ", &rest);
      }
      free(keyword_copy);
    }
  }
  
  free(escaped_tag);
  free(escaped_keywords);
  free(escaped_help);
  
  if (ch) {
    send_to_char(ch, "Generated help for '%s'\r\n", cmd->command);
  }
  
  return 1;
}

/* Auto-generate help files for commands */
ACMD(do_helpgen) {
  char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
  int generated = 0, skipped = 0, errors = 0;
  bool force = FALSE;
  int i, cmd_num;
  
  two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));
  
  if (!*arg1) {
    send_to_char(ch, "Usage: helpgen <missing|all|list|clean|repair|import|export|command> [options]\r\n"
                     "  missing - Generate help for commands without entries\r\n"
                     "  all     - Generate help for all commands\r\n"
                     "  list    - List all auto-generated help entries\r\n"
                     "  clean   - Delete all auto-generated help entries\r\n"
                     "  repair  - Fix orphaned keywords and entries without keywords\r\n"
                     "  import  - Import legacy help.hlp file into database (requires mode)\r\n"
                     "          Modes: preview (show what would be imported)\r\n"
                     "                 skip (only import new entries, skip existing)\r\n"
                     "                 merge (intelligently merge duplicates)\r\n"
                     "                 force (overwrite existing entries)\r\n"
                     "  export  - Export database to help.hlp file (requires mode)\r\n"
                     "          Modes: preview (dry run without writing)\r\n"
                     "                 backup (safe export with backup)\r\n"
                     "                 force (overwrite without backup)\r\n"
                     "          Filters: noauto (exclude auto-generated)\r\n"
                     "                   level <num> (only entries <= level)\r\n"
                     "  command - Generate help for specific command\r\n"
                     "  force   - Overwrite existing entries\r\n");
    return;
  }
  
  /* Check for force flag */
  if (*arg2 && !str_cmp(arg2, "force")) {
    force = TRUE;
  }
  
  if (!str_cmp(arg1, "missing")) {
    /* Generate only for missing entries */
    send_to_char(ch, "Generating help for missing commands...\r\n");
    
    for (i = 1; *(complete_cmd_info[i].command) != '\n'; i++) {
      if (complete_cmd_info[i].command_pointer != do_action && 
          complete_cmd_info[i].minimum_level >= 0) {
        
        /* Check if help exists */
        if (search_help(complete_cmd_info[i].command, LVL_IMPL) == NULL) {
          int result = generate_help_entry(ch, i, force);
          if (result > 0)
            generated++;
          else if (result < 0)
            errors++;
        } else {
          skipped++;
        }
      }
    }
    
  } else if (!str_cmp(arg1, "list")) {
    /* List all auto-generated entries */
    char query[MAX_STRING_LENGTH];
    MYSQL_RES *result;
    MYSQL_ROW row;
    int count = 0;
    
    snprintf(query, sizeof(query),
      "SELECT tag, min_level FROM help_entries WHERE auto_generated = TRUE ORDER BY tag");
    
    if (mysql_query(conn, query) != 0) {
      send_to_char(ch, "Database error: %s\r\n", mysql_error(conn));
      return;
    }
    
    result = mysql_store_result(conn);
    if (!result) {
      send_to_char(ch, "Error storing result: %s\r\n", mysql_error(conn));
      return;
    }
    
    send_to_char(ch, "Auto-generated help entries:\r\n");
    send_to_char(ch, "----------------------------\r\n");
    
    while ((row = mysql_fetch_row(result))) {
      send_to_char(ch, "  %-30s (Level: %s)\r\n", row[0], row[1]);
      count++;
    }
    
    mysql_free_result(result);
    send_to_char(ch, "\r\nTotal: %d auto-generated entries\r\n", count);
    
  } else if (!str_cmp(arg1, "clean")) {
    /* Delete all auto-generated entries */
    char query[MAX_STRING_LENGTH];
    
    if (!force) {
      send_to_char(ch, "WARNING: This will delete ALL auto-generated help entries!\r\n");
      send_to_char(ch, "Use 'helpgen clean force' to confirm deletion.\r\n");
      return;
    }
    
    /* Delete keywords first */
    snprintf(query, sizeof(query),
      "DELETE hk FROM help_keywords hk "
      "INNER JOIN help_entries he ON hk.help_tag = he.tag "
      "WHERE he.auto_generated = TRUE");
    
    if (mysql_query(conn, query) != 0) {
      send_to_char(ch, "Error deleting keywords: %s\r\n", mysql_error(conn));
      return;
    }
    
    int keywords_deleted = mysql_affected_rows(conn);
    
    /* Delete help entries */
    snprintf(query, sizeof(query),
      "DELETE FROM help_entries WHERE auto_generated = TRUE");
    
    if (mysql_query(conn, query) != 0) {
      send_to_char(ch, "Error deleting entries: %s\r\n", mysql_error(conn));
      return;
    }
    
    int entries_deleted = mysql_affected_rows(conn);
    
    send_to_char(ch, "Deleted %d auto-generated help entries and %d keywords.\r\n",
                 entries_deleted, keywords_deleted);
    
  } else if (!str_cmp(arg1, "all")) {
    /* Generate for all commands */
    send_to_char(ch, "Generating help for all commands%s...\r\n",
                 force ? " (forcing overwrite)" : "");
    
    for (i = 1; *(complete_cmd_info[i].command) != '\n'; i++) {
      if (complete_cmd_info[i].command_pointer != do_action && 
          complete_cmd_info[i].minimum_level >= 0) {
        
        int result = generate_help_entry(ch, i, force);
        if (result > 0)
          generated++;
        else if (result == 0)
          skipped++;
        else
          errors++;
      }
    }
    
  } else if (!str_cmp(arg1, "repair")) {
    /* Repair database - fix orphaned keywords and entries without keywords */
    char query[MAX_STRING_LENGTH * 2];
    MYSQL_RES *result;
    MYSQL_ROW row;
    int orphaned_keywords = 0, missing_keywords = 0, fixed = 0;
    
    send_to_char(ch, "Starting help database repair...\r\n\r\n");
    
    /* Step 1: Find and remove orphaned keywords (keywords pointing to non-existent help entries) */
    send_to_char(ch, "Step 1: Checking for orphaned keywords...\r\n");
    
    snprintf(query, sizeof(query),
      "SELECT hk.keyword, hk.help_tag "
      "FROM help_keywords hk "
      "LEFT JOIN help_entries he ON hk.help_tag = he.tag "
      "WHERE he.tag IS NULL");
    
    if (mysql_query(conn, query) == 0) {
      result = mysql_store_result(conn);
      if (result) {
        while ((row = mysql_fetch_row(result))) {
          send_to_char(ch, "  Found orphaned keyword '%s' -> '%s'\r\n", 
                       row[0] ? row[0] : "(null)", 
                       row[1] ? row[1] : "(null)");
          orphaned_keywords++;
        }
        mysql_free_result(result);
      }
    }
    
    if (orphaned_keywords > 0) {
      if (!force) {
        send_to_char(ch, "\r\nFound %d orphaned keywords. Use 'helpgen repair force' to delete them.\r\n", 
                     orphaned_keywords);
      } else {
        /* Delete orphaned keywords */
        snprintf(query, sizeof(query),
          "DELETE hk FROM help_keywords hk "
          "LEFT JOIN help_entries he ON hk.help_tag = he.tag "
          "WHERE he.tag IS NULL");
        
        if (mysql_query(conn, query) == 0) {
          int deleted = mysql_affected_rows(conn);
          send_to_char(ch, "  Deleted %d orphaned keywords.\r\n", deleted);
          fixed += deleted;
        } else {
          send_to_char(ch, "  Error deleting orphaned keywords: %s\r\n", mysql_error(conn));
        }
      }
    } else {
      send_to_char(ch, "  No orphaned keywords found.\r\n");
    }
    
    /* Step 2: Find help entries without any keywords */
    send_to_char(ch, "\r\nStep 2: Checking for help entries without keywords...\r\n");
    
    snprintf(query, sizeof(query),
      "SELECT he.tag, he.min_level "
      "FROM help_entries he "
      "LEFT JOIN help_keywords hk ON he.tag = hk.help_tag "
      "WHERE hk.help_tag IS NULL");
    
    if (mysql_query(conn, query) == 0) {
      result = mysql_store_result(conn);
      if (result) {
        while ((row = mysql_fetch_row(result))) {
          send_to_char(ch, "  Found entry without keywords: '%s' (level %s)\r\n", 
                       row[0] ? row[0] : "(null)",
                       row[1] ? row[1] : "0");
          missing_keywords++;
          
          if (force && row[0]) {
            /* Add the tag itself as a keyword */
            char *escaped_tag = (char *)malloc(strlen(row[0]) * 2 + 1);
            if (escaped_tag) {
              mysql_real_escape_string(conn, escaped_tag, row[0], strlen(row[0]));
              
              snprintf(query, sizeof(query),
                "INSERT INTO help_keywords (help_tag, keyword) VALUES ('%s', '%s')",
                escaped_tag, escaped_tag);
              
              if (mysql_query(conn, query) == 0) {
                send_to_char(ch, "    Added keyword '%s' for entry '%s'\r\n", row[0], row[0]);
                fixed++;
              } else {
                send_to_char(ch, "    Error adding keyword: %s\r\n", mysql_error(conn));
              }
              
              free(escaped_tag);
            }
          }
        }
        mysql_free_result(result);
      }
    }
    
    if (missing_keywords > 0 && !force) {
      send_to_char(ch, "\r\nFound %d entries without keywords. Use 'helpgen repair force' to add keywords.\r\n", 
                   missing_keywords);
    }
    
    /* Step 3: Summary */
    send_to_char(ch, "\r\n--- Repair Summary ---\r\n");
    send_to_char(ch, "Orphaned keywords found: %d\r\n", orphaned_keywords);
    send_to_char(ch, "Entries without keywords found: %d\r\n", missing_keywords);
    
    if (force) {
      send_to_char(ch, "Issues fixed: %d\r\n", fixed);
      send_to_char(ch, "\r\nDatabase repair complete.\r\n");
    } else {
      send_to_char(ch, "\r\nUse 'helpgen repair force' to fix these issues.\r\n");
    }
    
    return;
    
  } else if (!str_cmp(arg1, "import")) {
    /* Import legacy help.hlp file */
    const char *mode = NULL;
    
    if (!*arg2) {
      send_to_char(ch, "WARNING: Import requires an explicit mode to prevent accidental data changes.\r\n"
                       "\r\nUsage: helpgen import <mode>\r\n"
                       "Available modes:\r\n"
                       "  preview - Dry run, shows what would be imported without making changes\r\n"
                       "  skip    - Only import new entries, skip any that already exist\r\n"
                       "  merge   - Creates new entries with suffixes (_2, _3) for duplicates\r\n"
                       "  force   - Overwrites existing entries that share keywords\r\n"
                       "\r\nExample: helpgen import preview\r\n");
      return;
    }
    
    if (!str_cmp(arg2, "preview") || !str_cmp(arg2, "force") || !str_cmp(arg2, "merge") || !str_cmp(arg2, "skip")) {
      mode = arg2;
    } else {
      send_to_char(ch, "Invalid import mode '%s'.\r\n"
                       "Valid modes are: preview, skip, merge, or force\r\n", arg2);
      return;
    }
    
    send_to_char(ch, "Starting help.hlp import (%s mode)...\r\n\r\n", mode);
    int result = import_help_hlp_file(ch, mode);
    
    if (result < 0) {
      send_to_char(ch, "Import failed. Check logs for details.\r\n");
    }
    return;
    
  } else if (!str_cmp(arg1, "export")) {
    /* Export database to help.hlp file */
    const char *mode = NULL;
    
    if (!*arg2) {
      send_to_char(ch, "WARNING: Export requires an explicit mode to prevent accidental file overwrites.\r\n"
                       "\r\nUsage: helpgen export <mode> [options]\r\n"
                       "Required modes:\r\n"
                       "  preview - Dry run, shows what would be exported without writing files\r\n"
                       "  backup  - Creates timestamped backup before exporting\r\n"
                       "  force   - Overwrites help.hlp without creating backup\r\n"
                       "\r\nOptional filters (add after mode):\r\n"
                       "  noauto      - Exclude auto-generated entries\r\n"
                       "  level <num> - Only export entries accessible at level or below\r\n"
                       "\r\nExamples:\r\n"
                       "  helpgen export preview        - See what would be exported\r\n"
                       "  helpgen export backup         - Safe export with backup\r\n"
                       "  helpgen export force noauto   - Overwrite, excluding auto-generated\r\n");
      return;
    }
    
    /* Check for required mode as first argument */
    if (!str_cmp(arg2, "preview") || !str_cmp(arg2, "backup") || !str_cmp(arg2, "force")) {
      mode = arg2;
    } else {
      send_to_char(ch, "Invalid export mode '%s'.\r\n"
                       "Valid modes are: preview, backup, or force\r\n"
                       "Use 'helpgen export' for full usage information.\r\n", arg2);
      return;
    }
    
    send_to_char(ch, "Starting help database export (%s mode)...\r\n\r\n", mode);
    int result = export_help_to_hlp(ch, arg2);
    
    if (result < 0) {
      send_to_char(ch, "Export failed. Check logs for details.\r\n");
    } else {
      send_to_char(ch, "Successfully exported %d entries to help.hlp\r\n", result);
    }
    return;
    
  } else {
    /* Generate for specific command */
    cmd_num = find_command(arg1);
    
    if (cmd_num == -1) {
      send_to_char(ch, "Unknown command: %s\r\n", arg1);
      return;
    }
    
    if (complete_cmd_info[cmd_num].command_pointer == do_action) {
      send_to_char(ch, "'%s' is a social command, not generating help.\r\n", arg1);
      return;
    }
    
    int result = generate_help_entry(ch, cmd_num, force);
    if (result > 0) {
      send_to_char(ch, "Help entry generated successfully.\r\n");
    } else if (result == 0) {
      send_to_char(ch, "Help entry already exists. Use 'force' to overwrite.\r\n");
    } else {
      send_to_char(ch, "Error generating help entry.\r\n");
    }
    return;
  }
  
  send_to_char(ch, "\r\nHelp generation complete:\r\n"
                   "  Generated: %d\r\n"
                   "  Skipped: %d\r\n"
                   "  Errors: %d\r\n",
                   generated, skipped, errors);
  
  if (generated > 0) {
    send_to_char(ch, "\r\nUse 'helpcheck' to see remaining commands without help.\r\n");
  }
}

/* Export functionality implementations */

/* Helper function to count occurrences of a character in a string */
static int count_char_occurrences(const char *str, char ch) {
  int count = 0;
  if (!str) return 0;
  while (*str) {
    if (*str == ch) count++;
    str++;
  }
  return count;
}

/**
 * Write a single help entry to the file in help.hlp format
 * Format:
 * KEYWORD1 KEYWORD2 KEYWORD3
 * 
 * Help text content
 * Multiple lines...
 * #<min_level>
 */
static int write_help_entry_to_file(FILE *fp, const char *keywords, const char *content, int min_level) {
  char keyword_buf[MAX_STRING_LENGTH];
  char *token;
  int first = 1;
  
  if (!fp || !keywords || !content) {
    return -1;
  }
  
  /* Convert keywords to uppercase and write them space-separated */
  strlcpy(keyword_buf, keywords, sizeof(keyword_buf));
  
  /* Write keywords in uppercase - using strtok instead of strsep for C90 */
  token = strtok(keyword_buf, ",");
  while (token != NULL) {
    char *p;
    /* Skip leading spaces */
    while (*token && isspace(*token)) token++;
    if (*token) {
      /* Remove trailing spaces */
      p = token + strlen(token) - 1;
      while (p > token && isspace(*p)) {
        *p = '\0';
        p--;
      }
      
      /* Convert to uppercase */
      for (p = token; *p; p++) {
        *p = UPPER(*p);
      }
      
      /* Write with space separator */
      if (!first) {
        fprintf(fp, " ");
      }
      fprintf(fp, "%s", token);
      first = 0;
    }
    token = strtok(NULL, ",");
  }
  fprintf(fp, "\n\n");  /* Blank line after keywords */
  
  /* Write the content */
  fprintf(fp, "%s", content);
  
  /* Make sure content ends with newline before level marker */
  if (content[strlen(content) - 1] != '\n') {
    fprintf(fp, "\n");
  }
  
  /* Write the level marker */
  fprintf(fp, "#%d\n", min_level);
  
  return 0;
}

/**
 * Export all help entries from database to help.hlp file
 * Required mode: preview, backup, or force
 * Optional filters: noauto, level <num>
 */
static int export_help_to_hlp(struct char_data *ch, const char *options) {
  FILE *fp = NULL;
  char filepath[MAX_STRING_LENGTH];
  char backup_path[MAX_STRING_LENGTH];
  char query[MAX_STRING_LENGTH * 2];
  char keyword_query[MAX_STRING_LENGTH];
  MYSQL_RES *entries_result = NULL, *keywords_result = NULL;
  MYSQL_ROW entry_row, keyword_row;
  int exported = 0, errors = 0;
  int preview_mode = 0, backup_mode = 0, exclude_auto = 0, force_mode = 0;
  int max_level = LVL_IMPL;
  char opt_arg[MAX_INPUT_LENGTH];
  char *token;
  time_t now;
  struct tm *tm_info;
  
  /* Parse options - first token is the required mode, rest are optional filters */
  if (options && *options) {
    strlcpy(opt_arg, options, sizeof(opt_arg));
    
    token = strtok(opt_arg, " ");
    
    /* First token must be the mode */
    if (token) {
      if (!str_cmp(token, "preview")) {
        preview_mode = 1;
      } else if (!str_cmp(token, "backup")) {
        backup_mode = 1;
      } else if (!str_cmp(token, "force")) {
        force_mode = 1;
      }
      
      /* Parse additional optional filters */
      token = strtok(NULL, " ");
      while (token != NULL) {
        if (!str_cmp(token, "noauto")) {
          exclude_auto = 1;
        } else if (!str_cmp(token, "level")) {
        /* Get the level number */
        token = strtok(NULL, " ");
        if (token && *token) {
          max_level = atoi(token);
          if (max_level < 0) max_level = 0;
          if (max_level > LVL_IMPL) max_level = LVL_IMPL;
        }
          /* Continue to next token after processing level value */
          token = strtok(NULL, " ");
          continue;
        }
        token = strtok(NULL, " ");
      }
    }
  }
  
  /* Set file path */
  snprintf(filepath, sizeof(filepath), "text/help/help.hlp");
  
  /* Report mode to user */
  if (preview_mode) {
    send_to_char(ch, "PREVIEW MODE - No files will be written\r\n");
  } else if (backup_mode) {
    send_to_char(ch, "BACKUP MODE - Will create backup before exporting\r\n");
  } else if (force_mode) {
    send_to_char(ch, "FORCE MODE - Will overwrite without backup\r\n");
  }
  
  if (exclude_auto) {
    send_to_char(ch, "Excluding auto-generated entries\r\n");
  }
  if (max_level < LVL_IMPL) {
    send_to_char(ch, "Exporting entries with min_level <= %d\r\n", max_level);
  }
  send_to_char(ch, "\r\n");
  
  /* Create backup if in backup mode */
  if (backup_mode && !preview_mode) {
    /* Create timestamped backup */
    time(&now);
    tm_info = localtime(&now);
    snprintf(backup_path, sizeof(backup_path), 
             "text/help/help.hlp.%04d%02d%02d_%02d%02d%02d",
             tm_info->tm_year + 1900, tm_info->tm_mon + 1, tm_info->tm_mday,
             tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec);
    
    /* Copy existing file to backup */
    FILE *src = fopen(filepath, "r");
    if (src) {
      FILE *dst = fopen(backup_path, "w");
      if (dst) {
        char buffer[4096];
        size_t bytes;
        while ((bytes = fread(buffer, 1, sizeof(buffer), src)) > 0) {
          fwrite(buffer, 1, bytes, dst);
        }
        fclose(dst);
        send_to_char(ch, "Created backup: %s\r\n", backup_path);
      } else {
        send_to_char(ch, "Warning: Could not create backup file\r\n");
      }
      fclose(src);
    }
  }
  
  /* Build query for entries */
  snprintf(query, sizeof(query),
    "SELECT tag, entry, min_level "
    "FROM help_entries "
    "WHERE min_level <= %d %s "
    "ORDER BY min_level ASC, tag ASC",
    max_level,
    exclude_auto ? "AND auto_generated = FALSE" : "");
  
  /* Execute query */
  if (mysql_query(conn, query) != 0) {
    send_to_char(ch, "Database error fetching entries: %s\r\n", mysql_error(conn));
    mudlog(NRM, LVL_IMPL, TRUE, "SYSERR: export_help_to_hlp: Query failed: %s", mysql_error(conn));
    return -1;
  }
  
  entries_result = mysql_store_result(conn);
  if (!entries_result) {
    send_to_char(ch, "Error storing entries result: %s\r\n", mysql_error(conn));
    return -1;
  }
  
  /* Open output file if not in preview mode */
  if (!preview_mode) {
    fp = fopen(filepath, "w");
    if (!fp) {
      send_to_char(ch, "Error: Cannot open %s for writing\r\n", filepath);
      mysql_free_result(entries_result);
      return -1;
    }
  }
  
  /* Process each entry */
  while ((entry_row = mysql_fetch_row(entries_result))) {
    const char *tag = entry_row[0];
    const char *content = entry_row[1];
    const char *level_str = entry_row[2];
    int min_level = level_str ? atoi(level_str) : 0;
    char keywords_combined[MAX_STRING_LENGTH * 2];
    int first_keyword = 1;
    
    if (!tag || !content) continue;
    
    /* Fetch keywords for this entry - properly escaped */
    char escaped_tag[MAX_HELP_TAG_LENGTH * 2 + 1];  /* Worst case: every char needs escaping */
    mysql_real_escape_string(conn, escaped_tag, tag, strlen(tag));
    snprintf(keyword_query, sizeof(keyword_query),
      "SELECT keyword FROM help_keywords WHERE help_tag = '%s' ORDER BY keyword ASC",
      escaped_tag);
    
    if (mysql_query(conn, keyword_query) != 0) {
      send_to_char(ch, "Warning: Failed to fetch keywords for '%s': %s\r\n", 
                   tag, mysql_error(conn));
      errors++;
      continue;
    }
    
    keywords_result = mysql_store_result(conn);
    if (!keywords_result) {
      errors++;
      continue;
    }
    
    /* Build comma-separated keyword list */
    keywords_combined[0] = '\0';
    while ((keyword_row = mysql_fetch_row(keywords_result))) {
      if (keyword_row[0]) {
        if (!first_keyword) {
          strcat(keywords_combined, ",");
        }
        strcat(keywords_combined, keyword_row[0]);
        first_keyword = 0;
      }
    }
    mysql_free_result(keywords_result);
    
    /* If no keywords found, use the tag as keyword */
    if (keywords_combined[0] == '\0') {
      strlcpy(keywords_combined, tag, sizeof(keywords_combined));
    }
    
    /* Progress reporting */
    if (exported % 100 == 0 && exported > 0) {
      send_to_char(ch, "Processing entry %d...\r\n", exported);
    }
    
    /* Write or preview the entry */
    if (preview_mode) {
      if (exported < 20) {  /* Show first 20 in preview */
        send_to_char(ch, "  [%3d] %-30s (Level %d, %d keywords)\r\n", 
                     exported + 1, tag, min_level,
                     count_char_occurrences(keywords_combined, ',') + 1);
      }
    } else {
      /* Write to file */
      if (write_help_entry_to_file(fp, keywords_combined, content, min_level) < 0) {
        send_to_char(ch, "Error writing entry '%s'\r\n", tag);
        errors++;
      }
    }
    
    exported++;
  }
  
  mysql_free_result(entries_result);
  
  /* Write the file terminator before closing */
  if (fp) {
    fprintf(fp, "$~\n");
    fclose(fp);
  }
  
  /* Report results */
  send_to_char(ch, "\r\n=== Export Summary ===\r\n");
  if (preview_mode) {
    send_to_char(ch, "Would export: %d entries\r\n", exported);
    if (exported > 20) {
      send_to_char(ch, "(Showing first 20 entries only)\r\n");
    }
  } else {
    send_to_char(ch, "Exported: %d entries\r\n", exported);
    send_to_char(ch, "Errors: %d\r\n", errors);
    send_to_char(ch, "Output file: %s\r\n", filepath);
    
    /* Get file size */
    struct stat st;
    if (stat(filepath, &st) == 0) {
      send_to_char(ch, "File size: %ld bytes\r\n", (long)st.st_size);
    }
  }
  
  if (errors > 0) {
    send_to_char(ch, "\r\nExport completed with errors. Check logs for details.\r\n");
  }
  
  return exported;
}

/* Import functionality implementations */

/**
 * Free a help_entry_list structure and its allocated memory
 */
static void free_help_entry(struct help_entry_list *entry) {
  if (!entry) return;
  
  if (entry->tag) free(entry->tag);
  if (entry->keywords) free(entry->keywords);
  if (entry->entry) free(entry->entry);
  free(entry);
}

/**
 * Parse a single help entry from the help.hlp file
 * Format:
 * KEYWORD1 KEYWORD2 KEYWORD3
 * Help text content
 * Multiple lines...
 * #<min_level>
 */
static struct help_entry_list *parse_help_entry(FILE *fp, int *min_level) {
  char line[MAX_STRING_LENGTH];
  char keywords[MAX_STRING_LENGTH];
  char content[MAX_STRING_LENGTH * 8];
  struct help_entry_list *entry = NULL;
  int content_len = 0;
  
  *min_level = 0;
  keywords[0] = '\0';
  content[0] = '\0';
  
  /* Read the keywords line */
  if (!fgets(line, sizeof(line), fp)) {
    return NULL;
  }
  
  /* Skip empty lines */
  while (line[0] == '\n' || line[0] == '\r') {
    if (!fgets(line, sizeof(line), fp)) {
      return NULL;
    }
  }
  
  /* Check if this is a level marker (start of next entry) */
  if (line[0] == '#') {
    return NULL;
  }
  
  /* Copy keywords, removing newline */
  strlcpy(keywords, line, sizeof(keywords));
  char *newline = strchr(keywords, '\n');
  if (newline) *newline = '\0';
  newline = strchr(keywords, '\r');
  if (newline) *newline = '\0';
  
  /* Skip the blank line after keywords */
  if (!fgets(line, sizeof(line), fp)) {
    return NULL;
  }
  
  /* Read content until we hit #<level> */
  while (fgets(line, sizeof(line), fp)) {
    if (line[0] == '#') {
      /* Parse the level */
      *min_level = atoi(line + 1);
      break;
    }
    
    /* Add line to content */
    int line_len = strlen(line);
    if (content_len + line_len < sizeof(content) - 1) {
      strcat(content, line);
      content_len += line_len;
    }
  }
  
  /* Create the help entry structure */
  CREATE(entry, struct help_entry_list, 1);
  entry->keywords = strdup(keywords);
  entry->entry = strdup(content);
  entry->min_level = *min_level;
  entry->next = NULL;
  
  /* Generate tag from first keyword */
  char *first_keyword = strdup(keywords);
  char *space = strchr(first_keyword, ' ');
  char *p;
  if (space) *space = '\0';
  
  /* Convert to lowercase for tag */
  for (p = first_keyword; *p; p++) {
    *p = LOWER(*p);
  }
  entry->tag = first_keyword;
  
  return entry;
}

/**
 * Import a single entry with conflict resolution
 * Modes: "preview" = don't save, "force" = overwrite, "merge" = intelligent merge
 */
static int import_entry_with_resolution(struct char_data *ch, struct help_entry_list *entry, 
                                       int min_level, const char *mode, char *msg_buf, size_t msg_size) {
  char *query = NULL;
  char escaped_tag[MAX_STRING_LENGTH];
  char *escaped_entry = NULL;
  char escaped_keyword[MAX_STRING_LENGTH];
  MYSQL_RES *result;
  MYSQL_ROW row;
  int tag_exists = 0;
  int keyword_exists = 0;
  char *existing_help_tag = NULL;
  size_t query_size, entry_len;
  
  if (!entry || !entry->tag) return -1;
  
  /* Escape tag for SQL */
  mysql_real_escape_string(conn, escaped_tag, entry->tag, strlen(entry->tag));
  
  /* Check if tag already exists */
  query_size = strlen("SELECT tag FROM help_entries WHERE tag = ''") + strlen(escaped_tag) + 1;
  CREATE(query, char, query_size);
  snprintf(query, query_size,
    "SELECT tag FROM help_entries WHERE tag = '%s'", escaped_tag);
  
  if (mysql_query(conn, query) == 0) {
    result = mysql_store_result(conn);
    if (result) {
      tag_exists = (mysql_num_rows(result) > 0);
      mysql_free_result(result);
    }
  }
  free(query);
  
  /* For skip mode and preview, also check if any keywords already exist in the database */
  if ((!str_cmp(mode, "skip") || !str_cmp(mode, "preview")) && !tag_exists) {
    char keyword_copy[MAX_STRING_LENGTH];
    char *token, *rest;
    char *escaped_keywords = NULL;
    size_t escaped_size = 0;
    size_t escaped_len = 0;
    int first_keyword = 1;
    
    /* Build a list of escaped keywords for the IN clause */
    strlcpy(keyword_copy, entry->keywords, sizeof(keyword_copy));
    token = strtok_r(keyword_copy, " ", &rest);
    
    /* Allocate initial buffer for escaped keywords */
    escaped_size = MAX_STRING_LENGTH * 4;
    CREATE(escaped_keywords, char, escaped_size);
    escaped_keywords[0] = '\0';
    
    while (token) {
      mysql_real_escape_string(conn, escaped_keyword, token, strlen(token));
      
      /* Add comma if not first keyword */
      if (!first_keyword) {
        /* Check if we need to expand buffer */
        if (escaped_len + 3 > escaped_size - 1) {
          escaped_size *= 2;
          RECREATE(escaped_keywords, char, escaped_size);
        }
        escaped_len += snprintf(escaped_keywords + escaped_len, escaped_size - escaped_len, ", ");
      }
      first_keyword = 0;
      
      /* Add the escaped keyword in quotes */
      size_t keyword_len = strlen(escaped_keyword) + 3; /* For quotes and null */
      if (escaped_len + keyword_len > escaped_size - 1) {
        escaped_size *= 2;
        RECREATE(escaped_keywords, char, escaped_size);
      }
      escaped_len += snprintf(escaped_keywords + escaped_len, escaped_size - escaped_len, "'%s'", escaped_keyword);
      
      token = strtok_r(NULL, " ", &rest);
    }
    
    /* Check if any of these keywords already exist */
    if (escaped_len > 0) {
      query_size = strlen("SELECT DISTINCT help_tag FROM help_keywords WHERE keyword IN ()") + escaped_len + 1;
      CREATE(query, char, query_size);
      snprintf(query, query_size,
        "SELECT DISTINCT help_tag FROM help_keywords WHERE keyword IN (%s)", escaped_keywords);
      
      if (mysql_query(conn, query) == 0) {
        result = mysql_store_result(conn);
        if (result) {
          if (mysql_num_rows(result) > 0) {
            keyword_exists = 1;
            /* Get the first existing help tag for reporting */
            row = mysql_fetch_row(result);
            if (row && row[0]) {
              existing_help_tag = strdup(row[0]);
            }
          }
          mysql_free_result(result);
        }
      }
      free(query);
    }
    free(escaped_keywords);
  }
  
  /* Preview mode - just report what would happen */
  if (!str_cmp(mode, "preview")) {
    if (tag_exists) {
      snprintf(msg_buf, msg_size, "  [TAG EXISTS] %s - would skip/merge/force depending on mode\r\n", entry->tag);
      if (existing_help_tag) free(existing_help_tag);
      return 0;
    } else if (keyword_exists) {
      snprintf(msg_buf, msg_size, "  [KEYWORD EXISTS] %s - keywords already mapped to %s (would skip in skip mode)\r\n", 
               entry->tag, existing_help_tag ? existing_help_tag : "another entry");
      if (existing_help_tag) free(existing_help_tag);
      return 0;
    } else {
      snprintf(msg_buf, msg_size, "  [NEW] %s - would import\r\n", entry->tag);
      if (existing_help_tag) free(existing_help_tag);
      return 1;
    }
  }
  
  /* Handle existing entries based on mode */
  if (tag_exists) {
    if (!str_cmp(mode, "force")) {
      /* Delete existing entry and its keywords */
      query_size = strlen("DELETE FROM help_keywords WHERE help_tag = ''") + strlen(escaped_tag) + 1;
      CREATE(query, char, query_size);
      snprintf(query, query_size,
        "DELETE FROM help_keywords WHERE help_tag = '%s'", escaped_tag);
      mysql_query(conn, query);
      free(query);
      
      query_size = strlen("DELETE FROM help_entries WHERE tag = ''") + strlen(escaped_tag) + 1;
      CREATE(query, char, query_size);
      snprintf(query, query_size,
        "DELETE FROM help_entries WHERE tag = '%s'", escaped_tag);
      mysql_query(conn, query);
      free(query);
      
      snprintf(msg_buf, msg_size, "  [REPLACED] %s\r\n", entry->tag);
    } else if (!str_cmp(mode, "merge")) {
      /* Create a new tag with suffix */
      char new_tag[MAX_STRING_LENGTH];
      int suffix = 2;
      
      do {
        snprintf(new_tag, sizeof(new_tag), "%s_%d", entry->tag, suffix);
        mysql_real_escape_string(conn, escaped_tag, new_tag, strlen(new_tag));
        
        query_size = strlen("SELECT tag FROM help_entries WHERE tag = ''") + strlen(escaped_tag) + 1;
        CREATE(query, char, query_size);
        snprintf(query, query_size,
          "SELECT tag FROM help_entries WHERE tag = '%s'", escaped_tag);
        
        tag_exists = 0;
        if (mysql_query(conn, query) == 0) {
          result = mysql_store_result(conn);
          if (result) {
            tag_exists = (mysql_num_rows(result) > 0);
            mysql_free_result(result);
          }
        }
        free(query);
        suffix++;
      } while (tag_exists && suffix < 100);
      
      free(entry->tag);
      entry->tag = strdup(new_tag);
      snprintf(msg_buf, msg_size, "  [MERGED] %s (as %s)\r\n", entry->keywords, new_tag);
    } else if (!str_cmp(mode, "skip")) {
      /* Skip mode - explicitly skip existing entries */
      snprintf(msg_buf, msg_size, "  [SKIPPED] %s - tag already exists\r\n", entry->tag);
      if (existing_help_tag) free(existing_help_tag);
      return 0;
    } else {
      /* Unknown mode - should not happen but skip for safety */
      snprintf(msg_buf, msg_size, "  [ERROR] %s - unknown mode '%s'\r\n", entry->tag, mode);
      if (existing_help_tag) free(existing_help_tag);
      return -1;
    }
  } else if (keyword_exists && !str_cmp(mode, "skip")) {
    /* Skip entries where keywords already exist (skip mode only) */
    snprintf(msg_buf, msg_size, "  [SKIPPED] %s - keywords already mapped to %s\r\n", 
             entry->tag, existing_help_tag ? existing_help_tag : "another entry");
    if (existing_help_tag) free(existing_help_tag);
    return 0;
  } else {
    snprintf(msg_buf, msg_size, "  [IMPORTED] %s\r\n", entry->tag);
  }
  
  if (existing_help_tag) free(existing_help_tag);
  
  /* Insert the help entry */
  mysql_real_escape_string(conn, escaped_tag, entry->tag, strlen(entry->tag));
  
  /* Allocate escaped_entry buffer dynamically based on entry size */
  entry_len = strlen(entry->entry);
  CREATE(escaped_entry, char, (entry_len * 2 + 1));
  mysql_real_escape_string(conn, escaped_entry, entry->entry, entry_len);
  
  /* Allocate query buffer dynamically */
  query_size = strlen("INSERT INTO help_entries (tag, entry, min_level, auto_generated) VALUES ('', '', , FALSE)") 
               + strlen(escaped_tag) + strlen(escaped_entry) + 20;
  CREATE(query, char, query_size);
  snprintf(query, query_size,
    "INSERT INTO help_entries (tag, entry, min_level, auto_generated) "
    "VALUES ('%s', '%s', %d, FALSE)",
    escaped_tag, escaped_entry, min_level);
  
  if (mysql_query(conn, query) != 0) {
    snprintf(msg_buf, msg_size, "    ERROR: Failed to insert entry: %s\r\n", mysql_error(conn));
    free(escaped_entry);
    free(query);
    return -1;
  }
  free(escaped_entry);
  free(query);
  
  /* Insert keywords */
  char keyword_copy[MAX_STRING_LENGTH];
  char *token, *rest;
  
  strlcpy(keyword_copy, entry->keywords, sizeof(keyword_copy));
  token = strtok_r(keyword_copy, " ", &rest);
  
  while (token) {
    mysql_real_escape_string(conn, escaped_keyword, token, strlen(token));
    
    query_size = strlen("INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('', '')") 
                 + strlen(escaped_tag) + strlen(escaped_keyword) + 1;
    CREATE(query, char, query_size);
    snprintf(query, query_size,
      "INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('%s', '%s')",
      escaped_tag, escaped_keyword);
    
    mysql_query(conn, query); /* Ignore errors for duplicate keywords */
    free(query);
    
    token = strtok_r(NULL, " ", &rest);
  }
  
  return 1;
}

/**
 * Main import function - reads help.hlp and imports to database
 */
static int import_help_hlp_file(struct char_data *ch, const char *mode) {
  FILE *fp;
  struct help_entry_list *entry;
  int min_level;
  int total_entries = 0;
  int imported = 0;
  int skipped = 0;
  int errors = 0;
  char filename[256];
  char *output_buf = NULL;
  size_t output_size = 0;
  size_t output_len = 0;
  
  /* Initialize output buffer */
  output_size = MAX_STRING_LENGTH * 8;
  CREATE(output_buf, char, output_size);
  output_buf[0] = '\0';
  
  /* Helper macro to append to buffer */
  #define APPEND_TO_BUF(fmt, ...) do { \
    size_t needed = snprintf(NULL, 0, fmt, ##__VA_ARGS__) + 1; \
    if (output_len + needed > output_size - 1) { \
      size_t new_size = output_size * 2; \
      RECREATE(output_buf, char, new_size); \
      output_size = new_size; \
    } \
    output_len += snprintf(output_buf + output_len, output_size - output_len, fmt, ##__VA_ARGS__); \
  } while(0)
  
  /* Open the help.hlp file */
  snprintf(filename, sizeof(filename), "text/help/help.hlp");
  
  if (!(fp = fopen(filename, "r"))) {
    send_to_char(ch, "ERROR: Cannot open help.hlp file: %s\r\n", filename);
    mudlog(NRM, LVL_IMPL, TRUE, "HELP IMPORT: Failed to open %s", filename);
    free(output_buf);
    return -1;
  }
  
  APPEND_TO_BUF("Reading help.hlp file from: %s\r\n", filename);
  
  /* Start transaction for non-preview modes */
  if (str_cmp(mode, "preview")) {
    if (mysql_query(conn, "START TRANSACTION") != 0) {
      APPEND_TO_BUF("ERROR: Failed to start transaction: %s\r\n", mysql_error(conn));
      fclose(fp);
      free(output_buf);
      return -1;
    }
  }
  
  /* Process each entry in the file */
  while ((entry = parse_help_entry(fp, &min_level)) != NULL) {
    char msg_buf[MAX_STRING_LENGTH];
    total_entries++;
    
    /* Show progress every 50 entries */
    if (total_entries % 50 == 0) {
      APPEND_TO_BUF("Processing entry %d...\r\n", total_entries);
    }
    
    /* Check for duplicates and import based on mode */
    int result = import_entry_with_resolution(ch, entry, min_level, mode, msg_buf, sizeof(msg_buf));
    
    /* Add the message from import_entry_with_resolution */
    APPEND_TO_BUF("%s", msg_buf);
    
    if (result > 0) {
      imported++;
    } else if (result == 0) {
      skipped++;
    } else {
      errors++;
    }
    
    /* Free the entry */
    free_help_entry(entry);
    
    /* In preview mode, limit output */
    if (!str_cmp(mode, "preview") && total_entries >= 100) {
      APPEND_TO_BUF("\r\n... Preview limited to first 100 entries ...\r\n");
      break;
    }
  }
  
  fclose(fp);
  
  /* Commit or rollback transaction */
  if (str_cmp(mode, "preview")) {
    if (errors > 0) {
      mysql_query(conn, "ROLLBACK");
      APPEND_TO_BUF("\r\nTransaction rolled back due to errors.\r\n");
    } else {
      if (mysql_query(conn, "COMMIT") != 0) {
        APPEND_TO_BUF("ERROR: Failed to commit transaction: %s\r\n", mysql_error(conn));
        mysql_query(conn, "ROLLBACK");
        free(output_buf);
        return -1;
      }
      APPEND_TO_BUF("\r\nTransaction committed successfully.\r\n");
    }
  }
  
  /* Report summary */
  APPEND_TO_BUF("\r\n=== Import Summary ===\r\n");
  APPEND_TO_BUF("Total entries processed: %d\r\n", total_entries);
  
  if (!str_cmp(mode, "preview")) {
    APPEND_TO_BUF("Would import: %d\r\n", imported);
    APPEND_TO_BUF("Would skip: %d\r\n", skipped);
    APPEND_TO_BUF("\r\nThis was a PREVIEW. No changes were made.\r\n");
    APPEND_TO_BUF("Use 'helpgen import skip', 'merge', or 'force' to actually import.\r\n");
  } else {
    APPEND_TO_BUF("Successfully imported: %d\r\n", imported);
    APPEND_TO_BUF("Skipped (duplicates): %d\r\n", skipped);
    APPEND_TO_BUF("Errors: %d\r\n", errors);
    
    if (imported > 0) {
      /* Clear the help cache */
      extern void clear_help_cache(void);
      clear_help_cache();
      APPEND_TO_BUF("\r\nHelp cache cleared. New entries are now available.\r\n");
    }
  }
  
  /* Send paginated output to player */
  page_string(ch->desc, output_buf, TRUE);
  
  /* Clean up */
  free(output_buf);
  #undef APPEND_TO_BUF
  
  mudlog(NRM, LVL_IMPL, TRUE, "HELP IMPORT: %s imported %d entries from help.hlp (%s mode)",
    GET_NAME(ch), imported, mode);
  
  return imported;
}
