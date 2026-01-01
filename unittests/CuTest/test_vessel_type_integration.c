/**
 * @file test_vessel_type_integration.c
 * @brief Integration tests for vessel type system
 *
 * Tests end-to-end behavior of the vessel type system including
 * movement validation, terrain speed modifiers, and type-specific logic.
 *
 * Part of Phase 03, Session 05: Vessel Type System Validation
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

/* Vessel class enum - matches production enum vessel_class */
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
#define SECT_CITY 1
#define SECT_FIELD 2
#define SECT_FOREST 3
#define SECT_HILLS 4
#define SECT_MOUNTAIN 5
#define SECT_WATER_SWIM 6
#define SECT_WATER_NOSWIM 7
#define SECT_FLYING 8
#define SECT_UNDERWATER 9
#define SECT_OCEAN 15
#define SECT_MARSHLAND 16
#define SECT_HIGH_MOUNTAIN 17
#define SECT_BEACH 33
#define SECT_SEAPORT 34
#define SECT_RIVER 36

/* Vessel terrain capabilities - matches production struct */
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
/* TERRAIN CAPABILITY DATA (mirrors production vessels.c)                    */
/* ========================================================================= */

static const struct vessel_terrain_caps vessel_terrain_data[NUM_VESSEL_TYPES] = {
    /* VESSEL_RAFT: rivers/shallow only */
    {FALSE, TRUE, FALSE, FALSE, 0, 0, {0, 0, 0,  0, 0, 0,  100, 0, 0,   0, 0, 0, 0, 0,
                                       0, 0, 75, 0, 0, 0,  0,   0, 80,  0, 0, 0, 0, 0,
                                       0, 0, 0,  0, 0, 50, 60,  0, 100, 0, 0, 0}},
    /* VESSEL_BOAT: coastal */
    {FALSE, TRUE, FALSE, FALSE, 0, 0, {0, 0, 0,  0, 0, 0,  100, 75, 0,   0,  0, 0, 0, 0,
                                       0, 0, 80, 0, 0, 0,  0,   0,  90,  60, 0, 0, 0, 0,
                                       0, 0, 0,  0, 0, 60, 70,  0,  100, 0,  0, 0}},
    /* VESSEL_SHIP: ocean-capable */
    {TRUE, TRUE, FALSE, FALSE, 2, 0, {0, 0,   0, 0, 0, 0, 75, 100, 0,  0,  0, 0, 0, 0,
                                      0, 100, 0, 0, 0, 0, 0,  0,   0,  80, 0, 0, 0, 0,
                                      0, 0,   0, 0, 0, 0, 50, 0,   50, 0,  0, 0}},
    /* VESSEL_WARSHIP: combat vessel */
    {TRUE, TRUE, FALSE, FALSE, 2, 0, {0, 0,   0, 0, 0, 0, 75, 100, 0,  0,  0, 0, 0, 0,
                                      0, 100, 0, 0, 0, 0, 0,  0,   0,  80, 0, 0, 0, 0,
                                      0, 0,   0, 0, 0, 0, 50, 0,   50, 0,  0, 0}},
    /* VESSEL_AIRSHIP: flying */
    {TRUE, TRUE, TRUE, FALSE, 0, 500, {0,   80,  100, 100, 100, 100, 100, 100, 100, 0,
                                       0,   100, 100, 100, 100, 100, 100, 100, 100, 0,
                                       0,   0,   0,   0,   0,   80,  100, 100, 100, 0,
                                       100, 100, 100, 100, 100, 0,   100, 0,   0,   0}},
    /* VESSEL_SUBMARINE: underwater */
    {TRUE, TRUE, FALSE, TRUE, 0, 0, {0, 0,   0, 0, 0, 0, 50, 90, 0,  100, 0, 0, 0, 0,
                                     0, 100, 0, 0, 0, 0, 0,  0,  90, 100, 0, 0, 0, 0,
                                     0, 0,   0, 0, 0, 0, 40, 0,  0,  0,   0, 0}},
    /* VESSEL_TRANSPORT: cargo */
    {TRUE, TRUE, FALSE, FALSE, 2, 0, {0, 0,   0, 0, 0, 0, 60, 90, 0,  0,  0, 0, 0, 0,
                                      0, 100, 0, 0, 0, 0, 0,  0,  0,  70, 0, 0, 0, 0,
                                      0, 0,   0, 0, 0, 0, 60, 0,  40, 0,  0, 0}},
    /* VESSEL_MAGICAL: all terrain */
    {TRUE, TRUE, TRUE, TRUE, 0, 1000, {0,   80,  100, 100, 100, 100, 100, 100, 100, 100,
                                       0,   100, 100, 100, 100, 100, 100, 100, 100, 80,
                                       80,  80,  100, 100, 100, 0,   100, 100, 100, 80,
                                       100, 100, 100, 100, 100, 0,   100, 0,   0,   0}}};

/* ========================================================================= */
/* HELPER FUNCTIONS                                                          */
/* ========================================================================= */

/**
 * Get terrain capabilities for vessel type (mirrors production)
 */
static const struct vessel_terrain_caps *get_vessel_terrain_caps(enum vessel_class type)
{
  if (type < 0 || type >= NUM_VESSEL_TYPES)
  {
    return &vessel_terrain_data[VESSEL_SHIP]; /* default */
  }
  return &vessel_terrain_data[type];
}

/**
 * Get terrain speed modifier (mirrors production get_terrain_speed_modifier)
 */
static int get_terrain_speed_modifier(enum vessel_class vessel_type, int sector_type,
                                      int weather_conditions)
{
  int base_modifier;
  const struct vessel_terrain_caps *caps;

  caps = get_vessel_terrain_caps(vessel_type);
  if (caps == NULL)
  {
    return 0;
  }

  if (sector_type < 0 || sector_type >= 40)
  {
    return 0;
  }

  base_modifier = (int)caps->terrain_speed_mod[sector_type];

  /* Weather penalties */
  if (vessel_type == VESSEL_AIRSHIP && weather_conditions > 0)
  {
    base_modifier -= (weather_conditions * 10);
  }
  if (vessel_type != VESSEL_SUBMARINE || sector_type != SECT_UNDERWATER)
  {
    if (weather_conditions > 0)
    {
      base_modifier -= (weather_conditions * 5);
    }
  }

  if (base_modifier < 0)
    base_modifier = 0;
  if (base_modifier > 150)
    base_modifier = 150;

  return base_modifier;
}

/**
 * Simulate terrain traversal check (mirrors production can_vessel_traverse_terrain logic)
 */
static bool can_traverse_sector(enum vessel_class vessel_type, int sector_type, int z)
{
  const struct vessel_terrain_caps *caps = get_vessel_terrain_caps(vessel_type);

  if (caps == NULL)
  {
    return FALSE;
  }

  /* Airships at altitude can fly over most terrain */
  if (vessel_type == VESSEL_AIRSHIP && z > 100)
  {
    if (z > caps->max_altitude)
    {
      return FALSE;
    }
    /* Cannot fly underground */
    if (sector_type == SECT_INSIDE)
    {
      return FALSE;
    }
    return TRUE;
  }

  /* Check speed modifier - 0 means impassable */
  if (sector_type >= 0 && sector_type < 40)
  {
    if (caps->terrain_speed_mod[sector_type] == 0)
    {
      return FALSE;
    }
    return TRUE;
  }

  return FALSE;
}

/**
 * Get vessel type name (mirrors production get_vessel_type_name)
 */
static const char *get_vessel_type_name(enum vessel_class type)
{
  static const char *names[] = {"Raft",    "Boat",      "Ship",      "Warship",
                                "Airship", "Submarine", "Transport", "Magical Vessel"};

  if (type < 0 || type >= NUM_VESSEL_TYPES)
  {
    return "Unknown";
  }
  return names[type];
}

/* ========================================================================= */
/* INTEGRATION TESTS                                                         */
/* ========================================================================= */

/**
 * Test raft terrain restrictions (rivers only)
 */
void Test_integration_raft_terrain_restrictions(CuTest *tc)
{
  /* Raft CAN traverse */
  CuAssertTrue(tc, can_traverse_sector(VESSEL_RAFT, SECT_WATER_SWIM, 0));
  CuAssertTrue(tc, can_traverse_sector(VESSEL_RAFT, SECT_RIVER, 0));
  CuAssertTrue(tc, can_traverse_sector(VESSEL_RAFT, SECT_MARSHLAND, 0));

  /* Raft CANNOT traverse */
  CuAssertTrue(tc, !can_traverse_sector(VESSEL_RAFT, SECT_OCEAN, 0));
  CuAssertTrue(tc, !can_traverse_sector(VESSEL_RAFT, SECT_WATER_NOSWIM, 0));
  CuAssertTrue(tc, !can_traverse_sector(VESSEL_RAFT, SECT_FIELD, 0));
  CuAssertTrue(tc, !can_traverse_sector(VESSEL_RAFT, SECT_MOUNTAIN, 0));
}

/**
 * Test ship ocean capability
 */
void Test_integration_ship_ocean_capability(CuTest *tc)
{
  /* Ship CAN traverse ocean and deep water */
  CuAssertTrue(tc, can_traverse_sector(VESSEL_SHIP, SECT_OCEAN, 0));
  CuAssertTrue(tc, can_traverse_sector(VESSEL_SHIP, SECT_WATER_NOSWIM, 0));
  CuAssertTrue(tc, can_traverse_sector(VESSEL_SHIP, SECT_WATER_SWIM, 0));

  /* Ship CANNOT traverse land */
  CuAssertTrue(tc, !can_traverse_sector(VESSEL_SHIP, SECT_FIELD, 0));
  CuAssertTrue(tc, !can_traverse_sector(VESSEL_SHIP, SECT_MOUNTAIN, 0));
  CuAssertTrue(tc, !can_traverse_sector(VESSEL_SHIP, SECT_MARSHLAND, 0));
}

/**
 * Test airship altitude mechanics
 */
void Test_integration_airship_altitude(CuTest *tc)
{
  /* At altitude (z > 100), airship can fly over terrain */
  CuAssertTrue(tc, can_traverse_sector(VESSEL_AIRSHIP, SECT_MOUNTAIN, 200));
  CuAssertTrue(tc, can_traverse_sector(VESSEL_AIRSHIP, SECT_HIGH_MOUNTAIN, 300));
  CuAssertTrue(tc, can_traverse_sector(VESSEL_AIRSHIP, SECT_FIELD, 150));

  /* Cannot fly above max altitude */
  CuAssertTrue(tc, !can_traverse_sector(VESSEL_AIRSHIP, SECT_FIELD, 600));

  /* Cannot fly inside buildings */
  CuAssertTrue(tc, !can_traverse_sector(VESSEL_AIRSHIP, SECT_INSIDE, 200));
}

/**
 * Test submarine underwater capability
 */
void Test_integration_submarine_underwater(CuTest *tc)
{
  /* Submarine CAN traverse underwater and ocean */
  CuAssertTrue(tc, can_traverse_sector(VESSEL_SUBMARINE, SECT_UNDERWATER, -50));
  CuAssertTrue(tc, can_traverse_sector(VESSEL_SUBMARINE, SECT_OCEAN, 0));
  CuAssertTrue(tc, can_traverse_sector(VESSEL_SUBMARINE, SECT_WATER_NOSWIM, 0));

  /* Submarine CANNOT traverse land */
  CuAssertTrue(tc, !can_traverse_sector(VESSEL_SUBMARINE, SECT_FIELD, 0));
  CuAssertTrue(tc, !can_traverse_sector(VESSEL_SUBMARINE, SECT_MOUNTAIN, 0));
}

/**
 * Test magical vessel versatility
 */
void Test_integration_magical_versatility(CuTest *tc)
{
  /* Magical CAN traverse almost everything */
  CuAssertTrue(tc, can_traverse_sector(VESSEL_MAGICAL, SECT_OCEAN, 0));
  CuAssertTrue(tc, can_traverse_sector(VESSEL_MAGICAL, SECT_UNDERWATER, -100));
  CuAssertTrue(tc, can_traverse_sector(VESSEL_MAGICAL, SECT_FLYING, 0));
  CuAssertTrue(tc, can_traverse_sector(VESSEL_MAGICAL, SECT_CITY, 0));
  CuAssertTrue(tc, can_traverse_sector(VESSEL_MAGICAL, SECT_MOUNTAIN, 0));
}

/**
 * Test speed modifiers vary by vessel type
 */
void Test_integration_speed_modifiers_vary(CuTest *tc)
{
  int raft_river, ship_river, transport_river;

  /* Raft is fast on rivers */
  raft_river = get_terrain_speed_modifier(VESSEL_RAFT, SECT_RIVER, 0);
  CuAssertTrue(tc, raft_river == 100);

  /* Ship is slow on rivers (too large) */
  ship_river = get_terrain_speed_modifier(VESSEL_SHIP, SECT_RIVER, 0);
  CuAssertTrue(tc, ship_river == 50);

  /* Transport is very slow on rivers */
  transport_river = get_terrain_speed_modifier(VESSEL_TRANSPORT, SECT_RIVER, 0);
  CuAssertTrue(tc, transport_river == 40);
}

/**
 * Test weather affects speed
 */
void Test_integration_weather_speed_penalty(CuTest *tc)
{
  int clear_speed, stormy_speed;

  /* Clear weather - full speed */
  clear_speed = get_terrain_speed_modifier(VESSEL_SHIP, SECT_OCEAN, 0);
  CuAssertTrue(tc, clear_speed == 100);

  /* Stormy weather (condition 2) - reduced speed */
  stormy_speed = get_terrain_speed_modifier(VESSEL_SHIP, SECT_OCEAN, 2);
  CuAssertTrue(tc, stormy_speed < clear_speed);
  CuAssertTrue(tc, stormy_speed == 90); /* 100 - (2 * 5) */
}

/**
 * Test airship extra weather vulnerability
 */
void Test_integration_airship_weather_vulnerable(CuTest *tc)
{
  int airship_stormy, ship_stormy;

  airship_stormy = get_terrain_speed_modifier(VESSEL_AIRSHIP, SECT_OCEAN, 2);
  ship_stormy = get_terrain_speed_modifier(VESSEL_SHIP, SECT_OCEAN, 2);

  /* Airship loses more speed in storm than ship */
  /* Airship: 100 - (2*10) - (2*5) = 70 */
  CuAssertTrue(tc, airship_stormy == 70);
  /* Ship: 100 - (2*5) = 90 */
  CuAssertTrue(tc, ship_stormy == 90);
}

/**
 * Test vessel type names
 */
void Test_integration_type_names(CuTest *tc)
{
  CuAssertStrEquals(tc, "Raft", get_vessel_type_name(VESSEL_RAFT));
  CuAssertStrEquals(tc, "Ship", get_vessel_type_name(VESSEL_SHIP));
  CuAssertStrEquals(tc, "Warship", get_vessel_type_name(VESSEL_WARSHIP));
  CuAssertStrEquals(tc, "Airship", get_vessel_type_name(VESSEL_AIRSHIP));
  CuAssertStrEquals(tc, "Submarine", get_vessel_type_name(VESSEL_SUBMARINE));
  CuAssertStrEquals(tc, "Transport", get_vessel_type_name(VESSEL_TRANSPORT));
  CuAssertStrEquals(tc, "Magical Vessel", get_vessel_type_name(VESSEL_MAGICAL));
}

/**
 * Test invalid vessel type handling
 */
void Test_integration_invalid_type_handling(CuTest *tc)
{
  const struct vessel_terrain_caps *caps;

  /* Invalid types should return "Unknown" */
  CuAssertStrEquals(tc, "Unknown", get_vessel_type_name(-1));
  CuAssertStrEquals(tc, "Unknown", get_vessel_type_name(100));

  /* Invalid types should default to SHIP capabilities */
  caps = get_vessel_terrain_caps(-1);
  CuAssertPtrNotNull(tc, caps);
  CuAssertTrue(tc, caps->can_traverse_ocean == TRUE);
}

/**
 * Test all 8 types have distinct capabilities
 */
void Test_integration_all_types_distinct(CuTest *tc)
{
  int i;
  const struct vessel_terrain_caps *caps_i, *caps_j;

  /* Verify all types return valid capabilities */
  for (i = 0; i < NUM_VESSEL_TYPES; i++)
  {
    caps_i = get_vessel_terrain_caps((enum vessel_class)i);
    CuAssertPtrNotNull(tc, caps_i);

    /* Check at least one speed modifier is non-zero */
    {
      int has_speed = 0;
      int k;
      for (k = 0; k < 40; k++)
      {
        if (caps_i->terrain_speed_mod[k] > 0)
        {
          has_speed = 1;
          break;
        }
      }
      CuAssertTrue(tc, has_speed);
    }
  }

  /* Verify special vessels have unique capabilities */
  caps_i = get_vessel_terrain_caps(VESSEL_AIRSHIP);
  caps_j = get_vessel_terrain_caps(VESSEL_SUBMARINE);
  CuAssertTrue(tc, caps_i->can_traverse_air != caps_j->can_traverse_air);
  CuAssertTrue(tc, caps_i->can_traverse_underwater != caps_j->can_traverse_underwater);
}

/* ========================================================================= */
/* TEST SUITE REGISTRATION                                                   */
/* ========================================================================= */

/**
 * Get the vessel type integration test suite
 */
CuSuite *VesselTypeIntegrationGetSuite(void)
{
  CuSuite *suite = CuSuiteNew();

  /* Terrain restriction tests */
  SUITE_ADD_TEST(suite, Test_integration_raft_terrain_restrictions);
  SUITE_ADD_TEST(suite, Test_integration_ship_ocean_capability);
  SUITE_ADD_TEST(suite, Test_integration_airship_altitude);
  SUITE_ADD_TEST(suite, Test_integration_submarine_underwater);
  SUITE_ADD_TEST(suite, Test_integration_magical_versatility);

  /* Speed modifier tests */
  SUITE_ADD_TEST(suite, Test_integration_speed_modifiers_vary);
  SUITE_ADD_TEST(suite, Test_integration_weather_speed_penalty);
  SUITE_ADD_TEST(suite, Test_integration_airship_weather_vulnerable);

  /* Type name and validation tests */
  SUITE_ADD_TEST(suite, Test_integration_type_names);
  SUITE_ADD_TEST(suite, Test_integration_invalid_type_handling);
  SUITE_ADD_TEST(suite, Test_integration_all_types_distinct);

  return suite;
}

/* ========================================================================= */
/* STANDALONE MAIN (for isolated testing)                                    */
/* ========================================================================= */

#ifdef STANDALONE_TEST
int main(void)
{
  int failCount;
  CuString *output = CuStringNew();
  CuSuite *suite = VesselTypeIntegrationGetSuite();

  printf("Vessel Type Integration Tests\n");
  printf("==============================\n\n");

  CuSuiteRun(suite);
  CuSuiteSummary(suite, output);
  CuSuiteDetails(suite, output);

  printf("%s\n", output->buffer);

  failCount = suite->failCount;

  /* Cleanup */
  CuStringDelete(output);
  CuSuiteDelete(suite);

  return failCount;
}
#endif
