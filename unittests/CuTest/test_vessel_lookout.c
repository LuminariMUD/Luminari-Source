#include "CuTest.h"

#include "../../src/conf.h"
#include "../../src/sysdep.h"
#include "../../src/structs.h"
#include "../../src/utils.h"
#include "../../src/interpreter.h"
#include "../../src/vessels.h"
#include "../../src/wilderness.h"

void Test_vessel_lookout_samples_near_mid_and_horizon(CuTest *tc)
{
  int distances[6];
  int count;

  memset(distances, 0, sizeof(distances));
  count = vessel_lookout_sample_distances(15, distances, 6);
  CuAssertIntEquals(tc, 6, count);
  CuAssertIntEquals(tc, 1, distances[0]);
  CuAssertIntEquals(tc, 3, distances[1]);
  CuAssertIntEquals(tc, 5, distances[2]);
  CuAssertIntEquals(tc, 8, distances[3]);
  CuAssertIntEquals(tc, 10, distances[4]);
  CuAssertIntEquals(tc, 15, distances[5]);

  memset(distances, 0, sizeof(distances));
  count = vessel_lookout_sample_distances(50, distances, 6);
  CuAssertIntEquals(tc, 6, count);
  CuAssertIntEquals(tc, 1, distances[0]);
  CuAssertIntEquals(tc, 3, distances[1]);
  CuAssertIntEquals(tc, 5, distances[2]);
  CuAssertIntEquals(tc, 10, distances[3]);
  CuAssertIntEquals(tc, 25, distances[4]);
  CuAssertIntEquals(tc, 50, distances[5]);
}

void Test_vessel_lookout_samples_handle_small_or_invalid_views(CuTest *tc)
{
  int distances[3];
  int count;

  memset(distances, 0, sizeof(distances));
  CuAssertIntEquals(tc, 0, vessel_lookout_sample_distances(0, distances, 3));
  CuAssertIntEquals(tc, 0, vessel_lookout_sample_distances(10, NULL, 3));
  CuAssertIntEquals(tc, 0, vessel_lookout_sample_distances(10, distances, 0));

  count = vessel_lookout_sample_distances(2, distances, 3);
  CuAssertIntEquals(tc, 2, count);
  CuAssertIntEquals(tc, 1, distances[0]);
  CuAssertIntEquals(tc, 2, distances[1]);
}

void Test_vessel_lookout_bands_compress_consecutive_terrain(CuTest *tc)
{
  const int sectors[] = {SECT_OCEAN,      SECT_OCEAN, SECT_WATER_SWIM,
                         SECT_WATER_SWIM, SECT_BEACH, SECT_FOREST};
  const int distances[] = {1, 3, 5, 10, 25, 50};
  struct vessel_lookout_band bands[6];
  int count;

  memset(bands, 0, sizeof(bands));
  count = vessel_lookout_build_bands(sectors, distances, 6, bands, 6);
  CuAssertIntEquals(tc, 4, count);
  CuAssertIntEquals(tc, SECT_OCEAN, bands[0].sector_type);
  CuAssertIntEquals(tc, 1, bands[0].first_distance);
  CuAssertIntEquals(tc, 3, bands[0].last_distance);
  CuAssertIntEquals(tc, SECT_WATER_SWIM, bands[1].sector_type);
  CuAssertIntEquals(tc, 5, bands[1].first_distance);
  CuAssertIntEquals(tc, 10, bands[1].last_distance);
  CuAssertIntEquals(tc, SECT_BEACH, bands[2].sector_type);
  CuAssertIntEquals(tc, SECT_FOREST, bands[3].sector_type);
  CuAssertIntEquals(tc, 50, bands[3].last_distance);
}

void Test_vessel_lookout_bands_respect_capacity_and_inputs(CuTest *tc)
{
  const int sectors[] = {SECT_OCEAN, SECT_BEACH, SECT_FOREST};
  const int distances[] = {1, 5, 10};
  struct vessel_lookout_band bands[2];

  memset(bands, 0, sizeof(bands));
  CuAssertIntEquals(tc, 2, vessel_lookout_build_bands(sectors, distances, 3, bands, 2));
  CuAssertIntEquals(tc, SECT_OCEAN, bands[0].sector_type);
  CuAssertIntEquals(tc, SECT_BEACH, bands[1].sector_type);
  CuAssertIntEquals(tc, 0, vessel_lookout_build_bands(NULL, distances, 3, bands, 2));
  CuAssertIntEquals(tc, 0, vessel_lookout_build_bands(sectors, NULL, 3, bands, 2));
  CuAssertIntEquals(tc, 0, vessel_lookout_build_bands(sectors, distances, 0, bands, 2));
}

void Test_vessel_lookout_compass_boundaries_and_normalization(CuTest *tc)
{
  CuAssertStrEquals(tc, "N", vessel_lookout_compass_direction(0));
  CuAssertStrEquals(tc, "N", vessel_lookout_compass_direction(22));
  CuAssertStrEquals(tc, "NE", vessel_lookout_compass_direction(23));
  CuAssertStrEquals(tc, "E", vessel_lookout_compass_direction(90));
  CuAssertStrEquals(tc, "S", vessel_lookout_compass_direction(180));
  CuAssertStrEquals(tc, "W", vessel_lookout_compass_direction(270));
  CuAssertStrEquals(tc, "NW", vessel_lookout_compass_direction(336));
  CuAssertStrEquals(tc, "N", vessel_lookout_compass_direction(360));
  CuAssertStrEquals(tc, "W", vessel_lookout_compass_direction(-90));
}
