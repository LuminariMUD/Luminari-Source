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

/* Constants to replace magic numbers */
#define MAX_HELP_SEARCH_LENGTH 200
#define MAX_HELP_ENTRY_BUFFER 4096
#define MIN_SEARCH_LENGTH_FOR_STRICT 2
#define MAX_KEYWORD_LENGTH_DIFF 3

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

/* Centralized memory management for help entries - eliminates duplication */
static void free_help_entry_single(struct help_entry_list *entry);
static void free_help_keyword_list(struct help_keyword_list *keyword_list);
static struct help_entry_list *alloc_help_entry(void);
static struct help_keyword_list *alloc_help_keyword(void);

/* Debug flag for help system - set to 1 to enable debug logging, 0 to disable
 * This will log detailed information about:
 * - Database connection status
 * - SQL query preparation and execution
 * - Number of results returned
 * - Handler chain execution
 * - Cache hits/misses
 * - Fallback to file-based help
 */
#define HELP_DEBUG 0

/* puts -'s instead of spaces */
void space_to_minus(char *str)
{
  while ((str = strchr(str, ' ')) != NULL)
    *str = '-';
}

/* Search the file-based help_table for matching entries
 * This is used as a fallback when database search returns nothing
 * Returns a help_entry_list structure compatible with database results
 */
static struct help_entry_list *search_help_table(const char *argument, int level)
{
  extern struct help_index_element *help_table;
  extern int top_of_helpt;
  struct help_entry_list *help_entries = NULL, *new_entry = NULL, *cur = NULL;
  int i;
  
  /* No file-based help loaded */
  if (!help_table || top_of_helpt <= 0) {
    return NULL;
  }
  
  /* Search through file-based help entries */
  for (i = 0; i < top_of_helpt; i++) {
    /* Check if user has access level for this entry */
    if (help_table[i].min_level > level) {
      continue;
    }
    
    /* Check if keywords match (prefix match) */
    if (!help_table[i].keywords) {
      continue;
    }
    
    /* Case-insensitive prefix match */
    if (strn_cmp(argument, help_table[i].keywords, strlen(argument)) == 0) {
      /* Found a match - convert to help_entry_list format */
      new_entry = alloc_help_entry();
      if (!new_entry) {
        log("SYSERR: search_help_table: Failed to allocate help entry");
        continue;
      }
      
      /* Set entry data from file-based help */
      new_entry->tag = strdup(help_table[i].keywords);
      new_entry->entry = strdup(help_table[i].entry ? help_table[i].entry : "No help available.\r\n");
      new_entry->min_level = help_table[i].min_level;
      new_entry->last_updated = strdup("File-based");
      new_entry->keywords = strdup(help_table[i].keywords);
      new_entry->keyword_list = NULL;
      new_entry->next = NULL;
      
      /* Check for allocation failures - use centralized cleanup */
      if (!new_entry->tag || !new_entry->entry || !new_entry->last_updated || !new_entry->keywords) {
        log("SYSERR: search_help_table: Memory allocation failed for help entry '%s'", 
            help_table[i].keywords ? help_table[i].keywords : "(null)");
        free_help_entry_single(new_entry);
        free(new_entry);
        continue;
      }
      
      /* Add to linked list */
      if (help_entries == NULL) {
        help_entries = new_entry;
        cur = new_entry;
      } else {
        cur->next = new_entry;
        cur = new_entry;
      }
      
      /* For file-based help, return after first match
       * (traditional behavior - one entry per keyword) */
      break;
    }
  }
  
  return help_entries;
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
  PREPARED_STMT *pstmt;
  struct help_entry_list *help_entries = NULL, *new_help_entry = NULL, *cur = NULL;
  char search_pattern[MAX_STRING_LENGTH];
  int row_count = 0;
  
  /* Check cache first to avoid database query */
  cached_result = get_cached_help(argument, level);
  if (cached_result != NULL) {
    if (HELP_DEBUG) log("DEBUG: search_help: Found '%s' in cache", argument);
    return cached_result;  /* Return deep copy from cache */
  }
  
  if (HELP_DEBUG) log("DEBUG: search_help: Searching database for '%s' (level %d)", argument, level);

  /* Check the connection, reconnect if necessary */
  if (!conn) {
    log("SYSERR: search_help: No database connection available for help search '%s'", argument);
    if (HELP_DEBUG) log("DEBUG: search_help: ERROR - No database connection!");
    return search_help_table(argument, level);
  }
  
  /* Ensure connection is still alive */
  if (mysql_ping(conn) != 0) {
    log("SYSERR: search_help: Database connection lost while searching for '%s', using file-based help", argument);
    if (HELP_DEBUG) log("DEBUG: search_help: MySQL ping failed, connection lost");
    return search_help_table(argument, level);
  }
  if (HELP_DEBUG) log("DEBUG: search_help: Database connection OK");

  /* Create prepared statement for secure query execution */
  pstmt = mysql_stmt_create(conn);
  if (!pstmt) {
    log("SYSERR: Failed to create prepared statement for help search '%s': %s", 
        argument, mysql_error(conn));
    if (HELP_DEBUG) log("DEBUG: search_help: Failed to create prepared statement, falling back to file-based");
    return search_help_table(argument, level);
  }
  if (HELP_DEBUG) log("DEBUG: search_help: Created prepared statement");

  /* Prepare the parameterized query - ? placeholders prevent SQL injection
   * This query finds matching help entries with all their keywords */
  if (!mysql_stmt_prepare_query(pstmt, 
      "SELECT DISTINCT he.tag, he.entry, he.min_level, he.last_updated, "
      "GROUP_CONCAT(DISTINCT CONCAT(UCASE(LEFT(hk2.keyword, 1)), LCASE(SUBSTRING(hk2.keyword, 2))) SEPARATOR ', ') "
      "FROM help_entries he "
      "INNER JOIN help_keywords hk ON he.tag = hk.help_tag "
      "LEFT JOIN help_keywords hk2 ON he.tag = hk2.help_tag "
      "WHERE LOWER(hk.keyword) LIKE LOWER(?) AND he.min_level <= ? "
      "GROUP BY he.tag, he.entry, he.min_level, he.last_updated "
      "ORDER BY LENGTH(hk.keyword) ASC")) {
    log("SYSERR: Failed to prepare help search query for '%s': %s", 
        argument, mysql_error(conn));
    if (HELP_DEBUG) log("DEBUG: search_help: Failed to prepare query, falling back to file-based");
    mysql_stmt_cleanup(pstmt);
    return search_help_table(argument, level);
  }
  if (HELP_DEBUG) log("DEBUG: search_help: Query prepared successfully");

  /* Build search pattern for LIKE clause (add % for prefix matching) 
   * The query uses LOWER() on both sides, so we don't need to lowercase here
   * Actually, let's use LOWER(?) in the query to be safe */
  snprintf(search_pattern, sizeof(search_pattern), "%s%%", argument);
  if (HELP_DEBUG) log("DEBUG: search_help: Search pattern is '%s'", search_pattern);
  
  /* Bind parameters - completely safe from SQL injection */
  if (!mysql_stmt_bind_param_string(pstmt, 0, search_pattern) ||
      !mysql_stmt_bind_param_int(pstmt, 1, level)) {
    log("SYSERR: Failed to bind parameters for help search '%s' (pattern='%s', level=%d)", 
        argument, search_pattern, level);
    if (HELP_DEBUG) log("DEBUG: search_help: Failed to bind parameters, falling back to file-based");
    mysql_stmt_cleanup(pstmt);
    return search_help_table(argument, level);
  }
  if (HELP_DEBUG) log("DEBUG: search_help: Parameters bound (pattern='%s', level=%d)", search_pattern, level);

  /* Execute the prepared statement */
  if (!mysql_stmt_execute_prepared(pstmt)) {
    log("SYSERR: Failed to execute help search query for '%s': %s", 
        argument, mysql_error(conn));
    if (HELP_DEBUG) log("DEBUG: search_help: Query execution failed, falling back to file-based");
    mysql_stmt_cleanup(pstmt);
    return search_help_table(argument, level);
  }
  if (HELP_DEBUG) log("DEBUG: search_help: Query executed successfully");

  /* Fetch results row by row */
  while (mysql_stmt_fetch_row(pstmt)) {
    row_count++;
    if (HELP_DEBUG && row_count == 1) log("DEBUG: search_help: Found database results for '%s'", argument);
    
    /* Allocate memory for the help entry data using centralized function */
    new_help_entry = alloc_help_entry();
    if (!new_help_entry) {
      log("SYSERR: search_help: Failed to allocate help entry");
      continue;
    }
    
    /* Get column values safely from prepared statement results */
    new_help_entry->tag = strdup(mysql_stmt_get_string(pstmt, 0) ? mysql_stmt_get_string(pstmt, 0) : "");
    new_help_entry->entry = strdup(mysql_stmt_get_string(pstmt, 1) ? mysql_stmt_get_string(pstmt, 1) : "");
    new_help_entry->min_level = mysql_stmt_get_int(pstmt, 2);
    new_help_entry->last_updated = strdup(mysql_stmt_get_string(pstmt, 3) ? mysql_stmt_get_string(pstmt, 3) : "");
    new_help_entry->keywords = strdup(mysql_stmt_get_string(pstmt, 4) ? mysql_stmt_get_string(pstmt, 4) : "");
    
    /* Check for allocation failures - use centralized cleanup */
    if (!new_help_entry->tag || !new_help_entry->entry || 
        !new_help_entry->last_updated || !new_help_entry->keywords) {
      log("SYSERR: search_help: Memory allocation failed for help entry");
      free_help_entry_single(new_help_entry);
      free(new_help_entry);
      continue;
    }

    /* N+1 query optimization: keywords already fetched in main query,
     * only fetch keyword_list if actually needed elsewhere.
     * For now, set to NULL to avoid redundant query */
    new_help_entry->keyword_list = NULL;
    new_help_entry->next = NULL;

    /* Add to linked list */
    if (help_entries == NULL) {
      help_entries = new_help_entry;
      cur = new_help_entry;
    } else {
      cur->next = new_help_entry;
      cur = new_help_entry;
    }
    new_help_entry = NULL;
  }

  /* Clean up prepared statement */
  mysql_stmt_cleanup(pstmt);
  
  if (HELP_DEBUG) log("DEBUG: search_help: Query returned %d rows for '%s'", row_count, argument);

  /* If database returned no results, try file-based help as fallback */
  if (help_entries == NULL) {
    if (HELP_DEBUG) log("DEBUG: search_help: No database results for '%s', trying file-based", argument);
    help_entries = search_help_table(argument, level);
    if (help_entries) {
      if (HELP_DEBUG) log("DEBUG: search_help: Found file-based help for '%s'", argument);
    } else {
      if (HELP_DEBUG) log("DEBUG: search_help: No file-based help for '%s' either", argument);
    }
    /* Don't cache file-based results - they're already in memory */
  } else {
    if (HELP_DEBUG) log("DEBUG: search_help: Found database help for '%s'", argument);
    /* Add database results to cache */
    add_to_help_cache(argument, level, help_entries);
    
    /* Log successful keyword search for analytics */
    if (conn && mysql_available) {
      char escaped_term[MAX_HELP_SEARCH_LENGTH * 2 + 1];
      mysql_real_escape_string(conn, escaped_term, argument, strlen(argument));
      
      int count = 0;
      struct help_entry_list *tmp;
      for (tmp = help_entries; tmp; tmp = tmp->next) count++;
      
      char query[MAX_STRING_LENGTH];
      snprintf(query, sizeof(query),
        "INSERT INTO help_search_history (search_term, searcher_level, results_count, search_type) "
        "VALUES ('%s', %d, %d, 'keyword')",
        escaped_term, level, count);
      
      /* Non-critical - don't fail if logging fails */
      mysql_query_safe(conn, query);
    }
  }

  return help_entries;
}

/**
 * Full-text search function for help entries.
 * Searches within the actual content of help entries, not just keywords.
 * 
 * @param search_term The text to search for within help content
 * @param level The minimum level required to view results
 * @return List of matching help entries or NULL if none found
 */
struct help_entry_list *search_help_fulltext(const char *search_term, int level)
{
  PREPARED_STMT *pstmt;
  struct help_entry_list *help_entries = NULL, *new_help_entry = NULL, *cur = NULL;
  char search_pattern[MAX_STRING_LENGTH];
  int row_count = 0;
  
  if (HELP_DEBUG) log("DEBUG: search_help_fulltext: Searching for '%s' in help content (level %d)", search_term, level);

  /* Check the connection */
  if (!conn) {
    log("SYSERR: search_help_fulltext: No database connection available for fulltext search '%s'", search_term);
    return NULL;  /* No file-based fulltext search fallback */
  }
  
  /* Ensure connection is still alive */
  if (mysql_ping(conn) != 0) {
    log("SYSERR: search_help_fulltext: Database connection lost while searching for '%s'", search_term);
    return NULL;
  }

  /* Create prepared statement */
  pstmt = mysql_stmt_create(conn);
  if (!pstmt) {
    log("SYSERR: Failed to create prepared statement for fulltext help search '%s': %s", 
        search_term, mysql_error(conn));
    return NULL;
  }

  /* Prepare fulltext search query - searches in both entry content and keywords */
  if (!mysql_stmt_prepare_query(pstmt, 
      "SELECT DISTINCT he.tag, he.entry, he.min_level, he.last_updated, "
      "GROUP_CONCAT(DISTINCT CONCAT(UCASE(LEFT(hk.keyword, 1)), LCASE(SUBSTRING(hk.keyword, 2))) SEPARATOR ', ') "
      "FROM help_entries he "
      "LEFT JOIN help_keywords hk ON he.tag = hk.help_tag "
      "WHERE (LOWER(he.entry) LIKE LOWER(?) OR LOWER(hk.keyword) LIKE LOWER(?)) "
      "AND he.min_level <= ? "
      "GROUP BY he.tag, he.entry, he.min_level, he.last_updated "
      "ORDER BY "
      "  CASE WHEN LOWER(hk.keyword) LIKE LOWER(?) THEN 0 ELSE 1 END, "  /* Keyword matches first */
      "  LENGTH(he.entry) ASC "  /* Shorter entries first */
      "LIMIT 20")) {  /* Limit results to prevent overwhelming output */
    log("SYSERR: Failed to prepare fulltext help search query for '%s': %s", 
        search_term, mysql_error(conn));
    mysql_stmt_cleanup(pstmt);
    return NULL;
  }

  /* Build search pattern with wildcards for partial matching */
  snprintf(search_pattern, sizeof(search_pattern), "%%%s%%", search_term);

  /* Bind parameters - search pattern used 4 times (2 in WHERE, 1 in ORDER BY), level once */
  if (!mysql_stmt_bind_param_string(pstmt, 0, search_pattern) ||
      !mysql_stmt_bind_param_string(pstmt, 1, search_pattern) ||
      !mysql_stmt_bind_param_int(pstmt, 2, level) ||
      !mysql_stmt_bind_param_string(pstmt, 3, search_pattern)) {
    log("SYSERR: Failed to bind parameters for fulltext help search '%s': %s", 
        search_term, mysql_error(conn));
    mysql_stmt_cleanup(pstmt);
    return NULL;
  }

  /* Execute query */
  if (!mysql_stmt_execute_prepared(pstmt)) {
    log("SYSERR: Failed to execute fulltext help search for '%s': %s", 
        search_term, mysql_error(conn));
    mysql_stmt_cleanup(pstmt);
    return NULL;
  }

  /* Fetch and process each result row */
  while (mysql_stmt_fetch_row(pstmt)) {
    new_help_entry = alloc_help_entry();
    if (!new_help_entry) {
      log("SYSERR: search_help_fulltext: Failed to allocate help entry");
      continue;
    }
    
    /* Get result columns using the API */
    new_help_entry->tag = strdup(mysql_stmt_get_string(pstmt, 0) ? mysql_stmt_get_string(pstmt, 0) : "");
    new_help_entry->entry = strdup(mysql_stmt_get_string(pstmt, 1) ? mysql_stmt_get_string(pstmt, 1) : "");
    new_help_entry->min_level = mysql_stmt_get_int(pstmt, 2);
    new_help_entry->last_updated = strdup(mysql_stmt_get_string(pstmt, 3) ? mysql_stmt_get_string(pstmt, 3) : "");
    new_help_entry->keywords = strdup(mysql_stmt_get_string(pstmt, 4) ? mysql_stmt_get_string(pstmt, 4) : "");
    new_help_entry->next = NULL;
    new_help_entry->keyword_list = NULL;
    
    /* Get keyword list for this entry */
    if (new_help_entry->tag) {
      new_help_entry->keyword_list = get_help_keywords(new_help_entry->tag);
    }
    
    /* Add to result list */
    if (!help_entries) {
      help_entries = new_help_entry;
      cur = help_entries;
    } else {
      cur->next = new_help_entry;
      cur = cur->next;
    }
    row_count++;
  }

  /* Clean up */
  mysql_stmt_cleanup(pstmt);
  
  if (HELP_DEBUG) log("DEBUG: search_help_fulltext: Found %d results for '%s'", row_count, search_term);
  
  /* Log fulltext search to history for analytics */
  if (conn && mysql_available && search_term && *search_term) {
    char escaped_term[MAX_HELP_SEARCH_LENGTH * 2 + 1];
    mysql_real_escape_string(conn, escaped_term, search_term, strlen(search_term));
    
    char query[MAX_STRING_LENGTH];
    snprintf(query, sizeof(query),
      "INSERT INTO help_search_history (search_term, searcher_level, results_count, search_type) "
      "VALUES ('%s', %d, %d, 'fulltext')",
      escaped_term, level, row_count);
    
    /* Non-critical - don't fail if logging fails */
    mysql_query_safe(conn, query);
  }
  
  return help_entries;
}

struct help_keyword_list *get_help_keywords(const char *tag)
{
  PREPARED_STMT *pstmt;
  struct help_keyword_list *keywords = NULL, *new_keyword = NULL, *cur = NULL;

  /* Create prepared statement for secure query execution */
  pstmt = mysql_stmt_create(conn);
  if (!pstmt) {
    log("SYSERR: Failed to create prepared statement for get_help_keywords");
    return NULL;
  }

  /* Prepare parameterized query to get keywords for a help tag */
  if (!mysql_stmt_prepare_query(pstmt,
      "SELECT help_tag, CONCAT(UCASE(LEFT(keyword, 1)), LCASE(SUBSTRING(keyword, 2))) "
      "FROM help_keywords WHERE help_tag = ?")) {
    log("SYSERR: Failed to prepare get_help_keywords query");
    mysql_stmt_cleanup(pstmt);
    return NULL;
  }

  /* Bind the tag parameter - SQL injection safe */
  if (!mysql_stmt_bind_param_string(pstmt, 0, tag)) {
    log("SYSERR: Failed to bind tag parameter for get_help_keywords");
    mysql_stmt_cleanup(pstmt);
    return NULL;
  }

  /* Execute the prepared statement */
  if (!mysql_stmt_execute_prepared(pstmt)) {
    log("SYSERR: Failed to execute get_help_keywords query");
    mysql_stmt_cleanup(pstmt);
    return NULL;
  }

  /* Fetch results row by row */
  while (mysql_stmt_fetch_row(pstmt)) {
    new_keyword = alloc_help_keyword();
    if (!new_keyword) {
      log("SYSERR: get_help_keywords: Failed to allocate keyword");
      continue;
    }
    
    new_keyword->tag = strdup(mysql_stmt_get_string(pstmt, 0) ? mysql_stmt_get_string(pstmt, 0) : "");
    new_keyword->keyword = strdup(mysql_stmt_get_string(pstmt, 1) ? mysql_stmt_get_string(pstmt, 1) : "");
    new_keyword->next = NULL;
    
    /* Check for allocation failures - use single keyword free */
    if (!new_keyword->tag || !new_keyword->keyword) {
      log("SYSERR: get_help_keywords: Memory allocation failed");
      if (new_keyword->tag) { free(new_keyword->tag); new_keyword->tag = NULL; }
      if (new_keyword->keyword) { free(new_keyword->keyword); new_keyword->keyword = NULL; }
      free(new_keyword);
      continue;
    }

    if (keywords == NULL) {
      keywords = new_keyword;
      cur = new_keyword;
    } else {
      cur->next = new_keyword;
      cur = new_keyword;
    }
  }

  /* Clean up prepared statement */
  mysql_stmt_cleanup(pstmt);
  return keywords;
}

struct help_keyword_list *soundex_search_help_keywords(const char *argument, int level)
{
  PREPARED_STMT *pstmt;
  struct help_keyword_list *keywords = NULL, *new_keyword = NULL, *cur = NULL;
  int row_count = 0;
  
  if (HELP_DEBUG) log("DEBUG: soundex_search_help_keywords: Searching for suggestions for '%s' (level %d)", argument, level);

  /* Check the connection, reconnect if necessary */
  if (!conn) {
    if (HELP_DEBUG) log("DEBUG: soundex_search_help_keywords: ERROR - No database connection!");
    return NULL;
  }
  
  /* Ensure connection is still alive */
  if (mysql_ping(conn) != 0) {
    if (HELP_DEBUG) log("DEBUG: soundex_search_help_keywords: MySQL ping failed, connection lost");
    return NULL;
  }
  if (HELP_DEBUG) log("DEBUG: soundex_search_help_keywords: Database connection OK");

  /* Create prepared statement for secure soundex search */
  pstmt = mysql_stmt_create(conn);
  if (!pstmt) {
    log("SYSERR: Failed to create prepared statement for soundex search");
    if (HELP_DEBUG) log("DEBUG: soundex_search_help_keywords: Failed to create prepared statement");
    return NULL;
  }
  if (HELP_DEBUG) log("DEBUG: soundex_search_help_keywords: Created prepared statement");

  /* Prepare parameterized query for soundex (sounds like) search
   * This helps users find help entries when they don't know exact spelling
   * Enhanced query:
   * 1. First tries SOUNDS LIKE for phonetic matching
   * 2. Also includes keywords that start with the search term
   * 3. Also includes keywords where search term appears anywhere (for typos)
   * Results are scored by relevance */
  if (!mysql_stmt_prepare_query(pstmt,
      "SELECT DISTINCT hk.help_tag, hk.keyword, "
      "CASE "
      "  WHEN LOWER(hk.keyword) = LOWER(?) THEN 1 "  /* Exact match (shouldn't happen but just in case) */
      "  WHEN hk.keyword SOUNDS LIKE ? THEN 2 "      /* Soundex match */
      "  WHEN LOWER(hk.keyword) LIKE LOWER(CONCAT(?, '%%')) THEN 3 " /* Starts with */
      "  WHEN LOWER(hk.keyword) LIKE LOWER(CONCAT('%%', ?, '%%')) THEN 4 " /* Contains */
      "  ELSE 5 "
      "END as relevance "
      "FROM help_entries he, help_keywords hk "
      "WHERE he.tag = hk.help_tag "
      "AND he.min_level <= ? "
      "AND ( "
      "  hk.keyword SOUNDS LIKE ? OR "
      "  LOWER(hk.keyword) LIKE LOWER(CONCAT(?, '%%')) OR "
      "  (LENGTH(?) >= 4 AND LOWER(hk.keyword) LIKE LOWER(CONCAT('%%', ?, '%%'))) " /* Only do contains search for 4+ chars */
      ") "
      "ORDER BY relevance ASC, LENGTH(hk.keyword) ASC "
      "LIMIT 10")) {  /* Limit to 10 suggestions */
    log("SYSERR: Failed to prepare soundex search query");
    if (HELP_DEBUG) log("DEBUG: soundex_search_help_keywords: Failed to prepare query");
    mysql_stmt_cleanup(pstmt);
    return NULL;
  }
  if (HELP_DEBUG) log("DEBUG: soundex_search_help_keywords: Query prepared successfully");

  /* Bind parameters - SQL injection safe
   * We need to bind the same argument multiple times for different comparisons */
  if (!mysql_stmt_bind_param_string(pstmt, 0, argument) ||  /* For exact match case */
      !mysql_stmt_bind_param_string(pstmt, 1, argument) ||  /* For SOUNDS LIKE case */
      !mysql_stmt_bind_param_string(pstmt, 2, argument) ||  /* For starts with case */
      !mysql_stmt_bind_param_string(pstmt, 3, argument) ||  /* For contains case */
      !mysql_stmt_bind_param_int(pstmt, 4, level) ||       /* Level check */
      !mysql_stmt_bind_param_string(pstmt, 5, argument) ||  /* For SOUNDS LIKE in WHERE */
      !mysql_stmt_bind_param_string(pstmt, 6, argument) ||  /* For starts with in WHERE */
      !mysql_stmt_bind_param_string(pstmt, 7, argument) ||  /* For LENGTH check */
      !mysql_stmt_bind_param_string(pstmt, 8, argument)) {  /* For contains in WHERE */
    log("SYSERR: Failed to bind parameters for soundex search");
    if (HELP_DEBUG) log("DEBUG: soundex_search_help_keywords: Failed to bind parameters (argument='%s', level=%d)", argument, level);
    mysql_stmt_cleanup(pstmt);
    return NULL;
  }
  if (HELP_DEBUG) log("DEBUG: soundex_search_help_keywords: All parameters bound (argument='%s', level=%d)", argument, level);

  /* Execute the prepared statement */
  if (!mysql_stmt_execute_prepared(pstmt)) {
    log("SYSERR: Failed to execute soundex search query");
    if (HELP_DEBUG) log("DEBUG: soundex_search_help_keywords: Failed to execute query");
    mysql_stmt_cleanup(pstmt);
    return NULL;
  }
  if (HELP_DEBUG) log("DEBUG: soundex_search_help_keywords: Query executed successfully");

  /* Fetch results row by row */
  while (mysql_stmt_fetch_row(pstmt)) {
    row_count++;
    if (HELP_DEBUG && row_count == 1) log("DEBUG: soundex_search_help_keywords: Found soundex matches for '%s'", argument);
    
    /* Allocate memory for the keyword data using centralized function */
    new_keyword = alloc_help_keyword();
    if (!new_keyword) {
      log("SYSERR: soundex_search_help_keywords: Failed to allocate keyword");
      continue;
    }
    
    new_keyword->tag = strdup(mysql_stmt_get_string(pstmt, 0) ? mysql_stmt_get_string(pstmt, 0) : "");
    new_keyword->keyword = strdup(mysql_stmt_get_string(pstmt, 1) ? mysql_stmt_get_string(pstmt, 1) : "");
    new_keyword->next = NULL;
    
    /* Check for allocation failures - use single keyword free */
    if (!new_keyword->tag || !new_keyword->keyword) {
      log("SYSERR: soundex_search_help_keywords: Memory allocation failed");
      if (new_keyword->tag) { free(new_keyword->tag); new_keyword->tag = NULL; }
      if (new_keyword->keyword) { free(new_keyword->keyword); new_keyword->keyword = NULL; }
      free(new_keyword);
      continue;
    }

    if (keywords == NULL) {
      keywords = new_keyword;
      cur = new_keyword;
    } else {
      cur->next = new_keyword;
      cur = new_keyword;
    }
    new_keyword = NULL;
  }

  /* Clean up prepared statement */
  mysql_stmt_cleanup(pstmt);
  
  if (HELP_DEBUG) log("DEBUG: soundex_search_help_keywords: Found %d soundex matches for '%s'", row_count, argument);

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

/* ============================================================================
 * Chain of Responsibility Pattern Implementation for Help System
 * 
 * This refactoring addresses architectural issues in the help system:
 * - Eliminates deep nesting (was 8+ levels, now max 2)
 * - Separates concerns (each handler has single responsibility)
 * - Makes system extensible (new handlers can be added without modifying core)
 * - Centralizes memory management (single free point for raw_argument)
 * - Improves maintainability and testability
 * ============================================================================ */

/* Global handler chain head */
static struct help_handler *help_handler_chain = NULL;

/**
 * Registers a new help handler in the chain.
 * Handlers are added to the end to maintain priority order.
 * 
 * @param name Handler name for debugging/logging
 * @param handler Function pointer to the handler implementation
 */
void register_help_handler(const char *name, help_handler_func handler) {
    struct help_handler *new_handler, *last;
    
    /* Create new handler node */
    CREATE(new_handler, struct help_handler, 1);
    new_handler->name = name;
    new_handler->handler = handler;
    new_handler->next = NULL;
    
    /* Add to end of chain to maintain registration order */
    if (!help_handler_chain) {
        help_handler_chain = new_handler;
    } else {
        for (last = help_handler_chain; last->next; last = last->next);
        last->next = new_handler;
    }
}

/**
 * Initializes all help handlers in optimal order.
 * Called during boot to set up the handler chain.
 * 
 * Order optimized for performance:
 * 1. Database/file first (most specific, authored content)
 * 2. Game mechanics (generated but common queries)
 * 3. Special cases (less common)
 * 4. Fuzzy matching last (fallback)
 */
void init_help_handlers(void) {
    /* Primary database handler - most queries end here */
    register_help_handler("database", handle_database_help);
    
    /* Game mechanic handlers - frequently accessed */
    register_help_handler("feat", handle_feat_help);
    register_help_handler("class", handle_class_help);
    register_help_handler("race", handle_race_help);
    register_help_handler("weapon", handle_weapon_help);
    register_help_handler("armor", handle_armor_help);
    
    /* Special system handlers */
    register_help_handler("deity", handle_deity_help);
    register_help_handler("region", handle_region_help);
    register_help_handler("background", handle_background_help);
    
    /* Alchemy-related handlers */
    register_help_handler("discovery", handle_discovery_help);
    register_help_handler("grand_discovery", handle_grand_discovery_help);
    register_help_handler("bomb_types", handle_bomb_types_help);
    register_help_handler("discovery_types", handle_discovery_types_help);
    
    /* Evolution handler */
    register_help_handler("evolution", handle_evolution_help);
    
    /* Fallback - soundex suggestions (always last) */
    register_help_handler("soundex", handle_soundex_suggestions);
    
    log("Help handler chain initialized with %d handlers", 16);
}

/**
 * Cleans up the handler chain during shutdown.
 * Frees all allocated handler nodes.
 */
void cleanup_help_handlers(void) {
    struct help_handler *handler, *next;
    
    for (handler = help_handler_chain; handler; handler = next) {
        next = handler->next;
        free(handler);
    }
    help_handler_chain = NULL;
}

/* ============================================================================
 * Individual Handler Implementations
 * Each handler checks for one type of help content and returns 1 if handled
 * ============================================================================ */

/**
 * Handler for database/file-based help entries.
 * This is the primary handler for authored help content.
 * 
 * Enhanced to distinguish between exact and partial matches:
 * - Exact match: Display help and stop chain (return 1)
 * - Partial match only: Display help but continue to soundex (return 0)
 * This ensures users get "did you mean" suggestions for typos even when
 * partial matches exist.
 */
int handle_database_help(struct char_data *ch, const char *argument, const char *raw_argument, struct help_context *ctx) {
    struct help_entry_list *entries = NULL, *tmp = NULL;
    struct help_keyword_list *tmp_keyword = NULL;
    char help_entry_buffer[MAX_STRING_LENGTH] = {'\0'};
    char immo_data_buffer[1024];
    int exact_match_found = 0;
    int partial_match_displayed = 0;
    
    /* Search database for help entry */
    if ((entries = search_help(argument, GET_LEVEL(ch))) == NULL) {
        /* Debug: log when database help doesn't find anything */
        if (HELP_DEBUG) log("DEBUG: handle_database_help found no entry for '%s'", argument);
        return 0; /* Not found, let next handler try */
    }
    
    /* Check if any of the keywords is an exact match (case-insensitive) */
    if (HELP_DEBUG) log("DEBUG: handle_database_help: Checking for exact match among keywords");
    for (tmp_keyword = entries->keyword_list; tmp_keyword; tmp_keyword = tmp_keyword->next) {
        if (HELP_DEBUG) log("DEBUG: handle_database_help: Comparing '%s' with keyword '%s'", 
                           argument, tmp_keyword->keyword);
        if (strcasecmp(tmp_keyword->keyword, argument) == 0 ||
            strcasecmp(tmp_keyword->keyword, raw_argument) == 0) {
            exact_match_found = 1;
            if (HELP_DEBUG) log("DEBUG: handle_database_help: EXACT MATCH found for '%s' -> '%s'", 
                               argument, tmp_keyword->keyword);
            break;
        }
    }
    if (!exact_match_found && HELP_DEBUG) {
        log("DEBUG: handle_database_help: No exact match found, only partial matches");
    }
    
    /* If no exact match, check if this is really what user wanted */
    if (!exact_match_found) {
        /* Check if the first keyword is reasonably close to what was typed */
        /* If the search term is very short (1-2 chars), be more strict */
        if (strlen(argument) <= MIN_SEARCH_LENGTH_FOR_STRICT) {
            /* For very short searches, only show if it's a strong prefix match */
            int is_strong_match = 0;
            for (tmp_keyword = entries->keyword_list; tmp_keyword; tmp_keyword = tmp_keyword->next) {
                if (strncasecmp(tmp_keyword->keyword, argument, strlen(argument)) == 0) {
                    /* Check if the match is reasonable (not too long) */
                    if (strlen(tmp_keyword->keyword) <= strlen(argument) + MAX_KEYWORD_LENGTH_DIFF) {
                        is_strong_match = 1;
                        break;
                    }
                }
            }
            if (!is_strong_match) {
                if (HELP_DEBUG) log("DEBUG: Weak partial match for short search '%s', continuing to soundex", argument);
                /* Clean up and let soundex handle it */
                while (entries != NULL) {
                    tmp = entries;
                    entries = entries->next;
                    if (tmp->tag) free(tmp->tag);
                    if (tmp->entry) free(tmp->entry);
                    if (tmp->keywords) free(tmp->keywords);
                    if (tmp->last_updated) free(tmp->last_updated);
                    while (tmp->keyword_list != NULL) {
                        tmp_keyword = tmp->keyword_list->next;
                        if (tmp->keyword_list->tag) free(tmp->keyword_list->tag);
                        if (tmp->keyword_list->keyword) free(tmp->keyword_list->keyword);
                        free(tmp->keyword_list);
                        tmp->keyword_list = tmp_keyword;
                    }
                    free(tmp);
                }
                return 0;
            }
        }
        partial_match_displayed = 1;
        if (HELP_DEBUG) log("DEBUG: Only partial match found for '%s', will show help but continue to soundex", argument);
    }
    
    /* Format and display the help entry */
    snprintf(immo_data_buffer, sizeof(immo_data_buffer), "\tcHelp Tag      : \tn%s\r\n",
             entries->tag);
    snprintf(help_entry_buffer, sizeof(help_entry_buffer), "\tC%s\tn"
                                                           "%s"
                                                           "\tcHelp Keywords : \tn%s\r\n"
                                                           "\tcLast Updated  : \tn%s\r\n"
                                                           "\tC%s"
                                                           "\tn%s\r\n"
                                                           "\tC%s",
             line_string(GET_SCREEN_WIDTH(ch), '-', '-'),
             (IS_IMMORTAL(ch) ? immo_data_buffer : ""),
             entries->keywords,
             entries->last_updated,
             line_string(GET_SCREEN_WIDTH(ch), '-', '-'),
             entries->entry,
             line_string(GET_SCREEN_WIDTH(ch), '-', '-'));
    page_string(ch->desc, help_entry_buffer, 1);
    
    /* Clean up allocated memory */
    while (entries != NULL) {
        tmp = entries;
        entries = entries->next;
        
        /* Free strdup'd strings in help_entry_list */
        if (tmp->tag) free(tmp->tag);
        if (tmp->entry) free(tmp->entry);
        if (tmp->keywords) free(tmp->keywords);
        if (tmp->last_updated) free(tmp->last_updated);
        
        while (tmp->keyword_list != NULL) {
            tmp_keyword = tmp->keyword_list->next;
            /* Free strdup'd strings in keyword list */
            if (tmp->keyword_list->tag) free(tmp->keyword_list->tag);
            if (tmp->keyword_list->keyword) free(tmp->keyword_list->keyword);
            free(tmp->keyword_list);
            tmp->keyword_list = tmp_keyword;
        }
        
        free(tmp);
    }
    
    /* Return based on match type:
     * - Exact match: return 1 (stop chain, we found exactly what user wanted)
     * - Partial match: return 0 (continue chain to show soundex suggestions)
     * This ensures soundex suggestions appear for typos even when partial matches exist
     */
    if (exact_match_found) {
        if (HELP_DEBUG) log("DEBUG: Exact match handled, stopping chain for '%s'", argument);
        return 1; /* Exact match - stop chain */
    } else if (partial_match_displayed) {
        if (HELP_DEBUG) log("DEBUG: Partial match displayed, continuing to soundex for '%s'", argument);
        /* Set context flag for soundex handler to know partial help was displayed */
        ctx->partial_help_displayed = 1;
        return 0; /* Partial match - continue to soundex for suggestions */
    }
    return 1; /* Should not reach here, but default to handled */
}

/**
 * Handler for deity information.
 * Converts deity help requests to devote command calls.
 */
int handle_deity_help(struct char_data *ch, const char *argument, const char *raw_argument, struct help_context *ctx) {
    int i;
    char spell_argument[200];
    
    for (i = 0; i < NUM_DEITIES; i++) {
        if (is_abbrev(raw_argument, deity_list[i].name)) {
            snprintf(spell_argument, sizeof(spell_argument), "info %s", raw_argument);
            do_devote(ch, spell_argument, 0, 0);
            return 1; /* Handled */
        }
    }
    return 0; /* Not a deity */
}

/**
 * Handler for region information.
 * Handles special capitalization logic for region names.
 */
int handle_region_help(struct char_data *ch, const char *argument, const char *raw_argument, struct help_context *ctx) {
    int i;
    char *region_arg = strdup(raw_argument);
    
    /* Apply region name capitalization rules */
    for (i = 0; region_arg[i] != '\0'; i++) {
        /* First character should be capital */
        if (i == 0) {
            if ((region_arg[i] >= 'a' && region_arg[i] <= 'z'))
                region_arg[i] = region_arg[i] - 32;
            continue;
        }
        /* Character after space should be capital */
        if (region_arg[i] == ' ') {
            ++i;
            if (region_arg[i] >= 'a' && region_arg[i] <= 'z') {
                region_arg[i] = region_arg[i] - 32;
                continue;
            }
        } else {
            /* Other uppercase characters should be lowercase */
            if (region_arg[i] >= 'A' && region_arg[i] <= 'Z')
                region_arg[i] = region_arg[i] + 32;
        }
    }
    
    /* Check if it matches a region */
    for (i = 1; i < NUM_REGIONS; i++) {
        if (is_abbrev(region_arg, regions[i])) {
            display_region_info(ch, i);
            free(region_arg);
            return 1; /* Handled */
        }
    }
    
    free(region_arg);
    return 0; /* Not a region */
}

/**
 * Handler for background information.
 */
int handle_background_help(struct char_data *ch, const char *argument, const char *raw_argument, struct help_context *ctx) {
    int i;
    char *bg_arg = strdup(raw_argument);
    
    /* Apply same capitalization as regions for consistency */
    for (i = 0; bg_arg[i] != '\0'; i++) {
        if (i == 0) {
            if ((bg_arg[i] >= 'a' && bg_arg[i] <= 'z'))
                bg_arg[i] = bg_arg[i] - 32;
            continue;
        }
        if (bg_arg[i] == ' ') {
            ++i;
            if (bg_arg[i] >= 'a' && bg_arg[i] <= 'z') {
                bg_arg[i] = bg_arg[i] - 32;
                continue;
            }
        } else {
            if (bg_arg[i] >= 'A' && bg_arg[i] <= 'Z')
                bg_arg[i] = bg_arg[i] + 32;
        }
    }
    
    for (i = 1; i < NUM_BACKGROUNDS; i++) {
        if (is_abbrev(bg_arg, background_list[i].name)) {
            show_background_help(ch, i);
            free(bg_arg);
            return 1; /* Handled */
        }
    }
    
    free(bg_arg);
    return 0; /* Not a background */
}

/**
 * Handler for alchemist discoveries.
 */
int handle_discovery_help(struct char_data *ch, const char *argument, const char *raw_argument, struct help_context *ctx) {
    /* Alchemy functions require non-const char*, so we make a copy */
    char *arg_copy = strdup(raw_argument);
    int result = display_discovery_info(ch, arg_copy);
    free(arg_copy);
    return result ? 1 : 0;
}

/**
 * Handler for grand alchemist discoveries.
 */
int handle_grand_discovery_help(struct char_data *ch, const char *argument, const char *raw_argument, struct help_context *ctx) {
    /* Alchemy functions require non-const char*, so we make a copy */
    char *arg_copy = strdup(raw_argument);
    int result = display_grand_discovery_info(ch, arg_copy);
    free(arg_copy);
    return result ? 1 : 0;
}

/**
 * Handler for bomb types.
 */
int handle_bomb_types_help(struct char_data *ch, const char *argument, const char *raw_argument, struct help_context *ctx) {
    /* Alchemy functions require non-const char*, so we make a copy */
    char *arg_copy = strdup(raw_argument);
    int result = display_bomb_types(ch, arg_copy);
    free(arg_copy);
    return result ? 1 : 0;
}

/**
 * Handler for discovery types.
 */
int handle_discovery_types_help(struct char_data *ch, const char *argument, const char *raw_argument, struct help_context *ctx) {
    /* Alchemy functions require non-const char*, so we make a copy */
    char *arg_copy = strdup(raw_argument);
    int result = display_discovery_types(ch, arg_copy);
    free(arg_copy);
    return result ? 1 : 0;
}

/**
 * Handler for feat information.
 */
int handle_feat_help(struct char_data *ch, const char *argument, const char *raw_argument, struct help_context *ctx) {
    if (display_feat_info(ch, raw_argument)) {
        return 1; /* Handled */
    }
    return 0; /* Not a feat */
}

/**
 * Handler for evolution information.
 */
int handle_evolution_help(struct char_data *ch, const char *argument, const char *raw_argument, struct help_context *ctx) {
    if (display_evolution_info(ch, raw_argument)) {
        return 1; /* Handled */
    }
    return 0; /* Not an evolution */
}

/**
 * Handler for weapon information.
 */
int handle_weapon_help(struct char_data *ch, const char *argument, const char *raw_argument, struct help_context *ctx) {
    if (display_weapon_info(ch, raw_argument)) {
        return 1; /* Handled */
    }
    return 0; /* Not a weapon */
}

/**
 * Handler for armor information.
 */
int handle_armor_help(struct char_data *ch, const char *argument, const char *raw_argument, struct help_context *ctx) {
    if (display_armor_info(ch, raw_argument)) {
        return 1; /* Handled */
    }
    return 0; /* Not armor */
}

/**
 * Handler for class information.
 */
int handle_class_help(struct char_data *ch, const char *argument, const char *raw_argument, struct help_context *ctx) {
    if (display_class_info(ch, raw_argument)) {
        return 1; /* Handled */
    }
    return 0; /* Not a class */
}

/**
 * Handler for race information.
 */
int handle_race_help(struct char_data *ch, const char *argument, const char *raw_argument, struct help_context *ctx) {
    if (display_race_info(ch, raw_argument)) {
        return 1; /* Handled */
    }
    return 0; /* Not a race */
}

/**
 * Handler for soundex suggestions - always last in chain.
 * This is the fallback when no exact match is found.
 * Enhanced to work with partial matches from database handler.
 */
int handle_soundex_suggestions(struct char_data *ch, const char *argument, const char *raw_argument, struct help_context *ctx) {
    struct help_keyword_list *keywords = NULL, *tmp_keyword = NULL;
    int soundex_matches_found = 0;
    
    /* Check if database handler already displayed partial match help */
    if (!ctx->partial_help_displayed) {
        /* No partial match was shown, display the standard "not found" message */
        send_to_char(ch, "There is no help on '%s'.\r\n", raw_argument);
        mudlog(NRM, MAX(LVL_IMPL, GET_INVIS_LEV(ch)), TRUE,
              "%s tried to get help on '%s'", GET_NAME(ch), raw_argument);
    } else {
        /* Partial match was shown, add a note about it */
        send_to_char(ch, "\r\n\tcNote: Showing closest partial match above.\tn\r\n");
        if (HELP_DEBUG) log("DEBUG: Partial help was displayed, adding soundex suggestions for '%s'", raw_argument);
    }
    
    /* Try soundex search for suggestions - use raw_argument not modified argument */
    if ((keywords = soundex_search_help_keywords(raw_argument, GET_LEVEL(ch))) != NULL) {
        soundex_matches_found = 1;
        if (ctx->partial_help_displayed) {
            /* If we showed partial match, phrase differently */
            send_to_char(ch, "\r\nPerhaps you meant one of these:\r\n");
        } else {
            /* Standard "did you mean" for no matches at all */
            send_to_char(ch, "\r\nDid you mean:\r\n");
        }
        tmp_keyword = keywords;
        while (tmp_keyword != NULL) {
            send_to_char(ch, "  \t<send href=\"Help %s\">%s\t</send>\r\n",
                        tmp_keyword->keyword, tmp_keyword->keyword);
            tmp_keyword = tmp_keyword->next;
        }
        send_to_char(ch, "\tDYou can also check the help index, type 'hindex <keyword>'\tn\r\n");
        
        /* Clean up keyword list */
        while (keywords != NULL) {
            tmp_keyword = keywords->next;
            /* Free strdup'd strings in keyword list */
            if (keywords->tag) free(keywords->tag);
            if (keywords->keyword) free(keywords->keyword);
            free(keywords);
            keywords = tmp_keyword;
            tmp_keyword = NULL;
        }
    }
    
    /* Log soundex results for debugging */
    if (HELP_DEBUG) {
        if (soundex_matches_found) {
            log("DEBUG: handle_soundex_suggestions: Found soundex matches for '%s'", raw_argument);
        } else {
            log("DEBUG: handle_soundex_suggestions: No soundex suggestions found for '%s'", raw_argument);
        }
        log("DEBUG: handle_soundex_suggestions: Partial help was %sdisplayed", 
            ctx->partial_help_displayed ? "" : "NOT ");
    }
    
    return 1; /* Always returns handled as this is the final fallback */
}

/**
 * Main help command implementation - refactored using Chain of Responsibility pattern.
 * 
 * This refactored implementation:
 * - Reduces function size from 230+ lines to ~50 lines
 * - Eliminates deep nesting (was 8+ levels, now max 2)
 * - Centralizes memory management (single free point)
 * - Makes adding new help sources trivial
 * 
 * The handler chain processes help requests in priority order,
 * with each handler responsible for one type of help content.
 */
ACMDU(do_help)
{
  struct help_handler *handler;
  struct help_context ctx = {0};  /* Initialize context with zeros */
  char *raw_argument;
  char argument_copy[MAX_INPUT_LENGTH];
  
  /* Basic validation */
  if (!ch->desc)
    return;
  
  /* Handle empty help request */
  skip_spaces(&argument);
  if (!*argument) {
    /* Display appropriate help screen based on level */
    if (GET_LEVEL(ch) < LVL_IMMORT)
      page_string(ch->desc, help, 0);
    else
      page_string(ch->desc, ihelp, 0);
    return;
  }
  
  /* Prepare arguments for handlers */
  raw_argument = strdup(argument);    /* Keep original for handlers that need it */
  if (!raw_argument) {
    log("SYSERR: do_help: Memory allocation failed for argument '%s'", argument);
    send_to_char(ch, "Help system error: memory allocation failed.\r\n");
    return;
  }
  strlcpy(argument_copy, argument, sizeof(argument_copy));
  space_to_minus(argument_copy);      /* Convert spaces to dashes for database search */
  
  /* Initialize handler chain if not already done */
  if (!help_handler_chain) {
    init_help_handlers();
  }
  
  /* Process through handler chain until one handles the request */
  for (handler = help_handler_chain; handler; handler = handler->next) {
    /* Debug logging to trace handler execution */
    if (HELP_DEBUG) log("DEBUG: do_help: Trying handler '%s' for '%s' (raw: '%s')", 
                       handler->name, argument_copy, raw_argument);
    if (handler->handler(ch, argument_copy, raw_argument, &ctx)) {
      /* Handler processed the request successfully */
      if (HELP_DEBUG) log("DEBUG: do_help: Handler '%s' HANDLED request for '%s' - stopping chain", 
                         handler->name, argument_copy);
      free(raw_argument);
      return;
    }
    if (HELP_DEBUG) log("DEBUG: do_help: Handler '%s' DID NOT HANDLE '%s' - continuing chain", 
                       handler->name, argument_copy);
  }
  
  /* This should never happen as soundex handler always returns 1 */
  send_to_char(ch, "Help system error: no handlers available.\r\n");
  mudlog(NRM, LVL_IMPL, TRUE, "SYSERR: Help handler chain is broken!");
  free(raw_argument);
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
  struct help_cache_entry *new_entry, *entry, *oldest;
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
 * Clear the entire help cache.
 * Used when help entries are updated to ensure fresh data.
 */
void clear_help_cache(void)
{
  struct help_cache_entry *entry, *next;
  
  for (entry = help_cache; entry != NULL; entry = next) {
    next = entry->next;
    free(entry->search_key);
    free_help_entry_list(entry->result);
    free(entry);
  }
  
  help_cache = NULL;
  help_cache_count = 0;
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
      new_keyword = alloc_help_keyword();
      if (!new_keyword) {
        log("SYSERR: deep_copy_help_list: Failed to allocate keyword");
        continue;
      }
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
 * Allocates memory for a new help entry.
 * Centralizes allocation to ensure consistent initialization.
 * 
 * @return Pointer to allocated help_entry_list or NULL on failure
 */
static struct help_entry_list *alloc_help_entry(void)
{
  struct help_entry_list *entry;
  
  CREATE(entry, struct help_entry_list, 1);
  if (!entry) {
    log("SYSERR: alloc_help_entry: Failed to allocate memory for help entry");
    return NULL;
  }
  
  /* Initialize all fields to NULL/0 for safe cleanup */
  entry->tag = NULL;
  entry->keywords = NULL;
  entry->entry = NULL;
  entry->min_level = 0;
  entry->last_updated = NULL;
  entry->keyword_list = NULL;
  entry->next = NULL;
  
  return entry;
}

/**
 * Allocates memory for a new help keyword.
 * Centralizes allocation to ensure consistent initialization.
 * 
 * @return Pointer to allocated help_keyword_list or NULL on failure
 */
static struct help_keyword_list *alloc_help_keyword(void)
{
  struct help_keyword_list *keyword;
  
  CREATE(keyword, struct help_keyword_list, 1);
  if (!keyword) {
    log("SYSERR: alloc_help_keyword: Failed to allocate memory for help keyword");
    return NULL;
  }
  
  /* Initialize all fields to NULL */
  keyword->tag = NULL;
  keyword->keyword = NULL;
  keyword->next = NULL;
  
  return keyword;
}

/**
 * Frees a single help entry and all its associated memory.
 * This is the centralized cleanup function for a single entry.
 * 
 * @param entry The help entry to free (not the whole list)
 */
static void free_help_entry_single(struct help_entry_list *entry)
{
  if (!entry)
    return;
  
  /* Free all string fields */
  if (entry->tag) {
    free(entry->tag);
    entry->tag = NULL;
  }
  if (entry->entry) {
    free(entry->entry);
    entry->entry = NULL;
  }
  if (entry->keywords) {
    free(entry->keywords);
    entry->keywords = NULL;
  }
  if (entry->last_updated) {
    free(entry->last_updated);
    entry->last_updated = NULL;
  }
  
  /* Free keyword list */
  free_help_keyword_list(entry->keyword_list);
  entry->keyword_list = NULL;
  
  /* Do not free entry->next - that's handled by the list traversal */
}

/**
 * Frees a help keyword list and all associated memory.
 * 
 * @param keyword_list The keyword list to free
 */
static void free_help_keyword_list(struct help_keyword_list *keyword_list)
{
  struct help_keyword_list *keyword, *tmp_keyword;
  
  keyword = keyword_list;
  while (keyword != NULL) {
    tmp_keyword = keyword;
    keyword = keyword->next;
    
    /* Free string fields */
    if (tmp_keyword->tag) {
      free(tmp_keyword->tag);
      tmp_keyword->tag = NULL;
    }
    if (tmp_keyword->keyword) {
      free(tmp_keyword->keyword);
      tmp_keyword->keyword = NULL;
    }
    
    /* Free the structure itself */
    free(tmp_keyword);
  }
}

/**
 * Frees a help_entry_list and all associated memory.
 * This is the main cleanup function for help entry lists.
 * Uses centralized single-entry cleanup to avoid duplication.
 * 
 * @param entry The help list to free
 */
static void free_help_entry_list(struct help_entry_list *entry)
{
  struct help_entry_list *tmp;
  
  while (entry != NULL) {
    tmp = entry;
    entry = entry->next;
    
    /* Use centralized cleanup */
    free_help_entry_single(tmp);
    
    /* Free the structure itself */
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

/**
 * HELPSEARCH command - Full-text search within help content.
 * Searches for the specified text within help entry content, not just keywords.
 * 
 * Usage: helpsearch <search term>
 * 
 * This command allows players to find help entries that contain specific
 * text within their content, making it easier to find information when
 * you don't know the exact keyword.
 */
ACMDU(do_helpsearch)
{
  struct help_entry_list *results = NULL, *entry = NULL;
  char search_term[MAX_INPUT_LENGTH];
  int count = 0;
  
  /* Basic validation */
  if (!ch->desc)
    return;
  
  /* Get search term */
  skip_spaces(&argument);
  if (!*argument) {
    send_to_char(ch, "Usage: helpsearch <search term>\r\n");
    send_to_char(ch, "Example: helpsearch damage reduction\r\n");
    send_to_char(ch, "This searches for the text within help entries, not just keywords.\r\n");
    return;
  }
  
  strlcpy(search_term, argument, sizeof(search_term));
  
  /* Perform fulltext search */
  results = search_help_fulltext(search_term, GET_LEVEL(ch));
  
  if (results == NULL) {
    send_to_char(ch, "No help entries found containing '%s'.\r\n", search_term);
    mudlog(NRM, MAX(LVL_IMPL, GET_INVIS_LEV(ch)), TRUE,
          "%s searched help content for '%s' (no results)", GET_NAME(ch), search_term);
    return;
  }
  
  /* Display results header */
  send_to_char(ch, "\tC%s\tn\r\n", line_string(GET_SCREEN_WIDTH(ch), '-', '-'));
  send_to_char(ch, "\tcHelp entries containing '%s':\tn\r\n", search_term);
  send_to_char(ch, "\tC%s\tn\r\n", line_string(GET_SCREEN_WIDTH(ch), '-', '-'));
  
  /* Display each matching entry */
  for (entry = results; entry != NULL; entry = entry->next) {
    count++;
    
    /* Show entry tag and keywords */
    send_to_char(ch, "\r\n\tY%d.\tn \t<send href=\"Help %s\">%s\t</send>\r\n", 
                count, entry->tag, entry->tag);
    
    if (entry->keywords && *entry->keywords) {
      send_to_char(ch, "   \tcKeywords:\tn %s\r\n", entry->keywords);
    }
    
    /* Show a preview of the content (first 200 chars) */
    if (entry->entry && *entry->entry) {
      char preview[201];
      strlcpy(preview, entry->entry, sizeof(preview));
      
      /* Remove color codes for cleaner preview */
      char *p = preview;
      while (*p) {
        if (*p == '\t' && *(p+1)) {
          /* Skip color code */
          p += 2;
        } else if (*p == '\r' || *p == '\n') {
          *p = ' ';  /* Replace newlines with spaces */
          p++;
        } else {
          p++;
        }
      }
      
      /* Trim preview at word boundary if possible */
      if (strlen(entry->entry) > 200) {
        char *last_space = strrchr(preview, ' ');
        if (last_space && (last_space - preview) > 150) {
          strcpy(last_space, "...");
        } else {
          strcat(preview, "...");
        }
      }
      
      send_to_char(ch, "   \tdPreview:\tn %.200s\r\n", preview);
    }
  }
  
  /* Display results footer */
  send_to_char(ch, "\r\n\tC%s\tn\r\n", line_string(GET_SCREEN_WIDTH(ch), '-', '-'));
  send_to_char(ch, "Found \tY%d\tn help entries containing '%s'.\r\n", count, search_term);
  send_to_char(ch, "Click on any entry name to view its full help text.\r\n");
  
  /* Log the search */
  mudlog(NRM, MAX(LVL_IMPL, GET_INVIS_LEV(ch)), TRUE,
        "%s searched help content for '%s' (%d results)", GET_NAME(ch), search_term, count);
  
  /* Clean up results using centralized function */
  free_help_entry_list(results);
}
