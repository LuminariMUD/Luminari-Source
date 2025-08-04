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
#include <sys/time.h>
#include <float.h>
#include "perfmon.h"

/* ========================================================================
 * CONSTANTS
 * ======================================================================== */

#define MAX_PROF_SECTIONS 100  /* Maximum number of profiling sections */
#define USEC_PER_SEC 1000000   /* Microseconds per second */

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
struct perf_interval {
    double *avg_data;
    double *min_data;
    double *max_data;
    size_t size;
    size_t index;
    size_t count;
};

/* Performance threshold tracking */
static struct {
    int threshold;
    unsigned long count;
} thresholds[] = {
    {10, 0}, {30, 0}, {50, 0}, {70, 0}, {90, 0},
    {100, 0}, {250, 0}, {500, 0}, {1000, 0}, {2500, 0}
};

/* Profiling section structure */
struct PERF_prof_sect {
    char id[64];
    struct timeval last_enter;
    struct timeval pulse_total;
    struct timeval pulse_max;
    struct timeval total;
    struct timeval max;
    unsigned long pulse_enter_count;
    unsigned long pulse_exit_count;
    unsigned long total_enter_count;
};

/* ========================================================================
 * GLOBAL STATE
 * ======================================================================== */

/* Initialization tracking */
static int initialized = 0;
static time_t init_time;

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

/* Initialize a performance interval buffer */
static void init_interval(struct perf_interval *interval, size_t size) {
    interval->avg_data = calloc(size, sizeof(double));
    interval->min_data = calloc(size, sizeof(double));
    interval->max_data = calloc(size, sizeof(double));
    interval->size = size;
    interval->index = 0;
    interval->count = 0;
}


/* Add data to an interval buffer */
static void add_interval_data(struct perf_interval *interval, 
                              double avg, double min, double max) {
    if (!interval->avg_data) return;
    
    interval->avg_data[interval->index] = avg;
    interval->min_data[interval->index] = min;
    interval->max_data[interval->index] = max;
    
    if (interval->count <= interval->index) {
        interval->count = interval->index + 1;
    }
    
    interval->index++;
    if (interval->index >= interval->size) {
        interval->index = 0;
    }
}

/* Get average of averages from interval */
static double get_interval_avg(const struct perf_interval *interval) {
    double sum = 0.0;
    size_t i;
    
    if (interval->count == 0) return 0.0;
    
    for (i = 0; i < interval->count; i++) {
        sum += interval->avg_data[i];
    }
    
    return sum / interval->count;
}

/* Get minimum of minimums from interval */
static double get_interval_min(const struct perf_interval *interval) {
    double min = DBL_MAX;
    size_t i;
    
    if (interval->count == 0) return 0.0;
    
    for (i = 0; i < interval->count; i++) {
        if (interval->min_data[i] < min) {
            min = interval->min_data[i];
        }
    }
    
    return min;
}

/* Get maximum of maximums from interval */
static double get_interval_max(const struct perf_interval *interval) {
    double max = 0.0;
    size_t i;
    
    if (interval->count == 0) return 0.0;
    
    for (i = 0; i < interval->count; i++) {
        if (interval->max_data[i] > max) {
            max = interval->max_data[i];
        }
    }
    
    return max;
}

/* Initialize the performance monitoring system */
static void ensure_initialized(void) {
    if (initialized) return;
    
    init_time = time(NULL);
    
    /* Initialize interval buffers */
    init_interval(&pulse_data, PULSE_BUFFER_SIZE);
    init_interval(&sec_data, SEC_BUFFER_SIZE);
    init_interval(&min_data, MIN_BUFFER_SIZE);
    init_interval(&hour_data, HOUR_BUFFER_SIZE);
    
    initialized = 1;
}

/* Update threshold counters */
static void check_thresholds(double val) {
    size_t i;
    size_t count = sizeof(thresholds) / sizeof(thresholds[0]);
    
    for (i = 0; i < count; i++) {
        if (val > thresholds[i].threshold) {
            thresholds[i].count++;
        } else {
            break;  /* Thresholds are in ascending order */
        }
    }
}

/* Aggregate data from one level to the next */
static void aggregate_data(void) {
    /* When pulse buffer wraps, aggregate to seconds */
    if (pulse_data.index == 0 && pulse_data.count == pulse_data.size) {
        add_interval_data(&sec_data,
                          get_interval_avg(&pulse_data),
                          get_interval_min(&pulse_data),
                          get_interval_max(&pulse_data));
    }
    
    /* When seconds buffer wraps, aggregate to minutes */
    if (sec_data.index == 0 && sec_data.count == sec_data.size) {
        add_interval_data(&min_data,
                          get_interval_avg(&sec_data),
                          get_interval_min(&sec_data),
                          get_interval_max(&sec_data));
    }
    
    /* When minutes buffer wraps, aggregate to hours */
    if (min_data.index == 0 && min_data.count == min_data.size) {
        add_interval_data(&hour_data,
                          get_interval_avg(&min_data),
                          get_interval_min(&min_data),
                          get_interval_max(&hour_data));
    }
}

/* ========================================================================
 * PULSE MONITORING FUNCTIONS
 * ======================================================================== */

void PERF_log_pulse(double val) {
    ensure_initialized();
    
    last_pulse = val;
    
    if (val > max_pulse) {
        max_pulse = val;
    }
    
    check_thresholds(val);
    
    /* Add to pulse data buffer */
    add_interval_data(&pulse_data, val, val, val);
    
    /* Check for aggregation */
    aggregate_data();
}

size_t PERF_repr(char *out_buf, size_t n) {
    size_t written = 0;
    size_t i;
    time_t uptime;
    double total_pulses;
    double pulse_min, sec_min, min_min, hour_min;
    
    if (!out_buf || n < 1) return 0;
    
    ensure_initialized();
    
    uptime = time(NULL) - init_time;
    total_pulses = uptime * PULSE_PER_SECOND;
    
    /* Get minimum values */
    pulse_min = get_interval_min(&pulse_data);
    sec_min = get_interval_min(&sec_data);
    min_min = get_interval_min(&min_data);
    hour_min = get_interval_min(&hour_data);
    
    /* Format the report */
    written = snprintf(out_buf, n,
        "                     Avg         Min         Max\n\r"
        "  1 Pulse:   %10.2f%% %10.2f%% %10.2f%%\n\r"
        "%3zu Pulses:  %10.2f%% %10.2f%% %10.2f%%\n\r"
        "%3zu Seconds: %10.2f%% %10.2f%% %10.2f%%\n\r"
        "%3zu Minutes: %10.2f%% %10.2f%% %10.2f%%\n\r"
        "%3zu Hours:   %10.2f%% %10.2f%% %10.2f%%\n\r"
        "\n\rMax pulse:      %.2f\n\r\n\r",
        last_pulse, last_pulse, last_pulse,
        pulse_data.count, get_interval_avg(&pulse_data), pulse_min, get_interval_max(&pulse_data),
        sec_data.count, get_interval_avg(&sec_data), sec_min, get_interval_max(&sec_data),
        min_data.count, get_interval_avg(&min_data), min_min, get_interval_max(&min_data),
        hour_data.count, get_interval_avg(&hour_data), hour_min, get_interval_max(&hour_data),
        max_pulse);
    
    /* Add threshold statistics */
    for (i = 0; i < sizeof(thresholds) / sizeof(thresholds[0]) && written < n - 1; i++) {
        double percent = (total_pulses > 0) ? 
            (100.0 * thresholds[i].count / total_pulses) : 0.0;
        
        written += snprintf(out_buf + written, n - written,
            "Over %5d%%:      %.2f%% (%lu)\n\r",
            thresholds[i].threshold, percent, thresholds[i].count);
    }
    
    return written;
}

/* ========================================================================
 * CODE PROFILING FUNCTIONS
 * ======================================================================== */

void PERF_prof_sect_init(struct PERF_prof_sect **ptr, const char *id) {
    int i;
    
    if (!ptr || !id) return;
    
    /* If already initialized, return */
    if (*ptr) return;
    
    /* Look for existing section */
    for (i = 0; i < prof_section_count; i++) {
        if (strcmp(prof_sections[i]->id, id) == 0) {
            *ptr = prof_sections[i];
            return;
        }
    }
    
    /* Create new section if space available */
    if (prof_section_count < MAX_PROF_SECTIONS) {
        struct PERF_prof_sect *sect = calloc(1, sizeof(struct PERF_prof_sect));
        if (!sect) return;
        
        strncpy(sect->id, id, sizeof(sect->id) - 1);
        sect->id[sizeof(sect->id) - 1] = '\0';
        
        prof_sections[prof_section_count++] = sect;
        *ptr = sect;
    }
}

void PERF_prof_sect_enter(struct PERF_prof_sect *ptr) {
    if (!ptr) return;
    
    ptr->pulse_enter_count++;
    ptr->total_enter_count++;
    gettimeofday(&ptr->last_enter, NULL);
}

void PERF_prof_sect_exit(struct PERF_prof_sect *ptr) {
    struct timeval now, diff;
    
    if (!ptr) return;
    
    ptr->pulse_exit_count++;
    gettimeofday(&now, NULL);
    
    /* Calculate elapsed time */
    diff.tv_sec = now.tv_sec - ptr->last_enter.tv_sec;
    diff.tv_usec = now.tv_usec - ptr->last_enter.tv_usec;
    if (diff.tv_usec < 0) {
        diff.tv_sec--;
        diff.tv_usec += USEC_PER_SEC;
    }
    
    /* Add to pulse total */
    ptr->pulse_total.tv_sec += diff.tv_sec;
    ptr->pulse_total.tv_usec += diff.tv_usec;
    if (ptr->pulse_total.tv_usec >= USEC_PER_SEC) {
        ptr->pulse_total.tv_sec++;
        ptr->pulse_total.tv_usec -= USEC_PER_SEC;
    }
    
    /* Add to cumulative total */
    ptr->total.tv_sec += diff.tv_sec;
    ptr->total.tv_usec += diff.tv_usec;
    if (ptr->total.tv_usec >= USEC_PER_SEC) {
        ptr->total.tv_sec++;
        ptr->total.tv_usec -= USEC_PER_SEC;
    }
    
    /* Update pulse max if needed */
    if (diff.tv_sec > ptr->pulse_max.tv_sec ||
        (diff.tv_sec == ptr->pulse_max.tv_sec && diff.tv_usec > ptr->pulse_max.tv_usec)) {
        ptr->pulse_max = diff;
    }
    
    /* Update total max if needed */
    if (diff.tv_sec > ptr->max.tv_sec ||
        (diff.tv_sec == ptr->max.tv_sec && diff.tv_usec > ptr->max.tv_usec)) {
        ptr->max = diff;
    }
}

void PERF_prof_reset(void) {
    int i;
    
    for (i = 0; i < prof_section_count; i++) {
        struct PERF_prof_sect *sect = prof_sections[i];
        sect->pulse_enter_count = 0;
        sect->pulse_exit_count = 0;
        sect->pulse_total.tv_sec = 0;
        sect->pulse_total.tv_usec = 0;
        sect->pulse_max.tv_sec = 0;
        sect->pulse_max.tv_usec = 0;
    }
}

/* Helper function to format a profiling section */
static size_t format_prof_section(char *buf, size_t n, 
                                  const struct PERF_prof_sect *sect, 
                                  int is_total) {
    unsigned long enter_count;
    long usec_total, usec_max;
    double percent;
    
    if (is_total) {
        enter_count = sect->total_enter_count;
        usec_total = sect->total.tv_sec * USEC_PER_SEC + sect->total.tv_usec;
        usec_max = sect->max.tv_sec * USEC_PER_SEC + sect->max.tv_usec;
        
        /* Calculate percentage of total uptime */
        time_t uptime = time(NULL) - init_time;
        percent = (uptime > 0) ? (100.0 * usec_total / (uptime * USEC_PER_SEC)) : 0.0;
    } else {
        enter_count = sect->pulse_enter_count;
        usec_total = sect->pulse_total.tv_sec * USEC_PER_SEC + sect->pulse_total.tv_usec;
        usec_max = sect->pulse_max.tv_sec * USEC_PER_SEC + sect->pulse_max.tv_usec;
        
        /* Calculate percentage of pulse time */
        percent = (100.0 * usec_total) / USEC_PER_PULSE;
    }
    
    if (enter_count == 0) return 0;
    
    if (is_total) {
        return snprintf(buf, n, "%-20s|%12lu|%12ld|%11.2f%%|%19.2f%%\n\r",
                        sect->id, enter_count, usec_total, percent,
                        (100.0 * usec_max) / USEC_PER_PULSE);
    } else {
        return snprintf(buf, n, "%-20s|%12lu|%12lu|%12ld|%11.2f%%|%19.2f%%\n\r",
                        sect->id, enter_count, sect->pulse_exit_count, usec_total, percent,
                        (100.0 * usec_max) / USEC_PER_PULSE);
    }
}

size_t PERF_prof_repr_pulse(char *out_buf, size_t n) {
    size_t written = 0;
    int i;
    
    if (!out_buf || n < 1) return 0;
    
    written = snprintf(out_buf, n,
        "Pulse profiling info\n\r\n\r"
        "Section name        |Enter Count |Exit Count  |usec total  |pulse %%    |max pulse %% (1 entry)\n\r"
        "--------------------------------------------------------------------------------\n\r");
    
    for (i = 0; i < prof_section_count && written < n - 1; i++) {
        written += format_prof_section(out_buf + written, n - written, 
                                       prof_sections[i], 0);
    }
    
    return written;
}

size_t PERF_prof_repr_total(char *out_buf, size_t n) {
    size_t written = 0;
    int i;
    
    if (!out_buf || n < 1) return 0;
    
    ensure_initialized();
    
    written = snprintf(out_buf, n,
        "Cumulative profiling info\n\r\n\r"
        "Section name        |Enter Count |usec total  |total %%    |max pulse %% (1 entry)\n\r"
        "--------------------------------------------------------------------------------\n\r");
    
    for (i = 0; i < prof_section_count && written < n - 1; i++) {
        written += format_prof_section(out_buf + written, n - written, 
                                       prof_sections[i], 1);
    }
    
    return written;
}

size_t PERF_prof_repr_sect(char *out_buf, size_t n, const char *id) {
    size_t written = 0;
    int i;
    struct PERF_prof_sect *sect = NULL;
    
    if (!out_buf || n < 1 || !id) return 0;
    
    /* Find the section */
    for (i = 0; i < prof_section_count; i++) {
        if (strcmp(prof_sections[i]->id, id) == 0) {
            sect = prof_sections[i];
            break;
        }
    }
    
    if (!sect) {
        return snprintf(out_buf, n, "No such section '%s'\n\r", id);
    }
    
    /* Generate both pulse and total reports for this section */
    written = snprintf(out_buf, n,
        "Pulse profiling info\n\r\n\r"
        "Section name        |Enter Count |Exit Count  |usec total  |pulse %%    |max pulse %% (1 entry)\n\r"
        "--------------------------------------------------------------------------------\n\r");
    
    written += format_prof_section(out_buf + written, n - written, sect, 0);
    
    if (written < n - 1) {
        written += snprintf(out_buf + written, n - written,
            "\n\rCumulative profiling info\n\r\n\r"
            "Section name        |Enter Count |usec total  |total %%    |max pulse %% (1 entry)\n\r"
            "--------------------------------------------------------------------------------\n\r");
        
        written += format_prof_section(out_buf + written, n - written, sect, 1);
    }
    
    return written;
}