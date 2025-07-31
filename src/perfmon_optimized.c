/**
 * @file perfmon_optimized.c
 * @brief Optimized performance monitoring implementation
 *
 * This provides a more efficient performance monitoring system that:
 * - Reduces overhead by sampling instead of logging every pulse
 * - Allows runtime configuration of monitoring levels
 * - Automatically adjusts monitoring based on server load
 */

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "perfmon.h"
#include "perfmon_config.h"

/* Global configuration variables */
int perf_monitoring_level = PERF_DEFAULT_LEVEL;
int perf_sample_rate = PERF_SAMPLE_RATE;
bool perf_dynamic_monitoring = TRUE;

/* Static tracking variables */
static int pulse_sample_counter = 0;
static bool high_load_monitoring = FALSE;
static double recent_load_avg = 0.0;
static int load_sample_count = 0;

/**
 * Optimized pulse logging that respects monitoring level
 */
void PERF_log_pulse_optimized(double val)
{
  /* Level 0: No monitoring */
  if (perf_monitoring_level == PERF_LEVEL_OFF)
    return;

  /* Update load average */
  recent_load_avg = (recent_load_avg * load_sample_count + val) / (load_sample_count + 1);
  if (++load_sample_count > 100) {
    load_sample_count = 100; /* Prevent overflow, maintain rolling average */
  }

  /* Dynamic monitoring: switch to full monitoring under high load */
  if (perf_dynamic_monitoring) {
    perf_update_dynamic_monitoring(val);
    if (high_load_monitoring) {
      PERF_log_pulse(val);  /* Full monitoring when under load */
      return;
    }
  }

  /* Level 1: Basic monitoring - only track if over threshold */
  if (perf_monitoring_level == PERF_LEVEL_BASIC) {
    if (val > 100.0) {  /* Only log when over 100% */
      PERF_log_pulse(val);
    }
    return;
  }

  /* Level 2: Sampling - log every Nth pulse */
  if (perf_monitoring_level == PERF_LEVEL_SAMPLING) {
    if (++pulse_sample_counter >= perf_sample_rate) {
      PERF_log_pulse(val);
      pulse_sample_counter = 0;
    }
    return;
  }

  /* Level 3: Full monitoring */
  PERF_log_pulse(val);
}

/**
 * Update dynamic monitoring state based on current load
 */
void perf_update_dynamic_monitoring(double current_load)
{
  if (!perf_dynamic_monitoring)
    return;

  if (!high_load_monitoring && current_load > PERF_LOAD_THRESHOLD) {
    /* Enable high load monitoring */
    high_load_monitoring = TRUE;
    log("PERFMON: High load detected (%.1f%%), enabling full monitoring", current_load);
  } else if (high_load_monitoring && 
             current_load < (PERF_LOAD_THRESHOLD - PERF_LOAD_HYSTERESIS)) {
    /* Disable high load monitoring (with hysteresis to prevent flapping) */
    high_load_monitoring = FALSE;
    log("PERFMON: Load normalized (%.1f%%), returning to sampling mode", current_load);
  }
}

/**
 * Set the performance monitoring level
 */
void perf_set_monitoring_level(int level)
{
  if (level < PERF_LEVEL_OFF || level > PERF_LEVEL_FULL) {
    log("SYSERR: Invalid performance monitoring level %d", level);
    return;
  }

  perf_monitoring_level = level;
  pulse_sample_counter = 0;  /* Reset sampling counter */

  const char *level_names[] = {"OFF", "BASIC", "SAMPLING", "FULL"};
  log("PERFMON: Monitoring level set to %s", level_names[level]);
}

/**
 * Command to configure performance monitoring levels at runtime
 */
ACMD(do_perfconfig)
{
  char arg[MAX_INPUT_LENGTH];
  int level;

  one_argument(argument, arg, sizeof(arg));

  if (!*arg) {
    send_to_char(ch, "Performance Monitoring Status:\r\n");
    send_to_char(ch, "  Current Level: %d\r\n", perf_monitoring_level);
    send_to_char(ch, "  Sample Rate: 1/%d pulses\r\n", perf_sample_rate);
    send_to_char(ch, "  Dynamic Monitoring: %s\r\n", perf_dynamic_monitoring ? "ON" : "OFF");
    send_to_char(ch, "  Recent Load Average: %.1f%%\r\n", recent_load_avg);
    send_to_char(ch, "  High Load Mode: %s\r\n", high_load_monitoring ? "ACTIVE" : "inactive");
    send_to_char(ch, "\r\nUsage: perfconfig <off|basic|sampling|full|dynamic>\r\n");
    return;
  }

  if (!str_cmp(arg, "off")) {
    perf_set_monitoring_level(PERF_LEVEL_OFF);
  } else if (!str_cmp(arg, "basic")) {
    perf_set_monitoring_level(PERF_LEVEL_BASIC);
  } else if (!str_cmp(arg, "sampling")) {
    perf_set_monitoring_level(PERF_LEVEL_SAMPLING);
  } else if (!str_cmp(arg, "full")) {
    perf_set_monitoring_level(PERF_LEVEL_FULL);
  } else if (!str_cmp(arg, "dynamic")) {
    perf_dynamic_monitoring = !perf_dynamic_monitoring;
    send_to_char(ch, "Dynamic monitoring %s.\r\n", 
                 perf_dynamic_monitoring ? "enabled" : "disabled");
    return;
  } else if ((level = atoi(arg)) >= 0) {
    perf_sample_rate = MAX(1, level);
    send_to_char(ch, "Sample rate set to 1/%d pulses.\r\n", perf_sample_rate);
    return;
  } else {
    send_to_char(ch, "Invalid option. Use: off, basic, sampling, full, or dynamic\r\n");
    return;
  }

  send_to_char(ch, "Performance monitoring level changed.\r\n");
}