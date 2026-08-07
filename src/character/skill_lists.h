/**
 * @file character/skill_lists.h
 * Public skill prerequisite, listing, and training API.
 *
 * All rights reserved. See license for complete information.
 */

#ifndef LUMINARI_CHARACTER_SKILL_LISTS_H
#define LUMINARI_CHARACTER_SKILL_LISTS_H

#include "magic/spells.h"

struct char_data;

#define ABILITY_TYPE_ALL 0
#define ABILITY_TYPE_GENERAL 1
#define ABILITY_TYPE_CRAFT 2
#define ABILITY_TYPE_KNOWLEDGE 3

extern const char *cross_names[];

int meet_skill_reqs(struct char_data *ch, int skillnum);
void list_crafting_skills(struct char_data *ch);
void list_skills(struct char_data *ch);
void list_abilities(struct char_data *ch, int ability_type);
void process_skill(struct char_data *ch, int skillnum);

#endif /* LUMINARI_CHARACTER_SKILL_LISTS_H */
