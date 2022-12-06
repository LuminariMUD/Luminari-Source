/*/ \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \
\
/  Luminari Account System, Inspired by D20mud's Account System
/  Created By: Ornir
\
/  using act.h as the header file currently
\         todo: move header stuff into account.h
/
\ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ /*/

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

extern MYSQL *conn;

/* Forward reference */
void load_account_characters(struct account_data *account);
void load_account_unlocks(struct account_data *account);

#define Y TRUE
#define N FALSE

/* start functions! */

int locked_race_cost(int race)
{
  return (race_list[race].unlock_cost);
}

bool is_locked_race(int race)
{
  if (race_list[race].unlock_cost > 0)
    return TRUE;

  return FALSE;
}

int change_account_xp(struct char_data *ch, int change_val)
{
  GET_ACCEXP_DESC(ch) += change_val;

  if (GET_ACCEXP_DESC(ch) < 0)
    GET_ACCEXP_DESC(ch) = 0;

  if (GET_ACCEXP_DESC(ch) > 100000000)
    GET_ACCEXP_DESC(ch) = 100000000;

  save_account(ch->desc->account);

  return GET_ACCEXP_DESC(ch);
}

int has_unlocked_race(struct char_data *ch, int race)
{
#ifdef CAMPAIGN_FR
  if (!ch || !ch->desc || !ch->desc->account)
#else
  if (!ch || !ch->desc || !ch->desc->account || race == RACE_LICH || race == RACE_VAMPIRE)
#endif
    return FALSE;

  if (!is_locked_race(race))
    return TRUE;

  int i = 0;

  for (i = 0; i < MAX_UNLOCKED_RACES; i++)
    if (ch->desc->account->races[i] == race)
      return TRUE;

  return FALSE;
}

int has_unlocked_class(struct char_data *ch, int class)
{
  if (!ch || !ch->desc || !ch->desc->account)
    return FALSE;

  if (!CLSLIST_LOCK(class))
    return TRUE;

  int i = 0;

  for (i = 0; i < MAX_UNLOCKED_CLASSES; i++)
    if (ch->desc->account->classes[i] == class)
      return TRUE;

  return FALSE;
}

#define ALIGN_COST 2000
ACMD(do_accexp)
{
  char arg[MAX_INPUT_LENGTH] = {'\0'}, arg2[MAX_INPUT_LENGTH] = {'\0'};
  int i = 0, j = 0;
  int cost = 0;
  int align_change = 100;

  two_arguments(argument, arg, sizeof(arg), arg2, sizeof(arg2));

  if (!*arg)
  {
    send_to_char(ch, "Usage: accexp [class | race | align] [<class-name to unlock> | "
                     "<race-name to unlock> | <evil OR good>]\r\n");
    return;
  }

  if (is_abbrev(arg, "align"))
  {

    cost = ALIGN_COST;

    if (!*arg2)
    {
      send_to_char(ch, "Please choose 'good' for good alignment change or 'evil' for evil alignment change.  It costs %d account exp for each.\r\n", cost);
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
      send_to_char(ch, "Please choose 'good' for good alignment change or 'evil' for evil alignment change.  It costs %d account exp for each.\r\n", cost);
      return;
    }

    if (ch->desc && ch->desc->account)
    {
      if ((GET_ALIGNMENT(ch) + align_change) > 1099 || (GET_ALIGNMENT(ch) + align_change) < -1099)
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
    if (!*arg2)
    {
      send_to_char(ch, "Please choose from the following races:\r\n");
#ifdef CAMPAIGN_FR
      for (i = 0; i < NUM_EXTENDED_PC_RACES; i++)
#else
      for (i = 0; i < NUM_RACES; i++)
#endif
      {
        if (!is_locked_race(i) || has_unlocked_race(ch, i))
          continue;

        cost = locked_race_cost(i);
        send_to_char(ch, "%s (%d account experience)\r\n", race_list[i].type, cost);
      }
      return;
    }
#ifdef CAMPAIGN_FR
    for (i = 0; i < NUM_EXTENDED_PC_RACES; i++)
#else
    for (i = 0; i < NUM_RACES; i++)
#endif
    {
      if (race_list[i].is_pc && is_abbrev(arg2, race_list[i].type) && is_locked_race(i) && !has_unlocked_race(ch, i))
      {
        cost = locked_race_cost(i);
        break;
      }
    }
#ifdef CAMPAIGN_FR
    if (i >= NUM_EXTENDED_PC_RACES)
#else
    if (i >= NUM_RACES)
#endif
      {
        send_to_char(ch, "Either that race does not exist, is not an advanced race, "
                         "is not available for players, or you've already unlocked it.\r\n");
        return;
      }
    if (ch->desc && ch->desc->account)
    {
      for (j = 0; j < MAX_UNLOCKED_RACES; j++)
      {
        if (ch->desc->account->races[j] == 0) /* race 0 is human, never locked */
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
        send_to_char(ch, "You have unlocked the advanced race '%s' for all characters "
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
    if (!*arg2)
    {
      send_to_char(ch, "Please choose from the following classes:\r\n");
      for (i = 0; i < NUM_CLASSES; i++)
      {
        if (has_unlocked_class(ch, i) || !CLSLIST_LOCK(i))
          continue;
        cost = CLSLIST_COST(i);
        send_to_char(ch, "%s (%d account experience)\r\n", CLSLIST_NAME(i), cost);
      }
      return;
    }
    for (i = 0; i < NUM_CLASSES; i++)
    {
      if (is_abbrev(arg2, CLSLIST_NAME(i)) && !has_unlocked_class(ch, i) &&
          CLSLIST_LOCK(i))
      {
        cost = CLSLIST_COST(i);
        break;
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
      for (j = 0; j < MAX_UNLOCKED_CLASSES; j++)
      {
        if (ch->desc->account->classes[j] == 0) /* class 0 = wizard, never locked */
          break;
      }
      if (j >= MAX_UNLOCKED_CLASSES)
      {
        send_to_char(ch, "All of your prestige class slots are filled.  Please "
                         "ask the staff for the limit to be increased.\r\n");
        return;
      }
      if (GET_ACCEXP_DESC(ch) >= cost)
      {
        ch->desc->account->classes[j] = i;
        send_to_char(ch, "You have unlocked the prestige class '%s' for all "
                         "characters and future characters on your account!.\r\n",
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

int load_account(char *name, struct account_data *account)
{
  MYSQL_RES *result;
  MYSQL_ROW row;
  char buf[2048];

  /* Check if the account has data, if so, clear it. */
  /*   if (account != NULL) {
      if (account->name != NULL)
        free(account->name);
      if (account->email != NULL)
        free(account->email);
      for (i = 0; i < MAX_CHARS_PER_ACCOUNT; i++)
        if (account->character_names[i] != NULL)
          free(account->character_names[i]);
    }
   */
  /* Check the connection, reconnect if necessary. */
  mysql_ping(conn);

  snprintf(buf, sizeof(buf), "SELECT id, name, password, experience, email from account_data where lower(name) = lower('%s')",
           name);

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
    return -1; /* Account not found. */

  account->id = atoi(row[0]);
  account->name = strdup(row[1]);
  strncpy(account->password, row[2], MAX_PWD_LENGTH + 1);
  account->experience = atoi(row[3]);
  account->email = (row[4] ? strdup(row[4]) : NULL);

  mysql_free_result(result);
  load_account_characters(account);
  load_account_unlocks(account);

  return (0);
}

void load_account_characters(struct account_data *account)
{
  MYSQL_RES *result;
  MYSQL_ROW row;
  char buf[2048];
  int i = 0;

  for (i = 0; i < MAX_CHARS_PER_ACCOUNT; i++)
    if (account->character_names[i] != NULL)
    {
      free(account->character_names[i]);
      account->character_names[i] = NULL;
    }

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
  while ((row = mysql_fetch_row(result)))
  {
    account->character_names[i] = strdup(row[0]);
    i++;
  }

  if (result)
    mysql_free_result(result);
  return;
}

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
  while ((row = mysql_fetch_row(result)))
  {
    account->classes[i] = atoi(row[0]);
    i++;
  }

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
  while ((row = mysql_fetch_row(result)))
  {
    account->races[i] = atoi(row[0]);
    i++;
  }

  /* cleanup */
  mysql_free_result(result);
  return;
}

char *get_char_account_name(char *name)
{
  MYSQL_RES *result;
  MYSQL_ROW row;
  char buf[2048];
  char *acct_name = NULL;

  snprintf(buf, sizeof(buf), "select a.name from account_data a, player_data p where p.account_id = a.id and p.name = '%s'", name);

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
    acct_name = (row[0] ? strdup(row[0]) : NULL);
  mysql_free_result(result);
  return acct_name;
}

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

  for (i = 0; (i < MAX_CHARS_PER_ACCOUNT) && (account->character_names[i] != NULL); i++)
  {
    buf[0] = '\0';
    snprintf(buf, sizeof(buf), "INSERT into player_data (name, account_id) "
                               "VALUES('%s', %d) "
                               "on duplicate key update account_id = VALUES(account_id);",
             account->character_names[i], account->id);
    if (mysql_query(conn, buf))
    {
      log("SYSERR: Unable to UPSERT player_data: %s", mysql_error(conn));
      return;
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
     We need to update all characters in game with this account id */
  for (j = descriptor_list; j; j = next_desc)
  {
    next_desc = j->next;

    if (j->account)
    {
      if (j->account->id == account->id)
      {
        load_account_unlocks(j->account);
        if (IS_PLAYING(j))
          GET_ACCEXP_DESC(j->character) = account->experience;
      }
    }
  }
}

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
  mysql_ping(conn);

  MYSQL_RES *res = NULL;
  MYSQL_ROW row = NULL;

  char query[MAX_INPUT_LENGTH] = {'\0'};

  if (d->account)
  {
    for (i = 0; i < MAX_CHARS_PER_ACCOUNT; i++)
    {
      /* trying: Initialize a place for the player data to temporarially reside. -zusuk */
      CREATE(xtch, struct char_data, 1);
      clear_char(xtch);
      CREATE(xtch->player_specials, struct player_special_data, 1);
      new_mobile_data(xtch);

      if (d->account->character_names[i] != NULL && load_char(d->account->character_names[i], xtch) > -1)
      {
        free_char(xtch);

        write_to_output(d, " \tW%-3d\tn \tC|\tn \tW%-20s\tn\tC|\tn", i + 1, d->account->character_names[i]);
        snprintf(query, sizeof(query), "SELECT name FROM player_data WHERE lower(name)=lower('%s')", d->account->character_names[i]);

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
            /* Initialize a place for the player data to temporarially reside. */
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
                return;
              }

              write_to_output(d, " %3d \tC|\tn %4s \tC|\tn", GET_LEVEL(tch), race_list[GET_REAL_RACE(tch)].abbrev_color);

              if (GET_LEVEL(tch) >= LVL_IMMORT)
              {
                /* Staff */
                write_to_output(d, " %-36s", admin_level_names[(GET_LEVEL(tch) - LVL_IMMORT)]);
              }
              else
              {
                /* Mortal */

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

/* engine for ACMDU(do_account) */
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

ACMD(do_account)
{
  perform_do_account(ch, ch);
}

/* Remove the player from the database, so that accounts do not reference it. */
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

  snprintf(buf, sizeof(buf), "DELETE from player_data where lower(name) = lower('%s') and account_id = %d;",
           GET_NAME(ch), account->id);

  if (mysql_query(conn, buf))
  {
    log("SYSERR: Unable to DELETE from player_data: %s", mysql_error(conn));
    return;
  }

  /* Reload the character names */
  load_account_characters(account);

  log("INFO: Character %s removed from account %s : %s", GET_NAME(ch), account->name, mysql_info(conn));
}
