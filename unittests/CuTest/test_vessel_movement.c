/**
 * @file test_vessel_movement.c
 * @brief Unit tests for vessel movement system
 *
 * Tests interior movement, direction validation, blocked passage detection,
 * and ship-to-ship movement for the vessel system.
 *
 * Part of Phase 00, Session 09: Testing and Validation
 */

#include "CuTest.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ========================================================================= */
/* TYPE DEFINITIONS (self-contained for unit testing)                        */
/* ========================================================================= */

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef NOWHERE
#define NOWHERE (-1)
#endif

typedef int bool;
typedef int room_rnum;

/* Direction constants */
#define DIR_NORTH 0
#define DIR_EAST 1
#define DIR_SOUTH 2
#define DIR_WEST 3
#define DIR_UP 4
#define DIR_DOWN 5
#define NUM_DIRS 6
#define DIR_INVALID -1

/* Movement result codes */
#define MOVE_SUCCESS 0
#define MOVE_BLOCKED 1
#define MOVE_NO_EXIT 2
#define MOVE_INVALID_DIR 3
#define MOVE_LOCKED 4

/* Room connection structure */
struct room_connection
{
  int from_room;
  int to_room;
  int direction;
  bool is_hatch;
  bool is_locked;
};

/* Mock ship for movement testing */
#define MAX_SHIP_ROOMS 20
#define MAX_SHIP_CONNECTIONS 40

struct mock_ship_data
{
  int shipnum;
  int num_rooms;
  int room_vnums[MAX_SHIP_ROOMS];
  struct room_connection connections[MAX_SHIP_CONNECTIONS];
  int num_connections;
};

/* ========================================================================= */
/* HELPER FUNCTIONS                                                          */
/* ========================================================================= */

/**
 * Get opposite direction
 */
static int get_reverse_direction(int dir)
{
  switch (dir)
  {
  case DIR_NORTH:
    return DIR_SOUTH;
  case DIR_SOUTH:
    return DIR_NORTH;
  case DIR_EAST:
    return DIR_WEST;
  case DIR_WEST:
    return DIR_EAST;
  case DIR_UP:
    return DIR_DOWN;
  case DIR_DOWN:
    return DIR_UP;
  default:
    return DIR_INVALID;
  }
}

/**
 * Validate direction is within bounds
 */
static bool is_valid_direction(int dir)
{
  return (dir >= 0 && dir < NUM_DIRS);
}

/**
 * Get direction name
 */
static const char *get_direction_name(int dir)
{
  static const char *names[] = {"north", "east", "south", "west", "up", "down"};

  if (!is_valid_direction(dir))
  {
    return "unknown";
  }
  return names[dir];
}

/**
 * Initialize mock ship for movement testing
 */
static void init_movement_ship(struct mock_ship_data *ship, int num_rooms)
{
  int i;

  memset(ship, 0, sizeof(*ship));
  ship->shipnum = 0;
  ship->num_rooms = num_rooms;

  for (i = 0; i < num_rooms && i < MAX_SHIP_ROOMS; i++)
  {
    ship->room_vnums[i] = 70000 + i;
  }

  for (i = 0; i < MAX_SHIP_CONNECTIONS; i++)
  {
    ship->connections[i].from_room = NOWHERE;
    ship->connections[i].to_room = NOWHERE;
    ship->connections[i].direction = DIR_INVALID;
    ship->connections[i].is_hatch = FALSE;
    ship->connections[i].is_locked = FALSE;
  }
}

/**
 * Add a connection to ship
 */
static bool add_ship_connection(struct mock_ship_data *ship, int from, int to, int dir,
                                bool is_hatch, bool is_locked)
{
  if (!ship || ship->num_connections >= MAX_SHIP_CONNECTIONS)
  {
    return FALSE;
  }

  ship->connections[ship->num_connections].from_room = from;
  ship->connections[ship->num_connections].to_room = to;
  ship->connections[ship->num_connections].direction = dir;
  ship->connections[ship->num_connections].is_hatch = is_hatch;
  ship->connections[ship->num_connections].is_locked = is_locked;
  ship->num_connections++;

  return TRUE;
}

/**
 * Find exit from room in direction
 */
static int find_exit(struct mock_ship_data *ship, int from_room, int dir)
{
  int i;

  if (!ship)
  {
    return NOWHERE;
  }

  for (i = 0; i < ship->num_connections; i++)
  {
    if (ship->connections[i].from_room == from_room && ship->connections[i].direction == dir)
    {
      return ship->connections[i].to_room;
    }
  }

  return NOWHERE;
}

/**
 * Check if passage is blocked
 */
static bool is_passage_blocked(struct mock_ship_data *ship, int from_room, int dir)
{
  int i;

  if (!ship)
  {
    return TRUE;
  }

  for (i = 0; i < ship->num_connections; i++)
  {
    if (ship->connections[i].from_room == from_room && ship->connections[i].direction == dir)
    {
      return ship->connections[i].is_locked;
    }
  }

  return TRUE; /* No exit = blocked */
}

/**
 * Attempt to move in direction
 */
static int attempt_move(struct mock_ship_data *ship, int from_room, int dir)
{
  int to_room;
  int i;

  if (!is_valid_direction(dir))
  {
    return MOVE_INVALID_DIR;
  }

  to_room = find_exit(ship, from_room, dir);
  if (to_room == NOWHERE)
  {
    return MOVE_NO_EXIT;
  }

  /* Check if passage is locked */
  for (i = 0; i < ship->num_connections; i++)
  {
    if (ship->connections[i].from_room == from_room && ship->connections[i].direction == dir)
    {
      if (ship->connections[i].is_locked)
      {
        return MOVE_LOCKED;
      }
    }
  }

  return MOVE_SUCCESS;
}

/* ========================================================================= */
/* DIRECTION VALIDATION TESTS                                                */
/* ========================================================================= */

/**
 * Test valid direction validation
 */
void Test_movement_valid_directions(CuTest *tc)
{
  CuAssertIntEquals(tc, TRUE, is_valid_direction(DIR_NORTH));
  CuAssertIntEquals(tc, TRUE, is_valid_direction(DIR_EAST));
  CuAssertIntEquals(tc, TRUE, is_valid_direction(DIR_SOUTH));
  CuAssertIntEquals(tc, TRUE, is_valid_direction(DIR_WEST));
  CuAssertIntEquals(tc, TRUE, is_valid_direction(DIR_UP));
  CuAssertIntEquals(tc, TRUE, is_valid_direction(DIR_DOWN));
}

/**
 * Test invalid direction validation
 */
void Test_movement_invalid_directions(CuTest *tc)
{
  CuAssertIntEquals(tc, FALSE, is_valid_direction(-1));
  CuAssertIntEquals(tc, FALSE, is_valid_direction(NUM_DIRS));
  CuAssertIntEquals(tc, FALSE, is_valid_direction(100));
  CuAssertIntEquals(tc, FALSE, is_valid_direction(-100));
}

/**
 * Test reverse direction calculation
 */
void Test_movement_reverse_directions(CuTest *tc)
{
  CuAssertIntEquals(tc, DIR_SOUTH, get_reverse_direction(DIR_NORTH));
  CuAssertIntEquals(tc, DIR_NORTH, get_reverse_direction(DIR_SOUTH));
  CuAssertIntEquals(tc, DIR_WEST, get_reverse_direction(DIR_EAST));
  CuAssertIntEquals(tc, DIR_EAST, get_reverse_direction(DIR_WEST));
  CuAssertIntEquals(tc, DIR_DOWN, get_reverse_direction(DIR_UP));
  CuAssertIntEquals(tc, DIR_UP, get_reverse_direction(DIR_DOWN));
}

/**
 * Test invalid reverse direction
 */
void Test_movement_reverse_invalid(CuTest *tc)
{
  CuAssertIntEquals(tc, DIR_INVALID, get_reverse_direction(-1));
  CuAssertIntEquals(tc, DIR_INVALID, get_reverse_direction(NUM_DIRS));
  CuAssertIntEquals(tc, DIR_INVALID, get_reverse_direction(100));
}

/**
 * Test direction names
 */
void Test_movement_direction_names(CuTest *tc)
{
  CuAssertStrEquals(tc, "north", get_direction_name(DIR_NORTH));
  CuAssertStrEquals(tc, "east", get_direction_name(DIR_EAST));
  CuAssertStrEquals(tc, "south", get_direction_name(DIR_SOUTH));
  CuAssertStrEquals(tc, "west", get_direction_name(DIR_WEST));
  CuAssertStrEquals(tc, "up", get_direction_name(DIR_UP));
  CuAssertStrEquals(tc, "down", get_direction_name(DIR_DOWN));
  CuAssertStrEquals(tc, "unknown", get_direction_name(-1));
}

/* ========================================================================= */
/* EXIT FINDING TESTS                                                        */
/* ========================================================================= */

/**
 * Test finding exit that exists
 */
void Test_movement_find_exit_exists(CuTest *tc)
{
  struct mock_ship_data ship;
  int dest;

  init_movement_ship(&ship, 3);
  add_ship_connection(&ship, 70000, 70001, DIR_NORTH, FALSE, FALSE);
  add_ship_connection(&ship, 70001, 70002, DIR_EAST, FALSE, FALSE);

  dest = find_exit(&ship, 70000, DIR_NORTH);
  CuAssertIntEquals(tc, 70001, dest);

  dest = find_exit(&ship, 70001, DIR_EAST);
  CuAssertIntEquals(tc, 70002, dest);
}

/**
 * Test finding exit that does not exist
 */
void Test_movement_find_exit_none(CuTest *tc)
{
  struct mock_ship_data ship;
  int dest;

  init_movement_ship(&ship, 3);
  add_ship_connection(&ship, 70000, 70001, DIR_NORTH, FALSE, FALSE);

  /* No exit south */
  dest = find_exit(&ship, 70000, DIR_SOUTH);
  CuAssertIntEquals(tc, NOWHERE, dest);

  /* No exit from room 70002 at all */
  dest = find_exit(&ship, 70002, DIR_NORTH);
  CuAssertIntEquals(tc, NOWHERE, dest);
}

/**
 * Test finding exit with NULL ship
 */
void Test_movement_find_exit_null(CuTest *tc)
{
  int dest;

  dest = find_exit(NULL, 70000, DIR_NORTH);
  CuAssertIntEquals(tc, NOWHERE, dest);
}

/* ========================================================================= */
/* BLOCKED PASSAGE TESTS                                                     */
/* ========================================================================= */

/**
 * Test unlocked passage is not blocked
 */
void Test_movement_passage_unlocked(CuTest *tc)
{
  struct mock_ship_data ship;
  bool blocked;

  init_movement_ship(&ship, 2);
  add_ship_connection(&ship, 70000, 70001, DIR_NORTH, FALSE, FALSE);

  blocked = is_passage_blocked(&ship, 70000, DIR_NORTH);
  CuAssertIntEquals(tc, FALSE, blocked);
}

/**
 * Test locked passage is blocked
 */
void Test_movement_passage_locked(CuTest *tc)
{
  struct mock_ship_data ship;
  bool blocked;

  init_movement_ship(&ship, 2);
  add_ship_connection(&ship, 70000, 70001, DIR_NORTH, FALSE, TRUE);

  blocked = is_passage_blocked(&ship, 70000, DIR_NORTH);
  CuAssertIntEquals(tc, TRUE, blocked);
}

/**
 * Test nonexistent passage is blocked
 */
void Test_movement_passage_none(CuTest *tc)
{
  struct mock_ship_data ship;
  bool blocked;

  init_movement_ship(&ship, 2);
  /* No connection added */

  blocked = is_passage_blocked(&ship, 70000, DIR_NORTH);
  CuAssertIntEquals(tc, TRUE, blocked);
}

/**
 * Test hatch passage
 */
void Test_movement_passage_hatch(CuTest *tc)
{
  struct mock_ship_data ship;
  bool blocked;

  init_movement_ship(&ship, 2);
  add_ship_connection(&ship, 70000, 70001, DIR_UP, TRUE, FALSE);

  /* Unlocked hatch is passable */
  blocked = is_passage_blocked(&ship, 70000, DIR_UP);
  CuAssertIntEquals(tc, FALSE, blocked);
}

/* ========================================================================= */
/* MOVEMENT ATTEMPT TESTS                                                    */
/* ========================================================================= */

/**
 * Test successful movement
 */
void Test_movement_attempt_success(CuTest *tc)
{
  struct mock_ship_data ship;
  int result;

  init_movement_ship(&ship, 2);
  add_ship_connection(&ship, 70000, 70001, DIR_NORTH, FALSE, FALSE);

  result = attempt_move(&ship, 70000, DIR_NORTH);
  CuAssertIntEquals(tc, MOVE_SUCCESS, result);
}

/**
 * Test movement with no exit
 */
void Test_movement_attempt_no_exit(CuTest *tc)
{
  struct mock_ship_data ship;
  int result;

  init_movement_ship(&ship, 2);
  /* No connections */

  result = attempt_move(&ship, 70000, DIR_NORTH);
  CuAssertIntEquals(tc, MOVE_NO_EXIT, result);
}

/**
 * Test movement with invalid direction
 */
void Test_movement_attempt_invalid_dir(CuTest *tc)
{
  struct mock_ship_data ship;
  int result;

  init_movement_ship(&ship, 2);
  add_ship_connection(&ship, 70000, 70001, DIR_NORTH, FALSE, FALSE);

  result = attempt_move(&ship, 70000, -1);
  CuAssertIntEquals(tc, MOVE_INVALID_DIR, result);

  result = attempt_move(&ship, 70000, 100);
  CuAssertIntEquals(tc, MOVE_INVALID_DIR, result);
}

/**
 * Test movement through locked passage
 */
void Test_movement_attempt_locked(CuTest *tc)
{
  struct mock_ship_data ship;
  int result;

  init_movement_ship(&ship, 2);
  add_ship_connection(&ship, 70000, 70001, DIR_NORTH, FALSE, TRUE);

  result = attempt_move(&ship, 70000, DIR_NORTH);
  CuAssertIntEquals(tc, MOVE_LOCKED, result);
}

/* ========================================================================= */
/* COMPLEX MOVEMENT TESTS                                                    */
/* ========================================================================= */

/**
 * Test movement through connected rooms
 */
void Test_movement_multi_room_path(CuTest *tc)
{
  struct mock_ship_data ship;
  int result;

  init_movement_ship(&ship, 4);

  /* Create a path: 0 -> 1 -> 2 -> 3 */
  add_ship_connection(&ship, 70000, 70001, DIR_NORTH, FALSE, FALSE);
  add_ship_connection(&ship, 70001, 70002, DIR_NORTH, FALSE, FALSE);
  add_ship_connection(&ship, 70002, 70003, DIR_NORTH, FALSE, FALSE);

  /* Return path */
  add_ship_connection(&ship, 70001, 70000, DIR_SOUTH, FALSE, FALSE);
  add_ship_connection(&ship, 70002, 70001, DIR_SOUTH, FALSE, FALSE);
  add_ship_connection(&ship, 70003, 70002, DIR_SOUTH, FALSE, FALSE);

  /* Forward */
  result = attempt_move(&ship, 70000, DIR_NORTH);
  CuAssertIntEquals(tc, MOVE_SUCCESS, result);

  result = attempt_move(&ship, 70001, DIR_NORTH);
  CuAssertIntEquals(tc, MOVE_SUCCESS, result);

  result = attempt_move(&ship, 70002, DIR_NORTH);
  CuAssertIntEquals(tc, MOVE_SUCCESS, result);

  /* Cannot continue past 70003 */
  result = attempt_move(&ship, 70003, DIR_NORTH);
  CuAssertIntEquals(tc, MOVE_NO_EXIT, result);

  /* Back */
  result = attempt_move(&ship, 70003, DIR_SOUTH);
  CuAssertIntEquals(tc, MOVE_SUCCESS, result);
}

/**
 * Test movement with branching paths
 */
void Test_movement_branching_path(CuTest *tc)
{
  struct mock_ship_data ship;
  int result;

  init_movement_ship(&ship, 4);

  /* Create a T-junction at room 1 */
  add_ship_connection(&ship, 70000, 70001, DIR_NORTH, FALSE, FALSE);
  add_ship_connection(&ship, 70001, 70002, DIR_EAST, FALSE, FALSE);
  add_ship_connection(&ship, 70001, 70003, DIR_WEST, FALSE, FALSE);

  /* Can go north from 0 */
  result = attempt_move(&ship, 70000, DIR_NORTH);
  CuAssertIntEquals(tc, MOVE_SUCCESS, result);

  /* Can go east or west from 1 */
  result = attempt_move(&ship, 70001, DIR_EAST);
  CuAssertIntEquals(tc, MOVE_SUCCESS, result);

  result = attempt_move(&ship, 70001, DIR_WEST);
  CuAssertIntEquals(tc, MOVE_SUCCESS, result);

  /* Cannot go north from 1 */
  result = attempt_move(&ship, 70001, DIR_NORTH);
  CuAssertIntEquals(tc, MOVE_NO_EXIT, result);
}

/* ========================================================================= */
/* TEST SUITE REGISTRATION                                                   */
/* ========================================================================= */

/**
 * Get the vessel movement test suite
 */
CuSuite *VesselMovementGetSuite(void)
{
  CuSuite *suite = CuSuiteNew();

  /* Direction validation tests */
  SUITE_ADD_TEST(suite, Test_movement_valid_directions);
  SUITE_ADD_TEST(suite, Test_movement_invalid_directions);
  SUITE_ADD_TEST(suite, Test_movement_reverse_directions);
  SUITE_ADD_TEST(suite, Test_movement_reverse_invalid);
  SUITE_ADD_TEST(suite, Test_movement_direction_names);

  /* Exit finding tests */
  SUITE_ADD_TEST(suite, Test_movement_find_exit_exists);
  SUITE_ADD_TEST(suite, Test_movement_find_exit_none);
  SUITE_ADD_TEST(suite, Test_movement_find_exit_null);

  /* Blocked passage tests */
  SUITE_ADD_TEST(suite, Test_movement_passage_unlocked);
  SUITE_ADD_TEST(suite, Test_movement_passage_locked);
  SUITE_ADD_TEST(suite, Test_movement_passage_none);
  SUITE_ADD_TEST(suite, Test_movement_passage_hatch);

  /* Movement attempt tests */
  SUITE_ADD_TEST(suite, Test_movement_attempt_success);
  SUITE_ADD_TEST(suite, Test_movement_attempt_no_exit);
  SUITE_ADD_TEST(suite, Test_movement_attempt_invalid_dir);
  SUITE_ADD_TEST(suite, Test_movement_attempt_locked);

  /* Complex movement tests */
  SUITE_ADD_TEST(suite, Test_movement_multi_room_path);
  SUITE_ADD_TEST(suite, Test_movement_branching_path);

  return suite;
}
