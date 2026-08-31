#include "conf.h"
#include "sysdep.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <float.h>
#include <inttypes.h>
#include <math.h>
#include <pthread.h>
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
#include "dgscript/dg_scripts.h"
#include "mysql.h"
#include "perfmon.h"
#include "active_world.h"
#include "affected_owners.h"
#include "character_periodic.h"
#include "periodic_owners.h"

/* ========================================================================
 * CONSTANTS
 * ======================================================================== */

#define MAX_PROF_SECTIONS 2000        /* Maximum number of profiling sections */
#define USEC_PER_SEC 1000000          /* Microseconds per second */
#define PROF_SAMPLE_CAPACITY 16384    /* Per-section rolling percentile window */
#define EVENT_PROFILE_CAPACITY 512    /* Fixed callback identity registry */
#define EVENT_PROFILE_NAME_SIZE 64    /* Includes terminating NUL */
#define EVENT_PROFILE_REPORT_LIMIT 16 /* Maximum callback rows per report */
#define EVENT_SAMPLE_CAPACITY 1024    /* Per-callback rolling latency window */
#define EVENT_DELAY_BUCKET_COUNT 7    /* Privacy-safe requested-delay histogram */
#define SQL_SAMPLE_CAPACITY 4096      /* Main/worker rolling query latency window */
#define SQL_FAMILY_CAPACITY 128       /* Bounded normalized owner/verb/table registry */
#define SQL_FAMILY_NAME_SIZE 80       /* Includes terminating NUL */
#define SQL_FAMILY_REPORT_LIMIT 16    /* Maximum normalized query-family rows */
#define SLOW_PULSE_CAPACITY 128       /* Bounded over-budget flight recorder */
#define SLOW_PULSE_DEFAULT_COUNT 10   /* Default newest slow pulses to print */
#define SLOW_PULSE_SECTION_LIMIT 12   /* Largest profiled sections retained per pulse */
#define ENTITY_VNUM_CAPACITY 8192     /* Fixed open-addressed prototype counters */
#define ENTITY_ZONE_CAPACITY 2048     /* Fixed open-addressed zone counters */
#define ENTITY_REPORT_LIMIT 12        /* Maximum ranked lifecycle rows */
#define MEMORY_SAMPLE_CAPACITY 1440   /* One day of minute-resolution history */
#define COMBAT_SLOW_CAPACITY 64       /* Bounded slow/limited combat callback records */
#define COMBAT_SLOW_DEFAULT_COUNT 10  /* Default newest combat records to print */
#define COMBAT_SLOW_USEC 100000       /* Ordinary callback latency objective */
#define COMBAT_ATTACK_LIMIT 128       /* Maximum hit() entries in one callback */
#define COMBAT_PROC_LIMIT 128         /* Maximum combat special dispatches per callback */
#define COPYOVER_SNAPSHOT_BUFFER_SIZE (2U * 1024U * 1024U)

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
  uint64_t pulse_scheduled;
  uint64_t pulse_cancelled;
  uint64_t pulse_rescheduled;
  uint64_t pulse_calls;
  uint64_t pulse_total_usec;
  uint64_t pulse_max_usec;
  uint64_t total_scheduled;
  uint64_t total_cancelled;
  uint64_t total_rescheduled;
  uint64_t total_calls;
  uint64_t total_usec;
  uint64_t total_max_usec;
  uint64_t samples[EVENT_SAMPLE_CAPACITY];
  size_t sample_index;
  size_t sample_count;
  uint64_t samples_seen;
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
  uint64_t max_callbacks_per_call;
};

struct perf_event_lifecycle_stats
{
  uint64_t scheduled;
  uint64_t cancelled;
  uint64_t rescheduled;
  uint64_t delay_buckets[EVENT_DELAY_BUCKET_COUNT];
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

struct perf_sql_rollup
{
  uint64_t calls;
  uint64_t total_usec;
  uint64_t max_usec;
  uint64_t errors;
  uint64_t samples[SQL_SAMPLE_CAPACITY];
  size_t sample_index;
  size_t sample_count;
  uint64_t samples_seen;
};

struct perf_sql_family
{
  char identity[SQL_FAMILY_NAME_SIZE];
  uint64_t calls;
  uint64_t total_usec;
  uint64_t max_usec;
  uint64_t errors;
};

struct perf_slow_section
{
  char identity[64];
  uint64_t elapsed_usec;
};

struct perf_slow_pulse
{
  uint64_t wall_timestamp_sec;
  uint64_t monotonic_timestamp_usec;
  uint64_t pulse_number;
  uint64_t duration_usec;
  uint64_t schedule_flags;
  uint64_t sql_queries;
  uint64_t sql_usec;
  uint64_t event_callbacks;
  uint64_t slowest_event_usec;
  char slowest_event[EVENT_PROFILE_NAME_SIZE];
  uint64_t descriptors;
  uint64_t characters;
  uint64_t mobiles;
  uint64_t objects;
  uint64_t events;
  uint64_t pending_extractions;
  uint64_t entity_sample_age_sec;
  uint64_t requested_missed;
  uint64_t replayed_missed;
  uint64_t dropped_missed;
  struct perf_slow_section sections[SLOW_PULSE_SECTION_LIMIT];
  size_t section_count;
};

struct perf_entity_counter
{
  int key;
  int used;
  uint64_t created;
  uint64_t extracted;
};

struct perf_entity_zone_counter
{
  int key;
  int used;
  uint64_t mobiles_created;
  uint64_t mobiles_extracted;
  uint64_t objects_created;
  uint64_t objects_extracted;
  uint64_t resets;
  uint64_t reset_total_usec;
  uint64_t reset_max_usec;
  uint64_t reset_mobiles_created;
  uint64_t reset_mobiles_extracted;
  uint64_t reset_objects_created;
  uint64_t reset_objects_extracted;
};

struct perf_entity_reason_counter
{
  uint64_t mobiles_created;
  uint64_t mobiles_extracted;
  uint64_t objects_created;
  uint64_t objects_extracted;
};

struct perf_memory_sample
{
  struct perf_memory_stats stats;
  uint64_t mobiles_created;
  uint64_t mobiles_extracted;
  uint64_t objects_created;
  uint64_t objects_extracted;
  uint64_t queries;
  uint64_t pulses_over_100;
  uint64_t pulses_over_500;
};

struct perf_memory_slope
{
  uint64_t elapsed_sec;
  double rss_kib_per_min;
  double anon_kib_per_min;
  double heap_kib_per_min;
  double mobs_per_min;
  double objects_per_min;
  double residual_heap_kib_per_min;
};

struct perf_sweep_counter
{
  uint64_t calls;
  uint64_t visited;
  uint64_t eligible;
  uint64_t acted;
  uint64_t max_visited;
  uint64_t max_eligible;
  uint64_t max_acted;
};

struct perf_combat_context
{
  int active;
  unsigned nesting;
  uint64_t start_usec;
  uint64_t wall_timestamp_sec;
  int actor_is_npc;
  int actor_class;
  int mobile_vnum;
  int room_vnum;
  uint64_t participants;
  uint64_t attacks;
  uint64_t procs;
  uint64_t rejected_attacks;
  uint64_t rejected_procs;
};

struct perf_slow_combat
{
  uint64_t wall_timestamp_sec;
  uint64_t elapsed_usec;
  int actor_is_npc;
  int actor_class;
  int mobile_vnum;
  int room_vnum;
  uint64_t participants;
  uint64_t attacks;
  uint64_t procs;
  uint64_t rejected_attacks;
  uint64_t rejected_procs;
};

/* ========================================================================
 * GLOBAL STATE
 * ======================================================================== */

/* Initialization tracking */
static int initialized = 0;
static uint64_t prof_reset_usec;
static uint64_t prof_reset_wall_time_sec;
static uint64_t logged_pulse_count;
static uint64_t missed_pulse_count;
static uint64_t vessel_message_throttled_count;
static struct perf_event_callback event_profiles[EVENT_PROFILE_CAPACITY];
static size_t event_profile_count;
static struct perf_event_callback event_profile_overflow;
static struct perf_event_process_stats pulse_event_process_stats;
static struct perf_event_process_stats total_event_process_stats;
static struct perf_event_lifecycle_stats pulse_event_lifecycle_stats;
static struct perf_event_lifecycle_stats total_event_lifecycle_stats;
static const uint64_t event_delay_bucket_max[EVENT_DELAY_BUCKET_COUNT - 1] = {1U,   10U,   60U,
                                                                              600U, 6000U, 36000U};
static struct perf_extraction_stats pulse_extraction_stats;
static struct perf_extraction_stats total_extraction_stats;
static struct perf_catchup_stats pulse_catchup_stats;
static struct perf_catchup_stats total_catchup_stats;
static uint64_t pulse_schedule_flags;
static uint64_t pulse_last_heartbeat;
static uint64_t total_heartbeats_executed;
static struct perf_sql_rollup main_sql_stats;
static struct perf_sql_rollup worker_sql_stats;
static uint64_t pulse_main_sql_calls;
static uint64_t pulse_main_sql_usec;
static struct perf_sql_family sql_families[SQL_FAMILY_CAPACITY];
static size_t sql_family_count;
static uint64_t sql_family_overflow_calls;
static uint64_t sql_reconnect_attempts;
static uint64_t sql_reconnect_successes;
static uint64_t sql_reconnect_failures;
static pthread_mutex_t sql_stats_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_t perf_main_thread;
static int perf_main_thread_set;
static _Thread_local enum perf_sql_category current_sql_category = PERF_SQL_OTHER;
static _Thread_local enum perf_entity_reason current_entity_reason = PERF_ENTITY_UNKNOWN;
static struct perf_slow_pulse slow_pulses[SLOW_PULSE_CAPACITY];
static size_t slow_pulse_index;
static size_t slow_pulse_count;
static struct perf_entity_counter mobile_vnum_counters[ENTITY_VNUM_CAPACITY];
static struct perf_entity_counter object_vnum_counters[ENTITY_VNUM_CAPACITY];
static struct perf_entity_zone_counter entity_zone_counters[ENTITY_ZONE_CAPACITY];
static struct perf_entity_reason_counter entity_reason_counters[PERF_ENTITY_REASON_COUNT];
static struct perf_sweep_counter sweep_counters[PERF_SWEEP_COUNT];
static uint64_t mobiles_created_total;
static uint64_t mobiles_extracted_total;
static uint64_t objects_created_total;
static uint64_t objects_extracted_total;
static uint64_t mobile_vnum_overflow;
static uint64_t object_vnum_overflow;
static uint64_t entity_zone_overflow;
static struct perf_combat_context combat_context;
static struct perf_slow_combat slow_combats[COMBAT_SLOW_CAPACITY];
static size_t slow_combat_index;
static size_t slow_combat_count;
static uint64_t combat_callbacks;
static uint64_t combat_slow_callbacks;
static uint64_t combat_limited_callbacks;
static uint64_t combat_rejected_attacks;
static uint64_t combat_rejected_procs;

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
static struct perf_memory_stats latest_memory_stats;
static int latest_memory_stats_valid;
static uint64_t memory_boot_time_sec = 0;
static uint64_t memory_reset_time_sec = 0;
static uint64_t memory_peak_rss_kib = 0;
static uint64_t memory_peak_anon_kib = 0;
static uint64_t memory_last_alert_time_sec = 0;
static struct perf_memory_sample memory_samples[MEMORY_SAMPLE_CAPACITY];
static size_t memory_sample_index;
static size_t memory_sample_count;

static void append_memory_sample(const struct perf_memory_stats *stats);

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

static const char *sql_category_name(enum perf_sql_category category)
{
  static const char *names[PERF_SQL_CATEGORY_COUNT] = {
      "other", "account", "character", "pet", "crash_object", "house", "last_online", "artifact"};

  if (category < PERF_SQL_OTHER || category >= PERF_SQL_CATEGORY_COUNT)
    return names[PERF_SQL_OTHER];
  return names[category];
}

static const char *skip_sql_space(const char *text)
{
  while (text != NULL && *text != '\0' && isspace((unsigned char)*text))
    text++;
  return text;
}

static size_t copy_sql_word(char *destination, size_t capacity, const char *source)
{
  size_t used;

  if (destination == NULL || capacity == 0)
    return 0;
  destination[0] = '\0';
  source = skip_sql_space(source);
  if (source == NULL)
    return 0;

  while (*source == '`' || *source == '(')
    source++;
  used = 0;
  while (*source != '\0' && used + 1 < capacity &&
         (isalnum((unsigned char)*source) || *source == '_' || *source == '.'))
  {
    destination[used++] = (char)tolower((unsigned char)*source++);
  }
  destination[used] = '\0';
  return used;
}

static int sql_word_matches(const char *text, const char *word)
{
  size_t length;

  length = strlen(word);
  if (strncasecmp(text, word, length) != 0)
    return 0;
  return text[length] == '\0' || !(isalnum((unsigned char)text[length]) || text[length] == '_');
}

static const char *find_sql_word(const char *query, const char *word)
{
  const char *cursor;

  if (query == NULL || word == NULL || *word == '\0')
    return NULL;
  for (cursor = query; *cursor != '\0'; cursor++)
  {
    if ((cursor == query || !(isalnum((unsigned char)cursor[-1]) || cursor[-1] == '_')) &&
        sql_word_matches(cursor, word))
      return cursor;
  }
  return NULL;
}

static void normalize_sql_identity(char *identity, size_t capacity, const char *query,
                                   enum perf_sql_category category)
{
  char verb[16];
  char family[40];
  const char *table_start;
  const char *keyword;

  copy_sql_word(verb, sizeof(verb), query);
  if (verb[0] == '\0')
    strlcpy(verb, "unknown", sizeof(verb));

  keyword = NULL;
  if (strcmp(verb, "select") == 0 || strcmp(verb, "delete") == 0)
    keyword = "from";
  else if (strcmp(verb, "insert") == 0 || strcmp(verb, "replace") == 0)
    keyword = "into";
  else if (strcmp(verb, "update") == 0)
    keyword = "update";
  else if (strcmp(verb, "alter") == 0 || strcmp(verb, "create") == 0 || strcmp(verb, "drop") == 0)
    keyword = "table";

  family[0] = '\0';
  table_start = keyword != NULL ? find_sql_word(query, keyword) : NULL;
  if (table_start != NULL)
  {
    table_start += strlen(keyword);
    copy_sql_word(family, sizeof(family), table_start);
  }
  if (family[0] == '\0')
    strlcpy(family, "none", sizeof(family));

  snprintf(identity, capacity, "%s:%s.%s", sql_category_name(category), verb, family);
}

static void update_sql_rollup(struct perf_sql_rollup *stats, uint64_t elapsed_usec, int failed)
{
  stats->calls = saturating_add_u64(stats->calls, 1);
  stats->total_usec = saturating_add_u64(stats->total_usec, elapsed_usec);
  if (elapsed_usec > stats->max_usec)
    stats->max_usec = elapsed_usec;
  if (failed)
    stats->errors = saturating_add_u64(stats->errors, 1);
  stats->samples[stats->sample_index] = elapsed_usec;
  stats->sample_index = (stats->sample_index + 1) % SQL_SAMPLE_CAPACITY;
  if (stats->sample_count < SQL_SAMPLE_CAPACITY)
    stats->sample_count++;
  stats->samples_seen = saturating_add_u64(stats->samples_seen, 1);
}

enum perf_sql_category PERF_sql_scope_set(enum perf_sql_category category)
{
  enum perf_sql_category previous;

  previous = current_sql_category;
  if (category < PERF_SQL_OTHER || category >= PERF_SQL_CATEGORY_COUNT)
    current_sql_category = PERF_SQL_OTHER;
  else
    current_sql_category = category;
  return previous;
}

void PERF_sql_scope_restore(enum perf_sql_category category)
{
  if (category < PERF_SQL_OTHER || category >= PERF_SQL_CATEGORY_COUNT)
    current_sql_category = PERF_SQL_OTHER;
  else
    current_sql_category = category;
}

void PERF_note_sql_query(const char *query, uint64_t elapsed_usec, int failed)
{
  struct perf_sql_family *family;
  char identity[SQL_FAMILY_NAME_SIZE];
  int is_main_thread;
  size_t i;

  normalize_sql_identity(identity, sizeof(identity), query, current_sql_category);
  pthread_mutex_lock(&sql_stats_mutex);
  if (!perf_main_thread_set)
  {
    perf_main_thread = pthread_self();
    perf_main_thread_set = 1;
  }
  is_main_thread = pthread_equal(pthread_self(), perf_main_thread);
  if (is_main_thread)
  {
    update_sql_rollup(&main_sql_stats, elapsed_usec, failed);
    pulse_main_sql_calls = saturating_add_u64(pulse_main_sql_calls, 1);
    pulse_main_sql_usec = saturating_add_u64(pulse_main_sql_usec, elapsed_usec);
  }
  else
  {
    update_sql_rollup(&worker_sql_stats, elapsed_usec, failed);
  }

  family = NULL;
  for (i = 0; i < sql_family_count; i++)
  {
    if (strcmp(sql_families[i].identity, identity) == 0)
    {
      family = &sql_families[i];
      break;
    }
  }
  if (family == NULL && sql_family_count < SQL_FAMILY_CAPACITY)
  {
    family = &sql_families[sql_family_count++];
    strlcpy(family->identity, identity, sizeof(family->identity));
  }
  if (family != NULL)
  {
    family->calls = saturating_add_u64(family->calls, 1);
    family->total_usec = saturating_add_u64(family->total_usec, elapsed_usec);
    if (elapsed_usec > family->max_usec)
      family->max_usec = elapsed_usec;
    if (failed)
      family->errors = saturating_add_u64(family->errors, 1);
  }
  else
  {
    sql_family_overflow_calls = saturating_add_u64(sql_family_overflow_calls, 1);
  }
  pthread_mutex_unlock(&sql_stats_mutex);
}

void PERF_note_sql_reconnect(int succeeded)
{
  pthread_mutex_lock(&sql_stats_mutex);
  sql_reconnect_attempts = saturating_add_u64(sql_reconnect_attempts, 1);
  if (succeeded)
    sql_reconnect_successes = saturating_add_u64(sql_reconnect_successes, 1);
  else
    sql_reconnect_failures = saturating_add_u64(sql_reconnect_failures, 1);
  pthread_mutex_unlock(&sql_stats_mutex);
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

static void copy_slow_section_identity(char *destination, size_t capacity, const char *identity)
{
  size_t i;

  if (destination == NULL || capacity == 0)
    return;
  if (identity == NULL || *identity == '\0')
    identity = "unknown";

  for (i = 0; i + 1 < capacity && identity[i] != '\0'; i++)
  {
    if (identity[i] == ',' || identity[i] == ';' || identity[i] == '\r' || identity[i] == '\n')
      destination[i] = ' ';
    else
      destination[i] = identity[i];
  }
  destination[i] = '\0';
}

static void insert_slow_section(struct perf_slow_pulse *record, const char *identity,
                                uint64_t elapsed_usec)
{
  size_t position;
  size_t i;

  if (record == NULL || elapsed_usec == 0)
    return;
  position = 0;
  while (position < record->section_count &&
         record->sections[position].elapsed_usec >= elapsed_usec)
    position++;
  if (position >= SLOW_PULSE_SECTION_LIMIT)
    return;
  if (record->section_count < SLOW_PULSE_SECTION_LIMIT)
    record->section_count++;
  for (i = record->section_count - 1; i > position; i--)
    record->sections[i] = record->sections[i - 1];
  copy_slow_section_identity(record->sections[position].identity,
                             sizeof(record->sections[position].identity), identity);
  record->sections[position].elapsed_usec = elapsed_usec;
}

static void capture_slow_pulse(double usage_percent)
{
  struct perf_slow_pulse *record;
  struct perf_event_callback *profile;
  uint64_t now_sec;
  size_t i;

  if (usage_percent <= 100.0)
    return;

  record = &slow_pulses[slow_pulse_index];
  memset(record, 0, sizeof(*record));
  now_sec = (uint64_t)time(NULL);
  record->wall_timestamp_sec = now_sec;
  record->monotonic_timestamp_usec = monotonic_usec();
  record->pulse_number = pulse_last_heartbeat;
  record->duration_usec = usage_percent >= ((double)UINT64_MAX * 100.0 / (double)USEC_PER_PULSE)
                              ? UINT64_MAX
                              : (uint64_t)((usage_percent * (double)USEC_PER_PULSE) / 100.0);
  record->schedule_flags = pulse_schedule_flags;
  pthread_mutex_lock(&sql_stats_mutex);
  record->sql_queries = pulse_main_sql_calls;
  record->sql_usec = pulse_main_sql_usec;
  pthread_mutex_unlock(&sql_stats_mutex);
  record->event_callbacks = pulse_event_process_stats.callbacks_processed;

  for (i = 0; i < event_profile_count; i++)
  {
    profile = &event_profiles[i];
    if (profile->pulse_max_usec > record->slowest_event_usec)
    {
      record->slowest_event_usec = profile->pulse_max_usec;
      copy_event_identity(record->slowest_event, sizeof(record->slowest_event), profile->identity);
    }
  }
  if (event_profile_overflow.pulse_max_usec > record->slowest_event_usec)
  {
    record->slowest_event_usec = event_profile_overflow.pulse_max_usec;
    copy_event_identity(record->slowest_event, sizeof(record->slowest_event),
                        "unregistered overflow");
  }

  if (latest_memory_stats_valid)
  {
    record->descriptors = latest_memory_stats.count_descriptors;
    record->characters = latest_memory_stats.count_chars;
    record->mobiles = latest_memory_stats.count_mobs;
    record->objects = latest_memory_stats.count_objs;
    record->events = latest_memory_stats.count_events;
    record->pending_extractions = latest_memory_stats.count_pending_extractions;
    if (now_sec >= latest_memory_stats.timestamp_sec)
      record->entity_sample_age_sec = now_sec - latest_memory_stats.timestamp_sec;
  }

  record->requested_missed = pulse_catchup_stats.requested_missed;
  record->replayed_missed = pulse_catchup_stats.replayed_missed;
  record->dropped_missed = pulse_catchup_stats.remaining_backlog;
  for (i = 0; i < (size_t)prof_section_count; i++)
    insert_slow_section(record, prof_sections[i]->id, prof_sections[i]->pulse_total_usec);

  slow_pulse_index = (slow_pulse_index + 1) % SLOW_PULSE_CAPACITY;
  if (slow_pulse_count < SLOW_PULSE_CAPACITY)
    slow_pulse_count++;
}

static void reset_event_callback_pulse_stats(void)
{
  size_t i;

  for (i = 0; i < event_profile_count; i++)
  {
    event_profiles[i].pulse_scheduled = 0;
    event_profiles[i].pulse_cancelled = 0;
    event_profiles[i].pulse_rescheduled = 0;
    event_profiles[i].pulse_calls = 0;
    event_profiles[i].pulse_total_usec = 0;
    event_profiles[i].pulse_max_usec = 0;
  }
  event_profile_overflow.pulse_scheduled = 0;
  event_profile_overflow.pulse_cancelled = 0;
  event_profile_overflow.pulse_rescheduled = 0;
  event_profile_overflow.pulse_calls = 0;
  event_profile_overflow.pulse_total_usec = 0;
  event_profile_overflow.pulse_max_usec = 0;
}

static void reset_event_callback_total_stats(void)
{
  size_t i;

  for (i = 0; i < event_profile_count; i++)
  {
    event_profiles[i].total_scheduled = 0;
    event_profiles[i].total_cancelled = 0;
    event_profiles[i].total_rescheduled = 0;
    event_profiles[i].total_calls = 0;
    event_profiles[i].total_usec = 0;
    event_profiles[i].total_max_usec = 0;
    event_profiles[i].sample_index = 0;
    event_profiles[i].sample_count = 0;
    event_profiles[i].samples_seen = 0;
  }
  event_profile_overflow.total_scheduled = 0;
  event_profile_overflow.total_cancelled = 0;
  event_profile_overflow.total_rescheduled = 0;
  event_profile_overflow.total_calls = 0;
  event_profile_overflow.total_usec = 0;
  event_profile_overflow.total_max_usec = 0;
  event_profile_overflow.sample_index = 0;
  event_profile_overflow.sample_count = 0;
  event_profile_overflow.samples_seen = 0;
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
  if (callbacks_processed > stats->max_callbacks_per_call)
    stats->max_callbacks_per_call = callbacks_processed;
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
  prof_reset_wall_time_sec = (uint64_t)time(NULL);

  if (memory_boot_time_sec == 0)
  {
    PERF_sample_memory(&boot_memory_stats);
    reset_memory_stats = boot_memory_stats;
    memory_boot_time_sec = (uint64_t)time(NULL);
    memory_reset_time_sec = memory_boot_time_sec;
    append_memory_sample(&boot_memory_stats);
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
  pulse_schedule_flags = 0;
  pulse_last_heartbeat = 0;
  total_heartbeats_executed = 0;
  memset(slow_pulses, 0, sizeof(slow_pulses));
  slow_pulse_index = 0;
  slow_pulse_count = 0;
  memset(mobile_vnum_counters, 0, sizeof(mobile_vnum_counters));
  memset(object_vnum_counters, 0, sizeof(object_vnum_counters));
  memset(entity_zone_counters, 0, sizeof(entity_zone_counters));
  memset(entity_reason_counters, 0, sizeof(entity_reason_counters));
  memset(sweep_counters, 0, sizeof(sweep_counters));
  mobiles_created_total = 0;
  mobiles_extracted_total = 0;
  objects_created_total = 0;
  objects_extracted_total = 0;
  mobile_vnum_overflow = 0;
  object_vnum_overflow = 0;
  entity_zone_overflow = 0;
  active_world_reset_telemetry();
  periodic_owners_reset_telemetry();
  affected_owners_reset_telemetry();
  character_periodic_reset_telemetry();
  memset(&combat_context, 0, sizeof(combat_context));
  memset(slow_combats, 0, sizeof(slow_combats));
  slow_combat_index = 0;
  slow_combat_count = 0;
  combat_callbacks = 0;
  combat_slow_callbacks = 0;
  combat_limited_callbacks = 0;
  combat_rejected_attacks = 0;
  combat_rejected_procs = 0;
  current_entity_reason = PERF_ENTITY_UNKNOWN;
  pthread_mutex_lock(&sql_stats_mutex);
  memset(&main_sql_stats, 0, sizeof(main_sql_stats));
  memset(&worker_sql_stats, 0, sizeof(worker_sql_stats));
  pulse_main_sql_calls = 0;
  pulse_main_sql_usec = 0;
  memset(sql_families, 0, sizeof(sql_families));
  sql_family_count = 0;
  sql_family_overflow_calls = 0;
  sql_reconnect_attempts = 0;
  sql_reconnect_successes = 0;
  sql_reconnect_failures = 0;
  perf_main_thread = pthread_self();
  perf_main_thread_set = 1;
  pthread_mutex_unlock(&sql_stats_mutex);
  reset_event_callback_pulse_stats();
  reset_event_callback_total_stats();
  memset(&pulse_event_process_stats, 0, sizeof(pulse_event_process_stats));
  memset(&total_event_process_stats, 0, sizeof(total_event_process_stats));
  memset(&pulse_event_lifecycle_stats, 0, sizeof(pulse_event_lifecycle_stats));
  memset(&total_event_lifecycle_stats, 0, sizeof(total_event_lifecycle_stats));
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
  memset(memory_samples, 0, sizeof(memory_samples));
  memory_sample_index = 0;
  memory_sample_count = 0;
  append_memory_sample(&reset_memory_stats);
  memory_reset_time_sec = (uint64_t)time(NULL);
  if (memory_boot_time_sec == 0)
  {
    boot_memory_stats = reset_memory_stats;
    memory_boot_time_sec = memory_reset_time_sec;
  }

  prof_reset_usec = monotonic_usec();
  prof_reset_wall_time_sec = (uint64_t)time(NULL);
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
  capture_slow_pulse(val);

  /* Add to pulse data buffer */
  add_interval_data(&pulse_data, val, val, val);

  /* Check for aggregation */
  aggregate_data();
}

void PERF_note_heartbeat(uint64_t pulse_number)
{
  pulse_last_heartbeat = pulse_number;
  total_heartbeats_executed = saturating_add_u64(total_heartbeats_executed, 1);
}

void PERF_note_schedule(uint64_t schedule_flags)
{
  pulse_schedule_flags |= schedule_flags;
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

static struct perf_event_callback *event_profile_for_index(int profile_index)
{
  if (profile_index < 0 || (size_t)profile_index >= event_profile_count)
    return &event_profile_overflow;
  return &event_profiles[profile_index];
}

static void note_event_delay(struct perf_event_lifecycle_stats *stats, uint64_t delay_pulses)
{
  size_t bucket;

  bucket = 0;
  while (bucket < EVENT_DELAY_BUCKET_COUNT - 1 && delay_pulses > event_delay_bucket_max[bucket])
    bucket++;
  stats->delay_buckets[bucket] = saturating_add_u64(stats->delay_buckets[bucket], 1);
}

void PERF_note_event_scheduled(int profile_index, uint64_t delay_pulses)
{
  struct perf_event_callback *profile;

  ensure_initialized();
  profile = event_profile_for_index(profile_index);
  profile->pulse_scheduled = saturating_add_u64(profile->pulse_scheduled, 1);
  profile->total_scheduled = saturating_add_u64(profile->total_scheduled, 1);
  pulse_event_lifecycle_stats.scheduled =
      saturating_add_u64(pulse_event_lifecycle_stats.scheduled, 1);
  total_event_lifecycle_stats.scheduled =
      saturating_add_u64(total_event_lifecycle_stats.scheduled, 1);
  note_event_delay(&pulse_event_lifecycle_stats, delay_pulses);
  note_event_delay(&total_event_lifecycle_stats, delay_pulses);
}

void PERF_note_event_cancelled(int profile_index)
{
  struct perf_event_callback *profile;

  ensure_initialized();
  profile = event_profile_for_index(profile_index);
  profile->pulse_cancelled = saturating_add_u64(profile->pulse_cancelled, 1);
  profile->total_cancelled = saturating_add_u64(profile->total_cancelled, 1);
  pulse_event_lifecycle_stats.cancelled =
      saturating_add_u64(pulse_event_lifecycle_stats.cancelled, 1);
  total_event_lifecycle_stats.cancelled =
      saturating_add_u64(total_event_lifecycle_stats.cancelled, 1);
}

void PERF_note_event_rescheduled(int profile_index, uint64_t delay_pulses)
{
  struct perf_event_callback *profile;

  ensure_initialized();
  profile = event_profile_for_index(profile_index);
  profile->pulse_rescheduled = saturating_add_u64(profile->pulse_rescheduled, 1);
  profile->total_rescheduled = saturating_add_u64(profile->total_rescheduled, 1);
  pulse_event_lifecycle_stats.rescheduled =
      saturating_add_u64(pulse_event_lifecycle_stats.rescheduled, 1);
  total_event_lifecycle_stats.rescheduled =
      saturating_add_u64(total_event_lifecycle_stats.rescheduled, 1);
  note_event_delay(&pulse_event_lifecycle_stats, delay_pulses);
  note_event_delay(&total_event_lifecycle_stats, delay_pulses);
}

void PERF_note_event_callback(int profile_index, uint64_t elapsed_usec)
{
  struct perf_event_callback *profile;

  profile = event_profile_for_index(profile_index);

  profile->pulse_calls = saturating_add_u64(profile->pulse_calls, 1);
  profile->pulse_total_usec = saturating_add_u64(profile->pulse_total_usec, elapsed_usec);
  profile->total_calls = saturating_add_u64(profile->total_calls, 1);
  profile->total_usec = saturating_add_u64(profile->total_usec, elapsed_usec);
  if (elapsed_usec > profile->pulse_max_usec)
    profile->pulse_max_usec = elapsed_usec;
  if (elapsed_usec > profile->total_max_usec)
    profile->total_max_usec = elapsed_usec;
  profile->samples[profile->sample_index] = elapsed_usec;
  profile->sample_index = (profile->sample_index + 1) % EVENT_SAMPLE_CAPACITY;
  if (profile->sample_count < EVENT_SAMPLE_CAPACITY)
    profile->sample_count++;
  profile->samples_seen = saturating_add_u64(profile->samples_seen, 1);
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
  size_t sampled_section_count;
  size_t sample_bytes;
  uint64_t now_usec;
  uint64_t elapsed_usec;
  uint64_t expected_slots;
  double total_pulses;
  double pulse_min, sec_min, min_min, hour_min;
  char reset_time[32];
  time_t reset_wall_time;
  struct tm reset_tm;

  if (!out_buf || n < 1)
    return 0;

  ensure_initialized();

  sampled_section_count = 0;
  for (i = 0; i < (size_t)prof_section_count; i++)
    if (prof_sections[i]->sampling_enabled)
      sampled_section_count++;
  sample_bytes = sampled_section_count * PROF_SAMPLE_CAPACITY * sizeof(uint64_t);

  total_pulses = (double)logged_pulse_count;

  /* Get minimum values */
  pulse_min = get_interval_min(&pulse_data);
  sec_min = get_interval_min(&sec_data);
  min_min = get_interval_min(&min_data);
  hour_min = get_interval_min(&hour_data);

  now_usec = monotonic_usec();
  elapsed_usec = now_usec >= prof_reset_usec ? now_usec - prof_reset_usec : 0;
  expected_slots = USEC_PER_PULSE > 0 ? elapsed_usec / USEC_PER_PULSE : 0;
  reset_wall_time = (time_t)prof_reset_wall_time_sec;
  if (gmtime_r(&reset_wall_time, &reset_tm) != NULL)
    strftime(reset_time, sizeof(reset_time), "%Y-%m-%d %H:%M:%S UTC", &reset_tm);
  else
    strlcpy(reset_time, "unknown", sizeof(reset_time));

  /* Format the report */
  written = bounded_format_length(
      snprintf(
          out_buf, n,
          "Measurement started: %s\n\r"
          "Elapsed: %.2f seconds | Pulse budget: %.2f ms | Logged outer loops: %" PRIu64
          " | Expected slots: %" PRIu64 " | Executed heartbeats: %" PRIu64 "\n\r"
          "Catch-up: requested=%" PRIu64 " replayed=%" PRIu64 " dropped=%" PRIu64 "\n\r"
          "Rolling completed windows (percent of %.2f ms pulse budget)\n\r"
          "                     Avg         Min         Max\n\r"
          "  1 Pulse:   %10.2f%% %10.2f%% %10.2f%%\n\r"
          "%3zu Pulses:  %10.2f%% %10.2f%% %10.2f%%\n\r"
          "%3zu Seconds: %10.2f%% %10.2f%% %10.2f%%\n\r"
          "%3zu Minutes: %10.2f%% %10.2f%% %10.2f%%\n\r"
          "%3zu Hours:   %10.2f%% %10.2f%% %10.2f%%\n\r"
          "\n\rMax pulse:      %.2f ms (%.2f%%)\n\r\n\r",
          reset_time, (double)elapsed_usec / (double)USEC_PER_SEC, (double)USEC_PER_PULSE / 1000.0,
          logged_pulse_count, expected_slots, total_heartbeats_executed,
          total_catchup_stats.requested_missed, total_catchup_stats.replayed_missed,
          total_catchup_stats.remaining_backlog, (double)USEC_PER_PULSE / 1000.0, last_pulse,
          last_pulse, last_pulse, pulse_data.count, get_interval_avg(&pulse_data), pulse_min,
          get_interval_max(&pulse_data), sec_data.count, get_interval_avg(&sec_data), sec_min,
          get_interval_max(&sec_data), min_data.count, get_interval_avg(&min_data), min_min,
          get_interval_max(&min_data), hour_data.count, get_interval_avg(&hour_data), hour_min,
          get_interval_max(&hour_data), (max_pulse * (double)USEC_PER_PULSE) / 100000.0, max_pulse),
      n);

  /* Add threshold statistics */
  for (i = 0; (size_t)i < sizeof(thresholds) / sizeof(thresholds[0]) && written < n - 1; i++)
  {
    double percent = (total_pulses > 0) ? (100.0 * thresholds[i].count / total_pulses) : 0.0;

    written += bounded_format_length(
        snprintf(out_buf + written, n - written, "Over %5d%% (%7.1f ms): %.2f%% (%lu)\n\r",
                 thresholds[i].threshold,
                 ((double)thresholds[i].threshold * (double)USEC_PER_PULSE) / 100000.0, percent,
                 thresholds[i].count),
        n - written);
  }

  if (written < n - 1)
  {
    written += bounded_format_length(
        snprintf(out_buf + written, n - written,
                 "PERFMON overhead: sections=%d sampled=%zu sample_bytes=%zu "
                 "event_registry_bytes=%zu slow_ring_bytes=%zu sql_bytes=%zu\n\r",
                 prof_section_count, sampled_section_count, sample_bytes, sizeof(event_profiles),
                 sizeof(slow_pulses),
                 sizeof(main_sql_stats) + sizeof(worker_sql_stats) + sizeof(sql_families)),
        n - written);
  }

  return written;
}

static size_t append_schedule_name(char *buf, size_t n, size_t written, const char *name)
{
  if (written >= n - 1)
    return written;
  if (written > 0)
    written += bounded_format_length(snprintf(buf + written, n - written, "+"), n - written);
  if (written < n - 1)
    written += bounded_format_length(snprintf(buf + written, n - written, "%s", name), n - written);
  return written;
}

static void format_schedule_flags(char *buf, size_t n, uint64_t flags)
{
  size_t written;

  if (buf == NULL || n == 0)
    return;
  buf[0] = '\0';
  written = 0;
  if (flags & PERF_SCHEDULE_1_SECOND)
    written = append_schedule_name(buf, n, written, "1s");
  if (flags & PERF_SCHEDULE_3_SECONDS)
    written = append_schedule_name(buf, n, written, "3s");
  if (flags & PERF_SCHEDULE_5_SECONDS)
    written = append_schedule_name(buf, n, written, "5s");
  if (flags & PERF_SCHEDULE_6_SECONDS)
    written = append_schedule_name(buf, n, written, "6s");
  if (flags & PERF_SCHEDULE_13_SECONDS)
    written = append_schedule_name(buf, n, written, "13s");
  if (flags & PERF_SCHEDULE_30_SECONDS)
    written = append_schedule_name(buf, n, written, "30s");
  if (flags & PERF_SCHEDULE_60_SECONDS)
    written = append_schedule_name(buf, n, written, "60s");
  if (flags & PERF_SCHEDULE_75_SECONDS)
    written = append_schedule_name(buf, n, written, "75s");
  if (flags & PERF_SCHEDULE_AUTOSAVE)
    written = append_schedule_name(buf, n, written, "autosave");
  if (flags & PERF_SCHEDULE_LONG_INTERVAL)
    written = append_schedule_name(buf, n, written, "long");
  if (written == 0)
    strlcpy(buf, "base", n);
}

size_t PERF_slow_repr(char *out_buf, size_t n, size_t count, int csv)
{
  const struct perf_slow_pulse *record;
  char schedule[96];
  char timestamp[32];
  char sections[1024];
  struct tm timestamp_tm;
  time_t wall_time;
  size_t available;
  size_t index;
  size_t offset;
  size_t section_index;
  size_t section_written;
  size_t written;

  if (out_buf == NULL || n == 0)
    return 0;
  ensure_initialized();
  available = slow_pulse_count;
  if (count == 0)
    count = SLOW_PULSE_DEFAULT_COUNT;
  if (count > available)
    count = available;

  if (csv)
  {
    written = bounded_format_length(
        snprintf(out_buf, n,
                 "timestamp_utc,monotonic_usec,pulse,duration_usec,schedules,sql_queries,sql_usec,"
                 "event_callbacks,slowest_event,slowest_event_usec,descriptors,characters,mobiles,"
                 "objects,events,pending_extractions,entity_sample_age_sec,requested_missed,"
                 "replayed_missed,dropped_missed,top_sections\n\r"),
        n);
  }
  else
  {
    written = bounded_format_length(
        snprintf(out_buf, n,
                 "Slow pulse flight recorder (newest first, > %.1f ms, retained %zu/%d)\n\r",
                 (double)USEC_PER_PULSE / 1000.0, slow_pulse_count, SLOW_PULSE_CAPACITY),
        n);
  }

  for (offset = 0; offset < count && written < n - 1; offset++)
  {
    index = (slow_pulse_index + SLOW_PULSE_CAPACITY - 1 - offset) % SLOW_PULSE_CAPACITY;
    record = &slow_pulses[index];
    format_schedule_flags(schedule, sizeof(schedule), record->schedule_flags);
    wall_time = (time_t)record->wall_timestamp_sec;
    if (gmtime_r(&wall_time, &timestamp_tm) != NULL)
      strftime(timestamp, sizeof(timestamp), "%Y-%m-%dT%H:%M:%SZ", &timestamp_tm);
    else
      strlcpy(timestamp, "unknown", sizeof(timestamp));

    sections[0] = '\0';
    section_written = 0;
    for (section_index = 0; section_index < record->section_count; section_index++)
    {
      section_written += bounded_format_length(
          snprintf(sections + section_written, sizeof(sections) - section_written, "%s%s:%" PRIu64,
                   section_index == 0 ? "" : "|", record->sections[section_index].identity,
                   record->sections[section_index].elapsed_usec),
          sizeof(sections) - section_written);
      if (section_written >= sizeof(sections) - 1)
        break;
    }

    if (csv)
    {
      written += bounded_format_length(
          snprintf(out_buf + written, n - written,
                   "%s,%" PRIu64 ",%" PRIu64 ",%" PRIu64 ",%s,%" PRIu64 ",%" PRIu64 ",%" PRIu64
                   ",%s,%" PRIu64 ",%" PRIu64 ",%" PRIu64 ",%" PRIu64 ",%" PRIu64 ",%" PRIu64
                   ",%" PRIu64 ",%" PRIu64 ",%" PRIu64 ",%" PRIu64 ",%" PRIu64 ",%s\n\r",
                   timestamp, record->monotonic_timestamp_usec, record->pulse_number,
                   record->duration_usec, schedule, record->sql_queries, record->sql_usec,
                   record->event_callbacks,
                   record->slowest_event[0] != '\0' ? record->slowest_event : "none",
                   record->slowest_event_usec, record->descriptors, record->characters,
                   record->mobiles, record->objects, record->events, record->pending_extractions,
                   record->entity_sample_age_sec, record->requested_missed, record->replayed_missed,
                   record->dropped_missed, sections[0] != '\0' ? sections : "none"),
          n - written);
    }
    else
    {
      written += bounded_format_length(
          snprintf(
              out_buf + written, n - written,
              "%s pulse=%" PRIu64 " duration=%.3f ms schedules=%s SQL=%" PRIu64
              "/%.3f ms events=%" PRIu64 " slowest=%s/%.3f ms entities=%" PRIu64 " chars (%" PRIu64
              " mobs), %" PRIu64 " objs, %" PRIu64 " descriptors, %" PRIu64 " events, %" PRIu64
              " pending (sample_age=%" PRIu64 "s) catchup=%" PRIu64 "/%" PRIu64 "/%" PRIu64
              "\n\r  sections: %s\n\r",
              timestamp, record->pulse_number, (double)record->duration_usec / 1000.0, schedule,
              record->sql_queries, (double)record->sql_usec / 1000.0, record->event_callbacks,
              record->slowest_event[0] != '\0' ? record->slowest_event : "none",
              (double)record->slowest_event_usec / 1000.0, record->characters, record->mobiles,
              record->objects, record->descriptors, record->events, record->pending_extractions,
              record->entity_sample_age_sec, record->requested_missed, record->replayed_missed,
              record->dropped_missed, sections[0] != '\0' ? sections : "none"),
          n - written);
    }
  }

  return written;
}

static size_t collect_top_sql_families(const struct perf_sql_family *families, size_t family_count,
                                       size_t *indices)
{
  size_t top_count;
  size_t i;
  size_t j;
  size_t position;

  top_count = 0;
  for (i = 0; i < family_count; i++)
  {
    position = 0;
    while (position < top_count && families[indices[position]].total_usec >= families[i].total_usec)
      position++;
    if (position >= SQL_FAMILY_REPORT_LIMIT)
      continue;
    if (top_count < SQL_FAMILY_REPORT_LIMIT)
      top_count++;
    for (j = top_count - 1; j > position; j--)
      indices[j] = indices[j - 1];
    indices[position] = i;
  }
  return top_count;
}

size_t PERF_sql_repr(char *out_buf, size_t n, int csv)
{
  struct perf_sql_rollup main_snapshot;
  struct perf_sql_rollup worker_snapshot;
  struct perf_sql_family family_snapshot[SQL_FAMILY_CAPACITY];
  const struct perf_sql_family *family;
  size_t top_indices[SQL_FAMILY_REPORT_LIMIT];
  size_t family_count;
  size_t top_count;
  size_t written;
  size_t i;
  uint64_t overflow_calls;
  uint64_t reconnect_attempts;
  uint64_t reconnect_successes;
  uint64_t reconnect_failures;
  double main_median;
  double main_p95;
  double main_p99;
  double worker_median;
  double worker_p95;
  double worker_p99;

  if (out_buf == NULL || n == 0)
    return 0;
  pthread_mutex_lock(&sql_stats_mutex);
  main_snapshot = main_sql_stats;
  worker_snapshot = worker_sql_stats;
  family_count = sql_family_count;
  memcpy(family_snapshot, sql_families, family_count * sizeof(*family_snapshot));
  overflow_calls = sql_family_overflow_calls;
  reconnect_attempts = sql_reconnect_attempts;
  reconnect_successes = sql_reconnect_successes;
  reconnect_failures = sql_reconnect_failures;
  pthread_mutex_unlock(&sql_stats_mutex);

  calculate_percentile_set(main_snapshot.samples, main_snapshot.sample_count, &main_median,
                           &main_p95, &main_p99);
  calculate_percentile_set(worker_snapshot.samples, worker_snapshot.sample_count, &worker_median,
                           &worker_p95, &worker_p99);
  top_count = collect_top_sql_families(family_snapshot, family_count, top_indices);

  if (csv)
  {
    written = bounded_format_length(
        snprintf(out_buf, n,
                 "sql_thread,calls,total_usec,average_usec,median_usec,p95_usec,p99_usec,max_usec,"
                 "errors,samples_stored,samples_seen\n\r"
                 "main,%" PRIu64 ",%" PRIu64 ",%.2f,%.2f,%.2f,%.2f,%" PRIu64 ",%" PRIu64
                 ",%zu,%" PRIu64 "\n\r"
                 "worker,%" PRIu64 ",%" PRIu64 ",%.2f,%.2f,%.2f,%.2f,%" PRIu64 ",%" PRIu64
                 ",%zu,%" PRIu64 "\n\r"
                 "# sql_reconnect_attempts=%" PRIu64 "\n\r"
                 "# sql_reconnect_successes=%" PRIu64 "\n\r"
                 "# sql_reconnect_failures=%" PRIu64 "\n\r"
                 "sql_family,calls,total_usec,average_usec,max_usec,errors\n\r",
                 main_snapshot.calls, main_snapshot.total_usec,
                 main_snapshot.calls > 0
                     ? (double)main_snapshot.total_usec / (double)main_snapshot.calls
                     : 0.0,
                 main_median, main_p95, main_p99, main_snapshot.max_usec, main_snapshot.errors,
                 main_snapshot.sample_count, main_snapshot.samples_seen, worker_snapshot.calls,
                 worker_snapshot.total_usec,
                 worker_snapshot.calls > 0
                     ? (double)worker_snapshot.total_usec / (double)worker_snapshot.calls
                     : 0.0,
                 worker_median, worker_p95, worker_p99, worker_snapshot.max_usec,
                 worker_snapshot.errors, worker_snapshot.sample_count, worker_snapshot.samples_seen,
                 reconnect_attempts, reconnect_successes, reconnect_failures),
        n);
  }
  else
  {
    written = bounded_format_length(
        snprintf(out_buf, n,
                 "SQL Telemetry (rolling percentiles cover the newest %d calls per thread)\n\r"
                 "Thread |    Calls|  Total usec|  Avg usec|Median usec|  P95 usec|  P99 usec|"
                 "  Max usec| Errors|Samples stored/seen\n\r"
                 "main   |%9" PRIu64 "|%12" PRIu64 "|%10.2f|%11.2f|%10.2f|%10.2f|%10" PRIu64
                 "|%7" PRIu64 "|%7zu/%-7" PRIu64 "\n\r"
                 "worker |%9" PRIu64 "|%12" PRIu64 "|%10.2f|%11.2f|%10.2f|%10.2f|%10" PRIu64
                 "|%7" PRIu64 "|%7zu/%-7" PRIu64 "\n\r"
                 "Reconnects: attempts=%" PRIu64 " successes=%" PRIu64 " failures=%" PRIu64 "\n\r"
                 "Normalized query families (top %d by elapsed time, registered=%zu/%d, "
                 "overflow_calls=%" PRIu64 ")\n\r"
                 "Family                                                                          |"
                 "    Calls|  Total usec|  Avg usec|  Max usec| Errors\n\r",
                 SQL_SAMPLE_CAPACITY, main_snapshot.calls, main_snapshot.total_usec,
                 main_snapshot.calls > 0
                     ? (double)main_snapshot.total_usec / (double)main_snapshot.calls
                     : 0.0,
                 main_median, main_p95, main_p99, main_snapshot.max_usec, main_snapshot.errors,
                 main_snapshot.sample_count, main_snapshot.samples_seen, worker_snapshot.calls,
                 worker_snapshot.total_usec,
                 worker_snapshot.calls > 0
                     ? (double)worker_snapshot.total_usec / (double)worker_snapshot.calls
                     : 0.0,
                 worker_median, worker_p95, worker_p99, worker_snapshot.max_usec,
                 worker_snapshot.errors, worker_snapshot.sample_count, worker_snapshot.samples_seen,
                 reconnect_attempts, reconnect_successes, reconnect_failures,
                 SQL_FAMILY_REPORT_LIMIT, family_count, SQL_FAMILY_CAPACITY, overflow_calls),
        n);
  }

  for (i = 0; i < top_count && written < n - 1; i++)
  {
    family = &family_snapshot[top_indices[i]];
    if (csv)
    {
      written += bounded_format_length(
          snprintf(out_buf + written, n - written,
                   "%s,%" PRIu64 ",%" PRIu64 ",%.2f,%" PRIu64 ",%" PRIu64 "\n\r", family->identity,
                   family->calls, family->total_usec,
                   family->calls > 0 ? (double)family->total_usec / (double)family->calls : 0.0,
                   family->max_usec, family->errors),
          n - written);
    }
    else
    {
      written += bounded_format_length(
          snprintf(out_buf + written, n - written,
                   "%-80s|%9" PRIu64 "|%12" PRIu64 "|%10.2f|%10" PRIu64 "|%7" PRIu64 "\n\r",
                   family->identity, family->calls, family->total_usec,
                   family->calls > 0 ? (double)family->total_usec / (double)family->calls : 0.0,
                   family->max_usec, family->errors),
          n - written);
    }
  }
  return written;
}

/* ========================================================================
 * COMBAT CALLBACK MONITORING
 * ======================================================================== */

void PERF_combat_round_begin(const struct char_data *ch)
{
  const struct char_data *participant;

  ensure_initialized();
  if (combat_context.active)
  {
    combat_context.nesting++;
    return;
  }

  memset(&combat_context, 0, sizeof(combat_context));
  combat_context.active = TRUE;
  combat_context.start_usec = monotonic_usec();
  combat_context.wall_timestamp_sec = (uint64_t)time(NULL);
  combat_context.mobile_vnum = -1;
  combat_context.room_vnum = -1;
  if (ch == NULL)
    return;

  combat_context.actor_is_npc = IS_NPC(ch) ? TRUE : FALSE;
  combat_context.actor_class = GET_CLASS(ch);
  if (IS_NPC(ch))
    combat_context.mobile_vnum = GET_MOB_VNUM(ch);
  if (IN_ROOM(ch) == NOWHERE || world == NULL || IN_ROOM(ch) > top_of_world)
    return;

  combat_context.room_vnum = GET_ROOM_VNUM(IN_ROOM(ch));
  for (participant = world[IN_ROOM(ch)].people; participant != NULL;
       participant = participant->next_in_room)
    combat_context.participants++;
}

int PERF_combat_allow_attack(void)
{
  if (!combat_context.active)
    return TRUE;
  if (combat_context.attacks >= COMBAT_ATTACK_LIMIT)
  {
    combat_context.rejected_attacks++;
    return FALSE;
  }
  combat_context.attacks++;
  return TRUE;
}

int PERF_combat_allow_proc(void)
{
  if (!combat_context.active)
    return TRUE;
  if (combat_context.procs >= COMBAT_PROC_LIMIT)
  {
    combat_context.rejected_procs++;
    return FALSE;
  }
  combat_context.procs++;
  return TRUE;
}

void PERF_combat_round_end(void)
{
  struct perf_slow_combat *record;
  uint64_t elapsed_usec;
  uint64_t now_usec;
  int limited;

  if (!combat_context.active)
    return;
  if (combat_context.nesting > 0)
  {
    combat_context.nesting--;
    return;
  }

  now_usec = monotonic_usec();
  elapsed_usec = now_usec >= combat_context.start_usec ? now_usec - combat_context.start_usec : 0;
  limited = combat_context.rejected_attacks > 0 || combat_context.rejected_procs > 0;
  combat_callbacks = saturating_add_u64(combat_callbacks, 1);
  if (elapsed_usec >= COMBAT_SLOW_USEC)
    combat_slow_callbacks = saturating_add_u64(combat_slow_callbacks, 1);
  if (limited)
    combat_limited_callbacks = saturating_add_u64(combat_limited_callbacks, 1);
  combat_rejected_attacks =
      saturating_add_u64(combat_rejected_attacks, combat_context.rejected_attacks);
  combat_rejected_procs = saturating_add_u64(combat_rejected_procs, combat_context.rejected_procs);

  if (elapsed_usec >= COMBAT_SLOW_USEC || limited)
  {
    record = &slow_combats[slow_combat_index];
    record->wall_timestamp_sec = combat_context.wall_timestamp_sec;
    record->elapsed_usec = elapsed_usec;
    record->actor_is_npc = combat_context.actor_is_npc;
    record->actor_class = combat_context.actor_class;
    record->mobile_vnum = combat_context.mobile_vnum;
    record->room_vnum = combat_context.room_vnum;
    record->participants = combat_context.participants;
    record->attacks = combat_context.attacks;
    record->procs = combat_context.procs;
    record->rejected_attacks = combat_context.rejected_attacks;
    record->rejected_procs = combat_context.rejected_procs;
    slow_combat_index = (slow_combat_index + 1) % COMBAT_SLOW_CAPACITY;
    if (slow_combat_count < COMBAT_SLOW_CAPACITY)
      slow_combat_count++;
  }
  memset(&combat_context, 0, sizeof(combat_context));
}

size_t PERF_combat_repr(char *out_buf, size_t n, size_t count, int csv)
{
  const struct perf_slow_combat *record;
  size_t index;
  size_t available;
  size_t written;
  size_t i;

  if (out_buf == NULL || n == 0)
    return 0;
  available = slow_combat_count;
  if (count == 0)
    count = COMBAT_SLOW_DEFAULT_COUNT;
  if (count > available)
    count = available;

  if (csv)
    written = bounded_format_length(
        snprintf(out_buf, n,
                 "combat_timestamp,elapsed_usec,actor_kind,class,mob_vnum,room_vnum,participants,"
                 "attacks,procs,rejected_attacks,rejected_procs\n\r"),
        n);
  else
    written = bounded_format_length(
        snprintf(out_buf, n,
                 "Combat callback telemetry since reset\n\r"
                 "Callbacks=%" PRIu64 " slow_over_100ms=%" PRIu64 " limited=%" PRIu64
                 " rejected_attacks=%" PRIu64 " rejected_procs=%" PRIu64 "\n\r"
                 "Limits: attacks=%d procs=%d; retained slow/limited callbacks=%zu/%d\n\r"
                 "Timestamp |Elapsed ms|Kind|Class| Mob VNUM|Room VNUM|People|Attacks|Procs|"
                 "Rejected A/P\n\r",
                 combat_callbacks, combat_slow_callbacks, combat_limited_callbacks,
                 combat_rejected_attacks, combat_rejected_procs, COMBAT_ATTACK_LIMIT,
                 COMBAT_PROC_LIMIT, slow_combat_count, COMBAT_SLOW_CAPACITY),
        n);

  for (i = 0; i < count && written < n - 1; i++)
  {
    index = (slow_combat_index + COMBAT_SLOW_CAPACITY - 1 - i) % COMBAT_SLOW_CAPACITY;
    record = &slow_combats[index];
    if (csv)
      written += bounded_format_length(
          snprintf(out_buf + written, n - written,
                   "%" PRIu64 ",%" PRIu64 ",%s,%d,%d,%d,%" PRIu64 ",%" PRIu64 ",%" PRIu64
                   ",%" PRIu64 ",%" PRIu64 "\n\r",
                   record->wall_timestamp_sec, record->elapsed_usec,
                   record->actor_is_npc ? "npc" : "pc", record->actor_class, record->mobile_vnum,
                   record->room_vnum, record->participants, record->attacks, record->procs,
                   record->rejected_attacks, record->rejected_procs),
          n - written);
    else
      written += bounded_format_length(
          snprintf(out_buf + written, n - written,
                   "%-10" PRIu64 "|%10.3f|%-4s|%5d|%9d|%9d|%6" PRIu64 "|%7" PRIu64 "|%5" PRIu64
                   "|%8" PRIu64 "/%-8" PRIu64 "\n\r",
                   record->wall_timestamp_sec, (double)record->elapsed_usec / 1000.0,
                   record->actor_is_npc ? "NPC" : "PC", record->actor_class, record->mobile_vnum,
                   record->room_vnum, record->participants, record->attacks, record->procs,
                   record->rejected_attacks, record->rejected_procs),
          n - written);
  }
  return written;
}

/* ========================================================================
 * ENTITY LIFECYCLE MONITORING
 * ======================================================================== */

static const char *entity_reason_name(enum perf_entity_reason reason)
{
  static const char *names[PERF_ENTITY_REASON_COUNT] = {
      "unknown", "boot",   "zone_reset",  "dg_script", "spell_summon", "encounter",
      "quest",   "vessel", "pet_restore", "special",   "staff"};

  if (reason < PERF_ENTITY_UNKNOWN || reason >= PERF_ENTITY_REASON_COUNT)
    return names[PERF_ENTITY_UNKNOWN];
  return names[reason];
}

static size_t entity_hash(int key, size_t capacity)
{
  uint32_t value;

  value = (uint32_t)key;
  value ^= value >> 16;
  value *= UINT32_C(0x7feb352d);
  value ^= value >> 15;
  return (size_t)value & (capacity - 1);
}

static struct perf_entity_counter *entity_counter_slot(struct perf_entity_counter *table,
                                                       size_t capacity, int key)
{
  size_t start;
  size_t index;

  start = entity_hash(key, capacity);
  index = start;
  do
  {
    if (!table[index].used)
    {
      table[index].used = 1;
      table[index].key = key;
      return &table[index];
    }
    if (table[index].key == key)
      return &table[index];
    index = (index + 1) & (capacity - 1);
  } while (index != start);
  return NULL;
}

static struct perf_entity_zone_counter *entity_zone_slot(int key)
{
  size_t start;
  size_t index;

  start = entity_hash(key, ENTITY_ZONE_CAPACITY);
  index = start;
  do
  {
    if (!entity_zone_counters[index].used)
    {
      entity_zone_counters[index].used = 1;
      entity_zone_counters[index].key = key;
      return &entity_zone_counters[index];
    }
    if (entity_zone_counters[index].key == key)
      return &entity_zone_counters[index];
    index = (index + 1) & (ENTITY_ZONE_CAPACITY - 1);
  } while (index != start);
  return NULL;
}

enum perf_entity_reason PERF_entity_scope_set(enum perf_entity_reason reason)
{
  enum perf_entity_reason previous;

  previous = current_entity_reason;
  if (reason < PERF_ENTITY_UNKNOWN || reason >= PERF_ENTITY_REASON_COUNT)
    current_entity_reason = PERF_ENTITY_UNKNOWN;
  else
    current_entity_reason = reason;
  return previous;
}

void PERF_entity_scope_restore(enum perf_entity_reason reason)
{
  if (reason < PERF_ENTITY_UNKNOWN || reason >= PERF_ENTITY_REASON_COUNT)
    current_entity_reason = PERF_ENTITY_UNKNOWN;
  else
    current_entity_reason = reason;
}

enum perf_entity_reason PERF_entity_current_reason(void)
{
  return current_entity_reason;
}

static void note_entity_vnum(struct perf_entity_counter *table, size_t capacity, int vnum,
                             int created, uint64_t *overflow)
{
  struct perf_entity_counter *counter;

  counter = entity_counter_slot(table, capacity, vnum);
  if (counter == NULL)
  {
    *overflow = saturating_add_u64(*overflow, 1);
    return;
  }
  if (created)
    counter->created = saturating_add_u64(counter->created, 1);
  else
    counter->extracted = saturating_add_u64(counter->extracted, 1);
}

void PERF_note_mobile_created(int vnum, int zone_vnum, enum perf_entity_reason reason)
{
  struct perf_entity_zone_counter *zone;

  if (reason < PERF_ENTITY_UNKNOWN || reason >= PERF_ENTITY_REASON_COUNT)
    reason = PERF_ENTITY_UNKNOWN;
  mobiles_created_total = saturating_add_u64(mobiles_created_total, 1);
  entity_reason_counters[reason].mobiles_created =
      saturating_add_u64(entity_reason_counters[reason].mobiles_created, 1);
  note_entity_vnum(mobile_vnum_counters, ENTITY_VNUM_CAPACITY, vnum, 1, &mobile_vnum_overflow);
  zone = entity_zone_slot(zone_vnum);
  if (zone != NULL)
    zone->mobiles_created = saturating_add_u64(zone->mobiles_created, 1);
  else
    entity_zone_overflow = saturating_add_u64(entity_zone_overflow, 1);
}

void PERF_note_mobile_extracted(int vnum, int zone_vnum, enum perf_entity_reason reason)
{
  struct perf_entity_zone_counter *zone;

  if (reason < PERF_ENTITY_UNKNOWN || reason >= PERF_ENTITY_REASON_COUNT)
    reason = PERF_ENTITY_UNKNOWN;
  mobiles_extracted_total = saturating_add_u64(mobiles_extracted_total, 1);
  entity_reason_counters[reason].mobiles_extracted =
      saturating_add_u64(entity_reason_counters[reason].mobiles_extracted, 1);
  note_entity_vnum(mobile_vnum_counters, ENTITY_VNUM_CAPACITY, vnum, 0, &mobile_vnum_overflow);
  zone = entity_zone_slot(zone_vnum);
  if (zone != NULL)
    zone->mobiles_extracted = saturating_add_u64(zone->mobiles_extracted, 1);
  else
    entity_zone_overflow = saturating_add_u64(entity_zone_overflow, 1);
}

void PERF_note_object_created(int vnum, int zone_vnum, enum perf_entity_reason reason)
{
  struct perf_entity_zone_counter *zone;

  if (reason < PERF_ENTITY_UNKNOWN || reason >= PERF_ENTITY_REASON_COUNT)
    reason = PERF_ENTITY_UNKNOWN;
  objects_created_total = saturating_add_u64(objects_created_total, 1);
  entity_reason_counters[reason].objects_created =
      saturating_add_u64(entity_reason_counters[reason].objects_created, 1);
  note_entity_vnum(object_vnum_counters, ENTITY_VNUM_CAPACITY, vnum, 1, &object_vnum_overflow);
  zone = entity_zone_slot(zone_vnum);
  if (zone != NULL)
    zone->objects_created = saturating_add_u64(zone->objects_created, 1);
  else
    entity_zone_overflow = saturating_add_u64(entity_zone_overflow, 1);
}

void PERF_note_object_extracted(int vnum, int zone_vnum, enum perf_entity_reason reason)
{
  struct perf_entity_zone_counter *zone;

  if (reason < PERF_ENTITY_UNKNOWN || reason >= PERF_ENTITY_REASON_COUNT)
    reason = PERF_ENTITY_UNKNOWN;
  objects_extracted_total = saturating_add_u64(objects_extracted_total, 1);
  entity_reason_counters[reason].objects_extracted =
      saturating_add_u64(entity_reason_counters[reason].objects_extracted, 1);
  note_entity_vnum(object_vnum_counters, ENTITY_VNUM_CAPACITY, vnum, 0, &object_vnum_overflow);
  zone = entity_zone_slot(zone_vnum);
  if (zone != NULL)
    zone->objects_extracted = saturating_add_u64(zone->objects_extracted, 1);
  else
    entity_zone_overflow = saturating_add_u64(entity_zone_overflow, 1);
}

void PERF_note_zone_reset(int zone_vnum, uint64_t elapsed_usec, uint64_t mobiles_created,
                          uint64_t mobiles_extracted, uint64_t objects_created,
                          uint64_t objects_extracted)
{
  struct perf_entity_zone_counter *zone;

  zone = entity_zone_slot(zone_vnum);
  if (zone == NULL)
  {
    entity_zone_overflow = saturating_add_u64(entity_zone_overflow, 1);
    return;
  }
  zone->resets = saturating_add_u64(zone->resets, 1);
  zone->reset_total_usec = saturating_add_u64(zone->reset_total_usec, elapsed_usec);
  if (elapsed_usec > zone->reset_max_usec)
    zone->reset_max_usec = elapsed_usec;
  /* These deltas include cross-zone prototypes and therefore complement the
   * prototype-zone counters rather than replacing them. */
  zone->reset_mobiles_created = saturating_add_u64(zone->reset_mobiles_created, mobiles_created);
  zone->reset_mobiles_extracted =
      saturating_add_u64(zone->reset_mobiles_extracted, mobiles_extracted);
  zone->reset_objects_created = saturating_add_u64(zone->reset_objects_created, objects_created);
  zone->reset_objects_extracted =
      saturating_add_u64(zone->reset_objects_extracted, objects_extracted);
}

void PERF_entity_totals(uint64_t *mobiles_created, uint64_t *mobiles_extracted,
                        uint64_t *objects_created, uint64_t *objects_extracted)
{
  if (mobiles_created != NULL)
    *mobiles_created = mobiles_created_total;
  if (mobiles_extracted != NULL)
    *mobiles_extracted = mobiles_extracted_total;
  if (objects_created != NULL)
    *objects_created = objects_created_total;
  if (objects_extracted != NULL)
    *objects_extracted = objects_extracted_total;
}

void PERF_note_sweep(enum perf_sweep_kind kind, uint64_t visited, uint64_t eligible, uint64_t acted)
{
  struct perf_sweep_counter *counter;

  if (kind < PERF_SWEEP_AUTOPROC || kind >= PERF_SWEEP_COUNT)
    return;
  counter = &sweep_counters[kind];
  counter->calls = saturating_add_u64(counter->calls, 1);
  counter->visited = saturating_add_u64(counter->visited, visited);
  counter->eligible = saturating_add_u64(counter->eligible, eligible);
  counter->acted = saturating_add_u64(counter->acted, acted);
  if (visited > counter->max_visited)
    counter->max_visited = visited;
  if (eligible > counter->max_eligible)
    counter->max_eligible = eligible;
  if (acted > counter->max_acted)
    counter->max_acted = acted;
}

static const char *sweep_name(enum perf_sweep_kind kind)
{
  static const char *names[PERF_SWEEP_COUNT] = {"autoproc", "dg_mobile_random", "dg_object_random",
                                                "dg_room_random", "affect"};

  if (kind < PERF_SWEEP_AUTOPROC || kind >= PERF_SWEEP_COUNT)
    return "unknown";
  return names[kind];
}

static int64_t entity_counter_net(const struct perf_entity_counter *counter)
{
  return (int64_t)counter->created - (int64_t)counter->extracted;
}

static size_t collect_top_entity_counters(const struct perf_entity_counter *table, size_t capacity,
                                          size_t *indices)
{
  size_t top_count;
  size_t position;
  size_t i;
  size_t j;
  int64_t net;

  top_count = 0;
  for (i = 0; i < capacity; i++)
  {
    if (!table[i].used || (net = entity_counter_net(&table[i])) <= 0)
      continue;
    position = 0;
    while (position < top_count && entity_counter_net(&table[indices[position]]) >= net)
      position++;
    if (position >= ENTITY_REPORT_LIMIT)
      continue;
    if (top_count < ENTITY_REPORT_LIMIT)
      top_count++;
    for (j = top_count - 1; j > position; j--)
      indices[j] = indices[j - 1];
    indices[position] = i;
  }
  return top_count;
}

size_t PERF_entities_repr(char *out_buf, size_t n, int csv)
{
  const struct perf_entity_counter *counter;
  const struct perf_entity_reason_counter *reason;
  const struct perf_entity_zone_counter *zone;
  size_t mobile_top[ENTITY_REPORT_LIMIT];
  size_t object_top[ENTITY_REPORT_LIMIT];
  size_t mobile_count;
  size_t object_count;
  size_t written;
  size_t i;

  if (out_buf == NULL || n == 0)
    return 0;
  mobile_count =
      collect_top_entity_counters(mobile_vnum_counters, ENTITY_VNUM_CAPACITY, mobile_top);
  object_count =
      collect_top_entity_counters(object_vnum_counters, ENTITY_VNUM_CAPACITY, object_top);
  if (csv)
    written = bounded_format_length(
        snprintf(
            out_buf, n,
            "entity_kind,key,created,extracted,net,resets,reset_total_usec,reset_max_usec\n\r"),
        n);
  else
    written = bounded_format_length(
        snprintf(out_buf, n,
                 "Entity lifecycle since reset\n\r"
                 "Mobiles: created=%" PRIu64 " extracted=%" PRIu64 " net=%" PRId64
                 " | Objects: created=%" PRIu64 " extracted=%" PRIu64 " net=%" PRId64 "\n\r"
                 "Counter overflow: mob_vnum=%" PRIu64 " obj_vnum=%" PRIu64 " zone=%" PRIu64
                 "\n\rCreation reason                 | Mob create| Mob extract|    Mob net|"
                 " Obj create| Obj extract|    Obj net\n\r",
                 mobiles_created_total, mobiles_extracted_total,
                 (int64_t)mobiles_created_total - (int64_t)mobiles_extracted_total,
                 objects_created_total, objects_extracted_total,
                 (int64_t)objects_created_total - (int64_t)objects_extracted_total,
                 mobile_vnum_overflow, object_vnum_overflow, entity_zone_overflow),
        n);

  for (i = 0; i < PERF_ENTITY_REASON_COUNT && written < n - 1; i++)
  {
    reason = &entity_reason_counters[i];
    if (csv)
      written += bounded_format_length(
          snprintf(out_buf + written, n - written,
                   "reason,%s,%" PRIu64 ",%" PRIu64 ",%" PRId64 ",0,0,0\n\r"
                   "reason_object,%s,%" PRIu64 ",%" PRIu64 ",%" PRId64 ",0,0,0\n\r",
                   entity_reason_name((enum perf_entity_reason)i), reason->mobiles_created,
                   reason->mobiles_extracted,
                   (int64_t)reason->mobiles_created - (int64_t)reason->mobiles_extracted,
                   entity_reason_name((enum perf_entity_reason)i), reason->objects_created,
                   reason->objects_extracted,
                   (int64_t)reason->objects_created - (int64_t)reason->objects_extracted),
          n - written);
    else if (reason->mobiles_created != 0 || reason->mobiles_extracted != 0 ||
             reason->objects_created != 0 || reason->objects_extracted != 0)
      written += bounded_format_length(
          snprintf(out_buf + written, n - written,
                   "%-32s|%11" PRIu64 "|%12" PRIu64 "|%11" PRId64 "|%11" PRIu64 "|%12" PRIu64
                   "|%11" PRId64 "\n\r",
                   entity_reason_name((enum perf_entity_reason)i), reason->mobiles_created,
                   reason->mobiles_extracted,
                   (int64_t)reason->mobiles_created - (int64_t)reason->mobiles_extracted,
                   reason->objects_created, reason->objects_extracted,
                   (int64_t)reason->objects_created - (int64_t)reason->objects_extracted),
          n - written);
  }

  if (!csv && written < n - 1)
    written += bounded_format_length(
        snprintf(
            out_buf + written, n - written,
            "Top mobile VNUM positive net\n\rVNUM       |    Created|   Extracted|        Net\n\r"),
        n - written);
  for (i = 0; i < mobile_count && written < n - 1; i++)
  {
    counter = &mobile_vnum_counters[mobile_top[i]];
    written += bounded_format_length(
        snprintf(out_buf + written, n - written,
                 csv ? "mobile_vnum,%d,%" PRIu64 ",%" PRIu64 ",%" PRId64 ",0,0,0\n\r"
                     : "%-11d|%11" PRIu64 "|%12" PRIu64 "|%11" PRId64 "\n\r",
                 counter->key, counter->created, counter->extracted, entity_counter_net(counter)),
        n - written);
  }
  if (!csv && written < n - 1)
    written += bounded_format_length(
        snprintf(
            out_buf + written, n - written,
            "Top object VNUM positive net\n\rVNUM       |    Created|   Extracted|        Net\n\r"),
        n - written);
  for (i = 0; i < object_count && written < n - 1; i++)
  {
    counter = &object_vnum_counters[object_top[i]];
    written += bounded_format_length(
        snprintf(out_buf + written, n - written,
                 csv ? "object_vnum,%d,%" PRIu64 ",%" PRIu64 ",%" PRId64 ",0,0,0\n\r"
                     : "%-11d|%11" PRIu64 "|%12" PRIu64 "|%11" PRId64 "\n\r",
                 counter->key, counter->created, counter->extracted, entity_counter_net(counter)),
        n - written);
  }
  if (!csv && written < n - 1)
    written += bounded_format_length(snprintf(out_buf + written, n - written,
                                              "Zones with reset or lifecycle activity\n\rZone      "
                                              " | Resets| Reset total ms| Reset max ms|"
                                              " Mob create/extract| Obj create/extract\n\r"),
                                     n - written);
  for (i = 0; i < ENTITY_ZONE_CAPACITY && written < n - 1; i++)
  {
    zone = &entity_zone_counters[i];
    if (!zone->used || (zone->resets == 0 && zone->mobiles_created == zone->mobiles_extracted &&
                        zone->objects_created == zone->objects_extracted))
      continue;
    if (csv)
      written += bounded_format_length(
          snprintf(out_buf + written, n - written,
                   "zone,%d,%" PRIu64 ",%" PRIu64 ",%" PRId64 ",%" PRIu64 ",%" PRIu64 ",%" PRIu64
                   "\n\r",
                   zone->key, zone->mobiles_created + zone->objects_created,
                   zone->mobiles_extracted + zone->objects_extracted,
                   (int64_t)(zone->mobiles_created + zone->objects_created) -
                       (int64_t)(zone->mobiles_extracted + zone->objects_extracted),
                   zone->resets, zone->reset_total_usec, zone->reset_max_usec),
          n - written);
    else
      written += bounded_format_length(
          snprintf(out_buf + written, n - written,
                   "%-11d|%7" PRIu64 "|%15.3f|%13.3f|%11" PRIu64 "/%-7" PRIu64 "|%11" PRIu64
                   "/%-7" PRIu64 "\n\r",
                   zone->key, zone->resets, (double)zone->reset_total_usec / 1000.0,
                   (double)zone->reset_max_usec / 1000.0, zone->reset_mobiles_created,
                   zone->reset_mobiles_extracted, zone->reset_objects_created,
                   zone->reset_objects_extracted),
          n - written);
  }
  if (csv && written < n - 1)
    written += bounded_format_length(
        snprintf(out_buf + written, n - written,
                 "sweep,calls,visited,eligible,acted,max_visited,max_eligible,max_acted\n\r"),
        n - written);
  else if (written < n - 1)
    written += bounded_format_length(
        snprintf(out_buf + written, n - written,
                 "Population sweep telemetry\n\r"
                 "Sweep                           |    Calls|      Visited|     Eligible|"
                 "        Acted| Max visit/eligible/acted\n\r"),
        n - written);
  for (i = 0; i < PERF_SWEEP_COUNT && written < n - 1; i++)
  {
    const struct perf_sweep_counter *sweep;

    sweep = &sweep_counters[i];
    if (csv)
      written +=
          bounded_format_length(snprintf(out_buf + written, n - written,
                                         "%s,%" PRIu64 ",%" PRIu64 ",%" PRIu64 ",%" PRIu64
                                         ",%" PRIu64 ",%" PRIu64 ",%" PRIu64 "\n\r",
                                         sweep_name((enum perf_sweep_kind)i), sweep->calls,
                                         sweep->visited, sweep->eligible, sweep->acted,
                                         sweep->max_visited, sweep->max_eligible, sweep->max_acted),
                                n - written);
    else
      written +=
          bounded_format_length(snprintf(out_buf + written, n - written,
                                         "%-32s|%9" PRIu64 "|%13" PRIu64 "|%13" PRIu64 "|%13" PRIu64
                                         "|%10" PRIu64 "/%-8" PRIu64 "/%-8" PRIu64 "\n\r",
                                         sweep_name((enum perf_sweep_kind)i), sweep->calls,
                                         sweep->visited, sweep->eligible, sweep->acted,
                                         sweep->max_visited, sweep->max_eligible, sweep->max_acted),
                                n - written);
  }
  if (!csv && written < n - 1)
    written += bounded_format_length(
        snprintf(out_buf + written, n - written,
                 "Autoproc owners: mode=%s members=%zu scheduled=%zu limit=%zu rejected=%" PRIu64
                 " callbacks=%" PRIu64 " validation_mismatch=%zu\n\r",
                 periodic_autoproc_enabled() ? "scheduled" : "legacy",
                 autoproc_registry_count(), periodic_autoproc_scheduled_count(),
                 periodic_autoproc_admission_limit(), periodic_autoproc_admission_rejections(),
                 periodic_autoproc_callbacks(), autoproc_registry_validate()),
        n - written);
  if (!csv && written < n - 1)
    written += bounded_format_length(
        snprintf(out_buf + written, n - written,
                 "DG random owners: mode=%s mob=%zu/%zu/%zu obj=%zu/%zu/%zu "
                 "room=%zu/%zu/%zu (members/scheduled/mismatch) limit=%zu rejected=%" PRIu64
                 " callbacks=%" PRIu64 "/%" PRIu64 "/%" PRIu64
                 " executions=%" PRIu64 "/%" PRIu64 "/%" PRIu64 "\n\r",
                 periodic_dg_random_enabled() ? "scheduled" : "legacy",
                 dg_random_registry_count(MOB_TRIGGER),
                 periodic_dg_random_scheduled_count(MOB_TRIGGER),
                 dg_random_registry_validate(MOB_TRIGGER),
                 dg_random_registry_count(OBJ_TRIGGER),
                 periodic_dg_random_scheduled_count(OBJ_TRIGGER),
                 dg_random_registry_validate(OBJ_TRIGGER),
                 dg_random_registry_count(WLD_TRIGGER),
                 periodic_dg_random_scheduled_count(WLD_TRIGGER),
                 dg_random_registry_validate(WLD_TRIGGER), periodic_dg_random_admission_limit(),
                 periodic_dg_random_admission_rejections(),
                 periodic_dg_random_callbacks(MOB_TRIGGER),
                 periodic_dg_random_callbacks(OBJ_TRIGGER),
                 periodic_dg_random_callbacks(WLD_TRIGGER),
                 periodic_dg_random_executions(MOB_TRIGGER),
                 periodic_dg_random_executions(OBJ_TRIGGER),
                 periodic_dg_random_executions(WLD_TRIGGER)),
        n - written);
  if (!csv && written < n - 1)
    written += bounded_format_length(
        snprintf(out_buf + written, n - written,
                 "Affected owners: %s\n\r"
                 "  chars: members=%zu scheduled=%zu mismatch=%zu\n\r"
                 "  rooms: members=%zu scheduled=%zu mismatch=%zu\n\r"
                 "  limits: chars=%zu rooms=%zu rejected=%" PRIu64 "\n\r"
                 "  char work: callbacks=%" PRIu64 " affects=%" PRIu64 "\n\r"
                 "  room work: callbacks=%" PRIu64 " affects=%" PRIu64 "\n\r"
                 "  room behavior: runs=%" PRIu64 " affects=%" PRIu64 "\n\r",
                 affected_owner_events_enabled() ? "scheduled" : "legacy heartbeat",
                 affected_registry_count(), affected_character_scheduled_count(),
                 affected_registry_validate(), affected_room_owner_count(),
                 affected_room_scheduled_count(), affected_room_registry_validate(),
                 affected_character_admission_limit(), affected_room_admission_limit(),
                 affected_owner_admission_rejections(), affected_character_callbacks(),
                 affected_character_nodes_processed(), affected_room_callbacks(),
                 affected_room_nodes_processed(), affected_room_behavior_executions(),
                 affected_room_behavior_nodes_processed()),
        n - written);
  if (!csv && written < n - 1)
    written += bounded_format_length(
        snprintf(out_buf + written, n - written,
                 "Character owners: %s\n\r"
                 "  registry: members=%zu scheduled=%zu\n\r"
                 "  validation: mismatch=%zu\n\r"
                 "  capacity: limit=%zu rejected=%" PRIu64 "\n\r"
                 "  callbacks=%" PRIu64 "\n\r"
                 "  work: walk=%" PRIu64 " psp=%" PRIu64 "\n\r"
                 "  work: luminari=%" PRIu64 " damage=%" PRIu64 "\n\r"
                 "  work: player-misc=%" PRIu64 "\n\r"
                 "  work: bard=%" PRIu64 " hints=%" PRIu64 "\n\r",
                 character_periodic_events_enabled() ? "scheduled" : "legacy heartbeat",
                 character_periodic_owner_count(), character_periodic_scheduled_count(),
                 character_periodic_registry_validate(), character_periodic_admission_limit(),
                 character_periodic_admission_rejections(), character_periodic_callbacks(),
                 character_periodic_walk_executions(), character_periodic_psp_executions(),
                 character_periodic_luminari_executions(),
                 character_periodic_damage_effect_executions(),
                 character_periodic_player_misc_executions(),
                 character_periodic_bardic_executions(), character_periodic_hint_executions()),
        n - written);
  if (!csv && written < n - 1)
    written += bounded_format_length(
        snprintf(out_buf + written, n - written,
                 "Active world: mode=%s active=%zu cooling=%zu limit=%zu rejected=%" PRIu64
                 " callbacks=%" PRIu64 "\n\r",
                 active_world_enabled() ? "scheduled" : "legacy",
                 active_world_mobile_count(ACTIVE_WORLD_MOBILE_ACTIVE),
                 active_world_mobile_count(ACTIVE_WORLD_MOBILE_COOLING),
                 active_world_mobile_admission_limit(),
                 active_world_mobile_admission_rejections(), active_world_mobile_callbacks()),
        n - written);
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
  if (diff_usec == 0)
    diff_usec = 1;
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
  memset(&pulse_event_lifecycle_stats, 0, sizeof(pulse_event_lifecycle_stats));
  memset(&pulse_extraction_stats, 0, sizeof(pulse_extraction_stats));
  memset(&pulse_catchup_stats, 0, sizeof(pulse_catchup_stats));
  pulse_schedule_flags = 0;
  pulse_last_heartbeat = 0;
  pthread_mutex_lock(&sql_stats_mutex);
  pulse_main_sql_calls = 0;
  pulse_main_sql_usec = 0;
  pthread_mutex_unlock(&sql_stats_mutex);
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
  char median_text[16];
  char p95_text[16];
  char p99_text[16];

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
    if (sect->sampling_enabled && sect->sample_count > 0)
    {
      calculate_percentile_set(sect->samples, sect->sample_count, &median, &p95, &p99);
      snprintf(median_text, sizeof(median_text), "%.2f", median);
      snprintf(p95_text, sizeof(p95_text), "%.2f", p95);
      snprintf(p99_text, sizeof(p99_text), "%.2f", p99);
    }
    else
    {
      strlcpy(median_text, "n/a", sizeof(median_text));
      strlcpy(p95_text, "n/a", sizeof(p95_text));
      strlcpy(p99_text, "n/a", sizeof(p99_text));
    }

    return bounded_format_length(
        snprintf(buf, n,
                 "%-63s|%9" PRIu64 "|%12" PRIu64 "|%8.2f%%|%10.2f|%10s|%10s|%10s|"
                 "%10" PRIu64 "|%7zu/%-7" PRIu64 "\n\r",
                 sect->id, exit_count, usec_total, percent, average, median_text, p95_text,
                 p99_text, usec_max, sect->sample_count, sect->samples_seen),
        n);
  }
  else
  {
    return bounded_format_length(snprintf(buf, n,
                                          "%-63s|%9" PRIu64 "|%9" PRIu64 "|%12" PRIu64 "|%8.2f%%|"
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
  const struct perf_event_lifecycle_stats *lifecycle_stats;
  const struct perf_extraction_stats *extraction_stats;
  const struct perf_catchup_stats *catchup_stats;
  const struct perf_event_callback *profile;
  size_t top_indices[EVENT_PROFILE_REPORT_LIMIT];
  size_t top_count;
  size_t written;
  size_t i;
  uint64_t overflow_calls;
  double average;
  double median;
  double p95;
  double p99;

  if (buf == NULL || n == 0)
    return 0;

  if (is_total)
  {
    process_stats = &total_event_process_stats;
    lifecycle_stats = &total_event_lifecycle_stats;
    extraction_stats = &total_extraction_stats;
    catchup_stats = &total_catchup_stats;
  }
  else
  {
    process_stats = &pulse_event_process_stats;
    lifecycle_stats = &pulse_event_lifecycle_stats;
    extraction_stats = &pulse_extraction_stats;
    catchup_stats = &pulse_catchup_stats;
  }
  overflow_calls =
      is_total ? event_profile_overflow.total_calls : event_profile_overflow.pulse_calls;

  written = bounded_format_length(
      snprintf(
          buf, n,
          "\n\r%s game-loop telemetry\n\r"
          "Event queue: calls=%" PRIu64 " callbacks=%" PRIu64 " created=%" PRIu64 " depth=%" PRIu64
          "->%" PRIu64 " max_before=%" PRIu64 " max_after=%" PRIu64 " max_batch=%" PRIu64 "\n\r"
          "Event lifecycle: scheduled=%" PRIu64 " cancelled=%" PRIu64 " recurrences=%" PRIu64 "\n\r"
          "Requested delay buckets (pulses): <=1=%" PRIu64 " 2-10=%" PRIu64 " 11-60=%" PRIu64
          " 61-600=%" PRIu64 " 601-6000=%" PRIu64 " 6001-36000=%" PRIu64 " >36000=%" PRIu64 "\n\r"
          "Extractions: calls=%" PRIu64 " pending_before=%" PRIu64 " processed=%" PRIu64
          " pending_after=%" PRIu64 " max_processed=%" PRIu64 " max_pending_before=%" PRIu64
          " max_pending_after=%" PRIu64 "\n\r"
          "Catch-up: passes=%" PRIu64 " budget_exhausted=%" PRIu64 " requested_missed=%" PRIu64
          " replayed_missed=%" PRIu64 " dropped_missed=%" PRIu64 " max_requested=%" PRIu64
          " max_dropped=%" PRIu64 "\n\r"
          "Event callback registry: registered=%zu/%d report_limit=%d overflow_calls=%" PRIu64
          "\n\r",
          is_total ? "Cumulative" : "Pulse", process_stats->calls,
          process_stats->callbacks_processed, process_stats->events_created,
          process_stats->initial_depth, process_stats->latest_depth,
          process_stats->max_depth_before, process_stats->max_depth_after,
          process_stats->max_callbacks_per_call, lifecycle_stats->scheduled,
          lifecycle_stats->cancelled, lifecycle_stats->rescheduled,
          lifecycle_stats->delay_buckets[0], lifecycle_stats->delay_buckets[1],
          lifecycle_stats->delay_buckets[2], lifecycle_stats->delay_buckets[3],
          lifecycle_stats->delay_buckets[4], lifecycle_stats->delay_buckets[5],
          lifecycle_stats->delay_buckets[6], extraction_stats->calls,
          extraction_stats->pending_before, extraction_stats->processed,
          extraction_stats->pending_after, extraction_stats->max_processed,
          extraction_stats->max_pending_before, extraction_stats->max_pending_after,
          catchup_stats->passes, catchup_stats->budget_exhausted_passes,
          catchup_stats->requested_missed, catchup_stats->replayed_missed,
          catchup_stats->remaining_backlog, catchup_stats->max_requested_missed,
          catchup_stats->max_remaining_backlog, event_profile_count, EVENT_PROFILE_CAPACITY,
          EVENT_PROFILE_REPORT_LIMIT, overflow_calls),
      n);

  if (written >= n - 1)
    return written;

  written += bounded_format_length(
      snprintf(
          buf + written, n - written,
          "Event callbacks (top %d by total time)\n\r"
          "Identity                            |    Calls|  Total usec|  Avg usec| P50 usec|"
          " P95 usec| P99 usec|  Max usec|Samples stored/seen|    Sched|   Cancel|    Recur\n\r"
          "-----------------------------------------------------------------------------------"
          "-----------------------------------------------\n\r",
          EVENT_PROFILE_REPORT_LIMIT),
      n - written);

  top_count = collect_top_event_profiles(top_indices, is_total);
  for (i = 0; i < top_count && written < n - 1; i++)
  {
    profile = &event_profiles[top_indices[i]];
    calculate_percentile_set(profile->samples, profile->sample_count, &median, &p95, &p99);
    if (is_total)
    {
      average = profile->total_calls > 0
                    ? (double)profile->total_usec / (double)profile->total_calls
                    : 0.0;
      written += bounded_format_length(
          snprintf(buf + written, n - written,
                   "%-36.36s|%9" PRIu64 "|%12" PRIu64 "|%10.2f|%9.2f|%9.2f|%9.2f|%10" PRIu64
                   "|%7zu/%" PRIu64 "|%9" PRIu64 "|%9" PRIu64 "|%9" PRIu64 "\n\r",
                   profile->identity, profile->total_calls, profile->total_usec, average, median,
                   p95, p99, profile->total_max_usec, profile->sample_count, profile->samples_seen,
                   profile->total_scheduled, profile->total_cancelled, profile->total_rescheduled),
          n - written);
    }
    else
    {
      average = profile->pulse_calls > 0
                    ? (double)profile->pulse_total_usec / (double)profile->pulse_calls
                    : 0.0;
      written += bounded_format_length(
          snprintf(buf + written, n - written,
                   "%-36.36s|%9" PRIu64 "|%12" PRIu64 "|%10.2f|%9.2f|%9.2f|%9.2f|%10" PRIu64
                   "|%7zu/%" PRIu64 "|%9" PRIu64 "|%9" PRIu64 "|%9" PRIu64 "\n\r",
                   profile->identity, profile->pulse_calls, profile->pulse_total_usec, average,
                   median, p95, p99, profile->pulse_max_usec, profile->sample_count,
                   profile->samples_seen, profile->pulse_scheduled, profile->pulse_cancelled,
                   profile->pulse_rescheduled),
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
    calculate_percentile_set(profile->samples, profile->sample_count, &median, &p95, &p99);
    written += bounded_format_length(
        snprintf(buf + written, n - written,
                 "%-36.36s|%9" PRIu64 "|%12" PRIu64 "|%10.2f|%9.2f|%9.2f|%9.2f|%10" PRIu64
                 "|%7zu/%" PRIu64 "|%9" PRIu64 "|%9" PRIu64 "|%9" PRIu64 "\n\r",
                 "[unregistered overflow]", calls, total_usec, average, median, p95, p99, max_usec,
                 profile->sample_count, profile->samples_seen,
                 is_total ? profile->total_scheduled : profile->pulse_scheduled,
                 is_total ? profile->total_cancelled : profile->pulse_cancelled,
                 is_total ? profile->total_rescheduled : profile->pulse_rescheduled),
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
  double median;
  double p95;
  double p99;

  if (buf == NULL || n == 0)
    return 0;

  written = bounded_format_length(
      snprintf(
          buf, n,
          "# event_process_calls=%" PRIu64 "\n\r"
          "# event_callbacks_processed=%" PRIu64 "\n\r"
          "# events_created_during_processing=%" PRIu64 "\n\r"
          "# event_queue_depth_initial=%" PRIu64 "\n\r"
          "# event_queue_depth_latest=%" PRIu64 "\n\r"
          "# event_queue_depth_max_before=%" PRIu64 "\n\r"
          "# event_queue_depth_max_after=%" PRIu64 "\n\r"
          "# event_due_batch_max=%" PRIu64 "\n\r"
          "# events_scheduled=%" PRIu64 "\n\r"
          "# events_cancelled=%" PRIu64 "\n\r"
          "# events_rescheduled=%" PRIu64 "\n\r"
          "# event_delay_pulses_le_1=%" PRIu64 "\n\r"
          "# event_delay_pulses_2_10=%" PRIu64 "\n\r"
          "# event_delay_pulses_11_60=%" PRIu64 "\n\r"
          "# event_delay_pulses_61_600=%" PRIu64 "\n\r"
          "# event_delay_pulses_601_6000=%" PRIu64 "\n\r"
          "# event_delay_pulses_6001_36000=%" PRIu64 "\n\r"
          "# event_delay_pulses_gt_36000=%" PRIu64 "\n\r"
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
          "# catchup_dropped_missed=%" PRIu64 "\n\r"
          "# catchup_max_requested_missed=%" PRIu64 "\n\r"
          "# catchup_max_dropped_missed=%" PRIu64 "\n\r"
          "# event_profile_registered=%zu\n\r"
          "# event_profile_capacity=%d\n\r"
          "# event_profile_report_limit=%d\n\r"
          "# event_profile_overflow_calls=%" PRIu64 "\n\r",
          total_event_process_stats.calls, total_event_process_stats.callbacks_processed,
          total_event_process_stats.events_created, total_event_process_stats.initial_depth,
          total_event_process_stats.latest_depth, total_event_process_stats.max_depth_before,
          total_event_process_stats.max_depth_after,
          total_event_process_stats.max_callbacks_per_call, total_event_lifecycle_stats.scheduled,
          total_event_lifecycle_stats.cancelled, total_event_lifecycle_stats.rescheduled,
          total_event_lifecycle_stats.delay_buckets[0],
          total_event_lifecycle_stats.delay_buckets[1],
          total_event_lifecycle_stats.delay_buckets[2],
          total_event_lifecycle_stats.delay_buckets[3],
          total_event_lifecycle_stats.delay_buckets[4],
          total_event_lifecycle_stats.delay_buckets[5],
          total_event_lifecycle_stats.delay_buckets[6], total_extraction_stats.calls,
          total_extraction_stats.pending_before, total_extraction_stats.processed,
          total_extraction_stats.pending_after, total_extraction_stats.max_processed,
          total_extraction_stats.max_pending_before, total_extraction_stats.max_pending_after,
          total_catchup_stats.passes, total_catchup_stats.budget_exhausted_passes,
          total_catchup_stats.requested_missed, total_catchup_stats.replayed_missed,
          total_catchup_stats.remaining_backlog, total_catchup_stats.max_requested_missed,
          total_catchup_stats.max_remaining_backlog, event_profile_count, EVENT_PROFILE_CAPACITY,
          EVENT_PROFILE_REPORT_LIMIT, event_profile_overflow.total_calls),
      n);
  if (written >= n - 1)
    return written;

  written +=
      bounded_format_length(snprintf(buf + written, n - written,
                                     "event_identity,calls,total_usec,average_usec,p50_usec,"
                                     "p95_usec,p99_usec,max_usec,samples_stored,samples_seen,"
                                     "scheduled,cancelled,rescheduled\n\r"),
                            n - written);
  top_count = collect_top_event_profiles(top_indices, 1);
  for (i = 0; i < top_count && written < n - 1; i++)
  {
    profile = &event_profiles[top_indices[i]];
    average =
        profile->total_calls > 0 ? (double)profile->total_usec / (double)profile->total_calls : 0.0;
    calculate_percentile_set(profile->samples, profile->sample_count, &median, &p95, &p99);
    written += bounded_format_length(
        snprintf(buf + written, n - written,
                 "%s,%" PRIu64 ",%" PRIu64 ",%.2f,%.2f,%.2f,%.2f,%" PRIu64 ",%zu,%" PRIu64
                 ",%" PRIu64 ",%" PRIu64 ",%" PRIu64 "\n\r",
                 profile->identity, profile->total_calls, profile->total_usec, average, median, p95,
                 p99, profile->total_max_usec, profile->sample_count, profile->samples_seen,
                 profile->total_scheduled, profile->total_cancelled, profile->total_rescheduled),
        n - written);
  }

  profile = &event_profile_overflow;
  if (written < n - 1 && profile->total_calls > 0)
  {
    average = (double)profile->total_usec / (double)profile->total_calls;
    calculate_percentile_set(profile->samples, profile->sample_count, &median, &p95, &p99);
    written += bounded_format_length(
        snprintf(buf + written, n - written,
                 "[unregistered overflow],%" PRIu64 ",%" PRIu64 ",%.2f,%.2f,%.2f,%.2f,%" PRIu64
                 ",%zu,%" PRIu64 ",%" PRIu64 ",%" PRIu64 ",%" PRIu64 "\n\r",
                 profile->total_calls, profile->total_usec, average, median, p95, p99,
                 profile->total_max_usec, profile->sample_count, profile->samples_seen,
                 profile->total_scheduled, profile->total_cancelled, profile->total_rescheduled),
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
      snprintf(out_buf, n,
               "Pulse profiling info\n\r\n\r"
               "Section name                                                   |    Enter|     "
               "Exit|  Total usec| Pulse %%|"
               "  Max usec|Max %%\n\r"
               "-----------------------------------------------------------------------------------"
               "-------------------------------------------"
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
               "Cumulative profiling info (Total %% is inclusive elapsed-wall time)\n\r\n\r"
               "Section name                                                   |    Calls|  Total "
               "usec| Total %%|  Avg usec|Median usec|"
               "  P95 usec|  P99 usec|  Max usec|Samples stored/seen\n\r"
               "-----------------------------------------------------------------------------------"
               "---------------------------------------------"
               "-----------------------------------------------\n\r"),
      n);

  for (i = 0; i < prof_section_count && written < n - 1; i++)
  {
    written += format_prof_section(out_buf + written, n - written, prof_sections[i], 1);
  }

  if (written < n - 1)
    written += format_event_telemetry(out_buf + written, n - written, 1);

  return written;
}

enum perf_top_metric
{
  PERF_TOP_TOTAL = 0,
  PERF_TOP_MAX,
  PERF_TOP_P99
};

static double prof_top_score(const struct PERF_prof_sect *sect, enum perf_top_metric metric)
{
  if (sect == NULL || sect->total_exit_count == 0)
    return -1.0;
  if (metric == PERF_TOP_MAX)
    return (double)sect->max_usec;
  if (metric == PERF_TOP_P99)
  {
    if (!sect->sampling_enabled || sect->sample_count == 0)
      return -1.0;
    return PERF_calculate_percentile(sect->samples, sect->sample_count, 99.0);
  }
  return (double)sect->total_usec;
}

size_t PERF_prof_repr_top(char *out_buf, size_t n, const char *metric_name, size_t limit)
{
  enum perf_top_metric metric;
  struct PERF_prof_sect *sect;
  size_t top_indices[100];
  size_t top_count;
  size_t position;
  size_t written;
  size_t i;
  size_t j;
  double score;

  if (out_buf == NULL || n == 0)
    return 0;
  ensure_initialized();
  metric = PERF_TOP_TOTAL;
  if (metric_name != NULL && !strcasecmp(metric_name, "max"))
    metric = PERF_TOP_MAX;
  else if (metric_name != NULL && !strcasecmp(metric_name, "p99"))
    metric = PERF_TOP_P99;
  else if (metric_name != NULL && strcasecmp(metric_name, "total"))
    return bounded_format_length(
        snprintf(out_buf, n, "Unknown top metric '%s'; use total, max, or p99.\n\r", metric_name),
        n);

  if (limit == 0)
    limit = 15;
  if (limit > sizeof(top_indices) / sizeof(top_indices[0]))
    limit = sizeof(top_indices) / sizeof(top_indices[0]);
  top_count = 0;
  for (i = 0; i < (size_t)prof_section_count; i++)
  {
    score = prof_top_score(prof_sections[i], metric);
    if (score < 0.0)
      continue;
    position = 0;
    while (position < top_count &&
           prof_top_score(prof_sections[top_indices[position]], metric) >= score)
      position++;
    if (position >= limit)
      continue;
    if (top_count < limit)
      top_count++;
    for (j = top_count - 1; j > position; j--)
      top_indices[j] = top_indices[j - 1];
    top_indices[position] = i;
  }

  written = bounded_format_length(
      snprintf(out_buf, n,
               "Top profiling sections by %s (Total %% is inclusive elapsed-wall time)\n\r"
               "Section name                                                   |    Calls|  Total "
               "usec| Total %%|  Avg usec|Median usec|"
               "  P95 usec|  P99 usec|  Max usec|Samples stored/seen\n\r"
               "-----------------------------------------------------------------------------------"
               "-----------------------------------------------------------------------------------"
               "---------"
               "\n\r",
               metric == PERF_TOP_TOTAL ? "total"
               : metric == PERF_TOP_MAX ? "max"
                                        : "p99"),
      n);
  for (i = 0; i < top_count && written < n - 1; i++)
  {
    sect = prof_sections[top_indices[i]];
    written += format_prof_section(out_buf + written, n - written, sect, 1);
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
      snprintf(out_buf, n,
               "Pulse profiling info\n\r\n\r"
               "Section name                                                   |    Enter|     "
               "Exit|  Total usec| Pulse %%|"
               "  Max usec|Max %%\n\r"
               "-----------------------------------------------------------------------------------"
               "-------------------------------------------"
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
                 "\n\rCumulative profiling info (Total %% is inclusive elapsed-wall time)\n\r\n\r"
                 "Section name                                                   |    Calls|  "
                 "Total usec| Total %%|  Avg usec|Median usec|"
                 "  P95 usec|  P99 usec|  Max usec|Samples stored/seen\n\r"
                 "---------------------------------------------------------------------------------"
                 "-----------------------------------------------"
                 "-----------------------------------------------\n\r"),
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
  memset(&pulse_event_lifecycle_stats, 0, sizeof(pulse_event_lifecycle_stats));
  memset(&total_event_lifecycle_stats, 0, sizeof(total_event_lifecycle_stats));
  memset(&pulse_extraction_stats, 0, sizeof(pulse_extraction_stats));
  memset(&total_extraction_stats, 0, sizeof(total_extraction_stats));
  memset(&pulse_catchup_stats, 0, sizeof(pulse_catchup_stats));
  memset(&total_catchup_stats, 0, sizeof(total_catchup_stats));
  memset(slow_pulses, 0, sizeof(slow_pulses));
  slow_pulse_index = 0;
  slow_pulse_count = 0;
  memset(&combat_context, 0, sizeof(combat_context));
  memset(slow_combats, 0, sizeof(slow_combats));
  slow_combat_index = 0;
  slow_combat_count = 0;
  combat_callbacks = 0;
  combat_slow_callbacks = 0;
  combat_limited_callbacks = 0;
  combat_rejected_attacks = 0;
  combat_rejected_procs = 0;
  pulse_schedule_flags = 0;
  pulse_last_heartbeat = 0;
  total_heartbeats_executed = 0;
  pthread_mutex_lock(&sql_stats_mutex);
  memset(&main_sql_stats, 0, sizeof(main_sql_stats));
  memset(&worker_sql_stats, 0, sizeof(worker_sql_stats));
  pulse_main_sql_calls = 0;
  pulse_main_sql_usec = 0;
  memset(sql_families, 0, sizeof(sql_families));
  sql_family_count = 0;
  sql_family_overflow_calls = 0;
  sql_reconnect_attempts = 0;
  sql_reconnect_successes = 0;
  sql_reconnect_failures = 0;
  perf_main_thread_set = 0;
  pthread_mutex_unlock(&sql_stats_mutex);
  memset(&boot_memory_stats, 0, sizeof(boot_memory_stats));
  memset(&reset_memory_stats, 0, sizeof(reset_memory_stats));
  memset(&latest_memory_stats, 0, sizeof(latest_memory_stats));
  latest_memory_stats_valid = 0;
  memory_boot_time_sec = 0;
  memory_reset_time_sec = 0;
  memory_peak_rss_kib = 0;
  memory_peak_anon_kib = 0;
  memory_last_alert_time_sec = 0;
  memset(memory_samples, 0, sizeof(memory_samples));
  memory_sample_index = 0;
  memory_sample_count = 0;
  prof_reset_wall_time_sec = 0;
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
  struct affected_type *af;
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
    {
      stats->count_mobs++;
      if (ch->master != NULL)
      {
        stats->count_npc_followers++;
        if (AFF_FLAGGED(ch, AFF_CHARM))
          stats->count_charmed_npcs++;
      }
    }
    else
      stats->count_pcs++;

    if (ch->affected != NULL)
      stats->count_affected_chars++;
    for (af = ch->affected; af; af = af->next)
      stats->count_affects++;
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

  latest_memory_stats = *stats;
  latest_memory_stats_valid = 1;

  return 1;
}

static uint64_t threshold_count_for(int threshold)
{
  size_t i;

  for (i = 0; i < sizeof(thresholds) / sizeof(thresholds[0]); i++)
    if (thresholds[i].threshold == threshold)
      return thresholds[i].count;
  return 0;
}

static void append_memory_sample(const struct perf_memory_stats *stats)
{
  struct perf_memory_sample *sample;
  size_t last_index;

  if (stats == NULL)
    return;
  if (memory_sample_count > 0)
  {
    last_index = (memory_sample_index + MEMORY_SAMPLE_CAPACITY - 1) % MEMORY_SAMPLE_CAPACITY;
    if (memory_samples[last_index].stats.timestamp_sec == stats->timestamp_sec)
      sample = &memory_samples[last_index];
    else
      sample = NULL;
  }
  else
    sample = NULL;
  if (sample == NULL)
  {
    sample = &memory_samples[memory_sample_index];
    memory_sample_index = (memory_sample_index + 1) % MEMORY_SAMPLE_CAPACITY;
    if (memory_sample_count < MEMORY_SAMPLE_CAPACITY)
      memory_sample_count++;
  }
  memset(sample, 0, sizeof(*sample));
  sample->stats = *stats;
  PERF_entity_totals(&sample->mobiles_created, &sample->mobiles_extracted, &sample->objects_created,
                     &sample->objects_extracted);
  pthread_mutex_lock(&sql_stats_mutex);
  sample->queries = main_sql_stats.calls + worker_sql_stats.calls;
  pthread_mutex_unlock(&sql_stats_mutex);
  sample->pulses_over_100 = threshold_count_for(100);
  sample->pulses_over_500 = threshold_count_for(500);
}

static const struct perf_memory_sample *memory_sample_newest(void)
{
  size_t index;

  if (memory_sample_count == 0)
    return NULL;
  index = (memory_sample_index + MEMORY_SAMPLE_CAPACITY - 1) % MEMORY_SAMPLE_CAPACITY;
  return &memory_samples[index];
}

static const struct perf_memory_sample *memory_sample_for_window(uint64_t window_sec)
{
  const struct perf_memory_sample *newest;
  const struct perf_memory_sample *candidate;
  size_t offset;
  size_t index;

  newest = memory_sample_newest();
  if (newest == NULL || memory_sample_count < 2)
    return NULL;
  candidate = NULL;
  for (offset = 1; offset < memory_sample_count; offset++)
  {
    index = (memory_sample_index + MEMORY_SAMPLE_CAPACITY - 1 - offset) % MEMORY_SAMPLE_CAPACITY;
    candidate = &memory_samples[index];
    if (newest->stats.timestamp_sec >= candidate->stats.timestamp_sec + window_sec)
      break;
  }
  return candidate;
}

static int calculate_memory_slope(uint64_t window_sec, struct perf_memory_slope *slope)
{
  const struct perf_memory_sample *newest;
  const struct perf_memory_sample *oldest;
  int64_t mob_delta;
  int64_t obj_delta;
  int64_t heap_delta;
  double elapsed_min;
  double explained_kib;

  if (slope == NULL)
    return 0;
  memset(slope, 0, sizeof(*slope));
  newest = memory_sample_newest();
  oldest = memory_sample_for_window(window_sec);
  if (newest == NULL || oldest == NULL ||
      newest->stats.timestamp_sec <= oldest->stats.timestamp_sec)
    return 0;
  slope->elapsed_sec = newest->stats.timestamp_sec - oldest->stats.timestamp_sec;
  elapsed_min = (double)slope->elapsed_sec / 60.0;
  mob_delta = (int64_t)newest->stats.count_mobs - (int64_t)oldest->stats.count_mobs;
  obj_delta = (int64_t)newest->stats.count_objs - (int64_t)oldest->stats.count_objs;
  heap_delta = (int64_t)newest->stats.heap_inuse_kib - (int64_t)oldest->stats.heap_inuse_kib;
  slope->rss_kib_per_min =
      ((double)((int64_t)newest->stats.vm_rss_kib - (int64_t)oldest->stats.vm_rss_kib)) /
      elapsed_min;
  slope->anon_kib_per_min =
      ((double)((int64_t)newest->stats.rss_anon_kib - (int64_t)oldest->stats.rss_anon_kib)) /
      elapsed_min;
  slope->heap_kib_per_min = (double)heap_delta / elapsed_min;
  slope->mobs_per_min = (double)mob_delta / elapsed_min;
  slope->objects_per_min = (double)obj_delta / elapsed_min;
  explained_kib =
      ((double)mob_delta * sizeof(struct char_data) + (double)obj_delta * sizeof(struct obj_data)) /
      1024.0;
  slope->residual_heap_kib_per_min = ((double)heap_delta - explained_kib) / elapsed_min;
  return 1;
}

int PERF_memory_growth_rate(double *rss_kib_per_min, double *anon_kib_per_min,
                            double *heap_kib_per_min)
{
  const struct perf_memory_stats *current;
  uint64_t now_sec;
  double elapsed_min;
  int64_t rss_diff, anon_diff, heap_diff;

  if (rss_kib_per_min)
    *rss_kib_per_min = 0.0;
  if (anon_kib_per_min)
    *anon_kib_per_min = 0.0;
  if (heap_kib_per_min)
    *heap_kib_per_min = 0.0;

  if (!latest_memory_stats_valid)
    return 0;

  current = &latest_memory_stats;
  now_sec = current->timestamp_sec;
  if (memory_reset_time_sec == 0 || now_sec <= memory_reset_time_sec)
    return 0;

  elapsed_min = (double)(now_sec - memory_reset_time_sec) / 60.0;
  if (elapsed_min < 0.1)
    return 0;

  rss_diff = (int64_t)current->vm_rss_kib - (int64_t)reset_memory_stats.vm_rss_kib;
  anon_diff = (int64_t)current->rss_anon_kib - (int64_t)reset_memory_stats.rss_anon_kib;
  heap_diff = (int64_t)current->heap_inuse_kib - (int64_t)reset_memory_stats.heap_inuse_kib;

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
  int64_t mob_delta;
  int64_t obj_delta;
  int64_t affect_delta;
  int64_t charmed_npc_delta;
  int64_t event_delta;
  uint64_t now_sec;

  if (!PERF_sample_memory(&current))
    return;

  now_sec = current.timestamp_sec;
  append_memory_sample(&current);

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
        mob_delta = (int64_t)current.count_mobs - (int64_t)reset_memory_stats.count_mobs;
        obj_delta = (int64_t)current.count_objs - (int64_t)reset_memory_stats.count_objs;
        affect_delta = (int64_t)current.count_affects - (int64_t)reset_memory_stats.count_affects;
        charmed_npc_delta =
            (int64_t)current.count_charmed_npcs - (int64_t)reset_memory_stats.count_charmed_npcs;
        event_delta = (int64_t)current.count_events - (int64_t)reset_memory_stats.count_events;
        memory_last_alert_time_sec = now_sec;
        log("PERFMON [MEMORY ALERT]: Elevated anonymous memory growth detected: +%.1f KiB/min "
            "(+%.2f MiB/hr). "
            "RSS: %llu KiB, Anon: %llu KiB, Heap in-use: %llu KiB. Live entities: %llu PCs, %llu "
            "mobs (%+lld), %llu objs (%+lld), %llu affects (%+lld), %llu charmed NPCs (%+lld), "
            "%llu events (%+lld).",
            anon_rate, (anon_rate * 60.0) / 1024.0, (unsigned long long)current.vm_rss_kib,
            (unsigned long long)current.rss_anon_kib, (unsigned long long)current.heap_inuse_kib,
            (unsigned long long)current.count_pcs, (unsigned long long)current.count_mobs,
            (long long)mob_delta, (unsigned long long)current.count_objs, (long long)obj_delta,
            (unsigned long long)current.count_affects, (long long)affect_delta,
            (unsigned long long)current.count_charmed_npcs, (long long)charmed_npc_delta,
            (unsigned long long)current.count_events, (long long)event_delta);
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
  int64_t char_delta;
  int64_t mob_delta;
  int64_t obj_delta;
  int64_t affected_char_delta;
  int64_t affect_delta;
  int64_t npc_follower_delta;
  int64_t charmed_npc_delta;
  int64_t event_delta;
  double rss_rate = 0.0, anon_rate = 0.0, heap_rate = 0.0;
  double anon_pct = 0.0;
  double explained_heap_kib;
  double residual_heap_kib;
  struct perf_memory_slope short_slope;
  struct perf_memory_slope medium_slope;
  struct perf_memory_slope long_slope;
  int has_short_slope;
  int has_medium_slope;
  int has_long_slope;
  const char *assessment = "STABLE";

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

  char_delta = (int64_t)cur.count_chars - (int64_t)reset_memory_stats.count_chars;
  mob_delta = (int64_t)cur.count_mobs - (int64_t)reset_memory_stats.count_mobs;
  obj_delta = (int64_t)cur.count_objs - (int64_t)reset_memory_stats.count_objs;
  affected_char_delta =
      (int64_t)cur.count_affected_chars - (int64_t)reset_memory_stats.count_affected_chars;
  affect_delta = (int64_t)cur.count_affects - (int64_t)reset_memory_stats.count_affects;
  npc_follower_delta =
      (int64_t)cur.count_npc_followers - (int64_t)reset_memory_stats.count_npc_followers;
  charmed_npc_delta =
      (int64_t)cur.count_charmed_npcs - (int64_t)reset_memory_stats.count_charmed_npcs;
  event_delta = (int64_t)cur.count_events - (int64_t)reset_memory_stats.count_events;

  PERF_memory_growth_rate(&rss_rate, &anon_rate, &heap_rate);
  has_short_slope = calculate_memory_slope(15 * 60, &short_slope);
  has_medium_slope = calculate_memory_slope(60 * 60, &medium_slope);
  has_long_slope = calculate_memory_slope(6 * 60 * 60, &long_slope);
  explained_heap_kib =
      ((double)mob_delta * sizeof(struct char_data) + (double)obj_delta * sizeof(struct obj_data)) /
      1024.0;
  residual_heap_kib = (double)heap_delta - explained_heap_kib;

  if (cur.vm_rss_kib > 0)
    anon_pct = ((double)cur.rss_anon_kib / (double)cur.vm_rss_kib) * 100.0;

  if (cur.vm_swap_kib > 0 && cur.vm_rss_kib > 0)
    assessment = "CRITICAL HEADROOM";
  else if (reset_elapsed_sec < 15 * 60)
    assessment = "WARMING";
  else if (has_short_slope && short_slope.anon_kib_per_min > 200.0)
  {
    if ((short_slope.mobs_per_min > 0.1 || short_slope.objects_per_min > 0.1) &&
        short_slope.residual_heap_kib_per_min < short_slope.heap_kib_per_min * 0.5)
      assessment = "GROWING WITH ENTITIES";
    else
      assessment = "UNEXPLAINED GROWTH";
  }
  else if (has_short_slope &&
           (fabs(short_slope.mobs_per_min) > 0.1 || fabs(short_slope.objects_per_min) > 0.1))
    assessment = "WARMING";
  else
    assessment = "STABLE";

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
          "  Characters in World:      %llu total (%llu PCs, %llu Mobs) [%+lld total, %+lld "
          "Mobs]\n\r"
          "  Objects in World:         %llu [%+lld since reset]\n\r"
          "  Spell Affect Nodes:       %llu across %llu characters [%+lld nodes, %+lld "
          "characters]\n\r"
          "  NPC Followers:            %llu total (%llu charmed) [%+lld total, %+lld charmed]\n\r"
          "  Rooms & Zones:            %llu rooms across %llu zones\n\r"
          "  Active Timed Events:      %llu [%+lld since reset]\n\r"
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
          (unsigned long long)cur.count_mobs, (long long)char_delta, (long long)mob_delta,
          (unsigned long long)cur.count_objs, (long long)obj_delta,
          (unsigned long long)cur.count_affects, (unsigned long long)cur.count_affected_chars,
          (long long)affect_delta, (long long)affected_char_delta,
          (unsigned long long)cur.count_npc_followers, (unsigned long long)cur.count_charmed_npcs,
          (long long)npc_follower_delta, (long long)charmed_npc_delta,
          (unsigned long long)cur.count_rooms, (unsigned long long)cur.count_zones,
          (unsigned long long)cur.count_events, (long long)event_delta,
          (unsigned long long)cur.count_pending_extractions),
      n);

  if (written < n - 1)
  {
    written += bounded_format_length(
        snprintf(
            out_buf + written, n - written,
            "Memory Time Series:        %zu/%d minute samples\n\r"
            "  Short slope (%" PRIu64 "s): RSS %+.1f, anon %+.1f, heap %+.1f KiB/min; "
            "mobs %+.2f, objects %+.2f/min; residual heap %+.1f KiB/min\n\r"
            "  Medium slope (%" PRIu64 "s): RSS %+.1f, anon %+.1f, heap %+.1f KiB/min; "
            "mobs %+.2f, objects %+.2f/min; residual heap %+.1f KiB/min\n\r"
            "  Long slope (%" PRIu64 "s): RSS %+.1f, anon %+.1f, heap %+.1f KiB/min; "
            "mobs %+.2f, objects %+.2f/min; residual heap %+.1f KiB/min\n\r"
            "  Entity base-size explanation since reset: %+.1f KiB; residual heap %+.1f KiB\n\r",
            memory_sample_count, MEMORY_SAMPLE_CAPACITY,
            has_short_slope ? short_slope.elapsed_sec : 0,
            has_short_slope ? short_slope.rss_kib_per_min : 0.0,
            has_short_slope ? short_slope.anon_kib_per_min : 0.0,
            has_short_slope ? short_slope.heap_kib_per_min : 0.0,
            has_short_slope ? short_slope.mobs_per_min : 0.0,
            has_short_slope ? short_slope.objects_per_min : 0.0,
            has_short_slope ? short_slope.residual_heap_kib_per_min : 0.0,
            has_medium_slope ? medium_slope.elapsed_sec : 0,
            has_medium_slope ? medium_slope.rss_kib_per_min : 0.0,
            has_medium_slope ? medium_slope.anon_kib_per_min : 0.0,
            has_medium_slope ? medium_slope.heap_kib_per_min : 0.0,
            has_medium_slope ? medium_slope.mobs_per_min : 0.0,
            has_medium_slope ? medium_slope.objects_per_min : 0.0,
            has_medium_slope ? medium_slope.residual_heap_kib_per_min : 0.0,
            has_long_slope ? long_slope.elapsed_sec : 0,
            has_long_slope ? long_slope.rss_kib_per_min : 0.0,
            has_long_slope ? long_slope.anon_kib_per_min : 0.0,
            has_long_slope ? long_slope.heap_kib_per_min : 0.0,
            has_long_slope ? long_slope.mobs_per_min : 0.0,
            has_long_slope ? long_slope.objects_per_min : 0.0,
            has_long_slope ? long_slope.residual_heap_kib_per_min : 0.0, explained_heap_kib,
            residual_heap_kib),
        n - written);
  }

  return written;
}

static size_t perf_memory_csv_with_current(char *out_buf, size_t n,
                                           const struct perf_memory_stats *current)
{
  struct perf_memory_stats cur;
  const struct perf_memory_sample *sample;
  double rss_rate = 0.0, anon_rate = 0.0, heap_rate = 0.0;
  int64_t char_delta;
  int64_t mob_delta;
  int64_t obj_delta;
  int64_t affected_char_delta;
  int64_t affect_delta;
  int64_t npc_follower_delta;
  int64_t charmed_npc_delta;
  int64_t event_delta;
  size_t written = 0;
  size_t offset;
  size_t index;

  if (!out_buf || n < 1 || current == NULL)
    return 0;

  cur = *current;
  PERF_memory_growth_rate(&rss_rate, &anon_rate, &heap_rate);
  char_delta = (int64_t)cur.count_chars - (int64_t)reset_memory_stats.count_chars;
  mob_delta = (int64_t)cur.count_mobs - (int64_t)reset_memory_stats.count_mobs;
  obj_delta = (int64_t)cur.count_objs - (int64_t)reset_memory_stats.count_objs;
  affected_char_delta =
      (int64_t)cur.count_affected_chars - (int64_t)reset_memory_stats.count_affected_chars;
  affect_delta = (int64_t)cur.count_affects - (int64_t)reset_memory_stats.count_affects;
  npc_follower_delta =
      (int64_t)cur.count_npc_followers - (int64_t)reset_memory_stats.count_npc_followers;
  charmed_npc_delta =
      (int64_t)cur.count_charmed_npcs - (int64_t)reset_memory_stats.count_charmed_npcs;
  event_delta = (int64_t)cur.count_events - (int64_t)reset_memory_stats.count_events;

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
               "# memory_count_affected_chars=%" PRIu64 "\n\r"
               "# memory_count_affects=%" PRIu64 "\n\r"
               "# memory_count_npc_followers=%" PRIu64 "\n\r"
               "# memory_count_charmed_npcs=%" PRIu64 "\n\r"
               "# memory_count_objs=%" PRIu64 "\n\r"
               "# memory_count_rooms=%" PRIu64 "\n\r"
               "# memory_count_zones=%" PRIu64 "\n\r"
               "# memory_count_events=%" PRIu64 "\n\r"
               "# memory_delta_count_chars_since_reset=%" PRId64 "\n\r"
               "# memory_delta_count_mobs_since_reset=%" PRId64 "\n\r"
               "# memory_delta_count_affected_chars_since_reset=%" PRId64 "\n\r"
               "# memory_delta_count_affects_since_reset=%" PRId64 "\n\r"
               "# memory_delta_count_npc_followers_since_reset=%" PRId64 "\n\r"
               "# memory_delta_count_charmed_npcs_since_reset=%" PRId64 "\n\r"
               "# memory_delta_count_objs_since_reset=%" PRId64 "\n\r"
               "# memory_delta_count_events_since_reset=%" PRId64 "\n\r"
               "# memory_count_pending_extractions=%" PRIu64 "\n\r",
               cur.timestamp_sec, cur.vm_size_kib, cur.vm_rss_kib, cur.rss_anon_kib,
               cur.rss_file_kib, cur.rss_shmem_kib, cur.vm_data_kib, cur.vm_swap_kib,
               cur.max_rss_kib, cur.heap_arena_kib, cur.heap_inuse_kib, cur.heap_free_kib,
               cur.heap_mmap_kib, rss_rate, anon_rate, heap_rate, cur.count_descriptors,
               cur.count_playing, cur.count_chars, cur.count_pcs, cur.count_mobs,
               cur.count_affected_chars, cur.count_affects, cur.count_npc_followers,
               cur.count_charmed_npcs, cur.count_objs, cur.count_rooms, cur.count_zones,
               cur.count_events, char_delta, mob_delta, affected_char_delta, affect_delta,
               npc_follower_delta, charmed_npc_delta, obj_delta, event_delta,
               cur.count_pending_extractions),
      n);

  if (written < n - 1)
    written += bounded_format_length(
        snprintf(out_buf + written, n - written,
                 "memory_history_timestamp_sec,rss_kib,anon_kib,heap_kib,arena_kib,mmap_kib,"
                 "swap_kib,pcs,mobs,objects,followers,affects,events,descriptors,pending,"
                 "mobiles_created,mobiles_extracted,objects_created,objects_extracted,queries,"
                 "pulses_over_100,pulses_over_500\n\r"),
        n - written);
  for (offset = memory_sample_count; offset > 0 && written < n - 1; offset--)
  {
    index = (memory_sample_index + MEMORY_SAMPLE_CAPACITY - offset) % MEMORY_SAMPLE_CAPACITY;
    sample = &memory_samples[index];
    written += bounded_format_length(
        snprintf(out_buf + written, n - written,
                 "%" PRIu64 ",%" PRIu64 ",%" PRIu64 ",%" PRIu64 ",%" PRIu64 ",%" PRIu64 ",%" PRIu64
                 ",%" PRIu64 ",%" PRIu64 ",%" PRIu64 ",%" PRIu64 ",%" PRIu64 ",%" PRIu64 ",%" PRIu64
                 ",%" PRIu64 ",%" PRIu64 ",%" PRIu64 ",%" PRIu64 ",%" PRIu64 ",%" PRIu64 ",%" PRIu64
                 ",%" PRIu64 "\n\r",
                 sample->stats.timestamp_sec, sample->stats.vm_rss_kib, sample->stats.rss_anon_kib,
                 sample->stats.heap_inuse_kib, sample->stats.heap_arena_kib,
                 sample->stats.heap_mmap_kib, sample->stats.vm_swap_kib, sample->stats.count_pcs,
                 sample->stats.count_mobs, sample->stats.count_objs,
                 sample->stats.count_npc_followers, sample->stats.count_affects,
                 sample->stats.count_events, sample->stats.count_descriptors,
                 sample->stats.count_pending_extractions, sample->mobiles_created,
                 sample->mobiles_extracted, sample->objects_created, sample->objects_extracted,
                 sample->queries, sample->pulses_over_100, sample->pulses_over_500),
        n - written);
  }

  return written;
}

size_t PERF_memory_csv(char *out_buf, size_t n)
{
  struct perf_memory_stats current;

  if (!out_buf || n < 1)
    return 0;
  if (!PERF_sample_memory(&current))
  {
    out_buf[0] = '\0';
    return 0;
  }
  return perf_memory_csv_with_current(out_buf, n, &current);
}

static int perf_snapshot_write_report(FILE *snapshot, char *buffer, size_t capacity, size_t written,
                                      const char *name)
{
  if (written >= capacity - 1)
  {
    errno = EOVERFLOW;
    log("SYSERR: PERFMON copyover snapshot section '%s' exceeded %zu bytes", name, capacity);
    return 0;
  }
  if (fprintf(snapshot, "\n# BEGIN %s\n", name) < 0)
    return 0;
  if (written > 0 && fwrite(buffer, 1, written, snapshot) != written)
    return 0;
  if (fprintf(snapshot, "\n# END %s\n", name) < 0)
    return 0;
  return 1;
}

int PERF_write_copyover_snapshot(const char *path)
{
  FILE *snapshot;
  char *buffer;
  char *temp_path;
  const char *failure;
  struct perf_memory_stats memory_snapshot;
  struct tm captured_tm;
  char captured_utc[32];
  size_t path_length;
  size_t written;
  time_t captured_at;
  long snapshot_size;
  int saved_errno;
  int temp_created;

  snapshot = NULL;
  buffer = NULL;
  temp_path = NULL;
  failure = "initialization";
  snapshot_size = 0;
  saved_errno = 0;
  temp_created = FALSE;

  if (path == NULL || *path == '\0')
  {
    errno = EINVAL;
    log("SYSERR: PERFMON copyover snapshot requires a destination path");
    return 0;
  }
  if (!PERF_sample_memory(&memory_snapshot))
  {
    log("SYSERR: PERFMON copyover snapshot could not sample current memory");
    return 0;
  }

  path_length = strlen(path);
  if (path_length > SIZE_MAX - 5)
  {
    errno = ENAMETOOLONG;
    log("SYSERR: PERFMON copyover snapshot path is too long");
    return 0;
  }
  temp_path = malloc(path_length + 5);
  buffer = malloc(COPYOVER_SNAPSHOT_BUFFER_SIZE);
  if (temp_path == NULL || buffer == NULL)
  {
    errno = ENOMEM;
    failure = "allocation";
    goto fail;
  }
  snprintf(temp_path, path_length + 5, "%s.tmp", path);

  snapshot = fopen_restricted(temp_path, "w");
  if (snapshot == NULL)
  {
    failure = "open temporary file";
    goto fail;
  }
  temp_created = TRUE;

  captured_at = time(NULL);
  if (gmtime_r(&captured_at, &captured_tm) == NULL ||
      strftime(captured_utc, sizeof(captured_utc), "%Y-%m-%dT%H:%M:%SZ", &captured_tm) == 0)
  {
    snprintf(captured_utc, sizeof(captured_utc), "%lld", (long long)captured_at);
  }
  if (fprintf(snapshot,
              "# LuminariMUD pre-copyover PERFMON snapshot\n"
              "# snapshot_format=1\n"
              "# captured_utc=%s\n"
              "# replacement=atomic_overwrite\n",
              captured_utc) < 0)
  {
    failure = "write header";
    goto fail;
  }

  written = PERF_repr(buffer, COPYOVER_SNAPSHOT_BUFFER_SIZE);
  if (!perf_snapshot_write_report(snapshot, buffer, COPYOVER_SNAPSHOT_BUFFER_SIZE, written,
                                  "health_summary"))
  {
    failure = "write health summary";
    goto fail;
  }
  written = persistence_scheduler_repr(buffer, COPYOVER_SNAPSHOT_BUFFER_SIZE);
  if (!perf_snapshot_write_report(snapshot, buffer, COPYOVER_SNAPSHOT_BUFFER_SIZE, written,
                                  "persistence_scheduler"))
  {
    failure = "write persistence scheduler";
    goto fail;
  }
  written = PERF_prof_repr_top(buffer, COPYOVER_SNAPSHOT_BUFFER_SIZE, "max", 20);
  if (!perf_snapshot_write_report(snapshot, buffer, COPYOVER_SNAPSHOT_BUFFER_SIZE, written,
                                  "top_max"))
  {
    failure = "write maximum ranking";
    goto fail;
  }
  written = PERF_prof_repr_top(buffer, COPYOVER_SNAPSHOT_BUFFER_SIZE, "p99", 20);
  if (!perf_snapshot_write_report(snapshot, buffer, COPYOVER_SNAPSHOT_BUFFER_SIZE, written,
                                  "top_p99"))
  {
    failure = "write p99 ranking";
    goto fail;
  }
  written = PERF_prof_repr_csv(buffer, COPYOVER_SNAPSHOT_BUFFER_SIZE);
  if (!perf_snapshot_write_report(snapshot, buffer, COPYOVER_SNAPSHOT_BUFFER_SIZE, written,
                                  "profiling_csv"))
  {
    failure = "write profiling CSV";
    goto fail;
  }
  written = PERF_sql_repr(buffer, COPYOVER_SNAPSHOT_BUFFER_SIZE, TRUE);
  if (!perf_snapshot_write_report(snapshot, buffer, COPYOVER_SNAPSHOT_BUFFER_SIZE, written,
                                  "sql_csv"))
  {
    failure = "write SQL CSV";
    goto fail;
  }
  written = PERF_slow_repr(buffer, COPYOVER_SNAPSHOT_BUFFER_SIZE, 128, TRUE);
  if (!perf_snapshot_write_report(snapshot, buffer, COPYOVER_SNAPSHOT_BUFFER_SIZE, written,
                                  "slow_pulses_csv"))
  {
    failure = "write slow-pulse CSV";
    goto fail;
  }
  written = PERF_combat_repr(buffer, COPYOVER_SNAPSHOT_BUFFER_SIZE, 64, TRUE);
  if (!perf_snapshot_write_report(snapshot, buffer, COPYOVER_SNAPSHOT_BUFFER_SIZE, written,
                                  "combat_csv"))
  {
    failure = "write combat CSV";
    goto fail;
  }
  written = perf_memory_csv_with_current(buffer, COPYOVER_SNAPSHOT_BUFFER_SIZE, &memory_snapshot);
  if (!perf_snapshot_write_report(snapshot, buffer, COPYOVER_SNAPSHOT_BUFFER_SIZE, written,
                                  "memory_csv"))
  {
    failure = "write memory CSV";
    goto fail;
  }
  written = PERF_entities_repr(buffer, COPYOVER_SNAPSHOT_BUFFER_SIZE, TRUE);
  if (!perf_snapshot_write_report(snapshot, buffer, COPYOVER_SNAPSHOT_BUFFER_SIZE, written,
                                  "entities_csv"))
  {
    failure = "write entity CSV";
    goto fail;
  }
  if (fprintf(snapshot, "\n# database_queries=%llu\n# snapshot_complete=1\n",
              (unsigned long long)mysql_query_counter_value()) < 0)
  {
    failure = "write footer";
    goto fail;
  }

  if (fflush(snapshot) != 0 || ferror(snapshot) || fsync(fileno(snapshot)) != 0)
  {
    failure = "flush temporary file";
    goto fail;
  }
  snapshot_size = ftell(snapshot);
  if (snapshot_size < 0)
  {
    failure = "measure temporary file";
    goto fail;
  }
  if (fclose(snapshot) != 0)
  {
    snapshot = NULL;
    failure = "close temporary file";
    goto fail;
  }
  snapshot = NULL;

  if (rename(temp_path, path) != 0)
  {
    failure = "replace snapshot";
    goto fail;
  }

  log("PERFMON [SNAPSHOT]: Wrote %ld-byte pre-copyover snapshot to %s", snapshot_size, path);
  free(buffer);
  free(temp_path);
  return 1;

fail:
  saved_errno = errno != 0 ? errno : EIO;
  if (snapshot != NULL)
    fclose(snapshot);
  if (temp_created)
    unlink(temp_path);
  free(buffer);
  free(temp_path);
  errno = saved_errno;
  log("SYSERR: PERFMON copyover snapshot failed during %s: %s", failure, strerror(saved_errno));
  return 0;
}
