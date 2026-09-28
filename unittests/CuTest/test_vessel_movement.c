/* Vessel movement and pacing (vessels-ships study S2): class handling, the
 * maximum-speed formula, acceleration and turning, speed / 90 rooms per tick
 * with every room entered checked, berths, departures, anchoring, the setsail
 * maneuver, and autopilot steering through the same physics. */

#include "CuTest.h"

#include "conf.h"
#include "../../src/core/sysdep.h"
#include <math.h> /* before utils.h, which defines log() as a macro */
#include "../../src/core/structs.h"
#include "../../src/core/utils.h"
#include "../../src/core/comm.h"
#include "../../src/core/db.h"
#include "../../src/core/interpreter.h"
#include "../../src/vessels/vessels.h"

#include <string.h>
#include <time.h>

extern struct greyhawk_ship_data greyhawk_ships[GREYHAWK_MAXSHIPS];

/* A high fleet slot keeps this fixture clear of the rest of the suite. */
#define MOVEMENT_SHIP 470
#define MOVEMENT_ROOM_VNUM 169950
#define MOVEMENT_MAX_ENTRIES 64

struct movement_fixture
{
  struct room_data room;
  struct obj_data hull;
  struct char_data helm;
  struct player_special_data helm_specials;
  struct room_data *saved_world;
  room_rnum saved_top_of_world;
};

/* Rooms the movement code tried to enter, and the ones it may not. */
static int entered_count;
static int entered_x[MOVEMENT_MAX_ENTRIES];
static int entered_y[MOVEMENT_MAX_ENTRIES];
static int entered_z[MOVEMENT_MAX_ENTRIES];
static int refused_x;
static int refused_y;
static bool refuse_room;

static bool movement_enter_cell(int shipnum, int x, int y, int z)
{
  struct greyhawk_ship_data *ship = &greyhawk_ships[shipnum];

  if (refuse_room && x == refused_x && y == refused_y)
  {
    return FALSE;
  }
  if (entered_count < MOVEMENT_MAX_ENTRIES)
  {
    entered_x[entered_count] = x;
    entered_y[entered_count] = y;
    entered_z[entered_count] = z;
    entered_count++;
  }
  ship->x = (double)x;
  ship->y = (double)y;
  ship->z = (double)z;
  return TRUE;
}

/* A warship at (0,0) facing north at rest, with a helmsman on its bridge. */
static struct greyhawk_ship_data *movement_begin(struct movement_fixture *fixture,
                                                 enum vessel_class vessel_type)
{
  struct greyhawk_ship_data *ship = &greyhawk_ships[MOVEMENT_SHIP];

  memset(fixture, 0, sizeof(*fixture));
  fixture->saved_world = world;
  fixture->saved_top_of_world = top_of_world;
  fixture->room.number = MOVEMENT_ROOM_VNUM;
  fixture->room.sector_type = SECT_OCEAN;
  world = &fixture->room;
  top_of_world = 0;

  fixture->helm.player_specials = &fixture->helm_specials;
  fixture->helm.player.name = CuMutableString("Mara");
  IN_ROOM(&fixture->helm) = 0;
  GET_POS(&fixture->helm) = POS_STANDING;
  GET_LEVEL(&fixture->helm) = 30;
  IN_ROOM(&fixture->hull) = 0;

  memset(ship, 0, sizeof(*ship));
  ship->active = TRUE;
  ship->shipnum = MOVEMENT_SHIP;
  ship->vessel_type = vessel_type;
  ship->maxspeed = (short)vessel_class_handling(vessel_type)->speed;
  ship->docked_to_ship = -1;
  ship->bridge_room = MOVEMENT_ROOM_VNUM;
  ship->shipobj = &fixture->hull;
  ship->position_speed_percent = 100;
  strlcpy(ship->name, "the Heron", sizeof(ship->name));
  vessel_initialize_condition(ship, 40);
  fixture->room.ship = ship;

  entered_count = 0;
  refuse_room = FALSE;
  vessel_movement_set_cell_entry_for_test(movement_enter_cell);
  return ship;
}

static void movement_end(struct movement_fixture *fixture)
{
  struct greyhawk_ship_data *ship = &greyhawk_ships[MOVEMENT_SHIP];

  autopilot_cleanup(ship);
  memset(ship, 0, sizeof(*ship));
  vessel_movement_set_cell_entry_for_test(NULL);
  world = fixture->saved_world;
  top_of_world = fixture->saved_top_of_world;
}

static void movement_ticks(struct greyhawk_ship_data *ship, int ticks)
{
  int i;

  for (i = 0; i < ticks; i++)
  {
    autopilot_tick_one(ship);
    vessel_movement_tick_one(ship);
  }
}

void Test_vessel_class_handling_follows_the_study_table(CuTest *tc)
{
  const struct vessel_class_handling *warship = vessel_class_handling(VESSEL_WARSHIP);
  const struct vessel_class_handling *transport = vessel_class_handling(VESSEL_TRANSPORT);

  CuAssertIntEquals(tc, 17, warship->speed);
  CuAssertDblEquals(tc, 1.5, warship->accel, 0.0001);
  CuAssertDblEquals(tc, 4.0, warship->turn, 0.0001);
  CuAssertIntEquals(tc, 142, warship->max_load);
  CuAssertIntEquals(tc, 20, warship->free_fitout);
  CuAssertIntEquals(tc, 56, warship->full_hold);
  CuAssertIntEquals(tc, 0, warship->free_hold);

  CuAssertIntEquals(tc, 15, transport->speed);
  CuAssertIntEquals(tc, 140, transport->full_hold);
  CuAssertIntEquals(tc, 40, transport->free_hold);
  CuAssertIntEquals(tc, 30, vessel_class_handling(VESSEL_BOAT)->speed);
  CuAssertIntEquals(tc, 5, vessel_class_handling(VESSEL_RAFT)->speed);

  /* An unknown class handles like a ship. */
  CuAssertTrue(tc,
               vessel_class_handling(VESSEL_SHIP) == vessel_class_handling((enum vessel_class)99));
}

void Test_vessel_max_speed_combines_crew_load_sail_position_and_helm(CuTest *tc)
{
  CuAssertDblEquals(tc, 17.0, vessel_max_speed_from(17, 1.0, 1.0, 20, 20, 100, 0), 0.0001);
  CuAssertDblEquals(tc, 22.1, vessel_max_speed_from(17, 1.3, 1.0, 20, 20, 100, 0), 0.0001);
  CuAssertDblEquals(tc, 8.5, vessel_max_speed_from(17, 1.0, 0.5, 20, 20, 100, 0), 0.0001);
  CuAssertDblEquals(tc, 8.5, vessel_max_speed_from(17, 1.0, 1.0, 10, 20, 100, 0), 0.0001);
  CuAssertDblEquals(tc, 8.5, vessel_max_speed_from(17, 1.0, 1.0, 20, 20, 50, 0), 0.0001);
  CuAssertDblEquals(tc, 18.0, vessel_max_speed_from(17, 1.0, 1.0, 20, 20, 100, 1), 0.0001);

  /* Never below 1 while any sail stands, never above the speed limit. */
  CuAssertDblEquals(tc, 1.0, vessel_max_speed_from(17, 1.0, 0.01, 20, 20, 100, 0), 0.0001);
  CuAssertDblEquals(tc, 2.0, vessel_max_speed_from(17, 1.0, -0.5, 20, 20, 100, 1), 0.0001);
  CuAssertDblEquals(tc, 30.0, vessel_max_speed_from(30, 1.3, 1.0, 20, 20, 125, 1), 0.0001);

  /* No sail, no way. */
  CuAssertDblEquals(tc, 0.0, vessel_max_speed_from(17, 1.3, 1.0, 0, 20, 100, 1), 0.0001);
  CuAssertDblEquals(tc, 0.0, vessel_max_speed_from(0, 1.0, 1.0, 20, 20, 100, 0), 0.0001);
}

void Test_vessel_load_factor_counts_fitout_above_its_allowance(CuTest *tc)
{
  struct greyhawk_ship_data ship;

  memset(&ship, 0, sizeof(ship));
  ship.vessel_type = VESSEL_WARSHIP;
  CuAssertDblEquals(tc, 1.0, vessel_load_factor(&ship), 0.0001);

  /* The frigate carries 20 weight of fit-out free; 71 weighs 51 over. */
  ship.slot[0].type = 1;
  ship.slot[0].weight = 20;
  CuAssertDblEquals(tc, 1.0, vessel_load_factor(&ship), 0.0001);
  ship.slot[1].type = 1;
  ship.slot[1].weight = 51;
  CuAssertDblEquals(tc, 1.0 - 51.0 / 142.0, vessel_load_factor(&ship), 0.0001);

  /* An empty slot weighs nothing, whatever it records. */
  ship.slot[2].weight = 100;
  CuAssertDblEquals(tc, 1.0 - 51.0 / 142.0, vessel_load_factor(&ship), 0.0001);

  ship.crew_tier[CREW_SAILMASTER] = CREW_TIER_VETERAN;
  CuAssertDblEquals(tc, 1.3, vessel_sailmaster_multiplier(&ship), 0.0001);
}

void Test_vessel_turn_rate_follows_speed_rudder_and_crew(CuTest *tc)
{
  struct greyhawk_ship_data ship;

  memset(&ship, 0, sizeof(ship));
  ship.vessel_type = VESSEL_WARSHIP;
  ship.maxspeed = 17;
  ship.maxturnrate = ship.turnrate = 20;

  ship.speed = 17.0;
  CuAssertDblEquals(tc, 4.0, vessel_turn_rate(&ship, 17.0), 0.0001);
  ship.speed = 3.0; /* steerage way turns at three quarters */
  CuAssertDblEquals(tc, 3.0, vessel_turn_rate(&ship, 17.0), 0.0001);

  ship.speed = 17.0;
  ship.turnrate = 10;
  CuAssertDblEquals(tc, 2.0, vessel_turn_rate(&ship, 17.0), 0.0001);
  ship.crew_tier[CREW_SAILMASTER] = CREW_TIER_ABLE;
  CuAssertDblEquals(tc, 2.4, vessel_turn_rate(&ship, 17.0), 0.0001);

  /* A smashed rudder cannot turn; an immobile hull warps round slowly. */
  ship.turnrate = 0;
  CuAssertDblEquals(tc, 0.0, vessel_turn_rate(&ship, 17.0), 0.0001);
  ship.turnrate = 20;
  CuAssertDblEquals(tc, 1.0, vessel_turn_rate(&ship, 0.0), 0.0001);
}

void Test_vessel_hull_gathers_way_and_sails_speed_over_90_rooms_a_tick(CuTest *tc)
{
  struct movement_fixture fixture;
  struct greyhawk_ship_data *ship;

  ship = movement_begin(&fixture, VESSEL_WARSHIP);

  /* The frigate gains 1.5 a tick until it makes its ordered 17. */
  ship->setspeed = 17;
  vessel_movement_tick_one(ship);
  CuAssertDblEquals(tc, 1.5, ship->speed, 0.0001);
  movement_ticks(ship, 10);
  CuAssertDblEquals(tc, 16.5, ship->speed, 0.0001);
  movement_ticks(ship, 1);
  CuAssertDblEquals(tc, 17.0, ship->speed, 0.0001);

  /* At 17 she covers 17 rooms in 90 ticks (45 seconds), one room at a time. */
  ship->x = 0.0;
  ship->y = 0.0;
  ship->dx = 0.0;
  ship->dy = 0.0;
  entered_count = 0;
  movement_ticks(ship, 90);
  CuAssertIntEquals(tc, 17, entered_count);
  CuAssertIntEquals(tc, 17, (int)ship->y);
  CuAssertIntEquals(tc, 0, (int)ship->x);
  CuAssertIntEquals(tc, 1, entered_y[0]);
  CuAssertIntEquals(tc, 2, entered_y[1]);

  /* All stop: she loses way at the same rate and holds her room. */
  ship->setspeed = 0;
  movement_ticks(ship, 12);
  CuAssertDblEquals(tc, 0.0, ship->speed, 0.0001);
  CuAssertIntEquals(tc, 0, ship->setspeed);

  movement_end(&fixture);
}

void Test_vessel_hull_comes_about_at_its_turn_rate(CuTest *tc)
{
  struct movement_fixture fixture;
  struct greyhawk_ship_data *ship;
  int i;

  ship = movement_begin(&fixture, VESSEL_WARSHIP);
  ship->speed = 17.0;
  ship->setspeed = 17;
  ship->setheading = 90;

  vessel_movement_tick_one(ship);
  CuAssertDblEquals(tc, 4.0, ship->heading, 0.0001);
  for (i = 0; i < 22; i++)
  {
    vessel_movement_tick_one(ship);
  }
  CuAssertDblEquals(tc, 90.0, ship->heading, 0.0001);
  CuAssertIntEquals(tc, 90, vessel_display_heading(ship->heading));

  /* The short way round: from 90 to 350 turns to port. */
  ship->setheading = 350;
  vessel_movement_tick_one(ship);
  CuAssertDblEquals(tc, 86.0, ship->heading, 0.0001);

  /* The helm no longer answers with the rudder shot away. */
  ship->turnrate = 0;
  vessel_movement_tick_one(ship);
  CuAssertDblEquals(tc, 86.0, ship->heading, 0.0001);

  movement_end(&fixture);
}

void Test_vessel_refused_room_stops_the_hull_at_its_edge(CuTest *tc)
{
  struct movement_fixture fixture;
  struct greyhawk_ship_data *ship;
  struct ship_route *route;

  ship = movement_begin(&fixture, VESSEL_WARSHIP);
  ship->speed = 17.0;
  ship->setspeed = 17;
  refuse_room = TRUE;
  refused_x = 0;
  refused_y = 1;

  /* An autopilot bound north pauses when the coast refuses the next room. */
  route = route_create("coast");
  CuAssertPtrNotNull(tc, route);
  CuAssertPtrNotNull(tc, autopilot_init(ship));
  CuAssertIntEquals(tc, 0, waypoint_add(route, 0.0, 20.0, 0.0, "north"));
  CuAssertTrue(tc, autopilot_start(ship, route));

  movement_ticks(ship, 4);
  CuAssertIntEquals(tc, 0, entered_count);
  CuAssertIntEquals(tc, 0, (int)ship->y);
  CuAssertDblEquals(tc, 0.5, ship->dy, 0.0001);
  CuAssertDblEquals(tc, 0.0, ship->speed, 0.0001);
  CuAssertIntEquals(tc, 0, ship->setspeed);
  CuAssertIntEquals(tc, AUTOPILOT_PAUSED, ship->autopilot->state);

  movement_end(&fixture);
}

void Test_vessel_rest_in_port_berths_and_undock_casts_off(CuTest *tc)
{
  struct movement_fixture fixture;
  struct greyhawk_ship_data *ship;
  int i;

  ship = movement_begin(&fixture, VESSEL_WARSHIP);
  SET_BIT_AR(fixture.room.room_flags, ROOM_DOCKABLE);

  /* Coming to rest in port makes her fast at the berth. */
  ship->speed = 1.5;
  vessel_movement_tick_one(ship);
  CuAssertDblEquals(tc, 0.0, ship->speed, 0.0001);
  CuAssertIntEquals(tc, MOVEMENT_ROOM_VNUM, ship->dock);
  CuAssertTrue(tc, vessel_is_moored(ship));

  /* A berthed hull holds whatever is ordered. */
  ship->setspeed = 10;
  movement_ticks(ship, 5);
  CuAssertDblEquals(tc, 0.0, ship->speed, 0.0001);
  ship->setspeed = 0;

  /* Clearance waits on fees and a whole sail. */
  ship->dock_fee_balance = 40;
  CuAssertTrue(tc, !vessel_begin_departure(ship, NULL));
  ship->dock_fee_balance = 0;
  ship->mainsail = 0;
  CuAssertTrue(tc, !vessel_begin_departure(ship, NULL));
  ship->mainsail = ship->maxmainsail;

  /* Casting off takes 30 seconds. */
  CuAssertTrue(tc, vessel_begin_departure(ship, NULL));
  CuAssertIntEquals(tc, VESSEL_UNDOCK_TICKS, ship->departure_ticks);
  CuAssertTrue(tc, !vessel_begin_departure(ship, NULL));
  for (i = 0; i < VESSEL_UNDOCK_TICKS - 1; i++)
  {
    vessel_movement_tick_one(ship);
  }
  CuAssertTrue(tc, vessel_is_moored(ship));
  vessel_movement_tick_one(ship);
  CuAssertTrue(tc, !vessel_is_moored(ship));
  CuAssertIntEquals(tc, 0, ship->dock);

  /* Under way at rest in port, she is not berthed again until she moves. */
  vessel_movement_tick_one(ship);
  CuAssertIntEquals(tc, 0, ship->dock);

  /* After a reboot a hull at rest in port berths, and a berth away from
   * port is stale. */
  REMOVE_BIT_AR(fixture.room.room_flags, ROOM_DOCKABLE);
  fixture.room.sector_type = SECT_SEAPORT;
  vessel_sync_berth(ship);
  CuAssertIntEquals(tc, MOVEMENT_ROOM_VNUM, ship->dock);
  fixture.room.sector_type = SECT_OCEAN;
  vessel_sync_berth(ship);
  CuAssertIntEquals(tc, 0, ship->dock);

  movement_end(&fixture);
}

void Test_vessel_anchor_holds_and_weighs_in_13_seconds(CuTest *tc)
{
  struct movement_fixture fixture;
  struct greyhawk_ship_data *ship;
  int i;

  ship = movement_begin(&fixture, VESSEL_WARSHIP);
  IN_ROOM(&fixture.hull) = NOWHERE;

  /* She must be stopped to anchor. */
  ship->speed = 4.0;
  do_vessel_anchor(&fixture.helm, "", 0, 0);
  CuAssertTrue(tc, !ship->anchored);

  ship->speed = 0.0;
  do_vessel_anchor(&fixture.helm, "", 0, 0);
  CuAssertTrue(tc, ship->anchored);
  CuAssertTrue(tc, vessel_is_moored(ship));

  /* Speed orders wait until the anchor is up. */
  do_greyhawk_speed(&fixture.helm, "10", 0, 0);
  CuAssertIntEquals(tc, 0, ship->setspeed);

  do_undock(&fixture.helm, "", 0, 0);
  CuAssertIntEquals(tc, VESSEL_WEIGH_ANCHOR_TICKS, ship->departure_ticks);
  for (i = 0; i < VESSEL_WEIGH_ANCHOR_TICKS; i++)
  {
    vessel_movement_tick_one(ship);
  }
  CuAssertTrue(tc, !ship->anchored);

  do_greyhawk_speed(&fixture.helm, "10", 0, 0);
  CuAssertIntEquals(tc, 10, ship->setspeed);
  CuAssertDblEquals(tc, 0.0, ship->speed, 0.0001);
  vessel_movement_tick_one(ship);
  CuAssertDblEquals(tc, 1.5, ship->speed, 0.0001);

  movement_end(&fixture);
}

void Test_vessel_setsail_maneuvers_one_room_at_steerage_speed(CuTest *tc)
{
  struct movement_fixture fixture;
  struct greyhawk_ship_data *ship;
  int i;

  ship = movement_begin(&fixture, VESSEL_WARSHIP);
  IN_ROOM(&fixture.hull) = NOWHERE;

  /* Too fast to maneuver. */
  ship->speed = 7.0;
  CuAssertTrue(tc, !vessel_maneuver(ship, &fixture.helm, EAST));
  CuAssertIntEquals(tc, 0, entered_count);

  /* One room east, then stopped on the new heading. */
  ship->speed = 6.0;
  ship->setspeed = 6;
  CuAssertTrue(tc, vessel_maneuver(ship, &fixture.helm, EAST));
  CuAssertIntEquals(tc, 1, entered_count);
  CuAssertIntEquals(tc, 1, entered_x[0]);
  CuAssertIntEquals(tc, 0, entered_y[0]);
  CuAssertDblEquals(tc, 0.0, ship->speed, 0.0001);
  CuAssertIntEquals(tc, 0, ship->setspeed);
  CuAssertIntEquals(tc, 90, vessel_display_heading(ship->heading));
  CuAssertIntEquals(tc, VESSEL_MANEUVER_COOLDOWN_TICKS, ship->maneuver_ticks);

  /* Five seconds between maneuvers. */
  CuAssertTrue(tc, !vessel_maneuver(ship, &fixture.helm, EAST));
  for (i = 0; i < VESSEL_MANEUVER_COOLDOWN_TICKS; i++)
  {
    vessel_movement_tick_one(ship);
  }
  CuAssertTrue(tc, vessel_maneuver(ship, &fixture.helm, NORTHEAST));
  CuAssertIntEquals(tc, 2, entered_x[1]);
  CuAssertIntEquals(tc, 1, entered_y[1]);

  /* A refused room leaves her where she lay. */
  ship->maneuver_ticks = 0;
  refuse_room = TRUE;
  refused_x = 3;
  refused_y = 1;
  CuAssertTrue(tc, !vessel_maneuver(ship, &fixture.helm, EAST));
  CuAssertIntEquals(tc, 2, (int)ship->x);
  refuse_room = FALSE;

  /* Maneuvering into port makes her fast at the berth. */
  IN_ROOM(&fixture.hull) = 0;
  SET_BIT_AR(fixture.room.room_flags, ROOM_DOCKABLE);
  CuAssertTrue(tc, vessel_maneuver(ship, &fixture.helm, WEST));
  CuAssertIntEquals(tc, MOVEMENT_ROOM_VNUM, ship->dock);
  ship->maneuver_ticks = 0;
  CuAssertTrue(tc, !vessel_maneuver(ship, &fixture.helm, EAST));

  movement_end(&fixture);
}

void Test_vessel_airship_climbs_under_way(CuTest *tc)
{
  struct movement_fixture fixture;
  struct greyhawk_ship_data *ship;

  ship = movement_begin(&fixture, VESSEL_AIRSHIP);
  IN_ROOM(&fixture.hull) = NOWHERE;
  ship->speed = 20.0;
  ship->setspeed = 20;

  CuAssertTrue(tc, vessel_maneuver(ship, &fixture.helm, UP));
  CuAssertIntEquals(tc, 10, entered_z[0]);
  CuAssertDblEquals(tc, 20.0, ship->speed, 0.0001);
  CuAssertIntEquals(tc, 20, ship->setspeed);

  movement_end(&fixture);
}

void Test_vessel_autopilot_steers_turns_slow_and_stops_at_waypoints(CuTest *tc)
{
  struct movement_fixture fixture;
  struct greyhawk_ship_data *ship;
  struct ship_route *route;
  int ticks;

  ship = movement_begin(&fixture, VESSEL_WARSHIP);
  IN_ROOM(&fixture.hull) = NOWHERE;
  route = route_create("sound");
  CuAssertPtrNotNull(tc, route);
  CuAssertPtrNotNull(tc, autopilot_init(ship));
  CuAssertIntEquals(tc, 0, waypoint_add(route, 10.0, 0.0, 0.0, "buoy"));
  CuAssertIntEquals(tc, 1, waypoint_add(route, 10.0, 10.0, 0.0, "cape"));
  route->waypoints[0].tolerance = 0.5;
  route->waypoints[0].wait_time = 5;
  route->waypoints[1].tolerance = 0.5;
  CuAssertTrue(tc, autopilot_start(ship, route));

  /* Bow 90 degrees off: steerage speed until she has come about. */
  movement_ticks(ship, 1);
  CuAssertIntEquals(tc, 90, ship->setheading);
  CuAssertDblEquals(tc, (double)VESSEL_STEERAGE_SPEED, ship->autopilot->speed_limit, 0.0001);
  CuAssertTrue(tc, ship->speed <= (double)VESSEL_STEERAGE_SPEED);

  /* She reaches the buoy, heaves to, and keeps her cruise order unset. */
  for (ticks = 0; ticks < 400 && ship->autopilot->state == AUTOPILOT_TRAVELING; ticks++)
  {
    movement_ticks(ship, 1);
  }
  CuAssertIntEquals(tc, AUTOPILOT_WAITING, ship->autopilot->state);
  CuAssertIntEquals(tc, 10, (int)ship->x);
  CuAssertIntEquals(tc, 0, (int)ship->y);
  movement_ticks(ship, 20);
  CuAssertDblEquals(tc, 0.0, ship->speed, 0.0001);
  CuAssertIntEquals(tc, 10, (int)ship->x);

  /* The wait ends and she sails for the cape at full speed. */
  ship->autopilot->last_update = time(0) - 6;
  for (ticks = 0; ticks < 600 && ship->autopilot->waypoint_arrivals < 2; ticks++)
  {
    movement_ticks(ship, 1);
  }
  CuAssertTrue(tc, ship->autopilot->waypoint_arrivals >= 2);
  CuAssertIntEquals(tc, 10, (int)ship->y);
  CuAssertIntEquals(tc, 10, (int)ship->x);

  movement_end(&fixture);
}

void Test_vessel_autopilot_casts_off_before_following_its_route(CuTest *tc)
{
  struct movement_fixture fixture;
  struct greyhawk_ship_data *ship;
  struct ship_route *route;

  ship = movement_begin(&fixture, VESSEL_SHIP);
  SET_BIT_AR(fixture.room.room_flags, ROOM_DOCKABLE);
  vessel_berth(ship);
  route = route_create("outbound");
  CuAssertPtrNotNull(tc, route);
  CuAssertPtrNotNull(tc, autopilot_init(ship));
  CuAssertIntEquals(tc, 0, waypoint_add(route, 0.0, 10.0, 0.0, "offing"));
  CuAssertTrue(tc, autopilot_start(ship, route));

  movement_ticks(ship, 1);
  CuAssertIntEquals(tc, VESSEL_UNDOCK_TICKS - 1, ship->departure_ticks);
  movement_ticks(ship, VESSEL_UNDOCK_TICKS - 1);
  CuAssertIntEquals(tc, 0, ship->dock);
  REMOVE_BIT_AR(fixture.room.room_flags, ROOM_DOCKABLE);
  movement_ticks(ship, 2);
  CuAssertTrue(tc, ship->speed > 0.0);

  movement_end(&fixture);
}

void Test_vessel_paused_autopilot_holds_and_a_finished_route_stops(CuTest *tc)
{
  struct movement_fixture fixture;
  struct greyhawk_ship_data *ship;
  struct ship_route *route;

  ship = movement_begin(&fixture, VESSEL_WARSHIP);
  IN_ROOM(&fixture.hull) = NOWHERE;
  route = route_create("errand");
  CuAssertPtrNotNull(tc, route);
  CuAssertPtrNotNull(tc, autopilot_init(ship));
  CuAssertIntEquals(tc, 0, waypoint_add(route, 0.0, 30.0, 0.0, "far"));
  CuAssertTrue(tc, autopilot_start(ship, route));
  ship->speed = 12.0;
  ship->setspeed = 12;

  CuAssertTrue(tc, autopilot_pause(ship));
  movement_ticks(ship, 10);
  CuAssertDblEquals(tc, 0.0, ship->speed, 0.0001);
  CuAssertIntEquals(tc, 12, ship->setspeed);

  /* Arriving at the last waypoint of a one-way route brings her to a stop. */
  CuAssertTrue(tc, autopilot_resume(ship));
  ship->y = 30.0;
  movement_ticks(ship, 1);
  CuAssertIntEquals(tc, AUTOPILOT_COMPLETE, ship->autopilot->state);
  CuAssertIntEquals(tc, 0, ship->setspeed);

  movement_end(&fixture);
}

void Test_vessel_draft_keeps_deep_hulls_out_of_shallows(CuTest *tc)
{
  CuAssertTrue(tc, !vessel_draft_allows(VESSEL_SHIP, SECT_OCEAN, 1));
  CuAssertTrue(tc, vessel_draft_allows(VESSEL_SHIP, SECT_OCEAN, 2));
  CuAssertTrue(tc, !vessel_draft_allows(VESSEL_TRANSPORT, SECT_WATER_SWIM, 0));
  CuAssertTrue(tc, vessel_draft_allows(VESSEL_SHIP, SECT_SEAPORT, 0));
  CuAssertTrue(tc, vessel_draft_allows(VESSEL_RAFT, SECT_WATER_SWIM, 0));
  CuAssertTrue(tc, vessel_draft_allows(VESSEL_AIRSHIP, SECT_OCEAN, 0));
}
