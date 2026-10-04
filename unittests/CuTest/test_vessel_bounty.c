/* Vessel bounties: decay after a quiet day, and pay-off at a lawful port. */

#include "CuTest.h"

#include "conf.h"
#include "../../src/core/sysdep.h"
#include "../../src/core/structs.h"
#include "../../src/core/utils.h"
#include "../../src/core/comm.h"
#include "../../src/core/db.h"
#include "../../src/core/interpreter.h"
#include "../../src/database/mysql.h"
#include "../../src/net/protocol.h"
#include "../../src/vessels/vessels.h"
#include "test_vessel_stores.h"

#include <limits.h>
#include <stdlib.h>
#include <string.h>

#define BOUNTY_TEST_DAY ((long long)VESSEL_BOUNTY_DAY_SECONDS)

static MYSQL *bounty_open_test_database(void)
{
  const char *port_text;
  MYSQL *connection;

  if (getenv("LUMINARI_TEST_MYSQL_HOST") == NULL || getenv("LUMINARI_TEST_MYSQL_USER") == NULL ||
      getenv("LUMINARI_TEST_MYSQL_PASSWORD") == NULL ||
      getenv("LUMINARI_TEST_MYSQL_DATABASE") == NULL)
  {
    return NULL;
  }
  port_text = getenv("LUMINARI_TEST_MYSQL_PORT");
  connection = mysql_init(NULL);
  if (connection == NULL)
  {
    return NULL;
  }
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

void Test_vessel_bounty_decays_after_a_quiet_day(CuTest *tc)
{
  /* The first full day after an offense keeps the whole bounty. */
  CuAssertIntEquals(tc, 1000, vessel_bounty_after_decay(1000, 0));
  CuAssertIntEquals(tc, 1000, vessel_bounty_after_decay(1000, BOUNTY_TEST_DAY));
  CuAssertIntEquals(tc, 1000, vessel_bounty_after_decay(1000, 2 * BOUNTY_TEST_DAY - 1));
  CuAssertIntEquals(tc, 1000, vessel_bounty_after_decay(1000, -5));

  /* Then 5% of its size per further day, gone after 21 quiet days. */
  CuAssertIntEquals(tc, 950, vessel_bounty_after_decay(1000, 2 * BOUNTY_TEST_DAY));
  CuAssertIntEquals(tc, 500, vessel_bounty_after_decay(1000, 11 * BOUNTY_TEST_DAY));
  CuAssertIntEquals(tc, 50, vessel_bounty_after_decay(1000, 20 * BOUNTY_TEST_DAY));
  CuAssertIntEquals(tc, 0, vessel_bounty_after_decay(1000, 21 * BOUNTY_TEST_DAY));
  CuAssertIntEquals(tc, 0, vessel_bounty_after_decay(1000, 400 * BOUNTY_TEST_DAY));
  CuAssertIntEquals(tc, 0, vessel_bounty_after_decay(0, 3 * BOUNTY_TEST_DAY));
  CuAssertIntEquals(tc, INT_MAX - INT_MAX / 20,
                    vessel_bounty_after_decay(INT_MAX, 2 * BOUNTY_TEST_DAY));

  /* A lawful port clears a bounty for 125% of it, rounded up. */
  CuAssertIntEquals(tc, 625, vessel_bounty_payoff_cost(500));
  CuAssertIntEquals(tc, 2, vessel_bounty_payoff_cost(1));
  CuAssertIntEquals(tc, 0, vessel_bounty_payoff_cost(0));
  CuAssertIntEquals(tc, INT_MAX, vessel_bounty_payoff_cost(INT_MAX));
}

void Test_vessel_bounty_accrues_decays_and_pays_off(CuTest *tc)
{
  const char *enabled = getenv("LUMINARI_TEST_MYSQL_ENABLE");
  char output[4096];
  struct char_data pirate;
  struct player_special_data specials;
  struct descriptor_data descriptor;
  struct room_data rooms[2];
  struct room_data *saved_world;
  room_rnum saved_top_of_world;
  struct vessel_test_stores stores;
  MYSQL *saved_conn;
  MYSQL *connection;
  bool saved_mysql_available;

  if (enabled == NULL || strcmp(enabled, "1") != 0)
  {
    return;
  }

  connection = bounty_open_test_database();
  if (connection == NULL)
  {
    CuFail(tc, "could not connect to the explicitly configured test database");
    return;
  }
  if (mysql_query(connection, "CREATE TEMPORARY TABLE vessel_bounties ("
                              "player_name VARCHAR(64) PRIMARY KEY, "
                              "bounty INT NOT NULL DEFAULT 0, "
                              "marque_until INT NOT NULL DEFAULT 0, "
                              "last_offense_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP, "
                              "updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP "
                              "ON UPDATE CURRENT_TIMESTAMP)") != 0)
  {
    mysql_close(connection);
    CuFail(tc, "could not create the isolated bounty fixture");
    return;
  }

  saved_conn = conn;
  saved_mysql_available = mysql_available;
  saved_world = world;
  saved_top_of_world = top_of_world;
  conn = connection;
  mysql_available = TRUE;
  vessel_piracy_clear_laws();

  /* Offenses accumulate; the decay clock restarts with each one. */
  vessel_add_bounty("Corr", 400);
  CuAssertIntEquals(tc, 400, vessel_get_bounty("Corr"));
  CuAssertTrue(tc, mysql_query(conn, "UPDATE vessel_bounties SET last_offense_at = "
                                     "NOW() - INTERVAL 3 DAY WHERE player_name = 'Corr'") == 0);
  CuAssertIntEquals(tc, 360, vessel_get_bounty("Corr"));
  CuAssertTrue(tc, vessel_bounty_record_offense("Corr", 100));
  CuAssertIntEquals(tc, 460, vessel_get_bounty("Corr"));
  CuAssertTrue(tc, !vessel_bounty_record_offense("Corr", 0));

  /* Pay-off happens only in a port, and only in full. */
  memset(rooms, 0, sizeof(rooms));
  rooms[0].number = 100;
  rooms[1].number = 101;
  SET_BIT_AR(rooms[1].room_flags, ROOM_DOCKABLE);
  world = rooms;
  top_of_world = 1;
  memset(&pirate, 0, sizeof(pirate));
  memset(&specials, 0, sizeof(specials));
  memset(&descriptor, 0, sizeof(descriptor));
  memset(output, 0, sizeof(output));
  pirate.player_specials = &specials;
  pirate.player.name = CuMutableString("Corr");
  pirate.player.level = 20;
  pirate.desc = &descriptor;
  descriptor.character = &pirate;
  descriptor.output = output;
  descriptor.bufspace = sizeof(output) - 1;
  descriptor.pProtocol = ProtocolCreate();
  CuAssertPtrNotNull(tc, descriptor.pProtocol);

  /* The pay-off is saved with the captain. */
  IN_ROOM(&pirate) = 0;
  GET_GOLD(&pirate) = 1000;
  memset(&stores, 0, sizeof(stores));
  vessel_test_pfile_begin(tc, &stores, &pirate);
  do_bounty(&pirate, "pay", 0, 0);
  CuAssertTrue(tc, strstr(output, "lawful port's admiralty office") != NULL);
  CuAssertIntEquals(tc, 460, vessel_get_bounty("Corr"));

  IN_ROOM(&pirate) = 1;
  GET_GOLD(&pirate) = 500;
  do_bounty(&pirate, "pay", 0, 0);
  CuAssertIntEquals(tc, 460, vessel_get_bounty("Corr"));
  CuAssertIntEquals(tc, 500, GET_GOLD(&pirate));

  GET_GOLD(&pirate) = 1000;
  do_bounty(&pirate, "pay", 0, 0);
  CuAssertIntEquals(tc, 0, vessel_get_bounty("Corr"));
  CuAssertIntEquals(tc, 1000 - 575, GET_GOLD(&pirate));

  /* Cleared, the captain reads that in the second person. */
  memset(output, 0, sizeof(output));
  descriptor.bufptr = 0;
  descriptor.bufspace = sizeof(output) - 1;
  do_bounty(&pirate, "", 0, 0);
  CuAssertTrue(tc, strstr(output, "You carry no price.") != NULL);

  vessel_test_pfile_end(tc, &stores);
  ProtocolDestroy(descriptor.pProtocol);
  vessel_piracy_clear_laws();
  world = saved_world;
  top_of_world = saved_top_of_world;
  conn = saved_conn;
  mysql_available = saved_mysql_available;
  mysql_close(connection);
}
