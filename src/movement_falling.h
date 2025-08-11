/**************************************************************************
 *  File: movement_falling.h                          Part of LuminariMUD *
 *  Usage: Falling mechanics functions header                             *
 *                                                                         *
 *  All rights reserved.  See license complete information.               *
 *                                                                         *
 *  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.              *
 **************************************************************************/

#ifndef MOVEMENT_FALLING_H
#define MOVEMENT_FALLING_H

/* Function declarations for falling mechanics */
bool obj_should_fall(struct obj_data *obj);
bool char_should_fall(struct char_data *ch, bool silent);

/* Event function declaration */
EVENTFUNC(event_falling);

#endif /* MOVEMENT_FALLING_H */