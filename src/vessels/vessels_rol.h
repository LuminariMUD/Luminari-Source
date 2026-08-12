/**
 * @file vessels_rol.h
 * Public API for the converted Realms of Luminari fixed-interior ships.
 */

#ifndef LUMINARI_VESSELS_ROL_H
#define LUMINARI_VESSELS_ROL_H

#include "structs.h"

int rol_ship(struct char_data *ch, void *me, int cmd, const char *argument);
int rol_ship_control(struct char_data *ch, void *me, int cmd, const char *argument);
int rol_ship_exit(struct char_data *ch, void *me, int cmd, const char *argument);
int rol_ship_lookout(struct char_data *ch, void *me, int cmd, const char *argument);
int rol_ship_navigator(struct char_data *ch, void *me, int cmd, const char *argument);

void rol_ship_activity(void);

/* Stable inspection helpers used by production-linked conversion tests. */
int rol_ship_definition_count(void);
bool rol_ship_interior_contains(int ship_index, room_vnum room);
int rol_ship_move_delay_for_speed(int speed);
bool rol_ship_can_enter_sector(int sector, bool dockable);

#endif /* LUMINARI_VESSELS_ROL_H */
