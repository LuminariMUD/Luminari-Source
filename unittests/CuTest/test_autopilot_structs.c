/**
 * @file test_autopilot_structs.c
 * @brief Unit tests for autopilot data structures
 *
 * Tests structure definitions, sizes, and field accessibility
 * for the autopilot system introduced in Phase 01, Session 01.
 *
 * Part of Phase 01, Session 01: Autopilot Data Structures
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
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

/* Autopilot constants */
#define MAX_WAYPOINTS_PER_ROUTE     20
#define MAX_ROUTES_PER_SHIP         5
#define AUTOPILOT_TICK_INTERVAL     5
#define AUTOPILOT_NAME_LENGTH       64

/* Autopilot state enum */
enum autopilot_state {
  AUTOPILOT_OFF,
  AUTOPILOT_TRAVELING,
  AUTOPILOT_WAITING,
  AUTOPILOT_PAUSED,
  AUTOPILOT_COMPLETE
};

/* Waypoint structure */
struct waypoint {
  float x;
  float y;
  float z;
  char name[AUTOPILOT_NAME_LENGTH];
  float tolerance;
  int wait_time;
  int flags;
};

/* Ship route structure */
struct ship_route {
  int route_id;
  char name[AUTOPILOT_NAME_LENGTH];
  struct waypoint waypoints[MAX_WAYPOINTS_PER_ROUTE];
  int num_waypoints;
  bool loop;
  bool active;
};

/* Autopilot data structure */
struct autopilot_data {
  enum autopilot_state state;
  struct ship_route *current_route;
  int current_waypoint_index;
  int tick_counter;
  int wait_remaining;
  time_t last_update;
  int pilot_mob_vnum;
};

/* ========================================================================= */
/* CONSTANT TESTS                                                            */
/* ========================================================================= */

void test_autopilot_constants(CuTest *tc)
{
  /* Verify constants have expected values */
  CuAssertIntEquals(tc, 20, MAX_WAYPOINTS_PER_ROUTE);
  CuAssertIntEquals(tc, 5, MAX_ROUTES_PER_SHIP);
  CuAssertIntEquals(tc, 5, AUTOPILOT_TICK_INTERVAL);
  CuAssertIntEquals(tc, 64, AUTOPILOT_NAME_LENGTH);
}

/* ========================================================================= */
/* ENUM TESTS                                                                */
/* ========================================================================= */

void test_autopilot_state_enum(CuTest *tc)
{
  enum autopilot_state state;

  /* Verify enum values */
  CuAssertIntEquals(tc, 0, AUTOPILOT_OFF);
  CuAssertIntEquals(tc, 1, AUTOPILOT_TRAVELING);
  CuAssertIntEquals(tc, 2, AUTOPILOT_WAITING);
  CuAssertIntEquals(tc, 3, AUTOPILOT_PAUSED);
  CuAssertIntEquals(tc, 4, AUTOPILOT_COMPLETE);

  /* Verify enum fits in int */
  state = AUTOPILOT_TRAVELING;
  CuAssertTrue(tc, sizeof(state) <= sizeof(int));
}

/* ========================================================================= */
/* WAYPOINT STRUCTURE TESTS                                                  */
/* ========================================================================= */

void test_waypoint_struct_size(CuTest *tc)
{
  /* Waypoint should be reasonably sized */
  size_t size = sizeof(struct waypoint);
  CuAssertTrue(tc, size > 0);
  CuAssertTrue(tc, size < 256);  /* Should be well under 256 bytes */
}

void test_waypoint_field_accessibility(CuTest *tc)
{
  struct waypoint wp;

  /* Test that all fields are accessible */
  wp.x = 100.5f;
  wp.y = -200.25f;
  wp.z = 50.0f;
  wp.tolerance = 5.0f;
  wp.wait_time = 10;
  wp.flags = 0;
  strncpy(wp.name, "Test Waypoint", AUTOPILOT_NAME_LENGTH - 1);
  wp.name[AUTOPILOT_NAME_LENGTH - 1] = '\0';

  CuAssertTrue(tc, wp.x > 100.0f && wp.x < 101.0f);
  CuAssertTrue(tc, wp.y < -200.0f && wp.y > -201.0f);
  CuAssertTrue(tc, wp.z > 49.0f && wp.z < 51.0f);
  CuAssertTrue(tc, wp.tolerance > 4.0f && wp.tolerance < 6.0f);
  CuAssertIntEquals(tc, 10, wp.wait_time);
  CuAssertIntEquals(tc, 0, wp.flags);
  CuAssertTrue(tc, strcmp(wp.name, "Test Waypoint") == 0);
}

void test_waypoint_coordinate_range(CuTest *tc)
{
  struct waypoint wp;

  /* Test boundary coordinates */
  wp.x = -1024.0f;
  wp.y = 1024.0f;
  wp.z = 0.0f;

  CuAssertTrue(tc, wp.x < -1023.0f);
  CuAssertTrue(tc, wp.y > 1023.0f);
  CuAssertTrue(tc, wp.z > -1.0f && wp.z < 1.0f);
}

/* ========================================================================= */
/* SHIP ROUTE STRUCTURE TESTS                                                */
/* ========================================================================= */

void test_ship_route_struct_size(CuTest *tc)
{
  size_t size = sizeof(struct ship_route);
  CuAssertTrue(tc, size > 0);
  /* Route contains 20 waypoints, should be substantial but reasonable */
  CuAssertTrue(tc, size < 4096);  /* Should be under 4KB */
}

void test_ship_route_field_accessibility(CuTest *tc)
{
  struct ship_route route;

  /* Test that all fields are accessible */
  route.route_id = 42;
  strncpy(route.name, "Test Route", AUTOPILOT_NAME_LENGTH - 1);
  route.name[AUTOPILOT_NAME_LENGTH - 1] = '\0';
  route.num_waypoints = 0;
  route.loop = TRUE;
  route.active = TRUE;

  CuAssertIntEquals(tc, 42, route.route_id);
  CuAssertTrue(tc, strcmp(route.name, "Test Route") == 0);
  CuAssertIntEquals(tc, 0, route.num_waypoints);
  CuAssertIntEquals(tc, TRUE, route.loop);
  CuAssertIntEquals(tc, TRUE, route.active);
}

void test_ship_route_max_waypoints(CuTest *tc)
{
  struct ship_route route;
  int i;

  /* Verify we can access all waypoint slots */
  route.num_waypoints = MAX_WAYPOINTS_PER_ROUTE;

  for (i = 0; i < MAX_WAYPOINTS_PER_ROUTE; i++)
  {
    route.waypoints[i].x = (float)i;
    route.waypoints[i].y = (float)(i * 10);
    route.waypoints[i].z = 0.0f;
  }

  /* Verify first and last */
  CuAssertTrue(tc, route.waypoints[0].x < 1.0f);
  CuAssertTrue(tc, route.waypoints[MAX_WAYPOINTS_PER_ROUTE - 1].x > 18.0f);
}

void test_ship_route_empty(CuTest *tc)
{
  struct ship_route route;

  /* Test empty route */
  route.num_waypoints = 0;
  route.loop = FALSE;
  route.active = FALSE;

  CuAssertIntEquals(tc, 0, route.num_waypoints);
  CuAssertIntEquals(tc, FALSE, route.loop);
  CuAssertIntEquals(tc, FALSE, route.active);
}

/* ========================================================================= */
/* AUTOPILOT DATA STRUCTURE TESTS                                            */
/* ========================================================================= */

void test_autopilot_data_struct_size(CuTest *tc)
{
  size_t size = sizeof(struct autopilot_data);
  CuAssertTrue(tc, size > 0);
  /* Autopilot data should be compact (target <1KB) */
  CuAssertTrue(tc, size < 128);  /* Should be well under 128 bytes */
}

void test_autopilot_data_field_accessibility(CuTest *tc)
{
  struct autopilot_data ap;

  /* Test that all fields are accessible */
  ap.state = AUTOPILOT_OFF;
  ap.current_route = NULL;
  ap.current_waypoint_index = 0;
  ap.tick_counter = 0;
  ap.wait_remaining = 0;
  ap.last_update = time(0);
  ap.pilot_mob_vnum = -1;

  CuAssertIntEquals(tc, AUTOPILOT_OFF, ap.state);
  CuAssertPtrEquals(tc, NULL, ap.current_route);
  CuAssertIntEquals(tc, 0, ap.current_waypoint_index);
  CuAssertIntEquals(tc, 0, ap.tick_counter);
  CuAssertIntEquals(tc, 0, ap.wait_remaining);
  CuAssertTrue(tc, ap.last_update > 0);
  CuAssertIntEquals(tc, -1, ap.pilot_mob_vnum);
}

void test_autopilot_data_state_transitions(CuTest *tc)
{
  struct autopilot_data ap;

  /* Test state transitions */
  ap.state = AUTOPILOT_OFF;
  CuAssertIntEquals(tc, AUTOPILOT_OFF, ap.state);

  ap.state = AUTOPILOT_TRAVELING;
  CuAssertIntEquals(tc, AUTOPILOT_TRAVELING, ap.state);

  ap.state = AUTOPILOT_WAITING;
  CuAssertIntEquals(tc, AUTOPILOT_WAITING, ap.state);

  ap.state = AUTOPILOT_PAUSED;
  CuAssertIntEquals(tc, AUTOPILOT_PAUSED, ap.state);

  ap.state = AUTOPILOT_COMPLETE;
  CuAssertIntEquals(tc, AUTOPILOT_COMPLETE, ap.state);
}

void test_autopilot_route_pointer(CuTest *tc)
{
  struct autopilot_data ap;
  struct ship_route route;

  /* Initialize route */
  route.route_id = 1;
  route.num_waypoints = 5;

  /* Test pointer assignment */
  ap.current_route = NULL;
  CuAssertPtrEquals(tc, NULL, ap.current_route);

  ap.current_route = &route;
  CuAssertTrue(tc, ap.current_route != NULL);
  CuAssertIntEquals(tc, 1, ap.current_route->route_id);
  CuAssertIntEquals(tc, 5, ap.current_route->num_waypoints);
}

/* ========================================================================= */
/* MEMORY EFFICIENCY TESTS                                                   */
/* ========================================================================= */

void test_memory_efficiency(CuTest *tc)
{
  size_t waypoint_size = sizeof(struct waypoint);
  size_t route_size = sizeof(struct ship_route);
  size_t autopilot_size = sizeof(struct autopilot_data);
  size_t total_per_ship = autopilot_size;

  /* Print sizes for debugging */
  printf("\n  Autopilot structure sizes:\n");
  printf("    struct waypoint: %lu bytes\n", (unsigned long)waypoint_size);
  printf("    struct ship_route: %lu bytes\n", (unsigned long)route_size);
  printf("    struct autopilot_data: %lu bytes\n", (unsigned long)autopilot_size);
  printf("    Per-ship overhead: %lu bytes\n", (unsigned long)total_per_ship);

  /* Verify memory target: autopilot_data should be compact */
  CuAssertTrue(tc, autopilot_size < 128);

  /* Routes are stored separately, not per-ship */
  CuAssertTrue(tc, route_size < 4096);
}

/* ========================================================================= */
/* TEST SUITE REGISTRATION                                                   */
/* ========================================================================= */

CuSuite *get_autopilot_struct_suite(void)
{
  CuSuite *suite = CuSuiteNew();

  /* Constant tests */
  SUITE_ADD_TEST(suite, test_autopilot_constants);

  /* Enum tests */
  SUITE_ADD_TEST(suite, test_autopilot_state_enum);

  /* Waypoint tests */
  SUITE_ADD_TEST(suite, test_waypoint_struct_size);
  SUITE_ADD_TEST(suite, test_waypoint_field_accessibility);
  SUITE_ADD_TEST(suite, test_waypoint_coordinate_range);

  /* Ship route tests */
  SUITE_ADD_TEST(suite, test_ship_route_struct_size);
  SUITE_ADD_TEST(suite, test_ship_route_field_accessibility);
  SUITE_ADD_TEST(suite, test_ship_route_max_waypoints);
  SUITE_ADD_TEST(suite, test_ship_route_empty);

  /* Autopilot data tests */
  SUITE_ADD_TEST(suite, test_autopilot_data_struct_size);
  SUITE_ADD_TEST(suite, test_autopilot_data_field_accessibility);
  SUITE_ADD_TEST(suite, test_autopilot_data_state_transitions);
  SUITE_ADD_TEST(suite, test_autopilot_route_pointer);

  /* Memory tests */
  SUITE_ADD_TEST(suite, test_memory_efficiency);

  return suite;
}

/* ========================================================================= */
/* STANDALONE TEST RUNNER                                                    */
/* ========================================================================= */

int main(void)
{
  CuString *output = CuStringNew();
  CuSuite *suite = CuSuiteNew();

  printf("LuminariMUD Autopilot Structure Tests\n");
  printf("=====================================\n");
  printf("Phase 01, Session 01: Autopilot Data Structures\n\n");

  CuSuiteAddSuite(suite, get_autopilot_struct_suite());

  CuSuiteRun(suite);
  CuSuiteSummary(suite, output);
  CuSuiteDetails(suite, output);

  printf("%s\n", output->buffer);

  return suite->failCount;
}
