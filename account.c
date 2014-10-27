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

extern MYSQL *conn;

/* Forward reference */
void load_account_characters(struct account_data *account);

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

  sprintf(buf, "SELECT id, name, password, experience, email from account_data where lower(name) = lower('%s')",
               name);

  if (mysql_query(conn, buf)) {
    log("SYSERR: Unable to SELECT from account_data: %s", mysql_error(conn));
    return -1;
  }

  if (!(result = mysql_store_result(conn))) {
    log("SYSERR: Unable to SELECT from account_data: %s", mysql_error(conn));
    return -1;
  }

  if(!(row = mysql_fetch_row(result))) 
    return -1; /* Account not found. */
    
  account->id         = atoi(row[0]);
  account->name       = strdup(row[1]);
  strncpy(account->password, row[2], MAX_PWD_LENGTH + 1);
  account->experience = atoi(row[3]);
  account->email      = (row[4] ? strdup(row[4]) : NULL);

  mysql_free_result(result);
  load_account_characters(account);
  return (0);
}

void load_account_characters(struct account_data *account) {
  MYSQL_RES *result;
  MYSQL_ROW row;
  char buf[2048];
  int i = 0;

  for (i = 0; i < MAX_CHARS_PER_ACCOUNT; i++)
    if (account->character_names[i] != NULL) {
      free(account->character_names[i]);
      account->character_names[i] = NULL;
    }

  sprintf(buf, "select name from player_data where account_id = %d", account->id);

  if (mysql_query(conn, buf)) {
    log("SYSERR: Unable to SELECT from player_data: %s", mysql_error(conn));
    return;
  }
  if (!(result = mysql_store_result(conn))) {
    log("SYSERR: Unable to SELECT from player_data: %s", mysql_error(conn));
    return;
  }

  i = 0;
  while((row = mysql_fetch_row(result))) {   
    account->character_names[i] = strdup(row[0]);
    i++;
  }

  mysql_free_result(result);                                                      
  return;
}

char *get_char_account_name(char *name) {
  MYSQL_RES *result;
  MYSQL_ROW row;
  char buf[2048];
  char *acct_name = NULL;

  sprintf(buf, "select a.name from account_data a, player_data p where p.account_id = a.id and p.name = '%s'", name);

  if (mysql_query(conn, buf)) {
    log("SYSERR: Unable to retrieve account name for character %s: %s", name, mysql_error(conn));
    return NULL;
  }  
  if (!(result = mysql_store_result(conn))) {
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

  if (account == NULL) {
    log("SYSERR: Attempted to save NULL account.");
    return;
  }
  
  sprintf(buf, "INSERT into account_data (id, name, password, experience, email) values (%d, '%s', '%s', %d, %s%s%s)"
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

  if (mysql_query(conn, buf)) {
    log("SYSERR: Unable to UPSERT into account_data: %s", mysql_error(conn));
    return;
  }

  if (account->id == 0) /* This is a new account! */
    account->id = mysql_insert_id(conn);

  for (i = 0; (i < MAX_CHARS_PER_ACCOUNT) && (account->character_names[i] != NULL);i++) {
    buf[0] = '\0';
    sprintf(buf, "INSERT into player_data (name, account_id) "
                 "VALUES('%s', %d) "
                 "on duplicate key update account_id = VALUES(account_id);",
                 account->character_names[i], account->id);
    if (mysql_query(conn, buf)) {
      log("SYSERR: Unable to UPSERT player_data: %s", mysql_error(conn));
    return;
    }   
  }   
}

void show_account_menu(struct descriptor_data *d)
{
  int i = 0;
  struct char_data *tch = NULL;
  char class_list[MAX_INPUT_LENGTH];
  

//  write_to_output(d, "Account Menu\r\n");
  write_to_output(d, "\tC%s\tn", text_line_string("", 80, '-', '-'));
  write_to_output(d, "  \tc#  \tC| \tcName                \tC| \tcLvl \tC| \tcRace \tC| \tcClass\tn \r\n");
  write_to_output(d, "\tC%s\tn", text_line_string("", 80, '-', '-'));

  /*  Check the connection, reconnect if necessary. */
  mysql_ping(conn);


  MYSQL_RES *res = NULL;
  MYSQL_ROW row = NULL;

  char query[MAX_INPUT_LENGTH];

  if (d->account) {
    for (i = 0; i < MAX_CHARS_PER_ACCOUNT; i++) {
      if (d->account->character_names[i] != NULL) {
        write_to_output(d, " \tW%-3d\tn \tC|\tn \tW%-20s\tn\tC|\tn", i + 1, d->account->character_names[i]);
        sprintf(query, "SELECT name FROM player_data WHERE lower(name)=lower('%s')", d->account->character_names[i]);

        if (mysql_query(conn, query)) {
          log("SYSERR: Unable to SELECT from player_data: %s", mysql_error(conn));
        }

        if (!(res = mysql_store_result(conn))) {
          log("SYSERR: Unable to SELECT from player_data: %s", mysql_error(conn));
        }

        if (res != NULL) {
          if ((row = mysql_fetch_row(res)) != NULL) {
            /* Initialize a place for the player data to temporarially reside. */
            CREATE(tch, struct char_data, 1);
            clear_char(tch);
            CREATE(tch->player_specials, struct player_special_data, 1);
            new_mobile_data(tch);

            if ((load_char(d->account->character_names[i], tch)) > -1) {
              /* Player found! */
              if (PLR_FLAGGED(tch, PLR_DELETED)) {
                write_to_output(d, " \tR---===||DELETED||===---\tn\r\n");
                return;
              }
              
              write_to_output(d, " %3d \tC|\tn %4s \tC|\tn", GET_LEVEL(tch), RACE_ABBR(tch));
              
              if (GET_LEVEL(tch) >= LVL_IMMORT) {
                /* Staff */
                write_to_output(d, " %-36s", admin_level_names[(GET_LEVEL(tch) - LVL_IMMORT)]);
              } else {
                /* Mortal */
              
                int inc, classCount = 0;
                class_list[0] = '\0';
                for (inc = 0; inc < MAX_CLASSES; inc++) {
                  if (CLASS_LEVEL(tch, inc)) {
                    if (classCount)
                      strcat(class_list, "/");
                    strcat(class_list, class_abbrevs[inc]);
                    classCount++;
                  }
                }
                write_to_output(d, " %-36s", class_list);
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

ACMD(do_account)
{

  if (IS_NPC(ch) || !ch->desc || !ch->desc->account) {
    send_to_char(ch, "The account command can only be used by player characters with a valid account.\r\n");
    return;
  }

  struct account_data *acc = ch->desc->account;
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
    "\tcExperience: \tn%d\r\n"
//    "Gift Experience: %d\r\n"
//    "Web Site Password: %s\r\n"
    "\tcCharacters:\tn\r\n",
    (acc->email ? acc->email : "\trNot Set\tn"), acc->experience);

  int i = 0;
  for (i = 0; i < MAX_CHARS_PER_ACCOUNT; i++) {
    if (acc->character_names[i] != NULL)
      send_to_char(ch, "  \tn%s\r\n", acc->character_names[i]);
  }

/*  
  send_to_char(ch, "Unlocked Advanced Races:\r\n");
  for (i = 0; i < MAX_UNLOCKED_RACES; i++) {
    if (acc->races[i] > 0 && race_list[acc->races[i]].is_pc) {
      send_to_char(ch, "  %s\r\n", race_list[acc->races[i]].name);
      found = TRUE;
    }
  }  
  if (!found)
    send_to_char(ch, "  None.\r\n");

  found = FALSE;

  send_to_char(ch, "Unlocked Prestige Classes:\r\n");
  for (i = 0; i < MAX_UNLOCKED_CLASSES; i++) {
    if (acc->classes[i] < 999) {
      send_to_char(ch, "  %s\r\n", class_names_dl_aol[acc->classes[i]]);
      found = TRUE;
    }
  }  
  if (!found)
    send_to_char(ch, "  None.\r\n");
*/
  send_to_char(ch, "\tC");
  draw_line(ch, 80, '-', '-');
}


/* Remove the player from the database, so that accounts do not reference it. */
void remove_char_from_account(struct char_data *ch, struct account_data *account) {
  char buf[2048];

  if (ch == NULL) {
    log("SYSERR: Tried to remove a NULL char from account!");
    return;
  } else if (account == NULL) {
    log("SYSERR: Tried to remove a character from a NULL account!");
    return;
  }
  
  sprintf(buf, "DELETE from player_data where lower(name) = lower('%s') and account_id = %d;",
               GET_NAME(ch), account->id);

  if (mysql_query(conn, buf)) {
    log("SYSERR: Unable to DELETE from player_data: %s", mysql_error(conn));
    return;
  }

  /* Reload the character names */  
  load_account_characters(account);

  log("INFO: Character %s removed from account %s : %s", GET_NAME(ch), account->name, mysql_info(conn));
}
