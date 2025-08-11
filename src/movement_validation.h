/**************************************************************************
 *  File: movement_validation.h                       Part of LuminariMUD *
 *  Usage: Movement validation functions header                           *
 *                                                                         *
 *  All rights reserved.  See license complete information.               *
 *                                                                         *
 *  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.              *
 **************************************************************************/

#ifndef MOVEMENT_VALIDATION_H
#define MOVEMENT_VALIDATION_H

/* Function declarations for movement validation */
int has_boat(struct char_data *ch, room_rnum going_to);
int has_flight(struct char_data *ch);
int has_scuba(struct char_data *ch, room_rnum destination);
int can_climb(struct char_data *ch);

/* Single-file room functions */
bool is_top_of_room_for_singlefile(struct char_data *ch, int dir);
struct char_data *get_char_ahead_of_me(struct char_data *ch, int dir);

#endif /* MOVEMENT_VALIDATION_H */