/**
 * @file spec/spec_context.c
 * Narrow validation contracts for special-procedure event and owner context.
 */

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "utils.h"
#include "db.h"
#include "spec/spec_context.h"
#include "spec/spec_dispatch.h"

static bool spec_context_owner_type_is_single(spec_owner_mask owner_type)
{
  return owner_type == SPEC_OWNER_MOBILE || owner_type == SPEC_OWNER_OBJECT ||
         owner_type == SPEC_OWNER_ROOM;
}

static bool spec_context_event_is_single(spec_event_mask event)
{
  return event != SPEC_EVENT_NONE && (event & SPEC_EVENT_ALL) == event &&
         (event & (event - 1U)) == 0;
}

static bool spec_context_owner_supports_event(spec_owner_mask owner_type, spec_event_mask event)
{
  switch (event)
  {
  case SPEC_EVENT_COMMAND:
    return true;
  case SPEC_EVENT_MOBILE_ACTIVITY:
  case SPEC_EVENT_MOBILE_COMBAT_TURN:
  case SPEC_EVENT_MOUNT_CHARGE:
    return owner_type == SPEC_OWNER_MOBILE;
  case SPEC_EVENT_OBJECT_AUTO_PULSE:
  case SPEC_EVENT_ITEM_IDENTIFY:
  case SPEC_EVENT_WEAPON_HIT:
  case SPEC_EVENT_DEFENSE_REACTION:
  case SPEC_EVENT_COMBAT_MANEUVER:
    return owner_type == SPEC_OWNER_OBJECT;
  case SPEC_EVENT_MOVING_ROOM_RELOCATION:
    return owner_type == SPEC_OWNER_ROOM;
  default:
    return false;
  }
}

static bool spec_context_event_requires_actor(spec_event_mask event)
{
  return event != SPEC_EVENT_OBJECT_AUTO_PULSE && event != SPEC_EVENT_MOVING_ROOM_RELOCATION;
}

static bool spec_context_event_requires_target(spec_event_mask event)
{
  return event == SPEC_EVENT_WEAPON_HIT || event == SPEC_EVENT_DEFENSE_REACTION ||
         event == SPEC_EVENT_COMBAT_MANEUVER || event == SPEC_EVENT_MOUNT_CHARGE;
}

static bool spec_context_room_is_valid(room_rnum room)
{
  return world != NULL && room != NOWHERE && room <= top_of_world;
}

static bool spec_context_character_is_unavailable(const struct char_data *ch)
{
  return ch == NULL || GET_POS(ch) <= POS_DEAD || DEAD(ch);
}

const char *spec_context_result_name(enum spec_context_result result)
{
  switch (result)
  {
  case SPEC_CONTEXT_VALID:
    return "valid";
  case SPEC_CONTEXT_MISSING_CONTEXT:
    return "missing context";
  case SPEC_CONTEXT_INVALID_OWNER_TYPE:
    return "invalid owner type";
  case SPEC_CONTEXT_INVALID_EVENT:
    return "invalid event";
  case SPEC_CONTEXT_OWNER_EVENT_MISMATCH:
    return "owner/event mismatch";
  case SPEC_CONTEXT_MISSING_OWNER:
    return "missing owner";
  case SPEC_CONTEXT_MISSING_ACTOR:
    return "missing actor";
  case SPEC_CONTEXT_MISSING_TARGET:
    return "missing target";
  case SPEC_CONTEXT_MISSING_ARGUMENT:
    return "missing argument";
  case SPEC_CONTEXT_INVALID_MOVING_ROOM:
    return "invalid moving-room payload";
  case SPEC_CONTEXT_ACTOR_UNAVAILABLE:
    return "actor unavailable";
  case SPEC_CONTEXT_TARGET_UNAVAILABLE:
    return "target unavailable";
  case SPEC_CONTEXT_INVALID_ACTOR_ROOM:
    return "invalid actor room";
  case SPEC_CONTEXT_INVALID_TARGET_ROOM:
    return "invalid target room";
  case SPEC_CONTEXT_DIFFERENT_ROOMS:
    return "actor and target are in different rooms";
  case SPEC_CONTEXT_OBJECT_NOT_WORN:
    return "object instance is not worn by actor";
  case SPEC_CONTEXT_OBJECT_SLOT_MISMATCH:
    return "object wear slot does not point to the instance";
  case SPEC_CONTEXT_NOT_CURRENT_TARGET:
    return "target is not the actor's current opponent";
  default:
    return "unknown context result";
  }
}

enum spec_context_result spec_context_validate_event(const struct spec_event_context *context)
{
  if (context == NULL)
    return SPEC_CONTEXT_MISSING_CONTEXT;
  if (!spec_context_owner_type_is_single(context->owner_type))
    return SPEC_CONTEXT_INVALID_OWNER_TYPE;
  if (!spec_context_event_is_single(context->event))
    return SPEC_CONTEXT_INVALID_EVENT;
  if (!spec_context_owner_supports_event(context->owner_type, context->event))
    return SPEC_CONTEXT_OWNER_EVENT_MISMATCH;
  if (context->owner == NULL)
    return SPEC_CONTEXT_MISSING_OWNER;
  if (spec_context_event_requires_actor(context->event) && context->actor == NULL)
    return SPEC_CONTEXT_MISSING_ACTOR;
  if (spec_context_event_requires_target(context->event) && context->target == NULL)
    return SPEC_CONTEXT_MISSING_TARGET;
  if (context->event != SPEC_EVENT_MOVING_ROOM_RELOCATION && context->argument == NULL)
    return SPEC_CONTEXT_MISSING_ARGUMENT;
  if (context->event == SPEC_EVENT_MOVING_ROOM_RELOCATION &&
      (context->moving_room == NULL || context->owner != context->moving_room))
    return SPEC_CONTEXT_INVALID_MOVING_ROOM;

  return SPEC_CONTEXT_VALID;
}

enum spec_context_result spec_context_validate_worn_object(const struct char_data *actor,
                                                           const struct obj_data *obj)
{
  if (actor == NULL)
    return SPEC_CONTEXT_MISSING_ACTOR;
  if (obj == NULL)
    return SPEC_CONTEXT_MISSING_OWNER;
  if (spec_context_character_is_unavailable(actor))
    return SPEC_CONTEXT_ACTOR_UNAVAILABLE;
  if (!spec_context_room_is_valid(IN_ROOM(actor)))
    return SPEC_CONTEXT_INVALID_ACTOR_ROOM;
  if (obj->worn_by != actor)
    return SPEC_CONTEXT_OBJECT_NOT_WORN;
  if (obj->worn_on < 0 || obj->worn_on >= NUM_WEARS || GET_EQ(actor, obj->worn_on) != obj)
    return SPEC_CONTEXT_OBJECT_SLOT_MISMATCH;

  return SPEC_CONTEXT_VALID;
}

enum spec_context_result spec_context_validate_combat_target(const struct char_data *actor,
                                                             const struct char_data *target,
                                                             bool require_current_target)
{
  if (actor == NULL)
    return SPEC_CONTEXT_MISSING_ACTOR;
  if (target == NULL)
    return SPEC_CONTEXT_MISSING_TARGET;
  if (spec_context_character_is_unavailable(actor))
    return SPEC_CONTEXT_ACTOR_UNAVAILABLE;
  if (spec_context_character_is_unavailable(target))
    return SPEC_CONTEXT_TARGET_UNAVAILABLE;
  if (!spec_context_room_is_valid(IN_ROOM(actor)))
    return SPEC_CONTEXT_INVALID_ACTOR_ROOM;
  if (!spec_context_room_is_valid(IN_ROOM(target)))
    return SPEC_CONTEXT_INVALID_TARGET_ROOM;
  if (IN_ROOM(actor) != IN_ROOM(target))
    return SPEC_CONTEXT_DIFFERENT_ROOMS;
  if (require_current_target && FIGHTING(actor) != target)
    return SPEC_CONTEXT_NOT_CURRENT_TARGET;

  return SPEC_CONTEXT_VALID;
}
