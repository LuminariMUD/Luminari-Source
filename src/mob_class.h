/**************************************************************************
 *  File: mob_class.h                                 Part of LuminariMUD *
 *  Usage: Header for mobile class-specific behavior functions            *
 *                                                                         *
 *  All rights reserved.  See license for complete information.           *
 *                                                                         *
 *  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.              *
 **************************************************************************/

#ifndef _MOB_CLASS_H_
#define _MOB_CLASS_H_

/* Function prototypes for mob class behaviors */

/* main class behavior dispatcher */
void npc_class_behave(struct char_data *ch);

/* individual class behaviors */
void npc_monk_behave(struct char_data *ch, struct char_data *vict,
                     int engaged);
void npc_rogue_behave(struct char_data *ch, struct char_data *vict,
                      int engaged);
void npc_bard_behave(struct char_data *ch, struct char_data *vict,
                     int engaged);
void npc_warrior_behave(struct char_data *ch, struct char_data *vict,
                        int engaged);
void npc_ranger_behave(struct char_data *ch, struct char_data *vict,
                       int engaged);
void npc_paladin_behave(struct char_data *ch, struct char_data *vict,
                        int engaged);
void npc_dragonrider_behave(struct char_data *ch, struct char_data *vict,
                            int engaged);
void npc_berserker_behave(struct char_data *ch, struct char_data *vict,
                          int engaged);

/* ability behaviors */
void npc_ability_behave(struct char_data *ch);

#endif /* _MOB_CLASS_H_ */