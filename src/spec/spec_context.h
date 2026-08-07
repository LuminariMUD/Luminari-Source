/**
 * @file spec/spec_context.h
 * Narrow validation contracts for special-procedure event and owner context.
 *
 * All rights reserved. See license for complete information.
 */

#ifndef LUMINARI_SPEC_CONTEXT_H
#define LUMINARI_SPEC_CONTEXT_H

#include <stdbool.h>

struct char_data;
struct obj_data;
struct spec_event_context;

enum spec_context_result
{
  SPEC_CONTEXT_VALID = 0,
  SPEC_CONTEXT_MISSING_CONTEXT,
  SPEC_CONTEXT_INVALID_OWNER_TYPE,
  SPEC_CONTEXT_INVALID_EVENT,
  SPEC_CONTEXT_OWNER_EVENT_MISMATCH,
  SPEC_CONTEXT_MISSING_OWNER,
  SPEC_CONTEXT_MISSING_ACTOR,
  SPEC_CONTEXT_MISSING_TARGET,
  SPEC_CONTEXT_MISSING_ARGUMENT,
  SPEC_CONTEXT_INVALID_MOVING_ROOM,
  SPEC_CONTEXT_ACTOR_UNAVAILABLE,
  SPEC_CONTEXT_TARGET_UNAVAILABLE,
  SPEC_CONTEXT_INVALID_ACTOR_ROOM,
  SPEC_CONTEXT_INVALID_TARGET_ROOM,
  SPEC_CONTEXT_DIFFERENT_ROOMS,
  SPEC_CONTEXT_OBJECT_NOT_WORN,
  SPEC_CONTEXT_OBJECT_SLOT_MISMATCH,
  SPEC_CONTEXT_NOT_CURRENT_TARGET
};

const char *spec_context_result_name(enum spec_context_result result);

/**
 * Validate the owner, event, and required payload shape of one gateway context.
 * Context pointers are borrowed for this check and are never retained.
 */
enum spec_context_result spec_context_validate_event(const struct spec_event_context *context);

/**
 * Require the invoking object instance itself to occupy one of the actor's wear slots.
 * The actor must be alive, not pending extraction, and in a valid room.
 */
enum spec_context_result spec_context_validate_worn_object(const struct char_data *actor,
                                                           const struct obj_data *obj);

/**
 * Validate two live, colocated combat participants and optionally the actor's current target.
 * DEAD() flags are treated as pending extraction; no pointer may be retained after a later effect.
 */
enum spec_context_result spec_context_validate_combat_target(const struct char_data *actor,
                                                             const struct char_data *target,
                                                             bool require_current_target);

#endif /* LUMINARI_SPEC_CONTEXT_H */
