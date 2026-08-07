/**
 * @file spec/spec_phrase.h
 * Opt-in command and phrase matching for legacy special procedures.
 *
 * All rights reserved. See license for complete information.
 */

#ifndef LUMINARI_SPEC_PHRASE_H
#define LUMINARI_SPEC_PHRASE_H

#include <stdint.h>

typedef uint32_t spec_phrase_flag_mask;
enum spec_phrase_flag
{
  SPEC_PHRASE_EXACT = 0,
  SPEC_PHRASE_SKIP_LEADING_SPACES = (1U << 0)
};

#define SPEC_PHRASE_ALL SPEC_PHRASE_SKIP_LEADING_SPACES

struct spec_phrase_rule
{
  const char *command;
  const char *phrase;
  spec_phrase_flag_mask flags;
};

enum spec_phrase_result
{
  SPEC_PHRASE_UNRELATED = 0,
  SPEC_PHRASE_MATCHED,
  SPEC_PHRASE_INVALID
};

/**
 * Compare an already-resolved canonical command and its untouched argument.
 *
 * Matching remains byte-for-byte case and punctuation sensitive. The only
 * optional normalization is the legacy skip_spaces() leading-space policy;
 * tabs, trailing whitespace, and punctuation are deliberately retained.
 * MATCHED means handled, UNRELATED means the caller should continue, and
 * INVALID reports a malformed rule or input contract.
 */
enum spec_phrase_result spec_phrase_match(const char *resolved_command, const char *argument,
                                          const struct spec_phrase_rule *rule);

#endif /* LUMINARI_SPEC_PHRASE_H */
