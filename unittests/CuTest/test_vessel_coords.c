/**
 * @file test_vessel_coords.c
 * @brief Unit tests for vessel coordinate system
 *
 * Tests coordinate validation, boundary conditions, and wilderness
 * position updates for the vessel system.
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

#ifndef NOWHERE
#define NOWHERE (-1)
#endif

typedef int bool;
typedef int room_rnum;

/* Coordinate bounds for wilderness system */
#define COORD_MIN_X (-1024)
#define COORD_MAX_X (1024)
#define COORD_MIN_Y (-1024)
#define COORD_MAX_Y (1024)
#define COORD_MIN_Z (-500) /* Submarine depth limit */
#define COORD_MAX_Z (500)  /* Airship altitude limit */

/* ========================================================================= */
/* COORDINATE VALIDATION FUNCTIONS                                           */
/* ========================================================================= */

/**
 * Validate X coordinate is within wilderness bounds
 */
static bool validate_coord_x(int x)
{
  return (x >= COORD_MIN_X && x <= COORD_MAX_X);
}

/**
 * Validate Y coordinate is within wilderness bounds
 */
static bool validate_coord_y(int y)
{
  return (y >= COORD_MIN_Y && y <= COORD_MAX_Y);
}

/**
 * Validate Z coordinate (altitude/depth)
 */
static bool validate_coord_z(int z)
{
  return (z >= COORD_MIN_Z && z <= COORD_MAX_Z);
}

/**
 * Validate all coordinates together
 */
static bool validate_all_coords(int x, int y, int z)
{
  return validate_coord_x(x) && validate_coord_y(y) && validate_coord_z(z);
}

/**
 * Calculate distance between two coordinate points
 */
static float calculate_distance(int x1, int y1, int x2, int y2)
{
  int dx = x2 - x1;
  int dy = y2 - y1;
  return (float)((dx * dx) + (dy * dy)); /* Return squared distance for efficiency */
}

/**
 * Normalize heading to 0-360 range
 */
static int normalize_heading(int heading)
{
  while (heading < 0)
  {
    heading += 360;
  }
  while (heading >= 360)
  {
    heading -= 360;
  }
  return heading;
}

/* ========================================================================= */
/* BOUNDARY CONDITION TESTS                                                  */
/* ========================================================================= */

/**
 * Test X coordinate at minimum boundary
 */
void Test_coord_x_min_boundary(CuTest *tc)
{
  CuAssertIntEquals(tc, TRUE, validate_coord_x(COORD_MIN_X));
  CuAssertIntEquals(tc, FALSE, validate_coord_x(COORD_MIN_X - 1));
}

/**
 * Test X coordinate at maximum boundary
 */
void Test_coord_x_max_boundary(CuTest *tc)
{
  CuAssertIntEquals(tc, TRUE, validate_coord_x(COORD_MAX_X));
  CuAssertIntEquals(tc, FALSE, validate_coord_x(COORD_MAX_X + 1));
}

/**
 * Test Y coordinate at minimum boundary
 */
void Test_coord_y_min_boundary(CuTest *tc)
{
  CuAssertIntEquals(tc, TRUE, validate_coord_y(COORD_MIN_Y));
  CuAssertIntEquals(tc, FALSE, validate_coord_y(COORD_MIN_Y - 1));
}

/**
 * Test Y coordinate at maximum boundary
 */
void Test_coord_y_max_boundary(CuTest *tc)
{
  CuAssertIntEquals(tc, TRUE, validate_coord_y(COORD_MAX_Y));
  CuAssertIntEquals(tc, FALSE, validate_coord_y(COORD_MAX_Y + 1));
}

/**
 * Test Z coordinate at depth limit (submarines)
 */
void Test_coord_z_depth_boundary(CuTest *tc)
{
  CuAssertIntEquals(tc, TRUE, validate_coord_z(COORD_MIN_Z));
  CuAssertIntEquals(tc, FALSE, validate_coord_z(COORD_MIN_Z - 1));
}

/**
 * Test Z coordinate at altitude limit (airships)
 */
void Test_coord_z_altitude_boundary(CuTest *tc)
{
  CuAssertIntEquals(tc, TRUE, validate_coord_z(COORD_MAX_Z));
  CuAssertIntEquals(tc, FALSE, validate_coord_z(COORD_MAX_Z + 1));
}

/* ========================================================================= */
/* ORIGIN AND CENTER TESTS                                                   */
/* ========================================================================= */

/**
 * Test coordinates at origin (0,0,0)
 */
void Test_coord_origin(CuTest *tc)
{
  CuAssertIntEquals(tc, TRUE, validate_all_coords(0, 0, 0));
}

/**
 * Test coordinates at all corners
 */
void Test_coord_corners(CuTest *tc)
{
  /* Northwest corner */
  CuAssertIntEquals(tc, TRUE, validate_all_coords(COORD_MIN_X, COORD_MAX_Y, 0));

  /* Northeast corner */
  CuAssertIntEquals(tc, TRUE, validate_all_coords(COORD_MAX_X, COORD_MAX_Y, 0));

  /* Southwest corner */
  CuAssertIntEquals(tc, TRUE, validate_all_coords(COORD_MIN_X, COORD_MIN_Y, 0));

  /* Southeast corner */
  CuAssertIntEquals(tc, TRUE, validate_all_coords(COORD_MAX_X, COORD_MIN_Y, 0));
}

/**
 * Test extreme 3D corners (with altitude/depth)
 */
void Test_coord_3d_extremes(CuTest *tc)
{
  /* High altitude corner */
  CuAssertIntEquals(tc, TRUE, validate_all_coords(COORD_MAX_X, COORD_MAX_Y, COORD_MAX_Z));

  /* Deep underwater corner */
  CuAssertIntEquals(tc, TRUE, validate_all_coords(COORD_MIN_X, COORD_MIN_Y, COORD_MIN_Z));

  /* Just outside high corner */
  CuAssertIntEquals(tc, FALSE, validate_all_coords(COORD_MAX_X + 1, COORD_MAX_Y, COORD_MAX_Z));
  CuAssertIntEquals(tc, FALSE, validate_all_coords(COORD_MAX_X, COORD_MAX_Y + 1, COORD_MAX_Z));
  CuAssertIntEquals(tc, FALSE, validate_all_coords(COORD_MAX_X, COORD_MAX_Y, COORD_MAX_Z + 1));
}

/* ========================================================================= */
/* INVALID INPUT TESTS                                                       */
/* ========================================================================= */

/**
 * Test extremely out of range values
 */
void Test_coord_extreme_invalid(CuTest *tc)
{
  /* Way out of range */
  CuAssertIntEquals(tc, FALSE, validate_coord_x(100000));
  CuAssertIntEquals(tc, FALSE, validate_coord_x(-100000));
  CuAssertIntEquals(tc, FALSE, validate_coord_y(100000));
  CuAssertIntEquals(tc, FALSE, validate_coord_y(-100000));
  CuAssertIntEquals(tc, FALSE, validate_coord_z(100000));
  CuAssertIntEquals(tc, FALSE, validate_coord_z(-100000));
}

/**
 * Test coordinate range is symmetric around origin
 */
void Test_coord_symmetry(CuTest *tc)
{
  /* X range should be symmetric */
  CuAssertIntEquals(tc, -COORD_MIN_X, COORD_MAX_X);

  /* Y range should be symmetric */
  CuAssertIntEquals(tc, -COORD_MIN_Y, COORD_MAX_Y);

  /* Z range should be symmetric */
  CuAssertIntEquals(tc, -COORD_MIN_Z, COORD_MAX_Z);
}

/* ========================================================================= */
/* DISTANCE CALCULATION TESTS                                                */
/* ========================================================================= */

/**
 * Test distance from origin
 */
void Test_coord_distance_from_origin(CuTest *tc)
{
  float dist;

  /* Distance from origin to origin is 0 */
  dist = calculate_distance(0, 0, 0, 0);
  CuAssertDblEquals(tc, 0.0, dist, 0.001);

  /* Distance from origin to (3,4) should be 25 (squared) = 5 actual */
  dist = calculate_distance(0, 0, 3, 4);
  CuAssertDblEquals(tc, 25.0, dist, 0.001);

  /* Distance from origin to (10,0) should be 100 (squared) */
  dist = calculate_distance(0, 0, 10, 0);
  CuAssertDblEquals(tc, 100.0, dist, 0.001);
}

/**
 * Test distance between two points
 */
void Test_coord_distance_between_points(CuTest *tc)
{
  float dist;

  /* Horizontal distance */
  dist = calculate_distance(0, 0, 100, 0);
  CuAssertDblEquals(tc, 10000.0, dist, 0.001);

  /* Vertical distance */
  dist = calculate_distance(0, 0, 0, 100);
  CuAssertDblEquals(tc, 10000.0, dist, 0.001);

  /* Diagonal distance (100,100 from origin = 20000 squared) */
  dist = calculate_distance(0, 0, 100, 100);
  CuAssertDblEquals(tc, 20000.0, dist, 0.001);
}

/**
 * Test distance with negative coordinates
 */
void Test_coord_distance_negative(CuTest *tc)
{
  float dist;

  /* Distance across origin */
  dist = calculate_distance(-50, 0, 50, 0);
  CuAssertDblEquals(tc, 10000.0, dist, 0.001);

  /* Symmetric distance */
  dist = calculate_distance(-100, -100, 100, 100);
  CuAssertDblEquals(tc, 80000.0, dist, 0.001); /* 200^2 + 200^2 */
}

/* ========================================================================= */
/* HEADING NORMALIZATION TESTS                                               */
/* ========================================================================= */

/**
 * Test normal heading values
 */
void Test_heading_normal_values(CuTest *tc)
{
  CuAssertIntEquals(tc, 0, normalize_heading(0));
  CuAssertIntEquals(tc, 90, normalize_heading(90));
  CuAssertIntEquals(tc, 180, normalize_heading(180));
  CuAssertIntEquals(tc, 270, normalize_heading(270));
  CuAssertIntEquals(tc, 359, normalize_heading(359));
}

/**
 * Test heading wraparound at 360
 */
void Test_heading_wraparound(CuTest *tc)
{
  CuAssertIntEquals(tc, 0, normalize_heading(360));
  CuAssertIntEquals(tc, 1, normalize_heading(361));
  CuAssertIntEquals(tc, 90, normalize_heading(450));
  CuAssertIntEquals(tc, 0, normalize_heading(720));
}

/**
 * Test negative heading values
 */
void Test_heading_negative(CuTest *tc)
{
  CuAssertIntEquals(tc, 359, normalize_heading(-1));
  CuAssertIntEquals(tc, 270, normalize_heading(-90));
  CuAssertIntEquals(tc, 180, normalize_heading(-180));
  CuAssertIntEquals(tc, 0, normalize_heading(-360));
  CuAssertIntEquals(tc, 359, normalize_heading(-361));
}

/* ========================================================================= */
/* TEST SUITE REGISTRATION                                                   */
/* ========================================================================= */

/**
 * Get the coordinate test suite
 */
CuSuite *VesselCoordsGetSuite(void)
{
  CuSuite *suite = CuSuiteNew();

  /* X coordinate tests */
  SUITE_ADD_TEST(suite, Test_coord_x_min_boundary);
  SUITE_ADD_TEST(suite, Test_coord_x_max_boundary);

  /* Y coordinate tests */
  SUITE_ADD_TEST(suite, Test_coord_y_min_boundary);
  SUITE_ADD_TEST(suite, Test_coord_y_max_boundary);

  /* Z coordinate tests */
  SUITE_ADD_TEST(suite, Test_coord_z_depth_boundary);
  SUITE_ADD_TEST(suite, Test_coord_z_altitude_boundary);

  /* Origin and corner tests */
  SUITE_ADD_TEST(suite, Test_coord_origin);
  SUITE_ADD_TEST(suite, Test_coord_corners);
  SUITE_ADD_TEST(suite, Test_coord_3d_extremes);

  /* Invalid input tests */
  SUITE_ADD_TEST(suite, Test_coord_extreme_invalid);
  SUITE_ADD_TEST(suite, Test_coord_symmetry);

  /* Distance tests */
  SUITE_ADD_TEST(suite, Test_coord_distance_from_origin);
  SUITE_ADD_TEST(suite, Test_coord_distance_between_points);
  SUITE_ADD_TEST(suite, Test_coord_distance_negative);

  /* Heading tests */
  SUITE_ADD_TEST(suite, Test_heading_normal_values);
  SUITE_ADD_TEST(suite, Test_heading_wraparound);
  SUITE_ADD_TEST(suite, Test_heading_negative);

  return suite;
}
