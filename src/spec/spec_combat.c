/**
 * @file spec/spec_combat.c
 * Safe combat-target and legacy damage-result contracts for procedures.
 */

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "combat/fight.h"
#include "spec/spec_combat.h"

struct spec_damage_result spec_damage_current_target(struct char_data *actor,
                                                     struct char_data *target, int amount,
                                                     int attack_type, int damage_type,
                                                     int dualwield)
{
  struct spec_damage_result result = {SPEC_DAMAGE_INVALID_CONTEXT, SPEC_CONTEXT_MISSING_CONTEXT, 0};

  result.context_result = spec_context_validate_combat_target(actor, target, true);
  if (result.context_result != SPEC_CONTEXT_VALID)
    return result;
  if (amount < 0)
  {
    result.status = SPEC_DAMAGE_INVALID_AMOUNT;
    return result;
  }

  result.legacy_result = damage(actor, target, amount, attack_type, damage_type, dualwield);
  if (result.legacy_result < 0)
    result.status = SPEC_DAMAGE_TARGET_INVALIDATED;
  else if (result.legacy_result == 0)
    result.status = SPEC_DAMAGE_NO_EFFECT;
  else
    result.status = SPEC_DAMAGE_APPLIED;

  return result;
}
