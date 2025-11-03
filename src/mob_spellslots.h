/**************************************************************************
 *  File: mob_spellslots.h                           Part of LuminariMUD *
 *  Usage: Header file for mob spell slot tracking system.                 *
 *                                                                         *
 *  All rights reserved.  See license for complete information.            *
 **************************************************************************/

#ifndef _MOB_SPELLSLOTS_H_
#define _MOB_SPELLSLOTS_H_

/* Function prototypes */

/* Calculate the spell circle for a given spell and class */
int get_spell_circle(int spellnum, int char_class);

/* Initialize spell slots for a mob based on its class and level */
void init_mob_spell_slots(struct char_data *ch);

/* Check if mob has an available spell slot for the given spell */
bool has_spell_slot(struct char_data *ch, int spellnum);

/* Check if mob has sufficient spell slots for buffing (> 50% remaining) */
bool has_sufficient_slots_for_buff(struct char_data *ch, int spellnum);

/* Consume a spell slot when mob casts a spell */
void consume_spell_slot(struct char_data *ch, int spellnum);

/* Regenerate one random spell slot for a mob */
void regenerate_mob_spell_slot(struct char_data *ch);

/* Display spell slot information for a mob (for admin commands) */
void show_mob_spell_slots(struct char_data *ch, struct char_data *mob);

#endif /* _MOB_SPELLSLOTS_H_ */
