/**
 * @file spec/spec_cooldown.h
 * Explicit legacy object special-procedure cooldown operations.
 *
 * All rights reserved. See license for complete information.
 */

#ifndef LUMINARI_SPEC_COOLDOWN_H
#define LUMINARI_SPEC_COOLDOWN_H

#include <stdbool.h>

struct obj_data;

enum spec_object_cooldown_status
{
  SPEC_OBJECT_COOLDOWN_INVALID = 0,
  SPEC_OBJECT_COOLDOWN_READY,
  SPEC_OBJECT_COOLDOWN_ACTIVE
};

struct spec_object_cooldown_state
{
  enum spec_object_cooldown_status status;
  int remaining_mud_hours;
};

/**
 * Read one object-instance spec_timer slot.
 *
 * The point-update owner service advances this once per MUD hour. Storage
 * belongs to the object instance in a slot from [0, SPEC_TIMER_MAX), is
 * measured in MUD hours, is not serialized by objsave, and resets when the
 * instance is recreated or the server restarts. Values at or below zero are
 * ready.
 */
struct spec_object_cooldown_state spec_object_cooldown_read(const struct obj_data *obj, int slot);

/** Commit a positive MUD-hour duration only after validation and effect success. */
bool spec_object_cooldown_commit(struct obj_data *obj, int slot, int duration_mud_hours);

#endif /* LUMINARI_SPEC_COOLDOWN_H */
