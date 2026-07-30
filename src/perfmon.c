/**
 * @file perfmon.c
 * @brief Simple Performance Monitoring System Implementation
 *
 * This provides a lightweight performance monitoring system using only C.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <float.h>
#include <inttypes.h>
#include <math.h>
#include <stdint.h>
#include "perfmon.h"

/* ========================================================================
 * CONSTANTS
 * ======================================================================== */

#define MAX_PROF_SECTIONS 2000     /* Maximum number of profiling sections */
#define USEC_PER_SEC 1000000       /* Microseconds per second */
#define PROF_SAMPLE_CAPACITY 16384 /* Per-section rolling percentile window */

/* Time hierarchy constants */
#define PULSE_PER_SECOND (PERF_pulse_per_second)
#define SEC_PER_MIN 60
#define MIN_PER_HOUR 60
#define HOUR_PER_DAY 24

/* Buffer sizes for time intervals */
#define PULSE_BUFFER_SIZE PULSE_PER_SECOND
#define SEC_BUFFER_SIZE SEC_PER_MIN
#define MIN_BUFFER_SIZE MIN_PER_HOUR
#define HOUR_BUFFER_SIZE HOUR_PER_DAY

/* Microseconds per pulse */
#define USEC_PER_PULSE (USEC_PER_SEC / PULSE_PER_SECOND)

/* ========================================================================
 * DATA STRUCTURES
 * ======================================================================== */

/* Circular buffer for storing performance data */
struct perf_interval
{
  double *avg_data;
  double *min_data;
  double *max_data;
  size_t size;
  size_t index;
  size_t count;
};

/* Performance threshold tracking */
static struct
{
  int threshold;
  unsigned long count;
} thresholds[] = {{10, 0},  {30, 0},  {50, 0},  {70, 0},   {90, 0},
                  {100, 0}, {250, 0}, {500, 0}, {1000, 0}, {2500, 0}};

/* Profiling section structure */
struct PERF_prof_sect
{
  char id[64];
  int active;
  int sampling_enabled;
  uint64_t last_enter_usec;
  uint64_t pulse_total_usec;
  uint64_t pulse_max_usec;
  uint64_t total_usec;
  uint64_t max_usec;
  uint64_t pulse_enter_count;
  uint64_t pulse_exit_count;
  uint64_t total_enter_count;
  uint64_t total_exit_count;
  uint64_t *samples;
  size_t sample_index;
  size_t sample_count;
  uint64_t samples_seen;
};

/* ========================================================================
 * GLOBAL STATE
 * ======================================================================== */

/* Initialization tracking */
static int initialized = 0;
static uint64_t prof_reset_usec;
static uint64_t logged_pulse_count;
static uint64_t missed_pulse_count;
static uint64_t vessel_message_throttled_count;

/* Pulse performance tracking */
static double last_pulse = 0.0;
static double max_pulse = 0.0;

/* Performance data buffers */
static struct perf_interval pulse_data;
static struct perf_interval sec_data;
static struct perf_interval min_data;
static struct perf_interval hour_data;

/* Profiling sections */
static struct PERF_prof_sect *prof_sections[MAX_PROF_SECTIONS];
static int prof_section_count = 0;

/* ========================================================================
 * UTILITY FUNCTIONS
 * ======================================================================== */

/* Return monotonic elapsed time in microseconds. */
static uint64_t monotonic_usec(void)
{
  struct timespec now;

  if (clock_gettime(CLOCK_MONOTONIC, &now) != 0)
  {
    return 0;
  }

  return ((uint64_t)now.tv_sec * USEC_PER_SEC) + ((uint64_t)now.tv_nsec / 1000);
}

/* Convert snprintf()'s attempted length to the length actually stored. */
static size_t bounded_format_length(int result, size_t capacity)
{
  if (result < 0 || capacity == 0)
  {
    return 0;
  }
  if ((size_t)result >= capacity)
  {
    return capacity - 1;
  }

  return (size_t)result;
}

/* Initialize a performance interval buffer */
static void init_interval(struct perf_interval *interval, size_t size)
{
  interval->avg_data = calloc(size, sizeof(double));
  interval->min_data = calloc(size, sizeof(double));
  interval->max_data = calloc(size, sizeof(double));
  interval->size = size;
  interval->index = 0;
  interval->count = 0;
}

/* Clear an initialized performance interval buffer. */
static void reset_interval(struct perf_interval *interval)
{
  if (interval->avg_data == NULL)
  {
    return;
  }

  memset(interval->avg_data, 0, interval->size * sizeof(double));
  memset(interval->min_data, 0, interval->size * sizeof(double));
  memset(interval->max_data, 0, interval->size * sizeof(double));
  interval->index = 0;
  interval->count = 0;
}

/* Add data to an interval buffer */
static void add_interval_data(struct perf_interval *interval, double avg, double min, double max)
{
  if (!interval->avg_data)
    return;

  interval->avg_data[interval->index] = avg;
  interval->min_data[interval->index] = min;
  interval->max_data[interval->index] = max;

  if (interval->count <= interval->index)
  {
    interval->count = interval->index + 1;
  }

  interval->index++;
  if (interval->index >= interval->size)
  {
    interval->index = 0;
  }
}

/* Get average of averages from interval */
static double get_interval_avg(const struct perf_interval *interval)
{
  double sum = 0.0;
  size_t i;

  if (interval->count == 0)
    return 0.0;

  for (i = 0; i < interval->count; i++)
  {
    sum += interval->avg_data[i];
  }

  return sum / interval->count;
}

/* Get minimum of minimums from interval */
static double get_interval_min(const struct perf_interval *interval)
{
  double min = DBL_MAX;
  size_t i;

  if (interval->count == 0)
    return 0.0;

  for (i = 0; i < interval->count; i++)
  {
    if (interval->min_data[i] < min)
    {
      min = interval->min_data[i];
    }
  }

  return min;
}

/* Get maximum of maximums from interval */
static double get_interval_max(const struct perf_interval *interval)
{
  double max = 0.0;
  size_t i;

  if (interval->count == 0)
    return 0.0;

  for (i = 0; i < interval->count; i++)
  {
    if (interval->max_data[i] > max)
    {
      max = interval->max_data[i];
    }
  }

  return max;
}

/* Compare unsigned microsecond samples for qsort(). */
static int compare_samples(const void *left, const void *right)
{
  const uint64_t *left_sample;
  const uint64_t *right_sample;

  left_sample = left;
  right_sample = right;
  if (*left_sample < *right_sample)
  {
    return -1;
  }
  if (*left_sample > *right_sample)
  {
    return 1;
  }
  return 0;
}

/* Calculate one percentile from an already sorted sample array. */
static double percentile_from_sorted(const uint64_t *samples, size_t count, double percentile)
{
  double rank;
  double fraction;
  double result;
  size_t lower;
  size_t upper;

  rank = (percentile / 100.0) * (double)(count - 1);
  lower = (size_t)floor(rank);
  upper = (size_t)ceil(rank);
  fraction = rank - (double)lower;
  result = (double)samples[lower];
  if (upper != lower)
  {
    result += ((double)samples[upper] - (double)samples[lower]) * fraction;
  }

  return result;
}

double PERF_calculate_percentile(const uint64_t *samples, size_t count, double percentile)
{
  uint64_t *sorted;
  double result;

  if (samples == NULL || count == 0 || !isfinite(percentile) || percentile < 0.0 ||
      percentile > 100.0)
  {
    return 0.0;
  }

  sorted = malloc(count * sizeof(*sorted));
  if (sorted == NULL)
  {
    return 0.0;
  }
  memcpy(sorted, samples, count * sizeof(*sorted));
  qsort(sorted, count, sizeof(*sorted), compare_samples);
  result = percentile_from_sorted(sorted, count, percentile);
  free(sorted);
  return result;
}

/* Calculate the benchmark percentile set with one allocation and sort. */
static void calculate_percentile_set(const uint64_t *samples, size_t count, double *median,
                                     double *p95, double *p99)
{
  uint64_t *sorted;

  *median = 0.0;
  *p95 = 0.0;
  *p99 = 0.0;
  if (samples == NULL || count == 0)
  {
    return;
  }

  sorted = malloc(count * sizeof(*sorted));
  if (sorted == NULL)
  {
    return;
  }
  memcpy(sorted, samples, count * sizeof(*sorted));
  qsort(sorted, count, sizeof(*sorted), compare_samples);
  *median = percentile_from_sorted(sorted, count, 50.0);
  *p95 = percentile_from_sorted(sorted, count, 95.0);
  *p99 = percentile_from_sorted(sorted, count, 99.0);
  free(sorted);
}

/* Initialize the performance monitoring system */
static void ensure_initialized(void)
{
  if (initialized)
    return;

  /* Initialize interval buffers */
  init_interval(&pulse_data, PULSE_BUFFER_SIZE);
  init_interval(&sec_data, SEC_BUFFER_SIZE);
  init_interval(&min_data, MIN_BUFFER_SIZE);
  init_interval(&hour_data, HOUR_BUFFER_SIZE);
  prof_reset_usec = monotonic_usec();

  initialized = 1;
}

/* Update threshold counters */
static void check_thresholds(double val)
{
  size_t i;
  size_t count = sizeof(thresholds) / sizeof(thresholds[0]);

  for (i = 0; i < count; i++)
  {
    if (val > thresholds[i].threshold)
    {
      thresholds[i].count++;
    }
    else
    {
      break; /* Thresholds are in ascending order */
    }
  }
}

/* Aggregate data from one level to the next */
static void aggregate_data(void)
{
  /* When pulse buffer wraps, aggregate to seconds */
  if (pulse_data.index == 0 && pulse_data.count == pulse_data.size)
  {
    add_interval_data(&sec_data, get_interval_avg(&pulse_data), get_interval_min(&pulse_data),
                      get_interval_max(&pulse_data));

    /* Promote once for each completed lower-level buffer. Keeping these
     * checks nested prevents the same completed buffer from being promoted
     * again on every pulse before the next lower-level sample arrives. */
    if (sec_data.index == 0 && sec_data.count == sec_data.size)
    {
      add_interval_data(&min_data, get_interval_avg(&sec_data), get_interval_min(&sec_data),
                        get_interval_max(&sec_data));

      if (min_data.index == 0 && min_data.count == min_data.size)
      {
        add_interval_data(&hour_data, get_interval_avg(&min_data), get_interval_min(&min_data),
                          get_interval_max(&min_data));
      }
    }
  }
}

void PERF_reset(void)
{
  struct PERF_prof_sect *sect;
  size_t threshold_count;
  int i;

  ensure_initialized();

  reset_interval(&pulse_data);
  reset_interval(&sec_data);
  reset_interval(&min_data);
  reset_interval(&hour_data);
  last_pulse = 0.0;
  max_pulse = 0.0;
  logged_pulse_count = 0;
  missed_pulse_count = 0;
  vessel_message_throttled_count = 0;

  threshold_count = sizeof(thresholds) / sizeof(thresholds[0]);
  for (i = 0; (size_t)i < threshold_count; i++)
  {
    thresholds[i].count = 0;
  }

  for (i = 0; i < prof_section_count; i++)
  {
    sect = prof_sections[i];
    sect->active = 0;
    sect->last_enter_usec = 0;
    sect->pulse_total_usec = 0;
    sect->pulse_max_usec = 0;
    sect->total_usec = 0;
    sect->max_usec = 0;
    sect->pulse_enter_count = 0;
    sect->pulse_exit_count = 0;
    sect->total_enter_count = 0;
    sect->total_exit_count = 0;
    sect->sample_index = 0;
    sect->sample_count = 0;
    sect->samples_seen = 0;
  }

  prof_reset_usec = monotonic_usec();
}

/* ========================================================================
 * PULSE MONITORING FUNCTIONS
 * ======================================================================== */

void PERF_log_pulse(double val)
{
  ensure_initialized();

  last_pulse = val;
  logged_pulse_count++;

  if (val > max_pulse)
  {
    max_pulse = val;
  }

  check_thresholds(val);

  /* Add to pulse data buffer */
  add_interval_data(&pulse_data, val, val, val);

  /* Check for aggregation */
  aggregate_data();
}

void PERF_note_missed_pulses(uint64_t count)
{
  if (UINT64_MAX - missed_pulse_count < count)
  {
    missed_pulse_count = UINT64_MAX;
    return;
  }

  missed_pulse_count += count;
}

void PERF_note_vessel_message_throttled(void)
{
  if (vessel_message_throttled_count < UINT64_MAX)
  {
    vessel_message_throttled_count++;
  }
}

uint64_t PERF_missed_pulse_count(void)
{
  return missed_pulse_count;
}

uint64_t PERF_vessel_message_throttled_count(void)
{
  return vessel_message_throttled_count;
}

size_t PERF_repr(char *out_buf, size_t n)
{
  size_t written = 0;
  size_t i;
  double total_pulses;
  double pulse_min, sec_min, min_min, hour_min;

  if (!out_buf || n < 1)
    return 0;

  ensure_initialized();

  total_pulses = (double)logged_pulse_count;

  /* Get minimum values */
  pulse_min = get_interval_min(&pulse_data);
  sec_min = get_interval_min(&sec_data);
  min_min = get_interval_min(&min_data);
  hour_min = get_interval_min(&hour_data);

  /* Format the report */
  written =
      snprintf(out_buf, n,
               "                     Avg         Min         Max\n\r"
               "  1 Pulse:   %10.2f%% %10.2f%% %10.2f%%\n\r"
               "%3zu Pulses:  %10.2f%% %10.2f%% %10.2f%%\n\r"
               "%3zu Seconds: %10.2f%% %10.2f%% %10.2f%%\n\r"
               "%3zu Minutes: %10.2f%% %10.2f%% %10.2f%%\n\r"
               "%3zu Hours:   %10.2f%% %10.2f%% %10.2f%%\n\r"
               "\n\rMax pulse:      %.2f\n\r\n\r",
               last_pulse, last_pulse, last_pulse, pulse_data.count, get_interval_avg(&pulse_data),
               pulse_min, get_interval_max(&pulse_data), sec_data.count,
               get_interval_avg(&sec_data), sec_min, get_interval_max(&sec_data), min_data.count,
               get_interval_avg(&min_data), min_min, get_interval_max(&min_data), hour_data.count,
               get_interval_avg(&hour_data), hour_min, get_interval_max(&hour_data), max_pulse);

  /* Add threshold statistics */
  for (i = 0; (size_t)i < sizeof(thresholds) / sizeof(thresholds[0]) && written < n - 1; i++)
  {
    double percent = (total_pulses > 0) ? (100.0 * thresholds[i].count / total_pulses) : 0.0;

    written += snprintf(out_buf + written, n - written, "Over %5d%%:      %.2f%% (%lu)\n\r",
                        thresholds[i].threshold, percent, thresholds[i].count);
  }

  return written;
}

/* ========================================================================
 * CODE PROFILING FUNCTIONS
 * ======================================================================== */

void PERF_prof_sect_init(struct PERF_prof_sect **ptr, const char *id)
{
  struct PERF_prof_sect *sect;
  int i;

  if (!ptr || !id)
    return;

  ensure_initialized();

  /* If already initialized, return */
  if (*ptr)
    return;

  /* Look for existing section */
  for (i = 0; i < prof_section_count; i++)
  {
    if (strcmp(prof_sections[i]->id, id) == 0)
    {
      *ptr = prof_sections[i];
      return;
    }
  }

  /* Create new section if space available */
  if (prof_section_count < MAX_PROF_SECTIONS)
  {
    sect = calloc(1, sizeof(struct PERF_prof_sect));
    if (!sect)
      return;

    strncpy(sect->id, id, sizeof(sect->id) - 1);
    sect->id[sizeof(sect->id) - 1] = '\0';

    prof_sections[prof_section_count++] = sect;
    *ptr = sect;
  }
}

void PERF_prof_sect_enter(struct PERF_prof_sect *ptr)
{
  uint64_t now_usec;

  if (!ptr)
    return;

  now_usec = monotonic_usec();
  if (now_usec == 0)
  {
    return;
  }

  ptr->active = 1;
  ptr->pulse_enter_count++;
  ptr->total_enter_count++;
  ptr->last_enter_usec = now_usec;
}

void PERF_prof_sect_enable_sampling(struct PERF_prof_sect *ptr)
{
  if (ptr != NULL)
  {
    ptr->sampling_enabled = 1;
  }
}

void PERF_prof_sect_exit(struct PERF_prof_sect *ptr)
{
  uint64_t now_usec;
  uint64_t diff_usec;

  if (!ptr || !ptr->active)
    return;

  now_usec = monotonic_usec();
  ptr->active = 0;
  if (ptr->last_enter_usec == 0 || now_usec == 0 || now_usec < ptr->last_enter_usec)
  {
    ptr->last_enter_usec = 0;
    return;
  }
  diff_usec = now_usec - ptr->last_enter_usec;
  ptr->last_enter_usec = 0;

  ptr->pulse_exit_count++;
  ptr->total_exit_count++;
  ptr->pulse_total_usec += diff_usec;
  ptr->total_usec += diff_usec;

  /* Update pulse max if needed */
  if (diff_usec > ptr->pulse_max_usec)
  {
    ptr->pulse_max_usec = diff_usec;
  }

  /* Update total max if needed */
  if (diff_usec > ptr->max_usec)
  {
    ptr->max_usec = diff_usec;
  }

  if (ptr->sampling_enabled && ptr->samples == NULL)
  {
    ptr->samples = calloc(PROF_SAMPLE_CAPACITY, sizeof(*ptr->samples));
  }
  if (ptr->sampling_enabled && ptr->samples != NULL)
  {
    ptr->samples[ptr->sample_index] = diff_usec;
    ptr->sample_index = (ptr->sample_index + 1) % PROF_SAMPLE_CAPACITY;
    if (ptr->sample_count < PROF_SAMPLE_CAPACITY)
    {
      ptr->sample_count++;
    }
    ptr->samples_seen++;
  }
}

void PERF_prof_reset(void)
{
  struct PERF_prof_sect *sect;
  int i;

  for (i = 0; i < prof_section_count; i++)
  {
    sect = prof_sections[i];
    sect->pulse_enter_count = 0;
    sect->pulse_exit_count = 0;
    sect->pulse_total_usec = 0;
    sect->pulse_max_usec = 0;
  }
}

/* Helper function to format a profiling section */
static size_t format_prof_section(char *buf, size_t n, const struct PERF_prof_sect *sect,
                                  int is_total)
{
  uint64_t enter_count;
  uint64_t exit_count;
  uint64_t usec_total;
  uint64_t usec_max;
  uint64_t elapsed_usec;
  uint64_t now_usec;
  double average;
  double median;
  double p95;
  double p99;
  double percent;

  if (is_total)
  {
    enter_count = sect->total_enter_count;
    exit_count = sect->total_exit_count;
    usec_total = sect->total_usec;
    usec_max = sect->max_usec;

    now_usec = monotonic_usec();
    elapsed_usec = now_usec >= prof_reset_usec ? now_usec - prof_reset_usec : 0;
    percent = elapsed_usec > 0 ? (100.0 * (double)usec_total / (double)elapsed_usec) : 0.0;
  }
  else
  {
    enter_count = sect->pulse_enter_count;
    exit_count = sect->pulse_exit_count;
    usec_total = sect->pulse_total_usec;
    usec_max = sect->pulse_max_usec;

    /* Calculate percentage of pulse time */
    percent = (100.0 * (double)usec_total) / USEC_PER_PULSE;
  }

  if (enter_count == 0)
    return 0;

  if (is_total)
  {
    average = exit_count > 0 ? (double)usec_total / (double)exit_count : 0.0;
    calculate_percentile_set(sect->samples, sect->sample_count, &median, &p95, &p99);

    return bounded_format_length(
        snprintf(buf, n,
                 "%-24.24s|%9" PRIu64 "|%12" PRIu64 "|%8.2f%%|%10.2f|%10.2f|%10.2f|%10.2f|"
                 "%10" PRIu64 "|%7zu/%-7" PRIu64 "\n\r",
                 sect->id, exit_count, usec_total, percent, average, median, p95, p99, usec_max,
                 sect->sample_count, sect->samples_seen),
        n);
  }
  else
  {
    return bounded_format_length(
        snprintf(buf, n,
                 "%-24.24s|%9" PRIu64 "|%9" PRIu64 "|%12" PRIu64 "|%8.2f%%|"
                 "%10" PRIu64 "|%8.2f%%\n\r",
                 sect->id, enter_count, exit_count, usec_total, percent, usec_max,
                 (100.0 * (double)usec_max) / USEC_PER_PULSE),
        n);
  }
}

size_t PERF_prof_repr_pulse(char *out_buf, size_t n)
{
  size_t written = 0;
  int i;

  if (!out_buf || n < 1)
    return 0;

  written = bounded_format_length(
      snprintf(
          out_buf, n,
          "Pulse profiling info\n\r\n\r"
          "Section name            |    Enter|     Exit|  Total usec| Pulse %%|"
          "  Max usec|Max %%\n\r"
          "---------------------------------------------------------------------------------------"
          "\n\r"),
      n);

  for (i = 0; i < prof_section_count && written < n - 1; i++)
  {
    written += format_prof_section(out_buf + written, n - written, prof_sections[i], 0);
  }

  return written;
}

size_t PERF_prof_repr_total(char *out_buf, size_t n)
{
  size_t written = 0;
  int i;

  if (!out_buf || n < 1)
    return 0;

  ensure_initialized();

  written = bounded_format_length(
      snprintf(out_buf, n,
               "Cumulative profiling info\n\r\n\r"
               "Section name            |    Calls|  Total usec| Total %%|  Avg usec|Median usec|"
               "  P95 usec|  P99 usec|  Max usec|Samples stored/seen\n\r"
               "--------------------------------------------------------------------------------"
               "---------"
               "------------------------------------------------------\n\r"),
      n);

  for (i = 0; i < prof_section_count && written < n - 1; i++)
  {
    written += format_prof_section(out_buf + written, n - written, prof_sections[i], 1);
  }

  return written;
}

size_t PERF_prof_repr_sect(char *out_buf, size_t n, const char *id)
{
  size_t written = 0;
  int i;
  struct PERF_prof_sect *sect = NULL;

  if (!out_buf || n < 1 || !id)
    return 0;

  /* Find the section */
  for (i = 0; i < prof_section_count; i++)
  {
    if (strcmp(prof_sections[i]->id, id) == 0)
    {
      sect = prof_sections[i];
      break;
    }
  }

  if (!sect)
  {
    return bounded_format_length(snprintf(out_buf, n, "No such section '%s'\n\r", id), n);
  }

  /* Generate both pulse and total reports for this section */
  written = bounded_format_length(
      snprintf(
          out_buf, n,
          "Pulse profiling info\n\r\n\r"
          "Section name            |    Enter|     Exit|  Total usec| Pulse %%|"
          "  Max usec|Max %%\n\r"
          "---------------------------------------------------------------------------------------"
          "\n\r"),
      n);

  if (written < n - 1)
  {
    written += format_prof_section(out_buf + written, n - written, sect, 0);
  }

  if (written < n - 1)
  {
    written += bounded_format_length(
        snprintf(out_buf + written, n - written,
                 "\n\rCumulative profiling info\n\r\n\r"
                 "Section name            |    Calls|  Total usec| Total %%|  Avg usec|Median usec|"
                 "  P95 usec|  P99 usec|  Max usec|Samples stored/seen\n\r"
                 "---------------------------------------------------------------------------------"
                 "--"
                 "----------------------------------------------------------\n\r"),
        n - written);

    if (written < n - 1)
    {
      written += format_prof_section(out_buf + written, n - written, sect, 1);
    }
  }

  return written;
}

size_t PERF_prof_repr_csv(char *out_buf, size_t n)
{
  struct PERF_prof_sect *sect;
  size_t written;
  double average;
  double median;
  double p95;
  double p99;
  int i;

  if (out_buf == NULL || n < 1)
  {
    return 0;
  }

  ensure_initialized();
  written = bounded_format_length(
      snprintf(out_buf, n,
               "section,calls,total_usec,average_usec,median_usec,p95_usec,p99_usec,"
               "max_usec,samples_stored,samples_seen\n\r"),
      n);

  for (i = 0; i < prof_section_count && written < n - 1; i++)
  {
    sect = prof_sections[i];
    if (!sect->sampling_enabled || sect->total_exit_count == 0)
    {
      continue;
    }

    average = (double)sect->total_usec / (double)sect->total_exit_count;
    calculate_percentile_set(sect->samples, sect->sample_count, &median, &p95, &p99);
    written +=
        snprintf(out_buf + written, n - written,
                 "%s,%" PRIu64 ",%" PRIu64 ",%.2f,%.2f,%.2f,%.2f,%" PRIu64 ",%zu,%" PRIu64 "\n\r",
                 sect->id, sect->total_exit_count, sect->total_usec, average, median, p95, p99,
                 sect->max_usec, sect->sample_count, sect->samples_seen);
  }

  if (written < n - 1)
  {
    written += bounded_format_length(
        snprintf(out_buf + written, n - written, "# missed_pulses=%" PRIu64 "\n\r",
                 missed_pulse_count),
        n - written);
  }
  if (written < n - 1)
  {
    written += bounded_format_length(
        snprintf(out_buf + written, n - written, "# vessel_messages_throttled=%" PRIu64 "\n\r",
                 vessel_message_throttled_count),
        n - written);
  }

  return written;
}
