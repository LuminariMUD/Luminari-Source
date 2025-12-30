/**
 * @file test_vehicle_structs.c
 * @brief Unit tests for Phase 02 vehicle data structures
 *
 * Tests vehicle_data struct size, enum value uniqueness,
 * and terrain flag bit integrity.
 *
 * Part of Phase 02, Session 01: Vehicle Data Structures
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "CuTest.h"

/* ========================================================================= */
/* VEHICLE SYSTEM DEFINITIONS (copied from vessels.h for standalone testing) */
/* ========================================================================= */

/* Vehicle Types (land-based transport) */
enum vehicle_type
{
  VEHICLE_NONE = 0,
  VEHICLE_CART,     /* 1-2 passengers, low capacity */
  VEHICLE_WAGON,    /* 4-6 passengers, high capacity */
  VEHICLE_MOUNT,    /* 1 rider, fast movement */
  VEHICLE_CARRIAGE, /* 2-4 passengers, enclosed */
  NUM_VEHICLE_TYPES /* Must be last */
};

/* Vehicle States */
enum vehicle_state
{
  VSTATE_IDLE = 0,   /* Stationary, not in use */
  VSTATE_MOVING,     /* Currently traveling */
  VSTATE_LOADED,     /* Carrying cargo/passengers */
  VSTATE_HITCHED,    /* Attached to another vehicle */
  VSTATE_DAMAGED,    /* Broken, needs repair */
  NUM_VEHICLE_STATES /* Must be last */
};

/* Vehicle Terrain Flags (bitfield) */
#define VTERRAIN_ROAD (1 << 0)     /* Paved roads */
#define VTERRAIN_PLAINS (1 << 1)   /* Open grassland */
#define VTERRAIN_FOREST (1 << 2)   /* Light forest */
#define VTERRAIN_HILLS (1 << 3)    /* Hilly terrain */
#define VTERRAIN_MOUNTAIN (1 << 4) /* Mountain paths */
#define VTERRAIN_DESERT (1 << 5)   /* Desert/sand */
#define VTERRAIN_SWAMP (1 << 6)    /* Wetlands */
#define VTERRAIN_ALL 0x7F          /* All terrain flags combined */

/* Vehicle Capacity Constants */
#define VEHICLE_NAME_LENGTH 64   /* Max length for vehicle name */
#define VEHICLE_MAX_PASSENGERS 8 /* Maximum passengers any vehicle can hold */
#define VEHICLE_MAX_WEIGHT 5000  /* Maximum weight capacity in pounds */

/* Vehicle Speed Constants (rooms per tick) */
#define VEHICLE_SPEED_CART 2     /* Cart base speed */
#define VEHICLE_SPEED_WAGON 1    /* Wagon base speed (slow, high capacity) */
#define VEHICLE_SPEED_MOUNT 4    /* Mount base speed (fast) */
#define VEHICLE_SPEED_CARRIAGE 2 /* Carriage base speed */

/* Placeholder types for testing (would come from MUD headers) */
typedef int room_rnum;
struct obj_data
{
  int placeholder;
};

/* Core vehicle structure - target <512 bytes */
struct vehicle_data
{
  /* Identity */
  int id;                         /* Unique vehicle ID */
  enum vehicle_type type;         /* Vehicle classification */
  enum vehicle_state state;       /* Current state */
  char name[VEHICLE_NAME_LENGTH]; /* Vehicle name/description */

  /* Location */
  room_rnum location; /* Current room */
  int direction;      /* Facing direction */

  /* Capacity */
  int max_passengers;     /* Maximum passenger count */
  int current_passengers; /* Current passenger count */
  int max_weight;         /* Weight capacity in pounds */
  int current_weight;     /* Current load weight */

  /* Movement */
  int base_speed;    /* Base movement speed */
  int current_speed; /* Modified speed */
  int terrain_flags; /* VTERRAIN_* bitfield */

  /* Condition */
  int max_condition; /* Maximum durability */
  int condition;     /* Current durability */

  /* Ownership */
  long owner_id; /* Player ID of owner */

  /* Object reference */
  struct obj_data *obj; /* Associated object */
};

/* ========================================================================= */
/* STRUCT SIZE VALIDATION TESTS                                              */
/* ========================================================================= */

/**
 * Test that vehicle_data struct is under 512 bytes.
 * Target size is 256-384 bytes for memory efficiency.
 */
void Test_vehicle_struct_size_under_512(CuTest *tc)
{
  size_t size = sizeof(struct vehicle_data);
  CuAssertTrue(tc, size < 512);
}

/**
 * Test that vehicle_data struct is reasonably compact.
 * Should be under 256 bytes ideally.
 */
void Test_vehicle_struct_size_compact(CuTest *tc)
{
  size_t size = sizeof(struct vehicle_data);
  /* Based on field analysis: ~136 bytes expected */
  CuAssertTrue(tc, size < 256);
}

/**
 * Test that vehicle_data struct has expected minimum size.
 * Should be at least 100 bytes given all the fields.
 */
void Test_vehicle_struct_size_minimum(CuTest *tc)
{
  size_t size = sizeof(struct vehicle_data);
  CuAssertTrue(tc, size >= 100);
}

/* ========================================================================= */
/* ENUM VALUE UNIQUENESS TESTS                                               */
/* ========================================================================= */

/**
 * Test vehicle_type enum values are unique.
 */
void Test_vehicle_type_enum_unique(CuTest *tc)
{
  int values[NUM_VEHICLE_TYPES];
  int i, j;

  values[0] = VEHICLE_NONE;
  values[1] = VEHICLE_CART;
  values[2] = VEHICLE_WAGON;
  values[3] = VEHICLE_MOUNT;
  values[4] = VEHICLE_CARRIAGE;

  /* Check all pairs for uniqueness */
  for (i = 0; i < NUM_VEHICLE_TYPES; i++)
  {
    for (j = i + 1; j < NUM_VEHICLE_TYPES; j++)
    {
      CuAssertTrue(tc, values[i] != values[j]);
    }
  }
}

/**
 * Test vehicle_type enum starts at 0 (VEHICLE_NONE).
 */
void Test_vehicle_type_enum_start(CuTest *tc)
{
  CuAssertIntEquals(tc, 0, VEHICLE_NONE);
}

/**
 * Test vehicle_type enum is contiguous.
 */
void Test_vehicle_type_enum_contiguous(CuTest *tc)
{
  CuAssertIntEquals(tc, 0, VEHICLE_NONE);
  CuAssertIntEquals(tc, 1, VEHICLE_CART);
  CuAssertIntEquals(tc, 2, VEHICLE_WAGON);
  CuAssertIntEquals(tc, 3, VEHICLE_MOUNT);
  CuAssertIntEquals(tc, 4, VEHICLE_CARRIAGE);
}

/**
 * Test vehicle_state enum values are unique.
 */
void Test_vehicle_state_enum_unique(CuTest *tc)
{
  int values[NUM_VEHICLE_STATES];
  int i, j;

  values[0] = VSTATE_IDLE;
  values[1] = VSTATE_MOVING;
  values[2] = VSTATE_LOADED;
  values[3] = VSTATE_HITCHED;
  values[4] = VSTATE_DAMAGED;

  /* Check all pairs for uniqueness */
  for (i = 0; i < NUM_VEHICLE_STATES; i++)
  {
    for (j = i + 1; j < NUM_VEHICLE_STATES; j++)
    {
      CuAssertTrue(tc, values[i] != values[j]);
    }
  }
}

/**
 * Test vehicle_state enum starts at 0 (VSTATE_IDLE).
 */
void Test_vehicle_state_enum_start(CuTest *tc)
{
  CuAssertIntEquals(tc, 0, VSTATE_IDLE);
}

/**
 * Test vehicle_state enum is contiguous.
 */
void Test_vehicle_state_enum_contiguous(CuTest *tc)
{
  CuAssertIntEquals(tc, 0, VSTATE_IDLE);
  CuAssertIntEquals(tc, 1, VSTATE_MOVING);
  CuAssertIntEquals(tc, 2, VSTATE_LOADED);
  CuAssertIntEquals(tc, 3, VSTATE_HITCHED);
  CuAssertIntEquals(tc, 4, VSTATE_DAMAGED);
}

/* ========================================================================= */
/* TERRAIN FLAG BIT UNIQUENESS TESTS                                         */
/* ========================================================================= */

/**
 * Test terrain flags are unique powers of 2.
 */
void Test_terrain_flags_unique_bits(CuTest *tc)
{
  int flags[7];
  int i, j;

  flags[0] = VTERRAIN_ROAD;
  flags[1] = VTERRAIN_PLAINS;
  flags[2] = VTERRAIN_FOREST;
  flags[3] = VTERRAIN_HILLS;
  flags[4] = VTERRAIN_MOUNTAIN;
  flags[5] = VTERRAIN_DESERT;
  flags[6] = VTERRAIN_SWAMP;

  /* Check all pairs have no overlapping bits */
  for (i = 0; i < 7; i++)
  {
    for (j = i + 1; j < 7; j++)
    {
      CuAssertTrue(tc, (flags[i] & flags[j]) == 0);
    }
  }
}

/**
 * Test each terrain flag is a power of 2.
 */
void Test_terrain_flags_powers_of_two(CuTest *tc)
{
  /* Each flag should have exactly one bit set */
  CuAssertIntEquals(tc, 1, VTERRAIN_ROAD);
  CuAssertIntEquals(tc, 2, VTERRAIN_PLAINS);
  CuAssertIntEquals(tc, 4, VTERRAIN_FOREST);
  CuAssertIntEquals(tc, 8, VTERRAIN_HILLS);
  CuAssertIntEquals(tc, 16, VTERRAIN_MOUNTAIN);
  CuAssertIntEquals(tc, 32, VTERRAIN_DESERT);
  CuAssertIntEquals(tc, 64, VTERRAIN_SWAMP);
}

/**
 * Test VTERRAIN_ALL combines all flags correctly.
 */
void Test_terrain_all_combines_flags(CuTest *tc)
{
  int combined = VTERRAIN_ROAD | VTERRAIN_PLAINS | VTERRAIN_FOREST | VTERRAIN_HILLS |
                 VTERRAIN_MOUNTAIN | VTERRAIN_DESERT | VTERRAIN_SWAMP;
  CuAssertIntEquals(tc, VTERRAIN_ALL, combined);
}

/**
 * Test terrain flag operations work correctly.
 */
void Test_terrain_flag_operations(CuTest *tc)
{
  int flags = 0;

  /* Test setting flags */
  flags |= VTERRAIN_ROAD;
  CuAssertTrue(tc, (flags & VTERRAIN_ROAD) != 0);

  flags |= VTERRAIN_PLAINS;
  CuAssertTrue(tc, (flags & VTERRAIN_ROAD) != 0);
  CuAssertTrue(tc, (flags & VTERRAIN_PLAINS) != 0);

  /* Test clearing flags */
  flags &= ~VTERRAIN_ROAD;
  CuAssertTrue(tc, (flags & VTERRAIN_ROAD) == 0);
  CuAssertTrue(tc, (flags & VTERRAIN_PLAINS) != 0);
}

/* ========================================================================= */
/* CONSTANT VALIDATION TESTS                                                 */
/* ========================================================================= */

/**
 * Test speed constants are positive.
 */
void Test_speed_constants_positive(CuTest *tc)
{
  CuAssertTrue(tc, VEHICLE_SPEED_CART > 0);
  CuAssertTrue(tc, VEHICLE_SPEED_WAGON > 0);
  CuAssertTrue(tc, VEHICLE_SPEED_MOUNT > 0);
  CuAssertTrue(tc, VEHICLE_SPEED_CARRIAGE > 0);
}

/**
 * Test mount is fastest vehicle.
 */
void Test_mount_is_fastest(CuTest *tc)
{
  CuAssertTrue(tc, VEHICLE_SPEED_MOUNT >= VEHICLE_SPEED_CART);
  CuAssertTrue(tc, VEHICLE_SPEED_MOUNT >= VEHICLE_SPEED_WAGON);
  CuAssertTrue(tc, VEHICLE_SPEED_MOUNT >= VEHICLE_SPEED_CARRIAGE);
}

/**
 * Test capacity constants are reasonable.
 */
void Test_capacity_constants_reasonable(CuTest *tc)
{
  CuAssertTrue(tc, VEHICLE_MAX_PASSENGERS > 0);
  CuAssertTrue(tc, VEHICLE_MAX_PASSENGERS <= 20);
  CuAssertTrue(tc, VEHICLE_MAX_WEIGHT > 0);
  CuAssertTrue(tc, VEHICLE_NAME_LENGTH > 0);
  CuAssertTrue(tc, VEHICLE_NAME_LENGTH <= 128);
}

/* ========================================================================= */
/* STRUCT FIELD TESTS                                                        */
/* ========================================================================= */

/**
 * Test vehicle_data struct can be zero-initialized.
 */
void Test_vehicle_struct_zero_init(CuTest *tc)
{
  struct vehicle_data vehicle;
  memset(&vehicle, 0, sizeof(vehicle));

  CuAssertIntEquals(tc, 0, vehicle.id);
  CuAssertIntEquals(tc, VEHICLE_NONE, vehicle.type);
  CuAssertIntEquals(tc, VSTATE_IDLE, vehicle.state);
  CuAssertIntEquals(tc, 0, vehicle.location);
  CuAssertIntEquals(tc, 0, vehicle.max_passengers);
  CuAssertIntEquals(tc, 0, vehicle.current_passengers);
  CuAssertIntEquals(tc, 0, vehicle.terrain_flags);
  CuAssertIntEquals(tc, 0, vehicle.owner_id);
  CuAssertTrue(tc, vehicle.obj == NULL);
}

/**
 * Test vehicle_data fields can be assigned.
 */
void Test_vehicle_struct_field_assignment(CuTest *tc)
{
  struct vehicle_data vehicle;
  memset(&vehicle, 0, sizeof(vehicle));

  vehicle.id = 42;
  vehicle.type = VEHICLE_WAGON;
  vehicle.state = VSTATE_MOVING;
  strncpy(vehicle.name, "Test Wagon", VEHICLE_NAME_LENGTH - 1);
  vehicle.name[VEHICLE_NAME_LENGTH - 1] = '\0';
  vehicle.location = 100;
  vehicle.direction = 2;
  vehicle.max_passengers = 6;
  vehicle.current_passengers = 3;
  vehicle.max_weight = 2000;
  vehicle.current_weight = 500;
  vehicle.base_speed = VEHICLE_SPEED_WAGON;
  vehicle.current_speed = 1;
  vehicle.terrain_flags = VTERRAIN_ROAD | VTERRAIN_PLAINS;
  vehicle.max_condition = 100;
  vehicle.condition = 75;
  vehicle.owner_id = 12345;
  vehicle.obj = NULL;

  CuAssertIntEquals(tc, 42, vehicle.id);
  CuAssertIntEquals(tc, VEHICLE_WAGON, vehicle.type);
  CuAssertIntEquals(tc, VSTATE_MOVING, vehicle.state);
  CuAssertStrEquals(tc, "Test Wagon", vehicle.name);
  CuAssertIntEquals(tc, 100, vehicle.location);
  CuAssertIntEquals(tc, 2, vehicle.direction);
  CuAssertIntEquals(tc, 6, vehicle.max_passengers);
  CuAssertIntEquals(tc, 3, vehicle.current_passengers);
  CuAssertIntEquals(tc, 2000, vehicle.max_weight);
  CuAssertIntEquals(tc, 500, vehicle.current_weight);
  CuAssertIntEquals(tc, VEHICLE_SPEED_WAGON, vehicle.base_speed);
  CuAssertIntEquals(tc, 1, vehicle.current_speed);
  CuAssertTrue(tc, (vehicle.terrain_flags & VTERRAIN_ROAD) != 0);
  CuAssertTrue(tc, (vehicle.terrain_flags & VTERRAIN_PLAINS) != 0);
  CuAssertIntEquals(tc, 100, vehicle.max_condition);
  CuAssertIntEquals(tc, 75, vehicle.condition);
  CuAssertIntEquals(tc, 12345, (int)vehicle.owner_id);
}

/**
 * Test name field buffer size.
 */
void Test_vehicle_name_buffer_size(CuTest *tc)
{
  struct vehicle_data vehicle;
  char long_name[VEHICLE_NAME_LENGTH + 10];
  int i;

  memset(&vehicle, 0, sizeof(vehicle));
  memset(long_name, 'A', sizeof(long_name) - 1);
  long_name[sizeof(long_name) - 1] = '\0';

  /* Safe copy with truncation */
  strncpy(vehicle.name, long_name, VEHICLE_NAME_LENGTH - 1);
  vehicle.name[VEHICLE_NAME_LENGTH - 1] = '\0';

  /* Verify truncation worked */
  CuAssertIntEquals(tc, VEHICLE_NAME_LENGTH - 1, (int)strlen(vehicle.name));
  for (i = 0; i < VEHICLE_NAME_LENGTH - 1; i++)
  {
    CuAssertTrue(tc, vehicle.name[i] == 'A');
  }
}

/* ========================================================================= */
/* TEST SUITE REGISTRATION                                                   */
/* ========================================================================= */

CuSuite *GetVehicleStructsSuite(void)
{
  CuSuite *suite = CuSuiteNew();

  /* Struct size tests */
  SUITE_ADD_TEST(suite, Test_vehicle_struct_size_under_512);
  SUITE_ADD_TEST(suite, Test_vehicle_struct_size_compact);
  SUITE_ADD_TEST(suite, Test_vehicle_struct_size_minimum);

  /* Vehicle type enum tests */
  SUITE_ADD_TEST(suite, Test_vehicle_type_enum_unique);
  SUITE_ADD_TEST(suite, Test_vehicle_type_enum_start);
  SUITE_ADD_TEST(suite, Test_vehicle_type_enum_contiguous);

  /* Vehicle state enum tests */
  SUITE_ADD_TEST(suite, Test_vehicle_state_enum_unique);
  SUITE_ADD_TEST(suite, Test_vehicle_state_enum_start);
  SUITE_ADD_TEST(suite, Test_vehicle_state_enum_contiguous);

  /* Terrain flag tests */
  SUITE_ADD_TEST(suite, Test_terrain_flags_unique_bits);
  SUITE_ADD_TEST(suite, Test_terrain_flags_powers_of_two);
  SUITE_ADD_TEST(suite, Test_terrain_all_combines_flags);
  SUITE_ADD_TEST(suite, Test_terrain_flag_operations);

  /* Constant validation tests */
  SUITE_ADD_TEST(suite, Test_speed_constants_positive);
  SUITE_ADD_TEST(suite, Test_mount_is_fastest);
  SUITE_ADD_TEST(suite, Test_capacity_constants_reasonable);

  /* Struct field tests */
  SUITE_ADD_TEST(suite, Test_vehicle_struct_zero_init);
  SUITE_ADD_TEST(suite, Test_vehicle_struct_field_assignment);
  SUITE_ADD_TEST(suite, Test_vehicle_name_buffer_size);

  return suite;
}

/* Standalone test runner */
int main(void)
{
  CuString *output = CuStringNew();
  CuSuite *suite = GetVehicleStructsSuite();

  CuSuiteRun(suite);
  CuSuiteSummary(suite, output);
  CuSuiteDetails(suite, output);

  printf("%s\n", output->buffer);

  CuStringDelete(output);
  CuSuiteDelete(suite);

  return 0;
}
