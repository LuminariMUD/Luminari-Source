/**************************************************************************
 *  File: mob_psionic.h                               Part of LuminariMUD *
 *  Usage: Header for mobile psionic power functions                      *
 *                                                                         *
 *  All rights reserved.  See license for complete information.           *
 *                                                                         *
 *  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.              *
 **************************************************************************/

#ifndef _MOB_PSIONIC_H_
#define _MOB_PSIONIC_H_

/* Function prototypes for mob psionic behaviors */

/* handle psionic powerup behaviors for NPCs */
void npc_psionic_powerup(struct char_data *ch);

/* handle offensive psionic powers for NPCs */
void npc_offensive_powers(struct char_data *ch);

/* check if a power is valid for psionic spellup */
bool valid_psionic_spellup_power(int powernum);

/* check if a power is valid for psionic combat */
bool valid_psionic_combat_power(int powernum);

#endif /* _MOB_PSIONIC_H_ */