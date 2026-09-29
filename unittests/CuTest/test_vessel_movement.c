/* Vessel movement and pacing (vessels-ships study S2): class handling, the
 * maximum-speed formula, acceleration and turning, speed / 90 rooms per tick
 * with every room entered checked, berths, departures, anchoring, the setsail
 * maneuver, autopilot steering through the same physics, and scheduled-route
 * validation along the line the hull will sail. */

#include "CuTest.h"

#include "conf.h"
#include "../../src/core/sysdep.h"
#include <math.h> /* before utils.h, which defines log() as a macro */
#include "../../src/core/structs.h"
#include "../../src/core/utils.h"
#include "../../src/core/comm.h"
#include "../../src/core/db.h"
#include "../../src/core/interpreter.h"
#include "../../src/database/mysql.h"
#include "../../src/vessels/vessels.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

extern struct greyhawk_ship_data greyhawk_ships[GREYHAWK_MAXSHIPS];

/* A high fleet slot keeps this fixture clear of the rest of the suite. */
#define MOVEMENT_SHIP 470
#define MOVEMENT_ROOM_VNUM 169950
#define MOVEMENT_MAX_ENTRIES 64
#define MOVEMENT_WAYPOINT_ID 916001

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

/* The port room for movement_enter_port(). */
static struct room_data *port_room;
static int port_x;
static int port_y;

static bool movement_enter_cell(struct greyhawk_ship_data *ship, int x, int y, int z)
{
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

/* The fixture's single room is a port only while the hull lies at the port. */
static bool movement_enter_port(struct greyhawk_ship_data *ship, int x, int y, int z)
{
  if (!movement_enter_cell(ship, x, y, z))
  {
    return FALSE;
  }
  if (x == port_x && y == port_y)
  {
    SET_BIT_AR(port_room->room_flags, ROOM_DOCKABLE);
  }
  else
  {
    REMOVE_BIT_AR(port_room->room_flags, ROOM_DOCKABLE);
  }
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
  const struct vessel_class_handling *unknown;

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
  /* NOLINTNEXTLINE(clang-analyzer-optin.core.EnumCastOutOfRange) -- tests the invalid-value path */
  unknown = vessel_class_handling((enum vessel_class)99);
  CuAssertTrue(tc, vessel_class_handling(VESSEL_SHIP) == unknown);
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

  /* The frigate carries 20 weight of fit-out free: two large ballistae. */
  vessel_set_weapon(&ship.slot[0], VESSEL_WEAPON_LARGE_BALLISTA, GREYHAWK_PORT);
  vessel_set_weapon(&ship.slot[1], VESSEL_WEAPON_LARGE_BALLISTA, GREYHAWK_STARBOARD);
  CuAssertDblEquals(tc, 1.0, vessel_load_factor(&ship), 0.0001);
  vessel_set_weapon(&ship.slot[2], VESSEL_WEAPON_HEAVY_BALLISTA, GREYHAWK_PORT);
  CuAssertDblEquals(tc, 1.0 - 15.0 / 142.0, vessel_load_factor(&ship), 0.0001);

  /* A ram weighs (hull weight + 10) / 24, 12 on a frigate; colors nothing. */
  ship.slot[3].type = VESSEL_SLOT_EQUIPMENT;
  ship.slot[3].item = VESSEL_EQUIPMENT_RAM;
  ship.slot[4].type = VESSEL_SLOT_EQUIPMENT;
  ship.slot[4].item = VESSEL_EQUIPMENT_COLORS;
  CuAssertDblEquals(tc, 1.0 - 27.0 / 142.0, vessel_load_factor(&ship), 0.0001);

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

void Test_vessel_hull_crossing_a_corner_enters_the_diagonal_room(CuTest *tc)
{
  struct movement_fixture fixture;
  struct greyhawk_ship_data *ship;

  /* Due northeast from the room's centre she crosses both edges at once and
   * enters the diagonal room, as the eight-direction grid allows. */
  ship = movement_begin(&fixture, VESSEL_BOAT);
  IN_ROOM(&fixture.hull) = NOWHERE;
  ship->heading = 45.0;
  ship->setheading = 45;
  ship->speed = 30.0;
  ship->setspeed = 30;
  movement_ticks(ship, 3);
  CuAssertIntEquals(tc, 1, entered_count);
  CuAssertIntEquals(tc, 1, entered_x[0]);
  CuAssertIntEquals(tc, 1, entered_y[0]);

  movement_end(&fixture);
}

void Test_vessel_hull_crosses_edges_in_the_order_her_track_meets_them(CuTest *tc)
{
  struct movement_fixture fixture;
  struct greyhawk_ship_data *ship;

  /* From near the northeast corner on heading 60 at speed 30 her track meets
   * the east edge before the north edge: she enters (1,0), then (1,1). */
  ship = movement_begin(&fixture, VESSEL_BOAT);
  IN_ROOM(&fixture.hull) = NOWHERE;
  ship->dx = 0.49;
  ship->dy = 0.49;
  ship->heading = 60.0;
  ship->setheading = 60;
  ship->speed = 30.0;
  ship->setspeed = 30;
  vessel_movement_tick_one(ship);
  CuAssertIntEquals(tc, 2, entered_count);
  CuAssertIntEquals(tc, 1, entered_x[0]);
  CuAssertIntEquals(tc, 0, entered_y[0]);
  CuAssertIntEquals(tc, 1, entered_x[1]);
  CuAssertIntEquals(tc, 1, entered_y[1]);

  /* Land to the east stops her where her track met its edge, short of the
   * north edge; she never passes it to reach (1,1). */
  ship->x = 0.0;
  ship->y = 0.0;
  ship->dx = 0.49;
  ship->dy = 0.49;
  ship->speed = 30.0;
  ship->setspeed = 30;
  entered_count = 0;
  refuse_room = TRUE;
  refused_x = 1;
  refused_y = 0;
  vessel_movement_tick_one(ship);
  CuAssertIntEquals(tc, 0, entered_count);
  CuAssertIntEquals(tc, 0, (int)ship->x);
  CuAssertIntEquals(tc, 0, (int)ship->y);
  CuAssertDblEquals(tc, 0.5, ship->dx, 0.0001);
  CuAssertTrue(tc, ship->dy > 0.49 && ship->dy < 0.5);
  CuAssertDblEquals(tc, 0.0, ship->speed, 0.0001);

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

  /* The harbor makes good an unowned hull's rigging and rudder when it
   * berths; an owner repairs their own. */
  ship->dock = 0;
  ship->mainsail = 1;
  ship->turnrate = 1;
  strlcpy(ship->owner, "Mara", sizeof(ship->owner));
  vessel_berth(ship);
  CuAssertIntEquals(tc, 1, ship->mainsail);
  CuAssertIntEquals(tc, 1, ship->turnrate);
  ship->owner[0] = '\0';
  vessel_berth(ship);
  CuAssertIntEquals(tc, ship->maxmainsail, ship->mainsail);
  CuAssertIntEquals(tc, ship->maxturnrate, ship->turnrate);
  ship->dock = 0;

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
  route->waypoints[0].wait_time = 5;
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

void Test_vessel_autopilot_comes_about_before_sailing_astern(CuTest *tc)
{
  struct movement_fixture fixture;
  struct greyhawk_ship_data *ship;
  struct ship_route *route;
  int ticks;

  ship = movement_begin(&fixture, VESSEL_SHIP);
  IN_ROOM(&fixture.hull) = NOWHERE;
  route = route_create("astern");
  CuAssertPtrNotNull(tc, route);
  CuAssertPtrNotNull(tc, autopilot_init(ship));
  CuAssertIntEquals(tc, 0, waypoint_add(route, 0.0, -10.0, 0.0, "astern"));
  CuAssertTrue(tc, autopilot_start(ship, route));

  /* The waypoint lies astern: she turns where she lies before making way. */
  movement_ticks(ship, 1);
  CuAssertIntEquals(tc, 180, ship->setheading);
  CuAssertDblEquals(tc, 0.0, ship->autopilot->speed_limit, 0.0001);
  CuAssertDblEquals(tc, 0.0, ship->speed, 0.0001);
  for (ticks = 0; ticks < 200 && ship->speed <= 0.0; ticks++)
  {
    movement_ticks(ship, 1);
  }
  CuAssertTrue(tc, ship->speed > 0.0);
  CuAssertIntEquals(tc, 0, entered_count);
  CuAssertTrue(tc, fabs(vessel_heading_difference(ship->heading, 180.0)) <= 90.0);

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

void Test_vessel_created_route_reaches_and_berths_at_its_port(CuTest *tc)
{
  struct movement_fixture fixture;
  struct greyhawk_ship_data *ship;
  struct ship_route *route;
  int ticks;

  /* A new waypoint is reached on entering its own room, so a one-way route
   * to a port brings the hull to rest there, berthed. */
  ship = movement_begin(&fixture, VESSEL_WARSHIP);
  port_room = &fixture.room;
  port_x = 0;
  port_y = 12;
  vessel_movement_set_cell_entry_for_test(movement_enter_port);
  route = route_create("homeward");
  CuAssertPtrNotNull(tc, route);
  CuAssertPtrNotNull(tc, autopilot_init(ship));
  CuAssertIntEquals(tc, 0, waypoint_add(route, 0.0, 12.0, 0.0, "home"));
  CuAssertDblEquals(tc, AUTOPILOT_ARRIVAL_TOLERANCE, route->waypoints[0].tolerance, 0.0001);
  CuAssertTrue(tc, autopilot_start(ship, route));

  for (ticks = 0;
       ticks < 1000 && (ship->autopilot->state != AUTOPILOT_COMPLETE || ship->speed > 0.0); ticks++)
  {
    movement_ticks(ship, 1);
  }
  CuAssertIntEquals(tc, AUTOPILOT_COMPLETE, ship->autopilot->state);
  CuAssertIntEquals(tc, 0, (int)ship->x);
  CuAssertIntEquals(tc, 12, (int)ship->y);
  CuAssertIntEquals(tc, MOVEMENT_ROOM_VNUM, ship->dock);

  movement_end(&fixture);
}

void Test_vessel_schedule_check_sails_the_turn_the_hull_will_make(CuTest *tc)
{
  struct movement_fixture fixture;
  struct greyhawk_ship_data *ship;
  struct waypoint_node buoy;
  struct waypoint_node cape;
  struct waypoint_node *saved_waypoints;
  struct route_node sound;
  struct ship_route *route;
  const char *bad_waypoint;
  int waypoint_ids[2];
  int probe_x[MOVEMENT_MAX_ENTRIES];
  int probe_y[MOVEMENT_MAX_ENTRIES];
  int probe_count;
  int bad_x;
  int bad_y;
  bool overshoot;
  int ticks;
  int i;

  /* A frigate running east at full speed for a buoy at (10,0), then north
   * for a cape at (10,10): she carries her way past the buoy while she comes
   * round. */
  ship = movement_begin(&fixture, VESSEL_WARSHIP);
  IN_ROOM(&fixture.hull) = NOWHERE;
  ship->heading = 90.0;
  ship->setheading = 90;
  ship->speed = 17.0;

  memset(&buoy, 0, sizeof(buoy));
  memset(&cape, 0, sizeof(cape));
  memset(&sound, 0, sizeof(sound));
  buoy.waypoint_id = MOVEMENT_WAYPOINT_ID;
  buoy.data.x = 10.0;
  buoy.data.tolerance = AUTOPILOT_ARRIVAL_TOLERANCE;
  strlcpy(buoy.data.name, "buoy", sizeof(buoy.data.name));
  cape.waypoint_id = MOVEMENT_WAYPOINT_ID + 1;
  cape.data.x = 10.0;
  cape.data.y = 10.0;
  cape.data.tolerance = AUTOPILOT_ARRIVAL_TOLERANCE;
  strlcpy(cape.data.name, "cape", sizeof(cape.data.name));
  waypoint_ids[0] = buoy.waypoint_id;
  waypoint_ids[1] = cape.waypoint_id;
  sound.route_id = MOVEMENT_WAYPOINT_ID;
  strlcpy(sound.name, "sound", sizeof(sound.name));
  sound.active = TRUE;
  sound.num_waypoints = 2;
  sound.waypoint_ids = waypoint_ids;
  saved_waypoints = waypoint_list;
  cape.next = saved_waypoints;
  buoy.next = &cape;
  waypoint_list = &buoy;

  /* The check follows her wide of the buoy into (11,0), and leaves the hull
   * itself where she lies. */
  CuAssertTrue(tc, scheduled_route_is_traversable(ship, &sound, &bad_waypoint, &bad_x, &bad_y));
  probe_count = entered_count;
  overshoot = FALSE;
  for (i = 0; i < probe_count; i++)
  {
    probe_x[i] = entered_x[i];
    probe_y[i] = entered_y[i];
    overshoot = overshoot || (probe_x[i] == 11 && probe_y[i] == 0);
  }
  CuAssertTrue(tc, overshoot);
  CuAssertDblEquals(tc, 0.0, ship->x, 0.0001);
  CuAssertDblEquals(tc, 17.0, ship->speed, 0.0001);
  CuAssertDblEquals(tc, 90.0, ship->heading, 0.0001);

  /* The live autopilot on the same route enters the same rooms. */
  route = route_create("sound");
  CuAssertPtrNotNull(tc, route);
  CuAssertPtrNotNull(tc, autopilot_init(ship));
  CuAssertIntEquals(tc, 0, waypoint_add(route, 10.0, 0.0, 0.0, "buoy"));
  CuAssertIntEquals(tc, 1, waypoint_add(route, 10.0, 10.0, 0.0, "cape"));
  CuAssertTrue(tc, autopilot_start(ship, route));
  entered_count = 0;
  for (ticks = 0;
       ticks < 2000 && (ship->autopilot->state != AUTOPILOT_COMPLETE || ship->speed > 0.0); ticks++)
  {
    movement_ticks(ship, 1);
  }
  CuAssertIntEquals(tc, AUTOPILOT_COMPLETE, ship->autopilot->state);
  CuAssertIntEquals(tc, probe_count, entered_count);
  for (i = 0; i < probe_count && i < entered_count; i++)
  {
    CuAssertIntEquals(tc, probe_x[i], entered_x[i]);
    CuAssertIntEquals(tc, probe_y[i], entered_y[i]);
  }

  /* With land at (11,0) the check refuses the route at that room. */
  ship->x = 0.0;
  ship->y = 0.0;
  ship->dx = 0.0;
  ship->dy = 0.0;
  ship->heading = 90.0;
  ship->setheading = 90;
  ship->speed = 17.0;
  refuse_room = TRUE;
  refused_x = 11;
  refused_y = 0;
  CuAssertTrue(tc, !scheduled_route_is_traversable(ship, &sound, &bad_waypoint, &bad_x, &bad_y));
  CuAssertPtrNotNull(tc, bad_waypoint);
  CuAssertIntEquals(tc, 11, bad_x);
  CuAssertIntEquals(tc, 0, bad_y);

  waypoint_list = saved_waypoints;
  movement_end(&fixture);
}

static MYSQL *movement_open_test_database(void)
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

void Test_vessel_restart_keeps_an_owned_hull_damaged_in_port(CuTest *tc)
{
  const char *enabled = getenv("LUMINARI_TEST_MYSQL_ENABLE");
  struct movement_fixture fixture;
  struct greyhawk_ship_data *ship;
  char query[256];
  MYSQL *saved_conn;
  MYSQL *connection;
  bool saved_mysql_available;
  bool prepared;

  if (enabled == NULL || strcmp(enabled, "1") != 0)
  {
    return;
  }

  connection = movement_open_test_database();
  if (connection == NULL)
  {
    CuFail(tc, "could not connect to the explicitly configured test database");
    return;
  }
  snprintf(query, sizeof(query), "INSERT INTO ship_interiors (ship_id, owner) VALUES (%d, 'Mara')",
           MOVEMENT_SHIP);
  prepared = mysql_query(connection, "CREATE TEMPORARY TABLE ship_interiors ("
                                     "ship_id INT NOT NULL PRIMARY KEY, "
                                     "owner VARCHAR(64) NOT NULL DEFAULT '')") == 0 &&
             mysql_query(connection, query) == 0;
  if (!prepared)
  {
    mysql_close(connection);
    CuFail(tc, "could not create the isolated ship interior fixture");
    return;
  }

  /* A restart finds an owned hull at rest in port, sail and rudder shot
   * about, with her owner not yet loaded: the harbor must not repair her. */
  ship = movement_begin(&fixture, VESSEL_WARSHIP);
  SET_BIT_AR(fixture.room.room_flags, ROOM_DOCKABLE);
  ship->mainsail = 1;
  ship->turnrate = 1;
  saved_conn = conn;
  saved_mysql_available = mysql_available;
  conn = connection;
  mysql_available = TRUE;
  vessel_db_restore_berth(ship);
  conn = saved_conn;
  mysql_available = saved_mysql_available;
  mysql_close(connection);

  CuAssertStrEquals(tc, "Mara", ship->owner);
  CuAssertIntEquals(tc, MOVEMENT_ROOM_VNUM, ship->dock);
  CuAssertIntEquals(tc, 1, ship->mainsail);
  CuAssertIntEquals(tc, 1, ship->turnrate);

  movement_end(&fixture);
}
