/**************************************************************************
 *  File: mob_memory.h                                Part of LuminariMUD *
 *  Usage: Header for mobile memory management functions                  *
 *                                                                         *
 *  All rights reserved.  See license for complete information.           *
 *                                                                         *
 *  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.              *
 **************************************************************************/

#ifndef _MOB_MEMORY_H_
#define _MOB_MEMORY_H_

/* Function prototypes for mob memory management */

/* checks if vict is in memory of ch */
bool is_in_memory(struct char_data *ch, struct char_data *vict);

/* make ch remember victim */
void remember(struct char_data *ch, struct char_data *victim);

/* make ch forget victim */
void forget(struct char_data *ch, struct char_data *victim);

/* erase ch's memory completely, also freeing memory */
void clearMemory(struct char_data *ch);

#endif /* _MOB_MEMORY_H_ */