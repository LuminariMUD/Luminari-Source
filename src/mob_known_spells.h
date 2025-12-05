/**************************************************************************
 *  File: mob_known_spells.h                          Part of LuminariMUD *
 *  Usage: Header file for mob known spell slot tracking system.          *
 *                                                                         *
 *  All rights reserved.  See license for complete information.            *
 **************************************************************************/

#ifndef _MOB_KNOWN_SPELLS_H_
#define _MOB_KNOWN_SPELLS_H_

/* Function prototypes */

/* Initialize known spell slots for a mob (max 2 per known spell) */
void init_known_spell_slots(struct char_data *ch);

/* Check if mob has an available slot for the given known spell */
bool has_known_spell_slot(struct char_data *ch, int spellnum);

/* Consume a known spell slot when mob casts a known spell */
void consume_known_spell_slot(struct char_data *ch, int spellnum);

/* Regenerate one random known spell slot for a mob (1 per minute out of combat) */
void regenerate_known_spell_slot(struct char_data *ch);

/* Categorize a known spell (buff, heal, offensive, summon, etc.) */
int categorize_known_spell(int spellnum);

#define KNOWN_SPELL_CATEGORY_BUFF     0
#define KNOWN_SPELL_CATEGORY_HEAL     1
#define KNOWN_SPELL_CATEGORY_OFFENSIVE 2
#define KNOWN_SPELL_CATEGORY_SUMMON   3
#define KNOWN_SPELL_CATEGORY_UTILITY  4

#endif
