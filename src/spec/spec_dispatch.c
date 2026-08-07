/**
 * @file spec_dispatch.c
 * Phase 01 event gateways for special-procedure invocation.
 *
 * Each gateway builds complete event data at the call site, then performs the
 * exact legacy translation the caller used before the migration. No handler is
 * converted here: legacy procedures still receive the same `ch`, `me`, `cmd`,
 * and argument tokens, and each gateway returns exactly what its caller
 * interpreted previously.
 */

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "utils.h"
#include "db.h"
#include "spec/spec_context.h"
#include "spec/spec_dispatch.h"

#include <string.h>

/* Legacy handlers receive an empty string, never NULL, for internal events. */
static const char spec_empty_argument[] = "";

const char *spec_invalidate_name(spec_invalidate_mask invalidation)
{
  switch (invalidation)
  {
  case SPEC_INVALIDATE_OWNER:
    return "owner";
  case SPEC_INVALIDATE_ACTOR:
    return "actor";
  case SPEC_INVALIDATE_TARGET:
    return "target";
  default:
    return "unknown";
  }
}

/**
 * Report whether this event's caller acts on the handler return value.
 *
 * Notification-only events discard the return today; a gateway must not invent
 * flow for them.
 */
static bool spec_event_uses_flow(spec_event_mask event)
{
  switch (event)
  {
  case SPEC_EVENT_COMMAND:
  case SPEC_EVENT_MOBILE_ACTIVITY:
  case SPEC_EVENT_OBJECT_AUTO_PULSE:
    return TRUE;
  default:
    return FALSE;
  }
}

int spec_dispatch_legacy(struct spec_event_context *context, spec_legacy_handler handler)
{
  enum spec_context_result context_result;
  int result = 0;

  if (context == NULL)
  {
    log("SYSERR: spec_dispatch_legacy called without a context.");
    return 0;
  }

  context->flow = SPEC_FLOW_CONTINUE;
  context->invalidation = SPEC_INVALIDATE_NONE;
  context->legacy_return = 0;

  if (handler == NULL)
    return 0;

  context_result = spec_context_validate_event(context);
  if (context_result != SPEC_CONTEXT_VALID)
  {
    log("SYSERR: spec_dispatch_legacy rejected invalid context: %s.",
        spec_context_result_name(context_result));
    return 0;
  }

  result = (handler)(context->actor, context->owner, context->command, context->argument);

  context->legacy_return = result;
  if (result != 0 && spec_event_uses_flow(context->event))
    context->flow = SPEC_FLOW_STOP;

  return result;
}

/**
 * Initialize the fields every gateway sets, so payload-free events cannot leak
 * stale combat or relocation data into a handler.
 */
static void spec_context_init(struct spec_event_context *context, spec_owner_mask owner_type,
                              spec_event_mask event, void *owner, struct char_data *actor, int cmd,
                              const char *argument)
{
  memset(context, 0, sizeof(*context));
  context->owner_type = owner_type;
  context->event = event;
  context->owner = owner;
  context->actor = actor;
  context->command = cmd;
  context->argument = (argument != NULL) ? argument : spec_empty_argument;
  context->destination_room = NOWHERE;
  context->flow = SPEC_FLOW_CONTINUE;
  context->invalidation = SPEC_INVALIDATE_NONE;
}

/* -------------------------------------------------------------------------- */
/* Command gateway                                                            */
/* -------------------------------------------------------------------------- */

int spec_gateway_command_room(struct char_data *ch, struct room_data *room, int cmd,
                              const char *argument)
{
  struct spec_event_context context;

  if (room == NULL || room->func == NULL)
    return 0;

  spec_context_init(&context, SPEC_OWNER_ROOM, SPEC_EVENT_COMMAND, room, ch, cmd, argument);

  return (spec_dispatch_legacy(&context, room->func) != 0);
}

int spec_gateway_command_object(struct char_data *ch, struct obj_data *obj, int cmd,
                                const char *argument)
{
  struct spec_event_context context;
  spec_legacy_handler handler = NULL;

  if (obj == NULL)
    return 0;

  handler = GET_OBJ_SPEC(obj);
  if (handler == NULL)
    return 0;

  spec_context_init(&context, SPEC_OWNER_OBJECT, SPEC_EVENT_COMMAND, obj, ch, cmd, argument);

  return (spec_dispatch_legacy(&context, handler) != 0);
}

int spec_gateway_command_mobile(struct char_data *ch, struct char_data *mob, int cmd,
                                const char *argument)
{
  struct spec_event_context context;
  spec_legacy_handler handler = NULL;

  if (mob == NULL)
    return 0;

  handler = GET_MOB_SPEC(mob);
  if (handler == NULL)
    return 0;

  spec_context_init(&context, SPEC_OWNER_MOBILE, SPEC_EVENT_COMMAND, mob, ch, cmd, argument);

  return (spec_dispatch_legacy(&context, handler) != 0);
}

/* -------------------------------------------------------------------------- */
/* Pulse gateways                                                             */
/* -------------------------------------------------------------------------- */

int spec_gateway_mobile_activity(struct char_data *mob, spec_legacy_handler handler)
{
  struct spec_event_context context;

  if (mob == NULL || handler == NULL)
    return 0;

  /* The activity caller passes a writable empty buffer, not a literal. */
  spec_context_init(&context, SPEC_OWNER_MOBILE, SPEC_EVENT_MOBILE_ACTIVITY, mob, mob, 0,
                    spec_empty_argument);

  return (spec_dispatch_legacy(&context, handler) != 0);
}

void spec_gateway_mobile_combat_turn(struct char_data *mob)
{
  struct spec_event_context context;
  spec_legacy_handler handler = NULL;

  if (mob == NULL)
    return;

  handler = GET_MOB_SPEC(mob);
  if (handler == NULL)
    return;

  spec_context_init(&context, SPEC_OWNER_MOBILE, SPEC_EVENT_MOBILE_COMBAT_TURN, mob, mob, 0,
                    spec_empty_argument);

  (void)spec_dispatch_legacy(&context, handler);
}

void spec_gateway_object_auto_pulse(struct obj_data *obj)
{
  struct spec_event_context context;
  spec_legacy_handler handler = NULL;

  if (obj == NULL)
    return;

  handler = GET_OBJ_SPEC(obj);
  if (handler == NULL)
    return;

  /* Worn invocation first; a nonzero result skips the carried fallback. */
  spec_context_init(&context, SPEC_OWNER_OBJECT, SPEC_EVENT_OBJECT_AUTO_PULSE, obj, obj->worn_by, 0,
                    spec_empty_argument);
  if (spec_dispatch_legacy(&context, handler) != 0)
    return;

  spec_context_init(&context, SPEC_OWNER_OBJECT, SPEC_EVENT_OBJECT_AUTO_PULSE, obj, obj->carried_by,
                    0, spec_empty_argument);
  (void)spec_dispatch_legacy(&context, handler);
}

void spec_gateway_moving_room(struct room_data *room, struct moving_room_data *mover,
                              int destination_vnum)
{
  struct spec_event_context context;

  if (room == NULL || room->func == NULL)
    return;

  /*
   * The relocation caller selects a room procedure but passes moving-room
   * state through the owner slot, and the legacy argument is NULL rather than
   * an empty string. Both are preserved exactly.
   */
  spec_context_init(&context, SPEC_OWNER_ROOM, SPEC_EVENT_MOVING_ROOM_RELOCATION, mover, NULL, 0,
                    NULL);
  context.argument = NULL;
  context.moving_room = mover;
  context.destination_room = destination_vnum;

  (void)spec_dispatch_legacy(&context, room->func);
}

/* -------------------------------------------------------------------------- */
/* Display and combat gateways                                                */
/* -------------------------------------------------------------------------- */

void spec_gateway_item_identify(struct char_data *ch, struct obj_data *obj)
{
  struct spec_event_context context;
  spec_legacy_handler handler = NULL;

  if (obj == NULL)
    return;

  handler = GET_OBJ_SPEC(obj);
  if (handler == NULL)
    return;

  spec_context_init(&context, SPEC_OWNER_OBJECT, SPEC_EVENT_ITEM_IDENTIFY, obj, ch, 0, "identify");

  (void)spec_dispatch_legacy(&context, handler);
}

int spec_gateway_weapon_hit(struct char_data *ch, struct obj_data *weapon, struct char_data *target,
                            const char *hit_token)
{
  struct spec_event_context context;
  spec_legacy_handler handler = NULL;

  if (weapon == NULL)
    return 0;

  handler = GET_OBJ_SPEC(weapon);
  if (handler == NULL)
    return 0;

  spec_context_init(&context, SPEC_OWNER_OBJECT, SPEC_EVENT_WEAPON_HIT, weapon, ch, 0, hit_token);
  context.target = target;

  return spec_dispatch_legacy(&context, handler);
}

void spec_gateway_defense_reaction(struct char_data *defender, struct obj_data *obj,
                                   struct char_data *attacker, const char *reaction_token)
{
  struct spec_event_context context;
  spec_legacy_handler handler = NULL;

  if (obj == NULL)
    return;

  handler = GET_OBJ_SPEC(obj);
  if (handler == NULL)
    return;

  /* The defender is the actor; the attacker is the target of the reaction. */
  spec_context_init(&context, SPEC_OWNER_OBJECT, SPEC_EVENT_DEFENSE_REACTION, obj, defender, 0,
                    reaction_token);
  context.target = attacker;

  (void)spec_dispatch_legacy(&context, handler);
}

void spec_gateway_combat_maneuver(struct char_data *ch, struct obj_data *shield,
                                  struct char_data *target, const char *maneuver_token)
{
  struct spec_event_context context;
  spec_legacy_handler handler = NULL;

  if (shield == NULL)
    return;

  handler = GET_OBJ_SPEC(shield);
  if (handler == NULL)
    return;

  spec_context_init(&context, SPEC_OWNER_OBJECT, SPEC_EVENT_COMBAT_MANEUVER, shield, ch, 0,
                    maneuver_token);
  context.target = target;

  (void)spec_dispatch_legacy(&context, handler);
}

void spec_gateway_mount_charge(struct char_data *ch, struct char_data *mount,
                               struct char_data *target)
{
  struct spec_event_context context;
  spec_legacy_handler handler = NULL;

  if (mount == NULL)
    return;

  handler = GET_MOB_SPEC(mount);
  if (handler == NULL)
    return;

  spec_context_init(&context, SPEC_OWNER_MOBILE, SPEC_EVENT_MOUNT_CHARGE, mount, ch, 0, "charge");
  context.target = target;

  (void)spec_dispatch_legacy(&context, handler);
}

/* -------------------------------------------------------------------------- */
/* Compatibility composition                                                  */
/* -------------------------------------------------------------------------- */

/**
 * Forward the incoming command context to a saved secondary callback.
 *
 * Shop and quest wrappers are explicit compatibility composition, not a general
 * chain: the secondary sees the caller's own `ch`, `me`, `cmd`, and argument,
 * and a nonzero result propagates so the wrapper reports the command consumed.
 */
static int spec_gateway_secondary(spec_legacy_handler secondary, spec_owner_mask owner_type,
                                  struct char_data *ch, void *me, int cmd, const char *argument)
{
  struct spec_event_context context;

  if (secondary == NULL || me == NULL)
    return 0;

  spec_context_init(&context, owner_type, SPEC_EVENT_COMMAND, me, ch, cmd, argument);

  return (spec_dispatch_legacy(&context, secondary) != 0);
}

int spec_gateway_shop_secondary(spec_legacy_handler secondary, struct char_data *ch, void *me,
                                int cmd, const char *argument)
{
  return spec_gateway_secondary(secondary, SPEC_OWNER_MOBILE, ch, me, cmd, argument);
}

int spec_gateway_quest_secondary(spec_legacy_handler secondary, struct char_data *ch, void *me,
                                 int cmd, const char *argument)
{
  return spec_gateway_secondary(secondary, SPEC_OWNER_MOBILE, ch, me, cmd, argument);
}
