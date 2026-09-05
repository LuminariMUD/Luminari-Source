#ifndef READY_ACTION_H
#define READY_ACTION_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct char_data;

/* Bounded native deadline-lateness samples, in game pulses. */
struct ready_action_latency
{
  size_t samples;
  uint64_t callbacks;
  uint64_t p50, p95, p99, maximum;
};

void ready_action_latency_reset(void);
void ready_action_latency_read(struct ready_action_latency *stats);

bool ready_action_runtime_init(void);
void ready_action_runtime_shutdown(void);
void ready_action_cancel(struct char_data *ch, bool notify);

#endif /* READY_ACTION_H */
