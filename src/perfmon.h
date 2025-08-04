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
 * @brief Generate report for a specific profiling section
 *
 * @param out_buf Buffer to store the report
 * @param n Size of the buffer
 * @param id Identifier of the section to report on
 * @return Number of characters written
 */
size_t PERF_prof_repr_sect(char *out_buf, size_t n, const char *id);

/* ========================================================================
 * CONVENIENCE MACROS
 * ======================================================================== */

/**
 * @brief Macro to begin profiling a code section
 *
 * @param sect Variable name for the profiling section
 * @param sect_descr String description/identifier for this section
 */
#define PERF_PROF_ENTER(sect, sect_descr) \
    static struct PERF_prof_sect *sect = NULL; \
    PERF_prof_sect_init(&sect, sect_descr); \
    PERF_prof_sect_enter(sect)

/**
 * @brief Macro to end profiling a code section
 *
 * @param sect Variable name of the profiling section
 */
#define PERF_PROF_EXIT(sect) \
    PERF_prof_sect_exit(sect)

#endif /* PERFMON_H */