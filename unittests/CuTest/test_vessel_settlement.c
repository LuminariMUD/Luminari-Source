/* Two-phase vessel settlements (vessels-ships study S14): a cargo trade, a
 * freight acceptance and a dock-fee payment at each point where the server
 * can stop between the ship's side in the database and the captain's gold in
 * the player file. The tests lose real connections, so they use the real
 * tables with rows of a test hull, port and players, removed afterwards. */

#include "CuTest.h"

#include "conf.h"
#include "../../src/core/sysdep.h"
#include "../../src/core/structs.h"
#include "../../src/core/utils.h"
#include "../../src/core/comm.h"
#include "../../src/core/db.h"
#include "../../src/database/mysql.h"
#include "../../src/net/protocol.h"
#include "../../src/vessels/vessels.h"

#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

extern struct greyhawk_ship_data greyhawk_ships[GREYHAWK_MAXSHIPS];

/* High fleet slots and player IDs keep these fixtures clear of the suite. */
#define SETTLE_SHIP 486
#define SETTLE_LOST_SHIP 476
#define SETTLE_PORT 100
#define SETTLE_FAR_PORT 101
#define SETTLE_TERN 4249
#define SETTLE_WREN 4250

/* The statement that reads a ship's and a player's open settlements, and the
 * one that deletes a settlement: where a test loses the connection. */
#define SETTLE_READ "SELECT settlement_id, player_id"
#define SETTLE_DELETE "DELETE FROM vessel_settlements WHERE settlement_id"

/* Tern aboard his hull, berthed in a one-room harbor, with a player file and
 * 1,000 gold; 100 salt at the port; and Wren, who has only a player file. */
struct settle_fixture
{
  struct room_data room;
  struct obj_data hull;
  struct char_data captain;
  struct player_special_data specials;
  struct descriptor_data descriptor;
  char output[8192];
  struct greyhawk_ship_data *ship;
  struct room_data *saved_world;
  room_rnum saved_top_of_world;
  struct char_data *saved_character_list;
  char scratch[64];
  char home[PATH_MAX];
  struct player_index_element index[2];
  struct player_index_element *saved_table;
  int saved_top;
  MYSQL *connection;
  MYSQL *saved_conn;
  bool saved_mysql_available;
  int salt_id;
  int base_price;
};

static void settle_query_value(CuTest *tc, MYSQL *connection, const char *query, char *value,
                               size_t value_size)
{
  MYSQL_RES *result;
  MYSQL_ROW row;

  *value = '\0';
  CuAssertIntEquals(tc, 0, mysql_query(connection, query));
  result = mysql_store_result(connection);
  CuAssertPtrNotNull(tc, result);
  row = mysql_fetch_row(result);
  if (row != NULL && row[0] != NULL)
  {
    strlcpy(value, row[0], value_size);
  }
  mysql_free_result(result);
}

static long long settle_query_number(CuTest *tc, MYSQL *connection, const char *query)
{
  char value[64];

  settle_query_value(tc, connection, query, value, sizeof(value));
  return strtoll(value, NULL, 10);
}

/* A connection made as the server's own are. */
static MYSQL *settle_connect(CuTest *tc)
{
  const char *port_text = getenv("LUMINARI_TEST_MYSQL_PORT");
  my_bool reconnect = 1;
  MYSQL *connection;

  connection = mysql_init(NULL);
  CuAssertPtrNotNull(tc, connection);
  mysql_options(connection, MYSQL_OPT_RECONNECT, (const char *)&reconnect);
  if (mysql_real_connect(
          connection, getenv("LUMINARI_TEST_MYSQL_HOST"), getenv("LUMINARI_TEST_MYSQL_USER"),
          getenv("LUMINARI_TEST_MYSQL_PASSWORD"), getenv("LUMINARI_TEST_MYSQL_DATABASE"),
          port_text != NULL ? (unsigned int)strtoul(port_text, NULL, 10) : 3306, NULL, 0) == NULL)
  {
    mysql_close(connection);
    CuFail(tc, "could not connect to the explicitly configured test database");
  }
  return connection;
}

/* The test rows of the real tables. */
static void settle_rows_end(MYSQL *connection)
{
  mysql_query(connection, "DELETE FROM vessel_settlements WHERE ship_id IN (476, 486) "
                          "OR player_id IN (4249, 4250)");
  mysql_query(connection, "DELETE FROM freight_contracts WHERE origin_vnum = 100 "
                          "AND destination_vnum = 101");
  mysql_query(connection, "DELETE FROM ship_cargo_manifest WHERE ship_id IN (476, 486)");
  mysql_query(connection, "DELETE FROM ship_runtime_state WHERE ship_id = 486");
  mysql_query(connection, "DELETE FROM ship_interiors WHERE ship_id IN (476, 486)");
  mysql_query(connection, "DELETE FROM port_commodities WHERE port_vnum = 100");
  mysql_query(connection, "DELETE FROM vessel_bounties WHERE player_name = 'Tern'");
}

/* @return FALSE when the database cases are off */
static bool settle_begin(CuTest *tc, struct settle_fixture *fixture)
{
  const char *enabled = getenv("LUMINARI_TEST_MYSQL_ENABLE");
  struct greyhawk_ship_data *ship;
  char query[256];

  if (enabled == NULL || strcmp(enabled, "1") != 0)
  {
    return FALSE;
  }
  memset(fixture, 0, sizeof(*fixture));
  fixture->connection = settle_connect(tc);
  fixture->saved_conn = conn;
  fixture->saved_mysql_available = mysql_available;
  conn = fixture->connection;
  mysql_available = TRUE;

  /* What boot adds to a database made from master_schema.sql alone. */
  vessel_persistence_ensure_schema();
  vessel_trade_ensure_schema();
  vessel_contracts_ensure_schema();
  vessel_piracy_ensure_schema();
  settle_rows_end(fixture->connection);
  CuAssertIntEquals(
      tc, 0, mysql_query(fixture->connection, "INSERT INTO ship_interiors (ship_id) VALUES (486)"));
  fixture->salt_id = (int)settle_query_number(
      tc, fixture->connection, "SELECT commodity_id FROM trade_commodities WHERE name = 'salt'");
  fixture->base_price = vessel_commodity_base_price(fixture->salt_id);
  snprintf(query, sizeof(query),
           "INSERT INTO port_commodities (port_vnum, commodity_id, supply) VALUES (100, %d, 100)",
           fixture->salt_id);
  CuAssertIntEquals(tc, 0, mysql_query(fixture->connection, query));

  fixture->room.number = SETTLE_PORT;
  SET_BIT_AR(fixture->room.room_flags, ROOM_DOCKABLE);
  fixture->saved_world = world;
  fixture->saved_top_of_world = top_of_world;
  world = &fixture->room;
  top_of_world = 0;

  ship = &greyhawk_ships[SETTLE_SHIP];
  memset(ship, 0, sizeof(*ship));
  ship->active = TRUE;
  ship->shipnum = SETTLE_SHIP;
  ship->vessel_type = VESSEL_SHIP;
  ship->maxspeed = (short int)vessel_class_handling(VESSEL_SHIP)->speed;
  ship->docked_to_ship = -1;
  strlcpy(ship->id, "ST", sizeof(ship->id));
  strlcpy(ship->name, "the Tern", sizeof(ship->name));
  strlcpy(ship->owner, "Tern", sizeof(ship->owner));
  vessel_initialize_condition(ship, vessel_class_condition(VESSEL_SHIP)->beam_armor);
  ship->shipobj = &fixture->hull;
  IN_ROOM(&fixture->hull) = 0;
  ship->dock = SETTLE_PORT;
  fixture->room.ship = ship;
  fixture->ship = ship;

  fixture->captain.player_specials = &fixture->specials;
  fixture->captain.player.name = CuMutableString("Tern");
  fixture->captain.player.level = 20;
  GET_IDNUM(&fixture->captain) = SETTLE_TERN;
  IN_ROOM(&fixture->captain) = 0;
  GET_POS(&fixture->captain) = POS_STANDING;
  GET_GOLD(&fixture->captain) = 1000;
  fixture->captain.desc = &fixture->descriptor;
  fixture->descriptor.character = &fixture->captain;
  fixture->descriptor.output = fixture->output;
  fixture->descriptor.bufspace = sizeof(fixture->output) - 1;
  fixture->descriptor.pProtocol = ProtocolCreate();
  CuAssertPtrNotNull(tc, fixture->descriptor.pProtocol);
  fixture->saved_character_list = character_list;
  fixture->captain.next = character_list;
  character_list = &fixture->captain;

  /* A scratch player directory and index, so the players' saves succeed. */
  strlcpy(fixture->scratch, "/tmp/luminari-settlement-XXXXXX", sizeof(fixture->scratch));
  CuAssertPtrNotNull(tc, getcwd(fixture->home, sizeof(fixture->home)));
  CuAssertPtrNotNull(tc, mkdtemp(fixture->scratch));
  CuAssertIntEquals(tc, 0, chdir(fixture->scratch));
  CuAssertIntEquals(tc, 0, mkdir("plrfiles", 0700));
  CuAssertIntEquals(tc, 0, mkdir("plrfiles/P-T", 0700));
  CuAssertIntEquals(tc, 0, mkdir("plrfiles/U-Z", 0700));
  fixture->index[0].name = GET_NAME(&fixture->captain);
  fixture->index[0].id = SETTLE_TERN;
  fixture->index[1].name = CuMutableString("Wren");
  fixture->index[1].id = SETTLE_WREN;
  fixture->saved_table = player_table;
  fixture->saved_top = top_of_p_table;
  player_table = fixture->index;
  top_of_p_table = 0;
  GET_PFILEPOS(&fixture->captain) = 0;
  CuAssertTrue(tc, save_char_checked(&fixture->captain, 0));
  return TRUE;
}

static void settle_end(CuTest *tc, struct settle_fixture *fixture)
{
  mysql_test_drop_connection_at(NULL, 0, FALSE);
  unlink("plrfiles/P-T/tern.plr");
  unlink("plrfiles/U-Z/wren.plr");
  unlink("plrfiles/index");
  rmdir("plrfiles/P-T");
  rmdir("plrfiles/U-Z");
  rmdir("plrfiles");
  player_table = fixture->saved_table;
  top_of_p_table = fixture->saved_top;
  CuAssertIntEquals(tc, 0, chdir(fixture->home));
  rmdir(fixture->scratch);

  ProtocolDestroy(fixture->descriptor.pProtocol);
  character_list = fixture->saved_character_list;
  world = fixture->saved_world;
  top_of_world = fixture->saved_top_of_world;
  memset(&greyhawk_ships[SETTLE_SHIP], 0, sizeof(greyhawk_ships[0]));
  memset(&greyhawk_ships[SETTLE_LOST_SHIP], 0, sizeof(greyhawk_ships[0]));

  settle_rows_end(fixture->connection);
  conn = fixture->saved_conn;
  mysql_available = fixture->saved_mysql_available;
  mysql_close(fixture->connection);
}

/* Run a command as Tern and return what it printed. */
static const char *settle_command(struct settle_fixture *fixture, ACMD_DECL((*command)),
                                  const char *argument)
{
  memset(fixture->output, 0, sizeof(fixture->output));
  fixture->descriptor.bufptr = 0;
  fixture->descriptor.bufspace = sizeof(fixture->output) - 1;
  command(&fixture->captain, argument, 0, 0);
  return fixture->output;
}

/* Run a command whose gold cannot be saved and whose undo is lost with its
 * connection, so its settlement stays open. */
static const char *settle_command_held(struct settle_fixture *fixture, ACMD_DECL((*command)),
                                       const char *argument)
{
  const char *output;

  GET_PFILEPOS(&fixture->captain) = -1;
  mysql_test_drop_connection_at(SETTLE_DELETE, 1, FALSE);
  output = settle_command(fixture, command, argument);
  GET_PFILEPOS(&fixture->captain) = 0;
  return output;
}

/* Tern enters the game: what the harbor office's login delivery tells him. */
static const char *settle_login(struct settle_fixture *fixture)
{
  memset(fixture->output, 0, sizeof(fixture->output));
  fixture->descriptor.bufptr = 0;
  fixture->descriptor.bufspace = sizeof(fixture->output) - 1;
  vessel_deliver_pending_insurance(&fixture->captain);
  return fixture->output;
}

/* The server stops and boots again: the hull's hold comes from her manifest,
 * and she remembers nothing of a settlement. */
static void settle_reboot(struct settle_fixture *fixture)
{
  memset(fixture->ship->cargo, 0, sizeof(fixture->ship->cargo));
  fixture->ship->settlement_unresolved = 0;
  vessel_db_load_cargo(fixture->ship);
}

/* Put this much salt in her first bay and her manifest, and at the port. */
static void settle_stock(CuTest *tc, struct settle_fixture *fixture, int units, int supply)
{
  char query[128];

  memset(fixture->ship->cargo, 0, sizeof(fixture->ship->cargo));
  if (units > 0)
  {
    fixture->ship->cargo[0].commodity_id = fixture->salt_id;
    fixture->ship->cargo[0].quantity = units;
  }
  CuAssertTrue(tc, vessel_db_save_cargo(fixture->ship));
  snprintf(query, sizeof(query), "UPDATE port_commodities SET supply = %d WHERE port_vnum = 100",
           supply);
  CuAssertIntEquals(tc, 0, mysql_query(fixture->connection, query));
}

/* Tern's gold, the salt in her hold, what the manifest and the port's stock
 * record, and how many settlements are open. */
static void settle_assert(CuTest *tc, struct settle_fixture *fixture, int gold, int units,
                          int manifest, int supply, int open)
{
  int held = 0;
  int i;

  for (i = 0; i < MAX_CARGO_LOTS; i++)
  {
    if (fixture->ship->cargo[i].commodity_id == fixture->salt_id)
    {
      held += fixture->ship->cargo[i].quantity;
    }
  }
  CuAssertIntEquals(tc, gold, GET_GOLD(&fixture->captain));
  CuAssertIntEquals(tc, units, held);
  CuAssertIntEquals(tc, manifest,
                    (int)settle_query_number(tc, fixture->connection,
                                             "SELECT COALESCE(SUM(item_count), 0) "
                                             "FROM ship_cargo_manifest WHERE ship_id = 486"));
  CuAssertIntEquals(
      tc, supply,
      (int)settle_query_number(tc, fixture->connection,
                               "SELECT supply FROM port_commodities WHERE port_vnum = 100"));
  CuAssertIntEquals(tc, open,
                    (int)settle_query_number(tc, fixture->connection,
                                             "SELECT COUNT(*) FROM vessel_settlements "
                                             "WHERE ship_id IN (476, 486) "
                                             "OR player_id IN (4249, 4250)"));
}

/* The settlement a player's file was saved with (its VSet line). */
static unsigned long long settle_file_marker(CuTest *tc, const char *name)
{
  struct char_data *loaded;
  unsigned long long marker;

  loaded = new_char();
  CuAssertTrue(tc, load_char(name, loaded) >= 0);
  marker = GET_VESSEL_SETTLEMENT(loaded);
  free_char(loaded);
  return marker;
}

/* Give Wren a player file saved with this settlement. */
static void settle_wren_file(CuTest *tc, struct settle_fixture *fixture, unsigned long long marker)
{
  struct char_data wren;
  struct player_special_data specials;

  memset(&wren, 0, sizeof(wren));
  memset(&specials, 0, sizeof(specials));
  wren.player_specials = &specials;
  wren.player.name = fixture->index[1].name;
  GET_IDNUM(&wren) = SETTLE_WREN;
  IN_ROOM(&wren) = NOWHERE;
  GET_PFILEPOS(&wren) = 1;
  GET_VESSEL_SETTLEMENT(&wren) = marker;
  top_of_p_table = 1;
  CuAssertTrue(tc, save_char_checked(&wren, 0));
}

/* An open settlement of a purchase of ten salt aboard a hull, as its
 * transaction leaves it. @return its id */
static unsigned long long settle_open_purchase(CuTest *tc, struct settle_fixture *fixture,
                                               int player_id, int ship_id)
{
  char query[256];

  snprintf(query, sizeof(query),
           "INSERT INTO vessel_settlements (player_id, ship_id, port_vnum, commodity_id, "
           "supply_delta, cargo_delta) VALUES (%d, %d, 100, %d, 10, -10)",
           player_id, ship_id, fixture->salt_id);
  CuAssertIntEquals(tc, 0, mysql_query(fixture->connection, query));
  return mysql_insert_id(fixture->connection);
}

/* A trade's row is written with the port's stock and the manifest, the
 * captain's file is saved with the row's id, and the row is deleted. */
void Test_vessel_settlement_pays_a_trade_and_closes_its_row(CuTest *tc)
{
  struct settle_fixture fixture;
  unsigned long long purchase;
  const char *output;
  long long cost;
  long long revenue;
  int gold;

  if (!settle_begin(tc, &fixture))
  {
    return;
  }

  cost = vessel_trade_buy_cost(fixture.base_price, 100, 10);
  gold = 1000 - (int)cost;
  output = settle_command(&fixture, do_cargobuy, "salt 10");
  CuAssertTrue(tc, strstr(output, "You load 10 units of salt") != NULL);
  settle_assert(tc, &fixture, gold, 10, 10, 90, 0);
  purchase = GET_VESSEL_SETTLEMENT(&fixture.captain);
  CuAssertTrue(tc, purchase != 0);
  CuAssertTrue(tc, settle_file_marker(tc, "Tern") == purchase);

  revenue = vessel_trade_sell_revenue(fixture.base_price, 90, 4);
  gold += (int)revenue;
  output = settle_command(&fixture, do_cargosell, "salt 4");
  CuAssertTrue(tc, strstr(output, "You sell 4 units of salt") != NULL);
  settle_assert(tc, &fixture, gold, 6, 6, 94, 0);
  CuAssertTrue(tc, GET_VESSEL_SETTLEMENT(&fixture.captain) > purchase);
  CuAssertTrue(tc, settle_file_marker(tc, "Tern") == GET_VESSEL_SETTLEMENT(&fixture.captain));

  /* A trade that cannot be written moves nothing and leaves no row. */
  mysql_test_drop_connection_at("INSERT INTO ship_cargo_manifest", 1, FALSE);
  output = settle_command(&fixture, do_cargobuy, "salt 10");
  CuAssertTrue(tc, strstr(output, "cannot record that trade; no gold changed hands") != NULL);
  settle_assert(tc, &fixture, gold, 6, 6, 94, 0);
  mysql_test_drop_connection_at("DELETE FROM ship_cargo_manifest", 1, FALSE);
  output = settle_command(&fixture, do_cargosell, "salt all");
  CuAssertTrue(tc, strstr(output, "cannot record that trade; no gold changed hands") != NULL);
  settle_assert(tc, &fixture, gold, 6, 6, 94, 0);

  /* A captain who cannot be saved keeps his gold, and the trade is undone. */
  GET_PFILEPOS(&fixture.captain) = -1;
  output = settle_command(&fixture, do_cargobuy, "salt 10");
  CuAssertTrue(tc,
               strstr(output, "Your gold could not be recorded, so the trade is undone.") != NULL);
  settle_assert(tc, &fixture, gold, 6, 6, 94, 0);
  output = settle_command(&fixture, do_cargosell, "salt all");
  CuAssertTrue(tc,
               strstr(output, "Your gold could not be recorded, so the trade is undone.") != NULL);
  settle_assert(tc, &fixture, gold, 6, 6, 94, 0);
  CuAssertIntEquals(tc, fixture.salt_id, fixture.ship->cargo[0].commodity_id);
  CuAssertTrue(tc, fixture.ship->settlement_unresolved == 0);

  /* A mob has no player file, so its trade is undone the same way. */
  GET_PFILEPOS(&fixture.captain) = 0;
  SET_BIT_AR(MOB_FLAGS(&fixture.captain), MOB_ISNPC);
  output = settle_command(&fixture, do_cargobuy, "salt 10");
  REMOVE_BIT_AR(MOB_FLAGS(&fixture.captain), MOB_ISNPC);
  CuAssertTrue(tc,
               strstr(output, "Your gold could not be recorded, so the trade is undone.") != NULL);
  settle_assert(tc, &fixture, gold, 6, 6, 94, 0);

  settle_end(tc, &fixture);
}

/* The server stops after a trade's ship side is committed and before its
 * gold is saved: the row is open and the file does not name it. At the
 * captain's next login the trade is undone from the row, by its deltas. */
void Test_vessel_settlement_undoes_a_trade_whose_gold_was_never_saved(CuTest *tc)
{
  struct settle_fixture fixture;
  const char *output;

  if (!settle_begin(tc, &fixture))
  {
    return;
  }

  /* The purchase is out of the hold in memory; the database keeps it, with
   * its row, and the captain is not told it is undone. */
  output = settle_command_held(&fixture, do_cargobuy, "salt 10");
  CuAssertTrue(tc, strstr(output, "could not be undone yet") != NULL);
  CuAssertTrue(tc, strstr(output, "so the trade is undone") == NULL);
  settle_assert(tc, &fixture, 1000, 0, 10, 90, 1);
  CuAssertTrue(tc, fixture.ship->settlement_unresolved != 0);

  /* Her manifest is not written from memory while the undo still fails, nor
   * inside another transaction, where no reconcile can run. */
  mysql_test_drop_connection_at(SETTLE_DELETE, 1, FALSE);
  CuAssertTrue(tc, !vessel_db_save_cargo(fixture.ship));
  CuAssertIntEquals(tc, 0, mysql_query(fixture.connection, "START TRANSACTION"));
  CuAssertTrue(tc, !vessel_db_save_cargo(fixture.ship));
  CuAssertIntEquals(tc, 0, mysql_query(fixture.connection, "ROLLBACK"));
  settle_assert(tc, &fixture, 1000, 0, 10, 90, 1);
  CuAssertTrue(tc, fixture.ship->settlement_unresolved != 0);

  /* Booted again, she holds what the manifest lists. Before her captain is
   * back, other ships buy 30 salt at the port and customs take 3 of her 10. */
  settle_reboot(&fixture);
  CuAssertIntEquals(tc, 10, fixture.ship->cargo[0].quantity);
  CuAssertIntEquals(tc, 0,
                    mysql_query(fixture.connection, "UPDATE port_commodities SET supply = 60 "
                                                    "WHERE port_vnum = 100"));
  fixture.ship->cargo[0].quantity = 7;
  CuAssertTrue(tc, vessel_db_save_cargo(fixture.ship));
  output = settle_login(&fixture);
  CuAssertTrue(tc, strstr(output, "never recorded your gold for a cargo trade aboard the Tern, "
                                  "so it has been undone") != NULL);
  settle_assert(tc, &fixture, 1000, 0, 0, 70, 0);
  CuAssertIntEquals(tc, 0, fixture.ship->cargo[0].commodity_id);
  CuAssertTrue(tc, GET_VESSEL_SETTLEMENT(&fixture.captain) == 0);

  /* A sale of the whole lot: the goods come back to a free bay. */
  settle_stock(tc, &fixture, 10, 100);
  output = settle_command_held(&fixture, do_cargosell, "salt all");
  CuAssertTrue(tc, strstr(output, "could not be undone yet") != NULL);
  settle_assert(tc, &fixture, 1000, 10, 0, 110, 1);
  settle_reboot(&fixture);
  settle_assert(tc, &fixture, 1000, 0, 0, 110, 1);
  output = settle_login(&fixture);
  CuAssertTrue(tc, strstr(output, "so it has been undone") != NULL);
  settle_assert(tc, &fixture, 1000, 10, 10, 100, 0);

  /* A second login finds nothing to settle. */
  output = settle_login(&fixture);
  CuAssertStrEquals(tc, "", output);
  settle_assert(tc, &fixture, 1000, 10, 10, 100, 0);

  /* Goods he never paid for cannot be sold: the sale's gate takes them out
   * of the hold before the sale looks for them. */
  settle_stock(tc, &fixture, 0, 100);
  settle_command_held(&fixture, do_cargobuy, "salt 10");
  settle_reboot(&fixture);
  settle_assert(tc, &fixture, 1000, 10, 10, 90, 1);
  output = settle_command(&fixture, do_cargosell, "salt all");
  CuAssertTrue(tc, strstr(output, "so it has been undone") != NULL);
  CuAssertTrue(tc, strstr(output, "You carry no salt.") != NULL);
  settle_assert(tc, &fixture, 1000, 0, 0, 100, 0);

  settle_end(tc, &fixture);
}

/* While a settlement is open, no account is opened for the ship or the
 * player: all six commands wait for the reconcile, which undoes it. */
void Test_vessel_settlement_holds_the_ship_until_it_is_settled(CuTest *tc)
{
  static const struct
  {
    ACMD_DECL((*command));
    const char *argument;
  } accounts[] = {{do_cargobuy, "salt 5"},   {do_cargosell, "salt 1"},  {do_contractaccept, "1"},
                  {do_contractdeliver, "1"}, {do_contractabandon, "1"}, {do_dockfees, "pay"}};
  struct settle_fixture fixture;
  const char *output;
  long long cost;
  size_t i;

  if (!settle_begin(tc, &fixture))
  {
    return;
  }
  fixture.ship->dock_fee_balance = 25;
  fixture.ship->dock_fee_port = SETTLE_PORT;

  settle_command_held(&fixture, do_cargobuy, "salt 10");
  settle_assert(tc, &fixture, 1000, 0, 10, 90, 1);
  for (i = 0; i < sizeof(accounts) / sizeof(accounts[0]); i++)
  {
    mysql_test_drop_connection_at(SETTLE_READ, 1, FALSE);
    output = settle_command(&fixture, accounts[i].command, accounts[i].argument);
    CuAssertTrue(tc, strstr(output, "still settling an earlier account") != NULL);
    settle_assert(tc, &fixture, 1000, 0, 10, 90, 1);
    CuAssertIntEquals(tc, 25, fixture.ship->dock_fee_balance);
  }

  /* Inspecting the dock fee opens no account and does not wait. */
  mysql_test_drop_connection_at(SETTLE_READ, 1, FALSE);
  output = settle_command(&fixture, do_dockfees, "");
  CuAssertTrue(tc, strstr(output, "owes 25 gold") != NULL);
  mysql_test_drop_connection_at(NULL, 0, FALSE);

  /* The next account settles it first. Her hold in memory is already
   * without the purchase, so only the database is undone. */
  cost = vessel_trade_buy_cost(fixture.base_price, 100, 5);
  output = settle_command(&fixture, do_cargobuy, "salt 5");
  CuAssertTrue(tc, strstr(output, "so it has been undone") != NULL);
  CuAssertTrue(tc, strstr(output, "You load 5 units of salt") != NULL);
  settle_assert(tc, &fixture, 1000 - (int)cost, 5, 5, 95, 0);
  CuAssertTrue(tc, fixture.ship->settlement_unresolved == 0);

  /* With no database nothing can be settled, and a hull that remembers a
   * settlement stays held. */
  mysql_available = FALSE;
  CuAssertTrue(tc, vessel_settlements_reconcile(&fixture.captain, fixture.ship));
  fixture.ship->settlement_unresolved = 7;
  CuAssertTrue(tc, !vessel_settlements_reconcile(&fixture.captain, fixture.ship));
  fixture.ship->settlement_unresolved = 0;
  mysql_available = TRUE;

  settle_end(tc, &fixture);
}

/* The server stops after the gold is saved and before the row is deleted:
 * the file names the row, so it is deleted and nothing is undone. */
void Test_vessel_settlement_closes_a_paid_row_left_behind(CuTest *tc)
{
  struct settle_fixture fixture;
  const char *output;
  long long cost;
  long long revenue;
  int gold;

  if (!settle_begin(tc, &fixture))
  {
    return;
  }

  cost = vessel_trade_buy_cost(fixture.base_price, 100, 10);
  gold = 1000 - (int)cost;
  mysql_test_drop_connection_at(SETTLE_DELETE, 1, FALSE);
  output = settle_command(&fixture, do_cargobuy, "salt 10");
  CuAssertTrue(tc, strstr(output, "You load 10 units of salt") != NULL);
  settle_assert(tc, &fixture, gold, 10, 10, 90, 1);
  CuAssertTrue(tc, settle_file_marker(tc, "Tern") ==
                       (unsigned long long)settle_query_number(
                           tc, fixture.connection,
                           "SELECT settlement_id FROM vessel_settlements WHERE ship_id = 486"));
  settle_reboot(&fixture);
  output = settle_login(&fixture);
  CuAssertStrEquals(tc, "", output);
  settle_assert(tc, &fixture, gold, 10, 10, 90, 0);

  /* The next account closes one as well. */
  revenue = vessel_trade_sell_revenue(fixture.base_price, 90, 4);
  gold += (int)revenue;
  mysql_test_drop_connection_at(SETTLE_DELETE, 1, FALSE);
  output = settle_command(&fixture, do_cargosell, "salt 4");
  CuAssertTrue(tc, strstr(output, "You sell 4 units of salt") != NULL);
  settle_assert(tc, &fixture, gold, 6, 6, 94, 1);
  cost = vessel_trade_buy_cost(fixture.base_price, 94, 1);
  gold -= (int)cost;
  output = settle_command(&fixture, do_cargobuy, "salt 1");
  CuAssertTrue(tc, strstr(output, "undone") == NULL);
  CuAssertTrue(tc, strstr(output, "You load 1 units of salt") != NULL);
  settle_assert(tc, &fixture, gold, 7, 7, 93, 0);

  settle_end(tc, &fixture);
}

/* A COMMIT that gets no reply may have taken effect: the settlement's row
 * tells. If the row cannot be read either, the trade is taken out of memory
 * and the hull remembers it until a reconcile can read the database. */
void Test_vessel_settlement_settles_a_commit_without_a_reply(CuTest *tc)
{
  static char no_socket[] = "/nonexistent/s14.sock";
  struct settle_fixture fixture;
  const char *output;
  char *unix_socket;
  unsigned int tcp_port;
  unsigned long session;
  long long cost;
  long long revenue;
  int gold;

  if (!settle_begin(tc, &fixture))
  {
    return;
  }
  tcp_port = fixture.connection->port;
  unix_socket = fixture.connection->unix_socket;

  /* The purchase was committed, though its reply was lost. */
  session = mysql_thread_id(fixture.connection);
  cost = vessel_trade_buy_cost(fixture.base_price, 100, 10);
  gold = 1000 - (int)cost;
  mysql_test_drop_connection_at("COMMIT", 1, TRUE);
  output = settle_command(&fixture, do_cargobuy, "salt 10");
  CuAssertTrue(tc, strstr(output, "You load 10 units of salt") != NULL);
  settle_assert(tc, &fixture, gold, 10, 10, 90, 0);
  CuAssertTrue(tc, session != mysql_thread_id(fixture.connection));

  /* The sale's COMMIT never arrived, so the server rolled it back. */
  mysql_test_drop_connection_at("COMMIT", 1, FALSE);
  output = settle_command(&fixture, do_cargosell, "salt 4");
  CuAssertTrue(tc, strstr(output, "cannot record that trade; no gold changed hands") != NULL);
  settle_assert(tc, &fixture, gold, 10, 10, 90, 0);
  CuAssertTrue(tc, fixture.ship->settlement_unresolved == 0);

  /* The sale was committed, its reply was lost, and the database is away
   * for the read-back: no gold moves, the goods stay in her hold, and she
   * remembers the settlement. Her manifest is not written meanwhile. */
  mysql_test_drop_connection_at("COMMIT", 1, TRUE);
  fixture.connection->port = 1;
  fixture.connection->unix_socket = no_socket;
  output = settle_command(&fixture, do_cargosell, "salt 4");
  CuAssertTrue(tc, strstr(output, "cannot record that trade; no gold changed hands") != NULL);
  CuAssertTrue(tc, fixture.ship->settlement_unresolved != 0);
  CuAssertTrue(tc, !vessel_db_save_cargo(fixture.ship));
  fixture.connection->port = tcp_port;
  fixture.connection->unix_socket = unix_socket;
  settle_assert(tc, &fixture, gold, 10, 6, 94, 1);

  /* The next account undoes it in the database and is then made. */
  revenue = vessel_trade_sell_revenue(fixture.base_price, 90, 4);
  gold += (int)revenue;
  output = settle_command(&fixture, do_cargosell, "salt 4");
  CuAssertTrue(tc, strstr(output, "so it has been undone") != NULL);
  CuAssertTrue(tc, strstr(output, "You sell 4 units of salt") != NULL);
  settle_assert(tc, &fixture, gold, 6, 6, 94, 0);

  /* The purchase's COMMIT never arrived and the database is away: she
   * remembers a settlement that was never recorded, and the next reconcile
   * forgets it. */
  mysql_test_drop_connection_at("COMMIT", 1, FALSE);
  fixture.connection->port = 1;
  fixture.connection->unix_socket = no_socket;
  output = settle_command(&fixture, do_cargobuy, "salt 2");
  CuAssertTrue(tc, strstr(output, "cannot record that trade; no gold changed hands") != NULL);
  fixture.connection->port = tcp_port;
  fixture.connection->unix_socket = unix_socket;
  CuAssertTrue(tc, fixture.ship->settlement_unresolved != 0);
  settle_assert(tc, &fixture, gold, 6, 6, 94, 0);
  CuAssertTrue(tc, vessel_db_save_cargo(fixture.ship));
  CuAssertTrue(tc, fixture.ship->settlement_unresolved == 0);
  settle_assert(tc, &fixture, gold, 6, 6, 94, 0);

  /* The captain cannot be saved, and the undo's reply is lost after it was
   * committed: the purchase is undone. */
  GET_PFILEPOS(&fixture.captain) = -1;
  mysql_test_drop_connection_at("COMMIT", 2, TRUE);
  output = settle_command(&fixture, do_cargobuy, "salt 10");
  CuAssertTrue(tc,
               strstr(output, "Your gold could not be recorded, so the trade is undone.") != NULL);
  settle_assert(tc, &fixture, gold, 6, 6, 94, 0);
  CuAssertTrue(tc, fixture.ship->settlement_unresolved == 0);

  /* When the undo's COMMIT never arrived, the sale is still recorded: it
   * is out of memory, and the login's reconcile undoes it in the database. */
  mysql_test_drop_connection_at("COMMIT", 2, FALSE);
  output = settle_command(&fixture, do_cargosell, "salt all");
  CuAssertTrue(tc, strstr(output, "could not be undone yet") != NULL);
  GET_PFILEPOS(&fixture.captain) = 0;
  settle_assert(tc, &fixture, gold, 6, 0, 100, 1);
  output = settle_login(&fixture);
  CuAssertTrue(tc, strstr(output, "so it has been undone") != NULL);
  settle_assert(tc, &fixture, gold, 6, 6, 94, 0);

  settle_end(tc, &fixture);
}

/* A COMMIT that got no reply and could not be read back may still be on its
 * way. Until it lands or is rolled back the hull keeps remembering it: a
 * reconcile that sees no row yet does not take it for never recorded. */
void Test_vessel_settlement_waits_for_a_commit_still_on_its_way(CuTest *tc)
{
  struct settle_fixture fixture;
  unsigned long long settlement;
  const char *output;
  MYSQL *pending;
  long long revenue;
  char query[256];

  if (!settle_begin(tc, &fixture))
  {
    return;
  }
  pending = settle_connect(tc);
  CuAssertIntEquals(tc, 0,
                    mysql_query(fixture.connection, "SET SESSION innodb_lock_wait_timeout = 1"));

  /* Tern's sale of 4 of his 10 salt: its transaction is still open on the
   * session he lost, the goods are back in his hold, and she remembers it. */
  settle_stock(tc, &fixture, 10, 100);
  CuAssertIntEquals(tc, 0, mysql_query(pending, "START TRANSACTION"));
  snprintf(query, sizeof(query),
           "INSERT INTO vessel_settlements (player_id, ship_id, port_vnum, commodity_id, "
           "supply_delta, cargo_delta) VALUES (4249, 486, 100, %d, -4, 4)",
           fixture.salt_id);
  CuAssertIntEquals(tc, 0, mysql_query(pending, query));
  settlement = mysql_insert_id(pending);
  CuAssertIntEquals(tc, 0,
                    mysql_query(pending, "UPDATE port_commodities SET supply = 104 "
                                         "WHERE port_vnum = 100"));
  CuAssertIntEquals(tc, 0,
                    mysql_query(pending, "UPDATE ship_cargo_manifest SET item_count = 6 "
                                         "WHERE ship_id = 486"));
  fixture.ship->settlement_unresolved = settlement;

  /* No row can be read yet, and the locking read cannot get past the open
   * transaction: she is not settled, and her manifest is not written. */
  CuAssertTrue(tc, !vessel_db_save_cargo(fixture.ship));
  CuAssertTrue(tc, fixture.ship->settlement_unresolved == settlement);
  output = settle_command(&fixture, do_cargosell, "salt 4");
  CuAssertTrue(tc, strstr(output, "still settling an earlier account") != NULL);
  settle_assert(tc, &fixture, 1000, 10, 10, 100, 0);

  /* The COMMIT lands. The next account undoes the sale in the database
   * only, and is then made on the ten units he holds. */
  CuAssertIntEquals(tc, 0, mysql_query(pending, "COMMIT"));
  settle_assert(tc, &fixture, 1000, 10, 6, 104, 1);
  revenue = vessel_trade_sell_revenue(fixture.base_price, 100, 4);
  output = settle_command(&fixture, do_cargosell, "salt 4");
  CuAssertTrue(tc, strstr(output, "so it has been undone") != NULL);
  CuAssertTrue(tc, strstr(output, "You sell 4 units of salt") != NULL);
  settle_assert(tc, &fixture, 1000 + (int)revenue, 6, 6, 104, 0);
  CuAssertTrue(tc, fixture.ship->settlement_unresolved == 0);

  mysql_close(pending);
  settle_end(tc, &fixture);
}

/* A settlement another captain left on the ship is judged from that
 * player's file, so the ship does not wait for him. */
void Test_vessel_settlement_judges_another_captain_from_his_file(CuTest *tc)
{
  struct settle_fixture fixture;
  unsigned long long settlement;
  const char *output;
  long long cost;
  int gold;

  if (!settle_begin(tc, &fixture))
  {
    return;
  }

  /* Wren bought ten salt aboard, and his file was saved with it: the row
   * is deleted and his purchase stands. */
  settle_stock(tc, &fixture, 10, 90);
  settlement = settle_open_purchase(tc, &fixture, SETTLE_WREN, SETTLE_SHIP);
  settle_wren_file(tc, &fixture, settlement);
  CuAssertTrue(tc, settle_file_marker(tc, "Wren") == settlement);
  cost = vessel_trade_buy_cost(fixture.base_price, 90, 1);
  gold = 1000 - (int)cost;
  output = settle_command(&fixture, do_cargobuy, "salt 1");
  CuAssertTrue(tc, strstr(output, "undone") == NULL);
  CuAssertTrue(tc, strstr(output, "You load 1 units of salt") != NULL);
  settle_assert(tc, &fixture, gold, 11, 11, 89, 0);

  /* His file names an older settlement: this one is undone, and Tern is
   * not told of gold that was never his. */
  settle_open_purchase(tc, &fixture, SETTLE_WREN, SETTLE_SHIP);
  cost = vessel_trade_buy_cost(fixture.base_price, 99, 1);
  gold -= (int)cost;
  output = settle_command(&fixture, do_cargobuy, "salt 1");
  CuAssertTrue(tc, strstr(output, "undone") == NULL);
  CuAssertTrue(tc, strstr(output, "You load 1 units of salt") != NULL);
  settle_assert(tc, &fixture, gold, 2, 2, 98, 0);

  /* His file cannot be read: the ship waits. */
  settle_open_purchase(tc, &fixture, SETTLE_WREN, SETTLE_SHIP);
  CuAssertIntEquals(tc, 0, unlink("plrfiles/U-Z/wren.plr"));
  output = settle_command(&fixture, do_cargobuy, "salt 1");
  CuAssertTrue(tc, strstr(output, "still settling an earlier account") != NULL);
  settle_assert(tc, &fixture, gold, 2, 2, 98, 1);

  /* He was removed from the game: no file holds the gold, so it is undone. */
  top_of_p_table = 0;
  cost = vessel_trade_buy_cost(fixture.base_price, 108, 1);
  gold -= (int)cost;
  output = settle_command(&fixture, do_cargobuy, "salt 1");
  CuAssertTrue(tc, strstr(output, "You load 1 units of salt") != NULL);
  settle_assert(tc, &fixture, gold, 1, 1, 107, 0);

  settle_end(tc, &fixture);
}

/* A freight acceptance settles the same way: undone, the job is back on
 * the board and the freight out of the hold. */
void Test_vessel_settlement_covers_a_freight_acceptance(CuTest *tc)
{
  struct settle_fixture fixture;
  const char *output;
  char accept[32];
  char status[128];
  char query[256];
  int contract;

  if (!settle_begin(tc, &fixture))
  {
    return;
  }
  snprintf(query, sizeof(query),
           "INSERT INTO freight_contracts (origin_vnum, destination_vnum, commodity_id, quantity, "
           "payout, status) VALUES (100, 101, %d, 10, 300, 0)",
           fixture.salt_id);
  CuAssertIntEquals(tc, 0, mysql_query(fixture.connection, query));
  contract = (int)mysql_insert_id(fixture.connection);
  snprintf(accept, sizeof(accept), "%d", contract);
  snprintf(status, sizeof(status),
           "SELECT CONCAT(status, ':', taken_by) FROM freight_contracts WHERE contract_id = %d",
           contract);

  /* The captain cannot be saved: the bond is not taken, the job is open
   * again and the freight is unloaded. */
  GET_PFILEPOS(&fixture.captain) = -1;
  output = settle_command(&fixture, do_contractaccept, accept);
  CuAssertTrue(tc, strstr(output, "Your gold could not be recorded, so the contract is undone.") !=
                       NULL);
  GET_PFILEPOS(&fixture.captain) = 0;
  settle_assert(tc, &fixture, 1000, 0, 0, 100, 0);
  settle_query_value(tc, fixture.connection, status, query, sizeof(query));
  CuAssertStrEquals(tc, "0:", query);

  /* The server stops between the job and the bond: at his next login the
   * job is reopened and the freight unloaded. */
  output = settle_command_held(&fixture, do_contractaccept, accept);
  CuAssertTrue(tc, strstr(output, "could not be undone yet") != NULL);
  settle_assert(tc, &fixture, 1000, 0, 10, 100, 1);
  settle_query_value(tc, fixture.connection, status, query, sizeof(query));
  CuAssertStrEquals(tc, "1:Tern", query);
  settle_reboot(&fixture);
  output = settle_login(&fixture);
  CuAssertTrue(tc, strstr(output, "never recorded your gold for a freight contract aboard "
                                  "the Tern, so it has been undone") != NULL);
  settle_assert(tc, &fixture, 1000, 0, 0, 100, 0);
  settle_query_value(tc, fixture.connection, status, query, sizeof(query));
  CuAssertStrEquals(tc, "0:", query);

  /* Saved, the bond is posted and the row closed. */
  output = settle_command(&fixture, do_contractaccept, accept);
  CuAssertTrue(tc, strstr(output, "you post a 140-gold bond, 10 units are loaded") != NULL);
  settle_assert(tc, &fixture, 860, 10, 10, 100, 0);
  settle_query_value(tc, fixture.connection, status, query, sizeof(query));
  CuAssertStrEquals(tc, "1:Tern", query);
  CuAssertTrue(tc, settle_file_marker(tc, "Tern") == GET_VESSEL_SETTLEMENT(&fixture.captain));

  settle_end(tc, &fixture);
}

/* A dock-fee payment settles the same way: undone, the fee is owed again,
 * unless the hull already owes one. */
void Test_vessel_settlement_covers_a_dock_fee_payment(CuTest *tc)
{
  static const char *recorded_fee =
      "SELECT CONCAT(dock_fee_balance, ':', dock_fee_port) FROM ship_runtime_state "
      "WHERE ship_id = 486";
  struct settle_fixture fixture;
  struct greyhawk_ship_data *ship;
  const char *output;
  char value[64];

  if (!settle_begin(tc, &fixture))
  {
    return;
  }
  ship = fixture.ship;
  ship->dock_fee_balance = 25;
  ship->dock_fee_port = SETTLE_PORT;
  CuAssertTrue(tc, vessel_db_save_runtime(ship));

  /* The captain cannot be saved: the fee is owed again. */
  GET_PFILEPOS(&fixture.captain) = -1;
  output = settle_command(&fixture, do_dockfees, "pay");
  CuAssertTrue(tc, strstr(output, "Your gold could not be recorded, so the payment is undone.") !=
                       NULL);
  GET_PFILEPOS(&fixture.captain) = 0;
  settle_assert(tc, &fixture, 1000, 0, 0, 100, 0);
  CuAssertIntEquals(tc, 25, ship->dock_fee_balance);
  settle_query_value(tc, fixture.connection, recorded_fee, value, sizeof(value));
  CuAssertStrEquals(tc, "25:100", value);

  /* The server stops between the cleared fee and the gold. She boots owing
   * nothing and sails; at his next login the fee is owed again. */
  output = settle_command_held(&fixture, do_dockfees, "pay");
  CuAssertTrue(tc, strstr(output, "could not be undone yet") != NULL);
  settle_assert(tc, &fixture, 1000, 0, 0, 100, 1);
  settle_query_value(tc, fixture.connection, recorded_fee, value, sizeof(value));
  CuAssertStrEquals(tc, "0:100", value);
  settle_reboot(&fixture);
  ship->dock_fee_balance = 0;
  ship->dock_fee_port = 0;
  output = settle_login(&fixture);
  CuAssertTrue(tc, strstr(output, "never recorded your gold for a dock-fee payment aboard "
                                  "the Tern, so it has been undone") != NULL);
  settle_assert(tc, &fixture, 1000, 0, 0, 100, 0);
  CuAssertIntEquals(tc, 25, ship->dock_fee_balance);
  CuAssertIntEquals(tc, SETTLE_PORT, ship->dock_fee_port);
  settle_query_value(tc, fixture.connection, recorded_fee, value, sizeof(value));
  CuAssertStrEquals(tc, "25:100", value);

  /* Before he is back she berths elsewhere and owes that port: she keeps
   * that fee, and the unpaid one is not added to it. */
  settle_command_held(&fixture, do_dockfees, "pay");
  settle_reboot(&fixture);
  ship->dock_fee_balance = 40;
  ship->dock_fee_port = SETTLE_FAR_PORT;
  settle_login(&fixture);
  settle_assert(tc, &fixture, 1000, 0, 0, 100, 0);
  CuAssertIntEquals(tc, 40, ship->dock_fee_balance);
  CuAssertIntEquals(tc, SETTLE_FAR_PORT, ship->dock_fee_port);

  /* A payment begins by settling the last one: the fee it finds unpaid is
   * owed again, and is then paid, saved, and its row closed. */
  ship->dock_fee_balance = 25;
  ship->dock_fee_port = SETTLE_PORT;
  settle_command_held(&fixture, do_dockfees, "pay");
  settle_reboot(&fixture);
  ship->dock_fee_balance = 0;
  output = settle_command(&fixture, do_dockfees, "pay");
  CuAssertTrue(tc, strstr(output, "so it has been undone") != NULL);
  CuAssertTrue(tc, strstr(output, "You settle 25 gold in dock fees.") != NULL);
  settle_assert(tc, &fixture, 975, 0, 0, 100, 0);
  CuAssertIntEquals(tc, 0, ship->dock_fee_balance);
  settle_query_value(tc, fixture.connection, recorded_fee, value, sizeof(value));
  CuAssertStrEquals(tc, "0:100", value);
  CuAssertTrue(tc, settle_file_marker(tc, "Tern") == GET_VESSEL_SETTLEMENT(&fixture.captain));

  settle_end(tc, &fixture);
}

/* A hull that is not in memory, a hold with no bay for returned goods, and
 * a purged hull's settlement. */
void Test_vessel_settlement_waits_for_a_hull_not_in_memory(CuTest *tc)
{
  struct greyhawk_ship_data *unloaded = &greyhawk_ships[SETTLE_LOST_SHIP];
  struct settle_fixture fixture;
  const char *output;
  char query[256];
  int i;

  if (!settle_begin(tc, &fixture))
  {
    return;
  }

  /* Boot could not rebuild the hull Tern bought ten salt aboard, and left
   * her rows. Her settlement is not undone without her hold, and his
   * accounts wait. */
  settle_stock(tc, &fixture, 0, 90);
  settle_open_purchase(tc, &fixture, SETTLE_TERN, SETTLE_LOST_SHIP);
  output = settle_login(&fixture);
  CuAssertStrEquals(tc, "", output);
  settle_assert(tc, &fixture, 1000, 0, 0, 90, 1);
  output = settle_command(&fixture, do_cargobuy, "salt 1");
  CuAssertTrue(tc, strstr(output, "still settling an earlier account") != NULL);
  settle_assert(tc, &fixture, 1000, 0, 0, 90, 1);

  /* A later boot loads her with the ten units, and his login undoes them. */
  unloaded->active = TRUE;
  unloaded->shipnum = SETTLE_LOST_SHIP;
  strlcpy(unloaded->name, "the Wren", sizeof(unloaded->name));
  unloaded->cargo[0].commodity_id = fixture.salt_id;
  unloaded->cargo[0].quantity = 10;
  output = settle_login(&fixture);
  CuAssertTrue(tc, strstr(output, "a cargo trade aboard the Wren, so it has been undone") != NULL);
  CuAssertIntEquals(tc, 0, unloaded->cargo[0].quantity);
  settle_assert(tc, &fixture, 1000, 0, 0, 100, 0);
  memset(unloaded, 0, sizeof(*unloaded));

  /* Every bay holds other goods: the salt of an unpaid sale stays ashore,
   * and the rest of the undo is made. */
  for (i = 0; i < MAX_CARGO_LOTS; i++)
  {
    fixture.ship->cargo[i].commodity_id = 9000 + i;
    fixture.ship->cargo[i].quantity = 1;
  }
  snprintf(query, sizeof(query),
           "INSERT INTO vessel_settlements (player_id, ship_id, port_vnum, commodity_id, "
           "supply_delta, cargo_delta) VALUES (4249, 486, 100, %d, -10, 10)",
           fixture.salt_id);
  CuAssertIntEquals(tc, 0, mysql_query(fixture.connection, query));
  output = settle_login(&fixture);
  CuAssertTrue(tc, strstr(output, "so it has been undone") != NULL);
  settle_assert(tc, &fixture, 1000, 0, MAX_CARGO_LOTS, 90, 0);
  CuAssertIntEquals(tc, 9000, fixture.ship->cargo[0].commodity_id);

  /* A purged hull's settlement goes with her other rows. */
  settle_open_purchase(tc, &fixture, SETTLE_WREN, SETTLE_SHIP);
  CuAssertTrue(tc, vessel_delete_persistence(SETTLE_SHIP));
  settle_assert(tc, &fixture, 1000, 0, 0, 90, 0);

  settle_end(tc, &fixture);
}

/* A dock-fee payment and a freight acceptance whose COMMIT gets no reply
 * settle as a trade does: by the row when it can be read, and by the hull's
 * memory of it when the database is away for the read-back. */
void Test_vessel_settlement_settles_a_fee_and_a_bond_without_a_reply(CuTest *tc)
{
  static const char *recorded_fee =
      "SELECT CONCAT(dock_fee_balance, ':', dock_fee_port) FROM ship_runtime_state "
      "WHERE ship_id = 486";
  static char no_socket[] = "/nonexistent/s14.sock";
  struct settle_fixture fixture;
  struct greyhawk_ship_data *ship;
  const char *output;
  char *unix_socket;
  unsigned int tcp_port;
  char accept[32];
  char status[128];
  char query[256];
  char value[64];
  int contract;

  if (!settle_begin(tc, &fixture))
  {
    return;
  }
  tcp_port = fixture.connection->port;
  unix_socket = fixture.connection->unix_socket;
  ship = fixture.ship;
  ship->dock_fee_balance = 25;
  ship->dock_fee_port = SETTLE_PORT;
  CuAssertTrue(tc, vessel_db_save_runtime(ship));

  /* The payment was committed, though its reply was lost: it is paid once. */
  mysql_test_drop_connection_at("COMMIT", 1, TRUE);
  output = settle_command(&fixture, do_dockfees, "pay");
  CuAssertTrue(tc, strstr(output, "You settle 25 gold in dock fees.") != NULL);
  settle_assert(tc, &fixture, 975, 0, 0, 100, 0);
  CuAssertIntEquals(tc, 0, ship->dock_fee_balance);
  settle_query_value(tc, fixture.connection, recorded_fee, value, sizeof(value));
  CuAssertStrEquals(tc, "0:100", value);

  /* Its COMMIT never arrived: no gold is taken and the fee is still owed. */
  ship->dock_fee_balance = 25;
  CuAssertTrue(tc, vessel_db_save_runtime(ship));
  mysql_test_drop_connection_at("COMMIT", 1, FALSE);
  output = settle_command(&fixture, do_dockfees, "pay");
  CuAssertTrue(tc, strstr(output, "The harbor ledger is unavailable; no gold was taken.") != NULL);
  settle_assert(tc, &fixture, 975, 0, 0, 100, 0);
  CuAssertIntEquals(tc, 25, ship->dock_fee_balance);
  CuAssertTrue(tc, ship->settlement_unresolved == 0);
  settle_query_value(tc, fixture.connection, recorded_fee, value, sizeof(value));
  CuAssertStrEquals(tc, "25:100", value);

  /* Committed, its reply lost, and the database away for the read-back: no
   * gold is taken, the fee is owed in memory, and she remembers the
   * settlement, which the database holds with the cleared fee. */
  mysql_test_drop_connection_at("COMMIT", 1, TRUE);
  fixture.connection->port = 1;
  fixture.connection->unix_socket = no_socket;
  output = settle_command(&fixture, do_dockfees, "pay");
  CuAssertTrue(tc, strstr(output, "The harbor ledger is unavailable; no gold was taken.") != NULL);
  fixture.connection->port = tcp_port;
  fixture.connection->unix_socket = unix_socket;
  CuAssertTrue(tc, ship->settlement_unresolved != 0);
  CuAssertIntEquals(tc, 25, ship->dock_fee_balance);
  settle_assert(tc, &fixture, 975, 0, 0, 100, 1);
  settle_query_value(tc, fixture.connection, recorded_fee, value, sizeof(value));
  CuAssertStrEquals(tc, "0:100", value);

  /* The next payment undoes it and then pays the fee, once. */
  output = settle_command(&fixture, do_dockfees, "pay");
  CuAssertTrue(tc, strstr(output, "so it has been undone") != NULL);
  CuAssertTrue(tc, strstr(output, "You settle 25 gold in dock fees.") != NULL);
  settle_assert(tc, &fixture, 950, 0, 0, 100, 0);
  CuAssertIntEquals(tc, 0, ship->dock_fee_balance);
  CuAssertTrue(tc, ship->settlement_unresolved == 0);
  settle_query_value(tc, fixture.connection, recorded_fee, value, sizeof(value));
  CuAssertStrEquals(tc, "0:100", value);
  output = settle_command(&fixture, do_dockfees, "pay");
  CuAssertTrue(tc, strstr(output, "has no outstanding dock fees") != NULL);
  settle_assert(tc, &fixture, 950, 0, 0, 100, 0);

  /* A freight acceptance the same way: the freight is out of her hold, the
   * database holds the taken job and its row, and no bond is taken. */
  snprintf(query, sizeof(query),
           "INSERT INTO freight_contracts (origin_vnum, destination_vnum, commodity_id, quantity, "
           "payout, status) VALUES (100, 101, %d, 10, 300, 0)",
           fixture.salt_id);
  CuAssertIntEquals(tc, 0, mysql_query(fixture.connection, query));
  contract = (int)mysql_insert_id(fixture.connection);
  snprintf(accept, sizeof(accept), "%d", contract);
  snprintf(status, sizeof(status),
           "SELECT CONCAT(status, ':', taken_by) FROM freight_contracts WHERE contract_id = %d",
           contract);
  mysql_test_drop_connection_at("COMMIT", 1, TRUE);
  fixture.connection->port = 1;
  fixture.connection->unix_socket = no_socket;
  output = settle_command(&fixture, do_contractaccept, accept);
  CuAssertTrue(tc, strstr(output, "cannot record that freight; no gold was taken") != NULL);
  fixture.connection->port = tcp_port;
  fixture.connection->unix_socket = unix_socket;
  CuAssertTrue(tc, ship->settlement_unresolved != 0);
  settle_assert(tc, &fixture, 950, 0, 10, 100, 1);
  settle_query_value(tc, fixture.connection, status, value, sizeof(value));
  CuAssertStrEquals(tc, "1:Tern", value);

  /* The next acceptance undoes it and takes the job, with one bond. */
  output = settle_command(&fixture, do_contractaccept, accept);
  CuAssertTrue(tc, strstr(output, "so it has been undone") != NULL);
  CuAssertTrue(tc, strstr(output, "you post a 140-gold bond, 10 units are loaded") != NULL);
  settle_assert(tc, &fixture, 810, 10, 10, 100, 0);
  CuAssertTrue(tc, ship->settlement_unresolved == 0);
  settle_query_value(tc, fixture.connection, status, value, sizeof(value));
  CuAssertStrEquals(tc, "1:Tern", value);

  settle_end(tc, &fixture);
}

/* A hull that left memory while her purge failed keeps her rows, and her
 * fleet slot is held for them: no spawn takes it, so her settlement is never
 * undone on another hull. Staff see the slot and purge the rows. */
void Test_vessel_settlement_holds_the_slot_of_a_hull_not_in_memory(CuTest *tc)
{
  static const char *lost_rows = "SELECT COUNT(*) FROM ship_interiors WHERE ship_id = 476";
  struct settle_fixture fixture;
  const char *output;
  long long cost;

  if (!settle_begin(tc, &fixture))
  {
    return;
  }

  /* She sank with Tern's unpaid purchase of ten salt aboard, and the purge
   * of her rows lost its connection: every row is still there. */
  CuAssertIntEquals(
      tc, 0, mysql_query(fixture.connection, "INSERT INTO ship_interiors (ship_id) VALUES (476)"));
  settle_stock(tc, &fixture, 0, 90);
  settle_open_purchase(tc, &fixture, SETTLE_TERN, SETTLE_LOST_SHIP);
  CuAssertTrue(tc, vessel_slot_free(SETTLE_LOST_SHIP));
  mysql_test_drop_connection_at("DELETE FROM ship_interiors", 1, FALSE);
  CuAssertTrue(tc, !vessel_delete_persistence(SETTLE_LOST_SHIP));
  CuAssertIntEquals(tc, 1, (int)settle_query_number(tc, fixture.connection, lost_rows));
  settle_assert(tc, &fixture, 1000, 0, 0, 90, 1);

  /* Her slot is not free, a hull in play or stowed never is, and no number
   * outside the fleet is. Her settlement waits, and Tern's accounts. */
  CuAssertTrue(tc, !vessel_slot_free(SETTLE_LOST_SHIP));
  CuAssertTrue(tc, !vessel_slot_free(SETTLE_SHIP));
  CuAssertTrue(tc, !vessel_slot_free(-1));
  CuAssertTrue(tc, !vessel_slot_free(GREYHAWK_MAXSHIPS));
  output = settle_login(&fixture);
  CuAssertStrEquals(tc, "", output);
  output = settle_command(&fixture, do_cargobuy, "salt 1");
  CuAssertTrue(tc, strstr(output, "still settling an earlier account") != NULL);
  settle_assert(tc, &fixture, 1000, 0, 0, 90, 1);

  /* A purge that fails again leaves the slot held. */
  output = settle_command(&fixture, do_shiplist, "");
  CuAssertTrue(tc,
               strstr(output, " 476 (stored records of a hull that is not in the game") != NULL);
  mysql_test_drop_connection_at("DELETE FROM ship_interiors", 1, FALSE);
  output = settle_command(&fixture, do_shippurge, "476");
  CuAssertTrue(tc, strstr(output, "Database cleanup failed; slot 476 is still held.") != NULL);
  CuAssertTrue(tc, !vessel_slot_free(SETTLE_LOST_SHIP));

  /* Purged, her settlement goes with her rows: the slot is free and his
   * accounts are open. */
  output = settle_command(&fixture, do_shippurge, "476");
  CuAssertTrue(tc, strstr(output, "Slot 476 held the stored records of a hull") != NULL);
  CuAssertTrue(tc, vessel_slot_free(SETTLE_LOST_SHIP));
  CuAssertIntEquals(tc, 0, (int)settle_query_number(tc, fixture.connection, lost_rows));
  settle_assert(tc, &fixture, 1000, 0, 0, 90, 0);
  output = settle_command(&fixture, do_shiplist, "");
  CuAssertTrue(tc, strstr(output, " 476 (stored records") == NULL);
  output = settle_command(&fixture, do_shippurge, "476");
  CuAssertTrue(tc, strstr(output, "Slot 476 is empty.") != NULL);
  cost = vessel_trade_buy_cost(fixture.base_price, 90, 1);
  output = settle_command(&fixture, do_cargobuy, "salt 1");
  CuAssertTrue(tc, strstr(output, "You load 1 units of salt") != NULL);
  settle_assert(tc, &fixture, 1000 - (int)cost, 1, 1, 89, 0);

  settle_end(tc, &fixture);
}
