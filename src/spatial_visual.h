/*************************************************************************
*   File: spatial_visual.h                             Part of LuminariMUD *
*  Usage: Visual stimulus system declarations                              *
*  Author: Luminari Development Team                                       *
*                                                                          *
*  All rights reserved.  See license for complete information.            *
*                                                                          *
*  LuminariMUD is based on CircleMUD, Copyright (C) 1993, 94 by the       *
*  Department of Computer Science at the Johns Hopkins University         *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#ifndef _SPATIAL_VISUAL_H_
#define _SPATIAL_VISUAL_H_

#include "spatial_core.h"

/* Visual System Strategies */
extern struct stimulus_strategy visual_stimulus_strategy;
extern struct los_strategy physical_los_strategy;
extern struct modifier_strategy weather_terrain_modifier_strategy;

/* Visual System */
extern struct spatial_system visual_system;

/* Functions */
int spatial_visual_init(void);

/* Convenience functions for visual events */
int process_ship_visual_event(int ship_x, int ship_y, const char *ship_description);
int process_flying_object_visual_event(int x, int y, int altitude, const char *description);
int process_distant_structure_visual_event(int x, int y, const char *structure_description);

/* Testing functions */
int spatial_visual_test_ship_passing(int ship_x, int ship_y, const char *ship_desc);

#endif /* _SPATIAL_VISUAL_H_ */
