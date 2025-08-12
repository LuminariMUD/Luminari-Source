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

/* Debug flag for help system - set to 1 to enable debug logging, 0 to disable
 * This will log detailed information about:
 * - Database connection status
 * - SQL query preparation and execution
 * - Number of results returned
 * - Handler chain execution
 * - Cache hits/misses
 * - Fallback to file-based help
 */
#define HELP_DEBUG 1

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
      CREATE(new_entry, struct help_entry_list, 1);
      
      /* Set entry data from file-based help */
      new_entry->tag = strdup(help_table[i].keywords);
      new_entry->entry = strdup(help_table[i].entry ? help_table[i].entry : "No help available.\r\n");
      new_entry->min_level = help_table[i].min_level;
      new_entry->last_updated = strdup("File-based");
      new_entry->keywords = strdup(help_table[i].keywords);
      new_entry->keyword_list = NULL;
      new_entry->next = NULL;
      
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
    if (HELP_DEBUG) log("DEBUG: search_help: ERROR - No database connection!");
    return search_help_table(argument, level);
  }
  
  /* Ensure connection is still alive */
  if (mysql_ping(conn) != 0) {
    if (HELP_DEBUG) log("DEBUG: search_help: MySQL ping failed, connection lost");
    return search_help_table(argument, level);
  }
  if (HELP_DEBUG) log("DEBUG: search_help: Database connection OK");

  /* Create prepared statement for secure query execution */
  pstmt = mysql_stmt_create(conn);
  if (!pstmt) {
    log("SYSERR: Failed to create prepared statement for help search");
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
    log("SYSERR: Failed to prepare help search query");
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
    log("SYSERR: Failed to bind parameters for help search");
    if (HELP_DEBUG) log("DEBUG: search_help: Failed to bind parameters, falling back to file-based");
    mysql_stmt_cleanup(pstmt);
    return search_help_table(argument, level);
  }
  if (HELP_DEBUG) log("DEBUG: search_help: Parameters bound (pattern='%s', level=%d)", search_pattern, level);

  /* Execute the prepared statement */
  if (!mysql_stmt_execute_prepared(pstmt)) {
    log("SYSERR: Failed to execute help search query");
    if (HELP_DEBUG) log("DEBUG: search_help: Query execution failed, falling back to file-based");
    mysql_stmt_cleanup(pstmt);
    return search_help_table(argument, level);
  }
  if (HELP_DEBUG) log("DEBUG: search_help: Query executed successfully");

  /* Fetch results row by row */
  while (mysql_stmt_fetch_row(pstmt)) {
    row_count++;
    if (HELP_DEBUG && row_count == 1) log("DEBUG: search_help: Found database results for '%s'", argument);
    
    /* Allocate memory for the help entry data */
    CREATE(new_help_entry, struct help_entry_list, 1);
    
    /* Get column values safely from prepared statement results */
    new_help_entry->tag = strdup(mysql_stmt_get_string(pstmt, 0) ? mysql_stmt_get_string(pstmt, 0) : "");
    new_help_entry->entry = strdup(mysql_stmt_get_string(pstmt, 1) ? mysql_stmt_get_string(pstmt, 1) : "");
    new_help_entry->min_level = mysql_stmt_get_int(pstmt, 2);
    new_help_entry->last_updated = strdup(mysql_stmt_get_string(pstmt, 3) ? mysql_stmt_get_string(pstmt, 3) : "");
    new_help_entry->keywords = strdup(mysql_stmt_get_string(pstmt, 4) ? mysql_stmt_get_string(pstmt, 4) : "");

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
    CREATE(new_keyword, struct help_keyword_list, 1);
    new_keyword->tag = strdup(mysql_stmt_get_string(pstmt, 0) ? mysql_stmt_get_string(pstmt, 0) : "");
    new_keyword->keyword = strdup(mysql_stmt_get_string(pstmt, 1) ? mysql_stmt_get_string(pstmt, 1) : "");
    new_keyword->next = NULL;

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
   * This helps users find help entries when they don't know exact spelling */
  if (!mysql_stmt_prepare_query(pstmt,
      "SELECT hk.help_tag, hk.keyword "
      "FROM help_entries he, help_keywords hk "
      "WHERE he.tag = hk.help_tag "
      "AND hk.keyword SOUNDS LIKE ? "
      "AND he.min_level <= ? "
      "ORDER BY LENGTH(hk.keyword) ASC")) {
    log("SYSERR: Failed to prepare soundex search query");
    if (HELP_DEBUG) log("DEBUG: soundex_search_help_keywords: Failed to prepare query");
    mysql_stmt_cleanup(pstmt);
    return NULL;
  }
  if (HELP_DEBUG) log("DEBUG: soundex_search_help_keywords: Query prepared successfully");

  /* Bind parameters - SQL injection safe */
  if (!mysql_stmt_bind_param_string(pstmt, 0, argument) ||
      !mysql_stmt_bind_param_int(pstmt, 1, level)) {
    log("SYSERR: Failed to bind parameters for soundex search");
    if (HELP_DEBUG) log("DEBUG: soundex_search_help_keywords: Failed to bind parameters (argument='%s', level=%d)", argument, level);
    mysql_stmt_cleanup(pstmt);
    return NULL;
  }
  if (HELP_DEBUG) log("DEBUG: soundex_search_help_keywords: Parameters bound (argument='%s', level=%d)", argument, level);

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
    
    /* Allocate memory for the keyword data */
    CREATE(new_keyword, struct help_keyword_list, 1);
    new_keyword->tag = strdup(mysql_stmt_get_string(pstmt, 0) ? mysql_stmt_get_string(pstmt, 0) : "");
    new_keyword->keyword = strdup(mysql_stmt_get_string(pstmt, 1) ? mysql_stmt_get_string(pstmt, 1) : "");
    new_keyword->next = NULL;

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
 */
int handle_database_help(struct char_data *ch, const char *argument, const char *raw_argument) {
    struct help_entry_list *entries = NULL, *tmp = NULL;
    struct help_keyword_list *tmp_keyword = NULL;
    char help_entry_buffer[MAX_STRING_LENGTH] = {'\0'};
    char immo_data_buffer[1024];
    
    /* Search database for help entry */
    if ((entries = search_help(argument, GET_LEVEL(ch))) == NULL) {
        /* Debug: log when database help doesn't find anything */
        if (HELP_DEBUG) log("DEBUG: handle_database_help found no entry for '%s'", argument);
        return 0; /* Not found, let next handler try */
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
    
    return 1; /* Handled successfully */
}

/**
 * Handler for deity information.
 * Converts deity help requests to devote command calls.
 */
int handle_deity_help(struct char_data *ch, const char *argument, const char *raw_argument) {
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
int handle_region_help(struct char_data *ch, const char *argument, const char *raw_argument) {
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
int handle_background_help(struct char_data *ch, const char *argument, const char *raw_argument) {
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
int handle_discovery_help(struct char_data *ch, const char *argument, const char *raw_argument) {
    /* Alchemy functions require non-const char*, so we make a copy */
    char *arg_copy = strdup(raw_argument);
    int result = display_discovery_info(ch, arg_copy);
    free(arg_copy);
    return result ? 1 : 0;
}

/**
 * Handler for grand alchemist discoveries.
 */
int handle_grand_discovery_help(struct char_data *ch, const char *argument, const char *raw_argument) {
    /* Alchemy functions require non-const char*, so we make a copy */
    char *arg_copy = strdup(raw_argument);
    int result = display_grand_discovery_info(ch, arg_copy);
    free(arg_copy);
    return result ? 1 : 0;
}

/**
 * Handler for bomb types.
 */
int handle_bomb_types_help(struct char_data *ch, const char *argument, const char *raw_argument) {
    /* Alchemy functions require non-const char*, so we make a copy */
    char *arg_copy = strdup(raw_argument);
    int result = display_bomb_types(ch, arg_copy);
    free(arg_copy);
    return result ? 1 : 0;
}

/**
 * Handler for discovery types.
 */
int handle_discovery_types_help(struct char_data *ch, const char *argument, const char *raw_argument) {
    /* Alchemy functions require non-const char*, so we make a copy */
    char *arg_copy = strdup(raw_argument);
    int result = display_discovery_types(ch, arg_copy);
    free(arg_copy);
    return result ? 1 : 0;
}

/**
 * Handler for feat information.
 */
int handle_feat_help(struct char_data *ch, const char *argument, const char *raw_argument) {
    if (display_feat_info(ch, raw_argument)) {
        return 1; /* Handled */
    }
    return 0; /* Not a feat */
}

/**
 * Handler for evolution information.
 */
int handle_evolution_help(struct char_data *ch, const char *argument, const char *raw_argument) {
    if (display_evolution_info(ch, raw_argument)) {
        return 1; /* Handled */
    }
    return 0; /* Not an evolution */
}

/**
 * Handler for weapon information.
 */
int handle_weapon_help(struct char_data *ch, const char *argument, const char *raw_argument) {
    if (display_weapon_info(ch, raw_argument)) {
        return 1; /* Handled */
    }
    return 0; /* Not a weapon */
}

/**
 * Handler for armor information.
 */
int handle_armor_help(struct char_data *ch, const char *argument, const char *raw_argument) {
    if (display_armor_info(ch, raw_argument)) {
        return 1; /* Handled */
    }
    return 0; /* Not armor */
}

/**
 * Handler for class information.
 */
int handle_class_help(struct char_data *ch, const char *argument, const char *raw_argument) {
    if (display_class_info(ch, raw_argument)) {
        return 1; /* Handled */
    }
    return 0; /* Not a class */
}

/**
 * Handler for race information.
 */
int handle_race_help(struct char_data *ch, const char *argument, const char *raw_argument) {
    if (display_race_info(ch, raw_argument)) {
        return 1; /* Handled */
    }
    return 0; /* Not a race */
}

/**
 * Handler for soundex suggestions - always last in chain.
 * This is the fallback when no exact match is found.
 */
int handle_soundex_suggestions(struct char_data *ch, const char *argument, const char *raw_argument) {
    struct help_keyword_list *keywords = NULL, *tmp_keyword = NULL;
    
    /* Log failed help request */
    send_to_char(ch, "There is no help on that word.\r\n");
    mudlog(NRM, MAX(LVL_IMPL, GET_INVIS_LEV(ch)), TRUE,
          "%s tried to get help on %s", GET_NAME(ch), raw_argument);
    
    /* Try soundex search for suggestions - use raw_argument not modified argument */
    if ((keywords = soundex_search_help_keywords(raw_argument, GET_LEVEL(ch))) != NULL) {
        send_to_char(ch, "\r\nDid you mean:\r\n");
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
  strlcpy(argument_copy, argument, sizeof(argument_copy));
  space_to_minus(argument_copy);      /* Convert spaces to dashes for database search */
  
  /* Initialize handler chain if not already done */
  if (!help_handler_chain) {
    init_help_handlers();
  }
  
  /* Process through handler chain until one handles the request */
  for (handler = help_handler_chain; handler; handler = handler->next) {
    /* Debug logging to trace handler execution */
    if (HELP_DEBUG) log("DEBUG: Trying handler '%s' for '%s'", handler->name, argument_copy);
    if (handler->handler(ch, argument_copy, raw_argument)) {
      /* Handler processed the request successfully */
      if (HELP_DEBUG) log("DEBUG: Handler '%s' handled request for '%s'", handler->name, argument_copy);
      free(raw_argument);
      return;
    }
    if (HELP_DEBUG) log("DEBUG: Handler '%s' did not handle '%s'", handler->name, argument_copy);
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
