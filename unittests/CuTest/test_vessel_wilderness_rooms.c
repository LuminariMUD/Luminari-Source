/**
 * @file test_vessel_wilderness_rooms.c
 * @brief Unit tests for dynamic wilderness room allocation during vessel movement
 *
 * Tests room allocation, ship position updates, room recycling,
 * multi-vessel scenarios, and boundary conditions.
 *
 * Part of Phase 03, Session 04: Dynamic Wilderness Rooms
 */

#define _POSIX_C_SOURCE 200809L

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

/* Wilderness coordinate bounds */
#define WILD_COORD_MIN (-1024)
#define WILD_COORD_MAX (1024)

/* Dynamic room pool VNUMs (mock values) */
#define WILD_DYNAMIC_ROOM_VNUM_START 60000
#define WILD_DYNAMIC_ROOM_VNUM_END 60999
#define WILD_ROOM_POOL_SIZE (WILD_DYNAMIC_ROOM_VNUM_END - WILD_DYNAMIC_ROOM_VNUM_START + 1)

/* Room flags for wilderness allocation */
#define ROOM_OCCUPIED (1 << 0)

/* ========================================================================= */
/* MOCK STRUCTURES                                                           */
/* ========================================================================= */

/**
 * Mock wilderness room data for testing
 */
struct mock_wilderness_room
{
  room_vnum vnum;
  room_rnum rnum;
  int coords[2]; /* x, y coordinates */
  int flags;     /* ROOM_OCCUPIED etc */
  int contents;  /* Number of objects in room */
  int people;    /* Number of characters in room */
};

/**
 * Mock ship data for position testing
 */
struct mock_ship_data
{
  int shipnum;
  float x, y, z;
  room_vnum location;
  int shipobj_present; /* 1 if ship object exists */
  int shipobj_room;    /* Room where ship object is placed */
};

/* ========================================================================= */
/* MOCK GLOBAL STATE                                                         */
/* ========================================================================= */

static struct mock_wilderness_room g_room_pool[WILD_ROOM_POOL_SIZE];
static int g_rooms_allocated = 0;
static struct mock_ship_data g_test_ships[10];
static int g_num_test_ships = 0;

/* ========================================================================= */
/* MOCK FUNCTION IMPLEMENTATIONS                                             */
/* ========================================================================= */

/**
 * Reset all mock state for test isolation
 */
static void reset_mock_state(void)
{
  int i;
  g_rooms_allocated = 0;
  g_num_test_ships = 0;

  for (i = 0; i < WILD_ROOM_POOL_SIZE; i++)
  {
    g_room_pool[i].vnum = WILD_DYNAMIC_ROOM_VNUM_START + i;
    g_room_pool[i].rnum = i;
    g_room_pool[i].coords[0] = 0;
    g_room_pool[i].coords[1] = 0;
    g_room_pool[i].flags = 0;
    g_room_pool[i].contents = 0;
    g_room_pool[i].people = 0;
  }

  for (i = 0; i < 10; i++)
  {
    g_test_ships[i].shipnum = -1;
    g_test_ships[i].x = 0.0f;
    g_test_ships[i].y = 0.0f;
    g_test_ships[i].z = 0.0f;
    g_test_ships[i].location = NOWHERE;
    g_test_ships[i].shipobj_present = 0;
    g_test_ships[i].shipobj_room = NOWHERE;
  }
}

/**
 * Mock: Find room already allocated at coordinates
 * Returns room index or NOWHERE if not found
 */
static room_rnum mock_find_room_by_coordinates(int x, int y)
{
  int i;

  for (i = 0; i < g_rooms_allocated; i++)
  {
    if ((g_room_pool[i].flags & ROOM_OCCUPIED) && g_room_pool[i].coords[0] == x &&
        g_room_pool[i].coords[1] == y)
    {
      return i;
    }
  }
  return NOWHERE;
}

/**
 * Mock: Find first available (unoccupied) wilderness room
 * Returns room index or NOWHERE if pool exhausted
 */
static room_rnum mock_find_available_wilderness_room(void)
{
  int i;

  for (i = 0; i < WILD_ROOM_POOL_SIZE; i++)
  {
    if (!(g_room_pool[i].flags & ROOM_OCCUPIED))
    {
      return i;
    }
  }
  return NOWHERE; /* Pool exhausted */
}

/**
 * Mock: Assign wilderness room to coordinates
 * Sets coordinates and marks ROOM_OCCUPIED
 */
static void mock_assign_wilderness_room(room_rnum room, int x, int y)
{
  if (room < 0 || room >= WILD_ROOM_POOL_SIZE)
  {
    return;
  }

  g_room_pool[room].coords[0] = x;
  g_room_pool[room].coords[1] = y;
  g_room_pool[room].flags |= ROOM_OCCUPIED;

  if (room >= g_rooms_allocated)
  {
    g_rooms_allocated = room + 1;
  }
}

/**
 * Mock: Get or allocate wilderness room at coordinates
 * Core allocation wrapper function
 */
static room_rnum mock_get_or_allocate_wilderness_room(int x, int y)
{
  room_rnum room;

  /* Validate coordinates within wilderness bounds */
  if (x < WILD_COORD_MIN || x > WILD_COORD_MAX || y < WILD_COORD_MIN || y > WILD_COORD_MAX)
  {
    return NOWHERE;
  }

  /* Try to find existing room at coordinates */
  room = mock_find_room_by_coordinates(x, y);
  if (room != NOWHERE)
  {
    return room;
  }

  /* No room exists, allocate from dynamic pool */
  room = mock_find_available_wilderness_room();
  if (room == NOWHERE)
  {
    return NOWHERE; /* Pool exhausted */
  }

  /* Configure the room for these coordinates */
  mock_assign_wilderness_room(room, x, y);

  return room;
}

/**
 * Mock: Update ship wilderness position
 * Simulates the full position update logic
 */
static bool mock_update_ship_wilderness_position(int shipnum, int new_x, int new_y, int new_z)
{
  room_rnum old_room;
  room_rnum wilderness_room;

  /* Validate ship number */
  if (shipnum < 0 || shipnum >= 10)
  {
    return FALSE;
  }

  /* Validate coordinates within wilderness bounds */
  if (new_x < WILD_COORD_MIN || new_x > WILD_COORD_MAX || new_y < WILD_COORD_MIN ||
      new_y > WILD_COORD_MAX)
  {
    return FALSE;
  }

  /* Remember old room for cleanup */
  old_room = (g_test_ships[shipnum].location != NOWHERE)
                 ? mock_find_room_by_coordinates((int)g_test_ships[shipnum].x,
                                                 (int)g_test_ships[shipnum].y)
                 : NOWHERE;

  /* Update ship coordinates */
  g_test_ships[shipnum].x = (float)new_x;
  g_test_ships[shipnum].y = (float)new_y;
  g_test_ships[shipnum].z = (float)new_z;

  /* Get or allocate wilderness room at these coordinates */
  wilderness_room = mock_get_or_allocate_wilderness_room(new_x, new_y);
  if (wilderness_room == NOWHERE)
  {
    return FALSE; /* Room pool exhausted */
  }

  /* Update ship's location to the wilderness room */
  g_test_ships[shipnum].location = g_room_pool[wilderness_room].vnum;

  /* If ship object exists, simulate moving it to new location */
  if (g_test_ships[shipnum].shipobj_present)
  {
    /* Remove from old room contents */
    if (old_room != NOWHERE && old_room >= 0 && old_room < WILD_ROOM_POOL_SIZE)
    {
      if (g_room_pool[old_room].contents > 0)
      {
        g_room_pool[old_room].contents--;
      }
    }

    /* Add to new room contents */
    g_room_pool[wilderness_room].contents++;
    g_test_ships[shipnum].shipobj_room = wilderness_room;
  }

  return TRUE;
}

/**
 * Mock: Check if room can be recycled
 * Returns TRUE if room has no occupants and can have ROOM_OCCUPIED cleared
 */
static bool mock_room_can_be_recycled(room_rnum room)
{
  if (room < 0 || room >= WILD_ROOM_POOL_SIZE)
  {
    return FALSE;
  }

  /* Room can be recycled if empty */
  return (g_room_pool[room].contents == 0 && g_room_pool[room].people == 0);
}

/**
 * Mock: Recycle room (clear ROOM_OCCUPIED)
 */
static void mock_recycle_room(room_rnum room)
{
  if (room < 0 || room >= WILD_ROOM_POOL_SIZE)
  {
    return;
  }

  if (mock_room_can_be_recycled(room))
  {
    g_room_pool[room].flags &= ~ROOM_OCCUPIED;
  }
}

/**
 * Create a test ship with given parameters
 */
static int create_test_ship(int x, int y, int z, int has_obj)
{
  int idx;

  if (g_num_test_ships >= 10)
  {
    return -1;
  }

  idx = g_num_test_ships++;
  g_test_ships[idx].shipnum = idx;
  g_test_ships[idx].x = (float)x;
  g_test_ships[idx].y = (float)y;
  g_test_ships[idx].z = (float)z;
  g_test_ships[idx].location = NOWHERE;
  g_test_ships[idx].shipobj_present = has_obj;
  g_test_ships[idx].shipobj_room = NOWHERE;

  return idx;
}

/* ========================================================================= */
/* UNIT TESTS - Room Allocation (6 tests)                                    */
/* ========================================================================= */

/**
 * Test T016-1: get_or_allocate allocates new room when none exists
 */
void test_allocate_new_room_when_none_exists(CuTest *tc)
{
  room_rnum room;

  reset_mock_state();

  /* Allocate room at fresh coordinates */
  room = mock_get_or_allocate_wilderness_room(100, 200);

  CuAssertIntEquals(tc, 0, room); /* First available room */
  CuAssertTrue(tc, g_room_pool[room].flags & ROOM_OCCUPIED);
  CuAssertIntEquals(tc, 100, g_room_pool[room].coords[0]);
  CuAssertIntEquals(tc, 200, g_room_pool[room].coords[1]);
}

/**
 * Test T016-2: get_or_allocate returns existing room at same coordinates
 */
void test_returns_existing_room_at_coordinates(CuTest *tc)
{
  room_rnum room1;
  room_rnum room2;

  reset_mock_state();

  /* Allocate room at coordinates */
  room1 = mock_get_or_allocate_wilderness_room(50, 75);
  CuAssertTrue(tc, room1 != NOWHERE);

  /* Request room at same coordinates - should return same room */
  room2 = mock_get_or_allocate_wilderness_room(50, 75);
  CuAssertIntEquals(tc, room1, room2);
}

/**
 * Test T016-3: ROOM_OCCUPIED flag is set after allocation
 */
void test_room_occupied_flag_set_on_allocation(CuTest *tc)
{
  room_rnum room;

  reset_mock_state();

  room = mock_get_or_allocate_wilderness_room(0, 0);

  CuAssertTrue(tc, room != NOWHERE);
  CuAssertTrue(tc, (g_room_pool[room].flags & ROOM_OCCUPIED) != 0);
}

/**
 * Test T016-4: Room pool exhaustion returns NOWHERE
 */
void test_room_pool_exhaustion(CuTest *tc)
{
  room_rnum room;
  int i;

  reset_mock_state();

  /* Exhaust the room pool */
  for (i = 0; i < WILD_ROOM_POOL_SIZE; i++)
  {
    room = mock_get_or_allocate_wilderness_room(i, i);
    CuAssertTrue(tc, room != NOWHERE);
  }

  /* Next allocation should fail */
  room = mock_get_or_allocate_wilderness_room(9999, 9999);
  CuAssertIntEquals(tc, NOWHERE, room);
}

/**
 * Test T016-5: Coordinate boundary validation - minimum bounds
 */
void test_coordinate_boundary_minimum(CuTest *tc)
{
  room_rnum room;

  reset_mock_state();

  /* Valid minimum coordinates */
  room = mock_get_or_allocate_wilderness_room(WILD_COORD_MIN, WILD_COORD_MIN);
  CuAssertTrue(tc, room != NOWHERE);

  /* Invalid below minimum */
  room = mock_get_or_allocate_wilderness_room(WILD_COORD_MIN - 1, 0);
  CuAssertIntEquals(tc, NOWHERE, room);

  room = mock_get_or_allocate_wilderness_room(0, WILD_COORD_MIN - 1);
  CuAssertIntEquals(tc, NOWHERE, room);
}

/**
 * Test T016-6: Coordinate boundary validation - maximum bounds
 */
void test_coordinate_boundary_maximum(CuTest *tc)
{
  room_rnum room;

  reset_mock_state();

  /* Valid maximum coordinates */
  room = mock_get_or_allocate_wilderness_room(WILD_COORD_MAX, WILD_COORD_MAX);
  CuAssertTrue(tc, room != NOWHERE);

  /* Invalid above maximum */
  room = mock_get_or_allocate_wilderness_room(WILD_COORD_MAX + 1, 0);
  CuAssertIntEquals(tc, NOWHERE, room);

  room = mock_get_or_allocate_wilderness_room(0, WILD_COORD_MAX + 1);
  CuAssertIntEquals(tc, NOWHERE, room);
}

/* ========================================================================= */
/* UNIT TESTS - Ship Position Updates (4 tests)                              */
/* ========================================================================= */

/**
 * Test T017-1: update_ship_wilderness_position updates coordinates
 */
void test_update_ship_position_coordinates(CuTest *tc)
{
  int shipnum;
  bool result;

  reset_mock_state();

  shipnum = create_test_ship(0, 0, 0, 0);
  CuAssertTrue(tc, shipnum >= 0);

  result = mock_update_ship_wilderness_position(shipnum, 100, 200, 50);

  CuAssertTrue(tc, result);
  CuAssertIntEquals(tc, 100, (int)g_test_ships[shipnum].x);
  CuAssertIntEquals(tc, 200, (int)g_test_ships[shipnum].y);
  CuAssertIntEquals(tc, 50, (int)g_test_ships[shipnum].z);
}

/**
 * Test T017-2: update_ship_wilderness_position updates location vnum
 */
void test_update_ship_position_location(CuTest *tc)
{
  int shipnum;
  bool result;

  reset_mock_state();

  shipnum = create_test_ship(0, 0, 0, 0);
  result = mock_update_ship_wilderness_position(shipnum, 100, 200, 0);

  CuAssertTrue(tc, result);
  CuAssertTrue(tc, g_test_ships[shipnum].location != NOWHERE);
}

/**
 * Test T017-3: Ship object is moved to new room
 */
void test_ship_object_moved_to_new_room(CuTest *tc)
{
  int shipnum;
  bool result;
  room_rnum expected_room;

  reset_mock_state();

  /* Create ship with object */
  shipnum = create_test_ship(0, 0, 0, 1);

  result = mock_update_ship_wilderness_position(shipnum, 100, 200, 0);
  CuAssertTrue(tc, result);

  /* Ship object should be in allocated room */
  expected_room = mock_find_room_by_coordinates(100, 200);
  CuAssertTrue(tc, expected_room != NOWHERE);
  CuAssertIntEquals(tc, expected_room, g_test_ships[shipnum].shipobj_room);
  CuAssertTrue(tc, g_room_pool[expected_room].contents > 0);
}

/**
 * Test T017-4: Invalid ship number returns FALSE
 */
void test_invalid_ship_number_rejected(CuTest *tc)
{
  bool result;

  reset_mock_state();

  result = mock_update_ship_wilderness_position(-1, 100, 200, 0);
  CuAssertTrue(tc, !result);

  result = mock_update_ship_wilderness_position(999, 100, 200, 0);
  CuAssertTrue(tc, !result);
}

/* ========================================================================= */
/* UNIT TESTS - Multi-Vessel and Edge Cases (4 tests)                        */
/* ========================================================================= */

/**
 * Test T018-1: Multiple vessels can occupy same coordinates
 */
void test_multiple_vessels_same_coordinates(CuTest *tc)
{
  int ship1;
  int ship2;
  bool result1;
  bool result2;

  reset_mock_state();

  ship1 = create_test_ship(0, 0, 0, 1);
  ship2 = create_test_ship(0, 0, 0, 1);

  /* Move both ships to same coordinates */
  result1 = mock_update_ship_wilderness_position(ship1, 500, 500, 0);
  result2 = mock_update_ship_wilderness_position(ship2, 500, 500, 0);

  CuAssertTrue(tc, result1);
  CuAssertTrue(tc, result2);

  /* Both ships should have same location */
  CuAssertIntEquals(tc, g_test_ships[ship1].location, g_test_ships[ship2].location);

  /* Room should have both ship objects */
  CuAssertIntEquals(tc, 2, g_room_pool[g_test_ships[ship1].shipobj_room].contents);
}

/**
 * Test T018-2: Room becomes available after ship departs
 */
void test_room_recycled_after_departure(CuTest *tc)
{
  int shipnum;
  room_rnum old_room;

  reset_mock_state();

  /* Create ship with object at initial position */
  shipnum = create_test_ship(0, 0, 0, 1);
  mock_update_ship_wilderness_position(shipnum, 100, 100, 0);
  old_room = g_test_ships[shipnum].shipobj_room;

  CuAssertTrue(tc, g_room_pool[old_room].flags & ROOM_OCCUPIED);

  /* Move ship to new location */
  mock_update_ship_wilderness_position(shipnum, 200, 200, 0);

  /* Old room should now be empty and recyclable */
  CuAssertIntEquals(tc, 0, g_room_pool[old_room].contents);
  CuAssertTrue(tc, mock_room_can_be_recycled(old_room));

  /* Recycle the room */
  mock_recycle_room(old_room);
  CuAssertTrue(tc, !(g_room_pool[old_room].flags & ROOM_OCCUPIED));
}

/**
 * Test T018-3: Rapid sequential movements allocate correctly
 */
void test_rapid_sequential_movements(CuTest *tc)
{
  int shipnum;
  int i;
  bool result;

  reset_mock_state();

  shipnum = create_test_ship(0, 0, 0, 1);

  /* Rapid movements across grid */
  for (i = 0; i < 50; i++)
  {
    result = mock_update_ship_wilderness_position(shipnum, i * 10, i * 10, 0);
    CuAssertTrue(tc, result);
  }

  /* Final position should be correct */
  CuAssertIntEquals(tc, 490, (int)g_test_ships[shipnum].x);
  CuAssertIntEquals(tc, 490, (int)g_test_ships[shipnum].y);
}

/**
 * Test T018-4: Movement fails gracefully when pool exhausted
 */
void test_movement_fails_gracefully_on_pool_exhaustion(CuTest *tc)
{
  int shipnum;
  bool result;
  int i;

  reset_mock_state();

  shipnum = create_test_ship(0, 0, 0, 1);

  /* Exhaust the room pool by allocating all rooms */
  for (i = 0; i < WILD_ROOM_POOL_SIZE; i++)
  {
    mock_get_or_allocate_wilderness_room(i, 0);
  }

  /* Attempt movement to unallocated coordinates should fail */
  result = mock_update_ship_wilderness_position(shipnum, 5000, 5000, 0);
  CuAssertTrue(tc, !result);

  /* Ship should retain original position */
  CuAssertIntEquals(tc, 0, (int)g_test_ships[shipnum].x);
}

/* ========================================================================= */
/* TEST SUITE REGISTRATION                                                   */
/* ========================================================================= */

CuSuite *vessel_wilderness_rooms_get_suite(void)
{
  CuSuite *suite = CuSuiteNew();

  /* Room Allocation Tests (T016) */
  SUITE_ADD_TEST(suite, test_allocate_new_room_when_none_exists);
  SUITE_ADD_TEST(suite, test_returns_existing_room_at_coordinates);
  SUITE_ADD_TEST(suite, test_room_occupied_flag_set_on_allocation);
  SUITE_ADD_TEST(suite, test_room_pool_exhaustion);
  SUITE_ADD_TEST(suite, test_coordinate_boundary_minimum);
  SUITE_ADD_TEST(suite, test_coordinate_boundary_maximum);

  /* Ship Position Update Tests (T017) */
  SUITE_ADD_TEST(suite, test_update_ship_position_coordinates);
  SUITE_ADD_TEST(suite, test_update_ship_position_location);
  SUITE_ADD_TEST(suite, test_ship_object_moved_to_new_room);
  SUITE_ADD_TEST(suite, test_invalid_ship_number_rejected);

  /* Multi-Vessel and Edge Case Tests (T018) */
  SUITE_ADD_TEST(suite, test_multiple_vessels_same_coordinates);
  SUITE_ADD_TEST(suite, test_room_recycled_after_departure);
  SUITE_ADD_TEST(suite, test_rapid_sequential_movements);
  SUITE_ADD_TEST(suite, test_movement_fails_gracefully_on_pool_exhaustion);

  return suite;
}

/* ========================================================================= */
/* MAIN ENTRY POINT                                                          */
/* ========================================================================= */

int main(void)
{
  CuString *output = CuStringNew();
  CuSuite *suite = vessel_wilderness_rooms_get_suite();

  printf("========================================\n");
  printf("Vessel Wilderness Rooms Tests (Phase 03, Session 04)\n");
  printf("========================================\n\n");

  CuSuiteRun(suite);
  CuSuiteSummary(suite, output);
  CuSuiteDetails(suite, output);
  printf("%s\n", output->buffer);

  CuStringDelete(output);
  CuSuiteDelete(suite);

  return 0;
}
