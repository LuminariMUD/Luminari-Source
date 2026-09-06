/**
 * @file vessels/vessels_moving_rooms.h
 * Public API for legacy moving-room loading, scheduling, and relocation.
 *
 * All rights reserved. See license for complete information.
 */

#ifndef LUMINARI_VESSELS_MOVING_ROOMS_H
#define LUMINARI_VESSELS_MOVING_ROOMS_H

#include <stdio.h>

struct char_data;
struct moving_room_data;
struct oldNextMove;

int moving_rooms(struct char_data *ch, void *me, int cmd, const char *argument);
void setup_moving_room(FILE *fl, int rroom, int vroom, char *line);
int prepMovingRoom(struct moving_room_data *theRoom, struct oldNextMove *ONMdata, int *cibIdx,
                   int *nextIdx);
int linkMovingRoom(struct moving_room_data *theRoom, struct oldNextMove *ONMdata, int cibIdx);
int unlinkMovingRoom(struct moving_room_data *theRoom, struct oldNextMove *ONMdata, int cibIdx);

#endif /* LUMINARI_VESSELS_MOVING_ROOMS_H */
