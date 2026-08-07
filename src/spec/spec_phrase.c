/**
 * @file spec/spec_phrase.c
 * Opt-in command and phrase matching for legacy special procedures.
 */

#include "conf.h"
#include "sysdep.h"

#include "spec/spec_phrase.h"

#include <ctype.h>
#include <string.h>

static const char *spec_phrase_skip_leading_spaces(const char *argument)
{
  while (*argument != '\0' && *argument != '\t' && isspace((unsigned char)*argument))
    argument++;

  return argument;
}

enum spec_phrase_result spec_phrase_match(const char *resolved_command, const char *argument,
                                          const struct spec_phrase_rule *rule)
{
  if (resolved_command == NULL || argument == NULL || rule == NULL || rule->command == NULL ||
      rule->command[0] == '\0' || rule->phrase == NULL || rule->phrase[0] == '\0' ||
      (rule->flags & ~SPEC_PHRASE_ALL) != 0)
    return SPEC_PHRASE_INVALID;

  if (strcmp(resolved_command, rule->command) != 0)
    return SPEC_PHRASE_UNRELATED;

  if (rule->flags & SPEC_PHRASE_SKIP_LEADING_SPACES)
    argument = spec_phrase_skip_leading_spaces(argument);

  return strcmp(argument, rule->phrase) == 0 ? SPEC_PHRASE_MATCHED : SPEC_PHRASE_UNRELATED;
}
