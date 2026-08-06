/**
 * @file spec_menu.h
 * Shared owner-aware special-procedure selection for Oasis editors.
 */

#ifndef LUMINARI_OLC_SPEC_MENU_H
#define LUMINARI_OLC_SPEC_MENU_H

#include "spec/spec_registry.h"

#include <stddef.h>

struct descriptor_data;

/** Result of parsing one builder selection from a filtered menu. */
enum spec_olc_selection_result
{
  SPEC_OLC_SELECTION_INVALID = 0,
  SPEC_OLC_SELECTION_CLEAR,
  SPEC_OLC_SELECTION_DEFINITION
};

/** Return the number of selectable definitions for exactly one owner type. */
size_t spec_olc_menu_count(spec_owner_mask owner);

/** Return a selectable definition by zero-based filtered index, or NULL. */
const struct spec_definition *spec_olc_menu_get(spec_owner_mask owner, int index);

/**
 * Parse a one-based selection for one owner.
 *
 * A zero choice returns SPEC_OLC_SELECTION_CLEAR. A valid positive choice returns
 * SPEC_OLC_SELECTION_DEFINITION and stores the immutable definition. Every other input returns
 * SPEC_OLC_SELECTION_INVALID. The output is cleared before parsing.
 */
enum spec_olc_selection_result spec_olc_parse_selection(spec_owner_mask owner, const char *argument,
                                                        const struct spec_definition **definition);

/** Display the complete filtered definition menu and selection prompt. */
void spec_olc_display_menu(struct descriptor_data *d, spec_owner_mask owner);

#endif /* LUMINARI_OLC_SPEC_MENU_H */
