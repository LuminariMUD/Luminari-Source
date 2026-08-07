/**
 * @file spec/spec_combat.h
 * Safe combat-target and legacy damage-result contracts for procedures.
 *
 * All rights reserved. See license for complete information.
 */

#ifndef LUMINARI_SPEC_COMBAT_H
#define LUMINARI_SPEC_COMBAT_H

#include "spec/spec_context.h"

struct char_data;

enum spec_damage_status
{
  SPEC_DAMAGE_INVALID_CONTEXT = 0,
  SPEC_DAMAGE_INVALID_AMOUNT,
  SPEC_DAMAGE_NO_EFFECT,
  SPEC_DAMAGE_APPLIED,
  SPEC_DAMAGE_TARGET_INVALIDATED
};

struct spec_damage_result
{
  enum spec_damage_status status;
  enum spec_context_result context_result;
  int legacy_result;
};

/**
 * Apply damage to the actor's live, colocated current opponent.
 *
 * The raw damage() return is retained: zero is no effect, positive is applied
 * damage, and negative means the target may have died and entered extraction.
 * Callers must stop touching the target after TARGET_INVALIDATED.
 */
struct spec_damage_result spec_damage_current_target(struct char_data *actor,
                                                     struct char_data *target, int amount,
                                                     int attack_type, int damage_type,
                                                     int dualwield);

#endif /* LUMINARI_SPEC_COMBAT_H */
