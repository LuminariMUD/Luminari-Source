/**
* @file spec_procs.h
* Header file for special procedure modules. This file groups a lot of the
* legacy special procedures found in spec_procs.c and zone_procs.c.
* 
* Part of the core tbaMUD source code distribution, which is a derivative
* of, and continuation of, CircleMUD.
*                                                                        
* All rights reserved.  See license for complete information.                                                                
* Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University 
* CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.
* 
*/
#ifndef _SPEC_PROCS_H_
#define _SPEC_PROCS_H_

#include "spells.h"

int spell_sort_info[MAX_SKILLS + 1];
int sorted_spells[MAX_SKILLS + 1];
int sorted_skills[MAX_SKILLS + 1];

/*****************************************************************************
 * Begin Functions and defines for zone_procs.c 
 ****************************************************************************/
void assign_kings_castle(void);
int do_npc_rescue(struct char_data *ch, struct char_data *friend);

/*****************************************************************************
 * Begin Functions and defines for spec_assign.c 
 ****************************************************************************/
void assign_mobiles(void);
void assign_objects(void);
void assign_rooms(void);

/*****************************************************************************
 * Begin Functions and defines for spec_procs.c 
 ****************************************************************************/
/* Utility functions */
void sort_spells(void);
void list_skills(struct char_data *ch);
void list_spells(struct char_data *ch, int mode, int class);
void list_abilities(struct char_data *ch);

int compute_ability(struct char_data *ch, int abilityNum);

/* Special functions */
SPECIAL(guild);
SPECIAL(dump);
SPECIAL(mayor);
SPECIAL(snake);
SPECIAL(thief);
SPECIAL(wizard);
SPECIAL(guild_guard);
SPECIAL(puff);
SPECIAL(fido);
SPECIAL(janitor);
SPECIAL(cityguard);
SPECIAL(pet_shops);
SPECIAL(bank);
SPECIAL(crafting_kit);
SPECIAL(crafting_quest);
SPECIAL(wall);
SPECIAL(hound);

#endif /* _SPEC_PROCS_H_ */
