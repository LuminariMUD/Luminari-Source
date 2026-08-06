/**
 * @file spec_binding.c
 * Owned authored special-procedure binding metadata.
 */

#include "conf.h"
#include "sysdep.h"

#include "spec/spec_binding.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void spec_binding_set_error(char *error, size_t error_size, const char *format, ...)
{
  va_list arguments;

  if (error == NULL || error_size == 0)
    return;

  va_start(arguments, format);
  /* NOLINTNEXTLINE(clang-analyzer-valist.Uninitialized) -- va_start initializes arguments. */
  vsnprintf(error, error_size, format, arguments);
  va_end(arguments);
}

static bool spec_binding_mask_is_one_bit(uint32_t mask)
{
  return mask != 0 && (mask & (mask - 1U)) == 0;
}

static char *spec_binding_duplicate(const char *text)
{
  char *copy;
  size_t length;

  if (text == NULL)
    return NULL;

  length = strlen(text);
  if (length == SIZE_MAX)
    return NULL;

  copy = malloc(length + 1U);
  if (copy == NULL)
    return NULL;

  memcpy(copy, text, length + 1U);
  return copy;
}

static struct spec_binding *
spec_binding_allocate(spec_owner_mask owner, unsigned int prototype_vnum,
                      const char *requested_name, const struct spec_definition *definition,
                      spec_binding_source_mask source, const char *source_location,
                      enum spec_binding_resolution resolution, char *error, size_t error_size)
{
  struct spec_binding *binding;

  binding = calloc(1, sizeof(*binding));
  if (binding == NULL)
  {
    spec_binding_set_error(error, error_size, "unable to allocate authored binding");
    return NULL;
  }

  binding->requested_name = spec_binding_duplicate(requested_name);
  if (binding->requested_name == NULL)
  {
    spec_binding_set_error(error, error_size, "unable to copy authored binding name");
    free(binding);
    return NULL;
  }

  binding->source_location = spec_binding_duplicate(source_location);
  if (binding->source_location == NULL)
  {
    spec_binding_set_error(error, error_size, "unable to copy authored binding location");
    free(binding->requested_name);
    free(binding);
    return NULL;
  }

  binding->owner = owner;
  binding->prototype_vnum = prototype_vnum;
  binding->definition = definition;
  binding->source = source;
  binding->resolution = resolution;
  return binding;
}

bool spec_binding_replace(struct spec_binding **target, spec_owner_mask owner,
                          unsigned int prototype_vnum, const char *requested_name,
                          spec_binding_source_mask source, const char *source_location, char *error,
                          size_t error_size)
{
  const struct spec_definition *definition;
  struct spec_binding *replacement;
  enum spec_binding_resolution resolution;

  if (error != NULL && error_size > 0)
    error[0] = '\0';

  if (target == NULL)
  {
    spec_binding_set_error(error, error_size, "authored binding target is null");
    return false;
  }
  if (!spec_binding_mask_is_one_bit(owner) || (owner & ~SPEC_OWNER_ALL) != 0)
  {
    spec_binding_set_error(error, error_size, "authored binding owner is invalid");
    return false;
  }
  if (!spec_binding_mask_is_one_bit(source) || (source & ~SPEC_BINDING_SOURCE_ALL) != 0)
  {
    spec_binding_set_error(error, error_size, "authored binding source is invalid");
    return false;
  }
  if (requested_name == NULL || *requested_name == '\0')
  {
    spec_binding_set_error(error, error_size, "authored binding name is empty");
    return false;
  }
  if (source_location == NULL || *source_location == '\0')
  {
    spec_binding_set_error(error, error_size, "authored binding source location is empty");
    return false;
  }

  definition = spec_registry_find_by_name(requested_name);
  if (definition == NULL)
    resolution = SPEC_BINDING_UNKNOWN_NAME;
  else if (!spec_definition_supports_owner(definition, owner))
    resolution = SPEC_BINDING_INCOMPATIBLE_OWNER;
  else if (!spec_definition_allows_binding(definition, source))
    resolution = SPEC_BINDING_INCOMPATIBLE_SOURCE;
  else
    resolution = SPEC_BINDING_RESOLVED;

  replacement = spec_binding_allocate(owner, prototype_vnum, requested_name, definition, source,
                                      source_location, resolution, error, error_size);
  if (replacement == NULL)
    return false;

  spec_binding_free(target);
  *target = replacement;
  return true;
}

bool spec_binding_copy(struct spec_binding **target, const struct spec_binding *source, char *error,
                       size_t error_size)
{
  struct spec_binding *copy;

  if (error != NULL && error_size > 0)
    error[0] = '\0';

  if (target == NULL)
  {
    spec_binding_set_error(error, error_size, "authored binding copy target is null");
    return false;
  }
  if (*target == source)
    return true;
  if (source == NULL)
  {
    spec_binding_free(target);
    return true;
  }

  copy = spec_binding_allocate(source->owner, source->prototype_vnum, source->requested_name,
                               source->definition, source->source, source->source_location,
                               source->resolution, error, error_size);
  if (copy == NULL)
    return false;

  spec_binding_free(target);
  *target = copy;
  return true;
}

void spec_binding_free(struct spec_binding **binding)
{
  if (binding == NULL || *binding == NULL)
    return;

  free((*binding)->requested_name);
  free((*binding)->source_location);
  free(*binding);
  *binding = NULL;
}

spec_legacy_handler spec_binding_legacy_handler(const struct spec_binding *binding)
{
  if (binding == NULL || binding->resolution != SPEC_BINDING_RESOLVED ||
      binding->definition == NULL)
    return NULL;

  return binding->definition->legacy_handler;
}

const char *spec_binding_source_name(spec_binding_source_mask source)
{
  switch (source)
  {
  case SPEC_BINDING_SOURCE_WORLD:
    return "world";
  case SPEC_BINDING_SOURCE_LEGACY_ASSIGNMENT:
    return "legacy assignment";
  case SPEC_BINDING_SOURCE_PARSER_HOOK:
    return "parser hook";
  case SPEC_BINDING_SOURCE_SHOP:
    return "shop";
  case SPEC_BINDING_SOURCE_QUEST:
    return "quest";
  default:
    return NULL;
  }
}

const char *spec_binding_resolution_name(enum spec_binding_resolution resolution)
{
  switch (resolution)
  {
  case SPEC_BINDING_RESOLVED:
    return "resolved";
  case SPEC_BINDING_UNKNOWN_NAME:
    return "unknown special procedure";
  case SPEC_BINDING_INCOMPATIBLE_OWNER:
    return "incompatible owner";
  case SPEC_BINDING_INCOMPATIBLE_SOURCE:
    return "incompatible source";
  default:
    return NULL;
  }
}

bool spec_binding_format_diagnostic(const struct spec_binding *binding, char *buffer,
                                    size_t buffer_size)
{
  const char *owner_name;
  const char *resolution_name;
  const char *source_name;

  if (buffer != NULL && buffer_size > 0)
    buffer[0] = '\0';
  if (binding == NULL || binding->resolution == SPEC_BINDING_RESOLVED || buffer == NULL ||
      buffer_size == 0)
    return false;

  owner_name = spec_owner_name(binding->owner);
  source_name = spec_binding_source_name(binding->source);
  resolution_name = spec_binding_resolution_name(binding->resolution);
  if (owner_name == NULL || source_name == NULL || resolution_name == NULL ||
      binding->requested_name == NULL || binding->source_location == NULL)
    return false;

  snprintf(buffer, buffer_size, "%s binding at %s: %s prototype #%u requested '%s': %s",
           source_name, binding->source_location, owner_name, binding->prototype_vnum,
           binding->requested_name, resolution_name);
  return true;
}
