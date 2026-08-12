/**
 * @file spec/spec_rol_utility_objects.h
 * Converted Realms of Luminari utility-object special procedures.
 */

#ifndef LUMINARI_SPEC_ROL_UTILITY_OBJECTS_H
#define LUMINARI_SPEC_ROL_UTILITY_OBJECTS_H

#include <stdbool.h>

struct char_data;
struct spec_event_context;

int rol_utility_object(struct char_data *ch, void *me, int cmd, const char *argument);
int rol_utility_object_typed(struct spec_event_context *context);

bool rol_utility_sacrifice_keyword(const char *argument);
bool rol_utility_sacrifice_command_name(const char *command);
const char *rol_utility_necro_child_message(int roll);
bool rol_utility_monocle_room(int room_vnum);

#endif /* LUMINARI_SPEC_ROL_UTILITY_OBJECTS_H */
