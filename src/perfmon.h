/**
 * @file perfmon.h
 * @brief Performance Monitoring System for Game Server
 *
 * This header provides a comprehensive performance monitoring system designed for
 * real-time game servers. It tracks two main types of performance data:
 *
 * 1. PULSE MONITORING: Tracks server "pulse" performance (main game loop timing)
 *    - Records pulse duration as percentage of maximum allowed time
 *    - Maintains hierarchical statistics (pulse -> second -> minute -> hour -> day)
 *    - Tracks threshold violations for performance alerts
 *
 * 2. CODE PROFILING: Tracks execution time of specific code sections
 *    - Measures time spent in different parts of the codebase
 *    - Provides both per-pulse and cumulative statistics
 *    - Helps identify performance bottlenecks
 *
 * The system is designed to be lightweight and suitable for production use in
 * real-time applications where performance monitoring is critical.
 */

#ifndef PERFMON_H
#define PERFMON_H

/**
 * @brief Number of pulses (game loop iterations) per second
 *
 * This constant defines how many times per second the main game loop should
 * execute. It's used as the baseline for calculating performance percentages.
 * For example, if PERF_pulse_per_second = 10, then each pulse should take
 * no more than 100ms (1000ms / 10 pulses).
 */
extern const unsigned PERF_pulse_per_second;

/* ========================================================================
 * PULSE MONITORING FUNCTIONS
 * ========================================================================
 * These functions track the performance of the main game loop ("pulse").
 * Each pulse represents one iteration of the main server loop.
 */

/**
 * @brief Log the duration of a single pulse
 *
 * Records the time taken for one game loop iteration as a percentage of the
 * maximum allowed time. This data is used to build hierarchical performance
 * statistics and track performance threshold violations.
 *
 * @param val Pulse duration as percentage (0-100+). Values over 100% indicate
 *            the pulse took longer than the allocated time slice.
 *
 * Example usage:
 * @code
 * // If pulse should take 100ms but actually took 150ms:
 * PERF_log_pulse(150.0);  // 150% of allowed time
 * @endcode
 */
void PERF_log_pulse(double val);

/**
 * @brief Optimized pulse logging with configurable monitoring levels
 *
 * This function provides a more efficient alternative to PERF_log_pulse()
 * with support for different monitoring levels and sampling rates to reduce
 * overhead in production environments.
 *
 * @param val Pulse duration as percentage (0-100+)
 */
void PERF_log_pulse_optimized(double val);

/**
 * @brief Generate a formatted performance report
 *
 * Creates a comprehensive text report showing pulse performance statistics
 * across different time intervals (pulses, seconds, minutes, hours) and
 * threshold violation counts.
 *
 * @param out_buf Buffer to store the formatted report
 * @param n Size of the output buffer
 * @return Number of characters written to the buffer (excluding null terminator)
 *
 * The report includes:
 * - Average, minimum, and maximum performance for each time interval
 * - Maximum pulse time ever recorded
 * - Percentage of time spent above various performance thresholds
 */
size_t PERF_repr( char *out_buf, size_t n );

/* ========================================================================
 * CODE PROFILING SYSTEM
 * ========================================================================
 * These functions and structures provide detailed timing information for
 * specific sections of code, helping identify performance bottlenecks.
 */

/**
 * @brief Opaque structure representing a profiled code section
 *
 * This structure tracks timing information for a specific named section of code.
 * It maintains both per-pulse statistics (reset each game loop) and cumulative
 * statistics (accumulated over the entire server runtime).
 *
 * Users should not access this structure directly - use the provided functions
 * and macros instead.
 */
struct PERF_prof_sect;

/**
 * @brief Initialize a profiling section
 *
 * Creates or retrieves a profiling section with the given identifier.
 * This function uses lazy initialization - the section is created only
 * on first use and reused on subsequent calls.
 *
 * @param ptr Pointer to section pointer (will be set to the section instance)
 * @param id Unique string identifier for this profiling section
 *
 * Note: This function is typically called automatically by the PERF_PROF_ENTER macro.
 */
void PERF_prof_sect_init(struct PERF_prof_sect **ptr, const char *id);

/**
 * @brief Mark the beginning of a profiled code section
 *
 * Records the current timestamp as the start time for timing this code section.
 * Must be paired with a corresponding PERF_prof_sect_exit() call.
 *
 * @param ptr Pointer to the profiling section (from PERF_prof_sect_init)
 */
void PERF_prof_sect_enter(struct PERF_prof_sect *ptr);

/**
 * @brief Mark the end of a profiled code section
 *
 * Records the current timestamp and calculates the elapsed time since the
 * corresponding PERF_prof_sect_enter() call. Updates both per-pulse and
 * cumulative statistics for this section.
 *
 * @param ptr Pointer to the profiling section (from PERF_prof_sect_init)
 */
void PERF_prof_sect_exit(struct PERF_prof_sect *ptr);

/**
 * @brief Reset per-pulse profiling statistics
 *
 * Clears the per-pulse timing data for all profiling sections. This is
 * typically called at the beginning of each game loop iteration to start
 * fresh timing measurements for the new pulse.
 *
 * Note: This only resets per-pulse data; cumulative statistics are preserved.
 */
void PERF_prof_reset( void );

/**
 * @brief Generate per-pulse profiling report
 *
 * Creates a formatted report showing timing statistics for the current pulse
 * (since the last call to PERF_prof_reset). Shows how much time each code
 * section consumed during the most recent game loop iteration.
 *
 * @param out_buf Buffer to store the formatted report
 * @param n Size of the output buffer
 * @return Number of characters written to the buffer (excluding null terminator)
 */
size_t PERF_prof_repr_pulse( char *out_buf, size_t n );

/**
 * @brief Generate cumulative profiling report
 *
 * Creates a formatted report showing cumulative timing statistics since
 * server startup. Shows total time spent in each code section and what
 * percentage of total server runtime each section represents.
 *
 * @param out_buf Buffer to store the formatted report
 * @param n Size of the output buffer
 * @return Number of characters written to the buffer (excluding null terminator)
 */
size_t PERF_prof_repr_total( char *out_buf, size_t n );

/**
 * @brief Generate report for a specific profiling section
 *
 * Creates a detailed report for a single named profiling section, showing
 * both per-pulse and cumulative statistics for that section only.
 *
 * @param out_buf Buffer to store the formatted report
 * @param n Size of the output buffer
 * @param id String identifier of the section to report on
 * @return Number of characters written to the buffer (excluding null terminator)
 */
size_t PERF_prof_repr_sect( char *out_buf, size_t n, const char *id );

/* ========================================================================
 * CONVENIENCE MACROS
 * ========================================================================
 * These macros provide an easy way to add profiling to code sections
 * without manual management of profiling section structures.
 */

/**
 * @brief Macro to begin profiling a code section
 *
 * This macro handles the initialization and entry into a profiling section.
 * It creates a static profiling section variable and marks the beginning
 * of the timed code section.
 *
 * @param sect Variable name for the profiling section (will be declared static)
 * @param sect_descr String description/identifier for this section
 *
 * Usage example:
 * @code
 * void my_function() {
 *     PERF_PROF_ENTER(combat_section, "combat_processing");
 *
 *     // ... combat processing code here ...
 *
 *     PERF_PROF_EXIT(combat_section);
 * }
 * @endcode
 *
 * The static variable ensures the profiling section is initialized only once
 * per function, improving performance on subsequent calls.
 */
#define PERF_PROF_ENTER( sect, sect_descr ) \
    static struct PERF_prof_sect * sect = NULL; \
    PERF_prof_sect_init( & sect, sect_descr ); \
    PERF_prof_sect_enter( sect )

/**
 * @brief Macro to end profiling a code section
 *
 * This macro marks the end of a profiled code section and records the
 * elapsed time. Must be paired with a corresponding PERF_PROF_ENTER call.
 *
 * @param sect Variable name of the profiling section (from PERF_PROF_ENTER)
 */
#define PERF_PROF_EXIT( sect ) \
    PERF_prof_sect_exit( sect )

#endif // PERFMON_H
