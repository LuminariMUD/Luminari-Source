/*/ \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \
\
/  Luminari Account System, Inspired by D20mud's Account System
/  Created By: Ornir
\
/  This file includes both act.h and account.h for header definitions
\         Note: account.h contains external function declarations
/
\ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ /*/

/*
  File overview (beginner friendly):

  This module implements the "account" layer for the Luminari MUD. An "account"
  groups one or more player characters and stores cross-character data such as:
    - Account name, password (string copied from DB), email
    - Account "experience" points (a separate currency used for unlocks)
    - Which races/classes have been unlocked for this account
    - The list of character names that belong to the account

  The module interacts with a MySQL database (via MYSQL* conn declared elsewhere)
  and provides functions to:
    - Load an account and its related data from the DB
    - Save (upsert) account data back to the DB
    - Load a list of character names for an account
    - Load unlocked races/classes for an account
    - Remove a character from an account
    - Check and use account experience to unlock races/classes or adjust alignment
    - Display an account menu and basic account information

  Important conventions and constraints used here (derived from code only):
    - Many limits come from macros defined in headers, for example:
        MAX_CHARS_PER_ACCOUNT, MAX_UNLOCKED_RACES, MAX_UNLOCKED_CLASSES,
        NUM_RACES, NUM_CLASSES, MAX_PWD_LENGTH, etc.
      These determine array sizes and validation ranges.
    - The global 'conn' must point to a valid MySQL connection. This file assumes
      other code initializes it and sets mysql_available, descriptor_list, etc.
    - Many helpers/macros/functions come from other headers (e.g., send_to_char,
      write_to_output, GET_ALIGNMENT, CLSLIST_LOCK, race_list, etc.). This file
      uses those but does not define them.

  Safety notes:
    - All DB reads are followed by mysql_store_result/mysql_free_result.
    - strdup allocations are freed when reloading account data or character lists.
    - Account experience is clamped between 0 and 100,000,000 by change_account_xp.
    - Alignment is clamped to [-1000, 1000] after purchases in do_accexp.
    - SQL injection protection via mysql_real_escape_string for user input in queries.

  This file adds explanatory comments without changing behavior.
*/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "mysql.h"
#include "utils.h"
#include "db.h"
#include "handler.h"
#include "feats.h"
#include "dg_scripts.h"
#include "comm.h"
#include "interpreter.h"
#include "genmob.h"
#include "constants.h"
#include "spells.h"
#include "screen.h"
#include "class.h"
#include "act.h"
#include "account.h"
#include "routing.h"
#include "campaign.h"

extern MYSQL *conn;

/* Forward reference: helper loaders for attached structures on an account */
void load_account_characters(struct account_data *account);
void load_account_unlocks(struct account_data *account);

/* Simple aliases for boolean-like flags used in this file. */
#define Y TRUE
#define N FALSE

/* start functions! */

/*
  locked_race_cost(int race)
  Purpose: Return the account-experience cost to unlock a given race index.
  Parameters:
    - race: index into race_list (assumed valid by the caller).
  Return:
    - Integer cost stored in race_list[race].unlock_cost.
  Side effects: None (pure lookup).
  Notes:
    - No bounds checking here; callers should ensure 'race' is in range.
*/
int locked_race_cost(int race)
{
  return (race_list[race].unlock_cost);
}

/*
  is_locked_race(int race)
  Purpose: Determine if a race requires unlocking.
  Parameters:
    - race: index into race_list.
  Return:
    - TRUE if race_list[race].unlock_cost > 0, otherwise FALSE.
  Side effects: None.
  Notes:
    - A race with unlock_cost 0 is considered always available.
*/
bool is_locked_race(int race)
{
  if (race_list[race].unlock_cost > 0)
    return TRUE;

  return FALSE;
}

/*
  change_account_xp(struct char_data *ch, int change_val)
  Purpose: Adjust the account experience for the account tied to a character.
  Parameters:
    - ch: character whose descriptor/account will be updated
    - change_val: positive or negative delta to apply
  Return:
    - The resulting account experience after clamping.
  Behavior and constraints:
    - Clamps experience to [0, 100000000].
    - Persists the updated account via save_account(ch->desc->account).
  Requirements:
    - ch->desc and ch->desc->account must be valid (assumed by callers here).
*/
int change_account_xp(struct char_data *ch, int change_val)
{
  GET_ACCEXP_DESC(ch) += change_val;

  if (GET_ACCEXP_DESC(ch) < 0)
    GET_ACCEXP_DESC(ch) = 0;

  if (GET_ACCEXP_DESC(ch) > 100000000)
    GET_ACCEXP_DESC(ch) = 100000000;

  /* Persist to DB and update other descriptors that share this account */
  save_account(ch->desc->account);

  return GET_ACCEXP_DESC(ch);
}

/*
  has_unlocked_race(struct char_data *ch, int race)
  Purpose: Check if the account associated with 'ch' has unlocked a race.
  Parameters:
    - ch: character providing access to descriptor/account
    - race: race index
  Return:
    - TRUE if the race is not locked, or if it appears in account->races[]
    - FALSE otherwise or if preconditions fail (e.g., no descriptor/account)
  Conditional compilation:
    - For some campaigns (FR/DL), fewer constraints are checked here.
  Notes:
    - The loop searches up to MAX_UNLOCKED_RACES for an exact match.
*/
int has_unlocked_race(struct char_data *ch, int race)
{

  if (IS_CAMPAIGN_DL || IS_CAMPAIGN_FR)
  {
    // In FR/DL campaigns, we allow LICH and Vampire
    if (!ch || !ch->desc || !ch->desc->account)
      return FALSE;
  }
  else
  {
    // In non-FR/DL builds, LICH and VAMPIRE races are always locked out here.
    if (!ch || !ch->desc || !ch->desc->account || race == RACE_LICH || race == RACE_VAMPIRE)
      return FALSE;
  }

  /* If a race isn't locked, it's available by default. */
  if (!is_locked_race(race))
    return TRUE;

  int i = 0;

  for (i = 0; i < MAX_UNLOCKED_RACES; i++)
    if (ch->desc->account->races[i] == race)
      return TRUE;

  return FALSE;
}

/*
  has_unlocked_class(struct char_data *ch, int class)
  Purpose: Check if the account associated with 'ch' has unlocked a class.
  Parameters:
    - ch: character with an attached account
    - class: class index
  Return:
    - TRUE if the class is not flagged as locked (CLSLIST_LOCK false)
      or if the account has that class in account->classes[]
    - FALSE otherwise or if the account/descriptor is missing.
*/
int has_unlocked_class(struct char_data *ch, int class)
{
  if (!ch || !ch->desc || !ch->desc->account)
    return FALSE;

  /* If the class isn't locked by design, it's available. */
  if (!CLSLIST_LOCK(class))
    return TRUE;

  int i = 0;

  for (i = 0; i < MAX_UNLOCKED_CLASSES; i++)
    if (ch->desc->account->classes[i] == class)
      return TRUE;

  return FALSE;
}

/* Fixed cost per alignment-change purchase via 'accexp align ...' */
#define ALIGN_COST 2000

/*
  do_accexp (command)
  Purpose: Player command handler to spend account experience on:
    - Alignment adjustments (good/evil), or
    - Unlocking races, or
    - Unlocking classes
  Input format:
    accexp [class | race | align] [name-of-class | name-of-race | good|evil]
  Behavior:
    - For align changes: costs ALIGN_COST and shifts alignment +/- 100 with clamps.
    - For races: lists lockable races or purchases one if affordable and slot available.
    - For classes: lists lockable classes or purchases one if affordable and slot available.
  Side effects:
    - May adjust GET_ALIGNMENT(ch) with clamping [-1000, 1000].
    - Deducts account experience (change_account_xp).
    - Writes user feedback via send_to_char.
    - For race/class unlocks, writes into account arrays and saves account.
  Safety:
    - Performs null checks for descriptor/account presence.
    - Bounds: loops limited by MAX_* macros; name matching via is_abbrev.
*/
ACMD(do_accexp)
{
  char arg[MAX_INPUT_LENGTH] = {'\0'}, arg2[MAX_INPUT_LENGTH] = {'\0'};
  int i = 0, j = 0;
  int cost = 0;
  int align_change = 100;
  const char *remainder;

  /* Get first argument */
  remainder = one_argument(argument, arg, sizeof(arg));
  /* For class/race commands, get the rest of the line as arg2 */
  if (is_abbrev(arg, "class") || is_abbrev(arg, "race")) {
    /* Skip spaces and copy rest of line */
    skip_spaces_c(&remainder);
    strlcpy(arg2, remainder, sizeof(arg2));
  } else {
    /* For other commands, get the next single argument */
    one_argument(remainder, arg2, sizeof(arg2));
  }

  if (!*arg)
  {
    send_to_char(ch, "Usage: accexp [class | race | align] [<class-name to unlock> | "
                     "<race-name to unlock> | <evil OR good>]\r\n");
    return;
  }

  /* Alignment purchase branch */
  if (is_abbrev(arg, "align"))
  {

    cost = ALIGN_COST;

    if (!*arg2)
    {
      send_to_char(ch, "Please choose 'good' for good alignment change or 'evil' for evil alignment change.  It cost %d account exp for each.\r\n", cost);
      return;
    }

    if (is_abbrev(arg2, "evil"))
    {
      align_change *= -1;
    }
    else if (is_abbrev(arg2, "good"))
    {
      ; /* base value above is fine */
    }
    else
    {
      send_to_char(ch, "Please choose 'good' for good alignment change or 'evil' for evil alignment change.  It cost %d account exp for each.\r\n", cost);
      return;
    }

    if (ch->desc && ch->desc->account)
    {
      /* Hard bounds check on resulting alignment to prevent exceeding ±1000 */
      if ((GET_ALIGNMENT(ch) + align_change) > 1000 || (GET_ALIGNMENT(ch) + align_change) < -1000)
      {
        send_to_char(ch, "You have the maximum alignment already!\r\n");
        return;
      }
      else if (GET_ACCEXP_DESC(ch) >= cost)
      {
        change_account_xp(ch, -cost);
        send_to_char(ch, "You have changed your alignment by %d points, costing %d account points!\r\n",
                     align_change, cost);

        GET_ALIGNMENT(ch) += align_change;

        /* Final clamp to the game-legal range [-1000, 1000]. */
        if (GET_ALIGNMENT(ch) > 1000)
        {
          GET_ALIGNMENT(ch) = 1000;
          send_to_char(ch, "You have the maximum good alignment now.\r\n");
        }

        if (GET_ALIGNMENT(ch) < -1000)
        {
          GET_ALIGNMENT(ch) = -1000;
          send_to_char(ch, "You have the maximum evil alignment now.\r\n");
        }

        return;
      }
      else
      {
        send_to_char(ch, "You need %d account experience to purchase that change and you only have %d.\r\n",
                     cost, GET_ACCEXP_DESC(ch));
        return;
      }
    }
    else
    {
      send_to_char(ch, "There is a problem with your account and the item could "
                       "not be unlocked.  Please submit a request to staff.\r\n");
      return;
    }
  }
  /* try to unlock a race */
  else if (is_abbrev(arg, "race"))
  {
    int start = 0;
    int end = 0;

    if (IS_CAMPAIGN_FR) {
      start = 0;
      end = NUM_EXTENDED_PC_RACES;
    } else if (IS_CAMPAIGN_DL) {
      start = DL_RACE_START;
      end = DL_RACE_END;
    } else {
      start = 0;
      end = NUM_RACES;
    }

    /* No argument: list lockable races that are not yet unlocked */
    if (!*arg2)
    {
      send_to_char(ch, "Please choose from the following races:\r\n");

      for (i = start; i < end; i++) {
        if (!is_locked_race(i) || has_unlocked_race(ch, i))
          continue;

        int cost = locked_race_cost(i);
        send_to_char(ch, "%s (%d account experience)\r\n", race_list[i].type, cost);
      }
    }

    /* Identify the intended race to unlock by name abbreviation */
  for (i = start; i < end; i++) {
    if (race_list[i].is_pc &&
        is_abbrev(arg2, race_list[i].type) &&
        is_locked_race(i) &&
        !has_unlocked_race(ch, i)) {
      cost = locked_race_cost(i);
      break;
    }
  }

  if (i >= end) {
    send_to_char(ch, "Either that race does not exist, is not an advanced race, "
                     "is not available for players, or you've already unlocked it.\r\n");
    return;
  }
      {
        send_to_char(ch, "Either that race does not exist, is not an advanced race, "
                         "is not available for players, or you've already unlocked it.\r\n");
        return;
      }
    if (ch->desc && ch->desc->account)
    {
      /* Find an empty slot in account->races array */
      for (j = 0; j < MAX_UNLOCKED_RACES; j++)
      {
        if (ch->desc->account->races[j] == 0) /* 0 means empty slot */
          break;
      }
      if (j >= MAX_UNLOCKED_RACES)
      {
        send_to_char(ch, "All of your advanced race slots are filled.  Please "
                         "submit a petition to ask for the limit to be increased.\r\n");
        return;
      }
      if (GET_ACCEXP_DESC(ch) >= cost)
      {
        ch->desc->account->races[j] = i;
        send_to_char(ch, "You have unlocked the advanced race '%s' for all character "
                         "and future characters on your account!.\r\n",
                     race_list[i].type);
        change_account_xp(ch, -cost); /* this will call save_account() for us */
        return;
      }
      else
      {
        send_to_char(ch, "You need %d account experience to purchase that advanced "
                         "race and you only have %d.\r\n",
                     cost, GET_ACCEXP_DESC(ch));
        return;
      }
    }
    else
    {
      send_to_char(ch, "There is a problem with your account and the race could "
                       "not be unlocked.  Please submit a request to staff.\r\n");
      return;
    }

    /* try to unlock a locked class */
  }
  else if (is_abbrev(arg, "class"))
  {
    /* No argument: list lockable classes that are not yet unlocked */
    if (!*arg2)
    {
      send_to_char(ch, "Please choose from the following classes:\r\n");
      for (i = 0; i < NUM_CLASSES; i++)
      {
        if (has_unlocked_class(ch, i) || !CLSLIST_LOCK(i))
          continue;
        if (IS_CAMPAIGN_DL)
          cost = CLSLIST_COST(i) / 10;
        else
          cost = CLSLIST_COST(i);
        send_to_char(ch, "%s (%d account experience)\r\n", CLSLIST_NAME(i), cost);
      }
      return;
    }
    /* Identify class to unlock by name abbreviation */
    for (i = 0; i < NUM_CLASSES; i++)
    {
      /* Skip already unlocked classes and non-lockable classes */
      if (has_unlocked_class(ch, i) || !CLSLIST_LOCK(i))
        continue;
        
      /* Check if this class matches the input */
      if (is_abbrev(arg2, CLSLIST_NAME(i)))
      {
        if (IS_CAMPAIGN_DL)
          cost = CLSLIST_COST(i) / 10;
        else
          cost = CLSLIST_COST(i);
        break;
      }
      
      /* For knight classes, also check alternate names */
      if (i >= CLASS_KNIGHT_OF_THE_CROWN && i <= CLASS_KNIGHT_OF_THE_LILY)
      {
        bool matches_alternate = FALSE;
        switch (i) {
          case CLASS_KNIGHT_OF_THE_CROWN:
#ifdef CAMPAIGN_DL
            matches_alternate = is_abbrev(arg2, "knight of the crimson loom");
#else
            matches_alternate = is_abbrev(arg2, "knight of the crown");
#endif
            break;
          case CLASS_KNIGHT_OF_THE_SWORD:
#ifdef CAMPAIGN_DL
            matches_alternate = is_abbrev(arg2, "knight of the sundered dawn");
#else
            matches_alternate = is_abbrev(arg2, "knight of the sword");
#endif
            break;
          case CLASS_KNIGHT_OF_THE_ROSE:
#ifdef CAMPAIGN_DL
            matches_alternate = is_abbrev(arg2, "knight of the ember throne");
#else
            matches_alternate = is_abbrev(arg2, "knight of the rose");
#endif
            break;
          case CLASS_KNIGHT_OF_THE_LILY:
#ifdef CAMPAIGN_DL
            matches_alternate = is_abbrev(arg2, "knight of the howling moon");
#else
            matches_alternate = is_abbrev(arg2, "knight of the lily");
#endif
            break;
          case CLASS_KNIGHT_OF_THE_THORN:
#ifdef CAMPAIGN_DL
            matches_alternate = is_abbrev(arg2, "knight of the shattered mirror");
#else
            matches_alternate = is_abbrev(arg2, "knight of the thorn");
#endif
            break;
          case CLASS_KNIGHT_OF_THE_SKULL:
#ifdef CAMPAIGN_DL
            matches_alternate = is_abbrev(arg2, "knight of the pale throne");
#else
            matches_alternate = is_abbrev(arg2, "knight of the skull");
#endif
            break;
        }
        
        if (matches_alternate)
        {
          if (IS_CAMPAIGN_DL)
            cost = CLSLIST_COST(i) / 10;
          else
            cost = CLSLIST_COST(i);
          break;
        }
      }
    }
    if (i >= NUM_CLASSES)
    {
      send_to_char(ch, "Either that class does not exist, is not a prestige class, "
                       "is not available for players, or you've already unlocked it.\r\n");
      return;
    }
    if (ch->desc && ch->desc->account)
    {
      /* Find empty slot in account->classes array */
      for (j = 0; j < MAX_UNLOCKED_CLASSES; j++)
      {
        if (ch->desc->account->classes[j] == 0) /* 0 means empty slot */
          break;
      }
      if (j >= MAX_UNLOCKED_CLASSES)
      {
        send_to_char(ch, "All of your prestige class slots are filled.  Please "
                         "Ask the staff for the limit to be increased.\r\n");
        return;
      }
      if (GET_ACCEXP_DESC(ch) >= cost)
      {
        ch->desc->account->classes[j] = i;
        send_to_char(ch, "You have unlocked the prestige class '%s' for all "
                         "character and future characters on your account!.\r\n",
                     CLSLIST_NAME(i));
        change_account_xp(ch, -cost); /* this will call save_account() for us */
        return;
      }
      else
      {
        send_to_char(ch, "You need %d account experience to purchase that prestige class and you only have %d.\r\n",
                     cost, GET_ACCEXP_DESC(ch));
        return;
      }
    }
    else
    {
      send_to_char(ch, "There is a problem with your account and the class could "
                       "not be unlocked.  Please submit your issue to the staff.\r\n");
      return;
    }
  }
  else
  {
    send_to_char(ch, "You must choose to unlock either a race, class or alignment change.\r\n");
    return;
  }
}

/*
  load_account(char *name, struct account_data *account)
  Purpose: Load an account record (and then its characters/unlocks) from the DB by account name.
  Parameters:
    - name: account name to look up (case-insensitive)
    - account: pointer to a pre-allocated struct account_data to populate
  Return codes:
    - 0 on success
    - -1 on failure (DB unavailable, query error, or account not found)
  Behavior:
    - Frees any pre-existing owned memory inside 'account' (name, email, character_names[]).
    - Reads id, name, password, experience, email from 'account_data'.
    - Ensures password is null-terminated at MAX_PWD_LENGTH.
    - Calls load_account_characters() and load_account_unlocks() to populate arrays.
  Notes:
    - Uses mysql_ping(conn) before querying to ensure connection is alive.
*/
int load_account(char *name, struct account_data *account)
{
  MYSQL_RES *result;
  MYSQL_ROW row;
  char buf[2048];

  /* Check if MySQL is available */
  if (!mysql_available || !conn) {
    return -1;  /* Account not found - no MySQL */
  }

  /* Check if the account has data, if so, clear it. */
  if (account != NULL) {
    int i;
    if (account->name != NULL) {
      free(account->name);
      account->name = NULL;
    }
    if (account->email != NULL) {
      free(account->email);
      account->email = NULL;
    }
    for (i = 0; i < MAX_CHARS_PER_ACCOUNT; i++) {
      if (account->character_names[i] != NULL) {
        free(account->character_names[i]);
        account->character_names[i] = NULL;
      }
    }
  }
  /* Check the connection, reconnect if necessary. */
  if (!MYSQL_PING_CONN(conn)) {
    log("SYSERR: %s: Database connection failed", __func__);
    return -1;
  }

  /* Escape the account name to prevent SQL injection */
  char escaped_name[MAX_INPUT_LENGTH * 2 + 1];
  mysql_real_escape_string(conn, escaped_name, name, strlen(name));
  
  /* Case-insensitive match on the escaped account name */
  snprintf(buf, sizeof(buf), "SELECT id, name, password, experience, email from account_data where lower(name) = lower('%s')",
           escaped_name);

  if (mysql_query(conn, buf))
  {
    log("SYSERR: Unable to SELECT from account_data: %s", mysql_error(conn));
    return -1;
  }

  if (!(result = mysql_store_result(conn)))
  {
    log("SYSERR: Unable to SELECT from account_data: %s", mysql_error(conn));
    return -1;
  }

  if (!(row = mysql_fetch_row(result)))
  {
    mysql_free_result(result);
    return -1; /* Account not found. */
  }

  account->id = atoi(row[0]);
  account->name = strdup(row[1]);
  strncpy(account->password, row[2], MAX_PWD_LENGTH);
  account->password[MAX_PWD_LENGTH] = '\0'; /* Ensure null termination */
  account->experience = atoi(row[3]);
  account->email = (row[4] ? strdup(row[4]) : NULL);

  mysql_free_result(result);
  load_account_characters(account);
  load_account_unlocks(account);

  return (0);
}

/*
  cleanup_duplicate_characters(struct account_data *account)
  Purpose: Remove duplicate character entries from player_data for this account.
  Parameters:
    - account: account to clean up
  Behavior:
    - For each character name, keeps only one entry per character name
    - Uses LIMIT to keep first row, deletes rest
  Notes:
    - Should be called before load_account_characters when duplicates are detected
*/
void cleanup_duplicate_characters(struct account_data *account)
{
  MYSQL_RES *result;
  MYSQL_ROW row;
  char buf[2048];
  
  if (!account || account->id <= 0)
    return;
    
  /* Get list of duplicate character names for this account */
  snprintf(buf, sizeof(buf), 
    "SELECT name, COUNT(*) as cnt "
    "FROM player_data "
    "WHERE account_id = %d "
    "GROUP BY name "
    "HAVING cnt > 1", account->id);
    
  if (mysql_query(conn, buf))
  {
    log("SYSERR: Unable to check for duplicate characters: %s", mysql_error(conn));
    return;
  }
  
  if (!(result = mysql_store_result(conn)))
  {
    log("SYSERR: Unable to store duplicate check results: %s", mysql_error(conn));
    return;
  }
  
  /* Process each duplicate character */
  while ((row = mysql_fetch_row(result)))
  {
    char *name = row[0];
    int count = atoi(row[1]);
    
    /* Escape character name */
    char escaped_name[MAX_INPUT_LENGTH * 2 + 1];
    mysql_real_escape_string(conn, escaped_name, name, strlen(name));
    
    /* Delete all but one - we delete count-1 duplicates
     * ORDER BY ensures we keep a consistent row (the "first" one)
     * This approach works regardless of table structure */
    snprintf(buf, sizeof(buf),
      "DELETE FROM player_data "
      "WHERE account_id = %d "
      "AND lower(name) = lower('%s') "
      "ORDER BY name "
      "LIMIT %d",
      account->id, escaped_name, count - 1);
      
    if (mysql_query(conn, buf))
    {
      log("SYSERR: Unable to delete duplicate character %s: %s", name, mysql_error(conn));
    }
    else
    {
      log("Info: Cleaned up %ld duplicate(s) of character %s for account %s",
        (long)mysql_affected_rows(conn), name, account->name);
    }
  }
  
  mysql_free_result(result);
}

/*
  load_account_characters(struct account_data *account)
  Purpose: Populate account->character_names[] with names belonging to this account.
  Parameters:
    - account: account whose 'id' is already set.
  Behavior:
    - Clears/free any existing strings in character_names[].
    - SELECT name FROM player_data WHERE account_id = account->id
    - Copies up to MAX_CHARS_PER_ACCOUNT results using strdup.
    - Now includes duplicate cleanup if needed
*/
void load_account_characters(struct account_data *account)
{
  MYSQL_RES *result;
  MYSQL_ROW row;
  char buf[2048];
  int i = 0;

  /* Free existing names to avoid leaks on reload */
  for (i = 0; i < MAX_CHARS_PER_ACCOUNT; i++)
    if (account->character_names[i] != NULL)
    {
      free(account->character_names[i]);
      account->character_names[i] = NULL;
    }

  /* First check if we need to clean up duplicates */
  snprintf(buf, sizeof(buf), 
    "SELECT COUNT(*) as total, COUNT(DISTINCT name) as unique_names "
    "FROM player_data WHERE account_id = %d", account->id);
    
  if (mysql_query(conn, buf))
  {
    log("SYSERR: Unable to check for duplicates: %s", mysql_error(conn));
  }
  else if ((result = mysql_store_result(conn)))
  {
    if ((row = mysql_fetch_row(result)))
    {
      int total = atoi(row[0]);
      int unique_count = atoi(row[1]);
      if (total > unique_count)
      {
        log("Info: Detected %d duplicate character entries for account %s, cleaning up...", 
            total - unique_count, account->name);
        mysql_free_result(result);
        cleanup_duplicate_characters(account);
      }
      else
      {
        mysql_free_result(result);
      }
    }
    else
    {
      mysql_free_result(result);
    }
  }

  /* Now load the character names (duplicates have been cleaned) */
  snprintf(buf, sizeof(buf), "select name from player_data where account_id = %d", account->id);

  if (mysql_query(conn, buf))
  {
    log("SYSERR: Unable to SELECT from player_data: %s", mysql_error(conn));
    return;
  }
  if (!(result = mysql_store_result(conn)))
  {
    log("SYSERR: Unable to SELECT from player_data: %s", mysql_error(conn));
    return;
  }

  i = 0;
  while ((row = mysql_fetch_row(result)) && i < MAX_CHARS_PER_ACCOUNT)
  {
    account->character_names[i] = strdup(row[0]);
    i++;
  }

  mysql_free_result(result);
  return;
}

/*
  load_account_unlocks(struct account_data *account)
  Purpose: Populate account->classes[] and account->races[] with unlocked IDs.
  Parameters:
    - account: account whose 'id' is set.
  Behavior:
    - SELECT class_id FROM unlocked_classes WHERE account_id = ...
    - SELECT race_id FROM unlocked_races WHERE account_id = ...
    - Fills arrays up to their max sizes.
  Notes:
    - Existing values in arrays are overwritten in order.
*/
void load_account_unlocks(struct account_data *account)
{
  MYSQL_RES *result;
  MYSQL_ROW row;
  char buf[2048];
  int i = 0;

  /* load locked classes */
  snprintf(buf, sizeof(buf), "SELECT class_id from unlocked_classes "
                             "WHERE account_id = %d",
           account->id);

  if (mysql_query(conn, buf))
  {
    log("SYSERR: Unable to SELECT from unlocked_classes: %s", mysql_error(conn));
    return;
  }

  if (!(result = mysql_store_result(conn)))
  {
    log("SYSERR: Unable to SELECT from unlocked_classes: %s", mysql_error(conn));
    return;
  }

  i = 0;
  
  while ((row = mysql_fetch_row(result)) && i < MAX_UNLOCKED_CLASSES)
  {
    account->classes[i] = atoi(row[0]);
    i++;
  }
  mysql_free_result(result);

  /* load locked races */
  snprintf(buf, sizeof(buf), "SELECT race_id from unlocked_races "
                             "WHERE account_id = %d",
           account->id);
  if (mysql_query(conn, buf))
  {
    log("SYSERR: Unable to SELECT from unlocked_races: %s", mysql_error(conn));
    return;
  }
  if (!(result = mysql_store_result(conn)))
  {
    log("SYSERR: Unable to SELECT from unlocked_races: %s", mysql_error(conn));
    return;
  }
  i = 0;
  while ((row = mysql_fetch_row(result)) && i < MAX_UNLOCKED_RACES)
  {
    account->races[i] = atoi(row[0]);
    i++;
  }

  /* cleanup */
  mysql_free_result(result);
  return;
}

/*
  get_char_account_name(char *name)
  Purpose: Given a character name, return a newly-allocated string with the
           owning account name, or NULL if not found.
  Parameters:
    - name: exact character name (matched in SQL with quoted literal)
  Return:
    - char* allocated with strdup, caller must free, or NULL on error/not found.
  Behavior:
    - Joins account_data and player_data to find account name by character.
    - Uses proper SQL escaping to prevent injection.
    - If multiple rows are returned, it frees the previous copy and keeps the last.
*/
char *get_char_account_name(char *name)
{
  MYSQL_RES *result;
  MYSQL_ROW row;
  char buf[2048];
  char *acct_name = NULL;

  /* Escape character name to prevent SQL injection */
  char escaped_name[MAX_INPUT_LENGTH * 2 + 1];
  mysql_real_escape_string(conn, escaped_name, name, strlen(name));
  
  snprintf(buf, sizeof(buf), "select a.name from account_data a, player_data p where p.account_id = a.id and p.name = '%s'", escaped_name);

  if (mysql_query(conn, buf))
  {
    log("SYSERR: Unable to retrieve account name for character %s: %s", name, mysql_error(conn));
    return NULL;
  }
  if (!(result = mysql_store_result(conn)))
  {
    log("SYSERR: Unable to retreive account name for character %s: %s", name, mysql_error(conn));
    return NULL;
  }
  while ((row = mysql_fetch_row(result)))
  {
    if (acct_name)
      free(acct_name);  /* Free previous allocation if multiple rows */
    acct_name = (row[0] ? strdup(row[0]) : NULL);
  }
  mysql_free_result(result);
  return acct_name;
}

/*
  save_account(struct account_data *account)
  Purpose: Upsert account data and associated arrays (characters, races, classes) into DB.
  Parameters:
    - account: pointer to populated account (id may be 0 for new)
  Behavior:
    1) Upserts into account_data (id, name, password, experience, email).
       - If id is 0 (new), retrieves auto-generated id via mysql_insert_id.
    2) Upserts each character name in player_data with account_id.
    3) Upserts each entry in unlocked_races and unlocked_classes.
    4) Iterates descriptor_list so that all active descriptors sharing this account id
       get their unlock lists refreshed and their displayed account experience updated.
  Safety:
    - Checks for NULL account and logs error.
    - Uses VALUES(...) UPSERT pattern.
*/
void save_account(struct account_data *account)
{
  char buf[2048];
  int i = 0;
  struct descriptor_data *j, *next_desc;

  if (account == NULL)
  {
    log("SYSERR: Attempted to save NULL account.");
    return;
  }

  snprintf(buf, sizeof(buf), "INSERT into account_data (id, name, password, experience, email) values (%d, '%s', '%s', %d, %s%s%s)"
                             " on duplicate key update password = VALUES(password), "
                             "                         experience = VALUES(experience), "
                             "                         email = VALUES(email);",
           account->id,
           account->name,
           account->password,
           account->experience,
           (account->email ? "'" : ""),
           (account->email ? account->email : "NULL"),
           (account->email ? "'" : ""));

  if (mysql_query(conn, buf))
  {
    log("SYSERR: Unable to UPSERT into account_data: %s", mysql_error(conn));
    return;
  }

  if (account->id == 0) /* This is a new account! */
    account->id = mysql_insert_id(conn);

  /* Update account_id for characters belonging to this account
   * We only UPDATE, not INSERT - characters already exist in player_data from creation */
  for (i = 0; (i < MAX_CHARS_PER_ACCOUNT) && (account->character_names[i] != NULL); i++)
  {
    buf[0] = '\0';
    /* Escape character name to prevent SQL injection */
    char escaped_name[MAX_INPUT_LENGTH * 2 + 1];
    mysql_real_escape_string(conn, escaped_name, account->character_names[i], strlen(account->character_names[i]));
    
    snprintf(buf, sizeof(buf), "UPDATE player_data SET account_id = %d "
                               "WHERE lower(name) = lower('%s');",
             account->id, escaped_name);
    if (mysql_query(conn, buf))
    {
      /* Log error but continue - don't abort for single character update failure */
      log("SYSERR: Unable to UPDATE player_data for %s: %s", account->character_names[i], mysql_error(conn));
    }
  }

  /* save unlocked races */
  for (i = 0; i < MAX_UNLOCKED_RACES; i++)
  {
    buf[0] = '\0';
    snprintf(buf, sizeof(buf), "INSERT into unlocked_races (account_id, race_id) "
                               "VALUES (%d, %d)"
                               "on duplicate key update race_id = VALUES(race_id);",
             account->id, account->races[i]);
    if (mysql_query(conn, buf))
    {
      log("SYSERR: Unable to UPSERT unlocked_races: %s", mysql_error(conn));
      return;
    }
  }

  /* save unlocked classes */
  for (i = 0; i < MAX_UNLOCKED_CLASSES; i++)
  {
    buf[0] = '\0';
    snprintf(buf, sizeof(buf), "INSERT into unlocked_classes (account_id, class_id) "
                               "VALUES (%d, %d)"
                               "on duplicate key update class_id = VALUES(class_id);",
             account->id, account->classes[i]);
    if (mysql_query(conn, buf))
    {
      log("SYSERR: Unable to UPSERT unlocked_classes: %s", mysql_error(conn));
      return;
    }
  }

  /* what happens if you have multiple characters logged on at the same time?
     We need to update all characters in game with this account id
     so that they reflect new unlocks/experience immediately. */
  for (j = descriptor_list; j; j = next_desc)
  {
    next_desc = j->next;

    if (j->account)
    {
      if (j->account->id == account->id)
      {
        /* Reload unlock arrays from DB to keep them in sync */
        load_account_unlocks(j->account);
        if (IS_PLAYING(j))
          GET_ACCEXP_DESC(j->character) = account->experience;
      }
    }
  }
}

/*
  show_account_menu(struct descriptor_data *d)
  Purpose: Display a menu of characters on the descriptor's account with summary info.
  Parameters:
    - d: active descriptor with d->account set
  Behavior:
    - Renders a table header
    - Iterates through account->character_names[], for each:
        * Attempts to load character data to read level, race abbrev, and class list
        * Skips deleted characters
        * Prints staff title if level >= LVL_IMMORT, otherwise prints composed class list
    - Prints footer and available menu choices
    - Sets the descriptor state to CON_ACCOUNT_MENU
  Notes:
    - Uses temporary char_data allocations (xtch and tch) to safely load player data.
    - Escapes character names before querying player_data to avoid SQL issues.
*/
void show_account_menu(struct descriptor_data *d)
{
  int i = 0;
  struct char_data *tch = NULL, *xtch = NULL;
  char buf[MAX_STRING_LENGTH] = {'\0'};
  size_t len = 0;

  write_to_output(d, "\tC%s\tn", text_line_string("", 80, '-', '-'));
  write_to_output(d, "  \tc#  \tC| \tcName                \tC| \tcLvl \tC| \tcRace \tC| \tcClass\tn \r\n");
  write_to_output(d, "\tC%s\tn", text_line_string("", 80, '-', '-'));

  /*  Check the connection, reconnect if necessary. */
  if (!MYSQL_PING_CONN(conn)) {
    log("SYSERR: save_account: Database connection failed");
    return;
  }

  MYSQL_RES *res = NULL;
  MYSQL_ROW row = NULL;

  char query[MAX_INPUT_LENGTH] = {'\0'};

  if (d->account)
  {
    for (i = 0; i < MAX_CHARS_PER_ACCOUNT; i++)
    {
      /* Initialize a place for the player data to temporarily reside. */
      CREATE(xtch, struct char_data, 1);
      clear_char(xtch);
      CREATE(xtch->player_specials, struct player_special_data, 1);
      new_mobile_data(xtch);

      if (d->account->character_names[i] != NULL && load_char(d->account->character_names[i], xtch) > -1)
      {
        /* Character loaded successfully, we're done with xtch */
        free_char(xtch);
        xtch = NULL;

        write_to_output(d, " \tW%-3d\tn \tC|\tn \tW%-20s\tn\tC|\tn", i + 1, d->account->character_names[i]);
        char *escaped_name = mysql_escape_string_alloc(conn, d->account->character_names[i]);
        if (!escaped_name) {
          log("SYSERR: Failed to escape character name in display_account_menu");
          continue;
        }
        snprintf(query, sizeof(query), "SELECT name FROM player_data WHERE lower(name)=lower('%s')", escaped_name);
        free(escaped_name);

        if (mysql_query(conn, query))
        {
          log("SYSERR: Unable to SELECT from player_data: %s", mysql_error(conn));
        }

        if (!(res = mysql_store_result(conn)))
        {
          log("SYSERR: Unable to SELECT from player_data: %s", mysql_error(conn));
        }

        if (res != NULL)
        {
          if ((row = mysql_fetch_row(res)) != NULL)
          {
            /* Initialize another temporary char to format line output. */
            CREATE(tch, struct char_data, 1);
            clear_char(tch);
            CREATE(tch->player_specials, struct player_special_data, 1);
            new_mobile_data(tch);

            if ((load_char(d->account->character_names[i], tch)) > -1)
            {
              /* Player found! */
              if (PLR_FLAGGED(tch, PLR_DELETED))
              {
                write_to_output(d, " \tR---===||DELETED||===---\tn\r\n");
                free_char(tch);
                mysql_free_result(res);
                continue;
              }

              /* Level and race abbreviation with color formatting */
              write_to_output(d, " %3d \tC|\tn %4s \tC|\tn", GET_LEVEL(tch), race_list[GET_REAL_RACE(tch)].abbrev_color);

              if (GET_LEVEL(tch) >= LVL_IMMORT)
              {
                /* Staff */
                write_to_output(d, " %-36s", admin_level_names[(GET_LEVEL(tch) - LVL_IMMORT)]);
              }
              else
              {
                /* Mortal: build a slash-separated class abbreviation string */
                int inc, classCount = 0;
                buf[0] = '\0';
                len = 0;
                for (inc = 0; inc < MAX_CLASSES; inc++)
                {
                  if (CLASS_LEVEL(tch, inc))
                  {
                    if (classCount)
                      len += snprintf(buf + len, sizeof(buf) - len, "/");
                    len += snprintf(buf + len, sizeof(buf) - len, "%s",
                                    CLSLIST_CLRABBRV(inc));
                    classCount++;
                  }
                }
                write_to_output(d, " %-36s", buf);
              }
            }
            free_char(tch);
          }
        }
        mysql_free_result(res);
        write_to_output(d, "\r\n");
      }
      else
      {
        /* Load failed or character name was NULL - free the allocated memory */
        free_char(xtch);
        xtch = NULL;
      }
    }
  }
  write_to_output(d, "You can view more info about your account by typing "
                     "'account' in-game.\r\n");
  write_to_output(d, "\tC%s\tn", text_line_string("", 80, '-', '-'));
  write_to_output(d, "\tcType the # of a character listed above or choose one of the following:\r\n");
  write_to_output(d, "\tC%s\tn", text_line_string("", 80, '-', '-'));
  write_to_output(d, " \tn(\tCC\tn)\tcreate a new character \tC| \tn(\tCA\tn)\tcdd a character      \tC| \tn(\tCQ\tn)\tcuit\tn\r\n");
  write_to_output(d, "\tC%s\tn", text_line_string("", 80, '-', '-'));
  write_to_output(d, "\tcYour choice :\tn ");

  /* Set this here so we don't have to do it everywhere this procedure is called. */
  STATE(d) = CON_ACCOUNT_MENU;
}

/*
void combine_accounts(void) {

  struct descriptor_data *d;
  struct descriptor_data *k;

  for (d = descriptor_list; d; d = d->next) {
    for (k = descriptor_list; k; k = k->next) {
      if (d && k && d->account && k->account && d->account != k->account &&
        d->character && k->character && d->character->account_name && k->character->account_name &&
        !strcmp(d->character->account_name, k->character->account_name)) {
        d->account = k->account;
        return;
      }
    }
  }
}
 */

/*
  perform_do_account(struct char_data *ch, struct char_data *vict)
  Purpose: Core logic to display account information for 'vict' to 'ch'.
  Parameters:
    - ch: viewer (may be same as vict)
    - vict: the character whose account info to display
  Behavior:
    - Validates that 'vict' is a player with a descriptor and account.
    - Prints email (or Not Set), experience, character list, unlocked races/classes.
    - Adds a tip if 'ch == vict' about the 'accexp' command.
*/
void perform_do_account(struct char_data *ch, struct char_data *vict)
{
  bool found = FALSE;
  int i = 0;

  if (IS_NPC(vict) || !vict->desc || !vict->desc->account)
  {
    send_to_char(ch, "The account command can only be used by player characters "
                     "with a valid account.\r\n");
    return;
  }

  struct account_data *acc = vict->desc->account;
  send_to_char(ch, "\tC");
  draw_line(ch, 80, '-', '-');
  send_to_char(ch,
               "\tcAccount Information for \tW%s\tc\r\n",
               acc->name);
  send_to_char(ch, "\tC");
  draw_line(ch, 80, '-', '-');
  send_to_char(ch,
               "\tcEmail: \tn%s\r\n"
               //    "Level: %d\r\n"
               "\tcExperience: \tn%d (notice: this caps at 100mil)\r\n"
               //    "Gift Experience: %d\r\n"
               //    "Web Site Password: %s\r\n"
               "\tcCharacters:\tn\r\n",
               (acc->email ? acc->email : "\trNot Set\tn"), acc->experience);

  for (i = 0; i < MAX_CHARS_PER_ACCOUNT; i++)
  {
    if (acc->character_names[i] != NULL)
      send_to_char(ch, "  \tn%s\r\n", acc->character_names[i]);
  }

  /* show unlocked races */
  send_to_char(ch, "Unlocked Races:\r\n");
  for (i = 0; i < MAX_UNLOCKED_RACES; i++)
  {
    if (acc->races[i] != 0)
    {
      send_to_char(ch, "  %s\r\n", race_list[acc->races[i]].type);
      found = TRUE;
    }
  }
  if (!found)
    send_to_char(ch, "  None.\r\n");

  /* show unlocked classes */
  found = FALSE;
  send_to_char(ch, "Unlocked Classes:\r\n");
  for (i = 0; i < MAX_UNLOCKED_CLASSES; i++)
  {
    if (acc->classes[i] != 0)
    {
      send_to_char(ch, "  %s\r\n", CLSLIST_NAME(acc->classes[i]));
      found = TRUE;
    }
  }
  if (!found)
    send_to_char(ch, "  None.\r\n");

  if (ch == vict)
    send_to_char(ch, "You can unlock races and classes via the 'accexp' command.\r\n");

  send_to_char(ch, "\tC");
  draw_line(ch, 80, '-', '-');
}

/*
  do_account (command)
  Purpose: Player command to show account information for themselves.
  Behavior:
    - Thin wrapper that calls perform_do_account(ch, ch).
*/
ACMD(do_account)
{
  perform_do_account(ch, ch);
}

/*
  remove_char_from_account(struct char_data *ch, struct account_data *account)
  Purpose: Detach a character from an account at the database level.
  Parameters:
    - ch: character to remove
    - account: account from which to remove the character
  Behavior:
    - DELETE row in player_data where lower(name)=lower(GET_NAME(ch)) and account_id matches.
    - Calls load_account_characters(account) to refresh character_names[].
    - Logs the action.
  Safety:
    - Checks for NULL ch/account and logs errors.
*/
void remove_char_from_account(struct char_data *ch, struct account_data *account)
{
  char buf[2048];

  if (ch == NULL)
  {
    log("SYSERR: Tried to remove a NULL char from account!");
    return;
  }
  else if (account == NULL)
  {
    log("SYSERR: Tried to remove a character from a NULL account!");
    return;
  }

  /* Escape character name to prevent SQL injection */
  char escaped_name[MAX_INPUT_LENGTH * 2 + 1];
  mysql_real_escape_string(conn, escaped_name, GET_NAME(ch), strlen(GET_NAME(ch)));
  
  snprintf(buf, sizeof(buf), "DELETE from player_data where lower(name) = lower('%s') and account_id = %d;",
           escaped_name, account->id);

  if (mysql_query(conn, buf))
  {
    log("SYSERR: Unable to DELETE from player_data: %s", mysql_error(conn));
    return;
  }

  /* Reload the character names */
  load_account_characters(account);

  log("Info: Character %s removed from account %s : %s", GET_NAME(ch), account->name, mysql_info(conn));
}
