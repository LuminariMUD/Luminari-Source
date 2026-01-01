/* ************************************************************************
 *      File:   vehicle_transport_tests.c              Part of LuminariMUD  *
 *   Purpose:   Unit tests for vehicle transport (vehicle-in-vessel)        *
 *              Phase 02 Session 05: Vehicle-in-Vehicle Mechanics           *
 * ********************************************************************** */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "CuTest.h"

/* ========================================================================= */
/* MOCK DEFINITIONS (for standalone testing)                                 */
/* ========================================================================= */

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
  VSTATE_ON_VESSEL, /* Added in S0205 */
  NUM_VEHICLE_STATES
};

/* Constants */
#define VEHICLE_NAME_LENGTH 64
#define NOWHERE -1
#define MAX_VEHICLES_PER_VESSEL 10
#define MAX_VEHICLES 100

typedef int room_rnum;

/* Mock obj_data */
struct obj_data
{
  int placeholder;
};

/* Mock char_data */
struct char_data
{
  int id;
  char name[64];
};

/* Vehicle data structure (with parent_vessel_id from S0205) */
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
  int parent_vessel_id; /* Added in S0205 */
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

/* Mock greyhawk_ship_data (vessel) */
struct greyhawk_ship_data
{
  int shipnum;
  char name[64];
  float x, y, z;
  int speed;
  int docked_to_ship;
};

/* ========================================================================= */
/* MOCK VEHICLE LIST FOR TESTING                                             */
/* ========================================================================= */

static struct vehicle_data *test_vehicles[MAX_VEHICLES];
static int test_vehicle_count = 0;

static void reset_test_vehicles(void)
{
  int i;
  for (i = 0; i < MAX_VEHICLES; i++)
  {
    if (test_vehicles[i] != NULL)
    {
      free(test_vehicles[i]);
      test_vehicles[i] = NULL;
    }
  }
  test_vehicle_count = 0;
}

static struct vehicle_data *create_test_vehicle(int id, enum vehicle_type type, const char *name)
{
  struct vehicle_data *v;

  if (test_vehicle_count >= MAX_VEHICLES)
    return NULL;

  v = (struct vehicle_data *)malloc(sizeof(struct vehicle_data));
  if (v == NULL)
    return NULL;

  memset(v, 0, sizeof(struct vehicle_data));
  v->id = id;
  v->type = type;
  v->state = VSTATE_IDLE;
  strncpy(v->name, name, VEHICLE_NAME_LENGTH - 1);
  v->name[VEHICLE_NAME_LENGTH - 1] = '\0';
  v->parent_vessel_id = 0;
  v->max_passengers = 4;
  v->current_passengers = 0;
  v->max_weight = 1000;
  v->current_weight = 0;
  v->max_condition = 100;
  v->condition = 100;

  test_vehicles[test_vehicle_count++] = v;
  return v;
}

static struct greyhawk_ship_data *create_test_vessel(int id, const char *name)
{
  struct greyhawk_ship_data *v;

  v = (struct greyhawk_ship_data *)malloc(sizeof(struct greyhawk_ship_data));
  if (v == NULL)
    return NULL;

  memset(v, 0, sizeof(struct greyhawk_ship_data));
  v->shipnum = id;
  strncpy(v->name, name, 63);
  v->name[63] = '\0';
  v->x = 100.0;
  v->y = 100.0;
  v->z = 0.0;
  v->speed = 0;
  v->docked_to_ship = -1;

  return v;
}

/* ========================================================================= */
/* MOCK TRANSPORT FUNCTIONS (matching vehicles_transport.c signatures)      */
/* ========================================================================= */

static int mock_is_vessel_stationary(struct greyhawk_ship_data *vessel)
{
  if (vessel == NULL)
    return 0;
  return (vessel->speed == 0 || vessel->docked_to_ship >= 0);
}

static int mock_check_capacity(struct greyhawk_ship_data *vessel, struct vehicle_data *vehicle)
{
  int count = 0;
  int i;

  if (vessel == NULL || vehicle == NULL)
    return 0;

  for (i = 0; i < test_vehicle_count; i++)
  {
    if (test_vehicles[i] != NULL && test_vehicles[i]->parent_vessel_id == vessel->shipnum)
    {
      count++;
    }
  }

  return (count < MAX_VEHICLES_PER_VESSEL);
}

static int mock_load_vehicle(struct vehicle_data *vehicle, struct greyhawk_ship_data *vessel)
{
  if (vehicle == NULL || vessel == NULL)
    return 0;

  if (!mock_is_vessel_stationary(vessel))
    return 0;

  if (vehicle->current_passengers > 0)
    return 0;

  if (vehicle->parent_vessel_id > 0)
    return 0;

  if (vehicle->state == VSTATE_DAMAGED)
    return 0;

  if (!mock_check_capacity(vessel, vehicle))
    return 0;

  vehicle->parent_vessel_id = vessel->shipnum;
  vehicle->state = VSTATE_ON_VESSEL;
  vehicle->x_coord = (int)vessel->x;
  vehicle->y_coord = (int)vessel->y;

  return 1;
}

static int mock_unload_vehicle(struct vehicle_data *vehicle, struct greyhawk_ship_data *vessel)
{
  if (vehicle == NULL || vessel == NULL)
    return 0;

  if (vehicle->parent_vessel_id != vessel->shipnum)
    return 0;

  if (!mock_is_vessel_stationary(vessel))
    return 0;

  vehicle->parent_vessel_id = 0;
  vehicle->state = VSTATE_IDLE;

  return 1;
}

static void mock_sync_coordinates(struct vehicle_data *vehicle, struct greyhawk_ship_data *vessel)
{
  if (vehicle == NULL || vessel == NULL)
    return;

  if (vehicle->parent_vessel_id != vessel->shipnum)
    return;

  vehicle->x_coord = (int)vessel->x;
  vehicle->y_coord = (int)vessel->y;
}

/* ========================================================================= */
/* LOAD/UNLOAD TESTS (T017)                                                  */
/* ========================================================================= */

void test_load_vehicle_success(CuTest *tc)
{
  struct vehicle_data *vehicle;
  struct greyhawk_ship_data *vessel;
  int result;

  reset_test_vehicles();

  vehicle = create_test_vehicle(1, VEHICLE_CART, "Test Cart");
  vessel = create_test_vessel(100, "Test Ship");

  CuAssertPtrNotNull(tc, vehicle);
  CuAssertPtrNotNull(tc, vessel);

  result = mock_load_vehicle(vehicle, vessel);

  CuAssertIntEquals(tc, 1, result);
  CuAssertIntEquals(tc, vessel->shipnum, vehicle->parent_vessel_id);
  CuAssertIntEquals(tc, VSTATE_ON_VESSEL, vehicle->state);

  free(vessel);
  reset_test_vehicles();
}

void test_load_vehicle_vessel_moving(CuTest *tc)
{
  struct vehicle_data *vehicle;
  struct greyhawk_ship_data *vessel;
  int result;

  reset_test_vehicles();

  vehicle = create_test_vehicle(1, VEHICLE_CART, "Test Cart");
  vessel = create_test_vessel(100, "Test Ship");
  vessel->speed = 5; /* Moving */

  result = mock_load_vehicle(vehicle, vessel);

  CuAssertIntEquals(tc, 0, result);
  CuAssertIntEquals(tc, 0, vehicle->parent_vessel_id);
  CuAssertIntEquals(tc, VSTATE_IDLE, vehicle->state);

  free(vessel);
  reset_test_vehicles();
}

void test_load_vehicle_occupied(CuTest *tc)
{
  struct vehicle_data *vehicle;
  struct greyhawk_ship_data *vessel;
  int result;

  reset_test_vehicles();

  vehicle = create_test_vehicle(1, VEHICLE_CART, "Test Cart");
  vehicle->current_passengers = 1; /* Has passenger */
  vessel = create_test_vessel(100, "Test Ship");

  result = mock_load_vehicle(vehicle, vessel);

  CuAssertIntEquals(tc, 0, result);
  CuAssertIntEquals(tc, 0, vehicle->parent_vessel_id);

  free(vessel);
  reset_test_vehicles();
}

void test_load_vehicle_already_loaded(CuTest *tc)
{
  struct vehicle_data *vehicle;
  struct greyhawk_ship_data *vessel1;
  struct greyhawk_ship_data *vessel2;
  int result;

  reset_test_vehicles();

  vehicle = create_test_vehicle(1, VEHICLE_CART, "Test Cart");
  vessel1 = create_test_vessel(100, "Test Ship 1");
  vessel2 = create_test_vessel(101, "Test Ship 2");

  /* First load succeeds */
  result = mock_load_vehicle(vehicle, vessel1);
  CuAssertIntEquals(tc, 1, result);

  /* Second load fails */
  result = mock_load_vehicle(vehicle, vessel2);
  CuAssertIntEquals(tc, 0, result);
  CuAssertIntEquals(tc, vessel1->shipnum, vehicle->parent_vessel_id);

  free(vessel1);
  free(vessel2);
  reset_test_vehicles();
}

void test_load_vehicle_damaged(CuTest *tc)
{
  struct vehicle_data *vehicle;
  struct greyhawk_ship_data *vessel;
  int result;

  reset_test_vehicles();

  vehicle = create_test_vehicle(1, VEHICLE_CART, "Test Cart");
  vehicle->state = VSTATE_DAMAGED;
  vessel = create_test_vessel(100, "Test Ship");

  result = mock_load_vehicle(vehicle, vessel);

  CuAssertIntEquals(tc, 0, result);

  free(vessel);
  reset_test_vehicles();
}

void test_unload_vehicle_success(CuTest *tc)
{
  struct vehicle_data *vehicle;
  struct greyhawk_ship_data *vessel;
  int result;

  reset_test_vehicles();

  vehicle = create_test_vehicle(1, VEHICLE_CART, "Test Cart");
  vessel = create_test_vessel(100, "Test Ship");

  /* Load first */
  mock_load_vehicle(vehicle, vessel);

  /* Then unload */
  result = mock_unload_vehicle(vehicle, vessel);

  CuAssertIntEquals(tc, 1, result);
  CuAssertIntEquals(tc, 0, vehicle->parent_vessel_id);
  CuAssertIntEquals(tc, VSTATE_IDLE, vehicle->state);

  free(vessel);
  reset_test_vehicles();
}

void test_unload_vehicle_not_loaded(CuTest *tc)
{
  struct vehicle_data *vehicle;
  struct greyhawk_ship_data *vessel;
  int result;

  reset_test_vehicles();

  vehicle = create_test_vehicle(1, VEHICLE_CART, "Test Cart");
  vessel = create_test_vessel(100, "Test Ship");

  /* Try to unload without loading first */
  result = mock_unload_vehicle(vehicle, vessel);

  CuAssertIntEquals(tc, 0, result);

  free(vessel);
  reset_test_vehicles();
}

void test_unload_vehicle_vessel_moving(CuTest *tc)
{
  struct vehicle_data *vehicle;
  struct greyhawk_ship_data *vessel;
  int result;

  reset_test_vehicles();

  vehicle = create_test_vehicle(1, VEHICLE_CART, "Test Cart");
  vessel = create_test_vessel(100, "Test Ship");

  /* Load first */
  mock_load_vehicle(vehicle, vessel);

  /* Vessel starts moving */
  vessel->speed = 5;

  /* Try to unload */
  result = mock_unload_vehicle(vehicle, vessel);

  CuAssertIntEquals(tc, 0, result);
  CuAssertIntEquals(tc, vessel->shipnum, vehicle->parent_vessel_id);

  free(vessel);
  reset_test_vehicles();
}

/* ========================================================================= */
/* CAPACITY AND COORDINATE SYNC TESTS (T018)                                 */
/* ========================================================================= */

void test_capacity_within_limit(CuTest *tc)
{
  struct vehicle_data *vehicle;
  struct greyhawk_ship_data *vessel;
  int result;

  reset_test_vehicles();

  vehicle = create_test_vehicle(1, VEHICLE_CART, "Test Cart");
  vessel = create_test_vessel(100, "Test Ship");

  result = mock_check_capacity(vessel, vehicle);

  CuAssertIntEquals(tc, 1, result);

  free(vessel);
  reset_test_vehicles();
}

void test_capacity_at_limit(CuTest *tc)
{
  struct vehicle_data *vehicles[MAX_VEHICLES_PER_VESSEL + 1];
  struct greyhawk_ship_data *vessel;
  int i;
  int result;
  char name[32];

  reset_test_vehicles();

  vessel = create_test_vessel(100, "Test Ship");

  /* Load maximum vehicles */
  for (i = 0; i < MAX_VEHICLES_PER_VESSEL; i++)
  {
    sprintf(name, "Cart %d", i);
    vehicles[i] = create_test_vehicle(i + 1, VEHICLE_CART, name);
    mock_load_vehicle(vehicles[i], vessel);
  }

  /* Try to add one more */
  vehicles[MAX_VEHICLES_PER_VESSEL] = create_test_vehicle(100, VEHICLE_CART, "Extra Cart");
  result = mock_check_capacity(vessel, vehicles[MAX_VEHICLES_PER_VESSEL]);

  CuAssertIntEquals(tc, 0, result);

  free(vessel);
  reset_test_vehicles();
}

void test_coordinate_sync(CuTest *tc)
{
  struct vehicle_data *vehicle;
  struct greyhawk_ship_data *vessel;

  reset_test_vehicles();

  vehicle = create_test_vehicle(1, VEHICLE_CART, "Test Cart");
  vessel = create_test_vessel(100, "Test Ship");
  vessel->x = 200.0;
  vessel->y = 300.0;

  /* Load vehicle */
  mock_load_vehicle(vehicle, vessel);

  /* Check initial sync */
  CuAssertIntEquals(tc, 200, vehicle->x_coord);
  CuAssertIntEquals(tc, 300, vehicle->y_coord);

  /* Move vessel */
  vessel->x = 250.0;
  vessel->y = 350.0;

  /* Sync coordinates */
  mock_sync_coordinates(vehicle, vessel);

  CuAssertIntEquals(tc, 250, vehicle->x_coord);
  CuAssertIntEquals(tc, 350, vehicle->y_coord);

  free(vessel);
  reset_test_vehicles();
}

void test_coordinate_sync_not_loaded(CuTest *tc)
{
  struct vehicle_data *vehicle;
  struct greyhawk_ship_data *vessel;

  reset_test_vehicles();

  vehicle = create_test_vehicle(1, VEHICLE_CART, "Test Cart");
  vehicle->x_coord = 50;
  vehicle->y_coord = 50;
  vessel = create_test_vessel(100, "Test Ship");
  vessel->x = 200.0;
  vessel->y = 300.0;

  /* Try sync without loading */
  mock_sync_coordinates(vehicle, vessel);

  /* Coordinates should not change */
  CuAssertIntEquals(tc, 50, vehicle->x_coord);
  CuAssertIntEquals(tc, 50, vehicle->y_coord);

  free(vessel);
  reset_test_vehicles();
}

void test_vessel_docked_allows_loading(CuTest *tc)
{
  struct vehicle_data *vehicle;
  struct greyhawk_ship_data *vessel;
  int result;

  reset_test_vehicles();

  vehicle = create_test_vehicle(1, VEHICLE_CART, "Test Cart");
  vessel = create_test_vessel(100, "Test Ship");
  vessel->speed = 5;          /* Moving */
  vessel->docked_to_ship = 1; /* But docked */

  result = mock_load_vehicle(vehicle, vessel);

  CuAssertIntEquals(tc, 1, result); /* Should succeed because docked */

  free(vessel);
  reset_test_vehicles();
}

void test_parent_vessel_id_persistence(CuTest *tc)
{
  struct vehicle_data *vehicle;
  struct greyhawk_ship_data *vessel;

  reset_test_vehicles();

  vehicle = create_test_vehicle(1, VEHICLE_CART, "Test Cart");
  vessel = create_test_vessel(100, "Test Ship");

  /* Initially no parent */
  CuAssertIntEquals(tc, 0, vehicle->parent_vessel_id);

  /* After loading */
  mock_load_vehicle(vehicle, vessel);
  CuAssertIntEquals(tc, 100, vehicle->parent_vessel_id);

  /* After unloading */
  mock_unload_vehicle(vehicle, vessel);
  CuAssertIntEquals(tc, 0, vehicle->parent_vessel_id);

  free(vessel);
  reset_test_vehicles();
}

/* ========================================================================= */
/* TEST SUITE REGISTRATION                                                   */
/* ========================================================================= */

CuSuite *VehicleTransportGetSuite(void)
{
  CuSuite *suite = CuSuiteNew();

  /* Load/Unload Tests (T017) */
  SUITE_ADD_TEST(suite, test_load_vehicle_success);
  SUITE_ADD_TEST(suite, test_load_vehicle_vessel_moving);
  SUITE_ADD_TEST(suite, test_load_vehicle_occupied);
  SUITE_ADD_TEST(suite, test_load_vehicle_already_loaded);
  SUITE_ADD_TEST(suite, test_load_vehicle_damaged);
  SUITE_ADD_TEST(suite, test_unload_vehicle_success);
  SUITE_ADD_TEST(suite, test_unload_vehicle_not_loaded);
  SUITE_ADD_TEST(suite, test_unload_vehicle_vessel_moving);

  /* Capacity and Coordinate Sync Tests (T018) */
  SUITE_ADD_TEST(suite, test_capacity_within_limit);
  SUITE_ADD_TEST(suite, test_capacity_at_limit);
  SUITE_ADD_TEST(suite, test_coordinate_sync);
  SUITE_ADD_TEST(suite, test_coordinate_sync_not_loaded);
  SUITE_ADD_TEST(suite, test_vessel_docked_allows_loading);
  SUITE_ADD_TEST(suite, test_parent_vessel_id_persistence);

  return suite;
}

/* ========================================================================= */
/* MAIN (for standalone testing)                                             */
/* ========================================================================= */

int main(void)
{
  CuString *output = CuStringNew();
  CuSuite *suite = VehicleTransportGetSuite();
  int failed;

  CuSuiteRun(suite);
  CuSuiteSummary(suite, output);
  CuSuiteDetails(suite, output);

  printf("%s\n", output->buffer);

  failed = suite->failCount;

  /* Clean up CuTest objects */
  CuSuiteDelete(suite);
  CuStringDelete(output);

  return (failed > 0) ? 1 : 0;
}
