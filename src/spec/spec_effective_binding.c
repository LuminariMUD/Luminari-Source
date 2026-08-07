/**
 * @file spec_effective_binding.c
 * Owned effective special-procedure contribution metadata.
 */

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "utils.h"
#include "spec/spec_binding.h"
#include "spec/spec_effective_binding.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SPEC_EFFECTIVE_TEXT_LIMIT (READ_SIZE - 1U)
#define SPEC_EFFECTIVE_ESCAPED_LIMIT ((SPEC_EFFECTIVE_TEXT_LIMIT * 2U) + 1U)

static void spec_effective_set_error(char *error, size_t error_size, const char *format, ...)
{
  va_list arguments;

  if (error == NULL || error_size == 0)
    return;

  va_start(arguments, format);
  /* NOLINTNEXTLINE(clang-analyzer-valist.Uninitialized) -- va_start initializes arguments. */
  vsnprintf(error, error_size, format, arguments);
  va_end(arguments);
}

static bool spec_effective_mask_is_one_bit(uint32_t mask)
{
  return mask != 0 && (mask & (mask - 1U)) == 0;
}

static bool spec_effective_text_is_valid(const char *text)
{
  const unsigned char *cursor;
  size_t length;

  if (text == NULL || *text == '\0')
    return false;

  length = 0;
  for (cursor = (const unsigned char *)text; *cursor != '\0'; cursor++)
  {
    if (++length > SPEC_EFFECTIVE_TEXT_LIMIT || *cursor < 32U || *cursor == 127U)
      return false;
  }

  return true;
}

static char *spec_effective_duplicate(const char *text)
{
  char *copy;
  size_t length;

  if (text == NULL)
    return NULL;

  length = strlen(text);
  copy = malloc(length + 1U);
  if (copy == NULL)
    return NULL;

  memcpy(copy, text, length + 1U);
  return copy;
}

static void spec_effective_contribution_free(struct spec_effective_contribution *contribution)
{
  if (contribution == NULL)
    return;

  free(contribution->requested_name);
  free(contribution->handler_name);
  free(contribution->source_location);
  free(contribution->secondary_name);
  free(contribution);
}

static struct spec_effective_contribution *
spec_effective_contribution_allocate(const struct spec_effective_contribution_input *input,
                                     char *error, size_t error_size)
{
  struct spec_effective_contribution *contribution;

  contribution = calloc(1, sizeof(*contribution));
  if (contribution == NULL)
  {
    spec_effective_set_error(error, error_size, "unable to allocate effective contribution");
    return NULL;
  }

  contribution->requested_name = spec_effective_duplicate(input->requested_name);
  contribution->source_location = spec_effective_duplicate(input->source_location);
  if (input->handler_name != NULL)
    contribution->handler_name = spec_effective_duplicate(input->handler_name);
  if (input->secondary_name != NULL)
    contribution->secondary_name = spec_effective_duplicate(input->secondary_name);

  if (contribution->requested_name == NULL || contribution->source_location == NULL ||
      (input->handler_name != NULL && contribution->handler_name == NULL) ||
      (input->secondary_name != NULL && contribution->secondary_name == NULL))
  {
    spec_effective_set_error(error, error_size, "unable to copy effective contribution text");
    spec_effective_contribution_free(contribution);
    return NULL;
  }

  contribution->source = input->source;
  contribution->handler = input->handler;
  contribution->wrapper = input->wrapper;
  contribution->secondary_handler = input->secondary_handler;
  return contribution;
}

static bool spec_effective_input_is_valid(spec_owner_mask owner,
                                          const struct spec_effective_contribution_input *input,
                                          char *error, size_t error_size)
{
  if (!spec_effective_mask_is_one_bit(owner) || (owner & ~SPEC_OWNER_ALL) != 0)
  {
    spec_effective_set_error(error, error_size, "effective binding owner is invalid");
    return false;
  }
  if (input == NULL)
  {
    spec_effective_set_error(error, error_size, "effective contribution input is null");
    return false;
  }
  if (!spec_effective_mask_is_one_bit(input->source) ||
      (input->source & ~SPEC_BINDING_SOURCE_ALL) != 0)
  {
    spec_effective_set_error(error, error_size, "effective contribution source is invalid");
    return false;
  }
  if (!spec_effective_text_is_valid(input->requested_name))
  {
    spec_effective_set_error(
        error, error_size, "effective contribution requested name is not bounded single-line text");
    return false;
  }
  if (!spec_effective_text_is_valid(input->source_location))
  {
    spec_effective_set_error(error, error_size,
                             "effective contribution location is not bounded single-line text");
    return false;
  }
  if (input->handler != NULL && !spec_effective_text_is_valid(input->handler_name))
  {
    spec_effective_set_error(error, error_size,
                             "effective contribution handler name is not bounded single-line text");
    return false;
  }
  if (input->handler == NULL && input->handler_name != NULL)
  {
    spec_effective_set_error(error, error_size, "null effective handler has a handler name");
    return false;
  }
  if (input->secondary_handler != NULL && !spec_effective_text_is_valid(input->secondary_name))
  {
    spec_effective_set_error(error, error_size,
                             "effective secondary name is not bounded single-line text");
    return false;
  }
  if (input->secondary_handler == NULL && input->secondary_name != NULL)
  {
    spec_effective_set_error(error, error_size, "null effective secondary has a secondary name");
    return false;
  }
  if (input->wrapper && input->source != SPEC_BINDING_SOURCE_SHOP &&
      input->source != SPEC_BINDING_SOURCE_QUEST)
  {
    spec_effective_set_error(error, error_size, "effective wrapper source is invalid");
    return false;
  }
  if (!input->wrapper && input->secondary_handler != NULL)
  {
    spec_effective_set_error(error, error_size, "non-wrapper contribution has a secondary");
    return false;
  }
  if ((input->source == SPEC_BINDING_SOURCE_SHOP || input->source == SPEC_BINDING_SOURCE_QUEST) !=
      input->wrapper)
  {
    spec_effective_set_error(error, error_size, "effective wrapper and source do not agree");
    return false;
  }
  if (input->wrapper && input->handler == NULL)
  {
    spec_effective_set_error(error, error_size, "effective wrapper handler is null");
    return false;
  }
  if ((input->source == SPEC_BINDING_SOURCE_SHOP || input->source == SPEC_BINDING_SOURCE_QUEST) &&
      owner != SPEC_OWNER_MOBILE)
  {
    spec_effective_set_error(error, error_size, "wrapper contribution owner is not mobile");
    return false;
  }
  if (input->source == SPEC_BINDING_SOURCE_PARSER_HOOK && owner != SPEC_OWNER_ROOM)
  {
    spec_effective_set_error(error, error_size, "parser-hook contribution owner is not room");
    return false;
  }

  return true;
}

bool spec_effective_binding_contribute(struct spec_effective_binding **target,
                                       spec_owner_mask owner, unsigned int prototype_vnum,
                                       const struct spec_effective_contribution_input *input,
                                       char *error, size_t error_size)
{
  struct spec_effective_binding *binding;
  struct spec_effective_contribution *contribution;
  bool new_binding;

  if (error != NULL && error_size > 0)
    error[0] = '\0';
  if (target == NULL)
  {
    spec_effective_set_error(error, error_size, "effective binding target is null");
    return false;
  }
  if (!spec_effective_input_is_valid(owner, input, error, error_size))
    return false;
  if (*target != NULL && ((*target)->owner != owner || (*target)->prototype_vnum != prototype_vnum))
  {
    spec_effective_set_error(error, error_size, "effective binding prototype identity changed");
    return false;
  }

  contribution = spec_effective_contribution_allocate(input, error, error_size);
  if (contribution == NULL)
    return false;

  new_binding = (*target == NULL);
  binding = *target;
  if (new_binding)
  {
    binding = calloc(1, sizeof(*binding));
    if (binding == NULL)
    {
      spec_effective_set_error(error, error_size, "unable to allocate effective binding");
      spec_effective_contribution_free(contribution);
      return false;
    }
    binding->owner = owner;
    binding->prototype_vnum = prototype_vnum;
  }

  if (input->wrapper)
    contribution->outcome = SPEC_EFFECTIVE_WRAPPED;
  else if (input->handler == NULL)
    contribution->outcome = SPEC_EFFECTIVE_UNRESOLVED;
  else if (binding->effective_contribution == NULL)
    contribution->outcome = SPEC_EFFECTIVE_SELECTED;
  else if (binding->effective_handler == input->handler)
    contribution->outcome = SPEC_EFFECTIVE_REASSERTED;
  else
    contribution->outcome = SPEC_EFFECTIVE_OVERRIDDEN;

  if (binding->contribution_count > 0)
    binding->collision_count++;
  if (binding->last == NULL)
    binding->first = contribution;
  else
    binding->last->next = contribution;
  binding->last = contribution;
  binding->contribution_count++;
  binding->effective_handler = contribution->handler;
  binding->effective_contribution = contribution->handler != NULL ? contribution : NULL;

  if (new_binding)
    *target = binding;
  return true;
}

bool spec_effective_binding_copy(struct spec_effective_binding **target,
                                 const struct spec_effective_binding *source, char *error,
                                 size_t error_size)
{
  const struct spec_effective_contribution *contribution;
  struct spec_effective_binding *copy;
  struct spec_effective_contribution_input input;

  if (error != NULL && error_size > 0)
    error[0] = '\0';
  if (target == NULL)
  {
    spec_effective_set_error(error, error_size, "effective binding copy target is null");
    return false;
  }
  if (*target == source)
    return true;
  if (source == NULL)
  {
    spec_effective_binding_free(target);
    return true;
  }

  copy = NULL;
  for (contribution = source->first; contribution != NULL; contribution = contribution->next)
  {
    input.source = contribution->source;
    input.requested_name = contribution->requested_name;
    input.handler_name = contribution->handler_name;
    input.source_location = contribution->source_location;
    input.handler = contribution->handler;
    input.wrapper = contribution->wrapper;
    input.secondary_handler = contribution->secondary_handler;
    input.secondary_name = contribution->secondary_name;
    if (!spec_effective_binding_contribute(&copy, source->owner, source->prototype_vnum, &input,
                                           error, error_size))
    {
      spec_effective_binding_free(&copy);
      return false;
    }
  }

  spec_effective_binding_free(target);
  *target = copy;
  return true;
}

void spec_effective_binding_free(struct spec_effective_binding **binding)
{
  struct spec_effective_contribution *contribution;
  struct spec_effective_contribution *next;

  if (binding == NULL || *binding == NULL)
    return;

  for (contribution = (*binding)->first; contribution != NULL; contribution = next)
  {
    next = contribution->next;
    spec_effective_contribution_free(contribution);
  }
  free(*binding);
  *binding = NULL;
}

const char *spec_effective_binding_handler_name(const struct spec_effective_binding *binding,
                                                spec_legacy_handler handler)
{
  const struct spec_effective_contribution *contribution;
  const char *name;

  if (binding == NULL || handler == NULL)
    return NULL;

  name = NULL;
  for (contribution = binding->first; contribution != NULL; contribution = contribution->next)
  {
    if (contribution->handler == handler)
      name = contribution->handler_name;
  }
  return name;
}

const struct spec_effective_contribution *
spec_effective_binding_get(const struct spec_effective_binding *binding, size_t index)
{
  const struct spec_effective_contribution *contribution;
  size_t current;

  if (binding == NULL)
    return NULL;

  current = 0;
  for (contribution = binding->first; contribution != NULL; contribution = contribution->next)
  {
    if (current++ == index)
      return contribution;
  }
  return NULL;
}

const char *spec_effective_outcome_name(enum spec_effective_outcome outcome)
{
  switch (outcome)
  {
  case SPEC_EFFECTIVE_SELECTED:
    return "selected";
  case SPEC_EFFECTIVE_UNRESOLVED:
    return "unresolved";
  case SPEC_EFFECTIVE_OVERRIDDEN:
    return "overridden";
  case SPEC_EFFECTIVE_REASSERTED:
    return "reasserted";
  case SPEC_EFFECTIVE_WRAPPED:
    return "wrapped";
  default:
    return NULL;
  }
}

/* Hyphenated tokens: these appear in the machine-readable SPEC_BIND_* boot log,
 * which is a distinct format from the spaced builder-facing names in
 * spec_binding.c. */
static const char *spec_effective_source_token(spec_binding_source_mask source)
{
  switch (source)
  {
  case SPEC_BINDING_SOURCE_WORLD:
    return "world";
  case SPEC_BINDING_SOURCE_LEGACY_ASSIGNMENT:
    return "legacy-assignment";
  case SPEC_BINDING_SOURCE_PARSER_HOOK:
    return "parser-hook";
  case SPEC_BINDING_SOURCE_SHOP:
    return "shop";
  case SPEC_BINDING_SOURCE_QUEST:
    return "quest";
  default:
    return NULL;
  }
}

static bool spec_effective_escape(const char *text, char *buffer, size_t buffer_size)
{
  const unsigned char *cursor;
  size_t written;

  if (text == NULL || buffer == NULL || buffer_size == 0)
    return false;

  written = 0;
  for (cursor = (const unsigned char *)text; *cursor != '\0'; cursor++)
  {
    if (*cursor == '\\' || *cursor == '\'')
    {
      if (written + 2U >= buffer_size)
        return false;
      buffer[written++] = '\\';
    }
    else if (written + 1U >= buffer_size)
      return false;

    buffer[written++] = (char)*cursor;
  }
  buffer[written] = '\0';
  return true;
}

bool spec_effective_binding_format_contribution(const struct spec_effective_binding *binding,
                                                size_t index, bool no_specials_mode, char *buffer,
                                                size_t buffer_size)
{
  const struct spec_effective_contribution *contribution;
  const char *outcome;
  const char *owner;
  const char *source;
  char requested[SPEC_EFFECTIVE_ESCAPED_LIMIT];
  char handler[SPEC_EFFECTIVE_ESCAPED_LIMIT];
  char location[SPEC_EFFECTIVE_ESCAPED_LIMIT];
  char secondary[SPEC_EFFECTIVE_ESCAPED_LIMIT];
  int written;

  if (buffer != NULL && buffer_size > 0)
    buffer[0] = '\0';
  contribution = spec_effective_binding_get(binding, index);
  if (contribution == NULL || buffer == NULL || buffer_size == 0)
    return false;

  owner = spec_owner_name(binding->owner);
  source = spec_effective_source_token(contribution->source);
  outcome = spec_effective_outcome_name(contribution->outcome);
  if (owner == NULL || source == NULL || outcome == NULL ||
      !spec_effective_escape(contribution->requested_name, requested, sizeof(requested)) ||
      !spec_effective_escape(contribution->source_location, location, sizeof(location)) ||
      !spec_effective_escape(contribution->handler_name != NULL ? contribution->handler_name
                                                                : "none",
                             handler, sizeof(handler)) ||
      !spec_effective_escape(contribution->secondary_name != NULL ? contribution->secondary_name
                                                                  : "none",
                             secondary, sizeof(secondary)))
    return false;

  written = snprintf(buffer, buffer_size,
                     "SPEC_BIND mode=%s owner=%s vnum=%u step=%zu source=%s requested='%s' "
                     "handler='%s' outcome=%s location='%s' secondary='%s'",
                     no_specials_mode ? "no_specials" : "normal", owner, binding->prototype_vnum,
                     index + 1U, source, requested, handler, outcome, location, secondary);
  return written >= 0 && (size_t)written < buffer_size;
}

bool spec_effective_binding_format_final(const struct spec_effective_binding *binding,
                                         bool no_specials_mode, char *buffer, size_t buffer_size)
{
  const struct spec_effective_contribution *contribution;
  const char *authored_name;
  const char *chosen_name;
  const char *chosen_source;
  const char *owner;
  char authored[SPEC_EFFECTIVE_ESCAPED_LIMIT];
  char chosen[SPEC_EFFECTIVE_ESCAPED_LIMIT];
  int written;

  if (buffer != NULL && buffer_size > 0)
    buffer[0] = '\0';
  if (binding == NULL || buffer == NULL || buffer_size == 0)
    return false;

  authored_name = "none";
  for (contribution = binding->first; contribution != NULL; contribution = contribution->next)
  {
    if (contribution->source == SPEC_BINDING_SOURCE_WORLD)
      authored_name = contribution->requested_name;
  }

  chosen_name = "none";
  chosen_source = "none";
  if (binding->effective_contribution != NULL)
  {
    chosen_name = binding->effective_contribution->handler_name;
    chosen_source = spec_effective_source_token(binding->effective_contribution->source);
  }
  owner = spec_owner_name(binding->owner);
  if (owner == NULL || chosen_source == NULL ||
      !spec_effective_escape(authored_name, authored, sizeof(authored)) ||
      !spec_effective_escape(chosen_name, chosen, sizeof(chosen)))
    return false;

  written = snprintf(buffer, buffer_size,
                     "SPEC_BIND_FINAL mode=%s owner=%s vnum=%u authored='%s' contributions=%zu "
                     "collisions=%zu chosen_source=%s chosen='%s'",
                     no_specials_mode ? "no_specials" : "normal", owner, binding->prototype_vnum,
                     authored, binding->contribution_count, binding->collision_count, chosen_source,
                     chosen);
  return written >= 0 && (size_t)written < buffer_size;
}

void spec_effective_binding_log(const struct spec_effective_binding *binding, bool no_specials_mode)
{
  char diagnostic[MAX_STRING_LENGTH];
  size_t index;

  if (binding == NULL)
    return;

  for (index = 0; index < binding->contribution_count; index++)
  {
    if (spec_effective_binding_format_contribution(binding, index, no_specials_mode, diagnostic,
                                                   sizeof(diagnostic)))
      log("%s", diagnostic);
  }
  if (spec_effective_binding_format_final(binding, no_specials_mode, diagnostic,
                                          sizeof(diagnostic)))
    log("%s", diagnostic);
}
