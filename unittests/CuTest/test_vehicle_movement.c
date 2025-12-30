/* ************************************************************************
 *      File:   test_vehicle_movement.c                 Part of LuminariMUD  *
 *   Purpose:   Unit tests for vehicle movement system                       *
 *              Phase 02 Session 03: Vehicle Movement System                 *
 * ********************************************************************** */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "CuTest.h"

/* ========================================================================= */
/* MOCK DEFINITIONS (copied from structs.h and vessels.h)                    */
/* ========================================================================= */

/* Direction constants (from structs.h) */
#define NORTH 0
#define EAST 1
#define SOUTH 2
#define WEST 3
#define UP 4
#define DOWN 5
#define NORTHWEST 6
#define NORTHEAST 7
#define SOUTHEAST 8
#define SOUTHWEST 9

/* Sector types (from structs.h) */
#define SECT_INSIDE 0
#define SECT_CITY 1
#define SECT_FIELD 2
#define SECT_FOREST 3
#define SECT_HILLS 4
#define SECT_MOUNTAIN 5
#define SECT_WATER_SWIM 6
#define SECT_WATER_NOSWIM 7
#define SECT_FLYING 8
#define SECT_UNDERWATER 9
#define SECT_ROAD_NS 11
#define SECT_ROAD_EW 12
#define SECT_ROAD_INT 13
#define SECT_DESERT 14
#define SECT_OCEAN 15
#define SECT_MARSHLAND 16
#define SECT_HIGH_MOUNTAIN 17
#define SECT_D_ROAD_NS 26
#define SECT_D_ROAD_EW 27
#define SECT_D_ROAD_INT 28
#define SECT_JUNGLE 30
#define SECT_TUNDRA 31
#define SECT_TAIGA 32
#define SECT_BEACH 33
#define SECT_RIVER 36
#define SECT_LAVA 25
#define SECT_CAVE 29
#define SECT_INSIDE_ROOM 35
#define SECT_UD_WATER 22
#define SECT_UD_NOSWIM 23
#define SECT_UD_NOGROUND 24

/* Vehicle types */
enum vehicle_type
{
  VEHICLE_NONE = 0,
  VEHICLE_CART,
  VEHICLE_WAGON,
  VEHICLE_MOUNT,
  VEHICLE_CARRIAGE,
  NUM_VEHICLE_TYPES
};

/* Vehicle states */
enum vehicle_state
{
  VSTATE_IDLE = 0,
  VSTATE_MOVING,
  VSTATE_LOADED,
  VSTATE_HITCHED,
  VSTATE_DAMAGED,
  NUM_VEHICLE_STATES
};

/* Terrain flags */
#define VTERRAIN_ROAD (1 << 0)
#define VTERRAIN_PLAINS (1 << 1)
#define VTERRAIN_FOREST (1 << 2)
#define VTERRAIN_HILLS (1 << 3)
#define VTERRAIN_MOUNTAIN (1 << 4)
#define VTERRAIN_DESERT (1 << 5)
#define VTERRAIN_SWAMP (1 << 6)

/* Default terrain per type */
#define VTERRAIN_CART_DEFAULT (VTERRAIN_ROAD | VTERRAIN_PLAINS)
#define VTERRAIN_WAGON_DEFAULT (VTERRAIN_ROAD | VTERRAIN_PLAINS)
#define VTERRAIN_MOUNT_DEFAULT (VTERRAIN_ROAD | VTERRAIN_PLAINS | VTERRAIN_FOREST | VTERRAIN_HILLS)
#define VTERRAIN_CARRIAGE_DEFAULT (VTERRAIN_ROAD)

/* Speed modifiers */
#define VEHICLE_SPEED_MOD_ROAD 150
#define VEHICLE_SPEED_MOD_PLAINS 100
#define VEHICLE_SPEED_MOD_FOREST 75
#define VEHICLE_SPEED_MOD_HILLS 75
#define VEHICLE_SPEED_MOD_MOUNTAIN 50
#define VEHICLE_SPEED_MOD_SWAMP 50
#define VEHICLE_SPEED_MOD_DESERT 75
#define VEHICLE_SPEED_MOD_LOADED 75
#define VEHICLE_SPEED_MOD_DAMAGED 50

/* Condition constants */
#define VEHICLE_CONDITION_MAX 100
#define VEHICLE_CONDITION_FAIR 50
#define VEHICLE_CONDITION_BROKEN 0

/* Wilderness bounds */
#define VEHICLE_WILDERNESS_MIN_X (-1024)
#define VEHICLE_WILDERNESS_MAX_X (1024)
#define VEHICLE_WILDERNESS_MIN_Y (-1024)
#define VEHICLE_WILDERNESS_MAX_Y (1024)

/* Vehicle capacity constants */
#define VEHICLE_NAME_LENGTH 64
#define VEHICLE_SPEED_CART 2
#define VEHICLE_SPEED_WAGON 1
#define VEHICLE_SPEED_MOUNT 4
#define VEHICLE_SPEED_CARRIAGE 2
#define VEHICLE_PASSENGERS_CART 2
#define VEHICLE_WEIGHT_CART 500

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
/* MOCK MOVEMENT FUNCTIONS                                                    */
/* ========================================================================= */

/**
 * Get direction delta for wilderness movement.
 */
static void mock_vehicle_get_direction_delta(int direction, int *dx, int *dy)
{
  if (dx == NULL || dy == NULL)
  {
    return;
  }

  *dx = 0;
  *dy = 0;

  switch (direction)
  {
  case NORTH:
    *dy = 1;
    break;
  case SOUTH:
    *dy = -1;
    break;
  case EAST:
    *dx = 1;
    break;
  case WEST:
    *dx = -1;
    break;
  case NORTHEAST:
    *dx = 1;
    *dy = 1;
    break;
  case NORTHWEST:
    *dx = -1;
    *dy = 1;
    break;
  case SOUTHEAST:
    *dx = 1;
    *dy = -1;
    break;
  case SOUTHWEST:
    *dx = -1;
    *dy = -1;
    break;
  default:
    break;
  }
}

/**
 * Map SECT_* to VTERRAIN_*.
 */
static int mock_sector_to_vterrain(int sector_type)
{
  switch (sector_type)
  {
  case SECT_ROAD_NS:
  case SECT_ROAD_EW:
  case SECT_ROAD_INT:
  case SECT_D_ROAD_NS:
  case SECT_D_ROAD_EW:
  case SECT_D_ROAD_INT:
    return VTERRAIN_ROAD;

  case SECT_FIELD:
  case SECT_CITY:
  case SECT_INSIDE:
  case SECT_INSIDE_ROOM:
    return VTERRAIN_PLAINS;

  case SECT_FOREST:
  case SECT_JUNGLE:
  case SECT_TAIGA:
    return VTERRAIN_FOREST;

  case SECT_HILLS:
  case SECT_BEACH:
    return VTERRAIN_HILLS;

  case SECT_MOUNTAIN:
  case SECT_HIGH_MOUNTAIN:
  case SECT_CAVE:
    return VTERRAIN_MOUNTAIN;

  case SECT_DESERT:
  case SECT_TUNDRA:
    return VTERRAIN_DESERT;

  case SECT_MARSHLAND:
    return VTERRAIN_SWAMP;

  case SECT_WATER_SWIM:
  case SECT_WATER_NOSWIM:
  case SECT_OCEAN:
  case SECT_UNDERWATER:
  case SECT_RIVER:
  case SECT_FLYING:
  case SECT_LAVA:
  case SECT_UD_WATER:
  case SECT_UD_NOSWIM:
  case SECT_UD_NOGROUND:
  default:
    return 0;
  }
}

/**
 * Check if vehicle can traverse terrain.
 */
static int mock_vehicle_can_traverse_terrain(struct vehicle_data *vehicle, int sector_type)
{
  int required_terrain;

  if (vehicle == NULL)
  {
    return 0;
  }

  required_terrain = mock_sector_to_vterrain(sector_type);

  if (required_terrain == 0)
  {
    return 0;
  }

  return (vehicle->terrain_flags & required_terrain) ? 1 : 0;
}

/**
 * Get speed modifier for terrain.
 */
static int mock_get_vehicle_speed_modifier(struct vehicle_data *vehicle, int sector_type)
{
  int terrain;

  if (vehicle == NULL)
  {
    return 0;
  }

  terrain = mock_sector_to_vterrain(sector_type);

  if (terrain == 0)
  {
    return 0;
  }

  switch (terrain)
  {
  case VTERRAIN_ROAD:
    return VEHICLE_SPEED_MOD_ROAD;
  case VTERRAIN_PLAINS:
    return VEHICLE_SPEED_MOD_PLAINS;
  case VTERRAIN_FOREST:
    return VEHICLE_SPEED_MOD_FOREST;
  case VTERRAIN_HILLS:
    return VEHICLE_SPEED_MOD_HILLS;
  case VTERRAIN_MOUNTAIN:
    return VEHICLE_SPEED_MOD_MOUNTAIN;
  case VTERRAIN_SWAMP:
    return VEHICLE_SPEED_MOD_SWAMP;
  case VTERRAIN_DESERT:
    return VEHICLE_SPEED_MOD_DESERT;
  default:
    return VEHICLE_SPEED_MOD_PLAINS;
  }
}

/**
 * Check if vehicle is operational.
 */
static int mock_vehicle_is_operational(struct vehicle_data *vehicle)
{
  if (vehicle == NULL)
  {
    return 0;
  }
  return (vehicle->condition > VEHICLE_CONDITION_BROKEN && vehicle->state != VSTATE_DAMAGED);
}

/**
 * Get effective vehicle speed.
 */
static int mock_vehicle_get_speed(struct vehicle_data *vehicle)
{
  int speed;

  if (vehicle == NULL)
  {
    return 0;
  }

  if (vehicle->state == VSTATE_DAMAGED || vehicle->state == VSTATE_HITCHED)
  {
    return 0;
  }

  speed = vehicle->base_speed;

  if (vehicle->condition < VEHICLE_CONDITION_FAIR)
  {
    speed = (speed * VEHICLE_SPEED_MOD_DAMAGED) / 100;
  }

  if (vehicle->current_passengers == vehicle->max_passengers)
  {
    speed = (speed * VEHICLE_SPEED_MOD_LOADED) / 100;
  }

  if (speed < 1)
  {
    speed = 1;
  }

  return speed;
}

/**
 * Check if vehicle can move in direction.
 */
static int mock_vehicle_can_move(struct vehicle_data *vehicle, int direction, int dest_sector)
{
  int dx, dy;
  int new_x, new_y;

  if (vehicle == NULL)
  {
    return 0;
  }

  if (!mock_vehicle_is_operational(vehicle))
  {
    return 0;
  }

  if (vehicle->state == VSTATE_HITCHED)
  {
    return 0;
  }

  mock_vehicle_get_direction_delta(direction, &dx, &dy);

  if (dx == 0 && dy == 0)
  {
    return 0;
  }

  new_x = vehicle->x_coord + dx;
  new_y = vehicle->y_coord + dy;

  if (new_x < VEHICLE_WILDERNESS_MIN_X || new_x > VEHICLE_WILDERNESS_MAX_X ||
      new_y < VEHICLE_WILDERNESS_MIN_Y || new_y > VEHICLE_WILDERNESS_MAX_Y)
  {
    return 0;
  }

  if (!mock_vehicle_can_traverse_terrain(vehicle, dest_sector))
  {
    return 0;
  }

  return 1;
}

/**
 * Initialize a test vehicle.
 */
static void mock_init_test_vehicle(struct vehicle_data *vehicle, enum vehicle_type type)
{
  memset(vehicle, 0, sizeof(struct vehicle_data));
  vehicle->id = 1;
  vehicle->type = type;
  vehicle->state = VSTATE_IDLE;
  vehicle->location = 100;
  vehicle->x_coord = 0;
  vehicle->y_coord = 0;
  vehicle->condition = VEHICLE_CONDITION_MAX;
  vehicle->max_condition = VEHICLE_CONDITION_MAX;

  switch (type)
  {
  case VEHICLE_CART:
    vehicle->max_passengers = VEHICLE_PASSENGERS_CART;
    vehicle->max_weight = VEHICLE_WEIGHT_CART;
    vehicle->base_speed = VEHICLE_SPEED_CART;
    vehicle->terrain_flags = VTERRAIN_CART_DEFAULT;
    break;
  case VEHICLE_MOUNT:
    vehicle->max_passengers = 1;
    vehicle->max_weight = 200;
    vehicle->base_speed = VEHICLE_SPEED_MOUNT;
    vehicle->terrain_flags = VTERRAIN_MOUNT_DEFAULT;
    break;
  case VEHICLE_CARRIAGE:
    vehicle->max_passengers = 4;
    vehicle->max_weight = 800;
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

  vehicle->current_speed = vehicle->base_speed;
  strncpy(vehicle->name, "Test Vehicle", VEHICLE_NAME_LENGTH - 1);
}

/* ========================================================================= */
/* DIRECTION DELTA TESTS                                                      */
/* ========================================================================= */

void test_direction_delta_north(CuTest *tc)
{
  int dx, dy;
  mock_vehicle_get_direction_delta(NORTH, &dx, &dy);
  CuAssertIntEquals(tc, 0, dx);
  CuAssertIntEquals(tc, 1, dy);
}

void test_direction_delta_south(CuTest *tc)
{
  int dx, dy;
  mock_vehicle_get_direction_delta(SOUTH, &dx, &dy);
  CuAssertIntEquals(tc, 0, dx);
  CuAssertIntEquals(tc, -1, dy);
}

void test_direction_delta_east(CuTest *tc)
{
  int dx, dy;
  mock_vehicle_get_direction_delta(EAST, &dx, &dy);
  CuAssertIntEquals(tc, 1, dx);
  CuAssertIntEquals(tc, 0, dy);
}

void test_direction_delta_west(CuTest *tc)
{
  int dx, dy;
  mock_vehicle_get_direction_delta(WEST, &dx, &dy);
  CuAssertIntEquals(tc, -1, dx);
  CuAssertIntEquals(tc, 0, dy);
}

void test_direction_delta_northeast(CuTest *tc)
{
  int dx, dy;
  mock_vehicle_get_direction_delta(NORTHEAST, &dx, &dy);
  CuAssertIntEquals(tc, 1, dx);
  CuAssertIntEquals(tc, 1, dy);
}

void test_direction_delta_northwest(CuTest *tc)
{
  int dx, dy;
  mock_vehicle_get_direction_delta(NORTHWEST, &dx, &dy);
  CuAssertIntEquals(tc, -1, dx);
  CuAssertIntEquals(tc, 1, dy);
}

void test_direction_delta_southeast(CuTest *tc)
{
  int dx, dy;
  mock_vehicle_get_direction_delta(SOUTHEAST, &dx, &dy);
  CuAssertIntEquals(tc, 1, dx);
  CuAssertIntEquals(tc, -1, dy);
}

void test_direction_delta_southwest(CuTest *tc)
{
  int dx, dy;
  mock_vehicle_get_direction_delta(SOUTHWEST, &dx, &dy);
  CuAssertIntEquals(tc, -1, dx);
  CuAssertIntEquals(tc, -1, dy);
}

void test_direction_delta_invalid(CuTest *tc)
{
  int dx, dy;
  mock_vehicle_get_direction_delta(UP, &dx, &dy);
  CuAssertIntEquals(tc, 0, dx);
  CuAssertIntEquals(tc, 0, dy);

  mock_vehicle_get_direction_delta(DOWN, &dx, &dy);
  CuAssertIntEquals(tc, 0, dx);
  CuAssertIntEquals(tc, 0, dy);
}

/* ========================================================================= */
/* TERRAIN MAPPING TESTS                                                      */
/* ========================================================================= */

void test_sector_to_vterrain_roads(CuTest *tc)
{
  CuAssertIntEquals(tc, VTERRAIN_ROAD, mock_sector_to_vterrain(SECT_ROAD_NS));
  CuAssertIntEquals(tc, VTERRAIN_ROAD, mock_sector_to_vterrain(SECT_ROAD_EW));
  CuAssertIntEquals(tc, VTERRAIN_ROAD, mock_sector_to_vterrain(SECT_ROAD_INT));
  CuAssertIntEquals(tc, VTERRAIN_ROAD, mock_sector_to_vterrain(SECT_D_ROAD_NS));
  CuAssertIntEquals(tc, VTERRAIN_ROAD, mock_sector_to_vterrain(SECT_D_ROAD_EW));
  CuAssertIntEquals(tc, VTERRAIN_ROAD, mock_sector_to_vterrain(SECT_D_ROAD_INT));
}

void test_sector_to_vterrain_plains(CuTest *tc)
{
  CuAssertIntEquals(tc, VTERRAIN_PLAINS, mock_sector_to_vterrain(SECT_FIELD));
  CuAssertIntEquals(tc, VTERRAIN_PLAINS, mock_sector_to_vterrain(SECT_CITY));
  CuAssertIntEquals(tc, VTERRAIN_PLAINS, mock_sector_to_vterrain(SECT_INSIDE));
}

void test_sector_to_vterrain_forest(CuTest *tc)
{
  CuAssertIntEquals(tc, VTERRAIN_FOREST, mock_sector_to_vterrain(SECT_FOREST));
  CuAssertIntEquals(tc, VTERRAIN_FOREST, mock_sector_to_vterrain(SECT_JUNGLE));
  CuAssertIntEquals(tc, VTERRAIN_FOREST, mock_sector_to_vterrain(SECT_TAIGA));
}

void test_sector_to_vterrain_hills(CuTest *tc)
{
  CuAssertIntEquals(tc, VTERRAIN_HILLS, mock_sector_to_vterrain(SECT_HILLS));
  CuAssertIntEquals(tc, VTERRAIN_HILLS, mock_sector_to_vterrain(SECT_BEACH));
}

void test_sector_to_vterrain_mountain(CuTest *tc)
{
  CuAssertIntEquals(tc, VTERRAIN_MOUNTAIN, mock_sector_to_vterrain(SECT_MOUNTAIN));
  CuAssertIntEquals(tc, VTERRAIN_MOUNTAIN, mock_sector_to_vterrain(SECT_HIGH_MOUNTAIN));
  CuAssertIntEquals(tc, VTERRAIN_MOUNTAIN, mock_sector_to_vterrain(SECT_CAVE));
}

void test_sector_to_vterrain_desert(CuTest *tc)
{
  CuAssertIntEquals(tc, VTERRAIN_DESERT, mock_sector_to_vterrain(SECT_DESERT));
  CuAssertIntEquals(tc, VTERRAIN_DESERT, mock_sector_to_vterrain(SECT_TUNDRA));
}

void test_sector_to_vterrain_swamp(CuTest *tc)
{
  CuAssertIntEquals(tc, VTERRAIN_SWAMP, mock_sector_to_vterrain(SECT_MARSHLAND));
}

void test_sector_to_vterrain_impassable(CuTest *tc)
{
  CuAssertIntEquals(tc, 0, mock_sector_to_vterrain(SECT_WATER_SWIM));
  CuAssertIntEquals(tc, 0, mock_sector_to_vterrain(SECT_WATER_NOSWIM));
  CuAssertIntEquals(tc, 0, mock_sector_to_vterrain(SECT_OCEAN));
  CuAssertIntEquals(tc, 0, mock_sector_to_vterrain(SECT_UNDERWATER));
  CuAssertIntEquals(tc, 0, mock_sector_to_vterrain(SECT_RIVER));
  CuAssertIntEquals(tc, 0, mock_sector_to_vterrain(SECT_FLYING));
  CuAssertIntEquals(tc, 0, mock_sector_to_vterrain(SECT_LAVA));
}

/* ========================================================================= */
/* TERRAIN TRAVERSAL TESTS                                                    */
/* ========================================================================= */

void test_cart_can_traverse_road(CuTest *tc)
{
  struct vehicle_data vehicle;
  mock_init_test_vehicle(&vehicle, VEHICLE_CART);
  CuAssertIntEquals(tc, 1, mock_vehicle_can_traverse_terrain(&vehicle, SECT_ROAD_NS));
}

void test_cart_can_traverse_plains(CuTest *tc)
{
  struct vehicle_data vehicle;
  mock_init_test_vehicle(&vehicle, VEHICLE_CART);
  CuAssertIntEquals(tc, 1, mock_vehicle_can_traverse_terrain(&vehicle, SECT_FIELD));
}

void test_cart_cannot_traverse_forest(CuTest *tc)
{
  struct vehicle_data vehicle;
  mock_init_test_vehicle(&vehicle, VEHICLE_CART);
  CuAssertIntEquals(tc, 0, mock_vehicle_can_traverse_terrain(&vehicle, SECT_FOREST));
}

void test_cart_cannot_traverse_water(CuTest *tc)
{
  struct vehicle_data vehicle;
  mock_init_test_vehicle(&vehicle, VEHICLE_CART);
  CuAssertIntEquals(tc, 0, mock_vehicle_can_traverse_terrain(&vehicle, SECT_WATER_SWIM));
  CuAssertIntEquals(tc, 0, mock_vehicle_can_traverse_terrain(&vehicle, SECT_OCEAN));
}

void test_mount_can_traverse_forest(CuTest *tc)
{
  struct vehicle_data vehicle;
  mock_init_test_vehicle(&vehicle, VEHICLE_MOUNT);
  CuAssertIntEquals(tc, 1, mock_vehicle_can_traverse_terrain(&vehicle, SECT_FOREST));
}

void test_mount_can_traverse_hills(CuTest *tc)
{
  struct vehicle_data vehicle;
  mock_init_test_vehicle(&vehicle, VEHICLE_MOUNT);
  CuAssertIntEquals(tc, 1, mock_vehicle_can_traverse_terrain(&vehicle, SECT_HILLS));
}

void test_carriage_only_road(CuTest *tc)
{
  struct vehicle_data vehicle;
  mock_init_test_vehicle(&vehicle, VEHICLE_CARRIAGE);
  CuAssertIntEquals(tc, 1, mock_vehicle_can_traverse_terrain(&vehicle, SECT_ROAD_NS));
  CuAssertIntEquals(tc, 0, mock_vehicle_can_traverse_terrain(&vehicle, SECT_FIELD));
  CuAssertIntEquals(tc, 0, mock_vehicle_can_traverse_terrain(&vehicle, SECT_FOREST));
}

void test_traverse_null_vehicle(CuTest *tc)
{
  CuAssertIntEquals(tc, 0, mock_vehicle_can_traverse_terrain(NULL, SECT_ROAD_NS));
}

/* ========================================================================= */
/* SPEED MODIFIER TESTS                                                       */
/* ========================================================================= */

void test_speed_modifier_road(CuTest *tc)
{
  struct vehicle_data vehicle;
  mock_init_test_vehicle(&vehicle, VEHICLE_CART);
  CuAssertIntEquals(tc, VEHICLE_SPEED_MOD_ROAD,
                    mock_get_vehicle_speed_modifier(&vehicle, SECT_ROAD_NS));
}

void test_speed_modifier_plains(CuTest *tc)
{
  struct vehicle_data vehicle;
  mock_init_test_vehicle(&vehicle, VEHICLE_CART);
  CuAssertIntEquals(tc, VEHICLE_SPEED_MOD_PLAINS,
                    mock_get_vehicle_speed_modifier(&vehicle, SECT_FIELD));
}

void test_speed_modifier_forest(CuTest *tc)
{
  struct vehicle_data vehicle;
  mock_init_test_vehicle(&vehicle, VEHICLE_MOUNT);
  CuAssertIntEquals(tc, VEHICLE_SPEED_MOD_FOREST,
                    mock_get_vehicle_speed_modifier(&vehicle, SECT_FOREST));
}

void test_speed_modifier_mountain(CuTest *tc)
{
  struct vehicle_data vehicle;
  mock_init_test_vehicle(&vehicle, VEHICLE_MOUNT);
  /* Mount has mountain flag by default - let's add it for test */
  vehicle.terrain_flags |= VTERRAIN_MOUNTAIN;
  CuAssertIntEquals(tc, VEHICLE_SPEED_MOD_MOUNTAIN,
                    mock_get_vehicle_speed_modifier(&vehicle, SECT_MOUNTAIN));
}

void test_speed_modifier_impassable(CuTest *tc)
{
  struct vehicle_data vehicle;
  mock_init_test_vehicle(&vehicle, VEHICLE_CART);
  CuAssertIntEquals(tc, 0, mock_get_vehicle_speed_modifier(&vehicle, SECT_WATER_SWIM));
}

void test_speed_modifier_null(CuTest *tc)
{
  CuAssertIntEquals(tc, 0, mock_get_vehicle_speed_modifier(NULL, SECT_ROAD_NS));
}

/* ========================================================================= */
/* EFFECTIVE SPEED TESTS                                                      */
/* ========================================================================= */

void test_vehicle_get_speed_normal(CuTest *tc)
{
  struct vehicle_data vehicle;
  mock_init_test_vehicle(&vehicle, VEHICLE_CART);
  CuAssertIntEquals(tc, VEHICLE_SPEED_CART, mock_vehicle_get_speed(&vehicle));
}

void test_vehicle_get_speed_damaged(CuTest *tc)
{
  struct vehicle_data vehicle;
  mock_init_test_vehicle(&vehicle, VEHICLE_CART);
  vehicle.state = VSTATE_DAMAGED;
  CuAssertIntEquals(tc, 0, mock_vehicle_get_speed(&vehicle));
}

void test_vehicle_get_speed_hitched(CuTest *tc)
{
  struct vehicle_data vehicle;
  mock_init_test_vehicle(&vehicle, VEHICLE_CART);
  vehicle.state = VSTATE_HITCHED;
  CuAssertIntEquals(tc, 0, mock_vehicle_get_speed(&vehicle));
}

void test_vehicle_get_speed_null(CuTest *tc)
{
  CuAssertIntEquals(tc, 0, mock_vehicle_get_speed(NULL));
}

/* ========================================================================= */
/* CAN MOVE TESTS                                                             */
/* ========================================================================= */

void test_can_move_valid_direction(CuTest *tc)
{
  struct vehicle_data vehicle;
  mock_init_test_vehicle(&vehicle, VEHICLE_CART);
  CuAssertIntEquals(tc, 1, mock_vehicle_can_move(&vehicle, NORTH, SECT_ROAD_NS));
}

void test_can_move_invalid_direction(CuTest *tc)
{
  struct vehicle_data vehicle;
  mock_init_test_vehicle(&vehicle, VEHICLE_CART);
  CuAssertIntEquals(tc, 0, mock_vehicle_can_move(&vehicle, UP, SECT_ROAD_NS));
  CuAssertIntEquals(tc, 0, mock_vehicle_can_move(&vehicle, DOWN, SECT_ROAD_NS));
}

void test_can_move_impassable_terrain(CuTest *tc)
{
  struct vehicle_data vehicle;
  mock_init_test_vehicle(&vehicle, VEHICLE_CART);
  CuAssertIntEquals(tc, 0, mock_vehicle_can_move(&vehicle, NORTH, SECT_WATER_SWIM));
}

void test_can_move_damaged_vehicle(CuTest *tc)
{
  struct vehicle_data vehicle;
  mock_init_test_vehicle(&vehicle, VEHICLE_CART);
  vehicle.state = VSTATE_DAMAGED;
  vehicle.condition = 0;
  CuAssertIntEquals(tc, 0, mock_vehicle_can_move(&vehicle, NORTH, SECT_ROAD_NS));
}

void test_can_move_hitched_vehicle(CuTest *tc)
{
  struct vehicle_data vehicle;
  mock_init_test_vehicle(&vehicle, VEHICLE_CART);
  vehicle.state = VSTATE_HITCHED;
  CuAssertIntEquals(tc, 0, mock_vehicle_can_move(&vehicle, NORTH, SECT_ROAD_NS));
}

void test_can_move_boundary_north(CuTest *tc)
{
  struct vehicle_data vehicle;
  mock_init_test_vehicle(&vehicle, VEHICLE_CART);
  vehicle.y_coord = VEHICLE_WILDERNESS_MAX_Y;
  CuAssertIntEquals(tc, 0, mock_vehicle_can_move(&vehicle, NORTH, SECT_ROAD_NS));
}

void test_can_move_boundary_south(CuTest *tc)
{
  struct vehicle_data vehicle;
  mock_init_test_vehicle(&vehicle, VEHICLE_CART);
  vehicle.y_coord = VEHICLE_WILDERNESS_MIN_Y;
  CuAssertIntEquals(tc, 0, mock_vehicle_can_move(&vehicle, SOUTH, SECT_ROAD_NS));
}

void test_can_move_boundary_east(CuTest *tc)
{
  struct vehicle_data vehicle;
  mock_init_test_vehicle(&vehicle, VEHICLE_CART);
  vehicle.x_coord = VEHICLE_WILDERNESS_MAX_X;
  CuAssertIntEquals(tc, 0, mock_vehicle_can_move(&vehicle, EAST, SECT_ROAD_NS));
}

void test_can_move_boundary_west(CuTest *tc)
{
  struct vehicle_data vehicle;
  mock_init_test_vehicle(&vehicle, VEHICLE_CART);
  vehicle.x_coord = VEHICLE_WILDERNESS_MIN_X;
  CuAssertIntEquals(tc, 0, mock_vehicle_can_move(&vehicle, WEST, SECT_ROAD_NS));
}

void test_can_move_null_vehicle(CuTest *tc)
{
  CuAssertIntEquals(tc, 0, mock_vehicle_can_move(NULL, NORTH, SECT_ROAD_NS));
}

/* ========================================================================= */
/* TEST SUITE REGISTRATION                                                    */
/* ========================================================================= */

CuSuite *GetVehicleMovementSuite(void)
{
  CuSuite *suite = CuSuiteNew();

  /* Direction delta tests */
  SUITE_ADD_TEST(suite, test_direction_delta_north);
  SUITE_ADD_TEST(suite, test_direction_delta_south);
  SUITE_ADD_TEST(suite, test_direction_delta_east);
  SUITE_ADD_TEST(suite, test_direction_delta_west);
  SUITE_ADD_TEST(suite, test_direction_delta_northeast);
  SUITE_ADD_TEST(suite, test_direction_delta_northwest);
  SUITE_ADD_TEST(suite, test_direction_delta_southeast);
  SUITE_ADD_TEST(suite, test_direction_delta_southwest);
  SUITE_ADD_TEST(suite, test_direction_delta_invalid);

  /* Terrain mapping tests */
  SUITE_ADD_TEST(suite, test_sector_to_vterrain_roads);
  SUITE_ADD_TEST(suite, test_sector_to_vterrain_plains);
  SUITE_ADD_TEST(suite, test_sector_to_vterrain_forest);
  SUITE_ADD_TEST(suite, test_sector_to_vterrain_hills);
  SUITE_ADD_TEST(suite, test_sector_to_vterrain_mountain);
  SUITE_ADD_TEST(suite, test_sector_to_vterrain_desert);
  SUITE_ADD_TEST(suite, test_sector_to_vterrain_swamp);
  SUITE_ADD_TEST(suite, test_sector_to_vterrain_impassable);

  /* Terrain traversal tests */
  SUITE_ADD_TEST(suite, test_cart_can_traverse_road);
  SUITE_ADD_TEST(suite, test_cart_can_traverse_plains);
  SUITE_ADD_TEST(suite, test_cart_cannot_traverse_forest);
  SUITE_ADD_TEST(suite, test_cart_cannot_traverse_water);
  SUITE_ADD_TEST(suite, test_mount_can_traverse_forest);
  SUITE_ADD_TEST(suite, test_mount_can_traverse_hills);
  SUITE_ADD_TEST(suite, test_carriage_only_road);
  SUITE_ADD_TEST(suite, test_traverse_null_vehicle);

  /* Speed modifier tests */
  SUITE_ADD_TEST(suite, test_speed_modifier_road);
  SUITE_ADD_TEST(suite, test_speed_modifier_plains);
  SUITE_ADD_TEST(suite, test_speed_modifier_forest);
  SUITE_ADD_TEST(suite, test_speed_modifier_mountain);
  SUITE_ADD_TEST(suite, test_speed_modifier_impassable);
  SUITE_ADD_TEST(suite, test_speed_modifier_null);

  /* Effective speed tests */
  SUITE_ADD_TEST(suite, test_vehicle_get_speed_normal);
  SUITE_ADD_TEST(suite, test_vehicle_get_speed_damaged);
  SUITE_ADD_TEST(suite, test_vehicle_get_speed_hitched);
  SUITE_ADD_TEST(suite, test_vehicle_get_speed_null);

  /* Can move tests */
  SUITE_ADD_TEST(suite, test_can_move_valid_direction);
  SUITE_ADD_TEST(suite, test_can_move_invalid_direction);
  SUITE_ADD_TEST(suite, test_can_move_impassable_terrain);
  SUITE_ADD_TEST(suite, test_can_move_damaged_vehicle);
  SUITE_ADD_TEST(suite, test_can_move_hitched_vehicle);
  SUITE_ADD_TEST(suite, test_can_move_boundary_north);
  SUITE_ADD_TEST(suite, test_can_move_boundary_south);
  SUITE_ADD_TEST(suite, test_can_move_boundary_east);
  SUITE_ADD_TEST(suite, test_can_move_boundary_west);
  SUITE_ADD_TEST(suite, test_can_move_null_vehicle);

  return suite;
}

/* Standalone test runner */
int main(void)
{
  CuString *output;
  CuSuite *suite;
  int result;

  output = CuStringNew();
  suite = GetVehicleMovementSuite();

  printf("Running vehicle movement system unit tests...\n\n");

  CuSuiteRun(suite);
  CuSuiteSummary(suite, output);
  CuSuiteDetails(suite, output);

  printf("%s\n", output->buffer);

  /* Save fail count before freeing suite */
  result = (suite->failCount > 0) ? 1 : 0;

  CuStringDelete(output);
  CuSuiteDelete(suite);

  return result;
}
