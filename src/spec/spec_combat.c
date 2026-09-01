/**
 * @file spec/spec_combat.c
 * Safe combat-target and legacy damage-result contracts for procedures.
 */

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "combat/combat_damage.h"
#include "combat/fight.h"
#include "spec/spec_combat.h"

struct spec_damage_result spec_damage_current_target(struct char_data *actor,
                                                     struct char_data *target, int amount,
                                                     int attack_type, int damage_type,
                                                     int dualwield)
{
  struct combat_damage_result damage_result;
  struct spec_damage_result result = {SPEC_DAMAGE_INVALID_CONTEXT, SPEC_CONTEXT_MISSING_CONTEXT, 0};

  result.context_result = spec_context_validate_combat_target(actor, target, true);
  if (result.context_result != SPEC_CONTEXT_VALID)
    return result;
  if (amount < 0)
  {
    result.status = SPEC_DAMAGE_INVALID_AMOUNT;
    return result;
  }

  damage_result = combat_damage_apply(actor, target, amount, attack_type, damage_type, dualwield);
  result.legacy_result = damage_result.legacy_result;
  if (damage_result.status == COMBAT_DAMAGE_TARGET_DIED)
    result.status = SPEC_DAMAGE_TARGET_INVALIDATED;
  else if (damage_result.status == COMBAT_DAMAGE_NO_EFFECT ||
           damage_result.status == COMBAT_DAMAGE_QUEUED)
    result.status = SPEC_DAMAGE_NO_EFFECT;
  else if (damage_result.status == COMBAT_DAMAGE_APPLIED)
    result.status = SPEC_DAMAGE_APPLIED;
  else
    result.status = SPEC_DAMAGE_INVALID_CONTEXT;

  return result;
}
