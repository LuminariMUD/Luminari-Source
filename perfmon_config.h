/**
 * @file perfmon_config.h
 * @brief Performance monitoring configuration options
 */

#ifndef PERFMON_CONFIG_H
#define PERFMON_CONFIG_H

/* Performance monitoring levels */
#define PERF_LEVEL_OFF      0  /* No monitoring */
#define PERF_LEVEL_BASIC    1  /* Basic pulse monitoring only */
#define PERF_LEVEL_SAMPLING 2  /* Sample every N pulses */
#define PERF_LEVEL_FULL     3  /* Full monitoring (current behavior) */

/* Default configuration */
#define PERF_DEFAULT_LEVEL  PERF_LEVEL_SAMPLING
#define PERF_SAMPLE_RATE    10  /* Sample 1 in every 10 pulses */

/* Dynamic load-based monitoring */
#define PERF_LOAD_THRESHOLD 150.0  /* Enable full monitoring if load > 150% */
#define PERF_LOAD_HYSTERESIS 20.0  /* Disable when load drops below 130% */

/* Global performance monitoring configuration */
extern int perf_monitoring_level;
extern int perf_sample_rate;
extern bool perf_dynamic_monitoring;

/* Function prototypes */
void perf_set_monitoring_level(int level);
void perf_update_dynamic_monitoring(double current_load);

#endif /* PERFMON_CONFIG_H */