/**************************************************************************
 *  File: movement_messages.h                          Part of LuminariMUD *
 *  Usage: Header file for movement message display functions.            *
 *                                                                         *
 *  All rights reserved.  See license for complete information.           *
 *                                                                         *
 *  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 **************************************************************************/

#ifndef _MOVEMENT_MESSAGES_H_
#define _MOVEMENT_MESSAGES_H_

/* Function prototypes for movement message display */

/**
 * Display leave messages when a character exits a room
 * @param ch Character who is leaving
 * @param dir Direction they are leaving
 * @param riding TRUE if character is mounted
 * @param ridden_by TRUE if character is being ridden
 */
void display_leave_messages(struct char_data *ch, int dir, int riding, int ridden_by);

/**
 * Display enter messages when a character arrives in a room
 * @param ch Character who is entering
 * @param dir Direction they came from
 * @param riding TRUE if character is mounted
 * @param ridden_by TRUE if character is being ridden
 */
void display_enter_messages(struct char_data *ch, int dir, int riding, int ridden_by);

#endif /* _MOVEMENT_MESSAGES_H_ */