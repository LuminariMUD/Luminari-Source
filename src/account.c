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
    - SQL injection protection: every data value is bound through prepared statements.

  This file adds explanatory comments without changing behavior.
*/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "mysql.h"
#include "utils.h"
#include "db.h"
#include "handler.h"
#include "character/feats.h"
#include "dgscript/dg_scripts.h"
#include "comm.h"
#include "interpreter.h"
#include "olc/genmob.h"
#include "constants.h"
#include "magic/spells.h"
#include "screen.h"
#include "character/class.h"
#include "character/race.h"
#include "act.h"
#include "account.h"
#include "vessels/routing.h"
#include "perfmon.h"

extern MYSQL *conn;

/* Forward reference: helper loaders for attached structures on an account */
void load_account_characters(struct account_data *account);
void load_account_unlocks(struct account_data *account);
static void account_persistence_mark_clean(struct account_data *account);

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
  Notes:
    - The loop searches up to MAX_UNLOCKED_RACES for an exact match.
*/
int has_unlocked_race(struct char_data *ch, int race)
{
  /* Lich and vampire races are always locked out here. */
  if (!ch || !ch->desc || !ch->desc->account || race == RACE_LICH || race == RACE_VAMPIRE)
    return FALSE;

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
  if (is_abbrev(arg, "class") || is_abbrev(arg, "race"))
  {
    /* Skip spaces and copy rest of line */
    skip_spaces_c(&remainder);
    strlcpy(arg2, remainder, sizeof(arg2));
  }
  else
  {
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
      send_to_char(ch,
                   "Please choose 'good' for good alignment change or 'evil' for evil alignment "
                   "change.  It cost %d account exp for each.\r\n",
                   cost);
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
      send_to_char(ch,
                   "Please choose 'good' for good alignment change or 'evil' for evil alignment "
                   "change.  It cost %d account exp for each.\r\n",
                   cost);
      return;
    }

    if (ch->desc && ch->desc->account)
    {
      /* Hard bounds check on resulting alignment to prevent exceeding +-1000 */
      if ((GET_ALIGNMENT(ch) + align_change) > 1000 || (GET_ALIGNMENT(ch) + align_change) < -1000)
      {
        send_to_char(ch, "You have the maximum alignment already!\r\n");
        return;
      }
      else if (GET_ACCEXP_DESC(ch) >= cost)
      {
        change_account_xp(ch, -cost);
        send_to_char(ch,
                     "You have changed your alignment by %d points, costing %d account points!\r\n",
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
        send_to_char(
            ch, "You need %d account experience to purchase that change and you only have %d.\r\n",
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

    start = 0;
    end = NUM_EXTENDED_RACES;

    /* No argument: list lockable races that are not yet unlocked */
    if (!*arg2)
    {
      send_to_char(ch, "Please choose from the following races:\r\n");

      for (i = start; i < end; i++)
      {
        if (!race_is_creation_eligible(i) || !is_locked_race(i) || has_unlocked_race(ch, i))
          continue;

        int cost = locked_race_cost(i);
        send_to_char(ch, "%s (%d account experience)\r\n", race_list[i].type, cost);
      }
    }

    /* Identify the intended race to unlock by name abbreviation */
    for (i = start; i < end; i++)
    {
      if (race_is_creation_eligible(i) && is_abbrev(arg2, race_list[i].type) && is_locked_race(i) &&
          !has_unlocked_race(ch, i))
      {
        cost = locked_race_cost(i);
        break;
      }
    }

    if (i >= end)
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
        send_to_char(ch,
                     "You have unlocked the advanced race '%s' for all character "
                     "and future characters on your account!.\r\n",
                     race_list[i].type);
        change_account_xp(ch, -cost); /* this will call save_account() for us */
        return;
      }
      else
      {
        send_to_char(ch,
                     "You need %d account experience to purchase that advanced "
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
        if (!CLSLIST_INGAME(i) || has_unlocked_class(ch, i) || !CLSLIST_LOCK(i))
          continue;
        cost = CLSLIST_COST(i);
        send_to_char(ch, "%s (%d account experience)\r\n", CLSLIST_NAME(i), cost);
      }
      return;
    }
    /* Identify class to unlock by name abbreviation */
    for (i = 0; i < NUM_CLASSES; i++)
    {
      /* Skip disabled, already unlocked, and non-lockable classes */
      if (!CLSLIST_INGAME(i) || has_unlocked_class(ch, i) || !CLSLIST_LOCK(i))
        continue;

      /* Check if this class matches the input */
      if (is_abbrev(arg2, CLSLIST_NAME(i)))
      {
        cost = CLSLIST_COST(i);
        break;
      }

      /* For knight classes, also check alternate names */
      if (i == CLASS_KNIGHT_OF_SOLAMNIA ||
          (i >= CLASS_KNIGHT_OF_THE_THORN && i <= CLASS_KNIGHT_OF_THE_LILY))
      {
        bool matches_alternate = FALSE;
        switch (i)
        {
        case CLASS_KNIGHT_OF_SOLAMNIA:
          matches_alternate = is_abbrev(arg2, "knight of solamnia");
          break;
        case CLASS_KNIGHT_OF_THE_LILY:
          matches_alternate = is_abbrev(arg2, "knight of the lily");
          break;
        case CLASS_KNIGHT_OF_THE_THORN:
          matches_alternate = is_abbrev(arg2, "knight of the thorn");
          break;
        case CLASS_KNIGHT_OF_THE_SKULL:
          matches_alternate = is_abbrev(arg2, "knight of the skull");
          break;
        }

        if (matches_alternate)
        {
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
        send_to_char(ch,
                     "You have unlocked the prestige class '%s' for all "
                     "character and future characters on your account!.\r\n",
                     CLSLIST_NAME(i));
        change_account_xp(ch, -cost); /* this will call save_account() for us */
        return;
      }
      else
      {
        send_to_char(ch,
                     "You need %d account experience to purchase that prestige class and you only "
                     "have %d.\r\n",
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
  account_prepare_statement(const char *query)
  Purpose: Create and prepare a bound statement on the primary connection.
  Return:
    - Prepared statement ready for parameter binding, or NULL after logging.
  Notes:
    - Every account query binds its data values; SQL text here is constant.
    - The caller owns the result and must call mysql_stmt_cleanup().
*/
static PREPARED_STMT *account_prepare_statement(const char *query)
{
  PREPARED_STMT *statement;

  statement = mysql_stmt_create(conn);
  if (statement == NULL)
    return NULL;
  if (!mysql_stmt_prepare_query(statement, query))
  {
    mysql_stmt_cleanup(statement);
    return NULL;
  }
  return statement;
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
    - Copies the stored password hash with truncation-safe termination.
    - Calls load_account_characters() and load_account_unlocks() to populate arrays.
  Notes:
    - Uses mysql_ping(conn) before querying to ensure connection is alive.
*/
int load_account(char *name, struct account_data *account)
{
  PREPARED_STMT *statement;
  const char *value;

  /* Check if MySQL is available */
  if (!mysql_available || !conn)
  {
    return -1; /* Account not found - no MySQL */
  }

  /* Check if the account has data, if so, clear it. */
  if (account != NULL)
  {
    int i;
    if (account->name != NULL)
    {
      free(account->name);
      account->name = NULL;
    }
    if (account->email != NULL)
    {
      free(account->email);
      account->email = NULL;
    }
    for (i = 0; i < MAX_CHARS_PER_ACCOUNT; i++)
    {
      if (account->character_names[i] != NULL)
      {
        free(account->character_names[i]);
        account->character_names[i] = NULL;
      }
    }
  }
  /* Check the connection, reconnect if necessary. */
  if (!MYSQL_PING_CONN(conn))
  {
    log("SYSERR: %s: Database connection failed", __func__);
    return -1;
  }

  /* Case-insensitive match on the bound account name */
  statement = account_prepare_statement(
      "SELECT id, name, password, experience, email, quit_survey_completed "
      "FROM account_data WHERE lower(name) = lower(?)");
  if (statement == NULL || !mysql_stmt_bind_param_string(statement, 0, name) ||
      !mysql_stmt_execute_prepared(statement))
  {
    log("SYSERR: Unable to SELECT from account_data for account lookup.");
    mysql_stmt_cleanup(statement);
    return -1;
  }

  if (!mysql_stmt_fetch_row(statement))
  {
    mysql_stmt_cleanup(statement);
    return -1; /* Account not found. */
  }

  account->id = mysql_stmt_get_int(statement, 0);
  value = mysql_stmt_get_string(statement, 1);
  account->name = strdup(value != NULL ? value : name);
  value = mysql_stmt_get_string(statement, 2);
  strlcpy(account->password, value != NULL ? value : "", sizeof(account->password));
  account->experience = mysql_stmt_get_int(statement, 3);
  value = mysql_stmt_get_string(statement, 4);
  account->email = value != NULL ? strdup(value) : NULL;
  account->quit_survey_completed = mysql_stmt_get_int(statement, 5);

  mysql_stmt_cleanup(statement);
  load_account_characters(account);
  load_account_unlocks(account);
  account_persistence_mark_clean(account);

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
  PREPARED_STMT *duplicates;
  PREPARED_STMT *removal;
  const char *name;
  int count;

  if (!account || account->id <= 0)
    return;

  /* Get list of duplicate character names for this account */
  duplicates = account_prepare_statement("SELECT name, COUNT(*) AS cnt FROM player_data "
                                         "WHERE account_id = ? GROUP BY name HAVING cnt > 1");
  if (duplicates == NULL || !mysql_stmt_bind_param_int(duplicates, 0, account->id) ||
      !mysql_stmt_execute_prepared(duplicates))
  {
    log("SYSERR: Unable to check for duplicate characters for account %d.", account->id);
    mysql_stmt_cleanup(duplicates);
    return;
  }

  /* The duplicate list is buffered client-side, so removals may run inside the loop. */
  while (mysql_stmt_fetch_row(duplicates))
  {
    name = mysql_stmt_get_string(duplicates, 0);
    count = (int)mysql_stmt_get_long(duplicates, 1);
    if (name == NULL || count <= 1)
      continue;

    /* Delete all but one; ORDER BY keeps a consistent survivor row. */
    removal = account_prepare_statement("DELETE FROM player_data WHERE account_id = ? "
                                        "AND lower(name) = lower(?) ORDER BY name LIMIT ?");
    if (removal == NULL || !mysql_stmt_bind_param_int(removal, 0, account->id) ||
        !mysql_stmt_bind_param_string(removal, 1, name) ||
        !mysql_stmt_bind_param_int(removal, 2, count - 1) || !mysql_stmt_execute_prepared(removal))
    {
      log("SYSERR: Unable to delete duplicate character %s.", name);
    }
    else
    {
      log("Info: Cleaned up %ld duplicate(s) of character %s for account %s",
          (long)mysql_stmt_affected_rows_count(removal), name, account->name);
    }
    mysql_stmt_cleanup(removal);
  }

  mysql_stmt_cleanup(duplicates);
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
  PREPARED_STMT *statement;
  const char *name;
  long long total = 0;
  long long unique_count = 0;
  int i = 0;

  /* Free existing names to avoid leaks on reload */
  for (i = 0; i < MAX_CHARS_PER_ACCOUNT; i++)
    if (account->character_names[i] != NULL)
    {
      free(account->character_names[i]);
      account->character_names[i] = NULL;
    }

  /* First check if we need to clean up duplicates */
  statement = account_prepare_statement(
      "SELECT COUNT(*), COUNT(DISTINCT name) FROM player_data WHERE account_id = ?");
  if (statement == NULL || !mysql_stmt_bind_param_int(statement, 0, account->id) ||
      !mysql_stmt_execute_prepared(statement))
  {
    log("SYSERR: Unable to check for duplicate characters for account %d.", account->id);
  }
  else if (mysql_stmt_fetch_row(statement))
  {
    total = mysql_stmt_get_long(statement, 0);
    unique_count = mysql_stmt_get_long(statement, 1);
  }
  mysql_stmt_cleanup(statement);
  if (total > unique_count)
  {
    log("Info: Detected %lld duplicate character entries for account %s, cleaning up...",
        total - unique_count, account->name);
    cleanup_duplicate_characters(account);
  }

  /* Now load the character names (duplicates have been cleaned) */
  statement = account_prepare_statement("SELECT name FROM player_data WHERE account_id = ?");
  if (statement == NULL || !mysql_stmt_bind_param_int(statement, 0, account->id) ||
      !mysql_stmt_execute_prepared(statement))
  {
    log("SYSERR: Unable to SELECT character names for account %d.", account->id);
    mysql_stmt_cleanup(statement);
    return;
  }

  i = 0;
  while (i < MAX_CHARS_PER_ACCOUNT && mysql_stmt_fetch_row(statement))
  {
    name = mysql_stmt_get_string(statement, 0);
    if (name == NULL)
      continue;
    account->character_names[i] = strdup(name);
    i++;
  }

  mysql_stmt_cleanup(statement);
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
  PREPARED_STMT *statement;
  int i = 0;

  /* load unlocked classes */
  statement =
      account_prepare_statement("SELECT class_id FROM unlocked_classes WHERE account_id = ?");
  if (statement == NULL || !mysql_stmt_bind_param_int(statement, 0, account->id) ||
      !mysql_stmt_execute_prepared(statement))
  {
    log("SYSERR: Unable to SELECT from unlocked_classes for account %d.", account->id);
    mysql_stmt_cleanup(statement);
    return;
  }

  i = 0;
  while (i < MAX_UNLOCKED_CLASSES && mysql_stmt_fetch_row(statement))
  {
    account->classes[i] = mysql_stmt_get_int(statement, 0);
    i++;
  }
  mysql_stmt_cleanup(statement);

  /* load unlocked races */
  statement = account_prepare_statement("SELECT race_id FROM unlocked_races WHERE account_id = ?");
  if (statement == NULL || !mysql_stmt_bind_param_int(statement, 0, account->id) ||
      !mysql_stmt_execute_prepared(statement))
  {
    log("SYSERR: Unable to SELECT from unlocked_races for account %d.", account->id);
    mysql_stmt_cleanup(statement);
    return;
  }

  i = 0;
  while (i < MAX_UNLOCKED_RACES && mysql_stmt_fetch_row(statement))
  {
    account->races[i] = mysql_stmt_get_int(statement, 0);
    i++;
  }
  mysql_stmt_cleanup(statement);
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
  PREPARED_STMT *statement;
  const char *value;
  char *acct_name = NULL;

  statement = account_prepare_statement("SELECT a.name FROM account_data a, player_data p "
                                        "WHERE p.account_id = a.id AND p.name = ?");
  if (statement == NULL || !mysql_stmt_bind_param_string(statement, 0, name) ||
      !mysql_stmt_execute_prepared(statement))
  {
    log("SYSERR: Unable to retrieve account name for character %s.", name);
    mysql_stmt_cleanup(statement);
    return NULL;
  }
  while (mysql_stmt_fetch_row(statement))
  {
    if (acct_name)
      free(acct_name); /* Free previous allocation if multiple rows */
    value = mysql_stmt_get_string(statement, 0);
    acct_name = value != NULL ? strdup(value) : NULL;
  }
  mysql_stmt_cleanup(statement);
  return acct_name;
}

enum account_persistence_component
{
  ACCOUNT_PERSIST_CORE = 0,
  ACCOUNT_PERSIST_CHARACTERS,
  ACCOUNT_PERSIST_RACES,
  ACCOUNT_PERSIST_CLASSES,
  ACCOUNT_PERSIST_COMPONENT_COUNT
};

static uint64_t account_hash_bytes(uint64_t hash, const void *data, size_t size)
{
  const unsigned char *bytes;
  size_t i;

  bytes = data;
  for (i = 0; i < size; i++)
  {
    hash ^= bytes[i];
    hash *= UINT64_C(1099511628211);
  }
  return hash;
}

static uint64_t account_hash_string(uint64_t hash, const char *text)
{
  if (text == NULL)
    return account_hash_bytes(hash, "", 1);
  return account_hash_bytes(hash, text, strlen(text) + 1);
}

static void account_component_hashes(const struct account_data *account,
                                     uint64_t hashes[ACCOUNT_PERSIST_COMPONENT_COUNT])
{
  uint64_t hash;
  int i;

  hash = UINT64_C(1469598103934665603);
  hash = account_hash_string(hash, account->name);
  hash = account_hash_string(hash, account->password);
  hash = account_hash_bytes(hash, &account->experience, sizeof(account->experience));
  hash = account_hash_string(hash, account->email);
  hash = account_hash_bytes(hash, &account->quit_survey_completed,
                            sizeof(account->quit_survey_completed));
  hashes[ACCOUNT_PERSIST_CORE] = hash;

  hash = UINT64_C(1469598103934665603);
  for (i = 0; i < MAX_CHARS_PER_ACCOUNT; i++)
    hash = account_hash_string(hash, account->character_names[i]);
  hashes[ACCOUNT_PERSIST_CHARACTERS] = hash;

  hashes[ACCOUNT_PERSIST_RACES] =
      account_hash_bytes(UINT64_C(1469598103934665603), account->races, sizeof(account->races));
  hashes[ACCOUNT_PERSIST_CLASSES] =
      account_hash_bytes(UINT64_C(1469598103934665603), account->classes, sizeof(account->classes));
}

static void account_persistence_mark_clean(struct account_data *account)
{
  uint64_t hashes[ACCOUNT_PERSIST_COMPONENT_COUNT];
  int i;

  if (account == NULL)
    return;
  account_component_hashes(account, hashes);
  for (i = 0; i < ACCOUNT_PERSIST_COMPONENT_COUNT; i++)
  {
    account->persistence_hash[i] = hashes[i];
    account->persistence_dirty_generation[i] = 0;
    account->persistence_saved_generation[i] = 0;
  }
  account->persistence_hash_initialized = true;
}

static void account_persistence_detect_dirty(struct account_data *account,
                                             uint64_t hashes[ACCOUNT_PERSIST_COMPONENT_COUNT])
{
  int i;

  account_component_hashes(account, hashes);
  for (i = 0; i < ACCOUNT_PERSIST_COMPONENT_COUNT; i++)
  {
    if (!account->persistence_hash_initialized || account->persistence_hash[i] != hashes[i])
    {
      if (account->persistence_dirty_generation[i] == account->persistence_saved_generation[i])
        account->persistence_dirty_generation[i]++;
    }
  }
}

static bool save_account_character_links(struct account_data *account, char *query,
                                         size_t query_size)
{
  PREPARED_STMT *statement;
  bool bound;
  int used;
  int i;
  int count;

  count = 0;
  while (count < MAX_CHARS_PER_ACCOUNT && account->character_names[count] != NULL)
    count++;
  if (count == 0)
    return true;

  /* Only placeholders are appended here; every name is bound below. */
  used =
      snprintf(query, query_size, "UPDATE player_data SET account_id = ? WHERE lower(name) IN (");
  for (i = 0; i < count; i++)
  {
    used = snprintf_append(query, query_size, used, "%slower(?)", i > 0 ? "," : "");
    if ((size_t)used >= query_size - 1)
      return false;
  }
  used = snprintf_append(query, query_size, used, ")");
  if ((size_t)used >= query_size - 1)
    return false;

  statement = account_prepare_statement(query);
  bound = statement != NULL && mysql_stmt_bind_param_int(statement, 0, account->id);
  for (i = 0; bound && i < count; i++)
    bound = mysql_stmt_bind_param_string(statement, i + 1, account->character_names[i]);
  if (!bound || !mysql_stmt_execute_prepared(statement))
  {
    log("SYSERR: Unable to batch account character links for account %d.", account->id);
    mysql_stmt_cleanup(statement);
    return false;
  }
  mysql_stmt_cleanup(statement);
  return true;
}

/* Unlock tables addressable by save_account_integer_set(). Identifiers never come from data. */
enum account_unlock_set
{
  ACCOUNT_UNLOCK_RACES,
  ACCOUNT_UNLOCK_CLASSES
};

static const struct
{
  const char *table;
  const char *delete_sql;
  const char *insert_prefix;
  const char *insert_suffix;
} account_unlock_sql[] = {
    {"unlocked_races", "DELETE FROM unlocked_races WHERE account_id = ?",
     "INSERT INTO unlocked_races (account_id, race_id) VALUES ",
     " ON DUPLICATE KEY UPDATE race_id=VALUES(race_id)"},
    {"unlocked_classes", "DELETE FROM unlocked_classes WHERE account_id = ?",
     "INSERT INTO unlocked_classes (account_id, class_id) VALUES ",
     " ON DUPLICATE KEY UPDATE class_id=VALUES(class_id)"},
};

static bool save_account_integer_set(int account_id, enum account_unlock_set set, const int *values,
                                     int value_count, char *query, size_t query_size)
{
  PREPARED_STMT *statement;
  bool bound;
  int count;
  int i;
  int used;

  statement = account_prepare_statement(account_unlock_sql[set].delete_sql);
  if (statement == NULL || !mysql_stmt_bind_param_int(statement, 0, account_id) ||
      !mysql_stmt_execute_prepared(statement))
  {
    log("SYSERR: Unable to replace %s for account %d.", account_unlock_sql[set].table, account_id);
    mysql_stmt_cleanup(statement);
    return false;
  }
  mysql_stmt_cleanup(statement);

  /* Only placeholders are appended here; every value pair is bound below. */
  used = snprintf(query, query_size, "%s", account_unlock_sql[set].insert_prefix);
  count = 0;
  for (i = 0; i < value_count; i++)
  {
    if (values[i] == 0)
      continue;
    used = snprintf_append(query, query_size, used, "%s(?,?)", count > 0 ? "," : "");
    if ((size_t)used >= query_size - 1)
      return false;
    count++;
  }
  if (count == 0)
    return true;
  used = snprintf_append(query, query_size, used, "%s", account_unlock_sql[set].insert_suffix);
  if ((size_t)used >= query_size - 1)
    return false;

  statement = account_prepare_statement(query);
  bound = statement != NULL;
  count = 0;
  for (i = 0; bound && i < value_count; i++)
  {
    if (values[i] == 0)
      continue;
    bound = mysql_stmt_bind_param_int(statement, count * 2, account_id) &&
            mysql_stmt_bind_param_int(statement, count * 2 + 1, values[i]);
    count++;
  }
  if (!bound || !mysql_stmt_execute_prepared(statement))
  {
    log("SYSERR: Unable to batch %s for account %d.", account_unlock_sql[set].table, account_id);
    mysql_stmt_cleanup(statement);
    return false;
  }
  mysql_stmt_cleanup(statement);
  return true;
}

static void synchronize_connected_account_views(const struct account_data *source)
{
  struct descriptor_data *descriptor;
  struct account_data *target;
  char *character_name_copy;
  int i;

  for (descriptor = descriptor_list; descriptor != NULL; descriptor = descriptor->next)
  {
    target = descriptor->account;
    if (target == NULL || target->id != source->id)
      continue;
    if (target != source)
    {
      target->experience = source->experience;
      target->quit_survey_completed = source->quit_survey_completed;
      for (i = 0; i < MAX_CHARS_PER_ACCOUNT; i++)
      {
        character_name_copy =
            source->character_names[i] != NULL ? strdup(source->character_names[i]) : NULL;
        free(target->character_names[i]);
        target->character_names[i] = character_name_copy;
      }
      memcpy(target->races, source->races, sizeof(target->races));
      memcpy(target->classes, source->classes, sizeof(target->classes));
      memcpy(target->persistence_hash, source->persistence_hash, sizeof(target->persistence_hash));
      memcpy(target->persistence_dirty_generation, source->persistence_dirty_generation,
             sizeof(target->persistence_dirty_generation));
      memcpy(target->persistence_saved_generation, source->persistence_saved_generation,
             sizeof(target->persistence_saved_generation));
      target->persistence_hash_initialized = source->persistence_hash_initialized;
      snprintf(target->password, sizeof(target->password), "%s", source->password);
      free(target->email);
      target->email = source->email != NULL ? strdup(source->email) : NULL;
    }
    if (IS_PLAYING(descriptor))
      GET_ACCEXP_DESC(descriptor->character) = source->experience;
  }
}

/*
 * Persist account state in one transaction. Character membership and unlock
 * sets use bounded batch statements, so query volume follows changed data
 * rather than the fixed capacities of the in-memory arrays.
 */
bool save_account_checked(struct account_data *account)
{
  char query[16384];
  PREPARED_STMT *statement;
  enum perf_sql_category previous_sql_category;
  bool success;
  bool transaction_started;
  bool core_dirty;
  bool characters_dirty;
  bool races_dirty;
  bool classes_dirty;
  uint64_t hashes[ACCOUNT_PERSIST_COMPONENT_COUNT];
  int i;

  if (account == NULL || account->name == NULL)
  {
    log("SYSERR: Attempted to save an incomplete account.");
    return false;
  }

  PERF_PROF_ENTER_SAMPLED(pr_save_account_, "save.account");
  previous_sql_category = PERF_sql_scope_set(PERF_SQL_ACCOUNT);
  success = false;
  transaction_started = false;
  account_persistence_detect_dirty(account, hashes);
  core_dirty = account->persistence_dirty_generation[ACCOUNT_PERSIST_CORE] !=
               account->persistence_saved_generation[ACCOUNT_PERSIST_CORE];
  characters_dirty = account->persistence_dirty_generation[ACCOUNT_PERSIST_CHARACTERS] !=
                     account->persistence_saved_generation[ACCOUNT_PERSIST_CHARACTERS];
  races_dirty = account->persistence_dirty_generation[ACCOUNT_PERSIST_RACES] !=
                account->persistence_saved_generation[ACCOUNT_PERSIST_RACES];
  classes_dirty = account->persistence_dirty_generation[ACCOUNT_PERSIST_CLASSES] !=
                  account->persistence_saved_generation[ACCOUNT_PERSIST_CLASSES];
  if (!core_dirty && !characters_dirty && !races_dirty && !classes_dirty)
  {
    success = true;
    goto cleanup;
  }

  if (mysql_query(conn, "START TRANSACTION"))
  {
    log("SYSERR: Unable to start account save transaction: %s", mysql_error(conn));
    goto cleanup;
  }
  transaction_started = true;

  if (core_dirty)
  {
    /* A NULL email binds as SQL NULL; nothing here is interpolated into the text. */
    statement = account_prepare_statement(
        "INSERT INTO account_data (id,name,password,experience,email,quit_survey_completed) "
        "VALUES (?,?,?,?,?,?) "
        "ON DUPLICATE KEY UPDATE password=VALUES(password),experience=VALUES(experience),"
        "email=VALUES(email),quit_survey_completed=VALUES(quit_survey_completed)");
    if (statement == NULL || !mysql_stmt_bind_param_int(statement, 0, account->id) ||
        !mysql_stmt_bind_param_string(statement, 1, account->name) ||
        !mysql_stmt_bind_param_string(statement, 2, account->password) ||
        !mysql_stmt_bind_param_int(statement, 3, account->experience) ||
        !mysql_stmt_bind_param_string(statement, 4, account->email) ||
        !mysql_stmt_bind_param_int(statement, 5, account->quit_survey_completed ? 1 : 0) ||
        !mysql_stmt_execute_prepared(statement))
    {
      log("SYSERR: Unable to UPSERT account_data for account '%s'.", account->name);
      mysql_stmt_cleanup(statement);
      goto rollback;
    }
    if (account->id == 0)
      account->id = (int)mysql_stmt_insert_id(statement->stmt);
    mysql_stmt_cleanup(statement);
  }

  if ((characters_dirty && !save_account_character_links(account, query, sizeof(query))) ||
      (races_dirty && !save_account_integer_set(account->id, ACCOUNT_UNLOCK_RACES, account->races,
                                                MAX_UNLOCKED_RACES, query, sizeof(query))) ||
      (classes_dirty &&
       !save_account_integer_set(account->id, ACCOUNT_UNLOCK_CLASSES, account->classes,
                                 MAX_UNLOCKED_CLASSES, query, sizeof(query))))
    goto rollback;

  if (mysql_query(conn, "COMMIT"))
  {
    log("SYSERR: Unable to commit account save transaction: %s", mysql_error(conn));
    goto rollback;
  }
  transaction_started = false;
  success = true;
  for (i = 0; i < ACCOUNT_PERSIST_COMPONENT_COUNT; i++)
  {
    account->persistence_hash[i] = hashes[i];
    account->persistence_saved_generation[i] = account->persistence_dirty_generation[i];
  }
  account->persistence_hash_initialized = true;
  synchronize_connected_account_views(account);
  goto cleanup;

rollback:
  if (mysql_query(conn, "ROLLBACK"))
    log("SYSERR: Unable to roll back account save transaction: %s", mysql_error(conn));
  transaction_started = false;

cleanup:
  if (transaction_started && mysql_query(conn, "ROLLBACK"))
    log("SYSERR: Unable to clean up account save transaction: %s", mysql_error(conn));
  if (!success)
    log("SYSERR: Account '%s' was not durably saved; the current state remains retryable.",
        account->name);
  PERF_sql_scope_restore(previous_sql_category);
  PERF_PROF_EXIT(pr_save_account_);
  return success;
}

void save_account(struct account_data *account)
{
  (void)save_account_checked(account);
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
  write_to_output(
      d, "  \tc#  \tC| \tcName                \tC| \tcLvl \tC| \tcRace \tC| \tcClass\tn \r\n");
  write_to_output(d, "\tC%s\tn", text_line_string("", 80, '-', '-'));

  /*  Check the connection, reconnect if necessary. */
  if (!MYSQL_PING_CONN(conn))
  {
    log("SYSERR: save_account: Database connection failed");
    return;
  }

  PREPARED_STMT *statement = NULL;
  bool executed = false;
  bool found = false;

  if (d->account)
  {
    for (i = 0; i < MAX_CHARS_PER_ACCOUNT; i++)
    {
      /* Initialize a place for the player data to temporarily reside. */
      CREATE(xtch, struct char_data, 1);
      clear_char(xtch);
      CREATE(xtch->player_specials, struct player_special_data, 1);
      new_mobile_data(xtch);

      if (d->account->character_names[i] != NULL &&
          load_char(d->account->character_names[i], xtch) > -1)
      {
        /* Character loaded successfully, we're done with xtch */
        free_char(xtch);
        xtch = NULL;

        write_to_output(d, " \tW%-3d\tn \tC|\tn \tW%-20s\tn\tC|\tn", i + 1,
                        d->account->character_names[i]);
        statement =
            account_prepare_statement("SELECT name FROM player_data WHERE lower(name) = lower(?)");
        executed = statement != NULL &&
                   mysql_stmt_bind_param_string(statement, 0, d->account->character_names[i]) &&
                   mysql_stmt_execute_prepared(statement);
        found = executed && mysql_stmt_fetch_row(statement);
        mysql_stmt_cleanup(statement);
        statement = NULL;
        if (!executed)
        {
          log("SYSERR: Unable to SELECT from player_data for the account menu.");
        }

        if (executed)
        {
          if (found)
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
                continue;
              }

              /* Level and race abbreviation with color formatting */
              write_to_output(d, " %3d \tC|\tn %4s \tC|\tn", GET_LEVEL(tch),
                              race_list[GET_REAL_RACE(tch)].abbrev_color);

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
                      len = snprintf_append(buf, sizeof(buf), len, "/");
                    len = snprintf_append(buf, sizeof(buf), len, "%s", CLSLIST_CLRABBRV(inc));
                    classCount++;
                  }
                }
                write_to_output(d, " %-36s", buf);
              }
            }
            free_char(tch);
          }
          write_to_output(d, "\r\n");
        }
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
  write_to_output(d,
                  "\tcType the # of a character listed above or choose one of the following:\r\n");
  write_to_output(d, "\tC%s\tn", text_line_string("", 80, '-', '-'));
  write_to_output(d, " \tn(\tCC\tn)\tcreate a new character \tC| \tn(\tCA\tn)\tcdd a character     "
                     " \tC| \tn(\tCQ\tn)\tcuit\tn\r\n");
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
  send_to_char(ch, "\tcAccount Information for \tW%s\tc\r\n", acc->name);
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
  PREPARED_STMT *statement;
  long removed;

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

  statement = account_prepare_statement(
      "DELETE FROM player_data WHERE lower(name) = lower(?) AND account_id = ?");
  if (statement == NULL || !mysql_stmt_bind_param_string(statement, 0, GET_NAME(ch)) ||
      !mysql_stmt_bind_param_int(statement, 1, account->id) ||
      !mysql_stmt_execute_prepared(statement))
  {
    log("SYSERR: Unable to DELETE %s from player_data.", GET_NAME(ch));
    mysql_stmt_cleanup(statement);
    return;
  }
  removed = (long)mysql_stmt_affected_rows_count(statement);
  mysql_stmt_cleanup(statement);

  /* Reload the character names */
  load_account_characters(account);

  log("Info: Character %s removed from account %s : %ld row(s) affected", GET_NAME(ch),
      account->name, removed);
}

bool link_character_to_account_checked(struct char_data *ch, struct account_data *account)
{
  PREPARED_STMT *statement;

  if (ch == NULL || account == NULL || !mysql_available || conn == NULL || GET_NAME(ch) == NULL)
    return FALSE;

  statement = account_prepare_statement("INSERT INTO player_data (name, account_id, last_online) "
                                        "VALUES (?, ?, NOW()) "
                                        "ON DUPLICATE KEY UPDATE account_id = VALUES(account_id)");
  if (statement == NULL || !mysql_stmt_bind_param_string(statement, 0, GET_NAME(ch)) ||
      !mysql_stmt_bind_param_int(statement, 1, account->id) ||
      !mysql_stmt_execute_prepared(statement))
  {
    log("SYSERR: Unable to link new character %s to account %s.", GET_NAME(ch), account->name);
    mysql_stmt_cleanup(statement);
    return FALSE;
  }
  mysql_stmt_cleanup(statement);
  return TRUE;
}

bool begin_account_character_removal(struct char_data *ch, struct account_data *account)
{
  PREPARED_STMT *statement;

  if (ch == NULL || account == NULL || !mysql_available || conn == NULL || GET_NAME(ch) == NULL)
    return FALSE;

  if (mysql_query(conn, "START TRANSACTION"))
  {
    log("SYSERR: Could not begin character-removal transaction: %s", mysql_error(conn));
    return FALSE;
  }

  statement = account_prepare_statement(
      "DELETE FROM player_data WHERE lower(name) = lower(?) AND account_id = ?");
  if (statement == NULL || !mysql_stmt_bind_param_string(statement, 0, GET_NAME(ch)) ||
      !mysql_stmt_bind_param_int(statement, 1, account->id) ||
      !mysql_stmt_execute_prepared(statement))
  {
    log("SYSERR: Could not stage account unlink for character %s.", GET_NAME(ch));
    mysql_stmt_cleanup(statement);
    mysql_query(conn, "ROLLBACK");
    return FALSE;
  }
  mysql_stmt_cleanup(statement);

  return TRUE;
}

void rollback_account_character_removal(void)
{
  if (mysql_available && conn != NULL && mysql_query(conn, "ROLLBACK"))
    log("SYSERR: Could not roll back character-removal transaction: %s", mysql_error(conn));
}

bool commit_account_character_removal(struct account_data *account)
{
  struct descriptor_data *descriptor = NULL;

  if (account == NULL || !mysql_available || conn == NULL)
    return FALSE;

  if (mysql_query(conn, "COMMIT"))
  {
    log("SYSERR: Could not commit character-removal transaction: %s", mysql_error(conn));
    rollback_account_character_removal();
    return FALSE;
  }

  /*
   * Refresh every connected view of this account. Leaving a stale name in a
   * second descriptor could cause a later account save to recreate the link.
   */
  for (descriptor = descriptor_list; descriptor != NULL; descriptor = descriptor->next)
  {
    if (descriptor->account != NULL && descriptor->account->id == account->id)
      load_account_characters(descriptor->account);
  }

  return TRUE;
}
