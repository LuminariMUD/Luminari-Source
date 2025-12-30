/**
 * @file test_transport_unified.c
 * @brief Unit tests for Phase 02, Session 06 unified transport interface
 *
 * Tests transport type enumeration, transport_data structure,
 * and transport type detection helper functions.
 *
 * Part of Phase 02, Session 06: Unified Command Interface
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "CuTest.h"

/* ========================================================================= */
/* TRANSPORT TYPE DEFINITIONS (copied from transport_unified.h for testing) */
/* ========================================================================= */

/**
 * Transport type classification for unified command handling.
 */
enum transport_type
{
  TRANSPORT_NONE = 0,
  TRANSPORT_VEHICLE,
  TRANSPORT_VESSEL
};

/* Placeholder types for standalone testing */
typedef int room_rnum;
typedef int bool;
#define TRUE 1
#define FALSE 0
#define NOWHERE -1

/* Vehicle type enum (minimal copy from vessels.h) */
enum vehicle_type
{
  VEHICLE_NONE = 0,
  VEHICLE_CART,
  VEHICLE_WAGON,
  VEHICLE_MOUNT,
  VEHICLE_CARRIAGE,
  NUM_VEHICLE_TYPES
};

/* Vehicle state enum (minimal copy from vessels.h) */
enum vehicle_state
{
  VSTATE_IDLE = 0,
  VSTATE_MOVING,
  VSTATE_LOADED,
  VSTATE_HITCHED,
  VSTATE_DAMAGED,
  NUM_VEHICLE_STATES
};

/* Minimal vehicle_data structure for testing */
struct vehicle_data
{
  int id;
  enum vehicle_type type;
  enum vehicle_state state;
  char name[64];
  room_rnum location;
  int condition;
  int max_condition;
  int current_passengers;
  int max_passengers;
  int current_weight;
  int max_weight;
  int base_speed;
  int current_speed;
  int x_coord;
  int y_coord;
};

/* Minimal greyhawk_ship_data structure for testing */
struct greyhawk_ship_data
{
  char id[32];
  char name[64];
  char owner[64];
  float x, y, z;
  int heading;
  int speed;
  int maxspeed;
  int num_rooms;
  int dock;
};

/**
 * Transport data abstraction structure for unified handling.
 */
struct transport_data
{
  enum transport_type type;
  union
  {
    struct vehicle_data *vehicle;
    struct greyhawk_ship_data *vessel;
  } data;
};

/* ========================================================================= */
/* HELPER FUNCTION IMPLEMENTATIONS FOR STANDALONE TESTING                     */
/* ========================================================================= */

/**
 * Get display name for a transport type.
 */
static const char *transport_type_name(enum transport_type type)
{
  switch (type)
  {
  case TRANSPORT_VEHICLE:
    return "vehicle";
  case TRANSPORT_VESSEL:
    return "vessel";
  case TRANSPORT_NONE:
  default:
    return "unknown";
  }
}

/**
 * Get the name of a specific transport.
 */
static const char *get_transport_name(struct transport_data *td)
{
  if (td == NULL)
  {
    return "unknown";
  }

  switch (td->type)
  {
  case TRANSPORT_VEHICLE:
    if (td->data.vehicle != NULL)
    {
      return td->data.vehicle->name;
    }
    break;
  case TRANSPORT_VESSEL:
    if (td->data.vessel != NULL)
    {
      return td->data.vessel->name;
    }
    break;
  default:
    break;
  }

  return "unknown";
}

/**
 * Check if a transport is operational.
 */
static int is_transport_operational(struct transport_data *td)
{
  if (td == NULL)
  {
    return FALSE;
  }

  switch (td->type)
  {
  case TRANSPORT_VEHICLE:
    if (td->data.vehicle != NULL)
    {
      return (td->data.vehicle->condition > 0);
    }
    break;
  case TRANSPORT_VESSEL:
    if (td->data.vessel != NULL)
    {
      return TRUE;
    }
    break;
  default:
    break;
  }

  return FALSE;
}

/* ========================================================================= */
/* TEST: Transport Type Enumeration                                          */
/* ========================================================================= */

/**
 * Test that transport type enum values are correctly ordered.
 */
void test_transport_type_enum_values(CuTest *tc)
{
  CuAssertIntEquals(tc, 0, TRANSPORT_NONE);
  CuAssertIntEquals(tc, 1, TRANSPORT_VEHICLE);
  CuAssertIntEquals(tc, 2, TRANSPORT_VESSEL);
}

/**
 * Test transport type name strings.
 */
void test_transport_type_names(CuTest *tc)
{
  CuAssertStrEquals(tc, "unknown", transport_type_name(TRANSPORT_NONE));
  CuAssertStrEquals(tc, "vehicle", transport_type_name(TRANSPORT_VEHICLE));
  CuAssertStrEquals(tc, "vessel", transport_type_name(TRANSPORT_VESSEL));
}

/* ========================================================================= */
/* TEST: Transport Data Structure                                             */
/* ========================================================================= */

/**
 * Test transport_data structure size is reasonable.
 */
void test_transport_data_struct_size(CuTest *tc)
{
  size_t size = sizeof(struct transport_data);

  /* Structure should be small - just enum + pointer */
  CuAssertTrue(tc, size <= 16);

  printf("  transport_data size: %lu bytes\n", (unsigned long)size);
}

/**
 * Test transport_data initialization with vehicle.
 */
void test_transport_data_vehicle_init(CuTest *tc)
{
  struct transport_data td;
  struct vehicle_data vehicle;

  /* Initialize vehicle */
  memset(&vehicle, 0, sizeof(vehicle));
  vehicle.id = 42;
  vehicle.type = VEHICLE_WAGON;
  vehicle.state = VSTATE_IDLE;
  strncpy(vehicle.name, "Test Wagon", sizeof(vehicle.name) - 1);
  vehicle.condition = 100;
  vehicle.max_condition = 100;

  /* Initialize transport_data */
  td.type = TRANSPORT_VEHICLE;
  td.data.vehicle = &vehicle;

  /* Verify */
  CuAssertIntEquals(tc, TRANSPORT_VEHICLE, td.type);
  CuAssertPtrNotNull(tc, td.data.vehicle);
  CuAssertIntEquals(tc, 42, td.data.vehicle->id);
  CuAssertStrEquals(tc, "Test Wagon", td.data.vehicle->name);
}

/**
 * Test transport_data initialization with vessel.
 */
void test_transport_data_vessel_init(CuTest *tc)
{
  struct transport_data td;
  struct greyhawk_ship_data vessel;

  /* Initialize vessel */
  memset(&vessel, 0, sizeof(vessel));
  strncpy(vessel.id, "ship001", sizeof(vessel.id) - 1);
  strncpy(vessel.name, "The Flying Dutchman", sizeof(vessel.name) - 1);
  strncpy(vessel.owner, "Captain Hook", sizeof(vessel.owner) - 1);
  vessel.speed = 5;
  vessel.maxspeed = 10;

  /* Initialize transport_data */
  td.type = TRANSPORT_VESSEL;
  td.data.vessel = &vessel;

  /* Verify */
  CuAssertIntEquals(tc, TRANSPORT_VESSEL, td.type);
  CuAssertPtrNotNull(tc, td.data.vessel);
  CuAssertStrEquals(tc, "ship001", td.data.vessel->id);
  CuAssertStrEquals(tc, "The Flying Dutchman", td.data.vessel->name);
}

/**
 * Test transport_data with TRANSPORT_NONE type.
 */
void test_transport_data_none(CuTest *tc)
{
  struct transport_data td;

  td.type = TRANSPORT_NONE;
  td.data.vehicle = NULL;

  CuAssertIntEquals(tc, TRANSPORT_NONE, td.type);
  CuAssertPtrEquals(tc, NULL, td.data.vehicle);
}

/* ========================================================================= */
/* TEST: Transport Name Helper                                                */
/* ========================================================================= */

/**
 * Test getting transport name for vehicle.
 */
void test_get_transport_name_vehicle(CuTest *tc)
{
  struct transport_data td;
  struct vehicle_data vehicle;

  memset(&vehicle, 0, sizeof(vehicle));
  strncpy(vehicle.name, "Royal Carriage", sizeof(vehicle.name) - 1);

  td.type = TRANSPORT_VEHICLE;
  td.data.vehicle = &vehicle;

  CuAssertStrEquals(tc, "Royal Carriage", get_transport_name(&td));
}

/**
 * Test getting transport name for vessel.
 */
void test_get_transport_name_vessel(CuTest *tc)
{
  struct transport_data td;
  struct greyhawk_ship_data vessel;

  memset(&vessel, 0, sizeof(vessel));
  strncpy(vessel.name, "HMS Victory", sizeof(vessel.name) - 1);

  td.type = TRANSPORT_VESSEL;
  td.data.vessel = &vessel;

  CuAssertStrEquals(tc, "HMS Victory", get_transport_name(&td));
}

/**
 * Test getting transport name for NULL transport.
 */
void test_get_transport_name_null(CuTest *tc)
{
  CuAssertStrEquals(tc, "unknown", get_transport_name(NULL));
}

/**
 * Test getting transport name for TRANSPORT_NONE.
 */
void test_get_transport_name_none(CuTest *tc)
{
  struct transport_data td;

  td.type = TRANSPORT_NONE;
  td.data.vehicle = NULL;

  CuAssertStrEquals(tc, "unknown", get_transport_name(&td));
}

/* ========================================================================= */
/* TEST: Transport Operational Check                                          */
/* ========================================================================= */

/**
 * Test operational check for functional vehicle.
 */
void test_is_transport_operational_vehicle_ok(CuTest *tc)
{
  struct transport_data td;
  struct vehicle_data vehicle;

  memset(&vehicle, 0, sizeof(vehicle));
  vehicle.condition = 75;
  vehicle.max_condition = 100;

  td.type = TRANSPORT_VEHICLE;
  td.data.vehicle = &vehicle;

  CuAssertIntEquals(tc, TRUE, is_transport_operational(&td));
}

/**
 * Test operational check for damaged vehicle (condition = 0).
 */
void test_is_transport_operational_vehicle_broken(CuTest *tc)
{
  struct transport_data td;
  struct vehicle_data vehicle;

  memset(&vehicle, 0, sizeof(vehicle));
  vehicle.condition = 0;
  vehicle.max_condition = 100;

  td.type = TRANSPORT_VEHICLE;
  td.data.vehicle = &vehicle;

  CuAssertIntEquals(tc, FALSE, is_transport_operational(&td));
}

/**
 * Test operational check for vessel (always operational if exists).
 */
void test_is_transport_operational_vessel(CuTest *tc)
{
  struct transport_data td;
  struct greyhawk_ship_data vessel;

  memset(&vessel, 0, sizeof(vessel));
  strncpy(vessel.name, "Test Ship", sizeof(vessel.name) - 1);

  td.type = TRANSPORT_VESSEL;
  td.data.vessel = &vessel;

  CuAssertIntEquals(tc, TRUE, is_transport_operational(&td));
}

/**
 * Test operational check for NULL transport.
 */
void test_is_transport_operational_null(CuTest *tc)
{
  CuAssertIntEquals(tc, FALSE, is_transport_operational(NULL));
}

/**
 * Test operational check for TRANSPORT_NONE.
 */
void test_is_transport_operational_none(CuTest *tc)
{
  struct transport_data td;

  td.type = TRANSPORT_NONE;
  td.data.vehicle = NULL;

  CuAssertIntEquals(tc, FALSE, is_transport_operational(&td));
}

/* ========================================================================= */
/* TEST SUITE REGISTRATION                                                    */
/* ========================================================================= */

/**
 * Get the transport unified test suite.
 */
CuSuite *GetTransportUnifiedSuite(void)
{
  CuSuite *suite = CuSuiteNew();

  /* Transport type enum tests */
  SUITE_ADD_TEST(suite, test_transport_type_enum_values);
  SUITE_ADD_TEST(suite, test_transport_type_names);

  /* Transport data structure tests */
  SUITE_ADD_TEST(suite, test_transport_data_struct_size);
  SUITE_ADD_TEST(suite, test_transport_data_vehicle_init);
  SUITE_ADD_TEST(suite, test_transport_data_vessel_init);
  SUITE_ADD_TEST(suite, test_transport_data_none);

  /* Transport name helper tests */
  SUITE_ADD_TEST(suite, test_get_transport_name_vehicle);
  SUITE_ADD_TEST(suite, test_get_transport_name_vessel);
  SUITE_ADD_TEST(suite, test_get_transport_name_null);
  SUITE_ADD_TEST(suite, test_get_transport_name_none);

  /* Transport operational check tests */
  SUITE_ADD_TEST(suite, test_is_transport_operational_vehicle_ok);
  SUITE_ADD_TEST(suite, test_is_transport_operational_vehicle_broken);
  SUITE_ADD_TEST(suite, test_is_transport_operational_vessel);
  SUITE_ADD_TEST(suite, test_is_transport_operational_null);
  SUITE_ADD_TEST(suite, test_is_transport_operational_none);

  return suite;
}

/* ========================================================================= */
/* MAIN FUNCTION FOR STANDALONE EXECUTION                                     */
/* ========================================================================= */

#ifdef STANDALONE_TEST
int main(void)
{
  CuString *output;
  CuSuite *suite;
  int result;

  output = CuStringNew();
  suite = GetTransportUnifiedSuite();

  printf("Running Transport Unified Tests...\n\n");

  CuSuiteRun(suite);
  CuSuiteSummary(suite, output);
  CuSuiteDetails(suite, output);

  printf("%s\n", output->buffer);

  result = (suite->failCount > 0) ? 1 : 0;

  CuStringDelete(output);
  CuSuiteDelete(suite);

  return result;
}
#endif /* STANDALONE_TEST */
