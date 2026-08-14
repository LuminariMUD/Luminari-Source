#include "CuTest.h"

#include "../../src/conf.h"
#include "../../src/sysdep.h"
#include "../../src/structs.h"
#include "../../src/utils.h"
#include "../../src/interpreter.h"
#include "../../src/vessels/vessels.h"

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
