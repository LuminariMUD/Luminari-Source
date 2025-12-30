/* ************************************************************************
 *      File:   test_vehicle_commands.c                Part of LuminariMUD  *
 *   Purpose:   Unit tests for vehicle player commands                       *
 *              Phase 02 Session 04: Vehicle Player Commands                 *
 * ********************************************************************** */

/* Enable POSIX functions (strcasecmp) in C89 mode */
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include "CuTest.h"

/* ========================================================================= */
/* MOCK DEFINITIONS (copied from vessels.h for standalone testing)          */
/* ========================================================================= */

/* Direction constants */
#define NORTH 0
#define EAST 1
#define SOUTH 2
#define WEST 3
#define UP 4
#define DOWN 5
#define NORTHEAST 6
#define NORTHWEST 7
#define SOUTHEAST 8
#define SOUTHWEST 9

/* Vehicle Types */
enum vehicle_type
{
  VEHICLE_NONE = 0,
  VEHICLE_CART,
  VEHICLE_WAGON,
  VEHICLE_MOUNT,
  VEHICLE_CARRIAGE,
  NUM_VEHICLE_TYPES
};

/* Vehicle States */
enum vehicle_state
{
  VSTATE_IDLE = 0,
  VSTATE_MOVING,
  VSTATE_LOADED,
  VSTATE_HITCHED,
  VSTATE_DAMAGED,
  NUM_VEHICLE_STATES
};

/* Constants */
#define VEHICLE_NAME_LENGTH 64
#define VEHICLE_PASSENGERS_CART 2
#define VEHICLE_CONDITION_MAX 100
#define VEHICLE_CONDITION_BROKEN 0
#define MAX_MOUNTED_PLAYERS 100

/* Room type */
typedef int room_rnum;
#define NOWHERE -1

/* Mock obj_data */
struct obj_data
{
  int placeholder;
};

/* Vehicle data structure */
struct vehicle_data
{
  int id;
  enum vehicle_type type;
  enum vehicle_state state;
  char name[VEHICLE_NAME_LENGTH];
  room_rnum location;
  int direction;
  int x_coord;
  int y_coord;
  int max_passengers;
  int current_passengers;
  int max_weight;
  int current_weight;
  int base_speed;
  int current_speed;
  int terrain_flags;
  int max_condition;
  int condition;
  long owner_id;
  struct obj_data *obj;
};

/* ========================================================================= */
/* MOCK PLAYER-VEHICLE TRACKING (mirrors vehicles_commands.c)               */
/* ========================================================================= */

static struct player_vehicle_map
{
  long player_id;
  int vehicle_id;
} mock_mounted_players[MAX_MOUNTED_PLAYERS];

static int mock_mounted_count = 0;

/* ========================================================================= */
/* FUNCTION IMPLEMENTATIONS FOR TESTING                                       */
/* ========================================================================= */

/**
 * Parse a direction string for the drive command.
 * Copied from vehicles_commands.c for standalone testing.
 */
static int parse_drive_direction(const char *arg)
{
  if (arg == NULL || *arg == '\0')
  {
    return -1;
  }

  /* Check for numeric direction (0-7) */
  if (isdigit(*arg) && *(arg + 1) == '\0')
  {
    int dir = *arg - '0';
    if (dir >= 0 && dir <= 7)
    {
      switch (dir)
      {
      case 0:
        return NORTH;
      case 1:
        return EAST;
      case 2:
        return SOUTH;
      case 3:
        return WEST;
      case 4:
        return NORTHEAST;
      case 5:
        return NORTHWEST;
      case 6:
        return SOUTHEAST;
      case 7:
        return SOUTHWEST;
      }
    }
    return -1;
  }

  /* Check for string direction names */
  if (strcasecmp(arg, "north") == 0 || strcasecmp(arg, "n") == 0)
  {
    return NORTH;
  }
  if (strcasecmp(arg, "south") == 0 || strcasecmp(arg, "s") == 0)
  {
    return SOUTH;
  }
  if (strcasecmp(arg, "east") == 0 || strcasecmp(arg, "e") == 0)
  {
    return EAST;
  }
  if (strcasecmp(arg, "west") == 0 || strcasecmp(arg, "w") == 0)
  {
    return WEST;
  }
  if (strcasecmp(arg, "northeast") == 0 || strcasecmp(arg, "ne") == 0)
  {
    return NORTHEAST;
  }
  if (strcasecmp(arg, "northwest") == 0 || strcasecmp(arg, "nw") == 0)
  {
    return NORTHWEST;
  }
  if (strcasecmp(arg, "southeast") == 0 || strcasecmp(arg, "se") == 0)
  {
    return SOUTHEAST;
  }
  if (strcasecmp(arg, "southwest") == 0 || strcasecmp(arg, "sw") == 0)
  {
    return SOUTHWEST;
  }

  return -1;
}

/**
 * Check if a player is in a vehicle.
 */
static int is_player_in_vehicle_test(long player_id)
{
  int i;
  for (i = 0; i < MAX_MOUNTED_PLAYERS; i++)
  {
    if (mock_mounted_players[i].player_id == player_id)
    {
      return 1;
    }
  }
  return 0;
}

/**
 * Register a player mount.
 */
static int register_player_mount_test(long player_id, int vehicle_id)
{
  int i;
  if (is_player_in_vehicle_test(player_id))
  {
    return 0;
  }
  for (i = 0; i < MAX_MOUNTED_PLAYERS; i++)
  {
    if (mock_mounted_players[i].player_id == 0)
    {
      mock_mounted_players[i].player_id = player_id;
      mock_mounted_players[i].vehicle_id = vehicle_id;
      mock_mounted_count++;
      return 1;
    }
  }
  return 0;
}

/**
 * Unregister a player mount.
 */
static int unregister_player_mount_test(long player_id)
{
  int i;
  for (i = 0; i < MAX_MOUNTED_PLAYERS; i++)
  {
    if (mock_mounted_players[i].player_id == player_id)
    {
      mock_mounted_players[i].player_id = 0;
      mock_mounted_players[i].vehicle_id = 0;
      mock_mounted_count--;
      return 1;
    }
  }
  return 0;
}

/**
 * Get vehicle ID for a player.
 */
static int get_player_vehicle_id_test(long player_id)
{
  int i;
  for (i = 0; i < MAX_MOUNTED_PLAYERS; i++)
  {
    if (mock_mounted_players[i].player_id == player_id)
    {
      return mock_mounted_players[i].vehicle_id;
    }
  }
  return 0;
}

/**
 * Reset mock tracking data between tests.
 */
static void reset_mock_tracking(void)
{
  int i;
  for (i = 0; i < MAX_MOUNTED_PLAYERS; i++)
  {
    mock_mounted_players[i].player_id = 0;
    mock_mounted_players[i].vehicle_id = 0;
  }
  mock_mounted_count = 0;
}

/* ========================================================================= */
/* DIRECTION PARSING TESTS                                                    */
/* ========================================================================= */

void test_parse_direction_null(CuTest *tc)
{
  int result = parse_drive_direction(NULL);
  CuAssertIntEquals(tc, -1, result);
}

void test_parse_direction_empty(CuTest *tc)
{
  int result = parse_drive_direction("");
  CuAssertIntEquals(tc, -1, result);
}

void test_parse_direction_north_full(CuTest *tc)
{
  int result = parse_drive_direction("north");
  CuAssertIntEquals(tc, NORTH, result);
}

void test_parse_direction_north_abbrev(CuTest *tc)
{
  int result = parse_drive_direction("n");
  CuAssertIntEquals(tc, NORTH, result);
}

void test_parse_direction_south_full(CuTest *tc)
{
  int result = parse_drive_direction("south");
  CuAssertIntEquals(tc, SOUTH, result);
}

void test_parse_direction_south_abbrev(CuTest *tc)
{
  int result = parse_drive_direction("s");
  CuAssertIntEquals(tc, SOUTH, result);
}

void test_parse_direction_east_full(CuTest *tc)
{
  int result = parse_drive_direction("east");
  CuAssertIntEquals(tc, EAST, result);
}

void test_parse_direction_west_full(CuTest *tc)
{
  int result = parse_drive_direction("west");
  CuAssertIntEquals(tc, WEST, result);
}

void test_parse_direction_northeast_full(CuTest *tc)
{
  int result = parse_drive_direction("northeast");
  CuAssertIntEquals(tc, NORTHEAST, result);
}

void test_parse_direction_northeast_abbrev(CuTest *tc)
{
  int result = parse_drive_direction("ne");
  CuAssertIntEquals(tc, NORTHEAST, result);
}

void test_parse_direction_northwest_full(CuTest *tc)
{
  int result = parse_drive_direction("northwest");
  CuAssertIntEquals(tc, NORTHWEST, result);
}

void test_parse_direction_southeast_full(CuTest *tc)
{
  int result = parse_drive_direction("southeast");
  CuAssertIntEquals(tc, SOUTHEAST, result);
}

void test_parse_direction_southwest_full(CuTest *tc)
{
  int result = parse_drive_direction("southwest");
  CuAssertIntEquals(tc, SOUTHWEST, result);
}

void test_parse_direction_numeric_0(CuTest *tc)
{
  int result = parse_drive_direction("0");
  CuAssertIntEquals(tc, NORTH, result);
}

void test_parse_direction_numeric_1(CuTest *tc)
{
  int result = parse_drive_direction("1");
  CuAssertIntEquals(tc, EAST, result);
}

void test_parse_direction_numeric_2(CuTest *tc)
{
  int result = parse_drive_direction("2");
  CuAssertIntEquals(tc, SOUTH, result);
}

void test_parse_direction_numeric_3(CuTest *tc)
{
  int result = parse_drive_direction("3");
  CuAssertIntEquals(tc, WEST, result);
}

void test_parse_direction_invalid(CuTest *tc)
{
  int result = parse_drive_direction("invalid");
  CuAssertIntEquals(tc, -1, result);
}

void test_parse_direction_numeric_out_of_range(CuTest *tc)
{
  int result = parse_drive_direction("9");
  CuAssertIntEquals(tc, -1, result);
}

void test_parse_direction_case_insensitive(CuTest *tc)
{
  CuAssertIntEquals(tc, NORTH, parse_drive_direction("NORTH"));
  CuAssertIntEquals(tc, NORTH, parse_drive_direction("North"));
  CuAssertIntEquals(tc, SOUTH, parse_drive_direction("SOUTH"));
  CuAssertIntEquals(tc, NORTHEAST, parse_drive_direction("NE"));
}

/* ========================================================================= */
/* PLAYER-VEHICLE TRACKING TESTS                                              */
/* ========================================================================= */

void test_is_player_in_vehicle_not_mounted(CuTest *tc)
{
  int result;
  reset_mock_tracking();
  result = is_player_in_vehicle_test(12345);
  CuAssertIntEquals(tc, 0, result);
}

void test_is_player_in_vehicle_mounted(CuTest *tc)
{
  int result;
  reset_mock_tracking();
  register_player_mount_test(12345, 1);
  result = is_player_in_vehicle_test(12345);
  CuAssertIntEquals(tc, 1, result);
}

void test_register_player_mount_success(CuTest *tc)
{
  int result;
  reset_mock_tracking();
  result = register_player_mount_test(12345, 1);
  CuAssertIntEquals(tc, 1, result);
  CuAssertIntEquals(tc, 1, mock_mounted_count);
}

void test_register_player_mount_already_mounted(CuTest *tc)
{
  int result;
  reset_mock_tracking();
  register_player_mount_test(12345, 1);
  result = register_player_mount_test(12345, 2);
  CuAssertIntEquals(tc, 0, result);
  CuAssertIntEquals(tc, 1, mock_mounted_count);
}

void test_unregister_player_mount_success(CuTest *tc)
{
  int result;
  reset_mock_tracking();
  register_player_mount_test(12345, 1);
  CuAssertIntEquals(tc, 1, mock_mounted_count);
  result = unregister_player_mount_test(12345);
  CuAssertIntEquals(tc, 1, result);
  CuAssertIntEquals(tc, 0, mock_mounted_count);
}

void test_unregister_player_mount_not_mounted(CuTest *tc)
{
  int result;
  reset_mock_tracking();
  result = unregister_player_mount_test(12345);
  CuAssertIntEquals(tc, 0, result);
}

void test_get_player_vehicle_id_mounted(CuTest *tc)
{
  int result;
  reset_mock_tracking();
  register_player_mount_test(12345, 42);
  result = get_player_vehicle_id_test(12345);
  CuAssertIntEquals(tc, 42, result);
}

void test_get_player_vehicle_id_not_mounted(CuTest *tc)
{
  int result;
  reset_mock_tracking();
  result = get_player_vehicle_id_test(12345);
  CuAssertIntEquals(tc, 0, result);
}

void test_multiple_players_mount(CuTest *tc)
{
  reset_mock_tracking();
  register_player_mount_test(100, 1);
  register_player_mount_test(200, 1);
  register_player_mount_test(300, 2);
  CuAssertIntEquals(tc, 3, mock_mounted_count);
  CuAssertIntEquals(tc, 1, is_player_in_vehicle_test(100));
  CuAssertIntEquals(tc, 1, is_player_in_vehicle_test(200));
  CuAssertIntEquals(tc, 1, is_player_in_vehicle_test(300));
  CuAssertIntEquals(tc, 1, get_player_vehicle_id_test(100));
  CuAssertIntEquals(tc, 1, get_player_vehicle_id_test(200));
  CuAssertIntEquals(tc, 2, get_player_vehicle_id_test(300));
}

void test_mount_dismount_cycle(CuTest *tc)
{
  reset_mock_tracking();

  /* Mount */
  CuAssertIntEquals(tc, 1, register_player_mount_test(12345, 1));
  CuAssertIntEquals(tc, 1, is_player_in_vehicle_test(12345));

  /* Dismount */
  CuAssertIntEquals(tc, 1, unregister_player_mount_test(12345));
  CuAssertIntEquals(tc, 0, is_player_in_vehicle_test(12345));

  /* Mount again */
  CuAssertIntEquals(tc, 1, register_player_mount_test(12345, 2));
  CuAssertIntEquals(tc, 1, is_player_in_vehicle_test(12345));
  CuAssertIntEquals(tc, 2, get_player_vehicle_id_test(12345));
}

/* ========================================================================= */
/* VEHICLE CONDITION TESTS                                                    */
/* ========================================================================= */

void test_vehicle_condition_calculation(CuTest *tc)
{
  struct vehicle_data vehicle;
  int condition_pct;

  /* Test 100% condition */
  vehicle.max_condition = 100;
  vehicle.condition = 100;
  condition_pct = (vehicle.condition * 100) / vehicle.max_condition;
  CuAssertIntEquals(tc, 100, condition_pct);

  /* Test 50% condition */
  vehicle.condition = 50;
  condition_pct = (vehicle.condition * 100) / vehicle.max_condition;
  CuAssertIntEquals(tc, 50, condition_pct);

  /* Test 0% condition */
  vehicle.condition = 0;
  condition_pct = (vehicle.condition * 100) / vehicle.max_condition;
  CuAssertIntEquals(tc, 0, condition_pct);

  /* Test 75% condition */
  vehicle.condition = 75;
  condition_pct = (vehicle.condition * 100) / vehicle.max_condition;
  CuAssertIntEquals(tc, 75, condition_pct);
}

/* ========================================================================= */
/* TEST SUITE REGISTRATION                                                    */
/* ========================================================================= */

CuSuite *CuGetVehicleCommandsSuite(void)
{
  CuSuite *suite = CuSuiteNew();

  /* Direction parsing tests */
  SUITE_ADD_TEST(suite, test_parse_direction_null);
  SUITE_ADD_TEST(suite, test_parse_direction_empty);
  SUITE_ADD_TEST(suite, test_parse_direction_north_full);
  SUITE_ADD_TEST(suite, test_parse_direction_north_abbrev);
  SUITE_ADD_TEST(suite, test_parse_direction_south_full);
  SUITE_ADD_TEST(suite, test_parse_direction_south_abbrev);
  SUITE_ADD_TEST(suite, test_parse_direction_east_full);
  SUITE_ADD_TEST(suite, test_parse_direction_west_full);
  SUITE_ADD_TEST(suite, test_parse_direction_northeast_full);
  SUITE_ADD_TEST(suite, test_parse_direction_northeast_abbrev);
  SUITE_ADD_TEST(suite, test_parse_direction_northwest_full);
  SUITE_ADD_TEST(suite, test_parse_direction_southeast_full);
  SUITE_ADD_TEST(suite, test_parse_direction_southwest_full);
  SUITE_ADD_TEST(suite, test_parse_direction_numeric_0);
  SUITE_ADD_TEST(suite, test_parse_direction_numeric_1);
  SUITE_ADD_TEST(suite, test_parse_direction_numeric_2);
  SUITE_ADD_TEST(suite, test_parse_direction_numeric_3);
  SUITE_ADD_TEST(suite, test_parse_direction_invalid);
  SUITE_ADD_TEST(suite, test_parse_direction_numeric_out_of_range);
  SUITE_ADD_TEST(suite, test_parse_direction_case_insensitive);

  /* Player-vehicle tracking tests */
  SUITE_ADD_TEST(suite, test_is_player_in_vehicle_not_mounted);
  SUITE_ADD_TEST(suite, test_is_player_in_vehicle_mounted);
  SUITE_ADD_TEST(suite, test_register_player_mount_success);
  SUITE_ADD_TEST(suite, test_register_player_mount_already_mounted);
  SUITE_ADD_TEST(suite, test_unregister_player_mount_success);
  SUITE_ADD_TEST(suite, test_unregister_player_mount_not_mounted);
  SUITE_ADD_TEST(suite, test_get_player_vehicle_id_mounted);
  SUITE_ADD_TEST(suite, test_get_player_vehicle_id_not_mounted);
  SUITE_ADD_TEST(suite, test_multiple_players_mount);
  SUITE_ADD_TEST(suite, test_mount_dismount_cycle);

  /* Vehicle condition tests */
  SUITE_ADD_TEST(suite, test_vehicle_condition_calculation);

  return suite;
}

/* ========================================================================= */
/* STANDALONE TEST RUNNER                                                     */
/* ========================================================================= */

#ifdef TEST_VEHICLE_COMMANDS_STANDALONE

int main(void)
{
  CuString *output;
  CuSuite *suite;
  int result;

  output = CuStringNew();
  suite = CuGetVehicleCommandsSuite();

  CuSuiteRun(suite);
  CuSuiteSummary(suite, output);
  CuSuiteDetails(suite, output);

  printf("%s\n", output->buffer);

  /* Save fail count before freeing */
  result = (suite->failCount > 0) ? 1 : 0;

  CuStringDelete(output);
  CuSuiteDelete(suite);

  return result;
}

#endif /* TEST_VEHICLE_COMMANDS_STANDALONE */
