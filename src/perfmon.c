#include "conf.h"
#include "sysdep.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <float.h>
#include <inttypes.h>
#include <math.h>
#include <stdint.h>
#include <sys/resource.h>
#if defined(__GLIBC__)
#include <malloc.h>
#endif

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "handler.h"
#include "dgscript/dg_event.h"
#include "perfmon.h"

/* ========================================================================
 * CONSTANTS
 * ======================================================================== */

#define MAX_PROF_SECTIONS 2000        /* Maximum number of profiling sections */
#define USEC_PER_SEC 1000000          /* Microseconds per second */
#define PROF_SAMPLE_CAPACITY 16384    /* Per-section rolling percentile window */
#define EVENT_PROFILE_CAPACITY 512    /* Fixed callback identity registry */
#define EVENT_PROFILE_NAME_SIZE 64    /* Includes terminating NUL */
#define EVENT_PROFILE_REPORT_LIMIT 16 /* Maximum callback rows per report */

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

struct perf_event_callback
{
  char identity[EVENT_PROFILE_NAME_SIZE];
  uint64_t pulse_calls;
  uint64_t pulse_total_usec;
  uint64_t pulse_max_usec;
  uint64_t total_calls;
  uint64_t total_usec;
  uint64_t total_max_usec;
};

struct perf_event_process_stats
{
  uint64_t calls;
  uint64_t callbacks_processed;
  uint64_t events_created;
  uint64_t initial_depth;
  uint64_t latest_depth;
  uint64_t max_depth_before;
  uint64_t max_depth_after;
};

struct perf_extraction_stats
{
  uint64_t calls;
  uint64_t pending_before;
  uint64_t processed;
  uint64_t pending_after;
  uint64_t max_processed;
  uint64_t max_pending_before;
  uint64_t max_pending_after;
};

struct perf_catchup_stats
{
  uint64_t passes;
  uint64_t budget_exhausted_passes;
  uint64_t requested_missed;
  uint64_t replayed_missed;
  uint64_t remaining_backlog;
  uint64_t max_requested_missed;
  uint64_t max_remaining_backlog;
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
static struct perf_event_callback event_profiles[EVENT_PROFILE_CAPACITY];
static size_t event_profile_count;
static struct perf_event_callback event_profile_overflow;
static struct perf_event_process_stats pulse_event_process_stats;
static struct perf_event_process_stats total_event_process_stats;
static struct perf_extraction_stats pulse_extraction_stats;
static struct perf_extraction_stats total_extraction_stats;
static struct perf_catchup_stats pulse_catchup_stats;
static struct perf_catchup_stats total_catchup_stats;

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

/* Memory monitoring state */
static struct perf_memory_stats boot_memory_stats;
static struct perf_memory_stats reset_memory_stats;
static uint64_t memory_boot_time_sec = 0;
static uint64_t memory_reset_time_sec = 0;
static uint64_t memory_peak_rss_kib = 0;
static uint64_t memory_peak_anon_kib = 0;
static uint64_t memory_last_alert_time_sec = 0;

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

uint64_t PERF_monotonic_usec(void)
{
  return monotonic_usec();
}

static uint64_t saturating_add_u64(uint64_t current, uint64_t addition)
{
  if (UINT64_MAX - current < addition)
  {
    return UINT64_MAX;
  }
  return current + addition;
}

static void copy_event_identity(char *destination, size_t capacity, const char *identity)
{
  size_t i;

  if (destination == NULL || capacity == 0)
  {
    return;
  }

  if (identity == NULL || *identity == '\0')
  {
    identity = "unknown_event";
  }

  for (i = 0; i + 1 < capacity && identity[i] != '\0'; i++)
  {
    if (identity[i] == ',' || identity[i] == '\r' || identity[i] == '\n')
      destination[i] = ' ';
    else
      destination[i] = identity[i];
  }
  destination[i] = '\0';
}

static void reset_event_callback_pulse_stats(void)
{
  size_t i;

  for (i = 0; i < event_profile_count; i++)
  {
    event_profiles[i].pulse_calls = 0;
    event_profiles[i].pulse_total_usec = 0;
    event_profiles[i].pulse_max_usec = 0;
  }
  event_profile_overflow.pulse_calls = 0;
  event_profile_overflow.pulse_total_usec = 0;
  event_profile_overflow.pulse_max_usec = 0;
}

static void reset_event_callback_total_stats(void)
{
  size_t i;

  for (i = 0; i < event_profile_count; i++)
  {
    event_profiles[i].total_calls = 0;
    event_profiles[i].total_usec = 0;
    event_profiles[i].total_max_usec = 0;
  }
  event_profile_overflow.total_calls = 0;
  event_profile_overflow.total_usec = 0;
  event_profile_overflow.total_max_usec = 0;
}

static void update_event_process_stats(struct perf_event_process_stats *stats,
                                       uint64_t depth_before, uint64_t depth_after,
                                       uint64_t callbacks_processed, uint64_t events_created)
{
  if (stats->calls == 0)
  {
    stats->initial_depth = depth_before;
  }
  stats->calls = saturating_add_u64(stats->calls, 1);
  stats->callbacks_processed = saturating_add_u64(stats->callbacks_processed, callbacks_processed);
  stats->events_created = saturating_add_u64(stats->events_created, events_created);
  stats->latest_depth = depth_after;
  if (depth_before > stats->max_depth_before)
    stats->max_depth_before = depth_before;
  if (depth_after > stats->max_depth_after)
    stats->max_depth_after = depth_after;
}

static void update_extraction_stats(struct perf_extraction_stats *stats, uint64_t pending_before,
                                    uint64_t processed, uint64_t pending_after)
{
  stats->calls = saturating_add_u64(stats->calls, 1);
  stats->pending_before = saturating_add_u64(stats->pending_before, pending_before);
  stats->processed = saturating_add_u64(stats->processed, processed);
  stats->pending_after = saturating_add_u64(stats->pending_after, pending_after);
  if (processed > stats->max_processed)
    stats->max_processed = processed;
  if (pending_before > stats->max_pending_before)
    stats->max_pending_before = pending_before;
  if (pending_after > stats->max_pending_after)
    stats->max_pending_after = pending_after;
}

static void update_catchup_stats(struct perf_catchup_stats *stats, uint64_t requested_missed,
                                 uint64_t replayed_missed, uint64_t remaining_backlog,
                                 int budget_exhausted)
{
  stats->passes = saturating_add_u64(stats->passes, 1);
  if (budget_exhausted)
    stats->budget_exhausted_passes = saturating_add_u64(stats->budget_exhausted_passes, 1);
  stats->requested_missed = saturating_add_u64(stats->requested_missed, requested_missed);
  stats->replayed_missed = saturating_add_u64(stats->replayed_missed, replayed_missed);
  stats->remaining_backlog = saturating_add_u64(stats->remaining_backlog, remaining_backlog);
  if (requested_missed > stats->max_requested_missed)
    stats->max_requested_missed = requested_missed;
  if (remaining_backlog > stats->max_remaining_backlog)
    stats->max_remaining_backlog = remaining_backlog;
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

static void free_interval(struct perf_interval *interval)
{
  free(interval->avg_data);
  free(interval->min_data);
  free(interval->max_data);
  memset(interval, 0, sizeof(*interval));
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

  if (memory_boot_time_sec == 0)
  {
    PERF_sample_memory(&boot_memory_stats);
    reset_memory_stats = boot_memory_stats;
    memory_boot_time_sec = (uint64_t)time(NULL);
    memory_reset_time_sec = memory_boot_time_sec;
  }

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
  reset_event_callback_pulse_stats();
  reset_event_callback_total_stats();
  memset(&pulse_event_process_stats, 0, sizeof(pulse_event_process_stats));
  memset(&total_event_process_stats, 0, sizeof(total_event_process_stats));
  memset(&pulse_extraction_stats, 0, sizeof(pulse_extraction_stats));
  memset(&total_extraction_stats, 0, sizeof(total_extraction_stats));
  memset(&pulse_catchup_stats, 0, sizeof(pulse_catchup_stats));
  memset(&total_catchup_stats, 0, sizeof(total_catchup_stats));

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

  PERF_sample_memory(&reset_memory_stats);
  memory_reset_time_sec = (uint64_t)time(NULL);
  if (memory_boot_time_sec == 0)
  {
    boot_memory_stats = reset_memory_stats;
    memory_boot_time_sec = memory_reset_time_sec;
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

int PERF_register_event_callback(const char *identity)
{
  char normalized_identity[EVENT_PROFILE_NAME_SIZE];
  size_t i;

  ensure_initialized();
  copy_event_identity(normalized_identity, sizeof(normalized_identity), identity);
  for (i = 0; i < event_profile_count; i++)
  {
    if (strcmp(event_profiles[i].identity, normalized_identity) == 0)
    {
      return (int)i;
    }
  }

  if (event_profile_count >= EVENT_PROFILE_CAPACITY)
  {
    return -1;
  }

  copy_event_identity(event_profiles[event_profile_count].identity,
                      sizeof(event_profiles[event_profile_count].identity), normalized_identity);
  event_profile_count++;
  return (int)(event_profile_count - 1);
}

void PERF_note_event_callback(int profile_index, uint64_t elapsed_usec)
{
  struct perf_event_callback *profile;

  if (profile_index < 0 || (size_t)profile_index >= event_profile_count)
    profile = &event_profile_overflow;
  else
    profile = &event_profiles[profile_index];

  profile->pulse_calls = saturating_add_u64(profile->pulse_calls, 1);
  profile->pulse_total_usec = saturating_add_u64(profile->pulse_total_usec, elapsed_usec);
  profile->total_calls = saturating_add_u64(profile->total_calls, 1);
  profile->total_usec = saturating_add_u64(profile->total_usec, elapsed_usec);
  if (elapsed_usec > profile->pulse_max_usec)
    profile->pulse_max_usec = elapsed_usec;
  if (elapsed_usec > profile->total_max_usec)
    profile->total_max_usec = elapsed_usec;
}

void PERF_note_event_process(uint64_t depth_before, uint64_t depth_after,
                             uint64_t callbacks_processed, uint64_t events_created)
{
  update_event_process_stats(&pulse_event_process_stats, depth_before, depth_after,
                             callbacks_processed, events_created);
  update_event_process_stats(&total_event_process_stats, depth_before, depth_after,
                             callbacks_processed, events_created);
}

void PERF_note_pending_extractions(uint64_t pending_before, uint64_t processed,
                                   uint64_t pending_after)
{
  update_extraction_stats(&pulse_extraction_stats, pending_before, processed, pending_after);
  update_extraction_stats(&total_extraction_stats, pending_before, processed, pending_after);
}

void PERF_note_catchup_pass(uint64_t requested_missed, uint64_t replayed_missed,
                            uint64_t remaining_backlog, int budget_exhausted)
{
  update_catchup_stats(&pulse_catchup_stats, requested_missed, replayed_missed, remaining_backlog,
                       budget_exhausted);
  update_catchup_stats(&total_catchup_stats, requested_missed, replayed_missed, remaining_backlog,
                       budget_exhausted);
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
  reset_event_callback_pulse_stats();
  memset(&pulse_event_process_stats, 0, sizeof(pulse_event_process_stats));
  memset(&pulse_extraction_stats, 0, sizeof(pulse_extraction_stats));
  memset(&pulse_catchup_stats, 0, sizeof(pulse_catchup_stats));
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
    return bounded_format_length(snprintf(buf, n,
                                          "%-24.24s|%9" PRIu64 "|%9" PRIu64 "|%12" PRIu64
                                          "|%8.2f%%|"
                                          "%10" PRIu64 "|%8.2f%%\n\r",
                                          sect->id, enter_count, exit_count, usec_total, percent,
                                          usec_max, (100.0 * (double)usec_max) / USEC_PER_PULSE),
                                 n);
  }
}

static uint64_t event_profile_score(size_t index, int is_total)
{
  if (is_total)
    return event_profiles[index].total_usec;
  return event_profiles[index].pulse_total_usec;
}

static uint64_t event_profile_calls(size_t index, int is_total)
{
  if (is_total)
    return event_profiles[index].total_calls;
  return event_profiles[index].pulse_calls;
}

static size_t collect_top_event_profiles(size_t *top_indices, int is_total)
{
  uint64_t score;
  size_t top_count = 0;
  size_t i;
  size_t j;
  size_t position;

  for (i = 0; i < event_profile_count; i++)
  {
    if (event_profile_calls(i, is_total) == 0)
      continue;

    score = event_profile_score(i, is_total);
    position = 0;
    while (position < top_count && event_profile_score(top_indices[position], is_total) >= score)
      position++;

    if (position >= EVENT_PROFILE_REPORT_LIMIT)
      continue;
    if (top_count < EVENT_PROFILE_REPORT_LIMIT)
      top_count++;
    for (j = top_count - 1; j > position; j--)
      top_indices[j] = top_indices[j - 1];
    top_indices[position] = i;
  }

  return top_count;
}

static size_t format_event_telemetry(char *buf, size_t n, int is_total)
{
  const struct perf_event_process_stats *process_stats;
  const struct perf_extraction_stats *extraction_stats;
  const struct perf_catchup_stats *catchup_stats;
  const struct perf_event_callback *profile;
  size_t top_indices[EVENT_PROFILE_REPORT_LIMIT];
  size_t top_count;
  size_t written;
  size_t i;
  uint64_t overflow_calls;
  double average;

  if (buf == NULL || n == 0)
    return 0;

  if (is_total)
  {
    process_stats = &total_event_process_stats;
    extraction_stats = &total_extraction_stats;
    catchup_stats = &total_catchup_stats;
  }
  else
  {
    process_stats = &pulse_event_process_stats;
    extraction_stats = &pulse_extraction_stats;
    catchup_stats = &pulse_catchup_stats;
  }
  overflow_calls =
      is_total ? event_profile_overflow.total_calls : event_profile_overflow.pulse_calls;

  written = bounded_format_length(
      snprintf(buf, n,
               "\n\r%s game-loop telemetry\n\r"
               "Event queue: calls=%" PRIu64 " callbacks=%" PRIu64 " created=%" PRIu64
               " depth=%" PRIu64 "->%" PRIu64 " max_before=%" PRIu64 " max_after=%" PRIu64 "\n\r"
               "Extractions: calls=%" PRIu64 " pending_before=%" PRIu64 " processed=%" PRIu64
               " pending_after=%" PRIu64 " max_processed=%" PRIu64 " max_pending_before=%" PRIu64
               " max_pending_after=%" PRIu64 "\n\r"
               "Catch-up: passes=%" PRIu64 " budget_exhausted=%" PRIu64 " requested_missed=%" PRIu64
               " replayed_missed=%" PRIu64 " remaining_backlog=%" PRIu64 " max_requested=%" PRIu64
               " max_remaining=%" PRIu64 "\n\r"
               "Event callback registry: registered=%zu/%d report_limit=%d overflow_calls=%" PRIu64
               "\n\r",
               is_total ? "Cumulative" : "Pulse", process_stats->calls,
               process_stats->callbacks_processed, process_stats->events_created,
               process_stats->initial_depth, process_stats->latest_depth,
               process_stats->max_depth_before, process_stats->max_depth_after,
               extraction_stats->calls, extraction_stats->pending_before,
               extraction_stats->processed, extraction_stats->pending_after,
               extraction_stats->max_processed, extraction_stats->max_pending_before,
               extraction_stats->max_pending_after, catchup_stats->passes,
               catchup_stats->budget_exhausted_passes, catchup_stats->requested_missed,
               catchup_stats->replayed_missed, catchup_stats->remaining_backlog,
               catchup_stats->max_requested_missed, catchup_stats->max_remaining_backlog,
               event_profile_count, EVENT_PROFILE_CAPACITY, EVENT_PROFILE_REPORT_LIMIT,
               overflow_calls),
      n);

  if (written >= n - 1)
    return written;

  written += bounded_format_length(
      snprintf(
          buf + written, n - written,
          "Event callbacks (top %d by total time)\n\r"
          "Identity                            |    Calls|  Total usec|  Avg usec|  Max usec\n\r"
          "-----------------------------------------------------------------------------------\n\r",
          EVENT_PROFILE_REPORT_LIMIT),
      n - written);

  top_count = collect_top_event_profiles(top_indices, is_total);
  for (i = 0; i < top_count && written < n - 1; i++)
  {
    profile = &event_profiles[top_indices[i]];
    if (is_total)
    {
      average = profile->total_calls > 0
                    ? (double)profile->total_usec / (double)profile->total_calls
                    : 0.0;
      written += bounded_format_length(
          snprintf(buf + written, n - written,
                   "%-36.36s|%9" PRIu64 "|%12" PRIu64 "|%10.2f|%10" PRIu64 "\n\r",
                   profile->identity, profile->total_calls, profile->total_usec, average,
                   profile->total_max_usec),
          n - written);
    }
    else
    {
      average = profile->pulse_calls > 0
                    ? (double)profile->pulse_total_usec / (double)profile->pulse_calls
                    : 0.0;
      written += bounded_format_length(
          snprintf(buf + written, n - written,
                   "%-36.36s|%9" PRIu64 "|%12" PRIu64 "|%10.2f|%10" PRIu64 "\n\r",
                   profile->identity, profile->pulse_calls, profile->pulse_total_usec, average,
                   profile->pulse_max_usec),
          n - written);
    }
  }

  profile = &event_profile_overflow;
  if (written < n - 1 &&
      ((is_total && profile->total_calls > 0) || (!is_total && profile->pulse_calls > 0)))
  {
    uint64_t calls;
    uint64_t total_usec;
    uint64_t max_usec;

    calls = is_total ? profile->total_calls : profile->pulse_calls;
    total_usec = is_total ? profile->total_usec : profile->pulse_total_usec;
    max_usec = is_total ? profile->total_max_usec : profile->pulse_max_usec;
    average = calls > 0 ? (double)total_usec / (double)calls : 0.0;
    written += bounded_format_length(
        snprintf(buf + written, n - written,
                 "%-36.36s|%9" PRIu64 "|%12" PRIu64 "|%10.2f|%10" PRIu64 "\n\r",
                 "[unregistered overflow]", calls, total_usec, average, max_usec),
        n - written);
  }

  return written;
}

static size_t format_event_telemetry_csv(char *buf, size_t n)
{
  const struct perf_event_callback *profile;
  size_t top_indices[EVENT_PROFILE_REPORT_LIMIT];
  size_t top_count;
  size_t written;
  size_t i;
  double average;

  if (buf == NULL || n == 0)
    return 0;

  written = bounded_format_length(
      snprintf(buf, n,
               "# event_process_calls=%" PRIu64 "\n\r"
               "# event_callbacks_processed=%" PRIu64 "\n\r"
               "# events_created_during_processing=%" PRIu64 "\n\r"
               "# event_queue_depth_initial=%" PRIu64 "\n\r"
               "# event_queue_depth_latest=%" PRIu64 "\n\r"
               "# event_queue_depth_max_before=%" PRIu64 "\n\r"
               "# event_queue_depth_max_after=%" PRIu64 "\n\r"
               "# extraction_calls=%" PRIu64 "\n\r"
               "# extractions_pending_before=%" PRIu64 "\n\r"
               "# extractions_processed=%" PRIu64 "\n\r"
               "# extractions_pending_after=%" PRIu64 "\n\r"
               "# max_extractions_per_call=%" PRIu64 "\n\r"
               "# max_extractions_pending_before=%" PRIu64 "\n\r"
               "# max_extractions_pending_after=%" PRIu64 "\n\r"
               "# catchup_passes=%" PRIu64 "\n\r"
               "# catchup_budget_exhausted_passes=%" PRIu64 "\n\r"
               "# catchup_requested_missed=%" PRIu64 "\n\r"
               "# catchup_replayed_missed=%" PRIu64 "\n\r"
               "# catchup_remaining_backlog=%" PRIu64 "\n\r"
               "# catchup_max_requested_missed=%" PRIu64 "\n\r"
               "# catchup_max_remaining_backlog=%" PRIu64 "\n\r"
               "# event_profile_registered=%zu\n\r"
               "# event_profile_capacity=%d\n\r"
               "# event_profile_report_limit=%d\n\r"
               "# event_profile_overflow_calls=%" PRIu64 "\n\r",
               total_event_process_stats.calls, total_event_process_stats.callbacks_processed,
               total_event_process_stats.events_created, total_event_process_stats.initial_depth,
               total_event_process_stats.latest_depth, total_event_process_stats.max_depth_before,
               total_event_process_stats.max_depth_after, total_extraction_stats.calls,
               total_extraction_stats.pending_before, total_extraction_stats.processed,
               total_extraction_stats.pending_after, total_extraction_stats.max_processed,
               total_extraction_stats.max_pending_before, total_extraction_stats.max_pending_after,
               total_catchup_stats.passes, total_catchup_stats.budget_exhausted_passes,
               total_catchup_stats.requested_missed, total_catchup_stats.replayed_missed,
               total_catchup_stats.remaining_backlog, total_catchup_stats.max_requested_missed,
               total_catchup_stats.max_remaining_backlog, event_profile_count,
               EVENT_PROFILE_CAPACITY, EVENT_PROFILE_REPORT_LIMIT,
               event_profile_overflow.total_calls),
      n);
  if (written >= n - 1)
    return written;

  written +=
      bounded_format_length(snprintf(buf + written, n - written,
                                     "event_identity,calls,total_usec,average_usec,max_usec\n\r"),
                            n - written);
  top_count = collect_top_event_profiles(top_indices, 1);
  for (i = 0; i < top_count && written < n - 1; i++)
  {
    profile = &event_profiles[top_indices[i]];
    average =
        profile->total_calls > 0 ? (double)profile->total_usec / (double)profile->total_calls : 0.0;
    written += bounded_format_length(
        snprintf(buf + written, n - written, "%s,%" PRIu64 ",%" PRIu64 ",%.2f,%" PRIu64 "\n\r",
                 profile->identity, profile->total_calls, profile->total_usec, average,
                 profile->total_max_usec),
        n - written);
  }

  profile = &event_profile_overflow;
  if (written < n - 1 && profile->total_calls > 0)
  {
    average = (double)profile->total_usec / (double)profile->total_calls;
    written += bounded_format_length(
        snprintf(buf + written, n - written,
                 "[unregistered overflow],%" PRIu64 ",%" PRIu64 ",%.2f,%" PRIu64 "\n\r",
                 profile->total_calls, profile->total_usec, average, profile->total_max_usec),
        n - written);
  }

  return written;
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

  if (written < n - 1)
    written += format_event_telemetry(out_buf + written, n - written, 0);

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

  if (written < n - 1)
    written += format_event_telemetry(out_buf + written, n - written, 1);

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
    written +=
        bounded_format_length(snprintf(out_buf + written, n - written,
                                       "# missed_pulses=%" PRIu64 "\n\r", missed_pulse_count),
                              n - written);
  }
  if (written < n - 1)
  {
    written += bounded_format_length(snprintf(out_buf + written, n - written,
                                              "# vessel_messages_throttled=%" PRIu64 "\n\r",
                                              vessel_message_throttled_count),
                                     n - written);
  }
  if (written < n - 1)
    written += format_event_telemetry_csv(out_buf + written, n - written);

  return written;
}

void PERF_cleanup(void)
{
  int i;

  free_interval(&pulse_data);
  free_interval(&sec_data);
  free_interval(&min_data);
  free_interval(&hour_data);

  for (i = 0; i < prof_section_count; i++)
  {
    free(prof_sections[i]->samples);
    free(prof_sections[i]);
    prof_sections[i] = NULL;
  }

  prof_section_count = 0;
  memset(event_profiles, 0, sizeof(event_profiles));
  memset(&event_profile_overflow, 0, sizeof(event_profile_overflow));
  event_profile_count = 0;
  memset(&pulse_event_process_stats, 0, sizeof(pulse_event_process_stats));
  memset(&total_event_process_stats, 0, sizeof(total_event_process_stats));
  memset(&pulse_extraction_stats, 0, sizeof(pulse_extraction_stats));
  memset(&total_extraction_stats, 0, sizeof(total_extraction_stats));
  memset(&pulse_catchup_stats, 0, sizeof(pulse_catchup_stats));
  memset(&total_catchup_stats, 0, sizeof(total_catchup_stats));
  memset(&boot_memory_stats, 0, sizeof(boot_memory_stats));
  memset(&reset_memory_stats, 0, sizeof(reset_memory_stats));
  memory_boot_time_sec = 0;
  memory_reset_time_sec = 0;
  memory_peak_rss_kib = 0;
  memory_peak_anon_kib = 0;
  memory_last_alert_time_sec = 0;
  initialized = 0;
}

/* ========================================================================
 * MEMORY MONITORING IMPLEMENTATION
 * ======================================================================== */

static int sample_proc_status(struct perf_memory_stats *stats)
{
  FILE *f;
  char line[256];

  if (stats == NULL)
    return 0;

  f = fopen("/proc/self/status", "r");
  if (!f)
    return 0;

  while (fgets(line, sizeof(line), f))
  {
    if (strncmp(line, "VmSize:", 7) == 0)
      stats->vm_size_kib = strtoull(line + 7, NULL, 10);
    else if (strncmp(line, "VmRSS:", 6) == 0)
      stats->vm_rss_kib = strtoull(line + 6, NULL, 10);
    else if (strncmp(line, "RssAnon:", 8) == 0)
      stats->rss_anon_kib = strtoull(line + 8, NULL, 10);
    else if (strncmp(line, "RssFile:", 8) == 0)
      stats->rss_file_kib = strtoull(line + 8, NULL, 10);
    else if (strncmp(line, "RssShmem:", 9) == 0)
      stats->rss_shmem_kib = strtoull(line + 9, NULL, 10);
    else if (strncmp(line, "VmData:", 7) == 0)
      stats->vm_data_kib = strtoull(line + 7, NULL, 10);
    else if (strncmp(line, "VmSwap:", 7) == 0)
      stats->vm_swap_kib = strtoull(line + 7, NULL, 10);
  }
  fclose(f);
  return 1;
}

int PERF_sample_memory(struct perf_memory_stats *stats)
{
  struct rusage ru;
  struct descriptor_data *d;
  struct char_data *ch;
  struct obj_data *obj;

  if (stats == NULL)
    return 0;

  memset(stats, 0, sizeof(*stats));
  stats->timestamp_sec = (uint64_t)time(NULL);

  /* Sample Linux /proc/self/status */
  sample_proc_status(stats);

  /* Allocator statistics via glibc mallinfo2 / mallinfo */
#if defined(__GLIBC__) && ((__GLIBC__ > 2) || (__GLIBC__ == 2 && __GLIBC_MINOR__ >= 33))
  struct mallinfo2 mi = mallinfo2();
  stats->heap_arena_kib = (uint64_t)(mi.arena / 1024);
  stats->heap_inuse_kib = (uint64_t)(mi.uordblks / 1024);
  stats->heap_free_kib = (uint64_t)(mi.fordblks / 1024);
  stats->heap_mmap_kib = (uint64_t)(mi.hblkhd / 1024);
#elif defined(__GLIBC__)
  struct mallinfo mi = mallinfo();
  stats->heap_arena_kib = (uint64_t)((size_t)mi.arena / 1024);
  stats->heap_inuse_kib = (uint64_t)((size_t)mi.uordblks / 1024);
  stats->heap_free_kib = (uint64_t)((size_t)mi.fordblks / 1024);
  stats->heap_mmap_kib = (uint64_t)((size_t)mi.hblkhd / 1024);
#endif

  /* Fallback or supplement with getrusage */
  if (getrusage(RUSAGE_SELF, &ru) == 0)
  {
    stats->max_rss_kib = (uint64_t)ru.ru_maxrss;
    if (stats->vm_rss_kib == 0 && ru.ru_maxrss > 0)
      stats->vm_rss_kib = (uint64_t)ru.ru_maxrss;
  }

  /* Track peaks */
  if (stats->vm_rss_kib > memory_peak_rss_kib)
    memory_peak_rss_kib = stats->vm_rss_kib;
  if (stats->rss_anon_kib > memory_peak_anon_kib)
    memory_peak_anon_kib = stats->rss_anon_kib;

  /* Count descriptors */
  for (d = descriptor_list; d; d = d->next)
  {
    stats->count_descriptors++;
    if (IS_PLAYING(d))
      stats->count_playing++;
  }

  /* Count characters */
  for (ch = character_list; ch; ch = ch->next)
  {
    stats->count_chars++;
    if (IS_NPC(ch))
      stats->count_mobs++;
    else
      stats->count_pcs++;
  }

  /* Count objects */
  for (obj = object_list; obj; obj = obj->next)
    stats->count_objs++;

  /* World rooms & zones */
  if (world != NULL && top_of_world != NOWHERE)
    stats->count_rooms = (uint64_t)(top_of_world + 1);
  if (zone_table != NULL && top_of_zone_table != NOWHERE)
    stats->count_zones = (uint64_t)(top_of_zone_table + 1);

  /* Events and extractions */
  stats->count_events = (uint64_t)event_queue_depth();
  stats->count_pending_extractions = (uint64_t)pending_extractions_count();

  return 1;
}

int PERF_memory_growth_rate(double *rss_kib_per_min, double *anon_kib_per_min,
                            double *heap_kib_per_min)
{
  struct perf_memory_stats current;
  uint64_t now_sec;
  double elapsed_min;
  int64_t rss_diff, anon_diff, heap_diff;

  if (rss_kib_per_min)
    *rss_kib_per_min = 0.0;
  if (anon_kib_per_min)
    *anon_kib_per_min = 0.0;
  if (heap_kib_per_min)
    *heap_kib_per_min = 0.0;

  if (!PERF_sample_memory(&current))
    return 0;

  now_sec = current.timestamp_sec;
  if (memory_reset_time_sec == 0 || now_sec <= memory_reset_time_sec)
    return 0;

  elapsed_min = (double)(now_sec - memory_reset_time_sec) / 60.0;
  if (elapsed_min < 0.1)
    return 0;

  rss_diff = (int64_t)current.vm_rss_kib - (int64_t)reset_memory_stats.vm_rss_kib;
  anon_diff = (int64_t)current.rss_anon_kib - (int64_t)reset_memory_stats.rss_anon_kib;
  heap_diff = (int64_t)current.heap_inuse_kib - (int64_t)reset_memory_stats.heap_inuse_kib;

  if (rss_kib_per_min)
    *rss_kib_per_min = (double)rss_diff / elapsed_min;
  if (anon_kib_per_min)
    *anon_kib_per_min = (double)anon_diff / elapsed_min;
  if (heap_kib_per_min)
    *heap_kib_per_min = (double)heap_diff / elapsed_min;

  return 1;
}

void PERF_memory_periodic_check(void)
{
  struct perf_memory_stats current;
  double rss_rate = 0.0, anon_rate = 0.0, heap_rate = 0.0;
  uint64_t now_sec;

  if (!PERF_sample_memory(&current))
    return;

  now_sec = current.timestamp_sec;

  /* Initialize baselines if needed */
  if (memory_boot_time_sec == 0)
  {
    boot_memory_stats = current;
    reset_memory_stats = current;
    memory_boot_time_sec = now_sec;
    memory_reset_time_sec = now_sec;
    return;
  }

  /* Check growth rate if at least 5 minutes elapsed since reset */
  if (now_sec - memory_reset_time_sec >= 300)
  {
    if (PERF_memory_growth_rate(&rss_rate, &anon_rate, &heap_rate))
    {
      /* Alert if sustained anon RSS growth > 1024 KiB/min and 15 min since last alert */
      if (anon_rate > 1024.0 &&
          (memory_last_alert_time_sec == 0 || now_sec - memory_last_alert_time_sec >= 900))
      {
        memory_last_alert_time_sec = now_sec;
        log("PERFMON [MEMORY ALERT]: Elevated anonymous memory growth detected: +%.1f KiB/min "
            "(+%.2f MiB/hr). "
            "RSS: %llu KiB, Anon: %llu KiB, Heap in-use: %llu KiB. Live entities: %llu PCs, %llu "
            "mobs, "
            "%llu objs, %llu events.",
            anon_rate, (anon_rate * 60.0) / 1024.0, (unsigned long long)current.vm_rss_kib,
            (unsigned long long)current.rss_anon_kib, (unsigned long long)current.heap_inuse_kib,
            (unsigned long long)current.count_pcs, (unsigned long long)current.count_mobs,
            (unsigned long long)current.count_objs, (unsigned long long)current.count_events);
      }
    }
  }
}

size_t PERF_memory_repr(char *out_buf, size_t n)
{
  struct perf_memory_stats cur;
  size_t written = 0;
  uint64_t now_sec;
  uint64_t boot_elapsed_sec = 0;
  uint64_t reset_elapsed_sec = 0;
  int64_t rss_delta = 0, anon_delta = 0, heap_delta = 0;
  double rss_rate = 0.0, anon_rate = 0.0, heap_rate = 0.0;
  double anon_pct = 0.0;
  const char *assessment = "STABLE - Memory usage within normal parameters";

  if (!out_buf || n < 1)
    return 0;

  PERF_sample_memory(&cur);
  now_sec = cur.timestamp_sec;

  if (memory_boot_time_sec > 0 && now_sec >= memory_boot_time_sec)
    boot_elapsed_sec = now_sec - memory_boot_time_sec;
  if (memory_reset_time_sec > 0 && now_sec >= memory_reset_time_sec)
    reset_elapsed_sec = now_sec - memory_reset_time_sec;

  if (reset_memory_stats.vm_rss_kib > 0)
    rss_delta = (int64_t)cur.vm_rss_kib - (int64_t)reset_memory_stats.vm_rss_kib;
  if (reset_memory_stats.rss_anon_kib > 0)
    anon_delta = (int64_t)cur.rss_anon_kib - (int64_t)reset_memory_stats.rss_anon_kib;
  if (reset_memory_stats.heap_inuse_kib > 0)
    heap_delta = (int64_t)cur.heap_inuse_kib - (int64_t)reset_memory_stats.heap_inuse_kib;

  PERF_memory_growth_rate(&rss_rate, &anon_rate, &heap_rate);

  if (cur.vm_rss_kib > 0)
    anon_pct = ((double)cur.rss_anon_kib / (double)cur.vm_rss_kib) * 100.0;

  if (reset_elapsed_sec >= 300)
  {
    if (anon_rate >= 1024.0 || rss_rate >= 1024.0)
      assessment = "CRITICAL - High memory growth rate / possible leak detected";
    else if (anon_rate >= 200.0 || rss_rate >= 200.0)
      assessment = "WARNING - Elevated growth rate (monitor closely)";
    else if (anon_rate >= 20.0 || rss_rate >= 20.0)
      assessment = "MODERATE - Mild memory growth (within normal active bounds)";
  }
  else
  {
    assessment = "COLLECTING - Window under 5 minutes; baseline accumulating";
  }

  written = bounded_format_length(
      snprintf(
          out_buf, n,
          "Memory Monitoring Dashboard\n\r\n\r"
          "Window & Uptime:\n\r"
          "  Elapsed Since Boot:       %02luh %02lum %02lus\n\r"
          "  Elapsed Since Reset:      %02luh %02lum %02lus\n\r\n\r"
          "Operating System Memory (/proc/self/status & rusage):\n\r"
          "  Virtual Size (VmSize):    %8.2f MB (%llu KB)\n\r"
          "  Resident Set (VmRSS):     %8.2f MB (%llu KB)  [Peak: %.2f MB]\n\r"
          "  Anonymous RSS (RssAnon):  %8.2f MB (%llu KB)  [%.1f%% of RSS]\n\r"
          "  File-Backed RSS (RssFile):%8.2f MB (%llu KB)\n\r"
          "  Shared Memory (RssShmem): %8.2f MB (%llu KB)\n\r"
          "  Data Segment (VmData):    %8.2f MB (%llu KB)\n\r"
          "  Swap Used (VmSwap):       %8.2f MB (%llu KB)\n\r"
          "  Peak MaxRSS (rusage):     %8.2f MB (%llu KB)\n\r\n\r"
          "Heap Allocator (glibc mallinfo):\n\r"
          "  In-Use Heap (uordblks):   %8.2f MB (%llu KB)\n\r"
          "  Free in Arena (fordblks): %8.2f MB (%llu KB)\n\r"
          "  Mmap Allocated (hblkhd):  %8.2f MB (%llu KB)\n\r"
          "  Total Arena (arena):      %8.2f MB (%llu KB)\n\r\n\r"
          "Memory Growth Analysis (Since Reset):\n\r"
          "  RSS Net Change:           %+8.2f MB (%+lld KB) [%+.1f KiB/min, %+.2f MiB/hr]\n\r"
          "  Anonymous RSS Net Change: %+8.2f MB (%+lld KB) [%+.1f KiB/min, %+.2f MiB/hr]\n\r"
          "  Heap In-Use Net Change:   %+8.2f MB (%+lld KB) [%+.1f KiB/min, %+.2f MiB/hr]\n\r"
          "  Status Assessment:        %s\n\r\n\r"
          "Live Game Entity Inventory:\n\r"
          "  Sockets / Descriptors:    %llu connected (%llu playing)\n\r"
          "  Characters in World:      %llu total (%llu PCs, %llu Mobs)\n\r"
          "  Objects in World:         %llu\n\r"
          "  Rooms & Zones:            %llu rooms across %llu zones\n\r"
          "  Active Timed Events:      %llu\n\r"
          "  Pending Extractions:      %llu\n\r",
          (unsigned long)(boot_elapsed_sec / 3600), (unsigned long)((boot_elapsed_sec % 3600) / 60),
          (unsigned long)(boot_elapsed_sec % 60), (unsigned long)(reset_elapsed_sec / 3600),
          (unsigned long)((reset_elapsed_sec % 3600) / 60), (unsigned long)(reset_elapsed_sec % 60),
          (double)cur.vm_size_kib / 1024.0, (unsigned long long)cur.vm_size_kib,
          (double)cur.vm_rss_kib / 1024.0, (unsigned long long)cur.vm_rss_kib,
          (double)memory_peak_rss_kib / 1024.0, (double)cur.rss_anon_kib / 1024.0,
          (unsigned long long)cur.rss_anon_kib, anon_pct, (double)cur.rss_file_kib / 1024.0,
          (unsigned long long)cur.rss_file_kib, (double)cur.rss_shmem_kib / 1024.0,
          (unsigned long long)cur.rss_shmem_kib, (double)cur.vm_data_kib / 1024.0,
          (unsigned long long)cur.vm_data_kib, (double)cur.vm_swap_kib / 1024.0,
          (unsigned long long)cur.vm_swap_kib, (double)cur.max_rss_kib / 1024.0,
          (unsigned long long)cur.max_rss_kib, (double)cur.heap_inuse_kib / 1024.0,
          (unsigned long long)cur.heap_inuse_kib, (double)cur.heap_free_kib / 1024.0,
          (unsigned long long)cur.heap_free_kib, (double)cur.heap_mmap_kib / 1024.0,
          (unsigned long long)cur.heap_mmap_kib, (double)cur.heap_arena_kib / 1024.0,
          (unsigned long long)cur.heap_arena_kib, (double)rss_delta / 1024.0, (long long)rss_delta,
          rss_rate, (rss_rate * 60.0) / 1024.0, (double)anon_delta / 1024.0, (long long)anon_delta,
          anon_rate, (anon_rate * 60.0) / 1024.0, (double)heap_delta / 1024.0,
          (long long)heap_delta, heap_rate, (heap_rate * 60.0) / 1024.0, assessment,
          (unsigned long long)cur.count_descriptors, (unsigned long long)cur.count_playing,
          (unsigned long long)cur.count_chars, (unsigned long long)cur.count_pcs,
          (unsigned long long)cur.count_mobs, (unsigned long long)cur.count_objs,
          (unsigned long long)cur.count_rooms, (unsigned long long)cur.count_zones,
          (unsigned long long)cur.count_events, (unsigned long long)cur.count_pending_extractions),
      n);

  return written;
}

size_t PERF_memory_csv(char *out_buf, size_t n)
{
  struct perf_memory_stats cur;
  double rss_rate = 0.0, anon_rate = 0.0, heap_rate = 0.0;
  size_t written = 0;

  if (!out_buf || n < 1)
    return 0;

  PERF_sample_memory(&cur);
  PERF_memory_growth_rate(&rss_rate, &anon_rate, &heap_rate);

  written = bounded_format_length(
      snprintf(out_buf, n,
               "# memory_timestamp_sec=%" PRIu64 "\n\r"
               "# memory_vm_size_kib=%" PRIu64 "\n\r"
               "# memory_vm_rss_kib=%" PRIu64 "\n\r"
               "# memory_rss_anon_kib=%" PRIu64 "\n\r"
               "# memory_rss_file_kib=%" PRIu64 "\n\r"
               "# memory_rss_shmem_kib=%" PRIu64 "\n\r"
               "# memory_vm_data_kib=%" PRIu64 "\n\r"
               "# memory_vm_swap_kib=%" PRIu64 "\n\r"
               "# memory_max_rss_kib=%" PRIu64 "\n\r"
               "# memory_heap_arena_kib=%" PRIu64 "\n\r"
               "# memory_heap_inuse_kib=%" PRIu64 "\n\r"
               "# memory_heap_free_kib=%" PRIu64 "\n\r"
               "# memory_heap_mmap_kib=%" PRIu64 "\n\r"
               "# memory_growth_rss_kib_per_min=%.2f\n\r"
               "# memory_growth_anon_kib_per_min=%.2f\n\r"
               "# memory_growth_heap_kib_per_min=%.2f\n\r"
               "# memory_count_descriptors=%" PRIu64 "\n\r"
               "# memory_count_playing=%" PRIu64 "\n\r"
               "# memory_count_chars=%" PRIu64 "\n\r"
               "# memory_count_pcs=%" PRIu64 "\n\r"
               "# memory_count_mobs=%" PRIu64 "\n\r"
               "# memory_count_objs=%" PRIu64 "\n\r"
               "# memory_count_rooms=%" PRIu64 "\n\r"
               "# memory_count_zones=%" PRIu64 "\n\r"
               "# memory_count_events=%" PRIu64 "\n\r"
               "# memory_count_pending_extractions=%" PRIu64 "\n\r",
               cur.timestamp_sec, cur.vm_size_kib, cur.vm_rss_kib, cur.rss_anon_kib,
               cur.rss_file_kib, cur.rss_shmem_kib, cur.vm_data_kib, cur.vm_swap_kib,
               cur.max_rss_kib, cur.heap_arena_kib, cur.heap_inuse_kib, cur.heap_free_kib,
               cur.heap_mmap_kib, rss_rate, anon_rate, heap_rate, cur.count_descriptors,
               cur.count_playing, cur.count_chars, cur.count_pcs, cur.count_mobs, cur.count_objs,
               cur.count_rooms, cur.count_zones, cur.count_events, cur.count_pending_extractions),
      n);

  return written;
}
