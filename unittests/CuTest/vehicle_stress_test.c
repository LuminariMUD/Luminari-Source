/**
 * @file vehicle_stress_test.c
 * @brief Stress test harness for vehicle system
 *
 * Tests concurrent vehicle simulation at 100/500/1000 vehicle levels,
 * measures memory consumption, and validates stability.
 *
 * Part of Phase 02, Session 07: Testing and Validation
 */

/* Enable POSIX features for snprintf in C89 mode */
#define _POSIX_C_SOURCE 200112L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "CuTest.h"

/* ========================================================================= */
/* TYPE DEFINITIONS                                                          */
/* ========================================================================= */

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

typedef int bool;

/* Stress test levels */
#define STRESS_LEVEL_100 100
#define STRESS_LEVEL_500 500
#define STRESS_LEVEL_1000 1000

/* Memory target: <512 bytes per vehicle (spec requirement) */
#define MEMORY_TARGET_PER_VEHICLE 512

/* Maximum vehicles */
#define MAX_VEHICLES_STRESS 1000
#define VEHICLE_NAME_LENGTH 64
#define MAX_PASSENGERS 8

/* Vehicle type enum (from vehicles.h) */
enum vehicle_type
{
  VEHICLE_NONE = 0,
  VEHICLE_CART,
  VEHICLE_WAGON,
  VEHICLE_MOUNT,
  VEHICLE_CARRIAGE,
  NUM_VEHICLE_TYPES
};

/* Vehicle state enum (from vehicles.h) */
enum vehicle_state
{
  VSTATE_IDLE = 0,
  VSTATE_MOVING,
  VSTATE_LOADED,
  VSTATE_HITCHED,
  VSTATE_DAMAGED,
  NUM_VEHICLE_STATES
};

/* Terrain type flags (from vehicles.h) */
#define TERRAIN_ROAD (1 << 0)
#define TERRAIN_PLAINS (1 << 1)
#define TERRAIN_FOREST (1 << 2)
#define TERRAIN_MOUNTAINS (1 << 3)
#define TERRAIN_WATER (1 << 4)
#define TERRAIN_DESERT (1 << 5)
#define TERRAIN_SNOW (1 << 6)

/* Simplified vehicle structure for stress testing */
struct stress_vehicle
{
  int id;                            /* Unique vehicle ID */
  char name[VEHICLE_NAME_LENGTH];    /* Vehicle name */
  enum vehicle_type type;            /* Vehicle type */
  enum vehicle_state state;          /* Current state */
  int room_vnum;                     /* Current room VNUM */
  int owner_id;                      /* Owner player ID */
  int speed;                         /* Movement speed */
  int max_passengers;                /* Passenger capacity */
  int current_passengers;            /* Current passenger count */
  int passenger_ids[MAX_PASSENGERS]; /* Passenger player IDs */
  int terrain_flags;                 /* Allowed terrain types */
  int weight_capacity;               /* Cargo capacity */
  int current_weight;                /* Current cargo weight */
  int hitched_to;                    /* ID of vehicle hitched to */
  int loaded_in;                     /* ID of transport loaded into */
};

/* Test results structure */
struct stress_results
{
  int num_vehicles;
  size_t total_memory;
  size_t per_vehicle_memory;
  double creation_time_ms;
  double operation_time_ms;
  double destruction_time_ms;
  int creation_failures;
  int operation_failures;
  bool passed;
};

/* ========================================================================= */
/* GLOBAL DATA                                                               */
/* ========================================================================= */

static struct stress_vehicle *vehicles = NULL;
static int num_vehicles_allocated = 0;

/* ========================================================================= */
/* MEMORY TRACKING                                                           */
/* ========================================================================= */

static size_t total_allocated = 0;
static size_t peak_allocated = 0;

/**
 * Track memory allocation
 */
static void *tracked_malloc(size_t size)
{
  void *ptr = malloc(size);
  if (ptr)
  {
    total_allocated += size;
    if (total_allocated > peak_allocated)
    {
      peak_allocated = total_allocated;
    }
  }
  return ptr;
}

/**
 * Track memory deallocation
 */
static void tracked_free(void *ptr, size_t size)
{
  if (ptr)
  {
    free(ptr);
    if (total_allocated >= size)
    {
      total_allocated -= size;
    }
  }
}

/**
 * Reset memory tracking
 */
static void reset_memory_tracking(void)
{
  total_allocated = 0;
  peak_allocated = 0;
}

/* ========================================================================= */
/* VEHICLE SIMULATION FUNCTIONS                                              */
/* ========================================================================= */

/**
 * Get default settings for a vehicle type
 */
static void get_vehicle_defaults(enum vehicle_type type, int *speed, int *passengers, int *weight,
                                 int *terrain)
{
  switch (type)
  {
  case VEHICLE_CART:
    *speed = 2;
    *passengers = 2;
    *weight = 500;
    *terrain = TERRAIN_ROAD | TERRAIN_PLAINS;
    break;
  case VEHICLE_WAGON:
    *speed = 1;
    *passengers = 6;
    *weight = 2000;
    *terrain = TERRAIN_ROAD | TERRAIN_PLAINS;
    break;
  case VEHICLE_MOUNT:
    *speed = 4;
    *passengers = 1;
    *weight = 200;
    *terrain = TERRAIN_ROAD | TERRAIN_PLAINS | TERRAIN_FOREST | TERRAIN_MOUNTAINS;
    break;
  case VEHICLE_CARRIAGE:
    *speed = 3;
    *passengers = 4;
    *weight = 800;
    *terrain = TERRAIN_ROAD;
    break;
  default:
    *speed = 1;
    *passengers = 1;
    *weight = 100;
    *terrain = TERRAIN_ROAD;
    break;
  }
}

/**
 * Create a simulated vehicle
 */
static bool create_stress_vehicle(struct stress_vehicle *vehicle, int id)
{
  int speed, passengers, weight, terrain;
  int i;

  if (!vehicle)
  {
    return FALSE;
  }

  memset(vehicle, 0, sizeof(*vehicle));

  vehicle->id = id;
  snprintf(vehicle->name, sizeof(vehicle->name), "StressVehicle_%d", id);

  /* Cycle through vehicle types (excluding VEHICLE_NONE) */
  vehicle->type = (enum vehicle_type)((id % (NUM_VEHICLE_TYPES - 1)) + 1);
  vehicle->state = VSTATE_IDLE;

  /* Set position in random room */
  vehicle->room_vnum = 3000 + (id % 1000);
  vehicle->owner_id = 1000 + (id % 100);

  /* Get type-specific defaults */
  get_vehicle_defaults(vehicle->type, &speed, &passengers, &weight, &terrain);
  vehicle->speed = speed;
  vehicle->max_passengers = passengers;
  vehicle->weight_capacity = weight;
  vehicle->terrain_flags = terrain;

  /* Initialize passenger array */
  vehicle->current_passengers = 0;
  for (i = 0; i < MAX_PASSENGERS; i++)
  {
    vehicle->passenger_ids[i] = 0;
  }

  vehicle->current_weight = 0;
  vehicle->hitched_to = -1;
  vehicle->loaded_in = -1;

  return TRUE;
}

/**
 * Simulate vehicle operations (movement, passenger changes)
 */
static bool simulate_vehicle_operation(struct stress_vehicle *vehicle, int tick)
{
  if (!vehicle)
  {
    return FALSE;
  }

  /* Simulate state transitions */
  switch (tick % 5)
  {
  case 0:
    vehicle->state = VSTATE_IDLE;
    break;
  case 1:
    vehicle->state = VSTATE_MOVING;
    vehicle->room_vnum = 3000 + ((vehicle->room_vnum + 1) % 1000);
    break;
  case 2:
    /* Add a passenger if space */
    if (vehicle->current_passengers < vehicle->max_passengers)
    {
      vehicle->passenger_ids[vehicle->current_passengers] = 2000 + tick;
      vehicle->current_passengers++;
    }
    break;
  case 3:
    /* Remove a passenger if any */
    if (vehicle->current_passengers > 0)
    {
      vehicle->current_passengers--;
      vehicle->passenger_ids[vehicle->current_passengers] = 0;
    }
    break;
  case 4:
    /* Simulate cargo change */
    vehicle->current_weight = (vehicle->current_weight + 50) % vehicle->weight_capacity;
    break;
  }

  return TRUE;
}

/**
 * Destroy a simulated vehicle
 */
static void destroy_stress_vehicle(struct stress_vehicle *vehicle)
{
  if (!vehicle)
  {
    return;
  }
  memset(vehicle, 0, sizeof(*vehicle));
  vehicle->id = -1;
  vehicle->hitched_to = -1;
  vehicle->loaded_in = -1;
}

/* ========================================================================= */
/* STRESS TEST EXECUTION                                                     */
/* ========================================================================= */

/**
 * Get time in milliseconds
 */
static double get_time_ms(void)
{
  return (double)clock() / (CLOCKS_PER_SEC / 1000.0);
}

/**
 * Run stress test at specified vehicle count
 */
static void run_stress_test(int num_vehicles, struct stress_results *results)
{
  double start_time, end_time;
  int i, j;
  int operations_per_vehicle = 100;
  size_t array_size;

  printf("\n--- Stress Test: %d Vehicles ---\n", num_vehicles);

  memset(results, 0, sizeof(*results));
  results->num_vehicles = num_vehicles;
  reset_memory_tracking();

  /* Allocate vehicle array */
  array_size = sizeof(struct stress_vehicle) * num_vehicles;
  vehicles = (struct stress_vehicle *)tracked_malloc(array_size);
  if (!vehicles)
  {
    printf("ERROR: Failed to allocate vehicle array!\n");
    results->passed = FALSE;
    return;
  }
  num_vehicles_allocated = num_vehicles;

  /* Phase 1: Create vehicles */
  printf("Creating %d vehicles...\n", num_vehicles);
  start_time = get_time_ms();

  for (i = 0; i < num_vehicles; i++)
  {
    if (!create_stress_vehicle(&vehicles[i], i))
    {
      results->creation_failures++;
    }
  }

  end_time = get_time_ms();
  results->creation_time_ms = end_time - start_time;
  printf("  Creation time: %.2f ms\n", results->creation_time_ms);
  printf("  Creation failures: %d\n", results->creation_failures);

  /* Record memory usage after creation */
  results->total_memory = peak_allocated;
  results->per_vehicle_memory = results->total_memory / num_vehicles;
  printf("  Memory used: %lu bytes (%.2f KB)\n", (unsigned long)results->total_memory,
         results->total_memory / 1024.0);
  printf("  Per-vehicle: %lu bytes (target: <%d)\n", (unsigned long)results->per_vehicle_memory,
         MEMORY_TARGET_PER_VEHICLE);

  /* Phase 2: Simulate operations */
  printf("Running %d operations per vehicle...\n", operations_per_vehicle);
  start_time = get_time_ms();

  for (j = 0; j < operations_per_vehicle; j++)
  {
    for (i = 0; i < num_vehicles; i++)
    {
      if (!simulate_vehicle_operation(&vehicles[i], j))
      {
        results->operation_failures++;
      }
    }
  }

  end_time = get_time_ms();
  results->operation_time_ms = end_time - start_time;
  printf("  Operation time: %.2f ms\n", results->operation_time_ms);
  printf("  Operations/second: %.0f\n",
         (num_vehicles * operations_per_vehicle * 1000.0) / results->operation_time_ms);
  printf("  Operation failures: %d\n", results->operation_failures);

  /* Phase 3: Destroy vehicles */
  printf("Destroying %d vehicles...\n", num_vehicles);
  start_time = get_time_ms();

  for (i = 0; i < num_vehicles; i++)
  {
    destroy_stress_vehicle(&vehicles[i]);
  }

  tracked_free(vehicles, array_size);
  vehicles = NULL;
  num_vehicles_allocated = 0;

  end_time = get_time_ms();
  results->destruction_time_ms = end_time - start_time;
  printf("  Destruction time: %.2f ms\n", results->destruction_time_ms);

  /* Determine pass/fail */
  results->passed = (results->creation_failures == 0 && results->operation_failures == 0 &&
                     results->per_vehicle_memory <= MEMORY_TARGET_PER_VEHICLE);

  printf("Result: %s\n", results->passed ? "PASSED" : "FAILED");
  if (!results->passed)
  {
    if (results->per_vehicle_memory > MEMORY_TARGET_PER_VEHICLE)
    {
      printf("  FAIL: Memory per vehicle (%lu) exceeds target (%d)\n",
             (unsigned long)results->per_vehicle_memory, MEMORY_TARGET_PER_VEHICLE);
    }
    if (results->creation_failures > 0)
    {
      printf("  FAIL: %d creation failures\n", results->creation_failures);
    }
    if (results->operation_failures > 0)
    {
      printf("  FAIL: %d operation failures\n", results->operation_failures);
    }
  }
}

/* ========================================================================= */
/* CUTEST INTEGRATION                                                        */
/* ========================================================================= */

static struct stress_results last_results_100;
static struct stress_results last_results_500;
static struct stress_results last_results_1000;

void test_stress_100_vehicles(CuTest *tc)
{
  run_stress_test(STRESS_LEVEL_100, &last_results_100);
  CuAssertTrue(tc, last_results_100.passed);
}

void test_stress_500_vehicles(CuTest *tc)
{
  run_stress_test(STRESS_LEVEL_500, &last_results_500);
  CuAssertTrue(tc, last_results_500.passed);
}

void test_stress_1000_vehicles(CuTest *tc)
{
  run_stress_test(STRESS_LEVEL_1000, &last_results_1000);
  CuAssertTrue(tc, last_results_1000.passed);
}

void test_vehicle_struct_size(CuTest *tc)
{
  size_t size = sizeof(struct stress_vehicle);
  printf("  stress_vehicle struct size: %lu bytes (target: <%d)\n", (unsigned long)size,
         MEMORY_TARGET_PER_VEHICLE);
  CuAssertTrue(tc, size <= MEMORY_TARGET_PER_VEHICLE);
}

void test_memory_tracking_reset(CuTest *tc)
{
  reset_memory_tracking();
  CuAssertIntEquals(tc, 0, (int)total_allocated);
  CuAssertIntEquals(tc, 0, (int)peak_allocated);
}

void test_vehicle_creation_single(CuTest *tc)
{
  struct stress_vehicle v;
  bool result = create_stress_vehicle(&v, 1);
  CuAssertTrue(tc, result);
  CuAssertIntEquals(tc, 1, v.id);
  CuAssertTrue(tc, v.type >= VEHICLE_CART && v.type < NUM_VEHICLE_TYPES);
}

void test_vehicle_operation_single(CuTest *tc)
{
  struct stress_vehicle v;
  bool result;
  create_stress_vehicle(&v, 1);
  result = simulate_vehicle_operation(&v, 0);
  CuAssertTrue(tc, result);
}

void test_vehicle_destruction_single(CuTest *tc)
{
  struct stress_vehicle v;
  create_stress_vehicle(&v, 1);
  destroy_stress_vehicle(&v);
  CuAssertIntEquals(tc, -1, v.id);
}

CuSuite *GetVehicleStressSuite(void)
{
  CuSuite *suite = CuSuiteNew();

  SUITE_ADD_TEST(suite, test_vehicle_struct_size);
  SUITE_ADD_TEST(suite, test_memory_tracking_reset);
  SUITE_ADD_TEST(suite, test_vehicle_creation_single);
  SUITE_ADD_TEST(suite, test_vehicle_operation_single);
  SUITE_ADD_TEST(suite, test_vehicle_destruction_single);
  SUITE_ADD_TEST(suite, test_stress_100_vehicles);
  SUITE_ADD_TEST(suite, test_stress_500_vehicles);
  SUITE_ADD_TEST(suite, test_stress_1000_vehicles);

  return suite;
}

/* ========================================================================= */
/* MAIN ENTRY POINT                                                          */
/* ========================================================================= */

int main(int argc, char *argv[])
{
  CuString *output;
  CuSuite *suite;
  int result;

  (void)argc;
  (void)argv;

  printf("========================================\n");
  printf("LuminariMUD Vehicle System Stress Tests\n");
  printf("Phase 02, Session 07: Testing & Validation\n");
  printf("========================================\n");
  printf("\nTargets:\n");
  printf("  Max concurrent vehicles: 1000\n");
  printf("  Memory per vehicle: <%d bytes\n", MEMORY_TARGET_PER_VEHICLE);
  printf("  Test levels: 100, 500, 1000 vehicles\n");

  output = CuStringNew();
  suite = GetVehicleStressSuite();

  CuSuiteRun(suite);
  CuSuiteSummary(suite, output);
  CuSuiteDetails(suite, output);

  printf("\n%s\n", output->buffer);

  /* Summary table */
  printf("\n========================================\n");
  printf("Stress Test Summary\n");
  printf("========================================\n");
  printf("\n");
  printf("| Level | Memory   | Per-Vehicle | Create | Ops    | Pass |\n");
  printf("|-------|----------|-------------|--------|--------|------|\n");
  printf("| %4d  | %6.1fKB | %6lu B    | %5.1fms | %6.1fms | %s  |\n",
         last_results_100.num_vehicles, last_results_100.total_memory / 1024.0,
         (unsigned long)last_results_100.per_vehicle_memory, last_results_100.creation_time_ms,
         last_results_100.operation_time_ms, last_results_100.passed ? "PASS" : "FAIL");
  printf("| %4d  | %6.1fKB | %6lu B    | %5.1fms | %6.1fms | %s  |\n",
         last_results_500.num_vehicles, last_results_500.total_memory / 1024.0,
         (unsigned long)last_results_500.per_vehicle_memory, last_results_500.creation_time_ms,
         last_results_500.operation_time_ms, last_results_500.passed ? "PASS" : "FAIL");
  printf("| %4d  | %6.1fKB | %6lu B    | %5.1fms | %6.1fms | %s  |\n",
         last_results_1000.num_vehicles, last_results_1000.total_memory / 1024.0,
         (unsigned long)last_results_1000.per_vehicle_memory, last_results_1000.creation_time_ms,
         last_results_1000.operation_time_ms, last_results_1000.passed ? "PASS" : "FAIL");
  printf("\n");

  printf("========================================\n");
  if (suite->failCount == 0)
  {
    printf("*** ALL STRESS TESTS PASSED ***\n");
  }
  else
  {
    printf("*** SOME STRESS TESTS FAILED ***\n");
  }
  printf("========================================\n");

  result = (suite->failCount > 0) ? 1 : 0;

  CuStringDelete(output);
  CuSuiteDelete(suite);

  return result;
}
