/* ************************************************************************
 *      File:   test_schedule.c                         Part of LuminariMUD  *
 *   Purpose:   Unit tests for vessel schedule system                        *
 *              Phase 01 Session 06: Scheduled Route System                  *
 * ********************************************************************** */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "CuTest.h"

/* ========================================================================= */
/* MOCK DEFINITIONS                                                          */
/* ========================================================================= */

/* Schedule constants (mirrors vessels.h) */
#define SCHEDULE_INTERVAL_MIN 1
#define SCHEDULE_INTERVAL_MAX 24
#define SCHEDULE_FLAG_ENABLED (1 << 0)
#define SCHEDULE_FLAG_PAUSED (1 << 1)

/* Mock time structure */
struct time_info_data
{
  int hours;
  int day;
  int month;
  int year;
};

/* Mock vessel schedule structure */
struct vessel_schedule
{
  int schedule_id;
  int ship_id;
  int route_id;
  int interval_hours;
  int next_departure;
  int flags;
};

/* Global mock time */
static struct time_info_data mock_time_info = {0, 0, 0, 0};

/* ========================================================================= */
/* MOCK SCHEDULE FUNCTIONS                                                   */
/* ========================================================================= */

/**
 * Mock schedule_calculate_next_departure.
 * Calculates next departure based on current time and interval.
 */
static void mock_schedule_calculate_next_departure(struct vessel_schedule *sched)
{
  int current_hour;

  if (sched == NULL)
  {
    return;
  }

  current_hour = mock_time_info.hours;
  sched->next_departure = current_hour + sched->interval_hours;

  /* Wrap around 24-hour day */
  if (sched->next_departure >= 24)
  {
    sched->next_departure = sched->next_departure % 24;
  }
}

/**
 * Mock schedule_is_enabled.
 */
static int mock_schedule_is_enabled(struct vessel_schedule *sched)
{
  if (sched == NULL)
  {
    return 0;
  }

  return (sched->flags & SCHEDULE_FLAG_ENABLED) ? 1 : 0;
}

/**
 * Mock schedule_check_trigger.
 * Checks if schedule should trigger based on current time.
 */
static int mock_schedule_check_trigger(struct vessel_schedule *sched)
{
  int current_hour;

  if (sched == NULL)
  {
    return 0;
  }

  if (!(sched->flags & SCHEDULE_FLAG_ENABLED))
  {
    return 0;
  }

  current_hour = mock_time_info.hours;

  /* Use >= comparison for timer precision */
  if (current_hour >= sched->next_departure)
  {
    return 1;
  }

  return 0;
}

/* ========================================================================= */
/* SCHEDULE CONSTANT TESTS                                                   */
/* ========================================================================= */

void test_schedule_interval_bounds(CuTest *tc)
{
  /* Test that interval bounds are sensible */
  CuAssertIntEquals(tc, 1, SCHEDULE_INTERVAL_MIN);
  CuAssertIntEquals(tc, 24, SCHEDULE_INTERVAL_MAX);
  CuAssertTrue(tc, SCHEDULE_INTERVAL_MIN < SCHEDULE_INTERVAL_MAX);
}

void test_schedule_flags_distinct(CuTest *tc)
{
  /* Test that flags are distinct bits */
  CuAssertIntEquals(tc, 1, SCHEDULE_FLAG_ENABLED);
  CuAssertIntEquals(tc, 2, SCHEDULE_FLAG_PAUSED);
  CuAssertTrue(tc, (SCHEDULE_FLAG_ENABLED & SCHEDULE_FLAG_PAUSED) == 0);
}

/* ========================================================================= */
/* NEXT DEPARTURE CALCULATION TESTS                                          */
/* ========================================================================= */

void test_next_departure_simple(CuTest *tc)
{
  struct vessel_schedule sched;

  memset(&sched, 0, sizeof(sched));
  sched.interval_hours = 2;

  mock_time_info.hours = 10;
  mock_schedule_calculate_next_departure(&sched);

  CuAssertIntEquals(tc, 12, sched.next_departure);
}

void test_next_departure_wraparound(CuTest *tc)
{
  struct vessel_schedule sched;

  memset(&sched, 0, sizeof(sched));
  sched.interval_hours = 6;

  mock_time_info.hours = 20;
  mock_schedule_calculate_next_departure(&sched);

  /* 20 + 6 = 26, should wrap to 2 */
  CuAssertIntEquals(tc, 2, sched.next_departure);
}

void test_next_departure_exact_midnight(CuTest *tc)
{
  struct vessel_schedule sched;

  memset(&sched, 0, sizeof(sched));
  sched.interval_hours = 4;

  mock_time_info.hours = 20;
  mock_schedule_calculate_next_departure(&sched);

  /* 20 + 4 = 24, should wrap to 0 */
  CuAssertIntEquals(tc, 0, sched.next_departure);
}

void test_next_departure_min_interval(CuTest *tc)
{
  struct vessel_schedule sched;

  memset(&sched, 0, sizeof(sched));
  sched.interval_hours = SCHEDULE_INTERVAL_MIN;

  mock_time_info.hours = 5;
  mock_schedule_calculate_next_departure(&sched);

  CuAssertIntEquals(tc, 6, sched.next_departure);
}

void test_next_departure_max_interval(CuTest *tc)
{
  struct vessel_schedule sched;

  memset(&sched, 0, sizeof(sched));
  sched.interval_hours = SCHEDULE_INTERVAL_MAX;

  mock_time_info.hours = 0;
  mock_schedule_calculate_next_departure(&sched);

  /* 0 + 24 = 24, wraps to 0 */
  CuAssertIntEquals(tc, 0, sched.next_departure);
}

void test_next_departure_null_schedule(CuTest *tc)
{
  /* Should not crash on NULL */
  mock_schedule_calculate_next_departure(NULL);
  CuAssertTrue(tc, 1); /* If we get here, test passed */
}

/* ========================================================================= */
/* SCHEDULE TRIGGER TESTS                                                    */
/* ========================================================================= */

void test_trigger_before_departure(CuTest *tc)
{
  struct vessel_schedule sched;

  memset(&sched, 0, sizeof(sched));
  sched.flags = SCHEDULE_FLAG_ENABLED;
  sched.next_departure = 14;

  mock_time_info.hours = 12;
  CuAssertIntEquals(tc, 0, mock_schedule_check_trigger(&sched));
}

void test_trigger_at_departure(CuTest *tc)
{
  struct vessel_schedule sched;

  memset(&sched, 0, sizeof(sched));
  sched.flags = SCHEDULE_FLAG_ENABLED;
  sched.next_departure = 14;

  mock_time_info.hours = 14;
  CuAssertIntEquals(tc, 1, mock_schedule_check_trigger(&sched));
}

void test_trigger_after_departure(CuTest *tc)
{
  struct vessel_schedule sched;

  memset(&sched, 0, sizeof(sched));
  sched.flags = SCHEDULE_FLAG_ENABLED;
  sched.next_departure = 14;

  mock_time_info.hours = 16;
  CuAssertIntEquals(tc, 1, mock_schedule_check_trigger(&sched));
}

void test_trigger_disabled_schedule(CuTest *tc)
{
  struct vessel_schedule sched;

  memset(&sched, 0, sizeof(sched));
  sched.flags = 0; /* Disabled */
  sched.next_departure = 14;

  mock_time_info.hours = 14;
  CuAssertIntEquals(tc, 0, mock_schedule_check_trigger(&sched));
}

void test_trigger_paused_schedule(CuTest *tc)
{
  struct vessel_schedule sched;

  memset(&sched, 0, sizeof(sched));
  sched.flags = SCHEDULE_FLAG_PAUSED; /* Paused only, not enabled */
  sched.next_departure = 14;

  mock_time_info.hours = 14;
  CuAssertIntEquals(tc, 0, mock_schedule_check_trigger(&sched));
}

void test_trigger_null_schedule(CuTest *tc)
{
  CuAssertIntEquals(tc, 0, mock_schedule_check_trigger(NULL));
}

/* ========================================================================= */
/* SCHEDULE ENABLED TESTS                                                    */
/* ========================================================================= */

void test_is_enabled_true(CuTest *tc)
{
  struct vessel_schedule sched;

  memset(&sched, 0, sizeof(sched));
  sched.flags = SCHEDULE_FLAG_ENABLED;

  CuAssertIntEquals(tc, 1, mock_schedule_is_enabled(&sched));
}

void test_is_enabled_false(CuTest *tc)
{
  struct vessel_schedule sched;

  memset(&sched, 0, sizeof(sched));
  sched.flags = 0;

  CuAssertIntEquals(tc, 0, mock_schedule_is_enabled(&sched));
}

void test_is_enabled_null(CuTest *tc)
{
  CuAssertIntEquals(tc, 0, mock_schedule_is_enabled(NULL));
}

/* ========================================================================= */
/* TEST SUITE SETUP                                                          */
/* ========================================================================= */

CuSuite *GetScheduleSuite(void)
{
  CuSuite *suite = CuSuiteNew();

  /* Constant tests */
  SUITE_ADD_TEST(suite, test_schedule_interval_bounds);
  SUITE_ADD_TEST(suite, test_schedule_flags_distinct);

  /* Next departure calculation tests */
  SUITE_ADD_TEST(suite, test_next_departure_simple);
  SUITE_ADD_TEST(suite, test_next_departure_wraparound);
  SUITE_ADD_TEST(suite, test_next_departure_exact_midnight);
  SUITE_ADD_TEST(suite, test_next_departure_min_interval);
  SUITE_ADD_TEST(suite, test_next_departure_max_interval);
  SUITE_ADD_TEST(suite, test_next_departure_null_schedule);

  /* Trigger tests */
  SUITE_ADD_TEST(suite, test_trigger_before_departure);
  SUITE_ADD_TEST(suite, test_trigger_at_departure);
  SUITE_ADD_TEST(suite, test_trigger_after_departure);
  SUITE_ADD_TEST(suite, test_trigger_disabled_schedule);
  SUITE_ADD_TEST(suite, test_trigger_paused_schedule);
  SUITE_ADD_TEST(suite, test_trigger_null_schedule);

  /* Enabled tests */
  SUITE_ADD_TEST(suite, test_is_enabled_true);
  SUITE_ADD_TEST(suite, test_is_enabled_false);
  SUITE_ADD_TEST(suite, test_is_enabled_null);

  return suite;
}

/* ========================================================================= */
/* MAIN ENTRY POINT                                                          */
/* ========================================================================= */

int main(void)
{
  CuString *output = CuStringNew();
  CuSuite *suite = GetScheduleSuite();

  printf("Running schedule system unit tests...\n\n");

  CuSuiteRun(suite);
  CuSuiteSummary(suite, output);
  CuSuiteDetails(suite, output);

  printf("%s\n", output->buffer);

  /* Return number of failures for exit code */
  return suite->failCount;
}
