/**************************************************************************
 *  File: rol_spells.h                                 Part of LuminariMUD *
 *  Usage: Spells converted from Realms of Luminari.                       *
 *                                                                         *
 *  All rights reserved.  See license for complete information.            *
 **************************************************************************/

#ifndef _ROL_SPELLS_H_
#define _ROL_SPELLS_H_

struct char_data;
struct obj_data;

void assign_rol_gap_spells(void);
void cast_rol_gap_spell(int spellnum, int level, struct char_data *ch, struct char_data *victim,
                        struct obj_data *obj, int casttype);

int rol_spell_adjust_area_damage(struct char_data *victim, int damage);
int rol_spell_adjust_incoming_damage(struct char_data *attacker, struct char_data *victim,
                                     int damage);
bool is_rol_gap_spell(int spellnum);

#endif /* _ROL_SPELLS_H_ */
