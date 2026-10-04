#ifndef TEST_VESSEL_STORES_H
#define TEST_VESSEL_STORES_H

/* The two stores a vessel purchase or payout writes (study S15), for a test:
 * the test database as the game's connection with a hull's row, and a scratch
 * player directory with an index, so a captain's saves succeed. */

#include "CuTest.h"

#include "conf.h"
#include "../../src/core/sysdep.h"
#include "../../src/core/structs.h"
#include "../../src/core/db.h"
#include "../../src/database/mysql.h"

struct vessel_test_stores
{
  MYSQL *connection;
  MYSQL *saved_conn;
  bool saved_mysql_available;
  int shipnum;
  struct char_data *captain;
  char scratch[64];
  char home[PATH_MAX];
  struct player_index_element index[1];
  struct player_index_element *saved_table;
  int saved_top;
  unsigned int saved_port;
  char *saved_unix_socket;
};

/* A scratch player directory and index for the captain, whose file is saved. */
void vessel_test_pfile_begin(CuTest *tc, struct vessel_test_stores *stores,
                             struct char_data *captain);
void vessel_test_pfile_end(CuTest *tc, struct vessel_test_stores *stores);

/* Both stores: the database with the hull's interior row (which her other rows
 * reference), and the captain's player file.
 * @return FALSE when the database cases are off */
bool vessel_test_stores_begin(CuTest *tc, struct vessel_test_stores *stores, int shipnum,
                              struct char_data *captain);
void vessel_test_stores_end(CuTest *tc, struct vessel_test_stores *stores);

/* Put the database out of reach of the reconnect a lost connection needs, so
 * a command whose connection is lost finds it gone for the rest of its
 * writes; or bring it back. */
void vessel_test_database_away(struct vessel_test_stores *stores, bool away);

/* The gold the captain's player file holds. */
int vessel_test_file_gold(CuTest *tc, const struct vessel_test_stores *stores);

/* The first column of a one-row query on the test connection, as a number. */
long long vessel_test_number(CuTest *tc, const struct vessel_test_stores *stores,
                             const char *query);

#endif
