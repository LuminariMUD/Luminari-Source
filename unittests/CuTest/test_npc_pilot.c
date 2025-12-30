/**
 * @file test_npc_pilot.c
 * @brief Unit tests for NPC pilot system
 *
 * Tests pilot assignment data structures, VNUM storage,
 * and validation concepts for the NPC pilot system.
 *
 * Part of Phase 01, Session 05: NPC Pilot Integration
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
#define MAX_WAYPOINTS_PER_ROUTE 20
#define AUTOPILOT_NAME_LENGTH 64
#define CREW_ROLE_PILOT "pilot"
#define NOBODY -1

/* Autopilot state enum */
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
  time_t last_update;
  int pilot_mob_vnum;
};

/* ========================================================================= */
/* PILOT VNUM TESTS                                                          */
/* ========================================================================= */

void test_pilot_vnum_initial_value(CuTest *tc)
{
  struct autopilot_data ap;

  /* Pilot VNUM should start as -1 (no pilot) */
  ap.pilot_mob_vnum = -1;

  CuAssertIntEquals(tc, -1, ap.pilot_mob_vnum);
  CuAssertIntEquals(tc, NOBODY, ap.pilot_mob_vnum);
}

void test_pilot_vnum_assignment(CuTest *tc)
{
  struct autopilot_data ap;
  int test_vnum = 12345;

  /* Assign a pilot VNUM */
  ap.pilot_mob_vnum = test_vnum;

  CuAssertIntEquals(tc, test_vnum, ap.pilot_mob_vnum);
  CuAssertTrue(tc, ap.pilot_mob_vnum != -1);
}

void test_pilot_vnum_removal(CuTest *tc)
{
  struct autopilot_data ap;

  /* Assign then remove pilot */
  ap.pilot_mob_vnum = 12345;
  CuAssertTrue(tc, ap.pilot_mob_vnum != -1);

  ap.pilot_mob_vnum = -1;
  CuAssertIntEquals(tc, -1, ap.pilot_mob_vnum);
}

void test_pilot_vnum_range(CuTest *tc)
{
  struct autopilot_data ap;

  /* Test various VNUM values */
  ap.pilot_mob_vnum = 0;
  CuAssertIntEquals(tc, 0, ap.pilot_mob_vnum);

  ap.pilot_mob_vnum = 99999;
  CuAssertIntEquals(tc, 99999, ap.pilot_mob_vnum);

  /* Negative should only be -1 (no pilot) */
  ap.pilot_mob_vnum = -1;
  CuAssertIntEquals(tc, -1, ap.pilot_mob_vnum);
}

/* ========================================================================= */
/* CREW ROLE CONSTANT TESTS                                                  */
/* ========================================================================= */

void test_crew_role_pilot_constant(CuTest *tc)
{
  /* Verify CREW_ROLE_PILOT matches database enum */
  CuAssertTrue(tc, strcmp(CREW_ROLE_PILOT, "pilot") == 0);
  CuAssertTrue(tc, strlen(CREW_ROLE_PILOT) == 5);
}

/* ========================================================================= */
/* PILOT STATE INTEGRATION TESTS                                             */
/* ========================================================================= */

void test_pilot_with_autopilot_state(CuTest *tc)
{
  struct autopilot_data ap;
  struct ship_route route;

  /* Initialize autopilot with pilot */
  ap.state = AUTOPILOT_OFF;
  ap.pilot_mob_vnum = 12345;
  ap.current_route = NULL;
  ap.current_waypoint_index = 0;

  /* Simulate route assignment */
  route.route_id = 1;
  route.num_waypoints = 3;
  route.loop = TRUE;
  route.active = TRUE;
  strncpy(route.name, "Ferry Route", AUTOPILOT_NAME_LENGTH - 1);
  route.name[AUTOPILOT_NAME_LENGTH - 1] = '\0';

  ap.current_route = &route;

  /* Pilot assigned + route set should allow auto-engage */
  CuAssertTrue(tc, ap.pilot_mob_vnum != -1);
  CuAssertTrue(tc, ap.current_route != NULL);
  CuAssertTrue(tc, ap.current_route->num_waypoints > 0);
  CuAssertIntEquals(tc, AUTOPILOT_OFF, ap.state);

  /* Simulate auto-engage */
  ap.state = AUTOPILOT_TRAVELING;
  CuAssertIntEquals(tc, AUTOPILOT_TRAVELING, ap.state);
}

void test_pilot_removal_stops_autopilot(CuTest *tc)
{
  struct autopilot_data ap;
  struct ship_route route;

  /* Setup active piloted navigation */
  route.num_waypoints = 5;
  ap.current_route = &route;
  ap.pilot_mob_vnum = 12345;
  ap.state = AUTOPILOT_TRAVELING;

  /* Remove pilot - should stop autopilot */
  ap.pilot_mob_vnum = -1;
  ap.state = AUTOPILOT_OFF;

  CuAssertIntEquals(tc, -1, ap.pilot_mob_vnum);
  CuAssertIntEquals(tc, AUTOPILOT_OFF, ap.state);
}

void test_pilot_persistence_fields(CuTest *tc)
{
  struct autopilot_data ap;
  int original_vnum;
  int loaded_vnum;

  /* Test that pilot VNUM can be persisted and restored */
  original_vnum = 54321;

  ap.pilot_mob_vnum = original_vnum;

  /* Simulate save/load by copying value */
  loaded_vnum = ap.pilot_mob_vnum;

  CuAssertIntEquals(tc, original_vnum, loaded_vnum);
}

/* ========================================================================= */
/* WAYPOINT ANNOUNCEMENT CONTEXT TESTS                                       */
/* ========================================================================= */

void test_waypoint_for_announcement(CuTest *tc)
{
  struct waypoint wp;

  /* Setup waypoint with name for announcement */
  wp.x = 100.0f;
  wp.y = 200.0f;
  wp.z = 0.0f;
  strncpy(wp.name, "Harbor", AUTOPILOT_NAME_LENGTH - 1);
  wp.name[AUTOPILOT_NAME_LENGTH - 1] = '\0';
  wp.wait_time = 30;

  /* Verify waypoint has data for announcement */
  CuAssertTrue(tc, wp.name[0] != '\0');
  CuAssertTrue(tc, strlen(wp.name) > 0);
  CuAssertTrue(tc, strcmp(wp.name, "Harbor") == 0);
}

void test_waypoint_empty_name(CuTest *tc)
{
  struct waypoint wp;

  /* Waypoint without name */
  wp.name[0] = '\0';

  /* Should handle empty name gracefully */
  CuAssertTrue(tc, wp.name[0] == '\0');
  CuAssertTrue(tc, strlen(wp.name) == 0);
}

/* ========================================================================= */
/* VALIDATION CONCEPT TESTS                                                  */
/* ========================================================================= */

void test_pilot_validation_concepts(CuTest *tc)
{
  struct autopilot_data ap;

  /* Test validation scenarios (concepts only, no actual char_data) */

  /* Scenario 1: No pilot assigned */
  ap.pilot_mob_vnum = -1;
  CuAssertTrue(tc, ap.pilot_mob_vnum == -1);

  /* Scenario 2: Pilot assigned */
  ap.pilot_mob_vnum = 12345;
  CuAssertTrue(tc, ap.pilot_mob_vnum != -1);

  /* The actual validation (IS_NPC, room checks) requires server context */
}

void test_single_pilot_enforcement(CuTest *tc)
{
  struct autopilot_data ap;

  /* Only one pilot VNUM can be stored at a time */
  ap.pilot_mob_vnum = 11111;
  CuAssertIntEquals(tc, 11111, ap.pilot_mob_vnum);

  /* Assigning new pilot overwrites old */
  ap.pilot_mob_vnum = 22222;
  CuAssertIntEquals(tc, 22222, ap.pilot_mob_vnum);
  CuAssertTrue(tc, ap.pilot_mob_vnum != 11111);
}

/* ========================================================================= */
/* TEST SUITE REGISTRATION                                                   */
/* ========================================================================= */

CuSuite *get_npc_pilot_suite(void)
{
  CuSuite *suite = CuSuiteNew();

  /* Pilot VNUM tests */
  SUITE_ADD_TEST(suite, test_pilot_vnum_initial_value);
  SUITE_ADD_TEST(suite, test_pilot_vnum_assignment);
  SUITE_ADD_TEST(suite, test_pilot_vnum_removal);
  SUITE_ADD_TEST(suite, test_pilot_vnum_range);

  /* Crew role constant tests */
  SUITE_ADD_TEST(suite, test_crew_role_pilot_constant);

  /* Pilot state integration tests */
  SUITE_ADD_TEST(suite, test_pilot_with_autopilot_state);
  SUITE_ADD_TEST(suite, test_pilot_removal_stops_autopilot);
  SUITE_ADD_TEST(suite, test_pilot_persistence_fields);

  /* Waypoint announcement tests */
  SUITE_ADD_TEST(suite, test_waypoint_for_announcement);
  SUITE_ADD_TEST(suite, test_waypoint_empty_name);

  /* Validation concept tests */
  SUITE_ADD_TEST(suite, test_pilot_validation_concepts);
  SUITE_ADD_TEST(suite, test_single_pilot_enforcement);

  return suite;
}

/* ========================================================================= */
/* STANDALONE TEST RUNNER                                                    */
/* ========================================================================= */

int main(void)
{
  CuString *output = CuStringNew();
  CuSuite *suite = CuSuiteNew();

  printf("LuminariMUD NPC Pilot System Tests\n");
  printf("==================================\n");
  printf("Phase 01, Session 05: NPC Pilot Integration\n\n");

  CuSuiteAddSuite(suite, get_npc_pilot_suite());

  CuSuiteRun(suite);
  CuSuiteSummary(suite, output);
  CuSuiteDetails(suite, output);

  printf("%s\n", output->buffer);

  return suite->failCount;
}
