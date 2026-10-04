/* The two stores a vessel purchase or payout writes (vessels-ships study S15),
 * for the tests that buy and sell: the test database as the game's
 * connection, and a scratch player directory so a captain's saves succeed. */

#include "test_vessel_stores.h"

#include "../../src/core/utils.h"
#include "../../src/vessels/vessels.h"

#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

/* The directories a player file can lie in, by the first letter of the name. */
static const char *const pfile_dirs[] = {"plrfiles/A-E", "plrfiles/F-J", "plrfiles/K-O",
                                         "plrfiles/P-T", "plrfiles/U-Z", "plrfiles/ZZZ"};

void vessel_test_pfile_begin(CuTest *tc, struct vessel_test_stores *stores,
                             struct char_data *captain)
{
  size_t i;

  stores->captain = captain;
  memset(stores->index, 0, sizeof(stores->index));
  strlcpy(stores->scratch, "/tmp/luminari-vessel-stores-XXXXXX", sizeof(stores->scratch));
  CuAssertPtrNotNull(tc, getcwd(stores->home, sizeof(stores->home)));
  CuAssertPtrNotNull(tc, mkdtemp(stores->scratch));
  CuAssertIntEquals(tc, 0, chdir(stores->scratch));
  CuAssertIntEquals(tc, 0, mkdir("plrfiles", 0700));
  for (i = 0; i < sizeof(pfile_dirs) / sizeof(pfile_dirs[0]); i++)
  {
    CuAssertIntEquals(tc, 0, mkdir(pfile_dirs[i], 0700));
  }
  stores->index[0].name = GET_NAME(captain);
  stores->index[0].id = 4251;
  stores->saved_table = player_table;
  stores->saved_top = top_of_p_table;
  player_table = stores->index;
  top_of_p_table = 0;
  GET_PFILEPOS(captain) = 0;
  CuAssertTrue(tc, save_char_checked(captain, 0));
}

void vessel_test_pfile_end(CuTest *tc, struct vessel_test_stores *stores)
{
  char filename[MAX_FILEPATH];
  size_t i;

  if (get_filename(filename, sizeof(filename), PLR_FILE, GET_NAME(stores->captain)))
  {
    unlink(filename);
  }
  unlink("plrfiles/index");
  for (i = 0; i < sizeof(pfile_dirs) / sizeof(pfile_dirs[0]); i++)
  {
    rmdir(pfile_dirs[i]);
  }
  rmdir("plrfiles");
  player_table = stores->saved_table;
  top_of_p_table = stores->saved_top;
  CuAssertIntEquals(tc, 0, chdir(stores->home));
  rmdir(stores->scratch);
}

/* Delete the hull's rows: her other tables reference her interior row. */
static void stores_forget_hull(const struct vessel_test_stores *stores)
{
  char query[128];

  snprintf(query, sizeof(query), "DELETE FROM ship_interiors WHERE ship_id = %d", stores->shipnum);
  mysql_query(stores->connection, query);
}

bool vessel_test_stores_begin(CuTest *tc, struct vessel_test_stores *stores, int shipnum,
                              struct char_data *captain)
{
  const char *enabled = getenv("LUMINARI_TEST_MYSQL_ENABLE");
  const char *port_text = getenv("LUMINARI_TEST_MYSQL_PORT");
  my_bool reconnect = 1;
  char query[128];

  if (enabled == NULL || strcmp(enabled, "1") != 0)
  {
    return FALSE;
  }
  memset(stores, 0, sizeof(*stores));
  stores->shipnum = shipnum;
  stores->connection = mysql_init(NULL);
  CuAssertPtrNotNull(tc, stores->connection);
  mysql_options(stores->connection, MYSQL_OPT_RECONNECT, (const char *)&reconnect);
  if (mysql_real_connect(stores->connection, getenv("LUMINARI_TEST_MYSQL_HOST"),
                         getenv("LUMINARI_TEST_MYSQL_USER"), getenv("LUMINARI_TEST_MYSQL_PASSWORD"),
                         getenv("LUMINARI_TEST_MYSQL_DATABASE"),
                         port_text != NULL ? (unsigned int)strtoul(port_text, NULL, 10) : 3306,
                         NULL, 0) == NULL)
  {
    mysql_close(stores->connection);
    CuFail(tc, "could not connect to the explicitly configured test database");
  }
  stores->saved_conn = conn;
  stores->saved_mysql_available = mysql_available;
  conn = stores->connection;
  mysql_available = TRUE;

  /* What boot adds to a database made from master_schema.sql alone. */
  vessel_persistence_ensure_schema();
  vessel_ownership_ensure_schema();
  vessel_piracy_ensure_schema();
  stores_forget_hull(stores);
  snprintf(query, sizeof(query), "INSERT INTO ship_interiors (ship_id) VALUES (%d)", shipnum);
  CuAssertIntEquals(tc, 0, mysql_query(stores->connection, query));

  vessel_test_pfile_begin(tc, stores, captain);
  return TRUE;
}

void vessel_test_stores_end(CuTest *tc, struct vessel_test_stores *stores)
{
  mysql_test_drop_connection_at(NULL, 0, FALSE);
  vessel_test_pfile_end(tc, stores);
  stores_forget_hull(stores);
  conn = stores->saved_conn;
  mysql_available = stores->saved_mysql_available;
  mysql_close(stores->connection);
}

void vessel_test_database_away(struct vessel_test_stores *stores, bool away)
{
  static char no_socket[] = "/nonexistent/vessel-stores.sock";

  if (away)
  {
    stores->saved_port = stores->connection->port;
    stores->saved_unix_socket = stores->connection->unix_socket;
    stores->connection->port = 1;
    stores->connection->unix_socket = no_socket;
  }
  else
  {
    stores->connection->port = stores->saved_port;
    stores->connection->unix_socket = stores->saved_unix_socket;
  }
}

int vessel_test_file_gold(CuTest *tc, const struct vessel_test_stores *stores)
{
  struct char_data *loaded;
  int gold;

  loaded = new_char();
  CuAssertTrue(tc, load_char(GET_NAME(stores->captain), loaded) >= 0);
  gold = GET_GOLD(loaded);
  free_char(loaded);
  return gold;
}

long long vessel_test_number(CuTest *tc, const struct vessel_test_stores *stores, const char *query)
{
  MYSQL_RES *result;
  MYSQL_ROW row;
  long long number;

  CuAssertIntEquals(tc, 0, mysql_query(stores->connection, query));
  result = mysql_store_result(stores->connection);
  CuAssertPtrNotNull(tc, result);
  row = mysql_fetch_row(result);
  CuAssertPtrNotNull(tc, row);
  number = row[0] != NULL ? strtoll(row[0], NULL, 10) : 0;
  mysql_free_result(result);
  return number;
}
