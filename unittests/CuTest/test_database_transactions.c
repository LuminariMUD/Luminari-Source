/* Transactions that survive a lost connection (work item #13).
 *
 * The drops here are real: the connection's socket is shut down, by the test
 * or by mysql_test_drop_connection_at() inside the code under test, on a
 * connection that reconnects by itself as the server's do. A TEMPORARY table
 * goes with its session, so these tests use persistent tables and rows with
 * test-only keys, and remove them afterwards. */

#include "CuTest.h"

#include "conf.h"
#include "../../src/core/sysdep.h"
#include "../../src/core/structs.h"
#include "../../src/core/utils.h"
#include "../../src/core/comm.h"
#include "../../src/core/db.h"
#include "../../src/core/handler.h"
#include "../../src/core/help.h"
#include "../../src/core/interpreter.h"
#include "../../src/database/mysql.h"
#include "../../src/net/protocol.h"
#include "../../src/obj/house.h"
#include "../../src/obj/objsave.h"
#include "../../src/olc/oasis.h"
#include "../../src/vessels/vessels.h"

#include <limits.h>
#include <mariadb/errmsg.h>
#include <mariadb/mysqld_error.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

/* The table the layer's own tests write: one row, id 1. */
#define TRANSACTIONS_TABLE "s13_transactions_test"

struct transactions_database
{
  MYSQL *connection;
  MYSQL *saved_conn;
  bool saved_available;
};

static bool transactions_enabled(void)
{
  const char *enabled = getenv("LUMINARI_TEST_MYSQL_ENABLE");

  return enabled != NULL && strcmp(enabled, "1") == 0;
}

/* A connection made as the pool makes its own: it reconnects by itself. */
static MYSQL *transactions_connect(void)
{
  const char *port_text = getenv("LUMINARI_TEST_MYSQL_PORT");
  my_bool reconnect = 1;
  MYSQL *connection;

  connection = mysql_init(NULL);
  if (connection == NULL)
    return NULL;
  mysql_options(connection, MYSQL_OPT_RECONNECT, (const char *)&reconnect);
  if (mysql_real_connect(
          connection, getenv("LUMINARI_TEST_MYSQL_HOST"), getenv("LUMINARI_TEST_MYSQL_USER"),
          getenv("LUMINARI_TEST_MYSQL_PASSWORD"), getenv("LUMINARI_TEST_MYSQL_DATABASE"),
          port_text != NULL ? (unsigned int)strtoul(port_text, NULL, 10) : 3306, NULL, 0) == NULL)
  {
    mysql_close(connection);
    return NULL;
  }
  return connection;
}

/* Put a test connection in the server's place. FALSE when the database tests
 * are off. */
static bool transactions_begin(CuTest *tc, struct transactions_database *database)
{
  if (!transactions_enabled())
    return FALSE;
  database->connection = transactions_connect();
  if (database->connection == NULL)
    CuFail(tc, "could not connect to the explicitly configured test database");
  database->saved_conn = conn;
  database->saved_available = mysql_available;
  conn = database->connection;
  mysql_available = TRUE;
  return TRUE;
}

static void transactions_end(struct transactions_database *database)
{
  mysql_test_drop_connection_at(NULL, 0, FALSE);
  conn = database->saved_conn;
  mysql_available = database->saved_available;
  mysql_close(database->connection);
}

/* The first column of the query's first row; -1 when there is none. The
 * read waits for a lost session that is still finishing. */
static int transactions_value(MYSQL *connection, const char *query)
{
  MYSQL_RES *result;
  MYSQL_ROW row;
  int value = -1;

  if (mysql_query(connection, query) != 0)
    return -1;
  result = mysql_store_result(connection);
  if (result == NULL)
    return -1;
  row = mysql_fetch_row(result);
  if (row != NULL && row[0] != NULL)
    value = (int)strtol(row[0], NULL, 10);
  mysql_free_result(result);
  return value;
}

static int transactions_row(MYSQL *connection)
{
  return transactions_value(connection,
                            "SELECT v FROM " TRANSACTIONS_TABLE " WHERE id = 1 LOCK IN SHARE MODE");
}

static void transactions_table(CuTest *tc, MYSQL *connection)
{
  CuAssertIntEquals(tc, 0, mysql_query(connection, "DROP TABLE IF EXISTS " TRANSACTIONS_TABLE));
  CuAssertIntEquals(tc, 0,
                    mysql_query(connection, "CREATE TABLE " TRANSACTIONS_TABLE
                                            " (id INT PRIMARY KEY, v INT NOT NULL) ENGINE=InnoDB"));
  CuAssertIntEquals(tc, 0,
                    mysql_query(connection, "INSERT INTO " TRANSACTIONS_TABLE " VALUES (1, 0)"));
}

static void transactions_drop_socket(MYSQL *connection)
{
  shutdown(mysql_get_socket(connection), SHUT_RDWR);
}

/* One prepared UPDATE of the row through the server's wrappers. */
static bool transactions_prepared_update(int value)
{
  PREPARED_STMT *statement;
  bool updated;

  statement = mysql_stmt_create(conn);
  updated =
      statement != NULL &&
      mysql_stmt_prepare_query(statement, "UPDATE " TRANSACTIONS_TABLE " SET v = ? WHERE id = 1") &&
      mysql_stmt_bind_param_int(statement, 0, value) && mysql_stmt_execute_prepared(statement);
  mysql_stmt_cleanup(statement);
  return updated;
}

/* After a connection drops inside a transaction, the statements that follow
 * are refused instead of committing one by one on a new session, until the
 * caller ends the transaction. */
void Test_database_lost_transaction_takes_no_statement_until_it_ends(CuTest *tc)
{
  struct transactions_database database;
  unsigned long session;
  int after_sending;

  if (!transactions_begin(tc, &database))
    return;
  transactions_table(tc, conn);

  /* The drop comes before the statement is sent (2006), then after (2013). */
  for (after_sending = 0; after_sending <= 1; after_sending++)
  {
    CuAssertIntEquals(tc, 0, mysql_query(conn, "START TRANSACTION"));
    CuAssertIntEquals(tc, 0, mysql_query(conn, "UPDATE " TRANSACTIONS_TABLE " SET v = 1"));
    session = mysql_thread_id(conn);
    mysql_test_drop_connection_at("UPDATE", 1, after_sending != 0);
    CuAssertTrue(tc, mysql_query(conn, "UPDATE " TRANSACTIONS_TABLE " SET v = 2") != 0);
    CuAssertIntEquals(tc, after_sending ? CR_SERVER_LOST : CR_SERVER_GONE_ERROR, mysql_errno(conn));

    /* Nothing more is sent: no query, no prepared statement, no new session. */
    CuAssertTrue(tc, mysql_query(conn, "UPDATE " TRANSACTIONS_TABLE " SET v = 3") != 0);
    CuAssertTrue(tc, !transactions_prepared_update(4));
    CuAssertTrue(tc, session == mysql_thread_id(conn));

    /* The COMMIT fails and ends it; the row is what it was, and the
     * connection works again. */
    CuAssertTrue(tc, mysql_query(conn, "COMMIT") != 0);
    CuAssertIntEquals(tc, 0, transactions_row(conn));
    CuAssertTrue(tc, session != mysql_thread_id(conn));
    CuAssertIntEquals(tc, 0, mysql_query(conn, "UPDATE " TRANSACTIONS_TABLE " SET v = 0"));
  }

  /* A ROLLBACK ends it too, in any spelling the callers use. */
  CuAssertIntEquals(tc, 0, mysql_query(conn, "start transaction;"));
  mysql_test_drop_connection_at("UPDATE", 1, FALSE);
  CuAssertTrue(tc, mysql_query(conn, "UPDATE " TRANSACTIONS_TABLE " SET v = 5") != 0);
  CuAssertTrue(tc, mysql_query(conn, "UPDATE " TRANSACTIONS_TABLE " SET v = 6") != 0);
  CuAssertIntEquals(tc, 0, mysql_query(conn, "rollback;"));
  CuAssertIntEquals(tc, 0, transactions_row(conn));

  /* So does the next transaction of a caller that ended nothing. */
  CuAssertIntEquals(tc, 0, mysql_query(conn, "START TRANSACTION"));
  mysql_test_drop_connection_at("UPDATE", 1, FALSE);
  CuAssertTrue(tc, mysql_query(conn, "UPDATE " TRANSACTIONS_TABLE " SET v = 7") != 0);
  CuAssertIntEquals(tc, 0, mysql_query(conn, "START TRANSACTION"));
  CuAssertIntEquals(tc, 0, mysql_query(conn, "UPDATE " TRANSACTIONS_TABLE " SET v = 8"));
  CuAssertIntEquals(tc, 0, mysql_query(conn, "COMMIT"));
  CuAssertIntEquals(tc, 8, transactions_row(conn));

  /* A caller that ends nothing and is followed by no transaction cannot hold
   * the connection: all database work ends within its game pulse, so at the
   * next pulse the lost transaction is rolled back for it. */
  CuAssertIntEquals(tc, 0, mysql_query(conn, "UPDATE " TRANSACTIONS_TABLE " SET v = 0"));
  CuAssertIntEquals(tc, 0, mysql_query(conn, "START TRANSACTION"));
  CuAssertIntEquals(tc, 0, mysql_query(conn, "UPDATE " TRANSACTIONS_TABLE " SET v = 11"));
  mysql_test_drop_connection_at("UPDATE", 1, FALSE);
  CuAssertTrue(tc, mysql_query(conn, "UPDATE " TRANSACTIONS_TABLE " SET v = 12") != 0);
  CuAssertTrue(tc, mysql_query(conn, "UPDATE " TRANSACTIONS_TABLE " SET v = 13") != 0);
  pulse++;
  CuAssertIntEquals(tc, 0, transactions_row(conn));
  CuAssertIntEquals(tc, 0, mysql_query(conn, "UPDATE " TRANSACTIONS_TABLE " SET v = 8"));
  pulse--;
  CuAssertIntEquals(tc, 8, transactions_row(conn));

  /* Outside a transaction a drop loses nothing: the statement is sent again
   * on a new session, as before. */
  transactions_drop_socket(conn);
  CuAssertIntEquals(tc, 0, mysql_query(conn, "UPDATE " TRANSACTIONS_TABLE " SET v = 9"));
  CuAssertIntEquals(tc, 9, transactions_row(conn));

  /* A statement the server refuses loses nothing either: the transaction
   * goes on and commits. */
  CuAssertIntEquals(tc, 0, mysql_query(conn, "START TRANSACTION"));
  CuAssertTrue(tc, mysql_query(conn, "INSERT INTO " TRANSACTIONS_TABLE " VALUES (1, 1)") != 0);
  CuAssertIntEquals(tc, 0, mysql_query(conn, "UPDATE " TRANSACTIONS_TABLE " SET v = 10"));
  CuAssertIntEquals(tc, 0, mysql_query(conn, "COMMIT"));
  CuAssertIntEquals(tc, 10, transactions_row(conn));

  CuAssertIntEquals(tc, 0, mysql_query(conn, "DROP TABLE " TRANSACTIONS_TABLE));
  transactions_end(&database);
}

/* A prepared statement, the close of one, and a ping can each be the command
 * that meets the drop; the last two reconnect by themselves and report no
 * failed statement. */
void Test_database_lost_transaction_is_noticed_by_statements_closes_and_pings(CuTest *tc)
{
  static char no_socket[] = "/nonexistent/s13.sock";
  struct transactions_database database;
  PREPARED_STMT *statement;
  char *unix_socket;
  unsigned int tcp_port;

  if (!transactions_begin(tc, &database))
    return;
  transactions_table(tc, conn);

  /* A prepared execution. */
  CuAssertIntEquals(tc, 0, mysql_query(conn, "START TRANSACTION"));
  CuAssertTrue(tc, transactions_prepared_update(1));
  mysql_test_drop_connection_at("UPDATE", 1, FALSE);
  CuAssertTrue(tc, !transactions_prepared_update(2));
  CuAssertTrue(tc, !transactions_prepared_update(3));
  CuAssertTrue(tc, mysql_query(conn, "UPDATE " TRANSACTIONS_TABLE " SET v = 4") != 0);
  CuAssertTrue(tc, mysql_query(conn, "COMMIT") != 0);
  CuAssertIntEquals(tc, 0, transactions_row(conn));

  /* The close of a statement that ran before the drop. */
  CuAssertIntEquals(tc, 0, mysql_query(conn, "START TRANSACTION"));
  statement = mysql_stmt_create(conn);
  CuAssertTrue(tc,
               statement != NULL &&
                   mysql_stmt_prepare_query(statement, "UPDATE " TRANSACTIONS_TABLE " SET v = 5") &&
                   mysql_stmt_execute_prepared(statement));
  transactions_drop_socket(conn);
  mysql_stmt_cleanup(statement);
  CuAssertTrue(tc, mysql_query(conn, "UPDATE " TRANSACTIONS_TABLE " SET v = 6") != 0);
  CuAssertTrue(tc, mysql_query(conn, "COMMIT") != 0);
  CuAssertIntEquals(tc, 0, transactions_row(conn));

  /* A ping: it succeeds, on a new session. */
  CuAssertIntEquals(tc, 0, mysql_query(conn, "START TRANSACTION"));
  CuAssertIntEquals(tc, 0, mysql_query(conn, "UPDATE " TRANSACTIONS_TABLE " SET v = 7"));
  transactions_drop_socket(conn);
  CuAssertTrue(tc, MYSQL_PING_CONN(conn));
  CuAssertTrue(tc, mysql_query(conn, "UPDATE " TRANSACTIONS_TABLE " SET v = 8") != 0);
  CuAssertIntEquals(tc, 0, mysql_query(conn, "ROLLBACK"));
  CuAssertIntEquals(tc, 0, transactions_row(conn));

  /* A ping that cannot reconnect fails, and the transaction is as lost: the
   * statement after it must not open a session of its own once the
   * database is back. */
  CuAssertIntEquals(tc, 0, mysql_query(conn, "START TRANSACTION"));
  CuAssertIntEquals(tc, 0, mysql_query(conn, "UPDATE " TRANSACTIONS_TABLE " SET v = 7"));
  tcp_port = conn->port;
  unix_socket = conn->unix_socket;
  conn->port = 1;
  conn->unix_socket = no_socket;
  transactions_drop_socket(conn);
  CuAssertTrue(tc, !MYSQL_PING_CONN(conn));
  conn->port = tcp_port;
  conn->unix_socket = unix_socket;
  CuAssertTrue(tc, mysql_query(conn, "UPDATE " TRANSACTIONS_TABLE " SET v = 8") != 0);
  CuAssertIntEquals(tc, 0, mysql_query(conn, "ROLLBACK"));
  CuAssertIntEquals(tc, 0, transactions_row(conn));

  /* A ping with no transaction open marks nothing. */
  transactions_drop_socket(conn);
  CuAssertTrue(tc, MYSQL_PING_CONN(conn));
  CuAssertIntEquals(tc, 0, mysql_query(conn, "UPDATE " TRANSACTIONS_TABLE " SET v = 9"));
  CuAssertIntEquals(tc, 9, transactions_row(conn));

  CuAssertIntEquals(tc, 0, mysql_query(conn, "DROP TABLE " TRANSACTIONS_TABLE));
  transactions_end(&database);
}

/* A deadlock's victim loses its whole transaction with the connection up, and
 * the library goes on holding the transaction open. The statements after it
 * are refused all the same, and the COMMIT reports a refusal. */
void Test_database_deadlock_victim_takes_no_statement_until_it_ends(CuTest *tc)
{
  struct transactions_database database;
  MYSQL *other;
  const char *blocked = "UPDATE " TRANSACTIONS_TABLE " SET v = 9 WHERE id = 1";
  int failed;
  unsigned int failed_errno;

  if (!transactions_begin(tc, &database))
    return;
  transactions_table(tc, conn);
  CuAssertIntEquals(
      tc, 0, mysql_query(conn, "INSERT INTO " TRANSACTIONS_TABLE " VALUES (2, 0), (3, 0), (4, 0)"));
  other = transactions_connect();
  CuAssertPtrNotNull(tc, other);

  /* The other session is the heavier transaction: it holds rows 2 and 3. */
  CuAssertIntEquals(tc, 0, (mysql_query)(other, "START TRANSACTION"));
  CuAssertIntEquals(
      tc, 0, (mysql_query)(other, "UPDATE " TRANSACTIONS_TABLE " SET v = 9 WHERE id IN (2, 3)"));

  CuAssertIntEquals(tc, 0, mysql_query(conn, "START TRANSACTION"));
  CuAssertIntEquals(tc, 0,
                    mysql_query(conn, "UPDATE " TRANSACTIONS_TABLE " SET v = 1 WHERE id = 1"));

  /* The other session asks for row 1, and the game's second write for row 2:
   * whichever request closes the cycle, the lighter transaction is rolled
   * back, and that is the game's. */
  CuAssertIntEquals(tc, 0, mysql_send_query(other, blocked, (unsigned long)strlen(blocked)));
  failed = mysql_query(conn, "UPDATE " TRANSACTIONS_TABLE " SET v = 1 WHERE id = 2");
  failed_errno = mysql_errno(conn);
  CuAssertTrue(tc, failed != 0);
  CuAssertIntEquals(tc, ER_LOCK_DEADLOCK, (int)failed_errno);

  /* Row 4 is not written on its own, and nothing is reported committed. */
  CuAssertTrue(tc, mysql_query(conn, "UPDATE " TRANSACTIONS_TABLE " SET v = 7 WHERE id = 4") != 0);
  CuAssertIntEquals(tc, MYSQL_COMMIT_REFUSED, mysql_commit_transaction(conn));

  CuAssertIntEquals(tc, 0, mysql_read_query_result(other));
  CuAssertIntEquals(tc, 0, (mysql_query)(other, "ROLLBACK"));
  mysql_close(other);

  CuAssertIntEquals(tc, 0, transactions_row(conn));
  CuAssertIntEquals(tc, 0,
                    transactions_value(conn, "SELECT v FROM " TRANSACTIONS_TABLE " WHERE id = 4"));

  /* The transaction is over: the connection serves the next one. */
  CuAssertIntEquals(tc, 0, mysql_query(conn, "START TRANSACTION"));
  CuAssertIntEquals(tc, 0,
                    mysql_query(conn, "UPDATE " TRANSACTIONS_TABLE " SET v = 5 WHERE id = 4"));
  CuAssertIntEquals(tc, MYSQL_COMMIT_DONE, mysql_commit_transaction(conn));
  CuAssertIntEquals(tc, 5,
                    transactions_value(conn, "SELECT v FROM " TRANSACTIONS_TABLE " WHERE id = 4"));

  mysql_query(conn, "DROP TABLE IF EXISTS " TRANSACTIONS_TABLE);
  transactions_end(&database);
}

/* The COMMIT helper tells a confirmed commit, a refused one and one without
 * a reply apart, and leaves no transaction open. */
void Test_database_commit_reports_done_refused_and_unanswered(CuTest *tc)
{
  struct transactions_database database;

  if (!transactions_begin(tc, &database))
    return;
  transactions_table(tc, conn);

  CuAssertIntEquals(tc, 0, mysql_query(conn, "START TRANSACTION"));
  CuAssertIntEquals(tc, 0, mysql_query(conn, "UPDATE " TRANSACTIONS_TABLE " SET v = 1"));
  CuAssertIntEquals(tc, MYSQL_COMMIT_DONE, mysql_commit_transaction(conn));
  CuAssertIntEquals(tc, 1, transactions_row(conn));

  /* The reply is lost after the server committed. */
  CuAssertIntEquals(tc, 0, mysql_query(conn, "START TRANSACTION"));
  CuAssertIntEquals(tc, 0, mysql_query(conn, "UPDATE " TRANSACTIONS_TABLE " SET v = 2"));
  mysql_test_drop_connection_at("COMMIT", 1, TRUE);
  CuAssertIntEquals(tc, MYSQL_COMMIT_UNANSWERED, mysql_commit_transaction(conn));
  CuAssertIntEquals(tc, 2, transactions_row(conn));

  /* The COMMIT never reached the server, which rolled back: the client
   * cannot tell the two apart. */
  CuAssertIntEquals(tc, 0, mysql_query(conn, "START TRANSACTION"));
  CuAssertIntEquals(tc, 0, mysql_query(conn, "UPDATE " TRANSACTIONS_TABLE " SET v = 3"));
  mysql_test_drop_connection_at("COMMIT", 1, FALSE);
  CuAssertIntEquals(tc, MYSQL_COMMIT_UNANSWERED, mysql_commit_transaction(conn));
  CuAssertIntEquals(tc, 2, transactions_row(conn));

  /* The transaction was lost before its COMMIT: known not to be committed. */
  CuAssertIntEquals(tc, 0, mysql_query(conn, "START TRANSACTION"));
  mysql_test_drop_connection_at("UPDATE", 1, FALSE);
  CuAssertTrue(tc, mysql_query(conn, "UPDATE " TRANSACTIONS_TABLE " SET v = 4") != 0);
  CuAssertIntEquals(tc, MYSQL_COMMIT_REFUSED, mysql_commit_transaction(conn));
  CuAssertIntEquals(tc, 2, transactions_row(conn));

  /* A COMMIT that fails on a live connection is refused, and rolled back. */
  CuAssertIntEquals(tc, 0, mysql_query(conn, "START TRANSACTION"));
  CuAssertIntEquals(tc, 0, mysql_query(conn, "UPDATE " TRANSACTIONS_TABLE " SET v = 5"));
  mysql_test_fail_nth_query(1);
  CuAssertIntEquals(tc, MYSQL_COMMIT_REFUSED, mysql_commit_transaction(conn));
  CuAssertIntEquals(tc, 2, transactions_row(conn));

  /* A caller that sends its own COMMIT and no ROLLBACK after it failed is
   * not refused its next statement. */
  CuAssertIntEquals(tc, 0, mysql_query(conn, "START TRANSACTION"));
  CuAssertIntEquals(tc, 0, mysql_query(conn, "UPDATE " TRANSACTIONS_TABLE " SET v = 6"));
  mysql_test_drop_connection_at("COMMIT", 1, TRUE);
  CuAssertTrue(tc, mysql_query(conn, "COMMIT") != 0);
  CuAssertIntEquals(tc, 6, transactions_row(conn));

  CuAssertIntEquals(tc, 0, mysql_query(conn, "DROP TABLE " TRANSACTIONS_TABLE));
  transactions_end(&database);
}

/* The pool reconnects a stale handle in place, since conn, conn2 and conn3
 * are its handles, and hands out no connection while the database is away
 * instead of waiting for it: the game runs on the thread that asks. */
void Test_database_pool_keeps_its_handles_and_never_waits_for_the_database(CuTest *tc)
{
  static char no_socket[] = "/nonexistent/s13.sock";
  const char *host = getenv("LUMINARI_TEST_MYSQL_HOST");
  struct transactions_database database;
  MYSQL_POOL *saved_pool = mysql_pool;
  MYSQL_POOL pool;
  MYSQL_POOL_CONN entry;
  MYSQL_POOL_CONN *acquired;
  MYSQL_RES *result;
  char *unix_socket;
  unsigned int tcp_port;

  if (!transactions_begin(tc, &database))
    return;
  memset(&pool, 0, sizeof(pool));
  memset(&entry, 0, sizeof(entry));
  pthread_mutex_init(&pool.pool_mutex, NULL);
  pthread_cond_init(&pool.pool_cond, NULL);
  pthread_mutex_init(&entry.mutex, NULL);
  entry.conn = conn;
  entry.state = CONN_STATE_FREE;
  pool.connections = &entry;
  pool.current_size = 1;
  pool.initialized = TRUE;
  /* The server refuses this account, so the pool can open no connection of
   * its own. */
  strlcpy(pool.host, host != NULL ? host : "", sizeof(pool.host));
  strlcpy(pool.username, "s13-no-such-user", sizeof(pool.username));
  strlcpy(pool.password, "none", sizeof(pool.password));
  strlcpy(pool.database, "s13_no_such_database", sizeof(pool.database));
  mysql_pool = &pool;

  /* A long-idle connection that has dropped is reconnected, not replaced. */
  entry.last_used = 0;
  transactions_drop_socket(conn);
  acquired = mysql_pool_acquire();
  CuAssertPtrEquals(tc, &entry, acquired);
  CuAssertPtrEquals(tc, database.connection, entry.conn);
  CuAssertIntEquals(tc, 1, transactions_value(conn, "SELECT 1"));

  /* With its one connection out the pool would open another. It cannot, and
   * says so instead of trying again without end. */
  CuAssertPtrEquals(tc, NULL, mysql_pool_acquire());

  /* A query that gets no connection leaves its caller no result to free. */
  result = (MYSQL_RES *)&pool;
  CuAssertTrue(tc, mysql_pool_query("SELECT 1", &result) != 0);
  CuAssertPtrEquals(tc, NULL, result);
  mysql_pool_release(acquired);

  /* The database is away: no connection, the entry still free and the
   * handle still open, however often the pool is asked. */
  entry.last_used = 0;
  tcp_port = conn->port;
  unix_socket = conn->unix_socket;
  conn->port = 1;
  conn->unix_socket = no_socket;
  transactions_drop_socket(conn);
  CuAssertPtrEquals(tc, NULL, mysql_pool_acquire());
  CuAssertPtrEquals(tc, NULL, mysql_pool_acquire());
  CuAssertPtrEquals(tc, database.connection, entry.conn);
  CuAssertIntEquals(tc, CONN_STATE_FREE, entry.state);

  /* It is back: the same handle serves again. */
  conn->port = tcp_port;
  conn->unix_socket = unix_socket;
  acquired = mysql_pool_acquire();
  CuAssertPtrEquals(tc, &entry, acquired);
  CuAssertIntEquals(tc, 1, transactions_value(conn, "SELECT 1"));
  mysql_pool_release(acquired);

  mysql_pool = saved_pool;
  pthread_mutex_destroy(&entry.mutex);
  pthread_cond_destroy(&pool.pool_cond);
  pthread_mutex_destroy(&pool.pool_mutex);
  transactions_end(&database);
}

/* ------------------------------------------------------------------------ */
/* Object saves                                                             */
/* ------------------------------------------------------------------------ */

#define SAVES_OWNER "Sthirteen"
#define SAVES_OBJECT_VNUM 91301
#define SAVES_HOUSE_VNUM 91399

/* A player with a house, one object prototype, a staff member listening to
 * the system log, and a scratch directory for the save files. */
struct transactions_world
{
  struct transactions_database database;
  MYSQL *observer; /* a second session: it sees only what was committed */
  struct char_data owner;
  struct player_special_data specials;
  struct bag_data bags;
  struct char_data staff;
  struct player_special_data staff_specials;
  struct descriptor_data staff_descriptor;
  char staff_output[4096];
  struct descriptor_data *saved_descriptors;
  struct room_data room;
  struct room_data *saved_world;
  room_rnum saved_top_of_world;
  struct obj_data prototype;
  struct index_data index;
  struct zone_data zone;
  struct obj_data *saved_proto;
  struct index_data *saved_index;
  obj_rnum saved_top_of_objt;
  struct obj_data *saved_object_list;
  struct zone_data *saved_zone_table;
  zone_rnum saved_top_of_zone_table;
  struct player_index_element player_index[1];
  struct player_index_element *saved_player_table;
  int saved_top_of_p_table;
  char scratch[64];
  char home[PATH_MAX];
};

static void saves_remove_rows(MYSQL *connection)
{
  mysql_query(connection, "ALTER TABLE player_save_objs DROP CONSTRAINT IF EXISTS s13_refused");
  mysql_query(connection,
              "ALTER TABLE player_save_objs_sheathed DROP CONSTRAINT IF EXISTS s13_refused");
  mysql_query(connection, "ALTER TABLE house_data DROP CONSTRAINT IF EXISTS s13_refused");
  mysql_query(connection, "DELETE FROM player_save_objs WHERE name = '" SAVES_OWNER "'");
  mysql_query(connection,
              "DELETE FROM player_save_objs_sheathed WHERE owner_name = '" SAVES_OWNER "'");
  mysql_query(connection, "DELETE FROM house_data WHERE vnum = 91399");
}

static bool transactions_world_begin(CuTest *tc, struct transactions_world *w)
{
  memset(w, 0, sizeof(*w));
  if (!transactions_begin(tc, &w->database))
    return FALSE;
  w->observer = transactions_connect();
  CuAssertPtrNotNull(tc, w->observer);
  saves_remove_rows(conn);

  strlcpy(w->scratch, "/tmp/luminari-object-saves-XXXXXX", sizeof(w->scratch));
  CuAssertPtrNotNull(tc, getcwd(w->home, sizeof(w->home)));
  CuAssertPtrNotNull(tc, mkdtemp(w->scratch));
  CuAssertIntEquals(tc, 0, chdir(w->scratch));
  CuAssertIntEquals(tc, 0, mkdir("plrobjs", 0700));
  CuAssertIntEquals(tc, 0, mkdir("plrobjs/P-T", 0700));
  CuAssertIntEquals(tc, 0, mkdir("plrfiles", 0700));
  CuAssertIntEquals(tc, 0, mkdir("plrfiles/P-T", 0700));
  CuAssertIntEquals(tc, 0, mkdir("house", 0700));

  w->saved_object_list = object_list;
  w->saved_proto = obj_proto;
  w->saved_index = obj_index;
  w->saved_top_of_objt = top_of_objt;
  w->saved_zone_table = zone_table;
  w->saved_top_of_zone_table = top_of_zone_table;
  object_list = NULL;
  clear_object(&w->prototype);
  w->prototype.item_number = 0;
  w->prototype.name = CuMutableString("gem");
  w->prototype.short_description = CuMutableString("a gem");
  w->prototype.description = CuMutableString("A gem lies here.");
  GET_OBJ_TYPE(&w->prototype) = ITEM_TREASURE;
  w->index.vnum = SAVES_OBJECT_VNUM;
  obj_proto = &w->prototype;
  obj_index = &w->index;
  top_of_objt = 0;
  w->zone.number = 913;
  w->zone.bot = 91300;
  w->zone.top = 91399;
  zone_table = &w->zone;
  top_of_zone_table = 0;

  w->saved_world = world;
  w->saved_top_of_world = top_of_world;
  w->room.number = SAVES_HOUSE_VNUM;
  world = &w->room;
  top_of_world = 0;

  clear_char(&w->owner);
  w->owner.player_specials = &w->specials;
  w->owner.player.name = CuMutableString(SAVES_OWNER);
  w->owner.player.level = 10;
  w->owner.bags = &w->bags;
  IN_ROOM(&w->owner) = 0;
  GET_GOLD(&w->owner) = 1000000;
  w->player_index[0].name = GET_NAME(&w->owner);
  w->player_index[0].id = 4313;
  w->saved_player_table = player_table;
  w->saved_top_of_p_table = top_of_p_table;
  player_table = w->player_index;
  top_of_p_table = 0;
  GET_PFILEPOS(&w->owner) = 0;

  clear_char(&w->staff);
  w->staff.player_specials = &w->staff_specials;
  w->staff.player.name = CuMutableString("Sthirteenstaff");
  w->staff.player.level = LVL_STAFF;
  SET_BIT_AR(PRF_FLAGS(&w->staff), PRF_LOG1);
  w->staff.desc = &w->staff_descriptor;
  w->staff_descriptor.character = &w->staff;
  w->staff_descriptor.connected = CON_PLAYING;
  w->staff_descriptor.output = w->staff_output;
  w->staff_descriptor.bufspace = sizeof(w->staff_output) - 1;
  w->staff_descriptor.pProtocol = ProtocolCreate();
  CuAssertPtrNotNull(tc, w->staff_descriptor.pProtocol);
  w->saved_descriptors = descriptor_list;
  descriptor_list = &w->staff_descriptor;
  return TRUE;
}

static void transactions_world_end(CuTest *tc, struct transactions_world *w)
{
  char filename[MAX_FILEPATH];

  while (w->owner.carrying != NULL)
    extract_obj(w->owner.carrying);
  while (w->room.contents != NULL)
    extract_obj(w->room.contents);
  descriptor_list = w->saved_descriptors;
  ProtocolDestroy(w->staff_descriptor.pProtocol);
  player_table = w->saved_player_table;
  top_of_p_table = w->saved_top_of_p_table;
  world = w->saved_world;
  top_of_world = w->saved_top_of_world;
  zone_table = w->saved_zone_table;
  top_of_zone_table = w->saved_top_of_zone_table;
  obj_proto = w->saved_proto;
  obj_index = w->saved_index;
  top_of_objt = w->saved_top_of_objt;
  object_list = w->saved_object_list;

  if (get_filename(filename, sizeof(filename), CRASH_FILE, SAVES_OWNER))
    unlink(filename);
  if (get_filename(filename, sizeof(filename), PLR_FILE, SAVES_OWNER))
    unlink(filename);
  unlink("plrfiles/index");
  unlink("house/91399.house");
  rmdir("plrobjs/P-T");
  rmdir("plrobjs");
  rmdir("plrfiles/P-T");
  rmdir("plrfiles");
  rmdir("house");
  CuAssertIntEquals(tc, 0, chdir(w->home));
  rmdir(w->scratch);

  saves_remove_rows(conn);
  mysql_close(w->observer);
  transactions_end(&w->database);
}

/* One more gem, named apart from the others, in the owner's hands. */
static struct obj_data *saves_carry(struct transactions_world *w, const char *short_description)
{
  struct obj_data *obj = read_object(SAVES_OBJECT_VNUM, VIRTUAL);

  obj->short_description = strdup(short_description);
  obj->carried_by = &w->owner;
  obj->next_content = w->owner.carrying;
  w->owner.carrying = obj;
  return obj;
}

/* And one on the floor of the house. */
static struct obj_data *saves_furnish(struct transactions_world *w, const char *short_description)
{
  struct obj_data *obj = read_object(SAVES_OBJECT_VNUM, VIRTUAL);

  obj->short_description = strdup(short_description);
  IN_ROOM(obj) = 0;
  obj->next_content = w->room.contents;
  w->room.contents = obj;
  return obj;
}

/* What the staff member was told since the last call. */
static const char *transactions_staff_heard(struct transactions_world *w)
{
  static char heard[sizeof(w->staff_output)];

  strlcpy(heard, w->staff_output, sizeof(heard));
  memset(w->staff_output, 0, sizeof(w->staff_output));
  w->staff_descriptor.bufptr = 0;
  w->staff_descriptor.bufspace = sizeof(w->staff_output) - 1;
  return heard;
}

static int saves_player_rows(struct transactions_world *w)
{
  return transactions_value(w->observer,
                            "SELECT COUNT(*) FROM player_save_objs WHERE name = '" SAVES_OWNER
                            "' LOCK IN SHARE MODE");
}

static int saves_house_rows(struct transactions_world *w)
{
  return transactions_value(
      w->observer, "SELECT COUNT(*) FROM house_data WHERE vnum = 91399 LOCK IN SHARE MODE");
}

/* A crash save whose connection drops at a row is lost whole: the rows after
 * it are not written on their own beside the last save's, the player stays
 * flagged, and the next pass saves them. */
void Test_object_save_lost_at_a_row_keeps_the_last_save_and_is_retried(CuTest *tc)
{
  struct transactions_world w;

  if (!transactions_world_begin(tc, &w))
    return;

  saves_carry(&w, "a first gem");
  saves_carry(&w, "a second gem");
  SET_BIT_AR(PLR_FLAGS(&w.owner), PLR_CRASH);
  CuAssertTrue(tc, Crash_crashsave(&w.owner));
  CuAssertTrue(tc, !PLR_FLAGGED(&w.owner, PLR_CRASH));
  CuAssertIntEquals(tc, 2, saves_player_rows(&w));

  /* A third gem; the connection goes at the second of the three rows. The
   * save was not asked for by the flag (a player's "save" is not), and sets
   * it. */
  saves_carry(&w, "a third gem");
  mysql_test_drop_connection_at("INSERT INTO player_save_objs (", 2, FALSE);
  CuAssertTrue(tc, !Crash_crashsave(&w.owner));
  CuAssertTrue(tc, PLR_FLAGGED(&w.owner, PLR_CRASH));
  CuAssertIntEquals(tc, 2, saves_player_rows(&w));
  CuAssertStrEquals(tc, "", transactions_staff_heard(&w));

  /* The pass that saves a flagged player: the character saves, the objects
   * do not, and the flag stays. */
  mysql_test_drop_connection_at("INSERT INTO player_save_objs (", 1, FALSE);
  CuAssertIntEquals(tc, 1, Crash_save_single(&w.owner, NULL, NULL));
  CuAssertTrue(tc, PLR_FLAGGED(&w.owner, PLR_CRASH));
  CuAssertIntEquals(tc, 2, saves_player_rows(&w));

  /* The next pass. */
  CuAssertIntEquals(tc, 1, Crash_save_single(&w.owner, NULL, NULL));
  CuAssertTrue(tc, !PLR_FLAGGED(&w.owner, PLR_CRASH));
  CuAssertIntEquals(tc, 3, saves_player_rows(&w));

  /* A reply lost at the COMMIT sets the flag too, whichever way it went. */
  mysql_test_drop_connection_at("commit", 1, TRUE);
  CuAssertTrue(tc, !Crash_crashsave(&w.owner));
  CuAssertTrue(tc, PLR_FLAGGED(&w.owner, PLR_CRASH));
  CuAssertIntEquals(tc, 3, saves_player_rows(&w));

  transactions_world_end(tc, &w);
}

/* A row the database refuses on a good connection is left out: the rest of
 * the save commits, the player is flagged, and the staff are told whose
 * objects they were. */
void Test_object_save_commits_around_a_refused_row_and_tells_the_staff(CuTest *tc)
{
  struct transactions_world w;
  struct obj_data *scabbard;

  if (!transactions_world_begin(tc, &w))
    return;
  CuAssertIntEquals(tc, 0,
                    mysql_query(conn, "ALTER TABLE player_save_objs ADD CONSTRAINT s13_refused "
                                      "CHECK (serialized_obj NOT LIKE '%refused%')"));
  CuAssertIntEquals(tc, 0,
                    mysql_query(conn,
                                "ALTER TABLE player_save_objs_sheathed ADD CONSTRAINT s13_refused "
                                "CHECK (serialized_obj NOT LIKE '%refused%')"));

  saves_carry(&w, "a first gem");
  saves_carry(&w, "a refused gem");
  saves_carry(&w, "a third gem");
  CuAssertTrue(tc, !PLR_FLAGGED(&w.owner, PLR_CRASH));
  CuAssertTrue(tc, !Crash_crashsave(&w.owner));
  CuAssertTrue(tc, PLR_FLAGGED(&w.owner, PLR_CRASH));
  CuAssertIntEquals(tc, 2, saves_player_rows(&w));
  CuAssertTrue(tc,
               strstr(transactions_staff_heard(&w), "1 of " SAVES_OWNER "'s objects could not be "
                                                    "saved; the rest were.") != NULL);

  /* A sheathed blade the database refuses counts with its scabbard. */
  scabbard = saves_carry(&w, "a scabbard");
  SET_BIT_AR(GET_OBJ_WEAR(scabbard), ITEM_WEAR_SHEATH);
  scabbard->sheath_primary = read_object(SAVES_OBJECT_VNUM, VIRTUAL);
  scabbard->sheath_primary->short_description = strdup("a refused blade");
  CuAssertTrue(tc, !Crash_crashsave(&w.owner));
  CuAssertIntEquals(tc, 3, saves_player_rows(&w));
  CuAssertTrue(tc, strstr(transactions_staff_heard(&w), "2 of " SAVES_OWNER "'s objects") != NULL);
  extract_obj(scabbard->sheath_primary);
  scabbard->sheath_primary = NULL;

  /* Once the database takes the rows, the next save is whole. */
  CuAssertIntEquals(tc, 0,
                    mysql_query(conn, "ALTER TABLE player_save_objs DROP CONSTRAINT s13_refused"));
  CuAssertTrue(tc, Crash_crashsave(&w.owner));
  CuAssertTrue(tc, !PLR_FLAGGED(&w.owner, PLR_CRASH));
  CuAssertIntEquals(tc, 4, saves_player_rows(&w));
  CuAssertStrEquals(tc, "", transactions_staff_heard(&w));

  transactions_world_end(tc, &w);
}

/* A house save is lost whole with its connection and commits around a
 * refused row, and sets its flag both times. */
void Test_house_save_is_lost_whole_and_commits_around_a_refused_row(CuTest *tc)
{
  struct transactions_world w;

  if (!transactions_world_begin(tc, &w))
    return;

  saves_furnish(&w, "a first gem");
  saves_furnish(&w, "a second gem");
  SET_BIT_AR(ROOM_FLAGS(0), ROOM_HOUSE_CRASH);
  CuAssertTrue(tc, House_crashsave(SAVES_HOUSE_VNUM));
  CuAssertTrue(tc, !ROOM_FLAGGED(0, ROOM_HOUSE_CRASH));
  CuAssertIntEquals(tc, 2, saves_house_rows(&w));

  saves_furnish(&w, "a third gem");
  mysql_test_drop_connection_at("INSERT INTO house_data", 2, FALSE);
  CuAssertTrue(tc, !House_crashsave(SAVES_HOUSE_VNUM));
  CuAssertTrue(tc, ROOM_FLAGGED(0, ROOM_HOUSE_CRASH));
  CuAssertIntEquals(tc, 2, saves_house_rows(&w));
  CuAssertStrEquals(tc, "", transactions_staff_heard(&w));

  CuAssertTrue(tc, House_crashsave(SAVES_HOUSE_VNUM));
  CuAssertTrue(tc, !ROOM_FLAGGED(0, ROOM_HOUSE_CRASH));
  CuAssertIntEquals(tc, 3, saves_house_rows(&w));

  CuAssertIntEquals(tc, 0,
                    mysql_query(conn, "ALTER TABLE house_data ADD CONSTRAINT s13_refused "
                                      "CHECK (serialized_obj NOT LIKE '%refused%')"));
  saves_furnish(&w, "a refused gem");
  CuAssertTrue(tc, House_crashsave(SAVES_HOUSE_VNUM));
  CuAssertTrue(tc, ROOM_FLAGGED(0, ROOM_HOUSE_CRASH));
  CuAssertIntEquals(tc, 3, saves_house_rows(&w));
  CuAssertTrue(tc, strstr(transactions_staff_heard(&w),
                          "1 of house 91399's objects could not be saved; the rest were.") != NULL);

  transactions_world_end(tc, &w);
}

/* A house has no owner to store a sheath's weapons under: they are saved as
 * the house's own objects, beside the sheath. */
void Test_house_save_stores_a_sheaths_weapons_as_its_own(CuTest *tc)
{
  struct transactions_world w;
  struct obj_data *sheath;

  if (!transactions_world_begin(tc, &w))
    return;

  sheath = saves_furnish(&w, "a sheath");
  SET_BIT_AR(GET_OBJ_WEAR(sheath), ITEM_WEAR_SHEATH);
  sheath->sheath_primary = read_object(SAVES_OBJECT_VNUM, VIRTUAL);
  sheath->sheath_primary->short_description = strdup("a sheathed blade");
  sheath->sheath_secondary = read_object(SAVES_OBJECT_VNUM, VIRTUAL);
  sheath->sheath_secondary->short_description = strdup("a slung shield");

  CuAssertTrue(tc, House_crashsave(SAVES_HOUSE_VNUM));
  CuAssertIntEquals(tc, 3, saves_house_rows(&w));
  CuAssertIntEquals(tc, 1,
                    transactions_value(w.observer,
                                       "SELECT COUNT(*) FROM house_data WHERE vnum = 91399 AND "
                                       "serialized_obj LIKE '%a sheathed blade%'"));
  CuAssertIntEquals(tc, 0,
                    transactions_value(w.observer, "SELECT COUNT(*) FROM player_save_objs_sheathed "
                                                   "WHERE owner_name = '" SAVES_OWNER "'"));
  CuAssertStrEquals(tc, "", transactions_staff_heard(&w));

  extract_obj(sheath->sheath_primary);
  extract_obj(sheath->sheath_secondary);
  sheath->sheath_primary = NULL;
  sheath->sheath_secondary = NULL;
  transactions_world_end(tc, &w);
}

/* The idle save and the cryo save commit what they write, in place of the
 * last save's rows. */
void Test_idle_and_cryo_saves_commit_in_place_of_the_last_save(CuTest *tc)
{
  struct transactions_world w;

  if (!transactions_world_begin(tc, &w))
    return;

  saves_carry(&w, "a first gem");
  saves_carry(&w, "a second gem");
  CuAssertTrue(tc, Crash_crashsave(&w.owner));
  CuAssertIntEquals(tc, 2, saves_player_rows(&w));

  /* The idle save stores the three gems carried now and takes them along. */
  saves_carry(&w, "a third gem");
  Crash_idlesave(&w.owner);
  CuAssertTrue(tc, (conn->server_status & SERVER_STATUS_IN_TRANS) == 0);
  CuAssertIntEquals(tc, 3, saves_player_rows(&w));
  CuAssertPtrEquals(tc, NULL, w.owner.carrying);

  /* With nothing left to store, it keeps no row. */
  Crash_idlesave(&w.owner);
  CuAssertTrue(tc, (conn->server_status & SERVER_STATUS_IN_TRANS) == 0);
  CuAssertIntEquals(tc, 0, saves_player_rows(&w));

  /* The cryo save replaces the last save too. */
  saves_carry(&w, "a first gem");
  CuAssertTrue(tc, Crash_crashsave(&w.owner));
  saves_carry(&w, "a second gem");
  Crash_cryosave(&w.owner, 0);
  CuAssertTrue(tc, (conn->server_status & SERVER_STATUS_IN_TRANS) == 0);
  CuAssertIntEquals(tc, 2, saves_player_rows(&w));
  CuAssertPtrEquals(tc, NULL, w.owner.carrying);
  REMOVE_BIT_AR(PLR_FLAGS(&w.owner), PLR_CRYO);

  /* And the rent save, whose commit was already there. */
  saves_carry(&w, "a first gem");
  Crash_rentsave(&w.owner, 0);
  CuAssertIntEquals(tc, 1, saves_player_rows(&w));
  CuAssertPtrEquals(tc, NULL, w.owner.carrying);

  transactions_world_end(tc, &w);
}

/* A character who leaves the game takes its objects along whether the save
 * was written or not: what stayed in its hands would be dropped in the room,
 * beside a save that holds it. So a save that did not reach the database is
 * written a second time. */
void Test_leaving_save_is_written_again_and_takes_the_objects_along(CuTest *tc)
{
  static char no_socket[] = "/nonexistent/s13.sock";
  struct transactions_world w;
  struct obj_data *worn;
  char *unix_socket;
  unsigned int tcp_port;

  if (!transactions_world_begin(tc, &w))
    return;

  saves_carry(&w, "a first gem");
  CuAssertTrue(tc, Crash_crashsave(&w.owner));
  CuAssertIntEquals(tc, 1, saves_player_rows(&w));

  /* The connection goes at the first row. The second attempt stores the
   * worn gem with the two carried ones. */
  saves_carry(&w, "a second gem");
  worn = read_object(SAVES_OBJECT_VNUM, VIRTUAL);
  worn->short_description = strdup("a worn gem");
  worn->worn_by = &w.owner;
  worn->worn_on = WEAR_HOLD_1;
  GET_EQ(&w.owner, WEAR_HOLD_1) = worn;
  mysql_test_drop_connection_at("INSERT INTO player_save_objs (", 1, FALSE);
  Crash_rentsave(&w.owner, 0);
  CuAssertIntEquals(tc, 3, saves_player_rows(&w));
  CuAssertIntEquals(
      tc, 1,
      transactions_value(w.observer,
                         "SELECT COUNT(*) FROM player_save_objs WHERE name = '" SAVES_OWNER
                         "' AND serialized_obj LIKE '%a worn gem%'"));
  CuAssertPtrEquals(tc, NULL, w.owner.carrying);
  CuAssertPtrEquals(tc, NULL, GET_EQ(&w.owner, WEAR_HOLD_1));
  CuAssertStrEquals(tc, "", transactions_staff_heard(&w));

  /* The COMMIT is run and its reply lost: written again, nothing twice. */
  saves_carry(&w, "a first gem");
  saves_carry(&w, "a second gem");
  mysql_test_drop_connection_at("commit", 1, TRUE);
  Crash_cryosave(&w.owner, 0);
  REMOVE_BIT_AR(PLR_FLAGS(&w.owner), PLR_CRYO);
  CuAssertIntEquals(tc, 2, saves_player_rows(&w));
  CuAssertPtrEquals(tc, NULL, w.owner.carrying);
  CuAssertStrEquals(tc, "", transactions_staff_heard(&w));

  /* The database is away for both attempts: the last save stands, the gem
   * leaves with the character, and the staff are told. */
  saves_carry(&w, "a third gem");
  tcp_port = conn->port;
  unix_socket = conn->unix_socket;
  conn->port = 1;
  conn->unix_socket = no_socket;
  transactions_drop_socket(conn);
  Crash_idlesave(&w.owner);
  conn->port = tcp_port;
  conn->unix_socket = unix_socket;
  CuAssertIntEquals(tc, 2, saves_player_rows(&w));
  CuAssertPtrEquals(tc, NULL, w.owner.carrying);
  CuAssertTrue(tc, strstr(transactions_staff_heard(&w),
                          SAVES_OWNER "'s objects could not be saved "
                                      "as the character left the game") != NULL);

  transactions_world_end(tc, &w);
}

/* What a player sorted into a bag is held nowhere else. The cryo save and
 * the idle save replace the last save's rows, so they write the bags too. */
void Test_cryo_and_idle_saves_keep_what_is_in_the_bags(CuTest *tc)
{
  struct transactions_world w;
  struct obj_data *bagged;

  if (!transactions_world_begin(tc, &w))
    return;

  saves_carry(&w, "a carried gem");
  bagged = read_object(SAVES_OBJECT_VNUM, VIRTUAL);
  bagged->short_description = strdup("a bagged gem");
  GET_OBJ_SORT(bagged) = 1;
  w.bags.bag1 = bagged;
  CuAssertTrue(tc, Crash_crashsave(&w.owner));
  CuAssertIntEquals(tc, 2, saves_player_rows(&w));

  Crash_cryosave(&w.owner, 0);
  REMOVE_BIT_AR(PLR_FLAGS(&w.owner), PLR_CRYO);
  CuAssertPtrEquals(tc, NULL, w.owner.carrying);
  CuAssertIntEquals(tc, 2, saves_player_rows(&w));

  /* Nothing carried or worn, one gem in a bag. */
  Crash_idlesave(&w.owner);
  CuAssertIntEquals(tc, 1, saves_player_rows(&w));
  CuAssertIntEquals(
      tc, 1,
      transactions_value(w.observer,
                         "SELECT COUNT(*) FROM player_save_objs WHERE name = '" SAVES_OWNER
                         "' AND serialized_obj LIKE '%a bagged gem%'"));

  w.bags.bag1 = NULL;
  extract_obj(bagged);
  transactions_world_end(tc, &w);
}

/* ------------------------------------------------------------------------ */
/* Vessel sites                                                             */
/* ------------------------------------------------------------------------ */

extern struct greyhawk_ship_data greyhawk_ships[GREYHAWK_MAXSHIPS];

/* A high fleet slot keeps this hull clear of the rest of the suite. */
#define TRANSACTIONS_SHIP 497

/* The first column of the query's first row as text; "" when there is none. */
static const char *transactions_text(MYSQL *connection, const char *query)
{
  static char text[128];
  MYSQL_RES *result;
  MYSQL_ROW row;

  text[0] = '\0';
  if (mysql_query(connection, query) != 0)
    return text;
  result = mysql_store_result(connection);
  if (result == NULL)
    return text;
  row = mysql_fetch_row(result);
  if (row != NULL && row[0] != NULL)
    strlcpy(text, row[0], sizeof(text));
  mysql_free_result(result);
  return text;
}

static void transactions_hull_end(void)
{
  mysql_query(conn, "DELETE FROM ship_interiors WHERE ship_id = 497");
  memset(&greyhawk_ships[TRANSACTIONS_SHIP], 0, sizeof(greyhawk_ships[0]));
}

/* The test hull, owned by Oldowner in memory and in the real tables. */
static struct greyhawk_ship_data *transactions_hull_begin(CuTest *tc)
{
  struct greyhawk_ship_data *ship = &greyhawk_ships[TRANSACTIONS_SHIP];

  vessel_persistence_ensure_schema();
  vessel_ownership_ensure_schema();
  transactions_hull_end();
  ship->active = TRUE;
  ship->shipnum = TRANSACTIONS_SHIP;
  ship->vessel_type = VESSEL_SHIP;
  ship->docked_to_ship = -1;
  strlcpy(ship->id, "TX", sizeof(ship->id));
  strlcpy(ship->name, "the Thirteen", sizeof(ship->name));
  strlcpy(ship->owner, "Oldowner", sizeof(ship->owner));
  vessel_initialize_condition(ship, vessel_class_condition(VESSEL_SHIP)->beam_armor);
  CuAssertIntEquals(tc, 0,
                    mysql_query(conn, "INSERT INTO ship_interiors (ship_id, owner) "
                                      "VALUES (497, 'Oldowner')"));
  return ship;
}

static const char *transactions_hull_owner(void)
{
  return transactions_text(
      conn, "SELECT owner FROM ship_interiors WHERE ship_id = 497 LOCK IN SHARE MODE");
}

/* A hull changes hands in memory when, and only when, it did in the database,
 * also when the COMMIT got no reply. */
void Test_vessel_owner_transfer_reads_back_a_commit_without_a_reply(CuTest *tc)
{
  struct transactions_database database;
  struct greyhawk_ship_data *ship;

  if (!transactions_begin(tc, &database))
    return;
  ship = transactions_hull_begin(tc);

  /* The transfer was committed, though its reply was lost: she is the new
   * owner's, and the old owner's consent to be fought is gone. */
  ship->pvp_grace_until = 12345;
  strlcpy(ship->pvp_grace_attacker, "Rival", sizeof(ship->pvp_grace_attacker));
  mysql_test_drop_connection_at("COMMIT", 1, TRUE);
  CuAssertTrue(tc, vessel_transfer_owner(ship, "Newowner"));
  CuAssertStrEquals(tc, "Newowner", ship->owner);
  CuAssertStrEquals(tc, "Newowner", transactions_hull_owner());
  CuAssertTrue(tc, ship->pvp_grace_until == 0);

  /* The COMMIT never arrived, so the server rolled back: she stays, with
   * her standing consent. */
  ship->pvp_grace_until = 777;
  strlcpy(ship->pvp_grace_attacker, "Rival", sizeof(ship->pvp_grace_attacker));
  mysql_test_drop_connection_at("COMMIT", 1, FALSE);
  CuAssertTrue(tc, !vessel_transfer_owner(ship, "Thirdowner"));
  CuAssertStrEquals(tc, "Newowner", ship->owner);
  CuAssertStrEquals(tc, "Newowner", transactions_hull_owner());
  CuAssertTrue(tc, ship->pvp_grace_until == 777);
  CuAssertStrEquals(tc, "Rival", ship->pvp_grace_attacker);

  /* The connection lost between the two writes: the owner written before it
   * is rolled back with the rest. */
  mysql_test_drop_connection_at("REPLACE INTO ship_runtime_state", 1, FALSE);
  CuAssertTrue(tc, !vessel_transfer_owner(ship, "Thirdowner"));
  CuAssertStrEquals(tc, "Newowner", ship->owner);
  CuAssertStrEquals(tc, "Newowner", transactions_hull_owner());

  CuAssertTrue(tc, vessel_transfer_owner(ship, "Thirdowner"));
  CuAssertStrEquals(tc, "Thirdowner", transactions_hull_owner());

  transactions_hull_end();
  transactions_end(&database);
}

/* An offense is added to the standing bounty, so it is not recorded when the
 * standing bounty cannot be read: read as none, the offense alone would
 * replace it. */
void Test_vessel_bounty_is_not_overwritten_when_it_cannot_be_read(CuTest *tc)
{
  struct transactions_database database;

  if (!transactions_begin(tc, &database))
    return;
  vessel_piracy_ensure_schema();
  mysql_query(conn, "DELETE FROM vessel_bounties WHERE player_name = 'Sthirteen'");
  CuAssertTrue(tc, vessel_bounty_record_offense("Sthirteen", 400));
  CuAssertIntEquals(tc, 400, vessel_get_bounty("Sthirteen"));

  /* The connection goes as the bounty is read. No transaction is open, so
   * it reconnects by itself, and the write that follows would land. */
  mysql_test_drop_connection_at("SELECT bounty", 1, FALSE);
  CuAssertTrue(tc, !vessel_bounty_record_offense("Sthirteen", 100));
  CuAssertIntEquals(tc, 400, vessel_get_bounty("Sthirteen"));

  CuAssertTrue(tc, vessel_bounty_record_offense("Sthirteen", 100));
  CuAssertIntEquals(tc, 500, vessel_get_bounty("Sthirteen"));

  mysql_query(conn, "DELETE FROM vessel_bounties WHERE player_name = 'Sthirteen'");
  transactions_end(&database);
}

static int transactions_event_entries(void)
{
  return transactions_value(conn, "SELECT COALESCE(SUM(entries), 0) FROM vessel_event_leaderboards "
                                  "WHERE player_idnum = 4314 LOCK IN SHARE MODE");
}

/* A finished event's scores are added once: when the COMMIT gets no reply,
 * the event's own row says whether they went in, and no retry adds them
 * again. */
void Test_vessel_event_finish_reads_back_a_commit_without_a_reply(CuTest *tc)
{
  static char no_socket[] = "/nonexistent/s13.sock";
  struct transactions_world w;
  struct greyhawk_ship_data *ship;
  const char *heard;
  char *unix_socket;
  unsigned int tcp_port;

  if (!transactions_world_begin(tc, &w))
    return;
  vessel_event_ensure_schema();
  mysql_query(conn, "DELETE FROM vessel_event_leaderboards WHERE player_idnum = 4314");
  mysql_query(conn, "DELETE FROM vessel_showcase_events WHERE staff_idnum = 4314");
  ship = transactions_hull_begin(tc);
  strlcpy(ship->owner, GET_NAME(&w.staff), sizeof(ship->owner));
  ship->bridge_room = SAVES_HOUSE_VNUM;
  w.room.ship = ship;
  IN_ROOM(&w.staff) = 0;
  GET_IDNUM(&w.staff) = 4314;

  /* The completion was committed, though its reply was lost. */
  do_vevent(&w.staff, "start skirmish", 0, 0);
  do_vevent(&w.staff, "join red", 0, 0);
  transactions_staff_heard(&w);
  mysql_test_drop_connection_at("COMMIT", 1, TRUE);
  do_vevent(&w.staff, "end", 0, 0);
  heard = transactions_staff_heard(&w);
  CuAssertTrue(tc, strstr(heard, "Vessel event completed and scored.") != NULL);
  CuAssertIntEquals(tc, 1, transactions_event_entries());
  CuAssertStrEquals(tc, "completed",
                    transactions_text(conn, "SELECT status FROM vessel_showcase_events WHERE "
                                            "staff_idnum = 4314 ORDER BY event_id DESC LIMIT 1"));

  /* The COMMIT never arrived, so the server rolled back: the event stays
   * open, and the retry scores it once. */
  do_vevent(&w.staff, "start skirmish", 0, 0);
  do_vevent(&w.staff, "join red", 0, 0);
  transactions_staff_heard(&w);
  mysql_test_drop_connection_at("COMMIT", 1, FALSE);
  do_vevent(&w.staff, "end", 0, 0);
  heard = transactions_staff_heard(&w);
  CuAssertTrue(tc, strstr(heard, "The event remains open because finalization failed") != NULL);
  CuAssertIntEquals(tc, 1, transactions_event_entries());
  do_vevent(&w.staff, "end", 0, 0);
  heard = transactions_staff_heard(&w);
  CuAssertTrue(tc, strstr(heard, "Vessel event completed and scored.") != NULL);
  CuAssertIntEquals(tc, 2, transactions_event_entries());

  /* The completion was committed, its reply was lost, and the database is
   * away when the row is read back. The staff are told to retry; the retry
   * finds the event ended, adds no score again and leaves its row alone. */
  do_vevent(&w.staff, "start skirmish", 0, 0);
  do_vevent(&w.staff, "join red", 0, 0);
  transactions_staff_heard(&w);
  mysql_test_drop_connection_at("COMMIT", 1, TRUE);
  tcp_port = conn->port;
  unix_socket = conn->unix_socket;
  conn->port = 1;
  conn->unix_socket = no_socket;
  do_vevent(&w.staff, "end", 0, 0);
  conn->port = tcp_port;
  conn->unix_socket = unix_socket;
  heard = transactions_staff_heard(&w);
  CuAssertTrue(tc, strstr(heard, "The event remains open because finalization failed") != NULL);
  CuAssertIntEquals(tc, 3, transactions_event_entries());
  do_vevent(&w.staff, "end", 0, 0);
  heard = transactions_staff_heard(&w);
  CuAssertTrue(tc, strstr(heard, "Vessel event completed and scored.") != NULL);
  CuAssertIntEquals(tc, 3, transactions_event_entries());
  CuAssertStrEquals(tc, "completed",
                    transactions_text(conn, "SELECT status FROM vessel_showcase_events WHERE "
                                            "staff_idnum = 4314 ORDER BY event_id DESC LIMIT 1"));
  CuAssertStrEquals(tc, "ended by staff",
                    transactions_text(conn, "SELECT end_reason FROM vessel_showcase_events WHERE "
                                            "staff_idnum = 4314 ORDER BY event_id DESC LIMIT 1"));

  mysql_query(conn, "DELETE FROM vessel_event_leaderboards WHERE player_idnum = 4314");
  mysql_query(conn, "DELETE FROM vessel_showcase_events WHERE staff_idnum = 4314");
  w.room.ship = NULL;
  transactions_hull_end();
  transactions_world_end(tc, &w);
}

/* ------------------------------------------------------------------------ */
/* Help                                                                     */
/* ------------------------------------------------------------------------ */

#define TRANSACTIONS_HELP_TAG "sthirteentopic"

/* The help synchronization lock belongs to its session and goes with it; the
 * holder is told when it asks. */
void Test_help_sync_lock_is_lost_with_its_session(CuTest *tc)
{
  struct transactions_database database;

  if (!transactions_begin(tc, &database))
    return;

  CuAssertTrue(tc, !help_sync_database_lock_held());
  CuAssertTrue(tc, help_sync_database_lock_acquire(1));
  CuAssertTrue(tc, help_sync_database_lock_held());

  /* The connection drops and reconnects by itself at its next statement. */
  transactions_drop_socket(conn);
  CuAssertTrue(tc, !help_sync_database_lock_held());

  CuAssertTrue(tc, help_sync_database_lock_acquire(1));
  CuAssertTrue(tc, help_sync_database_lock_held());
  help_sync_database_lock_release();
  CuAssertTrue(tc, !help_sync_database_lock_held());

  transactions_end(&database);
}

static void transactions_help_rows_end(void)
{
  mysql_query(conn, "DELETE FROM help_keywords WHERE help_tag = '" TRANSACTIONS_HELP_TAG "'");
  mysql_query(conn, "DELETE FROM help_versions WHERE tag = '" TRANSACTIONS_HELP_TAG "'");
  mysql_query(conn, "DELETE FROM help_entries WHERE tag = '" TRANSACTIONS_HELP_TAG "'");
}

/* The staff member edits the help entry to this text and these keywords, and
 * answers the save prompt with yes. Returns what the editor printed. */
static const char *transactions_hedit_save(struct transactions_world *w, const char *text,
                                           char **keywords)
{
  struct descriptor_data *d = &w->staff_descriptor;
  struct help_keyword_list *node;
  char yes[] = "y";
  int i;

  CREATE(d->olc, struct oasis_olc_data, 1);
  CREATE(OLC_HELP(d), struct help_entry_list, 1);
  OLC_HELP(d)->tag = strdup(TRANSACTIONS_HELP_TAG);
  OLC_HELP(d)->entry = strdup(text);
  for (i = 0; keywords[i] != NULL; i++)
  {
    CREATE(node, struct help_keyword_list, 1);
    node->keyword = keywords[i];
    node->next = OLC_HELP(d)->keyword_list;
    OLC_HELP(d)->keyword_list = node;
  }
  OLC_MODE(d) = HEDIT_CONFIRM_SAVESTRING;
  STATE(d) = CON_HEDIT;
  transactions_staff_heard(w);
  hedit_parse(d, yes);
  /* A refused save leaves the editor open. */
  cleanup_olc(d, CLEANUP_ALL);
  STATE(d) = CON_PLAYING;
  return transactions_staff_heard(w);
}

static int transactions_help_keywords(void)
{
  return transactions_value(
      conn, "SELECT COUNT(*) FROM help_keywords WHERE help_tag = '" TRANSACTIONS_HELP_TAG "'");
}

static const char *transactions_help_text(void)
{
  return transactions_text(
      conn, "SELECT entry FROM help_entries WHERE tag = '" TRANSACTIONS_HELP_TAG "'");
}

/* A help save deletes the keywords the editor removed, is lost whole with
 * its connection, and is refused when its session lost the help
 * synchronization lock. */
void Test_hedit_save_removes_keywords_and_is_whole_or_refused(CuTest *tc)
{
  static char alpha[] = "sthirteenalpha";
  static char beta[] = "sthirteenbeta";
  char *both[] = {alpha, beta, NULL};
  char *one[] = {alpha, NULL};
  struct transactions_world w;
  const char *heard;

  if (!transactions_world_begin(tc, &w))
    return;
  transactions_help_rows_end();
  IN_ROOM(&w.staff) = 0;

  heard = transactions_hedit_save(&w, "First text.\r\n", both);
  CuAssertTrue(tc, strstr(heard, "Help saved successfully.") != NULL);
  CuAssertIntEquals(tc, 2, transactions_help_keywords());

  /* The editor dropped a keyword: its row goes. */
  heard = transactions_hedit_save(&w, "Second text.\r\n", one);
  CuAssertTrue(tc, strstr(heard, "Help saved successfully.") != NULL);
  CuAssertIntEquals(tc, 1, transactions_help_keywords());
  CuAssertStrEquals(tc, "Second text.\n", transactions_help_text());

  /* The connection goes at the version history, whose result the save does
   * not check: nothing after it lands on its own. */
  mysql_test_drop_connection_at("INSERT INTO help_versions", 1, FALSE);
  heard = transactions_hedit_save(&w, "Third text.\r\n", both);
  CuAssertTrue(tc, strstr(heard, "All changes have been rolled back.") != NULL);
  CuAssertStrEquals(tc, "Second text.\n", transactions_help_text());
  CuAssertIntEquals(tc, 1, transactions_help_keywords());

  /* The connection goes between taking the lock and opening the transaction,
   * and reconnects by itself: the session that saves no longer holds the
   * lock, and the save is refused. */
  mysql_test_drop_connection_at("START TRANSACTION", 1, FALSE);
  heard = transactions_hedit_save(&w, "Fourth text.\r\n", both);
  CuAssertTrue(tc, strstr(heard, "The help synchronization lock was lost") != NULL);
  CuAssertStrEquals(tc, "Second text.\n", transactions_help_text());

  heard = transactions_hedit_save(&w, "Fifth text.\r\n", both);
  CuAssertTrue(tc, strstr(heard, "Help saved successfully.") != NULL);
  CuAssertStrEquals(tc, "Fifth text.\n", transactions_help_text());
  CuAssertIntEquals(tc, 2, transactions_help_keywords());

  transactions_help_rows_end();
  transactions_world_end(tc, &w);
}
