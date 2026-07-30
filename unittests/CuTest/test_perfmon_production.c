#include "CuTest.h"

#include "../../src/perfmon.h"

#include <stdint.h>
#include <string.h>

void Test_perfmon_percentiles_use_linear_interpolation(CuTest *tc)
{
  const uint64_t samples[] = {40, 10, 30, 20};

  CuAssertDblEquals(tc, 10.0, PERF_calculate_percentile(samples, 4, 0.0), 0.001);
  CuAssertDblEquals(tc, 25.0, PERF_calculate_percentile(samples, 4, 50.0), 0.001);
  CuAssertDblEquals(tc, 38.5, PERF_calculate_percentile(samples, 4, 95.0), 0.001);
  CuAssertDblEquals(tc, 39.7, PERF_calculate_percentile(samples, 4, 99.0), 0.001);
  CuAssertDblEquals(tc, 40.0, PERF_calculate_percentile(samples, 4, 100.0), 0.001);
}

void Test_perfmon_percentiles_reject_invalid_input(CuTest *tc)
{
  const uint64_t sample = 10;

  CuAssertDblEquals(tc, 0.0, PERF_calculate_percentile(NULL, 1, 50.0), 0.001);
  CuAssertDblEquals(tc, 0.0, PERF_calculate_percentile(&sample, 0, 50.0), 0.001);
  CuAssertDblEquals(tc, 0.0, PERF_calculate_percentile(&sample, 1, -1.0), 0.001);
  CuAssertDblEquals(tc, 0.0, PERF_calculate_percentile(&sample, 1, 101.0), 0.001);
}

void Test_perfmon_hour_rollup_promotes_once_with_source_maximum(CuTest *tc)
{
  char report[4096];
  char hour_line[512];
  char *hour_start;
  char *hour_end;
  size_t hour_length;
  unsigned long pulse_count;
  unsigned long i;

  PERF_reset();
  pulse_count = (unsigned long)PERF_pulse_per_second * 60UL * 60UL;
  for (i = 0; i < pulse_count; i++)
  {
    PERF_log_pulse(i == pulse_count / 2 ? 321.0 : 1.0);
  }

  PERF_repr(report, sizeof(report));
  hour_start = strstr(report, "  1 Hours:");
  CuAssertPtrNotNull(tc, hour_start);
  hour_end = strchr(hour_start, '\n');
  CuAssertPtrNotNull(tc, hour_end);
  hour_length = (size_t)(hour_end - hour_start);
  CuAssertTrue(tc, hour_length < sizeof(hour_line));
  memcpy(hour_line, hour_start, hour_length);
  hour_line[hour_length] = '\0';
  CuAssertPtrNotNull(tc, strstr(hour_line, "321.00%"));

  PERF_log_pulse(1.0);
  PERF_repr(report, sizeof(report));
  CuAssertPtrNotNull(tc, strstr(report, "  1 Hours:"));
  CuAssertPtrEquals(tc, NULL, strstr(report, "  2 Hours:"));
}

void Test_perfmon_csv_reports_and_resets_section_samples(CuTest *tc)
{
  static struct PERF_prof_sect *section = NULL;
  char report[4096];
  volatile uint64_t accumulator;
  uint64_t i;

  PERF_reset();
  PERF_prof_sect_init(&section, "perfmon_test_section");
  PERF_prof_sect_enable_sampling(section);
  PERF_prof_sect_enter(section);
  accumulator = 0;
  for (i = 0; i < 10000; i++)
  {
    accumulator += i;
  }
  PERF_prof_sect_exit(section);
  CuAssertTrue(tc, accumulator > 0);

  PERF_prof_repr_csv(report, sizeof(report));
  CuAssertPtrNotNull(tc, strstr(report, "section,calls,total_usec"));
  CuAssertPtrNotNull(tc, strstr(report, "perfmon_test_section,1,"));

  PERF_reset();
  PERF_prof_repr_csv(report, sizeof(report));
  CuAssertPtrEquals(tc, NULL, strstr(report, "perfmon_test_section,"));
}

void Test_perfmon_reports_saturate_tiny_buffers(CuTest *tc)
{
  char report[8];
  size_t written;

  memset(report, 'x', sizeof(report));
  written = PERF_prof_repr_csv(report, sizeof(report));

  CuAssertIntEquals(tc, (int)sizeof(report) - 1, (int)written);
  CuAssertIntEquals(tc, '\0', report[sizeof(report) - 1]);
}

void Test_perfmon_ignores_stale_and_duplicate_section_exits(CuTest *tc)
{
  static struct PERF_prof_sect *section = NULL;
  char report[4096];

  PERF_reset();
  PERF_prof_sect_init(&section, "perfmon_exit_guard");
  PERF_prof_sect_enable_sampling(section);
  PERF_prof_sect_enter(section);
  PERF_prof_sect_exit(section);
  PERF_prof_sect_exit(section);
  PERF_prof_repr_csv(report, sizeof(report));
  CuAssertPtrNotNull(tc, strstr(report, "perfmon_exit_guard,1,"));
  CuAssertPtrEquals(tc, NULL, strstr(report, "perfmon_exit_guard,2,"));

  PERF_prof_sect_enter(section);
  PERF_reset();
  PERF_prof_sect_exit(section);
  PERF_prof_repr_csv(report, sizeof(report));
  CuAssertPtrEquals(tc, NULL, strstr(report, "perfmon_exit_guard,"));
}

void Test_perfmon_csv_reports_and_resets_event_counters(CuTest *tc)
{
  char report[4096];

  PERF_reset();
  PERF_note_missed_pulses(3);
  PERF_note_vessel_message_throttled();
  PERF_note_vessel_message_throttled();

  CuAssertTrue(tc, PERF_missed_pulse_count() == 3);
  CuAssertTrue(tc, PERF_vessel_message_throttled_count() == 2);
  PERF_prof_repr_csv(report, sizeof(report));
  CuAssertPtrNotNull(tc, strstr(report, "# missed_pulses=3"));
  CuAssertPtrNotNull(tc, strstr(report, "# vessel_messages_throttled=2"));

  PERF_reset();
  CuAssertTrue(tc, PERF_missed_pulse_count() == 0);
  CuAssertTrue(tc, PERF_vessel_message_throttled_count() == 0);
  PERF_prof_repr_csv(report, sizeof(report));
  CuAssertPtrNotNull(tc, strstr(report, "# missed_pulses=0"));
  CuAssertPtrNotNull(tc, strstr(report, "# vessel_messages_throttled=0"));
}
