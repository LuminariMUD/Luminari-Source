/* 
 * This file is part of Luminari MUD
 * 
 * File: help.c
 * Author: Ornir (Jamie McLaughlin)
 */
#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "modify.h"
#include "comm.h"
#include "interpreter.h"
#include "db.h"
#include "mysql.h"
#include "lists.h"
#include "help.h"
#include "feats.h"
#include "spells.h" /* need this for class.h NUM_ABILITIES */
#include "class.h"
#include "race.h"
#include "alchemy.h"

/* puts -'s instead of spaces */
void space_to_minus(char *str)
{
  while ((str = strchr(str, ' ')) != NULL)
    *str = '-';
}

/* Name: search_help
 * Author: Ornir (Jamie McLaughlin)
 * 
 * Original function hevaily modified (rewritten!) to use the help database
 * instead of the in-memory help structure.  
 * The consumer of the return value is responsible for freeing the memory!
 * YOU HAVE BEEN WARNED.  */
struct help_entry_list *search_help(const char *argument, int level)
{

  MYSQL_RES *result;
  MYSQL_ROW row;

  struct help_entry_list *help_entries = NULL, *new_help_entry = NULL, *cur = NULL;

  char buf[1024], escaped_arg[MAX_STRING_LENGTH];

  /*  Check the connection, reconnect if necessary. */
  mysql_ping(conn);

  mysql_real_escape_string(conn, escaped_arg, argument, strlen(argument));

  snprintf(buf, sizeof(buf), "SELECT distinct he.tag, he.entry, he.min_level, he.last_updated, group_concat(distinct CONCAT(UCASE(LEFT(hk2.keyword, 1)), LCASE(SUBSTRING(hk2.keyword, 2))) separator ', ')"
               " FROM `help_entries` he, `help_keywords` hk, `help_keywords` hk2"
               " WHERE he.tag = hk.help_tag and hk.help_tag = hk2.help_tag and lower(hk.keyword) like '%s%%' and he.min_level <= %d"
               " group by hk.help_tag ORDER BY length(hk.keyword) asc",
          argument, level);

  if (mysql_query(conn, buf))
  {
    log("SYSERR: Unable to SELECT from help_entries: %s", mysql_error(conn));
    return NULL;
  }

  if (!(result = mysql_store_result(conn)))
  {
    log("SYSERR: Unable to SELECT from help_entries: %s", mysql_error(conn));
    return NULL;
  }

  while ((row = mysql_fetch_row(result)))
  {

    /* Allocate memory for the help entry data. */
    CREATE(new_help_entry, struct help_entry_list, 1);
    new_help_entry->tag = strdup(row[0]);
    new_help_entry->entry = strdup(row[1]);
    new_help_entry->min_level = atoi(row[2]);
    new_help_entry->last_updated = strdup(row[3]);
    new_help_entry->keywords = strdup(row[4]);

    new_help_entry->keyword_list = get_help_keywords(new_help_entry->tag);

    if (help_entries == NULL)
    {
      help_entries = new_help_entry;
      cur = new_help_entry;
    }
    else
    {
      cur->next = new_help_entry;
      cur = new_help_entry;
    }
    new_help_entry = NULL;
  }

  mysql_free_result(result);

  return help_entries;
}

struct help_keyword_list *get_help_keywords(const char *tag)
{
  MYSQL_RES *result;
  MYSQL_ROW row;

  struct help_keyword_list *keywords = NULL, *new_keyword = NULL, *cur = NULL;

  char buf[1024];

  /* Get keywords for this entry. */
  snprintf(buf, sizeof(buf), "select help_tag, CONCAT(UCASE(LEFT(keyword, 1)), LCASE(SUBSTRING(keyword, 2))) from help_keywords where help_tag = '%s'", tag);
  if (mysql_query(conn, buf))
  {
    log("SYSERR: Unable to SELECT from help_keywords: %s", mysql_error(conn));
    return NULL;
  }
  if (!(result = mysql_store_result(conn)))
  {
    log("SYSERR: Unable to SELECT from help_keywords: %s", mysql_error(conn));
    return NULL;
  }
  while ((row = mysql_fetch_row(result)))
  {
    CREATE(new_keyword, struct help_keyword_list, 1);
    new_keyword->tag = strdup(row[0]);
    new_keyword->keyword = strdup(row[1]);
    new_keyword->next = NULL;

    if (keywords == NULL)
    {
      keywords = new_keyword;
      cur = new_keyword;
    }
    else
    {
      cur->next = new_keyword;
      cur = new_keyword;
    }
  }

  mysql_free_result(result);
  return keywords;
}

struct help_keyword_list *soundex_search_help_keywords(const char *argument, int level)
{

  MYSQL_RES *result;
  MYSQL_ROW row;

  struct help_keyword_list *keywords = NULL, *new_keyword = NULL, *cur = NULL;

  char buf[1024], escaped_arg[MAX_STRING_LENGTH];

  /*   Check the connection, reconnect if necessary. */
  mysql_ping(conn);

  mysql_real_escape_string(conn, escaped_arg, argument, strlen(argument));

  snprintf(buf, sizeof(buf), "SELECT hk.help_tag, "
               "       hk.keyword "
               "FROM help_entries he, "
               "     help_keywords hk "
               "WHERE he.tag = hk.help_tag "
               "  and hk.keyword sounds like '%s' "
               "  and he.min_level <= %d "
               "ORDER BY length(hk.keyword) asc",
          argument, level);

  if (mysql_query(conn, buf))
  {
    log("SYSERR: Unable to SELECT from help_keywords: %s", mysql_error(conn));
    return NULL;
  }

  if (!(result = mysql_store_result(conn)))
  {
    log("SYSERR: Unable to SELECT from help_keywords: %s", mysql_error(conn));
    return NULL;
  }

  while ((row = mysql_fetch_row(result)))
  {

    /* Allocate memory for the help entry data. */
    CREATE(new_keyword, struct help_keyword_list, 1);
    new_keyword->tag = strdup(row[0]);
    new_keyword->keyword = strdup(row[1]);

    if (keywords == NULL)
    {
      keywords = new_keyword;
      cur = new_keyword;
    }
    else
    {
      cur->next = new_keyword;
      cur = new_keyword;
    }
    new_keyword = NULL;
  }

  mysql_free_result(result);

  return keywords;
}

/* this is used for char creation help */

/* make sure arg doesn't have spaces */
void perform_help(struct descriptor_data *d, const char *argument)
{
  struct help_entry_list *entry = NULL, *tmp = NULL;

  if (!*argument)
    return;

  if ((entry = search_help(argument, LVL_IMPL)) == NULL)
    return;

  /* Disable paging for character creation. */
  if ((STATE(d) == CON_QRACE) ||
      (STATE(d) == CON_QCLASS))
    send_to_char(d->character, entry->entry);
  else
    page_string(d, entry->entry, 1);

  while (entry != NULL)
  {
    tmp = entry;
    entry = entry->next;
    free(tmp);
  }
}

ACMDU(do_help)
{
  struct help_entry_list *entries = NULL, *tmp = NULL;
  struct help_keyword_list *keywords = NULL, *tmp_keyword = NULL;

  char help_entry_buffer[MAX_STRING_LENGTH];
  char immo_data_buffer[1024];
  char *raw_argument;

  if (!ch->desc)
    return;

  skip_spaces(&argument);

  if (!*argument)
  {
    if (GET_LEVEL(ch) < LVL_IMMORT)
      page_string(ch->desc, help, 0);
    else
      page_string(ch->desc, ihelp, 0);
    return;
  }
  raw_argument = strdup(argument);
  space_to_minus(argument);

  if (display_class_info(ch, raw_argument))
  {
    free(raw_argument);
    return;
  }
  if (display_race_info(ch, raw_argument))
  {
    free(raw_argument);
    return;
  }

  if ((entries = search_help(argument, GET_LEVEL(ch))) == NULL)
  {
    /* Check alchemist discoveries for relevant entries! */
    if (!display_discovery_info(ch, raw_argument))
    {
      /* And check grand alchemist discoveries for relevant entries! */
      if (!display_grand_discovery_info(ch, raw_argument))
      {
        /* And list bomb types if keyword matched */
        if (!display_bomb_types(ch, raw_argument))
        {
          /* And list discovery types if keyword matched */
          if (!display_discovery_types(ch, raw_argument))
          {
            /* Check feats for relevant entries! */
            if (!display_feat_info(ch, raw_argument))
            {

              send_to_char(ch, "There is no help on that word.\r\n");
              mudlog(NRM, MAX(LVL_IMPL, GET_INVIS_LEV(ch)), TRUE,
                     "%s tried to get help on %s", GET_NAME(ch), argument);

              /* Implement 'SOUNDS LIKE' search here... */
              if ((keywords = soundex_search_help_keywords(argument, GET_LEVEL(ch))) != NULL)
              {
                send_to_char(ch, "\r\nDid you mean:\r\n");
                tmp_keyword = keywords;
                while (tmp_keyword != NULL)
                {
                  send_to_char(ch, "  \t<send href=\"Help %s\">%s\t</send>\r\n",
                               tmp_keyword->keyword, tmp_keyword->keyword);
                  tmp_keyword = tmp_keyword->next;
                }
                send_to_char(ch, "\tDYou can also check the help index, type 'hindex <keyword>'\tn\r\n");
                while (keywords != NULL)
                {
                  tmp_keyword = keywords->next;
                  free(keywords);
                  keywords = tmp_keyword;
                  tmp_keyword = NULL;
                }
              }
            }
          }
        }
      }
    }
    free(raw_argument);
    return;
  }

  /* Help entry format:
   *  -------------------------------Help Entry-----------------------------------
   *  Help tag      : <tag> (immortal only)
   *  Help Keywords : <Keywords>
   *  Help Category : <Category>
   *  Related Help  : <Related Help entries>
   *  Last Updated  : <Update date>
   *  ----------------------------------------------------------------------------
   *  <HELP ENTRY TEXT>
   *  ----------------------------------------------------------------------------
   *  */
  snprintf(immo_data_buffer, sizeof(immo_data_buffer), "\tcHelp Tag      : \tn%s\r\n",
          entries->tag);
  snprintf(help_entry_buffer, sizeof(help_entry_buffer), "\tC%s\tn"
                             "%s"
                             "\tcHelp Keywords : \tn%s\r\n"
                             // "\tcHelp Category : \tn%s\r\n"
                             // "\tcRelated Help  : \tn%s\r\n"
                             "\tcLast Updated  : \tn%s\r\n"
                             "\tC%s"
                             "\tn%s\r\n"
                             "\tC%s",
          line_string(GET_SCREEN_WIDTH(ch), '-', '-'),
          (IS_IMMORTAL(ch) ? immo_data_buffer : ""),
          entries->keywords,
          // "<N/I>",
          // "<N/I>",
          entries->last_updated,
          line_string(GET_SCREEN_WIDTH(ch), '-', '-'),
          entries->entry,
          line_string(GET_SCREEN_WIDTH(ch), '-', '-'));
  page_string(ch->desc, help_entry_buffer, 1);

  free(raw_argument);
  while (entries != NULL)
  {
    tmp = entries;
    entries = entries->next;

    while (tmp->keyword_list != NULL)
    {
      tmp_keyword = tmp->keyword_list->next;
      free(tmp->keyword_list);
      tmp->keyword_list = tmp_keyword;
    }

    free(tmp);
  }
}

//  send_to_char(ch, "\tDYou can also check the help index, type 'hindex <keyword>'\tn\r\n");
