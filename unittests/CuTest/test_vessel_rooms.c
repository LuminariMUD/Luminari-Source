/**
 * @file test_vessel_rooms.c
 * @brief Unit tests for vessel room generation system
 *
 * Tests room VNUM allocation, room linking, template application,
 * and interior room generation for the vessel system.
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
typedef int room_vnum;

/* Constants for room generation */
#define SHIP_INTERIOR_VNUM_BASE 70000
#define SHIP_INTERIOR_VNUM_MAX  79999
#define MAX_SHIP_ROOMS          20
#define MAX_SHIP_CONNECTIONS    40
#define GREYHAWK_MAXSHIPS       500

/* Vessel class enum */
enum vessel_class {
  VESSEL_RAFT = 0,
  VESSEL_BOAT = 1,
  VESSEL_SHIP = 2,
  VESSEL_WARSHIP = 3,
  VESSEL_AIRSHIP = 4,
  VESSEL_SUBMARINE = 5,
  VESSEL_TRANSPORT = 6,
  VESSEL_MAGICAL = 7
};

/* Ship room types */
enum ship_room_type {
  ROOM_TYPE_BRIDGE,
  ROOM_TYPE_QUARTERS,
  ROOM_TYPE_CARGO,
  ROOM_TYPE_ENGINEERING,
  ROOM_TYPE_WEAPONS,
  ROOM_TYPE_MEDICAL,
  ROOM_TYPE_MESS_HALL,
  ROOM_TYPE_CORRIDOR,
  ROOM_TYPE_AIRLOCK,
  ROOM_TYPE_DECK
};

#define NUM_ROOM_TYPES 10

/* Room connection structure */
struct room_connection {
  int from_room;
  int to_room;
  int direction;
  bool is_hatch;
  bool is_locked;
};

/* Mock ship data */
struct mock_ship_data {
  int shipnum;
  char name[128];
  enum vessel_class vessel_type;
  int num_rooms;
  int room_vnums[MAX_SHIP_ROOMS];
  int entrance_room;
  int bridge_room;
  struct room_connection connections[MAX_SHIP_CONNECTIONS];
  int num_connections;
};

/* ========================================================================= */
/* MOCK VNUM ALLOCATION SYSTEM                                               */
/* ========================================================================= */

static int allocated_vnums[10000];  /* Track which VNUMs are allocated */
static int num_allocated = 0;

/**
 * Reset VNUM allocation tracker
 */
static void reset_vnum_allocation(void)
{
  memset(allocated_vnums, 0, sizeof(allocated_vnums));
  num_allocated = 0;
}

/**
 * Check if VNUM is already allocated
 */
static bool is_vnum_allocated(int vnum)
{
  int i;
  for (i = 0; i < num_allocated; i++)
  {
    if (allocated_vnums[i] == vnum)
    {
      return TRUE;
    }
  }
  return FALSE;
}

/**
 * Mark VNUM as allocated
 */
static bool allocate_vnum(int vnum)
{
  if (num_allocated >= 10000)
  {
    return FALSE;
  }
  if (is_vnum_allocated(vnum))
  {
    return FALSE;
  }
  allocated_vnums[num_allocated++] = vnum;
  return TRUE;
}

/* ========================================================================= */
/* ROOM GENERATION FUNCTIONS                                                 */
/* ========================================================================= */

/**
 * Calculate VNUM for a ship room
 */
static int calculate_room_vnum(int shipnum, int room_index)
{
  int vnum;

  if (shipnum < 0 || shipnum >= GREYHAWK_MAXSHIPS)
  {
    return -1;
  }

  if (room_index < 0 || room_index >= MAX_SHIP_ROOMS)
  {
    return -1;
  }

  vnum = SHIP_INTERIOR_VNUM_BASE + (shipnum * MAX_SHIP_ROOMS) + room_index;

  if (vnum > SHIP_INTERIOR_VNUM_MAX)
  {
    return -1;
  }

  return vnum;
}

/**
 * Get base rooms for vessel type
 */
static int get_base_rooms_for_type(enum vessel_class type)
{
  switch (type)
  {
    case VESSEL_RAFT:       return 1;
    case VESSEL_BOAT:       return 2;
    case VESSEL_SHIP:       return 3;
    case VESSEL_WARSHIP:    return 5;
    case VESSEL_AIRSHIP:    return 4;
    case VESSEL_SUBMARINE:  return 4;
    case VESSEL_TRANSPORT:  return 6;
    case VESSEL_MAGICAL:    return 3;
    default:                return 1;
  }
}

/**
 * Get max rooms for vessel type
 */
static int get_max_rooms_for_type(enum vessel_class type)
{
  switch (type)
  {
    case VESSEL_RAFT:       return 2;
    case VESSEL_BOAT:       return 4;
    case VESSEL_SHIP:       return 8;
    case VESSEL_WARSHIP:    return 15;
    case VESSEL_AIRSHIP:    return 10;
    case VESSEL_SUBMARINE:  return 12;
    case VESSEL_TRANSPORT:  return 20;
    case VESSEL_MAGICAL:    return 10;
    default:                return 1;
  }
}

/**
 * Check if ship has interior rooms
 */
static bool ship_has_interior_rooms(struct mock_ship_data *ship)
{
  if (!ship)
  {
    return FALSE;
  }

  if (ship->num_rooms > 0)
  {
    return TRUE;
  }

  if (ship->room_vnums[0] != 0)
  {
    return TRUE;
  }

  return FALSE;
}

/**
 * Initialize ship data structure
 */
static void init_mock_ship(struct mock_ship_data *ship, int shipnum, enum vessel_class type)
{
  int i;

  memset(ship, 0, sizeof(*ship));
  ship->shipnum = shipnum;
  ship->vessel_type = type;
  snprintf(ship->name, sizeof(ship->name), "Test Ship %d", shipnum);

  for (i = 0; i < MAX_SHIP_ROOMS; i++)
  {
    ship->room_vnums[i] = 0;
  }

  for (i = 0; i < MAX_SHIP_CONNECTIONS; i++)
  {
    ship->connections[i].from_room = NOWHERE;
    ship->connections[i].to_room = NOWHERE;
  }
}

/**
 * Mock room creation - allocates VNUM and updates ship
 */
static int mock_create_ship_room(struct mock_ship_data *ship, enum ship_room_type type)
{
  int vnum;

  if (!ship)
  {
    return NOWHERE;
  }

  if (ship->num_rooms >= MAX_SHIP_ROOMS)
  {
    return NOWHERE;
  }

  vnum = calculate_room_vnum(ship->shipnum, ship->num_rooms);
  if (vnum < 0)
  {
    return NOWHERE;
  }

  if (!allocate_vnum(vnum))
  {
    return NOWHERE;  /* VNUM collision */
  }

  ship->room_vnums[ship->num_rooms] = vnum;
  ship->num_rooms++;

  /* First room becomes entrance and bridge */
  if (ship->num_rooms == 1)
  {
    ship->entrance_room = vnum;
    if (type == ROOM_TYPE_BRIDGE)
    {
      ship->bridge_room = vnum;
    }
  }

  return vnum;
}

/**
 * Add connection between rooms
 */
static bool mock_add_connection(struct mock_ship_data *ship, int from, int to, int dir)
{
  if (!ship)
  {
    return FALSE;
  }

  if (ship->num_connections >= MAX_SHIP_CONNECTIONS)
  {
    return FALSE;
  }

  ship->connections[ship->num_connections].from_room = from;
  ship->connections[ship->num_connections].to_room = to;
  ship->connections[ship->num_connections].direction = dir;
  ship->connections[ship->num_connections].is_hatch = FALSE;
  ship->connections[ship->num_connections].is_locked = FALSE;
  ship->num_connections++;

  return TRUE;
}

/* ========================================================================= */
/* VNUM CALCULATION TESTS                                                    */
/* ========================================================================= */

/**
 * Test VNUM calculation for first ship
 */
void Test_room_vnum_first_ship(CuTest *tc)
{
  CuAssertIntEquals(tc, 70000, calculate_room_vnum(0, 0));
  CuAssertIntEquals(tc, 70001, calculate_room_vnum(0, 1));
  CuAssertIntEquals(tc, 70019, calculate_room_vnum(0, 19));
}

/**
 * Test VNUM calculation for multiple ships
 */
void Test_room_vnum_multiple_ships(CuTest *tc)
{
  /* Ship 0 */
  CuAssertIntEquals(tc, 70000, calculate_room_vnum(0, 0));

  /* Ship 1 starts at 70020 */
  CuAssertIntEquals(tc, 70020, calculate_room_vnum(1, 0));

  /* Ship 10 starts at 70200 */
  CuAssertIntEquals(tc, 70200, calculate_room_vnum(10, 0));

  /* Ship 100 starts at 72000 */
  CuAssertIntEquals(tc, 72000, calculate_room_vnum(100, 0));
}

/**
 * Test VNUM range limits
 */
void Test_room_vnum_range_limits(CuTest *tc)
{
  /* Last valid ship that fits in range: (79999 - 70000) / 20 = 499 */
  CuAssertIntEquals(tc, 79980, calculate_room_vnum(499, 0));
  CuAssertIntEquals(tc, 79999, calculate_room_vnum(499, 19));

  /* Ship 500 would exceed range (70000 + 500*20 = 80000) */
  /* But shipnum 500 is == GREYHAWK_MAXSHIPS, so invalid */
  CuAssertIntEquals(tc, -1, calculate_room_vnum(500, 0));
}

/**
 * Test invalid VNUM parameters
 */
void Test_room_vnum_invalid_params(CuTest *tc)
{
  /* Negative ship number */
  CuAssertIntEquals(tc, -1, calculate_room_vnum(-1, 0));

  /* Negative room index */
  CuAssertIntEquals(tc, -1, calculate_room_vnum(0, -1));

  /* Room index too high */
  CuAssertIntEquals(tc, -1, calculate_room_vnum(0, 20));
  CuAssertIntEquals(tc, -1, calculate_room_vnum(0, 100));
}

/* ========================================================================= */
/* ROOM CREATION TESTS                                                       */
/* ========================================================================= */

/**
 * Test basic room creation
 */
void Test_room_create_basic(CuTest *tc)
{
  struct mock_ship_data ship;
  int vnum;

  reset_vnum_allocation();
  init_mock_ship(&ship, 0, VESSEL_SHIP);

  vnum = mock_create_ship_room(&ship, ROOM_TYPE_BRIDGE);

  CuAssertIntEquals(tc, 70000, vnum);
  CuAssertIntEquals(tc, 1, ship.num_rooms);
  CuAssertIntEquals(tc, 70000, ship.room_vnums[0]);
  CuAssertIntEquals(tc, 70000, ship.entrance_room);
  CuAssertIntEquals(tc, 70000, ship.bridge_room);
}

/**
 * Test multiple room creation
 */
void Test_room_create_multiple(CuTest *tc)
{
  struct mock_ship_data ship;
  int vnum1, vnum2, vnum3;

  reset_vnum_allocation();
  init_mock_ship(&ship, 0, VESSEL_SHIP);

  vnum1 = mock_create_ship_room(&ship, ROOM_TYPE_BRIDGE);
  vnum2 = mock_create_ship_room(&ship, ROOM_TYPE_QUARTERS);
  vnum3 = mock_create_ship_room(&ship, ROOM_TYPE_CARGO);

  CuAssertIntEquals(tc, 70000, vnum1);
  CuAssertIntEquals(tc, 70001, vnum2);
  CuAssertIntEquals(tc, 70002, vnum3);
  CuAssertIntEquals(tc, 3, ship.num_rooms);
}

/**
 * Test room limit enforcement
 */
void Test_room_create_max_limit(CuTest *tc)
{
  struct mock_ship_data ship;
  int i, vnum;

  reset_vnum_allocation();
  init_mock_ship(&ship, 0, VESSEL_TRANSPORT);

  /* Create maximum rooms */
  for (i = 0; i < MAX_SHIP_ROOMS; i++)
  {
    vnum = mock_create_ship_room(&ship, ROOM_TYPE_CORRIDOR);
    CuAssertTrue(tc, vnum != NOWHERE);
  }

  CuAssertIntEquals(tc, MAX_SHIP_ROOMS, ship.num_rooms);

  /* Next creation should fail */
  vnum = mock_create_ship_room(&ship, ROOM_TYPE_CORRIDOR);
  CuAssertIntEquals(tc, NOWHERE, vnum);
}

/**
 * Test NULL ship handling
 */
void Test_room_create_null_ship(CuTest *tc)
{
  int vnum;

  reset_vnum_allocation();

  vnum = mock_create_ship_room(NULL, ROOM_TYPE_BRIDGE);
  CuAssertIntEquals(tc, NOWHERE, vnum);
}

/* ========================================================================= */
/* VNUM COLLISION TESTS                                                      */
/* ========================================================================= */

/**
 * Test VNUM collision detection
 */
void Test_room_vnum_collision(CuTest *tc)
{
  struct mock_ship_data ship1, ship2;
  int vnum1, vnum2;

  reset_vnum_allocation();

  /* Create room on ship 0 */
  init_mock_ship(&ship1, 0, VESSEL_SHIP);
  vnum1 = mock_create_ship_room(&ship1, ROOM_TYPE_BRIDGE);
  CuAssertIntEquals(tc, 70000, vnum1);

  /* Try to create duplicate by manipulating ship number */
  init_mock_ship(&ship2, 0, VESSEL_SHIP);  /* Same shipnum! */
  vnum2 = mock_create_ship_room(&ship2, ROOM_TYPE_BRIDGE);

  /* Should fail due to collision */
  CuAssertIntEquals(tc, NOWHERE, vnum2);
}

/**
 * Test multiple ships no collision
 */
void Test_room_vnum_no_collision(CuTest *tc)
{
  struct mock_ship_data ship1, ship2;
  int vnum1, vnum2;

  reset_vnum_allocation();

  init_mock_ship(&ship1, 0, VESSEL_SHIP);
  init_mock_ship(&ship2, 1, VESSEL_SHIP);

  vnum1 = mock_create_ship_room(&ship1, ROOM_TYPE_BRIDGE);
  vnum2 = mock_create_ship_room(&ship2, ROOM_TYPE_BRIDGE);

  CuAssertIntEquals(tc, 70000, vnum1);
  CuAssertIntEquals(tc, 70020, vnum2);
}

/* ========================================================================= */
/* ROOM CONNECTION TESTS                                                     */
/* ========================================================================= */

/**
 * Test adding room connections
 */
void Test_room_connection_add(CuTest *tc)
{
  struct mock_ship_data ship;
  bool result;

  reset_vnum_allocation();
  init_mock_ship(&ship, 0, VESSEL_SHIP);

  mock_create_ship_room(&ship, ROOM_TYPE_BRIDGE);
  mock_create_ship_room(&ship, ROOM_TYPE_QUARTERS);

  result = mock_add_connection(&ship, 70000, 70001, 0);  /* North */

  CuAssertIntEquals(tc, TRUE, result);
  CuAssertIntEquals(tc, 1, ship.num_connections);
  CuAssertIntEquals(tc, 70000, ship.connections[0].from_room);
  CuAssertIntEquals(tc, 70001, ship.connections[0].to_room);
}

/**
 * Test connection limit
 */
void Test_room_connection_max_limit(CuTest *tc)
{
  struct mock_ship_data ship;
  int i;
  bool result;

  reset_vnum_allocation();
  init_mock_ship(&ship, 0, VESSEL_WARSHIP);

  /* Fill up connections */
  for (i = 0; i < MAX_SHIP_CONNECTIONS; i++)
  {
    result = mock_add_connection(&ship, i, i + 1, 0);
    CuAssertIntEquals(tc, TRUE, result);
  }

  /* Next should fail */
  result = mock_add_connection(&ship, 100, 101, 0);
  CuAssertIntEquals(tc, FALSE, result);
}

/* ========================================================================= */
/* INTERIOR DETECTION TESTS                                                  */
/* ========================================================================= */

/**
 * Test ship has interior rooms detection
 */
void Test_room_has_interior_empty(CuTest *tc)
{
  struct mock_ship_data ship;

  init_mock_ship(&ship, 0, VESSEL_SHIP);

  CuAssertIntEquals(tc, FALSE, ship_has_interior_rooms(&ship));
}

/**
 * Test ship has interior rooms with rooms
 */
void Test_room_has_interior_with_rooms(CuTest *tc)
{
  struct mock_ship_data ship;

  reset_vnum_allocation();
  init_mock_ship(&ship, 0, VESSEL_SHIP);

  mock_create_ship_room(&ship, ROOM_TYPE_BRIDGE);

  CuAssertIntEquals(tc, TRUE, ship_has_interior_rooms(&ship));
}

/**
 * Test ship has interior rooms NULL check
 */
void Test_room_has_interior_null(CuTest *tc)
{
  CuAssertIntEquals(tc, FALSE, ship_has_interior_rooms(NULL));
}

/* ========================================================================= */
/* BASE ROOMS PER TYPE TESTS                                                 */
/* ========================================================================= */

/**
 * Test base room counts by vessel type
 */
void Test_room_base_counts(CuTest *tc)
{
  CuAssertIntEquals(tc, 1, get_base_rooms_for_type(VESSEL_RAFT));
  CuAssertIntEquals(tc, 2, get_base_rooms_for_type(VESSEL_BOAT));
  CuAssertIntEquals(tc, 3, get_base_rooms_for_type(VESSEL_SHIP));
  CuAssertIntEquals(tc, 5, get_base_rooms_for_type(VESSEL_WARSHIP));
  CuAssertIntEquals(tc, 4, get_base_rooms_for_type(VESSEL_AIRSHIP));
  CuAssertIntEquals(tc, 4, get_base_rooms_for_type(VESSEL_SUBMARINE));
  CuAssertIntEquals(tc, 6, get_base_rooms_for_type(VESSEL_TRANSPORT));
  CuAssertIntEquals(tc, 3, get_base_rooms_for_type(VESSEL_MAGICAL));
}

/**
 * Test max room counts by vessel type
 */
void Test_room_max_counts(CuTest *tc)
{
  CuAssertIntEquals(tc, 2, get_max_rooms_for_type(VESSEL_RAFT));
  CuAssertIntEquals(tc, 4, get_max_rooms_for_type(VESSEL_BOAT));
  CuAssertIntEquals(tc, 8, get_max_rooms_for_type(VESSEL_SHIP));
  CuAssertIntEquals(tc, 15, get_max_rooms_for_type(VESSEL_WARSHIP));
  CuAssertIntEquals(tc, 10, get_max_rooms_for_type(VESSEL_AIRSHIP));
  CuAssertIntEquals(tc, 12, get_max_rooms_for_type(VESSEL_SUBMARINE));
  CuAssertIntEquals(tc, 20, get_max_rooms_for_type(VESSEL_TRANSPORT));
  CuAssertIntEquals(tc, 10, get_max_rooms_for_type(VESSEL_MAGICAL));
}

/* ========================================================================= */
/* TEST SUITE REGISTRATION                                                   */
/* ========================================================================= */

/**
 * Get the vessel rooms test suite
 */
CuSuite *VesselRoomsGetSuite(void)
{
  CuSuite *suite = CuSuiteNew();

  /* VNUM calculation tests */
  SUITE_ADD_TEST(suite, Test_room_vnum_first_ship);
  SUITE_ADD_TEST(suite, Test_room_vnum_multiple_ships);
  SUITE_ADD_TEST(suite, Test_room_vnum_range_limits);
  SUITE_ADD_TEST(suite, Test_room_vnum_invalid_params);

  /* Room creation tests */
  SUITE_ADD_TEST(suite, Test_room_create_basic);
  SUITE_ADD_TEST(suite, Test_room_create_multiple);
  SUITE_ADD_TEST(suite, Test_room_create_max_limit);
  SUITE_ADD_TEST(suite, Test_room_create_null_ship);

  /* Collision tests */
  SUITE_ADD_TEST(suite, Test_room_vnum_collision);
  SUITE_ADD_TEST(suite, Test_room_vnum_no_collision);

  /* Connection tests */
  SUITE_ADD_TEST(suite, Test_room_connection_add);
  SUITE_ADD_TEST(suite, Test_room_connection_max_limit);

  /* Interior detection tests */
  SUITE_ADD_TEST(suite, Test_room_has_interior_empty);
  SUITE_ADD_TEST(suite, Test_room_has_interior_with_rooms);
  SUITE_ADD_TEST(suite, Test_room_has_interior_null);

  /* Room count tests */
  SUITE_ADD_TEST(suite, Test_room_base_counts);
  SUITE_ADD_TEST(suite, Test_room_max_counts);

  return suite;
}
