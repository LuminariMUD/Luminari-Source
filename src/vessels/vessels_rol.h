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
void rol_ship_activity_one(int ship_index);
void rol_ship_periodic_init(void);
void rol_ship_periodic_shutdown(void);
void rol_ship_note_object_placed(struct obj_data *obj);
void rol_ship_note_object_extracted(struct obj_data *obj);
size_t rol_ship_periodic_loaded_count(void);
size_t rol_ship_periodic_scheduled_count(void);
size_t rol_ship_periodic_validate(void);
uint64_t rol_ship_periodic_callbacks(void);

/* Stable inspection helpers used by production-linked conversion tests. */
int rol_ship_definition_count(void);
bool rol_ship_interior_contains(int ship_index, room_vnum room);
int rol_ship_move_delay_for_speed(int speed);
bool rol_ship_can_enter_sector(int sector, bool dockable);

#endif /* LUMINARI_VESSELS_ROL_H */
