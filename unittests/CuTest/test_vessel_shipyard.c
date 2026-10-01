/* Vessel shipyard rules: hull minimum levels, the owned-hull cap, and the
 * for-sale shipyard catalog. */

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

extern struct greyhawk_ship_data greyhawk_ships[GREYHAWK_MAXSHIPS];

/* High fleet slots keep these fixtures clear of the rest of the suite. */
#define SHIPYARD_FIRST_SLOT 470
#define SHIPYARD_SLOT_COUNT 4

struct shipyard_player
{
  struct char_data ch;
  struct player_special_data specials;
};

static void shipyard_player_init(struct shipyard_player *player, const char *name, int level)
{
  memset(player, 0, sizeof(*player));
  player->ch.player_specials = &player->specials;
  player->ch.player.name = CuMutableString(name);
  player->ch.player.level = level;
  IN_ROOM(&player->ch) = NOWHERE;
  GET_POS(&player->ch) = POS_STANDING;
}

static void shipyard_own_ships(const char *owner, int count)
{
  struct greyhawk_ship_data *ship;
  int i;

  for (i = 0; i < SHIPYARD_SLOT_COUNT; i++)
  {
    ship = &greyhawk_ships[SHIPYARD_FIRST_SLOT + i];
    memset(ship, 0, sizeof(*ship));
    if (i < count)
    {
      ship->active = TRUE;
      ship->shipnum = SHIPYARD_FIRST_SLOT + i;
      strlcpy(ship->owner, owner, sizeof(ship->owner));
    }
  }
}

static MYSQL *shipyard_open_test_database(void)
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

void Test_vessel_hull_class_minimum_levels(CuTest *tc)
{
  CuAssertIntEquals(tc, 1, vessel_class_min_level(VESSEL_RAFT));
  CuAssertIntEquals(tc, 1, vessel_class_min_level(VESSEL_BOAT));
  CuAssertIntEquals(tc, 16, vessel_class_min_level(VESSEL_SHIP));
  CuAssertIntEquals(tc, 22, vessel_class_min_level(VESSEL_WARSHIP));
  CuAssertIntEquals(tc, 24, vessel_class_min_level(VESSEL_AIRSHIP));
  CuAssertIntEquals(tc, 23, vessel_class_min_level(VESSEL_SUBMARINE));
  CuAssertIntEquals(tc, 21, vessel_class_min_level(VESSEL_TRANSPORT));
  CuAssertIntEquals(tc, 25, vessel_class_min_level(VESSEL_MAGICAL));
  CuAssertIntEquals(tc, 16, vessel_class_min_level(-1));
  CuAssertIntEquals(tc, 16, vessel_class_min_level(NUM_VESSEL_TYPES));

  /* A prototype's own level overrides the class; 0 keeps the class. */
  CuAssertIntEquals(tc, 22, vessel_prototype_min_level(VESSEL_WARSHIP, 0));
  CuAssertIntEquals(tc, 12, vessel_prototype_min_level(VESSEL_WARSHIP, 12));
}

void Test_vessel_departures_need_the_hull_level(CuTest *tc)
{
  struct greyhawk_ship_data ship;
  struct shipyard_player junior;
  struct shipyard_player senior;
  struct shipyard_player staff;
  struct char_data npc;
  bool saved_mysql_available;

  saved_mysql_available = mysql_available;
  mysql_available = FALSE;
  memset(&ship, 0, sizeof(ship));
  ship.vessel_type = VESSEL_WARSHIP;
  strlcpy(ship.name, "the Gull", sizeof(ship.name));
  shipyard_player_init(&junior, "Mira", 21);
  shipyard_player_init(&senior, "Corr", 22);
  shipyard_player_init(&staff, "Zusuk", LVL_IMMORT);
  memset(&npc, 0, sizeof(npc));
  SET_BIT_AR(MOB_FLAGS(&npc), MOB_ISNPC);

  CuAssertIntEquals(tc, 22, vessel_ship_min_level(&ship));
  CuAssertTrue(tc, vessel_helm_level_refused(&junior.ch, &ship));
  CuAssertTrue(tc, !vessel_helm_level_refused(&senior.ch, &ship));
  CuAssertTrue(tc, !vessel_helm_level_refused(&staff.ch, &ship));
  CuAssertTrue(tc, !vessel_helm_level_refused(&npc, &ship));

  ship.vessel_type = VESSEL_RAFT;
  junior.ch.player.level = 1;
  CuAssertTrue(tc, !vessel_helm_level_refused(&junior.ch, &ship));

  /* A prototype's own level may exceed the class, so an unreadable one
   * refuses every mortal departure instead of falling back to the class. */
  ship.prototype_id = 7;
  CuAssertIntEquals(tc, VESSEL_MIN_LEVEL_UNKNOWN, vessel_ship_min_level(&ship));
  CuAssertTrue(tc, vessel_helm_level_refused(&junior.ch, &ship));
  CuAssertTrue(tc, vessel_helm_level_refused(&senior.ch, &ship));
  CuAssertTrue(tc, !vessel_helm_level_refused(&staff.ch, &ship));
  CuAssertTrue(tc, !vessel_helm_level_refused(&npc, &ship));

  mysql_available = saved_mysql_available;
}

void Test_vessel_owner_cap_counts_owned_hulls(CuTest *tc)
{
  struct shipyard_player owner;
  struct shipyard_player staff;
  int saved_cap;

  saved_cap = CONFIG_VESSEL_OWNER_CAP;
  shipyard_player_init(&owner, "Corr", 20);
  shipyard_player_init(&staff, "Zusuk", LVL_IMMORT);
  CONFIG_VESSEL_OWNER_CAP = VESSEL_OWNER_CAP_DEFAULT;

  shipyard_own_ships("Corr", 2);
  CuAssertIntEquals(tc, 2, vessel_owned_hull_count("Corr"));
  CuAssertIntEquals(tc, 2, vessel_owned_hull_count("corr"));
  CuAssertIntEquals(tc, 0, vessel_owned_hull_count(""));
  CuAssertIntEquals(tc, 0, vessel_owned_hull_count(NULL));
  CuAssertTrue(tc, !vessel_owner_at_cap(&owner.ch));

  shipyard_own_ships("Corr", 3);
  CuAssertTrue(tc, vessel_owner_at_cap(&owner.ch));

  /* A lowered cap leaves existing hulls alone but blocks new ones. */
  CONFIG_VESSEL_OWNER_CAP = 2;
  CuAssertTrue(tc, vessel_owner_at_cap(&owner.ch));
  CONFIG_VESSEL_OWNER_CAP = 4;
  CuAssertTrue(tc, !vessel_owner_at_cap(&owner.ch));

  /* Staff are exempt, and the cap stays inside its configured range. */
  shipyard_own_ships("Zusuk", 4);
  CuAssertTrue(tc, !vessel_owner_at_cap(&staff.ch));
  CONFIG_VESSEL_OWNER_CAP = 0;
  CuAssertIntEquals(tc, VESSEL_OWNER_CAP_MIN, vessel_owner_cap());
  CONFIG_VESSEL_OWNER_CAP = 200;
  CuAssertIntEquals(tc, VESSEL_OWNER_CAP_MAX, vessel_owner_cap());

  CONFIG_VESSEL_OWNER_CAP = (ubyte)saved_cap;
  shipyard_own_ships("", 0);
}

/* Parse one configuration text and report the hull cap it leaves. */
static int shipyard_config_cap(const char *text)
{
  char buffer[64];
  FILE *stream;

  strlcpy(buffer, text, sizeof(buffer));
  stream = fmemopen(buffer, strlen(buffer), "r");
  if (stream == NULL)
  {
    return -1;
  }
  load_config_stream(stream);
  fclose(stream);
  return CONFIG_VESSEL_OWNER_CAP;
}

void Test_vessel_owner_cap_config_is_clamped(CuTest *tc)
{
  pid_t child;
  int status;

  /* load_config_stream() resets every option first, so parse in a child. */
  child = fork();
  CuAssertTrue(tc, child >= 0);
  if (child == 0)
  {
    if (shipyard_config_cap("vessel_owner_cap = 5\n") != 5)
      CuTestChildExit(1);
    if (shipyard_config_cap("vessel_owner_cap = 99\n") != VESSEL_OWNER_CAP_MAX)
      CuTestChildExit(2);
    if (shipyard_config_cap("vessel_owner_cap = 0\n") != VESSEL_OWNER_CAP_MIN)
      CuTestChildExit(3);
    if (shipyard_config_cap("vessel_system = 1\n") != VESSEL_OWNER_CAP_DEFAULT)
      CuTestChildExit(4);
    CuTestChildExit(0);
  }
  CuAssertTrue(tc, waitpid(child, &status, 0) == child);
  CuAssertTrue(tc, WIFEXITED(status));
  CuAssertIntEquals(tc, 0, WEXITSTATUS(status));
}

void Test_vessel_shipyard_sells_only_listed_hulls(CuTest *tc)
{
  const char *enabled = getenv("LUMINARI_TEST_MYSQL_ENABLE");
  char output[8192];
  struct shipyard_player buyer;
  struct descriptor_data descriptor;
  struct room_data dock;
  struct room_data *saved_world;
  struct greyhawk_ship_data ship;
  room_rnum saved_top_of_world;
  MYSQL_RES *result;
  MYSQL *saved_conn;
  MYSQL *connection;
  bool saved_mysql_available;
  int saved_cap;
  bool prepared;

  if (enabled == NULL || strcmp(enabled, "1") != 0)
  {
    return;
  }

  connection = shipyard_open_test_database();
  if (connection == NULL)
  {
    CuFail(tc, "could not connect to the explicitly configured test database");
    return;
  }
  prepared = mysql_query(connection, "CREATE TEMPORARY TABLE ship_prototypes ("
                                     "prototype_id INT AUTO_INCREMENT PRIMARY KEY, "
                                     "name VARCHAR(127) NOT NULL, "
                                     "vessel_class INT NOT NULL DEFAULT 2, "
                                     "max_speed INT NOT NULL DEFAULT 10, "
                                     "armor INT NOT NULL DEFAULT 10, "
                                     "for_sale TINYINT(1) NOT NULL DEFAULT 0, "
                                     "min_level INT NOT NULL DEFAULT 0, "
                                     "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP)") == 0 &&
             mysql_query(connection, "INSERT INTO ship_prototypes "
                                     "(prototype_id, name, vessel_class, max_speed, armor, "
                                     "for_sale, min_level) VALUES "
                                     "(1, 'Listed Sloop', 0, 5, 2, 1, 0), "
                                     "(2, 'Navy Frigate', 3, 20, 40, 0, 0), "
                                     "(3, 'Elite Cutter', 2, 15, 20, 1, 28), "
                                     "(4, 'Wayfarer', 7, 15, 153, 1, 0)") == 0;
  if (!prepared)
  {
    mysql_close(connection);
    CuFail(tc, "could not create the isolated ship prototype fixture");
    return;
  }

  saved_conn = conn;
  saved_mysql_available = mysql_available;
  saved_world = world;
  saved_top_of_world = top_of_world;
  saved_cap = CONFIG_VESSEL_OWNER_CAP;
  conn = connection;
  mysql_available = TRUE;
  CONFIG_VESSEL_OWNER_CAP = VESSEL_OWNER_CAP_DEFAULT;

  memset(&dock, 0, sizeof(dock));
  dock.number = 100;
  SET_BIT_AR(dock.room_flags, ROOM_DOCKABLE);
  world = &dock;
  top_of_world = 0;
  shipyard_player_init(&buyer, "Corr", 20);
  IN_ROOM(&buyer.ch) = 0;
  GET_GOLD(&buyer.ch) = 1000000;
  memset(&descriptor, 0, sizeof(descriptor));
  memset(output, 0, sizeof(output));
  buyer.ch.desc = &descriptor;
  descriptor.character = &buyer.ch;
  descriptor.output = output;
  descriptor.bufspace = sizeof(output) - 1;
  descriptor.pProtocol = ProtocolCreate();
  CuAssertPtrNotNull(tc, descriptor.pProtocol);

  /* The catalog lists only for-sale hulls, with their departure level. */
  do_shipbrowse(&buyer.ch, "", 0, 0);
  CuAssertTrue(tc, strstr(output, "Listed Sloop") != NULL);
  CuAssertTrue(tc, strstr(output, "Elite Cutter") != NULL);
  CuAssertTrue(tc, strstr(output, "Navy Frigate") == NULL);
  CuAssertTrue(tc, strstr(output, " 28 ") != NULL);
  /* The shortest and longest class names line up under the same columns. */
  CuAssertTrue(tc, strstr(output, "1     Raft           5     2     ") != NULL);
  CuAssertTrue(tc, strstr(output, "4     Magical Vessel 15    153   ") != NULL);

  /* An unlisted hull is refused and nothing is charged. */
  memset(output, 0, sizeof(output));
  descriptor.bufptr = 0;
  descriptor.bufspace = sizeof(output) - 1;
  do_shipbuy(&buyer.ch, "2", 0, 0);
  CuAssertTrue(tc, strstr(output, "does not sell that hull") != NULL);
  CuAssertIntEquals(tc, 1000000, GET_GOLD(&buyer.ch));

  /* A captain at the cap cannot buy another hull. */
  shipyard_own_ships("Corr", 3);
  memset(output, 0, sizeof(output));
  descriptor.bufptr = 0;
  descriptor.bufspace = sizeof(output) - 1;
  do_shipbuy(&buyer.ch, "1", 0, 0);
  CuAssertTrue(tc, strstr(output, "You already own 3 hulls") != NULL);
  CuAssertIntEquals(tc, 1000000, GET_GOLD(&buyer.ch));

  /* A trade-in needs the buyer's own hull berthed at this dock with an empty
   * hold, and nothing is charged otherwise. */
  shipyard_own_ships("Corr", 1);
  memset(output, 0, sizeof(output));
  descriptor.bufptr = 0;
  descriptor.bufspace = sizeof(output) - 1;
  do_shipbuy(&buyer.ch, "1 trade", 0, 0);
  CuAssertTrue(tc, strstr(output, "You have no hull berthed here to trade in.") != NULL);
  greyhawk_ships[SHIPYARD_FIRST_SLOT].dock = 100;
  greyhawk_ships[SHIPYARD_FIRST_SLOT].cargo[0].commodity_id = 3;
  greyhawk_ships[SHIPYARD_FIRST_SLOT].cargo[0].quantity = 5;
  strlcpy(greyhawk_ships[SHIPYARD_FIRST_SLOT].name, "the Auk",
          sizeof(greyhawk_ships[SHIPYARD_FIRST_SLOT].name));
  memset(output, 0, sizeof(output));
  descriptor.bufptr = 0;
  descriptor.bufspace = sizeof(output) - 1;
  do_shipbuy(&buyer.ch, "1 trade", 0, 0);
  CuAssertTrue(tc, strstr(output, "Empty the Auk's hold before you trade her in.") != NULL);
  memset(greyhawk_ships[SHIPYARD_FIRST_SLOT].cargo, 0,
         sizeof(greyhawk_ships[SHIPYARD_FIRST_SLOT].cargo));
  autopilot_init(&greyhawk_ships[SHIPYARD_FIRST_SLOT]);
  greyhawk_ships[SHIPYARD_FIRST_SLOT].autopilot->pilot_mob_vnum = 31810;
  memset(output, 0, sizeof(output));
  descriptor.bufptr = 0;
  descriptor.bufspace = sizeof(output) - 1;
  do_shipbuy(&buyer.ch, "1 trade", 0, 0);
  CuAssertTrue(tc, strstr(output, "Unassign the Auk's NPC pilot before you trade her in.") != NULL);
  autopilot_cleanup(&greyhawk_ships[SHIPYARD_FIRST_SLOT]);

  /* As a Listed Sloop the Auk is worth 90% of one; as a wreck her
   * insurance has paid for her, and she is worth nothing. */
  greyhawk_ships[SHIPYARD_FIRST_SLOT].maxspeed = 5;
  vessel_initialize_condition(&greyhawk_ships[SHIPYARD_FIRST_SLOT], 2);
  GET_GOLD(&buyer.ch) = 0;
  memset(output, 0, sizeof(output));
  descriptor.bufptr = 0;
  descriptor.bufspace = sizeof(output) - 1;
  do_shipbuy(&buyer.ch, "1 trade", 0, 0);
  CuAssertTrue(tc, strstr(output, "With 164 gold for the Auk the new hull costs 19 more") != NULL);
  greyhawk_ships[SHIPYARD_FIRST_SLOT].wreck_hull = TRUE;
  memset(output, 0, sizeof(output));
  descriptor.bufptr = 0;
  descriptor.bufspace = sizeof(output) - 1;
  do_shipbuy(&buyer.ch, "1 trade", 0, 0);
  CuAssertTrue(tc, strstr(output, "With 0 gold for the Auk the new hull costs 183 more") != NULL);
  GET_GOLD(&buyer.ch) = 1000000;
  memset(output, 0, sizeof(output));
  descriptor.bufptr = 0;
  descriptor.bufspace = sizeof(output) - 1;
  do_shipbuy(&buyer.ch, "1 barter", 0, 0);
  CuAssertTrue(tc, strstr(output, "Usage: shipbuy <id> [trade]") != NULL);
  CuAssertIntEquals(tc, 1000000, GET_GOLD(&buyer.ch));

  /* A prototype's own level overrides its class minimum. */
  memset(&ship, 0, sizeof(ship));
  ship.vessel_type = VESSEL_SHIP;
  ship.prototype_id = 3;
  CuAssertIntEquals(tc, 28, vessel_ship_min_level(&ship));
  ship.prototype_id = 1;
  CuAssertIntEquals(tc, 16, vessel_ship_min_level(&ship));
  ship.prototype_id = 999;
  CuAssertIntEquals(tc, 16, vessel_ship_min_level(&ship));

  /* A failed read refuses the departure, and the command path runs no DDL
   * that would restore the missing column. */
  CuAssertIntEquals(tc, 0,
                    mysql_query(connection, "ALTER TABLE ship_prototypes DROP COLUMN "
                                            "min_level"));
  ship.prototype_id = 3;
  CuAssertIntEquals(tc, VESSEL_MIN_LEVEL_UNKNOWN, vessel_ship_min_level(&ship));
  CuAssertTrue(tc, vessel_helm_level_refused(&buyer.ch, &ship));
  CuAssertIntEquals(tc, 0,
                    mysql_query(connection, "SHOW COLUMNS FROM ship_prototypes LIKE "
                                            "'min_level'"));
  result = mysql_store_result(connection);
  CuAssertPtrNotNull(tc, result);
  CuAssertIntEquals(tc, 0, (int)mysql_num_rows(result));
  mysql_free_result(result);

  ProtocolDestroy(descriptor.pProtocol);
  shipyard_own_ships("", 0);
  CONFIG_VESSEL_OWNER_CAP = (ubyte)saved_cap;
  world = saved_world;
  top_of_world = saved_top_of_world;
  conn = saved_conn;
  mysql_available = saved_mysql_available;
  mysql_close(connection);
}
