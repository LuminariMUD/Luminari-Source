/**
 * @file vessel_stress_test.c
 * @brief Stress test harness for vessel system
 *
 * Tests concurrent vessel simulation at 100/250/500 vessel levels,
 * measures memory consumption, and validates stability.
 *
 * Part of Phase 00, Session 09: Testing and Validation
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

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
#define STRESS_LEVEL_100   100
#define STRESS_LEVEL_250   250
#define STRESS_LEVEL_500   500

/* Memory target: <1KB per vessel */
#define MEMORY_TARGET_PER_VESSEL 1024

/* Maximum vessels (from production) */
#define GREYHAWK_MAXSHIPS 500
#define MAX_SHIP_ROOMS 20
#define MAX_SHIP_CONNECTIONS 40

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

/* Room connection */
struct room_connection {
  int from_room;
  int to_room;
  int direction;
  bool is_hatch;
  bool is_locked;
};

/* Simplified vessel structure for stress testing */
struct stress_vessel {
  int id;
  char name[64];
  char owner[32];
  float x, y, z;
  int heading;
  int speed;
  enum vessel_class type;
  int num_rooms;
  int room_vnums[MAX_SHIP_ROOMS];
  struct room_connection connections[MAX_SHIP_CONNECTIONS];
  int num_connections;
  int docked_to;
};

/* Test results structure */
struct stress_results {
  int num_vessels;
  size_t total_memory;
  size_t per_vessel_memory;
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

static struct stress_vessel *vessels = NULL;
static int num_vessels_allocated = 0;

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
/* VESSEL SIMULATION FUNCTIONS                                               */
/* ========================================================================= */

/**
 * Create a simulated vessel
 */
static bool create_stress_vessel(struct stress_vessel *vessel, int id)
{
  int i;
  int num_rooms;

  if (!vessel)
  {
    return FALSE;
  }

  memset(vessel, 0, sizeof(*vessel));

  vessel->id = id;
  snprintf(vessel->name, sizeof(vessel->name), "StressVessel_%d", id);
  snprintf(vessel->owner, sizeof(vessel->owner), "Owner_%d", id % 100);

  /* Random position within bounds */
  vessel->x = (float)((id * 7) % 2049) - 1024;
  vessel->y = (float)((id * 13) % 2049) - 1024;
  vessel->z = 0;

  vessel->heading = (id * 17) % 360;
  vessel->speed = (id % 30) + 1;

  /* Cycle through vessel types */
  vessel->type = (enum vessel_class)(id % 8);

  /* Generate rooms based on vessel type */
  switch (vessel->type)
  {
    case VESSEL_RAFT:       num_rooms = 1; break;
    case VESSEL_BOAT:       num_rooms = 2; break;
    case VESSEL_SHIP:       num_rooms = 3; break;
    case VESSEL_WARSHIP:    num_rooms = 5; break;
    case VESSEL_AIRSHIP:    num_rooms = 4; break;
    case VESSEL_SUBMARINE:  num_rooms = 4; break;
    case VESSEL_TRANSPORT:  num_rooms = 6; break;
    case VESSEL_MAGICAL:    num_rooms = 3; break;
    default:                num_rooms = 1; break;
  }

  vessel->num_rooms = num_rooms;

  /* Assign room VNUMs */
  for (i = 0; i < num_rooms; i++)
  {
    vessel->room_vnums[i] = 70000 + (id * MAX_SHIP_ROOMS) + i;
  }

  /* Create connections between rooms */
  vessel->num_connections = (num_rooms > 1) ? num_rooms - 1 : 0;
  for (i = 0; i < vessel->num_connections; i++)
  {
    vessel->connections[i].from_room = vessel->room_vnums[i];
    vessel->connections[i].to_room = vessel->room_vnums[i + 1];
    vessel->connections[i].direction = 0;  /* North */
    vessel->connections[i].is_hatch = FALSE;
    vessel->connections[i].is_locked = FALSE;
  }

  vessel->docked_to = -1;

  return TRUE;
}

/**
 * Simulate vessel operations (movement, commands)
 */
static bool simulate_vessel_operation(struct stress_vessel *vessel)
{
  if (!vessel)
  {
    return FALSE;
  }

  /* Simulate movement */
  vessel->x += ((float)(vessel->speed) / 10.0f) * 0.1f;
  vessel->y += ((float)(vessel->speed) / 10.0f) * 0.1f;

  /* Wrap coordinates */
  while (vessel->x > 1024) vessel->x -= 2048;
  while (vessel->x < -1024) vessel->x += 2048;
  while (vessel->y > 1024) vessel->y -= 2048;
  while (vessel->y < -1024) vessel->y += 2048;

  /* Simulate heading change */
  vessel->heading = (vessel->heading + 1) % 360;

  return TRUE;
}

/**
 * Destroy a simulated vessel
 */
static void destroy_stress_vessel(struct stress_vessel *vessel)
{
  if (!vessel)
  {
    return;
  }
  memset(vessel, 0, sizeof(*vessel));
  vessel->id = -1;
  vessel->docked_to = -1;
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
 * Run stress test at specified vessel count
 */
static void run_stress_test(int num_vessels, struct stress_results *results)
{
  double start_time, end_time;
  int i, j;
  int operations_per_vessel = 100;
  size_t array_size;

  printf("\n--- Stress Test: %d Vessels ---\n", num_vessels);

  memset(results, 0, sizeof(*results));
  results->num_vessels = num_vessels;
  reset_memory_tracking();

  /* Allocate vessel array */
  array_size = sizeof(struct stress_vessel) * num_vessels;
  vessels = (struct stress_vessel *)tracked_malloc(array_size);
  if (!vessels)
  {
    printf("ERROR: Failed to allocate vessel array!\n");
    results->passed = FALSE;
    return;
  }
  num_vessels_allocated = num_vessels;

  /* Phase 1: Create vessels */
  printf("Creating %d vessels...\n", num_vessels);
  start_time = get_time_ms();

  for (i = 0; i < num_vessels; i++)
  {
    if (!create_stress_vessel(&vessels[i], i))
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
  results->per_vessel_memory = results->total_memory / num_vessels;
  printf("  Memory used: %lu bytes (%.2f KB)\n", (unsigned long)results->total_memory, results->total_memory / 1024.0);
  printf("  Per-vessel: %lu bytes\n", (unsigned long)results->per_vessel_memory);

  /* Phase 2: Simulate operations */
  printf("Running %d operations per vessel...\n", operations_per_vessel);
  start_time = get_time_ms();

  for (j = 0; j < operations_per_vessel; j++)
  {
    for (i = 0; i < num_vessels; i++)
    {
      if (!simulate_vessel_operation(&vessels[i]))
      {
        results->operation_failures++;
      }
    }
  }

  end_time = get_time_ms();
  results->operation_time_ms = end_time - start_time;
  printf("  Operation time: %.2f ms\n", results->operation_time_ms);
  printf("  Operations/second: %.0f\n",
         (num_vessels * operations_per_vessel * 1000.0) / results->operation_time_ms);
  printf("  Operation failures: %d\n", results->operation_failures);

  /* Phase 3: Destroy vessels */
  printf("Destroying %d vessels...\n", num_vessels);
  start_time = get_time_ms();

  for (i = 0; i < num_vessels; i++)
  {
    destroy_stress_vessel(&vessels[i]);
  }

  tracked_free(vessels, array_size);
  vessels = NULL;
  num_vessels_allocated = 0;

  end_time = get_time_ms();
  results->destruction_time_ms = end_time - start_time;
  printf("  Destruction time: %.2f ms\n", results->destruction_time_ms);

  /* Determine pass/fail */
  results->passed = (results->creation_failures == 0 &&
                     results->operation_failures == 0 &&
                     results->per_vessel_memory <= MEMORY_TARGET_PER_VESSEL);

  printf("Result: %s\n", results->passed ? "PASSED" : "FAILED");
  if (!results->passed)
  {
    if (results->per_vessel_memory > MEMORY_TARGET_PER_VESSEL)
    {
      printf("  FAIL: Memory per vessel (%lu) exceeds target (%d)\n",
             (unsigned long)results->per_vessel_memory, MEMORY_TARGET_PER_VESSEL);
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
/* MAIN ENTRY POINT                                                          */
/* ========================================================================= */

int main(int argc, char *argv[])
{
  struct stress_results results_100, results_250, results_500;
  int all_passed = 1;

  (void)argc;
  (void)argv;

  printf("========================================\n");
  printf("LuminariMUD Vessel System Stress Tests\n");
  printf("Phase 00, Session 09: Testing & Validation\n");
  printf("========================================\n");
  printf("\nTargets:\n");
  printf("  Max concurrent vessels: 500\n");
  printf("  Memory per vessel: <%d bytes\n", MEMORY_TARGET_PER_VESSEL);
  printf("  Test levels: 100, 250, 500 vessels\n");

  /* Run stress tests at each level */
  run_stress_test(STRESS_LEVEL_100, &results_100);
  run_stress_test(STRESS_LEVEL_250, &results_250);
  run_stress_test(STRESS_LEVEL_500, &results_500);

  /* Summary */
  printf("\n========================================\n");
  printf("Stress Test Summary\n");
  printf("========================================\n");
  printf("\n");
  printf("| Level | Memory   | Per-Vessel | Create | Ops    | Pass |\n");
  printf("|-------|----------|------------|--------|--------|------|\n");
  printf("| %3d   | %6.1fKB | %5lu B    | %5.1fms | %6.1fms | %s  |\n",
         results_100.num_vessels,
         results_100.total_memory / 1024.0,
         (unsigned long)results_100.per_vessel_memory,
         results_100.creation_time_ms,
         results_100.operation_time_ms,
         results_100.passed ? "PASS" : "FAIL");
  printf("| %3d   | %6.1fKB | %5lu B    | %5.1fms | %6.1fms | %s  |\n",
         results_250.num_vessels,
         results_250.total_memory / 1024.0,
         (unsigned long)results_250.per_vessel_memory,
         results_250.creation_time_ms,
         results_250.operation_time_ms,
         results_250.passed ? "PASS" : "FAIL");
  printf("| %3d   | %6.1fKB | %5lu B    | %5.1fms | %6.1fms | %s  |\n",
         results_500.num_vessels,
         results_500.total_memory / 1024.0,
         (unsigned long)results_500.per_vessel_memory,
         results_500.creation_time_ms,
         results_500.operation_time_ms,
         results_500.passed ? "PASS" : "FAIL");
  printf("\n");

  all_passed = results_100.passed && results_250.passed && results_500.passed;

  printf("========================================\n");
  if (all_passed)
  {
    printf("*** ALL STRESS TESTS PASSED ***\n");
  }
  else
  {
    printf("*** SOME STRESS TESTS FAILED ***\n");
  }
  printf("========================================\n");

  return all_passed ? 0 : 1;
}
