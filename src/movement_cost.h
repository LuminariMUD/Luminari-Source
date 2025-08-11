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

#endif /* MOVEMENT_COST_H */