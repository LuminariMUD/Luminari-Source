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
#include <sys/stat.h>
#include <unistd.h>

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

/* The tables a cargo trade reads and writes, shadowing any real ones. */
static bool rewards_trade_tables(MYSQL *connection)
{
  return mysql_query(connection, "CREATE TEMPORARY TABLE trade_commodities ("
                                 "commodity_id INT AUTO_INCREMENT PRIMARY KEY, "
                                 "name VARCHAR(63) NOT NULL UNIQUE, "
                                 "base_price INT NOT NULL DEFAULT 10, "
                                 "unit_weight INT NOT NULL DEFAULT 10) ENGINE=InnoDB") == 0 &&
         mysql_query(connection, "CREATE TEMPORARY TABLE port_commodities ("
                                 "port_vnum INT NOT NULL, commodity_id INT NOT NULL, "
                                 "supply INT NOT NULL DEFAULT 100, "
                                 "PRIMARY KEY (port_vnum, commodity_id)) ENGINE=InnoDB") == 0 &&
         mysql_query(connection, "CREATE TEMPORARY TABLE ship_cargo_manifest ("
                                 "ship_id INT NOT NULL, cargo_room INT NOT NULL, "
                                 "item_vnum INT NOT NULL, item_name VARCHAR(128) NOT NULL, "
                                 "item_count INT NOT NULL, item_weight INT NOT NULL) "
                                 "ENGINE=InnoDB") == 0 &&
         mysql_query(connection, "CREATE TEMPORARY TABLE vessel_bounties ("
                                 "player_name VARCHAR(64) PRIMARY KEY, "
                                 "bounty INT NOT NULL DEFAULT 0, "
                                 "marque_until INT NOT NULL DEFAULT 0, "
                                 "last_offense_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP) "
                                 "ENGINE=InnoDB") == 0;
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
  /* 35 - 160 / 5 = 3 is under the floor too: always a small chance. */
  CuAssertIntEquals(tc, 5, vessel_customs_chance(0, 25600, 1.0));
}

/* A scratch player directory and index, so the captain's saves succeed. */
struct rewards_pfiles
{
  char scratch[64];
  char home[PATH_MAX];
  struct player_index_element index[1];
  struct player_index_element *saved_table;
  int saved_top;
};

static void rewards_pfiles_begin(CuTest *tc, struct rewards_pfiles *pfiles, struct char_data *ch)
{
  memset(pfiles, 0, sizeof(*pfiles));
  strlcpy(pfiles->scratch, "/tmp/luminari-freight-bond-XXXXXX", sizeof(pfiles->scratch));
  CuAssertPtrNotNull(tc, getcwd(pfiles->home, sizeof(pfiles->home)));
  CuAssertPtrNotNull(tc, mkdtemp(pfiles->scratch));
  CuAssertIntEquals(tc, 0, chdir(pfiles->scratch));
  CuAssertIntEquals(tc, 0, mkdir("plrfiles", 0700));
  CuAssertIntEquals(tc, 0, mkdir("plrfiles/P-T", 0700));
  pfiles->index[0].name = GET_NAME(ch);
  pfiles->index[0].id = 4249;
  pfiles->saved_table = player_table;
  pfiles->saved_top = top_of_p_table;
  player_table = pfiles->index;
  top_of_p_table = 0;
  GET_PFILEPOS(ch) = 0;
}

static void rewards_pfiles_end(CuTest *tc, struct rewards_pfiles *pfiles, struct char_data *ch)
{
  char filename[MAX_FILEPATH];

  if (get_filename(filename, sizeof(filename), PLR_FILE, GET_NAME(ch)))
  {
    unlink(filename);
  }
  unlink("plrfiles/index");
  rmdir("plrfiles/P-T");
  rmdir("plrfiles");
  player_table = pfiles->saved_table;
  top_of_p_table = pfiles->saved_top;
  CuAssertIntEquals(tc, 0, chdir(pfiles->home));
  rmdir(pfiles->scratch);
}

void Test_vessel_freight_bond_pays_for_the_goods(CuTest *tc)
{
  struct rewards_berth berth;
  struct rewards_pfiles pfiles;
  struct greyhawk_ship_data *ship;
  const char *output;
  MYSQL *saved_conn;
  MYSQL *connection;
  bool saved_mysql_available;
  char salt[16];
  char query[512];
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
  if (!rewards_trade_tables(connection) ||
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

  /* The freight cannot be written: the job stays open and no gold is taken. */
  GET_GOLD(&berth.captain) = 1000;
  CuAssertIntEquals(tc, 0,
                    mysql_query(connection, "ALTER TABLE ship_cargo_manifest ADD CONSTRAINT "
                                            "freight_refused CHECK (item_count < 0)"));
  output = rewards_berth_command(&berth, do_contractaccept, "1");
  CuAssertTrue(tc, strstr(output, "cannot record that freight; no gold was taken") != NULL);
  CuAssertIntEquals(tc, 1000, GET_GOLD(&berth.captain));
  CuAssertIntEquals(tc, 0, ship->cargo[0].quantity);
  rewards_query_value(tc, connection, "SELECT status FROM freight_contracts WHERE contract_id = 1",
                      value, sizeof(value));
  CuAssertStrEquals(tc, "0", value);
  CuAssertIntEquals(
      tc, 0,
      mysql_query(connection, "ALTER TABLE ship_cargo_manifest DROP CONSTRAINT freight_refused"));

  /* The captain cannot be saved: the bond is not taken, so neither is the
   * job, and the freight comes back out of the hold. */
  GET_PFILEPOS(&berth.captain) = -1;
  output = rewards_berth_command(&berth, do_contractaccept, "1");
  CuAssertTrue(tc, strstr(output, "Your bond could not be recorded; no gold was taken.") != NULL);
  CuAssertIntEquals(tc, 1000, GET_GOLD(&berth.captain));
  CuAssertIntEquals(tc, 0, ship->cargo[0].quantity);
  rewards_query_value(tc, connection, "SELECT status FROM freight_contracts WHERE contract_id = 1",
                      value, sizeof(value));
  CuAssertStrEquals(tc, "0", value);
  rewards_query_value(tc, connection, "SELECT COUNT(*) FROM ship_cargo_manifest", value,
                      sizeof(value));
  CuAssertStrEquals(tc, "0", value);

  /* Taking the job posts the bond, saved with the captain; abandoning it
   * keeps the goods the bond bought, so taking and dropping the job again
   * gains nothing. */
  rewards_pfiles_begin(tc, &pfiles, &berth.captain);
  output = rewards_berth_command(&berth, do_contractaccept, "1");
  CuAssertTrue(tc, strstr(output, "you post a 140-gold bond, 10 units are loaded") != NULL);
  CuAssertIntEquals(tc, 860, GET_GOLD(&berth.captain));
  rewards_query_value(tc, connection,
                      "SELECT item_count FROM ship_cargo_manifest WHERE cargo_room = 0", value,
                      sizeof(value));
  CuAssertStrEquals(tc, "10", value);
  output = rewards_berth_command(&berth, do_contractabandon, "1");
  CuAssertTrue(tc, strstr(output, "The freight your bond paid for remains in your hold.") != NULL);
  CuAssertIntEquals(tc, 10, ship->cargo[0].quantity);
  output = rewards_berth_command(&berth, do_contractaccept, "1");
  CuAssertTrue(tc, strstr(output, "you post a 140-gold bond, 10 units are loaded") != NULL);
  CuAssertIntEquals(tc, 720, GET_GOLD(&berth.captain));
  CuAssertIntEquals(tc, 20, ship->cargo[0].quantity);
  rewards_pfiles_end(tc, &pfiles, &berth.captain);

  rewards_berth_end(&berth);
  conn = saved_conn;
  mysql_available = saved_mysql_available;
  mysql_close(connection);
}

/* The captain's gold, the units in the hold's first lot, and what the
 * manifest and the port's stock record. */
static void rewards_assert_trade(CuTest *tc, MYSQL *connection, struct rewards_berth *berth,
                                 int gold, int units, int manifest, int supply)
{
  char value[64];

  CuAssertIntEquals(tc, gold, GET_GOLD(&berth->captain));
  CuAssertIntEquals(tc, units, greyhawk_ships[REWARDS_TARGET].cargo[0].quantity);
  rewards_query_value(tc, connection,
                      "SELECT COALESCE(SUM(item_count), 0) FROM ship_cargo_manifest", value,
                      sizeof(value));
  CuAssertIntEquals(tc, manifest, (int)strtol(value, NULL, 10));
  rewards_query_value(tc, connection, "SELECT supply FROM port_commodities WHERE port_vnum = 100",
                      value, sizeof(value));
  CuAssertIntEquals(tc, supply, (int)strtol(value, NULL, 10));
}

/* Make the port's stock refuse the value a trade's undo would write back,
 * though the stock holds that value now. */
static void rewards_refuse_undo(CuTest *tc, MYSQL *connection, const char *check)
{
  char query[256];

  snprintf(query, sizeof(query),
           "ALTER TABLE port_commodities ADD CONSTRAINT undo_refused CHECK (%s)", check);
  CuAssertIntEquals(tc, 0, mysql_query(connection, "SET SESSION check_constraint_checks = 0"));
  CuAssertIntEquals(tc, 0, mysql_query(connection, query));
  CuAssertIntEquals(tc, 0, mysql_query(connection, "SET SESSION check_constraint_checks = 1"));
}

/* A cargo trade and its gold are recorded together: a refused manifest
 * write or a failed save leaves the gold, the hold, the manifest and the
 * port's stock as they were, so no crash or failed write gives free cargo
 * or cargo sold twice. If even the undo cannot be written, the trade stands
 * as recorded. */
void Test_vessel_cargo_trades_record_the_gold_with_the_goods(CuTest *tc)
{
  struct rewards_berth berth;
  struct rewards_pfiles pfiles;
  struct greyhawk_ship_data *ship;
  const char *output;
  MYSQL *saved_conn;
  MYSQL *connection;
  bool saved_mysql_available;
  char salt[16];
  char query[256];
  char expected[128];
  long long cost;
  long long revenue;
  int salt_id;
  int base_price;
  int gold;

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
  if (!rewards_trade_tables(connection))
  {
    mysql_close(connection);
    CuFail(tc, "could not create the isolated trade fixture");
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
  salt_id = (int)strtol(salt, NULL, 10);
  base_price = vessel_commodity_base_price(salt_id);
  snprintf(query, sizeof(query),
           "INSERT INTO port_commodities (port_vnum, commodity_id, supply) VALUES (100, %d, 100)",
           salt_id);
  CuAssertIntEquals(tc, 0, mysql_query(connection, query));

  ship = rewards_berth_begin(tc, &berth, VESSEL_SHIP);
  GET_GOLD(&berth.captain) = 1000;

  /* The cargo cannot be written: nothing is loaded and no gold is taken. */
  CuAssertIntEquals(tc, 0,
                    mysql_query(connection, "ALTER TABLE ship_cargo_manifest ADD CONSTRAINT "
                                            "trade_refused CHECK (item_count < 0)"));
  output = rewards_berth_command(&berth, do_cargobuy, "salt 10");
  CuAssertTrue(tc, strstr(output, "cannot record that trade; no gold changed hands") != NULL);
  rewards_assert_trade(tc, connection, &berth, 1000, 0, 0, 100);
  CuAssertIntEquals(tc, 0, ship->cargo[0].commodity_id);
  CuAssertIntEquals(
      tc, 0,
      mysql_query(connection, "ALTER TABLE ship_cargo_manifest DROP CONSTRAINT trade_refused"));

  /* The captain cannot be saved: the purchase is undone. */
  GET_PFILEPOS(&berth.captain) = -1;
  output = rewards_berth_command(&berth, do_cargobuy, "salt 10");
  CuAssertTrue(tc,
               strstr(output, "Your gold could not be recorded, so the trade is undone.") != NULL);
  rewards_assert_trade(tc, connection, &berth, 1000, 0, 0, 100);
  CuAssertIntEquals(tc, 0, ship->cargo[0].commodity_id);

  /* Saved, the purchase moves the gold, the hold, the manifest and the
   * port's stock. */
  rewards_pfiles_begin(tc, &pfiles, &berth.captain);
  cost = vessel_trade_buy_cost(base_price, 100, 10);
  gold = 1000 - (int)cost;
  output = rewards_berth_command(&berth, do_cargobuy, "salt 10");
  snprintf(expected, sizeof(expected), "You load 10 units of salt for %lld gold", cost);
  CuAssertTrue(tc, strstr(output, expected) != NULL);
  rewards_assert_trade(tc, connection, &berth, gold, 10, 10, 90);

  /* A sale whose cargo cannot be written keeps the goods and pays nothing:
   * the manifest refuses a lot of fewer than ten units. */
  CuAssertIntEquals(tc, 0,
                    mysql_query(connection, "ALTER TABLE ship_cargo_manifest ADD CONSTRAINT "
                                            "trade_refused CHECK (item_count >= 10)"));
  output = rewards_berth_command(&berth, do_cargosell, "salt 4");
  CuAssertTrue(tc, strstr(output, "cannot record that trade; no gold changed hands") != NULL);
  rewards_assert_trade(tc, connection, &berth, gold, 10, 10, 90);
  CuAssertIntEquals(
      tc, 0,
      mysql_query(connection, "ALTER TABLE ship_cargo_manifest DROP CONSTRAINT trade_refused"));

  /* A sale of the whole hold that cannot be saved puts the goods back. */
  GET_PFILEPOS(&berth.captain) = -1;
  output = rewards_berth_command(&berth, do_cargosell, "salt all");
  CuAssertTrue(tc,
               strstr(output, "Your gold could not be recorded, so the trade is undone.") != NULL);
  rewards_assert_trade(tc, connection, &berth, gold, 10, 10, 90);
  CuAssertIntEquals(tc, salt_id, ship->cargo[0].commodity_id);
  GET_PFILEPOS(&berth.captain) = 0;

  /* Saved, the sale pays and records what remains. */
  revenue = vessel_trade_sell_revenue(base_price, 90, 4);
  output = rewards_berth_command(&berth, do_cargosell, "salt 4");
  snprintf(expected, sizeof(expected), "You sell 4 units of salt for %lld gold", revenue);
  CuAssertTrue(tc, strstr(output, expected) != NULL);
  rewards_assert_trade(tc, connection, &berth, gold + (int)revenue, 6, 6, 94);
  gold += (int)revenue;

  /* The save fails and its undo cannot be written either: the sale stands
   * as recorded, in memory too, for the persistence service to save. */
  rewards_refuse_undo(tc, connection, "supply > 94");
  GET_PFILEPOS(&berth.captain) = -1;
  revenue = vessel_trade_sell_revenue(base_price, 94, 6);
  output = rewards_berth_command(&berth, do_cargosell, "salt all");
  CuAssertTrue(tc, strstr(output, "the trade stands and will be saved shortly") != NULL);
  CuAssertTrue(tc, strstr(output, "undone") == NULL);
  rewards_assert_trade(tc, connection, &berth, gold + (int)revenue, 0, 0, 100);
  CuAssertIntEquals(
      tc, 0, mysql_query(connection, "ALTER TABLE port_commodities DROP CONSTRAINT undo_refused"));
  gold += (int)revenue;

  /* So does a purchase. */
  rewards_refuse_undo(tc, connection, "supply < 100");
  cost = vessel_trade_buy_cost(base_price, 100, 10);
  output = rewards_berth_command(&berth, do_cargobuy, "salt 10");
  CuAssertTrue(tc, strstr(output, "the trade stands and will be saved shortly") != NULL);
  CuAssertTrue(tc, strstr(output, "undone") == NULL);
  rewards_assert_trade(tc, connection, &berth, gold - (int)cost, 10, 10, 90);
  CuAssertIntEquals(
      tc, 0, mysql_query(connection, "ALTER TABLE port_commodities DROP CONSTRAINT undo_refused"));
  rewards_pfiles_end(tc, &pfiles, &berth.captain);

  rewards_berth_end(&berth);
  conn = saved_conn;
  mysql_available = saved_mysql_available;
  mysql_close(connection);
}

/* A trade COMMIT whose reply is lost may have taken effect or not; the
 * trade is written again, so the gold, the hold, the manifest and the port's
 * stock agree whichever way it went. */
void Test_vessel_cargo_trades_settle_a_commit_without_a_reply(CuTest *tc)
{
  struct rewards_berth berth;
  struct rewards_pfiles pfiles;
  struct greyhawk_ship_data *ship;
  const char *output;
  MYSQL *saved_conn;
  MYSQL *connection;
  bool saved_mysql_available;
  char salt[16];
  char query[256];
  long long cost;
  long long revenue;
  int salt_id;
  int base_price;
  int gold;

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
  if (!rewards_trade_tables(connection))
  {
    mysql_close(connection);
    CuFail(tc, "could not create the isolated trade fixture");
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
  salt_id = (int)strtol(salt, NULL, 10);
  base_price = vessel_commodity_base_price(salt_id);
  snprintf(query, sizeof(query),
           "INSERT INTO port_commodities (port_vnum, commodity_id, supply) VALUES (100, %d, 100)",
           salt_id);
  CuAssertIntEquals(tc, 0, mysql_query(connection, query));
  ship = rewards_berth_begin(tc, &berth, VESSEL_SHIP);
  rewards_pfiles_begin(tc, &pfiles, &berth.captain);
  GET_GOLD(&berth.captain) = 1000;

  /* The purchase was committed, though its reply was lost. */
  vessel_trade_lose_commit_reply_for_test(1, TRUE);
  cost = vessel_trade_buy_cost(base_price, 100, 10);
  gold = 1000 - (int)cost;
  output = rewards_berth_command(&berth, do_cargobuy, "salt 10");
  CuAssertTrue(tc, strstr(output, "You load 10 units of salt") != NULL);
  rewards_assert_trade(tc, connection, &berth, gold, 10, 10, 90);

  /* The sale was rolled back, its reply lost too. */
  vessel_trade_lose_commit_reply_for_test(1, FALSE);
  revenue = vessel_trade_sell_revenue(base_price, 90, 4);
  gold += (int)revenue;
  output = rewards_berth_command(&berth, do_cargosell, "salt 4");
  CuAssertTrue(tc, strstr(output, "You sell 4 units of salt") != NULL);
  rewards_assert_trade(tc, connection, &berth, gold, 6, 6, 94);

  /* The captain cannot be saved, and the undo's reply is lost after it was
   * committed: the purchase is undone. */
  GET_PFILEPOS(&berth.captain) = -1;
  vessel_trade_lose_commit_reply_for_test(2, TRUE);
  output = rewards_berth_command(&berth, do_cargobuy, "salt 10");
  CuAssertTrue(tc,
               strstr(output, "Your gold could not be recorded, so the trade is undone.") != NULL);
  rewards_assert_trade(tc, connection, &berth, gold, 6, 6, 94);

  /* And when it was rolled back, so is a sale of the whole hold. */
  vessel_trade_lose_commit_reply_for_test(2, FALSE);
  output = rewards_berth_command(&berth, do_cargosell, "salt all");
  CuAssertTrue(tc,
               strstr(output, "Your gold could not be recorded, so the trade is undone.") != NULL);
  rewards_assert_trade(tc, connection, &berth, gold, 6, 6, 94);
  CuAssertIntEquals(tc, salt_id, ship->cargo[0].commodity_id);
  GET_PFILEPOS(&berth.captain) = 0;
  vessel_trade_lose_commit_reply_for_test(0, FALSE);
  rewards_pfiles_end(tc, &pfiles, &berth.captain);

  rewards_berth_end(&berth);
  conn = saved_conn;
  mysql_available = saved_mysql_available;
  mysql_close(connection);
}

void Test_vessel_contraband_is_sold_where_stocked_and_seized_elsewhere(CuTest *tc)
{
  struct rewards_berth berth;
  struct rewards_pfiles pfiles;
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
  if (!rewards_trade_tables(connection))
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
  rewards_pfiles_begin(tc, &pfiles, &berth.captain);
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
  /* Any word of the name finds the goods. */
  output = rewards_berth_command(&berth, do_cargobuy, "tomes 1");
  CuAssertTrue(tc, strstr(output, "You load 1 units of forbidden tomes") != NULL);
  ship->cargo[0].quantity = 2;

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
  rewards_pfiles_end(tc, &pfiles, &berth.captain);

  rewards_berth_end(&berth);
  conn = saved_conn;
  mysql_available = saved_mysql_available;
  mysql_close(connection);
}
