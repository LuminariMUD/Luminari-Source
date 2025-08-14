/**************************************************************************
 *  File: mob_spells.h                                Part of LuminariMUD *
 *  Usage: Header for mobile spell casting functions and data             *
 *                                                                         *
 *  All rights reserved.  See license for complete information.           *
 *                                                                         *
 *  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.              *
 **************************************************************************/

#ifndef _MOB_SPELLS_H_
#define _MOB_SPELLS_H_

/* Defines for spell arrays */
#define OFFENSIVE_SPELLS 60
#define OFFENSIVE_AOE_SPELLS 16
#define OFFENSIVE_AOE_POWERS 6

#if defined(CAMPAIGN_DL)
#define SPELLUP_SPELLS 53
#else
#define SPELLUP_SPELLS 56
#endif

/* External spell data arrays */
extern int valid_spellup_spell[SPELLUP_SPELLS];
extern int valid_aoe_spell[OFFENSIVE_AOE_SPELLS];
extern int valid_aoe_power[OFFENSIVE_AOE_POWERS];
extern int valid_offensive_spell[OFFENSIVE_SPELLS];

/* Function prototypes for mob spell behaviors */

/* handle NPC spell buff behaviors */
void npc_spellup(struct char_data *ch);

/* handle offensive spell casting for NPCs */
void npc_offensive_spells(struct char_data *ch);

/* handle assigned spell casting for NPCs */
void npc_assigned_spells(struct char_data *ch);

/* check if mob knows its assigned spells */
bool mob_knows_assigned_spells(struct char_data *ch);

#endif /* _MOB_SPELLS_H_ */