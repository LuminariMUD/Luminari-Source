/**
 * @file vessels_legacy.h
 * Public API for legacy route and Greyhawk vessel procedures.
 */

#ifndef LUMINARI_VESSELS_LEGACY_H
#define LUMINARI_VESSELS_LEGACY_H

#include "core/structs.h"

struct char_data;

int alandor_ferry(struct char_data *ch, void *me, int cmd, const char *argument);
int chionthar_ferry(struct char_data *ch, void *me, int cmd, const char *argument);
int greyhawk_ship_commands(struct char_data *ch, void *me, int cmd, const char *argument);
int greyhawk_ship_object(struct char_data *ch, void *me, int cmd, const char *argument);
int md_carpet(struct char_data *ch, void *me, int cmd, const char *argument);


void update_ship(struct obj_data *ship, room_vnum start, room_vnum end, int movedelay,
                 int waitdelay);
#endif /* LUMINARI_VESSELS_LEGACY_H */
