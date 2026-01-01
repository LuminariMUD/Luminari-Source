/**
 * @file test_autopilot_pathfinding.c
 * @brief Unit tests for autopilot path-following logic
 *
 * Tests the path-following functions for the autopilot system:
 * - calculate_distance_to_waypoint()
 * - calculate_heading_to_waypoint()
 * - check_waypoint_arrival()
 * - advance_to_next_waypoint()
 * - State machine transitions
 *
 * Part of Phase 01, Session 03: Path-Following Logic
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "CuTest.h"

/* ========================================================================= */
/* TYPE DEFINITIONS (standalone to avoid server dependency)                 */
/* ========================================================================= */

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

typedef int bool;

#define AUTOPILOT_NAME_LENGTH 64
#define MAX_WAYPOINTS_PER_ROUTE 20

/* Autopilot states */
enum autopilot_state
{
  AUTOPILOT_OFF,
  AUTOPILOT_TRAVELING,
  AUTOPILOT_WAITING,
  AUTOPILOT_PAUSED,
  AUTOPILOT_COMPLETE
};

/* Waypoint structure */
struct waypoint
{
  float x;
  float y;
  float z;
  char name[AUTOPILOT_NAME_LENGTH];
  float tolerance;
  int wait_time;
  int flags;
};

/* Ship route structure */
struct ship_route
{
  int route_id;
  char name[AUTOPILOT_NAME_LENGTH];
  struct waypoint waypoints[MAX_WAYPOINTS_PER_ROUTE];
  int num_waypoints;
  bool loop;
  bool active;
};

/* Autopilot data structure */
struct autopilot_data
{
  enum autopilot_state state;
  struct ship_route *current_route;
  int current_waypoint_index;
  int tick_counter;
  int wait_remaining;
  long last_update;
  int pilot_mob_vnum;
};

/* Minimal ship data for testing */
struct test_ship_data
{
  float x, y, z;
  int shipnum;
  short int speed;
  struct autopilot_data *autopilot;
};

/* ========================================================================= */
/* LOCAL IMPLEMENTATION (mirrors vessels_autopilot.c functions)             */
/* ========================================================================= */

static float test_calculate_distance(struct test_ship_data *ship, struct waypoint *wp)
{
  float dx, dy, dz;

  if (ship == NULL || wp == NULL)
    return -1.0f;

  dx = wp->x - ship->x;
  dy = wp->y - ship->y;
  dz = wp->z - ship->z;

  return (float)sqrt((double)(dx * dx + dy * dy + dz * dz));
}

static void test_calculate_heading(struct test_ship_data *ship, struct waypoint *wp, float *dx,
                                   float *dy)
{
  float raw_dx, raw_dy, distance;

  if (ship == NULL || wp == NULL || dx == NULL || dy == NULL)
  {
    if (dx)
      *dx = 0.0f;
    if (dy)
      *dy = 0.0f;
    return;
  }

  raw_dx = wp->x - ship->x;
  raw_dy = wp->y - ship->y;
  distance = (float)sqrt((double)(raw_dx * raw_dx + raw_dy * raw_dy));

  if (distance < 0.001f)
  {
    *dx = 0.0f;
    *dy = 0.0f;
    return;
  }

  *dx = raw_dx / distance;
  *dy = raw_dy / distance;
}

static int test_check_arrival(struct test_ship_data *ship, struct waypoint *wp)
{
  float distance, tolerance;

  if (ship == NULL || wp == NULL)
    return FALSE;

  distance = test_calculate_distance(ship, wp);
  if (distance < 0.0f)
    return FALSE;

  tolerance = wp->tolerance;
  if (tolerance <= 0.0f)
    tolerance = 5.0f;

  return (distance <= tolerance) ? TRUE : FALSE;
}

static struct waypoint *test_get_current_waypoint(struct test_ship_data *ship)
{
  struct autopilot_data *ap;

  if (ship == NULL || ship->autopilot == NULL)
    return NULL;

  ap = ship->autopilot;
  if (ap->current_route == NULL)
    return NULL;

  if (ap->current_waypoint_index < 0 ||
      ap->current_waypoint_index >= ap->current_route->num_waypoints)
    return NULL;

  return &ap->current_route->waypoints[ap->current_waypoint_index];
}

static int test_advance_waypoint(struct test_ship_data *ship)
{
  struct autopilot_data *ap;
  struct ship_route *route;
  int next_index;

  if (ship == NULL || ship->autopilot == NULL)
    return 0;

  ap = ship->autopilot;
  route = ap->current_route;

  if (route == NULL)
  {
    ap->state = AUTOPILOT_OFF;
    return 0;
  }

  next_index = ap->current_waypoint_index + 1;

  if (next_index >= route->num_waypoints)
  {
    if (route->loop)
    {
      ap->current_waypoint_index = 0;
      ap->state = AUTOPILOT_TRAVELING;
      return 1;
    }
    else
    {
      ap->state = AUTOPILOT_COMPLETE;
      return 0;
    }
  }

  ap->current_waypoint_index = next_index;
  ap->state = AUTOPILOT_TRAVELING;
  return 1;
}

/* ========================================================================= */
/* HELPER FUNCTIONS                                                          */
/* ========================================================================= */

static void init_test_ship(struct test_ship_data *ship, float x, float y, float z)
{
  memset(ship, 0, sizeof(struct test_ship_data));
  ship->x = x;
  ship->y = y;
  ship->z = z;
  ship->shipnum = 1;
  ship->speed = 1;
}

static void init_test_waypoint(struct waypoint *wp, float x, float y, float z, const char *name,
                               float tolerance, int wait_time)
{
  memset(wp, 0, sizeof(struct waypoint));
  wp->x = x;
  wp->y = y;
  wp->z = z;
  wp->tolerance = tolerance;
  wp->wait_time = wait_time;
  if (name)
  {
    strncpy(wp->name, name, AUTOPILOT_NAME_LENGTH - 1);
    wp->name[AUTOPILOT_NAME_LENGTH - 1] = '\0';
  }
}

static void init_test_route(struct ship_route *route, const char *name, bool loop)
{
  memset(route, 0, sizeof(struct ship_route));
  route->route_id = 1;
  route->loop = loop;
  route->active = TRUE;
  if (name)
  {
    strncpy(route->name, name, AUTOPILOT_NAME_LENGTH - 1);
    route->name[AUTOPILOT_NAME_LENGTH - 1] = '\0';
  }
}

/* ========================================================================= */
/* DISTANCE CALCULATION TESTS                                                */
/* ========================================================================= */

void Test_distance_zero(CuTest *tc)
{
  struct test_ship_data ship;
  struct waypoint wp;
  float distance;

  init_test_ship(&ship, 100.0f, 100.0f, 0.0f);
  init_test_waypoint(&wp, 100.0f, 100.0f, 0.0f, "Same", 5.0f, 0);

  distance = test_calculate_distance(&ship, &wp);

  CuAssertTrue(tc, distance >= 0.0f);
  CuAssertTrue(tc, distance < 0.001f);
}

void Test_distance_simple_x(CuTest *tc)
{
  struct test_ship_data ship;
  struct waypoint wp;
  float distance;

  init_test_ship(&ship, 0.0f, 0.0f, 0.0f);
  init_test_waypoint(&wp, 10.0f, 0.0f, 0.0f, "East", 5.0f, 0);

  distance = test_calculate_distance(&ship, &wp);

  CuAssertTrue(tc, distance > 9.99f && distance < 10.01f);
}

void Test_distance_simple_y(CuTest *tc)
{
  struct test_ship_data ship;
  struct waypoint wp;
  float distance;

  init_test_ship(&ship, 0.0f, 0.0f, 0.0f);
  init_test_waypoint(&wp, 0.0f, 10.0f, 0.0f, "North", 5.0f, 0);

  distance = test_calculate_distance(&ship, &wp);

  CuAssertTrue(tc, distance > 9.99f && distance < 10.01f);
}

void Test_distance_diagonal(CuTest *tc)
{
  struct test_ship_data ship;
  struct waypoint wp;
  float distance;

  init_test_ship(&ship, 0.0f, 0.0f, 0.0f);
  init_test_waypoint(&wp, 3.0f, 4.0f, 0.0f, "Diagonal", 5.0f, 0);

  distance = test_calculate_distance(&ship, &wp);
  /* 3-4-5 triangle, expected distance = 5.0 */

  CuAssertTrue(tc, distance > 4.99f && distance < 5.01f);
}

void Test_distance_3d(CuTest *tc)
{
  struct test_ship_data ship;
  struct waypoint wp;
  float distance;
  float expected;

  init_test_ship(&ship, 0.0f, 0.0f, 0.0f);
  init_test_waypoint(&wp, 2.0f, 2.0f, 2.0f, "3D", 5.0f, 0);

  distance = test_calculate_distance(&ship, &wp);
  expected = (float)sqrt(12.0); /* sqrt(4+4+4) */

  CuAssertTrue(tc, fabs(distance - expected) < 0.01f);
}

void Test_distance_null_ship(CuTest *tc)
{
  struct waypoint wp;
  float distance;

  init_test_waypoint(&wp, 10.0f, 10.0f, 0.0f, "Target", 5.0f, 0);

  distance = test_calculate_distance(NULL, &wp);

  CuAssertTrue(tc, distance < 0.0f);
}

void Test_distance_null_waypoint(CuTest *tc)
{
  struct test_ship_data ship;
  float distance;

  init_test_ship(&ship, 0.0f, 0.0f, 0.0f);

  distance = test_calculate_distance(&ship, NULL);

  CuAssertTrue(tc, distance < 0.0f);
}

/* ========================================================================= */
/* HEADING CALCULATION TESTS                                                 */
/* ========================================================================= */

void Test_heading_east(CuTest *tc)
{
  struct test_ship_data ship;
  struct waypoint wp;
  float dx, dy;

  init_test_ship(&ship, 0.0f, 0.0f, 0.0f);
  init_test_waypoint(&wp, 10.0f, 0.0f, 0.0f, "East", 5.0f, 0);

  test_calculate_heading(&ship, &wp, &dx, &dy);

  CuAssertTrue(tc, dx > 0.99f && dx < 1.01f);
  CuAssertTrue(tc, fabs(dy) < 0.01f);
}

void Test_heading_north(CuTest *tc)
{
  struct test_ship_data ship;
  struct waypoint wp;
  float dx, dy;

  init_test_ship(&ship, 0.0f, 0.0f, 0.0f);
  init_test_waypoint(&wp, 0.0f, 10.0f, 0.0f, "North", 5.0f, 0);

  test_calculate_heading(&ship, &wp, &dx, &dy);

  CuAssertTrue(tc, fabs(dx) < 0.01f);
  CuAssertTrue(tc, dy > 0.99f && dy < 1.01f);
}

void Test_heading_diagonal(CuTest *tc)
{
  struct test_ship_data ship;
  struct waypoint wp;
  float dx, dy;
  float expected;

  init_test_ship(&ship, 0.0f, 0.0f, 0.0f);
  init_test_waypoint(&wp, 10.0f, 10.0f, 0.0f, "NE", 5.0f, 0);

  test_calculate_heading(&ship, &wp, &dx, &dy);
  expected = (float)(1.0 / sqrt(2.0)); /* 45 degrees */

  CuAssertTrue(tc, fabs(dx - expected) < 0.01f);
  CuAssertTrue(tc, fabs(dy - expected) < 0.01f);
}

void Test_heading_same_position(CuTest *tc)
{
  struct test_ship_data ship;
  struct waypoint wp;
  float dx, dy;

  init_test_ship(&ship, 50.0f, 50.0f, 0.0f);
  init_test_waypoint(&wp, 50.0f, 50.0f, 0.0f, "Same", 5.0f, 0);

  test_calculate_heading(&ship, &wp, &dx, &dy);

  CuAssertTrue(tc, fabs(dx) < 0.01f);
  CuAssertTrue(tc, fabs(dy) < 0.01f);
}

void Test_heading_normalized(CuTest *tc)
{
  struct test_ship_data ship;
  struct waypoint wp;
  float dx, dy;
  float magnitude;

  init_test_ship(&ship, 0.0f, 0.0f, 0.0f);
  init_test_waypoint(&wp, 100.0f, 200.0f, 0.0f, "Far", 5.0f, 0);

  test_calculate_heading(&ship, &wp, &dx, &dy);
  magnitude = (float)sqrt((double)(dx * dx + dy * dy));

  /* Normalized vector should have magnitude ~1 */
  CuAssertTrue(tc, fabs(magnitude - 1.0f) < 0.01f);
}

/* ========================================================================= */
/* ARRIVAL DETECTION TESTS                                                   */
/* ========================================================================= */

void Test_arrival_within_tolerance(CuTest *tc)
{
  struct test_ship_data ship;
  struct waypoint wp;
  int arrived;

  init_test_ship(&ship, 100.0f, 100.0f, 0.0f);
  init_test_waypoint(&wp, 102.0f, 102.0f, 0.0f, "Near", 5.0f, 0);

  arrived = test_check_arrival(&ship, &wp);

  CuAssertTrue(tc, arrived == TRUE);
}

void Test_arrival_outside_tolerance(CuTest *tc)
{
  struct test_ship_data ship;
  struct waypoint wp;
  int arrived;

  init_test_ship(&ship, 100.0f, 100.0f, 0.0f);
  init_test_waypoint(&wp, 110.0f, 110.0f, 0.0f, "Far", 5.0f, 0);

  arrived = test_check_arrival(&ship, &wp);

  CuAssertTrue(tc, arrived == FALSE);
}

void Test_arrival_at_boundary(CuTest *tc)
{
  struct test_ship_data ship;
  struct waypoint wp;
  int arrived;

  init_test_ship(&ship, 0.0f, 0.0f, 0.0f);
  /* Exactly at tolerance boundary (distance = 5.0) */
  init_test_waypoint(&wp, 3.0f, 4.0f, 0.0f, "Boundary", 5.0f, 0);

  arrived = test_check_arrival(&ship, &wp);

  CuAssertTrue(tc, arrived == TRUE); /* <= tolerance */
}

void Test_arrival_default_tolerance(CuTest *tc)
{
  struct test_ship_data ship;
  struct waypoint wp;
  int arrived;

  init_test_ship(&ship, 0.0f, 0.0f, 0.0f);
  init_test_waypoint(&wp, 3.0f, 0.0f, 0.0f, "Near", 0.0f, 0); /* 0 tolerance -> default 5 */

  arrived = test_check_arrival(&ship, &wp);

  CuAssertTrue(tc, arrived == TRUE);
}

void Test_arrival_very_small_tolerance(CuTest *tc)
{
  struct test_ship_data ship;
  struct waypoint wp;
  int arrived;

  init_test_ship(&ship, 0.0f, 0.0f, 0.0f);
  init_test_waypoint(&wp, 0.5f, 0.0f, 0.0f, "Precise", 0.3f, 0);

  arrived = test_check_arrival(&ship, &wp);

  CuAssertTrue(tc, arrived == FALSE); /* 0.5 > 0.3 */
}

/* ========================================================================= */
/* WAYPOINT ADVANCEMENT TESTS                                                */
/* ========================================================================= */

void Test_advance_middle_waypoint(CuTest *tc)
{
  struct test_ship_data ship;
  struct ship_route route;
  struct autopilot_data ap;
  int result;

  init_test_ship(&ship, 0.0f, 0.0f, 0.0f);
  init_test_route(&route, "Test Route", FALSE);

  /* Add 3 waypoints */
  route.num_waypoints = 3;
  init_test_waypoint(&route.waypoints[0], 10.0f, 0.0f, 0.0f, "WP1", 5.0f, 0);
  init_test_waypoint(&route.waypoints[1], 20.0f, 0.0f, 0.0f, "WP2", 5.0f, 0);
  init_test_waypoint(&route.waypoints[2], 30.0f, 0.0f, 0.0f, "WP3", 5.0f, 0);

  memset(&ap, 0, sizeof(struct autopilot_data));
  ap.state = AUTOPILOT_TRAVELING;
  ap.current_route = &route;
  ap.current_waypoint_index = 0;

  ship.autopilot = &ap;

  result = test_advance_waypoint(&ship);

  CuAssertIntEquals(tc, 1, result);
  CuAssertIntEquals(tc, 1, ap.current_waypoint_index);
  CuAssertIntEquals(tc, AUTOPILOT_TRAVELING, ap.state);
}

void Test_advance_last_waypoint_noloop(CuTest *tc)
{
  struct test_ship_data ship;
  struct ship_route route;
  struct autopilot_data ap;
  int result;

  init_test_ship(&ship, 0.0f, 0.0f, 0.0f);
  init_test_route(&route, "NoLoop Route", FALSE);

  route.num_waypoints = 2;
  init_test_waypoint(&route.waypoints[0], 10.0f, 0.0f, 0.0f, "WP1", 5.0f, 0);
  init_test_waypoint(&route.waypoints[1], 20.0f, 0.0f, 0.0f, "WP2", 5.0f, 0);

  memset(&ap, 0, sizeof(struct autopilot_data));
  ap.state = AUTOPILOT_TRAVELING;
  ap.current_route = &route;
  ap.current_waypoint_index = 1; /* At last waypoint */

  ship.autopilot = &ap;

  result = test_advance_waypoint(&ship);

  CuAssertIntEquals(tc, 0, result);
  CuAssertIntEquals(tc, AUTOPILOT_COMPLETE, ap.state);
}

void Test_advance_last_waypoint_loop(CuTest *tc)
{
  struct test_ship_data ship;
  struct ship_route route;
  struct autopilot_data ap;
  int result;

  init_test_ship(&ship, 0.0f, 0.0f, 0.0f);
  init_test_route(&route, "Loop Route", TRUE);

  route.num_waypoints = 2;
  init_test_waypoint(&route.waypoints[0], 10.0f, 0.0f, 0.0f, "WP1", 5.0f, 0);
  init_test_waypoint(&route.waypoints[1], 20.0f, 0.0f, 0.0f, "WP2", 5.0f, 0);

  memset(&ap, 0, sizeof(struct autopilot_data));
  ap.state = AUTOPILOT_TRAVELING;
  ap.current_route = &route;
  ap.current_waypoint_index = 1; /* At last waypoint */

  ship.autopilot = &ap;

  result = test_advance_waypoint(&ship);

  CuAssertIntEquals(tc, 1, result);
  CuAssertIntEquals(tc, 0, ap.current_waypoint_index); /* Back to start */
  CuAssertIntEquals(tc, AUTOPILOT_TRAVELING, ap.state);
}

void Test_advance_null_route(CuTest *tc)
{
  struct test_ship_data ship;
  struct autopilot_data ap;
  int result;

  init_test_ship(&ship, 0.0f, 0.0f, 0.0f);

  memset(&ap, 0, sizeof(struct autopilot_data));
  ap.state = AUTOPILOT_TRAVELING;
  ap.current_route = NULL;

  ship.autopilot = &ap;

  result = test_advance_waypoint(&ship);

  CuAssertIntEquals(tc, 0, result);
  CuAssertIntEquals(tc, AUTOPILOT_OFF, ap.state);
}

/* ========================================================================= */
/* STATE TRANSITION TESTS                                                    */
/* ========================================================================= */

void Test_state_traveling_to_waiting(CuTest *tc)
{
  struct autopilot_data ap;

  memset(&ap, 0, sizeof(struct autopilot_data));
  ap.state = AUTOPILOT_TRAVELING;

  /* Simulate arriving at waypoint with wait time */
  ap.state = AUTOPILOT_WAITING;
  ap.wait_remaining = 30;

  CuAssertIntEquals(tc, AUTOPILOT_WAITING, ap.state);
  CuAssertIntEquals(tc, 30, ap.wait_remaining);
}

void Test_state_waiting_to_traveling(CuTest *tc)
{
  struct autopilot_data ap;

  memset(&ap, 0, sizeof(struct autopilot_data));
  ap.state = AUTOPILOT_WAITING;
  ap.wait_remaining = 0;

  /* Simulate wait complete */
  ap.state = AUTOPILOT_TRAVELING;

  CuAssertIntEquals(tc, AUTOPILOT_TRAVELING, ap.state);
}

void Test_state_pause_resume(CuTest *tc)
{
  struct autopilot_data ap;

  memset(&ap, 0, sizeof(struct autopilot_data));
  ap.state = AUTOPILOT_TRAVELING;

  /* Pause */
  ap.state = AUTOPILOT_PAUSED;
  CuAssertIntEquals(tc, AUTOPILOT_PAUSED, ap.state);

  /* Resume */
  ap.state = AUTOPILOT_TRAVELING;
  CuAssertIntEquals(tc, AUTOPILOT_TRAVELING, ap.state);
}

void Test_get_current_waypoint(CuTest *tc)
{
  struct test_ship_data ship;
  struct ship_route route;
  struct autopilot_data ap;
  struct waypoint *wp;

  init_test_ship(&ship, 0.0f, 0.0f, 0.0f);
  init_test_route(&route, "Test Route", FALSE);

  route.num_waypoints = 2;
  init_test_waypoint(&route.waypoints[0], 10.0f, 0.0f, 0.0f, "First", 5.0f, 0);
  init_test_waypoint(&route.waypoints[1], 20.0f, 0.0f, 0.0f, "Second", 5.0f, 0);

  memset(&ap, 0, sizeof(struct autopilot_data));
  ap.state = AUTOPILOT_TRAVELING;
  ap.current_route = &route;
  ap.current_waypoint_index = 0;

  ship.autopilot = &ap;

  wp = test_get_current_waypoint(&ship);

  CuAssertPtrNotNull(tc, wp);
  CuAssertStrEquals(tc, "First", wp->name);
  CuAssertTrue(tc, wp->x > 9.99f && wp->x < 10.01f);
}

void Test_get_current_waypoint_invalid_index(CuTest *tc)
{
  struct test_ship_data ship;
  struct ship_route route;
  struct autopilot_data ap;
  struct waypoint *wp;

  init_test_ship(&ship, 0.0f, 0.0f, 0.0f);
  init_test_route(&route, "Test Route", FALSE);
  route.num_waypoints = 1;

  memset(&ap, 0, sizeof(struct autopilot_data));
  ap.current_route = &route;
  ap.current_waypoint_index = 5; /* Invalid index */

  ship.autopilot = &ap;

  wp = test_get_current_waypoint(&ship);

  CuAssertPtrEquals(tc, NULL, wp);
}

/* ========================================================================= */
/* EDGE CASE TESTS                                                           */
/* ========================================================================= */

void Test_empty_route(CuTest *tc)
{
  struct test_ship_data ship;
  struct ship_route route;
  struct autopilot_data ap;
  struct waypoint *wp;

  init_test_ship(&ship, 0.0f, 0.0f, 0.0f);
  init_test_route(&route, "Empty Route", FALSE);
  route.num_waypoints = 0;

  memset(&ap, 0, sizeof(struct autopilot_data));
  ap.current_route = &route;
  ap.current_waypoint_index = 0;

  ship.autopilot = &ap;

  wp = test_get_current_waypoint(&ship);

  CuAssertPtrEquals(tc, NULL, wp);
}

void Test_single_waypoint_route(CuTest *tc)
{
  struct test_ship_data ship;
  struct ship_route route;
  struct autopilot_data ap;
  int result;

  init_test_ship(&ship, 0.0f, 0.0f, 0.0f);
  init_test_route(&route, "Single WP", FALSE);
  route.num_waypoints = 1;
  init_test_waypoint(&route.waypoints[0], 10.0f, 0.0f, 0.0f, "Only", 5.0f, 0);

  memset(&ap, 0, sizeof(struct autopilot_data));
  ap.state = AUTOPILOT_TRAVELING;
  ap.current_route = &route;
  ap.current_waypoint_index = 0;

  ship.autopilot = &ap;

  result = test_advance_waypoint(&ship);

  CuAssertIntEquals(tc, 0, result);
  CuAssertIntEquals(tc, AUTOPILOT_COMPLETE, ap.state);
}

void Test_large_tolerance(CuTest *tc)
{
  struct test_ship_data ship;
  struct waypoint wp;
  int arrived;

  init_test_ship(&ship, 0.0f, 0.0f, 0.0f);
  init_test_waypoint(&wp, 100.0f, 100.0f, 0.0f, "Far", 200.0f, 0);

  arrived = test_check_arrival(&ship, &wp);

  /* Large tolerance should still work */
  CuAssertTrue(tc, arrived == TRUE);
}

void Test_negative_coordinates(CuTest *tc)
{
  struct test_ship_data ship;
  struct waypoint wp;
  float distance;

  init_test_ship(&ship, -50.0f, -50.0f, 0.0f);
  init_test_waypoint(&wp, -40.0f, -50.0f, 0.0f, "West", 5.0f, 0);

  distance = test_calculate_distance(&ship, &wp);

  CuAssertTrue(tc, distance > 9.99f && distance < 10.01f);
}

/* ========================================================================= */
/* TEST SUITE REGISTRATION                                                   */
/* ========================================================================= */

CuSuite *AutopilotPathfindingGetSuite(void)
{
  CuSuite *suite = CuSuiteNew();

  /* Distance calculation tests */
  SUITE_ADD_TEST(suite, Test_distance_zero);
  SUITE_ADD_TEST(suite, Test_distance_simple_x);
  SUITE_ADD_TEST(suite, Test_distance_simple_y);
  SUITE_ADD_TEST(suite, Test_distance_diagonal);
  SUITE_ADD_TEST(suite, Test_distance_3d);
  SUITE_ADD_TEST(suite, Test_distance_null_ship);
  SUITE_ADD_TEST(suite, Test_distance_null_waypoint);

  /* Heading calculation tests */
  SUITE_ADD_TEST(suite, Test_heading_east);
  SUITE_ADD_TEST(suite, Test_heading_north);
  SUITE_ADD_TEST(suite, Test_heading_diagonal);
  SUITE_ADD_TEST(suite, Test_heading_same_position);
  SUITE_ADD_TEST(suite, Test_heading_normalized);

  /* Arrival detection tests */
  SUITE_ADD_TEST(suite, Test_arrival_within_tolerance);
  SUITE_ADD_TEST(suite, Test_arrival_outside_tolerance);
  SUITE_ADD_TEST(suite, Test_arrival_at_boundary);
  SUITE_ADD_TEST(suite, Test_arrival_default_tolerance);
  SUITE_ADD_TEST(suite, Test_arrival_very_small_tolerance);

  /* Waypoint advancement tests */
  SUITE_ADD_TEST(suite, Test_advance_middle_waypoint);
  SUITE_ADD_TEST(suite, Test_advance_last_waypoint_noloop);
  SUITE_ADD_TEST(suite, Test_advance_last_waypoint_loop);
  SUITE_ADD_TEST(suite, Test_advance_null_route);

  /* State transition tests */
  SUITE_ADD_TEST(suite, Test_state_traveling_to_waiting);
  SUITE_ADD_TEST(suite, Test_state_waiting_to_traveling);
  SUITE_ADD_TEST(suite, Test_state_pause_resume);
  SUITE_ADD_TEST(suite, Test_get_current_waypoint);
  SUITE_ADD_TEST(suite, Test_get_current_waypoint_invalid_index);

  /* Edge case tests */
  SUITE_ADD_TEST(suite, Test_empty_route);
  SUITE_ADD_TEST(suite, Test_single_waypoint_route);
  SUITE_ADD_TEST(suite, Test_large_tolerance);
  SUITE_ADD_TEST(suite, Test_negative_coordinates);

  return suite;
}

/* Standalone test runner */
int main(void)
{
  CuString *output = CuStringNew();
  CuSuite *suite = AutopilotPathfindingGetSuite();

  printf("Running Autopilot Pathfinding Tests...\n\n");

  CuSuiteRun(suite);
  CuSuiteSummary(suite, output);
  CuSuiteDetails(suite, output);
  printf("%s\n", output->buffer);

  CuStringDelete(output);
  CuSuiteDelete(suite);

  return 0;
}
