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
#include "constants.h"
#include "deities.h"
#include "act.h"
#include "evolutions.h"
#include "backgrounds.h"

/* Help cache system - stores recent help queries to reduce database load */
#define HELP_CACHE_SIZE 50    /* Number of cached help entries */
#define HELP_CACHE_TTL 300    /* Cache time-to-live in seconds (5 minutes) */

struct help_cache_entry {
  char *search_key;            /* The search term used */
  int level;                   /* The level restriction used */
  struct help_entry_list *result; /* Cached result (deep copy) */
  time_t timestamp;            /* When this entry was cached */
  struct help_cache_entry *next;
};

static struct help_cache_entry *help_cache = NULL;
static int help_cache_count = 0;

/* Forward declarations for cache functions */
static struct help_entry_list *get_cached_help(const char *argument, int level);
static void add_to_help_cache(const char *argument, int level, struct help_entry_list *result);
static struct help_entry_list *deep_copy_help_list(struct help_entry_list *src);
static void free_help_entry_list(struct help_entry_list *entry);
static void purge_old_cache_entries(void);

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
 * YOU HAVE BEEN WARNED.
 * 
 * Enhanced with caching system to reduce database queries.
 * Cache stores recent searches for HELP_CACHE_TTL seconds.  */
struct help_entry_list *search_help(const char *argument, int level)
{
  struct help_entry_list *cached_result;
  
  /* Check cache first to avoid database query */
  cached_result = get_cached_help(argument, level);
  if (cached_result != NULL) {
    return cached_result;  /* Return deep copy from cache */
  }

  MYSQL_RES *result;
  MYSQL_ROW row;

  struct help_entry_list *help_entries = NULL, *new_help_entry = NULL, *cur = NULL;

  char buf[1024], escaped_arg[MAX_STRING_LENGTH] = {'\0'};

  /*  Check the connection, reconnect if necessary. */
  mysql_ping(conn);

  mysql_real_escape_string(conn, escaped_arg, argument, strlen(argument));

  /* Optimized query - eliminates Cartesian product by using proper JOINs
   * First finds matching help entries, then gets all keywords in one query */
  snprintf(buf, sizeof(buf), "SELECT distinct he.tag, he.entry, he.min_level, he.last_updated, group_concat(distinct CONCAT(UCASE(LEFT(hk2.keyword, 1)), LCASE(SUBSTRING(hk2.keyword, 2))) separator ', ')"
                             " FROM `help_entries` he"
                             " INNER JOIN `help_keywords` hk ON he.tag = hk.help_tag"
                             " LEFT JOIN `help_keywords` hk2 ON he.tag = hk2.help_tag"
                             " WHERE lower(hk.keyword) like '%s%%' and he.min_level <= %d"
                             " group by he.tag, he.entry, he.min_level, he.last_updated ORDER BY length(hk.keyword) asc",
           escaped_arg, level);

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

    /* N+1 query optimization: keywords already fetched in main query,
     * only fetch keyword_list if actually needed elsewhere.
     * For now, set to NULL to avoid redundant query */
    new_help_entry->keyword_list = NULL;

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

  /* Add result to cache if found */
  if (help_entries != NULL) {
    add_to_help_cache(argument, level, help_entries);
  }

  return help_entries
}

struct help_keyword_list *get_help_keywords(const char *tag)
{
  MYSQL_RES *result;
  MYSQL_ROW row;

  struct help_keyword_list *keywords = NULL, *new_keyword = NULL, *cur = NULL;

  char buf[MAX_STRING_LENGTH * 2 + 200]; /* Large enough for escaped_tag plus SQL query */
  char escaped_tag[MAX_STRING_LENGTH * 2 + 1];

  /* Get keywords for this entry. */
  mysql_real_escape_string(conn, escaped_tag, tag, strlen(tag));
  snprintf(buf, sizeof(buf), "select help_tag, CONCAT(UCASE(LEFT(keyword, 1)), LCASE(SUBSTRING(keyword, 2))) from help_keywords where help_tag = '%s'", escaped_tag);
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

  char buf[1024], escaped_arg[MAX_STRING_LENGTH] = {'\0'};

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
           escaped_arg, level);

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
    send_to_char(d->character, "%s", entry->entry);
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
  int i = 0;
  char help_entry_buffer[MAX_STRING_LENGTH] = {'\0'};
  char immo_data_buffer[1024];
  char *raw_argument;
  char spell_argument[200];

  if (!ch->desc)
    return;

  skip_spaces(&argument);
  char *home_arg = strdup(argument);

  if (!*argument)
  {
    if (GET_LEVEL(ch) < LVL_IMMORT)
      page_string(ch->desc, help, 0);
    else
      page_string(ch->desc, ihelp, 0);
    return;
  }

  for (i = 0; i < NUM_DEITIES; i++)
  {
    if (is_abbrev(argument, deity_list[i].name))
    {
      snprintf(spell_argument, sizeof(spell_argument), "info %s", argument);
      do_devote(ch, spell_argument, 0, 0);
      return;
    }
  }

  raw_argument = strdup(argument);
  space_to_minus(argument);

  // help regions
  for (i = 0; home_arg[i] != '\0'; i++)
  {
    // check first character is lowercase alphabet
    if (i == 0)
    {
      if ((home_arg[i] >= 'a' && home_arg[i] <= 'z'))
        home_arg[i] = home_arg[i] - 32; // subtract 32 to make it capital
      continue;                         // continue to the loop
    }
    if (home_arg[i] == ' ') // check space
    {
      // if space is found, check next character
      ++i;
      // check next character is lowercase alphabet
      if (home_arg[i] >= 'a' && home_arg[i] <= 'z')
      {
        home_arg[i] = home_arg[i] - 32; // subtract 32 to make it capital
        continue;                       // continue to the loop
      }
    }
    else
    {
      // all other uppercase characters should be in lowercase
      if (home_arg[i] >= 'A' && home_arg[i] <= 'Z')
        home_arg[i] = home_arg[i] + 32; // subtract 32 to make it small/lowercase
    }
  }

  for (i = 1; i < NUM_REGIONS; i++)
  {
    if (is_abbrev(home_arg, regions[i]))
    {
      display_region_info(ch, i);
      free(raw_argument);
      return;
    }
  }

  for (i = 1; i < NUM_BACKGROUNDS; i++)
  {
    if (is_abbrev(home_arg, background_list[i].name))
    {
      show_background_help(ch, i);
      free(raw_argument);
      return;
    }
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
              /* Check feats for relevant entries! */
              if (!display_evolution_info(ch, raw_argument))
              {

                /* check weapon info */
                if (display_weapon_info(ch, raw_argument))
                {
                  free(raw_argument);
                  return;
                }

                /* check armor info */
                if (display_armor_info(ch, raw_argument))
                {
                  free(raw_argument);
                  return;
                }

                /* check class info */
                if (display_class_info(ch, raw_argument))
                {
                  free(raw_argument);
                  return;
                }

                /* check race info */
                if (display_race_info(ch, raw_argument))
                {
                  free(raw_argument);
                  return;
                }

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
                    /* Free strdup'd strings in keyword list */
                    if (keywords->tag) free(keywords->tag);
                    if (keywords->keyword) free(keywords->keyword);
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

    /* Free strdup'd strings in help_entry_list */
    if (tmp->tag) free(tmp->tag);
    if (tmp->entry) free(tmp->entry);
    if (tmp->keywords) free(tmp->keywords);
    if (tmp->last_updated) free(tmp->last_updated);

    while (tmp->keyword_list != NULL)
    {
      tmp_keyword = tmp->keyword_list->next;
      /* Free strdup'd strings in keyword list */
      if (tmp->keyword_list->tag) free(tmp->keyword_list->tag);
      if (tmp->keyword_list->keyword) free(tmp->keyword_list->keyword);
      free(tmp->keyword_list);
      tmp->keyword_list = tmp_keyword;
    }

    free(tmp);
  }
}

//  send_to_char(ch, "\tDYou can also check the help index, type 'hindex <keyword>'\tn\r\n");

/* Cache management functions implementation */

/**
 * Searches the help cache for a matching entry.
 * Returns a deep copy of the cached result if found and still valid.
 * 
 * @param argument The search term
 * @param level The minimum level restriction
 * @return Deep copy of cached help_entry_list, or NULL if not found/expired
 */
static struct help_entry_list *get_cached_help(const char *argument, int level)
{
  struct help_cache_entry *entry;
  time_t current_time;
  
  /* Periodically clean expired entries */
  purge_old_cache_entries();
  
  current_time = time(NULL);
  
  /* Search cache for matching entry */
  for (entry = help_cache; entry != NULL; entry = entry->next) {
    /* Check if entry matches search criteria */
    if (entry->level == level && 
        !str_cmp(entry->search_key, argument)) {
      
      /* Check if entry is still valid (not expired) */
      if ((current_time - entry->timestamp) < HELP_CACHE_TTL) {
        /* Return deep copy of cached result */
        return deep_copy_help_list(entry->result);
      }
    }
  }
  
  return NULL;  /* Not found in cache or expired */
}

/**
 * Adds a help search result to the cache.
 * Maintains cache size limit by removing oldest entries if needed.
 * 
 * @param argument The search term used
 * @param level The level restriction used  
 * @param result The help entries to cache (will be deep copied)
 */
static void add_to_help_cache(const char *argument, int level, 
                              struct help_entry_list *result)
{
  struct help_cache_entry *new_entry, *entry, *prev, *oldest;
  time_t oldest_time;
  
  /* Don't cache NULL results */
  if (result == NULL)
    return;
  
  /* Check if this search is already cached and update it */
  for (entry = help_cache; entry != NULL; entry = entry->next) {
    if (entry->level == level && !str_cmp(entry->search_key, argument)) {
      /* Update existing cache entry */
      free_help_entry_list(entry->result);
      entry->result = deep_copy_help_list(result);
      entry->timestamp = time(NULL);
      return;
    }
  }
  
  /* If cache is full, remove oldest entry */
  if (help_cache_count >= HELP_CACHE_SIZE) {
    oldest = help_cache;
    oldest_time = help_cache->timestamp;
    prev = NULL;
    
    /* Find oldest entry */
    for (entry = help_cache; entry != NULL; entry = entry->next) {
      if (entry->timestamp < oldest_time) {
        oldest = entry;
        oldest_time = entry->timestamp;
      }
    }
    
    /* Remove oldest from list */
    if (oldest == help_cache) {
      help_cache = oldest->next;
    } else {
      for (entry = help_cache; entry != NULL; entry = entry->next) {
        if (entry->next == oldest) {
          entry->next = oldest->next;
          break;
        }
      }
    }
    
    /* Free oldest entry */
    free(oldest->search_key);
    free_help_entry_list(oldest->result);
    free(oldest);
    help_cache_count--;
  }
  
  /* Create new cache entry */
  CREATE(new_entry, struct help_cache_entry, 1);
  new_entry->search_key = strdup(argument);
  new_entry->level = level;
  new_entry->result = deep_copy_help_list(result);
  new_entry->timestamp = time(NULL);
  new_entry->next = help_cache;
  help_cache = new_entry;
  help_cache_count++;
}

/**
 * Creates a deep copy of a help_entry_list.
 * Used to store/retrieve independent copies from cache.
 * 
 * @param src The help list to copy
 * @return New deep copy of the list
 */
static struct help_entry_list *deep_copy_help_list(struct help_entry_list *src)
{
  struct help_entry_list *new_list = NULL, *new_entry, *cur = NULL;
  struct help_keyword_list *keyword, *new_keyword, *keyword_cur;
  
  while (src != NULL) {
    /* Copy help entry */
    CREATE(new_entry, struct help_entry_list, 1);
    new_entry->tag = src->tag ? strdup(src->tag) : NULL;
    new_entry->keywords = src->keywords ? strdup(src->keywords) : NULL;
    new_entry->entry = src->entry ? strdup(src->entry) : NULL;
    new_entry->min_level = src->min_level;
    new_entry->last_updated = src->last_updated ? strdup(src->last_updated) : NULL;
    new_entry->next = NULL;
    
    /* Copy keyword list */
    new_entry->keyword_list = NULL;
    keyword_cur = NULL;
    for (keyword = src->keyword_list; keyword != NULL; keyword = keyword->next) {
      CREATE(new_keyword, struct help_keyword_list, 1);
      new_keyword->tag = keyword->tag ? strdup(keyword->tag) : NULL;
      new_keyword->keyword = keyword->keyword ? strdup(keyword->keyword) : NULL;
      new_keyword->next = NULL;
      
      if (new_entry->keyword_list == NULL) {
        new_entry->keyword_list = new_keyword;
        keyword_cur = new_keyword;
      } else {
        keyword_cur->next = new_keyword;
        keyword_cur = new_keyword;
      }
    }
    
    /* Add to list */
    if (new_list == NULL) {
      new_list = new_entry;
      cur = new_entry;
    } else {
      cur->next = new_entry;
      cur = new_entry;
    }
    
    src = src->next;
  }
  
  return new_list;
}

/**
 * Frees a help_entry_list and all associated memory.
 * 
 * @param entry The help list to free
 */
static void free_help_entry_list(struct help_entry_list *entry)
{
  struct help_entry_list *tmp;
  struct help_keyword_list *keyword, *tmp_keyword;
  
  while (entry != NULL) {
    tmp = entry;
    entry = entry->next;
    
    /* Free strings */
    if (tmp->tag) free(tmp->tag);
    if (tmp->entry) free(tmp->entry);
    if (tmp->keywords) free(tmp->keywords);
    if (tmp->last_updated) free(tmp->last_updated);
    
    /* Free keyword list */
    keyword = tmp->keyword_list;
    while (keyword != NULL) {
      tmp_keyword = keyword;
      keyword = keyword->next;
      if (tmp_keyword->tag) free(tmp_keyword->tag);
      if (tmp_keyword->keyword) free(tmp_keyword->keyword);
      free(tmp_keyword);
    }
    
    free(tmp);
  }
}

/**
 * Removes expired entries from the cache.
 * Called periodically to keep cache size manageable.
 */
static void purge_old_cache_entries(void)
{
  struct help_cache_entry *entry, *next, *prev = NULL;
  time_t current_time = time(NULL);
  
  entry = help_cache;
  while (entry != NULL) {
    next = entry->next;
    
    /* Check if entry is expired */
    if ((current_time - entry->timestamp) >= HELP_CACHE_TTL) {
      /* Remove from list */
      if (prev == NULL) {
        help_cache = next;
      } else {
        prev->next = next;
      }
      
      /* Free memory */
      free(entry->search_key);
      free_help_entry_list(entry->result);
      free(entry);
      help_cache_count--;
    } else {
      prev = entry;
    }
    
    entry = next;
  }
}
