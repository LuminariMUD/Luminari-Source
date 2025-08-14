/**************************************************************************
 *  File: movement_cost.h                             Part of LuminariMUD *
 *  Usage: Movement cost and speed calculation functions header          *
 *                                                                         *
 *  All rights reserved.  See license complete information.               *
 *                                                                         *
 *  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.              *
 **************************************************************************/

#ifndef MOVEMENT_COST_H
#define MOVEMENT_COST_H

/* Function declarations for movement cost/speed */
int get_speed(struct char_data *ch, sbyte to_display);

/* Movement cost calculation functions */
int calculate_movement_cost(struct char_data *ch, room_rnum from_room, room_rnum to_room, int riding);
bool check_movement_points(struct char_data *ch, int need_movement, int riding, int following);
void deduct_movement_points(struct char_data *ch, int need_movement, int riding, int ridden_by);

/* External functions needed */
extern const char *get_walkto_landmark_name(int index);
extern int walkto_vnum_to_list_row(int vnum);

#endif /* MOVEMENT_COST_H */