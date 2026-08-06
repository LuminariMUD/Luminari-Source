/**
 * @file spec_binding.h
 * Owned authored special-procedure binding metadata.
 */

#ifndef LUMINARI_SPEC_BINDING_H
#define LUMINARI_SPEC_BINDING_H

#include "spec/spec_registry.h"

#include <stdbool.h>
#include <stddef.h>

enum spec_binding_resolution
{
  SPEC_BINDING_RESOLVED = 1,
  SPEC_BINDING_UNKNOWN_NAME,
  SPEC_BINDING_INCOMPATIBLE_OWNER,
  SPEC_BINDING_INCOMPATIBLE_SOURCE
};

/** Authored identity and provenance for one prototype binding. */
struct spec_binding
{
  spec_owner_mask owner;
  unsigned int prototype_vnum;
  char *requested_name;
  const struct spec_definition *definition;
  spec_binding_source_mask source;
  char *source_location;
  enum spec_binding_resolution resolution;
};

/**
 * Resolve and transactionally replace an authored binding.
 *
 * Unknown and incompatible requests are valid owned records. Invalid arguments or allocation
 * failure return false and leave the target unchanged.
 */
bool spec_binding_replace(struct spec_binding **target, spec_owner_mask owner,
                          unsigned int prototype_vnum, const char *requested_name,
                          spec_binding_source_mask source, const char *source_location, char *error,
                          size_t error_size);

/** Deep-copy a binding, or clear the target when source is NULL. */
bool spec_binding_copy(struct spec_binding **target, const struct spec_binding *source, char *error,
                       size_t error_size);

/** Free a binding and null the caller-owned pointer. */
void spec_binding_free(struct spec_binding **binding);

/** Return the authored compatibility handler only for a fully resolved binding. */
spec_legacy_handler spec_binding_legacy_handler(const struct spec_binding *binding);

/** Return a stable diagnostic label for one exact binding source bit. */
const char *spec_binding_source_name(spec_binding_source_mask source);

/** Return a stable diagnostic label for a resolution state. */
const char *spec_binding_resolution_name(enum spec_binding_resolution resolution);

/**
 * Format an unresolved or incompatible content diagnostic.
 *
 * Returns false for a null/resolved binding or an unusable output buffer.
 */
bool spec_binding_format_diagnostic(const struct spec_binding *binding, char *buffer,
                                    size_t buffer_size);

#endif /* LUMINARI_SPEC_BINDING_H */
