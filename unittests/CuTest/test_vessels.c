/**
 * @file test_vessels.c
 * @brief Main vessel system unit test suite with fixtures and mocks
 *
 * This file provides the test fixture infrastructure and mock implementations
 * for isolated testing of the LuminariMUD vessel system. Tests are organized
 * into separate files per subsystem.
 *
 * Part of Phase 00, Session 09: Testing and Validation
 */

#include "CuTest.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ========================================================================= */
/* MOCK CONFIGURATION                                                        */
/* ========================================================================= */
/* Define VESSEL_UNIT_TEST to enable mock implementations */
#ifndef VESSEL_UNIT_TEST
#define VESSEL_UNIT_TEST 1
#endif

/* ========================================================================= */
/* MINIMAL TYPE DEFINITIONS FOR STANDALONE TESTING                           */
/* ========================================================================= */
/* These mirror the production types but are self-contained for unit tests   */

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

/* Vessel class enum (mirrors production) */
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

/* Sector types used in terrain validation */
#define SECT_INSIDE       0
#define SECT_CITY         1
#define SECT_FIELD        2
#define SECT_FOREST       3
#define SECT_HILLS        4
#define SECT_MOUNTAIN     5
#define SECT_WATER_SWIM   6
#define SECT_WATER_NOSWIM 7
#define SECT_FLYING       8
#define SECT_UNDERWATER   9
#define SECT_OCEAN       15
#define SECT_MARSHLAND   16
#define SECT_LAVA        25
#define SECT_CAVE        29
#define SECT_BEACH       33
#define SECT_SEAPORT     34
#define SECT_INSIDE_ROOM 35
#define SECT_RIVER       36
#define SECT_UD_WILD     19
#define SECT_UD_NOGROUND 24

/* Vessel terrain capabilities (mirrors production) */
struct vessel_terrain_caps {
  bool can_traverse_ocean;
  bool can_traverse_shallow;
  bool can_traverse_air;
  bool can_traverse_underwater;
  int min_water_depth;
  int max_altitude;
  float terrain_speed_mod[40];
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

/* ========================================================================= */
/* MOCK DATA STRUCTURES                                                      */
/* ========================================================================= */

#define MOCK_MAX_SHIPS 10
#define MOCK_MAX_ROOMS 20
#define MAX_SHIP_ROOMS 20
#define MAX_SHIP_CONNECTIONS 40
#define SHIP_INTERIOR_VNUM_BASE 70000
#define SHIP_INTERIOR_VNUM_MAX  79999
#define GREYHAWK_MAXSHIPS 500

/* Room connection structure */
struct room_connection {
  int from_room;
  int to_room;
  int direction;
  bool is_hatch;
  bool is_locked;
};

/* Simplified ship data for testing */
struct mock_ship_data {
  float x, y, z;
  int location;
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

/* Mock room data */
struct mock_room_data {
  room_vnum number;
  int sector_type;
  int room_flags;
  char *name;
  char *description;
};

/* Global mock data */
static struct mock_ship_data mock_ships[MOCK_MAX_SHIPS];
static struct mock_room_data mock_world[100];
static int mock_top_of_world = 0;

/* ========================================================================= */
/* MOCK FUNCTION IMPLEMENTATIONS                                             */
/* ========================================================================= */

/**
 * Mock wilderness room allocation
 * Returns a predictable room rnum based on coordinates
 */
static room_rnum mock_get_or_allocate_wilderness_room(int x, int y)
{
  /* Validate coordinates */
  if (x < -1024 || x > 1024 || y < -1024 || y > 1024)
  {
    return NOWHERE;
  }

  /* Return a mock room index based on hash of coordinates */
  /* Simple formula for test predictability */
  return (room_rnum)(((x + 1024) * 2049 + (y + 1024)) % 100);
}

/**
 * Mock room sector type getter
 */
static int mock_get_room_sector(room_rnum room)
{
  if (room < 0 || room >= 100)
  {
    return SECT_INSIDE;
  }
  return mock_world[room].sector_type;
}

/**
 * Reset all mock data to initial state
 * Called before each test for isolation
 */
void vessel_test_fixture_reset(void)
{
  int i, j;

  /* Clear ship data */
  for (i = 0; i < MOCK_MAX_SHIPS; i++)
  {
    memset(&mock_ships[i], 0, sizeof(struct mock_ship_data));
    mock_ships[i].shipnum = i;
    mock_ships[i].x = 0.0f;
    mock_ships[i].y = 0.0f;
    mock_ships[i].z = 0.0f;
    mock_ships[i].location = NOWHERE;
    mock_ships[i].vessel_type = VESSEL_SHIP;
    mock_ships[i].num_rooms = 0;
    snprintf(mock_ships[i].name, sizeof(mock_ships[i].name), "Test Ship %d", i);

    for (j = 0; j < MAX_SHIP_ROOMS; j++)
    {
      mock_ships[i].room_vnums[j] = 0;
    }
  }

  /* Clear room data with default sectors */
  for (i = 0; i < 100; i++)
  {
    mock_world[i].number = i;
    mock_world[i].sector_type = SECT_OCEAN;  /* Default to ocean for ships */
    mock_world[i].room_flags = 0;
    mock_world[i].name = NULL;
    mock_world[i].description = NULL;
  }

  mock_top_of_world = 99;
}

/**
 * Set up a ship at specific coordinates for testing
 */
void vessel_test_setup_ship(int shipnum, float x, float y, float z, enum vessel_class type)
{
  if (shipnum < 0 || shipnum >= MOCK_MAX_SHIPS)
  {
    return;
  }

  mock_ships[shipnum].x = x;
  mock_ships[shipnum].y = y;
  mock_ships[shipnum].z = z;
  mock_ships[shipnum].vessel_type = type;
}

/**
 * Set terrain type for a mock room
 */
void vessel_test_set_terrain(room_rnum room, int sector_type)
{
  if (room >= 0 && room < 100)
  {
    mock_world[room].sector_type = sector_type;
  }
}

/* ========================================================================= */
/* COORDINATE SYSTEM VALIDATION FUNCTIONS (TESTED)                           */
/* ========================================================================= */

/**
 * Validate wilderness coordinates are within bounds
 * @return TRUE if valid, FALSE otherwise
 */
int vessel_validate_coordinates(int x, int y, int z)
{
  /* X and Y must be in range -1024 to +1024 */
  if (x < -1024 || x > 1024)
  {
    return FALSE;
  }
  if (y < -1024 || y > 1024)
  {
    return FALSE;
  }
  /* Z has different limits for submarines (negative) and airships (positive) */
  if (z < -500 || z > 500)
  {
    return FALSE;
  }
  return TRUE;
}

/* ========================================================================= */
/* VESSEL TYPE FUNCTIONS (TESTED)                                            */
/* ========================================================================= */

/**
 * Get base number of rooms for vessel type
 */
int test_get_base_rooms_for_type(enum vessel_class type)
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
 * Get maximum number of rooms for vessel type
 */
int test_get_max_rooms_for_type(enum vessel_class type)
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
 * Derive vessel type from hull weight
 */
enum vessel_class test_derive_vessel_type_from_template(int hullweight)
{
  if (hullweight < 50)
  {
    return VESSEL_RAFT;
  }
  else if (hullweight < 150)
  {
    return VESSEL_BOAT;
  }
  else if (hullweight < 400)
  {
    return VESSEL_SHIP;
  }
  else if (hullweight < 800)
  {
    return VESSEL_WARSHIP;
  }
  else
  {
    return VESSEL_TRANSPORT;
  }
}

/**
 * Get vessel type name
 */
const char *test_get_vessel_type_name(enum vessel_class type)
{
  static const char *names[] = {
    "Raft",
    "Boat",
    "Ship",
    "Warship",
    "Airship",
    "Submarine",
    "Transport",
    "Magical Vessel"
  };

  if (type < 0 || type > VESSEL_MAGICAL)
  {
    return "Unknown";
  }

  return names[type];
}

/* ========================================================================= */
/* ROOM GENERATION FUNCTIONS (TESTED)                                        */
/* ========================================================================= */

/**
 * Calculate VNUM for a ship room
 * @return Valid VNUM or -1 on error
 */
int test_calculate_room_vnum(int shipnum, int room_index)
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
 * Check if ship has interior rooms
 */
int test_ship_has_interior_rooms(struct mock_ship_data *ship)
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

/* ========================================================================= */
/* PLACEHOLDER TESTS (to be expanded in separate files)                      */
/* ========================================================================= */

/**
 * Basic sanity test - verify test framework works
 */
void Test_vessel_test_framework_sanity(CuTest *tc)
{
  /* Reset fixture */
  vessel_test_fixture_reset();

  /* Verify mock data initialized */
  CuAssertIntEquals(tc, 0, mock_ships[0].shipnum);
  CuAssertIntEquals(tc, VESSEL_SHIP, mock_ships[0].vessel_type);
  CuAssertIntEquals(tc, SECT_OCEAN, mock_world[0].sector_type);
}

/**
 * Test coordinate validation at boundaries
 */
void Test_vessel_coordinate_boundaries(CuTest *tc)
{
  vessel_test_fixture_reset();

  /* Valid at exact boundaries */
  CuAssertIntEquals(tc, TRUE, vessel_validate_coordinates(-1024, 0, 0));
  CuAssertIntEquals(tc, TRUE, vessel_validate_coordinates(1024, 0, 0));
  CuAssertIntEquals(tc, TRUE, vessel_validate_coordinates(0, -1024, 0));
  CuAssertIntEquals(tc, TRUE, vessel_validate_coordinates(0, 1024, 0));
  CuAssertIntEquals(tc, TRUE, vessel_validate_coordinates(0, 0, -500));
  CuAssertIntEquals(tc, TRUE, vessel_validate_coordinates(0, 0, 500));

  /* Invalid just outside boundaries */
  CuAssertIntEquals(tc, FALSE, vessel_validate_coordinates(-1025, 0, 0));
  CuAssertIntEquals(tc, FALSE, vessel_validate_coordinates(1025, 0, 0));
  CuAssertIntEquals(tc, FALSE, vessel_validate_coordinates(0, -1025, 0));
  CuAssertIntEquals(tc, FALSE, vessel_validate_coordinates(0, 1025, 0));
  CuAssertIntEquals(tc, FALSE, vessel_validate_coordinates(0, 0, -501));
  CuAssertIntEquals(tc, FALSE, vessel_validate_coordinates(0, 0, 501));
}

/**
 * Test vessel type room counts
 */
void Test_vessel_type_room_counts(CuTest *tc)
{
  /* Base rooms */
  CuAssertIntEquals(tc, 1, test_get_base_rooms_for_type(VESSEL_RAFT));
  CuAssertIntEquals(tc, 2, test_get_base_rooms_for_type(VESSEL_BOAT));
  CuAssertIntEquals(tc, 3, test_get_base_rooms_for_type(VESSEL_SHIP));
  CuAssertIntEquals(tc, 5, test_get_base_rooms_for_type(VESSEL_WARSHIP));
  CuAssertIntEquals(tc, 6, test_get_base_rooms_for_type(VESSEL_TRANSPORT));

  /* Max rooms */
  CuAssertIntEquals(tc, 2, test_get_max_rooms_for_type(VESSEL_RAFT));
  CuAssertIntEquals(tc, 4, test_get_max_rooms_for_type(VESSEL_BOAT));
  CuAssertIntEquals(tc, 8, test_get_max_rooms_for_type(VESSEL_SHIP));
  CuAssertIntEquals(tc, 15, test_get_max_rooms_for_type(VESSEL_WARSHIP));
  CuAssertIntEquals(tc, 20, test_get_max_rooms_for_type(VESSEL_TRANSPORT));

  /* Max should always be >= base */
  CuAssertTrue(tc, test_get_max_rooms_for_type(VESSEL_RAFT) >= test_get_base_rooms_for_type(VESSEL_RAFT));
  CuAssertTrue(tc, test_get_max_rooms_for_type(VESSEL_SHIP) >= test_get_base_rooms_for_type(VESSEL_SHIP));
}

/**
 * Test vessel type derivation from hull weight
 */
void Test_vessel_type_derivation(CuTest *tc)
{
  /* Raft: < 50 */
  CuAssertIntEquals(tc, VESSEL_RAFT, test_derive_vessel_type_from_template(0));
  CuAssertIntEquals(tc, VESSEL_RAFT, test_derive_vessel_type_from_template(49));

  /* Boat: 50-149 */
  CuAssertIntEquals(tc, VESSEL_BOAT, test_derive_vessel_type_from_template(50));
  CuAssertIntEquals(tc, VESSEL_BOAT, test_derive_vessel_type_from_template(149));

  /* Ship: 150-399 */
  CuAssertIntEquals(tc, VESSEL_SHIP, test_derive_vessel_type_from_template(150));
  CuAssertIntEquals(tc, VESSEL_SHIP, test_derive_vessel_type_from_template(399));

  /* Warship: 400-799 */
  CuAssertIntEquals(tc, VESSEL_WARSHIP, test_derive_vessel_type_from_template(400));
  CuAssertIntEquals(tc, VESSEL_WARSHIP, test_derive_vessel_type_from_template(799));

  /* Transport: 800+ */
  CuAssertIntEquals(tc, VESSEL_TRANSPORT, test_derive_vessel_type_from_template(800));
  CuAssertIntEquals(tc, VESSEL_TRANSPORT, test_derive_vessel_type_from_template(10000));
}

/**
 * Test VNUM calculation for ship rooms
 */
void Test_vessel_room_vnum_calculation(CuTest *tc)
{
  /* First ship, first room */
  CuAssertIntEquals(tc, 70000, test_calculate_room_vnum(0, 0));

  /* First ship, last possible room */
  CuAssertIntEquals(tc, 70019, test_calculate_room_vnum(0, 19));

  /* Second ship, first room */
  CuAssertIntEquals(tc, 70020, test_calculate_room_vnum(1, 0));

  /* Invalid ship number */
  CuAssertIntEquals(tc, -1, test_calculate_room_vnum(-1, 0));
  CuAssertIntEquals(tc, -1, test_calculate_room_vnum(GREYHAWK_MAXSHIPS, 0));

  /* Invalid room index */
  CuAssertIntEquals(tc, -1, test_calculate_room_vnum(0, -1));
  CuAssertIntEquals(tc, -1, test_calculate_room_vnum(0, MAX_SHIP_ROOMS));
}

/**
 * Test ship interior detection
 */
void Test_vessel_has_interior_rooms(CuTest *tc)
{
  struct mock_ship_data ship;

  /* Empty ship has no rooms */
  memset(&ship, 0, sizeof(ship));
  CuAssertIntEquals(tc, FALSE, test_ship_has_interior_rooms(&ship));

  /* Ship with num_rooms set */
  ship.num_rooms = 3;
  CuAssertIntEquals(tc, TRUE, test_ship_has_interior_rooms(&ship));

  /* Ship with first vnum set but num_rooms zero */
  ship.num_rooms = 0;
  ship.room_vnums[0] = 70000;
  CuAssertIntEquals(tc, TRUE, test_ship_has_interior_rooms(&ship));

  /* NULL check */
  CuAssertIntEquals(tc, FALSE, test_ship_has_interior_rooms(NULL));
}

/**
 * Test vessel type name lookup
 */
void Test_vessel_type_names(CuTest *tc)
{
  CuAssertStrEquals(tc, "Raft", test_get_vessel_type_name(VESSEL_RAFT));
  CuAssertStrEquals(tc, "Ship", test_get_vessel_type_name(VESSEL_SHIP));
  CuAssertStrEquals(tc, "Warship", test_get_vessel_type_name(VESSEL_WARSHIP));
  CuAssertStrEquals(tc, "Airship", test_get_vessel_type_name(VESSEL_AIRSHIP));
  CuAssertStrEquals(tc, "Submarine", test_get_vessel_type_name(VESSEL_SUBMARINE));

  /* Invalid type */
  CuAssertStrEquals(tc, "Unknown", test_get_vessel_type_name(-1));
  CuAssertStrEquals(tc, "Unknown", test_get_vessel_type_name(100));
}

/* ========================================================================= */
/* TEST SUITE REGISTRATION                                                   */
/* ========================================================================= */

/**
 * Get the vessel test suite
 */
CuSuite *VesselGetSuite(void)
{
  CuSuite *suite = CuSuiteNew();

  /* Framework tests */
  SUITE_ADD_TEST(suite, Test_vessel_test_framework_sanity);

  /* Coordinate tests */
  SUITE_ADD_TEST(suite, Test_vessel_coordinate_boundaries);

  /* Type tests */
  SUITE_ADD_TEST(suite, Test_vessel_type_room_counts);
  SUITE_ADD_TEST(suite, Test_vessel_type_derivation);
  SUITE_ADD_TEST(suite, Test_vessel_type_names);

  /* Room tests */
  SUITE_ADD_TEST(suite, Test_vessel_room_vnum_calculation);
  SUITE_ADD_TEST(suite, Test_vessel_has_interior_rooms);

  return suite;
}
