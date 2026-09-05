/**
 * @file perfmon.h
 * @brief Simple Performance Monitoring System for LuminariMUD
 *
 * This header provides a lightweight performance monitoring system for
 * tracking server pulse performance and code section profiling.
 */

#ifndef PERFMON_H
#define PERFMON_H

#include <stddef.h>
#include <stdint.h>

#define PERFMON_COPYOVER_SNAPSHOT_FILE "../log/perfmon-pre-copyover.txt"
#define PERF_EVENT_IDENTITY_SIZE 64

struct PERF_event_summary
{
  uint64_t process_calls;
  uint64_t callbacks;
  uint64_t current_depth;
  uint64_t high_water_depth;
  uint64_t maximum_batch;
  uint64_t scheduled;
  uint64_t cancelled;
  uint64_t rescheduled;
  size_t registered_profiles;
  uint64_t overflow_calls;
};

struct PERF_event_profile_snapshot
{
  char identity[PERF_EVENT_IDENTITY_SIZE];
  uint64_t calls;
  uint64_t total_usec;
  uint64_t maximum_usec;
  uint64_t scheduled;
  uint64_t cancelled;
  uint64_t rescheduled;
};

/* Heartbeat schedule classes retained in slow-pulse flight records. */
enum perf_schedule_flag
{
  PERF_SCHEDULE_1_SECOND = 1ULL << 0,
  PERF_SCHEDULE_3_SECONDS = 1ULL << 1,
  PERF_SCHEDULE_5_SECONDS = 1ULL << 2,
  PERF_SCHEDULE_6_SECONDS = 1ULL << 3,
  PERF_SCHEDULE_13_SECONDS = 1ULL << 4,
  PERF_SCHEDULE_30_SECONDS = 1ULL << 5,
  PERF_SCHEDULE_60_SECONDS = 1ULL << 6,
  PERF_SCHEDULE_75_SECONDS = 1ULL << 7,
  PERF_SCHEDULE_AUTOSAVE = 1ULL << 8,
  PERF_SCHEDULE_LONG_INTERVAL = 1ULL << 9
};

/* Explicit SQL owners. Unscoped callers remain visible as "other". */
enum perf_sql_category
{
  PERF_SQL_OTHER = 0,
  PERF_SQL_ACCOUNT,
  PERF_SQL_CHARACTER,
  PERF_SQL_PET,
  PERF_SQL_CRASH_OBJECT,
  PERF_SQL_HOUSE,
  PERF_SQL_LAST_ONLINE,
  PERF_SQL_ARTIFACT,
  PERF_SQL_CATEGORY_COUNT
};

/* Stable entity-creation owners used by the lifecycle report. */
enum perf_entity_reason
{
  PERF_ENTITY_UNKNOWN = 0,
  PERF_ENTITY_BOOT,
  PERF_ENTITY_ZONE_RESET,
  PERF_ENTITY_DG_SCRIPT,
  PERF_ENTITY_SPELL_SUMMON,
  PERF_ENTITY_ENCOUNTER,
  PERF_ENTITY_QUEST,
  PERF_ENTITY_VESSEL,
  PERF_ENTITY_PET_RESTORE,
  PERF_ENTITY_SPECIAL,
  PERF_ENTITY_STAFF,
  PERF_ENTITY_REASON_COUNT
};

enum perf_sweep_kind
{
  PERF_SWEEP_AUTOPROC = 0,
  PERF_SWEEP_DG_MOBILE_RANDOM,
  PERF_SWEEP_DG_OBJECT_RANDOM,
  PERF_SWEEP_DG_ROOM_RANDOM,
  PERF_SWEEP_AFFECT,
  PERF_SWEEP_COUNT
};

/* Number of pulses per second (defined elsewhere in the codebase) */
extern const unsigned PERF_pulse_per_second;

/* ========================================================================
 * PULSE MONITORING
 * ======================================================================== */

/**
 * @brief Log the performance of a single game loop pulse
 *
 * @param val Performance as percentage of allocated time (e.g., 85.5 = 85.5%)
 *            Values over 100% indicate the pulse took longer than allocated
 */
void PERF_log_pulse(double val);

/** Record the logical heartbeat most recently executed in this outer loop. */
void PERF_note_heartbeat(uint64_t pulse_number);
void PERF_note_runtime_advance(uint64_t pulse_number, uint64_t elapsed_ticks);

/** Add one or more schedule-class bits to the current outer-loop context. */
void PERF_note_schedule(uint64_t schedule_flags);

/**
 * @brief Add game-loop heartbeats missed during a lagged pass
 *
 * @param count Number of missed heartbeats, excluding the normal pass
 */
void PERF_note_missed_pulses(uint64_t count);

/**
 * @brief Record one repetitive vessel message suppressed by its cooldown
 */
void PERF_note_vessel_message_throttled(void);

/**
 * @brief Return the current monotonic clock value in microseconds
 */
uint64_t PERF_monotonic_usec(void);

/**
 * @brief Register one stable event callback identity for bounded telemetry
 *
 * @return Non-negative aggregate slot, or -1 when the registry is full
 */
int PERF_register_event_callback(const char *identity);

/** Return a stable read-only identity for one callback registry slot. */
const char *PERF_event_callback_identity(int profile_index);

/** Copy cumulative event telemetry into compact operator-facing records. */
void PERF_get_event_summary(struct PERF_event_summary *summary);
size_t PERF_get_event_profiles(struct PERF_event_profile_snapshot *snapshots,
                               size_t snapshot_capacity);

/**
 * @brief Record one completed event callback invocation
 */
void PERF_note_event_callback(int profile_index, uint64_t elapsed_usec);

/** Record a successfully admitted event and its requested delay in pulses. */
void PERF_note_event_scheduled(int profile_index, uint64_t delay_pulses);

/** Record cancellation of a live event. */
void PERF_note_event_cancelled(int profile_index);

/** Record a callback-requested recurrence and its delay in pulses. */
void PERF_note_event_rescheduled(int profile_index, uint64_t delay_pulses);

/**
 * @brief Record one event_process() queue pass
 */
void PERF_note_event_process(uint64_t depth_before, uint64_t depth_after,
                             uint64_t callbacks_processed, uint64_t events_created);

/**
 * @brief Record pending-character extraction work completed by one call
 */
void PERF_note_pending_extractions(uint64_t pending_before, uint64_t processed,
                                   uint64_t pending_after);

/**
 * @brief Record one slow-loop heartbeat recovery plan
 */
void PERF_note_catchup_pass(uint64_t requested_missed, uint64_t replayed_missed,
                            uint64_t remaining_backlog, int budget_exhausted);

/**
 * @brief Return missed heartbeats recorded since the last reset
 */
uint64_t PERF_missed_pulse_count(void);

/**
 * @brief Return vessel messages suppressed since the last reset
 */
uint64_t PERF_vessel_message_throttled_count(void);

/**
 * @brief Reset pulse and cumulative profiling statistics
 */
void PERF_reset(void);

/**
 * @brief Release all performance-monitoring allocations at final shutdown
 *
 * Profiling macros cache section pointers, so no profiling call may follow
 * this final process cleanup.
 */
void PERF_cleanup(void);

/**
 * @brief Generate a performance report
 *
 * @param out_buf Buffer to store the formatted report
 * @param n Size of the output buffer
 * @return Number of characters written to the buffer
 */
size_t PERF_repr(char *out_buf, size_t n);

/**
 * Format the bounded slow-pulse flight recorder.
 *
 * @param out_buf Destination buffer.
 * @param n Destination capacity.
 * @param count Number of newest records to print; zero selects the default.
 * @param csv Non-zero selects machine-readable CSV.
 */
size_t PERF_slow_repr(char *out_buf, size_t n, size_t count, int csv);

/* ========================================================================
 * SQL MONITORING
 * ======================================================================== */

/** Set the current thread's explicit query owner and return the prior owner. */
enum perf_sql_category PERF_sql_scope_set(enum perf_sql_category category);

/** Restore a query owner returned by PERF_sql_scope_set(). */
void PERF_sql_scope_restore(enum perf_sql_category category);

/** Record one query without retaining SQL values or full statement text. */
void PERF_note_sql_query(const char *query, uint64_t elapsed_usec, int failed);

/** Record a reconnect attempt and whether it restored service. */
void PERF_note_sql_reconnect(int succeeded);

/** Format cumulative SQL latency and normalized-family telemetry. */
size_t PERF_sql_repr(char *out_buf, size_t n, int csv);

/* ========================================================================
 * ENTITY LIFECYCLE MONITORING
 * ======================================================================== */

/** Set the creation owner for nested mobile/object constructors. */
enum perf_entity_reason PERF_entity_scope_set(enum perf_entity_reason reason);

/** Restore a creation owner returned by PERF_entity_scope_set(). */
void PERF_entity_scope_restore(enum perf_entity_reason reason);

/** Return the current scoped entity owner for instance metadata. */
enum perf_entity_reason PERF_entity_current_reason(void);

void PERF_note_mobile_created(int vnum, int zone_vnum, enum perf_entity_reason reason);
void PERF_note_mobile_extracted(int vnum, int zone_vnum, enum perf_entity_reason reason);
void PERF_note_object_created(int vnum, int zone_vnum, enum perf_entity_reason reason);
void PERF_note_object_extracted(int vnum, int zone_vnum, enum perf_entity_reason reason);
void PERF_note_zone_reset(int zone_vnum, uint64_t elapsed_usec, uint64_t mobiles_created,
                          uint64_t mobiles_extracted, uint64_t objects_created,
                          uint64_t objects_extracted);
void PERF_entity_totals(uint64_t *mobiles_created, uint64_t *mobiles_extracted,
                        uint64_t *objects_created, uint64_t *objects_extracted);
size_t PERF_entities_repr(char *out_buf, size_t n, int csv);

/** Record population work so reports distinguish visited, eligible, and acted-on owners. */
void PERF_note_sweep(enum perf_sweep_kind kind, uint64_t visited, uint64_t eligible,
                     uint64_t acted);

/* ========================================================================
 * COMBAT CALLBACK MONITORING
 * ======================================================================== */

struct char_data;

/** Begin safe, bounded context collection for one combat-round callback. */
void PERF_combat_round_begin(const struct char_data *ch);

/** Count one generated attack and reject work beyond the per-callback safety limit. */
int PERF_combat_allow_attack(void);

/** Count one combat special/proc and reject work beyond the per-callback safety limit. */
int PERF_combat_allow_proc(void);

/** Complete the current callback and retain it when slow or guard-limited. */
void PERF_combat_round_end(void);

/** Format bounded slow-combat and chain-limit telemetry. */
size_t PERF_combat_repr(char *out_buf, size_t n, size_t count, int csv);

/* ========================================================================
 * CODE PROFILING
 * ======================================================================== */

/* Opaque structure for profiling sections */
struct PERF_prof_sect;

/**
 * @brief Initialize a profiling section
 *
 * @param ptr Pointer to section pointer (will be set to the section instance)
 * @param id Unique string identifier for this profiling section
 */
void PERF_prof_sect_init(struct PERF_prof_sect **ptr, const char *id);

/**
 * @brief Mark the beginning of a profiled code section
 *
 * @param ptr Pointer to the profiling section
 */
void PERF_prof_sect_enter(struct PERF_prof_sect *ptr);

/**
 * @brief Enable rolling percentile samples for a selected profiling section
 *
 * Sampling is opt-in because command and special-function wrappers can create
 * hundreds of sections over a long-running server process.
 *
 * @param ptr Pointer to the profiling section
 */
void PERF_prof_sect_enable_sampling(struct PERF_prof_sect *ptr);

/**
 * @brief Mark the end of a profiled code section
 *
 * @param ptr Pointer to the profiling section
 */
void PERF_prof_sect_exit(struct PERF_prof_sect *ptr);

/**
 * @brief Reset per-pulse profiling statistics
 */
void PERF_prof_reset(void);

/**
 * @brief Generate per-pulse profiling report
 *
 * @param out_buf Buffer to store the report
 * @param n Size of the buffer
 * @return Number of characters written
 */
size_t PERF_prof_repr_pulse(char *out_buf, size_t n);

/**
 * @brief Generate cumulative profiling report
 *
 * @param out_buf Buffer to store the report
 * @param n Size of the buffer
 * @return Number of characters written
 */
size_t PERF_prof_repr_total(char *out_buf, size_t n);

/** Format cumulative sections ranked by total time, maximum, or rolling p99. */
size_t PERF_prof_repr_top(char *out_buf, size_t n, const char *metric, size_t limit);

/**
 * @brief Generate machine-readable cumulative profiling data
 *
 * @param out_buf Buffer to store CSV rows
 * @param n Size of the buffer
 * @return Number of characters written
 */
size_t PERF_prof_repr_csv(char *out_buf, size_t n);

/**
 * @brief Generate report for a specific profiling section
 *
 * @param out_buf Buffer to store the report
 * @param n Size of the buffer
 * @param id Identifier of the section to report on
 * @return Number of characters written
 */
size_t PERF_prof_repr_sect(char *out_buf, size_t n, const char *id);

/**
 * @brief Calculate a linearly interpolated percentile from microsecond samples
 *
 * @param samples Sample array
 * @param count Number of samples
 * @param percentile Percentile in the inclusive range 0 through 100
 * @return Calculated percentile, or 0 for invalid input or allocation failure
 */
double PERF_calculate_percentile(const uint64_t *samples, size_t count, double percentile);

/* ========================================================================
 * MEMORY MONITORING
 * ======================================================================== */

struct perf_memory_stats
{
  uint64_t timestamp_sec;
  /* Process OS memory from /proc/self/status and rusage */
  uint64_t vm_size_kib;
  uint64_t vm_rss_kib;
  uint64_t rss_anon_kib;
  uint64_t rss_file_kib;
  uint64_t rss_shmem_kib;
  uint64_t vm_data_kib;
  uint64_t vm_swap_kib;
  uint64_t max_rss_kib;

  /* Glibc allocator metrics from mallinfo2() / mallinfo() */
  uint64_t heap_arena_kib;
  uint64_t heap_inuse_kib;
  uint64_t heap_free_kib;
  uint64_t heap_mmap_kib;

  /* Entity / subsystem resource counts */
  uint64_t count_descriptors;
  uint64_t count_playing;
  uint64_t count_chars;
  uint64_t count_pcs;
  uint64_t count_mobs;
  uint64_t count_affected_chars;
  uint64_t count_affects;
  uint64_t count_npc_followers;
  uint64_t count_charmed_npcs;
  uint64_t count_objs;
  uint64_t count_rooms;
  uint64_t count_zones;
  uint64_t count_events;
  uint64_t count_pending_extractions;
};

/**
 * @brief Sample current process memory usage and internal entity counts.
 *
 * @param stats Pointer to struct to receive sampled statistics.
 * @return 1 on success, 0 on failure.
 */
int PERF_sample_memory(struct perf_memory_stats *stats);

/**
 * @brief Generate a human-readable memory monitoring dashboard report.
 *
 * @param out_buf Buffer to store formatted report.
 * @param n Size of the buffer.
 * @return Number of characters written.
 */
size_t PERF_memory_repr(char *out_buf, size_t n);

/**
 * @brief Generate CSV rows for memory metrics.
 *
 * @param out_buf Buffer to store formatted report.
 * @param n Size of the buffer.
 * @return Number of characters written.
 */
size_t PERF_memory_csv(char *out_buf, size_t n);

/**
 * Atomically replace the durable pre-copyover snapshot with current telemetry.
 *
 * The previous complete snapshot remains intact if rendering or writing fails.
 *
 * @param path Destination path, normally PERFMON_COPYOVER_SNAPSHOT_FILE.
 * @return 1 on success, 0 on failure.
 */
int PERF_write_copyover_snapshot(const char *path);

/**
 * @brief Perform periodic check of memory growth and log alerts if threshold is exceeded.
 */
void PERF_memory_periodic_check(void);

/**
 * @brief Calculate rolling memory growth rates since reset/boot.
 *
 * @param rss_kib_per_min Pointer to receive RSS growth rate in KiB/minute.
 * @param anon_kib_per_min Pointer to receive Anon RSS growth rate in KiB/minute.
 * @param heap_kib_per_min Pointer to receive Heap in-use growth rate in KiB/minute.
 * @return 1 if growth rate could be calculated, 0 otherwise.
 */
int PERF_memory_growth_rate(double *rss_kib_per_min, double *anon_kib_per_min,
                            double *heap_kib_per_min);

/* ========================================================================
 * CONVENIENCE MACROS
 * ======================================================================== */

/**
 * @brief Macro to begin profiling a code section
 *
 * @param sect Variable name for the profiling section
 * @param sect_descr String description/identifier for this section
 */
#define PERF_PROF_ENTER(sect, sect_descr)                                                          \
  static struct PERF_prof_sect *sect = NULL;                                                       \
  PERF_prof_sect_init(&sect, sect_descr);                                                          \
  PERF_prof_sect_enter(sect)

/**
 * @brief Macro to end profiling a code section
 *
 * @param sect Variable name of the profiling section
 */
#define PERF_PROF_EXIT(sect) PERF_prof_sect_exit(sect)

/** Begin a profiled section with bounded rolling percentile sampling enabled. */
#define PERF_PROF_ENTER_SAMPLED(sect, sect_descr)                                                  \
  static struct PERF_prof_sect *sect = NULL;                                                       \
  PERF_prof_sect_init(&sect, sect_descr);                                                          \
  PERF_prof_sect_enable_sampling(sect);                                                            \
  PERF_prof_sect_enter(sect)

#endif /* PERFMON_H */
