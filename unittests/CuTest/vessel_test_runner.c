/**
 * @file vessel_test_runner.c
 * @brief Main test runner for vessel unit tests
 *
 * Aggregates all vessel test suites and runs them together.
 *
 * Part of Phase 00, Session 09: Testing and Validation
 */

#include <stdio.h>
#include <stdlib.h>
#include "CuTest.h"

/* ========================================================================= */
/* EXTERNAL TEST SUITE DECLARATIONS                                          */
/* ========================================================================= */

/* From test_vessels.c */
extern CuSuite *VesselGetSuite(void);

/* From test_vessel_coords.c */
extern CuSuite *VesselCoordsGetSuite(void);

/* From test_vessel_types.c */
extern CuSuite *VesselTypesGetSuite(void);

/* From test_vessel_rooms.c */
extern CuSuite *VesselRoomsGetSuite(void);

/* From test_vessel_movement.c */
extern CuSuite *VesselMovementGetSuite(void);

/* From test_vessel_persistence.c */
extern CuSuite *VesselPersistenceGetSuite(void);

/* ========================================================================= */
/* TEST RUNNER                                                               */
/* ========================================================================= */

/**
 * Run all vessel system unit tests
 */
void RunAllVesselTests(void)
{
  CuString *output = CuStringNew();
  CuSuite *suite = CuSuiteNew();

  /* Track sub-suites for proper cleanup */
  CuSuite *subSuites[6];
  int i;

  printf("========================================\n");
  printf("LuminariMUD Vessel System Unit Tests\n");
  printf("Phase 00, Session 09: Testing & Validation\n");
  printf("========================================\n\n");

  /* Add all test suites - keep references for cleanup */
  printf("Loading test suites...\n");

  printf("  - Main vessel tests\n");
  subSuites[0] = VesselGetSuite();
  CuSuiteAddSuite(suite, subSuites[0]);

  printf("  - Coordinate system tests\n");
  subSuites[1] = VesselCoordsGetSuite();
  CuSuiteAddSuite(suite, subSuites[1]);

  printf("  - Vessel type tests\n");
  subSuites[2] = VesselTypesGetSuite();
  CuSuiteAddSuite(suite, subSuites[2]);

  printf("  - Room generation tests\n");
  subSuites[3] = VesselRoomsGetSuite();
  CuSuiteAddSuite(suite, subSuites[3]);

  printf("  - Movement system tests\n");
  subSuites[4] = VesselMovementGetSuite();
  CuSuiteAddSuite(suite, subSuites[4]);

  printf("  - Persistence tests\n");
  subSuites[5] = VesselPersistenceGetSuite();
  CuSuiteAddSuite(suite, subSuites[5]);

  printf("\nRunning %d tests...\n\n", suite->count);

  /* Run all tests */
  CuSuiteRun(suite);

  /* Output results */
  CuSuiteSummary(suite, output);
  CuSuiteDetails(suite, output);

  printf("%s\n", output->buffer);

  /* Print summary */
  printf("========================================\n");
  printf("Test Results Summary:\n");
  printf("  Total tests: %d\n", suite->count);
  printf("  Passed:      %d\n", suite->count - suite->failCount);
  printf("  Failed:      %d\n", suite->failCount);
  printf("========================================\n");

  if (suite->failCount > 0)
  {
    printf("\n*** SOME TESTS FAILED ***\n");
  }
  else
  {
    printf("\n*** ALL TESTS PASSED ***\n");
  }

  /* Cleanup - CuSuiteAddSuite copies test pointers to parent suite,
   * so sub-suites should just be freed without deleting tests (parent owns them).
   * We free the sub-suite structs directly, then let parent delete all tests. */
  for (i = 0; i < 6; i++)
  {
    /* Just free the suite struct - don't call CuSuiteDelete which would
     * try to delete tests that are now owned by the parent suite */
    free(subSuites[i]);
  }
  CuStringDelete(output);
  CuSuiteDelete(suite);
}

/**
 * Main entry point
 */
int main(void)
{
  RunAllVesselTests();
  return 0;
}
