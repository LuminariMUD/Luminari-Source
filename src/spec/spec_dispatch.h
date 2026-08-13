/**
 * @file spec_dispatch.h
 * Event gateways and incremental typed dispatch for special procedures.
 *
 * Every engine call site builds a typed context here. Registered typed
 * adapters receive it directly; compatibility handlers retain the exact
 * `SPECIAL` translation. Gateways do not decide where a procedure is bound
 * and never mutate prototypes.
 */

#ifndef LUMINARI_SPEC_DISPATCH_H
#define LUMINARI_SPEC_DISPATCH_H

#include <stdbool.h>
#include <stdint.h>

#include "spec/spec_registry.h"

struct char_data;
struct obj_data;
struct room_data;
struct moving_room_data;

/** Gateway-local control flow. Each gateway names what STOP aborts. */
enum spec_flow
{
  SPEC_FLOW_CONTINUE = 0,
  SPEC_FLOW_STOP = 1
};

/** Pointer invalidation reported independently of control flow. */
typedef uint32_t spec_invalidate_mask;
enum spec_invalidate_flag
{
  SPEC_INVALIDATE_NONE = 0,
  SPEC_INVALIDATE_OWNER = (1U << 0),
  SPEC_INVALIDATE_ACTOR = (1U << 1),
  SPEC_INVALIDATE_TARGET = (1U << 2)
};

#define SPEC_INVALIDATE_ALL (SPEC_INVALIDATE_OWNER | SPEC_INVALIDATE_ACTOR | SPEC_INVALIDATE_TARGET)

/**
 * Complete event data captured where it still exists.
 *
 * Context pointers are borrowed for exactly one synchronous invocation. A
 * caller must not retain them past the gateway call and must consult
 * `invalidation` instead of probing potentially freed storage.
 */
struct spec_event_context
{
  /* Identity */
  spec_owner_mask owner_type;
  spec_event_mask event;
  void *owner;             /**< Typed by owner_type; room owners use struct room_data *. */
  struct char_data *actor; /**< Current character, or NULL when the caller supplies none. */

  /* Command payload */
  int command;
  const char *argument; /**< Never NULL except for the moving-room event. */

  /* Combat payloads */
  struct char_data *target;
  int damage;
  int attack_type;
  bool critical;

  /* Moving-room payload */
  struct moving_room_data *moving_room;
  int destination_room; /**< Virtual number selected by the relocation caller. */

  /* Results */
  enum spec_flow flow;
  spec_invalidate_mask invalidation;
  int legacy_return; /**< Raw value returned by a legacy handler. */
};

/** Return a stable diagnostic name for exactly one invalidation bit. */
const char *spec_invalidate_name(spec_invalidate_mask invalidation);

/**
 * Invoke one handler with exact legacy translation and record the result.
 *
 * Returns the raw legacy return value. `context` must already carry the owner
 * type, event, owner, actor, command, and argument for this call site.
 */
int spec_dispatch_legacy(struct spec_event_context *context, spec_legacy_handler handler);

/**
 * Invoke a typed definition after validating its owner and event contract.
 *
 * Flow-bearing events return nonzero when the typed result stops that gateway;
 * notification-only events return the typed handler's raw result while
 * discarding an invalid STOP request.
 */
int spec_dispatch_typed(struct spec_event_context *context,
                        const struct spec_definition *definition);

/**
 * Dispatch a callback-slot handler through typed metadata when registered,
 * otherwise preserve exact legacy invocation.
 */
int spec_dispatch(struct spec_event_context *context, spec_legacy_handler handler);

/* --------------------------------------------------------------------------
 * Command gateway. STOP consumes the command and stops later owner traversal.
 * -------------------------------------------------------------------------- */

int spec_gateway_command_room(struct char_data *ch, struct room_data *room, int cmd,
                              const char *argument);
int spec_gateway_command_object(struct char_data *ch, struct obj_data *obj, int cmd,
                                const char *argument);
int spec_gateway_command_mobile(struct char_data *ch, struct char_data *mob, int cmd,
                                const char *argument);

/* --------------------------------------------------------------------------
 * Pulse gateways.
 * -------------------------------------------------------------------------- */

/** STOP skips the remaining default activity for this mobile. */
int spec_gateway_mobile_activity(struct char_data *mob, spec_legacy_handler handler);
/** Notification only; the legacy return is discarded by the combat caller. */
void spec_gateway_mobile_combat_turn(struct char_data *mob);
/** Notification only; returns pointers invalidated by an on-hit handler. */
spec_invalidate_mask spec_gateway_mobile_hit(struct char_data *mob, struct char_data *target,
                                             int damage, int attack_type, bool critical);
/** Notification only; returns pointers invalidated after an NPC receives a successful hit. */
spec_invalidate_mask spec_gateway_mobile_was_hit(struct char_data *mob, struct char_data *attacker,
                                                 int damage, int attack_type, bool critical);
/** STOP suppresses the ordinary NPC corpse after the handler replaces it. */
int spec_gateway_mobile_death(struct char_data *mob, struct char_data *killer);
/** STOP skips the carried-object fallback invocation. */
void spec_gateway_object_auto_pulse(struct obj_data *obj);
/** Notification-only periodic activity for an explicitly scheduled room binding. */
void spec_gateway_room_activity(struct room_data *room);
/** Notification only; relocation state travels through the owner slot. */
void spec_gateway_moving_room(struct room_data *room, struct moving_room_data *mover,
                              int destination_vnum);

/* --------------------------------------------------------------------------
 * Display and combat gateways. All are notification only.
 * -------------------------------------------------------------------------- */

void spec_gateway_item_identify(struct char_data *ch, struct obj_data *obj);
int spec_gateway_weapon_hit(struct char_data *ch, struct obj_data *weapon, struct char_data *target,
                            int damage, int attack_type, bool critical, const char *hit_token);
void spec_gateway_defense_reaction(struct char_data *defender, struct obj_data *obj,
                                   struct char_data *attacker, const char *reaction_token);
void spec_gateway_combat_maneuver(struct char_data *ch, struct obj_data *shield,
                                  struct char_data *target, const char *maneuver_token);
void spec_gateway_mount_charge(struct char_data *ch, struct char_data *mount,
                               struct char_data *target);

/* --------------------------------------------------------------------------
 * Compatibility composition. Boot wraps an original mobile callback with the
 * shop callback and then the quest callback. Each wrapper forwards the incoming
 * context unchanged to its runtime-only saved secondary; nonzero propagates to
 * the caller. This quest -> shop -> original nesting is not a persisted general
 * procedure chain.
 * -------------------------------------------------------------------------- */

int spec_gateway_shop_secondary(spec_legacy_handler secondary, struct char_data *ch, void *me,
                                int cmd, const char *argument);
int spec_gateway_quest_secondary(spec_legacy_handler secondary, struct char_data *ch, void *me,
                                 int cmd, const char *argument);

#endif /* LUMINARI_SPEC_DISPATCH_H */
