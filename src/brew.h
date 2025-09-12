/*
 * Enhanced Brewing System Header for LuminariMUD
 * Supports multi-spell potions with skill checks and scaling costs
 */

#ifndef BREW_H
#define BREW_H

#include "structs.h"
#include "utils.h"

/* Function prototypes */
int get_mote_type_for_school(int school);
int get_motes_required_for_spell(int spell_circle);
int get_gold_cost_for_spell(int spell_circle);
int can_cast_spell(struct char_data *ch, int spellnum);
bool can_alchemist_brew_spell(struct char_data *ch, int spellnum);
int get_spell_circle(int spellnum);
struct obj_data *create_potion(int spell_num, struct char_data *ch);
struct obj_data *create_multi_spell_potion(int *spell_nums, int num_spells, struct char_data *ch);

ACMD_DECL(do_brew);
EVENTFUNC(event_brewing);

#endif /* BREW_H */
