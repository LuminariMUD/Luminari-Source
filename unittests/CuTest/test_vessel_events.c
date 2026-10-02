#include "CuTest.h"

#include "conf.h"
#include "../../src/core/sysdep.h"
#include "../../src/core/structs.h"
#include "../../src/core/utils.h"
#include "../../src/core/interpreter.h"
#include "../../src/core/comm.h"
#include "../../src/core/db.h"
#include "../../src/database/mysql.h"
#include "../../src/net/protocol.h"
#include "../../src/vessels/vessels.h"

#include <stdlib.h>
#include <string.h>

extern struct greyhawk_ship_data greyhawk_ships[GREYHAWK_MAXSHIPS];

void Test_vessel_event_type_parser_accepts_public_names(CuTest *tc)
{
  CuAssertIntEquals(tc, VESSEL_EVENT_REGATTA, vessel_event_type_from_name("regatta"));
  CuAssertIntEquals(tc, VESSEL_EVENT_REGATTA, vessel_event_type_from_name("RACE"));
  CuAssertIntEquals(tc, VESSEL_EVENT_SKIRMISH, vessel_event_type_from_name("battle"));
  CuAssertIntEquals(tc, VESSEL_EVENT_GHOST_FLEET, vessel_event_type_from_name("ghost-fleet"));
  CuAssertIntEquals(tc, VESSEL_EVENT_NONE, vessel_event_type_from_name("unknown"));
  CuAssertIntEquals(tc, VESSEL_EVENT_NONE, vessel_event_type_from_name(NULL));
}

void Test_vessel_event_finish_requires_entering_exact_coordinate(CuTest *tc)
{
  CuAssertTrue(tc, vessel_event_finish_reached(9, 20, 10, 20, 10, 20));
  CuAssertTrue(tc, !vessel_event_finish_reached(10, 20, 10, 20, 10, 20));
  CuAssertTrue(tc, !vessel_event_finish_reached(9, 20, 10, 21, 10, 20));
  CuAssertTrue(tc, !vessel_event_finish_reached(10, 19, 11, 20, 10, 20));
}

void Test_vessel_event_placement_points_have_floor(CuTest *tc)
{
  CuAssertIntEquals(tc, 0, vessel_event_placement_points(0));
  CuAssertIntEquals(tc, 100, vessel_event_placement_points(1));
  CuAssertIntEquals(tc, 90, vessel_event_placement_points(2));
  CuAssertIntEquals(tc, 10, vessel_event_placement_points(10));
  CuAssertIntEquals(tc, 10, vessel_event_placement_points(64));
}

void Test_vessel_event_team_winner_handles_ties(CuTest *tc)
{
  CuAssertIntEquals(tc, VESSEL_EVENT_TEAM_RED, vessel_event_winning_team(21, 20));
  CuAssertIntEquals(tc, VESSEL_EVENT_TEAM_BLUE, vessel_event_winning_team(10, 11));
  CuAssertIntEquals(tc, VESSEL_EVENT_TEAM_NONE, vessel_event_winning_team(8, 8));
}

/* A high fleet slot keeps this fixture clear of the rest of the suite. */
#define EVENTS_SHIP 482

static MYSQL *events_open_test_database(void)
{
  const char *port_text = getenv("LUMINARI_TEST_MYSQL_PORT");
  MYSQL *connection;

  if ((connection = mysql_init(NULL)) == NULL)
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

void Test_vessel_event_names_entrants_by_contact_id(CuTest *tc)
{
  struct greyhawk_ship_data *ship = &greyhawk_ships[EVENTS_SHIP];
  struct room_data bridge;
  struct room_data *saved_world;
  struct char_data captain;
  struct player_special_data specials;
  struct descriptor_data descriptor;
  char output[MAX_STRING_LENGTH];
  room_rnum saved_top_of_world;
  MYSQL *saved_conn;
  MYSQL *connection;
  bool saved_mysql_available;

  if (getenv("LUMINARI_TEST_MYSQL_ENABLE") == NULL ||
      strcmp(getenv("LUMINARI_TEST_MYSQL_ENABLE"), "1") != 0)
  {
    return;
  }
  connection = events_open_test_database();
  if (connection == NULL)
  {
    CuFail(tc, "could not connect to the explicitly configured test database");
    return;
  }
  if (mysql_query(connection, "CREATE TEMPORARY TABLE vessel_showcase_events ("
                              "event_id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY, "
                              "event_type VARCHAR(16) NOT NULL, "
                              "status VARCHAR(16) NOT NULL DEFAULT 'active', "
                              "staff_idnum BIGINT NOT NULL DEFAULT 0, "
                              "start_x INT NOT NULL DEFAULT 0, start_y INT NOT NULL DEFAULT 0, "
                              "finish_x INT NOT NULL DEFAULT 0, finish_y INT NOT NULL DEFAULT 0, "
                              "started_at BIGINT NOT NULL DEFAULT 0, "
                              "ended_at BIGINT NOT NULL DEFAULT 0, "
                              "end_reason VARCHAR(127) NOT NULL DEFAULT '') ENGINE=InnoDB") != 0 ||
      mysql_query(connection, "CREATE TEMPORARY TABLE vessel_event_participants ("
                              "event_id BIGINT UNSIGNED NOT NULL, ship_id INT NOT NULL, "
                              "player_idnum BIGINT NOT NULL DEFAULT 0, "
                              "team VARCHAR(8) NOT NULL DEFAULT 'none', "
                              "score INT NOT NULL DEFAULT 0, "
                              "finish_seconds INT NOT NULL DEFAULT 0, "
                              "placement INT NOT NULL DEFAULT 0, "
                              "status VARCHAR(16) NOT NULL DEFAULT 'active', "
                              "PRIMARY KEY (event_id, ship_id)) ENGINE=InnoDB") != 0)
  {
    mysql_close(connection);
    CuFail(tc, "could not create the isolated vessel event fixture");
    return;
  }

  /* A staff captain at her own helm opens a skirmish and enters it. */
  memset(&bridge, 0, sizeof(bridge));
  bridge.number = 169960;
  saved_world = world;
  saved_top_of_world = top_of_world;
  world = &bridge;
  top_of_world = 0;
  memset(ship, 0, sizeof(*ship));
  ship->active = TRUE;
  ship->shipnum = EVENTS_SHIP;
  ship->vessel_type = VESSEL_SHIP;
  ship->docked_to_ship = -1;
  ship->bridge_room = bridge.number;
  strlcpy(ship->id, "EV", sizeof(ship->id));
  strlcpy(ship->name, "the Petrel", sizeof(ship->name));
  strlcpy(ship->owner, "Mara", sizeof(ship->owner));
  bridge.ship = ship;
  memset(&captain, 0, sizeof(captain));
  memset(&specials, 0, sizeof(specials));
  captain.player_specials = &specials;
  captain.player.name = CuMutableString("Mara");
  GET_LEVEL(&captain) = LVL_IMMORT;
  GET_IDNUM(&captain) = 424244;
  IN_ROOM(&captain) = 0;
  memset(&descriptor, 0, sizeof(descriptor));
  descriptor.pProtocol = ProtocolCreate();
  CuAssertPtrNotNull(tc, descriptor.pProtocol);
  descriptor.character = &captain;
  descriptor.output = output;
  captain.desc = &descriptor;
  saved_conn = conn;
  saved_mysql_available = mysql_available;
  conn = connection;
  mysql_available = TRUE;

  do_vevent(&captain, "start skirmish", 0, 0);
  memset(output, 0, sizeof(output));
  descriptor.bufptr = 0;
  descriptor.bufspace = sizeof(output) - 1;
  do_vevent(&captain, "join red", 0, 0);
  do_vevent(&captain, "status", 0, 0);

  /* Players name a hull by her contact ID, never by fleet slot. */
  CuAssertTrue(tc, strstr(output, "Entered the Petrel [EV] in skirmish event #") != NULL);
  CuAssertTrue(tc, strstr(output, "  [EV] the Petrel") != NULL);
  CuAssertTrue(tc, strstr(output, "slot") == NULL);

  do_vevent(&captain, "cancel", 0, 0);
  conn = saved_conn;
  mysql_available = saved_mysql_available;
  mysql_close(connection);
  captain.desc = NULL;
  ProtocolDestroy(descriptor.pProtocol);
  memset(ship, 0, sizeof(*ship));
  world = saved_world;
  top_of_world = saved_top_of_world;
}

void Test_autopilot_cleanup_releases_assigned_route(CuTest *tc)
{
  struct greyhawk_ship_data ship;
  struct ship_route *route;

  memset(&ship, 0, sizeof(ship));
  CuAssertPtrNotNull(tc, autopilot_init(&ship));

  route = route_create("cleanup lifecycle route");
  CuAssertPtrNotNull(tc, route);
  CuAssertTrue(tc, autopilot_start(&ship, route));
  CuAssertTrue(tc, autopilot_stop(&ship));
  CuAssertPtrEquals(tc, route, ship.autopilot->current_route);
  CuAssertIntEquals(tc, AUTOPILOT_OFF, ship.autopilot->state);

  autopilot_cleanup(&ship);
  CuAssertPtrEquals(tc, NULL, ship.autopilot);
}

void Test_vessel_navigation_shutdown_releases_global_navigation_state(CuTest *tc)
{
  struct greyhawk_ship_data *ship;
  struct ship_route *route;

  ship = &greyhawk_ships[GREYHAWK_MAXSHIPS - 1];
  memset(ship, 0, sizeof(*ship));
  CuAssertPtrNotNull(tc, autopilot_init(ship));

  route = route_create("shutdown lifecycle route");
  CuAssertPtrNotNull(tc, route);
  CuAssertTrue(tc, autopilot_start(ship, route));
  ship->schedule = calloc(1, sizeof(*ship->schedule));
  CuAssertPtrNotNull(tc, ship->schedule);

  vessel_navigation_shutdown();

  CuAssertPtrEquals(tc, NULL, ship->autopilot);
  CuAssertPtrEquals(tc, NULL, ship->schedule);
  CuAssertPtrEquals(tc, NULL, route_list);
  CuAssertPtrEquals(tc, NULL, waypoint_list);

  vessel_navigation_shutdown();
}
