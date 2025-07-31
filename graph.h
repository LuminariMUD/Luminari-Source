/**
* @file graph.h                                       LuminariMUD
* Header file for Various graph algorithms.
* 
* Part of the core tbaMUD source code distribution, which is a derivative
* of, and continuation of, CircleMUD.
*                                                                        
* All rights reserved.  See license for complete information.                                                                
* Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University 
* CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               
*
* @todo the functions here should perhaps be a part of another module?
*/
#ifndef _GRAPH_H_
#define _GRAPH_H_

/* commands */
//ACMD(do_track);

/* functions */
void hunt_victim(struct char_data *ch);
void hunt_loadroom(struct char_data *ch);
int find_first_step(room_rnum src, room_rnum target);
int count_rooms_between(room_rnum src, room_rnum target);

#endif /* _GRAPH_H_*/
