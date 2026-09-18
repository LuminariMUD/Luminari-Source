/* Regression for issue #204: tokens in help_entries.alternate_keywords are lookup
 * keywords, both for the direct lookup and for the miss suggestions, while a real
 * help_keywords row still wins when an alias collides with it. The tables are
 * shadowed with TEMPORARY copies on the explicitly configured test database. */

#include "CuTest.h"

#include "conf.h"
#include "../../src/core/sysdep.h"
#include "../../src/core/structs.h"
#include "../../src/core/utils.h"
#include "../../src/core/help.h"
#include "../../src/database/mysql.h"

/** Connect to the explicitly configured test database, as test_database_persistence.c does. */
static MYSQL *help_alias_open_test_database(void)
{
  const char *host = getenv("LUMINARI_TEST_MYSQL_HOST");
  const char *user = getenv("LUMINARI_TEST_MYSQL_USER");
  const char *password = getenv("LUMINARI_TEST_MYSQL_PASSWORD");
  const char *database = getenv("LUMINARI_TEST_MYSQL_DATABASE");
  const char *port_text = getenv("LUMINARI_TEST_MYSQL_PORT");
  MYSQL *connection;

  if (host == NULL || user == NULL || password == NULL || database == NULL)
    return NULL;
  connection = mysql_init(NULL);
  if (connection == NULL)
    return NULL;
  if (mysql_real_connect(connection, host, user, password, database,
                         port_text != NULL ? (unsigned int)strtoul(port_text, NULL, 10) : 3306U,
                         NULL, 0) == NULL)
  {
    mysql_close(connection);
    return NULL;
  }
  return connection;
}

/** Shadow the help tables with a catalog where LOGS is only an alias of SYSLOG while LOGSEARCH
 * is a lower-level keyword that starts with it, and GOSSIP is a keyword of CHAT but an alias of
 * AUCTION. */
static void help_alias_shadow_tables(CuTest *tc, MYSQL *connection)
{
  const char *queries[] = {
      "CREATE TEMPORARY TABLE help_entries (tag VARCHAR(50) NOT NULL, "
      "alternate_keywords TEXT DEFAULT NULL, entry LONGTEXT NOT NULL, min_level INT DEFAULT 0, "
      "last_updated TIMESTAMP DEFAULT CURRENT_TIMESTAMP, PRIMARY KEY (tag))",
      "CREATE TEMPORARY TABLE help_keywords (help_tag VARCHAR(50) NOT NULL, "
      "keyword VARCHAR(100) NOT NULL, PRIMARY KEY (help_tag, keyword))",
      "CREATE TEMPORARY TABLE help_search_history (search_term VARCHAR(200), "
      "searcher_level INT, results_count INT, search_type VARCHAR(16))",
      "INSERT INTO help_entries (tag, alternate_keywords, entry, min_level) VALUES "
      "('syslog', 'SYSLOG-OFF LOGS AUTOLOG', 'The syslog entry.', 31), "
      "('auction', 'GOSSIP GRATS', 'The auction entry.', 1), "
      "('chat', NULL, 'The chat entry.', 1), "
      "('logsearch', NULL, 'The logsearch entry.', 1)",
      "INSERT INTO help_keywords (help_tag, keyword) VALUES "
      "('syslog', 'SYSLOG'), ('auction', 'AUCTION'), ('chat', 'CHAT'), ('chat', 'GOSSIP'), "
      "('logsearch', 'LOGSEARCH')",
  };
  size_t i;

  for (i = 0; i < sizeof(queries) / sizeof(queries[0]); i++)
    CuAssertIntEquals_Msg(tc, queries[i], 0, mysql_query(connection, queries[i]));
}

static void help_alias_free_entries(struct help_entry_list *entries)
{
  struct help_entry_list *next;
  struct help_keyword_list *keyword, *next_keyword;

  while (entries != NULL)
  {
    next = entries->next;
    free(entries->tag);
    free(entries->entry);
    free(entries->keywords);
    free(entries->last_updated);
    for (keyword = entries->keyword_list; keyword != NULL; keyword = next_keyword)
    {
      next_keyword = keyword->next;
      free(keyword->tag);
      free(keyword->keyword);
      free(keyword);
    }
    free(entries);
    entries = next;
  }
}

static void help_alias_free_keywords(struct help_keyword_list *keywords)
{
  struct help_keyword_list *next;

  while (keywords != NULL)
  {
    next = keywords->next;
    free(keywords->tag);
    free(keywords->keyword);
    free(keywords);
    keywords = next;
  }
}

/* The tag of the entry at a list position, or NULL when the list is shorter. */
static const char *help_alias_tag_at(const struct help_entry_list *entries, int index)
{
  while (entries != NULL && index > 0)
  {
    entries = entries->next;
    index--;
  }
  return entries != NULL ? entries->tag : NULL;
}

/* The displayed keyword line of the first entry, or an empty string without one. */
static const char *help_alias_keyword_line(const struct help_entry_list *entries)
{
  return entries != NULL && entries->keywords != NULL ? entries->keywords : "";
}

static bool help_alias_suggests(const struct help_keyword_list *keywords, const char *keyword)
{
  for (; keywords != NULL; keywords = keywords->next)
    if (keywords->keyword != NULL && str_cmp(keywords->keyword, keyword) == 0)
      return true;
  return false;
}

/* An alias-only token reaches its entry ahead of a keyword that merely starts with it, the
 * displayed keyword line lists the alias so the exact-match check accepts it, a colliding real
 * keyword outranks the alias, and a misspelled alias is suggested. */
void Test_help_alias_tokens_are_lookup_keywords(CuTest *tc)
{
  const char *enabled = getenv("LUMINARI_TEST_MYSQL_ENABLE");
  MYSQL *saved_conn = conn;
  bool saved_available = mysql_available;
  MYSQL *connection;
  struct help_entry_list *entries;
  struct help_keyword_list *suggestions;

  if (enabled == NULL || strcmp(enabled, "1") != 0)
    return;

  connection = help_alias_open_test_database();
  CuAssertPtrNotNull(tc, connection);
  help_alias_shadow_tables(tc, connection);

  conn = connection;
  mysql_available = true;
  clear_help_cache();

  entries = search_help("logs", LVL_IMPL);
  CuAssertStrEquals(tc, "syslog", help_alias_tag_at(entries, 0));
  CuAssertStrEquals(tc, "logsearch", help_alias_tag_at(entries, 1));
  CuAssertTrue(tc, strstr(help_alias_keyword_line(entries), "Logs") != NULL);
  CuAssertTrue(tc, strstr(help_alias_keyword_line(entries), "Syslog") != NULL);
  help_alias_free_entries(entries);

  /* Below the entry's level the alias is not reachable; only the prefix keyword remains. */
  clear_help_cache();
  entries = search_help("logs", 1);
  CuAssertStrEquals(tc, "logsearch", help_alias_tag_at(entries, 0));
  CuAssertTrue(tc, help_alias_tag_at(entries, 1) == NULL);
  help_alias_free_entries(entries);

  clear_help_cache();
  entries = search_help("gossip", 1);
  CuAssertStrEquals(tc, "chat", help_alias_tag_at(entries, 0));
  CuAssertStrEquals(tc, "auction", help_alias_tag_at(entries, 1));
  help_alias_free_entries(entries);

  suggestions = help_lookup_suggestions("lgos", LVL_IMPL);
  CuAssertTrue(tc, help_alias_suggests(suggestions, "LOGS"));
  help_alias_free_keywords(suggestions);

  suggestions = help_lookup_suggestions("grat", 1);
  CuAssertTrue(tc, help_alias_suggests(suggestions, "GRATS"));
  help_alias_free_keywords(suggestions);

  clear_help_cache();
  mysql_available = saved_available;
  conn = saved_conn;
  mysql_close(connection);
}
