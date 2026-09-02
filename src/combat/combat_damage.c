#include "combat/combat_damage.h"

#include "domain_event_world.h"

/* Seed a damage result with its participants and requested amount.
 * The status stays REJECTED until a caller sets the real outcome. */
static struct combat_damage_result
combat_damage_result_base(struct char_data *source, struct char_data *target, int requested)
{
  struct combat_damage_result result = {0};

  result.source = domain_event_character_handle(source);
  result.target = domain_event_character_handle(target);
  result.requested = requested;
  return result;
}

/* Translate a legacy damage() return value into a typed result.
 * Negative means the target died, zero means no effect, positive is the amount
 * actually applied; missing participants are reported as rejected. */
struct combat_damage_result combat_damage_result_from_legacy(struct char_data *source,
                                                             struct char_data *target,
                                                             int requested, int legacy_result)
{
  struct combat_damage_result result = combat_damage_result_base(source, target, requested);

  result.legacy_result = legacy_result;
  if (source == NULL || target == NULL || requested < 0)
    result.status = COMBAT_DAMAGE_REJECTED;
  else if (legacy_result < 0)
    result.status = COMBAT_DAMAGE_TARGET_DIED;
  else if (legacy_result == 0)
    result.status = COMBAT_DAMAGE_NO_EFFECT;
  else
  {
    result.status = COMBAT_DAMAGE_APPLIED;
    result.applied = legacy_result;
  }
  return result;
}

/* Build the result for damage deferred onto the active reaction queue.
 * The legacy result stays zero because nothing has been applied yet. */
struct combat_damage_result combat_damage_result_queued(struct char_data *source,
                                                        struct char_data *target, int requested)
{
  struct combat_damage_result result = combat_damage_result_base(source, target, requested);

  result.status = COMBAT_DAMAGE_QUEUED;
  return result;
}

/* Build the result for damage that could not be applied or scheduled. */
struct combat_damage_result combat_damage_result_rejected(struct char_data *source,
                                                          struct char_data *target, int requested)
{
  struct combat_damage_result result = combat_damage_result_base(source, target, requested);

  result.status = COMBAT_DAMAGE_REJECTED;
  return result;
}
