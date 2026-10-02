/* Vessel rewards and economy (vessels-ships study S7): salvage, the rewards
 * of a sinking, renown and its hiring gates, Ship Damage Control, the cargo
 * sale modifiers, contraband, and customs. */

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

extern struct greyhawk_ship_data greyhawk_ships[GREYHAWK_MAXSHIPS];

/* High fleet slots keep these fixtures clear of the rest of the suite. */
#define REWARDS_TARGET 490
#define REWARDS_VICTOR 491
#define REWARDS_ALLY 492
#define REWARDS_STRANGER 493
#define REWARDS_FAR_ALLY 494
#define REWARDS_SISTER 495
#define REWARDS_BERTHED 496

static struct greyhawk_ship_data *rewards_hull(int slot, enum vessel_class vessel_type,
                                               const char *name, const char *owner, double x)
{
  struct greyhawk_ship_data *ship = &greyhawk_ships[slot];

  memset(ship, 0, sizeof(*ship));
  ship->active = TRUE;
  ship->shipnum = slot;
  ship->vessel_type = vessel_type;
  ship->maxspeed = (short int)vessel_class_handling(vessel_type)->speed;
  ship->docked_to_ship = -1;
  ship->x = x;
  strlcpy(ship->id, "RW", sizeof(ship->id));
  strlcpy(ship->name, name, sizeof(ship->name));
  strlcpy(ship->owner, owner, sizeof(ship->owner));
  vessel_initialize_condition(ship, vessel_class_condition(vessel_type)->beam_armor);
  return ship;
}

static void rewards_clear(void)
{
  int slot;

  for (slot = REWARDS_TARGET; slot <= REWARDS_BERTHED; slot++)
  {
    memset(&greyhawk_ships[slot], 0, sizeof(greyhawk_ships[0]));
  }
}

/* A player in the game, by name. */
struct rewards_player
{
  struct char_data ch;
  struct player_special_data specials;
};

static void rewards_player_online(struct rewards_player *player, const char *name)
{
  memset(player, 0, sizeof(*player));
  player->ch.player_specials = &player->specials;
  player->ch.player.name = CuMutableString(name);
  IN_ROOM(&player->ch) = NOWHERE;
  player->ch.next = character_list;
  character_list = &player->ch;
}

/* Her owner, Tern, aboard his hull, berthed in a one-room harbor. */
struct rewards_berth
{
  struct room_data room;
  struct obj_data hull;
  struct char_data captain;
  struct player_special_data specials;
  struct descriptor_data descriptor;
  char output[8192];
  struct room_data *saved_world;
  room_rnum saved_top_of_world;
  struct char_data *saved_character_list;
};

static struct greyhawk_ship_data *rewards_berth_begin(CuTest *tc, struct rewards_berth *berth,
                                                      enum vessel_class vessel_type)
{
  struct greyhawk_ship_data *ship;

  memset(berth, 0, sizeof(*berth));
  berth->room.number = 100;
  SET_BIT_AR(berth->room.room_flags, ROOM_DOCKABLE);
  berth->saved_world = world;
  berth->saved_top_of_world = top_of_world;
  world = &berth->room;
  top_of_world = 0;

  ship = rewards_hull(REWARDS_TARGET, vessel_type, "the Tern", "Tern", 0.0);
  ship->shipobj = &berth->hull;
  IN_ROOM(&berth->hull) = 0;
  ship->dock = 100;
  berth->room.ship = ship;

  berth->captain.player_specials = &berth->specials;
  berth->captain.player.name = CuMutableString("Tern");
  berth->captain.player.level = 20;
  IN_ROOM(&berth->captain) = 0;
  GET_POS(&berth->captain) = POS_STANDING;
  GET_GOLD(&berth->captain) = 100000;
  berth->captain.desc = &berth->descriptor;
  berth->descriptor.character = &berth->captain;
  berth->descriptor.output = berth->output;
  berth->descriptor.bufspace = sizeof(berth->output) - 1;
  berth->descriptor.pProtocol = ProtocolCreate();
  CuAssertPtrNotNull(tc, berth->descriptor.pProtocol);
  berth->saved_character_list = character_list;
  berth->captain.next = character_list;
  character_list = &berth->captain;
  return ship;
}

/* Run a command and return what it printed. */
static const char *rewards_berth_command(struct rewards_berth *berth, ACMD_DECL((*command)),
                                         const char *argument)
{
  memset(berth->output, 0, sizeof(berth->output));
  berth->descriptor.bufptr = 0;
  berth->descriptor.bufspace = sizeof(berth->output) - 1;
  command(&berth->captain, argument, 0, 0);
  return berth->output;
}

static void rewards_berth_end(struct rewards_berth *berth)
{
  ProtocolDestroy(berth->descriptor.pProtocol);
  character_list = berth->saved_character_list;
  world = berth->saved_world;
  top_of_world = berth->saved_top_of_world;
  rewards_clear();
}

static MYSQL *rewards_open_test_database(void)
{
  const char *port_text;
  MYSQL *connection;

  connection = mysql_init(NULL);
  if (connection == NULL)
  {
    return NULL;
  }
  port_text = getenv("LUMINARI_TEST_MYSQL_PORT");
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

static bool rewards_database_enabled(void)
{
  const char *enabled = getenv("LUMINARI_TEST_MYSQL_ENABLE");

  return enabled != NULL && strcmp(enabled, "1") == 0;
}

/* The first column of the first row of query, or "" */
static void rewards_query_value(CuTest *tc, MYSQL *connection, const char *query, char *value,
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

/* The tables a sinking's settlement writes, shadowing any real ones. */
static bool rewards_prize_tables(MYSQL *connection)
{
  return mysql_query(connection, "CREATE TEMPORARY TABLE vessel_insurance_claims ("
                                 "claim_id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY, "
                                 "ship_id INT NOT NULL, owner VARCHAR(64) NOT NULL, "
                                 "ship_name VARCHAR(128) NOT NULL, amount INT NOT NULL, "
                                 "status VARCHAR(16) NOT NULL DEFAULT 'pending', "
                                 "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP, "
                                 "paid_at TIMESTAMP NULL DEFAULT NULL) ENGINE=InnoDB") == 0 &&
         mysql_query(connection, "CREATE TEMPORARY TABLE player_mail ("
                                 "mail_id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY, "
                                 "sender VARCHAR(255) NOT NULL, receiver VARCHAR(255) NOT NULL, "
                                 "subject VARCHAR(255) NOT NULL, message TEXT NOT NULL, "
                                 "date_sent DATE DEFAULT NULL) ENGINE=InnoDB") == 0 &&
         mysql_query(connection, "CREATE TEMPORARY TABLE vessel_bounties ("
                                 "player_name VARCHAR(64) PRIMARY KEY, "
                                 "bounty INT NOT NULL DEFAULT 0, "
                                 "marque_until INT NOT NULL DEFAULT 0, "
                                 "last_offense_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP) "
                                 "ENGINE=InnoDB") == 0 &&
         mysql_query(connection, "CREATE TEMPORARY TABLE ship_runtime_state ("
                                 "ship_id INT PRIMARY KEY, renown INT NOT NULL DEFAULT 0, "
                                 "last_attacker INT NOT NULL DEFAULT 0) ENGINE=InnoDB") == 0;
}

void Test_vessel_salvage_counts_what_is_left(CuTest *tc)
{
  struct greyhawk_ship_data *ship;

  /* A frigate at her class price, whole, with three large ballistae. */
  ship = rewards_hull(REWARDS_TARGET, VESSEL_WARSHIP, "the Tern", "", 0.0);
  vessel_fit_default_weapons(ship);
  CuAssertIntEquals(tc, 44000, vessel_hull_price(ship));
  CuAssertIntEquals(tc, (44000 + 3 * 500) / 8, vessel_salvage_value(ship));

  /* Stripped of her armor (155 of 525 points left) with a weapon destroyed:
   * a damaged one still counts. */
  ship->farmor = 0;
  ship->parmor = 0;
  ship->rarmor = 0;
  ship->sarmor = 0;
  ship->slot[0].damage = VESSEL_WEAPON_DESTROYED;
  ship->slot[1].damage = 20;
  CuAssertIntEquals(tc, (44000 * 155 / 525 + 2 * 500) / 8, vessel_salvage_value(ship));

  rewards_clear();
}

void Test_vessel_sinking_shares_renown_among_allies(CuTest *tc)
{
  struct rewards_player corr;
  struct rewards_player wren;
  struct rewards_player ash;
  struct rewards_player tern;
  struct group_data fleet;
  struct room_data harbor;
  struct obj_data berthed_hull;
  struct room_data *saved_world;
  room_rnum saved_top_of_world;
  struct char_data *saved_character_list;
  struct greyhawk_ship_data *target;
  struct greyhawk_ship_data *victor;
  struct greyhawk_ship_data *ally;
  struct greyhawk_ship_data *stranger;
  struct greyhawk_ship_data *far_ally;
  struct greyhawk_ship_data *berthed;
  struct greyhawk_ship_data *sister;
  MYSQL *saved_conn;
  MYSQL *connection;
  bool saved_mysql_available;

  if (!rewards_database_enabled())
  {
    return;
  }
  connection = rewards_open_test_database();
  if (connection == NULL)
  {
    CuFail(tc, "could not connect to the explicitly configured test database");
    return;
  }
  if (!rewards_prize_tables(connection))
  {
    mysql_close(connection);
    CuFail(tc, "could not create the isolated settlement fixture");
    return;
  }
  saved_conn = conn;
  saved_mysql_available = mysql_available;
  conn = connection;
  mysql_available = TRUE;

  saved_character_list = character_list;
  rewards_player_online(&corr, "Corr");
  rewards_player_online(&wren, "Wren");
  rewards_player_online(&ash, "Ash");
  rewards_player_online(&tern, "Tern");
  memset(&fleet, 0, sizeof(fleet));
  corr.ch.group = &fleet;
  wren.ch.group = &fleet;
  tern.ch.group = &fleet;

  /* Corr's frigate sinks Tern's with 400 renown. Wren's hull in sight shares
   * the frigate's 285 renown; Ash is not in the group, Wren's other hulls are
   * out of sight or berthed in port, and Tern's own sister hull never
   * shares. */
  memset(&harbor, 0, sizeof(harbor));
  memset(&berthed_hull, 0, sizeof(berthed_hull));
  SET_BIT_AR(harbor.room_flags, ROOM_DOCKABLE);
  saved_world = world;
  saved_top_of_world = top_of_world;
  world = &harbor;
  top_of_world = 0;
  target = rewards_hull(REWARDS_TARGET, VESSEL_WARSHIP, "the Tern", "Tern", 0.0);
  target->renown = 400;
  victor = rewards_hull(REWARDS_VICTOR, VESSEL_WARSHIP, "the Gull", "Corr", 5.0);
  ally = rewards_hull(REWARDS_ALLY, VESSEL_SHIP, "the Wren", "Wren", 10.0);
  stranger = rewards_hull(REWARDS_STRANGER, VESSEL_SHIP, "the Ash", "Ash", 3.0);
  far_ally = rewards_hull(REWARDS_FAR_ALLY, VESSEL_SHIP, "the Far Wren", "Wren", 400.0);
  sister = rewards_hull(REWARDS_SISTER, VESSEL_SHIP, "the Sister", "Tern", 2.0);
  berthed = rewards_hull(REWARDS_BERTHED, VESSEL_SHIP, "the Berthed Wren", "Wren", 4.0);
  berthed->shipobj = &berthed_hull;
  IN_ROOM(&berthed_hull) = 0;
  CuAssertIntEquals(tc, 285, vessel_settle_sinking(target, victor));
  CuAssertIntEquals(tc, 142, victor->renown);
  CuAssertIntEquals(tc, 142, ally->renown);
  CuAssertIntEquals(tc, 0, stranger->renown);
  CuAssertIntEquals(tc, 0, far_ally->renown);
  CuAssertIntEquals(tc, 0, berthed->renown);
  CuAssertIntEquals(tc, 0, sister->renown);
  CuAssertIntEquals(tc, 115, target->renown);

  /* Renown floors at zero. */
  CuAssertIntEquals(tc, 285, vessel_settle_sinking(target, victor));
  CuAssertIntEquals(tc, 0, target->renown);

  /* Her owner's other hull wins nothing sinking her, nor does an NPC hull. */
  target->renown = 400;
  CuAssertIntEquals(tc, 0, vessel_settle_sinking(target, sister));
  victor->owner[0] = '\0';
  CuAssertIntEquals(tc, 0, vessel_settle_sinking(target, victor));
  CuAssertIntEquals(tc, 400, target->renown);

  /* An NPC hull, a raider with her renown, trains crews but moves none. */
  strlcpy(victor->owner, "Corr", sizeof(victor->owner));
  victor->renown = 0;
  target->owner[0] = '\0';
  target->renown = 2000;
  CuAssertIntEquals(tc, 0, vessel_settle_sinking(target, victor));
  CuAssertIntEquals(tc, 0, victor->renown);
  CuAssertIntEquals(tc, 2000, target->renown);

  character_list = saved_character_list;
  world = saved_world;
  top_of_world = saved_top_of_world;
  rewards_clear();
  conn = saved_conn;
  mysql_available = saved_mysql_available;
  mysql_close(connection);
}

void Test_vessel_sinking_pays_prize_money_and_the_bounty(CuTest *tc)
{
  struct rewards_berth berth;
  struct greyhawk_ship_data *target;
  struct greyhawk_ship_data *victor;
  MYSQL *saved_conn;
  MYSQL *connection;
  bool saved_mysql_available;
  char value[64];

  if (!rewards_database_enabled())
  {
    return;
  }
  connection = rewards_open_test_database();
  if (connection == NULL)
  {
    CuFail(tc, "could not connect to the explicitly configured test database");
    return;
  }
  if (!rewards_prize_tables(connection) ||
      mysql_query(connection, "INSERT INTO ship_runtime_state (ship_id, renown, last_attacker) "
                              "VALUES (490, 400, 491), (491, 0, 0)") != 0 ||
      mysql_query(connection, "INSERT INTO vessel_bounties (player_name, bounty) "
                              "VALUES ('Tern', 2000)") != 0)
  {
    mysql_close(connection);
    CuFail(tc, "could not create the isolated prize fixture");
    return;
  }
  saved_conn = conn;
  saved_mysql_available = mysql_available;
  conn = connection;
  mysql_available = TRUE;

  /* Corr's frigate sinks Tern's with Tern aboard: 5,500 gold of salvage,
   * 1,000 on her 400 renown, and Tern's 2,000 gold bounty, all Corr's. */
  target = rewards_berth_begin(tc, &berth, VESSEL_WARSHIP);
  target->renown = 400;
  victor = rewards_hull(REWARDS_VICTOR, VESSEL_WARSHIP, "the Gull", "Corr", 5.0);
  CuAssertIntEquals(tc, 285, vessel_settle_sinking(target, victor));
  rewards_query_value(tc, connection,
                      "SELECT amount FROM vessel_insurance_claims "
                      "WHERE owner = 'Corr' AND ship_id = 491 AND status = 'pending'",
                      value, sizeof(value));
  CuAssertStrEquals(tc, "8500", value);
  rewards_query_value(tc, connection,
                      "SELECT COUNT(*) FROM player_mail WHERE receiver = 'Corr' "
                      "AND subject = 'Prize money for the Gull'",
                      value, sizeof(value));
  CuAssertStrEquals(tc, "1", value);
  CuAssertIntEquals(tc, 0, vessel_get_bounty("Tern"));

  /* The same settlement saved the renown that moved and consumed her last
   * attacker: restored mid-sinking, she goes down with no victor and pays
   * nothing twice. */
  rewards_query_value(tc, connection,
                      "SELECT GROUP_CONCAT(ship_id, ':', renown, ':', last_attacker "
                      "ORDER BY ship_id) FROM ship_runtime_state",
                      value, sizeof(value));
  CuAssertStrEquals(tc, "490:115:0,491:285:0", value);

  /* Ashore, her owner's bounty stands. */
  CuAssertIntEquals(tc, 0, mysql_query(connection, "UPDATE vessel_bounties SET bounty = 2000"));
  IN_ROOM(&berth.captain) = NOWHERE;
  vessel_settle_sinking(target, victor);
  CuAssertIntEquals(tc, 2000, vessel_get_bounty("Tern"));

  /* A settlement that cannot record its claim pays nothing: the bounty is
   * not collected and no renown moves. */
  IN_ROOM(&berth.captain) = 0;
  target->renown = 400;
  CuAssertIntEquals(tc, 0,
                    mysql_query(connection, "UPDATE ship_runtime_state SET renown = 400, "
                                            "last_attacker = 491 WHERE ship_id = 490"));
  CuAssertIntEquals(tc, 0,
                    mysql_query(connection, "ALTER TABLE vessel_insurance_claims "
                                            "CHANGE amount gold INT NOT NULL"));
  CuAssertIntEquals(tc, 0, vessel_settle_sinking(target, victor));
  CuAssertIntEquals(tc, 2000, vessel_get_bounty("Tern"));
  CuAssertIntEquals(tc, 400, target->renown);
  CuAssertIntEquals(tc, 570, victor->renown);
  rewards_query_value(tc, connection,
                      "SELECT GROUP_CONCAT(ship_id, ':', renown, ':', last_attacker "
                      "ORDER BY ship_id) FROM ship_runtime_state",
                      value, sizeof(value));
  CuAssertStrEquals(tc, "490:400:491,491:570:0", value);

  rewards_berth_end(&berth);
  conn = saved_conn;
  mysql_available = saved_mysql_available;
  mysql_close(connection);
}

void Test_vessel_renown_gates_the_hiring_hall(CuTest *tc)
{
  struct rewards_berth berth;
  struct greyhawk_ship_data *ship;
  const char *output;

  ship = rewards_berth_begin(tc, &berth, VESSEL_SHIP);
  ship->renown = 699;
  output = rewards_berth_command(&berth, do_shiphire, "gunner able");
  CuAssertTrue(tc, strstr(output, "No able gunner will sign on with a hull of less than 700 "
                                  "renown, and the Tern has 699.") != NULL);
  CuAssertIntEquals(tc, CREW_TIER_NONE, ship->crew_tier[CREW_GUNNER]);

  ship->renown = 700;
  output = rewards_berth_command(&berth, do_shiphire, "gunner able");
  CuAssertTrue(tc, strstr(output, "for 8000 gold") != NULL);
  CuAssertIntEquals(tc, CREW_TIER_ABLE, ship->crew_tier[CREW_GUNNER]);
  output = rewards_berth_command(&berth, do_shiphire, "sailmaster veteran");
  CuAssertTrue(tc, strstr(output, "less than 1350 renown") != NULL);

  rewards_berth_end(&berth);
}

void Test_vessel_damage_control_spares_the_owners_hull(CuTest *tc)
{
  struct rewards_berth berth;
  struct greyhawk_ship_data *ship;

  /* Five ranks spare 24% of each blow while Tern is aboard his ship. */
  ship = rewards_berth_begin(tc, &berth, VESSEL_SHIP);
  SET_FEAT(&berth.captain, FEAT_SHIP_DAMAGE_CONTROL, 5);
  CuAssertIntEquals(tc, 76, vessel_damage_hull(NULL, ship, 100, GREYHAWK_FORE, FALSE));
  CuAssertIntEquals(tc, 38, vessel_damage_sail(NULL, ship, 50));
  CuAssertIntEquals(tc, 1, vessel_damage_sail(NULL, ship, 1));

  /* Ashore, or without the feat, he spares her nothing. */
  ship->mainsail = ship->maxmainsail;
  IN_ROOM(&berth.captain) = NOWHERE;
  CuAssertIntEquals(tc, 50, vessel_damage_sail(NULL, ship, 50));
  IN_ROOM(&berth.captain) = 0;
  SET_FEAT(&berth.captain, FEAT_SHIP_DAMAGE_CONTROL, 0);
  CuAssertIntEquals(tc, 50, vessel_damage_sail(NULL, ship, 50));

  rewards_berth_end(&berth);
}

void Test_vessel_cargo_sales_carry_their_modifiers(CuTest *tc)
{
  struct rewards_berth berth;
  struct greyhawk_ship_data *ship;

  ship = rewards_berth_begin(tc, &berth, VESSEL_SHIP);
  CuAssertDblEquals(tc, 1.0, vessel_cargo_sale_factor(&berth.captain, ship), 0.0001);
  SET_FEAT(&berth.captain, FEAT_SEADOG, 1);
  CuAssertDblEquals(tc, 1.1, vessel_cargo_sale_factor(&berth.captain, ship), 0.0001);
  ship->slot[0].type = VESSEL_SLOT_EQUIPMENT;
  ship->slot[0].item = VESSEL_EQUIPMENT_COLORS;
  CuAssertDblEquals(tc, 0.99, vessel_cargo_sale_factor(&berth.captain, ship), 0.0001);
  ship->vessel_type = VESSEL_WARSHIP;
  CuAssertDblEquals(tc, 0.594, vessel_cargo_sale_factor(&berth.captain, ship), 0.0001);

  rewards_berth_end(&berth);
}

void Test_vessel_customs_chance_follows_duris(CuTest *tc)
{
  /* 35 + units / 2, less sqrt(renown) / 5, raised as the hold empties. */
  CuAssertIntEquals(tc, 40, vessel_customs_chance(10, 0, 1.0));
  CuAssertIntEquals(tc, 70, vessel_customs_chance(10, 0, 0.5));
  CuAssertIntEquals(tc, 20, vessel_customs_chance(10, 10000, 1.0));
  CuAssertIntEquals(tc, 100, vessel_customs_chance(140, 0, 1.0));
  CuAssertIntEquals(tc, 100, vessel_customs_chance(10, 0, 0.0));
  CuAssertIntEquals(tc, 5, vessel_customs_chance(0, 1000000, 1.0));
}

void Test_vessel_freight_bond_pays_for_the_goods(CuTest *tc)
{
  struct rewards_berth berth;
  struct greyhawk_ship_data *ship;
  const char *output;
  MYSQL *saved_conn;
  MYSQL *connection;
  bool saved_mysql_available;
  char salt[16];
  char query[512];

  if (!rewards_database_enabled())
  {
    return;
  }
  connection = rewards_open_test_database();
  if (connection == NULL)
  {
    CuFail(tc, "could not connect to the explicitly configured test database");
    return;
  }
  if (mysql_query(connection, "CREATE TEMPORARY TABLE trade_commodities ("
                              "commodity_id INT AUTO_INCREMENT PRIMARY KEY, "
                              "name VARCHAR(63) NOT NULL UNIQUE, "
                              "base_price INT NOT NULL DEFAULT 10, "
                              "unit_weight INT NOT NULL DEFAULT 10) ENGINE=InnoDB") != 0 ||
      mysql_query(connection, "CREATE TEMPORARY TABLE port_commodities ("
                              "port_vnum INT NOT NULL, commodity_id INT NOT NULL, "
                              "supply INT NOT NULL DEFAULT 100, "
                              "PRIMARY KEY (port_vnum, commodity_id)) ENGINE=InnoDB") != 0 ||
      mysql_query(connection, "CREATE TEMPORARY TABLE ship_cargo_manifest ("
                              "ship_id INT NOT NULL, cargo_room INT NOT NULL, "
                              "item_vnum INT NOT NULL, item_name VARCHAR(128) NOT NULL, "
                              "item_count INT NOT NULL, item_weight INT NOT NULL) "
                              "ENGINE=InnoDB") != 0 ||
      mysql_query(connection, "CREATE TEMPORARY TABLE vessel_bounties ("
                              "player_name VARCHAR(64) PRIMARY KEY, "
                              "bounty INT NOT NULL DEFAULT 0, "
                              "marque_until INT NOT NULL DEFAULT 0, "
                              "last_offense_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP) "
                              "ENGINE=InnoDB") != 0 ||
      mysql_query(connection, "CREATE TEMPORARY TABLE freight_contracts ("
                              "contract_id INT AUTO_INCREMENT PRIMARY KEY, "
                              "origin_vnum INT NOT NULL, destination_vnum INT NOT NULL, "
                              "commodity_id INT NOT NULL, quantity INT NOT NULL, "
                              "payout INT NOT NULL, status INT NOT NULL DEFAULT 0, "
                              "taken_by VARCHAR(64) NOT NULL DEFAULT '', "
                              "offered_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP) ENGINE=InnoDB") != 0)
  {
    mysql_close(connection);
    CuFail(tc, "could not create the isolated freight fixture");
    return;
  }
  saved_conn = conn;
  saved_mysql_available = mysql_available;
  conn = connection;
  mysql_available = TRUE;
  vessel_trade_ensure_schema();
  rewards_query_value(tc, connection,
                      "SELECT commodity_id FROM trade_commodities WHERE name = 'salt'", salt,
                      sizeof(salt));
  snprintf(
      query, sizeof(query),
      "INSERT INTO freight_contracts (contract_id, origin_vnum, destination_vnum, commodity_id, "
      "quantity, payout, status) VALUES (1, 100, 101, %s, 10, 300, 0)",
      salt);
  CuAssertIntEquals(tc, 0, mysql_query(connection, query));

  ship = rewards_berth_begin(tc, &berth, VESSEL_SHIP);

  /* Salt is worth 14 a unit: the board shows the bond beside the payout. */
  output = rewards_berth_command(&berth, do_contracts, "");
  CuAssertTrue(tc, strstr(output, "1      salt               10     140     300") != NULL);

  /* Too little gold for the bond, and nothing is loaded. */
  GET_GOLD(&berth.captain) = 100;
  output = rewards_berth_command(&berth, do_contractaccept, "1");
  CuAssertTrue(tc,
               strstr(output, "The shipper asks a 140-gold bond for that freight; you have 100.") !=
                   NULL);
  CuAssertIntEquals(tc, 0, ship->cargo[0].quantity);

  /* Taking the job posts the bond; abandoning it keeps the goods the bond
   * bought, so taking and dropping the job again gains nothing. */
  GET_GOLD(&berth.captain) = 1000;
  output = rewards_berth_command(&berth, do_contractaccept, "1");
  CuAssertTrue(tc, strstr(output, "you post a 140-gold bond, 10 units are loaded") != NULL);
  CuAssertIntEquals(tc, 860, GET_GOLD(&berth.captain));
  output = rewards_berth_command(&berth, do_contractabandon, "1");
  CuAssertTrue(tc, strstr(output, "The freight your bond paid for remains in your hold.") != NULL);
  CuAssertIntEquals(tc, 10, ship->cargo[0].quantity);
  output = rewards_berth_command(&berth, do_contractaccept, "1");
  CuAssertIntEquals(tc, 720, GET_GOLD(&berth.captain));
  CuAssertIntEquals(tc, 20, ship->cargo[0].quantity);

  rewards_berth_end(&berth);
  conn = saved_conn;
  mysql_available = saved_mysql_available;
  mysql_close(connection);
}

void Test_vessel_contraband_is_sold_where_stocked_and_seized_elsewhere(CuTest *tc)
{
  struct rewards_berth berth;
  struct greyhawk_ship_data *ship;
  const char *output;
  MYSQL *saved_conn;
  MYSQL *connection;
  bool saved_mysql_available;
  char tomes[16];
  char poisons[16];
  char query[256];
  char value[64];

  if (!rewards_database_enabled())
  {
    return;
  }
  connection = rewards_open_test_database();
  if (connection == NULL)
  {
    CuFail(tc, "could not connect to the explicitly configured test database");
    return;
  }
  if (mysql_query(connection, "CREATE TEMPORARY TABLE trade_commodities ("
                              "commodity_id INT AUTO_INCREMENT PRIMARY KEY, "
                              "name VARCHAR(63) NOT NULL UNIQUE, "
                              "base_price INT NOT NULL DEFAULT 10, "
                              "unit_weight INT NOT NULL DEFAULT 10) ENGINE=InnoDB") != 0 ||
      mysql_query(connection, "CREATE TEMPORARY TABLE port_commodities ("
                              "port_vnum INT NOT NULL, commodity_id INT NOT NULL, "
                              "supply INT NOT NULL DEFAULT 100, "
                              "PRIMARY KEY (port_vnum, commodity_id)) ENGINE=InnoDB") != 0 ||
      mysql_query(connection, "CREATE TEMPORARY TABLE ship_cargo_manifest ("
                              "ship_id INT NOT NULL, cargo_room INT NOT NULL, "
                              "item_vnum INT NOT NULL, item_name VARCHAR(128) NOT NULL, "
                              "item_count INT NOT NULL, item_weight INT NOT NULL) "
                              "ENGINE=InnoDB") != 0 ||
      mysql_query(connection, "CREATE TEMPORARY TABLE vessel_bounties ("
                              "player_name VARCHAR(64) PRIMARY KEY, "
                              "bounty INT NOT NULL DEFAULT 0, "
                              "marque_until INT NOT NULL DEFAULT 0, "
                              "last_offense_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP) "
                              "ENGINE=InnoDB") != 0)
  {
    mysql_close(connection);
    CuFail(tc, "could not create the isolated contraband fixture");
    return;
  }
  saved_conn = conn;
  saved_mysql_available = mysql_available;
  conn = connection;
  mysql_available = TRUE;

  /* The trade schema adds the contraband column and seeds the lawful goods;
   * two contraband goods join them. */
  vessel_trade_ensure_schema();
  CuAssertIntEquals(tc, 0,
                    mysql_query(connection, "INSERT INTO trade_commodities "
                                            "(name, base_price, unit_weight, contraband_renown) "
                                            "VALUES ('forbidden tomes', 190, 4, 150), "
                                            "('rare poisons', 210, 1, 200)"));
  vessel_trade_ensure_schema();
  rewards_query_value(tc, connection,
                      "SELECT commodity_id FROM trade_commodities WHERE name = 'forbidden tomes'",
                      tomes, sizeof(tomes));
  rewards_query_value(tc, connection,
                      "SELECT commodity_id FROM trade_commodities WHERE name = 'rare poisons'",
                      poisons, sizeof(poisons));

  /* A port that does not stock it will not sell it, and pays 85% of its
   * scarce price for it (190 raised to 292). */
  ship = rewards_berth_begin(tc, &berth, VESSEL_SHIP);
  output = rewards_berth_command(&berth, do_market, "");
  CuAssertTrue(tc,
               strstr(output, "forbidden tomes        4      -    248  none (contraband)") != NULL);
  output = rewards_berth_command(&berth, do_cargobuy, "forbidden 2");
  CuAssertTrue(tc, strstr(output, "Nobody here will sell you forbidden tomes.") != NULL);

  /* Where it is stocked it goes to a hull of its renown or with an able
   * sailmaster and quartermaster, never to a warship or a saint. */
  snprintf(query, sizeof(query),
           "INSERT INTO port_commodities (port_vnum, commodity_id, supply) VALUES (100, %s, 100)",
           tomes);
  CuAssertIntEquals(tc, 0, mysql_query(connection, query));
  output = rewards_berth_command(&berth, do_market, "");
  CuAssertTrue(tc, strstr(output, "steady (contraband)") != NULL);
  output = rewards_berth_command(&berth, do_cargobuy, "forbidden 2");
  CuAssertTrue(tc, strstr(output, "only to a hull of 150 renown") != NULL);
  ship->crew_tier[CREW_SAILMASTER] = CREW_TIER_ABLE;
  ship->crew_tier[CREW_QUARTERMASTER] = CREW_TIER_ABLE;
  GET_ALIGNMENT(&berth.captain) = VESSEL_CONTRABAND_ALIGNMENT;
  output = rewards_berth_command(&berth, do_cargobuy, "forbidden 2");
  CuAssertTrue(tc, strstr(output, "as upright as you") != NULL);
  GET_ALIGNMENT(&berth.captain) = 0;
  ship->vessel_type = VESSEL_WARSHIP;
  output = rewards_berth_command(&berth, do_cargobuy, "forbidden 2");
  CuAssertTrue(tc, strstr(output, "into a warship") != NULL);
  ship->vessel_type = VESSEL_SHIP;
  output = rewards_berth_command(&berth, do_cargobuy, "forbidden 2");
  CuAssertTrue(tc, strstr(output, "You load 2 units of forbidden tomes") != NULL);

  /* A sale where it is not stocked leaves no stock behind. */
  CuAssertIntEquals(tc, 0, mysql_query(connection, "DELETE FROM port_commodities"));
  output = rewards_berth_command(&berth, do_cargosell, "forbidden 1");
  CuAssertTrue(tc, strstr(output, "You sell 1 units of forbidden tomes for 248 gold") != NULL);
  snprintf(query, sizeof(query), "SELECT COUNT(*) FROM port_commodities WHERE commodity_id = %s",
           tomes);
  rewards_query_value(tc, connection, query, value, sizeof(value));
  CuAssertStrEquals(tc, "0", value);

  /* Every unit fetches that scarce price, so a hold sold whole fetches what
   * it would a unit at a time. */
  ship->cargo[0].quantity = 100;
  output = rewards_berth_command(&berth, do_cargosell, "forbidden all");
  CuAssertTrue(tc, strstr(output, "You sell 100 units of forbidden tomes for 24800 gold") != NULL);

  /* Customs take every one of 140 forbidden tomes the port does not stock,
   * and leave the poisons it does; when its stock cannot be looked up, they
   * take nothing. */
  ship->cargo[0].commodity_id = (int)strtol(tomes, NULL, 10);
  ship->cargo[0].quantity = 140;
  ship->cargo[1].commodity_id = (int)strtol(poisons, NULL, 10);
  ship->cargo[1].quantity = 3;
  snprintf(query, sizeof(query),
           "INSERT INTO port_commodities (port_vnum, commodity_id, supply) VALUES (100, %s, 100)",
           poisons);
  CuAssertIntEquals(tc, 0, mysql_query(connection, query));
  CuAssertIntEquals(tc, 0,
                    mysql_query(connection, "ALTER TABLE port_commodities "
                                            "CHANGE port_vnum port INT NOT NULL"));
  vessel_customs_inspection(ship, 0);
  CuAssertIntEquals(tc, 140, ship->cargo[0].quantity);
  CuAssertIntEquals(tc, 0,
                    mysql_query(connection, "ALTER TABLE port_commodities "
                                            "CHANGE port port_vnum INT NOT NULL"));
  vessel_customs_inspection(ship, 0);
  CuAssertIntEquals(tc, 0, ship->cargo[0].quantity);
  CuAssertIntEquals(tc, 0, ship->cargo[0].commodity_id);
  CuAssertIntEquals(tc, 3, ship->cargo[1].quantity);

  rewards_berth_end(&berth);
  conn = saved_conn;
  mysql_available = saved_mysql_available;
  mysql_close(connection);
}
