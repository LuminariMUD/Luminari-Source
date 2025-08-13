/*/ \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \
\
/  Luminari Mail System
/  Created By: Gicker
\
/  using new_mail.h as the header file currently
\         todo:
/
\ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ /*/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "spells.h"
#include "constants.h"
#include "spec_procs.h"
#include "feats.h"
#include "oasis.h"
#include "house.h"
#include "dg_scripts.h"
#include "clan.h"
#include "mysql.h"
#include "modify.h"

void perform_mail_delete(struct char_data *ch, int mnum);
void perform_mail_list(struct char_data *ch, int type);
void perform_mail_read(struct char_data *ch, int mnum);

extern MYSQL *conn;
extern struct clan_type *clan_info;

void send_editor_help(struct descriptor_data *d);

ACMD(do_new_mail)
{

  char arg3[200], arg4[200];

  skip_spaces_c(&argument);

  if (!*argument)
  {
    send_to_char(ch, "Commands are:\r\n"
                     "-- mail list inbox            : (shows the mail in your inbox)\r\n"
                     "-- mail list sent             : (shows the mail in your outbox)\r\n"
                     "-- mail read <num>            : (read the specified mail)\r\n"
                     "-- mail days <num of days>    : (determines how many days back to list mail for)\r\n"
                     "-- mail delete <num>          : (deletes the specified mail for you only)\r\n"
                     "-- mail send <name> <subject> : (sends a mail to specified person)\r\n");
    return;
  }
  else
  {
    half_chop_c(argument, arg3, sizeof(arg3), arg4, sizeof(arg4));

    if (!*arg3)
    {
      send_to_char(ch, "Commands are:\r\n"
                       "-- mail list inbox            : (shows the mail in your inbox)\r\n"
                       "-- mail list sent             : (shows the mail in your outbox)\r\n"
                       "-- mail read <num>            : (read the specified mail)\r\n"
                       "-- mail days <num of days>    : (determines how many days back to list mail for)\r\n"
                       "-- mail delete <num>          : (deletes the specified mail for you only)\r\n"
                       "-- mail send <name> <subject> : (sends a mail to specified person)\r\n");
      return;
    }
    if (is_abbrev(arg3, "list"))
    {
      if (!*arg4 || is_abbrev(arg4, "inbox"))
      {
        perform_mail_list(ch, 1);
        return;
      }
      else if (is_abbrev(arg4, "sent"))
      {
        perform_mail_list(ch, 2);
        return;
      }
      else
      {
        send_to_char(ch, "Options are: mail list (inbox|sent)\r\n");
        return;
      }
    }
    else if (is_abbrev(arg3, "days"))
    {
      if (!*arg4)
      {
        send_to_char(ch, "How many days back would you like to be notified of new mail for and have mail displayed for?\r\n");
        return;
      }
      int days = atoi(arg4);
      if (days <= 0 || days >= 365)
      {
        send_to_char(ch, "You must choose an amount between 1 and 365 days.\r\n");
        return;
      }
      ch->player_specials->saved.mail_days = days;
      send_to_char(ch, "You now will receive notifications for and view listed email up to %d in the past.\r\n", days);
      return;
    }
    else if (is_abbrev(arg3, "read"))
    {
      if (!*arg4)
      {
        send_to_char(ch, "You must specify which mail idnum you wish to read.\r\n");
        return;
      }
      int mnum = atoi(arg4);
      if (mnum <= 0)
      {
        send_to_char(ch, "The mail idnum must be greater than zero.\r\n");
        return;
      }
      perform_mail_read(ch, mnum);
      return;
    }
    else if (is_abbrev(arg3, "delete"))
    {
      if (!*arg4)
      {
        send_to_char(ch, "You must specify which mail idnum you wish to delete.\r\n");
        return;
      }
      int mnum = atoi(arg4);
      if (mnum <= 0)
      {
        send_to_char(ch, "The mail idnum must be greater than zero.\r\n");
        return;
      }
      perform_mail_delete(ch, mnum);
      return;
    }
    else if (is_abbrev(arg3, "send"))
    {
      /* no worries about copyovers for now
            extern int circle_copyover;
            if (circle_copyover) {
              send_to_char(ch, "A copyover is scheduled.  Please wait until after to write your mail.\r\n");
              return;
            }
       */
      char arg5[200], arg6[200];
      half_chop(arg4, arg5, arg6);
      if (!*arg5)
      {
        send_to_char(ch, "You must specify a mail recipient.\r\n");
        return;
      }
      if (!*arg6)
      {
        send_to_char(ch, "You must specify a mail subject.\r\n");
        return;
      }

      MYSQL_RES *res = NULL;
      MYSQL_ROW row = NULL;
      int found = FALSE;

      /* Check the connection, reconnect if necessary. */
      if (!MYSQL_PING_CONN(conn)) {
        log("SYSERR: %s: Database connection failed", __func__);
        send_to_char(ch, "Database connection error. Mail cannot be sent at this time.\r\n");
        return;
      }

      // snprintf(arg5, sizeof(arg5), "%s", CAP(arg5));

      char query[MAX_INPUT_LENGTH] = {'\0'};
      char *end;
      end = stpcpy(query, "SELECT name FROM player_data WHERE name=");
      *end++ = '\'';
      end += mysql_real_escape_string(conn, end, arg5, strlen(arg5));
      *end++ = '\'';
      *end++ = '\0';
      mysql_query(conn, query);
      log("%s %s", arg5, arg6);
      log("%s", query);
      res = mysql_use_result(conn);
      if (res != NULL)
      {
        if ((row = mysql_fetch_row(res)) != NULL)
        {
          found = TRUE;
        }
      }
      mysql_free_result(res);

      //      send_to_char(ch, "%s\r\n", query);
      /*
            struct clan_type *cptr = NULL;

            for (cptr = clan_info; cptr; cptr = cptr->next) {

                if (cptr == NULL) {
                  continue;
                }

                if (!strcmp(cptr->member_look_str, arg5)) {
                  found = TRUE;
                  snprintf(arg5, sizeof(arg5), "%s", CAP(cptr->leadersname));
      //            send_to_char(ch, "%s\r\n", cptr->member_look_str);
                  break;
                }
            }
       */

      if (!found && (strcmp(arg5, "All") || GET_LEVEL(ch) < LVL_IMPL))
      {
        send_to_char(ch, "That character doesn't exist in our mail database.\r\n");
        return;
      }

      ch->player_specials->new_mail_receiver = strdup(arg5);
      ch->player_specials->new_mail_subject = strdup(arg6);
      if (ch->player_specials->new_mail_content)
      {
        ch->player_specials->new_mail_content = NULL;
      }

      send_editor_help(ch->desc);
      act("$n starts to write a mail.", TRUE, ch, 0, 0, TO_ROOM);
      SET_BIT_AR(PLR_FLAGS(ch), PLR_MAILING); /* string_write() sets writing. */
      ch->player_specials->new_mail_content = strdup(" ");
      string_write(ch->desc, &ch->player_specials->new_mail_content, MAX_STRING_LENGTH, -999, NULL);
      STATE(ch->desc) = CON_NEWMAIL;
      send_to_char(ch, "Please write your mail in the space below.  Type /s when you are done.\r\n\r\n");
      return;
    }
    else
    {
      send_to_char(ch, "Commands are:\r\n"
                       "-- mail list inbox            : (shows the mail in your inbox)\r\n"
                       "-- mail list sent             : (shows the mail in your outbox)\r\n"
                       "-- mail read <num>            : (read the specified mail)\r\n"
                       "-- mail delete <num>          : (deletes the specified mail for you only)\r\n"
                       "-- mail send <name> <subject> : (sends a mail to specified person)\r\n");
      return;
    }
  }
}

void perform_mail_list(struct char_data *ch, int type)
{
  MYSQL_RES *res = NULL;
  MYSQL_ROW row = NULL;
  MYSQL_RES *res2 = NULL;
  MYSQL_ROW row2 = NULL;
  extern MYSQL *conn2;
  MYSQL_RES *res3 = NULL;
  MYSQL_ROW row3 = NULL;
  extern MYSQL *conn3;

  /* Check the connection, reconnect if necessary. */
  if (!MYSQL_PING_CONN(conn)) {
    log("SYSERR: %s: Database connection failed (conn)", __func__);
    send_to_char(ch, "Database connection error. Cannot list mail at this time.\r\n");
    return;
  }
  if (!MYSQL_PING_CONN(conn2)) {
    log("SYSERR: %s: Database connection failed (conn2)", __func__);
    send_to_char(ch, "Database connection error. Cannot list mail at this time.\r\n");
    return;
  }
  if (!MYSQL_PING_CONN(conn3)) {
    log("SYSERR: %s: Database connection failed (conn3)", __func__);
    send_to_char(ch, "Database connection error. Cannot list mail at this time.\r\n");
    return;
  }

  sbyte unread = TRUE, deleted = FALSE;

  send_to_char(ch, "    %-7s %-20s %s\r\n", "MAIL ID", type != 1 ? "RECIPIENT" : "SENDER", "SUBJECT");
  send_to_char(ch, "    %-7s %-20s %s\r\n", "-------", "--------------------", "-----------------------------------");
  char query[MAX_INPUT_LENGTH] = {'\0'};
  char days[200];
  if (ch->player_specials->saved.mail_days <= 0)
  {
    ch->player_specials->saved.mail_days = 14;
  }
  if (ch->player_specials->saved.mail_days > 0)
  {
    snprintf(days, sizeof(days), " AND date_sent >= DATE_SUB(NOW(), INTERVAL %d DAY) ", ch->player_specials->saved.mail_days);
  }
  else
  {
    snprintf(days, sizeof(days), " ");
  }
  char *escaped_name = mysql_escape_string_alloc(conn, GET_NAME(ch));
  if (!escaped_name) {
    log("SYSERR: Failed to escape player name in mail_list");
    return;
  }
  snprintf(query, sizeof(query), "SELECT mail_id,sender,receiver,subject FROM player_mail WHERE (%s='%s' OR %s='All') %s ORDER BY mail_id DESC", type == 1 ? "receiver" : "sender", escaped_name, type == 1 ? "receiver" : "sender", days);
  free(escaped_name);
  mysql_query(conn, query);
  res = mysql_use_result(conn);
  if (res != NULL)
  {
    while ((row = mysql_fetch_row(res)) != NULL)
    {

      unread = TRUE;
      deleted = FALSE;

      char *escaped_name2 = mysql_escape_string_alloc(conn2, GET_NAME(ch));
      char *escaped_mailid = mysql_escape_string_alloc(conn2, row[0]);
      if (!escaped_name2 || !escaped_mailid) {
        log("SYSERR: Failed to escape strings in mail_list deleted check");
        if (escaped_name2) free(escaped_name2);
        if (escaped_mailid) free(escaped_mailid);
        continue;
      }
      snprintf(query, sizeof(query), "SELECT mail_id,player_name FROM player_mail_deleted WHERE player_name='%s' AND mail_id='%s'", escaped_name2, escaped_mailid);
      free(escaped_name2);
      free(escaped_mailid);
      mysql_query(conn2, query);
      res2 = mysql_use_result(conn2);
      if (res2 != NULL)
      {
        if ((row2 = mysql_fetch_row(res2)) != NULL)
        {
          deleted = TRUE;
        }
      }
      mysql_free_result(res2);

      char *escaped_name3 = mysql_escape_string_alloc(conn3, GET_NAME(ch));
      char *escaped_mailid2 = mysql_escape_string_alloc(conn3, row[0]);
      if (!escaped_name3 || !escaped_mailid2) {
        log("SYSERR: Failed to escape strings in mail_list read check");
        if (escaped_name3) free(escaped_name3);
        if (escaped_mailid2) free(escaped_mailid2);
        continue;
      }
      snprintf(query, sizeof(query), "SELECT mail_id,player_name FROM player_mail_read WHERE player_name='%s' AND mail_id='%s'", escaped_name3, escaped_mailid2);
      free(escaped_name3);
      free(escaped_mailid2);
      //          send_to_char(ch, "%s\r\n", query);
      mysql_query(conn3, query);
      res3 = mysql_use_result(conn3);
      if (res3 != NULL)
      {
        if ((row3 = mysql_fetch_row(res3)) != NULL)
        {
          unread = FALSE;
        }
      }
      mysql_free_result(res3);

      if (!deleted)
      {
        send_to_char(ch, "%-3s %-7s %-20s %s\r\n", unread ? "NEW" : "",
                     row[0], type == 1 ? row[1] : row[2], row[3]);
      }
    }
  }
  mysql_free_result(res);
}

void perform_mail_read(struct char_data *ch, int mnum)
{

  MYSQL_RES *res = NULL;
  MYSQL_ROW row = NULL;
  extern MYSQL *conn2;

  /* Check the connection, reconnect if necessary. */
  if (!MYSQL_PING_CONN(conn)) {
    log("SYSERR: %s: Database connection failed (conn)", __func__);
    send_to_char(ch, "Database connection error. Cannot read mail at this time.\r\n");
    return;
  }
  if (!MYSQL_PING_CONN(conn2)) {
    log("SYSERR: %s: Database connection failed (conn2)", __func__);
    send_to_char(ch, "Database connection error. Cannot read mail at this time.\r\n");
    return;
  }

  char mnums[20];

  snprintf(mnums, sizeof(mnums), "%d", mnum);

  byte found = FALSE;

  char query[MAX_INPUT_LENGTH] = {'\0'};
  char *escaped_name = mysql_escape_string_alloc(conn, GET_NAME(ch));
  if (!escaped_name) {
    log("SYSERR: Failed to escape player name in mail_read");
    return;
  }
  snprintf(query, sizeof(query), "SELECT mail_id,sender,receiver,subject FROM player_mail WHERE (sender='%s' OR receiver='%s' OR receiver='All') AND mail_id='%d'", escaped_name, escaped_name, mnum);
  free(escaped_name);
  mysql_query(conn, query);
  res = mysql_use_result(conn);
  if (res != NULL)
  {
    if ((row = mysql_fetch_row(res)) != NULL)
    {
      if (atoi(row[0]) > 0)
        found = TRUE;
    }
  }
  mysql_free_result(res);

  if (!found)
  {
    send_to_char(ch, "That mail is not accessible to you.\r\n");
    return;
  }
  found = FALSE;

  char *end;
  char buf[200];

  end = stpcpy(query, "SELECT mail_id,sender,receiver,subject,message FROM player_mail WHERE mail_id=");
  *end++ = '\'';
  end += mysql_real_escape_string(conn, end, mnums, strlen(mnums));
  *end++ = '\'';
  *end++ = '\0';
  mysql_query(conn, query);

  res = mysql_use_result(conn);
  if (res != NULL)
  {
    if ((row = mysql_fetch_row(res)) != NULL)
    {
      found = TRUE;
      send_to_char(ch, "Mail Id: %s Sender: %s Recipient: %s\r\n"
                       "Subject: %s\r\n"
                       "Message:\r\n"
                       "%s\r\n\r\n",
                   row[0], row[1], row[2], row[3], row[4]);
      char *escaped_name_del = mysql_escape_string_alloc(conn2, GET_NAME(ch));
      if (!escaped_name_del) {
        log("SYSERR: Failed to escape player name in mail_read delete");
        mysql_free_result(res);
        return;
      }
      snprintf(buf, sizeof(buf), "DELETE FROM player_mail_read WHERE player_name='%s' AND mail_id=", escaped_name_del);
      free(escaped_name_del);
      end = stpcpy(query, buf);
      *end++ = '\'';
      end += mysql_real_escape_string(conn2, end, mnums, strlen(mnums));
      *end++ = '\'';
      *end++ = '\0';
      mysql_query(conn2, query);

      char *escaped_name_ins = mysql_escape_string_alloc(conn2, GET_NAME(ch));
      if (!escaped_name_ins) {
        log("SYSERR: Failed to escape player name in mail_read insert");
        mysql_free_result(res);
        return;
      }
      snprintf(buf, sizeof(buf), "INSERT INTO player_mail_read (player_name, mail_id) VALUES('%s',", escaped_name_ins);
      free(escaped_name_ins);
      end = stpcpy(query, buf);
      *end++ = '\'';
      end += mysql_real_escape_string(conn2, end, mnums, strlen(mnums));
      *end++ = '\'';
      *end++ = ')';
      *end++ = '\0';
      mysql_query(conn2, query);
    }
  }
  mysql_free_result(res);

  if (!found)
  {
    send_to_char(ch, "There is no mail by that id number.\r\n");
  }
}

void perform_mail_delete(struct char_data *ch, int mnum)
{

  MYSQL_RES *res = NULL;
  MYSQL_ROW row = NULL;

  /* Check the connection, reconnect if necessary. */
  if (!MYSQL_PING_CONN(conn)) {
    log("SYSERR: %s: Database connection failed", __func__);
    send_to_char(ch, "Database connection error. Cannot delete mail at this time.\r\n");
    return;
  }

  char mnums[20];

  snprintf(mnums, sizeof(mnums), "%d", mnum);

  sbyte found = FALSE;

  char query[MAX_INPUT_LENGTH] = {'\0'};
  char *escaped_name = mysql_escape_string_alloc(conn, GET_NAME(ch));
  if (!escaped_name) {
    log("SYSERR: Failed to escape player name in mail_delete");
    return;
  }
  snprintf(query, sizeof(query), "SELECT mail_id,sender,receiver,subject FROM player_mail WHERE sender='%s' OR receiver='%s' OR (receiver='All')", escaped_name, escaped_name);
  free(escaped_name);
  mysql_query(conn, query);
  res = mysql_use_result(conn);
  if (res != NULL)
  {
    if ((row = mysql_fetch_row(res)) != NULL)
    {
      found = TRUE;
    }
  }
  mysql_free_result(res);

  if (!found)
  {
    send_to_char(ch, "That mail is not accessible to you.\r\n");
    return;
  }
  found = FALSE;

  char *end;
  char buf[200];
  char *escaped_name_del1 = mysql_escape_string_alloc(conn, GET_NAME(ch));
  if (!escaped_name_del1) {
    log("SYSERR: Failed to escape player name in mail_delete read");
    return;
  }
  snprintf(buf, sizeof(buf), "DELETE FROM player_mail_read WHERE player_name='%s' AND mail_id=", escaped_name_del1);
  free(escaped_name_del1);
  end = stpcpy(query, buf);
  *end++ = '\'';
  end += mysql_real_escape_string(conn, end, mnums, strlen(mnums));
  *end++ = '\'';
  *end++ = '\0';
  mysql_query(conn, query);

  char *escaped_name_del2 = mysql_escape_string_alloc(conn, GET_NAME(ch));
  if (!escaped_name_del2) {
    log("SYSERR: Failed to escape player name in mail_delete deleted");
    return;
  }
  snprintf(buf, sizeof(buf), "DELETE FROM player_mail_deleted WHERE player_name='%s' AND mail_id=", escaped_name_del2);
  free(escaped_name_del2);
  end = stpcpy(query, buf);
  *end++ = '\'';
  end += mysql_real_escape_string(conn, end, mnums, strlen(mnums));
  *end++ = '\'';
  *end++ = '\0';
  mysql_query(conn, query);

  char *escaped_name_ins2 = mysql_escape_string_alloc(conn, GET_NAME(ch));
  if (!escaped_name_ins2) {
    log("SYSERR: Failed to escape player name in mail_delete insert");
    return;
  }
  snprintf(buf, sizeof(buf), "INSERT INTO player_mail_deleted (player_name, mail_id) VALUES('%s',", escaped_name_ins2);
  free(escaped_name_ins2);
  end = stpcpy(query, buf);
  *end++ = '\'';
  end += mysql_real_escape_string(conn, end, mnums, strlen(mnums));
  *end++ = '\'';
  *end++ = ')';
  *end++ = '\0';
  mysql_query(conn, query);

  send_to_char(ch, "You have successfully deleted that mail.\r\n");
}

/* adjusted to return number of NEW mail and added 'silent' mode -zusuk */
int new_mail_alert(struct char_data *ch, bool silent)
{
  MYSQL_RES *res = NULL;
  MYSQL_ROW row = NULL;
  MYSQL_RES *res2 = NULL;
  MYSQL_ROW row2 = NULL;
  extern MYSQL *conn2;
  MYSQL_RES *res3 = NULL;
  MYSQL_ROW row3 = NULL;
  extern MYSQL *conn3;

  /* Check the connection, reconnect if necessary. */
  if (!MYSQL_PING_CONN(conn)) {
    log("SYSERR: %s: Database connection failed (conn)", __func__);
    return 0;
  }
  if (!MYSQL_PING_CONN(conn2)) {
    log("SYSERR: %s: Database connection failed (conn2)", __func__);
    return 0;
  }
  if (!MYSQL_PING_CONN(conn3)) {
    log("SYSERR: %s: Database connection failed (conn3)", __func__);
    return 0;
  }

  int num_unread = 0, num_mails = 0, num_read = 0, num_deleted = 0;
  ;

  char days[200];
  if (ch->player_specials->saved.mail_days <= 0)
  {
    ch->player_specials->saved.mail_days = 14;
  }
  if (ch->player_specials->saved.mail_days > 0)
  {
    snprintf(days, sizeof(days), " AND date_sent >= DATE_SUB(NOW(), INTERVAL %d DAY) ", ch->player_specials->saved.mail_days);
  }
  else
  {
    snprintf(days, sizeof(days), " ");
  }

  char query[MAX_INPUT_LENGTH] = {'\0'};
  char *escaped_name = mysql_escape_string_alloc(conn, GET_NAME(ch));
  if (!escaped_name) {
    log("SYSERR: Failed to escape player name in new_mail_alert");
    return 0;
  }
  snprintf(query, sizeof(query), "SELECT mail_id,sender,receiver,subject FROM player_mail WHERE (receiver='%s' OR %s='All') %s ORDER BY mail_id DESC", escaped_name, "receiver", days);
  free(escaped_name);
  mysql_query(conn, query);
  res = mysql_use_result(conn);
  if (res != NULL)
  {
    while ((row = mysql_fetch_row(res)) != NULL)
    {

      num_mails++;

      char *escaped_name2 = mysql_escape_string_alloc(conn2, GET_NAME(ch));
      char *escaped_mailid = mysql_escape_string_alloc(conn2, row[0]);
      if (!escaped_name2 || !escaped_mailid) {
        log("SYSERR: Failed to escape strings in mail_list deleted check");
        if (escaped_name2) free(escaped_name2);
        if (escaped_mailid) free(escaped_mailid);
        continue;
      }
      snprintf(query, sizeof(query), "SELECT mail_id,player_name FROM player_mail_deleted WHERE player_name='%s' AND mail_id='%s'", escaped_name2, escaped_mailid);
      free(escaped_name2);
      free(escaped_mailid);
      mysql_query(conn2, query);
      res2 = mysql_use_result(conn2);
      if (res2 != NULL)
      {
        if ((row2 = mysql_fetch_row(res2)) != NULL)
        {
          num_deleted++;
        }
      }
      mysql_free_result(res2);

      char *escaped_name3 = mysql_escape_string_alloc(conn3, GET_NAME(ch));
      char *escaped_mailid2 = mysql_escape_string_alloc(conn3, row[0]);
      if (!escaped_name3 || !escaped_mailid2) {
        log("SYSERR: Failed to escape strings in mail_list read check");
        if (escaped_name3) free(escaped_name3);
        if (escaped_mailid2) free(escaped_mailid2);
        continue;
      }
      snprintf(query, sizeof(query), "SELECT mail_id,player_name FROM player_mail_read WHERE player_name='%s' AND mail_id='%s'", escaped_name3, escaped_mailid2);
      free(escaped_name3);
      free(escaped_mailid2);
      mysql_query(conn3, query);
      res3 = mysql_use_result(conn3);
      if (res3 != NULL)
      {
        if ((row3 = mysql_fetch_row(res3)) != NULL)
        {
          num_read++;
        }
      }
      mysql_free_result(res3);
    }
  }
  mysql_free_result(res);

  num_unread = num_mails - num_read - num_deleted;

  if (!silent && num_unread > 0)
  {
    send_to_char(ch, "\r\nYou have %d NEW mail messages!\r\n\r\n", num_unread);
  }

  return num_unread;
}
