#include "CuTest.h"

#include "../../src/conf.h"
#include "../../src/sysdep.h"
#include "../../src/structs.h"
#include "../../src/utils.h"
#include "../../src/mysql.h"

#include <stdlib.h>
#include <string.h>

extern MYSQL *conn;
extern bool mysql_available;

static MYSQL *open_test_database(void)
{
  const char *host;
  const char *user;
  const char *password;
  const char *database;
  const char *port_text;
  MYSQL *connection;
  unsigned int port;

  host = getenv("LUMINARI_TEST_MYSQL_HOST");
  user = getenv("LUMINARI_TEST_MYSQL_USER");
  password = getenv("LUMINARI_TEST_MYSQL_PASSWORD");
  database = getenv("LUMINARI_TEST_MYSQL_DATABASE");
  port_text = getenv("LUMINARI_TEST_MYSQL_PORT");
  port = port_text != NULL ? (unsigned int)strtoul(port_text, NULL, 10) : 3306;

  if (host == NULL || user == NULL || password == NULL || database == NULL)
    return NULL;

  connection = mysql_init(NULL);
  if (connection == NULL)
    return NULL;

  if (mysql_real_connect(connection, host, user, password, database, port, NULL, 0) == NULL)
  {
    mysql_close(connection);
    return NULL;
  }

  return connection;
}

void Test_database_production_invalid_prepared_statement_inputs(CuTest *tc)
{
  CuAssertPtrEquals(tc, NULL, mysql_stmt_create(NULL));
  CuAssertTrue(tc, !mysql_stmt_prepare_query(NULL, "SELECT 1"));
  CuAssertTrue(tc, !mysql_stmt_bind_param_string(NULL, 0, "value"));
  CuAssertTrue(tc, !mysql_stmt_bind_param_int(NULL, 0, 1));
  CuAssertTrue(tc, !mysql_stmt_execute_prepared(NULL));
  CuAssertTrue(tc, !mysql_stmt_fetch_row(NULL));
}

void Test_database_production_prepared_statement_round_trip(CuTest *tc)
{
  const char *enabled;
  MYSQL *connection;
  MYSQL *saved_conn;
  PREPARED_STMT *insert_stmt;
  PREPARED_STMT *select_stmt;
  bool saved_available;
  bool created;
  bool inserted;
  bool selected;
  bool fetched;
  bool payload_matches;
  int stored_count;

  enabled = getenv("LUMINARI_TEST_MYSQL_ENABLE");
  if (enabled == NULL || strcmp(enabled, "1") != 0)
  {
    CuAssertTrue(tc, 1);
    return;
  }

  connection = open_test_database();
  if (connection == NULL)
  {
    CuFail(tc, "could not connect to the explicitly configured test database");
    return;
  }

  saved_conn = conn;
  saved_available = mysql_available;
  conn = connection;
  mysql_available = true;
  insert_stmt = NULL;
  select_stmt = NULL;
  created = mysql_query_safe(
                connection,
                "CREATE TEMPORARY TABLE luminari_cutest_persistence ("
                "id INT PRIMARY KEY, payload VARCHAR(128) NOT NULL, item_count INT NOT NULL)") == 0;

  if (created)
  {
    insert_stmt = mysql_stmt_create(connection);
    inserted =
        insert_stmt != NULL &&
        mysql_stmt_prepare_query(
            insert_stmt, "INSERT INTO luminari_cutest_persistence (id, payload, item_count) VALUES "
                         "(?, ?, ?)") &&
        mysql_stmt_bind_param_int(insert_stmt, 0, 17) &&
        mysql_stmt_bind_param_string(insert_stmt, 1, "O'Brien coverage payload") &&
        mysql_stmt_bind_param_int(insert_stmt, 2, 42) && mysql_stmt_execute_prepared(insert_stmt);
  }
  else
  {
    inserted = false;
  }

  if (inserted)
  {
    select_stmt = mysql_stmt_create(connection);
    selected = select_stmt != NULL &&
               mysql_stmt_prepare_query(
                   select_stmt,
                   "SELECT payload, item_count FROM luminari_cutest_persistence WHERE id = ?") &&
               mysql_stmt_bind_param_int(select_stmt, 0, 17) &&
               mysql_stmt_execute_prepared(select_stmt);
  }
  else
  {
    selected = false;
  }

  fetched = selected && mysql_stmt_fetch_row(select_stmt);
  payload_matches = fetched && mysql_stmt_get_string(select_stmt, 0) != NULL &&
                    strcmp(mysql_stmt_get_string(select_stmt, 0), "O'Brien coverage payload") == 0;
  stored_count = fetched ? mysql_stmt_get_int(select_stmt, 1) : -1;

  mysql_stmt_cleanup(select_stmt);
  mysql_stmt_cleanup(insert_stmt);
  mysql_close(connection);
  conn = saved_conn;
  mysql_available = saved_available;

  CuAssertTrue(tc, created);
  CuAssertTrue(tc, inserted);
  CuAssertTrue(tc, selected);
  CuAssertTrue(tc, fetched);
  CuAssertTrue(tc, payload_matches);
  CuAssertIntEquals(tc, 42, stored_count);
}
