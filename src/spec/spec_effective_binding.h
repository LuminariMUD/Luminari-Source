/**
 * @file spec_effective_binding.h
 * Owned effective special-procedure contribution metadata.
 */

#ifndef LUMINARI_SPEC_EFFECTIVE_BINDING_H
#define LUMINARI_SPEC_EFFECTIVE_BINDING_H

#include "spec/spec_registry.h"

#include <stdbool.h>
#include <stddef.h>

enum spec_effective_outcome
{
  SPEC_EFFECTIVE_SELECTED = 1,
  SPEC_EFFECTIVE_UNRESOLVED,
  SPEC_EFFECTIVE_OVERRIDDEN,
  SPEC_EFFECTIVE_REASSERTED,
  SPEC_EFFECTIVE_WRAPPED
};

struct spec_effective_contribution
{
  spec_binding_source_mask source;
  char *requested_name;
  char *handler_name;
  char *source_location;
  enum spec_effective_outcome outcome;
  spec_legacy_handler handler;
  bool wrapper;
  spec_legacy_handler secondary_handler;
  char *secondary_name;
  struct spec_effective_contribution *next;
};

/** Ordered boot contributions and the final callback selected for one prototype. */
struct spec_effective_binding
{
  spec_owner_mask owner;
  unsigned int prototype_vnum;
  struct spec_effective_contribution *first;
  struct spec_effective_contribution *last;
  const struct spec_effective_contribution *effective_contribution;
  size_t contribution_count;
  size_t collision_count;
  spec_legacy_handler effective_handler;
};

/** Validated input for one callback-slot contribution. */
struct spec_effective_contribution_input
{
  spec_binding_source_mask source;
  const char *requested_name;
  const char *handler_name;
  const char *source_location;
  spec_legacy_handler handler;
  bool wrapper;
  spec_legacy_handler secondary_handler;
  const char *secondary_name;
};

/**
 * Append one contribution, creating the record when needed.
 *
 * Invalid input or allocation failure returns false and leaves an existing record unchanged.
 */
bool spec_effective_binding_contribute(struct spec_effective_binding **target,
                                       spec_owner_mask owner, unsigned int prototype_vnum,
                                       const struct spec_effective_contribution_input *input,
                                       char *error, size_t error_size);

/** Deep-copy an effective record, or clear the target when source is NULL. */
bool spec_effective_binding_copy(struct spec_effective_binding **target,
                                 const struct spec_effective_binding *source, char *error,
                                 size_t error_size);

/** Free an effective record and null the caller-owned pointer. */
void spec_effective_binding_free(struct spec_effective_binding **binding);

/** Return the latest recorded identity for a callback, or NULL when it was not recorded. */
const char *spec_effective_binding_handler_name(const struct spec_effective_binding *binding,
                                                spec_legacy_handler handler);

/** Return a contribution by zero-based index, or NULL when out of range. */
const struct spec_effective_contribution *
spec_effective_binding_get(const struct spec_effective_binding *binding, size_t index);

/** Return a stable diagnostic label for a contribution outcome. */
const char *spec_effective_outcome_name(enum spec_effective_outcome outcome);

/** Format one bounded structured contribution diagnostic. */
bool spec_effective_binding_format_contribution(const struct spec_effective_binding *binding,
                                                size_t index, bool no_specials_mode, char *buffer,
                                                size_t buffer_size);

/** Format one bounded structured final-winner diagnostic. */
bool spec_effective_binding_format_final(const struct spec_effective_binding *binding,
                                         bool no_specials_mode, char *buffer, size_t buffer_size);

/** Log every contribution and the final winner for one prototype. */
void spec_effective_binding_log(const struct spec_effective_binding *binding,
                                bool no_specials_mode);

#endif /* LUMINARI_SPEC_EFFECTIVE_BINDING_H */
