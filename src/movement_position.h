/**************************************************************************
 *  File: movement_position.h                         Part of LuminariMUD *
 *  Usage: Character position change functions header                     *
 *                                                                         *
 *  All rights reserved.  See license complete information.               *
 *                                                                         *
 *  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.              *
 **************************************************************************/

#ifndef MOVEMENT_POSITION_H
#define MOVEMENT_POSITION_H

/* Function declarations for position changes */
bool can_stand(struct char_data *ch);
int change_position(struct char_data *ch, int new_position);

/* ACMD declarations for position commands */
ACMD_DECL(do_stand);
ACMD_DECL(do_sit);
ACMD_DECL(do_rest);
ACMD_DECL(do_sleep);
ACMD_DECL(do_wake);
ACMD_DECL(do_recline);

#endif /* MOVEMENT_POSITION_H */