/* Checked vessel purchases and payouts (vessels-ships study S15): each command
 * that takes or pays gold for a hull writes the captain's player file and the
 * ship's rows inside itself. The tests fail each store in turn: a player file
 * that cannot be saved, and a real lost connection at a statement of the
 * ship's side, on the real tables with the rows of a test hull and captain. */

#include "CuTest.h"

#include "conf.h"
#include "../../src/core/sysdep.h"
#include "../../src/core/structs.h"
#include "../../src/core/utils.h"
#include "../../src/core/comm.h"
#include "../../src/core/db.h"
#include "../../src/core/handler.h"
#include "../../src/database/mysql.h"
#include "../../src/net/protocol.h"
#include "../../src/vessels/vessels.h"
#include "test_vessel_stores.h"

#include <stdlib.h>
#include <string.h>

extern struct greyhawk_ship_data greyhawk_ships[GREYHAWK_MAXSHIPS];

/* A high fleet slot and ports of their own keep these rows clear of the suite. */
#define PAYMENT_SHIP 487
#define PAYMENT_PORT 104
#define PAYMENT_ORIGIN 105
#define PAYMENT_GOLD 200000

/* What the hull's rows hold, as one number: her name, refits, fore armor and
 * its ceiling, design speed, weapons with their rounds, and crew. */
#define PAYMENT_ROWS                                                                               \
  "SELECT CRC32(CONCAT_WS('|', i.vessel_name, i.upgrades, r.farmor, r.maxfarmor, r.maxspeed, "     \
  "(SELECT CONCAT(COUNT(*), ':', COALESCE(SUM(ammo), 0)) FROM ship_weapons WHERE ship_id = 487), " \
  "(SELECT COUNT(*) FROM ship_crew_roster WHERE ship_id = 487))) "                                 \
  "FROM ship_interiors i JOIN ship_runtime_state r ON r.ship_id = i.ship_id "                      \
  "WHERE i.ship_id = 487"

/* Vane aboard her warship, berthed in a one-room harbor, with a player file
 * and her hull saved. */
struct payment_berth
{
  struct room_data room;
  struct char_data captain;
  struct player_special_data specials;
  struct descriptor_data descriptor;
  char output[8192];
  struct greyhawk_ship_data *ship;
  struct room_data *saved_world;
  room_rnum saved_top_of_world;
  struct vessel_test_stores stores;
};

static void payment_rows_end(MYSQL *connection)
{
  mysql_query(connection, "DELETE FROM vessel_bounties WHERE player_name = 'Vane'");
  mysql_query(connection, "DELETE FROM freight_contracts WHERE origin_vnum = 105");
}

/* @return FALSE when the database cases are off */
static bool payment_begin(CuTest *tc, struct payment_berth *berth)
{
  struct greyhawk_ship_data *ship;
  struct obj_data *hull;

  memset(berth, 0, sizeof(*berth));
  berth->room.number = PAYMENT_PORT;
  berth->room.zone = NOWHERE;
  SET_BIT_AR(berth->room.room_flags, ROOM_DOCKABLE);
  berth->saved_world = world;
  berth->saved_top_of_world = top_of_world;
  world = &berth->room;
  top_of_world = 0;

  berth->captain.player_specials = &berth->specials;
  berth->captain.player.name = CuMutableString("Vane");
  berth->captain.player.level = 20;
  IN_ROOM(&berth->captain) = 0;
  GET_POS(&berth->captain) = POS_STANDING;
  GET_GOLD(&berth->captain) = PAYMENT_GOLD;
  if (!vessel_test_stores_begin(tc, &berth->stores, PAYMENT_SHIP, &berth->captain))
  {
    world = berth->saved_world;
    top_of_world = berth->saved_top_of_world;
    return FALSE;
  }
  vessel_trade_ensure_schema();
  vessel_contracts_ensure_schema();
  vessel_piracy_ensure_schema();
  vessel_piracy_clear_laws();
  payment_rows_end(berth->stores.connection);

  berth->captain.desc = &berth->descriptor;
  berth->descriptor.character = &berth->captain;
  berth->descriptor.output = berth->output;
  berth->descriptor.bufspace = sizeof(berth->output) - 1;
  berth->descriptor.pProtocol = ProtocolCreate();
  CuAssertPtrNotNull(tc, berth->descriptor.pProtocol);

  ship = &greyhawk_ships[PAYMENT_SHIP];
  memset(ship, 0, sizeof(*ship));
  ship->active = TRUE;
  ship->shipnum = PAYMENT_SHIP;
  ship->vessel_type = VESSEL_WARSHIP;
  ship->maxspeed = (short int)vessel_class_handling(VESSEL_WARSHIP)->speed;
  ship->docked_to_ship = -1;
  strlcpy(ship->id, "PY", sizeof(ship->id));
  strlcpy(ship->name, "the Tally", sizeof(ship->name));
  strlcpy(ship->owner, "Vane", sizeof(ship->owner));
  vessel_initialize_condition(ship, vessel_class_condition(VESSEL_WARSHIP)->beam_armor);
  vessel_fit_default_weapons(ship);
  hull = create_obj();
  hull->name = strdup("tally hull");
  obj_to_room(hull, 0);
  ship->shipobj = hull;
  ship->dock = PAYMENT_PORT;
  berth->room.ship = ship;
  berth->ship = ship;
  CuAssertTrue(tc, vessel_save_hull(ship));
  return TRUE;
}

static void payment_end(CuTest *tc, struct payment_berth *berth)
{
  extract_obj(berth->ship->shipobj);
  memset(berth->ship, 0, sizeof(*berth->ship));
  ProtocolDestroy(berth->descriptor.pProtocol);
  world = berth->saved_world;
  top_of_world = berth->saved_top_of_world;
  vessel_piracy_clear_laws();
  mysql_test_drop_connection_at(NULL, 0, FALSE);
  payment_rows_end(berth->stores.connection);
  vessel_test_stores_end(tc, &berth->stores);
}

/* Run a command as Vane and return what it printed. */
static const char *payment_command(struct payment_berth *berth, ACMD_DECL((*command)),
                                   const char *argument)
{
  memset(berth->output, 0, sizeof(berth->output));
  berth->descriptor.bufptr = 0;
  berth->descriptor.bufspace = sizeof(berth->output) - 1;
  command(&berth->captain, argument, 0, 0);
  return berth->output;
}

/* The same, with a player file that cannot be saved. */
static const char *payment_command_unsaved(struct payment_berth *berth, ACMD_DECL((*command)),
                                           const char *argument)
{
  const char *output;

  GET_PFILEPOS(&berth->captain) = -1;
  output = payment_command(berth, command, argument);
  GET_PFILEPOS(&berth->captain) = 0;
  return output;
}

/* Both stores hold this gold and this hull. */
static void payment_assert_unchanged(CuTest *tc, struct payment_berth *berth,
                                     const struct greyhawk_ship_data *before, int gold,
                                     long long rows)
{
  CuAssertIntEquals(tc, 0, memcmp(berth->ship, before, sizeof(*before)));
  CuAssertIntEquals(tc, gold, GET_GOLD(&berth->captain));
  CuAssertIntEquals(tc, gold, vessel_test_file_gold(tc, &berth->stores));
  CuAssertTrue(tc, vessel_test_number(tc, &berth->stores, PAYMENT_ROWS) == rows);
}

/* Every shipyard job that charges gold: with a player file that cannot be
 * saved nothing is taken and nothing is done; with a hull that cannot be
 * written she is put back and the gold returned; else both stores hold it. */
void Test_vessel_purchase_writes_both_stores_or_neither(CuTest *tc)
{
  static const struct
  {
    ACMD_DECL((*command));
    const char *argument;
    const char *lost; /* the statement of the hull's write that loses the connection */
    bool written;     /* the server has run it and its reply is lost */
  } jobs[] = {
      /* A statement outside a transaction is lost with its reply: sent on a
       * lost connection, the library would reconnect and send it again. So
       * the rows hold the job when the hull is written back. */
      {do_shiphire, "gunner green", "INSERT INTO ship_crew_roster", TRUE},
      {do_shipweapon, "buy large ballista rear", "INSERT INTO ship_weapons", FALSE},
      {do_shipequip, "buy ram", "REPLACE INTO ship_runtime_state", TRUE},
      {do_shiprearm, "all", "DELETE FROM ship_weapons", FALSE},
      {do_shiprepair, "armor", "INSERT INTO ship_interiors", TRUE},
      {do_shipupgrade, "plating", "UPDATE ship_interiors SET upgrades", TRUE},
      {do_shipchristen, "Storm Petrel", "UPDATE ship_interiors SET owner", TRUE},
  };
  struct payment_berth berth;
  struct greyhawk_ship_data before;
  const char *output;
  long long rows;
  size_t i;
  int ceiling;
  int gold;

  if (!payment_begin(tc, &berth))
  {
    return;
  }
  ceiling = berth.ship->maxfarmor;

  for (i = 0; i < sizeof(jobs) / sizeof(jobs[0]); i++)
  {
    /* Work for the yard: an empty weapon and a worn arc, saved. */
    berth.ship->slot[0].ammo = 0;
    berth.ship->farmor = (unsigned char)(berth.ship->maxfarmor - 10);
    berth.ship->maintenance_ticks = 0;
    CuAssertTrue(tc, vessel_save_hull(berth.ship));
    before = *berth.ship;
    gold = GET_GOLD(&berth.captain);
    rows = vessel_test_number(tc, &berth.stores, PAYMENT_ROWS);

    output = payment_command_unsaved(&berth, jobs[i].command, jobs[i].argument);
    CuAssertTrue(tc,
                 strstr(output, "Your payment could not be recorded; no gold was taken.") != NULL);
    payment_assert_unchanged(tc, &berth, &before, gold, rows);

    mysql_test_drop_connection_at(jobs[i].lost, 1, jobs[i].written);
    output = payment_command(&berth, jobs[i].command, jobs[i].argument);
    CuAssertTrue(tc, strstr(output, "The harbor office could not record that, so nothing was "
                                    "done and your ") != NULL);
    CuAssertTrue(tc, strstr(output, " gold is returned.") != NULL);
    payment_assert_unchanged(tc, &berth, &before, gold, rows);

    output = payment_command(&berth, jobs[i].command, jobs[i].argument);
    CuAssertTrue(tc, strstr(output, "could not") == NULL);
    CuAssertTrue(tc, GET_GOLD(&berth.captain) < gold);
    CuAssertIntEquals(tc, GET_GOLD(&berth.captain), vessel_test_file_gold(tc, &berth.stores));
    CuAssertTrue(tc, memcmp(berth.ship, &before, sizeof(before)) != 0);
    CuAssertTrue(tc, vessel_test_number(tc, &berth.stores, PAYMENT_ROWS) != rows);
  }

  /* The plating's points are in her rows with its refit bit. */
  CuAssertStrEquals(tc, "Storm Petrel", berth.ship->name);
  CuAssertTrue(tc, vessel_test_number(tc, &berth.stores,
                                      "SELECT maxfarmor FROM ship_runtime_state "
                                      "WHERE ship_id = 487") == berth.ship->maxfarmor);
  CuAssertTrue(tc, berth.ship->maxfarmor > ceiling);

  /* Staff rename free, with nothing to charge or return. */
  GET_LEVEL(&berth.captain) = LVL_IMMORT;
  gold = GET_GOLD(&berth.captain);
  mysql_test_drop_connection_at("INSERT INTO ship_interiors", 1, TRUE);
  output = payment_command(&berth, do_shipchristen, "Gray Petrel");
  CuAssertTrue(tc, strstr(output, "The harbor office could not record that, so nothing was "
                                  "done.") != NULL);
  CuAssertStrEquals(tc, "Storm Petrel", berth.ship->name);
  CuAssertIntEquals(tc, gold, GET_GOLD(&berth.captain));
  GET_LEVEL(&berth.captain) = 20;

  /* Without a database the hull cannot be written, so nothing is sold. */
  mysql_available = FALSE;
  before = *berth.ship;
  output = payment_command(&berth, do_shiphire, "bosun green");
  mysql_available = TRUE;
  CuAssertTrue(tc, strstr(output, "so nothing was done and your ") != NULL);
  CuAssertIntEquals(tc, 0, memcmp(berth.ship, &before, sizeof(before)));
  CuAssertIntEquals(tc, gold, GET_GOLD(&berth.captain));

  payment_end(tc, &berth);
}

/* A weapon and equipment sold: a hull that cannot be written sells nothing;
 * a payment that cannot be saved puts the item back; and when the put-back
 * cannot be written either, the sale stands and the gold is paid in memory. */
void Test_vessel_sale_is_put_back_when_the_gold_cannot_be_saved(CuTest *tc)
{
  struct payment_berth berth;
  struct greyhawk_ship_data before;
  const char *output;
  long long rows;
  int gold;
  int value;

  if (!payment_begin(tc, &berth))
  {
    return;
  }
  before = *berth.ship;
  gold = GET_GOLD(&berth.captain);
  rows = vessel_test_number(tc, &berth.stores, PAYMENT_ROWS);
  value = vessel_slot_sale_value(&berth.ship->slot[0], berth.ship->vessel_type);
  CuAssertTrue(tc, value > 0);

  mysql_test_drop_connection_at("DELETE FROM ship_weapons", 1, FALSE);
  output = payment_command(&berth, do_shipweapon, "sell 0");
  CuAssertTrue(tc, strstr(output, "The harbor office could not record that, so nothing was "
                                  "sold.") != NULL);
  payment_assert_unchanged(tc, &berth, &before, gold, rows);

  output = payment_command_unsaved(&berth, do_shipweapon, "sell 0");
  CuAssertTrue(tc, strstr(output, "Your payment could not be recorded, so the sale is undone.") !=
                       NULL);
  payment_assert_unchanged(tc, &berth, &before, gold, rows);

  /* The second write of her interior row is the put-back's. */
  mysql_test_drop_connection_at("INSERT INTO ship_interiors", 2, TRUE);
  output = payment_command_unsaved(&berth, do_shipweapon, "sell 0");
  CuAssertTrue(tc, strstr(output, "out of slot 0 and pay you") != NULL);
  CuAssertIntEquals(tc, VESSEL_SLOT_EMPTY, berth.ship->slot[0].type);
  CuAssertIntEquals(tc, gold + value, GET_GOLD(&berth.captain));
  CuAssertIntEquals(tc, gold, vessel_test_file_gold(tc, &berth.stores));
  CuAssertTrue(tc, vessel_test_number(tc, &berth.stores, PAYMENT_ROWS) != rows);

  /* Saved, both stores hold a sale. */
  gold = GET_GOLD(&berth.captain);
  value = vessel_slot_sale_value(&berth.ship->slot[1], berth.ship->vessel_type);
  output = payment_command(&berth, do_shipweapon, "sell 1");
  CuAssertTrue(tc, strstr(output, "out of slot 1 and pay you") != NULL);
  CuAssertIntEquals(tc, gold + value, GET_GOLD(&berth.captain));
  CuAssertIntEquals(tc, gold + value, vessel_test_file_gold(tc, &berth.stores));

  /* Equipment is sold the same way. */
  payment_command(&berth, do_shipequip, "buy ram");
  CuAssertTrue(tc, vessel_equipment_slot(berth.ship, VESSEL_EQUIPMENT_RAM) >= 0);
  before = *berth.ship;
  gold = GET_GOLD(&berth.captain);
  rows = vessel_test_number(tc, &berth.stores, PAYMENT_ROWS);
  output = payment_command_unsaved(&berth, do_shipequip, "sell ram");
  CuAssertTrue(tc, strstr(output, "so the sale is undone") != NULL);
  payment_assert_unchanged(tc, &berth, &before, gold, rows);
  output = payment_command(&berth, do_shipequip, "sell ram");
  CuAssertTrue(tc, strstr(output, "The shipwrights remove the Ram and pay you") != NULL);
  CuAssertIntEquals(tc, -1, vessel_equipment_slot(berth.ship, VESSEL_EQUIPMENT_RAM));
  CuAssertTrue(tc, GET_GOLD(&berth.captain) > gold);
  CuAssertIntEquals(tc, GET_GOLD(&berth.captain), vessel_test_file_gold(tc, &berth.stores));

  payment_end(tc, &berth);
}

/* The admiralty's two fees: a bounty paid off and a letter of marque. */
void Test_vessel_admiralty_fee_is_refused_or_refunded(CuTest *tc)
{
  struct payment_berth berth;
  const char *output;
  int gold;

  if (!payment_begin(tc, &berth))
  {
    return;
  }
  CuAssertTrue(tc, vessel_add_bounty("Vane", 400));
  gold = GET_GOLD(&berth.captain);

  output = payment_command_unsaved(&berth, do_bounty, "pay");
  CuAssertTrue(tc, strstr(output, "no gold was taken") != NULL);
  CuAssertIntEquals(tc, 400, vessel_get_bounty("Vane"));
  CuAssertIntEquals(tc, gold, GET_GOLD(&berth.captain));

  mysql_test_drop_connection_at("UPDATE vessel_bounties SET bounty = 0", 1, FALSE);
  output = payment_command(&berth, do_bounty, "pay");
  CuAssertTrue(tc, strstr(output, "The clerk cannot record the settlement; your 500 gold is "
                                  "returned.") != NULL);
  CuAssertIntEquals(tc, 400, vessel_get_bounty("Vane"));
  CuAssertIntEquals(tc, gold, GET_GOLD(&berth.captain));
  CuAssertIntEquals(tc, gold, vessel_test_file_gold(tc, &berth.stores));

  output = payment_command(&berth, do_bounty, "pay");
  CuAssertTrue(tc, strstr(output, "You pay 500 gold.") != NULL);
  CuAssertIntEquals(tc, 0, vessel_get_bounty("Vane"));
  CuAssertIntEquals(tc, gold - 500, GET_GOLD(&berth.captain));
  CuAssertIntEquals(tc, gold - 500, vessel_test_file_gold(tc, &berth.stores));

  gold = GET_GOLD(&berth.captain);
  output = payment_command_unsaved(&berth, do_marque, "");
  CuAssertTrue(tc, strstr(output, "no gold was taken") != NULL);
  CuAssertTrue(tc, !vessel_has_letter_of_marque("Vane"));
  CuAssertIntEquals(tc, gold, GET_GOLD(&berth.captain));

  /* The command's second query issues the letter. */
  mysql_test_fail_nth_query(2);
  output = payment_command(&berth, do_marque, "");
  mysql_test_clear_query_failure();
  CuAssertTrue(tc, strstr(output, "The clerk cannot complete the commission; your ") != NULL);
  CuAssertTrue(tc, !vessel_has_letter_of_marque("Vane"));
  CuAssertIntEquals(tc, gold, GET_GOLD(&berth.captain));
  CuAssertIntEquals(tc, gold, vessel_test_file_gold(tc, &berth.stores));

  /* A letter issued with its reply lost is on record: it is paid for. */
  mysql_test_drop_connection_at("INSERT INTO vessel_bounties (player_name, marque_until)", 1, TRUE);
  output = payment_command(&berth, do_marque, "");
  CuAssertTrue(tc, strstr(output, "The admiralty commissions you as a privateer") != NULL);
  CuAssertTrue(tc, vessel_has_letter_of_marque("Vane"));
  CuAssertTrue(tc, GET_GOLD(&berth.captain) < gold);
  CuAssertIntEquals(tc, GET_GOLD(&berth.captain), vessel_test_file_gold(tc, &berth.stores));

  payment_end(tc, &berth);
}

/* Freight delivered: the contract and the hold are written together, then
 * the payout is saved, and a payout that cannot be saved loads the freight
 * again. */
void Test_vessel_delivery_is_undone_when_the_payout_cannot_be_saved(CuTest *tc)
{
  struct payment_berth berth;
  const char *output;
  char deliver[32];
  char query[256];
  char status[128];
  char manifest[128];
  int contract;
  int salt;
  int gold;
  int i;

  if (!payment_begin(tc, &berth))
  {
    return;
  }
  salt = (int)vessel_test_number(tc, &berth.stores,
                                 "SELECT commodity_id FROM trade_commodities WHERE name = 'salt'");
  snprintf(manifest, sizeof(manifest),
           "SELECT COALESCE(SUM(item_count), 0) FROM ship_cargo_manifest WHERE ship_id = %d",
           PAYMENT_SHIP);

  /* Two jobs of ten units each, taken by Vane and bound for this port. */
  for (i = 0; i < 2; i++)
  {
    snprintf(query, sizeof(query),
             "INSERT INTO freight_contracts (origin_vnum, destination_vnum, commodity_id, "
             "quantity, payout, status, taken_by) VALUES (%d, %d, %d, 10, 300, %d, 'Vane')",
             PAYMENT_ORIGIN, PAYMENT_PORT, salt, CONTRACT_STATUS_TAKEN);
    CuAssertIntEquals(tc, 0, mysql_query(berth.stores.connection, query));
  }
  contract = (int)mysql_insert_id(berth.stores.connection);
  snprintf(deliver, sizeof(deliver), "%d", contract);
  snprintf(status, sizeof(status), "SELECT status FROM freight_contracts WHERE contract_id = %d",
           contract);
  berth.ship->cargo[0].commodity_id = salt;
  berth.ship->cargo[0].quantity = 20;
  CuAssertTrue(tc, vessel_db_save_cargo(berth.ship));
  gold = GET_GOLD(&berth.captain);

  /* The books cannot be written: nothing is unloaded and nothing paid. */
  mysql_test_drop_connection_at("UPDATE freight_contracts SET status", 1, FALSE);
  output = payment_command(&berth, do_contractdeliver, deliver);
  CuAssertTrue(tc, strstr(output, "The freight office cannot record the delivery; the freight "
                                  "stays aboard.") != NULL);
  CuAssertIntEquals(tc, 20, berth.ship->cargo[0].quantity);
  CuAssertIntEquals(tc, gold, GET_GOLD(&berth.captain));
  CuAssertTrue(tc, vessel_test_number(tc, &berth.stores, status) == CONTRACT_STATUS_TAKEN);
  CuAssertTrue(tc, vessel_test_number(tc, &berth.stores, manifest) == 20);

  /* The payout cannot be saved: the freight is loaded again, in her rows too. */
  output = payment_command_unsaved(&berth, do_contractdeliver, deliver);
  CuAssertTrue(tc, strstr(output, "Your payment could not be recorded, so the delivery is "
                                  "undone.") != NULL);
  CuAssertIntEquals(tc, 20, berth.ship->cargo[0].quantity);
  CuAssertIntEquals(tc, gold, GET_GOLD(&berth.captain));
  CuAssertTrue(tc, vessel_test_number(tc, &berth.stores, status) == CONTRACT_STATUS_TAKEN);
  CuAssertTrue(tc, vessel_test_number(tc, &berth.stores, manifest) == 20);

  /* A COMMIT without a reply is sent again: delivered and paid once. */
  mysql_test_drop_connection_at("COMMIT", 1, TRUE);
  output = payment_command(&berth, do_contractdeliver, deliver);
  CuAssertTrue(tc, strstr(output, "Freight delivered. The consignee pays 300 gold.") != NULL);
  CuAssertIntEquals(tc, 10, berth.ship->cargo[0].quantity);
  CuAssertIntEquals(tc, gold + 300, GET_GOLD(&berth.captain));
  CuAssertIntEquals(tc, gold + 300, vessel_test_file_gold(tc, &berth.stores));
  CuAssertTrue(tc, vessel_test_number(tc, &berth.stores, status) == CONTRACT_STATUS_DONE);
  CuAssertTrue(tc, vessel_test_number(tc, &berth.stores, manifest) == 10);
  output = payment_command(&berth, do_contractdeliver, deliver);
  CuAssertTrue(tc, strstr(output, "That contract is not yours to deliver.") != NULL);

  /* The other job: the payout cannot be saved and the delivery cannot be
   * taken back in the books, so it stands and the gold is paid in memory. */
  contract--;
  snprintf(deliver, sizeof(deliver), "%d", contract);
  snprintf(status, sizeof(status), "SELECT status FROM freight_contracts WHERE contract_id = %d",
           contract);
  gold = GET_GOLD(&berth.captain);
  mysql_test_drop_connection_at("UPDATE freight_contracts SET status", 2, FALSE);
  output = payment_command_unsaved(&berth, do_contractdeliver, deliver);
  CuAssertTrue(tc, strstr(output, "Freight delivered. The consignee pays 300 gold.") != NULL);
  CuAssertIntEquals(tc, 0, berth.ship->cargo[0].quantity);
  CuAssertIntEquals(tc, gold + 300, GET_GOLD(&berth.captain));
  CuAssertIntEquals(tc, gold, vessel_test_file_gold(tc, &berth.stores));
  CuAssertTrue(tc, vessel_test_number(tc, &berth.stores, status) == CONTRACT_STATUS_DONE);
  CuAssertTrue(tc, vessel_test_number(tc, &berth.stores, manifest) == 0);

  payment_end(tc, &berth);
}
