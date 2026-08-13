/**
 * @file spec/spec_rol_avernus.h
 * Typed runtime adapters for converted Realms of Luminari Avernus procedures.
 */

#ifndef LUMINARI_SPEC_ROL_AVERNUS_H
#define LUMINARI_SPEC_ROL_AVERNUS_H

#include <stdbool.h>
#include <stddef.h>

struct char_data;
struct spec_event_context;

int rol_avernus_object(struct char_data *ch, void *me, int cmd, const char *argument);
int rol_avernus_object_typed(struct spec_event_context *context);
int rol_avernus_garden(struct char_data *ch, void *me, int cmd, const char *argument);
int rol_avernus_garden_typed(struct spec_event_context *context);

/** Schedule the one authored Avernus garden room binding on the mobile pulse. */
void rol_avernus_room_pulse(void);

/** Compose Avernus lifecycle behavior with the shared monster-combat profile. */
int rol_avernus_mobile_event(struct spec_event_context *context, struct char_data *mobile);

/** Pure profile helpers used by conversion regression tests. */
size_t rol_avernus_mobile_profile_count(void);
bool rol_avernus_mobile_profile(int mobile_vnum, bool *commands, bool *activity, bool *death);
size_t rol_avernus_object_profile_count(void);
bool rol_avernus_object_profile(int object_vnum, bool *commands, bool *pulse, bool *weapon_hit);
int rol_avernus_patrol_direction(int mobile_vnum, int room_vnum);
bool rol_avernus_garden_room_vnum(int room_vnum);

#endif /* LUMINARI_SPEC_ROL_AVERNUS_H */
