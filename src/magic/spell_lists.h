/**
 * @file magic/spell_lists.h
 * Public spell sorting and listing API.
 *
 * All rights reserved. See license for complete information.
 */

#ifndef LUMINARI_MAGIC_SPELL_LISTS_H
#define LUMINARI_MAGIC_SPELL_LISTS_H

#include "magic/spells.h"

struct char_data;

extern int spell_sort_info[TOP_SKILL_DEFINE];

void sort_spells(void);
void list_spells(struct char_data *ch, int mode, int class_num, int circle);

#endif /* LUMINARI_MAGIC_SPELL_LISTS_H */
