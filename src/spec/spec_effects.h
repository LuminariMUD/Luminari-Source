/**
 * @file spec/spec_effects.h
 * Source-owned, stacking-group-aware temporary affect operations.
 *
 * All rights reserved. See license for complete information.
 */

#ifndef LUMINARI_SPEC_EFFECTS_H
#define LUMINARI_SPEC_EFFECTS_H

#include <stdbool.h>
#include <stddef.h>

struct char_data;

#define SPEC_EFFECT_MAX_MODIFIERS 8
#define SPEC_EFFECT_NO_AFF_FLAG (-1)

enum spec_effect_source_namespace
{
  SPEC_EFFECT_SOURCE_INVALID = 0,
  SPEC_EFFECT_SOURCE_ARTIFACT,
  SPEC_EFFECT_SOURCE_LEGACY_PROCEDURE
};

enum spec_effect_result
{
  SPEC_EFFECT_INVALID = 0,
  SPEC_EFFECT_STACKING_CONFLICT,
  SPEC_EFFECT_APPLIED
};

struct spec_effect_modifier
{
  int location;
  int modifier;
  int bonus_type;
  int duration;
  int aff_flag;
};

/**
 * Build a stable negative source_id from a coordinated namespace and owner key.
 *
 * Keys are stable identities in [1, 999999], never pointers. The result is
 * -(namespace * 1000000 + key), remains stable across restarts, and cannot
 * collide with positive runtime source IDs.
 */
bool spec_effect_source_id(enum spec_effect_source_namespace source_namespace, long owner_key,
                           long *source_id);

/**
 * Report a stacking group in [1, SHRT_MAX], scoped by spell identity.
 * The group is independent of source ownership and is stored in specific.
 */
bool spec_effect_stack_active(const struct char_data *ch, int spell, int stacking_group);

/**
 * Atomically admit one modifier group after checking its shared stacking key.
 *
 * Every inserted affect receives a negative source_id owner and the stacking
 * group in specific. Existing affects with the same spell/group block the
 * complete batch, including affects from a different source. Modifier count
 * is bounded by SPEC_EFFECT_MAX_MODIFIERS; each duration must be positive.
 */
enum spec_effect_result spec_effect_apply_group(struct char_data *ch, int spell, long source_id,
                                                int stacking_group,
                                                const struct spec_effect_modifier *modifiers,
                                                size_t modifier_count);

#endif /* LUMINARI_SPEC_EFFECTS_H */
