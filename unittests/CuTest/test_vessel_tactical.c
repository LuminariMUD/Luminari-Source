#include "CuTest.h"

#include "../../src/conf.h"
#include "../../src/sysdep.h"
#include "../../src/structs.h"
#include "../../src/utils.h"
#include "../../src/interpreter.h"
#include "../../src/vessels/vessels.h"
#include "../../src/wilderness/wilderness.h"

void Test_vessel_tactical_water_classification(CuTest *tc)
{
  CuAssertTrue(tc, vessel_tactical_sector_is_water(SECT_OCEAN));
  CuAssertTrue(tc, vessel_tactical_sector_is_water(SECT_WATER_SWIM));
  CuAssertTrue(tc, vessel_tactical_sector_is_water(SECT_WATER_NOSWIM));
  CuAssertTrue(tc, vessel_tactical_sector_is_water(SECT_UNDERWATER));
  CuAssertTrue(tc, vessel_tactical_sector_is_water(SECT_RIVER));
  CuAssertTrue(tc, !vessel_tactical_sector_is_water(SECT_BEACH));
  CuAssertTrue(tc, !vessel_tactical_sector_is_water(SECT_FIELD));
}

void Test_vessel_tactical_terrain_symbols(CuTest *tc)
{
  CuAssertIntEquals(tc, '~', vessel_tactical_terrain_symbol(SECT_OCEAN, FALSE));
  CuAssertIntEquals(tc, '.', vessel_tactical_terrain_symbol(SECT_WATER_SWIM, FALSE));
  CuAssertIntEquals(tc, '=', vessel_tactical_terrain_symbol(SECT_RIVER, FALSE));
  CuAssertIntEquals(tc, 'u', vessel_tactical_terrain_symbol(SECT_UNDERWATER, FALSE));
  CuAssertIntEquals(tc, ':', vessel_tactical_terrain_symbol(SECT_BEACH, TRUE));
  CuAssertIntEquals(tc, 'D', vessel_tactical_terrain_symbol(SECT_SEAPORT, TRUE));
  CuAssertIntEquals(tc, '#', vessel_tactical_terrain_symbol(SECT_FIELD, TRUE));
  CuAssertIntEquals(tc, '^', vessel_tactical_terrain_symbol(SECT_FIELD, FALSE));
  CuAssertIntEquals(tc, '?', vessel_tactical_terrain_symbol(-1, FALSE));
}

void Test_vessel_tactical_range_rings(CuTest *tc)
{
  CuAssertIntEquals(tc, 0, vessel_tactical_range_ring(0, 0));
  CuAssertIntEquals(tc, 5, vessel_tactical_range_ring(3, 4));
  CuAssertIntEquals(tc, 5, vessel_tactical_range_ring(-5, 0));
  CuAssertIntEquals(tc, 0, vessel_tactical_range_ring(6, 0));
  CuAssertIntEquals(tc, 10, vessel_tactical_range_ring(6, 8));
  CuAssertIntEquals(tc, 10, vessel_tactical_range_ring(0, -10));
}

void Test_vessel_tactical_region_visibility_hides_encounters(CuTest *tc)
{
  CuAssertTrue(tc, vessel_tactical_region_type_visible(REGION_GEOGRAPHIC));
  CuAssertTrue(tc, vessel_tactical_region_type_visible(REGION_BATHYMETRIC));
  CuAssertTrue(tc, vessel_tactical_region_type_visible(REGION_ALTITUDE_LANE));
  CuAssertTrue(tc, vessel_tactical_region_type_visible(REGION_SKY_ISLAND));
  CuAssertTrue(tc, !vessel_tactical_region_type_visible(REGION_ENCOUNTER));
  CuAssertTrue(tc, !vessel_tactical_region_type_visible(REGION_SECTOR));
}

void Test_vessel_tactical_contact_damage_symbols(CuTest *tc)
{
  CuAssertIntEquals(tc, ' ', vessel_tactical_contact_symbol(VESSEL_STATUS_SOUND, 0));
  CuAssertIntEquals(tc, 'V', vessel_tactical_contact_symbol(VESSEL_STATUS_SOUND, 1));
  CuAssertIntEquals(tc, 'B', vessel_tactical_contact_symbol(VESSEL_STATUS_BATTERED, 1));
  CuAssertIntEquals(tc, 'C', vessel_tactical_contact_symbol(VESSEL_STATUS_CRIPPLED, 1));
  CuAssertIntEquals(tc, 'X', vessel_tactical_contact_symbol(VESSEL_STATUS_SINKING, 1));
  CuAssertIntEquals(tc, 'M', vessel_tactical_contact_symbol(VESSEL_STATUS_SINKING, 2));
}
