/**
 * @file test_vessel_types.c
 * @brief Unit tests for vessel type system
 *
 * Tests vessel classification, terrain capabilities, and type-specific
 * behavior for the vessel system.
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

typedef int bool;

/* Vessel class enum */
enum vessel_class
{
  VESSEL_RAFT = 0,
  VESSEL_BOAT = 1,
  VESSEL_SHIP = 2,
  VESSEL_WARSHIP = 3,
  VESSEL_AIRSHIP = 4,
  VESSEL_SUBMARINE = 5,
  VESSEL_TRANSPORT = 6,
  VESSEL_MAGICAL = 7
};

#define NUM_VESSEL_TYPES 8

/* Sector types used in terrain validation */
#define SECT_INSIDE 0
#define SECT_WATER_SWIM 6
#define SECT_WATER_NOSWIM 7
#define SECT_UNDERWATER 9
#define SECT_OCEAN 15
#define SECT_MARSHLAND 16
#define SECT_BEACH 33
#define SECT_RIVER 36

/* Vessel terrain capabilities */
struct vessel_terrain_caps
{
  bool can_traverse_ocean;
  bool can_traverse_shallow;
  bool can_traverse_air;
  bool can_traverse_underwater;
  int min_water_depth;
  int max_altitude;
  float terrain_speed_mod[40];
};

/* ========================================================================= */
/* TERRAIN CAPABILITY DATA (mirrors production)                               */
/* ========================================================================= */

static const struct vessel_terrain_caps vessel_terrain_data[NUM_VESSEL_TYPES] = {
    /* VESSEL_RAFT: Small, rivers/shallow water only */
    {FALSE, TRUE, FALSE, FALSE, 0, 0, {0, 0, 0,  0, 0, 0,  100, 0, 0,   0, 0, 0, 0, 0,
                                       0, 0, 75, 0, 0, 0,  0,   0, 80,  0, 0, 0, 0, 0,
                                       0, 0, 0,  0, 0, 50, 60,  0, 100, 0, 0, 0}},

    /* VESSEL_BOAT: Medium, coastal waters */
    {FALSE, TRUE, FALSE, FALSE, 0, 0, {0, 0, 0,  0, 0, 0,  100, 75, 0,   0,  0, 0, 0, 0,
                                       0, 0, 80, 0, 0, 0,  0,   0,  90,  60, 0, 0, 0, 0,
                                       0, 0, 0,  0, 0, 60, 70,  0,  100, 0,  0, 0}},

    /* VESSEL_SHIP: Large, ocean-capable */
    {TRUE, TRUE, FALSE, FALSE, 2, 0, {0, 0,   0, 0, 0, 0, 75, 100, 0,  0,  0, 0, 0, 0,
                                      0, 100, 0, 0, 0, 0, 0,  0,   0,  80, 0, 0, 0, 0,
                                      0, 0,   0, 0, 0, 0, 50, 0,   50, 0,  0, 0}},

    /* VESSEL_WARSHIP: Combat vessel */
    {TRUE, TRUE, FALSE, FALSE, 2, 0, {0, 0,   0, 0, 0, 0, 75, 100, 0,  0,  0, 0, 0, 0,
                                      0, 100, 0, 0, 0, 0, 0,  0,   0,  80, 0, 0, 0, 0,
                                      0, 0,   0, 0, 0, 0, 50, 0,   50, 0,  0, 0}},

    /* VESSEL_AIRSHIP: Flying vessel */
    {TRUE, TRUE, TRUE, FALSE, 0, 500, {0,   80,  100, 100, 100, 100, 100, 100, 100, 0,
                                       0,   100, 100, 100, 100, 100, 100, 100, 100, 0,
                                       0,   0,   0,   0,   0,   80,  100, 100, 100, 0,
                                       100, 100, 100, 100, 100, 0,   100, 0,   0,   0}},

    /* VESSEL_SUBMARINE: Underwater vessel */
    {TRUE, TRUE, FALSE, TRUE, 0, 0, {0, 0,   0, 0, 0, 0, 0,  100, 0,   100, 0, 0, 0, 0,
                                     0, 100, 0, 0, 0, 0, 0,  0,   100, 100, 0, 0, 0, 0,
                                     0, 0,   0, 0, 0, 0, 50, 0,   0,   0,   0, 0}},

    /* VESSEL_TRANSPORT: Cargo vessel */
    {TRUE, TRUE, FALSE, FALSE, 2, 0, {0, 0,   0, 0, 0, 0, 60, 100, 0,  0,  0, 0, 0, 0,
                                      0, 100, 0, 0, 0, 0, 0,  0,   0,  70, 0, 0, 0, 0,
                                      0, 0,   0, 0, 0, 0, 40, 0,   40, 0,  0, 0}},

    /* VESSEL_MAGICAL: Special magical vessels */
    {TRUE, TRUE, TRUE, TRUE, 0, 300, {50, 50, 75,  75,  75,  75,  100, 100, 100, 100,
                                      50, 75, 75,  75,  100, 100, 100, 100, 100, 50,
                                      50, 50, 100, 100, 50,  0,   75,  75,  75,  50,
                                      75, 75, 75,  80,  80,  50,  100, 0,   0,   0}}};

/* ========================================================================= */
/* HELPER FUNCTIONS                                                          */
/* ========================================================================= */

/**
 * Get terrain capabilities for vessel type
 */
static const struct vessel_terrain_caps *get_vessel_terrain_caps(enum vessel_class type)
{
  if (type < 0 || type >= NUM_VESSEL_TYPES)
  {
    return NULL;
  }
  return &vessel_terrain_data[type];
}

/**
 * Get base rooms for vessel type
 */
static int get_base_rooms_for_type(enum vessel_class type)
{
  switch (type)
  {
  case VESSEL_RAFT:
    return 1;
  case VESSEL_BOAT:
    return 2;
  case VESSEL_SHIP:
    return 3;
  case VESSEL_WARSHIP:
    return 5;
  case VESSEL_AIRSHIP:
    return 4;
  case VESSEL_SUBMARINE:
    return 4;
  case VESSEL_TRANSPORT:
    return 6;
  case VESSEL_MAGICAL:
    return 3;
  default:
    return 1;
  }
}

/**
 * Get max rooms for vessel type
 */
static int get_max_rooms_for_type(enum vessel_class type)
{
  switch (type)
  {
  case VESSEL_RAFT:
    return 2;
  case VESSEL_BOAT:
    return 4;
  case VESSEL_SHIP:
    return 8;
  case VESSEL_WARSHIP:
    return 15;
  case VESSEL_AIRSHIP:
    return 10;
  case VESSEL_SUBMARINE:
    return 12;
  case VESSEL_TRANSPORT:
    return 20;
  case VESSEL_MAGICAL:
    return 10;
  default:
    return 1;
  }
}

/**
 * Derive vessel type from hull weight
 */
static enum vessel_class derive_vessel_type_from_template(int hullweight)
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
 * Check if vessel can traverse ocean
 */
static bool can_traverse_ocean(enum vessel_class type)
{
  const struct vessel_terrain_caps *caps = get_vessel_terrain_caps(type);
  if (!caps)
    return FALSE;
  return caps->can_traverse_ocean;
}

/**
 * Check if vessel can fly
 */
static bool can_traverse_air(enum vessel_class type)
{
  const struct vessel_terrain_caps *caps = get_vessel_terrain_caps(type);
  if (!caps)
    return FALSE;
  return caps->can_traverse_air;
}

/**
 * Check if vessel can go underwater
 */
static bool can_traverse_underwater(enum vessel_class type)
{
  const struct vessel_terrain_caps *caps = get_vessel_terrain_caps(type);
  if (!caps)
    return FALSE;
  return caps->can_traverse_underwater;
}

/* ========================================================================= */
/* VESSEL TYPE CAPABILITY TESTS                                              */
/* ========================================================================= */

/**
 * Test raft capabilities
 */
void Test_vessel_raft_capabilities(CuTest *tc)
{
  const struct vessel_terrain_caps *caps = get_vessel_terrain_caps(VESSEL_RAFT);

  CuAssertPtrNotNull(tc, caps);
  CuAssertIntEquals(tc, FALSE, caps->can_traverse_ocean);
  CuAssertIntEquals(tc, TRUE, caps->can_traverse_shallow);
  CuAssertIntEquals(tc, FALSE, caps->can_traverse_air);
  CuAssertIntEquals(tc, FALSE, caps->can_traverse_underwater);
  CuAssertIntEquals(tc, 0, caps->min_water_depth);
  CuAssertIntEquals(tc, 0, caps->max_altitude);
}

/**
 * Test boat capabilities
 */
void Test_vessel_boat_capabilities(CuTest *tc)
{
  const struct vessel_terrain_caps *caps = get_vessel_terrain_caps(VESSEL_BOAT);

  CuAssertPtrNotNull(tc, caps);
  CuAssertIntEquals(tc, FALSE, caps->can_traverse_ocean);
  CuAssertIntEquals(tc, TRUE, caps->can_traverse_shallow);
  CuAssertIntEquals(tc, FALSE, caps->can_traverse_air);
  CuAssertIntEquals(tc, FALSE, caps->can_traverse_underwater);
}

/**
 * Test ship capabilities (ocean-going)
 */
void Test_vessel_ship_capabilities(CuTest *tc)
{
  const struct vessel_terrain_caps *caps = get_vessel_terrain_caps(VESSEL_SHIP);

  CuAssertPtrNotNull(tc, caps);
  CuAssertIntEquals(tc, TRUE, caps->can_traverse_ocean);
  CuAssertIntEquals(tc, TRUE, caps->can_traverse_shallow);
  CuAssertIntEquals(tc, FALSE, caps->can_traverse_air);
  CuAssertIntEquals(tc, FALSE, caps->can_traverse_underwater);
  CuAssertIntEquals(tc, 2, caps->min_water_depth);
}

/**
 * Test airship capabilities
 */
void Test_vessel_airship_capabilities(CuTest *tc)
{
  const struct vessel_terrain_caps *caps = get_vessel_terrain_caps(VESSEL_AIRSHIP);

  CuAssertPtrNotNull(tc, caps);
  CuAssertIntEquals(tc, TRUE, caps->can_traverse_ocean);
  CuAssertIntEquals(tc, TRUE, caps->can_traverse_shallow);
  CuAssertIntEquals(tc, TRUE, caps->can_traverse_air);
  CuAssertIntEquals(tc, FALSE, caps->can_traverse_underwater);
  CuAssertIntEquals(tc, 500, caps->max_altitude);
}

/**
 * Test submarine capabilities
 */
void Test_vessel_submarine_capabilities(CuTest *tc)
{
  const struct vessel_terrain_caps *caps = get_vessel_terrain_caps(VESSEL_SUBMARINE);

  CuAssertPtrNotNull(tc, caps);
  CuAssertIntEquals(tc, TRUE, caps->can_traverse_ocean);
  CuAssertIntEquals(tc, TRUE, caps->can_traverse_shallow);
  CuAssertIntEquals(tc, FALSE, caps->can_traverse_air);
  CuAssertIntEquals(tc, TRUE, caps->can_traverse_underwater);
}

/**
 * Test warship capabilities
 */
void Test_vessel_warship_capabilities(CuTest *tc)
{
  const struct vessel_terrain_caps *caps = get_vessel_terrain_caps(VESSEL_WARSHIP);

  CuAssertPtrNotNull(tc, caps);
  CuAssertIntEquals(tc, TRUE, caps->can_traverse_ocean);
  CuAssertIntEquals(tc, TRUE, caps->can_traverse_shallow);
  CuAssertIntEquals(tc, FALSE, caps->can_traverse_air);
  CuAssertIntEquals(tc, FALSE, caps->can_traverse_underwater);
  CuAssertIntEquals(tc, 2, caps->min_water_depth);
  CuAssertIntEquals(tc, 0, caps->max_altitude);
}

/**
 * Test transport capabilities
 */
void Test_vessel_transport_capabilities(CuTest *tc)
{
  const struct vessel_terrain_caps *caps = get_vessel_terrain_caps(VESSEL_TRANSPORT);

  CuAssertPtrNotNull(tc, caps);
  CuAssertIntEquals(tc, TRUE, caps->can_traverse_ocean);
  CuAssertIntEquals(tc, TRUE, caps->can_traverse_shallow);
  CuAssertIntEquals(tc, FALSE, caps->can_traverse_air);
  CuAssertIntEquals(tc, FALSE, caps->can_traverse_underwater);
  CuAssertIntEquals(tc, 2, caps->min_water_depth);
  CuAssertIntEquals(tc, 0, caps->max_altitude);
}

/**
 * Test magical vessel capabilities (most versatile)
 */
void Test_vessel_magical_capabilities(CuTest *tc)
{
  const struct vessel_terrain_caps *caps = get_vessel_terrain_caps(VESSEL_MAGICAL);

  CuAssertPtrNotNull(tc, caps);
  CuAssertIntEquals(tc, TRUE, caps->can_traverse_ocean);
  CuAssertIntEquals(tc, TRUE, caps->can_traverse_shallow);
  CuAssertIntEquals(tc, TRUE, caps->can_traverse_air);
  CuAssertIntEquals(tc, TRUE, caps->can_traverse_underwater);
  CuAssertIntEquals(tc, 300, caps->max_altitude);
}

/* ========================================================================= */
/* TERRAIN TRAVERSAL TESTS                                                   */
/* ========================================================================= */

/**
 * Test ocean traversal by vessel type
 */
void Test_vessel_ocean_traversal(CuTest *tc)
{
  /* Small vessels cannot cross ocean */
  CuAssertIntEquals(tc, FALSE, can_traverse_ocean(VESSEL_RAFT));
  CuAssertIntEquals(tc, FALSE, can_traverse_ocean(VESSEL_BOAT));

  /* Large vessels can cross ocean */
  CuAssertIntEquals(tc, TRUE, can_traverse_ocean(VESSEL_SHIP));
  CuAssertIntEquals(tc, TRUE, can_traverse_ocean(VESSEL_WARSHIP));
  CuAssertIntEquals(tc, TRUE, can_traverse_ocean(VESSEL_TRANSPORT));

  /* Special vessels can cross ocean */
  CuAssertIntEquals(tc, TRUE, can_traverse_ocean(VESSEL_AIRSHIP));
  CuAssertIntEquals(tc, TRUE, can_traverse_ocean(VESSEL_SUBMARINE));
  CuAssertIntEquals(tc, TRUE, can_traverse_ocean(VESSEL_MAGICAL));
}

/**
 * Test air traversal by vessel type
 */
void Test_vessel_air_traversal(CuTest *tc)
{
  /* Most vessels cannot fly */
  CuAssertIntEquals(tc, FALSE, can_traverse_air(VESSEL_RAFT));
  CuAssertIntEquals(tc, FALSE, can_traverse_air(VESSEL_BOAT));
  CuAssertIntEquals(tc, FALSE, can_traverse_air(VESSEL_SHIP));
  CuAssertIntEquals(tc, FALSE, can_traverse_air(VESSEL_WARSHIP));
  CuAssertIntEquals(tc, FALSE, can_traverse_air(VESSEL_SUBMARINE));
  CuAssertIntEquals(tc, FALSE, can_traverse_air(VESSEL_TRANSPORT));

  /* Only airships and magical can fly */
  CuAssertIntEquals(tc, TRUE, can_traverse_air(VESSEL_AIRSHIP));
  CuAssertIntEquals(tc, TRUE, can_traverse_air(VESSEL_MAGICAL));
}

/**
 * Test underwater traversal by vessel type
 */
void Test_vessel_underwater_traversal(CuTest *tc)
{
  /* Most vessels cannot go underwater */
  CuAssertIntEquals(tc, FALSE, can_traverse_underwater(VESSEL_RAFT));
  CuAssertIntEquals(tc, FALSE, can_traverse_underwater(VESSEL_BOAT));
  CuAssertIntEquals(tc, FALSE, can_traverse_underwater(VESSEL_SHIP));
  CuAssertIntEquals(tc, FALSE, can_traverse_underwater(VESSEL_WARSHIP));
  CuAssertIntEquals(tc, FALSE, can_traverse_underwater(VESSEL_AIRSHIP));
  CuAssertIntEquals(tc, FALSE, can_traverse_underwater(VESSEL_TRANSPORT));

  /* Only submarines and magical can go underwater */
  CuAssertIntEquals(tc, TRUE, can_traverse_underwater(VESSEL_SUBMARINE));
  CuAssertIntEquals(tc, TRUE, can_traverse_underwater(VESSEL_MAGICAL));
}

/* ========================================================================= */
/* HULL WEIGHT DERIVATION TESTS                                              */
/* ========================================================================= */

/**
 * Test hull weight boundaries for raft
 */
void Test_vessel_hullweight_raft(CuTest *tc)
{
  CuAssertIntEquals(tc, VESSEL_RAFT, derive_vessel_type_from_template(0));
  CuAssertIntEquals(tc, VESSEL_RAFT, derive_vessel_type_from_template(25));
  CuAssertIntEquals(tc, VESSEL_RAFT, derive_vessel_type_from_template(49));
}

/**
 * Test hull weight boundaries for boat
 */
void Test_vessel_hullweight_boat(CuTest *tc)
{
  CuAssertIntEquals(tc, VESSEL_BOAT, derive_vessel_type_from_template(50));
  CuAssertIntEquals(tc, VESSEL_BOAT, derive_vessel_type_from_template(100));
  CuAssertIntEquals(tc, VESSEL_BOAT, derive_vessel_type_from_template(149));
}

/**
 * Test hull weight boundaries for ship
 */
void Test_vessel_hullweight_ship(CuTest *tc)
{
  CuAssertIntEquals(tc, VESSEL_SHIP, derive_vessel_type_from_template(150));
  CuAssertIntEquals(tc, VESSEL_SHIP, derive_vessel_type_from_template(300));
  CuAssertIntEquals(tc, VESSEL_SHIP, derive_vessel_type_from_template(399));
}

/**
 * Test hull weight boundaries for warship
 */
void Test_vessel_hullweight_warship(CuTest *tc)
{
  CuAssertIntEquals(tc, VESSEL_WARSHIP, derive_vessel_type_from_template(400));
  CuAssertIntEquals(tc, VESSEL_WARSHIP, derive_vessel_type_from_template(600));
  CuAssertIntEquals(tc, VESSEL_WARSHIP, derive_vessel_type_from_template(799));
}

/**
 * Test hull weight boundaries for transport
 */
void Test_vessel_hullweight_transport(CuTest *tc)
{
  CuAssertIntEquals(tc, VESSEL_TRANSPORT, derive_vessel_type_from_template(800));
  CuAssertIntEquals(tc, VESSEL_TRANSPORT, derive_vessel_type_from_template(1000));
  CuAssertIntEquals(tc, VESSEL_TRANSPORT, derive_vessel_type_from_template(99999));
}

/* ========================================================================= */
/* ROOM COUNT TESTS                                                          */
/* ========================================================================= */

/**
 * Test room counts are consistent
 */
void Test_vessel_room_counts_consistency(CuTest *tc)
{
  int i;

  for (i = 0; i < NUM_VESSEL_TYPES; i++)
  {
    int base = get_base_rooms_for_type((enum vessel_class)i);
    int max = get_max_rooms_for_type((enum vessel_class)i);

    /* Base rooms should be at least 1 */
    CuAssertTrue(tc, base >= 1);

    /* Max should always be >= base */
    CuAssertTrue(tc, max >= base);

    /* Max should not exceed 20 (system limit) */
    CuAssertTrue(tc, max <= 20);
  }
}

/**
 * Test warship has more rooms than raft
 */
void Test_vessel_room_scaling(CuTest *tc)
{
  /* Larger vessels should have more rooms */
  CuAssertTrue(tc, get_base_rooms_for_type(VESSEL_SHIP) > get_base_rooms_for_type(VESSEL_RAFT));
  CuAssertTrue(tc, get_base_rooms_for_type(VESSEL_WARSHIP) > get_base_rooms_for_type(VESSEL_BOAT));
  CuAssertTrue(tc,
               get_max_rooms_for_type(VESSEL_TRANSPORT) >= get_max_rooms_for_type(VESSEL_WARSHIP));
}

/* ========================================================================= */
/* INVALID INPUT TESTS                                                       */
/* ========================================================================= */

/**
 * Test invalid vessel type handling
 */
void Test_vessel_invalid_type(CuTest *tc)
{
  const struct vessel_terrain_caps *caps;

  caps = get_vessel_terrain_caps(-1);
  CuAssertPtrEquals(tc, NULL, caps);

  caps = get_vessel_terrain_caps(NUM_VESSEL_TYPES);
  CuAssertPtrEquals(tc, NULL, caps);

  caps = get_vessel_terrain_caps(100);
  CuAssertPtrEquals(tc, NULL, caps);
}

/**
 * Test default room count for invalid type
 */
void Test_vessel_invalid_type_rooms(CuTest *tc)
{
  CuAssertIntEquals(tc, 1, get_base_rooms_for_type(-1));
  CuAssertIntEquals(tc, 1, get_max_rooms_for_type(-1));
  CuAssertIntEquals(tc, 1, get_base_rooms_for_type(100));
  CuAssertIntEquals(tc, 1, get_max_rooms_for_type(100));
}

/* ========================================================================= */
/* TEST SUITE REGISTRATION                                                   */
/* ========================================================================= */

/**
 * Get the vessel type test suite
 */
CuSuite *VesselTypesGetSuite(void)
{
  CuSuite *suite = CuSuiteNew();

  /* Capability tests */
  SUITE_ADD_TEST(suite, Test_vessel_raft_capabilities);
  SUITE_ADD_TEST(suite, Test_vessel_boat_capabilities);
  SUITE_ADD_TEST(suite, Test_vessel_ship_capabilities);
  SUITE_ADD_TEST(suite, Test_vessel_warship_capabilities);
  SUITE_ADD_TEST(suite, Test_vessel_airship_capabilities);
  SUITE_ADD_TEST(suite, Test_vessel_submarine_capabilities);
  SUITE_ADD_TEST(suite, Test_vessel_transport_capabilities);
  SUITE_ADD_TEST(suite, Test_vessel_magical_capabilities);

  /* Traversal tests */
  SUITE_ADD_TEST(suite, Test_vessel_ocean_traversal);
  SUITE_ADD_TEST(suite, Test_vessel_air_traversal);
  SUITE_ADD_TEST(suite, Test_vessel_underwater_traversal);

  /* Hull weight tests */
  SUITE_ADD_TEST(suite, Test_vessel_hullweight_raft);
  SUITE_ADD_TEST(suite, Test_vessel_hullweight_boat);
  SUITE_ADD_TEST(suite, Test_vessel_hullweight_ship);
  SUITE_ADD_TEST(suite, Test_vessel_hullweight_warship);
  SUITE_ADD_TEST(suite, Test_vessel_hullweight_transport);

  /* Room count tests */
  SUITE_ADD_TEST(suite, Test_vessel_room_counts_consistency);
  SUITE_ADD_TEST(suite, Test_vessel_room_scaling);

  /* Invalid input tests */
  SUITE_ADD_TEST(suite, Test_vessel_invalid_type);
  SUITE_ADD_TEST(suite, Test_vessel_invalid_type_rooms);

  return suite;
}
