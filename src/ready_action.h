#ifndef READY_ACTION_H
#define READY_ACTION_H

#include <stdbool.h>

struct char_data;

bool ready_action_runtime_init(void);
void ready_action_runtime_shutdown(void);
void ready_action_cancel(struct char_data *ch, bool notify);

#endif /* READY_ACTION_H */
