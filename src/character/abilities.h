/**
 * @file character/abilities.h
 * Public character ability calculation API.
 *
 * All rights reserved. See license for complete information.
 */

#ifndef LUMINARI_CHARACTER_ABILITIES_H
#define LUMINARI_CHARACTER_ABILITIES_H

#include <stdbool.h>

struct char_data;

int compute_ability(struct char_data *ch, int ability_num);
int compute_ability_full(struct char_data *ch, int ability_num, bool recursive);

#endif /* LUMINARI_CHARACTER_ABILITIES_H */
