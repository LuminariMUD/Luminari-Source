/**
 * @file magic/spellbook_scroll.h
 * Public API for the wizard spellbook research procedure.
 *
 * All rights reserved. See license for complete information.
 */

#ifndef LUMINARI_MAGIC_SPELLBOOK_SCROLL_H
#define LUMINARI_MAGIC_SPELLBOOK_SCROLL_H

struct char_data;

int wizard_library(struct char_data *ch, void *me, int cmd, const char *argument);

#endif /* LUMINARI_MAGIC_SPELLBOOK_SCROLL_H */
