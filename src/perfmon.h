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

/**
 * @brief Record one completed event callback invocation
 */
void PERF_note_event_callback(int profile_index, uint64_t elapsed_usec);

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

#endif /* PERFMON_H */
