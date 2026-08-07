/**
 * @file spec/spec_effects.c
 * Source-owned, stacking-group-aware temporary affect operations.
 */

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "utils.h"
#include "handler.h"
#include "spec/spec_effects.h"

#include <limits.h>

#define SPEC_EFFECT_SOURCE_NAMESPACE_WIDTH 1000000L

static bool spec_effect_modifier_is_valid(const struct spec_effect_modifier *modifier)
{
  return modifier != NULL && modifier->location >= APPLY_NONE && modifier->location < NUM_APPLIES &&
         modifier->modifier >= SHRT_MIN && modifier->modifier <= SHRT_MAX &&
         modifier->bonus_type >= 0 && modifier->bonus_type < NUM_BONUS_TYPES &&
         modifier->duration > 0 && modifier->duration <= SHRT_MAX &&
         modifier->aff_flag >= SPEC_EFFECT_NO_AFF_FLAG && modifier->aff_flag < NUM_AFF_FLAGS;
}

bool spec_effect_source_id(enum spec_effect_source_namespace source_namespace, long owner_key,
                           long *source_id)
{
  long encoded;

  if (source_id == NULL || source_namespace <= SPEC_EFFECT_SOURCE_INVALID ||
      source_namespace > SPEC_EFFECT_SOURCE_LEGACY_PROCEDURE || owner_key <= 0 ||
      owner_key >= SPEC_EFFECT_SOURCE_NAMESPACE_WIDTH)
    return false;

  if ((long)source_namespace > (LONG_MAX - owner_key) / SPEC_EFFECT_SOURCE_NAMESPACE_WIDTH)
    return false;

  encoded = ((long)source_namespace * SPEC_EFFECT_SOURCE_NAMESPACE_WIDTH) + owner_key;
  *source_id = -encoded;
  return true;
}

bool spec_effect_stack_active(const struct char_data *ch, int spell, int stacking_group)
{
  const struct affected_type *af;

  if (ch == NULL || spell <= 0 || spell > SHRT_MAX || stacking_group <= 0 ||
      stacking_group > SHRT_MAX)
    return false;

  for (af = ch->affected; af != NULL; af = af->next)
    if (af->spell == spell && af->specific == (sh_int)stacking_group)
      return true;

  return false;
}

enum spec_effect_result spec_effect_apply_group(struct char_data *ch, int spell, long source_id,
                                                int stacking_group,
                                                const struct spec_effect_modifier *modifiers,
                                                size_t modifier_count)
{
  struct affected_type af;
  size_t index;

  if (ch == NULL || spell <= 0 || spell > SHRT_MAX || source_id >= 0 || stacking_group <= 0 ||
      stacking_group > SHRT_MAX || modifiers == NULL || modifier_count == 0 ||
      modifier_count > SPEC_EFFECT_MAX_MODIFIERS)
    return SPEC_EFFECT_INVALID;

  for (index = 0; index < modifier_count; index++)
    if (!spec_effect_modifier_is_valid(&modifiers[index]))
      return SPEC_EFFECT_INVALID;

  if (spec_effect_stack_active(ch, spell, stacking_group))
    return SPEC_EFFECT_STACKING_CONFLICT;

  affect_batch_begin(ch);
  for (index = 0; index < modifier_count; index++)
  {
    new_affect(&af);
    af.spell = (sh_int)spell;
    af.duration = (sh_int)modifiers[index].duration;
    af.location = modifiers[index].location;
    af.modifier = (sh_int)modifiers[index].modifier;
    af.bonus_type = modifiers[index].bonus_type;
    af.specific = (sh_int)stacking_group;
    if (modifiers[index].aff_flag != SPEC_EFFECT_NO_AFF_FLAG)
      SET_BIT_AR(af.bitvector, modifiers[index].aff_flag);
    affect_to_char_source(ch, &af, source_id);
  }
  affect_batch_end(ch);

  return SPEC_EFFECT_APPLIED;
}
