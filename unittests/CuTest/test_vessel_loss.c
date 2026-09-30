/* Losing and recovering hulls (vessels-ships study S5): automatic insurance,
 * the summons fee and passage, stowed hulls, and the wreck prototype. The
 * wreck rebuild and a summoned hull making port need a booted world; the
 * live loss gate (scripts/vessels/test_vessel_loss_in_game.sh) runs them. */

#include "CuTest.h"

#include "conf.h"
#include "../../src/core/sysdep.h"
#include "../../src/core/structs.h"
#include "../../src/core/utils.h"
#include "../../src/core/comm.h"
#include "../../src/core/db.h"
#include "../../src/core/handler.h"
#include "../../src/core/interpreter.h"
#include "../../src/database/mysql.h"
#include "../../src/net/protocol.h"
#include "../../src/vessels/vessels.h"

#include <stdlib.h>
#include <string.h>

extern struct greyhawk_ship_data greyhawk_ships[GREYHAWK_MAXSHIPS];

/* High fleet slots keep these fixtures clear of the rest of the suite. */
#define LOSS_SHIP 485
#define LOSS_DOCK_VNUM 169980
#define LOSS_SEA_VNUM 169981
#define LOSS_BRIDGE_VNUM 169982

static struct greyhawk_ship_data *loss_ship(enum vessel_class vessel_type, const char *owner)
{
  struct greyhawk_ship_data *ship = &greyhawk_ships[LOSS_SHIP];

  memset(ship, 0, sizeof(*ship));
  ship->active = TRUE;
  ship->shipnum = LOSS_SHIP;
  ship->vessel_type = vessel_type;
  ship->maxspeed = (short int)vessel_class_handling(vessel_type)->speed;
  ship->docked_to_ship = -1;
  strlcpy(ship->name, "the Petrel", sizeof(ship->name));
  strlcpy(ship->owner, owner, sizeof(ship->owner));
  vessel_initialize_condition(ship, vessel_class_condition(vessel_type)->beam_armor);
  return ship;
}

void Test_vessel_insurance_pays_the_class_share_of_the_hull(CuTest *tc)
{
  struct greyhawk_ship_data *ship;

  /* A default warship is worth her class price and pays half of it, 90%
   * when an NPC hull sank her. */
  ship = loss_ship(VESSEL_WARSHIP, "Corr");
  CuAssertIntEquals(tc, 44000, vessel_hull_price(ship));
  CuAssertIntEquals(tc, 22000, vessel_insurance_payout(ship, FALSE));
  CuAssertIntEquals(tc, 39600, vessel_insurance_payout(ship, TRUE));

  /* Merchant classes pay three quarters. */
  ship = loss_ship(VESSEL_SHIP, "Corr");
  CuAssertIntEquals(tc, 6000, vessel_insurance_payout(ship, FALSE));

  /* Nothing for a raft, a hull rebuilt from a wreck, or a hull nobody owns. */
  ship = loss_ship(VESSEL_RAFT, "Corr");
  CuAssertIntEquals(tc, 0, vessel_insurance_payout(ship, TRUE));
  ship = loss_ship(VESSEL_BOAT, "Corr");
  CuAssertIntEquals(tc, 450, vessel_insurance_payout(ship, FALSE));
  ship->wreck_hull = TRUE;
  CuAssertIntEquals(tc, 0, vessel_insurance_payout(ship, TRUE));
  ship = loss_ship(VESSEL_WARSHIP, "");
  CuAssertIntEquals(tc, 0, vessel_insurance_payout(ship, TRUE));

  memset(ship, 0, sizeof(*ship));
}

void Test_vessel_summons_fee_and_passage_follow_duris(CuTest *tc)
{
  struct greyhawk_ship_data *ship;

  /* A frigate: 28 gold, and 70 / (56.7 - 20) mud hours at her full 17. */
  ship = loss_ship(VESSEL_WARSHIP, "Corr");
  CuAssertIntEquals(tc, 28, vessel_summon_fee(ship));
  CuAssertIntEquals(tc, 143, vessel_summon_seconds(ship));

  /* Her guns weigh her down; her cargo does not, as the hold is emptied. */
  vessel_set_weapon(&ship->slot[0], VESSEL_WEAPON_LARGE_BALLISTA, GREYHAWK_FORE);
  vessel_set_weapon(&ship->slot[1], VESSEL_WEAPON_LARGE_BALLISTA, GREYHAWK_PORT);
  vessel_set_weapon(&ship->slot[2], VESSEL_WEAPON_LARGE_BALLISTA, GREYHAWK_STARBOARD);
  CuAssertIntEquals(tc, 160, vessel_summon_seconds(ship));

  /* Holed, she makes no way: 35 mud hours, and twice that from the wreck
   * registry is held to 60. */
  ship->parmor = 0;
  ship->pinternal = 0;
  CuAssertIntEquals(tc, 35 * SECS_PER_MUD_HOUR, vessel_summon_seconds(ship));
  ship->stowed = TRUE;
  CuAssertIntEquals(tc, 60 * SECS_PER_MUD_HOUR, vessel_summon_seconds(ship));

  /* A boat without sails waits 25 mud hours, 50 from the registry; with
   * them, half an hour, one from the registry. */
  ship = loss_ship(VESSEL_BOAT, "Corr");
  CuAssertIntEquals(tc, 2, vessel_summon_fee(ship));
  ship->stowed = TRUE;
  CuAssertIntEquals(tc, 75, vessel_summon_seconds(ship));
  ship->mainsail = 0;
  CuAssertIntEquals(tc, 50 * SECS_PER_MUD_HOUR, vessel_summon_seconds(ship));

  memset(ship, 0, sizeof(*ship));
}

struct loss_harbor
{
  struct room_data rooms[3]; /* the shipyard dock, the sea, the bridge */
  struct char_data captain;
  struct player_special_data captain_specials;
  struct char_data passenger;
  struct player_special_data passenger_specials;
  struct descriptor_data descriptor;
  char output[8192];
  struct room_data *saved_world;
  room_rnum saved_top_of_world;
};

/* The captain stands on the shipyard dock; her frigate lies at sea with a
 * passenger on the bridge and cargo in the hold. */
static struct greyhawk_ship_data *loss_harbor_begin(CuTest *tc, struct loss_harbor *harbor)
{
  struct greyhawk_ship_data *ship;
  struct obj_data *hull;

  memset(harbor, 0, sizeof(*harbor));
  harbor->saved_world = world;
  harbor->saved_top_of_world = top_of_world;
  harbor->rooms[0].number = LOSS_DOCK_VNUM;
  SET_BIT_AR(harbor->rooms[0].room_flags, ROOM_DOCKABLE);
  harbor->rooms[0].coords[0] = 40;
  harbor->rooms[0].coords[1] = 41;
  harbor->rooms[1].number = LOSS_SEA_VNUM;
  harbor->rooms[1].sector_type = SECT_OCEAN;
  harbor->rooms[2].number = LOSS_BRIDGE_VNUM;
  harbor->rooms[0].zone = NOWHERE;
  harbor->rooms[1].zone = NOWHERE;
  harbor->rooms[2].zone = NOWHERE;
  world = harbor->rooms;
  top_of_world = 2;

  harbor->captain.player_specials = &harbor->captain_specials;
  harbor->captain.player.name = CuMutableString("Corr");
  GET_LEVEL(&harbor->captain) = 20;
  GET_POS(&harbor->captain) = POS_STANDING;
  GET_GOLD(&harbor->captain) = 1000;
  IN_ROOM(&harbor->captain) = 0;
  harbor->rooms[0].people = &harbor->captain;
  harbor->captain.desc = &harbor->descriptor;
  harbor->descriptor.character = &harbor->captain;
  harbor->descriptor.output = harbor->output;
  harbor->descriptor.bufspace = sizeof(harbor->output) - 1;
  harbor->descriptor.pProtocol = ProtocolCreate();
  CuAssertPtrNotNull(tc, harbor->descriptor.pProtocol);

  harbor->passenger.player_specials = &harbor->passenger_specials;
  harbor->passenger.player.name = CuMutableString("Mira");
  GET_POS(&harbor->passenger) = POS_STANDING;
  IN_ROOM(&harbor->passenger) = 2;
  harbor->rooms[2].people = &harbor->passenger;

  ship = loss_ship(VESSEL_WARSHIP, "Corr");
  ship->x = 10.0;
  ship->y = 12.0;
  ship->location = LOSS_SEA_VNUM;
  ship->num_rooms = 1;
  ship->room_vnums[0] = LOSS_BRIDGE_VNUM;
  ship->bridge_room = LOSS_BRIDGE_VNUM;
  ship->cargo[0].commodity_id = 3;
  ship->cargo[0].quantity = 40;
  ship->num_cargo_lots = 1;
  harbor->rooms[2].ship = ship;
  hull = create_obj();
  hull->name = strdup("petrel hull");
  obj_to_room(hull, 1);
  ship->shipobj = hull;
  return ship;
}

static const char *loss_harbor_command(struct loss_harbor *harbor, const char *argument)
{
  memset(harbor->output, 0, sizeof(harbor->output));
  harbor->descriptor.bufptr = 0;
  harbor->descriptor.bufspace = sizeof(harbor->output) - 1;
  do_shipsummon(&harbor->captain, argument, 0, 0);
  return harbor->output;
}

static void loss_harbor_end(struct loss_harbor *harbor)
{
  struct greyhawk_ship_data *ship = &greyhawk_ships[LOSS_SHIP];

  if (ship->shipobj != NULL)
  {
    extract_obj(ship->shipobj);
  }
  memset(ship, 0, sizeof(*ship));
  ProtocolDestroy(harbor->descriptor.pProtocol);
  world = harbor->saved_world;
  top_of_world = harbor->saved_top_of_world;
}

void Test_vessel_summons_sends_a_hull_out_of_the_world(CuTest *tc)
{
  struct loss_harbor harbor;
  struct greyhawk_ship_data *ship;
  const char *output;
  time_t ordered;

  ship = loss_harbor_begin(tc, &harbor);

  /* The list names each hull with its fee and passage. */
  output = loss_harbor_command(&harbor, "");
  CuAssertTrue(tc, strstr(output, " 1. the Petrel (Warship): at sea; 28 gold, about 2 minutes.") !=
                       NULL);

  /* Summoned, she leaves at once: the passenger goes over the side, the hold
   * is emptied, and she is out of the world until her passage is done. */
  ordered = time(0);
  output = loss_harbor_command(&harbor, "1");
  CuAssertTrue(tc, strstr(output, "You pay 28 gold.") != NULL);
  CuAssertIntEquals(tc, 972, GET_GOLD(&harbor.captain));
  CuAssertIntEquals(tc, 1, IN_ROOM(&harbor.passenger));
  CuAssertIntEquals(tc, 0, ship->cargo[0].quantity);
  CuAssertTrue(tc, ship->stowed);
  CuAssertTrue(tc, !ship->active);
  CuAssertTrue(tc, !is_valid_ship(ship));
  CuAssertPtrEquals(tc, NULL, ship->shipobj);
  CuAssertIntEquals(tc, LOSS_DOCK_VNUM, ship->location);
  CuAssertDblEquals(tc, 40.0, ship->x, 0.0);
  CuAssertTrue(tc, ship->summon_due >= ordered + 143 && ship->summon_due <= time(0) + 143);

  /* She still counts against the owner's cap, and is not summoned twice. */
  CuAssertIntEquals(tc, 1, vessel_owned_hull_count("Corr"));
  output = loss_harbor_command(&harbor, "the pet");
  CuAssertTrue(tc, strstr(output, "There is already an order out for the Petrel.") != NULL);
  output = loss_harbor_command(&harbor, "");
  CuAssertTrue(tc, strstr(output, "the Petrel (Warship): under summons, due in") != NULL);

  loss_harbor_end(&harbor);
}

void Test_vessel_summons_is_refused_to_a_hull_in_action(CuTest *tc)
{
  struct loss_harbor harbor;
  struct greyhawk_ship_data *ship;
  const char *output;

  ship = loss_harbor_begin(tc, &harbor);
  output = loss_harbor_command(&harbor, "3");
  CuAssertTrue(tc, strstr(output, "You own no such hull.") != NULL);

  ship->battle_ticks = 10;
  output = loss_harbor_command(&harbor, "1");
  CuAssertTrue(tc, strstr(output, "at battle stations and will not answer a summons") != NULL);
  ship->battle_ticks = 0;
  ship->sink_ticks = 100;
  output = loss_harbor_command(&harbor, "1");
  CuAssertTrue(tc, strstr(output, "is going down") != NULL);
  ship->sink_ticks = 0;
  GET_GOLD(&harbor.captain) = 5;
  output = loss_harbor_command(&harbor, "1");
  CuAssertTrue(tc, strstr(output, "wants 28 gold") != NULL);
  CuAssertTrue(tc, ship->active);

  /* Only from a shipyard. */
  REMOVE_BIT_AR(harbor.rooms[0].room_flags, ROOM_DOCKABLE);
  output = loss_harbor_command(&harbor, "1");
  CuAssertTrue(tc, strstr(output, "Hulls are summoned from a shipyard dock.") != NULL);

  loss_harbor_end(&harbor);
}

static MYSQL *loss_open_test_database(void)
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

void Test_vessel_wreck_is_rebuilt_as_the_cheapest_boat_for_sale(CuTest *tc)
{
  const char *enabled = getenv("LUMINARI_TEST_MYSQL_ENABLE");
  MYSQL *saved_conn;
  MYSQL *connection;
  bool saved_mysql_available;
  int id;
  int vclass;
  int speed;
  int armor;

  /* With no shipyard records the wreck is a boat to the vedit defaults. */
  saved_conn = conn;
  saved_mysql_available = mysql_available;
  mysql_available = FALSE;
  vessel_wreck_prototype(&id, &vclass, &speed, &armor);
  mysql_available = saved_mysql_available;
  CuAssertIntEquals(tc, 0, id);
  CuAssertIntEquals(tc, VESSEL_BOAT, vclass);
  CuAssertIntEquals(tc, 30, speed);
  CuAssertIntEquals(tc, 8, armor);

  if (enabled == NULL || strcmp(enabled, "1") != 0)
  {
    return;
  }
  connection = loss_open_test_database();
  if (connection == NULL)
  {
    CuFail(tc, "could not connect to the explicitly configured test database");
    return;
  }
  if (mysql_query(connection, "CREATE TEMPORARY TABLE ship_prototypes ("
                              "prototype_id INT PRIMARY KEY, name VARCHAR(127) NOT NULL, "
                              "vessel_class INT NOT NULL, max_speed INT NOT NULL, "
                              "armor INT NOT NULL, for_sale TINYINT(1) NOT NULL)") != 0 ||
      mysql_query(connection, "INSERT INTO ship_prototypes VALUES "
                              "(1, 'Skiff', 0, 5, 3, 1), (2, 'Gig', 1, 30, 8, 1), "
                              "(3, 'Pinnace', 1, 20, 6, 1), (4, 'Launch', 1, 30, 4, 0), "
                              "(5, 'Cog', 2, 20, 66, 1)") != 0)
  {
    mysql_close(connection);
    CuFail(tc, "could not create the isolated ship prototype fixture");
    return;
  }
  conn = connection;
  mysql_available = TRUE;

  /* The cheapest boat for sale, though a raft costs less and the unlisted
   * boat less still. */
  vessel_wreck_prototype(&id, &vclass, &speed, &armor);
  CuAssertIntEquals(tc, 3, id);
  CuAssertIntEquals(tc, VESSEL_BOAT, vclass);
  CuAssertIntEquals(tc, 20, speed);
  CuAssertIntEquals(tc, 6, armor);

  /* Without a boat for sale, the cheapest hull there is. */
  CuAssertIntEquals(tc, 0,
                    mysql_query(connection, "UPDATE ship_prototypes SET for_sale = 0 "
                                            "WHERE vessel_class = 1"));
  vessel_wreck_prototype(&id, &vclass, &speed, &armor);
  CuAssertIntEquals(tc, 1, id);
  CuAssertIntEquals(tc, VESSEL_RAFT, vclass);

  conn = saved_conn;
  mysql_available = saved_mysql_available;
  mysql_close(connection);
}

void Test_vessel_removed_players_stowed_hulls_are_purged(CuTest *tc)
{
  const char *enabled = getenv("LUMINARI_TEST_MYSQL_ENABLE");
  struct greyhawk_ship_data *ship;
  MYSQL *saved_conn;
  MYSQL *connection;
  bool saved_mysql_available;

  if (enabled == NULL || strcmp(enabled, "1") != 0)
  {
    return;
  }
  connection = loss_open_test_database();
  if (connection == NULL)
  {
    CuFail(tc, "could not connect to the explicitly configured test database");
    return;
  }
  saved_conn = conn;
  saved_mysql_available = mysql_available;
  conn = connection;
  mysql_available = TRUE;
  /* Boot adds the owner column; the CI test database starts from the
   * master schema alone. */
  vessel_ownership_ensure_schema();

  /* A wreck in the registry whose owner is deleted could never be
   * summoned again: her slot is freed. */
  ship = loss_ship(VESSEL_BOAT, "Lossremoved");
  ship->active = FALSE;
  ship->stowed = TRUE;
  CuAssertIntEquals(tc, 1, vessel_owned_hull_count("Lossremoved"));
  CuAssertTrue(tc, vessel_handle_player_removal("Lossremoved"));
  CuAssertTrue(tc, !ship->stowed);
  CuAssertIntEquals(tc, 0, ship->shipnum);
  CuAssertIntEquals(tc, 0, vessel_owned_hull_count("Lossremoved"));

  conn = saved_conn;
  mysql_available = saved_mysql_available;
  mysql_close(connection);
}

void Test_vessel_rename_costs_a_tenth_of_her_value(CuTest *tc)
{
  struct loss_harbor harbor;
  struct greyhawk_ship_data *ship;

  /* Her owner is aboard. The frigate is worth 44,000, and a hull without a
   * shipyard name of record has had her christening. */
  ship = loss_harbor_begin(tc, &harbor);
  char_from_room(&harbor.captain);
  char_to_room(&harbor.captain, 2);
  memset(harbor.output, 0, sizeof(harbor.output));
  harbor.descriptor.bufptr = 0;
  harbor.descriptor.bufspace = sizeof(harbor.output) - 1;
  do_shipchristen(&harbor.captain, "Storm Petrel", 0, 0);
  CuAssertTrue(tc, strstr(harbor.output, "The registry charges 4400 gold to rename the Petrel") !=
                       NULL);
  CuAssertStrEquals(tc, "the Petrel", ship->name);

  GET_GOLD(&harbor.captain) = 5000;
  do_shipchristen(&harbor.captain, "Storm Petrel", 0, 0);
  CuAssertStrEquals(tc, "Storm Petrel", ship->name);
  CuAssertIntEquals(tc, 600, GET_GOLD(&harbor.captain));

  /* Staff rename free. */
  GET_LEVEL(&harbor.captain) = LVL_IMMORT;
  do_shipchristen(&harbor.captain, "Gray Petrel", 0, 0);
  CuAssertStrEquals(tc, "Gray Petrel", ship->name);
  CuAssertIntEquals(tc, 600, GET_GOLD(&harbor.captain));

  loss_harbor_end(&harbor);
}
