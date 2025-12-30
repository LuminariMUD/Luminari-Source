/* ************************************************************************
 *      File:   test_vehicle_creation.c                Part of LuminariMUD  *
 *   Purpose:   Unit tests for vehicle creation system                       *
 *              Phase 02 Session 02: Vehicle Creation System                 *
 * ********************************************************************** */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "CuTest.h"

/* ========================================================================= */
/* MOCK DEFINITIONS (copied from vessels.h for standalone testing)          */
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
  NUM_VEHICLE_STATES
};

/* Constants */
#define VEHICLE_NAME_LENGTH 64
#define VEHICLE_PASSENGERS_CART 2
#define VEHICLE_PASSENGERS_WAGON 6
#define VEHICLE_PASSENGERS_MOUNT 1
#define VEHICLE_PASSENGERS_CARRIAGE 4
#define VEHICLE_WEIGHT_CART 500
#define VEHICLE_WEIGHT_WAGON 2000
#define VEHICLE_WEIGHT_MOUNT 200
#define VEHICLE_WEIGHT_CARRIAGE 800
#define VEHICLE_SPEED_CART 2
#define VEHICLE_SPEED_WAGON 1
#define VEHICLE_SPEED_MOUNT 4
#define VEHICLE_SPEED_CARRIAGE 2
#define VEHICLE_CONDITION_MAX 100
#define VEHICLE_CONDITION_BROKEN 0

/* Terrain flags */
#define VTERRAIN_ROAD (1 << 0)
#define VTERRAIN_PLAINS (1 << 1)
#define VTERRAIN_FOREST (1 << 2)
#define VTERRAIN_HILLS (1 << 3)
#define VTERRAIN_CART_DEFAULT (VTERRAIN_ROAD | VTERRAIN_PLAINS)
#define VTERRAIN_WAGON_DEFAULT (VTERRAIN_ROAD | VTERRAIN_PLAINS)
#define VTERRAIN_MOUNT_DEFAULT (VTERRAIN_ROAD | VTERRAIN_PLAINS | VTERRAIN_FOREST | VTERRAIN_HILLS)
#define VTERRAIN_CARRIAGE_DEFAULT (VTERRAIN_ROAD)

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
/* MOCK VEHICLE TRACKING                                                      */
/* ========================================================================= */

#define MAX_VEHICLES_TEST 10
static struct vehicle_data *mock_vehicle_list[MAX_VEHICLES_TEST];
static int mock_next_vehicle_id = 1;
static int mock_vehicle_count = 0;

static void mock_init_vehicle_list(void)
{
  int i;
  for (i = 0; i < MAX_VEHICLES_TEST; i++)
  {
    mock_vehicle_list[i] = NULL;
  }
  mock_next_vehicle_id = 1;
  mock_vehicle_count = 0;
}

static int mock_find_empty_slot(void)
{
  int i;
  for (i = 0; i < MAX_VEHICLES_TEST; i++)
  {
    if (mock_vehicle_list[i] == NULL)
    {
      return i;
    }
  }
  return -1;
}

/* ========================================================================= */
/* MOCK VEHICLE FUNCTIONS                                                     */
/* ========================================================================= */

static const char *mock_vehicle_type_name(enum vehicle_type type)
{
  switch (type)
  {
  case VEHICLE_NONE:
    return "none";
  case VEHICLE_CART:
    return "cart";
  case VEHICLE_WAGON:
    return "wagon";
  case VEHICLE_MOUNT:
    return "mount";
  case VEHICLE_CARRIAGE:
    return "carriage";
  default:
    return "unknown";
  }
}

static const char *mock_vehicle_state_name(enum vehicle_state state)
{
  switch (state)
  {
  case VSTATE_IDLE:
    return "idle";
  case VSTATE_MOVING:
    return "moving";
  case VSTATE_LOADED:
    return "loaded";
  case VSTATE_HITCHED:
    return "hitched";
  case VSTATE_DAMAGED:
    return "damaged";
  default:
    return "unknown";
  }
}

static void mock_vehicle_init(struct vehicle_data *vehicle, enum vehicle_type type)
{
  if (vehicle == NULL)
  {
    return;
  }

  vehicle->type = type;
  vehicle->state = VSTATE_IDLE;
  vehicle->location = NOWHERE;
  vehicle->direction = 0;

  switch (type)
  {
  case VEHICLE_CART:
    vehicle->max_passengers = VEHICLE_PASSENGERS_CART;
    vehicle->max_weight = VEHICLE_WEIGHT_CART;
    vehicle->base_speed = VEHICLE_SPEED_CART;
    vehicle->terrain_flags = VTERRAIN_CART_DEFAULT;
    break;
  case VEHICLE_WAGON:
    vehicle->max_passengers = VEHICLE_PASSENGERS_WAGON;
    vehicle->max_weight = VEHICLE_WEIGHT_WAGON;
    vehicle->base_speed = VEHICLE_SPEED_WAGON;
    vehicle->terrain_flags = VTERRAIN_WAGON_DEFAULT;
    break;
  case VEHICLE_MOUNT:
    vehicle->max_passengers = VEHICLE_PASSENGERS_MOUNT;
    vehicle->max_weight = VEHICLE_WEIGHT_MOUNT;
    vehicle->base_speed = VEHICLE_SPEED_MOUNT;
    vehicle->terrain_flags = VTERRAIN_MOUNT_DEFAULT;
    break;
  case VEHICLE_CARRIAGE:
    vehicle->max_passengers = VEHICLE_PASSENGERS_CARRIAGE;
    vehicle->max_weight = VEHICLE_WEIGHT_CARRIAGE;
    vehicle->base_speed = VEHICLE_SPEED_CARRIAGE;
    vehicle->terrain_flags = VTERRAIN_CARRIAGE_DEFAULT;
    break;
  default:
    vehicle->max_passengers = VEHICLE_PASSENGERS_CART;
    vehicle->max_weight = VEHICLE_WEIGHT_CART;
    vehicle->base_speed = VEHICLE_SPEED_CART;
    vehicle->terrain_flags = VTERRAIN_CART_DEFAULT;
    break;
  }

  vehicle->current_passengers = 0;
  vehicle->current_weight = 0;
  vehicle->current_speed = vehicle->base_speed;
  vehicle->max_condition = VEHICLE_CONDITION_MAX;
  vehicle->condition = VEHICLE_CONDITION_MAX;
  vehicle->owner_id = 0;
  vehicle->obj = NULL;
}

static struct vehicle_data *mock_vehicle_create(enum vehicle_type type, const char *name)
{
  struct vehicle_data *vehicle;
  int slot;

  if (type <= VEHICLE_NONE || type >= NUM_VEHICLE_TYPES)
  {
    return NULL;
  }

  slot = mock_find_empty_slot();
  if (slot < 0)
  {
    return NULL;
  }

  vehicle = (struct vehicle_data *)malloc(sizeof(struct vehicle_data));
  if (vehicle == NULL)
  {
    return NULL;
  }

  memset(vehicle, 0, sizeof(struct vehicle_data));
  vehicle->id = mock_next_vehicle_id++;

  if (name != NULL && name[0] != '\0')
  {
    strncpy(vehicle->name, name, VEHICLE_NAME_LENGTH - 1);
    vehicle->name[VEHICLE_NAME_LENGTH - 1] = '\0';
  }
  else
  {
    snprintf(vehicle->name, VEHICLE_NAME_LENGTH, "a %s", mock_vehicle_type_name(type));
  }

  mock_vehicle_init(vehicle, type);
  mock_vehicle_list[slot] = vehicle;
  mock_vehicle_count++;

  return vehicle;
}

static void mock_vehicle_destroy(struct vehicle_data *vehicle)
{
  int i;

  if (vehicle == NULL)
  {
    return;
  }

  for (i = 0; i < MAX_VEHICLES_TEST; i++)
  {
    if (mock_vehicle_list[i] == vehicle)
    {
      mock_vehicle_list[i] = NULL;
      mock_vehicle_count--;
      break;
    }
  }

  free(vehicle);
}

static int mock_vehicle_set_state(struct vehicle_data *vehicle, enum vehicle_state new_state)
{
  if (vehicle == NULL)
  {
    return 0;
  }

  if (new_state < VSTATE_IDLE || new_state >= NUM_VEHICLE_STATES)
  {
    return 0;
  }

  if (vehicle->state == VSTATE_DAMAGED)
  {
    if (new_state != VSTATE_IDLE && new_state != VSTATE_DAMAGED)
    {
      return 0;
    }
  }

  vehicle->state = new_state;
  return 1;
}

static int mock_vehicle_can_add_passenger(struct vehicle_data *vehicle)
{
  if (vehicle == NULL || vehicle->state == VSTATE_DAMAGED)
  {
    return 0;
  }
  return (vehicle->current_passengers < vehicle->max_passengers);
}

static int mock_vehicle_add_passenger(struct vehicle_data *vehicle)
{
  if (!mock_vehicle_can_add_passenger(vehicle))
  {
    return 0;
  }
  vehicle->current_passengers++;
  if (vehicle->state == VSTATE_IDLE)
  {
    vehicle->state = VSTATE_LOADED;
  }
  return 1;
}

static int mock_vehicle_remove_passenger(struct vehicle_data *vehicle)
{
  if (vehicle == NULL || vehicle->current_passengers <= 0)
  {
    return 0;
  }
  vehicle->current_passengers--;
  if (vehicle->current_passengers == 0 && vehicle->current_weight == 0 &&
      vehicle->state == VSTATE_LOADED)
  {
    vehicle->state = VSTATE_IDLE;
  }
  return 1;
}

static int mock_vehicle_can_add_weight(struct vehicle_data *vehicle, int weight)
{
  if (vehicle == NULL || weight < 0 || vehicle->state == VSTATE_DAMAGED)
  {
    return 0;
  }
  return ((vehicle->current_weight + weight) <= vehicle->max_weight);
}

static int mock_vehicle_add_weight(struct vehicle_data *vehicle, int weight)
{
  if (!mock_vehicle_can_add_weight(vehicle, weight))
  {
    return 0;
  }
  vehicle->current_weight += weight;
  if (vehicle->state == VSTATE_IDLE)
  {
    vehicle->state = VSTATE_LOADED;
  }
  return 1;
}

static int mock_vehicle_remove_weight(struct vehicle_data *vehicle, int weight)
{
  if (vehicle == NULL || weight < 0 || vehicle->current_weight < weight)
  {
    return 0;
  }
  vehicle->current_weight -= weight;
  if (vehicle->current_passengers == 0 && vehicle->current_weight == 0 &&
      vehicle->state == VSTATE_LOADED)
  {
    vehicle->state = VSTATE_IDLE;
  }
  return 1;
}

static int mock_vehicle_damage(struct vehicle_data *vehicle, int amount)
{
  if (vehicle == NULL)
  {
    return -1;
  }
  if (amount < 0)
  {
    amount = 0;
  }
  vehicle->condition -= amount;
  if (vehicle->condition < VEHICLE_CONDITION_BROKEN)
  {
    vehicle->condition = VEHICLE_CONDITION_BROKEN;
  }
  if (vehicle->condition <= VEHICLE_CONDITION_BROKEN)
  {
    vehicle->state = VSTATE_DAMAGED;
  }
  return vehicle->condition;
}

static int mock_vehicle_repair(struct vehicle_data *vehicle, int amount)
{
  if (vehicle == NULL)
  {
    return -1;
  }
  if (amount < 0)
  {
    amount = 0;
  }
  vehicle->condition += amount;
  if (vehicle->condition > vehicle->max_condition)
  {
    vehicle->condition = vehicle->max_condition;
  }
  if (vehicle->state == VSTATE_DAMAGED && vehicle->condition > VEHICLE_CONDITION_BROKEN)
  {
    vehicle->state = VSTATE_IDLE;
  }
  return vehicle->condition;
}

static int mock_vehicle_is_operational(struct vehicle_data *vehicle)
{
  if (vehicle == NULL)
  {
    return 0;
  }
  return (vehicle->condition > VEHICLE_CONDITION_BROKEN && vehicle->state != VSTATE_DAMAGED);
}

static struct vehicle_data *mock_vehicle_find_by_id(int id)
{
  int i;
  if (id <= 0)
  {
    return NULL;
  }
  for (i = 0; i < MAX_VEHICLES_TEST; i++)
  {
    if (mock_vehicle_list[i] != NULL && mock_vehicle_list[i]->id == id)
    {
      return mock_vehicle_list[i];
    }
  }
  return NULL;
}

static struct vehicle_data *mock_vehicle_find_in_room(room_rnum room)
{
  int i;
  if (room == NOWHERE)
  {
    return NULL;
  }
  for (i = 0; i < MAX_VEHICLES_TEST; i++)
  {
    if (mock_vehicle_list[i] != NULL && mock_vehicle_list[i]->location == room)
    {
      return mock_vehicle_list[i];
    }
  }
  return NULL;
}

/* ========================================================================= */
/* LIFECYCLE TESTS (T021)                                                     */
/* ========================================================================= */

void test_vehicle_create_cart(CuTest *tc)
{
  struct vehicle_data *vehicle;

  mock_init_vehicle_list();
  vehicle = mock_vehicle_create(VEHICLE_CART, "Test Cart");

  CuAssertPtrNotNull(tc, vehicle);
  CuAssertIntEquals(tc, VEHICLE_CART, vehicle->type);
  CuAssertIntEquals(tc, VSTATE_IDLE, vehicle->state);
  CuAssertStrEquals(tc, "Test Cart", vehicle->name);
  CuAssertIntEquals(tc, VEHICLE_PASSENGERS_CART, vehicle->max_passengers);
  CuAssertIntEquals(tc, VEHICLE_WEIGHT_CART, vehicle->max_weight);
  CuAssertIntEquals(tc, VEHICLE_SPEED_CART, vehicle->base_speed);

  mock_vehicle_destroy(vehicle);
}

void test_vehicle_create_wagon(CuTest *tc)
{
  struct vehicle_data *vehicle;

  mock_init_vehicle_list();
  vehicle = mock_vehicle_create(VEHICLE_WAGON, NULL);

  CuAssertPtrNotNull(tc, vehicle);
  CuAssertIntEquals(tc, VEHICLE_WAGON, vehicle->type);
  CuAssertStrEquals(tc, "a wagon", vehicle->name);
  CuAssertIntEquals(tc, VEHICLE_PASSENGERS_WAGON, vehicle->max_passengers);
  CuAssertIntEquals(tc, VEHICLE_WEIGHT_WAGON, vehicle->max_weight);

  mock_vehicle_destroy(vehicle);
}

void test_vehicle_create_mount(CuTest *tc)
{
  struct vehicle_data *vehicle;

  mock_init_vehicle_list();
  vehicle = mock_vehicle_create(VEHICLE_MOUNT, "Swift Horse");

  CuAssertPtrNotNull(tc, vehicle);
  CuAssertIntEquals(tc, VEHICLE_MOUNT, vehicle->type);
  CuAssertIntEquals(tc, VEHICLE_PASSENGERS_MOUNT, vehicle->max_passengers);
  CuAssertIntEquals(tc, VEHICLE_SPEED_MOUNT, vehicle->base_speed);

  mock_vehicle_destroy(vehicle);
}

void test_vehicle_create_carriage(CuTest *tc)
{
  struct vehicle_data *vehicle;

  mock_init_vehicle_list();
  vehicle = mock_vehicle_create(VEHICLE_CARRIAGE, "Royal Carriage");

  CuAssertPtrNotNull(tc, vehicle);
  CuAssertIntEquals(tc, VEHICLE_CARRIAGE, vehicle->type);
  CuAssertIntEquals(tc, VEHICLE_PASSENGERS_CARRIAGE, vehicle->max_passengers);
  CuAssertIntEquals(tc, VEHICLE_WEIGHT_CARRIAGE, vehicle->max_weight);

  mock_vehicle_destroy(vehicle);
}

void test_vehicle_create_invalid_type(CuTest *tc)
{
  struct vehicle_data *vehicle;

  mock_init_vehicle_list();
  vehicle = mock_vehicle_create(VEHICLE_NONE, "Invalid");
  CuAssertPtrEquals(tc, NULL, vehicle);

  vehicle = mock_vehicle_create(NUM_VEHICLE_TYPES, "Invalid");
  CuAssertPtrEquals(tc, NULL, vehicle);
}

void test_vehicle_destroy_null(CuTest *tc)
{
  /* Should not crash */
  mock_vehicle_destroy(NULL);
  CuAssertTrue(tc, 1);
}

void test_vehicle_unique_ids(CuTest *tc)
{
  struct vehicle_data *v1, *v2, *v3;

  mock_init_vehicle_list();
  v1 = mock_vehicle_create(VEHICLE_CART, "Cart 1");
  v2 = mock_vehicle_create(VEHICLE_WAGON, "Wagon 1");
  v3 = mock_vehicle_create(VEHICLE_MOUNT, "Mount 1");

  CuAssertTrue(tc, v1->id != v2->id);
  CuAssertTrue(tc, v2->id != v3->id);
  CuAssertTrue(tc, v1->id != v3->id);

  mock_vehicle_destroy(v1);
  mock_vehicle_destroy(v2);
  mock_vehicle_destroy(v3);
}

/* ========================================================================= */
/* STATE MANAGEMENT TESTS (T022)                                              */
/* ========================================================================= */

void test_vehicle_type_name(CuTest *tc)
{
  CuAssertStrEquals(tc, "none", mock_vehicle_type_name(VEHICLE_NONE));
  CuAssertStrEquals(tc, "cart", mock_vehicle_type_name(VEHICLE_CART));
  CuAssertStrEquals(tc, "wagon", mock_vehicle_type_name(VEHICLE_WAGON));
  CuAssertStrEquals(tc, "mount", mock_vehicle_type_name(VEHICLE_MOUNT));
  CuAssertStrEquals(tc, "carriage", mock_vehicle_type_name(VEHICLE_CARRIAGE));
  CuAssertStrEquals(tc, "unknown", mock_vehicle_type_name(99));
}

void test_vehicle_state_name(CuTest *tc)
{
  CuAssertStrEquals(tc, "idle", mock_vehicle_state_name(VSTATE_IDLE));
  CuAssertStrEquals(tc, "moving", mock_vehicle_state_name(VSTATE_MOVING));
  CuAssertStrEquals(tc, "loaded", mock_vehicle_state_name(VSTATE_LOADED));
  CuAssertStrEquals(tc, "hitched", mock_vehicle_state_name(VSTATE_HITCHED));
  CuAssertStrEquals(tc, "damaged", mock_vehicle_state_name(VSTATE_DAMAGED));
  CuAssertStrEquals(tc, "unknown", mock_vehicle_state_name(99));
}

void test_vehicle_set_state_valid(CuTest *tc)
{
  struct vehicle_data *vehicle;

  mock_init_vehicle_list();
  vehicle = mock_vehicle_create(VEHICLE_CART, "Test");

  CuAssertIntEquals(tc, 1, mock_vehicle_set_state(vehicle, VSTATE_MOVING));
  CuAssertIntEquals(tc, VSTATE_MOVING, vehicle->state);

  CuAssertIntEquals(tc, 1, mock_vehicle_set_state(vehicle, VSTATE_LOADED));
  CuAssertIntEquals(tc, VSTATE_LOADED, vehicle->state);

  mock_vehicle_destroy(vehicle);
}

void test_vehicle_set_state_damaged_restriction(CuTest *tc)
{
  struct vehicle_data *vehicle;

  mock_init_vehicle_list();
  vehicle = mock_vehicle_create(VEHICLE_CART, "Test");
  vehicle->state = VSTATE_DAMAGED;

  /* Damaged vehicle can only go to idle or stay damaged */
  CuAssertIntEquals(tc, 0, mock_vehicle_set_state(vehicle, VSTATE_MOVING));
  CuAssertIntEquals(tc, 0, mock_vehicle_set_state(vehicle, VSTATE_LOADED));
  CuAssertIntEquals(tc, 1, mock_vehicle_set_state(vehicle, VSTATE_IDLE));

  mock_vehicle_destroy(vehicle);
}

void test_vehicle_set_state_null(CuTest *tc)
{
  CuAssertIntEquals(tc, 0, mock_vehicle_set_state(NULL, VSTATE_IDLE));
}

void test_vehicle_set_state_invalid(CuTest *tc)
{
  struct vehicle_data *vehicle;

  mock_init_vehicle_list();
  vehicle = mock_vehicle_create(VEHICLE_CART, "Test");

  CuAssertIntEquals(tc, 0, mock_vehicle_set_state(vehicle, -1));
  CuAssertIntEquals(tc, 0, mock_vehicle_set_state(vehicle, NUM_VEHICLE_STATES));

  mock_vehicle_destroy(vehicle);
}

/* ========================================================================= */
/* CAPACITY TESTS (T022)                                                      */
/* ========================================================================= */

void test_vehicle_add_passenger(CuTest *tc)
{
  struct vehicle_data *vehicle;

  mock_init_vehicle_list();
  vehicle = mock_vehicle_create(VEHICLE_CART, "Test");

  CuAssertIntEquals(tc, 1, mock_vehicle_can_add_passenger(vehicle));
  CuAssertIntEquals(tc, 1, mock_vehicle_add_passenger(vehicle));
  CuAssertIntEquals(tc, 1, vehicle->current_passengers);
  CuAssertIntEquals(tc, VSTATE_LOADED, vehicle->state);

  mock_vehicle_destroy(vehicle);
}

void test_vehicle_passenger_limit(CuTest *tc)
{
  struct vehicle_data *vehicle;
  int i;

  mock_init_vehicle_list();
  vehicle = mock_vehicle_create(VEHICLE_CART, "Test"); /* max 2 passengers */

  for (i = 0; i < VEHICLE_PASSENGERS_CART; i++)
  {
    CuAssertIntEquals(tc, 1, mock_vehicle_add_passenger(vehicle));
  }

  /* Should fail when full */
  CuAssertIntEquals(tc, 0, mock_vehicle_can_add_passenger(vehicle));
  CuAssertIntEquals(tc, 0, mock_vehicle_add_passenger(vehicle));

  mock_vehicle_destroy(vehicle);
}

void test_vehicle_remove_passenger(CuTest *tc)
{
  struct vehicle_data *vehicle;

  mock_init_vehicle_list();
  vehicle = mock_vehicle_create(VEHICLE_CART, "Test");

  mock_vehicle_add_passenger(vehicle);
  CuAssertIntEquals(tc, 1, mock_vehicle_remove_passenger(vehicle));
  CuAssertIntEquals(tc, 0, vehicle->current_passengers);
  CuAssertIntEquals(tc, VSTATE_IDLE, vehicle->state);

  /* Can't remove below 0 */
  CuAssertIntEquals(tc, 0, mock_vehicle_remove_passenger(vehicle));

  mock_vehicle_destroy(vehicle);
}

void test_vehicle_add_weight(CuTest *tc)
{
  struct vehicle_data *vehicle;

  mock_init_vehicle_list();
  vehicle = mock_vehicle_create(VEHICLE_CART, "Test");

  CuAssertIntEquals(tc, 1, mock_vehicle_can_add_weight(vehicle, 100));
  CuAssertIntEquals(tc, 1, mock_vehicle_add_weight(vehicle, 100));
  CuAssertIntEquals(tc, 100, vehicle->current_weight);
  CuAssertIntEquals(tc, VSTATE_LOADED, vehicle->state);

  mock_vehicle_destroy(vehicle);
}

void test_vehicle_weight_limit(CuTest *tc)
{
  struct vehicle_data *vehicle;

  mock_init_vehicle_list();
  vehicle = mock_vehicle_create(VEHICLE_CART, "Test"); /* max 500 */

  CuAssertIntEquals(tc, 1, mock_vehicle_add_weight(vehicle, 400));
  CuAssertIntEquals(tc, 1, mock_vehicle_can_add_weight(vehicle, 100));
  CuAssertIntEquals(tc, 0, mock_vehicle_can_add_weight(vehicle, 101));
  CuAssertIntEquals(tc, 0, mock_vehicle_add_weight(vehicle, 200));

  mock_vehicle_destroy(vehicle);
}

void test_vehicle_remove_weight(CuTest *tc)
{
  struct vehicle_data *vehicle;

  mock_init_vehicle_list();
  vehicle = mock_vehicle_create(VEHICLE_CART, "Test");

  mock_vehicle_add_weight(vehicle, 200);
  CuAssertIntEquals(tc, 1, mock_vehicle_remove_weight(vehicle, 100));
  CuAssertIntEquals(tc, 100, vehicle->current_weight);

  /* Can't remove more than current */
  CuAssertIntEquals(tc, 0, mock_vehicle_remove_weight(vehicle, 200));

  mock_vehicle_destroy(vehicle);
}

/* ========================================================================= */
/* CONDITION TESTS (T022)                                                     */
/* ========================================================================= */

void test_vehicle_damage(CuTest *tc)
{
  struct vehicle_data *vehicle;

  mock_init_vehicle_list();
  vehicle = mock_vehicle_create(VEHICLE_CART, "Test");

  CuAssertIntEquals(tc, 80, mock_vehicle_damage(vehicle, 20));
  CuAssertIntEquals(tc, 80, vehicle->condition);
  CuAssertIntEquals(tc, 1, mock_vehicle_is_operational(vehicle));

  mock_vehicle_destroy(vehicle);
}

void test_vehicle_damage_to_zero(CuTest *tc)
{
  struct vehicle_data *vehicle;

  mock_init_vehicle_list();
  vehicle = mock_vehicle_create(VEHICLE_CART, "Test");

  CuAssertIntEquals(tc, 0, mock_vehicle_damage(vehicle, 150)); /* Clamp to 0 */
  CuAssertIntEquals(tc, VEHICLE_CONDITION_BROKEN, vehicle->condition);
  CuAssertIntEquals(tc, VSTATE_DAMAGED, vehicle->state);
  CuAssertIntEquals(tc, 0, mock_vehicle_is_operational(vehicle));

  mock_vehicle_destroy(vehicle);
}

void test_vehicle_repair(CuTest *tc)
{
  struct vehicle_data *vehicle;

  mock_init_vehicle_list();
  vehicle = mock_vehicle_create(VEHICLE_CART, "Test");
  vehicle->condition = 50;

  CuAssertIntEquals(tc, 80, mock_vehicle_repair(vehicle, 30));
  CuAssertIntEquals(tc, 80, vehicle->condition);

  mock_vehicle_destroy(vehicle);
}

void test_vehicle_repair_to_max(CuTest *tc)
{
  struct vehicle_data *vehicle;

  mock_init_vehicle_list();
  vehicle = mock_vehicle_create(VEHICLE_CART, "Test");
  vehicle->condition = 50;

  CuAssertIntEquals(tc, 100, mock_vehicle_repair(vehicle, 200)); /* Clamp to max */
  CuAssertIntEquals(tc, VEHICLE_CONDITION_MAX, vehicle->condition);

  mock_vehicle_destroy(vehicle);
}

void test_vehicle_repair_clears_damaged(CuTest *tc)
{
  struct vehicle_data *vehicle;

  mock_init_vehicle_list();
  vehicle = mock_vehicle_create(VEHICLE_CART, "Test");
  vehicle->condition = 0;
  vehicle->state = VSTATE_DAMAGED;

  mock_vehicle_repair(vehicle, 50);
  CuAssertIntEquals(tc, VSTATE_IDLE, vehicle->state);
  CuAssertIntEquals(tc, 1, mock_vehicle_is_operational(vehicle));

  mock_vehicle_destroy(vehicle);
}

void test_vehicle_is_operational(CuTest *tc)
{
  struct vehicle_data *vehicle;

  mock_init_vehicle_list();
  vehicle = mock_vehicle_create(VEHICLE_CART, "Test");

  CuAssertIntEquals(tc, 1, mock_vehicle_is_operational(vehicle));

  vehicle->condition = 0;
  vehicle->state = VSTATE_DAMAGED;
  CuAssertIntEquals(tc, 0, mock_vehicle_is_operational(vehicle));

  CuAssertIntEquals(tc, 0, mock_vehicle_is_operational(NULL));

  mock_vehicle_destroy(vehicle);
}

/* ========================================================================= */
/* LOOKUP TESTS (T022)                                                        */
/* ========================================================================= */

void test_vehicle_find_by_id(CuTest *tc)
{
  struct vehicle_data *v1, *v2, *found;

  mock_init_vehicle_list();
  v1 = mock_vehicle_create(VEHICLE_CART, "Cart 1");
  v2 = mock_vehicle_create(VEHICLE_WAGON, "Wagon 1");

  found = mock_vehicle_find_by_id(v1->id);
  CuAssertPtrEquals(tc, v1, found);

  found = mock_vehicle_find_by_id(v2->id);
  CuAssertPtrEquals(tc, v2, found);

  found = mock_vehicle_find_by_id(999);
  CuAssertPtrEquals(tc, NULL, found);

  found = mock_vehicle_find_by_id(0);
  CuAssertPtrEquals(tc, NULL, found);

  mock_vehicle_destroy(v1);
  mock_vehicle_destroy(v2);
}

void test_vehicle_find_in_room(CuTest *tc)
{
  struct vehicle_data *v1, *v2, *found;

  mock_init_vehicle_list();
  v1 = mock_vehicle_create(VEHICLE_CART, "Cart 1");
  v2 = mock_vehicle_create(VEHICLE_WAGON, "Wagon 1");

  v1->location = 100;
  v2->location = 200;

  found = mock_vehicle_find_in_room(100);
  CuAssertPtrEquals(tc, v1, found);

  found = mock_vehicle_find_in_room(200);
  CuAssertPtrEquals(tc, v2, found);

  found = mock_vehicle_find_in_room(300);
  CuAssertPtrEquals(tc, NULL, found);

  found = mock_vehicle_find_in_room(NOWHERE);
  CuAssertPtrEquals(tc, NULL, found);

  mock_vehicle_destroy(v1);
  mock_vehicle_destroy(v2);
}

/* ========================================================================= */
/* TEST SUITE REGISTRATION                                                    */
/* ========================================================================= */

CuSuite *GetVehicleCreationSuite(void)
{
  CuSuite *suite = CuSuiteNew();

  /* Lifecycle tests */
  SUITE_ADD_TEST(suite, test_vehicle_create_cart);
  SUITE_ADD_TEST(suite, test_vehicle_create_wagon);
  SUITE_ADD_TEST(suite, test_vehicle_create_mount);
  SUITE_ADD_TEST(suite, test_vehicle_create_carriage);
  SUITE_ADD_TEST(suite, test_vehicle_create_invalid_type);
  SUITE_ADD_TEST(suite, test_vehicle_destroy_null);
  SUITE_ADD_TEST(suite, test_vehicle_unique_ids);

  /* State management tests */
  SUITE_ADD_TEST(suite, test_vehicle_type_name);
  SUITE_ADD_TEST(suite, test_vehicle_state_name);
  SUITE_ADD_TEST(suite, test_vehicle_set_state_valid);
  SUITE_ADD_TEST(suite, test_vehicle_set_state_damaged_restriction);
  SUITE_ADD_TEST(suite, test_vehicle_set_state_null);
  SUITE_ADD_TEST(suite, test_vehicle_set_state_invalid);

  /* Capacity tests */
  SUITE_ADD_TEST(suite, test_vehicle_add_passenger);
  SUITE_ADD_TEST(suite, test_vehicle_passenger_limit);
  SUITE_ADD_TEST(suite, test_vehicle_remove_passenger);
  SUITE_ADD_TEST(suite, test_vehicle_add_weight);
  SUITE_ADD_TEST(suite, test_vehicle_weight_limit);
  SUITE_ADD_TEST(suite, test_vehicle_remove_weight);

  /* Condition tests */
  SUITE_ADD_TEST(suite, test_vehicle_damage);
  SUITE_ADD_TEST(suite, test_vehicle_damage_to_zero);
  SUITE_ADD_TEST(suite, test_vehicle_repair);
  SUITE_ADD_TEST(suite, test_vehicle_repair_to_max);
  SUITE_ADD_TEST(suite, test_vehicle_repair_clears_damaged);
  SUITE_ADD_TEST(suite, test_vehicle_is_operational);

  /* Lookup tests */
  SUITE_ADD_TEST(suite, test_vehicle_find_by_id);
  SUITE_ADD_TEST(suite, test_vehicle_find_in_room);

  return suite;
}

/* Standalone test runner */
int main(void)
{
  CuString *output = CuStringNew();
  CuSuite *suite = GetVehicleCreationSuite();

  printf("Running vehicle creation system unit tests...\n\n");

  CuSuiteRun(suite);
  CuSuiteSummary(suite, output);
  CuSuiteDetails(suite, output);

  printf("%s\n", output->buffer);

  CuStringDelete(output);
  CuSuiteDelete(suite);

  return suite->failCount;
}
