/**
 * @file spec/spec_cooldown.c
 * Explicit legacy object special-procedure cooldown operations.
 */

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "utils.h"
#include "spec/spec_cooldown.h"
#include "point_update_periodic.h"

struct spec_object_cooldown_state spec_object_cooldown_read(const struct obj_data *obj, int slot)
{
  struct spec_object_cooldown_state state = {SPEC_OBJECT_COOLDOWN_INVALID, 0};
  int remaining;

  if (obj == NULL || slot < 0 || slot >= SPEC_TIMER_MAX)
    return state;

  remaining = GET_OBJ_SPECTIMER(obj, slot);
  if (remaining > 0)
  {
    state.status = SPEC_OBJECT_COOLDOWN_ACTIVE;
    state.remaining_mud_hours = remaining;
  }
  else
  {
    state.status = SPEC_OBJECT_COOLDOWN_READY;
  }

  return state;
}

bool spec_object_cooldown_commit(struct obj_data *obj, int slot, int duration_mud_hours)
{
  if (obj == NULL || slot < 0 || slot >= SPEC_TIMER_MAX || duration_mud_hours <= 0)
    return false;

  point_update_object_spec_timer_set(obj, slot, duration_mud_hours);
  return true;
}
